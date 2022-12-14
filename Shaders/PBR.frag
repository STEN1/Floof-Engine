#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUv;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 fragViewPos;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D diffuseTexture;
layout (set = 0, binding = 1) uniform sampler2D normalsTexture;
layout (set = 0, binding = 2) uniform sampler2D metallicTexture;
layout (set = 0, binding = 3) uniform sampler2D roughnessTexture;
layout (set = 0, binding = 4) uniform sampler2D aoTexture;
layout (set = 0, binding = 5) uniform sampler2D opacityTexture;

struct PointLight {    
    vec4 position;
    vec4 diffuse;
    float intensity;
    float innerRange;
    float outerRange;
    float pad;
};  

layout (std140, set = 1, binding = 0) uniform SceneFrameUBO {
    vec4 cameraPos;
    vec4 sunPosition;
    vec4 sunColor;
    mat4 vp;
    mat4 lightSpaceMatrix[4];
    mat4 view;
    vec4 splitDists;
    float sunStrenght;
    int lightCount;
    int cascadeCount;
    float ambientIntensity;
    float bias;
} sceneFrameUBO;

layout (std140, set = 2, binding = 0) readonly buffer LightSSBO {
    PointLight lights[];
} lightSSBO;

layout (set = 3, binding = 0) uniform samplerCube irradianceMap;
layout (set = 4, binding = 0) uniform samplerCube prefilterMap;
layout (set = 5, binding = 0) uniform sampler2D brdfLut;

layout (set = 6, binding = 0) uniform sampler2DArray shadowMap;

const float PI = 3.14159265359;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
vec3 getNormal();
float textureProjection(vec4 shadowCoord, vec2 offset, int cascadeIndex);
float filterPCF(vec4 sc, int cascadeIndex);
vec3 CalcDirectionalLight(vec3 V, vec3 N, vec3 albedo, float roughness, float metallic, vec3 F0);

void main() {
    vec3 N = getNormal();
    vec3 V = normalize(sceneFrameUBO.cameraPos.xyz - fragPos);

    vec3 albedo = pow(texture(diffuseTexture, fragUv).xyz, vec3(2.2));
    float roughness = texture(roughnessTexture, fragUv).g;
    float metallic = texture(metallicTexture, fragUv).b;
    float ao = texture(aoTexture, fragUv).r;

    int cascadeIndex = 0;
    for (int i = 0; i < sceneFrameUBO.cascadeCount - 1; i++) {
        if (fragViewPos.z < sceneFrameUBO.splitDists[i]) {
            cascadeIndex = i + 1;
        }
    }
    vec4 shadowCoord = (biasMat * sceneFrameUBO.lightSpaceMatrix[cascadeIndex]) * vec4(fragPos, 1.0);
    float shadow = filterPCF(shadowCoord, cascadeIndex);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = CalcDirectionalLight(V, N, albedo, roughness, metallic, F0);
    Lo *= shadow;

    // Point lights
    for(int i = 0; i < sceneFrameUBO.lightCount; ++i) 
    {
        vec3 lighgPos = lightSSBO.lights[i].position.xyz;
        float distance    = length(lighgPos - fragPos);
        if (distance > lightSSBO.lights[i].outerRange) {
            continue;
        }
        vec3 lightColor = lightSSBO.lights[i].diffuse.xyz;
        // calculate per-light radiance
        vec3 L = normalize(lighgPos - fragPos);
        vec3 H = normalize(V + L);
        float attenuation = 1.0 / (distance * distance);
        float t = smoothstep(lightSSBO.lights[i].outerRange, lightSSBO.lights[i].innerRange, distance);
        attenuation *= t;
        vec3 radiance     = lightColor * attenuation * lightSSBO.lights[i].intensity;
        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    vec3 R = reflect(-V, N);
    const float MAX_REFLECTION_LOD = 9.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdfLut, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao * sceneFrameUBO.ambientIntensity;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

    float alpha = texture(diffuseTexture, fragUv).a * texture(opacityTexture, fragUv).r;

    outColor = vec4(color, alpha);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

vec3 getNormal()
{
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = texture(normalsTexture, fragUv).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(fragPos);
	vec3 q2 = dFdy(fragPos);
	vec2 st1 = dFdx(fragUv);
	vec2 st2 = dFdy(fragUv);

	vec3 N = normalize(fragNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

float textureProjection(vec4 shadowCoord, vec2 offset, int cascadeIndex)
{
	float shadow = 1.0;
    float baseBias = sceneFrameUBO.bias * (float(cascadeIndex) + 1.0);
	float bias = max((baseBias * 10.0) * (1.0 - dot(normalize(fragNormal), normalize(sceneFrameUBO.sunPosition.xyz))), baseBias);
    //float bias = sceneFrameUBO.bias;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
		float dist = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = 0.0;
		}
	}
	return shadow;
}

const vec2 PoissonSamples[64] = vec2[64]
(
    vec2(-0.5119625, -0.4827938),
    vec2(-0.2171264, -0.4768726),
    vec2(-0.7552931, -0.2426507),
    vec2(-0.7136765, -0.4496614),
    vec2(-0.5938849, -0.6895654),
    vec2(-0.3148003, -0.7047654),
    vec2(-0.42215, -0.2024607),
    vec2(-0.9466816, -0.2014508),
    vec2(-0.8409063, -0.03465778),
    vec2(-0.6517572, -0.07476326),
    vec2(-0.1041822, -0.02521214),
    vec2(-0.3042712, -0.02195431),
    vec2(-0.5082307, 0.1079806),
    vec2(-0.08429877, -0.2316298),
    vec2(-0.9879128, 0.1113683),
    vec2(-0.3859636, 0.3363545),
    vec2(-0.1925334, 0.1787288),
    vec2(0.003256182, 0.138135),
    vec2(-0.8706837, 0.3010679),
    vec2(-0.6982038, 0.1904326),
    vec2(0.1975043, 0.2221317),
    vec2(0.1507788, 0.4204168),
    vec2(0.3514056, 0.09865579),
    vec2(0.1558783, -0.08460935),
    vec2(-0.0684978, 0.4461993),
    vec2(0.3780522, 0.3478679),
    vec2(0.3956799, -0.1469177),
    vec2(0.5838975, 0.1054943),
    vec2(0.6155105, 0.3245716),
    vec2(0.3928624, -0.4417621),
    vec2(0.1749884, -0.4202175),
    vec2(0.6813727, -0.2424808),
    vec2(-0.6707711, 0.4912741),
    vec2(0.0005130528, -0.8058334),
    vec2(0.02703013, -0.6010728),
    vec2(-0.1658188, -0.9695674),
    vec2(0.4060591, -0.7100726),
    vec2(0.7713396, -0.4713659),
    vec2(0.573212, -0.51544),
    vec2(-0.3448896, -0.9046497),
    vec2(0.1268544, -0.9874692),
    vec2(0.7418533, -0.6667366),
    vec2(0.3492522, 0.5924662),
    vec2(0.5679897, 0.5343465),
    vec2(0.5663417, 0.7708698),
    vec2(0.7375497, 0.6691415),
    vec2(0.2271994, -0.6163502),
    vec2(0.2312844, 0.8725659),
    vec2(0.4216993, 0.9002838),
    vec2(0.4262091, -0.9013284),
    vec2(0.2001408, -0.808381),
    vec2(0.149394, 0.6650763),
    vec2(-0.09640376, 0.9843736),
    vec2(0.7682328, -0.07273844),
    vec2(0.04146584, 0.8313184),
    vec2(0.9705266, -0.1143304),
    vec2(0.9670017, 0.1293385),
    vec2(0.9015037, -0.3306949),
    vec2(-0.5085648, 0.7534177),
    vec2(0.9055501, 0.3758393),
    vec2(0.7599946, 0.1809109),
    vec2(-0.2483695, 0.7942952),
    vec2(-0.4241052, 0.5581087),
    vec2(-0.1020106, 0.672446f)
);

float filterPCF(vec4 sc, int cascadeIndex)
{
	float texDim = textureSize(shadowMap, 0).x;
	float scale = (0.45 * (sceneFrameUBO.cascadeCount - cascadeIndex + 1)) / texDim;

	float shadowFactor = 0.0;

    for (int i = 0; i < 64; i += 2) {
        shadowFactor += textureProjection(sc, PoissonSamples[i] * scale, cascadeIndex);
    }

	return shadowFactor / 64;
}

vec3 CalcDirectionalLight(vec3 V, vec3 N, vec3 albedo, float roughness, float metallic, vec3 F0) {
    vec3 Lo = vec3(0.0);
    vec3 lightColor = sceneFrameUBO.sunColor.xyz;
    // calculate per-light radiance
    vec3 L = normalize(sceneFrameUBO.sunPosition.xyz);
    vec3 H = normalize(V + L);
    vec3 radiance     = lightColor * sceneFrameUBO.sunStrenght;     
        
    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);        
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  
        
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;  
            
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);                
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    return Lo;
}
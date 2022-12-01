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
    vec4 ambient;
    float linear;
    float quadratic;
    float lightRange;
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
} sceneFrameUBO;

layout (std140, set = 2, binding = 0) readonly buffer LightSSBO {
    PointLight lights[];
} lightSSBO;

layout (set = 3, binding = 0) uniform samplerCube irradianceMap;
layout (set = 4, binding = 0) uniform samplerCube prefilterMap;
layout (set = 5, binding = 0) uniform sampler2D brdfLut;

layout (set = 6, binding = 0) uniform sampler2DArray shadowMap;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
vec3 getNormal();
float ShadowCalculation(vec4 shadowCoord, int cascadeIndex);
float textureProjection(vec4 shadowCoord, vec2 offset, int cascadeIndex);
float filterPCF(vec4 sc, int cascadeIndex);

const float PI = 3.14159265359;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

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

    vec3 Lo = vec3(0.0);

    // Point lights
    for(int i = 0; i < sceneFrameUBO.lightCount; ++i) 
    {
        vec3 lighgPos = lightSSBO.lights[i].position.xyz;
        vec3 lightColor = lightSSBO.lights[i].diffuse.xyz;
        // calculate per-light radiance
        vec3 L = normalize(lighgPos - fragPos);
        vec3 H = normalize(V + L);
        float distance    = length(lighgPos - fragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = lightColor * attenuation * lightSSBO.lights[i].lightRange;      
        
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

    vec3 ambient = (kD * diffuse + specular) * ao * max(shadow + 0.5, 1.0);
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 


//    vec4 projCoords = fragPosLightSpace;// / fragPosLightSpace.w;
//    shadow = textureProjection(projCoords);
//    outColor = vec4(shadow, projCoords.z, 0.0, 1.0);
    //outColor = fragPosLightSpace;
    //outColor = texture(shadowMap, projCoords.xy);
    //outColor = vec4(texture(shadowMap, projCoords.xy).x, 1.0, projCoords.z, 1.0);
    //outColor = vec4(brdf, 0.0, 1.0);
    outColor = vec4(color, texture(diffuseTexture, fragUv).a * (1.0 - texture(opacityTexture, fragUv).r));
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
	float bias = 0.0005;

	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
		float dist = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = 0.0;
		}
	}
	return shadow;
}


float filterPCF(vec4 sc, int cascadeIndex)
{
	ivec2 texDim = textureSize(shadowMap, 0).xy;
	float scale = 0.75;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadowFactor += textureProjection(sc, vec2(dx*x, dy*y), cascadeIndex);
			count++;
		}
	}
	return shadowFactor / count;
}

float ShadowCalculation(vec4 shadowCoord, int cascadeIndex)
{
    // perform perspective divide
    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    projCoords = vec3(projCoords.xy * 0.5 + 0.5, projCoords.z);
    float closestDepth = texture(shadowMap, vec3(projCoords.xy, cascadeIndex)).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(sceneFrameUBO.sunPosition.xyz - fragPos);
    //float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.0005);
    float bias = 0.0005;
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, cascadeIndex).xy;
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, vec3((projCoords.xy + vec2(x, y) * texelSize), cascadeIndex)).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}  
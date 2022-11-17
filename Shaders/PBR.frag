#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUv;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D diffuseTexture;
layout (set = 0, binding = 1) uniform sampler2D normalsTexture;
layout (set = 0, binding = 2) uniform sampler2D metallicTexture;
layout (set = 0, binding = 3) uniform sampler2D roughnessTexture;
layout (set = 0, binding = 4) uniform sampler2D aoTexture;

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
    vec4 sunDirection;
    vec4 sunColor;
    float sunStrenght;
    int lightCount;
} sceneFrameUBO;

layout (std140, set = 2, binding = 0) readonly buffer LightSSBO {
    PointLight lights[];
} lightSSBO;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 getNormal();

const float PI = 3.14159265359;

void main() {
    vec3 skyColor = sceneFrameUBO.sunColor.xyz;
    vec3 skyDir = normalize(sceneFrameUBO.sunDirection.xyz);

    vec3 N = getNormal();
    vec3 V = normalize(sceneFrameUBO.cameraPos.xyz - fragPos);

    vec3 albedo = pow(texture(diffuseTexture, fragUv).xyz, vec3(2.2));
    float roughness = texture(roughnessTexture, fragUv).r;
    float metallic = texture(metallicTexture, fragUv).r;
    float ao = texture(aoTexture, fragUv).r;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 Lo = vec3(0.0);

    // Skylight
    // calculate per-light radiance
    vec3 L = normalize(skyDir);
    vec3 H = normalize(V + L);
    vec3 radiance     = skyColor * sceneFrameUBO.sunStrenght;        
        
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
  
    vec3 ambient = vec3(0.01) * albedo * ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  

    outColor = vec4(color, 1.0);
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
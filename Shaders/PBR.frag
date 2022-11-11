#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUv;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D diffuseTexture;

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
    vec3 cameraPos;
    int lightCount;
    float roughness;
    float metallic;
    float ao;
    float pad;
} sceneFrameUBO;

layout (std140, set = 2, binding = 0) readonly buffer LightSSBO {
    PointLight lights[];
} lightSSBO;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

const float PI = 3.14159265359;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(sceneFrameUBO.cameraPos - fragPos);

    vec3 F0 = vec3(0.04);
    vec3 albedo = texture(diffuseTexture, fragUv).xyz;
    F0 = mix(F0, albedo, sceneFrameUBO.metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < sceneFrameUBO.lightCount; ++i) 
    {
        // calculate per-light radiance
        vec3 fragToLight = lightSSBO.lights[i].position.xyz - fragPos;
        vec3 L = normalize(fragToLight);
        vec3 H = normalize(V + L);
        float distance    = length(fragToLight);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = lightSSBO.lights[i].diffuse.xyz * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, sceneFrameUBO.roughness);        
        float G   = GeometrySmith(N, V, L, sceneFrameUBO.roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);    
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - sceneFrameUBO.metallic;
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    vec3 ambient = vec3(0.03) * albedo * sceneFrameUBO.ao;
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
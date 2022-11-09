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
} sceneFrameUBO;

layout (std140, set = 2, binding = 0) readonly buffer LightSSBO {
    PointLight lights[];
} lightSSBO;

void main() {
    vec3 viewDir = normalize(sceneFrameUBO.cameraPos - fragPos);
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(vec3(0.0,1.0,2.0));

    float ambientStrength = 0.1;
    vec3 lightColor = normalize(vec3(0.4, 0.4, 0.4));

    float diff = max(dot(lightDir, normal), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float shininess = 1.0;
    float specular = pow(max(dot(normal, halfwayDir), 0.0), 16.0) * shininess;

    vec3 ambient = lightColor * ambientStrength;
    vec3 diffuse = lightColor * diff;

    for (int i = 0; i < sceneFrameUBO.lightCount; i++) {
        float dist = length(lightSSBO.lights[i].position.xyz - fragPos);
        vec3 lightDir = normalize(lightSSBO.lights[i].position.xyz - fragPos);
        // diffuse shading amount.
        float diff = max(dot(lightDir, normal), 0.0);
        // specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
        // attenuation.
        float attenuation = 1.0 / (1.0 + lightSSBO.lights[i].linear * dist + 
  			            lightSSBO.lights[i].quadratic * (dist * dist));    
        // combine results.
        ambient += lightSSBO.lights[i].ambient.xyz * attenuation;
        diffuse += lightSSBO.lights[i].diffuse.xyz * attenuation * diff;
        specular += shininess * spec * attenuation;
    }
    vec3 lightResult = ambient + diffuse + (specular * vec3(1.0));
    vec4 text = texture(diffuseTexture, fragUv);

    outColor = text * vec4(lightResult, 1.0);
}
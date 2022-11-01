#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUv;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D diffuseTexture;

void main() {
    float ambientStrength = 0.1;
    vec3 lightDir = normalize(vec3(0.0,1.0,2.0));
    vec3 lightColor = normalize(vec3(1.0, 1.0, 1.0));

    //diffuse
    float diff = max(dot(lightDir, fragNormal), 0.0);
    vec3 diffuse = lightColor * (diff + ambientStrength);

    vec4 text = texture(diffuseTexture, fragUv);

    outColor = text * vec4(diffuse, 1.0);
}
#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;


void main() {
    float ambientStrength = 0.1;
    vec3 lightDir = normalize(vec3(0.0,1.0,2.0));
    vec3 lightColor = normalize(vec3(1.0, 1.0, 1.0));

    //diffuse
    float diff = max(dot(lightDir, fragNormal), 0.0);
    vec3 diffuse = lightColor * (diff + ambientStrength);

    outColor = fragColor * vec4(diffuse, 1.0);
}
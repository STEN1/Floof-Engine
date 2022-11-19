#version 450

layout(location = 0) in vec3 fragUv;

layout (set = 0, binding = 0) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 envColor = texture(skybox, fragUv).rgb;

    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2));

    outColor = vec4(envColor, 1.0);
}
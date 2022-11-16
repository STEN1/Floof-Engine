#version 450

layout(location = 0) in vec3 fragUv;

layout (set = 0, binding = 0) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(skybox, fragUv);
}
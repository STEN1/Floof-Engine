#version 450

layout(location = 0) in vec2 fragUv;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D diffuseTexture;

void main() {
    outColor = texture(diffuseTexture, fragUv);
}
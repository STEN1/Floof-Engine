#version 450

layout(location = 0) in vec2 fragUv;

layout (set = 1, binding = 0) uniform sampler2D diffuseTexture;
layout (set = 1, binding = 1) uniform sampler2D normalsTexture;
layout (set = 1, binding = 2) uniform sampler2D metallicTexture;
layout (set = 1, binding = 3) uniform sampler2D roughnessTexture;
layout (set = 1, binding = 4) uniform sampler2D aoTexture;
layout (set = 1, binding = 5) uniform sampler2D opacityTexture;

void main() {
    float alpha = texture(diffuseTexture, fragUv).a * texture(opacityTexture, fragUv).r;
    if (alpha < 0.4)
        discard;
}
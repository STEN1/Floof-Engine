#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragUv;

void main() {
    outColor = vec4(fragUv, 0.0, 1.0);
}
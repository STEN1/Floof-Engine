#version 450

layout(location = 0) in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) uniform sampler2D offscreenTexture;

void main() {
    FragColor = vec4(texture(offscreenTexture, TexCoords).rgb, 1.0);
}
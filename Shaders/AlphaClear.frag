#version 450

layout(location = 0) in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) uniform sampler2D offscreenTexture;

void main() {
    vec2 uv = vec2(TexCoords.x, 1.0 - TexCoords.y);
    FragColor = vec4(texture(offscreenTexture, uv).rgb, 1.0);
}
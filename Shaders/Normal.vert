#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 imodel;
} pushConstants;

void main() {
    gl_Position = pushConstants.mvp * vec4(pos, 1.0);
    fragColor = normalize(mat3(transpose(pushConstants.imodel)) * normal);
}
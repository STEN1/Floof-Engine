#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 imodel;
} pushConstants;


void main() {
    gl_Position = pushConstants.mvp * vec4(pos, 1.0);
    fragColor = vec4(color, 1.0);
    fragNormal = normalize(mat3(transpose(pushConstants.imodel)) * normal);
}
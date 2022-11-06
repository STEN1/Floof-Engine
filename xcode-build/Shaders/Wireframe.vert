#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitTangent;

layout(push_constant) uniform PushConstants {
    mat4 vp;
    mat4 model;
    mat4 imodel;
} pushConstants;

void main() {
    gl_Position = pushConstants.vp * pushConstants.model * vec4(pos, 1.0);
}
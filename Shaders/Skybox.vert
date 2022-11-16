#version 450

layout(location = 0) in vec3 pos;

layout(location = 0) out vec3 fragUv;

layout(push_constant) uniform PushConstants {
    mat4 vp;
    mat4 model;
    mat4 imodel;
} pushConstants;

void main() {
    fragUv = pos;
    gl_Position = pushConstants.vp * vec4(pos, 1.0);
}
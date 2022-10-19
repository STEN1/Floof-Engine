#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 MVP;
} pushConstants;


void main() {
    gl_PointSize = 1.0;
    gl_Position = pushConstants.MVP * vec4(pos, 1.0);
    fragColor = vec4(color, 1.0);
}
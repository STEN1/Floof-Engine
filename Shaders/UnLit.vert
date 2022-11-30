#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec2 fragUv;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 imodel;
} pushConstants;

layout (std140, set = 1, binding = 0) uniform SceneFrameUBO {
    vec4 cameraPos;
    vec4 sunDirection;
    vec4 sunColor;
    mat4 vp;
    mat4 lightSpaceMatrix;
    float sunStrenght;
    int lightCount;
} sceneFrameUBO;

void main() {
    gl_Position = sceneFrameUBO.vp * pushConstants.model * vec4(pos, 1.0);
    fragUv = uv;
}
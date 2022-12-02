#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 imodel;
    int cascadeIndex;
} pushConstants;

layout (std140, set = 0, binding = 0) uniform SceneFrameUBO {
    vec4 cameraPos;
    vec4 sunPosition;
    vec4 sunColor;
    mat4 vp;
    mat4 lightSpaceMatrix[4];
    mat4 view;
    vec4 splitDists;
    float sunStrenght;
    int lightCount;
    int cascadeCount;
} sceneFrameUBO;

void main() {
    gl_Position = sceneFrameUBO.lightSpaceMatrix[pushConstants.cascadeIndex] * pushConstants.model * vec4(pos, 1.0);
}
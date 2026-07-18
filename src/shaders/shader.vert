#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
} ubo;

layout(push_constant) uniform Constants {
    mat4 view;
    mat4 proj;
} camera;

void main() {
    gl_Position = camera.proj * camera.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
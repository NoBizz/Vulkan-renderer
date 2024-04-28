#version 450

layout(binding = 0) uniform UniformBufferObj::UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} object;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 textureCoordinates;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTextureCoordinates
void main() {
    gl_Position = object.proj * object.view * object.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTextureCoordinates = textureCoordinates;
}
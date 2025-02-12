#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragTexCoord;

void main() {
	// mat4 tra = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0.25, 0.25, 1, 0, 0, 0, 0, 1);
    // gl_Position = tra * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition);
    // gl_Position = inPosition;
    fragColor = inColor;
	//fragTexCoord = inTexCoord;
}

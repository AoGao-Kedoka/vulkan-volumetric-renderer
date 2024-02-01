#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D storageTexture;

layout(location = 0) in vec2 TexCoord;

layout(location = 0) out vec4 finalColor;

void main(){
    finalColor = texture(storageTexture, TexCoord);
}

#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(binding = 0) uniform sampler2D storageTexture;
//layout(binding = 1) uniform sampler2D causticTexture;

layout(location = 0) in vec2 TexCoord;

layout(location = 0) out vec4 finalColor;

void main(){
    // finalColor = vec4(texture(storageTexture, TexCoord).rbg, 1.0);
    finalColor = vec4(0.3,0.3,0.3,1.0);
}

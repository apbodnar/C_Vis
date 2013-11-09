#version 430

layout (binding=0) uniform sampler2D tex;  //particle texture

out vec4 fragcolor; // the final color of the fragment
in vec4 velocity2;
in vec2 TexCoord;

uniform float tick;  //incremented value used to translate texcoords

void main() { 
		fragcolor = abs(normalize(velocity2))*texture(tex,TexCoord)*2;
} 

#version 430

layout (binding=0) uniform sampler2D tex;  //particle texture

out vec4 fragcolor; // the final color of the fragment
in vec2 TexCoord;
in vec3 dir;
in float mag;
flat in float vertex_g;
uniform int mode;

void main() { 
	float dark = float(vertex_g)/16;
	
	if(mode == 0)
		fragcolor = (TexCoord.x > 0.8 || TexCoord.x < 0.2 ? vec4(0,0,0,0) : vec4(dir,0));
	else
		fragcolor = (TexCoord.x > 0.85 || TexCoord.x < 0.15 ? vec4(0,0,0,0) : vec4(mag/150,0,1-mag/150,0));
		
	//fragcolor = vec4(dir,1);
	//fragcolor = vec4(mag/150,0,1-mag/150,0));
} 

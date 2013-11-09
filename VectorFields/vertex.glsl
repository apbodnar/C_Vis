#version 430

layout(location=0) in vec3 coord;
uniform float tick;
out vec3 texcoords;

void main() {   
	texcoords = coord;
	gl_Position = vec4(coord,1);
} 

#version 430

layout(location=0) in vec4 coord;
layout(location=1) in vec4 velocity;

uniform mat4 MVM;
uniform mat4 PM;
uniform int mode;

out float vertex_v;
out vec4 vel;

void main() {
	vertex_v = mod(float(gl_VertexID), 32.0);
	vel = velocity;
	vec4 WorldCoord = MVM * coord;
	gl_Position = PM * WorldCoord;
} 

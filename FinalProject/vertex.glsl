#version 430

layout(location=0) in vec4 coord;
layout(location=1) in vec4 vel;

uniform float tick;
uniform mat4 MVM;
uniform mat4 PM;
out vec4 velocity1;
out vec4 points;

void main() {   
	velocity1 = vel;
	vec4 WorldCoord = MVM * coord;
	gl_Position = PM * WorldCoord;
} 

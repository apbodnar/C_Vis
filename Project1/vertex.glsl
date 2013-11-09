#version 420

// ------------------- INPUT VARIABLES ----------------------

// these are coordinates of a unit cube - all in {0,1}
layout(location=0) in vec3 coord;
layout(location=1) in vec3 vel;

out vec3 world;   // world coordinates of a vertex 
out vec3 boundvec;
// ------------------- UNIFORM VARIABLES -----------------------

uniform mat3 R;         // rotation matrix (defined by the trackball)
uniform mat4 P;         // projection matrix
uniform float fwd;      // amount of forward translation with respect to camera

void main() {   
	boundvec = coord*R;
	world = 1.0/4.0*(coord)-vec3(0,0,fwd);
	gl_Position = P*vec4(world,1);
} 

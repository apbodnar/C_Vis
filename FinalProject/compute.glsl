#version 430
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable


uniform float tick;
uniform int mode; 
uniform vec3 center;

layout( std430, binding=0 ) buffer Pos{
	vec4 position[]; // array of structures
};
layout( std430, binding=1 ) buffer Vel{
	vec4 velocity[]; // array of structures
};
layout( std430, binding=2 ) buffer Prev{
	vec4 prev_position[]; // array of structures
};

layout (local_size_x = 1024, local_size_y = 1,local_size_z = 1) in;


void main(){
	
	float G = 30.0;
	uint gid = gl_GlobalInvocationID.x;
	vec3 p = position[gid].xyz;
	if(mode == 0){
	float deltaT = 0.1;
		float r = distance(center, p);  // euler integration on gravitational potential
		vec3 a = (G/r/r) * normalize(center - p);
		prev_position[gid] = position[gid];
		velocity[gid].xyz = a*deltaT + velocity[gid].xyz;
		position[gid].xyz = velocity[gid].xyz*deltaT + p;
	}
	else{
		float deltaT = 0.01;
		vec3 pt;
		vec3 a = center - p;  //verlet integration on a spring potential
		a = normalize(a)*3*length(p.xyz);
		pt	= 2*p - 1*prev_position[gid].xyz + a*deltaT*deltaT;
		prev_position[gid].xyz = p;
		position[gid].xyz = pt;
		velocity[gid].xyz = (pt-p);
	}
	
}
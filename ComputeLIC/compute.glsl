#version 430
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable


uniform int tick;
uniform float rev;
uniform int mode; 
uniform int animate; 

layout (binding=0) uniform sampler3D tex;

layout( std430, binding=0 ) buffer Pos{
	vec4 position[];
};
layout( std430, binding=1 ) buffer Vel{
	vec4 velocity[];
};
//local size doesn't need to equal the number of lines, just a factor of it.
layout (local_size_x = 512, local_size_y = 1,local_size_z = 1) in;

bool in_bounds(vec4 c){
	float bound = 19.9;
	return !(c.x > bound || c.x < -bound || c.y > bound || c.y < -bound || c.z > bound || c.z < -bound);
}

vec4 tex_func(vec4 p){ 
	
	//texcoords in compute shader are still clamped
	vec3 alt = texture(tex, (p.xyz+20)/40).xyz;
	//return  dT*vec4(alt/(30.0+ length(alt)),0);
	return vec4((alt),0);
	
}

//vec2 RK4(vec2 xy){
//	float x0 = xy.x;
//	float y0 = xy.y;
//	vec2 k1,k2,k3,k4,k;
//	k1=deltaT*(func(vec2(x0,y0)));
//	k2=deltaT*(func(vec2((x0+(deltaT/2)),y0+(k1/2))));
//	k3=deltaT*(func(vec2((x0+(deltaT/2)),y0+(k2/2))));
//	k4=deltaT*(func(vec2((x0+deltaT),(y0+k3))));
//	k=(1/6.0)*(k1+(2*k2)+(2*k3)+k4);
//	return xy+k;
//}
vec4 synth(vec4 p){
	float sigma = 10;//*sin(tick/100.0);
	float beta = (8.0/3);//*sin(tick/50.0);
	float rho = 14;//*sin(tick/150.0);
	//return normalize(vec4(17*sin(p.y+tick/100.0)-p.x,p.y,17*sin(p.y-tick/100.0)-p.z,0));
	//return normalize(vec4(p.y,17*cos(p.y-tick/100.0)-p.z,17*sin(p.y-tick/133.0)-p.x,0));
	//return (vec4(sin(p.y*cos(tick/200.0))-p.z,-2+sin(tick/100.0),cos(p.y)+p.x,0));
	return normalize(vec4(-p.y*sin(tick/30.0),p.x*cos(tick/200.0),p.z*sin(tick/100.0),0));
}

vec4 lorenz(vec4 p){
	float sigma = 10*sin(tick/100.0);
	float beta = (8.0/3)*sin(tick/50.0);
	float rho = 14*sin(tick/150.0);
	return (vec4(sigma*(p.y-p.x),p.x*(rho-p.z)-p.y,p.x*p.y-beta*p.z,0));  //lorenz
}

vec4 rossler(vec4 p){
	float a = sin(tick/100.0);
	float b = sin(tick/50.0);
	float c = 14*sin(tick/150.0);
	return (vec4(-p.y-p.z,p.x+a*p.y,b+p.z*(p.x-c),0));  //lorenz
}

void main(){
	//2048 is the number of samples; don't change without changing it host-side
	float dT = 0.02;
	uint gidx = gl_GlobalInvocationID.x;
	uint n = 2048; //num samples +1, the last vertex stores the original position
	vec4 p1 = position[gidx*n]; //all lines will be processed in parallel
	for(int i=animate; i<n; i++){
		vec4 dir = rev*(mode == 0  ? lorenz(p1) : tex_func(p1));
		velocity[gidx*n+i] = dir;
		p1 += dT*normalize(dir);
		if(!in_bounds(p1) && mode != 0)
			position[gidx*n+i] = position[gidx*n+i-1];
		else
			position[gidx*n+i] = p1;
	}
}
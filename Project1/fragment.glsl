#version 420
 
// ------------------- INPUT VARIABLES ----------------------

in vec3 world;      // world coordinates of the entry point
in vec3 boundvec;
// ------------------- OUTPUT VARIABLE ----------------------

out vec4 fragcolor; // the final color of the fragment

uniform mat3 R;         // rotation matrix (defined by the trackball)
uniform vec3 Size;
float filter = 0.12;	// filters data noise  
//tcolor * (LightIntensity*(NdotL > 0.0 ? NdotL : 0.0) + AmbientIntensity);
layout (binding=0) uniform sampler3D tex;  // this is the texture, plugged into
// grdient calculation
vec3 gradient(vec3 texcoords){
	float h= 0.006;
	//use hardware interpolation with texture calls
	float gx = (texture(tex, vec3(texcoords.x+h,texcoords.y,texcoords.z)).x - texture(tex, vec3(texcoords.x-h,texcoords.y,texcoords.z)).x) / (2*h);
	float gy = (texture(tex, vec3(texcoords.x,texcoords.y+h,texcoords.z)).x - texture(tex, vec3(texcoords.x,texcoords.y-h,texcoords.z)).x) / (2*h);
	float gz = (texture(tex, vec3(texcoords.x,texcoords.y,texcoords.z+h)).x - texture(tex, vec3(texcoords.x,texcoords.y,texcoords.z-h)).x) / (2*h);
	vec3 grad = vec3(gx,gy,gz);
	return grad;
}
// transfer function
vec4 transfer(float p){
	if(p >= 0.22){ // produces brown rocks and trunk
		return vec4(p,p/2,p/4,p*0.8);
	}
	else if(p < filter){
		return vec4(0,0,0,0);
	}
	else{ //produces green leaves 
		return vec4(0,p,0,p*0.8);
	}
}

void main() { 
	float ambient_i = 0.3;
	float light_i = 1.9;
	vec3 texcoords;
	vec3 lightloc = vec3(5.5,5.5,0.0);
	float p;
	float p1;
	bool bound1 = boundvec.x < 0.49 && boundvec.y < 0.49 && boundvec.z < 0.49;
	bool bound2 = boundvec.x > -0.49 && boundvec.y > -0.49 && boundvec.z > -0.49;
	texcoords = boundvec + Size/2;
	vec3 N;// = normalize(gradient(texcoords));
	vec3 L;// = normalize(lightloc - texcoords);
	float NdotL;// = dot(N,L);
	vec3 grad;
	if(bound1 && bound2){
		  grad = gradient(texcoords);
		  N = normalize(grad);
		  L = normalize(lightloc - texcoords)*R;
		  NdotL = dot(N,L);
		  p1 = texture(tex, texcoords).x;
	}
	else{
		p = 0.0;
		p1 = 0.0;
		NdotL = 0.0;
	}
	vec4 color = transfer(p1);
	fragcolor = color * (light_i*(NdotL > 0.0 ? NdotL : 0.0) + ambient_i); 
	if(p1 >= filter){
		fragcolor.a = p1*2;
	}
	else{
		fragcolor.a = 0;
	}
} 

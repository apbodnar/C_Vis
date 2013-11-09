#version 430

out vec4 fragcolor; // the final color of the fragment
in vec3 texcoords;

uniform vec2 mouse; // mouse coords
uniform float tick;  //incremented value used to translate texcoords
uniform int res; // screen resolution
uniform int mode; //ocean or synthetic 
uniform float zoom; // zoom level

layout (binding=0) uniform sampler2D tex;  //noise texture
layout (binding=1) uniform sampler3D U;  
layout (binding=2) uniform sampler3D V;  

float deltaT = 1.0/(res);
int n = res/2;

//various vector field functions commented out
// mix and match for lolz
vec2 func(vec2 xy){
	float x = xy.x;
	float y = xy.y;
	return 0
	//+vec2(x,sin(x)+y)
	+ vec2(cos(sin(tick)*80*x+sin(tick*1.2785323)*80*y),sin(sin(tick*1.412354)*80*x+sin(tick*1.7214124)*80*y))
	//+vec2(x*x+y*y-0.25+y,x*x+y*y-0.25-x)
	//+vec2(0.4*sin(x+10*y)+y,0.4*sin(x+10*y)-x)
	//+ vec2(y,-x)
	//+ vec2(sin(tix)x,y)/4
	//+vec2(0.4*sin(5*x*x+5*y*y)+y,0.4*sin(5*x*x+5*y*y)-x)
	//+vec2(0.4*sin(x+5*y)+y,0.4*sin(5*x+5*y)-x)
	 + vec2(sin(tick/10),cos(tick/10)) //+vec2(-tick,-tick)*10;  //allows the user to mutate the scene with the mouse
	 ;
}
vec2 RK4(vec2 xy){
	float x0 = xy.x;
	float y0 = xy.y;
	vec2 k1,k2,k3,k4,k;
	k1=deltaT*(func(vec2(x0,y0)));
	k2=deltaT*(func(vec2((x0+(deltaT/2)),y0+(k1/2))));
	k3=deltaT*(func(vec2((x0+(deltaT/2)),y0+(k2/2))));
	k4=deltaT*(func(vec2((x0+deltaT),(y0+k3))));
	k=(1/6.0)*(k1+(2*k2)+(2*k3)+k4);
	return xy+k;
}

vec2 ocean_func(vec2 xy){
	float tx = texture(U, vec3(xy,0)/zoom).r;
	float ty = texture(V, vec3(xy,0)/zoom).r;
	vec2 p = vec2(tx,ty);
	return p;
}

vec2 F(vec2 xy){
	vec2 alt;
	if(mode == 0){
		alt = func(xy);
		return  alt/(1.0+ length(alt));
		return alt;
	}
	else{
		alt = ocean_func(xy);
		return alt/(0.1+ length(alt));
		return alt;
	}
}
//bad saturation function I made for when color is used
vec4 changeSaturation(vec4 rgb, float change){
	vec4 temp = vec4(0);
	temp.r = pow(rgb.r,change);
	temp.g = pow(rgb.g,change);
	temp.b = pow(rgb.b,change);
	
	return normalize(temp);
}



void main() { 
	
	float tx = texture(U, texcoords/zoom + vec3(mouse,0)).r;
	float ty = texture(V, texcoords/zoom + vec3(mouse,0)).r;
	vec2 p;

	p = texcoords.xy;// + mouse*zoom;
	
	vec4 rgb = vec4(0);
	for(int i=0;i < n; i++){
		p = RK4(p);
		rgb += texture(tex, -p);
	}
	rgb /= n;

	if(tx != 0 && ty != 0 && mode != 0)
		fragcolor = rgb;
	else if(mode == 0)
		//fragcolor = changeSaturation(rgb,1);
		fragcolor = rgb;
	else
		fragcolor = vec4(0,1,0,0);
} 

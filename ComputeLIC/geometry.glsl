#version 430
 
layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 4) out;

out vec3 dir;
out vec2 TexCoord;
out float mag;
flat out float vertex_g;
uniform mat4 PM;
in vec4 vel[];
in float vertex_v[];

void main () {
	float quad_dim = 0.04;
	vec3 vertex_avg = (gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz)/2.0;
	// make bilboards on each line segment
	
	vec3 front  = gl_in[0].gl_Position.xyz - gl_in[1].gl_Position.xyz;
	vec3 middle = gl_in[1].gl_Position.xyz - gl_in[2].gl_Position.xyz;
	vec3 back   = gl_in[2].gl_Position.xyz - gl_in[3].gl_Position.xyz;
	dir = abs(normalize(middle));
	mag = length(vel[0].xyz);
	vertex_g = vertex_v[0];

	vec3 line_front = (front.xyz + middle.xyz)/2.0;
	vec3 line_back  = (middle.xyz + back.xyz)/2.0;
	//vec4 normal = vec4(normalize(cross(vec3(strip),vec3(0,0,1))).xy,0,0);
	vec4 normal_front = vec4(normalize(cross(line_front,vertex_avg)),0);
	vec4 normal_back = vec4(normalize(cross(line_back,vertex_avg)),0);

	gl_Position = gl_in[0].gl_Position - quad_dim*normal_front;
	TexCoord = vec2(0,0);
    EmitVertex();

	gl_Position = gl_in[0].gl_Position + quad_dim*normal_front;
	TexCoord = vec2(1,0);
    EmitVertex();

	gl_Position = gl_in[1].gl_Position - quad_dim*normal_back;
	TexCoord = vec2(0,1);
    EmitVertex();

	gl_Position = gl_in[1].gl_Position + quad_dim*normal_back;
	TexCoord = vec2(1,1);
    EmitVertex();
	
    EndPrimitive();
    
}
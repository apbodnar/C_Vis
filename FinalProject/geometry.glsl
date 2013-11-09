#version 430
#extension GL_ARB_geometry_shader4 : enable
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in vec4 velocity1[];
out vec4 velocity2;
out vec2 TexCoord;

float quad_dim = 0.121;

void main () {
	velocity2=velocity1[0];
	for(int i = 0; i < gl_VerticesIn; i++) {
		gl_Position = gl_in[i].gl_Position;
		TexCoord = vec2(0,0);
		EmitVertex();

		gl_Position.y = gl_in[i].gl_Position.y;
		gl_Position.x = gl_in[i].gl_Position.x - quad_dim*1.0;
		TexCoord = vec2(1,0);
		EmitVertex();
	
		gl_Position.y = gl_in[i].gl_Position.y + quad_dim*1.0;
		gl_Position.x = gl_in[i].gl_Position.x;
		TexCoord = vec2(0,1);
		EmitVertex();
	
		gl_Position.y = gl_in[i].gl_Position.y + quad_dim*1.0;
		gl_Position.x = gl_in[i].gl_Position.x - quad_dim*1.0;
	
		TexCoord = vec2(1,1);
		EmitVertex();
	}
    EndPrimitive();
    
}
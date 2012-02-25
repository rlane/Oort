uniform mat4 p_matrix;
uniform mat4 mv_matrix;
uniform vec2 dimensions;
attribute vec2 vertex;
attribute vec2 texcoord;
varying vec2 v_texcoord;

void main(void) {
	v_texcoord = texcoord;
	vec2 vertex2 = vec2(vertex.x*dimensions.x, vertex.y*dimensions.y);
	gl_Position = p_matrix * mv_matrix * vec4(vertex2, 0.0, 1.0);
}

#version 120

uniform mat4 mv_matrix;
uniform mat4 p_matrix;
attribute vec2 vertex;
attribute vec4 color;
attribute vec2 jitter;
varying vec4 v_color;

void main()
{
	v_color = color;
	gl_Position = p_matrix * mv_matrix * vec4(vertex, 0.0, 1.0) + vec4(jitter, 0, 0);
}

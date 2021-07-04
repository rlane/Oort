#version 120

#if GL_ES
precision mediump float;
#endif

varying vec4 v_color;

void main()
{
	gl_FragColor = vec4(v_color.rgb * v_color.a, 1);
}

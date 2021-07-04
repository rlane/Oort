#version 120

#if GL_ES
precision mediump float;
#endif

varying vec4 v_color;
uniform sampler2D tex;

void main()
{
	float alpha = texture2D(tex, gl_PointCoord).a;
	gl_FragColor = vec4(v_color.xyz, 0) * alpha * v_color.w;
}

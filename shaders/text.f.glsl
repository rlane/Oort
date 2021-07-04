#version 120

#if GL_ES
precision mediump float;
#endif

uniform sampler2D tex;
varying float v_character;
const float numchars = 256.0;
const vec4 color = vec4(0.66, 1.0, 1.0, 0.66);

void main()
{
	vec2 char_pos = (vec2(1.0,1.0)-gl_PointCoord)*vec2(1.0/numchars,1.0);
	vec2 atlas_pos = vec2(v_character/numchars, 0);
	vec2 texcoord = atlas_pos + char_pos;
	float value = texture2D(tex, texcoord).r;
	if (value == 0.0) {
		discard;
	} else {
		gl_FragColor = color;
	}
}

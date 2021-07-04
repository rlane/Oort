#version 120

#if GL_ES
precision mediump float;
#endif

uniform vec4 color;

void main()
{
  gl_FragColor = vec4(color.rgb * color.a, 1);
}

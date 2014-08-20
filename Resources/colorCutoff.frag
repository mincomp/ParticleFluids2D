varying vec2 v_texCoord;
varying vec4 v_fragmentColor;

void main()
{
	vec4 color = texture2D(CC_Texture0, vec2(v_texCoord.x, v_texCoord.y));

	gl_FragColor = color * step(0.4, length(color.xy));
}

varying vec2 v_texCoord;
varying vec4 v_fragmentColor;

uniform sampler2D background;
uniform sampler2D particleProperties;

void main()
{
	// Referenced from https://github.com/klutch/StasisEngine/blob/master/StasisGame/StasisGameContent/fluid_effect.fx

	vec4 particleProp = texture2D(particleProperties, v_texCoord);
	vec2 speed = particleProp.xy;
	float density = particleProp.z;

	vec4 result;

	result = texture2D(background, v_texCoord - speed / 200);
	float pixelSize = 1.0f / 640.0f;
	if (density > 0)
	{
		float top = density;
		top -= tex2D(particleProperties, v_texCoord + float2(0, 6.5 * pixelSize)).z;
		top = top > 0.7 ? top : 0;
		result.rgb += top;

		if (density > 1.05)
			density = 1.05;
		if (density < 1)
			density = 1;
		result.rgb += density * vec4(0, 0, 0.6, 0);
	}

	gl_FragColor = result;
}

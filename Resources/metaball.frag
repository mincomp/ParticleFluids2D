varying vec2 v_texCoord;
varying vec4 v_fragmentColor;

float metaballFunc1(float r)
{
	if (r <= 1.0f / 3.0f)
	{
		return 1 - 3 * r * r;
	}
	else if (r <= 1)
	{
		return 1.5 * (1 - r) * (1 - r);
	}
	else
	{
		return 0;
	}
}

float metaballFunc2(float r)
{
	if (r * r <= 0.5)
	{
		return 2 * (pow(r, 4) - pow(r, 2) + 0.25);
	}
	else
	{
		return 0;
	}
}

float metaballFunc3(float r)
{
	if (r <= 1)
	{
		return (1 - r * r) * (1 - r * r);
	}
	else
	{
		return 0;
	}
}

void main()
{
	float r = length(v_texCoord);

	gl_FragColor = v_fragmentColor * metaballFunc1(r);
}

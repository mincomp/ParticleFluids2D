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

float metaballFunc4(float r)
{
	float range = 15;
	float p6WConst = 4 / 3.1415 / range;

	return p6WConst * pow(1 - r * r, 3) * 5;
}

void main()
{
	float r = length(v_texCoord);

	// x and y of v_fragmentColor is speed, z of v_fragmentColor is density.
	gl_FragColor = vec4(v_fragmentColor.xyz, 1) * metaballFunc1(r);
}

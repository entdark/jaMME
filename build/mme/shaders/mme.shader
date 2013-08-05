mme_additiveWhite
{
	{
		map *white
		blendfunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		alphaGen vertex
	}
}

mme/red
{
	{
		map *white
		rgbgen const ( 1 0 0 )
	}
}

mme/green
{
	{
		map *white
		rgbgen const ( 0 1 0 )
	}
}


mme/blue
{
	{
		map *white
		rgbgen const ( 0 0 1 )
	}
}

mme/black
{
	{
		map *white
		rgbgen const ( 0 0 0 )
	}
}

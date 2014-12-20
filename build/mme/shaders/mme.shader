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

mme/white
{
	{
		map *white
		rgbgen const ( 1 1 1 )
	}
}

mme_cursor
{
	nopicmip
	nomipmaps
	{
		clampmap data/cursor1
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
	{
		clampmap data/cursor2
		blendFunc GL_ONE GL_ONE
		rgbGen wave random 0.5 0.25 0.25 1
		tcMod stretch noise 1 0.02 0 10
	}
	{
		clampmap data/cursor3
		blendFunc GL_ONE GL_ONE
		rgbGen wave random 0.5 0.1 0 1
	}
}

mme_message_on
{
	nopicmip
	nomipmaps
	{
		map gfx/hud/message_on
//		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		blendfunc GL_ONE GL_ONE
		rgbGen const ( 0.7 1.0 0.8 )
	}
}

mme_message_off
{
	nopicmip
	nomipmaps
	{
		map gfx/hud/message_off
		blendfunc GL_ONE GL_ONE
	}
}

mme_rain
{
	q3map_nolightmap
    {
        map gfx/world/rain2
        blendFunc GL_ONE GL_ONE
    }
}

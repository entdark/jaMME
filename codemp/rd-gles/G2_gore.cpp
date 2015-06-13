#include "../ghoul2/G2_gore.h"
#include "../renderer/tr_common.h"

GoreTextureCoordinates::GoreTextureCoordinates()
{
	Com_Memset (tex, 0, sizeof (tex));
}

GoreTextureCoordinates::~GoreTextureCoordinates()
{
	for ( int i = 0; i < MAX_LODS; i++ )
	{
		if ( tex[i] )
		{
			ri->Z_Free(tex[i]);
			tex[i] = NULL;
		}
	}
}

#pragma once

#include "tr_local.h"

#define USE_PNG

#define BLURMAX 32
#define AVI_MAX_FRAMES	20000
#define AVI_MAX_SIZE	((2*1024-10)*1024*1024)
#define AVI_HEADER_SIZE	2048

typedef enum {
	mmeShotFormatTGA,
	mmeShotFormatJPG,
	mmeShotFormatPNG,
	mmeShotFormatAVI,
} mmeShotFormat_t;
/*	//went to tr_types.h because SaveJPG needs :s
typedef enum {
	mmeShotTypeRGB,
	mmeShotTypeRGBA,
	mmeShotTypeGray,
} mmeShotType_t;
*/
typedef struct mmeAviFile_s {
	char name[MAX_OSPATH];
	FILE *file;
	float fps;
	int	width, height;
	int frames;
	int index[AVI_MAX_FRAMES];
	int	written;
	int format;
	mmeShotType_t type;
} mmeAviFile_t;

typedef struct {
	char name[MAX_OSPATH];
	int	 counter;
	mmeShotFormat_t format;
	mmeShotType_t type;
	mmeAviFile_t avi;
} mmeShot_t;

typedef struct {
	int		commandId;
	char	name[MAX_QPATH];
	float	fps;
} captureCommand_t;

static struct {
	int		pixelCount;
	__m64	*accumAlign;
	__m64	*overlapAlign;
	int		overlapTotal, overlapIndex;
	int		blurTotal, blurIndex;
	__m64	blurMultiply[256];
	struct {
		qboolean		take;
		float			fps;
		mmeShot_t		main, stencil, depth;
	} shot;
	qboolean allocFailed;
} shotData;

static struct {
	int		pixelCount;
	__m64	*accumAlign;
	__m64	*overlapAlign;
	int		overlapTotal, overlapIndex;
	int		blurTotal, blurIndex;
	__m64	blurMultiply[256];
	struct {
		qboolean		take;
		float			fps;
		mmeShot_t		main, stencil, depth;
	} shot;
	qboolean allocFailed;
} shotDataStereo;
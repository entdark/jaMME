#pragma once

#include "tr_local.h"

#define AVI_MAX_FRAMES	20000
#define AVI_MAX_SIZE	((2*1024-10)*1024*1024)
#define AVI_HEADER_SIZE	2048

#define BLURMAX 256

typedef struct mmeAviFile_s {
	char name[MAX_OSPATH];
//	FILE *file;
	fileHandle_t f;
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
//	int		pixelCount;
	int		totalFrames;
	int		totalIndex;
	int		overlapFrames;
	int		overlapIndex;

	short	MMX[BLURMAX];
	short	SSE[BLURMAX];
	float	Float[BLURMAX];
} mmeBlurControl_t;

typedef struct {
	__m64	*accum;
	__m64	*overlap;
	int		count;
	mmeBlurControl_t *control;
} mmeBlurBlock_t;

void mmeAviShot( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, byte *inBuf );
void aviClose( mmeAviFile_t *aviFile );
void R_MME_JitterTable(float *jitarr, int num);

void MME_AccumClearSSE( void *w, const void *r, short int mul, int count );
void MME_AccumAddSSE( void* w, const void* r, short int mul, int count );
void MME_AccumShiftSSE( const void *r, void *w, int count );

extern cvar_t	*mme_aviFormat;

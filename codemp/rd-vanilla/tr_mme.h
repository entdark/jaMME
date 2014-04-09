#pragma once

#include "tr_local.h"

#define AVI_MAX_FRAMES			2000000
#define AVI_MAX_SIZE			((2*1024-10)*1024*1024)
#define AVI_HEADER_SIZE			2048
#define AVI_MAX_FILES			1000

#define BLURMAX 256

typedef struct mmeAviFile_s {
	char name[MAX_OSPATH];
	fileHandle_t f;
	float fps;
	int	width, height;
	unsigned int frames, aframes, iframes;
	int index[2*AVI_MAX_FRAMES];
	int aindex[2*AVI_MAX_FRAMES];
	int	written, awritten, maxSize;
	int header;
	int format;
	qboolean audio;
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

void R_MME_GetShot( void* output );
void R_MME_GetStencil( void *output );
void R_MME_GetDepth( byte *output );
void R_MME_SaveShot( mmeShot_t *shot, int width, int height, float fps, byte *inBuf );

void mmeAviShot( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, byte *inBuf, qboolean audio );
void mmeAviSound( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, const byte *soundBuf, int size );
void aviClose( mmeAviFile_t *aviFile );
void R_MME_JitterTable(float *jitarr, int num);

void MME_AccumClearSSE( void *w, const void *r, short int mul, int count );
void MME_AccumAddSSE( void* w, const void* r, short int mul, int count );
void MME_AccumShiftSSE( const void *r, void *w, int count );

void R_MME_BlurAccumAdd( mmeBlurBlock_t *block, const __m64 *add );
void R_MME_BlurOverlapAdd( mmeBlurBlock_t *block, int index );
void R_MME_BlurAccumShift( mmeBlurBlock_t *block  );
void blurCreate( mmeBlurControl_t* control, const char* type, int frames );

extern cvar_t	*mme_aviFormat;
extern cvar_t	*mme_aviAppend;

ID_INLINE byte * R_MME_BlurOverlapBuf( mmeBlurBlock_t *block ) {
	mmeBlurControl_t* control = block->control;
	int index = control->overlapIndex % control->overlapFrames;
	return (byte *)( block->overlap + block->count * index );
}

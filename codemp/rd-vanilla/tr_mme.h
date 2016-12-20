#ifndef TR_MME_H
#define TR_MME_H

#include "tr_local.h"

#if defined (X86_OR_64)
#include <mmintrin.h>
#endif

#define AVI_MAX_FRAMES	20000
#define AVI_MAX_SIZE	((2*1024-10)*1024*1024)
#define AVI_HEADER_SIZE	2048
#define AVI_MAX_FILES	1000

#define BLURMAX 256

#define PIPE_COMMAND_DEFAULT "ffmpeg -f avi -i - -threads 0 -preset ultrafast -y -pix_fmt yuv420p -crf 23 %o.mp4 2> ffmpeglog.txt"

typedef struct mmePipeFile_s {
    char name[MAX_OSPATH];
    fileHandle_t f;
    float fps;
    mmeShotType_t type;
} mmePipeFile_t;

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
    qboolean pipe;
	mmeShotType_t type;
} mmeAviFile_t;

typedef struct {
	char name[MAX_OSPATH];
	char nameOld[MAX_OSPATH];
	int	 counter;
	mmeShotFormat_t format;
	mmeShotType_t type;
    mmeAviFile_t avi;
    mmePipeFile_t pipe;
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
#if !defined (HAVE_GLES) || defined (X86_OR_64)
	__m64	*accum;
	__m64	*overlap;
#else
	void	*accum;
	void	*overlap;
#endif
	int		count;
	mmeBlurControl_t *control;
} mmeBlurBlock_t;

void R_MME_GetShot( void* output, mmeShotType_t type );
void R_MME_GetStencil( void *output );
void R_MME_GetDepth( byte *output );
void R_MME_SaveShot( mmeShot_t *shot, int width, int height, float fps, byte *inBuf, qboolean audio, int aSize, byte *aBuf );

void mmeAviShot( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, byte *inBuf, qboolean audio );
void mmeAviSound( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, const byte *soundBuf, int size );
void aviClose( mmeAviFile_t *aviFile );

void mmePipeShot(mmePipeFile_t *pipeFile, const char *name, mmeShotType_t type, int width, int height, float fps, byte *inBuf);
void pipeClose(mmePipeFile_t *pipeFile);

void MME_AccumClearSSE( void *w, const void *r, short int mul, int count );
void MME_AccumAddSSE( void* w, const void* r, short int mul, int count );
void MME_AccumShiftSSE( const void *r, void *w, int count );

void R_MME_BlurAccumAdd( mmeBlurBlock_t *block, 
#if !defined (HAVE_GLES) || defined (X86_OR_64)
	const __m64 *add
#else
	const void *add
#endif
);
void R_MME_BlurOverlapAdd( mmeBlurBlock_t *block, int index );
void R_MME_BlurAccumShift( mmeBlurBlock_t *block  );
void blurCreate( mmeBlurControl_t* control, const char* type, int frames );
void R_MME_JitterTable(float *jitarr, int num);

float R_MME_FocusScale(float focus);
void R_MME_ClampDof(float *focus, float *radius);

extern cvar_t	*mme_pipeCommand;

extern cvar_t	*mme_aviFormat;
extern cvar_t	*mme_aviLimit;

extern cvar_t	*mme_blurJitter;
extern cvar_t	*mme_blurStrength;
extern cvar_t	*mme_dofFrames;
extern cvar_t	*mme_dofRadius;

extern ID_INLINE byte * R_MME_BlurOverlapBuf( mmeBlurBlock_t *block );

#endif //TR_MME_H

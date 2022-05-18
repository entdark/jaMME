#ifndef TR_MME_H
#define TR_MME_H

#include "tr_local.h"
#include "../client/snd_public.h"

#if !defined (HAVE_GLES) || defined (X86_OR_64)
#include <mmintrin.h>
#endif

#define MME_MAX_WORKSIZE 2048

#define AVI_MAX_FRAMES	20000
#define AVI_MAX_SIZE	((2*1024-10)*1024*1024)
#define AVI_HEADER_SIZE	2048
#define AVI_MAX_FILES	1000

#define BLURMAX 256

#define PIPE_COMMAND_BASE(s, e)	"ffmpeg -f avi -i - -threads 0 " s " -c:a aac -c:v libx264 -preset ultrafast -y -pix_fmt yuv420p -crf 19 %o." e " 2> ffmpeglog.txt"
#define PIPE_COMMAND_DEFAULT	PIPE_COMMAND_BASE("", "mkv")
#define PIPE_COMMAND_STEREO		PIPE_COMMAND_BASE("-metadata:s:v stereo_mode=top_bottom", "mkv")
#define PIPE_COMMAND_VR180		PIPE_COMMAND_BASE("-vf v360=c1x6:he:in_forder=frblud:in_stereo=tb:out_stereo=sbs -metadata:s:v stereo_mode=left_right", "mkv")
#define PIPE_COMMAND_VR360		PIPE_COMMAND_BASE("-vf v360=c1x6:e:in_forder=frblud", "mp4")

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
	byte *stereoTemp;
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
	void	*accum;
	void	*overlap;
	int		count;
	mmeBlurControl_t *control;
} mmeBlurBlock_t;

void R_MME_GetShot( void* output, mmeShotType_t type, qboolean square );
void R_MME_GetStencil( void *output );
void R_MME_GetDepth( byte *output );
void R_MME_SaveShot( mmeShot_t *shot, int width, int height, float fps, byte *inBuf, qboolean audio, int aSize, byte *aBuf, qboolean stereo );

void mmeAviShot( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, byte *inBuf, qboolean audio );
void mmeAviSound( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, const byte *soundBuf, int size );
void aviClose( mmeAviFile_t *aviFile );

#if !defined (HAVE_GLES) || defined (X86_OR_64)
void MME_AccumClearSSE( void* Q_RESTRICT w, const void* Q_RESTRICT r, short int mul, int count );
void MME_AccumAddSSE( void* Q_RESTRICT w, const void* Q_RESTRICT r, short int mul, int count );
void MME_AccumShiftSSE( const void *r, void *w, int count );
#endif

void R_MME_BlurAccumAdd( mmeBlurBlock_t *block, const void *add);
void R_MME_BlurOverlapAdd( mmeBlurBlock_t *block, int index );
void R_MME_BlurAccumShift( mmeBlurBlock_t *block  );
void blurCreate( mmeBlurControl_t* control, const char* type, int frames );
void R_MME_JitterTable(float *jitarr, int num);

float R_MME_FocusScale(float focus);
void R_MME_ClampDof(float *focus, float *radius);

extern cvar_t	*mme_combineStereoShots;
extern cvar_t	*mme_pipeCommand;

extern cvar_t	*mme_aviFormat;
extern cvar_t	*mme_aviLimit;

extern cvar_t	*mme_blurJitter;
extern cvar_t	*mme_blurStrength;
extern cvar_t	*mme_dofFrames;
extern cvar_t	*mme_dofRadius;

extern ID_INLINE byte * R_MME_BlurOverlapBuf( mmeBlurBlock_t *block );

#endif //TR_MME_H

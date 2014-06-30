#pragma once

// snd_local.h -- private sound definations

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"

#define		SFX_SOUNDS		10000

typedef struct sfxEntry_s {
	struct		sfxEntry_s *next;
	char		name[MAX_QPATH];
} sfxEntry_t;

typedef struct {
	float		seekTime;
	float		length;
	char		startName[MAX_QPATH];
	char		loopName[MAX_QPATH];
	qboolean	playing;
	qboolean	reload;
	qboolean	override;
} backgroundSound_t;

typedef struct {
	int			channels;
	int			samples;				// mono samples in buffer
	int			submission_chunk;		// don't mix less than this #
	int			samplebits;
	int			speed;
	byte		*buffer;
} dma_t;

typedef struct {
	vec3_t		origin;
	vec3_t		velocity;
	sfxHandle_t	handle;
	int			lastFrame;
	int			volume;
} entitySound_t;

typedef struct {
	const void	*parent;
	vec3_t		origin;
	vec3_t		velocity;
	sfxHandle_t	handle;
	int			volume;
} loopQueue_t;

typedef struct {
	sfxHandle_t	handle;
	vec3_t		origin;
	short		entNum;
	char		entChan;
	char		hasOrigin;
	unsigned char volume;
} channelQueue_t;

struct openSound_s;
typedef int (*openSoundRead_t)( struct openSound_s *open, qboolean stereo, int size, short *data );
/* Maybe some kind of error return sometime? */
typedef int (*openSoundSeek_t)( struct openSound_s *open, int samples );
typedef void (*openSoundClose_t)( struct openSound_s *open );

typedef struct openSound_s {
	int					rate;
	int					totalSamples, doneSamples;
	char				buf[16*1024];
	int					bufUsed, bufPos;
	fileHandle_t		fileHandle;
	int					fileSize, filePos;

	openSoundRead_t		read;
	openSoundSeek_t		seek;
	openSoundClose_t	close;

	char				data[0];
} openSound_t;

//from SND_AMBIENT
extern void AS_Init( void );
extern void AS_Free( void );

/*
====================================================================

  SYSTEM SPECIFIC FUNCTIONS

====================================================================
*/

extern	dma_t	dma;
// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init(void);

// gets the current DMA position
int		SNDDMA_GetDMAPos(void);

// shutdown the DMA xfer.
void	SNDDMA_Shutdown(void);

void	SNDDMA_BeginPainting (void);

void	SNDDMA_Submit(void);

//====================================================================

#define	MAX_CHANNELS			32
#define	MAX_SNDQUEUE			512
#define	MAX_LOOPQUEUE			512

extern	sfxEntry_t		sfxEntries[SFX_SOUNDS];
extern	backgroundSound_t s_background;
extern	channelQueue_t	s_channelQueue[MAX_SNDQUEUE];
extern	int				s_channelQueueCount;
extern	entitySound_t	s_entitySounds[MAX_GENTITIES];
extern	loopQueue_t		s_loopQueue[MAX_LOOPQUEUE];
extern	int				s_loopQueueCount;

extern cvar_t	*s_volume;
extern cvar_t	*s_volumeVoice;
extern cvar_t	*s_musicVolume;
extern cvar_t	*s_khz;

extern cvar_t	*s_doppler;
extern cvar_t	*s_dopplerSpeed;
extern cvar_t	*s_dopplerFactor;

extern cvar_t	*s_attenuate;

extern cvar_t	*s_lip_threshold_1;
extern cvar_t	*s_lip_threshold_2;
extern cvar_t	*s_lip_threshold_3;
extern cvar_t	*s_lip_threshold_4;

extern int		s_listenNumber;
extern vec3_t	s_listenOrigin;
extern vec3_t	s_listenVelocity;
extern vec3_t	s_listenAxis[3];

extern qboolean	s_underWater;

qboolean S_FileExists(char *fileName);

openSound_t *S_SoundOpen( const char *fileName );
int S_SoundRead( openSound_t *open, qboolean stereo, int size, short *data );
int S_SoundSeek( openSound_t *open, int samples );
void S_SoundClose( openSound_t *open );

#pragma once

//All mixing speeds have this as 1.0
#define		MIX_SPEED		22050
#define		MIX_SHIFT		8

#define		FIRSCALE		4
#define		ONETHIRD		342*FIRSCALE

typedef struct mixSound_s {
	int			lastUsed;
	int			speed;
	int			samples;
	int			size;
	sfxHandle_t	handle;
	struct		mixSound_s *prev, *next;
	short		data[0];
} mixSound_t;

typedef struct {
	sfxHandle_t		handle;
	int				index;
	vec3_t			origin;
	short			entNum;
	char			entChan;
	char			hasOrigin;
	char			wasMixed;
} mixChannel_t;

typedef struct {
	const void		*parent;
	sfxHandle_t		handle;
	int				index;
} mixLoop_t;

typedef struct {
	openSound_t		*sound;
	char			soundName[MAX_QPATH];
	int				index;
	int				length;
	int				done;
	short			last[2];
} mixBackground_t;

typedef struct {
	int				statel, stater;
	double			zl[ONETHIRD], zr[ONETHIRD];
	int				index;
} mixEffect_t;

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
} channelQueue_t;

typedef struct {
	float		seekTime;
	float		length;
	char		startName[MAX_QPATH];
	char		loopName[MAX_QPATH];
	qboolean	playing;
	qboolean	reload;
	qboolean	override;
} backgroundSound_t;

extern	cvar_t			*s_effects;

#define	MAX_SNDQUEUE	512
#define	MAX_LOOPQUEUE	512
extern	channelQueue_t	s_channelQueue[MAX_SNDQUEUE];
extern	int				s_channelQueueCount;

extern	loopQueue_t		s_loopQueue[MAX_LOOPQUEUE];
extern	int				s_loopQueueCount;

extern	entitySound_t	s_entitySounds[MAX_GENTITIES];

extern	backgroundSound_t s_background;

extern void S_DMAInit(void);
extern void S_DMA_Update( float scale );
void S_DMAClearBuffer(void);
void S_DMAStopSound(int entityNum, int entchannel, sfxHandle_t sfxHandle);
void S_MMEStopSound(int entityNum, int entchannel, sfxHandle_t sfxHandle);

void S_MixInit(void);
void S_MixBackground( mixBackground_t *background, int speed, int count, int *output );
void S_MixChannels( mixChannel_t *ch, int channels, int speed, int count, int *output );
void S_MixLoops( mixLoop_t *loop, int loopCount, int speed, int count, int *output );
void S_MixClipOutput (int count, const int *input, short *output, int outStart, int outMask);
const mixSound_t *S_MixGetSound( sfxHandle_t sfxHandle );

void S_MixEffects(mixEffect_t *effect, int speed, const int count, int *output);
void S_EffectInit(void);

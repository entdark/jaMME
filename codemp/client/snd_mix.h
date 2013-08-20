//All mixing speeds have this as 1.0
#define		MIX_SPEED		22050
#define		MIX_SHIFT		8

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
	sfxHandle_t	handle;
	vec3_t		origin;
	short		entNum;
	char		entChan;
	char		hasOrigin;
} channelQueue_t;


void S_MixChannels( mixChannel_t *ch, int channels, int speed, int count, int *output );
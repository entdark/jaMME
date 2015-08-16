// Copyright (C) 2005 Eugene Bujak.
//
// cl_demos.c -- Enhanced client-side demo player

#define DEMO_MAX_INDEX	3600
#define DEMO_PLAY_CMDS  256
#define DEMO_SNAPS  32
#define DEMOCONVERTFRAMES 16

#define DEMO_CLIENT_UPDATE 0x1
#define DEMO_CLIENT_ACTIVE 0x2

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

typedef struct {
	int			offsets[MAX_CONFIGSTRINGS];
	int			used;
	char		data[MAX_GAMESTATE_CHARS];
} demoString_t;

typedef struct {
	int			serverTime;
	playerState_t clients[MAX_CLIENTS];
	playerState_t vehs[MAX_CLIENTS];
	byte		clientData[MAX_CLIENTS];
	entityState_t entities[MAX_GENTITIES];
	byte		entityData[MAX_GENTITIES];
	entityState_t entityBaselines[MAX_GENTITIES];
	int			commandUsed;
	char		commandData[2048*MAX_CLIENTS];
	byte		areaUsed;
	byte		areamask[MAX_MAP_AREA_BYTES];
	demoString_t string;
} demoFrame_t;

typedef struct {
	demoFrame_t		frames[DEMOCONVERTFRAMES];
	int				frameIndex;
	clSnapshot_t	snapshots[PACKET_BACKUP];
	entityState_t	entityBaselines[MAX_GENTITIES];
	entityState_t	parseEntities[MAX_PARSE_ENTITIES];
	int				messageNum, lastMessageNum;
} demoConvert_t;

#define FRAME_BUF_SIZE 20
typedef struct {
	fileHandle_t		fileHandle;
	char				fileName[MAX_QPATH];
	int					fileSize, filePos;
	int					totalFrames;
	int					startTime, endTime;			//serverTime from first snapshot 
	int					seekTime;
	qboolean			lastFrame;
	int					frameNumber;

	char				commandData[256*1024];
	int					commandFree;
	int					commandStart[DEMO_PLAY_CMDS];
	int					commandCount;
	int					clientNum;
	demoFrame_t			storageFrame[FRAME_BUF_SIZE];
	demoFrame_t			*frame, *nextFrame;
	qboolean			nextValid;
	struct	{
		int				pos, time, frame;
	} fileIndex[DEMO_MAX_INDEX];
	int					fileIndexCount;
} demoPlay_t;

#define DEMOLISTSIZE 1024
typedef struct {
	char demoName[MAX_OSPATH];
	char projectName[MAX_OSPATH];
} demoListEntry_t;

#define MAX_TIMESTAMPS 256

typedef struct {
	byte					buffer[128*1024];
	qboolean				firstPack;
	qboolean				precaching;
	qboolean				commandSmoothing;
	qboolean				del;
	int						nextNum, currentNum;
	struct {
		demoPlay_t			*handle;
		int					snapCount;
		int					messageCount;
		int					oldDelay;
		int					oldTime;
		int					oldFrameNumber;
		int					serverTime;
	} play;
	struct {
		demoListEntry_t		entry[DEMOLISTSIZE];
		int					index, count;
	} list;
	struct {
		char				defaultName[MAX_QPATH];
		char				demoName[MAX_QPATH];
		char				customName[MAX_QPATH];
		int					timeStamps[MAX_TIMESTAMPS];
		char				mod[MAX_QPATH];
	} record;
	struct {
		clientConnection_t	Clc;
		clientActive_t		Cl;
	} cut;
} demo_t;

extern demo_t demo;

extern int demoLength(void);
extern int demoTime(void);
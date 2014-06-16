// Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

#include "bg_demos.h"
#include "cg_local.h"
#include "cg_demos_math.h"

#define LOGLINES 8

#define DEMO_ANIM

typedef enum {
	editNone,
	editCamera,
	editChase,
	editLine,
	editDof,
	editAnim,
//	editEffect,
//	editScript,
	editLast,
} demoEditType_t;

typedef enum {
	viewCamera,
	viewChase,
//	viewEffect,
	viewLast
} demoViewType_t;


typedef enum {
	findNone,
	findObituary,
	findDirect,
} demofindType_t;

typedef struct demoLinePoint_s {
	struct			demoLinePoint_s *next, *prev;
	int				time, demoTime;
} demoLinePoint_t;

typedef struct demoCameraPoint_s {
	struct			demoCameraPoint_s *next, *prev;
	vec3_t			origin, angles;
	float			fov;
	int				time, flags;
	float			len, anglesLen;
} demoCameraPoint_t;

typedef struct demoChasePoint_s {
	struct			demoChasePoint_s *next, *prev;
	vec_t			distance;
	vec3_t			angles;
	int				target;
	vec3_t			origin;
	int				time;
	float			len;
} demoChasePoint_t;

#define MAX_BONES 72

typedef struct demoAnimPoint_s {
	struct			demoAnimPoint_s *next, *prev;
	vec3_t			angles[MAX_BONES];
	int				time;
} demoAnimPoint_t;

typedef struct demoDofPoint_s {
	struct			demoDofPoint_s *next, *prev;
	float			focus, radius;
	int				time;
} demoDofPoint_t;

typedef struct {
	char lines[LOGLINES][1024];
	int	 times[LOGLINES];
	int	 lastline;
} demoLog_t;

typedef struct demoMain_s {
	int				serverTime;
	float			serverDeltaTime;
#ifdef DEMO_ANIM
	struct {
		qboolean	locked, drawing;
		int			target;
		int			bone;
		vec3_t		angles[MAX_BONES];
		vec3_t		axis[3][MAX_BONES];
		qboolean	override[MAX_CLIENTS];
		demoAnimPoint_t *points[MAX_CLIENTS];
	} anim;
#endif
	struct {
		int			start, end;
		qboolean	locked;
		float		speed;
		int			offset;
		float		timeShift;
		int			shiftWarn;
		int			time;
		demoLinePoint_t *points;
	} line;
	struct {
		int			start;
		float		range;
		int			index, total;
		int			lineDelay;
	} loop;
	struct {
		int			start, end;
		qboolean	locked;
		int			target;
		vec3_t		angles, origin, velocity;
		vec_t		distance;
		float		timeShift;
		int			shiftWarn;
		demoChasePoint_t *points;
		centity_t	*cent;
	} chase;
	struct {
		int			start, end;
		int			target, flags;
		int			shiftWarn;
		float		timeShift;
		float		fov;
		qboolean	locked;
		vec3_t		angles, origin, velocity;
		demoCameraPoint_t *points;
		posInterpolate_t	smoothPos;
		angleInterpolate_t	smoothAngles;
	} camera;
	struct {
		int			start, end;
		int			target;
		int			shiftWarn;
		float		timeShift;
		float		focus, radius;
		qboolean	locked;
		demoDofPoint_t *points;
	} dof;
	struct {
		int			time;
		int			oldTime;
		int			lastTime;
		float		fraction;
		float		speed;
		qboolean	paused;
	} play;
	struct {
		float speed, acceleration, friction;
	} move;
	vec3_t			viewOrigin, viewAngles;
	demoViewType_t	viewType;
	vec_t			viewFov;
	int				viewTarget;
	float			viewFocus, viewFocusOld, viewRadius;
	demoEditType_t	editType;

	vec3_t		cmdDeltaAngles;
	usercmd_t	cmd, oldcmd;
	int			nextForwardTime, nextRightTime, nextUpTime;
	int			deltaForward, deltaRight, deltaUp;
	struct	{
		int		start, end;
		qboolean active, locked;
	} capture;
	struct {
		qhandle_t additiveWhiteShader;
		qhandle_t mouseCursor;
		qhandle_t switchOn, switchOff;
	} media;
	demofindType_t find;
	qboolean	seekEnabled;
	qboolean	initDone;
	qboolean	autoLoad;
	qboolean	drawFully;
	demoLog_t	log;
} demoMain_t;

extern demoMain_t demo;

void demoPlaybackInit(void);
void CG_DemosAddLog(const char *fmt, ...);
void demoCameraCommand_f(void);

demoCameraPoint_t *cameraPointSynch( int time );
void cameraMove( void );
void cameraMoveDirect( void );
void cameraUpdate( int time, float timeFraction );
void cameraDraw( int time, float timeFraction );
qboolean cameraParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void cameraSave( fileHandle_t fileHandle );

void demoLineCommand_f(void);
demoLinePoint_t *linePointSynch(int playTime);
void lineAt(int playTime, float playFraction, int *demoTime, float *demoFraction, float *demoSpeed );
qboolean lineParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void lineSave( fileHandle_t fileHandle );

void demoMovePoint( vec3_t origin, vec3_t velocity, vec3_t angles);
void demoMoveDirection( vec3_t origin, vec3_t angles );

void demoMoveChase( void );
void demoMoveChaseDirect( void );
void demoMoveLine( void );
void demoMoveUpdateAngles( void );
void demoMoveDeltaCmd( void );

void demoCaptureCommand_f( void );
void demoLoadCommand_f( void );
void demoSaveCommand_f( void );

#define PUGMOD_CONVERTER //none needs that except me :P

#ifdef PUGMOD_CONVERTER
extern int demoStartTime;
void demoConvertCommand_f( void );
void demoCamPoint_f(void);
#endif

qboolean demoProjectLoad( const char *fileName );
qboolean demoProjectSave( const char *fileName );

void demoSaveLine( fileHandle_t fileHandle, const char *fmt, ...);

void demoChaseCommand_f( void );
void chaseEntityOrigin( centity_t *cent, vec3_t origin );
demoChasePoint_t *chasePointSynch(int time );
qboolean chaseParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void chaseSave( fileHandle_t fileHandle );

demoDofPoint_t *dofPointSynch( int time );
void dofMove(void);
void dofUpdate( int time, float timeFraction );
void dofDraw( int time, float timeFraction );
qboolean dofParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data);
void dofSave( fileHandle_t fileHandle );
void demoDofCommand_f(void);

#ifdef DEMO_ANIM
void demoMoveAnim(void);
void animUpdate(int time, float timeFraction);
void demoAnimCommand_f(void);
demoAnimPoint_t *animPointSynch(int time, int target);
void animDraw(int time, float timeFraction);
void animBoneVectors(vec3_t vec[MAX_BONES], int flags, int client);

extern const char *boneList[72];
#endif

qboolean demoCentityBoxSize( const centity_t *cent, vec3_t container );
int demoHitEntities( const vec3_t start, const vec3_t forward );
centity_t *demoTargetEntity( int num );
void chaseUpdate( int time, float timeFraction );
void chaseDraw( int time, float timeFraction );
void demoDrawCrosshair( void );

void hudInitTables(void);
void hudToggleInput(void);
void hudDraw(void);
const char *demoTimeString( int time );

#define CAM_ORIGIN	0x001
#define CAM_ANGLES	0x002
#define CAM_FOV		0x004
#define CAM_TIME	0x100

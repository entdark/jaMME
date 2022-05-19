#include "cg_local.h"
#include "cg_demos.h"
#include "../ui/menudef.h"
#include "../ui/keycodes.h"

#define MIN_SCALE 0.1f
#define MAX_SCALE 1.0f
#define TEAMOVERLAY_SCALE 0.2f
#define TEAMOVERLAY_MAX_SCALE 0.5f

extern int CG_CalcViewValues(void);
extern float cg_linearFogOverride;
extern qboolean cg_rangedFogging;
extern qboolean CG_UsingEWeb(void);
extern void CG_CalcScreenEffects(void);
extern void CG_DrawAutoMap(void);
extern void CG_DrawSkyBoxPortal(const char *cstr);
extern void CG_DrawZoomMask(void);
extern void CG_UpdateFallVector(void);
extern qboolean CG_DrawCrosshairNames(void);
extern void CG_SetPredictedThirdPerson(void);
extern qboolean CG_Piloting(int vehNum);
extern void CG_InterpolatePlayerState( qboolean grabAngles );
extern void CG_InterpolateVehiclePlayerState( qboolean grabAngles );
extern void CG_CalcEntityLerpPositions( centity_t *cent ); //cg_ents.c
extern void trap_MME_TimeFraction( float timeFraction );

typedef struct multiSpecWindow_s {
	struct		multiSpecWindow_s *next;
	float		x, y;
	float		scale;
	float		fov;
	float		borderWidth;
	vec3_t		borderColour;
	int			clientNum; // -1 = chase, -2 = camera
	qboolean	draw2D;
} multiSpecWindow_t;

typedef struct multiSpec_s {
	multiSpecWindow_t	*windows;
	qboolean			background;
	multiSpecWindow_t	*current, *draw2D;
	qboolean			active;
	struct {
		float			cursorX, cursorY;
		float			downX, downY;
		int				keyCatcher;
		qboolean		active;
		multiSpecWindow_t *window;
	} edit;
	struct {
		qboolean		replace;
		float			offset;
	} teamoverlay;
	qhandle_t			cursor;
} multiSpec_t;

static multiSpec_t multiSpec;

static void multiSpecClampWindowBounds(multiSpecWindow_t *window);
static qboolean multiSpecAllowed(int clientNum);
static qboolean multiSpecAllowedAny(void);
static int multiSpecGetPrev(int clientNum);
static int multiSpecGetNext(int clientNum);

static multiSpecWindow_t *multiSpecWindowAlloc(void) {
	multiSpecWindow_t * point = (multiSpecWindow_t *)malloc(sizeof (multiSpecWindow_t));
	if (!point)
		CG_Error("Out of memory");
	memset( point, 0, sizeof( multiSpecWindow_t ));
	return point;
}

static void multiSpecWindowFree(multiSpecWindow_t *window) {
	free( window );
}

static int multiSpecWindowNum( multiSpecWindow_t *window ) {
	multiSpecWindow_t	*w = multiSpec.windows;
	int					i;
	for (i = 0; w && w != window; w = w->next, i++);
	return w == window ? i : -1;
}

static multiSpecWindow_t *multiSpecWindowGet( int windowNum ) {
	multiSpecWindow_t	*window = multiSpec.windows;
	int					i;
	for (i = 0; window && i < windowNum; window = window->next, i++);
	return window;
}

static qboolean multiSpecWindowUpdate( multiSpecWindow_t *window, const char *windowConfig, qboolean update ) {
	int					clientNum;
	const char			*v;

#define CAN_SET_VALUE (v[0] || (!v[0] && !update))
	v = Info_ValueForKey(windowConfig, "s");
	if (CAN_SET_VALUE) {
		if (!v[0])
			v = "0.25";
		window->scale = Com_Clamp(MIN_SCALE, MAX_SCALE, atof(v));
	}

	v = Info_ValueForKey(windowConfig, "x");
	if (CAN_SET_VALUE) {
		if (!v[0])
			window->x = (1.0f - window->scale) * SCREEN_WIDTH * 0.5f;
		else
			window->x = atof(v);
	}

	v = Info_ValueForKey(windowConfig, "y");
	if (CAN_SET_VALUE) {
		if (!v[0])
			window->y = (1.0f - window->scale) * SCREEN_HEIGHT * 0.5f;
		else
			window->y = atof(v);
	}

	multiSpecClampWindowBounds(window);

	v = Info_ValueForKey(windowConfig, "client");
	if (CAN_SET_VALUE) {
		clientNum = atoi(v);
		if (!v[0] || clientNum < 0 || clientNum >= MAX_CLIENTS)
			window->clientNum = cg.snap ? cg.snap->ps.clientNum : cg.clientNum;
		else
			window->clientNum = clientNum;
		if (!multiSpecAllowed(window->clientNum))
			window->clientNum = multiSpecGetNext(window->clientNum);
		if (!v[0] && window->clientNum < 0)
			window->clientNum = multiSpecGetNext(-1);
	}

	v = Info_ValueForKey(windowConfig, "fov");
	if (CAN_SET_VALUE) {
		if (!v[0])
			v = cg_fov.string;
		window->fov = atof(v);
	}

	v = Info_ValueForKey(windowConfig, "2d");
	if (CAN_SET_VALUE) {
		if (!v[0])
			v = "1";
		window->draw2D = atoi(v);
	}

	v = Info_ValueForKey(windowConfig, "bw");
	if (CAN_SET_VALUE) {
		if (!v[0])
			v = "1.0";
		window->borderWidth = atof(v);
		if (window->borderWidth < 0.0f)
			window->borderWidth = 0.0f;
	}

	v = Info_ValueForKey(windowConfig, "bc");
	if (CAN_SET_VALUE) {
		if (!v[0])
			v = "7";
		Q_parseColor( v, defaultColors, window->borderColour );
	}

	return qtrue;
}

static multiSpecWindow_t *multiSpecWindowAdd( const char *windowConfig ) {
	multiSpecWindow_t	*window = multiSpec.windows;
	multiSpecWindow_t	*newWindow;
	if (!window) {
		newWindow = multiSpec.windows = multiSpecWindowAlloc();
	} else {
		for( ;window->next; window = window->next);
		newWindow = window->next = multiSpecWindowAlloc();
	}

	multiSpecWindowUpdate(newWindow, windowConfig, qfalse);

	return newWindow;
}

static qboolean multiSpecWindowDel( int windowNum ) {
	multiSpecWindow_t	*window = multiSpec.windows;
	multiSpecWindow_t	*prev = NULL;
	int					i;
	for (i = 0; window && i < windowNum; prev = window, window = window->next, i++);
	if (!window)
		return qfalse;
	if (prev)
		prev->next = window->next;
	else
		multiSpec.windows = window->next;
	multiSpecWindowFree(window);
	return qtrue;
}

/*
===================
CG_TransitionSnapshot

The transition point from snap to nextSnap has passed
===================
*/
void CG_ReTransitionSnapshot(void) {
	playerState_t *ops, *ps;
	int i;

	if (!cg.snap)
		return;
	if (!cg.nextSnap)
		return;
	if (cg.timeFraction >= cg.snap->serverTime - cg.time && cg.timeFraction < cg.nextSnap->serverTime - cg.time)
		return;

	for (i = 0; i < cg.nextSnap->numEntities; i++) {
		centity_t *cent;
		entityState_t oldEs;
		cent = &cg_entities[cg.nextSnap->entities[i].number];
		oldEs = cent->currentState;
		cent->currentState = cent->nextState;
		// check for events
		CG_CheckEvents(cent);
		cent->currentState = oldEs;
	}
	
	// check for playerstate transition events
	ops = &cg.snap->ps;
	ps = &cg.nextSnap->ps;
	// teleporting checks are irrespective of prediction
	if ((ps->eFlags ^ ops->eFlags) & EF_TELEPORT_BIT) {
		cg.thisFrameTeleport = qtrue;	// will be cleared by prediction code
	}

	// if we are not doing client side movement prediction for any
	// reason, then the client events and view changes will be issued now
	if (cg.demoPlayback || (cg.nextSnap->ps.pm_flags & PMF_FOLLOW)
		|| cg_noPredict.integer || g_synchronousClients.integer || CG_UsingEWeb()) {
		snapshot_t *oldFrame = cg.snap;
		cg.snap = cg.nextSnap;
//		CG_TransitionPlayerState(ps, ops);
		cg.snap = oldFrame;
	}
}

/*
===================
CG_DrawPovFollow

Draw the Following text
===================
*/
static void multiSpecDrawFollow(clientInfo_t *ci, multiSpecWindow_t *window) {
	CG_Text_Paint(5.0f/window->scale*cgs.widthRatioCoef, 0.0f/window->scale,
		0.7f/window->scale, colorWhite, ci->name, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_SMALL);
}

static void multiSpecDrawHUD(clientInfo_t *ci, multiSpecWindow_t *window) {
	int i;
	gitem_t	*item;
	const char *s;
	int textHeight;
	int armor;
	float textOffsetScale;
	float x = 0.0f;
	float y = 0.0f;
	float w = 0.0f;
	float h = 0.0f;
	float offset = 5.0f / window->scale;
	qboolean hasForce = cg.enhanced.detected && (cg.enhanced.flags & BASE_ENHANCED_TEAMOVERLAY_FORCE);
	qboolean hasOffsetScale = cg.playerPredicted || hasForce;

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE) {
		return; // Not on any team
	}

	if (ci->team != cg.snap->ps.persistant[PERS_TEAM]) {
		return;
	}

	if (!demoTargetEntity(window->clientNum)) {
		return;
	}

	textOffsetScale = hasOffsetScale ? 3.0f : 2.0f;
	s = va("%i", ci->health);
	textHeight = CG_Text_Height(s, 0.7f/window->scale, FONT_SMALL);
	y = SCREEN_HEIGHT - (textHeight * textOffsetScale)/window->scale - offset;
	CG_Text_Paint((x/window->scale + offset)*cgs.widthRatioCoef, y,
		0.7f/window->scale, colorTable[CT_HUD_RED], s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_SMALL);

	textOffsetScale = hasOffsetScale ? 2.0f : 1.0f;
	armor = hasForce ? (ci->powerups >> 24) : ci->armor;
	s = va("%i", armor);
	textHeight = CG_Text_Height(s, 0.7f/window->scale, FONT_SMALL);
	y = SCREEN_HEIGHT - (textHeight * textOffsetScale)/window->scale - offset;
	CG_Text_Paint((x/window->scale + offset)*cgs.widthRatioCoef, y,
		0.7f/window->scale, colorTable[CT_HUD_GREEN], s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_SMALL);

	if (hasOffsetScale) {
		s = va("%i", cg.playerPredicted ? cg.snap->ps.fd.forcePower : ci->armor);
		textHeight = CG_Text_Height(s, 0.7f / window->scale, FONT_SMALL);
		y = SCREEN_HEIGHT - (textHeight) / window->scale - offset;
		CG_Text_Paint((x / window->scale + offset)*cgs.widthRatioCoef, y,
			0.7f / window->scale, colorTable[CT_ICON_BLUE], s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_SMALL);
	}

	trap_R_SetColor(NULL);
	offset = 2.0f / window->scale;
	h = 32.0f/window->scale*cgs.widthRatioCoef;
	w = h*cgs.widthRatioCoef;
	x = SCREEN_WIDTH - w - offset;
	y = SCREEN_HEIGHT - h - offset;
	if ( cg_weapons[ci->curWeapon].weaponIcon ) {
		CG_DrawPic( x, y, w, h, cg_weapons[ci->curWeapon].weaponIcon );
	} else {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
	y -= h;
	
	for (i = 0; i <= PW_NUM_POWERUPS; i++) {
		if (ci->powerups & (1 << i)) {
			item = BG_FindItemForPowerup( i );
			if (item) {
				CG_DrawPic( x, y, w, h, trap_R_RegisterShader( item->icon ) );
				y -= h;
			}
		}
	}
}

/*
===================
CG_DrawWindow2D

Draw the desired 2D effects in the POV window
===================
*/
static void multiSpecDrawWindow2D(multiSpecWindow_t *window, qboolean invalid) {
	clientInfo_t *ci;

	if (cg_draw2D.integer == 0) {
		return;
	}

	if (!window->draw2D) {
		return;
	}

	if (window->clientNum < 0) {
		return;
	}

	ci = &cgs.clientinfo[window->clientNum];

	multiSpec.draw2D = window;

	if (!invalid) {
		CG_DrawZoomMask();
		if (!(cg.playerCent->currentState.eFlags & EF_DEAD))
			CG_DrawCrosshairNames();
	}
//	CG_UpdateFallVector();
	multiSpecDrawFollow(ci, window);
	multiSpecDrawHUD(ci, window);

	multiSpec.draw2D = NULL;
	cg.crosshairClientNum = ENTITYNUM_NONE;
}

static void multiSpecDrawWindowBorder(multiSpecWindow_t *window) {
	vec4_t colour = { window->borderColour[0], window->borderColour[1], window->borderColour[2], 1.0f };
	CG_DrawRect(window->x, window->y, SCREEN_WIDTH * window->scale, SCREEN_HEIGHT * window->scale, 0.2f * window->borderWidth, colour);
}

/*
=====================
CG_DrawPov

Draw the Point of view of the current player window
=====================
*/
extern int demoSetupView( void);
static qboolean multiSpecDrawWindow(multiSpecWindow_t *window)	{
	refdef_t refdef = cg.refdef;
	const char* cstr;
	centity_t* cent;
	int weapon;
	qboolean invalid = qtrue;

	CG_FillRect(window->x, window->y, SCREEN_WIDTH * window->scale, SCREEN_HEIGHT * window->scale, colorBlack);
	if (cg.demoPlayback == 2 && window->clientNum < 0) {
		goto doScene;
	}
	if (!cg.snap || window->clientNum < 0 || window->clientNum >= MAX_CLIENTS) {
		multiSpecDrawWindowBorder(window);
		goto skipValid;
	}
	if (!multiSpecAllowed(window->clientNum)) {
		multiSpecDrawWindowBorder(window);
		if (window->draw2D && cg_draw2D.integer) {
			multiSpec.draw2D = window;
			multiSpecDrawFollow(&cgs.clientinfo[window->clientNum], window);
			multiSpec.draw2D = NULL;
		}
		goto skipValid;
	}
	cent = demoTargetEntity(window->clientNum);
	if (!cent) {
		goto skipScene;
	}
doScene:
	invalid = qfalse;
	trap_R_ClearScene();
	CG_ReTransitionSnapshot();

	CG_PreparePacketEntities( );

	weapon = cent->currentState.weapon;
	cg.trueView = (((weapon == WP_SABER || weapon == WP_MELEE) && !cg_trueGuns.integer)
		|| (weapon != WP_SABER && weapon != WP_MELEE && cg_trueGuns.integer));
	cg.zoomMode = 0;

	if (cg.demoPlayback == 2 && window->clientNum < 0) {
		demoViewType_t viewType = demo.viewType;
		demo.viewType = window->clientNum == -2 ? viewCamera : viewChase;
		demoSetupView();
		demo.viewType = viewType;
	} else {
		cg.playerCent = cent;
		cg.playerPredicted = cent == &cg_entities[cg.snap->ps.clientNum];
		if (!cg.playerPredicted) {
			//Make sure lerporigin of playercent is val
			CG_CalcEntityLerpPositions(cg.playerCent);
		}
		if (cg.playerPredicted) {
			if (cg.predictedPlayerState.zoomMode)
				cg.zoomMode = cg.predictedPlayerState.zoomMode;
			else if (cg.snap->ps.zoomMode && cg.demoPlayback)
				cg.zoomMode = cg.snap->ps.zoomMode;
			cg.renderingThirdPerson = qtrue; //always draw thirdperson in multispec?
			CG_SetPredictedThirdPerson();
		} else {
			cg.zoomMode = cg.playerCent->currentState.torsoAnim == TORSO_WEAPONREADY4
				|| cg.playerCent->currentState.torsoAnim == BOTH_ATTACK4;
			cg.renderingThirdPerson = (((cg_thirdPerson.integer % 2) || cent->currentState.eFlags & EF_DEAD
				|| (weapon == WP_SABER && !cg.trueView)
				|| (weapon == WP_MELEE && !cg.trueView))
				&& !cg.zoomMode);
		}
		CG_CalcViewValues();
	}

	if (cg.playerPredicted)
		cg.fallingToDeath = cg.snap->ps.fallingToDeath;
	else if ((cg.playerCent && (cg.fallingToDeath + 5000) < cg.time
		&& !(cg.playerCent->currentState.eFlags & EF_DEAD)) || !cg.playerCent)
		cg.fallingToDeath = 0;

	if (cg_linearFogOverride)
		trap_R_SetRangeFog(-cg_linearFogOverride);
	else if (cg_rangedFogging) {
		cg_rangedFogging = qfalse;
		trap_R_SetRangeFog(0.0f);
	}

	cstr = CG_ConfigString(CS_SKYBOXORG);
	if (cstr && cstr[0]) { //we have a skyportal
		CG_DrawSkyBoxPortal(cstr);
	}

//	CG_CalcScreenEffects();
	CG_AddPacketEntities(qfalse);	// after calcViewValues, so predicted player state is correct
	CG_AddMarks();
	CG_AddParticles();
	CG_AddLocalEntities();

	if (cg.playerCent == &cg_entities[cg.predictedPlayerState.clientNum]) {
		// warning sounds when powerup is wearing off
		CG_AddViewWeapon(&cg.predictedPlayerState);
	} else if (!(cg.predictedPlayerState.pm_type == PM_INTERMISSION) &&
		cg.playerCent && cg.playerCent->currentState.number < MAX_CLIENTS)  {
		CG_AddViewWeaponDirect(cg.playerCent);
	}
	trap_FX_AddScheduledEffects(qfalse);

	//do we need this?
/*	if (gCGHasFallVector) {
		vec3_t lookAng;
		cg.renderingThirdPerson = qtrue;
		VectorSubtract(cg.playerCent->lerpOrigin, cg.refdef.vieworg, lookAng);
		VectorNormalize(lookAng);
		vectoangles(lookAng, lookAng);
		VectorCopy(gCGFallVector, cg.refdef.vieworg);
		AnglesToAxis(lookAng, cg.refdef.viewaxis);
	}*/

	//do we need this here?
	CG_DrawAutoMap();

	cg.refdef.rdflags |= RDF_DRAWSKYBOX | RDF_NOGLOW;
	cg.refdef.x = window->x * cgs.screenXScale;
	cg.refdef.y = window->y * cgs.screenYScale;
	cg.refdef.height = cgs.glconfig.vidHeight * window->scale;
	cg.refdef.width = cgs.glconfig.vidWidth * window->scale;
	cg.refdef.time = cg.time;
	memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof(cg.refdef.areamask));
	trap_MME_TimeFraction(cg.timeFraction);
	trap_R_RenderScene(&cg.refdef);
skipScene:
	multiSpecDrawWindowBorder(window);
	multiSpecDrawWindow2D(window, invalid);
	cg.refdef = refdef;
skipValid:
	return !invalid;
}

static void multiSpecDrawEdit(void) {
	multiSpecWindow_t *window = multiSpec.edit.window;
	float x,y,w,h;

	if (!multiSpec.edit.active)
		return;

	if (window) {
		const char *s;
		switch (window->clientNum) {
			case -2:
				s = "camera";
				break;
			case -1:
				s = "chase";
				break;
			default:
				s = va("%d", window->clientNum);
				break;
		}
		CG_DrawRect(window->x, window->y, SCREEN_WIDTH * window->scale, SCREEN_HEIGHT * window->scale, 2.0f, colorWhite);
		CG_Text_Paint(window->x + SCREEN_WIDTH * window->scale - 7.0f*cgs.widthRatioCoef - CG_Text_Width(s, 0.7f, FONT_SMALL), window->y + 2.0f, 0.7f,
			colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_SMALL);
	}

	x = multiSpec.edit.cursorX;
	y = multiSpec.edit.cursorY;
	w = 40.0f;
	h = 40.0f;
	CG_DrawPic(x,y,w*cgs.widthRatioCoef,h, multiSpec.cursor);
}

void CG_MultiSpecAdjust2D(float *x, float *y, float *w, float *h) {
	if (multiSpec.active && multiSpec.draw2D) {
		float s = multiSpec.draw2D->scale;
		*x = multiSpec.draw2D->x + *x*s;
		*y = multiSpec.draw2D->y + *y*s;
		*w *= s;
		*h *= s;
	}
}

void CG_MultiSpecAdjustString(float *x, float *y, float *scale) {
	if (multiSpec.active && multiSpec.draw2D) {
		float s = multiSpec.draw2D->scale;
		*x = multiSpec.draw2D->x + *x*s;
		*y = multiSpec.draw2D->y + *y*s;
		*scale *= s;
	}
}

qboolean CG_MultiSpecActive(void)
{
	return multiSpec.active;
}

qboolean CG_MultiSpecSkipBackground(void)
{
	//return true if not to draw the main scene
	//but only if we have some windows
	return multiSpec.windows && multiSpec.background && cg.predictedPlayerState.pm_type != PM_INTERMISSION && cg.playerCent;
}

void CG_MultiSpecDrawBackground(void)
{
	if (CG_MultiSpecSkipBackground()) {
		if (multiSpec.background < 0)
			CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorBlack);
		else
			CG_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, multiSpec.background);
	}
}

qboolean CG_MultiSpecEditing(void)
{
	return multiSpec.edit.active;
}

float CG_MultiSpecFov(void)
{
	return multiSpec.current ? multiSpec.current->fov : cg_fov.value;
}

qboolean CG_MultiSpecTeamOverlayReplace(void)
{
	return multiSpec.windows && multiSpec.teamoverlay.replace;
}

float CG_MultiSpecTeamOverlayOffset(void)
{
	return multiSpec.windows ? multiSpec.teamoverlay.offset : 0.0f;
}

/*
===================
CG_MultiSpecInit

Entry point, called in cg_draw.c
===================
*/
void CG_MultiSpecInit(void) {
	memset(&multiSpec, 0, sizeof(multiSpec_t));
	multiSpec.edit.downX = multiSpec.edit.downY = -1.0f;
	multiSpec.cursor = trap_R_RegisterShaderNoMip("cursor");
}

void CG_MultiSpecShutDown(void) {
	
}

/*
===================
CG_MultiSpecMain

The whole multispec logic is located here
===================
*/
void CG_MultiSpecMain(void)
{
	centity_t *playerCent = cg.playerCent;
	qboolean playerPredicted = cg.playerPredicted;
	qboolean renderingThirdPerson = cg.renderingThirdPerson;
	qboolean trueView = cg.trueView;
	int zoomMode = cg.zoomMode;
	snapshot_t *snap = cg.snap, *nextSnap = cg.nextSnap;
	qboolean windowDrawn = qfalse;
	multiSpecWindow_t *window;

	if (!multiSpec.windows) {
		return;
	}
	if (cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		return;
	}
	if (!cg.playerCent) {
//		return;
	}
	multiSpec.active = qtrue;
	for (window = multiSpec.windows; window; window = window->next) {
		multiSpec.current = window;
		windowDrawn |= multiSpecDrawWindow(window);
		multiSpec.current = NULL;
	}
	//restore
	multiSpec.active = qfalse;
	cg.playerCent = playerCent;
	cg.playerPredicted = playerPredicted;
	cg.renderingThirdPerson = renderingThirdPerson;
	cg.trueView = trueView;
	cg.zoomMode = zoomMode;
	cg.snap = snap;
	cg.nextSnap = nextSnap;
	multiSpecDrawEdit();
}


static void multiSpecParseWindow( char *overrideString, int index ) {
	int argc = trap_Argc();
	char cmdType[64];
	int odd = index & 1;

	while ( index < argc ) {
		const char *line = CG_Argv( index );
		if ((index & 1 && !odd) || (!(index & 1) && odd)) {
			Info_SetValueForKey( overrideString, cmdType, line );
		} else {
			Q_strncpyz( cmdType, line, sizeof( cmdType ));
		}
		index++;
	}
}

static void multiSpecClampWindowBounds(multiSpecWindow_t *window) {
	if (window->x < 0)
		window->x = 0;
	else if ((window->x + SCREEN_WIDTH * window->scale) > SCREEN_WIDTH)
		window->x = (1.0f - window->scale) * SCREEN_WIDTH;
	if (window->y < 0)
		window->y = 0;
	else if ((window->y + SCREEN_HEIGHT * window->scale) > SCREEN_HEIGHT)
		window->y = (1.0f - window->scale) * SCREEN_HEIGHT;
}

static multiSpecWindow_t *multiSpecGetWindow(float x, float y) {
	multiSpecWindow_t	*window, *crossedWindow = NULL;
	float				x2, y2;
	for (window = multiSpec.windows; window; window = window->next) {
		x2 = window->x + SCREEN_WIDTH * window->scale;
		y2 = window->y + SCREEN_HEIGHT * window->scale;
		if (x >= window->x && x <= x2 && y >= window->y && y <= y2)
			crossedWindow = window;
	}
	return crossedWindow;
}

static qboolean multiSpecAllowed(int clientNum) {
	return (cg.demoPlayback == 2 && clientNum < 0)
		|| (clientNum >= 0 &&
		cgs.clientinfo[clientNum].team != TEAM_SPECTATOR
		&& (cg.demoPlayback
		|| cg.snap->ps.pm_flags & PMF_FOLLOW
		|| cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR
		  || ((cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED
			|| cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_BLUE)
			&& cgs.clientinfo[cg.snap->ps.clientNum].team == cgs.clientinfo[clientNum].team)));
}

static qboolean multiSpecAllowedAny(void) {
	int i;
	for (i = 0; i < MAX_CLIENTS; i++)
		if (multiSpecAllowed(i))
			return qtrue;
	return qfalse;
}

extern qboolean chasePrevTarget( int *oldTarget );
static int multiSpecGetPrev(int clientNum) {
	int oldClientNum = clientNum;
	do {
		chasePrevTarget(&clientNum);
		if (multiSpecAllowed(clientNum))
			break;
	} while (oldClientNum != clientNum);

	if (cg.demoPlayback == 2) {
		if (oldClientNum == -1)
			return -2;
		else if (oldClientNum != -2 && clientNum >= oldClientNum)
			return -1;
	}
	return clientNum;
}

extern qboolean chaseNextTarget( int *oldTarget );
static int multiSpecGetNext(int clientNum) {
	int oldClientNum = clientNum;
	do {
		chaseNextTarget(&clientNum);
		if (multiSpecAllowed(clientNum))
			break;
	} while (oldClientNum != clientNum);

	if (cg.demoPlayback == 2) {
		if (clientNum <= oldClientNum)
			return -2;
		else if (oldClientNum == -2)
			return -1;
	}
	return clientNum;
}

static void multiSpecFollowPrev() {
	if (!multiSpec.edit.window)
		return;
	multiSpec.edit.window->clientNum = multiSpecGetPrev(multiSpec.edit.window->clientNum);
}

static void multiSpecFollowNext() {
	if (!multiSpec.edit.window)
		return;
	multiSpec.edit.window->clientNum = multiSpecGetNext(multiSpec.edit.window->clientNum);
}

int CG_MultiSpecKeyEvent(int key, qboolean down) {
	multiSpecWindow_t *window = multiSpec.edit.window;
	int oldCatcher = trap_Key_GetCatcher();

	if (oldCatcher & KEYCATCH_CONSOLE || oldCatcher & KEYCATCH_UI || oldCatcher & KEYCATCH_MESSAGE)
		return qfalse;
	
	if (!multiSpec.edit.active)
		return qfalse;

	if (!down && (key == A_ESCAPE || key == A_ENTER || key == A_KP_ENTER)) {
		trap_SendConsoleCommand("multispec edit;");
		return qtrue;
	}

	if (down && key != A_MOUSE1)
		return qfalse;

	if (!window && !down && key == A_MOUSE1) {
		multiSpec.edit.downX = -1.0f;
		multiSpec.edit.downY = -1.0f;
	}

	if (!window)
		return qfalse;

	if (key == A_MOUSE1) {
		if (down) {
			if (multiSpec.edit.downX < 0.0f && multiSpec.edit.downY < 0.0f) {
				multiSpec.edit.downX = window->x;
				multiSpec.edit.downY = window->y;
			}
		} else {
			if (multiSpec.edit.downX == window->x
				&& multiSpec.edit.downY == window->y) {
				multiSpecFollowNext();
			}
			multiSpec.edit.downX = -1.0f;
			multiSpec.edit.downY = -1.0f;
		}
	} else if (key == A_MOUSE2) {
		multiSpecFollowPrev();
	} else if (key == A_MWHEELUP || key == A_MWHEELDOWN) {
		float oldScale = window->scale;
		if (trap_Key_IsDown(A_CTRL))
			window->scale += key == A_MWHEELUP ? 0.2f : -0.2f;
		else
			window->scale += key == A_MWHEELUP ? 0.02f : -0.02f;
		window->scale = Com_Clamp(MIN_SCALE, MAX_SCALE, window->scale);
		window->x += (oldScale - window->scale) * SCREEN_WIDTH * 0.5f;
		window->y += (oldScale - window->scale) * SCREEN_HEIGHT * 0.5f;
		multiSpecClampWindowBounds(window);
	} else if (key == A_DELETE || key == A_KP_PERIOD || key == A_BACKSPACE) {
		int i = multiSpecWindowNum(window);
		if (i >= 0 && multiSpecWindowDel(i))
			multiSpec.edit.window = NULL;
		if (!multiSpec.windows)
			trap_SendConsoleCommand("multispec edit;");
	}
	return qtrue;
}
void CG_MultiSpecMouseEvent(int dx, int dy) {
	multiSpecWindow_t *window = multiSpec.edit.window;

	// update mouse screen position
	multiSpec.edit.cursorX += dx * cgs.widthRatioCoef;
	if (multiSpec.edit.cursorX < 0)
		multiSpec.edit.cursorX = 0;
	else if (multiSpec.edit.cursorX > SCREEN_WIDTH)
		multiSpec.edit.cursorX = SCREEN_WIDTH;

	multiSpec.edit.cursorY += dy;
	if (multiSpec.edit.cursorY < 0)
		multiSpec.edit.cursorY = 0;
	else if (multiSpec.edit.cursorY > SCREEN_HEIGHT)
		multiSpec.edit.cursorY = SCREEN_HEIGHT;

	if (multiSpec.edit.downX < 0.0f && multiSpec.edit.downY < 0.0f)
		window = multiSpecGetWindow(multiSpec.edit.cursorX, multiSpec.edit.cursorY);

	if (window && multiSpec.edit.downX >= 0.0f && multiSpec.edit.downY >= 0.0f) {
		window->x += dx * cgs.widthRatioCoef;
		window->y += dy;
		multiSpecClampWindowBounds(window);
	}
	multiSpec.edit.window = window;
}


/*
===================
CG_MultiSpec_f

Get arguments from the console
===================
*/
void CG_MultiSpec_f(void) {
	int i;
	multiSpecWindow_t *window;
	int argc = trap_Argc();
	const char *cmd = CG_Argv(1);
	char windowConfig[MAX_INFO_STRING] = { 0 };

	if (argc < 2) {
		CG_Printf("multispec usage:\n");
		CG_Printf("list, view current settings, red tag means an override\n");
		CG_Printf("del id, delete window with id\n" );
		CG_Printf("clear, delete all windows\n" );
		CG_Printf("background (shader), toggle showing main scene, optional shader to fill whole screen\n" );
		CG_Printf("teamoverlay (replace/offset/scale), multispec for teamoverlay, optional replace to hide/show original or offset it, or scale for all windows\n" );
		CG_Printf("or\n" );
		CG_Printf("add, add a new multispec window\n");
		CG_Printf("edit (id), toggle editing mode, optional id to update\n");
		CG_Printf("Followed by a list of 2 parameter combinations\n" );
		CG_Printf("x \"X\", Set x coordinate\n" );
		CG_Printf("y \"Y\", Set y coordinate\n" );
		CG_Printf("s \"scale\", Set window scale\n" );
		CG_Printf("client \"0-31\", Set window pov on a client\n" );
		CG_Printf("fov \"1-180\", Set player FOV\n");
		CG_Printf("2d \"0,1\", Draw 2D for that client\n" );
		CG_Printf("bw \"width\", Set window border width\n" );
		CG_Printf("bc \"0-9,#hexcode\", Set window border colour\n" );
		return;
	}
	if (!Q_stricmp( cmd, "list")) {
		for (i = 0; i < MAX_CLIENTS; i++) {
			const char *configString;

			configString = CG_ConfigString(i + CS_PLAYERS);
			if (!configString[0])
				continue;

			Com_Printf("client %-2d %s\n", i, Info_ValueForKey(configString, "n"));
		}
		Com_Printf("\n");
		for (window = multiSpec.windows, i = 0; window; window = window->next, i++) {
			Com_Printf("window %-2d x %9.4f y %9.4f s %9.4f client %d fov %9.4f 2d %d\n", i, window->x, window->y, window->scale, window->clientNum, window->fov, window->draw2D);
		}
		return;
	} else if  (!Q_stricmp( cmd, "add" )) {
		if (!multiSpecAllowedAny()) {
			CG_Printf("Multispec is only allowed in spec or team gamemode\n");
			return;
		}
		multiSpecParseWindow( windowConfig, 2 );
		if (multiSpecWindowAdd( windowConfig ) && !multiSpec.edit.active)
			goto edit;
	} else if  (!Q_stricmp( cmd, "del" )) {
		if (argc < 3) {
			CG_Printf("Missing window id for deleteion, use /multispec list, to see all ids.\n");
			return;
		}
		i = atoi ( CG_Argv(2) );
		if (!multiSpecWindowDel( i ) ) {
			CG_Printf("Illegal window id %d\n", i);
			return;
		}
		if (!multiSpec.windows && multiSpec.edit.active)
			goto edit;
	} else if  (!Q_stricmp( cmd, "clear" )) {
		while (multiSpec.windows)
			multiSpecWindowDel( 0 );
		if (!multiSpec.windows && multiSpec.edit.active)
			goto edit;
	} else if  (!Q_stricmp( cmd, "edit" )) {
		int oldCatcher;
		if (argc != 2) {
			multiSpecWindow_t *window;
			if (argc < 3) {
				CG_Printf("Missing window id to edit, use /multispec list, to see all ids.\n");
				return;
			} else if (argc < 5) {
				CG_Printf("Too few parameters, use /multispec, to see all parameters.\n");
				return;
			}
			i = atoi ( CG_Argv(2) );
			window = multiSpecWindowGet( i );
			if (!window ) {
				CG_Printf("Illegal window id %d\n", i);
				return;
			}
			multiSpecParseWindow( windowConfig, 3 );
			multiSpecWindowUpdate( window, windowConfig, qtrue );
			return;
		}
edit:
		multiSpec.edit.downX = -1.0f;
		multiSpec.edit.downY = -1.0f;
		if (!multiSpec.windows && !multiSpec.edit.active) {
			CG_Printf("There are no windows to edit, use /multispec add, to add new window.\n");
			return;
		}
		oldCatcher = trap_Key_GetCatcher();
		if (oldCatcher & KEYCATCH_CGAME) {
			oldCatcher &= ~KEYCATCH_CGAME;
			multiSpec.edit.active = qfalse;
		} else {
			oldCatcher |= KEYCATCH_CGAME;
			multiSpec.edit.active = qtrue;
		}
		trap_Key_SetCatcher(oldCatcher);
	} else if (!Q_stricmp( cmd, "background" )) {
		cmd = CG_Argv(2);
		if (!cmd[0]) {
			if (multiSpec.background)
				multiSpec.background = 0;
			else
				multiSpec.background = -1;
		} else if (cmd[0] == '0' && !cmd[1]) {
			multiSpec.background = 0;
		} else if (cmd[0]) {
			i = trap_R_RegisterShader(cmd);
			multiSpec.background = i ? i : -1;
		}
	} else if (!Q_stricmp( cmd, "teamoverlay" )) {
		if (!multiSpecAllowedAny()) {
			CG_Printf("Multispec is only allowed in spec or team gamemode\n");
			return;
		}
		cmd = CG_Argv(2);
		if (!Q_stricmp( cmd, "replace" )) {
			multiSpec.teamoverlay.replace = !multiSpec.teamoverlay.replace;
		} else if  (!Q_stricmp( cmd, "offset" )) {
			cmd = CG_Argv(3);
			if(!cmd[0])
				multiSpec.teamoverlay.offset = TEAMOVERLAY_SCALE * SCREEN_HEIGHT;
			else
				multiSpec.teamoverlay.offset = Com_Clamp(0.0f, TEAMOVERLAY_MAX_SCALE * SCREEN_HEIGHT, atof(cmd) * SCREEN_HEIGHT);
		} else {
			float x, y, scale;
			if ( !cg.playerCent ) {
				CG_Printf("You need to follow someone\n");
				return;
			}
			if ( cgs.clientinfo[cg.playerCent->currentState.number].team != TEAM_RED && cgs.clientinfo[cg.playerCent->currentState.number].team != TEAM_BLUE ) {
				CG_Printf("You are not on any team\n");
				return;
			}
			if (!cmd[0])
				scale = TEAMOVERLAY_SCALE;
			else
				scale = Com_Clamp(MIN_SCALE, TEAMOVERLAY_MAX_SCALE, atof(cmd));
			x = (1.0f - scale) * SCREEN_WIDTH;
			y = 0.0f;
			while (multiSpec.windows)
				multiSpecWindowDel( 0 );
			for (i = 0; i < MAX_CLIENTS; i++) {
				if (i == cg.playerCent->currentState.number)
					continue;
				if (cgs.clientinfo[i].team != cgs.clientinfo[cg.playerCent->currentState.number].team)
					continue;
				if (x < 0.0f) {
					x = (1.0f - scale) * SCREEN_WIDTH;
					y += scale * SCREEN_HEIGHT;
				}
				Com_sprintf(windowConfig, sizeof(windowConfig), "x\\%f\\y\\%f\\s\\%f\\client\\%d\\", x, y, scale, i);
				multiSpecWindowAdd( windowConfig );
				x -= scale * SCREEN_WIDTH;
			}
			multiSpec.teamoverlay.replace = qtrue;
			multiSpec.teamoverlay.offset = Com_Clamp(0.0f, TEAMOVERLAY_MAX_SCALE * SCREEN_HEIGHT, y + scale * SCREEN_HEIGHT);
		}
	}
}

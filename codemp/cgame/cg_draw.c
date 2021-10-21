// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay
#include "cg_local.h"
#include "../game/bg_saga.h"
#include "../ui/ui_shared.h"
#include "../ui/ui_public.h"
#include "cg_multispec.h"
extern float CG_RadiusForCent( centity_t *cent );
qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y);
qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle );
static void CG_DrawSiegeTimer(int timeRemaining, qboolean isMyTeam);
static void CG_DrawSiegeDeathTimer( int timeRemaining );
static void CG_StrafeHelperActiveColorChange(void);
static void CG_CrosshairColorChange(void);



static void CG_StrafeHelper( centity_t *cent );//japro start
static void CG_CalculateSpeed(centity_t *cent);
static void CG_Speedometer(void);
static void CG_DrawAccelMeter(void);
static void CG_MovementKeys(centity_t *cent);
static void CG_JumpHeight(centity_t *cent);
static void CG_RaceTimer(void);
static void CG_JumpDistance( void );
static void CG_DrawVerticalSpeed(void);
static void CG_DrawYawSpeed(void);
static void CG_DrawTrajectoryLine(void);

#define SHELPER_SUPEROLDSTYLE	(1<<0)
#define SHELPER_OLDSTYLE		(1<<1)
#define SHELPER_NEWBARS			(1<<2)
#define SHELPER_OLDBARS			(1<<3)
#define SHELPER_SOUND			(1<<4)
#define SHELPER_W				(1<<5)
#define SHELPER_WA				(1<<6)
#define SHELPER_WD				(1<<7)
#define SHELPER_A				(1<<8)
#define SHELPER_D				(1<<9)
#define SHELPER_REAR			(1<<10)
#define SHELPER_CENTER			(1<<11)
#define SHELPER_ACCELMETER		(1<<12)
#define SHELPER_WEZE			(1<<13)
#define SHELPER_CROSSHAIR		(1<<14)

#define SPEEDOMETER_ENABLE			(1<<0)
#define SPEEDOMETER_GROUNDSPEED		(1<<1)
#define SPEEDOMETER_JUMPHEIGHT		(1<<2)
#define SPEEDOMETER_JUMPDISTANCE	(1<<3)
#define SPEEDOMETER_VERTICALSPEED	(1<<4)
#define SPEEDOMETER_YAWSPEED		(1<<5)
#define SPEEDOMETER_ACCELMETER		(1<<6)
#define SPEEDOMETER_SPEEDGRAPH		(1<<7)
#define SPEEDOMETER_KPH				(1<<8)
#define SPEEDOMETER_MPH				(1<<9)
//japro end

// nmckenzie: DUEL_HEALTH
void CG_DrawDuelistHealth ( float x, float y, float w, float h, int duelist );

// used for scoreboard
extern displayContextDef_t cgDC;
vec4_t	bluehudtint = {0.5, 0.5, 1.0, 1.0};
vec4_t	redhudtint = {1.0, 0.5, 0.5, 1.0};
float	*hudTintColor;
menuDef_t *menuScoreboard = NULL;


int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

float lastvalidlockdif;

extern float zoomFov; //this has to be global client-side

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

// The time at which you died and the time it will take for you to rejoin game.
int cg_siegeDeathTime = 0;

#define MAX_HUD_TICS 4
const char *armorTicName[MAX_HUD_TICS] = 
{
"armor_tic1", 
"armor_tic2", 
"armor_tic3", 
"armor_tic4", 
};

const char *healthTicName[MAX_HUD_TICS] = 
{
"health_tic1", 
"health_tic2", 
"health_tic3", 
"health_tic4", 
};

const char *forceTicName[MAX_HUD_TICS] = 
{
"force_tic1", 
"force_tic2", 
"force_tic3", 
"force_tic4", 
};

const char *ammoTicName[MAX_HUD_TICS] = 
{
"ammo_tic1", 
"ammo_tic2", 
"ammo_tic3", 
"ammo_tic4", 
};

char *showPowersName[] = 
{
	"HEAL2",//FP_HEAL
	"JUMP2",//FP_LEVITATION
	"SPEED2",//FP_SPEED
	"PUSH2",//FP_PUSH
	"PULL2",//FP_PULL
	"MINDTRICK2",//FP_TELEPTAHY
	"GRIP2",//FP_GRIP
	"LIGHTNING2",//FP_LIGHTNING
	"DARK_RAGE2",//FP_RAGE
	"PROTECT2",//FP_PROTECT
	"ABSORB2",//FP_ABSORB
	"TEAM_HEAL2",//FP_TEAM_HEAL
	"TEAM_REPLENISH2",//FP_TEAM_FORCE
	"DRAIN2",//FP_DRAIN
	"SEEING2",//FP_SEE
	"SABER_OFFENSE2",//FP_SABER_OFFENSE
	"SABER_DEFENSE2",//FP_SABER_DEFENSE
	"SABER_THROW2",//FP_SABERTHROW
	NULL
};

//Called from UI shared code. For now we'll just redirect to the normal anim load function.


int UI_ParseAnimationFile(const char *filename, animation_t *animset, qboolean isHumanoid) 
{
	return BG_ParseAnimationFile(filename, animset, isHumanoid);
}

int MenuFontToHandle(int iMenuFont)
{
	switch (iMenuFont)
	{
		case FONT_SMALL:	return cgDC.Assets.qhSmallFont;
		case FONT_SMALL2:	return cgDC.Assets.qhSmall2Font;
		case FONT_MEDIUM:	return cgDC.Assets.qhMediumFont;
		case FONT_LARGE:	return cgDC.Assets.qhMediumFont;//cgDC.Assets.qhBigFont;
			//fixme? Big fonr isn't registered...?
	}

	return cgDC.Assets.qhMediumFont;
}


int CG_Text_Width(const char *text, float scale, int iMenuFont) 
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap_R_Font_StrLenPixels(text, iFontIndex, scale);
}

int CG_Text_Height(const char *text, float scale, int iMenuFont) 
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap_R_Font_HeightPixels(iFontIndex, scale);
}

#include "../qcommon/qfiles.h"	// for STYLE_BLINK etc
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style, int iMenuFont) 
{
	int iStyleOR = 0;
	int iFontIndex = MenuFontToHandle(iMenuFont);
	
	switch (style)
	{
	case  ITEM_TEXTSTYLE_NORMAL:			iStyleOR = 0;break;					// JK2 normal text
	case  ITEM_TEXTSTYLE_BLINK:				iStyleOR = STYLE_BLINK;break;		// JK2 fast blinking
	case  ITEM_TEXTSTYLE_PULSE:				iStyleOR = STYLE_BLINK;break;		// JK2 slow pulsing
	case  ITEM_TEXTSTYLE_SHADOWED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINESHADOWED:	iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_SHADOWEDMORE:		iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	}

	trap_R_Font_DrawString(	x,		// int ox
							y,		// int oy
							text,	// const char *text
							color,	// paletteRGBA_c c
							iStyleOR | iFontIndex,	// const int iFontHandle
							!limit?-1:limit,		// iCharLimit (-1 = none)
							scale	// const float scale = 1.0f
							);
}

/*
qboolean CG_WorldCoordToScreenCoord(vec3_t worldCoord, int *x, int *y)

  Take any world coord and convert it to a 2D virtual 640x480 screen coord
*/
/*
qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y)
{
	int	xcenter, ycenter;
	vec3_t	local, transformed;

//	xcenter = cg.refdef.width / 2;//gives screen coords adjusted for resolution
//	ycenter = cg.refdef.height / 2;//gives screen coords adjusted for resolution
	
	//NOTE: did it this way because most draw functions expect virtual 640x480 coords
	//	and adjust them for current resolution
	xcenter = 640 / 2;//gives screen coords in virtual 640x480, to be adjusted when drawn
	ycenter = 480 / 2;//gives screen coords in virtual 640x480, to be adjusted when drawn

	VectorSubtract (worldCoord, cg.refdef.vieworg, local);

	transformed[0] = DotProduct(local,vright);
	transformed[1] = DotProduct(local,vup);
	transformed[2] = DotProduct(local,vfwd);		

	// Make sure Z is not negative.
	if(transformed[2] < 0.01)
	{
		return qfalse;
	}
	// Simple convert to screen coords.
	float xzi = xcenter / transformed[2] * (90.0/cg.refdef.fov_x);
	float yzi = ycenter / transformed[2] * (90.0/cg.refdef.fov_y);

	*x = xcenter + xzi * transformed[0];
	*y = ycenter - yzi * transformed[1];

	return qtrue;
}

qboolean CG_WorldCoordToScreenCoord( vec3_t worldCoord, int *x, int *y )
{
	float	xF, yF;
	qboolean retVal = CG_WorldCoordToScreenCoordFloat( worldCoord, &xF, &yF );
	*x = (int)xF;
	*y = (int)yF;
	return retVal;
}
*/

/*
================
CG_DrawZoomMask

================
*/
extern void trap_R_RotatePic2RatioFix(float ratio);
void CG_DrawZoomMask(void) {
	vec4_t		color1;
	float		level;
	static qboolean	flip = qtrue;
	float ratio = (cgs.media.zoomLeft || cgs.media.zoomRight) ? cgs.widthRatioCoef : 1.0f;
//	int ammo = cg_entities[0].gent->client->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex];
	float cx, cy;
//	int val[5];
	float max, fi;
	// Check for Binocular specific zooming since we'll want to render different bits in each case
	if ( cg.playerPredicted && cg.predictedPlayerState.zoomMode == 2 )
	{
		int val, i;
		float off;

		// zoom level
		level = (float)(80.0f - cg.predictedPlayerState.zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to scale amount
		level *= 162.0f;

		// draw blue tinted distortion mask, trying to make it as small as is necessary to fill in the viewable area
		trap_R_SetColor( colorTable[CT_WHITE] );
		CG_DrawPic( 34, 48, 570, 362, cgs.media.binocularStatic );
	
		// Black out the area behind the numbers
		trap_R_SetColor( colorTable[CT_BLACK]);
		CG_DrawPic( 212, 367, 200, 40, cgs.media.whiteShader );

		// Numbers should be kind of greenish
		color1[0] = 0.2f;
		color1[1] = 0.4f;
		color1[2] = 0.2f;
		color1[3] = 0.3f;
		trap_R_SetColor( color1 );

		// Draw scrolling numbers, use intervals 10 units apart--sorry, this section of code is just kind of hacked
		//	up with a bunch of magic numbers.....
		val = ((int)((cg.refdef.viewangles[YAW] + 180) / 10)) * 10;
		off = (cg.refdef.viewangles[YAW] + 180) - val;

		for ( i = -10; i < 30; i += 10 )
		{
			val -= 10;

			if ( val < 0 )
			{
				val += 360;
			}

			// we only want to draw the very far left one some of the time, if it's too far to the left it will
			//	poke outside the mask.
			if (( off > 3.0f && i == -10 ) || i > -10 )
			{
				// draw the value, but add 200 just to bump the range up...arbitrary, so change it if you like
				CG_DrawNumField( 320 - (320 - (155 + i * 10 + off * 10))*ratio, 374, 3, val + 200, 24*ratio, 14, NUM_FONT_CHUNKY, qtrue );
				CG_DrawPic( 320 - (320 - (245 + (i-1) * 10 + off * 10))*ratio, 376, 6*ratio, 6, cgs.media.whiteShader );
			}
		}

		CG_DrawPic( 212, 367, 200, 28, cgs.media.binocularOverlay );

		color1[0] = sin( cg.time * 0.01 + cg.timeFraction * 0.01 ) * 0.5f + 0.5f;
		color1[0] = color1[0] * color1[0];
		color1[1] = color1[0];
		color1[2] = color1[0];
		color1[3] = 1.0f;

		trap_R_SetColor( color1 );

		CG_DrawPic( 82, 94, 16*ratio, 16, cgs.media.binocularCircle );

		// Flickery color
		color1[0] = 0.7f + crandom() * 0.1f;
		color1[1] = 0.8f + crandom() * 0.1f;
		color1[2] = 0.7f + crandom() * 0.1f;
		color1[3] = 1.0f;
		trap_R_SetColor( color1 );
	
		CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, cgs.media.binocularMask );

		CG_DrawPic( 4, 282 - level, 16, 16, cgs.media.binocularArrow );

		// The top triangle bit randomly flips 
		if ( flip )
		{
			CG_DrawPic( 330, 60, -26, -30, cgs.media.binocularTri );
		}
		else
		{
			CG_DrawPic( 307, 40, 26, 30, cgs.media.binocularTri );
		}

		if ( random() > 0.98f && ( cg.time & 1024 ))
		{
			flip = !flip;
		}
	}
	else if (cg.zoomMode)
	{
		// disruptor zoom mode
		level = (float)(50.0f - zoomFov) / 50.0f;//(float)(80.0f - zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		if (!cg.playerPredicted)
			level = 0.46f;

		// Using a magic number to convert the zoom level to a rotation amount that correlates more or less with the zoom artwork. 
		level *= 103.0f;

		// Draw target mask
		trap_R_SetColor( colorTable[CT_WHITE] );
		if (ratio != 1.0f) {
			if (cgs.media.zoomLeft) {
				CG_DrawPic(0, 0, SCREEN_WIDTH / 2 - (SCREEN_WIDTH / 2)*ratio, SCREEN_HEIGHT, cgs.media.zoomLeft);
				if (!cgs.media.zoomRight) {
					CG_DrawPic(SCREEN_WIDTH, 0, -(SCREEN_WIDTH / 2 - (SCREEN_WIDTH / 2)*ratio), SCREEN_HEIGHT, cgs.media.zoomLeft);
				}
			}
			if (cgs.media.zoomRight) {
				CG_DrawPic(SCREEN_WIDTH / 2 + (SCREEN_WIDTH / 2)*ratio, 0, SCREEN_WIDTH / 2 - (SCREEN_WIDTH / 2)*ratio, SCREEN_HEIGHT, cgs.media.zoomRight);
				if (!cgs.media.zoomLeft) {
					CG_DrawPic(SCREEN_WIDTH / 2 - (SCREEN_WIDTH / 2)*ratio, 0, -(SCREEN_WIDTH / 2 - (SCREEN_WIDTH / 2)*ratio), SCREEN_HEIGHT, cgs.media.zoomRight);
				}
			}
		}
		CG_DrawPic(SCREEN_WIDTH / 2 - (SCREEN_WIDTH / 2)*ratio, 0, SCREEN_WIDTH*ratio, SCREEN_HEIGHT, cgs.media.disruptorMask);

		// apparently 99.0f is the full zoom level
		if ( level >= 99 )
		{
			// Fully zoomed, so make the rotating insert pulse
			color1[0] = 1.0f; 
			color1[1] = 1.0f;
			color1[2] = 1.0f;
			color1[3] = 0.7f + sin( cg.time * 0.01 + cg.timeFraction * 0.01 ) * 0.3f;

			trap_R_SetColor( color1 );
		}

		// Draw rotating insert
		trap_R_RotatePic2RatioFix(ratio);
		CG_DrawRotatePic2( SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT, -level, cgs.media.disruptorInsert );

		// Increase the light levels under the center of the target
//		CG_DrawPic( 198, 118, 246, 246, cgs.media.disruptorLight );

		// weirdness.....converting ammo to a base five number scale just to be geeky.
/*		val[0] = ammo % 5;
		val[1] = (ammo / 5) % 5;
		val[2] = (ammo / 25) % 5;
		val[3] = (ammo / 125) % 5;
		val[4] = (ammo / 625) % 5;
		
		color1[0] = 0.2f;
		color1[1] = 0.55f + crandom() * 0.1f;
		color1[2] = 0.5f + crandom() * 0.1f;
		color1[3] = 1.0f;
		trap_R_SetColor( color1 );

		for ( int t = 0; t < 5; t++ )
		{
			cx = 320 + sin( (t*10+45)/57.296f ) * 192;
			cy = 240 + cos( (t*10+45)/57.296f ) * 192;

			CG_DrawRotatePic2( cx, cy, 24, 38, 45 - t * 10, trap_R_RegisterShader( va("gfx/2d/char%d",val[4-t] )));
		}
*/
		//max = ( cg_entities[0].gent->health / 100.0f );

		if (cg.playerPredicted) {
			if ( (cg.snap->ps.eFlags & EF_DOUBLE_AMMO) )
				max = cg.snap->ps.ammo[weaponData[WP_DISRUPTOR].ammoIndex] / ((float)ammoData[weaponData[WP_DISRUPTOR].ammoIndex].max*2.0f);
			else
				max = cg.snap->ps.ammo[weaponData[WP_DISRUPTOR].ammoIndex] / (float)ammoData[weaponData[WP_DISRUPTOR].ammoIndex].max;
			if ( max > 1.0f )
				max = 1.0f;
		} else {
			max = 0.5f; // let it be half
		}

		color1[0] = (1.0f - max) * 2.0f; 
		color1[1] = max * 1.5f;
		color1[2] = 0.0f;
		color1[3] = 1.0f;

		// If we are low on health, make us flash
		if ( max < 0.15f && ( cg.time & 512 ))
		{
			VectorClear( color1 );
		}

		if ( color1[0] > 1.0f )
		{
			color1[0] = 1.0f;
		}

		if ( color1[1] > 1.0f )
		{
			color1[1] = 1.0f;
		}

		trap_R_SetColor( color1 );

		max *= 58.0f;

		for (fi = 18.5f; fi <= 18.5f + max; fi+= 3 ) // going from 15 to 45 degrees, with 5 degree increments
		{
			cx = SCREEN_WIDTH / 2 + sin( (fi+90.0f)/57.296f ) * 190*ratio;
			cy = SCREEN_HEIGHT / 2 + cos( (fi+90.0f)/57.296f ) * 190;

			CG_DrawRotatePic2( cx, cy, 12, 24, 90 - fi, cgs.media.disruptorInsertTick );
		}
		trap_R_RotatePic2RatioFix(1.0f);

		if ((cg.playerPredicted && cg.predictedPlayerState.weaponstate == WEAPON_CHARGING_ALT)
			|| (!cg.playerPredicted && cg.charging && cg.chargeTime && cg.time > cg.chargeTime))
		{
			trap_R_SetColor( colorTable[CT_WHITE] );

			// draw the charge level
			if (cg.playerPredicted)
				max = ((cg.time - cg.predictedPlayerState.weaponChargeTime) + cg.timeFraction) / (50.0f * 30.0f); // bad hardcodedness 50 is disruptor charge unit and 30 is max charge units allowed.
			else
				max = ((cg.time - cg.chargeTime) + cg.timeFraction) / (50.0f * 30.0f);

			if ( max > 1.0f )
			{
				max = 1.0f;
			}

			trap_R_DrawStretchPic(SCREEN_WIDTH / 2 - (SCREEN_WIDTH / 2 - 257)*ratio, 435, 134*max*ratio, 34, 0, 0, max, 1, cgs.media.disruptorChargeShader);
		}
//		trap_R_SetColor( colorTable[CT_WHITE] );
//		CG_DrawPic( 0, 0, 640, 480, cgs.media.disruptorMask );

	}
}


/*
================
CG_Draw3DModel

================
*/
extern void trap_MME_TimeFraction( float timeFraction );
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, void *ghoul2, int g2radius, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3DIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.ghoul2 = ghoul2;
	ent.radius = g2radius;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;
	trap_MME_TimeFraction(cg.timeFraction);

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) 
{
	clientInfo_t	*ci;

	if (clientNum >= MAX_CLIENTS)
	{ //npc?
		return;
	}

	ci = &cgs.clientinfo[ clientNum ];

	CG_DrawPic( x, y, w, h, ci->modelIcon );

	// if they are deferred, draw a cross out
	if ( ci->deferred ) 
	{
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3DIcons.integer ) {

		//Raz: need to adjust the coords only for 3d models
		//		2d icons use virtual screen coords
		x *= cgs.screenXScale;
		y *= cgs.screenYScale;
		w *= cgs.screenXScale;
		h *= cgs.screenYScale;

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin((double)((double)cg.time + cg.timeFraction) / 2000.0);;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = 0;//cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, NULL, 0, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawHealth
================
*/
void CG_DrawHealth( menuDef_t *menuHUD )
{
	vec4_t			calcColor;
	playerState_t	*ps;
	int				healthAmt;
	int				i,currValue,inc;
	itemDef_t		*focusItem;
	float percent;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	ps = &cg.snap->ps;

	// What's the health?
	healthAmt = cg.playerPredicted ? ps->stats[STAT_HEALTH] : cgs.clientinfo[cg.playerCent->currentState.number].health;
	if (healthAmt > ps->stats[STAT_MAX_HEALTH])
	{
		healthAmt = ps->stats[STAT_MAX_HEALTH];
	}


	inc = (float) ps->stats[STAT_MAX_HEALTH] / MAX_HUD_TICS;
	currValue = healthAmt;

	// Print the health tics, fading out the one which is partial health
	for (i=(MAX_HUD_TICS-1);i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, healthTicName[i]);

		if (!focusItem)	// This is bad
		{
			continue;
		}

		memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			percent = (float) currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		trap_R_SetColor( calcColor);

		CG_DrawPic( 
			focusItem->window.rect.x*cgs.widthRatioCoef,
			focusItem->window.rect.y,
			focusItem->window.rect.w*cgs.widthRatioCoef, 
			focusItem->window.rect.h, 
			focusItem->window.background
			);

		currValue -= inc;
	}

	healthAmt = cg.playerPredicted ? ps->stats[STAT_HEALTH] : cgs.clientinfo[cg.playerCent->currentState.number].health;
	// Print the mueric amount
	focusItem = Menu_FindItemByName(menuHUD, "healthamount");
	if (focusItem)
	{
		// Print health amount
		trap_R_SetColor( focusItem->window.foreColor );	

		CG_DrawNumField (
			focusItem->window.rect.x*cgs.widthRatioCoef, 
			focusItem->window.rect.y, 
			3, 
			healthAmt,
			focusItem->window.rect.w*cgs.widthRatioCoef, 
			focusItem->window.rect.h, 
			NUM_FONT_SMALL,
			qfalse);
	}

}

/*
================
CG_DrawArmor
================
*/
void CG_DrawArmor( menuDef_t *menuHUD )
{
	vec4_t			calcColor;
	playerState_t	*ps;
	int				maxArmor;
	itemDef_t		*focusItem;
	float			percent,quarterArmor;
	int				i,currValue,inc;

	//ps = &cg.snap->ps;
	ps = &cg.predictedPlayerState;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	maxArmor = ps->stats[STAT_MAX_HEALTH];

	currValue = cg.playerPredicted ? ps->stats[STAT_ARMOR] : cgs.clientinfo[cg.playerCent->currentState.number].armor;
	inc = (float) maxArmor / MAX_HUD_TICS;

	memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
	for (i=(MAX_HUD_TICS-1);i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, armorTicName[i]);

		if (!focusItem)	// This is bad
		{
			continue;
		}

		memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			percent = (float) currValue / inc;
			calcColor[3] *= percent;
		}

		trap_R_SetColor( calcColor);

		if ((i==(MAX_HUD_TICS-1)) && (currValue < inc))
		{
			if (cg.HUDArmorFlag)
			{
				CG_DrawPic( 
					focusItem->window.rect.x*cgs.widthRatioCoef,
					focusItem->window.rect.y,
					focusItem->window.rect.w*cgs.widthRatioCoef, 
					focusItem->window.rect.h, 
					focusItem->window.background
					);
			}
		}
		else 
		{
				CG_DrawPic( 
					focusItem->window.rect.x*cgs.widthRatioCoef,
					focusItem->window.rect.y,
					focusItem->window.rect.w*cgs.widthRatioCoef, 
					focusItem->window.rect.h, 
					focusItem->window.background
					);
		}

		currValue -= inc;
	}

	currValue = cg.playerPredicted ? ps->stats[STAT_ARMOR] : cgs.clientinfo[cg.playerCent->currentState.number].armor;

	focusItem = Menu_FindItemByName(menuHUD, "armoramount");

	if (focusItem)
	{
		// Print armor amount
		trap_R_SetColor( focusItem->window.foreColor );	

		CG_DrawNumField (
			focusItem->window.rect.x*cgs.widthRatioCoef, 
			focusItem->window.rect.y, 
			3, 
			currValue,
			focusItem->window.rect.w*cgs.widthRatioCoef, 
			focusItem->window.rect.h, 
			NUM_FONT_SMALL,
			qfalse);
	}

	// If armor is low, flash a graphic to warn the player
	if (currValue)	// Is there armor? Draw the HUD Armor TIC
	{
		quarterArmor = (float) (ps->stats[STAT_MAX_HEALTH] / 4.0f);

		// Make tic flash if armor is at 25% of full armor
		if (currValue < quarterArmor)		// Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time)			// Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 400;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag=qtrue;
		}
	}
	else						// No armor? Don't show it.
	{
		cg.HUDArmorFlag=qfalse;
	}

}

/*
================
CG_DrawSaberStyle

If the weapon is a light saber (which needs no ammo) then draw a graphic showing
the saber style (fast, medium, strong)
================
*/
static void CG_DrawSaberStyle( centity_t *cent, menuDef_t *menuHUD)
{
	itemDef_t		*focusItem;

	if (!cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	if ( cent->currentState.weapon != WP_SABER )
	{
		return;
	}

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}


	// draw the current saber style in this window
	switch ( cg.predictedPlayerState.fd.saberDrawAnimLevel )
	{
	case 1://FORCE_LEVEL_1:
	case 5://FORCE_LEVEL_5://Tavion

		focusItem = Menu_FindItemByName(menuHUD, "saberstyle_fast");

		if (focusItem)
		{
			trap_R_SetColor( colorTable[CT_WHITE] );

			CG_DrawPic( 
				SCREEN_WIDTH - (SCREEN_WIDTH - focusItem->window.rect.x)*cgs.widthRatioCoef,
				focusItem->window.rect.y,
				focusItem->window.rect.w*cgs.widthRatioCoef, 
				focusItem->window.rect.h, 
				focusItem->window.background
				);
		}

		break;
	case 2://FORCE_LEVEL_2:
	case 6://SS_DUAL
	case 7://SS_STAFF
		focusItem = Menu_FindItemByName(menuHUD, "saberstyle_medium");

		if (focusItem)
		{
			trap_R_SetColor( colorTable[CT_WHITE] );

			CG_DrawPic( 
				SCREEN_WIDTH - (SCREEN_WIDTH - focusItem->window.rect.x)*cgs.widthRatioCoef,
				focusItem->window.rect.y,
				focusItem->window.rect.w*cgs.widthRatioCoef, 
				focusItem->window.rect.h, 
				focusItem->window.background
				);
		}
		break;
	case 3://FORCE_LEVEL_3:
	case 4://FORCE_LEVEL_4://Desann
		focusItem = Menu_FindItemByName(menuHUD, "saberstyle_strong");

		if (focusItem)
		{
			trap_R_SetColor( colorTable[CT_WHITE] );

			CG_DrawPic( 
				SCREEN_WIDTH - (SCREEN_WIDTH - focusItem->window.rect.x)*cgs.widthRatioCoef,
				focusItem->window.rect.y,
				focusItem->window.rect.w*cgs.widthRatioCoef, 
				focusItem->window.rect.h, 
				focusItem->window.background
				);
		}
		break;
	}

}

/*
================
CG_DrawAmmo
================
*/
static void CG_DrawAmmo( centity_t	*cent,menuDef_t *menuHUD)
{
	playerState_t	*ps;
	int				i;
	vec4_t			calcColor;
	float			value=0.0f,inc = 0.0f,percent;
	itemDef_t		*focusItem;

	ps = &cg.snap->ps;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	if (!cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	value = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];
	if (value < 0)	// No ammo
	{
		return;
	}

	//
	// ammo
	//
	if (cg.oldammo < value)
	{
		cg.oldAmmoTime = cg.time + 200;
	}

	cg.oldammo = value;

	focusItem = Menu_FindItemByName(menuHUD, "ammoamount");

	if (weaponData[cent->currentState.weapon].energyPerShot == 0 &&
		weaponData[cent->currentState.weapon].altEnergyPerShot == 0)
	{ //just draw "infinite"
		inc = 8 / MAX_HUD_TICS;
		value = 8;

		focusItem = Menu_FindItemByName(menuHUD, "ammoinfinite");
		trap_R_SetColor( colorTable[CT_YELLOW] );
		if (focusItem)
		{
			UI_DrawProportionalString(SCREEN_WIDTH - (SCREEN_WIDTH - focusItem->window.rect.x)*cgs.widthRatioCoef, focusItem->window.rect.y, "--", NUM_FONT_SMALL, focusItem->window.foreColor);
		}
	}
	else
	{
		focusItem = Menu_FindItemByName(menuHUD, "ammoamount");

		// Firing or reloading?
		if (( cg.predictedPlayerState.weaponstate == WEAPON_FIRING
			&& cg.predictedPlayerState.weaponTime > 100 ))
		{
			memcpy(calcColor, colorTable[CT_LTGREY], sizeof(vec4_t));
		} 
		else 
		{
			if ( value > 0 ) 
			{
				if (cg.oldAmmoTime > cg.time)
				{
					memcpy(calcColor, colorTable[CT_YELLOW], sizeof(vec4_t));
				}
				else
				{
					memcpy(calcColor, focusItem->window.foreColor, sizeof(vec4_t));
				}
			} 
			else 
			{
				memcpy(calcColor, colorTable[CT_RED], sizeof(vec4_t));
			}
		}


		trap_R_SetColor( calcColor );
		if (focusItem)
		{

			if ( (cent->currentState.eFlags & EF_DOUBLE_AMMO) )
			{
				inc = (float) (ammoData[weaponData[cent->currentState.weapon].ammoIndex].max*2.0f) / MAX_HUD_TICS;
			}
			else
			{
				inc = (float) ammoData[weaponData[cent->currentState.weapon].ammoIndex].max / MAX_HUD_TICS;
			}
			value =ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];

			CG_DrawNumField (
				SCREEN_WIDTH - (SCREEN_WIDTH - focusItem->window.rect.x)*cgs.widthRatioCoef, 
				focusItem->window.rect.y, 
				3, 
				value, 
				focusItem->window.rect.w*cgs.widthRatioCoef, 
				focusItem->window.rect.h, 
				NUM_FONT_SMALL,
				qfalse);
		}
	}

	trap_R_SetColor( colorTable[CT_WHITE] );

	// Draw tics
	for (i=MAX_HUD_TICS-1;i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, ammoTicName[i]);

		if (!focusItem)
		{
			continue;
		}

		memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));

		if ( value <= 0 )	// done
		{
			break;
		}
		else if (value < inc)	// partial tic
		{
			percent = value / inc;
			calcColor[3] = percent;
		}

		trap_R_SetColor( calcColor);

		CG_DrawPic( 
			SCREEN_WIDTH - (SCREEN_WIDTH - focusItem->window.rect.x)*cgs.widthRatioCoef,
			focusItem->window.rect.y,
			focusItem->window.rect.w*cgs.widthRatioCoef, 
			focusItem->window.rect.h, 
			focusItem->window.background
			);

		value -= inc;
	}

}

/*
================
CG_DrawForcePower
================
*/
void CG_DrawForcePower( menuDef_t *menuHUD )
{
	int				i;
	vec4_t			calcColor;
	float			value,inc,percent;
	itemDef_t		*focusItem;
	const int		maxForcePower = 100;
	qboolean	flash=qfalse;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	if (cg.forceHUDTotalFlashTime < cg.time) {
		cg.forceHUDTotalFlashTime = 0;
		cg.forceHUDNextFlashTime = 0;
	}// else {
//		cg.forceHUDTotalFlashTime = cg.time + 1000;
//	}

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.forceHUDTotalFlashTime > cg.time )
	{
		flash = qtrue;
		if (cg.forceHUDNextFlashTime < cg.time)	
		{
			cg.forceHUDNextFlashTime = cg.time + 400;
			trap_S_StartSound (NULL, 0, CHAN_LOCAL, cgs.media.noforceSound );

			if (cg.forceHUDActive)
			{
				cg.forceHUDActive = qfalse;
			}
			else
			{
				cg.forceHUDActive = qtrue;
			}

		}
	}
	else	// turn HUD back on if it had just finished flashing time.
	{
		cg.forceHUDNextFlashTime = 0;
		cg.forceHUDActive = qtrue;
	}

//	if (!cg.forceHUDActive)
//	{
//		return;
//	}

	inc = (float)  maxForcePower / MAX_HUD_TICS;
	value = cg.snap->ps.fd.forcePower;

	for (i=MAX_HUD_TICS-1;i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, forceTicName[i]);

		if (!focusItem)
		{
			continue;
		}

//		memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));

		if ( value <= 0 )	// done
		{
			break;
		}
		else if (value < inc)	// partial tic
		{
			if (flash)
			{
				memcpy(calcColor,  colorTable[CT_RED], sizeof(vec4_t));
			}
			else 
			{
				memcpy(calcColor,  colorTable[CT_WHITE], sizeof(vec4_t));
			}

			percent = value / inc;
			calcColor[3] = percent;
		}
		else
		{
			if (flash)
			{
				memcpy(calcColor,  colorTable[CT_RED], sizeof(vec4_t));
			}
			else 
			{
				memcpy(calcColor,  colorTable[CT_WHITE], sizeof(vec4_t));
			}
		}

		trap_R_SetColor( calcColor);

		CG_DrawPic( 
			SCREEN_WIDTH - (SCREEN_WIDTH - (float)focusItem->window.rect.x)*cgs.widthRatioCoef,
			focusItem->window.rect.y,
			focusItem->window.rect.w*cgs.widthRatioCoef, 
			focusItem->window.rect.h, 
			focusItem->window.background
			);

		value -= inc;
	}

	focusItem = Menu_FindItemByName(menuHUD, "forceamount");

	if (focusItem)
	{
		// Print force amount
		trap_R_SetColor( focusItem->window.foreColor );	

		CG_DrawNumField (
			SCREEN_WIDTH - (SCREEN_WIDTH - focusItem->window.rect.x)*cgs.widthRatioCoef, 
			focusItem->window.rect.y, 
			3, 
			cg.snap->ps.fd.forcePower, 
			focusItem->window.rect.w*cgs.widthRatioCoef, 
			focusItem->window.rect.h, 
			NUM_FONT_SMALL,
			qfalse);
	}
}

static void CG_DrawSimpleSaberStyle(const centity_t *cent)
{
    uint32_t	calcColor;
    char		num[7] = { 0 };
    int			weapX = 24;

    if ( !cent->currentState.weapon ) // We don't have a weapon right now
    {
        return;
    }

    if ( cent->currentState.weapon != WP_SABER )
    {
        return;
    }

    switch ( cg.predictedPlayerState.fd.saberDrawAnimLevel )
    {
        default:
        case SS_FAST:
            Com_sprintf( num, sizeof( num ), "FAST" );
            calcColor = CT_ICON_BLUE;
            weapX -= 8;
            break;
        case SS_MEDIUM:
            Com_sprintf( num, sizeof( num ), "MEDIUM" );
            calcColor = CT_YELLOW;
            break;
        case SS_STRONG:
            Com_sprintf( num, sizeof( num ), "STRONG" );
            calcColor = CT_HUD_RED;
            break;
        case SS_DESANN:
            Com_sprintf( num, sizeof( num ), "DESANN" );
            calcColor = CT_HUD_RED;
            break;
        case SS_TAVION:
            Com_sprintf( num, sizeof( num ), "TAVION" );
            calcColor = CT_ICON_BLUE;
            break;
        case SS_DUAL:
            Com_sprintf( num, sizeof( num ), "AKIMBO" );
            calcColor = CT_HUD_ORANGE;
            break;
        case SS_STAFF:
            Com_sprintf( num, sizeof( num ), "STAFF" );
            calcColor = CT_HUD_ORANGE;
            break;
    }

    if (cg_hudColors.integer) //JAPRO - Clientside - Gradient simple hud coloring
        CG_DrawProportionalString(SCREEN_WIDTH - (weapX + 16 + 32)*cgs.widthRatioCoef, (SCREEN_HEIGHT - 80) + 40, num, UI_SMALLFONT | UI_DROPSHADOW, colorTable[calcColor]);
    else
        CG_DrawProportionalString(SCREEN_WIDTH - (weapX + 16 + 32)*cgs.widthRatioCoef, (SCREEN_HEIGHT - 80) + 40, num, UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_ORANGE]);
}

/*
================
CG_DrawHUD
================
*/

float speedometerXPos;
void Dzikie_CG_DrawLine (float x1, float y1, float x2, float y2, float size, vec4_t color, float alpha, float ycutoff);
void CG_DrawHUD(centity_t	*cent) {
    menuDef_t *menuHUD = NULL;
    itemDef_t *focusItem = NULL;
    const char *scoreStr = NULL;
    int scoreBias;
    char scoreBiasStr[16];
    int score;

    if ((cg_speedometer.integer & SPEEDOMETER_ENABLE) || cg_strafeHelper.integer || cg_raceTimer.integer > 1 ||
        cg_showpos.integer)
        CG_CalculateSpeed(cent);

    //JAPRO - Clientside - Movement Keys Start
    if (cg_movementKeys.integer)
        CG_MovementKeys(cent);
    //JAPRO - Clientside - Speedometer Start
    speedometerXPos = cg_speedometerX.integer;
    /*if (cg_strafeHelper.integer)
        CG_StrafeHelper(cent);*/

    if (cgs.newHud) {
        switch (cg_hudFiles.integer) {
            case 0:
                speedometerXPos -= 8;
                break;
            case 1:
                speedometerXPos -= 56;
                break;
            case 2:
                speedometerXPos -= 42;
                break;
            default:
                break;
        }
    }

    if (cg_speedometer.integer & SPEEDOMETER_ENABLE) {
        CG_Speedometer();
        if (cg_speedometer.integer & SPEEDOMETER_ACCELMETER || cg_strafeHelper.integer & SHELPER_ACCELMETER)
            CG_DrawAccelMeter();
        if (cg_speedometer.integer & SPEEDOMETER_JUMPHEIGHT)
            CG_JumpHeight(cent);
        if (cg_speedometer.integer & SPEEDOMETER_JUMPDISTANCE)
            CG_JumpDistance();
        if (cg_speedometer.integer & SPEEDOMETER_VERTICALSPEED)
            CG_DrawVerticalSpeed();
        if (cg_speedometer.integer & SPEEDOMETER_YAWSPEED)
            CG_DrawYawSpeed();
    }

//JAPRO - Clientside - Strafehelper Start
    if (cg_strafeHelper.integer)
        CG_StrafeHelper(cent);
    if (cg_strafeHelper.integer & SHELPER_CROSSHAIR) {
        vec4_t hcolor;
        float lineWidth;

        if (!cg.crosshairColor[0] && !cg.crosshairColor[1] && !cg.crosshairColor[2]) { //default to white
            hcolor[0] = 1.0f;
            hcolor[1] = 1.0f;
            hcolor[2] = 1.0f;
            hcolor[3] = 1.0f;
        } else {
            hcolor[0] = cg.crosshairColor[0];
            hcolor[1] = cg.crosshairColor[1];
            hcolor[2] = cg.crosshairColor[2];
            hcolor[3] = cg.crosshairColor[3];
        }

        lineWidth = cg_strafeHelperLineWidth.value;
        if (lineWidth < 0.25f)
            lineWidth = 0.25f;
        else if (lineWidth > 5)
            lineWidth = 5;

        Dzikie_CG_DrawLine(SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2) - 5, SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2) + 5,
                           lineWidth * cgs.widthRatioCoef, hcolor, hcolor[3], 0); //640x480, 320x240
    }
    if (cg_raceTimer.integer)
        CG_RaceTimer();
    if (cg_speedometer.integer & SPEEDOMETER_SPEEDGRAPH)
//        CG_DrawSpeedGraph();

        if (!cg.playerPredicted) {
            if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE) {
                return; // Not on any team
            }
            if (cgs.clientinfo[cent->currentState.number].team != cg.snap->ps.persistant[PERS_TEAM]) {
                return;
            }
        }

    if (cg_hudFiles.integer) {
        int x = 0;
        int y = SCREEN_HEIGHT - 80;
        char ammoString[64];
        int weapX = x;
        int health;
        int armor;

            if (cg.playerPredicted) {
                health = cg.snap->ps.stats[STAT_HEALTH];
                armor = cg.snap->ps.stats[STAT_ARMOR];
            } else {
                health = cgs.clientinfo[cent->currentState.number].health;
                armor = cgs.clientinfo[cent->currentState.number].armor;
            }

            UI_DrawProportionalString((x + 16) * cgs.widthRatioCoef, y + 40, va("%i", health),
                                      UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_RED]);

            UI_DrawProportionalString((x + 18 + 14) * cgs.widthRatioCoef, y + 40 + 14, va("%i", armor),
                                      UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_GREEN]);

            if (!cg.playerPredicted)
                return;

            if (cg.snap->ps.weapon == WP_SABER) {
                if (cg.snap->ps.fd.saberDrawAnimLevel == SS_DUAL) {
                    Com_sprintf(ammoString, sizeof(ammoString), "AKIMBO");
                    weapX += 25;
                } else if (cg.snap->ps.fd.saberDrawAnimLevel == SS_STAFF) {
                    Com_sprintf(ammoString, sizeof(ammoString), "STAFF");
                    weapX += 25;
                } else if (cg.snap->ps.fd.saberDrawAnimLevel == FORCE_LEVEL_3) {
                    Com_sprintf(ammoString, sizeof(ammoString), "STRONG");
                    weapX += 25;
                } else if (cg.snap->ps.fd.saberDrawAnimLevel == FORCE_LEVEL_2) {
                    Com_sprintf(ammoString, sizeof(ammoString), "MEDIUM");
                    weapX += 25;
                } else {
                    Com_sprintf(ammoString, sizeof(ammoString), "FAST");
                    weapX += 20;
                }
            } else if (weaponData[cent->currentState.weapon].energyPerShot == 0 &&
                       weaponData[cent->currentState.weapon].altEnergyPerShot == 0) {
                Q_strncpyz(ammoString, "--", sizeof(ammoString));
            } else {
                Com_sprintf(ammoString, sizeof(ammoString), "%i",
                            cg.snap->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex]);
            }

            UI_DrawProportionalString(SCREEN_WIDTH - (weapX + 16 + 32) * cgs.widthRatioCoef, y + 40,
                                      va("%s", ammoString),
                                      UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_ORANGE]);

            UI_DrawProportionalString(SCREEN_WIDTH - (x + 18 + 14 + 32) * cgs.widthRatioCoef, y + 40 + 14,
                                      va("%i", cg.snap->ps.fd.forcePower),
                                      UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_ICON_BLUE]);
            return;
        }

    if (cg.predictedPlayerState.pm_type != PM_SPECTATOR)
    {
        if (cg_hudFiles.integer == 1)
        {
            int x = 0;
            int y = SCREEN_HEIGHT - 80;

            //JAPRO - Clientside - Gradient simple hud coloring - Start
            if (cg_hudColors.integer)
            {
                vec4_t colorHealth = { .835f,	.015f,  .015f,  1 };
                vec4_t colorArmor = { 0,	.613f,  .097f,  1 };

                if (cg.snap->ps.stats[STAT_HEALTH] < 100)
                {
                    colorHealth[1] = 0.215 - (cg.snap->ps.stats[STAT_HEALTH] * 0.002);
                    colorHealth[2] = 0.215 - (cg.snap->ps.stats[STAT_HEALTH] * 0.002);
                }

                if (cg.snap->ps.stats[STAT_ARMOR] < 100)
                {
                    colorArmor[0] = 0.2 - (cg.snap->ps.stats[STAT_ARMOR] * 0.002);
                    colorArmor[2] = 0.297 - (cg.snap->ps.stats[STAT_ARMOR] * 0.002);
                }

                CG_DrawProportionalString((x + 16)*cgs.widthRatioCoef, y + 40, va("%i", cg.snap->ps.stats[STAT_HEALTH]), UI_SMALLFONT | UI_DROPSHADOW, colorHealth);
                CG_DrawProportionalString((x + 18 + 14)*cgs.widthRatioCoef, y + 40 + 14, va("%i", cg.snap->ps.stats[STAT_ARMOR]), UI_SMALLFONT | UI_DROPSHADOW, colorArmor);

            }
            else
            {
                CG_DrawProportionalString((x + 16)*cgs.widthRatioCoef, y + 40, va("%i", cg.snap->ps.stats[STAT_HEALTH]), UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_RED]);
                CG_DrawProportionalString((x + 18 + 14)*cgs.widthRatioCoef, y + 40 + 14, va("%i", cg.snap->ps.stats[STAT_ARMOR]), UI_SMALLFONT | UI_DROPSHADOW, colorTable[CT_HUD_GREEN]);
            }
            //JAPRO - Clientside - Gradient simple hud coloring - End

            if (cent->currentState.weapon == WP_SABER)
                CG_DrawSimpleSaberStyle(cent);

            // Draw the left HUD
            menuHUD = Menus_FindByName("lefthud");
            Menu_Paint(menuHUD, qtrue);

            if (menuHUD) {
                itemDef_t *focusItem;

                // Print scanline
                focusItem = Menu_FindItemByName(menuHUD, "scanline");
                if (focusItem) {
                    trap_R_SetColor(colorTable[CT_WHITE]);
                    CG_DrawPic(
                            focusItem->window.rect.x * cgs.widthRatioCoef,
                            focusItem->window.rect.y,
                            focusItem->window.rect.w * cgs.widthRatioCoef,
                            focusItem->window.rect.h,
                            focusItem->window.background
                    );
                }

                // Print frame
                focusItem = Menu_FindItemByName(menuHUD, "frame");
                if (focusItem) {
                    trap_R_SetColor(colorTable[CT_WHITE]);
                    CG_DrawPic(
                            focusItem->window.rect.x * cgs.widthRatioCoef,
                            focusItem->window.rect.y,
                            focusItem->window.rect.w * cgs.widthRatioCoef,
                            focusItem->window.rect.h,
                            focusItem->window.background
                    );
                }

                CG_DrawArmor(menuHUD);
                CG_DrawHealth(menuHUD);
            } else {
                //CG_Error("CG_ChatBox_ArrayInsert: unable to locate HUD menu file ");
            }

            if (cg.playerPredicted) {
                score = cg.snap->ps.persistant[PERS_SCORE];
            } else {
                score = cgs.clientinfo[cent->currentState.number].score;
            }

            //scoreStr = va("Score: %i", cgs.clientinfo[cg.snap->ps.clientNum].score);
            if (cgs.gametype ==
                GT_DUEL) {//A duel that requires more than one kill to knock the current enemy back to the queue
                //show current kills out of how many needed
                scoreStr = va("%s: %i/%i", CG_GetStringEdString("MP_INGAME", "SCORE"), score, cgs.fraglimit);
            } else if (0 && cgs.gametype < GT_TEAM) {    // This is a teamless mode, draw the score bias.
                scoreBias = score - cgs.scores1;
                if (scoreBias == 0) {    // We are the leader!
                    if (cgs.scores2 <= 0) {    // Nobody to be ahead of yet.
                        Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), "");
                    } else {
                        scoreBias = score - cgs.scores2;
                        if (scoreBias == 0) {
                            Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (Tie)");
                        } else {
                            Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (+%d)", scoreBias);
                        }
                    }
                } else // if (scoreBias < 0)
                {    // We are behind!
                    Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (%d)", scoreBias);
                }
                scoreStr = va("%s: %i%s", CG_GetStringEdString("MP_INGAME", "SCORE"), score, scoreBiasStr);
            } else {    // Don't draw a bias.
                scoreStr = va("%s: %i", CG_GetStringEdString("MP_INGAME", "SCORE"), score);
            }

            menuHUD = Menus_FindByName("righthud");
            Menu_Paint(menuHUD, qtrue);

            if (menuHUD) {
                if (cgs.gametype != GT_POWERDUEL) {
                    focusItem = Menu_FindItemByName(menuHUD, "score_line");
                    if (focusItem) {
                        UI_DrawScaledProportionalString(
                                SCREEN_WIDTH - (SCREEN_WIDTH - focusItem->window.rect.x) * cgs.widthRatioCoef - 17,
                                focusItem->window.rect.y,
                                scoreStr,
                                UI_RIGHT | UI_DROPSHADOW,
                                focusItem->window.foreColor,
                                0.7f);
                    }
                }

                if (!cg.playerPredicted)
                    return;

                // Print scanline
                focusItem = Menu_FindItemByName(menuHUD, "scanline");
                if (focusItem) {
                    trap_R_SetColor(colorTable[CT_WHITE]);
                    CG_DrawPic(
                            SCREEN_WIDTH - (SCREEN_WIDTH - focusItem->window.rect.x) * cgs.widthRatioCoef,
                            focusItem->window.rect.y,
                            focusItem->window.rect.w * cgs.widthRatioCoef,
                            focusItem->window.rect.h,
                            focusItem->window.background
                    );
                }

                focusItem = Menu_FindItemByName(menuHUD, "frame");
                if (focusItem) {
                    trap_R_SetColor(colorTable[CT_WHITE]);
                    CG_DrawPic(
                            SCREEN_WIDTH - (SCREEN_WIDTH - focusItem->window.rect.x) * cgs.widthRatioCoef,
                            focusItem->window.rect.y,
                            focusItem->window.rect.w * cgs.widthRatioCoef,
                            focusItem->window.rect.h,
                            focusItem->window.background
                    );
                }

                CG_DrawForcePower(menuHUD);

                // Draw ammo tics or saber style
                if (cent->currentState.weapon == WP_SABER) {
                    CG_DrawSaberStyle(cent, menuHUD);
                } else {
                    CG_DrawAmmo(cent, menuHUD);
                }
            } else {
                //CG_Error("CG_ChatBox_ArrayInsert: unable to locate HUD menu file ");
            }
        }
    }
}

#define MAX_SHOWPOWERS NUM_FORCE_POWERS

qboolean ForcePower_Valid(int i)
{
	if (i == FP_LEVITATION ||
		i == FP_SABER_OFFENSE ||
		i == FP_SABER_DEFENSE ||
		i == FP_SABERTHROW)
	{
		return qfalse;
	}

	if (cg.snap->ps.fd.forcePowersKnown & (1 << i))
	{
		return qtrue;
	}
	
	return qfalse;
}

/*
===================
CG_DrawForceSelect
===================
*/
void CG_DrawForceSelect( void ) 
{
	int		i;
	int		count;
	int		smallIconSize,bigIconSize;
	float	holdX,x,y,x2,y2,pad,length;
	int		sideLeftIconCnt,sideRightIconCnt;
	int		sideMax,holdCount,iconCnt;
	int		yOffset = 0;


	x2 = 0;
	y2 = 0;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) 
	{
		return;
	}

	if ((cg.forceSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		return;
	}

	if (!cg.snap->ps.fd.forcePowersKnown)
	{
		return;
	}

	// count the number of powers owned
	count = 0;

	for (i=0;i < NUM_FORCE_POWERS;++i)
	{
		if (ForcePower_Valid(i))
		{
			count++;
		}
	}

	if (count == 0)	// If no force powers, don't display
	{
		return;
	}

	sideMax = 3;	// Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	smallIconSize = 30;
	bigIconSize = 60;
	pad = 12;

	x = 320;
	y = 425;

	// Background
	length = (sideLeftIconCnt * smallIconSize) + (sideLeftIconCnt*pad) +
			bigIconSize + (sideRightIconCnt * smallIconSize) + (sideRightIconCnt*pad) + 12;
	
	i = BG_ProperForceIndex(cg.forceSelect) - 1;
	if (i < 0)
	{
		i = MAX_SHOWPOWERS - 1;
	}

	trap_R_SetColor(NULL);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize)*cgs.widthRatioCoef;
	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if (i < 0)
		{
			i = MAX_SHOWPOWERS - 1;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y + yOffset, smallIconSize*cgs.widthRatioCoef, smallIconSize, cgs.media.forcePowerIcons[forcePowerSorted[i]] ); 
			holdX -= (smallIconSize+pad)*cgs.widthRatioCoef;
		}
	}

	if (ForcePower_Valid(cg.forceSelect))
	{
		// Current Center Icon
		if (cgs.media.forcePowerIcons[cg.forceSelect])
		{
			CG_DrawPic( x-(bigIconSize/2)*cgs.widthRatioCoef, (y-((bigIconSize-smallIconSize)/2)) + yOffset, bigIconSize*cgs.widthRatioCoef, bigIconSize, cgs.media.forcePowerIcons[cg.forceSelect] ); //only cache the icon for display
		}
	}

	i = BG_ProperForceIndex(cg.forceSelect) + 1;
	if (i>=MAX_SHOWPOWERS)
	{
		i = 0;
	}

	// Work forwards from current icon
	holdX = x + ((bigIconSize/2) + pad)*cgs.widthRatioCoef;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if (i>=MAX_SHOWPOWERS)
		{
			i = 0;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y + yOffset, smallIconSize*cgs.widthRatioCoef, smallIconSize, cgs.media.forcePowerIcons[forcePowerSorted[i]] ); //only cache the icon for display
			holdX += (smallIconSize+pad)*cgs.widthRatioCoef;
		}
	}

	if ( showPowersName[cg.forceSelect] ) 
	{
		UI_DrawProportionalString(320, y + 30 + yOffset, CG_GetStringEdString("SP_INGAME", showPowersName[cg.forceSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	}
}

/*
===================
CG_DrawInventorySelect
===================
*/
void CG_DrawInvenSelect( void ) 
{
	int				i;
	int				sideMax,holdCount,iconCnt;
	int				smallIconSize,bigIconSize;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				count;
	float			holdX,x,y,y2,pad;
	int				height;
	float			addX;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) 
	{
		return;
	}

	if ((cg.invenSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	if (!cg.snap->ps.stats[STAT_HOLDABLE_ITEM] || !cg.snap->ps.stats[STAT_HOLDABLE_ITEMS])
	{
		return;
	}

	if (cg.itemSelect == -1)
	{
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
	}

//const int bits = cg.snap->ps.stats[ STAT_ITEMS ];

	// count the number of items owned
	count = 0;
	for ( i = 0 ; i < HI_NUM_HOLDABLE ; i++ ) 
	{
		if (/*CG_InventorySelectable(i) && inv_icons[i]*/
			(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) ) 
		{
			count++;
		}
	}

	if (!count)
	{
		y2 = 0; //err?
		UI_DrawProportionalString(320, y2 + 22, "EMPTY INVENTORY", UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
		return;
	}

	sideMax = 3;	// Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	i = cg.itemSelect - 1;
	if (i<0)
	{
		i = HI_NUM_HOLDABLE-1;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 16;

	x = 320;
	y = 410;

	// Left side ICONS
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize)*cgs.widthRatioCoef;
	height = smallIconSize * cg.iconHUDPercent;
	addX = (float) smallIconSize * .75;

	for (iconCnt=0;iconCnt<sideLeftIconCnt;i--)
	{
		if (i<0)
		{
			i = HI_NUM_HOLDABLE-1;
		}

		if ( !(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) || i == cg.itemSelect )
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (!BG_IsItemSelectable(&cg.predictedPlayerState, i))
		{
			continue;
		}

		if (cgs.media.invenIcons[i])
		{
			trap_R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize*cgs.widthRatioCoef, smallIconSize, cgs.media.invenIcons[i] );

			trap_R_SetColor(colorTable[CT_ICON_BLUE]);
			/*CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12, 
				NUM_FONT_SMALL,qfalse);
				*/

			holdX -= (smallIconSize+pad)*cgs.widthRatioCoef;
		}
	}

	// Current Center Icon
	height = bigIconSize * cg.iconHUDPercent;
	if (cgs.media.invenIcons[cg.itemSelect] && BG_IsItemSelectable(&cg.predictedPlayerState, cg.itemSelect))
	{
		int itemNdex;
		trap_R_SetColor(NULL);
		CG_DrawPic( x-(bigIconSize/2)*cgs.widthRatioCoef, (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize*cgs.widthRatioCoef, bigIconSize, cgs.media.invenIcons[cg.itemSelect] );
		addX = (float) bigIconSize * .75;
		trap_R_SetColor(colorTable[CT_ICON_BLUE]);
		/*CG_DrawNumField ((x-(bigIconSize/2)) + addX, y, 2, cg.snap->ps.inventory[cg.inventorySelect], 6, 12, 
			NUM_FONT_SMALL,qfalse);*/

		itemNdex = BG_GetItemIndexByTag(cg.itemSelect, IT_HOLDABLE);
		if (bg_itemlist[itemNdex].classname)
		{
			vec4_t	textColor = { .312f, .75f, .621f, 1.0f };
			char	text[1024];
			char	upperKey[1024];

			strcpy(upperKey, bg_itemlist[itemNdex].classname);
			
			if ( trap_SP_GetStringTextString( va("SP_INGAME_%s",Q_strupr(upperKey)), text, sizeof( text )))
			{
				UI_DrawProportionalString(320, y+45, text, UI_CENTER | UI_SMALLFONT, textColor);
			}
			else
			{
				UI_DrawProportionalString(320, y+45, bg_itemlist[itemNdex].classname, UI_CENTER | UI_SMALLFONT, textColor);
			}
		}
	}

	i = cg.itemSelect + 1;
	if (i> HI_NUM_HOLDABLE-1)
	{
		i = 0;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + ((bigIconSize/2) + pad)*cgs.widthRatioCoef;
	height = smallIconSize * cg.iconHUDPercent;
	addX = (float) smallIconSize * .75;
	for (iconCnt=0;iconCnt<sideRightIconCnt;i++)
	{
		if (i> HI_NUM_HOLDABLE-1)
		{
			i = 0;
		}

		if ( !(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) || i == cg.itemSelect )
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (!BG_IsItemSelectable(&cg.predictedPlayerState, i))
		{
			continue;
		}

		if (cgs.media.invenIcons[i])
		{
			trap_R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize*cgs.widthRatioCoef, smallIconSize, cgs.media.invenIcons[i] );

			trap_R_SetColor(colorTable[CT_ICON_BLUE]);
			/*CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12, 
				NUM_FONT_SMALL,qfalse);*/

			holdX += (smallIconSize+pad)*cgs.widthRatioCoef;
		}
	}
}

int cg_targVeh = ENTITYNUM_NONE;
int cg_targVehLastTime = 0;
qboolean CG_CheckTargetVehicle( centity_t **pTargetVeh, float *alpha )
{
	int targetNum = ENTITYNUM_NONE;
	centity_t	*targetVeh = NULL;
	
	if ( !pTargetVeh || !alpha )
	{//hey, where are my pointers?
		return qfalse;
	}

	*alpha = 1.0f;

	//FIXME: need to clear all of these when you die?
	if ( cg.predictedPlayerState.rocketLockIndex < ENTITYNUM_WORLD )
	{
		targetNum = cg.predictedPlayerState.rocketLockIndex;
	}
	else if ( cg.crosshairVehNum < ENTITYNUM_WORLD 
		&& cg.time - cg.crosshairVehTime < 3000 )
	{//crosshair was on a vehicle in the last 3 seconds
		targetNum = cg.crosshairVehNum;
	}
    else if ( cg.crosshairClientNum < ENTITYNUM_WORLD )
	{
		targetNum = cg.crosshairClientNum;
	}

	if ( targetNum < MAX_CLIENTS )
	{//real client
		if ( cg_entities[targetNum].currentState.m_iVehicleNum >= MAX_CLIENTS )
		{//in a vehicle
			targetNum = cg_entities[targetNum].currentState.m_iVehicleNum;
		}
	}
    if ( targetNum < ENTITYNUM_WORLD 
		&& targetNum >= MAX_CLIENTS )
	{
		//centity_t *targetVeh = &cg_entities[targetNum];
		targetVeh = &cg_entities[targetNum];
		if ( targetVeh->currentState.NPC_class == CLASS_VEHICLE 
			&& targetVeh->m_pVehicle
			&& targetVeh->m_pVehicle->m_pVehicleInfo
			&& targetVeh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
		{//it's a vehicle
			cg_targVeh = targetNum;
			cg_targVehLastTime = cg.time;
			*alpha = 1.0f;
		}
		else
		{
			targetVeh = NULL;
		}
	}
	if ( targetVeh )
	{
		*pTargetVeh = targetVeh;
		return qtrue;
	}

	if ( cg_targVehLastTime && cg.time - cg_targVehLastTime < 3000 )
	{
		targetVeh = &cg_entities[cg_targVeh];

		//stay at full alpha for 1 sec after lose them from crosshair
		if ( cg.time-cg_targVehLastTime < 1000 )
			*alpha = 1.0f;
		else //fade out over 2 secs
			*alpha = 1.0f-(((cg.time-cg_targVehLastTime)+cg.timeFraction-1000)/2000.0f);
	}
	return qfalse;
}

#define MAX_VHUD_SHIELD_TICS 12
#define MAX_VHUD_SPEED_TICS 5
#define MAX_VHUD_ARMOR_TICS 5
#define MAX_VHUD_AMMO_TICS 5

float CG_DrawVehicleShields( const menuDef_t	*menuHUD, const centity_t *veh )
{
	int				i;
	char			itemName[64];
	float			inc, currValue,maxShields;
	vec4_t			calcColor;
	itemDef_t		*item;
	float			percShields;

	item = Menu_FindItemByName((menuDef_t	*) menuHUD, "armorbackground");

	if (item)
	{
		trap_R_SetColor( item->window.foreColor );
		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );
	}

	maxShields = veh->m_pVehicle->m_pVehicleInfo->shields;
	currValue = cg.predictedVehicleState.stats[STAT_ARMOR];
	percShields = (float)currValue/(float)maxShields;
	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxShields / MAX_VHUD_ARMOR_TICS;
	for (i=1;i<=MAX_VHUD_ARMOR_TICS;i++)
	{
		sprintf( itemName, "armor_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *) menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		trap_R_SetColor( calcColor);

		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );

		currValue -= inc;
	}

	return percShields;
}

int cg_vehicleAmmoWarning = 0;
int cg_vehicleAmmoWarningTime = 0;
void CG_DrawVehicleAmmo( const menuDef_t *menuHUD, const centity_t *veh )
{
	int i;
	char itemName[64];
	float inc, currValue,maxAmmo;
	vec4_t	calcColor;
	itemDef_t	*item;

	item = Menu_FindItemByName((menuDef_t *) menuHUD, "ammobackground");

	if (item)
	{
		trap_R_SetColor( item->window.foreColor );
		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );
	}

	maxAmmo = veh->m_pVehicle->m_pVehicleInfo->weapon[0].ammoMax;
	currValue = cg.predictedVehicleState.ammo[0];
	
	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<=MAX_VHUD_AMMO_TICS;i++)
	{
		sprintf( itemName, "ammo_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg_vehicleAmmoWarningTime > cg.time 
			&& cg_vehicleAmmoWarning == 0 )
		{
			memcpy(calcColor, g_color_table[ColorIndex(COLOR_RED)], sizeof(vec4_t));
			calcColor[3] = sin(cg.time*0.005+cg.timeFraction*0.005)*0.5f+0.5f;
		}
		else
		{
			memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));

			if (currValue <= 0)	// don't show tic
			{
				break;
			}
			else if (currValue < inc)	// partial tic (alpha it out)
			{
				float percent = currValue / inc;
				calcColor[3] *= percent;		// Fade it out
			}
		}

		trap_R_SetColor( calcColor);

		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );

		currValue -= inc;
	}
}


void CG_DrawVehicleAmmoUpper( const menuDef_t *menuHUD, const centity_t *veh )
{
	int			i;
	char		itemName[64];
	float		inc, currValue,maxAmmo;
	vec4_t		calcColor;
	itemDef_t	*item;

	item = Menu_FindItemByName((menuDef_t *)menuHUD, "ammoupperbackground");

	if (item)
	{
		trap_R_SetColor( item->window.foreColor );
		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );
	}

	maxAmmo = veh->m_pVehicle->m_pVehicleInfo->weapon[0].ammoMax;
	currValue = cg.predictedVehicleState.ammo[0];

	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<MAX_VHUD_AMMO_TICS;i++)
	{
		sprintf( itemName, "ammoupper_tic%d",	i );
//		Com_sprintf(itemName, sizeof(itemName), "ammoupper_tic%d", i );
		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg_vehicleAmmoWarningTime > cg.time 
			&& cg_vehicleAmmoWarning == 0 )
		{
			memcpy(calcColor, g_color_table[ColorIndex(COLOR_RED)], sizeof(vec4_t));
			calcColor[3] = sin(cg.time*0.005+cg.timeFraction*0.005)*0.5f+0.5f;
		}
		else
		{
			memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));

			if (currValue <= 0)	// don't show tic
			{
				break;
			}
			else if (currValue < inc)	// partial tic (alpha it out)
			{
				float percent = currValue / inc;
				calcColor[3] *= percent;		// Fade it out
			}
		}

		trap_R_SetColor( calcColor);

		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );

		currValue -= inc;
	}
}


void CG_DrawVehicleAmmoLower( const menuDef_t *menuHUD, const centity_t *veh )
{
	int				i;
	char			itemName[64];
	float			inc, currValue,maxAmmo;
	vec4_t			calcColor;
	itemDef_t		*item;


	item = Menu_FindItemByName((menuDef_t *)menuHUD, "ammolowerbackground");

	if (item)
	{
		trap_R_SetColor( item->window.foreColor );
		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );
	}

	maxAmmo = veh->m_pVehicle->m_pVehicleInfo->weapon[1].ammoMax;
	currValue = cg.predictedVehicleState.ammo[1];

	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<MAX_VHUD_AMMO_TICS;i++)
	{
		sprintf( itemName, "ammolower_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg_vehicleAmmoWarningTime > cg.time 
			&& cg_vehicleAmmoWarning == 1 )
		{
			memcpy(calcColor, g_color_table[ColorIndex(COLOR_RED)], sizeof(vec4_t));
			calcColor[3] = sin(cg.time*0.005+cg.timeFraction*0.005)*0.5f+0.5f;
		}
		else
		{
			memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));

			if (currValue <= 0)	// don't show tic
			{
				break;
			}
			else if (currValue < inc)	// partial tic (alpha it out)
			{
				float percent = currValue / inc;
				calcColor[3] *= percent;		// Fade it out
			}
		}

		trap_R_SetColor( calcColor);

		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );

		currValue -= inc;
	}
}

// The HUD.menu file has the graphic print with a negative height, so it will print from the bottom up.
void CG_DrawVehicleTurboRecharge(const menuDef_t *menuHUD, const centity_t *veh)
{
	itemDef_t	*item;

	item = Menu_FindItemByName((menuDef_t *)menuHUD, "turborecharge");

	if (item) {
		float height = item->window.rect.h;
		float percent = 0.0f;
		float diff = (cg.time - veh->m_pVehicle->m_iTurboTime) + cg.timeFraction;

		if (diff > veh->m_pVehicle->m_pVehicleInfo->turboRecharge) {
			percent = 1.0f;
			trap_R_SetColor(colorTable[CT_GREEN]);
		} else {
			percent = diff / veh->m_pVehicle->m_pVehicleInfo->turboRecharge;
			if (percent < 0.0f)
				percent = 0.0f;
			trap_R_SetColor(colorTable[CT_RED]);
		}

		height *= percent;
		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			height, 
			cgs.media.whiteShader);	
	}
}

qboolean cg_drawLink = qfalse;
void CG_DrawVehicleWeaponsLinked( const menuDef_t	*menuHUD, const centity_t *veh )
{
	qboolean drawLink = qfalse;
	if ( veh->m_pVehicle
		&& veh->m_pVehicle->m_pVehicleInfo
		&& (veh->m_pVehicle->m_pVehicleInfo->weapon[0].linkable == 2|| veh->m_pVehicle->m_pVehicleInfo->weapon[1].linkable == 2) )
	{//weapon is always linked
		drawLink = qtrue;
	}
	else
	{
//MP way:
		//must get sent over network
		if ( cg.predictedVehicleState.vehWeaponsLinked )
		{
			drawLink = qtrue;
		}
//NOTE: below is SP way
/*
		//just cheat it
		if ( veh->gent->m_pVehicle->weaponStatus[0].linked
			|| veh->gent->m_pVehicle->weaponStatus[1].linked )
		{
			drawLink = qtrue;
		}
*/
	}

	if ( cg_drawLink != drawLink ) { //state changed, play sound
		cg_drawLink = drawLink;
		trap_S_StartSound (NULL, cg.predictedPlayerState.clientNum, CHAN_LOCAL, trap_S_RegisterSound( "sound/vehicles/common/linkweaps.wav" ) );
	}

	if ( drawLink )
	{
		itemDef_t	*item;

		item = Menu_FindItemByName( (menuDef_t	*) menuHUD, "weaponslinked");

		if (item)
		{
			trap_R_SetColor( colorTable[CT_CYAN] );

				CG_DrawPic( 
				item->window.rect.x, 
				item->window.rect.y, 
				item->window.rect.w, 
				item->window.rect.h, 
				cgs.media.whiteShader);	
		}
	}
}

void CG_DrawVehicleSpeed( const menuDef_t	*menuHUD, const centity_t *veh )
{
	int i;
	char itemName[64];
	float inc, currValue,maxSpeed;
	vec4_t		calcColor;
	itemDef_t	*item;

	item = Menu_FindItemByName((menuDef_t *) menuHUD, "speedbackground");

	if (item)
	{
		trap_R_SetColor( item->window.foreColor );
		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );
	}

	maxSpeed = veh->m_pVehicle->m_pVehicleInfo->speedMax;
	currValue = cg.predictedVehicleState.speed;


	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxSpeed / MAX_VHUD_SPEED_TICS;
	for (i=1;i<=MAX_VHUD_SPEED_TICS;i++)
	{
		sprintf( itemName, "speed_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg.time > veh->m_pVehicle->m_iTurboTime )
		{
			memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));
		}
		else	// In turbo mode
		{
			if (cg.VHUDFlashTime < cg.time)	
			{
				cg.VHUDFlashTime = cg.time + 200;
				if (cg.VHUDTurboFlag)
				{
					cg.VHUDTurboFlag = qfalse;
				}
				else
				{
					cg.VHUDTurboFlag = qtrue;
				}
			}

			if (cg.VHUDTurboFlag)
			{
				memcpy(calcColor, colorTable[CT_LTRED1], sizeof(vec4_t));
			}
			else
			{
				memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));
			}  
		}


		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		trap_R_SetColor( calcColor);

		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );

		currValue -= inc;
	}
}

void CG_DrawVehicleArmor( const menuDef_t *menuHUD, const centity_t *veh )
{
	int			i;
	vec4_t		calcColor;
	char		itemName[64];
	float		inc, currValue,maxArmor;
	itemDef_t	*item;

	maxArmor = veh->m_pVehicle->m_pVehicleInfo->armor;
	currValue = cg.predictedVehicleState.stats[STAT_HEALTH];

	item = Menu_FindItemByName( (menuDef_t	*) menuHUD, "shieldbackground");

	if (item)
	{
		trap_R_SetColor( item->window.foreColor );
		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );
	}


	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxArmor / MAX_VHUD_SHIELD_TICS;
	for (i=1;i <= MAX_VHUD_SHIELD_TICS;i++)
	{
		sprintf( itemName, "shield_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t	*) menuHUD, itemName);

		if (!item)
		{
			continue;
		}


		memcpy(calcColor, item->window.foreColor, sizeof(vec4_t));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor[3] *= percent;		// Fade it out
		}

		trap_R_SetColor( calcColor);

		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );

		currValue -= inc;
	}
}

enum
{
	VEH_DAMAGE_FRONT=0,
	VEH_DAMAGE_BACK,
	VEH_DAMAGE_LEFT,
	VEH_DAMAGE_RIGHT,
};

typedef struct 
{
	char	*itemName;
	short	heavyDamage;
	short	lightDamage;
} veh_damage_t;

    veh_damage_t vehDamageData[4] =
            {
                    { "vehicle_front",SHIPSURF_DAMAGE_FRONT_HEAVY,SHIPSURF_DAMAGE_FRONT_LIGHT },
                    { "vehicle_back",SHIPSURF_DAMAGE_BACK_HEAVY,SHIPSURF_DAMAGE_BACK_LIGHT },
                    { "vehicle_left",SHIPSURF_DAMAGE_LEFT_HEAVY,SHIPSURF_DAMAGE_LEFT_LIGHT },
                    { "vehicle_right",SHIPSURF_DAMAGE_RIGHT_HEAVY,SHIPSURF_DAMAGE_RIGHT_LIGHT },
            };

// Draw health graphic for given part of vehicle
void CG_DrawVehicleDamage(const centity_t *veh,int brokenLimbs,const menuDef_t	*menuHUD,float alpha,int index)
{
	itemDef_t		*item;
	int				colorI;
	vec4_t			color;
	int				graphicHandle=0;

	item = Menu_FindItemByName((menuDef_t *)menuHUD, vehDamageData[index].itemName);
	if (item)
	{
		if (brokenLimbs & (1<<vehDamageData[index].heavyDamage))
		{
			colorI = CT_RED;
			if (brokenLimbs & (1<<vehDamageData[index].lightDamage))
			{
				colorI = CT_DKGREY;
			}
		}
		else if (brokenLimbs & (1<<vehDamageData[index].lightDamage))
		{
			colorI = CT_YELLOW;
		}
		else
		{
			colorI = CT_GREEN;
		}

		VectorCopy4 ( colorTable[colorI], color );
		color[3] = alpha;
		trap_R_SetColor( color );

		switch ( index )
		{
			case VEH_DAMAGE_FRONT :
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconFrontHandle;
				break;
			case VEH_DAMAGE_BACK :
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconBackHandle;
				break;
			case VEH_DAMAGE_LEFT :
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconLeftHandle;
				break;
			case VEH_DAMAGE_RIGHT :
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconRightHandle;
				break;
		}

		if (graphicHandle)
		{
			if (item->window.rect.x <= SCREEN_WIDTH / 2) {
				CG_DrawPic( 
					item->window.rect.x*cgs.widthRatioCoef,
					item->window.rect.y, 
					item->window.rect.w*cgs.widthRatioCoef, 
					item->window.rect.h, 
					graphicHandle );
			} else {
				CG_DrawPic( 
					SCREEN_WIDTH - (SCREEN_WIDTH - item->window.rect.x)*cgs.widthRatioCoef, 
					item->window.rect.y, 
					item->window.rect.w*cgs.widthRatioCoef, 
					item->window.rect.h, 
					graphicHandle );
			}
		}
	}
}


// Used on both damage indicators :  player vehicle and the vehicle the player is locked on 
void CG_DrawVehicleDamageHUD(const centity_t *veh,int brokenLimbs,float percShields,char *menuName, float alpha)
{
	menuDef_t		*menuHUD;
	itemDef_t		*item;
	vec4_t			color;

	menuHUD = Menus_FindByName(menuName);

	if ( !menuHUD )
	{
		return;
	}

	item = Menu_FindItemByName(menuHUD, "background");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicBackgroundHandle)
		{
			if (veh->damageTime > cg.time)
			{//ship shields currently taking damage
				//NOTE: cent->damageAngle can be accessed to get the direction from the ship origin to the impact point (in 3-D space)
				float perc = 1.0f - (((veh->damageTime - cg.time) - cg.timeFraction) / 2000.0f/*MIN_SHIELD_TIME*/);
				
				if (perc < 0.0f)
					perc = 0.0f;
				else if (perc > 1.0f)
					perc = 1.0f;

				color[0] = item->window.foreColor[0];//flash red
				color[1] = item->window.foreColor[1]*perc;//fade other colors back in over time
				color[2] = item->window.foreColor[2]*perc;//fade other colors back in over time
				color[3] = item->window.foreColor[3];//always normal alpha
				trap_R_SetColor(color);
			}
			else
			{
				trap_R_SetColor( item->window.foreColor );
			}
			if (item->window.rect.x <= SCREEN_WIDTH / 2) {
				CG_DrawPic( 
					item->window.rect.x*cgs.widthRatioCoef,
					item->window.rect.y, 
					item->window.rect.w*cgs.widthRatioCoef, 
					item->window.rect.h, 
					veh->m_pVehicle->m_pVehicleInfo->dmgIndicBackgroundHandle );
			} else {
				CG_DrawPic( 
					SCREEN_WIDTH - (SCREEN_WIDTH - item->window.rect.x)*cgs.widthRatioCoef, 
					item->window.rect.y, 
					item->window.rect.w*cgs.widthRatioCoef, 
					item->window.rect.h, 
					veh->m_pVehicle->m_pVehicleInfo->dmgIndicBackgroundHandle );
			}
		}
	}

	item = Menu_FindItemByName(menuHUD, "outer_frame");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicFrameHandle)
		{
			trap_R_SetColor( item->window.foreColor );
			if (item->window.rect.x <= SCREEN_WIDTH / 2) {
				CG_DrawPic( 
					item->window.rect.x*cgs.widthRatioCoef,
					item->window.rect.y, 
					item->window.rect.w*cgs.widthRatioCoef, 
					item->window.rect.h, 
					veh->m_pVehicle->m_pVehicleInfo->dmgIndicFrameHandle );
			} else {
				CG_DrawPic( 
					SCREEN_WIDTH - (SCREEN_WIDTH - item->window.rect.x)*cgs.widthRatioCoef, 
					item->window.rect.y, 
					item->window.rect.w*cgs.widthRatioCoef, 
					item->window.rect.h, 
					veh->m_pVehicle->m_pVehicleInfo->dmgIndicFrameHandle );
			}
		}
	}

	item = Menu_FindItemByName(menuHUD, "shields");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicShieldHandle)
		{
			VectorCopy4 ( colorTable[CT_HUD_GREEN], color );
			color[3] = percShields;
			trap_R_SetColor( color );
			if (item->window.rect.x <= SCREEN_WIDTH / 2) {
				CG_DrawPic( 
					item->window.rect.x*cgs.widthRatioCoef,
					item->window.rect.y, 
					item->window.rect.w*cgs.widthRatioCoef, 
					item->window.rect.h, 
					veh->m_pVehicle->m_pVehicleInfo->dmgIndicShieldHandle );
			} else {
				CG_DrawPic( 
					SCREEN_WIDTH - (SCREEN_WIDTH - item->window.rect.x)*cgs.widthRatioCoef, 
					item->window.rect.y, 
					item->window.rect.w*cgs.widthRatioCoef, 
					item->window.rect.h, 
					veh->m_pVehicle->m_pVehicleInfo->dmgIndicShieldHandle );
			}
		}
	}

	//TODO: if we check nextState.brokenLimbs & prevState.brokenLimbs, we can tell when a damage flag has been added and flash that part of the ship
	//FIXME: when ship explodes, either stop drawing ship or draw all parts black
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_FRONT);
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_BACK);
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_LEFT);
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_RIGHT);
}

qboolean CG_DrawVehicleHud( const centity_t *cent )
{
	itemDef_t		*item;
	menuDef_t		*menuHUD;
	playerState_t	*ps;
	centity_t		*veh;
	float			shieldPerc,alpha;
	
	menuHUD = Menus_FindByName("swoopvehiclehud");
	if (!menuHUD)
	{
		return qtrue;	// Draw player HUD
	}

	ps = &cg.predictedPlayerState;

	if (!ps || !(ps->m_iVehicleNum))
	{
		return qtrue;	// Draw player HUD
	}
	veh = &cg_entities[ps->m_iVehicleNum];

	if ( !veh || !veh->m_pVehicle )
	{
		return qtrue;	// Draw player HUD
	}

	CG_DrawVehicleTurboRecharge( menuHUD, veh );
	CG_DrawVehicleWeaponsLinked( menuHUD, veh );

	item = Menu_FindItemByName(menuHUD, "leftframe");

	// Draw frame
	if (item)
	{
		trap_R_SetColor( item->window.foreColor );
		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );
	}

	item = Menu_FindItemByName(menuHUD, "rightframe");

	if (item)
	{
		trap_R_SetColor( item->window.foreColor );
		CG_DrawPic( 
			item->window.rect.x, 
			item->window.rect.y, 
			item->window.rect.w, 
			item->window.rect.h, 
			item->window.background );
	}


	CG_DrawVehicleArmor( menuHUD, veh );

	// Get animal hud for speed
//	if (veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
//	{
//		menuHUD = Menus_FindByName("tauntaunhud");
//	}
	

	CG_DrawVehicleSpeed( menuHUD, veh );

	// Revert to swoophud
//	if (veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
//	{
//		menuHUD = Menus_FindByName("swoopvehiclehud");
//	}

//	if (veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
//	{
		shieldPerc = CG_DrawVehicleShields( menuHUD, veh );
//	}

	if (veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID && !veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID)
	{
		CG_DrawVehicleAmmo( menuHUD, veh );
	}
	else if (veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID && veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID)
	{
		CG_DrawVehicleAmmoUpper( menuHUD, veh );
		CG_DrawVehicleAmmoLower( menuHUD, veh );
	}

	// If he's hidden, he must be in a vehicle
	if (veh->m_pVehicle->m_pVehicleInfo->hideRider)
	{
		CG_DrawVehicleDamageHUD(veh,cg.predictedVehicleState.brokenLimbs,shieldPerc,"vehicledamagehud",1.0f);

		// Has he targeted an enemy?
		if (CG_CheckTargetVehicle( &veh, &alpha ))
		{
			CG_DrawVehicleDamageHUD(veh,veh->currentState.brokenLimbs,((float)veh->currentState.activeForcePass/10.0f),"enemyvehicledamagehud",alpha);
		}

		return qfalse;	// Don't draw player HUD
	}

	return qtrue;	// Draw player HUD

}

/*
================
CG_DrawStats

================
*/
static void CG_DrawStats( void )  {
	centity_t		*cent;
	playerState_t	*ps;
	qboolean		drawHUD = qtrue;
/*	playerState_t	*ps;
	vec3_t			angles;
//	vec3_t		origin;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}
*/
	cent = &cg_entities[cg.snap->ps.clientNum];
/*	ps = &cg.snap->ps;

	VectorClear( angles );

	// Do start
	if (!cg.interfaceStartupDone)
	{
		CG_InterfaceStartup();
	}

	cgi_UI_MenuPaintAll();*/

	if (cent) {
		ps = &cg.predictedPlayerState;
		if (ps->m_iVehicleNum) { // In a vehicle???
			drawHUD = CG_DrawVehicleHud( cent );
		}
	}
	if (drawHUD) {
		CG_DrawHUD(cent);
	}

	/*CG_DrawArmor(cent);
	CG_DrawHealth(cent);
	CG_DrawAmmo(cent);

	CG_DrawTalk(cent);*/
}

/*
===================
CG_DrawPickupItem
===================
*/
static void CG_DrawPickupItem( void ) {
	int		value;
	float	*fadeColor;
	int		yOffset;

	if ( cg_lagometer.integer ) {
		yOffset = 170;
	} else {
		yOffset = 170 - ICON_SIZE;
	}

	value = cg.itemPickup;
	if ( value && cg_items[ value ].icon != -1 ) 
	{
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) 
		{
			CG_RegisterItemVisuals( value );
			trap_R_SetColor( fadeColor );
			CG_DrawPic( SCREEN_WIDTH - (SCREEN_WIDTH - 585)*cgs.widthRatioCoef, SCREEN_HEIGHT - yOffset - ICON_SIZE, ICON_SIZE*cgs.widthRatioCoef, ICON_SIZE, cg_items[ value ].icon );
			trap_R_SetColor( NULL );
		}
	}
}

static void CG_DrawMovementKeys( void ) {
	usercmd_t cmd = { 0 };
	char str1[32] = { 0 }, str2[32] = { 0 };
	float w1 = 0.0f, w2 = 0.0f, height = 0.0f;
	int fontIndex = FONT_MEDIUM;

	if ( !cg_drawMovementKeys.integer || !cg.snap ) //RAZTODO: works with demo playback??
		return;

	if ( cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback )
		trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd );
	else {
		int moveDir = cg.snap->ps.movementDir;
		float xyspeed = sqrtf( cg.snap->ps.velocity[0]*cg.snap->ps.velocity[0] + cg.snap->ps.velocity[1]*cg.snap->ps.velocity[1] );

		if ( (cg.snap->ps.pm_flags & PMF_JUMP_HELD) )//zspeed > lastZSpeed || zspeed > 10 )
			cmd.upmove = 1;
		else if ( (cg.snap->ps.pm_flags & PMF_DUCKED) )
			cmd.upmove = -1;

		if ( xyspeed < 10 )
			moveDir = -1;

		switch ( moveDir ) {
		case 0: // W
			cmd.forwardmove = 1;
			break;
		case 1: // WA
			cmd.forwardmove = 1;
			cmd.rightmove = -1;
			break;
		case 2: // A
			cmd.rightmove = -1;
			break;
		case 3: // AS
			cmd.rightmove = -1;
			cmd.forwardmove = -1;
			break;
		case 4: // S
			cmd.forwardmove = -1;
			break;
		case 5: // SD
			cmd.forwardmove = -1;
			cmd.rightmove = 1;
			break;
		case 6: // D
			cmd.rightmove = 1;
			break;
		case 7: // DW
			cmd.rightmove = 1;
			cmd.forwardmove = 1;
			break;
		default:
			break;
		}
	}

	w1 = CG_Text_Width( "v W ^", cg_drawMovementKeysScale.value, fontIndex );
	w2 = CG_Text_Width( "A S D", cg_drawMovementKeysScale.value, fontIndex );
	height = CG_Text_Height( "A S D v W ^", cg_drawMovementKeysScale.value, fontIndex );

	Com_sprintf( str1, sizeof(str1), va( "^%cv ^%cW ^%c^", (cmd.upmove < 0) ? COLOR_RED : COLOR_WHITE,
		(cmd.forwardmove > 0) ? COLOR_RED : COLOR_WHITE, (cmd.upmove > 0) ? COLOR_RED : COLOR_WHITE ) );
	Com_sprintf( str2, sizeof(str2), va( "^%cA ^%cS ^%cD", (cmd.rightmove < 0) ? COLOR_RED : COLOR_WHITE,
		(cmd.forwardmove < 0) ? COLOR_RED : COLOR_WHITE, (cmd.rightmove > 0) ? COLOR_RED : COLOR_WHITE ) );

	CG_Text_Paint(cg.moveKeysPos[0] - Q_max(w1, w2 ) / 2.0f, cg.moveKeysPos[1], cg_drawMovementKeysScale.value, colorWhite,
                  str1, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, fontIndex );
	CG_Text_Paint(cg.moveKeysPos[0] - Q_max(w1, w2 ) / 2.0f, cg.moveKeysPos[1] + height, cg_drawMovementKeysScale.value,
                  colorWhite, str2, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, fontIndex );
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = .2f;
		hcolor[2] = .2f;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = .2f;
		hcolor[1] = .2f;
		hcolor[2] = 1;
	} else {
		return;
	}
//	trap_R_SetColor( hcolor );

	CG_FillRect ( x, y, w, h, hcolor );
//	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}


/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawMiniScoreboard
================
*/
static float CG_DrawMiniScoreboard ( float y ) 
{
	char temp[MAX_QPATH];
	float xOffset = 0;

	if ( !cg_drawScores.integer )
	{
		return y;
	}

	if (cgs.gametype == GT_SIEGE)
	{ //don't bother with this in siege
		return y;
	}

	if ( cgs.gametype >= GT_TEAM )
	{
		strcpy ( temp, va("%s: ", CG_GetStringEdString("MP_INGAME", "RED")));
		Q_strcat ( temp, MAX_QPATH, cgs.scores1==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores1)) );
		Q_strcat ( temp, MAX_QPATH, va(" %s: ", CG_GetStringEdString("MP_INGAME", "BLUE")) );
		Q_strcat ( temp, MAX_QPATH, cgs.scores2==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores2)) );

		CG_Text_Paint( SCREEN_WIDTH - (SCREEN_WIDTH - 630 - xOffset)*cgs.widthRatioCoef - CG_Text_Width ( temp, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, temp, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM );
		y += 15;
	}
	else
	{
		/*
		strcpy ( temp, "1st: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores1==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores1)) );
		
		Q_strcat ( temp, MAX_QPATH, " 2nd: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores2==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores2)) );
		
		CG_Text_Paint( 630 - CG_Text_Width ( temp, 0.7f, FONT_SMALL ), y, 0.7f, colorWhite, temp, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM );
		y += 15;
		*/
		//rww - no longer doing this. Since the attacker now shows who is first, we print the score there.
	}		
	

	return y;
}

/*
================
CG_DrawEnemyInfo
================
*/
static float CG_DrawEnemyInfo ( float y ) {
	float		size;
	int			clientNum;
	const char	*title;
	clientInfo_t	*ci;
	float xOffset = 0;

	if (!cg.snap || !cg_drawEnemyInfo.integer || cgs.gametype == GT_POWERDUEL)
		return y;

	if ( cg.playerPredicted && cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) 
		return y;

	if ( cgs.gametype == GT_JEDIMASTER ) {
		//title = "Jedi Master";
		title = CG_GetStringEdString("MP_INGAME", "MASTERY7");
		clientNum = cgs.jediMaster;

		if ( clientNum < 0 ) {
			//return y;
//			title = "Get Saber!";
			title = CG_GetStringEdString("MP_INGAME", "GET_SABER");


			size = ICON_SIZE * 1.25;
			y += 5;

			CG_DrawPic( SCREEN_WIDTH - (size + 12 - xOffset)*cgs.widthRatioCoef, y, size*cgs.widthRatioCoef, size, cgs.media.weaponIcons[WP_SABER] );

			y += size;

			CG_Text_Paint( SCREEN_WIDTH - 10*cgs.widthRatioCoef - CG_Text_Width ( title, 0.7f, FONT_MEDIUM ) + xOffset*cgs.widthRatioCoef, y, 0.7f, colorWhite, title, 0, 0, 0, FONT_MEDIUM );

			return y + BIGCHAR_HEIGHT + 2;
		}
	} else if ( cg.snap->ps.duelInProgress ) {
//		title = "Dueling";
		title = CG_GetStringEdString("MP_INGAME", "DUELING");
		// good fix yeah?
		if (cg.playerPredicted)
			clientNum = cg.snap->ps.duelIndex;
		else
			clientNum = (cg.playerCent->currentState.number == cg.snap->ps.clientNum) ? cg.snap->ps.clientNum : cg.snap->ps.duelIndex;
	}
	else if ( cgs.gametype == GT_DUEL && cgs.clientinfo[cg.playerCent->currentState.number].team != TEAM_SPECTATOR) {
		title = CG_GetStringEdString("MP_INGAME", "DUELING");
		if (cg.playerCent->currentState.number == cgs.duelist1)
			clientNum = cgs.duelist2; //if power duel, should actually draw both duelists 2 and 3 I guess
		else if (cg.playerCent->currentState.number == cgs.duelist2)
			clientNum = cgs.duelist1;
		else if (cg.playerCent->currentState.number == cgs.duelist3)
			clientNum = cgs.duelist1;
		else
			return y;
	} else {
		//As of current, we don't want to draw the attacker. Instead, draw whoever is in first place.
		if (cgs.duelWinner < 0 || cgs.duelWinner >= MAX_CLIENTS)
			return y;

		title = va("%s: %i",CG_GetStringEdString("MP_INGAME", "LEADER"), cgs.scores1);

		clientNum = cgs.duelWinner;
	}

	if ( clientNum >= MAX_CLIENTS || !(&cgs.clientinfo[ clientNum ]) )
		return y;

	ci = &cgs.clientinfo[ clientNum ];

	size = ICON_SIZE * 1.25;
	y += 5;

	if ( ci->modelIcon )
		CG_DrawPic( SCREEN_WIDTH - (size + 5 - xOffset)*cgs.widthRatioCoef, y, size*cgs.widthRatioCoef, size, ci->modelIcon );

	y += size;

	CG_Text_Paint( SCREEN_WIDTH - 10*cgs.widthRatioCoef - CG_Text_Width ( ci->name, 1.0f, FONT_SMALL2 ) + xOffset*cgs.widthRatioCoef, y, 1.0f, colorWhite, ci->name, 0, 0, 0, FONT_SMALL2 );

	y += 15;
	CG_Text_Paint( SCREEN_WIDTH - 10*cgs.widthRatioCoef - CG_Text_Width ( title, 1.0f, FONT_SMALL2 ) + xOffset*cgs.widthRatioCoef, y, 1.0f, colorWhite, title, 0, 0, 0, FONT_SMALL2 );

	if ( (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) && cgs.clientinfo[cg.playerCent->currentState.number].team != TEAM_SPECTATOR) {
		//also print their score
		char text[1024];
		y += 23;//15;
		Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[clientNum].score, cgs.fraglimit );
		CG_Text_Paint( SCREEN_WIDTH - 10*cgs.widthRatioCoef - CG_Text_Width ( text, 0.7f, FONT_MEDIUM ) + xOffset*cgs.widthRatioCoef, y, 0.7f, colorWhite, text, 0, 0, 0, FONT_MEDIUM );
	}

// nmckenzie: DUEL_HEALTH - fixme - need checks and such here.  And this is coded to duelist 1 right now, which is wrongly.
	if ( cgs.showDuelHealths >= 2) {
		y += 15;
		if ( cgs.duelist1 == clientNum )
			CG_DrawDuelistHealth ( SCREEN_WIDTH - (size + 5 - xOffset)*cgs.widthRatioCoef, y, 64*cgs.widthRatioCoef, 8, 1 );
		else if ( cgs.duelist2 == clientNum )
			CG_DrawDuelistHealth ( SCREEN_WIDTH - (size + 5 - xOffset)*cgs.widthRatioCoef, y, 64*cgs.widthRatioCoef, 8, 2 );
	}

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char *s;
	int w;
	float xOffset = 0;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( SCREEN_WIDTH - (5 + w - xOffset)*cgs.widthRatioCoef, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	16
float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static unsigned short previousTimes[FPS_FRAMES];
	static unsigned short index;
	static int	previous, lastupdate;
	int		t, i, fps, total;
	unsigned short frameTime;
	const float		xOffset = 0;


	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;
	if (t - lastupdate > 50)	//don't sample faster than this
	{
		lastupdate = t;
		previousTimes[index % FPS_FRAMES] = frameTime;
		index++;
	}
	// average multiple frames together to smooth changes out a bit
	total = 0;
	for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
		total += previousTimes[i];
	}
	if ( !total ) {
		total = 1;
	}
	fps = 1000 * FPS_FRAMES / total;

	s = va( "%ifps", fps );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( SCREEN_WIDTH - (5 + w - xOffset)*cgs.widthRatioCoef, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

// nmckenzie: DUEL_HEALTH
#define MAX_HEALTH_FOR_IFACE	100
void CG_DrawHealthBarRough (float x, float y, int width, int height, float ratio, const float *color1, const float *color2)
{
	float midpoint, remainder;
	float color3[4] = {1, 0, 0, .7f};

	midpoint = width * ratio - 1;
	remainder = width - midpoint;
	color3[0] = color1[0] * 0.5f;

	assert(!(height%4));//this won't line up otherwise.
	CG_DrawRect(x + 1,			y + height/2-1,  midpoint,	1,	   height/4+1,  color1);	// creme-y filling.
	CG_DrawRect(x + midpoint,	y + height/2-1,  remainder,	1,	   height/4+1,  color3);	// used-up-ness.
	CG_DrawRect(x,				y,				 width,		height, 1,			color2);	// hard crispy shell
}

void CG_DrawDuelistHealth ( float x, float y, float w, float h, int duelist )
{
	float	duelHealthColor[4] = {1, 0, 0, 0.7f};
	float	healthSrc = 0.0f;
	float	ratio;

	if ( duelist == 1 )
	{
		healthSrc = cgs.duelist1health;
	}
	else if (duelist == 2 )
	{
		healthSrc = cgs.duelist2health;
	}

	ratio = healthSrc / MAX_HEALTH_FOR_IFACE;
	if ( ratio > 1.0f )
	{
		ratio = 1.0f;
	}
	if ( ratio < 0.0f )
	{
		ratio = 0.0f;
	}
	duelHealthColor[0] = (ratio * 0.2f) + 0.5f;

	CG_DrawHealthBarRough (x, y, w, h, ratio, duelHealthColor, colorTable[CT_WHITE]);	// new art for this?  I'm not crazy about how this looks.
}

/*
=====================
CG_DrawRadar
=====================
*/
//#define RADAR_RANGE				2500
float	cg_radarRange = 2500.0f;

#define RADAR_RADIUS			60
#define RADAR_X					(SCREEN_WIDTH - (SCREEN_WIDTH - 580)*cgs.widthRatioCoef - RADAR_RADIUS*cgs.widthRatioCoef)
//#define RADAR_Y					10 //dynamic based on passed Y val
#define RADAR_CHAT_DURATION		6000
static int radarLockSoundDebounceTime = 0;
static int impactSoundDebounceTime = 0;
#define	RADAR_MISSILE_RANGE					3000.0f
#define	RADAR_ASTEROID_RANGE				10000.0f
#define	RADAR_MIN_ASTEROID_SURF_WARN_DIST	1200.0f
float CG_DrawRadar (float y) {
	vec4_t			color;
	vec4_t			teamColor;
	float			arrow_w;
	float			arrow_h;
	clientInfo_t	*cl;
	clientInfo_t	*local;
	int				i;
	float			arrowBaseScale;
	float			zScale;
	float			xOffset = 0*cgs.widthRatioCoef;
	float			radar_x;

	// Make sure the radar should be showing
	if (!cg.snap || cg.snap->ps.stats[STAT_HEALTH] <= 0 || !cg.playerPredicted)
		return y;

	if ( (cg.predictedPlayerState.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR )
		return y;

	local = &cgs.clientinfo[ cg.snap->ps.clientNum ];
	if ( !local->infoValid )
		return y;
	
	trap_R_RotatePic2RatioFix(cgs.widthRatioCoef);
	radar_x = SCREEN_WIDTH - (SCREEN_WIDTH - (580 - RADAR_RADIUS)) * cgs.widthRatioCoef;

	// Draw the radar background image
	color[0] = color[1] = color[2] = 1.0f;
	color[3] = 0.6f;
	trap_R_SetColor ( color );
	CG_DrawPic( /*RADAR_X*/radar_x + xOffset, y, RADAR_RADIUS*2*cgs.widthRatioCoef, RADAR_RADIUS*2, cgs.media.radarShader );

	//Always green for your own team.
	VectorCopy ( g_color_table[ColorIndex(COLOR_GREEN)], teamColor );
	teamColor[3] = 1.0f;

	// Draw all of the radar entities.  Draw them backwards so players are drawn last
	for ( i = cg.radarEntityCount -1 ; i >= 0 ; i-- ) 
	{	
		vec3_t		dirLook;	
		vec3_t		dirPlayer;
		float		angleLook;
		float		anglePlayer;
		float		angle;
		float		distance, actualDist;
		centity_t*	cent;

		cent = &cg_entities[cg.radarEntities[i]];

		// Get the distances first
		VectorSubtract ( cg.predictedPlayerState.origin, cent->lerpOrigin, dirPlayer );		
		dirPlayer[2] = 0;
		actualDist = distance = VectorNormalize ( dirPlayer );

		if ( distance > cg_radarRange * 0.8f) 
		{
			if ( (cent->currentState.eFlags & EF_RADAROBJECT)//still want to draw the direction
				|| ( cent->currentState.eType==ET_NPC//FIXME: draw last, with players...
					&& cent->currentState.NPC_class == CLASS_VEHICLE 
					&& cent->currentState.speed > 0 ) )//always draw vehicles
			{ 
				distance = cg_radarRange*0.8f;
			}
			else
			{
				continue;
			}
		}

		distance  = distance / cg_radarRange;
		distance *= RADAR_RADIUS;

		AngleVectors ( cg.predictedPlayerState.viewangles, dirLook, NULL, NULL );

		dirLook[2] = 0;
		anglePlayer = atan2(dirPlayer[0],dirPlayer[1]);		
		VectorNormalize ( dirLook );
		angleLook = atan2(dirLook[0],dirLook[1]);
		angle = angleLook - anglePlayer;

		switch ( cent->currentState.eType )
		{
			default:
				{
					float  x;
					float  ly;
					qhandle_t shader;
					vec4_t    color;

					x = (float)/*RADAR_X*/radar_x + ((float)RADAR_RADIUS + (float)sin (angle) * distance)*cgs.widthRatioCoef;
					ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

					arrowBaseScale = 9.0f;
					shader = 0;
					zScale = 1.0f;

					//we want to scale the thing up/down based on the relative Z (up/down) positioning
					if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
					{ //higher, scale up (between 16 and 24)
						float dif = (cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]);
						
						//max out to 1.5x scale at 512 units above local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale += dif;
					}
					else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
					{ //lower, scale down (between 16 and 8)
						float dif = (cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2]);

						//half scale at 512 units below local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale -= dif;
					}

					arrowBaseScale *= zScale;

					if (cent->currentState.brokenLimbs)
					{ //slightly misleading to use this value, but don't want to add more to entstate.
						//any ent with brokenLimbs non-0 and on radar is an objective ent.
						//brokenLimbs is literal team value.
						char objState[1024];
						int complete;

						//we only want to draw it if the objective for it is not complete.
						//frame represents objective num.
						trap_Cvar_VariableStringBuffer(va("team%i_objective%i", cent->currentState.brokenLimbs, cent->currentState.frame), objState, 1024);

						complete = atoi(objState);

						if (!complete)
						{
						
							// generic enemy index specifies a shader to use for the radar entity.
							if ( cent->currentState.genericenemyindex && cent->currentState.genericenemyindex < MAX_ICONS )
							{
								color[0] = color[1] = color[2] = color[3] = 1.0f;
								shader = cgs.gameIcons[cent->currentState.genericenemyindex];
							}
							else
							{
								if (cg.snap &&
									cent->currentState.brokenLimbs == cg.snap->ps.persistant[PERS_TEAM])
								{
									VectorCopy ( g_color_table[ColorIndex(COLOR_RED)], color );
								}
								else
								{
									VectorCopy ( g_color_table[ColorIndex(COLOR_GREEN)], color );
								}

								shader = cgs.media.siegeItemShader;
							}
						}
					}
					else
					{
						color[0] = color[1] = color[2] = color[3] = 1.0f;
						
						// generic enemy index specifies a shader to use for the radar entity.
						if ( cent->currentState.genericenemyindex )
						{
							shader = cgs.gameIcons[cent->currentState.genericenemyindex];
						}
						else
						{
							shader = cgs.media.siegeItemShader;
						}
					
					}

					if ( shader )
					{
						// Pulse the alpha if time2 is set.  time2 gets set when the entity takes pain
						if ( (cent->currentState.time2 && cg.time - cent->currentState.time2 < 5000) ||
							(cent->currentState.time2 == 0xFFFFFFFF) )
						{								
							if ( (cg.time / 200) & 1 )
							{
								color[3] = 0.1f + 0.9f * (float) (cg.time % 200) / 200.0f;
							}
							else
							{
								color[3] = 1.0f - 0.9f * (float) (cg.time % 200) / 200.0f;
							}					
						}

						trap_R_SetColor ( color );
						CG_DrawPic ( x - 4*cgs.widthRatioCoef + xOffset, ly - 4, arrowBaseScale*cgs.widthRatioCoef, arrowBaseScale, shader );
					}
				}
				break;

			case ET_NPC://FIXME: draw last, with players...
				if ( cent->currentState.NPC_class == CLASS_VEHICLE 
					&& cent->currentState.speed > 0 )
				{
					if ( cent->m_pVehicle && cent->m_pVehicle->m_pVehicleInfo->radarIconHandle )
					{
						float  x;
						float  ly;
			
						x = (float)/*RADAR_X*/radar_x + ((float)RADAR_RADIUS + (float)sin (angle) * distance)*cgs.widthRatioCoef;
						ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

						arrowBaseScale = 9.0f;
						zScale = 1.0f;

						//we want to scale the thing up/down based on the relative Z (up/down) positioning
						if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
						{ //higher, scale up (between 16 and 24)
							float dif = (cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]);
							
							//max out to 1.5x scale at 512 units above local player's height
							dif /= 4096.0f;
							if (dif > 0.5f)
							{
								dif = 0.5f;
							}
							zScale += dif;
						}
						else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
						{ //lower, scale down (between 16 and 8)
							float dif = (cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2]);

							//half scale at 512 units below local player's height
							dif /= 4096.0f;
							if (dif > 0.5f)
							{
								dif = 0.5f;
							}
							zScale -= dif;
						}

						arrowBaseScale *= zScale;

						if ( cent->currentState.m_iVehicleNum //vehicle has a driver
							&& cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].infoValid )
						{
							if ( cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].team == local->team )
							{
								trap_R_SetColor ( teamColor );
							}
							else
							{
								trap_R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
							}
						}
						else
						{
							trap_R_SetColor ( NULL );
						}
						CG_DrawPic ( x - 4*cgs.widthRatioCoef + xOffset, ly - 4, arrowBaseScale*cgs.widthRatioCoef, arrowBaseScale, cent->m_pVehicle->m_pVehicleInfo->radarIconHandle );
					}
				}
				break; //maybe do something?

			case ET_MOVER:
				if ( cent->currentState.speed//the mover's size, actually
					&& actualDist < (cent->currentState.speed+RADAR_ASTEROID_RANGE)
					&& cg.predictedPlayerState.m_iVehicleNum )
				{//a mover that's close to me and I'm in a vehicle
					qboolean mayImpact = qfalse;
					float surfaceDist = (actualDist-cent->currentState.speed);
					if ( surfaceDist < 0.0f )
					{
						surfaceDist = 0.0f;
					}
					if ( surfaceDist < RADAR_MIN_ASTEROID_SURF_WARN_DIST )
					{//always warn!
						mayImpact = qtrue;
					}
					else
					{//not close enough to always warn, yet, so check its direction
						vec3_t	asteroidPos, myPos, moveDir;
						int		predictTime, timeStep = 500;
						float	newDist;
						for ( predictTime = timeStep; predictTime < 5000; predictTime+=timeStep )
						{
							//asteroid dir, speed, size, + my dir & speed...
							BG_EvaluateTrajectory( &cent->currentState.pos, cg.time+predictTime, asteroidPos );
							//FIXME: I don't think it's calcing "myPos" correctly
							AngleVectors( cg.predictedVehicleState.viewangles, moveDir, NULL, NULL );
							VectorMA( cg.predictedVehicleState.origin, cg.predictedVehicleState.speed*predictTime/1000.0f, moveDir, myPos );
							newDist = Distance( myPos, asteroidPos );
							if ( (newDist-cent->currentState.speed) <= RADAR_MIN_ASTEROID_SURF_WARN_DIST )//200.0f )
							{//heading for an impact within the next 5 seconds
								mayImpact = qtrue;
								break;
							}
						}
					}
					if ( mayImpact )
					{//possible collision
						vec4_t	asteroidColor = {0.5f,0.5f,0.5f,1.0f};
						float  x;
						float  ly;
						float asteroidScale = (cent->currentState.speed/2000.0f);//average asteroid radius?
						if ( actualDist > RADAR_ASTEROID_RANGE )
						{
							actualDist = RADAR_ASTEROID_RANGE;
						}
						distance = (actualDist/RADAR_ASTEROID_RANGE)*RADAR_RADIUS;

						x = (float)/*RADAR_X*/radar_x + ((float)RADAR_RADIUS + (float)sin (angle) * distance)*cgs.widthRatioCoef;
						ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;
						
						if ( asteroidScale > 3.0f )
						{
							asteroidScale = 3.0f;
						}
						else if ( asteroidScale < 0.2f )
						{
							asteroidScale = 0.2f;
						}
						arrowBaseScale = (9.0f*asteroidScale);
						if ( impactSoundDebounceTime < cg.time )
						{
							vec3_t	soundOrg;
							if ( surfaceDist > RADAR_ASTEROID_RANGE*0.66f )
							{
								impactSoundDebounceTime = cg.time + 1000;
							}
							else if ( surfaceDist > RADAR_ASTEROID_RANGE/3.0f )
							{
								impactSoundDebounceTime = cg.time + 400;
							}
							else 
							{
								impactSoundDebounceTime = cg.time + 100;
							}
							VectorMA( cg.refdef.vieworg, -500.0f*(surfaceDist/RADAR_ASTEROID_RANGE), dirPlayer, soundOrg );
							trap_S_StartSound( soundOrg, ENTITYNUM_WORLD, CHAN_AUTO, trap_S_RegisterSound( "sound/vehicles/common/impactalarm.wav" ) );
						}
						//brighten it the closer it is
						if ( surfaceDist > RADAR_ASTEROID_RANGE*0.66f )
						{
							asteroidColor[0] = asteroidColor[1] = asteroidColor[2] = 0.7f;
						}
						else if ( surfaceDist > RADAR_ASTEROID_RANGE/3.0f )
						{
							asteroidColor[0] = asteroidColor[1] = asteroidColor[2] = 0.85f;
						}
						else 
						{
							asteroidColor[0] = asteroidColor[1] = asteroidColor[2] = 1.0f;
						}
						//alpha out the longer it's been since it was considered dangerous
						if ( (cg.time-impactSoundDebounceTime) > 100 )
						{
							asteroidColor[3] = (float)((cg.time-impactSoundDebounceTime)+cg.timeFraction-100.0f)/900.0f;
						}

						trap_R_SetColor ( asteroidColor );
						CG_DrawPic ( x - 4*cgs.widthRatioCoef + xOffset, ly - 4, arrowBaseScale*cgs.widthRatioCoef, arrowBaseScale, trap_R_RegisterShaderNoMip( "gfx/menus/radar/asteroid" ) );
					}
				}
				break;

			case ET_MISSILE:
				if ( //cent->currentState.weapon == WP_ROCKET_LAUNCHER &&//a rocket
					cent->currentState.owner > MAX_CLIENTS //belongs to an NPC
					&& cg_entities[cent->currentState.owner].currentState.NPC_class == CLASS_VEHICLE )
				{//a rocket belonging to an NPC, FIXME: only tracking rockets!
					float  x;
					float  ly;
		
					x = (float)/*RADAR_X*/radar_x + ((float)RADAR_RADIUS + (float)sin (angle) * distance)*cgs.widthRatioCoef;
					ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

					arrowBaseScale = 3.0f;
					if ( cg.predictedPlayerState.m_iVehicleNum )
					{//I'm in a vehicle
						//if it's targetting me, then play an alarm sound if I'm in a vehicle
						if ( cent->currentState.otherEntityNum == cg.predictedPlayerState.clientNum || cent->currentState.otherEntityNum == cg.predictedPlayerState.m_iVehicleNum )
						{
							if ( radarLockSoundDebounceTime < cg.time )
							{
								vec3_t	soundOrg;
								int		alarmSound;
								if ( actualDist > RADAR_MISSILE_RANGE * 0.66f )
								{
									radarLockSoundDebounceTime = cg.time + 1000;
									arrowBaseScale = 3.0f;
									alarmSound = trap_S_RegisterSound( "sound/vehicles/common/lockalarm1.wav" );
								}
								else if ( actualDist > RADAR_MISSILE_RANGE/3.0f )
								{
									radarLockSoundDebounceTime = cg.time + 500;
									arrowBaseScale = 6.0f;
									alarmSound = trap_S_RegisterSound( "sound/vehicles/common/lockalarm2.wav" );
								}
								else
								{
									radarLockSoundDebounceTime = cg.time + 250;
									arrowBaseScale = 9.0f;
									alarmSound = trap_S_RegisterSound( "sound/vehicles/common/lockalarm3.wav" );
								}
								if ( actualDist > RADAR_MISSILE_RANGE )
								{
									actualDist = RADAR_MISSILE_RANGE;
								}
								VectorMA( cg.refdef.vieworg, -500.0f*(actualDist/RADAR_MISSILE_RANGE), dirPlayer, soundOrg );
								trap_S_StartSound( soundOrg, ENTITYNUM_WORLD, CHAN_AUTO, alarmSound );
							}
						}
					}
					zScale = 1.0f;

					//we want to scale the thing up/down based on the relative Z (up/down) positioning
					if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
					{ //higher, scale up (between 16 and 24)
						float dif = (cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]);
						
						//max out to 1.5x scale at 512 units above local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale += dif;
					}
					else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
					{ //lower, scale down (between 16 and 8)
						float dif = (cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2]);

						//half scale at 512 units below local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale -= dif;
					}

					arrowBaseScale *= zScale;

					if ( cent->currentState.owner >= MAX_CLIENTS//missile owned by an NPC
						&& cg_entities[cent->currentState.owner].currentState.NPC_class == CLASS_VEHICLE//NPC is a vehicle
						&& cg_entities[cent->currentState.owner].currentState.m_iVehicleNum <= MAX_CLIENTS//Vehicle has a player driver
						&& cgs.clientinfo[cg_entities[cent->currentState.owner].currentState.m_iVehicleNum-1].infoValid ) //player driver is valid
					{
						cl = &cgs.clientinfo[cg_entities[cent->currentState.owner].currentState.m_iVehicleNum-1];
						if ( cl->team == local->team )
						{
							trap_R_SetColor ( teamColor );
						}
						else
						{
							trap_R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
						}
					}
					else
					{
						trap_R_SetColor ( NULL );
					}
					CG_DrawPic ( x - 4*cgs.widthRatioCoef + xOffset, ly - 4, arrowBaseScale*cgs.widthRatioCoef, arrowBaseScale, cgs.media.mAutomapRocketIcon );
				}
				break;

			case ET_PLAYER:
			{
				vec4_t color;

				cl = &cgs.clientinfo[ cent->currentState.number ];

				// not valid then dont draw it
				if ( !cl->infoValid ) 
				{	
					continue;
				}

				VectorCopy4 ( teamColor, color );

				arrowBaseScale = 16.0f;
				zScale = 1.0f;

				// Pulse the radar icon after a voice message
				if ( cent->vChatTime + 2000 > cg.time )
				{
					float f = ((cent->vChatTime + 2000 - cg.time) - cg.timeFraction) / 3000.0f;
					arrowBaseScale = 16.0f + 4.0f * f;
					color[0] = teamColor[0] + (1.0f - teamColor[0]) * f;
					color[1] = teamColor[1] + (1.0f - teamColor[1]) * f;
					color[2] = teamColor[2] + (1.0f - teamColor[2]) * f;
				}

				trap_R_SetColor ( color );

				//we want to scale the thing up/down based on the relative Z (up/down) positioning
				if (cent->lerpOrigin[2] > cg.predictedPlayerState.origin[2])
				{ //higher, scale up (between 16 and 32)
					float dif = (cent->lerpOrigin[2] - cg.predictedPlayerState.origin[2]);
					
					//max out to 2x scale at 1024 units above local player's height
					dif /= 1024.0f;
					if (dif > 1.0f)
					{
						dif = 1.0f;
					}
					zScale += dif;
				}
                else if (cent->lerpOrigin[2] < cg.predictedPlayerState.origin[2])
				{ //lower, scale down (between 16 and 8)
					float dif = (cg.predictedPlayerState.origin[2] - cent->lerpOrigin[2]);

					//half scale at 512 units below local player's height
					dif /= 1024.0f;
					if (dif > 0.5f)
					{
						dif = 0.5f;
					}
					zScale -= dif;
				}

				arrowBaseScale *= zScale;

				arrow_w = arrowBaseScale * RADAR_RADIUS / 128;
				arrow_h = arrowBaseScale * RADAR_RADIUS / 128;

				CG_DrawRotatePic2( /*RADAR_X*/radar_x + (RADAR_RADIUS + sin (angle) * distance + xOffset)*cgs.widthRatioCoef,
								   y + RADAR_RADIUS + cos (angle) * distance, 
								   arrow_w, arrow_h, 
								   (360 - cent->lerpAngles[YAW]) + cg.predictedPlayerState.viewangles[YAW], cgs.media.mAutomapPlayerIcon );
				break;
			}
		}
	}

	arrowBaseScale = 16.0f;

	arrow_w = arrowBaseScale * RADAR_RADIUS / 128;
	arrow_h = arrowBaseScale * RADAR_RADIUS / 128;

	trap_R_SetColor ( colorWhite );
	CG_DrawRotatePic2( /*RADAR_X*/radar_x + (RADAR_RADIUS + xOffset)*cgs.widthRatioCoef, y + RADAR_RADIUS, arrow_w, arrow_h, 
					   0, cgs.media.mAutomapPlayerIcon );
	trap_R_RotatePic2RatioFix(1.0f);

	return y+(RADAR_RADIUS*2);
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;
	float		xOffset = 0;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( SCREEN_WIDTH - (5 + w - xOffset)*cgs.widthRatioCoef, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawSpeed
=================
*/
static float CG_DrawSpeed( float y ) {
	char		*s;
	int			w;
	int			xOffset = 0;
	static vec_t minJumpHeight = 0.0f, maxJumpHeight = 0.0f;
	static int	lastZVel = 0;
	vec3_t		velocity;

	if (cg.playerPredicted)
		VectorCopy(cg.snap->ps.velocity, velocity);
	else
		VectorCopy(cg.playerCent->currentState.pos.trDelta, velocity);

	if ((int)velocity[2] > 0) {
		if (lastZVel == 0)
			minJumpHeight = cg.playerCent->lerpOrigin[2];
		maxJumpHeight = cg.playerCent->lerpOrigin[2];
	}

	lastZVel = (int) velocity[2];

	s = va("%i XYvel", (int)sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1]));
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w + xOffset, y + 2, s, 1.0F);

	y += BIGCHAR_HEIGHT + 4;

	if (cg_drawSpeed.integer > 1) {
		s = va("%i Zvel", (int)velocity[2]);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

		CG_DrawBigString(635 - w + xOffset, y + 2, s, 1.0F);

		y += BIGCHAR_HEIGHT + 4;
	}

	if (cg_drawSpeed.integer > 2) {
		s = va("%i jump", (int) (maxJumpHeight - minJumpHeight));
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

		CG_DrawBigString(635 - w + xOffset, y + 2, s, 1.0F);

		y += BIGCHAR_HEIGHT + 4;
	}

	if (cg_drawSpeed.integer > 3) {
		s = va("%i base speed", cg.snap->ps.basespeed);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

		CG_DrawBigString(635 - w + xOffset, y + 2, s, 1.0F);

		y += BIGCHAR_HEIGHT + 4;

		s = va("%i gravity", cg.snap->ps.gravity);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

		CG_DrawBigString(635 - w + xOffset, y + 2, s, 1.0F);

		y += BIGCHAR_HEIGHT + 4;
	}

	return y;
}

/*
=================
CG_DrawTeamOverlay
=================
*/
extern const char *CG_GetLocationString(const char *loc); //cg_main.c
static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	float x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;
	float xOffset = 0*cgs.widthRatioCoef;

	y += CG_MultiSpecTeamOverlayOffset();

	if ( CG_MultiSpecTeamOverlayReplace() ) {
		return y;
	}

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}

	if ( cgs.clientinfo[cg.playerCent->currentState.number].team != cg.snap->ps.persistant[PERS_TEAM] ) {
		return y;
	}

	plyrs = 0;

	//TODO: On basejka servers, we won't have valid teaminfo if we're spectating someone.
	//		Find a way to detect invalid info and return early?

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_GetLocationString(CG_ConfigString(CS_LOCATIONS+i));
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH*cgs.widthRatioCoef;

	if ( right )
		x = SCREEN_WIDTH - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x + xOffset, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH*cgs.widthRatioCoef;

			CG_DrawStringExt( xx + xOffset, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH*cgs.widthRatioCoef, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_GetLocationString(CG_ConfigString(CS_LOCATIONS+ci->location));
				if (!p || !*p)
					p = "unknown";
				len = CG_DrawStrlen(p);
				if (len > lwidth)
					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth + 
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2*cgs.widthRatioCoef + TINYCHAR_WIDTH * pwidth*cgs.widthRatioCoef;
				CG_DrawStringExt( xx + xOffset, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH*cgs.widthRatioCoef, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3*cgs.widthRatioCoef + 
				TINYCHAR_WIDTH * pwidth*cgs.widthRatioCoef + TINYCHAR_WIDTH * lwidth*cgs.widthRatioCoef;

			CG_DrawStringExt( xx + xOffset, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH*cgs.widthRatioCoef, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3*cgs.widthRatioCoef;

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx + xOffset, y, TINYCHAR_WIDTH*cgs.widthRatioCoef, TINYCHAR_HEIGHT, 
					cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx + xOffset, y, TINYCHAR_WIDTH*cgs.widthRatioCoef, TINYCHAR_HEIGHT, 
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH*cgs.widthRatioCoef;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx + xOffset, y, TINYCHAR_WIDTH*cgs.widthRatioCoef, TINYCHAR_HEIGHT, 
						trap_R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH*cgs.widthRatioCoef;
						} else {
							xx += TINYCHAR_WIDTH*cgs.widthRatioCoef;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}


static void CG_DrawPowerupIcons(int y)
{
	int j;
	int ico_size = 64;
	//int y = ico_size/2;
	float xOffset = 0*cgs.widthRatioCoef;
	gitem_t	*item;

	//Raz: was missing this
	trap_R_SetColor( NULL );

	if (cg.playerPredicted && !cg.snap)
		return;

	if ( !cg.playerCent )
		return;

	y += 16;

	//Raz: fixed potential buffer overrun of cg.snap->ps.powerups
	for (j = 0; j < PW_NUM_POWERUPS; j++)
	{
		if ((!cg.playerPredicted?cg.playerCent->currentState.powerups & (1 << j):cg.snap->ps.powerups[j] > cg.time))
		{
			int secondsleft = (cg.snap->ps.powerups[j] - cg.time)/1000;

			if (!cg.playerPredicted)
				secondsleft = 1000; //we don't draw timer

			item = BG_FindItemForPowerup( j );

			if (item)
			{
				int icoShader = 0;
				if (cgs.gametype == GT_CTY && (j == PW_REDFLAG || j == PW_BLUEFLAG))
				{
					if (j == PW_REDFLAG)
					{
						icoShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_ys" );
					}
					else
					{
						icoShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_ys" );
					}
				}
				else
				{
					icoShader = trap_R_RegisterShader( item->icon );
				}

				CG_DrawPic( (SCREEN_WIDTH-(ico_size*1.1)*cgs.widthRatioCoef) + xOffset, y, ico_size*cgs.widthRatioCoef, ico_size, icoShader );
	
				y += ico_size;

				if (j != PW_REDFLAG && j != PW_BLUEFLAG && secondsleft < 999)
				{
					UI_DrawProportionalString((SCREEN_WIDTH-(ico_size*1.1)*cgs.widthRatioCoef)+(ico_size/2*cgs.widthRatioCoef) + xOffset, y-8, va("%i", secondsleft), UI_CENTER | UI_BIGFONT | UI_DROPSHADOW, colorTable[CT_WHITE]);
				}

				y += (ico_size/3);
			}
		}
	}
}


/*
=====================
CG_DrawUpperRight

=====================
*/
void CG_DrawUpperRight( void ) {
	float	y = 0;

	trap_R_SetColor( colorTable[CT_WHITE] );

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	} 
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}

	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}

	if ( cg_drawSpeed.integer ) {
		y = CG_DrawSpeed( y );
	}

	if ( ( cgs.gametype >= GT_TEAM || cg.predictedPlayerState.m_iVehicleNum )
		&& cg_drawRadar.integer )
	{//draw Radar in Siege mode or when in a vehicle of any kind
		y = CG_DrawRadar ( y );
	}

	y = CG_DrawEnemyInfo ( y );

	y = CG_DrawMiniScoreboard ( y );

	CG_DrawPowerupIcons(y);
}

/*
===================
CG_DrawReward
===================
*/
#ifdef JK2AWARDS
static void CG_DrawReward( void ) { 
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			if (!(mov_soundDisable.integer & SDISABLE_REWARD)) {
				trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
			}
		} else {
			return;
		}
	}

	trap_R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 56;
		x = 320 - (ICON_SIZE/2)*cgs.widthRatioCoef;
		CG_DrawPic( x, y, (ICON_SIZE-4)*cgs.widthRatioCoef, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen( buf )*cgs.widthRatioCoef ) / 2;
		CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH*cgs.widthRatioCoef, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = 320 - count * (ICON_SIZE/2)*cgs.widthRatioCoef;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE-4)*cgs.widthRatioCoef, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE*cgs.widthRatioCoef;
		}
	}
	trap_R_SetColor( NULL );
}
#endif


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	if (cg.demoPlayback) {
		//from pugmod, maybe useful
//		static int lasttime = 0;
		// rain - #67 - display snapshot time delta instead of ping when
		// playing a demo, since I can't think of any way to get the
		// real ping
//		lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->serverTime - lasttime;
//		lasttime = snap->serverTime;

		//enemy terriroty version
		snap->ping = (snap->serverTime - snap->ps.commandTime) - cg.frametime;
	}// else
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;  // bk010215 - FIXME char message[1024];

	if (cg.demoPlayback == 2)
		return;	//we don't need that msg in demos
	if (cg.mMapChange) {			
		s = CG_GetStringEdString("MP_INGAME", "SERVER_CHANGING_MAPS");	// s = "Server Changing Maps";			
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH*cgs.widthRatioCoef;
		CG_DrawBigString( SCREEN_WIDTH / 2 - w/2, 100, s, 1.0F);

		s = CG_GetStringEdString("MP_INGAME", "PLEASE_WAIT");	// s = "Please wait...";
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH*cgs.widthRatioCoef;
		CG_DrawBigString( SCREEN_WIDTH / 2 - w/2, 200, s, 1.0F);
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart // bk 0102165 - FIXME
		return;
	}

	// also add text in center of screen
	s = CG_GetStringEdString("MP_INGAME", "CONNECTION_INTERRUPTED"); // s = "Connection Interrupted"; // bk 010215 - FIXME
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH*cgs.widthRatioCoef;
	CG_DrawBigString( SCREEN_WIDTH / 2 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = SCREEN_WIDTH - 48*cgs.widthRatioCoef;
	y = SCREEN_HEIGHT - 48;

	CG_DrawPic( x, y, 48*cgs.widthRatioCoef, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

#define LAGOMETER_PING_DELAY 700

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, i;
	float	x, y;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;
	static int lastPingTime = 0;
	static int lastPing = 0;

	if ( !cg_lagometer.integer || cgs.localServer || !cg.playerPredicted) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	x = (float)SCREEN_WIDTH - 48.0f*cgs.widthRatioCoef;
	y = (float)SCREEN_HEIGHT - 165.0f;

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48*cgs.widthRatioCoef, 48, cgs.media.lagometerShader );

	ax = x-0.5f*cgs.widthRatioCoef;
	ay = y;
	aw = 48.0f*2.0f;
	ah = 48.0f;

	color = -1;
	range = ah / 3.0f;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + (0.5*aw - a*0.5)*cgs.widthRatioCoef, mid - v, 0.5*cgs.widthRatioCoef, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + (0.5*aw - a*0.5)*cgs.widthRatioCoef, mid, 0.5*cgs.widthRatioCoef, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2.0f;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + (0.5*aw - 0.5*a)*cgs.widthRatioCoef, ay + ah - v, 0.5*cgs.widthRatioCoef, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap_R_DrawStretchPic( ax + (0.5*aw - 0.5*a)*cgs.widthRatioCoef, ay + ah - range, 0.5*cgs.widthRatioCoef, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	//draw ping, WORKS WRONG
	if (cg.time > lastPingTime) {
		lastPing = lagometer.snapshotSamples[ (lagometer.snapshotCount - 1) & ( LAG_SAMPLES - 1) ];
		if (lastPing < 0)
			lastPing = 0;
		lastPingTime = cg.time + LAGOMETER_PING_DELAY;
	} else if (cg.time + LAGOMETER_PING_DELAY < lastPingTime) {
		lastPingTime = 0;
	}
	CG_Text_Paint(ax + 2, ay + 2, 0.5f, colorWhite, va("%i",lastPing), 0, 0, 3, FONT_SMALL );

	trap_R_SetColor( NULL );

	if ( !cg.demoPlayback && (cg_noPredict.integer || g_synchronousClients.integer) ) {
		CG_DrawBigString( ax, ay, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}

void CG_DrawSiegeMessage( const char *str, int objectiveScreen )
{
//	if (!( trap_Key_GetCatcher() & KEYCATCH_UI ))
	{
		trap_OpenUIMenu(UIMENU_CLOSEALL);
		trap_Cvar_Set("cg_siegeMessage", str);
		if (objectiveScreen)
		{
			trap_OpenUIMenu(UIMENU_SIEGEOBJECTIVES);
		}
		else
		{
			trap_OpenUIMenu(UIMENU_SIEGEMESSAGE);
		}
	}
}

/*static void CG_DrawSpeedGraph( void ) {
    int		a, i;
    float	x, y, v;
    float	ax, ay, aw, ah, mid, range;
    int		color;
    float	vscale;

    x = SCREEN_WIDTH - cg_lagometerX.integer * cgs.widthRatioCoef;
    y = SCREEN_HEIGHT - cg_lagometerY.integer - 56;

    if (cg_hudFiles.integer == 0) {
        y -= 16;
    }


    trap->R_SetColor(NULL);

    if (cg_lagometer.integer == 1 || cg_lagometer.integer == 2)
        CG_DrawPic(x, y, 48 * cgs.widthRatioCoef, 48, cgs.media.lagometerShader);

    if (cg_lagometer.integer == 2 || (cg_lagometer.integer == 3 && cg.currentSpeed != 0))
        CG_Text_Paint(x + 2 * cgs.widthRatioCoef, y, 0.5f, colorWhite, va("%.0f", cg.currentSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_SMALL);

    if (cg_lagometer.integer != 1 && cg_lagometer.integer != 2)
        y -= 96;

    x -= 1.0f * cgs.widthRatioCoef;

    ax = x;
    ay = y;
    aw = 48;
    if (cg_lagometer.integer != 1 && cg_lagometer.integer != 2)
        ah = 48*3;
    else
        ah = 48;

    color = -1;
    range = ah / 3;
    mid = ay + range;

    // draw the speed graph
    range = ah;
    vscale = range / (3000);

    for (a = 0 ; a < aw ; a++) {
        i = ( speedgraph.frameCount - 1 - a ) & (SPEED_SAMPLES - 1);
        v = speedgraph.frameSamples[i];
        if (v > 0) {
            trap_R_SetColor(g_color_table[ColorIndex(COLOR_GREEN)]);
            v = v * vscale;
            if (v > range)
                v = range;
            trap_R_DrawStretchPic(ax + (aw - a) * cgs.widthRatioCoef, ay + ah - v, 1 * cgs.widthRatioCoef, v, 0, 0, 0, 0, cgs.media.whiteShader);
        }
    }
    trap_R_SetColor(NULL);
}*/

void CG_DrawSiegeMessageNonMenu( const char *str )
{
	char	text[1024];
	if (str[0]=='@')
	{	
		trap_SP_GetStringTextString(str+1, text, sizeof(text));
		str = text;
	}
	CG_CenterPrint(str, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;
	//[BugFix19]
	int		i = 0;
	//[/BugFix19]

	if( mov_fragsOnly.integer ) {
		char sKilledStr[256];
		trap_SP_GetStringTextString("MP_INGAME_KILLED_MESSAGE", sKilledStr, sizeof(sKilledStr));
		if(strncmp(str,sKilledStr,strlen(sKilledStr))!=0)return;
	}	

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	Q_StripColorNew(s, CG_SwitchColorTable());
	while( *s ) 
	{
		//[BugFix19]
		i++;
		if(i >= 50)
		{//maxed out a line of text, this will make the line spill over onto another line.
			i = 0;
			cg.centerPrintLines++;
		}
		else if (*s == '\n')
		//if (*s == '\n')
		//[/BugFix19]
			cg.centerPrintLines++;
		s++;
	}
	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );
}


/*
===================
CG_DrawCenterString
===================
*/
qboolean BG_IsWhiteSpace( char c )
{//this function simply checks to see if the given character is whitespace.
	if ( c == ' ' || c == '\n' || c == '\0' )
		return qtrue;

	return qfalse;
}
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l, len;
	int		x, y, w;
	int h;
	float	*color;
	const float scale = 1.0; //0.5

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centerTime.value );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	len = strlen(cg.centerPrint);
	len -= Q_PrintStrlen(cg.centerPrint, CG_SwitchColorTable());
	if (len < 0) //must never happen
		len = 0;

	start = cg.centerPrint;

	if( mov_fragsOnly.integer != 0 ) {
		char sKilledStr[256];
		trap_SP_GetStringTextString("MP_INGAME_KILLED_MESSAGE", sKilledStr, sizeof(sKilledStr));
		if(strncmp(cg.centerPrint,sKilledStr,strlen(sKilledStr))!=0)return;
	}

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		if (cg.centerPrintLines > 1) {
			char linebuffer[1024];

			for ( l = 0; l < 50+len; l++ ) {
				if ( !start[l] || start[l] == '\n' ) {
					break;
				}
				linebuffer[l] = start[l];
			}
			linebuffer[l] = 0;

			//[BugFix19]
			if(!BG_IsWhiteSpace(start[l]) && ((l > 0 && !BG_IsWhiteSpace(linebuffer[l-1])) || l == 0) )
			{//we might have cut a word off, attempt to find a spot where we won't cut words off at.
				int savedL = l;
				int counter = l-2;

				for(; counter >= 0; counter--)
				{
					if(BG_IsWhiteSpace(start[counter]))
					{//this location is whitespace, line break from this position
						linebuffer[counter] = 0;
						l = counter + 1;
						break;
					}
				}
				if(counter < 0)
				{//couldn't find a break in the text, just go ahead and cut off the word mid-word.
					l = savedL;
				}
			}
			//[/BugFix19]

			w = CG_Text_Width(linebuffer, scale, FONT_MEDIUM);
			h = CG_Text_Height(linebuffer, scale, FONT_MEDIUM);
			x = (SCREEN_WIDTH - w) / 2;
			CG_Text_Paint(x, y + h, scale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
			y += h + 6;

			//[BugFix19]
			//this method of advancing to new line from the start of the array was causing long lines without
			//new lines to be totally truncated.
			if(start[l] && start[l] == '\n')
			{//next char is a newline, advance past
				l++;
			}

			len = 0; //only for the first string

			if ( !start[l] )
			{//end of string, we're done.
				break;
			}

			//advance pointer to the last character that we didn't read in.
			start = &start[l];
			//[/BugFix19]
		} else {
			char linebuffer[1024];

			for ( l = 0; l < 50+len; l++ ) {
				if ( !start[l] || start[l] == '\n' ) {
					break;
				}
				linebuffer[l] = start[l];
			}
			linebuffer[l] = 0;

			w = CG_Text_Width(linebuffer, scale, FONT_MEDIUM);
			h = CG_Text_Height(linebuffer, scale, FONT_MEDIUM);
			x = (SCREEN_WIDTH - w) / 2;
			CG_Text_Paint(x, y + h, scale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
			y += h + 6;

			while ( *start && ( *start != '\n' ) ) {
				start++;
			}
			if ( !*start ) {
				break;
			}
			start++;
		}
	}
	
	trap_R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/

#define HEALTH_WIDTH		50.0f
#define HEALTH_HEIGHT		5.0f

//see if we can draw some extra info on this guy based on our class
void CG_DrawSiegeInfo(centity_t *cent, float chX, float chY, float chW, float chH)
{
	siegeExtended_t *se = &cg_siegeExtendedData[cent->currentState.number];
	clientInfo_t *ci;
	const char *configstring, *v;
	siegeClass_t *siegeClass;
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x;
	float y;
	float percent;
	int ammoMax;

	assert(cent->currentState.number < MAX_CLIENTS);

	if (se->lastUpdated > cg.time)
	{ //strange, shouldn't happen
		return;
	}
	
	if ((cg.time - se->lastUpdated) > 10000)
	{ //if you haven't received a status update on this guy in 10 seconds, forget about it
		return;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{ //he's dead, don't display info on him
		return;
	}

	if (cent->currentState.weapon != se->weapon)
	{ //data is invalidated until it syncs back again
		return;
	}

	ci = &cgs.clientinfo[cent->currentState.number];
	if (ci->team != cg.predictedPlayerState.persistant[PERS_TEAM])
	{ //not on the same team
		return;
	}

	configstring = CG_ConfigString( cg.predictedPlayerState.clientNum + CS_PLAYERS );
	v = Info_ValueForKey( configstring, "siegeclass" );

	if (!v || !v[0])
	{ //don't have siege class in info?
		return;
	}

	siegeClass = BG_SiegeFindClassByName(v);

	if (!siegeClass)
	{ //invalid
		return;
	}

    if (!(siegeClass->classflags & (1<<CFL_STATVIEWER)))
	{ //doesn't really have the ability to see others' stats
		return;
	}

	x = chX+((chW/2)-(HEALTH_WIDTH/2));
	y = (chY+chH) + 8.0f;
	percent = ((float)se->health/(float)se->maxhealth)*HEALTH_WIDTH;

	//color of the bar
	aColor[0] = 0.0f;
	aColor[1] = 1.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.4f;

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing health"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, percent-1.0f, HEALTH_HEIGHT-1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, HEALTH_WIDTH-percent-1.0f, HEALTH_HEIGHT-1.0f, cColor);


	//now draw his ammo
	ammoMax = ammoData[weaponData[cent->currentState.weapon].ammoIndex].max;
	if ( (cent->currentState.eFlags & EF_DOUBLE_AMMO) )
	{
		ammoMax *= 2;
	}

	x = chX+((chW/2)-(HEALTH_WIDTH/2));
	y = (chY+chH) + HEALTH_HEIGHT + 10.0f;

	if (!weaponData[cent->currentState.weapon].energyPerShot &&
		!weaponData[cent->currentState.weapon].altEnergyPerShot)
	{ //a weapon that takes no ammo, so show full
		percent = HEALTH_WIDTH;
	}
	else
	{
		percent = ((float)se->ammo/(float)ammoMax)*HEALTH_WIDTH;
	}

	//color of the bar
	aColor[0] = 1.0f;
	aColor[1] = 1.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.4f;

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing health"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, percent-1.0f, HEALTH_HEIGHT-1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, HEALTH_WIDTH-percent-1.0f, HEALTH_HEIGHT-1.0f, cColor);
}

//draw the health bar based on current "health" and maxhealth
void CG_DrawHealthBar(centity_t *cent, float chX, float chY, float chW, float chH)
{
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x = chX+((chW/2)-(HEALTH_WIDTH/2))*cgs.widthRatioCoef;
	float y = (chY+chH) + 8.0f;
	float percent = ((float)cent->currentState.health/(float)cent->currentState.maxhealth)*HEALTH_WIDTH*cgs.widthRatioCoef;
	if (percent <= 0)
	{
		return;
	}

	//color of the bar
	if (!cent->currentState.teamowner || cgs.gametype < GT_TEAM)
	{ //not owned by a team or teamplay
		aColor[0] = 1.0f;
		aColor[1] = 1.0f;
		aColor[2] = 0.0f;
		aColor[3] = 0.4f;
	}
	else if (cent->currentState.teamowner == cg.predictedPlayerState.persistant[PERS_TEAM])
	{ //owned by my team
		aColor[0] = 0.0f;
		aColor[1] = 1.0f;
		aColor[2] = 0.0f;
		aColor[3] = 0.4f;
	}
	else
	{ //hostile
		aColor[0] = 1.0f;
		aColor[1] = 0.0f;
		aColor[2] = 0.0f;
		aColor[3] = 0.4f;
	}

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing health"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH*cgs.widthRatioCoef, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f*cgs.widthRatioCoef, y+1.0f, percent-1.0f*cgs.widthRatioCoef, HEALTH_HEIGHT-1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, (HEALTH_WIDTH-1.0f)*cgs.widthRatioCoef-percent, HEALTH_HEIGHT-1.0f, cColor);
}

//same routine (at least for now), draw progress of a "hack" or whatever
void CG_DrawHaqrBar(float chX, float chY, float chW, float chH) {
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x = chX+((chW/2.0f)-(HEALTH_WIDTH/2.0f))*cgs.widthRatioCoef;
	float y = (chY+chH) + 8.0f;
	float percent = (((cg.predictedPlayerState.hackingTime-cg.time)-cg.timeFraction)/(float)cg.predictedPlayerState.hackingBaseTime)*HEALTH_WIDTH*cgs.widthRatioCoef;

	if (percent > HEALTH_WIDTH*cgs.widthRatioCoef || percent < 1.0f) {
		return;
	}

	//color of the bar
	aColor[0] = 1.0f;
	aColor[1] = 1.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.4f;

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out done area
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH*cgs.widthRatioCoef, HEALTH_HEIGHT, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f*cgs.widthRatioCoef, y+1.0f, percent-1.0f*cgs.widthRatioCoef, HEALTH_HEIGHT-1.0f, aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, ((HEALTH_WIDTH-1.0f)*cgs.widthRatioCoef-percent), HEALTH_HEIGHT-1.0f, cColor);

	//draw the hacker icon
	CG_DrawPic(x, y-HEALTH_WIDTH, HEALTH_WIDTH*cgs.widthRatioCoef, HEALTH_WIDTH, cgs.media.hackerIconShader);
}

//generic timing bar
int cg_genericTimerBar = 0;
int cg_genericTimerDur = 0;
vec4_t cg_genericTimerColor;
#define CGTIMERBAR_H			50.0f
#define CGTIMERBAR_W			10.0f
#define CGTIMERBAR_X			(SCREEN_WIDTH-CGTIMERBAR_W-120.0f)
#define CGTIMERBAR_Y			(SCREEN_HEIGHT-CGTIMERBAR_H-20.0f)
void CG_DrawGenericTimerBar(void) {
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x = SCREEN_WIDTH - (SCREEN_WIDTH - CGTIMERBAR_X)*cgs.widthRatioCoef;
	float y = CGTIMERBAR_Y;
	float percent = (((cg_genericTimerBar-cg.time)-cg.timeFraction)/(float)cg_genericTimerDur)*CGTIMERBAR_H;

	if (percent > CGTIMERBAR_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar
	aColor[0] = cg_genericTimerColor[0];
	aColor[1] = cg_genericTimerColor[1];
	aColor[2] = cg_genericTimerColor[2];
	aColor[3] = cg_genericTimerColor[3];

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, CGTIMERBAR_W*cgs.widthRatioCoef, CGTIMERBAR_H, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f*cgs.widthRatioCoef, y+1.0f+(CGTIMERBAR_H-percent), (CGTIMERBAR_W-2.0f)*cgs.widthRatioCoef, CGTIMERBAR_H-1.0f-(CGTIMERBAR_H-percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f*cgs.widthRatioCoef, y+1.0f, (CGTIMERBAR_W-2.0f)*cgs.widthRatioCoef, CGTIMERBAR_H-percent, cColor);
}

static qboolean CG_CrosshairColour(vec4_t ecolor, centity_t *crossEnt, int chEntValid) {
	qboolean corona = qfalse;
	entityState_t *crossES = &cg_entities[cg.crosshairClientNum].currentState;
	//set color based on what kind of ent is under crosshair
	if ( cg.crosshairClientNum >= ENTITYNUM_WORLD ) {
		trap_R_SetColor( NULL );
	}
	//rwwFIXMEFIXME: Write this a different way, it's getting a bit too sloppy looking
	else if (chEntValid &&
		(crossES->number < MAX_CLIENTS || crossES->eType == ET_NPC || crossES->shouldtarget ||
		crossES->health || //always show ents with health data under crosshair
		(crossES->eType == ET_MOVER && crossES->bolt1 && cg.playerCent->currentState.weapon == WP_SABER) ||
		(crossES->eType == ET_MOVER && crossES->teamowner))) {

		clientInfo_t *ciCross = &cgs.clientinfo[crossES->number];
		clientInfo_t *ciFollow = &cgs.clientinfo[cg.playerCent->currentState.number];

		crossEnt = &cg_entities[cg.crosshairClientNum];

		if ( crossES->powerups & (1 <<PW_CLOAKED) ) {
		//don't show up for cloaked guys
			ecolor[0] = 1.0;//R
			ecolor[1] = 1.0;//G
			ecolor[2] = 1.0;//B
		} else if ( crossES->number < MAX_CLIENTS ) {
			if (cgs.gametype >= GT_TEAM &&
				ciCross->team == ciFollow->team ) {
			//Allies are green
				ecolor[0] = 0.0;//R
				ecolor[1] = 1.0;//G
				ecolor[2] = 0.0;//B
			} else {
				if (cgs.gametype == GT_POWERDUEL &&
					ciCross->team == ciFollow->duelTeam) {
				//on the same duel team in powerduel, so he's a friend
					ecolor[0] = 0.0;//R
					ecolor[1] = 1.0;//G
					ecolor[2] = 0.0;//B
				} else { //Enemies are red
					ecolor[0] = 1.0;//R
					ecolor[1] = 0.0;//G
					ecolor[2] = 0.0;//B
				}
			}

			if (cg.snap->ps.duelInProgress) {
				if (crossES->number != cg.snap->ps.duelIndex &&
					crossES->number != cg.snap->ps.clientNum) {
				//grey out crosshair for everyone but your foe if you're in a duel
					ecolor[0] = 0.4f;
					ecolor[1] = 0.4f;
					ecolor[2] = 0.4f;
				}
			} else if (crossES->bolt1) {
			//this fellow is in a duel. We just checked if we were in a duel above, so
			//this means we aren't and he is. Which of course means our crosshair greys out over him.
				ecolor[0] = 0.4f;
				ecolor[1] = 0.4f;
				ecolor[2] = 0.4f;
			}
		} else if (crossES->shouldtarget || crossES->eType == ET_NPC) {
			//VectorCopy( crossEnt->startRGBA, ecolor );
			if ( !ecolor[0] && !ecolor[1] && !ecolor[2] ) {
				// We really don't want black, so set it to yellow
				ecolor[0] = 1.0f;//R
				ecolor[1] = 0.8f;//G
				ecolor[2] = 0.3f;//B
			}

			if (crossES->eType == ET_NPC) {
				int plTeam;
				if (cgs.gametype == GT_SIEGE) {
					plTeam = ciFollow->team;
				} else {
					plTeam = NPCTEAM_PLAYER;
				}

				if ( crossES->powerups & (1 <<PW_CLOAKED) ) {
					ecolor[0] = 1.0f;//R
					ecolor[1] = 1.0f;//G
					ecolor[2] = 1.0f;//B
				} else if ( !crossES->teamowner ) {
					//not on a team
					if (!crossES->teamowner ||
						crossES->NPC_class == CLASS_VEHICLE) {
						//neutral
						if (crossES->owner < MAX_CLIENTS) {
						//base color on who is pilotting this thing
							if (cgs.gametype >= GT_TEAM && ciCross->team == ciFollow->team) {
							//friendly
								ecolor[0] = 0.0f;//R
								ecolor[1] = 1.0f;//G
								ecolor[2] = 0.0f;//B
							} else {
							//hostile
								ecolor[0] = 1.0f;//R
								ecolor[1] = 0.0f;//G
								ecolor[2] = 0.0f;//B
							}
						} else { //unmanned
							ecolor[0] = 1.0f;//R
							ecolor[1] = 1.0f;//G
							ecolor[2] = 0.0f;//B
						}
					} else {
						ecolor[0] = 1.0f;//R
						ecolor[1] = 0.0f;//G
						ecolor[2] = 0.0f;//B
					}
				} else if ( crossES->teamowner != plTeam ) {
				// on enemy team
					ecolor[0] = 1.0f;//R
					ecolor[1] = 0.0f;//G
					ecolor[2] = 0.0f;//B
				} else {
				//a friend
					ecolor[0] = 0.0f;//R
					ecolor[1] = 1.0f;//G
					ecolor[2] = 0.0f;//B
				}
			} else if ( crossES->teamowner == TEAM_RED
				|| crossES->teamowner == TEAM_BLUE ) {
				if (cgs.gametype < GT_TEAM) {
				//not teamplay, just neutral then
					ecolor[0] = 1.0f;//R
					ecolor[1] = 1.0f;//G
					ecolor[2] = 0.0f;//B
				} else if ( crossES->teamowner != ciFollow->team ) {
				//on the enemy team
					ecolor[0] = 1.0f;//R
					ecolor[1] = 0.0f;//G
					ecolor[2] = 0.0f;//B
				} else {
				//on my team
					ecolor[0] = 0.0f;//R
					ecolor[1] = 1.0f;//G
					ecolor[2] = 0.0f;//B
				}
			} else if (crossES->owner == cg.playerCent->currentState.number ||
				(cgs.gametype >= GT_TEAM && crossES->teamowner == ciFollow->team)) {
				ecolor[0] = 0.0f;//R
				ecolor[1] = 1.0f;//G
				ecolor[2] = 0.0f;//B
			} else if (crossES->teamowner == 16
				|| (cgs.gametype >= GT_TEAM && crossES->teamowner
				&& crossES->teamowner != ciFollow->team)) {
				ecolor[0] = 1.0f;//R
				ecolor[1] = 0.0f;//G
				ecolor[2] = 0.0f;//B
			}
		} else if (crossES->eType == ET_MOVER && crossES->bolt1
			&& cg.playerCent->currentState.weapon == WP_SABER) {
		//can push/pull this mover. Only show it if we're using the saber.
			ecolor[0] = 0.2f;
			ecolor[1] = 0.5f;
			ecolor[2] = 1.0f;

			corona = qtrue;
		} else if (crossES->eType == ET_MOVER && crossES->teamowner) {
		//a team owns this - if it's my team green, if not red, if not teamplay then yellow
			if (cgs.gametype < GT_TEAM) {
				ecolor[0] = 1.0f;//R
				ecolor[1] = 1.0f;//G
				ecolor[2] = 0.0f;//B
			} else if (ciFollow->team != crossES->teamowner) {
			//not my team
				ecolor[0] = 1.0f;//R
				ecolor[1] = 0.0f;//G
				ecolor[2] = 0.0f;//B
			} else {
			//my team
				ecolor[0] = 0.0f;//R
				ecolor[1] = 1.0f;//G
				ecolor[2] = 0.0f;//B
			}
		} else if (crossES->health) {
			if (!crossES->teamowner || cgs.gametype < GT_TEAM) {
			//not owned by a team or teamplay
				ecolor[0] = 1.0f;
				ecolor[1] = 1.0f;
				ecolor[2] = 0.0f;
			} else if (crossES->teamowner == ciFollow->team) {
			//owned by my team
				ecolor[0] = 0.0f;
				ecolor[1] = 1.0f;
				ecolor[2] = 0.0f;
			} else {
			//hostile
				ecolor[0] = 1.0f;
				ecolor[1] = 0.0f;
				ecolor[2] = 0.0f;
			}
		}
		ecolor[3] = 1.0f;
		trap_R_SetColor( ecolor );
	}
	return corona;
}

/*
=================
CG_DrawCrosshair
=================
*/

float cg_crosshairPrevPosX = 0;
float cg_crosshairPrevPosY = 0;
#define CRAZY_CROSSHAIR_MAX_ERROR_X	(100.0f*SCREEN_WIDTH/SCREEN_HEIGHT)
#define CRAZY_CROSSHAIR_MAX_ERROR_Y	(100.0f)
void CG_LerpCrosshairPos( float *x, float *y )
{
	if ( cg_crosshairPrevPosX )
	{//blend from old pos
		float maxMove = 30.0f * ((float)cg.frametime/500.0f) * (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT;
		float xDiff = (*x - cg_crosshairPrevPosX);
		if ( fabs(xDiff) > CRAZY_CROSSHAIR_MAX_ERROR_X )
		{
			maxMove = CRAZY_CROSSHAIR_MAX_ERROR_X;
		}
		if ( xDiff > maxMove )
		{
			*x = cg_crosshairPrevPosX + maxMove;
		}
		else if ( xDiff < -maxMove )
		{
			*x = cg_crosshairPrevPosX - maxMove;
		}
	}
	cg_crosshairPrevPosX = *x;

	if ( cg_crosshairPrevPosY )
	{//blend from old pos
		float maxMove = 30.0f * ((float)cg.frametime/500.0f);
		float yDiff = (*y - cg_crosshairPrevPosY);
		if ( fabs(yDiff) > CRAZY_CROSSHAIR_MAX_ERROR_Y )
		{
			maxMove = CRAZY_CROSSHAIR_MAX_ERROR_X;
		}
		if ( yDiff > maxMove )
		{
			*y = cg_crosshairPrevPosY + maxMove;
		}
		else if ( yDiff < -maxMove )
		{
			*y = cg_crosshairPrevPosY - maxMove;
		}
	}
	cg_crosshairPrevPosY = *y;
}

vec3_t cg_crosshairPos={0,0,0};
static void CG_DrawCrosshair( vec3_t worldPoint, int chEntValid ) {
    float		w, h;
    qhandle_t	hShader = 0;
    float		f;
    float		x, y;
    qboolean	corona = qfalse;
    vec4_t		ecolor = {0,0,0,0};
    vec4_t		hcolor;
    centity_t	*crossEnt = NULL;
    float		chX, chY;

    if ( worldPoint )
    {
        VectorCopy( worldPoint, cg_crosshairPos );
    }

    if (cg_strafeHelper.integer & SHELPER_CROSSHAIR)
    {
        return;
    }

    if ( !cg_drawCrosshair.integer )
    {
        return;
    }

    if (cg.predictedPlayerState.fallingToDeath)
    {
        return;
    }

    if ( cg.predictedPlayerState.zoomMode != 0 )
    {//not while scoped
        return;
    }

    if ( cg_crosshairHealth.integer )
    {
        CG_ColorForHealth( hcolor );
        trap_R_SetColor( hcolor );
    }
    else if (cg_crosshairSaberStyleColor.integer && cg.predictedPlayerState.weapon == WP_SABER) {
        switch (cg.predictedPlayerState.fd.saberDrawAnimLevel)
        {
            case 1://blue
            case 5://Tavion
                hcolor[0] = 0.0f;
                hcolor[1] = 0.0f;
                hcolor[2] = 1.0f;
                break;
            case 2://yellow
            case 6://SS_DUAL
            case 7://SS_STAFF
                hcolor[0] = 1.0f;
                hcolor[1] = 1.0f;
                hcolor[2] = 0.0f;
                break;
            case 3://red
            case 4://Desann
                hcolor[0] = 1.0f;
                hcolor[1] = 0.0f;
                hcolor[2] = 0.0f;
                break;
            default:
                hcolor[0] = 1.0f;
                hcolor[1] = 1.0f;
                hcolor[2] = 1.0f;
                break;
        }

        hcolor[3] = cg.crosshairColor[3];

        trap_R_SetColor(hcolor);
    }
    else if (cg_drawCrosshair.integer == 10) {
        hcolor[0] = 1.0f;
        hcolor[1] = 1.0f;
        hcolor[2] = 1.0f;
        hcolor[3] = 3.0f;
        trap_R_SetColor(hcolor);
    }
    else if ((cg.crosshairColor[0] || cg.crosshairColor[1] || cg.crosshairColor[2]) && !cg_crosshairIdentifyTarget.integer)
    {
        hcolor[0] = cg.crosshairColor[0];
        hcolor[1] = cg.crosshairColor[1];
        hcolor[2] = cg.crosshairColor[2];
        hcolor[3] = cg.crosshairColor[3];
        trap_R_SetColor( hcolor );
    }
    else
    {
        //set color based on what kind of ent is under crosshair
        if ( cg.crosshairClientNum >= ENTITYNUM_WORLD )
        {
            trap_R_SetColor( NULL );
        }
            //rwwFIXMEFIXME: Write this a different way, it's getting a bit too sloppy looking
        else if (chEntValid &&
                 (cg_entities[cg.crosshairClientNum].currentState.number < MAX_CLIENTS ||
                  cg_entities[cg.crosshairClientNum].currentState.eType == ET_NPC ||
                  cg_entities[cg.crosshairClientNum].currentState.shouldtarget ||
                  cg_entities[cg.crosshairClientNum].currentState.health || //always show ents with health data under crosshair
                  (cg_entities[cg.crosshairClientNum].currentState.eType == ET_MOVER && cg_entities[cg.crosshairClientNum].currentState.bolt1 && cg.predictedPlayerState.weapon == WP_SABER) ||
                  (cg_entities[cg.crosshairClientNum].currentState.eType == ET_MOVER && cg_entities[cg.crosshairClientNum].currentState.teamowner)))
        {
            crossEnt = &cg_entities[cg.crosshairClientNum];

            if ( crossEnt->currentState.powerups & (1 <<PW_CLOAKED) )
            { //don't show up for cloaked guys
                ecolor[0] = 1.0;//R
                ecolor[1] = 1.0;//G
                ecolor[2] = 1.0;//B
            }
            else if ( crossEnt->currentState.number < MAX_CLIENTS )
            {
                if (cgs.gametype >= GT_TEAM &&
                    cgs.clientinfo[crossEnt->currentState.number].team == cgs.clientinfo[cg.snap->ps.clientNum].team )
                {
                    //Allies are green
                    ecolor[0] = 0.0;//R
                    ecolor[1] = 1.0;//G
                    ecolor[2] = 0.0;//B
                }
                else
                {
                    if (cgs.gametype == GT_POWERDUEL &&
                        cgs.clientinfo[crossEnt->currentState.number].duelTeam == cgs.clientinfo[cg.snap->ps.clientNum].duelTeam)
                    { //on the same duel team in powerduel, so he's a friend
                        ecolor[0] = 0.0;//R
                        ecolor[1] = 1.0;//G
                        ecolor[2] = 0.0;//B
                    }
                    else
                    { //Enemies are red
                        ecolor[0] = 1.0;//R
                        ecolor[1] = 0.0;//G
                        ecolor[2] = 0.0;//B
                    }
                }

                if (cg.snap->ps.duelInProgress)
                {
                    if (crossEnt->currentState.number != cg.snap->ps.duelIndex)
                    { //grey out crosshair for everyone but your foe if you're in a duel
                        ecolor[0] = 0.4f;
                        ecolor[1] = 0.4f;
                        ecolor[2] = 0.4f;
                    }
                }
                else if (crossEnt->currentState.bolt1)
                { //this fellow is in a duel. We just checked if we were in a duel above, so
                    //this means we aren't and he is. Which of course means our crosshair greys out over him.
                    ecolor[0] = 0.4f;
                    ecolor[1] = 0.4f;
                    ecolor[2] = 0.4f;
                }
            }
            else if (crossEnt->currentState.shouldtarget || crossEnt->currentState.eType == ET_NPC)
            {
                //VectorCopy( crossEnt->startRGBA, ecolor );
                if ( !ecolor[0] && !ecolor[1] && !ecolor[2] )
                {
                    // We really don't want black, so set it to yellow
                    ecolor[0] = 1.0f;//R
                    ecolor[1] = 0.8f;//G
                    ecolor[2] = 0.3f;//B
                }

                if (crossEnt->currentState.eType == ET_NPC)
                {
                    int plTeam;
                    if (cgs.gametype == GT_SIEGE)
                    {
                        plTeam = cg.predictedPlayerState.persistant[PERS_TEAM];
                    }
                    else
                    {
                        plTeam = NPCTEAM_PLAYER;
                    }

                    if ( crossEnt->currentState.powerups & (1 <<PW_CLOAKED) )
                    {
                        ecolor[0] = 1.0f;//R
                        ecolor[1] = 1.0f;//G
                        ecolor[2] = 1.0f;//B
                    }
                    else if ( !crossEnt->currentState.teamowner )
                    { //not on a team
                        if (!crossEnt->currentState.teamowner ||
                            crossEnt->currentState.NPC_class == CLASS_VEHICLE)
                        { //neutral
                            if (crossEnt->currentState.owner < MAX_CLIENTS)
                            { //base color on who is pilotting this thing
                                clientInfo_t *ci = &cgs.clientinfo[crossEnt->currentState.owner];

                                if (cgs.gametype >= GT_TEAM && ci->team == cg.predictedPlayerState.persistant[PERS_TEAM])
                                { //friendly
                                    ecolor[0] = 0.0f;//R
                                    ecolor[1] = 1.0f;//G
                                    ecolor[2] = 0.0f;//B
                                }
                                else
                                { //hostile
                                    ecolor[0] = 1.0f;//R
                                    ecolor[1] = 0.0f;//G
                                    ecolor[2] = 0.0f;//B
                                }
                            }
                            else
                            { //unmanned
                                ecolor[0] = 1.0f;//R
                                ecolor[1] = 1.0f;//G
                                ecolor[2] = 0.0f;//B
                            }
                        }
                        else
                        {
                            ecolor[0] = 1.0f;//R
                            ecolor[1] = 0.0f;//G
                            ecolor[2] = 0.0f;//B
                        }
                    }
                    else if ( crossEnt->currentState.teamowner != plTeam )
                    {// on enemy team
                        ecolor[0] = 1.0f;//R
                        ecolor[1] = 0.0f;//G
                        ecolor[2] = 0.0f;//B
                    }
                    else
                    { //a friend
                        ecolor[0] = 0.0f;//R
                        ecolor[1] = 1.0f;//G
                        ecolor[2] = 0.0f;//B
                    }
                }
                else if ( crossEnt->currentState.teamowner == TEAM_RED
                          || crossEnt->currentState.teamowner == TEAM_BLUE )
                {
                    if (cgs.gametype < GT_TEAM)
                    { //not teamplay, just neutral then
                        ecolor[0] = 1.0f;//R
                        ecolor[1] = 1.0f;//G
                        ecolor[2] = 0.0f;//B
                    }
                    else if ( crossEnt->currentState.teamowner != cgs.clientinfo[cg.snap->ps.clientNum].team )
                    { //on the enemy team
                        ecolor[0] = 1.0f;//R
                        ecolor[1] = 0.0f;//G
                        ecolor[2] = 0.0f;//B
                    }
                    else
                    { //on my team
                        ecolor[0] = 0.0f;//R
                        ecolor[1] = 1.0f;//G
                        ecolor[2] = 0.0f;//B
                    }
                }
                else if (crossEnt->currentState.owner == cg.snap->ps.clientNum ||
                         (cgs.gametype >= GT_TEAM && crossEnt->currentState.teamowner == cgs.clientinfo[cg.snap->ps.clientNum].team))
                {
                    ecolor[0] = 0.0f;//R
                    ecolor[1] = 1.0f;//G
                    ecolor[2] = 0.0f;//B
                }
                else if (crossEnt->currentState.teamowner == 16 ||
                         (cgs.gametype >= GT_TEAM && crossEnt->currentState.teamowner && crossEnt->currentState.teamowner != cgs.clientinfo[cg.snap->ps.clientNum].team))
                {
                    ecolor[0] = 1.0f;//R
                    ecolor[1] = 0.0f;//G
                    ecolor[2] = 0.0f;//B
                }
            }
            else if (crossEnt->currentState.eType == ET_MOVER && crossEnt->currentState.bolt1 && cg.predictedPlayerState.weapon == WP_SABER)
            { //can push/pull this mover. Only show it if we're using the saber.
                ecolor[0] = 0.2f;
                ecolor[1] = 0.5f;
                ecolor[2] = 1.0f;

                corona = qtrue;
            }
            else if (crossEnt->currentState.eType == ET_MOVER && crossEnt->currentState.teamowner)
            { //a team owns this - if it's my team green, if not red, if not teamplay then yellow
                if (cgs.gametype < GT_TEAM)
                {
                    ecolor[0] = 1.0f;//R
                    ecolor[1] = 1.0f;//G
                    ecolor[2] = 0.0f;//B
                }
                else if (cg.predictedPlayerState.persistant[PERS_TEAM] != crossEnt->currentState.teamowner)
                { //not my team
                    ecolor[0] = 1.0f;//R
                    ecolor[1] = 0.0f;//G
                    ecolor[2] = 0.0f;//B
                }
                else
                { //my team
                    ecolor[0] = 0.0f;//R
                    ecolor[1] = 1.0f;//G
                    ecolor[2] = 0.0f;//B
                }
            }
            else if (crossEnt->currentState.health)
            {
                if (!crossEnt->currentState.teamowner || cgs.gametype < GT_TEAM)
                { //not owned by a team or teamplay
                    ecolor[0] = 1.0f;
                    ecolor[1] = 1.0f;
                    ecolor[2] = 0.0f;
                }
                else if (crossEnt->currentState.teamowner == cg.predictedPlayerState.persistant[PERS_TEAM])
                { //owned by my team
                    ecolor[0] = 0.0f;
                    ecolor[1] = 1.0f;
                    ecolor[2] = 0.0f;
                }
                else
                { //hostile
                    ecolor[0] = 1.0f;
                    ecolor[1] = 0.0f;
                    ecolor[2] = 0.0f;
                }
            }

            ecolor[3] = 1.0f;

            trap_R_SetColor( ecolor );
        }
    }

    if ( cg_dynamicCrosshair.integer < 2 && cg.predictedPlayerState.m_iVehicleNum )
    {//I'm in a vehicle
        centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
        if ( vehCent
             && vehCent->m_pVehicle
             && vehCent->m_pVehicle->m_pVehicleInfo
             && vehCent->m_pVehicle->m_pVehicleInfo->crosshairShaderHandle )
        {
            hShader = vehCent->m_pVehicle->m_pVehicleInfo->crosshairShaderHandle;
        }
        //bigger by default
        w = cg_crosshairSize.value*2.0f;
        h = w;
    }
    else
    {
        w = h = cg_crosshairSize.value;
    }


    //JAPRO - Clientside - Option to disable crosshair scaling.
    if (!cg_crosshairSizeScale.integer || cg_drawCrosshair.integer == 10) {
        w = (float)(cg_crosshairSize.value / (cgs.glconfig.vidWidth * (1.0 / SCREEN_WIDTH)));
        h = (float)(cg_crosshairSize.value / (cgs.glconfig.vidHeight * (1.0 / SCREEN_HEIGHT)));
    }

    // pulse the size of the crosshair when picking up items
    if (cg_dynamicCrosshair.integer > 2) {
        f = cg.time - cg.itemPickupBlendTime;
        if ( f > 0 && f < ITEM_BLOB_TIME ) {
            f /= ITEM_BLOB_TIME;
            w *= ( 1 + f );
            h *= ( 1 + f );
        }
    }

    if ( worldPoint && VectorLength( worldPoint ) )
    {
        if ( !CG_WorldCoordToScreenCoordFloat( worldPoint, &x, &y ) )
        {//off screen, don't draw it
            return;
        }
        //CG_LerpCrosshairPos( &x, &y );
        x -= SCREEN_WIDTH / 2;
        y -= SCREEN_HEIGHT / 2;
    }
    else
    {
        x = cg_crosshairX.integer;
        y = cg_crosshairY.integer;
    }

    if ( !hShader )
    {
        hShader = cgs.media.crosshairShader[cg_drawCrosshair.integer % NUM_CROSSHAIRS];
    }

    if (cg_crosshairSizeScale.integer && cg_drawCrosshair.integer != 10)
        w *= cgs.widthRatioCoef;

    chX = x + cg.refdef.x + 0.5 * (SCREEN_WIDTH - w);
    chY = y + cg.refdef.y + 0.5 * (SCREEN_HEIGHT - h);
    trap_R_DrawStretchPic(chX, chY, w, h, 0, 0, 1, 1, hShader);

    //draw a health bar directly under the crosshair if we're looking at something
    //that takes damage
    if (crossEnt &&	crossEnt->currentState.maxhealth && cg_drawPlayerNames.integer < 2)//loda
    {
        CG_DrawHealthBar(crossEnt, chX, chY, w, h);
        chY += HEALTH_HEIGHT*2;
    }
    else if (crossEnt && crossEnt->currentState.number < MAX_CLIENTS)
    {
        if (cgs.gametype == GT_SIEGE)
        {
            CG_DrawSiegeInfo(crossEnt, chX, chY, w, h);
            chY += HEALTH_HEIGHT*4;
        }
        if (cg.crosshairVehNum && cg.time == cg.crosshairVehTime)
        { //it was in the crosshair this frame
            centity_t *hisVeh = &cg_entities[cg.crosshairVehNum];

            if (hisVeh->currentState.eType == ET_NPC &&
                hisVeh->currentState.NPC_class == CLASS_VEHICLE &&
                hisVeh->currentState.maxhealth &&
                hisVeh->m_pVehicle)
            { //draw the health for this vehicle
                CG_DrawHealthBar(hisVeh, chX, chY, w, h);
                chY += HEALTH_HEIGHT*2;
            }
        }
    }

    if (cg.predictedPlayerState.hackingTime)
    { //hacking something
        CG_DrawHaqrBar(chX, chY, w, h);
    }

    if (cg_genericTimerBar > cg.time)
    { //draw generic timing bar, can be used for whatever
        CG_DrawGenericTimerBar();
    }

    if ( corona ) // drawing extra bits
    {
        ecolor[3] = 0.5f;
        ecolor[0] = ecolor[1] = ecolor[2] = (1 - ecolor[3]) * ( sin( cg.time * 0.001f ) * 0.08f + 0.35f ); // don't draw full color
        ecolor[3] = 1.0f;

        trap_R_SetColor( ecolor );

        w *= 2.0f;
        h *= 2.0f;

        trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (SCREEN_WIDTH - w * cgs.widthRatioCoef),
                               y + cg.refdef.y + 0.5 * (SCREEN_HEIGHT - h),
                               w * cgs.widthRatioCoef, h, 0, 0, 1, 1, cgs.media.forceCoronaShader);
    }

    trap_R_SetColor( NULL );
}

qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y) {
    vec3_t trans;
    vec_t xc, yc;
    vec_t px, py;
    vec_t z;

    px = tan(cg.refdef.fov_x * (M_PI / 360) );
    py = tan(cg.refdef.fov_y * (M_PI / 360) );
	
    VectorSubtract(worldCoord, cg.refdef.vieworg, trans);
   
    xc = SCREEN_WIDTH / 2.0f;
    yc = SCREEN_HEIGHT / 2.0f;
    
	// z = how far is the object in our forward direction
    z = DotProduct(trans, cg.refdef.viewaxis[0]);
    if (z <= 0.001)
        return qfalse;

    *x = xc - DotProduct(trans, cg.refdef.viewaxis[1])*xc/(z*px);
    *y = yc - DotProduct(trans, cg.refdef.viewaxis[2])*yc/(z*py);

    return qtrue;
}

qboolean CG_WorldCoordToScreenCoord( vec3_t worldCoord, int *x, int *y ) {
	float	xF, yF;
	qboolean retVal = CG_WorldCoordToScreenCoordFloat( worldCoord, &xF, &yF );
	*x = (int)xF;
	*y = (int)yF;
	return retVal;
}

/*
====================
CG_SaberClashFlare
====================
*/
int cg_saberFlashTime = 0;
vec3_t cg_saberFlashPos = {0, 0, 0};
void CG_SaberClashFlare( void ) {
	float maxTime = 150.0f;
	float t;
	vec3_t dif;
	vec3_t color;
	float x,y;
	float v, len;
	trace_t tr;

	t = (cg.time - cg_saberFlashTime) + cg.timeFraction;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}

	// Don't do clashes for things that are behind us
	VectorSubtract( cg_saberFlashPos, cg.refdef.vieworg, dif );
	if ( DotProduct( dif, cg.refdef.viewaxis[0] ) < 0.2 ) {
		return;
	}

	CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, cg_saberFlashPos, -1, CONTENTS_SOLID );
	if ( tr.fraction < 1.0f ) {
		return;
	}

	len = VectorNormalize( dif );
	// clamp to a known range
	if ( len > 1200 ) {
		return;
	}

	v =  (1.0f - (t / maxTime )) * ((1.0f - (len / 1200.0f)) * 2.0f + 0.35f);
	if (v < 0.001f) {
		v = 0.001f;
	}

	CG_WorldCoordToScreenCoordFloat( cg_saberFlashPos, &x, &y );

	VectorSet( color, 0.8f, 0.8f, 0.8f );
	trap_R_SetColor( color );

	CG_DrawPic( x - ( v * 300 )*cgs.widthRatioCoef, y - ( v * 300 ),
				v * 600*cgs.widthRatioCoef, v * 600,
				cgs.media.saberFlare);
}

void CG_DottedLine( float x1, float y1, float x2, float y2, float dotSize, int numDots, vec4_t color, float alpha )
{
	float x, y, xDiff, yDiff, xStep, yStep;
	vec4_t colorRGBA;
	int dotNum = 0;

	VectorCopy4( color, colorRGBA );
	colorRGBA[3] = alpha;

	trap_R_SetColor( colorRGBA );

	xDiff = x2-x1;
	yDiff = y2-y1;
	xStep = xDiff/(float)numDots;
	yStep = yDiff/(float)numDots;

	for ( dotNum = 0; dotNum < numDots; dotNum++ )
	{
		x = x1 + (xStep*dotNum) - (dotSize*0.5f);
		y = y1 + (yStep*dotNum) - (dotSize*0.5f);

		CG_DrawPic( x, y, dotSize*cgs.widthRatioCoef, dotSize, cgs.media.whiteShader );
	}
}

void CG_BracketEntity( centity_t *cent, float radius )
{
	trace_t tr;
	vec3_t dif;
	float	len, size, lineLength, lineWidth;
	float	x,	y;
	clientInfo_t *local;
	qboolean isEnemy = qfalse;

	VectorSubtract( cent->lerpOrigin, cg.refdef.vieworg, dif );
	len = VectorNormalize( dif );

	if ( cg.crosshairClientNum != cent->currentState.clientNum
		&& (!cg.snap||cg.snap->ps.rocketLockIndex!= cent->currentState.clientNum) )
	{//if they're the entity you're locking onto or under your crosshair, always draw bracket
		//Hmm... for now, if they're closer than 2000, don't bracket?
		if ( len < 2000.0f )
		{
			return;
		}

		CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, cent->lerpOrigin, -1, CONTENTS_OPAQUE );

		//don't bracket if can't see them
		if ( tr.fraction < 1.0f )
		{
			return;
		}
	}

	if ( !CG_WorldCoordToScreenCoordFloat(cent->lerpOrigin, &x, &y) )
	{//off-screen, don't draw it
		return;
	}

	//just to see if it's centered
	//CG_DrawPic( x-2, y-2, 4, 4, cgs.media.whiteShader );

	local = &cgs.clientinfo[cg.snap->ps.clientNum];
	if ( cent->currentState.m_iVehicleNum //vehicle has a driver
		&& (cent->currentState.m_iVehicleNum-1) < MAX_CLIENTS
		&& cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].infoValid )
	{
		if ( cgs.gametype < GT_TEAM )
		{//ffa?
			isEnemy = qtrue;
			trap_R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
		}
		else if ( cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].team == local->team )
		{
			trap_R_SetColor ( g_color_table[ColorIndex(COLOR_GREEN)] );
		}
		else
		{
			isEnemy = qtrue;
			trap_R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
		}
	}
	else if ( cent->currentState.teamowner )
	{
		if ( cgs.gametype < GT_TEAM )
		{//ffa?
			isEnemy = qtrue;
			trap_R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
		}
		else if ( cent->currentState.teamowner != cg.predictedPlayerState.persistant[PERS_TEAM] )
		{// on enemy team
			isEnemy = qtrue;
			trap_R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
		}
		else
		{ //a friend
			trap_R_SetColor ( g_color_table[ColorIndex(COLOR_GREEN)] );
		}
	}
	else
	{//FIXME: if we want to ever bracket anything besides vehicles (like siege objectives we want to blow up), we should handle the coloring here
		trap_R_SetColor ( NULL );
	}
	
	if ( len <= 1.0f )
	{//super-close, max out at 400 times radius (which is HUGE)
		size = radius*400.0f;
	}
	else
	{//scale by dist
		size = radius*(400.0f/len);
	}

	if ( size < 1.0f )
	{
		size = 1.0f;
	}
	
	//length scales with dist
	lineLength = (size*0.1f);
	if ( lineLength < 0.5f )
	{//always visible
		lineLength = 0.5f;
	}
	//always visible width
	lineWidth = 1.0f;

	x -= (size*0.5f)*cgs.widthRatioCoef;
	y -= (size*0.5f);

	/*
	if ( x >= 0 && x <= 640
		&& y >= 0 && y <= 480 )
	*/
	{//brackets would be drawn on the screen, so draw them
	//upper left corner
		//horz
        CG_DrawPic( x, y, lineLength*cgs.widthRatioCoef, lineWidth, cgs.media.whiteShader );
		//vert
		CG_DrawPic( x, y, lineWidth*cgs.widthRatioCoef, lineLength, cgs.media.whiteShader );
		//upper right corner
		//horz
		CG_DrawPic( x+(size-lineLength)*cgs.widthRatioCoef, y, lineLength*cgs.widthRatioCoef, lineWidth, cgs.media.whiteShader );
		//vert
		CG_DrawPic( x+(size-lineWidth)*cgs.widthRatioCoef, y, lineWidth*cgs.widthRatioCoef, lineLength, cgs.media.whiteShader );
		//lower left corner
		//horz
		CG_DrawPic( x, y+size-lineWidth, lineLength*cgs.widthRatioCoef, lineWidth, cgs.media.whiteShader );
		//vert
		CG_DrawPic( x, y+size-lineLength, lineWidth*cgs.widthRatioCoef, lineLength, cgs.media.whiteShader );
		//lower right corner
		//horz
		CG_DrawPic( x+(size-lineLength)*cgs.widthRatioCoef, y+size-lineWidth, lineLength*cgs.widthRatioCoef, lineWidth, cgs.media.whiteShader );
		//vert
		CG_DrawPic( x+(size-lineWidth)*cgs.widthRatioCoef, y+size-lineLength, lineWidth*cgs.widthRatioCoef, lineLength, cgs.media.whiteShader );
	}
	//Lead Indicator...
	if ( cg_drawVehLeadIndicator.integer )
	{//draw the lead indicator
		if ( isEnemy )
		{//an enemy object
			if ( cent->currentState.NPC_class == CLASS_VEHICLE )
			{//enemy vehicle
				if ( !VectorCompare( cent->currentState.pos.trDelta, vec3_origin ) )
				{//enemy vehicle is moving
					if ( cg.predictedPlayerState.m_iVehicleNum )
					{//I'm in a vehicle
						centity_t		*veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
						if ( veh //vehicle cent
							&& veh->m_pVehicle//vehicle
							&& veh->m_pVehicle->m_pVehicleInfo//vehicle stats
							&& veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID > VEH_WEAPON_BASE )//valid vehicle weapon
						{
							vehWeaponInfo_t *vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID];
							if ( vehWeapon 
								&& vehWeapon->bIsProjectile//primary weapon's shot is a projectile
								&& !vehWeapon->bHasGravity//primary weapon's shot is not affected by gravity
								&& !vehWeapon->fHoming//primary weapon's shot is not homing
								&& vehWeapon->fSpeed )//primary weapon's shot has speed
							{//our primary weapon's projectile has a speed
								vec3_t vehDiff, vehLeadPos;
								float vehDist, eta;
								float leadX, leadY;
								
								VectorSubtract( cent->lerpOrigin, cg.predictedVehicleState.origin, vehDiff );
								vehDist = VectorNormalize( vehDiff );
								eta = (vehDist/vehWeapon->fSpeed);//how many seconds it would take for my primary weapon's projectile to get from my ship to theirs
								//now extrapolate their position that number of seconds into the future based on their velocity
								VectorMA( cent->lerpOrigin, eta, cent->currentState.pos.trDelta, vehLeadPos );
								//now we have where we should be aiming at, project that onto the screen at a 2D co-ord
								if ( !CG_WorldCoordToScreenCoordFloat(cent->lerpOrigin, &x, &y) )
								{//off-screen, don't draw it
									return;
								}
								if ( !CG_WorldCoordToScreenCoordFloat(vehLeadPos, &leadX, &leadY) )
								{//off-screen, don't draw it
									//just draw the line
									CG_DottedLine( x, y, leadX, leadY, 1, 10, g_color_table[ColorIndex(COLOR_RED)], 0.5f );
									return;
								}
								//draw a line from the ship's cur pos to the lead pos
								CG_DottedLine( x, y, leadX, leadY, 1, 10, g_color_table[ColorIndex(COLOR_RED)], 0.5f );
								//now draw the lead indicator
								trap_R_SetColor ( g_color_table[ColorIndex(COLOR_RED)] );
								CG_DrawPic( leadX-8*cgs.widthRatioCoef, leadY-8, 16*cgs.widthRatioCoef, 16, trap_R_RegisterShader( "gfx/menus/radar/lead" ) );
							}
						}
					}
				}
			}
		}
	}
}

qboolean CG_InFighter( void )
{
	if ( cg.predictedPlayerState.m_iVehicleNum )
	{//I'm in a vehicle
		centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
	    if ( vehCent 
			&& vehCent->m_pVehicle 
			&& vehCent->m_pVehicle->m_pVehicleInfo
			&& vehCent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
		{//I'm in a fighter
			return qtrue;
		}
	}
	return qfalse;
}

qboolean CG_InATST( void )
{
	if ( cg.predictedPlayerState.m_iVehicleNum )
	{//I'm in a vehicle
		centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
	    if ( vehCent 
			&& vehCent->m_pVehicle 
			&& vehCent->m_pVehicle->m_pVehicleInfo
			&& vehCent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
		{//I'm in an atst
			return qtrue;
		}
	}
	return qfalse;
}

void CG_DrawBracketedEntities( void )
{
	int i;
	for ( i = 0; i < cg.bracketedEntityCount; i++ ) 
	{	
		centity_t *cent = &cg_entities[cg.bracketedEntities[i]];
		CG_BracketEntity( cent, CG_RadiusForCent( cent ) );
	}
}

//--------------------------------------------------------------
static void CG_DrawHolocronIcons(void) {
//--------------------------------------------------------------
	float icon_size = 40.0f;
	int i = 0;
	float startx = 10.0f;
	float starty = 10.0f;//SCREEN_HEIGHT - icon_size*3;

	float endx = icon_size;
	float endy = icon_size;

	int holocronBits = cg.playerPredicted ? cg.snap->ps.holocronBits : cg.playerCent->currentState.time2;

	if (cg.zoomMode) {
	//don't display over zoom mask
		return;
	}
	if (cgs.clientinfo[cg.playerCent->currentState.clientNum].team == TEAM_SPECTATOR) {
		return;
	}

	while (i < NUM_FORCE_POWERS) {
		if (holocronBits & (1 << forcePowerSorted[i])) {
			CG_DrawPic( startx, starty, endx*cgs.widthRatioCoef, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			starty += icon_size+2; //+2 for spacing
			if ((starty+icon_size) >= SCREEN_HEIGHT-100) {
				starty = 10;//SCREEN_HEIGHT - icon_size*3;
				startx += (icon_size+2)*cgs.widthRatioCoef;
			}
		}
		i++;
	}
}

static qboolean CG_IsDurationPower(int power) {
	if (power == FP_HEAL ||
		power == FP_SPEED ||
		power == FP_TELEPATHY ||
		power == FP_RAGE ||
		power == FP_PROTECT ||
		power == FP_ABSORB ||
		power == FP_SEE)
	{
		return qtrue;
	}
	return qfalse;
}

//--------------------------------------------------------------
static void CG_DrawActivePowers(void) {
//--------------------------------------------------------------
	float icon_size = 40.0f;
	int i = 0;
	float startx = (icon_size*2.0f+32.0f)*cgs.widthRatioCoef;
	float starty = (float)SCREEN_HEIGHT - icon_size*2.0f;

	float endx = icon_size*cgs.widthRatioCoef;
	float endy = icon_size;

	//don't display over zoom mask
	if (cg.zoomMode) {
		return;
	}

	if (cgs.clientinfo[cg.playerCent->currentState.number].team == TEAM_SPECTATOR)
		return;

	trap_R_SetColor( NULL );

	while (i < NUM_FORCE_POWERS) {
		if ((cg.playerCent->currentState.forcePowersActive & (1 << forcePowerSorted[i])) &&
			CG_IsDurationPower(forcePowerSorted[i])) {
			CG_DrawPic( startx, starty, endx, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			startx += (icon_size+2)*cgs.widthRatioCoef; //+2 for spacing
			if ((startx+endx) >= (float)SCREEN_WIDTH-80.0f) {
				startx = (icon_size*2.0f+16.0f)*cgs.widthRatioCoef;
				starty += (icon_size+2);
			}
		}
		i++;
	}
	//additionally, draw an icon force force rage recovery
	if (cg.playerPredicted && cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
		CG_DrawPic( startx, starty, endx, endy, cgs.media.rageRecShader);
}

//--------------------------------------------------------------
static void CG_DrawRocketLocking( int lockEntNum, int lockTime )
//--------------------------------------------------------------
{
	int		cx, cy;
	vec3_t	org;
	static int oldDif = 0;
	centity_t *cent = &cg_entities[lockEntNum];
	vec4_t color={0.0f,0.0f,0.0f,0.0f};
	float lockTimeInterval = ((cgs.gametype==GT_SIEGE)?2400.0f:1200.0f)/16.0f;
	//FIXME: if in a vehicle, use the vehicle's lockOnTime...
	int dif = ((cg.time - cg.snap->ps.rocketLockTime)/* + cg.timeFraction*/) / lockTimeInterval;
	int i;

	if (!cg.snap->ps.rocketLockTime || cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
		return;

	if ( cg.snap->ps.m_iVehicleNum )
	{//driving a vehicle
		centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
		if ( veh->m_pVehicle )
		{
			vehWeaponInfo_t *vehWeapon = NULL;
			if ( cg.predictedVehicleState.weaponstate == WEAPON_CHARGING_ALT )
			{
				if ( veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID > VEH_WEAPON_BASE 
					&& veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID < MAX_VEH_WEAPONS )
				{
					vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID];
				}
			}
			else
			{
				if ( veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID > VEH_WEAPON_BASE 
					&& veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID < MAX_VEH_WEAPONS )
				{
					vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID];
				}
			}
			if ( vehWeapon != NULL )
			{//we are trying to lock on with a valid vehicle weapon, so use *its* locktime, not the hard-coded one
				if ( !vehWeapon->iLockOnTime )
				{//instant lock-on
					dif = 10.0f;
				}
				else
				{//use the custom vehicle lockOnTime
					lockTimeInterval = (vehWeapon->iLockOnTime/16.0f);
					dif = ((cg.time - cg.snap->ps.rocketLockTime)/* + cg.timeFraction*/) / lockTimeInterval;
				}
			}
		}
	}
	//We can't check to see in pmove if players are on the same team, so we resort
	//to just not drawing the lock if a teammate is the locked on ent
	if (cg.snap->ps.rocketLockIndex >= 0 &&
		cg.snap->ps.rocketLockIndex < ENTITYNUM_NONE)
	{
		clientInfo_t *ci = NULL;

		if (cg.snap->ps.rocketLockIndex < MAX_CLIENTS)
		{
			ci = &cgs.clientinfo[cg.snap->ps.rocketLockIndex];
		}
		else
		{
			ci = cg_entities[cg.snap->ps.rocketLockIndex].npcClient;
		}

		if (ci)
		{
			if (ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team)
			{
				if (cgs.gametype >= GT_TEAM)
				{
					return;
				}
			}
			else if (cgs.gametype >= GT_TEAM)
			{
				centity_t *hitEnt = &cg_entities[cg.snap->ps.rocketLockIndex];
				if (hitEnt->currentState.eType == ET_NPC &&
					hitEnt->currentState.NPC_class == CLASS_VEHICLE &&
					hitEnt->currentState.owner < ENTITYNUM_WORLD)
				{ //this is a vehicle, if it has a pilot and that pilot is on my team, then...
					if (hitEnt->currentState.owner < MAX_CLIENTS)
					{
						ci = &cgs.clientinfo[hitEnt->currentState.owner];
					}
					else
					{
						ci = cg_entities[hitEnt->currentState.owner].npcClient;
					}
					if (ci && ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team)
					{
						return;
					}
				}
			}
		}
	}

	if (cg.snap->ps.rocketLockTime != -1) {
		lastvalidlockdif = dif;
	} else {
		dif = lastvalidlockdif;
	}

	if (!cent) {
		return;
	}

	VectorCopy( cent->lerpOrigin, org );

	if (CG_WorldCoordToScreenCoord(org, &cx, &cy)) {
		// we care about distance from enemy to eye, so this is good enough
		float sz = Distance( cent->lerpOrigin, cg.refdef.vieworg ) / 1024.0f; 
		if ( sz > 1.0f ) {
			sz = 1.0f;
		} else if ( sz < 0.0f ) {
			sz = 0.0f;
		}
		sz = (1.0f - sz) * (1.0f - sz) * 32 + 6;
		cy += sz * 0.5f;		
		
		if ( dif < 0 ) {
			oldDif = 0;
			return;
		} else if ( dif > 8 ) {
			dif = 8;
		}

		// do sounds
		if (oldDif != dif) {
			if (dif == 8) {
				if (cg.snap->ps.m_iVehicleNum) {
					trap_S_StartSound( org, 0, CHAN_AUTO, trap_S_RegisterSound( "sound/vehicles/weapons/common/lock.wav" ));
				} else {
					trap_S_StartSound( org, 0, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/rocket/lock.wav" ));
				}
			} else {
				if (cg.snap->ps.m_iVehicleNum) {
					trap_S_StartSound( org, 0, CHAN_AUTO, trap_S_RegisterSound( "sound/vehicles/weapons/common/tick.wav" ));
				} else {
					trap_S_StartSound( org, 0, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/rocket/tick.wav" ));
				}
			}
		}

		oldDif = dif;

		for (i = 0; i < dif; i++) {
			color[0] = 1.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 0.1f * i + 0.2f;

			trap_R_SetColor( color );
			// our slices are offset by about 45 degrees.
			CG_DrawRotatePic( cx - sz, cy - sz, sz, sz, i * 45.0f, trap_R_RegisterShaderNoMip( "gfx/2d/wedge" ));
		}

		// we are locked and loaded baby
		if (dif == 8) {
			//since we have hardcoded rotated pic with ratio fix above,
			//we want undepended (on mov_ratioFix) hardcoded coef here too
			float ratioFix = ((float)SCREEN_WIDTH*cgs.glconfig.vidHeight) / ((float)SCREEN_HEIGHT*cgs.glconfig.vidWidth);
			color[0] = color[1] = color[2] = sin(cg.time * 0.05 + cg.timeFraction * 0.05) * 0.5f + 0.5f;
			color[3] = 1.0f; // this art is additive, so the alpha value does nothing

			trap_R_SetColor( color );
			CG_DrawPic( cx - sz * ratioFix, cy - sz * 2, sz * 2 * ratioFix, sz * 2, trap_R_RegisterShaderNoMip( "gfx/2d/lock" ));
		}
	}
	trap_R_SetColor( NULL );
}
qboolean CG_InRollAnim( centity_t *cent );
//JAPRO - Clientside - Ground Distance function for use in jump detection for movement keys - Start
float PM_GroundDistance2(void)
{
    trace_t tr;
    vec3_t down;

    VectorCopy(cg.predictedPlayerState.origin, down);
    down[2] -= 4096;
    CG_Trace(&tr, cg.predictedPlayerState.origin, NULL, NULL, down, cg.predictedPlayerState.clientNum, MASK_SOLID);
    VectorSubtract(cg.predictedPlayerState.origin, tr.endpos, down);

    return VectorLength(down) - 24.0f;
}
static void CG_MovementKeys(centity_t *cent)
{
    usercmd_t cmd = { 0 };
    playerState_t *ps = NULL;
    int moveDir;
    float w, h, x, y, xOffset, yOffset;

    if (!cg.snap)
        return;

    ps = &cg.predictedPlayerState; //&cg.snap->ps;
    moveDir = ps->movementDir;

    if (cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback) {
        trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd );
    }
    else
    {
        float xyspeed = sqrtf( ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1] );
        float zspeed = ps->velocity[2];
        static float lastZSpeed = 0.0f;

        if ((PM_GroundDistance2() > 1 && zspeed > 8 && zspeed > lastZSpeed && !cg.snap->ps.fd.forceGripCripple) || (cg.snap->ps.pm_flags & PMF_JUMP_HELD))
            cmd.upmove = 1;
        else if ( (ps->pm_flags & PMF_DUCKED) || CG_InRollAnim(cent) )
            cmd.upmove = -1;

        if ( xyspeed < 9 )
            moveDir = -1;

        lastZSpeed = zspeed;

        switch ( moveDir )
        {
            case 0: // W
                cmd.forwardmove = 1;
                break;
            case 1: // WA
                cmd.forwardmove = 1;
                cmd.rightmove = -1;
                break;
            case 2: // A
                cmd.rightmove = -1;
                break;
            case 3: // AS
                cmd.rightmove = -1;
                cmd.forwardmove = -1;
                break;
            case 4: // S
                cmd.forwardmove = -1;
                break;
            case 5: // SD
                cmd.forwardmove = -1;
                cmd.rightmove = 1;
                break;
            case 6: // D
                cmd.rightmove = 1;
                break;
            case 7: // DW
                cmd.rightmove = 1;
                cmd.forwardmove = 1;
                break;
            default:
                break;
        }
    }

    x = SCREEN_WIDTH - (SCREEN_WIDTH - cg_movementKeysX.integer) * cgs.widthRatioCoef;
    y = cg_movementKeysY.integer;

    w = (16*cg_movementKeysSize.value)*cgs.widthRatioCoef;
    h = 16*cg_movementKeysSize.value;

    xOffset = yOffset = 0;
    if (cgs.newHud) {
        switch (cg_hudFiles.integer)
        {
            default:										break;
            case 0: 										break;
            case 1: xOffset += 51; 					break;
            case 2: xOffset += 26;  yOffset -= 3;	break;
            case 3: xOffset -= 18; 					break;
        }

        if (cgs.newHud) {
            if (!cg_drawScore.integer || cgs.gametype == GT_POWERDUEL || (cg.japro.detected && ps->stats[STAT_RACEMODE])) {
                yOffset += 12; //445
            }
            else if (cg_drawScore.integer > 1 && cgs.gametype >= GT_TEAM && cgs.gametype != GT_SIEGE) {
                xOffset -= cg_hudFiles.integer != 1 ? 12 : 23; //452 : //442
                yOffset -= 14; //420
            }
        }
    }

    x += xOffset*cgs.widthRatioCoef;
    y += yOffset;

    if (cmd.upmove < 0)
        CG_DrawPic( w*2 + x, y, w, h, cgs.media.keyCrouchOnShader );
    else
        CG_DrawPic( w*2 + x, y, w, h, cgs.media.keyCrouchOffShader );

    if (cmd.upmove > 0)
        CG_DrawPic( x, y, w, h, cgs.media.keyJumpOnShader );
    else
        CG_DrawPic( x, y, w, h, cgs.media.keyJumpOffShader );

    if (cmd.forwardmove < 0)
        CG_DrawPic( w + x, h + y, w, h, cgs.media.keyBackOnShader );
    else
        CG_DrawPic( w + x, h + y, w, h, cgs.media.keyBackOffShader );

    if (cmd.forwardmove > 0)
        CG_DrawPic( w + x, y, w, h, cgs.media.keyForwardOnShader );
    else
        CG_DrawPic( w + x, y, w, h, cgs.media.keyForwardOffShader );

    if (cmd.rightmove < 0)
        CG_DrawPic( x, h + y, w, h, cgs.media.keyLeftOnShader );
    else
        CG_DrawPic( x, h + y, w, h, cgs.media.keyLeftOffShader );

    if (cmd.rightmove > 0)
        CG_DrawPic( w*2 + x, h + y, w, h, cgs.media.keyRightOnShader );
    else
        CG_DrawPic( w*2 + x, h + y, w, h, cgs.media.keyRightOffShader );

}

extern void CG_CalcVehMuzzle(Vehicle_t *pVeh, centity_t *ent, int muzzleNum);
qboolean CG_CalcVehicleMuzzlePoint( int entityNum, vec3_t start, vec3_t d_f, vec3_t d_rt, vec3_t d_up) {
	centity_t *vehCent = &cg_entities[entityNum];
	if ( vehCent->m_pVehicle && vehCent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER ) {
	//draw from barrels
		VectorCopy( vehCent->lerpOrigin, start );
		start[2] += vehCent->m_pVehicle->m_pVehicleInfo->height-DEFAULT_MINS_2-48;
		AngleVectors( vehCent->lerpAngles, d_f, d_rt, d_up );
		/*
		mdxaBone_t		boltMatrix;
		int				bolt;
		vec3_t			yawOnlyAngles;
		
		VectorSet( yawOnlyAngles, 0, vehCent->lerpAngles[YAW], 0 );

		bolt = trap_G2API_AddBolt( vehCent->ghoul2, 0, "*flash1");
		trap_G2API_GetBoltMatrix( vehCent->ghoul2, 0, bolt, &boltMatrix, 
									yawOnlyAngles, vehCent->lerpOrigin, cg.time, 
									NULL, vehCent->modelScale );

		// work the matrix axis stuff into the original axis and origins used.
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, start );
		BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_X, d_f );
		VectorClear( d_rt );//don't really need this, do we?
		VectorClear( d_up );//don't really need this, do we?
		*/
	} else {
		//check to see if we're a turret gunner on this vehicle
		if ( cg.predictedPlayerState.generic1 )//as a passenger
		{//passenger in a vehicle
			if ( vehCent->m_pVehicle
				&& vehCent->m_pVehicle->m_pVehicleInfo 
				&& vehCent->m_pVehicle->m_pVehicleInfo->maxPassengers )
			{//a vehicle capable of carrying passengers
				int turretNum;
				for ( turretNum = 0; turretNum < MAX_VEHICLE_TURRETS; turretNum++ )
				{
					if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].iAmmoMax )
					{// valid turret
						if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].passengerNum == cg.predictedPlayerState.generic1 )
						{//I control this turret
							//Go through all muzzles, average their positions and directions and use the result for crosshair trace
							int vehMuzzle, numMuzzles = 0;
							vec3_t	muzzlesAvgPos={0},muzzlesAvgDir={0};
							int	i;

							for ( i = 0; i < MAX_VEHICLE_TURRET_MUZZLES; i++ )
							{
								vehMuzzle = vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].iMuzzle[i];
								if ( vehMuzzle )
								{
									vehMuzzle -= 1;
									CG_CalcVehMuzzle( vehCent->m_pVehicle, vehCent, vehMuzzle );
									VectorAdd( muzzlesAvgPos, vehCent->m_pVehicle->m_vMuzzlePos[vehMuzzle], muzzlesAvgPos );
									VectorAdd( muzzlesAvgDir, vehCent->m_pVehicle->m_vMuzzleDir[vehMuzzle], muzzlesAvgDir );
									numMuzzles++;
								}
								if ( numMuzzles )
								{
									VectorScale( muzzlesAvgPos, 1.0f/(float)numMuzzles, start );
									VectorScale( muzzlesAvgDir, 1.0f/(float)numMuzzles, d_f );
									VectorClear( d_rt );
									VectorClear( d_up );
									return qtrue;
								}
							}
						}
					}
				}
			}
		}
		VectorCopy( vehCent->lerpOrigin, start );
		AngleVectors( vehCent->lerpAngles, d_f, d_rt, d_up );
	}
	return qfalse;
}

//calc the muzzle point from the e-web itself
void CG_CalcEWebMuzzlePoint(centity_t *cent, vec3_t start, vec3_t d_f, vec3_t d_rt, vec3_t d_up)
{
	int bolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*cannonflash");

	assert(bolt != -1);

	if (bolt != -1)
	{
		mdxaBone_t boltMatrix;

		trap_G2API_GetBoltMatrix_NoRecNoRot(cent->ghoul2, 0, bolt, &boltMatrix, cent->lerpAngles, cent->lerpOrigin, cg.time, NULL, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, start);
		BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, d_f);

		//these things start the shot a little inside the bbox to assure not starting in something solid
		VectorMA(start, -16.0f, d_f, start);

		//I guess
		VectorClear( d_rt );//don't really need this, do we?
		VectorClear( d_up );//don't really need this, do we?
	}
}

/*
=================
CG_`Entity
=================
*/
#define MAX_XHAIR_DIST_ACCURACY	20000.0f
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;
	int			ignore;
	qboolean	bVehCheckTraceFromCamPos = qfalse;

	ignore = cg.playerCent->currentState.number;

	if ( cg_dynamicCrosshair.integer && cg.playerPredicted)
	{
		vec3_t d_f, d_rt, d_up;
		/*
		if ( cg.snap->ps.weapon == WP_NONE || 
			cg.snap->ps.weapon == WP_SABER || 
			cg.snap->ps.weapon == WP_STUN_BATON)
		{
			VectorCopy( cg.refdef.vieworg, start );
			AngleVectors( cg.refdef.viewangles, d_f, d_rt, d_up );
		}
		else
		*/
		//For now we still want to draw the crosshair in relation to the player's world coordinates
		//even if we have a melee weapon/no weapon.
		if ( cg.predictedPlayerState.m_iVehicleNum && (cg.predictedPlayerState.eFlags&EF_NODRAW) )
		{//we're *inside* a vehicle
			//do the vehicle's crosshair instead
			centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
			qboolean gunner = qfalse;

			//if (veh->currentState.owner == cg.predictedPlayerState.clientNum)
			{ //the pilot
				ignore = cg.predictedPlayerState.m_iVehicleNum;
				gunner = CG_CalcVehicleMuzzlePoint(cg.predictedPlayerState.m_iVehicleNum, start, d_f, d_rt, d_up);
			}
			/*
			else
			{ //a passenger
				ignore = cg.predictedPlayerState.m_iVehicleNum;
				VectorCopy( veh->lerpOrigin, start );
				AngleVectors( veh->lerpAngles, d_f, d_rt, d_up );
				VectorMA(start, 32.0f, d_f, start); //super hack
			}
			*/
			if ( veh->m_pVehicle 
				&& veh->m_pVehicle->m_pVehicleInfo 
				&& veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER 
				&& cg.distanceCull > MAX_XHAIR_DIST_ACCURACY 
				&& !gunner )
			{	
				//NOTE: on huge maps, the crosshair gets inaccurate at close range, 
				//		so we'll do an extra G2 trace from the cg.refdef.vieworg
				//		to see if we hit anything closer and auto-aim at it if so
				bVehCheckTraceFromCamPos = qtrue;
			}
		}
		else if (cg.snap && cg.snap->ps.weapon == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex &&
			cg_entities[cg.snap->ps.emplacedIndex].ghoul2 && cg_entities[cg.snap->ps.emplacedIndex].currentState.weapon == WP_NONE)
		{ //locked into our e-web, calc the muzzle from it
			CG_CalcEWebMuzzlePoint(&cg_entities[cg.snap->ps.emplacedIndex], start, d_f, d_rt, d_up);
		}
		else
		{
			if (cg.snap && cg.snap->ps.weapon == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
			{
				vec3_t pitchConstraint;

				ignore = cg.snap->ps.emplacedIndex;

				VectorCopy(cg.refdef.viewangles, pitchConstraint);

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.predictedPlayerState.viewangles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitchConstraint);
				}

				if (pitchConstraint[PITCH] > 40)
				{
					pitchConstraint[PITCH] = 40;
				}

				AngleVectors( pitchConstraint, d_f, d_rt, d_up );
			}
			else
			{
				vec3_t pitchConstraint;

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.predictedPlayerState.viewangles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitchConstraint);
				}

				AngleVectors( pitchConstraint, d_f, d_rt, d_up );
			}
			CG_CalcMuzzlePoint(cg.snap->ps.clientNum, start);
		}

		VectorMA( start, cg.distanceCull, d_f, end );
	}
	else if (cg_dynamicCrosshair.integer)
	{
		vec3_t d_f, d_rt, d_up;
		if (cg.playerCent->currentState.weapon == WP_EMPLACED_GUN && cg.playerCent->playerState->emplacedIndex &&
			cg_entities[cg.playerCent->playerState->emplacedIndex].ghoul2 && cg_entities[cg.playerCent->playerState->emplacedIndex].currentState.weapon == WP_NONE)
		{ //locked into our e-web, calc the muzzle from it
			CG_CalcEWebMuzzlePoint(&cg_entities[cg.playerCent->playerState->emplacedIndex], start, d_f, d_rt, d_up);
		}
		else
		{
			if (cg.playerCent->currentState.weapon == WP_EMPLACED_GUN)
			{
				vec3_t pitchConstraint;

				VectorCopy(cg.refdef.viewangles, pitchConstraint);

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.playerCent->lerpAngles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitchConstraint);
				}

				if (pitchConstraint[PITCH] > 40)
				{
					pitchConstraint[PITCH] = 40;
				}

				AngleVectors( pitchConstraint, d_f, d_rt, d_up );
			}
			else
			{
				vec3_t pitchConstraint;

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.playerCent->lerpAngles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitchConstraint);
				}

				AngleVectors( pitchConstraint, d_f, d_rt, d_up );
			}
			CG_CalcMuzzlePoint(cg.playerCent->currentState.number, start);
		}

		VectorMA( start, cg.distanceCull, d_f, end );
	}
	else
	{
		VectorCopy( cg.refdef.vieworg, start );
		VectorMA( start, 131072, cg.refdef.viewaxis[0], end );
	}

	if ( cg_dynamicCrosshair.integer && cg_dynamicCrosshairPrecision.integer )
	{ //then do a trace with ghoul2 models in mind
		CG_G2Trace( &trace, start, vec3_origin, vec3_origin, end, 
			ignore, CONTENTS_SOLID|CONTENTS_BODY );
		if ( bVehCheckTraceFromCamPos )
		{
			//NOTE: this MUST stay up to date with the method used in WP_VehCheckTraceFromCamPos
			centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
			trace_t	extraTrace;
			vec3_t	viewDir2End, extraEnd;
			float	minAutoAimDist = Distance( veh->lerpOrigin, cg.refdef.vieworg ) + (veh->m_pVehicle->m_pVehicleInfo->length/2.0f) + 200.0f;

			VectorSubtract( end, cg.refdef.vieworg, viewDir2End );
			VectorNormalize( viewDir2End );
			VectorMA( cg.refdef.vieworg, MAX_XHAIR_DIST_ACCURACY, viewDir2End, extraEnd );
			CG_G2Trace( &extraTrace, cg.refdef.vieworg, vec3_origin, vec3_origin, extraEnd, 
				ignore, CONTENTS_SOLID|CONTENTS_BODY );
			if ( !extraTrace.allsolid
				&& !extraTrace.startsolid )
			{
				if ( extraTrace.fraction < 1.0f )
				{
					if ( (extraTrace.fraction*MAX_XHAIR_DIST_ACCURACY) > minAutoAimDist )
					{
						if ( ((extraTrace.fraction*MAX_XHAIR_DIST_ACCURACY)-Distance( veh->lerpOrigin, cg.refdef.vieworg )) < (trace.fraction*cg.distanceCull) )
						{//this trace hit *something* that's closer than the thing the main trace hit, so use this result instead
							memcpy( &trace, &extraTrace, sizeof( trace_t ) );
						}
					}
				}
			}
		}
	}
	else
	{
		CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
			ignore, CONTENTS_SOLID|CONTENTS_BODY );
	}

	if (trace.entityNum < MAX_CLIENTS)
	{
		if (CG_IsMindTricked(cg_entities[trace.entityNum].currentState.trickedentindex,
			cg_entities[trace.entityNum].currentState.trickedentindex2,
			cg_entities[trace.entityNum].currentState.trickedentindex3,
			cg_entities[trace.entityNum].currentState.trickedentindex4,
			cg.playerCent->currentState.number))
		{
			if (cg.crosshairClientNum == trace.entityNum)
			{
				cg.crosshairClientNum = ENTITYNUM_NONE;
				cg.crosshairClientTime = 0;
			}

			CG_DrawCrosshair(trace.endpos, 0);
			return; //this entity is mind-tricking the current client, so don't render it
		}
	}

	if (cgs.clientinfo[cg.playerCent->currentState.number].team != TEAM_SPECTATOR) {
		if (trace.entityNum < /*MAX_CLIENTS*/ENTITYNUM_WORLD) {
			cg.crosshairClientNum = trace.entityNum;
			cg.crosshairClientTime = cg.time;
			
			if (cg.snap->ps.duelInProgress && mov_duelIsolation.integer
				&& trace.entityNum != cg.snap->ps.duelIndex
				&& trace.entityNum != cg.snap->ps.clientNum) {
				cg.crosshairClientNum = ENTITYNUM_NONE;
				cg.crosshairClientTime = 0;
				CG_DrawCrosshair(trace.endpos, 0);
				return;
			}

			if (cg.crosshairClientNum < ENTITYNUM_WORLD) {
				centity_t *veh = &cg_entities[cg.crosshairClientNum];

				if (veh->currentState.eType == ET_NPC &&
					veh->currentState.NPC_class == CLASS_VEHICLE &&
					veh->currentState.owner < MAX_CLIENTS) { //draw the name of the pilot then
					cg.crosshairClientNum = veh->currentState.owner;
					cg.crosshairVehNum = veh->currentState.number;
					cg.crosshairVehTime = cg.time;
				}
			}
			CG_DrawCrosshair(trace.endpos, 1);
		} else {
			CG_DrawCrosshair(trace.endpos, 0);
		}
	}

	//Raz: Put this back in so fading works again
	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// if the player is in fog, don't show it
	content = trap_CM_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}

//mme, taken from pugmod
void CG_SanitizeString( char *in, char *out ) {
	int i = 0;
	int r = 0;

	while (in[i]) {
		if (i >= 128-1) { //the ui truncates the name here..
			break;
		}
		if (in[i] == '^') {
			if (in[i+1] >= 48 && //'0'
				in[i+1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			} else { //just skip the ^
				i++;
				continue;
			}
		}
		if (in[i] < 32) {
			i++;
			continue;
		}
		out[r] = in[i];
		r++;
		i++;
	}
	out[r] = 0;
}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
void CG_DrawCrosshairNames(void) {
	float		*color;
	vec4_t		tcolor;
	char		*name;
	char		sanitized[1024];
	int			baseColor;
	qboolean	isVeh = qfalse;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}	
	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	//rww - still do the trace, our dynamic crosshair depends on it
	if (cg.crosshairClientNum < ENTITYNUM_WORLD) {
		centity_t *veh = &cg_entities[cg.crosshairClientNum];

		if (veh->currentState.eType == ET_NPC &&
			veh->currentState.NPC_class == CLASS_VEHICLE &&
			veh->currentState.owner < MAX_CLIENTS) {
			//draw the name of the pilot then
			cg.crosshairClientNum = veh->currentState.owner;
			cg.crosshairVehNum = veh->currentState.number;
			cg.crosshairVehTime = cg.time;
			isVeh = qtrue; //so we know we're drawing the pilot's name
		}
	}

	if (cg.crosshairClientNum >= MAX_CLIENTS) {
		return;
	}

	if (cg_entities[cg.crosshairClientNum].currentState.powerups & (1 << PW_CLOAKED)) {
		return;
	}

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].cleanname;

	if (cgs.gametype >= GT_TEAM) {
		//if (cgs.gametype == GT_SIEGE)
		if (1) {
		//instead of team-based we'll make it oriented based on which team we're on
			if (cgs.clientinfo[cg.crosshairClientNum].team == cgs.clientinfo[cg.playerCent->currentState.number].team) {
				baseColor = CT_GREEN;
			} else {
				baseColor = CT_RED;
			}
		} else {
			if (cgs.clientinfo[cg.crosshairClientNum].team == TEAM_RED) {
				baseColor = CT_RED;
			} else {
				baseColor = CT_BLUE;
			}
		}
	} else {
		//baseColor = CT_WHITE;
		if (cgs.gametype == GT_POWERDUEL &&
			cgs.clientinfo[cg.playerCent->currentState.number].team != TEAM_SPECTATOR &&
			cgs.clientinfo[cg.crosshairClientNum].duelTeam == cgs.clientinfo[cg.playerCent->currentState.number].duelTeam) {
		//on the same duel team in powerduel, so he's a friend
			baseColor = CT_GREEN;
		} else {
			baseColor = CT_RED; //just make it red in nonteam modes since everyone is hostile and crosshair will be red on them too
		}
	}

	if (cg.snap->ps.duelInProgress) {
		if (cg.crosshairClientNum != cg.snap->ps.duelIndex &&
			cg.crosshairClientNum != cg.snap->ps.clientNum) {
		  //grey out crosshair for everyone but your foe if you're in a duel
			baseColor = CT_BLACK;
		}
	} else if (cg_entities[cg.crosshairClientNum].currentState.bolt1) {
	  //this fellow is in a duel. We just checked if we were in a duel above, so
	  //this means we aren't and he is. Which of course means our crosshair greys out over him.
		baseColor = CT_BLACK;
	}

	tcolor[0] = colorTable[baseColor][0];
	tcolor[1] = colorTable[baseColor][1];
	tcolor[2] = colorTable[baseColor][2];
	tcolor[3] = color[3]*0.5f;

	CG_SanitizeString(name, sanitized);

	if (isVeh) {
		char str[MAX_STRING_CHARS];
		//mme, taken from pugmod
		Com_sprintf(str, MAX_STRING_CHARS, "%s (pilot)", sanitized);
		UI_DrawProportionalString(320, 170, str, UI_CENTER, tcolor);
	} else {
		//mme, taken from pugmod
		UI_DrawProportionalString(320, 170, sanitized, UI_CENTER, tcolor);
	}

	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) 
{	
	const char* s;

	s = CG_GetStringEdString("MP_INGAME", "SPECTATOR");
	if ((cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) &&
		cgs.duelist1 != -1 &&
		cgs.duelist2 != -1)
	{
		char text[1024];
		int size = 64;

		if (cgs.gametype == GT_POWERDUEL && cgs.duelist3 != -1)
		{
			Com_sprintf(text, sizeof(text), "%s^7 %s %s^7 %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStringEdString("MP_INGAME", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name, CG_GetStringEdString("MP_INGAME", "AND"), cgs.clientinfo[cgs.duelist3].name);
		}
		else
		{
			Com_sprintf(text, sizeof(text), "%s^7 %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStringEdString("MP_INGAME", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name);
		}
		CG_Text_Paint ( 320 - CG_Text_Width ( text, 1.0f, 3 ) / 2, 420, 1.0f, colorWhite, text, 0, 0, 0, 3 );

		trap_R_SetColor( colorTable[CT_WHITE] );
		if ( cgs.clientinfo[cgs.duelist1].modelIcon )
		{
			CG_DrawPic( 10, SCREEN_HEIGHT-(size*1.5), size, size, cgs.clientinfo[cgs.duelist1].modelIcon );
		}
		if ( cgs.clientinfo[cgs.duelist2].modelIcon )
		{
			CG_DrawPic( SCREEN_WIDTH-size-10, SCREEN_HEIGHT-(size*1.5), size, size, cgs.clientinfo[cgs.duelist2].modelIcon );
		}

// nmckenzie: DUEL_HEALTH
		if (cgs.gametype == GT_DUEL)
		{
			if ( cgs.showDuelHealths >= 1)
			{	// draw the healths on the two guys - how does this interact with power duel, though?
				CG_DrawDuelistHealth ( 10, SCREEN_HEIGHT-(size*1.5) - 12, 64, 8, 1 );
				CG_DrawDuelistHealth ( SCREEN_WIDTH-size-10, SCREEN_HEIGHT-(size*1.5) - 12, 64, 8, 2 );
			}
		}

		if (cgs.gametype != GT_POWERDUEL)
		{
			Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[cgs.duelist1].score, cgs.fraglimit );
			CG_Text_Paint( 42 - CG_Text_Width( text, 1.0f, 2 ) / 2, SCREEN_HEIGHT-(size*1.5) + 64, 1.0f, colorWhite, text, 0, 0, 0, 2 );

			Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[cgs.duelist2].score, cgs.fraglimit );
			CG_Text_Paint( SCREEN_WIDTH-size+22 - CG_Text_Width( text, 1.0f, 2 ) / 2, SCREEN_HEIGHT-(size*1.5) + 64, 1.0f, colorWhite, text, 0, 0, 0, 2 );
		}

		if (cgs.gametype == GT_POWERDUEL && cgs.duelist3 != -1)
		{
			if ( cgs.clientinfo[cgs.duelist3].modelIcon )
			{
				CG_DrawPic( SCREEN_WIDTH-size-10, SCREEN_HEIGHT-(size*2.8), size, size, cgs.clientinfo[cgs.duelist3].modelIcon );
			}
		}
	}
	else
	{
		CG_Text_Paint ( 320 - CG_Text_Width ( s, 1.0f, 3 ) / 2, 420, 1.0f, colorWhite, s, 0, 0, 0, 3 );
	}

	if ( cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL ) 
	{
		s = CG_GetStringEdString("MP_INGAME", "WAITING_TO_PLAY");	// "waiting to play";
		CG_Text_Paint ( 320 - CG_Text_Width ( s, 1.0f, 3 ) / 2, 440, 1.0f, colorWhite, s, 0, 0, 0, 3 );
	}
	else //if ( cgs.gametype >= GT_TEAM ) 
	{
		//s = "press ESC and use the JOIN menu to play";
		s = CG_GetStringEdString("MP_INGAME", "SPEC_CHOOSEJOIN");
		CG_Text_Paint ( 320 - CG_Text_Width ( s, 1.0f, 3 ) / 2, 440, 1.0f, colorWhite, s, 0, 0, 0, 3 );
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	const char *s = NULL, *sParm = NULL;
	int sec;
	char sYes[20] = {0}, sNo[20] = {0}, sVote[20] = {0}, sCmd[100] = {0};

	if ( !cgs.voteTime )
		return;

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}

	if (strncmp(cgs.voteString, "map_restart", 11)==0)
	{
		trap_SP_GetStringTextString("MENUS_RESTART_MAP", sCmd, sizeof(sCmd) );
	}
	else if (strncmp(cgs.voteString, "vstr nextmap", 12)==0)
	{
		trap_SP_GetStringTextString("MENUS_NEXT_MAP", sCmd, sizeof(sCmd) );
	}
	else if (strncmp(cgs.voteString, "g_doWarmup", 10)==0)
	{
		trap_SP_GetStringTextString("MENUS_WARMUP", sCmd, sizeof(sCmd) );
	}
	else if (strncmp(cgs.voteString, "g_gametype", 10)==0)
	{
		trap_SP_GetStringTextString("MENUS_GAME_TYPE", sCmd, sizeof(sCmd) );
		if      ( stricmp("Free For All", cgs.voteString+11)==0 ) 
		{
			sParm = CG_GetStringEdString("MENUS", "FREE_FOR_ALL");
		}
		else if ( stricmp("Duel", cgs.voteString+11)==0 ) 
		{
			sParm = CG_GetStringEdString("MENUS", "DUEL");
		}
		else if ( stricmp("Holocron FFA", cgs.voteString+11)==0  ) 
		{
			sParm = CG_GetStringEdString("MENUS", "HOLOCRON_FFA");
		}
		else if ( stricmp("Power Duel", cgs.voteString+11)==0  ) 
		{
			sParm = CG_GetStringEdString("MENUS", "POWERDUEL");
		}
		else if ( stricmp("Team FFA", cgs.voteString+11)==0  ) 
		{
			sParm = CG_GetStringEdString("MENUS", "TEAM_FFA");
		}
		else if ( stricmp("Siege", cgs.voteString+11)==0  ) 
		{
			sParm = CG_GetStringEdString("MENUS", "SIEGE");
		}
		else if ( stricmp("Capture the Flag", cgs.voteString+11)==0  ) 
		{
			sParm = CG_GetStringEdString("MENUS", "CAPTURE_THE_FLAG");
		}
		else if ( stricmp("Capture the Ysalamiri", cgs.voteString+11)==0  ) 
		{
			sParm = CG_GetStringEdString("MENUS", "CAPTURE_THE_YSALIMARI");
		} 
	}
	else if (strncmp(cgs.voteString, "map", 3)==0)
	{
		trap_SP_GetStringTextString("MENUS_NEW_MAP", sCmd, sizeof(sCmd) );
		sParm = cgs.voteString+4;
	}
	else if (strncmp(cgs.voteString, "kick", 4)==0)
	{
		trap_SP_GetStringTextString("MENUS_KICK_PLAYER", sCmd, sizeof(sCmd) );
		sParm = cgs.voteString+5;
	}
	else
	{//Raz: Added an else case for custom votes like ampoll, cointoss, etc
		sParm = cgs.voteString;
	}



	trap_SP_GetStringTextString("MENUS_VOTE", sVote, sizeof(sVote) );
	trap_SP_GetStringTextString("MENUS_YES", sYes, sizeof(sYes) );
	trap_SP_GetStringTextString("MENUS_NO",  sNo,  sizeof(sNo) );

	if (sParm && sParm[0])
	{
		s = va("%s(%i):<%s %s> %s:%i %s:%i", sVote, sec, sCmd, sParm, sYes, cgs.voteYes, sNo, cgs.voteNo);
	}
	else
	{
		s = va("%s(%i):<%s> %s:%i %s:%i",    sVote, sec, sCmd,        sYes, cgs.voteYes, sNo, cgs.voteNo);
	}
	CG_DrawStringExt( 4.0f*cgs.widthRatioCoef, 58.0f, s, colorWhite, qfalse, qtrue, SMALLCHAR_WIDTH*cgs.widthRatioCoef, SMALLCHAR_HEIGHT, 0 );
	s = CG_GetStringEdString("MP_INGAME", "OR_PRESS_ESC_THEN_CLICK_VOTE");	//	s = "or press ESC then click Vote";
	CG_DrawStringExt( 4.0f*cgs.widthRatioCoef, 58.0f + SMALLCHAR_HEIGHT + 2.0f, s, colorWhite, qfalse, qtrue, SMALLCHAR_WIDTH*cgs.widthRatioCoef, SMALLCHAR_HEIGHT, 0 );
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
//		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	if (strstr(cgs.teamVoteString[cs_offset], "leader"))
	{
		int i = 0;

		while (cgs.teamVoteString[cs_offset][i] && cgs.teamVoteString[cs_offset][i] != ' ')
		{
			i++;
		}

		if (cgs.teamVoteString[cs_offset][i] == ' ')
		{
			int voteIndex = 0;
			char voteIndexStr[256];

			i++;

			while (cgs.teamVoteString[cs_offset][i])
			{
				voteIndexStr[voteIndex] = cgs.teamVoteString[cs_offset][i];
				voteIndex++;
				i++;
			}
			voteIndexStr[voteIndex] = 0;

			voteIndex = atoi(voteIndexStr);

			s = va("TEAMVOTE(%i):(Make %s the new team leader) yes:%i no:%i", sec, cgs.clientinfo[voteIndex].name,
									cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
		}
		else
		{
			s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
									cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
		}
	}
	else
	{
		s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
								cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	}
	CG_DrawStringExt( 4.0f*cgs.widthRatioCoef, 90.0f, s, colorWhite, qfalse, qtrue, SMALLCHAR_WIDTH*cgs.widthRatioCoef, SMALLCHAR_HEIGHT, 0 );
}

static qboolean CG_DrawScoreboard() {
	return CG_DrawOldScoreboard();
#if 0
	static qboolean firstTime = qtrue;
	float fade, *fadeColor;

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			return qfalse;
		}
		fade = *fadeColor;
	}																					  


	if (menuScoreboard == NULL) {
		if ( cgs.gametype >= GT_TEAM ) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard) {
		if (firstTime) {
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
#endif
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

void IntegerToRaceName(int style, char *styleString, size_t styleStringSize)
{ //should be changed to return const char *
    qboolean coop = (qboolean)style >= MV_COOP_JKA;

    if (coop && styleStringSize >= 64) {
        style -= (MV_COOP_JKA - 1);
    }

    switch (style)
    {
        case MV_SIEGE:		Q_strncpyz(styleString, "siege",styleStringSize);		break;
        case MV_JKA:		Q_strncpyz(styleString, "jka", styleStringSize);		break;
        case MV_QW:			Q_strncpyz(styleString, "qw", styleStringSize);			break;
        case MV_CPM:		Q_strncpyz(styleString, "cpm", styleStringSize);		break;
        case MV_Q3:			Q_strncpyz(styleString, "q3", styleStringSize);			break;
        case MV_PJK:		Q_strncpyz(styleString, "pjk", styleStringSize);		break;
        case MV_WSW:		Q_strncpyz(styleString, "wsw", styleStringSize);		break;
        case MV_RJQ3:		Q_strncpyz(styleString, "rjq3", styleStringSize);		break;
        case MV_RJCPM:		Q_strncpyz(styleString, "rjcpm", styleStringSize);		break;
        case MV_SWOOP:		Q_strncpyz(styleString, "swoop", styleStringSize);		break;
        case MV_JETPACK:	Q_strncpyz(styleString, "jetpack", styleStringSize);	break;
        case MV_SPEED:		Q_strncpyz(styleString, "speed", styleStringSize);		break;
        case MV_SP:			Q_strncpyz(styleString, "sp", styleStringSize);			break;
        case MV_SLICK:		Q_strncpyz(styleString, "slick", styleStringSize);		break;
        case MV_BOTCPM:		Q_strncpyz(styleString, "botcpm", styleStringSize);		break;
        default:			Q_strncpyz(styleString, "ERROR", styleStringSize);		return;
    }

    if (coop)
    {
        if (cg.predictedPlayerState.duelInProgress) {//lets draw our partner's name too
            clientInfo_t *partner = &cgs.clientinfo[cg.predictedPlayerState.duelIndex];

            if (!partner || !partner->infoValid || !VALIDSTRING(partner->name)) {
                Q_strcat(styleString, styleStringSize, " (co-op)");
                return;
            }

            Q_strcat(styleString, styleStringSize, va(" (co-op: %s" S_COLOR_WHITE ")", partner->name));
        }
        else {
            Q_strcat(styleString, styleStringSize, " (co-op)");
        }
    }
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow(void) {
	char *s;
	if (!(cg.snap->ps.pm_flags & PMF_FOLLOW) && !cg.demoPlayback) {
		return qfalse;
	}
//	s = "following";
	if (cgs.gametype == GT_POWERDUEL) {
		clientInfo_t *ci = &cgs.clientinfo[ cg.snap->ps.clientNum ];

		if (ci->duelTeam == DUELTEAM_LONE) {
			s = CG_GetStringEdString("MP_INGAME", "FOLLOWINGLONE");
		} else if (ci->duelTeam == DUELTEAM_DOUBLE) {
			s = CG_GetStringEdString("MP_INGAME", "FOLLOWINGDOUBLE");
		} else {
			s = CG_GetStringEdString("MP_INGAME", "FOLLOWING");
		}
	} else {
		s = CG_GetStringEdString("MP_INGAME", "FOLLOWING");
	}
/*
	CG_Text_Paint (320 - CG_Text_Width (s, 1.0f, FONT_MEDIUM)/2, 60, 1.0f, colorWhite, s, 0, 0, 0, FONT_MEDIUM);

	s = cgs.clientinfo[cg.snap->ps.clientNum].name;
	CG_Text_Paint(320 - CG_Text_Width (s, 2.0f, FONT_MEDIUM)/2, 80, 2.0f, colorWhite, s, 0, 0, 0, FONT_MEDIUM);
*/
	strcat(s, va(": %s", cgs.clientinfo[cg.playerCent->currentState.clientNum].name));
	CG_Text_Paint(320 - CG_Text_Width (s, 0.7f, FONT_LARGE)/2, 1, 0.7f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_LARGE);
	return qtrue;
}

#if 0
static void CG_DrawTemporaryStats()
{ //placeholder for testing (draws ammo and force power)
	char s[512];

	if (!cg.snap)
	{
		return;
	}

	sprintf(s, "Force: %i", cg.snap->ps.fd.forcePower);

	CG_DrawBigString(SCREEN_WIDTH-164, SCREEN_HEIGHT-dmgIndicSize, s, 1.0f);

	sprintf(s, "Ammo: %i", cg.snap->ps.ammo[weaponData[cg.snap->ps.weapon].ammoIndex]);

	CG_DrawBigString(SCREEN_WIDTH-164, SCREEN_HEIGHT-112, s, 1.0f);

	sprintf(s, "Health: %i", cg.snap->ps.stats[STAT_HEALTH]);

	CG_DrawBigString(8, SCREEN_HEIGHT-dmgIndicSize, s, 1.0f);

	sprintf(s, "Armor: %i", cg.snap->ps.stats[STAT_ARMOR]);

	CG_DrawBigString(8, SCREEN_HEIGHT-112, s, 1.0f);
}
#endif

/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
#if 0
	const char	*s;
	int			w;

	if (!cg_drawStatus.integer)
	{
		return;
	}

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
#endif
}



/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int			w;
	int			sec;
	int			i;
	float scale;
	int			cw;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
//		s = "Waiting for players";		
		s = CG_GetStringEdString("MP_INGAME", "WAITING_FOR_PLAYERS");
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 24, s, 1.0F);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
	{
		// find the two active players
		clientInfo_t	*ci1, *ci2, *ci3;

		ci1 = NULL;
		ci2 = NULL;
		ci3 = NULL;

		if (cgs.gametype == GT_POWERDUEL)
		{
			if (cgs.duelist1 != -1)
			{
				ci1 = &cgs.clientinfo[cgs.duelist1];
			}
			if (cgs.duelist2 != -1)
			{
				ci2 = &cgs.clientinfo[cgs.duelist2];
			}
			if (cgs.duelist3 != -1)
			{
				ci3 = &cgs.clientinfo[cgs.duelist3];
			}
		}
		else
		{
			for ( i = 0 ; i < cgs.maxclients ; i++ ) {
				if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
					if ( !ci1 ) {
						ci1 = &cgs.clientinfo[i];
					} else {
						ci2 = &cgs.clientinfo[i];
					}
				}
			}
		}
		if ( ci1 && ci2 )
		{
			if (ci3)
			{
				s = va( "%s vs %s and %s", ci1->name, ci2->name, ci3->name );
			}
			else
			{
				s = va( "%s vs %s", ci1->name, ci2->name );
			}
			w = CG_Text_Width(s, 0.6f, FONT_MEDIUM);
			CG_Text_Paint(320 - w / 2, 60, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE,FONT_MEDIUM);
		}
	} else {
		if ( cgs.gametype == GT_FFA ) {
			s = CG_GetStringEdString("MENUS", "FREE_FOR_ALL");//"Free For All";
		} else if ( cgs.gametype == GT_HOLOCRON ) {
			s = CG_GetStringEdString("MENUS", "HOLOCRON_FFA");//"Holocron FFA";
		} else if ( cgs.gametype == GT_JEDIMASTER ) {
			s = CG_GetStringEdString("MENUS", "POWERDUEL");//"Jedi Master";??
		} else if ( cgs.gametype == GT_TEAM ) {
			s = CG_GetStringEdString("MENUS", "TEAM_FFA");//"Team FFA";
		} else if ( cgs.gametype == GT_SIEGE ) {
			s = CG_GetStringEdString("MENUS", "SIEGE");//"Siege";
		} else if ( cgs.gametype == GT_CTF ) {
			s = CG_GetStringEdString("MENUS", "CAPTURE_THE_FLAG");//"Capture the Flag";
		} else if ( cgs.gametype == GT_CTY ) {
			s = CG_GetStringEdString("MENUS", "CAPTURE_THE_YSALIMARI");//"Capture the Ysalamiri";
		} else {
			s = "";
		}
		w = CG_Text_Width(s, 1.5f, FONT_MEDIUM);
		CG_Text_Paint(320 - w / 2, 90, 1.5f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE,FONT_MEDIUM);
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
//	s = va( "Starts in: %i", sec + 1 );
	s = va( "%s: %i",CG_GetStringEdString("MP_INGAME", "STARTS_IN"), sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;

		if (!(mov_soundDisable.integer & SDISABLE_ANNOUNCER) && cgs.gametype != GT_SIEGE)
		{
			switch ( sec ) {
			case 0:
				trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
				break;
			case 1:
				trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
				break;
			case 2:
				trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
				break;
			default:
				break;
			}
		}
	}
	scale = 0.45f;
	switch ( cg.warmupCount ) {
	case 0:
		cw = 28;
		scale = 1.25f;
		break;
	case 1:
		cw = 24;
		scale = 1.15f;
		break;
	case 2:
		cw = 20;
		scale = 1.05f;
		break;
	default:
		cw = 16;
		scale = 0.9f;
		break;
	}

	w = CG_Text_Width(s, scale, FONT_MEDIUM);
	CG_Text_Paint(320 - w / 2, 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
}

//==================================================================================
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus() {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}

static qboolean CG_YourFlagDropped(void) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_CTY) {
		int team = cgs.clientinfo[cg.playerCent->currentState.number].team;
		if (team == TEAM_RED && cgs.redflag == FLAG_DROPPED) {
			return qtrue;
		} else if (team == TEAM_BLUE && cgs.blueflag == FLAG_DROPPED) {
			return qtrue;
		} else {
			return qfalse;
		}
	}
	return qfalse;
}

static qboolean CG_OtherFlagDropped(void) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_CTY) {
		int team = cgs.clientinfo[cg.playerCent->currentState.number].team;
		if (team == TEAM_RED && cgs.blueflag == FLAG_DROPPED) {
			return qtrue;
		} else if (team == TEAM_BLUE && cgs.redflag == FLAG_DROPPED) {
			return qtrue;
		} else {
			return qfalse;
		}
	}
	return qfalse;
}

void CG_DrawFlagStatus() {
	int myFlagTakenShader = 0;
	int theirFlagShader = 0;
	int team = 0;
	int startDrawPos = 2;
	int ico_size = 32;

	//Raz: was missing this
	trap_R_SetColor( NULL );

	if (!cg.snap)
		return;
	
	if (cgs.gametype != GT_CTF && cgs.gametype != GT_CTY)
		return;
	
	team = cgs.clientinfo[cg.playerCent->currentState.number].team;
	if (cgs.gametype == GT_CTY) {
		if (team == TEAM_RED) {
			myFlagTakenShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_x" );
			theirFlagShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_ys" );
		} else {
			myFlagTakenShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_x" );
			theirFlagShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_ys" );
		}
	} else {
		if (team == TEAM_RED) {
			myFlagTakenShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_x" );
			theirFlagShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag" );
		} else {
			myFlagTakenShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_x" );
			theirFlagShader = trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag" );
		}
	}

	if (CG_YourTeamHasFlag()) {
		CG_DrawPic( 7.0f*cgs.widthRatioCoef, 330-startDrawPos, ico_size*cgs.widthRatioCoef, ico_size, theirFlagShader );
		startDrawPos += ico_size+2;
	} else if (CG_OtherFlagDropped()) {
		vec4_t c = {1.0f, 1.0f, 1.0f, 0.5f};
		trap_R_SetColor(c);
		CG_DrawPic( 7.0f*cgs.widthRatioCoef, 330-startDrawPos, ico_size*cgs.widthRatioCoef, ico_size, theirFlagShader );
		startDrawPos += ico_size+2;
		trap_R_SetColor( NULL );
	}

	if (CG_OtherTeamHasFlag()) {
		CG_DrawPic( 7.0f*cgs.widthRatioCoef, 330-startDrawPos, ico_size*cgs.widthRatioCoef, ico_size, myFlagTakenShader );
	} else if (CG_YourFlagDropped()) {
		vec4_t c = {1.0f, 1.0f, 1.0f, 0.5f};
		trap_R_SetColor(c);
		CG_DrawPic( 7.0f*cgs.widthRatioCoef, 330-startDrawPos, ico_size*cgs.widthRatioCoef, ico_size, myFlagTakenShader );
		trap_R_SetColor( NULL );
	}
}

//draw meter showing jetpack fuel when it's not full
#define JPFUELBAR_H			100.0f
#define JPFUELBAR_W			20.0f
#define JPFUELBAR_X			(SCREEN_WIDTH-JPFUELBAR_W-8.0f)
#define JPFUELBAR_Y			260.0f
void CG_DrawJetpackFuel(void)
{
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x = SCREEN_WIDTH - (SCREEN_WIDTH - JPFUELBAR_X)*cgs.widthRatioCoef;
	float y = JPFUELBAR_Y;
	float percent = ((float)cg.snap->ps.jetpackFuel/100.0f)*JPFUELBAR_H;

	if (percent > JPFUELBAR_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar
	aColor[0] = 0.5f;
	aColor[1] = 0.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.8f;

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, JPFUELBAR_W*cgs.widthRatioCoef, JPFUELBAR_H, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f*cgs.widthRatioCoef, y+1.0f+(JPFUELBAR_H-percent), (JPFUELBAR_W-1.0f)*cgs.widthRatioCoef, JPFUELBAR_H-1.0f-(JPFUELBAR_H-percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f*cgs.widthRatioCoef, y+1.0f, (JPFUELBAR_W-1.0f)*cgs.widthRatioCoef, JPFUELBAR_H-percent, cColor);
}

//draw meter showing e-web health when it is in use
#define EWEBHEALTH_H			100.0f
#define EWEBHEALTH_W			20.0f
#define EWEBHEALTH_X			(SCREEN_WIDTH-EWEBHEALTH_W-8.0f)
#define EWEBHEALTH_Y			290.0f
void CG_DrawEWebHealth(void)
{
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x = SCREEN_WIDTH - (SCREEN_WIDTH - EWEBHEALTH_X)*cgs.widthRatioCoef;
	float y = EWEBHEALTH_Y;
	centity_t *eweb = &cg_entities[cg.predictedPlayerState.emplacedIndex];
	float percent = ((float)eweb->currentState.health/eweb->currentState.maxhealth)*EWEBHEALTH_H;

	if (percent > EWEBHEALTH_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//kind of hacky, need to pass a coordinate in here
	if (cg.snap->ps.jetpackFuel < 100)
	{
		x -= (JPFUELBAR_W+8.0f)*cgs.widthRatioCoef;
	}
	if (cg.snap->ps.cloakFuel < 100)
	{
		x -= (JPFUELBAR_W+8.0f)*cgs.widthRatioCoef;
	}

	//color of the bar
	aColor[0] = 0.5f;
	aColor[1] = 0.0f;
	aColor[2] = 0.0f;
	aColor[3] = 0.8f;

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.5f;
	cColor[1] = 0.5f;
	cColor[2] = 0.5f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, EWEBHEALTH_W*cgs.widthRatioCoef, EWEBHEALTH_H, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f*cgs.widthRatioCoef, y+1.0f+(EWEBHEALTH_H-percent), (EWEBHEALTH_W-1.0f)*cgs.widthRatioCoef, EWEBHEALTH_H-1.0f-(EWEBHEALTH_H-percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f*cgs.widthRatioCoef, y+1.0f, (EWEBHEALTH_W-1.0f)*cgs.widthRatioCoef, EWEBHEALTH_H-percent, cColor);
}

//draw meter showing cloak fuel when it's not full
#define CLFUELBAR_H			100.0f
#define CLFUELBAR_W			20.0f
#define CLFUELBAR_X			(SCREEN_WIDTH-CLFUELBAR_W-8.0f)
#define CLFUELBAR_Y			260.0f
void CG_DrawCloakFuel(void)
{
	vec4_t aColor;
	vec4_t bColor;
	vec4_t cColor;
	float x = SCREEN_WIDTH - (SCREEN_WIDTH - CLFUELBAR_X)*cgs.widthRatioCoef;
	float y = CLFUELBAR_Y;
	float percent = ((float)cg.snap->ps.cloakFuel/100.0f)*CLFUELBAR_H;

	if (percent > CLFUELBAR_H)
	{
		return;
	}

	if ( cg.snap->ps.jetpackFuel < 100 )
	{//if drawing jetpack fuel bar too, then move this over...?
		x -= (JPFUELBAR_W+8.0f)*cgs.widthRatioCoef;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar
	aColor[0] = 0.0f;
	aColor[1] = 0.0f;
	aColor[2] = 0.6f;
	aColor[3] = 0.8f;

	//color of the border
	bColor[0] = 0.0f;
	bColor[1] = 0.0f;
	bColor[2] = 0.0f;
	bColor[3] = 0.3f;

	//color of greyed out "missing fuel"
	cColor[0] = 0.1f;
	cColor[1] = 0.1f;
	cColor[2] = 0.3f;
	cColor[3] = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, CLFUELBAR_W*cgs.widthRatioCoef, CLFUELBAR_H, 1.0f, colorTable[CT_BLACK]);

	//now draw the part to show how much fuel there is in the color specified
	CG_FillRect(x+1.0f*cgs.widthRatioCoef, y+1.0f+(CLFUELBAR_H-percent), (CLFUELBAR_W-1.0f)*cgs.widthRatioCoef, CLFUELBAR_H-1.0f-(CLFUELBAR_H-percent), aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f*cgs.widthRatioCoef, y+1.0f, (CLFUELBAR_W-1.0f)*cgs.widthRatioCoef, CLFUELBAR_H-percent, cColor);
}

extern int cgSiegeRoundState;
extern int cgSiegeRoundTime;

extern int team1Timed;
extern int team2Timed;

int cg_beatingSiegeTime = 0;

int cgSiegeRoundBeganTime = 0;
int cgSiegeRoundCountTime = 0;

static void CG_DrawSiegeTimer(int timeRemaining, qboolean isMyTeam)
{ //rwwFIXMEFIXME: Make someone make assets and use them.
  //this function is pretty much totally placeholder.
//	int x = 0;
//	int y = SCREEN_HEIGHT-160;
	int fColor = 0;
	int minutes = 0;
	int seconds = 0;
	char timeStr[1024];
	menuDef_t	*menuHUD = NULL;
	itemDef_t	*item = NULL;

	menuHUD = Menus_FindByName("mp_timer");
	if (!menuHUD)
	{
		return;
	}

	item = Menu_FindItemByName(menuHUD, "frame");

	if (item)
	{
		trap_R_SetColor( item->window.foreColor );
		CG_DrawPic( 
			SCREEN_WIDTH - (SCREEN_WIDTH - item->window.rect.x)*cgs.widthRatioCoef, 
			item->window.rect.y, 
			item->window.rect.w*cgs.widthRatioCoef, 
			item->window.rect.h, 
			item->window.background );
	}

	seconds = timeRemaining;

	while (seconds >= 60)
	{
		minutes++;
		seconds -= 60;
	}

	strcpy(timeStr, va( "%i:%02i", minutes, seconds ));

	if (isMyTeam)
	{
		fColor = CT_HUD_RED;
	}
	else
	{
		fColor = CT_HUD_GREEN;
	}

//	trap_Cvar_Set("ui_siegeTimer", timeStr);

//	UI_DrawProportionalString( x+16, y+40, timeStr,
//		UI_SMALLFONT|UI_DROPSHADOW, colorTable[fColor] );

	item = Menu_FindItemByName(menuHUD, "timer");
	if (item)
	{
		UI_DrawProportionalString( 
			SCREEN_WIDTH - (SCREEN_WIDTH - item->window.rect.x)*cgs.widthRatioCoef, 
			item->window.rect.y, 
			timeStr,
			UI_SMALLFONT|UI_DROPSHADOW, 
			colorTable[fColor] );
	}

}

static void CG_DrawSiegeDeathTimer( int timeRemaining )
{
	int minutes = 0;
	int seconds = 0;
	char timeStr[1024];
	menuDef_t	*menuHUD = NULL;
	itemDef_t	*item = NULL;

	menuHUD = Menus_FindByName("mp_timer");
	if (!menuHUD)
	{
		return;
	}

	item = Menu_FindItemByName(menuHUD, "frame");

	if (item)
	{
		trap_R_SetColor( item->window.foreColor );
		CG_DrawPic( 
			SCREEN_WIDTH - (SCREEN_WIDTH - item->window.rect.x)*cgs.widthRatioCoef, 
			item->window.rect.y, 
			item->window.rect.w*cgs.widthRatioCoef, 
			item->window.rect.h, 
			item->window.background );
	}

	seconds = timeRemaining;

	while (seconds >= 60)
	{
		minutes++;
		seconds -= 60;
	}

	if (seconds < 10)
	{
		strcpy(timeStr, va( "%i:0%i", minutes, seconds ));
	}
	else
	{
		strcpy(timeStr, va( "%i:%i", minutes, seconds ));
	}

	item = Menu_FindItemByName(menuHUD, "deathtimer");
	if (item)
	{
		UI_DrawProportionalString( 
			SCREEN_WIDTH - (SCREEN_WIDTH - item->window.rect.x)*cgs.widthRatioCoef, 
			item->window.rect.y, 
			timeStr,
			UI_SMALLFONT|UI_DROPSHADOW, 
			item->window.foreColor );
	}
	
}

int cgSiegeEntityRender = 0;

static void CG_DrawSiegeHUDItem(void)
{
	void *g2;
	qhandle_t handle;
	vec3_t origin, angles;
	vec3_t mins, maxs;
	float len;
	centity_t *cent = &cg_entities[cgSiegeEntityRender];

	if (cent->ghoul2)
	{
		g2 = cent->ghoul2;
		handle = 0;
	}
	else
	{
		handle = cgs.gameModels[cent->currentState.modelindex];
		g2 = NULL;
	}

	if (handle)
	{
		trap_R_ModelBounds( handle, mins, maxs );
	}
	else
	{
		VectorSet(mins, -16, -16, -20);
		VectorSet(maxs, 16, 16, 32);
	}

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );
	len = 0.5 * ( maxs[2] - mins[2] );		
	origin[0] = len / 0.268;

	VectorClear(angles);
	angles[YAW] = cg.autoAngles[YAW];

	CG_Draw3DModel( 8, 8, 64, 64, handle, g2, cent->currentState.g2radius, 0, origin, angles );

	cgSiegeEntityRender = 0; //reset for next frame
}

/*====================================
chatbox functionality -rww
====================================*/
#define	CHATBOX_CUTOFF_LEN	550
#define	CHATBOX_FONT_HEIGHT	20

//utility func, insert a string into a string at the specified
//place (assuming this will not overflow the buffer)
static void CG_ChatBox_StrInsert(char *buffer, int place, char *str) {
	int insLen = strlen(str);
	int i = strlen(buffer);
	int k = 0;

	buffer[i+insLen+1] = 0; //terminate the string at its new length
	while (i >= place) {
		buffer[i+insLen] = buffer[i];
		i--;
	}

	i++;
	while (k < insLen) {
		buffer[i] = str[k];
		i++;
		k++;
	}
}

//add chatbox string
void CG_ChatBox_AddString(char *chatStr) {
	chatBoxItem_t *chat = &cg.chatItems[cg.chatItemActive];
	float chatLen;

	if (cg_chatBox.integer<=0) {
	//don't bother then.
		return;
	}

	memset(chat, 0, sizeof(chatBoxItem_t));

	if (strlen(chatStr) > sizeof(chat->string)) {
	//too long, terminate at proper len.
		chatStr[sizeof(chat->string)-1] = 0;
	}

	strcpy(chat->string, chatStr);
	chat->time = cg.time + cg_chatBox.integer;

	chat->lines = 1;

	chatLen = CG_Text_Width(chat->string, 1.0f, FONT_SMALL);
	if (chatLen > CHATBOX_CUTOFF_LEN) {
	//we have to break it into segments...
        int i = 0;
		int lastLinePt = 0;
		char s[2];

		chatLen = 0;
		while (chat->string[i]) {
			s[0] = chat->string[i];
			s[1] = 0;
			chatLen += CG_Text_Width(s, 0.65f, FONT_SMALL);

			if (chatLen >= CHATBOX_CUTOFF_LEN) {
				int j = i;
				while (j > 0 && j > lastLinePt) {
					if (chat->string[j] == ' ') {
						break;
					}
					j--;
				}
				if (chat->string[j] == ' ') {
					i = j;
				}

                chat->lines++;
				CG_ChatBox_StrInsert(chat->string, i, "\n");
				i++;
				chatLen = 0;
				lastLinePt = i+1;
			}
			i++;
		}
	}

	cg.chatItemActive++;
	if (cg.chatItemActive >= MAX_CHATBOX_ITEMS) {
		cg.chatItemActive = 0;
	}
}

//insert item into array (rearranging the array if necessary)
void CG_ChatBox_ArrayInsert(chatBoxItem_t **array, int insPoint, int maxNum, chatBoxItem_t *item) {
    if (array[insPoint]) { //recursively call, to move everything up to the top
		if (insPoint+1 >= maxNum) {
			CG_Error("CG_ChatBox_ArrayInsert: Exceeded array size");
		}
		CG_ChatBox_ArrayInsert(array, insPoint+1, maxNum, array[insPoint]);
	}
	//now that we have moved anything that would be in this slot up, insert what we want into the slot
	array[insPoint] = item;
}

//go through all the chat strings and draw them if they are not yet expired
static CGAME_INLINE void CG_ChatBox_DrawStrings(void) {
	chatBoxItem_t *drawThese[MAX_CHATBOX_ITEMS];
	int numToDraw = 0;
	int linesToDraw = 0;
	int i = 0;
	float x = 44.0f*cgs.widthRatioCoef;
	float y = cg.scoreBoardShowing ? 475 : cg_chatBoxHeight.integer;
	float fontScale = 0.65f;

	if (!cg_chatBox.integer) {
		return;
	}
//	if (cg.chatItems->time > cg_chatBox.integer + cg.time) {
//		cg.chatItems->time = cg.time;
//		return;
//	}
	memset(drawThese, 0, sizeof(drawThese));

	while (i < MAX_CHATBOX_ITEMS) {
		if (cg.chatItems[i].time >= cg.time) {
			int check = numToDraw;
			int insertionPoint = numToDraw;
			while (check >= 0) {
				if (drawThese[check] && cg.chatItems[i].time < drawThese[check]->time) {
				//insert here
					insertionPoint = check;
				}
				check--;
			}
			CG_ChatBox_ArrayInsert(drawThese, insertionPoint, MAX_CHATBOX_ITEMS, &cg.chatItems[i]);
			numToDraw++;
			linesToDraw += cg.chatItems[i].lines;
		}
		i++;
	}

	if (!numToDraw) { //nothing, then, just get out of here now.
		return;
	}

	//move initial point up so we draw bottom-up (visually)
	y -= (CHATBOX_FONT_HEIGHT*fontScale)*linesToDraw;

	//we have the items we want to draw, just quickly loop through them now
	i = 0;
	while (i < numToDraw) {
		CG_Text_Paint(x, y, fontScale, colorWhite, drawThese[i]->string, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		y += ((CHATBOX_FONT_HEIGHT*fontScale)*drawThese[i]->lines);
		i++;
	}
	trap_R_SetColor( NULL );
}

int cgRageTime = 0;
int cgRageFadeTime = 0;
float cgRageFadeVal = 0;

int cgRageRecTime = 0;
int cgRageRecFadeTime = 0;
float cgRageRecFadeVal = 0;

int cgAbsorbTime = 0;
int cgAbsorbFadeTime = 0;
float cgAbsorbFadeVal = 0;

int cgProtectTime = 0;
int cgProtectFadeTime = 0;
float cgProtectFadeVal = 0;

int cgYsalTime = 0;
int cgYsalFadeTime = 0;
float cgYsalFadeVal = 0;

void CG_Clear2DTintsTimes(void) {
	cgRageTime = 0;
	cgRageFadeTime = 0;
	cgRageFadeVal = 0;

	cgRageRecTime = 0;
	cgRageRecFadeTime = 0;
	cgRageRecFadeVal = 0;

	cgAbsorbTime = 0;
	cgAbsorbFadeTime = 0;
	cgAbsorbFadeVal = 0;

	cgProtectTime = 0;
	cgProtectFadeTime = 0;
	cgProtectFadeVal = 0;

	cgYsalTime = 0;
	cgYsalFadeTime = 0;
	cgYsalFadeVal = 0;
}

static void CG_DrawInWaterTints(void) {
	if ((cg.refdef.viewContents&CONTENTS_LAVA)) {
	//tint screen red
		vec4_t hcolor;
		float phase = cg.time / 1000.0f * WAVE_FREQUENCY * M_PI * 2.0f;
		hcolor[3] = 0.5 + (0.15f*sin(phase + (cg.timeFraction / 1000.0 * WAVE_FREQUENCY * M_PI * 2)));
		hcolor[0] = 0.7f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
	} else if ((cg.refdef.viewContents&CONTENTS_SLIME)) {
	//tint screen green
		vec4_t hcolor;
		float phase = cg.time / 1000.0f * WAVE_FREQUENCY * M_PI * 2.0f;
		hcolor[3] = 0.4 + (0.1f*sin(phase + (cg.timeFraction / 1000.0 * WAVE_FREQUENCY * M_PI * 2)));
		hcolor[0] = 0.0f;
		hcolor[1] = 0.7f;
		hcolor[2] = 0.0f;
		CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
	} else if ((cg.refdef.viewContents&CONTENTS_WATER)) {
	//tint screen light blue -- FIXME: don't do this if CONTENTS_FOG? (in case someone *does* make a water shader with fog in it?)
		vec4_t hcolor;
		float phase = cg.time / 1000.0f * WAVE_FREQUENCY * M_PI * 2.0f;
		hcolor[3] = 0.3f + (0.05f*(float)sin(phase + (cg.timeFraction / 1000.0 * WAVE_FREQUENCY * M_PI * 2)));
		hcolor[0] = 0.0f;
		hcolor[1] = 0.2f;
		hcolor[2] = 0.8f;
		//better underwater colour
//		hcolor[3] = 0.2f + (0.05f*(float)sin(phase + (cg.timeFraction / 1000.0 * WAVE_FREQUENCY * M_PI * 2)));
//		hcolor[0] = 0.1f;
//		hcolor[1] = 0.4f;
//		hcolor[2] = 0.7f;
		CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
	}
}

static qboolean CG_HasYsalamiri(int gametype, entityState_t *es) {
	if (gametype == GT_CTY && ((es->powerups & (1 << PW_REDFLAG)) || (es->powerups & (1 << PW_BLUEFLAG)))) {
		return qtrue;
	}
	if (es->powerups & (1 << PW_YSALAMIRI)) {
		return qtrue;
	}
	return qfalse;
}

static void CG_Draw2DScreenTints(void) {
	float	rageTime, rageRecTime, absorbTime, protectTime, ysalTime;
	vec4_t	hcolor;
	int		forcePowersActive = cg.playerPredicted ?
								cg.snap->ps.fd.forcePowersActive :
								cg.playerCent->currentState.forcePowersActive;

	if (cgs.clientinfo[cg.playerCent->currentState.clientNum].team != TEAM_SPECTATOR) {
		if (forcePowersActive & (1 << FP_RAGE)) {
			if (!cgRageTime)
				cgRageTime = cg.time;			
			rageTime = (cg.time - cgRageTime) + cg.timeFraction;			
			rageTime /= 9000.0f;
			
			if (rageTime < 0.0f)
				rageTime = 0.0f;
			else if (rageTime > 0.15f)
				rageTime = 0.15f;
			
			if (mov_rageColour.string[0] == '0') {
				hcolor[0] = 0.7f;
				hcolor[1] = 0.0f;
				hcolor[2] = 0.0f;
			} else {
				Q_parseColor(mov_rageColour.string, defaultColors, hcolor);
			}
			hcolor[3] = rageTime;
			
			if (!cg.renderingThirdPerson)
				CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			
			cgRageFadeTime = 0;
			cgRageFadeVal = 0.0f;
		} else if (cgRageTime) {
			if (!cgRageFadeTime) {
				cgRageFadeTime = cg.time;
				cgRageFadeVal = 0.15f;
			}			
			rageTime = cgRageFadeVal;			
			cgRageFadeVal -= ((cg.time - cgRageFadeTime) + cg.timeFraction) * 0.000005f;
			
			if (rageTime < 0.0f)
				rageTime = 0.0f;
			else if (rageTime > 0.15f)
				rageTime = 0.15f;
			
			if (cg.playerPredicted && cg.snap->ps.fd.forceRageRecoveryTime > cg.time) {
				float checkRageRecTime = rageTime;				
				if (checkRageRecTime < 0.15f) {
					checkRageRecTime = 0.15f;
				}
				
				if (mov_rageColour.string[0] == '0') {
					hcolor[0] = rageTime * 4.0f;
					if (hcolor[0] < 0.2f) {
						hcolor[0] = 0.2f;
					}
					hcolor[1] = 0.2f;
					hcolor[2] = 0.2f;
				} else {
					Q_parseColor(mov_rageColour.string, defaultColors, hcolor);
					hcolor[0] *= rageTime * 6.0f;
					hcolor[1] *= rageTime * 6.0f;
					hcolor[2] *= rageTime * 6.0f;
					if (hcolor[0] < 0.1f) {
						hcolor[0] = 0.1f;
					}
					if (hcolor[1] < 0.1f) {
						hcolor[1] = 0.1f;
					}
					if (hcolor[2] < 0.1f) {
						hcolor[2] = 0.1f;
					}
				}
				hcolor[3] = checkRageRecTime;
			} else {
				if (mov_rageColour.string[0] == '0') {
					hcolor[0] = 0.7f;
					hcolor[1] = 0;
					hcolor[2] = 0;
				} else {
					Q_parseColor(mov_rageColour.string, defaultColors, hcolor);
				}
				hcolor[3] = rageTime;
			}
			
			if (!cg.renderingThirdPerson && rageTime) {
				CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			} else {
				if (cg.playerPredicted && cg.snap->ps.fd.forceRageRecoveryTime > cg.time) {
					hcolor[0] = 0.2f;
					hcolor[1] = 0.2f;
					hcolor[2] = 0.2f;
					hcolor[3] = 0.15;
					CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
				}
				cgRageTime = 0;
			}
		} else if (cg.playerPredicted && cg.snap->ps.fd.forceRageRecoveryTime > cg.time) {
			if (!cgRageRecTime)
				cgRageRecTime = cg.time;			
			rageRecTime = (cg.time - cgRageRecTime) + cg.timeFraction;			
			rageRecTime /= 9000.0f;
			
			if (rageRecTime < 0.15f)
				rageRecTime = 0.15f;//0;
			else if (rageRecTime > 0.15f)
				rageRecTime = 0.15f;
			
			hcolor[3] = rageRecTime;
			hcolor[0] = 0.2f;
			hcolor[1] = 0.2f;
			hcolor[2] = 0.2f;
			
			if (!cg.renderingThirdPerson)
				CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			
			cgRageRecFadeTime = 0;
			cgRageRecFadeVal = 0.0f;
		} else if (cg.playerPredicted && cgRageRecTime) {
			if (!cgRageRecFadeTime) {
				cgRageRecFadeTime = cg.time;
				cgRageRecFadeVal = 0.15f;
			}			
			rageRecTime = cgRageRecFadeVal;			
			cgRageRecFadeVal -= ((cg.time - cgRageRecFadeTime) + cg.timeFraction) * 0.000005f;
			
			if (rageRecTime < 0.0f)
				rageRecTime = 0.0f;
			else if (rageRecTime > 0.15f)
				rageRecTime = 0.15f;
			
			hcolor[3] = rageRecTime;
			hcolor[0] = 0.2f;
			hcolor[1] = 0.2f;
			hcolor[2] = 0.2f;
			
			if (!cg.renderingThirdPerson && rageRecTime)
				CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			else
				cgRageRecTime = 0;
		}
		
		if (forcePowersActive & (1 << FP_ABSORB)) {
			if (!cgAbsorbTime)
				cgAbsorbTime = cg.time;			
			absorbTime = (cg.time - cgAbsorbTime) + cg.timeFraction;			
			absorbTime /= 9000.0f;
			
			if (absorbTime < 0.0f)
				absorbTime = 0.0f;
			else if (absorbTime > 0.15f)
				absorbTime = 0.15f;
			
			if (mov_absorbColour.string[0] == '0') {
				hcolor[0] = 0.0f;
				hcolor[1] = 0.0f;
				hcolor[2] = 0.7f;
			} else {
				Q_parseColor(mov_absorbColour.string, defaultColors, hcolor);
			}
			hcolor[3] = absorbTime / 2.0f;
			
			if (!cg.renderingThirdPerson)
				CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			
			cgAbsorbFadeTime = 0;
			cgAbsorbFadeVal = 0.0f;
		} else if (cgAbsorbTime) {
			if (!cgAbsorbFadeTime) {
				cgAbsorbFadeTime = cg.time;
				cgAbsorbFadeVal = 0.15f;
			}			
			absorbTime = cgAbsorbFadeVal;			
			cgAbsorbFadeVal -= ((cg.time - cgAbsorbFadeTime) + cg.timeFraction) * 0.000005f;
			
			if (absorbTime < 0.0f)
				absorbTime = 0.0f;
			else if (absorbTime > 0.15f)
				absorbTime = 0.15f;
			
			if (mov_absorbColour.string[0] == '0') {
				hcolor[0] = 0.0f;
				hcolor[1] = 0.0f;
				hcolor[2] = 0.7f;
			} else {
				Q_parseColor(mov_absorbColour.string, defaultColors, hcolor);
			}
			hcolor[3] = absorbTime / 2.0f;
			
			if (!cg.renderingThirdPerson && absorbTime)
				CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			else
				cgAbsorbTime = 0;
		}
		
		if (forcePowersActive & (1 << FP_PROTECT)) {
			if (!cgProtectTime) {
				cgProtectTime = cg.time;
			}			
			protectTime = (cg.time - cgProtectTime) + cg.timeFraction;			
			protectTime /= 9000.0f;
			
			if (protectTime < 0.0f)
				protectTime = 0.0f;
			else if (protectTime > 0.15f)
				protectTime = 0.15f;
			
			if (mov_protectColour.string[0] == '0') {
				hcolor[0] = 0.0f;
				hcolor[1] = 0.7f;
				hcolor[2] = 0.0f;
			} else {
				Q_parseColor(mov_protectColour.string, defaultColors, hcolor);
			}
			hcolor[3] = protectTime / 2.0f;
			
			if (!cg.renderingThirdPerson)
				CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			
			cgProtectFadeTime = 0;
			cgProtectFadeVal = 0.0f;
		} else if (cgProtectTime) {
			if (!cgProtectFadeTime) {
				cgProtectFadeTime = cg.time;
				cgProtectFadeVal = 0.15f;
			}			
			protectTime = cgProtectFadeVal;			
			cgProtectFadeVal -= ((cg.time - cgProtectFadeTime) + cg.timeFraction) * 0.000005f;
			
			if (protectTime < 0.0f)
				protectTime = 0.0f;
			else if (protectTime > 0.15f)
				protectTime = 0.15f;
			
			if (mov_protectColour.string[0] == '0') {
				hcolor[0] = 0.0f;
				hcolor[1] = 0.7f;
				hcolor[2] = 0.0f;
			} else {
				Q_parseColor(mov_protectColour.string, defaultColors, hcolor);
			}
			hcolor[3] = protectTime / 2.0f;
			
			if (!cg.renderingThirdPerson && protectTime)
				CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			else
				cgProtectTime = 0;
		}
		
		if ((cg.playerPredicted && BG_HasYsalamiri(cgs.gametype, &cg.snap->ps))
			|| (!cg.playerPredicted && CG_HasYsalamiri(cgs.gametype, &cg.playerCent->currentState))) {
			if (!cgYsalTime)
				cgYsalTime = cg.time;			
			ysalTime = (cg.time - cgYsalTime) + cg.timeFraction;			
			ysalTime /= 9000.0f;
			
			if (ysalTime < 0.0f)
				ysalTime = 0.0f;
			else if (ysalTime > 0.15f)
				ysalTime = 0.15f;
			
			hcolor[3] = ysalTime / 2.0f;
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.0f;
			
			if (!cg.renderingThirdPerson)
				CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			
			cgYsalFadeTime = 0;
			cgYsalFadeVal = 0.0f;
		} else if (cgYsalTime) {
			if (!cgYsalFadeTime) {
				cgYsalFadeTime = cg.time;
				cgYsalFadeVal = 0.15f;
			}			
			ysalTime = cgYsalFadeVal;			
			cgYsalFadeVal -= ((cg.time - cgYsalFadeTime) + cg.timeFraction) * 0.000005f;
			
			if (ysalTime < 0.0f)
				ysalTime = 0.0f;
			else if (ysalTime > 0.15f)
				ysalTime = 0.15f;
			
			hcolor[3] = ysalTime / 2.0f;
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.0f;
			
			if (!cg.renderingThirdPerson && ysalTime)
				CG_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
			else
				cgYsalTime = 0;
		}
	}
	CG_DrawInWaterTints();
}

qboolean gCGHasFallVector = qfalse;
vec3_t gCGFallVector;

void CG_UpdateFallVector (void) {
	if (!cg.playerCent)
		goto clearFallVector;

	if (cg.fallingToDeath) {
		float	fallTime; 
		vec4_t	hcolor;

		fallTime = (cg.time - cg.fallingToDeath) + cg.timeFraction;

		fallTime /= (float)(FALL_FADE_TIME/2.0f);

		if (fallTime < 0)
			fallTime = 0;
		else if (fallTime > 1)
			fallTime = 1;

		hcolor[3] = fallTime;
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 0;

		CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, hcolor);

		if (!gCGHasFallVector) {
			VectorCopy(cg.playerCent->lerpOrigin, gCGFallVector);
			gCGHasFallVector = qtrue;
		}
	} else {
		if (gCGHasFallVector) {
clearFallVector:
			gCGHasFallVector = qfalse;
			VectorClear(gCGFallVector);
		}
	}
}

void CG_CameraDraw2D( void ) {
	CG_SaberClashFlare();
	if (cg_drawFPS.integer) {
		CG_DrawFPS(0);
	}
	if (cg_draw2D.integer > 1) {
		if (cg_draw2D.integer == 3)
			CG_DrawInWaterTints();
		trap_S_UpdateEntityPosition(ENTITYNUM_NONE, cg.refdef.vieworg);
		CG_ChatBox_DrawStrings();
	}
}

/*
=================
CG_Draw2D
=================
*/
static vec4_t hcolor = {0, 0, 0, 0};
void CG_Draw2D (void) {
	float			inTime = cg.invenSelectTime+WEAPON_SELECT_TIME;
	float			wpTime = cg.weaponSelectTime+WEAPON_SELECT_TIME;
	float			bestTime;

	// if we are taking a levelshot for the menu, don't draw anything
	if (cg.levelShot) {
		return;
	}
	if (!cg.playerCent)
		return;
	
	CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);

	if (cgs.clientinfo[cg.playerCent->currentState.clientNum].team == TEAM_SPECTATOR) {
		CG_Clear2DTintsTimes();
	}

	if (cg_draw2D.integer == 0) {
		CG_UpdateFallVector();
		return;
	}

	if (cg_draw2D.integer == 3 && mov_fragsOnly.integer == 0) {
		CG_SaberClashFlare();
		CG_DrawInWaterTints();
		CG_UpdateFallVector();
		CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
		return;
	}

	if (mov_fragsOnly.integer != 0) {
		CG_SaberClashFlare();
		if(!cg.renderingThirdPerson && mov_fragsOnly.integer == 2)
			CG_DrawZoomMask();
		if (cg.playerPredicted)
			CG_DrawReward();
		CG_DrawCenterString();
		if (cg_draw2D.integer == 3) {
			CG_DrawInWaterTints();
		}
		CG_UpdateFallVector();
		CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hcolor);
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		CG_ChatBox_DrawStrings();
		return;
	}

	if (!cg.playerPredicted) {
		cg.scoreBoardShowing = qfalse;
		CG_Draw2DScreenTints();
		if (cg.playerCent->currentState.time2)
			CG_DrawHolocronIcons();
		if (cg.playerCent->currentState.forcePowersActive)
			CG_DrawActivePowers();
		CG_DrawZoomMask();
		if (!(cg.playerCent->currentState.eFlags & EF_DEAD))
			CG_DrawCrosshairNames();
		if (cg_drawStatus.integer)
			CG_DrawFlagStatus();
		CG_SaberClashFlare();
		if (cg_drawStatus.integer)
			CG_DrawHUD(cg.playerCent);
		CG_DrawPickupItem();
		CG_UpdateFallVector();
		CG_DrawVote();
		CG_DrawUpperRight();
		CG_DrawFollow();
		CG_DrawTeamVote();
		CG_DrawCenterString();
		CG_ChatBox_DrawStrings();
		return;
	}

	CG_Draw2DScreenTints();

	if (cg.snap->ps.rocketLockIndex != ENTITYNUM_NONE && (cg.time - cg.snap->ps.rocketLockTime) > 0) {
		CG_DrawRocketLocking( cg.snap->ps.rocketLockIndex, cg.snap->ps.rocketLockTime );
	}

	if (cg.snap->ps.holocronBits)
		CG_DrawHolocronIcons();
	if (cg.snap->ps.fd.forcePowersActive || cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
		CG_DrawActivePowers();

	if (cg.snap->ps.jetpackFuel < 100) //draw it as long as it isn't full
        CG_DrawJetpackFuel();        
	if (cg.snap->ps.cloakFuel < 100) //draw it as long as it isn't full
		CG_DrawCloakFuel();
	if (cg.predictedPlayerState.emplacedIndex > 0) {
		centity_t *eweb = &cg_entities[cg.predictedPlayerState.emplacedIndex];
		if (eweb->currentState.weapon == WP_NONE) //using an e-web, draw its health
			CG_DrawEWebHealth();
	}

	// Draw this before the text so that any text won't get clipped off
	CG_DrawZoomMask();

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		if (!cg.demoPlayback)
			CG_DrawSpectator();
		CG_DrawCrosshair(NULL, 0);
		CG_DrawCrosshairNames();
		CG_SaberClashFlare();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if (!cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0) {   
			int drawSelect = 0;
			//CG_DrawTemporaryStats();
			CG_DrawAmmoWarning();
			CG_DrawCrosshairNames();

			if (cg_drawStatus.integer)
				CG_DrawIconBackground();

			if (inTime > wpTime) {
				drawSelect = 1;
				bestTime = cg.invenSelectTime;
			} else { //only draw the most recent since they're drawn in the same place
				drawSelect = 2;
				bestTime = cg.weaponSelectTime;
			}
			if (cg.forceSelectTime > bestTime)
				drawSelect = 3;

			switch(drawSelect) {
			case 1:
				CG_DrawInvenSelect();
				break;
			case 2:
				CG_DrawWeaponSelect();
				break;
			case 3:
				CG_DrawForceSelect();
				break;
			default:
				break;
			}

			if (cg_drawStatus.integer)
				CG_DrawFlagStatus();
			CG_SaberClashFlare();
			if (cg_drawStatus.integer)
				CG_DrawStats();

			CG_DrawMovementKeys();
			CG_DrawPickupItem();
			//Do we want to use this system again at some point?
			CG_DrawReward();
		}   
	}
	
	CG_UpdateFallVector();

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();
	

	if (!cl_paused.integer) {
		CG_DrawBracketedEntities();
		CG_DrawUpperRight();
	}

	if ( !CG_DrawFollow() ) {
		CG_DrawWarmup();
	}

	if (cgSiegeRoundState) {
		char pStr[1024];
		int rTime = 0;

		//cgSiegeRoundBeganTime = 0;

		switch (cgSiegeRoundState) {
		case 1:
			CG_CenterPrint(CG_GetStringEdString("MP_INGAME", "WAITING_FOR_PLAYERS"), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
			break;
		case 2:
			rTime = (SIEGE_ROUND_BEGIN_TIME - (cg.time - cgSiegeRoundTime));
		
			if (rTime < 0)
				rTime = 0;
			else if (rTime > SIEGE_ROUND_BEGIN_TIME)
				rTime = SIEGE_ROUND_BEGIN_TIME;

			rTime /= 1000;

			rTime += 1;

			if (rTime < 1)
				rTime = 1;

			if (rTime <= 3 && rTime != cgSiegeRoundCountTime) {
				cgSiegeRoundCountTime = rTime;

				if (mov_soundDisable.integer & SDISABLE_ANNOUNCER) goto skipCounter;

				switch (rTime) {
				case 1:
					trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
					break;
				case 2:
					trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
					break;
				case 3:
					trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
					break;
				default:
					break;
				}
			}
skipCounter:
			strcpy(pStr, va("%s %i...", CG_GetStringEdString("MP_INGAME", "ROUNDBEGINSIN"), rTime));
			CG_CenterPrint(pStr, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
			//same
			break;
		default:
			break;
		}
		cgSiegeEntityRender = 0;
	} else if (cgSiegeRoundTime) {
		CG_CenterPrint("", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
		cgSiegeRoundTime = 0;

		//cgSiegeRoundBeganTime = cg.time;
		cgSiegeEntityRender = 0;
	} else if (cgSiegeRoundBeganTime) { //Draw how much time is left in the round based on local info.
		int timedTeam = TEAM_FREE;
		int timedValue = 0;

		//render the objective item model since this client has it
		if (cgSiegeEntityRender)
			CG_DrawSiegeHUDItem();

		if (team1Timed) {
			timedTeam = TEAM_RED; //team 1
			if (cg_beatingSiegeTime)
				timedValue = cg_beatingSiegeTime;
			else
				timedValue = team1Timed;
		} else if (team2Timed) {
			timedTeam = TEAM_BLUE; //team 2
			if (cg_beatingSiegeTime)
				timedValue = cg_beatingSiegeTime;
			else
				timedValue = team2Timed;
		}

		if (timedTeam != TEAM_FREE) { //one of the teams has a timer
			int timeRemaining;
			qboolean isMyTeam = qfalse;

			//in switchy mode but not beating a time, so count up.
			if (cgs.siegeTeamSwitch && !cg_beatingSiegeTime) {
				timeRemaining = (cg.time-cgSiegeRoundBeganTime);
				if (timeRemaining < 0)
					timeRemaining = 0;
			} else {
				timeRemaining = (((cgSiegeRoundBeganTime)+timedValue) - cg.time);
			}

			if (timeRemaining > timedValue)
				timeRemaining = timedValue;
			else if (timeRemaining < 0)
				timeRemaining = 0;

			if (timeRemaining)
				timeRemaining /= 1000;

			//the team that's timed is the one this client is on
			if (cg.predictedPlayerState.persistant[PERS_TEAM] == timedTeam)
				isMyTeam = qtrue;

			CG_DrawSiegeTimer(timeRemaining, isMyTeam);
		}
	} else {
		cgSiegeEntityRender = 0;
	}

	if (cg_siegeDeathTime) {
		int timeRemaining = (cg_siegeDeathTime - cg.time);

		if (timeRemaining < 0 ) {
			timeRemaining = 0;
			cg_siegeDeathTime = 0;
		}

		if (timeRemaining)
			timeRemaining /= 1000;

		CG_DrawSiegeDeathTimer(timeRemaining);
	}

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing) {
		CG_DrawCenterString();
	}
	
	// always draw chat
	CG_ChatBox_DrawStrings();
}

qboolean CG_CullPointAndRadius( const vec3_t pt, vec_t radius);
void CG_DrawMiscStaticModels( void ) {
	int i, j;
	refEntity_t ent;
	vec3_t cullorg;
	vec3_t diff;

	memset( &ent, 0, sizeof( ent ) );

	ent.reType = RT_MODEL;
	ent.frame = 0;
	ent.nonNormalizedAxes = qtrue;
	
	// static models don't project shadows
	ent.renderfx = RF_NOSHADOW;
	
	for( i = 0; i < cgs.numMiscStaticModels; i++ ) {
		VectorCopy(cgs.miscStaticModels[i].org, cullorg);
		cullorg[2] += 1.0f;

		if ( cgs.miscStaticModels[i].zoffset ) {
			cullorg[2] += cgs.miscStaticModels[i].zoffset;
		}
		if( cgs.miscStaticModels[i].radius ) {
			if( CG_CullPointAndRadius( cullorg, cgs.miscStaticModels[i].radius ) ) {
 				continue;
			}
		}

		if( !trap_R_inPVS( cg.refdef.vieworg, cullorg, cg.refdef.areamask ) ) {
			continue;
		}

		VectorCopy( cgs.miscStaticModels[i].org, ent.origin );
		VectorCopy( cgs.miscStaticModels[i].org, ent.oldorigin );
		VectorCopy( cgs.miscStaticModels[i].org, ent.lightingOrigin );

		for( j = 0; j < 3; j++ ) {
			VectorCopy( cgs.miscStaticModels[i].axes[j], ent.axis[j] );
		}
		ent.hModel = cgs.miscStaticModels[i].model;

		VectorSubtract(ent.origin, cg.refdef.vieworg, diff);
		if (VectorLength(diff)-(cgs.miscStaticModels[i].radius) <= cg.distanceCull) {
			trap_R_AddRefEntityToScene( &ent );
		}
	}
}

static void CG_DrawTourneyScoreboard() {
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView, qboolean draw2D ) {
	float		separation;
	vec3_t		baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	switch ( stereoView ) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error( "CG_DrawActive: Undefined stereoView" );
	}


	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}

	cg.refdef.rdflags |= RDF_DRAWSKYBOX;

	CG_DrawMiscStaticModels();


	if (CG_MultiSpecSkipBackground()) {
		trap_R_ClearScene();
		CG_MultiSpecDrawBackground();
	} else
		// draw 3D view
		trap_R_RenderScene(&cg.refdef);

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	// draw status bar and other floating elements
	if ( draw2D ) {
		CG_Draw2D();
	}
	CG_MultiSpecMain();

	doFX = qfalse;
}

void Dzikie_CG_DrawLine(float x1, float y1, float x2, float y2, float size, vec4_t color, float alpha, float ycutoff)
{
    float stepx, stepy, length = sqrt ((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
    int i;

    if (length < 1)
        length = 1;
    else if (length > 2000)
        length = 2000;
    if (!ycutoff)
        ycutoff = 480;

    stepx = (x2-x1)/(length/size);
    stepy = (y2-y1)/(length/size);

    trap_R_SetColor(color);

    for (i=0; i<=(length/size); i++) {
        if (x1 < 640 && y1 < 480 && y1 < ycutoff)
            CG_DrawPic(x1, y1, size, size, cgs.media.whiteShader);
        x1 += stepx;
        y1 += stepy;
    }
}

void Dzikie_CG_DrawSpeed(int moveDir) {
    float length;
    float diff;
    float midx;
    float midy;
    vec3_t velocity_copy;
    vec3_t viewangle_copy;
//	vec3_t velocity_normal;
    vec3_t velocity_angle;
    float g_speed;
    float accel;
    float optiangle;
    usercmd_t cmd = { 0 };

    if (cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback) {
        trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd );
    }
    else if (cg.snap) {
        moveDir = cg.snap->ps.movementDir;
        switch ( moveDir ) {
            case 0: // W
                cmd.forwardmove = 127; break;
            case 1: // WA
                cmd.forwardmove = 127; cmd.rightmove = -127; break;
            case 2: // A
                cmd.rightmove = -127;	break;
            case 3: // AS
                cmd.rightmove = -127;	cmd.forwardmove = -127; break;
            case 4: // S
                cmd.forwardmove = -127; break;
            case 5: // SD
                cmd.forwardmove = -127; cmd.rightmove = 127; break;
            case 6: // D
                cmd.rightmove = 127; break;
            case 7: // DW
                cmd.rightmove = 127; cmd.forwardmove = 127;	break;
            default:
                break;
        }
    }
    else {
        return; //No cg.snap causes this to return.
    }

    midx=SCREEN_WIDTH/2;
    midy=SCREEN_HEIGHT/2;
    VectorCopy(cg.predictedPlayerState.velocity,velocity_copy);
    velocity_copy[2]=0;
    VectorCopy(cg.refdef.viewangles, viewangle_copy);
    viewangle_copy[PITCH]=0;
    length=VectorNormalize(velocity_copy);
    g_speed=cg.predictedPlayerState.speed;
    accel = g_speed;
    accel*=8.0f;
    accel/=1000;
    optiangle=(g_speed-accel)/length;
    if ((optiangle<=1) && (optiangle>=-1))
        optiangle=acos(optiangle);
    else
        optiangle=0;
    length/=5;
    //length = VectorLength(cg.predictedPlayerState.velocity)/5;
    if (length>(SCREEN_HEIGHT/2))
        length=(float)(SCREEN_HEIGHT/2);
    vectoangles(velocity_copy,velocity_angle);
    diff=AngleSubtract(viewangle_copy[YAW],velocity_angle[YAW]);
    diff=diff/180*M_PI;

    //Com_Printf("Diff is %.3f\n", diff);

//	str = va( "%f %f %f", g_speed, accel, optiangle);
//	w = CG_Text_Width_Ext( str, 0.25f, 0, &cgs.media.limboFont1 );
//	CG_Text_Paint_Ext( (float)(SCREEN_WIDTH/2), (float)(SCREEN_HEIGHT/2), 0.25f, 0.25f, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1 );
    Dzikie_CG_DrawLine (midx, midy, midx+length*sin(diff), midy-length*cos(diff), 1, colorRed, 0.75f, 0);
    Dzikie_CG_DrawLine (midx,midy,midx+cmd.rightmove,midy-cmd.forwardmove,1,colorCyan,0.75f,0);
    Dzikie_CG_DrawLine (midx,midy,midx+length/2*sin(diff+optiangle),midy-length/2*cos(diff+optiangle),1,colorRed,0.75f,0);
    Dzikie_CG_DrawLine (midx,midy,midx+length/2*sin(diff-optiangle),midy-length/2*cos(diff-optiangle),1,colorRed,0.75f,0);

}


static void DrawStrafeLine(vec3_t velocity, float diff, qboolean active, int moveDir) { //moveDir is 1-7 for wasd combinations, and 8 for the centerline in cpm style, 9 and 10 for backwards a/d lines
    vec3_t start, angs, forward, delta, line;
    float x, y, startx, starty, lineWidth;
    int sensitivity = cg_strafeHelperPrecision.integer;
    static const int LINE_HEIGHT = 230; //240 is midpoint, so it should be a little higher so crosshair is always on it.
    static const vec4_t activeColor = {0, 1, 0, 0.75}, normalColor = {1, 1, 1, 0.75}, invertColor = {0.5f, 1, 1, 0.75}, wColor = {1, 0.5, 0.5, 0.75}, rearColor = {0.5, 1,1, 0.75}, centerColor = {0.5, 1, 1, 0.75};
    vec4_t color = {1, 1, 1, 0.75};

    //how the fuck do these colors work, 0111 is cyan?

    if (cg_strafeHelperPrecision.integer < 100)
        sensitivity = 100;
    else if (cg_strafeHelperPrecision.integer > 10000)
        sensitivity = 10000;

    lineWidth = cg_strafeHelperLineWidth.value;
    if (lineWidth < 0.25f)
        lineWidth = 0.25f;
    else if (lineWidth > 5)
        lineWidth = 5;

    if (active) {
        color[0] = cg.strafeHelperActiveColor[0];
        color[1] = cg.strafeHelperActiveColor[1];
        color[2] = cg.strafeHelperActiveColor[2];
        color[3] = cg.strafeHelperActiveColor[3];
        memcpy(color, activeColor, sizeof(vec4_t));
    }
    else {
        if (moveDir == 1 || moveDir == 7)
            memcpy(color, normalColor, sizeof(vec4_t));
        else if (moveDir == 2 || moveDir == 6)
            memcpy(color, invertColor, sizeof(vec4_t));
        else if (moveDir == 0 || moveDir == 4)
            memcpy(color, wColor, sizeof(vec4_t));
        else if (moveDir == 8)
            memcpy(color, centerColor, sizeof(vec4_t));
        else if (moveDir == 9 || moveDir == 10 || moveDir == 3 || moveDir == 5)
            memcpy(color, rearColor, sizeof(vec4_t));
        else if (moveDir == 3 || moveDir == 5)
            memcpy(color, invertColor, sizeof(vec4_t));
        color[3] = cg_strafeHelperInactiveAlpha.value / 255.0f;
    }

    if (!(cg_strafeHelper.integer & SHELPER_SUPEROLDSTYLE) && !cg.renderingThirdPerson)
        VectorCopy(cg.refdef.vieworg, start);
    else
        VectorCopy(cg.predictedPlayerState.origin, start); //This created problems for some peoplem, use refdef instead? (avygeil fix)

    VectorCopy(velocity, angs);
    angs[YAW] += diff;
    AngleVectors( angs, forward, NULL, NULL );
    VectorScale( forward, sensitivity, delta ); // line length

    line[0] = delta[0] + start[0];
    line[1] = delta[1] + start[1];
    line[2] = start[2];

    if ( !CG_WorldCoordToScreenCoordFloat(line, &x, &y))
        return;

    if (cg_strafeHelper.integer & SHELPER_NEWBARS) {
        Dzikie_CG_DrawLine(x, (SCREEN_HEIGHT / 2) + 10, x, (SCREEN_HEIGHT / 2) - 10, lineWidth, color, 0.75f, 0);
        //CG_DottedLine( x, 260, x, 220, 1, 100, color, 0.75f ); //240 is center, so 220 - 260 is symetrical on crosshair.'
    }
    if (cg_strafeHelper.integer & SHELPER_OLDBARS && active && moveDir != 0) { //Not sure how to deal with multiple lines for W only so just fuck it for now..
        //Proper way is to tell which line we are closest to aiming at and display the shit for that...
        CG_FillRect(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, (-4.444 * AngleSubtract(cg.predictedPlayerState.viewangles[YAW], angs[YAW])), 12, colorTable[CT_RED]);
    }
    if (cg_strafeHelper.integer & SHELPER_OLDSTYLE) {
        int cutoff = SCREEN_HEIGHT - cg_strafeHelperCutoff.integer; //Should be between 480 and LINE_HEIGHT
        //distance = sqrt( ((320-x)*(320-x)) + ((480-LINE_HEIGHT)*(480-LINE_HEIGHT)) );

        if (cutoff > SCREEN_HEIGHT)
            cutoff = SCREEN_HEIGHT;
        if (cutoff < LINE_HEIGHT + 20)
            cutoff = LINE_HEIGHT + 20;

        //Com_Printf("Numdots %i\n", distance);
        //if (distance < 0)
        //distance = 100;
        //else if (distance > 1000)
        //distance = 1000;

        Dzikie_CG_DrawLine(SCREEN_WIDTH / 2, SCREEN_HEIGHT, x, LINE_HEIGHT, lineWidth, color, color[3], cutoff);
        //CG_DottedLineSegment( 320, 480, x, LINE_HEIGHT, 1, distance, color, color[3], cutoff ); //240 is center, so 220 - 260 is symetrical on crosshair.
    }
    if (cg_strafeHelper.integer & SHELPER_SUPEROLDSTYLE) {
        int cutoff = SCREEN_HEIGHT - cg_strafeHelperCutoff.integer; //Should be between 480 and LINE_HEIGHT
        //distance = sqrt( ((320-x)*(320-x)) + ((480-LINE_HEIGHT)*(480-LINE_HEIGHT)) );

        if (cutoff > SCREEN_HEIGHT)
            cutoff = SCREEN_HEIGHT;
        if (cutoff < LINE_HEIGHT + 20)
            cutoff = LINE_HEIGHT + 20;

        if (CG_WorldCoordToScreenCoordFloat(start, &startx, &starty))
            Dzikie_CG_DrawLine(startx, starty, x, y, lineWidth, color, color[3], cutoff);
        //CG_DottedLineSegment( startx, starty, x, y, 1, distance, color, color[3], cutoff ); //240 is center, so 220 - 260 is symetrical on crosshair.
    }
    if (cg_strafeHelper.integer & SHELPER_WEZE) {
        Dzikie_CG_DrawSpeed(moveDir);
    }
}

QINLINE int PM_GetMovePhysics(void)
{
    if (cg.japro.detected) {
        if (!pm)
            return MV_JKA;

        if (pm->ps->m_iVehicleNum)
            return MV_SWOOP;

        if (pm->ps->stats[STAT_MOVEMENTSTYLE] >= MV_COOP_JKA)
            return (pm->ps->stats[STAT_MOVEMENTSTYLE] - (MV_COOP_JKA - 1));

        return pm->ps->stats[STAT_MOVEMENTSTYLE];
    }
    else if (cgs.gametype == GT_SIEGE) {
        return MV_SIEGE;
    }
    return MV_JKA;
}
static void CG_StrafeHelper(centity_t *cent)
{
    vec_t * velocity = (cent->currentState.clientNum == cg.clientNum ? cg.predictedPlayerState.velocity : cent->currentState.pos.trDelta);
    static vec3_t velocityAngle;
    const float currentSpeed = cg.currentSpeed;
    float pmAccel = 10.0f, pmAirAccel = 1.0f, pmFriction = 6.0f, frametime, optimalDeltaAngle, baseSpeed = cg.predictedPlayerState.speed;
    const int moveStyle = PM_GetMovePhysics();
    int moveDir;
    qboolean onGround;
    usercmd_t cmd = { 0 };

    if (moveStyle == 0)
        return;
    if (cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback) {
        trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd );
    }
    else if (cg.snap) {
        moveDir = cg.snap->ps.movementDir;
        switch ( moveDir ) {
            case 0: // W
                cmd.forwardmove = 1; break;
            case 1: // WA
                cmd.forwardmove = 1; cmd.rightmove = -1; break;
            case 2: // A
                cmd.rightmove = -1;	break;
            case 3: // AS
                cmd.rightmove = -1;	cmd.forwardmove = -1; break;
            case 4: // S
                cmd.forwardmove = -1; break;
            case 5: // SD
                cmd.forwardmove = -1; cmd.rightmove = 1; break;
            case 6: // D
                cmd.rightmove = 1; break;
            case 7: // DW
                cmd.rightmove = 1; cmd.forwardmove = 1;	break;
            default:
                break;
        }
        if (cg.snap->ps.pm_flags & PMF_JUMP_HELD)
            cmd.upmove = 1;
    }
    else {
        return; //No cg.snap causes this to return.
    }

    onGround = (qboolean)(cg.snap->ps.groundEntityNum == ENTITYNUM_WORLD); //sadly predictedPlayerState makes it jerky so need to use cg.snap groundentityNum, and check for cg.snap earlier

    if (moveStyle == MV_WSW) {
        pmAccel = 12.0f;
        pmFriction = 8.0f;
    }
    else if (moveStyle == MV_CPM || moveStyle == MV_RJCPM || moveStyle == MV_BOTCPM) {
        pmAccel = 15.0f;
        pmFriction = 8.0f;
    }
    else if (moveStyle == MV_SP) {
        pmAirAccel = 4.0f;
        pmAccel = 12.0f;
    }
    else if (moveStyle == MV_SLICK) {
        pmFriction = 0.0f;//unless walking?
        pmAccel = 30.0f;
    }

    if (currentSpeed < (baseSpeed - 1))
        return;

    if (cg.predictedPlayerState.pm_type == PM_JETPACK) {
        pmAirAccel = 1.4f; //idk
        if (cmd.upmove <= 0)
            baseSpeed *= 0.8f;
        else
            baseSpeed *= 2.0f;
    }
    else if (moveStyle == MV_SWOOP && cg.predictedPlayerState.m_iVehicleNum) {
        centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
        velocity = vehCent->currentState.pos.trDelta; //jerky otherwise?
        if (cg.predictedPlayerState.commandTime < vehCent->m_pVehicle->m_iTurboTime) {
            baseSpeed = vehCent->m_pVehicle->m_pVehicleInfo->turboSpeed;//1400
        }
        else {
            baseSpeed = vehCent->m_pVehicle->m_pVehicleInfo->speedMax;//700
        }
    }
    else if (moveStyle == MV_SP) {
        /*
        if ((DotProduct(cg.predictedPlayerState.velocity, wishdir)) < 0.0f)
        {//Encourage deceleration away from the current velocity
            wishspeed *= 1.35f;//pm_airDecelRate - adjust basespeed
        }
        */
        if (!(cg.predictedPlayerState.pm_flags & PMF_JUMP_HELD) && cmd.upmove > 0) { //Also, wishspeed *= scale.  Scale is different cuz of upmove in air.  Only works ingame not from spec
            baseSpeed /= 1.41421356237f; //umm.. dunno.. divide by sqrt(2)
        }
    }

    if (cg_strafeHelper_FPS.value < 1)
        frametime = ((float)cg.frametime * 0.001f);
    else if (cg_strafeHelper_FPS.value > 1000) // invalid
        frametime = 1;
    else frametime = 1 / cg_strafeHelper_FPS.value;

    if (onGround)//On ground
        optimalDeltaAngle = acos((double) ((baseSpeed - (pmAccel*baseSpeed*frametime)) / (currentSpeed*(1-pmFriction*(frametime))))) * (180.0f/M_PI) - 45.0f;
    else
        optimalDeltaAngle = acos((double) ((baseSpeed - (pmAirAccel*baseSpeed * frametime)) / currentSpeed)) * (180.0f/M_PI) - 45.0f;

    if (optimalDeltaAngle < 0 || optimalDeltaAngle > 360)
        optimalDeltaAngle = 0;

    //Com_Printf("Optimal Angle is %.3f\n", optimalDeltaAngle);

    velocity[2] = 0;
    vectoangles( velocity, velocityAngle ); //We have the offset from our Velocity angle that we should be aiming at, so now we need to get our velocity angle.

    if (moveStyle == MV_QW || moveStyle == MV_CPM || moveStyle == MV_PJK || moveStyle == MV_WSW || moveStyle == MV_RJCPM || moveStyle == MV_SWOOP || moveStyle == MV_BOTCPM || (moveStyle == MV_SLICK && !onGround)) {//QW, CPM, PJK, WSW, RJCPM have center line
        if (cg_strafeHelper.integer & SHELPER_CENTER)
            DrawStrafeLine(velocityAngle, 0, (qboolean)(cmd.forwardmove == 0 && cmd.rightmove != 0), 8); //Center
    }
    if (moveStyle != MV_QW && moveStyle != MV_SWOOP) { //Every style but QW has WA/WD lines
        if (cg_strafeHelper.integer & SHELPER_WA)
            DrawStrafeLine(velocityAngle, (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f)), (qboolean)(cmd.forwardmove > 0 && cmd.rightmove < 0), 1); //WA
        if (cg_strafeHelper.integer & SHELPER_WD)
            DrawStrafeLine(velocityAngle, (-optimalDeltaAngle - (cg_strafeHelperOffset.value * 0.01f)), (qboolean)(cmd.forwardmove > 0 && cmd.rightmove > 0), 7); //WD
    }
    if (moveStyle == MV_JKA || moveStyle == MV_Q3 || moveStyle == MV_RJQ3 || moveStyle == MV_JETPACK || moveStyle == MV_SPEED || moveStyle == MV_SP || (moveStyle == MV_SLICK && onGround)) { //JKA, Q3, RJQ3, Jetpack? have A/D
        if (cg_strafeHelper.integer & SHELPER_A)
            DrawStrafeLine(velocityAngle, -(45.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove == 0 && cmd.rightmove < 0), 2); //A
        if (cg_strafeHelper.integer & SHELPER_D)
            DrawStrafeLine(velocityAngle, (45.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove == 0 && cmd.rightmove > 0), 6); //D

        //A/D backwards strafe?
        if (cg_strafeHelper.integer & SHELPER_REAR) {
            DrawStrafeLine(velocityAngle, (225.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove == 0 && cmd.rightmove < 0), 9); //A
            DrawStrafeLine(velocityAngle, (135.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove == 0 && cmd.rightmove > 0), 10); //D
            DrawStrafeLine(velocityAngle, (180.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove < 0 && cmd.rightmove < 0), 3); //SA
            DrawStrafeLine(velocityAngle, (180.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove < 0 && cmd.rightmove > 0), 5); //SD
            DrawStrafeLine(velocityAngle, (180.0f + 45.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove < 0 && cmd.rightmove == 0), 4); //S
            DrawStrafeLine(velocityAngle, (-180.0f - 45.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove < 0 && cmd.rightmove == 0), 4); //S
        }
    }
    if (moveStyle == MV_JKA || moveStyle == MV_Q3 || moveStyle == MV_RJQ3 || moveStyle == MV_SWOOP || moveStyle == MV_JETPACK || moveStyle == MV_SPEED || moveStyle == MV_SP) {
        //W only
        if (cg_strafeHelper.integer & SHELPER_W) {
            DrawStrafeLine(velocityAngle, (45.0f + (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove > 0 && cmd.rightmove == 0), 0); //W
            DrawStrafeLine(velocityAngle, (-45.0f - (optimalDeltaAngle + (cg_strafeHelperOffset.value * 0.01f))), (qboolean)(cmd.forwardmove > 0 && cmd.rightmove == 0), 0); //W
        }
    }
}

#define PERCENT_SAMPLES 16
static void CG_DrawAccelMeter(void)
{
    float baseSpeed = cg.predictedPlayerState.speed;
    const float optimalAccel = baseSpeed * ((float)cg.frametime / 1000.0f);
    const float potentialSpeed = sqrtf(cg.previousSpeed * cg.previousSpeed - optimalAccel * optimalAccel + 2 * (250 * optimalAccel));
    float actualAccel, total, percentAccel, x;
    const float accel = cg.currentSpeed - cg.previousSpeed;
    static int t, i, previous, lastupdate;
    unsigned short frameTime;
    static float previousTimes[PERCENT_SAMPLES];
    static unsigned short index;

    x = speedometerXPos;

    if (cg_speedometer.integer & SPEEDOMETER_ENABLE) {
        if (cg_speedometer.integer & SPEEDOMETER_GROUNDSPEED)
            x -= 104;
        else
            x -= 52;
    }

    CG_DrawRect((x - 0.75f)*cgs.widthRatioCoef,
                cg_speedometerY.value - 11.0f,
                38.0f * cgs.widthRatioCoef,
                13.75f,
                0.5f,
                colorTable[CT_BLACK]);

    actualAccel = accel;
    if (actualAccel < 0)
        actualAccel = 0.001f;
    else if (actualAccel >(potentialSpeed - cg.currentSpeed)) //idk how
        actualAccel = (potentialSpeed - cg.currentSpeed) * 0.99f;

    //Com_Printf("Actual Accel this frame is %.3f, last speed was %.3f, current speed is %.3f, potential speed was %.3f, coef is %.3f\n", accel, cg.previousSpeed, cg.currentSpeed, potentialSpeed, actualAccel/(potentialSpeed - currentSpeed));

    t = trap_Milliseconds();
    frameTime = t - previous;
    previous = t;
    //if (t - lastupdate > 20)	//don't sample faster than this
    {
        lastupdate = t;
        previousTimes[index % PERCENT_SAMPLES] = actualAccel / (potentialSpeed - cg.currentSpeed);
        index++;
    }

    total = 0;
    for (i = 0; i < PERCENT_SAMPLES; i++) {
        total += previousTimes[i];
    }
    if (!total) {
        total = 1;
    }
    percentAccel = total / (float)PERCENT_SAMPLES;

    //if (cg_draw2D.integer == 2)
    //percentAccel = actualAccel / (potentialSpeed - cg.currentSpeed);

    //if ( percentAccel ) {
    if (percentAccel && cg.currentSpeed) {
        CG_FillRect((x + 0.25f) * cgs.widthRatioCoef,
                    cg_speedometerY.value - 9.9f,
                    36 * percentAccel * cgs.widthRatioCoef,
                    12,
                    colorTable[CT_RED]);
    }

    //Com_sprintf(accelPercentStr, sizeof(accelPercentStr), "%.2f%%", percentAccel); //how to average this
    //CG_Text_Paint(cg_speedometerX.integer, cg_speedometerY.integer -12, cg_speedometerSize.value, colorWhite, accelPercentStr, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);

    cg.previousSpeed = cg.currentSpeed;

}

static void CG_JumpHeight(centity_t *cent)
{
    const vec_t* const velocity = (cent->currentState.clientNum == cg.clientNum ? cg.predictedPlayerState.velocity : cent->currentState.pos.trDelta);
    char jumpHeightStr[32] = {0};

    if (cg.predictedPlayerState.fd.forceJumpZStart == -65536) //Coming back from a tele or w/e
        return;

    if (cg.predictedPlayerState.fd.forceJumpZStart && (cg.lastZSpeed > 0) && (velocity[2] <= 0)) {//If we were going up, and we are now going down, print our height.
        cg.lastJumpHeight = cg.predictedPlayerState.origin[2] - cg.predictedPlayerState.fd.forceJumpZStart;
        cg.lastJumpHeightTime = cg.time;
    }

    if ((cg.lastJumpHeightTime > cg.time - 1500) && (cg.lastJumpHeight > 0.0f)) {
        Com_sprintf(jumpHeightStr, sizeof(jumpHeightStr), "%.1f", cg.lastJumpHeight);
        CG_Text_Paint(speedometerXPos * cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorTable[CT_WHITE], jumpHeightStr, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
    }

    speedometerXPos += 42;

    cg.lastZSpeed = velocity[2];
}

static void CG_JumpDistance( void )
{
    char jumpDistanceStr[64] = {0};

    if (!cg.snap)
        return;

    if (cg.predictedPlayerState.groundEntityNum == ENTITYNUM_WORLD) {

        if (!cg.wasOnGround) {//We were just in the air, but now we arnt
            vec3_t distance;

            VectorSubtract(cg.predictedPlayerState.origin, cg.lastGroundPosition, distance);
            cg.lastJumpDistance = sqrtf(distance[0] * distance[0] + distance[1] * distance[1]); // is this right?
            cg.lastJumpDistanceTime = cg.time;
        }

        VectorCopy(cg.predictedPlayerState.origin, cg.lastGroundPosition);
        cg.wasOnGround = qtrue;
    }
    else {
        cg.wasOnGround = qfalse;
    }

    if ((cg.lastJumpDistanceTime > cg.time - 1500) && (cg.lastJumpDistance > 0.0f)) {
        Com_sprintf(jumpDistanceStr, sizeof(jumpDistanceStr), "%.1f", cg.lastJumpDistance);
        CG_Text_Paint(speedometerXPos * cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorTable[CT_WHITE], jumpDistanceStr, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
    }

    speedometerXPos += 52;
}

#define YAW_FRAMES    16
static void CG_DrawYawSpeed( void ) {
    static unsigned short previousYaws[YAW_FRAMES];
    static unsigned short index;
    static int    previous, lastupdate;
    int        t, i, yaw, total;
    unsigned short frameTime;
    const int        xOffset = 0;

    const float diff = AngleSubtract(cg.predictedPlayerState.viewangles[YAW], cg.lastYawSpeed);
    float yawspeed = diff / (cg.frametime * 0.001f);
    if (yawspeed < 0)
        yawspeed = -yawspeed;

    t = trap_Milliseconds();
    frameTime = t - previous;
    previous = t;
    if (t - lastupdate > 20)    //don't sample faster than this
    {
        lastupdate = t;
        previousYaws[index % YAW_FRAMES] = yawspeed;
        index++;
    }

    total = 0;
    for (i = 0; i < YAW_FRAMES; i++) {
        total += previousYaws[i];
    }
    if (!total) {
        total = 1;
    }
    yaw = total / (float)YAW_FRAMES;

    if (yaw) {
        char yawStr[64] = { 0 };
        if (yawspeed > 320)
            Com_sprintf(yawStr, sizeof(yawStr), "^1%03i", (int)(yaw + 0.5f)); //added 8 whitespaces idk how much to add - fixme
        else if (yawspeed > 265)
            Com_sprintf(yawStr, sizeof(yawStr), "^3%03i", (int)(yaw + 0.5f)); //added 8 whitespaces idk how much to add - fixme
        else
            Com_sprintf(yawStr, sizeof(yawStr), "%03i", (int)(yaw + 0.5f)); //added 8 whitespaces idk how much to add - fixme
        CG_Text_Paint(speedometerXPos * cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorTable[CT_WHITE], yawStr, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
    }

    cg.lastYawSpeed = cg.predictedPlayerState.viewangles[YAW];

    speedometerXPos += 16;
}

static void CG_DrawVerticalSpeed(void) {
    char speedStr5[64] = { 0 };
    float vertspeed = cg.predictedPlayerState.velocity[2];

    if (vertspeed < 0)
        vertspeed = -vertspeed;

    if (vertspeed) {
        Com_sprintf(speedStr5, sizeof(speedStr5), "%.0f", vertspeed);
        CG_Text_Paint(speedometerXPos * cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorWhite, speedStr5, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
    }

    speedometerXPos += 42;
}

static void CG_CalculateSpeed(centity_t *cent) {
    if (cg.predictedPlayerState.m_iVehicleNum) {
        centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];

        const vec_t * const velocity = (cent->currentState.clientNum == cg.clientNum ? vehCent->playerState->velocity : vehCent->currentState.pos.trDelta);
        cg.currentSpeed = sqrtf(velocity[0] * velocity[0] + velocity[1] * velocity[1]); // is this right?
    }
    else {
        const vec_t * const velocity = (cent->currentState.clientNum == cg.clientNum ? cg.predictedPlayerState.velocity : cent->currentState.pos.trDelta);
        cg.currentSpeed = sqrtf(velocity[0] * velocity[0] + velocity[1] * velocity[1]); // is this right?
    }
}

static void CG_RaceTimer(void)
{
    if (!cg.predictedPlayerState.stats[STAT_RACEMODE] || !cg.predictedPlayerState.duelTime) {
        cg.startSpeed = 0;
        cg.displacement = 0;
        cg.maxSpeed = 0;
        cg.displacementSamples = 0;
        return;
    }

    {
        char timerStr[48] = { 0 };
        const int time = (cg.time - cg.predictedPlayerState.duelTime);
        const int minutes = (time / 1000) / 60;
        const int seconds = (time / 1000) % 60;
        const int milliseconds = (time % 1000);

        if (time < cg.lastRaceTime) {
            cg.startSpeed = 0;
            cg.displacement = 0;
            cg.maxSpeed = 0;
            cg.displacementSamples = 0;
        }

        if (cg_raceTimer.integer > 1) {
            if (time > 0) {
                if (!cg.startSpeed)
                    cg.startSpeed = (int)(cg.currentSpeed + 0.5f);

                if (cg.currentSpeed > cg.maxSpeed)
                    cg.maxSpeed = (int)(cg.currentSpeed + 0.5f);
                cg.displacement += cg.currentSpeed;
                cg.displacementSamples++;
            }
        }

        cg.lastRaceTime = time;

        if (cg_raceTimer.integer < 3)
            Com_sprintf(timerStr, sizeof(timerStr), "%i:%02i.%i\n", minutes, seconds, milliseconds / 100);
        else
            Com_sprintf(timerStr, sizeof(timerStr), "%i:%02i.%03i\n", minutes, seconds, milliseconds);

        if (cg_raceTimer.integer > 1) {
            if (cg.displacementSamples)
                Q_strcat(timerStr, sizeof(timerStr), va("Max: %i\nAvg: %i", (int)(cg.maxSpeed + 0.5f), cg.displacement / cg.displacementSamples));
            if (time < 3000)
                Q_strcat(timerStr, sizeof(timerStr), va("\nStart: %i", cg.startSpeed));
        }

        CG_Text_Paint(cg_raceTimerX.integer, cg_raceTimerY.integer, cg_raceTimerSize.value, colorTable[CT_WHITE], timerStr, 0.0f, 0, ITEM_ALIGN_RIGHT | ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
    }
}


#define ACCEL_SAMPLES 32
static void CG_Speedometer(void)
{
    const char *accelStr, *accelStr2, *accelStr3;
    char speedStr[32] = {0}, speedStr2[32] = {0}, speedStr3[32] = {0};
    vec4_t colorSpeed = {1, 1, 1, 1};
    const float currentSpeed = cg.currentSpeed;
    static float lastSpeed = 0, previousAccels[ACCEL_SAMPLES];
    const float accel = currentSpeed - lastSpeed;
    float total, avgAccel;
    int t, i;
    unsigned short frameTime;
    static unsigned short index;
    static int	previous, lastupdate;

    lastSpeed = currentSpeed;

    if (currentSpeed > 250)
    {
        colorSpeed[1] = 1 / ((currentSpeed/250)*(currentSpeed/250));
        colorSpeed[2] = 1 / ((currentSpeed/250)*(currentSpeed/250));
    }

    t = trap_Milliseconds();
    frameTime = t - previous;
    previous = t;
    if (t - lastupdate > 5)	//don't sample faster than this
    {
        lastupdate = t;
        previousAccels[index % ACCEL_SAMPLES] = accel;
        index++;
    }

    total = 0;
    for ( i = 0 ; i < ACCEL_SAMPLES ; i++ ) {
        total += previousAccels[i];
    }
    if ( !total ) {
        total = 1;
    }
    avgAccel = total / (float)ACCEL_SAMPLES - 0.0625f;//fucking why does it offset by this number

    if (avgAccel > 0.0f)
    {
        accelStr = "^2\xb5:";
        accelStr2 = "^2k:";
        accelStr3 = "^2m: ";
    }
    else if (avgAccel < 0.0f)
    {
        accelStr = "^1\xb5:";
        accelStr2 = "^1k:";
        accelStr3 = "^1m: ";
    }
    else
    {
        accelStr = "^7\xb5:";
        accelStr2 = "^7k:";
        accelStr3 = "^7m: ";
    }

    if (!(cg_speedometer.integer & SPEEDOMETER_KPH) && !(cg_speedometer.integer & SPEEDOMETER_MPH))
    {
        Com_sprintf(speedStr, sizeof(speedStr), "   %.0f", floorf(currentSpeed + 0.5f));
        CG_Text_Paint(speedometerXPos*cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorWhite, accelStr, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
        CG_Text_Paint(speedometerXPos*cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorSpeed, speedStr, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
    }
    else if (cg_speedometer.integer & SPEEDOMETER_KPH)
    {
        Com_sprintf(speedStr2, sizeof(speedStr2), "   %.1f", currentSpeed * 0.05);
        CG_Text_Paint(speedometerXPos*cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorWhite, accelStr2, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
        CG_Text_Paint(speedometerXPos*cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorSpeed, speedStr2, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
    }
    else if (cg_speedometer.integer & SPEEDOMETER_MPH)
    {
        Com_sprintf(speedStr3, sizeof(speedStr3), "   %.1f", currentSpeed * 0.03106855);
        CG_Text_Paint(speedometerXPos*cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorWhite, accelStr3, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
        CG_Text_Paint(speedometerXPos*cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorSpeed, speedStr3, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
    }

    speedometerXPos += 52;

    if (cg_speedometer.integer & SPEEDOMETER_GROUNDSPEED) {
        char speedStr4[32] = {0};
        vec4_t colorGroundSpeed = {1, 1, 1, 1};

        if (cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE || cg.predictedPlayerState.velocity[2] < 0) { //On ground or Moving down
            cg.firstTimeInAir = qfalse;
        }
        else if (!cg.firstTimeInAir) { //Moving up for first time
            cg.firstTimeInAir = qtrue;
            cg.lastGroundSpeed = currentSpeed;
            cg.lastGroundTime = cg.time;
        }

        if (cg.lastGroundSpeed > 250) {
            colorGroundSpeed[1] = 1 / ((cg.lastGroundSpeed/250)*(cg.lastGroundSpeed/250));
            colorGroundSpeed[2] = 1 / ((cg.lastGroundSpeed/250)*(cg.lastGroundSpeed/250));
        }
        if ((cg.lastGroundTime > cg.time - 1500)) {
            if (cg.lastGroundSpeed) {
                Com_sprintf(speedStr4, sizeof(speedStr4), "%.0f", cg.lastGroundSpeed);
                CG_Text_Paint(speedometerXPos * cgs.widthRatioCoef, cg_speedometerY.integer, cg_speedometerSize.value, colorGroundSpeed, speedStr4, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_NONE);
            }
        }

        speedometerXPos += 52;
    }

    //Speedsounds
    for (i = 0; i<MAX_CLIENT_SPEEDPOINTS; i++) { //Add a speedpoint to the first available slot
        if (!cg.clientSpeedpoints[i].isSet)
            break;
        if ((int)(currentSpeed + 0.5f) >= cg.clientSpeedpoints[i].speed) {
            if (!cg.clientSpeedpoints[i].reached) {
                //trap_S_StartLocalSound(cgs.media.hitSound2, CHAN_LOCAL_SOUND);
                cg.clientSpeedpoints[i].reached = qtrue;
            }
        }
        else {
            cg.clientSpeedpoints[i].reached = qfalse;
        }
    }
}
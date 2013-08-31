// Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

#include "cg_demos.h"
#include "cg_lights.h"

demoMain_t demo;

#define BLURMAX 32

extern void CG_DamageBlendBlob( void );
extern void CG_PlayBufferedSounds( void );
extern int CG_CalcViewValues( void );
extern int CG_DemosCalcViewValues( void );
extern void CG_SetupFrustum( void );
extern void CG_CalcScreenEffects(void);
extern void CG_PowerupTimerSounds( void );
extern void CG_UpdateSoundTrackers();
extern float CG_DrawFPS( float y );
extern void CG_DrawUpperRight( void );
extern void CG_Draw2D( void );
extern void CG_DrawAutoMap(void);
extern void CG_DrawSkyBoxPortal(const char *cstr);
extern qboolean PM_InKnockDown( playerState_t *ps );
extern void CG_InterpolatePlayerState( qboolean grabAngles );

extern void trap_MME_BlurInfo( int* total, int * index );
extern void trap_MME_Capture( const char *baseName, float fps, float focus );
extern void trap_MME_CaptureStereo( const char *baseName, float fps, float focus );
extern int trap_MME_SeekTime( int seekTime );

static void CG_DemosUpdatePlayer( void ) {
	demo.oldcmd = demo.cmd;
	trap_GetUserCmd( trap_GetCurrentCmdNumber(), &demo.cmd );
	demoMoveUpdateAngles();
	demoMoveDeltaCmd();

	if ( demo.seekEnabled ) {
		int delta;
		demo.play.fraction -= demo.serverDeltaTime * 1000 * demo.cmdDeltaAngles[YAW];
		delta = (int)demo.play.fraction;
		demo.play.time += delta;
		demo.play.fraction -= delta;
	
		if (demo.deltaRight) {
			int interval = mov_seekInterval.value * 1000;
			int rem = demo.play.time % interval;
			if (demo.deltaRight > 0) {
                demo.play.time += interval - rem;
			} else {
                demo.play.time -= interval + rem;
			}
			demo.play.fraction = 0;
		}
		if (demo.play.fraction < 0) {
			demo.play.fraction += 1;
			demo.play.time -= 1;
		}
		if (demo.play.time < 0) {
			demo.play.time = demo.play.fraction = 0;
		}
		return;
	}
	switch ( demo.editType ) {
	case editCamera:
		cameraMove();
		break;
	case editChase:
		demoMoveChase();
		break;
/*	case editEffect:
		demoEffectMove();
		break;
*/	case editLine:
		demoMoveLine();
		break;
	}
}

#define RADIUS_LIMIT 701
#define EFFECT_LENGTH 400 //msec

static void VibrateView( const float range, const int eventTime, const int fxTime, const float wc, float scale, vec3_t origin, vec3_t angles) {
	float shadingTime;
	int sign;

	scale *= fx_Vibrate.value;

	if (range > 1 || range < 0) {
		return;
	}
	shadingTime = (float)((float)fxTime / 1000) - (float)((float)EFFECT_LENGTH / 1000);

	sign = fxRandomSign( eventTime );
	origin[2] += shadingTime * shadingTime * sin (shadingTime * M_PI * 23.0f) * range * scale * wc;
	angles[ROLL] += shadingTime * shadingTime * sin (shadingTime * M_PI * 1.642f) * range * scale * wc * 0.7f * sign;
}

static void FX_VibrateView( const float scale, vec3_t origin, vec3_t angles ) {
	if (/*cg_thirdPerson.integer && */cg.eventCoeff > 0 ) {
		float range, oldRange;
		int currentTime = cg.time;

		range = 1 - (cg.eventRadius / RADIUS_LIMIT);
		oldRange = 1 - (cg.eventOldRadius / RADIUS_LIMIT);

		if (cg.eventOldTime > cg.time) {
			cg.eventRadius = cg.eventOldRadius = 0;
			cg.eventTime = cg.eventOldTime = 0;
			cg.eventCoeff = cg.eventOldCoeff = 0;
		}

		if (cg.eventRadius > cg.eventOldRadius
			&& cg.eventOldRadius != 0
			&& (cg.eventOldTime + EFFECT_LENGTH) > cg.eventTime
			&& cg.eventCoeff * range < cg.eventOldCoeff * oldRange) {
			cg.eventRadius = cg.eventOldRadius;
			cg.eventTime = cg.eventOldTime;
			cg.eventCoeff = cg.eventOldCoeff;
		}

		if ((currentTime >= cg.eventTime) && (currentTime <= (cg.eventTime + EFFECT_LENGTH))) {
			int fxTime = currentTime - cg.eventTime;
			range = 1 - (cg.eventRadius / RADIUS_LIMIT);
			VibrateView(range, cg.eventTime, fxTime, cg.eventCoeff, scale, origin, angles);
			cg.eventOldRadius = cg.eventRadius;
			cg.eventOldTime = cg.eventTime;
			cg.eventOldCoeff = cg.eventCoeff;
		}
	}
}

int demoSetupView( void) {
	centity_t *cent = 0;
	vec3_t forward;
	qboolean zoomFix;	//to see disruptor zoom when we are chasing a player

	cg.playerCent = 0;
	demo.viewFocus = 0;
	demo.viewTarget = -1;

	switch (demo.viewType) {
	case viewChase:
		if ( demo.chase.cent && demo.chase.distance < mov_chaseRange.value ) {
			centity_t *cent = demo.chase.cent;
			
			if ( cent->currentState.number < MAX_CLIENTS ) {
				cg.playerCent = cent;
				cg.playerPredicted = cent == &cg_entities[cg.snap->ps.clientNum];
//				if (!cg.playerPredicted ) {
					//Make sure lerporigin of playercent is val
//					CG_CalcEntityLerpPositions( cg.playerCent );
//				}
				cg.renderingThirdPerson = (cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0)
					|| cg.playerCent->playerState->weapon == WP_SABER
					|| cg.playerCent->playerState->weapon == WP_MELEE)
					&& cg.predictedPlayerState.zoomMode == 0;
				CG_DemosCalcViewValues();
//				CG_CalcViewValues();
				// first person blend blobs, done after AnglesToAxis
				if ( !cg.renderingThirdPerson ) {
					CG_DamageBlendBlob();
				}
				VectorCopy( cg.refdef.vieworg, demo.viewOrigin );
				VectorCopy( cg.refdef.viewangles, demo.viewAngles );
				zoomFix = qtrue;
			} else {
				VectorCopy( cent->lerpOrigin, demo.viewOrigin );
				VectorCopy( cent->lerpAngles, demo.viewAngles );
				zoomFix = qfalse;
			}
			demo.viewFov = cg_fov.value;
		} else {
			memset( &cg.refdef, 0, sizeof(refdef_t));
			AngleVectors( demo.chase.angles, forward, 0, 0 );
			VectorMA( demo.chase.origin , -demo.chase.distance, forward, demo.viewOrigin );
			VectorCopy( demo.chase.angles, demo.viewAngles );
			demo.viewFov = cg_fov.value;
			demo.viewTarget = demo.chase.target;
			cg.renderingThirdPerson = qtrue;
			zoomFix = qfalse;
		}
		break;
	case viewCamera:
		memset( &cg.refdef, 0, sizeof(refdef_t));
		VectorCopy( demo.camera.origin, demo.viewOrigin );
		VectorCopy( demo.camera.angles, demo.viewAngles );
		demo.viewFov = demo.camera.fov + cg_fov.value;
		demo.viewTarget = demo.camera.target;
		cg.renderingThirdPerson = qtrue;
		cameraMove();
		zoomFix = qfalse;
		break;
	default:
		return 0;
	}

	demo.viewAngles[YAW]	+= mov_deltaYaw.value;
	demo.viewAngles[PITCH]	+= mov_deltaPitch.value;
	demo.viewAngles[ROLL]	+= mov_deltaRoll.value;

//	trap_FX_VibrateView( 1.0f, demo.viewOrigin, demo.viewAngles );
	if (fx_Vibrate.value > 0
		&& cg.eventCoeff > 0
		&&  demo.viewType == viewCamera
		|| ( demo.viewType == viewChase
		&& ((cg.renderingThirdPerson && demo.chase.distance < mov_chaseRange.value)//cg_thirdPerson.integer
		|| demo.chase.distance > mov_chaseRange.value /*|| demo.chase.target == -1*/))) {
			FX_VibrateView( 107.0f, demo.viewOrigin, demo.viewAngles );
	}
	VectorCopy( demo.viewOrigin, cg.refdef.vieworg );
	AnglesToAxis( demo.viewAngles, cg.refdef.viewaxis );

	if ( demo.viewTarget >= 0 ) {
		centity_t* targetCent = demoTargetEntity( demo.viewTarget );
		if ( targetCent ) {
			vec3_t targetOrigin;
			chaseEntityOrigin( targetCent, targetOrigin );
			//Find distance betwene plane of camera and this target
			demo.viewFocus = DotProduct( cg.refdef.viewaxis[0], targetOrigin ) - DotProduct( cg.refdef.viewaxis[0], cg.refdef.vieworg  );
		}
	}

	cg.refdef.width = cgs.glconfig.vidWidth*cg_viewsize.integer/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*cg_viewsize.integer/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
	if (!zoomFix) {
		cg.refdef.fov_x = demo.viewFov;
		cg.refdef.fov_y = atan2( cg.refdef.height, (cg.refdef.width / tan( demo.viewFov / 360 * M_PI )) ) * 360 / M_PI;
	}
}

extern snapshot_t *CG_ReadNextSnapshot( void );
extern void CG_SetNextSnap( snapshot_t *snap );
extern void CG_TransitionSnapshot( void );

void demoProcessSnapShots( qboolean hadSkip ) {
	int i;
	snapshot_t		*snap;

	// see what the latest snapshot the client system has is
	trap_GetCurrentSnapshotNumber( &cg.latestSnapshotNum, &cg.latestSnapshotTime );
	if (hadSkip || !cg.snap) {
		cgs.processedSnapshotNum = cg.latestSnapshotNum - 2;
		if (cg.nextSnap)
			cgs.serverCommandSequence = cg.nextSnap->serverCommandSequence;
		else if (cg.snap)
			cgs.serverCommandSequence = cg.snap->serverCommandSequence;
		cg.snap = 0;
		cg.nextSnap = 0;

		for (i=-1;i<MAX_GENTITIES;i++) {
			centity_t *cent = i < 0 ? &cg_entities[cg.predictedPlayerState.clientNum] : &cg_entities[i];
//			centity_t *cent = i < 0 ? &cg_entities[cg.snap->ps.clientNum] : &cg_entities[i];
			cent->trailTime = cg.time;
			cent->snapShotTime = cg.time;
			cent->currentValid = qfalse;
			cent->interpolate = qfalse;
			cent->muzzleFlashTime = cg.time - MUZZLE_FLASH_TIME - 1;
			cent->previousEvent = 0;
			if (cent->currentState.eType == ET_PLAYER) {
				memset( &cent->pe, 0, sizeof( cent->pe ) );
				cent->pe.legs.yawAngle = cent->lerpAngles[YAW];
				cent->pe.torso.yawAngle = cent->lerpAngles[YAW];
				cent->pe.torso.pitchAngle = cent->lerpAngles[PITCH];
			}
		}
	}

	/* Check if we have some transition between snapsnots */
	if (!cg.snap) {
		snap = CG_ReadNextSnapshot();
		if (!snap)
			return;
		cg.snap = snap;
		BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, qfalse );
		CG_BuildSolidList();
		CG_ExecuteNewServerCommands( snap->serverCommandSequence );
		for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
			entityState_t *state = &cg.snap->entities[ i ];
			centity_t *cent = &cg_entities[ state->number ];
			memcpy(&cent->currentState, state, sizeof(entityState_t));
			cent->interpolate = qfalse;
			cent->currentValid = qtrue;
			if (cent->currentState.eType > ET_EVENTS)
				cent->previousEvent = 1;
			else 
				cent->previousEvent = cent->currentState.event;
		}
	}
	do {
		if (!cg.nextSnap) {
			snap = CG_ReadNextSnapshot();
			if (!snap)
				break;
			CG_SetNextSnap( snap );
		}
		if ( cg.time >= cg.snap->serverTime && cg.time < cg.nextSnap->serverTime )
			break;
		//Todo our own transition checking if we wanna hear certain sounds
		CG_TransitionSnapshot();
	} while (1);
}

void demoAddViewPos( const char *baseName, const vec3_t origin, const vec3_t angles, float fov ) {
	char dataLine[256];
	char fileName[MAX_OSPATH];
	fileHandle_t fileHandle;

	Com_sprintf( fileName, sizeof( fileName ), "%s.cam", baseName );
	trap_FS_FOpenFile( fileName, &fileHandle, FS_APPEND );
	if (!fileHandle) 
		return;
	Com_sprintf( dataLine, sizeof( dataLine ), "%.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
		origin[0], origin[1], origin[2], angles[0], angles[1], angles[2], fov );
	trap_FS_Write( &dataLine, strlen( dataLine ), fileHandle );
	trap_FS_FCloseFile( fileHandle );
}

extern float cg_linearFogOverride;      // cg_view.c
extern float cg_radarRange;             // cg_draw.c
extern qboolean cg_rangedFogging;

void CG_DemosDrawActiveFrame( int serverTime, stereoFrame_t stereoView ) {
	int deltaTime;
	qboolean hadSkip;
	qboolean captureFrame;
	float captureFPS;
	float frameSpeed;
	int blurTotal, blurIndex;
	float blurFraction;
	qboolean stereoCaptureSkip = qfalse;

	int inwater;
	const char *cstr;
	float mSensitivity = cg.zoomSensitivity;
	float mPitchOverride = 0.0f;
	float mYawOverride = 0.0f;
	static centity_t *veh = NULL;

	if (!demo.initDone) {
		if ( !cg.snap ) {
			demoProcessSnapShots( qtrue );
		}
		if ( !cg.snap ) {
			CG_Error( "No Initial demo snapshot found" );
		}
		demoPlaybackInit();
	}

	if (cgQueueLoad)
	{ //do this before you start messing around with adding ghoul2 refents and crap
		CG_ActualLoadDeferredPlayers();
		cgQueueLoad = qfalse;
	}

	cg.demoPlayback = 2;

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] != 0 ) {
		CG_DrawInformation();
		return;
	}

	captureFrame = demo.capture.active && !demo.play.paused;
	if ( captureFrame ) {
		trap_MME_BlurInfo( &blurTotal, &blurIndex );
		captureFPS = mov_captureFPS.value;
		if ( blurTotal > 0) {
			captureFPS *= blurTotal;
			blurFraction = blurIndex / (float)blurTotal;
		} else {
			blurFraction = 0;
		}
	}

	/* Forward the demo */
	deltaTime = serverTime - demo.serverTime;
	if (deltaTime > 50)
		deltaTime = 50;
	demo.serverTime = serverTime;
	demo.serverDeltaTime = 0.001 * deltaTime;
	cg.oldTime = cg.time;
	cg.oldTimeFraction = cg.timeFraction;

	if (demo.play.time < 0) {
		demo.play.time = demo.play.fraction = 0;
	}

	demo.play.oldTime = demo.play.time;

	/* forward the time a bit till the moment of capture */
	if ( captureFrame && demo.capture.locked && demo.play.time < demo.capture.start) {
		int left = demo.capture.start - demo.play.time;
		if ( left > 2000) {
			left -= 1000;
			captureFrame = qfalse;
		} else if (left > 5) {
			captureFrame = qfalse;
			left = 5;
		}
		demo.play.time += left;
	} else if ( captureFrame && demo.loop.total && blurTotal ) {
		float loopFraction = demo.loop.index / (float)demo.loop.total;
		demo.play.time = demo.loop.start;
		demo.play.fraction = demo.loop.range * loopFraction;
		demo.play.time += (int)demo.play.fraction;
		demo.play.fraction -= (int)demo.play.fraction;
	} else if (captureFrame) {
		float frameDelay = 1000.0f / captureFPS;
		demo.play.fraction += frameDelay * demo.play.speed;
		demo.play.time += (int)demo.play.fraction;
		demo.play.fraction -= (int)demo.play.fraction;
	} else if ( demo.find ) {
		demo.play.time = demo.play.oldTime + 20;
		demo.play.fraction = 0;
		if ( demo.play.paused )
			demo.find = findNone;
	} else if (!demo.play.paused) {
		float delta = demo.play.fraction + deltaTime * demo.play.speed;
		demo.play.time += (int)delta;
		demo.play.fraction = delta - (int)delta;
	}

	//that is used for music, we don't have that (yet)
//	demo.play.lastTime = demo.play.time;

	if ( demo.loop.total && captureFrame && blurTotal ) {
		//Delay till we hit the right part at the start
		int time;
		float timeFraction;
		if ( demo.loop.lineDelay && !blurIndex ) {
			time = demo.loop.start - demo.loop.lineDelay;
			timeFraction = 0;
			if ( demo.loop.lineDelay > 8 )
				demo.loop.lineDelay -= 8;
			else
				demo.loop.lineDelay = 0;
			captureFrame = qfalse;
		} else {
			if ( blurIndex == blurTotal - 1 ) {
				//We'll restart back to the start again
				demo.loop.lineDelay = 2000;
				if ( ++demo.loop.index >= demo.loop.total ) {
					demo.loop.total = 0;
				}
			}
			time = demo.loop.start;
			timeFraction = demo.loop.range * blurFraction;
		}
		time += (int)timeFraction;
		timeFraction -= (int)timeFraction;
		lineAt( time, timeFraction, &demo.line.time, &cg.timeFraction, &frameSpeed );
	} else {
		lineAt( demo.play.time, demo.play.fraction, &demo.line.time, &cg.timeFraction, &frameSpeed );
	}

	/* Set the correct time */
	cg.time = trap_MME_SeekTime( demo.line.time );
	/* cg.time is shifted ahead a bit to correct some issues.. */
//	frameSpeed *= demo.play.speed;

	cg.frametime = (cg.time - cg.oldTime) + (cg.timeFraction - cg.oldTimeFraction);
	if (cg.frametime <0  || cg.frametime > 100) {
		cg.frametime = 0;
		hadSkip = qtrue;
		cg.oldTime = cg.time;
		cg.oldTimeFraction = cg.timeFraction;
		CG_InitLocalEntities();
		CG_InitMarkPolys();
		CG_ClearParticles ();
		trap_FX_Reset( );
//		trap_R_DecalReset();
		trap_R_ClearDecals ( );

		cg.centerPrintTime = 0;
        cg.damageTime = 0;
		cg.powerupTime = 0;
		cg.rewardTime = 0;
		cg.scoreFadeTime = 0;
		cg.lastKillTime = 0;
		cg.attackerTime = 0;
		cg.soundTime = 0;
		cg.itemPickupTime = 0;
		cg.itemPickupBlendTime = 0;
		cg.weaponSelectTime = 0;
		cg.headEndTime = 0;
		cg.headStartTime = 0;
		cg.v_dmg_time = 0;
		trap_S_ClearLoopingSounds();
	} else {
		hadSkip = qfalse;
	}
	/* Make sure the random seed is the same each time we hit this frame */
	srand( cg.time + cg.timeFraction * 1000);

	trap_FX_AdjustTime( cg.time );
	CG_RunLightStyles();
	/* Prepare to render the screen */		
	trap_S_ClearLoopingSounds();
	trap_R_ClearScene();
	/* Update demo related information */
	if (cg.snap && cg.snap->ps.saberLockTime > cg.time) {
		mSensitivity = 0.01f;
	}
	else if (cg.predictedPlayerState.weapon == WP_EMPLACED_GUN) { //lower sens for emplaced guns and vehicles
		mSensitivity = 0.2f;
	}
	if (cg.predictedPlayerState.m_iVehicleNum) {
		veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
	}
	if (veh &&
		veh->currentState.eType == ET_NPC &&
		veh->currentState.NPC_class == CLASS_VEHICLE &&
		veh->m_pVehicle &&
		veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER &&
		bg_fighterAltControl.integer) {
		trap_SetUserCmdValue( cg.weaponSelect, mSensitivity, mPitchOverride, mYawOverride, 0.0f, cg.forceSelect, cg.itemSelect, qtrue );
		veh = NULL; //this is done because I don't want an extra assign each frame because I am so perfect and super efficient.
	} else {
		trap_SetUserCmdValue( cg.weaponSelect, mSensitivity, mPitchOverride, mYawOverride, 0.0f, cg.forceSelect, cg.itemSelect, qfalse );
	}
	demoProcessSnapShots( hadSkip );
	trap_ROFF_UpdateEntities();

	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	CG_DemosUpdatePlayer( );
	chaseUpdate( demo.play.time, demo.play.fraction );
	cameraUpdate( demo.play.time, demo.play.fraction );

	cg.clientFrame++;
//	CG_InterpolatePlayerState( qfalse );
	CG_PredictPlayerState();
	BG_PlayerStateToEntityState( &cg.predictedPlayerState, &cg_entities[cg.snap->ps.clientNum].currentState, qfalse );


	if (cg.snap->ps.stats[STAT_HEALTH] > 0) {
		if (cg.predictedPlayerState.weapon == WP_EMPLACED_GUN && cg.predictedPlayerState.emplacedIndex /*&&
			cg_entities[cg.predictedPlayerState.emplacedIndex].currentState.weapon == WP_NONE*/) { //force third person for e-web and emplaced use
			cg.renderingThirdPerson = 1;
		} else if (cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE ||
			BG_InGrappleMove(cg.predictedPlayerState.torsoAnim) || BG_InGrappleMove(cg.predictedPlayerState.legsAnim) ||
			cg.predictedPlayerState.forceHandExtend == HANDEXTEND_KNOCKDOWN || cg.predictedPlayerState.fallingToDeath ||
			cg.predictedPlayerState.m_iVehicleNum || PM_InKnockDown(&cg.predictedPlayerState)) {
			if (cg_fpls.integer && cg.predictedPlayerState.weapon == WP_SABER) { //force to first person for fpls
				cg.renderingThirdPerson = 0;
			} else {
				cg.renderingThirdPerson = 1;
			}
		} else if (cg.snap->ps.zoomMode) { //always force first person when zoomed
			cg.renderingThirdPerson = 0;
		}
	}
	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR) { //always first person for spec
		cg.renderingThirdPerson = 0;
	}
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		cg.renderingThirdPerson = 0;
	}

	cg_entities[cg.snap->ps.clientNum].currentValid = qtrue;
	VectorCopy( cg_entities[cg.snap->ps.clientNum].currentState.pos.trBase, cg_entities[cg.snap->ps.clientNum].lerpOrigin );
	VectorCopy( cg_entities[cg.snap->ps.clientNum].currentState.apos.trBase, cg_entities[cg.snap->ps.clientNum].lerpAngles );

//	inwater = CG_CalcViewValues();
	inwater = demoSetupView();
	CG_SetupFrustum();

	if (cg_linearFogOverride) {
		trap_R_SetRangeFog(-cg_linearFogOverride);
	} else if (demo.viewType == viewCamera || ( demo.viewType == viewChase && (cg.renderingThirdPerson || demo.chase.distance > mov_chaseRange.value))) {
		trap_R_SetRangeFog(0.0f);
	} else if (cg.predictedPlayerState.zoomMode) { //zooming with binoculars or sniper, set the fog range based on the zoom level -rww
		cg_rangedFogging = qtrue;
		//smaller the fov the less fog we have between the view and cull dist
		trap_R_SetRangeFog(cg.refdef.fov_x*64.0f);
	} else if (cg_rangedFogging) { //disable it
		cg_rangedFogging = qfalse;
		trap_R_SetRangeFog(0.0f);
	}

	cstr = CG_ConfigString(CS_SKYBOXORG);
	if (cstr && cstr[0]) { //we have a skyportal
		CG_DrawSkyBoxPortal(cstr);
	}

	CG_CalcScreenEffects();
	if ( !cg.renderingThirdPerson && cg.predictedPlayerState.pm_type != PM_SPECTATOR ) {
		CG_DamageBlendBlob();
	}
	if ( !cg.hyperspace ) {
		CG_AddPacketEntities(qfalse);			// adter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		CG_AddParticles ();
		CG_AddLocalEntities();
	}
	if ( !cg.hyperspace) {
		trap_FX_AddScheduledEffects(qfalse);
	}

//	if ( cg.playerCent == &cg_entities[cg.predictedPlayerState.clientNum] ) {
	if ( cg.playerCent == &cg_entities[cg.snap->ps.clientNum] ) {
		CG_PlayBufferedSounds();
		// warning sounds when powerup is wearing off
		CG_PowerupTimerSounds();
		CG_AddViewWeapon( &cg.predictedPlayerState  );
	} else if ( cg.playerCent && cg.playerCent->currentState.number < MAX_CLIENTS )  {
//		CG_AddViewWeapon( &cg.predictedPlayerState );
//		CG_AddViewWeaponDirect( cg.playerCent );	//uncomment when fixed, now all clieants have weapon from recorded client
	}

	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	/* Render some extra demo related stuff */
	if (!captureFrame) {
		switch (demo.editType) {
		case editCamera:
			cameraDraw( demo.play.time, demo.play.fraction );
			break;
		case editChase:
			chaseDraw( demo.play.time, demo.play.fraction );
			break;
//		case editEffect:
//			demoEffectDraw( demo.play.time, demo.play.fraction );
//			break;
		}
		/* Add bounding boxes for easy aiming */
		if ( demo.editType && ( demo.cmd.buttons & BUTTON_ATTACK) && ( demo.cmd.buttons & BUTTON_ALT_ATTACK)  ) {
			int i;
			centity_t *targetCent;
			for (i = 0;i<MAX_GENTITIES;i++) {
	            targetCent = demoTargetEntity( i );
				if (targetCent) {
					vec3_t container, traceStart, traceImpact, forward;
					const float *color;

					demoCentityBoxSize( targetCent, container );
					VectorSubtract( demo.viewOrigin, targetCent->lerpOrigin, traceStart );
					AngleVectors( demo.viewAngles, forward, 0, 0 );
					if (BoxTraceImpact( traceStart, forward, container, traceImpact )) {
						color = colorRed;
					} else {
						color = colorYellow;
					}
					demoDrawBox( targetCent->lerpOrigin, container, color );
				}
			}

		}
/*		if ( mov_gridStep.value > 0 && mov_gridRange.value > 0) {
			vec4_t color;
			vec3_t offset;
			qhandle_t shader = trap_R_RegisterShader( "mme/gridline" );
			color[0] = color[1] = color[2] = 1;
			color[3] = 0;
			offset[0] = offset[1] = offset[2] = 0;
			Q_parseColor( mov_gridColor.string, ospColors, color );
			demoDrawGrid( demo.viewOrigin, color, offset, mov_gridWidth.value, mov_gridStep.value, mov_gridRange.value, shader );
		}
*/	}

//	CG_PowerupTimerSounds();
	CG_UpdateSoundTrackers();

	//do we need this?
	if (gCGHasFallVector) {
		vec3_t lookAng;
		//broken fix
		cg.renderingThirdPerson = qtrue;

		VectorSubtract(cg.snap->ps.origin, cg.refdef.vieworg, lookAng);
		VectorNormalize(lookAng);
		vectoangles(lookAng, lookAng);

		VectorCopy(gCGFallVector, cg.refdef.vieworg);
		AnglesToAxis(lookAng, cg.refdef.viewaxis);
	}

	//This is done from the vieworg to get origin for non-attenuated sounds
	cstr = CG_ConfigString( CS_GLOBAL_AMBIENT_SET );
	if (cstr && cstr[0]) {
		trap_S_UpdateAmbientSet( cstr, cg.refdef.vieworg );
	}

	//we don't have modified sound code, so we don't send sound scale
//	if (frameSpeed > 5)
//		frameSpeed = 6;
//	else frameSpeed += 1;
//	trap_S_Respatialize( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, *((int *)&frameSpeed));
	trap_S_Respatialize( cg.playerCent ? cg.playerCent->currentState.number : ENTITYNUM_NONE, cg.refdef.vieworg, cg.refdef.viewaxis, inwater);

	if ( demo.viewType == viewChase && cg.playerCent && ( cg.playerCent->currentState.number < MAX_CLIENTS ) ) {
		CG_Draw2D();
	} else if (cg_draw2D.integer) {
//		CG_DrawFPS( 0 );
		CG_DrawUpperRight();
	}

	CG_DrawActive( stereoView );

	CG_DrawAutoMap();

	if (captureFrame) {
		char fileName[MAX_OSPATH];
		float stereo = CG_Cvar_Get("r_stereoSeparation");
		Com_sprintf( fileName, sizeof( fileName ), "capture/%s/%s", mme_demoFileName.string, mov_captureName.string );
		if (stereo == 0) {
			trap_MME_Capture( fileName, captureFPS, demo.viewFocus );
		} else {
			//we can capture stereo 3d only through cl_main
			trap_Cvar_Set("cl_mme_name", fileName);
			trap_Cvar_Set("cl_mme_fps", va( "%f", captureFPS ));
			trap_Cvar_Set("cl_mme_focus", va( "%f", demo.viewFocus ));
			trap_Cvar_Set("mme_name", fileName);
			trap_Cvar_Set("mme_fps", va( "%f", captureFPS ));
			trap_Cvar_Set("mme_focus", va( "%f", demo.viewFocus ));
			trap_Cvar_Set("cl_mme_capture", "1");
		}
//		trap_Cvar_Set("cl_aviFrameRate", va( "%f", mov_captureFPS.value ));	//CL_AVI
//		trap_SendConsoleCommand( "video" );
		if ( mov_captureCamera.integer )
			demoAddViewPos( fileName, demo.viewOrigin, demo.viewAngles, demo.viewFov );
	} else {
		trap_Cvar_Set("cl_mme_capture", "0");
//		if (demo.editType)
//			demoDrawCrosshair();
		hudDraw();
	}

	if ( demo.capture.active && demo.capture.locked && demo.play.time > demo.capture.end  ) {
		Com_Printf( "Capturing ended\n" );
		if (demo.autoLoad) {
			trap_SendConsoleCommand( "disconnect\n" );
		} 
		demo.capture.active = qfalse;
	}
}

void CG_DemosAddLog(const char *fmt, ...) {
	char *dest;
	va_list		argptr;

	demo.log.lastline++;
	if (demo.log.lastline >= LOGLINES)
		demo.log.lastline = 0;
	
	demo.log.times[demo.log.lastline] = demo.serverTime;
	dest = demo.log.lines[demo.log.lastline];

	va_start ( argptr, fmt );
	_vsnprintf( dest, sizeof(demo.log.lines[0]), fmt, argptr );
	va_end (argptr);
//	Com_Printf("%s\n", dest);
}

static void demoViewCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "chase")) {
		demo.viewType = viewChase;
	} else if (!Q_stricmp(cmd, "camera")) {
		demo.viewType = viewCamera;
	} else if (!Q_stricmp(cmd, "prev")) {
		if (demo.viewType == 0)
			demo.viewType = viewLast - 1;
		else 
			demo.viewType--;
	} else if (!Q_stricmp(cmd, "next")) {
		demo.viewType++;
		if (demo.viewType >= viewLast)
			demo.viewType = 0;
	} else {
		Com_Printf("view usage:\n" );
		Com_Printf("view camera, Change to camera view.\n" );
		Com_Printf("view chase, Change to chase view.\n" );
//		Com_Printf("view effect, Change to effect view.\n" );
//		Com_Printf("view follow, Change to follow view.\n" );
		Com_Printf("view next/prev, Change to next or previous view.\n" );
		return;
	}

	switch (demo.viewType) {
	case viewCamera:
		CG_DemosAddLog("View set to camera" );
		break;
	case viewChase:
		CG_DemosAddLog("View set to chase" );
		break;
//	case viewEffect:
//		CG_DemosAddLog("View set to effect" );
//		break;
	}
}

static void demoEditCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "none"))  {
		demo.editType = editNone;
		CG_DemosAddLog("Not editing anything");
	} else if (!Q_stricmp(cmd, "chase")) {
		if ( demo.cmd.upmove > 0 ) {
			demoViewCommand_f();
			return;
		}
		demo.editType = editChase;
		CG_DemosAddLog("Editing chase view");
	} else if (!Q_stricmp(cmd, "camera")) {
		if ( demo.cmd.upmove > 0 ) {
			demoViewCommand_f();
			return;
		}
		demo.editType = editCamera;
		CG_DemosAddLog("Editing camera");
	} else if (!Q_stricmp(cmd, "line")) {
		if ( demo.cmd.upmove > 0 ) {
			demoViewCommand_f();
			return;
		}
		demo.editType = editLine;
		CG_DemosAddLog("Editing timeline");
/*	} else if (!Q_stricmp(cmd, "script")) {
		demo.editType = editScript;
		CG_DemosAddLog("Editing script");
	} else if (!Q_stricmp(cmd, "effect")) {
		if ( demo.cmd.upmove > 0 ) {
			demoViewCommand_f();
			return;
		}
		demo.editType = editEffect;
		CG_DemosAddLog("Editing Effect");
*/	} else {
		switch ( demo.editType ) {
		case editCamera:
			demoCameraCommand_f();
			break;
		case editChase:
			demoChaseCommand_f();
			break;
		case editLine:
			demoLineCommand_f();
			break;
/*		case editScript:
			demoScriptCommand_f();
			break;
		case editEffect:
			demoEffectCommand_f();
			break;
*/		}
	}
}

static void demoSeekTwoCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (isdigit( cmd[0] )) {
		//teh's parser for time MM:SS.MSEC, thanks *bow*
		int i;
		char *sec, *min;;
		min = (char *)cmd;
		for( i=0; min[i]!=':'&& min[i]!=0; i++ );
		if(cmd[i]==0)
			sec = 0;
		else
		{
			min[i] = 0;
			sec = min+i+1;
		}
		demo.play.time = ( atoi( min ) * 60000 + ( sec ? atof( sec ) : 0 ) * 1000 );
		demo.play.fraction = 0;
	}
}

static void demoSeekCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (cmd[0] == '+') {
		if (isdigit( cmd[1])) {
			demo.play.time += atof( cmd + 1 ) * 1000;
			demo.play.fraction = 0;
		}
	} else if (cmd[0] == '-') {
		if (isdigit( cmd[1])) {
			demo.play.time -= atof( cmd + 1 ) * 1000;
			demo.play.fraction = 0;
		}
	} else if (isdigit( cmd[0] )) {
		demo.play.time = atof( cmd ) * 1000;
		demo.play.fraction = 0;
	}
}

void demoPlaybackInit(void) {
	char projectFile[MAX_OSPATH];

	demo.initDone = qtrue;
	demo.autoLoad = qfalse;
	demo.play.time = 0;
	demo.play.lastTime = 0;
	demo.play.fraction = 0;
	demo.play.speed = 1.0f;
	demo.play.paused = 0;

	demo.move.acceleration = 8;
	demo.move.friction = 8;
	demo.move.speed = 400;

	demo.line.locked = qfalse;
	demo.line.offset = 0;
	demo.line.speed = 1.0f;
	demo.line.points = 0;

	demo.loop.total = 0;

	demo.editType = editCamera;
	demo.viewType = viewChase;

	demo.camera.flags = CAM_ORIGIN | CAM_ANGLES;
	demo.camera.target = -1;
	demo.camera.fov = 0;
	demo.camera.smoothPos = posBezier;
	demo.camera.smoothAngles = angleQuat;

	VectorClear( demo.chase.origin );
	VectorClear( demo.chase.angles );
	VectorClear( demo.chase.velocity );
	demo.chase.distance = 0;
	demo.chase.locked = qfalse;
	demo.chase.target = -1;

	hudInitTables();

	trap_AddCommand("camera");
	trap_AddCommand("edit");
	trap_AddCommand("view");
	trap_AddCommand("chase");
	trap_AddCommand("speed");
	trap_AddCommand("seek");
	trap_AddCommand("demoSeek");
	trap_AddCommand("pause");
	trap_AddCommand("capture");
	trap_AddCommand("hudInit");
	trap_AddCommand("hudToggle");
	trap_AddCommand("line");
	trap_AddCommand("save");
	trap_AddCommand("load");
	trap_AddCommand("+seek");
	trap_AddCommand("-seek");

	demo.media.additiveWhiteShader = trap_R_RegisterShader( "mme_additiveWhite" );
	demo.media.mouseCursor = trap_R_RegisterShaderNoMip( "mme_cursor" );
	demo.media.switchOn = trap_R_RegisterShaderNoMip( "mme_message_on" );
	demo.media.switchOff = trap_R_RegisterShaderNoMip( "mme_message_off" );

	trap_SendConsoleCommand("exec mmedemos.cfg\n");
	trap_Cvar_Set( "mov_captureName", "" );
	trap_Cvar_VariableStringBuffer( "mme_demoStartProject", projectFile, sizeof( projectFile ));
	if (projectFile[0]) {
		trap_Cvar_Set( "mme_demoStartProject", "" );
		demo.autoLoad = demoProjectLoad( projectFile );
		if (demo.autoLoad) {
			if (!demo.capture.start && !demo.capture.end) {
				trap_Error( "Loaded project file with empty capture range\n");
			}
			/* Check if the project had a cvar for the name else use project */
			if (!mov_captureName.string[0]) {
				trap_Cvar_Set( "mov_captureName", projectFile );
				trap_Cvar_Update( &mov_captureName );
			}
			trap_SendConsoleCommand("exec mmelist.cfg\n");
			demo.play.time = demo.capture.start - 1000;
			demo.capture.locked = qtrue;
			demo.capture.active = qtrue;
		} else {
			trap_Error( va("Couldn't load project %s\n", projectFile ));
		}
	}
}

qboolean CG_DemosConsoleCommand( void ) {
	const char *cmd = CG_Argv(0);
	if (!Q_stricmp(cmd, "camera")) {
		demoCameraCommand_f();
	} else if (!Q_stricmp(cmd, "view")) {
		demoViewCommand_f();
	} else if (!Q_stricmp(cmd, "edit")) {
		demoEditCommand_f();
	} else if (!Q_stricmp(cmd, "capture")) {
		demoCaptureCommand_f();
	} else if (!Q_stricmp(cmd, "seek")) {
		demoSeekCommand_f();
	} else if (!Q_stricmp(cmd, "demoSeek")) {
		demoSeekTwoCommand_f();
	} else if (!Q_stricmp(cmd, "speed")) {
		cmd = CG_Argv(1);
		if (cmd[0]) {
			demo.play.speed = atof(cmd);
		}
		CG_DemosAddLog("Play speed %f", demo.play.speed );
	} else if (!Q_stricmp(cmd, "pause")) {
		demo.play.paused = !demo.play.paused;
		if ( demo.play.paused )
			demo.find = findNone;
	} else if (!Q_stricmp(cmd, "chase")) {
		demoChaseCommand_f();
	} else if (!Q_stricmp(cmd, "hudInit")) {
		hudInitTables();
	} else if (!Q_stricmp(cmd, "hudToggle")) {
		hudToggleInput();
	} else if (!Q_stricmp(cmd, "+seek")) {
		demo.seekEnabled = qtrue;
	} else if (!Q_stricmp(cmd, "-seek")) {
		demo.seekEnabled = qfalse;
	} else if (!Q_stricmp(cmd, "line")) {
		demoLineCommand_f();
	} else if (!Q_stricmp(cmd, "load")) {
		demoLoadCommand_f();
	} else if (!Q_stricmp(cmd, "save")) {
		demoSaveCommand_f();
	} else {
		return CG_ConsoleCommand();
	}
	return qtrue;
}
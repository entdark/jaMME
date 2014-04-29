#include "cg_demos.h" 
#include "ghoul2/G2.h"

#ifdef DEMO_ANIM

/* vertical representation
! - does not exist in default kyle model
static char *boneList[72] = {
"model_root",
  |-"pelvis",
	  |-"Motion",
	  |
	  |-"lfemurYZ",
	  |	  |-"lfemurX",
	  |	  |
	  |	  |-"ltibia",
	  |	  |   |-"ltalus",
	  |	  | 
	  |	  |-"ltarsal", //"ltail"?
	  |	
	  |-"rfemurYZ",
	  |	  |-"rfemurX",
	  |	  | 
	  |	  | "rtibia",
	  |	  |   |-"rtalus",
	  |	  | 
	  |	  |-"rtarsal", //"rtail"?
	  |	
	  |-"lower_lumbar",
		  |-"upper_lumbar",
			  |-"thoracic",
				  |-"cervical",
				  |	  |-"cranium",
				  |		  |-"face_always_",//"face"?
				  |			  |-"ceyebrow",
				  |			  |-"jaw",
				  |			  |-"lblip2",
				  |			  |-"leye",
				  |			  |-"rblip2",
				  |			  |-"ltlip2",
				  |			  |-"rtlip2",
				  |			  |-"reye",
				  |
				  |-"rclavical",
				  |
				  |-"rhumerus",
				  |	  |-"rhumerusX",
				  |   |
				  |	  |-"rradius",
				  |		  |-"rradiusX",
				  |		  |-"rhand",
				  |		  |-"mc7",			!
				  |		  |-"r_d5_j1",		!
				  |		  |-"r_d5_j2",		!
				  |		  |-"r_d5_j3",		!
				  |		  |-"r_d1_j1",
				  |		  |-"r_d1_j2",
				  |		  |-"r_d1_j3",		!
				  |		  |-"r_d2_j1",
				  |		  |-"r_d2_j2",
				  |		  |-"r_d2_j3",		!
				  |		  |-"r_d3_j1",		!
				  |		  |-"r_d3_j2",		!
				  |		  |-"r_d3_j3",		!
				  |		  |-"r_d4_j1",
				  |		  |-"r_d4_j2",
				  |		  |-"r_d4_j3",		!
				  |		  |-"rhang_tag_bone",
				  |
				  |-"lclavical",
				  |
				  |-"lhumerus",
					  |-"lhumerusX",
					  |
					  |-"lradius",
						  |-"lradiusX",
						  |-"lhand",
						  |-"mc5",			!
						  |-"l_d5_j1",		!
						  |-"l_d5_j2",		!
						  |-"l_d5_j3",		!
						  |-"l_d4_j1",
						  |-"l_d4_j2",
						  |-"l_d4_j3",		!
						  |-"l_d3_j1",		!
						  |-"l_d3_j2",		!
						  |-"l_d3_j3",		!
						  |-"l_d2_j1",
						  |-"l_d2_j2",
						  |-"l_d2_j3",		!
						  |-"l_d1_j1",
						  |-"l_d1_j2",
						  |-"l_d1_j3",		!
};
//vertical representation */

const char *boneList[72] = {
	"model_root","pelvis","Motion","lfemurYZ","lfemurX","ltibia","ltalus",/*"ltarsal"*/"ltail",
	"rfemurYZ","rfemurX","rtibia","rtalus",/*"rtarsal"*/"rtail","lower_lumbar","upper_lumbar","thoracic",
	"cervical","cranium","ceyebrow","jaw","lblip2","leye","rblip2","ltlip2",
	"rtlip2","reye","rclavical","rhumerus","rhumerusX","rradius","rradiusX","rhand",
	"mc7","r_d5_j1","r_d5_j2","r_d5_j3","r_d1_j1","r_d1_j2","r_d1_j3","r_d2_j1",
	"r_d2_j2","r_d2_j3","r_d3_j1","r_d3_j2","r_d3_j3","r_d4_j1","r_d4_j2","r_d4_j3",
	"rhang_tag_bone","lclavical","lhumerus","lhumerusX","lradius","lradiusX","lhand","mc5",
	"l_d5_j1","l_d5_j2","l_d5_j3","l_d4_j1","l_d4_j2","l_d4_j3","l_d3_j1","l_d3_j2",
	"l_d3_j3","l_d2_j1","l_d2_j2","l_d2_j3","l_d1_j1","l_d1_j2","l_d1_j3",/*"face_always_"*/"face",
};

static demoAnimPoint_t *animPointAlloc(void) {
	demoAnimPoint_t *point = (demoAnimPoint_t *)malloc(sizeof (demoAnimPoint_t));
	if (!point)
		CG_Error("Out of memory");
	memset(point, 0, sizeof(demoAnimPoint_t));
	return point;
}

static void animPointFree(demoAnimPoint_t *point, int target) {
	if (point->prev) 
		point->prev->next = point->next;
	else 
		demo.anim.points[target] = point->next;
	if (point->next) 
		point->next->prev = point->prev;
	free(point);
}

demoAnimPoint_t *animPointSynch(int time, int target) {
	demoAnimPoint_t *point = target >= 0 ? demo.anim.points[target] : 0;
	if (!point)
		return 0;
	for( ;point->next && point->next->time <= time; point = point->next);
	return point;
}

static qboolean animPointAdd(int time, vec3_t angles[MAX_BONES], int target) {
	demoAnimPoint_t *point = animPointSynch(time, target);
	demoAnimPoint_t *newPoint;
	int i;
	if (target < 0)
		return qfalse;
	if (!point || point->time > time) {
		newPoint = animPointAlloc();
		newPoint->next = demo.anim.points[target];
		if (demo.anim.points[target])
			demo.anim.points[target]->prev = newPoint;
		demo.anim.points[target] = newPoint;
		newPoint->prev = 0;
	} else if (point->time == time) {
		newPoint = point;
	} else {
		newPoint = animPointAlloc();
		newPoint->prev = point;
		newPoint->next = point->next;
		if (point->next)
			point->next->prev = newPoint;
		point->next = newPoint;
	}
	newPoint->time = time;
	for (i = 0; i < MAX_BONES; i++)
		VectorCopy(angles[i], newPoint->angles[i]);
	return qtrue;
}

static qboolean animPointDel(int time, int target) {
	demoAnimPoint_t *point = animPointSynch(time, target);
	if (target < 0)
		return qfalse;
	if (!point || !point->time == time)
		return qfalse;
	animPointFree(point, target);
	return qtrue;
}

static void animPointTimes(demoAnimPoint_t *point, int times[4]) {
	if (!point->next)
		return;
	times[1] = point->time;
	times[2] = point->next->time;
	if (!point->prev) {
		times[0] = times[1] - (times[2] - times[1]);
	} else {
		times[0] = point->prev->time;
	}
	if (!point->next->next) {
		times[3] = times[2] + (times[2] - times[1]);
	} else {
		times[3] = point->next->next->time;
	}
}

static void animPointControl(demoAnimPoint_t *point, Quat_t quats[4], int b) {
	vec3_t tempAngles;

	if (!point->next)
		return;
	
	QuatFromAngles( point->angles[b], quats[1] );
	QuatFromAnglesClosest( point->next->angles[b], quats[1], quats[2] );
	if (!point->prev) {
		VectorSubtract( point->next->angles[b], point->angles[b], tempAngles );
		VectorAdd( tempAngles, point->angles[b], tempAngles );
		QuatFromAnglesClosest( tempAngles, quats[1], quats[0] );
	} else {
		QuatFromAnglesClosest( point->prev->angles[b], quats[1], quats[0] );
	}
	if (!point->next->next) {
		VectorSubtract( point->next->angles[b], point->angles[b], tempAngles );
		VectorAdd( tempAngles, point->next->angles[b], tempAngles );
		QuatFromAnglesClosest( tempAngles, quats[2], quats[3] );
	} else {
		QuatFromAnglesClosest( point->next->next->angles[b], quats[2], quats[3] );
	}
}

qboolean demoAnimBone(int bone) {
	centity_t	*cent = &cg_entities[demo.anim.target];	
	if (!(cent && cent->currentValid))
		return;
	//check if it exists and if so then select this bone
	if (trap_G2API_AddBolt(cg_entities[demo.anim.target].ghoul2, 0, boneList[bone]) == -1)
		return qfalse;
	
	return qtrue;
}

static qboolean animPrevBone(int *oldBone) {
	int i, old = *oldBone;
	if (old < MAX_BONES) {
		if ( old < 0) {
			i = MAX_BONES - 1;
		} else {
			i = old - 1;
		}
		for (; i >= 0; i--) {
			if (demoAnimBone(i)) {
				*oldBone = i;
				return qtrue;
			}
		}
	} else {
		old = -1;
	}
	for (i = MAX_BONES-1; i > old; i--) {
		if (demoAnimBone(i)) {
			*oldBone = i;
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean animNextBone(int *oldBone) {
	int i, old = *oldBone;
	if (old < (MAX_BONES - 2)) {
		if ( old < 0) {
			i = 0;
		} else {
			i = old + 1;
		}
		for (; i < MAX_BONES; i++) {
			if (demoAnimBone(i)) {
				*oldBone = i;
				return qtrue;
			}
		}
	} else {
		old = MAX_BONES;
	}
	for (i = 0; i < old; i++) {
		if (demoAnimBone(i)) {
			*oldBone = i;
			return qtrue;
		}
	}
	return qfalse;
}

qboolean animPrevTarget(int *oldTarget) {
	int i, old = *oldTarget;
	if (old < MAX_CLIENTS) {
		if ( old < 0) {
			i = MAX_CLIENTS - 1;
		} else {
			i = old - 1;
		}
		for (; i >= 0; i--) {
			if (demoTargetEntity(i)) {
				*oldTarget = i;
				return qtrue;
			}
		}
	} else {
		old = -1;
	}
	for (i = MAX_CLIENTS-1; i > old; i--) {
		if (demoTargetEntity(i)) {
			*oldTarget = i;
			return qtrue;
		}
	}
	return qfalse;
}

qboolean animNextTarget(int *oldTarget) {
	int i, old = *oldTarget;
	if (old < (MAX_CLIENTS - 2)) {
		if ( old < 0) {
			i = 0;
		} else {
			i = old + 1;
		}
		for (; i < MAX_CLIENTS; i++) {
			if (demoTargetEntity(i)) {
				*oldTarget = i;
				return qtrue;
			}
		}
	} else {
		old = MAX_CLIENTS;
	}
	for (i = 0; i < old; i++) {
		if (demoTargetEntity(i)) {
			*oldTarget = i;
			return qtrue;
		}
	}
	return qfalse;
}

static void animInterpolate(int time, float timeFraction, vec3_t *angles, int target) {
	float	lerp;
	int		times[4];
	int		bone;
	Quat_t	quats[4], qr;
	demoAnimPoint_t *point = animPointSynch(time, target);

	if (!point->next) {
		angles = point->angles;
		return;
	}
	animPointTimes(point, times);
	lerp = ((time - point->time) + timeFraction) / (point->next->time - point->time);

	for (bone = 0; bone < MAX_BONES; bone++) {
		animPointControl(point, quats, bone);
		QuatTimeSpline(lerp, times, quats, qr);
		QuatToAngles(qr, angles[bone]);
	}
}

static void animGetAngles(vec3_t angles[MAX_BONES], int client) {
	int			i;
	centity_t	*cent = &cg_entities[client];
	
	if (!(cent && cent->currentValid))
		return;
	
	for (i = 1; i < MAX_BONES; i++) {
		mdxaBone_t boltMatrix;
		int bolt = trap_G2API_AddBolt(cent->ghoul2, 0, boneList[i]);
		vec3_t convertedMatrix[3], ang;

		trap_G2API_GetBoltMatrix_NoRecNoRot(cent->ghoul2, 0, bolt, &boltMatrix, vec3_origin, vec3_origin, cg.time, cgs.gameModels, cent->modelScale);

		VectorSet(convertedMatrix[0], boltMatrix.matrix[0][0], boltMatrix.matrix[1][0], boltMatrix.matrix[2][0]);
		VectorSet(convertedMatrix[1], boltMatrix.matrix[0][1], boltMatrix.matrix[1][1], boltMatrix.matrix[2][1]);
		VectorSet(convertedMatrix[2], boltMatrix.matrix[0][2], boltMatrix.matrix[1][2], boltMatrix.matrix[2][2]);

		AxisToAngles(convertedMatrix, ang);
		ang[0] -= 180;
		VectorCopy(ang, angles[i]);
	}
}

static void animSetAngles(const vec3_t angles[MAX_BONES], const int client) {
	int			i;
	centity_t	*cent = &cg_entities[client];
	//flags is #define BONE_ANGLES_REPLACE 0x0004, should be 0? (lazy to share the constant here)
	//bone max amount is amount of all bones, should increment till found amount of bones (if we ever search them)
	//modellist is cgs.gameModels, should be another?
	//up, right and forward are POSITIVE_X, NEGATIVE_Y and NEGATIVE_Z, should be another?

	// never ever do that again, 0x0004 when you know it's BONE_ANGLES_REPLACE. =p
	// I also did this line below, but i don't think it's needed try this.
	//trap_G2API_SetBoneAngles(cent->ghoul2, 0, boneList[0], angles[0], 0, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.gameModels, 0, cg.time);
	if (!(cent && cent->currentValid))
		return;
	
	for (i = 1; i < MAX_BONES; i++) {
		trap_G2API_SetBoneAngles(cent->ghoul2, 0, boneList[i], angles[i], BONE_ANGLES_MME_DELTA, POSITIVE_Z, POSITIVE_Y, POSITIVE_X, cgs.gameModels, 0, cg.time);
	}
}

void animUpdate(int time, float timeFraction) {
	int		i;
	vec3_t	*angles;
	if (!demo.anim.locked)
		return;
	angles = (vec3_t *)malloc(sizeof(vec3_t)*MAX_BONES);
	memset(angles, 0, sizeof(vec3_t)*MAX_BONES);
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (demo.anim.points[i]) {
			if (!demo.anim.override[i] || !cg_entities[i].currentValid)
				continue;
			animInterpolate(time, timeFraction, angles, i);
			animSetAngles(angles, i);
		}
	}
}

void animBoneVectors(vec3_t vec[MAX_BONES], int flags, int client) {
	int			i;
	centity_t	*cent = &cg_entities[client];
	vec3_t		lerpAngles;

	if (!(cent && cent->currentValid))
		return;

	VectorCopy(cent->lerpAngles, lerpAngles);
	lerpAngles[0] = 0.0f;
	for (i = 0; i < MAX_BONES; i++) {
		mdxaBone_t boltMatrix;
		int bolt;
		if (!demoAnimBone(i))
			continue;
		bolt = trap_G2API_AddBolt(cent->ghoul2, 0, boneList[i]);
		trap_G2API_GetBoltMatrix(cent->ghoul2, 0, bolt, &boltMatrix, lerpAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, flags, vec[i]);
	}
}

static void animSkeletonDraw(const vec3_t origins[MAX_BONES], const vec3_t ang[MAX_BONES]) {
	int i;

	for (i = 0; i < MAX_BONES; i++) {
		float scale = i == demo.anim.bone ? 7.0f : 2.3f; //selected bone is bigger
		vec3_t origin2, forward, right, up, ang2;
		AngleVectors(ang[i], forward, right, up);

		VectorMA(origins[i], scale, forward, origin2);
		demoDrawLine(origins[i], origin2, colorYellow);
		VectorMA(origins[i], scale, right, origin2);
		demoDrawLine(origins[i], origin2, colorMagenta);
		VectorMA(origins[i], scale, up, origin2);
		demoDrawLine(origins[i], origin2, colorCyan);
	}
}

void animDraw(int time, float timeFraction) {
	if (demo.anim.target >= 0 && demo.anim.override[demo.anim.target]
	&& cg_entities[demo.anim.target].currentValid && demo.anim.bone >= 0) {
		int i, client = demo.anim.target;
		vec3_t container = {-1.0f, 1.0f, 1.0f};
		vec3_t axis[3][MAX_BONES], origins[MAX_BONES], ang[MAX_BONES];

		animBoneVectors(origins, ORIGIN, client);
		animBoneVectors(axis[0], POSITIVE_Y, client);
		animBoneVectors(axis[1], POSITIVE_Z, client);
		animBoneVectors(axis[2], POSITIVE_X, client);
		for (i = 0; i < MAX_BONES; i++) {
			vec3_t axis2[3];
			VectorCopy(axis[0][i], axis2[0]);
			VectorCopy(axis[1][i], axis2[1]);
			VectorCopy(axis[2][i], axis2[2]);
			AxisToAngles(axis2, ang[i]);
		}
		demo.anim.drawing = qtrue;
		animSkeletonDraw(origins, ang);
		demoDrawBox( origins[demo.anim.bone], container, colorWhite );
		demo.anim.drawing = qfalse;
	} else if (demo.anim.target >= 0 && !demo.anim.override[demo.anim.target]) {
		centity_t *cent = demoTargetEntity( demo.anim.target );
		if (cent) {
			vec3_t container;
			demoCentityBoxSize( cent, container );
			demoDrawBox( cent->lerpOrigin, container, colorWhite );
		}
	}
}

static int animHitBones(const vec3_t start, const vec3_t forward, const int target) {
    int i;
	vec3_t container = {-1.0f, 1.0f, 1.0f};
	vec3_t origins[MAX_BONES], traceStart, traceImpact;

	if (demo.anim.target < 0)
		return -1;
	for (i=0;i<MAX_BONES;i++) {
		animBoneVectors(origins, ORIGIN, demo.anim.target);
		VectorSubtract( start, origins[i], traceStart );
		if (!BoxTraceImpact( traceStart, forward, container, traceImpact ))
			continue;
		return i;
	}
	return -1;
}

void demoMoveAnim(void) {
	vec3_t *angles;

	if (demo.anim.locked) {
		demoAnimPoint_t *point = animPointSynch(demo.play.time, demo.anim.target);
		if (point && point->time == demo.play.time && !demo.play.fraction) {
			angles = point->angles;
		} else {
			return;
		}
	} else {
		angles = demo.anim.angles;
	}

#if 1
	 if ((demo.cmd.buttons & BUTTON_ATTACK && !(demo.cmd.upmove > 0))) {
		switch (demo.viewType) {
		case viewCamera:
			cameraMoveDirect();
			break;
		case viewChase:
			demoMoveChaseDirect();
			break;
		}
	} if (demo.cmd.upmove > 0 && demo.cmd.buttons & BUTTON_ATTACK) {
		if (demo.anim.target >= 0 && demo.anim.target < MAX_CLIENTS && demo.anim.override[demo.anim.target]) {
			VectorAdd(angles[demo.anim.bone], demo.cmdDeltaAngles, angles[demo.anim.bone]);
			AnglesNormalize180(angles[demo.anim.bone]);
			animSetAngles(angles, demo.anim.target);
		}
	} else if (demo.cmd.upmove > 0 && demo.cmd.buttons & BUTTON_ALT_ATTACK) {
		angles[demo.anim.bone][ROLL] -= demo.cmdDeltaAngles[YAW];
		AnglesNormalize180(angles[demo.anim.bone]);
		animSetAngles(angles, demo.anim.target);
	}
#else
	if (demo.anim.target >= 0 && demo.anim.target < MAX_CLIENTS && demo.anim.override[demo.anim.target]) {
		if (demo.cmd.buttons & BUTTON_ATTACK && demo.cmd.buttons & BUTTON_ALT_ATTACK) {
			angles[demo.anim.bone][PITCH] -= demo.cmdDeltaAngles[PITCH];
		} else if (demo.cmd.buttons & BUTTON_ATTACK) {
			angles[demo.anim.bone][YAW] -= demo.cmdDeltaAngles[YAW];
		} else if (demo.cmd.buttons & BUTTON_ALT_ATTACK) {
			angles[demo.anim.bone][ROLL] -= demo.cmdDeltaAngles[YAW];	
		}
		if (demo.cmd.buttons & BUTTON_ATTACK || demo.cmd.buttons & BUTTON_ALT_ATTACK) {
			AnglesNormalize180(angles[demo.anim.bone]);
			animSetAngles(angles, demo.anim.target);
		}
	}
#endif
}

/*
Default keys and combos:
R = lock/unlock animation
SPACE + R = lock/unlock animation on a target
Q/E = next prev animation keypoint
SPACE + Q/E = prev/next target/bone(on selected target)
MOUSE + F = select/deselect target/bone(on selected target) that crosses center of screen)
F = deselect target
*/

void demoAnimCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "lock")) {
		if (demo.cmd.upmove > 0) {
			int clientId, i;
			cmd = CG_Argv(2);
			clientId = cmd[0] ? atoi(cmd) : demo.anim.target;
			if (clientId >= 0 && clientId < MAX_CLIENTS) {
				demo.anim.override[clientId] = !demo.anim.override[clientId];
				if (demo.anim.override[clientId]) {
					//animGetAngles(demo.anim.angles, clientId);
					animSetAngles(demo.anim.angles, clientId);
					CG_DemosAddLog("Animation locked on %i client", clientId);
				} else {
					centity_t	*cent = &cg_entities[clientId];
					CG_DemosAddLog("Animation unlocked on %i client", clientId);
					for (i = 1; i < MAX_BONES; i++) {
						trap_G2API_SetBoneAngles(cent->ghoul2, 0, boneList[i], vec3_origin, 0, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.gameModels, 0, cg.time);
					}
				}
			} else {
				CG_DemosAddLog("Unable to lock any target");
			}
		} else {
			demo.anim.locked = !demo.anim.locked;
			if (demo.anim.locked) 
				CG_DemosAddLog("Animation locked");
			else 
				CG_DemosAddLog("Animation unlocked");
		}
	} else if (!Q_stricmp(cmd, "add")) {
		if (animPointAdd(demo.play.time, demo.anim.angles, demo.anim.target)) {
			CG_DemosAddLog("Added anim point on %i client", demo.anim.target);
		} else if (demo.anim.target < 0) {
			CG_DemosAddLog("Failed to add anim point, select a client");
		} else {
			CG_DemosAddLog("Failed to add anim point on %i client", demo.anim.target);
		}
	} else if (!Q_stricmp(cmd, "del")) {
		if (animPointDel(demo.play.time, demo.anim.target) ) {
			CG_DemosAddLog("Deleted anim point on %i client", demo.anim.target);
		} else if (demo.anim.target < 0) {
			CG_DemosAddLog("Failed to delete anim point, select a client");
		} else {
			CG_DemosAddLog("Failed to delete anim point on %i client", demo.anim.target);
		}
	} else if (!Q_stricmp(cmd, "next")) {
		if (demo.cmd.upmove > 0) {
			if (demo.anim.locked) {
				CG_DemosAddLog("Can't change target or bone when locked");
			} else if (demo.anim.target >= 0 && demo.anim.override[demo.anim.target]) {
				animNextBone(&demo.anim.bone);
				CG_DemosAddLog("Selected %s bone", boneList[demo.anim.bone]);
			} else {
				animNextTarget(&demo.anim.target);
			}
			return;
		} else {
			demoAnimPoint_t *point = animPointSynch(demo.play.time, demo.anim.target);
			if (!point)
				return;
			if (point->next)
				point = point->next;
			demo.play.time = point->time;
			demo.play.fraction = 0;
		}
	} else if (!Q_stricmp(cmd, "prev")) {
		if (demo.cmd.upmove > 0) {
			if (demo.anim.locked) {
				CG_DemosAddLog("Can't change target or bone when locked");
			} else if (demo.anim.target >= 0 && demo.anim.override[demo.anim.target]) {
				animPrevBone(&demo.anim.bone);
				CG_DemosAddLog("Selected %s bone", boneList[demo.anim.bone]);
			} else {
				animPrevTarget(&demo.anim.target);
			}
			return;
		} else {
			demoAnimPoint_t *point = animPointSynch(demo.play.time, demo.anim.target);
			if (!point)
				return;
			if (point->prev)
				point = point->prev;
			demo.play.time = point->time;
			demo.play.fraction = 0;
		}
	} else if (!Q_stricmp(cmd, "target")) {
		if ( demo.anim.locked ) {
			CG_DemosAddLog("Can't target while locked" );
			return;
		}
		if ( demo.cmd.buttons & BUTTON_ATTACK ) {
			if (demo.anim.target>=0 && demo.anim.override[demo.anim.target]) {
				vec3_t forward;
				AngleVectors( demo.viewAngles, forward, 0, 0 );
				demo.anim.bone = animHitBones( demo.viewOrigin, forward, demo.anim.target );
				if (demo.anim.bone >= 0) {
					CG_DemosAddLog("Bone set to %s", boneList[demo.anim.bone] );
				} else {
					CG_DemosAddLog("Unable to hit any bone" );
				}
			} else if (demo.anim.target>=0) {			
				CG_DemosAddLog("Cleared target %d", demo.anim.target );
				demo.anim.target = -1;
			} else {
				vec3_t forward;
				AngleVectors( demo.viewAngles, forward, 0, 0 );
				demo.anim.target = demoHitEntities( demo.viewOrigin, forward );
				if (demo.anim.target < 0 || demo.anim.target >= MAX_CLIENTS) {
					CG_DemosAddLog("Unable to hit any target" );
				} else {
					CG_DemosAddLog("Target set to %d", demo.anim.target );
				}
			}
		} else {
			CG_DemosAddLog("Cleared target %d", demo.anim.target );
			demo.anim.target = -1;
		}
	} else {
		Com_Printf("anim usage:\n" );
		Com_Printf("anim add/del, add/del an anim point at current time with current anim angles.\n" );
		Com_Printf("anim clear (target), clear all anim points, optional target to clear only points on selected target. (Use with caution...)\n" );
		Com_Printf("anim lock (target), lock the new animation or let use default, optional target to override selected target only.\n" );
		Com_Printf("anim target, Clear/Set the target currently being aimed at.\n" );
		Com_Printf("anim targetNext/Prev, Go the next or previous player.\n" );
		Com_Printf("anim shift time, move time indexes of chase points by certain amount.\n" );
		Com_Printf("anim next/prev, go to time index of next or previous point of selected target.\n" );
		Com_Printf("anim start/end, current time with be set as start/end of selection.\n" );
		Com_Printf("anim bone (a)0 (a)0 (a)0, Directly control angles of selected bone, optional a before number to add to current value\n" );
	}
}

int CG_DemosPlayerAnimation(int client) {
	if (cg.demoPlayback != 2 || client >= MAX_CLIENTS || (demo.anim.locked && !demo.anim.points[client]) || !demo.anim.override[client]) {
		return 0;
	} else if (demo.anim.locked && demo.anim.points[client]/* && (demo.capture.active || demo.editType != editAnim)*/) {
		return 1;
	} else {
		return 2;
	}
}
#else //DEMO_ANIM
int CG_DemosPlayerAnimation(int client) {
	return 0;
}
#endif //DEMO_ANIM
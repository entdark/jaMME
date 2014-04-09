#include "cg_demos.h" 
#include "ghoul2/G2.h"

#ifdef DEMO_ANIM

/* vertical representation
static char *boneList[72] = {
	"model_root",
	"pelvis",
	"Motion",
	"lfemurYZ",
	"lfemurX",
	"ltibia",
	"ltalus",
	"ltarsal",
	"rfemurYZ",
	"rfemurX",
	"rtibia",
	"rtalus",
	"rtarsal",
	"lower_lumbar",
	"upper_lumbar",
	"thoracic",
	"cervical",
	"cranium",
	"ceyebrow",
	"jaw",
	"lblip2",
	"leye",
	"rblip2",
	"ltlip2",
	"rtlip2",
	"reye",
	"rclavical",
	"rhumerus",
	"rhumerusX",
	"rradius",
	"rradiusX",
	"rhand",
	"mc7",
	"r_d5_j1",
	"r_d5_j2",
	"r_d5_j3",
	"r_d1_j1",
	"r_d1_j2",
	"r_d1_j3",
	"r_d2_j1",
	"r_d2_j2",
	"r_d2_j3",
	"r_d3_j1",
	"r_d3_j2",
	"r_d3_j3",
	"r_d4_j1",
	"r_d4_j2",
	"r_d4_j3",
	"rhang_tag_bone",
	"lclavical",
	"lhumerus",
	"lhumerusX",
	"lradius",
	"lradiusX",
	"lhand",
	"mc5",
	"l_d5_j1",
	"l_d5_j2",
	"l_d5_j3",
	"l_d4_j1",
	"l_d4_j2",
	"l_d4_j3",
	"l_d3_j1",
	"l_d3_j2",
	"l_d3_j3",
	"l_d2_j1",
	"l_d2_j2",
	"l_d2_j3",
	"l_d1_j1",
	"l_d1_j2",
	"l_d1_j3",
	"face_always_",
};
//vertical representation */

const char *boneList[72] = {
	"model_root","pelvis","Motion","lfemurYZ","lfemurX","ltibia","ltalus","ltarsal",
	"rfemurYZ","rfemurX","rtibia","rtalus","rtarsal","lower_lumbar","upper_lumbar","thoracic",
	"cervical","cranium","ceyebrow","jaw","lblip2","leye","rblip2","ltlip2",
	"rtlip2","reye","rclavical","rhumerus","rhumerusX","rradius","rradiusX","rhand",
	"mc7","r_d5_j1","r_d5_j2","r_d5_j3","r_d1_j1","r_d1_j2","r_d1_j3","r_d2_j1",
	"r_d2_j2","r_d2_j3","r_d3_j1","r_d3_j2","r_d3_j3","r_d4_j1","r_d4_j2","r_d4_j3",
	"rhang_tag_bone","lclavical","lhumerus","lhumerusX","lradius","lradiusX","lhand","mc5",
	"l_d5_j1","l_d5_j2","l_d5_j3","l_d4_j1","l_d4_j2","l_d4_j3","l_d3_j1","l_d3_j2",
	"l_d3_j3","l_d2_j1","l_d2_j2","l_d2_j3","l_d1_j1","l_d1_j2","l_d1_j3","face_always_",
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
	//check if it exists and if so then select this bone
	return qtrue; //all bones are valid for now
}

qboolean animPrevBone(int *oldBone) {
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

qboolean animNextBone(int *oldBone) {
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

static void animInterpolate(int time, float timeFraction, vec3_t angles[MAX_BONES], int target) {
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

	for (bone = 0; bone < MAX_BONES; bone++) { //replace 72 (see tr_ghoul2.cpp string 3851) with found amount of bones?
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
		mdxaBone_t	boltMatrix;
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

static void animRotateChild(const vec3_t parent, const vec3_t child, vec3_t out) {
	mdxaBone_t Tp, Tpinv, Tch;
	vec3_t Ap, Ach, m[3];
	
	VectorCopy(parent, Ap);
	Ap[0] += 180;
	MatrixCreate(Ap, &Tp);

	VectorCopy(child, Ach);
	Ach[0] += 180;
	MatrixCreate(Ach, &Tch);

	MatrixCreate(Ap, &Tpinv);
	MatrixInverse3x3(&Tpinv);

	MatrixMultiply3x4(&Tch, &Tch, &Tpinv);
	MatrixMultiply3x4(&Tch, &Tp, &Tch);
	
	VectorSet(m[0], Tch.matrix[0][0], Tch.matrix[1][0], Tch.matrix[2][0]);
	VectorSet(m[1], Tch.matrix[0][1], Tch.matrix[1][1], Tch.matrix[2][1]);
	VectorSet(m[2], Tch.matrix[0][2], Tch.matrix[1][2], Tch.matrix[2][2]);

	AxisToAngles(m, out);
	out[0] -= 180;
}
static void animNormalizeAngle(const vec3_t in, const vec3_t delta, vec3_t out) {
	//VectorSubtract(in, delta, out);
	VectorAdd(in, delta, out);
	AnglesNormalize360(out);
}
static void animSetNormalizedAngles(vec3_t angles[MAX_BONES], const vec3_t delta, const int bone) {
	int i;
	if (delta[0] == 0 && delta[1] == 0 && delta[2] == 0)
		return;

	if (bone == 0) {
		return;
	} else if (bone == 1) {
	/* normalize all "pelvis" children */
		for (i = 2; i < MAX_BONES; i++)
			animNormalizeAngle(angles[i], delta, angles[i]);
	
	
	//LEFT LEG
	} else if (bone == 3) {
	/* normalize all "lfemurYZ" children */
		animNormalizeAngle(angles[4], delta, angles[4]); //"lfemurX"
		animNormalizeAngle(angles[5], delta, angles[5]); //"ltibia"->"ltalus"
		animNormalizeAngle(angles[6], delta, angles[6]); //"ltalus"->"ltarsal"
		animNormalizeAngle(angles[7], delta, angles[7]); //"ltarsal"
	} else if (bone == 5) {
	/* normalize all "ltibia" children */
		animRotateChild(angles[5], angles[6], angles[6]);
		animRotateChild(angles[6], angles[7], angles[7]);
		//animNormalizeAngle(angles[6], delta, angles[6]); //"ltalus"->"ltarsal"
		//animNormalizeAngle(angles[7], delta, angles[7]); //"ltarsal"
	} else if (bone == 6) {
	/* normalize "ltalus" child "ltarsal" */
		animRotateChild(angles[6], angles[7], angles[7]);
//		animNormalizeAngle(angles[7], delta, angles[7]); //"ltarsal"
	
	
	//RIGHT LEG
	} else if (bone == 8) {
	/* normalize all "rfemurYZ" children */
		animNormalizeAngle(angles[9], delta, angles[9]); //"rfemurX"
		animNormalizeAngle(angles[10], delta, angles[10]); //"rtibia"->"rtalus"
		animNormalizeAngle(angles[11], delta, angles[11]); //"rtalus"->"rtarsal"
		animNormalizeAngle(angles[12], delta, angles[12]); //"rtarsal"
	} else if (bone == 10) {
	/* normalize all "rtibia" children */
		animNormalizeAngle(angles[11], delta, angles[11]); //"rtalus"->"rtarsal"
		animNormalizeAngle(angles[12], delta, angles[12]); //"rtarsal"
	} else if (bone == 11) {
	/* normalize "rtalus" child "rtarsal" */
		animNormalizeAngle(angles[12], delta, angles[12]); //"rtarsal"
	
	
	} else if (bone == 13) {
	/* normalize all "lower_lumbar" children */
		for (i = 14; i < MAX_BONES; i++)
			animNormalizeAngle(angles[i], delta, angles[i]);
	} else if (bone == 14) {
	/* normalize all "upper_lumbar" children */
		for (i = 15; i < MAX_BONES; i++)
			animNormalizeAngle(angles[i], delta, angles[i]);
	} else if (bone == 15) {
	/* normalize all "thoracic" children */
		for (i = 16; i < MAX_BONES; i++)
			animNormalizeAngle(angles[i], delta, angles[i]);
	} else if (bone == 16) {
	/* normalize all "cervical" children */
		for (i = 17; i < MAX_BONES; i++) {
			if (i >= 26 && i <= 70)
				continue;
			animNormalizeAngle(angles[i], delta, angles[i]);
		}
	} else if (bone == 17) {
	/* normalize all "cranium" children */
		for (i = 18; i < MAX_BONES; i++) {
			if (i >= 26 && i <= 70)
				continue;
			animNormalizeAngle(angles[i], delta, angles[i]);
		}
	
	
	//RIGHT ARM
	} else if (bone == 27) {
	/* normalize all "rhumerus" children */
		animNormalizeAngle(angles[28], delta, angles[28]); //"rhumerusX"
		animNormalizeAngle(angles[29], delta, angles[29]); //"rradius"->"rradiusX","rhand","r_d*","rhang_tag_bone"
	} else if (bone == 29) {
	/* normalize all "rradius" children */
		for (i = 30; i <= 48; i++)
			animNormalizeAngle(angles[i], delta, angles[i]); //"rradiusX","rhand","r_d*","rhang_tag_bone"
	
	
	//LEFT ARM
	} else if (bone == 50) {
	/* normalize all "lhumerus" children */
		animNormalizeAngle(angles[51], delta, angles[28]); //"lhumerusX"
		animNormalizeAngle(angles[52], delta, angles[52]); //"lradius"->"lradiusX","lhand","l_d*"
	} else if (bone == 52) {
	/* normalize all "lradius" children */
		for (i = 53; i <= 70; i++)
			animNormalizeAngle(angles[i], delta, angles[i]); //"lradiusX","lhand","l_d*"
	
	
	} else if (bone == 71) {
	/* normalize all "cervical" children */
		for (i = 18; i <= 25; i++)
			animNormalizeAngle(angles[i], delta, angles[i]);
	}
}

static void animSetSingleAngle(vec3_t angles[MAX_BONES], const int bone, const int client) {
	centity_t	*cent = &cg_entities[client];
	if (!(cent && cent->currentValid) || bone <= 0)
		return;
	trap_G2API_SetBoneAngles(cent->ghoul2, 0, boneList[bone], angles[bone], BONE_ANGLES_REPLACE, POSITIVE_Z, POSITIVE_Y, POSITIVE_X, cgs.gameModels, 0, cg.time);
	animGetAngles(angles, client);
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
		trap_G2API_SetBoneAngles(cent->ghoul2, 0, boneList[i], angles[i], BONE_ANGLES_REPLACE, POSITIVE_Z, POSITIVE_Y, POSITIVE_X, cgs.gameModels, 0, cg.time);
	}
}

void animUpdate(int time, float timeFraction) {
	int		i;
	vec3_t	angles[MAX_BONES];
	if (!demo.anim.locked)
		return;
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

	if (!(cent && cent->currentValid))
		return;

	for (i = 0; i < MAX_BONES; i++) {
		mdxaBone_t boltMatrix;
		int bolt = trap_G2API_AddBolt(cent->ghoul2, 0, boneList[i]);
		trap_G2API_GetBoltMatrix(cent->ghoul2, 0, bolt, &boltMatrix, vec3_origin, vec3_origin, cg.time, cgs.gameModels, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, flags, vec[i]);
	}
}

static void animSkeletonDraw(const vec3_t origins[MAX_BONES], const vec3_t ang[MAX_BONES]) {
	int i;

	if (ang[MAX_BONES]) {
		for (i = 0; i < MAX_BONES; i++) {
			float scale = i == demo.anim.bone ? 7.0f : 2.3f; //selected bone is bigger
			vec3_t origin2, forward, right, up, ang2;
			AngleVectors(ang[i], forward, right, up);

/*			VectorMA(origins[i], 2.3f, forward, origin2);
			demoDrawLine(origins[i], origin2, colorYellow);
			VectorMA(origins[i], 2.3f, right, origin2);
			demoDrawLine(origins[i], origin2, colorMagenta);
			VectorMA(origins[i], 2.3f, up, origin2);
			demoDrawLine(origins[i], origin2, colorCyan);
*/
			VectorMA(origins[i], scale, axisDefault[2], origin2);
			demoDrawLine(origins[i], origin2, colorBlue);

			VectorSet(ang2, 0.0f, ang[i][1], 0.0f);
			AngleVectors(ang2, 0, right, 0);
			VectorMA(origins[i], scale, right, origin2);
			demoDrawLine(origins[i], origin2, colorGreen);

			ang2[2] = 180.0f - ang[i][0];
			VectorSet(ang2, ang2[2], ang[i][1], 0.0f);
			AngleVectors(ang2, forward, 0, 0);
			VectorMA(origins[i], scale, forward, origin2);
			demoDrawLine(origins[i], origin2, colorRed);
		}
	} else {
		demoDrawLine(origins[0], origins[1], colorRed); //"model_root" <-> "pelvis"
		/* draw all "pelvis" children */
		for (i = 2; i <= 13; i++) {
			demoDrawLine(origins[1], origins[i], colorRed);
		}
		demoDrawLine(origins[13], origins[14], colorRed); //"lower_lumbar" <-> "upper_lumbar"
		demoDrawLine(origins[14], origins[15], colorRed); //"upper_lumbar" <-> "thoracic"
		demoDrawLine(origins[15], origins[16], colorRed); //"thoracic" <-> "cervical"
		demoDrawLine(origins[16], origins[17], colorRed); //"cervical" <-> "cranium"
		/* draw all "face_always_" children */
		for (i = 18; i <= 25; i++) {
			demoDrawLine(origins[71], origins[i], colorRed);
		}
		/* draw all "thoracic" children */
		for (i = 26; i <= 70; i++) {
			demoDrawLine(origins[15], origins[i], colorRed);
		}
		demoDrawLine(origins[17], origins[71], colorRed); //"cranium" <-> "face_always_"
	}
}

void animDraw(int time, float timeFraction) {
	int		i, bone;
	vec3_t	axis[3][MAX_BONES], origins[MAX_BONES], ang[MAX_BONES];
	for (i = 0; 0 && i < MAX_CLIENTS; i++) {
		int j;
		if (!demo.anim.override[i] || !cg_entities[i].currentValid)
			continue;
		animBoneVectors(origins, ORIGIN, i);
		animBoneVectors(axis[0], NEGATIVE_Y, i);
		animBoneVectors(axis[1], POSITIVE_Z, i);
		animBoneVectors(axis[2], POSITIVE_X, i);
		for (j = 0; j < MAX_BONES; j++) {
			vec3_t axis2[3];
			VectorCopy(axis[0][j], axis2[0]);
			VectorCopy(axis[1][j], axis2[1]);
			VectorCopy(axis[2][j], axis2[2]);
			AxisToAngles(axis2, ang[j]);
		}
//		animSkeletonDraw(origins, ang);
		animSkeletonDraw(origins, demo.anim.angles);
	}
	if (demo.anim.target >= 0 && demo.anim.override[demo.anim.target]) {
		vec3_t container = {-1.0f, 1.0f, 1.0f};
		animBoneVectors(origins, ORIGIN, demo.anim.target);
		demoDrawBox( origins[demo.anim.bone], container, colorWhite );
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

	if (demo.anim.target <= 0)
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
	 if (demo.cmd.buttons & BUTTON_ATTACK && (!(demo.cmd.buttons & BUTTON_ALT_ATTACK)
		 || !(demo.anim.target >= 0 && demo.anim.target < MAX_CLIENTS && demo.anim.override[demo.anim.target]))) {
		switch (demo.viewType) {
		case viewCamera:
			cameraMoveDirect();
			break;
		case viewChase:
			demoMoveChaseDirect();
			break;
		}
	} if (demo.cmd.buttons & BUTTON_ATTACK && demo.cmd.buttons & BUTTON_ALT_ATTACK) {
		if (demo.anim.target >= 0 && demo.anim.target < MAX_CLIENTS && demo.anim.override[demo.anim.target]) {
			VectorAdd(angles[demo.anim.bone], demo.cmdDeltaAngles, angles[demo.anim.bone]);
			animSetNormalizedAngles(angles, demo.cmdDeltaAngles, demo.anim.bone);
			AnglesNormalize180(angles[demo.anim.bone]);
			animSetAngles(angles, demo.anim.target);
//			animSetSingleAngle(angles, demo.anim.bone, demo.anim.target);
		}
	} else if (demo.cmd.buttons & BUTTON_ALT_ATTACK) {
		vec3_t delta = {0.0f, 0.0f, demo.cmdDeltaAngles[YAW]};
		angles[demo.anim.bone][ROLL] -= demo.cmdDeltaAngles[YAW];
		animSetNormalizedAngles(angles, delta, demo.anim.bone);
		AnglesNormalize180(angles[demo.anim.bone]);
		animSetAngles(angles, demo.anim.target);
//		animSetSingleAngle(angles, demo.anim.bone, demo.anim.target);
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
					animGetAngles(demo.anim.angles, clientId);
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

qboolean CG_DemosPlayerAnimation(int client) {
	if (cg.demoPlayback != 2 || (demo.anim.locked && !demo.anim.points[client]) || !demo.anim.override[client]) {
		return qfalse;
	} else {
		return qtrue;
	}
}
#else //DEMO_ANIM
qboolean CG_DemosPlayerAnimation(int client) {
	return qfalse;
}
#endif //DEMO_ANIM
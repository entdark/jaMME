#include "cg_local.h"
#include "../qcommon/q_shared.h"
#include "../ghoul2/G2.h"

//rww - The turret is heavily dependant on bone angles. We can't happily set that on the server, so it is done client-only.

void CreepToPosition(vec3_t ideal, vec3_t current) {
	int max_degree_switch = 90;
	float degrees_negative = 0.0f;
	float degrees_positive = 0.0f;
	qboolean doNegative = qfalse;
	float angle_ideal;
	float angle_current;

	angle_ideal = ideal[YAW];
	angle_current = current[YAW];

	if (angle_ideal <= angle_current) {
		degrees_negative = (angle_current - angle_ideal);
		degrees_positive = (360 - angle_current) + angle_ideal;
	} else {
		degrees_negative = angle_current + (360 - angle_ideal);
		degrees_positive = (angle_ideal - angle_current);
	}

	if (degrees_negative < degrees_positive) {
		doNegative = qtrue;
	}
	if (doNegative) {
		current[YAW] -= max_degree_switch;
		if (current[YAW] < ideal[YAW] && (current[YAW]+(max_degree_switch*2)) >= ideal[YAW]) {
			current[YAW] = ideal[YAW];
		}
		if (current[YAW] < 0) {
			current[YAW] += 361.f;
		}
	} else {
		current[YAW] += max_degree_switch;
		if (current[YAW] > ideal[YAW] && (current[YAW]-(max_degree_switch*2)) <= ideal[YAW]) {
			current[YAW] = ideal[YAW];
		}
		if (current[YAW] > 360) {
			current[YAW] -= 361.f;
		}
	}

	if (ideal[PITCH] < 0) {
		ideal[PITCH] += 360;
	}
	angle_ideal = ideal[PITCH];
	angle_current = current[PITCH];

	doNegative = qfalse;

	if (angle_ideal <= angle_current) {
		degrees_negative = (angle_current - angle_ideal);
		degrees_positive = (360 - angle_current) + angle_ideal;
	} else {
		degrees_negative = angle_current + (360 - angle_ideal);
		degrees_positive = (angle_ideal - angle_current);
	}

	if (degrees_negative < degrees_positive) {
		doNegative = qtrue;
	} 
	if (doNegative) {
		current[PITCH] -= max_degree_switch;
		if (current[PITCH] < ideal[PITCH] && (current[PITCH]+(max_degree_switch*2)) >= ideal[PITCH]) {
			current[PITCH] = ideal[PITCH];
		}
		if (current[PITCH] < 0) {
			current[PITCH] += 361.f;
		}
	} else {
		current[PITCH] += max_degree_switch;
		if (current[PITCH] > ideal[PITCH] && (current[PITCH]-(max_degree_switch*2)) <= ideal[PITCH]) {
			current[PITCH] = ideal[PITCH];
		}
		if (current[PITCH] > 360) {
			current[PITCH] -= 361.f;
		}
	}
}

void TurretClientRun(centity_t *ent) {
	if (!ent->ghoul2) {
		weaponInfo_t	*weaponInfo;

		trap_G2API_InitGhoul2Model(&ent->ghoul2, CG_ConfigString( CS_MODELS+ent->currentState.modelindex ), 0, 0, 0, 0, 0);

		if (!ent->ghoul2) { //bad
			return;
		}

		ent->torsoBolt = trap_G2API_AddBolt( ent->ghoul2, 0, "*flash02" );

		trap_G2API_SetBoneAngles( ent->ghoul2, 0, "bone_hinge", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 100, cg.time ); 
		trap_G2API_SetBoneAngles( ent->ghoul2, 0, "bone_gback", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 100, cg.time ); 
		trap_G2API_SetBoneAngles( ent->ghoul2, 0, "bone_barrel", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 100, cg.time ); 

		trap_G2API_SetBoneAnim( ent->ghoul2, 0, "model_root", 0, 11, BONE_ANIM_OVERRIDE_FREEZE, 0.8f, cg.time, 0, 0 );

		ent->turAngles[ROLL] = 0;
		ent->turAngles[PITCH] = 90;
		ent->turAngles[YAW] = 0;

		weaponInfo = &cg_weapons[WP_TURRET];

		if ( !weaponInfo->registered ) {
			CG_RegisterWeapon(WP_TURRET);
		}
	}

	if (ent->currentState.fireflag == 2) {
	//I'm about to blow
		if (ent->turAngles) {
			trap_G2API_SetBoneAngles( ent->ghoul2, 0, "bone_hinge", ent->turAngles, BONE_ANGLES_REPLACE, NEGATIVE_Y, NEGATIVE_Z, NEGATIVE_X, NULL, 100, cg.time ); 
		}
		return;
	} else if (ent->currentState.fireflag && ent->bolt4 != ent->currentState.fireflag) {
		vec3_t muzzleOrg, muzzleDir;
		mdxaBone_t boltMatrix;

		trap_G2API_GetBoltMatrix(ent->ghoul2, 0, ent->torsoBolt, &boltMatrix, /*ent->lerpAngles*/vec3_origin, ent->lerpOrigin, cg.time, cgs.gameModels, ent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, muzzleOrg);
		BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, muzzleDir);

		trap_FX_PlayEffectID(cgs.effects.mTurretMuzzleFlash, muzzleOrg, muzzleDir, -1, -1);

		ent->bolt4 = ent->currentState.fireflag;
	} else if (!ent->currentState.fireflag) {
		ent->bolt4 = 0;
	}

	if (ent->currentState.bolt2 != ENTITYNUM_NONE) {
	//turn toward the enemy
		centity_t *enemy = &cg_entities[ent->currentState.bolt2];
		if (enemy) {
			vec3_t enAng;
			vec3_t enPos;

			VectorCopy(enemy->currentState.pos.trBase, enPos);

			VectorSubtract(enPos, ent->lerpOrigin, enAng);
			VectorNormalize(enAng);
			vectoangles(enAng, enAng);
			enAng[ROLL] = 0;
			enAng[PITCH] += 90;

			CreepToPosition(enAng, ent->turAngles);
		}
	} else {
		vec3_t idleAng;
		float turnAmount;

		if (ent->turAngles[YAW] > 360) {
			ent->turAngles[YAW] -= 360.0f;
		}

		if (!ent->dustTrailTime) {
			ent->dustTrailTime = cg.time;
		}

		turnAmount = ((cg.time - ent->dustTrailTime) + cg.timeFraction) * 0.03f;
		if (turnAmount > 360) {
			turnAmount = 360;
		}

		idleAng[PITCH] = 90;
		idleAng[ROLL] = 0;
		if (cg.frametime)
			idleAng[YAW] = ent->turAngles[YAW] + turnAmount;
		ent->dustTrailTime = cg.time;
		if (cg.frametime)
			CreepToPosition(idleAng, ent->turAngles);
	}

	if (cg.time < ent->frame_minus1_refreshed) {
		ent->frame_minus1_refreshed = cg.time;
		return;
	}

	ent->frame_minus1_refreshed = cg.time;
	trap_G2API_SetBoneAngles( ent->ghoul2, 0, "bone_hinge", ent->turAngles, BONE_ANGLES_REPLACE, NEGATIVE_Y, NEGATIVE_Z, NEGATIVE_X, NULL, 100, cg.time ); 
}

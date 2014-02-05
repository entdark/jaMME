// Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

#include "bg_demos.h"
#include "cg_demos.h" 

#define Qx 0
#define Qy 1
#define Qz 2
#define Qw 3

/*
=================
GetPerpendicularViewVector

  Used to find an "up" vector for drawing a sprite so that it always faces the view as best as possible
=================
*/
void GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up )
{
	vec3_t	v1, v2;

	VectorSubtract( point, p1, v1 );
	VectorNormalize( v1 );

	VectorSubtract( point, p2, v2 );
	VectorNormalize( v2 );

	CrossProduct( v1, v2, up );
	VectorNormalize( up );
}

qboolean MatrixCompare(const mdxaBone_t m1, const mdxaBone_t m2) {
	// check inversed values too
	if ((m1.matrix[0][0] == m2.matrix[0][0] || m1.matrix[0][0] == -m2.matrix[0][0]) &&
		(m1.matrix[1][0] == m2.matrix[1][0] || m1.matrix[1][0] == -m2.matrix[1][0]) &&
		(m1.matrix[2][0] == m2.matrix[2][0] || m1.matrix[1][0] == -m2.matrix[1][0]) &&

		(m1.matrix[0][1] == m2.matrix[0][1] || m1.matrix[1][0] == -m2.matrix[1][0]) &&
		(m1.matrix[1][1] == m2.matrix[1][1] || m1.matrix[1][0] == -m2.matrix[1][0]) &&
		(m1.matrix[2][1] == m2.matrix[2][1] || m1.matrix[1][0] == -m2.matrix[1][0]) &&

		(m1.matrix[0][2] == m2.matrix[0][2] || m1.matrix[1][0] == -m2.matrix[1][0]) &&
		(m1.matrix[1][2] == m2.matrix[1][2] || m1.matrix[1][0] == -m2.matrix[1][0]) &&
		(m1.matrix[2][2] == m2.matrix[2][2] || m1.matrix[1][0] == -m2.matrix[1][0]))
		return qtrue;
	else
		return qfalse;
}

void AxisToMatrix(const vec3_t axis[3], mdxaBone_t *matrix) {
	matrix->matrix[0][0] = axis[0][0];
	matrix->matrix[1][0] = axis[0][1];
	matrix->matrix[2][0] = axis[0][2];

	matrix->matrix[0][1] = axis[1][0];
	matrix->matrix[1][1] = axis[1][1];
	matrix->matrix[2][1] = axis[1][2];

	matrix->matrix[0][2] = axis[2][0];
	matrix->matrix[1][2] = axis[2][1];
	matrix->matrix[2][2] = axis[2][2];

	matrix->matrix[0][3] = 0;
	matrix->matrix[1][3] = 0;
	matrix->matrix[2][3] = 0;
}

void CreateMatrix(const float *angle, mdxaBone_t *matrix) {
	vec3_t		axis[3];

	// convert angles to axis
	AnglesToAxis( angle, axis );
	matrix->matrix[0][0] = axis[0][0];
	matrix->matrix[1][0] = axis[0][1];
	matrix->matrix[2][0] = axis[0][2];

	matrix->matrix[0][1] = axis[1][0];
	matrix->matrix[1][1] = axis[1][1];
	matrix->matrix[2][1] = axis[1][2];

	matrix->matrix[0][2] = axis[2][0];
	matrix->matrix[1][2] = axis[2][1];
	matrix->matrix[2][2] = axis[2][2];

	matrix->matrix[0][3] = 0;
	matrix->matrix[1][3] = 0;
	matrix->matrix[2][3] = 0;
}

void MatrixAxisToAngles( const vec3_t axis[3], vec3_t angles ) {
	vec3_t	forward, right, up;
	float	angle;
	float	sp, cr, cp, cy;

	VectorCopy(axis[0], forward);
	VectorSubtract(vec3_origin, axis[1], right); //vector inverse? O:
	VectorCopy(axis[2], up);

	sp = -forward[2];
	angle = asin(sp);
	angles[PITCH] = angle / (M_PI*2 / 360);
	cp = cos(angle);

	cy = forward[0] / cp;
	angle = acos(cy);
	angles[YAW] = angle / (M_PI*2 / 360);

	cr = up[2] / cp;
	angle = acos(cr);
	angles[ROLL] = angle / (M_PI*2 / 360);
}

void AxisToAngles( const vec3_t axis[3], vec3_t angles ) {
	float	length1;
	float	yaw, pitch, roll = 0;
	
	if ( axis[0][1] == 0 && axis[0][0] == 0 ) {
		yaw = 0;
		if ( axis[0][2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if ( axis[0][0] ) {
			yaw = ( atan2 ( axis[0][1], axis[0][0] ) * 180 / M_PI );
		}
		else if ( axis[0][1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		length1 = sqrt ( axis[0][0]*axis[0][0] + axis[0][1]*axis[0][1] );
		pitch = ( atan2( axis[0][2], length1) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}

		roll = ( atan2( axis[1][2], axis[2][2] ) * 180 / M_PI );
		if ( roll < 0 ) {
			roll += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = roll;
}

qboolean ID_INLINE BoxTraceTestResult( int axis, float dist, const vec3_t start, const vec3_t forward, const vec3_t mins, const vec3_t maxs, vec3_t result ) {
	result[0] = start[0] + forward[0] * dist;
	result[1] = start[1] + forward[1] * dist;
	result[2] = start[2] + forward[2] * dist;

	if (axis != 0 && (result[0] < mins[0] || result[0] > maxs[0]))
		return qfalse;
	if (axis != 1 && (result[1] < mins[1] || result[1] > maxs[1]))
		return qfalse;
	if (axis != 2 && (result[2] < mins[2] || result[2] > maxs[2]))
		return qfalse;
	return qtrue;
}

qboolean ID_INLINE BoxTraceTestSides( int axis, const vec3_t start, const vec3_t forward, const vec3_t mins, const vec3_t maxs, vec3_t result ) {
	if (forward[axis] > 0 && start[axis] <= mins[axis]) {
		float dist = ( mins[axis] - start[axis] ) / forward[axis];
		if (BoxTraceTestResult(axis, dist, start, forward, mins, maxs, result))
			return qtrue;
	}
	if (forward[axis] < 0 && start[axis] >= maxs[axis]) {
		float dist = ( maxs[axis] - start[axis] ) / forward[axis];
		if (BoxTraceTestResult(axis, dist, start, forward, mins, maxs, result))
			return qtrue;
	}
	return qfalse;
}

qboolean BoxTraceImpact(const vec3_t start, const vec3_t forward, const vec3_t container, vec3_t result ) {
	vec3_t mins, maxs;

	mins[0] = -container[2];
	mins[1] = -container[2];
	mins[2] = container[0];
	maxs[0] = container[2];
	maxs[1] = container[2];
	maxs[2] = container[1];

	if (BoxTraceTestSides(0, start, forward, mins, maxs, result))
		return qtrue;
	if (BoxTraceTestSides(1, start, forward, mins, maxs, result))
		return qtrue;
	if (BoxTraceTestSides(2, start, forward, mins, maxs, result))
		return qtrue;
	return qfalse;
}

qboolean CylinderTraceImpact( const vec3_t start, const vec3_t forward, const vec3_t container, vec3_t result ) {
	float a = forward[0] * forward[0] + forward[1]*forward[1];
	float b = 2*(forward[0]*start[0] + forward[1]*start[1]);
	float c = start[0]*start[0] + start[1]*start[1] - container[2]*container[2];
	float base = b*b - 4*a*c;
	float t,r;

	/* Test if we intersect with the top or bottom */
	if ( (forward[2] > 0 && start[2] <= container[0]) || (forward[2] < 0 && start[2] >= container[1] )) {
		t = (container[0] - start[2] / forward[2]);
		VectorMA( start, t, forward, result );
		if ( (result[0]*result[0] + result[1]*result[1]) < ( container[2] * container[2]))
			return qtrue;
	}

	if (base >=0 ) {
		t = (-b - sqrt( base )) / 2*a;
		VectorMA( start, t, forward, result );
		r = result[0] * result[0] + result[1]*result[1];
		if (result[2] >= container[0] && result[2]<= container[1])
			return qtrue;
	}
	return qfalse;
};

void QuatMultiply( const Quat_t q1, const Quat_t q2, Quat_t qr) {
	qr[Qw] = q1[Qw]*q2[Qw] - q1[Qx]*q2[Qx] - q1[Qy]*q2[Qy] - q1[Qz]*q2[Qz];
	qr[Qx] = q1[Qw]*q2[Qx] + q1[Qx]*q2[Qw] + q1[Qy]*q2[Qz] - q1[Qz]*q2[Qy];
	qr[Qy] = q1[Qw]*q2[Qy] + q1[Qy]*q2[Qw] + q1[Qz]*q2[Qx] - q1[Qx]*q2[Qz];
	qr[Qz] = q1[Qw]*q2[Qz] + q1[Qz]*q2[Qw] + q1[Qx]*q2[Qy] - q1[Qy]*q2[Qx];
}

void QuatFromAxisAngle( const vec3_t axis, vec_t angle, Quat_t dst) {
	float halfAngle = 0.5 * DEG2RAD(angle);
	float fSin = sin(halfAngle);
    dst[Qw] = cos (halfAngle);
    dst[Qx] = fSin*axis[0];
    dst[Qy] = fSin*axis[1];
    dst[Qz] = fSin*axis[2];
}

void QuatFromAngles( const vec3_t angles, Quat_t dst) {
	const float pitchAngle = 0.5 * DEG2RAD(angles[PITCH]);
	const float sinPitch = sin( pitchAngle );
	const float cosPitch = cos( pitchAngle );
	const float yawAngle = 0.5 * DEG2RAD(angles[YAW]);
	const float sinYaw = sin( yawAngle );
	const float cosYaw = cos( yawAngle );
	const float rollAngle = 0.5 * DEG2RAD(angles[ROLL]);
	const float sinRoll = sin( rollAngle );
	const float cosRoll = cos( rollAngle );

	dst[Qx] = sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw;
	dst[Qy] = cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw;
	dst[Qz] = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;
	dst[Qw] = cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw;
}

void QuatFromAnglesClosest( const vec3_t angles, const Quat_t qc, Quat_t dst) {
	Quat_t t0, t1;

	QuatFromAngles( angles, dst );

	QuatAdd( qc, dst, t0 );
	QuatSub( qc, dst, t1 );
	if (QuatLengthSquared( t0 ) < QuatLengthSquared( t1))
		QuatNegate( dst, dst );
}

void QuatToAxisAngle (const Quat_t q, vec3_t axis, float *angle) {
    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)
	float sqrLength = q[Qx]*q[Qx] + q[Qy]*q[Qy] + q[Qz]*q[Qz];
	if ( sqrLength > 0.00001 ) {
		float invLength = 1 / sqrt( sqrLength );
		*angle = 2 * acos( q[Qw] );
		axis[0] = q[Qx] * invLength;
		axis[1] = q[Qy] * invLength;
		axis[2] = q[Qz] * invLength;
    } else {
		*angle = 0;
		axis[0] = 1;
		axis[1] = 0;
		axis[2] = 0;
	}
}

void QuatToAxis( const Quat_t q, vec3_t axis[3]) {
	const float tx = 2 * q[Qx];
	const float ty = 2 * q[Qy];
	const float tz = 2 * q[Qz];
	const float twx = q[Qw] * tx;
	const float twy = q[Qw] * ty;
	const float twz = q[Qw] * tz;
	const float txx = q[Qx] * tx;
	const float txy = q[Qx] * ty;
	const float txz = q[Qx] * tz;
	const float tyy = q[Qy] * ty;
	const float tyz = q[Qy] * tz;
	const float tzz = q[Qz] * tz;

	axis[0][0] = 1 - tyy - tzz;
	axis[1][0] = txy - twz;
	axis[2][0] = txz + twy;
	axis[0][1] = txy + twz;
	axis[1][1] = 1 - txx - tzz;
	axis[2][1] = tyz - twx;
	axis[0][2] = txz - twy;
	axis[1][2] = tyz + twx;
	axis[2][2] = 1 - txx - tyy;
#if 0	
	axis[0][0] = 1 - tyy - tzz;
	axis[0][1] = txy - twz;
	axis[0][2] = txz + twy;
	axis[1][0] = txy + twz;
	axis[1][1] = 1 - txx - tzz;
	axis[1][2] = tyz - twx;
	axis[2][0] = txz - twy;
	axis[2][1] = tyz + twx;
	axis[2][2] = 1 - txx - tyy;
#endif
}

void QuatToAngles( const Quat_t q, vec3_t angles) {
	vec3_t axis[3];
	QuatToAxis( q, axis );
	AxisToAngles( axis, angles );
}

void QuatLog(const Quat_t q0, Quat_t qr) { 
    if ( fabs(q0[Qw]) < 1 ) {
        float fAngle = acos(q0[Qw]);
		float fSin = sin(fAngle);
		if (fabs(fSin) >= 0.00005 ) {
            float fCoeff = fAngle/fSin;
			qr[Qw] = 0;
			qr[Qx] = fCoeff * q0[Qx];
			qr[Qy] = fCoeff * q0[Qy];
			qr[Qz] = fCoeff * q0[Qz];
			return;
        }
    }
	qr[Qw] = 0;
	qr[Qx] = q0[Qx];
	qr[Qy] = q0[Qy];
	qr[Qz] = q0[Qz];
}

void QuatExp(const Quat_t q0, Quat_t qr) {
	float fAngle = sqrt( q0[Qx]*q0[Qx] + q0[Qy]*q0[Qy] + q0[Qz]*q0[Qz]);
	float fSin = sin(fAngle);
	qr[Qw] = cos(fAngle);

    if ( fabs(fSin) >= 0.00005 ) {
		float fCoeff = fSin/fAngle;
		qr[Qx] = fCoeff * q0[Qx];
		qr[Qy] = fCoeff * q0[Qy];
		qr[Qz] = fCoeff * q0[Qz];
    } else {
		qr[Qx] = q0[Qx];
		qr[Qy] = q0[Qy];
		qr[Qz] = q0[Qz];
    }
}

static void QuatSquadPoint(const Quat_t q0, const Quat_t q1, const Quat_t q2, Quat_t qr) {
	Quat_t qi, qt, ql0, ql1;

	QuatConjugate( q1, qi );
	QuatMultiply( qi, q0, qt);
	QuatLog( qt, ql0 );
	QuatMultiply( qi, q2, qt);
	QuatLog( qt, ql1 );
	QuatAdd( ql0, ql1, qt );
	QuatScale( qt, -0.25f, ql0);
	QuatExp( ql0, qt);
	QuatMultiply( q1, qt, qr );
}

void QuatSlerp(float t, const Quat_t q0, const Quat_t q1, Quat_t qr) { 
	float fCos = QuatDot( q0, q1 );
	float angle = acos( fCos );
	if ( fabs(angle) >= 0.00001 ) {
		float invSin = 1 / sin(angle);
        float m0 = sin((1-t)*angle)*invSin;
        float m1 = sin(t*angle)*invSin;
		QuatScale( q0, m0, qr );
		QuatScaleAdd( qr, q1, m1, qr );
    } else {
		QuatCopy( q0, qr );
    }
}

void QuatSquad( float t, const Quat_t q0, const Quat_t q1, const Quat_t q2, const Quat_t q3, Quat_t qr) {  
	Quat_t qp0, qp1, qt0, qt1;

	QuatSquadPoint( q0, q1, q2, qp0);
	QuatSquadPoint( q1, q2, q3, qp1);
	QuatSlerp( t, q1, q2, qt0 );
	QuatSlerp( t, qp0, qp1, qt1 );
	QuatSlerp( 2*t*(1 - t), qt0, qt1, qr );

	QuatNormalize( qr );
}

void QuatTimeSpline( float t, const int times[4], const Quat_t quats[4], Quat_t qr) {
	Quat_t	qn, qt;
	Quat_t	qa, qb;
	Quat_t	qlog10, qlog21, qlog32;
	float fna, fnb; 

	QuatConjugate( quats[0], qn);
	QuatMultiply( qn, quats[1], qt );
	QuatLog( qt, qlog10 );

	QuatConjugate( quats[1], qn);
	QuatMultiply( qn, quats[2], qt );
	QuatLog( qt, qlog21 );

	QuatConjugate( quats[2], qn);
	QuatMultiply( qn, quats[3], qt );
	QuatLog( qt, qlog32 );

	fna = ( times[1] - times[0]) / (0.5 * ( times[2] - times[0]));
	fnb = ( times[3] - times[2]) / (0.5 * ( times[3] - times[1]));

	/* Calc qa */
	QuatScale( qlog10, 1 - 0.5 * fna, qt );
	QuatScaleAdd( qt, qlog21, -0.5 * fna, qt );
	QuatScale( qt, 0.5f, qt );
	QuatExp( qt, qt );
	QuatMultiply( quats[1], qt, qa );

	/* Calc qb */
	QuatScale( qlog21, 0.5*fnb, qt );
	QuatScaleAdd( qt, qlog32, 0.5 * fnb - 1, qt );
	QuatScale( qt, 0.5f, qt );
	QuatExp( qt, qt );
	QuatMultiply( quats[2], qt, qb );

	/* Slerp the final result */
	QuatSlerp( t, quats[1], quats[2], qt );
	QuatSlerp( t, qa, qb, qn );
	QuatSlerp( 2*t*(1 - t), qt, qn, qr );

	QuatNormalize( qr );
}

void VectorTimeSpline( float t, const int times[4], float *control, float *result, int dim) {
	int i;
	float diff10[8];
	float diff21[8];
	float diff32[8];
	float tOut[8];
	float tIn[8];
	float scaleIn, scaleOut;
	
	for (i = 0;i<dim;i++) {
		diff10[i] = control[i + 1*dim] - control[i + 0*dim];
		diff21[i] = control[i + 2*dim] - control[i + 1*dim];
		diff32[i] = control[i + 3*dim] - control[i + 2*dim];
	}
	scaleOut = (float)( times[2] - times[1]) / (float)( times[2] - times[0]);
	scaleIn = (float)( times[2] - times[1]) / (float)( times[3] - times[1]);
	for (i = 0;i<dim;i++) {
		tOut[i] = scaleOut * (diff10[i] + diff21[i]);
		tIn[i] = scaleIn * (diff21[i] + diff32[i]);
	}

	for (i = 0;i<dim;i++) {
		result[i] = ( -2 * diff21[i] + tOut[i] + tIn[i]);
		result[i] *= t;
		result[i] += -1*tIn[i] - 2*tOut[i] + 3 * diff21[i];
		result[i] *= t;
		result[i] += tOut[i];
		result[i] *= t;
		result[i] += control[ i + dim];
	}
}

static float dsMax ( float x, float y ) {
  if ( y > x )
	  return y;
  else 
    return x;
}

static float dsMin ( float x, float y ) {
  if ( y < x )
	  return y;
  else 
    return x;
}

static float dsplineTangent( float h1, float h2, float d1, float d2) {
	float	hsum = h1 + h2;
	float	del1 = d1 / h1;
	float	del2 = d2 / h2;

	if (del1 * del2 == 0) {
		return 0;
	} else {
		float hsumt3 = 3 * hsum;
		float w1 = ( hsum + h1 ) / hsumt3;
		float w2 = ( hsum + h2 ) / hsumt3;
		float dmax = dsMax ( fabs ( del1 ), fabs ( del2 ) );
		float dmin = dsMin ( fabs ( del1 ), fabs ( del2 ) );
		float drat1 = del1 / dmax;
		float drat2 = del2 / dmax;
		return  dmin / ( w1 * drat1 + w2 * drat2 );
	}
}

float dsplineCalc(float x, vec3_t dx, vec3_t dy, float *deriv ) {
	float tan1, tan2;
	float c2, c3;
	float delta, del1, del2;
	
	tan1 = dsplineTangent( dx[0], dx[1], dy[0], dy[1] );
	tan2 = dsplineTangent( dx[1], dx[2], dy[1], dy[2] );

	delta = dy[1] / dx[1];
	del1 =  (tan1 - delta) / dx[1];
	del2 =  (tan2 - delta) / dx[1];
	c2 = - ( del1 + del1 + del2 );
	c3 = (del1 + del2) / dx[1];
	if (deriv) {
		*deriv = tan1 + 2*x*c2 + 3*x*x*c3;
	}
	return  x * ( tan1 + x * ( c2 + x * c3 ) );
}

/* Some line drawing functions */
void demoDrawSetupVerts( polyVert_t *verts, const vec4_t color ) {
	int i;
	for (i = 0; i<4;i++) {
		verts[i].modulate[0] = color[0] * 255;
		verts[i].modulate[1] = color[1] * 255;
		verts[i].modulate[2] = color[2] * 255;
		verts[i].modulate[3] = color[3] * 255;
		verts[i].st[0] = (i & 2) ? 0 : 1;
		verts[i].st[1] = (i & 1) ? 0 : 1;
	}
}

void demoDrawRawLine(const vec3_t start, const vec3_t end, float width, polyVert_t *verts ) {
	vec3_t up;
	vec3_t middle;

	VectorScale( start, 0.5, middle ) ;
	VectorMA( middle, 0.5, end, middle );
	if ( VectorDistance( middle, cg.refdef.vieworg ) < 100 )
		return;
	GetPerpendicularViewVector( cg.refdef.vieworg, start, end, up );
	VectorMA( start, width, up, verts[0].xyz);
	VectorMA( start, -width, up, verts[1].xyz);
	VectorMA( end, -width, up, verts[2].xyz);
	VectorMA( end, width, up, verts[3].xyz);
	trap_R_AddPolyToScene( demo.media.additiveWhiteShader, 4, verts );
}

void demoDrawLine( const vec3_t p0, const vec3_t p1, const vec4_t color) {
	polyVert_t verts[4];

	demoDrawSetupVerts( verts, color );
#if defined DEMO_ANIM && defined _DEBUG
	demoDrawRawLine( p0, p1, 0.1f, verts );
#else
	demoDrawRawLine( p0, p1, 1.0f, verts );
#endif
}

void demoDrawCross( const vec3_t origin, const vec4_t color ) {
	unsigned int i;
	vec3_t start, end;
	polyVert_t verts[4];

	demoDrawSetupVerts( verts, color );
	/* Create a few lines indicating the point */
	for(i = 0; i < 3; i++) {
		VectorCopy(origin, start);
		VectorCopy(origin, end );
		start[i] -= 10;
		end[i] += 10;
		demoDrawRawLine( start, end, 0.6f, verts );
	}
}

void demoDrawBox( const vec3_t origin, const vec3_t container, const vec4_t color ) {
	unsigned int i;
	vec3_t boxCorners[8];
	polyVert_t verts[4];
	static const float xMul[4] = {1,1,-1,-1};
	static const float yMul[4] = {1,-1,-1,1};


	demoDrawSetupVerts( verts, color );
	/* Create the box corners */
	for(i = 0; i < 8; i++) {
		boxCorners[i][0] = origin[0] + (((i+0) & 2) ? +container[2] : -container[2]);
		boxCorners[i][1] = origin[1] + (((i+1) & 2) ? +container[2] : -container[2]);
		boxCorners[i][2] = origin[2] + container[i >>2];
	}
#if defined DEMO_ANIM && defined _DEBUG
	for (i = 0; i < 4;i++) {
		demoDrawRawLine( boxCorners[i], boxCorners[(i+1) & 3], 0.1f, verts );
		demoDrawRawLine( boxCorners[4+i], boxCorners[4+((i+1) & 3)], 0.1f, verts );
		demoDrawRawLine( boxCorners[i], boxCorners[4+i], 0.1f, verts );
	}
#else
	for (i = 0; i < 4;i++) {
		demoDrawRawLine( boxCorners[i], boxCorners[(i+1) & 3], 1.0f, verts );
		demoDrawRawLine( boxCorners[4+i], boxCorners[4+((i+1) & 3)], 1.0f, verts );
		demoDrawRawLine( boxCorners[i], boxCorners[4+i], 1.0f, verts );
	}
#endif
}

void demoDrawCrosshair( void ) {
	float		w, h;
	qhandle_t	hShader;
	float		x, y;
	int			ca;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	w = h = cg_crosshairSize.value;
	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640( &x, &y, &w, &h );
	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];
	trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w), 
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
		w, h, 0, 0, 1, 1, hShader );
}

void demoNowTrajectory( const trajectory_t *tr, vec3_t result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( cg.time - tr->trTime ) * 0.001 + cg.timeFraction * 0.001;
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( cg.time - tr->trTime ) % tr->trDuration;
		deltaTime = ( deltaTime + cg.timeFraction ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( cg.time > tr->trTime + tr->trDuration ) {
			deltaTime = tr->trDuration * 0.001;
		} else {
			deltaTime = ( cg.time - tr->trTime ) * 0.001 + cg.timeFraction * 0.001;
		}
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_NONLINEAR_STOP: //probably ported wrong
		if ( cg.time > tr->trTime + tr->trDuration ) {
			deltaTime = tr->trDuration * 0.001;
		} else {
			deltaTime = ( cg.time - tr->trTime ) * 0.001 + cg.timeFraction * 0.001;
		}
		//new slow-down at end
		if ( deltaTime > tr->trDuration || deltaTime <= 0  ) {
			deltaTime = 0;
		} else {//FIXME: maybe scale this somehow?  So that it starts out faster and stops faster?
			deltaTime = tr->trDuration*0.001f*((float)cos( DEG2RAD(90.0f - (90.0f*(((cg.time-tr->trTime)+cg.timeFraction))/(float)tr->trDuration)) ));
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( cg.time - tr->trTime ) * 0.001 + cg.timeFraction * 0.001;
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "demoNowTrajectory: unknown trType: %i", tr->trType );
		break;
	}
}

void demoRotatePoint(vec3_t point, const vec3_t matrix[3]) { 
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

void demoCreateRotationMatrix(const vec3_t angles, vec3_t matrix[3]) {
	AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
	VectorInverse(matrix[1]);
}
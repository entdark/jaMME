// Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

typedef enum {
	posLinear,
	posCatmullRom,
	posBezier,
	posLast,
} posInterpolate_t;

typedef enum {
	angleLinear,
	angleQuat,
	angleLast,
} angleInterpolate_t;

typedef float Quat_t[4];
void QuatFromAngles( const vec3_t angles, Quat_t dst );
void QuatFromAnglesClosest( const vec3_t angles, const Quat_t qc, Quat_t dst);
void QuatFromAxisAngle( const vec3_t axis, vec_t angle, Quat_t dst);
void QuatToAngles( const Quat_t q, vec3_t angles);
void QuatToAxis( const Quat_t q, vec3_t axis[3]);
void QuatToAxisAngle (const Quat_t q, vec3_t axis, float *angle);
void QuatMultiply( const Quat_t q1, const Quat_t q2, Quat_t qr);
void QuatSlerp(float t,const Quat_t q0,const Quat_t q1, Quat_t qr);
void QuatSquad( float t, const Quat_t q0, const Quat_t q1, const Quat_t q2, const Quat_t q3, Quat_t qr);
void QuatTimeSpline( float t, const int times[4], const Quat_t quats[4], Quat_t qr);
void QuatLog( const Quat_t q0, Quat_t qr );
void QuatExp( const Quat_t q0, Quat_t qr );

void VectorTimeSpline( float t, const int times[4], float *control, float *result, int dim);

#define Vector4Copy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])

static ID_INLINE float QuatDot( const Quat_t q0, const Quat_t q1) {
	return q0[0]*q1[0] + q0[1]*q1[1] + q0[2]*q1[2] + q0[3]*q1[3];
}
static ID_INLINE void QuatCopy( const Quat_t src, Quat_t dst) {
	dst[0] = src[0];dst[1] = src[1];dst[2] = src[2];dst[3] = src[3];
}
static ID_INLINE void QuatNegate( const Quat_t src, Quat_t dst) {
	dst[0] = -src[0];dst[1] = -src[1];dst[2] = -src[2];dst[3] = -src[3];
}
static ID_INLINE void QuatConjugate( const Quat_t src, Quat_t dst) {
	dst[0] = -src[0];dst[1] = -src[1];dst[2] = -src[2];dst[3] = src[3];
}
static ID_INLINE void QuatAdd( const Quat_t q0, const Quat_t q1, Quat_t qr) {
	qr[0] = q0[0] + q1[0];
	qr[1] = q0[1] + q1[1];
	qr[2] = q0[2] + q1[2];
	qr[3] = q0[3] + q1[3];
}
static ID_INLINE void QuatSub( const Quat_t q0, const Quat_t q1, Quat_t qr) {
	qr[0] = q0[0] - q1[0];
	qr[1] = q0[1] - q1[1];
	qr[2] = q0[2] - q1[2];
	qr[3] = q0[3] - q1[3];
}
static ID_INLINE void QuatScale( const Quat_t q0, float scale,Quat_t qr) {
	qr[0] = scale * q0[0];
	qr[1] = scale * q0[1];
	qr[2] = scale * q0[2];
	qr[3] = scale * q0[3];
}
static ID_INLINE void QuatScaleAdd( const Quat_t q0, const Quat_t q1, float scale, Quat_t qr) {
	qr[0] = q0[0] + scale * q1[0];
	qr[1] = q0[1] + scale * q1[1];
	qr[2] = q0[2] + scale * q1[2];
	qr[3] = q0[3] + scale * q1[3];
}
static ID_INLINE float QuatDistanceSquared( const Quat_t q1, const Quat_t q2 ) {
	Quat_t q;
	QuatSub(q1, q2, q);
	return q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
}
static ID_INLINE float QuatLengthSquared( const Quat_t q ) {
	return q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
}
static ID_INLINE float QuatLength( const Quat_t q ) {
	return sqrt( q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
}
static ID_INLINE void QuatClear( Quat_t q ) {
	q[0] = q[1] = q[2] = q[3] = 0;
}

static ID_INLINE float QuatNormalize( Quat_t q ) {
	float ilen, len = QuatLength( q );
	if (len) {
		ilen = 1 / len;
		q[0] *= ilen;
		q[1] *= ilen;
		q[2] *= ilen;
		q[3] *= ilen;
	} else {
		QuatClear( q );
	}
	return len;
}

static ID_INLINE void posMatrix( float t, posInterpolate_t type, vec4_t matrix) {
	switch ( type ) {
	case posLinear:
		matrix[0] = 0;
		matrix[1] = 1-t;
		matrix[2] = t;
		matrix[3] = 0;
		break;
	case posCatmullRom:
		matrix[0] = 0.5f * ( -t + 2*t*t - t*t*t );
		matrix[1] = 0.5f * ( 2 - 5*t*t + 3*t*t*t );
		matrix[2] = 0.5f * ( t + 4*t*t - 3*t*t*t );
		matrix[3] = 0.5f * (-t*t + t*t*t);
		break;
	default:
	case posBezier:
		matrix[0] = (((-t+3)*t-3)*t+1)/6;
		matrix[1] = (((3*t-6)*t)*t+4)/6;
		matrix[2] = (((-3*t+3)*t+3)*t+1)/6;
		matrix[3] = (t*t*t)/6;
		break;
	}
}

static ID_INLINE void posGet( float t, posInterpolate_t type, const vec3_t control[4], vec3_t out) {
	vec4_t matrix;

	posMatrix( t, type, matrix );
	VectorScale( control[0], matrix[0], out );
	VectorMA( out, matrix[1], control[1], out );
	VectorMA( out, matrix[2], control[2], out );
	VectorMA( out, matrix[3], control[3], out );
}

static ID_INLINE vec_t VectorDistanceSquared ( const vec3_t v1, const vec3_t v2 ) {
	vec3_t v;
	VectorSubtract( v1, v2, v );
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}
static ID_INLINE vec_t VectorDistance( const vec3_t v1, const vec3_t v2 ) {
	vec3_t v;
	VectorSubtract( v1, v2, v );
	return (vec_t)sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}
static ID_INLINE void AnglesNormalize360( vec3_t angles) {
	angles[0] = AngleNormalize360(angles[0]);
	angles[1] = AngleNormalize360(angles[1]);
	angles[2] = AngleNormalize360(angles[2]);
}
static ID_INLINE void AnglesNormalize180( vec3_t angles) {
	angles[0] = AngleNormalize180(angles[0]);
	angles[1] = AngleNormalize180(angles[1]);
	angles[2] = AngleNormalize180(angles[2]);
}

#define VectorAddDelta(a,b,c)	((c)[0]=(b)[0]+((b)[0]-(a)[0]),(c)[1]=(b)[1]+((b)[1]-(a)[1]),(c)[2]=(b)[2]+((b)[2]-(a)[2]))
#define VectorSubDelta(a,b,c)	((c)[0]=(a)[0]-((b)[0]-(a)[0]),(c)[1]=(a)[1]-((b)[1]-(a)[1]),(c)[2]=(a)[2]-((b)[2]-(a)[2]))

qboolean CylinderTraceImpact( const vec3_t start, const vec3_t forward, const vec3_t container, vec3_t result );
qboolean BoxTraceImpact(const vec3_t start, const vec3_t forward, const vec3_t container, vec3_t result );

qboolean MatrixCompare(const mdxaBone_t m1, const mdxaBone_t m2);
void AxisToMatrix(const vec3_t axis[3], mdxaBone_t *matrix);
void MatrixCreate(const float *angle, mdxaBone_t *matrix);
void MatrixInverse3x3(mdxaBone_t *m);
void MatrixMultiply3x4(mdxaBone_t *out, const mdxaBone_t *in2, const mdxaBone_t *in);
void MatrixAxisToAngles( const vec3_t axis[3], vec3_t angles );

float dsplineCalc(float x, vec3_t dx, vec3_t dy, float*deriv );

void demoDrawCross( const vec3_t origin, const vec4_t color );
void demoDrawBox( const vec3_t origin, const vec3_t container, const vec4_t color );
void demoDrawLine( const vec3_t p0, const vec3_t p1, const vec4_t color);
void demoDrawRawLine(const vec3_t start, const vec3_t end, float width, polyVert_t *verts );
void demoDrawSetupVerts( polyVert_t *verts, const vec4_t color );

void demoRotatePoint(vec3_t point, const vec3_t matrix[3]);
void demoCreateRotationMatrix(const vec3_t angles, vec3_t matrix[3]);

static ID_INLINE int fxRand( int r ) {
	r = (69069 * r + 1);
	return r;
}
static ID_INLINE float fxRandom( int r ) {
	return (fxRand(r) & 0xffff) * (1.0f / 0x10000);
}
static ID_INLINE int fxRandomSign( int r ) {
	float sign;
	sign = fxRandom(r);
	return sign = (sign > 0.5) ? 1 : -1;
}

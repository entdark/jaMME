#pragma once
#include "cg_local.h"

void CG_MultiSpecMain(void);
void CG_MultiSpec_f(void);
void CG_MultiSpecInit(void);
void CG_MultiSpecShutDown(void);
void CG_MultiSpecAdjust2D(float *x, float *y, float *w, float *h);
void CG_MultiSpecAdjustString(float *x, float *y, float *scale);
qboolean CG_MultiSpecActive(void);
qboolean CG_MultiSpecSkipBackground(void);
void CG_MultiSpecDrawBackground(void);
void CG_MultiSpecDynamicGlowUpdate(void);
qboolean CG_MultiSpecEditing(void);
float CG_MultiSpecFov(void);
qboolean CG_MultiSpecTeamOverlayReplace(void);
qboolean CG_MultiSpecTeamOverlayOffset(void);
int CG_MultiSpecKeyEvent(int key, qboolean down);
void CG_MultiSpecMouseEvent(int dx, int dy);
// Copyright (C) 2005 Eugene Bujak.
//
// cl_mme.c -- Movie Maker's Edition client-side work

#include "client.h"

//extern cvar_t *cl_avidemo;
//extern cvar_t *cl_forceavidemo;

// MME cvars
//cvar_t	*mme_anykeystopsdemo;
cvar_t	*mme_saveWav;
//cvar_t	*mme_gameOverride;
cvar_t	*mme_demoConvert;
cvar_t	*mme_demoSmoothen;
cvar_t	*mme_demoFileName;
cvar_t	*mme_demoStartProject;
cvar_t	*mme_demoListQuit;
cvar_t	*mme_demoAutoQuit;
cvar_t	*mme_demoRemove;
cvar_t	*mme_demoPrecache;
cvar_t	*mme_demoAutoNext;
cvar_t	*mme_demoPaused;
cvar_t	*mme_demoEscapeQuit;
void CL_MME_CheckCvarChanges(void) {
	if (cl_avidemo->modified) {
		cl_avidemo->modified = qfalse;
		clc.aviDemoRemain = 0;
	}
	if ( cls.state == CA_DISCONNECTED)
		CL_DemoListNext_f();
}


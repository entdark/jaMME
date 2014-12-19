// Nerevar's way to produce weather
#include "cg_demos.h" 

void demoDrawRain(void) {
	float backFactor = demo.rain.back ? 1.0f : 2.0f;
	float rainRange, rainNumber;
	sfxHandle_t sfx;
	if (demo.rain.number <= 0 || !demo.rain.active)
		return;
	rainRange = demo.rain.range / backFactor;
	rainNumber = (float)demo.rain.number * (rainRange / 1000.0f);
	if (demo.rain.number >= 1000 / backFactor) {
		sfx = demo.media.heavyRain;
	} else if (demo.rain.number >= 400 / backFactor) {
		sfx = demo.media.regularRain;
	} else {
		sfx = demo.media.lightRain;
	}
	trap_S_AddLoopingSound(ENTITYNUM_NONE, cg.refdef.vieworg, vec3_origin, sfx);
	if (demo.rain.time <= cg.time) {
		int i;						
		for (i = 0; i < (int)rainNumber; i++) {
			vec3_t start, end;
			trace_t tr;
				
			float angle = cg.refdef.viewangles[YAW];
			int seed = trap_Milliseconds() - i * 15;
			float range = Q_irand(0, 2.0f * rainRange) - rainRange;				
				
			if (!demo.rain.back) {
				vec3_t fwd;
				AngleVectors(cg.refdef.viewangles, fwd, NULL, NULL);	 //<-Viewangles YAW		
				VectorMA(cg.refdef.vieworg, rainRange, fwd, start);
			} else {
				VectorCopy(cg.refdef.vieworg, start);
			}
			start[2] = cg.refdef.vieworg[2];
				
			start[0] = start[0] + (cos(angle)*range);
			start[1] = start[1] + (sin(angle)*range);
				
			angle += DEG2RAD(90);
			range = Q_random(&seed) * 2.0f * rainRange - rainRange;
				
			start[0] = start[0] + (cos(angle)*range);
			start[1] = start[1] + (sin(angle)*range);
				
			//GET SKY
					
			VectorCopy(start,end);
			end[2] += 9999;				
					
			CG_Trace(&tr, start, NULL, NULL, end, 0, MASK_SOLID);
				
			if (tr.surfaceFlags & SURF_SKY) {
				vec3_t angles;
				if (tr.endpos[2] - start[2] > 200)
					tr.endpos[2] = start[2] + 200;
				tr.endpos[2] += Q_crandom(&seed) * 100.0f;
				angles[PITCH] = 0;
				angles[YAW] = 90;
				angles[ROLL] = 0;
				trap_FX_PlayEffectID(cgs.effects.rain, tr.endpos, angles, -1, 1);
			}
		}			
		demo.rain.time = cg.time + 30;				
	}
}

static qboolean demoTraceToNotSky(trace_t *tr, const vec3_t start, const vec3_t end, const float size) {
	int i;
	/* scan 4 corners, if any of them isn't sky, then return and let's search with lower size */
	for (i = 0; i < 4; i++) {
		vec3_t offset, trStart, trEnd;
		if (i == 0) VectorSet(offset, -size, -size, 0);
		if (i == 1) VectorSet(offset, -size, size, 0);
		if (i == 2) VectorSet(offset, size, -size, 0);
		if (i == 3) VectorSet(offset, size, size, 0);
		VectorSet(trStart, start[0] + offset[0], start[1] + offset[1], start[2]);
		VectorSet(trEnd, end[0] + offset[0], end[1] + offset[1], end[2]);
		CG_Trace(tr, trStart, NULL, NULL, trEnd, 0, MASK_SHOT);
		if (!(tr->surfaceFlags & SURF_SKY)) {
			return qtrue;
		}
	}
	return qfalse;
}
void demoDrawSun(void) {
	int i;
	float size = 0.0f;
	vec3_t origin, forward;
	trace_t tr;
	refEntity_t re;
	
	if (!demo.sun.active)
		return;
	
	AngleVectors(demo.sun.angles, forward, NULL, NULL);
	VectorMA(demo.viewOrigin, 9999, forward, origin);

	//CALCULATE SIZE //eats too much fps
	CG_Trace(&tr, cg.refdef.vieworg, NULL, NULL, origin, 0, MASK_SHOT);
	if (!(tr.surfaceFlags & SURF_SKY)) //not even worth it
		return;
	
	for (i = 31; i >= 0; i--) {
		qboolean notSky = demoTraceToNotSky(&tr, cg.refdef.vieworg, origin, i);
		size = i;
		if (notSky)
			continue;
		if (tr.surfaceFlags & SURF_SKY) {
			float f, step = 1.0f / demo.sun.precision;
			for (f = (float)(i + 1); f > (float)(i); f -= step) {
				notSky = demoTraceToNotSky(&tr, cg.refdef.vieworg, origin, f);
				size = f;
				if (notSky)
					continue;
				if (tr.surfaceFlags & SURF_SKY) {
					break;
				}
			}
			break;
		}
	}
	size *= demo.sun.size * 0.5f;
	if (size <= 0.0f)
		return;
	memset(&re, 0, sizeof(refEntity_t));
	re.reType = RT_SPRITE;
	re.customShader = cgs.media.saberFlare;	
	VectorCopy(tr.endpos,re.origin);
		
	re.radius = size * Distance(re.origin, cg.refdef.vieworg) / 20.0f;
	re.renderfx = RF_NODEPTH | RF_MINLIGHT;	
				
	re.shaderRGBA[0] = 255;
	re.shaderRGBA[1] = 230; //0.9
	re.shaderRGBA[2] = 191; //0.75
	re.shaderRGBA[3] = 255; 
		
	trap_R_AddRefEntityToScene( &re );
}

static qboolean sunParseActive( BG_XMLParse_t *parse,const char *line, void *data) {
	demo.sun.active = atoi( line );
	return qtrue;
}
static qboolean sunParseSize( BG_XMLParse_t *parse,const char *line, void *data) {
	demo.sun.size = atof( line );
	return qtrue;
}
static qboolean sunParsePrecision(BG_XMLParse_t *parse,const char *line, void *data) {
	demo.sun.precision = atof( line );
	return qtrue;
}
static qboolean sunParseYaw(BG_XMLParse_t *parse,const char *line, void *data) {
	demo.sun.angles[YAW] = atof( line );
	return qtrue;
}
static qboolean sunParsePitch(BG_XMLParse_t *parse,const char *line, void *data) {
	demo.sun.angles[PITCH] = -atof( line );
	return qtrue;
}

static qboolean sunParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t sunParseBlock[] = {
		{"active",	0,					sunParseActive },
		{"size",	0,					sunParseSize },
		{"precision",	0,				sunParsePrecision },
		{"yaw",	0,						sunParseYaw },
		{"pitch",	0,					sunParsePitch },
		{0, 0, 0}
	};

	if (!BG_XMLParse( parse, fromBlock, sunParseBlock, data ))
		return qfalse;

	return qtrue;
}

static qboolean rainParseActive( BG_XMLParse_t *parse,const char *line, void *data) {
	demo.rain.active = atoi( line );
	return qtrue;
}
static qboolean rainParseNumber( BG_XMLParse_t *parse,const char *line, void *data) {
	demo.rain.number = atoi( line );
	return qtrue;
}
static qboolean rainParseRange(BG_XMLParse_t *parse,const char *line, void *data) {
	demo.rain.range = atof( line );
	return qtrue;
}
static qboolean rainParseBack(BG_XMLParse_t *parse,const char *line, void *data) {
	demo.rain.back = atoi( line );
	return qtrue;
}

static qboolean rainParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t rainParseBlock[] = {
		{"active",	0,					rainParseActive },
		{"number",	0,					rainParseNumber },
		{"range",	0,					rainParseRange },
		{"back",	0,					rainParseBack },
		{0, 0, 0}
	};

	if (!BG_XMLParse( parse, fromBlock, rainParseBlock, data ))
		return qfalse;

	return qtrue;
}

qboolean weatherParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t weatherParseBlock[] = {
		{"sun",		sunParse,	0 },
		{"rain",	rainParse,	0 },
		{0, 0, 0}
	};

	if (!BG_XMLParse( parse, fromBlock, weatherParseBlock, data ))
		return qfalse;

	return qtrue;
}

void weatherSave( fileHandle_t fileHandle ) {
	demoSaveLine( fileHandle, "<weather>\n" );
		demoSaveLine( fileHandle, "\t<sun>\n" );
			demoSaveLine( fileHandle, "\t\t<active>%d</active>\n", demo.sun.active );
			demoSaveLine( fileHandle, "\t\t<size>%9.4f</size>\n", demo.sun.size );
			demoSaveLine( fileHandle, "\t\t<precision>%9.4f</precision>\n", demo.sun.precision );
			demoSaveLine( fileHandle, "\t\t<yaw>%9.4f</yaw>\n", demo.sun.angles[YAW] );
			demoSaveLine( fileHandle, "\t\t<pitch>%9.4f</pitch>\n", -demo.sun.angles[PITCH] );
		demoSaveLine( fileHandle, "\t</sun>\n" );	
		demoSaveLine( fileHandle, "\t<rain>\n" );
			demoSaveLine( fileHandle, "\t\t<active>%d</active>\n", demo.rain.active );
			demoSaveLine( fileHandle, "\t\t<number>%d</number>\n", demo.rain.number );
			demoSaveLine( fileHandle, "\t\t<range>%9.4f</range>\n", demo.rain.range );
			demoSaveLine( fileHandle, "\t\t<back>%d</back>\n", demo.rain.back );
		demoSaveLine( fileHandle, "\t</rain>\n" );
	demoSaveLine( fileHandle, "</weather>\n" );
}

void demoSunCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "on") || !Q_stricmp(cmd, "enable") || !Q_stricmp(cmd, "e")
		|| !Q_stricmp(cmd, "activate") || !Q_stricmp(cmd, "a")) {
		demo.sun.active = !demo.sun.active;
		if (demo.sun.active) 
			CG_DemosAddLog("Sun enabled");
		else 
			CG_DemosAddLog("Sun disabled");
	} else if (!Q_stricmp(cmd, "size") || !Q_stricmp(cmd, "scale") || !Q_stricmp(cmd, "s")) {
		if (trap_Argc() > 2) {
			demo.sun.size = atof(CG_Argv(2));
			if (demo.sun.size < 0.0f)
				demo.sun.size = 0.0f;
		}
		CG_DemosAddLog( "Sun size %.1f", demo.sun.size );
	} else if (!Q_stricmp(cmd, "precision") || !Q_stricmp(cmd, "pr")) {
		if (trap_Argc() > 2) {
			demo.sun.precision = atof(CG_Argv(2));
			if (demo.sun.precision < 1.0f)
				demo.sun.precision = 1.0f;
		}
		CG_DemosAddLog( "Sun scaling precision %.1f", demo.sun.precision );
	} else if (!Q_stricmp(cmd, "yaw") || !Q_stricmp(cmd, "y")) {
		if (trap_Argc() > 2) {
			demo.sun.angles[YAW] = atof(CG_Argv(2));
		}
		CG_DemosAddLog( "Sun yaw %.1f degrees", AngleNormalize360(demo.sun.angles[YAW]) );
	} else if (!Q_stricmp(cmd, "pitch") || !Q_stricmp(cmd, "p")) {
		if (trap_Argc() > 2) {
			demo.sun.angles[PITCH] = -atof(CG_Argv(2));
		}
		CG_DemosAddLog( "Sun pitch %.1f degrees", AngleNormalize360(-demo.sun.angles[PITCH]) );
	} else {
		Com_Printf("sun usage:\n" );
		Com_Printf("sun on/enable/e/activate/a, enable/disable sun effect\n" );
		Com_Printf("sun size/scale/s 0, change sun size\n");
		Com_Printf("sun precision/pr 0, how smoothly sun fades near borders\n");
		Com_Printf("sun yaw/y 0, left/right sun rotation\n" );
		Com_Printf("sun pitch/p 0, up/down sun rotation\n" );
		return;
	}
}

void demoRainCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "on") || !Q_stricmp(cmd, "enable") || !Q_stricmp(cmd, "e")
		|| !Q_stricmp(cmd, "activate") || !Q_stricmp(cmd, "a")) {
		demo.rain.active = !demo.rain.active;
		if (demo.rain.active) 
			CG_DemosAddLog("Rain enabled");
		else 
			CG_DemosAddLog("Rain disabled");
	} else if (!Q_stricmp(cmd, "number") || !Q_stricmp(cmd, "amount")
		|| !Q_stricmp(cmd, "quantity") || !Q_stricmp(cmd, "num") || !Q_stricmp(cmd, "n")) {
		if (trap_Argc() > 2) {
			demo.rain.number = atoi(CG_Argv(2));
			if (demo.rain.number < 0)
				demo.rain.number = 0;
			else if (demo.rain.number > 2000)
				demo.rain.number = 2000;
		}
		CG_DemosAddLog( "Rain amount %d", demo.rain.number );
	} else if (!Q_stricmp(cmd, "range") || !Q_stricmp(cmd, "distance")
		|| !Q_stricmp(cmd, "r") || !Q_stricmp(cmd, "d")) {
		if (trap_Argc() > 2) {
			demo.rain.range = atof(CG_Argv(2));
			if (demo.rain.range < 0.0f)
				demo.rain.range = 0.0f;
			else if (demo.rain.range > 9999.9f)
				demo.rain.range = 9999.9f;
		}
		CG_DemosAddLog( "Rain range %.1f", demo.rain.range );
	} else if (!Q_stricmp(cmd, "back") || !Q_stricmp(cmd, "behind") || !Q_stricmp(cmd, "b")) {
		demo.rain.back = !demo.rain.back;
		if (demo.rain.back) 
			CG_DemosAddLog("Drawing rain around");
		else 
			CG_DemosAddLog("Drawing rain in front");
	} else {
		Com_Printf("rain usage:\n" );
		Com_Printf("rain on/enable/e/activate/a, enable/disable rain effect\n" );
		Com_Printf("rain number/amount/quantity/num/n 0, number of rain drops\n");
		Com_Printf("rain range/r/distance/d 0, how far rain can appear\n");
		Com_Printf("rain back/behind/b, additionally draw rain behind camera\n");
		return;
	}
}

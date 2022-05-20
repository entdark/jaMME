// Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

#include "cg_demos.h"

static demoScriptPoint_t *scriptPointAlloc(void) {
	demoScriptPoint_t * point = (demoScriptPoint_t *)malloc(sizeof (demoScriptPoint_t));
	if (!point)
		CG_Error("Out of memory");
	memset( point, 0, sizeof( demoScriptPoint_t ));
	return point;
}

static void scriptPointFree(demoScriptPoint_t *point) {
	if (point->prev) 
		point->prev->next = point->next;
	else 
		demo.script.points = point->next;
	if (point->next ) 
		point->next->prev = point->prev;
	free( point );
}

demoScriptPoint_t *scriptPointSynch( int time ) {
	demoScriptPoint_t *point = demo.script.points;
	if (!point)
		return 0;
	for( ;point->next && point->next->time <= time; point = point->next);
	return point;
}

static demoScriptPoint_t *scriptPointAdd( int time ) {
	demoScriptPoint_t *point = scriptPointSynch( time );
	demoScriptPoint_t *newPoint;
	if (!point || point->time > time) {
		newPoint = scriptPointAlloc();
		newPoint->next = demo.script.points;
		if (demo.script.points)
			demo.script.points->prev = newPoint;
		demo.script.points = newPoint;
		newPoint->prev = 0;
	} else if (point->time == time) {
		newPoint = point;
	} else {
		newPoint = scriptPointAlloc();
		newPoint->prev = point;
		newPoint->next = point->next;
		if (point->next)
			point->next->prev = newPoint;
		point->next = newPoint;
	}
	newPoint->time = time;
	return newPoint;
}

static qboolean scriptPointDel( int playTime ) {
	demoScriptPoint_t *point = scriptPointSynch( playTime );

	if (!point || !point->time == playTime)
		return qfalse;
	scriptPointFree( point );
	return qtrue;
}

static void scriptClear( void ) {
	demoScriptPoint_t *next, *point = demo.script.points;
	while ( point ) {
		next = point->next;
		scriptPointFree( point );
		point = next;
	}
}

typedef struct {
	char init[DEMO_SCRIPT_SIZE];
	char run[DEMO_SCRIPT_SIZE];
	int time;
} parseScriptPoint_t;

static qboolean scriptParseRun( BG_XMLParse_t *parse,const char *line, void *data) {
	parseScriptPoint_t *point=(parseScriptPoint_t *)data;
	Q_strncpyz( point->run, line, sizeof( point->run ));
	return qtrue;
}
static qboolean scriptParseInit( BG_XMLParse_t *parse,const char *line, void *data) {
	parseScriptPoint_t *point=(parseScriptPoint_t *)data;
	Q_strncpyz( point->init, line, sizeof( point->init ));
	return qtrue;
}

static qboolean scriptParseTime( BG_XMLParse_t *parse,const char *line, void *data) {
	parseScriptPoint_t *point=(parseScriptPoint_t *)data;
	if (!line[0])
		return qfalse;
	point->time = atoi( line );
	return qtrue;
}
static qboolean scriptParsePoint( BG_XMLParse_t *parse,const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	parseScriptPoint_t pointLoad;
	demoScriptPoint_t *point;
	static BG_XMLParseBlock_t scriptParseBlock[] = {
		{"time", 0, scriptParseTime},
		{"run", 0, scriptParseRun},
		{"init", 0, scriptParseInit},
		{0, 0, 0}
	};
	memset( &pointLoad, 0, sizeof( pointLoad ));
	if (!BG_XMLParse( parse, fromBlock, scriptParseBlock, &pointLoad )) {
		return qfalse;
	}
	point = scriptPointAdd( pointLoad.time );
	Q_strncpyz( point->run, pointLoad.run, sizeof( point->run ));
	Q_strncpyz( point->init, pointLoad.init, sizeof( point->init ));
	return qtrue;
}
static qboolean scriptParseLocked( BG_XMLParse_t *parse,const char *line, void *data) {
	parseScriptPoint_t *point= (parseScriptPoint_t *)data;
	if (!line[0])
		return qfalse;
	demo.script.locked = atoi( line );
	return qtrue;
}
qboolean scriptParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t scriptParseBlock[] = {
		{"point",	scriptParsePoint,	0},
		{"locked",	0,					scriptParseLocked },
		{0, 0, 0}
	};

	scriptClear();
	if (!BG_XMLParse( parse, fromBlock, scriptParseBlock, data ))
		return qfalse;
	return qtrue;
}

void scriptSave( fileHandle_t fileHandle ) {
	demoScriptPoint_t *point;

	point = demo.script.points;
	demoSaveLine( fileHandle, "<script>\n" );
	demoSaveLine( fileHandle, "\t<locked>%d</locked>\n", demo.script.locked );
	while (point) {
		demoSaveLine( fileHandle, "\t<point>\n");
		demoSaveLine( fileHandle, "\t\t<time>%10d</time>\n", point->time );
		demoSaveLine( fileHandle, "\t\t<run>%s</run>\n", point->run );
		demoSaveLine( fileHandle, "\t\t<init>%s</init>\n", point->init );
		demoSaveLine( fileHandle, "\t</point>\n");
		point = point->next;
	}
	demoSaveLine( fileHandle, "</script>\n" );
}

static void scriptExecRun( demoScriptPoint_t *point ) {
	if ( strlen( point->run ) )
		trap_SendConsoleCommand( va( "%s\n", point->run ) );
}

static void scriptExecInit( demoScriptPoint_t *point ) {
	if ( strlen( point->init ) )
		trap_SendConsoleCommand( va( "%s\n", point->init ) );
}

void scriptRun( qboolean hadSkip ) {
	if ( !demo.script.locked )
		return;
	if ( hadSkip ) {
		demoScriptPoint_t *skipPoint = demo.script.points;
		while ( skipPoint && skipPoint->time <= demo.play.time ) {
			scriptExecInit( skipPoint );
			skipPoint = skipPoint->next;
		}
	} else {
		demoScriptPoint_t *point = scriptPointSynch( demo.play.time );
		if ( point && demo.play.oldTime < point->time && demo.play.time >= point->time ) {
			scriptExecInit( point );
			scriptExecRun( point );
		}
	}
}

void demoScriptCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "lock")) {
		demo.script.locked = !demo.script.locked;
		if (demo.script.locked) 
			CG_DemosAddLog("script locked");
		else 
			CG_DemosAddLog("script unlocked");
	} else if (!Q_stricmp(cmd, "add")) {
		demoScriptPoint_t *point = scriptPointAdd( demo.play.time );
		if (point) {
			Q_strncpyz( point->run, CG_Argv(2), sizeof( point->run ));
			Q_strncpyz( point->init, CG_Argv(3), sizeof( point->init ));
			CG_DemosAddLog("Added script point");
		} else {
			CG_DemosAddLog("Failed to add script point");
		}
	} else if (!Q_stricmp(cmd, "del")) {
		if (scriptPointDel( demo.play.time)) {
			CG_DemosAddLog("Deleted script point");
		} else {
			CG_DemosAddLog("Failed to delete script point");
		}
	} else if (!Q_stricmp(cmd, "clear")) {
		scriptClear();
	} else if (!Q_stricmp(cmd, "next")) {
		demoScriptPoint_t *point = scriptPointSynch( demo.play.time );
		if (!point)
			return;
		if (point->next)
			point = point->next;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else if (!Q_stricmp(cmd, "prev")) {
		demoScriptPoint_t *point = scriptPointSynch( demo.play.time );
		if (!point)
			return;
		if (point->prev)
			point = point->prev;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else {
		Com_Printf("script usage:\n" );
		Com_Printf("script add/del, add/del a script point at current time.\n" );
		Com_Printf("script clear, clear all script points. (Use with caution...)\n" );
		Com_Printf("script lock, lock script to use the keypoints\n" );
		Com_Printf("script next/prev, go to time index of next or previous point.\n" );
	}
}

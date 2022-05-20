// Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

#include "cg_demos.h"

static demoBookPoint_t *bookPointAlloc(void) {
	demoBookPoint_t * point = (demoBookPoint_t *)malloc(sizeof (demoBookPoint_t));
	if (!point)
		CG_Error("Out of memory");
	memset( point, 0, sizeof( demoBookPoint_t ));
	return point;
}

static void bookPointFree(demoBookPoint_t *point) {
	if (point->prev) 
		point->prev->next = point->next;
	else 
		demo.book.points = point->next;
	if (point->next ) 
		point->next->prev = point->prev;
	free( point );
}

demoBookPoint_t *bookPointSynch( int time ) {
	demoBookPoint_t *point = demo.book.points;
	if (!point)
		return 0;
	for( ;point->next && point->next->time <= time; point = point->next);
	return point;
}

static demoBookPoint_t *bookPointAdd( int time ) {
	demoBookPoint_t *point = bookPointSynch( time );
	demoBookPoint_t *newPoint;
	if (!point || point->time > time) {
		newPoint = bookPointAlloc();
		newPoint->next = demo.book.points;
		if (demo.book.points)
			demo.book.points->prev = newPoint;
		demo.book.points = newPoint;
		newPoint->prev = 0;
	} else if (point->time == time) {
		newPoint = point;
	} else {
		newPoint = bookPointAlloc();
		newPoint->prev = point;
		newPoint->next = point->next;
		if (point->next)
			point->next->prev = newPoint;
		point->next = newPoint;
	}
	newPoint->time = time;
	return newPoint;
}

static qboolean bookPointDel( int playTime ) {
	demoBookPoint_t *point = bookPointSynch( playTime );

	if (!point || !point->time == playTime)
		return qfalse;
	bookPointFree( point );
	return qtrue;
}

static void bookClear( void ) {
	demoBookPoint_t *next, *point = demo.book.points;
	while ( point ) {
		next = point->next;
		bookPointFree( point );
		point = next;
	}
}

typedef struct {
	int time;
} parseBookPoint_t;

static qboolean bookParseTime( BG_XMLParse_t *parse,const char *line, void *data) {
	parseBookPoint_t *point=(parseBookPoint_t *)data;
	if (!line[0])
		return qfalse;
	point->time = atoi( line );
	return qtrue;
}
static qboolean bookParsePoint( BG_XMLParse_t *parse,const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	parseBookPoint_t pointLoad;
	static BG_XMLParseBlock_t bookParseBlock[] = {
		{"time", 0, bookParseTime},
		{0, 0, 0}
	};
	memset( &pointLoad, 0, sizeof( pointLoad ));
	if (!BG_XMLParse( parse, fromBlock, bookParseBlock, &pointLoad )) {
		return qfalse;
	}
	bookPointAdd( pointLoad.time );
	return qtrue;
}
qboolean bookParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t bookParseBlock[] = {
		{"point",	bookParsePoint,	0},
		{0, 0, 0}
	};

	bookClear();
	if (!BG_XMLParse( parse, fromBlock, bookParseBlock, data ))
		return qfalse;
	return qtrue;
}

void bookSave( fileHandle_t fileHandle ) {
	demoBookPoint_t *point;

	point = demo.book.points;
	demoSaveLine( fileHandle, "<book>\n" );
	while (point) {
		demoSaveLine( fileHandle, "\t<point>\n");
		demoSaveLine( fileHandle, "\t\t<time>%10d</time>\n", point->time );
		demoSaveLine( fileHandle, "\t</point>\n");
		point = point->next;
	}
	demoSaveLine( fileHandle, "</book>\n" );
}

void demoBookCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "add")) {
		demoBookPoint_t *point = bookPointAdd( demo.play.time );
		if (point) {
			CG_DemosAddLog("Added bookmark point");
		} else {
			CG_DemosAddLog("Failed to add bookmark point");
		}
	} else if (!Q_stricmp(cmd, "del")) {
		if (bookPointDel( demo.play.time)) {
			CG_DemosAddLog("Deleted bookmark point");
		} else {
			CG_DemosAddLog("Failed to delete bookmark point");
		}
	} else if (!Q_stricmp(cmd, "clear")) {
		bookClear();
	} else if (!Q_stricmp(cmd, "next")) {
		demoBookPoint_t *point = bookPointSynch( demo.play.time );
		if (!point)
			return;
		if (point->next)
			point = point->next;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else if (!Q_stricmp(cmd, "prev")) {
		demoBookPoint_t *point = bookPointSynch( demo.play.time );
		if (!point)
			return;
		if (point->prev)
			point = point->prev;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else {
		Com_Printf("book usage:\n" );
		Com_Printf("book add/del, add/del a bookmark point at current time.\n" );
		Com_Printf("book clear, clear all bookmark points. (Use with caution...)\n" );
		Com_Printf("book next/prev, go to time index of next or previous point.\n" );
	}
}

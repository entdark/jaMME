// common.c -- misc functions used in client and server

//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "GenericParser2.h"
#include "stringed_ingame.h"
#include "../qcommon/game_version.h"
#ifndef __linux__
#include "../qcommon/platform.h"
#endif

#include "../server/NPCNav/navigator.h"

#ifdef USE_AIO
#include <pthread.h>
#endif

#if defined (_WIN32)
#if defined (DEDICATED)
#include "../null/win_local.h"
#else
#include "../win32/win_local.h"
#endif
#elif defined (__ANDROID__)
#include "../android/sys_local.h"
#else
#include "../sys/sys_local.h"
#endif

#define	MAXPRINTMSG	4096

#define MAX_NUM_ARGVS	50

int		com_argc;
char	*com_argv[MAX_NUM_ARGVS+1];


FILE *debuglogfile;
fileHandle_t logfile;
fileHandle_t	com_journalFile;			// events are written here
fileHandle_t	com_journalDataFile;		// config files are written here

cvar_t	*com_viewlog;
cvar_t	*com_speeds;
cvar_t	*com_developer;
cvar_t	*com_vmdebug;
cvar_t	*com_dedicated;
cvar_t	*com_timescale;
cvar_t	*com_fixedtime;
cvar_t	*com_dropsim;		// 0.0 to 1.0, simulated packet drops
cvar_t	*com_journal;
cvar_t	*com_maxfps;
cvar_t	*com_timedemo;
cvar_t	*com_sv_running;
cvar_t	*com_cl_running;
cvar_t	*com_logfile;		// 1 = buffer log, 2 = flush after each print
cvar_t	*com_showtrace;

cvar_t	*com_optvehtrace;

#ifdef G2_PERFORMANCE_ANALYSIS
cvar_t	*com_G2Report;
#endif

cvar_t	*com_terrainPhysics; //rwwRMG - added

cvar_t	*com_version;
cvar_t	*com_buildScript;	// for automated data building scripts
cvar_t	*com_introPlayed;
cvar_t	*cl_paused;
cvar_t	*sv_paused;
cvar_t	*com_cameraMode;
cvar_t	*com_unfocused;
cvar_t	*com_minimized;
cvar_t  *com_homepath;
#if defined(_WIN32) && defined(_DEBUG)
cvar_t	*com_noErrorInterrupt;
#endif
cvar_t	*com_affinity;

cvar_t	*com_RMG;

// com_speeds times
int		time_game;
int		time_frontend;		// renderer frontend time
int		time_backend;		// renderer backend time

int			com_frameTime;
int			com_frameMsec;
int			com_frameNumber;

qboolean	com_errorEntered;
qboolean	com_fullyInitialized;

char	com_errorMessage[MAXPRINTMSG];

void Com_WriteConfig_f( void );

//============================================================================

static char	*rd_buffer;
static int	rd_buffersize;
static void	(*rd_flush)( char *buffer );

void Com_BeginRedirect (char *buffer, int buffersize, void (*flush)( char *) )
{
	if (!buffer || !buffersize || !flush)
		return;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect (void)
{
	if ( rd_flush ) {
		rd_flush(rd_buffer);
	}

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
#ifdef USE_AIO
class autolock
{
	pthread_mutex_t *lock_;

public:
	autolock(pthread_mutex_t *lock )
		: lock_(lock)
	{
		pthread_mutex_lock( lock );
	}
	~autolock()
	{
		pthread_mutex_unlock( lock_ );
	}
};

static pthread_mutex_t printfLock;
static pthread_mutex_t pushLock;
#endif
void QDECL Com_Printf(const char *fmt, ...) {
#ifdef USE_AIO
	autolock autolock( &printfLock );
#endif

	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean opening_qconsole = qfalse;

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	if ( rd_buffer ) {
		if ((strlen (msg) + strlen(rd_buffer)) > (rd_buffersize - 1)) {
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		Q_strcat(rd_buffer, rd_buffersize, msg);
    // TTimo nooo .. that would defeat the purpose
		//rd_flush(rd_buffer);			
		//*rd_buffer = 0;
		return;
	}

	// echo to console if we're not a dedicated server
	CL_ConsolePrint( msg );

	// echo to dedicated console and early console
	Sys_Print( msg );

	// logfile
	if ( com_logfile && com_logfile->integer ) {
    // TTimo: only open the qconsole.log if the filesystem is in an initialized state
    //   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
		if ( !logfile && FS_Initialized() && !opening_qconsole ) {
			struct tm *newtime;
			time_t aclock;

			opening_qconsole = qtrue;

			time( &aclock );
			newtime = localtime( &aclock );

			logfile = FS_FOpenFileWrite( "qconsole.log" );

			if ( logfile ) {
				Com_Printf( "logfile opened on %s\n", asctime( newtime ) );
				if ( com_logfile->integer > 1 ) {
					// force it to not buffer so we get valid
					// data even if we are crashing
					FS_ForceFlush(logfile);
				}
			}
			else {
				Com_Printf( "Opening qconsole.log failed!\n" );
				Cvar_SetValue( "logfile", 0 );
			}
		}
		opening_qconsole = qfalse;
		if ( logfile && FS_Initialized()) {
			FS_Write(msg, strlen(msg), logfile);
		}
	}


#if defined(_WIN32) && defined(_DEBUG)
	if ( *msg )
	{
		OutputDebugString ( Q_CleanStr(msg) );
		OutputDebugString ("\n");
	}
#endif
}


/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void QDECL Com_DPrintf( const char *fmt, ...) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];
		
	if ( !com_developer || !com_developer->integer ) {
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);
	
	Com_Printf ("%s", msg);
}

// Outputs to the VC / Windows Debug window (only in debug compile)
void QDECL Com_OPrintf( const char *fmt, ...) 
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
		
	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);
#ifdef _WIN32
	OutputDebugString(msg);
#elif !defined(__ANDROID__)
	printf(msg);
#endif
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the appropriate things.
=============
*/
void QDECL Com_Error( int code, const char *fmt, ... ) {
	va_list		argptr;
	static int	lastErrorTime;
	static int	errorCount;
	int			currentTime;

	if ( com_errorEntered ) {
		Sys_Error( "recursive error after: %s", com_errorMessage );
	}
	com_errorEntered = qtrue;

	// when we are running automated scripts, make sure we
	// know if anything failed
	if ( com_buildScript && com_buildScript->integer ) {
		code = ERR_FATAL;
	}

	// if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
	currentTime = Sys_Milliseconds();
	if ( currentTime - lastErrorTime < 100 ) {
		if ( ++errorCount > 3 ) {
			code = ERR_FATAL;
		}
	} else {
		errorCount = 0;
	}
	lastErrorTime = currentTime;

	va_start (argptr,fmt);
	Q_vsnprintf (com_errorMessage,sizeof(com_errorMessage), fmt,argptr);
	va_end (argptr);

	if ( code != ERR_DISCONNECT && code != ERR_NEED_CD ) {
		Cvar_Get("com_errorMessage", "", CVAR_ROM);	//give com_errorMessage a default so it won't come back to life after a resetDefaults
		Cvar_Set("com_errorMessage", com_errorMessage);
	}

	if ( code == ERR_DISCONNECT || code == ERR_SERVERDISCONNECT || code == ERR_DROP || code == ERR_NEED_CD ) {
		throw code;
	} else {
		CL_Shutdown ();
		SV_Shutdown (va("Server fatal crashed: %s\n", com_errorMessage));
	}

	Com_Shutdown ();

	Sys_Error ("%s", com_errorMessage);
}


/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit_f( void ) {
	// don't try to shutdown if we are in a recursive error
	if ( !com_errorEntered ) {
		SV_Shutdown ("Server quit\n");
		CL_Shutdown ();
		Com_Shutdown ();
		FS_Shutdown(qtrue);
	}
	Sys_Quit ();
}



/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define	MAX_CONSOLE_LINES	32
int		com_numConsoleLines;
char	*com_consoleLines[MAX_CONSOLE_LINES];

typedef enum {
	PROTOCOL_CMD,
	PROTOCOL_CVAR,
} protocolParamType_t;
typedef struct {
	const char			*name;
	protocolParamType_t	type;
} protocolParams_t;
//allowed parameters to get parsed out from ja://
protocolParams_t parseParams[] = {
	{"password",PROTOCOL_CVAR},
	{"fs_game",PROTOCOL_CVAR},
};

//ja://ip:port&password=pass&fs_game=mod -> +connect ip:port +set password pass +set fs_game mod
char *Com_ParseProtocol(char *commandLine) {
	static char newCommandLine[256];
	char *protocol;
	//ip has to contain dots, doesn't it?
	if ((protocol = strstr(commandLine, "ja:")) && (strchr(commandLine, '.') || strstr(commandLine, "localhost"))) {
		protocol += 3;
		//allow only alphanumeric with ".", ":" and "-"
		char *ip = strtok(protocol, " /\\?!\"#$%&\'()*+,;<=>@[]^_`{|}~");
		if (ip) {
			Q_strcat(newCommandLine, sizeof(newCommandLine), "+connect ");
			Q_strcat(newCommandLine, sizeof(newCommandLine), ip);
			size_t len = ARRAY_LEN(parseParams);
			char *param;
			for (param = strtok(NULL, " /\\?&%\'"); param != NULL;
				param = strtok(param + strlen(param) + 1, " /\\?&%\'")) {
				char temp[64], *key, *val;
				Q_strncpyz(temp, param, sizeof (temp));
				key = strtok(temp, " =\"\'");
				val = strtok(NULL, " =\"\'");
				if (key && *key && val && *val && *val != '&') {
					for (size_t i = 0; i < len; i++) {
						if (!Q_stricmpn(key, parseParams[i].name, strlen(parseParams[i].name))) {
							//entTODO: add ignoring params into protocolParams_t?
							if (!Q_stricmp(parseParams[i].name, "fs_game") && !Q_stricmp(val, "base"))
								break;
							if (parseParams[i].type == PROTOCOL_CMD) {
								Q_strcat(newCommandLine, sizeof(newCommandLine), " +");
							} else {
								Q_strcat(newCommandLine, sizeof(newCommandLine), " +set ");
							}
							Q_strcat(newCommandLine, sizeof(newCommandLine), parseParams[i].name);
							Q_strcat(newCommandLine, sizeof(newCommandLine), " ");
							Q_strcat(newCommandLine, sizeof(newCommandLine), val);
							break;
						}
					}
				}
			}
			return newCommandLine;
		}
	}
	return commandLine;
}

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine( char *cmdLine ) {
	char *commandLine = Com_ParseProtocol(cmdLine);
	com_consoleLines[0] = commandLine;
	com_numConsoleLines = 1;

	while ( *commandLine ) {
		// look for a + seperating character
		// if commandLine came from a file, we might have real line seperators
		if ( *commandLine == '+' || *commandLine == '\n' ) {
			if ( com_numConsoleLines == MAX_CONSOLE_LINES ) {
				return;
			}
			com_consoleLines[com_numConsoleLines] = commandLine + 1;
			com_numConsoleLines++;
			*commandLine = 0;
		}
		commandLine++;
	}
}


/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of jampconfig.cfg
===================
*/
qboolean Com_SafeMode( void ) {
	int		i;

	for ( i = 0 ; i < com_numConsoleLines ; i++ ) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( !Q_stricmp( Cmd_Argv(0), "safe" )
			|| !Q_stricmp( Cmd_Argv(0), "cvar_restart" ) ) {
			com_consoleLines[i][0] = 0;
			return qtrue;
		}
	}
	return qfalse;
}


/*
===============
Com_StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets shouls
be after execing the config and default.
===============
*/
void Com_StartupVariable( const char *match ) {
	int		i;
	char	*s;

	for (i=0 ; i < com_numConsoleLines ; i++) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( strcmp( Cmd_Argv(0), "set" ) ) {
			continue;
		}

		s = Cmd_Argv(1);

		if(!match || !strcmp(s, match))
		{
			if(Cvar_Flags(s) == CVAR_NONEXISTENT)
				Cvar_Get(s, Cmd_Argv(2), CVAR_USER_CREATED);
			else
				Cvar_Set2(s, Cmd_Argv(2), qfalse);
		}
	}
}


/*
=================
Com_AddStartupCommands

Adds command line parameters as script statements
Commands are seperated by + signs

Returns qtrue if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Com_AddStartupCommands( void ) {
	int		i;
	qboolean	added;

	added = qfalse;
	// quote every token, so args with semicolons can work
	for (i=0 ; i < com_numConsoleLines ; i++) {
		if ( !com_consoleLines[i] || !com_consoleLines[i][0] ) {
			continue;
		}

		// set commands won't override menu startup
		if ( Q_stricmpn( com_consoleLines[i], "set", 3 ) ) {
			added = qtrue;
		}
		Cbuf_AddText( com_consoleLines[i] );
		Cbuf_AddText( "\n" );
	}

	return added;
}


//============================================================================

void Info_Print( const char *s ) {
	char	key[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	char	*o;
	int		l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			Com_Memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Com_Printf ("%s ", key);

		if (!*s)
		{
			Com_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Com_Printf ("%s\n", value);
	}
}

/*
============
Com_StringContains
============
*/
char *Com_StringContains(char *str1, char *str2, int casesensitive) {
	int len, i, j;

	len = strlen(str1) - strlen(str2);
	for (i = 0; i <= len; i++, str1++) {
		for (j = 0; str2[j]; j++) {
			if (casesensitive) {
				if (str1[j] != str2[j]) {
					break;
				}
			}
			else {
				if (toupper(str1[j]) != toupper(str2[j])) {
					break;
				}
			}
		}
		if (!str2[j]) {
			return str1;
		}
	}
	return NULL;
}

/*
============
Com_Filter
============
*/
int Com_Filter(char *filter, char *name, int casesensitive)
{
	char buf[MAX_TOKEN_CHARS];
	char *ptr;
	int i, found;

	while(*filter) {
		if (*filter == '*') {
			filter++;
			for (i = 0; *filter; i++) {
				if (*filter == '*' || *filter == '?') break;
				buf[i] = *filter;
				filter++;
			}
			buf[i] = '\0';
			if (strlen(buf)) {
				ptr = Com_StringContains(name, buf, casesensitive);
				if (!ptr) return qfalse;
				name = ptr + strlen(buf);
			}
		}
		else if (*filter == '?') {
			filter++;
			name++;
		}
		else if (*filter == '[' && *(filter+1) == '[') {
			filter++;
		}
		else if (*filter == '[') {
			filter++;
			found = qfalse;
			while(*filter && !found) {
				if (*filter == ']' && *(filter+1) != ']') break;
				if (*(filter+1) == '-' && *(filter+2) && (*(filter+2) != ']' || *(filter+3) == ']')) {
					if (casesensitive) {
						if (*name >= *filter && *name <= *(filter+2)) found = qtrue;
					}
					else {
						if (toupper(*name) >= toupper(*filter) &&
							toupper(*name) <= toupper(*(filter+2))) found = qtrue;
					}
					filter += 3;
				}
				else {
					if (casesensitive) {
						if (*filter == *name) found = qtrue;
					}
					else {
						if (toupper(*filter) == toupper(*name)) found = qtrue;
					}
					filter++;
				}
			}
			if (!found) return qfalse;
			while(*filter) {
				if (*filter == ']' && *(filter+1) != ']') break;
				filter++;
			}
			filter++;
			name++;
		}
		else {
			if (casesensitive) {
				if (*filter != *name) return qfalse;
			}
			else {
				if (toupper(*filter) != toupper(*name)) return qfalse;
			}
			filter++;
			name++;
		}
	}
	return qtrue;
}

/*
============
Com_FilterPath
============
*/
int Com_FilterPath(char *filter, char *name, int casesensitive)
{
	int i;
	char new_filter[MAX_QPATH];
	char new_name[MAX_QPATH];

	for (i = 0; i < MAX_QPATH-1 && filter[i]; i++) {
		if ( filter[i] == '\\' || filter[i] == ':' ) {
			new_filter[i] = '/';
		}
		else {
			new_filter[i] = filter[i];
		}
	}
	new_filter[i] = '\0';
	for (i = 0; i < MAX_QPATH-1 && name[i]; i++) {
		if ( name[i] == '\\' || name[i] == ':' ) {
			new_name[i] = '/';
		}
		else {
			new_name[i] = name[i];
		}
	}
	new_name[i] = '\0';
	return Com_Filter(new_filter, new_name, casesensitive);
}

/*
============
Com_HashKey
============
*/
int Com_HashKey(char *string, int maxlen) {
	int register hash, i;

	hash = 0;
	for (i = 0; i < maxlen && string[i] != '\0'; i++) {
		hash += string[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return hash;
}

/*
================
Com_RealTime
================
*/
int Com_RealTime(qtime_t *qtime) {
	time_t t;
	struct tm *tms;

	t = time(NULL);
	if (!qtime)
		return t;
	tms = localtime(&t);
	if (tms) {
		qtime->tm_sec = tms->tm_sec;
		qtime->tm_min = tms->tm_min;
		qtime->tm_hour = tms->tm_hour;
		qtime->tm_mday = tms->tm_mday;
		qtime->tm_mon = tms->tm_mon;
		qtime->tm_year = tms->tm_year;
		qtime->tm_wday = tms->tm_wday;
		qtime->tm_yday = tms->tm_yday;
		qtime->tm_isdst = tms->tm_isdst;
	}
	return t;
}


/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

// bk001129 - here we go again: upped from 64
#define	MAX_PUSHED_EVENTS	            1024
#ifdef USE_AIO
#define THREADACCESS volatile
#else
#define THREADACCESS
#endif
// bk001129 - init, also static
static THREADACCESS int		com_pushedEventsHead = 0;
static THREADACCESS int             com_pushedEventsTail = 0;
// bk001129 - static
static sysEvent_t	com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling( void ) {
	Com_StartupVariable( "journal" );
	com_journal = Cvar_Get ("journal", "0", CVAR_INIT);
	if ( !com_journal->integer ) {
		return;
	}

	if ( com_journal->integer == 1 ) {
		Com_Printf( "Journaling events\n");
		com_journalFile = FS_FOpenFileWrite( "journal.dat" );
		com_journalDataFile = FS_FOpenFileWrite( "journaldata.dat" );
	} else if ( com_journal->integer == 2 ) {
		Com_Printf( "Replaying journaled events\n");
		FS_FOpenFileRead( "journal.dat", &com_journalFile, qtrue );
		FS_FOpenFileRead( "journaldata.dat", &com_journalDataFile, qtrue );
	}

	if ( !com_journalFile || !com_journalDataFile ) {
		Cvar_Set( "com_journal", "0" );
		com_journalFile = 0;
		com_journalDataFile = 0;
		Com_Printf( "Couldn't open journal files\n" );
	}
}

/*
=================
Com_GetRealEvent
=================
*/
sysEvent_t	Com_GetRealEvent( void ) {
	int			r;
	sysEvent_t	ev;

	// either get an event from the system or the journal file
	if ( com_journal->integer == 2 ) {
		r = FS_Read( &ev, sizeof(ev), com_journalFile );
		if ( r != sizeof(ev) ) {
			Com_Error( ERR_FATAL, "Error reading from journal file" );
		}
		if ( ev.evPtrLength ) {
			ev.evPtr = Z_Malloc( ev.evPtrLength, TAG_EVENT, qtrue );
			r = FS_Read( ev.evPtr, ev.evPtrLength, com_journalFile );
			if ( r != ev.evPtrLength ) {
				Com_Error( ERR_FATAL, "Error reading from journal file" );
			}
		}
	} else {
		ev = Sys_GetEvent();

		// write the journal value out if needed
		if ( com_journal->integer == 1 ) {
			r = FS_Write( &ev, sizeof(ev), com_journalFile );
			if ( r != sizeof(ev) ) {
				Com_Error( ERR_FATAL, "Error writing to journal file" );
			}
			if ( ev.evPtrLength ) {
				r = FS_Write( ev.evPtr, ev.evPtrLength, com_journalFile );
				if ( r != ev.evPtrLength ) {
					Com_Error( ERR_FATAL, "Error writing to journal file" );
				}
			}
		}
	}

	return ev;
}


/*
=================
Com_InitPushEvent
=================
*/
// bk001129 - added
void Com_InitPushEvent( void ) {
  // clear the static buffer array
  // this requires SE_NONE to be accepted as a valid but NOP event
  memset( com_pushedEvents, 0, sizeof(com_pushedEvents) );
  // reset counters while we are at it
  // beware: GetEvent might still return an SE_NONE from the buffer
  com_pushedEventsHead = 0;
  com_pushedEventsTail = 0;
}


/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent( sysEvent_t *event ) {
#ifdef USE_AIO
	autolock autolock( &pushLock );
#endif

	sysEvent_t		*ev;
	static int printedWarning = 0; // bk001129 - init, bk001204 - explicit int

	ev = &com_pushedEvents[ com_pushedEventsHead & (MAX_PUSHED_EVENTS-1) ];

	if ( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS ) {

		// don't print the warning constantly, or it can give time for more...
		if ( !printedWarning ) {
			printedWarning = qtrue;
			Com_Printf( "WARNING: Com_PushEvent overflow\n" );
		}

		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		com_pushedEventsTail++;
	} else {
		printedWarning = qfalse;
	}

	*ev = *event;
	com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t	Com_GetEvent( void ) {
	{
#ifdef USE_AIO
		autolock autolock( &pushLock );
#endif
		if ( com_pushedEventsHead > com_pushedEventsTail ) {
			com_pushedEventsTail++;
			return com_pushedEvents[ (com_pushedEventsTail-1) & (MAX_PUSHED_EVENTS-1) ];
		}
	}
	return Com_GetRealEvent();
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket( netadr_t *evFrom, msg_t *buf ) {
	int		t1, t2, msec;

	t1 = 0;

	if ( com_speeds->integer ) {
		t1 = Sys_Milliseconds ();
	}

	SV_PacketEvent( *evFrom, buf );

	if ( com_speeds->integer ) {
		t2 = Sys_Milliseconds ();
		msec = t2 - t1;
		if ( com_speeds->integer == 3 ) {
			Com_Printf( "SV_PacketEvent time: %i\n", msec );
		}
	}
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/
int Com_EventLoop( void ) {
	sysEvent_t	ev;
	netadr_t	evFrom;
	byte		bufData[MAX_MSGLEN];
	msg_t		buf;

	MSG_Init( &buf, bufData, sizeof( bufData ) );

	while ( 1 ) {
		ev = Com_GetEvent();

		// if no more events are available
		if ( ev.evType == SE_NONE ) {
			// manually send packet events for the loopback channel
			while ( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) ) {
				CL_PacketEvent( evFrom, &buf );
			}

			while ( NET_GetLoopPacket( NS_SERVER, &evFrom, &buf ) ) {
				// if the server just shut down, flush the events
				if ( com_sv_running->integer ) {
					Com_RunAndTimeServerPacket( &evFrom, &buf );
				}
			}

			return ev.evTime;
		}


		switch ( ev.evType ) {
		default:
		  // bk001129 - was ev.evTime
			Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evType );
			break;
        case SE_NONE:
            break;
		case SE_KEY:
			CL_KeyEvent( ev.evValue, (qboolean)ev.evValue2, ev.evTime );
			break;
		case SE_CHAR:
			CL_CharEvent( ev.evValue );
			break;
		case SE_MOUSE:
			CL_MouseEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_JOYSTICK_AXIS:
			CL_JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_CONSOLE:
			if ( ((char *)ev.evPtr)[0] == '\\' || ((char *)ev.evPtr)[0] == '/' ) 
			{
				Cbuf_AddText( (char *)ev.evPtr+1 );	
			}
			else
			{
				Cbuf_AddText( (char *)ev.evPtr );
			}
			Cbuf_AddText( "\n" );
			break;
		case SE_PACKET:
			// this cvar allows simulation of connections that
			// drop a lot of packets.  Note that loopback connections
			// don't go through here at all.
			if ( com_dropsim->value > 0 ) {
				static int seed;

				if ( Q_random( &seed ) < com_dropsim->value ) {
					break;		// drop this packet
				}
			}

			evFrom = *(netadr_t *)ev.evPtr;
			buf.cursize = ev.evPtrLength - sizeof( evFrom );

			// we must copy the contents of the message out, because
			// the event buffers are only large enough to hold the
			// exact payload, but channel messages need to be large
			// enough to hold fragment reassembly
			if ( (unsigned)buf.cursize > buf.maxsize ) {
				Com_Printf("Com_EventLoop: oversize packet\n");
				continue;
			}
			Com_Memcpy( buf.data, (byte *)((netadr_t *)ev.evPtr + 1), buf.cursize );
			if ( com_sv_running->integer ) {
				Com_RunAndTimeServerPacket( &evFrom, &buf );
			} else {
				CL_PacketEvent( evFrom, &buf );
			}
			break;
#ifdef USE_AIO
		case SE_AIO_FCLOSE:
			{
				extern void	FS_FCloseAio( int handle );
				FS_FCloseAio( ev.evValue );
				break;
			}
#endif
		}

		// free any block data
		if ( ev.evPtr ) {
			Z_Free( ev.evPtr );
		}
	}

	return 0;	// never reached
}

/*
================
Com_Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int Com_Milliseconds (void) {
	sysEvent_t	ev;

	// get events and push them until we get a null event with the current time
	do {

		ev = Com_GetRealEvent();
		if ( ev.evType != SE_NONE ) {
			Com_PushEvent( &ev );
		}
	} while ( ev.evType != SE_NONE );
	
	return ev.evTime;
}

//============================================================================

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Com_Error_f (void) {
	if ( Cmd_Argc() > 1 ) {
		Com_Error( ERR_DROP, "Testing drop error" );
	} else {
		Com_Error( ERR_FATAL, "Testing fatal error" );
	}
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f (void) {
	float	s;
	int		start, now;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "freeze <seconds>\n" );
		return;
	}
	s = atof( Cmd_Argv(1) );

	start = Com_Milliseconds();

	while ( 1 ) {
		now = Com_Milliseconds();
		if ( ( now - start ) * 0.001 > s ) {
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void Com_Crash_f( void ) {
	* ( int * ) 0 = 0x12345678;
}


#ifdef MEM_DEBUG
	void SH_Register(void);
#endif

/*
==================
Com_ExecuteCfg
==================
*/

void Com_ExecuteCfg(void)
{
	Cbuf_ExecuteText(EXEC_NOW, "exec mpdefault.cfg\n");
	Cbuf_Execute(); // Always execute after exec to prevent text buffer overflowing

	if(!Com_SafeMode())
	{
		// skip the q3config.cfg and autoexec.cfg if "safe" is on the command line
		Cbuf_ExecuteText(EXEC_NOW, "exec " Q3CONFIG_CFG "\n");
		Cbuf_Execute();
		Cbuf_ExecuteText(EXEC_NOW, "exec autoexec.cfg\n");
		Cbuf_Execute();
	}
}

/*
=================
Com_ErrorString
Error string for the given error code (from Com_Error).
=================
*/
static const char *Com_ErrorString ( int code )
{
	switch ( code )
	{
		case ERR_DISCONNECT:
		// fallthrough
		case ERR_SERVERDISCONNECT:
			return "DISCONNECTED";

		case ERR_DROP:
			return "DROPPED";

		case ERR_NEED_CD:
			return "NEED CD";

		default:
			return "UNKNOWN";
	}
}

/*
=================
Com_CatchError
Handles freeing up of resources when Com_Error is called.
=================
*/
static void Com_CatchError ( int code )
{
	if ( code == ERR_DISCONNECT || code == ERR_SERVERDISCONNECT ) {
		SV_Shutdown( "Server disconnected" );
		CL_Disconnect( qtrue );
		CL_FlushMemory(  );
		// make sure we can get at our local stuff
		FS_PureServerSetLoadedPaks( "", "" );
		com_errorEntered = qfalse;
	} else if ( code == ERR_DROP ) {
		Com_Printf ("********************\n"
					"ERROR: %s\n"
					"********************\n", com_errorMessage);
		SV_Shutdown (va("Server crashed: %s\n",  com_errorMessage));
		CL_Disconnect( qtrue );
		CL_FlushMemory( );
		// make sure we can get at our local stuff
		FS_PureServerSetLoadedPaks( "", "" );
		com_errorEntered = qfalse;
	} else if ( code == ERR_NEED_CD ) {
		SV_Shutdown( "Server didn't have CD" );
		if ( com_cl_running && com_cl_running->integer ) {
			CL_Disconnect( qtrue );
			CL_FlushMemory( );
		} else {
			Com_Printf("Server didn't have CD\n" );
		}
		// make sure we can get at our local stuff
		FS_PureServerSetLoadedPaks( "", "" );
		com_errorEntered = qfalse;
	}
}

#ifdef _WIN32
static const char *GetErrorString( DWORD error ) {
	static char buf[MAX_STRING_CHARS];
	buf[0] = '\0';

	if ( error ) {
		LPVOID lpMsgBuf;
		DWORD bufLen = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPTSTR)&lpMsgBuf, 0, NULL );
		if ( bufLen ) {
			LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
			Q_strncpyz( buf, lpMsgStr, min( (size_t)(lpMsgStr + bufLen), sizeof(buf) ) );
			LocalFree( lpMsgBuf );
		}
	}
	return buf;
}
#endif

/*
==================
Com_UpdateProcessCoresAffinity

==================
*/
//smod feature
static void Com_SetProcessCoresAffinity() {
#ifdef _WIN32
	DWORD	processMask;
	DWORD	systemMask;
	int		dev = Cvar_VariableValue("developer");

	// get current affinity for
	if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask)) {
		// verbose something in developer mode...
		if (dev) {
			Com_Printf("Getting affinity mask failed, error: %i\n", GetLastError());
        }
		return;
	}

	if ( sscanf( com_affinity->string, "%X", &processMask ) != 1 )
		processMask = 1; // set to first core only

	// call win API to set desired affinity mask
	if (!SetProcessAffinityMask(GetCurrentProcess(), processMask) && dev) {
		// verbose something in developer mode...
		Com_Printf("Setting affinity mask failed, error: %i\n", GetLastError());
	}
#endif
}
char loadingMsg[64] = "Loading...";
void Com_SetLoadingMsg(char *msg) {
	if (!msg)
		return;
	memset(loadingMsg, 0, sizeof(loadingMsg));
	Com_sprintf(loadingMsg, sizeof(loadingMsg), msg);
}
/*
=================
Com_Init
=================
*/
void Com_Init(char *commandLine) {
	char *s;
	Com_Printf("%s %s %s\n", JK_VERSION, PLATFORM_STRING, __DATE__);
	Com_SetLoadingMsg("Starting the game...");
	try {
#ifdef USE_AIO
		{
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&printfLock, &attr);
			pthread_mutex_init(&pushLock, &attr);
		}
#endif
		// bk001129 - do this before anything else decides to push events
		Com_InitPushEvent();
		Cvar_Init();
		navigator.Init();
		// prepare enough of the subsystems to handle
		// cvar and command buffer management
		Com_ParseCommandLine(commandLine);
	//	Swap_Init();
		Cbuf_Init();
		Com_InitZoneMemory();
		Cmd_Init();
		// override anything from the config files with command line args
		Com_StartupVariable(NULL);
		// Seed the random number generator
		Rand_Init(Sys_Milliseconds(true));
		// get the developer cvar set as early as possible
		com_developer = Cvar_Get("developer", "0", CVAR_TEMP);
		// done early so bind command exists
		CL_InitKeyCommands();
		com_homepath = Cvar_Get("com_homepath", "", CVAR_INIT);
		FS_InitFilesystem();
		Com_InitJournaling();
		Com_ExecuteCfg();
		// override anything from the config files with command line args
		Com_StartupVariable(NULL);
	  // get dedicated here for proper hunk megs initialization
	#ifdef DEDICATED
		com_dedicated = Cvar_Get ("dedicated", "2", CVAR_ROM);
		Cvar_CheckRange(com_dedicated, 1, 2, qtrue);
	#else
		//OJKFIXME: Temporarily disabled dedicated server when not using the dedicated server binary.
		//			Issue is the server not having a renderer when not using ^^^^^
		//				and crashing in SV_SpawnServer calling re.RegisterMedia_LevelLoadBegin
		//			Until we fully remove the renderer from the server, the client executable
		//				will not have dedicated support capabilities.
		//			Use the dedicated server package.
		com_dedicated = Cvar_Get ("_dedicated", "0", CVAR_ROM|CVAR_INIT|CVAR_PROTECTED);
	//	Cvar_CheckRange(com_dedicated, 0, 2, qtrue);
	#endif
		// allocate the stack based hunk allocator
		Com_InitHunkMemory();
		// if any archived cvars are modified after this, we will trigger a writing
		// of the config file
		cvar_modifiedFlags &= ~CVAR_ARCHIVE;
		//
		// init commands and vars
		//
		com_maxfps = Cvar_Get("com_maxfps", "125", CVAR_ARCHIVE);
		com_vmdebug = Cvar_Get("vmdebug", "0", CVAR_TEMP);
		com_logfile = Cvar_Get("logfile", "0", CVAR_TEMP);
		com_timescale = Cvar_Get("timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO);
		com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
		com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);
		com_terrainPhysics = Cvar_Get("com_terrainPhysics", "1", CVAR_CHEAT);
		com_dropsim = Cvar_Get("com_dropsim", "0", CVAR_CHEAT);
		com_viewlog = Cvar_Get("viewlog", "0", 0);
		com_speeds = Cvar_Get("com_speeds", "0", 0);
		com_timedemo = Cvar_Get("timedemo", "0", 0);
		com_cameraMode = Cvar_Get("com_cameraMode", "0", CVAR_CHEAT);
		com_unfocused = Cvar_Get("com_unfocused", "0", CVAR_ROM);
		com_minimized = Cvar_Get("com_minimized", "0", CVAR_ROM);
		com_optvehtrace = Cvar_Get("com_optvehtrace", "0", 0);
		cl_paused = Cvar_Get("cl_paused", "0", CVAR_ROM);
		sv_paused = Cvar_Get("sv_paused", "0", CVAR_ROM);
		com_sv_running = Cvar_Get("sv_running", "0", CVAR_ROM);
		com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);
		com_buildScript = Cvar_Get( "com_buildScript", "0", 0);
#ifdef G2_PERFORMANCE_ANALYSIS
		com_G2Report = Cvar_Get("com_G2Report", "0", 0);
#endif
		com_RMG = Cvar_Get("RMG", "0", 0);
		Cvar_Get("RMG_seed", "0", 0);
		Cvar_Get("RMG_time", "day", 0);
		Cvar_Get("RMG_soundset", "", 0);
		Cvar_Get("RMG_textseed", "0", CVAR_SYSTEMINFO|CVAR_ARCHIVE);
		Cvar_Get("RMG_map", "small", CVAR_ARCHIVE|CVAR_SYSTEMINFO);
		Cvar_Get("RMG_timefile", "day", CVAR_ARCHIVE);
		Cvar_Get("RMG_terrain", "grassyhills", CVAR_ARCHIVE);
		Cvar_Get("RMG_sky", "", CVAR_SYSTEMINFO);
		Cvar_Get("RMG_fog", "", CVAR_SYSTEMINFO);
		Cvar_Get("RMG_weather", "", CVAR_SYSTEMINFO|CVAR_SERVERINFO|CVAR_CHEAT);
		Cvar_Get("RMG_instances", "colombia", CVAR_SYSTEMINFO);
		Cvar_Get("RMG_miscents", "deciduous", 0);
		Cvar_Get("RMG_music", "music/dm_kam1", 0);
		Cvar_Get("RMG_mission", "ctf", CVAR_SYSTEMINFO);
		Cvar_Get("RMG_course", "standard", CVAR_SYSTEMINFO);
		Cvar_Get("RMG_distancecull", "5000", CVAR_CHEAT );
		com_introPlayed = Cvar_Get("com_introplayed", "0", CVAR_ARCHIVE);
	#if defined(_WIN32) && defined(_DEBUG)
		com_noErrorInterrupt = Cvar_Get("com_noErrorInterrupt", "0", 0);
	#endif
		com_affinity = Cvar_Get("com_affinity", "1", CVAR_ARCHIVE);
		if (com_dedicated->integer) {
			if (!com_viewlog->integer) {
				Cvar_Set("viewlog", "1");
			}
		}
		if (com_developer && com_developer->integer) {
			Cmd_AddCommand("error", Com_Error_f);
			Cmd_AddCommand("crash", Com_Crash_f);
			Cmd_AddCommand("freeze", Com_Freeze_f);
		}
		Cmd_AddCommand("quit", Com_Quit_f);
		Cmd_AddCommand("changeVectors", MSG_ReportChangeVectors_f);
		Cmd_AddCommand("writeconfig", Com_WriteConfig_f);
		s = va("%s %s %s", JK_VERSION, PLATFORM_STRING, __DATE__);
		com_version = Cvar_Get("version", s, CVAR_ROM | CVAR_SERVERINFO);
		Com_SetProcessCoresAffinity();
		SE_Init();
		Sys_Init();
		Netchan_Init(Com_Milliseconds() & 0xffff);	// pick a port value that should be nice and random
		VM_Init();
		SV_Init();
		com_dedicated->modified = qfalse;
		if (!com_dedicated->integer) {
			CL_Init();
			Sys_ShowConsole(com_viewlog->integer, qfalse);
		}
		// set com_frameTime so that if a map is started on the
		// command line it will still be able to count on com_frameTime
		// being random enough for a serverid
		com_frameTime = Com_Milliseconds();
		// add + commands from command line
		if (!Com_AddStartupCommands()) {
			// if the user didn't give any commands, run default action
			if (!com_dedicated->integer) {
#ifndef _DEBUG
//				Cbuf_AddText ("cinematic openinglogos.roq\n");
#endif
				// intro.roq is iD's.
//				if( !com_introPlayed->integer ) {
//					Cvar_Set( com_introPlayed->name, "1" );
//					Cvar_Set( "nextmap", "cinematic intro.RoQ" );
//				}
			}
		}
		// start in full screen ui mode
		Cvar_Set("r_uiFullScreen", "1");
		CL_StartHunkUsers();
		// make sure single player is off by default
		Cvar_Set("ui_singlePlayerActive", "0");
#ifdef MEM_DEBUG
		SH_Register();
#endif
		com_fullyInitialized = qtrue;
		Com_Printf("--- Common Initialization Complete ---\n");
	} catch (int code) {
		Com_CatchError (code);
		Sys_Error("Error during initialization: %s", Com_ErrorString(code));
	}
}
//==================================================================
void Com_WriteConfigToFile(const char *filename) {
	fileHandle_t f;
	f = FS_FOpenFileWrite(filename);
	if (!f) {
		Com_Printf ("Couldn't write %s.\n", filename);
		return;
	}
	FS_Printf(f, "// generated by Star Wars Jedi Academy MP, do not modify\n");
	Key_WriteBindings(f);
	Cvar_WriteVariables(f);
	FS_FCloseFile(f);
}
/*
===============
Com_WriteConfiguration
Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration( void ) {
	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if ( !com_fullyInitialized ) {
		return;
	}

	if ( !(cvar_modifiedFlags & CVAR_ARCHIVE ) ) {
		return;
	}
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	Com_WriteConfigToFile( Q3CONFIG_CFG );
}


/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f( void ) {
	char	filename[MAX_QPATH];

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "Usage: writeconfig <filename>\n" );
		return;
	}

	Q_strncpyz( filename, Cmd_Argv(1), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );

	if(!COM_CompareExtension(filename, ".cfg"))
	{
		Com_Printf( "Com_WriteConfig_f: Only the \".cfg\" extension is supported by this command!\n" );
		return;
	}

	Com_Printf( "Writing %s.\n", filename );
	Com_WriteConfigToFile( filename );
}

/*
================
Com_ModifyMsec
================
*/
int Com_ModifyMsec( int msec ) {
	int		clampTime;

	//
	// modify time for debugging values
	//
	if ( com_fixedtime->integer ) {
		msec = com_fixedtime->integer;
	} else if ( com_timescale->value ) {
		msec *= com_timescale->value;
	} else if (com_cameraMode->integer) {
		msec *= com_timescale->value;
	}
	
	// don't let it scale below 1 msec
	if ( msec < 1 && com_timescale->value) {
		msec = 1;
	}

	if ( com_dedicated->integer ) {
		// dedicated servers don't want to clamp for a much longer
		// period, because it would mess up all the client's views
		// of time.
		if ( com_sv_running->integer && msec > 500 ) {
			Com_Printf( "Hitch warning: %i msec frame time\n", msec );
		}
		clampTime = 5000;
	} else 
	if ( !com_sv_running->integer ) {
		// clients of remote servers do not want to clamp time, because
		// it would skew their view of the server's time temporarily
		clampTime = 5000;
	} else {
		// for local single player gaming
		// we may want to clamp the time to prevent players from
		// flying off edges when something hitches.
		clampTime = 200;
	}

	if ( msec > clampTime ) {
		msec = clampTime;
	}

	return msec;
}

#ifdef G2_PERFORMANCE_ANALYSIS
#include "../qcommon/timing.h"
void G2Time_ResetTimers(void);
void G2Time_ReportTimers(void);
extern timing_c G2PerformanceTimer_PreciseFrame;
extern int G2Time_PreciseFrame;
#endif

/*
=================
Com_Frame
=================
*/
void Com_Frame( void ) {

try
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_PreciseFrame.Start();
#endif
	int		msec, minMsec;
	static int	lastTime;
 
	int		timeBeforeFirstEvents;
	int           timeBeforeServer;
	int           timeBeforeEvents;
	int           timeBeforeClient;
	int           timeAfter;


	// bk001204 - init to zero.
	//  also:  might be clobbered by `longjmp' or `vfork'
	timeBeforeFirstEvents =0;
	timeBeforeServer =0;
	timeBeforeEvents =0;
	timeBeforeClient = 0;
	timeAfter = 0;

	// write config file if anything changed
	Com_WriteConfiguration(); 

	// if "viewlog" has been modified, show or hide the log console
	if ( com_viewlog->modified ) {
		if ( !com_dedicated->value ) {
			Sys_ShowConsole( com_viewlog->integer, qfalse );
		}
		com_viewlog->modified = qfalse;
	}

	//
	// main event loop
	//
	if ( com_speeds->integer ) {
		timeBeforeFirstEvents = Sys_Milliseconds ();
	}

	// we may want to spin here if things are going too fast
	if ( !com_dedicated->integer && com_maxfps->integer > 0 && !com_timedemo->integer ) {
		minMsec = 1000 / com_maxfps->integer;
	} else {
		minMsec = 1;
	}
	do {
		com_frameTime = Com_EventLoop();
		if ( lastTime > com_frameTime ) {
			lastTime = com_frameTime;		// possible on first frame
		}
		msec = com_frameTime - lastTime;
	} while ( msec < minMsec );
	// make sure mouse and joystick are only called once a frame
	IN_Frame();

	Cbuf_Execute ();

	lastTime = com_frameTime;

	// mess with msec if needed
	com_frameMsec = msec;
	msec = Com_ModifyMsec( msec );

	//
	// server side
	//
	if ( com_speeds->integer ) {
		timeBeforeServer = Sys_Milliseconds ();
	}

	SV_Frame( msec );

	// if "dedicated" has been modified, start up
	// or shut down the client system.
	// Do this after the server may have started,
	// but before the client tries to auto-connect
	if ( com_dedicated->modified ) {
		// get the latched value
		Cvar_Get( "_dedicated", "0", 0 );
		com_dedicated->modified = qfalse;
		if ( !com_dedicated->integer ) {
			CL_Init();
			Sys_ShowConsole( com_viewlog->integer, qfalse );
			CL_StartHunkUsers();	//fire up the UI!
		} else {
			CL_Shutdown();
			Sys_ShowConsole( 1, qtrue );
		}
	}

	//
	// client system
	//
	if ( !com_dedicated->integer ) {
		//
		// run event loop a second time to get server to client packets
		// without a frame of latency
		//
		if ( com_speeds->integer ) {
			timeBeforeEvents = Sys_Milliseconds ();
		}
		Com_EventLoop();
		Cbuf_Execute ();


		//
		// client side
		//
		if ( com_speeds->integer ) {
			timeBeforeClient = Sys_Milliseconds ();
		}

		CL_Frame( msec );

		if ( com_speeds->integer ) {
			timeAfter = Sys_Milliseconds ();
		}
	}

	//
	// report timing information
	//
	if ( com_speeds->integer ) {
		int			all, sv, ev, cl;

		all = timeAfter - timeBeforeServer;
		sv = timeBeforeEvents - timeBeforeServer;
		ev = timeBeforeServer - timeBeforeFirstEvents + timeBeforeClient - timeBeforeEvents;
		cl = timeAfter - timeBeforeClient;
		sv -= time_game;
		cl -= time_frontend + time_backend;

		Com_Printf ("frame:%i all:%3i sv:%3i ev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n", 
					 com_frameNumber, all, sv, ev, cl, time_game, time_frontend, time_backend );
	}	

	//
	// trace optimization tracking
	//
	if ( com_showtrace->integer ) {
	
		extern	int c_traces, c_brush_traces, c_patch_traces;
		extern	int	c_pointcontents;

		Com_Printf ("%4i traces  (%ib %ip) %4i points\n", c_traces,
			c_brush_traces, c_patch_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces = 0;
		c_patch_traces = 0;
		c_pointcontents = 0;
	}

	com_frameNumber++;

}//try
	catch (int code) {
		Com_CatchError (code);
		Com_Printf ("%s\n", Com_ErrorString (code));
		return;
	}

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_PreciseFrame += G2PerformanceTimer_PreciseFrame.End();

	if (com_G2Report && com_G2Report->integer)
	{
		G2Time_ReportTimers();
	}

	G2Time_ResetTimers();
#endif
}

/*
=================
Com_Shutdown
=================
*/
void MSG_shutdownHuffman();
void Com_Shutdown (void) 
{
	CM_ClearMap();

	if (logfile) {
		FS_FCloseFile (logfile);
		logfile = 0;
		com_logfile->integer = 0;//don't open up the log file again!!
	}

	if ( com_journalFile ) {
		FS_FCloseFile( com_journalFile );
		com_journalFile = 0;
	}

	MSG_shutdownHuffman();
/*
	// Only used for testing changes to huffman frequency table when tuning.
	{
		extern float Huff_GetCR(void);
		char mess[256];
		sprintf(mess,"Eff. CR = %f\n",Huff_GetCR());
		OutputDebugString(mess);
	}
*/
}


//rwwRMG: Inserted:
/*
============
ParseTextFile
============
*/

bool Com_ParseTextFile(const char *file, class CGenericParser2 &parser, bool cleanFirst)
{
	fileHandle_t	f;
	int				length = 0;
	char			*buf = 0, *bufParse = 0;

	length = FS_FOpenFileByMode( file, &f, FS_READ );
	if (!f || !length)		
	{
		return false;
	}

	buf = new char [length + 1];
	FS_Read( buf, length, f );
	buf[length] = 0;

	bufParse = buf;
	parser.Parse(&bufParse, cleanFirst);
	delete[] buf;

	FS_FCloseFile( f );

	return true;
}

void Com_ParseTextFileDestroy(class CGenericParser2 &parser)
{
	parser.Clean();
}

CGenericParser2 *Com_ParseTextFile(const char *file, bool cleanFirst, bool writeable)
{
	fileHandle_t	f;
	int				length = 0;
	char			*buf = 0, *bufParse = 0;
	CGenericParser2 *parse;

	length = FS_FOpenFileByMode( file, &f, FS_READ );
	if (!f || !length)		
	{
		return 0;
	}

	buf = new char [length + 1];
	FS_Read( buf, length, f );
	FS_FCloseFile( f );
	buf[length] = 0;

	bufParse = buf;

	parse = new CGenericParser2;
	if (!parse->Parse(&bufParse, cleanFirst, writeable))
	{
		delete parse;
		parse = 0;
	}

	delete[] buf;

	return parse;
}

uint8_t ConvertUTF32ToExpectedCharset(uint32_t utf32) {
	switch (utf32) {
		// Cyrillic characters - mapped to Windows-1251 encoding
		case 0x0410: return 192;
		case 0x0411: return 193;
		case 0x0412: return 194;
		case 0x0413: return 195;
		case 0x0414: return 196;
		case 0x0415: return 197;
		case 0x0416: return 198;
		case 0x0417: return 199;
		case 0x0418: return 200;
		case 0x0419: return 201;
		case 0x041A: return 202;
		case 0x041B: return 203;
		case 0x041C: return 204;
		case 0x041D: return 205;
		case 0x041E: return 206;
		case 0x041F: return 207;
		case 0x0420: return 208;
		case 0x0421: return 209;
		case 0x0422: return 210;
		case 0x0423: return 211;
		case 0x0424: return 212;
		case 0x0425: return 213;
		case 0x0426: return 214;
		case 0x0427: return 215;
		case 0x0428: return 216;
		case 0x0429: return 217;
		case 0x042A: return 218;
		case 0x042B: return 219;
		case 0x042C: return 220;
		case 0x042D: return 221;
		case 0x042E: return 222;
		case 0x042F: return 223;
		case 0x0430: return 224;
		case 0x0431: return 225;
		case 0x0432: return 226;
		case 0x0433: return 227;
		case 0x0434: return 228;
		case 0x0435: return 229;
		case 0x0436: return 230;
		case 0x0437: return 231;
		case 0x0438: return 232;
		case 0x0439: return 233;
		case 0x043A: return 234;
		case 0x043B: return 235;
		case 0x043C: return 236;
		case 0x043D: return 237;
		case 0x043E: return 238;
		case 0x043F: return 239;
		case 0x0440: return 240;
		case 0x0441: return 241;
		case 0x0442: return 242;
		case 0x0443: return 243;
		case 0x0444: return 244;
		case 0x0445: return 245;
		case 0x0446: return 246;
		case 0x0447: return 247;
		case 0x0448: return 248;
		case 0x0449: return 249;
		case 0x044A: return 250;
		case 0x044B: return 251;
		case 0x044C: return 252;
		case 0x044D: return 253;
		case 0x044E: return 254;
		case 0x044F: return 255;

		// Eastern european characters - polish, czech, etc use Windows-1250 encoding
		case 0x0160: return 138;
		case 0x015A: return 140;
		case 0x0164: return 141;
		case 0x017D: return 142;
		case 0x0179: return 143;
		case 0x0161: return 154;
		case 0x015B: return 156;
		case 0x0165: return 157;
		case 0x017E: return 158;
		case 0x017A: return 159;
		case 0x0141: return 163;
		case 0x0104: return 165;
		case 0x015E: return 170;
		case 0x017B: return 175;
		case 0x0142: return 179;
		case 0x0105: return 185;
		case 0x015F: return 186;
		case 0x013D: return 188;
		case 0x013E: return 190;
		case 0x017C: return 191;
		case 0x0154: return 192;
		case 0x00C1: return 193;
		case 0x00C2: return 194;
		case 0x0102: return 195;
		case 0x00C4: return 196;
		case 0x0139: return 197;
		case 0x0106: return 198;
		case 0x00C7: return 199;
		case 0x010C: return 200;
		case 0x00C9: return 201;
		case 0x0118: return 202;
		case 0x00CB: return 203;
		case 0x011A: return 204;
		case 0x00CD: return 205;
		case 0x00CE: return 206;
		case 0x010E: return 207;
		case 0x0110: return 208;
		case 0x0143: return 209;
		case 0x0147: return 210;
		case 0x00D3: return 211;
		case 0x00D4: return 212;
		case 0x0150: return 213;
		case 0x00D6: return 214;
		case 0x0158: return 216;
		case 0x016E: return 217;
		case 0x00DA: return 218;
		case 0x0170: return 219;
		case 0x00DC: return 220;
		case 0x00DD: return 221;
		case 0x0162: return 222;
		case 0x00DF: return 223;
		case 0x0155: return 224;
		case 0x00E1: return 225;
		case 0x00E2: return 226;
		case 0x0103: return 227;
		case 0x00E4: return 228;
		case 0x013A: return 229;
		case 0x0107: return 230;
		case 0x00E7: return 231;
		case 0x010D: return 232;
		case 0x00E9: return 233;
		case 0x0119: return 234;
		case 0x00EB: return 235;
		case 0x011B: return 236;
		case 0x00ED: return 237;
		case 0x00EE: return 238;
		case 0x010F: return 239;
		case 0x0111: return 240;
		case 0x0144: return 241;
		case 0x0148: return 242;
		case 0x00F3: return 243;
		case 0x00F4: return 244;
		case 0x0151: return 245;
		case 0x00F6: return 246;
		case 0x0159: return 248;
		case 0x016F: return 249;
		case 0x00FA: return 250;
		case 0x0171: return 251;
		case 0x00FC: return 252;
		case 0x00FD: return 253;
		case 0x0163: return 254;
		case 0x02D9: return 255;

		default: return (uint8_t)utf32;
	}
}
/*
===============
Converts a UTF-8 character to UTF-32.
===============
*/
uint32_t ConvertUTF8ToUTF32(char *utf8CurrentChar, char **utf8NextChar) {
	uint32_t utf32 = 0;
	char *c = utf8CurrentChar;
	if((*c & 0x80) == 0) {
		utf32 = *c++;
	} else if((*c & 0xE0) == 0xC0) { // 110x xxxx
		utf32 |= (*c++ & 0x1F) << 6;
		utf32 |= (*c++ & 0x3F);
	} else if((*c & 0xF0) == 0xE0) { // 1110 xxxx
		utf32 |= (*c++ & 0x0F) << 12;
		utf32 |= (*c++ & 0x3F) << 6;
		utf32 |= (*c++ & 0x3F);
	} else if((*c & 0xF8) == 0xF0) { // 1111 0xxx
		utf32 |= (*c++ & 0x07) << 18;
		utf32 |= (*c++ & 0x3F) << 6;
		utf32 |= (*c++ & 0x3F) << 6;
		utf32 |= (*c++ & 0x3F);
	} else {
		Com_DPrintf("Unrecognised UTF-8 lead byte: 0x%x\n", (unsigned int)*c);
		c++;
	}
	*utf8NextChar = c;
	return utf32;
}
// Copyright (C) 2015 ent (entdark)
//
// cg_demos_auto.c - autorecording demos routine
//
// credits: 
// - teh aka dumbledore aka teh_1337: autorecording demos
// - CaNaBiS: formatting with %

#include "cg_local.h"

#define MAX_TIMESTAMPS 256

typedef struct demoAuto_s {
	char defaultName[MAX_QPATH];
	char demoName[MAX_QPATH];
	char customName[MAX_QPATH];
	int timeStamps[MAX_TIMESTAMPS];
} demoAuto_t;

static demoAuto_t demoAuto;
static char mod[MAX_QPATH];

void _mkdir(char *);
/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
static qboolean FS_CreatePath(char *OSPath) {
	char	*ofs;
	
	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if (strstr(OSPath, "..") || strstr(OSPath, "::")) {
		//Com_Printf("WARNING: refusing to create relative path \"%s\"\n", OSPath);
		return qtrue;
	}

	for (ofs = OSPath+1 ; *ofs ; ofs++) {
		if (*ofs == PATH_SEP) {	
			// create the directory
			*ofs = 0;
#ifdef _WIN32
			_mkdir (OSPath);
#else
			mkdir(OSPath, 0750);
#endif
			*ofs = PATH_SEP;
		}
	}
	return qfalse;
}

static qboolean FS_CopyFile(const char *fromOSPath, const char *toOSPath, char *overwrite, const int owSize) {
	FILE	*f;
	size_t	len, total;
	byte	*buf;
	int		fileCount = 1;
	char	*lExt, nExt[MAX_QPATH];
	char	stripped[MAX_QPATH];

	//Com_Printf( "copy %s to %s\n", fromOSPath, toOSPath );

	/*if (strstr(fromOSPath, "journal.dat") || strstr(fromOSPath, "journaldata.dat")) {
		Com_Printf( "Ignoring journal files\n");
		return;
	}*/

	f = fopen(fromOSPath, "rb");
	if (!f) {
		return qfalse;
	}
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);

	// we are using direct malloc instead of Z_Malloc here, so it
	// probably won't work on a mac... Its only for developers anyway...
	buf = malloc(len);
	total = fread(buf, 1, len, f);
	if (!total) {
		return qfalse;
	}
	if (total != len)
		;//Com_Error( ERR_FATAL, "Short read in FS_Copyfiles()\n" );
	fclose(f);

	if(FS_CreatePath(toOSPath)) {
		return qfalse;
	}

	// if the file exists then create a new one with (N)
	if (fopen(toOSPath, "rb") && overwrite && owSize > 0) {
		lExt = strchr(toOSPath, '.');
		if (!lExt) {
			lExt = "";
		}
		while (strchr(lExt+1, '.')) {
			lExt = strchr(lExt+1, '.');
		}
		Q_strncpyz(nExt, lExt, sizeof(nExt));
		COM_StripExtension(toOSPath, stripped, sizeof(stripped));
		fileCount++;
		while (fopen(va("%s (%i)%s", stripped, fileCount, nExt), "rb")) {
			fileCount++;
		}
	}
	if (fileCount > 1 && overwrite && owSize > 0) {
		Q_strncpyz(overwrite, va("%s (%i)%s", stripped, fileCount, nExt), owSize);
		f = fopen(overwrite, "wb");
	} else {
		if (overwrite && owSize > 0) {
			Q_strncpyz(overwrite, "", owSize);
		}
		f = fopen(toOSPath, "wb");
	}
	if (!f) {
		return qfalse;
	}
	total = fwrite(buf, 1, len, f);
	if (!total) {
		return qfalse;
	}
	if (total != len)
		;//Com_Error( ERR_FATAL, "Short write in FS_Copyfiles()\n" );
	fclose(f);
	free(buf);
	return qtrue;
}

static void FS_Remove(const char *osPath) {
	remove(osPath);
}

extern void CG_SanitizeString(char *in, char *out);
char *demoAutoFormat(const char* name) {	
	const	char *format;
	qboolean haveTag = qfalse;
	char	outBuf[512];
	int		outIndex = 0;
	int		outLeft = sizeof(outBuf) - 1;
	
	int t = 0;
	char timeStamps[MAX_QPATH] = "";
	qtime_t ct;

	char playerName[MAX_QPATH], *mapName = COM_SkipPath(Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "mapname"));
	CG_SanitizeString(cgs.clientinfo[cg.snap->ps.clientNum].name, playerName);
	trap_RealTime(&ct);
	
	format = cg_autoDemoFormat.string;
	if (!format || !format[0]) {
		if (!name || !name[0]) {
			format = "%t";
		} else {
			format = "%n_%t";
		}
	}

	while (*format && outLeft  > 0) {
		if (haveTag) {
			char ch = *format++;
			haveTag = qfalse;
			switch (ch) {
			case 'd':		//date
				Com_sprintf( outBuf + outIndex, outLeft, "%d-%02d-%02d-%02d%02d%02d",
								1900+ct.tm_year, ct.tm_mon+1,ct.tm_mday,
								ct.tm_hour, ct.tm_min, ct.tm_sec);
				outIndex += strlen( outBuf + outIndex );
				break;
			case 'm':		//map
				Com_sprintf( outBuf + outIndex, outLeft, mapName);
				outIndex += strlen( outBuf + outIndex );
				break;
			case 'n':		//custom demo name
				Com_sprintf( outBuf + outIndex, outLeft, name);
				outIndex += strlen( outBuf + outIndex );
				break;
			case 'p':		//current player name
				Com_sprintf( outBuf + outIndex, outLeft, playerName);
				outIndex += strlen( outBuf + outIndex );
				break;
			case 't':		//timestamp
				while (demoAuto.timeStamps[t] && t < MAX_TIMESTAMPS) {
					int min = demoAuto.timeStamps[t] / 60000;
					int sec = (demoAuto.timeStamps[t] / 1000) % 60;
					if (t == 0) {
						Com_sprintf(timeStamps, sizeof(timeStamps), "%0.2d%0.2d", min, sec);
					} else {
						Com_sprintf(timeStamps, sizeof(timeStamps), "%s_%0.2d%0.2d", timeStamps, min, sec);
					}
					t++;
				}
				Com_sprintf( outBuf + outIndex, outLeft, timeStamps);
				outIndex += strlen( outBuf + outIndex );
				t++;
				break;
			case '%':
				outBuf[outIndex++] = '%';
				break;
			default:
				continue;
			}
			outLeft = sizeof(outBuf) - outIndex - 1;
			continue;
		}
		if (*format == '%') {
			haveTag = qtrue;
			format++;
			continue;
		}
		outBuf[outIndex++] = *format++;
		outLeft = sizeof(outBuf) - outIndex - 1;
	}
	outBuf[ outIndex ] = 0;
	return va("%s", outBuf);
}

// Standard naming for screenshots/demos
static char *demoAutoGenerateDefaultFilename(void) {
	qtime_t ct;
	const char *pszServerInfo = CG_ConfigString(CS_SERVERINFO);
	
	trap_RealTime(&ct);
	return va("%d-%02d-%02d-%02d%02d%02d-%s",
								1900+ct.tm_year, ct.tm_mon+1,ct.tm_mday,
								ct.tm_hour, ct.tm_min, ct.tm_sec,
								COM_SkipPath(Info_ValueForKey(pszServerInfo, "mapname")));
}

void demoAutoSave_f(void) {
	int t = 0;
	if (strstr(cg_autoDemoFormat.string, "%t"))
		while (demoAuto.timeStamps[t] && t < MAX_TIMESTAMPS)
			t++;
	demoAuto.timeStamps[t] = cg.time - cgs.levelStartTime;

	if (!(trap_Argc() < 2)) {
		Q_strncpyz(demoAuto.customName, CG_Argv(1), sizeof(demoAuto.customName));
	}
	Com_sprintf(demoAuto.demoName, sizeof(demoAuto.demoName), demoAutoFormat(demoAuto.customName));
	CG_Printf(S_COLOR_WHITE"Demo will be saved into "S_COLOR_GREEN"%s.dm_26\n", demoAuto.demoName);
}

void demoAutoSaveLast_f(void) {
	if (trap_Argc() < 2 && FS_CopyFile(va("%s/demos/LastDemo/LastDemo.dm_26", mod), va("%s/demos/%s.dm_26", mod, demoAutoGenerateDefaultFilename()), NULL, NULL)) {
		CG_Printf(S_COLOR_GREEN"LastDemo successfully saved\n");
	} else if (FS_CopyFile(va("%s/demos/LastDemo/LastDemo.dm_26", mod), va("%s/demos/%s.dm_26", mod, CG_Argv(1)), NULL, NULL)) {
		CG_Printf(S_COLOR_GREEN"LastDemo successfully saved into %s.dm_26\n", CG_Argv(1));
	} else {
		CG_Printf(S_COLOR_RED"LastDemo has failed to save\n");
	}
}

void demoAutoComplete(void) {
	char newName[MAX_QPATH];
	//if we are not manually saving, then temporarily store a demo in LastDemo folder
	if (!*demoAuto.demoName && FS_CopyFile(va("%s/demos/%s.dm_26", mod, demoAuto.defaultName), va("%s/demos/LastDemo/LastDemo.dm_26", mod), NULL, NULL)) {
		CG_Printf(S_COLOR_GREEN"Demo temporarily saved into LastDemo/LastDemo.dm_26\n");
	} else if (FS_CopyFile(va("%s/demos/%s.dm_26", mod, demoAuto.defaultName), va("%s/demos/%s.dm_26", mod, demoAuto.demoName), newName, sizeof(newName))) {
		CG_Printf(S_COLOR_GREEN"Demo successfully saved into %s.dm_26\n", (Q_stricmp(newName, "")) ? newName : demoAuto.demoName);
	} else {
		CG_Printf(S_COLOR_RED"Demo has failed to save\n");
	}
}

// Dynamically names a demo and sets up the recording
void demoAutoRecord(void) {
	char val[MAX_STRING_CHARS];
	memset(&demoAuto, 0, sizeof(demoAuto_t));
	Q_strncpyz(demoAuto.defaultName, "LastDemo/LastDemo_recording", sizeof(demoAuto.defaultName));
	trap_Cvar_VariableStringBuffer("g_synchronousclients", val, sizeof(val));
	trap_SendConsoleCommand(va("set g_synchronousclients 1;record %s;set g_synchronousclients %s\n", demoAuto.defaultName, val));
}

void demoAutoInit(void) {
	memset(&demoAuto, 0, sizeof(demoAuto_t));
	trap_Cvar_VariableStringBuffer("fs_game", mod, sizeof(mod));
	if (!*mod) {
		Q_strncpyz(mod, "base", sizeof(mod));
	}
}

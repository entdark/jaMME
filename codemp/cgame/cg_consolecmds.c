// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "../ui/ui_shared.h"
#include "../game/bg_saga.h"
#include "cg_demos.h"
extern menuDef_t *menuScoreboard;



void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if (!targetNum ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendConsoleCommand( va( "gc %i %i", targetNum, atoi( test ) ) );
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	CG_Printf ("%s (%i %i %i) : %i\n", cgs.mapname, (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], 
		(int)cg.refdef.viewangles[YAW]);
}


static void CG_ScoresDown_f( void ) {

	CG_BuildSpectatorString();
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

static void CG_ScoresUp_f( void ) {
	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

extern menuDef_t *menuScoreboard;
void Menu_Reset();			// FIXME: add to right include file

static void CG_scrollScoresDown_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}


static void CG_scrollScoresUp_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}


static void CG_spWin_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.winnerSound);
	//trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_CenterPrint(CG_GetStringEdString("MP_INGAME", "YOU_WIN"), SCREEN_HEIGHT * .30, 0);
}

static void CG_spLose_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.loserSound);
	//trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_CenterPrint(CG_GetStringEdString("MP_INGAME", "YOU_LOSE"), SCREEN_HEIGHT * .30, 0);
}

void CG_ClientList_f( void )
{
	clientInfo_t *ci;
	int i;
	int count = 0;

	for( i = 0; i < MAX_CLIENTS; i++ ) 
	{
		ci = &cgs.clientinfo[ i ];
		if( !ci->infoValid ) 
			continue;

		switch( ci->team ) 
		{
		case TEAM_FREE:
			Com_Printf( "%2d " S_COLOR_YELLOW "F   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i, ci->name, (ci->botSkill != -1) ? " (bot)" : "" );
			break;

		case TEAM_RED:
			Com_Printf( "%2d " S_COLOR_RED "R   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
				ci->name, (ci->botSkill != -1) ? " (bot)" : "" );
			break;

		case TEAM_BLUE:
			Com_Printf( "%2d " S_COLOR_BLUE "B   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i,
				ci->name, (ci->botSkill != -1) ? " (bot)" : "" );
			break;

		default:
		case TEAM_SPECTATOR:
			Com_Printf( "%2d " S_COLOR_YELLOW "S   " S_COLOR_WHITE "%s" S_COLOR_WHITE "%s\n", i, ci->name, (ci->botSkill != -1) ? " (bot)" : "" );
			break;
		}

		count++;
	}

	Com_Printf( "Listed %2d clients\n", count );
}


static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}


/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set ("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

void CG_SiegeBriefingDisplay(int team, int dontshow);
static void CG_SiegeBriefing_f(void)
{
	int team;

	if (cgs.gametype != GT_SIEGE)
	{ //Cannot be displayed unless in this gametype
		return;
	}

	team = cg.predictedPlayerState.persistant[PERS_TEAM];

	if (team != SIEGETEAM_TEAM1 &&
		team != SIEGETEAM_TEAM2)
	{ //cannot be displayed if not on a valid team
		return;
	}

	CG_SiegeBriefingDisplay(team, 0);
}

static void CG_SiegeCvarUpdate_f(void)
{
	int team;

	if (cgs.gametype != GT_SIEGE)
	{ //Cannot be displayed unless in this gametype
		return;
	}

	team = cg.predictedPlayerState.persistant[PERS_TEAM];

	if (team != SIEGETEAM_TEAM1 &&
		team != SIEGETEAM_TEAM2)
	{ //cannot be displayed if not on a valid team
		return;
	}

	CG_SiegeBriefingDisplay(team, 1);
}
static void CG_SiegeCompleteCvarUpdate_f(void)
{

	if (cgs.gametype != GT_SIEGE)
	{ //Cannot be displayed unless in this gametype
		return;
	}

	// Set up cvars for both teams
	CG_SiegeBriefingDisplay(SIEGETEAM_TEAM1, 1);
	CG_SiegeBriefingDisplay(SIEGETEAM_TEAM2, 1);
}
/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/
//keep all stats since score ints!!!!!
typedef struct {
	team_t		team;
	char		name[64];
	int			score;
	int			captures;
	int			assist;
	int			defend;
	int			accuracy;
	int			time;
	int			flagCarrierKills;
	int			flagReturns;
	int			flagHold;
	int			teamHeals;
	int			teamEnergizes;
} stats;
static void CG_PrintfStats(const char *printStats, const int stats, const int maxStats) {
	if (stats == maxStats && maxStats != 0)
		CG_Printf("%s%s", S_COLOR_GREEN, va(printStats, stats));
	else
		CG_Printf("%s%s", S_COLOR_WHITE, va(printStats, stats));
}
static void CG_DisplayTeamStat(team_t t) {
	int i, nameLenMax = 0;
	stats max;
	memset(&max, 0, sizeof(max));
	for (i = 0; i < cg.numScores; i++) {
		int *enhancedStats = &(cg.enhanced.stats[i].score);
		int *maxStats = &(max.score);
		int j;
		if (!(cg.enhanced.stats[i].team == TEAM_RED
			|| cg.enhanced.stats[i].team == TEAM_BLUE))
			continue;
		int nameLen = Q_PrintStrlen(cg.enhanced.stats[i].name, qfalse);
		if (nameLen > nameLenMax)
			nameLenMax = nameLen;
		if (cg.enhanced.stats[i].team != t)
			continue;
		for (j = 0; j < 11; j++, enhancedStats++, maxStats++) {
			if (*enhancedStats > *maxStats)
				*maxStats = *enhancedStats;
		}
	}
	if (nameLenMax < 4)
		nameLenMax = 4;
	CG_Printf(S_COLOR_CYAN"TEAM NAME");
	for (i = 0; i < nameLenMax-4; i++)
		CG_Printf(" ");
	CG_Printf(S_COLOR_CYAN" SCORE CAPS ASSIST DEF  ACC TIME ");
	CG_Printf(S_COLOR_RED "FCKILLS FLAGRETS FLAGHOLD  TH/TE\n");
	CG_Printf(S_COLOR_CYAN"---- ");
	for (i = 0; i < nameLenMax; i++)
		CG_Printf(S_COLOR_CYAN"-");
	CG_Printf(S_COLOR_CYAN" ----- ---- ------ --- ---- ---- ");
	CG_Printf(S_COLOR_CYAN"------- -------- -------- ------\n");
	for (i = 0; i < cg.numScores; i++) {
		int nameLen;
		if (cg.enhanced.stats[i].team != t)
			continue;
		if (t == TEAM_RED)
			CG_Printf(S_COLOR_RED"RED  ");
		else if (t == TEAM_BLUE)
			CG_Printf(S_COLOR_BLUE"BLUE ");
		nameLen = Q_PrintStrlen(cg.enhanced.stats[i].name, qfalse);
		CG_Printf(S_COLOR_WHITE"%s", cg.enhanced.stats[i].name);
		if (nameLen < nameLenMax) {
			int j, d = nameLenMax - nameLen;
			for (j = 0; j < d; j++)
				CG_Printf(" ");
		}
		CG_Printf(" ");
		CG_Printf(S_COLOR_WHITE);
		CG_PrintfStats("%5d ", cg.enhanced.stats[i].score, max.score);
		CG_PrintfStats("%4d ", cg.enhanced.stats[i].captures, max.captures);
		CG_PrintfStats("%6d ", cg.enhanced.stats[i].assist, max.assist);
		CG_PrintfStats("%3d ", cg.enhanced.stats[i].defend, max.defend);
		CG_PrintfStats("%3d%% ", cg.enhanced.stats[i].accuracy, max.accuracy);
		CG_Printf(S_COLOR_WHITE"%4d ", cg.enhanced.stats[i].time);
		CG_PrintfStats("%7d ", cg.enhanced.stats[i].flagCarrierKills, max.flagCarrierKills);
		CG_PrintfStats("%8d ", cg.enhanced.stats[i].flagReturns, max.flagReturns);
		{char flagHold[17];
		int secs = (cg.enhanced.stats[i].flagHold / 1000);
		int mins = (secs / 60);
		if (cg.enhanced.stats[i].flagHold >= 60000) {
			secs %= 60;
//			Com_sprintf(flagHold, sizeof(flagHold), "%d:%02d", mins, secs);
			Com_sprintf(flagHold, sizeof(flagHold), "%dm %02ds", mins, secs);
		} else {
//			Com_sprintf(flagHold, sizeof(flagHold), "%d", secs);
			Com_sprintf(flagHold, sizeof(flagHold), "%ds", secs);
		}
		if (cg.enhanced.stats[i].flagHold == max.flagHold && max.flagHold != 0)
			CG_Printf(S_COLOR_GREEN"%8s ", flagHold);
		else
			CG_Printf(S_COLOR_WHITE"%8s ", flagHold);}
		CG_PrintfStats("%3d", cg.enhanced.stats[i].teamHeals, max.teamHeals);
		CG_Printf(S_COLOR_WHITE"/");
		CG_PrintfStats("%d", cg.enhanced.stats[i].teamEnergizes, max.teamEnergizes);
		CG_Printf("\n");
	}
}
void CG_EnhancedStatistics_f(void) {
	if (!cg.enhanced.detected || cgs.gametype != GT_CTF)
		return;
	if (!cg.enhanced.statsGenerated) {
		CG_Printf(S_COLOR_RED"Failed to load statistics. Try to refresh scoreboard\n");
		return;
	}
	CG_Printf("\n");
	CG_Printf(S_COLOR_CYAN"TEAM SCORE\n");
	CG_Printf(S_COLOR_RED"RED "S_COLOR_WHITE"%d "S_COLOR_BLUE"BLUE "S_COLOR_WHITE"%d\n", cg.teamScores[0], cg.teamScores[1]);
	CG_Printf("\n");
	CG_DisplayTeamStat(TEAM_RED);
	CG_Printf("\n");
	CG_DisplayTeamStat(TEAM_BLUE);
	CG_Printf("\n");
	CG_Printf(S_COLOR_CYAN"Stats generated.\n");
}
#ifdef __ANDROID__
/*
==================
ConcatArgs
==================
*/
char *ConcatArgs(int start) {
	int i, c, tlen;
	static char line[MAX_STRING_CHARS];
	int len;
	char arg[MAX_STRING_CHARS];
	len = 0;
	c = trap_Argc();
	for (i = start; i < c; i++) {
		trap_Argv(i, arg, sizeof(arg));
		tlen = strlen(arg);
		if (len + tlen >= MAX_STRING_CHARS - 1) {
			break;
		}
		memcpy(line + len, arg, tlen);
		len += tlen;
		if (i != c - 1) {
			line[len] = ' ';
			len++;
		}
	}
	line[len] = 0;
	return line;
}
static void CG_TargetPrev_f(void) {
	trap_SendConsoleCommand("followPrev");
}
static void CG_TargetNext_f(void) {
	trap_SendConsoleCommand("followNext");
}
static void CG_Chase_f(void) {
	const char *cmd = CG_Argv(1);
	if (trap_Argc() < 2)
		return;
	if (!Q_stricmp("targetPrev",cmd))
		CG_TargetPrev_f();
	else if (!Q_stricmp("targetNext",cmd))
		CG_TargetNext_f();
}
static void CG_Pause_f(void) {
}
static void CG_SayAlias_f(void) {
	char *p = NULL;
	if (trap_Argc () < 2)
		return;
	p = ConcatArgs(1);
	trap_SendConsoleCommand(va("cmd say %s", p));
}
static void CG_SayTeamAlias_f(void) {
	char *p = NULL;
	if (trap_Argc () < 2)
		return;
	p = ConcatArgs(1);
	trap_SendConsoleCommand(va("cmd say_team %s", p));
}
#endif
typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;
static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "weaponclean", CG_WeaponClean_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "tcmd", CG_TargetCommand_f },
	//JAC - Disable spWin and spLose as they're just used to troll people.
	//{ "spWin", CG_spWin_f },
	//{ "spLose", CG_spLose_f },
	{ "scoresDown", CG_scrollScoresDown_f },
	{ "scoresUp", CG_scrollScoresUp_f },
	{ "startOrbit", CG_StartOrbit_f },
	//{ "camera", CG_Camera_f },
	{ "loaddeferred", CG_LoadDeferredPlayers },
	{ "invnext", CG_NextInventory_f },
	{ "invprev", CG_PrevInventory_f },
	{ "forcenext", CG_NextForcePower_f },
	{ "forceprev", CG_PrevForcePower_f },
	{ "briefing", CG_SiegeBriefing_f },
	{ "siegeCvarUpdate", CG_SiegeCvarUpdate_f },
	{ "siegeCompleteCvarUpdate", CG_SiegeCompleteCvarUpdate_f },
	{ "clientlist", CG_ClientList_f },
	{ "sm_stats", CG_EnhancedStatistics_f },
	{ "clientOverride", CG_ClientOverride_f },
#ifdef __ANDROID__
	{ "targetPrev", CG_TargetPrev_f },
	{ "targetNext", CG_TargetNext_f },
	{ "chase", CG_Chase_f },
	{ "pause", CG_Pause_f },
	{ "s", CG_SayAlias_f },
	{ "st", CG_SayTeamAlias_f },
#endif
};

static size_t numCommands = ARRAY_LEN( commands );

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	int		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < numCommands ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}

static const char *gcmds[] = {
	"addbot",
	"callteamvote",
	"callvote",
	"duelteam",
	"follow",
	"follownext",
	"followprev",
	"forcechanged",
	"give",
	"god",
	"kill",
	"levelshot",
	"loaddefered",
	"noclip",
	"notarget",
	"NPC",
	"say",
	"say_team",
	"setviewpos",
	"siegeclass",
	"stats",
	//"stopfollow",
	"team",
	"teamtask",
	"teamvote",
	"tell",
	"voice_cmd",
	"vote",
	"where",
	"zoom",
	//base_enhanced
	"ignore",
	"whois"
};
static size_t numgcmds = ARRAY_LEN( gcmds );

/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < numCommands ; i++ )
		trap_AddCommand( commands[i].cmd );

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	for( i = 0; i < numgcmds; i++ )
		trap_AddCommand( gcmds[i] );
}

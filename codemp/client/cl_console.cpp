//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

// console.c

#include "client.h"
#include "../qcommon/stringed_ingame.h"
#include "../qcommon/game_version.h"

//#define CON_FILTER

int g_console_field_width = 78;

console_t con;

cvar_t *con_conspeed;
cvar_t *con_notifytime;
cvar_t *con_timestamps;
cvar_t *con_filter;

#define	DEFAULT_CONSOLE_WIDTH 78

vec4_t console_color = {1.0, 1.0, 1.0, 1.0};


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void) {
	// closing a full screen console restarts the demo loop
	if (cls.state == CA_DISCONNECTED && Key_GetCatcher() == KEYCATCH_CONSOLE) {
		CL_StartDemoLoop();
		return;
	}
	Field_Clear(&kg.g_consoleField);
	kg.g_consoleField.widthInChars = g_console_field_width;
	Con_ClearNotify ();
#ifdef __ANDROID__
	int state = con.state;
	if (state == conFull || state == conHidden)
#endif
		Key_SetCatcher(Key_GetCatcher() ^ KEYCATCH_CONSOLE);
#ifdef __ANDROID__
	state++;
	if (state == conMax)
		state = conHidden;
	con.state = (conState_t)state;
#endif
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f(void) {	//yell
	float ratio = (cls.ratioFix ? cls.ratioFix : 1.0f);
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30 / ratio;

	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f(void) {	//team chat
	float ratio = (cls.ratioFix ? cls.ratioFix : 1.0f);
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 25 / ratio;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f(void) {		//target chat
	float ratio = (cls.ratioFix ? cls.ratioFix : 1.0f);
	if (!cgvm) {
		assert(!"null cgvm");
		return;
	}
	chat_playerNum = VM_Call( cgvm, CG_CROSSHAIR_PLAYER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30 / ratio;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f(void) {	//attacker
	float ratio = (cls.ratioFix ? cls.ratioFix : 1.0f);
	if (!cgvm) {
		assert(!"null cgvm");
		return;
	}
	chat_playerNum = VM_Call( cgvm, CG_LAST_ATTACKER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30 / ratio;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void) {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		con.text[i] = ' ';
	}

	Con_Bottom();		// go to end
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int				l, x, i;
	int				*line;
	fileHandle_t	f;
	int		bufferlen;
	char	*buffer;
	char	filename[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("%s\n", SE_GetString("CON_TEXT_DUMP_USAGE"));
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".txt" );

	Com_Printf ("Dumped console text to %s.\n", filename );

	f = FS_FOpenFileWrite( filename );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open %s.\n", filename);
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

#ifdef _WIN32
	bufferlen = con.linewidth + 3 * sizeof ( char );
#else
	bufferlen = con.linewidth + 2 * sizeof ( char );
#endif

	buffer = (char *)Hunk_AllocateTempMemory( bufferlen );

	// write the remaining lines
	buffer[bufferlen-1] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for(i=0; i<con.linewidth; i++)
			buffer[i] = (char) (line[i] & 0xff);
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
#ifdef _WIN32
		Q_strcat(buffer, bufferlen, "\r\n");
#else
		Q_strcat(buffer, bufferlen, "\n");
#endif
		FS_Write(buffer, strlen(buffer), f);
	}
	Hunk_FreeTempMemory( buffer );
	FS_FCloseFile( f );
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int		i;
	
	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}

						

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	MAC_STATIC int	tbuf[CON_TEXTSIZE];

//	width = (SCREEN_WIDTH / SMALLCHAR_WIDTH) - 2;
	width = (cls.glconfig.vidWidth / SMALLCHAR_WIDTH) - 2;

	if (width == con.linewidth)
		return;


	if (width < 1)			// video hasn't been initialized yet
	{
		con.xadjust = 1;
		con.yadjust = 1;
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for(i=0; i<CON_TEXTSIZE; i++)
			con.text[i] = ' ';
	}
	else
	{
		// on wide screens, we will center the text
		con.xadjust = 640.0f / cls.glconfig.vidWidth;
		con.yadjust = 480.0f / cls.glconfig.vidHeight;

		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		Com_Memcpy(tbuf, con.text, sizeof(tbuf));
		for(i=0; i<CON_TEXTSIZE; i++)
			con.text[i] = ' ';


		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init(void) {
	int i;
	con_notifytime = Cvar_Get("con_notifytime", "3", 0);
	con_timestamps = Cvar_Get("con_timestamps", "0", CVAR_ARCHIVE);
	con_conspeed = Cvar_Get("scr_conspeed", "3", 0);
	con_filter = Cvar_Get("con_filter", "0", CVAR_TEMP);
	Field_Clear(&kg.g_consoleField);
	kg.g_consoleField.widthInChars = g_console_field_width;
	for (i = 0; i < COMMAND_HISTORY; i++) {
		Field_Clear(&kg.historyEditLines[i]);
		kg.historyEditLines[i].widthInChars = g_console_field_width;
	}
	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand("messagemode3", Con_MessageMode3_f);
	Cmd_AddCommand("messagemode4", Con_MessageMode4_f);
	Cmd_AddCommand("clear", Con_Clear_f);
	Cmd_AddCommand("condump", Con_Dump_f);
}


/*
===============
Con_Linefeed
===============
*/
static void Con_Linefeed(qboolean skipnotify) {
	int		i;

	// mark time for transparent overlay
	if (con.current >= 0) {
		if (skipnotify)
			  con.times[con.current % NUM_CON_TIMES] = 0;
		else
			  con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}

	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	for(i=0; i<con.linewidth; i++)
		con.text[(con.current%con.totallines)*con.linewidth+i] = ' ';
}

void Con_SetFilter(const byte filter) {
	con.filter = filter;
}
void Con_ResetFilter() {
	Con_SetFilter(CON_FILTER_CLIENT);
}
byte Con_GetFilter(void) {
	return con.filter;
}

byte Con_GetUserFilter(void) {
	byte filter = CON_FILTER_NONE;
	if (!con_filter)
		return filter;
	if (con_filter->integer > 0)
		filter = con_filter->integer;
	return filter;
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
static qboolean newString = qtrue;
const char *CL_ConsolePrintTimeStamp(const char *txt) {
	int c = (unsigned char)*txt;
	if (c == 0)
		return txt;
	if (!con_timestamps)
		return txt;
	if (!con_timestamps->integer)
		return txt;
	if (newString) {
		time_t rawtime;
		char timeStr[32] = {0};
		newString = qfalse;
		time(&rawtime);
		strftime(timeStr,sizeof(timeStr),"[%H:%M:%S]",localtime(&rawtime));
		return va(S_COLOR_WHITE"%s %s", timeStr, txt);
	}
	return txt;
}
void CL_ConsolePrint(const char *txt) {
	int y;
	int c, l;
	int color;
	qboolean skipnotify = qfalse;	// NERVE - SMF
	int	prev;						// NERVE - SMF
	byte filter = Con_GetFilter();
	qboolean setFilter = qfalse;
	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp(txt, "[skipnotify]", 12)) {
		skipnotify = qtrue;
		txt += 12;
	}
	if (txt[0] == '*') {
		if (Con_GetFilter() == CON_FILTER_SERVER)
			filter = CON_FILTER_CHAT;
		skipnotify = qtrue;
		txt += 1;
	}
	// for some demos we don't want to ever show anything on the console
	if (cl_noprint && cl_noprint->integer) {
		Con_ResetFilter();
		return;
	}
	if (!con.initialized) {
		con.color[0] = 
		con.color[1] = 
		con.color[2] =
		con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize();
		con.initialized = qtrue;
	}
//	color = ColorIndex(COLOR_WHITE);
	color = 0xffffff00;
	txt = CL_ConsolePrintTimeStamp(txt);
	while ((c = (unsigned char)*txt) != 0) {
		vec4_t newColor;
		int colorLen = Q_parseColorString( txt, newColor, cls.uag.newColors );
		if ( colorLen ) {
			color = ((int)(newColor[0] * 0xff)) << 8 |
					((int)(newColor[1] * 0xff)) << 16 |
					((int)(newColor[2] * 0xff)) << 24;
			txt += colorLen;
			continue;
		}
		// count word length
		for (l=0; l<con.linewidth; l++) {
			if (txt[l] <= ' ') {
				break;
			}
		}
		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth)) {
			Con_Linefeed(skipnotify);
		}
		txt++;
		switch (c) {
		case '\n':
			Con_Linefeed(skipnotify);
			newString = qtrue;
			break;
		case '\r':
			con.x = 0;
			newString = qtrue;
			break;
		default:	// display character and advance
			y = con.current % con.totallines;
#ifdef CON_FILTER
			if (!setFilter) {
				con.text[y*con.linewidth+con.x] = (short)(((short)filter << 8) | 0);
				con.x++;
				if (con.x >= con.linewidth) {
					Con_Linefeed(skipnotify);
				}
				setFilter = qtrue;
			}
#endif
			con.text[y*con.linewidth+con.x] = color | c;
			con.x++;
			if (con.x >= con.linewidth) {
				Con_Linefeed(skipnotify);
			}
			break;
		}
		txt = CL_ConsolePrintTimeStamp(txt);
	}
	// mark time for transparent overlay
	if (con.current >= 0) {
		// NERVE - SMF
		if (skipnotify) {
			prev = con.current % NUM_CON_TIMES - 1;
			if (prev < 0)
				prev = NUM_CON_TIMES - 1;
			con.times[prev] = 0;
		} else {
		// -NERVE - SMF
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}
	Con_ResetFilter();
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput (void) {
	int		y;

	if ( cls.state != CA_DISCONNECTED && !(Key_GetCatcher( ) & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = con.vislines - ( SMALLCHAR_HEIGHT * (re.Language_IsAsian() ? 1.5 : 2) );

	re.SetColor( con.color );

	SCR_DrawSmallChar( (int)(con.xadjust + 1 * SMALLCHAR_WIDTH), y, ']' );

	Field_Draw( &kg.g_consoleField, (int)(con.xadjust + 2 * SMALLCHAR_WIDTH), y,
				SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue, qfalse );
}

float chatColour[4] = {1.0f,1.0f,1.0f,1.0f};
/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void) {
	int		x, v;
	int		*text;
	int		i;
	int		time;
	int		skip;
	int		currentColor;
	const char* chattext;
	//filter out the notifications?
	const byte filter = CON_FILTER_NONE;//Con_GetUserFilter();
	qboolean filterLine = qfalse;

//	currentColor = 7;
//	re.SetColor( g_color_table[currentColor] );
	currentColor = 0xffffff00;
	re.SetColor( colorWhite );

	v = 0;
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++) {
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;

		if (cl.snap.ps.pm_type != PM_INTERMISSION && Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
			continue;
		}


		if (!cl_conXOffset)
		{
			cl_conXOffset = Cvar_Get ("cl_conXOffset", "0", 0);
		}

		// asian language needs to use the new font system to print glyphs...
		//
		// (ignore colours since we're going to print the whole thing as one string)
		//
		if (re.Language_IsAsian())
		{
			static int iFontIndex = re.RegisterFont("ocr_a");	// this seems naughty
			const float fFontScale = 0.75f*con.yadjust;
			const int iPixelHeightToAdvance =   2+(1.3/con.yadjust) * re.Font_HeightPixels(iFontIndex, fFontScale);	// for asian spacing, since we don't want glyphs to touch.

			// concat the text to be printed...
			//
			char sTemp[4096]={0};	// ott
			for (x = 0 ; x < con.linewidth ; x++) 
			{
				if ( ( (text[x]>>8)&7 ) != currentColor ) {
					currentColor = (text[x]>>8)&7;
					strcat(sTemp,va("^%i", (text[x]>>8)&7) );
				}
				strcat(sTemp,va("%c",text[x] & 0xFF));				
			}
			//
			// and print...
			//
			re.Font_DrawString(cl_conXOffset->integer + con.xadjust*(con.xadjust + (1*SMALLCHAR_WIDTH/*aesthetics*/)), con.yadjust*(v), sTemp, g_color_table[currentColor], iFontIndex, -1, fFontScale);

			v +=  iPixelHeightToAdvance;
		}
		else
		{		
			for (x = 0 ; x < con.linewidth ; x++) {
#ifdef CON_FILTER
				if ( ( text[x] & 0xff ) == 0 && ( !((text[x]>>8) & filter )) ) {
					filterLine = qtrue;
					break;
				}
#endif
				if ( ( text[x] & 0xff ) == ' ' ) {
					continue;
				}
				if ( ( text[x] & 0xffffff00 ) != currentColor ) {
					vec4_t setColor;
					currentColor = text[x] & 0xffffff00;
					setColor[0] = ((currentColor >>  8) & 0xff) * (1 / 255.0f);
					setColor[1] = ((currentColor >> 16) & 0xff) * (1 / 255.0f);
					setColor[2] = ((currentColor >> 24) & 0xff) * (1 / 255.0f);
					setColor[3] = 1.0f;
					re.SetColor( setColor );
				}
				if (!cl_conXOffset) {
					cl_conXOffset = Cvar_Get ("cl_conXOffset", "0", 0);
				}
				SCR_DrawSmallChar((int)(cl_conXOffset->integer + con.xadjust + (x+1)*SMALLCHAR_WIDTH), v, text[x] & 0xff);
			}
#ifdef CON_FILTER
			if (filterLine) {
				v += SMALLCHAR_HEIGHT;
				i--;
			}
#else
			v += SMALLCHAR_HEIGHT;
#endif
			filterLine = qfalse;
		}
	}

	re.SetColor( NULL );

	if (Key_GetCatcher() & (KEYCATCH_UI | KEYCATCH_CGAME)) {
		return;
	}

	// draw the chat line
	if (Key_GetCatcher() & KEYCATCH_MESSAGE) {
		if (chat_team) {
			chattext = SE_GetString("MP_SVGAME", "SAY_TEAM");
		} else {
			chattext = SE_GetString("MP_SVGAME", "SAY");
		}
		SCR_DrawStringExt2(8*cls.ratioFix, v, BIGCHAR_WIDTH*cls.ratioFix, BIGCHAR_HEIGHT, chattext, chatColour, qfalse, qfalse);
		skip = strlen(chattext)+1;
		Field_BigDraw(&chatField, skip * BIGCHAR_WIDTH, v, SCREEN_WIDTH - (skip + 1) * BIGCHAR_WIDTH, qtrue, qtrue);
		v += BIGCHAR_HEIGHT;
	}

}

//I want it be rainbow :>
static vec4_t conColourTable[16] = {
	{1, 0, 0, 1},		//conColorRed
	{1, 0.25f, 0, 1},	//conColorRedOrange
	{1, 0.5f, 0, 1},	//conColorOrange
	{1, 0.75f, 0, 1},	//conColorOrangeYellow
	{1, 1, 0, 1},		//conColorYellow
	{0.5f, 1, 0, 1},	//conColorYellowGreen
	{0, 1, 0, 1},		//conColorGreen
	{0, 1, 0.25f, 1},	//conColorGreenTurq
	{0, 1, 0.5f, 1},	//conColorTurquoise
	{0, 1, 1, 1},		//conColorCyan
	{0, 0.5f, 1, 1},	//conColorCyanBlue
	{0, 0, 1, 1},		//conColorBlue
	{0.25f, 0, 1, 1},	//conColorBluePurple
	{0.5f, 0, 1, 1},	//conColorPurple
	{1, 0, 1, 1},		//conColorPink
	{1, 0, 0.5f, 1}		//conColorMagenta
};

typedef struct {
	const byte type;
	const char *key;
} conDrawFilter_t;
conDrawFilter_t conDrawFilter[] = {
	{CON_FILTER_PUBCHAT,	"^1PC"},
	{CON_FILTER_TEAMCHAT,	"^2TC"},
	{CON_FILTER_SERVER,		"^3SV"},
	{CON_FILTER_CLIENT,		"^4CL"},
};

void Con_DrawFilter(const float x, const float y, const byte filter) {
	static char drawFilter[32];
	size_t len = ARRAY_LEN(conDrawFilter);
	int i;
	byte f = (filter & 0xff);
	byte mask = (byte)CON_FILTER_NONE;
	if (f == mask)
		return;
	Com_sprintf(drawFilter, sizeof(drawFilter), S_COLOR_WHITE"(");
	for (i = 0; i < len; i++) {
		if (conDrawFilter[i].type & f) {
			if (i > 0)
				Q_strcat(drawFilter, sizeof(drawFilter), S_COLOR_BLACK"|");
			Q_strcat(drawFilter, sizeof(drawFilter), conDrawFilter[i].key);
		}
	}
	Q_strcat(drawFilter, sizeof(drawFilter), S_COLOR_WHITE")");
	SCR_DrawSmallStringExt(x - (Q_PrintStrlen(drawFilter) + 1) * SMALLCHAR_WIDTH,
		y, drawFilter, g_color_table[ColorIndex(COLOR_WHITE)], qfalse, qfalse);
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( float frac ) {
	int				i, x, y;
	int				rows;
	int				*text;
	int				row;
	int				lines;
//	qhandle_t		conShader;
	int				currentColor;
	const			char *version = JK_VERSION;

	const byte		filter = Con_GetUserFilter();
	qboolean		filterLine = qfalse;

	lines = (int) (cls.glconfig.vidHeight * frac);
	if (lines <= 0)
		return;

	if (lines > cls.glconfig.vidHeight )
		lines = cls.glconfig.vidHeight;

	// draw the background
	y = (int) (frac * SCREEN_HEIGHT - 2);
	if ( y < 1 ) {
		y = 0;
	}
	else {
		SCR_DrawPic( 0, 0, SCREEN_WIDTH, (float) y, cls.consoleShader );
	}

	const vec4_t color = { 0.509f, 0.609f, 0.847f,  1.0f};
	// draw the bottom bar and version number

	re.SetColor( color );
	re.DrawStretchPic( 0, y, SCREEN_WIDTH, 2, 0, 0, 0, 0, cls.whiteShader );

	i = strlen( version );
	y = (cls.realtime >> 6);
	for (x=0 ; x<i ; x++) {
		if (version[x] ==' ')
			continue;
		/* Hackish use of color table */
		re.SetColor( conColourTable[y&15] );
		y++;
		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH, 
			(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)), version[x] );
	}
#ifdef CON_FILTER
	Con_DrawFilter(cls.glconfig.vidWidth - (i) * SMALLCHAR_WIDTH,
		(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)), filter);
#endif

	// draw the text
	con.vislines = lines;
	rows = (lines-SMALLCHAR_WIDTH)/SMALLCHAR_WIDTH;		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if (con.display != con.current)
	{
	// draw arrows to show the buffer is backscrolled
		re.SetColor( g_color_table[ColorIndex(COLOR_RED)] );
		for (x=0 ; x<con.linewidth ; x+=4)
			SCR_DrawSmallChar( (int) (con.xadjust + (x+1)*SMALLCHAR_WIDTH), y, '^' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}
	
	row = con.display;

	if ( con.x == 0 ) {
		row--;
	}

//	currentColor = 7;
//	re.SetColor( g_color_table[currentColor] );
	currentColor = 0xffffff00;
	re.SetColor( colorWhite );

	static int iFontIndexForAsian = 0;
	const float fFontScaleForAsian = 0.75f*con.yadjust;
	int iPixelHeightToAdvance = SMALLCHAR_HEIGHT;
	if (re.Language_IsAsian())
	{
		if (!iFontIndexForAsian) 
		{
			iFontIndexForAsian = re.RegisterFont("ocr_a");
		}
		iPixelHeightToAdvance = (1.3/con.yadjust) * re.Font_HeightPixels(iFontIndexForAsian, fFontScaleForAsian);	// for asian spacing, since we don't want glyphs to touch.
	}

	for (i=0 ; i<rows ; i++, y -= iPixelHeightToAdvance, row--)
	{
		if (row < 0)
			break;
		if (con.current - row >= con.totallines) {
			// past scrollback wrap point
			continue;	
		}

		text = con.text + (row % con.totallines)*con.linewidth;

		// asian language needs to use the new font system to print glyphs...
		//
		// (ignore colours since we're going to print the whole thing as one string)
		//
		if (re.Language_IsAsian())
		{
			// concat the text to be printed...
			//
			char sTemp[4096]={0};	// ott
			for (x = 0 ; x < con.linewidth ; x++) 
			{
				if ( ( (text[x]>>8)&7 ) != currentColor ) {
					currentColor = (text[x]>>8)&7;
					strcat(sTemp,va("^%i", (text[x]>>8)&7) );
				}
				strcat(sTemp,va("%c",text[x] & 0xFF));				
			}
			//
			// and print...
			//
			re.Font_DrawString(con.xadjust*(con.xadjust + (1*SMALLCHAR_WIDTH/*(aesthetics)*/)), con.yadjust*(y), sTemp, g_color_table[currentColor], iFontIndexForAsian, -1, fFontScaleForAsian);
		}
		else
		{		
			for (x=0 ; x<con.linewidth ; x++) {
#ifdef CON_FILTER
				byte left = ( text[x] & 0xff );
				byte right = (text[x]>>8) & 0xff;
				if ( left == 0 && ( !((right & filter)) ) ) {
					filterLine = qtrue;
					break;
				}
#endif
				if ( ( text[x] & 0xff ) == ' ' ) {
					continue;
				}
				if ( ( text[x] & 0xffffff00 ) != currentColor ) {
					vec4_t setColor;
					currentColor = text[x] & 0xffffff00;
					setColor[0] = ((currentColor >>  8) & 0xff) * (1 / 255.0f);
					setColor[1] = ((currentColor >> 16) & 0xff) * (1 / 255.0f);
					setColor[2] = ((currentColor >> 24) & 0xff) * (1 / 255.0f);
					setColor[3] = 1.0f;
					re.SetColor( setColor );
				}
#ifdef CON_FILTER
				SCR_DrawSmallChar(  (int) (con.xadjust + (x)*SMALLCHAR_WIDTH), y, text[x] & 0xff );
#else
				SCR_DrawSmallChar(  (int) (con.xadjust + (x+1)*SMALLCHAR_WIDTH), y, text[x] & 0xff );
#endif
			}
		}
#ifdef CON_FILTER
		if (filterLine) {
			y += iPixelHeightToAdvance; i--;
		}
		filterLine = qfalse;
#endif
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();

	re.SetColor( NULL );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	// check for console width changes from a vid mode change
	Con_CheckResize ();

	// if disconnected, render console full screen
	if ( cls.state == CA_DISCONNECTED ) {
		if ( !( Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( con.displayFrac ) {
		Con_DrawSolidConsole( con.displayFrac );
	} else {
		// draw notify lines
		if ( cls.state == CA_ACTIVE ) {
			Con_DrawNotify ();
		}
	}
}
//================================================================
/*
==================
Con_RunConsole
Scroll it up or down
==================
*/
void Con_RunConsole(void) {
	// decide on the destination height of the console
	if (Key_GetCatcher( ) & KEYCATCH_CONSOLE) {
#ifdef __ANDROID__
		if (con.state == conShort) {
			con.finalFrac = 0.25;
		} else if (con.state == conFull) {
			con.finalFrac = 1.0;
		} else {
			Com_Error(ERR_FATAL, "conHidden and KEYCATCH_CONSOLE are set together");
		}
#else
		con.finalFrac = 0.5; // half screen
#endif
	} else {
		con.finalFrac = 0; // none visible
	}
	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac) {
		con.displayFrac -= con_conspeed->value*(float)(cls.realFrametime*0.001);
		if (con.finalFrac > con.displayFrac)
			con.displayFrac = con.finalFrac;

	} else if (con.finalFrac > con.displayFrac) {
		con.displayFrac += con_conspeed->value*(float)(cls.realFrametime*0.001);
		if (con.finalFrac < con.displayFrac)
			con.displayFrac = con.finalFrac;
	}
}
void Con_PageUp(void) {
#ifdef __ANDROID__
	con.display -= 1;
#else
	con.display -= 2;
#endif
	if (con.current - con.display >= con.totallines) {
		con.display = con.current - con.totallines + 1;
	}
}
void Con_PageDown(void) {
#ifdef __ANDROID__
	con.display += 1;
#else
	con.display += 2;
#endif
	if (con.display > con.current) {
		con.display = con.current;
	}
}
void Con_Top(void) {
	con.display = con.totallines;
	if (con.current - con.display >= con.totallines) {
		con.display = con.current - con.totallines + 1;
	}
}
void Con_Bottom(void) {
	con.display = con.current;
}
void Con_Close(void) {
	if (!com_cl_running->integer) {
		return;
	}
	Field_Clear(&kg.g_consoleField);
	Con_ClearNotify();
	Key_SetCatcher(Key_GetCatcher() & ~KEYCATCH_CONSOLE);
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
#ifdef __ANDROID__
	if (Key_GetCatcher() & KEYCATCH_CONSOLE)
		con.state = conFull;
	else
		con.state = conHidden;
#endif
}
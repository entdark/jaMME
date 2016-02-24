// Copyright (C) 1999-2000 Id Software, Inc.
//
// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.h"


// this list was made using the model directories, this MUST be in the same order as the CLASS_ enum in teams.h
stringID_table_t ClassTable[] = {
	ENUM2STRING(CLASS_NONE),				// hopefully this will never be used by an npc), just covering all bases
	ENUM2STRING(CLASS_ATST),				// technically droid...
	ENUM2STRING(CLASS_BARTENDER),
	ENUM2STRING(CLASS_BESPIN_COP),		
	ENUM2STRING(CLASS_CLAW),
	ENUM2STRING(CLASS_COMMANDO),
	ENUM2STRING(CLASS_DESANN),			
	ENUM2STRING(CLASS_FISH),
	ENUM2STRING(CLASS_FLIER2),
	ENUM2STRING(CLASS_GALAK),
	ENUM2STRING(CLASS_GLIDER),
	ENUM2STRING(CLASS_GONK),				// droid
	ENUM2STRING(CLASS_GRAN),
	ENUM2STRING(CLASS_HOWLER),
//	ENUM2STRING(CLASS_RANCOR),
	ENUM2STRING(CLASS_IMPERIAL),
	ENUM2STRING(CLASS_IMPWORKER),
	ENUM2STRING(CLASS_INTERROGATOR),		// droid 
	ENUM2STRING(CLASS_JAN),				
	ENUM2STRING(CLASS_JEDI),				
	ENUM2STRING(CLASS_KYLE),
	ENUM2STRING(CLASS_LANDO),			
	ENUM2STRING(CLASS_LIZARD),
	ENUM2STRING(CLASS_LUKE),				
	ENUM2STRING(CLASS_MARK1),			// droid
	ENUM2STRING(CLASS_MARK2),			// droid
	ENUM2STRING(CLASS_GALAKMECH),		// droid
	ENUM2STRING(CLASS_MINEMONSTER),
	ENUM2STRING(CLASS_MONMOTHA),			
	ENUM2STRING(CLASS_MORGANKATARN),
	ENUM2STRING(CLASS_MOUSE),			// droid
	ENUM2STRING(CLASS_MURJJ),
	ENUM2STRING(CLASS_PRISONER),
	ENUM2STRING(CLASS_PROBE),			// droid
	ENUM2STRING(CLASS_PROTOCOL),			// droid
	ENUM2STRING(CLASS_R2D2),				// droid
	ENUM2STRING(CLASS_R5D2),				// droid
	ENUM2STRING(CLASS_REBEL),
	ENUM2STRING(CLASS_REBORN),
	ENUM2STRING(CLASS_REELO),
	ENUM2STRING(CLASS_REMOTE),
	ENUM2STRING(CLASS_RODIAN),
	ENUM2STRING(CLASS_SEEKER),			// droid
	ENUM2STRING(CLASS_SENTRY),
	ENUM2STRING(CLASS_SHADOWTROOPER),
	ENUM2STRING(CLASS_STORMTROOPER),
	ENUM2STRING(CLASS_SWAMP),
	ENUM2STRING(CLASS_SWAMPTROOPER),
	ENUM2STRING(CLASS_TAVION),
	ENUM2STRING(CLASS_TRANDOSHAN),
	ENUM2STRING(CLASS_UGNAUGHT),
	ENUM2STRING(CLASS_JAWA),
	ENUM2STRING(CLASS_WEEQUAY),
	ENUM2STRING(CLASS_BOBAFETT),
	//ENUM2STRING(CLASS_ROCKETTROOPER),
	//ENUM2STRING(CLASS_PLAYER),
	ENUM2STRING(CLASS_VEHICLE),
	ENUM2STRING(CLASS_RANCOR),
	ENUM2STRING(CLASS_WAMPA),
	{"",	-1}
};

/*
-------------------------
GetIDForString 
-------------------------
*/


int GetIDForString ( stringID_table_t *table, const char *string )
{
	int	index = 0;

	while ( ( table[index].name != NULL ) &&
			( table[index].name[0] != 0 ) )
	{
		if ( !Q_stricmp( table[index].name, string ) )
			return table[index].id;

		index++;
	}

	return -1;
}

/*
-------------------------
GetStringForID
-------------------------
*/

const char *GetStringForID( stringID_table_t *table, int id )
{
	int	index = 0;

	while ( ( table[index].name != NULL ) &&
			( table[index].name[0] != 0 ) )
	{
		if ( table[index].id == id )
			return table[index].name;

		index++;
	}

	return NULL;
}

ID_INLINE int Com_Clampi( int min, int max, int value ) 
{
	if ( value < min ) 
	{
		return min;
	}
	if ( value > max ) 
	{
		return max;
	}
	return value;
}

ID_INLINE float Com_Clamp( float min, float max, float value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

// some fucking joker deleted my code for ABSCLAMP, precisely before I was going to use it. so I added this --eez
ID_INLINE int Com_AbsClampi( int min, int max, int value )
{
	if( value < 0 )
	{
		return Com_Clampi( -max, -min, value );
	}
	else
	{
		return Com_Clampi( min, max, value );
	}
}

ID_INLINE float Com_AbsClamp( float min, float max, float value )
{
	if( value < 0.0f )
	{
		return Com_Clamp( -max, -min, value );
	}
	else
	{
		return Com_Clamp( min, max, value );
	}
}


/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (char *pathname)
{
	char	*last;
	
	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

/*
============
COM_GetExtension
============
*/
const char *COM_GetExtension( const char *name )
{
	const char *dot = strrchr(name, '.'), *slash;
	if (dot && (!(slash = strrchr(name, '/')) || slash < dot))
		return dot + 1;
	else
		return "";
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out, int destsize )
{
	const char *dot = strrchr(in, '.'), *slash;
	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		Q_strncpyz(out, in, (destsize < dot-in+1 ? destsize : dot-in+1));
	else
		Q_strncpyz(out, in, destsize);
}

/*
============
COM_CompareExtension

string compare the end of the strings and return qtrue if strings match
============
*/
qboolean COM_CompareExtension(const char *in, const char *ext)
{
	int inlen, extlen;

	inlen = strlen(in);
	extlen = strlen(ext);

	if(extlen <= inlen)
	{
		in += inlen - extlen;

		if(!Q_stricmp(in, ext))
			return qtrue;
	}

	return qfalse;
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( char *path, int maxSize, const char *extension )
{
	const char *dot = strrchr(path, '.'), *slash;
	if (dot && (!(slash = strrchr(path, '/')) || slash < dot))
		return;
	else
		Q_strcat(path, maxSize, extension);
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/
/*
// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
static short	(*_BigShort) (short l);
static short	(*_LittleShort) (short l);
static int		(*_BigLong) (int l);
static int		(*_LittleLong) (int l);
static qint64	(*_BigLong64) (qint64 l);
static qint64	(*_LittleLong64) (qint64 l);
static float	(*_BigFloat) (const float *l);
static float	(*_LittleFloat) (const float *l);

short	BigShort(short l){return _BigShort(l);}
short	LittleShort(short l) {return _LittleShort(l);}
int		BigLong (int l) {return _BigLong(l);}
int		LittleLong (int l) {return _LittleLong(l);}
qint64 	BigLong64 (qint64 l) {return _BigLong64(l);}
qint64 	LittleLong64 (qint64 l) {return _LittleLong64(l);}
float	BigFloat (const float *l) {return _BigFloat(l);}
float	LittleFloat (const float *l) {return _LittleFloat(l);}
*/

short   ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short	ShortNoSwap (short l)
{
	return l;
}

int    LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int	LongNoSwap (int l)
{
	return l;
}

qint64 Long64Swap (qint64 ll)
{
	qint64	result;

	result.b0 = ll.b7;
	result.b1 = ll.b6;
	result.b2 = ll.b5;
	result.b3 = ll.b4;
	result.b4 = ll.b3;
	result.b5 = ll.b2;
	result.b6 = ll.b1;
	result.b7 = ll.b0;

	return result;
}

qint64 Long64NoSwap (qint64 ll)
{
	return ll;
}

float FloatSwap (const float *f) {
	floatint_t out;

	out.f = *f;
	out.ui = LongSwap(out.ui);

	return out.f;
}

float FloatNoSwap (const float *f)
{
	return *f;
}

/*
================
Swap_Init
================
*/
/*
void Swap_Init (void)
{
	byte	swaptest[2] = {1,0};

// set the byte swapping variables in a portable manner	
	if ( *(short *)swaptest == 1)
	{
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigLong64 = Long64Swap;
		_LittleLong64 = Long64NoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
	}
	else
	{
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigLong64 = Long64NoSwap;
		_LittleLong64 = Long64Swap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
	}

}
*/

/*
============================================================================

PARSING

============================================================================
*/

static	char	com_token[MAX_TOKEN_CHARS];
static	char	com_parsename[MAX_TOKEN_CHARS];
static	int		com_lines;

void COM_BeginParseSession( const char *name )
{
	com_lines = 0;
	Com_sprintf(com_parsename, sizeof(com_parsename), "%s", name);
}

int COM_GetCurrentParseLine( void )
{
	return com_lines;
}

char *COM_Parse( const char **data_p )
{
	return COM_ParseExt( data_p, qtrue );
}

void COM_ParseError( char *format, ... )
{
	va_list argptr;
	static char string[4096];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof( string ), format, argptr);
	va_end (argptr);

	Com_Printf("ERROR: %s, line %d: %s\n", com_parsename, com_lines, string);
}

void COM_ParseWarning( char *format, ... )
{
	va_list argptr;
	static char string[4096];

	va_start (argptr, format);
	Q_vsnprintf (string, sizeof( string ), format, argptr);
	va_end (argptr);

	Com_Printf("WARNING: %s, line %d: %s\n", com_parsename, com_lines, string);
}

/*
==============
COM_Parse

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is
a newline.
==============
*/
const char *SkipWhitespace( const char *data, qboolean *hasNewLines ) {
	int c;

	while( (c = *data) <= ' ') {
		if( !c ) {
			return NULL;
		}
		if( c == '\n' ) {
			com_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

int COM_Compress( char *data_p ) {
	char *in, *out;
	int c;
	qboolean newline = qfalse, whitespace = qfalse;
	
	in = out = data_p;
	if (in) {
		while ((c = *in) != 0) {
			// skip double slash comments
			if ( c == '/' && in[1] == '/' ) {
				while (*in && *in != '\n') {
					in++;
				}
				// skip /* */ comments
			} else if ( c == '/' && in[1] == '*' ) {
				while ( *in && ( *in != '*' || in[1] != '/' ) ) 
					in++;
				if ( *in ) 
					in += 2;
				// record when we hit a newline
			} else if ( c == '\n' || c == '\r' ) {
				newline = qtrue;
				in++;
				// record when we hit whitespace
			} else if ( c == ' ' || c == '\t') {
				whitespace = qtrue;
				in++;
				// an actual token
			} else {
				// if we have a pending newline, emit it (and it counts as whitespace)
				if (newline) {
					*out++ = '\n';
					newline = qfalse;
					whitespace = qfalse;
				} if (whitespace) {
					*out++ = ' ';
					whitespace = qfalse;
				}
				
				// copy quoted strings unmolested
				if (c == '"') {
					*out++ = c;
					in++;
					while (1) {
						c = *in;
						if (c && c != '"') {
							*out++ = c;
							in++;
						} else {
							break;
						}
					}
					if (c == '"') {
						*out++ = c;
						in++;
					}
				} else {
					*out = c;
					out++;
					in++;
				}
			}
		}
	}
	*out = 0;
	return out - data_p;
}

char *COM_ParseExt( const char **data_p, qboolean allowLineBreaks )
{
	int c = 0, len;
	qboolean hasNewLines = qfalse;
	const char *data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = NULL;
		return com_token;
	}

	while ( 1 )
	{
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );
		if ( !data )
		{
			*data_p = NULL;
			return com_token;
		}
		if ( hasNewLines && !allowLineBreaks )
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			data += 2;
			while (*data && *data != '\n') {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			data += 2;
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS -1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if ( c == '\n' )
			com_lines++;
	} while (c>32);

	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}


#if 0
// no longer used
/*
===============
COM_ParseInfos
===============
*/
int COM_ParseInfos( char *buf, int max, char infos[][MAX_INFO_STRING] ) {
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		infos[count][0] = 0;
		while ( 1 ) {
			token = COM_ParseExt( &buf, qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( infos[count], key, token );
		}
		count++;
	}

	return count;
}
#endif

/*
===============
COM_ParseString
===============
*/
qboolean COM_ParseString( const char **data, const char **s ) 
{
//	*s = COM_ParseExt( data, qtrue );
	*s = COM_ParseExt( data, qfalse );
	if ( s[0] == 0 ) 
	{
		Com_Printf("unexpected EOF\n");
		return qtrue;
	}
	return qfalse;
}

/*
===============
COM_ParseInt
===============
*/
qboolean COM_ParseInt( const char **data, int *i ) 
{
	const char	*token;

	token = COM_ParseExt( data, qfalse );
	if ( token[0] == 0 ) 
	{
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	*i = atoi( token );
	return qfalse;
}

/*
===============
COM_ParseFloat
===============
*/
qboolean COM_ParseFloat( const char **data, float *f ) 
{
	const char	*token;

	token = COM_ParseExt( data, qfalse );
	if ( token[0] == 0 ) 
	{
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	*f = atof( token );
	return qfalse;
}

/*
===============
COM_ParseVec4
===============
*/
qboolean COM_ParseVec4( const char **buffer, vec4_t *c) 
{
	int i;
	float f;

	for (i = 0; i < 4; i++) 
	{
		if (COM_ParseFloat(buffer, &f)) 
		{
			return qtrue;
		}
		(*c)[i] = f;
	}
	return qfalse;
}

/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken( const char **buf_p, char *match ) {
	char	*token;

	token = COM_Parse( buf_p );
	if ( strcmp( token, match ) ) {
		Com_Error( ERR_DROP, "MatchToken: %s != %s", token, match );
	}
}


/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
void SkipBracedSection(const char **program) {
	char			*token;
	int				depth;

	depth = 0;
	do {
		token = COM_ParseExt( program, qtrue );
		if( token[1] == 0 ) {
			if( token[0] == '{' ) {
				depth++;
			}
			else if( token[0] == '}' ) {
				depth--;
			}
		}
	} while( depth && *program );
}
qboolean SkipBracedSection2(const char **program, int depth) {
	char *token;
	do {
		token = COM_ParseExt( program, qtrue );
		if( token[1] == 0 ) {
			if( token[0] == '{' ) {
				depth++;
			}
			else if( token[0] == '}' ) {
				depth--;
			}
		}
	} while( depth && *program );
	return (qboolean)( depth == 0 );
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine ( const char **data ) {
	const char	*p;
	int		c;

	p = *data;
	while ( (c = *p++) != 0 ) {
		if ( c == '\n' ) {
			com_lines++;
			break;
		}
	}

	*data = p;
}


void Parse1DMatrix (const char **buf_p, int x, float *m) {
	char	*token;
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < x ; i++) {
		token = COM_Parse(buf_p);
		m[i] = atof(token);
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse2DMatrix (const char **buf_p, int y, int x, float *m) {
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < y ; i++) {
		Parse1DMatrix (buf_p, x, m + i * x);
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse3DMatrix (const char **buf_p, int z, int y, int x, float *m) {
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < z ; i++) {
		Parse2DMatrix (buf_p, y, x, m + i * x*y);
	}

	COM_MatchToken( buf_p, ")" );
}

/*
===================
Com_HexStrToInt
===================
*/
int Com_HexStrToInt( const char *str )
{
	if ( !str || !str[ 0 ] )
		return -1;

	// check for hex code
	if( str[ 0 ] == '0' && str[ 1 ] == 'x' )
	{
		int i, n = 0;

		for( i = 2; i < strlen( str ); i++ )
		{
			char digit;

			n *= 16;

			digit = tolower( str[ i ] );

			if( digit >= '0' && digit <= '9' )
				digit -= '0';
			else if( digit >= 'a' && digit <= 'f' )
				digit = digit - 'a' + 10;
			else
				return -1;

			n += digit;
		}

		return n;
	}

	return -1;
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_isprint( int c )
{
	if ( c >= 0x20 && c <= 0x7E )
		return ( 1 );
	return ( 0 );
}

int Q_islower( int c )
{
	if (c >= 'a' && c <= 'z')
		return ( 1 );
	return ( 0 );
}

int Q_isupper( int c )
{
	if (c >= 'A' && c <= 'Z')
		return ( 1 );
	return ( 0 );
}

int Q_isalpha( int c )
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return ( 1 );
	return ( 0 );
}

qboolean Q_isanumber( const char *s )
{
	char *p;
	double ret;

	if( *s == '\0' )
		return qfalse;

	ret = strtod( s, &p );
	
	if ( ret == HUGE_VAL || errno == ERANGE )
		return qfalse;

	return (qboolean)(*p == '\0');
}

qboolean Q_isintegral( float f )
{
	return (qboolean)( (int)f == f );
}

char* Q_strrchr( const char* string, int c )
{
	char cc = c;
	char *s;
	char *sp=(char *)0;

	s = (char*)string;

	while (*s)
	{
		if (*s == cc)
			sp = s;
		s++;
	}
	if (cc == 0)
		sp = s;

	return sp;
}

/*
=============
Q_strncpyz
 
Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz( char *dest, const char *src, int destsize ) {
	// bk001129 - also NULL dest
	if ( !dest ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );
		return;
	}
	if ( !src ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
		return;
	}
	if ( destsize < 1 ) {
		Com_Error( ERR_FATAL,"Q_strncpyz: destsize < 1" ); 
		return;
	}

	strncpy( dest, src, destsize-1 );
	dest[destsize-1] = 0;
}

int Q_stricmpn (const char *s1, const char *s2, int n) {
	int		c1, c2;

	// bk001129 - moved in 1.17 fix not in id codebase
        if ( s1 == NULL ) {
           if ( s2 == NULL )
             return 0;
           else
             return -1;
        }
        else if ( s2==NULL )
          return 1;


	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_strncmp (const char *s1, const char *s2, int n) {
	int		c1, c2;
	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_stricmp (const char *s1, const char *s2) {
	return (s1 && s2) ? Q_stricmpn (s1, s2, 99999) : -1;
}


char *Q_strlwr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = tolower(*s);
		s++;
	}
    return s1;
}

char *Q_strupr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = toupper(*s);
		s++;
	}
    return s1;
}


// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src ) {
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}

vec3_t defaultColors[10] =
{
	{0.0f, 0.0f, 0.0f},
	{1.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f},
	{1.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 1.0f},
	{0.0f, 1.0f, 1.0f},
	{1.0f, 0.0f, 1.0f},
	{1.0f, 1.0f, 1.0f},
	{0.0f, 0.0f, 0.0f},
	{1.0f, 0.0f, 0.0f},
};
vec3_t uagColors[43] =
{
	{ 0.000, 0.000, 0.000 }, // '0' 0x30 48:   0   0   0
	{ 1.000, 0.000, 0.000 }, // '1' 0x31 49: 255   0   0
	{ 0.000, 1.000, 0.000 }, // '2' 0x32 50:   0 255   0
	{ 1.000, 1.000, 0.000 }, // '3' 0x33 51: 255 255   0
	{ 0.000, 0.000, 1.000 }, // '4' 0x34 52:   0   0 255
	{ 0.000, 1.000, 1.000 }, // '5' 0x35 53:   0 255 255
	{ 1.000, 0.000, 1.000 }, // '6' 0x36 54: 255   0 255
	{ 1.000, 1.000, 1.000 }, // '7' 0x37 55: 255 255 255
// '8'-'@' will be skipped, but '8' and '9' will appear in the console as black and red (as in original jamp)
	{ 0.000, 0.000, 0.000 }, // '8' 0x38 56:   0   0   0
	{ 1.000, 0.000, 0.000 }, // '9' 0x39 57: 255   0   0
	{ 1.000, 1.000, 1.000 }, // ':' 0x3A 58: 255 255 255
	{ 1.000, 1.000, 1.000 }, // ';' 0x3B 59: 255 255 255
	{ 1.000, 1.000, 1.000 }, // '<' 0x3C 60: 255 255 255
	{ 1.000, 1.000, 1.000 }, // '=' 0x3D 61: 255 255 255
	{ 1.000, 1.000, 1.000 }, // '>' 0x3E 62: 255 255 255
	{ 1.000, 1.000, 1.000 }, // '?' 0x3F 63: 255 255 255
	{ 1.000, 1.000, 1.000 }, // '@' 0x40 64: 255 255 255
// end of skip
	{ 1.000, 0.561, 0.561 }, // 'A' 0x41 65: 255 143 143
	{ 0.500, 0.039, 0.039 }, // 'B' 0x42 66: 127  10  10
	{ 0.500, 0.302, 0.027 }, // 'C' 0x43 67: 127  77   7
	{ 1.000, 0.584, 0.000 }, // 'D' 0x44 68: 255 149   0
	{ 0.843, 0.757, 0.545 }, // 'E' 0x45 69: 215 193 139
	{ 1.000, 1.000, 0.612 }, // 'F' 0x46 70: 255 255 156
	{ 0.647, 1.000, 0.655 }, // 'G' 0x47 71: 165 255 167
	{ 0.541, 0.863, 0.035 }, // 'H' 0x48 72: 138 220   9
	{ 0.004, 0.475, 0.004 }, // 'I' 0x49 73:   1 121   1
	{ 0.439, 0.447, 0.176 }, // 'J' 0x4A 74: 112 114  45
	{ 0.388, 0.490, 0.321 }, // 'K' 0x4B 75:  99 125  82
	{ 0.000, 0.902, 0.659 }, // 'L' 0x4C 76:   0 230 168
	{ 0.612, 0.996, 1.000 }, // 'M' 0x4D 77: 156 254 255
	{ 0.000, 0.580, 0.580 }, // 'N' 0x4E 78:   0 148 148
	{ 0.321, 0.482, 0.510 }, // 'O' 0x4F 79:  82 123 130
	{ 0.000, 0.667, 1.000 }, // 'P' 0x50 80:   0 170 255
	{ 0.494, 0.500, 1.000 }, // 'Q' 0x51 81: 126 127 255
	{ 0.306, 0.306, 0.573 }, // 'R' 0x52 82:  78  78 146
	{ 0.518, 0.000, 1.000 }, // 'S' 0x53 83: 132   0 255
	{ 0.769, 0.518, 1.000 }, // 'T' 0x54 84: 196 132 255
	{ 0.510, 0.329, 0.506 }, // 'U' 0x55 85: 130  84 129
	{ 0.988, 0.608, 0.937 }, // 'V' 0x56 86: 252 155 239
	{ 1.000, 0.000, 0.500 }, // 'W' 0x57 87: 255   0 127
	{ 0.596, 0.000, 0.588 }, // 'X' 0x58 88: 152   0 150
	{ 0.725, 0.725, 0.725 }, // 'Y' 0x59 89: 185 185 185
	{ 0.329, 0.329, 0.329 }, // 'Z' 0x5A 90:  84  84  84
};

int Q_parseColor( const char *p, const vec3_t *numberColors, float *color ) {
	char c = *p++;
	if (((c >= '0' && c <='9') || (c >= 'A' && c <= 'Z')) && numberColors == &(uagColors[0])) {
		if (!color)
			return 1;
		memcpy( color, numberColors + c - '0', sizeof( vec3_t ));
		return 1;
	} else if (c >= '0' && c <='9') {
		if (!color)
			return 1;
		memcpy( color, numberColors + c - '0', sizeof( vec3_t ));
		return 1;
/*	} else if ( ( c >= 'a' && c < 'u') || (c >= 'A' && c < 'U') ) {
		int deg;
		float angle, v;
		if (!color)
			return 1;
		deg = (((c|32) - 'a') * 360) / 24;
		angle = (DEG2RAD(deg % 120));
		v = ((cos(angle) / cos((M_PI / 3) - angle)) + 1) / 3;
		if ( deg <= 120) {
			color[0] = v;
			color[1] = 1-v;
			color[2] = 0;
		} else if ( deg <= 240) {
			color[0] = 0;
			color[1] = v;
			color[2] = 1-v;
		} else {
			color[0] = 1-v;
			color[1] = 0;
			color[2] = v;
		}
		return 1;*/
	} else if ( c == 'u' || c == 'U' || c == 'v' || c == 'V'
				|| c == 'w' || c == 'W' || c == 'x' || c == 'X'
				|| c == 'y' || c == 'Y' || c == 'z' || c == 'Z'
				|| c == '#') {
		int i;
		int val;
		for (i = 0;i<6;i++) {
            int readHex;
			c = p[i];
			if ( c >= '0' && c<= '9') {
                readHex = c - '0';
			} else if ( c >= 'a' && c<= 'f') {
				readHex = 0xa + c - 'a';
			} else if ( c >= 'A' && c<= 'F') {
				readHex = 0xa + c - 'A';
			} else {
/*				if (color) {
					color[0] = color[1] = color[2] = 1.0f;
				}*/
				return -1;
			}
			if (!color)
				continue;
			if ( i & 1) {
				val|= readHex;
				color[i >> 1] = val * (1 / 255.0f);
			} else {
				val = readHex << 4;
			}
		}
		return 7;
	}
/*	if (color) {
		color[0] = color[1] = color[2] = 1.0f;
	}*/
	return -1;
}

int Q_parseColorString( const char *p, float *color, qboolean uagColours) {
	char c;
	if (!p )
		return 0;
	c = *p;
	if (!c || c!= Q_COLOR_ESCAPE || (c== Q_COLOR_ESCAPE && !p[1]))
		return 0;
	return 1 + Q_parseColor( p+1, !(uagColours)?(defaultColors):(uagColors), color );
}

/*
* Find the first occurrence of find in s.
*/
const char *Q_stristr( const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0)
  {
    if (c >= 'a' && c <= 'z')
    {
      c -= ('a' - 'A');
    }
    len = strlen(find);
    do
    {
      do
      {
        if ((sc = *s++) == 0)
          return NULL;
        if (sc >= 'a' && sc <= 'z')
        {
          sc -= ('a' - 'A');
        }
      } while (sc != c);
    } while (Q_stricmpn(s, find, len) != 0);
    s--;
  }
  return s;
}

#if defined (UI_EXPORTS) || (QAGAME)
static const qboolean uagColours = qfalse;
#else
#endif
int Q_PrintStrlen( const char *string
#if defined (UI_EXPORTS) || (QAGAME)
#else
	, qboolean uagColours
#endif
	) {
	int			len;
	const char	*p;

	if( !string ) {
		return 0;
	}

	len = 0;
	p = string;
	while( *p ) {
		int colorLen = Q_parseColorString( p, 0, uagColours);
		if( colorLen ) {
			p += colorLen;
			continue;
		}
		p++;
		len++;
	}

	return len;
}


char *Q_CleanStr( char *string
#if defined (UI_EXPORTS) || (QAGAME)
#else
	, qboolean uagColours
#endif
	) {
	char*	d;
	char*	s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0 ) {
		int colorLen = Q_parseColorString( s, 0, uagColours);
		if ( colorLen ) {
			s += colorLen;
			continue;
		}		
		if ( c >= 0x20 && c <= 0x7E ) {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

/*
==================
Q_StripColor
 
Strips coloured strings in-place using multiple passes: "fgs^^56fds" -> "fgs^6fds" -> "fgsfds"

(Also strips ^8 and ^9)
==================
*/
void Q_StripColor(char *text
#if defined (UI_EXPORTS) || (QAGAME)
#else
	, qboolean uagColours
#endif
	) {
	qboolean doPass = qtrue;
	char *read;
	char *write;

	while ( doPass )
	{
		doPass = qfalse;
		read = write = text;
		while ( *read )
		{
			int colorLen = Q_parseColorString( read, 0, uagColours);
			if( colorLen ) {
				doPass = qtrue;
				read += colorLen;
			}
			else
			{
				// Avoid writing the same data over itself
				if (write != read)
				{
					*write = *read;
				}
				write++;
				read++;
			}
		}
		if ( write < read )
		{
			// Add trailing NUL byte if string has shortened
			*write = '\0';
		}
	}
}

/*
==================
Q_StripColorNew
 
Strips coloured strings in-place: "fgs^^^223fds" -> "fgs^^23fds"
==================
*/
void Q_StripColorNew(char *text
#if defined (UI_EXPORTS) || (QAGAME)
#else
	, qboolean uagColours
#endif
	) {
	char *read;
	char *write;

	read = write = text;
	while ( *read ) {
		int colorLen = Q_parseColorString( read, 0, uagColours);
		if( colorLen ) {
			read += colorLen;
		} else {
			// Avoid writing the same data over itself
			if (write != read) {
				*write = *read;
			}
			write++;
			read++;
		}
	}
	if ( write < read ) {
		// Add trailing NUL byte if string has shortened
		*write = '\0';
	}
}

/*
Q_strstrip

	Description:	Replace strip[x] in string with repl[x] or remove characters entirely
	Mutates:		string
	Return:			--

	Examples:		Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "123" );	// "Bo1b is h2airy33"
					Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "12" );	// "Bo1b is h2airy"
					Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", NULL );	// "Bob is hairy"
*/

void Q_strstrip( char *string, const char *strip, const char *repl )
{
	char		*out=string, *p=string, c;
	const char	*s=strip;
	int			replaceLen = repl?strlen( repl ):0, offset=0;

	while ( (c = *p++) != '\0' )
	{
		for ( s=strip; *s; s++ )
		{
			offset = s-strip;
			if ( c == *s )
			{
				if ( !repl || offset >= replaceLen )
					c = *p++;
				else
					c = repl[offset];
				break;
			}
		}
		*out++ = c;
	}
	*out = '\0';
}

/*
Q_strchrs

	Description:	Find any characters in a string. Think of it as a shorthand strchr loop.
	Mutates:		--
	Return:			first instance of any characters found
					 otherwise NULL
*/

const char *Q_strchrs( const char *string, const char *search )
{
	const char *p = string, *s = search;

	while ( *p != '\0' )
	{
		for ( s=search; *s; s++ )
		{
			if ( *p == *s )
				return p;
		}
		p++;
	}

	return NULL;
}

#ifdef _MSC_VER
/*
=============
Q_vsnprintf
 
Special wrapper function for Microsoft's broken _vsnprintf() function.
MinGW comes with its own snprintf() which is not broken.
=============
*/

int Q_vsnprintf(char *str, size_t size, const char *format, va_list ap) {
	int retval;
	retval = _vsnprintf(str, size, format, ap);
	if(retval < 0 || retval == size) {
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.
		str[size - 1] = '\0';
		return size;
	}
	return retval;
}
#endif

//Raz: Patched version of Com_sprintf
//Ensiform: But this is better
int QDECL Com_sprintf( char *dest, int size, const char *fmt, ...) {
	int		len;
	va_list		argptr;

	va_start (argptr,fmt);
	len = Q_vsnprintf(dest, size, fmt, argptr);
	va_end (argptr);

	if(len >= size)
		Com_Printf("Com_sprintf: Output length %d too short, require %d bytes.\n", size, len + 1);
	
	return len;
}

void QDECL Com_sprintfOld( char *dest, int size, const char *fmt, ...) {
	int		len;
	va_list		argptr;
	char	bigbuffer[32000];	// big, but small enough to fit in PPC stack

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);
	if ( len >= sizeof( bigbuffer ) ) {
		Com_Error( ERR_FATAL, "Com_sprintf: overflowed bigbuffer" );
	}
	if (len >= size) {
		Com_Printf ("Com_sprintf: overflow of %i in %i\n", len, size);
#if 0 //def	_DEBUG
		__asm {
			int 3;
		}
#endif
	}
	Q_strncpyz (dest, bigbuffer, size );
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
#define	MAX_VA_STRING	32000
#define MAX_VA_BUFFERS 4

char * QDECL va( const char *format, ... )
{
	va_list		argptr;
	static char	string[MAX_VA_BUFFERS][MAX_VA_STRING];	// in case va is called by nested functions
	static int	index = 0;
	char		*buf;

	va_start( argptr, format );
	buf = (char *)&string[index++ & 3];
	Q_vsnprintf( buf, MAX_VA_STRING-1, format, argptr );
	va_end( argptr );

	return buf;
}


/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
char *Info_ValueForKey( const char *s, const char *key ) {
	char	pkey[BIG_INFO_KEY];
	static	char value[2][BIG_INFO_VALUE];	// use two buffers so compares
											// work without stomping on each other
	static	int	valueindex = 0;
	char	*o;
	
	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring" );
	}

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp (key, pkey) )
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}

/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair( const char **head, char *key, char *value ) {
	char	*o;
	const char	*s;

	s = *head;

	if ( *s == '\\' ) {
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	o = key;
	while ( *s != '\\' ) {
		if ( !*s ) {
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while ( *s != '\\' && *s ) {
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}

/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( char *s, const char *key ) {
	char	*start = NULL, *o = NULL;
	char	pkey[MAX_INFO_KEY] = {0};
	char	value[MAX_INFO_VALUE] = {0};

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring" );
		return;
	}

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		//OJKNOTE: static analysis pointed out pkey may not be null-terminated
		if (!strcmp (key, pkey) )
		{
			memmove(start, s, strlen(s) + 1);	// remove this part
			return;
		}

		if (!*s)
			return;
	}
}

/*
===================
Info_RemoveKey_Big
===================
*/
void Info_RemoveKey_Big( char *s, const char *key ) {
	char	*start;
	static char	pkey[BIG_INFO_KEY], value[BIG_INFO_VALUE];
	char	*o;

	pkey[0] = value[0] = '\0';

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey_Big: oversize infostring" );
		return;
	}

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		//OJKNOTE: static analysis pointed out pkey may not be null-terminated
		if (!strcmp (key, pkey) )
		{
			memmove(start, s, strlen(s) + 1);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}

/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s ) {
	const char *c = s;

	while ( *c != '\0' )
	{
		if( !Q_isprint( *c ) )
			return qfalse;

		if( *c == '\"' )
			return qfalse;

		if( *c == ';' )
			return qfalse;

		++c;
	}

	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char	newi[MAX_INFO_STRING];
	const char* blacklist = "\\;\"";

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	for(; *blacklist; ++blacklist)
	{
		if (strchr (key, *blacklist) || strchr (value, *blacklist))
		{
			Com_Printf (S_COLOR_YELLOW "Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}
	}

	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) >= MAX_INFO_STRING)
	{
		Com_Printf ("Info string length exceeded\n");
		return;
	}

	strcat (newi, s);
	strcpy (s, newi);
}

/*
==================
Info_SetValueForKey_Big

Changes or adds a key/value pair
Includes and retains zero-length values
==================
*/
void Info_SetValueForKey_Big( char *s, const char *key, const char *value ) {
	char	newi[BIG_INFO_STRING];
	const char* blacklist = "\\;\"";

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey_Big: oversize infostring" );
	}

	for(; *blacklist; ++blacklist)
	{
		if (strchr (key, *blacklist) || strchr (value, *blacklist))
		{
			Com_Printf (S_COLOR_YELLOW "Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}
	}

	Info_RemoveKey_Big (s, key);
	if (!value)
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) >= BIG_INFO_STRING)
	{
		Com_Printf ("BIG Info string length exceeded\n");
		return;
	}

	strcat (s, newi);
}

int demo_protocols[] =
{ 26, 25, NULL };
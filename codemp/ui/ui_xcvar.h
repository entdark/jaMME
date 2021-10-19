#ifdef XCVAR_PROTO
	#define XCVAR_DEF( name, defVal, update, flags ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, update, flags ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
	#define XCVAR_DEF( name, defVal, update, flags ) { & name , #name , defVal , update , flags },
#endif

//from japro
XCVAR_DEF( cg_crosshairColor,			    	"255 0 255 255",				NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_speedometer,			    	    "0",				NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_speedometerSize,			        "0.75",				NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_speedometerY,			    	    "200",				NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_speedometerX,			    	    "250",				NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_raceTimer,			    	    "2",				NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_raceTimerX,			    	    "170",				NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_raceTimerSize,			        "0.75",				NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_raceTimerY,			    	    "200",				NULL,					CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelperActiveColor,			"0 255 0 255",				NULL,			    	CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelperInactiveAlpha,		"255",				NULL,			    	CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelperLineWidth,		    "0.75",				NULL,			    	CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelperPrecision,		    "512",				NULL,			    	CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelper,			            "0",				    NULL,			    	CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelperCutoff,	            "0",				    NULL,			    	CVAR_ARCHIVE )
XCVAR_DEF( cg_speedometer,			            "0",				    NULL,			    	CVAR_ARCHIVE )
XCVAR_DEF( cg_movementKeys,			            "0",				    NULL,			    	CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelperOffset,			    "0",				    NULL,			    	CVAR_ARCHIVE )
XCVAR_DEF( cg_strafeHelper_FPS,			        "1000",				    NULL,			    	CVAR_ARCHIVE )
XCVAR_DEF( cg_showpos,			                "0",				    NULL,			    	CVAR_ARCHIVE )

#undef XCVAR_DEF

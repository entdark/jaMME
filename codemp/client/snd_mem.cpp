//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

// snd_mem.c: sound caching

#include "snd_local.h"
#include "snd_mp3.h"
#include "snd_ambient.h"

#ifndef _WIN32
#include <algorithm>
#include <string>
#include "qcommon/platform.h"
#endif

// Open AL
void S_PreProcessLipSync(sfx_t *sfx);
extern int s_UseOpenAL;
/*
===============================================================================

WAV loading

===============================================================================
*/

byte	*data_p;
byte 	*iff_end;
byte 	*last_chunk;
byte 	*iff_data;
int 	iff_chunk_len;
extern sfx_t		s_knownSfx[];
extern	int			s_numSfx;

extern cvar_t		*s_lip_threshold_1;
extern cvar_t		*s_lip_threshold_2;
extern cvar_t		*s_lip_threshold_3;
extern cvar_t		*s_lip_threshold_4;

short GetLittleShort(void)
{
	short val = 0;
	val = *data_p;
	val = (short)(val + (*(data_p+1)<<8));
	data_p += 2;
	return val;
}

int GetLittleLong(void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	val = val + (*(data_p+2)<<16);
	val = val + (*(data_p+3)<<24);
	data_p += 4;
	return val;
}

void FindNextChunk(char *name)
{
	while (1)
	{
		data_p=last_chunk;

		if (data_p >= iff_end)
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}
		
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
		data_p -= 8;
		last_chunk = data_p + 8 + ( (iff_chunk_len + 1) & ~1 );
		if (!strncmp((char *)data_p, name, 4))
			return;
	}
}

void FindChunk(char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
}


void DumpChunks(void)
{
	char	str[5];
	
	str[4] = 0;
	data_p=iff_data;
	do
	{
		memcpy (str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		Com_Printf ("0x%x : %s (%d)\n", (intptr_t)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
}

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo (const char *name, byte *wav, int wavlength)
{
	wavinfo_t	info;
	int		samples;

	memset (&info, 0, sizeof(info));

	if (!wav)
		return info;
		
	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !strncmp((char *)data_p+8, "WAVE", 4)))
	{
		Com_Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		Com_Printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	info.format = GetLittleShort();
	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

	if (info.format != 1)
	{
		Com_Printf("Microsoft PCM format only\n");
		return info;
	}


// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		Com_Printf("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	samples = GetLittleLong () / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Com_Error (ERR_DROP, "Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;


	return info;
}


/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
void ResampleSfx (sfx_t *sfx, int iInRate, int iInWidth, byte *pData)
{
	int		iOutCount;
	int		iSrcSample;
	float	fStepScale;
	int		i;
	int		iSample;
	unsigned int uiSampleFrac, uiFracStep;	// uiSampleFrac MUST be unsigned, or large samples (eg music tracks) crash
	
	fStepScale = (float)iInRate / dma.speed;	// this is usually 0.5, 1, or 2

	// When stepscale is > 1 (we're downsampling), we really ought to run a low pass filter on the samples

	iOutCount = (int)(sfx->iSoundLengthInSamples / fStepScale);
	sfx->iSoundLengthInSamples = iOutCount;

	sfx->pSoundData = (short *) SND_malloc( sfx->iSoundLengthInSamples*2 ,sfx );

	sfx->fVolRange	= 0;
	uiSampleFrac	= 0;
	uiFracStep		= (int)(fStepScale*256);

	for (i=0 ; i<sfx->iSoundLengthInSamples ; i++)
	{
		iSrcSample = uiSampleFrac >> 8;
		uiSampleFrac += uiFracStep;
		if (iInWidth == 2) {
			iSample = LittleShort ( ((short *)pData)[iSrcSample] );			
		} else {
			iSample = (int)( (unsigned char)(pData[iSrcSample]) - 128) << 8;			
		}

		sfx->pSoundData[i] = (short)iSample;

		// work out max vol for this sample...
		//
		if (iSample < 0)
			iSample = -iSample;
		if (sfx->fVolRange < (iSample >> 8) )
		{
			sfx->fVolRange =  iSample >> 8;
		}
	}
}


//=============================================================================


void S_LoadSound_Finalize(wavinfo_t	*info, sfx_t *sfx, byte *data)
{				   
	float	stepscale	= (float)info->rate / dma.speed;	
	int		len			= (int)(info->samples / stepscale);

	len *= info->width;

	sfx->eSoundCompressionMethod = ct_16;
	sfx->iSoundLengthInSamples	 = info->samples;
	ResampleSfx( sfx, info->rate, info->width, data + info->dataofs );	
}





// maybe I'm re-inventing the wheel, here, but I can't see any functions that already do this, so...
//
char *Filename_WithoutPath(const char *psFilename)
{
	static char sString[MAX_QPATH];	// !!
	const char *p = strrchr(psFilename,'\\');

  	if (!p++)
		p=psFilename;

	strcpy(sString,p);

	return sString;

}

// returns (eg) "\dir\name" for "\dir\name.bmp"
//
char *Filename_WithoutExt(const char *psFilename)
{
	static char sString[MAX_QPATH];	// !

	strcpy(sString,psFilename);

	char *p = strrchr(sString,'.');		
	char *p2= strrchr(sString,'\\');

	// special check, make sure the first suffix we found from the end wasn't just a directory suffix (eg on a path'd filename with no extension anyway)
	//
	if (p && (p2==0 || (p2 && p>p2)))
		*p=0;	

	return sString;

}



int iFilesFound;
int iFilesUpdated;
int iErrors;
sboolean qbForceRescan;
sboolean qbForceStereo;
string strErrors;

void R_CheckMP3s( const char *psDir )
{
//	Com_Printf(va("Scanning Dir: %s\n",psDir));
	Com_Printf(".");	// stops useful info scrolling off screen

	char	**sysFiles, **dirFiles;
	int		numSysFiles, i, numdirs;

	dirFiles = FS_ListFiles( psDir, "/", &numdirs);
	if (numdirs > 2)
	{
		for (i=2;i<numdirs;i++)
		{
			char	sDirName[MAX_QPATH];
			sprintf(sDirName, "%s\\%s", psDir, dirFiles[i]);
			R_CheckMP3s(sDirName);
		}
	}

	sysFiles = FS_ListFiles( psDir, ".mp3", &numSysFiles );
	for(i=0; i<numSysFiles; i++)
	{
		char	sFilename[MAX_QPATH];
		sprintf(sFilename,"%s\\%s", psDir, sysFiles[i]);		
			
		Com_Printf("%sFound file: %s",!i?"\n":"",sFilename);

		iFilesFound++;

		// read it in...
		//
		byte *pbData = NULL;
		int iSize = FS_ReadFile( sFilename, (void **)&pbData);

		if (pbData)
		{
			id3v1_1* pTAG;

			// do NOT check 'qbForceRescan' here as an opt, because we need to actually fill in 'pTAG' if there is one...
			//
			sboolean qbTagNeedsUpdating = (/* qbForceRescan || */ !MP3_ReadSpecialTagInfo(pbData, iSize, &pTAG))?qtrue:qfalse;

			if (pTAG == NULL || qbTagNeedsUpdating || qbForceRescan)
			{
				Com_Printf(" ( Updating )\n");

				// I need to scan this file to get the volume...
				//
				// For EF1 I used a temp sfx_t struct, but I can't do that now with this new alloc scheme,
				//	I have to ask for it legally, so I'll keep re-using one, and restoring it's name after use.
				//	(slightly dodgy, but works ok if no-one else changes stuff)
				//
				//sfx_t SFX = {0};
				extern sfx_t *S_FindName( const char *name );
				//
				static sfx_t *pSFX = NULL;
				const char sReservedSFXEntrynameForMP3[] = "reserved_for_mp3";	// ( strlen() < MAX_QPATH )

				if (pSFX == NULL)	// once only
				{
					pSFX = S_FindName(sReservedSFXEntrynameForMP3);	// always returns, else ERR_FATAL					
				}

				if (MP3_IsValid(sFilename,pbData, iSize, qbForceStereo))
				{
					wavinfo_t info;

					int iRawPCMDataSize = MP3_GetUnpackedSize(sFilename, pbData, iSize, qtrue, qbForceStereo);

					if (iRawPCMDataSize)	// should always be true, unless file is fucked, in which case, stop this conversion process
					{
						float fMaxVol = 128;	// any old default
						int iActualUnpackedSize = iRawPCMDataSize;	// default, override later if not doing music

						if (!qbForceStereo)	// no point for stereo files, which are for music and therefore no lip-sync
						{
							byte *pbUnpackBuffer = (byte *) Z_Malloc( iRawPCMDataSize+10, TAG_TEMP_WORKSPACE, qfalse );	// won't return if fails

							iActualUnpackedSize = MP3_UnpackRawPCM( sFilename, pbData, iSize, pbUnpackBuffer );
							if (iActualUnpackedSize != iRawPCMDataSize)
							{
								Com_Error(ERR_DROP, "******* Whoah! MP3 %s unpacked to %d bytes, but size calc said %d!\n",sFilename,iActualUnpackedSize,iRawPCMDataSize);
							}
						
							// fake up a WAV structure so I can use the other post-load sound code such as volume calc for lip-synching
							//
							MP3_FakeUpWAVInfo( sFilename, pbData, iSize, iActualUnpackedSize,
												// these params are all references...
												info.format, info.rate, info.width, info.channels, info.samples, info.dataofs
												);

							S_LoadSound_Finalize(&info, pSFX, pbUnpackBuffer);	// all this just for lipsynch. Oh well.

							fMaxVol = pSFX->fVolRange;

							// free sfx->data...
							//
							{
								#ifndef INT_MIN
								#define INT_MIN     (-2147483647 - 1) /* minimum (signed) int value */
								#endif
								//
								pSFX->iLastTimeUsed = INT_MIN;		// force this to be oldest sound file, therefore disposable...
								pSFX->bInMemory = qtrue;
								SND_FreeOldestSound();		// ... and do the disposal

								// now set our temp SFX struct back to default name so nothing else accidentally uses it...
								//
								strcpy(pSFX->sSoundName, sReservedSFXEntrynameForMP3);
								pSFX->bDefaultSound = qfalse;								
							}

//							Com_OPrintf("File: \"%s\"   MaxVol %f\n",sFilename,pSFX->fVolRange);

							// other stuff...
							//
							Z_Free(pbUnpackBuffer);
						}

						// well, time to update the file now...
						//
						fileHandle_t f = FS_FOpenFileWrite( sFilename );
						if (f)
						{
							// write the file back out, but omitting the tag if there was one...
							//
							int iWritten = FS_Write(pbData, iSize-(pTAG?sizeof(*pTAG):0), f);

							if (iWritten)
							{
								// make up a new tag if we didn't find one in the original file...
								//
								id3v1_1 TAG;
								if (!pTAG)
								{
									pTAG = &TAG;
									memset(&TAG,0,sizeof(TAG));
									strncpy(pTAG->id,"TAG",3);
								}

								strncpy(pTAG->title,	Filename_WithoutPath(Filename_WithoutExt(sFilename)), sizeof(pTAG->title));
								strncpy(pTAG->artist,	"Raven Software",						sizeof(pTAG->artist)	);
								strncpy(pTAG->year,		"2002",									sizeof(pTAG->year)		);
								strncpy(pTAG->comment,	va("%s %g",sKEY_MAXVOL,fMaxVol),		sizeof(pTAG->comment)	);
								strncpy(pTAG->album,	va("%s %d",sKEY_UNCOMP,iActualUnpackedSize),sizeof(pTAG->album)	);
								
								if (FS_Write( pTAG, sizeof(*pTAG), f ))	// NZ = success
								{
									iFilesUpdated++;
								}
								else
								{
									Com_Printf("*********** Failed write to file \"%s\"!\n",sFilename);
									iErrors++;
									strErrors += va("Failed to write: \"%s\"\n",sFilename);
								}
							}
							else
							{
								Com_Printf("*********** Failed write to file \"%s\"!\n",sFilename);
								iErrors++;
								strErrors += va("Failed to write: \"%s\"\n",sFilename);
							}
							FS_FCloseFile( f );
						}
						else
						{
							Com_Printf("*********** Failed to re-open for write \"%s\"!\n",sFilename);
							iErrors++;
							strErrors += va("Failed to re-open for write: \"%s\"\n",sFilename);
						}					
					}
					else
					{
						Com_Error(ERR_DROP, "******* This MP3 should be deleted: \"%s\"\n",sFilename);
					}
				}
				else
				{
					Com_Printf("*********** File was not a valid MP3!: \"%s\"\n",sFilename);
					iErrors++;
					strErrors += va("Not game-legal MP3 format: \"%s\"\n",sFilename);
				}
			}
			else
			{
				Com_Printf(" ( OK )\n");
			}

			FS_FreeFile( pbData );
		}
	}
	FS_FreeFileList( sysFiles );
	FS_FreeFileList( dirFiles );
}

// this console-function is for development purposes, and makes sure that sound/*.mp3 /s have tags in them
//	specifying stuff like their max volume (and uncompressed size) etc...
//
void S_MP3_CalcVols_f( void )
{
	char sStartDir[MAX_QPATH] = {"sound"};
	const char sUsage[] = "Usage: mp3_calcvols [-rescan] <startdir>\ne.g. mp3_calcvols sound/chars";

	if (Cmd_Argc() == 1 || Cmd_Argc()>4)	// 3 optional arguments
	{
		Com_Printf(sUsage);
		return;
	}

	S_StopAllSounds();


	qbForceRescan = qfalse;
	qbForceStereo = qfalse;
	iFilesFound		= 0;
	iFilesUpdated	= 0;
	iErrors			= 0;
	strErrors		= "";

	for (int i=1; i<Cmd_Argc(); i++)
	{
		if (Cmd_Argv(i)[0] == '-')
		{
			if (!Q_stricmp(Cmd_Argv(i),"-rescan"))
			{
				qbForceRescan = qtrue;
			}
			else
			if (!Q_stricmp(Cmd_Argv(i),"-stereo"))
			{
				qbForceStereo = qtrue;
			}
			else
			{
				// unknown switch...
				//
				Com_Printf(sUsage);
				return;
			}
			continue;
		}
		strcpy(sStartDir,Cmd_Argv(i));
	}

	Com_Printf(va("Starting Scan for Updates in Dir: %s\n",sStartDir));
	R_CheckMP3s( sStartDir );

	Com_Printf("\n%d files found/scanned, %d files updated      ( %d errors total)\n",iFilesFound,iFilesUpdated,iErrors);

	if (iErrors)
	{
		Com_Printf("\nBad Files:\n%s\n",strErrors.c_str());
	}
}




// adjust filename for foreign languages and WAV/MP3 issues. 
//
// returns qfalse if failed to load, else fills in *pData
//
extern	cvar_t	*com_buildScript;
extern	cvar_t* s_language;
static sboolean S_LoadSound_FileLoadAndNameAdjuster(char *psFilename, byte **pData, int *piSize, int iNameStrlen)
{
	char *psVoice = strstr(psFilename,"chars");
	if (psVoice)
	{
		// cache foreign voices...
		//		
		if (com_buildScript->integer)
		{
			fileHandle_t hFile;
			//German
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//French
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//Spanish
			strncpy(psVoice,"chr_e",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			strncpy(psVoice,"chars",5);	//put it back to chars
		}

		// account for foreign voices...
		//		

		if (s_language && stricmp("DEUTSCH",s_language->string)==0)
		{				
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
		}
		else if (s_language && stricmp("FRANCAIS",s_language->string)==0)
		{				
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
		}
		else if (s_language && stricmp("ESPANOL",s_language->string)==0)
		{				
			strncpy(psVoice,"chr_e",5);	// same number of letters as "chars"
		}
		else
		{
			psVoice = NULL;	// use this ptr as a flag as to whether or not we substituted with a foreign version
		}
#ifndef SND_MME
		return 0;
#endif
	}
#ifndef SND_MME
	else {
		psVoice = strstr(psFilename,"chr_d");
		if (psVoice) {
			strncpy(psVoice,"chars",5);
			return 1;
		} else {
			psVoice = strstr(psFilename,"chr_f");
			if (psVoice) {
				strncpy(psVoice,"chars",5);
				return 1;
			} else {
				psVoice = strstr(psFilename,"chr_f");
				if (psVoice) {
					strncpy(psVoice,"chars",5);
					return 1;
				}
			}
		}
		return 0; //has to reach that only if it's not "chars" related sound, 1 - replaced sound
	}
#endif

	*piSize = FS_ReadFile( psFilename, (void **)pData );	// try WAV
	if ( !*pData ) {
		psFilename[iNameStrlen-3] = 'm';
		psFilename[iNameStrlen-2] = 'p';
		psFilename[iNameStrlen-1] = '3';
		*piSize = FS_ReadFile( psFilename, (void **)pData );	// try MP3

		if ( !*pData ) 
		{
			//hmmm, not found, ok, maybe we were trying a foreign noise ("arghhhhh.mp3" that doesn't matter?) but it
			// was missing?   Can't tell really, since both types are now in sound/chars. Oh well, fall back to English for now...
			
			if (psVoice)	// were we trying to load foreign?
			{
				// yep, so fallback to re-try the english...
				//				
#ifndef FINAL_BUILD
				Com_Printf(S_COLOR_YELLOW "Foreign file missing: \"%s\"! (using English...)\n",psFilename);
#endif

				strncpy(psVoice,"chars",5);

				psFilename[iNameStrlen-3] = 'w';
				psFilename[iNameStrlen-2] = 'a';
				psFilename[iNameStrlen-1] = 'v';
				*piSize = FS_ReadFile( psFilename, (void **)pData );	// try English WAV
				if ( !*pData ) 
				{						
					psFilename[iNameStrlen-3] = 'm';
					psFilename[iNameStrlen-2] = 'p';
					psFilename[iNameStrlen-1] = '3';
					*piSize = FS_ReadFile( psFilename, (void **)pData );	// try English MP3
				}
			}

			if (!*pData)
			{
				return qfalse;	// sod it, give up...
			}
		}
	}

	return qtrue;
}

static sboolean S_SwitchLang(char *psFilename) {
	int iNameStrlen = strlen(psFilename);
	char *psVoice = strstr(psFilename,"chars");
	if (psVoice) {
		// cache foreign voices...
		//		
		if (com_buildScript->integer) {
			fileHandle_t hFile;
			//German
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile) {
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile) {
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//French
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile) {
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile) {
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//Spanish
			strncpy(psVoice,"chr_e",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile) {
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile) {
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			strncpy(psVoice,"chars",5);	//put it back to chars
		}

		// account for foreign voices...
		//		

		if (s_language && stricmp("DEUTSCH",s_language->string)==0) {				
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
		}
		else if (s_language && stricmp("FRANCAIS",s_language->string)==0) {				
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
		}
		else if (s_language && stricmp("ESPANOL",s_language->string)==0) {				
			strncpy(psVoice,"chr_e",5);	// same number of letters as "chars"
		}
		else {
			psVoice = NULL;	// use this ptr as a flag as to whether or not we substituted with a foreign version
		}
		return 0;
	}
	else {
		psVoice = strstr(psFilename,"chr_d");
		if (psVoice) {
			strncpy(psVoice,"chars",5);
			return 1;
		} else {
			psVoice = strstr(psFilename,"chr_f");
			if (psVoice) {
				strncpy(psVoice,"chars",5);
				return 1;
			} else {
				psVoice = strstr(psFilename,"chr_f");
				if (psVoice) {
					strncpy(psVoice,"chars",5);
					return 1;
				}
			}
		}
		return 0; //has to reach that only if it's not "chars" related sound, 1 - replaced sound
	}
}


// returns qtrue if this dir is allowed to keep loaded MP3s, else qfalse if they should be WAV'd instead...
//
// note that this is passed the original, un-language'd name
//
// (I was going to remove this, but on kejim_post I hit an assert because someone had got an ambient sound when the
//	perimter fence goes online that was an MP3, then it tried to get added as looping. Presumably it sounded ok or
//	they'd have noticed, but we therefore need to stop other levels using those. "sound/ambience" I can check for,
//	but doors etc could be anything. Sigh...)
//
static sboolean S_LoadSound_DirIsAllowedToKeepMP3s(const char *psFilename)
{
	static const char *psAllowedDirs[] = 
	{
		"sound/chars/",
//		"sound/chr_d/"	// no need for this now, or any other language, since we'll always compare against english
	};

	int i;
	for (i=0; i< (sizeof(psAllowedDirs) / sizeof(psAllowedDirs[0])); i++)
	{
		if (strnicmp(psFilename, psAllowedDirs[i], strlen(psAllowedDirs[i]))==0)
			return qtrue;	// found a dir that's allowed to keep MP3s
	}

	return qfalse;
}



/*
==============
S_LoadSound

The filename may be different than sfx->name in the case
of a forced fallback of a player specific sound	(or of a wav/mp3 substitution now -Ste)
==============
*/
qboolean gbInsideLoadSound = qfalse;
static sboolean S_LoadSound_Actual( sfx_t *sfx )
{
	byte	*data;
	short	*samples;
	wavinfo_t	info;
	int		size;
	char	*psExt;
	char	sLoadName[MAX_QPATH];
	ALuint  Buffer;
	
	int		len = strlen(sfx->sSoundName);
	if (len<5)
	{
		return qfalse;
	}

	// player specific sounds are never directly loaded...
	//
	if ( sfx->sSoundName[0] == '*') {
		return qfalse;
	}
	// make up a local filename to try wav/mp3 substitutes...
	//	
	Q_strncpyz(sLoadName, sfx->sSoundName, sizeof(sLoadName));	
#ifdef _WIN32
	strlwr( sLoadName );
#else
	string s = sLoadName;
	transform(s.begin(), s.end(), s.begin(), ::tolower);
#endif
	//
	// Ensure name has an extension (which it must have, but you never know), and get ptr to it...
	//
	psExt = &sLoadName[strlen(sLoadName)-4];
	if (*psExt != '.')
	{
		//Com_Printf( "WARNING: soundname '%s' does not have 3-letter extension\n",sLoadName);
		COM_DefaultExtension(sLoadName,sizeof(sLoadName),".wav");	// so psExt below is always valid
		psExt = &sLoadName[strlen(sLoadName)-4];
		len = strlen(sLoadName);
	}

	if (!S_LoadSound_FileLoadAndNameAdjuster(sLoadName, &data, &size, len))
	{
		return qfalse;
	}

	SND_TouchSFX(sfx);

//=========
	if (strnicmp(psExt,".mp3",4)==0)
	{
		// load MP3 file instead...
		//		
		if (MP3_IsValid(sLoadName,data, size, qfalse))
		{
			int iRawPCMDataSize = MP3_GetUnpackedSize(sLoadName,data,size,qfalse,qfalse);
			
			if (S_LoadSound_DirIsAllowedToKeepMP3s(sfx->sSoundName)	// NOT sLoadName, this uses original un-languaged name
				&&
				MP3Stream_InitFromFile(sfx, data, size, sLoadName, iRawPCMDataSize + 2304 /* + 1 MP3 frame size, jic */,qfalse)				
				)
			{
//				Com_DPrintf("(Keeping file \"%s\" as MP3)\n",sLoadName);

				if (s_UseOpenAL)
				{
					// Create space for lipsync data (4 lip sync values per streaming AL buffer)
					if ((strstr(sfx->sSoundName, "chars")) || (strstr(sfx->sSoundName, "CHARS")))
						sfx->lipSyncData = (char *)Z_Malloc(16, TAG_SND_RAWDATA, qfalse);
					else
						sfx->lipSyncData = NULL;
				}
			}
			else
			{
				// small file, not worth keeping as MP3 since it would increase in size (with MP3 header etc)...
				//
				Com_DPrintf("S_LoadSound: Unpacking MP3 file(%i) \"%s\" to wav(%i).\n",size,sLoadName,iRawPCMDataSize);
				//
				// unpack and convert into WAV...
				//
				{
					byte *pbUnpackBuffer = (byte *) Z_Malloc( iRawPCMDataSize+10 +2304 /* <g> */, TAG_TEMP_WORKSPACE, qfalse );	// won't return if fails
					
					{
						int iResultBytes = MP3_UnpackRawPCM( sLoadName, data, size, pbUnpackBuffer, qfalse );

						if (iResultBytes!= iRawPCMDataSize){
							Com_Printf(S_COLOR_YELLOW"**** MP3 %s final unpack size %d different to previous value %d\n",sLoadName,iResultBytes,iRawPCMDataSize);
							//assert (iResultBytes == iRawPCMDataSize);
						}


						// fake up a WAV structure so I can use the other post-load sound code such as volume calc for lip-synching
						//
						// (this is a bit crap really, but it lets me drop through into existing code)...
						//
						MP3_FakeUpWAVInfo( sLoadName, data, size, iResultBytes,
											// these params are all references...
											info.format, info.rate, info.width, info.channels, info.samples, info.dataofs,
											qfalse
										);

						S_LoadSound_Finalize(&info,sfx,pbUnpackBuffer);

						// Open AL
						if (s_UseOpenAL)
						{
							if ((strstr(sfx->sSoundName, "chars")) || (strstr(sfx->sSoundName, "CHARS")))
							{
								sfx->lipSyncData = (char *)Z_Malloc((sfx->iSoundLengthInSamples / 1000) + 1, TAG_SND_RAWDATA, qfalse);
								S_PreProcessLipSync(sfx);
							}
							else
								sfx->lipSyncData = NULL;

							// Clear Open AL Error state
							alGetError();

							// Generate AL Buffer
							alGenBuffers(1, &Buffer);
							if (alGetError() == AL_NO_ERROR)
							{
								// Copy audio data to AL Buffer
								alBufferData(Buffer, AL_FORMAT_MONO16, sfx->pSoundData, sfx->iSoundLengthInSamples*2, 22050);
								if (alGetError() == AL_NO_ERROR)
								{
									sfx->Buffer = Buffer;
									Z_Free(sfx->pSoundData);
									sfx->pSoundData = NULL;
								}
							}
						}

						Z_Free(pbUnpackBuffer);
					}
				}
			}
		}
		else
		{
			// MP3_IsValid() will already have printed any errors via Com_Printf at this point...
			//			
			FS_FreeFile (data);
			return qfalse;
		}
	}
	else
	{
		// loading a WAV, presumably...

//=========

		info = GetWavinfo( sLoadName, data, size );
		if ( info.channels != 1 ) {
			Com_Printf ("%s is a stereo wav file\n", sLoadName);
			FS_FreeFile (data);
			return qfalse;
		}

/*		if ( info.width == 1 ) {
			Com_Printf(S_COLOR_YELLOW "WARNING: %s is a 8 bit wav file\n", sLoadName);
		}

		if ( info.rate != 22050 ) {
			Com_Printf(S_COLOR_YELLOW "WARNING: %s is not a 22kHz wav file\n", sLoadName);
		}
*/
		samples = (short *)Z_Malloc(info.samples * sizeof(short) * 2, TAG_TEMP_WORKSPACE, qfalse);

		sfx->eSoundCompressionMethod = ct_16;
		sfx->iSoundLengthInSamples	 = info.samples;
		sfx->pSoundData = NULL;
		ResampleSfx( sfx, info.rate, info.width, data + info.dataofs );

		// Open AL
		if (s_UseOpenAL)
		{
			if ((strstr(sfx->sSoundName, "chars")) || (strstr(sfx->sSoundName, "CHARS")))
			{
				sfx->lipSyncData = (char *)Z_Malloc((sfx->iSoundLengthInSamples / 1000) + 1, TAG_SND_RAWDATA, qfalse);
				S_PreProcessLipSync(sfx);
			}
			else
				sfx->lipSyncData = NULL;

			// Clear Open AL Error State
			alGetError();

			// Generate AL Buffer
			alGenBuffers(1, &Buffer);
			if (alGetError() == AL_NO_ERROR)
			{
				// Copy audio data to AL Buffer
				alBufferData(Buffer, AL_FORMAT_MONO16, sfx->pSoundData, sfx->iSoundLengthInSamples*2, 22050);
				if (alGetError() == AL_NO_ERROR)
				{
					// Store AL Buffer in sfx struct, and release sample data
					sfx->Buffer = Buffer;
					Z_Free(sfx->pSoundData);
					sfx->pSoundData = NULL;
				}
			}
		}

		Z_Free(samples);
	}
	
	FS_FreeFile( data );

	return qtrue;
}


// wrapper function for above so I can guarantee that we don't attempt any audio-dumping during this call because
//	of a z_malloc() fail recovery...
//
sboolean S_LoadSound( sfx_t *sfx )
{
	gbInsideLoadSound = qtrue;	// !!!!!!!!!!!!!
		
		sboolean bReturn = S_LoadSound_Actual( sfx );

	gbInsideLoadSound = qfalse;	// !!!!!!!!!!!!!

	return bReturn;
}


/*
	Precalculate the lipsync values for the whole sample
*/
void S_PreProcessLipSync(sfx_t *sfx)
{
	int i, j;
	int sample;
	int sampleTotal = 0;

	j = 0;
	for (i = 0; i < sfx->iSoundLengthInSamples; i += 100)
	{
		sample = LittleShort(sfx->pSoundData[i]);

		sample = sample >> 8;
		sampleTotal += sample * sample;
		if (((i + 100) % 1000) == 0)
		{
			sampleTotal /= 10;

			if (sampleTotal < sfx->fVolRange *  s_lip_threshold_1->value)
			{
				// tell the scripts that are relying on this that we are still going, but actually silent right now.
				sample = -1;
			}
			else if (sampleTotal < sfx->fVolRange * s_lip_threshold_2->value)
				sample = 1;
			else if (sampleTotal < sfx->fVolRange * s_lip_threshold_3->value)
				sample = 2;
			else if (sampleTotal < sfx->fVolRange * s_lip_threshold_4->value)
				sample = 3;
			else
				sample = 4;
			
			sfx->lipSyncData[j] = sample;
			j++;

			sampleTotal = 0;
		}
	}

	if ((i % 1000) == 0)
		return;

	i -= 100;
	i = i % 1000;
	i = i / 100;
	// Process last < 1000 samples
	if (i != 0)
		sampleTotal /= i;
	else
		sampleTotal = 0;

	if (sampleTotal < sfx->fVolRange * s_lip_threshold_1->value)
	{
		// tell the scripts that are relying on this that we are still going, but actually silent right now.
		sample = -1;
	}
	else if (sampleTotal < sfx->fVolRange * s_lip_threshold_2->value)
		sample = 1;
	else if (sampleTotal < sfx->fVolRange * s_lip_threshold_3->value)
		sample = 2;
	else if (sampleTotal < sfx->fVolRange * s_lip_threshold_4->value)
		sample = 3;
	else
		sample = 4;
			
	sfx->lipSyncData[j] = sample;
}




//-----------------------------------//
//	     MME SOUND FILE SYSTEM		 //
//-----------------------------------//

#define	WAV_FORMAT_PCM		1

typedef struct {
	int				channels;
	int				bits;
	int				dataStart;
} wavOpen_t;

static unsigned short readLittleShort(byte *p) {
	unsigned short val;
	val = p[0];
	val += p[1] << 8;
	return val;
}

static unsigned int readLittleLong(byte *p) {
	unsigned int val;
	val = p[0];
	val += p[1] << 8;
	val += p[2] << 16;
	val += p[3] << 24;
	return val;
}

static openSound_t *S_StreamOpen( const char *fileName, int dataSize ) {
	fileHandle_t fileHandle = 0;
	int fileSize = 0;
	openSound_t *open;

	fileSize = FS_FOpenFileRead( fileName, &fileHandle, qtrue );
	if ( fileSize <= 0 || fileHandle <= 0)
		return 0;
	open = (openSound_t *)Z_Malloc( sizeof( openSound_t ) + dataSize, TAG_SND_RAWDATA );	//???
	memset( open, 0, sizeof( openSound_t ) + dataSize);
	open->fileSize = fileSize;
	open->fileHandle = fileHandle;
	return open;
}

static int S_StreamRead( openSound_t *open, int size, void *data ) {
	char *dataWrite = (char *)data;
	int done;
	if (size + open->filePos > open->fileSize)
		size = (open->fileSize - open->filePos);
	if (size <= 0)
		return 0;

	done = 0;
	while( 1  ) {
		if (!open->bufUsed ) {
			int toRead; 
			if ( size >= sizeof( open->buf )) {
				FS_Read( dataWrite + done, size, open->fileHandle );
				open->filePos += size;
				done += size;
				return done;
			}
			toRead = open->fileSize - open->filePos;
			if (toRead <= 0) {
				//wtf error
				return done;
			}
			if (toRead > sizeof( open->buf ))
				toRead = sizeof( open->buf );
			open->bufUsed = FS_Read( open->buf, toRead, open->fileHandle );
			if (open->bufUsed <= 0) {
				open->bufUsed = 0;
				return done;
			}
			open->bufPos = 0;
		} 
		if ( size > open->bufUsed ) {
			Com_Memcpy( dataWrite + done, open->buf + open->bufPos, open->bufUsed );
			open->filePos += open->bufUsed;
			size -= open->bufUsed;
			done += open->bufUsed;
			open->bufUsed = 0;
			continue;
		}
		Com_Memcpy( dataWrite + done, open->buf + open->bufPos, size );
		open->bufPos += size;
		open->bufUsed -= size;
		open->filePos += size;
		done += size;
		break;
	}
	return done;
}

static int S_StreamSeek( openSound_t *open, int pos ) {
	if ( pos < 0)
		pos = 0;
	else if (pos >= open->fileSize)
		pos = open->fileSize;
	open->bufUsed = 0;
	open->filePos = pos;
	return FS_Seek( open->fileHandle, pos, FS_SEEK_SET );
}

static void S_StreamClose( openSound_t *open ) {
	if ( open->fileHandle > 0) {
		FS_FCloseFile( open->fileHandle );
		open->fileHandle = 0;
	}
}

static void S_WavClose( openSound_t *open ) {
	if (!open )
		return;
	S_StreamClose( open );
	return;
}

static int S_WavSeek( struct openSound_s *open, int samples ) {
	wavOpen_t *wav;
	int wantPos;

	if (!open || !open->data )
		return 0;
	wav = (wavOpen_t *)(open->data);
	if (wav->bits == 8)
		wantPos = samples + wav->dataStart;
	else if (wav->bits == 16)
		wantPos = samples * 2 + wav->dataStart;
	S_StreamSeek( open, wantPos );
	return wantPos;
}

static int S_WavRead( openSound_t *open, qboolean stereo, int size, short *data ) {
	short buf[2048];
	int bufSize, i, done;
	wavOpen_t *wav;
	
	if (!open || !open->data )
		return 0;

	wav = (wavOpen_t *)(open->data);
	if ( wav->bits == 16 ) {
		size *= 2;
		bufSize = sizeof( buf );
	} else {
		/* Use half the bufsize for 8bits so we can first convert the buf */
		bufSize = sizeof( buf ) / 2;
	}
	if ( wav->channels == 2)
		size *= 2;

	done = 0;
	while ( size > 0) {
		int read = size > bufSize ? bufSize : size;
		read = S_StreamRead( open, read, buf );
		if (!read)
			break;
		size -= read;
		/* Convert buffer from to 8u to 16s in reverse */
		if (wav->bits == 8) {
			for (i = read - 1;i >= 0 ;i--) 
				buf[i] = (((char *)buf)[i] ^ 0x80) << 8;
			read *= 2;
		}
		/* Stereo wav input */
		if ( wav->channels == 2) {
			/* Stereo output, just copy */
			if (stereo) {
				Com_Memcpy( &data[done * 2], buf, read );
				done += read / 4;
			/* Mono output sum up and /2 for output */
			} else {
				read /= 4;
				for ( i = 0;i < read;i++)  {
					data[done++] = (buf[i * 2 + 0] + buf[i*2 + 1]) >> 1;
				}
			}
		/* Mono wav input */
		} else {
			/* Stereo output, both channels the same */
			if (stereo) {
				read /= 2;
				for ( i = 0;i < read;i++)  {
					data[done*2 + 0] = data[done*2 + 1] = buf[i];
					done++;
				}
			/* Mono output just copy */
			} else {
				Com_Memcpy( &data[done], buf, read );
				done += read / 2;
			}
		}
	}

	return done;
}

static int S_WavFindChunk( openSound_t *open, const char *chunk ) {
	char data[8];
	int readSize;
	while (1) {
		readSize = S_StreamRead( open, 8, data );
		if (readSize != 8) {
			return 0;
		}
		readSize = readLittleLong( (byte *)data + 4 ) ;		
		if (!memcmp(data + 0x00, chunk, 4))
			break;
		S_StreamSeek( open, open->filePos + readSize );
	}
	return readSize;
}

static openSound_t * S_WavOpen( const char *fileName ) {
	openSound_t *open;
	wavOpen_t *wav;
	byte buf[16];
	int	readSize;
	int format;
	int riffSize, fmtSize, dataSize;

	// load it in
	open = S_StreamOpen( fileName, sizeof( wavOpen_t ));
	if (!open) {
		if (!doNotYell)
			Com_Printf("WavOpen:File %s failed to open\n", fileName);
		return 0;
	}
	open->read = S_WavRead;
	open->close = S_WavClose;
	open->seek = S_WavSeek;
	wav = (wavOpen_t *)open->data;
	riffSize = S_WavFindChunk( open, "RIFF" );
	if (!riffSize ) {
		Com_Printf("WavOpen:File %s failed to find RIFF chunk\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	readSize = S_StreamRead( open, 4, buf );
	if (!readSize || memcmp( buf, "WAVE", 4)) {
		Com_Printf("WavOpen:File %s failed to find RIFF WAVE type\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	fmtSize = S_WavFindChunk( open, "fmt " );
	if (!fmtSize || fmtSize < 16 ) {
		Com_Printf("WavOpen:File %s failed to find fmt chunk\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	readSize = S_StreamRead( open, 16, buf );
	if (readSize < 16) {
		Com_Printf("WavOpen:unexpected end of file while reading fmt chunk\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	format = readLittleShort( buf + 0x00 );
	wav->channels = readLittleShort( buf + 0x2 );
	open->rate = readLittleLong( buf + 0x4 );
	wav->bits = readLittleShort( buf + 0xe );
	if (format != 1) {
		Com_Printf("WavOpen:Microsoft PCM format only, %s\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	if ((wav->channels != 1) && (wav->channels != 2)) {
		Com_Printf("WavOpen:can't open file with %d channels, %s\n", wav->channels, fileName);
		S_SoundClose( open );
		return 0;
	}
	/* Skip the remainder of the format if any */
	fmtSize -= readSize;
	if (fmtSize)
		S_StreamSeek( open, open->filePos + fmtSize );
	/* Scan the cunks till you find the "data" chunk and find sample count */
	/* Finish reading the format if needed */
	dataSize = S_WavFindChunk( open, "data" );
	if (!dataSize) {
		Com_Printf("WavOpen:failed %s no data\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	/* Try to find the data chunk */
	/* Read the chunksize of the data chunk */
	open->totalSamples = dataSize / (wav->bits / 8);
	wav->dataStart = open->filePos;
	return open;
}

#define HAVE_LIBMAD

//=============================================================================

#ifdef HAVE_LIBMAD 
#include <mad.h>
//#pragma comment (lib, "libmad.lib")
//#pragma comment (lib, "libmadd.lib")
#define MP3_SEEKINTERVAL 16 
#define MP3_SEEKMAX 4096

typedef struct {
	struct	mad_stream stream;
	struct	mad_frame frame;
	struct	mad_synth synth;
	char	buf[8 * 1024];
	int		pcmLeft;
	long	seekStart[MP3_SEEKMAX];
	int		seekCount;
	int		frameCount, frameSamples;
} mp3Open_t;


static int S_MP3Fill( openSound_t *open ) {
	int bufLeft, readSize;
	unsigned char *readStart;
	mp3Open_t *mp3;

	if (!open || !open->data)
		return 0;

	mp3 = (mp3Open_t *) open->data;

	if (mp3->stream.next_frame != NULL) {
		bufLeft = mp3->stream.bufend - mp3->stream.next_frame;
		memmove (mp3->buf, mp3->stream.next_frame, bufLeft);
		readStart = (unsigned char *)mp3->buf + bufLeft;
		readSize = sizeof(mp3->buf) - bufLeft;
	} else {
		readStart = (unsigned char *)mp3->buf;
		readSize = sizeof(mp3->buf);
		bufLeft = 0;
	}

	readSize = S_StreamRead( open, readSize, readStart );
	if (readSize <= 0)
		return 0;
	mad_stream_buffer(&mp3->stream, (unsigned char *)mp3->buf, readSize + bufLeft);
	mp3->stream.error = (mad_error)0;
	return readSize;
}

static inline int S_MadSample (mad_fixed_t sample) {
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static int S_MP3Read( openSound_t *open, qboolean stereo, int size, short *data ) {
	mp3Open_t *mp3;
	int done;

	if (!open || !open->data )
		return 0;
	mp3 = (mp3Open_t *)(open->data);
	done = 0;
	while ( size > 0) {
		/* Have we got any pcm data waiting */
		if (mp3->pcmLeft) {
			int i, todo;
			if ( mp3->pcmLeft <= size ) {
				todo = mp3->pcmLeft;
			} else {
				todo = size;
			}
			if ( data ) {
				const struct mad_pcm *pcm = &mp3->synth.pcm;
				if ( pcm->channels == 1 ) {
					const mad_fixed_t *left = &pcm->samples[0][ pcm->length - mp3->pcmLeft ];
					if ( stereo ) {
						for (i = 0;i<todo;i++) {
							data[0] = data[1] = S_MadSample( left[i]  );
							data += 2;
						}
					} else{
						for (i = 0;i<todo;i++) {
							*data++ = S_MadSample( left[i]  );
						}
					}
				} else if ( pcm->channels == 2 ) {
					const mad_fixed_t *left =  &pcm->samples[0][ pcm->length - mp3->pcmLeft ];
					const mad_fixed_t *right = &pcm->samples[1][ pcm->length - mp3->pcmLeft ];
					if ( stereo ) {
						for (i = 0;i<todo;i++) {
							data[0] = S_MadSample( left[i] );
							data[1] = S_MadSample( right[i] );
							data+=2;
						}
					} else {
						for (i = 0;i<todo;i++) {
							int addSample;
							addSample = S_MadSample( left[i] );
							addSample += S_MadSample( right[i] );
							*data++ = addSample >> 1;
						}
					}
				} else {
					return done;
				}
			} 
			done += todo;
			size -= todo;
			mp3->pcmLeft -= todo;
			if ( mp3->pcmLeft )
				return done;
		}
		/* is there still pcm data left then we must have reached size */
		if (mp3->stream.next_frame == NULL || mp3->stream.error == MAD_ERROR_BUFLEN) {
			if (!S_MP3Fill( open ))
				return done;
		}
		if (mad_frame_decode (&mp3->frame, &mp3->stream)) {
			if (MAD_RECOVERABLE(mp3->stream.error)) {
				continue;
			} else if (mp3->stream.error == MAD_ERROR_BUFLEN) {
				continue;
			} else {
				return done;
			}
		}
		/* Sound parameters. */
		if (mp3->frame.header.samplerate != open->rate ) {
			Com_Printf( "mp3read, samplerate change?!?!?!\n");
			return done;
		}
		mad_synth_frame (&mp3->synth, &mp3->frame);
		mad_stream_sync (&mp3->stream);
		mp3->pcmLeft = mp3->synth.pcm.length;
	}
	return done;
}

static int S_MP3Seek( struct openSound_s *open, int samples ) {
	mp3Open_t *mp3;
	int index, frame, seek;

	if (!open || !open->data )
		return 0;
	mp3 = (mp3Open_t *)(open->data);
	frame = samples / mp3->frameSamples;
	if ( frame >= mp3->frameCount) 
		frame = mp3->frameCount - 1;
	index = frame / MP3_SEEKINTERVAL;
	frame %= MP3_SEEKINTERVAL;
	if ( index >= mp3->seekCount) 
		index = mp3->seekCount - 1;
	seek = index * MP3_SEEKINTERVAL * mp3->frameSamples;
	samples -= seek;
	S_StreamSeek( open, mp3->seekStart[ index ] );

	//mad_frame_mute (&mp3->frame);
	mad_frame_init( &mp3->frame );
	//mad_synth_mute (&mp3->synth);
	mad_synth_init( &mp3->synth );
	mp3->stream.sync = 0;
	mp3->stream.next_frame = NULL;
	mp3->pcmLeft = 0;
	seek += S_MP3Read( open, qfalse, samples, 0 );
	return seek;
}

static void S_MP3Close( openSound_t *open) {
	mp3Open_t *mp3;
	if (!open || !open->data )
		return;
	mp3 = (mp3Open_t *)(open->data);
	mad_stream_finish (&mp3->stream);
	mad_frame_finish (&mp3->frame);
	mad_synth_finish (&mp3->synth);
}

static openSound_t *S_MP3Open( const char *fileName ) {
	mp3Open_t *mp3;
	openSound_t *open;
	struct mad_header header;
	mad_timer_t duration;

	open = S_StreamOpen( fileName, sizeof( mp3Open_t ) );
	if (!open) {
		if (!doNotYell)
			Com_Printf("MP3Open:File %s failed to open\n", fileName);
		return 0;
	}
	mp3 = (mp3Open_t *)open->data;
	mad_header_init (&header);
	mad_stream_init (&mp3->stream);
	mad_frame_init (&mp3->frame);
	mad_synth_init (&mp3->synth);
	mad_stream_options (&mp3->stream, MAD_OPTION_IGNORECRC);
	open->read = S_MP3Read;
	open->seek = S_MP3Seek;
	open->close = S_MP3Close;
	/* Determine file length in samples */
	duration = mad_timer_zero;
	while (1) {
		if (mp3->stream.buffer == NULL || mp3->stream.error == MAD_ERROR_BUFLEN) {
			if (!S_MP3Fill( open ))
				break;
		}
		if (mad_header_decode(&header, &mp3->stream) == -1) {
			if (MAD_RECOVERABLE(mp3->stream.error))
				continue;
			else if (mp3->stream.error == MAD_ERROR_BUFLEN)
				continue;
			else {
				/* Another error, let's just stop! */
				break;
			}
		}
		mad_timer_add (&duration, header.duration);
		/* Should probably just count the amount of frames and do a multiply */
		if ( 0 || mp3->frameCount % MP3_SEEKINTERVAL == 0 ) {
			if (mp3->seekCount < MP3_SEEKMAX) {
				int pos = open->filePos;
				if ( mp3->stream.this_frame ) {
					pos -= mp3->stream.bufend - mp3->stream.this_frame;
				}
				mp3->seekStart[mp3->seekCount++] = pos;
			}
		}
		mp3->frameCount++;
	}
	/* Reset to the beginning and finalize setting of values */
	S_StreamSeek( open, 0 );
	mp3->frameSamples = 32 * MAD_NSBSAMPLES( &header );
	open->totalSamples = mp3->frameSamples * mp3->frameCount;
	open->rate = header.samplerate;
	return open;
}

#endif

openSound_t *S_SoundOpen( char *fileName ) {
	const char *fileExt;
	openSound_t *open;
	qboolean wasHere = qfalse;

#ifdef FINAL_BUILD
	doNotYell = qtrue;
#else
	doNotYell = qfalse;
#endif

	if (!fileName || !fileName[0]) {
		Com_Printf("SoundOpen:Filename is empty\n" );
		return 0;
	}

	/* switch language: /chars/ -> /chr_f/, /chr_d/, /chr_e/ */
	if (Q_stricmp(s_language->string, "english"))
		S_SwitchLang(fileName);

	/* sound/mp_generic_female/sound -> sound/chars/mp_generic_female/misc/sound */
	char *match = strstr( fileName, "sound/mp_generic_female" );
	if ( match ) {
		char out[MAX_QPATH];
		Q_strncpyz( out, fileName, MAX_QPATH );
		char *pos = strstr( match, "le/" );
		pos = strchr( pos, '/' );
		Q_strncpyz( out, "sound/chars/mp_generic_female/misc", MAX_QPATH );
		Q_strcat( out, sizeof( out ), pos );
		fileName = out;
	} else {
	/* or sound/mp_generic_male/sound -> sound/chars/mp_generic_male/misc/sound */
		match = strstr( fileName, "sound/mp_generic_male" );
		if ( match ) {
			char out[MAX_QPATH];
			Q_strncpyz( out, fileName, MAX_QPATH );
			char *pos = strstr( match, "le/" );
			pos = strchr( pos, '/' );
			Q_strncpyz( out, "sound/chars/mp_generic_male/misc", MAX_QPATH );
			Q_strcat( out, sizeof( out ), pos );
			fileName = out;
		}
	}

tryAgainThisSound:
	fileExt = Q_strrchr( fileName, '.' );
	if (!fileExt) {
		char *blah = fileName;
#ifdef HAVE_LIBMAD
		if (!doNotYell)
			Com_Printf("File %s has no extension, trying to use .mp3\n", fileName );
		strcat( blah, ".mp3" );
		open = S_MP3Open( blah );
		if (open) {
			return open;
		}
		else
#endif
		{
			if (!doNotYell)
				Com_Printf("File %s has no extension or .mp3 did not work, trying to use .wav\n", fileName );
			char *p = strchr( blah, '.' ); /* find first '.' */
			if ( p != NULL ) {/* found '.' */
				Q_strncpyz(p, ".wav", sizeof (blah) + 1); /* change ext */
			} else {
				strcat( blah, ".wav" );
			}
			open = S_WavOpen( blah );
			if (open) {
				return open;
			} else if (Q_stricmp(s_language->string, "english")) {
				if (S_SwitchLang(fileName)) {
					if (!doNotYell)
						Com_Printf("File %s doesn't exist in %s, retrying with english\n", fileName, s_language->string );
					goto tryAgainThisSound;
				}
			}
		}
		Com_Printf("SoundOpen:File %s does not exist\n", fileName );
		return 0;
	}	
	if (!Q_stricmp( fileExt, ".wav")) {
		open = S_WavOpen( fileName );
		if (open) {
			return open;
#ifdef HAVE_LIBMAD
		} else {
			//ZTM's super replacer
			char *blah = fileName; /* need to set blah */
			char *p = strchr( blah, '.' ); /* find first '.' */
			if ( p != NULL ) {/* found '.' */
				Q_strncpyz(p, ".mp3", sizeof (blah) + 1); /* change ext */
			} else { //should never happen
				strcat( blah, ".mp3" ); /* has no ext, so just add it */
			}
			open = S_MP3Open( blah );
			if (open) {
				return open;
			} else if (Q_stricmp(s_language->string, "english") && !wasHere) {
				if (S_SwitchLang(fileName)) {
					wasHere = qtrue;
					if (!doNotYell)
						Com_Printf("File %s doesn't exist in %s, retrying with english\n", fileName, s_language->string );
					goto tryAgainThisSound;
				}
			}
#endif
		}
#ifdef HAVE_LIBMAD
	} else if (!Q_stricmp( fileExt, ".mp3")) {
		open = S_MP3Open( fileName );
		if (open) {
			return open;
		} else {
			//ZTM's super replacer
			char *blah = fileName; /* need to set blah */
			char *p = strchr( blah, '.' ); /* find first '.' */
			if ( p != NULL ) {/* found '.' */
				Q_strncpyz(p, ".wav", sizeof (blah) + 1); /* change ext */
			} else { //should never happen
				strcat( blah, ".wav" ); /* has no ext, so just add it */
			}
			open = S_WavOpen( blah );
			if (open) {
				return open;
			} else if (Q_stricmp(s_language->string, "english") && !wasHere) {
				if (S_SwitchLang(fileName)) {
					wasHere = qtrue;
					if (!doNotYell)
						Com_Printf("File %s doesn't exist in %s, retrying with english\n", fileName, s_language->string );
					goto tryAgainThisSound;
				}
			}
		}
#endif
	} else {
		Com_Printf("SoundOpen:File %s has unknown extension %s\n", fileName, fileExt );
		return 0;
	}
}

int S_SoundRead( openSound_t *open, qboolean stereo, int samples, short *data ){
	int read ;
	if (!open)
		return 0;
	if ( samples + open->doneSamples > open->totalSamples )
		samples = open->totalSamples - open->doneSamples;
	if ( samples <= 0)
		return 0;
	read = open->read( open, stereo, samples, data );
    open->doneSamples += read;
	return read;
}

int S_SoundSeek( openSound_t *open, int samples ) {
	if ( samples > open->totalSamples)
		samples = open->totalSamples;
	if ( samples < 0)
		samples = 0;
	open->doneSamples = open->seek( open, samples );
	return open->doneSamples;
}

void S_SoundClose( openSound_t *open ) {
	open->close( open );
	S_StreamClose( open );
	Z_Free( open );
}

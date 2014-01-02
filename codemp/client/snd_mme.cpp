// Copyright (C) 2005 Eugene Bujak.
//
// snd_mme.c -- Movie Maker's Edition sound routines

#include "snd_local.h"
#include "snd_mix.h"

#define MME_SNDCHANNELS 128
#define MME_LOOPCHANNELS 128

#define MME_SAMPLERATE	44100 //ja is full of 44kz mp3

extern	cvar_t	*mme_saveWav;

typedef struct {
	char			baseName[MAX_OSPATH];
	fileHandle_t	fileHandle;
	long			fileSize;
	float			deltaSamples, sampleRate;
	mixLoop_t		loops[MME_LOOPCHANNELS];
	mixChannel_t	channels[MME_SNDCHANNELS];
	qboolean		gotFrame;
} mmeWav_t;

static mmeWav_t mmeSound;

/*
=================================================================================

WAV FILE HANDLING FUNCTIONS

=================================================================================
*/

#define WAV_HEADERSIZE 44
/*
===================
S_MME_PrepareWavHeader

Fill in wav header so that we can write sound after the header
===================
*/
static void S_MMEFillWavHeader(void* buffer, long fileSize, int sampleRate) {
	((unsigned long*)buffer)[0] = 0x46464952;	// "RIFF"
	((unsigned long*)buffer)[1] = fileSize-8;		// WAVE chunk length
	((unsigned long*)buffer)[2] = 0x45564157;	// "WAVE"

	((unsigned long*)buffer)[3] = 0x20746D66;	// "fmt "
	((unsigned long*)buffer)[4] = 16;			// fmt chunk size
	((unsigned short*)buffer)[(5*2)] = 1;		// audio format. 1 - PCM uncompressed
	((unsigned short*)buffer)[(5*2)+1] = 2;		// number of channels
	((unsigned long*)buffer)[6] = sampleRate;	// sample rate
	((unsigned long*)buffer)[7] = sampleRate * 2 * 2;	// byte rate
	((unsigned short*)buffer)[(8*2)] = 2 * 2;	// block align
	((unsigned short*)buffer)[(8*2)+1] = 16;	// sample bits

	((unsigned long*)buffer)[9] = 0x61746164;	// "data"
	((unsigned long*)buffer)[10] = fileSize-44;	// data chunk length
}

void S_MMEWavClose(void) {
	byte header[WAV_HEADERSIZE];

	if (!mmeSound.fileHandle)
		return;

	S_MMEFillWavHeader( header, mmeSound.fileSize, MME_SAMPLERATE );
	FS_Seek( mmeSound.fileHandle, 0, FS_SEEK_SET );
	FS_Write( header, sizeof(header), mmeSound.fileHandle );
	FS_FCloseFile( mmeSound.fileHandle );
	Com_Memset( &mmeSound, 0, sizeof(mmeSound) );
}

/*
===================
S_MME_Update

Called from CL_Frame() in cl_main.c when shooting avidemo
===================
*/
#define MAXUPDATE 4096
void S_MMEUpdate( float scale ) {
	int count, speed;
	int mixTemp[MAXUPDATE*2];
	short mixClip[MAXUPDATE*2];

	if (!mmeSound.fileHandle)
		return;
	if (!mmeSound.gotFrame) {
		S_MMEWavClose();
		return;
	}
	count = (int)mmeSound.deltaSamples;
	if (!count)
		return;
	mmeSound.deltaSamples -= count;
	if (count > MAXUPDATE)
		count = MAXUPDATE;

	speed = (scale * (MIX_SPEED << MIX_SHIFT)) / MME_SAMPLERATE;
	if (speed < 0 || (speed == 0 && scale) )
		speed = 1;

	Com_Memset( mixTemp, 0, sizeof(int) * count * 2);
	if ( speed > 0 ) {
		S_MixChannels( mmeSound.channels, MME_SNDCHANNELS, speed, count, mixTemp );
		S_MixLoops( mmeSound.loops, MME_LOOPCHANNELS, speed, count, mixTemp );
	}	
	S_MixClipOutput( count, mixTemp, mixClip, 0, MAXUPDATE - 1 );
	FS_Write( mixClip, count*4, mmeSound.fileHandle );
	mmeSound.fileSize += count * 4;
	mmeSound.gotFrame = qfalse;
}

void S_MMERecord( const char *baseName, float deltaTime ) {
#ifdef SND_MME
	char fileName[MAX_OSPATH];

	if (!mme_saveWav->integer)
		return;
	if (Q_stricmp(baseName, mmeSound.baseName)) {
		if (mmeSound.fileHandle)
			S_MMEWavClose();
		Com_sprintf( fileName, sizeof(fileName), "%s.wav", baseName );
		mmeSound.fileHandle = FS_FOpenFileReadWrite( fileName );
		if (!mmeSound.fileHandle) {
			mmeSound.fileHandle = FS_FOpenFileWrite( fileName );
			if (!mmeSound.fileHandle) 
				return;
		}
		Q_strncpyz( mmeSound.baseName, baseName, sizeof( mmeSound.baseName ));
		mmeSound.deltaSamples = 0;
		mmeSound.sampleRate = MME_SAMPLERATE;
		FS_Seek( mmeSound.fileHandle, 0, FS_SEEK_END );
		mmeSound.fileSize = FS_filelength( mmeSound.fileHandle );
		if ( mmeSound.fileSize < WAV_HEADERSIZE) {
			int left = WAV_HEADERSIZE - mmeSound.fileSize;
			while (left) {
				FS_Write( &left, 1, mmeSound.fileHandle );
				left--;
       		}
			mmeSound.fileSize = WAV_HEADERSIZE;
		}
	}
	mmeSound.deltaSamples += deltaTime * mmeSound.sampleRate;
	mmeSound.gotFrame = qtrue;
#endif
}

/* This is a seriously crappy hack, but it'll work for now */
void S_MMEMusic( const char *musicName, float time, float length ) {
	if ( !musicName || !musicName[0] ) {
		s_background.playing = qfalse;
		s_background.override = qfalse;
		return;
	}
	s_background.override = qtrue;
	s_background.seekTime = time;
	s_background.length = length;
	s_background.playing = qtrue;
	s_background.reload = qtrue;

	Q_strncpyz( s_background.startName, musicName, sizeof( s_background.startName ));
	COM_DefaultExtension( s_background.startName, sizeof( s_background.startName ), ".wav" );
}

void S_MMEStopSound(int entityNum, int entchannel, sfxHandle_t sfxHandle) {
	int i;
	if (!mmeSound.fileHandle)
		return;
	for (i = 0; i < MME_SNDCHANNELS; i++) {
		if (mmeSound.channels[i].entChan == entchannel
			&& mmeSound.channels[i].entNum == entityNum
			&& (mmeSound.channels[i].handle == sfxHandle || sfxHandle < 0)) {
			mmeSound.channels[i].entChan = 0;
			mmeSound.channels[i].entNum = 0;
			mmeSound.channels[i].handle = 0;
			mmeSound.channels[i].hasOrigin = 0;
			VectorClear(mmeSound.channels[i].origin);
			mmeSound.channels[i].index = 0;
			mmeSound.channels[i].wasMixed = 0;
			if (sfxHandle >= -1)
				break;
		}
	}
}




// similar looking sound capture to actual MME capture, but for not modified sound code
// let's leave actual MME capture above for future with rainbows and unicorns
// UPD since jaMME 1.0: unicorns and rainbows arrived, so things below are prolly not needed

extern int s_soundtime;
extern portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];

typedef struct {
	char			baseName[MAX_OSPATH];
	fileHandle_t	fileHandle;
	long			fileSize;
	unsigned int	deltaSamples;
	int				time;
	qboolean		gotFrame;
	float			fps;
	portable_samplepair_t buffer[PAINTBUFFER_SIZE];
} mmeWave_t;

static mmeWave_t mmeWave;

void S_MMEClose(void) {
	byte header[WAV_HEADERSIZE];
	unsigned int something, something2;

	if (!mmeWave.fileHandle)
		return;

	S_MMEFillWavHeader( header, mmeWave.fileSize, dma.speed );
	FS_Seek( mmeWave.fileHandle, 0, FS_SEEK_SET );
	FS_Write( header, sizeof(header), mmeWave.fileHandle );
	FS_FCloseFile( mmeWave.fileHandle );
	Com_Memset( &mmeWave, 0, sizeof(mmeWave) );
}

extern int s_UseOpenAL;

void S_MMEWavRecord(const char *baseName, const float fps) {
#ifndef SND_MME
	char fileName[MAX_OSPATH];
	if (!mme_saveWav->integer || s_UseOpenAL)
		return;

	if (Q_stricmp(baseName, mmeWave.baseName)) {
		if (mmeWave.fileHandle)
			S_MMEClose();
		Com_sprintf( fileName, sizeof(fileName), "%s.wav", baseName );
		mmeWave.fileHandle = FS_FOpenFileReadWrite( fileName );
		if (!mmeWave.fileHandle) {
			mmeWave.fileHandle = FS_FOpenFileWrite( fileName );
			if (!mmeWave.fileHandle) 
				return;
		}
		mmeWave.fps = fps;
		Q_strncpyz( mmeWave.baseName, baseName, sizeof( mmeWave.baseName ));
		FS_Seek( mmeWave.fileHandle, 0, FS_SEEK_END );
		mmeWave.fileSize = FS_filelength( mmeWave.fileHandle );
		if ( mmeWave.fileSize < WAV_HEADERSIZE) {
			int left = WAV_HEADERSIZE - mmeWave.fileSize;
			while (left) {
				FS_Write( &left, 1, mmeWave.fileHandle );
				left--;
       		}
			mmeWave.fileSize = WAV_HEADERSIZE;
		}
		mmeWave.time = s_soundtime;
		memcpy( mmeWave.buffer, paintbuffer, sizeof( mmeWave.buffer ) );
	}
	mmeWave.gotFrame = qtrue;
#endif
}

void S_MMETransferWavChunks(void) {
	int count;
	short mixClip[MAXUPDATE*2];

	if (!mmeWave.fileHandle) {
		return;
	}
	if (!mmeWave.gotFrame) {
		S_MMEClose();
		return;
	}
	if (s_soundtime <= mmeWave.time) {
		return;
	}
	count = s_soundtime - mmeWave.time;
	if ( count < 1 ) {
		return;
	}
	mmeWave.time = s_soundtime;

	S_MixClipOutput(count, (int *)mmeWave.buffer, mixClip, 0, MAXUPDATE - 1);
	FS_Write( mixClip, count * 4, mmeWave.fileHandle );
	mmeWave.fileSize += count * 4;

	memcpy( mmeWave.buffer, paintbuffer, sizeof( mmeWave.buffer ) );
	mmeWave.gotFrame = qfalse;
}

qboolean S_MMERecording (void) {
	if (mmeWave.gotFrame) {
		s_soundtime += (int)ceil( dma.speed / mmeWave.fps );
	}
	return mmeWave.gotFrame;
}
#include "snd_local.h"
#include "snd_mix.h"

#define MME_SNDCHANNELS 128
#define MME_LOOPCHANNELS 128

#define SAMPLERATE	22050

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

	S_MMEFillWavHeader( header, mmeSound.fileSize, SAMPLERATE );
	FS_Seek( mmeSound.fileHandle, 0, FS_SEEK_SET );
	FS_Write( header, sizeof(header), mmeSound.fileHandle );
	FS_FCloseFile( mmeSound.fileHandle );
	Com_Memset( &mmeSound, 0, sizeof(mmeSound) );
}

void S_MixClipOutput (int count, const int *input, short *output, int outStart, int outMask) {
	int		i;
	int		val;

	for (i=0 ; i<count ; i++)
	{
		val = input[i*2+0] >> MIX_SHIFT;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < -32768)
			val = -32768;
		output[outStart*2 + 0] = val;

		val = input[i*2+1] >> MIX_SHIFT;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < -32768)
			val = -32768;
		output[outStart*2 + 1] = val;
		outStart = (outStart+1) & outMask;
	}
}

/*
===================
S_MME_Update

Called from CL_Frame() in cl_main.c when shooting avidemo
===================
*/
#define MAXUPDATE 4096
void S_MMEUpdate( float scale, const byte *buffer, int size ) {
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

	speed = (scale * (MIX_SPEED << MIX_SHIFT)) / SAMPLERATE;
	if (speed < 0 || (speed == 0 && scale) )
		speed = 1;

	Com_Memset( mixTemp, 0, sizeof(int) * count * 2);
/*	if ( speed > 0 ) {
		S_MixChannels( mmeSound.channels, MME_SNDCHANNELS, speed, count, mixTemp );
		S_MixLoops( mmeSound.loops, MME_LOOPCHANNELS, speed, count, mixTemp );
	}	
	S_MixClipOutput( count, mixTemp, mixClip, 0, MAXUPDATE - 1 );
*/	//FS_Write( mixClip, count*4, mmeSound.fileHandle );
	FS_Write( buffer, size, mmeSound.fileHandle );
	mmeSound.fileSize += size;
	mmeSound.gotFrame = qfalse;
}

void S_MMERecord( const char *baseName, float deltaTime ) {
	char fileName[MAX_OSPATH];

//	if (!mme_saveWav->integer)	//uncomment when audio capture be fine
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
		mmeSound.sampleRate = SAMPLERATE;
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
}
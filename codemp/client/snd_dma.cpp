#include "snd_local.h"
#include "snd_mix.h"

#define		DMA_SNDCHANNELS		128
#define		DMA_LOOPCHANNELS	128

extern qboolean s_soundMuted;
dma_t		dma;

static mixBackground_t	dmaBackground;
static mixLoop_t	dmaLoops[DMA_LOOPCHANNELS];
static mixChannel_t	dmaChannels[DMA_SNDCHANNELS];
static mixEffect_t	dmaEffect;
static qboolean		dmaInit;
static int   		dmaWrite;

cvar_t		*s_khz;
cvar_t		*s_mixahead;

void S_DMAStopSound(int entityNum, int entchannel, sfxHandle_t sfxHandle) {
	int i;
	for (i = 0; i < DMA_SNDCHANNELS; i++) {
		if (dmaChannels[i].entChan == entchannel
			&& dmaChannels[i].entNum == entityNum
			&& (dmaChannels[i].handle == sfxHandle || sfxHandle < 0)) {
			dmaChannels[i].entChan = 0;
			dmaChannels[i].entNum = 0;
			dmaChannels[i].handle = 0;
			dmaChannels[i].hasOrigin = 0;
			VectorClear(dmaChannels[i].origin);
			dmaChannels[i].index = 0;
			dmaChannels[i].wasMixed = 0;
			if (sfxHandle >= -1)
				break;
		}
	}
}

void S_DMAClearBuffer( void ) {
	int		clear;
		
	if (!dmaInit)
		return;

	/* Clear the active channels and loops */
	Com_Memset( dmaLoops, 0, sizeof( dmaLoops ));
	Com_Memset( dmaChannels, 0, sizeof( dmaChannels ));
	Com_Memset( &dmaEffect, 0, sizeof( dmaEffect ));

	if (dma.samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

	/* Fill the dma buffer */
	SNDDMA_BeginPainting ();
	if (dma.buffer)
		Snd_Memset(dma.buffer, clear, dma.samples * dma.samplebits/8);
	SNDDMA_Submit ();
}

void S_DMAInit(void) {
	s_khz = Cvar_Get ("s_khz", "22", CVAR_ARCHIVE);
	s_mixahead = Cvar_Get ("s_mixahead", "0.2", CVAR_ARCHIVE);

	dmaInit = SNDDMA_Init();
	dmaWrite = 0;
}

void S_DMA_Update( float scale ) {
	int				sane, count;
	float			ma, op;
	static int		lastPos;
	int				thisPos;
	int				lastWrite, lastRead;
	int				bufSize, bufDone;
	int				speed;
	int				buf[2048];
	int				thisTime;
	static int		lastTime = 0;

	if (!dmaInit)
		return;

	thisTime = Com_Milliseconds();

	bufSize = dma.samples >> (dma.channels-1);

	// Check for possible buffer underruns

	thisPos = SNDDMA_GetDMAPos() >> (dma.channels - 1);
	lastWrite = (lastPos <= dmaWrite) ? (dmaWrite - lastPos) : (bufSize - lastPos + dmaWrite);
	lastRead = ( lastPos <= thisPos ) ? (thisPos - lastPos) : (bufSize - lastPos + thisPos);
	if (lastRead > lastWrite) {
		bufDone = 0;
		dmaWrite = thisPos;
//		Com_Printf("OMG Buffer underrun\n");
	} else {
		bufDone = lastWrite - lastRead;
	}
//	Com_Printf( "lastRead %d lastWrite %d done %d\n", lastRead, lastWrite, bufDone );
	lastPos = thisPos;
	
	sane = thisTime - lastTime;
	if (sane<11) {
		sane = 11;			// 85hz
	}
	ma = s_mixahead->value * dma.speed;
	op = 0.05f + sane*dma.speed*0.01f;
	if (op < ma) {
		ma = op;
	}

	count = lastRead;
	if (bufDone + count < ma) {
		count = ma - bufDone + 1;
	} else if (bufDone + count > bufSize) {
		count = bufSize - bufDone;
	}

	if (count > sizeof(buf) / (2 * sizeof(int))) {
		count = sizeof(buf) / (2 * sizeof(int));
	}
	// mix to an even submission block size
	count = (count + dma.submission_chunk-1) & ~(dma.submission_chunk-1);

	// never mix more than the complete buffer
	speed = (scale * (MIX_SPEED << MIX_SHIFT)) / dma.speed;

	/* Make sure that the speed will always go forward for very small scales */
	if (speed == 0 && scale)
		speed = 1;

	/* Mix sound or fill with silence depending on speed */
	if (speed > 0) {
		/* mix the background track or init the buffer with silence */
		S_MixBackground(&dmaBackground, speed, count, buf);
		S_MixChannels(dmaChannels, DMA_SNDCHANNELS, speed, count, buf);
		S_MixLoops(dmaLoops, DMA_LOOPCHANNELS, speed, count, buf);
		S_MixEffects(&dmaEffect, speed, count, buf);
		/* We have to process all sounds and then fill with silence if muted */
		if (s_soundMuted)
			Com_Memset(buf, 0, sizeof(buf[0]) * count * 2);
	} else {
		Com_Memset(buf, 0, sizeof(buf[0]) * count * 2);
	}
	/* Lock dma buffer and copy/clip the final data */
	SNDDMA_BeginPainting ();
	S_MixClipOutput(count, buf, (short *)dma.buffer, dmaWrite, bufSize-1);
	SNDDMA_Submit ();
	dmaWrite += count;
	if (dmaWrite >= bufSize)
		dmaWrite -= bufSize;
	lastTime = thisTime;
}

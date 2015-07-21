//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

/*****************************************************************************
 * name:		snd_mem.c
 *
 * desc:		sound caching
 *
 * $Archive: /MissionPack/code/client/snd_mem.c $
 *
 *****************************************************************************/
#include "snd_local.h"

#define HAVE_LIBMAD
#ifndef __ANDROID__
#define HAVE_LIBOGG
#define HAVE_LIBFLAC
#endif
const char *ext[] = {
	".wav",
#ifdef HAVE_LIBMAD
	".mp3",
#endif
#ifdef HAVE_LIBOGG
	".ogg",
	".oga",
#endif
#ifdef HAVE_LIBFLAC
	".flac",
#endif
	NULL,
};

extern	cvar_t* s_language;
qboolean S_FileExists(char *fileName) {
	char *voice = strstr(fileName,"chars");
	fileHandle_t f;
	int i;
	if (voice && s_language) {
		if (stricmp("DEUTSCH",s_language->string)==0) {				
			strncpy(voice,"chr_d",5);	// same number of letters as "chars"
		} else if (stricmp("FRANCAIS",s_language->string)==0) {				
			strncpy(voice,"chr_f",5);	// same number of letters as "chars"
		} else if (stricmp("ESPANOL",s_language->string)==0) {				
			strncpy(voice,"chr_e",5);	// same number of letters as "chars"
		} else {
			voice = NULL;	// use this ptr as a flag as to whether or not we substituted with a foreign version
		}
	}
tryDefaultLanguage:
	i = 0;
	while (ext[i]) {
		COM_StripExtension(fileName, fileName, MAX_QPATH);
		COM_DefaultExtension(fileName, MAX_QPATH, ext[i]);
		FS_FOpenFileRead(fileName, &f, qtrue);
		if (f)
			break;
		i++;
	}
	/* switch back to english (default) and try again */
	if (!f && voice) {
		strncpy(voice, "chars", 5);
		voice = NULL;
		goto tryDefaultLanguage;
	}
	if (!f)
		return qfalse;
	FS_FCloseFile(f);
	return qtrue;
}

/*
===============================================================================

WAV loading

===============================================================================
*/

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
	open = (openSound_t *)Z_Malloc( sizeof( openSound_t ) + dataSize, TAG_SND_RAWDATA );
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

//=============================================================================

#ifdef HAVE_LIBMAD 
#ifdef _WIN32
#include "mad.h"
#pragma comment (lib, "libmad.lib")
#elif defined(__ANDROID__)
#include "../libs/android/include/mad.h"
#else
#include <mad.h>
#endif
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
			open->rate = mp3->frame.header.samplerate;
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

#ifdef HAVE_LIBOGG
#ifdef _WIN32
#include "vorbis/codec.h"
#pragma comment (lib, "libvorbis_static.lib")
#pragma comment (lib, "libogg_static.lib")
#else
#include <vorbis/codec.h>
#endif

typedef struct {
	ogg_sync_state	sync;
	ogg_stream_state stream;
	ogg_page		page;
	ogg_packet		packet;
	vorbis_info		info;
	vorbis_comment	comment;
	vorbis_dsp_state state;
	vorbis_block	block;
	int				samplesLeft;
	int				eos;
	float			**pcm;
} oggOpen_t;

static qboolean S_OggInit( openSound_t *open ) {
	oggOpen_t *ogg;
	char *buffer;
	int bytes;
	int i;

	ogg = (oggOpen_t *)open->data;

	ogg_sync_init(&ogg->sync);

	/* fragile!	Assumes all of our headers will fit in the first 8kB,
		 which currently they will */
	buffer = ogg_sync_buffer(&ogg->sync, 8192);
	bytes = S_StreamRead(open, 8192, buffer);
	ogg_sync_wrote(&ogg->sync, bytes);

	if(ogg_sync_pageout(&ogg->sync, &ogg->page) != 1) {
		if(bytes < 8192) {
			Com_Printf("OggInit:Out of data.\n");
		}
		Com_Printf("OggInit:Input does not appear to be an Ogg bitstream.\n");
		return qfalse;
	}

	ogg_stream_init(&ogg->stream, ogg_page_serialno(&ogg->page));

	vorbis_info_init(&ogg->info);
	vorbis_comment_init(&ogg->comment);
	if (ogg_stream_pagein(&ogg->stream, &ogg->page) < 0) {
		Com_Printf("OggInit:Error reading first page of Ogg bitstream data.\n");
		return qfalse;
	}

	if (ogg_stream_packetout(&ogg->stream, &ogg->packet) != 1) {
		Com_Printf("OggInit:Error reading initial header packet.\n");
		return qfalse;
	}

	if (vorbis_synthesis_headerin(&ogg->info, &ogg->comment, &ogg->packet) < 0) {
		Com_Printf("OggInit:This Ogg bitstream does not contain Vorbis audio data.\n");
		return qfalse;
	}

	i = 0;
	while (i < 2) {
		while (i < 2) {
			int result = ogg_sync_pageout(&ogg->sync, &ogg->page);
			if(result == 0)
				break;
			if(result == 1) {
				ogg_stream_pagein(&ogg->stream, &ogg->page);
				while (i < 2) {
					result = ogg_stream_packetout(&ogg->stream, &ogg->packet);
					if (result == 0)
						break;
					if (result < 0) {
						Com_Printf("OggInit:Corrupt secondary header.\n");
						return qfalse;
					}
					vorbis_synthesis_headerin(&ogg->info, &ogg->comment, &ogg->packet);
					i++;
				}
			}
		}
		buffer = ogg_sync_buffer(&ogg->sync, 4096);
		bytes = S_StreamRead(open, 4096, buffer);
		if (bytes == 0 && i < 2) {
			Com_Printf("OggInit:End of file before finding all Vorbis headers!\n");
			return qfalse;
		}
		ogg_sync_wrote(&ogg->sync ,bytes);
	}
	vorbis_synthesis_init(&ogg->state, &ogg->info);
	vorbis_block_init(&ogg->state, &ogg->block);
	return qtrue;
}

static int S_OggTotalSamples( openSound_t *open ) {
	oggOpen_t *ogg;
	int eos;
	int totalSamples;
	if (!open || !open->data)
		return 0;
	ogg = (oggOpen_t *)open->data;
	eos = 0;
	totalSamples = 0;
	while(!eos) {
		while (!eos) {
			int result = ogg_sync_pageout(&ogg->sync, &ogg->page);
			if (result == 0)
				break;
			if (result < 0) {
				Com_Printf("OggTotalSamples:Corrupt or missing data in bitstream; continuing...\n");
			} else {
				ogg_stream_pagein(&ogg->stream, &ogg->page);
				while (1) {
					result = ogg_stream_packetout(&ogg->stream, &ogg->packet);
					if (result == 0)
						break;
					if (result < 0) {
						/* no reason to complain; already complained above */
					} else {
						int samples;
						if (vorbis_synthesis(&ogg->block, &ogg->packet) == 0)
							vorbis_synthesis_blockin(&ogg->state, &ogg->block);
						while ((samples = vorbis_synthesis_pcmout(&ogg->state, NULL)) > 0) {
							vorbis_synthesis_read(&ogg->state, samples);
							totalSamples += samples ;
						}
					}
				}
				if (ogg_page_eos(&ogg->page))
					eos = 1;
			}
		}
		if (!eos) {
			char *buffer = ogg_sync_buffer(&ogg->sync, 4096);
			int bytes = S_StreamRead(open, 4096, buffer);
			ogg_sync_wrote(&ogg->sync, bytes);
			if (bytes == 0)
				eos = 1;
		}
	}
	return totalSamples;
}

static short S_OggSample(float fsample) {
	int sample = floor(fsample*32767.f+.5f);
	if (sample > 32767) {
		return 32767;
	} else if (sample < -32768) {
		return -32768;
	}
	return sample;
}

static int S_OggRead( openSound_t *open, qboolean stereo, int size, short *data ) {
	oggOpen_t *ogg;
	int done;
	int bufSize;
	int result;

	if (!open || !open->data )
		return 0;
	ogg = (oggOpen_t *)(open->data);
	bufSize = 4096/ogg->info.channels;
	done = 0;
	if (ogg->samplesLeft) {
		goto finishSamples;
	}
	/* Have we got any pcm data waiting */
	while(!ogg->eos && size > 0) {
		while (!ogg->eos && size > 0) {
			result = ogg_sync_pageout(&ogg->sync, &ogg->page);
			if (result == 0)
				break;
			if (result < 0) {
//				Com_Printf("S_OggRead:Corrupt or missing data in bitstream; continuing...\n");
			} else {
				ogg_stream_pagein(&ogg->stream, &ogg->page);
				while (1) {
					result = ogg_stream_packetout(&ogg->stream, &ogg->packet);
					if (result == 0)
						break;
					if (result < 0) {
						/* no reason to complain; already complained above */
					} else {
						if (vorbis_synthesis(&ogg->block, &ogg->packet) == 0)
							vorbis_synthesis_blockin(&ogg->state, &ogg->block);
						while ((ogg->samplesLeft = vorbis_synthesis_pcmout(&ogg->state, &ogg->pcm)) > 0 && size > 0) {
finishSamples:
							int i, todo;
							todo = ogg->samplesLeft;
							todo = todo < size ? todo : size;
							todo = todo < bufSize ? todo : bufSize;
							if ( ogg->info.channels == 1 ) {
								float *left = ogg->pcm[0];
								if ( stereo ) {
									for (i = 0;i<todo;i++) {
										data[0] = data[1] = S_OggSample( left[i] );
										data += 2;
									}
								} else{
									for (i = 0;i<todo;i++) {
										*data++ = S_OggSample( left[i] );
									}
								}
								ogg->pcm[0] += todo;
							} else if ( ogg->info.channels == 2 ) {
								float *left = ogg->pcm[0];
								float *right = ogg->pcm[1];
								if ( stereo ) {
									for (i = 0;i<todo;i++) {
										data[0] = S_OggSample( left[i] );
										data[1] = S_OggSample( right[i]);
										data+=2;
									}
								} else {
									for (i = 0;i<todo;i++) {
										int addSample;
										addSample = S_OggSample( left[i] );
										addSample += S_OggSample( right[i] );
										*data++ = addSample >> 1;
									}
								}
								ogg->pcm[0] += todo;
								ogg->pcm[1] += todo;
							} else {
								return done;
							}
							vorbis_synthesis_read(&ogg->state, todo);
							done += todo;
							size -= todo;
							ogg->samplesLeft -= todo;
							if ( ogg->samplesLeft || size <= 0 )
								return done;
						}
					}
				}
				if (ogg_page_eos(&ogg->page))
					ogg->eos = 1;
			}
		}
		if (!ogg->eos) {
			char *buffer = ogg_sync_buffer(&ogg->sync, 4096);
			int bytes = S_StreamRead(open, 4096, buffer);
			ogg_sync_wrote(&ogg->sync, bytes);
			if (bytes == 0)
				ogg->eos = 1;
		}
	}
	return done;
}

static int S_OggSeek( struct openSound_s *open, int samples ) {
	oggOpen_t *ogg;
	if (!open || !open->data )
		return 0;
	ogg = (oggOpen_t *)open->data;
	ogg->samplesLeft = 0;
	ogg->eos = 0;
	S_StreamSeek(open, ((float)samples/open->totalSamples)*(open->fileSize));
	return samples;
}

static void S_OggClose( openSound_t *open) {
	oggOpen_t *ogg;
	if (!open || !open->data )
		return;
	ogg = (oggOpen_t *)(open->data);	
	ogg_stream_clear (&ogg->stream);
	vorbis_block_clear (&ogg->block);
	vorbis_dsp_clear (&ogg->state);
	vorbis_comment_clear (&ogg->comment);
	vorbis_info_clear (&ogg->info);
	ogg_sync_clear (&ogg->sync);
}

#ifdef HAVE_LIBFLAC
extern "C" {
#ifdef _WIN32
//ent: important to define FLAC__NO_DLL if we use a static lib
#define FLAC__NO_DLL
#define FLAC__HAS_OGG
#include "FLAC/stream_decoder.h"
#else
#define FLAC__NO_DLL
#define FLAC__HAS_OGG
#include <FLAC/stream_decoder.h>
#endif
}
static openSound_t *S_FlacOpen( const char *fileName, FLAC__bool is_ogg );
#endif
static openSound_t *S_OggOpen( const char *fileName ) {
	oggOpen_t *ogg;
	openSound_t *open;

	open = S_StreamOpen( fileName, sizeof( oggOpen_t ) );
	if (!open) {
		Com_Printf("OggOpen:File %s failed to open\n", fileName);
		return 0;
	}
	ogg = (oggOpen_t *)open->data;
	open->read = S_OggRead;
	open->seek = S_OggSeek;
	open->close = S_OggClose;
	
	if (!S_OggInit(open)) {
		Com_Printf("OggOpen:File %s failed to init\n", fileName);
		S_SoundClose( open );
#if defined HAVE_LIBFLAC && defined FLAC__HAS_OGG
		Com_Printf("OggOpen:Checking for flac codec...\n", fileName);
		open = S_FlacOpen(fileName, /*is_ogg =*/1);
		if (open)
			return open;
		Com_Printf("OggOpen:File %s failed to open as flac\n", fileName);
#endif
		return 0;
	}
	if ((open->totalSamples = S_OggTotalSamples(open)) <= 0) {
		Com_Printf("OggOpen:File %s has no samples\n", fileName);
		S_SoundClose( open );
		return 0;
	}

	/* Reset to the beginning and finalize setting of values */
	S_StreamSeek(open, 0);
	open->rate = ogg->info.rate;
	return open;
}

#endif

#ifdef HAVE_LIBFLAC
//ent: my windows static lib built with FLAC__HAS_OGG, FLAC__CPU_IA32, FLAC__HAS_NASM, FLAC__HAS_X86INTRIN,
//FLAC__SSE2_SUPPORTED, FLAC__SSE4_1_SUPPORTED
#ifdef _WIN32
#pragma comment (lib, "libFLAC_static.lib")
#endif

typedef struct {
	FLAC__StreamDecoder *decoder;
	qboolean			stereo;
	int					size;
	short				*data;
	int					done;
	const FLAC__int32	*buffer[FLAC__MAX_CHANNELS];
	int					samplesLeft;
	unsigned int		channels;
	qboolean			seeked;
} flacOpen_t;

short *S_FlacWriteData(flacOpen_t *flac, short *data, int todo) {
	qboolean stereo = flac->stereo;
	unsigned int channels = flac->channels;
	int i;
	if ( channels == 1 ) {
		if ( stereo ) {
			for (i = 0;i<todo;i++) {
				data[0] = data[1] = (FLAC__int16)flac->buffer[0][i];
				data += 2;
			}
		} else{
			for (i = 0;i<todo;i++) {
				*data++ = (FLAC__int16)flac->buffer[0][i];
			}
		}
		flac->buffer[0] += todo;
	} else if ( channels == 2 ) {
		if ( stereo ) {
			for (i = 0;i<todo;i++) {
				data[0] = (FLAC__int16)flac->buffer[0][i];
				data[1] = (FLAC__int16)flac->buffer[1][i];
				data+=2;
			}
		} else {
			for (i = 0;i<todo;i++) {
				int addSample;
				addSample = (FLAC__int16)flac->buffer[0][i];
				addSample += (FLAC__int16)flac->buffer[1][i];
				*data++ = addSample >> 1;
			}
		}
		flac->buffer[0] += todo;
		flac->buffer[1] += todo;
	} else if ( channels >= 3 && channels <= FLAC__MAX_CHANNELS) {
		if ( stereo ) {
			for (i = 0;i<todo;i++) {
				int addSample;
				int u;
				//if we have even amount of channels, then put all the odd ones to left output channel
				//and all the even ones to right output channel
				//if we have odd amount of channels, then put all the odd ones except the last to left output channel
				//and all even ones to right output channel, the last odd one goes to both left and right channels
				float halfChan = (float)channels/2;
				addSample = (FLAC__int16)flac->buffer[0][i];
				for (u = 1; u < (halfChan-0.2f); u++)
					addSample += (FLAC__int16)flac->buffer[u*2][i];
				data[0] = (float)addSample / ((channels%2 == 0)?(channels/2):(channels+1)/2);
				addSample = (FLAC__int16)flac->buffer[1][i];
				for (u = 1; u < (halfChan-0.2f); u++)
					addSample += (FLAC__int16)flac->buffer[(u*2)+((halfChan-u)<0.8f)?0:1][i];
				data[1] = (float)addSample / ((channels%2 == 0)?(channels/2):(channels+1)/2);
				data+=2;
			}
		} else {
			for (i = 0;i<todo;i++) {
				int addSample;
				int u;
				addSample = (FLAC__int16)flac->buffer[0][i];
				for (u = 1; u < (channels); u++)
					addSample += (FLAC__int16)flac->buffer[u][i];
				addSample += (FLAC__int16)flac->buffer[2][i];
				*data++ = (float)addSample / (channels);
			}
		}
		for (i = 0; i < channels; i++) {
			flac->buffer[i] += todo;
		}
	} else {
		return NULL;
	} 
	return data;
}

FLAC__StreamDecoderWriteStatus S_FlacWriteCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data) {
	openSound_t	*open = (openSound_t *)client_data;
	flacOpen_t	*flac = (flacOpen_t *)(open->data);
	int			i;
	if(buffer [0] == NULL) {
		Com_Printf("Flac error: buffer [0] is NULL\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	flac->channels = frame->header.channels;
	flac->samplesLeft = frame->header.blocksize;
	for (i = 0; i < flac->channels; i++)
		flac->buffer[i] = buffer[i];
	/* write decoded PCM samples */
	while(flac->size > 0) {
		int todo;
		short *data;
		if ( flac->samplesLeft <= flac->size ) {
			todo = flac->samplesLeft;
		} else {
			todo = flac->size;
		}
		data = S_FlacWriteData(flac, flac->data, todo);
		if ( !data ) {
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		} 
		flac->data = data;
		flac->done += todo;
		flac->size -= todo;
		flac->samplesLeft -= todo;
		if ( flac->samplesLeft && !flac->size )
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		else if ( flac->size && !flac->samplesLeft )
			return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
		//wtf should never reach, or maybe when flac->samplesLeft == size == 0
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	//although we call this func when flac->size is always > 0, but to please the compiler we return anyways
	return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

void S_FlacMetadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
}

void S_FlacErrorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
	openSound_t *open = (openSound_t *)client_data;
	flacOpen_t *flac = (flacOpen_t *)open->data;
	//entFIXME: special hack to ignore error message of bad header although the sound is being played fine
	if (!(flac->seeked && (status == FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER
		//ent: since we can jump in time sometimes we can catch FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC
		//and according to docs that error below is not fatal
		|| status == FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)))
		Com_Printf("Flac error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}
FLAC__StreamDecoderReadStatus S_FlacReadCallback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data) {
	openSound_t *open = (openSound_t *)client_data;
	if(*bytes > 0) {
		*bytes = S_StreamRead(open, (*bytes)*sizeof(FLAC__byte), buffer);
		if(*bytes == 0)
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		else
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}
	else
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT; /* abort to avoid a deadlock */
}

FLAC__StreamDecoderSeekStatus S_FlacSeekCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data) {
	openSound_t *open = (openSound_t *)client_data;
	flacOpen_t *flac = (flacOpen_t *)open->data;
	if(S_StreamSeek(open, (int)absolute_byte_offset)) {
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	} else {
		flac->seeked = qtrue;
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
	}
}

FLAC__StreamDecoderTellStatus S_FlacTellCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data) {
	int pos;
	openSound_t *open = (openSound_t *)client_data;
	if((pos = FS_FTell(open->fileHandle)) < 0)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}
}

FLAC__StreamDecoderLengthStatus S_FlacLengthCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data) {
	openSound_t *open = (openSound_t *)client_data;
	if(open->fileSize <= 0)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	else {
		*stream_length = open->fileSize;
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

FLAC__bool S_FlacEOFCallback(const FLAC__StreamDecoder *decoder, void *client_data) {
	openSound_t *open = (openSound_t *)client_data;
	return FS_FEof(open->fileHandle)? 1 : 0;
}

static int S_FlacRead( openSound_t *open, qboolean stereo, int size, short *data ) {
	flacOpen_t *flac;
	FLAC__StreamDecoderState state;

	if (!open || !open->data )
		return 0;
	flac = (flacOpen_t *)(open->data);
	flac->done = 0;
	if (flac->samplesLeft && flac->channels) {
		int todo;
		if ( flac->samplesLeft <= size ) {
			todo = flac->samplesLeft;
		} else {
			todo = size;
		}
		flac->data = S_FlacWriteData(flac, data, todo);
		flac->done += todo;
		size -= todo;
		flac->samplesLeft -= todo;
		if ( !size || !flac->data )
			return flac->done;
		data = flac->data;
	}
	flac->data = data;
	flac->size = size;
	flac->stereo = stereo;
//	state = FLAC__stream_decoder_get_state(flac->decoder);
//	Com_Printf("Flac decoder state: %s\n", FLAC__StreamDecoderStateString[state]);
	if (!FLAC__stream_decoder_process_until_end_of_stream(flac->decoder)) {
		state = FLAC__stream_decoder_get_state(flac->decoder);
//		Com_Printf("Flac decoder state: %s\n", FLAC__StreamDecoderStateString[state]);
		if (state == FLAC__STREAM_DECODER_SEEK_ERROR) {
			FLAC__stream_decoder_flush(flac->decoder);
			if (flac->size)
				FLAC__stream_decoder_process_until_end_of_stream(flac->decoder);
		}
	}
	return flac->done;
}

static int S_FlacSeek( struct openSound_s *open, int samples ) {
	flacOpen_t *flac;
	FLAC__StreamDecoderState state;
	if (!open || !open->data )
		return 0;
	flac = (flacOpen_t *)open->data;
	flac->samplesLeft = 0;
	if (samples == 0) {
		FLAC__stream_decoder_reset(flac->decoder);
		FLAC__stream_decoder_process_until_end_of_metadata(flac->decoder);
		return 0;
	}
	state = FLAC__stream_decoder_get_state(flac->decoder);
	if (state != FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC)
		FLAC__stream_decoder_flush(flac->decoder);
	flac->seeked = qfalse;
	if (FLAC__stream_decoder_seek_absolute(flac->decoder, samples) || flac->seeked) {
		state = FLAC__stream_decoder_get_state(flac->decoder);
//		Com_Printf("Flac decoder state: %s\n", FLAC__StreamDecoderStateString[state]);
		if (state != FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC)
			FLAC__stream_decoder_flush(flac->decoder);
//		state = FLAC__stream_decoder_get_state(flac->decoder);
//		Com_Printf("Flac decoder state: %s\n", FLAC__StreamDecoderStateString[state]);
//		FLAC__stream_decoder_process_until_end_of_metadata(flac->decoder);
//		state = FLAC__stream_decoder_get_state(flac->decoder);
//		Com_Printf("Flac decoder state: %s\n", FLAC__StreamDecoderStateString[state]);
		return samples;
	}
//	state = FLAC__stream_decoder_get_state(flac->decoder);
//	Com_Printf("Flac decoder state: %s\n", FLAC__StreamDecoderStateString[state]);
	return 0;
}

static void S_FlacClose( openSound_t *open) {
	flacOpen_t *flac;
	if (!open || !open->data )
		return;
	flac = (flacOpen_t *)(open->data);	
	FLAC__stream_decoder_delete(flac->decoder);
}

static openSound_t *S_FlacOpen( const char *fileName, FLAC__bool is_ogg ) {
	flacOpen_t *flac;
	openSound_t *open;
	FLAC__StreamDecoderInitStatus init_status;

	open = S_StreamOpen( fileName, sizeof( flacOpen_t ) );
	if (!open) {
		Com_Printf("FlacOpen:File %s failed to open\n", fileName);
		return 0;
	}
	flac = (flacOpen_t *)open->data;
	open->read = S_FlacRead;
	open->seek = S_FlacSeek;
	open->close = S_FlacClose;	
	flac->decoder = 0;
	if((flac->decoder = FLAC__stream_decoder_new()) == NULL) {
		Com_Printf("FlacOpen:File %s failed to allocate decoder\n", fileName);
		S_SoundClose( open );
		return 0;
	}	
	(void)FLAC__stream_decoder_set_md5_checking(flac->decoder, true);
	if (!is_ogg)
	init_status = FLAC__stream_decoder_init_stream(flac->decoder,
		S_FlacReadCallback,
		S_FlacSeekCallback,
		S_FlacTellCallback,
		S_FlacLengthCallback,
		S_FlacEOFCallback,
		S_FlacWriteCallback,
		S_FlacMetadataCallback,
		S_FlacErrorCallback,
		open);
	else
	init_status = FLAC__stream_decoder_init_ogg_stream(flac->decoder,
		S_FlacReadCallback,
		S_FlacSeekCallback,
		S_FlacTellCallback,
		S_FlacLengthCallback,
		S_FlacEOFCallback,
		S_FlacWriteCallback,
		S_FlacMetadataCallback,
		S_FlacErrorCallback,
		open);

	if(init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		Com_Printf("FlacOpen:File %s failed to init decoder: %s\n", fileName, FLAC__StreamDecoderInitStatusString[init_status]);
		S_SoundClose( open );
		return 0;
	}

	/* Reset to the beginning and finalize setting of values */
	FLAC__stream_decoder_process_until_end_of_metadata(flac->decoder);
	open->totalSamples = FLAC__stream_decoder_get_total_samples(flac->decoder);
	//special hack for flac.ogg
	if (!open->totalSamples) {
		while (1) {
			if (FLAC__stream_decoder_process_single(flac->decoder) == 0 ||
				FLAC__stream_decoder_get_state(flac->decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
				break;
		}
		open->totalSamples = FLAC__stream_decoder_get_total_samples(flac->decoder);
	}
	if (!open->totalSamples) {
		Com_Printf("FlacOpen:File %s has unknown amount of samples\n", fileName);
		S_SoundClose( open );
		return 0;
	}
	FLAC__stream_decoder_process_single(flac->decoder);
	open->rate = FLAC__stream_decoder_get_sample_rate(flac->decoder);//flac->decoder->private_->stream_info.data.stream_info.sample_rate;
	FLAC__stream_decoder_reset(flac->decoder);
	FLAC__stream_decoder_process_until_end_of_metadata(flac->decoder);
	return open;
}
#endif

openSound_t *S_SoundOpen( const char *fileName ) {
	const char *fileExt;

	if (!fileName || !fileName[0]) {
		Com_Printf("SoundOpen:Filename is empty\n" );
		return 0;
	}	
	fileExt = Q_strrchr( fileName, '.' );
	if (!fileExt) {
		Com_Printf("SoundOpen:File %s has no extension\n", fileName );
		return 0;
	}	
	if (!Q_stricmp( fileExt, ".wav")) {
		return S_WavOpen( fileName );
#ifdef HAVE_LIBMAD
	} else if (!Q_stricmp( fileExt, ".mp3")) {
		return S_MP3Open( fileName );
#endif
#ifdef HAVE_LIBOGG
	} else if (!Q_stricmp( fileExt, ".ogg") || !Q_stricmp( fileExt, ".oga")) {
		return S_OggOpen( fileName );
#endif
#ifdef HAVE_LIBFLAC
	} else if (!Q_stricmp( fileExt, ".flac")) {
		return S_FlacOpen( fileName, /*is_ogg =*/0 );
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

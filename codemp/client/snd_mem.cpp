//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

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

/*
===============================================================================

WAV loading

===============================================================================
*/

//ZTM's super replacer
static void S_SoundChangeExt(char *fileName, const char *ext) {
	char *p = strchr(fileName, '.');			/* find first '.' */
	if (p != NULL) {							/* found '.' */
		Q_strncpyz(p, ext, strlen(ext) + 1);	/* change ext */
	} else {									/* should never happen */
		strcat(fileName, ext);					/* has no ext, so just add it */
	}
}

extern	cvar_t* s_language;
qboolean S_FileExists(char *fileName) {
	char *voice = strstr(fileName,"chars");
	fileHandle_t f;
	if (voice) {
		if (s_language && stricmp("DEUTSCH",s_language->string)==0) {				
			strncpy(voice,"chr_d",5);	// same number of letters as "chars"
		} else if (s_language && stricmp("FRANCAIS",s_language->string)==0) {				
			strncpy(voice,"chr_f",5);	// same number of letters as "chars"
		} else if (s_language && stricmp("ESPANOL",s_language->string)==0) {				
			strncpy(voice,"chr_e",5);	// same number of letters as "chars"
		} else {
			voice = NULL;	// use this ptr as a flag as to whether or not we substituted with a foreign version
		}
	}
	COM_StripExtension(fileName, fileName, MAX_QPATH);
	S_SoundChangeExt(fileName, ".wav"); //append
	FS_FOpenFileRead(fileName, &f, qtrue);
	if (!f) {
#ifdef HAVE_LIBMAD
		S_SoundChangeExt(fileName, ".mp3");
		FS_FOpenFileRead(fileName, &f, qtrue);
#endif
		if (!f) {
			if (voice) {
				strncpy(voice,"chars",5);
				S_SoundChangeExt(fileName, ".wav");
				FS_FOpenFileRead(fileName, &f, qtrue);
				if (!f) {
#ifdef HAVE_LIBMAD
					S_SoundChangeExt(fileName, ".mp3");
					FS_FOpenFileRead(fileName, &f, qtrue);
#endif
				}
			}
			if (!f) {
				return qfalse;
			}
		}
	}
	FS_FCloseFile(f);
	return qtrue;
}

static qboolean S_SwitchLang(char *fileName) {
	int iNameStrlen = strlen(fileName);
	char *voice = strstr(fileName,"chars");
	if (voice) {
		if (s_language && stricmp("DEUTSCH",s_language->string)==0) {				
			strncpy(voice,"chr_d",5);	// same number of letters as "chars"
		} else if (s_language && stricmp("FRANCAIS",s_language->string)==0) {				
			strncpy(voice,"chr_f",5);	// same number of letters as "chars"
		} else if (s_language && stricmp("ESPANOL",s_language->string)==0) {				
			strncpy(voice,"chr_e",5);	// same number of letters as "chars"
		}
		return qfalse;
	} else {
		voice = strstr(fileName,"chr_d");
		if (voice) {
			strncpy(voice,"chars",5);
			return qtrue;
		} else {
			voice = strstr(fileName,"chr_f");
			if (voice) {
				strncpy(voice,"chars",5);
				return qtrue;
			} else {
				voice = strstr(fileName,"chr_f");
				if (voice) {
					strncpy(voice,"chars",5);
					return qtrue;
				}
			}
		}
		return qfalse; //has to reach that only if it's not "chars" related sound, 1 - replaced sound
	}
}

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

openSound_t *S_SoundOpen(const char *constFileName) {
	const char *fileExt;
	char temp[MAX_QPATH];
	char *fileName;
	openSound_t *open;
	qboolean wasHere = qfalse;

#ifdef FINAL_BUILD
	doNotYell = qtrue;
#else
	doNotYell = qfalse;
#endif

	Q_strncpyz(temp, constFileName, sizeof(temp));
	fileName = temp;

	if (!fileName || !fileName[0]) {
		Com_Printf("SoundOpen:Filename is empty\n");
		return 0;
	}

	// do we still need language switcher
	// and sound/mp_generic_female/sound -> sound/chars/mp_generic_female/misc/sound (or male) converter?

	/* switch language: /chars/ -> /chr_f/, /chr_d/, /chr_e/ */
	if (Q_stricmp(s_language->string, "english"))
		S_SwitchLang(fileName);

	/* sound/mp_generic_female/sound -> sound/chars/mp_generic_female/misc/sound */
	char *match = strstr(fileName, "sound/mp_generic_female");
	if (match) {
		char out[MAX_QPATH];
		Q_strncpyz(out, fileName, MAX_QPATH);
		char *pos = strstr(match, "le/");
		pos = strchr(pos, '/');
		Q_strncpyz(out, "sound/chars/mp_generic_female/misc", MAX_QPATH);
		Q_strcat(out, sizeof(out), pos);
		fileName = out;
	} else {
	/* or sound/mp_generic_male/sound -> sound/chars/mp_generic_male/misc/sound */
		match = strstr(fileName, "sound/mp_generic_male");
		if (match) {
			char out[MAX_QPATH];
			Q_strncpyz(out, fileName, MAX_QPATH);
			char *pos = strstr(match, "le/");
			pos = strchr(pos, '/');
			Q_strncpyz(out, "sound/chars/mp_generic_male/misc", MAX_QPATH);
			Q_strcat(out, sizeof(out), pos);
			fileName = out;
		}
	}

	fileExt = Q_strrchr(fileName, '.');
	if (!fileExt) {
		strcat(fileName, ".wav");
	} else if (!Q_stricmp(fileExt, ".mp3")) { //we want to start seeking .wav at first
		S_SoundChangeExt(fileName, ".wav");
	} else if (Q_stricmp(fileExt, ".wav")
#ifdef HAVE_LIBMAD
		&& Q_stricmp(fileExt, ".mp3")
#endif
		) {
		Com_Printf("SoundOpen:File %s has unknown extension %s\n", fileName, fileExt );
		return 0;
	}

tryAgainThisSound:
	open = S_WavOpen(fileName);
	if (open) {
		return open;
	} else {
#ifdef HAVE_LIBMAD
		S_SoundChangeExt(fileName, ".mp3");
		open = S_MP3Open(fileName);
		if (open) {
			return open;
		} else
#endif
		if (Q_stricmp(s_language->string, "english") && !wasHere) {
			if (S_SwitchLang(fileName)) {
				wasHere = qtrue;
				if (!doNotYell)
					Com_Printf("SoundOpen:File %s doesn't exist in %s, retrying with english\n", fileName, s_language->string );
#ifdef HAVE_LIBMAD
				//set back to wav
				S_SoundChangeExt(fileName, ".wav");
#endif
				goto tryAgainThisSound;
			}
		}
	}
	Com_Printf("S_SoundOpen:File %s failed to open\n", constFileName);
	return 0;
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

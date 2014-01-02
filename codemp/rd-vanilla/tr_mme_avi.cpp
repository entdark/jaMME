// Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

#include "tr_mme.h"

static void aviWrite16( void *pos, unsigned short val) {
	unsigned char *data = (unsigned char *)pos;
	data[0] = (val >> 0) & 0xff;
	data[1] = (val >> 8) & 0xff;
}

static void aviWrite32( void *pos, unsigned int val) {
	unsigned char *data = (unsigned char *)pos;
	data[0] = (val >> 0) & 0xff;
	data[1] = (val >> 8) & 0xff;
	data[2] = (val >> 16) & 0xff;
	data[3] = (val >> 24) & 0xff;
}

static void aviWrite64( void *pos, unsigned __int64 val) {
	unsigned char *data = (unsigned char *)pos;
	data[0] = (val >> 0) & 0xff;
	data[1] = (val >> 8) & 0xff;
	data[2] = (val >> 16) & 0xff;
	data[3] = (val >> 24) & 0xff;
	data[4] = (val >> 32) & 0xff;
	data[5] = (val >> 40) & 0xff;
	data[6] = (val >> 48) & 0xff;
	data[7] = (val >> 56) & 0xff;
}

void aviCloseAppend( mmeAviFile_t *aviFile ) {
	char avi_header[AVI_HEADER_SIZE_APPEND];
	char index[16];
	int main_list, nmain, njunk;
	int header_pos=0;
	int framePos, i;
	qboolean storeMJPEG = (qboolean)aviFile->format;

	char fileName[MAX_OSPATH];
	fileHandle_t aviHandle;

	if (!aviFile->f)
		return;

	ri.FS_FCloseFile(aviFile->f);
	aviFile->f = 0;

	ri.FS_FOpenFileByMode(va("%s.avi", aviFile->name), &aviHandle, FS_APPEND);

	Com_Memset( avi_header, 0, sizeof( avi_header ));

#define AVIOUT4AP(_S_) memcpy(&avi_header[header_pos],_S_,4);header_pos+=4;
#define AVIOUTwAP(_S_) aviWrite16(&avi_header[header_pos], _S_);header_pos+=2;
#define AVIOUTdAP(_S_) aviWrite32(&avi_header[header_pos], _S_);header_pos+=4;
#define AVIOUTddAP(_S_) aviWrite64(&avi_header[header_pos], _S_);header_pos+=8;
	/* Try and write an avi header */
	AVIOUT4AP("RIFF");                    // Riff header 
	AVIOUTdAP(AVI_HEADER_SIZE_APPEND + aviFile->writtenTotal - 8 + aviFile->frames * 16);
	AVIOUT4AP("AVI ");
	AVIOUT4AP("LIST");                    // List header
	main_list = header_pos;
	AVIOUTdAP(0);				            // TODO size of list
	AVIOUT4AP("hdrl");
	AVIOUT4AP("avih");
	AVIOUTdAP(56);                         /* # of bytes to follow */
	AVIOUTdAP(1000000 / aviFile->fps);    /* Microseconds per frame */
	AVIOUTdAP(0);
	AVIOUTdAP(0);                         /* PaddingGranularity (whatever that might be) */
	AVIOUTdAP(0x110);                     /* Flags,0x10 has index, 0x100 interleaved */
	AVIOUTdAP(aviFile->frames);		    /* TotalFrames */
	AVIOUTdAP(0);                         /* InitialFrames */
	AVIOUTdAP(1);                         /* Stream count */
	AVIOUTdAP(0);                         /* SuggestedBufferSize */
	AVIOUTdAP(aviFile->width);	        /* Width */
	AVIOUTdAP(aviFile->height);		    /* Height */
	AVIOUTdAP(0);                         /* TimeScale:  Unit used to measure time */
	AVIOUTdAP(0);                         /* DataRate:   Data rate of playback     */
	AVIOUTdAP(0);                         /* StartTime:  Starting time of AVI data */
	AVIOUTdAP(0);                         /* DataLength: Size of AVI data chunk    */

	/* Video stream list */
	AVIOUT4AP("LIST");
	AVIOUTdAP(4 + 8 + 56 + 8 + 40);       /* Size of the list */
	AVIOUT4AP("strl");
	/* video stream header */
    AVIOUT4AP("strh");
	AVIOUTdAP(56);                        /* # of bytes to follow */
	AVIOUT4AP("vids");                    /* Type */
	if ( aviFile->format == 1) { /* Handler */
		AVIOUT4AP("MJPG");      		    
	} else {
		AVIOUTdAP(0);
	}
	AVIOUTdAP(0);                         /* Flags */
	AVIOUTdAP(0);                         /* Reserved, MS says: wPriority, wLanguage */
	AVIOUTdAP(0);                         /* InitialFrames */
	AVIOUTdAP(1000000);                   /* Scale */
	AVIOUTdAP(1000000 * aviFile->fps);    /* Rate: Rate/Scale == samples/second */
	AVIOUTdAP(0);                         /* Start */
	AVIOUTdAP(aviFile->frames);           /* Length */
	AVIOUTdAP(0);                         /* SuggestedBufferSize */
	AVIOUTdAP(~0);                        /* Quality */
	AVIOUTdAP(0);                         /* SampleSize */
	AVIOUTdAP(0);                         /* Frame */
	AVIOUTdAP(0);                         /* Frame */
	/* The video stream format */
	AVIOUT4AP("strf");
	AVIOUTdAP(40);                        /* # of bytes to follow */
	AVIOUTdAP(40);                        /* Size */
	AVIOUTdAP(aviFile->width);            /* Width */
	AVIOUTdAP(aviFile->height);           /* Height */
	AVIOUTwAP(1);							/* Planes */
	if (!storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		AVIOUTwAP(8);						/* BitCount */
	} else {
		AVIOUTwAP(24);					/* BitCount */
	}
	if ( storeMJPEG ) { /* Compression */
		AVIOUT4AP("MJPG");      		    
	} else {
		AVIOUTdAP(0);
	}
	AVIOUTdAP(aviFile->width * aviFile->height*3);  /* SizeImage (in bytes?) */
	AVIOUTdAP(0);                  /* XPelsPerMeter */
	AVIOUTdAP(0);                  /* YPelsPerMeter */
	if ( !storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		AVIOUTdAP(255);			 /* colorUsed */
	} else {
		AVIOUTdAP(0);				 /* colorUsed */
	}
	AVIOUTdAP(0);                  /* ClrImportant: Number of colors important */
	if ( !storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		for(i = 0;i<255;i++) {
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = 0;
		}
	}

	
	AVIOUT4AP("indx");
	AVIOUTdAP(280);
	AVIOUTdAP(0);
	AVIOUTdAP(0);
	AVIOUT4AP("00db");							// 5203353600 our total
	AVIOUTdAP(0);
	AVIOUTdAP(0);
	AVIOUTdAP(0);

	unsigned __int64 ofst = aviFile->writtenTotal + AVI_HEADER_SIZE_APPEND - 8 + aviFile->frames * 16;

	for (i = 0; i <= aviFile->openDMLindexes; i++) {
		int sizeFrames = aviFile->openDMLframes[i]*8;
		AVIOUTddAP(ofst);
		AVIOUTdAP(sizeFrames);
		AVIOUTdAP(aviFile->openDMLframes[i]);
		ofst += sizeFrames;
	}

	header_pos += 1024 * 4;

	AVIOUT4AP("LIST");
	AVIOUTdAP(260);
	AVIOUT4AP("odml");
	AVIOUT4AP("dmlh");
	AVIOUTdAP(248);
	AVIOUTdAP(aviFile->frames);

	header_pos += 4 * 4 * 16 - 24;


	nmain = header_pos - main_list - 4;
	/* Finish stream list, i.e. put number of bytes in the list to proper pos */
	njunk = AVI_HEADER_SIZE_APPEND - 8 - header_pos;
	AVIOUT4AP("JUNK");
	AVIOUTdAP(njunk);
	if ( header_pos > AVI_HEADER_SIZE_APPEND) {
		ri.Error( ERR_FATAL, "Avi Header too large\n" );
	}
	/* Fix the size of the main list */
	header_pos = main_list;
	AVIOUTdAP(nmain);
	header_pos = AVI_HEADER_SIZE_APPEND - 12;
	AVIOUT4AP("LIST");
	AVIOUTdAP(aviFile->writtenTotal+4); /* Length of list in bytes */
	AVIOUT4AP("movi");
	
	ri.FS_Seek(aviHandle, 0, FS_SEEK_SET);
	ri.FS_Write( &avi_header, AVI_HEADER_SIZE_APPEND, aviHandle );

	byte *aviBuffer = new byte[AVI_MAX_SIZE_APPEND];

	memset(aviBuffer, 0, AVI_MAX_SIZE_APPEND);

	for (i = 0; i < AVI_MAX_APPEND_SEGMENTS; i++) {
		fileHandle_t aviSegment;
		int len;

		memset(fileName, 0, sizeof(fileName));
		
		Com_sprintfOld(fileName, sizeof(fileName), "%s.%03d.seg", aviFile->name, i);
		if (!ri.FS_FileExists(fileName)) {
			break;
		}

		ri.FS_FOpenFileRead(fileName, &aviSegment, qfalse);
		if (aviSegment <=0) {
			Com_Printf("Failed to open avi segment %s to append\n", fileName);
			return;
		}

		len = ri.FS_ReadFile(fileName, (void**)&aviBuffer);
		if (aviBuffer)
			ri.FS_Write(aviBuffer, len, aviHandle);

		Z_Free(aviBuffer);
		ri.FS_FCloseFile(aviSegment);
		ri.FS_FileErase(fileName);
	}
//	free(aviBuffer);

	/* First add the index table to the end */
	ri.FS_Write( "idx1", 4, aviHandle );
	aviWrite32( index, aviFile->frames * 16 );
	ri.FS_Write( index, 4, aviHandle );
	/* Bring on the load of small writes! */
	index[0] = '0';index[1] = '0';
	index[2] = 'd';index[3] = 'b';
	framePos = 4;		//filepos points to the block size
	for(i = 0;i<aviFile->frames;i++) {
		int size = aviFile->index[i];
		aviWrite32( index+4, 0x10 );
		aviWrite32( index+8, framePos );
		aviWrite32( index+12, size );
		framePos += (size + 9) & ~1;
		ri.FS_Write( index, 16, aviHandle );
	}

	ri.FS_FCloseFile(aviHandle);
	Com_Memset( aviFile, 0, sizeof( *aviFile ));
}

void aviClose( mmeAviFile_t *aviFile ) {
	char avi_header[AVI_HEADER_SIZE];
	char index[16];
	int main_list, nmain, njunk;
	int header_pos=0;
	int framePos, i;
	qboolean storeMJPEG = (qboolean)aviFile->format;

	if (!aviFile->f)
		return;
	Com_Memset( avi_header, 0, sizeof( avi_header ));

#define AVIOUT4(_S_) memcpy(&avi_header[header_pos],_S_,4);header_pos+=4;
#define AVIOUTw(_S_) aviWrite16(&avi_header[header_pos], _S_);header_pos+=2;
#define AVIOUTd(_S_) aviWrite32(&avi_header[header_pos], _S_);header_pos+=4;
	/* Try and write an avi header */
	AVIOUT4("RIFF");                    // Riff header 
	AVIOUTd(AVI_HEADER_SIZE + aviFile->written - 8 + aviFile->frames * 16);
	AVIOUT4("AVI ");
	AVIOUT4("LIST");                    // List header
	main_list = header_pos;
	AVIOUTd(0);				            // TODO size of list
	AVIOUT4("hdrl");
	AVIOUT4("avih");
	AVIOUTd(56);                         /* # of bytes to follow */
	AVIOUTd(1000000 / aviFile->fps );    /* Microseconds per frame */
	AVIOUTd(0);
	AVIOUTd(0);                         /* PaddingGranularity (whatever that might be) */
	AVIOUTd(0x110);                     /* Flags,0x10 has index, 0x100 interleaved */
	AVIOUTd( aviFile->frames );		    /* TotalFrames */
	AVIOUTd(0);                         /* InitialFrames */
	AVIOUTd(1);                         /* Stream count */
	AVIOUTd(0);                         /* SuggestedBufferSize */
	AVIOUTd(aviFile->width);	        /* Width */
	AVIOUTd(aviFile->height);		    /* Height */
	AVIOUTd(0);                         /* TimeScale:  Unit used to measure time */
	AVIOUTd(0);                         /* DataRate:   Data rate of playback     */
	AVIOUTd(0);                         /* StartTime:  Starting time of AVI data */
	AVIOUTd(0);                         /* DataLength: Size of AVI data chunk    */

	/* Video stream list */
	AVIOUT4("LIST");
	AVIOUTd(4 + 8 + 56 + 8 + 40);       /* Size of the list */
	AVIOUT4("strl");
	/* video stream header */
    AVIOUT4("strh");
	AVIOUTd(56);                        /* # of bytes to follow */
	AVIOUT4("vids");                    /* Type */
	if ( aviFile->format == 1) { /* Handler */
		AVIOUT4("MJPG");      		    
	} else {
		AVIOUTd(0);
	}
	AVIOUTd(0);                         /* Flags */
	AVIOUTd(0);                         /* Reserved, MS says: wPriority, wLanguage */
	AVIOUTd(0);                         /* InitialFrames */
	AVIOUTd(1000000);                   /* Scale */
	AVIOUTd(1000000 * aviFile->fps);    /* Rate: Rate/Scale == samples/second */
	AVIOUTd(0);                         /* Start */
	AVIOUTd(aviFile->frames);           /* Length */
	AVIOUTd(0);                         /* SuggestedBufferSize */
	AVIOUTd(~0);                        /* Quality */
	AVIOUTd(0);                         /* SampleSize */
	AVIOUTd(0);                         /* Frame */
	AVIOUTd(0);                         /* Frame */
	/* The video stream format */
	AVIOUT4("strf");
	AVIOUTd(40);                        /* # of bytes to follow */
	AVIOUTd(40);                        /* Size */
	AVIOUTd(aviFile->width);            /* Width */
	AVIOUTd(aviFile->height);           /* Height */
	AVIOUTw(1);							/* Planes */
	if (!storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		AVIOUTw(8);						/* BitCount */
	} else {
		AVIOUTw(24);					/* BitCount */
	}
	if ( storeMJPEG ) { /* Compression */
		AVIOUT4("MJPG");      		    
	} else {
		AVIOUTd(0);
	}
	AVIOUTd(aviFile->width * aviFile->height*3);  /* SizeImage (in bytes?) */
	AVIOUTd(0);                  /* XPelsPerMeter */
	AVIOUTd(0);                  /* YPelsPerMeter */
	if ( !storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		AVIOUTd(255);			 /* colorUsed */
	} else {
		AVIOUTd(0);				 /* colorUsed */
	}
	AVIOUTd(0);                  /* ClrImportant: Number of colors important */
	if ( !storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		for(i = 0;i<255;i++) {
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = 0;
		}
	}
	nmain = header_pos - main_list - 4;
	/* Finish stream list, i.e. put number of bytes in the list to proper pos */
	njunk = AVI_HEADER_SIZE - 8 - 12 - header_pos;
	AVIOUT4("JUNK");
	AVIOUTd(njunk);
	if ( header_pos > AVI_HEADER_SIZE) {
		ri.Error( ERR_FATAL, "Avi Header too large\n" );
	}
	/* Fix the size of the main list */
	header_pos = main_list;
	AVIOUTd(nmain);
	header_pos = AVI_HEADER_SIZE - 12;
	AVIOUT4("LIST");
	AVIOUTd(aviFile->written+4); /* Length of list in bytes */
	AVIOUT4("movi");
	/* First add the index table to the end */
	ri.FS_Write( "idx1", 4, aviFile->f );
	aviWrite32( index, aviFile->frames * 16 );
	ri.FS_Write( index, 4, aviFile->f );
	/* Bring on the load of small writes! */
	index[0] = '0';index[1] = '0';
	index[2] = 'd';index[3] = 'b';
	framePos = 4;		//filepos points to the block size
	for(i = 0;i<aviFile->frames;i++) {
		int size = aviFile->index[i];
		aviWrite32( index+4, 0x10 );
		aviWrite32( index+8, framePos );
		aviWrite32( index+12, size );
		framePos += (size + 9) & ~1;
		ri.FS_Write( index, 16, aviFile->f );
	}
	ri.FS_Seek(aviFile->f, 0, FS_SEEK_SET);
	ri.FS_Write( &avi_header, AVI_HEADER_SIZE, aviFile->f );
	ri.FS_FCloseFile(aviFile->f);
	Com_Memset( aviFile, 0, sizeof( *aviFile ));
}

qboolean aviOpen( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps) {
	char fileName[MAX_OSPATH];
	int i;
	char aviHeader[AVI_HEADER_SIZE];

	if (aviFile->f) {
		Com_Printf( "wtf openAvi on an open handler" );
		return qfalse;
	}
	/* First see if the file already exist */
	for (i = 0;i < AVI_MAX_FILES;i++) {
		Com_sprintfOld( fileName, sizeof(fileName), "%s.%03d.avi", name, i );
		if (!ri.FS_FileExists( fileName ))
			break;
	}
	if (i == AVI_MAX_FILES) {
		Com_Printf( "Max avi segments reached\n");
		return qfalse;
	}

	ri.FS_WriteFile( fileName, aviHeader, AVI_HEADER_SIZE );
	aviFile->f = ri.FS_FDirectOpenFileWrite( fileName, "w+b");
	if (!aviFile->f) {
		Com_Printf( "Failed to open %s for avi output\n", fileName );
		return qfalse;
	}
	/* File should have been reset to 0 size */
	ri.FS_Write( aviHeader, AVI_HEADER_SIZE, aviFile->f );
	aviFile->width = width;
	aviFile->height = height;
	aviFile->fps = fps;
	aviFile->frames = 0;
	aviFile->written = 0;
	aviFile->format = mme_aviFormat->integer;
	aviFile->type = type;
	Q_strncpyz( aviFile->name, name, sizeof( aviFile->name ));
	return qtrue;
}

qboolean aviValid( const mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps ) {
	if (!aviFile->f)
		return qfalse;
	if (aviFile->width != width)
		return qfalse;
	if (aviFile->height != height)
		return qfalse;
	if (aviFile->fps != fps )
		return qfalse;
	if (aviFile->type != type)
		return qfalse;
	if (aviFile->written >= AVI_MAX_SIZE)
		return qfalse;
	if ( mme_aviFormat->integer != aviFile->format)
		return qfalse;
	return qtrue;
}

qboolean aviOpenAppend( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps) {
	char fileName[MAX_OSPATH];
	int i;

	if (aviFile->f) {
		Com_Printf( "wtf openAvi on an open handler" );
		return qfalse;
	}
	/* First see if the file already exist */
	for (i = 0;i < AVI_MAX_APPEND_SEGMENTS;i++) {
		Com_sprintfOld( fileName, sizeof(fileName), "%s.%03d.seg", name, i );
		if (!ri.FS_FileExists( fileName ))
			break;
	}
	if (i == AVI_MAX_APPEND_SEGMENTS) {
		Com_Printf( "Max avi segments reached\n");
		return qfalse;
	}

	aviFile->f = ri.FS_FDirectOpenFileWrite( fileName, "w+b");
	if (!aviFile->f) {
		Com_Printf( "Failed to open %s for avi output\n", fileName );
		return qfalse;
	}

	aviFile->width = width;
	aviFile->height = height;
	aviFile->fps = fps;
	if (i == 0) {
		aviFile->frames = 0;
		for (i = 0; i < AVI_MAX_OPENDML_INDEXES; i++) {
			aviFile->openDMLframes[i] = 0;
		}
		aviFile->openDMLsize = 0;
		aviFile->openDMLindexes = 0;
	}
	aviFile->written = 0;
	aviFile->format = mme_aviFormat->integer;
	aviFile->type = type;
	Q_strncpyz( aviFile->name, name, sizeof( aviFile->name ));

	return qtrue;
}

qboolean aviValidAppend( const mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps ) {
	if (!aviFile->f)
		return qfalse;
	if (aviFile->width != width)
		return qfalse;
	if (aviFile->height != height)
		return qfalse;
	if (aviFile->fps != fps )
		return qfalse;
	if (aviFile->type != type)
		return qfalse;
	if (aviFile->written >= AVI_MAX_SIZE_APPEND)
		return qfalse;
	if ( mme_aviFormat->integer != aviFile->format)
		return qfalse;
	return qtrue;
}

void mmeAviShot( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, byte *inBuf ) {
	byte *outBuf;
	int i, pixels, outSize;
	if (!fps)
		return;
	if (mme_aviAppend->integer) {
		if (!aviValidAppend( aviFile, name, type, width, height, fps )) {
			if (aviFile->f) {
				ri.FS_FCloseFile(aviFile->f);
				aviFile->f = 0;
			}
			if (!aviOpenAppend( aviFile, name, type, width, height, fps ))
				return;
		}
	} else {
		if (!aviValid( aviFile, name, type, width, height, fps )) {
			aviClose( aviFile );
			if (!aviOpen( aviFile, name, type, width, height, fps ))
				return;
		}
	}
	pixels = width*height;
	outSize = width*height*3 + 2048; //Allocate bit more to be safish?
	outBuf = (byte *)ri.Hunk_AllocateTempMemory( outSize + 8);
	outBuf[0] = '0';outBuf[1] = '0';
	outBuf[2] = 'd';outBuf[3] = 'b';
	if ( aviFile->format == 0 ) {
		switch (type) {
		case mmeShotTypeGray:
			for (i = 0;i<pixels;i++) {
				outBuf[8 + i] = inBuf[i];
			};
			outSize = pixels;
			break;
		case mmeShotTypeRGB:
			for (i = 0;i<pixels;i++) {
				outBuf[8 + i*3 + 0 ] = inBuf[ i*3 + 2];
				outBuf[8 + i*3 + 1 ] = inBuf[ i*3 + 1];
				outBuf[8 + i*3 + 2 ] = inBuf[ i*3 + 0];
			}
			outSize = width * height * 3;
			break;
		} 
	} else if ( aviFile->format == 1 ) {
		outSize = SaveJPG( mme_jpegQuality->integer, width, height, type, inBuf, outBuf + 8, outSize );
	}
	aviWrite32( outBuf + 4, outSize );
	aviFile->index[ aviFile->frames ] = outSize;
	aviFile->frames++;

	outSize = (outSize + 9) & ~1;	//make sure we align on 2 byte boundary, hurray M$
	ri.FS_Write( outBuf, outSize, aviFile->f );
	aviFile->written += outSize;
	aviFile->writtenTotal += outSize;

	aviFile->openDMLsize += outSize;
	aviFile->openDMLframes[aviFile->openDMLindexes]++;

	if (aviFile->openDMLsize > AVI_MAX_OPENDML_APPEND) {
		aviFile->openDMLsize = 0;
		aviFile->openDMLindexes++;
	}

	ri.Hunk_FreeTempMemory( outBuf );

	if (aviFile->openDMLindexes >= AVI_MAX_OPENDML_INDEXES) {
		aviFile->openDMLindexes--;
		aviCloseAppend(aviFile);
	}
}
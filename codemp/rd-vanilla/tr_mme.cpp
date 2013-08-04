/*
============================================================================

MME additions

============================================================================
*/

#include "tr_mme.h"
#include "libpng/png.h"
#include "zlib/zlib.h"

static char *workAlloc = 0;
static char *workAlign = 0;
static int workSize, workUsed;

static char *workAlloc2 = 0;
static char *workAlign2 = 0;
static int workSize2, workUsed2;

#define TGAMASK 0xffffff
static int SaveTGA_RLERGB(byte *out, const int image_width, const int image_height, const void* image_buffer ) {
	int y;
	const unsigned int *inBuf = ( const unsigned int*)image_buffer;
	int dataSize = 0;
	
	for (y=0; y < image_height;y++) {
		int left = image_width;
		/* Prepare for the first block and write the first pixel */
		while ( left > 0 ) {
			/* Search for a block of similar pixels */
			int i, block = left > 128 ? 128 : left;
			unsigned int pixel = inBuf[0];
			/* Check for rle pixels */
			for ( i = 1;i < block;i++) {
				if ( inBuf[i] != pixel)
					break;
			}
			if ( i > 1  ) {
				out[dataSize++] = 0x80 | ( i - 1);
				out[dataSize++] = pixel >> 16;
				out[dataSize++] = pixel >> 8;
				out[dataSize++] = pixel >> 0;
			} else {
				int blockStart = dataSize++;
				/* Write some raw pixels no matter what*/
				out[dataSize++] = pixel >> 16;
				out[dataSize++] = pixel >> 8;
				out[dataSize++] = pixel >> 0;
				pixel = inBuf[1];
				for ( i = 1;i < block;i++) {
					if ( inBuf[i+1] == pixel)
						break;
					out[dataSize++] = pixel >> 16;
					out[dataSize++] = pixel >> 8;
					out[dataSize++] = pixel >> 0;
					pixel = inBuf[i+1];
				}
				out[blockStart] = i - 1;
			}
			inBuf += i;
			left -= i;
		}
	}
	return dataSize;
}


static int SaveTGA_RLEGray(byte *out, const int image_width, const int image_height, const void* image_buffer ) {
	int y;
	unsigned char *inBuf = (unsigned char*)image_buffer;

	int dataSize = 0;

	for (y=0; y < image_height;y++) {
		int left = image_width;
		int diffIndex, diff;
		unsigned char lastPixel, nextPixel;
		lastPixel = *inBuf++;

		diff = 0;
		while (left > 0 ) {
			int c, n;
			if (left >= 2) {
				nextPixel = *inBuf++;
				if (lastPixel == nextPixel) {
					if (diff) {
						out[diffIndex] = diff - 1;
						diff = 0;
					}
					left -= 2;
					c = left > 126 ? 126 : left;
					n = 0;

					while (c) {
						nextPixel = *inBuf++;
						if (lastPixel != nextPixel)
							break;
						c--; n++;
					}
					left -= n;
					out[dataSize++] = 0x80 | (n + 1);
					out[dataSize++] = lastPixel;
					lastPixel = nextPixel;
				} else {
finalDiff:
					left--;
					if (!diff) {
						diff = 1;
						diffIndex = dataSize++;
					} else if (++diff >= 128) {
						out[diffIndex] = diff - 1;
						diff = 0;
					}
					out[dataSize++] = lastPixel;
					lastPixel = nextPixel;
				}
			} else {
				goto finalDiff;
			}
		}
		if (diff) {
			out[diffIndex] = diff - 1;
		}
	}
	return dataSize;
}

/*
===============
SaveTGA
===============
*/

int SaveTGA( int image_compressed, int image_width, int image_height, mmeShotType_t image_type, byte *image_buffer, byte *out_buffer, int out_size ) {
	int i;
	int imagePixels = image_height * image_width;
	int pixelSize;
	int filesize = 18;	// header is here by default
	byte tgaFormat;

	// Fill in the header
	switch (image_type) {
	case mmeShotTypeGray:
		tgaFormat = 3;
		pixelSize = 1;
		break;
	case mmeShotTypeRGB:
		tgaFormat = 2;
		pixelSize = 3;
		break;
	default:
		return 0;
	}
	if (image_compressed)
		tgaFormat += 8;

	/* Clear the header */
	Com_Memset( out_buffer, 0, filesize );

	out_buffer[2] = tgaFormat;
	out_buffer[12] = image_width & 255;
	out_buffer[13] = image_width >> 8;
	out_buffer[14] = image_height & 255;
	out_buffer[15] = image_height >> 8;
	out_buffer[16] = 24;

	// Fill output buffer
	if (!image_compressed) { // Plain memcpy
		byte *buftemp = out_buffer+filesize;
		switch (image_type) {
		case mmeShotTypeRGB:
			for (i = 0; i < imagePixels; i++ ) {
				/* Also handle the RGB to BGR conversion here */
				*buftemp++ = image_buffer[2];
				*buftemp++ = image_buffer[1];
				*buftemp++ = image_buffer[0];
				image_buffer += 4;
			}
			filesize += image_width*image_height*3;
			break;
		case mmeShotTypeGray:
			/* Stupid copying of data here but oh well */
			Com_Memcpy( buftemp, image_buffer, image_width*image_height );
			filesize += image_width*image_height;
			break;
		}
	} else {
		switch (image_type) {
		case mmeShotTypeRGB:
			filesize += SaveTGA_RLERGB(out_buffer+filesize, image_width, image_height, image_buffer );
			break;
		case mmeShotTypeGray:
			filesize += SaveTGA_RLEGray(out_buffer+filesize, image_width, image_height, image_buffer );
			break;
		}
	}
	return filesize;
}


#ifdef USE_PNG
typedef struct {
	char *buffer;
	unsigned int bufferSize;
	unsigned int bufferUsed;
} PNGWriteData_t;

static void PNG_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	PNGWriteData_t *ioData = (PNGWriteData_t *)png_get_io_ptr( png_ptr );
	if ( ioData->bufferUsed + length < ioData->bufferSize) {
		Com_Memcpy( ioData->buffer + ioData->bufferUsed, data, length );
		ioData->bufferUsed += length;
	}
}

static void PNG_flush_data(png_structp png_ptr) {

}

/* Save PNG */
int SavePNG( int compresslevel, int image_width, int image_height, mmeShotType_t image_type, byte *image_buffer, byte *out_buffer, int out_size ) {
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	png_bytep *row_pointers = 0;
	PNGWriteData_t writeData;
	int i, rowSize;

	writeData.bufferUsed = 0;
	writeData.bufferSize = out_size;
	writeData.buffer = (char *)out_buffer;
	if (!writeData.buffer)
		goto skip_shot;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,NULL, NULL);
	if (!png_ptr)
		goto skip_shot;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto skip_shot;

	/* Finalize the initing of png library */
    png_set_write_fn(png_ptr, &writeData, PNG_write_data, PNG_flush_data );
	if (compresslevel < 0 || compresslevel > Z_BEST_COMPRESSION)
		compresslevel = Z_DEFAULT_COMPRESSION;
	png_set_compression_level(png_ptr, compresslevel );
	
	/* set other zlib parameters */
	png_set_compression_mem_level(png_ptr, 5);
	png_set_compression_strategy(png_ptr,Z_DEFAULT_STRATEGY);
	png_set_compression_window_bits(png_ptr, 15);
	png_set_compression_method(png_ptr, 8);
	png_set_compression_buffer_size(png_ptr, 8192);
	if ( image_type == mmeShotTypeRGB ) {
		rowSize = image_width*4;
		png_set_IHDR(png_ptr, info_ptr, image_width, image_height, 8, 
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr );
		png_set_filler(png_ptr, 1, PNG_FILLER_AFTER);
	} else if ( image_type == mmeShotTypeGray ) {
		rowSize = image_width*1;
		png_set_IHDR(png_ptr, info_ptr, image_width, image_height, 8, 
			PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr );
	}
	/*Allocate an array of scanline pointers*/
//	trap_TrueMalloc( (void **)&row_pointers, image_height*sizeof(png_bytep) );
//	row_pointers=(png_bytep*)malloc(image_height*sizeof(png_bytep));
	row_pointers = (png_bytep *)ri.Hunk_AllocateTempMemory( image_height*sizeof(png_bytep) );
	for (i=0;i<image_height;i++) {
		row_pointers[i]=(image_buffer+(image_height -1 - i )*rowSize );
	}
	/*tell the png library what to encode.*/
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, 0);

	//PNG_TRANSFORM_PACKSWAP | PNG_TRANSFORM_STRIP_FILLER
skip_shot:
	if (png_ptr)
		png_destroy_write_struct(&png_ptr, &info_ptr);
	if (row_pointers)
//		trap_TrueFree( (void **)&row_pointers );
//		free(row_pointers);
		ri.Hunk_FreeTempMemory( row_pointers );
	return writeData.bufferUsed;
}
#endif


static void R_MME_SaveShot( mmeShot_t *shot, int width, int height, float fps, qboolean gammaCorrect, byte *inBuf ) {
	mmeShotFormat_t format;
	char *extension;
	char *outBuf;
	int outSize;
	char fileName[MAX_OSPATH];

	// gamma correct
	if ( gammaCorrect ) {
		//R_GammaCorrect( inBuf, width * height * 4 );
	}
	format = shot->format;
	switch (format) {
	case mmeShotFormatJPG:
		extension = "jpg";
		break;
	case mmeShotFormatTGA:
		/* Seems hardly any program can handle grayscale tga, switching to png */
		if (shot->type == mmeShotTypeGray) {
			format = mmeShotFormatPNG;
			extension = "png";
		} else {
			extension = "tga";
		}
		break;
	case mmeShotFormatPNG:
		extension = "png";
		break;
	case mmeShotFormatAVI:
		//mmeAviShot( &shot->avi, shot->name, shot->type, width, height, fps, inBuf );
		extension = "png";	//to have no problem with ioq3 avi and cgame demos capture
		return;
	}

	if (shot->counter < 0) {
		shot->counter = 0;
		while (shot->counter < 1000000000) {
			Com_sprintf( fileName, sizeof(fileName), "screenshots/%s.%010d.%s", shot->name, shot->counter, extension);
//			if (!FS_FileExists( fileName ))
			if (!ri.FS_FileExists( fileName ))
				break;
			shot->counter++;
		}
	}

	Com_sprintf( fileName,	sizeof(fileName), "screenshots/%s.%010d.%s", shot->name, shot->counter, extension );
	shot->counter++;

	outSize = width * height * 4;
	outBuf = (char *)ri.Hunk_AllocateTempMemory( outSize );
	switch ( format ) {
	case mmeShotFormatJPG:	//tried it but it worked wrong
		outSize = SaveJPG( mme_jpegQuality->integer, width, height, shot->type, inBuf, (byte *)outBuf, outSize );
		break;
	case mmeShotFormatTGA:
		outSize = SaveTGA( mme_tgaCompression->integer, width, height, shot->type, inBuf, (byte *)outBuf, outSize );
		break;
#ifdef USE_PNG
	case mmeShotFormatPNG:
		outSize = SavePNG( mme_pngCompression->integer, width, height, shot->type, inBuf, (byte *)outBuf, outSize );
		break;
#endif
	default:
		outSize = 0;
	}
	if (outSize)
		ri.FS_WriteFile( fileName, outBuf, outSize );
	ri.Hunk_FreeTempMemory( outBuf );
}


static void accumCreateMultiply( void ) {
	float	blurBase[256];
	int		blurEntries[256];
	float	blurHalf = 0.5f * (shotData.blurTotal - 1 );
	float	strength = 128;
	float	bestStrength = 0;
	int		passes, bestTotal = 0;
	int		i;
	
	if (blurHalf <= 0)
		return;

	if (!Q_stricmp(mme_blurType->string, "gaussian")) {
		for (i = 0; i < shotData.blurTotal; i++) {
			double xVal = ((i - blurHalf) / blurHalf) * 3;
			double expVal = exp( - (xVal * xVal) / 2);
			double sqrtVal = 1.0f / sqrt( 2 * M_PI);
			blurBase[i] = sqrtVal * expVal;
		}
	} else if (!Q_stricmp(mme_blurType->string, "triangle")) {
		for (i = 0; i < shotData.blurTotal; i++) {
			if ( i <= blurHalf )
				blurBase[i] = 1 + i;
			else
				blurBase[i] = 1 + (shotData.blurTotal - 1 - i);
		}
	} else {
		int mulDelta = (256 << 10) / shotData.blurTotal;
		int mulIndex = 0;
		for (i = 0; i < shotData.blurTotal; i++) {
			int lastMul = mulIndex & ~((1 << 10) -1);
			mulIndex += mulDelta;
			blurBase[i] = (mulIndex - lastMul) >> 10;
		}
	}
	/* Check for best 256 match */
	for (passes = 32;passes >0;passes--) {
		int total = 0;
		for (i = 0; i < shotData.blurTotal; i++) 
			total += strength * blurBase[i];
		if (total > 256) {
			strength *= (256.0 / total);
		} else if (total < 256) {
			if ( total > bestTotal) {
				bestTotal = total;
				bestStrength = strength;
			}
			strength *= (256.0 / total); 
		} else {
			bestTotal = total;
			bestStrength = strength;
			break;
		}
	}
	for (i = 0; i < shotData.blurTotal; i++) {
		blurEntries[i] = bestStrength * blurBase[i];
	}
	for (i = 0; i < shotData.blurTotal; i++) {
		int a = blurEntries[i];
		shotData.blurMultiply[i] = _mm_set_pi16( a, a, a, a);
	}
	_mm_empty();
}


static void accumCreateMultiplyStereo( void ) {
	float	blurBase[256];
	int		blurEntries[256];
	float	blurHalf = 0.5f * (shotDataStereo.blurTotal - 1 );
	float	strength = 128;
	float	bestStrength = 0;
	int		passes, bestTotal = 0;
	int		i;
	
	if (blurHalf <= 0)
		return;

	if (!Q_stricmp(mme_blurType->string, "gaussian")) {
		for (i = 0; i < shotDataStereo.blurTotal; i++) {
			double xVal = ((i - blurHalf) / blurHalf) * 3;
			double expVal = exp( - (xVal * xVal) / 2);
			double sqrtVal = 1.0f / sqrt( 2 * M_PI);
			blurBase[i] = sqrtVal * expVal;
		}
	} else if (!Q_stricmp(mme_blurType->string, "triangle")) {
		for (i = 0; i < shotDataStereo.blurTotal; i++) {
			if ( i <= blurHalf )
				blurBase[i] = 1 + i;
			else
				blurBase[i] = 1 + (shotDataStereo.blurTotal - 1 - i);
		}
	} else {
		int mulDelta = (256 << 10) / shotDataStereo.blurTotal;
		int mulIndex = 0;
		for (i = 0; i < shotDataStereo.blurTotal; i++) {
			int lastMul = mulIndex & ~((1 << 10) -1);
			mulIndex += mulDelta;
			blurBase[i] = (mulIndex - lastMul) >> 10;
		}
	}
	/* Check for best 256 match */
	for (passes = 32;passes >0;passes--) {
		int total = 0;
		for (i = 0; i < shotDataStereo.blurTotal; i++) 
			total += strength * blurBase[i];
		if (total > 256) {
			strength *= (256.0 / total);
		} else if (total < 256) {
			if ( total > bestTotal) {
				bestTotal = total;
				bestStrength = strength;
			}
			strength *= (256.0 / total); 
		} else {
			bestTotal = total;
			bestStrength = strength;
			break;
		}
	}
	for (i = 0; i < shotDataStereo.blurTotal; i++) {
		blurEntries[i] = bestStrength * blurBase[i];
	}
	for (i = 0; i < shotDataStereo.blurTotal; i++) {
		int a = blurEntries[i];
		shotDataStereo.blurMultiply[i] = _mm_set_pi16( a, a, a, a);
	}
	_mm_empty();
}


static void accumClearMultiply( __m64 *writer, const __m64 *reader, __m64 *multp, int count ) {
	 __m64 readVal, zeroVal, work0, work1, multiply;
	 int i;
	 multiply = *multp;
	 zeroVal = _mm_setzero_si64();
	 /* Add 2 pixels in a loop */
	 for (i = count / 2 ; i>0 ; i--) {
		 readVal = *reader++;
		 work0 = _mm_unpacklo_pi8( readVal, zeroVal );
		 work1 = _mm_unpackhi_pi8( readVal, zeroVal );
		 work0 = _mm_mullo_pi16( work0, multiply );
		 work1 = _mm_mullo_pi16( work1, multiply );
		 writer[0] = work0;
		 writer[1] = work1;
		 writer += 2;
	 }
	 _mm_empty();
}


static void accumAddMultiply( __m64 *writer, const __m64 *reader, __m64 *multp, int count ) {
	 __m64 readVal, zeroVal, work0, work1, multiply;
	 int i;
	 multiply = *multp;
	 zeroVal = _mm_setzero_si64();
	 /* Add 2 pixels in a loop */
	 for (i = count / 2 ; i>0 ; i--) {
		 readVal = *reader++;
		 work0 = _mm_unpacklo_pi8( readVal, zeroVal );
		 work1 = _mm_unpackhi_pi8( readVal, zeroVal );
		 work0 = _mm_mullo_pi16( work0, multiply );
		 work1 = _mm_mullo_pi16( work1, multiply );
		 writer[0] = _mm_add_pi16( writer[0], work0 );
		 writer[1] = _mm_add_pi16( writer[1], work1 );
		 writer += 2;
	 }
	 _mm_empty();
}


static void accumShift( const __m64 *reader,  __m64 *writer, int count ) {
	__m64 work0, work1, work2, work3;
	int i;
	/* Handle 4 pixels at once */
	for (i = count /4;i>0;i--) {
		work0 = _mm_srli_pi16 (reader[0], 8);
		work1 = _mm_srli_pi16 (reader[1], 8);
		work2 = _mm_srli_pi16 (reader[2], 8);
		work3 = _mm_srli_pi16 (reader[3], 8);
//		_mm_stream_pi( writer + 0, _mm_packs_pu16( work0, work1 ));
//		_mm_stream_pi( writer + 1, _mm_packs_pu16( work2, work3 ));
		writer[0] = _mm_packs_pu16( work0, work1 );
		writer[1] = _mm_packs_pu16( work2, work3 );
		writer+=2;
		reader+=4;
	}
	_mm_empty();
}

#define MME_STRING( s ) # s


void R_MME_CheckCvars( void ) {
	int pixelCount, newBlur;
	static int oldmbtc = -1;

	pixelCount = glConfig.vidHeight * glConfig.vidWidth;
	if (mme_blurType->modificationCount != oldmbtc) {
		oldmbtc = mme_blurType->modificationCount;
		accumCreateMultiply();
	}
	if (mme_blurFrames->integer > BLURMAX) {
		ri.Cvar_Set( "mme_blurFrames", va( "%d", BLURMAX) );
	} else if (mme_blurFrames->integer < 0) {
		ri.Cvar_Set( "mme_blurFrames", "0" );
	}

	if (mme_blurOverlap->integer > BLURMAX ) {
		ri.Cvar_Set( "mme_blurOverlap", va( "%d", BLURMAX) );
	} else if (mme_blurOverlap->integer < 0 ) {
		ri.Cvar_Set( "mme_blurOverlap", "0");
	}

	newBlur = mme_blurFrames->integer + mme_blurOverlap->integer ;

	if ((newBlur != shotData.blurTotal || pixelCount != shotData.pixelCount || shotData.overlapTotal != mme_blurOverlap->integer) 
		&& !shotData.allocFailed ) {
		workUsed = 0;

		shotData.blurTotal = newBlur;
		if ( newBlur ) {
			shotData.accumAlign = (__m64 *)(workAlign + workUsed);
			workUsed += pixelCount * sizeof( __m64 );
			workUsed = (workUsed + 7) & ~7;
			if ( workUsed > workSize) {
				ri.Error( ERR_FATAL, "Failed to allocate %d bytes from the mme_workMegs buffer\n", workUsed );
				shotData.allocFailed = qtrue;
				goto alloc_Skip;
			}
			accumCreateMultiply();
		}
		shotData.overlapTotal = mme_blurOverlap->integer;
		if ( shotData.overlapTotal ) {
			shotData.overlapAlign = (__m64 *)(workAlign + workUsed);
			workUsed += shotData.overlapTotal * pixelCount * 4;
			workUsed = (workUsed + 7) & ~7;
			if ( workUsed > workSize) {
				ri.Error( ERR_FATAL, "Failed to allocate %d bytes from the mme_workMegs buffer\n", workUsed );
				shotData.allocFailed = qtrue;
				goto alloc_Skip;
			}
		}
		shotData.overlapIndex = 0;
		shotData.blurIndex = 0;
	}
alloc_Skip:
	shotData.pixelCount = pixelCount;
}


void R_MME_CheckCvarsStereo( void ) {
	int pixelCount, newBlur;
	static int oldmbtc = -1;

	pixelCount = glConfig.vidHeight * glConfig.vidWidth;
	if (mme_blurType->modificationCount != oldmbtc) {
		oldmbtc = mme_blurType->modificationCount;
		accumCreateMultiplyStereo();
	}
	if (mme_blurFrames->integer > BLURMAX) {
		ri.Cvar_Set( "mme_blurFrames", va( "%d", BLURMAX) );
	} else if (mme_blurFrames->integer < 0) {
		ri.Cvar_Set( "mme_blurFrames", "0" );
	}

	if (mme_blurOverlap->integer > BLURMAX ) {
		ri.Cvar_Set( "mme_blurOverlap", va( "%d", BLURMAX) );
	} else if (mme_blurOverlap->integer < 0 ) {
		ri.Cvar_Set( "mme_blurOverlap", "0");
	}

	newBlur = mme_blurFrames->integer + mme_blurOverlap->integer ;

	if ((newBlur != shotDataStereo.blurTotal || pixelCount != shotDataStereo.pixelCount || shotDataStereo.overlapTotal != mme_blurOverlap->integer) 
		&& !shotDataStereo.allocFailed ) {
		workUsed2 = 0;

		shotDataStereo.blurTotal = newBlur;
		if ( newBlur ) {
			shotDataStereo.accumAlign = (__m64 *)(workAlign2 + workUsed2);
			workUsed2 += pixelCount * sizeof( __m64 );
			workUsed2 = (workUsed2 + 7) & ~7;
			if ( workUsed2 > workSize2) {
				ri.Error( ERR_FATAL, "Failed to allocate %d bytes from the mme_workMegs buffer\n", workUsed2 );
				shotDataStereo.allocFailed = qtrue;
				goto alloc_Skip;
			}
			accumCreateMultiplyStereo();
		}
		shotDataStereo.overlapTotal = mme_blurOverlap->integer;
		if ( shotDataStereo.overlapTotal ) {
			shotDataStereo.overlapAlign = (__m64 *)(workAlign2 + workUsed2);
			workUsed2 += shotDataStereo.overlapTotal * pixelCount * 4;
			workUsed2 = (workUsed2 + 7) & ~7;
			if ( workUsed2 > workSize2) {
				ri.Error( ERR_FATAL, "Failed to allocate %d bytes from the mme_workMegs buffer\n", workUsed2 );
				shotDataStereo.allocFailed = qtrue;
				goto alloc_Skip;
			}
		}
		shotDataStereo.overlapIndex = 0;
		shotDataStereo.blurIndex = 0;
	}
alloc_Skip:
	shotDataStereo.pixelCount = pixelCount;
}


float my_zFar = 0.0f;
float my_zNear = 0.0f;

//extern void (WINAPI * trueglFrustum)(GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble);
//extern void (WINAPI * trueglMatrixMode)( GLenum mode );
//extern void (WINAPI * trueglLoadMatrixf)( const GLfloat *mat );
/*
void WINAPI my_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	my_zFar = zFar;
	trueglFrustum( left, right, bottom, top, zNear, zFar );
}

GLenum my_mode = GL_MODELVIEW;

void WINAPI my_glMatrixMode( GLenum mode )
{
	my_mode = mode;
	trueglMatrixMode( mode );
}

void WINAPI my_glLoadMatrixf( const GLfloat *mat )
{
	float x = mat[10]; // = (-zFar - zNear) / (zFar - zNear)
	float y=  mat[14]; // = -2 * zFar * zNear / depth
	if( my_mode == GL_PROJECTION )
	{
		my_zFar = y / ( x + 1 );
		my_zNear = y / ( x - 1 );
	}
	trueglLoadMatrixf( mat );
}
*/

void R_MME_TakeShot( void ) {
	byte *outAlloc;
	__m64 *outAlign;
	/*qboolean doGamma;*/

	if ( !shotData.shot.take )
		return;


	/*doGamma = ( mme_screenShotGamma->integer || (tr.overbrightBits > 0) ) && glConfig.deviceSupportsGamma;*/
	R_MME_CheckCvars();

//	trap_TrueMalloc( (void **)&outAlloc, shotData.pixelCount * 4 + 8);
	outAlloc = (byte*)ri.Hunk_AllocateTempMemory( shotData.pixelCount * 4 + 8 );
	outAlign = (__m64 *)((((int)(outAlloc))+7) & ~7);

	if (mme_saveShot->integer && shotData.blurTotal /*&& !shotData.allocFailed*/) {
		if ( shotData.overlapTotal ) {
			int lapIndex = shotData.overlapIndex % shotData.overlapTotal;
			shotData.overlapIndex++;
			/* First frame in a sequence, fill the buffer with the last frames */
			if (shotData.blurIndex == 0) {
				int i, index;
				index = lapIndex;
				accumClearMultiply( shotData.accumAlign, shotData.overlapAlign + (index * shotData.pixelCount/2), shotData.blurMultiply + 0, shotData.pixelCount );
				for (i = 1; i < shotData.overlapTotal; i++) {
					index = (index + 1 ) % shotData.overlapTotal;
					accumAddMultiply( shotData.accumAlign, shotData.overlapAlign + (index * shotData.pixelCount/2), shotData.blurMultiply + i, shotData.pixelCount );
				}
				shotData.blurIndex = shotData.overlapTotal;
			}
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, (byte *)(shotData.overlapAlign + (lapIndex * shotData.pixelCount/2)) ); 
			/*if ( doGamma && mme_blurGamma->integer ) {
				R_GammaCorrect( (byte *)outAlign, glConfig.vidWidth * glConfig.vidHeight * 4 );
			}*/
			accumAddMultiply( shotData.accumAlign, shotData.overlapAlign + (lapIndex * shotData.pixelCount/2), shotData.blurMultiply + shotData.blurIndex, shotData.pixelCount );
			shotData.blurIndex++;
		} else {
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, (byte *)outAlign ); 
			/*if ( doGamma && mme_blurGamma->integer ) {
				R_GammaCorrect( (byte *) outAlign, glConfig.vidWidth * glConfig.vidHeight * 4 );
			}*/
			if (shotData.blurIndex == 0) {
				accumClearMultiply( shotData.accumAlign, outAlign, shotData.blurMultiply + 0, shotData.pixelCount );
			} else {
				accumAddMultiply( shotData.accumAlign, outAlign, shotData.blurMultiply + shotData.blurIndex, shotData.pixelCount );
			}
			shotData.blurIndex++;
		}

		if (shotData.blurIndex >= shotData.blurTotal) {
			shotData.blurIndex = 0;
			accumShift( shotData.accumAlign, outAlign, shotData.pixelCount );
			R_MME_SaveShot( &shotData.shot.main, glConfig.vidWidth, glConfig.vidHeight, shotData.shot.fps, qfalse /*doGamma && !mme_blurGamma->integer*/, (byte *)outAlign );
		}
	} else {
		if (mme_saveStencil->integer) {
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, outAlign ); 
			R_MME_SaveShot( &shotData.shot.stencil, glConfig.vidWidth, glConfig.vidHeight, shotData.shot.fps, (qboolean)0, (byte *)outAlign );
		}
		if ( mme_saveDepth->integer && mme_depthRange->value > 0 ) {
			float focusStart, focusEnd, focusMul;
			float zBase, zAdd, zRange;
			int i;
			float zFar, zNear;
			float zMax = 0.0f;

			focusStart = mme_depthFocus->value - mme_depthRange->value;
			focusEnd = mme_depthFocus->value + mme_depthRange->value;
			focusMul = 255.0f / (2 * mme_depthRange->value);

			qglDepthRange( 0.0f, 1.0f );
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_DEPTH_COMPONENT, GL_FLOAT, outAlign ); 
			
			
//			trap_R_GetDistanceCull( &zFar );
			zFar = my_zFar;
			zNear = my_zNear;//CG_Cvar_Get( "r_znear" );
			
			zRange = zFar - zNear;
            zBase = ( zFar + zNear ) / zRange;
			zAdd =  ( 2 * zFar * zNear ) / zRange;

//			CG_Printf("zFar: %f     my_zFar: %f\n", zFar, my_zFar);
//			CG_Printf("zNear: %f     my_zNear: %f\n", zNear, my_zNear);
			
			/* Could probably speed this up a bit with SSE but frack it for now */
			for (i=0;i<shotData.pixelCount;i++) {
				int outVal;
				/* Read from the 0 - 1 depth */
				float zVal = ((float *)outAlign)[i];
				if( zVal > zMax ) zMax = zVal;
				
				/* Back to the original -1 to 1 range */
				zVal = zVal * 2.0f - 1.0f;
				/* Back to the original z values */
				zVal = zAdd / ( zBase - zVal );
				/* Clip and scale the range that's been selected */
				if (zVal <= focusStart)
					outVal = 0;
				else if (zVal >= focusEnd)
					outVal = 255;
				else 
					outVal = (zVal - focusStart) * focusMul;
				((byte *)outAlign)[i] = outVal;
			}
			R_MME_SaveShot( &shotData.shot.depth, glConfig.vidWidth, glConfig.vidHeight, shotData.shot.fps, (qboolean)0, (byte *)outAlign );
		}
		if ( mme_saveShot->integer ) {
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, outAlign ); 
			R_MME_SaveShot( &shotData.shot.main, glConfig.vidWidth, glConfig.vidHeight, shotData.shot.fps, qfalse/*doGamma*/, (byte *)outAlign );
		}

	}
//	trap_TrueFree( (void **)&outAlloc );
//	free( (void **)&outAlloc );
	ri.Hunk_FreeTempMemory( outAlloc );
	shotData.shot.take = qfalse;
}


void R_MME_TakeShotStereo( void ) {
	byte *outAlloc;
	__m64 *outAlign;
	/*qboolean doGamma;*/

	if ( !shotDataStereo.shot.take )
		return;


	/*doGamma = ( mme_screenShotGamma->integer || (tr.overbrightBits > 0) ) && glConfig.deviceSupportsGamma;*/
	R_MME_CheckCvarsStereo();

//	trap_TrueMalloc( (void **)&outAlloc, shotDataStereo.pixelCount * 4 + 8);
	outAlloc = (byte*)ri.Hunk_AllocateTempMemory( shotDataStereo.pixelCount * 4 + 8 );
	outAlign = (__m64 *)((((int)(outAlloc))+7) & ~7);

	if (mme_saveShot->integer && shotDataStereo.blurTotal /*&& !shotDataStereo.allocFailed*/) {
		if ( shotDataStereo.overlapTotal ) {
			int lapIndex = shotDataStereo.overlapIndex % shotDataStereo.overlapTotal;
			shotDataStereo.overlapIndex++;
			/* First frame in a sequence, fill the buffer with the last frames */
			if (shotDataStereo.blurIndex == 0) {
				int i, index;
				index = lapIndex;
				accumClearMultiply( shotDataStereo.accumAlign, shotDataStereo.overlapAlign + (index * shotDataStereo.pixelCount/2), shotDataStereo.blurMultiply + 0, shotDataStereo.pixelCount );
				for (i = 1; i < shotDataStereo.overlapTotal; i++) {
					index = (index + 1 ) % shotDataStereo.overlapTotal;
					accumAddMultiply( shotDataStereo.accumAlign, shotDataStereo.overlapAlign + (index * shotDataStereo.pixelCount/2), shotDataStereo.blurMultiply + i, shotDataStereo.pixelCount );
				}
				shotDataStereo.blurIndex = shotDataStereo.overlapTotal;
			}
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, (byte *)(shotDataStereo.overlapAlign + (lapIndex * shotDataStereo.pixelCount/2)) ); 
			/*if ( doGamma && mme_blurGamma->integer ) {
				R_GammaCorrect( (byte *)outAlign, glConfig.vidWidth * glConfig.vidHeight * 4 );
			}*/
			accumAddMultiply( shotDataStereo.accumAlign, shotDataStereo.overlapAlign + (lapIndex * shotDataStereo.pixelCount/2), shotDataStereo.blurMultiply + shotDataStereo.blurIndex, shotDataStereo.pixelCount );
			shotDataStereo.blurIndex++;
		} else {
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, (byte *)outAlign ); 
			/*if ( doGamma && mme_blurGamma->integer ) {
				R_GammaCorrect( (byte *) outAlign, glConfig.vidWidth * glConfig.vidHeight * 4 );
			}*/
			if (shotDataStereo.blurIndex == 0) {
				accumClearMultiply( shotDataStereo.accumAlign, outAlign, shotDataStereo.blurMultiply + 0, shotDataStereo.pixelCount );
			} else {
				accumAddMultiply( shotDataStereo.accumAlign, outAlign, shotDataStereo.blurMultiply + shotDataStereo.blurIndex, shotDataStereo.pixelCount );
			}
			shotDataStereo.blurIndex++;
		}

		if (shotDataStereo.blurIndex >= shotDataStereo.blurTotal) {
			shotDataStereo.blurIndex = 0;
			accumShift( shotDataStereo.accumAlign, outAlign, shotDataStereo.pixelCount );
			R_MME_SaveShot( &shotDataStereo.shot.main, glConfig.vidWidth, glConfig.vidHeight, shotDataStereo.shot.fps, qfalse /*doGamma && !mme_blurGamma->integer*/, (byte *)outAlign );
		}
	} else {
		if (mme_saveStencil->integer) {
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, outAlign ); 
			R_MME_SaveShot( &shotDataStereo.shot.stencil, glConfig.vidWidth, glConfig.vidHeight, shotDataStereo.shot.fps, (qboolean)0, (byte *)outAlign );
		}
		if ( mme_saveDepth->integer && mme_depthRange->value > 0 ) {
			float focusStart, focusEnd, focusMul;
			float zBase, zAdd, zRange;
			int i;
			float zFar, zNear;
			float zMax = 0.0f;

			focusStart = mme_depthFocus->value - mme_depthRange->value;
			focusEnd = mme_depthFocus->value + mme_depthRange->value;
			focusMul = 255.0f / (2 * mme_depthRange->value);

			qglDepthRange( 0.0f, 1.0f );
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_DEPTH_COMPONENT, GL_FLOAT, outAlign ); 
			
			
//			trap_R_GetDistanceCull( &zFar );
			zFar = my_zFar;
			zNear = my_zNear;//CG_Cvar_Get( "r_znear" );
			
			zRange = zFar - zNear;
            zBase = ( zFar + zNear ) / zRange;
			zAdd =  ( 2 * zFar * zNear ) / zRange;

//			CG_Printf("zFar: %f     my_zFar: %f\n", zFar, my_zFar);
//			CG_Printf("zNear: %f     my_zNear: %f\n", zNear, my_zNear);
			
			/* Could probably speed this up a bit with SSE but frack it for now */
			for (i=0;i<shotDataStereo.pixelCount;i++) {
				int outVal;
				/* Read from the 0 - 1 depth */
				float zVal = ((float *)outAlign)[i];
				if( zVal > zMax ) zMax = zVal;
				
				/* Back to the original -1 to 1 range */
				zVal = zVal * 2.0f - 1.0f;
				/* Back to the original z values */
				zVal = zAdd / ( zBase - zVal );
				/* Clip and scale the range that's been selected */
				if (zVal <= focusStart)
					outVal = 0;
				else if (zVal >= focusEnd)
					outVal = 255;
				else 
					outVal = (zVal - focusStart) * focusMul;
				((byte *)outAlign)[i] = outVal;
			}
			R_MME_SaveShot( &shotDataStereo.shot.depth, glConfig.vidWidth, glConfig.vidHeight, shotDataStereo.shot.fps, (qboolean)0, (byte *)outAlign );
		}
		if ( mme_saveShot->integer ) {
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, outAlign ); 
			R_MME_SaveShot( &shotDataStereo.shot.main, glConfig.vidWidth, glConfig.vidHeight, shotDataStereo.shot.fps, qfalse/*doGamma*/, (byte *)outAlign );
		}

	}
//	trap_TrueFree( (void **)&outAlloc );
//	free( (void **)&outAlloc );
	ri.Hunk_FreeTempMemory( outAlloc );
	shotDataStereo.shot.take = qfalse;
}


void R_MME_CaptureShotCmd( const void *data ) {
	const captureCommand_t *cmd = (const captureCommand_t *)data;
	static int mssfmc = -1;

	if (!cmd->name[0])
		return;// (const void *)(cmd + 1);

	shotData.shot.take = qtrue;
	shotData.shot.fps = cmd->fps;
	if (strcmp( cmd->name, shotData.shot.main.name) || mme_screenShotFormat->modificationCount != mssfmc ) {
	//	Also clear the buffer surfaces
		shotData.blurIndex = 0;
		if ( workAlign )
			Com_Memset( workAlign, 0, workUsed );
		Com_sprintf( shotData.shot.main.name, sizeof( shotData.shot.main.name ), "%s", cmd->name );
		Com_sprintf( shotData.shot.depth.name, sizeof( shotData.shot.depth.name ), "%s.depth", cmd->name );
		Com_sprintf( shotData.shot.stencil.name, sizeof( shotData.shot.stencil.name ), "%s.stencil", cmd->name );
		
		mssfmc = mme_screenShotFormat->modificationCount;

		if (!Q_stricmp(mme_screenShotFormat->string, "jpg")) {
			shotData.shot.main.format = mmeShotFormatJPG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "tga")) {
			shotData.shot.main.format = mmeShotFormatTGA;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "png")) {
			shotData.shot.main.format = mmeShotFormatPNG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "avi")) {
			shotData.shot.main.format = mmeShotFormatAVI;
		} else {
			shotData.shot.main.format = mmeShotFormatTGA;
		}
		
//		if (shotData.shot.main.format != mmeShotFormatAVI) {
			shotData.shot.depth.format = mmeShotFormatPNG;
			shotData.shot.stencil.format = mmeShotFormatPNG;
//		} else {
//			shotData.shot.depth.format = mmeShotFormatAVI;
//			shotData.shot.stencil.format = mmeShotFormatAVI;
//		}

		shotData.shot.main.type = mmeShotTypeRGB;
		shotData.shot.main.counter = -1;
		shotData.shot.depth.type = mmeShotTypeGray;
		shotData.shot.depth.counter = -1;
		shotData.shot.stencil.type = mmeShotTypeGray;
		shotData.shot.stencil.counter = -1;	
	}
	return;// (const void *)(cmd + 1);	
}


void R_MME_CaptureShotCmdStereo( const void *data ) {
	const captureCommand_t *cmd = (const captureCommand_t *)data;
	static int mssfmc = -1;

	if (!cmd->name[0])
		return;// (const void *)(cmd + 1);

	shotDataStereo.shot.take = qtrue;
	shotDataStereo.shot.fps = cmd->fps;
	if (strcmp( cmd->name, shotDataStereo.shot.main.name) || mme_screenShotFormat->modificationCount != mssfmc ) {
	//	Also clear the buffer surfaces
		shotDataStereo.blurIndex = 0;
		if ( workAlign2 )
			Com_Memset( workAlign2, 0, workUsed2 );
		Com_sprintf( shotDataStereo.shot.main.name, sizeof( shotDataStereo.shot.main.name ), "%s", cmd->name );
		Com_sprintf( shotDataStereo.shot.depth.name, sizeof( shotDataStereo.shot.depth.name ), "%s.depth", cmd->name );
		Com_sprintf( shotDataStereo.shot.stencil.name, sizeof( shotDataStereo.shot.stencil.name ), "%s.stencil", cmd->name );
		
		mssfmc = mme_screenShotFormat->modificationCount;

		if (!Q_stricmp(mme_screenShotFormat->string, "jpg")) {
			shotDataStereo.shot.main.format = mmeShotFormatJPG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "tga")) {
			shotDataStereo.shot.main.format = mmeShotFormatTGA;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "png")) {
			shotDataStereo.shot.main.format = mmeShotFormatPNG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "avi")) {
			shotDataStereo.shot.main.format = mmeShotFormatAVI;
		} else {
			shotDataStereo.shot.main.format = mmeShotFormatTGA;
		}
		
//		if (shotDataStereo.shot.main.format != mmeShotFormatAVI) {
			shotDataStereo.shot.depth.format = mmeShotFormatPNG;
			shotDataStereo.shot.stencil.format = mmeShotFormatPNG;
//		} else {
//			shotDataStereo.shot.depth.format = mmeShotFormatAVI;
//			shotDataStereo.shot.stencil.format = mmeShotFormatAVI;
//		}

		shotDataStereo.shot.main.type = mmeShotTypeRGB;
		shotDataStereo.shot.main.counter = -1;
		shotDataStereo.shot.depth.type = mmeShotTypeGray;
		shotDataStereo.shot.depth.counter = -1;
		shotDataStereo.shot.stencil.type = mmeShotTypeGray;
		shotDataStereo.shot.stencil.counter = -1;	
	}
	return;// (const void *)(cmd + 1);	
}


void R_MME_Init(void) {
//	mme_worldShader->modified = qtrue;

	if (!workAlloc) {
		workSize = mme_workMegs->integer;
		if (workSize < 64)
			workSize = 64;
		if (workSize > 512)
			workSize = 512;
		workSize *= 1024 * 1024;
		workAlloc = (char *)calloc( workSize + 16, 1 );
		if (!workAlloc) {
			ri.Printf(PRINT_ALL, "Failed to allocate %d bytes for mme work buffer\n", workSize );
			return;
		}
		Com_Memset( workAlloc, 0, workSize + 16 );
		workAlign = (char *)(((int)workAlloc + 15) & ~15);
	}

	if (!workAlloc2) {
		workSize2 = mme_workMegs->integer;
		if (workSize2 < 64)
			workSize2 = 64;
		if (workSize2 > 512)
			workSize2 = 512;
		workSize2 *= 1024 * 1024;
		workAlloc2 = (char *)calloc( workSize2 + 16, 1 );
		if (!workAlloc2) {
			ri.Printf(PRINT_ALL, "Failed to allocate %d bytes for mme work buffer\n", workSize2 );
			return;
		}
		Com_Memset( workAlloc2, 0, workSize2 + 16 );
		workAlign2 = (char *)(((int)workAlloc2 + 15) & ~15);
	}
}


void R_MME_Shutdown(void)
{
	if (workAlloc) {
//		trap_TrueFree( (void **)&workAlloc );
//		free( (void **)&workAlloc );
//		ri.Hunk_FreeTempMemory( workAlloc );
//		Z_Free( workAlloc );
	}
	if (workAlloc2) {
		//moo?
	}
}


void R_Screenshot_PNG(int x, int y, int width, int height, char *fileName)
{
	static int curCount = 0;
//	static byte *buffer;
	captureCommand_t myCmd;
	myCmd.commandId = 0;
	Q_strncpyz( myCmd.name, mme_screenshotName->string, sizeof( myCmd.name ) );
//	myCmd.fps = cg_blendScreenshots.integer;
	myCmd.fps = 0; // cg_blendScreenshots was 0 in pugmod, never changed
	R_MME_CaptureShotCmd( (const void *)&myCmd );
	R_MME_TakeShot();
	
	/*if( cg_blendScreenshots.integer )
	{
		int factor = CG_Cvar_Int( "cl_avidemo" ) / cg_blendScreenshots.integer;
		byte *tmpBuffer;
		// read / average stuff
		if( !buffer )
		{
			trap_TrueMalloc( (void **)&buffer, cgs.glconfig.vidWidth*cgs.glconfig.vidHeight*3 );
			memset( buffer, 0, cgs.glconfig.vidWidth*cgs.glconfig.vidHeight*3 );
		}
		trap_TrueMalloc( (void **)&tmpBuffer, cgs.glconfig.vidWidth*cgs.glconfig.vidHeight*3 );
		if( !buffer || !tmpBuffer )
		{
			CG_Printf("out of memory!\n");
			return;
		}
		glReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, tmpBuffer );
		for( i=0; i<cgs.glconfig.vidWidth*cgs.glconfig.vidHeight*3; i++ )
		{
			buffer[i] = ( buffer[i] * curCount / ( curCount + 1 ) ) + ( tmpBuffer[i] / ( curCount + 1 ) );
		}
		curCount++;
		if( curCount >= factor )
		{
			pngWrite( fileName, width, height, (unsigned char *)buffer );
			trap_TrueFree( (void **)&buffer );
			buffer = curCount = 0;
		}
		trap_TrueFree( (void **)&tmpBuffer );
	} else {
		trap_TrueMalloc( (void **)&buffer, cgs.glconfig.vidWidth*cgs.glconfig.vidHeight*3 );
		glReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer );
		pngWrite( fileName, width, height, (unsigned char *)buffer );
		trap_TrueFree( (void **)&buffer );
	}*/
}

void R_Screenshot_PNG_Stereo(int x, int y, int width, int height, char *fileName)
{
	static int curCount = 0;
//	static byte *buffer;
	captureCommand_t myCmd;
	myCmd.commandId = 0;
//	Q_strncpyz( myCmd.name, mme_screenshotName->string, sizeof( myCmd.name ) );
	Com_sprintf( myCmd.name, sizeof( myCmd.name ), "%s.stereo", mme_screenshotName->string );
//	myCmd.fps = cg_blendScreenshots.integer;
	myCmd.fps = 0; // cg_blendScreenshots was 0 in pugmod, never changed
	R_MME_CaptureShotCmdStereo( (const void *)&myCmd );
	R_MME_TakeShotStereo();
}
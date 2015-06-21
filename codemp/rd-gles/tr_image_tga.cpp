#include "../renderer/tr_common.h"
/*
struct TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
};
*/
bool LoadTGAPalletteImage ( const char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*buf_p;
	byte	*buffer;
	TargaHeader	targa_header;
	byte	*dataStart;

	*pic = NULL;

	//
	// load the file
	//
	ri->FS_ReadFile ( ( char * ) name, (void **)&buffer);
	if (!buffer) {
		return false;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	targa_header.colormap_index = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormap_length = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.y_origin = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.width = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.height = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type!=1 )
	{
		Com_Error (ERR_DROP, "LoadTGAPalletteImage: Only type 1 (uncompressed pallettised) TGA images supported\n");
	}

	if ( targa_header.colormap_type == 0 )
	{
		Com_Error( ERR_DROP, "LoadTGAPalletteImage: colormaps ONLY supported\n" );
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	*pic = (unsigned char *) Z_Malloc (numPixels, TAG_TEMP_WORKSPACE, qfalse );
	if (targa_header.id_length != 0)
	{
		buf_p += targa_header.id_length;  // skip TARGA image comment
	}
	dataStart = buf_p + (targa_header.colormap_length * (targa_header.colormap_size / 4));
	memcpy(*pic, dataStart, numPixels);
	ri->FS_FreeFile (buffer);

	return true;
}

// My TGA loader...
//
//---------------------------------------------------
#pragma pack(push,1)
typedef struct TGAHeader_s {
	byte	byIDFieldLength;	// must be 0
	byte	byColourmapType;	// 0 = truecolour, 1 = paletted, else bad
	byte	byImageType;		// 1 = colour mapped (palette), uncompressed, 2 = truecolour, uncompressed, else bad
	word	w1stColourMapEntry;	// must be 0
	word	wColourMapLength;	// 256 for 8-bit palettes, else 0 for true-colour
	byte	byColourMapEntrySize; // 24 for 8-bit palettes, else 0 for true-colour
	word	wImageXOrigin;		// ignored
	word	wImageYOrigin;		// ignored
	word	wImageWidth;		// in pixels
	word	wImageHeight;		// in pixels
	byte	byImagePlanes;		// bits per pixel	(8 for paletted, else 24 for true-colour)
	byte	byScanLineOrder;	// Image descriptor bytes
								// bits 0-3 = # attr bits (alpha chan)
								// bits 4-5 = pixel order/dir
								// bits 6-7 scan line interleave (00b=none,01b=2way interleave,10b=4way)
} TGAHeader_t;
#pragma pack(pop)


// *pic == pic, else NULL for failed.
//
//  returns false if found but had a format error, else true for either OK or not-found (there's a reason for this)
//

void LoadTGA ( const char *name, byte **pic, int *width, int *height)
{
	char sErrorString[1024];
	bool bFormatErrors = false;

	// these don't need to be declared or initialised until later, but the compiler whines that 'goto' skips them.
	//
	byte *pRGBA = NULL;
	byte *pOut	= NULL;
	byte *pIn	= NULL;


	*pic = NULL;

#define TGA_FORMAT_ERROR(blah) {sprintf(sErrorString,blah); bFormatErrors = true; goto TGADone;}
//#define TGA_FORMAT_ERROR(blah) Com_Error( ERR_DROP, blah );

	//
	// load the file
	//
	byte *pTempLoadedBuffer = 0;
	ri->FS_ReadFile ( ( char * ) name, (void **)&pTempLoadedBuffer);
	if (!pTempLoadedBuffer) {
		return;
	}

	TGAHeader_t *pHeader = (TGAHeader_t *) pTempLoadedBuffer;

	if (pHeader->byColourmapType!=0)
	{
		TGA_FORMAT_ERROR("LoadTGA: colourmaps not supported\n" );
	}

	if (pHeader->byImageType != 2 && pHeader->byImageType != 3 && pHeader->byImageType != 10)
	{
		TGA_FORMAT_ERROR("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RLE-RGB) images supported\n");
	}

	if (pHeader->w1stColourMapEntry != 0)
	{
		TGA_FORMAT_ERROR("LoadTGA: colourmaps not supported\n" );
	}

	if (pHeader->wColourMapLength !=0 && pHeader->wColourMapLength != 256)
	{
		TGA_FORMAT_ERROR("LoadTGA: ColourMapLength must be either 0 or 256\n" );
	}

	if (pHeader->byColourMapEntrySize != 0 && pHeader->byColourMapEntrySize != 24)
	{
		TGA_FORMAT_ERROR("LoadTGA: ColourMapEntrySize must be either 0 or 24\n" );
	}

	if ( ( pHeader->byImagePlanes != 24 && pHeader->byImagePlanes != 32) && (pHeader->byImagePlanes != 8 && pHeader->byImageType != 3))
	{
		TGA_FORMAT_ERROR("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n");
	}

	if ((pHeader->byScanLineOrder&0x30)!=0x00 &&
		(pHeader->byScanLineOrder&0x30)!=0x10 &&
		(pHeader->byScanLineOrder&0x30)!=0x20 &&
		(pHeader->byScanLineOrder&0x30)!=0x30
		)
	{
		TGA_FORMAT_ERROR("LoadTGA: ScanLineOrder must be either 0x00,0x10,0x20, or 0x30\n");
	}



	// these last checks are so i can use ID's RLE-code. I don't dare fiddle with it or it'll probably break...
	//
	if ( pHeader->byImageType == 10)
	{
		if ((pHeader->byScanLineOrder & 0x30) != 0x00)
		{
			TGA_FORMAT_ERROR("LoadTGA: RLE-RGB Images (type 10) must be in bottom-to-top format\n");
		}
		if (pHeader->byImagePlanes != 24 && pHeader->byImagePlanes != 32)	// probably won't happen, but avoids compressed greyscales?
		{
			TGA_FORMAT_ERROR("LoadTGA: RLE-RGB Images (type 10) must be 24 or 32 bit\n");
		}
	}

	// now read the actual bitmap in...
	//
	// Image descriptor bytes
	// bits 0-3 = # attr bits (alpha chan)
	// bits 4-5 = pixel order/dir
	// bits 6-7 scan line interleave (00b=none,01b=2way interleave,10b=4way)
	//
	int iYStart,iXStart,iYStep,iXStep;

	switch(pHeader->byScanLineOrder & 0x30)
	{
		default:	// default case stops the compiler complaining about using uninitialised vars
		case 0x00:					//	left to right, bottom to top

			iXStart = 0;
			iXStep  = 1;

			iYStart = pHeader->wImageHeight-1;
			iYStep  = -1;

			break;

		case 0x10:					//  right to left, bottom to top

			iXStart = pHeader->wImageWidth-1;
			iXStep  = -1;

			iYStart = pHeader->wImageHeight-1;
			iYStep	= -1;

			break;

		case 0x20:					//  left to right, top to bottom

			iXStart = 0;
			iXStep  = 1;

			iYStart = 0;
			iYStep  = 1;

			break;

		case 0x30:					//  right to left, top to bottom

			iXStart = pHeader->wImageWidth-1;
			iXStep  = -1;

			iYStart = 0;
			iYStep  = 1;

			break;
	}

	// feed back the results...
	//
	if (width)
		*width = pHeader->wImageWidth;
	if (height)
		*height = pHeader->wImageHeight;

	pRGBA	= (byte *) Z_Malloc (pHeader->wImageWidth * pHeader->wImageHeight * 4, TAG_TEMP_WORKSPACE, qfalse);
	*pic	= pRGBA;
	pOut	= pRGBA;
	pIn		= pTempLoadedBuffer + sizeof(*pHeader);

	// I don't know if this ID-thing here is right, since comments that I've seen are at the end of the file,
	//	with a zero in this field. However, may as well...
	//
	if (pHeader->byIDFieldLength != 0)
		pIn += pHeader->byIDFieldLength;	// skip TARGA image comment

	byte red,green,blue,alpha;

	if ( pHeader->byImageType == 2 || pHeader->byImageType == 3 )	// RGB or greyscale
	{
		for (int y=iYStart, iYCount=0; iYCount<pHeader->wImageHeight; y+=iYStep, iYCount++)
		{
			pOut = pRGBA + y * pHeader->wImageWidth *4;
			for (int x=iXStart, iXCount=0; iXCount<pHeader->wImageWidth; x+=iXStep, iXCount++)
			{
				switch (pHeader->byImagePlanes)
				{
					case 8:
						blue	= *pIn++;
						green	= blue;
						red		= blue;
						*pOut++ = red;
						*pOut++ = green;
						*pOut++ = blue;
						*pOut++ = 255;
						break;

					case 24:
						blue	= *pIn++;
						green	= *pIn++;
						red		= *pIn++;
						*pOut++ = red;
						*pOut++ = green;
						*pOut++ = blue;
						*pOut++ = 255;
						break;

					case 32:
						blue	= *pIn++;
						green	= *pIn++;
						red		= *pIn++;
						alpha	= *pIn++;
						*pOut++ = red;
						*pOut++ = green;
						*pOut++ = blue;
						*pOut++ = alpha;
						break;

					default:
						assert(0);	// if we ever hit this, someone deleted a header check higher up
						TGA_FORMAT_ERROR("LoadTGA: Image can only have 8, 24 or 32 planes for RGB/greyscale\n");
						break;
				}
			}
		}
	}
	else
	if (pHeader->byImageType == 10)   // RLE-RGB
	{
		// I've no idea if this stuff works, I normally reject RLE targas, but this is from ID's code
		//	so maybe I should try and support it...
		//
		byte packetHeader, packetSize, j;

		for (int y = pHeader->wImageHeight-1; y >= 0; y--)
		{
			pOut = pRGBA + y * pHeader->wImageWidth *4;
			for (int x=0; x<pHeader->wImageWidth;)
			{
				packetHeader = *pIn++;
				packetSize   = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80)         // run-length packet
				{
					switch (pHeader->byImagePlanes)
					{
						case 24:

							blue	= *pIn++;
							green	= *pIn++;
							red		= *pIn++;
							alpha	= 255;
							break;

						case 32:

							blue	= *pIn++;
							green	= *pIn++;
							red		= *pIn++;
							alpha	= *pIn++;
							break;

						default:
							assert(0);	// if we ever hit this, someone deleted a header check higher up
							TGA_FORMAT_ERROR("LoadTGA: RLE-RGB can only have 24 or 32 planes\n");
							break;
					}

					for (j=0; j<packetSize; j++)
					{
						*pOut++	= red;
						*pOut++	= green;
						*pOut++	= blue;
						*pOut++	= alpha;
						x++;
						if (x == pHeader->wImageWidth)  // run spans across rows
						{
							x = 0;
							if (y > 0)
								y--;
							else
								goto breakOut;
							pOut = pRGBA + y * pHeader->wImageWidth * 4;
						}
					}
				}
				else
				{	// non run-length packet

					for (j=0; j<packetSize; j++)
					{
						switch (pHeader->byImagePlanes)
						{
							case 24:

								blue	= *pIn++;
								green	= *pIn++;
								red		= *pIn++;
								*pOut++ = red;
								*pOut++ = green;
								*pOut++ = blue;
								*pOut++ = 255;
								break;

							case 32:
								blue	= *pIn++;
								green	= *pIn++;
								red		= *pIn++;
								alpha	= *pIn++;
								*pOut++ = red;
								*pOut++ = green;
								*pOut++ = blue;
								*pOut++ = alpha;
								break;

							default:
								assert(0);	// if we ever hit this, someone deleted a header check higher up
								TGA_FORMAT_ERROR("LoadTGA: RLE-RGB can only have 24 or 32 planes\n");
								break;
						}
						x++;
						if (x == pHeader->wImageWidth)  // pixel packet run spans across rows
						{
							x = 0;
							if (y > 0)
								y--;
							else
								goto breakOut;
							pOut = pRGBA + y * pHeader->wImageWidth * 4;
						}
					}
				}
			}
			breakOut:;
		}
	}

TGADone:

	ri->FS_FreeFile (pTempLoadedBuffer);

	if (bFormatErrors)
	{
		Com_Error( ERR_DROP, "%s( File: \"%s\" )\n",sErrorString,name);
	}
}
/*
============================================================================
MME additions
============================================================================
*/
static int SaveTGA_RLERGBA(byte *out, const int image_width, const int image_height, const void* image_buffer ) {
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
				out[dataSize++] = pixel >> 24;
			} else {
				int blockStart = dataSize++;
				/* Write some raw pixels no matter what*/
				out[dataSize++] = pixel >> 16;
				out[dataSize++] = pixel >> 8;
				out[dataSize++] = pixel >> 0;
				out[dataSize++] = pixel >> 24;
				pixel = inBuf[1];
				for ( i = 1;i < block;i++) {
					if ( inBuf[i+1] == pixel)
						break;
					out[dataSize++] = pixel >> 16;
					out[dataSize++] = pixel >> 8;
					out[dataSize++] = pixel >> 0;
					out[dataSize++] = pixel >> 24;
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
static int SaveTGA_RLERGB(byte *out, const int image_width, const int image_height, const void* image_buffer ) {
	int y;
	const byte *inBuf = ( const byte*)image_buffer;
	int dataSize = 0;
	
	for (y=0; y < image_height;y++) {
		int left = image_width;
		/* Prepare for the first block and write the first pixel */
		while ( left > 0 ) {
			/* Search for a block of similar pixels */
			int i, block = left > 128 ? 128 : left;
			unsigned int pixel = inBuf[0] | (inBuf[1] << 8) | (inBuf[2] << 16);
			/* Check for rle pixels */
			for ( i = 1;i < block;i++) {
				unsigned int testPixel = inBuf[i*3+0] | (inBuf[i*3+1] << 8) | (inBuf[i*3+2] << 16);
				if ( testPixel != pixel)
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
				pixel = inBuf[3] | (inBuf[4] << 8) | (inBuf[5] << 16);
				for ( i = 1;i < block;i++) {
					unsigned int testPixel = inBuf[i*3+3] | (inBuf[i*3+4] << 8) | (inBuf[i*3+5] << 16);
					if ( testPixel == pixel)
						break;
					out[dataSize++] = pixel >> 16;
					out[dataSize++] = pixel >> 8;
					out[dataSize++] = pixel >> 0;
					pixel = testPixel;
				}
				out[blockStart] = i - 1;
			}
			inBuf += i*3;
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
	int filesize = 18;	// header is here by default
	int bitDepth;
	byte tgaFormat;

	// Fill in the header
	switch (image_type) {
	case mmeShotTypeGray:
		tgaFormat = 3;
		bitDepth = 8;
		break;
	case mmeShotTypeRGB:
		bitDepth = 24;
		tgaFormat = 2;
		break;
	case mmeShotTypeRGBA:
		bitDepth = 32;
		tgaFormat = 2;
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
	out_buffer[16] = bitDepth;
	//Alpha/Attribute bits whatever
	out_buffer[17] = bitDepth == 32 ? 8 : 0;

	// Fill output buffer
	if (!image_compressed) { // Plain memcpy
		byte *buftemp = out_buffer+filesize;
		switch (image_type) {
		case mmeShotTypeRGBA:
			for (i = 0; i < imagePixels; i++ ) {
				/* Also handle the RGBA to BGRA conversion here */
				*buftemp++ = image_buffer[2];
				*buftemp++ = image_buffer[1];
				*buftemp++ = image_buffer[0];
				*buftemp++ = image_buffer[3];
				image_buffer += 4;
			}
			filesize += image_width*image_height*4;
			break;
		case mmeShotTypeRGB:
			for (i = 0; i < imagePixels; i++ ) {
				/* Also handle the RGB to BGR conversion here */
				*buftemp++ = image_buffer[2];
				*buftemp++ = image_buffer[1];
				*buftemp++ = image_buffer[0];
				image_buffer += 3;
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
		case mmeShotTypeRGBA:
			filesize += SaveTGA_RLERGBA(out_buffer+filesize, image_width, image_height, image_buffer );
			break;
		case mmeShotTypeGray:
			filesize += SaveTGA_RLEGray(out_buffer+filesize, image_width, image_height, image_buffer );
			break;
		}
	}
	return filesize;
}
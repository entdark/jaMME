/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_font.c
// 
//
// The font system uses FreeType 2.x to render TrueType fonts for use within the game.
// As of this writing ( Nov, 2000 ) Team Arena uses these fonts for all of the ui and 
// about 90% of the cgame presentation. A few areas of the CGAME were left uses the old 
// fonts since the code is shared with standard Q3A.
//
// If you include this font rendering code in a commercial product you MUST include the
// following somewhere with your product, see www.freetype.org for specifics or changes.
// The Freetype code also uses some hinting techniques that MIGHT infringe on patents 
// held by apple so be aware of that also.
//
// As of Q3A 1.25+ and Team Arena, we are shipping the game with the font rendering code
// disabled. This removes any potential patent issues and it keeps us from having to 
// distribute an actual TrueTrype font which is 1. expensive to do and 2. seems to require
// an act of god to accomplish. 
//
// What we did was pre-render the fonts using FreeType ( which is why we leave the FreeType
// credit in the credits ) and then saved off the glyph data and then hand touched up the 
// font bitmaps so they scale a bit better in GL.
//
// There are limitations in the way fonts are saved and reloaded in that it is based on 
// point size and not name. So if you pre-render Helvetica in 18 point and Impact in 18 point
// you will end up with a single 18 point data file and image set. Typically you will want to 
// choose 3 sizes to best approximate the scaling you will be doing in the ui scripting system
// 
// In the UI Scripting code, a scale of 1.0 is equal to a 48 point font. In Team Arena, we
// use three or four scales, most of them exactly equaling the specific rendered size. We 
// rendered three sizes in Team Arena, 12, 16, and 20. 
//
// To generate new font data you need to go through the following steps.
// 1. delete the fontImage_x_xx.tga files and fontImage_xx.dat files from the fonts path.
// 2. in a ui script, specificy a font, smallFont, and bigFont keyword with font name and 
//    point size. the original TrueType fonts must exist in fonts at this point.
// 3. run the game, you should see things normally.
// 4. Exit the game and there will be three dat files and at least three tga files. The 
//    tga's are in 256x256 pages so if it takes three images to render a 24 point font you 
//    will end up with fontImage_0_24.tga through fontImage_2_24.tga
// 5. You will need to flip the tga's in Photoshop as the tga output code writes them upside
//    down.
// 6. In future runs of the game, the system looks for these images and data files when a s
//    specific point sized font is rendered and loads them for use. 
// 7. Because of the original beta nature of the FreeType code you will probably want to hand
//    touch the font bitmaps.
// 
// Currently a define in the project turns on or off the FreeType code which is currently 
// defined out. To pre-render new fonts you need enable the define ( BUILD_FREETYPE ) and 
// uncheck the exclude from build check box in the FreeType2 area of the Renderer project. 


#ifdef BUILD_FREETYPE
#include "tr_local.h"
#include "../qcommon/qcommon.h"

#include <ft2build.h>
#include <freetype/fttypes.h>
#include <freetype/fterrors.h>
#include <freetype/ftsystem.h>
#include <freetype/ftimage.h>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>

#define FONT_SIZE 256
#define DPI 72

#define _FLOOR(x)  ((x) & -64)
#define _CEIL(x)   (((x)+63) & -64)
#define _TRUNC(x)  ((x) >> 6)

FT_Library ftLibrary = NULL; 
#pragma comment (lib, "freetype.lib")

#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND 127
#define GLYPHS_PER_FONT GLYPH_END - GLYPH_START + 1
typedef struct {
	uint16_t height;       // number of scan lines
	int16_t top;          // top of glyph in buffer
	int16_t bottom;       // bottom of glyph in buffer
	int16_t xSkip;        // x adjustment
	uint16_t imageWidth;   // width of actual image
	uint16_t imageHeight;  // height of actual image
	int16_t pitch;        // width for copying
	float s;          // x offset in image where glyph starts
	float t;          // y offset in image where glyph starts
	float s2;
	float t2;
	qhandle_t glyph;  // handle to the shader with the glyph
	char shaderName[32];
} freeTypeGlyphInfo_t;

typedef struct {
	freeTypeGlyphInfo_t glyphs[0x110000];
	char glyphsValid[0x1100];
	int pointSize;
	float glyphScale;
	char name[MAX_QPATH];
	FT_Face face;
} fontInfo_t;

#define MAX_FONTS 6
static int registeredFontCount = 0;
static fontInfo_t registeredFont[MAX_FONTS];
static freeTypeGlyphInfo_t emptyGlyph;

typedef struct freeTypeFont_s {
	char fontName[MAX_QPATH];
	fontInfo_t fontInfo;
	struct freeTypeFont_s *next;
} freeTypeFont_t;

static freeTypeFont_t *freeTypeFontList;

void R_GetGlyphInfo(FT_GlyphSlot glyph, int *left, int *right, int *width, int *top, int *bottom, int *height, int *pitch) {

  *left  = _FLOOR( glyph->metrics.horiBearingX );
  *right = _CEIL( glyph->metrics.horiBearingX + glyph->metrics.width );
  *width = _TRUNC(*right - *left);
    
  *top    = _CEIL( glyph->metrics.horiBearingY );
  *bottom = _FLOOR( glyph->metrics.horiBearingY - glyph->metrics.height );
  *height = _TRUNC( *top - *bottom );
  *pitch  = ( qtrue ? (*width+3) & -4 : (*width+7) >> 3 );
}


FT_Bitmap *R_RenderGlyph(FT_GlyphSlot glyph, freeTypeGlyphInfo_t* glyphOut) {

  FT_Bitmap  *bit2;
  int left, right, width, top, bottom, height, pitch, size;

  R_GetGlyphInfo(glyph, &left, &right, &width, &top, &bottom, &height, &pitch);

  if ( glyph->format == FT_GLYPH_FORMAT_OUTLINE ) {
    size   = pitch*height; 

    bit2 = (FT_Bitmap *)Z_Malloc(sizeof(FT_Bitmap), TAG_FONT, qfalse);

    bit2->width      = width;
    bit2->rows       = height;
    bit2->pitch      = pitch;
    bit2->pixel_mode = FT_PIXEL_MODE_GRAY;
    //bit2->pixel_mode = ft_pixel_mode_mono;
    bit2->buffer     = (unsigned char *)Z_Malloc(size, TAG_FONT, qtrue);
    bit2->num_grays = 256;

    FT_Outline_Translate( &glyph->outline, -left, -bottom );

    FT_Outline_Get_Bitmap( ftLibrary, &glyph->outline, bit2 );

    glyphOut->height = height;
    glyphOut->pitch = pitch;
    glyphOut->top = (glyph->metrics.horiBearingY >> 6) + 1;
    glyphOut->bottom = bottom;
    
    return bit2;
  }
  else if ( glyph->format == FT_GLYPH_FORMAT_BITMAP ) {
    size   = glyph->bitmap.pitch*glyph->bitmap.rows;

    bit2 = (FT_Bitmap *)Z_Malloc(sizeof(FT_Bitmap), TAG_FONT, qfalse);

    bit2->width      = glyph->bitmap.width;
    bit2->rows       = glyph->bitmap.rows;
    bit2->pitch      = glyph->bitmap.pitch;
    bit2->pixel_mode = glyph->bitmap.pixel_mode;
    //bit2->pixel_mode = ft_pixel_mode_mono;
    bit2->buffer     = (unsigned char *)Z_Malloc(size, TAG_FONT, qtrue);
    bit2->num_grays = 256;

    Com_Memcpy( bit2->buffer, glyph->bitmap.buffer, size );

    //FT_Outline_Translate( &glyph->outline, -left, -bottom );

    //FT_Outline_Get_Bitmap( ftLibrary, &glyph->outline, bit2 );

    glyphOut->height = glyph->bitmap.rows;
    glyphOut->pitch = glyph->bitmap.pitch;
    glyphOut->top = (glyph->metrics.horiBearingY >> 6) + 1;
    glyphOut->bottom = bottom;
    
    return bit2;
  }
  else {
    ri.Printf(PRINT_ALL, "Non-outline fonts are not supported\n");
  }
  return NULL;
}

void WriteTGA (char *filename, byte *data, int width, int height) {
	byte	*buffer;
	int		i, c;

	buffer = (byte *)Z_Malloc(width*height*4 + 18, TAG_GENERAL, qfalse);
	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width&255;
	buffer[13] = width>>8;
	buffer[14] = height&255;
	buffer[15] = height>>8;
	buffer[16] = 32;	// pixel size

	// swap rgb to bgr
	c = 18 + width * height * 4;
	for (i=18 ; i<c ; i+=4)
	{
		buffer[i] = data[i-18+2];		// blue
		buffer[i+1] = data[i-18+1];		// green
		buffer[i+2] = data[i-18+0];		// red
		buffer[i+3] = data[i-18+3];		// alpha
	}

	ri.FS_WriteFile(filename, buffer, c);

	//f = fopen (filename, "wb");
	//fwrite (buffer, 1, c, f);
	//fclose (f);

	Z_Free (buffer);
}

static freeTypeGlyphInfo_t *RE_ConstructGlyphInfo(int imageSize, unsigned char *imageOut, int *xOut, int *yOut, int *maxHeight, FT_Face face, FT_ULong c, qboolean calcHeight) {
  int i, scaledX, scaledY;
  static freeTypeGlyphInfo_t glyph;
  unsigned char *src, *dst;
  float scaled_width, scaled_height;
  FT_Bitmap *bitmap = NULL;

  Com_Memset(&glyph, 0, sizeof(freeTypeGlyphInfo_t));
  // make sure everything is here
  if (face != NULL) {
    FT_Load_Glyph(face, FT_Get_Char_Index( face, c), FT_LOAD_DEFAULT | FT_LOAD_COLOR );
	//if (c == 0x1F973)
  FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    bitmap = R_RenderGlyph(face->glyph, &glyph);
    if (bitmap) {
      glyph.xSkip = (face->glyph->metrics.horiAdvance >> 6) + 1;
    } else {
      return &glyph;
    }

    if (glyph.height > *maxHeight) {
      *maxHeight = glyph.height;
    }

    if (calcHeight) {
      Z_Free(bitmap->buffer);
      Z_Free(bitmap);
      return &glyph;
    }

/*
    // need to convert to power of 2 sizes so we do not get 
    // any scaling from the gl upload
  	for (scaled_width = 1 ; scaled_width < glyph.pitch ; scaled_width<<=1)
	  	;
  	for (scaled_height = 1 ; scaled_height < glyph.height ; scaled_height<<=1)
	  	;
*/

    scaled_width = bitmap->width;
    scaled_height = bitmap->rows;

	scaledX = *xOut;
	scaledY = *yOut;
    // we need to make sure we fit
    if (*xOut + scaled_width + 1 >= (imageSize)-1) {
      if (*yOut + (*maxHeight+*maxHeight) + 1 >= (imageSize)-1) {
        *yOut = -1;
        *xOut = -1;
        Z_Free(bitmap->buffer);
        Z_Free(bitmap);
        return &glyph;
      } else {
        *xOut = 0;
        *yOut += (*maxHeight) + 1;
      }
    } else if (*yOut + (*maxHeight+*maxHeight) + 1 >= (imageSize)-1) {
      *yOut = -1;
      *xOut = -1;
      Z_Free(bitmap->buffer);
      Z_Free(bitmap);
      return &glyph;
    }


    src = bitmap->buffer;
    dst = imageOut + (*yOut * imageSize*4) + *xOut*4;

		if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO) {
			for (i = 0; i < glyph.height; i++) {
				int j;
				unsigned char *_src = src;
				unsigned char *_dst = dst;
				unsigned char mask = 0x80;
				unsigned char val = *_src;
				for (j = 0; j < glyph.pitch; j++) {
					if (mask == 0x80) {
						val = *_src++;
					}
					if (val & mask) {
						*_dst = 0xff;
					}
					mask >>= 1;
        
					if ( mask == 0 ) {
						mask = 0x80;
					}
					_dst++;
				}

				src += glyph.pitch;
				dst += imageSize;

			}
		} else if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY) {
	    for (i = 0; i < bitmap->rows; i++) {
				int j;
				for (j = 0; j < bitmap->width; j++) {
					dst[j*4+0] = 255;
					dst[j*4+1] = 255;
					dst[j*4+2] = 255;
					dst[j*4+3] = src[j];
				}
			  src += glyph.pitch;
				dst += imageSize*4;
	    }
		} else if (bitmap->pixel_mode == FT_PIXEL_MODE_BGRA) {
	    for (i = 0; i < bitmap->rows; i++) {
				int j;
				for (j = 0; j < bitmap->width; j++) {
					dst[j*4+0] = src[j*4+2];
					dst[j*4+1] = src[j*4+1];
					dst[j*4+2] = src[j*4+0];
					dst[j*4+3] = src[j*4+3];
				}
//				Com_Memcpy(dst, src, glyph.pitch*4);
			  src += glyph.pitch;
				dst += imageSize*4;
	    }
		}

    // we now have an 8 bit per pixel grey scale bitmap 
    // that is width wide and pf->ftSize->metrics.y_ppem tall

    glyph.imageHeight = scaled_height;
    glyph.imageWidth = scaled_width;
    glyph.s = (float)*xOut / imageSize;
    glyph.t = (float)*yOut / imageSize;
    glyph.s2 = glyph.s + (float)scaled_width / imageSize;
    glyph.t2 = glyph.t + (float)scaled_height / imageSize;

    *xOut += scaled_width + 1;
  }

  Z_Free(bitmap->buffer);
  Z_Free(bitmap);

  return &glyph;
}

static int fdOffset;
static byte	*fdFile;

int readInt( void ) {
	int i = fdFile[fdOffset]+(fdFile[fdOffset+1]<<8)+(fdFile[fdOffset+2]<<16)+(fdFile[fdOffset+3]<<24);
	fdOffset += 4;
	return i;
}

typedef union {
	byte	fred[4];
	float	ffred;
} poor;

float readFloat( void ) {
	poor	me;
#if 0
//#if defined BigFloat
	me.fred[0] = fdFile[fdOffset+3];
	me.fred[1] = fdFile[fdOffset+2];
	me.fred[2] = fdFile[fdOffset+1];
	me.fred[3] = fdFile[fdOffset+0];
//#elif defined LittleFloat
#else
	me.fred[0] = fdFile[fdOffset+0];
	me.fred[1] = fdFile[fdOffset+1];
	me.fred[2] = fdFile[fdOffset+2];
	me.fred[3] = fdFile[fdOffset+3];
#endif
	fdOffset += 4;
	return me.ffred;
}

qboolean R_RegisterGlyphs(fontInfo_t *font, uint32_t c) {
	int j, k, xOut, yOut, lastStart, imageNumber;
	int scaledSize, newSize, maxHeight, left, satLevels;
	unsigned char *out, *imageBuff;
	freeTypeGlyphInfo_t *glyph;
	image_t *image;
	qhandle_t h;
	float max;
	freeTypeFont_t *freeTypeFont;
	void *faceData;
	int i, len;
	char name[1024], fontNameNoExt[1024];
	int imageSize;
	int start, end;
	float dpi = (float)DPI *(glConfig.vidHeight / (float)SCREEN_HEIGHT);

	start = c & ~0xFF;
	end = start + FONT_SIZE;

	len = ri.FS_ReadFile(font->name, &faceData);
	if (len <= 0) {
		ri.Printf(PRINT_ALL, "RE_RegisterFont: Unable to read font file\n");
		return qfalse;
	}

	// allocate on the stack first in case we fail
	if (FT_New_Memory_Face(ftLibrary, (FT_Byte *)faceData, len, 0, &font->face)) {
		ri.FS_FreeFile(faceData);
		ri.Printf(PRINT_ALL, "RE_RegisterFont: FreeType2, unable to allocate new face.\n");
		return qfalse;
	}

	if (FT_Set_Char_Size(font->face, font->pointSize << 6, font->pointSize << 6, dpi, dpi)) {
		ri.FS_FreeFile(faceData);
		ri.Printf(PRINT_ALL, "RE_RegisterFont: FreeType2, Unable to set face char size.\n");
		return qfalse;
	}

	FT_Select_Charmap(font->face, FT_ENCODING_UNICODE);

	// scale image size based on screen height, use the next higher power of two
	for (imageSize = FONT_SIZE; imageSize < (float)FONT_SIZE * (glConfig.vidHeight / (float)SCREEN_HEIGHT); imageSize <<= 1)
		;

	// do not exceed maxTextureSize
	if (imageSize > glConfig.maxTextureSize)
	{
		imageSize = glConfig.maxTextureSize;
	}

	// make a 256x256 image buffer, once it is full, register it, clean it and keep going 
	// until all glyphs are rendered

	out = (unsigned char *)Z_Malloc(imageSize*imageSize * 4, TAG_FONT, qtrue);
	if (out == NULL) {
		ri.FS_FreeFile(faceData);
		ri.Printf(PRINT_ALL, "RE_RegisterFont: Z_Malloc failure during output image creation.\n");
		return qfalse;
	}

	maxHeight = 0;

	for (i = start; i < end; i++) {
		glyph = RE_ConstructGlyphInfo(imageSize, out, &xOut, &yOut, &maxHeight, font->face, i, qtrue);
	}
	
	xOut = 0;
	yOut = 0;
	i = start;
	lastStart = i;
	imageNumber = start;
	
	COM_StripExtension(font->name, fontNameNoExt, MAX_QPATH);
	while ( i <= end) {
	    glyph = RE_ConstructGlyphInfo(imageSize, out, &xOut, &yOut, &maxHeight, font->face, i, qfalse);
		if (xOut == -1 || yOut == -1 || i == end)  {
			// ran out of room
			// we need to create an image from the bitmap, set all the handles in the glyphs to this point
			// 
			newSize = imageSize*imageSize * 4;
			imageBuff = (unsigned char *)Z_Malloc(newSize, TAG_FONT, qfalse);
			Com_Memcpy(imageBuff, out, newSize);

			Com_sprintf (name, sizeof(name), "%s_%i_%i.tga", fontNameNoExt, imageNumber++, font->pointSize);
//			if (r_saveFontData->integer) { 
//				WriteTGA(name, imageBuff, 256, 256);
//			}

			//Com_sprintf (name, sizeof(name), "fonts/fontImage_%i_%i", imageNumber++, pointSize);
			image = R_CreateImage(name, imageBuff, imageSize, imageSize, GL_RGBA, qfalse, qfalse, qfalse, GL_CLAMP);
			h = RE_RegisterShaderFromImage(name, (int *)lightmaps2d, (byte *)stylesDefault, image, qfalse);
			for (j = lastStart; j < i; j++) {
				font->glyphs[j].glyph = h;
				Q_strncpyz(font->glyphs[j].shaderName, name, sizeof(font->glyphs[j].shaderName));
			}
			lastStart = i;
			Com_Memset(out, 0, newSize);
			xOut = 0;
			yOut = 0;
			Z_Free(imageBuff);
			i++;
		} else {
			Com_Memcpy(&font->glyphs[i], glyph, sizeof(freeTypeGlyphInfo_t));
			i++;
		}
	}
	font->glyphsValid[c>>8] = qtrue;

	Z_Free(out);
	ri.FS_FreeFile(faceData);

	return qtrue;
}

int RE_RegisterFontFreeType(const char *fontName, int pointSize/*, fontInfo_t *font*/) {
	fontInfo_t *font;
	freeTypeFont_t *freeTypeFont;
	int i, len;
  char name[1024];
  float dpi = (float)DPI *(glConfig.vidHeight / (float)SCREEN_HEIGHT);											//
  float glyphScale = 72.0f / dpi; 		// change the scale to be relative to 1 based on 72 dpi ( so dpi of 144 means a scale of .5 )


	if (!fontName || !fontName[0] ) {
		ri.Printf(PRINT_ALL, "RE_RegisterFont: called with empty name\n");
		return 0;
	}

	if (pointSize <= 0) {
		pointSize = 12;
	}
	// we also need to adjust the scale based on point size relative to 48 points as the ui scaling is based on a 48 point font
//	glyphScale *= 48.0f / pointSize;

	// make sure the render thread is stopped
	R_SyncRenderThread();

	len = ri.FS_ReadFile(fontName, NULL );
	if ( len >= 0 )
		goto freetypeFont;

	if (registeredFontCount >= MAX_FONTS) {
		ri.Printf(PRINT_ALL, "RE_RegisterFont: Too many fonts registered already.\n");
		return 0;
	}

	Com_sprintf(name, sizeof(name), "fonts/fontImage_%i.dat",pointSize);
	for (i = 0; i < registeredFontCount; i++) {
		if (Q_stricmp(name, registeredFont[i].name) == 0) {
//			Com_Memcpy(font, &registeredFont[i], sizeof(fontInfo_t));
			return i+1;
		}
	}

/*	len = ri.FS_ReadFile(name, NULL);
	if (len == sizeof(fontInfo_t)) {
		ri.FS_ReadFile(name, &faceData);
		fdOffset = 0;
		fdFile = (byte *)faceData;
		for(i=0; i<GLYPHS_PER_FONT; i++) {
			font->glyphs[i].height		= readInt();
			font->glyphs[i].top			= readInt();
			font->glyphs[i].bottom		= readInt();
			font->glyphs[i].pitch		= readInt();
			font->glyphs[i].xSkip		= readInt();
			font->glyphs[i].imageWidth	= readInt();
			font->glyphs[i].imageHeight = readInt();
			font->glyphs[i].s			= readFloat();
			font->glyphs[i].t			= readFloat();
			font->glyphs[i].s2			= readFloat();
			font->glyphs[i].t2			= readFloat();
			font->glyphs[i].glyph		= readInt();
			Com_Memcpy(font->glyphs[i].shaderName, &fdFile[fdOffset], 32);
			fdOffset += 32;
		}
		font->glyphScale = readFloat();
		Com_Memcpy(font->name, &fdFile[fdOffset], MAX_QPATH);

//		Com_Memcpy(font, faceData, sizeof(fontInfo_t));
		Q_strncpyz(font->name, name, sizeof(font->name));
		for (i = GLYPH_START; i < GLYPH_END; i++) {
			font->glyphs[i].glyph = RE_RegisterShaderNoMip(font->glyphs[i].shaderName);
		}
	  Com_Memcpy(&registeredFont[registeredFontCount++], font, sizeof(fontInfo_t));
		return;
	}*/

  if (ftLibrary == NULL) {
    ri.Printf(PRINT_ALL, "RE_RegisterFont: FreeType not initialized.\n");
    return 0;
  }

freetypeFont:

  //Check if we've already loaded this font/pointsize combination
	freeTypeFont = freeTypeFontList;
	i = 0;
	while ( freeTypeFont ) {
		if (Q_stricmp( fontName, freeTypeFont->fontName ) && freeTypeFont->fontInfo.pointSize == pointSize ) {
//			*font = freeTypeFont->fontInfo;
			return i+1;
		}
		freeTypeFont = freeTypeFont->next;
		i++;
	}

	font = &registeredFont[registeredFontCount];

	/* Register the font in the cached list */
	freeTypeFont = (freeTypeFont_t *)Z_Malloc( sizeof( *freeTypeFont ), TAG_FONT, qfalse);
	Q_strncpyz( freeTypeFont->fontName, fontName, sizeof( freeTypeFont->fontName ));
	Q_strncpyz( font->name, fontName, sizeof( font->name ));
	freeTypeFont->fontInfo = *font;
	freeTypeFont->next = freeTypeFontList;
	freeTypeFontList = freeTypeFont;
	registeredFontCount++;

	font->pointSize = pointSize;
	font->glyphScale = glyphScale;

	R_RegisterGlyphs(font, 0);


//	Com_Memcpy(&registeredFont[registeredFontCount++], &font, sizeof(fontInfo_t));

//	if (r_saveFontData->integer) { 
//		ri.FS_WriteFile(va("fonts/fontImage_%i.dat", pointSize), font, sizeof(fontInfo_t));
//	}
	return registeredFontCount;
}



void R_InitFreeType(void) {
	if (FT_Init_FreeType( &ftLibrary )) {
		ri.Printf(PRINT_ALL, "R_InitFreeType: Unable to initialize FreeType.\n");
	}
	freeTypeFontList = 0;
	registeredFontCount = 0;
	Com_Memset(&emptyGlyph, 0, sizeof(freeTypeGlyphInfo_t));
}


void R_DoneFreeType(void) {
	if (ftLibrary) {
		FT_Done_FreeType( ftLibrary );
		ftLibrary = NULL;
	}
	freeTypeFontList = 0;
	registeredFontCount = 0;
}

freeTypeGlyphInfo_t *R_GetGlyph(fontInfo_t *font, char *currentChar, char **nextChar) {
/*
	Wikipedia: https://en.wikipedia.org/wiki/UTF-32
	"Each 32-bit value in UTF-32 represents one Unicode code point
	and is exactly equal to that code point's numerical value."
*/
	uint32_t unicode = ConvertUTF8ToUTF32(currentChar, nextChar);
	if (unicode <= 0x10FFFF) {
		if (font->glyphsValid[unicode>>8] || R_RegisterGlyphs(font, unicode)) {
			return &font->glyphs[unicode];
		} else {

		}
	}
	return &emptyGlyph;
}

float FCG_Text_Width(char *text, float scale, int limit, const int handle) {
  int count,len;
	float out;
	const freeTypeGlyphInfo_t *glyph;
	float useScale;
	char *s = text;
	fontInfo_t *font = &registeredFont[handle-2];
	useScale = scale * font->glyphScale;
	out = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			int colorLen = Q_parseColorString( s, 0, tr.cTable );
			if ( colorLen ) {
				s += colorLen;
				continue;
			} else {
				glyph = R_GetGlyph(font, s, &s); // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
				out += glyph->xSkip/**cgs.widthRatioCoef*/;
//				s++;
				count++;
			}
		}
	}
	return out * useScale;
}

float FCG_Text_Height(char *text, float scale, int limit, const int handle) {
	int len, count;
	float max;
	freeTypeGlyphInfo_t *glyph;
	float useScale;
// TTimo: FIXME
//	const unsigned char *s = text;
	char *s = text;
	fontInfo_t *font = &registeredFont[handle-2];
	useScale = scale * font->glyphScale;
	max = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			int colorLen = Q_parseColorString( s, 0, tr.cTable );
			if ( colorLen ) {
				s += colorLen;
				continue;
			} else {
				glyph = R_GetGlyph(font, s, &s); // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build

				if (max < glyph->height) {
					max = glyph->height;
				}
//				s++;
				count++;
			}
		}
	}
	return max * useScale;
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h, xs, ys;
  w = width * scale;
  h = height * scale;
//  xs = (glConfig.vidWidth / (float)SCREEN_WIDTH);
//  ys = (glConfig.vidHeight / (float)SCREEN_HEIGHT);
//  CG_AdjustFrom640( &x, &y, &w, &h );
  RE_StretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void FCG_Text_Paint(float x, float y, float scale, vec4_t color, char *text, qboolean shadowed, const int handle ) {
	int len, count;
	vec4_t newColor;
	freeTypeGlyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &registeredFont[handle-2];
	useScale = scale * font->glyphScale;//*0.87f;
//	y += useScale * font->glyphs['A'].height*2.0f;
	y += FCG_Text_Height(text, scale, 0, handle)*1.3f;
	if (text) {
		char *s = text;
		RE_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
		len = strlen(text);
		count = 0;
		while (s && *s && count < len) {
			int colorLen;
			//int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			//float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			colorLen = Q_parseColorString( s, newColor, tr.cTable );
			if ( colorLen ) {
				newColor[3] = color[3];
				RE_SetColor( newColor );
				s += colorLen;
				continue;
			} else {
				glyph = R_GetGlyph(font, s, &s); // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
				float yadj = useScale * glyph->top;
				if ( shadowed ) {
					int ofs = 2;
					colorBlack[3] = newColor[3];
					RE_SetColor( colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					RE_SetColor( newColor );
				}
				RE_SetColor( NULL );
				CG_Text_PaintChar(x, y - yadj, 
					glyph->imageWidth, glyph->imageHeight,
					useScale, glyph->s,	glyph->t, glyph->s2, glyph->t2, glyph->glyph);
				x += (glyph->xSkip * useScale)/**cgs.widthRatioCoef*/;
//				s++;
				count++;
			}
		}
		RE_SetColor( NULL );
	}
}

void RE_DrawStringFreeType(const char *text, float x, float y, const float *color, const int handle, float scale) {

}


#endif
#ifndef TR_COMMON_H
#define TR_COMMON_H

#include "../renderer/tr_public.h"
#include "../rd-gles/tr_font.h"

extern refimport_t *ri;

/*
================================================================================
 Noise Generation
================================================================================
*/
// Initialize the noise generator.
void R_NoiseInit( void );

// Get random 4-component vector.
float R_NoiseGet4f( float x, float y, float z, float t );

// Get the noise time.
float GetNoiseTime( int t );

/*
================================================================================
 Image Loading
================================================================================
*/
// Initialize the image loader.
void R_ImageLoader_Init();

typedef void (*ImageLoaderFn)( const char *filename, byte **pic, int *width, int *height );

// Adds a new image loader to handle a new image type. The extension should not
// begin with a period (a full stop).
qboolean R_ImageLoader_Add( const char *extension, ImageLoaderFn imageLoader );

// Load an image from file.
void R_LoadImage( const char *shortname, byte **pic, int *width, int *height );

// Load an image from file.
// xyc: How does this differ from R_LoadImage (except it doesn't load PNG files)?
void R_LoadDataImage( const char *name, byte **pic, int *width, int *height );

// Load raw image data from pallette-colored TGA image.
bool LoadTGAPalletteImage( const char *name, byte **pic, int *width, int *height );

// Load raw image data from TGA image.
void LoadTGA( const char *name, byte **pic, int *width, int *height );

// Load raw image data from JPEG image.
void LoadJPG( const char *filename, byte **pic, int *width, int *height );

// Load raw image data from PNG image.
void LoadPNG( const char *filename, byte **data, int *width, int *height );


/*
================================================================================
 Image saving
================================================================================
*/
// Convert raw image data to JPEG format and store in buffer.
size_t RE_SaveJPGToBuffer( byte *buffer, size_t bufSize, int quality, int image_width, int image_height, byte *image_buffer, int padding );

// Save raw image data as JPEG image file.
void RE_SaveJPG( const char * filename, int quality, int image_width, int image_height, byte *image_buffer, int padding );

// Save raw image data as PNG image file.
int RE_SavePNG( const char *filename, byte *buf, size_t width, size_t height, int byteDepth );


/*
================================================================================
 Image manipulation
================================================================================
*/
// Flip an image along its y-axis.
void R_InvertImage( byte *data, int width, int height, int depth );

// Resize an image by resampling the image.
void R_Resample( byte *source, int swidth, int sheight, byte *dest, int dwidth, int dheight, int components );

//mme
int SaveJPG( int quality, int image_width, int image_height, mmeShotType_t image_type, byte *image_buffer, byte *out_buffer, int out_size );
int SaveTGA( int image_compressed, int image_width, int image_height, mmeShotType_t image_type, byte *image_buffer, byte *out_buffer, int out_size );
int SavePNG( int compresslevel, int image_width, int image_height, mmeShotType_t image_type, byte *image_buffer, byte *out_buffer, int out_size );
#endif
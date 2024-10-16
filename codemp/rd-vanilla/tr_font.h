// Filename:-	tr_font.h
//
// font support 

// This file is shared in the single and multiplayer codebases, so be CAREFUL WHAT YOU ADD/CHANGE!!!!!

#pragma once

void R_ShutdownFonts(void);
void R_InitFonts(void);
int RE_RegisterFont(const char *psName);
int RE_Font_StrLenPixels(const char *psText, const int iFontHandle, const float fScale = 1.0f);
int RE_Font_StrLenChars(const char *psText);
int RE_Font_HeightPixels(const int iFontHandle, const float fScale = 1.0f);
void RE_Font_DrawString(float ox, float oy, const char *psText, const float *rgba, const int iFontHandle, int iMaxPixelWidth, const float fScale = 1.0f);

// Dammit, I can't use this more elegant form because of !@#@!$%% VM code... (can't alter passed in ptrs, only contents of)
//
//unsigned int AnyLanguage_ReadCharFromString( const char **ppsText, qboolean *pbIsTrailingPunctuation = NULL);
//
// so instead we have to use this messier method...
//
unsigned int AnyLanguage_ReadCharFromString( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation = NULL);

qboolean Language_IsAsian(void);
qboolean Language_UsesSpaces(void);

void RE_FontRatioFix(float ratio);

/* Freetype */

void R_InitFreeType(void);
void R_DoneFreeType(void);
int RE_RegisterFontFreeType(const char *fontName, int pointSize/*, fontInfo_t *font*/);

// end

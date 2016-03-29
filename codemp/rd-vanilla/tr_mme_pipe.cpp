// Copyright (C) 2009 ent( entdark @ mail.ru )

#include "tr_mme.h"

void pipeClose( mmePipeFile_t *pipeFile ) {
    if (!pipeFile->f)
        return;
    ri.FS_PipeClose(pipeFile->f);
}

static qboolean pipeOpen(mmePipeFile_t *pipeFile, const char *name, mmeShotType_t type, int width, int height, float fps) {
    const	char *format;
    qboolean haveTag = qfalse;
    char	outBuf[512];
    int		outIndex = 0;
    int		outLeft = sizeof(outBuf) - 1;
    
    format = mme_pipeCommand->string;
    if (!format || !format[0]) {
        format = "ffmpeg -r %f -f rawvideo -pix_fmt rgb24 -s %wx%h -i - -threads 0 -preset fast -y -pix_fmt yuv420p -crf 17 -vf vflip %o.mp4 2> ffmpeglog.txt";
    }
    
    while (*format && outLeft  > 0) {
        if (haveTag) {
            char ch = *format++;
            haveTag = qfalse;
            switch (ch) {
                case 'f':		//fps
                    Com_sprintf( outBuf + outIndex, outLeft, "%.3f", fps);
                    outIndex += strlen( outBuf + outIndex );
                    break;
                case 'w':		//map
                    Com_sprintf( outBuf + outIndex, outLeft, "%d", width);
                    outIndex += strlen( outBuf + outIndex );
                    break;
                case 'h':		//map
                    Com_sprintf( outBuf + outIndex, outLeft, "%d", height);
                    outIndex += strlen( outBuf + outIndex );
                    break;
                case 'o':		//map
                    Com_sprintf( outBuf + outIndex, outLeft, name);
                    outIndex += strlen( outBuf + outIndex );
                    break;
                case '%':
                    outBuf[outIndex++] = '%';
                    break;
                default:
                    continue;
            }
            outLeft = sizeof(outBuf) - outIndex - 1;
            continue;
        }
        if (*format == '%') {
            haveTag = qtrue;
            format++;
            continue;
        }
        outBuf[outIndex++] = *format++;
        outLeft = sizeof(outBuf) - outIndex - 1;
    }
    outBuf[ outIndex ] = 0;
#ifdef _WIN32
    pipeFile->f = ri.FS_PipeOpen(outBuf, name, "wb");
#else
    pipeFile->f = ri.FS_PipeOpen(outBuf, name, "w");
#endif
    if (!pipeFile->f) {
        return qfalse;
    }
    pipeFile->fps = fps;
    pipeFile->type = type;
    Q_strncpyz(pipeFile->name, name, sizeof(pipeFile->name));
    return qtrue;
}

static qboolean pipeValid(const mmePipeFile_t *pipeFile, const char *name, mmeShotType_t type, float fps) {
    if (!pipeFile->f)
        return qfalse;
    if (pipeFile->fps != fps)
        return qfalse;
    if (pipeFile->type != type)
        return qfalse;
    if (Q_stricmp(pipeFile->name, name))
        return qfalse;
    return qtrue;
}

void mmePipeShot(mmePipeFile_t *pipeFile, const char *name, mmeShotType_t type, int width, int height, float fps, byte *inBuf) {
    byte *outBuf;
    int i, pixels, outSize;
    if (!fps)
        return;
    if (!pipeValid(pipeFile, name, type, fps)) {
        pipeClose(pipeFile);
        if (!pipeOpen(pipeFile, name, type, width, height, fps))
            return;
    }
    pixels = width*height;
    outSize = width*height*3;
    outBuf = (byte *)ri.Hunk_AllocateTempMemory(outSize + 8);
    
    switch (type) {
        case mmeShotTypeGray:
            for (i = 0;i<pixels;i++) {
                outBuf[i] = inBuf[i];
            };
            outSize = pixels;
            break;
        case mmeShotTypeRGB:
            for (i = 0;i<pixels;i++) {
                outBuf[i*3 + 0] = inBuf[i*3 + 0];
                outBuf[i*3 + 1] = inBuf[i*3 + 1];
                outBuf[i*3 + 2] = inBuf[i*3 + 2];
            }
            outSize = width * height * 3;
            break;
    }
    
    ri.FS_PipeWrite(outBuf, outSize, pipeFile->f);
    
    ri.Hunk_FreeTempMemory(outBuf);
}
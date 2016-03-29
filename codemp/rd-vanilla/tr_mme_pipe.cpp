// Copyright (C) 2009 ent( entdark @ mail.ru )

#include "tr_mme.h"

void pipeClose( mmePipeFile_t *pipeFile ) {
    if (!pipeFile->f)
        return;
    ri.FS_PipeClose(pipeFile->f);
}

static qboolean pipeOpen(mmePipeFile_t *pipeFile, const char *name, mmeShotType_t type, int width, int height, float fps) {
    char cmd[MAX_OSPATH];
    Com_sprintf(cmd, sizeof(cmd), mme_pipeCommand->string, (int)fps, width, height);
    pipeFile->f = ri.FS_PipeOpen(cmd, name, "wb");
    if (!pipeFile->f) {
        return qfalse;
    }
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
                outBuf[i*3 + 0] = inBuf[i*3 + 2];
                outBuf[i*3 + 1] = inBuf[i*3 + 1];
                outBuf[i*3 + 2] = inBuf[i*3 + 0];
            }
            outSize = width * height * 3;
            break;
    }
    
    outSize = (outSize + 9) & ~1;	//make sure we align on 2 byte boundary, hurray M$
    ri.FS_PipeWrite(outBuf, outSize, pipeFile->f);
    
    ri.Hunk_FreeTempMemory(outBuf);
}
/* $XFree86: $ */
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Original Mesa / 3Dfx device driver (C) 1999 David Bucciarelli, by the
 * terms stated above.
 *
 * Thank you for your contribution, David!
 *
 * Please make note of the above copyright/license statement.  If you
 * contributed code or bug fixes to this code under the previous (GNU
 * Library) license and object to the new license, your code will be
 * removed at your request.  Please see the Mesa docs/COPYRIGHT file
 * for more information.
 *
 * Additional Mesa/3Dfx driver developers:
 *   Daryll Strauss <daryll@precisioninsight.com>
 *   Keith Whitwell <keith@precisioninsight.com>
 *
 * See fxapi.h for more revision/author details.
 */


/* fxtexman.c - 3Dfx VooDoo texture memory functions */


#include "fxdrv.h"
#include "fxtexman.h"
#include "fxddtex.h"


#define BAD_ADDRESS ((FxU32) -1)

int texSwaps = 0;



#ifdef TEXSANITY
static void
fubar(void)
{
}

/*
 * Sanity Check
 */
static void
sanity(fxMesaContext fxMesa)
{
    MemRange *tmp, *prev, *pos;

    prev = 0;
    tmp = fxMesa->tmFree[0];
    while (tmp) {
        if (!tmp->startAddr && !tmp->endAddr) {
            fprintf(stderr, "Textures fubar\n");
            fubar();
        }
        if (tmp->startAddr >= tmp->endAddr) {
            fprintf(stderr, "Node fubar\n");
            fubar();
        }
        if (prev && (prev->startAddr >= tmp->startAddr ||
                     prev->endAddr > tmp->startAddr)) {
            fprintf(stderr, "Sorting fubar\n");
            fubar();
        }
        prev = tmp;
        tmp = tmp->next;
    }
    prev = 0;
    tmp = fxMesa->tmFree[1];
    while (tmp) {
        if (!tmp->startAddr && !tmp->endAddr) {
            fprintf(stderr, "Textures fubar\n");
            fubar();
        }
        if (tmp->startAddr >= tmp->endAddr) {
            fprintf(stderr, "Node fubar\n");
            fubar();
        }
        if (prev && (prev->startAddr >= tmp->startAddr ||
                     prev->endAddr > tmp->startAddr)) {
            fprintf(stderr, "Sorting fubar\n");
            fubar();
        }
        prev = tmp;
        tmp = tmp->next;
    }
}
#endif


/*
 * Allocate and initialize a new MemRange struct.
 * Try to allocate it from the pool of free MemRange nodes rather than malloc.
 */
static MemRange *
fxTMNewRangeNode(fxMesaContext fxMesa, FxU32 start, FxU32 end)
{
    MemRange *result;

    if (fxMesa->tmPool) {
        result = fxMesa->tmPool;
        fxMesa->tmPool = fxMesa->tmPool->next;
    }
    else {
        result = MALLOC(sizeof(MemRange));
        if (!result) {
            /*fprintf(stderr, "fxDriver: out of memory!\n");*/
            return NULL;
        }
    }
    result->startAddr = start;
    result->endAddr = end;
    result->next = NULL;
    return result;
}


/*
 * Delete a MemRange struct.
 * We keep a linked list of free/available MemRange structs to
 * avoid extra malloc/free calls.
 */
static void
fxTMDeleteRangeNode(fxMesaContext fxMesa, MemRange *range)
{
    range->next = fxMesa->tmPool;
    fxMesa->tmPool = range;
}


/*
 * When we've run out of texture memory we have to throw out an
 * existing texture to make room for the new one.  This function
 * determins the texture to throw out.
 */
static struct gl_texture_object *
fxTMFindOldestObject(fxMesaContext fxMesa, int tmu)
{
    const GLuint bindnumber = fxMesa->texBindNumber;
    struct gl_texture_object *oldestObj, *obj, *lowestPriorityObj;
    GLfloat lowestPriority;
    GLuint oldestAge;

    oldestObj = NULL;
    oldestAge = 0;

    lowestPriority = 1.0F;
    lowestPriorityObj = NULL;

    for (obj = fxMesa->glCtx->Shared->TexObjectList; obj; obj = obj->Next) {
        tfxTexInfo *info = fxTMGetTexInfo(obj);

        if (info && info->isInTM &&
            ((info->whichTMU == tmu) || (info->whichTMU == FX_TMU_BOTH) ||
             (info->whichTMU == FX_TMU_SPLIT))) {
            GLuint age, lasttime;

            lasttime = info->lastTimeUsed;

            if (lasttime > bindnumber)
                age = bindnumber + (UINT_MAX - lasttime + 1); /* TO DO: check wrap around */
            else
                age = bindnumber - lasttime;

            if (age >= oldestAge) {
                oldestAge = age;
                oldestObj = obj;
            }

            /* examine priority */
            if (obj->Priority < lowestPriority) {
                lowestPriority = obj->Priority;
                lowestPriorityObj = obj;
            }
        }
    }

    if (lowestPriority < 1.0) {
        ASSERT(lowestPriorityObj);
        /*
        printf("discard %d pri=%f\n", lowestPriorityObj->Name, lowestPriority);
        */
        return lowestPriorityObj;
    }
    else {
        /*
        printf("discard %d age=%d\n", oldestObj->Name, oldestAge);
        */
        return oldestObj;
    }
}


/*
 * Find the address (offset?) at which we can store a new texture.
 * <tmu> is the texture unit.
 * <size> is the texture size in bytes.
 */
static FxU32
fxTMFindStartAddr(fxMesaContext fxMesa, GLint tmu, FxU32 size)
{
    MemRange *prev, *block;
    FxU32 result;
    struct gl_texture_object *obj;

    while (1) {
        prev = NULL;
        block = fxMesa->tmFree[tmu];
        while (block) {
            if (block->endAddr - block->startAddr >= size) {
                /* The texture will fit here */
                result = block->startAddr;
                block->startAddr += size;
                if (block->startAddr == block->endAddr) {
                    /* Remove this node since it's empty */
                    if (prev) {
                        prev->next = block->next;
                    }
                    else {
                        fxMesa->tmFree[tmu] = block->next;
                    }
                    fxTMDeleteRangeNode(fxMesa, block);
                }
                fxMesa->freeTexMem[tmu] -= size;
                return result;
            }
            prev = block;
            block = block->next;
        }
        /* No free space. Discard oldest */
        obj = fxTMFindOldestObject(fxMesa, tmu);
        if (!obj) {
            /*gl_problem(NULL, "fx Driver: No space for texture\n");*/
            return BAD_ADDRESS;
        }
        fxTMMoveOutTM(fxMesa, obj);
        texSwaps++;
    }
}


/*
 * Remove the given MemRange node from hardware texture memory.
 */
static void
fxTMRemoveRange(fxMesaContext fxMesa, GLint tmu, MemRange *range)
{
    MemRange *block, *prev;

    if (!range)
        return;

    if (range->startAddr == range->endAddr) {
        fxTMDeleteRangeNode(fxMesa, range);
        return;
    }
    fxMesa->freeTexMem[tmu] += range->endAddr - range->startAddr;

    /* find position in linked list to insert this MemRange node */
    prev = NULL;
    block = fxMesa->tmFree[tmu];
    while (block) {
        if (range->startAddr > block->startAddr) {
            prev = block;
            block = block->next;
        }
        else {
            break;
        }
    }

    /* Insert the free block, combine with adjacent blocks when possible */
    range->next = block;
    if (block) {
        if (range->endAddr == block->startAddr) {
            /* Combine */
            block->startAddr = range->startAddr;
            fxTMDeleteRangeNode(fxMesa, range);
            range = block;
        }
    }
    if (prev) {
        if (prev->endAddr == range->startAddr) {
            /* Combine */
            prev->endAddr = range->endAddr;
            prev->next = range->next;
            fxTMDeleteRangeNode(fxMesa, range);
        }
        else {
            prev->next = range;
        }
    }
    else {
        fxMesa->tmFree[tmu] = range;
    }
}


/*
 * Allocate space for a texture image.
 * <tmu> is the texture unit
 * <texmemsize> is the number of bytes to allocate
 */
static MemRange *
fxTMAllocTexMem(fxMesaContext fxMesa, GLint tmu, FxU32 texmemsize)
{
    FxU32 startAddr = fxTMFindStartAddr(fxMesa, tmu, texmemsize);
    if (startAddr == BAD_ADDRESS) {
        return NULL;
    }
    else {
        MemRange *range;
        range = fxTMNewRangeNode(fxMesa, startAddr, startAddr + texmemsize);
        return range;
    }
}



/*
 * Move the given texture back into hardare texture memory.
 */
void
fxTMMoveInTM_NoLock(fxMesaContext fxMesa, struct gl_texture_object *tObj,
                    GLint where)
{
    tfxTexInfo *ti = fxTMGetTexInfo(tObj);
    int i, l;
    FxU32 texmemsize;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxTMMoveInTM(%d)\n", tObj->Name);
    }

    fxMesa->stats.reqTexUpload++;

    if (!ti->validated) {
        gl_problem(NULL,
            "fx Driver: internal error in fxTMMoveInTM() -> not validated\n");
        return;  /* used to abort here */
    }

    if (ti->isInTM) {
        if (ti->whichTMU == where)
            return;
        if (where == FX_TMU_SPLIT || ti->whichTMU == FX_TMU_SPLIT) {
            fxTMMoveOutTM_NoLock(fxMesa, tObj);
        }
        else {
            if (ti->whichTMU == FX_TMU_BOTH)
                return;
            where = FX_TMU_BOTH;
        }
    }

    if (MESA_VERBOSE & (VERBOSE_DRIVER | VERBOSE_TEXTURE)) {
        fprintf(stderr,
                "fxmesa: downloading %x (%d) in texture memory in %d\n",
                (GLuint) tObj, tObj->Name, where);
    }

    ti->whichTMU = (FxU32) where;

    switch (where) {
    case FX_TMU0:
    case FX_TMU1:
        texmemsize = FX_grTexTextureMemRequired_NoLock(GR_MIPMAPLEVELMASK_BOTH,
                                                       &(ti->info));
        ti->tm[where] = fxTMAllocTexMem(fxMesa, where, texmemsize);
        if (ti->tm[where]) {
            fxMesa->stats.memTexUpload += texmemsize;

            for (i = FX_largeLodValue(ti->info), l = ti->minLevel;
                 i <= FX_smallLodValue(ti->info); i++, l++)
                FX_grTexDownloadMipMapLevel_NoLock(where,
                                                   ti->tm[where]->startAddr,
                                                   FX_valueToLod(i),
                                                   FX_largeLodLog2(ti->info),
                                                   FX_aspectRatioLog2(ti->info),
                                                   ti->info.format,
                                                   GR_MIPMAPLEVELMASK_BOTH,
                                                   ti->mipmapLevel[l].data);
        }
        break;
    case FX_TMU_SPLIT:
        texmemsize = FX_grTexTextureMemRequired_NoLock(GR_MIPMAPLEVELMASK_ODD,
                                                       &(ti->info));
        ti->tm[FX_TMU0] = fxTMAllocTexMem(fxMesa, FX_TMU0, texmemsize);
        if (ti->tm[FX_TMU0])
           fxMesa->stats.memTexUpload += texmemsize;

        texmemsize = FX_grTexTextureMemRequired_NoLock(GR_MIPMAPLEVELMASK_EVEN,
                                                       &(ti->info));
        ti->tm[FX_TMU1] = fxTMAllocTexMem(fxMesa, FX_TMU1, texmemsize);
        if (ti->tm[FX_TMU0] && ti->tm[FX_TMU1]) {
            fxMesa->stats.memTexUpload += texmemsize;

            for (i = FX_largeLodValue(ti->info), l = ti->minLevel;
                 i <= FX_smallLodValue(ti->info); i++, l++) {
                FX_grTexDownloadMipMapLevel_NoLock(GR_TMU0,
                                                  ti->tm[FX_TMU0]->startAddr,
                                                  FX_valueToLod(i),
                                                  FX_largeLodLog2(ti->info),
                                                  FX_aspectRatioLog2(ti->info),
                                                  ti->info.format,
                                                  GR_MIPMAPLEVELMASK_ODD,
                                                  ti->mipmapLevel[l].data);

                FX_grTexDownloadMipMapLevel_NoLock(GR_TMU1,
                                                  ti->tm[FX_TMU1]->startAddr,
                                                  FX_valueToLod(i),
                                                  FX_largeLodLog2(ti->info),
                                                  FX_aspectRatioLog2(ti->info),
                                                  ti->info.format,
                                                  GR_MIPMAPLEVELMASK_EVEN,
                                                  ti->mipmapLevel[l].data);
            }
        }
        break;
    case FX_TMU_BOTH:
        texmemsize = FX_grTexTextureMemRequired_NoLock(GR_MIPMAPLEVELMASK_BOTH,
                                                       &(ti->info));
        ti->tm[FX_TMU0] = fxTMAllocTexMem(fxMesa, FX_TMU0, texmemsize);
        if (ti->tm[FX_TMU0])
           fxMesa->stats.memTexUpload += texmemsize;

        texmemsize = FX_grTexTextureMemRequired_NoLock(GR_MIPMAPLEVELMASK_BOTH,
                                                       &(ti->info));
        ti->tm[FX_TMU1] = fxTMAllocTexMem(fxMesa, FX_TMU1, texmemsize);
        if (ti->tm[FX_TMU0] && ti->tm[FX_TMU1]) {
            fxMesa->stats.memTexUpload += texmemsize;

            for (i = FX_largeLodValue(ti->info), l = ti->minLevel;
                 i <= FX_smallLodValue(ti->info); i++, l++) {
                FX_grTexDownloadMipMapLevel_NoLock(GR_TMU0,
                                                  ti->tm[FX_TMU0]->startAddr,
                                                  FX_valueToLod(i),
                                                  FX_largeLodLog2(ti->info),
                                                  FX_aspectRatioLog2(ti->info),
                                                  ti->info.format,
                                                  GR_MIPMAPLEVELMASK_BOTH,
                                                  ti->mipmapLevel[l].data);

                FX_grTexDownloadMipMapLevel_NoLock(GR_TMU1,
                                                  ti->tm[FX_TMU1]->startAddr,
                                                  FX_valueToLod(i),
                                                  FX_largeLodLog2(ti->info),
                                                  FX_aspectRatioLog2(ti->info),
                                                  ti->info.format,
                                                  GR_MIPMAPLEVELMASK_BOTH,
                                                  ti->mipmapLevel[l].data);
            }
        }
        break;
    default:
        fprintf(stderr,
            "fx Driver: internal error in fxTMMoveInTM() -> wrong tmu (%d)\n",
            where);
        return;  /* used to abort here */
    }

    fxMesa->stats.texUpload++;

    ti->isInTM = GL_TRUE;
}


void
fxTMMoveInTM(fxMesaContext fxMesa, struct gl_texture_object *tObj,
             GLint where)
{
    BEGIN_BOARD_LOCK(fxMesa);
    fxTMMoveInTM_NoLock(fxMesa, tObj, where);
    END_BOARD_LOCK(fxMesa);
}


void
fxTMReloadMipMapLevel(GLcontext *ctx, struct gl_texture_object *tObj,
                      GLint level)
{
    fxMesaContext fxMesa = FX_CONTEXT(ctx);
    tfxTexInfo *ti = fxTMGetTexInfo(tObj);
    GrLOD_t lodlevel;
    GLint tmu;

    if (!ti->validated) {
        gl_problem(ctx, "internal error in fxTMReloadMipMapLevel() -> not validated\n");
        return;
    }

    tmu = (int) ti->whichTMU;
    fxTMMoveInTM(fxMesa, tObj, tmu);

    fxTexGetInfo(ctx, ti->mipmapLevel[0].width, ti->mipmapLevel[0].height,
                 &lodlevel, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

#ifdef FX_GLIDE3
    lodlevel -= level;
#else
    lodlevel += level;
#endif
    switch (tmu) {
    case FX_TMU0:
    case FX_TMU1:
        FX_grTexDownloadMipMapLevel(fxMesa, tmu,
                                    ti->tm[tmu]->startAddr,
                                    FX_valueToLod(FX_lodToValue(lodlevel)),
                                    FX_largeLodLog2(ti->info),
                                    FX_aspectRatioLog2(ti->info),
                                    ti->info.format,
                                    GR_MIPMAPLEVELMASK_BOTH,
                                    ti->mipmapLevel[level].data);
        break;
    case FX_TMU_SPLIT:
        FX_grTexDownloadMipMapLevel(fxMesa, GR_TMU0,
                                    ti->tm[GR_TMU0]->startAddr,
                                    FX_valueToLod(FX_lodToValue(lodlevel)),
                                    FX_largeLodLog2(ti->info),
                                    FX_aspectRatioLog2(ti->info),
                                    ti->info.format,
                                    GR_MIPMAPLEVELMASK_ODD,
                                    ti->mipmapLevel[level].data);

        FX_grTexDownloadMipMapLevel(fxMesa, GR_TMU1,
                                    ti->tm[GR_TMU1]->startAddr,
                                    FX_valueToLod(FX_lodToValue(lodlevel)),
                                    FX_largeLodLog2(ti->info),
                                    FX_aspectRatioLog2(ti->info),
                                    ti->info.format,
                                    GR_MIPMAPLEVELMASK_EVEN,
                                    ti->mipmapLevel[level].data);
        break;
    case FX_TMU_BOTH:
        FX_grTexDownloadMipMapLevel(fxMesa, GR_TMU0,
                                    ti->tm[GR_TMU0]->startAddr,
                                    FX_valueToLod(FX_lodToValue(lodlevel)),
                                    FX_largeLodLog2(ti->info),
                                    FX_aspectRatioLog2(ti->info),
                                    ti->info.format,
                                    GR_MIPMAPLEVELMASK_BOTH,
                                    ti->mipmapLevel[level].data);

        FX_grTexDownloadMipMapLevel(fxMesa, GR_TMU1,
                                    ti->tm[GR_TMU1]->startAddr,
                                    FX_valueToLod(FX_lodToValue(lodlevel)),
                                    FX_largeLodLog2(ti->info),
                                    FX_aspectRatioLog2(ti->info),
                                    ti->info.format,
                                    GR_MIPMAPLEVELMASK_BOTH,
                                    ti->mipmapLevel[level].data);
        break;

    default:
        fprintf(stderr,
                "fx Driver: internal error in fxTMReloadMipMapLevel() -> wrong tmu (%d)\n",
                tmu);
        break;
    }
}

#if	0
/*
 * This doesn't work.  It can't work for compressed textures.
 */
void
fxTMReloadSubMipMapLevel(GLcontext *ctx,
                         struct gl_texture_object *tObj,
                         GLint level, GLint yoffset, GLint height)
{
    fxMesaContext fxMesa = FX_CONTEXT(ctx);
    tfxTexInfo *ti = fxTMGetTexInfo(tObj);
    GrLOD_t lodlevel;
    unsigned short *data;
    GLint tmu;

    if (!ti->validated) {
       gl_problem(ctx, "fx Driver: internal error in fxTMReloadSubMipMapLevel() -> not validated\n");
       return;
    }

    tmu = (int) ti->whichTMU;
    fxTMMoveInTM(fxMesa, tObj, tmu);

    fxTexGetInfo(ctx, ti->mipmapLevel[0].width, ti->mipmapLevel[0].height,
                 &lodlevel, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    data = ti->mipmapLevel[level].data +
        yoffset * ti->mipmapLevel[level].width *
        ti->mipmapLevel[level].texelSize;

    switch (tmu) {
    case FX_TMU0:
    case FX_TMU1:
        FX_grTexDownloadMipMapLevelPartial(fxMesa, tmu,
                                           ti->tm[tmu]->startAddr,
                                           FX_valueToLod(FX_lodToValue
                                                         (lodlevel) + level),
                                           FX_largeLodLog2(ti->info),
                                           FX_aspectRatioLog2(ti->info),
                                           ti->info.format,
                                           GR_MIPMAPLEVELMASK_BOTH, data,
                                           yoffset, yoffset + height - 1);
        break;
    case FX_TMU_SPLIT:
        FX_grTexDownloadMipMapLevelPartial(fxMesa, GR_TMU0,
                                           ti->tm[FX_TMU0]->startAddr,
                                           FX_valueToLod(FX_lodToValue
                                                         (lodlevel) + level),
                                           FX_largeLodLog2(ti->info),
                                           FX_aspectRatioLog2(ti->info),
                                           ti->info.format,
                                           GR_MIPMAPLEVELMASK_ODD, data,
                                           yoffset, yoffset + height - 1);

        FX_grTexDownloadMipMapLevelPartial(fxMesa, GR_TMU1,
                                           ti->tm[FX_TMU1]->startAddr,
                                           FX_valueToLod(FX_lodToValue
                                                         (lodlevel) + level),
                                           FX_largeLodLog2(ti->info),
                                           FX_aspectRatioLog2(ti->info),
                                           ti->info.format,
                                           GR_MIPMAPLEVELMASK_EVEN, data,
                                           yoffset, yoffset + height - 1);
        break;
    case FX_TMU_BOTH:
        FX_grTexDownloadMipMapLevelPartial(fxMesa, GR_TMU0,
                                           ti->tm[FX_TMU0]->startAddr,
                                           FX_valueToLod(FX_lodToValue
                                                         (lodlevel) + level),
                                           FX_largeLodLog2(ti->info),
                                           FX_aspectRatioLog2(ti->info),
                                           ti->info.format,
                                           GR_MIPMAPLEVELMASK_BOTH, data,
                                           yoffset, yoffset + height - 1);

        FX_grTexDownloadMipMapLevelPartial(fxMesa, GR_TMU1,
                                           ti->tm[FX_TMU1]->startAddr,
                                           FX_valueToLod(FX_lodToValue
                                                         (lodlevel) + level),
                                           FX_largeLodLog2(ti->info),
                                           FX_aspectRatioLog2(ti->info),
                                           ti->info.format,
                                           GR_MIPMAPLEVELMASK_BOTH, data,
                                           yoffset, yoffset + height - 1);
        break;
    default:
        fprintf(stderr,
                "fx Driver: internal error in fxTMReloadSubMipMapLevel() -> wrong tmu (%d)\n",
                tmu);
        return;
    }
}
#endif

/*
 * Move the given texture out of hardware texture memory.
 */
void
fxTMMoveOutTM(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
    tfxTexInfo *ti = fxTMGetTexInfo(tObj);

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxTMMoveOutTM(%x (%d))\n", (GLuint) tObj,
                tObj->Name);
    }

    if (!ti->isInTM)
        return;

    switch (ti->whichTMU) {
    case FX_TMU0:
    case FX_TMU1:
        fxTMRemoveRange(fxMesa, (int) ti->whichTMU, ti->tm[ti->whichTMU]);
        break;
    case FX_TMU_SPLIT:
    case FX_TMU_BOTH:
        fxTMRemoveRange(fxMesa, FX_TMU0, ti->tm[FX_TMU0]);
        fxTMRemoveRange(fxMesa, FX_TMU1, ti->tm[FX_TMU1]);
        break;
    default:
        fprintf(stderr, "fx Driver: internal error in fxTMMoveOutTM()\n");
        return;
    }

    ti->isInTM = GL_FALSE;
    ti->whichTMU = FX_TMU_NONE;
}


/*
 * Called via glDeleteTexture to delete a texture object.
 */
void
fxTMFreeTexture(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
    tfxTexInfo *ti = fxTMGetTexInfo(tObj);
    int i;

    fxTMMoveOutTM(fxMesa, tObj);

    for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
        if (ti->mipmapLevel[i].data) {
            FREE(ti->mipmapLevel[i].data);
            ti->mipmapLevel[i].data = NULL;
        }
    }
    switch (ti->whichTMU) {
    case FX_TMU0:
    case FX_TMU1:
        fxTMDeleteRangeNode(fxMesa, ti->tm[ti->whichTMU]);
        break;
    case FX_TMU_SPLIT:
    case FX_TMU_BOTH:
        fxTMDeleteRangeNode(fxMesa, ti->tm[FX_TMU0]);
        fxTMDeleteRangeNode(fxMesa, ti->tm[FX_TMU1]);
        break;
    }
}


/*
 * Initialize texture memory.
 * We take care of one or both TMU's here.
 */
void
fxTMInit(fxMesaContext fxMesa)
{
    const char *extensions = FX_grGetString(fxMesa, GR_EXTENSION);

    fxMesa->texBindNumber = 0;
    fxMesa->tmPool = NULL;

    /* On Voodoo4 and later there's a UMA texture memory instead of
     * separate TMU0 and TMU1 segments.  We setup UMA mode here if
     * possible.
     */
    if (strstr(extensions, " TEXUMA disabled for now")) {
        FxU32 start, end;
        fxMesa->umaTexMemory = GL_TRUE;
        FX_grEnable(fxMesa, GR_TEXTURE_UMA_EXT);
        start = FX_grTexMinAddress(fxMesa, 0);
        end = FX_grTexMaxAddress(fxMesa, 0);
        fxMesa->freeTexMem[0] = end - start;
        fxMesa->tmFree[0] = fxTMNewRangeNode(fxMesa, start, end);
    }
    else {
        const int numTMUs = fxMesa->haveTwoTMUs ? 2 : 1;
        int tmu;
        fxMesa->umaTexMemory = GL_FALSE;
        for (tmu = 0; tmu < numTMUs; tmu++) {
            FxU32 start = FX_grTexMinAddress(fxMesa, tmu);
            FxU32 end = FX_grTexMaxAddress(fxMesa, tmu);
            fxMesa->freeTexMem[tmu] = end - start;
            fxMesa->tmFree[tmu] = fxTMNewRangeNode(fxMesa, start, end);
        }
    }

}


/*
 * Clean-up texture memory before destroying context.
 */
void
fxTMClose(fxMesaContext fxMesa)
{
    const int numTMUs = fxMesa->haveTwoTMUs ? 2 : 1;
    int tmu;
    MemRange *tmp, *next;

    /* Deallocate the pool of free MemRange nodes */
    tmp = fxMesa->tmPool;
    while (tmp) {
        next = tmp->next;
        FREE(tmp);
        tmp = next;
    }

    /* Delete the texture memory block MemRange nodes */
    for (tmu = 0; tmu < numTMUs; tmu++) {
        tmp = fxMesa->tmFree[tmu];
        while (tmp) {
            next = tmp->next;
            FREE(tmp);
            tmp = next;
        }
    }
}


/*
 * After a context switch this function will be called to restore
 * texture memory for the new context.
 */
void
fxTMRestoreTextures_NoLock(fxMesaContext ctx)
{
    tfxTexInfo *ti;
    struct gl_texture_object *tObj;
    int i, where;

    tObj = ctx->glCtx->Shared->TexObjectList;
    while (tObj) {
        ti = fxTMGetTexInfo(tObj);
        if (ti && ti->isInTM) {
            for (i = 0; i < MAX_TEXTURE_UNITS; i++)
                if (ctx->glCtx->Texture.Unit[i].Current == tObj) {
                    /* Force the texture onto the board, as it could be in use */
                    where = ti->whichTMU;
                    fxTMMoveOutTM_NoLock(ctx, tObj);
                    fxTMMoveInTM_NoLock(ctx, tObj, where);
                    break;
                }
            if (i == MAX_TEXTURE_UNITS) /* Mark the texture as off the board */
                fxTMMoveOutTM_NoLock(ctx, tObj);
        }
        tObj = tObj->Next;
    }
}

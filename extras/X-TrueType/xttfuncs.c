/* ===EmacsMode: -*- Mode: C; tab-width:4; c-basic-offset: 4; -*- === */
/* ===FileName: ===
   Copyright (c) 1998 Go Watanabe, All rights reserved.
   Copyright (c) 1998 Kazushi (Jam) Marukawa, All rights reserved.
   Copyright (c) 1998 Takuya SHIOZAKI, All rights reserved.
   Copyright (c) 1998 X-TrueType Server Project, All rights reserved.
  
===Notice
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.

   Major Release ID: X-TrueType Server Version 1.2 [Aoi MATSUBARA Release 2]

Notice===
*/

#include "xttversion.h"

static char const * const releaseID =
    _XTT_RELEASE_NAME;

/*
  X-TrueType Server -- invented by Go Watanabe.

  This Version includes the follow feautures:
    JAMPATCHs:
      written by Kazushi (Jam) Marukawa.
    CodeConv:
    Moduled CodeConv:
    TTCap:
      written by Takuya SHIOZAKI.
*/

#include <X11/Xos.h>
#include "fntfilst.h"
#include "xttcommon.h"
#include "xttcap.h"
#include "xttcconv.h"
#include "xttcache.h"
#include "xttstruct.h"

/* prototypes */
/* from lib/font/fontfile/defaults.c */
extern void
FontDefaultFormat(int *bit, int *byte, int *glyph, int *scan);
/* from lib/font/fontfile/renderers.c */
extern Bool
FontFileRegisterRenderer(FontRendererPtr renderer);
/* from lib/font/util/format.c */
extern int
CheckFSFormat(fsBitmapFormat format, fsBitmapFormatMask fmask,
              int *bit_order, int *byte_order, int *scan,
              int *glyph, int *image);
/* from lib/font/util/fontaccel.c */
extern void
FontComputeInfoAccelerators(FontInfoPtr pFontInfo);

static int FreeType_InitCount = 0;
static TT_Engine engine;

static int
FreeType_Init(void)
{
    dprintf((stderr, "FreeTypeInit\n"));
    if (FreeType_InitCount == 0) {
        if (TT_Init_FreeType(&engine)) {
            fprintf(stderr, "freetype: Could not create engine instance\n");
            return -1;
        }
    }
    FreeType_InitCount++;
    return 0;
}

static int
FreeType_Deinit(void)
{
    dprintf((stderr, "FreeTypeDeInit\n"));
    if (FreeType_InitCount <= 0) {
        return -1;
    }
    if (--FreeType_InitCount == 0) {
        if (TT_Done_FreeType(engine) < 0) {
            return -1;
        }
    }
    return 0;
}

static FreeTypeFaceInfo *faceTable = NULL;
static int faceTableCount = 0;

static int
FreeType_OpenFace(FreeTypeOpenFaceHints const *refHints)
{
    int i, error, num;
    TT_Face face;
    TT_Face_Properties prop;
    TT_Glyph glyph;
    FreeTypeFaceInfoPtr ptr;
    int ttcno = 0;
    SPropRecValContainer contRecValue;

    dprintf((stderr,
             "FreeType_OpenFace: %s %s %s\n",
             refHints->fontName, refHints->familyName, refHints->ttFontName));

    if ((error = TT_Open_Face(engine, refHints->ttFontName, &face))) {
        fprintf(stderr, "freetype: can't open face: %s\n", refHints->ttFontName);
        return -1;
    }

    if ((error = TT_Get_Face_Properties(face, &prop))) {
        TT_Close_Face(face);
        fprintf(stderr, "freetype: can't get face property.\n");
        return -1;
    }   

    if (SPropRecValList_search_record(refHints->refListPropRecVal,
                                      &contRecValue,
                                      "FaceNumber")) {
        /* get face number from Property File directly */
        if (SPropContainer_value_int(contRecValue)<0 ||
            SPropContainer_value_int(contRecValue)>=prop.num_Faces) {
            fprintf(stderr, "Bad face collection:%d\n",
                    SPropContainer_value_int(contRecValue));
            return -1;
        }
        ttcno = SPropContainer_value_int(contRecValue);
        if (ttcno) {
            TT_Close_Face(face);
            if ((error = TT_Open_Collection(engine, refHints->ttFontName, ttcno,
                                            &face))) {
                fprintf(stderr, "Can't Open face collection:%d\n", ttcno);
                return -1;
            }
        }
    }
#ifdef USE_XLFD_AUTO_CONTROL
    else {
        /* check font's family name from face infomation */
        int n, i, j;
        TT_UShort len;
        short platform, encoding, language, id;
        char *str;
        for (i = prop.num_Faces - 1; i>=0; i--) {
            TT_Close_Face(face);
            dprintf((stderr, "Open Collection No:%d\n", i));
            if ((error = TT_Open_Collection(engine, refHints->ttFontName, i,
                                            &face))) {
                fprintf(stderr, "Can't open face collection:%d\n", i);
                return -1;
            }
            n = TT_Get_Name_Count(face);
            for (j=0;j<n;j++) {
                TT_Get_Name_ID(face, j, &platform, &encoding, &language, &id);
                TT_Get_Name_String(face, j, &str, &len);
#if 0
                if (str)
                    fprintf(stderr, "id:%02d [%x] [%x] [%x] [%d] %*.*s\n", 
                            id, platform, encoding, language, len, len, len, str);
#endif
                if (str && id == 4 /*Name Id*/) {
                    if (platform == 3 && encoding == 1) {   /* MS-Unicode */
                        int k;
                        len /= 2;
#if 0
                        for (k=0;k<len;k++)
                            fprintf(stderr, "%c", str[k*2+1]);
                        fprintf(stderr, "\n");
#endif
                        for (k=0;k<len;k++) {
                            if (str[k*2+1] == '-' && refHints->familyName[k] == '_')
                                continue;
                            if (tolower(str[k*2+1]) != tolower(refHints->familyName[k]))
                                break;
                        }
                        if (k == len)
                            goto ttccheck_end;
                    } else {
#if 0
                        fprintf(stderr, "%*.*s\n", len, len, str);
#endif
                        if (!strncasecmp(str, refHints->familyName, len))
                            goto ttccheck_end;
                    }
                }
            }
        }
    ttccheck_end:
        ttcno = i;
    }
#endif /* USE_XLFD_AUTO_CONTROL - obsoleted. */
    
    dprintf((stderr, "Select Collection %d\n", ttcno));

    /* check !! */
    for (i = 0; i < faceTableCount; i++) {
        if (strcmp(faceTable[i].fontName, refHints->fontName) == 0 &&
            faceTable[i].ttcno == ttcno) {
            /* reopen */
            if (faceTable[i].flag) {
                if ((error = TT_Get_Face_Properties(face, &prop))) {
                    TT_Close_Face(face);
                    fprintf(stderr, "freetype: can't get face property.\n");
                    return -1;
                }
                if ((num = TT_Get_CharMap_Count(face)) < 0) {
                    TT_Close_Face(face);
                    fprintf(stderr, "freetype: can't get charmap count.\n");
                    return -1;
                }
                if ((error = TT_New_Glyph(face, &glyph))) {
                    TT_Close_Face(face);
                    fprintf(stderr, "freetype: can't get new glyph.\n");
                    return -1;
                }
                faceTable[i].face   = face;
                faceTable[i].ttcno  = ttcno;
                faceTable[i].prop   = prop;
                faceTable[i].glyph  = glyph;
                faceTable[i].mapnum = num;
                faceTable[i].flag   = 0;
            } else
                TT_Close_Face(face);
            faceTable[i].refCount++;
            return i;
        }
    }

    dprintf((stderr, "No Face. Make New Face\n"));

    if (!(ptr = (FreeTypeFaceInfoPtr)
          xrealloc(faceTable, (faceTableCount+1) * sizeof(*ptr)))){
        fprintf(stderr, "xrealloc: can't alloc memory for fonttable\n");
        return -1;
    }   

    if ((error = TT_Get_Face_Properties(face, &prop))) {
        TT_Close_Face(face);
        fprintf(stderr, "freetype: can't get face property.\n");
        return -1;
    }       
    if ((num = TT_Get_CharMap_Count(face)) < 0) {
        TT_Close_Face(face);
        fprintf(stderr, "freetype: can't get charmap count.\n");
        return -1;
    }
    if ((error = TT_New_Glyph(face, &glyph))) {
        TT_Close_Face(face);
        fprintf(stderr, "freetype: can't get new glyph.\n");
        return -1;
    }

    faceTable = ptr;
    faceTable[faceTableCount].fontName = strdup(refHints->fontName);
    faceTable[faceTableCount].ttcno    = ttcno;
    faceTable[faceTableCount].refCount = 1;
    faceTable[faceTableCount].face     = face;
    faceTable[faceTableCount].prop     = prop;
    faceTable[faceTableCount].glyph    = glyph;
    faceTable[faceTableCount].mapnum   = num;
    faceTable[faceTableCount].flag     = 0;
    faceTable[faceTableCount].scaleWidth     = 1.0;
    faceTable[faceTableCount].scaleBBoxWidth = 1.0;
    faceTable[faceTableCount].isAutoBold     = False;
    if (SPropRecValList_search_record(refHints->refListPropRecVal,
                                      &contRecValue,
                                      "ScaleWidth")) {
        /* Scaling to Width */
        double scaleWidth = SPropContainer_value_dbl(contRecValue);

        if (scaleWidth<=0.0) {
            fprintf(stderr, "ScaleWitdh needs plus.\n");
            return -1;
        }
        faceTable[faceTableCount].scaleWidth = scaleWidth;
    }
    if (SPropRecValList_search_record(refHints->refListPropRecVal,
                                      &contRecValue,
                                      "ScaleBBoxWidth")) {
        /* Scaling to Bounding Box Width */
        double scaleBBoxWidth = SPropContainer_value_dbl(contRecValue);

        if (scaleBBoxWidth<=0.0) {
            fprintf(stderr, "ScaleBBoxWitdh needs plus.\n");
            return -1;
        }
        faceTable[faceTableCount].scaleBBoxWidth = scaleBBoxWidth;
    }
    faceTable[faceTableCount].scaleBBoxWidth *=
        faceTable[faceTableCount].scaleWidth;
    if (SPropRecValList_search_record(refHints->refListPropRecVal,
                                      &contRecValue,
                                      "AutoBold")) {
        /* Set or Reset Auto Bold Flag */
        if (SPropContainer_value_bool(contRecValue))
            faceTable[faceTableCount].isAutoBold = True;
    }


    i = faceTableCount++;
    return i;
}

static int
FreeType_CloseFace(int index)
{
    dprintf((stderr, "FreeType_CloseFace: %d\n", index));
    if (index < faceTableCount) {
        if (faceTable[index].refCount <= 0) {
            fprintf(stderr, "FreeType_CloseFace: bad index\n");
            return -1;
        }
        if (--faceTable[index].refCount == 0) {
            TT_Done_Glyph(faceTable[index].glyph);
            TT_Close_Face(faceTable[index].face);
            faceTable[index].flag = -1;
        }
        return 0;
    }
    fprintf(stderr, "FreeType_CloseFace: bad index\n");
    return -1;
}

static void
convertNothing(FreeTypeFont *ft, unsigned char *p, int size)
{
}

static void
convertBitOrder(FreeTypeFont *ft, unsigned char *p, int size)
{
    extern void
        BitOrderInvert(register unsigned char *buf, register int nbytes);
    
    BitOrderInvert(p, size);
}

static void
convertByteOrder(FreeTypeFont *ft, unsigned char *p, int size)
{
    extern void TwoByteSwap(register unsigned char *buf, register int nbytes);
    extern void FourByteSwap(register unsigned char *buf, register int nbytes);
    
    if (ft->pFont->bit != ft->pFont->byte) {
        switch (ft->pFont->scan) {
        case 1:
            break;
        case 2:
            TwoByteSwap(p, size);
        case 4:
            FourByteSwap(p, size);
        }
    }
}

static void
convertBitByteOrder(FreeTypeFont *ft, unsigned char *p, int size)
{
    convertBitOrder(ft, p, size);
    convertByteOrder(ft, p, size);
}

#define ABS(a)  (((a)<0)?-(a):(a))

static int
FreeType_OpenFont(FreeTypeFont *ft,
                  FontScalablePtr vals, int glyph,
                  FreeTypeOpenFaceHints const *refHints)
{
    int mapID, fid, error, result;
    FreeTypeFaceInfo *fi;
    TT_Instance instance;
    double base_size;

    base_size = hypot(vals->point_matrix[2], vals->point_matrix[3]);
   
    result = Successful;
    fid = -1;
    ft->pool = NULL;
    
    if (FreeType_Init()< 0) {
        result = AllocError;
        goto abort;
    }

    if ((ft->fid = fid = FreeType_OpenFace(refHints)) < 0) {
        result = BadFontName;
        goto deinitQuit;
    }

    fi = &faceTable[fid];
    if (!codeconv_search_code_converter(refHints->charsetName,
                                        fi->face, fi->mapnum,
                                        refHints->refListPropRecVal,
                                        &ft->codeConverterInfo,
                                        &mapID)) {
        fprintf(stderr,
                "FreeType_OpenFont: don't mach charset %s\n",
                refHints->charsetName);
        result = BadFontName;
        goto deinitQuit;
    }

    TT_Get_CharMap(fi->face, mapID, &ft->charmap);

    /* create instance */
    if ((error = TT_New_Instance(fi->face, &instance))) {
        result = BadFontName;
        goto deinitQuit;
    }

    /* set resolution of instance */
    if ((error = TT_Set_Instance_Resolutions(instance, 
                                             (int)vals->x, (int)vals->y))) {
        result = BadFontName;
        goto doneInstQuit;
    }

    {
        int flRotated = 0, flStretched = 0;
        
        if (vals->point_matrix[1] != 0 || vals->point_matrix[2] != 0)
            flRotated = flStretched = 1;
        else if (vals->point_matrix[0] != vals->point_matrix[3])
            flStretched = 1;
        else if (fi->scaleWidth != 1.0)
            flStretched = 1;

        TT_Set_Instance_Transform_Flags(instance, flRotated, flStretched);

        /*
         * The size depend on the height, not the width.
         * So use point_matrix[3].
         */
        if ((error = TT_Set_Instance_CharSize(instance,
                                              base_size *64))) {
            result = BadFontName;
            goto doneInstQuit;
        }
        /* set matrix */
        ft->matrix.xx =
            fi->scaleWidth *
            vals->point_matrix[0] / base_size * 65536;
        ft->matrix.xy =
            fi->scaleWidth *
            vals->point_matrix[2] / base_size * 65536;
        ft->matrix.yx =
            vals->point_matrix[1] / base_size * 65536;
        ft->matrix.yy =
            vals->point_matrix[3] / base_size * 65536;

        dprintf((stderr, "matrix: %x %x %x %x\n",
                 ft->matrix.xx, 
                 ft->matrix.yx, 
                 ft->matrix.xy, 
                 ft->matrix.yy)); 
    }

#if 0
    /*
     * ft->map will be calculated in get_glyph_prop and get_glyph_const.
     * So now commented out them.
     */
    {
        int bytes;
        ft->map.rows = ABS(vals->pixel_matrix[3]) 
            + ABS(vals->pixel_matrix[1]) + 0.5;
        if (ft->spacing)
            bytes =
                (fi->scaleWidth*
                 (ABS(vals->pixel_matrix[0]) + ABS(vals->pixel_matrix[2]))
                 +(fi->isAutoBold?1.0:0.0)
                 +7.5) / 8;
        else
            bytes =
                (fi->scaleBBoxWidth*
                 (ABS(vals->pixel_matrix[0]) + ABS(vals->pixel_matrix[2]))
                 +(fi->isAutoBold?1.0:0.0)
                 +7.5) / 8;
        bytes += glyph - 1;
        bytes -= bytes % glyph;
        ft->map.cols  = bytes;
        ft->map.width = bytes * 8;
        ft->map.flow = TT_Flow_Down;
        ft->map.size = ft->map.rows * ft->map.cols;
        dprintf((stderr, "rows:%d cols:%d\n", ft->map.rows, ft->map.cols));
    }
#endif

    ft->instance = instance;
    TT_Get_Instance_Metrics(instance, &ft->imetrics);

    if ((ft->pool = CharInfoPool_Alloc()) == NULL) {
        result = AllocError;
        goto doneInstQuit;
    }

    if ((ft->btree = BTree_Alloc()) == NULL) {
        result = AllocError;
        goto doneInstQuit;
    }

    return result;

 doneInstQuit:
    TT_Done_Instance(instance);
 deinitQuit:
    if (ft->pool)
        CharInfoPool_Free(ft->pool);
    if (fid>=0)
        FreeType_CloseFace(fid);
    FreeType_Deinit();
 abort:
    return result;
}

static void
FreeType_CloseFont(FontPtr pFont)
{
    FreeTypeFont *ft = (FreeTypeFont*) pFont->fontPrivate;

    dprintf((stderr, "FreeType_CloseFont: %x\n", pFont));

    TT_Done_Instance(ft->instance);
    BTree_Free(ft->btree);
    CharInfoPool_Free(ft->pool);

    FreeType_CloseFace(ft->fid);
    FreeType_Deinit();
    codeconv_free_code_converter(&ft->codeConverterInfo);
    
    xfree(ft);
    xfree(pFont->info.props);
}

static char nochardat;
static CharInfoRec nocharinfo = {/*metrics*/{0,0,0,0,0,0}, 
                                     /*bits*/&nochardat};

static void
make_up_bold_bitmap(TT_Raster_Map *map)
{
    int x, y;
    char *p = (char *)map->bitmap;
    for (y=0; y<map->rows; y++) {
        char lsb = 0;
        for (x=0; x<map->cols; x++) {
            char tmp = *p<<7;
            *p |= (*p>>1) | lsb;
            lsb = tmp;
            p++;
        }
    }
}


/* Back Door into the FreeType... */

#ifndef VERY_LAZY
#define VERY_LAZY 1
#endif


#if VERY_LAZY
/* table "HMTX" */
typedef struct  
{
    TT_UShort  advance;
    TT_Short   bearing;
} TT_LongMetrics;
typedef TT_Short TT_ShortMetrics;
#endif


/* calculate glyph metric from glyph index */

static xCharInfo *
get_metrics(FreeTypeFont *ft, int c)
{
    FreeTypeFaceInfo *fi = &faceTable[ft->fid];

    CharInfoPtr charInfo;
    int width, ascent, descent;
    int lbearing, rbearing;
    TT_Glyph_Metrics metrics;

    TT_BBox bbox;

    if (!BTree_Search(ft->btree, c, (void **)&charInfo)) {
#if VERY_LAZY
        if (!ft->isVeryLazy) {
#endif

            TT_Load_Glyph(ft->instance, fi->glyph, c,  ft->flag);
            TT_Get_Glyph_Metrics(fi->glyph, &metrics);

            {
                TT_Outline outline;
                TT_Get_Glyph_Outline(fi->glyph, &outline);
                outline.dropout_mode = 2;
                TT_Transform_Outline(&outline, &ft->matrix);
                TT_Get_Outline_BBox(&outline, &bbox);
            }
#if VERY_LAZY
        } else {
            /* very lazy method */
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
            TT_Vector p0, p1, p2, p3;

            {
                /* horizontal */
                TT_Horizontal_Header *pHH = fi->prop.horizontal;
                int numHMs = pHH->number_Of_HMetrics;
                
                if (c<numHMs) {
                    metrics.advance =
                        TT_MulFix( ((TT_LongMetrics*)pHH->long_metrics+c)
                                                                   ->advance,
                                   ft->imetrics.x_scale );
                    metrics.bbox.xMax =
                        TT_MulFix( ((TT_LongMetrics*)pHH->long_metrics+c)
                                                                   ->advance
#if 0
                                               - pHH->min_Right_Side_Bearing,
#else
                                   ,
#endif
                                   ft->imetrics.x_scale );
                    metrics.bbox.xMin = metrics.bearingX =
                        TT_MulFix( ((TT_LongMetrics*)pHH->long_metrics+c)
                                                                   ->bearing,
                                   ft->imetrics.x_scale );
                } else {
                    metrics.advance =
                        TT_MulFix( ((TT_LongMetrics*)pHH
                                                 ->long_metrics+numHMs-1)
                                                                    ->advance,
                                   ft->imetrics.x_scale );
                    metrics.bbox.xMax =
                        TT_MulFix( ((TT_LongMetrics*)pHH
                                                 ->long_metrics+numHMs-1)
                                                                    ->advance
                                                - pHH->min_Right_Side_Bearing,
                                   ft->imetrics.x_scale );
                    metrics.bbox.xMin = metrics.bearingX =
                        TT_MulFix( *((TT_ShortMetrics*)pHH
                                                 ->short_metrics+c-numHMs),
                                   ft->imetrics.x_scale );
                }
            }
            {
                /* vertical */
                TT_Header *pH = fi->prop.header;
                metrics.bbox.yMin = TT_MulFix( pH->yMin,
                                               ft->imetrics.y_scale );
                metrics.bbox.yMax = TT_MulFix( pH->yMax,
                                               ft->imetrics.y_scale );
            }
            p0.x = p2.x = metrics.bbox.xMin;
            p1.x = p3.x = metrics.bbox.xMax;
            p0.y = p1.y = metrics.bbox.yMin;
            p2.y = p3.y = metrics.bbox.yMax;

            TT_Transform_Vector(&p0.x, &p0.y, &ft->matrix);
            TT_Transform_Vector(&p1.x, &p1.y, &ft->matrix);
            TT_Transform_Vector(&p2.x, &p2.y, &ft->matrix);
            TT_Transform_Vector(&p3.x, &p3.y, &ft->matrix);
#if 0
            fprintf(stderr, 
                    "(%.1f %.1f) (%.1f %.1f)"
                    "(%.1f %.1f) (%.1f %.1f)\n",
                    p0.x / 64.0, p0.y / 64.0,
                    p1.x / 64.0, p1.y / 64.0,
                    p2.x / 64.0, p2.y / 64.0,
                    p3.x / 64.0, p3.y / 64.0);
#endif
            bbox.xMin = MIN(p0.x, MIN(p1.x, MIN(p2.x, p3.x)));
            bbox.xMax = MAX(p0.x, MAX(p1.x, MAX(p2.x, p3.x)));
            bbox.yMin = MIN(p0.y, MIN(p1.y, MIN(p2.y, p3.y)));
            bbox.yMax = MAX(p0.y, MAX(p1.y, MAX(p2.y, p3.y)));
        }
#endif  /* VERY_LAZY */

        
        /*
         * Use floor64 and ceil64 to calulate exectly since values might
         * be minus.
         */
#define FLOOR64(x) ((x) & -64)
#define CEIL64(x) (((x) + 64 - 1) & -64)
        ascent = FLOOR64(bbox.yMax + 32) / 64;
        descent = CEIL64(-bbox.yMin - 32) / 64;
        lbearing = FLOOR64(bbox.xMin + 32) / 64;
        rbearing = FLOOR64(bbox.xMax + 32) / 64 + (fi->isAutoBold?1:0);

        width =
            FLOOR64((int)floor(metrics.advance * fi->scaleBBoxWidth
                               * ft->pixel_width_unit_x + 0.5) + 32) / 64;

        if ((charInfo = CharInfoPool_Get(ft->pool)) == NULL) {
            charInfo = &nocharinfo;
            fprintf(stderr, "can't get charinfo area\n");
            goto next;
        }
        charInfo->metrics.leftSideBearing  = lbearing;
        charInfo->metrics.rightSideBearing = rbearing;
        charInfo->metrics.ascent           = ascent;
        charInfo->metrics.descent          = descent;
        charInfo->metrics.characterWidth   = width;
        charInfo->metrics.attributes =  
            (unsigned short)(short)(floor(metrics.advance/64. * 1000.
                                          / ft->pixel_size));
        BTree_Insert(ft->btree, c, charInfo);
    }   
 next:
    return &charInfo->metrics;
}


static CharInfoPtr
get_glyph_prop(FreeTypeFont *ft, int c)
{
    FreeTypeFaceInfo *fi = &faceTable[ft->fid];
    CharInfoPtr charInfo;
    TT_Outline outline;

    int width, height, descent, lbearing;
    int glyph = ft->pFont->glyph;
    int bytes;

    if (!BTree_Search(ft->btree, c, (void **)&charInfo) ||
        charInfo->bits == NULL) {
        if (!charInfo) {
            /* Make charInfo */
            get_metrics(ft, c);
            /* Retry to get it created in get_metrics(). */
            BTree_Search(ft->btree, c, (void **)&charInfo);
            if (!charInfo) {
                charInfo = &nocharinfo;
                fprintf(stderr, "can't get charinfo area\n");
                goto next;
            }
        }

        TT_Load_Glyph(ft->instance, fi->glyph, c,  ft->flag);
        TT_Get_Glyph_Outline(fi->glyph, &outline);
        outline.dropout_mode = 2;
        TT_Transform_Outline(&outline, &ft->matrix);

        lbearing = charInfo->metrics.leftSideBearing;
        descent  = charInfo->metrics.descent;
        width    = charInfo->metrics.rightSideBearing - lbearing;
        height   = charInfo->metrics.ascent + descent;

        /* Make just fit bitmap and draw character on it */
        ft->map.rows  = height;
        bytes = (width + 7) / 8;
        bytes = (bytes + (glyph) - 1) & -glyph;
        ft->map.cols  = bytes;
        ft->map.width = width;
        ft->map.flow = TT_Flow_Down;
        ft->map.size  = ft->map.rows * ft->map.cols;
        CharInfoPool_Set(ft->pool, charInfo, ft->map.size);
        ft->map.bitmap = charInfo->bits;

        TT_Translate_Outline(&outline, -lbearing * 64, descent * 64);
        TT_Get_Outline_Bitmap(engine, &outline, &ft->map);
        if (fi->isAutoBold)
            make_up_bold_bitmap(&ft->map);
        (*ft->convert)(ft, ft->map.bitmap, ft->map.size);
    }
 next:
    return charInfo;
}

static CharInfoPtr
get_glyph_const(FreeTypeFont *ft, int c)
{
    FreeTypeFaceInfo *fi = &faceTable[ft->fid];
    CharInfoPtr charInfo;
    TT_Outline outline;

    int width, height, descent, lbearing;
    int glyph = ft->pFont->glyph;
    int bytes;

    if (!BTree_Search(ft->btree, c, (void **)&charInfo) ||
        charInfo->bits == NULL) {
        if (!charInfo) {
#if 1
            if ((charInfo = CharInfoPool_Get(ft->pool)) == NULL) {
                charInfo = &nocharinfo;
                fprintf(stderr, "can't get charinfo area\n");
                goto next;
            }
            BTree_Insert(ft->btree, c, charInfo);
#else
            /* Make charInfo */
            get_metrics(ft, c);
            /* Retry to get it created in get_metrics(). */
            BTree_Search(ft->btree, c, (void **)&charInfo);
            if (!charInfo) {
                charInfo = &nocharinfo;
                fprintf(stderr, "can't get charinfo area\n");
                goto next;
            }
#endif
        }

        TT_Load_Glyph(ft->instance, fi->glyph, c,  ft->flag);
        TT_Get_Glyph_Outline(fi->glyph, &outline);
        outline.dropout_mode = 2;
        TT_Transform_Outline(&outline, &ft->matrix);

        lbearing = ft->pFont->info.maxbounds.leftSideBearing;
        descent = ft->pFont->info.maxbounds.descent;
        width = ft->pFont->info.maxbounds.rightSideBearing - lbearing;
        height = ft->pFont->info.maxbounds.ascent + descent;

        /* Make maxbounds bitmap and draw character on it */
        ft->map.rows  = height;
        bytes = (width + 7) / 8;
        bytes = (bytes + (glyph) - 1) & -glyph;
        ft->map.cols  = bytes;
        ft->map.width = width;
        ft->map.flow = TT_Flow_Down;
        ft->map.size  = ft->map.rows * ft->map.cols;
        CharInfoPool_Set(ft->pool, charInfo, ft->map.size);
        ft->map.bitmap = charInfo->bits;

        /* Write metrics */
        charInfo->metrics = ft->pFont->info.maxbounds;

        TT_Translate_Outline(&outline, -lbearing * 64, descent * 64);
        TT_Get_Outline_Bitmap(engine, &outline, &ft->map);
        if (fi->isAutoBold)
            make_up_bold_bitmap(&ft->map);
        (*ft->convert)(ft, ft->map.bitmap, ft->map.size);
    }
 next:
    return charInfo;
}

int
FreeTypeGetGlyphs (FontPtr pFont,
                   unsigned long count,
                   unsigned char *chars,
                   FontEncoding encoding,
                   unsigned long *pCount,
                   CharInfoPtr *glyphs)
{
    FreeTypeFont *ft = (FreeTypeFont*) pFont->fontPrivate;
    CharInfoPtr *glyphsBase = glyphs;

    dprintf((stderr, "FreeTypeGetGlyphs: %p %d\n", pFont, count));

    if (ft->spacing == 'p' || ft->spacing == 'm') {         /* proportional */
        switch (encoding) {
        case Linear8Bit:
        case TwoD8Bit:
            while (count--) {
                unsigned c = *chars++;
                c = ft->codeConverterInfo.ptrCodeConverter(c);
                dprintf((stderr, "%04x\n", c));
                *glyphs++ = get_glyph_prop(ft,
                                           TT_Char_Index(ft->charmap, c));
            }               
            break;
        case Linear16Bit:
        case TwoD16Bit:
            while (count--) {
                unsigned c1, c2;
                c1 = *chars++;
                c2 = *chars++;
                dprintf((stderr, "code: %02x%02x ->", c1,c2));
                if (!(c1 >= pFont->info.firstRow &&
                      c1 <= pFont->info.lastRow  &&
                      c2 >= pFont->info.firstCol &&
                      c2 <= pFont->info.lastCol)) {
                    *glyphs++ = &nocharinfo;
                    dprintf((stderr, "invalid code\n"));
                } else {
                    c1 = ft->codeConverterInfo.ptrCodeConverter(c1<<8|c2);
                    dprintf((stderr, "%04x\n", c1));
                    *glyphs++ = get_glyph_prop(ft,
                                               TT_Char_Index(ft->charmap, c1));
                }
            }
            break;
        }
    } else {
        switch (encoding) {
        case Linear8Bit:
        case TwoD8Bit:
            while (count--) {
                unsigned c = *chars++;
                c = ft->codeConverterInfo.ptrCodeConverter(c);
                *glyphs++ = get_glyph_const(ft,
                                            TT_Char_Index(ft->charmap, c));
            }
            break;
        case Linear16Bit:
        case TwoD16Bit:
            while (count--) {
                unsigned c1, c2;
                c1 = *chars++;
                c2 = *chars++;
                dprintf((stderr, "code: %02x%02x ->", c1,c2));
                if (!(c1 >= pFont->info.firstRow &&
                      c1 <= pFont->info.lastRow  &&
                      c2 >= pFont->info.firstCol &&
                      c2 <= pFont->info.lastCol)) {
                    *glyphs++ = &nocharinfo;
                    dprintf((stderr, "invalid code\n"));
                } else {
                    c1 = ft->codeConverterInfo.ptrCodeConverter(c1<<8|c2);
                    dprintf((stderr, "%04x\n", c1));
                    *glyphs++ = get_glyph_const(ft,
                                                TT_Char_Index(ft->charmap,
                                                              c1));
                }
            }
            break;
        }
    }

    *pCount = glyphs - glyphsBase;
    return Successful;
}

int 
FreeTypeGetMetrics (FontPtr pFont,
                    unsigned long count,
                    unsigned char *chars,
                    FontEncoding encoding,
                    unsigned long *pCount,
                    xCharInfo **glyphs)
{
    FreeTypeFont *ft = (FreeTypeFont*) pFont->fontPrivate;
    xCharInfo **glyphsBase = glyphs;
    unsigned int c;

    /*dprintf((stderr, "FreeTypeGetMetrics: %d\n", count));*/
    if (ft->spacing == 'm' || ft->spacing == 'p') {
        switch (encoding) {
        case Linear8Bit:
        case TwoD8Bit:
            while (count--) {
                c = *chars++;
/*              dprintf((stderr, "code: %04x ->", c));*/
                c = ft->codeConverterInfo.ptrCodeConverter(c);
/*              dprintf((stderr, "%04x\n", c));*/
                *glyphs++ = get_metrics(ft, TT_Char_Index(ft->charmap, c));
            }
            break;
        case Linear16Bit:
        case TwoD16Bit:
            while (count--) {
                c = *chars++ << 8; c |= *chars++;
/*              dprintf((stderr, "code: %04x ->", c));*/
                c = ft->codeConverterInfo.ptrCodeConverter(c);
/*              dprintf((stderr, "%04x\n", c));*/
                *glyphs++ = get_metrics(ft, TT_Char_Index(ft->charmap, c));
            }
            break;
        }
        *pCount = glyphs - glyphsBase;
    } else {                                    /* -c- */
        switch (encoding) {
        case Linear8Bit:
        case TwoD8Bit:
            while (count--) {
                chars++; chars++;
                *glyphs++ = &pFont->info.maxbounds;
            }
            break;
        case Linear16Bit:
        case TwoD16Bit:
            while (count--) {
                chars++; chars++;
                *glyphs++ = &pFont->info.maxbounds;
            }
            break;
        }
        *pCount = glyphs - glyphsBase;
    }

    return Successful;
}

void
FreeTypeUnloadFont(FontPtr pFont)
{
    dprintf((stderr, "FreeTypeUnloadFont: %x\n", pFont));
    FreeType_CloseFont(pFont);
}

#ifdef USE_XLFD_AUTO_CONTROL
struct {
    char *name;
    int sign;
} slantinfo[] = {
    { "o", 1 },
    { "ro", -1 }
};
#endif /* USE_XLFD_AUTO_CONTROL - obsoleted. */

static void
adjust_min_max(minc, maxc, tmp)
     xCharInfo  *minc, *maxc, *tmp; 
{
#define MINMAX(field,ci) \
    if (minc->field > (ci)->field) \
    minc->field = (ci)->field; \
    if (maxc->field < (ci)->field) \
    maxc->field = (ci)->field;  
            
    MINMAX(ascent, tmp);
    MINMAX(descent, tmp);
    MINMAX(leftSideBearing, tmp);
    MINMAX(rightSideBearing, tmp);
    MINMAX(characterWidth, tmp);
    
    if ((INT16)minc->attributes > (INT16)tmp->attributes)
        minc->attributes = tmp->attributes;
    if ((INT16)maxc->attributes < (INT16)tmp->attributes)
        maxc->attributes = tmp->attributes;
#undef  MINMAX
}

void
freetype_compute_bounds(FreeTypeFont *ft,
                        FontInfoPtr pinfo,
                        FontScalablePtr vals)
{
    int row, col;
    short c;
    xCharInfo minchar, maxchar, *tmpchar;
    int overlap, maxOverlap;
    long swidth      = 0;
    long total_width = 0;
    int num_chars    = 0; 

    minchar.ascent = minchar.descent =
    minchar.leftSideBearing = minchar.rightSideBearing =
    minchar.characterWidth = minchar.attributes = 32767;
    maxchar.ascent = maxchar.descent =
    maxchar.leftSideBearing = maxchar.rightSideBearing =
    maxchar.characterWidth = maxchar.attributes = -32767;
    maxOverlap = -32767;

    for (row = pinfo->firstRow; row <= pinfo->lastRow; row++) {
        for (col = pinfo->firstCol; col <= pinfo->lastCol; col++) {
            c = row<<8|col;
#if 0
            fprintf(stderr, "comp_bounds: %x ->", c);
#endif
            c = ft->codeConverterInfo.ptrCodeConverter(c);
#if 0
            fprintf(stderr, "%x\n", c);
#endif
            if (c) {
                tmpchar = get_metrics(ft, TT_Char_Index(ft->charmap, c));
                adjust_min_max(&minchar, &maxchar, tmpchar);
                overlap = tmpchar->rightSideBearing - tmpchar->characterWidth;
                if (maxOverlap < overlap)
                    maxOverlap = overlap;
                num_chars++;
                swidth += ABS(tmpchar->characterWidth);
                total_width += tmpchar->characterWidth;
            }
        }
    }
    
    if (num_chars > 0) {
        swidth = (swidth * 10.0 + num_chars / 2.0) / num_chars;
        if (total_width < 0)
            swidth = -swidth;
        vals->width = swidth;
    } else
        vals->width = 0;

    pinfo->maxbounds     = maxchar;
    pinfo->minbounds     = minchar;
    pinfo->ink_maxbounds = maxchar;
    pinfo->ink_minbounds = minchar;
    pinfo->maxOverlap    = maxOverlap;
}


/*
 * restrict code range
 *
 * boolean for the numeric zone:
 *   results = results & (ranges[0] | ranges[1] | ... ranges[nranges-1])
 */

static void
restrict_code_range(unsigned short *refFirstCol,
                   unsigned short *refFirstRow,
                   unsigned short *refLastCol,
                   unsigned short *refLastRow,
                   fsRange const *ranges, int nRanges)
{
    if (nRanges) {
        int minCol = 256, minRow = 256, maxCol = -1, maxRow = -1;
        fsRange const *r = ranges;
        int i;
        
        for (i=0; i<nRanges; i++) {
            if (r->min_char_high != r->max_char_high) {
                minCol = 0x00;
                maxCol = 0xff;
            } else {
                if (minCol > r->min_char_low)
                    minCol = r->min_char_low;
                if (maxCol < r->max_char_low)
                    maxCol = r->max_char_low;
            }
            if (minRow > r->min_char_high)
                minRow = r->min_char_high;
            if (maxRow < r->max_char_high)
                maxRow = r->max_char_high;
            r++;
        }

        if (minCol > *refLastCol)
            *refFirstCol = *refLastCol;
        else if (minCol > *refFirstCol)
            *refFirstCol = minCol;
        
        if (maxCol < *refFirstCol)
            *refLastCol = *refFirstCol;
        else if (maxCol < *refLastCol)
            *refLastCol = maxCol;
        
        if (minRow > *refLastRow) {
            *refFirstRow = *refLastRow;
            *refFirstCol = *refLastCol;
        } else if (minRow > *refFirstRow)
            *refFirstRow = minRow;

        if (maxRow < *refFirstRow) {
            *refLastRow = *refFirstRow;
            *refLastCol = *refFirstCol;
        } else if (maxRow < *refLastRow)
            *refLastRow = maxRow;
    }
}


static void
restrict_code_range_by_str(unsigned short *refFirstCol,
                          unsigned short *refFirstRow,
                          unsigned short *refLastCol,
                          unsigned short *refLastRow,
                          char const *str)
{
    int nRanges = 0;
    fsRange *ranges = NULL;
    char const *p, *q;

    p = q = str;
    for (;;) {
        int minpoint=0, maxpoint=65535;
        long val;

        /* skip comma and/or space */
        while (',' == *p || isspace(*p))
            p++;

        /* begin point */
        if ('-' != *p) {
            val = strtol(p, (char **)&q, 0);
            if (p == q)
                /* end or illegal */
                break; 
            if (val<0 || val>65535) {
                /* out of zone */
                break;
            }
            minpoint = val;
            p=q;
        }
        
        /* skip space */
        while (isspace(*p))
            p++;

        if (',' != *p && '\0' != *p) {
            /* contiune */
            if ('-' == *p)
                /* hyphon */
                p++;
            else
                /* end or illegal */
                break;
            
            /* skip space */
            while (isspace(*p))
                p++;
            
            val = strtol(p, (char **)&q, 0);
            if (p != q) {
                if (val<0 || val>65535)
                    break;
                maxpoint = val;
            } else if (',' != *p && '\0' != *p)
                /* end or illegal */
                break;
            p=q;
        } else
            /* comma - single code */
            maxpoint = minpoint;
        
        if (minpoint>maxpoint) {
            int tmp;
            tmp = minpoint;
            minpoint = maxpoint;
            maxpoint = tmp;
        }

        /* add range */
#if 0
        fprintf(stderr, "zone: 0x%04X - 0x%04X\n", minpoint, maxpoint);
        fflush(stderr);
#endif
        nRanges++;
        ranges = (fsRange *)xrealloc(ranges, nRanges*sizeof(*ranges));
        if (NULL == ranges)
            break;
        {
            fsRange *r = ranges+nRanges-1;
            
            r->min_char_low = minpoint & 0xff;
            r->max_char_low = maxpoint & 0xff;
            r->min_char_high = (minpoint>>8) & 0xff;
            r->max_char_high = (maxpoint>>8) & 0xff;
        }
    }

    if (ranges) {
        restrict_code_range(refFirstCol, refFirstRow, refLastCol, refLastRow,
                           ranges, nRanges);
        xfree(ranges);
    }
}


int
FreeTypeOpenScalable (fpe, ppFont, flags, entry, fileName, vals, 
                      format, fmask, non_cachable_font)
     FontPathElementPtr fpe;
     FontPtr            *ppFont;
     int                flags;
     FontEntryPtr       entry;
     char               *fileName;
     FontScalablePtr    vals;
     fsBitmapFormat     format;
     fsBitmapFormatMask fmask;
     FontPtr            non_cachable_font;  /* We don't do licensing */
{
    int result = Successful;
    int i;
    FontPtr pFont;
    FontInfoPtr pinfo;
    FreeTypeFont *ft;
    int ret, bit, byte, glyph, scan, image;
/*    FontScalableRec tmpvals;*/

    char xlfdName[MAXFONTNAMELEN];
    char familyname[MAXFONTNAMELEN];
    char charset[MAXFONTNAMELEN], slant[MAXFONTNAMELEN];
    int spacing = 'r'; /* avoid 'uninitialized variable using' warning */

    SDynPropRecValList listPropRecVal;
    FreeTypeOpenFaceHints hints;
    char *dynStrTTFileName = NULL;
    char *dynStrRealFileName = NULL;
    SPropRecValContainer contRecValue;

    double base_width, base_height;

    dprintf((stderr, 
             "\n+FreeTypeOpenScalable(%x, %x, %x, %x, %s, %x, %x, %x, %x)\n",
             fpe, ppFont, flags, entry, fileName, vals,
             format, fmask, non_cachable_font));

#ifdef DUMP
    DumpFontPathElement(fpe);
    DumpFontEntry(entry);
    DumpFontScalable(vals);
#endif

    hints.isProp = False;
    hints.fontName = fileName;
    hints.charsetName = charset;
    hints.familyName = familyname;
    hints.refListPropRecVal = &listPropRecVal;

    
    if (SPropRecValList_new(&listPropRecVal)) {
        result = AllocError;
        goto quit;
    }
    {
        int len = strlen(fileName);
        char *capHead = NULL;
        {
            /* font cap */
            char *p1=NULL, *p2=NULL;

            if (NULL != (p1=strrchr(fileName, '/')))
                if (NULL != (p2=strrchr(p1, ':'))) {
                    /* colon exist in the right side of slash. */
                    int dirLen = p1-fileName+1;
                    int baseLen = fileName+len - p2 -1;

                    dynStrRealFileName = (char *)xalloc(dirLen+baseLen+1);
                    memcpy(dynStrRealFileName, fileName, dirLen);
                    strcpy(dynStrRealFileName+dirLen, p2+1);
                    capHead = p1+1;
                } else
                    dynStrRealFileName = strdup(fileName);
            else
                dynStrRealFileName = strdup(fileName);
        } /* font cap */

#ifdef USE_TTP_FILE
        /* ttp file - obsoleted. */
        if (len>0 && 'p' == fileName[len-1]) {
            hints.isProp = True;
            if (SPropRecValList_read_prop_file(&listPropRecVal,
                                               dynStrRealFileName)) {
                result = BadFontFormat;
                goto quit;
            }
            if (SPropRecValList_search_record(&listPropRecVal,
                                              &contRecValue,
                                              "FontFile")) {
                /* TrueTypeFont File from Property File Record */
#if 0
                fprintf(stderr, "FontFile %s\n",
                        SPropContainer_value_str(contRecValue));
#endif
                if ('/' == SPropContainer_value_str(contRecValue)[0])
                    /* absolute path */
                dynStrTTFileName =
                strdup(SPropContainer_value_str(contRecValue));
                else {
                    /* relative path */
                    char const *baseName =
                        strrchr(fileName, '/');
                    if (NULL == baseName)
                        /* relative from current path */
                        dynStrTTFileName =
                            strdup(SPropContainer_value_str(contRecValue));
                    else {
                        int lenDirName =
                            baseName-fileName+1;
                        dynStrTTFileName =
                            xalloc(lenDirName+
                                   strlen(
                                       SPropContainer_value_str(contRecValue))
                                   +1);
                        memcpy(dynStrTTFileName, fileName, lenDirName);
                        strcpy(&dynStrTTFileName[lenDirName],
                               SPropContainer_value_str(contRecValue));
                    }
                }
#if 0
                fprintf(stderr, "FontFile(after) %s\n",
                        dynStrTTFileName);
#endif
            } else {
                /* TrueTypeFont File from Property File Name */
                dynStrTTFileName = strdup(dynStrRealFileName);
                dynStrTTFileName[len-1] = 'f';
                if (access(dynStrTTFileName, F_OK)) {
                    dynStrTTFileName[len-1] = 'c';
                    if (access(dynStrTTFileName, F_OK)) {
                        result = BadFontPath;
                        xfree(dynStrTTFileName);
                        goto quit;
                    }
                }
            }
            hints.ttFontName = dynStrTTFileName;
        } else
#endif /* USE_TTP_FILE  --- obsoleted. */
            hints.ttFontName = dynStrRealFileName;

        {
            /* font cap */
            if (capHead)
                if (SPropRecValList_add_by_font_cap(&listPropRecVal,
                                                    capHead)) {
                    result = BadFontPath;
                    goto quit;
                }
        }
    }

    {
        char *p = entry->name.name, *p1=NULL;
        for (i=0; i<13; i++) {
            if (*p) {
                p1 = p + 1;
                if (!(p = strchr(p1,'-'))) p = strchr(p1, '\0');
            }
            switch (i) {
            case 0: /* foundry */
                break;
            case 1: /* family  */
                strncpy(familyname, p1, p-p1);
                familyname[p-p1] = '\0';
                break;
            case 2: /* weight  */
                break;
            case 3: /* slant   */
                strncpy(slant, p1, p-p1);
                slant[p-p1] = '\0';
                break;
            case 4: /* width   */
            case 5: /* none    */
            case 6: /* pixel   */
            case 7: /* point   */
            case 8: /* res x   */
            case 9: /* res y   */
                break;
            case 10: /* spacing */
                spacing = tolower(*p1);
                break;
            case 11: /* av width */
                break;
            case 12: /* charset */
                strcpy(charset, p1);
                /* eliminate zone description */
                {
                    char *p2;
                    if (NULL != (p2 = strchr(charset, '[')))
                        *p2 = '\0';
                }
            }
        }
    }
    dprintf((stderr, "charset: %s spacing: %d slant: %s\n", 
             charset, spacing, slant));


    /* slant control */
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "AutoItalic")) {
        vals->pixel_matrix[2] += 
            vals->pixel_matrix[0] * SPropContainer_value_dbl(contRecValue);
        vals->point_matrix[2] += 
            vals->point_matrix[0] * SPropContainer_value_dbl(contRecValue);
        vals->pixel_matrix[3] += 
            vals->pixel_matrix[1] * SPropContainer_value_dbl(contRecValue);
        vals->point_matrix[3] += 
            vals->point_matrix[1] * SPropContainer_value_dbl(contRecValue);
    }
#ifdef USE_XLFD_AUTO_CONTROL
    else 
        for (i=0; i<sizeof(slantinfo)/sizeof(slantinfo[0]); i++) {
            if (!mystrcasecmp(slant, slantinfo[i].name)) {
                vals->pixel_matrix[2] += 
                    vals->pixel_matrix[0] * slantinfo[i].sign * 0.5;
                vals->point_matrix[2] += 
                    vals->point_matrix[0] * slantinfo[i].sign * 0.5;
                vals->pixel_matrix[3] += 
                    vals->pixel_matrix[1] * slantinfo[i].sign * 0.5;
                vals->point_matrix[3] += 
                    vals->point_matrix[1] * slantinfo[i].sign * 0.5;
                break;
            }
        }
#endif /* USE_XLFD_AUTO_CONTROL - obsoleted. */

    /* Reject ridiculously small font sizes that will blow up the math */
    if   ((base_width = hypot(vals->pixel_matrix[0], vals->pixel_matrix[1])) < 1.0 ||
          (base_height = hypot(vals->pixel_matrix[2], vals->pixel_matrix[3])) < 1.0) {
        fprintf(stderr, "too small font\n");
        result = BadFontName;
        goto quit;
    }

    /* set up default values */
    FontDefaultFormat(&bit, &byte, &glyph, &scan);
    /* get any changes made from above */
    ret = CheckFSFormat(format, fmask, &bit, &byte, &scan, &glyph, &image);
    if (ret != Successful) {
        result = ret;
        goto quit;
    }

    /* allocate font struct */
    if ((pFont = (FontPtr)xalloc(sizeof(*pFont))) == NULL) {
        result = AllocError;
        goto quit;
    }

    /* allocate private font data */
    if ((ft = (FreeTypeFont *)xalloc(sizeof(*ft))) == NULL) {
        xfree(pFont);
        result = AllocError;
        goto quit;
    }

    /* init private font data */
    memset(ft, 0, sizeof(*ft));

    ft->pixel_size   = base_height;
    ft->pixel_width_unit_x = vals->pixel_matrix[0]/base_height;

    /* hinting control */
    ft->flag = TTLOAD_DEFAULT;
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "Hinting"))
        /* if not true, turn off hinting
         * some broken truetype font cannot get bitmaps when
         * hinting is applied */
        if (!SPropContainer_value_bool(contRecValue))
            ft->flag = TTLOAD_SCALE_GLYPH;

    if ((ret = FreeType_OpenFont(ft, vals, glyph, &hints))
        != Successful) {
        xfree(ft);
        xfree(pFont);
        result = BadFontName;
        goto quit;
    }

    ft->pFont = pFont;
    if (bit == MSBFirst) {
        if (byte == LSBFirst) {
            ft->convert = convertNothing;
        } else {
            ft->convert = convertByteOrder;
        }
    } else {
        if (byte == LSBFirst) {
            ft->convert = convertBitOrder;
        } else {
            ft->convert = convertBitByteOrder;
        }
    }
        
    ft->spacing = spacing;
#if True /* obsoleted ->->-> */
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "ForceProportional"))
        ft->spacing = SPropContainer_value_bool(contRecValue)?'p':'c';
    else
#endif /* <-<-<- obsoleted */
        if (SPropRecValList_search_record(&listPropRecVal,
                                           &contRecValue,
                                           "ForceSpacing")) {
            char *strSpace = SPropContainer_value_str(contRecValue);
            Bool err = False;
        
            if (1 != strlen(strSpace))
                err = True;
            else
                switch (strSpace[0]) {
                case 'p':
                case 'm':
                case 'c':
                    ft->spacing = strSpace[0];
                    break;
                default:
                    err = True;
                }
            if (err) {
                xfree(ft);
                xfree(pFont);
                result = BadFontName;
                goto quit;
            }
        }
    /* very lazy metrics */
    ft->isVeryLazy = False;
    if (SPropRecValList_search_record(&listPropRecVal,
                                      &contRecValue,
                                      "VeryLazyMetrics"))
        if (NULL == getenv("NOVERYLAZY"))
            /* If NOVERYLAZY is defined, the vl option is ignored. */
            ft->isVeryLazy = SPropContainer_value_bool(contRecValue);

    /* Fill in font record. Data format filled in by reader. */
    pFont->format = format;
    pFont->bit    = bit;
    pFont->byte   = byte;
    pFont->glyph  = glyph;
    pFont->scan   = scan;
    pFont->get_metrics   = FreeTypeGetMetrics;
    pFont->get_glyphs    = FreeTypeGetGlyphs;
    pFont->unload_font   = FreeTypeUnloadFont;
    pFont->unload_glyphs = NULL;
    pFont->refcnt = 0;
    pFont->maxPrivate = -1;
    pFont->devPrivates = 0;
    pFont->fontPrivate = (unsigned char *)ft;

    /* set ColInfo */
    pinfo = &pFont->info;
    pinfo->firstCol  = ft->codeConverterInfo.refCharSetInfo->firstCol;
    pinfo->firstRow  = ft->codeConverterInfo.refCharSetInfo->firstRow;
    pinfo->lastCol   = ft->codeConverterInfo.refCharSetInfo->lastCol;
    pinfo->lastRow   = ft->codeConverterInfo.refCharSetInfo->lastRow;
    pinfo->defaultCh = ft->codeConverterInfo.refCharSetInfo->defaultCh;
    /* restriction of the code range */
    {
        
        if (SPropRecValList_search_record(&listPropRecVal,
                                           &contRecValue,
                                           "CodeRange")) {
            restrict_code_range_by_str(&pinfo->firstCol, &pinfo->firstRow,
                                      &pinfo->lastCol, &pinfo->lastRow,
                                      SPropContainer_value_str(contRecValue));
        }
        restrict_code_range(&pinfo->firstCol, &pinfo->firstRow,
                           &pinfo->lastCol, &pinfo->lastRow,
                           vals->ranges, vals->nranges);
    }

    pinfo->allExist = 0;
    pinfo->drawDirection = LeftToRight;
    pinfo->cachable = 1;
    pinfo->anamorphic = False;  /* XXX ? */

    pFont->info.inkMetrics = 0;
    pFont->info.allExist = 0;
    pFont->info.maxOverlap = 0;
    pFont->info.pad = (short)0xf0f0; /* 0, 0xf0f0 ??? */

    {
        /* exact bounding box */
        int raw_ascent, raw_descent, raw_width;     /* RAW */
        int lsb, rsb, desc, asc;
        double newlsb, newrsb, newdesc, newasc;
        double point[2];
        double scale;

        /*
         * X11's values are not same as TrueType values.
         *  lsb is same.
         *  rsb is not same.  X's rsb is xMax.
         *  asc is same.
         *  desc is negative.
         *
         * NOTE THAT:
         *   `prop' is instance of TT_Face_Properties, and
         *   get by using TT_Get_Face_Properties().
         *   Thus, `prop' applied no transformation.
         */

        asc   = faceTable[ft->fid].prop.horizontal->Ascender;
        desc  = -(faceTable[ft->fid].prop.horizontal->Descender);
        lsb   = faceTable[ft->fid].prop.horizontal->min_Left_Side_Bearing;
        rsb   = faceTable[ft->fid].prop.horizontal->xMax_Extent;
		if (rsb == 0) 
            rsb = faceTable[ft->fid].prop.horizontal->advance_Width_Max;

        raw_width   = faceTable[ft->fid].prop.horizontal->advance_Width_Max;
        raw_ascent  = faceTable[ft->fid].prop.horizontal->Ascender;
        raw_descent = -(faceTable[ft->fid].prop.horizontal->Descender);

        /*
         * Apply scaleBBoxWidth.
         */
        lsb    = (int)floor(lsb * faceTable[ft->fid].scaleBBoxWidth);
        rsb    = (int)floor(rsb * faceTable[ft->fid].scaleBBoxWidth + 0.5);
        raw_width = raw_width * faceTable[ft->fid].scaleBBoxWidth;

#define TRANSFORM_POINT(matrix, x, y, dest) \
    ((dest)[0] = (matrix)[0] * (x) + (matrix)[2] * (y), \
     (dest)[1] = (matrix)[1] * (x) + (matrix)[3] * (y))

#define CHECK_EXTENT(lsb, rsb, desc, asc, data) \
    ((lsb) > (data)[0] ? (lsb) = (data)[0] : 0 , \
     (rsb) < (data)[0] ? (rsb) = (data)[0] : 0, \
     (-desc) > (data)[1] ? (desc) = -(data)[1] : 0 , \
     (asc) < (data)[1] ? (asc) = (data)[1] : 0)

        /* Compute new extents for this glyph */
        TRANSFORM_POINT(vals->pixel_matrix, lsb, -desc, point);
        newlsb = point[0];
        newrsb = newlsb;
        newdesc = -point[1];
        newasc = -newdesc;
        TRANSFORM_POINT(vals->pixel_matrix, lsb, asc, point);
        CHECK_EXTENT(newlsb, newrsb, newdesc, newasc, point);
        TRANSFORM_POINT(vals->pixel_matrix, rsb, -desc, point);
        CHECK_EXTENT(newlsb, newrsb, newdesc, newasc, point);
        TRANSFORM_POINT(vals->pixel_matrix, rsb, asc, point);
        CHECK_EXTENT(newlsb, newrsb, newdesc, newasc, point);

        /*
         * TrueType font is scaled.  So we made scaling value.
         */
        scale = 1.0 / faceTable[ft->fid].prop.header->Units_Per_EM;
        lsb  = (int)floor(newlsb * scale + 0.5);
        rsb  = (int)floor(newrsb * scale + 0.5);
        desc = (int)ceil(newdesc * scale - 0.5);
        asc  = (int)floor(newasc * scale + 0.5);
        
        raw_width   *= base_width * 1000. * scale / base_height;
        raw_ascent  *= 1000. * scale;
        raw_descent *= 1000. * scale;

        /* ComputeBounds */
        if (ft->spacing == 'c' || ft->spacing == 'm') { /* constant width */

            int width = 
                faceTable[ft->fid].prop.horizontal->advance_Width_Max
                    * faceTable[ft->fid].scaleBBoxWidth;
            
            /* AVERAGE_WIDTH ... 1/10 pixel unit */
            vals->width =(int)floor(width * 10.0 * 
                                    vals->pixel_matrix[0] * scale + 0.5);
            /* width */
            width = (int)floor(width * vals->pixel_matrix[0]  * scale + 0.5);

            if (ft->spacing == 'c') {
                /* Use same maxbounds and minbounds to make fast. */
                pFont->info.maxbounds.leftSideBearing  = lsb;
                pFont->info.maxbounds.rightSideBearing = rsb;
                pFont->info.maxbounds.characterWidth   = width;
                pFont->info.maxbounds.ascent           = asc;
                pFont->info.maxbounds.descent          = desc;
                pFont->info.maxbounds.attributes       = raw_width;

                pFont->info.minbounds = pFont->info.maxbounds;
            } else {
                /* Use different maxbounds and minbounds 
                   to let X check metrics. */
                pFont->info.maxbounds.leftSideBearing  = 0;
                pFont->info.maxbounds.rightSideBearing = rsb;
                pFont->info.maxbounds.characterWidth   = width;
                pFont->info.maxbounds.ascent           = asc;
                pFont->info.maxbounds.descent          = desc;
                pFont->info.maxbounds.attributes       = raw_width;

                pFont->info.minbounds.leftSideBearing  = lsb;
                pFont->info.minbounds.rightSideBearing = 0;
                pFont->info.minbounds.characterWidth   = width;
                pFont->info.minbounds.ascent           = asc;
                pFont->info.minbounds.descent          = desc;
                pFont->info.minbounds.attributes       = 0;
            }

            pFont->info.ink_maxbounds = pFont->info.maxbounds;
            pFont->info.ink_minbounds = pFont->info.minbounds;
            pFont->info.maxOverlap    = rsb - width;

        } else { /* proportional */
            freetype_compute_bounds(ft, pinfo, vals);
        }

        /* set ascent/descent */
        pFont->info.fontAscent  = asc;
        pFont->info.fontDescent = desc;
    
        /* set name for property */
        strncpy(xlfdName, vals->xlfdName, sizeof(xlfdName));
        FontParseXLFDName(xlfdName, vals, FONT_XLFD_REPLACE_VALUE);
        dprintf((stderr, "name: %s\n", xlfdName));

        /* set properties */
        freetype_compute_props(&pFont->info, vals, 
                               raw_width, raw_ascent, raw_descent,
                               xlfdName);
    }

    /* Set the pInfo flags */
    /* Properties set by FontComputeInfoAccelerators:
       pInfo->noOverlap;
       pInfo->terminalFont;
       pInfo->constantMetrics;
       pInfo->constantWidth;
       pInfo->inkInside;
    */
    FontComputeInfoAccelerators(pinfo);
#ifdef DUMP
    DumpFont(pFont);
#endif
    *ppFont = pFont;
    
    result = Successful;
    
 quit:
    if (dynStrTTFileName)
        xfree(dynStrTTFileName);
    if (dynStrRealFileName)
        xfree(dynStrRealFileName);
    return result;
}

int
FreeTypeGetInfoScalable(fpe, pFontInfo, entry, fontName, fileName, vals)
     FontPathElementPtr fpe;
     FontInfoPtr        pFontInfo;
     FontEntryPtr   entry;
     FontNamePtr        fontName;
     char        *fileName;
     FontScalablePtr    vals;
{
    FontPtr pfont;
    int flags = 0;
    long format = 0;
    long fmask  = 0;
    int ret;

    dprintf((stderr, "FreeTypeGetInfoScalable\n"));

    ret = FreeTypeOpenScalable(fpe, &pfont, flags, entry, 
                               fileName, vals, format, fmask, 0);
    if (ret != Successful)
        return ret;
    *pFontInfo  = pfont->info;

    pfont->info.props = NULL;
    pfont->info.isStringProp = NULL;
    
    FreeType_CloseFont(pfont);
    return Successful;
}

static FontRendererRec renderers[] =
{
    {   
        ".ttf", 4,
        (int (*)()) 0, FreeTypeOpenScalable,
        (int (*)()) 0, FreeTypeGetInfoScalable,
        0, CAP_MATRIX | CAP_CHARSUBSETTING
    },
    {   
        ".ttc", 4,
        (int (*)()) 0, FreeTypeOpenScalable,
        (int (*)()) 0, FreeTypeGetInfoScalable,
        0, CAP_MATRIX | CAP_CHARSUBSETTING
    }
#ifdef USE_TTP_FILE
    ,
    {   
        ".ttp", 4,
        (int (*)()) 0, FreeTypeOpenScalable,
        (int (*)()) 0, FreeTypeGetInfoScalable,
        0, CAP_MATRIX | CAP_CHARSUBSETTING
    }
#endif /* obsoleted */
};
    
int
XTrueTypeRegisterFontFileFunctions()
{
    int i;
    /* make standard prop */
    freetype_make_standard_props();

    /* reset */
    /* register */
    for (i=0;i<sizeof(renderers)/sizeof(renderers[0]);i++)
        FontFileRegisterRenderer(renderers + i);

    return 0;
}


/* end of file */

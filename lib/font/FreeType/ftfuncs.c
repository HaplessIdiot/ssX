/*
Copyright (c) 1997 by Mark Leisher
Copyright (c) 1998 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/* $XFree86: xc/lib/font/FreeType/ftfuncs.c,v 1.7 1999/01/31 04:59:22 dawes Exp $ */

#include "fntfilst.h"
#include "FSproto.h"
#include "freetype.h"
#include "ftxcmap.h"

#include "ttconfig.h"
#include "ft.h"
#include "ftfuncs.h"

/* The propery names for all the XLFD properties. */

static char *xlfd_props[] = {
  "FOUNDRY",
  "FAMILY_NAME",
  "WEIGHT_NAME",
  "SLANT",
  "SETWIDTH_NAME",
  "ADD_STYLE_NAME",
  "PIXEL_SIZE",
  "POINT_SIZE",
  "RESOLUTION_X",
  "RESOLUTION_Y",
  "SPACING",
  "AVERAGE_WIDTH",
  "CHARSET_REGISTRY",
  "CHARSET_ENCODING",
};


static int ftypeInitP = 0;      /* is the engine initialised? */
static TT_Engine ftypeEngine;   /* the engine */

/* Free a font.  If freePropsP is 0, don't free the properties. */

static void
FreeTypeFreeFont(FontPtr pFont, int freePropsP)
{
  int i;
  ttfont_t tf;

  if(pFont) {
    if(tf=(ttfont_t)pFont->fontPrivate) {
      xfree(tf->encoding);
      for (i = 0; i < tf->used; i++)
        xfree(tf->glyphs[i].bits);
      xfree(tf->glyphs);
      xfree(tf);
    }
    if(freePropsP && pFont->info.nprops>0) {
      xfree(pFont->info.isStringProp);
      xfree(pFont->info.props);
    }
    xfree(pFont);
  }
}


/* Unload a font */

static void
FreeTypeUnloadFont(FontPtr pFont)
{
  MUMBLE("Unloading\n");
  FreeTypeFreeFont(pFont, 1);
}

/* Add the font properties, including the Font name, the XLFD
 * properties, some strings from the font, and various typographical
 * data.  We only provide data readily available in the tables in the
 * font for now, altough FIGURE_WIDTH would be a good idea as it is
 * used by Xaw. */

static int
FreeTypeAddProperties(FontScalablePtr vals, FontInfoPtr info, 
                      char *fontname, 
                      TT_Face face, TT_Instance instance,
                      NormalisedTransformation *trans,
                      int rawAverageWidth)
{
  int i, j, maxprops;
  char *sp, *ep, val[256];
  TT_Instance_Metrics imetrics;
  TT_Face_Properties properties;
  int upm;                      /* units per em */

  int xlfdPropsP=0, hheaPropsP=0, os2PropsP=0, postPropsP=0;

  info->nprops=0;               /* in case we abort */

  strcpy(val, fontname);
  if(FontParseXLFDName(val, vals, FONT_XLFD_REPLACE_VALUE)) {
    xlfdPropsP=1;
  } else {
    MUMBLE("Couldn't parse XLFD\n");
    xlfdPropsP=0;
  }

  /* What properties are we going to set?  Check what tables there are. */
  if(!TT_Get_Face_Properties(face, &properties)) {
    if(properties.header) {
      upm=properties.header->Units_Per_EM;
      if(properties.horizontal)
        hheaPropsP=1;
      if(properties.os2)
        os2PropsP=1;
      if(properties.postscript)
        postPropsP=1;
    }
  }

  /* Compute an upper bound on the number of props in order to
   * allocate the right size of vector.  That's because we use
   * wonderfully flexible data structures. */
  maxprops=
    1+                          /* NAME */
    (xlfdPropsP?14:0)+          /* from XLFD */
    (hheaPropsP?5:0)+           /* from `hhea' table */
    3+                          /* from `name' table */
    (os2PropsP?6:0)+            /* from `os/2' table */
    (postPropsP?3:0)+           /* from `post' table */
    1;                          /* type */

  if ((info->props =
       (FontPropPtr)xalloc(maxprops * sizeof(FontPropRec))) == 0)
    return AllocError;

  if ((info->isStringProp = (char*)xalloc(maxprops)) == 0) {
    xfree(info->props);
    return AllocError;
  }

  memset((char *)info->isStringProp, 0, maxprops);

  /* Add the FONT property as the very first */
  info->props[0].name = MakeAtom("FONT", 4, TRUE);
  info->props[0].value = MakeAtom(val, strlen(val), TRUE);
  info->isStringProp[0] = 1;
  i=1;

  if(*val && *(sp=val+1)) {
    for (i = 0, sp=val+1; i < 14; i++) {
      /* Locate the next field. */
      if (i == 13)
        /* Handle the case of the final field containing a subset
         * specification. */
        for (ep = sp; *ep && *ep != '['; ep++);
      else
        for (ep = sp; *ep && *ep != '-'; ep++);
      
      /* Create the property name.*/
      info->props[i+1].name =
        MakeAtom(xlfd_props[i], strlen(xlfd_props[i]), TRUE);

      switch(i) {
      case 6:                   /* pixel size */
        info->props[i+1].value = 
          (int)(fabs(vals->pixel_matrix[3])+0.5);
        break;
      case 7:                   /* point size */
        info->props[i+1].value = 
          (int)(fabs(vals->point_matrix[3])*10.0 + 0.5);
        break;
      case 8:                   /* resolution x */
        info->props[i+1].value = vals->x;
        break;
      case 9:                   /* resolution y */
        info->props[i+1].value = vals->y;
        break;
      case 11:                  /* average width */
        info->props[i+1].value = vals->width;
        break;
      default:                  /* a string */
        info->props[i+1].value = MakeAtom(sp, ep - sp, TRUE);
        info->isStringProp[i+1] = 1;
      }
      sp = ++ep;
    }
    i++;
  }


  /* the following two have already been properly scaled */

  if(hheaPropsP) {
    info->props[i].name = MakeAtom("RAW_AVERAGE_WIDTH", 17, TRUE);
    info->props[i].value = rawAverageWidth;
    i++;

    info->props[i].name = MakeAtom("FONT_ASCENT", 11, TRUE);
    info->props[i].value = info->fontAscent;
    i++;

    info->props[i].name = MakeAtom("RAW_ASCENT", 15, TRUE);
    info->props[i].value = 
      ((double)properties.horizontal->Ascender/(double)upm*1000.0);
    i++;

    info->props[i].name = MakeAtom("FONT_DESCENT", 12, TRUE);
    info->props[i].value = info->fontDescent;
    i++;

    info->props[i].name = MakeAtom("RAW_DESCENT", 16, TRUE);
    info->props[i].value = 
      -((double)properties.horizontal->Descender/(double)upm*1000.0);
    i++;
  }

  if((j=ttf_GetEnglishName(face, val, TTF_COPYRIGHT))>0) {
    info->props[i].name = MakeAtom("COPYRIGHT", 9, TRUE);
    info->props[i].value = MakeAtom(val, j, TRUE);
    info->isStringProp[i] = 1;
    i++;
  }

  /* Get the typeface name from the TT font. */
  if((j=ttf_GetEnglishName(face, val, TTF_TYPEFACE))>0) {
    info->props[i].name = MakeAtom("FACE_NAME", 9, TRUE);
    info->props[i].value = MakeAtom(val, j, TRUE);
    info->isStringProp[i] = 1;
    i++;
  }

  /* Get the Postscript name from the TT font. */
  if((j=ttf_GetEnglishName(face, val, TTF_PSNAME))>0) {
    info->props[i].name = MakeAtom("_ADOBE_POSTSCRIPT_FONTNAME", 26, TRUE);
    info->props[i].value = MakeAtom(val, j, TRUE);
    info->isStringProp[i] = 1;
    i++;
  }

  /* These macros handle the case of a diagonal matrix.  They convert
   * FUnits into pixels. */
#define TRANSFORM_FUNITS_X(xval) \
  ((int) \
   ((((double)(xval)/(double)upm) * \
     ((double)trans->matrix.xx/TWO_SIXTEENTH)*(double)imetrics.x_ppem)+0.5))

#define TRANSFORM_FUNITS_Y(yval) \
  ((int) \
   ((((double)(yval)/(double)upm) * \
     ((double)trans->matrix.yy/TWO_SIXTEENTH) * (double)imetrics.y_ppem)+0.5))

  /* In what follows, we assume the matrix is diagonal.  In the rare
   * case when it is not, the values will be somewhat wrong. */
  
  if(TT_Get_Instance_Metrics(instance, &imetrics)==0) {
    if(os2PropsP) {
      info->props[i].name = MakeAtom("SUBSCRIPT_SIZE",14,TRUE);
      info->props[i].value = 
        TRANSFORM_FUNITS_Y(properties.os2->ySubscriptYSize);
      i++;
      info->props[i].name = MakeAtom("SUBSCRIPT_X",11,TRUE);
      info->props[i].value = 
        TRANSFORM_FUNITS_X(properties.os2->ySubscriptXOffset);
      i++;
      info->props[i].name = MakeAtom("SUBSCRIPT_Y",11,TRUE);
      info->props[i].value = 
        TRANSFORM_FUNITS_Y(properties.os2->ySubscriptYOffset);
      i++;
      info->props[i].name = MakeAtom("SUPERSCRIPT_SIZE",16,TRUE);
      info->props[i].value = 
        TRANSFORM_FUNITS_Y(properties.os2->ySuperscriptYSize);
      i++;
      info->props[i].name = MakeAtom("SUPERSCRIPT_X",13,TRUE);
      info->props[i].value = 
        TRANSFORM_FUNITS_X(properties.os2->ySuperscriptXOffset);
      i++;
      info->props[i].name = MakeAtom("SUPERSCRIPT_Y",13,TRUE);
      info->props[i].value = 
        TRANSFORM_FUNITS_Y(properties.os2->ySuperscriptYOffset);
      i++;
    }
    if(postPropsP) {
      long underlineThickness, underlinePosition;
      /* tk uses the following two */
      info->props[i].name = MakeAtom("UNDERLINE_THICKNESS",19,TRUE);
      underlineThickness=
        TRANSFORM_FUNITS_Y(properties.postscript->underlineThickness);
      if(underlineThickness<=0)
        underlineThickness=1;
      info->props[i].value = underlineThickness;
      i++;
      info->props[i].name = MakeAtom("UNDERLINE_POSITION",18,TRUE);
      /* PostScript and X use opposite conventions */
      underlinePosition=
        TRANSFORM_FUNITS_Y(-properties.postscript->underlinePosition);
      info->props[i].value = underlinePosition;
      i++;
      if(trans->matrix.xx == trans->matrix.yy) {
        info->props[i].name = MakeAtom("ITALIC_ANGLE",12,TRUE);
        info->props[i].value = 
          /* Convert from TT_Fixed to 
           * 64th of a degree counterclockwise from 3 o'clock */
          90*64+(properties.postscript->italicAngle>>10);
        i++;
      }
    }
#undef TRANSFORM_FUNITS_X
#undef TRANSFORM_FUNITS_Y
  }

  info->props[i].name  = MakeAtom("FONT_TYPE", 9, TRUE);
  info->props[i].value = MakeAtom("TrueType", 8, TRUE);
  i++;

  info->nprops=i;
  return Successful;
}

/* Prepare the library to work on a face. */

static int
FreeTypePrepareRasteriser(char *fileName,
                          TT_Face *face, TT_Instance *instance,
                          TT_Instance_Metrics *imetrics,
                          TT_Face_Properties *properties,
                          TT_Glyph *glyph,
                          NormalisedTransformation *trans,
                          FontScalablePtr vals)
{
  int ftrc;                     /* FreeType error code */
  int stretchedP, rotatedP;
  double fscale;

  /* Make sure FreeType is initialized. */
  if (!ftypeInitP) {
    /* Report initialization errors as an allocation error because X does
     * not provide any other kind of error code related to intialization.
     */
    if (ftrc=TT_Init_FreeType(&ftypeEngine)) {
      MUMBLE1("Error initializing ftypeEngine: %d\n", ftrc);
      return AllocError;
    }
    ftypeInitP = 1;
  }

  /* Open the input file. */
  if (ftrc=TT_Open_Face(ftypeEngine, fileName, face)) {
    MUMBLE1("Couldn't open face: %d\n", ftrc);
    return BadFontPath;
  }

  /* Create a new instance of the font.  Report errors as a bad allocation.
   */
  if (ftrc=TT_New_Instance(*face, instance)) {
    TT_Close_Face(*face);
    MUMBLE1("Couldn't open instance: %d\n", ftrc);
    return FTtoXReturnCode(ftrc);
  }
  
  /* Set the instance resolution, point size, and get the instance metrics
   * structure. */
  if(ftrc=TT_Set_Instance_Resolutions(*instance, vals->x, vals->y)) {
    TT_Close_Face(*face);
    MUMBLE1("Couldn't set resolution: %d\n:", ftrc);
    return FTtoXReturnCode(ftrc);
  }

  /* This value cannot be 0. */
  trans->scale=MAX(hypot(vals->point_matrix[0],vals->point_matrix[2]),
                   hypot(vals->point_matrix[1],vals->point_matrix[3]));

  if(ftrc=TT_Set_Instance_CharSize(*instance, 
                                   (int)(trans->scale*(1<<6)+0.5))) {
    TT_Close_Face(*face);
    MUMBLE1("Couldn't set charsize: %d\n", ftrc);
    return FTtoXReturnCode(ftrc);
  }

  /* Try to round stuff.  We want approximate zeros to be exact zeros,
     and if the elements on the diagonal are approximately equal, we
     want them equal.  We do this to avoid breaking hinting. */
  if(DIFFER(vals->point_matrix[0], vals->point_matrix[3])) {
    trans->matrix.xx=
      (int)((vals->point_matrix[0]*(double)TWO_SIXTEENTH)/trans->scale);
    trans->matrix.yy=
      (int)((vals->point_matrix[3]*(double)TWO_SIXTEENTH)/trans->scale);
  } else {
    trans->matrix.xx=trans->matrix.yy=
      ((vals->point_matrix[0]+vals->point_matrix[3])/2*
       (double)TWO_SIXTEENTH)/trans->scale;
  }

  trans->matrix.yx=
    DIFFER0(vals->point_matrix[1], trans->scale)?
    (int)((vals->point_matrix[1]*(double)TWO_SIXTEENTH)/trans->scale):
    0;
  trans->matrix.xy=
    DIFFER0(vals->point_matrix[2], trans->scale)?
    (int)((vals->point_matrix[2]*(double)TWO_SIXTEENTH)/trans->scale):
    0;

  /* What does streching mean? */
  stretchedP = 
    ((trans->matrix.xx>>8)*(trans->matrix.xx>>8) + 
     ((trans->matrix.yx>>8)*(trans->matrix.yx>>8)) != (1<<16)) ||
    ((trans->matrix.xy>>8) * (trans->matrix.xy>>8) +
     (trans->matrix.yy>>8) * (trans->matrix.yy>>8) != (1<<16));
  rotatedP = trans->matrix.yx!=0 || trans->matrix.xy!=0;

  if(stretchedP || rotatedP)
    TT_Set_Instance_Transform_Flags(*instance, rotatedP, stretchedP);
      
  if(ftrc=TT_Get_Instance_Metrics(*instance, imetrics)) {
    TT_Close_Face(*face);
    MUMBLE1("Couldn't get instance metrics: %d\n", ftrc);
    return FTtoXReturnCode(ftrc);
  }
  if(ftrc=TT_Get_Face_Properties(*face, properties)) {
    TT_Close_Face(*face);
    MUMBLE1("Couldn't get face properties: %d\n", ftrc);
    return FTtoXReturnCode(ftrc);
  }

  if(ftrc=TT_New_Glyph(*face, glyph)) {
    TT_Close_Face(*face);
    MUMBLE1("Couldn't create glyph: %d\n", ftrc);
    return FTtoXReturnCode(ftrc);
  }

  return Successful;
}


/* Do all the real work for OpenFont or FontInfo */
/* xf->info is only accessed through info, and xf might be null */

static int
FreeTypeLoadFont(char *fileName, 
                 FontScalablePtr vals, FontPtr xf, FontInfoPtr info,
                 FontBitmapFormat *bmfmt, FontEntryPtr entry)
{
  TT_Face face;
  TT_Face_Properties properties;
  TT_Instance instance;
  TT_Instance_Metrics imetrics;
  TT_Glyph glyph;
  NormalisedTransformation trans;
  ttfont_t tf;
  int nocmap;
  struct ttf_mapping mapping;
  long totalWidth, rawTotalWidth;
  int gcnt, used, size;
  int code;
  unsigned short idx;
  unsigned short *tep;
  CharInfoPtr tgp=NULL;
  int i;
  int haveDefaultGlyphP=0;
  int xrc;                /* X error code */

  if((xrc=FreeTypePrepareRasteriser(fileName,
                                    &face, &instance, &imetrics,
                                    &properties, &glyph,
                                    &trans,
                                    vals))
     !=Successful)
    return xrc;

  /* Get the requested cmap. */

  if(entry->name.ndashes==14)
    nocmap=ttf_pick_cmap(entry->name.name, entry->name.length, fileName,
                         face, &mapping);
  else
    nocmap=ttf_pick_cmap(0,0,fileName,face,&mapping);

  /* Create fontPrivate storage which will store a list of the glyphs
   * and encodings. */
  if(xf) {
    if ((tf = (ttfont_t)xalloc(sizeof(ttfont_rec))) == 0) {
      TT_Close_Face(face);
      return AllocError;
    }
    xf->fontPrivate = (pointer)tf;
    tf->defaultGlyph = 0;
  }

  /* Allocate the glyph and encoding arrays. */

  /* Guess the number of glyphs.  If we underestimate, we'll have to
   * resize the vector later on. */
  if(nocmap)
    size= properties.num_Glyphs;
  else
    size= MIN(mapping.size, properties.num_Glyphs);

  if(size<=0) {
    if(xf)
      xfree(tf);
    TT_Close_Face(face);
    return BadFontName;
  }

  used = 0;

  if(xf) {
    if ((tf->encoding = (unsigned short *)xalloc(size*sizeof(unsigned short)))
        == 0) {
      xfree(tf);
      TT_Close_Face(face);
      return AllocError;
    }
    if ((tf->glyphs = (CharInfoPtr)xalloc(size*sizeof(CharInfoRec))) 
        == 0) {
      xfree(tf->encoding);
      /* A strange feeling comes over you -more-
       * you find yourself wishing for a garbage collector -more-
       * the strange feeling passes -more-
       * you return to hacking C. */
      xfree(tf);
      TT_Close_Face(face);
      return AllocError;
    }
  }

  /* Set the font ascent and descent.  This should not be changed,
   * even if only a subset of glyphs are being loaded because the
   * correct values are determined by the font designer. */

  if(properties.horizontal) {
    info->fontAscent = 
      (double)properties.horizontal->Ascender /
      (double)properties.header->Units_Per_EM *
      ((double)trans.matrix.yy/TWO_SIXTEENTH) *
      (double)imetrics.y_ppem+0.5;
    info->fontDescent = 
      -((double)properties.horizontal->Descender) /
      (double)properties.header->Units_Per_EM *
      ((double)trans.matrix.yy/TWO_SIXTEENTH) *
      (double)imetrics.y_ppem+0.5;
  } else {
    /* Can this possibly happen?  Better safe then sorry. */
    info->fontAscent=info->fontDescent=0;
  }

  /* Prepare the row and column fields. */
  info->firstRow = info->firstCol = 0xffff;
  info->lastRow = info->lastCol = 0;

  /* Prepare the font info so the min and max bounds can be determined while
   * the glyphs are being loaded. */
  info->maxbounds.ascent = info->maxbounds.descent =
    info->maxbounds.characterWidth =
    info->maxbounds.leftSideBearing = 
    info->maxbounds.rightSideBearing = 
    info->maxbounds.attributes = (unsigned short)-32767;
  info->minbounds.ascent = info->minbounds.descent =
    info->minbounds.characterWidth =
    info->minbounds.leftSideBearing = 
    info->minbounds.rightSideBearing = 
    info->minbounds.attributes = 32767;
  info->maxOverlap = -32767;

  if(xf) {
    tgp = tf->glyphs;
    tep = tf->encoding;
  }
  gcnt=0;
  totalWidth=rawTotalWidth=0;

  /* The following loop condition is complicated by the fact that we
   * want it to work whether there is a cmap or not, and with a recode
   * function.  Ever heard of flet? */

  for(idx=code=0;               /* need to set idx for the case !recode */
      nocmap?
        ((gcnt<properties.num_Glyphs) && code<=0xfff):
        (code<MIN(mapping.size, 0xFFFF));
      nocmap?code++:
        (mapping.mapping?code++:
         (code=TT_CharMap_Next(mapping.cmap, code, &idx)))) {

    if(code<0)
      break;

    if (nocmap)
      idx = code;
    else
      if(mapping.mapping)
        idx = ttf_remap(code, &mapping);

    /* As a special case, we pass 0 even when its not in the ranges;
     * this will allow for the default glyph, which should exist in
     * any TrueType font. */
    
    if(code>0 && vals->nranges) {
      for(i=0; i<vals->nranges; i++) {
        if((code >= 
            vals->ranges[i].min_char_low+
            (vals->ranges[i].min_char_high<<8)) &&
           (code <=
            vals->ranges[i].max_char_low+(vals->ranges[i].max_char_high<<8)))
          break;
      }
      if(i==vals->nranges)
        continue;
    }

    if(idx==0) {                /* default glyph */
      if(haveDefaultGlyphP)
        continue;
      else {
        info->defaultCh=code;
        if(xf)
          tf->defaultGlyph=tgp;
        haveDefaultGlyphP=1;
      }
    }

    if(TT_Load_Glyph(instance, glyph, idx, TTLOAD_DEFAULT)) {
      MUMBLE1("%d unglyph\n", idx);
      continue;
    }


    if((xrc=FreeTypeLoadGlyph(vals, info, glyph, &trans, tgp, bmfmt,
                              &totalWidth, &rawTotalWidth))
       !=Successful) {
      MUMBLE1("%d doubleplusunloaded\n", idx);
      continue;
    }

    /* Set the encoding along with the row and column bounds. */
    if(xf)
      *tep = code;
    ADJUSTMIN(info->firstRow, code>>8);
    ADJUSTMAX(info->lastRow, code>>8);
    ADJUSTMIN(info->firstCol, code & 0xff);
    ADJUSTMAX(info->lastCol, code & 0xff);

    gcnt++;
    used++;
    if(xf) {
      tep++;
      tgp++;
    }
    if(used >= size) {
      /* our initial estimate was wrong; expand encoding and glyphs */
      /* I do not know if realloc(3) can be trusted on all systems */
      unsigned short *new_encoding;
      CharInfoPtr new_glyphs;
      int new_size=size+(size+1)/2;

      if(xf) {
        if((new_encoding=(unsigned short *)xalloc(new_size*
                                                  sizeof(unsigned short)))
           ==0)
          break;                /* give up */
        if((new_glyphs=
            (CharInfoPtr)xalloc(new_size*sizeof(CharInfoRec)))==0) {
          xfree(new_encoding);
          break;
        }
        memcpy(new_encoding, tf->encoding, size*sizeof(short));
        memcpy(new_glyphs, tf->glyphs, size*sizeof(CharInfoRec));
        tep = new_encoding + (tep - tf->encoding);
        free(tf->encoding);
        tf->encoding=new_encoding;
        if (haveDefaultGlyphP) {
          /* Reset the default glyph if it's been set previously. */
          tf->defaultGlyph = new_glyphs + (tf->defaultGlyph - tf->glyphs);
        }
        tgp = new_glyphs + (tgp - tf->glyphs);
        free(tf->glyphs);
        tf->glyphs=new_glyphs;
      }
      size=new_size;
    }
  }

  MUMBLE1("used=%d\n", used);

  if(used==0) {                 /* no glyphs found! */
    if(xf) {
      xfree(tf->encoding);
      xfree(tf->glyphs);
      xfree(tf);
    }
    TT_Close_Face(face);
    return BadFontName;
  }

  if(xf) {
    tf->used=used;
    tf->size=size;
  }

  /* Final adjustment to the maximum overlap. */
  info->maxOverlap += -info->minbounds.leftSideBearing;

  /* Copy the min and max bounds to the ink bounds. */
  memcpy((char *) &info->ink_maxbounds,
         (char *) &info->maxbounds, sizeof(xCharInfo));
  memcpy((char *) &info->ink_minbounds,
         (char *) &info->minbounds, sizeof(xCharInfo));

  /* Calculate the average width and the direction hint */
  info->drawDirection = (totalWidth>=0)?LeftToRight:RightToLeft;
  if(info->minbounds.characterWidth == info->maxbounds.characterWidth) {
    /* The font is monospace.*/
    vals->width = info->maxbounds.characterWidth * 10;
    info->constantWidth=1;
    if(info->minbounds.ascent == info->maxbounds.ascent &&
       info->minbounds.descent == info->maxbounds.descent &&
       info->minbounds.leftSideBearing == 
         info->maxbounds.leftSideBearing &&
       info->minbounds.rightSideBearing == 
         info->maxbounds.rightSideBearing) {
      info->constantMetrics = 1;
    }
  } else
    /* The font is proportional */
    vals->width = (totalWidth*10+used/2)/used;

  /* Use the new XLFD name to generate the properties for the new font. */
  if((xrc = FreeTypeAddProperties(vals, info, entry->name.name, 
                                  face, instance, &trans, 
                                  (int)((rawTotalWidth*10+used/2)/used)))
     != Successful) {
    MUMBLE1("Couldn't add properties: %d\n", xrc);
    if(xf)
      FreeTypeFreeFont(xf,0);
  }
  
  TT_Close_Face(face);

  return xrc;
}


/* Do the work for one glyph */
/* If tgp is 0, we're only interested in updating info */
static int
FreeTypeLoadGlyph(FontScalablePtr vals, FontInfoPtr info, 
                  TT_Glyph glyph, NormalisedTransformation *trans,
                  CharInfoPtr tgp, FontBitmapFormat *bmfmt,
                  long *totalWidth, long *rawTotalWidth)
{
  TT_Raster_Map raster;
  TT_Glyph_Metrics metrics;
  TT_Outline outline;
  TT_BBox outline_bbox, *bbox;
  short xoff, yoff, xoff_pixels, yoff_pixels;
  int wd, ht, bpr;              /* width, height, bytes per row */
  long overlap;
  int nonIdentityMatrixP=(trans->matrix.xx!=1<<16 || trans->matrix.yy!=1<<16 ||
                          trans->matrix.xy!=0 || trans->matrix.yx!=0);
  
  int leftSideBearing, rightSideBearing, characterWidth, rawCharacterWidth,
    ascent, descent;
  
  /* Macros for transforming F26.6 values into pixels. */

#define TRANSFORM_X(x_value) \
  ((int)((((double)(x_value)*(double)trans->matrix.xx)/\
          (TWO_SIXTEENTH*TWO_SIXTH))+0.5))
    
#define TRANSFORM_Y(y_value) \
  ((int)((((double)(y_value)*(double)trans->matrix.yy)/\
           (TWO_SIXTEENTH*TWO_SIXTH))+0.5))

#define TRANSFORM_RAW(value) \
  ((int)((double)(value)/trans->scale/TWO_SIXTH/(vals->y/72.0)*1000.0+0.5))
    
  TT_Get_Glyph_Metrics(glyph, &metrics);

  if(nonIdentityMatrixP) {
    TT_Get_Glyph_Outline(glyph, &outline);
    TT_Transform_Outline(&outline, &trans->matrix);
    TT_Get_Outline_BBox(&outline, &outline_bbox);
    bbox=&outline_bbox;
  } else {
    bbox=&metrics.bbox;
  }

  xoff=(63-bbox->xMin) & -64;
  yoff=(63-bbox->yMin) & -64;
  wd=(bbox->xMax+63+xoff) >> 6;
  ht=(bbox->yMax+63+yoff) >> 6;

  /* The X convention is to consider a character with an empty
   * bounding box as undefined.  This convention is broken. */
  if(wd<=0) wd=1;
  if(ht<=0) ht=1;

  /* Compute the number of bytes per row. */
  bpr=((wd+(bmfmt->glyph<<3)-1)>>3) & -bmfmt->glyph;
  if(tgp) {
    raster.flow = TT_Flow_Down;
    raster.rows=ht;
    raster.width=wd;
    raster.cols=bpr;
    raster.size = (long)raster.rows*raster.cols;
    tgp->bits=raster.bitmap=(void*)xalloc(ht*bpr);
    if(tgp->bits==0)
      return AllocError;
    memset(raster.bitmap,0,(int)raster.size);

    /* The original version of this code used to go to quite a bit of
     * trouble to make sure that it had the optimal bounding box.
     * Trusting the library instead makes the code simpler, avoids
     * copying bitmaps to and fro, and allows us not to rasterise when
     * we're only being asked for font info. */
    
    if(TT_Get_Glyph_Bitmap(glyph,&raster,xoff,yoff)) {
      MUMBLE("Failed to draw bitmap\n");
      xfree(tgp->bits);
      return BadFontFormat;
    }

    /*  Munge happily with bit and byte order */
    if(bmfmt->bit==LSBFirst)
      BitOrderInvert((unsigned char *)(tgp->bits), ht*bpr);
    
    if(bmfmt->byte!=bmfmt->bit)
      switch(bmfmt->scan) {
      case 1:
        break;
      case 2:
        TwoByteSwap((unsigned char *)(tgp->bits), ht*bpr);
        break;
      case 4:
        FourByteSwap((unsigned char *)(tgp->bits), ht*bpr);
        break;
      case 8:                   /* no util function for 64 bits! */
        {
          int i,j;
          char c, *cp=tgp->bits;
          for(i=ht*bpr; i>=0; i-=8, cp+=8)
            for(j=0; j<4; j++) {
              c=cp[j];
              cp[j]=cp[7-j];
              cp[7-j]=cp[j];
            }
        }
        break;
      default:
        ;
      }
  }
  xoff_pixels = xoff>>6;
  yoff_pixels = yoff>>6;

  /* Determine the glyph metrics. */
  leftSideBearing = -xoff_pixels;
  rightSideBearing = wd - xoff_pixels;
           
  characterWidth = TRANSFORM_X(metrics.advance);
  rawCharacterWidth = TRANSFORM_RAW(metrics.advance);
  ascent = ht - yoff_pixels;
  descent = yoff_pixels;

  /* Adjust the min and max font metrics. */

  ADJUSTMIN(info->minbounds.ascent,ascent);
  ADJUSTMAX(info->maxbounds.ascent,ascent);
  ADJUSTMIN(info->minbounds.descent,descent);
  ADJUSTMAX(info->maxbounds.descent,descent);
  ADJUSTMIN(info->minbounds.leftSideBearing,
            leftSideBearing);
  ADJUSTMAX(info->maxbounds.leftSideBearing,
            leftSideBearing);
  ADJUSTMIN(info->minbounds.rightSideBearing,
            rightSideBearing);
  ADJUSTMAX(info->maxbounds.rightSideBearing,
            rightSideBearing);
  ADJUSTMIN(info->minbounds.characterWidth, characterWidth);
  ADJUSTMAX(info->maxbounds.characterWidth, characterWidth);
  /* cannot use ADJUST* as attributes is an unsigned field */
  if(rawCharacterWidth<(short)info->minbounds.attributes)
    info->minbounds.attributes=(unsigned short)((short)rawCharacterWidth);
  if(rawCharacterWidth>(short)info->maxbounds.attributes)
    info->maxbounds.attributes=(unsigned short)((short)rawCharacterWidth);
  
  /* Determine the maximum overlap.  Is this code wrong? */
  overlap = rightSideBearing - characterWidth;
  ADJUSTMAX(info->maxOverlap, overlap);

  /* Increment the width accumulator. */
  (*totalWidth) += characterWidth;
  (*rawTotalWidth) += rawCharacterWidth;

  if(tgp) {
    /* Set the glyph metrics. */
    tgp->metrics.attributes = (unsigned short)((short)rawCharacterWidth);
    tgp->metrics.leftSideBearing = leftSideBearing;
    tgp->metrics.rightSideBearing = rightSideBearing;
    tgp->metrics.characterWidth = characterWidth;
    tgp->metrics.ascent = ascent;
    tgp->metrics.descent = descent;
  }

  return Successful;
#undef TRANSFORM_X
#undef TRANSFORM_Y
#undef TRANSFORM_RAW
}


/* Locate a code in the encoding vector. 
 * Currently, we use our own encoding vector, completely unrelated to
 * the rest of the world, and do a binary search in it for every
 * single character.
 * This means that the time needed to get m characters from an
 * encoding that has n realized characters takes time m*n*logn.  This
 * is okay for small (8 bit) encodings, but not for bigger ones.
 * The simple solution -- mapping from codes to indexes in the
 * encoding array -- is not a good idea as TrueType fonts tend to be
 * rather sparse (we'd often have a 256Kb encoding vector for a font
 * containing only a few hundred characters).
 * This is not a big deal as we spend most time in the rasteriser
 * anyway.
 */

static int
FreeTypeFindCode(unsigned code, ttfont_t tf)
{
  int l, m, r;
  if (code>=tf->encoding[0] && code<=tf->encoding[tf->used-1]) {
    l=0;
    r=tf->used-1;
    while(l<=r){
      m=(l+r)/2;
      if(tf->encoding[m]<code)
        l=m+1;
      else if(tf->encoding[m]>code)
        r=m-1;
      else {
        return m;
      }
    }
  }
  return -1;
}


/* Routines used by X11 to get info and glyphs from the font. */

static int
FreeTypeGetMetrics(FontPtr pFont, unsigned long count, unsigned char *chars,
                   FontEncoding charEncoding, unsigned long *metricCount,
                   xCharInfo **metrics)
{
  unsigned code;
  int idx;
  ttfont_t tf;
  xCharInfo **mp;

  /* The compiler is supposed to initialise all the fields to 0 */
  static xCharInfo noSuchChar;

  /*  MUMBLE1("Get metrics for %ld characters\n", count);*/

  tf = (ttfont_t) pFont->fontPrivate;
  mp = metrics;

  while (count-- > 0) {
    switch (charEncoding) {
    case Linear8Bit: 
    case TwoD8Bit:
      code = *chars++;
      break;
    case Linear16Bit: 
    case TwoD16Bit:
      code = (*chars++ << 8);
      code |= *chars++;
      break;
    }

    if((idx=FreeTypeFindCode(code, tf))>=0)
      *mp++=&tf->glyphs[idx].metrics;
    else
      *mp++=&noSuchChar;
  }
    
  *metricCount = mp - metrics;
  return Successful;
}

static int
FreeTypeGetGlyphs(FontPtr pFont, unsigned long count, unsigned char *chars,
                  FontEncoding charEncoding, unsigned long *glyphCount,
                  CharInfoPtr *glyphs)
{
  int idx;
  unsigned code;
  ttfont_t tf;
  CharInfoPtr *gp;

  tf = (ttfont_t) pFont->fontPrivate;
  gp = glyphs;

  while (count-- > 0) {
    switch (charEncoding) {
    case Linear8Bit: case TwoD8Bit:
      code = *chars++;
      break;
    case Linear16Bit: case TwoD16Bit:
      code = *chars++ << 8; 
      code |= *chars++;
      break;
    }
      
    if((idx=FreeTypeFindCode(code, tf))>=0)
      *gp++ = &tf->glyphs[idx];
    else
      if(tf->defaultGlyph)
        *gp++ = tf->defaultGlyph;
  }
    
  *glyphCount = gp - glyphs;
  return Successful;
}

/* Set up the X font with the various fields it needs.
 * xf may be 0, in which case we only want the info */
static int
FreeTypeSetUpFont(FontPathElementPtr fpe, FontPtr xf, FontInfoPtr info, 
                  fsBitmapFormat format, fsBitmapFormatMask fmask,
                  FontBitmapFormat *bmfmt)
{
  int xrc;

  /* Get the default bitmap format information for this X installation.
   * Also update it for the client if running in the font server. */
  FontDefaultFormat(&bmfmt->bit, &bmfmt->byte, &bmfmt->glyph, &bmfmt->scan);
  if ((xrc = CheckFSFormat(format, fmask, &bmfmt->bit, &bmfmt->byte,
                           &bmfmt->scan, &bmfmt->glyph,
                           &bmfmt->image)) != Successful) {
    MUMBLE1("Aborting after checking FS format: %d\n", xrc);
    return xrc;
  }

  if(xf) {
    xf->refcnt = 0;
    xf->bit = bmfmt->bit;
    xf->byte = bmfmt->byte;
    xf->glyph = bmfmt->glyph;
    xf->scan = bmfmt->scan;
    xf->format = format;
    xf->get_glyphs = FreeTypeGetGlyphs;
    xf->get_metrics = FreeTypeGetMetrics;
    xf->unload_font = FreeTypeUnloadFont;
    xf->unload_glyphs = 0;
    xf->fpe = fpe;
    xf->svrPrivate = 0;
    xf->fontPrivate = 0;        /* we'll set it later */
    xf->fpePrivate = 0;
    xf->maxPrivate = -1;
    xf->devPrivates = 0;
  }

  info->defaultCh = 0;
  info->noOverlap = 0;          /* not updated */
  info->terminalFont = 0;       /* not updated */
  info->constantMetrics = 0;    /* we'll set it later */
  info->constantWidth = 0;      /* we'll set it later */
  info->inkInside = 1;
  info->inkMetrics = 1;
  info->allExist=0;             /* not updated */
  info->drawDirection = LeftToRight; /* we'll set it later */
  info->cachable = 1;           /* we don't do licensing */
  info->anamorphic = 0;         /* can hinting lead to anamorphic scaling? */
  info->maxOverlap = 0;         /* we'll set it later. */
  info->pad = 0;                /* ??? */
  return Successful;
}

/* Routines to load a TrueType font. */

static int
FreeTypeOpenScalable(FontPathElementPtr fpe, FontPtr *ppFont, int flags,
                     FontEntryPtr entry, char *fileName, FontScalablePtr vals,
                     fsBitmapFormat format, fsBitmapFormatMask fmask,
                     FontPtr non_cachable_font)
{
  int xrc;
  FontPtr xf;
  FontBitmapFormat bmfmt;

  MUMBLE1("Open Scalable %s, XLFD=",fileName);
  #ifdef DEBUG_TRUETYPE
  fwrite(entry->name.name, entry->name.length, 1, stdout);
  #endif
  MUMBLE("\n");

  /* Reject ridiculously small values.  Singular matrices are okay. */
  if(MAX(hypot(vals->pixel_matrix[0], vals->pixel_matrix[1]),
         hypot(vals->pixel_matrix[2], vals->pixel_matrix[3]))
     <1.0)
    return BadFontName;

  /* Create an X11 server-side font. */
  if ((xf = (FontPtr)xalloc(sizeof(FontRec))) == 0)
    return AllocError;

  /* set up the necessary fields */
  if((xrc=FreeTypeSetUpFont(fpe, xf, &xf->info, format, fmask, &bmfmt))
     != Successful) {
    xfree(xf);
    return xrc;
  }

  /* Load the font and fill its info structure. */
  if ((xrc = FreeTypeLoadFont(fileName, vals, xf, &xf->info, &bmfmt, entry)) 
      != Successful) {
    /* Free everything up at this level and return the error code. */
    MUMBLE1("Error during load: %d\n",xrc);
    xfree(xf);
    return xrc;
  }

  /* Set the font and return. */
  *ppFont = xf;

  return xrc;
}

/* Routine to get requested font info. */

static int
FreeTypeGetInfoScalable(FontPathElementPtr fpe, FontInfoPtr info,
                        FontEntryPtr entry, FontNamePtr fontName,
                        char *fileName, FontScalablePtr vals)
{
  int xrc;
  FontBitmapFormat bmfmt;

  MUMBLE("Get info, XLFD= ");
  #ifdef DEBUG_TRUETYPE
  fwrite(entry->name.name, entry->name.length, 1, stdout);
  #endif
  MUMBLE("\n");

  if(MAX(hypot(vals->pixel_matrix[0], vals->pixel_matrix[1]),
         hypot(vals->pixel_matrix[2], vals->pixel_matrix[3]))
     <1.0)
    return BadFontName;

  /* Format and fmask are irrelevant but should be valid;
   * we pass 0 as the font pointer so that no rasterisation will be
   * done. */
  if((xrc=FreeTypeSetUpFont(fpe, 0, info, 0, 0, &bmfmt))
     != Successful) {
    return xrc;
  }

  bmfmt.glyph <<= 3;

  if ((xrc = FreeTypeLoadFont(fileName, vals, 0, info, &bmfmt, entry)) 
      != Successful) {
    MUMBLE1("Error during load: %d\n", xrc);
    return xrc;
  }

  return Successful;
}

/* Renderer registration. */

/* Set the capabilities of this renderer. */
#define CAPABILITIES (CAP_CHARSUBSETTING | CAP_MATRIX)

/* Set it up so file names with either upper or lower case can be
 * loaded.  We don't support compressed fonts. */
static FontRendererRec renderers[] = {
  {".ttf", 4, 0, FreeTypeOpenScalable, 0,
   FreeTypeGetInfoScalable, 0, CAPABILITIES},
  {".TTF", 4, 0, FreeTypeOpenScalable, 0,
   FreeTypeGetInfoScalable, 0, CAPABILITIES},
};
static int num_renderers = sizeof(renderers) / sizeof(renderers[0]);

void
FreeTypeRegisterFontFileFunctions(void)
{
  int i;

  for (i = 0; i < num_renderers; i++)
    FontFileRegisterRenderer(&renderers[i]);
}

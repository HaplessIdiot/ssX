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

/* $XFree86: xc/lib/font/FreeType/ftfuncs.h,v 1.4 1998/09/06 07:31:58 dawes Exp $ */

/* Types */

/* defined in atom.c */
Atom MakeAtom(char *, unsigned, int);

/* A font type that is passed around as part of the fontPrivate field.  It is
 * used to get ranges of glyphs and metrics when the X or font server asks for
 * them. */

typedef struct {
  CARD16 *encoding;             /* our index->code */
  CharInfoPtr glyphs;           /* our index->CharInfo */
  CharInfoPtr defaultGlyph;     /* a pointer to the default glyph */
  int size;                     /* length of the vectors */
  int used;                     /* number of entries actually used */
} ttfont_rec, *ttfont_t;

/* A structure that holds bitmap order and padding info. */

typedef struct {
  int bit;                      /* bit order */
  int byte;                     /* byte order */
  int glyph;                    /* glyph pad size */
  int scan;                     /* machine word size */
  int image;                    /* image rectangle info */
} FontBitmapFormat;

/* A structure that holds our format for a linear transformation. */

typedef struct {
  double scale;
  TT_Matrix matrix;
} NormalisedTransformation;

/* Prototypes for local functions */

static void FreeTypeFreeFont(FontPtr, int);
static void FreeTypeUnloadFont(FontPtr);
static int FreeTypeAddProperties(FontScalablePtr, FontInfoPtr, 
                                 char *, TT_Face, TT_Instance,
                                 NormalisedTransformation *, int);
static int FreeTypePrepareRasteriser(char *, TT_Face *, TT_Instance *,
                                     TT_Instance_Metrics *,
                                     TT_Face_Properties *,
                                     TT_Glyph *,
                                     NormalisedTransformation *,
                                     FontScalablePtr);

static int FreeTypeLoadFont(char *, FontScalablePtr, FontPtr, FontInfoPtr,
                            FontBitmapFormat *, FontEntryPtr);
static int FreeTypeLoadGlyph(FontScalablePtr, FontInfoPtr, 
                             TT_Glyph, NormalisedTransformation*,
                             CharInfoPtr, FontBitmapFormat *, long *, long *);
static int FreeTypeFindCode(unsigned, ttfont_t);
static int FreeTypeGetMetrics(FontPtr pFont, unsigned long, unsigned char *,
                              FontEncoding, unsigned long *,
                              xCharInfo **);
static int FreeTypeGetGlyphs(FontPtr, unsigned long, unsigned char *,
                             FontEncoding, unsigned long *,
                             CharInfoPtr *);
static int FreeTypeOpenScalable(FontPathElementPtr, FontPtr *, int,
                                FontEntryPtr, char *, FontScalablePtr,
                                fsBitmapFormat, fsBitmapFormatMask,
                                FontPtr);
static int FreeTypeGetInfoScalable(FontPathElementPtr, FontInfoPtr ,
                                   FontEntryPtr, FontNamePtr,
                                   char *, FontScalablePtr);

/* $XFree86: xc/lib/font/FreeType/ftfuncs.h,v 1.3 1998/09/06 05:05:31 dawes Exp $ */

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

/*
 *  ttf2pfb.c -- TrueType to PostScript Type 1 Font converter.
 * 
 * Author: Chun-Yu Lee <d791013@ce.ntu.edu.tw>
 *
 * The generated output is in a raw Type 1 Font format, required
 * encoders (e.g. t1asm or t1binary) converting to PFA or PFB font.
 *
 * This program was adapted from the ntu2cjk package (part of the
 * LaTeX2e CJK package (by Werner Lemberg <wl@gnu.org>))
 * and those of contributed and test programs in the FreeType archive
 * tree.
 */

/*
 * Requirement:
 *   - the FreeType (v1.0, maybe higher) Library.
 *   - T1asm or similar converter if PFA or PFB format required
 *   - getafm or similar one if AFM font metrics required.
 *   - afm2tfm or similar one if TFM form metrics required.
 *   - CJK package (including the ntu2cjk part) if typesetting LaTeX
 *     documents using the generated fonts.
 *   - dvips 5.66 or higher if self-contained PostScript document
 *     outputs are required. 
 *   - Ghostscript 3.33 or higher to be complement of some packages
 *     listed above.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "freetype.h"

#ifndef lint
char rcsid[] = "$Id: ttf2pfb.c,v 1.1 1999/01/24 03:21:52 dawes Exp $";
#endif

#ifndef TTF2PFB_VERSION
#define TTF2PFB_VERSION "alpha"
#endif

/* Set default values */
#ifndef DEFAULT_PLATFORM_ID
#define DEFAULT_PLATFORM_ID 3   /* for MS platform */
#endif

#ifndef DEFAULT_ENCODING_ID
#define DEFAULT_ENCODING_ID 4   /* for Big-5 encoding (ideally) */
/* #define DEFAULT_ENCODING_ID 1 */   /* for Unicode encoding */
#endif

#define PID_UNICODE 3
#define EID_UNICODE 1
#define PID_SJIS    3
#define EID_SJIS    2
#define PID_GB      3
#define EID_GB      3
#define PID_BIG5    3
#define EID_BIG5    4
#if 0
#define PID_KS      3  /* ???? */
#define EID_KS      5  /* ???? */
#endif

/* The possible values for the `force_enc' variable. */
typedef enum _enc_type
{
  GB = 1, JIS, KS, SJIS, X
} enc_type;

/* A variable to enforce a certain font encoding (if > 0). */
enc_type force_enc = 0;

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#define FAILURE -1
#endif

#define LINELEN 40  /* max # of columns of code range file */

/*
 * Flags and globals
 */ 
int     verbose   = FALSE;  /* messages should be printed or not. */
int     compact   = FALSE;  /* generate compact font? */
float   fontShift = 0; 
/* float   fontShift = -0.15; */ /* shift down each char 15 percent of bbox */

#ifndef UShort
typedef unsigned short UShort;
#endif
#ifndef UChar
typedef unsigned char  UChar;
#endif

#define TT_Flag_On_Curve 1

/* default platform and encoding ID's. */
int     pid = DEFAULT_PLATFORM_ID;
int     eid = DEFAULT_ENCODING_ID;

char*   prog;             /* program name */

/* for orthogonality with fprintf */
#define Fputs(_string_) fprintf(out, "%s\n", _string_)

/* Postscript font related defines and functions */
TT_F26Dot6 lastpsx, lastpsy;

#define Coord(x) (int)(x)
#define PS_LastPt(x,y)  lastpsx = x; lastpsy = y
#define PS_Moveto(x,y)  \
        fprintf(out, "%d %d rmoveto\n", \
                Coord(x-lastpsx),Coord(y-lastpsy)); \
        PS_LastPt(x,y)
#define PS_Lineto(x,y)  \
        fprintf(out, "%d %d rlineto\n", \
                Coord(x-lastpsx),Coord(y-lastpsy)); \
        PS_LastPt(x,y)

/*
 *  Freetype globals.
 */

TT_Engine   engine;
TT_Face     face;
TT_Instance instance;
TT_Glyph    glyph;
TT_CharMap  cmap;
TT_Error    error;

TT_Outline          outline;
TT_Glyph_Metrics    metrics;
TT_Face_Properties  properties;
/* TT_BBox             bbox; */

/* 
 *  Data structures defined for encoding vector
 */

/* A code range file for the encoding vector of a font contains code
   range pairs, each pair a line. Code starting and ending are seperated
   by ' - ', note that the spaces before and after the minus sign are
   significant. Several types of syntax are shown as follows:

   (Note that declared code ranges must be appeared in ascending order.)

   1. Absolute range, i.e. code is two byte number, e.g.:

      0xA140 - 0xA17E
      41280  - 41342
      0xE00000 - 0xE000FF

   The first two lines represent the same range. 

   2. Relative range, i.e. code is one byte number. If the line is ended
   with a colon ':', it is designated the high byte(s) range, otherwise the
   low byte range (only the last one byte). If there is no high byte(s)
   range declared before the low byte range, the lastest defined high
   byte(s) range or '0x00 - 0x00:' will be used. e.g.:

      0xA1 - 0xFE:
        0x40 - 0x7E
        0xA1 - 0xFE

   which is the Big-5 Encoding.

   3. Single code. Similar to absolute or relative range but the second
   number of the range is the same as the first number. E.g.:

      0xA141  ==  0xA141 - 0xA141
      0xA1:   ==  0xA1 - 0xA1:
      0xA1    ==  0xA1 - 0xA1

   4. If the high byte range is declared and there is no low byte range
   declared consecutively, it is assumed the low byte range is '0x00 - 0xFF'.
 
   5. Comment line. A hash mark '#' followed by any stuffs to the end of the
   line is ignored. Blank line is also discarded.

   */

typedef struct _EVHigh
{
  UShort  start, end;
} EVHigh;

typedef struct _EVLow 
{
  UChar  start, end;
} EVLow;

typedef struct _EVcRange 
{
  EVHigh  high;
  UShort  numLowRanges;
  EVLow*  low;
} EVcRange;

typedef struct _EncVec 
{
  UShort     numCodeRanges;
  EVcRange*  codeRange;
} EncVec;

/* Select encoding vector with respect to pid and eid */
EncVec* eVecMap[5][10];

/* Select encoding vector with respect to force_enc */
EncVec* eVecMap_force[10];

/*
  Functions
*/

void
mesg (const char *msg, ...) 
{
  va_list ap;
  va_start (ap, msg);
  vfprintf (stderr, msg, ap);
  va_end (ap);
}

/*
 * Usage
 */   
void
Usage (int eval)
{
#ifdef DEBUG
  mesg ("Usage: %s [-h][-v][-c][-d charcode][-l][-ng][-nc]\n", prog);
#else
  mesg ("Usage: %s [-h][-v][-c]\n", prog);
#endif
  mesg ("\t[-pid id] [-eid id] [-force enc] [-enc file]\n");
  mesg ("\t[-plane pl] [-f fname] [-uid id] [-s shift]\n");
  mesg ("\t[-o output] [-ttf font.ttf | font.ttf]\n");
  mesg ("-h\t\tThis message.\n");
  mesg ("-v\t\tPrint messages during conversion.\n");
  mesg ("-c\t\tCompact font, ie 256 chars a font");
  mesg (" (useful for the CJK package).\n");
#ifdef DEBUG
  mesg ("-d charcode\tDebug CharString for the given character code.\n");
  mesg ("-l\t\tDisplay point labels.\n");
  mesg ("-ng\t\tDo not show glyph outline.\n");
  mesg ("-nc\t\tDo not show control paths.\n");
#endif
  mesg ("-pid id\t\tSet the platform ID [%d].\n", DEFAULT_PLATFORM_ID);
  mesg ("-eid id\t\tSet the encoding ID [%d].\n", DEFAULT_ENCODING_ID);
  mesg ("\t\t(use a strange pid & eid pair to list all possible pid,eid)\n");
  mesg ("-force enc\tForce a certain encoding [none].\n");
  mesg ("\t\tpossible values are GB, JIS, KS, SJIS, and X).\n");
  mesg ("-enc file\tFile contains code ranges [none].\n");
  mesg ("-plane pl\tA single font plane [0x0].\n");
  mesg ("-f fname\tFont name [UNKNOWN].\n");
  mesg ("-uid id\t\tThe first four digits of UID of Type1 font [7000].\n");
  mesg ("-s shift\tY-offset factor [%5.3f].\n", fontShift);
  mesg ("-o outfile\tSet the output filename [stdout].\n");
  mesg ("-ttf ttfpath\tThe TTF font pathname.\n");

  exit(eval);
}

void
fatal (const char *msg, ...) 
{
  va_list ap;
  va_start (ap, msg);
  fprintf (stderr, "%s: ", prog);
  vfprintf (stderr, msg, ap);
  fprintf (stderr, "\n");
  va_end (ap);
  exit (FAILURE);
}

void
fatal_error (const char *msg, ...) 
{
  va_list ap;
  va_start (ap, msg);
  fprintf (stderr, "%s: error code 0x%04lx: ", prog, error);
  vfprintf (stderr, msg, ap);
  fprintf (stderr, "\n");
  va_end (ap);
  exit (FAILURE);
}

/*
 * Realloc a pointer.
 */
void *
re_alloc (void* ptr, size_t size, char* sub)
{
  register void* value = realloc (ptr, size);
  if (value == NULL)
    fatal ("%s: Virtual memory exhausted.\n", sub);
  return value;
}

/* We have to introduce the `dummy' integer to assure correct handling of
   the stack.  Using `UShort' as the first parameter may fail in case
   this type is promoted to a different type (as happens e.g. under
   emx for DOS */
EncVec* 
Alloc_EncVec (int dummy, ...)
{
  va_list    vp;
  EncVec*    encVec = NULL;
  EVcRange*  cRange = NULL;
  EVLow*     evLow  = NULL;
  UShort     numCR, numLows;
  int        i,j;

  va_start (vp, dummy);
  numCR = va_arg (vp, UShort);

  encVec = re_alloc (encVec, 1*sizeof(EncVec), "Alloc_EncVec");
  encVec->numCodeRanges = numCR;

  cRange = re_alloc (cRange, numCR*sizeof(EVcRange), "Alloc_EncVec");
  for (i=0; i<numCR; i++) {
    (cRange+i)->high.start   = va_arg (vp, UShort);
    (cRange+i)->high.end     = va_arg (vp, UShort);
    (cRange+i)->numLowRanges = numLows = va_arg (vp, UShort);
    evLow = NULL;
    evLow = re_alloc (evLow, numLows*sizeof(EVLow), "Alloc_EncVec");
    for (j=0; j<numLows; j++) {
      (evLow+j)->start = va_arg (vp, UChar);
      (evLow+j)->end   = va_arg (vp, UChar);
    }
    (cRange+i)->low          = evLow;
  }
  encVec->codeRange     = cRange;

  va_end (vp);
  return encVec;
}
  
void
Known_Encodings (void)
{
  EncVec*   encVec;

  /* Big-5 encoding */
  encVec = Alloc_EncVec (1, 1, 0xA1, 0xFE, 2, 0x40, 0x7E, 0xA1, 0xFE);
  eVecMap[PID_BIG5][EID_BIG5] = encVec;
  /* eVecMap_force[BIG5] = encVec; */

  /* GB encoding */
  encVec = Alloc_EncVec (2, 1, 0xA1, 0xF7, 1, 0xA1, 0xFE);
  eVecMap[PID_GB][EID_GB] = encVec;
  eVecMap_force[GB] = encVec;

  /* KS encoding */
  encVec = Alloc_EncVec (3, 1, 0xA1, 0xFE, 1, 0xA1, 0xFE);
  /* eVecMap[PID_KS][EID_KS] = encVec; */
  eVecMap_force[KS] = encVec;

  /* JIS encoding */
  encVec = Alloc_EncVec (4, 1, 0xA1, 0xF4, 1, 0xA1, 0xFE);
  /* eVecMap[PID_JIS][EID_JIS] = encVec; */
  eVecMap_force[JIS] = encVec;
  eVecMap_force[X] = encVec;     /* will be internally translated to SJIS */

  /* Unicode encoding */
  encVec = Alloc_EncVec (5, 1, 0x00, 0xFF, 1, 0x00, 0xFF);
  eVecMap[PID_UNICODE][EID_UNICODE] = encVec;
  /* eVecMap_force[UNICODE] = encVec; */

  /* SJIS encoding */
  encVec = Alloc_EncVec (6, 3,  0x0,  0x0, 1, 0xA1, 0xDF,
                            0x81, 0x9F, 2, 0x40, 0x7E, 0x80, 0xFC,
                            0xE0, 0xEF, 2, 0x40, 0x7E, 0x80, 0xFC);
  eVecMap[PID_SJIS][EID_SJIS] = encVec;
  eVecMap_force[SJIS] = encVec;
}

/*
 *  Convert JIS to SJIS encoding.
 */
UShort
JIS_to_SJIS (UShort code)
{
  UShort index;
  UShort cc0 = (code >> 8) & 0xFF;
  UShort cc1 = code & 0xFF;

  index = (cc0 - 0xa1) * (0xfe - 0xa1 + 1) + (cc1 - 0xa1);
  cc0 = index / ((0x7e - 0x40 + 1) + (0xfc - 0x80 + 1));
  cc1 = index % ((0x7e - 0x40 + 1) + (0xfc - 0x80 + 1));
  if (cc0 < (0x9f - 0x81 + 1))
    cc0 += 0x81;
  else
    cc0 += 0xe0 - (0x9f - 0x81 + 1);
  if (cc1 < (0x7e - 0x40 + 1))
    cc1 += 0x40;
  else
    cc1 += 0x80 - (0x7E - 0x40 + 1);

  return (cc0 << 8) + cc1;
}

/*
 * Open TTF file and select cmap
 */
int
Init_Font_Engine (char* infile)
{
  short cmapindex, platformID, encodingID, num_cmap;

  error = TT_Init_FreeType (&engine);
  if (error) 
    fatal_error ("couldn't initialize FreeType engine.");
                                /* Open the input file. */
  error = TT_Open_Face (engine, infile, &face);
  if (error) fatal_error ("unable to open input file '%s'", infile);
  /* Load the instance. */
  error = TT_New_Instance (face, &instance);
  if (error) fatal_error ("couldn't create instance.");

  error = TT_Set_Instance_Resolutions (instance, 600, 600);
  if (error) fatal_error ("error setting resoluations.");
  error = TT_Set_Instance_CharSize (instance, 120*64);
  if (error) fatal_error ("error setting character size.");

  error = TT_New_Glyph (face, &glyph);
  if (error) fatal_error ("couldn't create new glyph.");
                                /* Get the requested cmap. */
  num_cmap = TT_Get_CharMap_Count (face);
  for (cmapindex=0; cmapindex < num_cmap; cmapindex++) {
    TT_Get_CharMap_ID (face, cmapindex, &platformID, &encodingID);
    if (platformID == pid && encodingID == eid) break;
  }
  if (cmapindex == num_cmap) { 
    mesg ("Possible platform and encoding ID pairs:");
    for (cmapindex=0; cmapindex < num_cmap; cmapindex++) {
      TT_Get_CharMap_ID (face, cmapindex, &platformID, &encodingID);
      mesg ("  (%d, %d)", platformID, encodingID);
    }
    mesg ("\n");
    fatal ("No character map for given platform %d encoding %d.", pid, eid);
  }
  /*  malloc for glyph data */
  error = TT_Get_CharMap (face, cmapindex, &cmap);
  if (error) fatal_error ("cannot load cmap.");
  
  return TRUE;
}   

/* 
 *  Load glyph's outline and metrics
 */
int
LoadTrueTypeChar (int idx)
{
  TT_Matrix scale = {(1<<16)/64, 0, 0, (1<<16)/64};
  error = TT_Load_Glyph (instance, glyph, idx, TTLOAD_DEFAULT);
  if (error) fatal_error ("load glyph");
  error = TT_Get_Glyph_Outline (glyph, &outline);
  if (error) fatal_error ("get glyph outlines.");
  TT_Transform_Outline (&outline, &scale);
  error = TT_Get_Glyph_Metrics (glyph, &metrics);
  if (error) fatal_error ("get glyph_metrics.");

  return TRUE;
}

/*
 *  Header of Type1 font.
 */
void 
PS_Head(FILE *out, int plane, EncVec* planeEV, char* font, int UID) 
{
  EVcRange* cRange = planeEV->codeRange; 
  UShort    numCR  = planeEV->numCodeRanges;
  int       cjk = 0, nGlyph = 0, irange;
  EVLow*    pLow = cRange->low;
  UShort    nLow = cRange->numLowRanges;
  int       ipl, ilow, ich;


  fprintf(out, "%%!FontType1-1.0: CJK-%s 001.001\n",font);
  fprintf(out, "%%%%Creator: %s ", prog);
  fprintf(out, "%s %s\n", TTF2PFB_VERSION, rcsid);
  Fputs("%%CreationDate: Feb 1st, 1998");
  Fputs("%%VMusage: 030000 030000");
  Fputs("11 dict begin");
  Fputs("/FontInfo 8 dict dup begin");
  Fputs("/version (001.001) readonly def");
  if (compact)
    fprintf(out, "/FullName (%s%02d) readonly def\n",font,plane);
  else
    fprintf(out, "/FullName (%s%02X) readonly def\n",font,plane);
  fprintf(out, "/FamilyName (%s) readonly def\n",font);
  Fputs("/Weight (Regular) readonly def");
  Fputs("/ItalicAngle 0 def");
  Fputs("/isFixedPitch false def");
  /*  Fputs("/UnderlineThickness 50 def"); */
  Fputs("end readonly def");
  if (compact)
    fprintf(out, "/FontName /%s%02d def\n",font,plane);
  else
    fprintf(out, "/FontName /%s%02X def\n",font,plane);
  Fputs("/PaintType 0 def");
  Fputs("/FontType 1 def");
  if (fontShift == 0) 
    Fputs("/FontMatrix [0.001 0 0 0.001 0 0] readonly def");
  else
    fprintf(out, "/FontMatrix [0.001 0 0 0.001 0 %5.3f] readonly def\n",
            fontShift);
  Fputs("/Encoding 256 array");
  Fputs("0 1 255 {1 index exch /.notdef put} for");
                                /* encoding vector */
  for (irange=0; irange<numCR; irange++, cRange++) {
    pLow = cRange->low;
    nLow = cRange->numLowRanges;
    for (ipl=cRange->high.start; ipl<=cRange->high.end; ipl++) {
      if (nLow == 0) {
        nGlyph = 0x100;
        for (ich=0; ich<=0xff; ich++)
          fprintf(out, "dup %d /cjk%02X%02X put\n",ich,ipl,ich);
      } else
        for (ilow = 0; ilow < nLow; ilow++, pLow++) {
          if (!compact) cjk = pLow->start;
          nGlyph += pLow->end - pLow->start + 1;
          for (ich=pLow->start; ich<=pLow->end; ich++, cjk++)
            fprintf(out, "dup %d /cjk%02X%02X put\n",cjk,ipl,ich);
        }
    }
  }

  Fputs("readonly def");
  Fputs("/FontBBox [0 -300 1000 1000] readonly def");
  fprintf(out, "/UniqueID %d%03d def\n",UID,plane);
  Fputs("currentdict end");
  Fputs("currentfile eexec");
  Fputs("dup /Private 8 dict dup begin");
  Fputs("/-| { string currentfile exch readstring pop } executeonly def");
  Fputs("/|- { noaccess def } executeonly def");
  Fputs("/| { noaccess put } executeonly def");
  Fputs("/BlueValues [ ] |-");
  Fputs("/ForceBold true def");
  Fputs("/LanguageGroup 1 def");
  Fputs("/RndStemUp false def");
  Fputs("/MinFeature{16 16} |-");
  Fputs("/password 5839 def");
  fprintf(out, "/UniqueID %d%03d def\n",UID,plane);
  Fputs("/Subrs 4 array");
  Fputs("dup 0 { 3 0 callothersubr pop pop setcurrentpoint return } |");
  Fputs("dup 1 { 0 1 callothersubr return } |");
  Fputs("dup 2 { 0 2 callothersubr return } |");
  Fputs("dup 3 { return } |");
  Fputs("|-");
  fprintf(out, "2 index /CharStrings %d dict dup begin\n", nGlyph+1);
}

/*
 * tail of Type1 font
 */
void
PS_Tail (FILE *out)
{
  Fputs("/.notdef { 0 250 hsbw endchar } |-");
  Fputs("end end readonly put noaccess put");
  Fputs("dup /FontName get exch definefont pop");
  Fputs("mark currentfile closefile");
}

/*
 *  use the rrcurveto command on more than one off points  
 */
void 
PS_Curveto (FILE *out, TT_F26Dot6 x, TT_F26Dot6 y, int s, int e)
{
  int  N, i;
  TT_F26Dot6 sx[3], sy[3], cx[4], cy[4];

  N = e-s+1;
  cx[0] = lastpsx; cy[0] = lastpsy;
  if (s == e) {
    cx[1] = (2*outline.points[s].x+outline.points[s-1].x)/3;
    cy[1] = (2*outline.points[s].y+outline.points[s-1].y)/3;
    cx[2] = (2*outline.points[s].x+x)/3;
    cy[2] = (2*outline.points[s].y+y)/3;
    cx[3] = x;
    cy[3] = y;
    fprintf(out, "%d %d %d %d %d %d rrcurveto\n",
            Coord(cx[1]-cx[0]),Coord(cy[1]-cy[0]),
            Coord(cx[2]-cx[1]),Coord(cy[2]-cy[1]),
            Coord(cx[3]-cx[2]),Coord(cy[3]-cy[2]));
  } else {
    for(i=0; i<N; i++) {
      sx[0] = (i==0) ? outline.points[s-1].x :
        (outline.points[i+s].x+outline.points[i+s-1].x)/2;
      sy[0] = (i==0) ? outline.points[s-1].y :
        (outline.points[i+s].y+outline.points[i+s-1].y)/2;
      sx[1] = outline.points[s+i].x;
      sy[1] = outline.points[s+i].y;
      sx[2] = (i==N-1) ? x : (outline.points[s+i].x+outline.points[s+i+1].x)/2;
      sy[2] = (i==N-1) ? y : (outline.points[s+i].y+outline.points[s+i+1].y)/2;
      cx[1] = (2*sx[1]+sx[0])/3;
      cy[1] = (2*sy[1]+sy[0])/3;
      cx[2] = (2*sx[1]+sx[2])/3;
      cy[2] = (2*sy[1]+sy[2])/3;
      cx[3] = sx[2];
      cy[3] = sy[2];
      fprintf(out, "%d %d %d %d %d %d rrcurveto\n",
              Coord(cx[1]-cx[0]),Coord(cy[1]-cy[0]),
              Coord(cx[2]-cx[1]),Coord(cy[2]-cy[1]),
              Coord(cx[3]-cx[2]),Coord(cy[3]-cy[2]));
      cx[0] = cx[3]; cy[0] = cy[3];
    }
  }
  PS_LastPt(x,y);
}

#ifdef DEBUG
int     debug_Char_Code = 0xFFFF;
FILE*   tmpout;
int     showlabel = FALSE;
int     no_glyph  = FALSE;
int     no_control= FALSE;
#define Fputps(_msg_) fprintf(tmpout, "%s\n", _msg_)

void 
tmp_out (FILE* tmpout)
{
  int i, j;
  /* backup the line for easy coding this postscript program */
  /*
  fprintf(tmpout,"/llx %d.0 def /lly %d.0 def /urx %d.0 def /ury %d.0 def\n",
          Coord(metrics.bbox.xMin/64), Coord(metrics.bbox.yMin/64),
          Coord(metrics.bbox.xMax/64), Coord(metrics.bbox.yMax/64));
  */

  Fputps("%!PS");
  Fputps("%%% CharString debugging program.");
  Fputps("%%% Generated by: ttf2pfb $Revision: 1.1 $");
  Fputps("%%% plot char-string (pathes defined in /cjkxxxx)");
  Fputps("");
  Fputps("%%% user-defined parameter");
  Fputps("/scalefactor .6 def");
  Fputps("%% 0 black, 1 white");
  Fputps("/glyph-outline-gray 0 def");
  Fputps("/control-point-gray 0.7 def");
  Fputps("");
  Fputps("%%% calculate shifts and scale factor");
  Fputps("currentpagedevice /PageSize get dup");
  Fputps("0 get /pagewidth exch def");
  Fputps("1 get /pageheight exch def");
  Fputps("");
  fprintf(tmpout,"/llx %d.0 def /lly %d.0 def /urx %d.0 def /ury %d.0 def\n",
          Coord(metrics.bbox.xMin/64), Coord(metrics.bbox.yMin/64),
          Coord(metrics.bbox.xMax/64), Coord(metrics.bbox.yMax/64));
  Fputps("/olwidth urx llx sub def");
  Fputps("/olheight ury lly sub def");
  Fputps("");
  Fputps("/scale scalefactor pagewidth mul olwidth div def");
  Fputps("/xshift pagewidth 1 scalefactor sub mul 2 div def");
  Fputps("/yshift pageheight olheight scale mul sub 2 div def");
  Fputps("");
  Fputps("%% save old gray-scale value");
  Fputps("/oldgray currentgray def");
  Fputps("");
  Fputps("%%% for point sequence label");
  Fputps("/TimesRoman 8 selectfont");
  Fputps("/i++ {i /i i 1 add def} def");
  Fputps("/itos {4 string cvs} def");
  Fputps("/point {2 copy i++ 3 1 roll 5 3 roll} def");
  Fputps("/drawlabel");
  Fputps("  {{moveto dup 0 eq {exit}");
  Fputps("    {itos show} ifelse} loop pop} def");
  Fputps("/nodrawlabel {clear} def");
  Fputps("/i 0 def");
  Fputps("");
  Fputps("%%% for drawing glyph paths, redefine commands used in CharString");
  Fputps("%% scaled to proper size");
  Fputps("/addr {scale mul 3 -1 roll add 3 1 roll");
  Fputps("       scale mul add exch 2 copy} def");
  if (no_glyph) {
    Fputps("/rmoveto {addr pop pop point} def");
    Fputps("/rlineto {addr pop pop point} def");
    Fputps("/rrcurveto {8 4 roll addr 8 -2 roll addr 8 -2 roll addr");
    Fputps("            8 2 roll 6 {pop} repeat point} def");
  } else {
    Fputps("/rmoveto {addr moveto point} def");
    Fputps("/rlineto {addr lineto point} def");
    Fputps("/rrcurveto {8 4 roll addr 8 -2 roll addr 8 -2 roll addr");
    Fputps("            8 2 roll curveto point} def");
  }
  Fputps("/hsbw {pop pop");
  Fputps("  xshift llx scale mul sub");
  Fputps("  yshift lly scale mul sub} def");
  Fputps("/endchar {stroke pop pop} def");
  Fputps("");
  Fputps("%%% for drawing control paths");
  Fputps("/T {pop lly sub scale mul yshift add exch");
  Fputps("        llx sub scale mul xshift add exch } def");
  Fputps("/mt {T 2 copy moveto} def");
  if (no_control) 
    Fputps("/lt {T} def");
  else
    Fputps("/lt {T 2 copy lineto} def");
  Fputps("");
  Fputps("1 setlinecap 1 setlinejoin");
  Fputps("%%% draw control points and paths");
  Fputps("control-point-gray setgray");

  for (i = 0, j = 0; i < outline.n_contours; i++) {
    fprintf (tmpout, "\n");
    fprintf (tmpout, "%d %d %d %d mt\n", j, Coord(outline.points[j].x),
             Coord(outline.points[j].y), outline.flags[j]);
    j++;
    for (; j <= outline.contours[i]; j++) 
      fprintf (tmpout, "%d %d %d %d lt\n", j, Coord(outline.points[j].x),
               Coord(outline.points[j].y), outline.flags[j]);
    Fputps("closepath");
  }
  Fputps("stroke");
  if (showlabel && !no_control)
    Fputps("drawlabel");
  else
    Fputps("nodrawlabel");
  Fputps("");
  Fputps("%%% draw glyph outlines");
  Fputps("glyph-outline-gray setgray");
  Fputps("");
}
#endif

/*
 * Construct CharString of a glyph
 */
short
PS_CharString (FILE *out, UShort char_Code) 
{
  int    idx, i, j;
  UShort start_offpt, end_offpt = 0, fst;
#if DEBUG
  FILE* oldout = out;
  int   loop   = 1;
#endif

  if (force_enc == X)
    char_Code = JIS_to_SJIS (char_Code);

  idx = TT_Char_Index (cmap, char_Code);
  if (idx == 0) {
    fprintf(out, "/cjk%04X { 0 250 hsbw endchar } |-\n", char_Code);
    return FALSE;
  }

  if (!LoadTrueTypeChar(idx))
    fatal ("couldn't load character: %d, code= %d", idx, char_Code);

  /* Begin string */
  fprintf(out, "/cjk%04X {\n", char_Code);

#ifdef DEBUG
  if (char_Code == debug_Char_Code) {
    tmp_out(tmpout);
    out  = tmpout;
    loop = 0;
  }
  for (;loop<2;loop++) {
#endif

  /*  coordinates are all relative to (0,0) in FreeType */
  fprintf(out, "0 %d hsbw\n", (int)(metrics.advance/64));

  /* Initialize ending contour point, relative coordinates */
  lastpsx = lastpsy = 0;

  for (i = 0, j = 0; i < outline.n_contours /*contours*/; i++) {
    fst = j;
    PS_Moveto (outline.points[j].x, outline.points[j].y);
    j++;

    start_offpt = 0; /*start at least 1*/
    /* data pts for all contours stored in one array.
       each round j init at last j + 1 */

    /* start_offpt means start of off points.
       0 means no off points in record.
       N means the position of the off point.
       end_offpt means the ending off point.
       lastx, lasty is the last ON point from which Curve and Line
       shall start.
       */

    /* start with j=0. into loop, j=1.
       if pt[1] off, if start_offpt == 0, toggle start_offpt
       next j=2. if on, now start_off != 0, run Curveto.
       if pt[1] on, start_off == 0, will run Lineto.
       */
    for (; j <= outline.contours[i]; j++) {
      if (!(outline.flags[j] & TT_Flag_On_Curve)) {
        if (!start_offpt) { start_offpt = end_offpt = j; }
        else end_offpt++;
      }
      else {                    /*On Curve*/
        if (start_offpt) {
          /* start_offpt stuck at j, end_offpt++.
             end_offpt - start_offpt gives no of off pts.
             start_offpt gives start of sequence.
             why need outline.xCoord[j] outline.yCoord[j]?
             */
          PS_Curveto(out, outline.points[j].x, outline.points[j].y,
                     start_offpt, end_offpt);
          start_offpt = 0;
            
          /* also use start_offpt as indicator to save one variable!! 
             after curveto, reset condition. */
        }
        else PS_Lineto(outline.points[j].x, outline.points[j].y);
      }
    }
    /* looks like closepath fst = first, i.e. go back to first */
    if (start_offpt)
      PS_Curveto(out, outline.points[fst].x, outline.points[fst].y,
                 start_offpt, end_offpt);
    else
      Fputs("closepath"); 
  }

  Fputs("endchar");

#if DEBUG
    out = oldout;
  }
  if (char_Code == debug_Char_Code) {
    if (showlabel && !no_glyph)
      Fputps("drawlabel");
    else
      Fputps("nodrawlabel");
    Fputps("");
    Fputps("%%% end of drawing");
    Fputps("oldgray setgray");
    Fputps("showpage");
    fclose(tmpout);
  }
#endif

  Fputs(" } |-");
  return TRUE;
}

/*
 *  Get code ranges of an encoding scheme either from 
 *  the eVecMap or an code range file.
 */
EncVec*
Get_EncVec (FILE *enc)
{
  EncVec*    encVec = NULL;
  EVcRange*  cRange = NULL;
  EVLow*     lByte  = NULL;
  UShort     numCR = 0, numLow = 0;
  int        start, end;
  int        buflen = LINELEN, numAssigned;
  char       buf[LINELEN];

  if (force_enc != 0)
    return eVecMap_force[force_enc];

  if (enc == NULL && eVecMap[pid][eid] != NULL)
    return eVecMap[pid][eid];
                                /* parse each code range line */
  while (fgets(buf, buflen, enc) != NULL) {
    if (buf[0] != '#' && buf[0] != '\n') {
      if (strrchr(buf,':') != NULL) {
                                /* if there is no high declared before low */
        if (lByte != NULL) {
          if (cRange == NULL) {
                                /* default code range '0x00-0x00:' */
            cRange = re_alloc (cRange, ++numCR*sizeof(EVcRange), "Get_EncVec");
            cRange->high.start = cRange->high.end = 0;
          }
                                /* Assign the lastest low */
          cRange->low = lByte;
          cRange->numLowRanges = numLow;
        }
                                /* New high byte range */
        cRange = re_alloc (cRange, ++numCR*sizeof(EVcRange), "Get_EncVec");
        (cRange+numCR-1)->numLowRanges = numLow = 0;
        lByte = NULL;
                                /* Parse code range */
        numAssigned = sscanf (buf, "%i %*40s %i", &start, &end);
        if (numAssigned <= 0 || numAssigned > 2) {
          mesg ("%s: Get_EncVec: wrong code range.\n", prog);
          return NULL;
        } else {
          (cRange+numCR-1)->high.start = start;
          if (numAssigned == 1)
            (cRange+numCR-1)->high.end = start;
          else
            (cRange+numCR-1)->high.end = end;
        }
      } else {
        lByte = re_alloc (lByte, ++numLow*sizeof(EVLow), "Get_EncVec");
        numAssigned = sscanf (buf, "%i %*40s %i", &start, &end);
        if (numAssigned <= 0 || numAssigned > 2) {
          mesg ("%s: Get_EncVec: wrong code range.\n", prog);
          return NULL;
        } else {
          (lByte+numLow-1)->start = start;
          if (numAssigned == 1)
            (lByte+numLow-1)->end = start;
          else
            (lByte+numLow-1)->end = end;
        }
      }
    }
  }
  if (cRange == NULL) {
    cRange = re_alloc (cRange, ++numCR*sizeof(EVcRange), "Get_EncVec");
    cRange->high.start = cRange->high.end = 0;
    cRange->numLowRanges = 0;
  }
  if (lByte != NULL) {
    cRange->low = lByte;
    cRange->numLowRanges = numLow;
  }
  encVec = re_alloc (encVec, 1*sizeof(EncVec), "Get_EncVec");
  encVec->numCodeRanges = numCR;
  encVec->codeRange = cRange;
  return encVec;
}

/*
 *  Match code ranges by a font plane. 
 */

EncVec*
Get_PlaneEV (EncVec* encVec, int plane)
{
  UShort    numCR  = encVec->numCodeRanges;
  EVcRange* cRange = encVec->codeRange;

  EncVec*   encV = NULL;
  EVcRange* planeCR = NULL;
  EVLow*    planeLow = NULL;
  UShort    nCR = 0, nLow = 0;

  int       icr;

  if (compact) {
    int iChar = 0;              /* summed # of chars */
    int nChar = (plane-1) * 256; /* the first char code ranges recorded */
    int recording = 0;
                                /* if compact, plane starts from 1 to be */
                                /* compatible to the CJK package */
    if (plane < 1 || plane > 256) 
      fatal ("Get_PlaneEV: given plane out of range.");

    for (icr = 0; icr < numCR; icr++, cRange++) {
      UShort numLow = cRange->numLowRanges;
      int    ipl;

      for (ipl=cRange->high.start; ipl<=cRange->high.end; ipl++) {
        EVLow* pLow   = cRange->low;
        int ilow;
                                
        if (recording) {        /* if we have made a hit */
          if (planeLow != NULL) { /* if low byte range has not been saved */
            (planeCR+nCR-1)->low = planeLow;
            (planeCR+nCR-1)->numLowRanges = nLow;
            planeLow = NULL;
          }
                                /* each new plane starts a EVcRange if */
                                /* iChar is still less than nChar */
          if (iChar <= nChar) {
            planeCR = re_alloc (planeCR, ++nCR*sizeof(EVcRange), "Get_PlaneEV");
            (planeCR+nCR-1)->high.start = (planeCR+nCR-1)->high.end = ipl;
            (planeCR+nCR-1)->numLowRanges = nLow = 0;
          }
        }
                                /* scan each low byte range */
        for (ilow = 0; ilow < (numLow==0?1:numLow); ilow++, pLow++) {
          int start, end, nLowChar;

          if (numLow == 0) {    /* default range */
            start = 0x0;
            end   = 0xff;
          } else { 
            start = pLow->start;
            end   = pLow->end;
          }
          nLowChar = end - start + 1;
          if (iChar+nLowChar > nChar) { /* a hit! */
            int bchar = start + nChar - iChar;
            if (planeCR == NULL) {  /* the first time code range is recorded */
              planeCR = re_alloc (planeCR, ++nCR*sizeof(EVcRange), "Get_PlaneEV");
              (planeCR+nCR-1)->high.start = ipl;
              (planeCR+nCR-1)->high.end   = ipl;
            }
                                /* adjust range boundary */
            if (recording==0)
              start = bchar;
            else 
              end = bchar; 
            nChar += 0xff;
                                /* recording starts */
            recording++;
          }
          iChar += nLowChar;    /* next range */

          if (recording) {
                                /* a new low range */
            if (iChar <= nChar) {
              planeLow = re_alloc (planeLow, ++nLow*sizeof(EVLow), "Get_PlaneEV");
              (planeLow+nLow-1)->start = start;
              (planeLow+nLow-1)->end   = end;
            }
            if (recording > 1 || iChar > nChar) { /* beyond recording range */
              (planeCR+nCR-1)->numLowRanges = nLow;
              (planeCR+nCR-1)->low = planeLow;
              encV = re_alloc (encV, 1*sizeof(EncVec), "Get_PlaneEV");
              encV->numCodeRanges = nCR;
              encV->codeRange = planeCR;
              return encV;
            }
          }
        }
      }
    }
  } else 
    for (icr = 0; icr < numCR; icr++, cRange++) {
      if (plane >= cRange->high.start && plane <= cRange->high.end) {
        encV = re_alloc (encV, 1*sizeof(EncVec), "Get_PlaneEV");
        planeCR = re_alloc (planeCR, 1*sizeof(EVcRange), "Get_PlaneEV");

        planeCR->high.start = planeCR->high.end = plane;
        planeCR->numLowRanges = cRange->numLowRanges;
        planeCR->low = cRange->low;
        encV->numCodeRanges = 1;
        encV->codeRange = planeCR;
        return encV;
      }
    }
  return NULL;
}

/*
 *  The main subroutine for generate Type1 font.
 *  One subfont per call.
 */
short
Generate_Fonts (FILE *out, int plane, FILE *enc, char *fname, int pfxUID)
{
  EncVec*    encVec = Get_EncVec (enc);
  EncVec*    planeEncVec;
  EVcRange*  cRange;
  UShort     numCR;
  UShort     code;
  int        ilow, iplan, ichar, irange;
  
  if (encVec == NULL)
    return FALSE;
  if ((planeEncVec = Get_PlaneEV (encVec, plane)) == NULL) {
    mesg ("%s: can't find encoding vector for the font plane 0x%X.\n",
         prog, plane);
    return FALSE;
  }
                                /* Header of Type1 font */
  PS_Head (out, plane, planeEncVec, fname, pfxUID);
  
  numCR  = planeEncVec->numCodeRanges;
  cRange = planeEncVec->codeRange;
  for (irange=0; irange<numCR; irange++, cRange++) {
    EVLow* pLow = cRange->low;
    UShort nLow = cRange->numLowRanges;
    for (iplan=cRange->high.start; iplan<=cRange->high.end; iplan++) {
      if (nLow == 0) {
        for (ichar=0; ichar<=0xff; ichar++) {
          code = iplan<<8 | ichar;
          PS_CharString (out, code);
        }
      } else
        for (ilow = 0; ilow < nLow; ilow++, pLow++)
          for (ichar=pLow->start; ichar<=pLow->end; ichar++) {
            code = iplan<<8 | ichar;
            PS_CharString (out, code);
          }
    }
  }
  PS_Tail (out);
  
  return TRUE;
}

/*
 *  Main: process options, file I/O, etc.
 */
void
main(int argc, char *argv[])
{
  char  *infile, *outfile, *encFile, *fname = "UNKNOWN";
  FILE  *out, *enc;
  int   result, plane = 0, pfxUID = 7000;
   
  if ((prog = strrchr(argv[0], '/')))
    prog++;
  else
    prog = argv[0];
   
  /* setup encoding vectors of all I known */
  Known_Encodings ();

  out    = stdout;
  enc    = NULL;
  infile = outfile = encFile = NULL;
   
  argc--;
  argv++;
   
  while (argc > 0) {
    if (argv[0][0] == '-') {
      switch (argv[0][1]) {
      case 'v': case 'V':
        verbose = TRUE;
        break;
      case 'c': case 'C':
        compact = TRUE;
        break;
      case 'p': case 'P':
        result = argv[0][2];
        argc--;
        argv++;
        if (result == 'i' || result == 'I') {
          /* Set the platform ID. Assumed upper bound is 64 */
          if ((pid = atoi(argv[0])) < 0 || pid > 64) 
                                /* Check the platform and encoding IDs. */
            fatal ("%s: invalid platform ID '%d'.\n", prog, pid);
        }
        else if (result == 'l' || result == 'L') {
          result = 0;
          while (argv[0][result] == '0' &&
                 toupper(argv[0][result+1]) != 'X')
            result++; /* no octal number */
          sscanf (&argv[0][result], "%i", &plane);
        }
        break;
      case 'e': case 'E':
                                /* Set the encoding ID. */
        result = argv[0][2];
        argc--;
        argv++;
        if (result == 'i' || result == 'I') {
          if ((eid = atoi(argv[0])) < 0 || eid > 64) {
            fatal ("%s: invalid encoding ID '%d'.\n", prog, eid);
            exit (1);
          }}
        else if (result == 'n' || result == 'N')
          encFile = argv[0];
        break;
      case 'u': case 'U':
        argc--;
        argv++;
        pfxUID = atoi(argv[0]);
        break;
      case 'f': case 'F':
        result = argv[0][2];
        argc--;
        argv++;
        if (result == '\0')
          fname = argv[0];
        else if (result == 'o' || result == 'O') {
          switch (argv[0][0]) {
          case 'g': case 'G':
            force_enc = GB;
            break;
          case 'k': case 'K':
            force_enc = KS;
            break;
          case 'j': case 'J':
            force_enc = JIS;
            break;
          case 's': case 'S':
            force_enc = SJIS;
            break;
          case 'x': case 'X':
            force_enc = X;
          }
        }
        break;
      case 't': case 'T':
                                /* Get the TTF file name. */
        argc--;
        argv++;
        infile = argv[0];
        break;
      case 'o': case 'O':
                                /* Set the output file name. */
        argc--;
        argv++;
        outfile = argv[0];
        break;
      case 's': case 'S':       /* shift font bbox up or down */
        argc--; argv++;
        sscanf(argv[0], "%f", &fontShift); 
        break;
#ifdef DEBUG
      case 'd': case 'D':        /* character code for debuging */
        argc--; argv++;
        sscanf(argv[0], "%i", &debug_Char_Code); 
        tmpout = fopen("charstr-debug.ps", "wt");
        mesg ("You have specified the character code 0x%04x for debugging.\n",
              debug_Char_Code);
        mesg ("A PostScript program named charstr-debug.ps will be created.\n");
        break;
      case 'l': case 'L':
        showlabel = TRUE;
        break;
      case 'n': case 'N':
        result = argv[0][2];
        if (result == 'g' || result == 'G') 
          no_glyph = TRUE;
        else if (result == 'c' || result == 'C') 
          no_control = TRUE;
        break;
#endif
      default:
        Usage(1);
      }
    } else
                                /* Set the input file name. */
      infile = argv[0];

    argc--;
    argv++;
  }

                                /* Open the output file if specified. */
  if (outfile != NULL)
                                /* Attempt to open the output file. */
    if ((out = fopen(outfile, "wt")) == 0) {
      fatal ("%s: unable to open the output file '%s'.\n",
              prog, outfile);
      exit(1);
    }
 
                                 /* Validate the values passed on the
                                   command line. */
  if (infile == NULL) {
    mesg ("%s: no input TTF file provided.\n", prog);
    Usage(1);
  }

  if (encFile != NULL) {
    if ((enc = fopen(encFile, "rt")) == 0) {
      fatal ("%s: no input code range file.\n", prog);
      exit (1);
    }
  }
                                /* Initialize font engine */
  if (! Init_Font_Engine (infile)) {
    if (out != stdout) {
      fclose(out);
      (void) unlink(outfile);
    }
    exit (1);
  }
                                /* Generate the disassembled PFB font
                                   from the TrueType font.  */
  if (Generate_Fonts (out, plane, enc, fname, pfxUID))
    result = 0;
  else
    result = 2;

  if (out != stdout) {
    fclose(out);
    if (result != 0) {
      mesg ("%s: An error occured when generating", prog);
      mesg (" the font, so delete the output file.\n");
      /* (void) unlink(outfile); */
    }
  }

  TT_Close_Face (face);
  TT_Done_FreeType (engine);

  exit(result);
}


/* end of ttf2pfb.c */

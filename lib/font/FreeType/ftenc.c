/* This code is Copyright (c) 1997 by Mark Leisher
 *              Copyright (c) 1998 by Juliusz Chroboczek
 * 
 * License, to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO ANY IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NONINFRINGEMENT OF THIRD PARTY RIGHTS.  THE ENTIRE RISK AS TO THE
 * QUALITY AND PERFORMANCE OF THE SOFTWARE, INCLUDING ANY DUTY TO SUPPORT
 * OR MAINTAIN, BELONGS TO THE LICENSEE.  SHOULD ANY PORTION OF THE
 * SOFTWARE PROVE DEFECTIVE, THE LICENSEE ASSUMES THE ENTIRE COST OF ALL
 * SERVICING, REPAIR AND CORRECTION.  IN NO EVENT SHALL THE AUTHORS BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE. */

/* $XFree86: xc/lib/font/FreeType/ftenc.c,v 1.6 1998/09/06 05:05:30 dawes Exp $ */

#include "ttconfig.h"
#include "freetype.h"
#include "ft.h"

/* Reencoding functions and search for a suitable CharMap. */

static int ttf_find_cmap(int, int, TT_Face, TT_CharMap *);

/* This structure represents a cmap and eventually a recoding
 * function.  In order to convert a code into an index, we first apply
 * the recoding function and then the cmap. */

struct ttf_encoding_alternative {
  int pid, eid;
  unsigned (*recode)(unsigned);
};

/* This is the structure that holds the info for one charset.  It
 * consists of a charset name, its size, and an alternative like
 * above.  For efficiency reasons, there should be no recoding
 * functions for large charsets (but it should still work). */

struct ttf_encoding_info {
  char *charset;
  int size;
  struct ttf_encoding_alternative *alternatives;
};

/* Of course, one could add millions of tables here.  In order for a table
 * to be suitable for inclusion, it must satisfy the following condition:
 * some fonts will be useless under Unix for some people without it.  This
 * means that an encoding function must go from some encoding used under
 * Unix to either Unicode or an Apple encoding.  Apple encodings should be
 * avoided, as most fonts will have a Unicode table anyway. */

/* What we are going to do just screams for higher-order functions.
 * 
 * (defun recoding-function (table, offset, condition)
 *   (lambda (code) 
 *     (if (funcall condition code) (aref table (- code offset)) code)))
 *
 * As this is C, we'll have to fake them.  There are two ways to do this:
 *  1. split the closure into code and environment (now you know what
 *     the private pointers in fonts and client data pointers in
 *     callbacks really are);
 *  2. change scopes so that all functions can live with a null
 *     environment; this is not possible in general.
 * Option 1 complicates the interface somewhat, so we chose option 2.
 * Is it possible to use the preprocessor for this? */

static struct ttf_encoding_alternative iso10646[]=
{
  {0,0,0},                      /* any Unicode table */
  {-1,-1,0}
};

/* Notice that the Apple encodings do not have all the characters in the
 * corresponding ISO 8859, and the tables have some holes.  There's not
 * much more we can do with fonts without a Unicode cmap unless we are
 * willing to combine cmaps (which we are not). */

static unsigned short 
iso8859_1_apple_roman[]=
{ 0xCA, 0xC1, 0xA2, 0xA3, 0xDB, 0xB4, 0x00, 0xA4,
  0xAC, 0xA9, 0xBB, 0xC7, 0xC2, 0x00, 0xA8, 0xF8,
  0xA1, 0xB1, 0x00, 0x00, 0xAB, 0xB5, 0xA6, 0xE1,
  0xFC, 0x00, 0xBC, 0xC8, 0x00, 0x00, 0x00, 0xC0,
  0xCB, 0xE7, 0xE5, 0xCC, 0x80, 0x81, 0xAE, 0x82,
  0xE9, 0x83, 0xE6, 0xE8, 0xED, 0xEA, 0xEB, 0xEC,
  0x00, 0x84, 0xF1, 0xEE, 0xEF, 0xCD, 0x85, 0x00,
  0xAF, 0xF4, 0xF2, 0xF3, 0x86, 0x00, 0x00, 0xA7,
  0x88, 0x87, 0x89, 0x8B, 0x8A, 0x8C, 0xBE, 0x8D,
  0x8F, 0x8E, 0x90, 0x91, 0x93, 0x92, 0x94, 0x95,
  0x00, 0x96, 0x98, 0x97, 0x99, 0x9B, 0x9A, 0xD6,
  0xBF, 0x9D, 0x9C, 0x9E, 0x9F, 0x00, 0x00, 0xD8 };

static unsigned
iso8859_1_to_apple_roman(unsigned isocode)
{
  if(isocode<=0x80)
    return isocode;
  else if(isocode>=0xA0)
    return iso8859_1_apple_roman[isocode-0xA0];
  else
    return 0;
}

static struct ttf_encoding_alternative iso8859_1[]=
{
  {2,2,0},                      /* ISO 8859-1 */
  {0,0,0},                      /* ISO 8859-1 coincides with Unicode*/
  {1,0,iso8859_1_to_apple_roman},
  {-1,-1,0}
};

static unsigned short iso8859_2_tophalf[]=
{ 0x00A0, 0x0104, 0x02D8, 0x0141, 0x00A4, 0x013D, 0x015A, 0x00A7,
  0x00A8, 0x0160, 0x015E, 0x0164, 0x0179, 0x00AD, 0x017D, 0x017B,
  0x00B0, 0x0105, 0x02DB, 0x0142, 0x00B4, 0x013E, 0x015B, 0x02C7,
  0x00B8, 0x0161, 0x015F, 0x0165, 0x017A, 0x02DD, 0x017E, 0x017C,
  0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
  0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
  0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
  0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
  0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
  0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9};

static unsigned 
iso8859_2_to_unicode(unsigned isocode)
{
  if(isocode<=0xa0)
    return isocode;
  else
    return iso8859_2_tophalf[isocode-0xa0];
}

static unsigned short iso8859_2_apple_centeuro[]=
{ 0xCA, 0x84, 0x00, 0xFC, 0x00, 0xBB, 0xE5, 0xA4,
  0xAC, 0xE1, 0x00, 0xE8, 0x8F, 0x00, 0xEB, 0xFB,
  0xA1, 0x88, 0x00, 0xB8, 0x00, 0xBC, 0xE6, 0xFF,
  0x00, 0xE4, 0x00, 0xE9, 0x90, 0x00, 0xEC, 0xFD,
  0xD9, 0xE7, 0x00, 0x00, 0x80, 0xBD, 0x8C, 0x00,
  0x89, 0x83, 0xA2, 0x00, 0x9D, 0xEA, 0x00, 0x91,
  0x00, 0xC1, 0xC5, 0xEE, 0xEF, 0xCC, 0x85, 0x00,
  0xDB, 0xF1, 0xF2, 0xF4, 0x86, 0xF8, 0x00, 0xA7,
  0xDA, 0x87, 0x00, 0x00, 0x8A, 0xBE, 0x8D, 0x00,
  0x8B, 0x8E, 0xAB, 0x00, 0x9E, 0x92, 0x00, 0x93,
  0x00, 0xC4, 0xCB, 0x97, 0x99, 0xCE, 0x9A, 0xD6,
  0xDE, 0xF3, 0x9C, 0xF5, 0x9F, 0xF9, 0x00, 0x00 };

static unsigned
iso8859_2_to_apple_centeuro(unsigned isocode)
{
  if(isocode<=0x80)
    return isocode;
  else if(isocode>=0xA0)
    return iso8859_2_apple_centeuro[isocode-0xA0];
  else
    return 0;
}


static struct ttf_encoding_alternative iso8859_2[]=
{
  {0,0,iso8859_2_to_unicode},
  {1,29,iso8859_2_to_apple_centeuro},
  {-1,-1,0}
};

static unsigned short iso8859_3_tophalf[]=
{ 0x00A0, 0x0126, 0x02D8, 0x00A3, 0x00A4, 0x0000, 0x0124, 0x00A7,
  0x00A8, 0x0130, 0x015E, 0x011E, 0x0134, 0x00AD, 0x0000, 0x017B,
  0x00B0, 0x0127, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x0125, 0x00B7,
  0x00B8, 0x0131, 0x015F, 0x011F, 0x0135, 0x00BD, 0x0000, 0x017C,
  0x00C0, 0x00C1, 0x00C2, 0x0000, 0x00C4, 0x010A, 0x0108, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x0000, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x0120, 0x00D6, 0x00D7,
  0x011C, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x016C, 0x015C, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E4, 0x010B, 0x0109, 0x00E7, 0x00E8,
  0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x0000,
  0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x0121, 0x00F6, 0x00F7, 0x011D,
  0x00F9, 0x0000, 0x00FA, 0x00FB, 0x00FC, 0x016D, 0x015D, 0x02D9};

static unsigned 
iso8859_3_to_unicode(unsigned isocode)
{
  if(isocode<=0xa0)
    return isocode;
  else
    return iso8859_3_tophalf[isocode-0xa0];
}

static struct ttf_encoding_alternative iso8859_3[]=
{
  {0,0,iso8859_3_to_unicode},
  {-1,-1,0}
};


static unsigned short iso8859_4_tophalf[]=
{ 0x00A0, 0x0104, 0x0138, 0x0156, 0x00A4, 0x0128, 0x013B, 0x00A7,
  0x00A8, 0x0160, 0x0112, 0x0122, 0x0166, 0x00AD, 0x017D, 0x00AF,
  0x00B0, 0x0105, 0x02DB, 0x0157, 0x00B4, 0x0129, 0x013C, 0x02C7,
  0x00B8, 0x0161, 0x0113, 0x0123, 0x0167, 0x014A, 0x017E, 0x014B,
  0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x012A,
  0x0110, 0x0145, 0x014C, 0x0136, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x0168, 0x016A, 0x00DF,
  0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x012B,
  0x0111, 0x0146, 0x014D, 0x0137, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x0169, 0x016B, 0x02D9,
};

static unsigned 
iso8859_4_to_unicode(unsigned isocode)
{
  if(isocode<=0xa0)
    return isocode;
  else
    return iso8859_4_tophalf[isocode-0xa0];
}

static struct ttf_encoding_alternative iso8859_4[]=
{
  {0,0,iso8859_4_to_unicode},
  {-1,-1,0}
};

static unsigned short iso8859_5_tophalf[]=
{ 0x00A0, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407,
  0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x00AD, 0x040E, 0x040F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
  0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
  0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
  0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
  0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
  0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
  0x2116, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457,
  0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x00A7, 0x045E, 0x045F};

static unsigned 
iso8859_5_to_unicode(unsigned isocode)
{
  if(isocode<=0xa0)
    return isocode;
  else
    return iso8859_5_tophalf[isocode-0xa0];
}

static unsigned short 
iso8859_5_apple_cyrillic[]=
{ 0xCA, 0xDD, 0xAB, 0xAE, 0xB8, 0xC1, 0xA7, 0xBA,
  0xB7, 0xBC, 0xBE, 0xCB, 0xCD, 0x00, 0xD8, 0xDA,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
  0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
  0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
  0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xDF,
  0xDC, 0xDE, 0xAC, 0xAF, 0xB9, 0xCF, 0xB4, 0xBB,
  0xC0, 0xBD, 0xBF, 0xCC, 0xCE, 0xA4, 0xD9, 0xDB };

static unsigned
iso8859_5_to_apple_cyrillic(unsigned isocode)
{
  if(isocode<=0x80)
    return isocode;
  else if(isocode>=0xA0)
    return iso8859_5_apple_cyrillic[isocode-0x80];
  else return 0;
}

static struct ttf_encoding_alternative iso8859_5[]=
{
  {0,0,iso8859_5_to_unicode},
  {1,7,iso8859_5_to_apple_cyrillic},
  {-1,-1,0}
};

/* ISO 8859-6 seems useless for serving fonts (not enough presentation
 * forms).  What do Arabic-speakers use?  Perhaps we should have
 * tables for the Mule specific encodings.  */

static unsigned 
iso8859_6_to_unicode(unsigned isocode)
{
  if(isocode<=0xA0 || isocode==0xA4 || isocode==0xAD)
    return isocode;
  else if(isocode==0xAC || isocode==0xBB || 
          (isocode>=0xBF && isocode<=0xDA) ||
          (isocode>=0xE0 && isocode<=0xEF) ||
          (isocode>=0xF0 && isocode<=0xF2))
    return isocode-0xA0+0x0600;
  else
    return 0;
}

static struct ttf_encoding_alternative iso8859_6[]=
{
  {0,0,iso8859_6_to_unicode},
  {-1,-1,0}
};

static unsigned 
iso8859_7_to_unicode(unsigned isocode)
{
  if(isocode<=0xA0 ||
     (isocode>=0xA3 && isocode<=0xAD) ||
     (isocode>=0xB0 && isocode<=0xB3) ||
     isocode==0xB7 || isocode==0xBB || isocode==0xBD)
    return isocode;
  else if(isocode==0xA1)
    return 0x02BD;
  else if(isocode==0xA2)
    return 0x02BC;
  else if(isocode==0xAF)
    return 0x2015;
  else if(isocode>=0xB4)
    return isocode-0xA0+0x0370;
  else
    return 0;
}

static struct ttf_encoding_alternative iso8859_7[]=
{
  {0,0,iso8859_7_to_unicode},
  {-1,-1,0}
};

static unsigned
iso8859_8_to_unicode(unsigned isocode)
{
  if(isocode==0xA1)
    return 0;
  else if(isocode<0xBF)
    return isocode;
  else if(isocode==0xDF)
    return 0x2017;
  else if(isocode>=0xE0 && isocode<=0xFA)
    return isocode+0x04F0;
  else 
    return 0;
}

static struct ttf_encoding_alternative iso8859_8[]=
{
  {0,0,iso8859_8_to_unicode},
  {-1,-1,0}
};

static unsigned
iso8859_9_to_unicode(unsigned isocode)
{
  switch(isocode) {
  case 0xD0: return 0x011E;
  case 0xDD: return 0x0130;
  case 0xDE: return 0x015E;
  case 0xF0: return 0x011F;
  case 0xFD: return 0x0131;
  case 0xFE: return 0x015F;
  default: return isocode;
  }
}

static struct ttf_encoding_alternative iso8859_9[]=
{
  {0,0,iso8859_9_to_unicode},
  {-1,-1,0}
};

static unsigned short iso8859_10_tophalf[]=
{ 0x00A0, 0x0104, 0x0112, 0x0122, 0x012A, 0x0128, 0x0136, 0x00A7,
  0x013B, 0x0110, 0x0160, 0x0166, 0x017D, 0x00AD, 0x016A, 0x014A,
  0x00B0, 0x0105, 0x0113, 0x0123, 0x012B, 0x0129, 0x0137, 0x00B7,
  0x013C, 0x0111, 0x0161, 0x0167, 0x017E, 0x2014, 0x016B, 0x014B,
  0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x00CF,
  0x00D0, 0x0145, 0x014C, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0168,
  0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
  0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x00EF,
  0x00F0, 0x0146, 0x014D, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0169,
  0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x0138};

static unsigned 
iso8859_10_to_unicode(unsigned isocode)
{
  if(isocode<=0xa0)
    return isocode;
  else
    return iso8859_10_tophalf[isocode-0xa0];
}

static struct ttf_encoding_alternative iso8859_10[]=
{
  {0,0,iso8859_10_to_unicode},
  {-1,-1,0}
};

static unsigned
iso8859_15_to_unicode(unsigned isocode)
{
  switch(isocode) {
  case 0xA4: return 0x20AC;
  case 0xA6: return 0x0160;
  case 0xA8: return 0x0161;
  case 0xB4: return 0x017D;
  case 0xB8: return 0x017E;
  case 0xBC: return 0x0152;
  case 0xBD: return 0x0153;
  case 0xBE: return 0x0178;
  default: return isocode;
  }
}

static struct ttf_encoding_alternative iso8859_15[]=
{
  {0,0,iso8859_15_to_unicode},
  {-1,-1,0}
};

static unsigned short koi8_r_tophalf[]=
{ 0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524,
  0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590,
  0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2022, 0x221A, 0x2248,
  0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0, 0x00B2, 0x00B7, 0x00F7,
  0x2550, 0x2551, 0x2552, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556,
  0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E,
  0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565,
  0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0x00A9,
  0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
  0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
  0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432,
  0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A,
  0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
  0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
  0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412,
  0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042A};

static unsigned 
koi8_r_to_unicode(unsigned koicode)
{
  if(koicode<=0x80)
    return koicode;
  else
    return koi8_r_tophalf[koicode-0x80];
}

static struct ttf_encoding_alternative koi8_r[]=
{
  {0,0,koi8_r_to_unicode},
  {-1,-1,0}
};

/* See RFC 2319 */

static unsigned 
koi8_u_to_unicode(unsigned koicode)
{
  switch(koicode) {
  case 0xA4: return 0x0454;
  case 0xA6: return 0x0456;
  case 0xA7: return 0x0457;
  case 0xAD: return 0x0491;
  case 0xB4: return 0x0403;
  case 0xB6: return 0x0406;
  case 0xB7: return 0x0407;
  case 0xBD: return 0x0490;
  default: return koi8_r_to_unicode(koicode);
  }
}

static struct ttf_encoding_alternative koi8_u[]=
{
  {0,0,koi8_u_to_unicode},
  {-1,-1,0}
};

static unsigned 
koi8_ru_to_unicode(unsigned koicode)
{
  switch(koicode) {
  case 0x93: return 0x201C;
  case 0x96: return 0x201D;
  case 0x97: return 0x2014;
  case 0x98: return 0x2116;
  case 0x99: return 0x2122;
  case 0x9B: return 0x00BB;
  case 0x9C: return 0x00AE;
  case 0x9D: return 0x00AB;
  case 0x9F: return 0x00A4;
  case 0xA4: return 0x0454;
  case 0xA6: return 0x0456;
  case 0xA7: return 0x0457;
  case 0xAD: return 0x0491;
  case 0xAE: return 0x045E;
  case 0xB4: return 0x0404;
  case 0xB6: return 0x0406;
  case 0xB7: return 0x0407;
  case 0xBD: return 0x0490;
  case 0xBE: return 0x040E;
  default: return koi8_r_to_unicode(koicode);
  }
}

static struct ttf_encoding_alternative koi8_ru[]=
{
  {0,0,koi8_ru_to_unicode},
  {-1,-1,0}
};

static unsigned short koi8_uni_tophalf[]=
{ 0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524,
  0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590,
  0x2591, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x00A9, 0x2122, 0x00A0, 0x00BB, 0x00AE, 0x00AB, 0x00B7, 0x00A4,
  0x00A0, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457,
  0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x0491, 0x045E, 0x045F,
  0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407,
  0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x0490, 0x040E, 0x040F,
  0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
  0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
  0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432,
  0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A,
  0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
  0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
  0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412,
  0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042A };

static unsigned 
koi8_uni_to_unicode(unsigned koicode)
{
  if(koicode<=0x80)
    return koicode;
  else
    return koi8_uni_tophalf[koicode-0x80];
}

static struct ttf_encoding_alternative koi8_uni[]=
{
  {0,0,koi8_uni_to_unicode},
  {-1,-1,0}
};

/* Apparently, Microsoft Symbol aims at being compatible with Unicode
 * by using the 16 columns of the Private Use Area starting at code
 * 0xF000. */

static unsigned
eight_bit_to_microsoft_symbol(unsigned code)
{
  return code+0xF000;
}

static struct ttf_encoding_alternative microsoft_symbol[]=
{{3,0,eight_bit_to_microsoft_symbol}, {-1,-1,0}};

static struct ttf_encoding_alternative apple_roman[]=
{{1,0,0}, {-1,-1,0}};

/* The data for recodings */

static struct ttf_encoding_info ttf_encoding_info[]=
{
  {"iso10646-1",256*256,iso10646}, /* Unicode */
  {"iso8859-1",256,iso8859_1},  /* Latin 1 (West European) */
  {"iso8859-2",256,iso8859_2},  /* Latin 2 (East European) */
  {"iso8859-3",256,iso8859_3},  /* Latin 3 (South European) */
  {"iso8859-4",256,iso8859_4},  /* Latin 4 (North European) */
  {"iso8859-5",256,iso8859_5},  /* Cyrillic */
  {"iso8859-6",256,iso8859_6},  /* Arabic */
  {"iso8859-7",256,iso8859_7},  /* Greek */
  {"iso8859-8",256,iso8859_8},  /* Hebrew */
  {"iso8859-9",256,iso8859_9},  /* Latin 5 (Turkish) */
  {"iso8859-10",256,iso8859_10},/* Latin 6 (Nordic) */
  {"iso8859-15",256,iso8859_15},/* Latin 9 */
  {"fcd8859-15",256,iso8859_15},/* for compatibility with X11R6.4 */
  {"koi8-r",256,koi8_r},        /* Russian */
  {"koi8-u",256,koi8_u},        /* Ukrainian */
  {"koi8-ru",256,koi8_ru},      /* Ukrainian too */
  {"koi8-uni",256,koi8_uni},    /* Russian/Ukrainian/Bielorussian */
  {"microsoft-symbol",256,microsoft_symbol},
  {"apple-roman",256,apple_roman},
  {0,0,0}
};

int
ttf_find_cmap_from_charset(char *charset, TT_Face face,
                           struct ttf_encoding *cinfo)
{
  struct ttf_encoding_info *info;
  struct ttf_encoding_alternative *alt;
  TT_CharMap cmap;

  for(info=ttf_encoding_info; info->charset; info++)
    if(!strcasecmp(info->charset, charset)) {
      for(alt=info->alternatives; alt->pid>=0; alt++) {
        if(!ttf_find_cmap(alt->pid, alt->eid, face, &cmap)) {
          cinfo->cmap=cmap;
          cinfo->nchars=info->size;
          cinfo->recode=alt->recode;
          return 0;
        }
      }
    }
  return 1;
}


int ttf_find_cmap_default(TT_Face face, 
                          struct ttf_encoding *cinfo)
{
  struct ttf_encoding_alternative *alt;
  TT_CharMap cmap;

  /* We won't need a recoding map in this case */
  cinfo->recode=0;

  /* Try to find a Unicode charmap */
    if(ttf_find_cmap(0, 0, face, &cmap)) {
      cinfo->cmap=cmap;
      cinfo->nchars=256;
      cinfo->recode=0;
      return 0;
    }

  /* Try to get the first charmap in the file */
  if(!TT_Get_CharMap(face, 0, &cmap)) {
    cinfo->cmap=cmap;
    cinfo->nchars=256*256;
    return 0;
  }

  /* Tough. */
  return 1;
}

/* Find a cmap for a given (pid,eid).  pid=0 means any Unicode table. */

static int 
ttf_find_cmap(int pid, int eid, TT_Face face, TT_CharMap *cmap)
{
  int i, n;
  unsigned short p,e;

  n=TT_Get_CharMap_Count(face);

  if(pid!=0) {                  /* specific cmap */
    for(i=0; i<n; i++) {
      if(!TT_Get_CharMap_ID(face, i, &p, &e) && p==pid && e==eid) {
        if(!TT_Get_CharMap(face, i, cmap))
          return 0;
      }
    }
  } else {                      /* any Unicode cmap */
    /* prefer Microsoft Unicode */
    for(i=0; i<n; i++) {
      if(!TT_Get_CharMap_ID(face, i, &p, &e) && p==3 && e==1) {
        if(!TT_Get_CharMap(face, i, cmap))
          return 0;
        else
          break;
      }
    }
    /* Try Apple Unicode */
    for(i=0; i<n; i++) {
      if(!TT_Get_CharMap_ID(face, i, &p, &e) && p==0) {
        if(!TT_Get_CharMap(face, i, cmap))
          return 0;
        /* but don't give up yet -- there may be more than one cmaps
         * with pid=0 */
      }
    }
    /* ISO Unicode? */
    for(i=0; i<n; i++) {
      if(!TT_Get_CharMap_ID(face, i, &p, &e) && p==2 && e==1) {
        if(!TT_Get_CharMap(face, i, cmap))
          return 0;
        else
          break;
      }
    }
  }
  return 1;
}

/* Pick a suitable CMAP.  Returns the CMAP id, a bound on the number
 * of characters and a recoding function. */

int
ttf_pick_cmap(char *name, int length, TT_Face face, 
              struct ttf_encoding *cinfo)
{
  char *p,*q;
  char charset[256];
  int len;

  if(name==0)
    p=0;
  else {
    p=name+length-1;
    while(p>name && *p!='-')
      p--;
    p--;
    while(p>=name && *p!='-')
      p--;
  if(p<=name)
    p=0;
  }

  /* now p either is null or points at the '-' before the charset registry */

  if(p) {
    len=length-(p-name)-1;
    memcpy(charset,p+1,len);
    charset[len]=0;

    /* check for a subset specification */
    if(q=strchr(charset,(int)'['))
      *q=0;
  }

  if(!p || strcasecmp(charset,"truetype-raw")) {
    /* Search for a suitable cmap */
    if(p && !ttf_find_cmap_from_charset(charset, face, cinfo)) {
      return 0;
    }
    /* search for an unsuitable one */
    else if(!ttf_find_cmap_default(face, cinfo))
      return 0;
  }

  /* Either we failed in finding a cmap, or we were asked for a raw font */
  return 1;
}


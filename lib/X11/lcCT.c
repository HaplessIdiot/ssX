/* $TOG: lcCT.c /main/17 1998/06/18 13:17:06 kaleb $ */
/*
 * Copyright 1992, 1993 by TOSHIBA Corp.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. TOSHIBA make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * TOSHIBA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *			   	mopi@osa.ilab.toshiba.co.jp
 */
/*
 * Copyright 1995 by FUJITSU LIMITED
 * This is source code modified by FUJITSU LIMITED under the Joint
 * Development Agreement for the CDE/Motif PST.
 *
 * Modifier: Takanori Tateno   FUJITSU LIMITED
 *
 */
/*
 *  2000
 *  Modifier: Ivan Pascal   The XFree86 Project
 */
/* $XFree86: xc/lib/X11/lcCT.c,v 3.13 1999/06/06 14:05:53 dawes Exp $ */

#include "Xlibint.h"
#include "XlcPubI.h"
#include <X11/Xos.h>
#include <stdio.h>

typedef struct _StateRec {
    XlcCharSet charset;
    XlcCharSet GL_charset;
    XlcCharSet GR_charset;
    XlcCharSet ext_seg_charset;
    int ext_seg_left;
} StateRec, *State;

typedef struct _CTDataRec {
    char *name;
    char *encoding;		/* Compound Text encoding */
} CTDataRec, *CTData;

typedef struct _CTInfoRec {
    XlcCharSet charset;
    unsigned int type;
    unsigned char final_byte;
    int ext_segment_len;
    char *ext_segment;		/* extended segment */
    struct _CTInfoRec *next;
} CTInfoRec, *CTInfo;

static CTDataRec default_ct_data[] =
{
    { "ISO8859-1:GL", "\033(B" },
    { "ISO8859-1:GR", "\033-A" },
    { "ISO8859-2:GR", "\033-B" },
    { "ISO8859-3:GR", "\033-C" },
    { "ISO8859-4:GR", "\033-D" },
    { "ISO8859-7:GR", "\033-F" },
    { "ISO8859-6:GR", "\033-G" },
    { "ISO8859-8:GR", "\033-H" },
    { "ISO8859-5:GR", "\033-L" },
    { "ISO8859-9:GR", "\033-M" },
    { "ISO8859-10:GR", "\033-V" },
    { "JISX0201.1976-0:GL", "\033(J" },
    { "JISX0201.1976-0:GR", "\033)I" },

    { "GB2312.1980-0:GL", "\033$(A" },
    { "GB2312.1980-0:GR", "\033$)A" },
    { "JISX0208.1983-0:GL", "\033$(B" },
    { "JISX0208.1983-0:GR", "\033$)B" },
    { "KSC5601.1987-0:GL", "\033$(C" },
    { "KSC5601.1987-0:GR", "\033$)C" },
#ifdef notdef
    { "JISX0212.1990-0:GL", "\033$(D" },
    { "JISX0212.1990-0:GR", "\033$)D" },
    { "CNS11643.1986-1:GL", "\033$(G" },
    { "CNS11643.1986-1:GR", "\033$)G" },
    { "CNS11643.1986-2:GL", "\033$(H" },
    { "CNS11643.1986-2:GR", "\033$)H" },
#endif
    { "TIS620.2533-1:GR", "\033-T"},
    { "ISO10646-1", "\033%B"},
    /* Non-Standard Character Set Encodings */
    { "KOI8-R:GR", "\033%/1\200\210koi8-r\002"},
    { "KOI8-U:GR", "\033%/1\200\211koi8-u\002"},
    { "ISO8859-15:GR", "\033%/1\200\213iso8859-15\002"},
    { "ARMSCII-8:GR", "\033%/1\200\210armscii-8\002"},
    { "ISO8859-14:GR", "\033%/1\200\213iso8859-14\002"},
    { "IBM-CP1133:GR", "\033%/1\200\210ibm-cp1133\002"},
    { "MULELAO-1:GR", "\033%/1\200\210mulelao-1\002"},
    { "VISCII1.1-1:GR", "\033%/1\200\210viscii1.1-1\002"},
    { "TCVN-5712:GR", "\033%/1\200\210tcvn-5712\002"},
    { "GEORGIAN-ACADEMY:GR", "\033%/1\200\210georgian-academy\002"},
    { "GEORGIAN-PS:GR", "\033%/1\200\210georgian-ps\002"},
} ; 

#define XctC0		0x0000
#define XctHT		0x0009
#define XctNL		0x000a
#define XctESC		0x001b
#define XctGL		0x0020
#define XctC1		0x0080
#define XctCSI		0x009b
#define XctGR		0x00a0
#define XctSTX		0x0002

#define XctCntrlFunc	0x0023
#define XctMB		0x0024
#define XctOtherCoding	0x0025
#define XctGL94		0x0028
#define XctGR94		0x0029
#define XctGR96		0x002d
#define XctNonStandard	0x002f
#define XctIgnoreExt	0x0030
#define XctNotIgnoreExt	0x0031
#define XctLeftToRight	0x0031
#define XctRightToLeft	0x0032
#define XctDirection	0x005d
#define XctDirectionEnd	0x005d

#define XctGL94MB	0x2428
#define XctGR94MB	0x2429
#define XctExtSeg	0x252f
#define XctOtherSeg	0x2f00

#define XctESCSeq	0x1b00
#define XctCSISeq	0x9b00

CTDataRec *default_ct_data_list()
{
	return(default_ct_data);
}

size_t default_ct_data_list_num()
{
	size_t num = sizeof(default_ct_data) / sizeof(CTDataRec);
	return(num);
}

static CTInfo ct_list = NULL;

static unsigned int
_XlcParseCT(text, length, extra_data)
    char **text;
    int *length;
    unsigned int *extra_data;
{
    unsigned int ret = 0, char_size = 1, dummy, *data = extra_data;
    unsigned char ch;
    register unsigned char *str = (unsigned char *) *text;

    if (data == NULL)
       data = &dummy;
    *data = 0;

    switch (ch = *str++) {
       case XctESC:
          switch (ch = *str++) {
             case XctOtherCoding:             /* % */
                ch = *str++;
                if (ch == XctNonStandard) {
                   ret = XctExtSeg;
                   ch = *str++;
                } else {
                   ret = XctOtherCoding;
                }
                *data = (unsigned int) ch;
                break;

             case XctCntrlFunc:               /* # */
                *data = (unsigned int) *str++;
                switch (*str++){
                   case XctIgnoreExt:
                      ret = XctIgnoreExt;
                      break;
                   case XctNotIgnoreExt:
                      ret = XctNotIgnoreExt;
                      break;
                   default:
                      ret = 0;
                      break;
                }
                break;

             case XctMB:                      /* $ */
                ch = *str++;
	        switch (ch) {
	           case XctGL94:
		      ret = XctGL94MB;
		      break;
	           case XctGR94:
                      ret = XctGR94MB;
	              break;
                   default:
                      ret = 0;
                      break;
                }
                *data = (unsigned int) *str++;
                break;

	     case XctGL94:
		ret = XctGL94;
                *data = (unsigned int) *str++;
		break;
	     case XctGR94:
                ret = XctGR94;
                *data = (unsigned int) *str++;
	        break;
             case XctGR96:
                ret = XctGR96;
                *data = (unsigned int) *str++;
                break;
          }
          break;
       case XctCSI:
	  /* direction */
	  if (*str == XctLeftToRight && *(str + 1) == XctDirection) {
	     ret = XctLeftToRight;
	     str += 2;
	  } else if (*str == XctRightToLeft && *(str + 1) == XctDirection) {
	     ret = XctRightToLeft;
	     str += 2;
	  } else if (*str == XctDirectionEnd) {
	     ret = XctDirectionEnd;
	     str++;
	  } else {
             ret = 0;
	  }
          break;
    }

    if (ret) {
       *length -= (char *) str - *text;
       *text = (char *) str;
    }
    return ret;
}

static CTInfo
_XlcGetCTInfo(text, type, final_byte)
   char *text;
   unsigned int type;
   unsigned char final_byte;
{
   CTInfo ct_info;

   for (ct_info = ct_list; ct_info; ct_info = ct_info->next) {
      if (ct_info->type == type && ct_info->final_byte == final_byte) {
         if (ct_info->ext_segment) {
            if (text &&
                !strncmp(text, ct_info->ext_segment, ct_info->ext_segment_len))
               return ct_info;
         } else {
            return ct_info;
         }
      }
   }
   return (CTInfo) NULL;
}

XlcCharSet
_XlcAddCT(name, encoding)
    _Xconst char *name;
    char *encoding;
{
    CTInfo ct_info;
    XlcCharSet charset;
    char *ct_ptr = encoding;
    int length;
    unsigned int type, final_byte;

    length = strlen(encoding);

    charset = _XlcGetCharSet((char *) name);
    if (charset == NULL) {
        charset = _XlcCreateDefaultCharSet(name, encoding);
        if (charset == NULL)
	    return (XlcCharSet) NULL;
        _XlcAddCharSet(charset);
    }

    ct_info = (CTInfo) Xmalloc(sizeof(CTInfoRec));
    if (ct_info == NULL)
	return (XlcCharSet) NULL;

    ct_info->ext_segment = NULL;
    ct_info->ext_segment_len = 0;
 
    type = _XlcParseCT(&ct_ptr, &length, &final_byte);

    switch (type) {
	case XctExtSeg:
	   if (strlen(charset->ct_sequence) > 6) {
              ct_info->ext_segment = charset->ct_sequence + 6;
              ct_info->ext_segment_len = strlen(ct_info->ext_segment) - 1;
	   } else {
              ct_info->ext_segment = charset->encoding_name;
              ct_info->ext_segment_len = strlen(ct_info->ext_segment);
	   }
	case XctGL94:
	case XctGL94MB:
	case XctGR94:
	case XctGR94MB:
	case XctGR96:
	case XctOtherCoding:
           ct_info->type = type;
           ct_info->final_byte = (unsigned char) final_byte;
           ct_info->charset = charset;
           break;
	default:
           Xfree(ct_info);
	   return (XlcCharSet) NULL;
    }

    if (!_XlcGetCTInfo( ct_info->ext_segment, type, ct_info->final_byte)) {
       ct_info->next = ct_list;
       ct_list = ct_info;
    } else {
       Xfree(ct_info);
    }

    return charset;
}

static CTInfo
_XlcGetCTInfoFromCharSet(charset)
    register XlcCharSet charset;
{
    register CTInfo ct_info;

    for (ct_info = ct_list; ct_info; ct_info = ct_info->next)
	if (ct_info->charset == charset)
	    return ct_info;

    return (CTInfo) NULL;
}

Bool
_XlcParseCharSet(charset)
    XlcCharSet charset;
{
    unsigned int type, final_byte;
    char *ptr = charset->ct_sequence;
    int length;
    int char_size = 1;
    
    if (ptr == NULL || *ptr == '\0')
    	return False;

    length = strlen(ptr);

    type = _XlcParseCT(&ptr, &length, &final_byte);

    if (type == XctGR94MB || type == XctGL94MB) {
       if (final_byte < 0x60) {
          char_size = 2;
       } else if (final_byte < 0x70) {
          char_size = 3;
       } else {
          char_size = 4;
       }
    }

    if (type == XctExtSeg) {
       char_size = final_byte - '0';
       if ((char_size < 1) || (char_size > 4))
          char_size = 1;
    }

    switch (type) {
       case XctGR94MB :
       case XctGR94 :
          charset->side = XlcGR;
          charset->set_size = 94;
          charset->char_size = char_size;
          break;
       case XctGL94MB :
       case XctGL94 :
          charset->side = XlcGL;
          charset->set_size = 94;
          charset->char_size = char_size;
          break;
       case XctGR96:
          charset->side = XlcGR;
          charset->set_size = 96;
          charset->char_size = char_size;
          break;
       case XctOtherCoding:
       case XctExtSeg:
          charset->side = XlcGLGR;
          charset->char_size = char_size;
          break;
    }
    return True;
}

static void init_converter();

Bool
_XlcInitCTInfo()
{
    register XlcCharSet charset;
    register CTData ct_data;
    register int num;

    if (ct_list == NULL) {
	num = sizeof(default_ct_data) / sizeof(CTDataRec);
	for (ct_data = default_ct_data; num-- > 0; ct_data++) {
	    charset = _XlcAddCT(ct_data->name, ct_data->encoding);
	    if (charset == NULL)
		continue;
	}
	init_converter();
    }

    return True;
}

typedef enum { resOK, resNotCTSeq, resNotInList } CheckResult;
/*  resNotCTSeq - EscSeq not recognized, pointers not changed
*   resNotInList - EscSeq recognized but charset not found,
*                  sequence skiped
*   resOK - OK. Charset saved in 'state', sequence skiped
*/  

static CheckResult
_XlcCheckCTSequence(state, ctext, ctext_len)
    State state;
    char **ctext;
    int *ctext_len;
{
    XlcCharSet charset;
    CTInfo ct_info;
    int length;
    unsigned int type, final_byte;
    unsigned int ext_seg_left;

    type = _XlcParseCT(ctext, ctext_len, &final_byte);

    if (!type)
       return resNotCTSeq;

    if ((type == XctExtSeg) && (*ctext_len > 2)) {
    	ext_seg_left = (*(*ctext)++ - 128) << 7 + (*(*ctext)++ - 128) - 2;
        *ctext_len -= 2;
    }

    ct_info = _XlcGetCTInfo(*ctext, type, (unsigned char) final_byte);

    if (ct_info) {
       charset = ct_info->charset;
       if (ct_info->ext_segment_len) {
          *ctext += ct_info->ext_segment_len + 1;
          *ctext_len -= ct_info->ext_segment_len + 1;
       }
       if (charset->side == XlcGL) {
          state->GL_charset = charset;
       } else if (charset->side == XlcGR) {
          state->GR_charset = charset;
       } else {
          state->GL_charset = charset;
          state->GR_charset = charset;
       }
    } else {
       if (type == XctExtSeg) {
          *ctext += ext_seg_left;
          *ctext_len -= ext_seg_left;
       }
       return resNotInList;
    }
    return resOK;
}

static void
init_state(conv)
    XlcConv conv;
{
    State state = (State) conv->state;
    static XlcCharSet GL_charset = NULL;
    static XlcCharSet GR_charset = NULL;

    if (GL_charset == NULL) {
	GL_charset = _XlcGetCharSet("ISO8859-1:GL");
	GR_charset = _XlcGetCharSet("ISO8859-1:GR");
    }

    state->GL_charset = state->charset = GL_charset;
    state->GR_charset = GR_charset;
    state->ext_seg_charset = NULL;
    state->ext_seg_left = 0;
}

static int
cttocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    register State state = (State) conv->state;
    register unsigned char ch;
    int ret;
    XlcCharSet charset = NULL;
    char *ctptr, *bufptr;
    int ctext_len, buf_len;

    ctptr = (unsigned char *) *from;
    bufptr = (unsigned char *) *to;
    ctext_len = *from_left;
    buf_len = *to_left;

    while (ctext_len > 0 && buf_len > 0) {
       ch = *ctptr;
       if (ch == XctCSI) {
          /* do nothing except skip sequence if recognized*/
          if(_XlcParseCT(&ctptr, &ctext_len, NULL))
             continue;
       }
       if (ch == XctESC) {
          ret = _XlcCheckCTSequence(state, &ctptr, &ctext_len);
          if (ret == resOK || ret == resNotInList)
             continue;
       }
       if (charset) {
          if (charset != (ch & 0x80 ? state->GR_charset : state->GL_charset))
             break; 
       } else {
          charset = ch & 0x80 ? state->GR_charset : state->GL_charset;
       }

       *bufptr++ = *ctptr++;
       ctext_len--;
       buf_len--;
    }

    if (charset)
	state->charset = charset;
    if (num_args > 0)
	*((XlcCharSet *) args[0]) = state->charset;

    *from_left -= ctptr - *((char **) from);
    *from = (XPointer) ctptr;

    *to_left -= bufptr - *((char **) to);
    *to = (XPointer) bufptr;

    return 0;
}

static int
cstoct(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    XlcSide side;
    unsigned char min_ch, max_ch;
    register unsigned char ch;
    int length;
    CTInfo ct_info;
    XlcCharSet charset;
    char *csptr, *ctptr;
    int csstr_len, ct_len;

    if (num_args < 1)
	return -1;
    
    csptr = *((char **) from);
    ctptr = *((char **) to);
    csstr_len = *from_left;
    ct_len = *to_left;
    
    charset = (XlcCharSet) args[0];

    ct_info = _XlcGetCTInfoFromCharSet(charset);
    if (ct_info == NULL)
	return -1;

    side = charset->side;
    length = strlen(charset->ct_sequence);

    if (((side == XlcGR || side == XlcGLGR) &&
          charset != state->GR_charset) ||
        ((side == XlcGL || side == XlcGLGR) &&
          charset != state->GL_charset) ) {

        /* output esc-sequence */
        if ((ct_info->type == XctExtSeg) && (length < 7)) {
           int comp_len = length + strlen(ct_info->ext_segment) + 3;

           if (ct_len < comp_len)
              return -1;

	   strcpy(ctptr, ct_info->charset->ct_sequence);
	   ctptr += length;

	   length = ct_info->ext_segment_len;
	   *ctptr++ = ((length + 3) / 128) | 0x80;
	   *ctptr++ = ((length + 3) % 128) | 0x80;	   
   	   strncpy(ctptr, ct_info->ext_segment, length);
	   ctptr += length;
	   *ctptr++ = XctSTX;
           ct_len -= comp_len;

        } else {
           if (ct_len < length)
              return -1;

	   strcpy(ctptr, ct_info->charset->ct_sequence);
	   ctptr += length;
           ct_len -= length;
        }
    }
    min_ch = 0x20;
    max_ch = 0x7f;

    if (charset->set_size == 94) {
       max_ch--;
    if (charset->char_size > 1 || side == XlcGR)
       min_ch++;
    }

    while (csstr_len > 0 && ct_len > 0) {
       ch = *((unsigned char *) csptr) & 0x7f;
       if (ch < min_ch || ch > max_ch)
          if (ch != 0x00 && ch != 0x09 && ch != 0x0a && ch != 0x1b) {
             csptr++;
             csstr_len--;
             continue;	/* XXX */
          }

       if (side == XlcGL)
          *ctptr++ = *csptr++ & 0x7f;
       else if (side == XlcGR)
          *ctptr++ = *csptr++ | 0x80;
       else
          *ctptr++ = *csptr++;
       csstr_len--;
       ct_len--;
    }

    if (side == XlcGR || side == XlcGLGR)
       state->GR_charset = charset;
    if (side == XlcGL || side == XlcGLGR)
       state->GL_charset = charset;

    *from_left -= csptr - *((char **) from);
    *from = (XPointer) csptr;

    *to_left -= ctptr - *((char **) to);
    *to = (XPointer) ctptr;

    return 0;
}

static int
strtocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    register char *src, *dst;
    unsigned char side;
    register int length;

    src = (char *) *from;
    dst = (char *) *to;

    length = min(*from_left, *to_left);
    side = *((unsigned char *) src) & 0x80;

    while (side == (*((unsigned char *) src) & 0x80) && length-- > 0)
	*dst++ = *src++;
    
    *from_left -= src - (char *) *from;
    *from = (XPointer) src;
    *to_left -= dst - (char *) *to;
    *to = (XPointer) dst;

    if (num_args > 0)
	*((XlcCharSet *)args[0]) = side ? state->GR_charset : state->GL_charset;

    return 0;
}

static int
cstostr(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    State state = (State) conv->state;
    char *csptr, *string_ptr;
    int csstr_len, str_len;
    unsigned char ch;
    int unconv_num = 0;

    if (num_args < 1 || (state->GL_charset != (XlcCharSet) args[0] &&
	state->GR_charset != (XlcCharSet) args[0]))
	return -1;
    
    csptr = *((char **) from);
    string_ptr = *((char **) to);
    csstr_len = *from_left;
    str_len = *to_left;

    while (csstr_len-- > 0 && str_len > 0) {
	ch = *((unsigned char *) csptr++);
	if ((ch < 0x20 && ch != 0x00 && ch != 0x09 && ch != 0x0a) ||
	    ch == 0x7f || ((ch & 0x80) && ch < 0xa0)) {
	    unconv_num++;
	    continue;
	}
	*((unsigned char *) string_ptr++) = ch;
	str_len--;
    }

    *from_left -= csptr - *((char **) from);
    *from = (XPointer) csptr;

    *to_left -= string_ptr - *((char **) to);
    *to = (XPointer) string_ptr;

    return unconv_num;
}


static void
close_converter(conv)
    XlcConv conv;
{
    if (conv->state)
	Xfree((char *) conv->state);

    Xfree((char *) conv);
}

static XlcConv
create_conv(methods)
    XlcConvMethods methods;
{
    register XlcConv conv;

    conv = (XlcConv) Xmalloc(sizeof(XlcConvRec));
    if (conv == NULL)
	return (XlcConv) NULL;

    conv->state = (XPointer) Xmalloc(sizeof(StateRec));
    if (conv->state == NULL)
	goto err;
    
    conv->methods = methods;

    init_state(conv);

    return conv;

err:
    close_converter(conv);

    return (XlcConv) NULL;
}

static XlcConvMethodsRec cttocs_methods = {
    close_converter,
    cttocs,
    init_state
} ;

static XlcConv
open_cttocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&cttocs_methods);
}

static XlcConvMethodsRec cstoct_methods = {
    close_converter,
    cstoct,
    init_state
} ;

static XlcConv
open_cstoct(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&cstoct_methods);
}

static XlcConvMethodsRec strtocs_methods = {
    close_converter,
    strtocs,
    init_state
} ;

static XlcConv
open_strtocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&strtocs_methods);
}

static XlcConvMethodsRec cstostr_methods = {
    close_converter,
    cstostr,
    init_state
} ;

static XlcConv
open_cstostr(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(&cstostr_methods);
}

static void
init_converter()
{
    _XlcSetConverter((XLCd) NULL, XlcNCompoundText, (XLCd) NULL, XlcNCharSet,
		     open_cttocs);
    _XlcSetConverter((XLCd) NULL, XlcNString, (XLCd) NULL, XlcNCharSet,
		     open_strtocs);

    _XlcSetConverter((XLCd) NULL, XlcNCharSet, (XLCd) NULL, XlcNCompoundText,
		     open_cstoct);
    _XlcSetConverter((XLCd) NULL, XlcNCharSet, (XLCd) NULL, XlcNString,
		     open_cstostr);
}

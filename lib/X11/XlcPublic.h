/* $TOG: XlcPublic.h /main/6 1997/06/22 07:38:55 kaleb $ */
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
/* $XFree86: xc/lib/X11/XlcPublic.h,v 1.6 2000/11/28 17:25:08 dawes Exp $ */
/*
 * Most of this API is documented in i18n/Framework.PS
 */

#ifndef _XLCPUBLIC_H_
#define _XLCPUBLIC_H_

#include "Xlcint.h"

#define XlcNCharSize 		"charSize"
#define XlcNCodeset 		"codeset"
#define XlcNControlSequence 	"controlSequence"
#define XlcNDefaultString 	"defaultString"
#define XlcNEncodingName 	"encodingName"
#define XlcNLanguage 		"language"
#define XlcNMbCurMax 		"mbCurMax"
#define XlcNName 		"name"
#define XlcNSetSize 		"setSize"
#define XlcNSide 		"side"
#define XlcNStateDependentEncoding "stateDependentEncoding"
#define XlcNTerritory 		"territory"

typedef enum {
    XlcUnknown, XlcC0, XlcGL, XlcC1, XlcGR, XlcGLGR, XlcOther, XlcNONE
} XlcSide;

typedef struct _FontScope {
        unsigned long   start;
        unsigned long   end;
        unsigned long   shift;
        unsigned long   shift_direction;
} FontScopeRec, *FontScope;

typedef struct _UDCArea {
        unsigned long 	start,end;
} UDCAreaRec, *UDCArea;

typedef struct _XlcCharSetRec *XlcCharSet;

typedef char* (*XlcGetCSValuesProc)(
#if NeedFunctionPrototypes
    XlcCharSet		/* charset */,
    XlcArgList		/* args */,
    int			/* num_args */
#endif
);

typedef enum {CSsrcUndef = 0, CSsrcStd, CSsrcXLC} CSSrc;

typedef struct _XlcCharSetRec {
    char 		*name;		/* character set name */
    XrmQuark 		xrm_name;
    char 		*encoding_name;	/* XLFD encoding name */
    XrmQuark 		xrm_encoding_name;
    XlcSide 		side;		/* GL, GR or others */
    int 		char_size;	/* number of bytes per character */
    int 		set_size;	/* graphic character sets */
    char 		*ct_sequence;	/* control sequence of CT */
    XlcGetCSValuesProc 	get_values;
    /* UDC */
    Bool        	string_encoding;
    UDCArea 		udc_area;
    int     		udc_area_num;
    /* CS description source */
    CSSrc		source;
} XlcCharSetRec;

/*
 * conversion methods
 */

typedef struct _XlcConvRec *XlcConv;

typedef XlcConv (*XlcOpenConverterProc)(
    XLCd		from_lcd,
    const char*		from_type,
    XLCd		to_lcd,
    const char*		to_type
);

typedef void (*XlcCloseConverterProc)(
#if NeedFunctionPrototypes
    XlcConv		/* conv */
#endif
);

typedef int (*XlcConvertProc)(
#if NeedFunctionPrototypes
    XlcConv		/* conv */,
    XPointer*		/* from */,
    int*		/* from_left */,
    XPointer*		/* to */,
    int*		/* to_left */,
    XPointer*		/* args */,
    int			/* num_args */
#endif
);

typedef void (*XlcResetConverterProc)(
#if NeedFunctionPrototypes
    XlcConv		/* conv */
#endif
);

typedef struct _XlcConvMethodsRec{
    XlcCloseConverterProc 	close;
    XlcConvertProc 		convert;
    XlcResetConverterProc 	reset;
} XlcConvMethodsRec, *XlcConvMethods;

/*
 * conversion data
 */

#define XlcNMultiByte 		"multiByte"
#define XlcNWideChar 		"wideChar"
#define XlcNCompoundText 	"compoundText"
#define XlcNString 		"string"
#define XlcNUtf8String 		"utf8String"
#define XlcNCharSet 		"charSet"
#define XlcNCTCharSet 		"CTcharSet"
#define XlcNFontCharSet		"FontCharSet"
#define XlcNChar 		"char"
#define XlcNUcsChar 		"UCSchar"

typedef struct _XlcConvRec {
    XlcConvMethods 		methods;
    XPointer 			state;
} XlcConvRec;


_XFUNCPROTOBEGIN

extern Bool _XInitOM(
#if NeedFunctionPrototypes
    XLCd		/* lcd */
#endif
);

extern Bool _XInitIM(
#if NeedFunctionPrototypes
    XLCd		/* lcd */
#endif
);

extern char *_XGetLCValues(
#if NeedVarargsPrototypes
    XLCd		/* lcd */,
    ...
#endif
);

extern XlcCharSet _XlcGetCharSet(
    const char*		name
);

extern XlcCharSet _XlcGetCharSetWithSide(
    const char*		encoding_name,
    XlcSide		side
);

extern Bool _XlcAddCharSet(
    XlcCharSet		charset
);

extern char *_XlcGetCSValues(
#if NeedVarargsPrototypes
    XlcCharSet		/* charset */,
    ...
#endif
);

extern XlcConv _XlcOpenConverter(
    XLCd		from_lcd,
    const char*		from_type,
    XLCd		to_lcd,
    const char*		to_type
);

extern void _XlcCloseConverter(
    XlcConv		conv
);

extern int _XlcConvert(
    XlcConv		conv,
    XPointer*		from,
    int*		from_left,
    XPointer*		to,
    int*		to_left,
    XPointer*		args,
    int			num_args
);

extern void _XlcResetConverter(
    XlcConv		conv
);

extern Bool _XlcSetConverter(
    XLCd			from_lcd,
    const char*			from_type,
    XLCd			to_lcd,
    const char*			to_type,
    XlcOpenConverterProc	open_converter
);

extern void _XlcGetResource(
    XLCd		lcd,
    const char*		category,
    const char*		_class,
    char***		value,
    int*		count
);

extern char *_XlcFileName(
    XLCd		lcd,
    const char*		category
);

extern int _Xwcslen(
#if NeedFunctionPrototypes
    wchar_t*		/* wstr */
#endif
);

extern wchar_t *_Xwcscpy(
#if NeedFunctionPrototypes
    wchar_t*		/* wstr1 */,
    wchar_t*		/* wstr2 */
#endif
);

/* Compares two ISO 8859-1 strings, ignoring case of ASCII letters.
   Like strcasecmp in an ASCII locale. */
extern int _XlcCompareISOLatin1(
    const char*		str1,
    const char*		str2
);

/* Compares two ISO 8859-1 strings, at most len bytes of each, ignoring
   case of ASCII letters. Like strncasecmp in an ASCII locale. */
extern int _XlcNCompareISOLatin1(
    const char*		str1,
    const char*		str2,
    int			len
);

_XFUNCPROTOEND

#endif  /* _XLCPUBLIC_H_ */

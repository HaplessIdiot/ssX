/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo César Pereira de Andrade
 */

/* $XFree86: xc/programs/xedit/lisp/io.c,v 1.2 2002/01/31 04:33:27 paulo Exp $ */

#include "write.h"
#include <math.h>

/*
 * Prototypes
 */
static void check_stream(LispMac*, LispObj*, LispFile**, LispString**, int);
static void parse_double(char*, int*, double, int);
static void format_integer(char*, long, int);
static int LispWriteCPointer(LispMac*, LispObj*, void*);
static int LispWriteCString(LispMac*, LispObj*, char*);
static int LispDoWriteList(LispMac*, LispObj*, LispObj*, int);
static int LispDoFormatExponentialFloat(LispMac*, LispObj*, LispObj*,
					int, int, int*, int, int,
					int, int, int, int);


/*
 * Initialization
 */
extern char *LispCharNames[];

/*
 * Implementation
 */
static void
check_stream(LispMac *mac, LispObj *stream,
	     LispFile **file, LispString **string, int check_writable)
{
    /* NIL is UNIX stdout, *STANDARD-OUTPUT* may not be UNIX stdout */
    if (stream == NIL) {
	*file = Stdout;
	*string = NULL;
    }
    else if (!STREAM_P(stream))
	LispDestroy(mac, "%s is not a stream", STROBJ(stream));
    else if (check_writable && !stream->data.stream.writable)
	LispDestroy(mac, "%s is not writable", STROBJ(stream));
    else if (stream->data.stream.type == LispStreamString) {
	*string = SSTREAMP(stream);
	*file = NULL;
    }
    else {
	if (stream->data.stream.type == LispStreamPipe)
	    *file = OPSTREAMP(stream);
	else
	    *file = stream->data.stream.source.file;
	*string = NULL;
    }
}

/* Assumes buffer has enough storage, 64 bytes should be more than enough */
static void
parse_double(char *buffer, int *exponent, double value, int d)
{
    char stk[64], fmt[32], *ptr, *fract = NULL;
    int positive = value >= 0.0;

parse_double_again:
    if (d >= 8) {
	double dcheck;
	int icheck, count;

	/* this should to do the correct rounding */
	for (count = 2;  count >= 0; count--) {
	    icheck = d <= 0 ? 0 : d > 16 ? 16 - count : d - count;
	    sprintf(fmt, "%%.%de", icheck);
	    sprintf(stk, fmt, value);
	    if (count) {
		/* if the value read back is the same formatted */
		sscanf(stk, "%lf", &dcheck);
		if (dcheck == value)
		    break;
	    }
	}
    }
    else {
	sprintf(fmt, "%%.%de", d <= 0 ? 0 : d > 16 ? 16 : d);
	sprintf(stk, fmt, value);
    }

    /* this "should" never fail */
    ptr = strchr(stk, 'e');
    if (ptr) {
	*ptr++ = '\0';
	*exponent = atoi(ptr);
    }
    else
	*exponent = 0;

    /* find start of number representation */
    for (ptr = stk; *ptr && !isdigit(*ptr); ptr++)
	;

    /* check if did not trim any significant digit,
     * this may happen because '%.e' puts only one digit before the '.' */
    if (d > 0 && d < 16 && fabs(value) >= 10.0 &&
	strlen(ptr) - 1 - !positive <= *exponent) {
	d += *exponent - (strlen(ptr) - 1 - !positive) + 1;
	goto parse_double_again;
    }

    /* this "should" never fail */
    fract = strchr(ptr, '.');
    if (fract)
	*fract++ = '\0';

    /* store number representation in buffer */
    *buffer = positive ? '+' : '-';
    strcpy(buffer + 1, ptr);
    if (fract)
	strcpy(buffer + strlen(buffer), fract);
}

static void
format_integer(char *buffer, long value, int radix)
{
    if (radix == 10)
	sprintf(buffer, "%d", value);
    else if (radix == 16)
	sprintf(buffer, "%x", value);
    else if (radix == 8)
	sprintf(buffer, "%o", value);
    else {
	/* use bignum routine to convert number to string */
	mpi integer;

	mpi_init(&integer);
	mpi_seti(&integer, value);
	mpi_getstr(buffer, &integer, radix);
	mpi_clear(&integer);
    }
}

static int
LispWriteCPointer(LispMac *mac, LispObj *stream, void *data)
{
    char stk[32];

    sprintf(stk, "0x%08x", (long)data);
    return (LispWriteStr(mac, stream, stk));
}

static int
LispWriteCString(LispMac *mac, LispObj *stream, char *string)
{
    int length, ch;
    char *prt, *ptr, *pquote, *pslash;

    if (!LispGetEscape(mac, stream)) {
	length = LispWriteChar(mac, stream, '"');
	for (prt = string, pquote = strchr(prt, '"'),
	     pslash = strchr(prt, '\\');  pquote || pslash;
	     prt = ptr, pquote = pquote ? strchr(prt, '"') : NULL,
			pslash = pslash ? strchr(prt, '\\') : NULL) {
	    if (pquote && pslash)
		ptr = pquote < pslash ? pquote : pslash;
	    else
		ptr = pquote ? pquote : pslash;
	    ch = ptr == pquote ? '"' : '\\';
	    *ptr = '\0';
	    length += LispWriteStr(mac, stream, prt);
	    length += LispWriteChar(mac, stream, '\\');
	    length += LispWriteChar(mac, stream, ch);
	    *ptr = ch;
	    ++ptr;
	}
	length += LispWriteStr(mac, stream, prt);
	length += LispWriteChar(mac, stream, '"');
    }
    else
	length = LispWriteStr(mac, stream, string);

    return (length);
}

static int
LispDoWriteList(LispMac *mac, LispObj *stream, LispObj *object, int paren)
{
    int length = 0;
    LispObj *car, *cdr;

    car = CAR(object);
    cdr = CDR(object);
    if (cdr == NIL) {
	if (paren)
	    length += LispWriteChar(mac, stream, '(');
	length += LispDoWriteObject(mac, stream, car, CONS_P(car));
	if (paren)
	    length += LispWriteChar(mac, stream, ')');
    }
    else {
	if (paren)
	    length += LispWriteChar(mac, stream, '(');
	length += LispDoWriteObject(mac, stream, car, car->type == LispCons_t);
	if (!CONS_P(cdr)) {
	    length += LispWriteStr(mac, stream, " . ");
	    length += LispDoWriteObject(mac, stream, cdr, 0);
	}
	else {
	    length += LispWriteChar(mac, stream, ' ');
	    length += LispDoWriteList(mac, stream, cdr,
				      !CONS_P(car) && !CONS_P(cdr));
	}
	if (paren)
	    length += LispWriteChar(mac, stream, ')');
    }

    return (length);
}

int
LispDoWriteObject(LispMac *mac, LispObj *stream, LispObj *object, int paren)
{
    int length;
    char stk[64];

    switch (object->type) {
	case LispNil_t:
	    length = LispWriteStr(mac, stream, "NIL");
	    break;
	case LispTrue_t:
	    length = LispWriteStr(mac, stream, "T");
	    break;
	case LispOpaque_t:
	    length = LispWriteChar(mac, stream, '#');
	    length += LispWriteCPointer(mac, stream, object->data.opaque.data);
	    length += LispWriteStr(mac, stream,
			        LispIntToOpaqueType(mac, object->data.opaque.type));
	    break;
	case LispAtom_t:
	    length = LispWriteStr(mac, stream, STRPTR(object));
	    break;
	case LispString_t:
	    length = LispWriteString(mac, stream, object);
	    break;
	case LispCharacter_t:
	    LispWriteCharacter(mac, stream, object);
	    break;
	case LispReal_t:
	    LispWriteFloat(mac, stream, object);
	    break;
	case LispInteger_t:
	case LispBigInteger_t:
	    LispWriteInteger(mac, stream, object);
	    break;
	case LispRatio_t:
	    format_integer(stk, object->data.ratio.numerator, 10);
	    length = LispWriteStr(mac, stream, stk);
	    length += LispWriteChar(mac, stream, '/');
	    format_integer(stk, object->data.ratio.denominator, 10);
	    length += LispWriteStr(mac, stream, stk);
	    break;
	case LispBigRatio_t: {
	    int sz;
	    char *ptr;

	    sz = mpi_getsize(mpr_num(object->data.mp.ratio), 10) + 1 +
		 mpi_getsize(mpr_den(object->data.mp.ratio), 10) + 1;
	    if (sz + 2 > sizeof(stk))
		ptr = LispMalloc(mac, sz + 2);
	    else
		ptr = stk;
	    mpr_getstr(ptr, object->data.mp.ratio, 10);
	    length = LispWriteStr(mac, stream, ptr);
	    if (ptr != stk)
		LispFree(mac, ptr);
	}   break;
	case LispComplex_t:
	    length = LispWriteStr(mac, stream, "#C(");
	    length += LispDoWriteObject(mac, stream,
					object->data.complex.real, 0);
	    length += LispWriteStr(mac, stream, " ");
	    length += LispDoWriteObject(mac, stream,
					object->data.complex.imag, 0);
	    length += LispWriteStr(mac, stream, ")");
	    break;
	case LispCons_t:
	    length = LispDoWriteList(mac, stream, object, paren);
	    break;
	case LispQuote_t:
	    length = LispWriteChar(mac, stream, '\'');
	    length += LispDoWriteObject(mac, stream, object->data.quote, 1);
	    break;
	case LispKeyword_t:
	    length = LispWriteChar(mac, stream, ':');
	    length += LispWriteStr(mac, stream, STRPTR(object->data.quote));
	    break;
	case LispBackquote_t:
	    length = LispWriteChar(mac, stream, '`');
	    length += LispDoWriteObject(mac, stream, object->data.quote, 1);
	    break;
	case LispComma_t:
	    if (object->data.comma.atlist)
		length = LispWriteStr(mac, stream, ",@");
	    else
		length = LispWriteChar(mac, stream, ',');
	    length += LispDoWriteObject(mac, stream, object->data.comma.eval, 1);
	    break;
	case LispArray_t:
	    length = LispWriteArray(mac, stream, object);
	    break;
	case LispStruct_t:
	    length = LispWriteStruct(mac, stream, object);
	    break;
	case LispLambda_t:
	    switch (object->data.lambda.type) {
		case LispLambda:
		    length = LispWriteStr(mac, stream, "#<LAMBDA ");
		    break;
		case LispFunction:
		    length = LispWriteStr(mac, stream, "#<FUNCTION ");
		    break;
		case LispMacro:
		    length = LispWriteStr(mac, stream, "#<MACRO ");
		    break;
		case LispSetf:
		    length = LispWriteStr(mac, stream, "#<SETF ");
		    break;
	    }
	    if (object->data.lambda.type != LispLambda) {
		length += LispWriteStr(mac, stream,
				       STRPTR(object->data.lambda.name));
		length += LispWriteChar(mac, stream, ' ');
	    }
	    length += LispDoWriteObject(mac, stream, object->data.lambda.code, 1);
	    length += LispWriteChar(mac, stream, '>');
	    break;
	case LispStream_t:
	    length = LispWriteStr(mac, stream, "#<");
	    if (object->data.stream.type == LispStreamFile)
		length += LispWriteStr(mac, stream, "FILE-STREAM ");
	    else if (object->data.stream.type == LispStreamString)
		length += LispWriteStr(mac, stream, "STRING-STREAM ");
	    else if (object->data.stream.type == LispStreamStandard)
		length += LispWriteStr(mac, stream, "STANDARD-STREAM ");
	    else if (object->data.stream.type == LispStreamPipe)
		length += LispWriteStr(mac, stream, "PIPE-STREAM ");

	    if (!object->data.stream.readable && !object->data.stream.writable)
		length += LispWriteStr(mac, stream, "CLOSED");
	    else {
		if (object->data.stream.readable)
		    length += LispWriteStr(mac, stream, "READ");
		if (object->data.stream.writable) {
		    if (object->data.stream.readable)
			length += LispWriteChar(mac, stream, '-');
		    length += LispWriteStr(mac, stream, "WRITE");
		}
	    }
	    length += LispWriteChar(mac, stream, ' ');
	    if (object->data.stream.type == LispStreamString)
		length += LispWriteCString(mac, stream,
					   LispGetSstring(SSTREAMP(object)));
	    else {
		length += LispDoWriteObject(mac, stream,
					    object->data.stream.pathname, 1);
		/* same address/size for pipes */
		length += LispWriteChar(mac, stream, ' ');
		length += LispWriteCPointer(mac, stream,
					    object->data.stream.source.file);
	    }
	    length += LispWriteChar(mac, stream, '>');
	    break;
	case LispPathname_t:
	    length = LispWriteStr(mac, stream, "#P");
	    length += LispDoWriteObject(mac, stream, CAR(object->data.quote), 1);
	    break;
	default:
	    length = 0;
	    break;
    }

    return (length);
}

/* return current column number in stream */
int
LispGetColumn(LispMac *mac, LispObj *stream)
{
    LispFile *file;
    LispString *string;

    check_stream(mac, stream, &file, &string, 0);
    if (file != NULL)
	return (file->column);
    return (string->column);
}

int
LispGetEscape(LispMac *mac, LispObj *stream)
{
    LispFile *file;
    LispString *string;

    check_stream(mac, stream, &file, &string, 0);
    if (file != NULL)
	return (file->escape);
    return (string->escape);
}

void
LispSetEscape(LispMac *mac, LispObj *stream, int escape)
{
    LispFile *file;
    LispString *string;

    check_stream(mac, stream, &file, &string, 0);
    if (file != NULL)
	file->escape = escape;
    else
	string->escape = escape;
}

/* write a character to stream */
int
LispWriteChar(LispMac *mac, LispObj *stream, int character)
{
    LispFile *file;
    LispString *string;

    check_stream(mac, stream, &file, &string, 1);
    if (file != NULL)
	LispFputc(file, character);
    else
	LispSputc(string, character);
}

/* write a character count times to stream */
int
LispWriteChars(LispMac *mac, LispObj *stream, int character, int count)
{
    if (count > 0) {
	char stk[64];
	LispFile *file;
	LispString *string;

	check_stream(mac, stream, &file, &string, 1);
	if (count >= sizeof(stk)) {
	    memset(stk, character, sizeof(stk));
	    for (; count >= sizeof(stk); count -= sizeof(stk)) {
		if (file != NULL)
		    LispFwrite(file, stk, sizeof(stk));
		else
		    LispSwrite(string, stk, sizeof(stk));
	    }
	}
	else
	    memset(stk, character, count);

	if (count) {
	    if (file != NULL)
		LispFwrite(file, stk, count);
	    else
		LispSwrite(string, stk, count);
	}
    }

    return (count);
}

/* write a string to stream */
int
LispWriteStr(LispMac *mac, LispObj *stream, char *buffer)
{
    LispFile *file;
    LispString *string;

    check_stream(mac, stream, &file, &string, 1);
    if (file != NULL)
	return (LispFputs(file, buffer));
    return (LispSputs(string, buffer));
}

int
LispWriteInteger(LispMac *mac, LispObj *stream, LispObj *object)
{
    return (LispFormatInteger(mac, stream, object, 10, 0, 0, 0, 0, 0, 0));
}

int
LispWriteCharacter(LispMac *mac, LispObj *stream, LispObj *object)
{
    return (LispFormatCharacter(mac, stream, object, 1,
				LispGetEscape(mac, stream)));
}

int
LispWriteString(LispMac *mac, LispObj *stream, LispObj *object)
{
    return (LispWriteCString(mac, stream, STRPTR(object)));
}

int
LispWriteFloat(LispMac *mac, LispObj *stream, LispObj *object)
{
    double value = object->data.real;

    if (fabs(value) < 1.0E7 && fabs(value) > 1.0E-4)
	return (LispFormatFixedFloat(mac, stream, object, 0, 0, NULL, 0, 0, 0));

    return (LispDoFormatExponentialFloat(mac, stream, object, 0, 0, NULL,
					 0, 1, 0, ' ', 'E', 0));
}

int
LispWriteList(LispMac *mac, LispObj *stream, LispObj *object)
{
    return (LispDoWriteList(mac, stream, object, 1));
}

int
LispWriteArray(LispMac *mac, LispObj *stream, LispObj *object)
{
    int length;

    if (object->data.array.rank == 0) {
	length = LispWriteStr(mac, stream, "#0A");
	length += LispDoWriteObject(mac, stream, object->data.array.list, 1);
	return (length);
    }

    if (object->data.array.rank == 1)
	length = LispWriteStr(mac, stream, "#(");
    else {
	char stk[32];

	format_integer(stk, object->data.array.rank, 10);
	length = LispWriteChar(mac, stream, '#');
	length += LispWriteStr(mac, stream, stk);
	length += LispWriteStr(mac, stream, "A(");
    }

    if (!object->data.array.zero) {
	if (object->data.array.rank == 1) {
	    LispObj *ary;
	    long count;

	    for (ary = object->data.array.dim, count = 1;
		 ary != NIL; ary = CDR(ary))
		count *= CAR(ary)->data.integer;
	    for (ary = object->data.array.list; count > 0;
		 ary = CDR(ary), count--) {
		length += LispDoWriteObject(mac, stream, CAR(ary), 0);
		if (count - 1 > 0)
		    length += LispWriteChar(mac, stream, ' ');
	    }
	}
	else {
	    LispObj *ary;
	    int i, k, rank, *dims, *loop;

	    rank = object->data.array.rank;
	    dims = LispMalloc(mac, sizeof(int) * rank);
	    loop = LispCalloc(mac, 1, sizeof(int) * (rank - 1));

	    /* fill dim */
	    for (i = 0, ary = object->data.array.dim; ary != NIL;
		 i++, ary = CDR(ary))
		dims[i] = CAR(ary)->data.integer;

	    i = 0;
	    ary = object->data.array.list;
	    while (loop[0] < dims[0]) {
		for (; i < rank - 1; i++)
		    length += LispWriteChar(mac, stream, '(');
		--i;
		for (;;) {
		    ++loop[i];
		    if (i && loop[i] >= dims[i])
			loop[i] = 0;
		    else
			break;
		    --i;
		}
		for (k = 0; k < dims[rank - 1] - 1; k++, ary = CDR(ary)) {
		    length += LispDoWriteObject(mac, stream, CAR(ary), 1);
		    length += LispWriteChar(mac, stream, ' ');
		}
		length += LispDoWriteObject(mac, stream, CAR(ary), 0);
		ary = CDR(ary);
		for (k = rank - 1; k > i; k--)
		    length += LispWriteChar(mac, stream, ')');
		if (loop[0] < dims[0])
		    length += LispWriteChar(mac, stream,  ' ');
	    }
	    LispFree(mac, dims);
	    LispFree(mac, loop);
	}
    }
    length += LispWriteChar(mac, stream, ')');
}

int
LispWriteStruct(LispMac *mac, LispObj *stream, LispObj *object)
{
    int length;
    LispObj *def = object->data.struc.def;
    LispObj *field = object->data.struc.fields;

    length = LispWriteStr(mac, stream, "S#");
    length += LispWriteStr(mac, stream, STRPTR(CAR(def)));
    def = CDR(def);
    for (; def != NIL; def = CDR(def), field = CDR(field)) {
	length += LispWriteStr(mac, stream, " :");
	length += LispWriteStr(mac, stream,
			       SYMBOL_P(CAR(def)) ?
					STRPTR(CAR(def)) :
					STRPTR(CAR(CAR(def))));
	length += LispDoWriteObject(mac, stream, CAR(field), 1);
    }
    length += LispWriteChar(mac, stream, ')');

    return (length);
}

int
LispWriteObject(LispMac *mac, LispObj *stream, LispObj *object)
{
    return (LispDoWriteObject(mac, stream, object, 1));
}

int
LispFormatInteger(LispMac *mac, LispObj *stream, LispObj *object, int radix,
		  int atsign, int collon, int mincol,
		  int padchar, int commachar, int commainterval)
{
    char stk[128], *str = stk;
    int i, length, sign, intervals;

    if (INT_P(object))
	format_integer(stk, object->data.integer, radix);
    else {		/* BIGINT_P */
	if (mpi_getsize(object->data.mp.integer, radix) >= sizeof(stk))
	    str = mpi_getstr(NULL, object->data.mp.integer, radix);
	else
	    mpi_getstr(str, object->data.mp.integer, radix);
    }

    sign = *str == '-';
    length = strlen(str);

    /* if collon, update length for the number of commachars to be printed */
    if (collon && commainterval > 0 && commachar) {
	intervals = length / commainterval;
	length += intervals;
    }
    else
	intervals = 0;

    /* if sign must be printed, and number is positive */
    if (atsign && !sign)
	++length;

    /* if need padding */
    if (padchar && mincol > length)
	LispWriteChars(mac, stream, padchar, mincol - length);

    /* if need to print number sign */
    if (sign || atsign)
	LispWriteChar(mac, stream, sign ? '-' : '+');

    /* if need to print commas to separate groups of numbers */
    if (intervals) {
	int j;
	char *ptr;

	i = (length - atsign) - intervals;
	j = i % commainterval;
	/* make the loop below easier */
	if (j == 0)
	    j = commainterval;
	i -= j;
	ptr = str + sign;
	for (; j > 0; j--, ptr++)
	    LispWriteChar(mac, stream, *ptr);
	for (; i > 0; i -= commainterval) {
	    LispWriteChar(mac, stream, commachar);
	    for (j = 0; j < commainterval; j++, ptr++)
		LispWriteChar(mac, stream, *ptr);
	}
    }
    /* else, just print the string */
    else
	LispWriteStr(mac, stream, str + sign);

    /* if number required more than sizeof(stk) bytes */
    if (str != stk)
	free(str);

    return (length);
}

int
LispFormatRomanInteger(LispMac *mac, LispObj *stream, long value, int new_roman)
{
    char stk[32];
    int length;

    length = 0;
    while (value > 1000) {
	stk[length++] = 'M';
	value -= 1000;
    }
    if (new_roman) {
	if (value >= 900) {
	    strcpy(stk + length, "CM");
	    length += 2,
	    value -= 900;
	}
	else if (value < 500 && value >= 400) {
	    strcpy(stk + length, "CD");
	    length += 2;
	    value -= 400;
	}
    }
    if (value >= 500) {
	stk[length++] = 'D';
	value -= 500;
    }
    while (value >= 100) {
	stk[length++] = 'C';
	value -= 100;
    }
    if (new_roman) {
	if (value >= 90) {
	    strcpy(stk + length, "XC");
	    length += 2,
	    value -= 90;
	}
	else if (value < 50 && value >= 40) {
	    strcpy(stk + length, "XL");
	    length += 2;
	    value -= 40;
	}
    }
    if (value >= 50) {
	stk[length++] = 'L';
	value -= 50;
    }
    while (value >= 10) {
	stk[length++] = 'X';
	value -= 10;
    }
    if (new_roman) {
	if (value == 9) {
	    strcpy(stk + length, "IX");
	    length += 2,
	    value -= 9;
	}
	else if (value == 4) {
	    strcpy(stk + length, "IV");
	    length += 2;
	    value -= 4;
	}
    }
    if (value >= 5) {
	stk[length++] = 'V';
	value -= 5;
    }
    while (value) {
	stk[length++] = 'I';
	--value;
    }

    stk[length] = '\0';

    return (LispWriteStr(mac, stream, stk));
}

int
LispFormatEnglishInteger(LispMac *mac, LispObj *stream, long number, int ordinal)
{
    static char *ds[] = {
	"",	      "one",	   "two",	 "three",      "four",
	"five",       "six",	   "seven",	 "eight",      "nine",
	"ten",	      "eleven",    "twelve",	 "thirteen",   "fourteen",
	"fifteen",    "sixteen",   "seventeen",  "eighteen",   "nineteen"
    };
    static char *dsth[] = {
	"",	      "first",	   "second",	  "third",	"fourth",
	"fifth",      "sixth",	   "seventh",	  "eighth",	"ninth",
	"tenth",      "eleventh",  "twelfth",	  "thirteenth", "fourteenth",
	 "fifteenth", "sixteenth", "seventeenth", "eighteenth", "nineteenth"
    };
    static char *hs[] = {
	"",	      "",	   "twenty",	  "thirty",	"forty",
	"fifty",      "sixty",	   "seventy",	  "eighty",	"ninety"
    };
    static char *hsth[] = {
	"",	      "",	   "twentieth",   "thirtieth",	"fortieth",
       "fiftieth",    "sixtieth",  "seventieth",  "eightieth",	"ninetieth"
    };
    static char *ts[] = {
	"",	      "thousand",   "million"
    };
    static char *tsth[] = {
	"",	     "thousandth", "millionth"
    };
    char stk[256];
    int length, sign;

    sign = number < 0;
    if (sign)
	number = -number;
    length = 0;

#define SIGNLEN		6	/* strlen("minus ") */
    if (sign) {
	strcpy(stk, "minus ");
	length += SIGNLEN;
    }
    else if (number == 0) {
	if (ordinal) {
	    strcpy(stk, "zeroth");
	    length += 6;	/* strlen("zeroth") */
	}
	else {
	    strcpy(stk, "zero");
	    length += 6;	/* strlen("zero") */
	}
    }
    for (;;) {
	int count, temp;
	char *t, *h, *d;
	long value = number;

	for (count = 0; value >= 1000; value /= 1000, count++)
	    ;

	t = ds[value / 100];
	if (ordinal && !count && (value % 10) == 0)
	    h = hsth[(value % 100) / 10];
	else
	    h = hs[(value % 100) / 10];

	if (ordinal && !count)
	    d = *h ? dsth[value % 10] : dsth[value % 20];
	else
	    d = *h ? ds[value % 10] : ds[value % 20];

	if (((!sign && length) || length > SIGNLEN) && (*t || *h || *d)) {
	    if (!ordinal || count || *h || *t) {
		strcpy(stk + length, ", ");
		length += 2;
	    }
	    else {
		strcpy(stk + length, " ");
		++length;
	    }
	}

	if (*t) {
	    if (ordinal && !count && (value % 100) == 0)
		temp = sprintf(stk + length, "%s hundredth", t);
	    else
		temp = sprintf(stk + length, "%s hundred", t);
	    length += temp;
	}

	if (*h) {
	    if (*t) {
		if (ordinal && !count) {
		    strcpy(stk + length, " ");
		    ++length;
		}
		else {
		    strcpy(stk + length, " and ");
		    length += 5;	/* strlen(" and ") */
		}
	    }
	    strcpy(stk + length, h);
	    length += strlen(h);
	}

	if (*d) {
	    if (*h) {
		strcpy(stk + length, "-");
		++length;
	    }
	    else if (*t) {
		if (ordinal && !count) {
		    strcpy(stk + length, " ");
		    ++length;
		}
		else {
		    strcpy(stk + length, " and ");
		    length += 5;	/* strlen(" and ") */
		}
	    }
	    strcpy(stk + length, d);
	    length += strlen(d);
	}

	if (!count)
	    break;
	else
	    temp = count;

	if (count > 1) {
	    value *= 1000;
	    while (--count)
		value *= 1000;
	    number -= value;
	}
	else
	    number %= 1000;

	if (ordinal && number == 0 && !*t && !*h)
	    temp = sprintf(stk + length, " %s", tsth[temp]);
	else
	    temp = sprintf(stk + length, " %s", ts[temp]);
	length += temp;

	if (!number)
	    break;
    }

    return (LispWriteStr(mac, stream, stk));
}

int
LispFormatCharacter(LispMac *mac, LispObj *stream, LispObj *object,
		    int atsign, int collon)
{
    int length = 0;
    int ch = object->data.integer;

    if (atsign && !collon)
	length += LispWriteStr(mac, stream, "#\\");
    if ((atsign || collon) && (ch <= ' ' || ch == 0177))
	length += LispWriteStr(mac, stream,
			       ch == 0177 ? "Rubout" : LispCharNames[ch]);
    else
	length += LispWriteChar(mac, stream, ch);

    return (length);
}

int
LispFormatFixedFloat(LispMac *mac, LispObj *stream, LispObj *object,
		     int atsign, int w, int *pd, int k, int overflowchar,
		     int padchar)
{
    char buffer[512], stk[64];
    int sign, exponent, length, offset, d = pd ? *pd : 16;
    double value = object->data.real;

    if (value == 0.0) {
	exponent = k = 0;
	strcpy(stk, "+0");
    }
    else
	/* calculate format parameters, adjusting scale factor */
	parse_double(stk, &exponent, value, d + 1 + k);

    /* make sure k won't cause overflow */
    if (k > 128)
	k = 128;
    else if (k < -128)
	k = -128;

    /* make sure d won't cause overflow */
    if (d > 128)
	d = 128;
    else if (d < -128)
	d = -128;

    /* adjust scale factor, exponent is used as an index in stk */
    exponent += k + 1;

    /* how many bytes in float representation */
    length = strlen(stk) - 1;

    /* need to print a sign? */
    sign = atsign || (stk[0] == '-');

    /* format number, cannot overflow, as control variables were checked */
    offset = 0;
    if (sign)
	buffer[offset++] = stk[0];
    if (exponent > 0) {
	if (exponent > length) {
	    memcpy(buffer + offset, stk + 1, length);
	    memset(buffer + offset + length, '0', exponent - length);
	}
	else
	    memcpy(buffer + offset, stk + 1, exponent);
	offset += exponent;
	buffer[offset++] = '.';
	if (length > exponent) {
	    memcpy(buffer + offset, stk + 1 + exponent, length - exponent);
	    offset += length - exponent;
	}
	else
	    buffer[offset++] = '0';
    }
    else {
	buffer[offset++] = '0';
	buffer[offset++] = '.';
	while (exponent < 0) {
	    buffer[offset++] = '0';
	    exponent++;
	}
	memcpy(buffer + offset, stk + 1, length);
	offset += length;
    }
    buffer[offset] = '\0';

fixed_float_check_again:
    /* make sure only d digits are printed after decimal point */
    if (d > 0) {
	char *dptr = strchr(buffer, '.');

	length = strlen(dptr) - 1;
	/* check if need to remove excess digits */
	if (length > d) {
	    int digit;

	    offset = (dptr - buffer) + 1 + d;
	    digit = buffer[offset];

	    /* remove extra digits */
	    buffer[offset] = '\0';

	    /* check if need to round */
	    if (offset > 1 && isdigit(digit) && digit >= '5') {
		/* XXX should adjust carry here? */
		if (isdigit(buffer[offset - 1]) &&
		    buffer[offset - 1] < '9')
		    buffer[offset - 1]++;
	    }
	}
	/* check if need to add extra zero digits to fill space */
	else if (length < d) {
	    offset += d - length;
	    for (++length; length <= d; length++)
		dptr[length] = '0';
	    dptr[length] = '\0';
	}
    }
    else {
	/* no digits after decimal point */
	int digit;
	char *dptr = strchr(buffer, '.') + 1;

	digit = *dptr;
	if (digit >= '5' && dptr > buffer + 2 &&
	    isdigit(dptr[-2]) && dptr[-2] < '9')
	    /* XXX should adjust carry here? */
	    dptr[-2]++;

	offset = (dptr - buffer);
	buffer[offset] = '\0';
    }

    /* if d was not specified, remove any extra zeros */
    if (pd == NULL) {
	while (offset > 2 && buffer[offset - 2] != '.' &&
	       buffer[offset - 1] == '0')
	    --offset;
	buffer[offset] = '\0';
    }

    if (w > 0 && offset > w) {
	/* first check if can remove extra fractional digits */
	if (pd == NULL) {
	    char *ptr = strchr(buffer, '.') + 1;

	    if (ptr - buffer < w) {
		d = w - (ptr - buffer);
		goto fixed_float_check_again;
	    }
	}

	/* remove leading "zero" to save space */
 	if ((!sign && buffer[0] == '0') || (sign && buffer[1] == '0')) {
	    /* ending nul also copied */
	    memmove(buffer + sign, buffer + sign + 1, offset);
	    --offset;
	}
	/* remove leading '+' to "save" space */
	if (offset > w && buffer[0] == '+') {
	    /* ending nul also copied */
	    memmove(buffer, buffer + 1, offset);
	    --offset;
	}
    }

    /* if cannot represent number in given width */
    if (overflowchar && offset > w)
	goto fixed_float_overflow;

    /* print padding if required */
    if (w > offset)
	LispWriteChars(mac, stream, padchar, w - offset);

    /* print float number representation */
    return (LispWriteStr(mac, stream, buffer));

fixed_float_overflow:
    return (LispWriteChars(mac, stream, overflowchar, w));
}

int
LispFormatExponentialFloat(LispMac *mac, LispObj *stream, LispObj *object,
			   int atsign, int w, int *pd, int e, int k,
			   int overflowchar, int padchar, int exponentchar)
{
    return (LispDoFormatExponentialFloat(mac, stream, object, atsign, w,
					 pd, e, k, overflowchar, padchar,
					 exponentchar, 1));
}

int
LispDoFormatExponentialFloat(LispMac *mac, LispObj *stream, LispObj *object,
			     int atsign, int w, int *pd, int e, int k,
			     int overflowchar, int padchar, int exponentchar,
			     int format)
{
    char buffer[512], stk[64];
    int sign, exponent, length, offset, d = pd ? *pd : 16;
    double value = object->data.real;

    if (value == 0.0) {
	exponent = 0;
	k = 1;
	strcpy(stk, "+0");
    }
    else
	/* calculate format parameters, adjusting scale factor */
	parse_double(stk, &exponent, value, d + k - 1);

    /* set e to a value that won't overflow */
    if (e > 16)
	e = 16;

    /* set k to a value that won't overflow */
    if (k > 128)
	k = 128;
    else if (k < -128)
	k = -128;

    /* set d to a value that won't overflow */
    if (d > 128)
	d = 128;
    else if (d < -128)
	d = -128;

    /* how many bytes in float representation */
    length = strlen(stk) - 1;

    /* need to print a sign? */
    sign = atsign || (stk[0] == '-');

    /* adjust number of digits after decimal point */
    if (k > 0)
	d -= k - 1;

    /* adjust exponent, based on scale factor */
    exponent -= k - 1;

    /* format number, cannot overflow, as control variables were checked */
    offset = 0;
    if (sign)
	buffer[offset++] = stk[0];
    if (k > 0) {
	if (k > length) {
	    memcpy(buffer + offset, stk + 1, length);
	    offset += length;
	}
	else {
	    memcpy(buffer + offset, stk + 1, k);
	    offset += k;
	}
	buffer[offset++] = '.';
	if (length > k) {
	    memcpy(buffer + offset, stk + 1 + k, length - k);
	    offset += length - k;
	}
 	else
	    buffer[offset++] = '0';
    }
    else {
	int tmp = k;

	buffer[offset++] = '0';
	buffer[offset++] = '.';
	while (tmp < 0) {
	    buffer[offset++] = '0';
	    tmp++;
	}
	memcpy(buffer + offset, stk + 1, length);
	offset += length;
    }

    /* if format, then always add a sign to exponent */
    buffer[offset++] = exponentchar;
    if (format || exponent < 0)
	buffer[offset++] = exponent < 0 ? '-' : '+';

    /* XXX destroy stk contents */
    sprintf(stk, "%%0%dd", e);
    /* format scale factor*/
    length = sprintf(buffer + offset, stk,
		     exponent < 0 ? -exponent : exponent);
    /* check for overflow in exponent */
    if (length > e && overflowchar)
	goto exponential_float_overflow;
    offset += length;

    /* make sure only d digits are printed after decimal point */
    if (d > 0) {
	int currd;
	char *dptr = strchr(buffer, '.'),
	     *eptr = strchr(dptr, exponentchar);

	currd = eptr - dptr - 1;
	length = strlen(eptr);

	/* check if need to remove excess digits */
	if (currd > d) {
	    int digit, dpos;

	    dpos = offset = (dptr - buffer) + 1 + d;
	    digit = buffer[offset];

	    memmove(buffer + offset, eptr, length + 1);
	    /* also copy ending nul character */

	    /* adjust offset to length of total string */
	    offset += length;

	    /* check if need to round */
	    if (dpos > 1 && isdigit(digit) && digit >= '5') {
		/* XXX should adjust carry here? */
		if (isdigit(buffer[dpos - 1]) &&
		    buffer[dpos - 1] < '9')
		    buffer[dpos - 1]++;
	    }
	}
	/* check if need to add extra zero digits to fill space */
	else if (pd && currd < d) {
	    memmove(eptr + d - currd, eptr, length + 1);
	    /* also copy ending nul character */

	    offset += d - currd;
	    for (++currd; currd <= d; currd++)
		dptr[currd] = '0';
	}
	/* check if need to remove zeros */
	else if (pd == NULL) {
	    int zeros = 1;

	    while (eptr[-zeros] == '0')
		++zeros;
	    if (eptr[-zeros] == '.')
		--zeros;
	    if (zeros > 1) {
		memmove(eptr - zeros + 1, eptr, length + 1);
		offset -= zeros - 1;
	    }
	}
    }
    else {
	/* no digits after decimal point */
	int digit;
	char *dptr = strchr(buffer, '.'),
	     *eptr = strchr(dptr, exponentchar);

	digit = dptr[1];

	offset = (dptr - buffer) + 1;
	length = strlen(eptr);
	memmove(buffer + offset, eptr, length + 1);
	/* also copy ending nul character */

 	if (digit >= '5' && dptr > buffer + 2 &&
	    isdigit(dptr[-2]) && dptr[-2] < '9')
	    /* XXX should adjust carry here? */
	    dptr[-2]++;

	/* adjust offset to length of total string */
	offset += length;
    }

    if (w > 0 && offset > w) {
	/* remove leading "zero" to save space */
	if ((!sign && buffer[0] == '0') || (sign && buffer[1] == '0')) {
	    /* ending nul also copied */
	    memmove(buffer + sign, buffer + sign + 1, offset);
	    --offset;
	}
	/* remove leading '+' to "save" space */
	if (offset > w && buffer[0] == '+') {
	    /* ending nul also copied */
	    memmove(buffer, buffer + 1, offset);
	    --offset;
	}
    }

    /* if cannot represent number in given width */
    if (overflowchar && offset > w)
	goto exponential_float_overflow;

    /* print padding if required */
    if (w > offset)
	LispWriteChars(mac, stream, padchar, w - offset);

    /* print float number representation */
    return (LispWriteStr(mac, stream, buffer));

exponential_float_overflow:
    return (LispWriteChars(mac, stream, overflowchar, w));
}

int
LispFormatGeneralFloat(LispMac *mac, LispObj *stream, LispObj *object,
		       int atsign, int w, int *pd, int e, int k,
		       int overflowchar, int padchar, int exponentchar)
{
    char stk[64];
    int length, exponent, n, dd, ee, ww, d = pd ? *pd : 16;
    double value = object->data.real;

    if (value == 0.0) {
	exponent = 0;
	n = 0;
	d = 1;
	strcpy(stk, "+0");
    }
    else {
	/* calculate format parameters, adjusting scale factor */
	parse_double(stk, &exponent, value, d + k - 1);
	n = exponent + 1;
    }

    /* Let ee equal e+2, or 4 if e is omitted. */
    if (e)
	ee = e + 2;
    else
	ee = 4;

    /* Let ww equal w-ee, or nil if w is omitted. */
    if (w)
	ww = w - ee;
    else
	ww = 0;

    dd = d - n;
    if (d >= dd && dd >= 0) {
	length = LispFormatFixedFloat(mac, stream, object, atsign, ww,
				      pd, 0, overflowchar, padchar);

	/* ~ee@T */
	length += LispWriteChars(mac, stream, padchar, ee);
    }
    else
	length = LispFormatExponentialFloat(mac, stream, object, atsign,
					    w, pd, e, k, overflowchar,
					    padchar, exponentchar);

    return (length);
}

int
LispFormatDollarFloat(LispMac *mac, LispObj *stream, LispObj *object,
		      int atsign, int collon, int d, int n, int w, int padchar)
{
    char buffer[512], stk[64];
    int bytes, sign, exponent, length, offset;
    double value = object->data.real;

    bytes = 0;

    if (value == 0.0) {
	exponent = 0;
	strcpy(stk, "+0");
    }
    else
	/* calculate format parameters, adjusting scale factor */
	parse_double(stk, &exponent, value, d == 0 ? 16 : d + 1);

    /* set d to a "sane" value */
    if (d > 128)
	d = 128;

    /* set n to a "sane" value */
    if (n > 128)
	n = 128;

    /* use exponent as index in stk */
    ++exponent;

    /* don't put sign in buffer,
     * if collon specified, must go before padding */
    sign = atsign || (stk[0] == '-');

    offset = 0;

    /* pad with zeros if required */
    if (exponent > 0)
	n -= exponent;
    while (n > 0) {
	buffer[offset++] = '0';
	n--;
    }

    /* how many bytes in float representation */
    length = strlen(stk) - 1;

    if (exponent > 0) {
	if (exponent > length) {
	    memcpy(buffer + offset, stk + 1, length);
	    memset(buffer + offset + length, '0', exponent - length);
	}
	else
	    memcpy(buffer + offset, stk + 1, exponent);
	offset += exponent;
	buffer[offset++] = '.';
	if (length > exponent) {
	    memcpy(buffer + offset, stk + 1 + exponent, length - exponent);
	    offset += length - exponent;
	}
	else
	    buffer[offset++] = '0';
    }
    else {
	if (n > 0)
	    buffer[offset++] = '0';
	buffer[offset++] = '.';
	while (exponent < 0) {
	    buffer[offset++] = '0';
	    exponent++;
	}
	memcpy(buffer + offset, stk + 1, length);
	offset += length;
    }
    buffer[offset] = '\0';

    /* make sure only d digits are printed after decimal point */
    if (d > 0) {
	char *dptr = strchr(buffer, '.');

	length = strlen(dptr) - 1;
	/* check if need to remove excess digits */
	if (length > d) {
	    int digit;

	    offset = (dptr - buffer) + 1 + d;
	    digit = buffer[offset];

	    /* remove extra digits */
	    buffer[offset] = '\0';

	    /* check if need to round */
	    if (offset > 1 && isdigit(digit) && digit >= '5') {
		/* XXX should adjust carry here? */
		if (isdigit(buffer[offset - 1]) &&
		    buffer[offset - 1] < '9')
		    buffer[offset - 1]++;
	    }
	}
	/* check if need to add extra zero digits to fill space */
	else if (length < d) {
	    offset += d - length;
	    for (++length; length <= d; length++)
		dptr[length] = '0';
	    dptr[length] = '\0';
	}
    }
    else {
	/* no digits after decimal point */
	int digit;
	char *dptr = strchr(buffer, '.') + 1;

	digit = *dptr;
	if (digit >= '5' && dptr > buffer + 2 &&
	    isdigit(dptr[-2]) && dptr[-2] < '9')
	    /* XXX should adjust carry here? */
	    dptr[-2]++;

	offset = (dptr - buffer);
	buffer[offset] = '\0';
    }

    if (sign) {
	++offset;
	if (atsign && collon)
	    bytes += LispWriteChar(mac, stream, value >= 0.0 ? '+' : '-');
    }

    /* print padding if required */
    if (w > offset)
	bytes += LispWriteChars(mac, stream, padchar, w - offset);

    if (atsign && !collon)
	bytes += LispWriteChar(mac, stream, value >= 0.0 ? '+' : '-');

    /* print float number representation */
    return (LispWriteStr(mac, stream, buffer) + bytes);
}

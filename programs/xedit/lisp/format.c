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

/* $XFree86: xc/programs/xedit/lisp/format.c,v 1.15 2002/02/08 03:54:07 paulo Exp $ */

#include "io.h"
#include "write.h"
#include "format.h"
#include <ctype.h>

#define MAXFMT			8
#define NOERROR			0

/* parse error codes */
#define PARSE_2MANYPARM		1	/* too many directive parameters */
#define PARSE_2MANYATS		2	/* more than one @ in directive */
#define PARSE_2MANYCOLS		3	/* more than one : in directive */
#define PARSE_NOARGSLEFT	4	/* no arguments left to format */
#define PARSE_BADFMTARG		5	/* argument is not an integer or char */
#define PARSE_BADDIRECTIVE	6	/* unknown format directive */
#define PARSE_BADINTEGER	7	/* bad integer representation */

/* merge error codes */
#define MERGE_2MANY		1	/* too many parameters to directive */
#define MERGE_NOCHAR		2	/* parameter must be a character */
#define MERGE_NOINT		3	/* parameter must be an integer */

/* generic error codes */
#define GENERIC_RADIX		1	/* radix not in range 2-36 */
#define GENERIC_NEGATIVE	2	/* parameter is negative */
#define GENERIC_BADSTRING	3	/* argument is not a string */
#define GENERIC_BADLIST		4	/* argument is not a list */

#define IF_SPECIFIED(arg)	(arg).specified ? &((arg).value) : NULL

#define UPANDOUT_NORMAL		1
#define UPANDOUT_COLLON		2
#define UPANDOUT_HASH		4	/* only useful inside a ~{ iteration
					 * forces loop finalization. */

#define ITERATION_NORMAL	1
#define ITERATION_LAST		2

/*
 * Types
 */
/* parameter to format */
typedef struct {
    unsigned int achar : 1;	/* value was specified as a character */
    unsigned int specified : 1;	/* set if value was specified */
    unsigned int offset : 30;	/* offset in format string, for error printing */
    int value;
} FmtArg;

/* information about format parameters */
typedef struct {
    unsigned int atsign : 1;	/* @ specified */
    unsigned int collon : 1;	/* : specified */
    unsigned int command : 8;	/* the format command */
    unsigned int count : 4;	/* number of arguments processed */
    unsigned int offset : 10;	/* offset in format string, for error printing */
    char *base, *format;
    FmtArg arguments[MAXFMT];
} FmtArgs;

/* used for combining default format parameter values */
typedef struct {
    int achar;
    int value;
} FmtDef;

/* number of default format parameter values and defaults */
typedef struct {
    int count;
    FmtDef defaults[MAXFMT];
} FmtDefs;

/* used on recursive calls to LispFormat */
typedef struct {
    FmtArgs args;
    LispObj *base_arguments;	/* pointer to first format argument */
    int total_arguments;	/* number of objects in base_arguments */
    char **format;		/* if need to update format string pointer */
    LispObj **object;		/* CAR(arguments), for plural check */
    LispObj **arguments;	/* current element of base_arguments */
    int *num_arguments;		/* number of arguments after arguments */
    int upandout;		/* information for recursive calls */
    int iteration;		/* only set if in ~:{... or ~:@{ and in the
				 * last argument list, hint for upandout */
} FmtInfo;

/*
 * Prototypes
 */
static void merge_arguments(FmtArgs*, FmtDefs*, int*);
static char *parse_arguments(char*, FmtArgs*, int*, LispObj**, int*);
static void merge_error(LispMac*, FmtArgs*, int);
static void parse_error(LispMac*, FmtArgs*, int);
static void generic_error(LispMac*, FmtArgs*, int);
static void format_error(LispMac*, FmtArgs*, char*);

static int format_object(LispMac*, LispObj*, LispObj*);

static void format_ascii(LispMac*, LispObj*, LispObj*, FmtArgs*);
static void format_in_radix(LispMac*, LispObj*, LispObj*, int, FmtArgs*);
static void format_radix_special(LispMac*, LispObj*, LispObj*, FmtArgs*);
static void format_roman(LispMac*, LispObj*, LispObj*, FmtArgs*);
static void format_english(LispMac*, LispObj*, LispObj*, FmtArgs*);
static void format_character(LispMac*, LispObj*, LispObj*, FmtArgs*);
static void format_fixed_float(LispMac*, LispObj*, LispObj*, FmtArgs*);
static void format_exponential_float(LispMac*, LispObj*, LispObj*, FmtArgs*);
static void format_general_float(LispMac*, LispObj*, LispObj*, FmtArgs*);
static void format_dollar_float(LispMac*, LispObj*, LispObj*, FmtArgs*);
static void format_tabulate(LispMac*, LispObj*, FmtArgs*);

static void format_goto(LispMac*, FmtInfo*);
static void format_indirection(LispMac*, LispObj*, LispObj*, FmtInfo*);

static void list_formats(LispMac*, FmtInfo*, int, char**, char***,
			 int*, int*, int*, int*);
static void free_formats(LispMac*, char**, int);

static void format_case_conversion(LispMac*, LispObj*, FmtInfo*);
static void format_conditional(LispMac*, LispObj*, FmtInfo*);
static void format_iterate(LispMac*, LispObj*, FmtInfo*);
static void format_justify(LispMac*, LispObj*, FmtInfo*);

static void LispFormat(LispMac*, LispObj*, FmtInfo*);

/*
 * Initialization
 */
static FmtDefs AsciiDefs = {
    4,
    {
	{0, 0},			/* mincol */
	{0, 1},			/* colinc */
	{0, 0},			/* minpad */
	{1, ' '},		/* padchar */
    },
};

static FmtDefs IntegerDefs = {
    4,
    {
	{0, 0},			/* mincol */
	{1, ' '},		/* padchar */
	{1, ','},		/* commachar */
	{0, 3},			/* commainterval */
    },
};

static FmtDefs RadixDefs = {
    5,
    {
	{0, 10},		/* radix */
	{0, 0},			/* mincol */
	{1, ' '},		/* padchar */
	{1, ','},		/* commachar */
	{0, 3},			/* commainterval */
    },
};

static FmtDefs NoneDefs = {
    0,
};

static FmtDefs FixedFloatDefs = {
    5,
    {
	{0, 0},			/* w */
	{0, 16},		/* d */
	{0, 0},			/* k */
	{1, '\0'},		/* overflowchar */
	{1, ' '},		/* padchar */
    },
};

static FmtDefs ExponentialFloatDefs = {
    7,
    {
	{0, 0},			/* w */
	{0, 16},		/* d */
	{0, 0},			/* e */
	{0, 1},			/* k */
	{1, '\0'},		/* overflowchar */
	{1, ' '},		/* padchar */
	{1, 'E'},		/* exponentchar */
	/* XXX if/when more than one float format,
	 * should default to object type */
    },
};

static FmtDefs DollarFloatDefs = {
    4,
    {
	{0, 2},			/* d */
	{0, 1},			/* n */
	{0, 0},			/* w */
	{1, ' '},		/* padchar */
    },
};

static FmtDefs OneDefs = {
    1,
    {
	{0, 1},
    },
};

static FmtDefs TabulateDefs = {
    2,
    {
	{0, 0},			/* colnum */
	{0, 1},			/* colinc */
    },
};

/*
 * Implementation
 */
static void
merge_arguments(FmtArgs *arguments, FmtDefs *defaults, int *code)
{
    int count;
    FmtDef *defaul;
    FmtArg *argument;

    defaul = &(defaults->defaults[0]);
    argument = &(arguments->arguments[0]);
    for (count = 0; count < defaults->count; count++, argument++, defaul++) {
	if (count >= arguments->count)
	    argument->specified = 0;
	if (argument->specified) {
	    if (argument->achar != defaul->achar) {
		*code = defaul->achar ? MERGE_NOCHAR : MERGE_NOINT;
		arguments->offset = argument->offset;
		return;
	    }
	}
	else {
	    argument->specified = 0;
	    argument->achar = defaul->achar;
	    argument->value = defaul->value;
	}
    }

    /* check if extra arguments were provided */
    if (arguments->count > defaults->count)
	*code = MERGE_2MANY;
}

/* the pointer arguments may be null, useful when just testing/parsing
 * the directive parameters */
static char *
parse_arguments(char *format, FmtArgs *arguments,
		int *num_objects, LispObj **objects, int *code)
{
    int test;
    char *ptr;
    FmtArg *argument;

    /* initialize */
    test = objects == NULL || code == NULL || num_objects == NULL;
    ptr = format;
    argument = &(arguments->arguments[0]);
    arguments->atsign = arguments->collon = arguments->command = 0;

    /* parse format parameters */
    for (arguments->count = 0;; arguments->count++) {
	arguments->offset = ptr - format + 1;
	if (arguments->count >= MAXFMT) {
	    if (!test)
		*code = PARSE_2MANYPARM;
	    return (ptr);
	}
	if (*ptr == '\'') {		/* character parameter value */
	    ++ptr;			/* skip ' */
	    argument->achar = argument->specified = 1;
	    argument->value = *ptr++;
	}
	else if (*ptr == ',') {		/* use default parameter value */
	    argument->achar = 0;
	    argument->specified = 0;
	    /* don't increment ptr, will be incremented below */
	}
	else if (*ptr == '#') {		/* number of arguments is value */
	    ++ptr;			/* skip # */
	    argument->achar = 0;
	    argument->specified = 1;
	    if (!test)
		argument->value = *num_objects;
	}
	else if (*ptr == 'v' ||
		 *ptr == 'V') {		/* format object argument is value */
	    LispObj *object;

	    ++ptr;			/* skip V */
	    if (!test) {
		if (!CONS_P(*objects)) {
		    *code = PARSE_NOARGSLEFT;
		    return (ptr);
		}
		object = CAR((*objects));
		if (INT_P(object)) {
		    argument->achar = 0;
		    argument->specified = 1;
		    argument->value = object->data.integer;
		}
		else if (CHAR_P(object)) {
		    argument->achar = argument->specified = 1;
		    argument->value = object->data.integer;
		}
		else {
		    *code = PARSE_BADFMTARG;
		    return (ptr);
		}
		*objects = CDR(*objects);
		--*num_objects;
	    }
	}
	else if (isdigit(*ptr) ||
		*ptr == '-' || *ptr == '+') {	/* integer parameter value */
	    int sign;

	    argument->achar = 0;
	    argument->specified = 1;
	    if (!isdigit(*ptr)) {
		sign = *ptr++ == '-';
	    }
	    else
		sign = 0;
	    if (!test && !isdigit(*ptr)) {
		*code = PARSE_BADINTEGER;
		return (ptr);
	    }
	    argument->value = *ptr++ - '0';
	    while (isdigit(*ptr)) {
		argument->value = (argument->value * 10) + (*ptr++ - '0');
		if (argument->value > 65536) {
		    if (!test) {
			*code = PARSE_BADINTEGER;
			return (ptr);
		    }
		}
	    }
	    if (sign)
		argument->value = -argument->value;
	}
	else				/* no more arguments to format */
	    break;

	if (*ptr == ',')
	    ++ptr;

	/* remember offset of format parameter, for better error printing */
	argument->offset = arguments->offset;
	argument++;
    }

    /* check for extra flags */
    for (;;) {
	if (*ptr == '@') {		/* check for special parameter atsign */
	    if (arguments->atsign) {
		if (!test) {
		    *code = PARSE_2MANYATS;
		    return (ptr);
		}
	    }
	    ++ptr;
	    ++arguments->offset;
	    arguments->atsign = 1;
	}
	else if (*ptr == ':') {		/* check for special parameter collon */
	    if (arguments->collon) {
		if (!test) {
		    *code = PARSE_2MANYCOLS;
		    return (ptr);
		}
	    }
	    ++ptr;
	    ++arguments->offset;
	    arguments->collon = 1;
	}
	else				/* next value is format command */
	    break;
    }

    if (!test)
	*code = NOERROR;
    arguments->command = *ptr++;
    if (islower(arguments->command))
	arguments->command = toupper(arguments->command);
    ++arguments->offset;

    return (ptr);
}

static void
parse_error(LispMac *mac, FmtArgs *args, int code)
{
    static char *errors[] = {
	NULL,
	"too many parameters to directive",
	"too many @ parameters",
	"too many : parameters",
	"no arguments left to format",
	"argument is not a fixnum integer or a character",
	"unknown format directive",
	"parameter is not a fixnum integer",
    };

    format_error(mac, args, errors[code]);
}

static void
merge_error(LispMac *mac, FmtArgs *args, int code)
{
    static char *errors[] = {
	NULL,
	"too many parameters to directive",
	"argument must be a character",
	"argument must be a fixnum integer",
    };

    format_error(mac, args, errors[code]);
}

static void
generic_error(LispMac *mac, FmtArgs *args, int code)
{
    static char *errors[] = {
	NULL,
	"radix must be in the range 2 to 36, inclusive",
	"parameter must be positive",
	"argument must be a string",
	"argument must be a list",
    };

    format_error(mac, args, errors[code]);
}

static void
format_error(LispMac *mac, FmtArgs *args, char *str)
{
    char *message;
    int errorlen, formatlen;

    /* number of bytes of format to be printed */
    formatlen = (args->format - args->base) + args->offset;

    /* length of specific error message */
    errorlen = strlen(str) + 1;			/* plus '\n' */

    /* XXX allocate string with LispMalloc,
     * so that it will be freed in LispTopLevel */
    message = LispMalloc(mac, formatlen + errorlen + 1);

    sprintf(message, "%s\n", str);
    memcpy(message + errorlen, args->base, formatlen);
    message[errorlen + formatlen] = '\0';

    LispDestroy(mac, "FORMAT: %s", message);
}

static int
format_object(LispMac *mac, LispObj *stream, LispObj *object)
{
    int length;

    length = LispWriteObject(mac, stream, object);

    return (length);
}

static void
format_ascii(LispMac *mac, LispObj *stream, LispObj *object, FmtArgs *args)
{
    LispObj *string = NIL;
    int length = 0,
	protect = mac->protect.length,
	atsign = args->atsign,
	collon = args->collon,
	mincol = args->arguments[0].value,
	colinc = args->arguments[1].value,
	minpad = args->arguments[2].value,
	padchar = args->arguments[3].value;

    /* check/correct arguments */
    if (mincol < 0)
	mincol = 0;
    if (colinc < 0)
	colinc = 1;
    if (minpad < 0)
	minpad = 0;
    /* XXX pachar can be the null character? */

    if (object == NIL)
	length = collon ? 2 : 3;	    /* () or NIL */
    else if (CHAR_P(object))
	length = 1;

    /* left padding */
    if (atsign) {
	/* if length not yet known */
	if (object != NIL && !CHAR_P(object)) {
	    string = STRINGSTREAM((unsigned char*)"",
				  STREAM_READ | STREAM_WRITE);
	    if (protect + 1 >= mac->protect.space)
		LispMoreProtects(mac);
	    mac->protect.objects[mac->protect.length++] = string;
	    length = LispWriteObject(mac, string, object);
	}

	/* output minpad characters at left */
	if (minpad) {
	    length += minpad;
	    LispWriteChars(mac, stream, padchar, minpad);
	}

	if (colinc) {
	    /* puts colinc spaces at a time,
	     * until at least mincol chars out */
	    while (length < mincol) {
		LispWriteChars(mac, stream, padchar, colinc);
		length += colinc;
	    }
	}
    }

    if (object == NIL)
	LispWriteStr(mac, stream, collon ? "()" : "NIL");
    else {
	if (CHAR_P(object))
	    LispWriteChar(mac, stream, object->data.integer);
	else {
	    /* if string is not NIL, atsign was specified
	     * and object printed to string */
	    if (string == NIL)
		length = format_object(mac, stream, object);
	    else
		/* XXX a string-stream can have nulls */
		LispWriteStr(mac, stream, LispGetSstring(SSTREAMP(string)));
	}
    }

    /* right padding */
    if (!atsign) {
	/* output minpad characters at left */
	if (minpad) {
	    length += minpad;
	    LispWriteChars(mac, stream, padchar, minpad);
	}
	if (colinc) {
	    /* puts colinc spaces at a time,
	     * until at least mincol chars out */
	    while (length < mincol) {
		LispWriteChars(mac, stream, padchar, colinc);
		length += colinc;
	    }
	}
    }

    mac->protect.length = protect;
}

/* assumes radix is 0 or in range 2 - 36 */
static void
format_in_radix(LispMac *mac, LispObj *stream, LispObj *object,
	       int radix, FmtArgs *args)
{
    if (INTEGER_P(object)) {
	int i, check, atsign, collon, mincol, padchar, commachar, commainterval;

	i = check = (radix == 0);
	atsign = args->atsign;
	collon = args->collon;
	if (radix == 0) {
	    radix = args->arguments[0].value;
	    if (radix < 2 || radix > 36) {
		args->offset = args->arguments[0].offset;
		generic_error(mac, args, GENERIC_RADIX);
	    }
	}
	mincol = args->arguments[i++].value;
	padchar = args->arguments[i++].value;
	commachar = args->arguments[i++].value;
	commainterval = args->arguments[i++].value;

	LispFormatInteger(mac, stream, object, radix, atsign, collon,
			  mincol, padchar, commachar, commainterval);
    }
    else
	format_object(mac, stream, object);
}

static void
format_radix_special(LispMac *mac, LispObj *stream,
		     LispObj *object, FmtArgs *args)
{
    if (INTEGER_P(object)) {
	if (args->atsign)
	    format_roman(mac, stream, object, args);
	else
	    format_english(mac, stream, object, args);
    }
    else
	format_object(mac, stream, object);
}

static void
format_roman(LispMac *mac, LispObj *stream, LispObj *object, FmtArgs *args)
{
    long value = 0;
    int cando, new_roman = args->collon == 0;

    if (INT_P(object)) {
	value = object->data.integer;
	if (new_roman)
	    cando = value >= 1 && value <= 3999;
	else
	    cando = value >= 1 && value <= 4999;
    }
    else
	cando = 0;

    if (cando)
	LispFormatRomanInteger(mac, stream, value, new_roman);
    else
	format_object(mac, stream, object);
}

static void
format_english(LispMac *mac, LispObj *stream, LispObj *object, FmtArgs *args)
{
    int cando;
    long number = 0;

    if (INT_P(object)) {
	number = object->data.integer;
	cando = number >= -999999999 && number <= 999999999;
    }
    else
	cando = 0;

    if (cando)
	LispFormatEnglishInteger(mac, stream, number, args->collon);
    else
	format_object(mac, stream, object);
}

static void
format_character(LispMac *mac, LispObj *stream, LispObj *object, FmtArgs *args)
{
    if (CHAR_P(object))
	LispFormatCharacter(mac, stream, object, args->atsign, args->collon);
    else
	format_object(mac, stream, object);
}

static void
format_fixed_float(LispMac *mac, LispObj *stream,
		   LispObj *object, FmtArgs *args)
{
    if (FLOAT_P(object))
	LispFormatFixedFloat(mac, stream, object, args->atsign,
			     args->arguments[0].value,
			     IF_SPECIFIED(args->arguments[1]),
			     args->arguments[2].value,
			     args->arguments[3].value,
			     args->arguments[4].value);
    else
	format_object(mac, stream, object);
}

static void
format_exponential_float(LispMac *mac, LispObj *stream,
			 LispObj *object, FmtArgs *args)
{
    if (FLOAT_P(object))
	LispFormatExponentialFloat(mac, stream, object, args->atsign,
				   args->arguments[0].value,
				   IF_SPECIFIED(args->arguments[1]),
				   args->arguments[2].value,
				   args->arguments[3].value,
				   args->arguments[4].value,
				   args->arguments[5].value,
				   args->arguments[6].value);
    else
	format_object(mac, stream, object);
}

static void
format_general_float(LispMac *mac, LispObj *stream,
		     LispObj *object, FmtArgs *args)
{
    if (FLOAT_P(object))
	LispFormatGeneralFloat(mac, stream, object, args->atsign,
				args->arguments[0].value,
				IF_SPECIFIED(args->arguments[1]),
				args->arguments[2].value,
				args->arguments[3].value,
				args->arguments[4].value,
				args->arguments[5].value,
				args->arguments[6].value);
    else
	format_object(mac, stream, object);
}

static void
format_dollar_float(LispMac *mac, LispObj *stream,
		    LispObj *object, FmtArgs *args)
{
    if (FLOAT_P(object))
	LispFormatDollarFloat(mac, stream, object,
			      args->atsign, args->collon,
			      args->arguments[0].value,
			      args->arguments[1].value,
			      args->arguments[2].value,
			      args->arguments[3].value);
    else
	format_object(mac, stream, object);
}

static void
format_tabulate(LispMac *mac, LispObj *stream, FmtArgs *args)
{
    int atsign = args->atsign,
	colnum = args->arguments[0].value,
	colinc = args->arguments[1].value,
	column;

    column = LispGetColumn(mac, stream);

    if (atsign) {
	/* relative tabulation */
	if (colnum > 0) {
	    LispWriteChars(mac, stream, ' ', colnum);
	    column += colnum;
	}
	/* tabulate until at a multiple of colinc */
	if (colinc > 0)
	    LispWriteChars(mac, stream, ' ', colinc - (column % colinc));
    }
    else {
	/* if colinc not specified, just move to given column */
	if (colinc <= 0)
	    LispWriteChars(mac, stream, ' ', column - colnum);
	else {
	    /* always output at least colinc spaces */
	    do {
		LispWriteChars(mac, stream, ' ', colinc);
		colnum -= colinc;
	    } while (colnum > column);
	}
    }
}

static void
format_goto(LispMac *mac, FmtInfo *info)
{
    int count, num_arguments;
    LispObj *object, *arguments;

    /* number of arguments to ignore or goto offset */
    count = info->args.arguments[0].value;
    if (count < 0)
	generic_error(mac, &(info->args), GENERIC_NEGATIVE);

    if (info->args.atsign) {
	/* absolute goto */

	/* if not specified, defaults to zero */
	if (!(info->args.arguments[0].specified))
	    count = 0;

	/* if offset too large */
	if (count > info->total_arguments)
	    parse_error(mac, &(info->args), PARSE_NOARGSLEFT);
	else if (count != info->total_arguments - *(info->num_arguments)) {
	    /* calculate new parameters */
	    object = NIL;
	    arguments = info->base_arguments;
	    num_arguments = info->total_arguments - count;

	    for (; count > 0; count--, arguments = CDR(arguments))
		object = CAR(arguments);

	    /* update format information */
	    *(info->object) = object;
	    *(info->arguments) = arguments;
	    *(info->num_arguments) = num_arguments;
	}
    }
    else if (count) {
	/* relative goto, ignore or go back count arguments */

	/* prepare to update parameters */
	arguments = *(info->arguments);
	num_arguments = *(info->num_arguments);

	/* go back count arguments? */
	if (info->args.collon)
	    count = -count;

	num_arguments -= count;

	if (count > 0) {
	    if (count > *(info->num_arguments))
		parse_error(mac, &(info->args), PARSE_NOARGSLEFT);

	    object = *(info->object);
	    for (; count > 0; count--, arguments = CDR(arguments))
		object = CAR(arguments);
	}
	else {		/* count < 0 */
	    if (info->total_arguments + count - *(info->num_arguments) < 0)
		parse_error(mac, &(info->args), PARSE_NOARGSLEFT);

	    object = NIL;
	    arguments = info->base_arguments;
	    for (count = 0; info->total_arguments - count < num_arguments;
		count++, arguments = CDR(arguments))
		object = CAR(arguments);
	}

	/* update format parameters */
	*(info->object) = object;
	*(info->arguments) = arguments;
	*(info->num_arguments) = num_arguments;
    }
}

static void
format_indirection(LispMac *mac, LispObj *stream, LispObj *format, FmtInfo *info)
{
    char *string;
    LispObj *object;
    FmtInfo indirect_info;

    if (!STRING_P(format))
	generic_error(mac, &(info->args), GENERIC_BADSTRING);
    string = STRPTR(format);

    /* most information is the same */
    memcpy(&indirect_info, info, sizeof(FmtInfo));

    /* set new format string */
    indirect_info.args.base = indirect_info.args.format = string;
    indirect_info.format = &string;

    if (info->args.atsign) {
	/* use current arguments */

	/* do the indirect format */
	LispFormat(mac, stream, &indirect_info);
    }
    else {
	/* next argument is the recursive call arguments */

	int num_arguments;

	/* it is valid to not have an list following string, as string may
	 * not have format directives */
	if (CONS_P(*(indirect_info.arguments)))
	    object = CAR(*(indirect_info.arguments));
	else
	    object = NIL;

	if (object != NIL && !CONS_P(object))
	    generic_error(mac, &(info->args), GENERIC_BADLIST);

	/* update information now */
	*(info->object) = object;
	*(info->arguments) = CDR(*(info->arguments));
	*(info->num_arguments) -= 1;

	/* set arguments for recursive call */
	indirect_info.base_arguments = object;
	indirect_info.arguments = &object;
	for (num_arguments = 0; CONS_P(object); object = CDR(object))
	    ++num_arguments;

	/* note that indirect_info.arguments is a pointer to "object",
	 * keep it pointing to the correct object */
	object = indirect_info.base_arguments;
	indirect_info.total_arguments = num_arguments;
	indirect_info.num_arguments = &num_arguments;

	/* do the indirect format */
	LispFormat(mac, stream, &indirect_info);
    }
}

/* update pointers to a list of format strings:
 *	for '(' and '{' only one list is required
 *	for '[' and '<' more than one may be returned
 *	has_default is only meaningful for '[' and '<'
 *	comma_width and line_width are only meaningful to '<', and
 *	    only valid if has_default set
 * if the string is finished prematurely, LispDestroy is called
 * format_ptr is updated to the correct pointer in the "main" format string
 */
static void
list_formats(LispMac *mac, FmtInfo *info, int command, char **format_ptr,
	     char ***format_list, int *format_count, int *has_default,
	     int *comma_width, int *line_width)
{
    /* instead of processing the directives recursively, just separate the
     * input formats in separate strings, then see if one of then need to
     * be used */
    FmtArgs args;
    int counters[] = {  0,   0,   0,   0};
		    /* '[', '(', '{', '<' */
    char *format, *next_format, *start, **formats;
    int num_formats, format_index, separator, add_format;

    /* initialize */
    formats = NULL;
    num_formats = format_index = 0;
    if (has_default != NULL)
	*has_default = 0;
    if (comma_width != NULL)
	*comma_width = 0;
    if (line_width != NULL)
	*line_width = 0;
    format = start = next_format = *format_ptr;
    switch (command) {
	case '[': counters[0] = 1; format_index = 0; break;
	case '(': counters[1] = 1; format_index = 1; break;
	case '{': counters[2] = 1; format_index = 2; break;
	case '<': counters[3] = 1; format_index = 3; break;
    }

#define	LIST_FORMATS_ADD	1
#define	LIST_FORMATS_DONE	2

    /* fill list of format options to conditional */
    while (*format) {
	if (*format == '~') {
	    separator = add_format = 0;
	    args.format = format + 1;
	    next_format = parse_arguments(format + 1, &args, NULL, NULL, NULL);
	    switch (args.command) {
 		case '[': ++counters[0];    break;
		case ']': --counters[0];    break;
		case '(': ++counters[1];    break;
		case ')': --counters[1];    break;
		case '{': ++counters[2];    break;
		case '}': --counters[2];    break;
		case '<': ++counters[3];    break;
		case '>': --counters[3];    break;
		case ';': separator = 1;    break;
	    }

	    /* check if a new format string must be added */
	    if (separator && counters[format_index] == 1 &&
		(command == '[' || command == '<'))
		add_format = LIST_FORMATS_ADD;
	    else if (counters[format_index] == 0)
		add_format = LIST_FORMATS_DONE;

	    if (add_format) {
		int length = format - start;

		formats = LispRealloc(mac, formats,
				      (num_formats + 1) * sizeof(char*));

		formats[num_formats] = LispMalloc(mac, length + 1);
		strncpy(formats[num_formats], start, length);
		formats[num_formats][length] = '\0';
		++num_formats;
		/* loop finished? */
		if (add_format == LIST_FORMATS_DONE)
		    break;
		else if (command == '[' && has_default != NULL)
		    /* will be set only for the last parameter, what is
		     * expected, just don't warn about it in the incorrect
		     * place */
		    *has_default = args.collon != 0;
		else if (command == '<' && num_formats == 1) {
		    /* if the first parameter to '<', there may be overrides
		     * to comma-width and line-width */
		    if (args.collon && has_default != NULL) {
			*has_default = 1;
			if (comma_width != NULL &&
			    args.arguments[0].specified &&
			    !args.arguments[0].achar)
			    *comma_width = args.arguments[0].value;
			if (line_width != NULL &&
			    args.arguments[1].specified &&
			    !args.arguments[1].achar)
			    *line_width = args.arguments[1].value;
		    }
		}
		start = next_format;
	    }
	    format = next_format;
	}
	else
	    ++format;
    }

    /* check if format string did not finish prematurely */
    if (counters[format_index] != 0) {
	char error_message[64];

	sprintf(error_message, "expecting ~%c", command);
	format_error(mac, &(info->args), error_message);
    }

    /* update pointers */
    *format_list = formats;
    *format_count = num_formats;
    *format_ptr = next_format;
}

static void
free_formats(LispMac *mac, char **formats, int num_formats)
{
    if (num_formats) {
	while (--num_formats >= 0)
	    LispFree(mac, formats[num_formats]);
	LispFree(mac, formats);
    }
}

static void
format_case_conversion(LispMac *mac, LispObj *stream, FmtInfo *info)
{
    LispObj *string;
    FmtInfo case_info;
    unsigned char *str, *ptr;
    char *format, *next_format, **formats;
    int atsign, collon, num_formats, protect = mac->protect.length;

    atsign = info->args.atsign;
    collon = info->args.collon;

    if (protect + 1 >= mac->protect.space)
	LispMoreProtects(mac);

    /* output to a string, before case conversion */
    string = STRINGSTREAM((unsigned char*)"", STREAM_READ | STREAM_WRITE);
    mac->protect.objects[mac->protect.length++] = string;

    /* most information is the same */
    memcpy(&case_info, info, sizeof(FmtInfo));

    /* list formats */
    next_format = *(info->format);
    list_formats(mac, info, '(', &next_format, &formats, &num_formats,
		 NULL, NULL, NULL);

    /* set new format string */
    format = formats[0];
    case_info.args.base = case_info.args.format = format;
    case_info.format = &format;

    /* format text to string */
    LispFormat(mac, string, &case_info);

    str = ptr = (unsigned char*)LispGetSstring(SSTREAMP(string));

    /* do case conversion */
    if (!atsign && !collon) {
	/* convert all upercase to lowercase */
	for (; *ptr; ptr++) {
	    if (isupper(*ptr))
		*ptr = tolower(*ptr);
	}
    }
    else if (atsign && collon) {
	/* convert all lowercase to upercase */
	for (; *ptr; ptr++) {
	    if (islower(*ptr))
		*ptr = toupper(*ptr);
	}
    }
    else {
	int upper = 1;

	/* skip non-alphanumeric characters */
	for (; *ptr; ptr++)
	    if (isalnum(*ptr))
		break;

	/* capitalize words */
	for (; *ptr; ptr++) {
	    if (isalnum(*ptr)) {
		if (upper) {
		    if (islower(*ptr))
			*ptr = toupper(*ptr);
		    upper = 0;
		}
		else if (isupper(*ptr))
		    *ptr = tolower(*ptr);
	    }
	    else
		upper = collon;
		/* if collon, capitalize all words, else just first word */
	}
    }

    /* output case converted string */
    LispWriteStr(mac, stream, (char*)str);

    /* temporary string stream is not necessary anymore */
    mac->protect.length = protect;

    /* free temporary memory */
    free_formats(mac, formats, num_formats);

    /* this information always updated */
    *(info->format) = next_format;
}

static void
format_conditional(LispMac *mac, LispObj *stream, FmtInfo *info)
{
    LispObj *object, *arguments;
    char *format, *next_format, **formats;
    int choice, num_formats, has_default, num_arguments;

    /* save information that may change */
    object = *(info->object);
    arguments = *(info->arguments);
    num_arguments = *(info->num_arguments);

    /* initialize */
    choice = -1;
    next_format = *(info->format);

    /* list formats */
    list_formats(mac, info, '[',
		 &next_format, &formats, &num_formats, &has_default, NULL, NULL);

    /* ~:[false;true] */
    if (info->args.collon) {
	/* one argument always consumed */
	if (!CONS_P(arguments))
	    parse_error(mac, &(info->args), PARSE_NOARGSLEFT);
	object = CAR(arguments);
	arguments = CDR(arguments);
	--num_arguments;
	choice = object == NIL ? 0 : 1;
    }
    /* ~@[true] */
    else if (info->args.atsign) {
	/* argument consumed only if nil, but one must be available */
	if (!CONS_P(arguments))
	    parse_error(mac, &(info->args), PARSE_NOARGSLEFT);
	if (CAR(arguments) != NIL)
	    choice = 0;
	else {
	    object = CAR(arguments);
	    arguments = CDR(arguments);
	    --num_arguments;
	}
    }
    /* ~n[...~] */
    else if (info->args.arguments[0].specified)
	/* no arguments consumed */
	choice = info->args.arguments[0].value;
    /* ~[...~] */
    else {
	/* one argument consumed, it is the index in the available formats */
	if (!CONS_P(arguments))
	    parse_error(mac, &(info->args), PARSE_NOARGSLEFT);
	object = CAR(arguments);
	arguments = CDR(arguments);
	--num_arguments;
	/* */
	if (INT_P(object))
	    choice = object->data.integer;
	/* if choice is out of range check if there is a default choice */
	if (has_default && (choice < 0 || choice >= num_formats))
	    choice = num_formats - 1;
    }

    /* update anything that may have changed */
    *(info->object) = object;
    *(info->arguments) = arguments;
    *(info->num_arguments) = num_arguments;

    /* if one of the formats must be parsed */
    if (choice >= 0 && choice < num_formats) {
	FmtInfo conditional_info;

	/* most information is the same */
	memcpy(&conditional_info, info, sizeof(FmtInfo));

	/* set new format string */
	format = formats[choice];
	conditional_info.args.base = conditional_info.args.format = format;
	conditional_info.format = &format;

	/* do the conditional format */
	LispFormat(mac, stream, &conditional_info);
    }

    /* free temporary memory */
    free_formats(mac, formats, num_formats);

    /* this information always updated */
    *(info->format) = next_format;
}

static void
format_iterate(LispMac *mac, LispObj *stream, FmtInfo *info)
{
    FmtInfo iterate_info;
    LispObj *object, *arguments, *iarguments, *iobject;
    char *format, *next_format, *loop_format, **formats;
    int num_arguments, iterate, iterate_max, has_max, has_min, inum_arguments,
	num_formats;

    /* save information that may change */
    object = *(info->object);
    arguments = *(info->arguments);
    num_arguments = *(info->num_arguments);

    /* initialize */
    iterate = has_min = 0;
    next_format = *(info->format);

    /* if has_max set, iterate at most iterate_max times */
    has_max = info->args.arguments[0].specified;
    iterate_max = info->args.arguments[0].value;

    /* list formats */
    list_formats(mac, info, '{', &next_format, &formats, &num_formats,
		 NULL, NULL, NULL);
    loop_format = formats[0];

    /* most information is the same */
    memcpy(&iterate_info, info, sizeof(FmtInfo));

    /* ~{...~} */
    if (!info->args.atsign && !info->args.collon) {
	/* next argument is the argument list for the iteration */

	/* fetch argument list, must exist */
	if (!CONS_P(arguments))
	    parse_error(mac, &(info->args), PARSE_NOARGSLEFT);
	iarguments = object = CAR(arguments);
	object = CAR(arguments);
	arguments = CDR(arguments);
	--num_arguments;

	inum_arguments = 0;
	if (CONS_P(object)) {
	    /* count arguments to format */
	    for (iobject = object; CONS_P(iobject); iobject = CDR(iobject))
		++inum_arguments;
	}
	else if (object != NIL)
	    generic_error(mac, &(info->args), GENERIC_BADLIST);

	iobject = NIL;

	/* set new arguments to recursive calls */
	iarguments = object;
	iterate_info.base_arguments = iarguments;
	iterate_info.total_arguments = inum_arguments;
	iterate_info.object = &iobject;
	iterate_info.arguments = &iarguments;
	iterate_info.num_arguments = &inum_arguments;

	/* iterate */
	for (;; iterate++) {
	    /* if maximum iterations done or all arguments consumed */
	    if (has_max && iterate > iterate_max)
		break;
	    else if (inum_arguments == 0 && (!has_min || iterate > 0))
		break;

	    format = loop_format;

	    /* set new format string */
	    iterate_info.args.base = iterate_info.args.format = format;
	    iterate_info.format = &format;

	    /* information for possible ~^, in this case ~:^ is a noop */
	    iterate_info.iteration = ITERATION_NORMAL;

	    /* do the format */
	    LispFormat(mac, stream, &iterate_info);

	    /* check for forced loop break */
	    if (iterate_info.upandout & UPANDOUT_HASH)
		break;
	}
    }
    /* ~:@{...~} */
    else if (info->args.atsign && info->args.collon) {
	/* every following argument is the argument list for the iteration */

	/* iterate */
	for (;; iterate++) {
	    /* if maximum iterations done or all arguments consumed */
	    if (has_max && iterate > iterate_max)
		break;
	    else if (num_arguments == 0 && (!has_min || iterate > 0))
		break;

	    /* fetch argument list, must exist */
	    if (!CONS_P(arguments))
		parse_error(mac, &(info->args), PARSE_NOARGSLEFT);
	    iarguments = object = CAR(arguments);
	    object = CAR(arguments);
	    arguments = CDR(arguments);
	    --num_arguments;

	    inum_arguments = 0;
	    if (CONS_P(object)) {
		/* count arguments to format */
		for (iobject = object; CONS_P(iobject); iobject = CDR(iobject))
		    ++inum_arguments;
	    }
	    else if (object != NIL)
		generic_error(mac, &(info->args), GENERIC_BADLIST);

	    iobject = NIL;

	    /* set new arguments to recursive calls */
	    iarguments = object;
	    iterate_info.base_arguments = iarguments;
	    iterate_info.total_arguments = inum_arguments;
	    iterate_info.object = &iobject;
	    iterate_info.arguments = &iarguments;
	    iterate_info.num_arguments = &inum_arguments;

	    format = loop_format;

	    /* set new format string */
	    iterate_info.args.base = iterate_info.args.format = format;
	    iterate_info.format = &format;

	    /* information for possible ~^ */
	    iterate_info.iteration =
		num_arguments > 0 ? ITERATION_NORMAL : ITERATION_LAST;

	    /* do the format */
	    LispFormat(mac, stream, &iterate_info);

	    /* check for forced loop break */
	    if (iterate_info.upandout & UPANDOUT_HASH)
		break;
	}
    }
    /* ~:{...~} */
    else if (info->args.collon) {
	/* next argument is a list of lists */

	LispObj *sarguments, *sobject;
	int snum_arguments;

	/* fetch argument list, must exist */
	if (!CONS_P(arguments))
	    parse_error(mac, &(info->args), PARSE_NOARGSLEFT);
	sarguments = object = CAR(arguments);
	object = CAR(arguments);
	arguments = CDR(arguments);
	--num_arguments;

	snum_arguments = 0;
	if (CONS_P(object)) {
	    /* count arguments to format */
	    for (sobject = object; CONS_P(sobject); sobject = CDR(sobject))
		++snum_arguments;
	}
	else
	    generic_error(mac, &(info->args), GENERIC_BADLIST);

	/* iterate */
	for (;; iterate++) {
	    /* if maximum iterations done or all arguments consumed */
	    if (has_max && iterate > iterate_max)
		break;
	    else if (snum_arguments == 0 && (!has_min || iterate > 0))
		break;

	    /* fetch argument list, must exist */
	    if (!CONS_P(sarguments))
		parse_error(mac, &(info->args), PARSE_NOARGSLEFT);
	    iarguments = sobject = CAR(sarguments);
	    sobject = CAR(sarguments);
	    sarguments = CDR(sarguments);
	    --snum_arguments;

	    inum_arguments = 0;
	    if (CONS_P(object)) {
		/* count arguments to format */
		for (iobject = sobject; CONS_P(iobject); iobject = CDR(iobject))
		    ++inum_arguments;
	    }
	    else if (sobject != NIL)
		generic_error(mac, &(info->args), GENERIC_BADLIST);

	    iobject = NIL;

	    /* set new arguments to recursive calls */
	    iarguments = sobject;
	    iterate_info.base_arguments = iarguments;
	    iterate_info.total_arguments = inum_arguments;
	    iterate_info.object = &iobject;
	    iterate_info.arguments = &iarguments;
	    iterate_info.num_arguments = &inum_arguments;

	    format = loop_format;

	    /* set new format string */
	    iterate_info.args.base = iterate_info.args.format = format;
	    iterate_info.format = &format;

	    /* information for possible ~^ */
	    iterate_info.iteration =
		snum_arguments > 0 ? ITERATION_NORMAL : ITERATION_LAST;

	    /* do the format */
	    LispFormat(mac, stream, &iterate_info);

	    /* check for forced loop break */
	    if (iterate_info.upandout & UPANDOUT_HASH)
		break;
	}
    }
    /* ~@{...~} */
    else if (info->args.atsign) {
	/* current argument list is used */

	/* set new arguments to recursive calls */
	iterate_info.base_arguments = info->base_arguments;
	iterate_info.total_arguments = info->total_arguments;
	iterate_info.object = &object;
	iterate_info.arguments = &arguments;
	iterate_info.num_arguments = &num_arguments;

	for (;; iterate++) {
	    /* if maximum iterations done or all arguments consumed */
	    if (has_max && iterate > iterate_max)
		break;
	    else if (num_arguments == 0 && (!has_min || iterate > 0))
		break;

	    format = loop_format;

	    /* set new format string */
	    iterate_info.args.base = iterate_info.args.format = format;
	    iterate_info.format = &format;

	    /* information for possible ~^, in this case ~:^ is a noop */
	    iterate_info.iteration = ITERATION_NORMAL;

	    /* do the format */
	    LispFormat(mac, stream, &iterate_info);

	    /* check for forced loop break */
	    if (iterate_info.upandout & UPANDOUT_HASH)
		break;
	}
    }

    /* free temporary memory */
    free_formats(mac, formats, num_formats);

    /* update anything that may have changed */
    *(info->object) = object;
    *(info->arguments) = arguments;
    *(info->num_arguments) = num_arguments;

    /* this information always updated */
    *(info->format) = next_format;
}

static void
format_justify(LispMac *mac, LispObj *stream, FmtInfo *info)
{
    FmtInfo justify_info;
    char **formats, *format, *next_format;
    LispObj *string, *strings = NIL, *cons;
    int atsign = info->args.atsign,
	collon = info->args.collon,
	mincol = info->args.arguments[0].value,
	colinc = info->args.arguments[1].value,
	minpad = info->args.arguments[2].value,
	padchar = info->args.arguments[3].value;
    int i, k, total_length, length, padding, num_formats, has_default,
	comma_width, line_width, protect = mac->protect.length;

    next_format = *(info->format);

    /* list formats */
    list_formats(mac, info, '<', &next_format, &formats, &num_formats,
		 &has_default, &comma_width, &line_width);

    if (protect + 1 <= mac->protect.space)
	LispMoreProtects(mac);

    /* initialize list of strings streams */
    if (num_formats) {
	string = STRINGSTREAM((unsigned char*)"", STREAM_READ | STREAM_WRITE);
	strings = cons = CONS(string, NIL);
	mac->protect.objects[mac->protect.length++] = strings;
	for (i = 1; i < num_formats; i++) {
	    string = STRINGSTREAM((unsigned char*)"", STREAM_READ | STREAM_WRITE);
	    CDR(cons) = CONS(string, NIL);
	    cons = CDR(cons);
	}
    }

    /* most information is the same */
    memcpy(&justify_info, info, sizeof(FmtInfo));

    /* loop formating strings */
    for (i = 0, cons = strings; i < num_formats; i++, cons = CDR(cons)) {
	/* set new format string */
	format = formats[i];
	justify_info.args.base = justify_info.args.format = format;
	justify_info.format = &format;

	/* format string, maybe consuming arguments */
	LispFormat(mac, CAR(cons), &justify_info);

	/* if format was aborted, it is discarded */
	if (justify_info.upandout)
	    CAR(cons) = NIL;
	/* check if the entire "main" iteration must be aborted */
	if (justify_info.upandout & UPANDOUT_COLLON) {
	    for (cons = CDR(cons); i < num_formats; i++, cons = CDR(cons))
		CAR(cons) = NIL;
	    break;
	}
    }

    /* free temporary format strings */
    free_formats(mac, formats, num_formats);

    /* remove aborted formats */
	/* first remove leading discarded formats */
    if (CAR(strings) == NIL) {
	while (CAR(strings) == NIL) {
	    strings = CDR(strings);
	    --num_formats;
	}
	/* keep strings gc protected, discarding first entries */
	mac->protect.objects[protect] = strings;
    }
	/* now remove intermediary discarded formats */
    cons = strings;
    while (CONS_P(cons)) {
	if (CONS_P(CDR(cons)) && CAR(CDR(cons)) == NIL) {
	    CDR(cons) = CDR(CDR(cons));
	    --num_formats;
	}
	else
	    cons = CDR(cons);
    }

    /* calculate total length required for output */
    if (has_default)
	cons = CDR(strings);	/* if has_defaults, strings is surely a list */
    else
	cons = strings;
    for (total_length = 0; CONS_P(cons); cons = CDR(cons))
	total_length += SSTREAMP(CAR(cons))->length;

    /* initialize pointer to string streams */
    if (has_default)
	cons = CDR(strings);
    else
	cons = strings;

    /* check if padding will need to be printed */
    padding = mincol - total_length;
    if (padding < 0)
	k = padding = 0;
    else {
	int num_fields = num_formats - (has_default != 0);

	if (num_fields > 1) {
	    /* check if padding is distributed in num_fields or
	     * num_fields - 1 steps */
	    if ((!atsign && !collon) || (atsign ^ collon))
		--num_fields;
	}

	if (num_fields)
	    k = padding / num_fields;
	else
	    k = padding;

	if (k <= 0)
	    k = colinc;
	else if (colinc)
	    k = k + (k % colinc);
    }
    if (padding && k < minpad) {
	k = minpad;
	if (colinc)
	    k = k + (k % colinc);
    }

    /* first check for the special case of only one string being justified */
    if (num_formats - has_default == 1) {
	if (has_default && line_width > 0 && comma_width >= 0 &&
	    total_length + comma_width > line_width)
	    LispWriteStr(mac, stream, LispGetSstring(SSTREAMP(CAR(strings))));
	string = has_default ? CAR(CDR(strings)) : CAR(strings);
	/* check if need left padding */
	if (k && !atsign) {
	    LispWriteChars(mac, stream, padchar, k);
	    k = 0;
	}
	/* check for centralizing text */
	else if (k && atsign && collon) {
	    LispWriteChars(mac, stream, padchar, k / 2);
	    k -= k / 2;
	}
	LispWriteStr(mac, stream, LispGetSstring(SSTREAMP(string)));
	/* if any padding remaining */
	if (k)
	    LispWriteChars(mac, stream, padchar, k);
    }
    else {
	LispObj *result;
	int last, spaces_before, padout;

	/* if has default, need to check output length */
	if (has_default && line_width > 0 && comma_width >= 0) {
	    if (mac->protect.length + 1 <= mac->protect.space)
		LispMoreProtects(mac);

	    result = STRINGSTREAM((unsigned char*)"", STREAM_READ | STREAM_WRITE);
	    mac->protect.objects[mac->protect.length++] = result;
	}
	/* else write directly to stream */
	else
	    result = stream;

	/* loop printing justified text */
	    /* padout controls padding for cases where padding is
	     * is separated in n-1 chunks, where n is the number of
	     * formatted strings.
	     */
	for (i = padout = 0; CONS_P(cons); i++, cons = CDR(cons)) {
	    string = CAR(cons);
	    last = !CONS_P(CDR(cons));

	    spaces_before = (i != 0 || collon) && (!last || !atsign);

	    if (!spaces_before) {
		/* check for special case */
		if (last && atsign && collon && padding > 0) {
		    int spaces;

		    spaces = minpad > colinc ? minpad : colinc;
		    LispWriteChars(mac, result, padchar, spaces);
		    k -= spaces;
		}
		LispWriteStr(mac, result, LispGetSstring(SSTREAMP(string)));
		padout = 0;
	    }
	    if (!padout)
		LispWriteChars(mac, result, padchar, k);
	    padout = k;
	    /* if not first string, or if left padding specified */
	    if (spaces_before) {
		LispWriteStr(mac, result, LispGetSstring(SSTREAMP(string)));
		padout = 0;
	    }
	    padding -= k;	
	}

	if (has_default && line_width > 0 && comma_width >= 0) {
	    length = SSTREAMP(result)->length + LispGetColumn(mac, stream);

	    /* if current line is too large */
	    if (has_default && length + comma_width > line_width)
		LispWriteStr(mac, stream, LispGetSstring(SSTREAMP(CAR(strings))));

	    /* write result to stream */
	    LispWriteStr(mac, stream, LispGetSstring(SSTREAMP(result)));
	}
    }

    /* unprotect string streams from GC */
    mac->protect.length = protect;

    /* this information always updated */
    *(info->format) = next_format;
}

static void
LispFormat(LispMac *mac, LispObj *stream, FmtInfo *info)
{
    FmtArgs *args;
    FmtDefs *defs = NULL;
    LispObj *object, *arguments;
    char stk[256], *format, *next_format;
    int length, num_arguments, code, need_update, need_argument, hash, escape;

    /* arguments that will be updated on function exit */
    format = *(info->format);
    object = *(info->object);
    arguments = *(info->arguments);
    num_arguments = *(info->num_arguments);

    /* initialize */
    length = 0;
    args = &(info->args);
    info->upandout = 0;

    while (*format) {
	if (*format == '~') {
	    /* flush non formatted characters */
	    if (length) {
		stk[length] = '\0';
		LispWriteStr(mac, stream, stk);
		length = 0;
	    }

	    need_argument = need_update = hash = 0;

	    /* parse parameters */
	    args->format = format + 1;
	    next_format = parse_arguments(format + 1, args, &num_arguments,
					  &arguments, &code);
	    if (code != NOERROR)
		parse_error(mac, args, code);

	    /* check parameters */
	    switch (args->command) {
		case 'A': case 'S':
		    defs = &AsciiDefs;
		    break;
		case 'B': case 'O': case 'D': case 'X':
		    defs = &IntegerDefs;
		    break;
		case 'R':
		    defs = &RadixDefs;
		    break;
		case 'P': case 'C':
		    defs = &NoneDefs;
		    break;
		case 'F':
		    defs = &FixedFloatDefs;
		    break;
		case 'E': case 'G':
		    defs = &ExponentialFloatDefs;
		    break;
		case '$':
		    defs = &DollarFloatDefs;
		    break;
		case '%': case '&': case '|': case '~': case '\n':
		    defs = &OneDefs;
		    break;
		case 'T':
		    defs = &TabulateDefs;
		    break;
		case '*':
		    defs = &OneDefs;
		    break;
		case '?': case '(':
		    defs = &NoneDefs;
		    break;
		case ')':
		    /* this is never seen, processed in format_case_conversion */
		    format_error(mac, args, "no match for directive ~)");
		case '[':
		    defs = &OneDefs;
		    break;
		case ']':
		    /* this is never seen, processed in format_conditional */
		    format_error(mac, args, "no match for directive ~]");
		case '{':
		    defs = &OneDefs;
		    break;
		case '}':
		    /* this is never seen, processed in format_iterate */
		    format_error(mac, args, "no match for directive ~}");
		case '<':
		    defs = &AsciiDefs;
		    break;
		case '>':
		    /* this is never seen, processed in format_justify */
		    format_error(mac, args, "no match for directive ~>");
		case ';':
		    /* this is never seen here */
		    format_error(mac, args, "misplaced directive ~;");
		case '#':
		    /* special handling for ~#^ */
		    if (*next_format == '^') {
			++next_format;
			hash = 1;
			defs = &NoneDefs;
			args->command = '^';
			break;
		    }
		    parse_error(mac, args, PARSE_BADDIRECTIVE);
		case '^':
		    defs = &NoneDefs;
		    break;
		default:
		    parse_error(mac, args, PARSE_BADDIRECTIVE);
		    break;
	    }
	    merge_arguments(args, defs, &code);
	    if (code != NOERROR)
		merge_error(mac, args, code);

	    /* check if an argument is required by directive */
	    switch (args->command) {
		case 'A': case 'S':
		case 'B': case 'O': case 'D': case 'X': case 'R':
		    need_argument = 1;
		    break;
		case 'P':
		    /* if collon specified, plural is the last print argument */
		    need_argument = !args->collon;
		    break;
		case 'C':
		    need_argument = 1;
		    break;
		case 'F': case 'E': case 'G': case '$':
		    need_argument = 1;
		    break;
		case '%': case '&': case '|': case '~': case '\n':
		    break;
		case 'T':
		    break;
		case '*':			/* check arguments below */
		    need_update = 1;
		    break;
		case '?':
		    need_argument = need_update = 1;
		    break;
		case '(': case '[': case '{': case '<':
		    need_update = 1;
		    break;
		case '^':
		    break;
	    }
	    if (need_argument) {
		if (!CONS_P(arguments))
		    parse_error(mac, args, PARSE_NOARGSLEFT);
		object = CAR(arguments);
		arguments = CDR(arguments);
		--num_arguments;
	    }

	    /* will do recursive calls that change info */
	    if (need_update) {
		*(info->format) = next_format;
		*(info->object) = object;
		*(info->arguments) = arguments;
		*(info->num_arguments) = num_arguments;
	    }

	    /* everything seens fine, print the format directive */
	    switch (args->command) {
		case 'A':
		    format_ascii(mac, stream, object, args);
		    break;
		case 'S':
		    escape = LispGetEscape(mac, stream);
		    LispSetEscape(mac, stream, 1);
		    format_ascii(mac, stream, object, args);
		    LispSetEscape(mac, stream, escape);
		    break;
		case 'B':
		    format_in_radix(mac, stream, object, 2, args);
		    break;
		case 'O':
		    format_in_radix(mac, stream, object, 8, args);
		    break;
		case 'D':
		    format_in_radix(mac, stream, object, 10, args);
		    break;
		case 'X':
		    format_in_radix(mac, stream, object, 16, args);
		    break;
		case 'R':
		    /* if a single argument specified */
		    if (args->count)
			format_in_radix(mac, stream, object, 0, args);
		    else
			format_radix_special(mac, stream, object, args);
		    break;
		case 'P':
		    if (args->atsign) {
			if (INT_P(object) && object->data.integer == 1)
			    LispWriteChar(mac, stream, 'y');
			else
			    LispWriteStr(mac, stream, "ies");
		    }
		    else if (!INT_P(object) || object->data.integer != 1)
			LispWriteChar(mac, stream, 's');
		    break;
		case 'C':
		    format_character(mac, stream, object, args);
		    break;
		case 'F':
		    format_fixed_float(mac, stream, object, args);
		    break;
		case 'E':
		    format_exponential_float(mac, stream, object, args);
		    break;
		case 'G':
		    format_general_float(mac, stream, object, args);
		    break;
		case '$':
		    format_dollar_float(mac, stream, object, args);
		    break;
		case '&':
		    if (LispGetColumn(mac, stream) == 0)
			--args->arguments[0].value;
		case '%':
		    LispWriteChars(mac, stream, '\n', args->arguments[0].value);
		    break;
		case '|':
		    LispWriteChars(mac, stream, '\f', args->arguments[0].value);
		    break;
		case '~':
		    LispWriteChars(mac, stream, '~', args->arguments[0].value);
		    break;
		case '\n':
		    if (!args->collon) {
			if (args->atsign)
			    LispWriteChar(mac, stream, '\n');
			/* ignore newline and following spaces */
			while (*next_format && isspace(*next_format))
			    ++next_format;
		    }
		    break;
		case 'T':
		    format_tabulate(mac, stream, args);
		    break;
		case '*':
		    format_goto(mac, info);
		    break;
		case '?':
		    format_indirection(mac, stream, object, info);
		    need_update = 1;
		    break;
		case '(':
		    format_case_conversion(mac, stream, info);
		    /* next_format if far from what is set now */
		    next_format = *(info->format);
		    break;
		case '[':
		    format_conditional(mac, stream, info);
		    /* next_format if far from what is set now */
		    next_format = *(info->format);
		    break;
		case '{':
		    format_iterate(mac, stream, info);
		    /* next_format if far from what is set now */
		    next_format = *(info->format);
		    break;
		case '<':
		    format_justify(mac, stream, info);
		    /* next_format if far from what is set now */
		    next_format = *(info->format);
		    break;
		case '^':
		    if (args->collon) {
			if (hash && num_arguments == 0) {
			    info->upandout = UPANDOUT_HASH;
			    goto format_up_and_out;
			}
			if (info->iteration &&
			    info->iteration == ITERATION_NORMAL)
			/* not exactly an error, but in this case,
			 * command is ignored */
			    break;
			info->upandout = UPANDOUT_COLLON;
			goto format_up_and_out;
		    }
		    else if (num_arguments == 0) {
			info->upandout = UPANDOUT_NORMAL;
			goto format_up_and_out;
		    }
		    break;
	    }

	    if (need_update) {
		object = *(info->object);
		arguments = *(info->arguments);
		num_arguments = *(info->num_arguments);
	    }

	    format = next_format;
	}
	else {
	    if (length + 1 >= sizeof(stk)) {
		stk[length] = '\0';
		LispWriteStr(mac, stream, stk);
		length = 0;
	    }
	    stk[length++] = *format++;
	}
    }

    /* flush any peding output */
    if (length) {
	stk[length] = '\0';
	LispWriteStr(mac, stream, stk);
    }

format_up_and_out:
    /* update for recursive call */
    *(info->format) = format;
    *(info->object) = object;
    *(info->arguments) = arguments;
    *(info->num_arguments) = num_arguments;
}

LispObj *
Lisp_Format(LispMac *mac, LispBuiltin *builtin)
/*
 format destination control-string &rest arguments
 */
{
    FmtInfo info;
    LispObj *object;
    char *control_string;
    int protect, num_arguments;

    LispObj *stream, *format, *arguments;

    arguments = ARGUMENT(2);
    format = ARGUMENT(1);
    stream = ARGUMENT(0);

    protect = mac->protect.length;

    /* check format and stream */
    if (!STRING_P(format))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(format));
    if (stream == NIL) {	/* return a string */
	stream = STRINGSTREAM((unsigned char*)"", STREAM_READ | STREAM_WRITE);
	if (protect + 1 >= mac->protect.space)
	    LispMoreProtects(mac);
	mac->protect.objects[mac->protect.length++] = stream;
    }
    else if (stream == T)	/* print directly to *standard-output* */
	stream = NIL;
    else if (!STREAM_P(stream))
	LispDestroy(mac, "%s: %s is not a stream",
		    STRFUN(builtin), STROBJ(stream));
    else if (!stream->data.stream.writable)
	LispDestroy(mac, "%s: stream %s is not writable",
		    STRFUN(builtin), STROBJ(stream));

    /* count number of arguments */
    for (object = arguments, num_arguments = 0; CONS_P(object);
	 object = CDR(object), num_arguments++)
	;

    /* initialize plural/argument info */
    object = NIL;

    /* the format string */
    control_string = STRPTR(format);

    /* arguments to recursive calls */
    info.args.base = control_string;
    info.base_arguments = arguments;
    info.total_arguments = num_arguments;
    info.format = &control_string;
    info.object = &object;
    info.arguments = &arguments;
    info.num_arguments = &num_arguments;
    info.iteration = 0;

    /* format arguments */
    LispFormat(mac, stream, &info);

    /* if printing to stdout */
    if (stream == NIL || (stream->data.stream.type == LispStreamStandard &&
	stream->data.stream.source.file == Stdout))
	LispFflush(Stdout);

    /* if printing to string-stream, return a string */
    if (stream != NIL && stream->data.stream.type == LispStreamString)
	stream = STRING(LispGetSstring(SSTREAMP(stream)));

    mac->protect.length = protect;

    return (stream);
}

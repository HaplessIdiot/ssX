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

/* $XFree86: xc/programs/xedit/lisp/read.c,v 1.8 2002/02/14 04:48:10 paulo Exp $ */

#include <errno.h>
#include "read.h"
#include "package.h"

/*
 * Protypes
 */
static int LispSkipWhiteSpace(LispMac*);
static LispObj *LispReadList(LispMac*);
static LispObj *LispReadQuote(LispMac*);
static LispObj *LispReadBackquote(LispMac*);
static LispObj *LispReadCommaquote(LispMac*);
static LispObj *LispReadObject(LispMac*);
static LispObj *LispParseAtom(LispMac*, char*, char*, int, int);
static LispObj *LispParseNumber(LispMac*, char*, int);
static int StringInRadix(char*, int);
static LispObj *LispReadVector(LispMac*);
static LispObj *LispReadMacro(LispMac*);
static LispObj *LispReadFunction(LispMac*);
static LispObj *LispReadRational(LispMac*, int);
static LispObj *LispReadCharacter(LispMac*);
static void LispSkipComment(LispMac*);
static LispObj *LispReadEval(LispMac*);
static LispObj *LispReadComplex(LispMac*);
static LispObj *LispReadPathname(LispMac*);
static LispObj *LispReadStruct(LispMac*);
static LispObj *LispReadMacroArg(LispMac*);
static LispObj *LispReadArray(LispMac*, long);
static LispObj *LispReadFeature(LispMac*, int);
static LispObj *LispEvalFeature(LispMac*, LispObj*);

/*
 * Initialization
 */
char *LispCharNames[] = {
    "Null",		"Soh",		"Stx",		"Etx",
    "Eot",		"Enq",		"Ack",		"Bell",
    "Backspace",	"Tab",		"Newline",	"Vt",
    "Page",		"Return",	"So",		"Si",
    "Dle",		"Dc1",		"Dc2",		"Dc3",
    "Dc4",		"Nak",		"Syn",		"Etb",
    "Can",		"Em",		"Sub",		"Escape",
    "Fs",		"Gs",		"Rs",		"Us",
    "Space"
};

Atom_id Sand, Sor, Snot;

static LispObj lispdot = {LispAtom_t};
LispObj *DOT = &lispdot;

/*
 * Implementation
 */
LispObj *
LispRead(LispMac *mac)
{
    LispObj *object, *code = COD;
    int ch = LispSkipWhiteSpace(mac);

    switch (ch) {
	case '(':
	    object = LispReadList(mac);
	    break;
	case ')':
	    return (EOLIST);
	case EOF:
	    return (NULL);
	case '\'':
	    object = LispReadQuote(mac);
	    break;
	case '`':
	    object = LispReadBackquote(mac);
	    break;
	case ',':
	    object = LispReadCommaquote(mac);
	    break;
	case '#':
	    object = LispReadMacro(mac);
	    break;
	default:
	    LispUnget(mac, ch);
	    object = LispReadObject(mac);
	    break;
    }

    /* keep data gc protected while recursing, when returning to the first
     * call, COD will point to a single copy of the form to be evaluated */
    if (code == NIL)
	COD = object;
    else
	COD = CONS(COD, object);

    return (object);
}

static LispObj *
LispReadMacro(LispMac *mac)
{
    int ch = LispGet(mac);

    switch (ch) {
	case '(':
	    return (LispReadVector(mac));
	case '\'':
	    return (LispReadFunction(mac));
	case 'b':
	case 'B':
	    return (LispReadRational(mac, 2));
	case 'o':
	case 'O':
	    return (LispReadRational(mac, 8));
	case 'x':
	case 'X':
	    return (LispReadRational(mac, 16));
	case '\\':
	    return (LispReadCharacter(mac));
	case '|':
	    LispSkipComment(mac);
	    return (LispRead(mac));
	case '.':	/* eval when compiling */
	case ',':	/* eval when loading */
	    return (LispReadEval(mac));
	case 'c':
	case 'C':
	    return (LispReadComplex(mac));
	case 'p':
	case 'P':
	    return (LispReadPathname(mac));
	case 's':
	case 'S':
	    return (LispReadStruct(mac));
	case '+':
	    return (LispReadFeature(mac, 1));
	case '-':
	    return (LispReadFeature(mac, 0));
	default:
	    if (isdigit(ch)) {
		LispUnget(mac, ch);
		return (LispReadMacroArg(mac));
	    }
	    if (!mac->discard)
		LispDestroy(mac, "READ: undefined dispatch macro character #%c", ch);
    }

    return (NIL);
}

static LispObj *
LispReadMacroArg(LispMac *mac)
{
    long integer;
    int ch;

    /* skip leading zeros */
    while (ch = LispGet(mac), ch != EOF && isdigit(ch) && ch == '0')
	;

    if (ch == EOF)
	LispDestroy(mac, "READ: unexpected end of input");

    /* if ch is not a number the argument was zero */
    if (isdigit(ch)) {
	char stk[32], *str;
	int len = 1;

	stk[0] = ch;
	for (;;) {
	    ch = LispGet(mac);
	    if (!isdigit(ch))
		break;
	    if (len + 1 >= sizeof(stk)) {
		if (mac->discard)
		    continue;
		LispDestroy(mac, "READ: number is not a FIXNUM");
	    }
	    stk[len++] = ch;
	}
	stk[len] = '\0';
	integer = strtol(stk, &str, 10);
	if (*str || errno == ERANGE) {
	    if (!mac->discard)
		LispDestroy(mac, "READ: number is not a FIXNUM");
	}
    }
    else
	integer = 0;

    switch (ch) {
	case 'a':
	case 'A':
	    if (integer == 1) {
		/* LispReadArray and LispReadList expect
		 * the '(' being already read  */
		if ((ch = LispSkipWhiteSpace(mac)) != '(') {
		    if (mac->discard)
			return (ch == EOF ? NULL : NIL);
		    LispDestroy(mac, "READ: bad array specification");
		}
		return (LispReadVector(mac));
	    }
	    return (LispReadArray(mac, integer));
	case 'r':
	case 'R':
	    return (LispReadRational(mac, integer));
	default:
	    if (!mac->discard)
		LispDestroy(mac, "READ: undefined dispatch macro character #%c", ch);
    }

    return (NIL);
}

static int
LispSkipWhiteSpace(LispMac *mac)
{
    int ch;

    for (;;) {
	while (ch = LispGet(mac), isspace(ch) && ch != EOF)
	    ;
	if (ch == ';') {
	    while (ch = LispGet(mac), ch != '\n' && ch != EOF)
		;
	    if (ch == EOF)
		return (EOF);
	}
	else
	    break;
    }

    return (ch);
}

/* any data in the format '(' FORM ')' is read here */
static LispObj *
LispReadList(LispMac *mac)
{
    LispObj *result, *cons, *object;
    int dot = 0, protect = mac->protect.length;

    /* check for () */
    object = LispRead(mac);
    if (object == EOLIST)
	return (NIL);

    if (object == DOT) {
	if (!mac->discard)
	    LispDestroy(mac, "READ: illegal start of dotted list");
	return (NIL);
    }

    result = cons = CONS(object, NIL);

    /* make sure GC will not release data being read */
    if (mac->protect.length + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = result;

    while ((object = LispRead(mac)) != EOLIST) {
	if (object == NULL)
	    LispDestroy(mac, "READ: unexpected end of input");
	if (object == DOT) {
	    /* this is a dotted list */
	    if (dot) {
		if (!mac->discard)
		    LispDestroy(mac, "READ: more than one . in list");
		mac->protect.length = protect;
		return (NIL);
	    }
	    dot = 1;
	}
	else {
	    if (dot) {
		/* only one object after a dot */
		if (++dot > 2) {
		    if (!mac->discard)
			LispDestroy(mac, "READ: more than one object after . in list");
		    mac->protect.length = protect;

		    return (NIL);
		}
		CDR(cons) = object;
	    }
	    else {
		CDR(cons) = CONS(object, NIL);
		cons = CDR(cons);
	    }
	}
    }

    /* this will happen if last list element was a dot */
    if (dot == 1) {
	if (!mac->discard)
	    LispDestroy(mac, "READ: illegal end of dotted list");
	mac->protect.length = protect;

	return (NIL);
    }

    mac->protect.length = protect;

    return (result);
}

static LispObj *
LispReadQuote(LispMac *mac)
{
    LispObj *quote = LispRead(mac);

    if (INVALID_P(quote)) {
	if (mac->discard)
	    return (NULL);
	LispDestroy(mac, "READ: illegal quoted object");
    }

    return (QUOTE(quote));
}

static LispObj *
LispReadBackquote(LispMac *mac)
{
    LispObj *backquote = LispRead(mac);

    if (INVALID_P(backquote)) {
	if (mac->discard)
	    return (NULL);
	LispDestroy(mac, "READ: illegal back-quoted object");
    }

    return (BACKQUOTE(backquote));
}

static LispObj *
LispReadCommaquote(LispMac *mac)
{
    LispObj *comma;
    int atlist = LispGet(mac);

    if (atlist == EOF)
	LispDestroy(mac, "READ: unexpected end of input");
    else if (atlist != '@' && atlist != '.')
	LispUnget(mac, atlist);

    comma = LispRead(mac);
    if (comma == DOT) {
	atlist = '@';
	comma = LispRead(mac);
    }
    if (INVALID_P(comma)) {
	if (mac->discard)
	    return (NULL);
	LispDestroy(mac, "READ: illegal comma-quoted object");
    }

    return (COMMA(comma, atlist == '@' || atlist == '.'));
}

/*
 * Read anything that is not readily identifiable by it's first character
 * and also put the code for reading atoms, numbers and strings together.
 */
static LispObj *
LispReadObject(LispMac *mac)
{
    LispObj *object;
    char stk[128], *string, *package, *symbol;
    int ch, length, backslash, size, quote, unreadable, collon;

    package = symbol = string = stk;
    size = sizeof(stk);
    backslash = quote = unreadable = collon = 0;
    length = 0;

    ch = LispGet(mac);
    if (ch == '"' || ch == '|')
	quote = ch;
    else if (ch == '\\')
	unreadable = backslash = 1;
    else if (ch == ':') {
	collon = 1;
	string[length++] = ch;
	symbol = string + 1;
    }
    else {
	if (islower(ch))
	    ch = toupper(ch);
	string[length++] = ch;
    }

    /* read remaining data */
    for (;;) {
	ch = LispGet(mac);

	if (ch == EOF) {
	    if (quote) {
		/* if quote, file ended with an open quoted object */
		if (string != stk)
		    LispFree(mac, string);
		return (NULL);
	    }
	    break;
	}

	if (ch == '\\') {
	    if (backslash)
		backslash = 0;
	    else if (quote != '|') {
		backslash = 1;
		if (!quote) {
		    unreadable = 1;
		    continue;
		}
		else
		    continue;
	    }
	}
	else if (backslash)
	    backslash = 0;
	else if (ch == quote)
	    break;
	else if (!quote && !backslash) {
	    if (islower(ch))
		ch = toupper(ch);
	    else if (isspace(ch))
		break;
	    /* don't include any of these characters in an atom name */
	    else if (ch == ')' || ch == '"' || ch == ';' || ch == '(' ||
		     ch == '\'' || ch == '`' || ch == '#' || ch == '|' ||
		     ch == ',') {
		LispUnget(mac, ch);
		break;
	    }
	    else if (ch == ':') {
		if (collon == 0 ||
		    (collon == 1 && symbol == string + length)) {
		    ++collon;
		    symbol = string + length + 1;
		}
		else
		    LispDestroy(mac, "READ: too many collons");
	    }
	}

	if (length + 2 >= size) {
	    if (string == stk) {
		size = 1024;
		string = LispMalloc(mac, size);
		strcpy(string, stk);
	    }
	    else {
		size += 1024;
		string = LispRealloc(mac, string, size);
	    }
	    symbol = string + (symbol - package);
	    package = string;
	}
	string[length++] = ch;
    }

    if (mac->discard) {
	if (string != stk)
	    LispFree(mac, string);

	return (ch == EOF ? NULL : NIL);
    }

    string[length] = '\0';

    if (quote == '"')
	object = STRING(string);

    else if (quote == '|' || (unreadable && !collon)) {
	/* Set unreadable field, this atom needs quoting to be read back */
	object = ATOM(string);
	object->data.atom->unreadable = 1;
    }

    else if (collon) {
	/* Package specified in object name */
	symbol[-1] = '\0';
	if (collon > 1)
	    symbol[-2] = '\0';
	object = LispParseAtom(mac, package, symbol,
			       collon == 2, unreadable || length == 0);
    }

    /* Check some common symbols */
    else if (length == 1 && string[0] == 'T')
	/* The T */
	object = T;

    else if (length == 1 && string[0] == '.')
	/* The dot */
	object = DOT;

    else if (length == 3 &&
	     string[0] == 'N' && string[1] == 'I' && string[2] == 'L')
	/* The NIL */
	object = NIL;

    else if (isdigit(string[0]) || string[0] == '.' ||
	     ((string[0] == '-' || string[0] == '+') && string[1]))
	/* Looks like a number */
	object = LispParseNumber(mac, string, 10);

    else
	/* A normal atom */
	object = ATOM(string);

    if (string != stk)
	LispFree(mac, string);

    return (object);
}

static LispObj *
LispParseAtom(LispMac *mac, char *package, char *symbol,
	      int intern, int unreadable)
{
    LispObj *object = NULL, *thepackage = NULL;
    LispPackage *pack = NULL;

    /* If package is empty, it is a keyword */
    if (package[0] == '\0') {
	thepackage = mac->keyword;
	pack = mac->key;
    }

    else {
	/* Else, search it in the package list */
	thepackage = LispFindPackageFromString(mac, package);

	if (thepackage == NIL)
	    LispDestroy(mac, "READ: the package %s is not available", package);

	pack = thepackage->data.package.package;
    }

    if (pack == mac->pack && intern) {
	/* Redundant package specification, since requesting a
	 * intern symbol, create it if does not exist */

	object = ATOM(symbol);
	if (unreadable)
	    object->data.atom->unreadable = 1;
    }

    else if (intern || pack == mac->key) {
	/* Symbol is created, or just fetched from the specified package */

	LispPackage *savepack;
	LispObj *savepackage = PACKAGE;

	/* Remember curent package */
	savepack = mac->pack;

	/* Temporarily set another package */
	mac->pack = pack;
	PACKAGE = thepackage;

	/* Get the object pointer */
	object = ATOM(symbol);
	if (unreadable)
	    object->data.atom->unreadable = 1;

	if (pack == mac->key)
	    /* All keywords are external symbols */
	    LispExportSymbol(mac, object);

	/* Restore current package */
	mac->pack = savepack;
	PACKAGE = savepackage;
    }

    else {
	/* Symbol must exist (and be extern) in the specified package */

	int i;
	LispAtom *atom;

	for (i = 0; i < STRTBLSZ && object == NULL; i++) {
	    atom = pack->atoms[i];
	    while (atom) {
		if (strcmp(atom->string, symbol) == 0) {
		    object = atom->object;
		    break;
		}

		atom = atom->next;
	    }
	}

	/* No object found */
	if (object == NULL || object->data.atom->ext == 0)
	    LispDestroy(mac, "READ: no extern symbol %s in package %s",
			symbol, package);
    }

    return (object);
}

static LispObj *
LispParseNumber(LispMac *mac, char *str, int radix)
{
    mpr *rop;
    mpi *iop;
    int len;
    double real;
    long integer;
    LispObj *number;
    char *ratio, *ptr;

    if (*str == '\0')
	return (NULL);

    ratio = strchr(str, '/');
    if (ratio) {
	/* check if looks like a correctly specified ratio */
	if (ratio[1] == '\0' || strchr(ratio + 1, '/') != NULL)
	    return (ATOM(str));

	/* ratio must point to an integer in radix base */
	*ratio++ = '\0';
    }
    else if (radix == 10) {
	int dot = 0;
	int type = 0;

	/* check if it is a floating point number */
	ptr = str;
	if (*ptr == '-' || *ptr == '+')
	    ++ptr;
	else if (*ptr == '.') {
	    dot = 1;
	    ++ptr;
	}
	while (*ptr) {
	    if (*ptr == '.') {
		if (dot)
		    return (ATOM(str));
		/* ignore it if last char is a dot */
		if (ptr[1] == '\0') {
		    *ptr = '\0';
		    break;
		}
		dot = 1;
	    }
	    else if (!isdigit(*ptr))
		break;
	    ++ptr;
	}

	switch (*ptr) {
	    case '\0':
		if (dot)		/* if dot, it is default float */
		    type = 'E';
		break;
	    case 'E': case 'S': case 'F': case 'D': case 'L':
		type = *ptr;
		*ptr = 'E';
		break;
	    default:
		return (ATOM(str));	/* syntax error */
	}

	/* if type set, it is not an integer specification */
	if (type) {
	    if (*ptr) {
		int itype = *ptr;
		char *ptype = ptr;

		++ptr;
		if (*ptr == '+' || *ptr == '-')
		    ++ptr;
		while (*ptr && isdigit(*ptr))
		    ++ptr;
		if (*ptr) {
		    *ptype = itype;

		    return (ATOM(str));
		}
	    }

	    real = strtod(str, NULL);
	    if (!finite(real))
		LispDestroy(mac, "READ: floating point overflow");

	    return (REAL(real));
	}
    }

    iop = NULL;			/* if ratio and set, numerator was too large */
    rop = NULL;			/* if ratio and set, denominator was too large */

    /* check if correctly specified in the given radix */

    len = strlen(str) - 1;
    if (!ratio && radix != 10 && str[len] == '.')
	str[len] = '\0';

    if (ratio || radix != 10) {
	if (!StringInRadix(str, radix))
	    return (ATOM(str));
	if (ratio && !StringInRadix(ratio, radix)) {
	    *ratio = '/';
	    return (ATOM(str));
	}
    }

    errno = 0;
    /* number is an integer or ratio */
    integer = strtol(str, NULL, radix);

    /* if does not fit in a long */
    if (errno == ERANGE &&
	((*str == '-' && integer == LONG_MIN) ||
	 (*str != '-' && integer == LONG_MAX))) {
	iop = LispMalloc(mac, sizeof(mpi));
	mpi_init(iop);
	mpi_setstr(iop, str, radix);
	if (ratio == NULL) {
	    number = BIGINTEGER(iop);
	    LispMused(mac, iop);

	    return (number);
	}
    }

    if (ratio) {
	long denominator;

	errno = 0;
	denominator = strtol(ratio, NULL, radix);
	if (denominator == 0)
	    LispDestroy(mac, "READ: divide by zero");

	/* if denominator won't fit in a long */
	if (denominator == LONG_MAX && errno == ERANGE) {
	    rop = LispMalloc(mac, sizeof(mpr));
	    mpr_init(rop);
	    if (iop) {
		mpi_set(mpr_num(rop), iop);
		mpi_clear(iop);
		LispFree(mac, iop);
		iop = NULL;
	    }
	    else		
		mpi_setstr(mpr_num(rop), str, radix);
	    mpi_setstr(mpr_den(rop), ratio, radix);
	}
	else if (iop) {
	    rop = LispMalloc(mac, sizeof(mpr));
	    mpr_init(rop);
	    mpi_set(mpr_num(rop), iop);
	    mpi_clear(iop);
	    LispFree(mac, iop);
	    iop = NULL;
	    mpi_seti(mpr_den(rop), denominator);
	}

	if (rop) {
	    mpr_canonicalize(rop);
	    if (mpr_fiti(rop)) {
		integer = mpi_geti(mpr_num(rop));
		denominator = mpi_geti(mpr_den(rop));
		mpr_clear(rop);
		LispFree(mac, rop);
		rop = NULL;
		/* if denominator becames 1, RATIO will convert to INTEGER */
		number = RATIO(integer, denominator);
	    }
	    else {
		number = BIGRATIO(rop);
		LispMused(mac, rop);
	    }

	    return (number);
	}

	return (RATIO(integer, denominator));
    }

    if (iop) {
	number = BIGINTEGER(iop);
	LispMused(mac, iop);
    }
    else
	number = INTEGER(integer);

    return (number);
}

static int
StringInRadix(char *str, int radix)
{
    while (*str) {
	if (*str >= '0' && *str <= '9') {
	    if (*str - '0' >= radix)
		return (0);
	}
	else if (*str >= 'A' && *str <= 'Z') {
	    if (radix <= 10 || *str - 'A' >= radix)
		return (0);
	}
	else
	    return (0);
	str++;
    }

    return (1);
}

static LispObj *
LispReadVector(LispMac *mac)
{
    LispObj *object;
    LispObj *objects;

    objects = LispReadList(mac);

    if (mac->discard)
	return (NULL);

    for (object = objects; CONS_P(object); object = CDR(object))
	;
    if (object != NIL)
	LispDestroy(mac, "READ: vector cannot be a dotted list");

    return (VECTOR(objects));
}

static LispObj *
LispReadFunction(LispMac *mac)
{
    LispObj *function = LispRead(mac);

    if (mac->discard)
	return (function);

    if (INVALID_P(function)) 
	LispDestroy(mac, "READ: illegal function object");
    else if (CONS_P(function)) {
	if (!SYMBOL_P(CAR(function)) ||
	    ATOMID(CAR(function)) != Slambda) {
	    LispDestroy(mac, "READ: %s is not a valid lambda", STROBJ(function));
	}

	return (EVAL(function));
    }
    else if (!SYMBOL_P(function))
	LispDestroy(mac, "READ: %s cannot name a function", STROBJ(function));

    return (QUOTE(function));
}

static LispObj *
LispReadRational(LispMac *mac, int radix)
{
    LispObj *number;
    int ch, len, size;
    char stk[128], *str;

    len = 0;
    str = stk;
    size = sizeof(stk);

    for (;;) {
	ch = LispGet(mac);
	if (ch == EOF || isspace(ch))
	    break;
	else if (islower(ch))
	    ch = toupper(ch);
	if ((ch < '0' || ch > '9') && (ch < 'A' || ch > 'Z') &&
	    ch != '+' && ch != '-' && ch != '/') {
	    if (str != stk)
		LispFree(mac, str);
	    if (!mac->discard)
		LispDestroy(mac, "READ: bad character %c for rational number", ch);
	}
	if (len + 1 >= size) {
	    if (str == stk) {
		size = 512;
		str = LispMalloc(mac, size);
		strcpy(str + 1, stk + 1);
	    }
	    else {
		size += 512;
		str = LispRealloc(mac, str, size);
	    }
	}
	str[len++] = ch;
    }

    if (mac->discard) {
	if (str != stk)
	    LispFree(mac, str);

	return (ch == EOF ? NULL : NIL);
    }

    str[len] = '\0';

    number = LispParseNumber(mac, str, radix);
    if (str != stk)
	LispFree(mac, str);

    if (!RATIONAL_P(number))
	LispDestroy(mac, "READ: bad rational number specification");

    return (number);
}

static LispObj *
LispReadCharacter(LispMac *mac)
{
    long c;
    int ch, len;
    char stk[64];

    ch = LispGet(mac);
    if (ch == EOF)
	return (NULL);

    stk[0] = ch;
    len = 1;

    for (;;) {
	ch = LispGet(mac);
	if (ch == EOF)
	    break;
	else if (ch != '-' && !isalnum(ch)) {
	    LispUnget(mac, ch);
	    break;
	}
	if (len + 1 < sizeof(stk))
	    stk[len++] = ch;
    }
    if (len > 1) {
	stk[len] = '\0';

	for (c = 0; c <= ' '; c++) {
	    if (strcasecmp(LispCharNames[c], stk) == 0)
		break;
	}
	if (c > ' ') {
	    /* extra or special cases */
	    if (strcasecmp(stk, "Rubout") == 0)
		c = 0177;
	    else if (strcasecmp(stk, "Nul") == 0)
		c = 0;
	    else if (strcasecmp(stk, "Bel") == 0)
		c = 007;
	    else if (strcasecmp(stk, "Bs") == 0)
		c = 010;
	    else if (strcasecmp(stk, "Ht") == 0)
		c = 011;
	    else if (strcasecmp(stk, "Lf") == 0)
		c = 012;
	    else if (strcasecmp(stk, "Ff") == 0)
		c = 014;
	    else if (strcasecmp(stk, "Cr") == 0)
		c = 015;
	    else if (strcasecmp(stk, "Esc") == 0)
		c = 033;
	    else if (strcasecmp(stk, "Del") == 0)
		c = 0177;
	    else if (strcasecmp(stk, "Linefeed") == 0)
		c = 012;
	    else if (strcasecmp(stk, "Delete") == 0)
		c = 0177;
	    else {
		if (mac->discard)
		    return (NIL);
		LispDestroy(mac, "READ: unkwnown character %s", stk);
	    }
	}
    }
    else
	c = stk[0];

    return (CHAR(c));
}

static void
LispSkipComment(LispMac *mac)
{
    int ch, comm = 1;

    for (;;) {
	ch = LispGet(mac);
	if (ch == '|' && LispGet(mac) == '#') {
	    if (--comm == 0)
		return;
	}
	else if (ch == '#' && LispGet(mac) == '|')
	    ++comm;
	else if (ch == EOF)
	    LispDestroy(mac, "READ: unexpected end of input");
    }
}

static LispObj *
LispReadEval(LispMac *mac)
{
    LispObj *code = LispRead(mac);

    if (mac->discard)
	return (code);

    if (INVALID_P(code))
	LispDestroy(mac, "READ: invalid eval code");

    return (EVAL(code));
}

static LispObj *
LispReadComplex(LispMac *mac)
{
    LispObj *number, *function, *arguments = LispRead(mac);
    int protect = mac->protect.length;

    /* form read */
    if (mac->discard)
	return (arguments);

    if (INVALID_P(arguments) || !CONS_P(arguments))
	LispDestroy(mac, "READ: invalid complex-number specification");

    if (mac->protect.length + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = arguments;
    function = Ocomplex;
    number = APPLY(function, arguments);
    mac->protect.length = protect;

    return (number);
}

static LispObj *
LispReadPathname(LispMac *mac)
{
    LispObj *function, *arguments;
    LispObj *path = LispRead(mac);
    int protect = mac->protect.length;

    /* form read */
    if (mac->discard)
	return (path);

    if (INVALID_P(path))
	LispDestroy(mac, "READ: invalid pathname specification");

    function = Oparse_namestring;
    if (mac->protect.length + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    arguments = CONS(path, NIL);
    mac->protect.objects[mac->protect.length++] = arguments;
    path = APPLY(function, arguments);
    mac->protect.length = protect;

    return (path);
}

static LispObj *
LispReadStruct(LispMac *mac)
{
    int len;
    char stk[128], *str;
    int protect = mac->protect.length;
    LispObj *struc, *function, *arguments = LispRead(mac);

    /* form read */
    if (mac->discard)
	return (arguments);

    if (INVALID_P(arguments) || !CONS_P(arguments) || !SYMBOL_P(CAR(arguments)))
	LispDestroy(mac, "READ: invalid structure specification");

    if (mac->protect.length + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = arguments;

    len = strlen(STRPTR(CAR(arguments)));
	   /* MAKE- */
    if (len + 6 > sizeof(stk))
	str = LispMalloc(mac, len + 6);
    else
	str = stk;
    sprintf(str, "MAKE-%s", STRPTR(CAR(arguments)));
    CAR(arguments) = ATOM(str);
    if (str != stk)
	LispFree(mac, str);
    function = Omake_struct;
    struc = APPLY(function, arguments);
    mac->protect.length = protect;

    return (struc);
}

static LispObj *
LispReadArray(LispMac *mac, long dimensions)
{
    long count;
    LispObj *function, *arguments, *initial, *dim, *cons;
    LispObj *data = LispRead(mac);
    int protect = mac->protect.length;

    /* form read */
    if (mac->discard)
	return (data);

    if (INVALID_P(data))
	LispDestroy(mac, "READ: invalid array specification");

    initial = Kinitial_contents;

    if (dimensions) {
	dim = cons = CONS(INTEGER(dimensions), NIL);

	for (count = 1; count < dimensions; count++) {
	    long length;
	    LispObj *obj;

	    for (obj = data, length = 0; length < count; length++) {
		if (!CONS_P(obj))
		    LispDestroy(mac, "READ: bad array for given dimension");
		obj = CAR(obj);
	    }

	    for (length = 1, obj = data; CONS_P(obj); obj = CDR(obj), length++)
		;
	    CDR(cons) = CONS(INTEGER(length), NIL);
	    cons = CDR(cons);
	}
    }
    else
	dim = NIL;

    function = Omake_array;

    protect = mac->protect.length;
    if (mac->protect.length + 2 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = dim;
    arguments = CONS(dim, CONS(initial, CONS(data, NIL)));
    mac->protect.objects[mac->protect.length++] = arguments;

    data = APPLY(function, arguments);
    mac->protect.length = protect;

    return (data);
}

static LispObj *
LispReadFeature(LispMac *mac, int with)
{
    LispObj *status;
    LispObj *feature = LispRead(mac);

    /* form read */
    if (mac->discard)
	return (feature);

    if (INVALID_P(feature))
	LispDestroy(mac, "READ: invalid feature specification");

    status = LispEvalFeature(mac, feature);

    if (with) {
	if (status == T)
	    return (LispRead(mac));

	/* need to use the field discard because the following expression
	 * may be #.FORM or #,FORM or any other form that may generate
	 * side effects */
	mac->discard = 1;
	LispRead(mac);
	mac->discard = 0;

	return (LispRead(mac));
    }

    if (status == NIL)
	return (LispRead(mac));

    mac->discard = 1;
    LispRead(mac);
    mac->discard = 0;

    return (LispRead(mac));
}

/*
 * A very simple eval loop with AND, NOT, and OR functions for testing
 * the available features.
 */
static LispObj *
LispEvalFeature(LispMac *mac, LispObj *feature)
{
    Atom_id test;
    LispObj *object;

    if (CONS_P(feature)) {
	LispObj *function = CAR(feature), *arguments = CDR(feature);

	if (!SYMBOL_P(function))
	    LispDestroy(mac, "READ: bad feature test function %s",
			STROBJ(function));
	if (!CONS_P(arguments))
	    LispDestroy(mac, "READ: bad feature test arguments %s",
			STROBJ(arguments));
	test = ATOMID(function);
	if (test == Sand) {
	    for (; CONS_P(arguments); arguments = CDR(arguments)) {
		if (LispEvalFeature(mac, CAR(arguments)) == NIL)
		    return (NIL);
	    }
	    return (T);
	}
	else if (test == Sor) {
	    for (; CONS_P(arguments); arguments = CDR(arguments)) {
		if (LispEvalFeature(mac, CAR(arguments)) == T)
		    return (T);
	    }
	    return (NIL);
	}
	else if (test == Snot) {
	    if (CONS_P(CDR(arguments)))
		LispDestroy(mac, "READ: too many arguments to NOT");

	    return (LispEvalFeature(mac, CAR(arguments)) == NIL ? T : NIL);
	}
	else
	    LispDestroy(mac, "READ: unimplemented feature test function %s",
			test);
    }

    if (KEYWORD_P(feature))
	feature = feature->data.quote;

    if (!SYMBOL_P(feature))
	LispDestroy(mac, "READ: bad feature specification %s",
		    STROBJ(feature));

    test = ATOMID(feature);

    /* check if specified atom is in the feature list
     * note that all elements of the feature list must be keywords */
    for (object = FEAT; CONS_P(object); object = CDR(object))
	if (ATOMID(CAR(object)) == test)
	    return (T);

    /* unknown feature */
    return (NIL);
}

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

/* $XFree86: xc/programs/xedit/lisp/read.c,v 1.23 2002/11/08 08:00:57 paulo Exp $ */

#include <errno.h>
#include "read.h"
#include "package.h"

/*
 * Protypes
 */
static int LispSkipWhiteSpace(void);
static LispObj *LispReadList(void);
static LispObj *LispReadQuote(void);
static LispObj *LispReadBackquote(void);
static LispObj *LispReadCommaquote(void);
static LispObj *LispReadObject(int);
static LispObj *LispParseAtom(char*, char*, int, int);
static LispObj *LispParseNumber(char*, int);
static int StringInRadix(char*, int, int);
static int AtomSeparator(int, int, int);
static LispObj *LispReadVector(void);
static LispObj *LispReadMacro(void);
static LispObj *LispReadFunction(void);
static LispObj *LispReadRational(int);
static LispObj *LispReadCharacter(void);
static void LispSkipComment(void);
static LispObj *LispReadEval(void);
static LispObj *LispReadComplex(void);
static LispObj *LispReadPathname(void);
static LispObj *LispReadStruct(void);
static LispObj *LispReadMacroArg(void);
static LispObj *LispReadArray(long);
static LispObj *LispReadFeature(int);
static LispObj *LispEvalFeature(LispObj*);

/*
 * Initialization
 */
static char *Char_Nul[] = {"Null", "Nul", NULL};
static char *Char_Soh[] = {"Soh", NULL};
static char *Char_Stx[] = {"Stx", NULL};
static char *Char_Etx[] = {"Etx", NULL};
static char *Char_Eot[] = {"Eot", NULL};
static char *Char_Enq[] = {"Enq", NULL};
static char *Char_Ack[] = {"Ack", NULL};
static char *Char_Bel[] = {"Bell", "Bel", NULL};
static char *Char_Bs[]  = {"Backspace", "Bs", NULL};
static char *Char_Tab[] = {"Tab", NULL};
static char *Char_Nl[]  = {"Newline", "Nl", "Lf", "Linefeed", NULL};
static char *Char_Vt[]  = {"Vt", NULL};
static char *Char_Np[]  = {"Page", "Np", NULL};
static char *Char_Cr[]  = {"Return", "Cr", NULL};
static char *Char_Ff[]  = {"So", "Ff", NULL};
static char *Char_Si[]  = {"Si", NULL};
static char *Char_Dle[] = {"Dle", NULL};
static char *Char_Dc1[] = {"Dc1", NULL};
static char *Char_Dc2[] = {"Dc2", NULL};
static char *Char_Dc3[] = {"Dc3", NULL};
static char *Char_Dc4[] = {"Dc4", NULL};
static char *Char_Nak[] = {"Nak", NULL};
static char *Char_Syn[] = {"Syn", NULL};
static char *Char_Etb[] = {"Etb", NULL};
static char *Char_Can[] = {"Can", NULL};
static char *Char_Em[]  = {"Em", NULL};
static char *Char_Sub[] = {"Sub", NULL};
static char *Char_Esc[] = {"Escape", "Esc", NULL};
static char *Char_Fs[]  = {"Fs", NULL};
static char *Char_Gs[]  = {"Gs", NULL};
static char *Char_Rs[]  = {"Rs", NULL};
static char *Char_Us[]  = {"Us", NULL};
static char *Char_Sp[]  = {"Space", "Sp", NULL};
static char *Char_Del[] = {"Rubout", "Del", "Delete", NULL};

LispCharInfo LispChars[256] = {
    {Char_Nul},
    {Char_Soh},
    {Char_Stx},
    {Char_Etx},
    {Char_Eot},
    {Char_Enq},
    {Char_Ack},
    {Char_Bel},
    {Char_Bs},
    {Char_Tab},
    {Char_Nl},
    {Char_Vt},
    {Char_Np},
    {Char_Cr},
    {Char_Ff},
    {Char_Si},
    {Char_Dle},
    {Char_Dc1},
    {Char_Dc2},
    {Char_Dc3},
    {Char_Dc4},
    {Char_Nak},
    {Char_Syn},
    {Char_Etb},
    {Char_Can},
    {Char_Em},
    {Char_Sub},
    {Char_Esc},
    {Char_Fs},
    {Char_Gs},
    {Char_Rs},
    {Char_Us},
    {Char_Sp},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {Char_Del},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL}
};

Atom_id Sand, Sor, Snot;


/*
 * Implementation
 */
LispObj *
LispRead(void)
{
    LispObj *object, *code = COD;
    int ch = LispSkipWhiteSpace();

    switch (ch) {
	case '(':
	    object = LispReadList();
	    break;
	case ')':
	    for (ch = LispGet(); ch != EOF && ch != '\n'; ch = LispGet()) {
		if (!isspace(ch)) {
		    LispUnget(ch);
		    break;
		}
	    }
	    return (EOLIST);
	case EOF:
	    return (NULL);
	case '\'':
	    object = LispReadQuote();
	    break;
	case '`':
	    object = LispReadBackquote();
	    break;
	case ',':
	    object = LispReadCommaquote();
	    break;
	case '#':
	    object = LispReadMacro();
	    /* Don't "link" EOLIST to COD, this may happen if
	     * a multiline comment is the last token in a form. */
	    if (INVALIDP(object))
		return (object);
	    break;
	default:
	    LispUnget(ch);
	    object = LispReadObject(0);
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
LispReadMacro(void)
{
    int ch = LispGet();

    switch (ch) {
	case '(':
	    return (LispReadVector());
	case '\'':
	    return (LispReadFunction());
	case 'b':
	case 'B':
	    return (LispReadRational(2));
	case 'o':
	case 'O':
	    return (LispReadRational(8));
	case 'x':
	case 'X':
	    return (LispReadRational(16));
	case '\\':
	    return (LispReadCharacter());
	case '|':
	    LispSkipComment();
	    return (LispRead());
	case '.':	/* eval when compiling */
	case ',':	/* eval when loading */
	    return (LispReadEval());
	case 'c':
	case 'C':
	    return (LispReadComplex());
	case 'p':
	case 'P':
	    return (LispReadPathname());
	case 's':
	case 'S':
	    return (LispReadStruct());
	case '+':
	    return (LispReadFeature(1));
	case '-':
	    return (LispReadFeature(0));
	case ':':
	    /* Uninterned symbol */
	    return (LispReadObject(1));
	default:
	    if (isdigit(ch)) {
		LispUnget(ch);
		return (LispReadMacroArg());
	    }
	    if (!lisp__data.discard)
		LispDestroy("READ: undefined dispatch macro character #%c", ch);
    }

    return (NIL);
}

static LispObj *
LispReadMacroArg(void)
{
    long integer;
    int ch;

    /* skip leading zeros */
    while (ch = LispGet(), ch != EOF && isdigit(ch) && ch == '0')
	;

    if (ch == EOF)
	LispDestroy("READ: unexpected end of input");

    /* if ch is not a number the argument was zero */
    if (isdigit(ch)) {
	char stk[32], *str;
	int len = 1;

	stk[0] = ch;
	for (;;) {
	    ch = LispGet();
	    if (!isdigit(ch))
		break;
	    if (len + 1 >= sizeof(stk)) {
		if (lisp__data.discard)
		    continue;
		LispDestroy("READ: number is not a fixnum");
	    }
	    stk[len++] = ch;
	}
	stk[len] = '\0';
	integer = strtol(stk, &str, 10);
	if (*str || errno == ERANGE) {
	    if (!lisp__data.discard)
		LispDestroy("READ: number is not a fixnum");
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
		if ((ch = LispSkipWhiteSpace()) != '(') {
		    if (lisp__data.discard)
			return (ch == EOF ? NULL : NIL);
		    LispDestroy("READ: bad array specification");
		}
		return (LispReadVector());
	    }
	    return (LispReadArray(integer));
	case 'r':
	case 'R':
	    return (LispReadRational(integer));
	default:
	    if (!lisp__data.discard)
		LispDestroy("READ: undefined dispatch macro character #%c", ch);
    }

    return (NIL);
}

static int
LispSkipWhiteSpace(void)
{
    int ch;

    for (;;) {
	while (ch = LispGet(), isspace(ch) && ch != EOF)
	    ;
	if (ch == ';') {
	    while (ch = LispGet(), ch != '\n' && ch != EOF)
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
LispReadList(void)
{
    GC_ENTER();
    LispObj *result, *cons, *object;
    int dot = 0;

    /* check for () */
    object = LispRead();
    if (object == EOLIST)
	return (NIL);

    if (object == DOT) {
	if (!lisp__data.discard)
	    LispDestroy("READ: illegal start of dotted list");
	return (NIL);
    }

    result = cons = CONS(object, NIL);

    /* make sure GC will not release data being read */
    GC_PROTECT(result);

    while ((object = LispRead()) != EOLIST) {
	if (object == NULL)
	    LispDestroy("READ: unexpected end of input");
	if (object == DOT) {
	    /* this is a dotted list */
	    if (dot) {
		if (!lisp__data.discard)
		    LispDestroy("READ: more than one . in list");
		GC_LEAVE();

		return (NIL);
	    }
	    dot = 1;
	}
	else {
	    if (dot) {
		/* only one object after a dot */
		if (++dot > 2) {
		    if (!lisp__data.discard)
			LispDestroy("READ: more than one object after . in list");
		    GC_LEAVE();

		    return (NIL);
		}
		RPLACD(cons, object);
	    }
	    else {
		RPLACD(cons, CONS(object, NIL));
		cons = CDR(cons);
	    }
	}
    }

    /* this will happen if last list element was a dot */
    if (dot == 1) {
	if (!lisp__data.discard)
	    LispDestroy("READ: illegal end of dotted list");
	GC_LEAVE();

	return (NIL);
    }

    GC_LEAVE();

    return (result);
}

static LispObj *
LispReadQuote(void)
{
    LispObj *quote = LispRead();

    if (INVALIDP(quote)) {
	if (lisp__data.discard)
	    return (NULL);
	LispDestroy("READ: illegal quoted object");
    }

    return (QUOTE(quote));
}

static LispObj *
LispReadBackquote(void)
{
    LispObj *backquote = LispRead();

    if (INVALIDP(backquote)) {
	if (lisp__data.discard)
	    return (NULL);
	LispDestroy("READ: illegal back-quoted object");
    }

    return (BACKQUOTE(backquote));
}

static LispObj *
LispReadCommaquote(void)
{
    LispObj *comma;
    int atlist = LispGet();

    if (atlist == EOF)
	LispDestroy("READ: unexpected end of input");
    else if (atlist != '@' && atlist != '.')
	LispUnget(atlist);

    comma = LispRead();
    if (comma == DOT) {
	atlist = '@';
	comma = LispRead();
    }
    if (INVALIDP(comma)) {
	if (lisp__data.discard)
	    return (NULL);
	LispDestroy("READ: illegal comma-quoted object");
    }

    return (COMMA(comma, atlist == '@' || atlist == '.'));
}

/*
 * Read anything that is not readily identifiable by it's first character
 * and also put the code for reading atoms, numbers and strings together.
 */
static LispObj *
LispReadObject(int unintern)
{
    LispObj *object;
    char stk[128], *string, *package, *symbol;
    int ch, length, backslash, size, quote, unreadable, collon;

    package = symbol = string = stk;
    size = sizeof(stk);
    backslash = quote = unreadable = collon = 0;
    length = 0;

    ch = LispGet();
    if (unintern && (ch == ':' || ch == '"'))
	LispDestroy("READ: syntax error after #:");
    else if (ch == '"' || ch == '|')
	quote = ch;
    else if (ch == '\\') {
	unreadable = backslash = 1;
	string[length++] = ch;
    }
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
	ch = LispGet();

	if (ch == EOF) {
	    if (quote) {
		/* if quote, file ended with an open quoted object */
		if (string != stk)
		    LispFree(string);
		return (NULL);
	    }
	    break;
	}

	if (ch == '\\') {
	    backslash = !backslash;
	    if (quote == '"') {
		/* only remove backslashs from strings */
		if (backslash)
		    continue;
	    }
	    else
		unreadable = 1;
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
	    else if (AtomSeparator(ch, 0, 0)) {
		LispUnget(ch);
		break;
	    }
	    else if (ch == ':') {
		if (collon == 0 ||
		    (collon == (1 - unintern) && symbol == string + length)) {
		    ++collon;
		    symbol = string + length + 1;
		}
		else
		    LispDestroy("READ: too many collons");
	    }
	}

	if (length + 2 >= size) {
	    if (string == stk) {
		size = 1024;
		string = LispMalloc(size);
		strcpy(string, stk);
	    }
	    else {
		size += 1024;
		string = LispRealloc(string, size);
	    }
	    symbol = string + (symbol - package);
	    package = string;
	}
	string[length++] = ch;
    }

    if (lisp__data.discard) {
	if (string != stk)
	    LispFree(string);

	return (ch == EOF ? NULL : NIL);
    }

    string[length] = '\0';

    if (unintern) {
	if (length == 0)
	    LispDestroy("READ: syntax error after #:");
	object = UNINTERNED_ATOM(string);
    }

    else if (quote == '"')
	object = LSTRING(string, length);

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
	object = LispParseAtom(package, symbol,
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
	object = LispParseNumber(string, 10);

    else
	/* A normal atom */
	object = ATOM(string);

    if (string != stk)
	LispFree(string);

    return (object);
}

static LispObj *
LispParseAtom(char *package, char *symbol, int intern, int unreadable)
{
    LispObj *object = NULL, *thepackage = NULL;
    LispPackage *pack = NULL;

    /* If package is empty, it is a keyword */
    if (package[0] == '\0') {
	thepackage = lisp__data.keyword;
	pack = lisp__data.key;
    }

    else {
	/* Else, search it in the package list */
	thepackage = LispFindPackageFromString(package);

	if (thepackage == NIL)
	    LispDestroy("READ: the package %s is not available", package);

	pack = thepackage->data.package.package;
    }

    if (pack == lisp__data.pack && intern) {
	/* Redundant package specification, since requesting a
	 * intern symbol, create it if does not exist */

	object = ATOM(symbol);
	if (unreadable)
	    object->data.atom->unreadable = 1;
    }

    else if (intern || pack == lisp__data.key) {
	/* Symbol is created, or just fetched from the specified package */

	LispPackage *savepack;
	LispObj *savepackage = PACKAGE;

	/* Remember curent package */
	savepack = lisp__data.pack;

	/* Temporarily set another package */
	lisp__data.pack = pack;
	PACKAGE = thepackage;

	/* Get the object pointer */
	if (pack == lisp__data.key)
	    object = KEYWORD(LispDoGetAtom(symbol, 0)->string);
	else
	    object = ATOM(symbol);
	if (unreadable)
	    object->data.atom->unreadable = 1;

	/* Restore current package */
	lisp__data.pack = savepack;
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
	    LispDestroy("READ: no extern symbol %s in package %s",
			symbol, package);
    }

    return (object);
}

static LispObj *
LispParseNumber(char *str, int radix)
{
    mpr *rop;
    mpi *iop;
    int len;
    double dfloat;
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

	    dfloat = strtod(str, NULL);
	    if (!finite(dfloat))
		LispDestroy("READ: floating point overflow");

	    return (DFLOAT(dfloat));
	}
    }

    iop = NULL;			/* if ratio and set, numerator was too large */
    rop = NULL;			/* if ratio and set, denominator was too large */

    /* check if correctly specified in the given radix */

    len = strlen(str) - 1;
    if (!ratio && radix != 10 && str[len] == '.')
	str[len] = '\0';

    if (ratio || radix != 10) {
	if (!StringInRadix(str, radix, 1)) {
	    if (ratio)
		ratio[-1] = '/';
	    return (ATOM(str));
	}
	if (ratio && !StringInRadix(ratio, radix, 0)) {
	    ratio[-1] = '/';
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
	iop = LispMalloc(sizeof(mpi));
	mpi_init(iop);
	mpi_setstr(iop, str, radix);
	if (ratio == NULL) {
	    number = BIGNUM(iop);
	    LispMused(iop);

	    return (number);
	}
    }

    if (ratio) {
	long denominator;

	errno = 0;
	denominator = strtol(ratio, NULL, radix);
	if (denominator == 0)
	    LispDestroy("READ: divide by zero");

	/* if denominator won't fit in a long */
	if (denominator == LONG_MAX && errno == ERANGE) {
	    rop = LispMalloc(sizeof(mpr));
	    mpr_init(rop);
	    if (iop) {
		mpi_set(mpr_num(rop), iop);
		mpi_clear(iop);
		LispFree(iop);
		iop = NULL;
	    }
	    else		
		mpi_setstr(mpr_num(rop), str, radix);
	    mpi_setstr(mpr_den(rop), ratio, radix);
	}
	else if (iop) {
	    rop = LispMalloc(sizeof(mpr));
	    mpr_init(rop);
	    mpi_set(mpr_num(rop), iop);
	    mpi_clear(iop);
	    LispFree(iop);
	    iop = NULL;
	    mpi_seti(mpr_den(rop), denominator);
	}

	if (rop) {
	    mpr_canonicalize(rop);
	    if (mpr_fiti(rop)) {
		integer = mpi_geti(mpr_num(rop));
		denominator = mpi_geti(mpr_den(rop));
		mpr_clear(rop);
		LispFree(rop);
		rop = NULL;
		/* if denominator becames 1, RATIO will convert to INTEGER */
		number = RATIO(integer, denominator);
	    }
	    else {
		number = BIGRATIO(rop);
		LispMused(rop);
	    }

	    return (number);
	}

	return (RATIO(integer, denominator));
    }

    if (iop) {
	number = BIGNUM(iop);
	LispMused(iop);
    }
    else
	number = INTEGER(integer);

    return (number);
}

static int
StringInRadix(char *str, int radix, int skip_sign)
{
    if (skip_sign && (*str == '-' || *str == '+'))
	++str;
    while (*str) {
	if (*str >= '0' && *str <= '9') {
	    if (*str - '0' >= radix)
		return (0);
	}
	else if (*str >= 'A' && *str <= 'Z') {
	    if (radix <= 10 || *str - 'A' + 10 >= radix)
		return (0);
	}
	else
	    return (0);
	str++;
    }

    return (1);
}

static int
AtomSeparator(int ch, int check_space, int check_backslash)
{
    if (check_space && isspace(ch))
	return (1);
    if (check_backslash && ch == '\\')
	return (1);
    return (strchr("(),\";'`#|,@", ch) != NULL);
}

static LispObj *
LispReadVector(void)
{
    LispObj *object;
    LispObj *objects;

    objects = LispReadList();

    if (lisp__data.discard)
	return (NULL);

    for (object = objects; CONSP(object); object = CDR(object))
	;
    if (object != NIL)
	LispDestroy("READ: vector cannot be a dotted list");

    return (VECTOR(objects));
}

static LispObj *
LispReadFunction(void)
{
    LispObj *function = LispRead();

    if (lisp__data.discard)
	return (function);

    if (INVALIDP(function)) 
	LispDestroy("READ: illegal function object");
    else if (CONSP(function)) {
	if (CAR(function) != Olambda)
	    LispDestroy("READ: %s is not a valid lambda", STROBJ(function));

	return (EVAL(function));
    }
    else if (!SYMBOLP(function))
	LispDestroy("READ: %s cannot name a function", STROBJ(function));

    return (QUOTE(function));
}

static LispObj *
LispReadRational(int radix)
{
    LispObj *number;
    int ch, len, size;
    char stk[128], *str;

    len = 0;
    str = stk;
    size = sizeof(stk);

    for (;;) {
	ch = LispGet();
	if (ch == EOF || isspace(ch))
	    break;
	else if (AtomSeparator(ch, 0, 1)) {
	    LispUnget(ch);
	    break;
	}
	else if (islower(ch))
	    ch = toupper(ch);
	if ((ch < '0' || ch > '9') && (ch < 'A' || ch > 'Z') &&
	    ch != '+' && ch != '-' && ch != '/') {
	    if (str != stk)
		LispFree(str);
	    if (!lisp__data.discard)
		LispDestroy("READ: bad character %c for rational number", ch);
	}
	if (len + 1 >= size) {
	    if (str == stk) {
		size = 512;
		str = LispMalloc(size);
		strcpy(str + 1, stk + 1);
	    }
	    else {
		size += 512;
		str = LispRealloc(str, size);
	    }
	}
	str[len++] = ch;
    }

    if (lisp__data.discard) {
	if (str != stk)
	    LispFree(str);

	return (ch == EOF ? NULL : NIL);
    }

    str[len] = '\0';

    number = LispParseNumber(str, radix);
    if (str != stk)
	LispFree(str);

    if (!RATIONALP(number))
	LispDestroy("READ: bad rational number specification");

    return (number);
}

static LispObj *
LispReadCharacter(void)
{
    long c;
    int ch, len;
    char stk[64];

    ch = LispGet();
    if (ch == EOF)
	return (NULL);

    stk[0] = ch;
    len = 1;

    for (;;) {
	ch = LispGet();
	if (ch == EOF)
	    break;
	else if (ch != '-' && !isalnum(ch)) {
	    LispUnget(ch);
	    break;
	}
	if (len + 1 < sizeof(stk))
	    stk[len++] = ch;
    }
    if (len > 1) {
	char **names;
	int found = 0;
	stk[len] = '\0';

	for (c = ch = 0; ch <= ' ' && !found; ch++) {
	    for (names = LispChars[ch].names; *names; names++)
		if (strcasecmp(*names, stk) == 0) {
		    c = ch;
		    found = 1;
		    break;
		}
	}
	if (!found) {
	    for (names = LispChars[0177].names; *names; names++)
		if (strcasecmp(*names, stk) == 0) {
		    c = 0177;
		    found = 1;
		    break;
		}
	}

	if (!found) {
	    if (lisp__data.discard)
		return (NIL);
	    LispDestroy("READ: unkwnown character %s", stk);
	}
    }
    else
	c = stk[0];

    return (SCHAR(c));
}

static void
LispSkipComment(void)
{
    int ch, comm = 1;

    for (;;) {
	ch = LispGet();
	if (ch == '#') {
	    ch = LispGet();
	    if (ch == '|')
		++comm;
	    continue;
	}
	while (ch == '|') {
	    ch = LispGet();
	    if (ch == '#' && --comm == 0)
		return;
	}
	if (ch == EOF)
	    LispDestroy("READ: unexpected end of input");
    }
}

static LispObj *
LispReadEval(void)
{
    LispObj *code = LispRead();

    if (lisp__data.discard)
	return (code);

    if (INVALIDP(code))
	LispDestroy("READ: invalid eval code");

    return (EVAL(code));
}

static LispObj *
LispReadComplex(void)
{
    GC_ENTER();
    LispObj *number, *arguments = LispRead();

    /* form read */
    if (lisp__data.discard)
	return (arguments);

    if (INVALIDP(arguments) || !CONSP(arguments))
	LispDestroy("READ: invalid complex-number specification");

    GC_PROTECT(arguments);
    number = APPLY(Ocomplex, arguments);
    GC_LEAVE();

    return (number);
}

static LispObj *
LispReadPathname(void)
{
    GC_ENTER();
    LispObj *path = LispRead();

    /* form read */
    if (lisp__data.discard)
	return (path);

    if (INVALIDP(path))
	LispDestroy("READ: invalid pathname specification");

    GC_PROTECT(path);
    path = APPLY1(Oparse_namestring, path);
    GC_LEAVE();

    return (path);
}

static LispObj *
LispReadStruct(void)
{
    GC_ENTER();
    int len;
    char stk[128], *str;
    LispObj *struc, *arguments = LispRead();

    /* form read */
    if (lisp__data.discard)
	return (arguments);

    if (INVALIDP(arguments) || !CONSP(arguments) || !SYMBOLP(CAR(arguments)))
	LispDestroy("READ: invalid structure specification");

    GC_PROTECT(arguments);

    len = strlen(ATOMID(CAR(arguments)));
	   /* MAKE- */
    if (len + 6 > sizeof(stk))
	str = LispMalloc(len + 6);
    else
	str = stk;
    sprintf(str, "MAKE-%s", ATOMID(CAR(arguments)));
    RPLACA(arguments, ATOM(str));
    if (str != stk)
	LispFree(str);
    struc = APPLY(Omake_struct, arguments);
    GC_LEAVE();

    return (struc);
}

static LispObj *
LispReadArray(long dimensions)
{
    GC_ENTER();
    long count;
    LispObj *arguments, *initial, *dim, *cons;
    LispObj *data = LispRead();

    /* form read */
    if (lisp__data.discard)
	return (data);

    if (INVALIDP(data))
	LispDestroy("READ: invalid array specification");

    initial = Kinitial_contents;

    dim = cons = NIL;
    if (dimensions) {
	LispObj *array;

	for (count = 0, array = data; count < dimensions; count++) {
	    long length;
	    LispObj *item;

	    if (!CONSP(array))
		LispDestroy("READ: bad array for given dimension");
	    item = array;
	    array = CAR(array);

	    for (length = 0; CONSP(item); item = CDR(item), length++)
		;

	    if (dim == NIL) {
		dim = cons = CONS(FIXNUM(length), NIL);
		GC_PROTECT(dim);
	    }
	    else {
		RPLACD(cons, CONS(FIXNUM(length), NIL));
		cons = CDR(cons);
	    }
	}
    }

    arguments = CONS(dim, CONS(initial, CONS(data, NIL)));
    GC_PROTECT(arguments);
    data = APPLY(Omake_array, arguments);
    GC_LEAVE();

    return (data);
}

static LispObj *
LispReadFeature(int with)
{
    LispObj *status;
    LispObj *feature = LispRead();

    /* form read */
    if (lisp__data.discard)
	return (feature);

    if (INVALIDP(feature))
	LispDestroy("READ: invalid feature specification");

    status = LispEvalFeature(feature);

    if (with) {
	if (status == T)
	    return (LispRead());

	/* need to use the field discard because the following expression
	 * may be #.FORM or #,FORM or any other form that may generate
	 * side effects */
	lisp__data.discard = 1;
	LispRead();
	lisp__data.discard = 0;

	return (LispRead());
    }

    if (status == NIL)
	return (LispRead());

    lisp__data.discard = 1;
    LispRead();
    lisp__data.discard = 0;

    return (LispRead());
}

/*
 * A very simple eval loop with AND, NOT, and OR functions for testing
 * the available features.
 */
static LispObj *
LispEvalFeature(LispObj *feature)
{
    Atom_id test;
    LispObj *object;

    if (CONSP(feature)) {
	LispObj *function = CAR(feature), *arguments = CDR(feature);

	if (!SYMBOLP(function))
	    LispDestroy("READ: bad feature test function %s",
			STROBJ(function));
	if (!CONSP(arguments))
	    LispDestroy("READ: bad feature test arguments %s",
			STROBJ(arguments));
	test = ATOMID(function);
	if (test == Sand) {
	    for (; CONSP(arguments); arguments = CDR(arguments)) {
		if (LispEvalFeature(CAR(arguments)) == NIL)
		    return (NIL);
	    }
	    return (T);
	}
	else if (test == Sor) {
	    for (; CONSP(arguments); arguments = CDR(arguments)) {
		if (LispEvalFeature(CAR(arguments)) == T)
		    return (T);
	    }
	    return (NIL);
	}
	else if (test == Snot) {
	    if (CONSP(CDR(arguments)))
		LispDestroy("READ: too many arguments to NOT");

	    return (LispEvalFeature(CAR(arguments)) == NIL ? T : NIL);
	}
	else
	    LispDestroy("READ: unimplemented feature test function %s", test);
    }

    if (KEYWORDP(feature))
	feature = feature->data.quote;
    else if (!SYMBOLP(feature))
	LispDestroy("READ: bad feature specification %s", STROBJ(feature));

    test = ATOMID(feature);

    /* check if specified atom is in the feature list
     * note that all elements of the feature list must be keywords */
    for (object = FEATURES; CONSP(object); object = CDR(object))
	if (ATOMID(CAR(object)) == test)
	    return (T);

    /* unknown feature */
    return (NIL);
}

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

/* $XFree86: xc/programs/xedit/lisp/read.c,v 1.20 2002/08/25 02:48:31 paulo Exp $ */

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
static int StringInRadix(char*, int, int);
static int AtomSeparator(int, int, int);
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
    {{LispCharacter_t /*   0 */}, Char_Nul},
    {{LispCharacter_t /*   1 */}, Char_Soh},
    {{LispCharacter_t /*   2 */}, Char_Stx},
    {{LispCharacter_t /*   3 */}, Char_Etx},
    {{LispCharacter_t /*   4 */}, Char_Eot},
    {{LispCharacter_t /*   5 */}, Char_Enq},
    {{LispCharacter_t /*   6 */}, Char_Ack},
    {{LispCharacter_t /*   7 */}, Char_Bel},
    {{LispCharacter_t /*   8 */}, Char_Bs},
    {{LispCharacter_t /*   9 */}, Char_Tab},
    {{LispCharacter_t /*  10 */}, Char_Nl},
    {{LispCharacter_t /*  11 */}, Char_Vt},
    {{LispCharacter_t /*  12 */}, Char_Np},
    {{LispCharacter_t /*  13 */}, Char_Cr},
    {{LispCharacter_t /*  14 */}, Char_Ff},
    {{LispCharacter_t /*  15 */}, Char_Si},
    {{LispCharacter_t /*  16 */}, Char_Dle},
    {{LispCharacter_t /*  17 */}, Char_Dc1},
    {{LispCharacter_t /*  18 */}, Char_Dc2},
    {{LispCharacter_t /*  19 */}, Char_Dc3},
    {{LispCharacter_t /*  20 */}, Char_Dc4},
    {{LispCharacter_t /*  21 */}, Char_Nak},
    {{LispCharacter_t /*  22 */}, Char_Syn},
    {{LispCharacter_t /*  23 */}, Char_Etb},
    {{LispCharacter_t /*  24 */}, Char_Can},
    {{LispCharacter_t /*  25 */}, Char_Em},
    {{LispCharacter_t /*  26 */}, Char_Sub},
    {{LispCharacter_t /*  27 */}, Char_Esc},
    {{LispCharacter_t /*  28 */}, Char_Fs},
    {{LispCharacter_t /*  29 */}, Char_Gs},
    {{LispCharacter_t /*  30 */}, Char_Rs},
    {{LispCharacter_t /*  31 */}, Char_Us},
    {{LispCharacter_t /*  32 */}, Char_Sp},
    {{LispCharacter_t /*  33 */}, NULL},
    {{LispCharacter_t /*  34 */}, NULL},
    {{LispCharacter_t /*  35 */}, NULL},
    {{LispCharacter_t /*  36 */}, NULL},
    {{LispCharacter_t /*  37 */}, NULL},
    {{LispCharacter_t /*  38 */}, NULL},
    {{LispCharacter_t /*  39 */}, NULL},
    {{LispCharacter_t /*  40 */}, NULL},
    {{LispCharacter_t /*  41 */}, NULL},
    {{LispCharacter_t /*  42 */}, NULL},
    {{LispCharacter_t /*  43 */}, NULL},
    {{LispCharacter_t /*  44 */}, NULL},
    {{LispCharacter_t /*  45 */}, NULL},
    {{LispCharacter_t /*  46 */}, NULL},
    {{LispCharacter_t /*  47 */}, NULL},
    {{LispCharacter_t /*  48 */}, NULL},
    {{LispCharacter_t /*  49 */}, NULL},
    {{LispCharacter_t /*  50 */}, NULL},
    {{LispCharacter_t /*  51 */}, NULL},
    {{LispCharacter_t /*  52 */}, NULL},
    {{LispCharacter_t /*  53 */}, NULL},
    {{LispCharacter_t /*  54 */}, NULL},
    {{LispCharacter_t /*  55 */}, NULL},
    {{LispCharacter_t /*  56 */}, NULL},
    {{LispCharacter_t /*  57 */}, NULL},
    {{LispCharacter_t /*  58 */}, NULL},
    {{LispCharacter_t /*  59 */}, NULL},
    {{LispCharacter_t /*  60 */}, NULL},
    {{LispCharacter_t /*  61 */}, NULL},
    {{LispCharacter_t /*  62 */}, NULL},
    {{LispCharacter_t /*  63 */}, NULL},
    {{LispCharacter_t /*  64 */}, NULL},
    {{LispCharacter_t /*  65 */}, NULL},
    {{LispCharacter_t /*  66 */}, NULL},
    {{LispCharacter_t /*  67 */}, NULL},
    {{LispCharacter_t /*  68 */}, NULL},
    {{LispCharacter_t /*  69 */}, NULL},
    {{LispCharacter_t /*  70 */}, NULL},
    {{LispCharacter_t /*  71 */}, NULL},
    {{LispCharacter_t /*  72 */}, NULL},
    {{LispCharacter_t /*  73 */}, NULL},
    {{LispCharacter_t /*  74 */}, NULL},
    {{LispCharacter_t /*  75 */}, NULL},
    {{LispCharacter_t /*  76 */}, NULL},
    {{LispCharacter_t /*  77 */}, NULL},
    {{LispCharacter_t /*  78 */}, NULL},
    {{LispCharacter_t /*  79 */}, NULL},
    {{LispCharacter_t /*  80 */}, NULL},
    {{LispCharacter_t /*  81 */}, NULL},
    {{LispCharacter_t /*  82 */}, NULL},
    {{LispCharacter_t /*  83 */}, NULL},
    {{LispCharacter_t /*  84 */}, NULL},
    {{LispCharacter_t /*  85 */}, NULL},
    {{LispCharacter_t /*  86 */}, NULL},
    {{LispCharacter_t /*  87 */}, NULL},
    {{LispCharacter_t /*  88 */}, NULL},
    {{LispCharacter_t /*  89 */}, NULL},
    {{LispCharacter_t /*  90 */}, NULL},
    {{LispCharacter_t /*  91 */}, NULL},
    {{LispCharacter_t /*  92 */}, NULL},
    {{LispCharacter_t /*  93 */}, NULL},
    {{LispCharacter_t /*  94 */}, NULL},
    {{LispCharacter_t /*  95 */}, NULL},
    {{LispCharacter_t /*  96 */}, NULL},
    {{LispCharacter_t /*  97 */}, NULL},
    {{LispCharacter_t /*  98 */}, NULL},
    {{LispCharacter_t /*  99 */}, NULL},
    {{LispCharacter_t /* 100 */}, NULL},
    {{LispCharacter_t /* 101 */}, NULL},
    {{LispCharacter_t /* 102 */}, NULL},
    {{LispCharacter_t /* 103 */}, NULL},
    {{LispCharacter_t /* 104 */}, NULL},
    {{LispCharacter_t /* 105 */}, NULL},
    {{LispCharacter_t /* 106 */}, NULL},
    {{LispCharacter_t /* 107 */}, NULL},
    {{LispCharacter_t /* 108 */}, NULL},
    {{LispCharacter_t /* 109 */}, NULL},
    {{LispCharacter_t /* 110 */}, NULL},
    {{LispCharacter_t /* 111 */}, NULL},
    {{LispCharacter_t /* 112 */}, NULL},
    {{LispCharacter_t /* 113 */}, NULL},
    {{LispCharacter_t /* 114 */}, NULL},
    {{LispCharacter_t /* 115 */}, NULL},
    {{LispCharacter_t /* 116 */}, NULL},
    {{LispCharacter_t /* 117 */}, NULL},
    {{LispCharacter_t /* 118 */}, NULL},
    {{LispCharacter_t /* 119 */}, NULL},
    {{LispCharacter_t /* 110 */}, NULL},
    {{LispCharacter_t /* 121 */}, NULL},
    {{LispCharacter_t /* 122 */}, NULL},
    {{LispCharacter_t /* 123 */}, NULL},
    {{LispCharacter_t /* 124 */}, NULL},
    {{LispCharacter_t /* 125 */}, NULL},
    {{LispCharacter_t /* 126 */}, NULL},
    {{LispCharacter_t /* 127 */}, Char_Del},
    {{LispCharacter_t /* 128 */}, NULL},
    {{LispCharacter_t /* 129 */}, NULL},
    {{LispCharacter_t /* 130 */}, NULL},
    {{LispCharacter_t /* 131 */}, NULL},
    {{LispCharacter_t /* 132 */}, NULL},
    {{LispCharacter_t /* 133 */}, NULL},
    {{LispCharacter_t /* 134 */}, NULL},
    {{LispCharacter_t /* 135 */}, NULL},
    {{LispCharacter_t /* 136 */}, NULL},
    {{LispCharacter_t /* 137 */}, NULL},
    {{LispCharacter_t /* 138 */}, NULL},
    {{LispCharacter_t /* 139 */}, NULL},
    {{LispCharacter_t /* 140 */}, NULL},
    {{LispCharacter_t /* 141 */}, NULL},
    {{LispCharacter_t /* 142 */}, NULL},
    {{LispCharacter_t /* 143 */}, NULL},
    {{LispCharacter_t /* 144 */}, NULL},
    {{LispCharacter_t /* 145 */}, NULL},
    {{LispCharacter_t /* 146 */}, NULL},
    {{LispCharacter_t /* 147 */}, NULL},
    {{LispCharacter_t /* 148 */}, NULL},
    {{LispCharacter_t /* 149 */}, NULL},
    {{LispCharacter_t /* 150 */}, NULL},
    {{LispCharacter_t /* 151 */}, NULL},
    {{LispCharacter_t /* 152 */}, NULL},
    {{LispCharacter_t /* 153 */}, NULL},
    {{LispCharacter_t /* 154 */}, NULL},
    {{LispCharacter_t /* 155 */}, NULL},
    {{LispCharacter_t /* 156 */}, NULL},
    {{LispCharacter_t /* 157 */}, NULL},
    {{LispCharacter_t /* 158 */}, NULL},
    {{LispCharacter_t /* 159 */}, NULL},
    {{LispCharacter_t /* 160 */}, NULL},
    {{LispCharacter_t /* 161 */}, NULL},
    {{LispCharacter_t /* 162 */}, NULL},
    {{LispCharacter_t /* 163 */}, NULL},
    {{LispCharacter_t /* 164 */}, NULL},
    {{LispCharacter_t /* 165 */}, NULL},
    {{LispCharacter_t /* 166 */}, NULL},
    {{LispCharacter_t /* 167 */}, NULL},
    {{LispCharacter_t /* 168 */}, NULL},
    {{LispCharacter_t /* 169 */}, NULL},
    {{LispCharacter_t /* 170 */}, NULL},
    {{LispCharacter_t /* 171 */}, NULL},
    {{LispCharacter_t /* 172 */}, NULL},
    {{LispCharacter_t /* 173 */}, NULL},
    {{LispCharacter_t /* 174 */}, NULL},
    {{LispCharacter_t /* 175 */}, NULL},
    {{LispCharacter_t /* 176 */}, NULL},
    {{LispCharacter_t /* 177 */}, NULL},
    {{LispCharacter_t /* 178 */}, NULL},
    {{LispCharacter_t /* 179 */}, NULL},
    {{LispCharacter_t /* 180 */}, NULL},
    {{LispCharacter_t /* 181 */}, NULL},
    {{LispCharacter_t /* 182 */}, NULL},
    {{LispCharacter_t /* 183 */}, NULL},
    {{LispCharacter_t /* 184 */}, NULL},
    {{LispCharacter_t /* 185 */}, NULL},
    {{LispCharacter_t /* 186 */}, NULL},
    {{LispCharacter_t /* 187 */}, NULL},
    {{LispCharacter_t /* 188 */}, NULL},
    {{LispCharacter_t /* 189 */}, NULL},
    {{LispCharacter_t /* 190 */}, NULL},
    {{LispCharacter_t /* 191 */}, NULL},
    {{LispCharacter_t /* 192 */}, NULL},
    {{LispCharacter_t /* 193 */}, NULL},
    {{LispCharacter_t /* 194 */}, NULL},
    {{LispCharacter_t /* 195 */}, NULL},
    {{LispCharacter_t /* 196 */}, NULL},
    {{LispCharacter_t /* 197 */}, NULL},
    {{LispCharacter_t /* 198 */}, NULL},
    {{LispCharacter_t /* 199 */}, NULL},
    {{LispCharacter_t /* 200 */}, NULL},
    {{LispCharacter_t /* 201 */}, NULL},
    {{LispCharacter_t /* 202 */}, NULL},
    {{LispCharacter_t /* 203 */}, NULL},
    {{LispCharacter_t /* 204 */}, NULL},
    {{LispCharacter_t /* 205 */}, NULL},
    {{LispCharacter_t /* 206 */}, NULL},
    {{LispCharacter_t /* 207 */}, NULL},
    {{LispCharacter_t /* 208 */}, NULL},
    {{LispCharacter_t /* 209 */}, NULL},
    {{LispCharacter_t /* 210 */}, NULL},
    {{LispCharacter_t /* 211 */}, NULL},
    {{LispCharacter_t /* 212 */}, NULL},
    {{LispCharacter_t /* 213 */}, NULL},
    {{LispCharacter_t /* 214 */}, NULL},
    {{LispCharacter_t /* 215 */}, NULL},
    {{LispCharacter_t /* 216 */}, NULL},
    {{LispCharacter_t /* 217 */}, NULL},
    {{LispCharacter_t /* 218 */}, NULL},
    {{LispCharacter_t /* 219 */}, NULL},
    {{LispCharacter_t /* 210 */}, NULL},
    {{LispCharacter_t /* 221 */}, NULL},
    {{LispCharacter_t /* 222 */}, NULL},
    {{LispCharacter_t /* 223 */}, NULL},
    {{LispCharacter_t /* 224 */}, NULL},
    {{LispCharacter_t /* 225 */}, NULL},
    {{LispCharacter_t /* 226 */}, NULL},
    {{LispCharacter_t /* 227 */}, NULL},
    {{LispCharacter_t /* 228 */}, NULL},
    {{LispCharacter_t /* 229 */}, NULL},
    {{LispCharacter_t /* 230 */}, NULL},
    {{LispCharacter_t /* 231 */}, NULL},
    {{LispCharacter_t /* 232 */}, NULL},
    {{LispCharacter_t /* 233 */}, NULL},
    {{LispCharacter_t /* 234 */}, NULL},
    {{LispCharacter_t /* 235 */}, NULL},
    {{LispCharacter_t /* 236 */}, NULL},
    {{LispCharacter_t /* 237 */}, NULL},
    {{LispCharacter_t /* 238 */}, NULL},
    {{LispCharacter_t /* 239 */}, NULL},
    {{LispCharacter_t /* 240 */}, NULL},
    {{LispCharacter_t /* 241 */}, NULL},
    {{LispCharacter_t /* 242 */}, NULL},
    {{LispCharacter_t /* 243 */}, NULL},
    {{LispCharacter_t /* 244 */}, NULL},
    {{LispCharacter_t /* 245 */}, NULL},
    {{LispCharacter_t /* 246 */}, NULL},
    {{LispCharacter_t /* 247 */}, NULL},
    {{LispCharacter_t /* 248 */}, NULL},
    {{LispCharacter_t /* 249 */}, NULL},
    {{LispCharacter_t /* 250 */}, NULL},
    {{LispCharacter_t /* 251 */}, NULL},
    {{LispCharacter_t /* 252 */}, NULL},
    {{LispCharacter_t /* 253 */}, NULL},
    {{LispCharacter_t /* 254 */}, NULL},
    {{LispCharacter_t /* 255 */}, NULL}
};

Atom_id Sand, Sor, Snot;

static LispObj lispdot = {LispNil_t};
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
	    for (ch = LispGet(mac); ch != EOF && ch != '\n'; ch = LispGet(mac)) {
		if (!isspace(ch)) {
		    LispUnget(mac, ch);
		    break;
		}
	    }
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
	    /* Don't "link" EOLIST to COD, this may happen if
	     * a multiline comment is the last token in a form. */
	    if (INVALID_P(object))
		return (object);
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
    GC_ENTER();
    LispObj *result, *cons, *object;
    int dot = 0;

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
    GC_PROTECT(result);

    while ((object = LispRead(mac)) != EOLIST) {
	if (object == NULL)
	    LispDestroy(mac, "READ: unexpected end of input");
	if (object == DOT) {
	    /* this is a dotted list */
	    if (dot) {
		if (!mac->discard)
		    LispDestroy(mac, "READ: more than one . in list");
		GC_LEAVE();

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
	if (!mac->discard)
	    LispDestroy(mac, "READ: illegal end of dotted list");
	GC_LEAVE();

	return (NIL);
    }

    GC_LEAVE();

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
	if (pack == mac->key)
	    object = KEYWORD(LispDoGetAtom(mac, symbol, 0)->string);
	else
	    object = ATOM(symbol);
	if (unreadable)
	    object->data.atom->unreadable = 1;

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
	number = SMALLINT(integer);

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
	if (CAR(function) != Olambda)
	    LispDestroy(mac, "READ: %s is not a valid lambda", STROBJ(function));

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
	else if (AtomSeparator(ch, 0, 1)) {
	    LispUnget(mac, ch);
	    break;
	}
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
	    if (mac->discard)
		return (NIL);
	    LispDestroy(mac, "READ: unkwnown character %s", stk);
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
	if (ch == '#') {
	    ch = LispGet(mac);
	    if (ch == '|')
		++comm;
	    continue;
	}
	while (ch == '|') {
	    ch = LispGet(mac);
	    if (ch == '#' && --comm == 0)
		return;
	}
	if (ch == EOF)
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
    GC_ENTER();
    LispObj *number, *arguments = LispRead(mac);

    /* form read */
    if (mac->discard)
	return (arguments);

    if (INVALID_P(arguments) || !CONS_P(arguments))
	LispDestroy(mac, "READ: invalid complex-number specification");

    GC_PROTECT(arguments);
    number = APPLY(Ocomplex, arguments);
    GC_LEAVE();

    return (number);
}

static LispObj *
LispReadPathname(LispMac *mac)
{
    GC_ENTER();
    LispObj *path = LispRead(mac);

    /* form read */
    if (mac->discard)
	return (path);

    if (INVALID_P(path))
	LispDestroy(mac, "READ: invalid pathname specification");

    GC_PROTECT(path);
    path = APPLY1(Oparse_namestring, path);
    GC_LEAVE();

    return (path);
}

static LispObj *
LispReadStruct(LispMac *mac)
{
    GC_ENTER();
    int len;
    char stk[128], *str;
    LispObj *struc, *arguments = LispRead(mac);

    /* form read */
    if (mac->discard)
	return (arguments);

    if (INVALID_P(arguments) || !CONS_P(arguments) || !SYMBOL_P(CAR(arguments)))
	LispDestroy(mac, "READ: invalid structure specification");

    GC_PROTECT(arguments);

    len = strlen(STRPTR(CAR(arguments)));
	   /* MAKE- */
    if (len + 6 > sizeof(stk))
	str = LispMalloc(mac, len + 6);
    else
	str = stk;
    sprintf(str, "MAKE-%s", STRPTR(CAR(arguments)));
    RPLACA(arguments, ATOM(str));
    if (str != stk)
	LispFree(mac, str);
    struc = APPLY(Omake_struct, arguments);
    GC_LEAVE();

    return (struc);
}

static LispObj *
LispReadArray(LispMac *mac, long dimensions)
{
    GC_ENTER();
    long count;
    LispObj *arguments, *initial, *dim, *cons;
    LispObj *data = LispRead(mac);

    /* form read */
    if (mac->discard)
	return (data);

    if (INVALID_P(data))
	LispDestroy(mac, "READ: invalid array specification");

    initial = Kinitial_contents;

    dim = cons = NIL;
    if (dimensions) {
	LispObj *array;

	for (count = 0, array = data; count < dimensions; count++) {
	    long length;
	    LispObj *item;

	    if (!CONS_P(array))
		LispDestroy(mac, "READ: bad array for given dimension");
	    item = array;
	    array = CAR(array);

	    for (length = 0; CONS_P(item); item = CDR(item), length++)
		;

	    if (dim == NIL) {
		dim = cons = CONS(SMALLINT(length), NIL);
		GC_PROTECT(dim);
	    }
	    else {
		RPLACD(cons, CONS(SMALLINT(length), NIL));
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
    for (object = FEATURES; CONS_P(object); object = CDR(object))
	if (ATOMID(CAR(object)) == test)
	    return (T);

    /* unknown feature */
    return (NIL);
}

/*
 * Copyright (c) 2001 by The XFree86 Project, Inc.
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

/* $XFree86: xc/programs/xedit/lisp/format.c,v 1.1 2001/09/09 23:03:47 paulo Exp $ */

#include "format.h"
#include <ctype.h>
#include <math.h>

/*
 * Initialization
 */
static char *BadArgument = "bad argument to directive, at %s";
/* not very descriptive... */

/*
 * Implementation
 */
LispObj *
Lisp_Format(LispMac *mac, LispObj *list, char *fname)
{
    int len, princ = mac->princ, newline = mac->newline;
    char *fmt, stk[512];
    LispObj *stream, *format, *arguments, *arg, *plural;

    stream = CAR(list);
    list = CDR(list);
    if ((format = CAR(list))->type != LispString_t)
	LispDestroy(mac, "expecting string, at %s", fname);
    list = CDR(list);
    arg = arguments = list;

    if (stream == NIL) {
	stream = LispNew(mac, NIL, NIL);
	stream->type = LispStream_t;
	stream->data.stream.source.str = NULL;
	stream->data.stream.size = 0;
	stream->data.stream.idx = 0;
    }
    else if (stream == T)
	stream = NIL;
    else if (stream->type != LispStream_t)
	LispDestroy(mac, "%s is not a stream, at %s",
		    LispStrObj(mac, stream), fname);

    plural = NIL;
    len = 0;
    fmt = format->data.atom;
    while (*fmt) {
	if (*fmt == '~') {
	    char *end;
	    int radix = 10, mincol = 0, minpad = 0, colinc = 1, padchar = ' ';
	    int argc, nargs[7], defs[7], padidx, isradix, isprinc, ise;
	    int atsign = 0, collon = 0;
	    int w = 0, d = 0, e = 1, k = 0, overidx, overflowchar = 0,
		expidx, exponentchar = 'e';
					/* float parameters */

	    if (len) {
		stk[len] = '\0';
		LispPrintf(mac, stream, "%s", stk);
		len = 0;
	    }
	    ++fmt;
	    argc = 0;
	    padidx = overidx = expidx = -1;	/* minimal error check */
	    while (1) {
		if (*fmt == ',') {	/* use default value */
		    ++fmt;
		    defs[argc] = 1;
		}
		else if (*fmt == '-' || *fmt == '+' ||
			 isdigit(*fmt)) {	/* mincol specified */
		    nargs[argc] = strtol(fmt, &end, 10);
		    defs[argc] = 0;
		    fmt = end;
		    if (*fmt == ',')	/* more args */
			++fmt;
		}
		else if (*fmt == '\'') {	/* use default value */
		    ++fmt;
		    if (overidx != 1)
			expidx = argc;
		    else {
			overidx = padidx;
			padidx = argc;
		    }
		    nargs[argc] = *fmt++;
		    defs[argc] = 0;
		    if (*fmt == ',')	/* more args */
			++fmt;
		}
		else if (*fmt == ':') {
		    ++fmt;
		    collon = 1;
		    continue;
		}
		else if (*fmt == '@') {
		    ++fmt;
		    atsign = 1;
		    continue;
		}
		else
		    break;
		if (++argc > sizeof(nargs) / sizeof(nargs[0]))
		    LispDestroy(mac, "too many arguments to directive, at %s",
				fname);
	    }

	    switch (*fmt) {
		case 'a':	/* Ascii */
		case 'A':
		    isprinc = 1;
		    goto print_object;
		case 'b':	/* Binary */
		case 'B':
		    isradix = 0;
		    radix = 2;
		    goto print_number;
		case 's':	/* S-expression */
		case 'S':
		    isprinc = 0;
		    goto print_object;
		case 'd':	/* Decimal */
		case 'D':
		    isradix = 0;
		    radix = 10;
		    goto print_number;
		case 'o':	/* Octal */
		case 'O':
		    isradix = 0;
		    radix = 8;
		    goto print_number;
		case 'x':	/* Hexadecimal */
		case 'X':
		    isradix = 0;
		    radix = 16;
		    goto print_number;
		case 'r':	/* Radix */
		case 'R':
		    isradix = 1;
		    goto print_number;
		case 'p':	/* Plural */
		case 'P':
		    mac->newline = 0;
		    if (collon && plural != NIL)
			arg = plural;
		    if (CAR(plural)->type != LispReal_t ||
			CAR(plural)->data.real != 1.0) {
			if (atsign)
			    LispPrintf(mac, stream, "ies");
			else
			    LispPrintf(mac, stream, "s");
		    }
		    else {
			if (atsign)
			    LispPrintf(mac, stream, "y");
		    }
		    break;
		case 'f':	/* Floating-point */
		case 'F':
		    ise = 0;
		    goto print_float_number;
		case 'e':	/* Exponential floating-point */
		case 'E':
		    ise = 1;
		    goto print_float_number;
		case '&':
		    if (mac->newline)
			len = -1;
		case '%':
		    if (argc && !defs[0])
			len += nargs[0];
		    else
			len += 1;
		    if (padidx >= 0)
			LispDestroy(mac, BadArgument, fname);
		    mac->newline = 1;
		    while (len > 0) {
			LispPrintf(mac, stream, "\n");
			--len;
		    }
		    len = 0;
		    ++fmt;
		    /* no arguments used */
		    continue;
		case '~':
		    if (argc && !defs[0])
			len = nargs[0];
		    else
			len = 1;
		    if (padidx >= 0)
			LispDestroy(mac, BadArgument, fname);
		    while (len) {
			LispPrintf(mac, stream, "~");
			--len;
		    }
		    ++fmt;
		    /* no arguments used */
		    continue;
		case '|':
		    if (argc && !defs[0])
			len = nargs[0];
		    else
			len = 1;
		    if (padidx >= 0)
			LispDestroy(mac, BadArgument, fname);
		    while (len) {
			LispPrintf(mac, stream, "\f");
			--len;
		    }
		    ++fmt;
		    /* no arguments used */
		    continue;
		default:
		    LispDestroy(mac, "unknown directive %c, at %s",
				*fmt, fname);
	    }
	    ++fmt;
	    plural = arg;
	    arg = CDR(arg);
	    continue;

print_number:
	    mac->newline = 0;
	    if (arg == NIL)
		goto not_enough_args;
	    if (CAR(arg)->type != LispReal_t) {
		/* print just as 'A' */
		isprinc = 1;
		goto print_object;
	    }
	    else {
		int sign;
		long num = (long)CAR(arg)->data.real;

		if ((double)num != CAR(arg)->data.real)
		    LispDestroy(mac, BadArgument, fname);

		len = 0;
		if ((sign = num < 0) != 0)
		    num = -num;

		/* check for radix */
		if (isradix) {
		    if (argc == 0 || defs[0]) {
			radix = 0;
			++len;
			goto print_number_args;
		    }
		    radix = nargs[0];
		    ++len;
		}
		if (radix < 2 || radix > 32)
		    LispDestroy(mac, "radix must be in the range 2 to 32,"
				" at %s", fname);

print_number_args:
		/* get print arguments */
		if (len < argc && !defs[len])
		    mincol = nargs[len];
		++len;
		if (len < argc && !defs[len])
		    padchar = nargs[len];

		if (padidx >= 0 && padidx != len)
		    LispDestroy(mac, BadArgument, fname);

		if (radix) {
		    len = 0;
		    do {
			int val;

			val = num % radix;
			num -= val;
			num /= radix;
			if (len)
			    memmove(stk + 1, stk, len);
			*stk = val < 10 ? val + '0' : (val - 10) + 'a';
			++len;
		    } while (num);
		    if (sign) {
			memmove(stk + 1, stk, len);
			*stk = '-';
			++len;
		    }
		}
		else if (atsign) {	/* roman */
		    long num = (long)CAR(arg)->data.real;

		if ((double)num != CAR(arg)->data.real ||
		    num <= 0 || num > (3999 + (collon ? 1000 : 0)))
		    LispDestroy(mac, BadArgument, fname);

		    /* if collon, print in old roman format */
		    len = 0;
		    while (num > 1000) {
			stk[len++] = 'M';
			num -= 1000;
		    }
		    if (!collon) {
			if (num >= 900) {
			    strcpy(stk + len, "CM");
			    len += 2,
			    num -= 900;
			}
		        else if (num < 500 && num >= 400) {
			    strcpy(stk + len, "CD");
			    len += 2;
			    num -= 400;
			}
		    }
		    if (num >= 500) {
			stk[len++] = 'D';
			num -= 500;
		    }
		    while (num >= 100) {
			stk[len++] = 'C';
			num -= 100;
		    }

		    if (!collon) {
			if (num >= 90) {
			    strcpy(stk + len, "XC");
			    len += 2,
			    num -= 90;
			}
			else if (num < 50 && num >= 40) {
			    strcpy(stk + len, "XL");
			    len += 2;
			    num -= 40;
			}
		    }
		    if (num >= 50) {
			stk[len++] = 'L';
			num -= 50;
		    }
		    while (num >= 10) {
			stk[len++] = 'X';
			num -= 10;
		    }

		    if (!collon) {
			if (num == 9) {
			    strcpy(stk + len, "IX");
			    len += 2,
			    num -= 9;
	 		}
			else if (num == 4) {
			    strcpy(stk + len, "IV");
			    len += 2;
			    num -= 4;
			}
		    }
		    if (num >= 5) {
			stk[len++] = 'V';
			num -= 5;
		    }
		    while (num) {
			stk[len++] = 'I';
			num -= 1;
		    }
		}
		else {			/* english */
		    len = 0;
#define SIGNLEN		6		/* strlen("minus ") */
		    if (sign) {
			strcpy(stk, "minus ");
			len += SIGNLEN;
		    }
		    else if (num == 0) {
			if (collon) {
			    strcpy(stk, "zeroth");
			    len += 6;  /*123456*/
			}
			else {
			    strcpy(stk, "zero");
			    len += 4;  /*1234*/
			}
		    }
		    while (1) {
			char *d, *h, *t;
			int l, count = 0, tmp;
			long val = num;
			static char *ds[] = {
			    "",         "one",      "two",        "three",
			    "four",     "five",     "six",        "seven",
			    "eight",    "nine",      "ten",       "eleven",
			    "twelve",   "thirteen",  "fourteen",  "fifteen",
			    "sixteen",  "seventeen",  "eighteen", "nineteen"
			};
			static char *dsth[] = {
			    "",           "first",      "second",
			    "third",      "fourth",     "fifth",
			    "sixth",      "seventh",    "eighth",
			    "ninth",      "tenth",      "eleventh",
			    "twelfth",    "thirteenth", "fourteenth",
			    "fifteenth",  "sixteenth",  "seventeenth",
			    "eighteenth", "nineteenth"
			};
			static char *hs[] = {
			    "",      "",      "twenty",  "thirty", "forty",
			    "fifty", "sixty", "seventy", "eighty", "ninety"
			};
			static char *hsth[] = {
			    "",          "",         "twentieth",  "thirtieth",
			    "fortieth",  "fiftieth", "sixtieth",   "seventieth",
			    "eightieth", "ninetieth"
			};
			static char *ts[] = {
			    "", "thousand", "million", "billion", "trillion"
			};
			static char *tsth[] = {
			    "",          "thousandth", "millionth", "billionth",
			    "trillionth"
			};

			while (val >= 1000) {
			    val /= 1000;
			    ++count;
			}
			if (count > sizeof(ts) / sizeof(ts[0]))
			    LispDestroy(mac, "format is too large, at %s",
					fname);

			t = ds[val / 100];
			if (collon && !count && (val % 10) == 0)
			    h = hsth[(val % 100) / 10];
			else
			    h = hs[(val % 100) / 10];

			if (collon && !count)
			    d = *h ? dsth[val % 10] : dsth[val % 20];
			else
			    d = *h ? ds[val % 10] : ds[val % 20];

			if (((!sign && len) || len > SIGNLEN) &&
			    (*t || *h || *d)) {
			    if (!collon || count || *h || *t) {
				strcpy(stk + len, ", ");
				len += 2;
			    }
			    else {
				strcpy(stk + len, " ");
				++len;
			    }
			}
			if (*t) {
			    if (collon && !count && (val % 100) == 0)
				l = sprintf(stk + len, "%s hundredth", t);
			    else
				l = sprintf(stk + len, "%s hundred", t);
			    len += l;
			}
			if (*h) {
			    if (*t) {
				if (collon && !count) {
				    strcpy(stk + len, " ");
				    ++len;
				}
				else {
				    strcpy(stk + len, " and ");
				    len += 5;        /*12345*/
				}
			    }
			    l = sprintf(stk + len, "%s", h);
			    len += l;
			}
			if (*d) {
			    if (*h) {
				strcpy(stk + len, "-");
				++len;
			    }
			    else if (*t) {
				if (collon && !count) {
				    strcpy(stk + len, " ");
				    ++len;
				}
				else {
				    strcpy(stk + len, " and ");
				    len += 5;        /*12345*/
				}
			    }
			    l = sprintf(stk + len, "%s", d);
			    len += l;
			}
			if (!count)
			    break;
			else
			    tmp = count;
			if (count > 1) {
			    val *= 1000;
			    while (--count)
				val *= 1000;
			    num -= val;
			}
			else
			    num %= 1000;

			if (collon && num == 0 && !*t && !*h)
			    l = sprintf(stk + len, " %s", tsth[tmp]);
			else
			    l = sprintf(stk + len, " %s", ts[tmp]);
			len += l;

			if (num == 0)
			    break;
		    }
		}

		stk[len] = '\0';
		while (mincol > len) {
		    LispPrintf(mac, stream, "%c", padchar);
		    --mincol;
		}
		LispPrintf(mac, stream, "%s", stk);
		len = 0;
	    }
	    ++fmt;
	    plural = arg;
	    arg = CDR(arg);
	    continue;

print_float_number:
	    mac->newline = 0;
	    if (arg == NIL)
		goto not_enough_args;
	    if (CAR(arg)->type != LispReal_t) {
		/* print just as 'A' */
		isprinc = 1;
		goto print_object;
	    }
	    else {
		double num = CAR(arg)->data.real;
		char sprint[64];
		int l, sign, expt = 0, elen = 1;

		/* get print arguments */
		l = 0;
		if (argc && !defs[l])
		    w = nargs[l];
		++l;
		if (argc > l && !defs[l])
		    d = nargs[l];
		++l;
		if (ise) {
		    if (argc > l && !defs[l])
			e = nargs[l];
		    ++l;
		}
		if (argc > l && !defs[l])
		    k = nargs[l];
		++l;
		if (argc > l && !defs[l])
		    overflowchar = nargs[l];
		++l;
		if (argc > l && !defs[l])
		    padchar = nargs[l];
		++l;
		if (argc > l && !defs[l])
		    exponentchar = nargs[l];

		if (overidx == -1 && padidx != -1) {
		    overidx = padidx;
		    padidx = -1;
		}
		if (k >= 64 || k <= -64 || (argc > 2 && !defs[2] && w < 2) ||
		    (argc > 1 && !defs[1] && d < 0) ||
		    (overidx != -1 && (overidx != 3 + ise)) ||
		    (padidx != -1 && (padidx != 4 + ise)))
		    LispDestroy(mac, BadArgument, fname);

		if (ise) {
		    if (k > 0 && d) {
			if ((d -= (k - 1)) < 0)
			    d = 0;	/* XXX this is an error */
		    }
		    else if (k < 0 && -k > d)
			k = 0;		/* XXX this is an error */
		}

		sign = num < 0.0;
		if (sign)
		    num = -num;
		len = 0;
		if (sign)
		    stk[len++] = '-';
		else if (atsign)
		    stk[len++] = '+';

		/* adjust scale factor/exponent */
		l = k;
		while (l > 0) {
		    --l;
		    --expt;
		    num *= 10.0;
		}
		while (l < 0) {
		    ++l;
		    ++expt;
		    num /= 10.0;
		}
		if (ise) {
		    if (!k)
			k = 1;
		    if (num > 1.0) {
			l = sprintf(sprint, "%1.1f", num);
			while (l > 1 && sprint[--l] != '.')
			    ;
		    }
		    else {
			int pos;
			char sprint2[64];

			if (d) {
			    sprintf(sprint2, "%%1.%df", d);
			    l = sprintf(sprint, sprint2, num);
			}
			else
			    l = sprintf(sprint, "%f", num);
			for (pos = 0; sprint[pos] && sprint[pos] != '.'; pos++)
			    ;
			if (sprint[pos]) {
			    for (l = 0, pos++; sprint[pos] == '0'; pos++, l--)
				;
			    if (!sprint[pos])
				l = k;
			}
			else
			    l = k;
		    }
		    while (l > k) {
			--l;
			num /= 10.0;
			++expt;
		    }
		    while (l < k) {
			++l;
			num *= 10.0;
			--expt;
		    }
		}

		if (!d) {
		    int left = 20;
		    double integral, fractional;

		    fractional = modf(num, &integral);
		    if (w) {
			l = sprintf(sprint, "%f", integral);
			while (l > 1 && sprint[l - 1] == '0')
			    --l;
			if (l && sprint[l - 1] == '.')
			    --l;
			left = w - l - 1 - sign;
		    }
		    l = sprintf(sprint, "%f", fractional);
		    while (l && sprint[l - 1] == '0')
			--l;
		    l -= 2 + (w && sign);
		    if (l > left)
			l = left;
		    sprintf(sprint, "%%1.%df", l > 0 ? l : 0);
		}
		else
		    sprintf(sprint, "%%1.%df", d);
		l = sprintf(stk + len, sprint, num);

		len += l;
		if (ise) {
		    l = sprintf(stk + len, "%c%c", exponentchar,
				expt < 0 ? '-' : '+');
		    len += l;
		    if (e)
			sprintf(sprint, "%%0%dd", e);
		    else
			strcpy(sprint, "%d");
		    l = sprintf(stk + len, sprint, expt < 0 ? -expt : expt);
		    len += l;
		    elen = l + 2;	/* sign and exponentchar */
		}
		if (w && len > w) {
		    int tmp;

		    /* cut fractional part to fit in width */
		    l = len;
		    --len;
		    if (ise)
			len -= elen;
		    while (len > w) {
			if (stk[len] == '.')
			    break;
			else
			    --len;
		    }
		    if (d) {
			int gotdigit = 0;

			/* check for overflow in fractional part */
			while (len && stk[len] != '.')
			    --len;
			for (tmp = 0, ++len; (!gotdigit || tmp < d) && len < l;
			     tmp++, len++)
			    if (stk[len] != '0')
				gotdigit = 1;
		    }
		    if (ise) {
			if (l > len + elen)
			    memmove(stk + len, stk + l - elen, elen);
			len += elen;
		    }
		    if (len > w && num < 1) {
			int inc = sign || atsign;

			/* cut the leading '0' */
			memmove(stk + inc, stk + inc + 1, len - inc - 1);
			--len;
		    }
		    if (((ise && elen - 2 > e) || len > w) && overflowchar) {
			for (len = 0; len < w; len++)
			    stk[len] = overflowchar;
		    }
		}
		stk[len] = '\0';
		while (len < w) {
		    LispPrintf(mac, stream, "%c", padchar);
		    ++len;
		}
		LispPrintf(mac, stream, "%s", stk);
		len = 0;
	    }
	    ++fmt;
	    plural = arg;
	    arg = CDR(arg);
	    continue;

print_object:
	    mac->newline = 0;
	    if (arg == NIL)
		goto not_enough_args;

	    if (padidx >= 0 && padidx != 3)
		LispDestroy(mac, BadArgument, fname);

	    /* get print arguments */
	    if (argc && !defs[0])
		mincol = nargs[0];
	    if (argc > 1 && !defs[1])
		colinc = nargs[1];
	    if (argc > 2 && !defs[2])
		minpad = nargs[2];
	    if (argc > 3 && !defs[3])
		padchar = nargs[3];

	    if (atsign) {
		int justsize = mac->justsize;

		mac->justsize = 1;
		len = LispPrintObj(mac, stream, CAR(arg), 1);
		mac->justsize = justsize;
		while (len < mincol) {
		    LispPrintf(mac, stream, "%c", padchar);
		    ++len;
		}
	    }

	    if (isprinc)
		mac->princ = 1;
	    len = LispPrintObj(mac, stream, CAR(arg), 1);
	    if (!atsign) {
		while (len < mincol) {
		    LispPrintf(mac, stream, "%c", padchar);
		    ++len;
		}
	    }
	    if (isprinc)
		mac->princ = princ;
	    len = 0;
	    ++fmt;
	    plural = arg;
	    arg = CDR(arg);
	    continue;

not_enough_args:
	    LispDestroy(mac, "no arguments left, at %s", fname);
	}
	else {
	    mac->newline = 0;
	    if (len + 1 < sizeof(stk))
		stk[len++] = *fmt;
	    else {
		stk[len] = '\0';
		LispPrintf(mac, stream, "%s", stk);
		len = 0;
	    }
	}
	++fmt;
    }
    if (len) {
	stk[len] = '\0';
	LispPrintf(mac, stream, "%s", stk);
    }

    if (stream != NIL && (stream->data.stream.size >= 0 ||
	stream->data.stream.source.fp != lisp_stdout))
	mac->newline = newline;
    else
	fflush(lisp_stdout);

    return (stream);
}

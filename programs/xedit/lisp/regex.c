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

/* $XFree86: xc/programs/xedit/lisp/regex.c,v 1.2 2002/07/22 07:26:28 paulo Exp $ */

#include "regex.h"
#include "private.h"
#include "helper.h"

/*
 * Prototypes
 */
static regex_t *LispRegcomp(LispMac*, LispBuiltin*, char*, int);

/*
 * Initialization
 */
LispObj *Knomatch;

/*
 * Implementation
 */
static regex_t *
LispRegcomp(LispMac *mac, LispBuiltin *builtin, char *pattern, int cflags)
{
    int code;
    regex_t *regex = LispMalloc(mac, sizeof(regex_t));

    if ((code = regcomp(regex, pattern, cflags)) != 0) {
	char buffer[256];

	regerror(code, regex, buffer, sizeof(buffer));
	regfree(regex);
	LispFree(mac, regex);
	LispDestroy(mac, "%s: regcomp(\"%s\"): %s",
		    STRFUN(builtin), pattern, buffer);
    }

    return (regex);
}

void
LispRegexInit(LispMac *mac)
{
    Knomatch = KEYWORD("NOMATCH");
}

LispObj *
Lisp_Regcomp(LispMac *mac, LispBuiltin *builtin)
/*
 regcomp pattern &key (extended t) nospec icase nosub newline
 */
{
    regex_t *regex;
    int cflags = 0;

    LispObj *result;

    LispObj *pattern, *extended, *nospec, *icase, *nosub, *newline;

    newline = ARGUMENT(5);
    nosub = ARGUMENT(4);
    icase = ARGUMENT(3);
    nospec = ARGUMENT(2);
    extended = ARGUMENT(1);
    pattern = ARGUMENT(0);

    /* Don't generate an error if it is already a compiled regex. */
    if (REGEX_P(pattern))
	return (pattern);

    ERROR_CHECK_STRING(pattern);

#ifdef REG_EXTENDED
    if (extended != NIL)
	cflags |= REG_EXTENDED;
#endif
#ifdef REG_NOSPEC
    if (nospec != NIL) {
	cflags &= ~REG_EXTENDED;
	cflags |= REG_NOSPEC;
    }
#endif
#ifdef REG_ICASE
    if (icase != NIL)
	cflags |= REG_ICASE;
#endif
#ifdef REG_NOSUB
    if (nosub != NIL)
	cflags |= REG_NOSUB;
#endif
#ifdef REG_NEWLINE
    if (newline != NIL)
	cflags |= REG_NEWLINE;
#endif

    regex = LispRegcomp(mac, builtin, THESTR(pattern), cflags);
    result = LispNew(mac, pattern, NIL);
    result->type = LispRegex_t;
    result->data.regex.regex = regex;
    result->data.regex.pattern = pattern;
    result->data.regex.options = cflags;
    LispMused(mac, regex);

    return (result);
}

LispObj *
Lisp_Regexec(LispMac *mac, LispBuiltin *builtin)
/*
 regexec regex string &key count start end notbol noteol
 */
{
    GC_ENTER();
    size_t nmatch;
    regmatch_t match[4], *pmatch;
    long start = 0, end, length;
    int code, eflags = 0;
    char *string;
    LispObj *result;
    regex_t *regexp;
#ifndef REG_STARTEND
    char buffer[128];
    int alloced = 0;
#endif

    LispObj *regex, *ostring, *count, *ostart, *oend, *notbol, *noteol;

    noteol = ARGUMENT(6);
    notbol = ARGUMENT(5);
    oend = ARGUMENT(4);
    ostart = ARGUMENT(3);
    count = ARGUMENT(2);
    ostring = ARGUMENT(1);
    regex = ARGUMENT(0);

    if (STRING_P(regex))
	regexp = LispRegcomp(mac, builtin, THESTR(regex),
#ifdef REG_EXTENDED
			     REG_EXTENDED
#else
			     0
#endif
			    );
    else {
	ERROR_CHECK_REGEX(regex);
	regexp = regex->data.regex.regex;
    }

    ERROR_CHECK_STRING(ostring);

    if (count == NIL)
	nmatch = 1;
    else {
	ERROR_CHECK_INDEX(count);
	nmatch = count->data.integer;
    }

#ifdef REG_NOTBOL
    if (notbol != NIL)
	eflags |= REG_NOTBOL;
#endif
#ifdef REG_NOTEOL
    if (noteol != NIL)
	eflags |= REG_NOTEOL;
#endif

    string = THESTR(ostring);
    if (ostart != NIL || oend != NIL) {
	LispCheckSequenceStartEnd(mac, builtin, ostring, ostart, oend,
				  &start, &end, &length);
#ifndef REG_STARTEND
	if (start > 0 || end < length) {
	    if (end == length)
		string += start;
	    else {
		char *pointer;

		length = end - start;
		if (length < sizeof(buffer) - 1)
		    pointer = buffer;
		else {
		    pointer = LispMalloc(mac, length + 1);
		    alloced = 1;
		}
		memcpy(pointer, string + start, length);
		string = pointer;
		string[length] = '\0';
	    }
	}
#endif
    }
#ifdef REG_STARTEND
    else
	end = strlen(string);
#endif

    if (nmatch > sizeof(match) / sizeof(match[0]))
	pmatch = LispMalloc(mac, sizeof(regmatch_t) * nmatch);
    else
	pmatch = (regmatch_t*)match;

#ifdef REG_STARTEND
    eflags |= REG_STARTEND;
    pmatch[0].rm_so = start;
    pmatch[0].rm_eo = end;
#endif
    code = regexec(regexp, string, nmatch, pmatch, eflags);

    if (code == 0) {
	result = NIL;
	if (nmatch && pmatch[0].rm_eo >= pmatch[0].rm_so) {
	    result = CONS(CONS(NIL, NIL), NIL);
	    GC_PROTECT(result);
#ifdef REG_STARTEND
	    CAAR(result) = SMALLINT(pmatch[0].rm_so);
	    CDAR(result) = SMALLINT(pmatch[0].rm_eo);
#else
	    CAAR(result) = SMALLINT(pmatch[0].rm_so + start);
	    CDAR(result) = SMALLINT(pmatch[0].rm_eo + start);
#endif
	    if (nmatch > 1 && pmatch[1].rm_eo >= pmatch[1].rm_so) {
		int i;
		LispObj *cons = result;

		for (i = 1; i < nmatch && pmatch[i].rm_eo > pmatch[i].rm_so; i++) {
		    CDR(cons) = CONS(CONS(NIL, NIL), NIL);
		    cons = CDR(cons);
#ifdef REG_STARTEND
		    CAAR(cons) = SMALLINT(pmatch[i].rm_so);
		    CDAR(cons) = SMALLINT(pmatch[i].rm_eo);
#else
		    CAAR(cons) = SMALLINT(pmatch[i].rm_so + start);
		    CDAR(cons) = SMALLINT(pmatch[i].rm_eo + start);
#endif
		}
	    }
	}
    }
    else
	result = Knomatch;

    if (pmatch != match)
	LispFree(mac, match);

#ifndef REG_STARTEND
    if (alloced)
	LispFree(mac, string);
#endif
    if (!REGEX_P(regex)) {
	regfree(regexp);
	LispFree(mac, regexp);
    }

    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Regexp(LispMac *mac, LispBuiltin *builtin)
/*
 regexp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (REGEX_P(object) ? T : NIL);
}

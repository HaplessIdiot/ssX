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

/* $XFree86: xc/programs/xedit/lisp/helper.c,v 1.14 2001/10/20 00:19:34 paulo Exp $ */

#include "helper.h"
#include "pathname.h"
#include "read.h"
#include "stream.h"
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>

/*
 * Prototypes
 */
static LispObj *LispReallyDo(LispMac*, LispBuiltin*, int);
static LispObj *LispReallyDoListTimes(LispMac*, LispBuiltin*, int);

/*
 * Implementation
 */
LispObj *
LispEqual(LispMac *mac, LispObj *left, LispObj *right)
{
    LispObj *result = NIL;

    if (left->type == right->type) {
	switch (left->type) {
	    case LispNil_t:
	    case LispTrue_t:
		result = T;
		break;
	    case LispReal_t:
		if (left->data.real == right->data.real)
		    result = T;
		break;
	    case LispCharacter_t:
	    case LispInteger_t:
		if (left->data.integer == right->data.integer)
		    result = T;
		break;
	    case LispAtom_t:
	    case LispString_t:
		if (STRPTR(left) == STRPTR(right))
		    result = T;
		break;
	    case LispCons_t:
		if (LispEqual(mac, CAR(left), CAR(right)) == T &&
		    LispEqual(mac, CDR(left), CDR(right)) == T)
		    result = T;
		break;
	    case LispQuote_t:
	    case LispKeyword_t:
	    case LispBackquote_t:
	    case LispPathname_t:
		result = LispEqual(mac, left->data.quote, right->data.quote);
		break;
	    case LispLambda_t:
		result = LispEqual(mac, CAR(left->data.lambda.name),
				 CAR(right->data.lambda.name));
		break;
	    case LispOpaque_t:
		if (left->data.opaque.data == right->data.opaque.data)
		    result = T;
		break;
	    case LispBigInteger_t:
		if (mpi_cmp(left->data.mp.integer, right->data.mp.integer) == 0)
		    result = T;
		break;
	    case LispBigRatio_t:
		if (mpr_cmp(left->data.mp.ratio, right->data.mp.ratio) == 0)
		    result = T;
		break;
	    default:
		if (left == right)
		    result = T;
		break;
	}
    }

    return (result);
}

static LispObj *
LispReallyDo(LispMac *mac, LispBuiltin *builtin, int refs)
/*
 do init test &rest body
 do* init test &rest body
 */
{
    int length = mac->protect.length;
    LispObj *result, *init, *test, *body, *object, *list, *env;

    body = ARGUMENT(2);
    test = ARGUMENT(1);
    init = ARGUMENT(0);
    MACRO_ARGUMENT3();

    env = result = NIL;

    if (!CONS_P(test))
	LispDestroy(mac, "%s: end test condition must be a list, not %s",
		    STRFUN(builtin), STROBJ(init));

    /* Add variables */
    if (init != NIL && !CONS_P(init))
	LispDestroy(mac, "%s: %s is not a list",
		    STRFUN(builtin), STROBJ(init));

    for (object = init; CONS_P(object); object = CDR(object)) {
	LispObj *symbol, *value, *step;

	symbol = value = NIL;
	step = NULL;
	list = CAR(object);
	if (SYMBOL_P(list))
	    symbol = list;
	else if (!CONS_P(list))
		LispDestroy(mac, "%s: %s is not a list",
			    STRFUN(builtin), STROBJ(list));
	else {
	    if (!SYMBOL_P(symbol = CAR(list)))
		LispDestroy(mac, "%s: %s is not a symbol",
			    STRFUN(builtin), STROBJ(symbol));
	    if ((list = CDR(list)) != NIL) {
		if (NCONSTANT_P(value = CAR(list)))
		    value = EVAL(value);
		if ((list = CDR(list)) != NIL)
		    step = CAR(list);
	    }
	}
	GCProtect();
	if (step)
	    list = CONS(symbol, CONS(value, CONS(step, NIL)));
	else
	    list = CONS(symbol, CONS(value, NIL));
	if (env == NIL) {
	    env = CONS(list, NIL);
	    if (length + 1 >= mac->protect.space)
		LispMoreProtects(mac);
	    mac->protect.objects[mac->protect.length++] = env;
	}
	else {
	    CDR(env) = CONS(CAR(env), CDR(env));
	    CAR(env) = list;
	}
	GCUProtect();
	if (refs) {
	    LispAddVar(mac, symbol, value);
	    mac->env.head += 2;
	}
    }

    env = mac->protect.objects[length] = LispReverse(env);
    if (!refs) {
	for (object = env; object != NIL; object = CDR(object)) {
	    list = CAR(object);
	    LispAddVar(mac, CAR(list), CAR(CDR(list)));
	    mac->env.head += 2;
	}
    }

    /* Execute iterations */
    for (;;) {
	if (EVAL(CAR(test)) != NIL) {
	    if (CDR(test) != NIL)
		result = EVAL(CAR(CDR(test)));
	    break;
	}
	for (object = body; CONS_P(object); object = CDR(object))
	    (void)EVAL(CAR(object));
	/* Update variables */
	for (object = env; object != NIL; object = CDR(object)) {
	    list = CAR(object);
	    if (CDR(CDR(list)) != NIL)
		LispSetVar(mac, CAR(list),
			   EVAL(CAR(CDR(CDR(list)))));
	}
    }

    mac->protect.length = length;

    return (result);
}

LispObj *
LispDo(LispMac *mac, LispBuiltin *builtin, int refs)
/*
 do init test &rest body
 do* init test &rest body
 */
{
    int did_jump, *pdid_jump = &did_jump;
    LispObj *res, **pres = &res;
    LispBlock *block;

    *pres = NIL;
    *pdid_jump = 1;
    block = LispBeginBlock(mac, NIL, LispBlockTag);
    if (setjmp(block->jmp) == 0) {
	*pres = LispReallyDo(mac, builtin, refs);
	*pdid_jump = 0;
    }
    LispEndBlock(mac, block);
    if (*pdid_jump)
	*pres = mac->block.block_ret;

    return (*pres);
}

static LispObj *
LispReallyDoListTimes(LispMac *mac, LispBuiltin *builtin, int times)
/*
 dolist init &rest body
 dotimes init &rest body
 */
{
    int length = mac->protect.length;
    long count = 0, end;
    LispObj *symbol, *value = NIL, *result = NIL, *init, *body, *object;

    end = 0;	/* fix gcc warning */

    body = ARGUMENT(1);
    init = ARGUMENT(0);
    MACRO_ARGUMENT2();

    /* Parse arguments */
    if (!CONS_P(init))
	LispDestroy(mac, "%s: %s is not a list",
		    STRFUN(builtin), STROBJ(init));
    if (!SYMBOL_P(symbol = CAR(init)))
	LispDestroy(mac, "%s: %s is not a symbol",
		    STRFUN(builtin), STROBJ(symbol));
    init = CDR(init);

    if (init == NIL) {
	if (!times)
	    value = result = NIL;
	else
	    LispDestroy(mac, "%s: NIL is not a number", STRFUN(builtin));
    }
    else {
	if (CONS_P(init)) {
	    value = CAR(init);
	    init = CDR(init);
	    if (init == NIL)
		result = NIL;
	    else if (CONS_P(init))
		result = CAR(init);
	    else
		LispDestroy(mac, "%s: %s is not a list",
			    STRFUN(builtin), STROBJ(init));
	}
	else
	    LispDestroy(mac, "%s: %s is not a list",
			STRFUN(builtin), STROBJ(value));

	if (NCONSTANT_P(value))
	    value = EVAL(value);

	if (times) {
	    if (!INT_P(value)) {
		if (INTEGRAL_P(value))
		    end = FIXNUM_VALUE(value);
		else
		    LispDestroy(mac, "%s: %s is not an integer",
				STRFUN(builtin), STROBJ(value));
	    }
	    else
		end = value->data.integer;
	}
	else if (value != NIL && !CONS_P(value))
	    LispDestroy(mac, "%s: %s is not a list",
			STRFUN(builtin), STROBJ(value));
    }

    /* Protect iteration control from gc */
    if (length + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = value;

    /* Initialize counter */
    if (times)
	LispAddVar(mac, symbol, INTEGER(count));
    else
	LispAddVar(mac, symbol, CAR(value));
    mac->env.head += 2;

    /* Execute iterations */
    for (;;) {
	/* Check loop */
	if (times) {
	    if ((count += 1) > end)
		break;
	}
	else if (value == NIL)
	    break;

	for (object = body; CONS_P(object); object = CDR(object))
	    (void)EVAL(CAR(object));

	/* Update symbols */
	if (times)
	    LispSetVar(mac, symbol, INTEGER(count));
	else {
	    value = CDR(value);
	    if (value == NIL)
		break;
	    else if (!CONS_P(value))
		LispDestroy(mac, "%s: NIL terminated list required, not %s",
			    STRFUN(builtin), STROBJ(value));
	    LispSetVar(mac, symbol, CAR(value));
	}
    }

    mac->protect.length = length;

    return (result == NIL ? NIL : EVAL(result));
}

LispObj *
LispDoListTimes(LispMac *mac, LispBuiltin *builtin, int times)
/*
 dolist init &rest body
 dotimes init &rest body
 */
{
    int did_jump, *pdid_jump = &did_jump;
    LispObj *res, **pres = &res;
    LispBlock *block;

    *pres = NIL;
    *pdid_jump = 1;
    block = LispBeginBlock(mac, NIL, LispBlockTag);
    if (setjmp(block->jmp) == 0) {
	*pres = LispReallyDoListTimes(mac, builtin, times);
	*pdid_jump = 0;
    }
    LispEndBlock(mac, block);
    if (*pdid_jump)
	*pres = mac->block.block_ret;

    return (*pres);
}

LispObj *
LispSet(LispMac *mac, LispObj *var, LispObj *val, char *fname, int eval)
{
    char *name;

    if (!SYMBOL_P(var))
	LispDestroy(mac, "%s: %s is not a symbol", fname, STROBJ(var));

    name = STRPTR(var);
    if (isdigit(name[0]) || name[0] == '(' || name[0] == ')'
	|| name[0] == ';' || name[0] == '\'' || name[0] == '#')
	LispDestroy(mac, "bad name %s, at %s", name, fname);
    if (eval && !CONSTANT_P(val))
	val = EVAL(val);

    return (LispSetVar(mac, var, val));
}

LispObj *
LispLoadFile(LispMac *mac, LispObj *filename,
	     int verbose, int print, int ifdoesnotexist)
{
    LispObj *stream, *eval, *ext, *obj, *result = NIL;
    int ch, eof = mac->eof, length = mac->protect.length;

    if (verbose)
	LispMessage(mac, "; Loading %s", STRPTR(filename));

    GCProtect();
    ext = ifdoesnotexist ? KEYWORD(SYMBOL(mac->error_atom)) : NIL;
    eval = CONS(SYMBOL(mac->open_atom),
		CONS(filename,
		     CONS(KEYWORD(SYMBOL(mac->if_does_not_exist_atom)),
			  CONS(ext, NIL))));
    stream = EVAL(eval);
    GCUProtect();

    if (stream == NIL)
	return (NIL);

    LispPushInput(mac, stream);
    ch = LispGet(mac);
    if (ch != '#')
	LispUnget(mac, ch);
    else if ((ch = LispGet(mac)) == '!') {
	for (;;) {
	    ch = LispGet(mac);
	    if (ch == '\n' || ch == EOF)
		break;
	}
    }
    else {
	LispUnget(mac, ch);
	LispUnget(mac, '#');
    }

    if (mac->protect.length + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = NIL;
    /*CONSTCOND*/
    while (1) {
	if ((obj = LispRead(mac)) != NULL) {
	    if (obj == EOLIST)
		LispDestroy(mac, "object cannot start with #\\)");
	    mac->protect.objects[length] = obj;
	    result = EVAL(obj);
	    if (print)
		LispPrint(mac, result, NIL, 1);
	}
	if (mac->eof)
	    break;
    }
    LispPopInput(mac, stream);
    mac->eof = eof;
    mac->protect.length = length;

    GCProtect();
    eval = CONS(SYMBOL(mac->close_atom), CONS(stream, NIL));
    EVAL(eval);
    GCUProtect();

    return (result);
}

void
LispGetStringArgs(LispMac *mac, LispBuiltin *builtin,
		  char **string1, char **string2,
		  int *start1, int *end1, int *start2, int *end2)
{
    int length1, length2;
    LispObj *ostring1, *ostring2, *ostart1, *oend1, *ostart2, *oend2;

    oend2 = ARGUMENT(5);
    ostart2 = ARGUMENT(4);
    oend1 = ARGUMENT(3);
    ostart1 = ARGUMENT(2);
    ostring2 = ARGUMENT(1);
    ostring1 = ARGUMENT(0);

    length1 = length2 = 0;	/* fix gcc warning */

    if (!STRING_P(ostring1) && !SYMBOL_P(ostring1))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(ostring1));
    else {
	*string1 = STRPTR(ostring1);
	length1 = strlen(*string1);
    }

    if (!STRING_P(ostring2) && !SYMBOL_P(ostring2))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(ostring2));
    else {
	*string2 = STRPTR(ostring2);
	length2 = strlen(*string2);
    }

    if (ostart1 == NIL)
	*start1 = 0;
    else if (!INDEX_P(ostart1))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(ostart1));
    else
	*start1 = ostart1->data.integer;

    if (oend1 == NIL)
	*end1 = length1;
    else if (!INDEX_P(oend1))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(oend1));
    else
	*end1 = oend1->data.integer;

    if (ostart2 == NIL)
	*start2 = 0;
    else if (!INDEX_P(ostart2))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(ostart2));
    else
	*start2 = ostart2->data.integer;

    if (oend2 == NIL)
	*end2 = length2;
    else if (!INDEX_P(oend2))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(oend2));
    else
	*end2 = oend2->data.integer;

    if (*start1 > *end1)
	LispDestroy(mac, "%s: :START1 %d larger than :END1 %d",
		    STRFUN(builtin), *start1, *end1);
    if (*start2 > *end2)
	LispDestroy(mac, "%s: :START2 %d larger than :END2 %d",
		    STRFUN(builtin), *start2, *end2);
    if (*end1 > length1)
	LispDestroy(mac, "%s: :END1 %d larger than string length %d",
		    STRFUN(builtin), *end1, length1);
    if (*end2 > length2)
	LispDestroy(mac, "%s: :END2 %d larger than string length %d",
		    STRFUN(builtin), *end2, length2);
}

void
LispGetStringCaseArgs(LispMac *mac, LispBuiltin *builtin,
		      LispObj **result, char **string, int *start, int *end)
/*
 string-upcase string &key start end
 string-downcase string &key start end
 string-capitalize string &key start end
*/
{
    int length;
    LispObj *ostring, *ostart, *oend;

    oend = ARGUMENT(2);
    ostart = ARGUMENT(1);
    ostring = ARGUMENT(0);

    if (!STRING_P(ostring) && !SYMBOL_P(ostring))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(ostring));
    *result = ostring;
    *string = STRPTR(ostring);
    length = strlen(*string);

    if (ostart == NIL)
	*start = 0;
    else if (!INDEX_P(ostart))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(ostart));
    else
	*start = ostart->data.integer;

    if (oend == NIL)
	*end = length;
    else if (!INDEX_P(oend))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(oend));
    else
	*end = oend->data.integer;

    if (*start > *end)
	LispDestroy(mac, "%s: :START %d larger than :END %d",
		    STRFUN(builtin), *start, *end);
    if (*end > length)
	LispDestroy(mac, "%s: :END %d larger than string length %d",
		    STRFUN(builtin), *end, length);
}

LispObj *
LispStringTrim(LispMac *mac, LispBuiltin *builtin, int left, int right)
/*
 string-trim character-bag string
 string-left-trim character-bag string
 string-right-trim character-bag string
*/
{
    char *str;
    int start, end, sstart, send, length;

    LispObj *chars, *string;

    string = ARGUMENT(1);
    chars = ARGUMENT(0);

    if (!STRING_P(chars) && !CONS_P(chars)) {
	if (chars->type == LispArray_t && chars->data.array.rank == 1)
	    chars = chars->data.array.list;
	else
	    LispDestroy(mac, "%s: %s is not a sequence",
			STRFUN(builtin), STROBJ(chars));
    }
    if (!STRING_P(string) && !SYMBOL_P(string))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(string));

    sstart = start = 0;
    send = end = strlen(STRPTR(string));

    if (STRING_P(chars)) {
	char *cmp;

	if (left) {
	    for (str = STRPTR(string); *str; str++) {
		for (cmp = STRPTR(chars); *cmp; cmp++)
		    if (*str == *cmp)
			break;
		if (*cmp == '\0')
		    break;
		++start;
	    }
	}
	if (right) {
	    for (str = STRPTR(string) + end - 1; end > 0; str--) {
		for (cmp = STRPTR(chars); *cmp; cmp++)
		    if (*str == *cmp)
			break;
		if (*cmp == '\0')
		    break;
		--end;
	    }
	}
    }
    else {
	LispObj *obj;

	if (left) {
	    for (str = STRPTR(string); *str; str++) {
		for (obj = chars; obj != NIL; obj = CDR(obj))
		    /* Should really ignore non character input ? */
		    if (CHAR_P(CAR(obj)) && *str == CAR(obj)->data.integer)
			break;
		if (obj == NIL)
		    break;
		++start;
	    }
	}
	if (right) {
	    for (str = STRPTR(string) + end - 1; end > 0; str--) {
		for (obj = chars; obj != NIL; obj = CDR(obj))
		    /* Should really ignore non character input ? */
		    if (CHAR_P(CAR(obj)) && *str == CAR(obj)->data.integer)
			break;
		if (obj == NIL)
		    break;
		--end;
	    }
	}
    }

    if (sstart == start && send == end)
	return (string);

    length = end - start;
    str = LispMalloc(mac, length + 1);
    strncpy(str, STRPTR(string) + start, length);
    str[length] = '\0';

    string = STRING(str);
    LispFree(mac, str);

    return (string);
}

LispObj *
LispPathnameField(LispMac *mac, int field, int string)
{
    int offset = field;
    LispObj *pathname, *result, *object;

    pathname = ARGUMENT(0);

    if (!PATHNAME_P(pathname))
	pathname = EXECUTE("(parse-namestring pathname)");

    result = pathname->data.quote;
    while (offset) {
	result = CDR(result);
	--offset;
    }
    object = result;
    result = CAR(result);

    if (string) {
	if (!STRING_P(result)) {
	    if (result == NIL)
		result = STRING("");
	    else if (field == PATH_DIRECTORY) {
		char *name = STRPTR(CAR(pathname->data.quote)), *ptr;

		ptr = strrchr(name, PATH_SEP);
		if (ptr) {
		    int length = ptr - name + 1;
		    char data[PATH_MAX];

		    if (length > PATH_MAX - 1)
			length = PATH_MAX - 1;
		    strncpy(data, name, length);
		    data[length] = '\0';
		    result = STRING(data);
		}
		else
		    result = STRING("");
	    }
	    else
		result = KEYWORD(SYMBOL(mac->unspecific_atom));
	}
	else if (field == PATH_NAME) {
	    object = CAR(CDR(object));
	    if (STRING_P(object)) {
		int length;
		char name[PATH_MAX + 1];

		strcpy(name, STRPTR(result));
		length = strlen(name);
		if (length + 1 < sizeof(name)) {
		    name[length++] = PATH_TYPESEP;
		    name[length] = '\0';
		}
		if (strlen(STRPTR(object)) + length < sizeof(name))
		    strcpy(name + length, STRPTR(object));
		/* else LispDestroy ... */
		result = STRING(name);
	    }
	}
    }

    return (result);
}

LispObj *
LispProbeFile(LispMac *mac, LispBuiltin *builtin, int probe)
{
    LispObj *result;
    char *name = NULL, resolved[PATH_MAX + 1];
    struct stat st;

    LispObj *pathname;

    pathname = ARGUMENT(0);

    if (STRING_P(pathname)) {
	name = STRPTR(pathname);
	goto probeit;
    }
    else if (PATHNAME_P(pathname)) {
	name = STRPTR(CAR(pathname->data.quote));
	goto probeit;
    }
    else if (STREAM_P(pathname) &&
	     pathname->data.stream.type == LispStreamFile) {
	name = STRPTR(CAR(pathname->data.stream.pathname->data.quote));
	goto probeit;
    }
    else
	LispDestroy(mac, "%s: bad pathname %s",
		    STRFUN(builtin), STROBJ(pathname));

probeit:
    if (realpath(name, &resolved[0]) == NULL || stat(resolved, &st)) {
	if (probe)
	    return (NIL);
	LispDestroy(mac, "%s: realpath(%s): %s",
		    STRFUN(builtin), name, strerror(errno));
    }

    if (S_ISDIR(st.st_mode)) {
	int length = strlen(resolved);

	if (!length || resolved[length - 1] != PATH_SEP) {
	    resolved[length++] = PATH_SEP;
	    resolved[length] = '\0';
	}
    }

    GCProtect();
    result = EVAL(CONS(SYMBOL(mac->parse_namestring_atom),
		       CONS(STRING(resolved), NIL)));
    GCUProtect();

    return (result);
}

LispObj *
LispReadChar(LispMac *mac, LispBuiltin *builtin, int nohang)
/*
 read-char &optional input-stream (eof-error-p t) eof-value recursive-p
 read-char-no-hang &optional input-stream (eof-error-p t) eof-value recursive-p
 */
{
    int character;
    LispObj *result;

    LispObj *input_stream, *eof_error_p, *eof_value, *recursive_p;

    recursive_p = ARGUMENT(3);
    eof_value = ARGUMENT(2);
    eof_error_p = ARGUMENT(1);
    input_stream = ARGUMENT(0);

    if (input_stream != NIL) {
	if (!STREAM_P(input_stream))
	    LispDestroy(mac, "%s: %s is not a stream",
			STRFUN(builtin), STROBJ(input_stream));
    }
    else
	input_stream = mac->input;

    result = eof_value;
    character = EOF;

    if (input_stream->data.stream.readable) {
	LispFile *file = NULL;

	switch (input_stream->data.stream.type) {
	    case LispStreamStandard:
	    case LispStreamFile:
		file = FSTREAMP(input_stream);
		break;
	    case LispStreamPipe:
		file = IPSTREAMP(input_stream);
		break;
	    case LispStreamString:
		if (SSTREAMP(input_stream)->input >=
		    SSTREAMP(input_stream)->length)
		    character = EOF;		/* EOF reading from string */
		else
		    character = SSTREAMP(input_stream)->
			string[SSTREAMP(input_stream)->input++];
		break;
	    default:
		break;
	}
	if (file != NULL) {
	    if (file->available || file->offset < file->length)
		character = LispFgetc(file);
	    else {
		if (nohang && !file->nonblock) {
		    if (fcntl(file->descriptor, F_SETFL, O_NONBLOCK) < 0)
			LispDestroy(mac, "%s: fcntl(%d): %s",
				    STRFUN(builtin), file->descriptor,
				    strerror(errno));
		    file->nonblock = 1;
		}
		else if (!nohang && file->nonblock) {
		    if (fcntl(file->descriptor, F_SETFL, 0) < 0)
			LispDestroy(mac, "%s: fcntl(%s): %s",
				    STRFUN(builtin), file->descriptor,
				    strerror(errno));
		    file->nonblock = 0;
		}
		if (nohang) {
		    unsigned char ch;

		    if (read(file->descriptor, &ch, 1) == 1)
			character = ch;
		    else if (errno == EAGAIN)
			return (NIL);	/* XXX no character available */
		    else
			character = EOF;
		}
		else
		    character = LispFgetc(file);
	    }
	}
    }
    else
	LispDestroy(mac, "%s: stream %s is unreadable",
		    STRFUN(builtin), STROBJ(input_stream));

    if (character == EOF) {
	if (eof_error_p != NIL)
	    LispDestroy(mac, "%s: EOF reading stream %s",
			STRFUN(builtin), STROBJ(input_stream));

	return (eof_value);
    }

    return (CHAR(character));
}

LispObj *
LispWriteString(LispMac *mac, LispBuiltin *builtin, int newline)
/*
 write-line string &optional output-stream &key start end
 write-string string &optional output-stream &key start end
 */
{
    char *text;
    int start, end, length;

    LispObj *string, *output_stream, *ostart, *oend;

    oend = ARGUMENT(3);
    ostart = ARGUMENT(2);
    output_stream = ARGUMENT(1);
    string = ARGUMENT(0);

    if (!STRING_P(string))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(string));
    text = STRPTR(string);
    length = strlen(text);

    if (ostart != NIL) {
	if (!INDEX_P(ostart))
	    LispDestroy(mac, "%s: %s is not a positive integer",
			STRFUN(builtin), STROBJ(ostart));
	start = ostart->data.integer;
    }
    else
	start = 0;

    if (oend != NIL) {
	if (!INDEX_P(oend))
	    LispDestroy(mac, "%s: %s is not a positive integer",
			STRFUN(builtin), STROBJ(oend));
	end = oend->data.integer;
    }
    else
	end = length;

    if (start > end)
	LispDestroy(mac, "%s: :START %d is larger than :END %d",
		    STRFUN(builtin), start, end);
    if (end > length)
	LispDestroy(mac, "%s: :END %d is larger than string length %d",
		    STRFUN(builtin), end, length);

    if (end > start) {
	if (start == 0 && end == length)
	    LispPrintf(mac, output_stream, "%s", text);
	else {
	    char *ptr = LispMalloc(mac, end - start + 1);

	    strncpy(ptr, text + start, end - start);
	    ptr[end - start] = '\0';
	    LispPrintf(mac, output_stream, "%s", ptr);
	    LispFree(mac, ptr);
	}
    }
    if (newline) {
	LispPrintf(mac, output_stream, "%c", '\n');
	if (output_stream == NIL) {
	    mac->column = 0;
	    mac->newline = 1;
	}
    }
    else if (output_stream == NIL) {
	mac->newline = 0;
	mac->column += end - start;
    }

    return (string);
}

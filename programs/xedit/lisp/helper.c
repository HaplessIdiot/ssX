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

/* $XFree86: xc/programs/xedit/lisp/helper.c,v 1.25 2002/03/19 20:03:57 paulo Exp $ */

#include "helper.h"
#include "pathname.h"
#include "package.h"
#include "read.h"
#include "stream.h"
#include "write.h"
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

/* in math.c */
extern LispObj *LispFloatCoerce(LispMac*, LispBuiltin*, LispObj*);

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
	    case LispRatio_t:
		if (left->data.ratio.numerator ==
		    right->data.ratio.numerator &&
		    left->data.ratio.denominator ==
		    right->data.ratio.denominator)
		    result = T;
		break;
	    case LispComplex_t:
		if (LispEqual(mac, left->data.complex.real,
			      right->data.complex.real) &&
		    LispEqual(mac, left->data.complex.imag,
			      right->data.complex.imag))
		    result = T;
		break;
	    case LispString_t:
		if (strcmp(THESTR(left), THESTR(right)) == 0)
		    result = T;
		break;
	    case LispCons_t:
		if (LispEqual(mac, CAR(left), CAR(right)) == T &&
		    LispEqual(mac, CDR(left), CDR(right)) == T)
		    result = T;
		break;
	    case LispQuote_t:
	    case LispBackquote_t:
	    case LispPathname_t:
		result = LispEqual(mac, left->data.pathname,
				   right->data.pathname);
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

long
LispLength(LispMac *mac, LispObj *sequence)
{
    long length = 0;

    switch (sequence->type) {
	case LispNil_t:
	    break;
	case LispString_t:
	    length = strlen(THESTR(sequence));
	    break;
	case LispArray_t:
	    if (sequence->data.array.rank != 1)
		goto not_a_sequence;
	    sequence = sequence->data.array.list;
	    /*FALLTROUGH*/
	case LispCons_t:
	    for (; CONS_P(sequence); sequence = CDR(sequence))
		++length;
	    break;
	default:
not_a_sequence:
	    LispDestroy(mac, "LENGTH: %s is not a sequence", STROBJ(sequence));
	    /*NOTREACHED*/
    }

    return (length);
}

LispObj *
LispCharacterCoerce(LispMac *mac, LispBuiltin *builtin, LispObj *object)
{
    if (CHAR_P(object))
	return (object);
    else if (STRING_P(object) && strlen(THESTR(object)) == 1)
	return (CHAR((unsigned char)THESTR(object)[0]));
    else if (SYMBOL_P(object) && STRPTR(object)[1] == '\0')
	return (CHAR((unsigned char)STRPTR(object)[0]));
    else if (INDEX_P(object)) {
	int c = object->data.integer;

	if (c <= 0xffff)
	    return (CHAR(c));
    }

    LispDestroy(mac, "%s: cannot convert %s to character",
		STRFUN(builtin), STROBJ(object));
    /*NOTREACHED*/
    return (NIL);
}

LispObj *
LispStringCoerce(LispMac *mac, LispBuiltin *builtin, LispObj *object)
{
    if (STRING_P(object))
	return (object);
    else if (SYMBOL_P(object))
	return (STRING(STRPTR(object)));
    else if (CHAR_P(object)) {
	char string[2];

	string[0] = object->data.integer;
	string[1] = '\0';
	return (STRING(string));
    }
    else if (object == NIL)
	return (STRING("NIL"));
    else if (object == T)
	return (STRING("T"));
    else
	LispDestroy(mac, "%s: cannot convert %s to string",
		    STRFUN(builtin), STROBJ(object));
    /*NOTREACHED*/
    return (NIL);
}

LispObj *
LispCoerce(LispMac *mac, LispBuiltin *builtin,
	   LispObj *object, LispObj *result_type)
{
    LispObj *result = NIL;
    LispType type = LispNil_t;

    if (result_type == NIL)
	/* not even NIL can be converted to NIL? */
	LispDestroy(mac, "%s: cannot convert %s to nil",
		    STRFUN(builtin), STROBJ(object));

    else if (result_type == T)
	/* no conversion */
	return (object);

    else if (!SYMBOL_P(result_type))
	/* only know about simple types */
	LispDestroy(mac, "%s: bad argument %s",
		    STRFUN(builtin), STROBJ(result_type));

    else {
	/* check all known types */

	Atom_id atom = ATOMID(result_type);

	if (atom == Satom) {
	    if (CONS_P(object))
		goto coerce_fail;
	    return (object);
	}
	/* only convert ATOM to SYMBOL */

	if (atom == Sreal || atom == Sfloat)
	    type = LispReal_t;
	else if (atom == Sinteger)
	    type = LispInteger_t;
	else if (atom == Scons || atom == Slist) {
	    if (object == NIL)
		return (object);
	    type = LispCons_t;
	}
	else if (atom == Sstring)
	    type = LispString_t;
	else if (atom == Scharacter)
	    type = LispCharacter_t;
	else if (atom == Scomplex)
	    type = LispComplex_t;
	else if (atom == Svector || atom == Sarray)
	    type = LispArray_t;
	else if (atom == Sopaque)
	    type = LispOpaque_t;
	else if (atom == Srational)
	    type = LispRatio_t;
	else if (atom == Spathname)
	    type = LispPathname_t;
	else
	    LispDestroy(mac, "%s: invalid type specification %s",
			STRFUN(builtin), STRPTR(result_type));
    }

    if (object->type == LispOpaque_t) {
	switch (type) {
	    case LispAtom_t:
		result = ATOM(object->data.opaque.data);
		break;
	    case LispString_t:
		result = STRING(object->data.opaque.data);
		break;
	    case LispCharacter_t:
		result = CHAR((long)object->data.opaque.data);
		break;
	    case LispReal_t:
		result = REAL((double)((long)object->data.opaque.data));
		break;
	    case LispInteger_t:
		result = INTEGER(((long)object->data.opaque.data));
		break;
	    case LispOpaque_t:
		result = OPAQUE(object->data.opaque.data, 0);
		break;
	    default:
		goto coerce_fail;
		break;
	}
    }

    else if (object->type != type) {
	switch (type) {
	    case LispInteger_t:
		if (FLOAT_P(object)) {
		    if ((long)object->data.real == object->data.real)
			result = INTEGER((long)object->data.real);
		    else {
			mpi *integer = LispMalloc(mac, sizeof(mpi));

			mpi_init(integer);
			mpi_setd(integer, object->data.real);
			if (mpi_getd(integer) != object->data.real) {
			    mpi_clear(integer);
			    LispFree(mac, integer);
			    goto coerce_fail;
			}
			result = BIGINTEGER(integer);
			LispMused(mac, integer);
		    }
		}
		else if (BIGINT_P(object))
		    result = object;
		else
		    goto coerce_fail;
		break;
	    case LispRatio_t:
		if (FLOAT_P(object)) {
		    mpr *ratio = LispMalloc(mac, sizeof(mpr));

		    mpr_init(ratio);
		    mpr_setd(ratio, object->data.real);
		    if (mpr_fiti(ratio)) {
			result = RATIO(mpi_geti(mpr_num(ratio)),
				       mpi_geti(mpr_den(ratio)));
			mpr_clear(ratio);
			LispFree(mac, ratio);
		    }
		    else {
			result = BIGRATIO(ratio);
			LispMused(mac, ratio);
		    }
		}
		else if (RATIONAL_P(object))
		    result = object;
		else
		    goto coerce_fail;
		break;
	    case LispReal_t:
		result = LispFloatCoerce(mac, builtin, object);
	    	break;
	    case LispComplex_t:
		if (NUMBER_P(object))
		    result = object;
		else
		    goto coerce_fail;
		break;
	    case LispString_t:
		if (object == NIL)
		    result = STRING("");
		else
		    result = LispStringCoerce(mac, builtin, object);
		break;
	    case LispCharacter_t:
		result = LispCharacterCoerce(mac, builtin, object);
		break;
	    case LispArray_t:
		if (object == NIL || CONS_P(object))
		    result = VECTOR(object);
		else
		    goto coerce_fail;
		break;
	    case LispCons_t:
		if (object->type == LispArray_t && object->data.array.rank == 1)
		    result = object->data.array.list;
		else
		    goto coerce_fail;
		break;
	    case LispPathname_t:
		GCProtect();
		result = APPLY(Oparse_namestring, CONS(object, NIL));
		GCUProtect();
		break;
	    default:
		goto coerce_fail;
	}
    }
    else
	result = object;

    return (result);

coerce_fail:
    LispDestroy(mac, "%s: cannot convert %s to %s",
		STRFUN(builtin), STROBJ(object), STRPTR(result_type));
    /* NOTREACHED */
    return (NIL);
}

static LispObj *
LispReallyDo(LispMac *mac, LispBuiltin *builtin, int refs)
/*
 do init test &rest body
 do* init test &rest body
 */
{
    int syms, length = mac->protect.length;
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

    syms = 0;
    for (object = init; CONS_P(object); object = CDR(object), syms++) {
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
	    ERROR_CHECK_CONSTANT(symbol);
	    LispAddVar(mac, symbol, value);
	    ++mac->env.head;
	}
    }

    env = mac->protect.objects[length] = LispReverse(env);
    if (!refs) {
	for (object = env; object != NIL; object = CDR(object)) {
	    list = CAR(object);
	    ERROR_CHECK_CONSTANT(CAR(list));
	    LispAddVar(mac, CAR(list), CADR(list));
	    ++mac->env.head;
	}
    }

    /* Execute iterations */
    for (;;) {
	if (EVAL(CAR(test)) != NIL)
	    break;
	for (object = body; CONS_P(object); object = CDR(object))
	    (void)EVAL(CAR(object));

	/* Update variables */
	if (refs) {
	    /* Variables are sequentially updated */
	    for (object = env; CONS_P(object); object = CDR(object)) {
		list = CAR(object);
		if (CONS_P(CDDR(list)))
		    LispSetVar(mac, CAR(list), EVAL(CAR(CDDR(list))));
	    }
	}
	else {
	    /* Variables are only bound after all new values calculated */
	    int i, protect;

	    protect = mac->protect.length;
	    while (protect + syms > mac->protect.space)
		LispMoreProtects(mac);

	    /* Calculate new symbols values */
	    for (object = env; CONS_P(object); object = CDR(object)) {
		list = CAR(object);
		if (CONS_P(CDDR(list)))
		    result = EVAL(CAR(CDDR(list)));
		else
		    result = NIL;
		/* XXX this assumes mac->protect.length is always propertly
		 * restored. Another option would set the offset correctly,
		 * but would need to initialize fields to NIL, and it would
		 * consume a "vital" time for loops */
		mac->protect.objects[mac->protect.length++] = result;
	    }

	    /* Update symbols values */
	    i = protect;
	    for (object = env; CONS_P(object); object = CDR(object), i++) {
		list = CAR(object);
		if (CONS_P(CDDR(list)))
		    LispSetVar(mac, CAR(list), mac->protect.objects[i]);
	    }

	    mac->protect.length = protect;
	}
    }

    if (CONS_P(CDR(test)))
	result = EVAL(CADR(test));

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
    long count = 0, end = 0;
    LispObj *symbol, *value = NIL, *result = NIL, *init, *body, *object;

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
    ERROR_CHECK_CONSTANT(symbol);
    if (times)
	LispAddVar(mac, symbol, INTEGER(count));
    else
	LispAddVar(mac, symbol, CAR(value));
    ++mac->env.head;

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
LispLoadFile(LispMac *mac, LispObj *filename,
	     int verbose, int print, int ifdoesnotexist)
{
    LispObj *stream, *eval, *ext, *obj, *result = NIL;
    int ch, eof = mac->eof, length = mac->protect.length;

    LispObj *savepackage;
    LispPackage *savepack;

    if (verbose)
	LispMessage(mac, "; Loading %s", THESTR(filename));

    GCProtect();
    ext = ifdoesnotexist ? Kerror : NIL;
    eval = CONS(Oopen,
	        CONS(filename,
		     CONS(Kif_does_not_exist, CONS(ext, NIL))));
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

    /* Save package environment */
    savepackage = PACKAGE;
    savepack = mac->pack;

    /*CONSTCOND*/
    while (1) {
	if ((obj = LispRead(mac)) != NULL) {
	    if (obj == EOLIST)
		LispDestroy(mac, "object cannot start with #\\)");
	    else if (obj == DOT)
		LispDestroy(mac, "dot allowed only on lists");
	    mac->protect.objects[length] = obj;
	    result = EVAL(obj);
	    if (print) {
		int i;

		LispPrint(mac, result, NIL, 1);
		for (i = 0; i < RETURN_COUNT; i++)
		    LispPrint(mac, RETURN(i), NIL, 1);
	    }
	}
	if (mac->eof)
	    break;
    }
    LispPopInput(mac, stream);
    mac->eof = eof;
    mac->protect.length = length;

    /* Restore package environment */
    PACKAGE = savepackage;
    mac->pack = savepack;

    GCProtect();
    eval = CONS(Oclose, CONS(stream, NIL));
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

    if (!STRING_P(ostring1))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(ostring1));
    else {
	*string1 = THESTR(ostring1);
	length1 = strlen(*string1);
    }

    if (!STRING_P(ostring2))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(ostring2));
    else {
	*string2 = THESTR(ostring2);
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

    if (!STRING_P(ostring))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(ostring));
    *result = ostring;
    *string = THESTR(ostring);
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
    if (!STRING_P(string))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(string));

    sstart = start = 0;
    send = end = strlen(THESTR(string));

    if (STRING_P(chars)) {
	char *cmp;

	if (left) {
	    for (str = THESTR(string); *str; str++) {
		for (cmp = THESTR(chars); *cmp; cmp++)
		    if (*str == *cmp)
			break;
		if (*cmp == '\0')
		    break;
		++start;
	    }
	}
	if (right) {
	    for (str = THESTR(string) + end - 1; end > 0; str--) {
		for (cmp = THESTR(chars); *cmp; cmp++)
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
	    for (str = THESTR(string); *str; str++) {
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
	    for (str = THESTR(string) + end - 1; end > 0; str--) {
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
    strncpy(str, THESTR(string) + start, length);
    str[length] = '\0';

    string = STRING2(str);

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

    result = pathname->data.pathname;
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
		char *name = THESTR(CAR(pathname->data.pathname)), *ptr;

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
		result = Kunspecific;
	}
	else if (field == PATH_NAME) {
	    object = CAR(CDR(object));
	    if (STRING_P(object)) {
		int length;
		char name[PATH_MAX + 1];

		strcpy(name, THESTR(result));
		length = strlen(name);
		if (length + 1 < sizeof(name)) {
		    name[length++] = PATH_TYPESEP;
		    name[length] = '\0';
		}
		if (strlen(THESTR(object)) + length < sizeof(name))
		    strcpy(name + length, THESTR(object));
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
	name = THESTR(pathname);
	goto probeit;
    }
    else if (PATHNAME_P(pathname)) {
	name = THESTR(CAR(pathname->data.pathname));
	goto probeit;
    }
    else if (STREAM_P(pathname) &&
	     pathname->data.stream.type == LispStreamFile) {
	name = THESTR(CAR(pathname->data.stream.pathname->data.pathname));
	goto probeit;
    }
    else
	LispDestroy(mac, "%s: bad pathname %s",
		    STRFUN(builtin), STROBJ(pathname));

probeit:
    if (realpath(name, &resolved[0]) == NULL || stat(resolved, &st)) {
	if (probe)
	    return (NIL);
	LispDestroy(mac, "%s: realpath(\"%s\"): %s",
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
    result = EVAL(CONS(Oparse_namestring, CONS(STRING(resolved), NIL)));
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
		character = LispSgetc(SSTREAMP(input_stream));
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
LispWriteString_(LispMac *mac, LispBuiltin *builtin, int newline)
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
    text = THESTR(string);
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
	    LispWriteStr(mac, output_stream, text);
	else {
	    char *ptr = LispMalloc(mac, end - start + 1);

	    strncpy(ptr, text + start, end - start);
	    ptr[end - start] = '\0';
	    LispWriteStr(mac, output_stream, ptr);
	    LispFree(mac, ptr);
	}
    }
    if (newline)
	LispWriteChar(mac, output_stream, '\n');

    return (string);
}

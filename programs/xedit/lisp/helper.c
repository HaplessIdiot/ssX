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

/* $XFree86: xc/programs/xedit/lisp/helper.c,v 1.39 2002/10/06 17:11:42 paulo Exp $ */

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
static LispObj *LispReallyDo(LispBuiltin*, int);
static LispObj *LispReallyDoListTimes(LispBuiltin*, int);

/* in math.c */
extern LispObj *LispFloatCoerce(LispBuiltin*, LispObj*);

/*
 * Implementation
 */
LispObj *
LispObjectCompare(LispObj *left, LispObj *right, int function)
{
    LispObj *result = left == right ? T : NIL;

    /* If left and right are the same object, or if function is EQ */
    if (result == T || function == FEQ)
	return (result);

    /* Equalp requires that numeric objects be compared by value, and
     * strings or characters comparison be case insenstive */
    if (function == FEQUALP) {
	switch (left->type) {
	    case LispReal_t:
	    case LispInteger_t:
	    case LispRatio_t:
	    case LispBigInteger_t:
	    case LispBigRatio_t:
	    case LispComplex_t:
		switch (right->type) {
		    case LispReal_t:
		    case LispInteger_t:
		    case LispRatio_t:
		    case LispBigInteger_t:
		    case LispBigRatio_t:
		    case LispComplex_t:
			result = APPLY2(Oequal_, left, right);
			break;
		    default:
			break;
		}
		goto compare_done;
	    case LispCharacter_t:
		if (right->type == LispCharacter_t &&
		    toupper(GETINT(left)) == toupper(GETINT(right)))
		    result = T;
		goto compare_done;
	    case LispString_t:
		if (right->type == LispString_t &&
		    strcasecmp(THESTR(left), THESTR(right)) == 0)
		    result = T;
		goto compare_done;
	    default:
		break;
	}
    }

    /* Function is EQL or EQUAL, or EQUALP on arguments with the same rules */
    if (left->type == right->type) {
	switch (left->type) {
	    case LispNil_t:
	    case LispTrue_t:
		/*  Do nothing, result already set. Some internal objects
		 * like #<unbound> and #<dot> may be used as labels or
		 * variables in bytecode.c */
		break;
	    case LispReal_t:
		if (left->data.real == right->data.real)
		    result = T;
		break;
	    case LispCharacter_t:
	    case LispInteger_t:
		if (GETINT(left) == GETINT(right))
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
		if (LispObjectCompare(left->data.complex.real,
				      right->data.complex.real, function) &&
		    LispObjectCompare(left->data.complex.imag,
				      right->data.complex.imag, function))
		    result = T;
		break;
	    case LispBigInteger_t:
		if (mpi_cmp(left->data.mp.integer,
			    right->data.mp.integer) == 0)
		    result = T;
		break;
	    case LispBigRatio_t:
		if (mpr_cmp(left->data.mp.ratio, right->data.mp.ratio) == 0)
		    result = T;
		break;
	    default:
		break;
	}

	/* Next types must be the same object for EQL */
	if (function == FEQL)
	    goto compare_done;

	switch (left->type) {
	    case LispString_t:
		if (strcmp(THESTR(left), THESTR(right)) == 0)
		    result = T;
		break;
	    case LispCons_t:
		if (LispObjectCompare(CAR(left), CAR(right), function) == T &&
		    LispObjectCompare(CDR(left), CDR(right), function) == T)
		    result = T;
		break;
	    case LispQuote_t:
	    case LispBackquote_t:
	    case LispPathname_t:
		result = LispObjectCompare(left->data.pathname,
					   right->data.pathname, function);
		break;
	    case LispLambda_t:
		result = LispObjectCompare(left->data.lambda.name,
					   right->data.lambda.name,
					   function);
		break;
	    case LispOpaque_t:
		if (left->data.opaque.data == right->data.opaque.data)
		    result = T;
		break;
	    case LispRegex_t:
		/* If the regexs are guaranteed to generate the same matches */
		if (left->data.regex.options == right->data.regex.options)
		    result = LispObjectCompare(left->data.regex.pattern,
					       right->data.regex.pattern,
					       function);
		break;
	    default:
		break;
	}
    }

compare_done:
    return (result);
}

void
LispCheckSequenceStartEnd(LispBuiltin *builtin,
			  LispObj *sequence, LispObj *start, LispObj *end,
			  long *pstart, long *pend, long *plength)
{
    /* Calculate length of sequence and check it's type */
    *plength = LispLength(sequence);

    /* Check start argument */
    if (start == NIL)
	*pstart = 0;
    else {
	ERROR_CHECK_INDEX(start);
	*pstart = GETINT(start);
    }

    /* Check end argument */
    if (end == NIL)
	*pend = *plength;
    else {
	ERROR_CHECK_INDEX(end);
	*pend = GETINT(end);
    }

    /* Check start argument */
    if (*pstart > *pend)
	LispDestroy("%s: :START %ld is larger than :END %ld",
		    STRFUN(builtin), *pstart, *pend);

    /* Check end argument */
    if (*pend > *plength)
	LispDestroy("%s: :END %ld is larger then sequence length %ld",
		    STRFUN(builtin), *pend, *plength);
}

long
LispLength(LispObj *sequence)
{
    long length;

    switch (sequence->type) {
	case LispNil_t:
	    length = 0;
	    break;
	case LispString_t:
	    length = STRLEN(sequence);
	    break;
	case LispArray_t:
	    if (sequence->data.array.rank != 1)
		goto not_a_sequence;
	    sequence = sequence->data.array.list;
	    /*FALLTROUGH*/
	case LispCons_t:
	    for (length = 0;
		 CONS_P(sequence);
		 length++, sequence = CDR(sequence))
		;
	    break;
	default:
not_a_sequence:
	    LispDestroy("LENGTH: %s is not a sequence", STROBJ(sequence));
	    /*NOTREACHED*/
	    length = 0;
    }

    return (length);
}

LispObj *
LispCharacterCoerce(LispBuiltin *builtin, LispObj *object)
{
    if (CHAR_P(object))
	return (object);
    else if (STRING_P(object) && STRLEN(object) == 1)
	return (CHAR((unsigned char)THESTR(object)[0]));
    else if (SYMBOL_P(object) && STRPTR(object)[1] == '\0')
	return (CHAR((unsigned char)STRPTR(object)[0]));
    else if (INDEX_P(object)) {
	int c = GETINT(object);

	if (c <= 0xff)
	    return (CHAR(c));
    }

    LispDestroy("%s: cannot convert %s to character",
		STRFUN(builtin), STROBJ(object));
    /*NOTREACHED*/
    return (NIL);
}

LispObj *
LispStringCoerce(LispBuiltin *builtin, LispObj *object)
{
    if (STRING_P(object))
	return (object);
    else if (SYMBOL_P(object))
	return (STRING(STRPTR(object)));
    else if (CHAR_P(object)) {
	char string[1];

	string[0] = GETINT(object);
	return (LSTRING(string, 1));
    }
    else if (object == NIL)
	return (LSTRING(Snil, 3));
    else if (object == T)
	return (LSTRING(St, 1));
    else
	LispDestroy("%s: cannot convert %s to string",
		    STRFUN(builtin), STROBJ(object));
    /*NOTREACHED*/
    return (NIL);
}

LispObj *
LispCoerce(LispBuiltin *builtin,
	   LispObj *object, LispObj *result_type)
{
    LispObj *result = NIL;
    LispType type = LispNil_t;

    if (result_type == NIL)
	/* not even NIL can be converted to NIL? */
	LispDestroy("%s: cannot convert %s to NIL",
		    STRFUN(builtin), STROBJ(object));

    else if (result_type == T)
	/* no conversion */
	return (object);

    else if (!SYMBOL_P(result_type))
	/* only know about simple types */
	LispDestroy("%s: bad argument %s",
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
	    LispDestroy("%s: invalid type specification %s",
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
			result = SMALLINT((long)object->data.real);
		    else {
			mpi *integer = LispMalloc(sizeof(mpi));

			mpi_init(integer);
			mpi_setd(integer, object->data.real);
			if (mpi_getd(integer) != object->data.real) {
			    mpi_clear(integer);
			    LispFree(integer);
			    goto coerce_fail;
			}
			result = BIGINTEGER(integer);
			LispMused(integer);
		    }
		}
		else if (BIGINT_P(object))
		    result = object;
		else
		    goto coerce_fail;
		break;
	    case LispRatio_t:
		if (FLOAT_P(object)) {
		    mpr *ratio = LispMalloc(sizeof(mpr));

		    mpr_init(ratio);
		    mpr_setd(ratio, object->data.real);
		    if (mpr_fiti(ratio)) {
			result = RATIO(mpi_geti(mpr_num(ratio)),
				       mpi_geti(mpr_den(ratio)));
			mpr_clear(ratio);
			LispFree(ratio);
		    }
		    else {
			result = BIGRATIO(ratio);
			LispMused(ratio);
		    }
		}
		else if (RATIONAL_P(object))
		    result = object;
		else
		    goto coerce_fail;
		break;
	    case LispReal_t:
		result = LispFloatCoerce(builtin, object);
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
		    result = LispStringCoerce(builtin, object);
		break;
	    case LispCharacter_t:
		result = LispCharacterCoerce(builtin, object);
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
		result = APPLY1(Oparse_namestring, object);
		break;
	    default:
		goto coerce_fail;
	}
    }
    else
	result = object;

    return (result);

coerce_fail:
    LispDestroy("%s: cannot convert %s to %s",
		STRFUN(builtin), STROBJ(object), STRPTR(result_type));
    /* NOTREACHED */
    return (NIL);
}

static LispObj *
LispReallyDo(LispBuiltin *builtin, int refs)
/*
 do init test &rest body
 do* init test &rest body
 */
{
    GC_ENTER();
    int stack, lex, head;
    LispObj *list, *symbol, *value, *values, *cons;

    LispObj *init, *test, *body;

    body = ARGUMENT(2);
    test = ARGUMENT(1);
    init = ARGUMENT(0);

    if (!CONS_P(test))
	LispDestroy("%s: end test condition must be a list, not %s",
		    STRFUN(builtin), STROBJ(init));

    if (init != NIL && !CONS_P(init))
	LispDestroy("%s: %s is not a list",
		    STRFUN(builtin), STROBJ(init));

    /* Save state */
    stack = lisp__data.stack.length;
    lex = lisp__data.env.lex;
    head = lisp__data.env.length;

    values = cons = NIL;
    for (list = init; CONS_P(list); list = CDR(list)) {
	symbol = CAR(list);
	if (!SYMBOL_P(symbol)) {
	    ERROR_CHECK_LIST(symbol);
	    value = CDR(symbol);
	    symbol = CAR(symbol);
	    ERROR_CHECK_SYMBOL(symbol);
	    ERROR_CHECK_LIST(value);
	    value = EVAL(CAR(value));
	}
	else
	    value = NIL;

	ERROR_CHECK_CONSTANT(symbol);

	LispAddVar(symbol, value);

	/* Bind variable now */
	if (refs) {
	    ++lisp__data.env.head;
	}
	else {
	    if (values == NIL) {
		values = cons = CONS(NIL, NIL);
		GC_PROTECT(values);
	    }
	    else {
		RPLACD(cons, CONS(NIL, NIL));
		cons = CDR(cons);
	    }
	}
    }
    if (!refs)
	lisp__data.env.head = lisp__data.env.length;

    for (;;) {
	if (EVAL(CAR(test)) != NIL)
	    break;

	/* TODO Run this code in an implicit tagbody */
	for (list = body; CONS_P(list); list = CDR(list))
	    (void)EVAL(CAR(list));

	/* Error checking already done in the initialization */
	for (list = init, cons = values; CONS_P(list); list = CDR(list)) {
	    symbol = CAR(list);
	    if (CONS_P(symbol)) {
		value = CDDR(symbol);
		symbol = CAR(symbol);
		if (CONS_P(value))
		    value = EVAL(CAR(value));
		else
		    value = NIL;
	    }
	    else
		value = NIL;

	    if (refs)
		LispSetVar(symbol, value);
	    else {
		RPLACA(cons, value);
		cons = CDR(cons);
	    }
	}
	if (!refs) {
	    for (list = init, cons = values; CONS_P(list);
		 list = CDR(list), cons = CDR(cons)) {
		symbol = CAR(list);
		if (CONS_P(symbol)) {
		    if (CONS_P(CDR(symbol)))
			LispSetVar(CAR(symbol), CAR(cons));
		}
	    }
	}
    }

    if (CONS_P(CDR(test)))
	value = EVAL(CADR(test));
    else
	value = NIL;

    /* Restore state */
    lisp__data.stack.length = stack;
    lisp__data.env.lex = lex;
    lisp__data.env.head = lisp__data.env.length = head;
    GC_LEAVE();

    return (value);
}

LispObj *
LispDo(LispBuiltin *builtin, int refs)
/*
 do init test &rest body
 do* init test &rest body
 */
{
    int jumped, *pjumped;
    LispObj *result, **presult;
    LispBlock *block;

    jumped = 1;
    result = NIL;
    presult = &result;
    pjumped = &jumped;
    block = LispBeginBlock(NIL, LispBlockTag);
    if (setjmp(block->jmp) == 0) {
	result = LispReallyDo(builtin, refs);
	jumped = 0;
    }
    LispEndBlock(block);
    if (jumped)
	result = lisp__data.block.block_ret;

    return (result);
}

static LispObj *
LispReallyDoListTimes(LispBuiltin *builtin, int times)
/*
 dolist init &rest body
 dotimes init &rest body
 */
{
    GC_ENTER();
    int head = lisp__data.env.length;
    long count = 0, end = 0;
    LispObj *symbol, *value = NIL, *result = NIL, *init, *body, *object;

    body = ARGUMENT(1);
    init = ARGUMENT(0);

    /* Parse arguments */
    ERROR_CHECK_LIST(init);
    symbol = CAR(init);
    ERROR_CHECK_SYMBOL(symbol);
    init = CDR(init);

    if (init == NIL) {
	if (times)
	    LispDestroy("%s: NIL is not a number", STRFUN(builtin));
    }
    else {
	ERROR_CHECK_LIST(init);
	value = CAR(init);
	init = CDR(init);
	if (init != NIL) {
	    ERROR_CHECK_LIST(init);
	    result = CAR(init);
	}

	if (NCONSTANT_P(value))
	    value = EVAL(value);

	if (times) {
	    ERROR_CHECK_INDEX(value);
	    end = GETINT(value);
	}
	else {
	    if (value != NIL) {
		ERROR_CHECK_LIST(value);
	    }
	    /* Protect iteration control from gc */
	    GC_PROTECT(value);
	}
    }

    /* The variable is only bound inside the loop, so it is safe to optimize
     * it out if there is no code to execute. But the result form may reference
     * the bound variable. */
    if (!CONS_P(body)) {
	if (times)
	    count = end;
	else
	    value = NIL;
    }

    /* Initialize counter */
    ERROR_CHECK_CONSTANT(symbol);
    if (times)
	LispAddVar(symbol, SMALLINT(count));
    else
	LispAddVar(symbol, CONS_P(value) ? CAR(value) : value);
    ++lisp__data.env.head;

    if (!CONS_P(body) || (times && count >= end) || (!times && !CONS_P(value)))
	goto loop_done;

    /* Execute iterations */
    for (;;) {
	for (object = body; CONS_P(object); object = CDR(object))
	    (void)EVAL(CAR(object));

	/* Update symbols and check exit condition */
	if (times) {
	    ++count;
	    LispSetVar(symbol, SMALLINT(count));
	    if (count >= end)
		break;
	}
	else {
	    value = CDR(value);
	    if (!CONS_P(value)) {
		LispSetVar(symbol, NIL);
		break;
	    }
	    LispSetVar(symbol, CAR(value));
	}
    }

loop_done:
    if (NCONSTANT_P(result))
	result = EVAL(result);
    lisp__data.env.head = lisp__data.env.length = head;
    GC_LEAVE();

    return (result);
}

LispObj *
LispDoListTimes(LispBuiltin *builtin, int times)
/*
 dolist init &rest body
 dotimes init &rest body
 */
{
    int did_jump, *pdid_jump = &did_jump;
    LispObj *result, **presult = &result;
    LispBlock *block;

    *presult = NIL;
    *pdid_jump = 1;
    block = LispBeginBlock(NIL, LispBlockTag);
    if (setjmp(block->jmp) == 0) {
	result = LispReallyDoListTimes(builtin, times);
	did_jump = 0;
    }
    LispEndBlock(block);
    if (did_jump)
	result = lisp__data.block.block_ret;

    return (result);
}

LispObj *
LispLoadFile(LispObj *filename, int verbose, int print, int ifdoesnotexist)
{
    GC_ENTER();
    LispObj *stream, *ext, *cod, *obj, *result;
    int ch;

    LispObj *savepackage;
    LispPackage *savepack;

    if (verbose)
	LispMessage("; Loading %s", THESTR(filename));

    ext = ifdoesnotexist ? Kerror : NIL;
    result = CONS(filename, CONS(Kif_does_not_exist, CONS(ext, NIL)));
    GC_PROTECT(result);
    stream = APPLY(Oopen, result);
    GC_LEAVE();

    if (stream == NIL)
	return (NIL);

    result = NIL;
    LispPushInput(stream);
    ch = LispGet();
    if (ch != '#')
	LispUnget(ch);
    else if ((ch = LispGet()) == '!') {
	for (;;) {
	    ch = LispGet();
	    if (ch == '\n' || ch == EOF)
		break;
	}
    }
    else {
	LispUnget(ch);
	LispUnget('#');
    }

    /* Save package environment */
    savepackage = PACKAGE;
    savepack = lisp__data.pack;

    cod = COD;

    /*CONSTCOND*/
    while (1) {
	if ((obj = LispRead()) != NULL) {
	    if (obj == EOLIST)
		LispDestroy("object cannot start with #\\)");
	    else if (obj == DOT)
		LispDestroy("dot allowed only on lists");
	    result = EVAL(obj);
	    COD = cod;
	    if (print) {
		int i;

		LispPrint(result, NIL, 1);
		for (i = 0; i < RETURN_COUNT; i++)
		    LispPrint(RETURN(i), NIL, 1);
	    }
	}
	if (lisp__data.eof)
	    break;
    }
    LispPopInput(stream);

    /* Restore package environment */
    PACKAGE = savepackage;
    lisp__data.pack = savepack;

    APPLY1(Oclose, stream);

    return (T);
}

void
LispGetStringArgs(LispBuiltin *builtin,
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

    ERROR_CHECK_STRING(ostring1);
    *string1 = THESTR(ostring1);
    length1 = STRLEN(ostring1);

    ERROR_CHECK_STRING(ostring2);
    *string2 = THESTR(ostring2);
    length2 = STRLEN(ostring2);

    if (ostart1 == NIL)
	*start1 = 0;
    else {
	ERROR_CHECK_INDEX(ostart1);
	*start1 = GETINT(ostart1);
    }
    if (oend1 == NIL)
	*end1 = length1;
    else {
	ERROR_CHECK_INDEX(oend1);
	*end1 = GETINT(oend1);
    }

    if (ostart2 == NIL)
	*start2 = 0;
    else {
	ERROR_CHECK_INDEX(ostart2);
	*start2 = GETINT(ostart2);
    }

    if (oend2 == NIL)
	*end2 = length2;
    else {
	ERROR_CHECK_INDEX(oend2);
	*end2 = GETINT(oend2);
    }

    if (*start1 > *end1)
	LispDestroy("%s: :START1 %d larger than :END1 %d",
		    STRFUN(builtin), *start1, *end1);
    if (*start2 > *end2)
	LispDestroy("%s: :START2 %d larger than :END2 %d",
		    STRFUN(builtin), *start2, *end2);
    if (*end1 > length1)
	LispDestroy("%s: :END1 %d larger than string length %d",
		    STRFUN(builtin), *end1, length1);
    if (*end2 > length2)
	LispDestroy("%s: :END2 %d larger than string length %d",
		    STRFUN(builtin), *end2, length2);
}

LispObj *
LispStringTrim(LispBuiltin *builtin, int left, int right)
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
	    LispDestroy("%s: %s is not a sequence",
			STRFUN(builtin), STROBJ(chars));
    }
    ERROR_CHECK_STRING(string);

    sstart = start = 0;
    send = end = STRLEN(string);

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
		    if (CHAR_P(CAR(obj)) && *str == GETINT(CAR(obj)))
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
		    if (CHAR_P(CAR(obj)) && *str == GETINT(CAR(obj)))
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
    str = LispMalloc(length + 1);
    memcpy(str, THESTR(string) + start, length);
    str[length] = '\0';

    string = LSTRING2(str, length);

    return (string);
}

LispObj *
LispPathnameField(int field, int string)
{
    int offset = field;
    LispObj *pathname, *result, *object;

    pathname = ARGUMENT(0);

    if (!PATHNAME_P(pathname))
	pathname = APPLY1(Oparse_namestring, pathname);

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
		length = STRLEN(result);
		if (length + 1 < sizeof(name)) {
		    name[length++] = PATH_TYPESEP;
		    name[length] = '\0';
		}
		if (STRLEN(object) + length < sizeof(name))
		    strcpy(name + length, THESTR(object));
		/* else LispDestroy ... */
		result = STRING(name);
	    }
	}
    }

    return (result);
}

LispObj *
LispProbeFile(LispBuiltin *builtin, int probe)
{
    GC_ENTER();
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
	LispDestroy("%s: bad pathname %s",
		    STRFUN(builtin), STROBJ(pathname));

probeit:
    if (realpath(name, &resolved[0]) == NULL || stat(resolved, &st)) {
	if (probe)
	    return (NIL);
	LispDestroy("%s: realpath(\"%s\"): %s",
		    STRFUN(builtin), name, strerror(errno));
    }

    if (S_ISDIR(st.st_mode)) {
	int length = strlen(resolved);

	if (!length || resolved[length - 1] != PATH_SEP) {
	    resolved[length++] = PATH_SEP;
	    resolved[length] = '\0';
	}
    }

    result = STRING(resolved);
    GC_PROTECT(result);
    result = APPLY1(Oparse_namestring, result);
    GC_LEAVE();

    return (result);
}

LispObj *
LispReadChar(LispBuiltin *builtin, int nohang)
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
	ERROR_CHECK_STREAM(input_stream);
    }
    else
	input_stream = lisp__data.input;

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
			LispDestroy("%s: fcntl(%d): %s",
				    STRFUN(builtin), file->descriptor,
				    strerror(errno));
		    file->nonblock = 1;
		}
		else if (!nohang && file->nonblock) {
		    if (fcntl(file->descriptor, F_SETFL, 0) < 0)
			LispDestroy("%s: fcntl(%d): %s",
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
	LispDestroy("%s: stream %s is unreadable",
		    STRFUN(builtin), STROBJ(input_stream));

    if (character == EOF) {
	if (eof_error_p != NIL)
	    LispDestroy("%s: EOF reading stream %s",
			STRFUN(builtin), STROBJ(input_stream));

	return (eof_value);
    }

    return (CHAR(character));
}

LispObj *
LispWriteString_(LispBuiltin *builtin, int newline)
/*
 write-line string &optional output-stream &key start end
 write-string string &optional output-stream &key start end
 */
{
    char *text;
    long start, end, length;

    LispObj *string, *output_stream, *ostart, *oend;

    oend = ARGUMENT(3);
    ostart = ARGUMENT(2);
    output_stream = ARGUMENT(1);
    string = ARGUMENT(0);

    ERROR_CHECK_STRING(string);
    LispCheckSequenceStartEnd(builtin, string, ostart, oend,
			      &start, &end, &length);
    text = THESTR(string);
    if (end > start)
	LispWriteStr(output_stream, text + start, end - start);
    if (newline)
	LispWriteChar(output_stream, '\n');

    return (string);
}

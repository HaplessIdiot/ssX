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

/* $XFree86: xc/programs/xedit/lisp/core.c,v 1.57 2002/11/13 04:35:45 paulo Exp $ */

#include "io.h"
#include "core.h"
#include "format.h"
#include "helper.h"
#include "private.h"
#include "write.h"

#ifdef NEED_SETENV
extern int setenv(const char *name, const char *value, int overwrite);
extern void unsetenv(const char *name);
#endif

/*
 * Prototypes
 */
#define NONE		0

#define	REMOVE		1
#define	SUBSTITUTE	2
#define DELETE		3
#define	NSUBSTITUTE	4

#define ASSOC		1
#define MEMBER		2

#define FIND		1
#define POSITION	2

#define	IF		1
#define	IFNOT		2

#define UNION		1
#define INTERSECTION	2
#define SETDIFFERENCE	3
#define SETEXCLUSIVEOR	4
#define SUBSETP		5
#define NSETDIFFERENCE	6

/* Call directly LispObjectCompare() if possible */
#define FCODE(predicate)					\
    predicate == Oeql ? FEQL :					\
	predicate == Oequal ? FEQUAL :				\
	    predicate == Oeq ? FEQ :				\
		predicate == Oequalp ? FEQUALP : 0
#define FCOMPARE(predicate, left, right, code)			\
    code == FEQ ? (left == right ? T : NIL) :			\
	code ? LispObjectCompare(left, right, code) :	\
	       APPLY2(predicate, left, right)

LispObj *LispAdjoin(LispBuiltin*,
		    LispObj*, LispObj*, LispObj*, LispObj*, LispObj*);
LispObj *LispAssocOrMember(LispBuiltin*, int, int);
LispObj *LispFindOrPosition(LispBuiltin*, int, int);
LispObj *LispDeleteOrRemoveDuplicates(LispBuiltin*, int);
LispObj *LispDeleteRemoveXSubstitute(LispBuiltin*, int, int);
LispObj *LispListSet(LispBuiltin*, int);
extern LispObj *LispRunSetf(LispArgList*, LispObj*, LispObj*, LispObj*);
static LispObj *LispMergeSort(LispObj*, LispObj*, LispObj*, int);
LispObj *LispXReverse(LispBuiltin*, int);

/*
 * Initialization
 */
LispObj *Oeq, *Oeql, *Oequal, *Oequalp, *Omake_array,
	*Kinitial_contents, *Osetf, *Ootherwise;
LispObj *Ogensym_counter;

Atom_id Svariable, Sstructure, Stype, Ssetf;

/*
 * Implementation
 */
void
LispCoreInit(void)
{
    Oeq			= STATIC_ATOM("EQ");
    Oeql		= STATIC_ATOM("EQL");
    Oequal		= STATIC_ATOM("EQUAL");
    Oequalp		= STATIC_ATOM("EQUALP");
    Omake_array		= STATIC_ATOM("MAKE-ARRAY");
    Kinitial_contents	= KEYWORD("INITIAL-CONTENTS");
    Osetf		= STATIC_ATOM("SETF");
    Ootherwise		= STATIC_ATOM("OTHERWISE");

    Svariable		= GETATOMID("VARIABLE");
    Sstructure		= GETATOMID("STRUCTURE");
    Stype		= GETATOMID("TYPE");

    /* Create as a constant so that only the C code should change the value */
    Ogensym_counter	= STATIC_ATOM("*GENSYM-COUNTER*");
    LispDefconstant(Ogensym_counter, FIXNUM(0), NIL);
    LispExportSymbol(Ogensym_counter);

    Ssetf	= ATOMID(Osetf);
}

LispObj *
Lisp_Acons(LispBuiltin *builtin)
/*
 acons key datum alist
 */
{
    LispObj *key, *datum, *alist;

    alist = ARGUMENT(2);
    datum = ARGUMENT(1);
    key = ARGUMENT(0);

    return (CONS(CONS(key, datum), alist));
}

LispObj *
LispAdjoin(LispBuiltin*builtin, LispObj *item, LispObj *list,
	   LispObj *key, LispObj *test, LispObj *test_not)
{
    GC_ENTER();
    int code;
    LispObj *lambda, *compare, *expect, *value, *object;

    CHECK_LIST(list);
    if (test != NIL && test_not != NIL)
	LispDestroy("%s: specify either :TEST or :TEST-NOT", STRFUN(builtin));

    if (test_not == NIL) {
	if (test == NIL)
	    lambda = Oeql;
	else
	    lambda = test;
	expect = T;
    }
    else {
	lambda = test_not;
	expect = NIL;
    }
    code = FCODE(lambda);
 
    if (key != NIL) {
	item = APPLY1(key, item);
	/* Result is not guaranteed to be gc protected */
	GC_PROTECT(item);
    }

    /* Check if item is not already in place */
    for (object = list; CONSP(object); object = CDR(object)) {
	compare = CAR(object);
	if (key != NIL) {
	    compare = APPLY1(key, compare);
	    GC_PROTECT(compare);
	    value = FCOMPARE(lambda, item, compare, code);
	    /* Unprotect compare... */
	    --lisp__data.protect.length;
	}
	else
	    value = FCOMPARE(lambda, item, compare, code);

	if (value != NIL)
	    value = T;
	if (value == expect) {
	    /* Item is already in list */
	    GC_LEAVE();

	    return (list);
	}
    }
    GC_LEAVE();

    return (CONS(item, list));
}

LispObj *
Lisp_Adjoin(LispBuiltin *builtin)
/*
 adjoin item list &key key test test-not
 */
{
    LispObj *item, *list, *key, *test, *test_not;

    test_not = ARGUMENT(4);
    test = ARGUMENT(3);
    key = ARGUMENT(2);
    list = ARGUMENT(1);
    item = ARGUMENT(0);

    return (LispAdjoin(builtin, item, list, key, test, test_not));
}

LispObj *
Lisp_Append(LispBuiltin *builtin)
/*
 append &rest lists
 */
{
    GC_ENTER();

    LispObj *result, *cons, *list, *lists;

    lists = ARGUMENT(0);

    result = cons = NIL;
    for (; CONSP(lists); lists = CDR(lists)) {
	list = CAR(lists);
	if (list == NIL)
	    continue;
	CHECK_CONS(list);
	if (result == NIL) {
	    result = cons = CONS(CAR(list), CDR(list));
	    GC_PROTECT(result);
	}
	else {
	    if (CONSP(CDR(cons))) {
		LispObj *obj = CDR(cons);

		while (CONSP(CDR(obj))) {
		    RPLACD(cons, CONS(CAR(obj), CDR(obj)));
		    cons = CDR(cons);
		    obj = CDR(obj);
		}
		RPLACD(cons, CONS(CADR(cons), list));
	    }
	    else
		RPLACD(cons, list);
	    cons = CDR(cons);
	}
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Aref(LispBuiltin *builtin)
/*
 aref array &rest subscripts
 */
{
    long c, count, idx, seq;
    LispObj *obj, *dim;

    LispObj *array, *subscripts;

    subscripts = ARGUMENT(1);
    array = ARGUMENT(0);

    /* accept strings also */
    if (STRINGP(array) && CONSP(subscripts) && CDR(subscripts) == NIL) {
	long offset, length = STRLEN(array);

	CHECK_INDEX(CAR(subscripts));
	offset = FIXNUM_VALUE(CAR(subscripts));

	if (offset >= length)
	    LispDestroy("%s: index %ld too large for sequence length %ld",
			STRFUN(builtin), offset, length);

	return (SCHAR(THESTR(array)[offset]));
    }

    CHECK_ARRAY(array);

    for (count = 0, dim = subscripts, obj = array->data.array.dim; CONSP(dim);
	 count++, dim = CDR(dim), obj = CDR(obj)) {
	if (count >= array->data.array.rank)
	    LispDestroy("%s: too many subscripts %s",
			STRFUN(builtin), STROBJ(subscripts));
	if (!INDEXP(CAR(dim)) ||
	    FIXNUM_VALUE(CAR(dim)) >= FIXNUM_VALUE(CAR(obj)))
	    LispDestroy("%s: %s is out of range or a bad index",
			STRFUN(builtin), STROBJ(CAR(dim)));
    }
    if (count < array->data.array.rank)
	LispDestroy("%s: too few subscripts %s",
		    STRFUN(builtin), STROBJ(subscripts));

    for (count = seq = 0, dim = subscripts; CONSP(dim); dim = CDR(dim), seq++) {
	for (idx = 0, obj = array->data.array.dim; idx < seq;
	     obj = CDR(obj), ++idx)
	    ;
	for (c = 1, obj = CDR(obj); obj != NIL; obj = CDR(obj))
	    c *= FIXNUM_VALUE(CAR(obj));
	count += c * FIXNUM_VALUE(CAR(dim));
    }

    for (array = array->data.array.list; count > 0; array = CDR(array), count--)
	;

    return (CAR(array));
}

LispObj *
LispAssocOrMember(LispBuiltin *builtin, int function, int comparison)
/*
 assoc item list &key test test-not key
 assoc-if predicate list &key key
 assoc-if-not predicate list &key key
 member item list &key test test-not key
 member-if predicate list &key key
 member-if-not predicate list &key key
 */
{
    int code = 0;
    LispObj *predicate, *expect, *result, *compare;

    LispObj *item, *list, *test, *test_not, *key;

    if (comparison == NONE) {
	key = ARGUMENT(4);
	test_not = ARGUMENT(3);
	test = ARGUMENT(2);
	list = ARGUMENT(1);
	item = ARGUMENT(0);
	predicate = NIL;
    }
    else {
	key = ARGUMENT(2);
	list = ARGUMENT(1);
	predicate = ARGUMENT(0);
	test = test_not = item = NIL;
    }

    if (list == NIL)
	return (NIL);
    else {
	CHECK_CONS(list);
    }

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy("%s: specify either :TEST or :TEST-NOT", STRFUN(builtin));

    /* Resolve compare function, and expected result of comparison */
    if (comparison == NONE) {
	if (test_not == NIL) {
	    if (test == NIL)
		predicate = Oeql;
	    else
		predicate = test;
	    expect = T;
	}
	else {
	    predicate = test_not;
	    expect = NIL;
	}
	code = FCODE(predicate);
    }
    else
	expect = comparison == IFNOT ? NIL : T;

    result = NIL;
    for (; CONSP(list); list = CDR(list)) {
	compare = CAR(list);
	if (function == ASSOC) {
	    if (!CONSP(compare))
		continue;
	    compare = CAR(compare);
	}
	if (key != NIL)
	    compare = APPLY1(key, compare);

	if (comparison == NONE)
	    compare = FCOMPARE(predicate, item, compare, code);
	else
	    compare = APPLY1(predicate, compare);
	if (compare != NIL)
	    compare = T;
	if (compare == expect) {
	    result = list;
	    if (function == ASSOC)
		result = CAR(result);
	    break;
	}
    }

    return (result);
}

LispObj *
Lisp_Assoc(LispBuiltin *builtin)
/*
 assoc item list &key test test-not key
 */
{
    return (LispAssocOrMember(builtin, ASSOC, NONE));
}

LispObj *
Lisp_AssocIf(LispBuiltin *builtin)
/*
 assoc-if predicate list &key key
 */
{
    return (LispAssocOrMember(builtin, ASSOC, IF));
}

LispObj *
Lisp_AssocIfNot(LispBuiltin *builtin)
/*
 assoc-if-not predicate list &key key
 */
{
    return (LispAssocOrMember(builtin, ASSOC, IFNOT));
}

LispObj *
Lisp_And(LispBuiltin *builtin)
/*
 and &rest args
 */
{
    LispObj *result = T, *args;

    args = ARGUMENT(0);

    for (; CONSP(args); args = CDR(args)) {
	result = EVAL(CAR(args));
	if (result == NIL)
	    break;
    }

    return (result);
}

LispObj *
Lisp_Apply(LispBuiltin *builtin)
/*
 apply function arg &rest more-args
 */
{
    GC_ENTER();
    LispObj *result, *function, *arg, *more_args;

    more_args = ARGUMENT(2);
    arg = ARGUMENT(1);
    function = ARGUMENT(0);

    if (more_args != NIL) {
	CHECK_CONS(more_args);
    }
    else {
	CHECK_LIST(arg);
    }

    if (more_args == NIL)
	result = arg;
    else {
	LispObj *cons;

	result = cons = CONS(arg, NIL);
	GC_PROTECT(result);

	for (;; more_args = CDR(more_args)) {
	    if (CONSP(CDR(more_args))) {
		RPLACD(cons, CONS(CAR(more_args), NIL));
		cons = CDR(cons);
	    }
	    else {
		arg = RPLACD(cons, CAR(more_args));
		if (CONSP(arg))
		    for (; CONSP(arg); arg = CDR(arg))
			;
		if (arg != NIL)
		    LispDestroy("%s: argument to %s is a dotted list",
				STRFUN(builtin), STROBJ(function));
		break;
	    }
	}
    }

    result = APPLY(function, result);

    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Atom(LispBuiltin *builtin)
/*
 atom object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (CONSP(object) ? NIL : T);
}

LispObj *
Lisp_Block(LispBuiltin *builtin)
/*
 block name &rest body
 */
{
    int did_jump, *pdid_jump = &did_jump;
    LispObj *res, **pres = &res, **pbody;
    LispBlock *block;

    LispObj *name, *body;

    body = ARGUMENT(1);
    name = ARGUMENT(0);

    if (!SYMBOLP(name) && name != NIL && name != T)
	LispDestroy("%s: %s cannot name a block",
		    STRFUN(builtin), STROBJ(name));

    pbody = &body;
    *pres = NIL;
    *pdid_jump = 1;
    block = LispBeginBlock(name, LispBlockTag);
    if (setjmp(block->jmp) == 0) {
	for (; CONSP(body); body = CDR(body))
	    res = EVAL(CAR(body));
	*pdid_jump = 0;
    }
    LispEndBlock(block);
    if (*pdid_jump)
	*pres = lisp__data.block.block_ret;

    return (res);
}

LispObj *
Lisp_Boundp(LispBuiltin *builtin)
/*
 boundp symbol
 */
{
    LispAtom *atom;

    LispObj *symbol = ARGUMENT(0);

    if (symbol == NIL || symbol == T)
	return (T);

    CHECK_SYMBOL(symbol);

    atom = symbol->data.atom;
    if (atom->package == lisp__data.keyword ||
	(atom->a_object && atom->property->value != UNBOUND))
	return (T);

    return (NIL);
}

LispObj *
Lisp_Butlast(LispBuiltin *builtin)
/*
 butlast list &optional count
 */
{
    GC_ENTER();
    long length, count;
    LispObj *result, *cons, *list, *ocount;

    ocount = ARGUMENT(1);
    list = ARGUMENT(0);

    if (list == NIL)
	return (NIL);
    CHECK_CONS(list);
    if (ocount == NIL)
	count = 1;
    else {
	CHECK_INDEX(ocount);
	count = FIXNUM_VALUE(ocount);
    }
    length = LispLength(list);

    if (count == 0)
	return (list);
    else if (count >= length)
	return (NIL);

    length -= count + 1;
    result = cons = CONS(CAR(list), NIL);
    GC_PROTECT(result);
    for (list = CDR(list); length > 0; list = CDR(list), length--) {
	RPLACD(cons, CONS(CAR(list), NIL));
	cons = CDR(cons);
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Car(LispBuiltin *builtin)
/*
 car list
 */
{
    LispObj *list, *result = NULL;

    list = ARGUMENT(0);

    if (list == NIL)
	result = NIL;
    else {
	CHECK_CONS(list);
	result = CAR(list);
    }

    return (result);
}

LispObj *
Lisp_Case(LispBuiltin *builtin)
/*
 case keyform &rest body
 */
{
    LispObj *result, *code, *keyform, *body, *form;

    body = ARGUMENT(1);
    keyform = ARGUMENT(0);

    result = NIL;
    keyform = EVAL(keyform);

    for (; CONSP(body); body = CDR(body)) {
	code = CAR(body);
	CHECK_CONS(code);

	form = CAR(code);
	if (form == T || form == Ootherwise) {
	    if (CONSP(CDR(body)))
		LispDestroy("%s: %s must be the last clause",
			    STRFUN(builtin), STROBJ(CAR(code)));
	    result = CDR(code);
	    break;
	}
	else if (CONSP(form)) {
	    for (; CONSP(form); form = CDR(form))
		if (XEQL(keyform, CAR(form)) == T) {
		    result = CDR(code);
		    break;
		}
	    if (CONSP(form))	/* if found match */
		break;
	}
	else if (XEQL(keyform, form) == T) {
	    result = CDR(code);
	    break;
	}
    }

    for (body = result; CONSP(body); body = CDR(body))
	result = EVAL(CAR(body));

    return (result);
}

LispObj *
Lisp_Catch(LispBuiltin *builtin)
/*
 catch tag &rest body
 */
{
    int did_jump, *pdid_jump = &did_jump;
    LispObj *res, **pres = &res;
    LispBlock *block;

    LispObj *tag, *body, **pbody;

    body = ARGUMENT(1);
    tag = ARGUMENT(0);

    pbody = &body;
    *pres = NIL;
    *pdid_jump = 1;
    block = LispBeginBlock(tag, LispBlockCatch);
    if (setjmp(block->jmp) == 0) {
	for (; CONSP(body); body = CDR(body))
	    res = EVAL(CAR(body));
	*pdid_jump = 0;
    }
    LispEndBlock(block);
    if (*pdid_jump)
	*pres = lisp__data.block.block_ret;

    return (res);
}

LispObj *
Lisp_Coerce(LispBuiltin *builtin)
/*
 coerce object result-type
 */
{
    LispObj *object, *result_type;

    result_type = ARGUMENT(1);
    object = ARGUMENT(0);

    return (LispCoerce(builtin, object, result_type));
}

LispObj *
Lisp_Cdr(LispBuiltin *builtin)
/*
 cdr list
 */
{
    LispObj *list, *result = NULL;

    list = ARGUMENT(0);

    if (list == NIL)
	result = NIL;
    else {
	CHECK_CONS(list);
	result = CDR(list);
    }

    return (result);
}

LispObj *
Lisp_C_r(LispBuiltin *builtin)
/*
 c[ad]{2,4}r list
 */
{
    char *desc;

    LispObj *list, *result = NULL;

    list = ARGUMENT(0);

    result = list;
    desc = STRFUN(builtin);
    while (desc[1] != 'R')
	++desc;
    while (*desc != 'C') {
	if (result == NIL)
	    break;
	CHECK_CONS(result);
	result = *desc == 'A' ? CAR(result) : CDR(result);
	--desc;
    }

    return (result);
}

LispObj *
Lisp_Cond(LispBuiltin *builtin)
/*
 cond &rest body
 */
{
    LispObj *result, *code, *body;

    body = ARGUMENT(0);

    result = NIL;
    for (; CONSP(body); body = CDR(body)) {
	code = CAR(body);

	CHECK_CONS(code);
	result = EVAL(CAR(code));
	if (result == NIL)
	    continue;
	for (code = CDR(code); CONSP(code); code = CDR(code))
	    result = EVAL(CAR(code));
	break;
    }

    return (result);
}

LispObj *
Lisp_CopyList(LispBuiltin *builtin)
/*
 copy-list list
 */
{
    GC_ENTER();
    LispObj *result, *cons;

    LispObj *list;

    list = ARGUMENT(0);

    if (list == NIL)
	return (list);

    result = cons = CONS(CAR(list), CDR(list));
    GC_PROTECT(result);
    for (list = CDR(list); CONSP(list); list = CDR(list)) {
	RPLACD(cons, CONS(CAR(list), CDR(list)));
	cons = CDR(cons);
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Cons(LispBuiltin *builtin)
/*
 cons car cdr
 */
{
    LispObj *car, *cdr;

    cdr = ARGUMENT(1);
    car = ARGUMENT(0);

    return (CONS(car, cdr));
}

LispObj *
Lisp_Consp(LispBuiltin *builtin)
/*
 consp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (CONSP(object) ? T : NIL);
}

LispObj *
Lisp_Constantp(LispBuiltin *builtin)
/*
 constantp form &optional environment
 */
{
    LispObj *form, *environment;

    environment = ARGUMENT(1);
    form = ARGUMENT(0);

    if (CONSTANTP(form) ||
	(SYMBOLP(form) && form->data.atom->constant) ||
	QUOTEP(form))
	return (T);

    return (NIL);
}

LispObj *
Lisp_Defconstant(LispBuiltin *builtin)
/*
 defconstant name initial-value &optional documentation
 */
{
    LispObj *name, *initial_value, *documentation;

    documentation = ARGUMENT(2);
    initial_value = ARGUMENT(1);
    name = ARGUMENT(0);

    CHECK_SYMBOL(name);
    if (documentation != NIL) {
	CHECK_STRING(documentation);
    }
    LispDefconstant(name, EVAL(initial_value), documentation);

    return (name);
}

LispObj *
Lisp_Defmacro(LispBuiltin *builtin)
/*
 defmacro name lambda-list &rest body
 */
{
    LispArgList *alist;

    LispObj *lambda, *name, *lambda_list, *body;

    body = ARGUMENT(2);
    lambda_list = ARGUMENT(1);
    name = ARGUMENT(0);

    CHECK_SYMBOL(name);
    alist = LispCheckArguments(LispMacro, lambda_list, ATOMID(name));

    if (CONSP(body) && STRINGP(CAR(body))) {
	LispAddDocumentation(name, CAR(body), LispDocFunction);
	body = CDR(body);
    }

    lambda_list = LispListProtectedArguments(alist);
    lambda = LispNewLambda(name, body, lambda_list, LispMacro);

    if (name->data.atom->a_builtin || name->data.atom->a_compiled) {
	if (name->data.atom->a_builtin) {
	    ERROR_CHECK_SPECIAL_FORM(name->data.atom);
	}
	/* redefining these may cause surprises if bytecode
	 * compiled functions references them */
	LispWarning("%s: %s is being redefined", STRFUN(builtin), ATOMID(name));

	LispRemAtomBuiltinProperty(name->data.atom);
    }

    LispSetAtomFunctionProperty(name->data.atom, lambda, alist);
    LispUseArgList(alist);

    return (name);
}

LispObj *
Lisp_Defun(LispBuiltin *builtin)
/*
 defun name lambda-list &rest body
 */
{
    LispArgList *alist;

    LispObj *lambda, *name, *lambda_list, *body;

    body = ARGUMENT(2);
    lambda_list = ARGUMENT(1);
    name = ARGUMENT(0);

    CHECK_SYMBOL(name);
    alist = LispCheckArguments(LispFunction, lambda_list, ATOMID(name));

    if (CONSP(body) && STRINGP(CAR(body))) {
	LispAddDocumentation(name, CAR(body), LispDocFunction);
	body = CDR(body);
    }

    lambda_list = LispListProtectedArguments(alist);
    lambda = LispNewLambda(name, body, lambda_list, LispFunction);

    if (name->data.atom->a_builtin || name->data.atom->a_compiled) {
	if (name->data.atom->a_builtin) {
	    ERROR_CHECK_SPECIAL_FORM(name->data.atom);
	}
	/* redefining these may cause surprises if bytecode
	 * compiled functions references them */
	LispWarning("%s: %s is being redefined", STRFUN(builtin), ATOMID(name));

	LispRemAtomBuiltinProperty(name->data.atom);
    }
    LispSetAtomFunctionProperty(name->data.atom, lambda, alist);
    LispUseArgList(alist);

    return (name);
}

LispObj *
Lisp_Defsetf(LispBuiltin *builtin)
/*
 defsetf function lambda-list &rest body
 */
{
    LispArgList *alist;
    LispObj *obj;
    LispObj *lambda, *function, *lambda_list, *store, *body;

    body = ARGUMENT(2);
    lambda_list = ARGUMENT(1);
    function = ARGUMENT(0);

    CHECK_SYMBOL(function);

    if (body == NIL || (CONSP(body) && STRINGP(CAR(body)))) {
	if (!SYMBOLP(lambda_list))
	    LispDestroy("%s: syntax error %s %s",
			STRFUN(builtin), STROBJ(function), STROBJ(lambda_list));
	if (body != NIL)
	    LispAddDocumentation(function, CAR(body), LispDocSetf);

	LispSetAtomSetfProperty(function->data.atom, lambda_list, NULL);

	return (function);
    }

    alist = LispCheckArguments(LispSetf, lambda_list, ATOMID(function));

    store = CAR(body);
    if (!CONSP(store))
	LispDestroy("%s: %s is a bad store value",
		    STRFUN(builtin), STROBJ(store));
    for (obj = store; CONSP(obj); obj = CDR(obj)) {
	CHECK_SYMBOL(CAR(obj));
    }

    body = CDR(body);
    if (CONSP(body) && STRINGP(CAR(body))) {
	LispAddDocumentation(function, CAR(body), LispDocSetf);
	body = CDR(body);
    }

    lambda = LispNewLambda(function, body, store, LispSetf);
    LispSetAtomSetfProperty(function->data.atom, lambda, alist);
    LispUseArgList(alist);

    return (function);
}

LispObj *
Lisp_Defvar(LispBuiltin *builtin)
/*
 defvar name &optional (initial-value nil bound) documentation
 */
{
    LispObj *name, *initial_value, *bound, *documentation;

    documentation = ARGUMENT(3);
    bound = ARGUMENT(2);
    initial_value = ARGUMENT(1);
    name = ARGUMENT(0);

    CHECK_SYMBOL(name);
    if (documentation != NIL) {
	CHECK_STRING(documentation);
    }

    LispProclaimSpecial(name, bound == T ? EVAL(initial_value) : NULL,
			documentation);

    return (name);
}

LispObj *
Lisp_Delete(LispBuiltin *builtin)
/*
 delete item sequence &key from-end test test-not start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, DELETE, NONE));
}

LispObj *
Lisp_DeleteIf(LispBuiltin *builtin)
/*
 delete-if predicate sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, DELETE, IF));
}

LispObj *
Lisp_DeleteIfNot(LispBuiltin *builtin)
/*
 delete-if-not predicate sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, DELETE, IFNOT));
}

LispObj *
Lisp_DeleteDuplicates(LispBuiltin *builtin)
/*
 delete-duplicates sequence &key from-end test test-not start end key
 */
{
    return (LispDeleteOrRemoveDuplicates(builtin, DELETE));
}

LispObj *
Lisp_Do(LispBuiltin *builtin)
/*
 do init test &rest body
 */
{
    return (LispDo(builtin, 0));
}

LispObj *
Lisp_DoP(LispBuiltin *builtin)
/*
 do* init test &rest body
 */
{
    return (LispDo(builtin, 1));
}

LispObj *
Lisp_Documentation(LispBuiltin *builtin)
/*
 documentation symbol type
 */
{
    Atom_id atom;
    LispObj *symbol, *type;
    LispDocType_t doc_type = LispDocVariable;

    type = ARGUMENT(1);
    symbol = ARGUMENT(0);

    CHECK_SYMBOL(symbol);
    CHECK_SYMBOL(type);
    atom = ATOMID(type);

    if (atom == Svariable)
	doc_type = LispDocVariable;
    else if (atom == Sfunction)
	doc_type = LispDocFunction;
    else if (atom == Sstructure)
	doc_type = LispDocStructure;
    else if (atom == Stype)
	doc_type = LispDocType;
    else if (atom == Ssetf)
	doc_type = LispDocSetf;
    else {
	LispDestroy("%s: unknown documentation type %s",
		    STRFUN(builtin), STROBJ(type));
	/*NOTREACHED*/
	doc_type = 0;
    }

    return (LispGetDocumentation(symbol, doc_type));
}

LispObj *
Lisp_DoList(LispBuiltin *builtin)
{
    return (LispDoListTimes(builtin, 0));
}

LispObj *
Lisp_DoTimes(LispBuiltin *builtin)
{
    return (LispDoListTimes(builtin, 1));
}

LispObj *
Lisp_Elt(LispBuiltin *builtin)
/*
 elt sequence index
 svref sequence index
 */
{
    long offset, length;
    LispObj *result, *sequence, *oindex;

    oindex = ARGUMENT(1);
    sequence = ARGUMENT(0);

    length = LispLength(sequence);

    CHECK_INDEX(oindex);
    offset = FIXNUM_VALUE(oindex);

    if (offset >= length)
	LispDestroy("%s: index %ld too large for sequence length %ld",
		    STRFUN(builtin), offset, length);

    if (STRINGP(sequence))
	result = SCHAR(THESTR(sequence)[offset]);
    else {
	if (ARRAYP(sequence))
	    sequence = sequence->data.array.list;

	for (; offset > 0; offset--, sequence = CDR(sequence))
	    ;
	result = CAR(sequence);
    }

    return (result);
}

LispObj *
Lisp_Endp(LispBuiltin *builtin)
/*
 endp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    if (object == NIL)
	return (T);
    CHECK_CONS(object);

    return (NIL);
}

LispObj *
Lisp_Eq(LispBuiltin *builtin)
/*
 eq left right
 */
{
    LispObj *left, *right;

    right = ARGUMENT(1);
    left = ARGUMENT(0);

    return (XEQ(left, right));
}

LispObj *
Lisp_Eql(LispBuiltin *builtin)
/*
 eql left right
 */
{
    LispObj *left, *right;

    right = ARGUMENT(1);
    left = ARGUMENT(0);

    return (XEQL(left, right));
}

LispObj *
Lisp_Equal(LispBuiltin *builtin)
/*
 equal left right
 */
{
    LispObj *left, *right;

    right = ARGUMENT(1);
    left = ARGUMENT(0);

    return (XEQUAL(left, right));
}

LispObj *
Lisp_Equalp(LispBuiltin *builtin)
/*
 equalp left right
 */
{
    LispObj *left, *right;

    right = ARGUMENT(1);
    left = ARGUMENT(0);

    return (XEQUALP(left, right));
}

LispObj *
Lisp_Error(LispBuiltin *builtin)
/*
 error control-string &rest arguments
 */
{
    LispObj *string, *arglist;

    LispObj *control_string, *arguments;

    arguments = ARGUMENT(1);
    control_string = ARGUMENT(0);

    arglist = CONS(NIL, CONS(control_string, arguments));
    GC_PROTECT(arglist);
    string = APPLY(Oformat, arglist);
    LispDestroy("%s", THESTR(string));
    /*NOTREACHED*/

    /* No need to call GC_ENTER() and GC_LEAVE() macros */
    return (NIL);
}

LispObj *
Lisp_Eval(LispBuiltin *builtin)
/*
 eval form
 */
{
    LispObj *form;

    form = ARGUMENT(0);

    return (EVAL(form));
}

LispObj *
Lisp_Fboundp(LispBuiltin *builtin)
/*
 fboundp symbol
 */
{
    LispAtom *atom;

    LispObj *symbol = ARGUMENT(0);

    CHECK_SYMBOL(symbol);

    atom = symbol->data.atom;
    if (atom->a_function || atom->a_builtin)
	return (T);

    return (NIL);
}

LispObj *
Lisp_Find(LispBuiltin *builtin)
/*
 find item sequence &key from-end test test-not start end key
 */
{
    return (LispFindOrPosition(builtin, FIND, NONE));
}

LispObj *
Lisp_FindIf(LispBuiltin *builtin)
/*
 find-if predicate sequence &key from-end start end key
 */
{
    return (LispFindOrPosition(builtin, FIND, IF));
}

LispObj *
Lisp_FindIfNot(LispBuiltin *builtin)
/*
 find-if-not predicate sequence &key from-end start end key
 */
{
    return (LispFindOrPosition(builtin, FIND, IFNOT));
}

LispObj *
Lisp_Fill(LispBuiltin *builtin)
/*
 fill sequence item &key start end
 */
{
    long i, start, end, length;

    LispObj *sequence, *item, *ostart, *oend;

    oend = ARGUMENT(3);
    ostart = ARGUMENT(2);
    item = ARGUMENT(1);
    sequence = ARGUMENT(0);

    LispCheckSequenceStartEnd(builtin, sequence, ostart, oend,
			      &start, &end, &length);

    if (STRINGP(sequence)) {
	int ch;
	char *string = THESTR(sequence);

	CHECK_SCHAR(item);
	ch = SCHAR_VALUE(item);
	for (i = start; i < end; i++)
	    string[i] = ch;
    }
    else {
	LispObj *list;

	if (CONSP(sequence))
	    list = sequence;
	else
	    list = sequence->data.array.list;

	for (i = 0; i < start; i++, list = CDR(list))
	    ;
	for (; i < end; i++, list = CDR(list))
	    RPLACA(list, item);
    }

    return (sequence);
}

LispObj *
Lisp_Fmakunbound(LispBuiltin *builtin)
/*
 fmkaunbound symbol
 */
{
    LispObj *symbol;

    symbol = ARGUMENT(0);

    CHECK_SYMBOL(symbol);
    if (symbol->data.atom->a_function)
	LispRemAtomFunctionProperty(symbol->data.atom);
    else if (symbol->data.atom->a_builtin)
	LispRemAtomBuiltinProperty(symbol->data.atom);

    return (symbol);
}

LispObj *
Lisp_Funcall(LispBuiltin *builtin)
/*
 funcall function &rest arguments
 */
{
    LispObj *result;

    LispObj *function, *arguments;

    arguments = ARGUMENT(1);
    function = ARGUMENT(0);

    result = APPLY(function, arguments);

    return (result);
}

LispObj *
Lisp_Get(LispBuiltin *builtin)
/*
 get symbol indicator &optional default
 */
{
    LispObj *result;

    LispObj *symbol, *indicator, *defalt;

    defalt = ARGUMENT(2);
    indicator = ARGUMENT(1);
    symbol = ARGUMENT(0);

    if (!SYMBOLP(symbol)) {
	if (symbol == NIL)
	    symbol = ATOM(Snil);
	else if (symbol == T)
	    symbol = ATOM(St);
	else
	    LispDestroy("%s: %s is not a symbol",
			STRFUN(builtin), STROBJ(symbol));
    }

    result = LispGetAtomProperty(symbol->data.atom, indicator);

    if (result != NIL)
	result = CAR(result);
    else
	result = defalt;

    return (result);
}

/*
 * ext::getenv
 */
LispObj *
Lisp_Getenv(LispBuiltin *builtin)
/*
 getenv name
 */
{
    char *value;

    LispObj *name;

    name = ARGUMENT(0);

    CHECK_STRING(name);
    value = getenv(THESTR(name));

    return (value ? STRING(value) : NIL);
}

LispObj *
Lisp_Gc(LispBuiltin *builtin)
/*
 gc &optional car cdr
 */
{
    LispObj *car, *cdr;

    cdr = ARGUMENT(1);
    car = ARGUMENT(0);

    LispGC(car, cdr);

    return (car == NIL && cdr == NIL ? NIL : T);
}

LispObj *
Lisp_Gensym(LispBuiltin *builtin)
/*
 gensym &optional arg
 */
{
    int inc = 1, unreadable = 0;
    char *ptr, *preffix = "G", name[132];
    long counter = LONGINT_VALUE(Ogensym_counter->data.atom->property->value);
    LispObj *symbol;

    LispObj *arg;

    arg = ARGUMENT(0);
    if (arg != NIL) {
	if (STRINGP(arg))
	    preffix = THESTR(arg);
	else {
	    CHECK_INDEX(arg);
	    counter = FIXNUM_VALUE(arg);
	    inc = 0;
	}
    }
    snprintf(name, sizeof(name), "%s%ld", preffix, counter);
    if (strlen(name) >= 128)
	LispDestroy("%s: name %s too long", STRFUN(builtin), name);
    Ogensym_counter->data.atom->property->value = INTEGER(counter + inc);

    /* Check if string can be safely read back */
    for (ptr = name; *ptr; ptr++)
	if (islower(*ptr) || *ptr == '"' || *ptr == '\\' || *ptr == ';' ||
	    *ptr == '#' || *ptr == ',' || *ptr == '@' || *ptr == '(' ||
	    *ptr == ')' || *ptr == '`' || *ptr == '\'' || *ptr == '|' ||
	    *ptr == ':') {
	    unreadable = 1;
	    break;
	}

    symbol = UNINTERNED_ATOM(name);
    symbol->data.atom->unreadable = unreadable;

    return (symbol);
}

LispObj *
Lisp_Go(LispBuiltin *builtin)
/*
 go tag
 */
{
    unsigned blevel = lisp__data.block.block_level;

    LispObj *tag;

    tag = ARGUMENT(0);

    while (blevel) {
	LispBlock *block = lisp__data.block.block[--blevel];

	if (block->type == LispBlockClosure)
	    /* if reached a function call */
	    break;
	if (block->type == LispBlockBody) {
	    lisp__data.block.block_ret = tag;
	    LispBlockUnwind(block);
	    BLOCKJUMP(block);
	}
     }

    LispDestroy("%s: no visible tagbody for %s",
		STRFUN(builtin), STROBJ(tag));
    /*NOTREACHED*/
    return (NIL);
}

LispObj *
Lisp_If(LispBuiltin *builtin)
/*
 if test then &optional else
 */
{
    LispObj *result, *test, *then, *oelse;

    oelse = ARGUMENT(2);
    then = ARGUMENT(1);
    test = ARGUMENT(0);

    test = EVAL(test);
    if (test != NIL)
	result = EVAL(then);
    else
	result = EVAL(oelse);

    return (result);
}

LispObj *
Lisp_Intersection(LispBuiltin *builtin)
/*
 intersection list1 list2 &key test test-not key
 */
{
    return (LispListSet(builtin, INTERSECTION));
}

LispObj *
Lisp_Keywordp(LispBuiltin *builtin)
/*
 keywordp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (KEYWORDP(object) ? T : NIL);
}

LispObj *
Lisp_Lambda(LispBuiltin *builtin)
/*
 lambda lambda-list &rest body
 */
{
    GC_ENTER();
    LispObj *name;
    LispArgList *alist;

    LispObj *lambda, *lambda_list, *body;

    body = ARGUMENT(1);
    lambda_list = ARGUMENT(0);

    alist = LispCheckArguments(LispLambda, lambda_list, ATOMID(Olambda));

    name = OPAQUE(alist, LispArgList_t);
    lambda_list = LispListProtectedArguments(alist);
    GC_PROTECT(name);
    GC_PROTECT(lambda_list);
    lambda = LispNewLambda(name, body, lambda_list, LispLambda);
    LispUseArgList(alist);
    GC_LEAVE();

    return (lambda);
}

LispObj *
Lisp_Last(LispBuiltin *builtin)
/*
 last list &optional count
 */
{
    long count, length;
    LispObj *list, *ocount;

    ocount = ARGUMENT(1);
    list = ARGUMENT(0);

    if (!CONSP(list))
	return (list);

    length = LispLength(list);

    if (ocount == NIL)
	count = 1;
    else {
	CHECK_INDEX(ocount);
	count = FIXNUM_VALUE(ocount);
    }

    if (count >= length)
	return (list);

    length -= count;
    for (; length > 0; length--)
	list = CDR(list);

    return (list);
}

LispObj *
Lisp_Length(LispBuiltin *builtin)
/*
 length sequence
 */
{
    LispObj *sequence;

    sequence = ARGUMENT(0);

    return (FIXNUM(LispLength(sequence)));
}

LispObj *
Lisp_Let(LispBuiltin *builtin)
/*
 let init &rest body
 */
{
    GC_ENTER();
    int head = lisp__data.env.length;
    LispObj *init, *body, *pair, *result, *list, *cons = NIL;

    body = ARGUMENT(1);
    init = ARGUMENT(0);

    CHECK_LIST(init);
    for (list = NIL; CONSP(init); init = CDR(init)) {
	LispObj *symbol, *value;

	pair = CAR(init);
	if (SYMBOLP(pair)) {
	    symbol = pair;
	    value = NIL;
	}
	else {
	    CHECK_CONS(pair);
	    symbol = CAR(pair);
	    CHECK_SYMBOL(symbol);
	    pair = CDR(pair);
	    if (CONSP(pair)) {
		value = CAR(pair);
		if (CDR(pair) != NIL)
		    LispDestroy("%s: too much arguments to initialize %s",
				STRFUN(builtin), STROBJ(symbol));
		value = EVAL(value);
	    }
	    else
		value = NIL;
	}
	pair = CONS(symbol, value);
	if (list == NIL) {
	    list = cons = CONS(pair, NIL);
	    GC_PROTECT(list);
	}
	else {
	    RPLACD(cons, CONS(pair, NIL));
	    cons = CDR(cons);
	}
    }
    /* Add variables */
    for (; CONSP(list); list = CDR(list)) {
	pair = CAR(list);
	CHECK_CONSTANT(CAR(pair));
	LispAddVar(CAR(pair), CDR(pair));
	++lisp__data.env.head;
    }
    /* Values of symbols are now protected */
    GC_LEAVE();

    /* execute body */
    for (result = NIL; CONSP(body); body = CDR(body))
	result = EVAL(CAR(body));

    lisp__data.env.head = lisp__data.env.length = head;

    return (result);
}

LispObj *
Lisp_LetP(LispBuiltin *builtin)
/*
 let* init &rest body
 */
{
    int head = lisp__data.env.length;
    LispObj *init, *body, *pair, *result;

    body = ARGUMENT(1);
    init = ARGUMENT(0);

    CHECK_LIST(init);
    for (; CONSP(init); init = CDR(init)) {
	LispObj *symbol, *value;

	pair = CAR(init);
	if (SYMBOLP(pair)) {
	    symbol = pair;
	    value = NIL;
	}
	else {
	    CHECK_CONS(pair);
	    symbol = CAR(pair);
	    CHECK_SYMBOL(symbol);
	    pair = CDR(pair);
	    if (CONSP(pair)) {
		value = CAR(pair);
		if (CDR(pair) != NIL)
		    LispDestroy("%s: too much arguments to initialize %s",
				STRFUN(builtin), STROBJ(symbol));
		value = EVAL(value);
	    }
	    else
		value = NIL;
	}

	CHECK_CONSTANT(symbol);
	LispAddVar(symbol, value);
	++lisp__data.env.head;
    }

    /* execute body */
    for (result = NIL; CONSP(body); body = CDR(body))
	result = EVAL(CAR(body));

    lisp__data.env.head = lisp__data.env.length = head;

    return (result);
}

LispObj *
Lisp_List(LispBuiltin *builtin)
/*
 list &rest args
 */
{
    LispObj *args;

    args = ARGUMENT(0);

    return (args);
}

LispObj *
Lisp_ListP(LispBuiltin *builtin)
/*
 list* object &rest more-objects
 */
{
    GC_ENTER();
    LispObj *result, *cons;

    LispObj *object, *more_objects;

    more_objects = ARGUMENT(1);
    object = ARGUMENT(0);

    if (!CONSP(more_objects))
	return (object);

    result = cons = CONS(object, CAR(more_objects));
    GC_PROTECT(result);
    for (more_objects = CDR(more_objects); CONSP(more_objects);
	 more_objects = CDR(more_objects)) {
	object = CAR(more_objects);
	RPLACD(cons, CONS(CDR(cons), object));
	cons = CDR(cons);
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Listp(LispBuiltin *builtin)
/*
 listp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (object == NIL || CONSP(object) ? T : NIL);
}

LispObj *
LispListSet(LispBuiltin *builtin, int function)
/*
 intersection list1 list2 &key test test-not key
 set-difference list1 list2 &key test test-not key
 nset-difference list1 list2 &key test test-not key
 set-exclusive-or list1 list2 &key test test-not key
 subsetp list1 list2 &key test test-not key
 union list1 list2 &key test test-not key
 */
{
    GC_ENTER();
    int code, inplace, setdifference;
    LispObj *lambda, *expect, *result, *cmp, *cmp1, *cmp2,
	    *item, *value, *clist1, *clist2, *cons, *cdr;

    LispObj *list1, *list2, *test, *test_not, *key;

    key = ARGUMENT(4);
    test_not = ARGUMENT(3);
    test = ARGUMENT(2);
    list2 = ARGUMENT(1);
    list1 = ARGUMENT(0);

    /* Check if arguments are valid lists */
    CHECK_LIST(list1);
    CHECK_LIST(list2);

    setdifference = function == SETDIFFERENCE || function == NSETDIFFERENCE;

    /* Check for fast return */
    if (list1 == NIL)
	return (setdifference || function == INTERSECTION ?
		NIL : function == SUBSETP ? T : list2);
    if (list2 == NIL)
	return (function == INTERSECTION || function == UNION ||
		function == SUBSETP ?
		NIL : list1);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy("%s: specify either :TEST or :TEST-NOT", STRFUN(builtin));

    /* Resolve comparison function, and expected result of comparison */
    if (test_not == NIL) {
	if (test == NIL)
	    lambda = Oeql;
	else
	    lambda = test;
	expect = T;
    }
    else {
	lambda = test_not;
	expect = NIL;
    }
    code = FCODE(lambda);
    inplace = function == NSETDIFFERENCE; 
    clist1 = cdr = NIL;

    /* Make a copy of list2 with the key predicate applied */
    if (key != NIL) {
	result = cons = CONS(APPLY1(key, CAR(list2)), NIL);
	GC_PROTECT(result);
	for (cmp2 = CDR(list2); CONSP(cmp2); cmp2 = CDR(cmp2)) {
	    item = APPLY1(key, CAR(cmp2));
	    RPLACD(cons, CONS(APPLY1(key, CAR(cmp2)), NIL));
	    cons = CDR(cons);
	}
	clist2 = result;
	result = cons = NIL;
    }
    else {
	result = cons = NIL;
	clist2 = list2;
    }

    /* Compare elements of lists
     * Logic:
     *	   UNION
     *		1) Walk list1 and if CAR(list1) not in list2, add it to result
     *		2) Add list2 to result
     *	   INTERSECTION
     *		1) Walk list1 and if CAR(list1) in list2, add it to result
     *	   SET-DIFFERENCE
     *		1) Walk list1 and if CAR(list1) not in list2, add it to result
     *	   SET-EXCLUSIVE-OR
     *		1) Walk list1 and if CAR(list1) not in list2, add it to result
     *		2) Walk list2 and if CAR(list2) not in list1, add it to result
     *	   SUBSETP
     *		1) Walk list1 and if CAR(list1) not in list2, return NIL
     *		2) Return T
     */
    value = NIL;
    for (cmp1 = list1; CONSP(cmp1); cmp1 = CDR(cmp1)) {
	item = CAR(cmp1);

	/* Apply key predicate if required */
	if (key != NIL) {
	    cmp = APPLY1(key, item);
	    if (function == SETEXCLUSIVEOR) {
		if (clist1 == NIL) {
		    clist1 = cdr = CONS(cmp, NIL);
		    GC_PROTECT(clist1);
		}
		else {
		    RPLACD(cdr, CONS(cmp, NIL));
		    cdr = CDR(cdr);
		}
	    }
	}
	else
	    cmp = item;

	/* Compare against list2 */
	for (cmp2 = clist2; CONSP(cmp2); cmp2 = CDR(cmp2)) {
	    value = FCOMPARE(lambda, cmp, CAR(cmp2), code);
	    if (value != NIL)
		value = T;
	    if (value == expect)
		break;
	}

	if (function == SUBSETP) {
	    /* Element of list1 not in list2? */
	    if (value != expect) {
		GC_LEAVE();

		return (NIL);
	    }
	}
	/* If need to add item to result */
	else if (((setdifference ||
	      function == UNION ||
	      function == SETEXCLUSIVEOR) &&
	     value != expect) ||
	    (function == INTERSECTION && value == expect)) {
	    if (inplace) {
		if (result == NIL) {
		    RPLACA(list1, CAR(cmp1));
		    RPLACD(list1, CDR(cmp1));
		    result = cons = cmp1;
		}
		else {
		    RPLACD(cons, cmp1);
		    cons = cmp1;
		}
	    }
	    else {
		if (result == NIL) {
		    result = cons = CONS(item, NIL);
		    GC_PROTECT(result);
		}
		else {
		    RPLACD(cons, CONS(item, NIL));
		    cons = CDR(cons);
		}
	    }
	}
    }

    if (function == SUBSETP) {
	GC_LEAVE();

	return (T);
    }
    else if (function == UNION) {
	/* Add list2 to tail of result */
	if (result == NIL)
	    result = list2;
	else
	    RPLACD(cons, list2);
    }
    else if (function == SETEXCLUSIVEOR) {
	for (cmp2 = list2; CONSP(cmp2); cmp2 = CDR(cmp2)) {
	    item = CAR(cmp2);

	    if (key != NIL) {
		cmp = CAR(clist2);
		/* XXX changing clist2 */
		clist2 = CDR(clist2);
		cmp1 = clist1;
	    }
	    else {
		cmp = item;
		cmp1 = list1;
	    }

	    /* Compare against list1 */
	    for (; CONSP(cmp1); cmp1 = CDR(cmp1)) {
		value = FCOMPARE(lambda, cmp, CAR(cmp1), code);
		if (value != NIL)
		    value = T;
		if (value == expect)
		    break;
	    }

	    if (value != expect) {
		/* Variable result may be NIL if all
		 * elements of list1 are also in list2 */
		if (result == NIL) {
		    result = cons = CONS(item, NIL);
		    GC_PROTECT(result);
		}
		else {
		    RPLACD(cons, CONS(item, NIL));
		    cons = CDR(cons);
		}
	    }
	}
    }
    else if (function == NSETDIFFERENCE && CONSP(cons))
	RPLACD(cons, NIL);

    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Loop(LispBuiltin *builtin)
/*
 loop &rest body
 */
{
    LispObj *code, *result;
    LispBlock *block;

    LispObj *body;

    body = ARGUMENT(0);

    result = NIL;
    block = LispBeginBlock(NIL, LispBlockTag);
    if (setjmp(block->jmp) == 0) {
	for (;;)
	    for (code = body; CONSP(code); code = CDR(code))
		(void)EVAL(CAR(code));
    }
    LispEndBlock(block);
    result = lisp__data.block.block_ret;

    return (result);
}

/* XXX This function is broken, needs a review
 (being delayed until true array/vectors be implemented) */
LispObj *
Lisp_MakeArray(LispBuiltin *builtin)
/*
 make-array dimensions &key element-type initial-element initial-contents
			    adjustable fill-pointer displaced-to
			    displaced-index-offset
 */
{
    long rank = 0, count = 1, offset, zero, c;
    LispObj *obj, *dim, *array;
    LispType type;

    LispObj *dimensions, *element_type, *initial_element, *initial_contents,
	    *adjustable, *fill_pointer, *displaced_to,
	    *displaced_index_offset;

    dim = array = NIL;
    type = LispNil_t;

    displaced_index_offset = ARGUMENT(7);
    displaced_to = ARGUMENT(6);
    fill_pointer = ARGUMENT(5);
    adjustable = ARGUMENT(4);
    initial_contents = ARGUMENT(3);
    initial_element = ARGUMENT(2);
    element_type = ARGUMENT(1);
    dimensions = ARGUMENT(0);

    if (INDEXP(dimensions)) {
	dim = CONS(dimensions, NIL);
	rank = 1;
	count = FIXNUM_VALUE(dimensions);
    }
    else if (CONSP(dimensions)) {
	dim = dimensions;

	for (rank = 0; CONSP(dim); rank++, dim = CDR(dim)) {
	    obj = CAR(dim);
	    CHECK_INDEX(obj);
	    count *= FIXNUM_VALUE(obj);
	}
	dim = dimensions;
    }
    else if (dimensions == NIL) {
	dim = NIL;
	rank = count = 0;
    }
    else
	LispDestroy("%s: %s is a bad array dimension",
		    STRFUN(builtin), STROBJ(dimensions));

    /* check element-type */
    if (element_type != NIL) {
	if (element_type == T)
	    type = LispNil_t;
	else if (!SYMBOLP(element_type))
	    LispDestroy("%s: unsupported element type %s",
			STRFUN(builtin), STROBJ(element_type));
	else {
	    Atom_id atom = ATOMID(element_type);

	    if (atom == Satom)
		type = LispAtom_t;
	    else if (atom == Sinteger)
		type = LispInteger_t;
	    else if (atom == Scharacter)
		type = LispSChar_t;
	    else if (atom == Sstring)
		type = LispString_t;
	    else if (atom == Slist)
		type = LispCons_t;
	    else if (atom == Sopaque)
		type = LispOpaque_t;
	    else
		LispDestroy("%s: unsupported element type %s",
			    STRFUN(builtin), ATOMID(element_type));
	}
    }

    /* check initial-contents */
    if (rank) {
	CHECK_LIST(initial_contents);
    }

    /* check displaced-to */
    if (displaced_to != NIL) {
	CHECK_ARRAY(displaced_to);
    }

    /* check displaced-index-offset */
    offset = -1;
    if (displaced_index_offset != NIL) {
	CHECK_INDEX(displaced_index_offset);
	offset = FIXNUM_VALUE(displaced_index_offset);
    }

    c = 0;
    if (initial_element != NIL)
	++c;
    if (initial_contents != NIL)
	++c;
    if (displaced_to != NIL || offset >= 0)
	++c;
    if (c > 1)
	LispDestroy("%s: more than one initialization specified",
		    STRFUN(builtin));

    zero = count == 0;
    if (displaced_to != NIL) {
	if (offset < 0)
	    offset = 0;
	for (c = 1, obj = displaced_to->data.array.dim; obj != NIL;
	     obj = CDR(obj))
	    c *= FIXNUM_VALUE(CAR(obj));
	if (c < count + offset)
	    LispDestroy("%s: array-total-size + displaced-index-offset "
			"exceeds total size", STRFUN(builtin));
	for (c = 0, array = displaced_to->data.array.list; c < offset; c++)
	    array = CDR(array);
    }
    else if (initial_contents != NIL) {
	if (rank == 0)
	    array = initial_contents;
	else if (rank == 1) {
	    for (array = initial_contents, c = 0; c < count;
		 array = CDR(array), c++)
		if (!CONSP(array))
		    LispDestroy("%s: bad argument or size %s",
				STRFUN(builtin), STROBJ(array));
	    if (array != NIL)
		LispDestroy("%s: bad argument or size %s",
			    STRFUN(builtin), STROBJ(array));
	    array = initial_contents;
	}
	else {
	    LispObj *err = NIL;
	    /* check if list matches */
	    int i, j, k, *dims, *loop;

	    /* create iteration variables */
	    dims = LispMalloc(sizeof(int) * rank);
	    loop = LispCalloc(1, sizeof(int) * (rank - 1));
	    for (i = 0, obj = dim; CONSP(obj); i++, obj = CDR(obj))
		dims[i] = FIXNUM_VALUE(CAR(obj));

	    /* check if list matches specified dimensions */
	    while (loop[0] < dims[0]) {
		for (obj = initial_contents, i = 0; i < rank - 1; i++) {
		    for (j = 0; j < loop[i]; j++)
			obj = CDR(obj);
		    err = obj;
		    if (!CONSP(obj = CAR(obj)))
			goto make_array_error;
		    err = obj;
		}
		--i;
		for (;;) {
		    ++loop[i];
		    if (i && loop[i] >= dims[i])
			loop[i] = 0;
		    else
			break;
		    --i;
		}
		for (k = 0; k < dims[rank - 1]; obj = CDR(obj), k++) {
		    if (!CONSP(obj))
			goto make_array_error;
		}
		if (obj == NIL)
		    continue;
make_array_error:
		LispFree(dims);
		LispFree(loop);
		LispDestroy("%s: bad argument or size %s",
			    STRFUN(builtin), STROBJ(err));
	    }

	    /* list is correct, use it to fill initial values */

	    /* reset loop */
	    memset(loop, 0, sizeof(int) * (rank - 1));

	    GCDisable();
	    /* fill array with supplied values */
	    array = NIL;
	    while (loop[0] < dims[0]) {
		for (obj = initial_contents, i = 0; i < rank - 1; i++) {
		    for (j = 0; j < loop[i]; j++)
			obj = CDR(obj);
		    obj = CAR(obj);
		}
		--i;
		for (;;) {
		    ++loop[i];
		    if (i && loop[i] >= dims[i])
			loop[i] = 0;
		    else
			break;
		    --i;
		}
		for (k = 0; k < dims[rank - 1]; obj = CDR(obj), k++) {
		    if (array == NIL)
			array = CONS(CAR(obj), NIL);
		    else {
			RPLACD(array, CONS(CAR(array), CDR(array)));
			RPLACA(array, CAR(obj));
		    }
		}
	    }
	    LispFree(dims);
	    LispFree(loop);
	    array = LispReverse(array);
	    GCEnable();
	}
    }
    else {
	GCDisable();
	/* allocate array */
	if (count) {
	    --count;
	    array = CONS(initial_element, NIL);
	    while (count) {
		RPLACD(array, CONS(CAR(array), CDR(array)));
		RPLACA(array, initial_element);
		count--;
	    }
	}
	GCEnable();
    }

    obj = LispNew(array, dim);
    obj->type = LispArray_t;
    obj->data.array.list = array;
    obj->data.array.dim = dim;
    obj->data.array.rank = rank;
    obj->data.array.type = type;
    obj->data.array.zero = zero;

    return (obj);
}

LispObj *
Lisp_MakeList(LispBuiltin *builtin)
/*
 make-list size &key initial-element
 */
{
    GC_ENTER();
    long count;
    LispObj *result, *cons;

    LispObj *size, *initial_element;

    initial_element = ARGUMENT(1);
    size = ARGUMENT(0);

    CHECK_INDEX(size);
    count = FIXNUM_VALUE(size);

    if (count == 0)
	return (NIL);

    result = cons = CONS(initial_element, NIL);
    GC_PROTECT(result);
    for (; count > 1; count--) {
	RPLACD(cons, CONS(initial_element, NIL));
	cons = CDR(cons);
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Makunbound(LispBuiltin *builtin)
/*
 makunbound symbol
 */
{
    LispObj *symbol;

    symbol = ARGUMENT(0);

    CHECK_SYMBOL(symbol);
    LispUnsetVar(symbol);

    return (symbol);
}

LispObj *
Lisp_Mapcar(LispBuiltin *builtin)
/*
 mapcar function list &rest more-lists
 */
{
    GC_ENTER();
    long i, offset, count, length;
    LispObj *result = NIL, *cons, *arguments, *acons, *rest, *alist;

    LispObj *function, *list, *more_lists;

    more_lists = ARGUMENT(2);
    list = ARGUMENT(1);
    function = ARGUMENT(0);

    /* Result will be no longer than this */
    for (length = 0, alist = list; CONSP(alist); length++, alist = CDR(alist))
	;

    /* If first argument is not a list... */
    if (length == 0)
	return (NIL);

    /* At least one argument will be passed to function, count how many
     * extra arguments will be used, and calculate result length. */
    count = 0;
    for (rest = more_lists; CONSP(rest); rest = CDR(rest), count++) {

	/* Check if extra list is really a list, and if it is smaller
	 * than the first list */
	for (i = 0, alist = CAR(rest);
	     i < length && CONSP(alist);
	     i++, alist = CDR(alist))
	    ;

	/* If it is not a true list */
	if (i == 0)
	    return (NIL);

	/* If it is smaller than the currently calculated result length */
	if (i < length)
	    length = i;
    }

    /* Initialize gc protected object cells for resulting list */
    result = cons = CONS(NIL, NIL);
    GC_PROTECT(result);

    /* Initialize gc protected object cells for argument list */
    arguments = acons = CONS(NIL, NIL);
    GC_PROTECT(arguments);

    /* Allocate space for extra arguments */
    for (; count > 0; count--) {
	RPLACD(acons, CONS(NIL, NIL));
	acons = CDR(acons);
    }

    /* For every element of the list that will be used */
    for (offset = 0; ; list = CDR(list)) {
	acons = arguments;

	/* Add first argument */
	RPLACA(acons, CAR(list));
	acons = CDR(acons);

	/* For every extra list argument */
	for (rest = more_lists; CONSP(rest); rest = CDR(rest)) {

	    /* Find the cons cell of the next argument */
	    for (alist = CAR(rest), count = 0;
		 count < offset;
		 alist = CDR(alist), count++)
		;

	    /* Add element to argument list */
	    RPLACA(acons, CAR(alist));
	    acons = CDR(acons);
	}

	/* Store result */
	RPLACA(cons, APPLY(function, arguments));

	/* Allocate new result cell if required */
	if (++offset < length) {
	    RPLACD(cons, CONS(NIL, NIL));
	    cons = CDR(cons);
	}
	else
	    break;
    }

    /* Unprotect argument and result list */
    GC_LEAVE();

    return (result);
}

/* XXX maplist and extra map* functions need to be rewritten in the
 * format of the new mapcar version. */
LispObj *
Lisp_Maplist(LispBuiltin *builtin)
/*
 maplist function list &rest more-lists
 */
{
    int i, count, length;
    LispObj *code, *cons, *acons, *object, *result;

    LispObj *function, *list, *more_lists;

    more_lists = ARGUMENT(2);
    list = ARGUMENT(1);
    function = ARGUMENT(0);

    length = lisp__data.protect.length;
    if (length + 2 >= lisp__data.protect.space)
	LispMoreProtects();
    lisp__data.protect.objects[lisp__data.protect.length++] = NIL;
    lisp__data.protect.objects[lisp__data.protect.length++] = NIL;

    result = acons = NIL;
    for (count = 0; CONSP(list); count++, list = CDR(list)) {
	cons = CONS(QUOTE(list), NIL);
	code = CONS(function, cons);
	lisp__data.protect.objects[length] = code;
	for (object = more_lists; CONSP(object); object = CDR(object)) {
	    LispObj *arglist = CAR(object);

	    if (!CONSP(arglist))
		goto maplist_done;
	    for (i = count; i > 0; i--) {
		if (!CONSP(arglist = CDR(arglist)))
		    goto maplist_done;
	    }
	    RPLACD(cons, CONS(QUOTE(arglist), NIL));
	}
	cons = EVAL(code);
	if (result == NIL) {
	    result = acons = CONS(cons, NIL);
	    lisp__data.protect.objects[length + 1] = result;
	}
	else {
	    RPLACD(acons, CONS(cons, NIL));
	    acons = CDR(acons);
	}
    }

maplist_done:
    lisp__data.protect.length = length;

    return (result);
}

LispObj *
Lisp_Member(LispBuiltin *builtin)
/*
 member item list &key test test-not key
 */
{
    return (LispAssocOrMember(builtin, MEMBER, NONE));
}

LispObj *
Lisp_MemberIf(LispBuiltin *builtin)
/*
 member-if predicate list &key key
 */
{
    return (LispAssocOrMember(builtin, MEMBER, IF));
}

LispObj *
Lisp_MemberIfNot(LispBuiltin *builtin)
/*
 member-if-not predicate list &key key
 */
{
    return (LispAssocOrMember(builtin, MEMBER, IFNOT));
}

LispObj *
Lisp_MultipleValueBind(LispBuiltin *builtin)
/*
 multiple-value-bind symbols values &rest body
 */
{
    int i, head = lisp__data.env.length;
    LispObj *result, *symbol, *value;

    LispObj *symbols, *values, *body;

    body = ARGUMENT(2);
    values = ARGUMENT(1);
    symbols = ARGUMENT(0);

    result = EVAL(values);
    for (i = -1; CONSP(symbols); symbols = CDR(symbols), i++) {
	symbol = CAR(symbols);
	CHECK_SYMBOL(symbol);
	CHECK_CONSTANT(symbol);
	if (i >= 0 && i < RETURN_COUNT)
	    value = RETURN(i);
	else if (i < 0)
	    value = result;
	else
	    value = NIL;
	LispAddVar(symbol, value);
	++lisp__data.env.head;
    }

    /* Execute code with binded variables (if any) */
    for (result = NIL; CONSP(body); body = CDR(body))
	result = EVAL(CAR(body));

    lisp__data.env.head = lisp__data.env.length = head;

    return (result);
}

LispObj *
Lisp_MultipleValueList(LispBuiltin *builtin)
/*
 multiple-value-list form
 */
{
    int i;
    GC_ENTER();
    LispObj *form, *result, *cons;

    form = ARGUMENT(0);

    result = EVAL(form);

    if (RETURN_COUNT < 0)
	return (NIL);

    result = cons = CONS(result, NIL);
    GC_PROTECT(result);
    for (i = 0; i < RETURN_COUNT; i++) {
	RPLACD(cons, CONS(RETURN(i), NIL));
	cons = CDR(cons);
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_MultipleValueSetq(LispBuiltin *builtin)
/*
 multiple-value-setq symbols form
 */
{
    int i;
    LispObj *result, *symbol, *value;

    LispObj *symbols, *form;

    form = ARGUMENT(1);
    symbols = ARGUMENT(0);

    CHECK_LIST(symbols);
    result = EVAL(form);
    if (CONSP(symbols)) {
	symbol = CAR(symbols);
	CHECK_SYMBOL(symbol);
	CHECK_CONSTANT(symbol);
	LispSetVar(symbol, result);
	symbols = CDR(symbols);
    }
    for (i = 0; CONSP(symbols); symbols = CDR(symbols), i++) {
	symbol = CAR(symbols);
	CHECK_SYMBOL(symbol);
	CHECK_CONSTANT(symbol);
	if (i < RETURN_COUNT && RETURN_COUNT > 0)
	    value = RETURN(i);
	else
	    value = NIL;
	LispSetVar(symbol, value);
    }

    return (result);
}

LispObj *
Lisp_Nconc(LispBuiltin *builtin)
/*
 nconc &rest lists
 */
{
    LispObj *list, *lists, *tail;

    lists = ARGUMENT(0);

    if (lists == NIL)
	return (NIL);

    /* first check if all arguments but last are lists */
    for (tail = lists; CONSP(CDR(tail)); tail = CDR(tail)) {
	list = CAR(tail);
	CHECK_LIST(list);
    }

    /* skip initial NIL lists */
    for (list = CAR(lists); CONSP(lists); lists = CDR(lists))
	if ((list = CAR(lists)) != NIL)
	    break;

    if (list != NIL && CONSP(list)) {
	for (tail = list; CONSP(CDR(tail)); tail = CDR(tail))
	    ;
	for (lists = CDR(lists); CONSP(lists); lists = CDR(lists)) {
	    RPLACD(tail, CAR(lists));
	    for (; CONSP(CDR(tail)); tail = CDR(tail))
		;
	}
	/* if last argument is not a list */
	if (lists != NIL)
	    RPLACD(tail, lists);
    }

    return (list);
}

LispObj *
Lisp_Nreverse(LispBuiltin *builtin)
/*
 nreverse sequence
 */
{
    return (LispXReverse(builtin, 1));
}

LispObj *
Lisp_NsetDifference(LispBuiltin *builtin)
/*
 nset-difference list1 list2 &key test test-not key
 */
{
    return (LispListSet(builtin, NSETDIFFERENCE));
}

LispObj *
Lisp_Nsubstitute(LispBuiltin *builtin)
/*
 nsubstitute newitem olditem sequence &key from-end test test-not start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, NSUBSTITUTE, NONE));
}

LispObj *
Lisp_NsubstituteIf(LispBuiltin *builtin)
/*
 nsubstitute-if newitem test sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, NSUBSTITUTE, IF));
}

LispObj *
Lisp_NsubstituteIfNot(LispBuiltin *builtin)
/*
 nsubstitute-if-not newitem test sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, NSUBSTITUTE, IFNOT));
}

LispObj *
Lisp_Nth(LispBuiltin *builtin)
/*
 nth index list
 */
{
    long position;
    LispObj *oindex, *list;

    list = ARGUMENT(1);
    oindex = ARGUMENT(0);

    CHECK_INDEX(oindex);
    position = FIXNUM_VALUE(oindex);

    if (list == NIL)
	return (NIL);

    CHECK_CONS(list);
    for (; position > 0; position--) {
	if (!CONSP(list))
	    return (NIL);
	list = CDR(list);
    }

    return (CONSP(list) ? CAR(list) : NIL);
}

LispObj *
Lisp_Nthcdr(LispBuiltin *builtin)
/*
 nthcdr index list
 */
{
    long position;
    LispObj *oindex, *list;

    list = ARGUMENT(1);
    oindex = ARGUMENT(0);

    CHECK_INDEX(oindex);
    position = FIXNUM_VALUE(oindex);

    if (list == NIL)
	return (NIL);
    CHECK_CONS(list);

    for (; position > 0; position--) {
	if (!CONSP(list))
	    return (NIL);
	list = CDR(list);
    }

    return (list);
}

LispObj *
Lisp_Null(LispBuiltin *builtin)
/*
 null list
 */
{
    LispObj *list;

    list = ARGUMENT(0);

    return (list == NIL ? T : NIL);
}

LispObj *
Lisp_Or(LispBuiltin *builtin)
/*
 or &rest args
 */
{
    LispObj *result = NIL, *args;

    args = ARGUMENT(0);

    for (; CONSP(args); args = CDR(args)) {
	result = EVAL(CAR(args));
	if (result != NIL)
	    break;
    }

    return (result);
}

LispObj *
LispFindOrPosition(LispBuiltin *builtin,
		   int function, int comparison)
/*
 find item sequence &key from-end test test-not start end key
 find-if predicate sequence &key from-end start end key
 find-if-not predicate sequence &key from-end start end key
 position item sequence &key from-end test test-not start end key
 position-if predicate sequence &key from-end start end key
 position-if-not predicate sequence &key from-end start end key
 */
{
    GC_ENTER();
    int code = 0, istring;
    char *string = NULL;
    long offset = -1, start, end, length, i = comparison == NONE ? 7 : 5;
    LispObj *cmp, *element, *result, **objects = NULL;

    LispObj *item, *predicate, *sequence, *from_end,
	    *test, *test_not, *ostart, *oend, *key;

    key = ARGUMENT(i);		--i;
    oend = ARGUMENT(i);		--i;
    ostart = ARGUMENT(i);	--i;
    if (comparison == NONE) {
	test_not = ARGUMENT(i);	--i;
	test = ARGUMENT(i);	--i;
    }
    else
	test_not = test = NIL;
    from_end = ARGUMENT(i);	--i;
    sequence = ARGUMENT(i);	--i;
    if (comparison == NONE) {
	item = ARGUMENT(i);
	predicate = Oeql;
    }
    else {
	predicate = ARGUMENT(i);
	item = NIL;
    }

    LispCheckSequenceStartEnd(builtin, sequence, ostart, oend,
			      &start, &end, &length);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy("%s: specify either :TEST or :TEST-NOT", STRFUN(builtin));

    if (comparison == NONE) {
	if (test != NIL)
	    predicate = test;
	else if (test_not != NIL)
	    predicate = test_not;
	code = FCODE(predicate);
    }

    cmp = element = NIL;
    istring = STRINGP(sequence);
    if (istring) {
	if (comparison == NONE) {
	    CHECK_SCHAR(item);
	}
	string = THESTR(sequence);
    }
    else {
	if (!CONSP(sequence))
	    sequence = sequence->data.array.list;
	for (i = 0; i < start; i++)
	    sequence = CDR(sequence);
    }

    if ((length = end - start) == 0)
	return (NIL);

    if (from_end != NIL && !istring) {
	objects = LispMalloc(sizeof(LispObj*) * length);
	for (i = length - 1; i >= 0; i--, sequence = CDR(sequence))
	    objects[i] = CAR(sequence);
    }

    for (i = 0; i < length; i++) {
	if (istring)
	    element = SCHAR(string[from_end == NIL ? i + start : end - i - 1]);
	else
	    element = from_end == NIL ? CAR(sequence) : objects[i];

	if (key != NIL) {
	    cmp = APPLY1(key, element);
	    GC_PROTECT(cmp);
	}
	else
	    cmp = element;

	/* Update list */
	if (!istring && from_end == NIL)
	    sequence = CDR(sequence);

	if (comparison == NONE)
	    result = FCOMPARE(predicate, item, cmp, code);
	else
	    result = APPLY1(predicate, cmp);

	/* Unprotect cmp */
	GC_LEAVE();

	if ((result == NIL &&
	     (comparison == IFNOT ||
	      (comparison == NONE && test_not != NIL))) ||
	    (result != NIL &&
	     (comparison == IF ||
	      (comparison == NONE && test_not == NIL)))) {
	    offset = from_end == NIL ? i + start : end - i - 1;
	    break;
	}
    }

    if (from_end != NIL && !istring)
	LispFree(objects);

    return (offset == -1 ? NIL : function == FIND ? element : FIXNUM(offset));
}

LispObj *
Lisp_Pop(LispBuiltin *builtin)
/*
 pop place
 */
{
    LispObj *result, *value;

    LispObj *place;

    place = ARGUMENT(0);

    if (SYMBOLP(place)) {
	result = LispGetVar(place);
	if (result == NULL)
	    LispDestroy("EVAL: the variable %s is unbound", STROBJ(place));
	CHECK_CONSTANT(place);
	if (result != NIL) {
	    CHECK_CONS(result);
	    value = CDR(result);
	    result = CAR(result);
	}
	else
	    value = NIL;
	LispSetVar(place, value);
    }
    else {
	GC_ENTER();
	LispObj quote;

	result = EVAL(place);
	if (result != NIL) {
	    CHECK_CONS(result);
	    value = CDR(result);
	    GC_PROTECT(value);
	    result = CAR(result);
	}
	else
	    value = NIL;
	quote.type = LispQuote_t;
	quote.data.quote = value;
	APPLY2(Osetf, place, &quote);
	GC_LEAVE();
    }

    return (result);
}

LispObj *
Lisp_Position(LispBuiltin *builtin)
/*
 position item sequence &key from-end test test-not start end key
 */
{
    return (LispFindOrPosition(builtin, POSITION, NONE));
}

LispObj *
Lisp_PositionIf(LispBuiltin *builtin)
/*
 position-if predicate sequence &key from-end start end key
 */
{
    return (LispFindOrPosition(builtin, POSITION, IF));
}

LispObj *
Lisp_PositionIfNot(LispBuiltin *builtin)
/*
 position-if-not predicate sequence &key from-end start end key
 */
{
    return (LispFindOrPosition(builtin, POSITION, IFNOT));
}

LispObj *
Lisp_Proclaim(LispBuiltin *builtin)
/*
 proclaim declaration
 */
{
    LispObj *arguments, *object;
    char *operation;

    LispObj *declaration;

    declaration = ARGUMENT(0);

    CHECK_CONS(declaration);

    arguments = declaration;
    object = CAR(arguments);
    CHECK_SYMBOL(object);

    operation = ATOMID(object);
    if (strcmp(operation, "SPECIAL") == 0) {
	for (arguments = CDR(arguments); CONSP(arguments);
	     arguments = CDR(arguments)) {
	    object = CAR(arguments);
	    CHECK_SYMBOL(object);
	    LispProclaimSpecial(object, NULL, NIL);
	}
    }
    else if (strcmp(operation, "TYPE") == 0) {
	/* XXX no type checking yet, but should be added */
    }
    /* else do nothing */

    return (NIL);
}

LispObj *
Lisp_Prog1(LispBuiltin *builtin)
/*
 prog1 first &rest body
 */
{
    GC_ENTER();
    LispObj *result;

    LispObj *first, *body;

    body = ARGUMENT(1);
    first = ARGUMENT(0);

    result = EVAL(first);

    GC_PROTECT(result);
    for (; CONSP(body); body = CDR(body))
	(void)EVAL(CAR(body));
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Prog2(LispBuiltin *builtin)
/*
 prog2 first second &rest body
 */
{
    GC_ENTER();
    LispObj *result;

    LispObj *first, *second, *body;

    body = ARGUMENT(2);
    second = ARGUMENT(1);
    first = ARGUMENT(0);

    (void)EVAL(first);
    result = EVAL(second);
    GC_PROTECT(result);
    for (; CONSP(body); body = CDR(body))
	(void)EVAL(CAR(body));
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Progn(LispBuiltin *builtin)
/*
 progn &rest body
 */
{
    LispObj *result = NIL;

    LispObj *body;

    body = ARGUMENT(0);

    for (; CONSP(body); body = CDR(body))
	result = EVAL(CAR(body));

    return (result);
}

LispObj *
Lisp_Progv(LispBuiltin *builtin)
/*
 progv symbols values &rest body
 */
{
    int head = lisp__data.env.length;
    GC_ENTER();
    LispObj *res = NIL, *cons = NIL, *valist = NIL;
    LispObj *symbols, *values, *body;

    body = ARGUMENT(2);
    values = ARGUMENT(1);
    symbols = ARGUMENT(0);

    /* get symbol names */
    symbols = EVAL(symbols);
    GC_PROTECT(symbols);

    /* get symbol values */
    values = EVAL(values);
    GC_PROTECT(values);

    /* fill variable list */
    for (; CONSP(symbols); symbols = CDR(symbols)) {
	if (!CONSP(values))
	    break;
	CHECK_SYMBOL(CAR(symbols));
	if (valist == NIL) {
	    valist = cons = CONS(CONS(CAR(symbols), CAR(values)), NIL);
	    GC_PROTECT(valist);
	}
	else {
	    RPLACD(cons, CONS(CONS(CAR(symbols), CAR(values)), NIL));
	    cons = CDR(cons);
	}
	values = CDR(values);
    }

    /* add variables */
    for (; valist != NIL; valist = CDR(valist)) {
	cons = CAR(valist);
	CHECK_CONSTANT(CAR(cons));
	LispAddVar(CAR(cons), CDR(cons));
	++lisp__data.env.head;
    }
    GC_LEAVE();

    for (; CONSP(body); body = CDR(body))
	res = EVAL(CAR(body));

    lisp__data.env.head = lisp__data.env.length = head;

    return (res);
}

LispObj *
Lisp_Provide(LispBuiltin *builtin)
/*
 provide module
 */
{
    LispObj *module, *obj;

    module = ARGUMENT(0);

    CHECK_STRING(module);
    for (obj = MOD; obj != NIL; obj = CDR(obj)) {
	if (STRLEN(CAR(obj)) == STRLEN(module) &&
	    memcmp(THESTR(CAR(obj)), THESTR(module), STRLEN(module)) == 0)
	    return (module);
    }

    if (MOD == NIL)
	MOD = CONS(module, NIL);
    else {
	RPLACD(MOD, CONS(CAR(MOD), CDR(MOD)));
	RPLACA(MOD, module);
    }

    LispSetVar(lisp__data.modules, MOD);

    return (MOD);
}

LispObj *
Lisp_Push(LispBuiltin *builtin)
/*
 push item place
 */
{
    LispObj *result, *list;

    LispObj *item, *place;

    place = ARGUMENT(1);
    item = ARGUMENT(0);

    item = EVAL(item);

    if (SYMBOLP(place)) {
	list = LispGetVar(place);
	if (list == NULL)
	    LispDestroy("EVAL: the variable %s is unbound", STROBJ(place));
	CHECK_CONSTANT(place);
	LispSetVar(place, result = CONS(item, list));
    }
    else {
	GC_ENTER();
	LispObj quote;

	list = EVAL(place);
	result = CONS(item, list);
	GC_PROTECT(result);
	quote.type = LispQuote_t;
	quote.data.quote = result;
	APPLY2(Osetf, place, &quote);
	GC_LEAVE();
    }

    return (result);
}

LispObj *
Lisp_Pushnew(LispBuiltin *builtin)
/*
 pushnew item place &key key test test-not
 */
{
    GC_ENTER();
    LispObj *result, *list;

    LispObj *item, *place, *key, *test, *test_not;

    test_not = ARGUMENT(4);
    test = ARGUMENT(3);
    key = ARGUMENT(2);
    place = ARGUMENT(1);
    item = ARGUMENT(0);

    /* Evaluate place */
    if (SYMBOLP(place)) {
	list = LispGetVar(place);
	if (list == NULL)
	    LispDestroy("EVAL: the variable %s is unbound", STROBJ(place));
	/* Do error checking now. */
	CHECK_CONSTANT(place);
    }
    else
	/* It is possible that list is not gc protected? */
	list = EVAL(place);
 
    item = EVAL(item);
    GC_PROTECT(item);
    if (key != NIL) {
	key = EVAL(key);
	GC_PROTECT(key);
    }
    if (test != NIL) {
	test = EVAL(test);
	GC_PROTECT(test);
    }
    else if (test_not != NIL) {
	test_not = EVAL(test_not);
	GC_PROTECT(test_not);
    }

    result = LispAdjoin(builtin, item, list, key, test, test_not);

    /* Item already in list */
    if (result == list) {
	GC_LEAVE();

	return (result);
    }

    if (SYMBOLP(place)) {
	CHECK_CONSTANT(place);
	LispSetVar(place, result);
    }
    else {
	LispObj quote;

	GC_PROTECT(result);
	quote.type = LispQuote_t;
	quote.data.quote = result;
	APPLY2(Osetf, place, &quote);
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Quit(LispBuiltin *builtin)
/*
 quit &optional status
 */
{
    int status = 0;
    LispObj *ostatus;

    ostatus = ARGUMENT(0);

    if (FIXNUMP(ostatus))
	status = (int)FIXNUM_VALUE(ostatus);
    else if (ostatus != NIL)
	LispDestroy("%s: bad exit status argument %s",
		    STRFUN(builtin), STROBJ(ostatus));

    exit(status);
}

LispObj *
Lisp_Quote(LispBuiltin *builtin)
/*
 quote object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (object);
}

LispObj *
Lisp_Replace(LispBuiltin *builtin)
/*
 replace sequence1 sequence2 &key start1 end1 start2 end2
 */
{
    long length, length1, length2, start1, end1, start2, end2;
    LispObj *sequence1, *sequence2, *ostart1, *oend1, *ostart2, *oend2;

    oend2 = ARGUMENT(5);
    ostart2 = ARGUMENT(4);
    oend1 = ARGUMENT(3);
    ostart1 = ARGUMENT(2);
    sequence2 = ARGUMENT(1);
    sequence1 = ARGUMENT(0);

    LispCheckSequenceStartEnd(builtin, sequence1, ostart1, oend1,
			      &start1, &end1, &length1);
    LispCheckSequenceStartEnd(builtin, sequence2, ostart2, oend2,
			      &start2, &end2, &length2);

    if (start1 == end1 || start2 == end2)
	return (sequence1);

    length = end1 - start1;
    if (length > end2 - start2)
	length = end2 - start2;

    if (STRINGP(sequence1)) {
	if (!STRINGP(sequence2))
	    LispDestroy("%s: cannot store %s in %s",
			STRFUN(builtin), STROBJ(sequence2), THESTR(sequence1));

	memcpy(THESTR(sequence1) + start1, THESTR(sequence2) + start2, length);
    }
    else {
	int i;
	LispObj *from, *to;

	if (ARRAYP(sequence1))
	    sequence1 = sequence1->data.array.list;
	if (ARRAYP(sequence2))
	    sequence2 = sequence2->data.array.list;

	/* adjust pointers */
	for (i = 0, from = sequence2; i < start2; i++, from = CDR(from))
	    ;
	for (i = 0, to = sequence1; i < start1; i++, to = CDR(to))
	    ;

	/* copy data */
	for (i = 0; i < length; i++, from = CDR(from), to = CDR(to))
	    RPLACA(to, CAR(from));
    }

    return (sequence1);
}

LispObj *
LispDeleteOrRemoveDuplicates(LispBuiltin *builtin, int function)
/*
 delete-duplicates sequence &key from-end test test-not start end key
 remove-duplicates sequence &key from-end test test-not start end key
 */
{
    GC_ENTER();
    int code;
    long i, j, start, end, length, count;
    LispObj *lambda, *expect, *result, *cons, *value, *compare;

    LispObj *sequence, *from_end, *test, *test_not, *ostart, *oend, *key;

    key = ARGUMENT(6);
    oend = ARGUMENT(5);
    ostart = ARGUMENT(4);
    test_not = ARGUMENT(3);
    test = ARGUMENT(2);
    from_end = ARGUMENT(1);
    sequence = ARGUMENT(0);

    LispCheckSequenceStartEnd(builtin, sequence, ostart, oend,
			      &start, &end, &length);

    /* Check if need to do something */
    if (start == end)
	return (sequence);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy("%s: specify either :TEST or :TEST-NOT", STRFUN(builtin));

    /* Resolve comparison function, and expected result of comparison */
    if (test_not == NIL) {
	if (test == NIL)
	    lambda = Oeql;
	else
	    lambda = test;
	expect = T;
    }
    else {
	lambda = test_not;
	expect = NIL;
    }
    code = FCODE(lambda);

    /* Initialize */
    count = 0;

    result = cons = NIL;
    if (STRINGP(sequence)) {
	char *ptr, *string, *buffer = LispMalloc(length + 1);

	/* Use same code, update start/end offsets */
	if (from_end != NIL) {
	    i = length - start;
	    start = length - end;
	    end = i;
	}

	if (from_end == NIL)
	    string = THESTR(sequence);
	else {
	    /* Make a reversed copy of the sequence */
	    string = LispMalloc(length + 1);
	    for (ptr = THESTR(sequence) + length - 1, i = 0; i < length; i++)
		string[i] = *ptr--;
	    string[i] = '\0';
	}

	ptr = buffer;
	/* Copy leading bytes */
	for (i = 0; i < start; i++)
	    *ptr++ = string[i];

	value = SCHAR(string[i]);
	if (key != NIL)
	    value = APPLY1(key, value);
	result = cons = CONS(value, NIL);
	GC_PROTECT(result);
	for (++i; i < end; i++) {
	    value = SCHAR(string[i]);
	    if (key != NIL)
		value = APPLY1(key, value);
	    RPLACD(cons, CONS(value, NIL));
	    cons = CDR(cons);
	}

	for (i = start; i < end; i++, result = CDR(result)) {
	    compare = CAR(result);
	    for (j = i + 1, cons = CDR(result); j < end; j++, cons = CDR(cons)) {
		value = FCOMPARE(lambda, compare, CAR(cons), code);
		if (value != NIL)
		    value = T;
		if (value == expect)
		    break;
	    }
	    if (value != expect)
		*ptr++ = string[i];
	    else
		++count;
	}

	if (count) {
	    /* Copy ending bytes */
	    for (; i <= length; i++)   /* Also copy the ending nul */
		*ptr++ = string[i];

	    if (from_end == NIL)
		ptr = buffer;
	    else {
		for (i = 0, ptr = buffer + strlen(buffer);
		     ptr > buffer;
		     i++)
		    string[i] = *--ptr;
		string[i] = '\0';
		ptr = string;
	    }
	    if (function == REMOVE)
		result = STRING2(ptr);
	    else {
		result = sequence;
		free(THESTR(result));
		THESTR(result) = ptr;
		LispMused(ptr);
	    }
	}
	else
	    result = sequence;
	if (!count || from_end != NIL) {
	    LispFree(buffer);
	    if (count)
		LispFree(string);
	}
    }
    else {
	long xlength = end - start;
	LispObj *list, *object, **kobjects = NULL, **xobjects;
	LispObj **objects = LispMalloc(sizeof(LispObj*) * xlength);

	if (!CONSP(sequence))
	    object = sequence->data.array.list;
	else
	    object = sequence;
	list = object;

	for (i = 0; i < start; i++)
	    object = CDR(object);

	/* Put data in a vector */
	if (from_end == NIL) {
	    for (i = 0; i < xlength; i++, object = CDR(object))
		objects[i] = CAR(object);
	}
	else {
	    for (i = xlength - 1; i >= 0; i--, object = CDR(object))
		objects[i] = CAR(object);
	}

	/* Apply key predicate if required */
	if (key != NIL) {
	    kobjects = LispMalloc(sizeof(LispObj*) * xlength);
	    for (i = 0; i < xlength; i++) {
		kobjects[i] = APPLY1(key, objects[i]);
		GC_PROTECT(kobjects[i]);
	    }
	    xobjects = kobjects;
	}
	else
	    xobjects = objects;

	/* Check if needs to remove something */
	for (i = 0; i < xlength; i++) {
	    compare = xobjects[i];
	    for (j = i + 1; j < xlength; j++) {
		value = FCOMPARE(lambda, compare, xobjects[j], code);
		if (value != NIL)
		    value = T;
		if (value == expect) {
		    objects[i] = NULL;
		    ++count;
		    break;
		}
	    }
	}

	if (count) {
	    /* Create/set result list */
	    object = list;

	    if (start) {
		/* Skip first elements of resulting list */
		if (function == REMOVE) {
		    result = cons = CONS(CAR(object), NIL);
		    GC_PROTECT(result);
		    for (i = 1, object = CDR(object);
			 i < start;
			 i++, object = CDR(object)) {
			RPLACD(cons, CONS(CAR(object), NIL));
			cons = CDR(cons);
		    }
		}
		else {
		    result = cons = object;
		    for (i = 1; i < start; i++, cons = CDR(cons))
			;
		}
	    }
	    else if (function == DELETE)
		result = list;

	    /* Skip initial removed elements */
	    if (function == REMOVE) {
		for (i = 0; objects[i] == NULL && i < xlength; i++)
		    ;
	    }
	    else
		i = 0;

	    if (i < xlength) {
		int xstart, xlimit, xinc;

		if (from_end == NIL) {
		    xstart = i;
		    xlimit = xlength;
		    xinc = 1;
		}
		else {
		    xstart = xlength - 1;
		    xlimit = i - 1;
		    xinc = -1;
		}

		if (function == REMOVE) {
		    for (i = xstart; i != xlimit; i += xinc) {
			if (objects[i] != NULL) {
			    if (result == NIL) {
				result = cons = CONS(objects[i], NIL);
				GC_PROTECT(result);
			    }
			    else {
				RPLACD(cons, CONS(objects[i], NIL));
				cons = CDR(cons);
			    }
			}
		    }
		}
		else {
		    /* Delete duplicates */
		    for (i = xstart; i != xlimit; i += xinc) {
			if (objects[i] == NULL) {
			    if (cons == NIL) {
				if (CONSP(CDR(result))) {
				    RPLACA(result, CADR(result));
				    RPLACD(result, CDDR(result));
				}
				else {
				    RPLACA(result, CDR(result));
				    RPLACD(result, NIL);
				}
			    }
			    else {
				if (CONSP(CDR(cons)))
				    RPLACD(cons, CDDR(cons));
				else
				    RPLACD(cons, NIL);
			    }
			}
			else {
			    if (cons == NIL)
				cons = result;
			    else
				cons = CDR(cons);
			}
		    }
		}
	    }
	    if (end < length && function == REMOVE) {
		for (i = start; i < end; i++, object = CDR(object))
		    ;
		if (result == NIL) {
		    result = cons = CONS(CAR(object), NIL);
		    GC_PROTECT(result);
		    ++i;
		    object = CDR(object);
		}
		for (; i < length; i++, object = CDR(object)) {
		    RPLACD(cons, CONS(CAR(object), NIL));
		    cons = CDR(cons);
		}
	    }
	}
	else
	    result = sequence;
	LispFree(objects);
	if (key != NIL)
	    LispFree(kobjects);

	if (count && !CONSP(sequence)) {
	    if (function == REMOVE)
		result = VECTOR(result);
	    else {
		length = FIXNUM_VALUE(CAR(sequence->data.array.dim)) - count;
		CAR(sequence->data.array.dim) = FIXNUM(length);
		result = sequence;
	    }
	}
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_RemoveDuplicates(LispBuiltin *builtin)
/*
 remove-duplicates sequence &key from-end test test-not start end key
 */
{
    return (LispDeleteOrRemoveDuplicates(builtin, REMOVE));
}



LispObj *
LispDeleteRemoveXSubstitute(LispBuiltin *builtin,
			    int function, int comparison)
/*
 delete item sequence &key from-end test test-not start end count key
 delete-if predicate sequence &key from-end start end count key
 delete-if-not predicate sequence &key from-end start end count key
 remove item sequence &key from-end test test-not start end count key
 remove-if predicate sequence &key from-end start end count key
 remove-if-not predicate sequence &key from-end start end count key
 substitute newitem olditem sequence &key from-end test test-not start end count key
 substitute-if newitem test sequence &key from-end start end count key
 substitute-if-not newitem test sequence &key from-end start end count key
 nsubstitute newitem olditem sequence &key from-end test test-not start end count key
 nsubstitute-if newitem test sequence &key from-end start end count key
 nsubstitute-if-not newitem test sequence &key from-end start end count key
 */
{
    GC_ENTER();
    int code, inplace, substitute;
    long i, j, start, end, length, copy, count, xstart, xend, xinc, xlength;

    LispObj *result, *expect, *compare, *value;

    LispObj *item, *newitem, *predicate, *sequence, *from_end,
	    *test, *test_not, *ostart, *oend, *ocount, *key;

    substitute = function == SUBSTITUTE || function == NSUBSTITUTE;
    if (!substitute)
	i = comparison == NONE ? 8 : 6;
    else /* substitute */
	i = comparison == NONE ? 9 : 7;

    /* Get function arguments */
    key = ARGUMENT(i);			--i;
    ocount = ARGUMENT(i);		--i;
    oend = ARGUMENT(i);			--i;
    ostart = ARGUMENT(i);		--i;
    if (comparison == NONE) {
	test_not = ARGUMENT(i);		--i;
	test = ARGUMENT(i);		--i;
    }
    else
	test_not = test = NIL;
    from_end = ARGUMENT(i);		--i;
    sequence = ARGUMENT(i);		--i;
    if (comparison != NONE) {
	predicate = ARGUMENT(i);	--i;
	if (substitute)
	    newitem = ARGUMENT(0);
	else
	    newitem = NIL;
	item = NIL;
    }
    else {
	predicate = NIL;
	if (substitute) {
	    item = ARGUMENT(1);
	    newitem = ARGUMENT(0);
	}
	else {
	    item = ARGUMENT(0);
	    newitem = NIL;
	}
    }

    /* Check if argument is a valid sequence, and if start/end
     * are correctly specified. */
    LispCheckSequenceStartEnd(builtin, sequence, ostart, oend,
			      &start, &end, &length);

    /* Check count argument */
    if (ocount == NIL) {
	count = length;
	/* Doesn't matter, but left to right should be slightly faster */
	from_end = NIL;
    }
    else {
	CHECK_INDEX(ocount);
	count = FIXNUM_VALUE(ocount);
    }

    /* Check if need to do something */
    if (start == end || count == 0)
	return (sequence);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy("%s: specify either :TEST or :TEST-NOT", STRFUN(builtin));

    /* Resolve comparison function, and expected result of comparison */
    if (comparison == NONE) {
	if (test_not == NIL) {
	    if (test == NIL)
		predicate = Oeql;
	    else
		predicate = test;
	    expect = T;
	}
	else {
	    predicate = test_not;
	    expect = NIL;
	}
    }
    else
	expect = comparison == IFNOT ? NIL : T;

    /* Check for fast path to comparison function */
    code = FCODE(predicate);

    /* Initialize for loop */
    copy = count;
    result = sequence;
    inplace = function == DELETE || function == NSUBSTITUTE;
    xlength = end - start;

    /* String is easier */
    if (STRINGP(sequence)) {
	char *buffer, *string;

	if (comparison == NONE) {
	    CHECK_SCHAR(item);
	}
	if (substitute) {
	    CHECK_SCHAR(newitem);
	}

	if (from_end == NIL) {
	    xstart = start;
	    xend = end;
	    xinc = 1;
	}
	else {
	    xstart = end - 1;
	    xend = start - 1;
	    xinc = -1;
	}

	string = THESTR(sequence);
	buffer = LispMalloc(length + 1);

	/* Copy leading bytes, if any */
	for (i = 0; i < start; i++)
	    buffer[i] = string[i];

	for (j = xstart; i != xend && count > 0; i += xinc) {
	    compare = SCHAR(string[i]);
	    if (key != NIL) {
		compare = APPLY1(key, compare);
		/* Value returned by the key predicate may not be protected */
		GC_PROTECT(compare);
		if (comparison == NONE)
		    value = FCOMPARE(predicate, item, compare, code);
		else
		    value = APPLY1(predicate, compare);
		/* Unprotect value returned by the key predicate */
		GC_LEAVE();
	    }
	    else {
		if (comparison == NONE)
		    value = FCOMPARE(predicate, item, compare, code);
		else
		    value = APPLY1(predicate, compare);
	    }
	    if (value != NIL)
		value = T;

	    if (value != expect) {
		buffer[j] = string[i];
		j += xinc;
	    }
	    else {
		if (substitute) {
		    buffer[j] = SCHAR_VALUE(newitem);
		    j += xinc;
		}
		else
		    --count;
	    }
	}

	if (count != copy && from_end != NIL)
	    memmove(buffer + start, buffer + copy - count, count);

	/* Copy remaining bytes, if any */
	for (; i < length; i++, j++)
	    buffer[j] = string[i];
	buffer[j] = '\0';

	xlength = length - (copy - count);
	if (inplace) {
	    /* result is a pointer to sequence */
	    LispFree(THESTR(sequence));
	    LispMused(buffer);
	    THESTR(sequence) = buffer;
	    STRLEN(sequence) = xlength;
	}
	else
	    result = LSTRING2(buffer, xlength);
    }

    /* If inplace, need to update CAR and CDR of sequence */
    else {
	LispObj *list, *object;
	LispObj **objects = LispMalloc(sizeof(LispObj*) * xlength);

	if (!CONSP(sequence))
	    list = sequence->data.array.list;
	else
	    list = sequence;

	/* Put data in a vector */
	for (i = 0, object = list; i < start; i++)
	    object = CDR(object);

	for (i = 0; i < xlength; i++, object = CDR(object))
	    objects[i] = CAR(object);

	if (from_end == NIL) {
	    xstart = 0;
	    xend = xlength;
	    xinc = 1;
	}
	else {
	    xstart = xlength - 1;
	    xend = -1;
	    xinc = -1;
	}

	/* Check if needs to remove something */
	for (i = xstart; i != xend && count > 0; i += xinc) {
	    compare = objects[i];
	    if (key != NIL) {
		compare = APPLY1(key, compare);
		GC_PROTECT(compare);
		if (comparison == NONE)
		    value = FCOMPARE(predicate, item, compare, code);
		else
		    value = APPLY1(predicate, compare);
		GC_LEAVE();
	    }
	    else {
		if (comparison == NONE)
		    value = FCOMPARE(predicate, item, compare, code);
		else
		    value = APPLY1(predicate, compare);
	    }
	    if (value != NIL)
		value = T;
	    if (value == expect) {
		if (substitute)
		    objects[i] = newitem;
		else
		    objects[i] = NULL;
		--count;
	    }
	}

	if (copy != count) {
	    LispObj *cons = NIL;

	    i = 0;
	    object = list;
	    if (inplace) {
		/* While result is NIL, skip initial elements of sequence */
		result = start ? list : NIL;

		/* Skip initial elements, if any */
		for (; i < start; i++, cons = object, object = CDR(object))
		    ;
	    }
	    /* Copy initial elements, if any */
	    else {
		result = NIL;
		if (start) {
		    result = cons = CONS(CAR(list), NIL);
		    GC_PROTECT(result);
		    for (++i, object = CDR(list);
			 i < start;
			 i++, object = CDR(object)) {
			RPLACD(cons, CONS(CAR(object), NIL));
		 	cons = CDR(cons);
		    }
		}
	    }

	    /* Skip initial removed elements, if any */
	    for (i = 0; objects[i] == NULL && i < xlength; i++)
		;

	    for (i = 0; i < xlength; i++, object = CDR(object)) {
		if (objects[i]) {
		    if (inplace) {
			if (result == NIL)
			    result = cons = object;
			else {
			    RPLACD(cons, object);
			    cons = CDR(cons);
			}
			if (function == NSUBSTITUTE)
			    RPLACA(cons, objects[i]);
		    }
		    else {
			if (result == NIL) {
			    result = cons = CONS(objects[i], NIL);
			    GC_PROTECT(result);
			}
			else {
			    RPLACD(cons, CONS(objects[i], NIL));
			    cons = CDR(cons);
			}
		    }
		}
	    }

	    if (inplace) {
		if (result == NIL)
		    result = object;
		else
		    RPLACD(cons, object);

		if (!CONSP(sequence)) {
		    result = sequence;
		    CAR(result)->data.array.dim =
			FIXNUM(length - (copy - count));
		}
	    }
	    else if (end < length) {
		i = end;
		/* Copy ending elements, if any */
		if (result == NIL) {
		    result = cons = CONS(CAR(object), NIL);
		    GC_PROTECT(result);
		    object = CDR(object);
		    i++;
		}
		for (; i < length; i++, object = CDR(object)) {
		    RPLACD(cons, CONS(CAR(object), NIL));
		    cons = CDR(cons);
		}
	    }
	}

	/* Release comparison vector */
	LispFree(objects);
    }

    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Remove(LispBuiltin *builtin)
/*
 remove item sequence &key from-end test test-not start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, REMOVE, NONE));
}

LispObj *
Lisp_RemoveIf(LispBuiltin *builtin)
/*
 remove-if predicate sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, REMOVE, IF));
}

LispObj *
Lisp_RemoveIfNot(LispBuiltin *builtin)
/*
 remove-if-not predicate sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, REMOVE, IFNOT));
}

LispObj *
Lisp_Return(LispBuiltin *builtin)
/*
 return &optional result
 */
{
    unsigned blevel = lisp__data.block.block_level;

    LispObj *result;

    result = ARGUMENT(0);

    while (blevel) {
	LispBlock *block = lisp__data.block.block[--blevel];

	if (block->type == LispBlockClosure)
	    /* if reached a function call */
	    break;
	if (block->type == LispBlockTag && block->tag == NIL) {
	    RETURN_COUNT = 0;
	    lisp__data.block.block_ret = EVAL(result);
	    LispBlockUnwind(block);
	    BLOCKJUMP(block);
	}
    }
    LispDestroy("%s: no visible NIL block", STRFUN(builtin));

    /*NOTREACHED*/
    return (NIL);
}

LispObj *
Lisp_ReturnFrom(LispBuiltin *builtin)
/*
 return-from name &optional result
 */
{
    unsigned blevel = lisp__data.block.block_level;

    LispObj *name, *result;

    result = ARGUMENT(1);
    name = ARGUMENT(0);

    if (name != NIL && name != T && !SYMBOLP(name))
	LispDestroy("%s: %s is not a valid block name",
		    STRFUN(builtin), STROBJ(name));

    while (blevel) {
	int jmp = 1;
	LispBlock *block = lisp__data.block.block[--blevel];

	if (name == block->tag) {
	    switch (OBJECT_TYPE(name)) {
		case LispNil_t:
		case LispAtom_t:
		    jmp = name == block->tag;
		    break;
		default:
		    /* only atom, nil or t can be used */
		    jmp = 0;
		    break;
	    }
	    if (jmp &&
		(block->type == LispBlockTag ||
		 block->type == LispBlockClosure)) {
		RETURN_COUNT = 0;
		lisp__data.block.block_ret = EVAL(result);
		LispBlockUnwind(block);
		BLOCKJUMP(block);
	    }
	    if (block->type == LispBlockClosure)
		/* can use return-from only in the current function */
		break;
	}
    }
    LispDestroy("%s: no visible block named %s",
		STRFUN(builtin), STROBJ(name));

    /*NOTREACHED*/
    return (NIL);
}

LispObj *
LispXReverse(LispBuiltin *builtin, int inplace)
/*
 nreverse sequence
 reverse sequence
 */
{
    long length;
    LispObj *list, *result = NIL;

    LispObj *sequence;

    sequence = ARGUMENT(0);

    /* Do error checking for arrays and object type. */
    length = LispLength(sequence);
    if (length <= 1)
	return (sequence);

    switch (XOBJECT_TYPE(sequence)) {
	case LispString_t: {
	    long i;
	    char *from, *to;

	    from = THESTR(sequence) + length - 1;
	    if (inplace) {
		char temp;

		to = THESTR(sequence);
		for (i = 0; i < length / 2; i++) {
		    temp = to[i];
		    to[i] = from[-i];
		    from[-i] = temp;
		}
		result = sequence;
	    }
	    else {
		to = LispMalloc(length + 1);
		to[length] = '\0';
		for (i = 0; i < length; i++)
		    to[i] = from[-i];
		result = STRING2(to);
	    }
	}   return (result);
	case LispCons_t:
	    if (inplace) {
		long i, j;
		LispObj *temp;

		/* For large lists this can be very slow, but for small
		 * amounts of data, this avoid allocating a buffer to
		 * to store the CAR of the sequence. This is only done
		 * to not destroy the contents of a variable.
		 */
		for (i = 0, list = sequence;
		     i < (length + 1) / 2;
		     i++, list = CDR(list))
		    ;
		length /= 2;
		for (i = 0; i < length; i++, list = CDR(list)) {
		    for (j = length - i - 1, result = sequence;
			 j > 0;
			 j--, result = CDR(result))
			;
		    temp = CAR(list);
		    RPLACA(list, CAR(result));
		    RPLACA(result, temp);
		}
		return (sequence);
	    }
	    list = sequence;
	    break;
	case LispArray_t:
	    if (inplace) {
		sequence->data.array.list =
		    LispReverse(sequence->data.array.list);
		return (sequence);
	    }
	    list = sequence->data.array.list;
	    break;
	default:	/* LispNil_t */
	    return (result);
    }

    {
	GC_ENTER();
	LispObj *cons;

	result = cons = CONS(CAR(list), NIL);
	GC_PROTECT(result);
	for (list = CDR(list); CONSP(list); list = CDR(list)) {
	    RPLACD(cons, CONS(CAR(list), NIL));
	    cons = CDR(cons);
	}
	result = LispReverse(result);

	GC_LEAVE();
    }

    if (ARRAYP(sequence)) {
	list = result;

	result = LispNew(list, NIL);
	result->type = LispArray_t;
	result->data.array.list = list;
	result->data.array.dim = sequence->data.array.dim;
	result->data.array.rank = sequence->data.array.rank;
	result->data.array.type = sequence->data.array.type;
	result->data.array.zero = sequence->data.array.zero;
    }

    return (result);
}

LispObj *
Lisp_Reverse(LispBuiltin *builtin)
/*
 reverse sequence
 */
{
    return (LispXReverse(builtin, 0));
}

LispObj *
Lisp_Rplaca(LispBuiltin *builtin)
/*
 rplaca place value
 */
{
    LispObj *place, *value;

    value = ARGUMENT(1);
    place = ARGUMENT(0);

    CHECK_CONS(place);
    RPLACA(place, value);

    return (place);
}

LispObj *
Lisp_Rplacd(LispBuiltin *builtin)
/*
 rplacd place value
 */
{
    LispObj *place, *value;

    value = ARGUMENT(1);
    place = ARGUMENT(0);

    CHECK_CONS(place);
    RPLACD(place, value);

    return (place);
}

LispObj *
Lisp_Search(LispBuiltin *builtin)
/*
 search sequence1 sequence2 &key from-end test test-not key start1 start2 end1 end2
 */
{
    int istring, code = 0;
    long start1, start2, end1, end2, length1, length2, offset = -1;
    char *string1 = NULL, *string2 = NULL;
    LispObj *list1 = NIL, *list2 = NIL;
    LispObj *cmp1, *cmp2, *lambda, *value, *expect;

    LispObj *sequence1, *sequence2, *from_end, *test, *test_not,
	    *key, *ostart1, *ostart2, *oend1, *oend2;

    oend2 = ARGUMENT(9);
    oend1 = ARGUMENT(8);
    ostart2 = ARGUMENT(7);
    ostart1 = ARGUMENT(6);
    key = ARGUMENT(5);
    test_not = ARGUMENT(4);
    test = ARGUMENT(3);
    from_end = ARGUMENT(2);
    sequence2 = ARGUMENT(1);
    sequence1 = ARGUMENT(0);

    LispCheckSequenceStartEnd(builtin, sequence1, ostart1, oend1,
			      &start1, &end1, &length1);
    LispCheckSequenceStartEnd(builtin, sequence2, ostart2, oend2,
			      &start2, &end2, &length2);

    /* Check if both arguments are or aren't strings */
    if ((STRINGP(sequence1)) ^ (STRINGP(sequence2)))
	return (NIL);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy("%s: specify either :TEST or :TEST-NOT", STRFUN(builtin));

    /* Check for special conditions */
    if (start1 == end1)
	return (FIXNUM(end2));
    else if (start2 == end2)
	return (start1 == end1 ? FIXNUM(start2) : NIL);

    /* Resolve comparison function, and expected result of comparison */
    if (test_not == NIL) {
	if (test == NIL)
	    lambda = Oeql;
	else
	    lambda = test;
	expect = T;
    }
    else {
	lambda = test_not;
	expect = NIL;
    }
    code = FCODE(lambda);

    /* Searching a substring? */
    istring = STRINGP(sequence1);

    if (istring) {
	string1 = THESTR(sequence1);
	string2 = THESTR(sequence2);
    }
    else {
	list1 = sequence1;
	list2 = sequence2;
	if (!CONSP(list1))
	    list1 = list1->data.array.list;
	if (!CONSP(list2))
	    list2 = list2->data.array.list;
    }

    length1 = end1 - start1;
    length2 = end2 - start2;

    if (from_end == NIL) {
	LispObj *slist1, *slist2;
	long cstart1, cstart2;

	slist1 = list1;
	slist2 = list2;
	cstart2 = start2;
	while (end2 - start2 >= length1) {
	    list1 = slist1;
	    list2 = slist2;
	    cstart1 = start1;
	    cstart2 = start2;
	    while (cstart1 < end1) {
		if (istring) {
		    cmp1 = SCHAR(string1[cstart1]);
		    cmp2 = SCHAR(string2[cstart2]);
		}
		else {
		    cmp1 = CAR(list1);
		    cmp2 = CAR(list2);
		}
		if (key != NIL) {
		    cmp1 = APPLY1(key, cmp1);
		    cmp2 = APPLY1(key, cmp2);
		}

		/* Compare elements */
		if ((value = FCOMPARE(lambda, cmp1, cmp2, code)) != NIL)
		    value = T;
		if (value != expect)
		    break;

		/* Update offsets/sequence pointers */
		++cstart1;
		++cstart2;
		if (!istring) {
		    list1 = CDR(list1);
		    list2 = CDR(list2);
		}
	    }

	    /* If all elements matched */
	    if (cstart1 == end1) {
		offset = cstart2 - length1;
		break;
	    }

	    /* Update offset/sequence2 pointer */
	    ++start2;
	    if (!istring)
		slist2 = CDR(slist2);
	}
    }
    else {
	LispObj **plist1 = NULL, **plist2 = NULL;
	long i, coff1, coff2;

	/* Allocate vector of list elements to search backwards */
	if (!istring) {
	    plist1 = LispMalloc(sizeof(LispObj*) * length1);
	    plist2 = LispMalloc(sizeof(LispObj*) * length2);
	    for (i = 0, coff1 = start1; coff1 < end1;
		 i++, coff1++, list1 = CDR(list1))
		plist1[i] = CAR(list1);
	    for (i = 0, coff2 = start2; coff2 < end2;
		 i++, coff2++, list2 = CDR(list2))
		plist2[i] = CAR(list2);
	}

	while (end2 - start2 > length1) {
	    coff1 = end1 - 1;
	    coff2 = end2 - 1;
	    while (coff1 >= start1) {
		if (istring) {
		    cmp1 = SCHAR(string1[coff1]);
		    cmp2 = SCHAR(string2[coff2]);
		}
		else {
		    cmp1 = plist1[coff1 - start1];
		    cmp2 = plist2[coff2 - start2];
		}
		if (key != NIL) {
		    cmp1 = APPLY1(key, cmp1);
		    cmp2 = APPLY1(key, cmp2);
		}

		/* Compare elements */
		if ((value = FCOMPARE(lambda, cmp1, cmp2, code)) != NIL)
		    value = T;
		if (value != expect)
		    break;

		/* Update offsets */
		--coff1;
		--coff2;
	    }

	    /* If all elements matched */
	    if (coff1 == start1 - 1) {
		offset = coff2 + 1;
		break;
	    }

	    /* Update offset */
	    --end2;
	}

	if (!istring) {
	    LispFree(plist1);
	    LispFree(plist2);
	}
    }

    return (offset == -1 ? NIL : FIXNUM(offset));
}

/*
 * ext::getenv
 */
LispObj *
Lisp_Setenv(LispBuiltin *builtin)
/*
 setenv name value &optional overwrite
 */
{
    char *name, *value;

    LispObj *oname, *ovalue, *overwrite;

    overwrite = ARGUMENT(2);
    ovalue = ARGUMENT(1);
    oname = ARGUMENT(0);

    CHECK_STRING(oname);
    name = THESTR(oname);

    CHECK_STRING(ovalue);
    value = THESTR(ovalue);

    setenv(name, value, overwrite != NIL);
    value = getenv(name);

    return (value ? STRING(value) : NIL);
}

LispObj *
Lisp_Set(LispBuiltin *builtin)
/*
 set symbol value
 */
{
    LispObj *symbol, *value;

    value = ARGUMENT(1);
    symbol = ARGUMENT(0);

    CHECK_SYMBOL(symbol);
    CHECK_CONSTANT(symbol);
    LispSetVar(symbol, value);

    return (value);
}

LispObj *
Lisp_SetDifference(LispBuiltin *builtin)
/*
 set-difference list1 list2 &key test test-not key
 */
{
    return (LispListSet(builtin, SETDIFFERENCE));
}

LispObj *
Lisp_SetExclusiveOr(LispBuiltin *builtin)
/*
 set-exclusive-or list1 list2 &key test test-not key
 */
{
    return (LispListSet(builtin, SETEXCLUSIVEOR));
}

LispObj *
Lisp_SetQ(LispBuiltin *builtin)
/*
 setq &rest form
 */
{
    LispObj *result, *variable, *form;

    form = ARGUMENT(0);

    result = NIL;
    for (; CONSP(form); form = CDR(form)) {
	variable = CAR(form);
	CHECK_SYMBOL(variable);
	CHECK_CONSTANT(variable);
	form = CDR(form);
	if (!CONSP(form))
	    LispDestroy("%s: odd number of arguments", STRFUN(builtin));
	result = EVAL(CAR(form));
	LispSetVar(variable, result);
    }

    return (result);
}

LispObj *
Lisp_Setf(LispBuiltin *builtin)
/*
 setf &rest form
 */
{
    LispAtom *atom;
    LispObj *place, *setf, *result = NIL;

    LispObj *form;

    form = ARGUMENT(0);

    for (; CONSP(form); form = CDR(form)) {
	place = CAR(form);
	form = CDR(form);
	if (!CONSP(form))
	    LispDestroy("%s: odd number of arguments", STRFUN(builtin));
	result = CAR(form);

	/* if a variable, just work like setq */
	if (SYMBOLP(place)) {
	    CHECK_CONSTANT(place);
	    result = EVAL(result);
	    (void)LispSetVar(place, result);
	}

	else if (CONSP(place)) {
	    int struc_access = 0;

	    /* the default setf method for structures is generated here
	     * (cannot be done in EVAL as SETF is a macro), and the
	     * code executed is as if this definition were supplied:
	     *	(defsetf THE-STRUCT-FIELD (struct) (value)
	     *		`(lisp::struct-store 'THE-STRUCT-FIELD ,struct ,value))
	     */

	    setf = CAR(place);
	    if (!SYMBOLP(setf))
		goto invalid_place;

	    atom = setf->data.atom;

	    if (atom->a_defsetf == 0) {
		if (atom->a_defstruct && atom->property->structure.function >= 0) {
		    /* user didn't provide any special defsetf */
		    setf = Ostruct_store;
		    struc_access = 1;
		}
		else
		    goto invalid_place;
	    }
	    else
		setf = setf->data.atom->property->setf;

	    if (SYMBOLP(setf)) {
		/* just change function call, and append value to arguments */
		GC_ENTER();
		LispObj *cod, *cdr, *obj;

		cod = cdr = CONS(setf, NIL);
		GC_PROTECT(cod);

		if (struc_access) {
		    /* using builtin setf method for structure field */
		    RPLACD(cdr, CONS(QUOTE(CAR(place)), NIL));
		    cdr = CDR(cdr);
		}

		for (obj = CDR(place); obj != NIL; obj = CDR(obj)) {
		    RPLACD(cdr, CONS(CAR(obj), NIL));
		    cdr = CDR(cdr);
		}
		RPLACD(cdr, CONS(result, NIL));
		result = EVAL(cod);
		GC_LEAVE();
	    }
	    else
		result = LispRunSetf(atom->property->salist,
				     setf, place, result);
	}
	else
	    goto invalid_place;
    }

    return (result);

invalid_place:
    LispDestroy("%s: %s is an invalid place", STRFUN(builtin), STROBJ(place));
    /*NOTREACHED*/
    return (NIL);
}

LispObj *
Lisp_Sleep(LispBuiltin *builtin)
/*
 sleep seconds
 */
{
    long sec, msec;
    double value, dsec;

    LispObj *seconds;

    seconds = ARGUMENT(0);

    value = -1.0;
    switch (OBJECT_TYPE(seconds)) {
	case LispFixnum_t:
	    value = FIXNUM_VALUE(seconds);
	    break;
	case LispInteger_t:
	    value = INT_VALUE(seconds);
	    break;
	case LispDFloat_t:
	    value = DFLOAT_VALUE(seconds);
	    break;
	default:
	    break;
    }

    if (value < 0.0 || !finite(value) || value > INT_MAX)
	LispDestroy("%s: %s is not a positive fixnum",
		    STRFUN(builtin), STROBJ(seconds));

    msec = modf(value, &dsec) * 1e6;
    sec = dsec;

    if (sec)
	sleep(sec);
    if (msec)
	usleep(msec);

    return (NIL);
}

/*
 *   This function is called recursively, but the contents of "list2" are
 * kept gc protected until it returns to LispSort. This is required partly
 * because the "gc protection logic" protects an object, not the contents
 * of the c pointer.
 */
static LispObj *
LispMergeSort(LispObj *list, LispObj *predicate,
	      LispObj *key, int code)
{
    int protect;
    LispObj *list1, *list2, *left, *right, *result, *cons;

    /* Check if list length is larger than 1 */
    if (!CONSP(list) || !CONSP(CDR(list)))
	return (list);

    list1 = list2 = list;
    for (;;) {
	list = CDR(list);
	if (!CONSP(list))
	    break;
	list = CDR(list);
	if (!CONSP(list))
	    break;
	list2 = CDR(list2);
    }
    cons = list2;
    list2 = CDR(list2);
    RPLACD(cons, NIL);

    protect = 0;
    if (lisp__data.protect.length + 2 >= lisp__data.protect.space)
	LispMoreProtects();
    lisp__data.protect.objects[lisp__data.protect.length++] = list2;
    list1 = LispMergeSort(list1, predicate, key, code);
    list2 = LispMergeSort(list2, predicate, key, code);

    left = CAR(list1);
    right = CAR(list2);
    if (key != NIL) {
	protect = lisp__data.protect.length;
	left = APPLY1(key, left);
	lisp__data.protect.objects[protect] = left;
	right = APPLY1(key, right);
	lisp__data.protect.objects[protect + 1] = right;
    }

    result = NIL;
    for (;;) {
	if ((FCOMPARE(predicate, left, right, code)) == NIL &&
	    (FCOMPARE(predicate, right, left, code)) != NIL) {
	    /* right is "smaller" */
	    if (result == NIL)
		result = list2;
	    else
		RPLACD(cons, list2);
	    cons = list2;
	    list2 = CDR(list2);
	    if (!CONSP(list2)) {
		RPLACD(cons, list1);
		break;
	    }
	    right = CAR(list2);
	    if (key != NIL) {
		right = APPLY1(key, right);
		lisp__data.protect.objects[protect + 1] = right;
	    }
	}
	else {
	    /* left is "smaller" */
	    if (result == NIL)
		result = list1;
	    else
		RPLACD(cons, list1);
	    cons = list1;
	    list1 = CDR(list1);
	    if (!CONSP(list1)) {
		RPLACD(cons, list2);
		break;
	    }
	    left = CAR(list1);
	    if (key != NIL) {
		left = APPLY1(key, left);
		lisp__data.protect.objects[protect] = left;
	    }
	}
    }
    if (key != NIL)
	lisp__data.protect.length = protect;

    return (result);
}

/* XXX The first version made a copy of the list and then adjusted
 *     the CARs of the list. To minimize GC time now it is now doing
 *     the sort inplace. So, instead of writing just (sort variable)
 *     now it is required to write (setq variable (sort variable))
 *     if the variable should always keep all elements.
 */
LispObj *
Lisp_Sort(LispBuiltin *builtin)
/*
 sort sequence predicate &key key
 */
{
    GC_ENTER();
    int istring, code;
    long length;
    char *string;

    LispObj *list, *work, *cons = NULL;

    LispObj *sequence, *predicate, *key;

    key = ARGUMENT(2);
    predicate = ARGUMENT(1);
    sequence = ARGUMENT(0);

    length = LispLength(sequence);
    if (length < 2)
	return (sequence);

    list = sequence;
    istring = XSTRINGP(sequence);
    if (istring) {
	/* Convert string to list */
	string = THESTR(sequence);
	work = cons = CONS(SCHAR(string[0]), NIL);
	GC_PROTECT(work);
	for (++string; *string; ++string) {
	    RPLACD(cons, CONS(SCHAR(*string), NIL));
	    cons = CDR(cons);
	}
    }
    else if (ARRAYP(list))
	work = list->data.array.list;
    else
	work = list;

    code = FCODE(predicate);
    work = LispMergeSort(work, predicate, key, code);

    if (istring) {
	/* Convert list to string */
	string = THESTR(sequence);
	for (; CONSP(work); ++string, work = CDR(work))
	    *string = SCHAR_VALUE(CAR(work));
    }
    else if (ARRAYP(list))
	list->data.array.list = work;
    else
	sequence = work;
    GC_LEAVE();

    return (sequence);
}

LispObj *
Lisp_Subseq(LispBuiltin *builtin)
/*
 subseq sequence start &optional end
 */
{
    long start, end, length, seqlength;

    LispObj *sequence, *ostart, *oend, *result;

    oend = ARGUMENT(2);
    ostart = ARGUMENT(1);
    sequence = ARGUMENT(0);

    LispCheckSequenceStartEnd(builtin, sequence, ostart, oend,
			      &start, &end, &length);

    seqlength = end - start;

    if (sequence == NIL)
	result = NIL;
    else if (XSTRINGP(sequence)) {
	char *string = LispMalloc(seqlength + 1);

	memcpy(string, THESTR(sequence) + start, seqlength);
	string[seqlength] = '\0';
	result = STRING2(string);
    }
    else {
	GC_ENTER();
	LispObj *object;

	if (end > start) {
	    /* list or array */
	    int count;
	    LispObj *cons;

	    if (ARRAYP(sequence))
		object = sequence->data.array.list;
	    else
		object = sequence;
	    /* goto first element to copy */
	    for (count = 0; count < start; count++, object = CDR(object))
		;
	    result = cons = CONS(CAR(object), NIL);
	    GC_PROTECT(result);
	    for (++count, object = CDR(object); count < end; count++,
		 object = CDR(object)) {
		RPLACD(cons, CONS(CAR(object), NIL));
		cons = CDR(cons);
	    }
	}
	else
	    result = NIL;

	if (ARRAYP(sequence)) {
	    object = LispNew(NIL, NIL);
	    GC_PROTECT(object);
	    object->type = LispArray_t;
	    object->data.array.list = result;
	    object->data.array.dim = CONS(FIXNUM(seqlength), NIL);
	    object->data.array.rank = 1;
	    object->data.array.type = sequence->data.array.type;
	    object->data.array.zero = length == 0;
	    result = object;
	}
	GC_LEAVE();
    }

    return (result);
}

LispObj *
Lisp_Subsetp(LispBuiltin *builtin)
/*
 subsetp list1 list2 &key test test-not key
 */
{
    return (LispListSet(builtin, SUBSETP));
}


LispObj *
Lisp_Substitute(LispBuiltin *builtin)
/*
 substitute newitem olditem sequence &key from-end test test-not start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, SUBSTITUTE, NONE));
}

LispObj *
Lisp_SubstituteIf(LispBuiltin *builtin)
/*
 substitute-if newitem test sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, SUBSTITUTE, IF));
}

LispObj *
Lisp_SubstituteIfNot(LispBuiltin *builtin)
/*
 substitute-if-not newitem test sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(builtin, SUBSTITUTE, IFNOT));
}

LispObj *
Lisp_Symbolp(LispBuiltin *builtin)
/*
 symbolp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    switch (OBJECT_TYPE(object)) {
	case LispNil_t:
	case LispAtom_t:
	case LispLambda_t:
	    return (T);
	default:
	    break;
    }
    return (NIL);
}

LispObj *
Lisp_SymbolPlist(LispBuiltin *builtin)
/*
 symbol-plist symbol
 */
{
    LispObj *symbol;

    symbol = ARGUMENT(0);

    if (!SYMBOLP(symbol)) {
	if (symbol == NIL)
	    symbol = ATOM(Snil);
	else if (symbol == T)
	    symbol = ATOM(St);
	else
	    LispDestroy("%s: %s is not a symbol",
			STRFUN(builtin), STROBJ(symbol));
    }

    return (symbol->data.atom->a_property ?
	    symbol->data.atom->property->properties : NIL);
}

LispObj *
Lisp_SymbolValue(LispBuiltin *builtin)
/*
 symbol-value symbol
 */
{
    LispAtom *atom;
    LispObj *symbol;

    symbol = ARGUMENT(0);

    if (!SYMBOLP(symbol)) {
	if (symbol == NIL || symbol == T)
	    return (symbol);
	LispDestroy("%s: %s is not a symbol",
		    STRFUN(builtin), STROBJ(symbol));
    }
    atom = symbol->data.atom;
    if (!atom->a_object)
	LispDestroy("%s: the symbol %s has no value",
		    STRFUN(builtin), STROBJ(symbol));

    return (atom->dyn ? LispGetVar(symbol) : atom->property->value);
}

LispObj *
Lisp_Tagbody(LispBuiltin *builtin)
/*
 tagbody &rest body
 */
{
    GC_ENTER();
    int stack, lex, length;
    LispObj *list, *body, *ptr, *tag, *labels, *map,
	    **p_list, **p_body, **p_labels;
    LispBlock *block;

    body = ARGUMENT(0);

    /* Save environment information */
    stack = lisp__data.stack.length;
    lex = lisp__data.env.lex;
    length = lisp__data.env.length;

    /* Since the body may be large, and the code may iterate several
     * thousand times, it is not a bad idea to avoid checking all
     * elements of the body to verify if it is a tag. */
    for (labels = map = NIL, ptr = body; CONSP(ptr); ptr = CDR(ptr)) {
	tag = CAR(ptr);
	switch (OBJECT_TYPE(tag)) {
	    case LispNil_t:
	    case LispAtom_t:
	    case LispFixnum_t:
		/* Don't allow duplicated labels */
		for (list = labels; CONSP(list); list = CDDR(list)) {
		    if (CAR(list) == tag)
			LispDestroy("%s: tag %s specified more than once",
				    STRFUN(builtin), STROBJ(tag));
		}
		if (labels == NIL) {
		    labels = CONS(tag, CONS(NIL, NIL));
		    map = CDR(labels);
		    GC_PROTECT(labels);
		}
		else {
		    RPLACD(map, CONS(tag, CONS(NIL, NIL)));
		    map = CDDR(map);
		}
		break;
	    case LispCons_t:
		/* Restart point for tag */
		if (map != NIL && CAR(map) == NIL)
		    RPLACA(map, ptr);
		break;
	    default:
		break;
	}
    }
    /* Check for consecutive labels without code between them */
    for (ptr = labels; CONSP(ptr); ptr = CDDR(ptr)) {
	if (CADR(ptr) == NIL) {
	    for (map = CDDR(ptr); CONSP(map); map = CDDR(map)) {
		if (CADR(map) != NIL) {
		    RPLACA(CDR(ptr), CADR(map));
		    break;
		}
	    }
	}
    }

    /* Initialize */
    list = body;
    p_list = &list;
    p_body = &body;
    p_labels = &labels;
    block = LispBeginBlock(NIL, LispBlockBody);

    /* Loop */
    if (setjmp(block->jmp) != 0) {
	/* Restore environment */
	lisp__data.stack.length = stack;
	lisp__data.env.lex = lex;
	lisp__data.env.head = lisp__data.env.length = length;

	tag = lisp__data.block.block_ret;
	for (ptr = labels; CONSP(ptr); ptr = CDDR(ptr)) {
	    map = CAR(ptr);
	    if (map == tag)
		break;
	}

	if (!CONSP(ptr))
	    LispDestroy("%s: no such tag %s", STRFUN(builtin), STROBJ(tag));

	*p_body = CADR(ptr);
    }

    /* Execute code */
    for (; CONSP(body); body = CDR(body)) {
	LispObj *form = CAR(body);

	if (CONSP(form))
	    EVAL(form);
    }
    /* If got here, (go) not called, else, labels will be candidate to gc
     * when GC_LEAVE() be called by the code in the bottom of the stack. */
    GC_LEAVE();

    /* Finished */
    LispEndBlock(block);

    /* Always return NIL */
    return (NIL);
}

LispObj *
Lisp_Terpri(LispBuiltin *builtin)
/*
 terpri &optional output-stream
 */
{
    LispObj *output_stream;

    output_stream = ARGUMENT(0);

    if (output_stream != NIL) {
	CHECK_STREAM(output_stream);
    }
    LispWriteChar(output_stream, '\n');
    if (output_stream == NIL ||
	(output_stream->data.stream.type == LispStreamStandard &&
	 output_stream->data.stream.source.file == Stdout))
	LispFflush(Stdout);

    return (NIL);
}

LispObj *
Lisp_The(LispBuiltin *builtin)
/*
 the value-type form
 */
{
    LispObj *value_type, *form;

    form = ARGUMENT(1);
    value_type = ARGUMENT(0);

    form = EVAL(form);

    return (LispCoerce(builtin, form, value_type));
}

LispObj *
Lisp_Throw(LispBuiltin *builtin)
/*
 throw tag result
 */
{
    unsigned blevel = lisp__data.block.block_level;

    LispObj *tag, *result;

    result = ARGUMENT(1);
    tag = ARGUMENT(0);

    tag = EVAL(tag);

    if (blevel == 0)
	LispDestroy("%s: not within a block", STRFUN(builtin));

    while (blevel) {
	LispBlock *block = lisp__data.block.block[--blevel];

	if (block->type == LispBlockCatch && XEQ(tag, block->tag) != NIL) {
	    RETURN_COUNT = 0;
	    lisp__data.block.block_ret = EVAL(result);
	    LispBlockUnwind(block);
	    BLOCKJUMP(block);
	}
    }
    LispDestroy("%s: %s is not a valid tag", STRFUN(builtin), STROBJ(tag));

    /*NOTREACHED*/
    return (NIL);
}

LispObj *
Lisp_Typep(LispBuiltin *builtin)
/*
 typep object type
 */
{
    LispObj *result = NULL;

    LispObj *object, *type;

    type = ARGUMENT(1);
    object = ARGUMENT(0);

    if (SYMBOLP(type)) {
	Atom_id atom = ATOMID(type);

	if (OBJECT_TYPE(object) == LispStruct_t)
	    result = ATOMID(CAR(object->data.struc.def)) == atom ? T : NIL;
	else if (type->data.atom->a_defstruct &&
		 type->data.atom->property->structure.function == STRUCT_NAME)
	    result = NIL;
	else if (atom == Snil)
	    result = object == NIL ? T : NIL;
	else if (atom == St)
	    result = object == T ? T : NIL;
	else if (atom == Satom)
	    result = !CONSP(object) ? T : NIL;
	else if (atom == Ssymbol)
	    result = SYMBOLP(object) || object == NIL || object == T ? T : NIL;
	else if (atom == Sinteger)
	    result = INTEGERP(object) ? T : NIL;
	else if (atom == Srational)
	    result = RATIONALP(object) ? T : NIL;
	else if (atom == Scons || atom == Slist)
	    result = CONSP(object) ? T : NIL;
	else if (atom == Sstring)
	    result = STRINGP(object) ? T : NIL;
	else if (atom == Scharacter)
	    result = SCHARP(object) ? T : NIL;
	else if (atom == Scomplex)
	    result = COMPLEXP(object) ? T : NIL;
	else if (atom == Svector || atom == Sarray)
	    result = ARRAYP(object) ? T : NIL;
	else if (atom == Skeyword)
	    result = KEYWORDP(object) ? T : NIL;
	else if (atom == Sfunction)
	    result = LAMBDAP(object) ? T : NIL;
	else if (atom == Spathname)
	    result = PATHNAMEP(object) ? T : NIL;
	else if (atom == Sopaque)
	    result = OPAQUEP(object) ? T : NIL;
    }
    else if (CONSP(type)) {
	if (OBJECT_TYPE(object) == LispStruct_t &&
	    SYMBOLP(CAR(type)) && ATOMID(CAR(type)) == Sstruct &&
	    SYMBOLP(CAR(CDR(type))) && CDR(CDR(type)) == NIL) {
	    result = ATOMID(CAR(object->data.struc.def)) ==
		     ATOMID(CAR(CDR(type))) ? T : NIL;
	}
    }
    else if (type == NIL)
	result = object == NIL ? T : NIL;
    else if (type == T)
	result = object == T ? T : NIL;
    if (result == NULL)
	LispDestroy("%s: bad type specification %s",
		    STRFUN(builtin), STROBJ(type));

    return (result);
}

LispObj *
Lisp_Union(LispBuiltin *builtin)
/*
 union list1 list2 &key test test-not key
 */
{
    return (LispListSet(builtin, UNION));
}

LispObj *
Lisp_Unless(LispBuiltin *builtin)
/*
 unless test &rest body
 */
{
    LispObj *result, *test, *body;

    body = ARGUMENT(1);
    test = ARGUMENT(0);

    result = NIL;
    test = EVAL(test);
    if (test == NIL) {
	for (; CONSP(body); body = CDR(body))
	    result = EVAL(CAR(body));
    }

    return (result);
}

/*
 * ext::until
 */
LispObj *
Lisp_Until(LispBuiltin *builtin)
/*
 until test &rest body
 */
{
    LispObj *result, *test, *body, *prog;

    body = ARGUMENT(1);
    test = ARGUMENT(0);

    result = NIL;
    for (;;) {
	if ((result = EVAL(test)) == NIL) {
	    for (prog = body; CONSP(prog); prog = CDR(prog))
		(void)EVAL(CAR(prog));
	}
	else
	    break;
    }

    return (result);
}

LispObj *
Lisp_UnwindProtect(LispBuiltin *builtin)
/*
 unwind-protect protect &rest cleanup
 */
{
    LispObj *result, **presult = &result;
    int did_jump, *pdid_jump = &did_jump;
    LispBlock *block;

    LispObj *protect, *cleanup, **pcleanup = &cleanup;

    cleanup = ARGUMENT(1);
    protect = ARGUMENT(0);

    /* run protected code */
    *presult = NIL;
    *pdid_jump = 1;
    block = LispBeginBlock(NIL, LispBlockProtect);
    if (setjmp(block->jmp) == 0) {
	*presult = EVAL(protect);
	*pdid_jump = 0;
    }
    LispEndBlock(block);
    if (!lisp__data.destroyed && *pdid_jump)
	*presult = lisp__data.block.block_ret;

    /* run cleanup, unprotected code */
    if (CONSP(*pcleanup))
	for (; CONSP(cleanup); cleanup = CDR(cleanup))
	    (void)EVAL(CAR(cleanup));
    else if (lisp__data.destroyed)
	/* no cleanup code */
	LispDestroy(".");

    return (result);
}

LispObj *
Lisp_Values(LispBuiltin *builtin)
/*
 values &rest objects
 */
{
    long i, count;
    LispObj *result;

    LispObj *objects;

    objects = ARGUMENT(0);

    count = LispLength(objects) - 1;

    if (count >= 0) {
	result = CAR(objects);
	RETURN_COUNT = count;
	count = RETURN_CHECK(count);
	for (i = 0, objects = CDR(objects); count && CONSP(objects);
	     count--, i++, objects = CDR(objects))
	    RETURN(i) = CAR(objects);
    }
    else {
	RETURN_COUNT = -1;
	result = NIL;
    }

    return (result);
}

LispObj *
Lisp_Vector(LispBuiltin *builtin)
/*
 vector &rest objects
 */
{
    LispObj *objects;

    objects = ARGUMENT(0);

    return (VECTOR(objects));
}

LispObj *
Lisp_When(LispBuiltin *builtin)
/*
 when test &rest body
 */
{
    LispObj *result, *test, *body;

    body = ARGUMENT(1);
    test = ARGUMENT(0);

    result = NIL;
    test = EVAL(test);
    if (test != NIL) {
	for (; CONSP(body); body = CDR(body))
	    result = EVAL(CAR(body));
    }

    return (result);
}

/*
 * ext::while
 */
LispObj *
Lisp_While(LispBuiltin *builtin)
/*
 while test &rest body
 */
{
    LispObj *result, *test, *body, *prog;

    body = ARGUMENT(1);
    test = ARGUMENT(0);

    result = NIL;
    for (;;) {
	if (EVAL(test) != NIL) {
	    for (prog = body; CONSP(prog); prog = CDR(prog))
		(void)EVAL(CAR(prog));
	}
	else
	    break;
    }

    return (NIL);
}

/*
 * ext::unsetenv
 */
LispObj *
Lisp_Unsetenv(LispBuiltin *builtin)
/*
 unsetenv name
 */
{
    char *name;

    LispObj *oname;

    oname = ARGUMENT(0);

    CHECK_STRING(oname);
    name = THESTR(oname);

    unsetenv(name);

    return (NIL);
}

LispObj *
Lisp_XeditEltStore(LispBuiltin *builtin)
/*
 lisp::elt-store sequence index value
 */
{
    int length, offset;

    LispObj *sequence, *oindex, *value;

    value = ARGUMENT(2);
    oindex = ARGUMENT(1);
    sequence = ARGUMENT(0);

    CHECK_INDEX(oindex);
    offset = FIXNUM_VALUE(oindex);
    length = LispLength(sequence);

    if (offset >= length)
	LispDestroy("%s: index %d too large for sequence length %d",
		    STRFUN(builtin), offset, length);

    if (STRINGP(sequence)) {
	int ch;

	CHECK_SCHAR(value);
	ch = SCHAR_VALUE(value);
	if (ch < 0 || ch > 255)
	    LispDestroy("%s: cannot represent character %d",
			STRFUN(builtin), ch);
	THESTR(sequence)[offset] = ch;
    }
    else {
	if (ARRAYP(sequence))
	    sequence = sequence->data.array.list;

	for (; offset > 0; offset--, sequence = CDR(sequence))
	    ;
	RPLACA(sequence, value);
    }

    return (value);
}

LispObj *
Lisp_XeditPut(LispBuiltin *builtin)
/*
 lisp::put symbol indicator value
 */
{
    LispObj *symbol, *indicator, *value;

    value = ARGUMENT(2);
    indicator = ARGUMENT(1);
    symbol = ARGUMENT(0);

    if (!SYMBOLP(symbol)) {
	if (symbol == NIL)
	    symbol = ATOM(Snil);
	else if (symbol == T)
	    symbol = ATOM(St);
	else
	    LispDestroy("%s: %s is not a symbol",
			STRFUN(builtin), STROBJ(symbol));
    }

    return (CAR(LispPutAtomProperty(symbol->data.atom, indicator, value)));
}

LispObj *
Lisp_XeditVectorStore(LispBuiltin *builtin)
/*
 lisp::vector-store array &rest values
 */
{
    LispObj *value, *list, *object;
    long rank, count, sequence, offset, accum;

    LispObj *array, *values;

    values = ARGUMENT(1);
    array = ARGUMENT(0);

    /* check for errors */
    for (rank = 0, list = values;
	 CONSP(list) && CONSP(CDR(list));
	 list = CDR(list), rank++) {
	CHECK_INDEX(CAR(values));
    }

    if (rank == 0)
	LispDestroy("%s: too few subscripts", STRFUN(builtin));
    value = CAR(list);

    if (STRINGP(array) && rank == 1) {
	long ch;
	long length = STRLEN(array);
	long offset = FIXNUM_VALUE(CAR(values));

	CHECK_SCHAR(value);
	ch = SCHAR_VALUE(value);
	if (offset >= length)
	    LispDestroy("%s: index %ld too large for sequence length %ld",
			STRFUN(builtin), offset, length);

	if (ch < 0 || ch > 255)
	    LispDestroy("%s: cannot represent character %ld",
			STRFUN(builtin), ch);
	THESTR(array)[offset] = ch;

	return (value);
    }

    CHECK_ARRAY(array);
    if (rank != array->data.array.rank)
	LispDestroy("%s: too %s subscripts", STRFUN(builtin),
		    rank < array->data.array.rank ? "few" : "many");

    for (list = values, object = array->data.array.dim;
	 CONSP(CDR(list));
	 list = CDR(list), object = CDR(object)) {
	if (FIXNUM_VALUE(CAR(list)) >= FIXNUM_VALUE(CAR(object)))
	    LispDestroy("%s: %ld is out of range, index %ld",
			STRFUN(builtin),
			FIXNUM_VALUE(CAR(list)),
			FIXNUM_VALUE(CAR(object)));
    }

    for (count = sequence = 0, list = values;
	 CONSP(CDR(list));
	 list = CDR(list), sequence++) {
	for (offset = 0, object = array->data.array.dim;
	     offset < sequence; object = CDR(object), offset++)
	    ;
	for (accum = 1, object = CDR(object); CONSP(object);
	     object = CDR(object))
	    accum *= FIXNUM_VALUE(CAR(object));
	count += accum * FIXNUM_VALUE(CAR(list));
    }

    for (array = array->data.array.list; count > 0; array = CDR(array), count--)
	;

    RPLACA(array, value);

    return (value);
}

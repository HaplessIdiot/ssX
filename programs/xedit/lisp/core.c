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

/* $XFree86: xc/programs/xedit/lisp/core.c,v 1.49 2002/09/11 19:54:51 tsi Exp $ */

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
	code ? LispObjectCompare(mac, left, right, code) :	\
	       APPLY2(predicate, left, right)

LispObj *LispAdjoin(LispMac*, LispBuiltin*,
		    LispObj*, LispObj*, LispObj*, LispObj*, LispObj*);
LispObj *LispAssocOrMember(LispMac*, LispBuiltin*, int, int);
LispObj *LispPosition(LispMac*, LispBuiltin*, int);
LispObj *LispDeleteOrRemoveDuplicates(LispMac*, LispBuiltin*, int);
LispObj *LispDeleteRemoveXSubstitute(LispMac*, LispBuiltin*, int, int);
LispObj *LispListSet(LispMac*, LispBuiltin*, int);
extern LispObj *LispRunSetf(LispMac*, LispArgList*, LispObj*, LispObj*, LispObj*);
static LispObj *LispMergeSort(LispMac*, LispObj*, LispObj*, LispObj*, int);
LispObj *LispXReverse(LispMac*, LispBuiltin*, int);

/*
 * Initialization
 */
LispObj *Oeq, *Oeql, *Oequal, *Oequalp, *Omake_array,
	*Kinitial_contents, *Osetf, *Ootherwise;

Atom_id Svariable, Sstructure, Stype, Ssetf;

/*
 * Implementation
 */
void
LispCoreInit(LispMac *mac)
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

    Ssetf	= ATOMID(Osetf);
}

LispObj *
Lisp_Acons(LispMac *mac, LispBuiltin *builtin)
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
LispAdjoin(LispMac *mac, LispBuiltin*builtin, LispObj *item, LispObj *list,
	   LispObj *key, LispObj *test, LispObj *test_not)
{
    GC_ENTER();
    int code;
    LispObj *lambda, *compare, *expect, *value, *object;

    if (list != NIL) {
	ERROR_CHECK_LIST(list);
    }
    if (test != NIL && test_not != NIL)
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT",
		    STRFUN(builtin));

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
    for (object = list; CONS_P(object); object = CDR(object)) {
	compare = CAR(object);
	if (key != NIL) {
	    compare = APPLY1(key, compare);
	    GC_PROTECT(compare);
	    value = FCOMPARE(lambda, item, compare, code);
	    /* Unprotect compare... */
	    --mac->protect.length;
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
Lisp_Adjoin(LispMac *mac, LispBuiltin *builtin)
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

    return (LispAdjoin(mac, builtin, item, list, key, test, test_not));
}

LispObj *
Lisp_Append(LispMac *mac, LispBuiltin *builtin)
/*
 append &rest lists
 */
{
    GC_ENTER();

    LispObj *result, *cons, *list, *lists;

    lists = ARGUMENT(0);

    result = cons = NIL;
    for (; CONS_P(lists); lists = CDR(lists)) {
	list = CAR(lists);
	if (list == NIL)
	    continue;
	ERROR_CHECK_LIST(list);
	if (result == NIL) {
	    result = cons = CONS(CAR(list), CDR(list));
	    GC_PROTECT(result);
	}
	else {
	    if (CONS_P(CDR(cons))) {
		LispObj *obj = CDR(cons);

		while (CONS_P(CDR(obj))) {
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
Lisp_Aref(LispMac *mac, LispBuiltin *builtin)
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
    if (STRING_P(array) && INDEX_P(CAR(subscripts)) && CDR(subscripts) == NIL) {
	long length = STRLEN(array);
	long offset = CAR(subscripts)->data.integer;

	if (offset >= length)
	    LispDestroy(mac, "%s: index %ld too large for sequence length %ld",
			STRFUN(builtin), offset, length);

	return (CHAR(*(unsigned char*)(THESTR(array) + offset)));
    }

    if (array->type != LispArray_t)
	LispDestroy(mac, "%s: %s is not an array",
		    STRFUN(builtin), STROBJ(array));

    for (count = 0, dim = subscripts, obj = array->data.array.dim; CONS_P(dim);
	 count++, dim = CDR(dim), obj = CDR(obj)) {
	if (count >= array->data.array.rank)
	    LispDestroy(mac, "%s: too many subscripts %s",
			STRFUN(builtin), STROBJ(subscripts));
	if (!INDEX_P(CAR(dim)) ||
	    CAR(dim)->data.integer >= CAR(obj)->data.integer)
	    LispDestroy(mac, "%s: %s is out of range or a bad index",
			STRFUN(builtin), STROBJ(CAR(dim)));
    }
    if (count < array->data.array.rank)
	LispDestroy(mac, "%s: too few subscripts %s",
		    STRFUN(builtin), STROBJ(subscripts));

    for (count = seq = 0, dim = subscripts; CONS_P(dim); dim = CDR(dim), seq++) {
	for (idx = 0, obj = array->data.array.dim; idx < seq;
	     obj = CDR(obj), ++idx)
	    ;
	for (c = 1, obj = CDR(obj); obj != NIL; obj = CDR(obj))
	    c *= CAR(obj)->data.integer;
	count += c * CAR(dim)->data.integer;
    }

    for (array = array->data.array.list; count > 0; array = CDR(array), count--)
	;

    return (CAR(array));
}

LispObj *
LispAssocOrMember(LispMac *mac, LispBuiltin *builtin,
		  int function, int comparison)
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
    else
	ERROR_CHECK_LIST(list);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT",
		    STRFUN(builtin));

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
    for (; CONS_P(list); list = CDR(list)) {
	compare = CAR(list);
	if (function == ASSOC) {
	    if (!CONS_P(compare))
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
Lisp_Assoc(LispMac *mac, LispBuiltin *builtin)
/*
 assoc item list &key test test-not key
 */
{
    return (LispAssocOrMember(mac, builtin, ASSOC, NONE));
}

LispObj *
Lisp_AssocIf(LispMac *mac, LispBuiltin *builtin)
/*
 assoc-if predicate list &key key
 */
{
    return (LispAssocOrMember(mac, builtin, ASSOC, IF));
}

LispObj *
Lisp_AssocIfNot(LispMac *mac, LispBuiltin *builtin)
/*
 assoc-if-not predicate list &key key
 */
{
    return (LispAssocOrMember(mac, builtin, ASSOC, IFNOT));
}

LispObj *
Lisp_And(LispMac *mac, LispBuiltin *builtin)
/*
 and &rest args
 */
{
    LispObj *result = T, *args;

    args = ARGUMENT(0);

    for (; CONS_P(args); args = CDR(args)) {
	if (NCONSTANT_P(result = CAR(args)))
	    result = EVAL(result);
	if (result == NIL)
	    break;
    }

    return (result);
}

LispObj *
Lisp_Apply(LispMac *mac, LispBuiltin *builtin)
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
 	ERROR_CHECK_LIST(more_args);
    }
    else if (arg != NIL) {
	ERROR_CHECK_LIST(arg);
    }

    if (more_args == NIL)
	result = arg;
    else {
	LispObj *cons;

	result = cons = CONS(arg, NIL);
	GC_PROTECT(result);

	for (;; more_args = CDR(more_args)) {
	    if (CONS_P(CDR(more_args))) {
		RPLACD(cons, CONS(CAR(more_args), NIL));
		cons = CDR(cons);
	    }
	    else {
		arg = RPLACD(cons, CAR(more_args));
		if (CONS_P(arg))
		    for (; CONS_P(arg); arg = CDR(arg))
			;
		if (arg != NIL)
		    LispDestroy(mac, "%s: argument to %s is a dotted list",
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
Lisp_Atom(LispMac *mac, LispBuiltin *builtin)
/*
 atom object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (CONS_P(object) ? NIL : T);
}

LispObj *
Lisp_Block(LispMac *mac, LispBuiltin *builtin)
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

    if (name != NIL && name != T && !SYMBOL_P(name))
	LispDestroy(mac, "%s: %s cannot name a block",
		    STRFUN(builtin), STROBJ(name));

    pbody = &body;
    *pres = NIL;
    *pdid_jump = 1;
    block = LispBeginBlock(mac, name, LispBlockTag);
    if (setjmp(block->jmp) == 0) {
	for (; CONS_P(body); body = CDR(body)) {
	    if (NCONSTANT_P(res = CAR(body)))
		res = EVAL(res);
	}
	*pdid_jump = 0;
    }
    LispEndBlock(mac, block);
    if (*pdid_jump)
	*pres = mac->block.block_ret;

    return (res);
}

LispObj *
Lisp_Boundp(LispMac *mac, LispBuiltin *builtin)
/*
 boundp symbol
 */
{
    LispAtom *atom;

    LispObj *symbol = ARGUMENT(0);

    ERROR_CHECK_SYMBOL(symbol);

    atom = symbol->data.atom;
    if (atom->package == mac->keyword ||
	(atom->a_object && atom->property->value != UNBOUND))
	return (T);

    return (NIL);
}

LispObj *
Lisp_Butlast(LispMac *mac, LispBuiltin *builtin)
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
    ERROR_CHECK_LIST(list);
    if (ocount == NIL)
	count = 1;
    else {
	ERROR_CHECK_INDEX(ocount);
	count = ocount->data.integer;
    }
    length = LispLength(mac, list);

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
Lisp_Car(LispMac *mac, LispBuiltin *builtin)
/*
 car list
 */
{
    LispObj *list, *result = NULL;

    list = ARGUMENT(0);

    if (list == NIL)	result = NIL;
    else ERROR_CHECK_LIST(list);
    else		result = CAR(list);

    return (result);
}

LispObj *
Lisp_Case(LispMac *mac, LispBuiltin *builtin)
/*
 case keyform &rest body
 */
{
    LispObj *result, *code, *keyform, *body, *form;

    body = ARGUMENT(1);
    keyform = ARGUMENT(0);

    result = NIL;
    keyform = EVAL(keyform);

    for (; CONS_P(body); body = CDR(body)) {
	code = CAR(body);
	ERROR_CHECK_LIST(code);

	form = CAR(code);
	if (form == T || form == Ootherwise) {
	    if (CONS_P(CDR(body)))
		LispDestroy(mac, "%s: %s must be the last clause",
			    STRFUN(builtin), STROBJ(CAR(code)));
	    result = CDR(code);
	    break;
	}
	else if (CONS_P(form)) {
	    for (; CONS_P(form); form = CDR(form))
		if (XEQL(keyform, CAR(form)) == T) {
		    result = CDR(code);
		    break;
		}
	    if (CONS_P(form))	/* if found match */
		break;
	}
	else if (XEQL(keyform, form) == T) {
	    result = CDR(code);
	    break;
	}
    }

    for (body = result; CONS_P(body); body = CDR(body))
	result = EVAL(CAR(body));

    return (result);
}

LispObj *
Lisp_Catch(LispMac *mac, LispBuiltin *builtin)
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
    block = LispBeginBlock(mac, tag, LispBlockCatch);
    if (setjmp(block->jmp) == 0) {
	for (; CONS_P(body); body = CDR(body))
	    if (NCONSTANT_P(res = CAR(body)))
		res = EVAL(res);
	*pdid_jump = 0;
    }
    LispEndBlock(mac, block);
    if (*pdid_jump)
	*pres = mac->block.block_ret;

    return (res);
}

LispObj *
Lisp_Coerce(LispMac *mac, LispBuiltin *builtin)
/*
 coerce object result-type
 */
{
    LispObj *object, *result_type;

    result_type = ARGUMENT(1);
    object = ARGUMENT(0);

    return (LispCoerce(mac, builtin, object, result_type));
}

LispObj *
Lisp_Cdr(LispMac *mac, LispBuiltin *builtin)
/*
 cdr list
 */
{
    LispObj *list, *result = NULL;

    list = ARGUMENT(0);

    if (list == NIL)	result = NIL;
    else ERROR_CHECK_LIST(list);
    else		result = CDR(list);

    return (result);
}

LispObj *
Lisp_C_r(LispMac *mac, LispBuiltin *builtin)
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
	ERROR_CHECK_LIST(result);
	result = *desc == 'A' ? CAR(result) : CDR(result);
	--desc;
    }

    return (result);
}

LispObj *
Lisp_Cond(LispMac *mac, LispBuiltin *builtin)
/*
 cond &rest body
 */
{
    LispObj *result, *code, *body;

    body = ARGUMENT(0);

    result = NIL;
    for (; CONS_P(body); body = CDR(body)) {
	code = CAR(body);

	ERROR_CHECK_LIST(code);
	if (NCONSTANT_P(result = CAR(code)))
	    result = EVAL(result);
	if (result == NIL)
	    continue;
	for (code = CDR(code); CONS_P(code); code = CDR(code))
	    if (NCONSTANT_P(result = CAR(code)))
		result = EVAL(result);
	break;
    }

    return (result);
}

LispObj *
Lisp_CopyList(LispMac *mac, LispBuiltin *builtin)
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
    for (list = CDR(list); CONS_P(list); list = CDR(list)) {
	RPLACD(cons, CONS(CAR(list), CDR(list)));
	cons = CDR(cons);
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Cons(LispMac *mac, LispBuiltin *builtin)
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
Lisp_Consp(LispMac *mac, LispBuiltin *builtin)
/*
 consp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (CONS_P(object) ? T : NIL);
}

LispObj *
Lisp_Defconstant(LispMac *mac, LispBuiltin *builtin)
/*
 defconstant name initial-value &optional documentation
 */
{
    LispObj *name, *initial_value, *documentation;

    documentation = ARGUMENT(2);
    initial_value = ARGUMENT(1);
    name = ARGUMENT(0);

    ERROR_CHECK_SYMBOL(name);
    if (documentation != NIL) {
	ERROR_CHECK_STRING(documentation);
    }
    LispDefconstant(mac, name, EVAL(initial_value), documentation);

    return (name);
}

LispObj *
Lisp_Defmacro(LispMac *mac, LispBuiltin *builtin)
/*
 defmacro name lambda-list &rest body
 */
{
    LispArgList *alist;

    LispObj *lambda, *name, *lambda_list, *body;

    body = ARGUMENT(2);
    lambda_list = ARGUMENT(1);
    name = ARGUMENT(0);

    if (!SYMBOL_P(name))
	LispDestroy(mac, "%s: bad MACRO name %s",
		    STRFUN(builtin), STROBJ(name));

    alist = LispCheckArguments(mac, LispMacro, lambda_list, STRPTR(name));

    if (CONS_P(body) && STRING_P(CAR(body))) {
	LispAddDocumentation(mac, name, CAR(body), LispDocFunction);
	body = CDR(body);
    }

    lambda_list = LispListProtectedArguments(mac, alist);
    lambda = LispNewLambda(mac, name, body, lambda_list, LispMacro);

    if (name->data.atom->a_builtin) {
	ERROR_CHECK_SPECIAL_FORM(name->data.atom);
	/* only warn when redefining builtin functions */
	LispWarning(mac, "%s: %s is being redefined",
		    STRFUN(builtin), STRPTR(name));

	LispRemAtomBuiltinProperty(mac, name->data.atom);
    }

    LispSetAtomFunctionProperty(mac, name->data.atom, lambda, alist);
    LispUseArgList(mac, alist);

    return (name);
}

LispObj *
Lisp_Defun(LispMac *mac, LispBuiltin *builtin)
/*
 defun name lambda-list &rest body
 */
{
    LispArgList *alist;

    LispObj *lambda, *name, *lambda_list, *body;

    body = ARGUMENT(2);
    lambda_list = ARGUMENT(1);
    name = ARGUMENT(0);

    if (!SYMBOL_P(name))
	LispDestroy(mac, "%s: bad FUNCTION name %s",
		    STRFUN(builtin), STROBJ(name));

    alist = LispCheckArguments(mac, LispFunction, lambda_list, STRPTR(name));

    if (CONS_P(body) && STRING_P(CAR(body))) {
	LispAddDocumentation(mac, name, CAR(body), LispDocFunction);
	body = CDR(body);
    }

    lambda_list = LispListProtectedArguments(mac, alist);
    lambda = LispNewLambda(mac, name, body, lambda_list, LispFunction);

    if (name->data.atom->a_builtin) {
	ERROR_CHECK_SPECIAL_FORM(name->data.atom);
	/* only warn when redefining builtin functions */
	LispWarning(mac, "%s: %s is being redefined",
		    STRFUN(builtin), STRPTR(name));

	LispRemAtomBuiltinProperty(mac, name->data.atom);
    }
    LispSetAtomFunctionProperty(mac, name->data.atom, lambda, alist);
    LispUseArgList(mac, alist);

    return (name);
}

LispObj *
Lisp_Defsetf(LispMac *mac, LispBuiltin *builtin)
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

    if (!SYMBOL_P(function))
	LispDestroy(mac, "%s: bad SETF-METHOD name %s",
		    STRFUN(builtin), STROBJ(function));

    if (body == NIL || (CONS_P(body) && STRING_P(CAR(body)))) {
	if (!SYMBOL_P(lambda_list))
	    LispDestroy(mac, "%s: syntax error %s %s",
			STRFUN(builtin), STROBJ(function), STROBJ(lambda_list));
	if (body != NIL)
	    LispAddDocumentation(mac, function, CAR(body), LispDocSetf);

	LispSetAtomSetfProperty(mac, function->data.atom, lambda_list, NULL);

	return (function);
    }

    alist = LispCheckArguments(mac, LispSetf, lambda_list, STRPTR(function));

    store = CAR(body);
    if (!CONS_P(store))
	LispDestroy(mac, "%s: %s is a bad store value",
		    STRFUN(builtin), STROBJ(store));
    for (obj = store; CONS_P(obj); obj = CDR(obj))
	if (!SYMBOL_P(CAR(obj)))
	    LispDestroy(mac, "%s: %s cannot be a variable name",
			STRFUN(builtin), STROBJ(CAR(obj)));

    body = CDR(body);
    if (CONS_P(body) && STRING_P(CAR(body))) {
	LispAddDocumentation(mac, function, CAR(body), LispDocSetf);
	body = CDR(body);
    }

    lambda = LispNewLambda(mac, function, body, store, LispSetf);
    LispSetAtomSetfProperty(mac, function->data.atom, lambda, alist);
    LispUseArgList(mac, alist);

    return (function);
}

LispObj *
Lisp_Defvar(LispMac *mac, LispBuiltin *builtin)
/*
 defvar name &optional (initial-value nil bound) documentation
 */
{
    LispObj *name, *initial_value, *bound, *documentation;

    documentation = ARGUMENT(3);
    bound = ARGUMENT(2);
    initial_value = ARGUMENT(1);
    name = ARGUMENT(0);

    ERROR_CHECK_SYMBOL(name);
    if (documentation != NIL) {
	ERROR_CHECK_STRING(documentation);
    }

    LispProclaimSpecial(mac, name, bound == T ? EVAL(initial_value) : NULL,
			documentation);

    return (name);
}

LispObj *
Lisp_Delete(LispMac *mac, LispBuiltin *builtin)
/*
 delete item sequence &key from-end test test-not start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, DELETE, NONE));
}

LispObj *
Lisp_DeleteIf(LispMac *mac, LispBuiltin *builtin)
/*
 delete-if predicate sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, DELETE, IF));
}

LispObj *
Lisp_DeleteIfNot(LispMac *mac, LispBuiltin *builtin)
/*
 delete-if-not predicate sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, DELETE, IFNOT));
}

LispObj *
Lisp_DeleteDuplicates(LispMac *mac, LispBuiltin *builtin)
/*
 delete-duplicates sequence &key from-end test test-not start end key
 */
{
    return (LispDeleteOrRemoveDuplicates(mac, builtin, DELETE));
}

LispObj *
Lisp_Do(LispMac *mac, LispBuiltin *builtin)
/*
 do init test &rest body
 */
{
    return (LispDo(mac, builtin, 0));
}

LispObj *
Lisp_DoP(LispMac *mac, LispBuiltin *builtin)
/*
 do init test &rest body
 */
{
    return (LispDo(mac, builtin, 1));
}

LispObj *
Lisp_Documentation(LispMac *mac, LispBuiltin *builtin)
/*
 documentation symbol type
 */
{
    Atom_id atom;
    LispObj *symbol, *type;
    LispDocType_t doc_type = LispDocVariable;

    type = ARGUMENT(1);
    symbol = ARGUMENT(0);

    ERROR_CHECK_SYMBOL(symbol);
    ERROR_CHECK_SYMBOL(type);
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
	LispDestroy(mac, "%s: unknown documentation type %s",
		    STRFUN(builtin), STROBJ(type));
	/*NOTREACHED*/
	doc_type = 0;
    }

    return (LispGetDocumentation(mac, symbol, doc_type));
}

LispObj *
Lisp_DoList(LispMac *mac, LispBuiltin *builtin)
{
    return (LispDoListTimes(mac, builtin, 0));
}

LispObj *
Lisp_DoTimes(LispMac *mac, LispBuiltin *builtin)
{
    return (LispDoListTimes(mac, builtin, 1));
}

LispObj *
Lisp_Elt(LispMac *mac, LispBuiltin *builtin)
/*
 elt sequence index
 svref sequence index
 */
{
    long offset, length;
    LispObj *result, *sequence, *oindex;

    oindex = ARGUMENT(1);
    sequence = ARGUMENT(0);

    length = LispLength(mac, sequence);
    ERROR_CHECK_INDEX(oindex);
    offset = oindex->data.integer;

    if (offset >= length)
	LispDestroy(mac, "%s: index %ld too large for sequence length %ld",
		    STRFUN(builtin), offset, length);

    if (STRING_P(sequence))
	result = CHAR(*(unsigned char*)(THESTR(sequence) + offset));
    else {
	if (sequence->type == LispArray_t)
	    sequence = sequence->data.array.list;

	for (; offset > 0; offset--, sequence = CDR(sequence))
	    ;
	result = CAR(sequence);
    }

    return (result);
}

/*
  The predicate endp is the recommended way to test for the end of a list.
  It is false of conses, true of nil, and an error for all other arguments.

  Implementation note: Implementations are encouraged to signal an error,
  especially in the interpreter, for a non-list argument. The endp function
  is defined so as to allow compiled code to perform simply an atom check or
  a null check if speed is more important than safety.
 */
LispObj *
Lisp_Endp(LispMac *mac, LispBuiltin *builtin)
/*
 endp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    if (object == NIL)
	return (T);
    ERROR_CHECK_LIST(object);

    return (NIL);
}

LispObj *
Lisp_Eq(LispMac *mac, LispBuiltin *builtin)
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
Lisp_Eql(LispMac *mac, LispBuiltin *builtin)
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
Lisp_Equal(LispMac *mac, LispBuiltin *builtin)
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
Lisp_Equalp(LispMac *mac, LispBuiltin *builtin)
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
Lisp_Error(LispMac *mac, LispBuiltin *builtin)
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
    LispDestroy(mac, "%s", THESTR(string));
    /*NOTREACHED*/

    /* No need to call GC_ENTER() and GC_LEAVE() macros */
    return (NIL);
}

LispObj *
Lisp_Eval(LispMac *mac, LispBuiltin *builtin)
/*
 eval form
 */
{
    LispObj *form;

    form = ARGUMENT(0);

    return (EVAL(form));
}

LispObj *
Lisp_Fboundp(LispMac *mac, LispBuiltin *builtin)
/*
 fboundp symbol
 */
{
    LispAtom *atom;

    LispObj *symbol = ARGUMENT(0);

    ERROR_CHECK_SYMBOL(symbol);

    atom = symbol->data.atom;
    if (atom->a_function || atom->a_builtin)
	return (T);

    return (NIL);
}

LispObj *
Lisp_Fill(LispMac *mac, LispBuiltin *builtin)
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

    LispCheckSequenceStartEnd(mac, builtin, sequence, ostart, oend,
			      &start, &end, &length);

    if (STRING_P(sequence)) {
	int ch;
	char *string = THESTR(sequence);

	ERROR_CHECK_CHARACTER(item);
	ch = item->data.integer;
	for (i = start; i < end; i++)
	    string[i] = ch;
    }
    else {
	LispObj *list;

	if (CONS_P(sequence))
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
Lisp_Fmakunbound(LispMac *mac, LispBuiltin *builtin)
/*
 fmkaunbound symbol
 */
{
    LispObj *symbol;

    symbol = ARGUMENT(0);

    ERROR_CHECK_SYMBOL(symbol);
    if (symbol->data.atom->a_function)
	LispRemAtomFunctionProperty(mac, symbol->data.atom);
    else if (symbol->data.atom->a_builtin)
	LispRemAtomBuiltinProperty(mac, symbol->data.atom);

    return (symbol);
}

LispObj *
Lisp_Funcall(LispMac *mac, LispBuiltin *builtin)
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
Lisp_Get(LispMac *mac, LispBuiltin *builtin)
/*
 get symbol indicator &optional default
 */
{
    LispObj *result;

    LispObj *symbol, *indicator, *defalt;

    defalt = ARGUMENT(2);
    indicator = ARGUMENT(1);
    symbol = ARGUMENT(0);

    if (!SYMBOL_P(symbol)) {
	if (symbol == NIL)
	    symbol = ATOM(Snil);
	else if (symbol == T)
	    symbol = ATOM(St);
	else
	    LispDestroy(mac, "%s: %s is not a symbol",
			STRFUN(builtin), STROBJ(symbol));
    }

    result = LispGetAtomProperty(mac, symbol->data.atom, indicator);

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
Lisp_Getenv(LispMac *mac, LispBuiltin *builtin)
/*
 getenv name
 */
{
    char *value;

    LispObj *name;

    name = ARGUMENT(0);

    ERROR_CHECK_STRING(name);
    value = getenv(THESTR(name));

    return (value ? STRING(value) : NIL);
}

LispObj *
Lisp_Gc(LispMac *mac, LispBuiltin *builtin)
/*
 gc &optional car cdr
 */
{
    LispObj *car, *cdr;

    cdr = ARGUMENT(1);
    car = ARGUMENT(0);

    LispGC(mac, car, cdr);

    return (car == NIL && cdr == NIL ? NIL : T);
}

LispObj *
Lisp_Go(LispMac *mac, LispBuiltin *builtin)
/*
 go tag
 */
{
    unsigned blevel = mac->block.block_level;

    LispObj *tag;

    tag = ARGUMENT(0);

    while (blevel) {
	LispBlock *block = mac->block.block[--blevel];

	if (block->type == LispBlockClosure)
	    /* if reached a function call */
	    break;
	if (block->type == LispBlockBody) {
	    mac->block.block_ret = tag;
	    LispBlockUnwind(mac, block);
	    BLOCKJUMP(block);
	}
     }

    LispDestroy(mac, "%s: no visible tagbody for %s",
		STRFUN(builtin), STROBJ(tag));
    /*NOTREACHED*/
    return (NIL);
}

LispObj *
Lisp_If(LispMac *mac, LispBuiltin *builtin)
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
Lisp_Intersection(LispMac *mac, LispBuiltin *builtin)
/*
 intersection list1 list2 &key test test-not key
 */
{
    return (LispListSet(mac, builtin, INTERSECTION));
}

LispObj *
Lisp_Keywordp(LispMac *mac, LispBuiltin *builtin)
/*
 keywordp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    /* KEYWORD_P only checks the symbol package, not the object type */
    return (SYMBOL_P(object) && KEYWORD_P(object) ? T : NIL);
}

LispObj *
Lisp_Lambda(LispMac *mac, LispBuiltin *builtin)
/*
 lambda lambda-list &rest body
 */
{
    LispArgList *alist;

    LispObj *lambda, *lambda_list, *body;

    body = ARGUMENT(1);
    lambda_list = ARGUMENT(0);

    alist = LispCheckArguments(mac, LispLambda, lambda_list, STRPTR(Olambda));

    lambda_list = LispListProtectedArguments(mac, alist);
    GCProtect();
    lambda = LispNewLambda(mac, OPAQUE(alist, LispArgList_t),
			   body, lambda_list, LispLambda);
    GCUProtect();
    LispUseArgList(mac, alist);

    return (lambda);
}

LispObj *
Lisp_Last(LispMac *mac, LispBuiltin *builtin)
/*
 last list &optional count
 */
{
    long count, length;
    LispObj *list, *ocount;

    ocount = ARGUMENT(1);
    list = ARGUMENT(0);

    if (!CONS_P(list))
	return (list);

    length = LispLength(mac, list);

    if (ocount == NIL)
	count = 1;
    else {
	ERROR_CHECK_INDEX(ocount);
	count = ocount->data.integer;
    }

    if (count >= length)
	return (list);

    length -= count;
    for (; length > 0; length--)
	list = CDR(list);

    return (list);
}

LispObj *
Lisp_Length(LispMac *mac, LispBuiltin *builtin)
/*
 length sequence
 */
{
    LispObj *sequence;

    sequence = ARGUMENT(0);

    return (INTEGER(LispLength(mac, sequence)));
}

LispObj *
Lisp_Let(LispMac *mac, LispBuiltin *builtin)
/*
 let init &rest body
 */
{
    GC_ENTER();
    int head = mac->env.length;
    LispObj *init, *body, *pair, *result, *list, *cons = NIL;

    body = ARGUMENT(1);
    init = ARGUMENT(0);

    if (init != NIL) {
	ERROR_CHECK_LIST(init);
    }
    for (list = NIL; CONS_P(init); init = CDR(init)) {
	LispObj *symbol, *value;

	pair = CAR(init);
	if (SYMBOL_P(pair)) {
	    symbol = pair;
	    value = NIL;
	}
	else {
	    ERROR_CHECK_LIST(pair);
	    symbol = CAR(pair);
	    ERROR_CHECK_SYMBOL(symbol);
	    pair = CDR(pair);
	    if (CONS_P(pair)) {
		value = CAR(pair);
		if (CDR(pair) != NIL)
		    LispDestroy(mac, "%s: too much arguments to initialize %s",
				STRFUN(builtin), STROBJ(symbol));
		if (NCONSTANT_P(value))
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
    for (; CONS_P(list); list = CDR(list)) {
	pair = CAR(list);
	ERROR_CHECK_CONSTANT(CAR(pair));
	LispAddVar(mac, CAR(pair), CDR(pair));
	++mac->env.head;
    }
    /* Values of symbols are now protected */
    GC_LEAVE();

    /* execute body */
    for (result = NIL; CONS_P(body); body = CDR(body))
	result = EVAL(CAR(body));

    mac->env.head = mac->env.length = head;

    return (result);
}

LispObj *
Lisp_LetP(LispMac *mac, LispBuiltin *builtin)
/*
 let* init &rest body
 */
{
    int head = mac->env.length;
    LispObj *init, *body, *pair, *result;

    body = ARGUMENT(1);
    init = ARGUMENT(0);

    if (init != NIL) {
	ERROR_CHECK_LIST(init);
    }
    for (; CONS_P(init); init = CDR(init)) {
	LispObj *symbol, *value;

	pair = CAR(init);
	if (SYMBOL_P(pair)) {
	    symbol = pair;
	    value = NIL;
	}
	else {
	    ERROR_CHECK_LIST(pair);
	    symbol = CAR(pair);
	    ERROR_CHECK_SYMBOL(symbol);
	    pair = CDR(pair);
	    if (CONS_P(pair)) {
		value = CAR(pair);
		if (CDR(pair) != NIL)
		    LispDestroy(mac, "%s: too much arguments to initialize %s",
				STRFUN(builtin), STROBJ(symbol));
		if (NCONSTANT_P(value))
		    value = EVAL(value);
	    }
	    else
		value = NIL;
	}

	ERROR_CHECK_CONSTANT(symbol);
	LispAddVar(mac, symbol, value);
	++mac->env.head;
    }

    /* execute body */
    for (result = NIL; CONS_P(body); body = CDR(body))
	result = EVAL(CAR(body));

    mac->env.head = mac->env.length = head;

    return (result);
}

LispObj *
Lisp_List(LispMac *mac, LispBuiltin *builtin)
/*
 list &rest args
 */
{
    LispObj *args;

    args = ARGUMENT(0);

    return (args);
}

LispObj *
Lisp_ListP(LispMac *mac, LispBuiltin *builtin)
/*
 list* object &rest more-objects
 */
{
    GC_ENTER();
    LispObj *result, *cons;

    LispObj *object, *more_objects;

    more_objects = ARGUMENT(1);
    object = ARGUMENT(0);

    if (more_objects == NIL)
	return (object);

    result = cons = CONS(object, CAR(more_objects));
    GC_PROTECT(result);
    for (more_objects = CDR(more_objects); CONS_P(more_objects);
	 more_objects = CDR(more_objects)) {
	object = CAR(more_objects);
	RPLACD(cons, CONS(CDR(cons), object));
	cons = CDR(cons);
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Listp(LispMac *mac, LispBuiltin *builtin)
/*
 listp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (object == NIL || CONS_P(object) ? T : NIL);
}

LispObj *
LispListSet(LispMac *mac, LispBuiltin *builtin, int function)
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
    if (list1 != NIL) {
	ERROR_CHECK_LIST(list1);
    }
    if (list2 != NIL) {
	ERROR_CHECK_LIST(list2);
    }

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
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT",
		    STRFUN(builtin));

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
	for (cmp2 = CDR(list2); CONS_P(cmp2); cmp2 = CDR(cmp2)) {
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
    for (cmp1 = list1; CONS_P(cmp1); cmp1 = CDR(cmp1)) {
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
	for (cmp2 = clist2; CONS_P(cmp2); cmp2 = CDR(cmp2)) {
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
	for (cmp2 = list2; CONS_P(cmp2); cmp2 = CDR(cmp2)) {
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
	    for (; CONS_P(cmp1); cmp1 = CDR(cmp1)) {
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
    else if (function == NSETDIFFERENCE && CONS_P(cons))
	RPLACD(cons, NIL);

    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Loop(LispMac *mac, LispBuiltin *builtin)
/*
 loop &rest body
 */
{
    LispObj *code, *result;
    LispBlock *block;

    LispObj *body;

    body = ARGUMENT(0);

    result = NIL;
    block = LispBeginBlock(mac, NIL, LispBlockTag);
    if (setjmp(block->jmp) == 0) {
	for (;;)
	    for (code = body; CONS_P(code); code = CDR(code))
		(void)EVAL(CAR(code));
    }
    LispEndBlock(mac, block);
    result = mac->block.block_ret;

    return (result);
}

LispObj *
Lisp_MakeArray(LispMac *mac, LispBuiltin *builtin)
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

    dim = array = NIL; type = LispNil_t;	/* fix gcc warning */

    displaced_index_offset = ARGUMENT(7);
    displaced_to = ARGUMENT(6);
    fill_pointer = ARGUMENT(5);
    adjustable = ARGUMENT(4);
    initial_contents = ARGUMENT(3);
    initial_element = ARGUMENT(2);
    element_type = ARGUMENT(1);
    dimensions = ARGUMENT(0);

    if (INDEX_P(dimensions)) {
	dim = CONS(dimensions, NIL);
	rank = 1;
	count = dimensions->data.integer;
    }
    else if (CONS_P(dimensions)) {
	dim = dimensions;

	for (rank = 0; CONS_P(dim); rank++, dim = CDR(dim)) {
	    obj = CAR(dim);
	    if (!INDEX_P(obj))
		LispDestroy(mac, "%s: %s is a bad array dimension",
			    STRFUN(builtin), STROBJ(dimensions));
	    count *= obj->data.integer;
	}
	dim = dimensions;
    }
    else if (dimensions == NIL) {
	dim = NIL;
	rank = count = 0;
    }
    else
	LispDestroy(mac, "%s: %s is a bad array dimension",
		    STRFUN(builtin), STROBJ(dimensions));

    /* check element-type */
    if (element_type != NIL) {
	if (element_type == T)
	    type = LispTrue_t;
	else if (!SYMBOL_P(element_type))
	    LispDestroy(mac, "%s: unsupported element type %s",
			STRFUN(builtin), STROBJ(element_type));
	else {
	    Atom_id atom = ATOMID(element_type);

	    if (atom == Satom)
		type = LispAtom_t;
	    else if (atom == Sinteger)
		type = LispInteger_t;
	    else if (atom == Scharacter)
		type = LispCharacter_t;
	    else if (atom == Sreal)
		type = LispReal_t;
	    else if (atom == Sstring)
		type = LispString_t;
	    else if (atom == Slist)
		type = LispCons_t;
	    else if (atom == Sopaque)
		type = LispOpaque_t;
	    else
		LispDestroy(mac, "%s: unsupported element type %s",
			    STRFUN(builtin), STRPTR(element_type));
	}
    }

    /* check initial-contents */
    if (rank && initial_contents != NIL && !CONS_P(initial_contents))
	LispDestroy(mac, "%s: %s is not a list",
		    STRFUN(builtin), STROBJ(initial_contents));

    /* check displaced-to */
    if (displaced_to != NIL && displaced_to->type != LispArray_t)
	LispDestroy(mac, "%s: %s is not an array",
		    STRFUN(builtin), STROBJ(displaced_to));

    /* check displaced-index-offset */
    offset = -1;
    if (displaced_index_offset != NIL) {
	if (!INDEX_P(displaced_index_offset))
	    LispDestroy(mac, "%s: %s is a bad :DISPLACED-INDEX-OFFSET",
			STRFUN(builtin), STROBJ(displaced_index_offset));
	offset = displaced_index_offset->data.integer;
    }

    c = 0;
    if (initial_element != NIL)
	++c;
    if (initial_contents != NIL)
	++c;
    if (displaced_to != NIL || offset >= 0)
	++c;
    if (c > 1)
	LispDestroy(mac, "%s: more than one initialization specified",
		    STRFUN(builtin));

    zero = count == 0;
    if (displaced_to != NIL) {
	if (offset < 0)
	    offset = 0;
	for (c = 1, obj = displaced_to->data.array.dim; obj != NIL;
	     obj = CDR(obj))
	    c *= CAR(obj)->data.integer;
	if (c < count + offset)
	    LispDestroy(mac, "%s: array-total-size + displaced-index-offset "
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
		if (!CONS_P(array))
		    LispDestroy(mac, "%s: bad argument or size %s",
				STRFUN(builtin), STROBJ(array));
	    if (array != NIL)
		LispDestroy(mac, "%s: bad argument or size %s",
			    STRFUN(builtin), STROBJ(array));
	    array = initial_contents;
	}
	else {
	    LispObj *err = NIL;
	    /* check if list matches */
	    int i, j, k, *dims, *loop;

	    /* create iteration variables */
	    dims = LispMalloc(mac, sizeof(int) * rank);
	    loop = LispCalloc(mac, 1, sizeof(int) * (rank - 1));
	    for (i = 0, obj = dim; CONS_P(obj); i++, obj = CDR(obj))
		dims[i] = CAR(obj)->data.integer;

	    /* check if list matches specified dimensions */
	    while (loop[0] < dims[0]) {
		for (obj = initial_contents, i = 0; i < rank - 1; i++) {
		    for (j = 0; j < loop[i]; j++)
			obj = CDR(obj);
		    err = obj;
		    if (!CONS_P(obj = CAR(obj)))
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
		    if (!CONS_P(obj))
			goto make_array_error;
		}
		if (obj == NIL)
		    continue;
make_array_error:
		LispFree(mac, dims);
		LispFree(mac, loop);
		LispDestroy(mac, "%s: bad argument or size %s",
			    STRFUN(builtin), STROBJ(err));
	    }

	    /* list is correct, use it to fill initial values */

	    /* reset loop */
	    memset(loop, 0, sizeof(int) * (rank - 1));

	    GCProtect();
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
	    LispFree(mac, dims);
	    LispFree(mac, loop);
	    array = LispReverse(array);
	    GCUProtect();
	}
    }
    else {
	GCProtect();
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
	GCUProtect();
    }

    if (type == LispNil_t)
	type = LispTrue_t;
    obj = LispNew(mac, array, dim);
    obj->type = LispArray_t;
    obj->data.array.list = array;
    obj->data.array.dim = dim;
    obj->data.array.rank = rank;
    obj->data.array.type = type;
    obj->data.array.zero = zero;

    return (obj);
}

LispObj *
Lisp_MakeList(LispMac *mac, LispBuiltin *builtin)
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

    ERROR_CHECK_INDEX(size);
    count = size->data.integer;

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
Lisp_Makunbound(LispMac *mac, LispBuiltin *builtin)
/*
 makunbound symbol
 */
{
    LispObj *symbol;

    symbol = ARGUMENT(0);

    ERROR_CHECK_SYMBOL(symbol);
    LispUnsetVar(mac, symbol);

    return (symbol);
}

LispObj *
Lisp_Mapcar(LispMac *mac, LispBuiltin *builtin)
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
    for (length = 0, alist = list; CONS_P(alist); length++, alist = CDR(alist))
	;

    /* If first argument is not a list... */
    if (length == 0)
	return (NIL);

    /* At least one argument will be passed to function, count how many
     * extra arguments will be used, and calculate result length. */
    count = 0;
    for (rest = more_lists; CONS_P(rest); rest = CDR(rest), count++) {

	/* Check if extra list is really a list, and if it is smaller
	 * than the first list */
	for (i = 0, alist = CAR(rest);
	     i < length && CONS_P(alist);
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
	for (rest = more_lists; CONS_P(rest); rest = CDR(rest)) {

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
Lisp_Maplist(LispMac *mac, LispBuiltin *builtin)
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

    length = mac->protect.length;
    if (length + 2 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = NIL;
    mac->protect.objects[mac->protect.length++] = NIL;

    result = acons = NIL;
    for (count = 0; CONS_P(list); count++, list = CDR(list)) {
	cons = CONS(QUOTE(list), NIL);
	code = CONS(function, cons);
	mac->protect.objects[length] = code;
	for (object = more_lists; CONS_P(object); object = CDR(object)) {
	    LispObj *arglist = CAR(object);

	    if (!CONS_P(arglist))
		goto maplist_done;
	    for (i = count; i > 0; i--) {
		if (!CONS_P(arglist = CDR(arglist)))
		    goto maplist_done;
	    }
	    RPLACD(cons, CONS(QUOTE(arglist), NIL));
	}
	cons = EVAL(code);
	if (result == NIL) {
	    result = acons = CONS(cons, NIL);
	    mac->protect.objects[length + 1] = result;
	}
	else {
	    RPLACD(acons, CONS(cons, NIL));
	    acons = CDR(acons);
	}
    }

maplist_done:
    mac->protect.length = length;

    return (result);
}

LispObj *
Lisp_Member(LispMac *mac, LispBuiltin *builtin)
/*
 member item list &key test test-not key
 */
{
    return (LispAssocOrMember(mac, builtin, MEMBER, NONE));
}

LispObj *
Lisp_MemberIf(LispMac *mac, LispBuiltin *builtin)
/*
 member-if predicate list &key key
 */
{
    return (LispAssocOrMember(mac, builtin, MEMBER, IF));
}

LispObj *
Lisp_MemberIfNot(LispMac *mac, LispBuiltin *builtin)
/*
 member-if-not predicate list &key key
 */
{
    return (LispAssocOrMember(mac, builtin, MEMBER, IFNOT));
}

LispObj *
Lisp_MultipleValueBind(LispMac *mac, LispBuiltin *builtin)
/*
 multiple-value-bind symbols values &rest body
 */
{
    int i, head = mac->env.length;
    LispObj *result, *symbol, *value;

    LispObj *symbols, *values, *body;

    body = ARGUMENT(2);
    values = ARGUMENT(1);
    symbols = ARGUMENT(0);

    result = EVAL(values);
    for (i = -1; CONS_P(symbols); symbols = CDR(symbols), i++) {
	symbol = CAR(symbols);
	ERROR_CHECK_SYMBOL(symbol);
	ERROR_CHECK_CONSTANT(symbol);
	if (i >= 0)
	    value = RETURN(i);
	else if (i < 0)
	    value = result;
	else
	    value = NIL;
	LispAddVar(mac, symbol, value);
	++mac->env.head;
    }

    /* Execute code with binded variables (if any) */
    for (result = NIL; CONS_P(body); body = CDR(body))
	result = EVAL(CAR(body));

    mac->env.head = mac->env.length = head;

    return (result);
}

LispObj *
Lisp_MultipleValueList(LispMac *mac, LispBuiltin *builtin)
/*
 multiple-value-list form
 */
{
    int i;
    GC_ENTER();
    LispObj *form, *result, *cons;

    form = ARGUMENT(0);

    result = EVAL(form);

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
Lisp_Nconc(LispMac *mac, LispBuiltin *builtin)
/*
 nconc &rest lists
 */
{
    LispObj *list, *lists, *tail;

    lists = ARGUMENT(0);

    if (lists == NIL)
	return (NIL);

    /* first check if all arguments but last are lists */
    for (tail = lists; CONS_P(CDR(tail)); tail = CDR(tail)) {
	list = CAR(tail);
	if (list != NIL) {
	    ERROR_CHECK_LIST(list);
	}
    }

    /* skip initial NIL lists */
    for (list = CAR(lists); CONS_P(lists); lists = CDR(lists))
	if ((list = CAR(lists)) != NIL)
	    break;

    if (list != NIL && CONS_P(list)) {
	for (tail = list; CONS_P(CDR(tail)); tail = CDR(tail))
	    ;
	for (lists = CDR(lists); CONS_P(lists); lists = CDR(lists)) {
	    RPLACD(tail, CAR(lists));
	    for (; CONS_P(CDR(tail)); tail = CDR(tail))
		;
	}
	/* if last argument is not a list */
	if (lists != NIL)
	    RPLACD(tail, lists);
    }

    return (list);
}

LispObj *
Lisp_Nreverse(LispMac *mac, LispBuiltin *builtin)
/*
 nreverse sequence
 */
{
    return (LispXReverse(mac, builtin, 1));
}

LispObj *
Lisp_NsetDifference(LispMac *mac, LispBuiltin *builtin)
/*
 nset-difference list1 list2 &key test test-not key
 */
{
    return (LispListSet(mac, builtin, NSETDIFFERENCE));
}

LispObj *
Lisp_Nsubstitute(LispMac *mac, LispBuiltin *builtin)
/*
 nsubstitute newitem olditem sequence &key from-end test test-not start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, NSUBSTITUTE, NONE));
}

LispObj *
Lisp_NsubstituteIf(LispMac *mac, LispBuiltin *builtin)
/*
 nsubstitute-if newitem test sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, NSUBSTITUTE, IF));
}

LispObj *
Lisp_NsubstituteIfNot(LispMac *mac, LispBuiltin *builtin)
/*
 nsubstitute-if-not newitem test sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, NSUBSTITUTE, IFNOT));
}

LispObj *
Lisp_Nth(LispMac *mac, LispBuiltin *builtin)
/*
 nth index list
 */
{
    long position;
    LispObj *oindex, *list;

    list = ARGUMENT(1);
    oindex = ARGUMENT(0);

    ERROR_CHECK_INDEX(oindex);
    position = oindex->data.integer;

    if (list == NIL)
	return (NIL);

    ERROR_CHECK_LIST(list);
    for (; position > 0; position--) {
	if (!CONS_P(list))
	    return (NIL);
	list = CDR(list);
    }

    return (CONS_P(list) ? CAR(list) : NIL);
}

LispObj *
Lisp_Nthcdr(LispMac *mac, LispBuiltin *builtin)
/*
 nthcdr index list
 */
{
    long position;
    LispObj *oindex, *list;

    list = ARGUMENT(1);
    oindex = ARGUMENT(0);

    ERROR_CHECK_INDEX(oindex);
    position = oindex->data.integer;

    if (list == NIL)
	return (NIL);
    ERROR_CHECK_LIST(list);

    for (; position > 0; position--) {
	if (!CONS_P(list))
	    return (NIL);
	list = CDR(list);
    }

    return (list);
}

LispObj *
Lisp_Null(LispMac *mac, LispBuiltin *builtin)
/*
 null list
 */
{
    LispObj *list;

    list = ARGUMENT(0);

    return (list == NIL ? T : NIL);
}

LispObj *
Lisp_Or(LispMac *mac, LispBuiltin *builtin)
/*
 or &rest args
 */
{
    LispObj *result = NIL, *args;

    args = ARGUMENT(0);

    for (; CONS_P(args); args = CDR(args)) {
	if (NCONSTANT_P(result = CAR(args)))
	    result = EVAL(result);
	if (result != NIL)
	    break;
    }

    return (result);
}

LispObj *
LispPosition(LispMac *mac, LispBuiltin *builtin, int comparison)
/*
 position item sequence &key from-end test test-not start end key
 position-if predicate sequence &key from-end start end key
 position-if-not predicate sequence &key from-end start end key
 */
{
    GC_ENTER();
    int code = 0, istring;
    char *string = NULL;
    long offset = -1, start, end, length, i = comparison == NONE ? 7 : 5;
    LispObj *arg1, *arg2, *result, **objects = NULL;

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

    LispCheckSequenceStartEnd(mac, builtin, sequence, ostart, oend,
			      &start, &end, &length);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT",
		    STRFUN(builtin));

    if (comparison == NONE) {
	if (test != NIL)
	    predicate = test;
	else if (test_not != NIL)
	    predicate = test_not;
	code = FCODE(predicate);
    }

    arg1 = NIL;
    istring = STRING_P(sequence);
    if (istring) {
	if (comparison == NONE) {
	    ERROR_CHECK_CHARACTER(item);
	}
	string = THESTR(sequence);
    }
    else {
	if (!CONS_P(sequence))
	    sequence = sequence->data.array.list;
	for (i = 0; i < start; i++)
	    sequence = CDR(sequence);
    }
    arg2 = comparison == NONE ? item : NIL;

    if ((length = end - start) == 0)
	return (NIL);

    if (from_end != NIL && !istring) {
	objects = LispMalloc(mac, sizeof(LispObj*) * length);
	for (i = length - 1; i >= 0; i--, sequence = CDR(sequence))
	    objects[i] = CAR(sequence);
    }

    for (i = 0; i < length; i++) {
	if (istring)
	    arg1 = CHAR(string[from_end == NIL ? i + start : end - i - 1]);
	else
	    arg1 = from_end == NIL ? CAR(sequence) : objects[i];

	if (key != NIL) {
	    arg1 = APPLY1(key, arg1);
	    GC_PROTECT(arg1);
	    if (comparison == NONE) {
		arg2 = APPLY1(key, item);
		GC_PROTECT(arg2);
	    }
	}

	/* Update list */
	if (!istring && from_end == NIL)
	    sequence = CDR(sequence);

	if (comparison == NONE)
	    result = FCOMPARE(predicate, arg1, arg2, code);
	else
	    result = APPLY1(predicate, arg1);

	/* Unprotect arg1 and arg2 */
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
	LispFree(mac, objects);

    return (offset == -1 ? NIL : SMALLINT(offset));
}

LispObj *
Lisp_Pop(LispMac *mac, LispBuiltin *builtin)
/*
 pop place
 */
{
    LispObj *result, *value;

    LispObj *place;

    place = ARGUMENT(0);

    if (SYMBOL_P(place)) {
	result = LispGetVar(mac, place);
	if (result == NULL)
	    LispDestroy(mac, "EVAL: the variable %s is unbound", STROBJ(place));
	ERROR_CHECK_CONSTANT(place);
	if (result != NIL) {
	    ERROR_CHECK_LIST(result);
	    value = CDR(result);
	    result = CAR(result);
	}
	else
	    value = NIL;
	LispSetVar(mac, place, value);
    }
    else {
	GC_ENTER();
	LispObj quote;

	result = EVAL(place);
	if (result != NIL) {
	    ERROR_CHECK_LIST(result);
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
Lisp_Position(LispMac *mac, LispBuiltin *builtin)
/*
 position item sequence &key from-end test test-not start end key
 */
{
    return (LispPosition(mac, builtin, NONE));
}

LispObj *
Lisp_PositionIf(LispMac *mac, LispBuiltin *builtin)
/*
 position-if predicate sequence &key from-end start end key
 */
{
    return (LispPosition(mac, builtin, IF));
}

LispObj *
Lisp_PositionIfNot(LispMac *mac, LispBuiltin *builtin)
/*
 position-if-not predicate sequence &key from-end start end key
 */
{
    return (LispPosition(mac, builtin, IFNOT));
}

LispObj *
Lisp_Prin1(LispMac *mac, LispBuiltin *builtin)
/*
 prin1 object &optional output-stream
 */
{
    LispObj *object, *output_stream;

    output_stream = ARGUMENT(1);
    object = ARGUMENT(0);

    LispPrint(mac, object, output_stream, 0);

    return (object);
}

LispObj *
Lisp_Princ(LispMac *mac, LispBuiltin *builtin)
/*
 princ object &optional output-stream
 */
{
    int escape;
    LispObj *object, *output_stream;

    output_stream = ARGUMENT(1);
    object = ARGUMENT(0);

    escape = LispGetEscape(mac, output_stream);
    LispSetEscape(mac, output_stream, 1);
    LispPrint(mac, object, output_stream, 0);
    LispSetEscape(mac, output_stream, escape);

    return (object);
}

LispObj *
Lisp_Print(LispMac *mac, LispBuiltin *builtin)
/*
 print object &optional output-stream
 */
{
    LispObj *object, *output_stream;

    output_stream = ARGUMENT(1);
    object = ARGUMENT(0);

    LispPrint(mac, object, output_stream, 1);

    return (object);
}

LispObj *
Lisp_Proclaim(LispMac *mac, LispBuiltin *builtin)
/*
 proclaim declaration
 */
{
    LispObj *declaration;

    declaration = ARGUMENT(0);

    ERROR_CHECK_LIST(declaration);
    else {
	LispObj *arguments = declaration, *object = CAR(arguments);
	char *operation;

	if (!SYMBOL_P(object))
	    LispDestroy(mac, "%s: cannot proclaim %s",
			STRFUN(builtin), STROBJ(object));
	operation = STRPTR(object);
	if (strcmp(operation, "SPECIAL") == 0) {
	    for (arguments = CDR(arguments); CONS_P(arguments);
		 arguments = CDR(arguments)) {
		object = CAR(arguments);
		ERROR_CHECK_SYMBOL(object);
		LispProclaimSpecial(mac, object, NULL, NIL);
	    }
	}
	else if (strcmp(operation, "TYPE") == 0) {
	    /* XXX no type checking yet, but should be added */
	}
	/* else do nothing */
    }

    return (NIL);
}

LispObj *
Lisp_Prog1(LispMac *mac, LispBuiltin *builtin)
/*
 prog1 first &rest body
 */
{
    GC_ENTER();
    LispObj *result;

    LispObj *first, *body;

    body = ARGUMENT(1);
    first = ARGUMENT(0);

    if (NCONSTANT_P(result = first))
	result = EVAL(result);

    GC_PROTECT(result);
    for (; CONS_P(body); body = CDR(body))
	(void)EVAL(CAR(body));
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Prog2(LispMac *mac, LispBuiltin *builtin)
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

    if (NCONSTANT_P(first))
	(void)EVAL(first);
    if (NCONSTANT_P(result = second))
	result = EVAL(result);
    GC_PROTECT(result);
    for (; CONS_P(body); body = CDR(body))
	(void)EVAL(CAR(body));
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Progn(LispMac *mac, LispBuiltin *builtin)
/*
 progn &rest body
 */
{
    LispObj *result = NIL;

    LispObj *body;

    body = ARGUMENT(0);

    for (; CONS_P(body); body = CDR(body))
	if (NCONSTANT_P(result = CAR(body)))
	    result = EVAL(result);

    return (result);
}

LispObj *
Lisp_Progv(LispMac *mac, LispBuiltin *builtin)
/*
 progv symbols values &rest body
 */
{
    int head = mac->env.length;
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
    for (; CONS_P(symbols); symbols = CDR(symbols)) {
	if (!CONS_P(values))
	    break;
	ERROR_CHECK_SYMBOL(CAR(symbols));
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
	ERROR_CHECK_CONSTANT(CAR(cons));
	LispAddVar(mac, CAR(cons), CDR(cons));
	++mac->env.head;
    }
    GC_LEAVE();

    for (; CONS_P(body); body = CDR(body))
	if (NCONSTANT_P(res = CAR(body)))
	    res = EVAL(res);

    mac->env.head = mac->env.length = head;

    return (res);
}

LispObj *
Lisp_Provide(LispMac *mac, LispBuiltin *builtin)
/*
 provide module
 */
{
    LispObj *module, *obj;

    module = ARGUMENT(0);

    ERROR_CHECK_STRING(module);
    for (obj = MOD; obj != NIL; obj = CDR(obj)) {
	if (THESTR(CAR(obj)) == THESTR(module))
	    return (module);
    }

    if (MOD == NIL)
	MOD = CONS(module, NIL);
    else {
	RPLACD(MOD, CONS(CAR(MOD), CDR(MOD)));
	RPLACA(MOD, module);
    }

    LispSetVar(mac, mac->modules, MOD);

    return (MOD);
}

LispObj *
Lisp_Push(LispMac *mac, LispBuiltin *builtin)
/*
 push item place
 */
{
    LispObj *result, *list;

    LispObj *item, *place;

    place = ARGUMENT(1);
    item = ARGUMENT(0);

    item = EVAL(item);

    if (SYMBOL_P(place)) {
	list = LispGetVar(mac, place);
	if (list == NULL)
	    LispDestroy(mac, "EVAL: the variable %s is unbound", STROBJ(place));
	ERROR_CHECK_CONSTANT(place);
	LispSetVar(mac, place, result = CONS(item, list));
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
Lisp_Pushnew(LispMac *mac, LispBuiltin *builtin)
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
    if (SYMBOL_P(place)) {
	list = LispGetVar(mac, place);
	if (list == NULL)
	    LispDestroy(mac, "EVAL: the variable %s is unbound", STROBJ(place));
	/* Do error checking now. */
	ERROR_CHECK_CONSTANT(place);
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

    result = LispAdjoin(mac, builtin, item, list, key, test, test_not);

    /* Item already in list */
    if (result == list) {
	GC_LEAVE();

	return (result);
    }

    if (SYMBOL_P(place)) {
	ERROR_CHECK_CONSTANT(place);
	LispSetVar(mac, place, result);
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
Lisp_Quit(LispMac *mac, LispBuiltin *builtin)
/*
 quit &optional status
 */
{
    int status = 0;
    LispObj *ostatus;

    ostatus = ARGUMENT(0);

    if (INT_P(ostatus))
	status = (int)ostatus->data.integer;
    else if (ostatus != NIL)
	LispDestroy(mac, "%s: bad exit status argument %s",
		    STRFUN(builtin), STROBJ(ostatus));

    exit(status);
}

LispObj *
Lisp_Quote(LispMac *mac, LispBuiltin *builtin)
/*
 quote object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (object);
}

LispObj *
Lisp_Replace(LispMac *mac, LispBuiltin *builtin)
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

    LispCheckSequenceStartEnd(mac, builtin, sequence1, ostart1, oend1,
			      &start1, &end1, &length1);
    LispCheckSequenceStartEnd(mac, builtin, sequence2, ostart2, oend2,
			      &start2, &end2, &length2);

    if (start1 == end1 || start2 == end2)
	return (sequence1);

    length = end1 - start1;
    if (length > end2 - start2)
	length = end2 - start2;

    if (STRING_P(sequence1)) {
	if (!STRING_P(sequence2))
	    LispDestroy(mac, "%s: cannot store %s in %s",
			STRFUN(builtin), STROBJ(sequence2), THESTR(sequence1));

	memcpy(THESTR(sequence1) + start1, THESTR(sequence2) + start2, length);
    }
    else {
	int i;
	LispObj *from, *to;

	if (sequence1->type == LispArray_t)
	    sequence1 = sequence1->data.array.list;
	if (sequence2->type == LispArray_t)
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
LispDeleteOrRemoveDuplicates(LispMac *mac, LispBuiltin *builtin, int function)
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

    LispCheckSequenceStartEnd(mac, builtin, sequence, ostart, oend,
			      &start, &end, &length);

    /* Check if need to do something */
    if (start == end)
	return (sequence);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT",
		    STRFUN(builtin));

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
    if (STRING_P(sequence)) {
	char *ptr, *string, *buffer = LispMalloc(mac, length + 1);

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
	    string = LispMalloc(mac, length + 1);
	    for (ptr = THESTR(sequence) + length - 1, i = 0; i < length; i++)
		string[i] = *ptr--;
	    string[i] = '\0';
	}

	ptr = buffer;
	/* Copy leading bytes */
	for (i = 0; i < start; i++)
	    *ptr++ = string[i];

	value = CHAR(string[i]);
	if (key != NIL)
	    value = APPLY1(key, value);
	result = cons = CONS(value, NIL);
	GC_PROTECT(result);
	for (++i; i < end; i++) {
	    value = CHAR(string[i]);
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
		for (i = 0, ptr = buffer + strlen(buffer); ptr > buffer; i++)
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
		LispMused(mac, ptr);
	    }
	}
	else
	    result = sequence;
	if (!count || from_end != NIL) {
	    LispFree(mac, buffer);
	    if (count)
		LispFree(mac, string);
	}
    }
    else {
	long xlength = end - start;
	LispObj *list, *object, **kobjects = NULL, **xobjects;
	LispObj **objects = LispMalloc(mac, sizeof(LispObj*) * xlength);

	if (!CONS_P(sequence))
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
	    kobjects = LispMalloc(mac, sizeof(LispObj*) * xlength);
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
				if (CONS_P(CDR(result))) {
				    RPLACA(result, CADR(result));
				    RPLACD(result, CDDR(result));
				}
				else {
				    RPLACA(result, CDR(result));
				    RPLACD(result, NIL);
				}
			    }
			    else {
				if (CONS_P(CDR(cons)))
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
	LispFree(mac, objects);
	if (key != NIL)
	    LispFree(mac, kobjects);

	if (count && !CONS_P(sequence)) {
	    if (function == REMOVE)
		result = VECTOR(result);
	    else {
		length = CAR(sequence->data.array.dim)->data.integer - count;
		CAR(sequence->data.array.dim) = SMALLINT(length);
		result = sequence;
	    }
	}
    }
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_RemoveDuplicates(LispMac *mac, LispBuiltin *builtin)
/*
 remove-duplicates sequence &key from-end test test-not start end key
 */
{
    return (LispDeleteOrRemoveDuplicates(mac, builtin, REMOVE));
}



LispObj *
LispDeleteRemoveXSubstitute(LispMac *mac, LispBuiltin *builtin,
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
    LispCheckSequenceStartEnd(mac, builtin, sequence, ostart, oend,
			      &start, &end, &length);

    /* Check count argument */
    if (ocount == NIL) {
	count = length;
	/* Doesn't matter, but left to right should be slightly faster */
	from_end = NIL;
    }
    else {
	ERROR_CHECK_INDEX(ocount);
	count = ocount->data.integer;
    }

    /* Check if need to do something */
    if (start == end || count == 0)
	return (sequence);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT",
		    STRFUN(builtin));

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
    if (STRING_P(sequence)) {
	char *buffer, *string;

	if (comparison == NONE) {
	    ERROR_CHECK_CHARACTER(item);
	}
	if (substitute) {
	    ERROR_CHECK_CHARACTER(newitem);
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
	buffer = LispMalloc(mac, length + 1);

	/* Copy leading bytes, if any */
	for (i = 0; i < start; i++)
	    buffer[i] = string[i];

	for (j = xstart; i != xend && count > 0; i += xinc) {
	    compare = CHAR(string[i]);
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
		    buffer[j] = newitem->data.integer;
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
	    LispFree(mac, THESTR(sequence));
	    LispMused(mac, buffer);
	    THESTR(sequence) = buffer;
	    STRLEN(sequence) = xlength;
	}
	else
	    result = LSTRING2(buffer, xlength);
    }

    /* If inplace, need to update CAR and CDR of sequence */
    else {
	LispObj *list, *object;
	LispObj **objects = LispMalloc(mac, sizeof(LispObj*) * xlength);

	if (!CONS_P(sequence))
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

		if (!CONS_P(sequence)) {
		    result = sequence;
		    CAR(result)->data.array.dim =
			SMALLINT(length - (copy - count));
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
	LispFree(mac, objects);
    }

    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Remove(LispMac *mac, LispBuiltin *builtin)
/*
 remove item sequence &key from-end test test-not start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, REMOVE, NONE));
}

LispObj *
Lisp_RemoveIf(LispMac *mac, LispBuiltin *builtin)
/*
 remove-if predicate sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, REMOVE, IF));
}

LispObj *
Lisp_RemoveIfNot(LispMac *mac, LispBuiltin *builtin)
/*
 remove-if-not predicate sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, REMOVE, IFNOT));
}

LispObj *
Lisp_Return(LispMac *mac, LispBuiltin *builtin)
/*
 return &optional result
 */
{
    unsigned blevel = mac->block.block_level;

    LispObj *result;

    result = ARGUMENT(0);

    while (blevel) {
	LispBlock *block = mac->block.block[--blevel];

	if (block->type == LispBlockClosure)
	    /* if reached a function call */
	    break;
	if (block->type == LispBlockTag && block->tag->type == LispNil_t) {
	    mac->block.block_ret = NCONSTANT_P(result) ? EVAL(result) : result;
	    LispBlockUnwind(mac, block);
	    BLOCKJUMP(block);
	}
    }
    LispDestroy(mac, "%s: no visible NIL block", STRFUN(builtin));

    /*NOTREACHED*/
    return (NIL);
}

LispObj *
Lisp_ReturnFrom(LispMac *mac, LispBuiltin *builtin)
/*
 return-from name &optional result
 */
{
    unsigned blevel = mac->block.block_level;

    LispObj *name, *result;

    result = ARGUMENT(1);
    name = ARGUMENT(0);

    if (name != NIL && name != T && !SYMBOL_P(name))
	LispDestroy(mac, "%s: %s is not a valid block name",
		    STRFUN(builtin), STROBJ(name));

    while (blevel) {
	int jmp = 1;
	LispBlock *block = mac->block.block[--blevel];

	if (name->type == block->tag->type) {
	    switch (name->type) {
		case LispNil_t:
		case LispTrue_t:
		case LispAtom_t:
		    jmp = name == block->tag;
		    break;
		default:
		    /* only atom, nil or t can be used */
		    jmp = 0;
		    break;
	    }
	    if (jmp &&
		(block->type == LispBlockTag || block->type == LispBlockClosure)) {
		mac->block.block_ret = NCONSTANT_P(result) ?
		    EVAL(result) : result;
		LispBlockUnwind(mac, block);
		BLOCKJUMP(block);
	    }
	    if (block->type == LispBlockClosure)
		/* can use return-from only in the current function */
		break;
	}
    }
    LispDestroy(mac, "%s: no visible block named %s",
		STRFUN(builtin), STROBJ(name));

    /*NOTREACHED*/
    return (NIL);
}

LispObj *
LispXReverse(LispMac *mac, LispBuiltin *builtin, int inplace)
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
    length = LispLength(mac, sequence);
    if (length <= 1)
	return (sequence);

    switch (sequence->type) {
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
		to = LispMalloc(mac, length + 1);
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
	for (list = CDR(list); CONS_P(list); list = CDR(list)) {
	    RPLACD(cons, CONS(CAR(list), NIL));
	    cons = CDR(cons);
	}
	result = LispReverse(result);

	GC_LEAVE();
    }

    if (sequence->type == LispArray_t) {
	list = result;

	result = LispNew(mac, list, NIL);
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
Lisp_Reverse(LispMac *mac, LispBuiltin *builtin)
/*
 reverse sequence
 */
{
    return (LispXReverse(mac, builtin, 0));
}

LispObj *
Lisp_Rplaca(LispMac *mac, LispBuiltin *builtin)
/*
 rplaca place value
 */
{
    LispObj *place, *value;

    value = ARGUMENT(1);
    place = ARGUMENT(0);

    ERROR_CHECK_CONS(place);
    RPLACA(place, value);

    return (place);
}

LispObj *
Lisp_Rplacd(LispMac *mac, LispBuiltin *builtin)
/*
 rplacd place value
 */
{
    LispObj *place, *value;

    value = ARGUMENT(1);
    place = ARGUMENT(0);

    ERROR_CHECK_CONS(place);
    RPLACD(place, value);

    return (place);
}

LispObj *
Lisp_Search(LispMac *mac, LispBuiltin *builtin)
/*
 search sequence1 sequence2 &key from-end test test-not key start1 start2 end1 end2
 */
{
    GC_ENTER();
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

    LispCheckSequenceStartEnd(mac, builtin, sequence1, ostart1, oend1,
			      &start1, &end1, &length1);
    LispCheckSequenceStartEnd(mac, builtin, sequence2, ostart2, oend2,
			      &start2, &end2, &length2);

    /* Check if both arguments are or aren't strings */
    if (STRING_P(sequence1) ^ STRING_P(sequence2))
	return (NIL);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT",
		    STRFUN(builtin));

    /* Check for special conditions */
    if (start1 == end1)
	return (SMALLINT(end2));
    else if (start2 == end2)
	return (start1 == end1 ? SMALLINT(start2) : NIL);

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
    istring = STRING_P(sequence1);

    if (istring) {
	string1 = THESTR(sequence1);
	string2 = THESTR(sequence2);
    }
    else {
	list1 = sequence1;
	list2 = sequence2;
	if (!CONS_P(list1))
	    list1 = list1->data.array.list;
	if (!CONS_P(list2))
	    list2 = list2->data.array.list;
    }

    length1 = end1 - start1;
    length2 = end2 - start2;

    if (from_end == NIL) {
	LispObj *slist1;
	long cstart1, cstart2;

	slist1 = list1;
	cstart2 = start2;
	while (cstart2 < end2) {
	    list1 = slist1;
	    cstart1 = start1;
	    while (cstart1 < end1) {
		if (istring) {
		    cmp1 = CHAR(string1[cstart1]);
		    cmp2 = CHAR(string2[cstart2]);
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
	    ++cstart2;
	    if (!istring)
		list2 = CDR(list2);
	}
    }
    else {
	LispObj **plist1 = NULL, **plist2 = NULL;
	long i, coff1, coff2;

	/* Allocate vector of list elements to search backwards */
	if (!istring) {
	    plist1 = LispMalloc(mac, sizeof(LispObj*) * length1);
	    plist2 = LispMalloc(mac, sizeof(LispObj*) * length2);
	    for (i = 0, coff1 = start1; coff1 < end1;
		 i++, coff1++, list1 = CDR(list1))
		plist1[i] = CAR(list1);
	    for (i = 0, coff2 = start2; coff2 < end2;
		 i++, coff2++, list2 = CDR(list2))
		plist2[i] = CAR(list2);
	}

	coff2 = end2 - 1;
	while (coff2 >= start2) {
	    coff1 = end1 - 1;
	    while (coff1 >= start1) {
		if (istring) {
		    cmp1 = CHAR(string1[coff1]);
		    cmp2 = CHAR(string2[coff2]);
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
	    --coff2;
	}

	if (!istring) {
	    LispFree(mac, plist1);
	    LispFree(mac, plist2);
	}
    }

    GC_LEAVE();

    return (offset == -1 ? NIL : SMALLINT(offset));
}

/*
 * ext::getenv
 */
LispObj *
Lisp_Setenv(LispMac *mac, LispBuiltin *builtin)
/*
 setenv name value &optional overwrite
 */
{
    char *name, *value;

    LispObj *oname, *ovalue, *overwrite;

    overwrite = ARGUMENT(2);
    ovalue = ARGUMENT(1);
    oname = ARGUMENT(0);

    ERROR_CHECK_STRING(oname);
    name = THESTR(oname);

    ERROR_CHECK_STRING(ovalue);
    value = THESTR(ovalue);

    setenv(name, value, overwrite != NIL);
    value = getenv(name);

    return (value ? STRING(value) : NIL);
}

LispObj *
Lisp_Set(LispMac *mac, LispBuiltin *builtin)
/*
 set symbol value
 */
{
    LispObj *symbol, *value;

    value = ARGUMENT(1);
    symbol = ARGUMENT(0);

    ERROR_CHECK_SYMBOL(symbol);
    ERROR_CHECK_CONSTANT(symbol);
    LispSetVar(mac, symbol, value);

    return (value);
}

LispObj *
Lisp_SetDifference(LispMac *mac, LispBuiltin *builtin)
/*
 set-difference list1 list2 &key test test-not key
 */
{
    return (LispListSet(mac, builtin, SETDIFFERENCE));
}

LispObj *
Lisp_SetExclusiveOr(LispMac *mac, LispBuiltin *builtin)
/*
 set-exclusive-or list1 list2 &key test test-not key
 */
{
    return (LispListSet(mac, builtin, SETEXCLUSIVEOR));
}

LispObj *
Lisp_SetQ(LispMac *mac, LispBuiltin *builtin)
/*
 setq &rest form
 */
{
    LispObj *result, *variable, *form;

    form = ARGUMENT(0);

    result = NIL;
    for (; CONS_P(form); form = CDR(form)) {
	variable = CAR(form);
	ERROR_CHECK_SYMBOL(variable);
	ERROR_CHECK_CONSTANT(variable);
	form = CDR(form);
	if (!CONS_P(form))
	    LispDestroy(mac, "%s: odd number of arguments", STRFUN(builtin));
	result = EVAL(CAR(form));
	LispSetVar(mac, variable, result);
    }

    return (result);
}

LispObj *
Lisp_Setf(LispMac *mac, LispBuiltin *builtin)
/*
 setf &rest form
 */
{
    LispAtom *atom;
    LispObj *place, *setf, *result = NIL;

    LispObj *form;

    form = ARGUMENT(0);

    for (; CONS_P(form); form = CDR(form)) {
	place = CAR(form);
	form = CDR(form);
	if (!CONS_P(form))
	    LispDestroy(mac, "%s: odd number of arguments", STRFUN(builtin));
	result = CAR(form);

	/* if a variable, just work like setq */
	if (SYMBOL_P(place)) {
	    ERROR_CHECK_CONSTANT(place);
	    result = EVAL(result);
	    (void)LispSetVar(mac, place, result);
	}

	else if (CONS_P(place)) {
	    int struc_access = 0;

	    /* the default setf method for structures is generated here
	     * (cannot be done in EVAL as SETF is a macro), and the
	     * code executed is as if this definition were supplied:
	     *	(defsetf THE-STRUCT-FIELD (struct) (value)
	     *		`(lisp::struct-store 'THE-STRUCT-FIELD ,struct ,value))
	     */

	    setf = CAR(place);
	    if (!SYMBOL_P(setf))
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

	    if (SYMBOL_P(setf)) {
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
		result = LispRunSetf(mac, atom->property->salist,
				     setf, place, result);
	}
	else
	    goto invalid_place;
    }

    return (result);

invalid_place:
    LispDestroy(mac, "%s: %s is an invalid place",
		STRFUN(builtin), STROBJ(place));
    /*NOTREACHED*/
    return (NIL);
}

LispObj *
Lisp_Sleep(LispMac *mac, LispBuiltin *builtin)
/*
 sleep seconds
 */
{
    long sec, msec;
    double value, dsec;

    LispObj *seconds;

    seconds = ARGUMENT(0);

    value = -1.0;
    if (FIXNUM_P(seconds))
	value = FIXNUM_VALUE(seconds);

    if (value < 0.0 || !finite(value) || value > INT_MAX)
	LispDestroy(mac, "%s: %s is not a positive fixnum",
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
LispMergeSort(LispMac *mac, LispObj *list, LispObj *predicate,
	      LispObj *key, int code)
{
    int protect;
    LispObj *list1, *list2, *left, *right, *result, *cons;

    /* Check if list length is larger than 1 */
    if (!CONS_P(list) || !CONS_P(CDR(list)))
	return (list);

    list1 = list2 = list;
    while (CONS_P(list = CDR(list)) && CONS_P(list = CDR(list)))
	list2 = CDR(list2);
    cons = list2;
    list2 = CDR(list2);
    RPLACD(cons, NIL);

    protect = 0;
    if (mac->protect.length + 2 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = list2;
    list1 = LispMergeSort(mac, list1, predicate, key, code);
    list2 = LispMergeSort(mac, list2, predicate, key, code);

    left = CAR(list1);
    right = CAR(list2);
    if (key != NIL) {
	protect = mac->protect.length;
	left = APPLY1(key, left);
	mac->protect.objects[protect] = left;
	right = APPLY1(key, right);
	mac->protect.objects[protect + 1] = right;
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
	    if (!CONS_P(list2)) {
		RPLACD(cons, list1);
		break;
	    }
	    right = CAR(list2);
	    if (key != NIL) {
		right = APPLY1(key, right);
		mac->protect.objects[protect + 1] = right;
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
	    if (!CONS_P(list1)) {
		RPLACD(cons, list2);
		break;
	    }
	    left = CAR(list1);
	    if (key != NIL) {
		left = APPLY1(key, left);
		mac->protect.objects[protect] = left;
	    }
	}
    }
    if (key != NIL)
	mac->protect.length = protect;

    return (result);
}

/* XXX The first version made a copy of the list and then adjusted
 *     the CARs of the list. To minimize GC time now it is now doing
 *     the sort inplace. So, instead of writing just (sort variable)
 *     now it is required to write (setq variable (sort variable))
 *     if the variable should always keep all elements.
 */
LispObj *
Lisp_Sort(LispMac *mac, LispBuiltin *builtin)
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

    length = LispLength(mac, sequence);
    if (length < 2)
	return (sequence);

    list = sequence;
    istring = STRING_P(sequence);
    if (istring) {
	/* Convert string to list */
	string = THESTR(sequence);
	work = cons = CONS(CHAR(string[0]), NIL);
	GC_PROTECT(work);
	for (++string; *string; ++string) {
	    RPLACD(cons, CONS(CHAR(*string), NIL));
	    cons = CDR(cons);
	}
    }
    else if (!CONS_P(list))
	work = list->data.array.list;
    else
	work = list;

    code = FCODE(predicate);
    work = LispMergeSort(mac, work, predicate, key, code);

    if (istring) {
	/* Convert list to string */
	string = THESTR(sequence);
	for (; CONS_P(work); ++string, work = CDR(work))
	    *string = CAR(work)->data.integer;
    }
    else if (!CONS_P(list))
	list->data.array.list = work;
    else
	sequence = work;
    GC_LEAVE();

    return (sequence);
}

LispObj *
Lisp_Subseq(LispMac *mac, LispBuiltin *builtin)
/*
 subseq sequence start &optional end
 */
{
    long start, end, length, seqlength;

    LispObj *sequence, *ostart, *oend, *result;

    oend = ARGUMENT(2);
    ostart = ARGUMENT(1);
    sequence = ARGUMENT(0);

    LispCheckSequenceStartEnd(mac, builtin, sequence, ostart, oend,
			      &start, &end, &length);

    seqlength = end - start;

    if (sequence == NIL)
	result = NIL;
    else if (STRING_P(sequence)) {
	char *string = LispMalloc(mac, seqlength + 1);

	strncpy(string, THESTR(sequence) + start, seqlength);
	string[seqlength] = '\0';
	result = STRING2(string);
    }
    else {
	LispObj *object;

	GCProtect();
	if (end > start) {
	    /* list or array */
	    int count;
	    LispObj *cons;

	    if (sequence->type == LispArray_t)
		object = sequence->data.array.list;
	    else
		object = sequence;
	    /* goto first element to copy */
	    for (count = 0; count < start; count++, object = CDR(object))
		;
	    result = cons = CONS(CAR(object), NIL);
	    for (++count, object = CDR(object); count < end; count++,
		 object = CDR(object)) {
		RPLACD(cons, CONS(CAR(object), NIL));
		cons = CDR(cons);
	    }
	}
	else
	    result = NIL;

	if (sequence->type == LispArray_t) {
	    object = LispNew(mac, result, NIL);
	    object->type = LispArray_t;
	    object->data.array.list = result;
	    object->data.array.dim = CONS(SMALLINT(seqlength), NIL);
	    object->data.array.rank = 1;
	    object->data.array.type = sequence->data.array.type;
	    object->data.array.zero = length == 0;
	    result = object;
	}
	GCUProtect();
    }

    return (result);
}

LispObj *
Lisp_Subsetp(LispMac *mac, LispBuiltin *builtin)
/*
 subsetp list1 list2 &key test test-not key
 */
{
    return (LispListSet(mac, builtin, SUBSETP));
}


LispObj *
Lisp_Substitute(LispMac *mac, LispBuiltin *builtin)
/*
 substitute newitem olditem sequence &key from-end test test-not start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, SUBSTITUTE, NONE));
}

LispObj *
Lisp_SubstituteIf(LispMac *mac, LispBuiltin *builtin)
/*
 substitute-if newitem test sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, SUBSTITUTE, IF));
}

LispObj *
Lisp_SubstituteIfNot(LispMac *mac, LispBuiltin *builtin)
/*
 substitute-if-not newitem test sequence &key from-end start end count key
 */
{
    return (LispDeleteRemoveXSubstitute(mac, builtin, SUBSTITUTE, IFNOT));
}

LispObj *
Lisp_Symbolp(LispMac *mac, LispBuiltin *builtin)
/*
 symbolp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    switch (object->type) {
	case LispNil_t:
	case LispTrue_t:
	case LispAtom_t:
	case LispLambda_t:
	    return (T);
	default:
	    break;
    }
    return (NIL);
}

LispObj *
Lisp_SymbolPlist(LispMac *mac, LispBuiltin *builtin)
/*
 symbol-plist symbol
 */
{
    LispObj *symbol;

    symbol = ARGUMENT(0);

    if (!SYMBOL_P(symbol)) {
	if (symbol == NIL)
	    symbol = ATOM(Snil);
	else if (symbol == T)
	    symbol = ATOM(St);
	else
	    LispDestroy(mac, "%s: %s is not a symbol",
			STRFUN(builtin), STROBJ(symbol));
    }

    return (symbol->data.atom->a_property ?
	    symbol->data.atom->property->properties : NIL);
}

LispObj *
Lisp_Tagbody(LispMac *mac, LispBuiltin *builtin)
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
    stack = mac->stack.length;
    lex = mac->env.lex;
    length = mac->env.length;

    /* Since the body may be large, and the code may iterate several
     * thousand times, it is not a bad idea to avoid checking all
     * elements of the body to verify if it is a tag. */
    for (labels = map = NIL, ptr = body; CONS_P(ptr); ptr = CDR(ptr)) {
	tag = CAR(ptr);
	switch (tag->type) {
	    case LispNil_t:
	    case LispTrue_t:
	    case LispAtom_t:
	    case LispInteger_t:
		/* Don't allow duplicated labels */
		for (list = labels; CONS_P(list); list = CDDR(list)) {
		    if (CAR(list) == tag)
			LispDestroy(mac, "%s: tag %s specified more than once",
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
    for (ptr = labels; CONS_P(ptr); ptr = CDDR(ptr)) {
	if (CADR(ptr) == NIL) {
	    for (map = CDDR(ptr); CONS_P(map); map = CDDR(map)) {
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
    block = LispBeginBlock(mac, NIL, LispBlockBody);

    /* Loop */
    if (setjmp(block->jmp) != 0) {
	/* Restore environment */
	mac->stack.length = stack;
	mac->env.lex = lex;
	mac->env.head = mac->env.length = length;

	tag = mac->block.block_ret;
	for (ptr = labels; CONS_P(ptr); ptr = CDDR(ptr)) {
	    map = CAR(ptr);
	    if (map == tag ||
		(tag->type == LispInteger_t &&
		 map->type == LispInteger_t &&
		 tag->data.integer == map->data.integer))
		break;
	}

	if (!CONS_P(ptr))
	    LispDestroy(mac, "%s: no such tag %s", STRFUN(builtin), STROBJ(tag));

	*p_body = CADR(ptr);
    }

    /* Execute code */
    for (; CONS_P(body); body = CDR(body)) {
	LispObj *form = CAR(body);

	if (CONS_P(form))
	    EVAL(form);
    }
    /* If got here, (go) not called, else, labels will be candidate to gc
     * when GC_LEAVE() be called by the code in the bottom of the stack. */
    GC_LEAVE();

    /* Finished */
    LispEndBlock(mac, block);

    /* Always return NIL */
    return (NIL);
}

LispObj *
Lisp_Terpri(LispMac *mac, LispBuiltin *builtin)
/*
 terpri &optional output-stream
 */
{
    LispObj *output_stream;

    output_stream = ARGUMENT(0);

    if (output_stream != NIL && !STREAM_P(output_stream))
	LispDestroy(mac, "%s: %s is not a stream",
		    STRFUN(builtin), STROBJ(output_stream));

    LispWriteChar(mac, output_stream, '\n');
    if (output_stream == NIL ||
	(output_stream->data.stream.type == LispStreamStandard &&
	 output_stream->data.stream.source.file == Stdout))
	LispFflush(Stdout);

    return (NIL);
}

LispObj *
Lisp_The(LispMac *mac, LispBuiltin *builtin)
/*
 the value-type form
 */
{
    LispObj *value_type, *form;

    form = ARGUMENT(1);
    value_type = ARGUMENT(0);

    if (NCONSTANT_P(form))
	form = EVAL(form);

    return (LispCoerce(mac, builtin, form, value_type));
}

LispObj *
Lisp_Throw(LispMac *mac, LispBuiltin *builtin)
/*
 throw tag result
 */
{
    unsigned blevel = mac->block.block_level;

    LispObj *tag, *result;

    result = ARGUMENT(1);
    tag = ARGUMENT(0);

    if (NCONSTANT_P(tag))
	tag = EVAL(tag);

    if (blevel == 0)
	LispDestroy(mac, "%s: not within a block",
		    STRFUN(builtin));

    while (blevel) {
	LispBlock *block = mac->block.block[--blevel];

	if (block->type == LispBlockCatch && tag->type == block->tag->type) {
	    if (XEQ(tag, block->tag) != NIL) {
		mac->block.block_ret = NCONSTANT_P(result) ? EVAL(result) : result;
		LispBlockUnwind(mac, block);
		BLOCKJUMP(block);
	    }
	}
    }
    LispDestroy(mac, "%s: %s is not a valid tag",
		STRFUN(builtin), STROBJ(tag));

    /*NOTREACHED*/
    return (NIL);
}

LispObj *
Lisp_Typep(LispMac *mac, LispBuiltin *builtin)
/*
 typep object type
 */
{
    LispObj *result = NULL;

    LispObj *object, *type;

    type = ARGUMENT(1);
    object = ARGUMENT(0);

    if (SYMBOL_P(type)) {
	Atom_id atom = ATOMID(type);

	if (object->type == LispStruct_t)
	    result = ATOMID(CAR(object->data.struc.def)) == atom ? T : NIL;
	else if (type->data.atom->a_defstruct &&
		 type->data.atom->property->structure.function == STRUCT_NAME)
	    result = NIL;
	else if (atom == Snil)
	    result = object == NIL ? T : NIL;
	else if (atom == St)
	    result = object == T ? T : NIL;
	else if (atom == Satom)
	    result = !CONS_P(object) ? T : NIL;
	else if (atom == Ssymbol)
	    result = SYMBOL_P(object) || KEYWORD_P(object) ||
		     object == NIL || object == T ? T : NIL;
	else if (atom == Sreal)
	    result = FLOAT_P(object) ? T : NIL;
	else if (atom == Sinteger)
	    result = INTEGER_P(object) ? T : NIL;
	else if (atom == Srational)
	    result = RATIONAL_P(object) ? T : NIL;
	else if (atom == Scons || atom == Slist)
	    result = CONS_P(object) ? T : NIL;
	else if (atom == Sstring)
	    result = STRING_P(object) ? T : NIL;
	else if (atom == Scharacter)
	    result = CHAR_P(object) ? T : NIL;
	else if (atom == Scomplex)
	    result = COMPLEX_P(object) ? T : NIL;
	else if (atom == Svector || atom == Sarray)
	    result = object->type == LispArray_t ? T : NIL;
	else if (atom == Skeyword)
	    result = KEYWORD_P(object) ? T : NIL;
	else if (atom == Sfunction)
	    result = object->type == LispLambda_t ? T : NIL;
	else if (atom == Spathname)
	    result = PATHNAME_P(object) ? T : NIL;
	else if (atom == Sopaque)
	    result = object->type == LispOpaque_t ? T : NIL;
    }
    else if (CONS_P(type)) {
	if (object->type == LispStruct_t &&
	    SYMBOL_P(CAR(type)) && ATOMID(CAR(type)) == Sstruct &&
	    SYMBOL_P(CAR(CDR(type))) && CDR(CDR(type)) == NIL) {
	    result = ATOMID(CAR(object->data.struc.def)) ==
		     ATOMID(CAR(CDR(type))) ? T : NIL;
	}
    }
    else if (type == NIL)
	result = object == NIL ? T : NIL;
    else if (type == T)
	result = object == T ? T : NIL;
    if (result == NULL)
	LispDestroy(mac, "%s: bad type specification %s",
		    STRFUN(builtin), STROBJ(type));

    return (result);
}

LispObj *
Lisp_Union(LispMac *mac, LispBuiltin *builtin)
/*
 union list1 list2 &key test test-not key
 */
{
    return (LispListSet(mac, builtin, UNION));
}

LispObj *
Lisp_Unless(LispMac *mac, LispBuiltin *builtin)
/*
 unless test &rest body
 */
{
    LispObj *result, *test, *body;

    body = ARGUMENT(1);
    test = ARGUMENT(0);

    result = NIL;
    if (NCONSTANT_P(test))
	test = EVAL(test);
    if (test == NIL) {
	for (; CONS_P(body); body = CDR(body))
	    if (NCONSTANT_P(result = CAR(body)))
		result = EVAL(result);
    }

    return (result);
}

/*
 * ext::until
 */
LispObj *
Lisp_Until(LispMac *mac, LispBuiltin *builtin)
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
	    for (prog = body; CONS_P(prog); prog = CDR(prog))
		(void)EVAL(CAR(prog));
	}
	else
	    break;
    }

    return (result);
}

LispObj *
Lisp_UnwindProtect(LispMac *mac, LispBuiltin *builtin)
/*
 unwind-protect protect cleanup
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
    block = LispBeginBlock(mac, NIL, LispBlockProtect);
    if (setjmp(block->jmp) == 0) {
	*presult = EVAL(protect);
	*pdid_jump = 0;
    }
    LispEndBlock(mac, block);
    if (!mac->destroyed && *pdid_jump)
	*presult = mac->block.block_ret;

    /* run cleanup, unprotected code */
    if (CONS_P(*pcleanup))
	for (; CONS_P(cleanup); cleanup = CDR(cleanup))
	    (void)EVAL(CAR(cleanup));
    else if (mac->destroyed)
	/* no cleanup code */
	LispDestroy(mac, NULL);	/* special handling if mac->destroyed */

    return (result);
}

LispObj *
Lisp_Vector(LispMac *mac, LispBuiltin *builtin)
/*
 vector &rest objects
 */
{
    LispObj *objects;

    objects = ARGUMENT(0);

    return (VECTOR(objects));
}

LispObj *
Lisp_When(LispMac *mac, LispBuiltin *builtin)
/*
 when test &rest body
 */
{
    LispObj *result, *test, *body;

    body = ARGUMENT(1);
    test = ARGUMENT(0);

    result = NIL;
    if (NCONSTANT_P(test))
	test = EVAL(test);
    if (test != NIL) {
	for (; CONS_P(body); body = CDR(body))
	    if (NCONSTANT_P(result = CAR(body)))
		result = EVAL(result);
    }

    return (result);
}

/*
 * ext::while
 */
LispObj *
Lisp_While(LispMac *mac, LispBuiltin *builtin)
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
	    for (prog = body; CONS_P(prog); prog = CDR(prog))
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
Lisp_Unsetenv(LispMac *mac, LispBuiltin *builtin)
/*
 unsetenv name
 */
{
    char *name;

    LispObj *oname;

    oname = ARGUMENT(0);

    if (!STRING_P(oname))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(oname));
    name = THESTR(oname);

    unsetenv(name);

    return (NIL);
}

LispObj *
Lisp_XeditEltStore(LispMac *mac, LispBuiltin *builtin)
/*
 lisp::elt-store sequence index value
 */
{
    int length, offset;

    LispObj *sequence, *oindex, *value;

    value = ARGUMENT(2);
    oindex = ARGUMENT(1);
    sequence = ARGUMENT(0);

    ERROR_CHECK_INDEX(oindex);
    offset = oindex->data.integer;
    length = LispLength(mac, sequence);

    if (offset >= length)
	LispDestroy(mac, "%s: index %d too large for sequence length %d",
		    STRFUN(builtin), offset, length);

    if (STRING_P(sequence)) {
	int character;

	ERROR_CHECK_CHARACTER(value);
	character = value->data.integer;
	if (character < 0 || character > 255)
	    LispDestroy(mac, "%s: cannot represent character %d",
			STRFUN(builtin), character);

	THESTR(sequence)[offset] = character;
    }
    else {
	if (sequence->type == LispArray_t)
	    sequence = sequence->data.array.list;

	for (; offset > 0; offset--, sequence = CDR(sequence))
	    ;
	RPLACA(sequence, value);
    }

    return (value);
}

LispObj *
Lisp_XeditPut(LispMac *mac, LispBuiltin *builtin)
/*
 lisp::put symbol indicator value
 */
{
    LispObj *symbol, *indicator, *value;

    value = ARGUMENT(2);
    indicator = ARGUMENT(1);
    symbol = ARGUMENT(0);

    if (!SYMBOL_P(symbol)) {
	if (symbol == NIL)
	    symbol = ATOM(Snil);
	else if (symbol == T)
	    symbol = ATOM(St);
	else
	    LispDestroy(mac, "%s: %s is not a symbol",
			STRFUN(builtin), STROBJ(symbol));
    }

    return (CAR(LispPutAtomProperty(mac, symbol->data.atom, indicator, value)));
}

LispObj *
Lisp_XeditVectorStore(LispMac *mac, LispBuiltin *builtin)
/*
 lisp::vector-store array subscripts value
 */
{
    long count, sequence, offset, accum;
    LispObj *list, *object;

    LispObj *array, *subscripts, *value;

    value = ARGUMENT(2);
    subscripts = ARGUMENT(1);
    array = ARGUMENT(0);

    if (STRING_P(array) && CONS_P(subscripts) &&
	INDEX_P(CAR(subscripts)) && CDR(subscripts) == NIL &&
	CHAR_P(value)) {
	long length = STRLEN(array);
	long offset = CAR(subscripts)->data.integer;

	if (offset >= length)
	    LispDestroy(mac, "%s: index %ld too large for sequence length %ld",
			STRFUN(builtin), offset, length);

	if (value->data.integer < 0 || value->data.integer > 255)
	    LispDestroy(mac, "%s: cannot represent character %ld",
			STRFUN(builtin), value->data.integer);

	return (CHAR(*(unsigned char*)(THESTR(array) + offset) =
		value->data.integer));
    }

    if (array->type != LispArray_t)
	LispDestroy(mac, "%s: %s is not an array",
		    STRFUN(builtin), STROBJ(array));

    for (count = 0, list = subscripts, object = array->data.array.dim;
	 CONS_P(list); count++, list = CDR(list), object = CDR(object)) {
	if (count >= array->data.array.rank)
	    LispDestroy(mac, "%s: too many subscripts %s",
			STRFUN(builtin), STROBJ(subscripts));
	if (!INDEX_P(CAR(list)))
	    LispDestroy(mac, "%s: %s is not a positive integer",
			STRFUN(builtin), STROBJ(CAR(list)));
	if (CAR(list)->data.integer >= CAR(object)->data.integer)
	    LispDestroy(mac, "%s: %ld is out of range, index %ld",
			STRFUN(builtin), CAR(list)->data.integer,
			CAR(object)->data.integer);
    }
    if (count < array->data.array.rank)
	LispDestroy(mac, "%s: too few subscripts %s",
		    STRFUN(builtin), STROBJ(subscripts));

    for (count = sequence = 0, list = subscripts; CONS_P(list);
	 list = CDR(list), sequence++) {
	for (offset = 0, object = array->data.array.dim;
	     offset < sequence; object = CDR(object), offset++)
	    ;
	for (accum = 1, object = CDR(object); CONS_P(object);
	     object = CDR(object))
	    accum *= CAR(object)->data.integer;
	count += accum * CAR(list)->data.integer;
    }

    for (array = array->data.array.list; count > 0; array = CDR(array), count--)
	;

    RPLACA(array, value);

    return (value);
}

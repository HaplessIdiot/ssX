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

/* $XFree86: xc/programs/xedit/lisp/core.c,v 1.39 2002/05/17 20:24:11 paulo Exp $ */

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
LispObj *LispMemberIf(LispMac*, LispBuiltin*, int);
LispObj *LispRemove_IfNot(LispMac*, LispBuiltin*, int, int);
extern LispObj *LispRunSetf(LispMac*, LispArgList*, LispObj*, LispObj*, LispObj*);

/*
 * Initialization
 */
LispObj *Oequal, *Omake_array, *Kinitial_contents, *Osetf;

Atom_id Sequal, Sotherwise, Svariable, Sstructure, Stype, Ssetf;

/*
 * Implementation
 */
void
LispCoreInit(LispMac *mac)
{
    Oequal		= STATIC_ATOM("EQUAL");
    Omake_array		= STATIC_ATOM("MAKE-ARRAY");
    Kinitial_contents	= KEYWORD("INITIAL-CONTENTS");
    Osetf		= STATIC_ATOM("SETF");

    Sotherwise		= GETATOMID("OTHERWISE");
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
		    CDR(cons) = CONS(CAR(obj), CDR(obj));
		    cons = CDR(cons);
		    obj = CDR(obj);
		}
		CDR(cons) = CONS(CADR(cons), list);
	    }
	    else
		CDR(cons) = list;
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
	long length = strlen(THESTR(array));
	long offset = CAR(subscripts)->data.integer;

	if (offset >= length)
	    LispDestroy(mac, "%s: index %d too large for sequence length %d",
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
	    c *= FIXNUM_VALUE(CAR(obj));
	count += c * FIXNUM_VALUE(CAR(dim));
    }

    for (array = array->data.array.list; count > 0; array = CDR(array), count--)
	;

    return (CAR(array));
}

/* XXX the &KEY parameters are being ignored */
LispObj *
Lisp_Assoc(LispMac *mac, LispBuiltin *builtin)
/*
 assoc item list &key test test-not key
 */
{
    LispObj *cmp;
    LispObj *item, *list, *test, *test_not, *key;

    key = ARGUMENT(4);
    test_not = ARGUMENT(3);
    test = ARGUMENT(2);
    list = ARGUMENT(1);
    item = ARGUMENT(0);

    if (list == NIL)
	return (NIL);

    for (; CONS_P(list); list = CDR(list)) {
	if (!CONS_P(cmp = CAR(list)))
	    LispDestroy(mac, "%s: %s is a bad argument",
			STRFUN(builtin), STROBJ(list));
	if (LispEqual(mac, item, CAR(cmp)) == T)
	    return (cmp);
    }

    return (NIL);
}

LispObj *
Lisp_And(LispMac *mac, LispBuiltin *builtin)
/*
 and &rest args
 */
{
    LispObj *result = T, *args;

    args = ARGUMENT(0);
    MACRO_ARGUMENT1();

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

    if (function->type != LispLambda_t && !SYMBOL_P(function))
	LispDestroy(mac, "%s: %s is not a valid function name",
		    STRFUN(builtin), STROBJ(function));
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
		CDR(cons) = CONS(CAR(more_args), NIL);
		cons = CDR(cons);
	    }
	    else {
		arg = CDR(cons) = CAR(more_args);
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
    MACRO_ARGUMENT2();

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
Lisp_Butlast(LispMac *mac, LispBuiltin *builtin)
/*
 butlast list &optional (count 1) &aux (length (length list))
 */
{
    GC_ENTER();
    long length, count;
    LispObj *result, *cons, *list, *ocount, *olength;

    cons = NIL;		/* fix gcc warning */

    olength = ARGUMENT(2);
    ocount = ARGUMENT(1);
    list = ARGUMENT(0);

    if (list == NIL)
	return (NIL);
    ERROR_CHECK_LIST(list);
    ERROR_CHECK_INDEX(ocount);
    count = ocount->data.integer;
    length = olength->data.integer;

    if (count == 0)
	return (list);
    else if (count >= length)
	return (NIL);

    result = NIL;
    length -= count;
    for (; length > 0; list = CDR(list), length--) {
	if (result == NIL) {
	    result = cons = CONS(CAR(list), NIL);
	    GC_PROTECT(result);
	}
	else {
	    CDR(cons) = CONS(CAR(list), NIL);
	    cons = CDR(cons);
	}
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
    LispObj *result, *code, *keyform, *body;

    body = ARGUMENT(1);
    keyform = ARGUMENT(0);
    MACRO_ARGUMENT2();

    result = NIL;
    if (NCONSTANT_P(keyform))
	keyform = EVAL(keyform);

    for (; CONS_P(body); body = CDR(body)) {
	code = CAR(body);
	ERROR_CHECK_LIST(code);
	else if (CAR(code) == T ||
		 (SYMBOL_P(CAR(code)) && ATOMID(CAR(code)) == Sotherwise)) {
	    if (CONS_P(CDR(body)))
		LispDestroy(mac, "%s: %s must be the last clause",
			    STRFUN(builtin), STROBJ(CAR(code)));
	    result = CDR(code);
	    break;
	}
	else if (CONS_P(CAR(code))) {
	    LispObj *keylist = CAR(code);

	    for (; CONS_P(keylist); keylist = CDR(keylist))
		if (LispEqual(mac, keyform, CAR(keylist)) == T) {
		    result = CDR(code);
		    break;
		}
	    if (CONS_P(keylist))	/* if found match */
		break;
	}
	else if (LispEqual(mac, keyform, CAR(code)) == T) {
	    result = CDR(code);
	    break;
	}
    }

    if (CONS_P(result)) {
	for (body = result; CONS_P(body); body = CDR(body))
	    if (NCONSTANT_P(result = CAR(body)))
		result = EVAL(result);
    }

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
    MACRO_ARGUMENT2();

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
Lisp_Cond(LispMac *mac, LispBuiltin *builtin)
/*
 cond &rest body
 */
{
    LispObj *result, *code, *body;

    body = ARGUMENT(0);
    MACRO_ARGUMENT1();

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
Lisp_Defconstant(LispMac *mac, LispBuiltin *builtin)
/*
 defconstant name initial-value &optional documentation
 */
{
    LispObj *name, *initial_value, *documentation;

    documentation = ARGUMENT(2);
    initial_value = ARGUMENT(1);
    name = ARGUMENT(0);
    MACRO_ARGUMENT3();

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
    MACRO_ARGUMENT3();

    if (!SYMBOL_P(name))
	LispDestroy(mac, "%s: bad MACRO name %s",
		    STRFUN(builtin), STROBJ(name));

    alist = LispCheckArguments(mac, LispMacro, lambda_list, STRPTR(name));

    if (CONS_P(body) && STRING_P(CAR(body))) {
	LispAddDocumentation(mac, name, CAR(body), LispDocFunction);
	body = CDR(body);
    }

    GCProtect();
    lambda = LispNewLambda(mac, name, lambda_list, body, LispMacro);
    GCUProtect();

    if (name->data.atom->a_builtin) {
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
    MACRO_ARGUMENT3();

    if (!SYMBOL_P(name))
	LispDestroy(mac, "%s: bad FUNCTION name %s",
		    STRFUN(builtin), STROBJ(name));

    alist = LispCheckArguments(mac, LispFunction, lambda_list, STRPTR(name));

    if (CONS_P(body) && STRING_P(CAR(body))) {
	LispAddDocumentation(mac, name, CAR(body), LispDocFunction);
	body = CDR(body);
    }

    GCProtect();
    lambda = LispNewLambda(mac, name, lambda_list, body, LispFunction);
    GCUProtect();

    if (name->data.atom->a_builtin) {
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
    MACRO_ARGUMENT3();

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

    lambda_list = CONS(lambda_list, store);
    body = CDR(body);
    if (CONS_P(body) && STRING_P(CAR(body))) {
	LispAddDocumentation(mac, function, CAR(body), LispDocSetf);
	body = CDR(body);
    }

    GCProtect();
    lambda = LispNewLambda(mac, function, lambda_list, body, LispSetf);
    GCUProtect();
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
    MACRO_ARGUMENT3();

    ERROR_CHECK_SYMBOL(name);
    if (documentation != NIL) {
	ERROR_CHECK_STRING(documentation);
    }

    LispProclaimSpecial(mac, name, bound == T ? EVAL(initial_value) : NULL,
			documentation);

    return (name);
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
 elt sequence index &aux (length (length sequence))
 svref sequence index &aux (length (length sequence))
 */
{
    long offset, length;
    LispObj *result, *sequence, *oindex, *olength;

    olength = ARGUMENT(2);
    oindex = ARGUMENT(1);
    sequence = ARGUMENT(0);

    ERROR_CHECK_INDEX(oindex);
    offset = oindex->data.integer;
    length = olength->data.integer;

    if (offset >= length)
	LispDestroy(mac, "%s: index %d too large for sequence length %d",
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
    LispObj *left, *right, *result;

    right = ARGUMENT(1);
    left = ARGUMENT(0);

    result = left == right ? T : NIL;

    return (result);
}

LispObj *
Lisp_Eql(LispMac *mac, LispBuiltin *builtin)
/*
 eql left right
 */
{
    LispObj *left, *right, *result;

    right = ARGUMENT(1);
    left = ARGUMENT(0);

    result = NIL;

    if (left->type == right->type)
	switch (left->type) {
	    case LispAtom_t:
	    case LispReal_t:
	    case LispCharacter_t:
	    case LispInteger_t:
	    case LispRatio_t:
	    case LispBigInteger_t:
	    case LispBigRatio_t:
	    case LispComplex_t:
		result = LispEqual(mac, left, right);
		break;
	    default:
		if (left == right)
		    result = T;
		break;
	}

    return (result);
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

    return (LispEqual(mac, left, right));
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
    MACRO_ARGUMENT2();

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
	    CAR(list) = item;
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
    MACRO_ARGUMENT1();

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
    MACRO_ARGUMENT3();

    test = EVAL(test);
    if (test != NIL)
	result = EVAL(then);
    else
	result = EVAL(oelse);

    return (result);
}

LispObj *
Lisp_Keywordp(LispMac *mac, LispBuiltin *builtin)
/*
 keywordp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (KEYWORD_P(object) ? T : NIL);
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
    MACRO_ARGUMENT2();

    alist = LispCheckArguments(mac, LispLambda, lambda_list, Slambda);

    GCProtect();
    lambda = LispNewLambda(mac, OPAQUE(alist, LispArgList_t),
			   lambda_list, body, LispLambda);
    GCUProtect();
    LispUseArgList(mac, alist);

    return (lambda);
}

LispObj *
Lisp_Last(LispMac *mac, LispBuiltin *builtin)
/*
 last list &optional (count 1) &aux (length (length list))
 */
{
    long count, length;
    LispObj *list, *ocount, *olength;

    olength = ARGUMENT(2);
    ocount = ARGUMENT(1);
    list = ARGUMENT(0);

    ERROR_CHECK_INDEX(ocount);
    count = FIXNUM_VALUE(ocount);
    length = FIXNUM_VALUE(olength);

    if (list == NIL)
	return (NIL);
    else if (count >= length)
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
    LispObj *init, *body, *pair, *result, *list, *cons;

    body = ARGUMENT(1);
    init = ARGUMENT(0);
    MACRO_ARGUMENT2();

    cons = NIL;	/* fix gcc warning */

    if (init != NIL) {
	ERROR_CHECK_LIST(init);
    }
    list = NIL;
    for (; CONS_P(init); init = CDR(init)) {
	LispObj *var, *val;

	var = val = NIL;
	pair = CAR(init);
	if (SYMBOL_P(pair)) {
	    var = pair;
	    val = NIL;
	}
	else ERROR_CHECK_LIST(pair);
	else {
	    var = CAR(pair);
	    if (!SYMBOL_P(var))
		LispDestroy(mac, "%s: %s cannot be a variable name",
			    STRFUN(builtin), STROBJ(var));
	    pair = CDR(pair);
	    if (CONS_P(pair)) {
		val = CAR(pair);
		if (CDR(pair) != NIL)
		    LispDestroy(mac, "%s: too much arguments to initialize %s",
				STRFUN(builtin), STROBJ(var));
		if (NCONSTANT_P(val))
		    val = EVAL(val);
	    }
	    else
		val = NIL;
	}

	pair = CONS(var, val);
	if (list == NIL) {
	    list = cons = CONS(pair, NIL);
	    GC_PROTECT(list);
	}
	else {
	    CDR(cons) = CONS(pair, NIL);
	    cons = CDR(cons);
	}
    }

    /* add variables */
    for (; CONS_P(list); list = CDR(list)) {
	pair = CAR(list);
	ERROR_CHECK_CONSTANT(CAR(pair));
	LispAddVar(mac, CAR(pair), CDR(pair));
	++mac->env.head;
    }

    GC_LEAVE();

    result = NIL;

    /* execute body */
    for (; CONS_P(body); body = CDR(body))
	if (NCONSTANT_P(result = CAR(body)))
	    result = EVAL(result);

    return (result);
}

LispObj *
Lisp_LetP(LispMac *mac, LispBuiltin *builtin)
/*
 let* init &rest body
 */
{
    LispObj *init, *body, *pair, *result;

    body = ARGUMENT(1);
    init = ARGUMENT(0);
    MACRO_ARGUMENT2();

    if (init != NIL) {
	ERROR_CHECK_LIST(init);
    }

    for (; CONS_P(init); init = CDR(init)) {
	LispObj *var, *val;

	var = val = NIL;
	pair = CAR(init);
	if (SYMBOL_P(pair)) {
	    var = pair;
	    val = NIL;
	}
	else ERROR_CHECK_LIST(pair);
	else {
	    var = CAR(pair);
	    if (!SYMBOL_P(var))
		LispDestroy(mac, "%s: %s cannot be a variable name",
			    STRFUN(builtin), STROBJ(var));
	    pair = CDR(pair);
	    if (CONS_P(pair)) {
		val = CAR(pair);
		if (CDR(pair) != NIL)
		    LispDestroy(mac, "%s: too much arguments to initialize %s",
				STRFUN(builtin), STROBJ(var));
		if (NCONSTANT_P(val))
		    val = EVAL(val);
	    }
	    else
		val = NIL;
	}

	ERROR_CHECK_CONSTANT(var);
	LispAddVar(mac, var, val);
	++mac->env.head;
    }

    result = NIL;

    /* execute body */
    for (; CONS_P(body); body = CDR(body))
	if (NCONSTANT_P(result = CAR(body)))
	    result = EVAL(result);

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
	CDR(cons) = CONS(CDR(cons), object);
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

    return (CONS_P(object) ? T : NIL);
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
    MACRO_ARGUMENT1();

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
	count = FIXNUM_VALUE(dimensions);
    }
    else if (CONS_P(dimensions)) {
	dim = dimensions;

	for (rank = 0; CONS_P(dim); rank++, dim = CDR(dim)) {
	    obj = CAR(dim);
	    if (!INDEX_P(obj))
		LispDestroy(mac, "%s: %s is a bad array dimension",
			    STRFUN(builtin), STROBJ(dimensions));
	    count *= FIXNUM_VALUE(obj);
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
	offset = (int)FIXNUM_VALUE(displaced_index_offset);
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
	    c *= (long)FIXNUM_VALUE(CAR(obj));
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
		dims[i] = (int)FIXNUM_VALUE(CAR(obj));

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
			CDR(array) = CONS(CAR(array), CDR(array));
			CAR(array) = CAR(obj);
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
		CDR(array) = CONS(CAR(array), CDR(array));
		CAR(array) = initial_element;
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
    long count;
    LispObj *result, *tail;

    LispObj *size, *initial_element;

    initial_element = ARGUMENT(1);
    size = ARGUMENT(0);

    ERROR_CHECK_INDEX(size);
    count = size->data.integer;

    GCProtect();
    result = tail = CONS(initial_element, initial_element);
    for (; count > 1; count--)
	result = CONS(initial_element, result);
    CDR(tail) = NIL;
    GCUProtect();

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
    int i, count, length;
    LispObj *code, *cons, *acons, *object, *result;

    LispObj *function, *list, *more_lists;

    more_lists = ARGUMENT(2);
    list = ARGUMENT(1);
    function = ARGUMENT(0);

    if (!SYMBOL_P(function) && function->type != LispLambda_t)
	LispDestroy(mac, "%s: %s is a bad function",
		    STRFUN(builtin), STROBJ(function));

    length = mac->protect.length;
    if (length + 2 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = NIL;
    mac->protect.objects[mac->protect.length++] = NIL;

    result = acons = NIL;
    for (count = 0; CONS_P(list); count++, list = CDR(list)) {
	cons = CONS(QUOTE(CAR(list)), NIL);
	code = CONS(function, cons);
	mac->protect.objects[length] = code;
	for (object = more_lists; CONS_P(object); object = CDR(object)) {
	    LispObj *arglist = CAR(object);

	    if (!CONS_P(arglist))
		goto mapcar_done;
	    for (i = count; i > 0; i--) {
		if (!CONS_P(arglist = CDR(arglist)))
		    goto mapcar_done;
	    }
	    CDR(cons) = CONS(QUOTE(CAR(arglist)), NIL);
	}
	cons = EVAL(code);
	if (result == NIL) {
	    result = acons = CONS(cons, NIL);
	    mac->protect.objects[length + 1] = result;
	}
	else {
	    CDR(acons) = CONS(cons, NIL);
	    acons = CDR(acons);
	}
    }

mapcar_done:
    mac->protect.length = length;

    return (result);
}

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

    if (!SYMBOL_P(function) && function->type != LispLambda_t)
	LispDestroy(mac, "%s: %s is a bad function",
		    STRFUN(builtin), STROBJ(function));

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
	    CDR(cons) = CONS(QUOTE(arglist), NIL);
	}
	cons = EVAL(code);
	if (result == NIL) {
	    result = acons = CONS(cons, NIL);
	    mac->protect.objects[length + 1] = result;
	}
	else {
	    CDR(acons) = CONS(cons, NIL);
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
    GC_ENTER();
    LispObj *karguments = NULL, *lambda, *arguments, *expect, *result = NIL;

    LispObj *item, *list, *test, *test_not, *key;

    key = ARGUMENT(4);
    test_not = ARGUMENT(3);
    test = ARGUMENT(2);
    list = ARGUMENT(1);
    item = ARGUMENT(0);

    if (list == NIL)	return (NIL);
    else ERROR_CHECK_LIST(list);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT");

    /* Allocate argument list to comparison function */
    arguments = CONS(NIL, CONS(NIL, NIL));
    GC_PROTECT(arguments);

    /* Resolve compare function, and expected result of comparison */
    if (test_not == NIL) {
	if (test == NIL)
	    lambda = Oequal;
	else
	    lambda = test;
	expect = T;
    }
    else {
	lambda = test_not;
	expect = NIL;
    }

    /* Allocate list for arguments to key lambda, if required */
    if (key != NIL) {
	karguments = CONS(NIL, NIL);
	GC_PROTECT(karguments);
    }

    /* Initialize first argument */
    CAR(arguments) = item;

    /* Loop on list, comparing every element with item,
     * or result of applying key to it */
    for (; CONS_P(list); list = CDR(list)) {
	if (karguments != NULL) {
	    CAR(karguments) = CAR(list);
	    CADR(arguments) = APPLY(key, karguments);
	}
	else
	    CADR(arguments) = CAR(list);
	if (APPLY(lambda, arguments) == expect) {
	    result = list;
	    break;
	}
    }

    GC_LEAVE();

    return (result);
}

LispObj *
LispMemberIf(LispMac *mac, LispBuiltin *builtin, int ifnot)
/*
 member-if predicate list &key key
 member-if-not predicate list &key key
 */
{
    GC_ENTER();
    LispObj *arguments, *expect, *result = NIL, *karguments = NULL;

    LispObj *predicate, *list, *key;

    key = ARGUMENT(2);
    list = ARGUMENT(1);
    predicate = ARGUMENT(0);

    if (list == NIL)	return (NIL);
    else ERROR_CHECK_LIST(list);

    /* Allocate argument list */
    arguments = CONS(NIL, NIL);
    GC_PROTECT(arguments);

    /* Resolve compare function, and expected result of comparison */
    expect = ifnot ? NIL : T;

    if (key != NIL) {
	karguments = CONS(NIL, NIL);
	GC_PROTECT(karguments);
    }

    /* Loop on list, applying predicate and checking result */
    for (; CONS_P(list); list = CDR(list)) {
	if (karguments != NULL) {
	    CAR(karguments) = CAR(list);
	    CAR(arguments) = APPLY(key, karguments);
	}
	else
	    CAR(arguments) = CAR(list);
	if (APPLY(predicate, arguments) == expect) {
	    result = list;
	    break;
	}
    }

    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_MemberIf(LispMac *mac, LispBuiltin *builtin)
/*
 member-if predicate list &key key
 */
{
    return (LispMemberIf(mac, builtin, 0));
}

LispObj *
Lisp_MemberIfNot(LispMac *mac, LispBuiltin *builtin)
/*
 member-if-not predicate list &key key
 */
{
    return (LispMemberIf(mac, builtin, 1));
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
    MACRO_ARGUMENT1();

    result = EVAL(form);

    result = cons = CONS(result, NIL);
    GC_PROTECT(result);
    for (i = 0; i < RETURN_COUNT; i++) {
	CDR(cons) = CONS(RETURN(i), NIL);
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
    for (list = CAR(lists); CONS_P(lists);
	 lists = CDR(lists), list = CAR(lists))
	if (list != NIL)
	    break;

    if (CONS_P(list)) {
	for (tail = list; CONS_P(CDR(tail)); tail = CDR(tail))
	    ;
	for (lists = CDR(lists); CONS_P(lists); lists = CDR(lists)) {
	    CDR(tail) = CAR(lists);
	    for (; CONS_P(CDR(tail)); tail = CDR(tail))
		;
	}
	/* if last argument is not a list */
	if (lists != NIL)
	    CDR(tail) = lists;
    }

    return (list);
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
    MACRO_ARGUMENT1();

    for (; CONS_P(args); args = CDR(args)) {
	if (NCONSTANT_P(result = CAR(args)))
	    result = EVAL(result);
	if (result != NIL)
	    break;
    }

    return (result);
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
	LispObj *arguments = CAR(declaration), *object = CAR(arguments);
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
    MACRO_ARGUMENT2();

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
    MACRO_ARGUMENT3();

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
    MACRO_ARGUMENT1();

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
    GC_ENTER();
    LispObj *res = NIL, *cons = NIL, *valist = NIL;
    LispObj *symbols, *values, *body;

    body = ARGUMENT(2);
    values = ARGUMENT(1);
    symbols = ARGUMENT(0);
    MACRO_ARGUMENT3();

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
	    CDR(cons) = CONS(CONS(CAR(symbols), CAR(values)), NIL);
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

    for (; CONS_P(body); body = CDR(body))
	if (NCONSTANT_P(res = CAR(body)))
	    res = EVAL(res);
    GC_LEAVE();

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
	CDR(MOD) = CONS(CAR(MOD), CDR(MOD));
	CAR(MOD) = module;
    }

    LispSetVar(mac, mac->modules, MOD);

    return (MOD);
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

	/* SPECIAL MACRO, with no EVAL's */
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
	    CAR(to) = CAR(from);
    }

    return (sequence1);
}

LispObj *
Lisp_RemoveDuplicates(LispMac *mac, LispBuiltin *builtin)
/*
 remove-duplicates sequence &key from-end test test-not start end key
 */
{
    GC_ENTER();
    long i, j, start, end, length, count;
    LispObj *lambda, *arguments, *karguments, *expect, *result, *cons, *value;

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
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT");

    /* Allocate argument list to comparison function */
    arguments = CONS(NIL, CONS(NIL, NIL));
    GC_PROTECT(arguments);

    /* Resolve comparison function, and expected result of comparison */
    if (test_not == NIL) {
	if (test == NIL)
	    lambda = Oequal;
	else
	    lambda = test;
	expect = T;
    }
    else {
	lambda = test_not;
	expect = NIL;
    }

    /* Allocate space for arguments if needs a predicate for comparison */
    if (key != NIL) {
	karguments = CONS(NIL, NIL);
	GC_PROTECT(karguments);
    }
    else
	karguments = NULL;


    /* Initialize */
    count = 0;
    result = cons = NIL;

    /* Use same code, update start/end offsets */
    if (from_end != NIL) {
	i = length - start;
	start = length - end;
	end = i;
    }

    if (STRING_P(sequence)) {
	char *ptr, *string, *buffer = LispMalloc(mac, length + 1);

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

	/* Check if needs to remove something */
	if (karguments == NULL) {
	    LispObj cleft, cright;

	    /* Build arguments to comparison function */
	    cleft.type = cright.type = LispCharacter_t;
	    CAR(arguments) = &cleft;
	    CADR(arguments) = &cright;

	    for (; i < end; i++) {
		value = expect == NIL ? T : NIL;
		cleft.data.integer = string[i];
		for (j = i + 1; j < end; j++) {
		    cright.data.integer = string[j];
		    value = APPLY(lambda, arguments);
		    if (value == expect)
			break;
		}
		if (value != expect)
		    *ptr++ = string[i];
		else
		    ++count;
	    }
	}
	else {
	    LispObj *chars, *cdr;

	    /* Apply key to characters in string only once */
	    for (chars = cdr = NIL; i < end; i++) {
		CAR(karguments) = CHAR(string[i]);
		value = APPLY(key, karguments);
		if (chars == NIL) {
		    chars = cdr = CONS(value, NIL);
		    GC_PROTECT(chars);
		}
		else {
		    CDR(cdr) = CONS(value, NIL);
		    cdr = CDR(cdr);
		}
	    }

	    for (i = start; i < end; i++, chars = CDR(chars)) {
		value = expect == NIL ? T : NIL;
		CAR(arguments) = CAR(chars);
		for (j = i + 1, cdr = CDR(chars); j < end; j++, cdr = CDR(cdr)) {
		    CADR(arguments) = CAR(cdr);
		    value = APPLY(lambda, arguments);
		    if (value == expect)
			break;
		}
		if (value != expect)
		    *ptr++ = string[i];
		else
		    ++count;
	    }
	}

	if (count) {
	    /* Copy ending bytes */
	    for (; i <= length; i++)   /* Also copy the ending nul */
		*ptr++ = string[i];
	    if (from_end == NIL)
		result = STRING(buffer);
	    else {
		for (i = 0, ptr = buffer + strlen(buffer); ptr > buffer; i++)
		    string[i] = *--ptr;
		string[i] = '\0';
		result = STRING(string);
	    }
	}
	else
	    result = sequence;
	LispFree(mac, buffer);
	if (from_end != NIL)
	    LispFree(mac, string);
    }
    else {
	LispObj *object, **kobjects = NULL;
	LispObj **objects = LispMalloc(mac, sizeof(LispObj*) * length);

	if (!CONS_P(sequence))
	    object = sequence->data.array.list;
	else
	    object = sequence;

	/* Put data in a vector */
	if (from_end == NIL) {
	    for (i = 0; i < length; i++, object = CDR(object))
		objects[i] = CAR(object);
	}
	else {
	    for (i = length - 1; i >= 0; i--, object = CDR(object))
		objects[i] = CAR(object);
	}

	/* Apply key predicate if required */
	if (karguments != NULL) {
	    kobjects = LispMalloc(mac, sizeof(LispObj*) * (end - start));
	    for (i = start; i < end; i++) {
		CAR(karguments) = objects[i];
		kobjects[i - start] = APPLY(key, karguments);
		GC_PROTECT(kobjects[i - start]);
	    }
	}

	/* Check if needs to remove something */
	for (i = start; i < end; i++) {
	    value = expect == NIL ? T : NIL;
	    if (karguments == NULL)
		CAR(arguments) = objects[i];
	    else
		CAR(arguments) = kobjects[i - start];
	    for (j = i + 1; j < end; j++) {
		if (karguments == NULL)
		    CADR(arguments) = objects[j];
		else
		    CADR(arguments) = kobjects[j - start];
		value = APPLY(lambda, arguments);
		if (value == expect)
		    break;
	    }
	    if (value == expect) {
		objects[i] = NULL;
		++count;
	    }
	}

	if (count) {
	    /* Create result list */
	    for (i = 0; i < length; i++) {
		if (objects[i] != NULL) {
		    if (result == NIL) {
			result = cons = CONS(objects[i], NIL);
			GC_PROTECT(result);
		    }
		    else {
			CDR(cons) = CONS(objects[i], NIL);
			cons = CDR(cons);
		    }
		}
	    }
	    if (from_end != NIL)
		result = LispReverse(result);
	}
	else
	    result = sequence;
	LispFree(mac, objects);
	if (karguments != NULL)
	    LispFree(mac, kobjects);

	if (!CONS_P(sequence))
	    result = VECTOR(result);
    }
    GC_LEAVE();

    return (result);
}

LispObj *
LispRemove_IfNot(LispMac *mac, LispBuiltin *builtin, int remove, int ifnot)
/*
 remove item sequence &key from-end test test-not start end count key
 remove-if predicate sequence &key from-end start end count key
 remove-if-not predicate sequence &key from-end start end count key
 */
{
    GC_ENTER();
    long i, start, end, length, count, copy;
    LispObj *arguments, *karguments, *expect, *result, *cons;

    LispObj *item, *predicate, *sequence, *from_end,
	    *test, *test_not, *ostart, *oend, *ocount, *key;

    if (remove) {
	key = ARGUMENT(8);
	ocount = ARGUMENT(7);
	oend = ARGUMENT(6);
	ostart = ARGUMENT(5);
	test_not = ARGUMENT(4);
	test = ARGUMENT(3);
    }
    else {
	key = ARGUMENT(6);
	ocount = ARGUMENT(5);
	oend = ARGUMENT(4);
	ostart = ARGUMENT(3);
	test_not = test = NIL;
    }
    from_end = ARGUMENT(2);
    sequence = ARGUMENT(1);
    if (remove) {
	item = ARGUMENT(0);
	predicate = NIL;
    }
    else {
	predicate = ARGUMENT(0);
	item = NIL;
    }

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
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT");

    /* Allocate argument list to comparison function */
    if (remove)
	arguments = CONS(NIL, CONS(NIL, NIL));
    else
	arguments = CONS(NIL, NIL);
    GC_PROTECT(arguments);

    /* Resolve comparison function, and expected result of comparison */
    if (remove) {
	if (test_not == NIL) {
	    if (test == NIL)
		predicate = Oequal;
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
	expect = ifnot ? NIL : T;

    /* Allocate space for arguments if needs a predicate for comparison */
    if (key != NIL) {
	karguments = CONS(NIL, NIL);
	GC_PROTECT(karguments);
    }
    else
	karguments = NULL;

    /* Use value of copy to check if something was changed */
    copy = count;
    result = cons = NIL;

    /* Use same code, update start/end offsets */
    if (from_end != NIL) {
	i = length - start;
	start = length - end;
	end = i;
    }

    /* Initialize */
    if (remove)
	CAR(arguments) = item;

    if (STRING_P(sequence)) {
	LispObj compare;
	char *ptr, *string, *buffer = LispMalloc(mac, length + 1);

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

	/* Build arguments to comparison function */
	compare.type = LispCharacter_t;
	if (karguments == NULL) {
	    if (remove)
		CADR(arguments) = &compare;
	    else
		CAR(arguments) = &compare;
	}
	else
	    CAR(karguments) = &compare;

	/* Check if needs to remove something */
	for (; i < end && count > 0; i++) {
	    compare.data.integer = string[i];
	    if (karguments != NULL) {
		/* Apply predicate */
		if (remove)
		    CADR(arguments) = APPLY(key, karguments);
		else
		    CAR(arguments) = APPLY(key, karguments);
	    }
	    if (APPLY(predicate, arguments) != expect)
		*ptr++ = string[i];
	    else
		--count;
	}

	if (copy != count) {
	    /* Copy ending bytes */
	    for (; i <= length; i++)   /* Also copy the ending nul */
		*ptr++ = string[i];
	    if (from_end == NIL)
		result = STRING(buffer);
	    else {
		for (i = 0, ptr = buffer + strlen(buffer); ptr > buffer; i++)
		    string[i] = *--ptr;
		string[i] = '\0';
		result = STRING(string);
	    }
	}
	else
	    result = sequence;
	LispFree(mac, buffer);
	if (from_end != NIL)
	    LispFree(mac, string);
    }
    else {
	LispObj *object;
	LispObj **objects = LispMalloc(mac, sizeof(LispObj*) * length);

	if (!CONS_P(sequence))
	    object = sequence->data.array.list;
	else
	    object = sequence;

	/* Put data in a vector */
	if (from_end == NIL) {
	    for (i = 0; i < length; i++, object = CDR(object))
		objects[i] = CAR(object);
	}
	else {
	    for (i = length - 1; i >= 0; i--, object = CDR(object))
		objects[i] = CAR(object);
	}

	/* Check if needs to remove something */
	for (i = start; i < end && count > 0; i++) {
	    if (karguments == NULL) {
		if (remove)
		    CADR(arguments) = objects[i];
		else
		    CAR(arguments) = objects[i];
	    }
	    else {
		CAR(karguments) = objects[i];
		if (remove)
		    CADR(arguments) = APPLY(key, karguments);
		else
		    CAR(arguments) = APPLY(key, karguments);
	    }
	    if (APPLY(predicate, arguments) == expect) {
		objects[i] = NULL;
		--count;
	    }
	}

	if (copy != count) {
	    /* Create result list */
	    for (i = 0; i < length; i++) {
		if (objects[i] != NULL) {
		    if (result == NIL) {
			result = cons = CONS(objects[i], NIL);
			GC_PROTECT(result);
		    }
		    else {
			CDR(cons) = CONS(objects[i], NIL);
			cons = CDR(cons);
		    }
		}
	    }
	    if (from_end != NIL)
		result = LispReverse(result);
	}
	else
	    result = sequence;
	LispFree(mac, objects);

	if (!CONS_P(sequence))
	    result = VECTOR(result);
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
    return (LispRemove_IfNot(mac, builtin, 1, 0));
}

LispObj *
Lisp_RemoveIf(LispMac *mac, LispBuiltin *builtin)
/*
 remove-if predicate sequence &key from-end start end count key
 */
{
    return (LispRemove_IfNot(mac, builtin, 0, 0));
}

LispObj *
Lisp_RemoveIfNot(LispMac *mac, LispBuiltin *builtin)
/*
 remove-if-not predicate sequence &key from-end start end count key
 */
{
    return (LispRemove_IfNot(mac, builtin, 0, 1));
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
    MACRO_ARGUMENT1();

    while (blevel) {
	LispBlock *block = mac->block.block[--blevel];

	if (block->type == LispBlockClosure)
	    /* if reached a function call */
	    break;
	if (block->type == LispBlockTag && block->tag.type == LispNil_t) {
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
    MACRO_ARGUMENT2();

    if (name != NIL && name != T && !SYMBOL_P(name))
	LispDestroy(mac, "%s: %s is not a valid block name",
		    STRFUN(builtin), STROBJ(name));

    while (blevel) {
	int jmp = 1;
	LispBlock *block = mac->block.block[--blevel];

	if (name->type == block->tag.type) {
	    switch (name->type) {
		case LispNil_t:
		case LispTrue_t:
		    break;
		case LispAtom_t:
		    jmp = name->data.atom == block->tag.data.atom;
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
Lisp_Reverse(LispMac *mac, LispBuiltin *builtin)
/*
 reverse sequence
 */
{
    LispObj *list, *result;

    LispObj *sequence;

    sequence = ARGUMENT(0);

    list = NIL;
    switch (sequence->type) {
	case LispNil_t:
	    return (NIL);
	    break;
	case LispString_t: {
	    int i, length;
	    char *from, *to;

	    from = THESTR(sequence);
	    length = strlen(from);
	    to = LispMalloc(mac, length + 1);
	    to[length] = '\0';
	    from += length - 1;
	    for (i = 0; i < length; i++)
		to[i] = from[-i];
	    result = STRING2(to);

	}   return (result);
	case LispArray_t:
	    if (sequence->data.array.rank != 1)
		goto bad_reverse_sequence;
	    list = sequence->data.array.list;
	    break;
	case LispCons_t:
	    list = sequence;
	    break;
	default:
bad_reverse_sequence:
	    LispDestroy(mac, "%s: %s is not a sequence",
			STRFUN(builtin), STROBJ(sequence));
	    /*NOTREACHED*/
    }

    GCProtect();
    result = NIL;
    while (CONS_P(list)) {
	result = CONS(CAR(list), result);
	list = CDR(list);
    }
    GCUProtect();

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
Lisp_Rplaca(LispMac *mac, LispBuiltin *builtin)
/*
 rplaca place value
 */
{
    LispObj *place, *value;

    value = ARGUMENT(1);
    place = ARGUMENT(0);

    ERROR_CHECK_CONS(place);
    CAR(place) = value;

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
    CDR(place) = value;

    return (place);
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
Lisp_SetQ(LispMac *mac, LispBuiltin *builtin)
/*
 setq &rest form
 */
{
    LispObj *result, *variable, *form;

    form = ARGUMENT(0);
    MACRO_ARGUMENT1();

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
    MACRO_ARGUMENT1();

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
		    CDR(cdr) = CONS(QUOTE(CAR(place)), NIL);
		    cdr = CDR(cdr);
		}

		for (obj = CDR(place); obj != NIL; obj = CDR(obj)) {
		    CDR(cdr) = CONS(CAR(obj), NIL);
		    cdr = CDR(cdr);
		}
		CDR(cdr) = CONS(result, NIL);
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

LispObj *
Lisp_Stringp(LispMac *mac, LispBuiltin *builtin)
/*
 stringp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (STRING_P(object) ? T : NIL);
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
		CDR(cons) = CONS(CAR(object), NIL);
		cons = CDR(cons);
	    }
	}
	else
	    result = NIL;

	if (sequence->type == LispArray_t) {
	    object = LispNew(mac, result, NIL);
	    object->type = LispArray_t;
	    object->data.array.list = result;
	    object->data.array.dim = CONS(INTEGER(seqlength), NIL);
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
    int head, lex, length, dyn, found;
    LispObj *list, *body, *ptr, *tag, **p_list, **p_body;
    LispBlock *block;

    body = ARGUMENT(0);
    MACRO_ARGUMENT1();

    /* Initialize */
    list = body;
    p_list = &list;
    p_body = &body;
    block = LispBeginBlock(mac, NIL, LispBlockBody);

    /* Save environment information */
    head = mac->env.head;
    lex = mac->env.lex;
    length = mac->env.length;
    dyn = mac->dyn.length;

    /* Loop */
    if (setjmp(block->jmp) != 0) {
	/* Restore environment */
	mac->env.head = head;
	mac->env.lex = lex;
	mac->env.length = length;
	mac->dyn.length = dyn;

	found = 0;
	tag = mac->block.block_ret;
	for (ptr = *p_list; CONS_P(ptr); ptr = CDR(ptr)) {
	    if (CAR(ptr)->type == tag->type) {
		switch (tag->type) {
		    case LispNil_t:
		    case LispTrue_t:
		    case LispAtom_t:
			found = CAR(ptr) == tag;
			break;
		    case LispInteger_t:
			found = CAR(ptr)->data.integer == tag->data.integer;
			break;
		    default:
			break;
		}
	    }
	    if (found)
		break;
	}

	if (!found)
	    LispDestroy(mac, "%s: no such tag %s", STRFUN(builtin), STROBJ(tag));

	*p_body = CDR(ptr);
    }

    /* Skip tags */
    for (; CONS_P(body); body = CDR(body)) {
	if (CONS_P(CAR(body)))
	    break;
    }

    /* Execute code */
    for (; CONS_P(body); body = CDR(body)) {
	if (CONS_P(CAR(body)))
	    /* If not a tag */
	    EVAL(CAR(body));
    }
    /* If got here, (go) not called */

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
    MACRO_ARGUMENT2();

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
    MACRO_ARGUMENT2();

    if (NCONSTANT_P(tag))
	tag = EVAL(tag);

    if (blevel == 0)
	LispDestroy(mac, "%s: not within a block",
		    STRFUN(builtin));

    while (blevel) {
	int jmp = 1;
	LispBlock *block = mac->block.block[--blevel];

	if (block->type == LispBlockCatch && tag->type == block->tag.type) {
	    switch(tag->type) {
		case LispNil_t:
		case LispTrue_t:
		    break;
		case LispAtom_t:
		case LispString_t:
		    jmp = tag->data.atom == block->tag.data.atom;
		    break;
		case LispCharacter_t:
		case LispInteger_t:
		    jmp = tag->data.integer == block->tag.data.integer;
		    break;
		case LispRatio_t:
		    jmp = tag->data.ratio.numerator ==
			  block->tag.data.ratio.numerator &&
			  tag->data.ratio.denominator ==
			  block->tag.data.ratio.denominator;
		    break;
		case LispReal_t:
		    jmp = tag->data.real == block->tag.data.real;
		    break;
		default:
		    jmp = memcmp(tag, &(block->tag), sizeof(LispObj)) == 0;
		    break;
	    }
	    if (jmp) {
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
    GC_ENTER();
    LispObj *karguments = NULL, *lambda,
	    *arguments, *expect, *result,
	    *item, *value, *clist2, *compare, *cons;

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

    /* Check for fast return */
    if (list1 == NIL)
	return (list2);
    if (list2 == NIL)
	return (list1);

    /* Cannot specify both :test and :test-not */
    if (test != NIL && test_not != NIL)
	LispDestroy(mac, "%s: specify either :TEST or :TEST-NOT");

    /* Allocate argument list to comparison function */
    arguments = CONS(NIL, CONS(NIL, NIL));
    GC_PROTECT(arguments);

    /* Resolve comparison function, and expected result of comparison */
    if (test_not == NIL) {
	if (test == NIL)
	    lambda = Oequal;
	else
	    lambda = test;
	expect = T;
    }
    else {
	lambda = test_not;
	expect = NIL;
    }

    result = cons = NIL;

    /* Allocate list for arguments to key lambda, if required,
     * and also make a copy of list2 with the key predicate applyied */
    if (key != NIL) {
	karguments = CONS(NIL, NIL);
	GC_PROTECT(karguments);

	/* Apply predicate to list2 only once */
	for (compare = list2; CONS_P(compare); compare = CDR(compare)) {
	    CAR(karguments) = CAR(compare);
	    item = APPLY(key, karguments);
	    if (result == NIL) {
		result = cons = CONS(item, NIL);
		GC_PROTECT(result);
	    }
	    else {
		CDR(cons) = CONS(item, NIL);
		cons = CDR(cons);
	    }
	}
	clist2 = result;
	result = cons = NIL;
    }
    else
	clist2 = list2;

    /* Compare elements of lists
     * Logic:
     *		1) Walk list1 and if CAR(list1) not in list2, add it to result
     *		2) Add list2 to result
     */
    for (; CONS_P(list1); list1 = CDR(list1)) {
	item = CAR(list1);

	/* Apply key predicate if required */
	if (karguments != NULL) {
	    CAR(karguments) = item;
	    CAR(arguments) = APPLY(key, karguments);
	}
	else
	    CAR(arguments) = item;

	/* Initialize for comparison loop */
	value = expect == NIL ? T : NIL;

	/* Compare against list2 */
	for (compare = clist2; CONS_P(compare); compare = CDR(compare)) {
	    CADR(arguments) = CAR(compare);
	    value = APPLY(lambda, arguments);
	    if (value == expect)
		break;
	}

	/* If item not found */
	if (value != expect) {
	    if (result == NIL) {
		result = cons = CONS(item, NIL);
		GC_PROTECT(result);
	    }
	    else {
		CDR(cons) = CONS(item, NIL);
		cons = CDR(cons);
	    }
	}
    }

    /* Add list2 to tail of result */
    if (result == NIL)
	result = list2;
    else
	CDR(cons) = list2;

    GC_LEAVE();

    return (result);
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
    MACRO_ARGUMENT2();

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
    MACRO_ARGUMENT2();

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
    MACRO_ARGUMENT2();

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
    MACRO_ARGUMENT2();

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
    MACRO_ARGUMENT2();

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
 lisp::elt-store sequence index value &aux (length (length sequence))
 */
{
    int length, offset;

    LispObj *sequence, *oindex, *value, *olength;

    olength = ARGUMENT(3);
    value = ARGUMENT(2);
    oindex = ARGUMENT(1);
    sequence = ARGUMENT(0);

    ERROR_CHECK_INDEX(oindex);
    offset = oindex->data.integer;
    length = olength->data.integer;

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
	CAR(sequence) = value;
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
	long length = strlen(THESTR(array));
	long offset = CAR(subscripts)->data.integer;

	if (offset >= length)
	    LispDestroy(mac, "%s: index %d too large for sequence length %d",
			STRFUN(builtin), offset, length);

	if (value->data.integer < 0 || value->data.integer > 255)
	    LispDestroy(mac, "%s: cannot represent character %d",
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
	    LispDestroy(mac, "%s: %d is out of range, index %d",
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

    CAR(array) = value;

    return (value);
}

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

/* $XFree86: xc/programs/xedit/lisp/core.c,v 1.1 2001/08/31 15:00:13 paulo Exp $ */

#include "core.h"
#include "helper.h"
#include "private.h"

/*
 * Implementation
 */
LispObj *
Lisp_Mul(LispMac *mac, LispObj *list, char *fname)
{
    double result = 1.0;

    for (; list != NIL; list = CDR(list)) {
	if (CAR(list)->type != LispReal_t)
	    LispDestroy(mac, ExpectingNumberAt, fname);
	result *= CAR(list)->data.real;
    }
    return (REAL(result));
}

LispObj *
Lisp_Plus(LispMac *mac, LispObj *list, char *fname)
{
    double result = 0.0;

    for (; list != NIL; list = CDR(list)) {
	if (CAR(list)->type != LispReal_t)
	    LispDestroy(mac, ExpectingNumberAt, fname);
	result += CAR(list)->data.real;
    }
    return (REAL(result));
}

LispObj *
Lisp_Minus(LispMac *mac, LispObj *list, char *fname)
{
    double result;

    if (CAR(list)->type != LispReal_t)
	LispDestroy(mac, ExpectingNumberAt, fname);
    result = CAR(list)->data.real;
    list = CDR(list);
    if (list == NIL)
	return (REAL(-result));
    for (; list != NIL; list = CDR(list)) {
	if (CAR(list)->type != LispReal_t)
	    LispDestroy(mac, ExpectingNumberAt, fname);
	result -= CAR(list)->data.real;
    }
    return (REAL(result));
}

LispObj *
Lisp_Div(LispMac *mac, LispObj *list, char *fname)
{
    double result;

    if (CAR(list)->type != LispReal_t)
	LispDestroy(mac, ExpectingNumberAt, fname);
    result = CAR(list)->data.real;
    list = CDR(list);
    if (list == NIL) {
	if (result == 0.0)
	    LispDestroy(mac, "divide by 0, at %s", fname);
	return (REAL(1.0 / result));
    }
    for (; list != NIL; list = CDR(list)) {
	if (CAR(list)->type != LispReal_t)
	    LispDestroy(mac, ExpectingNumberAt, fname);
	if (CAR(list)->data.real == 0.0)
	    LispDestroy(mac, "divide by 0, at %s", fname);
	result /= CAR(list)->data.real;
    }
    return (REAL(result));
}

LispObj *
Lisp_OnePlus(LispMac *mac, LispObj *list, char *fname)
{
    if (CAR(list)->type != LispReal_t)
	LispDestroy(mac, ExpectingNumberAt, fname);
    return (REAL(CAR(list)->data.real + 1.0));
}

LispObj *
Lisp_OneMinus(LispMac *mac, LispObj *list, char *fname)
{
    if (CAR(list)->type != LispReal_t)
	LispDestroy(mac, ExpectingNumberAt, fname);
    return (REAL(CAR(list)->data.real - 1.0));
}

LispObj *
Lisp_Less(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispBoolCond(mac, list, fname, LESS));
}

LispObj *
Lisp_LessEqual(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispBoolCond(mac, list, fname, LESS_EQUAL));
}

LispObj *
Lisp_Equal_(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispBoolCond(mac, list, fname, EQUAL));
}

LispObj *
Lisp_Greater(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispBoolCond(mac, list, fname, GREATER));
}

LispObj *
Lisp_GreaterEqual(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispBoolCond(mac, list, fname, GREATER_EQUAL));
}

LispObj *
Lisp_Aref(LispMac *mac, LispObj *list, char *fname)
{
    long c, count, idx, seq;
    LispObj *ary = CAR(list), *dim = CDR(list), *obj;

    if (ary->type != LispArray_t)
	LispDestroy(mac, "%s is not an array, at %s",
		    LispStrObj(mac, ary), fname);

    for (count = 0, list = dim, obj = ary->data.array.dim; list != NIL;
	 count++, list = CDR(list), obj = CDR(obj)) {
	if (count >= ary->data.array.rank)
	    LispDestroy(mac, "too many subscripts %s, at %s",
			LispStrObj(mac, dim), fname);
	if (CAR(list)->type != LispReal_t ||
	    (int)CAR(list)->data.real != CAR(list)->data.real ||
	    CAR(list)->data.real < 0 ||
	    CAR(list)->data.real >= CAR(obj)->data.real)
	    LispDestroy(mac, "%s is out of range or a bad index, at %s",
			LispStrObj(mac, CAR(list)), fname);
    }
    if (count < ary->data.array.rank)
	LispDestroy(mac, "too few subscripts %s, at %s",
		    LispStrObj(mac, dim), fname);

    for (count = seq = 0, list = dim; list != NIL; list = CDR(list), seq++) {
	for (idx = 0, obj = ary->data.array.dim; idx < seq; obj = CDR(obj), ++idx)
	    ;
	for (c = 1, obj = CDR(obj); obj != NIL; obj = CDR(obj))
	    c *= CAR(obj)->data.real;
	count += c * CAR(list)->data.real;
    }

    for (ary = ary->data.array.list; count > 0; ary = CDR(ary), count--)
	;
    mac->setf = ary;
    mac->cdr = 0;

    return (CAR(ary));
}

LispObj *
Lisp_Assoc(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *cmp, *obj, *res = NIL;

    cmp = CAR(list);
    list = CDR(list);
    if (list == NIL)
	return (NIL);

    for (list = CAR(list); list != NIL; list = CDR(list)) {
	if (list->type != LispCons_t || (obj = CAR(list))->type != LispCons_t)
	    LispDestroy(mac, ExpectingListAt, fname);
	else if (_LispEqual(mac, cmp, CAR(obj)) == T) {
	    res = obj;
	    break;
	}
    }

    return (res);
}

LispObj *
Lisp_And(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *obj, *res = T;

    for (; list != NIL; list = CDR(list)) {
	obj = EVAL(CAR(list));
	if (obj->type == LispNil_t) {
	    res = NIL;
	    break;
	}
    }
    return (res);
}

LispObj *
Lisp_Append(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res, *obj, *cdr, *cons;

    if (list == NIL)
	return (NIL);
    else if (CDR(list) == NIL)
	return (EVAL(CAR(list)));

    res = cdr = NIL;

    for (; list != NIL; list = CDR(list)) {
	obj = EVAL(CAR(list));
	if (obj->type != LispCons_t) {
	    if (CDR(list) != NIL)
		LispDestroy(mac, ExpectingListAt, fname);
	}
	if (res == NIL)
	    res = cdr = CONS(CAR(obj), CDR(obj));
	else {
	    if (CDR(cdr)->type == LispCons_t) {
		cons = CDR(cdr);
		while (CDR(cons)->type == LispCons_t) {
		    CDR(cdr) = CONS(CAR(cons), CDR(cons));
		    cons = CDR(cons);
		    cdr = CDR(cdr);
		}
		CDR(cdr) = CONS(CAR(CDR(cdr)), obj);
	    }
	    else
		CDR(cdr) = obj;
	    cdr = CDR(cdr);
	}
    }

    return (res);
}

LispObj *
Lisp_Apply(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *obj, *eval, *cdr;

    eval = EVAL(CAR(list));
    if (eval->type != LispAtom_t && eval->type != LispLambda_t)
	LispDestroy(mac, "%s is not a valid function name, at %s",
		LispStrObj(mac, eval), fname);
    obj = NIL;
    eval = cdr = CONS(eval, NIL);
    for (list = CDR(list); list != NIL; list = CDR(list)) {
	obj = EVAL(CAR(list));
	if (CDR(list) != NIL) {
	    CDR(cdr) = CONS(obj, NIL);
	    cdr = CDR(cdr);
	}
	else
	    CDR(cdr) = obj;
    }
    if (obj != NIL && (obj->type != LispCons_t || CDR(obj)->type != LispCons_t))
	LispDestroy(mac, "last apply argument must be a list");
    /* Need to quote back to avoid double evaluation */
    while (obj != NIL) {
	CAR(obj) = QUOTE(CAR(obj));
	obj = CDR(obj);
    }

    return (EVAL(eval));
}

LispObj *
Lisp_Atom(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res;

    if (list == NIL)
	res = T;
    else
	switch (CAR(list)->type) {
	    case LispCons_t:
		res = NIL;
		break;
	    default:
		res = T;
		break;
	}
    return (res);
}

LispObj *
Lisp_Butlast(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res, *obj, *cdr;
    int nlist;

    if ((obj = CAR(list))->type != LispCons_t)
	LispDestroy(mac, "%s is not a list, at %s",
		LispStrObj(mac, obj), fname);
    cdr = NIL;
    nlist = 0;
    while (obj->type == LispCons_t) {
	++nlist;
	obj = CDR(obj);
    }
    --nlist;
    obj = CDR(list);
    if (obj != NIL) {
	obj = CAR(obj);
	if (obj->type == LispReal_t) {
	    if (obj->data.real == 0) {
		res = CAR(list);
		if (CDR(res)->type != LispCons_t)
		    /* CL compatible return value */
		    return (CONS(CAR(res), NIL));
		return (res);
	    }
	    else if ((int)obj->data.real == obj->data.real &&
		obj->data.real > 0) {
		if (obj->data.real > nlist)
		    return (NIL);
		nlist -= obj->data.real - 1;
	    }
	    else
		LispDestroy(mac, "%s is a invalid argument, at %s",
			    LispStrObj(mac, obj), fname);
	}
	else
	    LispDestroy(mac, "%s is not a number, at %s",
			LispStrObj(mac, obj), fname);
    }

    res = NIL;
    list = CAR(list);
    for (; nlist > 0; list = CDR(list), nlist--) {
	obj = CAR(list);
	if (res == NIL)
	    res = cdr = CONS(obj, NIL);
	else {
	    CDR(cdr) = CONS(obj, NIL);
	    cdr = CDR(cdr);
	}
    }

    return (res);
}

LispObj *
Lisp_Car(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res = NIL;

    switch (CAR(list)->type) {
	case LispNil_t:
	    return (NIL);
	    break;
	case LispCons_t:
	    res = CAR(CAR(list));
	    break;
	default:
	    LispDestroy(mac, ExpectingListAt, fname);
	    /*NOTREACHED*/
    }
    mac->setf = CAR(list);
    mac->cdr = 0;

    return (res);
}

LispObj *
Lisp_Catch(LispMac *mac, LispObj *list, char *fname)
{
    int mlevel = mac->level, did_throw = 1;
    unsigned blevel = mac->block.block_level + 1;
    LispObj *res;
    LispBlock *block;

    if (blevel >= mac->block.block_size) {
	if ((block = (LispBlock*)realloc(mac->block.block,
					 sizeof(LispBlock) * blevel)) == NULL)
	    LispDestroy(mac, "out of memory, at %s", fname);
	mac->block.block = block;
	mac->block.block_size = blevel;
    }
    block = &(mac->block.block[mac->block.block_level]);
    memcpy(&(block->tag), EVAL(CAR(list)), sizeof(LispObj));
    mac->block.block_level = blevel;
    res = NIL;
    if (setjmp(block->jmp) == 0) {
	res = Lisp_Progn(mac, CDR(list), fname);
	did_throw = 0;
    }

    mac->level = mlevel;
    mac->block.block_level = blevel - 1;

    if (did_throw)
	res = mac->block.block_ret;

    return (res);
}

LispObj *
Lisp_Coerce(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *from, *to, *res = NIL;
    LispType type = LispNil_t;

    from = CAR(list);
    to = CAR(CDR(list));
    if (to == NIL)
	LispDestroy(mac, "cannot convert %s to nil, at %s",
		    LispStrObj(mac, from), fname);
    else if (to == T)
	return (from);
    else if (to->type != LispAtom_t)
	LispDestroy(mac, "bad argument %s, at %s", LispStrObj(mac, to), fname);
    else {
	if (strcmp(to->data.atom, "atom") == 0)
	    type = LispAtom_t;
	else if (strcmp(to->data.atom, "real") == 0)
	    type = LispReal_t;
	else if (strcmp(to->data.atom, "cons") == 0)
	    type = LispCons_t;
	else if (strcmp(to->data.atom, "string") == 0)
	    type = LispString_t;
	else if (strcmp(to->data.atom, "symbol") == 0)
	    type = LispSymbol_t;
	else if (strcmp(to->data.atom, "opaque") == 0)
	    type = LispOpaque_t;
	else
	    LispDestroy(mac, "invalid type specification %s, at %s",
			to->data.atom, fname);
    }

    if (from->type == LispOpaque_t) {
	switch (type) {
	    case LispAtom_t:
		res = ATOM(from->data.opaque.data);
		break;
	    case LispString_t:
		res = STRING(from->data.opaque.data);
		break;
	    case LispReal_t:
		res = REAL((double)((int)from->data.opaque.data));
		break;
	    case LispOpaque_t:
		res = OPAQUE(from->data.opaque.data, 0);
		break;
	    default:
		LispDestroy(mac, "cannot convert %s to %s, at %s",
			    LispStrObj(mac, from), to->data.atom, fname);
	}
    }
    else if (from->type != type)
		LispDestroy(mac, "cannot convert %s to %s, at %s",
			    LispStrObj(mac, from), to->data.atom, fname);
    else
	res = from;

    return (res);
}

LispObj *
Lisp_Cdr(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res = NIL;

    switch (CAR(list)->type) {
	case LispNil_t:
	    return (NIL);
	    break;
	case LispCons_t:
	    res = CDR(CAR(list));
	    break;
	default:
	    LispDestroy(mac, ExpectingListAt, fname);
	    /*NOTREACHED*/
    }
    mac->setf = CAR(list);
    mac->cdr = 1;

    return (res);
}

LispObj *
Lisp_Cond(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *eval, *res = NIL;

    for (; list != NIL; list = CDR(list)) {
	eval = CAR(list);
	if (eval->type != LispCons_t)
	    LispDestroy(mac, "%s is a illegal clause for %s",
			LispStrObj(mac, eval), fname);
	res = EVAL(CAR(eval));
	if (res->type == LispNil_t)
	    continue;
	for (eval = CDR(eval); eval != NIL; eval = CDR(eval))
	    res = EVAL(CAR(eval));
	break;
    }

    return (res);
}

LispObj *
Lisp_Cons(LispMac *mac, LispObj *list, char *fname)
{
    return (CONS(CAR(list), CAR(CDR(list))));
}

LispObj *
Lisp_Defmacro(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispDefLambda(mac, list, LispMacro));
}

LispObj *
Lisp_Defun(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispDefLambda(mac, list, LispFunction));
}

LispObj *
Lisp_Defstruct(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *str = list, *obj;

    /* get structure name */
    if (CAR(list)->type != LispAtom_t ||
	/* reserved name(s) */
	strcmp(CAR(list)->data.atom, "array") == 0)
	LispDestroy(mac, "%s cannot be a structure name, at %s",
		    LispStrObj(mac, CAR(list)), fname);

    /* get structure fields and default values */
    for (list = CDR(list); list != NIL; list = CDR(list)) {
	if ((CAR(list)->type != LispAtom_t &&
	     /* if not field name, with NIL as default value */
	     (CAR(list)->type != LispCons_t ||
	      CAR(CAR(list))->type != LispAtom_t)) ||
	    /* and not a pair, with field name and default value */
	    CAR(list)->data.atom[0] == ':' ||
	    /* and it is a valid field name */
	    strcmp(CAR(list)->data.atom, "p") == 0)
	    /* p is invalid as a field name due to `type'-p */
	    LispDestroy(mac, "%s cannot be a field for %s, at %s",
			LispStrObj(mac, CAR(list)), CAR(str)->data.atom, fname);

	/* check for repeated field names */
	for (obj = CDR(str); obj != list; obj = CDR(obj)) {
	    if (CAR(obj)->data.atom == CAR(list)->data.atom)
		LispDestroy(mac, "only one slot named :%s allowed, at %s",
			    LispStrObj(mac, CAR(obj)), fname);
	}
    }

    for (obj = STR; obj != NIL; obj = CDR(obj)) {
	if (CAR(CAR(obj))->data.atom == CAR(str)->data.atom) {
	    fprintf(stderr, "*** Warning: structure %s is being redefined\n",
		    CAR(CAR(obj))->data.atom);
	    break;
	}
    }

    if (obj != NIL)
	CAR(obj) = str;
    else if (STR == NIL)
	STR = CONS(str, NIL);
    else {
	CDR(STR) = CONS(CAR(STR), CDR(STR));
	CAR(STR) = str;
    }

    return (CAR(str));
}

LispObj *
Lisp_Equal(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispEqual(mac, CAR(list), CAR(CDR(list))));
}

LispObj *
Lisp_Eval(LispMac *mac, LispObj *list, char *fname)
{
    return (EVAL(CAR(list)));
}

LispObj *
Lisp_Funcall(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *fun = EVAL(CAR(list));

    return (EVAL(CONS(fun, CDR(list))));
}

LispObj *
Lisp_Get(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *sym, *key, *var, *obj;

    if ((sym = CAR(list))->type != LispAtom_t)
	LispDestroy(mac, "expecting symbol, at %s", fname);
    if ((key = CAR(CDR(list))) == NIL)
	return (NIL);
    if ((var = LispGetVar(mac, sym->data.atom, 1)) != NULL) {
	for (obj = var->data.symbol.plist; obj != NIL; obj = CDR(CDR(obj))) {
	    if (_LispEqual(mac, key, CAR(obj)) == T) {
		mac->setf = CDR(obj);
		mac->cdr = 0;

		return (CAR(CDR(obj)));
	    }
	}
    }
    else
	var = LispSetVar(mac, sym->data.atom, NULL, 1);
    GCProtect();
    var->data.symbol.plist = CONS(key, CONS(NIL, var->data.symbol.plist));
    GCUProtect();

    mac->setf = CDR(var->data.symbol.plist);
    mac->cdr = 0;

    return (NIL);
}

LispObj *
Lisp_Gc(LispMac *mac, LispObj *list, char *fname)
{
    LispGC(mac, NIL, NIL);

    return (list == NIL || CAR(list)->type == LispNil_t ? NIL : T);
}

LispObj *
Lisp_If(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *cond, *res;

    cond = EVAL(CAR(list));
    list = CDR(list);
    if (cond->type != LispNil_t)
	res = EVAL(CAR(list));
    else {
	if (CDR(list) == NIL)
	    res = NIL;
	else
	    res = EVAL(CAR(CDR(list)));
    }

    return (res);
}

LispObj *
Lisp_Lambda(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispDefLambda(mac, list, LispLambda));
}

LispObj *
Lisp_Last(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *seq, *obj;
    int nseq, count;

    if ((seq = CAR(list)) == NIL)
	return (seq);
    else if (seq->type != LispCons_t)
	LispDestroy(mac, ExpectingListAt, fname);

    if (CDR(list) != NIL) {
	obj = CAR(CDR(list));
	if (obj->type != LispReal_t || obj->data.real < 0 ||
	    (int)obj->data.real != obj->data.real)
	    LispDestroy(mac, "bad index %s, at %s", LispStrObj(mac, obj), fname);
	count = obj->data.real;
    }
    else
	count = 1;

    for (nseq = 0, obj = seq; obj->type == LispCons_t; nseq++, obj = CDR(obj))
	;

    count = nseq - count;

    if (count > nseq)
	return (NIL);
    else if (count <= 0)
	return (seq);

    for (; count > 0; count--, seq = CDR(seq))
	;

    return (seq);
}

LispObj *
Lisp_Length(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *obj;
    int length = 0;

    obj = CAR(list);
    switch (obj->type) {
	case LispNil_t:
	    break;
	case LispString_t:
	    length = strlen(obj->data.atom);
	    break;
	case LispCons_t:
	    while (obj->type == LispCons_t) {
		++length;
		obj = CDR(obj);
	    }
	    break;
	default:
	    LispDestroy(mac, "%s is not a sequence, at %s",
		    LispStrObj(mac, obj), fname);
	    /*NOTREACHED*/
    }

    return (REAL(length));
}

LispObj *
Lisp_Let(LispMac *mac, LispObj *list, char *fname)
{
    return (LispEnvRun(mac, list, Lisp_Progn, fname, 0));
}

LispObj *
Lisp_LetP(LispMac *mac, LispObj *list, char *fname)
{
    return (LispEnvRun(mac, list, Lisp_Progn, fname, 1));
}

LispObj *
Lisp_List(LispMac *mac, LispObj *list, char *fname)
{
    return (list);
}

LispObj *
Lisp_ListP(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res, *cdr, *obj;

    obj = EVAL(CAR(list));
    if (CDR(list) == NIL)
	return (obj);

    res = NIL;
    cdr = obj;
    for (list = CDR(list); list != NIL; list = CDR(list)) {
	obj = EVAL(CAR(list));
	if (res == NIL)
	    res = cdr = CONS(cdr, obj);
	else {
	    CDR(cdr) = CONS(CDR(cdr), obj);
	    cdr = CDR(cdr);
	}
    }

    return (res);
}

LispObj *
Lisp_Listp(LispMac *mac, LispObj *list, char *fname)
{
    switch (CAR(list)->type) {
	case LispNil_t:
	case LispCons_t:
	    return (T);
	default:
	    return (NIL);
    }
    /*NOTREACHED*/
}

LispObj *
Lisp_Makearray(LispMac *mac, LispObj *list, char *fname)
{
    LispType type = LispNil_t;
    long rank = 0, count = 1, zero, offset, c;
    LispObj *ary = NIL, *dim = NIL, *init, *cont, *disp, *obj;

    if (CAR(list)->type == LispReal_t) {
	if ((int)CAR(list)->data.real != CAR(list)->data.real ||
	    CAR(list)->data.real < 0)
	    LispDestroy(mac, "%s is a bad array dimension, at %s",
			LispStrObj(mac, CAR(list)), fname);
	else
	    dim = CONS(CAR(list), NIL);
	rank = 1;
	count = CAR(list)->data.real;
    }
    else if (CAR(list)->type == LispCons_t) {
	dim = CAR(list);

	for (obj = dim, rank = 0; obj != NIL; obj = CDR(obj), ++rank) {
	    if (obj->type != LispCons_t || CAR(obj)->type != LispReal_t ||
		(int)CAR(obj)->data.real != CAR(obj)->data.real ||
		CAR(obj)->data.real < 0)
		LispDestroy(mac, "%s is a bad array dimension, at %s",
			    LispStrObj(mac, dim), fname);
		count *= CAR(obj)->data.real;
	}
    }

    offset = -1;
    init = cont = disp = NULL;
    /* parse extra arguments */
    for (list = CDR(list); list != NIL; list = CDR(list)) {
	if (CAR(list)->type == LispAtom_t) {
	    if (strcmp(CAR(list)->data.atom, ":initial-element") == 0) {
		if ((list = CDR(list)) == NIL)
		    LispDestroy(mac, "expecting :initial-element, at %s",
				fname);
		if (init == NULL)
		    init = CAR(list);
	    }
	    else if (strcmp(CAR(list)->data.atom, ":element-type") == 0) {
		if ((list = CDR(list)) == NIL)
		    LispDestroy(mac, "expecting :element-type, at %s",
				fname);
		if (type == LispNil_t) {
		    if (CAR(list)->type != LispAtom_t) {
			if (CAR(list) == T)
			    type = LispTrue_t;
			else
			    LispDestroy(mac, "unsupported element type %s, at %s",
					LispStrObj(mac, CAR(list)), fname);
		    }
		    else {
			if (strcmp(CAR(list)->data.atom, "atom") == 0)
			    type = LispAtom_t;
			else if (strcmp(CAR(list)->data.atom, "real") == 0)
			    type = LispReal_t;
			else if (strcmp(CAR(list)->data.atom, "string") == 0)
			    type = LispString_t;
			else if (strcmp(CAR(list)->data.atom, "list") == 0)
			    type = LispCons_t;
			else if (strcmp(CAR(list)->data.atom, "opaque") == 0)
			    type = LispOpaque_t;
			else
			    LispDestroy(mac, "unsupported element type %s, at %s",
					CAR(list)->data.atom, fname);
		    }
		}
	    }
	    else if (strcmp(CAR(list)->data.atom, ":initial-contents") == 0) {
		if ((list = CDR(list)) == NIL)
		    LispDestroy(mac, "expecting :initial-contents, at %s",
				fname);
		if (cont == NULL)
		    cont = CAR(list);
		if (cont->type != LispCons_t)
		    LispDestroy(mac, "%s is not a list, at %s",
				LispStrObj(mac, cont), fname);
	    }
	    else if (strcmp(CAR(list)->data.atom, ":displaced-to") == 0) {
		if ((list = CDR(list)) == NIL)
		    LispDestroy(mac, "expecting :displaced-to, at %s",
				fname);
		if (disp == NULL)
		    disp = CAR(list);
		if (disp->type != LispArray_t)
		    LispDestroy(mac, "%s is not an array, at %s",
				LispStrObj(mac, disp), fname);
	    }
	    else if (strcmp(CAR(list)->data.atom, ":displaced-index-offset") == 0) {
		if ((list = CDR(list)) == NIL)
		    LispDestroy(mac, "expecting :displaced-index-offset, at %s",
				fname);
		if (CAR(list)->type != LispReal_t ||
		    (int)CAR(list)->data.real != CAR(list)->data.real ||
		    CAR(list)->data.real < 0)
		    LispDestroy(mac, "%s is a bad :displaced-index-offset, at %s",
				LispStrObj(mac, CAR(list)), fname);
		if (offset < 0)
		    offset = (int)CAR(list)->data.real;
	    }
	    else
		LispDestroy(mac, "%s is a illegal keyword for %s",
			    CAR(list)->data.atom, fname);
	}
	else
	    LispDestroy(mac, "%s is an invalid argument for %s",
			LispStrObj(mac, CAR(list)), fname);
    }

    c = 0;
    if (init)
	++c;
    if (cont)
	++c;
    if (disp || offset >= 0)
	++c;
    if (c > 1)
	LispDestroy(mac, "more than one initialization specified, at %s",
		    fname);

    zero = count == 0;
    if (disp) {
	if (offset < 0)
	    offset = 0;
	for (c = 1, obj = disp->data.array.dim; obj != NIL; obj = CDR(obj))
	    c *= (int)CAR(obj)->data.real;
	if (c < count + offset)
	    LispDestroy(mac, "array-total-size + displaced-index-offset "
			"exceeds total size, at %s", fname);
	for (c = 0, ary = disp->data.array.list; c < offset; c++)
	    ary = CDR(ary);
    }
    else if (cont) {
	if (rank == 1) {
	    for (ary = cont, c = 0; c < count; ary = CDR(ary), ++c)
		if (ary->type != LispCons_t)
		    LispDestroy(mac, "bad argument or size %s, at %s",
				LispStrObj(mac, ary), fname);
	    if (ary != NIL)
		LispDestroy(mac, "bad argument or size %s, at %s",
			    LispStrObj(mac, ary), fname);
	    ary = cont;
	}
	else {
	    LispObj *err = NIL;
	    /* check if list matches */
	    int i, j, k, *dims, *loop;

	    /* create iteration variables */
	    dims = LispMalloc(mac, sizeof(int) * rank);
	    loop = LispCalloc(mac, 1, sizeof(int) * (rank - 1));
	    for (i = 0, obj = dim; obj != NIL; i++, obj = CDR(obj))
		dims[i] = (int)CAR(obj)->data.real;

	    /* check if list matches specified dimensions */
	    while (loop[0] < dims[0]) {
		for (obj = cont, i = 0; i < rank - 1; i++) {
		    for (j = 0; j < loop[i]; j++)
			obj = CDR(obj);
		    err = obj;
		    if ((obj = CAR(obj))->type != LispCons_t)
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
		    if (obj->type != LispCons_t)
			goto make_array_error;
		}
		if (obj == NIL)
		    continue;
make_array_error:
		LispFree(mac, dims);
		LispFree(mac, loop);
		LispDestroy(mac, "bad argument or size %s, at %s",
			    LispStrObj(mac, err), fname);
	    }

	    /* list is correct, use it to fill initial values */

	    /* reset loop */
	    memset(loop, 0, sizeof(int) * (rank - 1));

	    GCProtect();
	    /* fill array with supplied values */
	    while (loop[0] < dims[0]) {
		for (obj = cont, i = 0; i < rank - 1; i++) {
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
		    if (ary == NIL)
			ary = CONS(CAR(obj), NIL);
		    else {
			CDR(ary) = CONS(CAR(ary), CDR(ary));
			CAR(ary) = CAR(obj);
		    }
		}
	    }
	    LispFree(mac, dims);
	    LispFree(mac, loop);
	    ary = LispReverse(ary);
	    GCUProtect();
	}
    }
    else {
	if (init == NULL)
	    init = NIL;
	GCProtect();
	/* allocate array */
	if (count) {
	    --count;
	    ary = CONS(init, NIL);
	    while (count) {
		CDR(ary) = CONS(CAR(ary), CDR(ary));
		CAR(ary) = init;
		count--;
	    }
	}
	GCUProtect();
    }

    if (type == LispNil_t)
	type = LispTrue_t;
    obj = LispNew(mac, ary, dim);
    obj->type = LispArray_t;
    obj->data.array.list = ary;
    obj->data.array.dim = dim;
    obj->data.array.rank = rank;
    obj->data.array.type = type;	/* XXX ignored */
    obj->data.array.zero = zero;

    return (obj);
}

LispObj *
Lisp_Mapcar(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *obj, *eval, *res, *cdres, *car, *cdr, *ptr, *fun;
    int i, level;

    fun = EVAL(CAR(list));
    if (fun->type != LispAtom_t && fun->type != LispLambda_t)
	LispDestroy(mac, "%s is not a valid function name, at %s",
		    LispStrObj(mac, fun), fname);
    cdres = NIL;
    for (level = 0, res = NIL; ; level++) {
	eval = cdr = CONS(fun, NIL);
	for (ptr = CDR(list); ptr != NIL; ptr = CDR(ptr)) {
	    car = EVAL(CAR(ptr));
	    if (car->type != LispCons_t)
		goto mapcar_done;
	    for (i = 0, obj = car; i < level; i++) {
		if ((obj = CDR(obj)) == NIL)
		    goto mapcar_done;
	    }
	    /* quote back to avoid double eval */
	    car = QUOTE(CAR(obj));
	    CDR(cdr) = CONS(car, NIL);
	    cdr = CDR(cdr);
	}
	obj = EVAL(eval);
	if (res == NIL)
	    res = cdres = CONS(obj, NIL);
	else {
	    CDR(cdres) = CONS(obj, NIL);
	    cdres = CDR(cdres);
	}
    }

    /* to be CL compatible */
mapcar_done:
    return (res);
}

LispObj *
Lisp_Max(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispMinMax(mac, list, fname, 1));
}

LispObj *
Lisp_Member(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *obj = CAR(list), *res = NIL;

    list = CAR(CDR(list));
    if (list->type == LispNil_t)
	return (NIL);
    else if (list->type != LispCons_t)
	LispDestroy(mac, ExpectingListAt, fname);

    for (; list != NIL; list = CDR(list))
	if (_LispEqual(mac, obj, CAR(list)) == T) {
	    res = list;
	    break;
	}

    return (res);
}

LispObj *
Lisp_Min(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispMinMax(mac, list, fname, 0));
}

LispObj *
Lisp_Nth(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispNth(mac, list, fname, 0));
}

LispObj *
Lisp_Nthcdr(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispNth(mac, list, fname, 1));
}

LispObj *
Lisp_Null(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res = NIL;

    if (list != NIL && CAR(list)->type == LispNil_t)
	res = T;
    return (res);
}

LispObj *
Lisp_Numberp(LispMac *mac, LispObj *list, char *fname)
{
    return (CAR(list)->type == LispReal_t ? T : NIL);
}

LispObj *
Lisp_Or(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *obj, *res = NIL;

    for (; list != NIL; list = CDR(list)) {
	obj = EVAL(CAR(list));
	if (obj->type != LispNil_t) {
	    res = T;
	    break;
	}
    }
    return (res);
}

LispObj *
Lisp_Pop(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res, *var, *cdr;

    if ((var = CAR(list))->type != LispAtom_t)
	LispDestroy(mac, "cannot pop from %s", LispStrObj(mac, var));

    var = EVAL(var);
    if (var->type == LispNil_t)
	return (var);
    else if (var->type != LispCons_t)
	LispDestroy(mac, "%s is not of type list, at %s",
		    LispStrObj(mac, var), fname);

    res = CAR(var);
    cdr = CDR(var);

    switch (var->type = cdr->type) {
	case LispAtom_t:
	case LispString_t:
	    var->data.atom = cdr->data.atom;
	    break;
	case LispReal_t:
	    var->data.real = cdr->data.real;
	    break;
	case LispQuote_t:
	    var->data.quote = cdr->data.quote;
	    break;
	case LispCons_t:
	    CAR(var) = CAR(cdr);
	    CDR(var) = CDR(cdr);
	    break;
	default:
	    break;
    }

    return (res);
}

LispObj *
Lisp_Print(LispMac *mac, LispObj *list, char *fname)
{
    LispPrint(mac, CAR(list));
    return (CAR(list));
}

LispObj *
Lisp_Prog1(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res = EVAL(CAR(list));
    LispObj *setf = mac->setf;
    int cdr = mac->cdr;

    for (list = CDR(list); list != NIL; list = CDR(list))
	(void)EVAL(CAR(list));

    mac->setf = setf;
    mac->cdr = cdr;

    return (res);
}

LispObj *
Lisp_Prog2(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res, *setf;
    int cdr;

    (void)EVAL(CAR(list));
    list = CDR(list);
    res = EVAL(CAR(list));
    setf = mac->setf;
    cdr = mac->cdr;

    for (list = CDR(list); list != NIL; list = CDR(list))
	(void)EVAL(CAR(list));

    mac->setf = setf;
    mac->cdr = cdr;

    return (res);
}

LispObj *
Lisp_Progn(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res = NIL;

    for (; list != NIL; list = CDR(list))
	res = EVAL(CAR(list));

    return (res);
}

LispObj *
Lisp_Provide(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *feat = CAR(list), *obj;

    if (feat->type != LispString_t)
	LispDestroy(mac, "cannot provide %s", LispStrObj(mac, feat));

    for (obj = MOD; obj != NIL; obj = CDR(obj)) {
	if (CAR(obj)->data.atom == feat->data.atom)
	    return (feat);
    }

    if (MOD == NIL)
	MOD = CONS(feat, NIL);
    else {
	CDR(MOD) = CONS(CAR(MOD), CDR(MOD));
	CAR(MOD) = feat;
    }

    return (feat);
}

LispObj *
Lisp_Push(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *obj, *var, *cons;

    if ((var = CAR(CDR(list)))->type != LispAtom_t)
	LispDestroy(mac, "cannot push to %s", LispStrObj(mac, var));

    var = EVAL(var);

    obj = EVAL(CAR(list));
    if (var == obj) {
	if (var->type == LispCons_t)
	    obj = CONS(CAR(var), CDR(var));
	else {
	    switch (var->type) {
		case LispNil_t:	
		    obj = NIL;
		    break;
		case LispTrue_t:
		    obj = T;
		    break;
		case LispAtom_t:
		    obj = ATOM(var->data.atom);
		    break;
		case LispString_t:
		    obj = STRING(var->data.atom);
		    break;
		case LispReal_t:
		    obj = REAL(var->data.real);
		    break;
		case LispQuote_t:
		    obj = QUOTE(var->data.quote);
		    break;
		case LispOpaque_t:
		    obj = OPAQUE(var->data.opaque.data, var->data.opaque.type);
		    break;
		default:
		    break;
	    }
	}
    }

    if (var->type == LispCons_t)
	cons = CONS(CAR(var), CDR(var));
    else {
	LispObj *car = NIL;

	switch (var->type) {
	    case LispNil_t:
		car = NIL;
		break;
	    case LispTrue_t:
		car = T;
		break;
	    case LispAtom_t:
		car = ATOM(var->data.atom);
		break;
	    case LispString_t:
		car = STRING(var->data.atom);
		break;
	    case LispReal_t:
		car = REAL(var->data.real);
		break;
	    case LispQuote_t:
		car = QUOTE(var->data.quote);
		break;
	    default:
		break;
	}

	cons = car;
	var->type = LispCons_t;
    }

    CAR(var) = obj;
    CDR(var) = cons;

    return (var);
}

LispObj *
Lisp_Quit(LispMac *mac, LispObj *list, char *fname)
{
    int status = 0;

    if (list != NIL) {
	if (CAR(list)->type != LispReal_t ||
	    (int)CAR(list)->data.real != CAR(list)->data.real)
	    LispDestroy(mac, "bad exit status argument %s, at %s",
			LispStrObj(mac, CAR(list)), fname);
	status = (int)CAR(list)->data.real;
    }

    exit(status);
}

LispObj *
Lisp_Quote(LispMac *mac, LispObj *list, char *fname)
{
    return (CAR(list));
}

LispObj *
Lisp_Reverse(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *res;

    switch (CAR(list)->type) {
	case LispNil_t:
	    return (NIL);
	    break;
	case LispCons_t:
	    break;
	default:
	    LispDestroy(mac, ExpectingListAt, fname);
	    /*NOTREACHED*/
    }

    res = NIL;
    list = CAR(list);
    while (list->type == LispCons_t && list != NIL) {
	res = CONS(CAR(list), res);
	list = CDR(list);
    }

    return (res);
}

LispObj *
Lisp_Rplaca(LispMac *mac, LispObj *list, char *fname)
{
    if (CAR(list)->type != LispCons_t)
	LispDestroy(mac, "%s is not of type cons, at %s",
		    LispStrObj(mac, CAR(list)), fname);

    CAR(CAR(list)) = CAR(CDR(list));

    return (CAR(list));
}

LispObj *
Lisp_Rplacd(LispMac *mac, LispObj *list, char *fname)
{
    if (CAR(list)->type != LispCons_t)
	LispDestroy(mac, "%s is not of type cons, at %s",
		    LispStrObj(mac, CAR(list)), fname);

    CDR(CAR(list)) = CAR(CDR(list));

    return (CAR(list));
}

LispObj *
Lisp_Set(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispSet(mac, CAR(list), CAR(CDR(list)), fname, 0));
}

LispObj *
Lisp_SetQ(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispSet(mac, CAR(list), CAR(CDR(list)), fname, 1));
}

/*
 * This setf implementation is not very restrictive about the `place' argument.
 * Any object reference is accepted. The code just makes sure that calls like
 * (setq a (list 1 2 3)) (setf (car (cdr (last a))) a)
 *  won't work, but calls like:
 * (setq a (list 1 2 3)) (setf (cdr (last a)) a)
 * will.
 */
LispObj *
Lisp_Setf(LispMac *mac, LispObj *list, char *fname)
{
    int count, cdr = 0;
    LispObj *cons, *place, *res = NIL, *resp, *sym, *obj = NIL, *setf;

    for (count = 0, place = list; place != NIL; count++, place = CDR(place))
	;
    if (count & 1)
	LispDestroy(mac, "odd number of arguments, at %s", fname);

    for (place = CAR(list), list = CDR(list);
	 ; place = CAR(list), list = CDR(list)) {
	if (place->type == LispAtom_t)
	    res = _LispSet(mac, place, CAR(list), fname, 1);
	else {
	    cons = NULL;

	    res = EVAL(place);

	    if (mac->setf == NULL || mac->setf == NIL) {
		setf = NULL;
		for (sym = SYM; sym != NIL; sym = CDR(sym)) {
		    if (CAR(sym)->data.symbol.obj == res) {
			cons = res;
			break;
		    }
		    else if (CAR(sym)->data.symbol.obj &&
			CAR(sym)->data.symbol.obj->type == LispCons_t) {
			obj = _LispFindPlace(mac, CAR(sym)->data.symbol.obj, res);
			if (obj != NULL) {
			    cons = obj;
			    break;
			}
		    }
		}
		if (cons == NULL || res == NIL)
		    LispDestroy(mac, "%s is an invalid %s place",
				LispStrObj(mac, res), fname);
	    }
	    else {
		setf = mac->setf;
		cdr = mac->cdr;
	    }

	    resp = res;
	    res = EVAL(CAR(list));
	    if (setf && setf != NIL) {
		if (cdr)
		    CDR(setf) = res;
		else
		    CAR(setf) = res;
	    }
	    else {
		if (cons == NULL)
		    LispDestroy(mac, "internal error, at internal:setf");
		switch (cons->type) {
		    case LispSymbol_t:
			cons->data.symbol.obj = res;
			break;
		    case LispCons_t:
			if (CAR(cons) == CAR(obj)) {
			    CAR(cons) = res;
			    break;
			}
			else if (CDR(cons) == CDR(obj)) {
			    CDR(cons) = res;
			    break;
			}
			/*FALLTHROUGH*/
		    default:
			LispDestroy(mac, "internal error, at internal:setf");
			/*NOTREACHED*/
		}
	    }
	}
	if ((list = CDR(list)) == NIL)
	    break;
    }

    return (res);
}

LispObj *
Lisp_Stringp(LispMac *mac, LispObj *list, char *fname)
{
    return (CAR(list)->type == LispString_t ? T : NIL);
}

LispObj *
Lisp_Symbolp(LispMac *mac, LispObj *list, char *fname)
{
    switch (CAR(list)->type) {
	case LispNil_t:
	case LispTrue_t:
	case LispAtom_t:
	case LispSymbol_t:
	case LispLambda_t:
	    return (T);
	default:
	    return (NIL);
    }
    /*NOTREACHED*/
}

LispObj *
Lisp_SymbolPlist(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *sym = CAR(list);

    if (sym == NIL || sym == T)
	return (sym);
    else if (sym->type != LispAtom_t)
	LispDestroy(mac, "%s is not a symbol, at %s",
		    LispStrObj(mac, sym), fname);
    if ((sym = LispGetVar(mac, sym->data.atom, 1)) == NULL)
	return (NIL);

    return (sym->data.symbol.plist);
}

LispObj *
Lisp_Throw(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *tag = EVAL(CAR(list));
    unsigned blevel = mac->block.block_level;

    if (blevel == 0)
	LispDestroy(mac, "%s called not within a block", fname);

    while (blevel) {
	int jmp = 1;
	LispBlock *block = &(mac->block.block[--blevel]);

	if (tag->type == block->tag.type) {
	    switch(tag->type) {
		case LispNil_t:
		case LispTrue_t:
		    break;
		case LispAtom_t:
		case LispString_t:
		    jmp = tag->data.atom == block->tag.data.atom;
		    break;
		case LispReal_t:
		    jmp = tag->data.real == block->tag.data.real;
		    break;
		default:
		    jmp = memcmp(tag, &(block->tag), sizeof(LispObj)) == 0;
		    break;
	    }
	    if (jmp) {
		mac->block.block_ret = EVAL(CAR(CDR(list)));
		longjmp(block->jmp, 0);
	    }
	}
    }
    LispDestroy(mac, "%s is not a tag to %s", LispStrObj(mac, tag), fname);
    /*NOTREACHED*/

    return (NIL);
}

LispObj *
Lisp_Typep(LispMac *mac, LispObj *list, char *fname)
{
    LispType type = LispStruct_t;
    LispObj *obj;
    char *atom = NULL;

    obj = CAR(CDR(list));
    if (obj == NIL || obj == T)
	return (obj);
    else if (obj->type != LispAtom_t)
	LispDestroy(mac, "%s is a bad type specification, at %s",
		    LispStrObj(mac, obj), fname);
    else {
	atom = obj->data.atom;
	if (strcmp(atom, "atom") == 0)
	    type = LispAtom_t;
	else if (strcmp(atom, "real") == 0)
	    type = LispReal_t;
	else if (strcmp(atom, "list") == 0)
	    type = LispCons_t;
	else if (strcmp(atom, "string") == 0)
	    type = LispString_t;
	else if (strcmp(atom, "opaque") == 0)
	    type = LispOpaque_t;
    }

    obj = CAR(list);
    if (type != LispStruct_t && obj->type == type)
	return (T);
    else if (obj->type == LispStruct_t)
	return (CAR(obj->data.struc.def)->data.atom == atom ? T : NIL);

    return (NIL);
}

LispObj *
Lisp_Unless(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispWhenUnless(mac, list, 0));
}

LispObj *
Lisp_Vector(LispMac *mac, LispObj *list, char *fname)
{
    int count;
    LispObj *dim, *ary = list, *obj;

    for (count = 0; list != NIL; count++, list = CDR(list))
	;
    dim = CONS(REAL((double)count), NIL);

    obj = LispNew(mac, ary, dim);
    obj->type = LispArray_t;
    obj->data.array.list = ary;
    obj->data.array.dim = dim;
    obj->data.array.rank = 1;
    obj->data.array.type = LispTrue_t;
    obj->data.array.zero = count == 0;

    return (obj);
}

LispObj *
Lisp_When(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispWhenUnless(mac, list, 1));
}

LispObj *
Lisp_Until(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispWhileUntil(mac, list, 0));
}

LispObj *
Lisp_While(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispWhileUntil(mac, list, 1));
}

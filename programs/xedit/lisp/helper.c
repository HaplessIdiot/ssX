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

/* $XFree86: xc/programs/xedit/lisp/helper.c,v 1.6 2001/09/28 04:38:31 paulo Exp $ */

#include "helper.h"
#include <ctype.h>

/*
 * Implementation
 */
LispObj *
_LispEqual(LispMac *mac, LispObj *left, LispObj *right)
{
    LispObj *res = NIL;

    if (left->type == right->type) {
	switch (left->type) {
	    case LispNil_t:
	    case LispTrue_t:
		res = T;
		break;
	    case LispReal_t:
		if (left->data.real == right->data.real)
		    res = T;
		break;
	    case LispAtom_t:
	    case LispString_t:
		if (left->data.atom == right->data.atom)
		    res = T;
		break;
	    case LispCons_t:
		if (_LispEqual(mac, CAR(left), CAR(right)) == T &&
		    _LispEqual(mac, CDR(left), CDR(right)) == T)
		    res = T;
		break;
	    case LispQuote_t:
		res = _LispEqual(mac, left->data.quote, right->data.quote);
		break;
	    case LispSymbol_t:
		if (left->data.symbol.name == right->data.symbol.name)
		    res = T;
		break;
	    case LispLambda_t:
		if (left->data.lambda.name == right->data.lambda.name)
		    res = T;
		break;
	    case LispOpaque_t:
		if (left->data.opaque.data == right->data.opaque.data)
		    res = T;
		break;
	}
    }

    return (res);
}

LispObj *
_LispNth(LispMac *mac, LispObj *list, char *name, int cdr)
{
    int count;
    LispObj *nth = CAR(list), *seq = CDR(list), *setf = list;

    if (nth->type != LispReal_t || nth->data.real < 0 ||
	(int)nth->data.real != nth->data.real)
	LispDestroy(mac, "bad index %s, at %s", LispStrObj(mac, nth), name);
    if (seq->type != LispCons_t)
	LispDestroy(mac, "%s is not of type list, at %s",
		    LispStrObj(mac, seq), name);
    for (count = 0, seq = CAR(seq);
	 count < nth->data.real && seq->type == LispCons_t;
	 ++count, setf = seq, seq = CDR(seq))
	;

    if (count == nth->data.real) {
	mac->cdr = cdr;
	mac->setf = cdr ? setf : seq;
	return ((cdr || seq == NIL) ? seq : CAR(seq));
    }
    return (NIL);
}

LispObj *
_LispFindPlace(LispMac *mac, LispObj *list, LispObj *ref)
{
    LispObj *place;

    for (; list->type == LispCons_t; list = CDR(list)) {
	if (CAR(list) == ref)
	    return (list);
	else if (CDR(list) == ref)
	    return (list);
	else if ((place = _LispFindPlace(mac, CAR(list), ref)) != NULL)
	    return (place);
    }

    return (NULL);
}

LispObj *
_LispMinMax(LispMac *mac, LispObj *list, char *name, int max)
{
    double real;
    LispObj *obj;

    obj = EVAL(CAR(list));
    if (obj->type != LispReal_t)
	LispDestroy(mac, ExpectingNumberAt, name);
    real = obj->data.real;
    for (list = CDR(list); list != NIL; list = CDR(list)) {
	obj = EVAL(CAR(list));
	if (obj->type != LispReal_t)
	    LispDestroy(mac, ExpectingNumberAt, name);
	else if ((max && obj->data.real > real) ||
	     (!max && obj->data.real < real))
	    real = obj->data.real;
    }

    return (REAL(real));
}

LispObj *
_LispBoolCond(LispMac *mac, LispObj *list, char *name, int op)
{
    LispObj *obj;
    double value;
    int cond;

    cond = 1;
    obj = EVAL(CAR(list));
    if (obj->type != LispReal_t)
	LispDestroy(mac, ExpectingNumberAt, name);
    value = obj->data.real;

    for (list = CDR(list); list != NIL; list = CDR(list)) {
	obj = EVAL(CAR(list));
	if (obj->type != LispReal_t)
	    LispDestroy(mac, ExpectingNumberAt, name);
	switch (op) {
	    case LESS:
		if (value >= obj->data.real)
		    cond = 0;
		break;
	    case LESS_EQUAL:
		if (value > obj->data.real)
		    cond = 0;
		break;
	    case EQUAL:
		if (value != obj->data.real)
		    cond = 0;
		break;
	    case GREATER:
		if (value <= obj->data.real)
		    cond = 0;
		break;
	    case GREATER_EQUAL:
		if (value < obj->data.real)
		    cond = 0;
		break;
	}
	if (!cond)
	    break;
	value = obj->data.real;
    }
    return (cond ? T : NIL);
}

LispObj *
_LispDefLambda(LispMac *mac, LispObj *list, LispFunType type)
{
    LispObj *name = NIL, *args, *code, *obj = NIL, *fun;
    static char *types[3] = {"LAMBDA", "FUNCTION", "MACRO"};
    static char *fnames[3] = {"LAMBDA", "DEFUN", "DEFMACRO"};
    int num_args, rest, optional, key;

    /* name */
    if (type != LispLambda) {
	if ((name = CAR(list))->type != LispAtom_t)
	    LispDestroy(mac, "%s cannot be a %s name, at %s",
			LispStrObj(mac, name), types[type], fnames[type]);

	if (LispFindBuiltin(name->data.atom, strlen(name->data.atom)) != NULL)
	    LispDestroy(mac, "cannot redefine %s", name->data.atom);

	list = CDR(list);
    }

    /* args */
    args = CAR(list);
    num_args = rest = optional = key = 0;

    if (args->type == LispCons_t) {
	for (obj = args; obj != NIL; obj = CDR(obj), ++num_args)
	    if (CAR(obj)->type == LispCons_t && (key || optional)) {
		/* is this a default value? */
		if (CAR(CAR(obj))->type != LispAtom_t)
		    LispDestroy(mac, "%s cannot be a %s argument name, at %s %s",
				LispStrObj(mac, CAR(CAR(obj))), types[type],
				fnames[type],
				type == LispLambda ? "..." : name->data.atom);
		else if (CDR(CAR(obj)) != NIL &&
			 (CDR(CAR(obj))->type != LispCons_t ||
			  CDR(CDR(CAR(obj))) != NIL))
		    LispDestroy(mac, "bad argument specification %s, at %s %s",
				LispStrObj(mac, CAR(obj)), types[type],
				fnames[type],
				type == LispLambda ? "..." : name->data.atom);
	    }
	    else if (CAR(obj)->type != LispAtom_t ||
		CAR(obj)->data.atom[0] == ':')
		LispDestroy(mac, "%s cannot be a %s argument name, at %s %s",
			    LispStrObj(mac, CAR(obj)), types[type], fnames[type],
			    type == LispLambda ? "..." : name->data.atom);
	    else if (CAR(obj)->data.atom[0] == '&') {
		if (strcmp(CAR(obj)->data.atom + 1, "REST") == 0) {
		    if (rest || CDR(obj) == NIL || CDR(CDR(obj)) != NIL)
			LispDestroy(mac, "syntax error parsing &REST,"
				    " at %s %s", fnames[type],
				    type == LispLambda ?
				    "..." : name->data.atom);
		    rest = 1;
		}
		else if (strcmp(CAR(obj)->data.atom + 1, "KEY") == 0) {
		    if (rest)
			LispDestroy(mac, "&KEY not allowed after &REST,"
				    " at %s %s", fnames[type],
				    type == LispLambda ?
				    "..." : name->data.atom);
		    if (key || optional || CDR(obj) == NIL)
			LispDestroy(mac, "syntax error parsing &KEY,"
				    " at %s %s", fnames[type],
				    type == LispLambda ?
				    "..." : name->data.atom);
		    key = 1;
		}
		else if (strcmp(CAR(obj)->data.atom + 1, "OPTIONAL") == 0) {
		    if (rest)
			LispDestroy(mac, "&OPTIONAL not allowed after &REST,"
				    " at %s %s", fnames[type],
				    type == LispLambda ?
				    "..." : name->data.atom);
		    if (key || optional || CDR(obj) == NIL)
			LispDestroy(mac, "syntax error parsing &OPTIONAL,"
				    " at %s %s", fnames[type],
				    type == LispLambda ?
				    "..." : name->data.atom);
		    optional = 1;
		}
		else
		    LispDestroy(mac, "%s not allowed %at %s %s",
				CAR(obj)->data.atom, fnames[type],
				type == LispLambda ? "..." : name->data.atom);
	    }
    }
    else if (args != NIL)
	LispDestroy(mac, "%s cannot be a %s argument list, at %s %s",
		    LispStrObj(mac, args), types[type], fnames[type],
		    type == LispLambda ? "..." : name->data.atom);

    /* code */
    code = CDR(list);

    if (type != LispLambda) {
	for (obj = FUN; obj != NIL; obj = CDR(obj))
	    if (CAR(obj)->data.lambda.name == name->data.atom) {
		fprintf(lisp_stderr, "*** Warning: %s is being redefined\n",
			name->data.atom);
		break;
	    }
    }

    GCProtect();
    fun = LispNewLambda(mac, type == LispLambda ? NULL : name->data.atom,
			args, code, num_args, type, key, optional, rest);

    if (type != LispLambda) {
	if (obj != NIL)
	    CAR(obj) = fun;
	else if (FUN == NIL)
	    FUN = CONS(fun, NIL);
	else {
	    CDR(FUN) = CONS(CAR(FUN), CDR(FUN));
	    CAR(FUN) = fun;
	}
	GCUProtect();

	return (name);
    }
    GCUProtect();

    return (fun);
}

LispObj *
_LispSet(LispMac *mac, LispObj *var, LispObj *val, char *fname, int eval)
{
    char *name;

    if (var->type != LispAtom_t)
	LispDestroy(mac, "%s is not a symbol, at %s",
		    LispStrObj(mac, var), fname);

    name = var->data.atom;
    if (isdigit(name[0]) || name[0] == '(' || name[0] == ')'
	|| name[0] == ';' || name[0] == '\'' || name[0] == '#')
	LispDestroy(mac, "bad name %s, at %s", name, fname);
    if (eval)
	val = EVAL(val);

    return (LispSetVar(mac, name, val, 0));
}

LispObj *
_LispWhenUnless(LispMac *mac, LispObj *list, int op)
{
    LispObj *obj, *res = NIL;

    obj = EVAL(CAR(list));
    if ((obj->type == LispNil_t) ^ op) {
	for (obj = CDR(list); obj != NIL; obj = CDR(obj))
	    res = EVAL(CAR(obj));
    }
    return (res);
}

LispObj *
_LispWhileUntil(LispMac *mac, LispObj *list, int op)
{
    LispObj *obj, *res = NIL;

    /*CONSTCOND*/
    while (1) {
	obj = EVAL(CAR(list));
	if ((obj->type == LispNil_t) ^ op) {
	    for (obj = CDR(list); obj != NIL; obj = CDR(obj))
		res = EVAL(CAR(obj));
	}
	else
	    break;
    }
    return (res);
}

LispObj *
_LispLoadFile(LispMac *mac, char *filename, char *fname,
	      int verbose, int print, int ifdoesnotexist)
{
    LispObj *obj, *res = NIL;
    FILE *fp;
    int level;

    if ((fp = fopen(filename, "r")) == NULL) {
	if (ifdoesnotexist)
	    LispDestroy(mac, "cannot open %s, at %s", filename, fname);
	return (NIL);
    }

    if (verbose)
	fprintf(lisp_stderr, "; Loading %s\n", filename);

    if (mac->stream.stream_level + 1 >= mac->stream.stream_size) {
	LispStream *stream = (LispStream*)
	    realloc(mac->stream.stream, sizeof(LispStream) *
		    (mac->stream.stream_size + 1));

	if (stream == NULL) {
	    fclose(fp);
	    LispDestroy(mac, "out of memory");
	}

	mac->stream.stream = stream;
	++mac->stream.stream_size;
    }
    mac->stream.stream[mac->stream.stream_level].fp = mac->fp;
    mac->stream.stream[mac->stream.stream_level].st = mac->st;
    mac->stream.stream[mac->stream.stream_level].cp = mac->cp;
    mac->stream.stream[mac->stream.stream_level].tok = mac->tok;
    ++mac->stream.stream_level;
    memset(mac->stream.stream + mac->stream.stream_level, 0, sizeof(LispStream));
    mac->stream.stream[mac->stream.stream_level].fp = fp;
    mac->fp = fp;
    mac->st = mac->cp = NULL;
    mac->tok = 0;

    level = mac->level;
    mac->level = 0;
    /*CONSTCOND*/
    while (1) {
	if ((obj = LispRun(mac)) != NULL) {
	    GCProtect();
	    res = EVAL(obj);
	    if (print)
		LispPrint(mac, res);
	    GCUProtect();
	}
	if (mac->tok == EOF)
	    break;
    }
    mac->level = level;
    free(mac->st);
    --mac->stream.stream_level;

    mac->fp = mac->stream.stream[mac->stream.stream_level].fp;
    mac->st = mac->stream.stream[mac->stream.stream_level].st;
    mac->cp = mac->stream.stream[mac->stream.stream_level].cp;
    mac->tok = mac->stream.stream[mac->stream.stream_level].tok;

    return (res);
}

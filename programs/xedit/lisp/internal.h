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

/* $XFree86$ */

#ifndef Lisp_internal_h
#define Lisp_internal_h

#include "lisp.h"

/*
 * Defines
 */
#define	CAR(list)		(list->data.cons.car)
#define CDR(list)		(list->data.cons.cdr)
#define CONS(car, cdr)		LispNewCons(mac, car, cdr)
#define EVAL(list)		LispEval(mac, list)
#define ATOM(atom)		LispNewAtom(mac, atom)
#define ATOM2(atom)		LispNewAtom(mac, LispGetString(mac, atom))
#define QUOTE(quote)		LispNewQuote(mac, quote)
#define REAL(num)		LispNewReal(mac, num)
#define STRING(str)		LispNewString(mac, str)
#define OPAQUE(data, type)	LispNewOpaque(mac, (void*)((long)data), type)
#define CHECKO(obj, typ)						\
	obj->type == LispOpaque_t && 					\
	(obj->data.opaque.type == typ || obj->data.opaque.type == 0)
#define GCPRO()			LispGC(mac, NIL, NIL); GCProtect()
#define GCUPRO()		GCUProtect()
#define PROTECT(list)		list->prot = LispTrue_t
#define UPROTECT(list)		list->prot = LispNil_t

#define	GCProtect()		++gcpro
#define	GCUProtect()		--gcpro

/*
 * Types
 */
typedef struct _LispObj LispObj;
typedef struct _LispBuiltin LispBuiltin;
typedef struct _LispModuleData LispModuleData;

typedef enum _LispType {
    LispNil_t,
    LispTrue_t,
    LispAtom_t,
    LispReal_t,
    LispCons_t,
    LispQuote_t,
    LispString_t,
    LispSymbol_t,
    LispLambda_t,
    LispArray_t,
    LispStruct_t,
    LispOpaque_t
} LispType;

typedef enum _LispFunType {
    LispLambda,
    LispFunction,
    LispMacro
} LispFunType;

struct _LispObj {
    unsigned int type : 6;
    unsigned int mark : 1;	/* gc protected */
    unsigned int dirty : 1;
    unsigned int prot: 1;	/* protection for constant/unamed variables */
    union {
	char *atom;
	double real;
	LispObj *quote;
	struct {
	    LispObj *car;
	    LispObj *cdr;
	} cons;
	struct {
	    char *name;
	    LispObj *obj;
	    LispObj *plist;
	} symbol;
	struct {
	    char *name;		/* if name is NULL, it is a lambda expression */
	    LispObj *code;
	    LispFunType num_args : 30;
	    LispFunType type : 2;
	} lambda;
	struct {
	    LispObj *list;	/* stored as a linear list */
	    LispObj *dim;	/* dimensions of array */
	    int rank : 8;	/* i.e. array-rank-limit => 256 */
	    int type : 7;	/* converted to LispType, if not Lisp{Nil,True}_t
				 * only accepts given type in array fields */
	    int zero : 1;	/* at least one of the dimensions is zero */
	} array;
	struct {
	    LispObj *fields;	/* structure fields */
	    LispObj *def;	/* structure definition */
	} struc;
	struct {
	    void *data;
	    int type;
	} opaque;
    } data;
};

struct _LispBuiltin {
    char *name;
    LispObj *(*fn)(LispMac*, LispObj*, char*);
    int eval : 1;
    int min_args : 15;
    int max_args : 15;
};

typedef	LispObj *(*LispFunPtr)(LispMac*, LispObj*, char*);
typedef LispBuiltin *(*LispFindFunPtr)(char*, int);
typedef int (*LispLoadModule)(LispMac*);
typedef int (*LispUnloadModule)(LispMac*);

struct _LispModuleData {
    LispFindFunPtr find_fun;
    LispLoadModule load;
    LispUnloadModule unload;
};

/*
 * Prototypes
 */
LispObj *LispEval(LispMac*, LispObj*);

LispObj *LispNew(LispMac*, LispObj*, LispObj*);
LispObj *LispNewNil(LispMac*);
LispObj *LispNewTrue(LispMac*);
LispObj *LispNewAtom(LispMac*, char*);
LispObj *LispNewReal(LispMac*, double);
LispObj *LispNewString(LispMac*, char*);
LispObj *LispNewQuote(LispMac*, LispObj*);
LispObj *LispNewCons(LispMac*, LispObj*, LispObj*);
LispObj *LispNewSymbol(LispMac*, char*, LispObj*);
LispObj *LispNewLambda(LispMac*, char*, LispObj*, LispObj*, unsigned, unsigned);
LispObj *LispNewStruct(LispMac*, LispObj*, LispObj*);
LispObj *LispNewOpaque(LispMac*, void*, int);

char *LispGetString(LispMac*, char*);

void *LispMalloc(LispMac*, unsigned);
void *LispCalloc(LispMac*, unsigned, unsigned);
void *LispRealloc(LispMac*, void*, unsigned);
char *LispStrdup(LispMac*, char*);
void LispFree(LispMac*, void*);

void LispGC(LispMac*, LispObj*, LispObj*);

char *LispStrObj(LispMac*, LispObj*);

void LispDestroy(LispMac *mac, char *fmt, ...);

LispObj *LispSetVariable(LispMac*, LispObj*, LispObj*, char*, int);

int LispRegisterOpaqueType(LispMac*, char*);

/*
 * Initialization
 */
extern LispObj *NIL, *T;
extern int gcpro;

#endif /* Lisp_internal_h */

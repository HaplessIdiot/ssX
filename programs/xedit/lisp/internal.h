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

/* $XFree86: xc/programs/xedit/lisp/internal.h,v 1.13 2002/02/08 02:59:29 paulo Exp $ */

#ifndef Lisp_internal_h
#define Lisp_internal_h

#include <stdio.h>
#include "lisp.h"

#include "mp.h"

/*
 * Defines
 */
#define STREAM_READ		1
#define STREAM_WRITE		2

#define	CAR(list)		((list)->data.cons.car)
#define	CAAR(list)		((list)->data.cons.car->data.cons.car)
#define	CADR(list)		((list)->data.cons.cdr->data.cons.car)
#define CDR(list)		((list)->data.cons.cdr)
#define CDAR(list)		((list)->data.cons.car->data.cons.cdr)
#define CONS(car, cdr)		LispNewCons(mac, car, cdr)
#define EVAL(list)		LispEval(mac, list)
#define APPLY(fun, args)	LispApply(mac, fun, args)
#define EXECUTE(string)		LispExecute(mac, string)
#define SYMBOL(atom)		LispNewSymbol(mac, atom)
#define ATOM(string)		LispNewAtom(mac, string)

	/* atom string is a static variable */
#define ATOM2(string)		LispNewSymbol(mac, LispGetPermAtom(mac, string))

	/* make a gc never released variable with a static string argument */
#define STATIC_ATOM(string)	LispNewStaticAtom(mac, string)

#define QUOTE(quote)		LispNewQuote(mac, quote)
#define BACKQUOTE(bquote)	LispNewBackquote(mac, bquote)
#define COMMA(comma, at)	LispNewComma(mac, comma, at)
#define REAL(num)		LispNewReal(mac, num)
#define STRING(str)		LispNewString(mac, str)

	/* string must be from the LispXXX allocation functions,
	 * and LispMused not yet called on it */
#define STRING2(str)		LispNewAllocedString(mac, str)

#define CHAR(c)			LispNewCharacter(mac, c)
#define INTEGER(i)		LispNewInteger(mac, i)
#define RATIO(n, d)		LispNewRatio(mac, n, d)
#define VECTOR(objects)		LispNewVector(mac, objects)
#define COMPLEX(r, i)		LispNewComplex(mac, r, i)
#define OPAQUE(data, type)	LispNewOpaque(mac, (void*)((long)data), type)
#define KEYWORD(key)		LispNewKeyword(mac, key)
#define PATHNAME(p)		LispNewPathname(mac, p)
#define STRINGSTREAM(str, flag)	LispNewStringStream(mac, str, flag)
#define FILESTREAM(file, path, flag)	\
	LispNewFileStream(mac, file, path, flag)
#define PIPESTREAM(file, path, flag)	\
	LispNewPipeStream(mac, file, path, flag)

#define BIGINTEGER(i)		LispNewBigInteger(mac, i)
#define BIGRATIO(r)		LispNewBigRational(mac, r)

#define CHECKO(obj, typ)						\
	((obj)->type == LispOpaque_t && 				\
	 ((obj)->data.opaque.type == typ || (obj)->data.opaque.type == 0))
#define PROTECT(key, list)	LispProtect(mac, key, list)
#define UPROTECT(key, list)	LispUProtect(mac, key, list)

/* create a new unique static atom string */
#define GETATOMID(string)	LispGetAtomString(mac, string, 1)

#define	GCProtect()		++gcpro
#define	GCUProtect()		--gcpro

/* pointer to string of a LispAtom_t object */
#define	STRPTR(obj)		(obj)->data.atom->string

/* pointer to something unique to all atoms with the same print representation */
#define ATOMID(obj)		(obj)->data.atom->string

/* pointer to string of a LispString_t object */
#define THESTR(obj)		(obj)->data.string

#define INT_P(obj)		((obj)->type == LispInteger_t)
#define FLOAT_P(obj)		((obj)->type == LispReal_t)
#define RATIO_P(obj)		((obj)->type == LispRatio_t)

#define BIGINT_P(obj)		((obj)->type == LispBigInteger_t)
#define BIGRATIO_P(obj)		((obj)->type == LispBigRatio_t)

#define FIXNUM_P(obj)							\
	(INT_P(obj) || RATIO_P(obj) || FLOAT_P(obj))

/* assumes FIXNUM_P is true */
#define FIXNUM_VALUE(obj)						\
	(INT_P(obj) ? (obj)->data.integer :				\
	 FLOAT_P(obj) ? (obj)->data.real :				\
	 (double)((obj)->data.ratio.numerator) / 			\
	 (double)((obj)->data.ratio.denominator))

#define INTEGER_P(obj)					\
	(INT_P(obj) || BIGINT_P(obj))

#define RATIONAL_P(obj)					\
	(INT_P(obj) || RATIO_P(obj) ||			\
	 BIGINT_P(obj) || BIGRATIO_P(obj))
#define REAL_P(obj)					\
	(FIXNUM_P(obj) || BIGINT_P(obj) ||		\
	 BIGRATIO_P(obj))
#define NUMBER_P(obj)					\
	(FIXNUM_P(obj) || BIGINT_P(obj) ||		\
	 BIGRATIO_P(obj) || COMPLEX_P(obj))

#define INTEGRAL_P(obj)					\
	(INT_P(obj) || (FLOAT_P(obj) &&			\
	 (long)(obj)->data.real == (obj)->data.real))

/* positive integer */
#define INDEX_P(obj)		(INT_P(obj) && obj->data.integer >= 0)

#define SYMBOL_P(obj)		((obj)->type == LispAtom_t)
#define STRING_P(obj)		((obj)->type == LispString_t)
#define CHAR_P(obj)		((obj)->type == LispCharacter_t)
#define COMPLEX_P(obj)		((obj)->type == LispComplex_t)
#define CONS_P(obj)		((obj)->type == LispCons_t)
#define KEYWORD_P(obj)		((obj)->type == LispKeyword_t)
#define PATHNAME_P(obj)		((obj)->type == LispPathname_t)

#define STREAM_P(obj)		((obj)->type == LispStream_t)

#define SSTREAMP(str)		((str)->data.stream.source.string)

#define FSTREAMP(str)		((str)->data.stream.source.file)

#define PSTREAMP(str)		((str)->data.stream.source.program)
#define PIDPSTREAMP(str)	((str)->data.stream.source.program->pid)
#define IPSTREAMP(str)		((str)->data.stream.source.program->input)
#define OPSTREAMP(str)		((str)->data.stream.source.program->output)
#define EPSTREAMP(str)		\
	FSTREAMP((str)->data.stream.source.program->errorp)

#define PACKAGE_P(object)	((object)->type == LispPackage_t)

#define LispFileno(file)	((file)->descriptor)

#define STRFUN(builtin)		STRPTR(CAR(builtin->description))
#define STROBJ(obj)		LispStrObj(mac, obj)

#define CONSTANT_P(obj)					\
    ((obj)->type < LispAtom_t || KEYWORD_P(obj))

/* slightly faster test, since keywords are very uncommon as eval arguments */
#define NCONSTANT_P(obj)				\
    ((obj)->type >= LispAtom_t)

/* fetch builtin function/macro argument value
 */
#define ARGUMENT(index)					\
	mac->env.pairs[mac->env.base + ((index) << 1)]

/* fetch argument name for builtin functions
 */
#define ARGUMENT_NAME(index)				\
	mac->env.pairs[mac->env.base + ((index) << 1) + 1]

/* unbound builtin macro arguments, but keep objects gc protected,
 * avoid name clashes
*/
#define MACRO_ARGUMENT1()				\
	mac->env.pairs[mac->env.base + 1] = NIL
#define MACRO_ARGUMENT2()				\
	mac->env.pairs[mac->env.base + 1] =		\
	    mac->env.pairs[mac->env.base + 3] = NIL
#define MACRO_ARGUMENT3()				\
	mac->env.pairs[mac->env.base + 1] =		\
	  mac->env.pairs[mac->env.base + 3] =		\
	    mac->env.pairs[mac->env.base + 5] = NIL
#define MACRO_ARGUMENT4()				\
	mac->env.pairs[mac->env.base + 1] =		\
	  mac->env.pairs[mac->env.base + 3] =		\
	    mac->env.pairs[mac->env.base + 5] =		\
		mac->env.pairs[mac->env.base + 7] = NIL
#define MACRO_ARGUMENTS(count)				\
    {							\
	int i = (count << 1) + mac->env.base + 1;	\
	for (; i > mac->env.base; i -= 2)		\
	    mac->env.pairs[i] = NIL;			\
    }

/*
 * Types
 */
typedef struct _LispObj LispObj;
typedef struct _LispAtom LispAtom;
typedef struct _LispBuiltin LispBuiltin;
typedef struct _LispModuleData LispModuleData;
typedef struct _LispFile LispFile;
typedef struct _LispString LispString;
typedef struct _LispPackage LispPackage;

typedef char *Atom_id;

typedef enum _LispType {
	/* simple types, self contained objects first */
    LispNil_t,
    LispTrue_t,
    LispInteger_t,
    LispReal_t,
    LispCharacter_t,
    LispString_t,
    LispRatio_t,
    LispOpaque_t,

	/* non simple types, like streams, need special cleanup in gc,
	 * but simple access for marking */
    LispBigInteger_t,
    LispBigRatio_t,

	/* self contained for GC, but not for eval */
    LispAtom_t,

	/* almost a simple object, holds a pointer to an atom */
    LispKeyword_t,

	/* not a simple object, but name field is either nil or an atom */
    LispLambda_t,

	/* a cons of two numbers */
    LispComplex_t,
    LispCons_t,
    LispQuote_t,
    LispArray_t,
    LispStruct_t,
    LispStream_t,
    LispBackquote_t,
    LispComma_t,
    LispPathname_t,
    LispPackage_t
} LispType;

typedef enum _LispFunType {
    LispLambda,
    LispFunction,
    LispMacro,
    LispSetf
} LispFunType;

typedef enum _LispStreamType {
    LispStreamString,
    LispStreamFile,
    LispStreamStandard,
    LispStreamPipe
} LispStreamType;

typedef struct {
    int pid;			/* process id of program */
    LispFile *input;		/* if READABLE: stdout of program */
    LispFile *output;		/* if WRITABLE: stdin of program */
    LispObj *errorp;		/* ALWAYS (ONLY) READABLE: stderr of program */
} LispPipe;

struct _LispObj {
    LispType type : 6;
    unsigned int mark : 1;	/* gc protected */
    unsigned int prot: 1;	/* protection for constant/unamed variables */
    union {
	LispAtom *atom;
	char *string;		/* will be converted to a more complete
				 * type at some time*/
	long integer;
	double real;
	LispObj *quote;
	struct {
	    long numerator;
	    long denominator;
	} ratio;
	union {
	    mpi *integer;
	    mpr *ratio;
	} mp;
	struct {
	    LispObj *real;
	    LispObj *imag;
	} complex;
	struct {
	    LispObj *car;
	    LispObj *cdr;
	} cons;
	struct {
	    LispObj *name;
	    LispObj *code;
	    LispFunType type;
	} lambda;
	struct {
	    LispObj *list;		/* stored as a linear list */
	    LispObj *dim;		/* dimensions of array */
	    unsigned int rank : 8;	/* i.e. array-rank-limit => 256 */
	    unsigned int type : 7;	/* converted to LispType, if not
					 * Lisp{Nil,True}_t only accepts given
					 * type in array fields */
	    unsigned int zero : 1;	/* at least one of the dimensions
					 * is zero */
	} array;
	struct {
	    LispObj *fields;	/* structure fields */
	    LispObj *def;	/* structure definition */
	} struc;
	struct {
	    union {
		LispFile *file;
		LispPipe *program;
		LispString *string;
	    } source;
	    LispObj *pathname;
	    LispStreamType type : 6;
	    int readable : 1;
	    int writable : 1;
	} stream;
	struct {
	    void *data;
	    int type;
	} opaque;
	struct {
	    LispObj *eval;
	    int atlist;
	} comma;
	struct {
	    LispObj *name;
	    LispObj *nicknames;
	    LispPackage *package;
	} package;
    } data;
};

typedef	LispObj *(*LispFunPtr)(LispMac*, LispBuiltin*);

struct _LispBuiltin {
    /* these fields must be set */
    LispFunType type;
    LispFunPtr function;
    char *declaration;

    /* this field is optional, set if the function should not be exported */
    int internal;

    /* this field is set at runtime */
    LispObj *description;
};

typedef int (*LispLoadModule)(LispMac*);
typedef int (*LispUnloadModule)(LispMac*);

#define LISP_MODULE_VERSION		1
struct _LispModuleData {
    int version;
    LispLoadModule load;
    LispUnloadModule unload;
};

/*
 * Prototypes
 */
LispObj *LispEval(LispMac*, LispObj*);
LispObj *LispApply(LispMac*, LispObj*, LispObj*);

LispObj *LispNew(LispMac*, LispObj*, LispObj*);
LispObj *LispNewSymbol(LispMac*, LispAtom*);
LispObj *LispNewAtom(LispMac*, char*);
LispObj *LispNewStaticAtom(LispMac*, char*);
LispObj *LispNewReal(LispMac*, double);
LispObj *LispNewString(LispMac*, char*);
LispObj *LispNewAllocedString(LispMac*, char*);
LispObj *LispNewCharacter(LispMac*, long);
LispObj *LispNewInteger(LispMac*, long);
LispObj *LispNewRatio(LispMac*, long, long);
LispObj *LispNewVector(LispMac*, LispObj*);
LispObj *LispNewQuote(LispMac*, LispObj*);
LispObj *LispNewBackquote(LispMac*, LispObj*);
LispObj *LispNewComma(LispMac*, LispObj*, int);
LispObj *LispNewCons(LispMac*, LispObj*, LispObj*);
LispObj *LispNewLambda(LispMac*, LispObj*, LispObj*, LispObj*, LispFunType);
LispObj *LispNewStruct(LispMac*, LispObj*, LispObj*);
LispObj *LispNewComplex(LispMac*, LispObj*, LispObj*);
LispObj *LispNewOpaque(LispMac*, void*, int);
LispObj *LispNewKeyword(LispMac*, LispObj*);
LispObj *LispNewPathname(LispMac*, LispObj*);
LispObj *LispNewStringStream(LispMac*, unsigned char*, int);
LispObj *LispNewFileStream(LispMac*, LispFile*, LispObj*, int);
LispObj *LispNewPipeStream(LispMac*, LispPipe*, LispObj*, int);
LispObj *LispNewBigInteger(LispMac*, mpi*);
LispObj *LispNewBigRational(LispMac*, mpr*);

LispAtom *LispGetAtom(LispMac*, char*);

/* This function does not allocate a copy of it's argument, but the argument
 * itself. The argument string should never change. */
LispAtom *LispGetPermAtom(LispMac*, char*);

void *LispMalloc(LispMac*, unsigned);
void *LispCalloc(LispMac*, unsigned, unsigned);
void *LispRealloc(LispMac*, void*, unsigned);
char *LispStrdup(LispMac*, char*);
void LispFree(LispMac*, void*);
/* LispMused means memory is now safe from LispDestroy, and should not be
 * freed in case of an error */
void LispMused(LispMac*, void*);

void LispGC(LispMac*, LispObj*, LispObj*);

char *LispStrObj(LispMac*, LispObj*);

void LispDestroy(LispMac *mac, char *fmt, ...);
	/* continuable error */
void LispContinuable(LispMac *mac, char *fmt, ...);
void LispMessage(LispMac *mac, char *fmt, ...);
void LispWarning(LispMac *mac, char *fmt, ...);

LispObj *LispSetVariable(LispMac*, LispObj*, LispObj*, char*, int);

int LispRegisterOpaqueType(LispMac*, char*);

int LispPrintf(LispMac*, LispObj*, char*, ...);
int LispPrintString(LispMac*, LispObj*, char*);

void LispProtect(LispMac*, LispObj*, LispObj*);
void LispUProtect(LispMac*, LispObj*, LispObj*);

/* this function should be called when a module is loaded, and is called
 * when loading the interpreter */
void LispAddBuiltinFunction(LispMac*, LispBuiltin*);

/*
 * Initialization
 */
extern LispObj *NIL, *T, *DOT, *UNBOUND;
extern int gcpro;

extern Atom_id Snil, St, Slambda, Skey, Srest, Soptional, Saux;
extern Atom_id Sand, Sor, Snot;
extern Atom_id Satom, Ssymbol, Sinteger, Scharacter, Sreal, Sstring, Slist,
	       Scons, Svector, Sarray, Sstruct, Skeyword, Sfunction, Spathname,
	       Srational, Sfloat, Scomplex, Sopaque, Sdefault;

extern LispObj *Ocomplex, *Oformat, *Ounspecific;

extern LispObj *Omake_array, *Oinitial_contents, *Osetf;
extern Atom_id Sotherwise, Svariable, Sstructure, Stype, Ssetf;

extern Atom_id Smake_struct, Sstruct_access, Sstruct_store, Sstruct_type;
extern LispObj *Omake_struct, *Ostruct_access, *Ostruct_store, *Ostruct_type;

extern Atom_id Serror, Sabsolute, Srelative, Sskip;
extern LispObj *Oparse_namestring, *Oerror, *Oabsolute, *Orelative, *Oopen,
	       *Oclose, *Oif_does_not_exist;

extern LispFile *Stdout, *Stdin, *Stderr;

#endif /* Lisp_internal_h */

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

/* $XFree86: xc/programs/xedit/lisp/internal.h,v 1.32 2002/09/22 07:09:07 paulo Exp $ */

#ifndef Lisp_internal_h
#define Lisp_internal_h

#include <stdio.h>
#include "lisp.h"

#include "mp.h"
#include "re.h"

/*
 * Defines
 */
#define STREAM_READ		1
#define STREAM_WRITE		2

#define RPLACA(cons, object)	(CAR(cons) = object)
#define RPLACD(cons, object)	(CDR(cons) = object)
#define SETVALUE(atom, object)	((atom)->property->value = object)

#define	CAR(list)		((list)->data.cons.car)
#define	CAAR(list)		((list)->data.cons.car->data.cons.car)
#define	CADR(list)		((list)->data.cons.cdr->data.cons.car)
#define CDR(list)		((list)->data.cons.cdr)
#define CDAR(list)		((list)->data.cons.car->data.cons.cdr)
#define CDDR(list)		((list)->data.cons.cdr->data.cons.cdr)
#define CONS(car, cdr)		LispNewCons(mac, car, cdr)
#define EVAL(list)		LispEval(mac, list)
#define APPLY(fun, args)	LispFuncall(mac, fun, args, 0)
#define APPLY1(fun, arg)	LispApply1(mac, fun, arg)
#define APPLY2(fun, arg1, arg2)	LispApply2(mac, fun, arg1, arg2)
#define EXECUTE(string)		LispExecute(mac, string)
#define SYMBOL(atom)		LispNewSymbol(mac, atom)
#define ATOM(string)		LispNewAtom(mac, string, 1)
#define UNINTERNED_ATOM(string)	LispNewAtom(mac, string, 0)

	/* atom string is a static variable */
#define ATOM2(string)		LispNewSymbol(mac, LispGetPermAtom(mac, string))

	/* make a gc never released variable with a static string argument */
#define STATIC_ATOM(string)	LispNewStaticAtom(mac, string)

#define QUOTE(quote)		LispNewQuote(mac, quote)
#define BACKQUOTE(bquote)	LispNewBackquote(mac, bquote)
#define COMMA(comma, at)	LispNewComma(mac, comma, at)
#define REAL(num)		LispNewReal(mac, num)
#define STRING(str)		LispNewString(mac, str, strlen(str), 0)
#define LSTRING(str, size)	LispNewString(mac, str, size, 0)

	/* string must be from the LispXXX allocation functions,
	 * and LispMused not yet called on it */
#define STRING2(str)		LispNewString(mac, str, strlen(str), 1)
#define LSTRING2(str, size)	LispNewString(mac, str, size, 1)

#define CHAR(c)			LispNewCharacter(mac, c)
#define INTEGER(i)		LispNewInteger(mac, i)
	/* The object returned by SMALLINT cannot be changed, is a constant */
#define SMALLINT(i)		LispNewSmallInt(mac, i)
#define RATIO(n, d)		LispNewRatio(mac, n, d)
#define VECTOR(objects)		LispNewVector(mac, objects)
#define COMPLEX(r, i)		LispNewComplex(mac, r, i)
#define OPAQUE(data, type)	LispNewOpaque(mac, (void*)((long)data), type)
#define KEYWORD(key)		LispNewKeyword(mac, key)
#define PATHNAME(p)		LispNewPathname(mac, p)

	/* STRINGSTREAM2 and LSTRINGSTREAM2 require that the
	 * string be allocated from the LispXXX allocation functions,
	 * and LispMused not yet called on it */
#define STRINGSTREAM(str, flag)			\
	LispNewStringStream(mac, str, flag, strlen(str), 0)
#define STRINGSTREAM2(str, flag)		\
	LispNewStringStream(mac, str, flag, strlen(str), 1)
#define LSTRINGSTREAM(str, flag, length)	\
	LispNewStringStream(mac, str, flag, length, 0)
#define LSTRINGSTREAM2(str, flag, length)	\
	LispNewStringStream(mac, str, flag, length, 1)

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

#define	GCDisable()		++gcpro
#define	GCEnable()		--gcpro

/* pointer to string of a LispAtom_t object */
#define	STRPTR(obj)		(obj)->data.atom->string

/* pointer to something unique to all atoms with the same print representation */
#define ATOMID(obj)		(obj)->data.atom->string

/* pointer to string of a LispString_t object */
#define THESTR(obj)		(obj)->data.string.string
#define STRLEN(obj)		(obj)->data.string.length

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
#define KEYWORD_P(obj)		((obj)->data.atom->package == mac->keyword)
#define PATHNAME_P(obj)		((obj)->type == LispPathname_t)
#define HASHTABLE_P(obj)	((obj)->type == LispHashTable_t)

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
#define REGEX_P(object)		((object)->type == LispRegex_t)

#define LispFileno(file)	((file)->descriptor)

#define STRFUN(builtin)		STRPTR(builtin->symbol)
#define STROBJ(obj)		LispStrObj(mac, obj)

#define CONSTANT_P(obj)					\
    ((obj)->type < LispAtom_t || (obj->type == LispAtom_t && KEYWORD_P(obj)))

/* slightly faster test, since keywords are very uncommon as eval arguments */
#define NCONSTANT_P(obj)				\
    ((obj)->type >= LispAtom_t)

/* fetch builtin function/macro argument value
 */
#define ARGUMENT(index)					\
	mac->stack.values[mac->stack.base + (index)]

#define RETURN(index)	mac->returns.values[(index)]
#define RETURN_COUNT	mac->returns.count
#define RETURN_CHECK(value)				\
    value < MULTIPLE_VALUES_LIMIT ?			\
	value : MULTIPLE_VALUES_LIMIT

#define GC_ENTER()					\
    int gc__protect = mac->protect.length

#define GC_PROTECT(object)				\
    if (mac->protect.length >= mac->protect.space)	\
	LispMoreProtects(mac);				\
    mac->protect.objects[mac->protect.length++] = object

#define GC_LEAVE()					\
    mac->protect.length = gc__protect

/* Error checking, to be called from builtin functions */
#define ERROR_CHECK_SYMBOL(object)				\
    if (!SYMBOL_P(object))					\
	LispDestroy(mac, "%s: %s is not a symbol",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_KEYWORD(object)				\
    if (!SYMBOL_P(object) || !KEYWORD_P(object))		\
	LispDestroy(mac, "%s: %s is not a keyword",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_LIST(object)				\
    if (!CONS_P(object))					\
	LispDestroy(mac, "%s: %s is not a list",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_STRING(object)				\
    if (!STRING_P(object))					\
	LispDestroy(mac, "%s: %s is not a string",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_REAL(object)				\
    if (!REAL_P(object))					\
	LispDestroy(mac, "%s: %s is not a real number",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_INTEGER(object)				\
    if (!INTEGER_P(object))					\
	LispDestroy(mac, "%s: %s is not an integer",		\
		    STRFUN(builtin), STROBJ(object))

/* XXX previous macros misnamed FIXNUM */
#define ERROR_CHECK_FIXNUM(object)				\
    if (!INT_P(object))						\
	LispDestroy(mac, "%s: %s is not a fixnum",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_FLOAT(object)				\
    if (!FLOAT_P(object))					\
	LispDestroy(mac, "%s: %s is not a float number",	\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_NUMBER(object)				\
    if (!NUMBER_P(object))					\
	LispDestroy(mac, "%s: %s is not a number",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_CHARACTER(object)				\
    if (!CHAR_P(object))					\
	LispDestroy(mac, "%s: %s is not a character",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_INDEX(object)				\
    if (!INDEX_P(object))					\
	LispDestroy(mac, "%s: %s is not a positive fixnum",	\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_CONS(object)				\
    if (!CONS_P(object))					\
	LispDestroy(mac, "%s: %s is not of type cons",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_STREAM(object)				\
    if (!STREAM_P(object))					\
	LispDestroy(mac, "%s: %s is not a stream",		\
		    STRFUN(builtin), STROBJ(object))

/* Don't check if object is a symbol */
#define ERROR_CHECK_CONSTANT(object)				\
    if ((object)->data.atom->constant)				\
	LispDestroy(mac, "%s: %s is a constant",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_REGEX(object)				\
    if (!REGEX_P(object))					\
	LispDestroy(mac, "%s: %s is not a regexp",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_HASHTABLE(object)				\
    if (!HASHTABLE_P(object))					\
	LispDestroy(mac, "%s: %s is not a hash-table",		\
		    STRFUN(builtin), STROBJ(object))

#define ERROR_CHECK_SPECIAL_FORM(atom)					\
    if (atom->property->fun.builtin->compile)				\
	LispDestroy(mac, "%s: the special form %s cannot be redefined",	\
		    STRFUN(builtin), atom->string)

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
typedef struct _LispBytecode LispBytecode;
typedef struct _LispHashTable LispHashTable;

/* Bytecode compiler data */
typedef struct _LispCom LispCom;

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
    LispPackage_t,
    LispRegex_t,
    LispBytecode_t,
    LispHashTable_t
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
    LispFunType funtype : 4;	/* this is subject to change in the future */
    union {
	LispAtom *atom;
	struct {
	    char *string;
	    long length;
	} string;
	long integer;
	double real;
	LispObj *quote;
	LispObj *pathname;	/* don't use quote generic name,
				 * to avoid confusing code */
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
	    LispObj *data;		/* extra data to protect */
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
	struct {
	    re_cod *regex;
	    LispObj *pattern;		/* regex string */
	    int options;		/* regex compile flags */
	} regex;
	struct {
	    LispBytecode *bytecode;
	    LispObj *code;		/* object used to generate bytecode */
	} bytecode;
	struct {
	    LispHashTable *table;
	    LispObj *test;
	} hash;
    } data;
};

typedef	LispObj *(*LispFunPtr)(LispMac*, LispBuiltin*);
typedef void (*LispComPtr)(LispCom*, LispBuiltin*);

struct _LispBuiltin {
    /* these fields must be set */
    LispFunType type;
    LispFunPtr function;
    char *declaration;

    /* this field is optional, set if the function returns multiple values */
    int multiple_values;

    /* this field is also optional, set if the function should not be exported */
    int internal;

    /* this optional field points to a function of the bytecode compiler */
    LispComPtr compile;

    /* this field is set at runtime */
    LispObj *symbol;

    /* if the builtin function has default values, it is protected here */
    LispObj *data;
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
LispObj *LispFuncall(LispMac*, LispObj*, LispObj*, int);
LispObj *LispApply1(LispMac*, LispObj*, LispObj*);
LispObj *LispApply2(LispMac*, LispObj*, LispObj*, LispObj*);

LispObj *LispNew(LispMac*, LispObj*, LispObj*);
LispObj *LispNewSymbol(LispMac*, LispAtom*);
LispObj *LispNewAtom(LispMac*, char*, int);
LispObj *LispNewStaticAtom(LispMac*, char*);
LispObj *LispNewReal(LispMac*, double);
LispObj *LispNewString(LispMac*, char*, long, int);
LispObj *LispNewCharacter(LispMac*, long);
LispObj *LispNewSmallInt(LispMac*, long);
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
LispObj *LispNewKeyword(LispMac*, char*);
LispObj *LispNewPathname(LispMac*, LispObj*);
LispObj *LispNewStringStream(LispMac*, unsigned char*, int, long, int);
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

#ifdef __GNUC__
#define PRINTF_FORMAT	__attribute__ ((format (printf, 2, 3)))
#else
#define PRINTF_FORMAT	/**/
#endif
void LispDestroy(LispMac *mac, char *fmt, ...) PRINTF_FORMAT;
	/* continuable error */
void LispContinuable(LispMac *mac, char *fmt, ...) PRINTF_FORMAT;
void LispMessage(LispMac *mac, char *fmt, ...) PRINTF_FORMAT;
void LispWarning(LispMac *mac, char *fmt, ...) PRINTF_FORMAT;

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

extern LispObj *Okey, *Orest, *Ooptional, *Oaux, *Olambda;
extern Atom_id Snil, St, Skey, Srest, Soptional, Saux;
extern Atom_id Sand, Sor, Snot;
extern Atom_id Satom, Ssymbol, Sinteger, Scharacter, Sreal, Sstring, Slist,
	       Scons, Svector, Sarray, Sstruct, Skeyword, Sfunction, Spathname,
	       Srational, Sfloat, Scomplex, Sopaque, Sdefault;

extern LispObj *Ocomplex, *Oformat, *Kunspecific;

extern LispObj *Omake_array, *Kinitial_contents, *Osetf;
extern Atom_id Svariable, Sstructure, Stype, Ssetf;

extern Atom_id Smake_struct, Sstruct_access, Sstruct_store, Sstruct_type;
extern LispObj *Omake_struct, *Ostruct_access, *Ostruct_store, *Ostruct_type;

extern Atom_id Serror, Sabsolute, Srelative, Sskip;
extern LispObj *Oparse_namestring, *Kerror, *Kabsolute, *Krelative, *Oopen,
	       *Oclose, *Kif_does_not_exist;

extern LispObj *Oequal_;

extern LispFile *Stdout, *Stdin, *Stderr;

#endif /* Lisp_internal_h */

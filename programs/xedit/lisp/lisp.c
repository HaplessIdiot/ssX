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

/* $XFree86: xc/programs/xedit/lisp/lisp.c,v 1.60 2002/09/15 21:32:20 paulo Exp $ */

#include <stdlib.h>
#include <string.h>
#ifdef sun	/* Don't ask.... */
#include <strings.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>

#ifndef X_NOT_POSIX
#include <unistd.h>	/* for sysconf(), and getpagesize() */
#endif

#if defined(linux)
#include <asm/page.h>	/* for PAGE_SIZE */
#define HAS_GETPAGESIZE
#define HAS_SC_PAGESIZE	/* _SC_PAGESIZE may be an enum for Linux */
#endif

#if defined(CSRG_BASED)
#define HAS_GETPAGESIZE
#endif

#if defined(sun)
#define HAS_GETPAGESIZE
#endif

#if defined(QNX4)
#define HAS_GETPAGESIZE
#endif

#if defined(__QNXNTO__)
#define HAS_SC_PAGESIZE
#endif

#include "bytecode.h"

#include "read.h"
#include "format.h"
#include "math.h"
#include "package.h"
#include "pathname.h"
#include "regex.h"
#include "require.h"
#include "stream.h"
#include "struct.h"
#include "time.h"
#include "write.h"
#include <math.h>

typedef struct {
    LispObj **objects;
    LispObj *freeobj;
    int nsegs;
    int nobjs;
    int nfree;
} LispObjSeg;

/*
 * Prototypes
 */
static void Lisp__GC(LispMac*, LispObj*, LispObj*);
LispObj *Lisp__New(LispMac*, LispObj*, LispObj*);

/* run a user function, to be called only by LispEval */
static LispObj *LispRunFunMac(LispMac*, LispObj*, LispObj*, int, int);

/* expands and executes a setf method, to be called only by Lisp_Setf */
LispObj *LispRunSetf(LispMac*, LispArgList*, LispObj*, LispObj*, LispObj*);

/* increases storage size for environment */
void LispMoreEnvironment(LispMac*);

/* increases storage size for stack of builtin arguments */
void LispMoreStack(LispMac*);

/* increases storage size for global variables */
void LispMoreGlobals(LispMac*, LispPackage*);

#ifdef __GNUC__
static INLINE LispObj *LispDoGetVar(LispMac*, LispObj*);
#endif
static INLINE void LispDoAddVar(LispMac*, LispObj*, LispObj*);

/* Helper for importing symbol(s) functions,
 * Search for the specified object in the current package */
static INLINE LispObj *LispGetVarPack(LispMac*, LispObj*);

/* create environment for function call */
static int LispMakeEnvironment(LispMac*, LispArgList*,
			       LispObj*, LispObj*, int, int);

	/* if not already in keyword package, move atom to keyword package */
static LispObj *LispCheckKeyword(LispMac*, LispObj*);

	/* builtin backquote parsing */
static LispObj *LispEvalBackquoteObject(LispMac*, LispObj*, int, int);
	/* used also by the bytecode compiler */
LispObj *LispEvalBackquote(LispMac*, LispObj*, int);

	/* create or change object property */
void LispSetAtomObjectProperty(LispMac*, LispAtom*, LispObj*);
	/* remove object property */
static void LispRemAtomObjectProperty(LispMac*, LispAtom*);

	/* allocates a new LispProperty for the given atom */
static void LispAllocAtomProperty(LispMac*, LispAtom*);
	/* Increment reference count of atom property */
static void LispIncrementAtomReference(LispMac*, LispAtom*);
	/* Decrement reference count of atom property */
static void LispDecrementAtomReference(LispMac*, LispAtom*);
	/* Removes all atom properties */
static void LispRemAtomAllProperties(LispMac*, LispAtom*);

static LispObj *LispDoGetAtomProperty(LispMac*, LispAtom*, LispObj*, int);

static INLINE void LispCheckMemLevel(LispMac*);

void LispAllocSeg(LispMac*, LispObjSeg*, int);
static INLINE void LispMark(LispObj*);

/* functions, macros, setf methods, and structure definitions */
static INLINE void LispImmutable(LispObj*);
/* if redefined or unbounded */
static INLINE void LispMutable(LispObj*);

static LispObj *LispCheckNeedProtect(LispObj*);

#ifdef SIGNALRETURNSINT
int LispAbortSignal(int);
int LispFPESignal(int);
#else
void LispAbortSignal(int);
void LispFPESignal(int);
#endif

/*
 * Initialization
 */
static LispObj lispnil = {LispNil_t};
static LispObj lispt = {LispTrue_t};
static LispObj lispunbound = {LispNil_t};
LispObj *NIL = &lispnil, *T = &lispt, *UNBOUND = &lispunbound;

LispObj *Okey, *Orest, *Ooptional, *Oaux, *Olambda;

Atom_id Snil, St;
Atom_id Saux, Skey, Soptional, Srest;
Atom_id Satom, Ssymbol, Sinteger, Scharacter, Sreal, Sstring, Slist,
	Scons, Svector, Sarray, Sstruct, Skeyword, Sfunction, Spathname,
	Srational, Sfloat, Scomplex, Sopaque, Sdefault;

LispObj *Oformat, *Kunspecific;
LispObj *Oexpand_setf_method;

static LispProperty noproperty;
LispProperty *NOPROPERTY = &noproperty;
static int segsize, minfree;
int pagesize, gcpro;

static LispObjSeg objseg = {
    NULL, &lispnil
};

static LispObjSeg atomseg = {
    NULL, &lispnil
};

int LispArgList_t;

LispFile *Stdout, *Stdin, *Stderr;

static LispBuiltin lispbuiltins[] = {
    {LispFunction, Lisp_Mul, "* &rest numbers"},
    {LispFunction, Lisp_Plus, "+ &rest numbers"},
    {LispFunction, Lisp_Minus, "- number &rest more-numbers"},
    {LispFunction, Lisp_Div, "/ number &rest more-numbers"},
    {LispFunction, Lisp_OnePlus, "1+ number"},
    {LispFunction, Lisp_OneMinus, "1- number"},
    {LispFunction, Lisp_Less, "< number &rest more-numbers"},
    {LispFunction, Lisp_LessEqual, "<= number &rest more-numbers"},
    {LispFunction, Lisp_Equal_, "= number &rest more-numbers"},
    {LispFunction, Lisp_Greater, "> number &rest more-numbers"},
    {LispFunction, Lisp_GreaterEqual, ">= number &rest more-numbers"},
    {LispFunction, Lisp_NotEqual, "/= number &rest more-numbers"},
    {LispFunction, Lisp_Max, "max number &rest more-numbers"},
    {LispFunction, Lisp_Min, "min number &rest more-numbers"},
    {LispFunction, Lisp_Abs, "abs number"},
    {LispFunction, Lisp_Acons, "acons key datum alist"},
    {LispFunction, Lisp_Adjoin, "adjoin item list &key key test test-not"},
    {LispFunction, Lisp_AlphaCharP, "alpha-char-p char"},
    {LispMacro, Lisp_And, "and &rest args", 0, 0, Com_And},
    {LispFunction, Lisp_Append, "append &rest lists"},
    {LispFunction, Lisp_Apply, "apply function arg &rest more-args"},
    {LispFunction, Lisp_Aref, "aref array &rest subscripts"},
    {LispFunction, Lisp_Assoc, "assoc item list &key test test-not key"},
    {LispFunction, Lisp_AssocIf, "assoc-if predicate list &key key"},
    {LispFunction, Lisp_AssocIfNot, "assoc-if-not predicate list &key key"},
    {LispFunction, Lisp_Atom, "atom object"},
    {LispMacro, Lisp_Block, "block name &rest body", 0, 0, Com_Block},
    {LispFunction, Lisp_Boundp, "boundp symbol"},
    {LispFunction, Lisp_Butlast, "butlast list &optional count"},
    {LispFunction, Lisp_Car, "car list", 0, 0, Com_C_r},
    {LispFunction, Lisp_Car, "first list", 0, 0, Com_C_r},
    {LispMacro, Lisp_Case, "case keyform &rest body"},
    {LispMacro, Lisp_Catch, "catch tag &rest body"},
    {LispFunction, Lisp_Cdr, "cdr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_Cdr, "rest list", 0, 0, Com_C_r},
    {LispFunction, Lisp_Char, "char string index"},
    {LispFunction, Lisp_Char, "schar simple-string index"},
    {LispFunction, Lisp_CharLess, "char< character &rest more-characters"},
    {LispFunction, Lisp_CharLessEqual, "char<= character &rest more-characters"},
    {LispFunction, Lisp_CharEqual_, "char= character &rest more-characters"},
    {LispFunction, Lisp_CharGreater, "char> character &rest more-characters"},
    {LispFunction, Lisp_CharGreaterEqual, "char>= character &rest more-characters"},
    {LispFunction, Lisp_CharNotEqual_, "char/= character &rest more-characters"},
    {LispFunction, Lisp_CharLessp, "char-lessp character &rest more-characters"},
    {LispFunction, Lisp_CharNotGreaterp, "char-not-greaterp character &rest more-characters"},
    {LispFunction, Lisp_CharEqual, "char-equal character &rest more-characters"},
    {LispFunction, Lisp_CharGreaterp, "char-greaterp character &rest more-characters"},
    {LispFunction, Lisp_CharNotLessp, "char-not-lessp character &rest more-characters"},
    {LispFunction, Lisp_CharNotEqual, "char-not-equal character &rest more-characters"},
    {LispFunction, Lisp_CharDowncase, "char-downcase character"},
    {LispFunction, Lisp_CharInt, "char-int character"},
    {LispFunction, Lisp_CharUpcase, "char-upcase character"},
    {LispFunction, Lisp_Character, "character object"},
    {LispFunction, Lisp_Characterp, "characterp object"},
    {LispFunction, Lisp_Coerce, "coerce object result-type"},
    {LispFunction, Lisp_Compile, "compile name &optional definition", 1},
    {LispFunction, Lisp_Complex, "complex realpart &optional imagpart"},
    {LispMacro, Lisp_Cond, "cond &rest body", 0, 0, Com_Cond},
    {LispFunction, Lisp_Cons, "cons car cdr", 0, 0, Com_Cons},
    {LispFunction, Lisp_Consp, "consp object", 0, 0, Com_Consp},
    {LispFunction, Lisp_Conjugate, "conjugate number"},
    {LispFunction, Lisp_Complexp, "complexp object"},
    {LispFunction, Lisp_CopyList, "copy-list list"},
    {LispFunction, Lisp_Close, "close stream &key abort"},
    {LispFunction, Lisp_C_r, "caar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cadr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cdar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cddr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "caaar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "caadr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cadar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "caddr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cdaar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cdadr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cddar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cdddr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "caaaar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "caaadr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "caadar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "caaddr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cadaar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cadadr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "caddar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cadddr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cdaaar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cdaadr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cdadar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cdaddr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cddaar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cddadr list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cdddar list", 0, 0, Com_C_r},
    {LispFunction, Lisp_C_r, "cddddr list", 0, 0, Com_C_r},
    {LispMacro, Lisp_Decf, "decf place &optional delta"},
    {LispMacro, Lisp_Defconstant, "defconstant name initial-value &optional documentation"},
    {LispMacro, Lisp_Defmacro, "defmacro name lambda-list &rest body"},
    {LispMacro, Lisp_Defstruct, "defstruct name &rest description"},
    {LispMacro, Lisp_Defun, "defun name lambda-list &rest body"},
    {LispMacro, Lisp_Defsetf, "defsetf function lambda-list &rest body"},
    {LispMacro, Lisp_Defvar, "defvar name &optional (initial-value nil bound) documentation"},
    {LispFunction, Lisp_Delete, "delete item sequence &key from-end test test-not start end count key"},
    {LispFunction, Lisp_DeleteDuplicates, "delete-duplicates sequence &key from-end test test-not start end key"},
    {LispFunction, Lisp_DeleteIf, "delete-if predicate sequence &key from-end start end count key"},
    {LispFunction, Lisp_DeleteIfNot, "delete-if-not predicate sequence &key from-end start end count key"},
    {LispFunction, Lisp_Denominator, "denominator rational"},
    {LispFunction, Lisp_DigitCharP, "digit-char-p character &optional (radix 10)"},
    {LispFunction, Lisp_Directory, "directory pathname &key all if-cannot-read"},
    {LispFunction, Lisp_DirectoryNamestring, "directory-namestring pathname"},
    {LispFunction, Lisp_Disassemble, "disassemble function"},
    {LispMacro, Lisp_Do, "do init test &rest body"},
    {LispMacro, Lisp_DoP, "do* init test &rest body"},
    {LispFunction, Lisp_Documentation, "documentation symbol type"},
    {LispMacro, Lisp_DoList, "dolist init &rest body", 0, 0, Com_Dolist},
    {LispMacro, Lisp_DoTimes, "dotimes init &rest body"},
    {LispMacro, Lisp_DoAllSymbols, "do-all-symbols init &rest body"},
    {LispMacro, Lisp_DoExternalSymbols, "do-external-symbols init &rest body"},
    {LispMacro, Lisp_DoSymbols, "do-symbols init &rest body"},
    {LispFunction, Lisp_Elt, "elt sequence index"},
    {LispFunction, Lisp_Endp, "endp object"},
    {LispFunction, Lisp_EnoughNamestring, "enough-namestring pathname &optional defaults"},
    {LispFunction, Lisp_Eq, "eq left right", 0, 0, Com_Eq},
    {LispFunction, Lisp_Eql, "eql left right", 0, 0, Com_Eq},
    {LispFunction, Lisp_Equal, "equal left right", 0, 0, Com_Eq},
    {LispFunction, Lisp_Equalp, "equalp left right", 0, 0, Com_Eq},
    {LispFunction, Lisp_Error, "error control-string &rest arguments"},
    {LispFunction, Lisp_Evenp, "evenp integer"},
    {LispFunction, Lisp_Export, "export symbols &optional package"},
    {LispFunction, Lisp_Eval, "eval form"},
    {LispFunction, Lisp_Fboundp, "fboundp symbol"},
    {LispFunction, Lisp_FileNamestring, "file-namestring pathname"},
    {LispFunction, Lisp_Fill, "fill sequence item &key start end"},
    {LispFunction, Lisp_FindAllSymbols, "find-all-symbols string-or-symbol"},
    {LispFunction, Lisp_FindPackage, "find-package name"},
    {LispFunction, Lisp_Float, "float number &optional (other 1.0)"},
    {LispFunction, Lisp_Floatp, "floatp object"},
    {LispFunction, Lisp_Fmakunbound, "fmakunbound symbol"},
    {LispFunction, Lisp_Format, "format destination control-string &rest arguments"},
    {LispFunction, Lisp_Funcall, "funcall function &rest arguments"},
    {LispFunction, Lisp_Gc, "gc &optional car cdr"},
    {LispFunction, Lisp_Gcd, "gcd &rest integers"},
    {LispFunction, Lisp_Gensym, "gensym &optional arg"},
    {LispFunction, Lisp_Get, "get symbol indicator &optional default"},
    {LispMacro, Lisp_Go, "go tag", 0, 0, Com_Go},
    {LispFunction, Lisp_HostNamestring, "host-namestring pathname"},
    {LispMacro, Lisp_If, "if test then &optional else", 0, 0, Com_If},
    {LispFunction, Lisp_Imagpart, "imagpart number"},
    {LispMacro, Lisp_InPackage, "in-package name"},
    {LispMacro, Lisp_Incf, "incf place &optional delta"},
    {LispFunction, Lisp_Import, "import symbols &optional package"},
    {LispFunction, Lisp_InputStreamP, "input-stream-p stream"},
    {LispFunction, Lisp_IntChar, "int-char integer"},
    {LispFunction, Lisp_Integerp, "integerp object"},
    {LispFunction, Lisp_Intern, "intern string &optional package", 1},
    {LispFunction, Lisp_Intersection, "intersection list1 list2 &key test test-not key"},
    {LispFunction, Lisp_Isqrt, "isqrt natural"},
    {LispFunction, Lisp_Keywordp, "keywordp object"},
    {LispFunction, Lisp_Last, "last list &optional count", 0, 0, Com_Last},
    {LispMacro, Lisp_Lambda, "lambda lambda-list &rest body"},
    {LispFunction, Lisp_Lcm, "lcm &rest integers"},
    {LispFunction, Lisp_Length, "length sequence", 0, 0, Com_Length},
    {LispMacro, Lisp_Let, "let init &rest body", 0, 0, Com_Let},
    {LispMacro, Lisp_LetP, "let* init &rest body", 0, 0, Com_Letx},
    {LispFunction, Lisp_ListP, "list* object &rest more-objects"},
    {LispFunction, Lisp_ListAllPackages, "list-all-packages"},
    {LispFunction, Lisp_List, "list &rest args"},
    {LispFunction, Lisp_Listp, "listp object", 0, 0, Com_Listp},
    {LispFunction, Lisp_Listen, "listen &optional input-stream"},
    {LispFunction, Lisp_Load, "load filename &key verbose print if-does-not-exist"},
    {LispFunction, Lisp_Logand, "logand &rest integers"},
    {LispFunction, Lisp_Logeqv, "logeqv &rest integers"},
    {LispFunction, Lisp_Logior, "logior &rest integers"},
    {LispFunction, Lisp_Lognot, "lognot integer"},
    {LispFunction, Lisp_Logxor, "logxor &rest integers"},
    {LispMacro, Lisp_Loop, "loop &rest body", 0, 0, Com_Loop},
    {LispFunction, Lisp_MakeArray, "make-array dimensions &key element-type initial-element initial-contents adjustable fill-pointer displaced-to displaced-index-offset"},
    {LispFunction, Lisp_MakeList, "make-list size &key initial-element"},
    {LispFunction, Lisp_MakePackage, "make-package package-name &key nicknames (use '(\"LISP\"))"},
    {LispFunction, Lisp_MakePathname, "make-pathname &key host device directory name type version defaults"},
    {LispFunction, Lisp_MakeStringInputStream, "make-string-input-stream string &optional start end"},
    {LispFunction, Lisp_MakeStringOutputStream, "make-string-output-stream &key element-type"},
    {LispFunction, Lisp_GetOutputStreamString, "get-output-stream-string string-output-stream"},
    {LispFunction, Lisp_Makunbound, "makunbound symbol"},
    {LispFunction, Lisp_Mapcar, "mapcar function list &rest more-lists"},
    {LispFunction, Lisp_Maplist, "maplist function list &rest more-lists"},
    {LispFunction, Lisp_Member, "member item list &key test test-not key"},
    {LispFunction, Lisp_MemberIf, "member-if predicate list &key key"},
    {LispFunction, Lisp_MemberIfNot, "member-if-not predicate list &key key"},
    {LispFunction, Lisp_Minusp, "minusp number"},
    {LispMacro, Lisp_MultipleValueBind, "multiple-value-bind symbols values &rest body"},
    {LispMacro, Lisp_MultipleValueList, "multiple-value-list form"},
    {LispFunction, Lisp_Nconc, "nconc &rest lists"},
    {LispFunction, Lisp_Nreverse, "nreverse sequence"},
    {LispFunction, Lisp_NsetDifference, "nset-difference list1 list2 &key test test-not key"},
    {LispFunction, Lisp_Nsubstitute, "nsubstitute newitem olditem sequence &key from-end test test-not start end count key"},
    {LispFunction, Lisp_NsubstituteIf, "nsubstitute-if newitem test sequence &key from-end start end count key"},
    {LispFunction, Lisp_NsubstituteIfNot, "nsubstitute-if-not newitem test sequence &key from-end start end count key"},
    {LispFunction, Lisp_Nth, "nth index list"},
    {LispFunction, Lisp_Nthcdr, "nthcdr index list", 0, 0, Com_Nthcdr},
    {LispFunction, Lisp_Numerator, "numerator rational"},
    {LispFunction, Lisp_Namestring, "namestring pathname"},
    {LispFunction, Lisp_Null, "not arg", 0, 0, Com_Null},
    {LispFunction, Lisp_Null, "null list", 0, 0, Com_Null},
    {LispFunction, Lisp_Numberp, "numberp object", 0, 0, Com_Numberp},
    {LispFunction, Lisp_Oddp, "oddp integer"},
    {LispFunction, Lisp_Open, "open filename &key direction element-type if-exists if-does-not-exist external-format"},
    {LispFunction, Lisp_OpenStreamP, "open-stream-p stream"},
    {LispMacro, Lisp_Or, "or &rest args", 0, 0, Com_Or},
    {LispFunction, Lisp_OutputStreamP, "output-stream-p stream"},
    {LispFunction, Lisp_PackageName, "package-name package"},
    {LispFunction, Lisp_PackageNicknames, "package-nicknames package"},
    {LispFunction, Lisp_PackageUseList, "package-use-list package"},
    {LispFunction, Lisp_PackageUsedByList, "package-used-by-list package"},
    {LispFunction, Lisp_ParseInteger, "parse-integer string &key start end radix junk-allowed", 1},
    {LispFunction, Lisp_ParseNamestring, "parse-namestring object &optional host defaults &key start end junk-allowed"},
    {LispFunction, Lisp_PathnameHost, "pathname-host pathname"},
    {LispFunction, Lisp_PathnameDevice, "pathname-device pathname"},
    {LispFunction, Lisp_PathnameDirectory, "pathname-directory pathname"},
    {LispFunction, Lisp_PathnameName, "pathname-name pathname"},
    {LispFunction, Lisp_PathnameType, "pathname-type pathname"},
    {LispFunction, Lisp_PathnameVersion, "pathname-version pathname"},
    {LispFunction, Lisp_Pathnamep, "pathnamep object"},
    {LispFunction, Lisp_Plusp, "plusp number"},
    {LispMacro, Lisp_Pop, "pop place"},
    {LispFunction, Lisp_Position, "position item sequence &key from-end test test-not start end key"},
    {LispFunction, Lisp_PositionIf, "position-if predicate sequence &key from-end start end key"},
    {LispFunction, Lisp_PositionIfNot, "position-if-not predicate sequence &key from-end start end key"},
    {LispFunction, Lisp_Prin1, "prin1 object &optional output-stream"},
    {LispFunction, Lisp_Princ, "princ object &optional output-stream"},
    {LispFunction, Lisp_Print, "print object &optional output-stream"},
    {LispFunction, Lisp_ProbeFile, "probe-file pathname"},
    {LispFunction, Lisp_Proclaim, "proclaim declaration"},
    {LispMacro, Lisp_Prog1, "prog1 first &rest body"},
    {LispMacro, Lisp_Prog2, "prog2 first second &rest body"},
    {LispMacro, Lisp_Progn, "progn &rest body", 0, 0, Com_Progn},
    {LispMacro, Lisp_Progv, "progv symbols values &rest body"},
    {LispFunction, Lisp_Provide, "provide module"},
    {LispMacro, Lisp_Push, "push item place"},
    {LispMacro, Lisp_Pushnew, "pushnew item place &key key test test-not"},
    {LispFunction, Lisp_Quit, "quit &optional status"},
    {LispMacro, Lisp_Quote, "quote object"},
    {LispFunction, Lisp_Rational, "rational number"},
    {LispFunction, Lisp_Rationalp, "rationalp object"},
    {LispFunction, Lisp_Read, "read &optional input-stream (eof-error-p t) eof-value recursive-p"},
    {LispFunction, Lisp_ReadChar, "read-char &optional input-stream (eof-error-p t) eof-value recursive-p"},
    {LispFunction, Lisp_ReadCharNoHang, "read-char-no-hang &optional input-stream (eof-error-p t) eof-value recursive-p"},
    {LispFunction, Lisp_ReadLine, "read-line &optional input-stream (eof-error-p t) eof-value recursive-p", 1},
    {LispFunction, Lisp_Realpart, "realpart number"},
    {LispFunction, Lisp_Replace, "replace sequence1 sequence2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_ReadFromString, "read-from-string string &optional eof-error-p eof-value &key start end preserve-whitespace", 1},
    {LispFunction, Lisp_Require, "require module &optional pathname"},
    {LispFunction, Lisp_Remove, "remove item sequence &key from-end test test-not start end count key"},
    {LispFunction, Lisp_RemoveDuplicates, "remove-duplicates sequence &key from-end test test-not start end key"},
    {LispFunction, Lisp_RemoveIf, "remove-if predicate sequence &key from-end start end count key"},
    {LispFunction, Lisp_RemoveIfNot, "remove-if-not predicate sequence &key from-end start end count key"},
    {LispMacro, Lisp_Return, "return &optional result", 0, 0, Com_Return},
    {LispMacro, Lisp_ReturnFrom, "return-from name &optional result", 0, 0, Com_ReturnFrom},
    {LispFunction, Lisp_Reverse, "reverse sequence"},
    {LispFunction, Lisp_Rplaca, "rplaca place value", 0, 0, Com_Rplac_},
    {LispFunction, Lisp_Rplacd, "rplacd place value", 0, 0, Com_Rplac_},
    {LispFunction, Lisp_Search, "search sequence1 sequence2 &key from-end test test-not key start1 start2 end1 end2"},
    {LispFunction, Lisp_Set, "set symbol value"},
    {LispFunction, Lisp_SetDifference, "set-difference list1 list2 &key test test-not key"},
    {LispFunction, Lisp_SetExclusiveOr, "set-exclusive-or list1 list2 &key test test-not key"},
    {LispMacro, Lisp_Setf, "setf &rest form"},
    {LispMacro, Lisp_SetQ, "setq &rest form", 0, 0, Com_Setq},
    {LispFunction, Lisp_Sleep, "sleep seconds"},
    {LispFunction, Lisp_Sort, "sort sequence predicate &key key"},
    {LispFunction, Lisp_Sqrt, "sqrt number"},
    {LispFunction, Lisp_Elt, "svref sequence index"},
    {LispFunction, Lisp_Sort, "stable-sort sequence predicate &key key"},
    {LispFunction, Lisp_Streamp, "streamp object"},
    {LispFunction, Lisp_String, "string object"},
    {LispFunction, Lisp_Stringp, "stringp object"},
    {LispFunction, Lisp_StringEqual_, "string= string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringLess, "string< string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringGreater, "string> string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringLessEqual, "string<= string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringGreaterEqual, "string>= string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringNotEqual_, "string/= string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringConcat, "string-concat &rest strings"},
    {LispFunction, Lisp_StringEqual, "string-equal string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringGreaterp, "string-greaterp string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringNotEqual, "string-not-equal string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringNotGreaterp, "string-not-greaterp string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringNotLessp, "string-not-lessp string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringLessp, "string-lessp string1 string2 &key start1 end1 start2 end2"},
    {LispFunction, Lisp_StringTrim, "string-trim character-bag string"},
    {LispFunction, Lisp_StringLeftTrim, "string-left-trim character-bag string"},
    {LispFunction, Lisp_StringRightTrim, "string-right-trim character-bag string"},
    {LispFunction, Lisp_StringUpcase, "string-upcase string &key start end"},
    {LispFunction, Lisp_StringDowncase, "string-downcase string &key start end"},
    {LispFunction, Lisp_StringCapitalize, "string-capitalize string &key start end"},
    {LispFunction, Lisp_Subseq, "subseq sequence start &optional end"},
    {LispFunction, Lisp_Subsetp, "subsetp list1 list2 &key test test-not key"},
    {LispFunction, Lisp_Substitute, "substitute newitem olditem sequence &key from-end test test-not start end count key"},
    {LispFunction, Lisp_SubstituteIf, "substitute-if newitem test sequence &key from-end start end count key"},
    {LispFunction, Lisp_SubstituteIfNot, "substitute-if-not newitem test sequence &key from-end start end count key"},
    {LispFunction, Lisp_Symbolp, "symbolp object"},
    {LispFunction, Lisp_SymbolPlist, "symbol-plist symbol"},
    {LispFunction, Lisp_SymbolValue, "symbol-value symbol"},
    {LispMacro, Lisp_Tagbody, "tagbody &rest body", 0, 0, Com_Tagbody},
    {LispFunction, Lisp_Terpri, "terpri &optional output-stream"},
    {LispFunction, Lisp_Typep, "typep object type"},
    {LispMacro, Lisp_The, "the value-type form"},
    {LispMacro, Lisp_Throw, "throw tag result"},
    {LispMacro, Lisp_Time, "time form"},
    {LispFunction, Lisp_Truename, "truename pathname"},
    {LispFunction, Lisp_Unexport, "unexport symbols &optional package"},
    {LispFunction, Lisp_Union, "union list1 list2 &key test test-not key"},
    {LispMacro, Lisp_Unless, "unless test &rest body", 0, 0, Com_Unless},
    {LispFunction, Lisp_UserHomedirPathname, "user-homedir-pathname &optional host"},
    {LispMacro, Lisp_UnwindProtect, "unwind-protect protect &rest cleanup"},
    {LispFunction, Lisp_Vector, "vector &rest objects"},
    {LispMacro, Lisp_When, "when test &rest body", 0, 0, Com_When},
    {LispFunction, Lisp_WriteChar, "write-char string &optional output-stream"},
    {LispFunction, Lisp_WriteLine, "write-line string &optional output-stream &key start end"},
    {LispFunction, Lisp_WriteString, "write-string string &optional output-stream &key start end"},
    {LispFunction, Lisp_XeditCharStore, "lisp::char-store string index value", 0, 1},
    {LispFunction, Lisp_XeditEltStore, "lisp::elt-store sequence index value", 0, 1},
    {LispFunction, Lisp_XeditMakeStruct, "lisp::make-struct atom &rest init", 0, 1},
    {LispFunction, Lisp_XeditPut, " lisp::put symbol indicator value", 0, 1},
    {LispFunction, Lisp_XeditStructAccess, "lisp::struct-access atom struct", 0, 1},
    {LispFunction, Lisp_XeditStructType, "lisp::struct-type atom struct", 0, 1},
    {LispFunction, Lisp_XeditStructStore, "lisp::struct-store atom struct value", 0, 1},
    {LispFunction, Lisp_XeditVectorStore, "lisp::vector-store array subscripts value", 0, 1},
    {LispFunction, Lisp_Zerop, "zerop number"},
};

static LispBuiltin extbuiltins[] = {
    {LispFunction, Lisp_Getenv, "getenv name"},
    {LispFunction, Lisp_MakePipe, "make-pipe command-line &key direction element-type external-format"},
    {LispFunction, Lisp_PipeBroken, "pipe-broken pipe-stream"},
    {LispFunction, Lisp_PipeErrorStream, "pipe-error-stream pipe-stream"},
    {LispFunction, Lisp_PipeInputDescriptor, "pipe-input-descriptor pipe-stream"},
    {LispFunction, Lisp_PipeErrorDescriptor, "pipe-error-descriptor pipe-stream"},
    {LispFunction, Lisp_Recomp, "re-comp pattern &key nospec icase nosub newline"},
    {LispFunction, Lisp_Reexec, "re-exec regex string &key count start end notbol noteol"},
    {LispFunction, Lisp_Rep, "re-p object"},
    {LispFunction, Lisp_Setenv, "setenv name value &optional overwrite"},
    {LispFunction, Lisp_Unsetenv, "unsetenv name"},
    {LispMacro, Lisp_Until, "until test &rest body", 0, 0, Com_Until},
    {LispMacro, Lisp_While, "while test &rest body", 0, 0, Com_While},
};

/* 2048 bytes if sizeof(LispObj) == 16, actually, an integer object uses
 * 8 bytes in 32 bit computers, but since it is an union, ends using 16.
 * This can save a lot of gc time as most times, numeric values are in
 * the range -16 - 111.
 */
static LispObj LispSmallInts[128];

/* byte code function argument list for functions that don't change it's
 * &REST argument list. */
extern LispObj x_cons[8];

/*
 * Implementation
 */
static int
LispGetPageSize(void)
{
    static int pagesize = -1;

    if (pagesize != -1)
	return pagesize;

    /* Try each supported method in the preferred order */

#if defined(_SC_PAGESIZE) || defined(HAS_SC_PAGESIZE)
    pagesize = sysconf(_SC_PAGESIZE);
#endif

#ifdef _SC_PAGE_SIZE
    if (pagesize == -1)
	pagesize = sysconf(_SC_PAGE_SIZE);
#endif

#ifdef HAS_GETPAGESIZE
    if (pagesize == -1)
	pagesize = getpagesize();
#endif

#ifdef PAGE_SIZE
    if (pagesize == -1)
	pagesize = PAGE_SIZE;
#endif

    if (pagesize < sizeof(LispObj) * 16)
	pagesize = sizeof(LispObj) * 16;	/* need a reasonable sane size */

    return pagesize;
}

void
LispDestroy(LispMac *mac, char *fmt, ...)
{
    static char Error[] = "*** ";

    if (!mac->destroyed) {
	char string[128];
	va_list ap;

	if (Stderr->column)
	    LispFputc(Stderr, '\n');
	LispFputs(Stderr, Error);
	va_start(ap, fmt);
	vsnprintf(string, sizeof(string), fmt, ap);
	va_end(ap);
	LispFputs(Stderr, string);
	LispFputc(Stderr, '\n');
	LispFflush(Stderr);

#ifdef DEBUGGER
	if (mac->debugging) {
	    LispDebugger(mac, LispDebugCallWatch, NIL, NIL);
	    LispDebugger(mac, LispDebugCallFatal, NIL, NIL);
	}
#endif

	mac->destroyed = 1;
	LispBlockUnwind(mac, NULL);
	if (mac->errexit)
	    exit(1);
    }

#ifdef DEBUGGER
    if (mac->debugging) {
	/* when stack variables could be changed, this must be also changed! */
	mac->debug_level = -1;
	mac->debug = LispDebugUnspec;
    }
#endif

    while (mac->mem.level) {
	--mac->mem.level;
	if (mac->mem.mem[mac->mem.level])
	    free(mac->mem.mem[mac->mem.level]);
    }
    mac->mem.index = 0;

    /* If the package was changed and an error happened */
    PACKAGE = mac->savepackage;
    mac->pack = mac->savepack;

    LispTopLevel(mac);

    if (!mac->running) {
	static char Fatal[] = "*** Fatal: nowhere to longjmp.\n";

	LispFputs(Stderr, Fatal);
	LispFflush(Stderr);
	abort();
    }

    siglongjmp(mac->jmp, 1);
}

void
LispContinuable(LispMac *mac, char *fmt, ...)
{
    va_list ap;
    char string[128];
    static char Error[] = "*** Error: ";

    if (Stderr->column)
	LispFputc(Stderr, '\n');
    LispFputs(Stderr, Error);
    va_start(ap, fmt);
    vsnprintf(string, sizeof(string), fmt, ap);
    va_end(ap);
    LispFputs(Stderr, string);
    LispFputc(Stderr, '\n');
    LispFputs(Stderr, "Type 'continue' if you want to proceed: ");
    LispFflush(Stderr);

    /* NOTE: does not check if stdin is a tty */
    if (LispFgets(Stdin, string, sizeof(string)) &&
	strcmp(string, "continue\n") == 0)
	return;

    LispDestroy(mac, "aborted on continuable error");
}

void
LispMessage(LispMac *mac, char *fmt, ...)
{
    va_list ap;
    char string[128];

    if (Stderr->column)
	LispFputc(Stderr, '\n');
    va_start(ap, fmt);
    vsnprintf(string, sizeof(string), fmt, ap);
    va_end(ap);
    LispFputs(Stderr, string);
    LispFputc(Stderr, '\n');
    LispFflush(Stderr);
}

void
LispWarning(LispMac *mac, char *fmt, ...)
{
    va_list ap;
    char string[128];
    static char Warning[] = "*** Warning: ";

    if (Stderr->column)
	LispFputc(Stderr, '\n');
    LispFputs(Stderr, Warning);
    va_start(ap, fmt);
    vsnprintf(string, sizeof(string), fmt, ap);
    va_end(ap);
    LispFputs(Stderr, string);
    LispFputc(Stderr, '\n');
    LispFflush(Stderr);
}

void
LispTopLevel(LispMac *mac)
{
    int count;

    COD = NIL;
#ifdef DEBUGGER
    if (mac->debugging) {
	DBG = NIL;
	if (mac->debug == LispDebugFinish)
	    mac->debug = LispDebugUnspec;
	mac->debug_level = -1;
	mac->debug_step = 0;
    }
#endif
    gcpro = 0;
    mac->block.block_level = 0;
    if (mac->block.block_size) {
	while (mac->block.block_size)
	    free(mac->block.block[--mac->block.block_size]);
	free(mac->block.block);
	mac->block.block = NULL;
    }

    mac->discard = 0;
    mac->destroyed = 0;

    if (CONS_P(mac->input_list)) {
	LispUngetInfo **info, *unget = mac->unget[0];

	while (CONS_P(mac->input_list))
	    mac->input_list = CDR(mac->input_list);
	SINPUT = mac->input_list;
	while (mac->nunget > 1)
	    free(mac->unget[--mac->nunget]);
	if ((info = realloc(mac->unget, sizeof(LispUngetInfo*))) != NULL)
	    mac->unget = info;
	mac->unget[0] = unget;
	mac->iunget = 0;
    }

    for (count = 0; mac->mem.level;) {
	--mac->mem.level;
	if (mac->mem.mem[mac->mem.level])
	    ++count;
    }
    mac->mem.index = 0;
    if (count)
	LispWarning(mac, "%d raw memory pointer(s) left. Probably a leak.",
		    count);

    mac->stack.base = mac->stack.length =
	mac->env.lex = mac->env.length = mac->env.head = 0;
    RETURN_COUNT = 0;
    mac->protect.length = 0;

    mac->savepackage = PACKAGE;
    mac->savepack = mac->pack;
}

void
LispGC(LispMac *mac, LispObj *car, LispObj *cdr)
{
    Lisp__GC(mac, car, cdr);
}

static void
Lisp__GC(LispMac *mac, LispObj *car, LispObj *cdr)
{
    register LispObj *entry, *last, *freeobj, **pentry, **eentry;
    register int nfree;
    unsigned i, j;
    LispAtom *atom;
    struct timeval start, end;
#ifdef DEBUG
    long sec, msec;
    int count = objseg.nfree;
#else
    long msec;
#endif

    if (gcpro)
	return;

    nfree = 0;
    freeobj = NIL;

    ++mac->gc.count;

#ifdef DEBUG
    gettimeofday(&start, NULL);
#else
    if (mac->gc.timebits)
	gettimeofday(&start, NULL);
#endif

    /* Protect all packages */
    for (entry = PACK; CONS_P(entry); entry = CDR(entry)) {
	LispObj *package = CAR(entry);
	LispPackage *pack = package->data.package.package;

	/* Protect cons cell */
	entry->mark = LispTrue_t;

	/* Protect the package cell */
	package->mark = LispTrue_t;

	/* Protect package name */
	package->data.package.name->mark = LispTrue_t;

	/* Protect package nicknames */
	LispMark(package->data.package.nicknames);

	/* Protect global symbols */
	for (pentry = pack->glb.pairs, eentry = pentry + pack->glb.length;
	    pentry < eentry; pentry++)
	    LispMark((*pentry)->data.atom->property->value);

	/* Traverse atom list, protecting properties, and function/structure
	 * definitions if mac->gc.immutablebits set */
	for (i = 0; i < STRTBLSZ; i++) {
	    atom = pack->atoms[i];
	    while (atom) {
		if (atom->property != NOPROPERTY) {
		    if (atom->a_property)
			LispMark(atom->property->properties);
		    if (mac->gc.immutablebits) {
			if (atom->a_function)
			    LispImmutable(atom->property->fun.function);
			if (atom->a_builtin)
			    LispImmutable(atom->property->fun.builtin->data);
			if (atom->a_defsetf)
			    LispImmutable(atom->property->setf);
			if (atom->a_defstruct)
			    LispImmutable(atom->property->structure.definition);
		    }
		}
		atom = atom->next;
	    }
	}
    }

    /* protect environment */
    for (pentry = mac->env.values, eentry = pentry + mac->env.length;
	 pentry < eentry; pentry++)
	LispMark(*pentry);

    /* protect multiple return values */
    for (pentry = mac->returns.values, eentry = pentry + mac->returns.length;
	 pentry < eentry; pentry++)
	LispMark(*pentry);

    /* protect stack of arguments to builtin functions */
    for (pentry = mac->stack.values, eentry = pentry + mac->stack.length;
	 pentry < eentry; pentry++)
	LispMark(*pentry);

    /* protect temporary data used by builtin functions */
    for (pentry = mac->protect.objects, eentry = pentry + mac->protect.length;
	 pentry < eentry; pentry++)
	LispMark(*pentry);

    for (i = 0; i < sizeof(x_cons) / sizeof(x_cons[0]); i++)
	x_cons[i].mark = LispNil_t;

    LispMark(COD);
#ifdef DEBUGGER
    LispMark(DBG);
    LispMark(BRK);
#endif
    LispMark(PRO);
    LispMark(DOC);
    LispMark(mac->input_list);
    LispMark(mac->output_list);
    LispMark(car);
    LispMark(cdr);

    for (j = 0; j < objseg.nsegs; j++) {
	for (entry = objseg.objects[j], last = entry + segsize;
	     entry < last; entry++) {
	    if (entry->prot)
		continue;
	    else if (entry->mark)
		entry->mark = LispNil_t;
	    else {
		switch (entry->type) {
		    case LispString_t:
			free(THESTR(entry));
			entry->type = LispCons_t;
			break;
		    case LispStream_t:
			switch (entry->data.stream.type) {
			    case LispStreamString:
				free(SSTREAMP(entry)->string);
				free(SSTREAMP(entry));
				break;
			    case LispStreamFile:
				if (FSTREAMP(entry))
				    LispFclose(FSTREAMP(entry));
				break;
			    case LispStreamPipe:
				/* XXX may need special handling if child hangs */
				if (PSTREAMP(entry)) {
				    if (IPSTREAMP(entry))
					LispFclose(IPSTREAMP(entry));
				    if (OPSTREAMP(entry))
					LispFclose(OPSTREAMP(entry));
				    /* don't bother with error stream, will also
				     * freed in this GC call, maybe just out
				     * of order */
				    if (PIDPSTREAMP(entry) > 0) {
					kill(PIDPSTREAMP(entry), SIGTERM);
					waitpid(PIDPSTREAMP(entry), NULL, 0);
				    }
				    free(PSTREAMP(entry));
				}
				break;
			    default:
				break;
			}
			entry->type = LispCons_t;
			break;
		    case LispBigInteger_t:
			mpi_clear(entry->data.mp.integer);
			free(entry->data.mp.integer);
			entry->type = LispCons_t;
			break;
		    case LispBigRatio_t:
			mpr_clear(entry->data.mp.ratio);
			free(entry->data.mp.ratio);
			entry->type = LispCons_t;
			break;
		    case LispLambda_t:
			if (!SYMBOL_P(entry->data.lambda.name))
			    LispFreeArgList(mac, (LispArgList*)
				entry->data.lambda.name->data.opaque.data);
			entry->type = LispCons_t;
			break;
		    case LispRegex_t:
			refree(entry->data.regex.regex);
			free(entry->data.regex.regex);
			entry->type = LispCons_t;
			break;
		    case LispBytecode_t:
			free(entry->data.bytecode.bytecode->code);
			free(entry->data.bytecode.bytecode);
			entry->type = LispCons_t;
			break;
		    case LispCons_t:
			/* Probably the compiler would not generate code
			 * to set the memory value to it's current value
			 * but add this case anyway... */
			break;
		    default:
			entry->type = LispCons_t;
			break;
		}
		CDR(entry) = freeobj;
		freeobj = entry;
		++nfree;
	    }
	}
    }

    objseg.nfree = nfree;
    objseg.freeobj = freeobj;

    mac->gc.immutablebits = 0;

#ifdef DEBUG
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    msec = end.tv_usec - start.tv_usec;
    if (msec < 0) {
	--sec;
	msec += 1000000;
    }
    LispMessage(mac, "gc: "
		"%ld sec, %ld msec, "
		"%d recovered, %d free, %d protected, %d total",
		sec, msec,
		objseg.nfree - count, objseg.nfree,
		objseg.nobjs - objseg.nfree, objseg.nobjs);
#else
    if (mac->gc.timebits) {
	gettimeofday(&end, NULL);
	if ((msec = end.tv_usec - start.tv_usec) < 0)
	    msec += 1000000;
	mac->gc.gctime += msec;
    }
#endif
}

static INLINE void
LispCheckMemLevel(LispMac *mac)
{
    int i;

    /* Check for a free slot before the end. */
    for (i = mac->mem.index; i < mac->mem.level; i++)
	if (mac->mem.mem[i] == NULL) {
	    mac->mem.index = i;
	    return;
	}

    /* Check for a free slot in the beginning */
    for (i = 0; i < mac->mem.index; i++)
	if (mac->mem.mem[i] == NULL) {
	    mac->mem.index = i;
	    return;
	}

    mac->mem.index = mac->mem.level;
    ++mac->mem.level;
    if (mac->mem.index < mac->mem.space)
	/* There is free space to store pointer. */
	return;
    else {
	void **ptr = (void**)realloc(mac->mem.mem,
				     (mac->mem.space + 16) * sizeof(void*));

	if (ptr == NULL)
	    LispDestroy(mac, "out of memory");
	mac->mem.mem = ptr;
	mac->mem.space += 16;
    }
}

void
LispMused(LispMac *mac, void *pointer)
{
    int i;

    for (i = mac->mem.index; i >= 0; i--)
	if (mac->mem.mem[i] == pointer) {
	    mac->mem.mem[i] = NULL;
	    mac->mem.index = i;
	    return;
	}

    for (i = mac->mem.level - 1; i > mac->mem.index; i--)
	if (mac->mem.mem[i] == pointer) {
	    mac->mem.mem[i] = NULL;
	    mac->mem.index = i;
	}
}

void *
LispMalloc(LispMac *mac, unsigned size)
{
    void *pointer;

    LispCheckMemLevel(mac);
    if ((pointer = malloc(size)) == NULL)
	LispDestroy(mac, "out of memory, couldn't allocate %u bytes", size);

    mac->mem.mem[mac->mem.index] = pointer;

    return (pointer);
}

void *
LispCalloc(LispMac *mac, unsigned nmemb, unsigned size)
{
    void *pointer;

    LispCheckMemLevel(mac);
    if ((pointer = calloc(nmemb, size)) == NULL)
	LispDestroy(mac, "out of memory, couldn't allocate %u bytes", size);

    mac->mem.mem[mac->mem.index] = pointer;

    return (pointer);
}

void *
LispRealloc(LispMac *mac, void *pointer, unsigned size)
{
    void *ptr;
    int i;

    if (pointer == NULL)
	i = mac->mem.level;
    else {
	for (i = 0; i < mac->mem.level; i++)
	    if (mac->mem.mem[i] == pointer)
		break;
    }
    if (i == mac->mem.level) {
	/* XXX Realloc on a pointer not from LispXXXalloc */
	LispCheckMemLevel(mac);
	i = mac->mem.index;
    }

    if ((ptr = realloc(pointer, size)) == NULL)
	LispDestroy(mac, "out of memory, couldn't realloc");

    mac->mem.mem[i] = ptr;

    return (ptr);
}

char *
LispStrdup(LispMac *mac, char *str)
{
    char *ptr = LispMalloc(mac, strlen(str) + 1);

    strcpy(ptr, str);

    return (ptr);
}

void
LispFree(LispMac *mac, void *pointer)
{
    int i;

    for (i = mac->mem.index; i >= 0; i--)
	if (mac->mem.mem[i] == pointer) {
	    mac->mem.mem[i] = NULL;
	    mac->mem.index = i;
	    free(pointer);
	    return;
	}

    for (i = mac->mem.level - 1; i > mac->mem.index; i--)
	if (mac->mem.mem[i] == pointer) {
	    mac->mem.mem[i] = NULL;
	    mac->mem.index = i;
	}

    free(pointer);
}

LispObj *
LispSetVariable(LispMac *mac, LispObj *var, LispObj *val, char *fname, int eval)
{
    if (!SYMBOL_P(var))
	LispDestroy(mac, "%s: %s is not a symbol", fname, STROBJ(var));
    if (eval && !CONSTANT_P(val))
	val = EVAL(val);

    return (LispSetVar(mac, var, val));
}

int
LispRegisterOpaqueType(LispMac *mac, char *desc)
{
    LispOpaque *opaque;
    int ii = 0;
    char *pp = desc;

    while (*pp)
	ii = (ii << 1) ^ *pp++;
    if (ii < 0)
	ii = -ii;
    ii %= STRTBLSZ;
    for (opaque = mac->opqs[ii]; opaque; opaque = opaque->next)
	if (strcmp(opaque->desc, desc) == 0)
	    return (opaque->type);
    opaque = (LispOpaque*)LispMalloc(mac, sizeof(LispOpaque));
    opaque->desc = LispStrdup(mac, desc);
    opaque->next = mac->opqs[ii];
    mac->opqs[ii] = opaque;
    LispMused(mac, opaque->desc);
    LispMused(mac, opaque);

    return (opaque->type = ++mac->opaque);
}

char *
LispIntToOpaqueType(LispMac *mac, int type)
{
    int i;
    LispOpaque *opaque;

    if (type) {
	for (i = 0; i < STRTBLSZ; i++) {
	    opaque = mac->opqs[i];
	    while (opaque) {
		if (opaque->type == type)
		    return (opaque->desc);
		opaque = opaque->next;
	    }
	}
	LispDestroy(mac, "Opaque type %d not registered", type);
    }

    return (Snil);
}

char *
LispGetAtomString(LispMac *mac, char *string, int perm)
{
    LispStringHash *entry;
    int ii = 0;
    char *pp = string;

    while (*pp)
	ii = (ii << 1) ^ *pp++;
    if (ii < 0)
	ii = -ii;
    ii %= STRTBLSZ;

    for (entry = mac->strings[ii]; entry != NULL; entry = entry->next)
	if (strcmp(entry->string, string) == 0)
	    return (entry->string);

    entry = (LispStringHash*)LispCalloc(mac, 1, sizeof(LispStringHash));
    if (perm)
	entry->string = string;
    else
	entry->string = LispStrdup(mac, string);
    LispMused(mac, entry);
    if (!perm)
	LispMused(mac, entry->string);
    entry->next = mac->strings[ii];
    mac->strings[ii] = entry;

    return (entry->string);
}

LispAtom *
LispDoGetAtom(LispMac *mac, char *str, int perm)
{
    LispAtom *atom;
    int ii = 0;
    char *pp = str;

    while (*pp)
	ii = (ii << 1) ^ *pp++;
    if (ii < 0)
	ii = -ii;
    ii %= STRTBLSZ;
    for (atom = mac->pack->atoms[ii]; atom; atom = atom->next)
	if (strcmp(atom->string, str) == 0)
	    return (atom);

    atom = (LispAtom*)LispCalloc(mac, 1, sizeof(LispAtom));
    atom->string = LispGetAtomString(mac, str, perm);
    LispMused(mac, atom);
    atom->next = mac->pack->atoms[ii];
    mac->pack->atoms[ii] = atom;
    atom->property = NOPROPERTY;

    return (atom);
}

static void
LispAllocAtomProperty(LispMac *mac, LispAtom *atom)
{
    LispProperty *property;

    if (atom->property != NOPROPERTY)
	LispDestroy(mac, "internal error at ALLOC-ATOM-PROPERTY");

    property = LispCalloc(mac, 1, sizeof(LispProperty));
    LispMused(mac, property);
    atom->property = property;
    property->package = mac->pack;
    if (atom->package == NULL)
	atom->package = PACKAGE;

    LispIncrementAtomReference(mac, atom);
}

static void
LispIncrementAtomReference(LispMac *mac, LispAtom *atom)
{
    if (atom->property != NOPROPERTY)
	/* if atom->property is NOPROPERTY, this is an unbound symbol */
	++atom->property->refcount;
}

/* Assumes atom property is not NOPROPERTY */
static void
LispDecrementAtomReference(LispMac *mac, LispAtom *atom)
{
    if (atom->property == NOPROPERTY)
	/* if atom->property is NOPROPERTY, this is an unbound symbol */
	return;

    --atom->property->refcount;

    if (atom->property->refcount < 0)
	LispDestroy(mac, "internal error at DECREMENT-ATOM-REFERENCE");

    if (atom->property->refcount == 0) {
	LispRemAtomAllProperties(mac, atom);
	free(atom->property);
	atom->property = NOPROPERTY;
    }
}

static void
LispRemAtomAllProperties(LispMac *mac, LispAtom *atom)
{
    if (atom->property != NOPROPERTY) {
	if (atom->a_object)
	    LispRemAtomObjectProperty(mac, atom);
	if (atom->a_function)
	    LispRemAtomFunctionProperty(mac, atom);
	else if (atom->a_builtin)
	    LispRemAtomBuiltinProperty(mac, atom);
	if (atom->a_defsetf)
	    LispRemAtomSetfProperty(mac, atom);
	if (atom->a_defstruct)
	    LispRemAtomStructProperty(mac, atom);
    }
}

void
LispSetAtomObjectProperty(LispMac *mac, LispAtom *atom, LispObj *object)
{
    if (atom->property == NOPROPERTY)
	LispAllocAtomProperty(mac, atom);
    else if (atom->watch) {
	if (atom->object == mac->package) {
	    if (!PACKAGE_P(object))
		LispDestroy(mac, "Symbol %s must be a package, not %s",
			    STRPTR(mac->package), STROBJ(object));
	    mac->pack = object->data.package.package;
	}
    }

    atom->a_object = 1;
    SETVALUE(atom, object);
}

static void
LispRemAtomObjectProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->a_object) {
	atom->a_object = 0;
	atom->property->value = NULL;
    }
}

void
LispSetAtomCompiledProperty(LispMac *mac, LispAtom *atom, LispObj *bytecode)
{
    if (atom->property == NOPROPERTY)
	LispAllocAtomProperty(mac, atom);

    mac->gc.immutablebits = 1;
    atom->a_function = 0;
    LispMutable(atom->property->fun.function);

    LispImmutable(bytecode);

    atom->a_compiled = 1;
    atom->property->fun.function = bytecode;
}

void
LispRemAtomCompiledProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->a_compiled) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property->fun.function);
	atom->property->fun.function = NULL;
	atom->a_compiled = 0;
	LispFreeArgList(mac, atom->property->alist);
	atom->property->alist = NULL;
    }
}

void
LispSetAtomFunctionProperty(LispMac *mac, LispAtom *atom, LispObj *function,
			    LispArgList *alist)
{
    if (atom->property == NOPROPERTY)
	LispAllocAtomProperty(mac, atom);

    if (atom->a_function == 0 && atom->a_builtin == 0 && atom->a_compiled == 0)
	atom->a_function = 1;
    else {
	mac->gc.immutablebits = 1;
	if (atom->a_builtin) {
	    LispMutable(atom->property->fun.builtin->data);
	    atom->a_builtin = 0;
	    LispFreeArgList(mac, atom->property->alist);
	}
	else {
	    atom->a_compiled = 0;
	    LispMutable(atom->property->fun.function);
	    atom->a_function = 1;
	}
    }

    LispImmutable(function);

    atom->property->fun.function = function;
    atom->property->alist = alist;
}

void
LispRemAtomFunctionProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->a_function) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property->fun.function);
	atom->property->fun.function = NULL;
	atom->a_function = 0;
	LispFreeArgList(mac, atom->property->alist);
	atom->property->alist = NULL;
    }
}

void
LispSetAtomBuiltinProperty(LispMac *mac, LispAtom *atom, LispBuiltin *builtin,
			   LispArgList *alist)
{
    if (atom->property == NOPROPERTY)
	LispAllocAtomProperty(mac, atom);

    if (atom->a_builtin == 0 && atom->a_function == 0)
	atom->a_builtin = 1;
    else {
	mac->gc.immutablebits = 1;
	if (atom->a_function) {
	    LispMutable(atom->property->fun.function);
	    atom->a_function = 0;
	    LispFreeArgList(mac, atom->property->alist);
	}
	else
	    LispMutable(atom->property->fun.builtin->data);
    }

    LispImmutable(builtin->data);

    atom->property->fun.builtin = builtin;
    atom->property->alist = alist;
}

void
LispRemAtomBuiltinProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->a_builtin) {
	mac->gc.immutablebits = 1;
	LispImmutable(atom->property->fun.builtin->data);
	atom->property->fun.function = NULL;
	atom->a_builtin = 0;
	LispFreeArgList(mac, atom->property->alist);
	atom->property->alist = NULL;
    }
}

void
LispSetAtomSetfProperty(LispMac *mac, LispAtom *atom, LispObj *setf,
			LispArgList *alist)
{
    if (atom->property == NOPROPERTY)
	LispAllocAtomProperty(mac, atom);

    if (atom->a_defsetf) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property->setf);
	LispFreeArgList(mac, atom->property->salist);
    }

    LispImmutable(setf);

    atom->a_defsetf = 1;
    atom->property->setf = setf;
    atom->property->salist = alist;
}

void
LispRemAtomSetfProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->a_defsetf) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property->setf);
	atom->property->setf = NULL;
	atom->a_defsetf = 0;
	LispFreeArgList(mac, atom->property->salist);
	atom->property->salist = NULL;
    }
}

void
LispSetAtomStructProperty(LispMac *mac, LispAtom *atom, LispObj *def, int fun)
{
    if (fun > 0xff)
	/* Not suported by the bytecode compiler... */
	LispDestroy(mac, "SET-ATOM-STRUCT-PROPERTY: "
		    "more than 256 fields not supported");

    if (atom->property == NOPROPERTY)
	LispAllocAtomProperty(mac, atom);

    if (atom->a_defstruct) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->object);
	LispMutable(atom->property->structure.definition);
    }

    atom->a_defstruct = 1;
    atom->property->structure.definition = def;
    atom->property->structure.function = fun;

    LispImmutable(atom->object);
    LispImmutable(def);
}

void
LispRemAtomStructProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->a_defstruct) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->object);
	LispMutable(atom->property->structure.definition);
	atom->property->structure.definition = NULL;
	atom->a_defstruct = 0;
    }
}

LispAtom *
LispGetAtom(LispMac *mac, char *str)
{
    return (LispDoGetAtom(mac, str, 0));
}

LispAtom *
LispGetPermAtom(LispMac *mac, char *str)
{
    return (LispDoGetAtom(mac, str, 1));
}

static LispObj *
LispDoGetAtomProperty(LispMac *mac, LispAtom *atom, LispObj *key, int add)
{
    LispObj *obj = NIL, *res = NIL;

    if (add && atom->property->properties == NULL) {
	atom->a_property = 1;
	atom->property->properties = NIL;
    }

    if (atom->a_property) {
	for (obj = atom->property->properties; obj != NIL; obj = CDR(CDR(obj))) {
	    if (XEQUAL(key, CAR(obj)) == T) {
		res = CDR(obj);
		break;
	    }
	}
    }

    if (obj == NIL) {
	if (add) {
	    atom->property->properties =
		CONS(key, CONS(NIL, atom->property->properties));
	    res = CDR(atom->property->properties);
	}
    }

    return (res);
}

LispObj *
LispGetAtomProperty(LispMac *mac, LispAtom *atom, LispObj *key)
{
    return (LispDoGetAtomProperty(mac, atom, key, 0));
}

LispObj *
LispPutAtomProperty(LispMac *mac, LispAtom *atom, LispObj *key, LispObj *val)
{
    LispObj *res = LispDoGetAtomProperty(mac, atom, key, 1);

    RPLACA(res, val);

    return (res);
}

/* Used to make sure that when defining a function like:
 *	(defun my-function (... &key key1 key2 key3 ...)
 * key1, key2, and key3 will be in the keyword package
 */
static LispObj *
LispCheckKeyword(LispMac *mac, LispObj *keyword)
{
    if (KEYWORD_P(keyword))
	return (keyword);

    return (KEYWORD(STRPTR(keyword)));
}

void
LispUseArgList(LispMac *mac, LispArgList *alist)
{
    if (alist->normals.num_symbols)
	LispMused(mac, alist->normals.symbols);
    if (alist->optionals.num_symbols) {
	LispMused(mac, alist->optionals.symbols);
	LispMused(mac, alist->optionals.defaults);
	LispMused(mac, alist->optionals.sforms);
    }
    if (alist->keys.num_symbols) {
	LispMused(mac, alist->keys.symbols);
	LispMused(mac, alist->keys.defaults);
	LispMused(mac, alist->keys.sforms);
	LispMused(mac, alist->keys.keys);
    }
    if (alist->auxs.num_symbols) {
	LispMused(mac, alist->auxs.symbols);
	LispMused(mac, alist->auxs.initials);
    }
    LispMused(mac, alist);
}

void
LispFreeArgList(LispMac *mac, LispArgList *alist)
{
    if (alist->normals.num_symbols)
	LispFree(mac, alist->normals.symbols);
    if (alist->optionals.num_symbols) {
	LispFree(mac, alist->optionals.symbols);
	LispFree(mac, alist->optionals.defaults);
	LispFree(mac, alist->optionals.sforms);
    }
    if (alist->keys.num_symbols) {
	LispFree(mac, alist->keys.symbols);
	LispFree(mac, alist->keys.defaults);
	LispFree(mac, alist->keys.sforms);
	LispFree(mac, alist->keys.keys);
    }
    if (alist->auxs.num_symbols) {
	LispFree(mac, alist->auxs.symbols);
	LispFree(mac, alist->auxs.initials);
    }
    LispFree(mac, alist);
}

static LispObj *
LispCheckNeedProtect(LispObj *object)
{
    if (object) {
	switch (object->type) {
	    case LispNil_t:
	    case LispTrue_t:
	    case LispAtom_t:
	    case LispCharacter_t:
		return (NULL);
	    default:
		return (object);
	}
    }
    return (NULL);
}

LispObj *
LispListProtectedArguments(LispMac *mac, LispArgList *alist)
{
    int i;
    GC_ENTER();
    LispObj *arguments, *cons, *obj, *prev;

    arguments = cons = prev = NIL;
    for (i = 0; i < alist->optionals.num_symbols; i++) {
	if ((obj = LispCheckNeedProtect(alist->optionals.defaults[i])) != NULL) {
	    if (arguments == NIL) {
		arguments = cons = prev = CONS(obj, NIL);
		GC_PROTECT(arguments);
	    }
	    else {
		RPLACD(cons, CONS(obj, NIL));
		prev = cons;
		cons = CDR(cons);
	    }
	}
    }
    for (i = 0; i < alist->keys.num_symbols; i++) {
	if ((obj = LispCheckNeedProtect(alist->keys.defaults[i])) != NULL) {
	    if (arguments == NIL) {
		arguments = cons = prev = CONS(obj, NIL);
		GC_PROTECT(arguments);
	    }
	    else {
		RPLACD(cons, CONS(obj, NIL));
		prev = cons;
		cons = CDR(cons);
	    }
	}
    }
    for (i = 0; i < alist->auxs.num_symbols; i++) {
	if ((obj = LispCheckNeedProtect(alist->auxs.initials[i])) != NULL) {
	    if (arguments == NIL) {
		arguments = cons = prev = CONS(obj, NIL);
		GC_PROTECT(arguments);
	    }
	    else {
		RPLACD(cons, CONS(obj, NIL));
		prev = cons;
		cons = CDR(cons);
	    }
	}
    }
    GC_LEAVE();

    /* Don't add a NIL cell at the end, to save some space */
    if (arguments != NIL) {
	if (arguments == cons)
	    arguments = CAR(cons);
	else
	    CDR(prev) = CAR(cons);
    }

    return (arguments);
}

LispArgList *
LispCheckArguments(LispMac *mac, LispFunType type, LispObj *list, char *name)
{
    static char *types[4] = {"LAMBDA-LIST", "FUNCTION", "MACRO", "SETF-METHOD"};
    static char *fnames[4] = {"LAMBDA", "DEFUN", "DEFMACRO", "DEFSETF"};
#define IKEY		0
#define IOPTIONAL	1
#define IREST		2
#define IAUX		3
    static char *keys[4] = {"&KEY", "&OPTIONAL", "&REST", "&AUX"};
    int rest, optional, key, aux, count;
    LispArgList *alist;
    LispObj *spec, *sform, *defval;
    char description[8], *desc;

/* If LispRealloc fails, the previous memory will be released
 * in LispTopLevel, unless LispMused was called on the pointer */
#define REALLOC_OBJECTS(pointer, count)		\
    pointer = LispRealloc(mac, pointer, (count) * sizeof(LispObj*))

    alist = LispCalloc(mac, 1, sizeof(LispArgList));
    if (!CONS_P(list)) {
	if (list != NIL)
	    LispDestroy(mac, "%s %s: %s cannot be a %s argument list",
			fnames[type], name, STROBJ(list), types[type]);
	alist->description = GETATOMID("");

	return (alist);
    }

    description[0] = '\0';
    desc = description;
    rest = optional = key = aux = 0;
    for (; CONS_P(list); list = CDR(list)) {
	spec = CAR(list);

	if (CONS_P(spec)) {
	    if (aux) {
		if (!SYMBOL_P(CAR(spec)) ||
		    (CDR(spec) != NIL && CDDR(spec) != NIL))
		    LispDestroy(mac, "%s %s: bad &AUX argument %s",
				fnames[type], name, STROBJ(spec));
		defval = CDR(spec) != NIL ? CADR(spec) : NIL;
		count = alist->auxs.num_symbols;
		REALLOC_OBJECTS(alist->auxs.symbols, count + 1);
		REALLOC_OBJECTS(alist->auxs.initials, count + 1);
		alist->auxs.symbols[count] = CAR(spec);
		alist->auxs.initials[count] = defval;
		++alist->auxs.num_symbols;
		if (count == 0)
		    *desc++ = 'a';
		++alist->num_arguments;
	    }
	    else if (rest)
		LispDestroy(mac, "%s %s: syntax error parsing %s",
			    fnames[type], name, keys[IREST]);
	    else if (key) {
		LispObj *akey = CAR(spec);

		defval = NIL;
		sform = NULL;
		if (CONS_P(akey)) {
		    /* check for special case, as in:
		     *	(defun a (&key ((key name) 'default-value)) name)
		     *	(a 'key 'test)	=> TEST
		     *	(a)		=> DEFAULT-VALUE
		     */
		    if (!SYMBOL_P(CAR(akey)) || !CONS_P(CDR(akey)) ||
			!SYMBOL_P(CADR(akey)) || CDDR(akey) != NIL ||
			(CDR(spec) != NIL && CDDR(spec) != NIL))
			LispDestroy(mac, "%s %s: bad special &KEY %s",
				    fnames[type], name, STROBJ(spec));
		    if (CDR(spec) != NIL)
			defval = CADR(spec);
		    spec = CADR(akey);
		    akey = CAR(akey);
		}
		else {
		    akey = NULL;

		    if (!SYMBOL_P(CAR(spec)))
			LispDestroy(mac,
				    "%s %s: %s cannot be a %s argument name",
				    fnames[type], name,
				    STROBJ(CAR(spec)), types[type]);
		    /* check if default value provided, and optionally a `svar' */
		    else if (CDR(spec) != NIL && (!CONS_P(CDR(spec)) ||
			      (CDDR(spec) != NIL &&
			       (!SYMBOL_P(CAR(CDDR(spec))) ||
				CDR(CDDR(spec)) != NIL))))
			LispDestroy(mac, "%s %s: bad argument specification %s",
				    fnames[type], name, STROBJ(spec));
		    if (CONS_P(CDR(spec))) {
			defval = CADR(spec);
			if (CONS_P(CDDR(spec)))
			    sform = CAR(CDDR(spec));
		    }
		    /* Add to keyword package, and set the keyword in the
		     * argument list, so that a function argument keyword
		     * will reference the same object, and make comparison
		     * simpler. */
		    spec = LispCheckKeyword(mac, CAR(spec));
		}

		count = alist->keys.num_symbols;
		REALLOC_OBJECTS(alist->keys.keys, count + 1);
		REALLOC_OBJECTS(alist->keys.defaults, count + 1);
		REALLOC_OBJECTS(alist->keys.sforms, count + 1);
		REALLOC_OBJECTS(alist->keys.symbols, count + 1);
		alist->keys.symbols[count] = spec;
		alist->keys.defaults[count] = defval;
		alist->keys.sforms[count] = sform;
		alist->keys.keys[count] = akey;
		++alist->keys.num_symbols;
		if (count == 0)
		    *desc++ = 'k';
		alist->num_arguments += 1 + (sform != NULL);
	    }
	    else if (optional) {
		defval = NIL;
		sform = NULL;

		if (!SYMBOL_P(CAR(spec)))
		    LispDestroy(mac,
				"%s %s: %s cannot be a %s argument name",
				fnames[type], name,
				STROBJ(CAR(spec)), types[type]);
		/* check if default value provided, and optionally a `svar' */
		else if (CDR(spec) != NIL && (!CONS_P(CDR(spec)) ||
			  (CDDR(spec) != NIL &&
			   (!SYMBOL_P(CAR(CDDR(spec))) ||
			    CDR(CDDR(spec)) != NIL))))
		    LispDestroy(mac, "%s %s: bad argument specification %s",
				fnames[type], name, STROBJ(spec));
		if (CONS_P(CDR(spec))) {
		    defval = CADR(spec);
		    if (CONS_P(CDDR(spec)))
			sform = CAR(CDDR(spec));
		}
		spec = CAR(spec);

		count = alist->optionals.num_symbols;
		REALLOC_OBJECTS(alist->optionals.symbols, count + 1);
		REALLOC_OBJECTS(alist->optionals.defaults, count + 1);
		REALLOC_OBJECTS(alist->optionals.sforms, count + 1);
		alist->optionals.symbols[count] = spec;
		alist->optionals.defaults[count] = defval;
		alist->optionals.sforms[count] = sform;
		++alist->optionals.num_symbols;
		if (count == 0)
		    *desc++ = 'o';
		alist->num_arguments += 1 + (sform != NULL);
	    }

	    /* Normal arguments cannot have default value */
	    else
		LispDestroy(mac, "%s %s: syntax error parsing %s",
			    fnames[type], name, STROBJ(spec));
	}

	/* spec must be an atom, excluding keywords */
	else if (!SYMBOL_P(spec) || KEYWORD_P(spec))
	    LispDestroy(mac, "%s %s: %s cannot be a %s argument",
			fnames[type], name, STROBJ(spec), types[type]);
	else {
	    Atom_id atom = ATOMID(spec);

	    if (atom[0] == '&') {
		if (atom == Srest) {
		    if (rest || aux || CDR(list) == NIL || !SYMBOL_P(CADR(list))
			/* only &aux allowed after &rest */
			|| (CDDR(list) != NIL && !SYMBOL_P(CAR(CDDR(list))) &&
			    STRPTR(CAR(CDDR(list))) != Saux))
			LispDestroy(mac, "%s %s: syntax error parsing %s",
				    fnames[type], name, STRPTR(spec));
		    if (key)
			LispDestroy(mac, "%s %s: %s not allowed after %s",
				    fnames[type], name, keys[IREST], keys[IKEY]);
		    rest = 1;
		    continue;
		}

		else if (atom == Skey) {
		    if (rest || aux)
			LispDestroy(mac, "%s %s: %s not allowed after %s",
				    fnames[type], name, STRPTR(spec),
				    rest ? keys[IREST] : keys[IAUX]);
		    key = 1;
		    continue;
		}

		else if (atom == Soptional) {
		    if (rest || optional || aux || key)
			LispDestroy(mac, "%s %s: %s not allowed after %s",
				    fnames[type], name, STRPTR(spec),
				    rest ? keys[IREST] :
					optional ?
					keys[IOPTIONAL] :
					    aux ? keys[IAUX] : keys[IKEY]);
		    optional = 1;
		    continue;
		}

		else if (atom == Saux) {
		    /* &AUX must be the last keyword parameter */
		    if (aux)
			LispDestroy(mac, "%s %s: syntax error parsing %s",
				    fnames[type], name, STRPTR(spec));
		    aux = 1;
		    continue;
		}

		/* Untill more lambda-list keywords supported, don't allow
		 * argument names starting with the '&' character */
		else
		    LispDestroy(mac, "%s %s: %s not allowed/implemented",
				fnames[type], name, STRPTR(spec));
	    }

	    /* Add argument to alist */
	    if (aux) {
		count = alist->auxs.num_symbols;
		REALLOC_OBJECTS(alist->auxs.symbols, count + 1);
		REALLOC_OBJECTS(alist->auxs.initials, count + 1);
		alist->auxs.symbols[count] = spec;
		alist->auxs.initials[count] = NIL;
		++alist->auxs.num_symbols;
		if (count == 0)
		    *desc++ = 'a';
		++alist->num_arguments;
	    }
	    else if (rest) {
		alist->rest = spec;
		*desc++ = 'r';
		++alist->num_arguments;
	    }
	    else if (key) {
		/* Add to keyword package, and set the keyword in the
		 * argument list, so that a function argument keyword
		 * will reference the same object, and make comparison
		 * simpler. */
		spec = LispCheckKeyword(mac, spec);
		count = alist->keys.num_symbols;
		REALLOC_OBJECTS(alist->keys.keys, count + 1);
		REALLOC_OBJECTS(alist->keys.defaults, count + 1);
		REALLOC_OBJECTS(alist->keys.sforms, count + 1);
		REALLOC_OBJECTS(alist->keys.symbols, count + 1);
		alist->keys.symbols[count] = spec;
		alist->keys.defaults[count] = NIL;
		alist->keys.sforms[count] = NULL;
		alist->keys.keys[count] = NULL;
		++alist->keys.num_symbols;
		if (count == 0)
		    *desc++ = 'k';
		++alist->num_arguments;
	    }
	    else if (optional) {
		count = alist->optionals.num_symbols;
		REALLOC_OBJECTS(alist->optionals.symbols, count + 1);
		REALLOC_OBJECTS(alist->optionals.defaults, count + 1);
		REALLOC_OBJECTS(alist->optionals.sforms, count + 1);
		alist->optionals.symbols[count] = spec;
		alist->optionals.defaults[count] = NIL;
		alist->optionals.sforms[count] = NULL;
		++alist->optionals.num_symbols;
		if (count == 0)
		    *desc++ = 'o';
		++alist->num_arguments;
	    }
	    else {
		count = alist->normals.num_symbols;
		REALLOC_OBJECTS(alist->normals.symbols, count + 1);
		alist->normals.symbols[count] = spec;
		++alist->normals.num_symbols;
		if (count == 0)
		    *desc++ = '.';
		++alist->num_arguments;
	    }
	}
    }

    /* Check for dotted argument list */
    if (list != NIL)
	LispDestroy(mac, "%s %s: %s cannot end %s arguments",
		    fnames[type], name, STROBJ(list), types[type]);

    *desc = '\0';
    alist->description = LispGetAtomString(mac, description, 0);

    return (alist);
}

void
LispAddBuiltinFunction(LispMac *mac, LispBuiltin *builtin)
{
    static LispObj stream;
    static LispString string;
    static int first = 1;
    LispObj *name, *obj, *list, *cons, *code;
    LispAtom *atom;
    LispArgList *alist;
    int length = mac->protect.length;

    if (first) {
	stream.type = LispStream_t;
	stream.data.stream.source.string = &string;
	stream.data.stream.pathname = NIL;
	stream.data.stream.type = LispStreamString;
	stream.data.stream.readable = 1;
	stream.data.stream.writable = 0;
	string.output = 0;
	first = 0;
    }
    string.string = (unsigned char*)builtin->declaration;
    string.length = strlen(builtin->declaration);
    string.input = 0;

    code = COD;
    LispPushInput(mac, &stream);
    name = LispRead(mac);
    list = cons = CONS(name, NIL);
    if (length + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = list;
    while ((obj = LispRead(mac)) != NULL) {
	RPLACD(cons, CONS(obj, NIL));
	cons = CDR(cons);
    }
    LispPopInput(mac, &stream);

    atom = name->data.atom;
    alist = LispCheckArguments(mac, builtin->type, CDR(list), atom->string);
    builtin->symbol = CAR(list);
    builtin->data = LispListProtectedArguments(mac, alist);
    LispSetAtomBuiltinProperty(mac, atom, builtin, alist);
    LispUseArgList(mac, alist);

    /* Make function a extern symbol, unless told to not do so */
    if (!builtin->internal)
	LispExportSymbol(mac, name);

    mac->protect.length = length;
    COD = code;			/* LispRead protect data in COD */
}

void
LispAllocSeg(LispMac *mac, LispObjSeg *seg, int cellcount)
{
    unsigned int i;
    LispObj **list, *obj;

    while (seg->nfree < cellcount) {
	if ((obj = (LispObj*)calloc(1, sizeof(LispObj) * segsize)) == NULL)
	    LispDestroy(mac, "out of memory");
	if ((list = (LispObj**)realloc(seg->objects,
	    sizeof(LispObj*) * (seg->nsegs + 1))) == NULL) {
	    free(obj);
	    LispDestroy(mac, "out of memory");
	}
	seg->objects = list;
	seg->objects[seg->nsegs] = obj;

	seg->nfree += segsize;
	seg->nobjs += segsize;
	for (i = 1; i < segsize; i++, obj++) {
	    /* Objects of type cons are the most used, save some time
	     * by not setting it's type in LispNewCons. */
	    obj->type = LispCons_t;
	    CDR(obj) = obj + 1;
	}
	obj->type = LispCons_t;
	CDR(obj) = seg->freeobj;
	seg->freeobj = seg->objects[seg->nsegs];
	++seg->nsegs;
    }
#ifdef DEBUG
    LispMessage(mac, "gc: %d cell(s) allocated at %d segment(s)",
		seg->nobjs, seg->nsegs);
#endif
}

static INLINE void
LispMark(register LispObj *obj)
{
mark_again:
    switch (obj->type) {
	case LispNil_t:		/* only NIL and UNBOUND (should) have this type */
	case LispTrue_t:	/* only T (should) has this type */
	case LispAtom_t:	/* atoms are never release, once created */
	case LispCharacter_t:
	    return;
	case LispLambda_t:
	    obj->data.lambda.name->mark = LispTrue_t;
	    obj->mark = LispTrue_t;
	    LispMark(obj->data.lambda.data);
	    obj = obj->data.lambda.code;
	    goto mark_cons;
	case LispQuote_t:
	case LispBackquote_t:
	    obj->mark = LispTrue_t;
	    obj = obj->data.quote;
	    goto mark_again;
	case LispPathname_t:
	    obj->mark = LispTrue_t;
	    obj = obj->data.pathname;
	    goto mark_again;
	case LispComma_t:
	    obj->mark = LispTrue_t;
	    obj = obj->data.comma.eval;
	    goto mark_again;
	case LispComplex_t:
	    obj->data.complex.real->mark = LispTrue_t;
	    obj->data.complex.imag->mark = LispTrue_t;
	    break;
	case LispCons_t:
mark_cons:
	    for (; !obj->mark && CONS_P(obj); obj = CDR(obj)) {
		switch (CAR(obj)->type) {
		    case LispNil_t:
		    case LispTrue_t:
		    case LispAtom_t:
		    case LispCharacter_t:
		    case LispPackage_t:		/* Protected in Lisp__GC */
			break;
		    case LispInteger_t:
		    case LispReal_t:
		    case LispString_t:
		    case LispRatio_t:
		    case LispOpaque_t:
		    case LispBigInteger_t:
		    case LispBigRatio_t:
			CAR(obj)->mark = LispTrue_t;
			break;
		    default:
			LispMark(CAR(obj));
			break;
		}
		obj->mark = LispTrue_t;
	    }
	    if (!obj->mark)
		goto mark_again;
	    return;
	case LispArray_t:
	    LispMark(obj->data.array.list);
	    obj->mark = LispTrue_t;
	    obj = obj->data.array.dim;
	    goto mark_cons;
	case LispStruct_t:
	    obj->mark = LispTrue_t;
	    obj = obj->data.struc.fields;
	    goto mark_cons;
	case LispStream_t:
mark_stream:
	    LispMark(obj->data.stream.pathname);
	    if (obj->data.stream.type == LispStreamPipe) {
		obj->mark = LispTrue_t;
		obj = obj->data.stream.source.program->errorp;
		goto mark_stream;
	    }
	    break;
	case LispRegex_t:
	    obj->data.regex.pattern->mark = LispTrue_t;
	    break;
	case LispBytecode_t:
	    obj->mark = LispTrue_t;
	    obj = obj->data.bytecode.code;
	    goto mark_cons;
	default:
	    break;
    }
    obj->mark = LispTrue_t;
}

static INLINE void
LispImmutable(register LispObj *obj)
{
immutable_again:
    switch (obj->type) {
	case LispNil_t:		/* only NIL and UNBOUND (should) have this type */
	case LispTrue_t:	/* only T (should) has this type */
	case LispAtom_t:	/* atoms are never release, once created */
	case LispCharacter_t:
	    return;
	case LispLambda_t:
	    obj->data.lambda.name->prot = LispTrue_t;
	    obj->prot = LispTrue_t;
	    LispImmutable(obj->data.lambda.data);
	    obj = obj->data.lambda.code;
	    goto immutable_again;
	case LispQuote_t:
	case LispBackquote_t:
	    obj->prot = LispTrue_t;
	    obj = obj->data.quote;
	    goto immutable_again;
	case LispPathname_t:
	    obj->prot = LispTrue_t;
	    obj = obj->data.pathname;
	    goto immutable_again;
	case LispComma_t:
	    obj->prot = LispTrue_t;
	    obj = obj->data.comma.eval;
	    goto immutable_again;
	case LispComplex_t:
	    obj->data.complex.real->prot = LispTrue_t;
	    obj->data.complex.imag->prot = LispTrue_t;
	    break;
	case LispCons_t:
	    for (; CONS_P(obj); obj = CDR(obj)) {
		if (CAR(obj)->type <= LispAtom_t)
		    CAR(obj)->prot = LispTrue_t;
		else
		    LispImmutable(CAR(obj));
		obj->prot = LispTrue_t;
	    }
	    if (obj != NIL)
		goto immutable_again;
	    return;
	case LispArray_t:
	    LispImmutable(obj->data.array.list);
	    obj->prot = LispTrue_t;
	    obj = obj->data.array.dim;
	    goto immutable_again;
	case LispStruct_t:
	    obj->prot = LispTrue_t;
	    obj = obj->data.struc.fields;
	    goto immutable_again;
	case LispStream_t:
	    LispImmutable(obj->data.stream.pathname);
	    if (obj->data.stream.type == LispStreamPipe) {
		obj->prot = LispTrue_t;
		obj = obj->data.stream.source.program->errorp;
		goto immutable_again;
	    }
	    break;
	case LispRegex_t:
	    obj->data.regex.pattern->prot = LispTrue_t;
	    break;
	case LispBytecode_t:
	    obj->prot = LispTrue_t;
	    obj = obj->data.bytecode.code;
	    goto immutable_again;
	default:
	    break;
    }
    obj->prot = LispTrue_t;
}

static INLINE void
LispMutable(register LispObj *obj)
{
mutable_again:
    switch (obj->type) {
	case LispNil_t:		/* only NIL and UNBOUND (should) have this type */
	case LispTrue_t:	/* only T (should) has this type */
	case LispAtom_t:	/* atoms are never release, once created */
	case LispCharacter_t:
	    return;
	case LispLambda_t:
	    obj->data.lambda.name->prot = LispNil_t;
	    obj->prot = LispNil_t;
	    LispMutable(obj->data.lambda.data);
	    obj = obj->data.lambda.code;
	    goto mutable_again;
	case LispQuote_t:
	case LispBackquote_t:
	    obj->prot = LispNil_t;
	    obj = obj->data.quote;
	    goto mutable_again;
	case LispPathname_t:
	    obj->prot = LispNil_t;
	    obj = obj->data.pathname;
	    goto mutable_again;
	case LispComma_t:
	    obj->prot = LispNil_t;
	    obj = obj->data.comma.eval;
	    goto mutable_again;
	case LispComplex_t:
	    obj->data.complex.real->prot = LispNil_t;
	    obj->data.complex.imag->prot = LispNil_t;
	    break;
	case LispCons_t:
	    for (; CONS_P(obj); obj = CDR(obj)) {
		if (CAR(obj)->type <= LispAtom_t)
		    CAR(obj)->prot = LispNil_t;
		else
		    LispImmutable(CAR(obj));
		obj->prot = LispNil_t;
	    }
	    if (obj != NIL)
		goto mutable_again;
	    return;
	case LispArray_t:
	    LispImmutable(obj->data.array.list);
	    obj->prot = LispNil_t;
	    obj = obj->data.array.dim;
	    goto mutable_again;
	case LispStruct_t:
	    obj->prot = LispNil_t;
	    obj = obj->data.struc.fields;
	    goto mutable_again;
	case LispStream_t:
	    LispImmutable(obj->data.stream.pathname);
	    if (obj->data.stream.type == LispStreamPipe) {
		obj->prot = LispNil_t;
		obj = obj->data.stream.source.program->errorp;
		goto mutable_again;
	    }
	    break;
	case LispRegex_t:
	    obj->data.regex.pattern->prot = LispNil_t;
	    break;
	case LispBytecode_t:
	    obj->prot = LispNil_t;
	    obj = obj->data.bytecode.code;
	    goto mutable_again;
	default:
	    break;
    }
    obj->prot = LispNil_t;
}

void
LispProtect(LispMac *mac, LispObj *key, LispObj *list)
{
    PRO = CONS(CONS(key, list), PRO);
}

void
LispUProtect(LispMac *mac, LispObj *key, LispObj *list)
{
    LispObj *prev, *obj;

    for (prev = obj = PRO; obj != NIL; prev = obj, obj = CDR(obj))
	if (CAR(CAR(obj)) == key && CDR(CAR(obj)) == list) {
	    if (obj == PRO)
		PRO = CDR(PRO);
	    else
		CDR(prev) = CDR(obj);
	    return;
	}

    LispDestroy(mac, "no match for %s, at UPROTECT", STROBJ(key));
}

LispObj *
Lisp__New(LispMac *mac, LispObj *car, LispObj *cdr)
{
    int cellcount;
    LispObj *obj;

    Lisp__GC(mac, car, cdr);
#if 0
    mac->gc.average = (objseg.nfree + mac->gc.average) >> 1;
    if (mac->gc.average < minfree) {
	if (mac->gc.expandbits < 6)
	    ++mac->gc.expandbits;
    }
    else if (mac->gc.expandbits)
	--mac->gc.expandbits;
    /* For 32 bit computers, where sizeof(LispObj) == 16,
     * minfree is set to 1024, and expandbits limited to 6,
     * the maximum extra memory requested here should be 1Mb
     */
    cellcount = minfree << mac->gc.expandbits;
#else
    /* Try to keep at least 3 times more free cells than the de number
     * of used cells in the freelist, to amenize the cost of the gc time,
     * in the, currently, very simple gc strategy code. */
    cellcount = (objseg.nobjs - objseg.nfree) * 3;
    cellcount = cellcount + (minfree - (cellcount % minfree));
#endif
    if (objseg.freeobj == NIL || objseg.nfree < cellcount)
	LispAllocSeg(mac, &objseg, cellcount);

    obj = objseg.freeobj;
    objseg.freeobj = CDR(obj);

    return (obj);
}

LispObj *
LispNew(LispMac *mac, LispObj *car, LispObj *cdr)
{
    LispObj *obj = objseg.freeobj;

    if (obj == NIL)
	obj = Lisp__New(mac, car, cdr);
    else
	objseg.freeobj = CDR(obj);

    return (obj);
}

LispObj *
LispNewAtom(LispMac *mac, char *str, int intern)
{
    LispObj *object;
    LispAtom *atom = LispDoGetAtom(mac, str, 0);

    if (atom->object) {
	if (intern && atom->package == NULL)
	    atom->package = PACKAGE;

	return (atom->object);
    }

    if (atomseg.freeobj == NIL)
	LispAllocSeg(mac, &atomseg, pagesize);
    object = atomseg.freeobj;
    atomseg.freeobj = CDR(object);
    --atomseg.nfree;

    object->type = LispAtom_t;
    object->data.atom = atom;
    atom->object = object;
    if (intern)
	atom->package = PACKAGE;

    return (object);
}

LispObj *
LispNewStaticAtom(LispMac *mac, char *str)
{
    LispObj *object;
    LispAtom *atom = LispDoGetAtom(mac, str, 1);

    object = LispNewSymbol(mac, atom);

    return (object);
}

LispObj *
LispNewSymbol(LispMac *mac, LispAtom *atom)
{
    if (atom->object) {
	if (atom->package == NULL)
	    atom->package = PACKAGE;

	return (atom->object);
    }
    else {
	LispObj *symbol;

	if (atomseg.freeobj == NIL)
	    LispAllocSeg(mac, &atomseg, pagesize);
	symbol = atomseg.freeobj;
	atomseg.freeobj = CDR(symbol);
	--atomseg.nfree;

	symbol->type = LispAtom_t;
	symbol->data.atom = atom;
	atom->object = symbol;
	atom->package = PACKAGE;

	return (symbol);
    }
}

LispObj *
LispNewReal(LispMac *mac, double value)
{
    LispObj *real = objseg.freeobj;

    if (real == NIL)
	real = Lisp__New(mac, NIL, NIL);
    else
	objseg.freeobj = CDR(real);

    real->type = LispReal_t;
    real->data.real = value;

    return (real);
}

LispObj *
LispNewString(LispMac *mac, char *str, long length, int alloced)
{
    char *cstring;
    LispObj *string = objseg.freeobj;

    if (string == NIL)
	string = Lisp__New(mac, NIL, NIL);
    else
	objseg.freeobj = CDR(string);

    if (alloced)
	cstring = str;
    else {
	cstring = LispMalloc(mac, length + 1);
	memcpy(cstring, str, length);
	cstring[length] = '\0';
    }
    LispMused(mac, cstring);
    string->type = LispString_t;
    THESTR(string) = cstring;
    STRLEN(string) = length;

    return (string);
}

LispObj *
LispNewComplex(LispMac *mac, LispObj *realpart, LispObj *imagpart)
{
    LispObj *complexp = objseg.freeobj;

    if (complexp == NIL)
	complexp = Lisp__New(mac, realpart, imagpart);
    else
	objseg.freeobj = CDR(complexp);

    complexp->type = LispComplex_t;
    complexp->data.complex.real = realpart;
    complexp->data.complex.imag = imagpart;

    return (complexp);
}

LispObj *
LispNewCharacter(LispMac *mac, long c)
{
    if (c < 0)
	c += 256;
    if (c < 0 || c > 255)
	LispDestroy(mac, "%ld is not an ascii character", c);

    return (&(LispChars[c].character));
}

LispObj *
LispNewInteger(LispMac *mac, long i)
{
    LispObj *integer = objseg.freeobj;

    if (integer == NIL)
	integer = Lisp__New(mac, NIL, NIL);
    else
	objseg.freeobj = CDR(integer);

    integer->type = LispInteger_t;
    integer->data.integer = i;

    return (integer);
}

LispObj *
LispNewSmallInt(LispMac *mac, long i)
{
    if (i < 112 && i >= -16)
	return (&(LispSmallInts[i + 16]));
    else {
	LispObj *integer = objseg.freeobj;

	if (integer == NIL)
	    integer = Lisp__New(mac, NIL, NIL);
	else
	    objseg.freeobj = CDR(integer);

	integer->type = LispInteger_t;
	integer->data.integer = i;

	return (integer);
    }
}

LispObj *
LispNewRatio(LispMac *mac, long num, long den)
/* Note that reduction is done here, so a integer may be returned */
{
    LispObj *ratio;
    long rest, numerator = num, denominator = den;

    if (numerator == 0)
	return (SMALLINT(0));

    if (num < 0)
	num = -num;

    for (;;) {
	if ((rest = den % num) == 0)
	    break;
	den = num;
	num = rest;
    }
    if (den != 1) {
	denominator /= num;
	numerator /= num;
    }

    if (denominator < 0) {
	numerator = -numerator;
	denominator = -denominator;
    }

    if (denominator == 1)
	return (SMALLINT(numerator));

    ratio = objseg.freeobj;
    if (ratio == NIL)
	ratio = Lisp__New(mac, NIL, NIL);
    else
	objseg.freeobj = CDR(ratio);

    ratio->type = LispRatio_t;
    ratio->data.ratio.numerator = numerator;
    ratio->data.ratio.denominator = denominator;

    return (ratio);
}

LispObj *
LispNewVector(LispMac *mac, LispObj *objects)
{
    long count;
    LispObj *array, *dimension;

    for (count = 0, array = objects; CONS_P(array); count++, array = CDR(array))
	;

    GCProtect();
    dimension = CONS(SMALLINT(count), NIL);
    array = LispNew(mac, objects, dimension);
    array->type = LispArray_t;
    array->data.array.list = objects;
    array->data.array.dim = dimension;
    array->data.array.rank = 1;
    array->data.array.type = LispTrue_t;
    array->data.array.zero = count == 0;
    GCUProtect();

    return (array);
}

LispObj *
LispNewQuote(LispMac *mac, LispObj *obj)
{
    LispObj *quote = LispNew(mac, obj, NIL);

    quote->type = LispQuote_t;
    quote->data.quote = obj;

    return (quote);
}

LispObj *
LispNewBackquote(LispMac *mac, LispObj *obj)
{
    LispObj *backquote = LispNew(mac, obj, NIL);

    backquote->type = LispBackquote_t;
    backquote->data.quote = obj;

    return (backquote);
}

LispObj *
LispNewComma(LispMac *mac, LispObj *obj, int atlist)
{
    LispObj *comma = LispNew(mac, obj, NIL);

    comma->type = LispComma_t;
    comma->data.comma.eval = obj;
    comma->data.comma.atlist = atlist;

    return (comma);
}

LispObj *
LispNewCons(LispMac *mac, LispObj *car, LispObj *cdr)
{
    LispObj *cons = objseg.freeobj;

    if (cons == NIL)
	cons = Lisp__New(mac, car, cdr);
    else
	objseg.freeobj = CDR(cons);

    /* Fresh objects are already of type LispCons_t */
    CAR(cons) = car;
    CDR(cons) = cdr;

    return (cons);
}

LispObj *
LispNewLambda(LispMac *mac, LispObj *name, LispObj *code,
	      LispObj *data, LispFunType type)
{
    LispObj *fun = LispNew(mac, data, code);

    fun->type = LispLambda_t;
    fun->funtype = type;
    fun->data.lambda.name = name;
    fun->data.lambda.code = code;
    fun->data.lambda.data = data;

    return (fun);
}

LispObj *
LispNewStruct(LispMac *mac, LispObj *fields, LispObj *def)
{
    LispObj *struc = LispNew(mac, fields, def);

    struc->type = LispStruct_t;
    struc->data.struc.fields = fields;
    struc->data.struc.def = def;

    return (struc);
}

LispObj *
LispNewOpaque(LispMac *mac, void *data, int type)
{
    LispObj *opaque = LispNew(mac, NIL, NIL);

    opaque->type = LispOpaque_t;
    opaque->data.opaque.data = data;
    opaque->data.opaque.type = type;

    return (opaque);
}

/* string argument must be static, or allocated */
LispObj *
LispNewKeyword(LispMac *mac, char *string)
{
    LispObj *keyword;

    if (PACKAGE != mac->keyword) {
	LispObj *savepackage;
	LispPackage *savepack;

	/* Save package environment */
	savepackage = PACKAGE;
	savepack = mac->pack;

	/* Change package environment */
	PACKAGE = mac->keyword;
	mac->pack = mac->key;

	/* Create symbol in keyword package */
	keyword = LispNewStaticAtom(mac, string);

	/* Restore package environment */
	PACKAGE = savepackage;
	mac->pack = savepack;
    }
    else
	/* Just create symbol in keyword package */
	keyword = LispNewStaticAtom(mac, string);

    /* Export keyword symbol */
    LispExportSymbol(mac, keyword);

    /* All keywords are constants */
    keyword->data.atom->constant = LispTrue_t;

    /* XXX maybe should bound the keyword to itself, but that would
     * require allocating a LispProperty structure for every keyword */

    return (keyword);
}

LispObj *
LispNewPathname(LispMac *mac, LispObj *obj)
{
    LispObj *path = LispNew(mac, obj, NIL);

    path->type = LispPathname_t;
    path->data.pathname = obj;

    return (path);
}

LispObj *
LispNewStringStream(LispMac *mac, unsigned char *string,
		    int flags, long length, int alloced)
{
    LispObj *stream = LispNew(mac, NIL, NIL);

    SSTREAMP(stream) = LispCalloc(mac, 1, sizeof(LispString));
    if (alloced)
	SSTREAMP(stream)->string = string;
    else {
	SSTREAMP(stream)->string = LispMalloc(mac, length + 1);
	memcpy(SSTREAMP(stream)->string, string, length);
	SSTREAMP(stream)->string[length] = '\0';
    }

    stream->type = LispStream_t;

    SSTREAMP(stream)->length = length;
    LispMused(mac, SSTREAMP(stream));
    LispMused(mac, SSTREAMP(stream)->string);
    stream->data.stream.type = LispStreamString;
    stream->data.stream.readable = (flags & STREAM_READ) != 0;
    stream->data.stream.writable = (flags & STREAM_WRITE) != 0;
    SSTREAMP(stream)->space = length + 1;

    stream->data.stream.pathname = NIL;

    return (stream);
}

LispObj *
LispNewFileStream(LispMac *mac, LispFile *file, LispObj *path, int flags)
{
    LispObj *stream = LispNew(mac, NIL, NIL);

    stream->type = LispStream_t;
    FSTREAMP(stream) = file;
    stream->data.stream.pathname = path;
    stream->data.stream.type = LispStreamFile;
    stream->data.stream.readable = (flags & STREAM_READ) != 0;
    stream->data.stream.writable = (flags & STREAM_WRITE) != 0;

    return (stream);
}

LispObj *
LispNewPipeStream(LispMac *mac, LispPipe *program, LispObj *path, int flags)
{
    LispObj *stream = LispNew(mac, NIL, NIL);

    stream->type = LispStream_t;
    PSTREAMP(stream) = program;
    stream->data.stream.pathname = path;
    stream->data.stream.type = LispStreamPipe;
    stream->data.stream.readable = (flags & STREAM_READ) != 0;
    stream->data.stream.writable = (flags & STREAM_WRITE) != 0;

    return (stream);
}

LispObj *
LispNewStandardStream(LispMac *mac, LispFile *file, LispObj *description, int flags)
{
    LispObj *stream = LispNew(mac, NIL, NIL);

    stream->type = LispStream_t;
    FSTREAMP(stream) = file;
    stream->data.stream.pathname = description;
    stream->data.stream.type = LispStreamStandard;
    stream->data.stream.readable = (flags & STREAM_READ) != 0;
    stream->data.stream.writable = (flags & STREAM_WRITE) != 0;

    return (stream);
}

LispObj *
LispNewBigInteger(LispMac *mac, mpi *i)
{
    LispObj *integer = LispNew(mac, NIL, NIL);

    integer->type = LispBigInteger_t;
    integer->data.mp.integer = i;

    return (integer);
}

LispObj *
LispNewBigRational(LispMac *mac, mpr *r)
{
    LispObj *rational = LispNew(mac, NIL, NIL);

    rational->type = LispBigRatio_t;
    rational->data.mp.ratio = r;

    return (rational);
}

/* name must be of type LispString_t */
LispObj *
LispNewPackage(LispMac *mac, LispObj *name, LispObj *nicknames)
{
    LispObj *package = LispNew(mac, name, nicknames);
    LispPackage *pack = LispCalloc(mac, 1, sizeof(LispPackage));

    package->type = LispPackage_t;
    package->data.package.name = name;
    package->data.package.nicknames = nicknames;
    package->data.package.package = pack;

    LispMused(mac, pack);

    return (package);
}

static INLINE LispObj *
LispGetVarPack(LispMac *mac, LispObj *symbol)
{
    int i;
    char *string;
    LispAtom *atom;
    LispProperty *property;

    string = STRPTR(symbol);
    property = symbol->data.atom->property;

    for (i = 0; i < STRTBLSZ; i++) {
	atom = mac->pack->atoms[i];
	while (atom) {

	    if (property != NOPROPERTY && atom->property == property)
		/* Symbol already in the current package */
		return (atom->object);

	    else if (strcmp(atom->string, string) == 0)
		/* Different symbol with the same name found */
		return (atom->object);

	    atom = atom->next;
	}
    }

    /* Symbol not found, just import it */
    return (NULL);
}

/* package must be of type LispPackage_t */
void
LispUsePackage(LispMac *mac, LispObj *package)
{
    unsigned i;
    LispAtom *atom;
    LispPackage *pack;
    LispObj **pentry, **eentry;

    /* Already using its own symbols... */
    if (package == PACKAGE)
	return;

    /* Check if package not already in use-package list */
    for (pentry = mac->pack->use.pairs, eentry = pentry + mac->pack->use.length;
	 pentry < eentry; pentry++)
	if (*pentry == package)
	return;

    /* Remember this package is in the use-package list */
    if (mac->pack->use.length + 1 >= mac->pack->use.space) {
	LispObj **pairs = realloc(mac->pack->use.pairs,
				  (mac->pack->use.space + 1) * sizeof(LispObj*));

	if (pairs == NULL)
	    LispDestroy(mac, "out of memory");

	mac->pack->use.pairs = pairs;
	++mac->pack->use.space;
    }
    mac->pack->use.pairs[mac->pack->use.length++] = package;

    /* Import all extern symbols from package */
    pack = package->data.package.package;

    /* Traverse atom list, searching for extern symbols */
    for (i = 0; i < STRTBLSZ; i++) {
	atom = pack->atoms[i];
	while (atom) {
	    if (atom->ext)
		LispImportSymbol(mac, atom->object);
	    atom = atom->next;
	}
    }
}

/* symbol must be of type LispAtom_t */
void
LispImportSymbol(LispMac *mac, LispObj *symbol)
{
    int increment;
    LispAtom *atom;
    LispObj *current;

    current = LispGetVarPack(mac, symbol);
    if (current == NULL || current->data.atom->property == NOPROPERTY) {
	/* No conflicts */

	if (symbol->data.atom->a_object) {
	    /* If it is a bounded variable */
	    if (mac->pack->glb.length + 1 >= mac->pack->glb.space)
		LispMoreGlobals(mac, mac->pack);
	    mac->pack->glb.pairs[mac->pack->glb.length++] = symbol;
	}

	/* Create copy of atom in current package */
	atom = LispDoGetAtom(mac, STRPTR(symbol), 0);
	/*   Need to create a copy because if anything new is atached to the
	 * property, the current package is the owner, not the previous one. */

	/* And reference the same properties */
	atom->property = symbol->data.atom->property;

	increment = 1;
    }
    else if (current->data.atom->property != symbol->data.atom->property) {
	/* Symbol already exists in the current package,
	 * but does not reference the same variable */
	LispContinuable(mac, "Symbol %s already defined in package %s. Redefine?",
			STRPTR(symbol), THESTR(PACKAGE->data.package.name));

	atom = current->data.atom;

	/* Continued from error, redefine variable */
	LispDecrementAtomReference(mac, atom);
	atom->property = symbol->data.atom->property;
	
	atom->a_object = atom->a_function = atom->a_builtin =
	    atom->a_property = atom->a_defsetf = atom->a_defstruct = 0;

	increment = 1;
    }
    else {
	/* Symbol is already available in the current package, just update */
	atom = current->data.atom;

	increment = 0;
    }

    /* If importing an important system variable */
    atom->watch = symbol->data.atom->watch;

    /* Update constant flag */
    atom->constant = symbol->data.atom->constant;

    /* Set home-package and unique-atom associated with symbol */
    atom->package = symbol->data.atom->package;
    atom->object = symbol->data.atom->object;

    if (symbol->data.atom->a_object)
	atom->a_object = 1;
    if (symbol->data.atom->a_function)
	atom->a_function = 1;
    else if (symbol->data.atom->a_builtin)
	atom->a_builtin = 1;
    else if (symbol->data.atom->a_compiled)
	atom->a_compiled = 1;
    if (symbol->data.atom->a_property)
	atom->a_property = 1;
    if (symbol->data.atom->a_defsetf)
	atom->a_defsetf = 1;
    if (symbol->data.atom->a_defstruct)
	atom->a_defstruct = 1;

    if (increment)
	/* Increase reference count, more than one package using the symbol */
	LispIncrementAtomReference(mac, symbol->data.atom);
}

/* symbol must be of type LispAtom_t */
void
LispExportSymbol(LispMac *mac, LispObj *symbol)
{
    /* This does not automatically export symbols to another package using
     * the symbols of the current package */
    symbol->data.atom->ext = LispTrue_t;
}

#ifdef __GNUC__
LispObj *
LispGetVar(LispMac *mac, LispObj *atom)
{
    return (LispDoGetVar(mac, atom));
}

static INLINE LispObj *
LispDoGetVar(LispMac *mac, LispObj *atom)
#else
#define LispDoGetVar LispGetVar
LispObj *
LispGetVar(LispMac *mac, LispObj *atom)
#endif
{
    LispAtom *name;
    int i, base, offset;
    Atom_id id;

    name = atom->data.atom;
    if (name->constant && name->package == mac->keyword)
	return (atom);

    /* XXX offset should be stored elsewhere, it is unique, like the string
     * pointer. Unless a multi-thread interface is implemented (where
     * multiple stacks would be required, the offset value should be
     * stored with the string, so that a few cpu cicles could be saved
     * by initializing the value to -1, and only searching for the symbol
     * binding if it is not -1, and if no binding is found, because the
     * lexical scope was left, reset offset to -1. */
    offset = name->offset;
    id = name->string;
    base = mac->env.lex;
    i = mac->env.head - 1;

    if (offset <= i && (offset >= base || name->dyn) &&
	mac->env.names[offset] == id)
	return (mac->env.values[offset]);

    for (; i >= base; i--)
	if (mac->env.names[i] == id) {
	    name->offset = i;

	    return (mac->env.values[i]);
	}

    if (name->dyn) {
	/* Keep searching as maybe a rebound dynamic variable */
	for (; i >= 0; i--)
	    if (mac->env.names[i] == id) {
		name->offset = i;

	    return (mac->env.values[i]);
	}

	if (name->a_object) {
	    /* Check for a symbol defined as special, but not yet bound. */
	    if (name->property->value == UNBOUND)
		return (NULL);

	    return (name->property->value);
	}
    }

    return (name->a_object ? name->property->value : NULL);
}

#ifdef DEBUGGER
/* Same code as LispDoGetVar, but returns the address of the pointer to
 * the object value. Used only by the debugger */
void *
LispGetVarAddr(LispMac *mac, LispObj *atom)
{
    LispAtom *name;
    int i, base;
    Atom_id id;

    name = atom->data.atom;
    if (name->constant && name->package == mac->keyword)
	return (&atom);

    id = name->string;

    i = mac->env.head - 1;
    for (base = mac->env.lex; i >= base; i--)
	if (mac->env.names[i] == id)
	    return (&(mac->env.values[i]));

    if (name->dyn) {
	for (; i >= 0; i--)
	    if (mac->env.names[i] == id)
		return (&(mac->env.values[i]));

	if (name->a_object) {
	    /* Check for a symbol defined as special, but not yet bound */
	    if (name->property->value == UNBOUND)
		return (NULL);

	    return (&(name->property->value));
	}
    }

    return (name->a_object ? &(name->property->value) : NULL);
}
#endif

/* Only removes global variables. To be called by makunbound
 * Local variables are unbounded once their block is closed anyway.
 */
void
LispUnsetVar(LispMac *mac, LispObj *atom)
{
    int i;
    LispAtom *name = atom->data.atom;
    LispPackage *pack = name->package->data.package.package;

    LispRemDocumentation(mac, atom, LispDocVariable);

    for (i = pack->glb.length - 1; i > 0; i--)
	if (pack->glb.pairs[i] == atom) {
	    LispRemAtomObjectProperty(mac, name);
	    --pack->glb.length;
	    if (i < pack->glb.length)
		memmove(pack->glb.pairs + i, pack->glb.pairs + i + 1,
			sizeof(LispObj*) * (pack->glb.length - i));

	    /* unset hint about dynamically binded variable */
	    if (name->dyn)
		name->dyn = 0;
	    break;
	}
}

LispObj *
LispAddVar(LispMac *mac, LispObj *atom, LispObj *obj)
{
    if (mac->env.length >= mac->env.space)
	LispMoreEnvironment(mac);

    LispDoAddVar(mac, atom, obj);

    return (obj);
}

static INLINE void
LispDoAddVar(LispMac *mac, LispObj *symbol, LispObj *value)
{
    LispAtom *atom = symbol->data.atom;

    atom->offset = mac->env.length;
    mac->env.values[mac->env.length] = value;
    mac->env.names[mac->env.length++] = atom->string;
}

LispObj *
LispSetVar(LispMac *mac, LispObj *atom, LispObj *obj)
{
    LispPackage *pack;
    LispAtom *name;
    int i, base, offset;
    Atom_id id;

    name = atom->data.atom;
    offset = name->offset;
    id = name->string;
    base = mac->env.lex;
    i = mac->env.head - 1;

    if (offset <= i && (offset >= base || name->dyn) &&
	mac->env.names[offset] == id)
	return (mac->env.values[offset] = obj);

    for (; i >= base; i--)
	if (mac->env.names[i] == id) {
	    name->offset = i;

	    return (mac->env.values[i] = obj);
	}

    if (name->dyn) {
	for (; i >= 0; i--)
	    if (mac->env.names[i] == id)
		return (mac->env.values[i] = obj);

	if (name->watch) {
	    LispSetAtomObjectProperty(mac, name, obj);

	    return (obj);
	}

	return (SETVALUE(name, obj));
    }

    if (name->a_object) {
	if (name->watch) {
	    LispSetAtomObjectProperty(mac, name, obj);

	    return (obj);
	}

	return (SETVALUE(name, obj));
    }

    LispSetAtomObjectProperty(mac, name, obj);

    pack = name->package->data.package.package;
    if (pack->glb.length >= pack->glb.space)
	LispMoreGlobals(mac, pack);

    pack->glb.pairs[pack->glb.length++] = atom;

    return (obj);
}

void
LispProclaimSpecial(LispMac *mac, LispObj *atom, LispObj *value, LispObj *doc)
{
    int i = 0, dyn, glb;
    LispAtom *name;
    LispPackage *pack;

    glb = 0;
    name = atom->data.atom;
    pack = name->package->data.package.package;
    dyn = name->dyn;

    if (!dyn) {
	/* Note: don't check if a local variable already is using the symbol */
	for (i = pack->glb.length - 1; i >= 0; i--)
	    if (pack->glb.pairs[i] == atom) {
		glb = 1;
		break;
	    }
    }

    if (dyn) {
	if (name->property->value == UNBOUND && value)
	    /* if variable was just made special, but not bounded */
	    LispSetAtomObjectProperty(mac, name, value);
    }
    else if (glb)
	/* Already a global variable, but not marked as special.
	 * Set hint about dynamically binded variable. */
	name->dyn = 1;
    else {
	/* create new special variable */
	LispSetAtomObjectProperty(mac, name, value ? value : UNBOUND);

	if (pack->glb.length >= pack->glb.space)
	    LispMoreGlobals(mac, pack);

	pack->glb.pairs[pack->glb.length] = atom;
	++pack->glb.length;
	/* set hint about possibly dynamically binded variable */
	name->dyn = 1;
    }

    if (doc != NIL)
	LispAddDocumentation(mac, atom, doc, LispDocVariable);
}

void
LispDefconstant(LispMac *mac, LispObj *atom, LispObj *value, LispObj *doc)
{
    int i;
    LispAtom *name = atom->data.atom;
    LispPackage *pack = name->package->data.package.package;

    /* Unset hint about dynamically binded variable, if set. */
    name->dyn = 0;

    /* Check if variable is bounded as a global variable */
    for (i = pack->glb.length - 1; i >= 0; i--)
	if (pack->glb.pairs[i] == atom)
	    break;

    if (i < 0) {
	/* Not a global variable */
	if (pack->glb.length >= pack->glb.space)
	    LispMoreGlobals(mac, pack);

	pack->glb.pairs[pack->glb.length] = atom;
	++pack->glb.length;
    }

    /* If already a constant variable */
    if (name->constant && name->a_object &&
	XEQUAL(name->property->value, value) == NIL)
	LispWarning(mac, "constant %s is being redefined", STROBJ(atom));
    else
	name->constant = LispTrue_t;

    /* Set constant value */
    LispSetAtomObjectProperty(mac, name, value);

    if (doc != NIL)
	LispAddDocumentation(mac, atom, doc, LispDocVariable);
}

void
LispAddDocumentation(LispMac *mac, LispObj *symbol, LispObj *documentation,
		     LispDocType_t type)
{
    LispObj *env;
    Atom_id atom;

    if (!SYMBOL_P(symbol))
	LispDestroy(mac, "ADD-DOCUMENTATION: %s is not a symbol",
		    STROBJ(symbol));
    if (!STRING_P(documentation))
	LispDestroy(mac, "ADD-DOCUMENTATION: %s is not a string",
		    STROBJ(documentation));

    atom = ATOMID(symbol);
    for (env = DOC; env != NIL; env = CDR(env)) {
	if (ATOMID(CAAR(env)) == atom) {
	    LispObj *doc = CAR(env);

	    switch (type) {
		case LispDocVariable:
		    CAR(CDR(doc)) = documentation;
		    break;
		case LispDocFunction:
		    CAR(CDR(CDR(doc))) = documentation;
		    break;
		case LispDocStructure:
		    CAR(CDR(CDR(CDR(doc)))) = documentation;
		    break;
		case LispDocType:
		    CAR(CDR(CDR(CDR(CDR(doc))))) = documentation;
		    break;
		case LispDocSetf:
		    CDR(CDR(CDR(CDR(CDR(doc))))) = documentation;
		    break;
		default:
		    LispDestroy(mac, "ADD-DOCUMENTATION: bad documentation type");
		    break;
	    }
	    break;
	}
    }

    env = CONS(NIL, CONS(NIL, (CONS(NIL, (CONS(NIL, CONS(NIL, NIL)))))));
    DOC = CONS(env, DOC);

    RPLACA(env, symbol);

    switch (type) {
	case LispDocVariable:
	    CAR(CDR(env)) = documentation;
	    break;
	case LispDocFunction:
	    CAR(CDR(CDR(env))) = documentation;
	    break;
	case LispDocStructure:
	    CAR(CDR(CDR(CDR(env)))) = documentation;
	    break;
	case LispDocType:
	    CAR(CDR(CDR(CDR(CDR(env))))) = documentation;
	    break;
	case LispDocSetf:
	    CDR(CDR(CDR(CDR(CDR(env))))) = documentation;
	    break;
	default:
	    LispDestroy(mac, "ADD-DOCUMENTATION: bad documentation type");
	    break;
    }
}

void
LispRemDocumentation(LispMac *mac, LispObj *symbol, LispDocType_t type)
{
    LispObj *prev, *env;
    Atom_id atom;

    if (!SYMBOL_P(symbol))
	LispDestroy(mac, "REMOVE-DOCUMENTATION: %s is not a symbol",
		    STROBJ(symbol));

    atom = ATOMID(symbol);
    for (prev = env = DOC; env != NIL; prev = env, env = CDR(env)) {
	if (ATOMID(CAAR(env)) == atom) {
	    LispObj *doc = CAR(env);

	    switch (type) {
		case LispDocVariable:
		    CAR(CDR(doc)) = NIL;
		    break;
		case LispDocFunction:
		    CAR(CDR(CDR(doc))) = NIL;
		    break;
		case LispDocStructure:
		    CAR(CDR(CDR(CDR(doc)))) = NIL;
		    break;
		case LispDocType:
		    CAR(CDR(CDR(CDR(CDR(doc))))) = NIL;
		    break;
		case LispDocSetf:
		    CDR(CDR(CDR(CDR(CDR(doc))))) = NIL;
		    break;
		default:
		    LispDestroy(mac, "REMOVE-DOCUMENTATION: bad documentation type");
		    break;
	    }
	    if (CAR(CDR(doc)) == NIL &&
		CAR(CDR(CDR(doc))) == NIL &&
		CAR(CDR(CDR(CDR(doc)))) == NIL &&
		CAR(CDR(CDR(CDR(CDR(doc))))) == NIL &&
		CAR(CDR(CDR(CDR(CDR(doc))))) == NIL) {
		/* no documentation remaining */
		if (prev == DOC)
		    doc = CDR(doc);
		else
		    RPLACD(prev, CDR(env));
	    }
	    break;
	}
    }
}

LispObj *
LispGetDocumentation(LispMac *mac, LispObj *symbol, LispDocType_t type)
{
    LispObj *env, *result = NIL;
    Atom_id atom;

    if (!SYMBOL_P(symbol))
	LispDestroy(mac, "GET-DOCUMENTATION: %s is not a symbol",
		    STROBJ(symbol));

    atom = ATOMID(symbol);
    for (env = DOC; env != NIL; env = CDR(env)) {
	if (ATOMID(CAAR(env)) == atom) {
	    LispObj *doc = CAR(env);

	    switch (type) {
		case LispDocVariable:
		    result = CAR(CDR(doc));
		    break;
		case LispDocFunction:
		    result = CAR(CDR(CDR(doc)));
		    break;
		case LispDocStructure:
		    result = CAR(CDR(CDR(CDR(doc))));
		    break;
		case LispDocType:
		    result = CAR(CDR(CDR(CDR(CDR(doc)))));
		    break;
		case LispDocSetf:
		    result = CDR(CDR(CDR(CDR(CDR(doc)))));
		    break;
		default:
		    LispDestroy(mac, "GET-DOCUMENTATION: bad documentation type");
		    break;
	    }
	    break;
	}
    }

    return (result);
}

LispObj *
LispReverse(LispObj *list)
{
    LispObj *tmp, *res = NIL;

    while (list != NIL) {
	tmp = CDR(list);
	CDR(list) = res;
	res = list;
	list = tmp;
    }

    return (res);
}

LispBlock *
LispBeginBlock(LispMac *mac, LispObj *tag, LispBlockType type)
{
    LispBlock *block;
    unsigned blevel = mac->block.block_level + 1;

    if (blevel > mac->block.block_size) {
	LispBlock **blk = realloc(mac->block.block,
				  sizeof(LispBlock*) * (blevel + 1));

	block = NULL;
	if (blk == NULL || (block = malloc(sizeof(LispBlock))) == NULL)
	    LispDestroy(mac, "out of memory");
	mac->block.block = blk;
	mac->block.block[mac->block.block_size] = block;
	mac->block.block_size = blevel;
    }
    block = mac->block.block[mac->block.block_level];
    if (type == LispBlockCatch && !CONSTANT_P(tag)) {
	tag = EVAL(tag);
	mac->protect.objects[mac->protect.length++] = tag;
    }
    block->type = type;
    block->tag = tag;
    block->stack = mac->stack.length;
    block->protect = mac->protect.length;
    block->block_level = mac->block.block_level;

    mac->block.block_level = blevel;

#ifdef DEBUGGER
    if (mac->debugging) {
	block->debug_level = mac->debug_level;
	block->debug_step = mac->debug_step;
    }
#endif

    return (block);
}

void
LispEndBlock(LispMac *mac, LispBlock *block)
{
    mac->protect.length = block->protect;
    mac->block.block_level = block->block_level;

#ifdef DEBUGGER
    if (mac->debugging) {
	if (mac->debug_level >= block->debug_level) {
	    while (mac->debug_level > block->debug_level) {
		DBG = CDR(DBG);
		--mac->debug_level;
	    }
	}
	mac->debug_step = block->debug_step;
    }
#endif
}

void
LispBlockUnwind(LispMac *mac, LispBlock *block)
{
    LispBlock *unwind;
    int blevel = mac->block.block_level;

    while (blevel > 0) {
	unwind = mac->block.block[--blevel];
	if (unwind->type == LispBlockProtect) {
	    BLOCKJUMP(unwind);
	}
	if (unwind == block)
	    /* jump above unwind block */
	    break;
    }
}

static LispObj *
LispEvalBackquoteObject(LispMac *mac, LispObj *argument, int list, int quote)
{
    LispObj *result = NIL, *object;

    if (argument->type == LispComma_t) {
	/* argument may need to be evaluated */

	int atlist;

	if (!list && argument->data.comma.atlist)
	    /* cannot append, not in a list */
	    LispDestroy(mac, "EVAL: ,@ only allowed on lists");

	--quote;
	if (quote < 0)
	    LispDestroy(mac, "EVAL: comma outside of backquote");

	result = object = argument->data.comma.eval;
	atlist = result->type == LispComma_t && object->data.comma.atlist;

	if (result->type == LispComma_t || result->type == LispBackquote_t)
	    /* nested commas, reduce 1 level, or backquote,
	     * don't call LispEval or quote argument will be reset */
	    result = LispEvalBackquoteObject(mac, object, 0, quote);

	else if (quote == 0 && !CONSTANT_P(result))
	   /* just evaluate it */
	    result = EVAL(result);

	if (quote != 0)
	    result = result == object ? argument : COMMA(result, atlist);
    }

    else if (argument->type == LispBackquote_t) {
	object = argument->data.quote;

	result = LispEvalBackquote(mac, object, quote + 1);
	if (quote)
	    result = result == object ? argument : BACKQUOTE(result);
    }

    else if (argument->type == LispQuote_t &&
	     (argument->data.quote->type == LispComma_t ||
	      argument->data.quote->type == LispBackquote_t ||
	      CONS_P(argument->data.quote))) {
	/* ensures `',sym to be the same as `(quote ,sym) */
	object = argument->data.quote;

	result = LispEvalBackquote(mac, argument->data.quote, quote);
	result = result == object ? argument : QUOTE(result);
    }

    else
	/* anything else is not evaluated */
	result = argument;

    return (result);
}

LispObj *
LispEvalBackquote(LispMac *mac, LispObj *argument, int quote)
{
    int protect;
    LispObj *result, *object, *cons, *cdr;

    if (!CONS_P(argument))
	return (LispEvalBackquoteObject(mac, argument, 0, quote));

    result = cdr = NIL;
    protect = mac->protect.length;

    /* always generate a new list for the result, even if nothing
     * is evaluated. It is not expected to use backqoutes when
     * not required. */

    /* reserve a GC protected slot for the result */
    if (protect + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = NIL;

    for (cons = argument; ; cons = CDR(cons)) {
	/* if false, last argument, and if cons is not NIL, a dotted list */
	int list = CONS_P(cons), insert;

	if (list)
	    object = CAR(cons);
	else
	    object = cons;

	if (object->type == LispComma_t)
	    /* need to insert list elements in result, not just cons it? */
	    insert = object->data.comma.atlist;
	else
	    insert = 0;

	/* evaluate object, if required */
	if (CONS_P(object))
	    object = LispEvalBackquote(mac, object, quote);
	else
	    object = LispEvalBackquoteObject(mac, object, insert, quote);

	if (result == NIL) {
	    /* if starting result list */
	    if (!insert) {
		if (list)
		    result = cdr = CONS(object, NIL);
		else
		    result = cdr = object;
		/* gc protect result */
		mac->protect.objects[protect] = result;
	    }
	    else {
		if (!CONS_P(object)) {
		    result = cdr = object;
		    /* gc protect result */
		    mac->protect.objects[protect] = result;
		}
		else {
		    result = cdr = CONS(CAR(object), NIL);
		    /* gc protect result */
		    mac->protect.objects[protect] = result;

		    /* add remaining elements to result */
		    for (object = CDR(object); CONS_P(object); object = CDR(object)) {
			RPLACD(cdr, CONS(CAR(object), NIL));
			cdr = CDR(cdr);
		    }
		    if (object != NIL) {
			/* object was a dotted list */
			RPLACD(cdr, object);
			cdr = CDR(cdr);
		    }
		}
	    }
	}
	else {
	    if (!CONS_P(cdr))
		LispDestroy(mac, "EVAL: cannot append to %s", STROBJ(cdr));

	    if (!insert) {
		if (list) {
		    RPLACD(cdr, CONS(object, NIL));
		    cdr = CDR(cdr);
		}
		else {
		    RPLACD(cdr, object);
		    cdr = object;
		}
	    }
	    else {
		if (!CONS_P(object)) {
		    RPLACD(cdr, object);
		    /* if object is NIL, it is a empty list appended, not
		     * creating a dotted list. */
		    if (object != NIL)
			cdr = object;
		}
		else {
		    for (; CONS_P(object); object = CDR(object)) {
			RPLACD(cdr, CONS(CAR(object), NIL));
			cdr = CDR(cdr);
		    }
		    if (object != NIL) {
			/* object was a dotted list */
			RPLACD(cdr, object);
			cdr = CDR(cdr);
		    }
		}
	    }
	}

	/* if last argument list element processed */
	if (!list)
	    break;
    }

    mac->protect.length = protect;

    return (result);
}

void
LispMoreEnvironment(LispMac *mac)
{
    Atom_id *names;
    LispObj **values;

    names = realloc(mac->env.names, (mac->env.space + 256) * sizeof(Atom_id));
    if (names != NULL) {
	values = realloc(mac->env.values,
			 (mac->env.space + 256) * sizeof(LispObj*));
	if (values != NULL) {
	    mac->env.names = names;
	    mac->env.values = values;
	    mac->env.space += 256;

	    return;
	}
	else
	    free(names);
    }
    LispDestroy(mac, "out of memory");
}

void
LispMoreStack(LispMac *mac)
{
    LispObj **values = realloc(mac->stack.values,
			       (mac->stack.space + 256) * sizeof(LispObj*));

    if (values == NULL)
	LispDestroy(mac, "out of memory");

    mac->stack.values = values;
    mac->stack.space += 256;
}

void
LispMoreGlobals(LispMac *mac, LispPackage *pack)
{
    LispObj **pairs = realloc(pack->glb.pairs,
			      (pack->glb.space + 256) * sizeof(LispObj*));

    if (pairs == NULL)
	LispDestroy(mac, "out of memory");

    pack->glb.pairs = pairs;
    pack->glb.space += 256;
}

void
LispMoreReturns(LispMac *mac)
{
    LispObj **values = realloc(mac->returns.values,
			       (mac->returns.space + 32) * sizeof(LispObj*));

    if (values == NULL)
	LispDestroy(mac, "out of memory");

    mac->returns.values = values;
    mac->returns.space += 32;
}

void
LispMoreProtects(LispMac *mac)
{
    LispObj **objects = realloc(mac->protect.objects,
				(mac->protect.space + 256) * sizeof(LispObj*));

    if (objects == NULL)
	LispDestroy(mac, "out of memory");

    mac->protect.objects = objects;
    mac->protect.space += 256;
}

static int
LispMakeEnvironment(LispMac *mac, LispArgList *alist, LispObj *values,
		    LispObj *name, int eval, int builtin)
{
    char *desc;
    int i, count, base;
    LispObj **symbols, **defaults, **sforms;

#define BUILTIN_ARGUMENT(value)				\
    mac->stack.values[mac->stack.length++] = value

/* If the index value is from register variables, this
 * can save some cpu time. Useful for normal arguments
 * that are the most common, and thus the ones that
 * consume more time in LispMakeEnvironment. */
#define BUILTIN_NO_EVAL_ARGUMENT(index, value)		\
    mac->stack.values[index] = value

#define NORMAL_ARGUMENT(symbol, value)			\
    LispDoAddVar(mac, symbol, value)

    if (builtin) {
	base = mac->stack.length;
	if (base + alist->num_arguments > mac->stack.space) {
	    do
		LispMoreStack(mac);
	    while (base + alist->num_arguments > mac->stack.space);
	}
    }
    else {
	base = mac->env.length;
	if (base + alist->num_arguments > mac->env.space) {
	    do
		LispMoreEnvironment(mac);
	    while (base + alist->num_arguments > mac->env.space);
	}
    }

    desc = alist->description;
    switch (*desc++) {
	case '.':
	    goto normal_label;
	case 'o':
	    goto optional_label;
	case 'k':
	    goto key_label;
	case 'r':
	    goto rest_label;
	case 'a':
	    goto aux_label;
	default:
	    goto done_label;
    }


    /* Code below is done in several almost identical loops, to avoid
     * checking the value of the arguments eval and builtin too much times */


    /* Normal arguments */
normal_label:
    i = 0;
    count = alist->normals.num_symbols;
    if (builtin) {
	if (eval) {
	    for (; i < count && CONS_P(values); i++, values = CDR(values)) {
		BUILTIN_ARGUMENT(EVAL(CAR(values)));
	    }
	}
	else {
	    for (; i < count && CONS_P(values); i++, values = CDR(values)) {
		BUILTIN_NO_EVAL_ARGUMENT(base + i, CAR(values));
	    }
	    /* macro BUILTIN_NO_EVAL_ARGUMENT does not update
	     * mac->stack.length, as there is no risk of GC while
	     * adding the arguments. */
	    mac->stack.length += i;
	}
    }
    else {
	symbols = alist->normals.symbols;
	if (eval) {
	    for (; i < count && CONS_P(values); i++, values = CDR(values)) {
		NORMAL_ARGUMENT(symbols[i], EVAL(CAR(values)));
	    }
	}
	else {
	    for (; i < count && CONS_P(values); i++, values = CDR(values)) {
		NORMAL_ARGUMENT(symbols[i], CAR(values));
	    }
	}
    }
    if (i < count)
	LispDestroy(mac, "%s: too few arguments", STROBJ(name));

    switch (*desc++) {
	case 'o':
	    goto optional_label;
	case 'k':
	    goto key_label;
	case 'r':
	    goto rest_label;
	case 'a':
	    goto aux_label;
	default:
	    goto done_label;
    }

    /* &OPTIONAL */
optional_label:
    i = 0;
    count = alist->optionals.num_symbols;
    defaults = alist->optionals.defaults;
    sforms = alist->optionals.sforms;
    if (builtin) {
	if (eval) {
	    for (; i < count && CONS_P(values); i++, values = CDR(values)) {
		BUILTIN_ARGUMENT(EVAL(CAR(values)));
		if (sforms[i]) {
		    BUILTIN_ARGUMENT(T);
		}
	    }
	    for (; i < count; i++) {
		if (NCONSTANT_P(defaults[i])) {
		    BUILTIN_ARGUMENT(EVAL(defaults[i]));
		}
		else {
		    BUILTIN_ARGUMENT(defaults[i]);
		}
		if (sforms[i]) {
		    BUILTIN_ARGUMENT(NIL);
		}
	    }
	}
	else {
	    for (; i < count && CONS_P(values); i++, values = CDR(values)) {
		BUILTIN_ARGUMENT(CAR(values));
		if (sforms[i]) {
		    BUILTIN_ARGUMENT(T);
		}
	    }
	    for (; i < count; i++) {
		BUILTIN_ARGUMENT(defaults[i]);
		if (sforms[i]) {
		    BUILTIN_ARGUMENT(NIL);
		}
	    }
	}
    }
    else {
	symbols = alist->optionals.symbols;
	if (eval) {
	    for (; i < count && CONS_P(values); i++, values = CDR(values)) {
		NORMAL_ARGUMENT(symbols[i], EVAL(CAR(values)));
		if (sforms[i]) {
		    NORMAL_ARGUMENT(sforms[i], T);
		}
	    }
	    for (; i < count; i++) {
		/* XXX BUILTIN FUNCTIONS CANNOT HAVE A DEFAULT VALUE THAT
		 * IS A VALUE OF ANOTHER FUNCTION ARGUMENT. */
		if (NCONSTANT_P(defaults[i])) {
		    int head = mac->env.head;
		    int lex = mac->env.lex;

		    mac->env.lex = base;
		    mac->env.head = mac->env.length;
		    NORMAL_ARGUMENT(symbols[i], EVAL(defaults[i]));
		    mac->env.head = head;
		    mac->env.lex = lex;
		}
		else {
		    NORMAL_ARGUMENT(symbols[i], defaults[i]);
		}
		if (sforms[i]) {
		    NORMAL_ARGUMENT(sforms[i], NIL);
		}
	    }
	}
	else {
	    for (; i < count && CONS_P(values); i++, values = CDR(values)) {
		NORMAL_ARGUMENT(symbols[i], CAR(values));
		if (sforms[i]) {
		    NORMAL_ARGUMENT(sforms[i], T);
		}
	    }
	    for (; i < count; i++) {
		NORMAL_ARGUMENT(symbols[i], defaults[i]);
		if (sforms[i]) {
		    NORMAL_ARGUMENT(sforms[i], NIL);
		}
	    }
	}
    }
    switch (*desc++) {
	case 'k':
	    goto key_label;
	case 'r':
	    goto rest_label;
	case 'a':
	    goto aux_label;
	default:
	    goto done_label;
    }

    /* &KEY */
key_label:
    {
	int argc, nused;
	LispObj *val, *karg, **keys;

	/* Count number of remaining arguments */
	for (karg = values, argc = 0; CONS_P(karg); karg = CDR(karg), argc++) {
	    karg = CDR(karg);
	    if (!CONS_P(karg))
		LispDestroy(mac, "%s: &KEY needs arguments as pairs",
			    STROBJ(name));
	}


	/* OPTIMIZATION:
	 * Builtin functions require that the keyword be in the keyword package.
	 * User functions don't need the arguments being pushed in the stack
	 * in the declared order.
	 * XXX Error checking should be done elsewhere, code may be looping
	 * and doing error check here may consume too much cpu time.
	 * XXX Would also be good to already have the arguments specified in
	 * the correct order.
	 */


	nused = 0;
	val = NIL;
	count = alist->keys.num_symbols;
	symbols = alist->keys.symbols;
	defaults = alist->keys.defaults;
	sforms = alist->keys.sforms;
	if (builtin) {

	    /* Arguments must be created in the declared order */
	    i = 0;
	    if (eval) {
		for (; i < count; i++) {
		    for (karg = values; CONS_P(karg); karg = CDDR(karg)) {
			/* This is only true if both point to the
			 * same symbol in the keyword package. */
			if (symbols[i] == CAR(karg)) {
			    if (karg == values)
				values = CDDR(values);
			    ++nused;
			    BUILTIN_ARGUMENT(EVAL(CADR(karg)));
			    if (sforms[i]) {
				BUILTIN_ARGUMENT(T);
			    }
			    goto keyword_builtin_eval_used_label;
			}
		    }
		    BUILTIN_ARGUMENT(EVAL(defaults[i]));
		    if (sforms[i]) {
			BUILTIN_ARGUMENT(NIL);
		    }
keyword_builtin_eval_used_label:;
		}
	    }
	    else {
		for (; i < count; i++) {
		    for (karg = values; CONS_P(karg); karg = CDDR(karg)) {
			if (symbols[i] == CAR(karg)) {
			    if (karg == values)
				values = CDDR(values);
			    ++nused;
			    BUILTIN_ARGUMENT(CADR(karg));
			    if (sforms[i]) {
				BUILTIN_ARGUMENT(T);
			    }
			    goto keyword_builtin_used_label;
			}
		    }
		    BUILTIN_ARGUMENT(defaults[i]);
		    if (sforms[i]) {
			BUILTIN_ARGUMENT(NIL);
		    }
keyword_builtin_used_label:;
		}
	    }

	    if (argc != nused) {
		/* Argument(s) may be incorrectly specified, or specified
		 * twice (what is not an error). */
		for (karg = values; CONS_P(karg); karg = CDDR(karg)) {
		    val = CAR(karg);
		    if (SYMBOL_P(val) && KEYWORD_P(val)) {
			for (i = 0; i < count; i++)
			    if (symbols[i] == val)
				break;
		    }
		    else
			/* Just make the error test true */
			i = count;

		    if (i == count)
			goto invalid_keyword_label;
		}
	    }
	}

	else {
	    /* The base offset of the atom in the stack, to check for
	     * keywords specified twice. */
	    LispObj *symbol;
	    int offset = mac->env.length;

	    keys = alist->keys.keys;
	    for (karg = values; CONS_P(karg); karg = CDDR(karg)) {
		symbol = CAR(karg);
		if (SYMBOL_P(symbol)) {
		    /* Must be a keyword, but even if it is a keyword, may
		     * be a typo, so assume it is correct. If it is not
		     * in the argument list, it is an error. */
		    for (i = 0; i < count; i++) {
			if (!keys[i] && symbols[i] == symbol) {
			    LispAtom *atom = symbol->data.atom;

			    /* Symbol found in the argument list. */
			    if (atom->offset >= offset &&
				atom->offset < offset + nused &&
				mac->env.names[atom->offset] == atom->string)
				/* Specified more than once... */
				goto keyword_duplicated_label;
			    break;
			}
		    }
		}
		else {
		    Atom_id id;

		    if (symbol->type != LispQuote_t ||
			!SYMBOL_P(val = symbol->data.quote)) {
			/* Bad argument. */
			val = symbol;
			goto invalid_keyword_label;
		    }

		    id = ATOMID(val);
		    for (i = 0; i < count; i++) {
			if (keys[i] && ATOMID(keys[i]) == id) {
			    LispAtom *atom = val->data.atom;

			    /* Symbol found in the argument list. */
			    if (atom->offset >= offset &&
				atom->offset < offset + nused &&
				mac->env.names[atom->offset] == atom->string)
				/* Specified more than once... */
				goto keyword_duplicated_label;
			    break;
			}
		    }
		}
		if (i == count) {
		    /* Argument specification not found. */
		    val = symbol;
		    goto invalid_keyword_label;
		}
		++nused;
		if (eval) {
		    NORMAL_ARGUMENT(symbols[i], EVAL(CADR(karg)));
		}
		else {
		    NORMAL_ARGUMENT(symbols[i], CADR(karg));
		}
		if (sforms[i]) {
		    NORMAL_ARGUMENT(sforms[i], T);
		}
keyword_duplicated_label:;
	    }

	    /* Add variables that were not specified in the function call. */
	    if (nused < count) {
		int j;

		for (i = 0; i < count; i++) {
		    Atom_id id = ATOMID(symbols[i]);

		    for (j = offset + nused - 1; j >= offset; j--) {
			if (mac->env.names[j] == id)
			    break;
		    }

		    if (j < offset) {
			/* Argument not specified. Use default value */

			if (eval && NCONSTANT_P(defaults[i])) {
			    /* XXX BUILTIN FUNCTIONS CANNOT HAVE A DEFAULT VALUE
			     * THAT IS A VALUE OF ANOTHER FUNCTION ARGUMENT. */
			    int head = mac->env.head;
			    int lex = mac->env.lex;

			    mac->env.lex = base;
			    mac->env.head = mac->env.length;
			    NORMAL_ARGUMENT(symbols[i], EVAL(defaults[i]));
			    mac->env.head = head;
			    mac->env.lex = lex;
			}
			else {
			    NORMAL_ARGUMENT(symbols[i], defaults[i]);
			}
			if (sforms[i]) {
			    NORMAL_ARGUMENT(sforms[i], NIL);
			}
		    }
		}
	    }
	}
	goto check_aux_label;

invalid_keyword_label:
	{
	    /* If not in argument specification list... */
	    char function_name[36];

	    strcpy(function_name, STROBJ(name));
	    LispDestroy(mac, "%s: %s is an invalid keyword",
			function_name, STROBJ(val));
	}
    }

check_aux_label:
    if (*desc == 'a') {
	/* &KEY uses all remaining arguments */
	values = NIL;
	goto aux_label;
    }
    goto finished_label;

    /* &REST */
rest_label:
    if (!eval || !CONS_P(values)) {
	if (builtin) {
	    BUILTIN_ARGUMENT(values);
	}
	else {
	    NORMAL_ARGUMENT(alist->rest, values);
	}
    }
    else {
	LispObj *list;

	for (list = values; CONS_P(list); list = CDR(list))
	    if (NCONSTANT_P(CAR(list))) {
		LispObj *cons;

		cons = CONS(EVAL(CAR(values)), NIL);
		if (builtin) {
		    BUILTIN_ARGUMENT(cons);
		}
		else {
		    NORMAL_ARGUMENT(alist->rest, cons);
		}
		values = CDR(values);
		for (; CONS_P(values); values = CDR(values)) {
		    RPLACD(cons, CONS(EVAL(CAR(values)), NIL));
		    cons = CDR(cons);
		}
		goto rest_done;
	    }

	/* list of constants, don't allocate new cells */
	if (builtin) {
	    BUILTIN_ARGUMENT(values);
	}
	else {
	    NORMAL_ARGUMENT(alist->rest, values);
	}
    }
rest_done:
    if (*desc != 'a')
	goto finished_label;

    /* &AUX */
aux_label:
    i = 0;
    count = alist->auxs.num_symbols;
    defaults = alist->auxs.initials;
    if (builtin) {
	for (; i < count; i++) {
	    if (eval) {
		BUILTIN_ARGUMENT(EVAL(defaults[i]));
	    }
	    else {
		BUILTIN_ARGUMENT(defaults[i]);
	    }
	}
    }
    else {
	symbols = alist->auxs.symbols;
	/* allow using the variables defined here.
	 * XXX THIS IS NOT ALLOWED FOR BUILTIN FUNCTIONS! */
	if (eval) {
	    int lex = mac->env.lex;

	    mac->env.lex = base;
	    mac->env.head = mac->env.length;
	    for (; i < count; i++) {
		NORMAL_ARGUMENT(symbols[i], EVAL(defaults[i]));
		++mac->env.head;
	    }
	    mac->env.lex = lex;
	}
	else {
	    for (; i < count; i++) {
		NORMAL_ARGUMENT(symbols[i], defaults[i]);
	    }
	}
    }

done_label:
    if (CONS_P(values))
	LispDestroy(mac, "%s: too many arguments", STROBJ(name));

finished_label:
    if (builtin)
	mac->stack.base = base;
    else {
	mac->env.head = mac->env.length;
    }
#undef BULTIN_ARGUMENT
#undef NORMAL_ARGUMENT
#undef BUILTIN_NO_EVAL_ARGUMENT

    return (base);
}

LispObj *
LispFuncall(LispMac *mac, LispObj *function, LispObj *arguments, int eval)
{
    LispAtom *atom;
    LispArgList *alist;
    LispBuiltin *builtin;
    LispObj *lambda, *result;
    int multiple_values, macro, base;

#ifdef DEBUGGER
    if (mac->debugging)
	LispDebugger(mac, LispDebugCallBegin, function, arguments);
#endif

    switch (function->type) {
	case LispAtom_t:
	    atom = function->data.atom;
	    if (atom->a_builtin) {
		builtin = atom->property->fun.builtin;

		if (eval)
		    eval = builtin->type != LispMacro;
		base = LispMakeEnvironment(mac, atom->property->alist,
					   arguments, function, eval, 1);
		multiple_values = builtin->multiple_values;
		result = builtin->function(mac, builtin);
		mac->stack.base = mac->stack.length = base;
	    }
	    else if (atom->a_compiled) {
		int lex = mac->env.lex;
		lambda = atom->property->fun.function;
		alist = atom->property->alist;

		base = LispMakeEnvironment(mac, alist, arguments,
					   function, eval, 0);
		multiple_values = 0;
		result = LispExecuteBytecode(mac, lambda);
		mac->env.lex = lex;
		mac->env.head = mac->env.length = base;
	    }
	    else if (atom->a_function) {
		lambda = atom->property->fun.function;
		macro = lambda->funtype == LispMacro;
		alist = atom->property->alist;

		lambda = lambda->data.lambda.code;
		if (eval)
		    eval = !macro;
		base = LispMakeEnvironment(mac, alist, arguments,
					   function, eval, 0);
		multiple_values = 0;
		result = LispRunFunMac(mac, function, lambda, macro, base);
	    }
	    else if (atom->a_defstruct &&
		     atom->property->structure.function != STRUCT_NAME) {
		LispObj cons;

		if (atom->property->structure.function == STRUCT_CONSTRUCTOR)
		    atom = Omake_struct->data.atom;
		else if (atom->property->structure.function == STRUCT_CHECK)
		    atom = Ostruct_type->data.atom;
		else
		    atom = Ostruct_access->data.atom;
		builtin = atom->property->fun.builtin;

		cons.type = LispCons_t;
		cons.data.cons.cdr = arguments;
		if (eval) {
		    LispObj quote;

		    quote.type = LispQuote_t;
		    quote.data.quote = function;
		    cons.data.cons.car = &quote;
		    base = LispMakeEnvironment(mac, atom->property->alist,
					       &cons, function, 1, 1);
		}
		else {
		    cons.data.cons.car = function;
		    base = LispMakeEnvironment(mac, atom->property->alist,
					       &cons, function, 0, 1);
		}
		multiple_values = 0;
		result = builtin->function(mac, builtin);
		mac->stack.length = base;
	    }
	    else {
		LispDestroy(mac, "EVAL: the function %s is not defined",
			    STROBJ(function));
		/*NOTREACHED*/
		multiple_values = 0;
		result = NIL;
	    }
	    break;
	case LispLambda_t:
	    lambda = function->data.lambda.code;
	    alist = (LispArgList*)function->data.lambda.name->data.opaque.data;
	    base = LispMakeEnvironment(mac, alist, arguments, function, eval, 0);
	    multiple_values = 0;
	    result = LispRunFunMac(mac, function, lambda, 0, base);
	    break;
	case LispCons_t:
	    if (CAR(function) == Olambda) {
		function = EVAL(function);
		if (function->type == LispLambda_t) {
		    GC_ENTER();

		    GC_PROTECT(function);
		    lambda = function->data.lambda.code;
		    alist = (LispArgList*)function->data.lambda.name->data.opaque.data;
		    base = LispMakeEnvironment(mac, alist, arguments, NIL, eval, 0);
		    multiple_values = 0;
		    result = LispRunFunMac(mac, NIL, lambda, 0, base);
		    GC_LEAVE();
		    break;
		}
	    }
	default:
	    LispDestroy(mac, "EVAL: %s is invalid as a function",
			STROBJ(function));
	    /*NOTREACHED*/
	    multiple_values = 0;
	    result = NIL;
	    break;
    }

#ifdef DEBUGGER
    if (mac->debugging)
	LispDebugger(mac, LispDebugCallEnd, function, result);
#endif

    if (RETURN_COUNT && !multiple_values)
	RETURN_COUNT = 0;

    return (result);
}

LispObj *
LispEval(LispMac *mac, LispObj *object)
{
    LispObj *result;

    switch (object->type) {
	case LispAtom_t:
	    if ((result = LispDoGetVar(mac, object)) == NULL)
		LispDestroy(mac, "EVAL: the variable %s is unbound",
			    STROBJ(object));
	    break;
	case LispCons_t:
	    result = LispFuncall(mac, CAR(object), CDR(object), 1);
	    break;
	case LispQuote_t:
	    result = object->data.quote;
	    break;
	case LispBackquote_t:
	    result = LispEvalBackquote(mac, object->data.quote, 1);
	    break;
	case LispComma_t:
	    result = LispEvalBackquote(mac, object->data.quote, 0);
	    break;
	default:
	    result = object;
	    break;
    }

    return (result);
}

LispObj *
LispApply1(LispMac *mac, LispObj *function, LispObj *argument)
{
    LispObj arguments;

    arguments.type = LispCons_t;
    arguments.mark = LispNil_t;
    arguments.data.cons.car = argument;
    arguments.data.cons.cdr = NIL;

    return (LispFuncall(mac, function, &arguments, 0));
}

LispObj *
LispApply2(LispMac *mac, LispObj *function,
	   LispObj *argument1, LispObj *argument2)
{
    LispObj arguments, cdr;

    arguments.type = cdr.type = LispCons_t;
    arguments.mark = cdr.mark = LispNil_t;
    arguments.data.cons.car = argument1;
    arguments.data.cons.cdr = &cdr;
    cdr.data.cons.car = argument2;
    cdr.data.cons.cdr = NIL;

    return (LispFuncall(mac, function, &arguments, 0));
}

static LispObj *
LispRunFunMac(LispMac *mac, LispObj *name, LispObj *code, int macro, int base)
{
    LispObj *result = NIL;

    if (!macro) {
	int lex = mac->env.lex;
	int did_jump = 1, *pdid_jump;
	LispObj **pcode, **presult;
	LispBlock *block = LispBeginBlock(mac, name, LispBlockClosure);

	mac->env.lex = base;
	if (setjmp(block->jmp) == 0) {
	    for (pcode = &code, presult = &result, pdid_jump = &did_jump;
		 CONS_P(code); code = CDR(code))
		result = EVAL(CAR(code));
	    did_jump = 0;
	}
	LispEndBlock(mac, block);
	if (did_jump)
	    result = mac->block.block_ret;
	mac->env.lex = lex;
	mac->env.head = mac->env.length = base;
    }
    else {
	GC_ENTER();

	for (; CONS_P(code); code = CDR(code))
	    result = EVAL(CAR(code));
	mac->env.head = mac->env.length = base;

	GC_PROTECT(result);
	result = EVAL(result);
	GC_LEAVE();
    }

    return (result);
}

LispObj *
LispRunSetf(LispMac *mac, LispArgList *alist,
	    LispObj *setf, LispObj *place, LispObj *value)
{
    GC_ENTER();
    LispObj *store, *code, *expression, *result, quote;
    int base;

    code = setf->data.lambda.code;
    store = setf->data.lambda.data;

    if (NCONSTANT_P(value))
	value = EVAL(value);
    quote.type = LispQuote_t;
    quote.mark = LispNil_t;
    quote.data.quote = value;
    LispDoAddVar(mac, CAR(store), &quote);
    ++mac->env.head;
    base = LispMakeEnvironment(mac, alist, CDR(place), Oexpand_setf_method, 0, 0);

    /* build expansion macro */
    expression = NIL;
    for (; CONS_P(code); code = CDR(code))
	expression = EVAL(CAR(code));

    /* Minus 1 to pop the added variable */
    mac->env.head = mac->env.length = base - 1;

    /* protect expansion, and executes it */
    GC_PROTECT(expression);
    result = EVAL(expression);
    GC_LEAVE();

    return (result);
}

char *
LispStrObj(LispMac *mac, LispObj *object)
{
    static int first = 1;
    static char buffer[34];
    static LispObj stream;
    static LispString string;

    if (first) {
	stream.type = LispStream_t;
	stream.data.stream.source.string = &string;
	stream.data.stream.pathname = NIL;
	stream.data.stream.type = LispStreamString;
	stream.data.stream.readable = 0;
	stream.data.stream.writable = 1;

	string.string = (unsigned char*)buffer;
	string.fixed = 1;
	string.space = sizeof(buffer) - 1;
	first = 0;
    }

    string.length = string.output = 0;

    LispDoWriteObject(mac, &stream, object, 1);

    /* make sure string is nul terminated */
    string.string[string.length] = '\0';
    if (string.length >= 32) {
	if (buffer[0] == '(')
	    strcpy(buffer + 27, "...)");
	else
	    strcpy(buffer + 28, "...");
    }

    return (buffer);
}

void
LispPrint(LispMac *mac, LispObj *obj, LispObj *stream, int newline)
{
    if (!obj || !stream)
	LispDestroy(mac, "internal error, at PRINT");
    if (stream != NIL && !STREAM_P(stream))
	LispDestroy(mac, "%s is not a stream", STROBJ(stream));
    if (newline && LispGetColumn(mac, stream))
	LispWriteChar(mac, stream, '\n');
    LispWriteObject(mac, stream, obj);
    if (stream == NIL || (stream->data.stream.type == LispStreamStandard &&
	stream->data.stream.source.file == Stdout))
	LispFflush(Stdout);
}

void
LispUpdateResults(LispMac *mac, LispObj *cod, LispObj *res)
{
    LispSetVar(mac, RUN[2], LispGetVar(mac, RUN[1]));
    LispSetVar(mac, RUN[1], LispGetVar(mac, RUN[0]));
    LispSetVar(mac, RUN[0], cod);

    LispSetVar(mac, RES[2], LispGetVar(mac, RES[1]));
    LispSetVar(mac, RES[1], LispGetVar(mac, RES[0]));
    LispSetVar(mac, RES[0], res);
}

/* Needs a rewrite to either allow only one LispMac per process or some
 * smarter error handling */
static LispMac *global_mac = NULL;

#ifdef SIGNALRETURNSINT
int
LispAbortSignal(int signum)
{
    if (global_mac != NULL)
	LispDestroy(global_mac, "aborted");
}

int
LispFPESignal(int signum)
{
    if (global_mac != NULL)
	LispDestroy(global_mac, "floating point exception");
}
#else
void
LispAbortSignal(int signum)
{
    if (global_mac != NULL)
	LispDestroy(global_mac, "aborted");
}

void
LispFPESignal(int signum)
{
    if (global_mac != NULL)
	LispDestroy(global_mac, "floating point exception");
}
#endif

void
LispMachine(LispMac *mac)
{
    LispObj *cod, *obj;

    mac->sigint = signal(SIGINT, LispAbortSignal);
    mac->sigfpe = signal(SIGFPE, LispFPESignal);
    global_mac = mac;

    /*CONSTCOND*/
    while (1) {
	if (sigsetjmp(mac->jmp, 1) == 0) {
	    mac->running = 1;
	    if (mac->interactive && mac->prompt) {
		LispFputs(Stdout, mac->prompt);
		LispFflush(Stdout);
	    }
	    if ((cod = LispRead(mac)) != NULL) {
		if (cod == EOLIST)
		    LispDestroy(mac, "object cannot start with #\\)");
		else if (cod == DOT)
		    LispDestroy(mac, "dot allowed only on lists");
		obj = EVAL(cod);
		if (mac->interactive) {
		    LispPrint(mac, obj, NIL, 1);
		    if (RETURN_COUNT) {
			int i;

			for (i = 0; i < RETURN_COUNT; i++)
			    LispPrint(mac, RETURN(i), NIL, 1);
		    }
		    LispUpdateResults(mac, cod, obj);
		    if (LispGetColumn(mac, NIL))
			LispWriteChar(mac, NIL, '\n');
		}
	    }
	    LispTopLevel(mac);
	    if (mac->eof)
		break;
	    continue;
	}
    }

    signal(SIGINT, mac->sigint);
    signal(SIGFPE, mac->sigfpe);
    global_mac = NULL;

    mac->running = 0;
}

void *
LispExecute(LispMac *mac, char *str)
{
    static LispObj stream;
    static LispString string;
    static int first = 1;

    int running = mac->running;
    LispObj *result, *cod, *obj, **presult = &result;

    if (str == NULL || *str == '\0')
	return (NIL);

    *presult = NIL;

    if (first) {
	stream.type = LispStream_t;
	stream.data.stream.source.string = &string;
	stream.data.stream.pathname = NIL;
	stream.data.stream.type = LispStreamString;
	stream.data.stream.readable = 1;
	stream.data.stream.writable = 0;
	string.output = 0;
	first = 0;
    }
    string.string = (unsigned char*)str;
    string.length = strlen(str);
    string.input = 0;

    LispPushInput(mac, &stream);
    if (!running) {
	mac->running = 1;
	if (sigsetjmp(mac->jmp, 1) != 0)
	    return (NULL);
    }

    cod = COD;
    /*CONSTCOND*/
    while (1) {
	if ((obj = LispRead(mac)) != NULL) {
	    if (obj == EOLIST)
		LispDestroy(mac, "EXECUTE: object cannot start with #\\)");
	    else if (obj == DOT)
		LispDestroy(mac, "dot allowed only on lists");
	    result = EVAL(obj);
	    COD = cod;
	}
	if (mac->eof)
	    break;
    }
    LispPopInput(mac, &stream);

    mac->running = running;

    return (result);
}

LispMac *
LispBegin(void)
{
    int i;
    LispAtom *atom;
    char results[4];
    LispMac *mac = malloc(sizeof(LispMac));
    LispObj *object, *path, *lisp, *ext;

    if (mac == NULL)
	return (NULL);

    pagesize = LispGetPageSize();
    segsize = pagesize / sizeof(LispObj);

    /* Initialize constant objects. */
    for (i = 0; i < 256; i++)
	LispChars[i].character.data.integer = i;
    for (i = 0; i < 128; i++) {
	LispSmallInts[i].type = LispInteger_t;
	LispSmallInts[i].data.integer = i - 16;
    }

    NIL->mark = LispTrue_t;

    /* Initialize memory management */
    mac->mem.mem = (void**)calloc(mac->mem.space = 16, sizeof(void*));
    mac->mem.index = mac->mem.level = 0;

    /* Allow LispGetVar to check ATOMID() of unbound symbols */
    UNBOUND->data.atom = (LispAtom*)LispCalloc(mac, 1, sizeof(LispAtom));
    LispMused(mac, UNBOUND->data.atom);
    noproperty.value = UNBOUND;

    if (Stdin == NULL)
	Stdin = LispFdopen(0, FILE_READ);
    if (Stdout == NULL)
	Stdout = LispFdopen(1, FILE_WRITE | FILE_BUFFERED);
    if (Stderr == NULL)
	Stderr = LispFdopen(2, FILE_WRITE);

    /* minimum number of free cells after GC
     * if sizeof(LispObj) == 16, than a minfree of 1024 would try to keep
     * at least 16Kb of free cells.
     */
    minfree = 1024;

    bzero((char*)mac, sizeof(LispMac));
    MOD = COD = PRO = DOC = NIL;
#ifdef DEBUGGER
    DBG = BRK = NIL;
#endif

    /* allocate initial object cells */
    LispAllocSeg(mac, &objseg, minfree);
    LispAllocSeg(mac, &atomseg, pagesize);
    mac->gc.average = segsize;

    /* Don't allow gc in initialization */
    GCProtect();

    /* Initialize package system, the current package is LISP. Order of
     * initialization is very important here */
    lisp = LispNewPackage(mac, STRING("LISP"), NIL);

    /* Make LISP package the current one */
    mac->pack = mac->savepack = lisp->data.package.package;

    /* Allocate space in LISP package */
    LispMoreGlobals(mac, mac->pack);

    /* Allocate initial space for multiple returns */
    LispMoreReturns(mac);

    /*  Create the first atom, do it "by hand" because macro "PACKAGE"
     * cannot yet be used. */
    atom = LispGetPermAtom(mac, "*PACKAGE*");
    mac->package = atomseg.freeobj;
    atomseg.freeobj = CDR(atomseg.freeobj);
    --atomseg.nfree;
    mac->package->type = LispAtom_t;
    mac->package->data.atom = atom;
    atom->object = mac->package;
    atom->package = lisp;

    /* Set package list, to be used by (gc) and (list-all-packages) */
    PACK = CONS(lisp, NIL);

    /* Make *PACKAGE* a special variable */
    LispProclaimSpecial(mac, mac->package, lisp, NIL);

	/* Value of macro "PACKAGE" is now properly available */

    /* Changing *PACKAGE* is like calling (in-package) */
    mac->package->data.atom->watch = 1;

    /* And available to other packages */
    LispExportSymbol(mac, mac->package);

    /* Initialize stacks */
    LispMoreEnvironment(mac);
    LispMoreStack(mac);

    /* Create the KEYWORD package */
    Skeyword = GETATOMID("KEYWORD");
    object = LispNewPackage(mac, STRING(Skeyword), CONS(STRING(""), NIL));

    /* Update list of packages */
    PACK = CONS(object, PACK);

    /* Allow easy access to the keyword package */
    mac->keyword = object;
    mac->key = object->data.package.package;

    /* Initialize some static important symbols */
    Olambda		= STATIC_ATOM("LAMBDA");
    Okey		= STATIC_ATOM("&KEY");
    LispExportSymbol(mac, Okey);
    Orest		= STATIC_ATOM("&REST");
    LispExportSymbol(mac, Orest);
    Ooptional		= STATIC_ATOM("&OPTIONAL");
    LispExportSymbol(mac, Ooptional);
    Oaux		= STATIC_ATOM("&AUX");
    LispExportSymbol(mac, Oaux);
    Kunspecific		= KEYWORD("UNSPECIFIC");
    Oformat		= STATIC_ATOM("FORMAT");
    Oexpand_setf_method	= STATIC_ATOM("EXPAND-SETF-METHOD");

    Omake_struct	= STATIC_ATOM("MAKE-STRUCT");
    Ostruct_access	= STATIC_ATOM("STRUCT-ACCESS");
    Ostruct_store	= STATIC_ATOM("STRUCT-STORE");
    Ostruct_type	= STATIC_ATOM("STRUCT-TYPE");
    Smake_struct	= ATOMID(Omake_struct);
    Sstruct_access	= ATOMID(Ostruct_access);
    Sstruct_store	= ATOMID(Ostruct_store);
    Sstruct_type	= ATOMID(Ostruct_type);

    /* Initialize some static atom ids */
    Snil		= GETATOMID("NIL");
    St			= GETATOMID("T");
    Saux		= ATOMID(Oaux);
    Skey		= ATOMID(Okey);
    Soptional		= ATOMID(Ooptional);
    Srest		= ATOMID(Orest);
    Sand		= GETATOMID("AND");
    Sor			= GETATOMID("OR");
    Snot		= GETATOMID("NOT");
    Satom		= GETATOMID("ATOM");
    Ssymbol		= GETATOMID("SYMBOL");
    Sinteger		= GETATOMID("INTEGER");
    Scharacter		= GETATOMID("CHARACTER");
    Sreal		= GETATOMID("REAL");
    Sstring		= GETATOMID("STRING");
    Slist		= GETATOMID("LIST");
    Scons		= GETATOMID("CONS");
    Svector		= GETATOMID("VECTOR");
    Sarray		= GETATOMID("ARRAY");
    Sstruct		= GETATOMID("STRUCT");
    Sfunction		= GETATOMID("FUNCTION");
    Spathname		= GETATOMID("PATHNAME");
    Srational		= GETATOMID("RATIONAL");
    Sfloat		= GETATOMID("FLOAT");
    Scomplex		= GETATOMID("COMPLEX");
    Sopaque		= GETATOMID("OPAQUE");
    Sdefault		= GETATOMID("DEFAULT");

    LispArgList_t	= LispRegisterOpaqueType(mac, "LispArgList*");

    mac->unget = malloc(sizeof(LispUngetInfo*));
    mac->unget[0] = calloc(1, sizeof(LispUngetInfo));
    mac->nunget = 1;

    mac->standard_input = ATOM2("*STANDARD-INPUT*");
    SINPUT = STANDARDSTREAM(Stdin, object, STREAM_READ);
    mac->interactive = 1;
    LispProclaimSpecial(mac, mac->standard_input, mac->input_list = SINPUT, NIL);
    LispExportSymbol(mac, mac->standard_input);

    mac->standard_output = ATOM2("*STANDARD-OUTPUT*");
    SOUTPUT = STANDARDSTREAM(Stdout, object, STREAM_WRITE);
    LispProclaimSpecial(mac, mac->standard_output, mac->output_list = SOUTPUT, NIL);
    LispExportSymbol(mac, mac->standard_output);

    object = ATOM2("*STANDARD-ERROR*");
    mac->error_stream = STANDARDSTREAM(Stderr, object, STREAM_WRITE);
    LispProclaimSpecial(mac, object, mac->error_stream, NIL);
    LispExportSymbol(mac, object);

    mac->modules = ATOM2("*MODULES*");
    LispProclaimSpecial(mac, mac->modules, MOD, NIL);
    LispExportSymbol(mac, mac->modules);

    object = CONS(KEYWORD("UNIX"), CONS(KEYWORD("XEDIT"), NIL));
    mac->features = ATOM2("*FEATURES*");
    LispProclaimSpecial(mac, mac->features, object, NIL);
    LispExportSymbol(mac, mac->features);

    /* Reenable gc */
    GCUProtect();

    LispBytecodeInit(mac);
    LispPackageInit(mac);
    LispCoreInit(mac);
    LispMathInit(mac);
    LispPathnameInit(mac);
    LispStreamInit(mac);
    LispRegexInit(mac);

    mac->prompt = "> ";

    mac->errexit = !mac->interactive;

    if (mac->interactive) {
	/* add +, ++, +++, *, **, and *** */
	for (i = 0; i < 3; i++) {
	    results[i] = '+';
	    results[i + 1] = '\0';
	    RUN[i] = ATOM(results);
	    LispSetVar(mac, RUN[i], NIL);
	    LispExportSymbol(mac, RUN[i]);
	}
	for (i = 0; i < 3; i++) {
	    results[i] = '*';
	    results[i + 1] = '\0';
	    RES[i] = ATOM(results);
	    LispSetVar(mac, RES[i], NIL);
	    LispExportSymbol(mac, RES[i]);
	}
    }
    else
	RUN[0] = RUN[1] = RUN[2] = RES[0] = RES[1] = RES[2] = NIL;

    /* Add LISP builtin functions */
    for (i = 0; i < sizeof(lispbuiltins) / sizeof(lispbuiltins[0]); i++)
	LispAddBuiltinFunction(mac, &lispbuiltins[i]);

    EXECUTE("(require \"lisp\")");

    object = ATOM2("*DEFAULT-PATHNAME-DEFAULTS*");
#ifdef LISPDIR
    {
	int length;
	char *pathname = LISPDIR;

	length = strlen(pathname);
	if (length && pathname[length - 1] != '/') {
	    pathname = LispMalloc(mac, length + 2);

	    strcpy(pathname, LISPDIR);
	    strcpy(pathname + length, "/");
	    path = LSTRING2(pathname, length + 1);
	}
	else
	    path = LSTRING(pathname, length);
    }
#else
    path = STRING("");
#endif
    /* XXX When pathname.c be fixed in the gc handling, will need to
     * protect path here. */
    LispProclaimSpecial(mac, object, APPLY1(Oparse_namestring, path), NIL);
    LispExportSymbol(mac, object);

    /* Create and make EXT the current package */
    PACKAGE = ext = LispNewPackage(mac, STRING("EXT"), NIL);
    mac->pack = mac->savepack = PACKAGE->data.package.package;

    /* Update list of packages */
    PACK = CONS(ext, PACK);

    /* Import LISP external symbols in EXT package */
    LispUsePackage(mac, lisp);

    /* Add EXT non standard builtin functions */
    for (i = 0; i < sizeof(extbuiltins) / sizeof(extbuiltins[0]); i++)
	LispAddBuiltinFunction(mac, &extbuiltins[i]);

    /* Create and make USER the current package */
    PACKAGE = LispNewPackage(mac, STRING("USER"), NIL);
    mac->pack = mac->savepack = PACKAGE->data.package.package;

    /* Update list of packages */
    PACK = CONS(PACKAGE, PACK);

    /* USER package inherits all LISP external symbols */
    LispUsePackage(mac, lisp);
    /* And all EXT external symbols */
    LispUsePackage(mac, ext);

    LispTopLevel(mac);

    return (mac);
}

void
LispEnd(LispMac *mac)
{
    /* XXX needs to free all used memory, not just close file descriptors */
}

void
LispSetPrompt(LispMac *mac, char *prompt)
{
    mac->prompt = prompt;
}

void
LispSetInteractive(LispMac *mac, int interactive)
{
    mac->interactive = !!interactive;
}

void
LispSetExitOnError(LispMac *mac, int errexit)
{
    mac->errexit = !!errexit;
}

void
LispDebug(LispMac *mac, int enable)
{
    mac->debugging = !!enable;

#ifdef DEBUGGER
    /* assumes we are at the toplevel */
    DBG = BRK = NIL;
    mac->debug_level = -1;
    mac->debug_step = 0;
#endif
}

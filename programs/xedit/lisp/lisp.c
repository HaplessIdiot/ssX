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

/* $XFree86: xc/programs/xedit/lisp/lisp.c,v 1.33 2002/02/12 16:07:54 paulo Exp $ */

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

#include "private.h"

#include "read.h"
#include "format.h"
#include "math.h"
#include "package.h"
#include "pathname.h"
#include "require.h"
#include "stream.h"
#include "struct.h"
#include "time.h"
#include "write.h"
#include <math.h>

/*
 * Prototypes
 */
/* run a user function, to be called only by LispEval */
static LispObj *LispRunFunMac(LispMac*, LispObj*, LispObj*, char*);

/* expands and executes a setf method, to be called only by Lisp_Setf */
LispObj *LispRunSetf(LispMac*, LispObj*, LispObj*, LispObj*);

/* increases storage size for environment */
static void LispMoreEnvironment(LispMac*);

/* increases storage size for global variables */
static void LispMoreGlobals(LispMac*);

/* increases storage size for special variables */
static void LispMoreSpecials(LispMac*);

/* increases storage size for dynamic variables */
static void LispMoreDynamics(LispMac*);

#ifdef __GNUC__
static INLINE LispObj *LispDoGetVar(LispMac*, LispObj*);
static INLINE void LispDoAddVar(LispMac*, LispObj*, LispObj*);
#endif

/* Helper for importing symbol(s) functions,
 * Search for the specified object in the current package */
static INLINE LispObj *LispGetVarPack(LispMac*, LispObj*);

/* create environment for function call */
static void LispMakeEnvironment(LispMac*, LispObj*, LispObj*, char*, int);

	/* if not already in keyword package, move atom to keyword package */
static void LispCheckKeyword(LispMac*, LispObj*);

	/* builtin backquote parsing */
static LispObj *LispEvalBackquoteObject(LispMac*, LispObj*, int, int);
static LispObj *LispEvalBackquote(LispMac*, LispObj*, int);

	/* create or change object property */
static INLINE void LispSetAtomObjectProperty(LispMac*, LispAtom*, LispObj*);
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

void LispCheckMemLevel(LispMac*);

void LispAllocSeg(LispMac*, int);
static INLINE void LispMark(LispObj*);

/* functions, macros, setf methods, and structure definitions */
static INLINE void LispImmutable(LispObj*);
/* if redefined or unbounded */
static INLINE void LispMutable(LispObj*);

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

LispObj *Okey, *Orest, *Ooptional, *Oaux;

Atom_id Snil, St;
Atom_id Slambda, Saux, Skey, Soptional, Srest;
Atom_id Satom, Ssymbol, Sinteger, Scharacter, Sreal, Sstring, Slist,
	Scons, Svector, Sarray, Sstruct, Skeyword, Sfunction, Spathname,
	Srational, Sfloat, Scomplex, Sopaque, Sdefault;

LispObj *Oformat, *Kunspecific;

static LispObj **objseg, *freeobj = &lispnil;
static LispProperty noproperty;
LispProperty *NOPROPERTY = &noproperty;
static int segsize, minfree, numseg;
static int nfree, nobjs;
int pagesize, gcpro;

LispFile *Stdout, *Stdin, *Stderr;

extern char *LispCharNames[];

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
    {LispFunction, Lisp_AlphaCharP, "alpha-char-p char"},
    {LispFunction, Lisp_Append, "append &rest lists"},
    {LispFunction, Lisp_Apply, "apply function arg &rest more-args"},
    {LispMacro, Lisp_And, "and &rest args"},
    {LispFunction, Lisp_Aref, "aref array &rest subscripts"},
    {LispFunction, Lisp_Assoc, "assoc item list &key test test-not key"},
    {LispFunction, Lisp_Atom, "atom object"},
    {LispMacro, Lisp_Block, "block name &rest body"},
    {LispFunction, Lisp_Butlast, "butlast list &optional (count 1) &aux (length (length list))"},
    {LispFunction, Lisp_Car, "car list"},
    {LispMacro, Lisp_Case, "case keyform &rest body"},
    {LispMacro, Lisp_Catch, "catch tag &rest body"},
    {LispFunction, Lisp_Cdr, "cdr list"},
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
    {LispFunction, Lisp_Complex, "complex realpart &optional imagpart"},
    {LispFunction, Lisp_Complexp, "complexp object"},
    {LispMacro, Lisp_Cond, "cond &rest body"},
    {LispFunction, Lisp_Conjugate, "conjugate number"},
    {LispFunction, Lisp_Cons, "cons car cdr"},
    {LispFunction, Lisp_Listp, "consp object"},
    {LispFunction, Lisp_Close, "close stream &key abort"},
    {LispMacro, Lisp_Decf, "decf place &optional delta"},
    {LispMacro, Lisp_Defmacro, "defmacro name lambda-list &rest body"},
    {LispMacro, Lisp_Defstruct, "defstruct name &rest description"},
    {LispMacro, Lisp_Defun, "defun name lambda-list &rest body"},
    {LispMacro, Lisp_Defsetf, "defsetf function lambda-list &rest body"},
    {LispMacro, Lisp_Defvar, "defvar name &optional initial-value documentation"},
    {LispFunction, Lisp_Denominator, "denominator rational"},
    {LispFunction, Lisp_DigitCharP, "digit-char-p character &optional (radix 10)"},
    {LispFunction, Lisp_Directory, "directory pathname &key all if-cannot-read"},
    {LispFunction, Lisp_DirectoryNamestring, "directory-namestring pathname"},
    {LispMacro, Lisp_Do, "do init test &rest body"},
    {LispMacro, Lisp_DoP, "do* init test &rest body"},
    {LispFunction, Lisp_Documentation, "documentation symbol type"},
    {LispMacro, Lisp_DoList, "dolist init &rest body"},
    {LispMacro, Lisp_DoTimes, "dotimes init &rest body"},
    {LispMacro, Lisp_DoAllSymbols, "do-all-symbols init &rest body"},
    {LispMacro, Lisp_DoExternalSymbols, "do-external-symbols init &rest body"},
    {LispMacro, Lisp_DoSymbols, "do-symbols init &rest body"},
    {LispFunction, Lisp_Elt, "elt sequence index &aux (length (length sequence))"},
    {LispFunction, Lisp_Null, "endp list"},
    {LispFunction, Lisp_EnoughNamestring, "enough-namestring pathname &optional defaults"},
    {LispFunction, Lisp_Eq, "eq left right"},
    {LispFunction, Lisp_Eql, "eql left right"},
    {LispFunction, Lisp_Equal, "equal left right"},
    {LispMacro, Lisp_Error, "error control-string &rest arguments"},
    {LispFunction, Lisp_Eval, "eval form"},
    {LispFunction, Lisp_Evenp, "evenp integer"},
    {LispFunction, Lisp_FileNamestring, "file-namestring pathname"},
    {LispFunction, Lisp_Car, "first list"},
    {LispFunction, Lisp_FindAllSymbols, "find-all-symbols string-or-symbol"},
    {LispFunction, Lisp_FindPackage, "find-package name"},
    {LispFunction, Lisp_Float, "float number &optional (other 1.0)"},
    {LispFunction, Lisp_Floatp, "floatp object"},
    {LispFunction, Lisp_Fmakunbound, "fmakunbound symbol"},
    {LispFunction, Lisp_Format, "format destination control-string &rest arguments"},
    {LispFunction, Lisp_Funcall, "funcall function &rest arguments"},
    {LispFunction, Lisp_Gc, "gc &optional car cdr"},
    {LispFunction, Lisp_Gcd, "gcd &rest integers"},
    {LispFunction, Lisp_Get, "get symbol indicator &optional default"},
    {LispMacro, Lisp_Go, "go tag"},
    {LispFunction, Lisp_HostNamestring, "host-namestring pathname"},
    {LispMacro, Lisp_If, "if test then &optional else"},
    {LispFunction, Lisp_Imagpart, "imagpart number"},
    {LispMacro, Lisp_InPackage, "in-package name"},
    {LispMacro, Lisp_Incf, "incf place &optional delta"},
    {LispFunction, Lisp_InputStreamP, "input-stream-p stream"},
    {LispFunction, Lisp_IntChar, "int-char integer"},
    {LispFunction, Lisp_Integerp, "integerp object"},
    {LispFunction, Lisp_Isqrt, "isqrt natural"},
    {LispFunction, Lisp_Keywordp, "keywordp object"},
    {LispFunction, Lisp_Last, "last list &optional (count 1) &aux (length (length list))"},
    {LispMacro, Lisp_Lambda, "lambda lambda-list &rest body"},
    {LispFunction, Lisp_Lcm, "lcm &rest integers"},
    {LispFunction, Lisp_Length, "length sequence"},
    {LispMacro, Lisp_Let, "let init &rest body"},
    {LispMacro, Lisp_LetP, "let* init &rest body"},
    {LispFunction, Lisp_List, "list &rest args"},
    {LispFunction, Lisp_ListP, "list* object &rest more-objects"},
    {LispFunction, Lisp_ListAllPackages, "list-all-packages"},
    {LispFunction, Lisp_Listp, "listp object"},
    {LispFunction, Lisp_Listen, "listen &optional input-stream"},
    {LispFunction, Lisp_Load, "load filename &key verbose print if-does-not-exist"},
    {LispFunction, Lisp_Logand, "logand &rest integers"},
    {LispFunction, Lisp_Logeqv, "logeqv &rest integers"},
    {LispFunction, Lisp_Logior, "logior &rest integers"},
    {LispFunction, Lisp_Lognot, "lognot integer"},
    {LispFunction, Lisp_Logxor, "logxor &rest integers"},
    {LispMacro, Lisp_Loop, "loop &rest body"},
    {LispFunction, Lisp_MakeArray, "make-array dimensions &key element-type initial-element initial-contents adjustable fill-pointer displaced-to displaced-index-offset"},
    {LispFunction, Lisp_MakeList, "make-list size &key initial-element"},
    {LispFunction, Lisp_MakePathname, "make-pathname &key host device directory name type version defaults"},
    {LispFunction, Lisp_MakeStringInputStream, "make-string-input-stream string &optional start end"},
    {LispFunction, Lisp_MakeStringOutputStream, "make-string-output-stream &key element-type"},
    {LispFunction, Lisp_GetOutputStreamString, "get-output-stream-string string-output-stream"},
    {LispFunction, Lisp_Makunbound, "makunbound symbol"},
    {LispFunction, Lisp_Mapcar, "mapcar function list &rest more-lists"},
    {LispFunction, Lisp_Maplist, "maplist function list &rest more-lists"},
    {LispFunction, Lisp_Member, "member item list &key test test-not key"},
    {LispFunction, Lisp_Minusp, "minusp number"},
    {LispFunction, Lisp_Nconc, "nconc &rest lists"},
    {LispFunction, Lisp_Null, "not arg"},
    {LispFunction, Lisp_Nth, "nth index list"},
    {LispFunction, Lisp_Nthcdr, "nthcdr index list"},
    {LispFunction, Lisp_Null, "null list"},
    {LispFunction, Lisp_Numerator, "numerator rational"},
    {LispFunction, Lisp_Namestring, "namestring pathname"},
    {LispFunction, Lisp_Numberp, "numberp object"},
    {LispFunction, Lisp_Oddp, "oddp integer"},
    {LispFunction, Lisp_Open, "open filename &key direction element-type if-exists if-does-not-exist external-format"},
    {LispFunction, Lisp_OpenStreamP, "open-stream-p stream"},
    {LispMacro, Lisp_Or, "or &rest args"},
    {LispFunction, Lisp_OutputStreamP, "output-stream-p stream"},
    {LispFunction, Lisp_PackageName, "package-name package"},
    {LispFunction, Lisp_PackageNicknames, "package-nicknames package"},
    {LispFunction, Lisp_PackageUseList, "package-use-list package"},
    {LispFunction, Lisp_PackageUsedByList, "package-used-by-list package"},
    {LispFunction, Lisp_ParseInteger, "parse-integer string &key start end radix junk-allowed"},
    {LispFunction, Lisp_ParseNamestring, "parse-namestring object &optional host defaults &key start end junk-allowed"},
    {LispFunction, Lisp_PathnameHost, "pathname-host pathname"},
    {LispFunction, Lisp_PathnameDevice, "pathname-device pathname"},
    {LispFunction, Lisp_PathnameDirectory, "pathname-directory pathname"},
    {LispFunction, Lisp_PathnameName, "pathname-name pathname"},
    {LispFunction, Lisp_PathnameType, "pathname-type pathname"},
    {LispFunction, Lisp_PathnameVersion, "pathname-version pathname"},
    {LispFunction, Lisp_Pathnamep, "pathnamep object"},
    {LispFunction, Lisp_Plusp, "plusp number"},
    {LispFunction, Lisp_Prin1, "prin1 object &optional output-stream"},
    {LispFunction, Lisp_Princ, "princ object &optional output-stream"},
    {LispFunction, Lisp_Print, "print object &optional output-stream"},
    {LispFunction, Lisp_ProbeFile, "probe-file pathname"},
    {LispFunction, Lisp_Proclaim, "proclaim declaration"},
    {LispMacro, Lisp_Prog1, "prog1 first &rest body"},
    {LispMacro, Lisp_Prog2, "prog2 first second &rest body"},
    {LispMacro, Lisp_Progn, "progn &rest body"},
    {LispMacro, Lisp_Progv, "progv symbols values &rest body"},
    {LispFunction, Lisp_Provide, "provide module"},
    {LispFunction, Lisp_Quit, "quit &optional status"},
    {LispMacro, Lisp_Quote, "quote object"},
    {LispFunction, Lisp_Rational, "rational number"},
    {LispFunction, Lisp_Rationalp, "rationalp object"},
    {LispFunction, Lisp_Read, "read &optional input-stream (eof-error-p t) eof-value recursive-p"},
    {LispFunction, Lisp_ReadChar, "read-char &optional input-stream (eof-error-p t) eof-value recursive-p"},
    {LispFunction, Lisp_ReadCharNoHang, "read-char-no-hang &optional input-stream (eof-error-p t) eof-value recursive-p"},
    {LispFunction, Lisp_ReadLine, "read-line &optional input-stream (eof-error-p t) eof-value recursive-p"},
    {LispFunction, Lisp_Realpart, "realpart number"},
    {LispFunction, Lisp_Replace, "replace sequence1 sequence2 &key start1 end1 start2 end2 &aux (length1 (length sequence1)) (length2 (length sequence2))"},
    {LispFunction, Lisp_ReadFromString, "read-from-string string &optional eof-error-p eof-value &key start end preserve-whitespace"},
    {LispFunction, Lisp_Require, "require module &optional pathname"},
    {LispFunction, Lisp_Cdr, "rest list"},
    {LispMacro, Lisp_Return, "return &optional result"},
    {LispMacro, Lisp_ReturnFrom, "return-from name &optional result"},
    {LispFunction, Lisp_Reverse, "reverse sequence"},
    {LispFunction, Lisp_Rplaca, "rplaca place value"},
    {LispFunction, Lisp_Rplacd, "rplacd place value"},
    {LispFunction, Lisp_Set, "set symbol value"},
    {LispMacro, Lisp_Setf, "setf &rest form"},
    {LispMacro, Lisp_SetQ, "setq &rest form"},
    {LispFunction, Lisp_Sleep, "sleep seconds"},
    {LispFunction, Lisp_Sqrt, "sqrt number"},
    {LispFunction, Lisp_Elt, "svref sequence index &aux (length (length sequence))"},
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
    {LispFunction, Lisp_Subseq, "subseq sequence start &optional end &aux (length (length sequence))"},
    {LispFunction, Lisp_Symbolp, "symbolp object"},
    {LispFunction, Lisp_SymbolPlist, "symbol-plist symbol"},
    {LispMacro, Lisp_Tagbody, "tagbody &rest body"},
    {LispFunction, Lisp_Terpri, "terpri &optional output-stream"},
    {LispFunction, Lisp_Typep, "typep object type"},
    {LispMacro, Lisp_The, "the value-type form"},
    {LispMacro, Lisp_Throw, "throw tag result"},
    {LispMacro, Lisp_Time, "time form"},
    {LispFunction, Lisp_Truename, "truename pathname"},
    {LispMacro, Lisp_Unless, "unless test &rest body"},
    {LispFunction, Lisp_Unsetenv, "unsetenv name"},
    {LispFunction, Lisp_UserHomedirPathname, "user-homedir-pathname &optional host"},
    {LispMacro, Lisp_UnwindProtect, "unwind-protect protect &rest cleanup"},
    {LispFunction, Lisp_Vector, "vector &rest objects"},
    {LispMacro, Lisp_When, "when test &rest body"},
    {LispFunction, Lisp_WriteChar, "write-char string &optional output-stream"},
    {LispFunction, Lisp_WriteLine, "write-line string &optional output-stream &key start end"},
    {LispFunction, Lisp_WriteString, "write-string string &optional output-stream &key start end"},
    {LispFunction, Lisp_XeditCharStore, "lisp::char-store string index value &aux (length (length string))", 1},
    {LispFunction, Lisp_XeditEltStore, "lisp::elt-store sequence index value &aux (length (length sequence))", 1},
    {LispFunction, Lisp_XeditMakeStruct, "lisp::make-struct atom &rest init", 1},
    {LispFunction, Lisp_XeditPut, " lisp::put symbol indicator value", 1},
    {LispFunction, Lisp_XeditStructAccess, "lisp::struct-access atom struct", 1},
    {LispFunction, Lisp_XeditStructType, "lisp::struct-type atom struct", 1},
    {LispFunction, Lisp_XeditStructStore, "lisp::struct-store atom struct value", 1},
    {LispFunction, Lisp_XeditVectorStore, "lisp::vector-store array subscripts value", 1},
    {LispFunction, Lisp_Zerop, "zerop number"},
};

static LispBuiltin extbuiltins[] = {
    {LispFunction, Lisp_Getenv, "getenv name"},
    {LispFunction, Lisp_MakePipe, "make-pipe command-line &key direction element-type external-format"},
    {LispFunction, Lisp_PipeBroken, "pipe-broken pipe-stream"},
    {LispFunction, Lisp_PipeErrorStream, "pipe-error-stream pipe-stream"},
    {LispFunction, Lisp_PipeInputDescriptor, "pipe-input-descriptor pipe-stream"},
    {LispFunction, Lisp_PipeErrorDescriptor, "pipe-error-descriptor pipe-stream"},
    {LispFunction, Lisp_Setenv, "setenv name value &optional overwrite"},
    {LispMacro, Lisp_Until, "until test &rest body"},
    {LispMacro, Lisp_While, "while test &rest body"},
};

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

	if (mac->debugging) {
	    LispDebugger(mac, LispDebugCallWatch, NIL, NIL);
	    LispDebugger(mac, LispDebugCallFatal, NIL, NIL);
	}

	mac->destroyed = 1;
	LispBlockUnwind(mac, NULL);
	if (mac->errexit)
	    exit(1);
    }

    if (mac->debugging) {
	/* when stack variables could be changed, this must be also changed! */
	mac->debug_level = -1;
	mac->debug = LispDebugUnspec;
    }

    while (mac->mem.mem_level)
	free(mac->mem.mem[--mac->mem.mem_level]);

    /* If the package was changed and an error happened */
    PACKAGE = mac->savepackage;
    mac->pack = mac->savepack;

    LispTopLevel(mac);

    if (!mac->running) {
	static char Fatal[] = "*** Fatal: nowhere to longjmp.\n";

	LispFputs(Stderr, Fatal);
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
    COD = FRM = NIL;
    if (mac->debugging) {
	DBG = NIL;
	if (mac->debug == LispDebugFinish)
	    mac->debug = LispDebugUnspec;
	mac->debug_level = -1;
	mac->debug_step = 0;
    }
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

    if (CONS_P(mac->input)) {
	LispUngetInfo **info, *unget = mac->unget[0];

	while (CONS_P(mac->input))
	    mac->input = CDR(mac->input);
	SINPUT = mac->input;
	while (mac->nunget > 1)
	    free(mac->unget[--mac->nunget]);
	if ((info = realloc(mac->unget, sizeof(LispUngetInfo*))) != NULL)
	    mac->unget = info;
	mac->unget[0] = unget;
	mac->iunget = 0;
    }

    if (mac->mem.mem_level) {
	LispWarning(mac, "%d raw memory pointer(s) left. Probably a leak.",
		    mac->mem.mem_level);
	mac->mem.mem_level = 0;
    }

    mac->env.lex = mac->env.base = mac->env.length = mac->env.head = 0;
    mac->dyn.length = 0;
    mac->protect.length = 0;

    mac->savepackage = PACKAGE;
    mac->savepack = mac->pack;
}

void
LispGC(LispMac *mac, LispObj *car, LispObj *cdr)
{
    LispObj *entry, *last, **pentry, **eentry;
    unsigned i, j;
    LispAtom *atom;
    struct timeval start, end;
#ifdef DEBUG
    long sec, msec;
    int count = nfree;
#else
    long msec;
#endif

    if (gcpro)
	return;

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
	    pentry < eentry; pentry++) {
	    (*pentry)->mark = LispTrue_t;
	    LispMark((*pentry)->data.atom->property->value);
	}

	/* Protect special symbols */
	for (pentry = pack->spc.pairs, eentry = pentry + pack->spc.length;
	     pentry < eentry; pentry++) {
	    (*pentry)->mark = LispTrue_t;
	    LispMark((*pentry)->data.atom->property->value);
	}

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
			else if (atom->a_builtin)
			    LispImmutable(atom->property->fun.builtin->description);
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

    /* protect environment i.e. stack of function arguments */
    for (pentry = mac->env.pairs, eentry = pentry + mac->env.length;
	 pentry < eentry; pentry += 2)
	/* don't need to protect atom, as it is protected in function definition */
	LispMark(*pentry);

    /* protect rebounded special variables */
    for (pentry = mac->dyn.pairs, eentry = pentry + mac->dyn.length;
	 pentry < eentry; pentry += 2)
	/* don't need to protect atom, as it is protected in packages step */
	LispMark(*pentry);

    /* protect temporary data used by builtin functions */
    for (pentry = mac->protect.objects, eentry = pentry + mac->protect.length;
	 pentry < eentry; pentry++)
	LispMark(*pentry);

    LispMark(COD);
    LispMark(FRM);
    LispMark(DBG);
    LispMark(BRK);
    LispMark(PRO);
    LispMark(DOC);
    LispMark(mac->input);
    LispMark(mac->output);
    LispMark(car);
    LispMark(cdr);

    /* this actually only need to be done when directly calling LispGC */
    nfree = 0;
    freeobj = NIL;

    for (j = 0; j < numseg; j++) {
	for (entry = objseg[j], last = entry + segsize; entry < last; entry++) {
	    if (entry->mark)
		entry->mark = LispNil_t;
	    else if (!entry->prot) {
		switch (entry->type) {
		    case LispString_t:
			free(THESTR(entry));
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
			break;
		    case LispBigInteger_t:
			mpi_clear(entry->data.mp.integer);
			free(entry->data.mp.integer);
			break;
		    case LispBigRatio_t:
			mpr_clear(entry->data.mp.ratio);
			free(entry->data.mp.ratio);
			break;
		    default:
			break;
		}
		entry->type = LispNil_t;
		CDR(entry) = freeobj;
		freeobj = entry;
		++nfree;
	    }
	}
    }

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
		nfree - count, nfree, nobjs - nfree, nobjs);
#else
    if (mac->gc.timebits) {
	gettimeofday(&end, NULL);
	if ((msec = end.tv_usec - start.tv_usec) < 0)
	    msec += 1000000;
	mac->gc.gctime += msec;
    }
#endif
}

void
LispCheckMemLevel(LispMac *mac)
{
    if (mac->mem.mem_level + 1 >= mac->mem.mem_size) {
	void **ptr = (void**)realloc(mac->mem.mem,
				     (mac->mem.mem_size + 16) * sizeof(void*));

	if (ptr == NULL)
	    LispDestroy(mac, "out of memory");
	mac->mem.mem = ptr;
	mac->mem.mem_size += 16;
    }
}

void
LispMused(LispMac *mac, void *pointer)
{
    int i;

    for (i = 0; i < mac->mem.mem_level; i++)
	if (mac->mem.mem[i] == pointer) {
	    --mac->mem.mem_level;
	    if (mac->mem.mem_level > i)
		memmove(mac->mem.mem + i, mac->mem.mem + i + 1,
			sizeof(void*) * (mac->mem.mem_level - i));
	    break;
	}
}

void *
LispMalloc(LispMac *mac, unsigned size)
{
    void *pointer;

    LispCheckMemLevel(mac);
    if ((pointer = malloc(size)) == NULL)
	LispDestroy(mac, "out of memory, couldn't allocate %u bytes", size);

    mac->mem.mem[mac->mem.mem_level++] = pointer;

    return (pointer);
}

void *
LispCalloc(LispMac *mac, unsigned nmemb, unsigned size)
{
    void *pointer;

    LispCheckMemLevel(mac);
    if ((pointer = calloc(nmemb, size)) == NULL)
	LispDestroy(mac, "out of memory, couldn't allocate %u bytes", size);

    mac->mem.mem[mac->mem.mem_level++] = pointer;

    return (pointer);
}

void *
LispRealloc(LispMac *mac, void *pointer, unsigned size)
{
    void *ptr;
    int i;

    for (i = 0; i < mac->mem.mem_level; i++)
	if (mac->mem.mem[i] == pointer)
	    break;
    if (i == mac->mem.mem_level)
	LispCheckMemLevel(mac);

    if ((ptr = realloc(pointer, size)) == NULL)
	LispDestroy(mac, "out of memory, couldn't realloc");

    if (i == mac->mem.mem_level)
	mac->mem.mem[mac->mem.mem_level++] = ptr;
    else
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

    for (i = 0; i < mac->mem.mem_level; i++)
	if (mac->mem.mem[i] == pointer)
	    break;

    /* If the memory was allocated on a previous form, just free it */
    if (i < mac->mem.mem_level) {
	memmove(mac->mem.mem + i, mac->mem.mem + i + 1,
		sizeof(void*) * (mac->mem.mem_level - i - 1));
	--mac->mem.mem_level;
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

static INLINE void
LispSetAtomObjectProperty(LispMac *mac, LispAtom *atom, LispObj *object)
{
    if (atom->property == NOPROPERTY)
	LispAllocAtomProperty(mac, atom);
    else if (atom->watch) {
	if (atom->object == PACKNAM) {
	    if (!PACKAGE_P(object))
		LispDestroy(mac, "Symbol %s must be a package, not %s",
			    STRPTR(PACKNAM), STROBJ(object));
	    mac->pack = object->data.package.package;
	}
    }

    atom->a_object = 1;
    atom->property->value = object;
}

static void
LispRemAtomObjectProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->a_object) {
	atom->a_object = 0;
	atom->property->value = NULL;
    }
#ifdef DEBUG
    else
	LispDestroy(mac, "internal error at REMOVE-OBJECT-PROPERTY");
#endif
}

void
LispSetAtomFunctionProperty(LispMac *mac, LispAtom *atom, LispObj *function)
{
    if (atom->property == NOPROPERTY)
	LispAllocAtomProperty(mac, atom);

    if (atom->a_function == 0 && atom->a_builtin == 0)
	atom->a_function = 1;
    else {
	mac->gc.immutablebits = 1;
	if (atom->a_builtin) {
	    LispMutable(atom->property->fun.builtin->description);
	    atom->a_builtin = 0;
	}
	else
	    LispMutable(atom->property->fun.function);
    }

    LispImmutable(function);

    atom->property->fun.function = function;
}

void
LispRemAtomFunctionProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->a_function) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property->fun.function);
	atom->property->fun.function = NULL;
	atom->a_function = 0;
    }
#if 0
    else
	LispDestroy(mac, "internal error at REMOVE-FUNCTION-PROPERTY");
#endif
}

void
LispSetAtomBuiltinProperty(LispMac *mac, LispAtom *atom, LispBuiltin *builtin)
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
	}
	else
	    LispMutable(atom->property->fun.builtin->description);
    }

    LispImmutable(builtin->description);

    atom->property->fun.builtin = builtin;
}

void
LispRemAtomBuiltinProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->a_builtin) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property->fun.builtin->description);
	atom->property->fun.function = NULL;
	atom->a_builtin = 0;
    }
#ifdef DEBUG
    else
	LispDestroy(mac, "internal error at REMOVE-BUILTIN-PROPERTY");
#endif
}

void
LispSetAtomSetfProperty(LispMac *mac, LispAtom *atom, LispObj *setf)
{
    if (atom->property == NOPROPERTY)
	LispAllocAtomProperty(mac, atom);

    if (atom->a_defsetf) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property->setf);
    }

    atom->a_defsetf = 1;
    atom->property->setf = setf;

    LispImmutable(setf);
}

void
LispRemAtomSetfProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->a_defsetf) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property->setf);
	atom->property->setf = NULL;
	atom->a_defsetf = 0;
    }
#ifdef DEBUG
    else
	LispDestroy(mac, "internal error at REMOVE-SETF-PROPERTY");
#endif
}

void
LispSetAtomStructProperty(LispMac *mac, LispAtom *atom, LispObj *def, int fun)
{
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
#ifdef DEBUG
    else
	LispDestroy(mac, "internal error at REMOVE-STRUCTURE-PROPERTY");
#endif
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
	    if (LispEqual(mac, key, CAR(obj)) == T) {
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

    CAR(res) = val;

    return (res);
}

/* Used to make sure that when defining a function like:
 *	(defun my-function (... &key key1 key2 key3 ...)
 * key1, key2, and key3 will be in the keyword package
 */
static void
LispCheckKeyword(LispMac *mac, LispObj *keyword)
{
    if (KEYWORD_P(keyword))
	return;

    (void)KEYWORD(STRPTR(keyword));
}

void
LispCheckArguments(LispMac *mac, LispFunType type, LispObj *args, char *name)
{
    static char *types[4] = {"LAMBDA-LIST", "FUNCTION", "MACRO", "SETF-METHOD"};
    static char *fnames[4] = {"LAMBDA", "DEFUN", "DEFMACRO", "DEFSETF"};
#define IKEY		0
#define IOPTIONAL	1
#define IREST		2
#define IAUX		3
    static char *keys[4] = {"&KEY", "&OPTIONAL", "&REST", "&AUX"};
    LispObj *obj;
    int rest, optional, key, aux;

    rest = optional = key = aux = 0;
    if (CONS_P(args)) {
	for(; CONS_P(args); args = CDR(args)) {
	    obj = CAR(args);

	    if (CONS_P(obj)) {
		if (aux) {
		    if (!SYMBOL_P(CAR(obj)) ||
			(CDR(obj) != NIL && CDR(CDR(obj)) != NIL))
			LispDestroy(mac,
				    "%s %s: bad &AUX argument %s",
				    fnames[type], name, LispStrObj(mac, obj));
		}
		else if ((key || optional) && !rest) {
		    if (key && CONS_P(CAR(obj))) {
			/* check for special case, as in:
			 *  (defun a (&key ((key name) 'default-value)) name)
			 *  (a 'key 'test)  => TEST
			 *  (a) 	    => DEFAULT-VALUE
			 */
			if (!SYMBOL_P(CAR(CAR(obj))) ||
			    !CONS_P(CDR(CAR(obj))) ||
			    !SYMBOL_P(CAR(CDR(CAR(obj)))) ||
			    (CDR(CDR(CAR(obj))) != NIL &&
			     CDR(CDR(CDR(CAR(obj))))))
			    LispDestroy(mac, "%s %s: bad special &KEY %s",
					fnames[type], name, STROBJ(obj));
		    }
		    /* is this a default value? */
		    else if (!SYMBOL_P(CAR(obj)))
			LispDestroy(mac,
				    "%s %s: %s cannot be a %s argument name",
				    fnames[type], name,
				    STROBJ(CAR(obj)), types[type]);
		    /* check if default value provided, and optionally a `svar' */
		    else if (CDR(obj) != NIL &&
			     (!CONS_P(CDR(obj)) ||
			      (CDR(CDR(obj)) != NIL &&
			       (!SYMBOL_P(CAR(CDR(CDR(obj)))) ||
				CDR(CDR(CDR(obj))) != NIL))))
			LispDestroy(mac,
				    "%s %s: bad argument specification %s",
				    fnames[type], name, STROBJ(obj));
		    else if (key)
			/* add to keyword package */
			LispCheckKeyword(mac, CAR(obj));
		}
		else
		    LispDestroy(mac, "%s %s: bad argument specification %s",
				fnames[type], name, STROBJ(obj));
	    }
	    else if (!SYMBOL_P(obj) || KEYWORD_P(obj))
		LispDestroy(mac, "%s %s: %s cannot be a %s argument",
			    fnames[type], name, STROBJ(obj), types[type]);
	    else if (STRPTR(obj)[0] == '&') {
		Atom_id atom = ATOMID(obj);

		if (atom == Srest) {
		    if (rest || aux ||
			CDR(args) == NIL || !SYMBOL_P(CAR(CDR(args))))
			LispDestroy(mac, "%s %s: syntax error parsing %s",
				    fnames[type], name, STRPTR(obj));
		    if (key)
			LispDestroy(mac, "%s %s: %s not allowed after %s",
				    fnames[type], name, keys[IREST], keys[IKEY]);
		    rest = 1;
		}
		else if (atom == Skey) {
		    if (rest || aux)
			LispDestroy(mac, "%s %s: %s not allowed after %s",
				    fnames[type], name, STRPTR(obj),
				    rest ? keys[IREST] : keys[IAUX]);
		    key = 1;
		}
		else if (atom == Soptional) {
		    if (rest || optional || aux)
			LispDestroy(mac, "%s %s: %s not allowed after %s",
				    fnames[type], name, STRPTR(obj),
				    rest ? keys[IREST] : optional ?
				    keys[IOPTIONAL] : keys[IAUX]);
		    optional = 1;
		}
		else if (atom == Saux) {
		    /* &AUX must be the last keyword parameter */
		    if (aux)
			LispDestroy(mac, "%s %s: syntax error parsing %s",
				    fnames[type], name, STRPTR(obj));
		    aux = 1;
		}
		else
		    LispDestroy(mac, "%s %s: %s not allowed/implemented",
				fnames[type], name, STRPTR(obj));
	    }
	    else if (key)
		/* add to keyword package */
		LispCheckKeyword(mac, obj);
	}
	if (args != NIL)
	    LispDestroy(mac, "%s %s: %s cannot end %s arguments",
			fnames[type], name, STROBJ(args), types[type]);
    }
    else if (args != NIL)
	LispDestroy(mac, "%s %s: %s cannot be a %s argument",
		    fnames[type], name, STROBJ(args), types[type]);
}

void
LispAddBuiltinFunction(LispMac *mac, LispBuiltin *builtin)
{
    static LispObj stream;
    static LispString string;
    static int first = 1;
    LispObj *name, *obj, *list, *cons, *code;
    LispAtom *atom;
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
	CDR(cons) = CONS(obj, NIL);
	cons = CDR(cons);
    }
    LispPopInput(mac, &stream);

    atom = name->data.atom;
    LispCheckArguments(mac, builtin->type, CDR(list), atom->string);
    builtin->description = list;
    LispSetAtomBuiltinProperty(mac, atom, builtin);

    /* Make function a extern symbol, unless told to not do so */
    if (!builtin->internal)
	LispExportSymbol(mac, name);

    mac->protect.length = length;
    mac->eof = 0;
    COD = code;			/* LispRead protect data in COD */
}

void
LispAllocSeg(LispMac *mac, int cellcount)
{
    unsigned int i;
    LispObj **list, *obj;

    while (nfree < cellcount) {
	if ((obj = (LispObj*)calloc(1, sizeof(LispObj) * segsize)) == NULL)
	    LispDestroy(mac, "out of memory");
	if ((list = (LispObj**)realloc(objseg,
				       sizeof(LispObj*) * (numseg + 1))) == NULL) {
	    free(obj);
	    LispDestroy(mac, "out of memory");
	}
	objseg = list;
	objseg[numseg] = obj;

	nfree += segsize;
	nobjs += segsize;
	for (i = 1; i < segsize; i++, obj++) {
	    CAR(obj) = NIL;
	    CDR(obj) = obj + 1;
	}
	CAR(obj) = NIL;
	CDR(obj) = freeobj;
	freeobj = objseg[numseg];
	++numseg;
    }
#ifdef DEBUG
    LispMessage(mac, "gc: %d cell(s) allocated at %d segment(s)", nobjs, numseg);
#endif
}

static INLINE void
LispMark(register LispObj *obj)
{
    if (obj->mark)
	return;

mark_again:
    switch (obj->type) {
	case LispNil_t:		/* only NIL and UNBOUND (should) have this type */
	case LispTrue_t:	/* only T (should) has this type */
	    return;
	case LispLambda_t:
	    if (obj->data.lambda.name != NIL)
		obj->data.lambda.name->mark = LispTrue_t;
	    obj->mark = LispTrue_t;
	    obj = obj->data.lambda.code;
	    goto mark_again;
	case LispQuote_t:
	case LispBackquote_t:
	    if (obj->data.quote->type <= LispAtom_t) {
		obj->data.quote->mark = LispTrue_t;
		break;
	    }
	    else {
		obj->mark = LispTrue_t;
		obj = obj->data.quote;
		goto mark_again;
	    }
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
	    /* circular list on car */
	    if (CAR(obj) == obj) {
		obj->mark = LispTrue_t;
		obj = CDR(obj);
	    }
	    for (; CONS_P(obj) && !obj->mark; obj = CDR(obj)) {
		if (CAR(obj)->type <= LispAtom_t)
		    CAR(obj)->mark = LispTrue_t;
		else
		    LispMark(CAR(obj));
		obj->mark = LispTrue_t;
	    }
	    if (obj != NIL && !obj->mark)
		goto mark_again;
	    return;
	case LispArray_t:
	    LispMark(obj->data.array.list);
	    obj->mark = LispTrue_t;
	    obj = obj->data.array.dim;
	    goto mark_again;
	    break;
	case LispStruct_t:
	    obj->mark = LispTrue_t;
	    obj = obj->data.struc.fields;
	    goto mark_again;
	    break;
	case LispStream_t:
	    LispMark(obj->data.stream.pathname);
	    if (obj->data.stream.type == LispStreamPipe) {
		obj->mark = LispTrue_t;
		obj = obj->data.stream.source.program->errorp;
		goto mark_again;
	    }
	    break;
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
	    return;
	case LispLambda_t:
	    if (obj->data.lambda.name != NIL)
		obj->data.lambda.name->prot = LispTrue_t;
	    obj->prot = LispTrue_t;
	    obj = obj->data.lambda.code;
	    goto immutable_again;
	case LispQuote_t:
	case LispBackquote_t:
	    if (obj->data.quote->type <= LispAtom_t) {
		obj->data.quote->prot = LispTrue_t;
		break;
	    }
	    else {
		obj->prot = LispTrue_t;
		obj = obj->data.quote;
		goto immutable_again;
	    }
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
	    break;
	case LispStruct_t:
	    obj->prot = LispTrue_t;
	    obj = obj->data.struc.fields;
	    goto immutable_again;
	    break;
	case LispStream_t:
	    LispImmutable(obj->data.stream.pathname);
	    if (obj->data.stream.type == LispStreamPipe) {
		obj->prot = LispTrue_t;
		obj = obj->data.stream.source.program->errorp;
		goto immutable_again;
	    }
	    break;
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
	    return;
	case LispLambda_t:
	    if (obj->data.lambda.name != NIL)
		obj->data.lambda.name->prot = LispNil_t;
	    obj->prot = LispNil_t;
	    obj = obj->data.lambda.code;
	    goto mutable_again;
	case LispQuote_t:
	case LispBackquote_t:
	    if (obj->data.quote->type <= LispAtom_t) {
		obj->data.quote->prot = LispNil_t;
		break;
	    }
	    else {
		obj->prot = LispNil_t;
		obj = obj->data.quote;
		goto mutable_again;
	    }
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
	    break;
	case LispStruct_t:
	    obj->prot = LispNil_t;
	    obj = obj->data.struc.fields;
	    goto mutable_again;
	    break;
	case LispStream_t:
	    LispImmutable(obj->data.stream.pathname);
	    if (obj->data.stream.type == LispStreamPipe) {
		obj->prot = LispNil_t;
		obj = obj->data.stream.source.program->errorp;
		goto mutable_again;
	    }
	    break;
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
LispNew(LispMac *mac, LispObj *car, LispObj *cdr)
{
    LispObj *obj;

    if (freeobj == NIL) {
	int cellcount;

	LispGC(mac, car, cdr);
	mac->gc.average = (nfree + mac->gc.average) >> 1;
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
	if (freeobj == NIL || nfree < cellcount)
	    LispAllocSeg(mac, cellcount);
    }

    obj = freeobj;
    freeobj = CDR(obj);
    --nfree;

    return (obj);
}

LispObj *
LispNewAtom(LispMac *mac, char *str)
{
    LispObj *object;
    LispAtom *atom = LispDoGetAtom(mac, str, 0);

    if (atom->object)
	return (atom->object);

    object = LispNew(mac, NIL, NIL);
    object->type = LispAtom_t;
    object->data.atom = atom;
    atom->object = object;
    atom->package = PACKAGE;

    return (object);
}

LispObj *
LispNewStaticAtom(LispMac *mac, char *str)
{
    LispObj *object;
    LispAtom *atom = LispDoGetAtom(mac, str, 1);

    object = LispNewSymbol(mac, atom);
    object->prot = LispTrue_t;

    return (object);
}

LispObj *
LispNewSymbol(LispMac *mac, LispAtom *atom)
{
    if (atom->object)
	return (atom->object);
    else {
	LispObj *symbol;

	symbol = LispNew(mac, NIL, NIL);
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
    LispObj *real = LispNew(mac, NIL, NIL);

    real->type = LispReal_t;
    real->data.real = value;

    return (real);
}

LispObj *
LispNewString(LispMac *mac, char *str)
{
    LispObj *string = LispNew(mac, NIL, NIL);

    string->data.string = LispStrdup(mac, str);
    /* if allocation did not fail, set the object type */
    string->type = LispString_t;
    LispMused(mac, string->data.string);

    return (string);
}

LispObj *
LispNewAllocedString(LispMac *mac, char *str)
{
    LispObj *string = LispNew(mac, NIL, NIL);

    string->type = LispString_t;
    string->data.string = str;
    LispMused(mac, str);

    return (string);
}

LispObj *
LispNewComplex(LispMac *mac, LispObj *realpart, LispObj *imagpart)
{
    LispObj *complexp = LispNew(mac, realpart, imagpart);

    complexp->type = LispComplex_t;
    complexp->data.complex.real = realpart;
    complexp->data.complex.imag = imagpart;

    return (complexp);
}

LispObj *
LispNewCharacter(LispMac *mac, long c)
{
    LispObj *character = LispNew(mac, NIL, NIL);

    character->type = LispCharacter_t;
    character->data.integer = c;

    return (character);
}

LispObj *
LispNewInteger(LispMac *mac, long i)
{
    LispObj *integer = LispNew(mac, NIL, NIL);

    integer->type = LispInteger_t;
    integer->data.integer = i;

    return (integer);
}

LispObj *
LispNewRatio(LispMac *mac, long num, long den)
/* Note that reduction is done here, so a integer may be returned */
{
    LispObj *ratio;
    long rest, numerator = num, denominator = den;

    if (numerator == 0)
	return (INTEGER(0));

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
	return (INTEGER(numerator));

    ratio = LispNew(mac, NIL, NIL);

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
    dimension = CONS(INTEGER(count), NIL);
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
    LispObj *cons = LispNew(mac, car, cdr);

    cons->type = LispCons_t;
    CAR(cons) = car;
    CDR(cons) = cdr;

    return (cons);
}

LispObj *
LispNewLambda(LispMac *mac, LispObj *name, LispObj *args, LispObj *code,
	      LispFunType type)
{
    LispObj *fun = LispNew(mac, args, code);

    fun->type = LispLambda_t;
    fun->data.lambda.name = name;
    fun->data.lambda.code = CONS(args, code);
    fun->data.lambda.type = type;

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
LispNewStringStream(LispMac *mac, unsigned char *string, int flags)
{
    LispObj *stream = LispNew(mac, NIL, NIL);

    stream->type = LispStream_t;
    SSTREAMP(stream) = LispCalloc(mac, 1, sizeof(LispString));
    SSTREAMP(stream)->string = (unsigned char*)LispStrdup(mac, (char*)string);
    SSTREAMP(stream)->length = strlen((char*)string);
    LispMused(mac, SSTREAMP(stream));
    LispMused(mac, SSTREAMP(stream)->string);
    stream->data.stream.type = LispStreamString;
    stream->data.stream.readable = (flags & STREAM_READ) != 0;
    stream->data.stream.writable = (flags & STREAM_WRITE) != 0;
    SSTREAMP(stream)->space = SSTREAMP(stream)->length + 1;

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
    LispObj *package = LispNew(mac, package, nicknames);
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
    if (current == NULL) {
	/* No conflicts */

	if (symbol->data.atom->a_object) {
	    /* If it is a bounded variable */
	    if (symbol->data.atom->dyn) {
		if (mac->pack->spc.length + 1 >= mac->pack->spc.space)
		    LispMoreSpecials(mac);
		mac->pack->spc.pairs[mac->pack->spc.length++] = symbol;
	    }
	    else {
		if (mac->pack->glb.length + 1 >= mac->pack->glb.space)
		    LispMoreGlobals(mac);
		mac->pack->glb.pairs[mac->pack->glb.length++] = symbol;
	    }
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

    /* Set home-package and unique-atom associated with symbol */
    atom->package = symbol->data.atom->package;
    atom->object = symbol->data.atom->object;

    if (symbol->data.atom->a_object)
	atom->a_object = 1;
    if (symbol->data.atom->a_function)
	atom->a_function = 1;
    if (symbol->data.atom->a_builtin)
	atom->a_builtin = 1;
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
    LispObj **entry, **first;

    for (first = mac->env.pairs + mac->env.lex,
	 entry = mac->env.pairs + mac->env.head - 1;
	 entry > first; entry -= 2)
	if (*entry == atom)
	    return (*(entry - 1));

    name = atom->data.atom;

    if (name->dyn) {
	for (first = mac->dyn.pairs,
	     entry = first + mac->dyn.length - 1;
	     entry > first; entry -= 2)
	    if (*entry == atom)
		return (*(entry - 1));
    }

    return (name->property->value);
}

/* Same code as LispDoGetVar, but returns the address of the pointer to
 * the object value. Used only by the debugger */
void *
LispGetVarAddr(LispMac *mac, LispObj *atom)
{
    LispAtom *name;
    LispObj **entry, **first;

    for (first = mac->env.pairs + mac->env.lex,
	 entry = mac->env.pairs + mac->env.head - 1;
	 entry > first; entry -= 2)
	if (*entry == atom)
	    return (entry - 1);

    name = atom->data.atom;

    if (name->dyn) {
	for (first = mac->dyn.pairs,
	     entry = first + mac->dyn.length - 1;
	     entry > first; entry -= 2)
	    if (*entry == atom)
		return (entry - 1);
    }

    return (name->a_object ? &(name->property->value) : NULL);
}

/* Only removes global variables. To be called by makunbound
 * Local variables are unbounded once their block is closed anyway.
 */
void
LispUnsetVar(LispMac *mac, LispObj *atom)
{
    int i;
    LispAtom *name = atom->data.atom;
    /* XXX no type checking for maximal speed,
     * if got here, atom must be an ATOM */

    LispRemDocumentation(mac, atom, LispDocVariable);

    if (!name->dyn) {
	for (i = mac->pack->glb.length - 1; i > 0; i--)
	    if (mac->pack->glb.pairs[i] == atom) {
		LispRemAtomObjectProperty(mac, name);
		--mac->pack->glb.length;
		if (i < mac->pack->glb.length)
		    memmove(mac->pack->glb.pairs + i, mac->pack->glb.pairs + i + 1,
			    sizeof(LispObj*) * (mac->pack->glb.length - i));
		return;
	    }
    }
    else {
	for (i = mac->pack->spc.length - 1; i > 0; i--)
	    if (mac->pack->spc.pairs[i] == atom) {
		LispRemAtomObjectProperty(mac, name);
		--mac->pack->spc.length;
		if (i < mac->pack->spc.length)
		    memmove(mac->pack->spc.pairs + i, mac->pack->spc.pairs + i + 1,
			    sizeof(LispObj*) * (mac->pack->spc.length - i));
		/* unset hint about dynamically binded variable */
		name->dyn = 0;
		return;
	    }
    }
}

#ifdef __GNUC__
LispObj *
LispAddVar(LispMac *mac, LispObj *atom, LispObj *obj)
{
    LispDoAddVar(mac, atom, obj);

    return (obj);
}

static INLINE void
LispDoAddVar(LispMac *mac, LispObj *atom, LispObj *obj)
#else
#define LispDoAddVar LispAddVar
LispObj *
LispAddVar(LispMac *mac, LispObj *atom, LispObj *obj)
#endif
{
    LispAtom *name = atom->data.atom;

    if (name->dyn) {
	if (mac->dyn.length + 2 >= mac->dyn.space)
	    LispMoreDynamics(mac);

	mac->dyn.pairs[mac->dyn.length++] = obj;
	mac->dyn.pairs[mac->dyn.length++] = atom;
    }

    if (mac->env.length + 2 >= mac->env.space)
	LispMoreEnvironment(mac);

    mac->env.pairs[mac->env.length++] = obj;
    mac->env.pairs[mac->env.length++] = atom;
#ifndef __GNUC__
    return (obj);
#endif
}

LispObj *
LispSetVar(LispMac *mac, LispObj *atom, LispObj *obj)
{
    LispAtom *name;
    LispObj **entry, **first;

    for (first = mac->env.pairs + mac->env.lex,
	 entry = mac->env.pairs + mac->env.head - 1;
	 entry > first; entry -= 2)
	if (*entry == atom)
	    return (*(entry - 1) = obj);

    name = atom->data.atom;

    if (name->dyn) {
	for (first = mac->dyn.pairs, entry = first + mac->dyn.length - 1;
	     entry > first; entry -= 2)
	    if (*entry == atom)
		return (*(entry - 1) = obj);

	if (name->watch) {
	    LispSetAtomObjectProperty(mac, name, obj);

	    return (obj);
	}

	return (name->property->value = obj);
    }

    if (name->a_object) {
	if (name->watch) {
	    LispSetAtomObjectProperty(mac, name, obj);

	    return (obj);
	}

	return (name->property->value = obj);
    }

    LispSetAtomObjectProperty(mac, name, obj);

    if (mac->pack->glb.length + 1 >= mac->pack->glb.space)
	LispMoreGlobals(mac);

    mac->pack->glb.pairs[mac->pack->glb.length++] = atom;

    return (obj);
}

void
LispProclaimSpecial(LispMac *mac, LispObj *atom, LispObj *value, LispObj *doc)
{
    int i, dyn, glb = 0;
    LispAtom *name = atom->data.atom;

    dyn = name->dyn;

    if (!dyn) {
	/* Note: don't check if a local variable already is using the symbol */
	for (i = mac->pack->glb.length - 1; i > 0; i--)
	    if (mac->pack->glb.pairs[i] == atom) {
		glb = 1;
		break;
	    }
    }

    if (dyn) {
	if (name->property->value == UNBOUND && value)
	    /* if variable was just made special, but not bounded */
	    LispSetAtomObjectProperty(mac, name, value);
    }
    else if (glb) {
	/* move variable from GLB to SPC */
	--mac->pack->glb.length;
	if (i < mac->pack->glb.length)
	    memmove(mac->pack->glb.pairs + i, mac->pack->glb.pairs + i + 1,
		    sizeof(LispObj*) * (mac->pack->glb.length - i));

	if (mac->pack->spc.length + 1 >= mac->pack->spc.space)
	    LispMoreSpecials(mac);

	mac->pack->spc.pairs[mac->pack->spc.length] = atom;
	++mac->pack->spc.length;
	/* set hint about dynamically binded variable */
	name->dyn = 1;
    }
    else {
	/* create new special variable */
	LispSetAtomObjectProperty(mac, name, value ? value : UNBOUND);

	if (mac->pack->spc.length + 1 >= mac->pack->spc.space)
	    LispMoreSpecials(mac);

	mac->pack->spc.pairs[mac->pack->spc.length] = atom;
	++mac->pack->spc.length;
	/* set hint about possibly dynamically binded variable */
	name->dyn = 1;
    }

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

    CAR(env) = symbol;

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
		    CDR(prev) = CDR(env);
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

	if (blk == NULL || (block = malloc(sizeof(LispBlock))) == NULL)
	    LispDestroy(mac, "out of memory");
	mac->block.block = blk;
	mac->block.block[mac->block.block_size] = block;
	mac->block.block_size = blevel;
    }
    block = mac->block.block[mac->block.block_level];
    if (type == LispBlockCatch && !CONSTANT_P(tag))
	tag = EVAL(tag);
    block->type = type;
    memcpy(&(block->tag), tag, sizeof(LispObj));

    block->block_level = mac->block.block_level;

    mac->block.block_level = blevel;

    if (mac->debugging) {
	block->debug_level = mac->debug_level;
	block->debug_step = mac->debug_step;
    }

    return (block);
}

void
LispEndBlock(LispMac *mac, LispBlock *block)
{
    mac->block.block_level = block->block_level;

    if (mac->debugging) {
	if (mac->debug_level >= block->debug_level) {
	    while (mac->debug_level > block->debug_level) {
		DBG = CDR(DBG);
		--mac->debug_level;
	    }
	}
	mac->debug_step = block->debug_step;
    }
}

void
LispBlockUnwind(LispMac *mac, LispBlock *block)
{
    LispBlock *unwind;
    int blevel = mac->block.block_level;

    while (blevel > 0) {
	unwind = mac->block.block[--blevel];
	if (unwind->type == LispBlockProtect)
	    longjmp(unwind->jmp, 1);
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
	      argument->data.quote->type == LispBackquote_t)) {
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

static LispObj *
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
			CDR(cdr) = CONS(CAR(object), NIL);
			cdr = CDR(cdr);
		    }
		    if (object != NIL) {
			/* object was a dotted list */
			CDR(cdr) = object;
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
		    CDR(cdr) = CONS(object, NIL);
		    cdr = CDR(cdr);
		}
		else {
		    CDR(cdr) = object;
		    cdr = object;
		}
	    }
	    else {
		if (!CONS_P(object)) {
		    CDR(cdr) = object;
		    /* if object is NIL, it is a empty list appended, not
		     * creating a dotted list. */
		    if (object != NIL)
			cdr = object;
		}
		else {
		    for (; CONS_P(object); object = CDR(object)) {
			CDR(cdr) = CONS(CAR(object), NIL);
			cdr = CDR(cdr);
		    }
		    if (object != NIL) {
			/* object was a dotted list */
			CDR(cdr) = object;
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

static void
LispMoreEnvironment(LispMac *mac)
{
    LispObj **pairs = realloc(mac->env.pairs,
			      (mac->env.space + 256) * sizeof(LispObj*));

    if (pairs == NULL)
	LispDestroy(mac, "out of memory");

    mac->env.pairs = pairs;
    mac->env.space += 256;
}

static void
LispMoreGlobals(LispMac *mac)
{
    LispObj **pairs = realloc(mac->pack->glb.pairs,
			      (mac->pack->glb.space + 256) * sizeof(LispObj*));

    if (pairs == NULL)
	LispDestroy(mac, "out of memory");

    mac->pack->glb.pairs = pairs;
    mac->pack->glb.space += 256;
}

static void
LispMoreSpecials(LispMac *mac)
{
    LispObj **pairs = realloc(mac->pack->spc.pairs,
			      (mac->pack->spc.space + 256) * sizeof(LispObj*));

    if (pairs == NULL)
	LispDestroy(mac, "out of memory");

    mac->pack->spc.pairs = pairs;
    mac->pack->spc.space += 256;
}

/* this is not commonly used, so allocates few data */
static void
LispMoreDynamics(LispMac *mac)
{
    LispObj **pairs = realloc(mac->dyn.pairs,
			      (mac->dyn.space + 32) * sizeof(LispObj*));

    if (pairs == NULL)
	LispDestroy(mac, "out of memory");

    mac->dyn.pairs = pairs;
    mac->dyn.space += 32;
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

static void
LispMakeEnvironment(LispMac *mac, LispObj *desc, LispObj *values,
		    char *fname, int eval)
{
#define XEVAL(var)						\
if (NCONSTANT_P(var)) {						\
    if (SYMBOL_P(var)) {					\
	LispObj *result = LispDoGetVar(mac, var);		\
								\
	if (result == NULL)					\
	    LispDestroy(mac, "EVAL: the variable %s is unbound",\
			STRPTR(var));				\
	var = result;						\
    }								\
    else							\
	var = EVAL(var);					\
}

    Atom_id atom;
    int base, head;
    int rest, optional, key, aux, varset, nkey, ncvt;
    LispObj *arg, *spec, *list, *val, *keyword, *keyp, *restp, *auxp, *karg;

    head = mac->env.head;
    base = mac->env.length;

    arg = values;
    rest = optional = key = aux = 0;
    for (list = desc; CONS_P(list); list = CDR(list)) {
	spec = CAR(list);

	/* check for special lambda list keywords */
	if (SYMBOL_P(spec)) {
	    atom = ATOMID(spec);
	    if (atom[0] == '&') {
		if (atom == Srest) {
		    restp = CDR(list);
		    rest = 1;
		    /* this assumes no errors in arguments specification */
		    if (CDR(restp) != NIL) {
			auxp = CDR(CDR(restp));
			aux = 1;
		    }
		    break;
		}
		else if (atom == Soptional) {
		    optional = 1;
		    continue;
		}
		else if (atom == Skey) {
		    nkey = ncvt = 0;
		    keyp = CDR(list);
		    for (karg = arg; CONS_P(karg); karg = CDR(karg))
			++nkey;
		    if (nkey & 1)
			LispDestroy(mac, "%s: &KEY needs arguments as pairs",
				    fname);
		    nkey >>= 1;
		    karg = arg;
		    key = 1;
		    continue;
		}
		else if (atom == Saux) {
		    auxp = CDR(list);
		    aux = 1;
		    break;
		}
	    }
	    /* else just add to environment */
	}

	if (!key && !optional) {
	    if (!CONS_P(arg))
		LispDestroy(mac, "%s: too few arguments", fname);

	    val = CAR(arg);
	    if (eval) {
		XEVAL(val);
	    }
	    LispDoAddVar(mac, spec, val);

	    arg = CDR(arg);
	    continue;
	}

	varset = 0;

	if (key) {
	    LispObj *keylist, *compar, *keyvar, *defval;
	    keyword = spec;

	    keylist = karg;

	    if (CONS_P(keyword)) {
		defval = CAR(CDR(keyword));
		keyword = CAR(keyword);
	    }
	    else
		defval = NIL;

	    if (CONS_P(keyword)) {
		/* keyword may be in the format 'name */
		keyvar = CAR(CDR(keyword));
		keyword = CAR(keyword);
		atom = ATOMID(keyword);
		for (; CONS_P(keylist); keylist = CDR(keylist)) {
		    compar = CAR(keylist);

		    if (compar->type == LispQuote_t) {
			compar = compar->data.quote;
			if (SYMBOL_P(compar) && ATOMID(compar) == atom) {
			    if (!CONS_P(CDR(keylist)))
				LispDestroy(mac, "%s: expecting '%s value",
					    fname, STRPTR(keyword));
			    val = CADR(keylist);
			    varset = 1;
			    ++ncvt;
			    break;
			}
		    }
		    else if (!KEYWORD_P(compar))
			LispDestroy(mac, "%s: &KEY needs arguments as pairs",
				    fname);
		    keylist = CDR(keylist);
		}
	    }
	    else {
		/* keyword must be in the format :name */
		keyvar = keyword;
		atom = ATOMID(keyword);
		for (; CONS_P(keylist); keylist = CDR(keylist)) {
		    compar = CAR(keylist);

		    if (KEYWORD_P(compar)) {
			if (ATOMID(compar) == atom) {
			    if (!CONS_P(CDR(keylist)))
				LispDestroy(mac, "%s: expecting :%s value",
					    fname, STRPTR(keyword));
			    val = CADR(keylist);
			    varset = 1;
			    ++ncvt;
			    break;
			}
		    }
		    else if (compar->type != LispQuote_t)
			LispDestroy(mac, "%s: &KEY needs arguments as pairs",
				    fname);
		    keylist = CDR(keylist);
		}
	    }

	    if (varset) {
		if (eval) {
		    XEVAL(val);
		}
		LispDoAddVar(mac, keyvar, val);
	    }
	    else {
		/* parameter not specified */
		if (eval && !CONSTANT_P(defval)) {
		    if (CONS_P(spec)) {
			mac->env.lex = base;
			mac->env.head = mac->env.length;
			defval = EVAL(defval);
			mac->env.head = head;
		    }
		    else
			defval = EVAL(defval);
		}
		LispDoAddVar(mac, keyvar, defval);
	    }
	}
	else if (CONS_P(arg)) {
	    varset = 1;
	    val = CAR(arg);
	}
	else
	    val = NIL;

	if (!key) {
	    if (CONS_P(spec)) {
		/* will be true for &OPTIONAL variables with default value */
		if (!varset) {
		    val = CADR(spec);
		    if (eval && !CONSTANT_P(val)) {
			mac->env.lex = base;
			mac->env.head = mac->env.length;
			val = EVAL(val);
			mac->env.head = head;
		    }
		}
		else if (eval) {
		    XEVAL(val);
		}
		LispDoAddVar(mac, CAR(spec), val);
	    }
	    else {
		if (eval) {
		    XEVAL(val);
		}
		LispDoAddVar(mac, spec, val);
	    }
	}

	/* Check for sval form */
	if (CONS_P(spec) && CONS_P(CDR(spec)) && CONS_P(CDR(CDR(spec))))
	    LispDoAddVar(mac, CAR(CDR(CDR(spec))), varset ? T : NIL);

	if (CONS_P(arg)) {
	    if (key)
		arg = CDR(arg);
	    arg = CDR(arg);
	}
    }

    /* make list for &REST, if required */
    if (rest) {
	if (!eval || !CONS_P(arg))
	    LispDoAddVar(mac, CAR(restp), arg);
	else {
	    for (list = arg; CONS_P(list); list = CDR(list))
		if (!CONSTANT_P(CAR(list)))
		    break;

	    if (CONS_P(list)) {
		LispObj *cons;

		val = CAR(arg);
		XEVAL(val);
		LispDoAddVar(mac, CAR(restp), cons = CONS(val, NIL));

		arg = CDR(arg);

		for (; CONS_P(arg); arg = CDR(arg)) {
		    val = CAR(arg);
		    XEVAL(val);
		    CDR(cons) = CONS(val, NIL);
		    cons = CDR(cons);
		}
	    }
	    else
		/* list of constants, don't allocate new cells */
		LispDoAddVar(mac, CAR(restp), arg);
	}
    }
    else if (arg != NIL)
	LispDestroy(mac, "%s: too many arguments", fname);

    if (aux) {
	/* allow using the variables defined here */
	if (eval)
	    mac->env.lex = base;

	for (; CONS_P(auxp); auxp = CDR(auxp)) {
	    spec = CAR(auxp);

	    if (CONS_P(spec)) {
		val = CAR(CDR(spec));
		if (eval) {
		    mac->env.head = mac->env.length;
		    XEVAL(val);
		}
		LispDoAddVar(mac, CAR(spec), val);
	    }
	    else
		LispDoAddVar(mac, spec, NIL);
	}
    }

    if (key && ncvt < nkey) {
	/* an incorrect parameter may have been specified */
	int found;
	LispObj *keylist, *compar, *keywords;

	for (keylist = karg; CONS_P(keylist); keylist = CDR(keylist)) {
	    compar = CAR(keylist);

	    found = 0;
	    for (keywords = keyp; CONS_P(keywords);
		 keywords = CDR(keywords)) {
		keyword = CAR(keywords);

		if (aux && SYMBOL_P(keyword) && ATOMID(keyword) == Saux)
		    /* if &AUX in the list... */
		    break;

		if (CONS_P(keyword))
		    /* if has default value */
		    keyword = CAR(keyword);

		arg = compar;
		if (CONS_P(keyword)) {
		    keyword = CAR(keyword);
		    if (arg->type == LispQuote_t) {
			arg = compar->data.quote;
			if (!SYMBOL_P(arg))
			    break;
			if (ATOMID(arg) == ATOMID(keyword)) {
			    found = 1;
			    break;
			}
		    }
		}
		else {
		    if (KEYWORD_P(arg)) {
			if (ATOMID(arg) == ATOMID(keyword)) {
			    found = 1;
			    break;
			}
		    }
		}
	    }
	    if (!found)
		LispDestroy(mac, "%s: %s is an invalid keyword",
			    fname, STROBJ(compar));
	    keylist = CDR(keylist);
	}
    }

    /* setup stack offsets */
    mac->env.base = base;
    mac->env.head = mac->env.length;
}

LispObj *
LispEval(LispMac *mac, LispObj *obj)
{
    int debug;
    char *strname;
    LispAtom *atom;
    LispObj *name, *fun, *cons, *res;

    switch (obj->type) {
	case LispAtom_t:
	    if (KEYWORD_P(obj))
		res = obj;
	    else if ((res = LispDoGetVar(mac, obj)) == NULL)
		LispDestroy(mac, "EVAL: the variable %s is unbound",
			    STROBJ(obj));
	    return (res);
	case LispQuote_t:
	    return (obj->data.quote);
	case LispBackquote_t:
	    return LispEvalBackquote(mac, obj->data.quote, 1);
	case LispComma_t:
	    LispDestroy(mac, "EVAL: comma outside of backquote");
	case LispCons_t:
	    cons = obj;
	    break;
	default:
	    return (obj);
    }

    debug = mac->debugging;

    name = CAR(cons);
    cons = CDR(cons);
    switch (name->type) {
	case LispAtom_t:
	    atom = name->data.atom;
	    strname = atom->string;
	    break;
	case LispLambda_t:
	    strname = Snil;
	    fun = name;
	    if (debug)
		LispDebugger(mac, LispDebugCallBegin, NIL, cons);
	    goto eval_lambda;
#ifndef COMPILE_LAMBDA
	/* I am not exactly sure if it should be correct to "compile"
	 * the lambda list at read time. It would only break things like:
	 *	(car '(lambda ..))
	 * Anyway, "compiling" the lambda expression would only make
	 * loops like (loop ((lambda ...))) faster, but yet slower than
	 * if a function were defined, because functions aren't traversed
	 * at gc time (unless fmakunbound was called or the function redefined,
	 * but even in this case, the function is traversed only once).
	 */
	case LispCons_t:
	    if (SYMBOL_P(CAR(name)) &&
		ATOMID(CAR(name)) == Slambda) {
		fun = EVAL(name);
		if (fun->type == LispLambda_t) {
		    LispObj *cod;

		    cod = COD;
		    COD = CONS(fun, COD);
		    strname = Snil;
		    if (debug)
			LispDebugger(mac, LispDebugCallBegin, name, cons);

		    res = LispRunFunMac(mac, fun, cons, strname);
		    if (debug)
			LispDebugger(mac, LispDebugCallEnd, fun->data.lambda.name, res);
		    COD = cod;

		    return (res);
		}
	    }
#endif
	default:
	    LispDestroy(mac, "EVAL: %s is invalid as a function", STROBJ(name));
	    /*NOTREACHED*/
    }

    if (debug)
	LispDebugger(mac, LispDebugCallBegin, name, cons);

    if (atom->a_builtin) {
	int head = mac->env.head,
	    lex = mac->env.lex,
	    length = mac->env.length,
	    dyn = mac->dyn.length;
	LispBuiltin *fn = atom->property->fun.builtin;

	LispMakeEnvironment(mac, CDR(fn->description),
			    cons, strname, fn->type != LispMacro);
	if (fn->type != LispMacro)
	    mac->env.lex = length;

	res = fn->function(mac, fn);
	if (debug)
	    LispDebugger(mac, LispDebugCallEnd, name, res);
	mac->env.head = head;
	mac->env.lex = lex;
	mac->env.length = length;
	mac->dyn.length = dyn;

	return (res);
    }

    if (atom->a_function)
	fun = atom->property->fun.function;

    else if (atom->a_defstruct &&
	     atom->property->structure.function != STRUCT_NAME) {
	int head = mac->env.head,
	    lex = mac->env.lex,
	    length = mac->env.length,
	    dyn = mac->dyn.length;
	LispBuiltin *function;

	/* Expands call to xedit::struct-* functions */

	if (atom->property->structure.function == STRUCT_CONSTRUCTOR)
	    atom = Omake_struct->data.atom;
	else if (atom->property->structure.function == STRUCT_CHECK)
	    atom = Ostruct_type->data.atom;
	else
	    atom = Ostruct_access->data.atom;

	function = atom->property->fun.builtin;

	/* XXX it would be better to directly build the environment here,
	 * this would at least avoid the need of quoting `name' */
	LispMakeEnvironment(mac, CDR(function->description),
			    CONS(QUOTE(name), cons),
			    strname, function->type != LispMacro);
	mac->env.lex = length;
	res = function->function(mac, function);
	if (debug)
	    LispDebugger(mac, LispDebugCallEnd, name, res);
	mac->env.head = head;
	mac->env.lex = lex;
	mac->env.length = length;
	mac->dyn.length = dyn;

	return (res);
    }
    else
	LispDestroy(mac, "EVAL: the function %s is not defined", strname);

eval_lambda:
    res = LispRunFunMac(mac, fun, cons, strname);
    if (debug)
	LispDebugger(mac, LispDebugCallEnd, fun->data.lambda.name, res);

    return (res);
}

/* Note: This function is almost a cut&past of LispEval, and basically
 * exists just to make APPLY easier to implement, and faster. Changes
 * in LispEval may need to be reproduced here,
 */
LispObj *
LispApply(LispMac *mac, LispObj *function, LispObj *arguments)
{
    LispObj *result;
    LispAtom *atom;
    char *strname;
    int debug;

    debug = mac->debugging;
    if (SYMBOL_P(function)) {
	atom = function->data.atom;
	strname = atom->string;
	if (debug)
	    LispDebugger(mac, LispDebugCallBegin, function, arguments);
    }
    else {
	strname = Snil;
	if (debug)
	    LispDebugger(mac, LispDebugCallBegin, NIL, arguments);
	goto apply_lambda;
    }

    if (atom->a_builtin) {
	int head = mac->env.head,
	    lex = mac->env.lex,
	    length = mac->env.length,
	    dyn = mac->dyn.length;
	LispBuiltin *fn = atom->property->fun.builtin;

	LispMakeEnvironment(mac, CDR(fn->description),
			    arguments, strname, 0);
	if (fn->type != LispMacro)
	    mac->env.lex = length;

	result = fn->function(mac, fn);
	if (debug)
	    LispDebugger(mac, LispDebugCallEnd, function, result);
	mac->env.head = head;
	mac->env.lex = lex;
	mac->env.length = length;
	mac->dyn.length = dyn;

	return (result);
    }

    if (atom->a_function)
	function = atom->property->fun.function;

    else if (atom->a_defstruct &&
	     atom->property->structure.function != STRUCT_NAME) {
	int head = mac->env.head,
	    lex = mac->env.lex,
	    length = mac->env.length,
	    dyn = mac->dyn.length;
	LispBuiltin *fn;

	/* Expands call to xedit::struct-* functions */

	if (atom->property->structure.function == STRUCT_CONSTRUCTOR)
	    atom = Omake_struct->data.atom;
	else if (atom->property->structure.function == STRUCT_CHECK)
	    atom = Ostruct_type->data.atom;
	else
	    atom = Ostruct_access->data.atom;

	fn = atom->property->fun.builtin;

	LispMakeEnvironment(mac, CDR(fn->description),
			    CONS(function, arguments),
			    strname, 0);
	mac->env.lex = length;
	result = fn->function(mac, fn);
	if (debug)
	    LispDebugger(mac, LispDebugCallEnd, function, result);
	mac->env.head = head;
	mac->env.lex = lex;
	mac->env.length = length;
	mac->dyn.length = dyn;

	return (result);
    }
    else
	LispDestroy(mac, "APPLY: the function %s is not defined", strname);

apply_lambda:
    result = LispRunFunMac(mac, function, arguments, strname);
    if (debug)
	LispDebugger(mac, LispDebugCallEnd, function->data.lambda.name, result);

    return (result);
}

static LispObj *
LispRunFunMac(LispMac *mac, LispObj *fun, LispObj *args, char *strname)
{
    int head = mac->env.head,
	lex = mac->env.lex,
	length = mac->env.length,
	dyn = mac->dyn.length;
    LispObj *code, *result = NIL;
    LispFunType type = fun->data.lambda.type;

    LispMakeEnvironment(mac, CAR(fun->data.lambda.code), args,
			strname, type != LispMacro);
    if (type != LispMacro)
	mac->env.lex = length;

    code = CDR(fun->data.lambda.code);

    if (type != LispMacro) {
	int did_jump = 1, *pdid_jump = &did_jump;
	LispObj **pres = &result;
	LispBlock *block =
	    LispBeginBlock(mac, fun->data.lambda.name, LispBlockClosure);

	if (setjmp(block->jmp) == 0) {
	    for (; CONS_P(code); code = CDR(code))
		if (NCONSTANT_P(result = CAR(code)))
		    result = EVAL(result);
	    *pdid_jump = 0;
	}
	LispEndBlock(mac, block);
	if (*pdid_jump)
	    *pres = mac->block.block_ret;
    }
    else {
	for (; CONS_P(code); code = CDR(code))
	    result = EVAL(CAR(code));
    }

    mac->env.head = head;
    mac->env.lex = lex;
    mac->env.length = length;
    mac->dyn.length = dyn;

    if (type == LispMacro) {
	int length = mac->protect.length;

	if (length + 1 >= mac->protect.space)
	    LispMoreProtects(mac);
	mac->protect.objects[mac->protect.length++] = result;
	result = EVAL(result);
	mac->protect.length = length;
    }

    return (result);
}

LispObj *
LispRunSetf(LispMac *mac, LispObj *setf, LispObj *place, LispObj *value)
{
    LispObj *description, *store, *code, *expression, *result;
    int head = mac->env.head,
	lex = mac->env.lex,
	length = mac->env.length,
	dyn = mac->dyn.length;

    code = setf->data.lambda.code;
    description = CAAR(code);
    store = CDAR(code);
    code = CDR(code);

    if (NCONSTANT_P(value))
	value = EVAL(value);
    LispAddVar(mac, CAR(store), QUOTE(value));
    LispMakeEnvironment(mac, description, CDR(place), "EXPAND-SETF-METHOD", 0);

    /* build expansion macro */
    expression = NIL;
    for (; CONS_P(code); code = CDR(code))
	expression = EVAL(CAR(code));

    mac->env.head = head;
    mac->env.lex = lex;
    mac->env.length = length;
    mac->dyn.length = dyn;

    /* protect expansion, and executes it */
    length = mac->protect.length;
    if (length + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = expression;
    result = EVAL(expression);
    mac->protect.length = length;

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
	string.space = sizeof(buffer);
	first = 0;
    }

    string.length = string.output = 0;

    LispDoWriteObject(mac, &stream, object, 1);

    /* make sure string is nul terminated */
    (void)LispGetSstring(&string);
    if (string.length > 32) {
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
    GCProtect();
    LispSetVar(mac, RUN[2], LispGetVar(mac, RUN[1]));
    LispSetVar(mac, RUN[1], LispGetVar(mac, RUN[0]));
    LispSetVar(mac, RUN[0], cod);

    LispSetVar(mac, RES[2], LispGetVar(mac, RES[1]));
    LispSetVar(mac, RES[1], LispGetVar(mac, RES[0]));
    LispSetVar(mac, RES[0], res);
    GCUProtect();
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

    LispTopLevel(mac);
    /*CONSTCOND*/
    while (1) {
	mac->sigint = signal(SIGINT, LispAbortSignal);
	mac->sigfpe = signal(SIGFPE, LispFPESignal);
	global_mac = mac;
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
		    LispUpdateResults(mac, cod, obj);
		    if (LispGetColumn(mac, NIL))
			LispWriteChar(mac, NIL, '\n');
		}
	    }
	    signal(SIGINT, mac->sigint);
	    signal(SIGFPE, mac->sigfpe);
	    global_mac = NULL;
	    LispTopLevel(mac);
	    if (mac->eof)
		break;
	    continue;
	}
	signal(SIGINT, mac->sigint);
	signal(SIGFPE, mac->sigfpe);
	global_mac = NULL;
    }
    mac->running = 0;
}

void *
LispExecute(LispMac *mac, char *str)
{
    static LispObj stream;
    static LispString string;
    static int first = 1;

    int eof = mac->eof, running = mac->running, length = mac->protect.length;
    LispObj *result, *obj, **presult = &result;

    if (str == NULL || *str == '\0')
	return (NIL);

    *presult = NIL;

    if (length + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = NIL;

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
    if (running || sigsetjmp(mac->jmp, 1) == 0) {
	if (!running)
	    mac->running = 1;

	/*CONSTCOND*/
	while (1) {
	    if ((obj = LispRead(mac)) != NULL) {
		if (obj == EOLIST)
		    LispDestroy(mac, "EXECUTE: object cannot start with #\\)");
		else if (obj == DOT)
		    LispDestroy(mac, "dot allowed only on lists");
		mac->protect.objects[length] = obj;
		result = EVAL(obj);
	    }
	    if (mac->eof)
		break;
	}
    }
    LispPopInput(mac, &stream);

    mac->eof = eof;
    mac->running = running;
    mac->protect.length = length;

    return (result);
}

LispMac *
LispBegin(int argc, char *argv[])
{
    int i;
    char results[4];
    LispMac *mac = malloc(sizeof(LispMac));
    LispObj *object, *lisp, *ext;

    if (mac == NULL)
	return (NULL);

    pagesize = LispGetPageSize();
    segsize = pagesize / sizeof(LispObj);

    /* Initialize memory management */
    mac->mem.mem = (void**)calloc(mac->mem.mem_size = 16, sizeof(void*));
    mac->mem.mem_level = 0;

    if (Stdin == NULL)
	Stdin = LispFdopen(0, FILE_READ);
    if (Stdout == NULL) {
	Stdout = LispFdopen(1, FILE_WRITE);
	Stdout->buffered = 0;
    }
    if (Stderr == NULL) {
	Stderr = LispFdopen(2, FILE_WRITE);
	Stdout->buffered = 0;
    }

    /* minimum number of free cells after GC
     * if sizeof(LispObj) == 16, than a minfree of 1024 would try to keep
     * at least 16Kb of free cells.
     */
    minfree = 1024;

    bzero((char*)mac, sizeof(LispMac));
    MOD = FEAT = COD = FRM = DBG = BRK = PRO = DOC = NIL;

    /* allocate initial object cells */
    LispAllocSeg(mac, minfree);
    mac->gc.average = segsize;

    /* Don't allow gc in initialization */
    GCProtect();

    /* Initialize package system, the current package is LISP. Order of
     * initialization is very important here */
    PACKAGE = lisp = LispNewPackage(mac, STRING("LISP"), NIL);

    /* Make LISP package the current one */
    mac->pack = mac->savepack = PACKAGE->data.package.package;

    /* Allocate space in LISP package */
    LispMoreGlobals(mac);
    LispMoreSpecials(mac);

    /* Now that a package exists, create the very first variable */
    PACKNAM = ATOM2("*PACKAGE*");

    /* Set package list, to be used by (gc) and (list-all-packages) */
    PACK = CONS(PACKAGE, NIL);

    /* Make *PACKAGE* a special variable */
    LispProclaimSpecial(mac, PACKNAM, PACKAGE, NIL);

    /* Changing *PACKAGE* is like calling (in-package) */
    PACKNAM->data.atom->watch = 1;

    /* And available to other packages */
    LispExportSymbol(mac, PACKNAM);

    /* Initialize stacks */
    LispMoreEnvironment(mac);
    LispMoreDynamics(mac);

    /* Create the KEYWORD package */
    Skeyword = GETATOMID("KEYWORD");
    object = LispNewPackage(mac, STRING(Skeyword), CONS(STRING(""), NIL));

    /* Update list of packages */
    PACK = CONS(object, PACK);

    /* Allow easy access to the keyword package */
    mac->keyword = object;
    mac->key = object->data.package.package;

    /* Initialize some static important symbols */
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
    Slambda		= GETATOMID("LAMBDA");
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

    mac->unget = malloc(sizeof(LispUngetInfo*));
    mac->unget[0] = calloc(1, sizeof(LispUngetInfo));
    mac->nunget = 1;

    object = ATOM2("*STANDARD-INPUT*");
    if (argc > 1) {
	int ch;
	LispFile *input = NULL;

	i = 1;
	if (strcmp(argv[1], "-d") == 0) {
	    mac->debugging = 1;
	    mac->debug_level = -1;
	    ++i;
	}
	if (i < argc && (input = LispFopen(argv[i], FILE_READ)) == NULL) {
	    LispMessage(mac, "Cannot open %s.", argv[i]);
	    exit(1);
	}
	if (input) {
	    SINPUT = STANDARDSTREAM(input, object, STREAM_READ);
	    /* ignore first line if starts with #! */
	    ch = LispGet(mac);
	    if (ch != '#')
		LispUnget(mac, ch);
	    else if ((ch = LispGet(mac)) == '!') {
		for (;;) {
		    ch = LispGet(mac);
		    if (ch == '\n' || ch == EOF)
			break;
		}
	    }
	    else {
		LispUnget(mac, ch);
		LispUnget(mac, '#');
	    }
	}
    }
    if (!SINPUT) {
	SINPUT = STANDARDSTREAM(Stdin, object, STREAM_READ);
	mac->interactive = 1;
    }
    LispProclaimSpecial(mac, object, mac->input = SINPUT, NIL);
    LispExportSymbol(mac, object);

    object = ATOM2("*STANDARD-OUTPUT*");
    SOUTPUT = STANDARDSTREAM(Stdout, object, STREAM_WRITE);
    LispProclaimSpecial(mac, object, mac->output = SOUTPUT, NIL);
    LispExportSymbol(mac, object);

    object = ATOM2("*STANDARD-ERROR*");
    mac->error_stream = STANDARDSTREAM(Stderr, object, STREAM_WRITE);
    LispProclaimSpecial(mac, object, mac->error_stream, NIL);
    LispExportSymbol(mac, object);

    mac->modules = ATOM2("*MODULES*");
    LispProclaimSpecial(mac, mac->modules, MOD, NIL);
    LispExportSymbol(mac, mac->modules);

    FEAT = object = CONS(KEYWORD("UNIX"), NIL);
    CDR(object) = CONS(KEYWORD("XEDIT"), NIL);
    /* add more features... object = CDR(object); CDR(object) = ... */
    mac->features = ATOM2("*FEATURES*");
    LispProclaimSpecial(mac, mac->features, FEAT, NIL);
    LispExportSymbol(mac, mac->features);

    /* Reenable gc */
    GCUProtect();

    LispCoreInit(mac);
    LispMathInit(mac);
    LispPathnameInit(mac);
    LispStreamInit(mac);

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

    /* Create EXT package */
    PACKAGE = ext = LispNewPackage(mac, STRING("EXT"), NIL);

    /* Make EXT the current package */
    LispSetVar(mac, PACKNAM, PACKAGE);
    mac->pack = mac->savepack = PACKAGE->data.package.package;

    /* Update list of packages */
    PACK = CONS(PACKAGE, PACK);

    /* Import LISP external symbols in EXT package */
    LispUsePackage(mac, lisp);

    /* Add EXT non standard builtin functions */
    for (i = 0; i < sizeof(extbuiltins) / sizeof(extbuiltins[0]); i++)
	LispAddBuiltinFunction(mac, &extbuiltins[i]);

    /* Create USER package */
    PACKAGE = LispNewPackage(mac, STRING("USER"), NIL);

    /* Make USER the current package */
    LispSetVar(mac, PACKNAM, PACKAGE);
    mac->pack = mac->savepack = PACKAGE->data.package.package;

    /* Update list of packages */
    PACK = CONS(PACKAGE, PACK);

    /* USER package inherits all LISP external symbols */
    LispUsePackage(mac, lisp);
    /* And all EXT external symbols */
    LispUsePackage(mac, ext);

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

    /* assumes we are at the toplevel */
    DBG = BRK = NIL;
    mac->debug_level = -1;
    mac->debug_step = 0;
}

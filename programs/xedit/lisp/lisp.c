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

/* $XFree86: xc/programs/xedit/lisp/lisp.c,v 1.29 2002/01/31 04:33:27 paulo Exp $ */

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

/* create environment for function call */
static void LispMakeEnvironment(LispMac*, LispObj*, LispObj*, char*, int);

static LispObj *LispEvalBackquote(LispMac*, LispObj*);

	/* if no properties remaining, free atom->property,
	 * and this way, make string candidate to gc */
static void LispCheckAtomProperty(LispMac*, LispAtom*);

static LispObj *LispDoGetAtomProperty(LispMac*, LispAtom*, LispObj*, int);

void LispCheckMemLevel(LispMac*);

void LispAllocSeg(LispMac*, int);
void LispMark(LispObj*);

/* functions, macros, setf methods, and structure definitions */
void LispImmutable(LispObj*);
/* if redefined or unbounded */
void LispMutable(LispObj*);

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
LispObj *NIL = &lispnil, *T = &lispt;
static LispObj *UNBOUND = &lispunbound;
static LispObj **objseg, *freeobj = &lispnil;
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
    {LispFunction, Lisp_Float, "float number &optional (other 1.0)"},
    {LispFunction, Lisp_Floatp, "floatp object"},
    {LispFunction, Lisp_Fmakunbound, "fmakunbound symbol"},
    {LispFunction, Lisp_Format, "format destination control-string &rest arguments"},
    {LispFunction, Lisp_Funcall, "funcall function &rest arguments"},
    {LispFunction, Lisp_Gc, "gc &optional car cdr"},
    {LispFunction, Lisp_Gcd, "gcd &rest integers"},
    {LispFunction, Lisp_Get, "get symbol indicator &optional default"},
    {LispFunction, Lisp_Getenv, "getenv name"},
    {LispMacro, Lisp_Go, "go tag"},
    {LispFunction, Lisp_HostNamestring, "host-namestring pathname"},
    {LispMacro, Lisp_If, "if test then &optional else"},
    {LispFunction, Lisp_Imagpart, "imagpart number"},
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
    {LispFunction, Lisp_MakePipe, "make-pipe command-line &key direction element-type external-format"},
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
    {LispFunction, Lisp_ParseInteger, "parse-integer string &key start end radix junk-allowed"},
    {LispFunction, Lisp_ParseNamestring, "parse-namestring object &optional host defaults &key start end junk-allowed"},
    {LispFunction, Lisp_PathnameHost, "pathname-host pathname"},
    {LispFunction, Lisp_PathnameDevice, "pathname-device pathname"},
    {LispFunction, Lisp_PathnameDirectory, "pathname-directory pathname"},
    {LispFunction, Lisp_PathnameName, "pathname-name pathname"},
    {LispFunction, Lisp_PathnameType, "pathname-type pathname"},
    {LispFunction, Lisp_PathnameVersion, "pathname-version pathname"},
    {LispFunction, Lisp_Pathnamep, "pathnamep object"},
    {LispFunction, Lisp_PipeBroken, "pipe-broken pipe-stream"},
    {LispFunction, Lisp_PipeErrorStream, "pipe-error-stream pipe-stream"},
    {LispFunction, Lisp_PipeInputDescriptor, "pipe-input-descriptor pipe-stream"},
    {LispFunction, Lisp_PipeErrorDescriptor, "pipe-error-descriptor pipe-stream"},
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
#if 0
    {LispFunction, Lisp_Rationalize, "rationalize number"},
#endif
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
    {LispFunction, Lisp_Setenv, "setenv name value &optional overwrite"},
    {LispFunction, Lisp_Set, "set symbol value"},
    {LispMacro, Lisp_Setf, "setf &rest form"},
    {LispMacro, Lisp_SetQ, "setq &rest form"},
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
    {LispMacro, Lisp_Throw, "throw tag result"},
    {LispMacro, Lisp_Time, "time form"},
    {LispFunction, Lisp_Truename, "truename pathname"},
    {LispMacro, Lisp_Unless, "unless test &rest body"},
    {LispMacro, Lisp_Until, "until test &rest body"},
    {LispFunction, Lisp_Unsetenv, "unsetenv name"},
    {LispFunction, Lisp_UserHomedirPathname, "user-homedir-pathname &optional host"},
    {LispMacro, Lisp_UnwindProtect, "unwind-protect protect &rest cleanup"},
    {LispFunction, Lisp_Vector, "vector &rest objects"},
    {LispMacro, Lisp_When, "when test &rest body"},
    {LispMacro, Lisp_While, "while test &rest body"},
    {LispFunction, Lisp_WriteChar, "write-char string &optional output-stream"},
    {LispFunction, Lisp_WriteLine, "write-line string &optional output-stream &key start end"},
    {LispFunction, Lisp_WriteString, "write-string string &optional output-stream &key start end"},
    {LispFunction, Lisp_XeditCharStore, "xedit::char-store string index value &aux (length (length string))"},
    {LispFunction, Lisp_XeditEltStore, "xedit::elt-store sequence index value &aux (length (length sequence))"},
    {LispFunction, Lisp_XeditMakeStruct, "xedit::make-struct atom &rest init"},
    {LispFunction, Lisp_XeditPut, " xedit::put symbol indicator value"},
    {LispFunction, Lisp_XeditStructAccess, "xedit::struct-access atom struct"},
    {LispFunction, Lisp_XeditStructType, "xedit::struct-type atom struct"},
    {LispFunction, Lisp_XeditStructStore, "xedit::struct-store atom struct value"},
    {LispFunction, Lisp_XeditVectorStore, "xedit::vector-store array subscripts value"},
    {LispFunction, Lisp_Zerop, "zerop number"},
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

    LispTopLevel(mac);

    if (!mac->running) {
	static char Fatal[] = "*** Fatal: nowhere to longjmp.\n";

	LispFputs(Stderr, Fatal);
	abort();
    }

    siglongjmp(mac->jmp, 1);
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
}

void
LispGC(LispMac *mac, LispObj *car, LispObj *cdr)
{
    LispObj *entry, *last, **pentry, **eentry;
    unsigned i, j;
    LispAtom *atom, *patom, *natom;
    struct timeval start, end;
#ifdef DEBUG
    long sec, msec;
    int count = nfree;
    int strings_free = 0;
#else
    long msec;
#endif
    int full;

    if (gcpro)
	return;

#ifdef DEBUG
    gettimeofday(&start, NULL);
#else
    if (mac->gc.timebits)
	gettimeofday(&start, NULL);
#endif

    if (mac->gc.immutablebits || ++mac->gc.fullbits == 0xff) {
	mac->gc.fullbits = 0;
	full = 1;
    }
    else
	full = 0;

    for (pentry = mac->env.pairs, eentry = pentry + mac->env.length;
	 pentry < eentry;) {
	LispMark(*pentry);
	++pentry;
	(*pentry)->mark = LispTrue_t;
	++pentry;
    }

    for (pentry = mac->glb.pairs, eentry = pentry + mac->glb.length;
	 pentry < eentry; pentry++) {
	(*pentry)->mark = LispTrue_t;
	LispMark((*pentry)->data.atom->property.value);
    }

    for (pentry = mac->spc.pairs, eentry = pentry + mac->spc.length;
	 pentry < eentry; pentry++) {
	(*pentry)->mark = LispTrue_t;
	LispMark((*pentry)->data.atom->property.value);
    }

    for (pentry = mac->dyn.pairs, eentry = pentry + mac->dyn.length;
	 pentry < eentry; pentry += 2) {
	/* don't need to protect atom, as it is protected in spc */
	LispMark((*pentry)->data.atom->property.value);
    }

    for (pentry = mac->protect.objects, eentry = pentry + mac->protect.length;
	 pentry < eentry; pentry++) {
	LispMark((*pentry));
    }

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

    /* Make all strings candidate to be released */
    if (full) {
	for (i = 0; i < STRTBLSZ; i++) {
	    atom = mac->strs[i];
	    while (atom) {
		if (atom->prop) {
		    if (atom->property.property)
			LispMark(atom->property.properties);
		    if (mac->gc.immutablebits) {
			if (atom->property.function)
			    LispImmutable(atom->property.fun.function);
			else if (atom->property.builtin)
			    LispImmutable(atom->property.fun.builtin->description);
			if (atom->property.defsetf)
			    LispImmutable(atom->property.setf);
			if (atom->property.defstruct)
			    LispImmutable(atom->property.structure.definition);
		    }
		    atom->mark = LispTrue_t;
		}
		else
		    atom->mark = LispNil_t;
		atom = atom->next;
	    }
	}

	for (j = 0; j < numseg; j++) {
	    for (entry = objseg[j], last = entry + segsize; entry < last; entry++) {
		if (entry->mark) {
		    entry->mark = LispNil_t;
		    if (entry->type == LispAtom_t || entry->type == LispString_t)
			entry->data.atom->mark = LispTrue_t;
		}
		else if (entry->prot) {
		    if (entry->type == LispAtom_t || entry->type == LispString_t)
			entry->data.atom->mark = LispTrue_t;
		}
		else {
		    switch (entry->type) {
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

	/* Free unused strings */
	for (i = 0; i < STRTBLSZ; i++) {
	    patom = atom = mac->strs[i];
	    while (atom) {
		natom = atom->next;
		if (!atom->mark && !atom->prot) {
		    /* it is not required to call LispFree here */
		    free(atom->string);
		    free(atom);
		    if (patom == atom)
			patom = mac->strs[i] = natom;
		    else
			patom->next = natom;
#ifdef DEBUG
		    ++strings_free;
#endif
		}
		else
		    patom = atom;
		atom = natom;
	    }
	}

    }
    else {
	/* just mark property lists */
	for (i = 0; i < STRTBLSZ; i++) {
	    atom = mac->strs[i];
	    while (atom) {
		if (atom->property.property)
		    LispMark(atom->property.properties);
		atom = atom->next;
	    }
	}

	for (j = 0; j < numseg; j++) {
	    for (entry = objseg[j], last = entry + segsize; entry < last; entry++) {
		if (entry->mark)
		    entry->mark = LispNil_t;
		else if (!entry->prot) {
		    switch (entry->type) {
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
    LispMessage(mac, "gc: %d string(s) freed", strings_free);
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
    return (LispSet(mac, var, val, fname, eval));
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
    opaque->desc = LispDoGetAtom(mac, desc, 1, 0)->string;
    opaque->next = mac->opqs[ii];
    mac->opqs[ii] = opaque;
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

    return ("NIL");
}

LispAtom *
LispDoGetAtom(LispMac *mac, char *str, int prot, int perm)
{
    LispAtom *atom;
    int ii = 0;
    char *pp = str;

    while (*pp)
	ii = (ii << 1) ^ *pp++;
    if (ii < 0)
	ii = -ii;
    ii %= STRTBLSZ;
    for (atom = mac->strs[ii]; atom; atom = atom->next)
	if (strcmp(atom->string, str) == 0) {
	    if (prot && !atom->prot)
		atom->prot = 1;
	    return (atom);
	}
    atom = (LispAtom*)LispCalloc(mac, 1, sizeof(LispAtom));
    if (perm)
	atom->string = str;
    else
	atom->string = LispStrdup(mac, str);
    LispMused(mac, atom);
    if (!perm)
	LispMused(mac, atom->string);
    atom->next = mac->strs[ii];
    mac->strs[ii] = atom;
    if (prot)
	atom->prot = 1;

    return (atom);
}

static void
LispCheckAtomProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->property.object && atom->property.value == NULL)
	atom->property.object = 0;
    if ((atom->property.function || atom->property.builtin) &&
	atom->property.fun.function == NULL)
	atom->property.function = atom->property.builtin = 0;
    if (atom->property.property && atom->property.properties == NULL)
	atom->property.property = 0;
    if (atom->property.defsetf && atom->property.setf == NULL)
	atom->property.defsetf = 0;
    if (atom->property.defstruct &&
	atom->property.structure.definition == NULL)
	atom->property.defstruct = 0;

    if (atom->property.object == 0 && atom->property.function == 0 &&
	atom->property.builtin == 0 && atom->property.properties == 0 &&
	atom->property.defsetf == 0 && atom->property.defstruct == 0)
	atom->prop = 0;
}

void
LispSetAtomObjectProperty(LispMac *mac, LispAtom *atom, LispObj *object)
{
    atom->prop = 1;
    atom->property.object = 1;
    atom->property.value = object;
}

void
LispRemAtomObjectProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->property.value) {
	atom->property.value = NULL;
	LispCheckAtomProperty(mac, atom);
    }
    else
	LispDestroy(mac, "internal error at REMOVE-OBJECT-PROPERTY");
}

void
LispSetAtomFunctionProperty(LispMac *mac, LispAtom *atom, LispObj *function)
{
    atom->prop = 1;

    if (atom->property.fun.function == NULL)
	atom->property.function = 1;
    else {
	mac->gc.immutablebits = 1;
	if (atom->property.builtin) {
	    LispMutable(atom->property.fun.builtin->description);
	    atom->property.builtin = 0;
	}
	else
	    LispMutable(atom->property.fun.function);
    }

    LispImmutable(function);

    atom->property.fun.function = function;
}

void
LispRemAtomFunctionProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->property.fun.function) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property.fun.function);
	atom->property.fun.function = NULL;
	LispCheckAtomProperty(mac, atom);
    }
    else
	LispDestroy(mac, "internal error at REMOVE-FUNCTION-PROPERTY");
}

void
LispSetAtomBuiltinProperty(LispMac *mac, LispAtom *atom, LispBuiltin *builtin)
{
    atom->prop = 1;

    if (atom->property.fun.function == NULL)
	atom->property.builtin = 1;
    else {
	mac->gc.immutablebits = 1;
	if (atom->property.function) {
	    LispMutable(atom->property.fun.function);
	    atom->property.function = 0;
	}
	else
	    LispMutable(atom->property.fun.builtin->description);
    }

    LispImmutable(builtin->description);

    atom->property.fun.builtin = builtin;
}

void
LispRemAtomBuiltinProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->property.fun.builtin) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property.fun.builtin->description);
	atom->property.fun.function = NULL;
	LispCheckAtomProperty(mac, atom);
    }
    else
	LispDestroy(mac, "internal error at REMOVE-BUILTIN-PROPERTY");
}

void
LispSetAtomSetfProperty(LispMac *mac, LispAtom *atom, LispObj *setf)
{
    atom->prop = 1;

    if (atom->property.setf) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property.setf);
    }

    atom->property.defsetf = 1;
    atom->property.setf = setf;

    LispImmutable(setf);
}

void
LispRemAtomSetfProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->property.setf) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->property.setf);
	atom->property.setf = NULL;
	LispCheckAtomProperty(mac, atom);
    }
    else
	LispDestroy(mac, "internal error at REMOVE-SETF-PROPERTY");
}

void
LispSetAtomStructProperty(LispMac *mac, LispAtom *atom, LispObj *def, int fun)
{
    atom->prop = 1;

    if (atom->property.structure.definition) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->object);
	LispMutable(atom->property.structure.definition);
    }

    atom->property.defstruct = 1;
    atom->property.structure.definition = def;
    atom->property.structure.function = fun;

    LispImmutable(atom->object);
    LispImmutable(def);
}

void
LispRemAtomStructProperty(LispMac *mac, LispAtom *atom)
{
    if (atom->property.defstruct) {
	mac->gc.immutablebits = 1;
	LispMutable(atom->object);
	LispMutable(atom->property.structure.definition);
	atom->property.structure.definition = NULL;
	LispCheckAtomProperty(mac, atom);
    }
    else
	LispDestroy(mac, "internal error at REMOVE-STRUCTURE-PROPERTY");
}

char *
LispGetString(LispMac *mac, char *str)
{
    return (LispDoGetAtom(mac, str, 0, 0)->string);
}

char *
LispGetPermString(LispMac *mac, char *str)
{
    return (LispDoGetAtom(mac, str, 1, 1)->string);
}

static LispObj *
LispDoGetAtomProperty(LispMac *mac, LispAtom *atom, LispObj *key, int add)
{
    LispObj *obj = NIL, *res = NIL;

    if (add)
	atom->prop = 1;

    if (add && atom->property.properties == NULL) {
	atom->property.property = 1;
	atom->property.properties = NIL;
    }

    if (atom->property.property) {
	for (obj = atom->property.properties; obj != NIL; obj = CDR(CDR(obj))) {
	    if (LispEqual(mac, key, CAR(obj)) == T) {
		res = CDR(obj);
		break;
	    }
	}
    }

    if (obj == NIL) {
	if (add) {
	    atom->property.properties =
		CONS(key, CONS(NIL, atom->property.properties));
	    res = CDR(atom->property.properties);
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
					fnames[type], name,
					LispStrObj(mac, obj));
		    }
		    /* is this a default value? */
		    else if (!SYMBOL_P(CAR(obj)))
			LispDestroy(mac,
				    "%s %s: %s cannot be a %s argument name",
				    fnames[type], name,
				    LispStrObj(mac, CAR(obj)), types[type]);
		    /* check if default value provided, and optionally a `svar' */
		    else if (CDR(obj) != NIL &&
			     (!CONS_P(CDR(obj)) ||
			      (CDR(CDR(obj)) != NIL &&
			       (!SYMBOL_P(CAR(CDR(CDR(obj)))) ||
				CDR(CDR(CDR(obj))) != NIL))))
				/* only accept svar for &OPTIONAL */
			LispDestroy(mac,
				    "%s %s: bad argument specification %s",
				    fnames[type], name, LispStrObj(mac, obj));
		}
		else
		    LispDestroy(mac, "%s %s: bad argument specification %s",
				fnames[type], name, LispStrObj(mac, obj));
	    }
	    else if (!SYMBOL_P(obj))
		LispDestroy(mac, "%s %s: %s cannot be a %s argument",
			    fnames[type], name, STROBJ(obj), types[type]);
	    else if (STRPTR(obj)[0] == '&') {
		if (obj->data.atom == mac->rest_atom) {
		    if (rest || aux ||
			CDR(args) == NIL || !SYMBOL_P(CAR(CDR(args))) ||
			STRPTR(CAR(CDR(args)))[0] == '&')
			LispDestroy(mac, "%s %s: syntax error parsing %s",
				    fnames[type], name, STRPTR(obj));
		    if (key)
			LispDestroy(mac, "%s %s: %s not allowed after %s",
				    fnames[type], name, keys[IREST], keys[IKEY]);
		    rest = 1;
		}
		else if (obj->data.atom == mac->key_atom) {
		    if (rest || aux)
			LispDestroy(mac, "%s %s: %s not allowed after %s",
				    fnames[type], name, STRPTR(obj),
				    rest ? keys[IREST] : keys[IAUX]);
		    key = 1;
		}
		else if (obj->data.atom == mac->optional_atom) {
		    if (rest || optional || aux)
			LispDestroy(mac, "%s %s: %s not allowed after %s",
				    fnames[type], name,
				    STRPTR(obj),
				    rest ? keys[IREST] : optional ?
				    keys[IOPTIONAL] : keys[IAUX]);
		    optional = 1;
		}
		else if (obj->data.atom == mac->aux_atom) {
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

    mac->protect.length = length;
    mac->eof = 0;
    COD = code;			/* LispRead protect data in COD */
}

void
LispAllocSeg(LispMac *mac, int cellcount)
{
    unsigned i;
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
	for (i = 0; i < segsize - 1; i++, obj++) {
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

void
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
	case LispKeyword_t:
	    obj->data.quote->mark = LispTrue_t;
	    break;
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
	    obj = obj->data.quote;
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
	    if (obj && !CONS_P(obj))
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

void
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
	case LispKeyword_t:
	    obj->data.quote->prot = LispTrue_t;
	    break;
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
	    obj = obj->data.quote;
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
	    if (obj && !CONS_P(obj))
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

void
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
	case LispKeyword_t:
	    obj->data.quote->prot = LispNil_t;
	    break;
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
	    obj = obj->data.quote;
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
	    if (obj && !CONS_P(obj))
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

    LispDestroy(mac, "no match for (%s %s), at UPROTECT",
		LispStrObj(mac, key), LispStrObj(mac, list));
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
    LispAtom *atom = LispDoGetAtom(mac, str, 0, 0);

    if (atom->object)
	return (atom->object);

    if (!atom->prot) {
	atom->prot = LispTrue_t;
	object = LispNew(mac, NIL, NIL);
	atom->prot = LispNil_t;
    }
    else
	object = LispNew(mac, NIL, NIL);
    object->type = LispAtom_t;
    object->data.atom = atom;
    atom->object = object;

    return (object);
}

LispObj *
LispNewSymbol(LispMac *mac, LispAtom *atom)
{
    if (atom->object)
	return (atom->object);
    else {
	LispObj *symbol;

	if (!atom->prot) {
	    atom->prot = LispTrue_t;
	    symbol = LispNew(mac, NIL, NIL);
	    atom->prot = LispNil_t;
	}
	else
	    symbol = LispNew(mac, NIL, NIL);
 
	symbol->type = LispAtom_t;
	symbol->data.atom = atom;
	atom->object = symbol;

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

    string->type = LispString_t;
    string->data.atom = LispDoGetAtom(mac, str, 0, 0);

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

LispObj *
LispNewKeyword(LispMac *mac, LispObj *obj)
{
    LispObj *key = LispNew(mac, obj, NIL);

    key->type = LispKeyword_t;
    key->data.quote = obj;

    return (key);
}

LispObj *
LispNewPathname(LispMac *mac, LispObj *obj)
{
    LispObj *path = LispNew(mac, obj, NIL);

    path->type = LispPathname_t;
    path->data.quote = obj;

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
	     entry = first + mac->env.length - 1;
	     entry > first; entry -= 2)
	    if (*entry == atom)
		return (*(entry - 1));
    }

    /* atom->property is only set for global variables */
    return (name->property.value);
}

void *
LispGetVarAddr(LispMac *mac, LispObj *atom)
{
    int i;
    LispAtom *name;

    for (i = mac->env.head - 1; i > mac->env.lex; i -= 2)
	if (mac->env.pairs[i] == atom)
	    return (&(mac->env.pairs[i - 1]));

    name = atom->data.atom;

    if (name->dyn) {
	for (i = mac->dyn.length - 1; i > 0; i -= 2)
	    if (mac->dyn.pairs[i] == atom)
		return (&(mac->dyn.pairs[i - 1]));
    }

    return (name->property.value ? &(name->property.value) : NULL);
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
	for (i = mac->glb.length - 1; i > 0; i--)
	    if (mac->glb.pairs[i] == atom) {
		LispRemAtomObjectProperty(mac, name);
		--mac->glb.length;
		if (i < mac->glb.length)
		    memmove(mac->glb.pairs + i, mac->glb.pairs + i + 1,
			    sizeof(LispObj*) * (mac->glb.length - i));
		return;
	    }
    }
    else {
	for (i = mac->spc.length - 1; i > 0; i--)
	    if (mac->spc.pairs[i] == atom) {
		LispRemAtomObjectProperty(mac, name);
		--mac->spc.length;
		if (i < mac->spc.length)
		    memmove(mac->spc.pairs + i, mac->spc.pairs + i + 1,
			    sizeof(LispObj*) * (mac->spc.length - i));
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

	return (name->property.value = obj);
    }

    if (name->property.object)
	return (name->property.value = obj);

    LispSetAtomObjectProperty(mac, name, obj);

    if (mac->glb.length + 1 >= mac->glb.space)
	LispMoreGlobals(mac);

    mac->glb.pairs[mac->glb.length++] = atom;

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
	for (i = mac->glb.length - 1; i > 0; i--)
	    if (mac->glb.pairs[i] == atom) {
		glb = 1;
		break;
	    }
    }

    if (dyn) {
	if (name->property.value == UNBOUND && value)
	    /* if variable was just made special, but not bounded */
	    LispSetAtomObjectProperty(mac, name, value);
    }
    else if (glb) {
	/* move variable from GLB to SPC */
	--mac->glb.length;
	if (i < mac->glb.length)
	    memmove(mac->glb.pairs + i, mac->glb.pairs + i + 1,
		    sizeof(LispObj*) * (mac->glb.length - i));

	if (mac->spc.length + 1 >= mac->spc.space)
	    LispMoreSpecials(mac);

	mac->spc.pairs[mac->spc.length] = atom;
	++mac->spc.length;
	/* set hint about dynamically binded variable */
	name->dyn = 1;
    }
    else {
	/* create new dynamic variable */
	LispSetAtomObjectProperty(mac, name, value ? value : UNBOUND);

	if (mac->spc.length + 1 >= mac->spc.space)
	    LispMoreSpecials(mac);

	mac->spc.pairs[mac->spc.length] = atom;
	++mac->spc.length;
	/* set hint about dynamically binded variable */
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
    LispAtom *atom;

    if (!SYMBOL_P(symbol))
	LispDestroy(mac, "ADD-DOCUMENTATION: %s is not a symbol",
		    STROBJ(symbol));
    if (!STRING_P(documentation))
	LispDestroy(mac, "ADD-DOCUMENTATION: %s is not a string",
		    STROBJ(documentation));

    atom = symbol->data.atom;
    for (env = DOC; env != NIL; env = CDR(env)) {
	if (CAAR(env)->data.atom == atom) {
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
    LispAtom *atom;

    if (!SYMBOL_P(symbol))
	LispDestroy(mac, "REMOVE-DOCUMENTATION: %s is not a symbol",
		    STROBJ(symbol));

    atom = symbol->data.atom;
    for (prev = env = DOC; env != NIL; prev = env, env = CDR(env)) {
	if (CAAR(env)->data.atom == atom) {
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
    LispAtom *atom;

    if (!SYMBOL_P(symbol))
	LispDestroy(mac, "GET-DOCUMENTATION: %s is not a symbol",
		    STROBJ(symbol));

    atom = symbol->data.atom;
    for (env = DOC; env != NIL; env = CDR(env)) {
	if (CAAR(env)->data.atom == atom) {
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

LispObj *
LispEvalBackquote(LispMac *mac, LispObj *arg)
{
    LispObj *res = NIL;

    if (arg->type == LispComma_t) {
	if (arg->data.comma.atlist)
	    LispDestroy(mac, ",@ only allowed on lists");
	else if (arg->data.comma.eval->type == LispComma_t)
	    res = arg->data.comma.eval;
	else {
	    /* just evaluate it */
	    if (NCONSTANT_P(res = arg->data.comma.eval))
		res = EVAL(res);
	}
    }
    else if (CONS_P(arg)) {
	int length = mac->protect.length;
	LispObj *obj, *cdr = NIL, *ptr;
	/* create new form, evaluating any commas inside */

	res = NIL;
	for (ptr = arg; ; ptr = CDR(ptr)) {
	    int atcons = 1, atlist = 0;

	    if (!CONS_P(ptr)) {
		atcons = 0;
		obj = ptr;
	    }
	    else
		obj = CAR(ptr);
	    if (obj->type == LispComma_t) {
		atlist = obj->data.comma.atlist;
		if (obj->data.comma.eval->type == LispComma_t) {
		    if (atlist)
			LispDestroy(mac, ",@ only allowed on lists");
		    obj = obj->data.comma.eval;
		}
		else {
		    obj = obj->data.comma.eval;
		    if (NCONSTANT_P(obj))
			obj = EVAL(obj);
		}
	    }
	    else if (obj->type == LispBackquote_t)
		obj = LispEvalBackquote(mac, obj->data.quote);
	    else if (CONS_P(obj))
		obj = LispEvalBackquote(mac, obj);
	    /* else do nothing */

	    if (res == NIL) {
		GCProtect();		/* XXX disable GC */
		if (length + 1 >= mac->protect.space)
		    LispMoreProtects(mac);
		if (!atlist) {
		    if (atcons)
			/* easier case */
			res = cdr = CONS(obj, NIL);
		    else
			res = cdr = obj;
		}
		else {
		    /* add list contents */
		    if (!CONS_P(obj))
			res = cdr = obj;
		    else {
			res = cdr = CONS(CAR(obj), NIL);
			for (obj = CDR(obj); CONS_P(obj); obj = CDR(obj)) {
			    CDR(cdr) = CONS(CAR(obj), NIL);
			    cdr = CDR(cdr);
			}
			if (obj != NIL) {
			    CDR(cdr) = obj;
			    cdr = CDR(cdr);
			}
		    }
		}
		mac->protect.objects[mac->protect.length++] = res;
		GCUProtect();		/* enable GC */
	    }
	    else {
		if (!CONS_P(cdr))
		    LispDestroy(mac, "cannot append to %s",
				LispStrObj(mac, cdr));
		if (!atlist) {
		    if (atcons) {
			CDR(cdr) = CONS(obj, NIL);
			cdr = CDR(cdr);
		    }
		    else {
			CDR(cdr) = obj;
			cdr = obj;
		    }
		}
		else {
		    if (!CONS_P(obj)) {
			CDR(cdr) = obj;
			if (obj != NIL)
			    cdr = obj;
		    }
		    else {
			for (; CONS_P(obj); obj = CDR(obj)) {
			    CDR(cdr) = CONS(CAR(obj), NIL);
			    cdr = CDR(cdr);
			}
			if (obj != NIL) {
			    CDR(cdr) = obj;
			    cdr = CDR(cdr);
			}
		    }
		}
	    }
	    if (!CONS_P(ptr))
		break;
	}
	/* XXX res is not GC protected anymore */
	mac->protect.length = length;
    }
    else if (arg->type == LispBackquote_t)
	res = BACKQUOTE(LispEvalBackquote(mac, arg->data.quote));
    else if (arg->type == LispQuote_t)
	/* also check quoted objects, to make
	 * `',sym the same as `(quote ,sym)
	 */
	res = QUOTE(LispEvalBackquote(mac, arg->data.quote));
    else
	/* 'obj == `obj */
	res = arg;

    return (res);
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
    LispObj **pairs = realloc(mac->glb.pairs,
			      (mac->glb.space + 256) * sizeof(LispObj*));

    if (pairs == NULL)
	LispDestroy(mac, "out of memory");

    mac->glb.pairs = pairs;
    mac->glb.space += 256;
}

static void
LispMoreSpecials(LispMac *mac)
{
    LispObj **pairs = realloc(mac->spc.pairs,
			      (mac->spc.space + 256) * sizeof(LispObj*));

    if (pairs == NULL)
	LispDestroy(mac, "out of memory");

    mac->spc.pairs = pairs;
    mac->spc.space += 256;
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

    LispAtom *atom;
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
	    atom = spec->data.atom;
	    if (atom->string[0] == '&') {
		if (atom == mac->rest_atom) {
		    restp = CDR(list);
		    rest = 1;
		    /* this assumes no errors in arguments specification */
		    if (CDR(restp) != NIL) {
			auxp = CDR(CDR(restp));
			aux = 1;
		    }
		    break;
		}
		else if (atom == mac->optional_atom) {
		    optional = 1;
		    continue;
		}
		else if (atom == mac->key_atom) {
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
		else if (atom == mac->aux_atom) {
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
		atom = keyword->data.atom;
		for (; CONS_P(keylist); keylist = CDR(keylist)) {
		    compar = CAR(keylist);

		    if (compar->type == LispQuote_t) {
			compar = compar->data.quote;
			if (SYMBOL_P(compar) && compar->data.atom == atom) {
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
		atom = keyword->data.atom;
		for (; CONS_P(keylist); keylist = CDR(keylist)) {
		    compar = CAR(keylist);

		    if (KEYWORD_P(compar)) {
			if (compar->data.quote->data.atom == atom) {
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

		if (aux && SYMBOL_P(keyword) && STRPTR(keyword)[0] == '&')
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
			if (arg->data.atom == keyword->data.atom) {
			    found = 1;
			    break;
			}
		    }
		}
		else {
		    if (KEYWORD_P(arg)) {
			arg = arg->data.quote;
			if (arg->data.atom == keyword->data.atom) {
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
	    if ((res = LispDoGetVar(mac, obj)) == NULL)
		LispDestroy(mac, "EVAL: the variable %s is unbound",
			    STRPTR(obj));
	    return (res);
	case LispQuote_t:
	    return (obj->data.quote);
	case LispBackquote_t:
	    return LispEvalBackquote(mac, obj->data.quote);
	case LispComma_t:
	    LispDestroy(mac, "EVAL: illegal comma outside of backquote");
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
	    strname = "NIL";
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
		CAR(name)->data.atom == mac->lambda_atom) {
		fun = EVAL(name);
		if (fun->type == LispLambda_t) {
		    LispObj *cod;

		    cod = COD;
		    COD = CONS(fun, COD);
		    strname = "NIL";
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
	    LispDestroy(mac, "EVAL: %s is invalid as a function",
			LispStrObj(mac, name));
	    /*NOTREACHED*/
    }

    if (debug)
	LispDebugger(mac, LispDebugCallBegin, name, cons);

    if (atom->property.builtin) {
	int head = mac->env.head,
	    lex = mac->env.lex,
	    length = mac->env.length,
	    dyn = mac->dyn.length;
	LispBuiltin *fn = atom->property.fun.builtin;

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

    if (atom->property.function)
	fun = atom->property.fun.function;

    else if (atom->property.defstruct && atom->property.structure.function != STRUCT_NAME) {
	int head = mac->env.head,
	    lex = mac->env.lex,
	    length = mac->env.length,
	    dyn = mac->dyn.length;
	LispBuiltin *function;

	/* Expands call to xedit::struct-* functions */

	if (atom->property.structure.function == STRUCT_CONSTRUCTOR)
	    atom = mac->make_struct_atom;
	else if (atom->property.structure.function == STRUCT_CHECK)
	    atom = mac->struct_type_atom;
	else
	    atom = mac->struct_access_atom;

	function = atom->property.fun.builtin;

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
	strname = "NIL";
	if (debug)
	    LispDebugger(mac, LispDebugCallBegin, NIL, arguments);
	goto apply_lambda;
    }

    if (atom->property.builtin) {
	int head = mac->env.head,
	    lex = mac->env.lex,
	    length = mac->env.length,
	    dyn = mac->dyn.length;
	LispBuiltin *fn = atom->property.fun.builtin;

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

    if (atom->property.function)
	function = atom->property.fun.function;

    else if (atom->property.defstruct && atom->property.structure.function != STRUCT_NAME) {
	int head = mac->env.head,
	    lex = mac->env.lex,
	    length = mac->env.length,
	    dyn = mac->dyn.length;
	LispBuiltin *fn;

	/* Expands call to xedit::struct-* functions */

	if (atom->property.structure.function == STRUCT_CONSTRUCTOR)
	    atom = mac->make_struct_atom;
	else if (atom->property.structure.function == STRUCT_CHECK)
	    atom = mac->struct_type_atom;
	else
	    atom = mac->struct_access_atom;

	fn = atom->property.fun.builtin;

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
    char *fname = "BEGIN";
    LispMac *mac = malloc(sizeof(LispMac));
    LispObj *obj;

    if (mac == NULL)
	return (NULL);

    pagesize = LispGetPageSize();
    segsize = pagesize / sizeof(LispObj);

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

    /* initialize stacks */
    LispMoreEnvironment(mac);
    LispMoreGlobals(mac);
    LispMoreSpecials(mac);
    LispMoreDynamics(mac);

    /* atoms predefined for faster lookup */
    mac->nil_atom = LispDoGetAtom(mac, "NIL", 1, 1);
    mac->t_atom = LispDoGetAtom(mac, "T", 1, 1);

    mac->atom_atom = LispDoGetAtom(mac, "ATOM", 1, 1);
    mac->symbol_atom = LispDoGetAtom(mac, "SYMBOL", 1, 1);
    mac->real_atom = LispDoGetAtom(mac, "REAL", 1, 1);
    mac->integer_atom = LispDoGetAtom(mac, "INTEGER", 1, 1);
    mac->cons_atom = LispDoGetAtom(mac, "CONS", 1, 1);
    mac->list_atom = LispDoGetAtom(mac, "LIST", 1, 1);
    mac->string_atom = LispDoGetAtom(mac, "STRING", 1, 1);
    mac->character_atom = LispDoGetAtom(mac, "CHARACTER", 1, 1);
    mac->vector_atom = LispDoGetAtom(mac, "VECTOR", 1, 1);
    mac->array_atom = LispDoGetAtom(mac, "ARRAY", 1, 1);
    mac->struct_atom = LispDoGetAtom(mac, "STRUCT", 1, 1);
    mac->keyword_atom = LispDoGetAtom(mac, "KEYWORD", 1, 1);
    mac->function_atom = LispDoGetAtom(mac, "FUNCTION", 1, 1);
    mac->pathname_atom = LispDoGetAtom(mac, "PATHNAME", 1, 1);
    mac->opaque_atom = LispDoGetAtom(mac, "OPAQUE", 1, 1);
    mac->rational_atom = LispDoGetAtom(mac, "RATIONAL", 1, 1);
    mac->float_atom = LispDoGetAtom(mac, "FLOAT", 1, 1);
    mac->complex_atom = LispDoGetAtom(mac, "COMPLEX", 1, 1);

    mac->variable_atom = LispDoGetAtom(mac, "VARIABLE", 1, 1);
    mac->structure_atom = LispDoGetAtom(mac, "STRUCTURE", 1, 1);
    mac->type_atom = LispDoGetAtom(mac, "TYPE", 1, 1);

    mac->key_atom = LispDoGetAtom(mac, "&KEY", 1, 1);
    mac->optional_atom = LispDoGetAtom(mac, "&OPTIONAL", 1, 1);
    mac->rest_atom = LispDoGetAtom(mac, "&REST", 1, 1);
    mac->aux_atom = LispDoGetAtom(mac, "&AUX", 1, 1);

    mac->close_atom = LispDoGetAtom(mac, "CLOSE", 1, 1);
    mac->format_atom = LispDoGetAtom(mac, "FORMAT", 1, 1);
    mac->lambda_atom = LispDoGetAtom(mac, "LAMBDA", 1, 1);
    mac->parse_namestring_atom = LispDoGetAtom(mac, "PARSE-NAMESTRING", 1, 1);
    mac->open_atom = LispDoGetAtom(mac, "OPEN", 1, 1);
    mac->setf_atom = LispDoGetAtom(mac, "SETF", 1, 1);

    mac->otherwise_atom = LispDoGetAtom(mac, "OTHERWISE", 1, 1);

    mac->make_struct_atom = LispDoGetAtom(mac, "XEDIT::MAKE-STRUCT", 1, 1);
    mac->struct_access_atom = LispDoGetAtom(mac, "XEDIT::STRUCT-ACCESS", 1, 1);
    mac->struct_store_atom = LispDoGetAtom(mac, "XEDIT::STRUCT-STORE", 1, 1);
    mac->struct_type_atom = LispDoGetAtom(mac, "XEDIT::STRUCT-TYPE", 1, 1);

    mac->error_atom = LispDoGetAtom(mac, "ERROR", 1, 1);
    mac->if_does_not_exist_atom = LispDoGetAtom(mac, "IF-DOES-NOT-EXIST", 1, 1);

    mac->absolute_atom = LispDoGetAtom(mac, "ABSOLUTE", 1, 1);
    mac->relative_atom = LispDoGetAtom(mac, "RELATIVE", 1, 1);
    mac->unspecific_atom = LispDoGetAtom(mac, "UNSPECIFIC", 1, 1);

    mac->probe_atom = LispDoGetAtom(mac, "PROBE", 1, 1);
    mac->input_atom = LispDoGetAtom(mac, "INPUT", 1, 1);
    mac->output_atom = LispDoGetAtom(mac, "OUTPUT", 1, 1);
    mac->io_atom = LispDoGetAtom(mac, "IO", 1, 1);
    mac->new_version_atom = LispDoGetAtom(mac, "NEW-VERSION", 1, 1);
    mac->rename_atom = LispDoGetAtom(mac, "RENAME", 1, 1);
    mac->rename_and_delete_atom = LispDoGetAtom(mac, "RENAME-AND-DELETE", 1, 1);
    mac->overwrite_atom = LispDoGetAtom(mac, "OVERWRITE", 1, 1);
    mac->append_atom = LispDoGetAtom(mac, "APPEND", 1, 1);
    mac->supersede_atom = LispDoGetAtom(mac, "SUPERSEDE", 1, 1);
    mac->create_atom = LispDoGetAtom(mac, "CREATE", 1, 1);

    mac->default_atom = LispDoGetAtom(mac, "DEFAULT", 1, 1);

    mac->make_array_atom = LispDoGetAtom(mac, "MAKE-ARRAY", 1, 1);
    mac->initial_contents_atom = LispDoGetAtom(mac, "INITIAL-CONTENTS", 1, 1);

    /* initial cells */
    LispAllocSeg(mac, minfree);
    mac->gc.average = segsize;

    mac->unget = malloc(sizeof(LispUngetInfo*));
    mac->unget[0] = calloc(1, sizeof(LispUngetInfo));
    mac->nunget = 1;

    obj = ATOM("*STANDARD-INPUT*");
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
	    SINPUT = STANDARDSTREAM(input, obj, STREAM_READ);
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
	SINPUT = STANDARDSTREAM(Stdin, obj, STREAM_READ);
	mac->interactive = 1;
    }
    LispProclaimSpecial(mac, obj, mac->input = SINPUT, NIL);

    obj = ATOM("*STANDARD-OUTPUT*");
    SOUTPUT = STANDARDSTREAM(Stdout, obj, STREAM_WRITE);
    LispProclaimSpecial(mac, obj, mac->output = SOUTPUT, NIL);

    obj = ATOM("*STANDARD-ERROR*");
    mac->error_stream = STANDARDSTREAM(Stderr, obj, STREAM_WRITE);
    LispProclaimSpecial(mac, obj, mac->error_stream, NIL);

    mac->modules = ATOM("*MODULES*");
    LispProclaimSpecial(mac, mac->modules, MOD, NIL);

    FEAT = obj = CONS(KEYWORD(ATOM("UNIX")), NIL);
    CDR(obj) = CONS(KEYWORD(ATOM("XEDIT")), NIL);
    /* add more features... obj = CDR(obj); CDR(obj) = ... */
    mac->features = ATOM("*FEATURES*");
    LispProclaimSpecial(mac, mac->features, FEAT, NIL);

    LispMathInit(mac);
    LispPathnameInit(mac);

    /* initialize memory management */
    mac->mem.mem = (void**)calloc(mac->mem.mem_size = 16, sizeof(void*));
    mac->mem.mem_level = 0;

    mac->prompt = "> ";

    mac->errexit = !mac->interactive;

    if (mac->interactive) {
	/* add +, ++, +++, *, **, and *** */
	for (i = 0; i < 3; i++) {
	    results[i] = '+';
	    results[i + 1] = '\0';
	    RUN[i] = ATOM2(results);
	    LispSet(mac, RUN[i], NIL, fname, 0);
	}
	for (i = 0; i < 3; i++) {
	    results[i] = '*';
	    results[i + 1] = '\0';
	    RES[i] = ATOM2(results);
	    LispSet(mac, RES[i], NIL, fname, 0);
	}
    }
    else
	RUN[0] = RUN[1] = RUN[2] = RES[0] = RES[1] = RES[2] = NIL;

    for (i = 0; i < sizeof(lispbuiltins) / sizeof(lispbuiltins[0]); i++)
	LispAddBuiltinFunction(mac, &lispbuiltins[i]);

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

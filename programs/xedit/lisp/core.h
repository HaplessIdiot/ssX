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

/* $XFree86: xc/programs/xedit/lisp/core.h,v 1.23 2002/09/22 07:09:06 paulo Exp $ */

#ifndef Lisp_core_h
#define Lisp_core_h

#include "internal.h"

void LispCoreInit(LispMac*);

LispObj *Lisp_Acons(LispMac*, LispBuiltin*);		/* acons */
LispObj *Lisp_Adjoin(LispMac*, LispBuiltin*);		/* adjoin */
LispObj *Lisp_Append(LispMac*, LispBuiltin*);		/* append */
LispObj *Lisp_And(LispMac*, LispBuiltin*);		/* and */
LispObj *Lisp_Aref(LispMac*, LispBuiltin*);		/* aref */
LispObj *Lisp_Assoc(LispMac*, LispBuiltin*);		/* assoc */
LispObj *Lisp_AssocIf(LispMac*, LispBuiltin*);		/* assoc-if */
LispObj *Lisp_AssocIfNot(LispMac*, LispBuiltin*);	/* assoc-if-not */
LispObj *Lisp_Apply(LispMac*, LispBuiltin*);		/* apply */
LispObj *Lisp_Atom(LispMac*, LispBuiltin*);		/* atom */
LispObj *Lisp_Block(LispMac*, LispBuiltin*);		/* block */
LispObj *Lisp_Boundp(LispMac*, LispBuiltin*);		/* boundp */
LispObj *Lisp_Butlast(LispMac*, LispBuiltin*);		/* butlast */
LispObj *Lisp_Car(LispMac*, LispBuiltin*);		/* car */
LispObj *Lisp_Case(LispMac*, LispBuiltin*);		/* case */
LispObj *Lisp_Catch(LispMac*, LispBuiltin*);		/* catch */
LispObj *Lisp_Cdr(LispMac*, LispBuiltin*);		/* cdr */
LispObj *Lisp_C_r(LispMac*, LispBuiltin*);		/* c[ad]{2,4}r */
LispObj *Lisp_Coerce(LispMac*, LispBuiltin*);		/* coerce */
LispObj *Lisp_Cond(LispMac*, LispBuiltin*);		/* cond */
LispObj *Lisp_Cons(LispMac*, LispBuiltin*);		/* cons */
LispObj *Lisp_Consp(LispMac*, LispBuiltin*);		/* consp */
LispObj *Lisp_CopyList(LispMac*, LispBuiltin*);		/* copy-list */
LispObj *Lisp_Defconstant(LispMac*, LispBuiltin*);	/* defconstant */
LispObj *Lisp_Defmacro(LispMac*, LispBuiltin*);		/* defmacro */
LispObj *Lisp_Defun(LispMac*, LispBuiltin*);		/* defun */
LispObj *Lisp_Defsetf(LispMac*, LispBuiltin*);		/* defsetf */
LispObj *Lisp_Defvar(LispMac*, LispBuiltin*);		/* defvar */
LispObj *Lisp_Delete(LispMac*, LispBuiltin*);		/* delete */
LispObj *Lisp_DeleteDuplicates(LispMac*, LispBuiltin*);	/* delete-duplicates */
LispObj *Lisp_DeleteIf(LispMac*, LispBuiltin*);		/* delete-if */
LispObj *Lisp_DeleteIfNot(LispMac*, LispBuiltin*);	/* delete-if-not */
LispObj *Lisp_Do(LispMac*, LispBuiltin*);		/* do */
LispObj *Lisp_DoP(LispMac*, LispBuiltin*);		/* do* */
LispObj *Lisp_Documentation(LispMac*, LispBuiltin*);	/* documentation */
LispObj *Lisp_DoList(LispMac*, LispBuiltin*);		/* dolist */
LispObj *Lisp_DoTimes(LispMac*, LispBuiltin*);		/* dotimes */
LispObj *Lisp_Elt(LispMac*, LispBuiltin*);		/* elt */
LispObj *Lisp_Endp(LispMac*, LispBuiltin*);		/* endp */
LispObj *Lisp_Eq(LispMac*, LispBuiltin*);		/* eq */
LispObj *Lisp_Eql(LispMac*, LispBuiltin*);		/* eql */
LispObj *Lisp_Equal(LispMac*, LispBuiltin*);		/* equal */
LispObj *Lisp_Equalp(LispMac*, LispBuiltin*);		/* equalp */
LispObj *Lisp_Error(LispMac*, LispBuiltin*);		/* error */
LispObj *Lisp_Eval(LispMac*, LispBuiltin*);		/* eval */
LispObj *Lisp_Fboundp(LispMac*, LispBuiltin*);		/* fboundp */
LispObj *Lisp_Fill(LispMac*, LispBuiltin*);		/* fill */
LispObj *Lisp_Fmakunbound(LispMac*, LispBuiltin*);	/* fmakunbound */
LispObj *Lisp_Funcall(LispMac*, LispBuiltin*);		/* funcall */
LispObj *Lisp_Gc(LispMac*, LispBuiltin*);		/* gc */
LispObj *Lisp_Gensym(LispMac*, LispBuiltin*);		/* gensym */
LispObj *Lisp_Get(LispMac*, LispBuiltin*);		/* get */
LispObj *Lisp_Getenv(LispMac*, LispBuiltin*);		/* getenv */
LispObj *Lisp_Go(LispMac*, LispBuiltin*);		/* go */
LispObj *Lisp_If(LispMac*, LispBuiltin*);		/* if */
LispObj *Lisp_Intersection(LispMac*, LispBuiltin*);	/* intersection */
LispObj *Lisp_Keywordp(LispMac*, LispBuiltin*);		/* keywordp */
LispObj *Lisp_Lambda(LispMac*, LispBuiltin*);		/* lambda */
LispObj *Lisp_Last(LispMac*, LispBuiltin*);		/* last */
LispObj *Lisp_Let(LispMac*, LispBuiltin*);		/* let */
LispObj *Lisp_Length(LispMac*, LispBuiltin*);		/* length */
LispObj *Lisp_LetP(LispMac*, LispBuiltin*);		/* let* */
LispObj *Lisp_List(LispMac*, LispBuiltin*);		/* list */
LispObj *Lisp_ListP(LispMac*, LispBuiltin*);		/* list* */
LispObj *Lisp_Listp(LispMac*, LispBuiltin*);		/* listp */
LispObj *Lisp_Loop(LispMac*, LispBuiltin*);		/* loop */
LispObj *Lisp_MakeArray(LispMac*, LispBuiltin*);	/* make-array */
LispObj *Lisp_MakeList(LispMac*, LispBuiltin*);		/* make-list */
LispObj *Lisp_Makunbound(LispMac*, LispBuiltin*);	/* makunbound */
LispObj *Lisp_Mapcar(LispMac*, LispBuiltin*);		/* mapcar */
LispObj *Lisp_Maplist(LispMac*, LispBuiltin*);		/* maplist */
LispObj *Lisp_Member(LispMac*, LispBuiltin*);		/* member */
LispObj *Lisp_MemberIf(LispMac*, LispBuiltin*);		/* member-if */
LispObj *Lisp_MemberIfNot(LispMac*, LispBuiltin*);	/* member-if-not */
LispObj *Lisp_MultipleValueBind(LispMac*, LispBuiltin*);/* multiple-value-bind */
LispObj *Lisp_MultipleValueList(LispMac*, LispBuiltin*);/* multiple-value-list */
LispObj *Lisp_MultipleValueSetq(LispMac*, LispBuiltin*);/* multiple-value-setq */
LispObj *Lisp_Nconc(LispMac*, LispBuiltin*);		/* nconc */
LispObj *Lisp_Nreverse(LispMac*, LispBuiltin*);		/* nreverse */
LispObj *Lisp_NsetDifference(LispMac*, LispBuiltin*);	/* nset-difference */
LispObj *Lisp_Nsubstitute(LispMac*, LispBuiltin*);	/* nsubstitute */
LispObj *Lisp_NsubstituteIf(LispMac*, LispBuiltin*);	/* nsubstitute-if */
LispObj *Lisp_NsubstituteIfNot(LispMac*, LispBuiltin*);	/* nsubstitute-if-not */
LispObj *Lisp_Nth(LispMac*, LispBuiltin*);		/* nth */
LispObj *Lisp_Nthcdr(LispMac*, LispBuiltin*);		/* nthcdr */
LispObj *Lisp_Null(LispMac*, LispBuiltin*);		/* null */
LispObj *Lisp_Or(LispMac*, LispBuiltin*);		/* or */
LispObj *Lisp_Pop(LispMac*, LispBuiltin*);		/* pop */
LispObj *Lisp_Position(LispMac*, LispBuiltin*);		/* position */
LispObj *Lisp_PositionIf(LispMac*, LispBuiltin*);	/* position-if */
LispObj *Lisp_PositionIfNot(LispMac*, LispBuiltin*);	/* position-if-not */
LispObj *Lisp_Prin1(LispMac*, LispBuiltin*);		/* prin1 */
LispObj *Lisp_Princ(LispMac*, LispBuiltin*);		/* princ */
LispObj *Lisp_Print(LispMac*, LispBuiltin*);		/* print */
LispObj *Lisp_Proclaim(LispMac*, LispBuiltin*);		/* proclaim */
LispObj *Lisp_Prog1(LispMac*, LispBuiltin*);		/* prog1 */
LispObj *Lisp_Prog2(LispMac*, LispBuiltin*);		/* prog2 */
LispObj *Lisp_Progn(LispMac*, LispBuiltin*);		/* progn */
LispObj *Lisp_Progv(LispMac*, LispBuiltin*);		/* progv */
LispObj *Lisp_Provide(LispMac*, LispBuiltin*);		/* provide */
LispObj *Lisp_Push(LispMac*, LispBuiltin*);		/* push */
LispObj *Lisp_Pushnew(LispMac*, LispBuiltin*);		/* pushnew */
LispObj *Lisp_Quit(LispMac*, LispBuiltin*);		/* quit */
LispObj *Lisp_Quote(LispMac*, LispBuiltin*);		/* quote */
LispObj *Lisp_Remove(LispMac*, LispBuiltin*);		/* remove */
LispObj *Lisp_RemoveDuplicates(LispMac*, LispBuiltin*);	/* remove-duplicates */
LispObj *Lisp_RemoveIf(LispMac*, LispBuiltin*);		/* remove-if */
LispObj *Lisp_RemoveIfNot(LispMac*, LispBuiltin*);	/* remove-if-not */
LispObj *Lisp_Replace(LispMac*, LispBuiltin*);		/* replace */
LispObj *Lisp_Return(LispMac*, LispBuiltin*);		/* return */
LispObj *Lisp_ReturnFrom(LispMac*, LispBuiltin*);	/* return-from */
LispObj *Lisp_Reverse(LispMac*, LispBuiltin*);		/* reverse */
LispObj *Lisp_Rplaca(LispMac*, LispBuiltin*);		/* rplaca */
LispObj *Lisp_Rplacd(LispMac*, LispBuiltin*);		/* rplacd */
LispObj *Lisp_Search(LispMac*, LispBuiltin*);		/* search */
LispObj *Lisp_Setenv(LispMac*, LispBuiltin*);		/* setenv */
LispObj *Lisp_Set(LispMac*, LispBuiltin*);		/* set */
LispObj *Lisp_SetDifference(LispMac*, LispBuiltin*);	/* set-difference */
LispObj *Lisp_SetExclusiveOr(LispMac*, LispBuiltin*);	/* set-exclusive-or */
LispObj *Lisp_Setf(LispMac*, LispBuiltin*);		/* setf */
LispObj *Lisp_SetQ(LispMac*, LispBuiltin*);		/* setq */
LispObj *Lisp_Sleep(LispMac*, LispBuiltin*);		/* sleep */
LispObj *Lisp_Sort(LispMac*, LispBuiltin*);		/* sort */
LispObj *Lisp_Subseq(LispMac*, LispBuiltin*);		/* subseq */
LispObj *Lisp_Subsetp(LispMac*, LispBuiltin*);		/* subsetp */
LispObj *Lisp_Substitute(LispMac*, LispBuiltin*);	/* substitute */
LispObj *Lisp_SubstituteIf(LispMac*, LispBuiltin*);	/* substitute-if */
LispObj *Lisp_SubstituteIfNot(LispMac*, LispBuiltin*);	/* substitute-if-not */
LispObj *Lisp_Symbolp(LispMac*, LispBuiltin*);		/* symbolp */
LispObj *Lisp_SymbolPlist(LispMac*, LispBuiltin*);	/* symbol-plist */
LispObj *Lisp_SymbolValue(LispMac*, LispBuiltin*);	/* symbol-value */
LispObj *Lisp_Tagbody(LispMac*, LispBuiltin*);		/* tagbody */
LispObj *Lisp_Terpri(LispMac*, LispBuiltin*);		/* terpri */
LispObj *Lisp_Throw(LispMac*, LispBuiltin*);		/* throw */
LispObj *Lisp_The(LispMac*, LispBuiltin*);		/* the */
LispObj *Lisp_Typep(LispMac*, LispBuiltin*);		/* typep */
LispObj *Lisp_Union(LispMac*, LispBuiltin*);		/* union */
LispObj *Lisp_Unless(LispMac*, LispBuiltin*);		/* unless */
LispObj *Lisp_Until(LispMac*, LispBuiltin*);		/* until */
LispObj *Lisp_Unsetenv(LispMac*, LispBuiltin*);		/* unsetenv */
LispObj *Lisp_UnwindProtect(LispMac*, LispBuiltin*);	/* unwind-protect */
LispObj *Lisp_Values(LispMac*, LispBuiltin*);		/* values */
LispObj *Lisp_Vector(LispMac*, LispBuiltin*);		/* vector */
LispObj *Lisp_When(LispMac*, LispBuiltin*);		/* when */
LispObj *Lisp_While(LispMac*, LispBuiltin*);		/* while */
LispObj *Lisp_XeditEltStore(LispMac*, LispBuiltin*);    /* lisp::elt-store */
LispObj *Lisp_XeditPut(LispMac*, LispBuiltin*);		/* lisp::put */
LispObj *Lisp_XeditVectorStore(LispMac*, LispBuiltin*);	/* lisp::vector-store */

#endif /* Lisp_core_h */

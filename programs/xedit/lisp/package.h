/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
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

/* $XFree86: xc/programs/xedit/lisp/package.h,v 1.1 2002/02/12 16:07:55 paulo Exp $ */

#ifndef Lisp_package_h
#define Lisp_package_h

#include "internal.h"

LispObj *LispFindPackage(LispMac*, LispObj*);
LispObj *LispFindPackageFromString(LispMac*, char*);

LispObj *Lisp_DoAllSymbols(LispMac*, LispBuiltin*);	/* do-all-symbols */
LispObj *Lisp_DoExternalSymbols(LispMac*, LispBuiltin*);/* do-external-symbols */
LispObj *Lisp_DoSymbols(LispMac*, LispBuiltin*);	/* do-symbols */
LispObj *Lisp_FindAllSymbols(LispMac*, LispBuiltin*);	/* find-all-symbols */
LispObj *Lisp_FindPackage(LispMac*, LispBuiltin*);	/* find-package */
LispObj *Lisp_InPackage(LispMac*, LispBuiltin*);	/* in-package */
LispObj *Lisp_ListAllPackages(LispMac*, LispBuiltin*);	/* list-all-packages */
LispObj *Lisp_MakePackage(LispMac*, LispBuiltin*);	/* make-package */
LispObj *Lisp_PackageName(LispMac*, LispBuiltin*);	/* package-name */
LispObj *Lisp_PackageNicknames(LispMac*, LispBuiltin*);	/* package-nicknames */
LispObj *Lisp_PackageUseList(LispMac*, LispBuiltin*);	/* package-use-list */
LispObj *Lisp_PackageUsedByList(LispMac*, LispBuiltin*);/* package-used-by-list */

#endif /* Lisp_package_h */

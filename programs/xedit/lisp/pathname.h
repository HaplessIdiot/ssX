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

/* $XFree86: xc/programs/xedit/lisp/pathname.h,v 1.2 2002/08/05 03:56:24 paulo Exp $ */

#ifndef Lisp_pathname_h
#define Lisp_pathname_h

#include "internal.h"

#define	PATH_SEP	'/'
#define	PATH_TYPESEP	'.'

#ifndef PATH_MAX
#define PATH_MAX	4096
#endif

#ifndef NAME_MAX
#define NAME_MAX	256
#endif

/* field number in list */
#define PATH_STRING	0
#define PATH_HOST	1
#define PATH_DEVICE	2
#define PATH_DIRECTORY	3
#define PATH_NAME	4
#define PATH_TYPE	5
#define PATH_VERSION	6

void LispPathnameInit(LispMac*);

LispObj *Lisp_Directory(LispMac*, LispBuiltin*);	  /* directory */
LispObj *Lisp_Namestring(LispMac*, LispBuiltin*);	  /* namestring */
LispObj *Lisp_FileNamestring(LispMac*, LispBuiltin*);     /* file-namestring */
LispObj *Lisp_DirectoryNamestring(LispMac*, LispBuiltin*);/* directory-namestring */
LispObj *Lisp_EnoughNamestring(LispMac*, LispBuiltin*);	  /* enough-namestring */
LispObj *Lisp_HostNamestring(LispMac*, LispBuiltin*);	  /* host-namestring */
LispObj *Lisp_MakePathname(LispMac*, LispBuiltin*);	  /* make-pathname */
LispObj *Lisp_Pathnamep(LispMac*, LispBuiltin*);	  /* pathnamep */
LispObj *Lisp_ParseNamestring(LispMac*, LispBuiltin*);	  /* parse-namestring */
LispObj *Lisp_PathnameHost(LispMac*, LispBuiltin*);	  /* pathname-host */
LispObj *Lisp_PathnameDevice(LispMac*, LispBuiltin*);	  /* pathname-device */
LispObj *Lisp_PathnameDirectory(LispMac*, LispBuiltin*);  /* pathname-directory */
LispObj *Lisp_PathnameName(LispMac*, LispBuiltin*);	  /* pathname-name */
LispObj *Lisp_PathnameType(LispMac*, LispBuiltin*);	  /* pathname-type */
LispObj *Lisp_PathnameVersion(LispMac*, LispBuiltin*);	  /* pathname-version */
LispObj *Lisp_Truename(LispMac*, LispBuiltin*);		  /* truename */
LispObj *Lisp_ProbeFile(LispMac*, LispBuiltin*);	  /* probe-file */
LispObj *Lisp_UserHomedirPathname(LispMac*, LispBuiltin*);/* user-homedir-pathname */

#endif /* Lisp_pathname_h */

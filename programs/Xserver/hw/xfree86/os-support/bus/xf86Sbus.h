/*
 * Platform specific SBUS and OpenPROM access declarations.
 *
 * Copyright (C) 2000 Jakub Jelinek (jakub@redhat.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * JAKUB JELINEK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* $XFree86$ */

#ifndef _XF86_SBUS_H
#define _XF86_SBUS_H

#ifdef linux
#include <asm/types.h>
#include <asm/fbio.h>
#include <asm/openpromio.h>
#elif defined(SVR4)
#include <sys/fbio.h>
#elif define(CSRG_BASED)
#include <machine/fbio.h>
#else
#include <sun/fbio.h>
#endif

extern int sparcPromInit(void);
extern void sparcPromClose(void);
extern char * sparcPromGetProperty(sbusPromNodePtr pnode, const char *prop, int *lenp);
extern int sparcPromGetBool(sbusPromNodePtr pnode, const char *prop);
extern void sparcPromAssignNodes(void);
extern char * sparcPromNode2Pathname(sbusPromNodePtr pnode);
extern int sparcPromPathname2Node(const char *pathName);

#endif /* _XF86_SBUS_H */

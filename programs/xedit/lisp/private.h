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

/* $XFree86: xc/programs/xedit/lisp/private.h,v 1.5 2001/09/21 05:08:43 paulo Exp $ */

#ifndef Lisp_private_h
#define Lisp_private_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/time.h>
#include "internal.h"

#include "core.h"
#include "helper.h"
#include "struct.h"

/*
 * Defines
 */
#define	STRTBLSZ	23

#define MOD	mac->modlist
#define GLB	mac->glblist
#define ENV	mac->envlist
#define SYM	mac->symlist
#define LEX	mac->lexlist
#define FUN	mac->funlist
#define COD	mac->codlist
#define FRM	mac->frmlist
#define STR	mac->strlist
#define RUN	mac->runlist
#define RES	mac->reslist

/*
 * Types
 */
typedef struct _LispStream LispStream;
typedef struct _LispBlock LispBlock;
typedef struct _LispString LispString;
typedef struct _LispOpaque LispOpaque;
typedef struct _LispModule LispModule;

struct _LispStream {
    FILE *fp;
    char *st;
    char *cp;
    int tok;
};

struct _LispString {
    unsigned int mark	: 1;	/* gc protected */
    unsigned int dirty	: 1;
    unsigned int prot	: 1;	/* never released */
    char *string;
    LispString *next;
};

struct _LispOpaque {
    int type;
    char *desc;
    LispOpaque *next;
};

struct _LispBlock {
    LispObj tag;
    jmp_buf jmp;
    int level;
    int block_level;
};

struct _LispModule {
    LispModule *next;
    void *handle;
    LispModuleData *data;
};

struct _LispMac {
    FILE *fp;
    char *st;
    char *cp;
    int tok;
    int level;
    LispObj *setf;	/* setf place, set on some functions, reset at eval */
    int cdr;		/* use CDR instead of CAR in setf? */
    int princ;		/* don't quote strings? */
    int justsize;	/* just calculate size of output,
			 * needed to calculate formatted output */
    int newline;	/* at a newline in the output */
    int column;		/* column number in the output */
    int interactive;
    int errexit;
    LispString *strs[STRTBLSZ];
    LispOpaque *opqs[STRTBLSZ];
    int opaque;
    jmp_buf jmp;
    struct {
	unsigned stream_level;
	unsigned stream_size;
	LispStream *stream;	
    } stream;
    struct {
	unsigned block_level;
	unsigned block_size;
	LispObj *block_ret;
	LispBlock *block;
    } block;
    struct {
	unsigned mem_level;
	unsigned mem_size;
	void **mem;
    } mem;		/* memory from Lisp*Alloc, to be release in error */
    LispModule *module;
    LispObj *struc;	/* to avoid unecessary extra lookups and */
			/* used to pass arguments to structure access functions */
    int struc_field;
    char *prompt;

    LispObj *modlist;		/* module list */
    LispObj *glblist;		/* global variables */
    LispObj *envlist;		/* alive variables */
    LispObj *symlist;		/* symbols in this form, for faster lookup */
    LispObj *lexlist;		/* lexical instead of dynamic scope */
    LispObj *funlist;		/* user functions */
    LispObj *codlist;		/* current code */
    LispObj *frmlist;		/* input data */
    LispObj *strlist;		/* structure definitions */
    LispObj *runlist[3];	/* +, ++, and +++ */
    LispObj *reslist[3];	/* *, **, and *** */

#ifdef SIGNALRETURNSINT
    int (*sigint)(int);
    int (*sigfpe)(int);
#else
    void (*sigint)(int);
    void (*sigfpe)(int);
#endif
};

/*
 * Prototypes
 */
LispObj *LispEnvRun(LispMac*, LispObj*, LispFunPtr, char*, int);
LispObj *LispGetVar(LispMac*, char*, int);
LispObj *LispAddVar(LispMac*, char*, LispObj*);
LispObj *LispSetVar(LispMac*, char*, LispObj*, int);

/* destructive fast reverse, note that don't receive a LispMac* argument */
LispObj *LispReverse(LispObj *list);

/* reads an expression from the selected stream */
LispObj *LispRun(LispMac*);

/* generated by gperf */
extern struct _LispBuiltin *LispFindBuiltin(const char*, unsigned int);

/* (print) */
void LispPrint(LispMac*, LispObj*, int);

LispBlock *LispBeginBlock(LispMac*, LispObj*, int);
void LispEndBlock(LispMac*, LispBlock*);

void LispUpdateResults(LispMac*, LispObj*, LispObj*);

#endif /* Lisp_private_h */

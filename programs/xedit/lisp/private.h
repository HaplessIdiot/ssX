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

/* $XFree86: xc/programs/xedit/lisp/private.h,v 1.23 2002/03/08 04:33:18 paulo Exp $ */

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
#include "debugger.h"
#include "helper.h"
#include "string.h"
#include "struct.h"

/*
 * Defines
 */
#define	STRTBLSZ	23

#define FEAT	mac->featlist
#define PACK	mac->packlist
#define PACKNAM	mac->package_name
#define PACKAGE	mac->package
#define MOD	mac->modlist
#define COD	mac->codlist
#define FRM	mac->frmlist
#define RUN	mac->runlist
#define RES	mac->reslist
#define DBG	mac->dbglist
#define BRK	mac->brklist
#define PRO	mac->prolist
#define DOC	mac->doclist

#define	EOLIST	(LispObj*)1	/* end-of-list ")" found in LispRun */
#define INVALID_P(obj) ((obj) == NULL || (obj) == EOLIST || (obj) == DOT)

#define SINPUT	mac->standard_input
#define SOUTPUT	mac->standard_output
#define STANDARDSTREAM(file, desc, flags)	\
	LispNewStandardStream(mac, file, desc, flags)

/*
 * Types
 */
typedef struct _LispStream LispStream;
typedef struct _LispBlock LispBlock;
typedef struct _LispOpaque LispOpaque;
typedef struct _LispModule LispModule;
typedef struct _LispProperty LispProperty;
typedef struct _LispObjList LispObjList;
typedef struct _LispStringHash LispStringHash;

/* Normal function/macro arguments */
typedef struct _LispNormalArgs {
    int num_symbols;
    LispObj **symbols;		/* symbol names */
} LispNormalArgs;

/* &optional function/macro arguments */
typedef struct _LispOptionalArgs {
    int num_symbols;
    LispObj **symbols;		/* symbol names */
    LispObj **defaults;		/* default values, when unspecifed */
    LispObj **sforms;		/* T if variable specified, NIL otherwise */
} LispOptionalArgs;

/* &key function/macro arguments */
typedef struct _LispKeyArgs {
    int num_symbols;
    LispObj **symbols;		/* symbol names */
    LispObj **defaults;		/* default values */
    LispObj **sforms;		/* T if variable specified, NIL otherwise */
    LispObj **keys;		/* key names, for special keywords */
} LispKeyArgs;

/* &aux function/macro arguments */
typedef struct _LispAuxArgs {
    int num_symbols;
    LispObj **symbols;		/* symbol names */
    LispObj **initials;		/* initial values */
} LispAuxArgs;

/* characters in the field description have the format:
 *	'.'	normals has a list of normal arguments
 *	'o'	optionals has a list of &optional arguments
 *	'k'	keys has a list of &key arguments
 *	'r'	rest is a valid pointer to a &rest symbol
 *	'a'	auxs has a list of &aux arguments
 */
typedef struct _LispArgList {
    LispNormalArgs normals;
    LispOptionalArgs optionals;
    LispKeyArgs keys;
    LispObj *rest;
    LispAuxArgs auxs;
    int num_arguments;
    char *description;
} LispArgList;

struct _LispProperty {
    /* may be used by multiple packages */
    unsigned int refcount;

    /* package where the property was created */
    LispPackage *package;

    /* value of variable attached to symbol */
    LispObj *value;

    union {
	/* function attached to symbol */
	LispObj *function;
	/* builtin function attached to symbol*/
	LispBuiltin *builtin;
    } fun;
    /* function/macro argument list description */
    LispArgList *alist;

    /* symbol properties list */
    LispObj *properties;

    /* setf method */
    LispObj *setf;
    /* setf argument list description */
    LispArgList *salist;

    /* structure information */
    struct {
	LispObj *definition;
#define STRUCT_NAME		-3
#define STRUCT_CHECK		-2
#define STRUCT_CONSTRUCTOR	-1
	int function;		/* if >= 0, it is a structure field index */
    } structure;
};

struct _LispAtom {
    /* hint: dynamically binded variable */
    unsigned int dyn : 1;

    /* Property has useful data in value field */
    unsigned int a_object : 1;
    /* Property has useful data in fun.function field */
    unsigned int a_function : 1;
    /* Property has useful data in fun.builtin field */
    unsigned int a_builtin : 1;
    /* Property has useful data in properties field */
    unsigned int a_property : 1;
    /* Property has useful data in setf field */
    unsigned int a_defsetf : 1;
    /* Property has useful data in defstruct field */
    unsigned int a_defstruct : 1;

    /* Symbol is extern */
    unsigned int ext : 1;

    /* Symbol must be quoted with '|' to be allow reading back */
    unsigned int unreadable : 1;

    /* Symbol value may need special handling when changed */
    unsigned int watch : 1;

    /* Symbol value is constant, cannot be changed */
    unsigned int constant : 1;

    char *string;
    LispObj *object;		/* backpointer to object ATOM */
    LispObj *package;		/* package home of symbol */
    LispProperty *property;
    struct _LispAtom *next;
};

struct _LispObjList {
    LispObj **pairs;		/* name0 ... nameN */
    int length;			/* number of objects */
    int space;			/* space allocated in field pairs */
};

struct _LispPackage {
    LispObjList glb;		/* global symbols in package */
    LispObjList spc;		/* global special symbols in package */
    LispObjList use;		/* inherited packages */
    LispAtom *atoms[STRTBLSZ];	/* atoms in this package */
};

struct _LispOpaque {
    int type;
    char *desc;
    LispOpaque *next;
};

/* These strings are never released, they are used to avoid
 * the need of strcmp() on two symbol names, just compare pointers */
struct _LispStringHash {
    char *string;
    LispStringHash *next;
};

typedef enum _LispBlockType {
    LispBlockTag,	/* may become "invisible" */
    LispBlockCatch,	/* can be used to jump across function calls */
    LispBlockClosure,	/* hides blocks of type LispBlockTag bellow it */
    LispBlockProtect,	/* used by unwind-protect */
    LispBlockBody	/* used by tagbody and go */
} LispBlockType;

struct _LispBlock {
    LispBlockType type;
    LispObj tag;
    jmp_buf jmp;
    int block_level;
    int debug_level;
    int debug_step;
};

struct _LispModule {
    LispModule *next;
    void *handle;
    LispModuleData *data;
};

typedef struct _LispUngetInfo {
    unsigned char buffer[16];
    int offset;
} LispUngetInfo;

struct _LispMac {
    /* environment */
    struct {
	LispObj **values;
	LispObj **names;
	int lex;		/* until where variables are visible */
	int head;		/* top of environment */
	int base;		/* base of arguments to function */
	int length;		/* number of used pairs */
	int space;		/* number of objects in pairs */
    } env;

    /* rebound special variables, dynamic variables */
    struct {
	LispObj **values;
	LispObj **names;
	int length;		/* number of dynamics * 2 */
	int space;		/* number of objects in pairs */
    } dyn;

    struct {
	LispObj **values;
	int length;
	int space;
    } returns;

    struct {
	LispObj **objects;
	int length;
	int space;
    } protect;

    LispObj *package_name;	/* atom *PACKAGE* */
    LispObj *package;		/* current package object */
    LispPackage *pack;		/* pointer to mac->package->data.package.package */

    /* fast access to the KEYWORD package */
    LispObj *keyword;
    LispPackage *key;

    /* only used if the package was changed, but an error generated
     * before returning to the toplevel */
    LispObj *savepackage;
    LispPackage *savepack;

    struct {
	unsigned block_level;
	unsigned block_size;
	LispObj *block_ret;
	LispBlock **block;
    } block;

    sigjmp_buf jmp;

    struct {
	unsigned int expandbits : 3;	/* code doesn't look like reusing cells
					 * so try to have a larger number of
					 * free cells */
	unsigned int immutablebits : 1;	/* need to reset immutable bits */
	unsigned int timebits : 1;	/* update gctime counter */
	long gctime;
	int average;			/* of cells freed after gc calls */
    } gc;

    LispStringHash *strings[STRTBLSZ];
    LispOpaque *opqs[STRTBLSZ];
    int opaque;

    LispObj *standard_input, *input;
    LispObj *standard_output, *output;
    LispObj *error_stream;
    LispUngetInfo **unget;
    int iunget, nunget;
    int eof;

    int interactive;
    int errexit;

    struct {
	unsigned mem_level;
	unsigned mem_size;
	void **mem;
    } mem;		/* memory from Lisp*Alloc, to be release in error */
    LispModule *module;
    LispObj *modules;
    char *prompt;

    LispObj *features;

    LispObj *modlist;		/* module list */
    LispObj *featlist;		/* features list */
    LispObj *packlist;		/* list of packages */
    LispObj *codlist;		/* current code */
    LispObj *frmlist;		/* input data */
    LispObj *runlist[3];	/* +, ++, and +++ */
    LispObj *reslist[3];	/* *, **, and *** */
    LispObj *dbglist;		/* debug information */
    LispObj *brklist;		/* breakpoints information */
    LispObj *prolist;		/* protect objects list */
    LispObj *doclist;		/* variables documentation */

#ifdef SIGNALRETURNSINT
    int (*sigint)(int);
    int (*sigfpe)(int);
#else
    void (*sigint)(int);
    void (*sigfpe)(int);
#endif

    int discard;		/* just discard #.EXP */

    int destroyed;		/* reached LispDestroy, used by unwind-protect */
    int running;		/* there is somewhere to siglongjmp */

    int debugging;		/* debugger enabled? */
    int debug_level;		/* almost always the same as mac->level */
    int debug_step;		/* control for stoping and printing output */
    int debug_break;		/* next breakpoint number */
    LispDebugState debug;
};

/*
 * Prototypes
 */
void LispUseArgList(LispMac*, LispArgList*);
void LispFreeArgList(LispMac*, LispArgList*);
LispArgList *LispCheckArguments(LispMac*, LispFunType, LispObj*, char*);

LispObj *LispGetDoc(LispMac*, LispObj*);
LispObj *LispGetVar(LispMac*, LispObj*);
void *LispGetVarAddr(LispMac*, LispObj*);	/* used by debugger */
LispObj *LispAddVar(LispMac*, LispObj*, LispObj*);
LispObj *LispSetVar(LispMac*, LispObj*, LispObj*);
void LispUnsetVar(LispMac*, LispObj*);

	/* only used at initialization time */
LispObj *LispNewStandardStream(LispMac*, LispFile*, LispObj*, int);

	/* create a new package */
LispObj *LispNewPackage(LispMac*, LispObj*, LispObj*);
	/* add package to use-list of current, and imports all extern symbols */
void LispUsePackage(LispMac*, LispObj*);
	/* make symbol extern in the current package */
void LispExportSymbol(LispMac*, LispObj*);
	/* imports symbol to current package */
void LispImportSymbol(LispMac*, LispObj*);

	/* always returns the same string */
char *LispGetAtomString(LispMac*, char*, int);

/* destructive fast reverse, note that don't receive a LispMac* argument */
LispObj *LispReverse(LispObj *list);

char *LispIntToOpaqueType(LispMac*, int);

/* (print) */
void LispPrint(LispMac*, LispObj*, LispObj*, int);

LispBlock *LispBeginBlock(LispMac*, LispObj*, LispBlockType);
#define BLOCKJUMP(block)	longjmp((block)->jmp, 1)
void LispEndBlock(LispMac*, LispBlock*);
	/* if unwind-protect active, jump to cleanup code, else do nothing */
void LispBlockUnwind(LispMac*, LispBlock*);

void LispUpdateResults(LispMac*, LispObj*, LispObj*);
void LispTopLevel(LispMac*);

LispAtom *LispDoGetAtom(LispMac*, char *str, int);
	/* get value from atom's property list */
LispObj *LispGetAtomProperty(LispMac*, LispAtom*, LispObj*);
	/* put value in atom's property list */
LispObj *LispPutAtomProperty(LispMac*, LispAtom*, LispObj*, LispObj*);

	/* define function, or replace function definition */
void LispSetAtomFunctionProperty(LispMac*, LispAtom*, LispObj*, LispArgList*);
	/* remove function property */
void LispRemAtomFunctionProperty(LispMac*, LispAtom*);
	/* define builtin, or replace builtin definition */
void LispSetAtomBuiltinProperty(LispMac*, LispAtom*, LispBuiltin*, LispArgList*);
	/* remove builtin property */
void LispRemAtomBuiltinProperty(LispMac*, LispAtom*);
	/* define setf macro, or replace current definition */
void LispSetAtomSetfProperty(LispMac*, LispAtom*, LispObj*, LispArgList*);
	/* remove setf macro */
void LispRemAtomSetfProperty(LispMac*, LispAtom*);
	/* create or change structure property */
void LispSetAtomStructProperty(LispMac*, LispAtom*, LispObj*, int);
	/* remove structure property */
void LispRemAtomStructProperty(LispMac*, LispAtom*);

void LispProclaimSpecial(LispMac*, LispObj*, LispObj*, LispObj*);
void LispDefconstant(LispMac*, LispObj*, LispObj*, LispObj*);

typedef enum _LispDocType_t {
    LispDocVariable,
    LispDocFunction,
    LispDocStructure,
    LispDocType,
    LispDocSetf
} LispDocType_t;

void LispAddDocumentation(LispMac*, LispObj*, LispObj*, LispDocType_t);
void LispRemDocumentation(LispMac*, LispObj*, LispDocType_t);
LispObj *LispGetDocumentation(LispMac*, LispObj*, LispDocType_t);

/* increases storage for functions returning multiple values */
void LispMoreReturns(LispMac*);

/* increases storage for temporarily protected data */
void LispMoreProtects(LispMac*);

/* Initialization */
extern int LispArgList_t;

#endif /* Lisp_private_h */

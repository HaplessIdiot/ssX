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

/* $XFree86: xc/programs/xedit/lisp/private.h,v 1.15 2001/10/20 00:19:35 paulo Exp $ */

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

/* Possible values to attach to a LispAtom include:
 *	a generic LispObject for global variable value
 *	a function definition
 *	a pointer to a builtin function definition
 *	the atom properties list, read with (get), set with (setf (get ...) ...)
 *	a setf expansion macro or function replacement name
 *	a structure definition
 */
struct _LispProperty {
    unsigned int object : 1;
    unsigned int function : 1;
    unsigned int builtin : 1;
    unsigned int property : 1;
    unsigned int defsetf : 1;
    unsigned int defstruct : 1;
    LispObj *value;
    union {
	LispObj *function;
	LispBuiltin *builtin;
    } fun;	/* cannot have both, a builtin and user function attached,
		 * virtually, builtin and user function are the same */
    LispObj *properties;
    LispObj *setf;
    struct {
	LispObj *definition;
#define STRUCT_NAME		-3
#define STRUCT_CHECK		-2
#define STRUCT_CONSTRUCTOR	-1
	int function;		/* if >= 0, it is a structure field index */
    } structure;
};

struct _LispAtom {
    unsigned int mark : 1;	/* gc protected */
    unsigned int prot : 1;	/* never released */
    unsigned int dyn : 1;	/* hint: dynamically binded variable */
    unsigned int prop : 1;	/* property field has useful data */
    char *string;
    LispObj *object;		/* backpointer to object ATOM */
    LispProperty property;
    struct _LispAtom *next;
};

struct _LispOpaque {
    int type;
    char *desc;
    LispOpaque *next;
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
    LispObj *standard_input, *input;
    LispObj *standard_output, *output;
    LispObj *error_stream;
    LispUngetInfo **unget;
    int iunget, nunget;
    int eof;

    struct {
	unsigned int fullbits : 8;	/* how many calls to to do a full gc
					 * including freeing unused strings */
	unsigned int expandbits : 3;	/* code doesn't look like reusing cells
					 * so try to have a larger number of
					 * free cells */
	unsigned int immutablebits : 1;	/* need to reset immutable bits */
	unsigned int timebits : 1;	/* update gctime counter */
	long gctime;
	int average;			/* of cells freed after gc calls */
    } gc;

    int princ;		/* don't quote strings? */
    int justsize;	/* just calculate size of output,
			 * needed to calculate formatted output */
    int newline;	/* at a newline in the output */
    int column;		/* column number in the output */
    int interactive;
    int errexit;
    LispAtom *strs[STRTBLSZ];
    LispOpaque *opqs[STRTBLSZ];
    int opaque;
    sigjmp_buf jmp;
    struct {
	unsigned block_level;
	unsigned block_size;
	LispObj *block_ret;
	LispBlock **block;
    } block;
    struct {
	unsigned mem_level;
	unsigned mem_size;
	void **mem;
    } mem;		/* memory from Lisp*Alloc, to be release in error */
    LispModule *module;
    LispObj *modules;
    char *prompt;

    LispObj *features;

    /* ENVIRONMENT */
    struct {
	LispObj **pairs;	/* value0 name0 ... valueN nameN */
	int lex;		/* until where variables are visible */
	int head;		/* top of environment */
	int base;		/* base of arguments to function */
	int length;		/* number of used pairs */
	int space;		/* number of objects in pairs */
    } env;

    /* GLOBAL VARIABLES */
    struct {
	LispObj **pairs;	/* name0 ... nameN */
	int length;		/* number of globals */
	int space;		/* number of objects in pairs */
    } glb;

    /* SPECIAL VARIABLES */
    struct {
	LispObj **pairs;	/* name0 ... nameN */
	int length;		/* number of specias */
	int space;		/* number of objects in pairs */
    } spc;

    /* DYNAMIC VARIABLES */
    struct {
	LispObj **pairs;	/* value0 name0 ... valueN nameN */
	int length;		/* number of dynamics * 2 */
	int space;		/* number of objects in pairs */
    } dyn;

    /* Unfortunately, evaluation of backquotes and macros sometimes uses a
     * lot of object cells.
     * This structure, to be used like the ones above allows minimizing
     * the need of temporary cells to protect data being evaluated. */
    struct {
	LispObj **objects;
	int length;
	int space;
    } protect;

    /* NIL, T */
    LispAtom *nil_atom, *t_atom;

    LispAtom *atom_atom, *symbol_atom, *real_atom, *integer_atom, *cons_atom,
	     *list_atom, *string_atom, *character_atom, *vector_atom,
	     *array_atom, *struct_atom, *keyword_atom, *function_atom,
	     *pathname_atom, *opaque_atom, *rational_atom, *float_atom,
	     *complex_atom;

    /* VARIABLE, STRUCTURE */
    LispAtom *variable_atom, *structure_atom, *type_atom;

    /* &KEY, &OPTIONAL, &REST, &AUX */
    LispAtom *key_atom, *optional_atom, *rest_atom, *aux_atom;

    /* OTHERWISE */
    LispAtom *otherwise_atom;

    /* XEDIT::MAKE-STRUCT, XEDIT::STRUCT-ACCESS, XEDIT::STRUCT-STORE,
       XEDIT::STRUCT-TYPE */
    LispAtom *make_struct_atom, *struct_access_atom, *struct_store_atom,
	     *struct_type_atom;

    /* CLOSE, FORMAT, LAMBDA, OPEN, SETF */
    LispAtom *close_atom, *format_atom, *lambda_atom, *open_atom,
	     *parse_namestring_atom, *setf_atom;

    /* :ERROR, IF-DOES-NOT-EXIST */
    LispAtom *error_atom, *if_does_not_exist_atom;

    /* :ABSOLUTE, :RELATIVE */
    LispAtom *absolute_atom, *relative_atom, *unspecific_atom;

    /* :PROBE, :INPUT, :OUTPUT, :IO, :NEW-VERSION, :RENAME,
       :RENAME-AND-DELETE, :OVERWRITE, :APPEND, :SUPERSEDE, CREATE */
    LispAtom *probe_atom, *input_atom, *output_atom, *io_atom,
	     *new_version_atom, *rename_atom, *rename_and_delete_atom,
	     *overwrite_atom, *append_atom, *supersede_atom, *create_atom;

    LispAtom *default_atom;

    LispAtom *make_array_atom;
    LispAtom *initial_contents_atom;

    LispObj *modlist;		/* module list */
    LispObj *featlist;		/* features list */
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
void LispCheckArguments(LispMac*, LispFunType, LispObj*, char*);

LispObj *LispGetDoc(LispMac*, LispObj*);
LispObj *LispGetVar(LispMac*, LispObj*);
void *LispGetVarAddr(LispMac*, LispObj*);	/* used by debugger */
LispObj *LispAddVar(LispMac*, LispObj*, LispObj*);
LispObj *LispSetVar(LispMac*, LispObj*, LispObj*);
void LispUnsetVar(LispMac*, LispObj*);

LispObj *LispNewStandardStream(LispMac*, LispFile*, LispObj*, int);

/* destructive fast reverse, note that don't receive a LispMac* argument */
LispObj *LispReverse(LispObj *list);

/* reads an expression from the selected stream */
LispObj *LispRun(LispMac*);

/* (print) */
void LispPrint(LispMac*, LispObj*, LispObj*, int);

LispBlock *LispBeginBlock(LispMac*, LispObj*, LispBlockType);
void LispEndBlock(LispMac*, LispBlock*);
	/* if unwind-protect active, jump to cleanup code, else do nothing */
void LispBlockUnwind(LispMac*, LispBlock*);

void LispUpdateResults(LispMac*, LispObj*, LispObj*);
void LispTopLevel(LispMac*);

LispAtom *LispDoGetAtom(LispMac*, char *str, int, int);
	/* get value from atom's property list */
LispObj *LispGetAtomProperty(LispMac*, LispAtom*, LispObj*);
	/* put value in atom's property list */
LispObj *LispPutAtomProperty(LispMac*, LispAtom*, LispObj*, LispObj*);

	/* create or change object property */
void LispSetAtomObjectProperty(LispMac*, LispAtom*, LispObj*);
	/* remove object property */
void LispRemAtomObjectProperty(LispMac*, LispAtom*);
	/* define function, or replace function definition */
void LispSetAtomFunctionProperty(LispMac*, LispAtom*, LispObj*);
	/* remove function property */
void LispRemAtomFunctionProperty(LispMac*, LispAtom*);
	/* define builtin, or replace builtin definition */
void LispSetAtomBuiltinProperty(LispMac*, LispAtom*, LispBuiltin*);
	/* remove builtin property */
void LispRemAtomBuiltinProperty(LispMac*, LispAtom*);
	/* define setf macro, or replace current definition */
void LispSetAtomSetfProperty(LispMac*, LispAtom*, LispObj*);
	/* remove setf macro */
void LispRemAtomSetfProperty(LispMac*, LispAtom*);
	/* create or change structure property */
void LispSetAtomStructProperty(LispMac*, LispAtom*, LispObj*, int);
	/* remove structure property */
void LispRemAtomStructProperty(LispMac*, LispAtom*);

void LispProclaimSpecial(LispMac*, LispObj*, LispObj*, LispObj*);

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

/* increases storage size for temporarily protected data */
void LispMoreProtects(LispMac*);

#endif /* Lisp_private_h */

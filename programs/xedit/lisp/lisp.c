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

/* $XFree86: xc/programs/xedit/lisp/lisp.c,v 1.11 2001/10/02 06:38:38 paulo Exp $ */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>

#include "private.h"

#include "format.h"
#include "require.h"
#include "time.h"

/*
 * Prototypes
 */
LispObj *LispRunFunMac(LispMac*, LispObj*, LispObj*);
void LispTopLevel(LispMac*);

static int LispGet(LispMac*);
static int LispUnget(LispMac*);
static int LispSkipComment(LispMac*);
static int LispSkipWhiteSpace(LispMac*);
static char *LispIntToOpaqueType(LispMac*, int);
static LispString *LispDoGetString(LispMac*, char *str, int prot);

void LispSnprint(LispMac*, LispObj*, char*, int);
void LispSnprintObj(LispMac*, LispObj*, char**, int*, int);

void LispCheckMemLevel(LispMac*);

void LispAllocSeg(LispMac*);
void LispMark(LispObj*);

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
static LispObj lispdot = {LispAtom_t};
LispObj *NIL = &lispnil, *T = &lispt;
static LispObj *DOT = &lispdot;
static LispObj **objseg, *freeobj = &lispnil;
static int pagesize, segsize, numseg;
static int nfree, nobjs;
int gcpro;

char *ExpectingListAt = "expecting list, at %s";
char *ExpectingNumberAt = "expecting number, at %s";
FILE *lisp_stdin, *lisp_stdout, *lisp_stderr;

/*
 * Implementation
 */
#include "table.c"

void
LispDestroy(LispMac *mac, char *fmt, ...)
{
    va_list ap;

    if (mac->debugging)
	LispDebugger(mac, LispDebugCallFatal, NIL, NIL);

    while (mac->mem.mem_level)
	free(mac->mem.mem[--mac->mem.mem_level]);

    /* panic */
    LispTopLevel(mac);
    if (mac->st) {
	mac->cp = &(mac->st[strlen(mac->st)]);
	mac->tok = 0;
    }

    fprintf(lisp_stderr, "%s", "*** Error: ");

    va_start(ap, fmt);
    vfprintf(lisp_stderr, fmt, ap);
    va_end(ap);

    fputc('\n', lisp_stderr);
    fflush(lisp_stderr);

    mac->column = 0;
    mac->newline = 1;

    if (mac->errexit)
	exit(1);

    longjmp(mac->jmp, 0);
}

void
LispTopLevel(LispMac *mac)
{
    ENV = GLB;
    SYM = LEX = COD = FRM = NIL;
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

    mac->setf = NULL;
    mac->cdr = mac->princ = mac->justsize = 0;
    if (mac->stream.stream_level) {
	free(mac->st);
	if (mac->stream.stream[mac->stream.stream_level].fp)
	    fclose(mac->stream.stream[mac->stream.stream_level].fp);
	--mac->stream.stream_level;
	while (mac->stream.stream_level) {
	    if (mac->stream.stream[mac->stream.stream_level].fp)
		fclose(mac->stream.stream[mac->stream.stream_level].fp);
	    free(mac->stream.stream[mac->stream.stream_level].st);
	    --mac->stream.stream_level;
	}
	mac->fp = mac->stream.stream[0].fp;
	mac->st = mac->stream.stream[0].st;
	mac->cp = mac->stream.stream[0].cp;
	mac->tok = mac->stream.stream[0].tok;
    }
    if (mac->mem.mem_level) {
	fprintf(lisp_stderr, "*** Warning: %d raw memory pointer(s) left. "
		"Probably a leak.\n", mac->mem.mem_level);
	mac->mem.mem_level = 0;
    }

    fflush(lisp_stdout);
    fflush(lisp_stderr);
}

void
LispGC(LispMac *mac, LispObj *car, LispObj *cdr)
{
    LispObj *entry;
    unsigned i, j;
    LispString *string, *pstring, *nstring;
#ifdef DEBUG
    struct timeval start, end;
    long sec, msec;
    int count = nfree;
    int strings_free = 0;
#endif

    if (gcpro)
	return;

#ifdef DEBUG
    fprintf(lisp_stderr, "gc: ");
    gettimeofday(&start, NULL);
#endif

    LispMark(MOD);
    LispMark(GLB);
    LispMark(FUN);
    LispMark(ENV);
    LispMark(SYM);
    LispMark(COD);
    LispMark(FRM);
    LispMark(STR);
    LispMark(RUN[0]);
    LispMark(RUN[1]);
    LispMark(RUN[2]);
    LispMark(RES[0]);
    LispMark(RES[1]);
    LispMark(RES[2]);
    LispMark(DBG);
    LispMark(BRK);
    LispMark(car);
    LispMark(cdr);

    /* Make all strings candidate to be released */
    for (i = 0; i < STRTBLSZ; i++) {
	string = mac->strs[i];
	while (string) {
	    string->mark = LispNil_t;
	    string = string->next;
	}
    }

    for (j = 0; j < numseg; j++)
	for (i = 0, entry = objseg[j]; i < segsize; i++, entry++) {
	    if (entry->prot)
		entry->dirty = entry->mark = LispTrue_t;
	    else if (entry->mark)
		entry->mark = LispNil_t;
	    else if (entry->dirty) {
		if (entry->type == LispStream_t) {
		    if (entry->data.stream.size < 0)
			fclose(entry->data.stream.source.fp);
		    else
			free(entry->data.stream.source.str);
		}
		CAR(entry) = NIL;
		CDR(entry) = freeobj;
		freeobj = entry;
		entry->dirty = LispNil_t;
		++nfree;
	    }
	}

    /* Needs a new pass because of the 'prot' field of cells. */
    for (j = 0; j < numseg; j++)
	for (i = 0, entry = objseg[j]; i < segsize; i++, entry++) {
	    if (entry->dirty || entry->mark) {
		switch (entry->type) {
		    case LispAtom_t:
		    case LispString_t:
			LispDoGetString(mac, entry->data.atom, 0)
			    ->mark = LispTrue_t;
			break;
		    case LispSymbol_t:
			LispDoGetString(mac, entry->data.symbol.name, 0)
			    ->mark = LispTrue_t;
			break;
		    default:
			break;
		}
	    }
	}

    /* Free unused strings */
    for (i = 0; i < STRTBLSZ; i++) {
	pstring = string = mac->strs[i];
	while (string) {
	    nstring = string->next;
	    if (!string->prot && string->mark == LispNil_t) {
		/* it is not required to call LispFree here */
		free(string->string);
		free(string);
		if (pstring == string)
		    pstring = mac->strs[i] = nstring;
		else
		    pstring->next = nstring;
#ifdef DEBUG
		++strings_free;
#endif
	    }
	    else
		pstring = string;
	    string = nstring;
	}
    }

#ifdef DEBUG
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    msec = end.tv_usec - start.tv_usec;
    if (msec < 0) {
	--sec;
	msec += 1000000;
    }
    fprintf(lisp_stderr, "%ld sec, %ld msec, ", sec, msec);
    fprintf(lisp_stderr, "%d recovered, %d free, %d protected, %d total\n", nfree - count, nfree, nobjs - nfree, nobjs);
    fprintf(lisp_stderr, "%d string(s) freed\n", strings_free);
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
    return (_LispSet(mac, var, val, fname, eval));
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
    opaque->desc = LispDoGetString(mac, desc, 1)->string;
    opaque->next = mac->opqs[ii];
    mac->opqs[ii] = opaque;
    LispMused(mac, opaque);

    return (opaque->type = ++mac->opaque);
}

static char *
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

static LispString *
LispDoGetString(LispMac *mac, char *str, int prot)
{
    LispString *string;
    int ii = 0;
    char *pp = str;

    while (*pp)
	ii = (ii << 1) ^ *pp++;
    if (ii < 0)
	ii = -ii;
    ii %= STRTBLSZ;
    for (string = mac->strs[ii]; string; string = string->next)
	if (strcmp(string->string, str) == 0) {
	    if (prot && !string->prot)
		string->prot = 1;
	    return (string);
	}
    string = (LispString*)LispMalloc(mac, sizeof(LispString));
    string->string = LispStrdup(mac, str);
    LispMused(mac, string);
    LispMused(mac, string->string);
    string->next = mac->strs[ii];
    mac->strs[ii] = string;
    string->dirty = 1;
    string->mark = 0;
    string->prot = !!prot;

    return (string);
}

char *
LispGetString(LispMac *mac, char *str)
{
    return (LispDoGetString(mac, str, 0)->string);
}

void
LispAllocSeg(LispMac *mac)
{
    unsigned i;
    LispObj **list, *obj;

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
#ifdef DEBUG
    fprintf(lisp_stdout, "gc: %d cell(s) allocated at %d segment(s)\n", nobjs, numseg);
#endif
}

void
LispMark(LispObj *obj)
{
    if (obj->mark)
	return;

    switch (obj->type) {
	case LispNil_t:
	    if (obj == NIL)
		return;
	    break;
	case LispTrue_t:
	    if (obj == T)
		return;
	    break;
	case LispSymbol_t:
	    if (obj->data.symbol.obj)
		LispMark(obj->data.symbol.obj);
	    LispMark(obj->data.symbol.plist);
	    break;
	case LispLambda_t:
	    LispMark(obj->data.lambda.name);
	    LispMark(obj->data.lambda.code);
	    break;
	case LispQuote_t:
	    LispMark(obj->data.quote);
	    break;
	case LispCons_t:
	    /* circular list on car */
	    if (CAR(obj) == obj) {
		obj->mark = LispTrue_t;
		obj = CDR(obj);
	    }
	    for (; obj->type == LispCons_t && obj->mark == LispNil_t;
		 obj = CDR(obj)) {
		LispMark(CAR(obj));
		obj->mark = LispTrue_t;
	    }
	    if (obj->type != LispCons_t)
		LispMark(obj);
	    return;
	case LispArray_t:
	    LispMark(obj->data.array.list);
	    LispMark(obj->data.array.dim);
	    break;
	case LispStruct_t:
	    /* def is protected when protecting STR */
	    LispMark(obj->data.struc.fields);
	    break;
	default:
	    break;
    }
    obj->mark = LispTrue_t;
}

void
LispProtect(LispObj *obj, int state)
{
    state = !!state;

    if (obj->prot == state)
	return;

    switch (obj->type) {
	case LispNil_t:
	    if (obj == NIL)
		return;
	    break;
	case LispTrue_t:
	    if (obj == T)
		return;
	    break;
	case LispSymbol_t:
	    if (obj->data.symbol.obj)
		LispProtect(obj->data.symbol.obj, state);
	    LispProtect(obj->data.symbol.plist, state);
	    break;
	case LispLambda_t:
	    LispProtect(obj->data.lambda.name, state);
	    LispProtect(obj->data.lambda.code, state);
	    break;
	case LispQuote_t:
	    LispProtect(obj->data.quote, state);
	    break;
	case LispCons_t:
	    /* circular list on car */
	    if (CAR(obj) == obj) {
		obj->mark = LispTrue_t;
		obj = CDR(obj);
	    }
	    for (; obj->type == LispCons_t && obj->mark == LispNil_t;
		 obj = CDR(obj)) {
		LispProtect(CAR(obj), state);
		obj->prot = state;
	    }
	    if (obj->type != LispCons_t)
		LispProtect(obj, state);
	    return;
	case LispArray_t:
	    LispProtect(obj->data.array.list, state);
	    LispProtect(obj->data.array.dim, state);
	    break;
	case LispStruct_t:
	    /* def is protected when protecting STR */
	    LispProtect(obj->data.struc.fields, state);
	    break;
	default:
	    break;
    }
    obj->prot = state;
}

static int
LispGet(LispMac *mac)
{
    int ch;

    if (mac->tok == EOF)
	return (EOF);

    if (mac->cp)
	ch = *mac->cp;
    else
	ch = 0;

    if (!ch) {
	if (mac->fp) {
	    char *ret;
	    char *code = malloc(1024);
	    int len;

	    if (code) {
		ret = fgets(code, 1024, mac->fp);
		len = ret ? strlen(code) : -1;
	    }
	    else
		len = -1;
	    if (len <= 0) {
		free(code);
		return (mac->tok = EOF);
	    }
	    if (mac->level == 0 || !mac->st) {
		if (mac->st)
		    free(mac->st);
		mac->st = mac->cp = code;
	    }
	    else {
		char *tmp = realloc(mac->st, (len = strlen(mac->st)) + strlen(code) + 1);

		if (!tmp) {
		    free(mac->st);
		    mac->st = NULL;
		    return (mac->tok = EOF);
		}
		mac->cp = &tmp[len];
		mac->st = tmp;
		strcpy(mac->cp, code);
		free(code);
	    }
	    return (LispGet(mac));
	}
	else
	    return (mac->tok = EOF);
    }

    ++mac->cp;
    if (ch == '\n' && mac->interactive && mac->fp == lisp_stdin) {
	mac->newline = 1;
	mac->column = 0;
    }

    return (mac->tok = ch);
}

static int
LispUnget(LispMac *mac)
{
    if (mac->cp > mac->st) {
	--mac->cp;
	return (1);
    }
    return (0);
}

LispObj *
LispNew(LispMac *mac, LispObj *car, LispObj *cdr)
{
    LispObj *obj;

    if (freeobj == NIL) {
	LispGC(mac, car, cdr);
	if (freeobj == NIL) {
	    LispAllocSeg(mac);
	    if (freeobj == NIL)
		LispDestroy(mac, "internal error, at INTERNAL:NEW");
	}
    }

    obj = freeobj;
    freeobj = CDR(obj);

    obj->dirty = LispTrue_t;
    obj->prot = LispNil_t;
    --nfree;

    return (obj);
}

LispObj *
LispNewNil(LispMac *mac)
{
    LispObj *nil = LispNew(mac, NIL, NIL);

    nil->type = LispNil_t;

    return (nil);
}

LispObj *
LispNewTrue(LispMac *mac)
{
    LispObj *t = LispNew(mac, NIL, NIL);

    t->type = LispTrue_t;

    return (t);
}

LispObj *
LispNewAtom(LispMac *mac, char *str)
{
    char *ptr = str;
    LispObj *atom = LispNew(mac, NIL, NIL);

    /* store atoms as uppercase */
    while (*ptr) {
	if (toupper(*ptr) != *ptr) {
	    unsigned char *tmp;

	    ptr = LispStrdup(mac, str);
	    for (tmp = (unsigned char*)ptr; *tmp; tmp++)
		*tmp = toupper(*tmp);
	    break;
	}
	++ptr;
    }
    if (*ptr == '\0')
	ptr = str;

    atom->type = LispAtom_t;
    atom->data.atom = LispGetString(mac, ptr);
    if (ptr != str)
	LispFree(mac, ptr);

    return (atom);
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
    string->data.atom = LispGetString(mac, str);

    return (string);
}

LispObj *
LispNewSymbol(LispMac *mac, char *name, LispObj *obj)
{
    LispObj *symbol = LispNew(mac, obj, NIL);

    symbol->type = LispSymbol_t;
    symbol->data.symbol.name = LispGetString(mac, name);
    symbol->data.symbol.obj = obj;
    symbol->data.symbol.plist = NIL;

    return (symbol);
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
	      int num_args, LispFunType type, int key, int optional, int rest)
{
    LispObj *fun = LispNew(mac, args, code);

    fun->type = LispLambda_t;
    fun->data.lambda.name = name;
    GCProtect();
    fun->data.lambda.code = CONS(args, code);
    GCUProtect();
    fun->data.lambda.num_args = num_args;
    fun->data.lambda.type = type;
    fun->data.lambda.key = key;
    fun->data.lambda.optional = optional;
    fun->data.lambda.rest = rest;

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
LispGetVar(LispMac *mac, char *name, int sym)
{
    LispObj *env, *var;

    if (LEX != NIL) {
	for (env = SYM; ; env = CDR(env)) {
	    if ((var = CAR(env)) == LEX)
		break;
	    if (var->data.symbol.name == name)
		return (sym ? var : var->data.symbol.obj);
	}
    }
    else {
	for (env = SYM; env != NIL; env = CDR(env))
	    if ((var = CAR(env))->data.symbol.name == name)
		return (sym ? var : var->data.symbol.obj);
    }

    for (env = ENV; env != LEX; env = CDR(env))
	if ((var = CAR(env))->data.symbol.name == name) {
	    GCProtect();
	    SYM = CONS(var, SYM);
	    GCUProtect();
	    return (sym ? var : var->data.symbol.obj);
	}

    if (LEX != NIL) {
	for (env = GLB; env != NIL; env = CDR(env))
	    if ((var = CAR(env))->data.symbol.name == name) {
		GCProtect();
		SYM = CONS(var, SYM);
		GCUProtect();
		return (sym ? var : var->data.symbol.obj);
	    }
    }

    return (NULL);
}

LispObj *
LispAddVar(LispMac *mac, char *name, LispObj *obj)
{
    LispObj *env, *var;

    if (LEX != NIL) {
	for (env = ENV; env != LEX; env = CDR(env))
	    if ((var = CAR(env))->data.symbol.name == name) {
		GCProtect();
		SYM = CONS(var, SYM);
		GCUProtect();
		return (var->data.symbol.obj = obj);
	    }
    }
    else {
	for (env = ENV; env != GLB; env = CDR(env))
	    if ((var = CAR(env))->data.symbol.name == name) {
		GCProtect();
		SYM = CONS(var, SYM);
		GCUProtect();
		return (var->data.symbol.obj = obj);
	    }
    }

    GCProtect();
    ENV = CONS(LispNewSymbol(mac, name, obj), ENV);
    SYM = CONS(CAR(ENV), SYM);
    GCUProtect();

    return (obj);
}

LispObj *
LispSetVar(LispMac *mac, char *name, LispObj *obj, int sym)
{
    LispObj *env, *var;

    /* try fast search */
    for (env = SYM; env != NIL; env = CDR(env))
	if ((var = CAR(env))->data.symbol.name == name)
	    return (var->data.symbol.obj = obj);

    /* normal search */
    for (env = ENV; env != NIL; env = CDR(env))
	if ((var = CAR(env))->data.symbol.name == name) {
	    GCProtect();
	    SYM = CONS(var, SYM);
	    GCUProtect();
	    return (var->data.symbol.obj = obj);
	}

    GCProtect();
    var = LispNewSymbol(mac, name, obj);
    if (GLB == NIL)
	SYM = ENV = GLB = CONS(var, NIL);
    else {
	CDR(GLB) = CONS(CAR(GLB), CDR(GLB));
	CAR(GLB) = var;
	SYM = CONS(var, SYM);
    }
    GCUProtect();

    return (sym ? var : obj);
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

LispObj *
LispEnvRun(LispMac *mac, LispObj *args, LispFunPtr fn, char *fname, int refs)
{
    LispObj *old_frm, *old_env, *old_sym, *env, *res, *list, *pair;

    old_frm = FRM;
    old_env = ENV;
    old_sym = SYM;
    env = CAR(args);
    list = NIL;

    if (env != NIL) {
	if (env->type != LispCons_t)
	    LispDestroy(mac, "%s is not of type list, at %s",
			LispStrObj(mac, env), fname);
    }

    for (; env != NIL; env = CDR(env)) {
	LispObj *var = NIL, *val = NIL;

	pair = CAR(env);
	if (pair->type == LispAtom_t) {
	    var = pair;
	    val = NIL;
	}
	else if (pair->type == LispCons_t) {
	    var = CAR(pair);
	    if (var->type != LispAtom_t)
		LispDestroy(mac, "%s is invalid as a variable name, at %s",
			    LispStrObj(mac, var), fname);
	    pair = CDR(pair);
	    if (pair == NIL)
		val = NIL;
	    else {
		val = CAR(pair);
		if (CDR(pair) != NIL)
		    LispDestroy(mac, "too much arguments to initialize %s, at %s",
				var->data.atom, fname);
	    }
	}
	else
	    LispDestroy(mac, "%s is not of type list, at %s",
			LispStrObj(mac, pair), fname);
	val = EVAL(val);
	if (!refs) {
	    GCProtect();
	    pair = CONS(var, val);
	    if (list == NIL) {
		list = CONS(pair, NIL);
		FRM = CONS(list, FRM);
	    }
	    else {
		CDR(list) = CONS(CAR(list), CDR(list));
		CAR(list) = pair;
	    }
	    GCUProtect();
	}
	else
	    LispAddVar(mac, var->data.atom, val);
    }

    if (!refs && list != NIL) {
	/* Need to update CAR(FRM) or will run function without gc protection! */
	list = CAR(FRM) = LispReverse(list);
	for (; list != NIL; list = CDR(list)) {
	    pair = CAR(list);
	    LispAddVar(mac, CAR(pair)->data.atom, CDR(pair));
	}
    }

    res = fn(mac, CDR(args), fname);

    SYM = old_sym;
    ENV = old_env;
    FRM = old_frm;

    return (res);
}

LispBlock *
LispBeginBlock(LispMac *mac, LispObj *tag, int eval)
{
    unsigned blevel = mac->block.block_level + 1;
    LispBlock *block;

    if (blevel >= mac->block.block_size) {
	LispBlock **blk = realloc(mac->block.block,
				  sizeof(LispBlock*) * (blevel + 1));

	if (blk == NULL)
	    LispDestroy(mac, "out of memory");
	else if ((block = malloc(sizeof(LispBlock))) == NULL)
	    LispDestroy(mac, "out of memory");
	mac->block.block = blk;
	mac->block.block[mac->block.block_size] = block;
	mac->block.block_size = blevel;
    }
    block = mac->block.block[mac->block.block_level];
    if (eval)
	tag = EVAL(tag);
    memcpy(&(block->tag), tag, sizeof(LispObj));

    block->level = mac->level;
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
    mac->level = block->level;
    mac->block.block_level = block->block_level;

    if (mac->debugging) {
	if (mac->debug_level >= block->debug_level) {
	    while (mac->debug_level > block->debug_level) {
		DBG = CDR(DBG);
		--mac->debug_level;
	    }
	}
	else
	    LispDestroy(mac, "this should never happen: "
			"mac->debug_level < block->debug_level");
	mac->debug_step = block->debug_step;
    }
}

static int
LispSkipComment(LispMac *mac)
{
    int ch;

    /*CONSTCOND*/
    while (1) {
	while (ch = LispGet(mac), ch != '\n' && ch != EOF)
	    ;
	if (ch == EOF)
	    return (0);
	while (ch = LispGet(mac), isspace(ch) && ch != EOF)
	    ;
	if (ch == EOF)
	    return (0);
	if (ch != ';') {
	    LispUnget(mac);
	    return (1);
	}
    }
    /*NOTREACHED*/
}

static int
LispSkipWhiteSpace(LispMac *mac)
{
    int ch;

    while (ch = LispGet(mac), isspace(ch) && ch != EOF)
	;
    if (ch == ';') {
	if (!LispSkipComment(mac))
	    return (EOF);
	return (LispGet(mac));
    }
    return (ch);
}

LispObj *
LispRun(LispMac *mac)
{
    static char *DOTMSG = "Illegal end of dotted list";
    int ch, len, dquote = 0, escape = 0, size, dot = 0;
    LispObj *res, *obj, *cons, *code, *frm;
    char stk[1024], *str;

    code = COD;
    frm = FRM;
    switch (ch = LispSkipWhiteSpace(mac)) {
	case '(':
	    if (LispSkipWhiteSpace(mac) == ')') {
		res = NIL;
		break;
	    }
	    (void)LispUnget(mac);
	    res = cons = CONS(NIL, NIL);
	    if (COD == NIL)
		COD = res;
	    else
		FRM = CONS(res, FRM);
	    if ((CAR(cons) = LispRun(mac)) == DOT)
		LispDestroy(mac, "Illegal start of dotted list");
	    while ((obj = LispRun(mac)) != NULL) {
		if (obj == DOT) {
		    if (dot)
			LispDestroy(mac, DOTMSG);
		    dot = 1;
		}
		else {
		    if (dot) {
			if (++dot > 2)
			    LispDestroy(mac, DOTMSG);
			CDR(cons) = obj;
		    }
		    else {
			CDR(cons) = CONS(obj, NIL);
			cons = CDR(cons);
		    }
		}
	    }
	    if (dot == 1)
		LispDestroy(mac, DOTMSG);
	    break;
	case ')':
	case EOF:
	    return (NULL);
	case '\'':
	    if ((obj = LispRun(mac)) == NULL)
		LispDestroy(mac, "Illegal quoted object");
	    res = QUOTE(obj);
	    break;
	case '"':
	    dquote = 1;
	    escape = 1;
	    /*FALLTHROUGH*/
	default:
	    len = 0;
	    size = sizeof(stk);
	    str = stk;
	    while (ch != EOF && (dquote ||
		   (!isspace(ch) && ch != ')' && ch != '(' && ch != ';'))) {
		if (len >= size - 1) {
		    char *tmp;

		    if (str == stk)
			tmp = (char*)LispMalloc(mac, size + 1024);
		    else
			tmp = (char*)LispRealloc(mac, str, size + 1024);
		    str = tmp;
		    size += 1024;
		}
		if (dquote) {
		    if (ch == '\\')
			escape = !escape;
		    else if (ch == '"' && !escape) {
			str[len++] = ch;
			break;
		    }
		    else
			escape = 0;
		    str[len++] = ch;
		}
		else
		    str[len++] = toupper(ch);
		ch = LispGet(mac);
	    }
	    str[len] = '\0';
	    if (ch == '(' || ch == ')' || ch == ';')
		LispUnget(mac);
	    if (!len)
		res = NIL;
	    else if (str[0] == '"') {
		str[strlen(str) - 1] = '\0';
		res = STRING(str + 1);
	    }
	    else if (isdigit(str[0]) ||
		     ((str[0] == '-' || str[0] == '.' || str[0] == '+') &&
		      isdigit(str[1]))) {
		double value;
		char *cp;

		value = strtod(str, &cp);
		if (cp && *cp)
		    res = ATOM(str);
		else
		    res = REAL(value);
	    }
	    else {
		if (strcmp(str, "NIL") == 0)
		    res = NIL;
		else if (strcmp(str, "T") == 0)
		    res = T;
		else if (strcmp(str, ".") == 0)
		    res = DOT;
		else
		    res = ATOM(str);
	    }
	    if (str != stk)
		LispFree(mac, str);
	    break;
    }

    if (code == NIL)
	COD = res;
    FRM = frm;

    return (res);
}

LispObj *
LispEval(LispMac *mac, LispObj *obj)
{
    char *strname;
    LispObj *name, *fun, *cons = NIL, *frm, *res;
    LispObj *car, *cdr;
    LispBuiltin *fn;
    unsigned num_objs;

    if (!obj)
	LispDestroy(mac, "internal error, at INTERNAL:EVAL");

    switch (obj->type) {
	case LispAtom_t:
	    strname = obj->data.atom;
	    if (mac->debugging)
		LispDebugger(mac, LispDebugCallBegini, NIL, obj);
	    if (obj->data.atom[0] != ':' &&
		(obj = LispGetVar(mac, obj->data.atom, 0)) == NULL)
		LispDestroy(mac, "the variable %s is unbound", strname);
	    if (mac->debugging)
		LispDebugger(mac, LispDebugCallEndi, NIL, obj);
	    return (obj);
	case LispQuote_t:
	    if (mac->debugging) {
		LispDebugger(mac, LispDebugCallBegini, NIL, obj);
		LispDebugger(mac, LispDebugCallEndi, NIL, obj->data.quote);
	    }
	    return (obj->data.quote);
	case LispSymbol_t:
	    /* don't call debugger here, let LispEval be called again. */
	    return (EVAL(obj->data.symbol.obj));
	case LispCons_t:
	    cons = obj;
	    break;
	case LispNil_t:
	case LispTrue_t:
	case LispReal_t:
	case LispString_t:
	case LispOpaque_t:
	default:
	    /* don't {step,next}i on literals */
	    return (obj);
    }
    car = CAR(cons);
    fun = NIL;
    switch (car->type) {
	case LispAtom_t:
	    name = car;
	    strname = name->data.atom;
	    break;
	case LispLambda_t:
	    name = NIL;
	    strname = "NIL";
	    fun = car;
	    break;
	default:
	    LispDestroy(mac, "%s is invalid as a function, at INTERNAL:EVAL",
			LispStrObj(mac, car));
	    /*NOTREACHED*/
    }

    ++mac->level;
    frm = FRM;

    if (mac->debugging)
	LispDebugger(mac, LispDebugCallBegin, name, CDR(cons));

    if (fun == NIL) {
	int len = strlen(strname);

	if ((fn = LispFindBuiltin(strname, len)) == NULL) {
	    LispModule *module = mac->module;

	    while (module != NULL) {
		if (module->data->find_fun &&
		    (fn = (module->data->find_fun)(strname, len)) != NULL)
		    break;
		module = module->next;
	    }

	    if (fn == NULL) {
		LispObj *str;

		/* first check for structure functions */
		if (strncmp("MAKE-", strname, 5) == 0) {
		    for (str = STR; str != NIL; str = CDR(str)) {
			if (strcmp(CAR(CAR(str))->data.atom, strname + 5) == 0) {
			    mac->struc = CAR(str);
			    fn = &LispMakeStructDef;
			    break;
			}
		    }
		}
		else if (strchr(strname, '-')) {
		    char *sname, *ptr;

		    /* check if it is a structure field access function */
		    for (str = STR; str != NIL; str = CDR(str)) {
			ptr = strname;
			sname = CAR(CAR(str))->data.atom;
			while (*ptr == *sname) {
			    ++ptr;
			    ++sname;
			}
			if (*sname == '\0' && *ptr == '-') {
			    LispObj *res;

			    ++ptr;
			    if (ptr[0] == 'P' && ptr[1] == '\0') {
				/* Just checking type */
				if (CDR(cons) == NIL)
				    LispDestroy(mac, "too few arguments to %s",
						strname);
				else if (CDR(CDR(cons)) != NIL)
				    LispDestroy(mac, "too many arguments to %s",
						strname);
				res = EVAL(CAR(CDR(cons)));

				return (res->type == LispStruct_t &&
					res->data.struc.def == CAR(str) ?
					T : NIL);
			    }
			    mac->struc_field = 0;
			    for (res = CDR(CAR(str)); res != NIL;
				 res = CDR(res), mac->struc_field++) {
				if ((CAR(res)->type == LispAtom_t &&
				     strcmp(CAR(res)->data.atom, ptr) == 0) ||
				    (CAR(res)->type == LispCons_t &&
				     strcmp(CAR(CAR(res))->data.atom, ptr) == 0))
				    break;
			    }
			    if (res != NIL) {
				fn = &LispStructAccessDef;
				mac->struc = CAR(str);
				break;
			    }
			}
		    }
		}
	    }
	}

	if (fn) {
	    LispObj *args;

	    cdr = CDR(cons);
	    num_objs = 0;
	    while (cdr->type == LispCons_t) {
		++num_objs;
		cdr = CDR(cdr);
	    }
	    if ((fn->max_args && num_objs > fn->max_args)
		|| (fn->min_args && num_objs < fn->min_args))
		LispDestroy(mac, "bad number of arguments to %s", strname);

	    if (fn->eval && num_objs) {
		LispObj *arg = CDR(cons);

		res = EVAL(CAR(arg));
		GCProtect();
		args = cdr = CONS(res, NIL);
		FRM = CONS(args, FRM);
		GCUProtect();
		arg = CDR(arg);
		while (arg->type == LispCons_t) {
		    CDR(cdr) = CONS(EVAL(CAR(arg)), NIL);
		    cdr = CDR(cdr);
		    arg = CDR(arg);
		}
	    }
	    else
		args = CDR(cons);

	    mac->setf = NULL;
	    mac->cdr = 0;
	    res = fn->fn(mac, args, strname);
	    if (mac->debugging)
		LispDebugger(mac, LispDebugCallEnd, name, res);
	    FRM = frm;
	    --mac->level;

	    return (res);
	}

	for (fun = FUN; fun != NIL; fun = CDR(fun))
	    if (CAR(fun)->data.lambda.name->data.atom == strname) {
		fun = CAR(fun);
		break;
	    }
    }

    if (fun != NIL) {
	LispObj *args = NIL;

	if (!fun->data.lambda.key && !fun->data.lambda.optional &&
	    !fun->data.lambda.rest) {
	    cdr = CDR(cons);
	    num_objs = 0;
	    while (cdr->type == LispCons_t) {
		++num_objs;
		cdr = CDR(cdr);
	    }
	    if (num_objs != fun->data.lambda.num_args)
		LispDestroy(mac, "%u is a bad number of arguments to %s,"
			    " needs %u", num_objs, strname,
			    fun->data.lambda.num_args);

	    if (fun->data.lambda.type == LispMacro)
		args = CDR(cons);
	    else if (num_objs) {
		LispObj *arg = CDR(cons);

		res = EVAL(CAR(arg));
		GCProtect();
		args = cdr = CONS(res, NIL);
		FRM = CONS(args, FRM);
		GCUProtect();
		arg = CDR(arg);
		while (arg->type == LispCons_t) {
		    CDR(cdr) = CONS(EVAL(CAR(arg)), NIL);
		    cdr = CDR(cdr);
		    arg = CDR(arg);
		}
	    }
	}
	else {
	    int rest, optional, key;
	    LispObj *arg, *list = CAR(fun->data.lambda.code), *keyword = NIL;

	    /* build argument list */
	    while (list != NIL) {
		if (CAR(list)->data.atom[0] != '&') {
		    if (args == NIL) {
			GCProtect();
			args = cdr = CONS(list, NIL);
			FRM = CONS(args, FRM);
			GCUProtect();
		    }
		    else {
			CDR(cdr) = CONS(list, NIL);
			cdr = CDR(cdr);
		    }
		}
		list = CDR(list);
	    }

	    /* fill argument list */
	    list = CAR(fun->data.lambda.code);
	    rest = optional = key = 0;
	    cdr = CDR(cons);
	    arg = args;
	    while (list != NIL) {
		if (CAR(list)->data.atom[0] == '&') {
		    if (strcmp(CAR(list)->data.atom + 1, "KEY") == 0)
			key = 1;
		    else if (strcmp(CAR(list)->data.atom + 1, "REST") == 0) {
			rest = 1;
			break;
		    }
		    else /* "OPTIONAL" */
			optional = 1;
		}
		else {
		    if (cdr == NIL) {
			if (key || optional)
			    break;
			else
			    LispDestroy(mac, "too few arguments to %s", strname);
		    }
		    else {
			if (key) {
			    if (CAR(cdr)->type != LispAtom_t ||
				CAR(cdr)->data.atom[0] != ':')
				LispDestroy(mac, "&KEY needs arguments as pairs,"
					    " at %s", strname);
			    else if (CDR(cdr) == NIL)
				LispDestroy(mac, "expecting %s value, at %s",
					    CAR(cdr)->data.atom, strname);
			    keyword = CAR(cdr);
			    cdr = CDR(cdr);
			}
			if (fun->data.lambda.type == LispMacro)
			    res = CAR(cdr);
			else
			    res = EVAL(CAR(cdr));
			if (key) {
			    LispObj *atmp, *cmp,
				    *ltmp = CAR(fun->data.lambda.code);

			    for (; ltmp != NIL; ltmp = CDR(ltmp)) {
				if ((cmp = CAR(ltmp))->type == LispCons_t)
				    cmp = CAR(cmp);
				else if (cmp->data.atom[0] == '&')
				    continue;
				if (strcmp(cmp->data.atom,
					   keyword->data.atom + 1) == 0)
				    break;
			    }
			    if (ltmp == NIL)
				LispDestroy(mac, "%s is not an argument to %s",
					    keyword->data.atom, strname);
			    for (atmp = args; atmp != NIL; atmp = CDR(atmp))
				if (CAR(atmp) == ltmp) {
				    CAR(atmp) = res;
				    break;
				}
				/* else, silently ignore setting argument
				 * more than once? */
			}
			else
			    CAR(arg) = res;
			cdr = CDR(cdr);
		    }
		    arg = CDR(arg);
		}
		list = CDR(list);
	    }
	    if (rest) {
		if (cdr == NIL)
		    CAR(arg) = NIL;
		else if (fun->data.lambda.type == LispMacro)
		    CAR(arg) = CONS(cdr, NIL);
		else {
		    res = EVAL(CAR(cdr));
		    CAR(arg) = CONS(res, NIL);
		    arg = CAR(arg);
		    cdr = CDR(cdr);
		    while (cdr->type == LispCons_t) {
			res = EVAL(CAR(cdr));
			CDR(arg) = CONS(res, NIL);
			arg = CDR(arg);
			cdr = CDR(cdr);
		    }
		}
	    }
	    else if (cdr != NIL)
		LispDestroy(mac, "too many arguments to %s", strname);

	    /* set to NIL or default any unspecified arguments */
	    if (key || optional) {
		arg = args;
		list = CAR(fun->data.lambda.code);

		for (; list != NIL; list = CDR(list))
		    if (CAR(list)->data.atom[0] != '&') {
			if (CAR(arg) == list) {
			    if (CAR(list)->type == LispCons_t &&
				CDR(CAR(list))->type == LispCons_t)
				CAR(arg) = EVAL(CAR(CDR(CAR(list))));
			    else
				CAR(arg) = NIL;
			}
			arg = CDR(arg);
		    }
	    }
	}

	res = LispRunFunMac(mac, fun, args);
	if (mac->debugging)
	    LispDebugger(mac, LispDebugCallEnd, fun->data.lambda.name, res);
	FRM = frm;
	--mac->level;

	return (res);
    }

    LispDestroy(mac, "the function %s is not defined", strname);
    /*NOTREACHED*/

    return (NIL);
}

LispObj *
LispRunFunMac(LispMac *mac, LispObj *fun, LispObj *list)
{
    LispFunType type = fun->data.lambda.type;
    LispObj *old_env, *old_sym, *old_lex, *args, *code, *res;

    old_env = ENV;
    old_sym = SYM;
    old_lex = LEX;

    args = CAR(fun->data.lambda.code);
    code = CDR(fun->data.lambda.code);

    LEX = ENV;
    SYM = CONS(LEX, SYM);

    for (; args != NIL; args = CDR(args)) {
	if (CAR(args)->type == LispCons_t) {
	    LispAddVar(mac, CAR(CAR(args))->data.atom, CAR(list));
	    list = CDR(list);
	}
	else if (CAR(args)->data.atom[0] != '&') {
	    LispAddVar(mac, CAR(args)->data.atom, CAR(list));
	    list = CDR(list);
	}
    }

    if (type != LispMacro) {
	int did_jump = 1;
	LispBlock *block = LispBeginBlock(mac, fun->data.lambda.name, 0);

	res = NIL;
	if (setjmp(block->jmp) == 0) {
	    res = Lisp_Progn(mac, code, fun->data.lambda.name->data.atom);
	    did_jump = 0;
	}
	LispEndBlock(mac, block);
	if (did_jump)
	    res = mac->block.block_ret;
    }
    else
	res = Lisp_Progn(mac, code, "#<LAMBDA>");

    LEX = old_lex;
    SYM = old_sym;
    ENV = old_env;

    if (type == LispMacro)
	res = EVAL(res);

    return (res);
}

void
LispSnprintObj(LispMac *mac, LispObj *obj, char **str, int *len, int paren)
{
    int sz;

    if (*len < 1)
	return;
    switch (obj->type) {
	case LispNil_t:
	    sz = snprintf(*str, *len, "NIL");
	    *len -= sz;
	    *str += sz;
	    break;
	case LispTrue_t:
	    sz = snprintf(*str, *len, "T");
	    *len -= sz;
	    *str += sz;
	    break;
	case LispOpaque_t:
	    sz = snprintf(*str, *len, "#0x%08x-%s", (int)obj->data.opaque.data,
			  LispIntToOpaqueType(mac, obj->data.opaque.type));
	    *len -= sz;
	    *str += sz;
	    break;
	case LispAtom_t:
	    sz = snprintf(*str, *len, "%s", obj->data.atom);
	    *len -= sz;
	    *str += sz;
	    break;
	case LispString_t:
	    sz = snprintf(*str, *len, "\"%s\"", obj->data.atom);
	    *len -= sz;
	    *str += sz;
	    break;
	case LispReal_t: {
	    char stk[32];

	    snprintf(stk, sizeof(stk), "%g", obj->data.real);
	    sz = snprintf(*str, *len, stk);
	    *len -= sz;
	    *str += sz;
	}    break;
	case LispCons_t: {
	    LispObj *car, *cdr;

	    car = CAR(obj);
	    cdr = CDR(obj);
	    if (!cdr || cdr->type == LispNil_t) {
		if (paren) {
		    sz = snprintf(*str, *len, "(");
		    if ((*len -= sz) <= 0)
			return;
		    *str += sz;
		}
		LispSnprintObj(mac, car, str, len, car->type == LispCons_t);
		if (*len <= 0)
		    return;
		if (paren) {
		    sz = snprintf(*str, *len, ")");
		    if ((*len -= sz) <= 0)
			return;
		    *str += sz;
		}
	    }
	    else {
		if (paren) {
		    sz = snprintf(*str, *len, "(");
		    if ((*len -= sz) <= 0)
			return;
		    *str += sz;
		}
		LispSnprintObj(mac, car, str, len, car->type == LispCons_t);
		if (*len <= 0)
		    return;
		if (cdr->type == LispQuote_t) {
		    sz = snprintf(*str, *len, " QUOTE ");
		    if ((*len -= sz) <= 0)
			return;
		    *str += sz;
		    LispSnprintObj(mac, cdr->data.quote, str, len, 0);
		}
		else if (cdr->type != LispCons_t) {
		    sz = snprintf(*str, *len, " . ");
		    if ((*len -= sz) <= 0)
			return;
		    *str += sz;
		    LispSnprintObj(mac, cdr, str, len, 0);
		}
		else {
		    sz = snprintf(*str, *len, " ");
		    if ((*len -= sz) <= 0)
			return;
		    *str += sz;
		    LispSnprintObj(mac, cdr, str, len, car->type != LispCons_t &&
				   cdr->type != LispCons_t);
		    if (*len <= 0)
			return;
		}
		if (paren) {
		    sz = snprintf(*str, *len, ")");
		    *len -= sz;
		    *str += sz;
		}
	    }
	}    break;
	case LispQuote_t:
	    sz = snprintf(*str, *len, "'");
	    *len -= sz;
	    *str += sz;
	    LispSnprintObj(mac, obj->data.quote, str, len, 1);
	    break;
	case LispArray_t:
	    if (obj->data.array.rank == 1)
		sz = snprintf(*str, *len, "#(");
	    else
		sz = snprintf(*str, *len, "#%dA(", obj->data.array.rank);
	    if ((*len -= sz) <= 0)
		return;
	    *str += sz;
	    if (!obj->data.array.zero) {
		if (obj->data.array.rank == 1) {
		    LispObj *ary;
		    long count;

		    for (ary = obj->data.array.dim, count = 1;
			 ary != NIL; ary = CDR(ary))
			count *= (int)CAR(ary)->data.real;
		    for (ary = obj->data.array.list; count > 0;
			ary = CDR(ary), count--) {
			LispSnprintObj(mac, CAR(ary), str, len, 0);
			if (*len <= 0)
			    return;
			if (count - 1 > 0) {
			    sz = snprintf(*str, *len, " ");
			    if ((*len -= sz) <= 0)
				return;
			    *str += sz;
			}
		    }
		}
		else {
		    LispObj *ary;
		    int i, k, rank, *dims, *loop;

		    rank = obj->data.array.rank;
		    dims = LispMalloc(mac, sizeof(int) * rank);
		    loop = LispCalloc(mac, 1, sizeof(int) * (rank - 1));

		    /* fill dim */
		    for (i = 0, ary = obj->data.array.dim; ary != NIL;
			 i++, ary = CDR(ary))
			dims[i] = (int)CAR(ary)->data.real;

		    i = 0;
		    ary = obj->data.array.list;
		    while (loop[0] < dims[0]) {
			for (; i < rank - 1; i++) {
			    sz = snprintf(*str, *len, "(");
			    if ((*len -= sz) <= 0)
				goto snprint_array_done;
			    *str += sz;
			}
			--i;
			for (;;) {
			    ++loop[i];
			    if (i && loop[i] >= dims[i])
				loop[i] = 0;
			    else
				break;
			    --i;
			}
			for (k = 0; k < dims[rank - 1] - 1; k++, ary = CDR(ary)) {
			    LispSnprintObj(mac, CAR(ary), str, len, 0);
			    if (*len <= 0)
				goto snprint_array_done;
			    sz = snprintf(*str, *len, " ");
			    if ((*len -= sz) <= 0)
				goto snprint_array_done;
			    *str += sz;
			}
			LispSnprintObj(mac, CAR(ary), str, len, 1);
			if (*len <= 0)
			    goto snprint_array_done;
			ary = CDR(ary);
			for (k = rank - 1; k > i; k--) {
			    sz = snprintf(*str, *len, ")");
			    if ((*len -= sz) <= 0)
				goto snprint_array_done;
			    *str += sz;
			}
			if (loop[0] < dims[0]) {
			    sz = snprintf(*str, *len, " ");
			    if ((*len -= sz) <= 0)
				goto snprint_array_done;
			    *str += sz;
			}
		    }

snprint_array_done:
		    LispFree(mac, dims);
		    LispFree(mac, loop);
		}
	    }
	    sz = snprintf(*str, *len, ")");
	    *len -= sz;
	    *str += sz;
	    break;
	case LispStruct_t: {
	    LispObj *def = obj->data.struc.def;
	    LispObj *field = obj->data.struc.fields;

	    sz = snprintf(*str, *len, "S#(%s", CAR(def)->data.atom);
	    if ((*len -= sz) <= 0)
		return;
	    *str += sz;
	    def = CDR(def);
	    for (; def != NIL; def = CDR(def), field = CDR(field)) {
		sz = snprintf(*str, *len, " :%s ", CAR(def)->type == LispAtom_t ?
			      CAR(def)->data.atom : CAR(CAR(def))->data.atom);
		if ((*len -= sz) <= 0)
		    return;
		*str += sz;
		LispSnprintObj(mac, CAR(field), str, len, 1);
		if (*len <= 0)
		    return;
	    }
	    sz = snprintf(*str, *len, ")");
	    *len -= sz;
	    *str += sz;
	}   break;
	default:
	    break;
    }
}

char *
LispStrObj(LispMac *mac, LispObj *obj)
{
    static char string[12];

    LispSnprint(mac, obj, string, sizeof(string) - 1);
    return (string);
}

void
LispSnprint(LispMac *mac, LispObj *obj, char *str, int len)
{
    char *s = str;
    int l = len;

    if (!obj || !str || len <= 0)
	LispDestroy(mac, "internal error, at INTERNAL:SPRINT");
    LispSnprintObj(mac, obj, &str, &len, 1);
    if (len <= 0) {
    /* this is a internal function, so I assume that str has enough space */
	if (*s == '(')
	    strcpy(s + l - 5, "...)");
	else
	    strcpy(s + l - 4, "...");
    }
}

int
LispPrintf(LispMac *mac, LispObj *stream, char *fmt, ...)
{
    int size;
    va_list ap;
    FILE *fp = NULL;

    if (stream == NIL)
	fp = lisp_stdout;
    else if (stream->data.stream.size < 0)
	fp = stream->data.stream.source.fp;

    va_start(ap, fmt);
    if (fp && !mac->justsize)
	size = vfprintf(fp, fmt, ap);
    else {
	int n;
	unsigned char stk[1024], *ptr = stk;

	size = sizeof(stk);
	n = vsnprintf((char*)stk, size, fmt, ap);
	if (n < 0 || n >= size) {
	    while (1) {
		char *tmp;

		va_end(ap);
		if (n > size)
		    size = n + 1;
		else
		    size *= 2;
		if ((tmp = realloc(ptr, size)) == NULL) {
		    free(ptr);
		    LispDestroy(mac, "out of memory");
		}
		va_start(ap, fmt);
		n = vsnprintf((char*)ptr, size, fmt, ap);
		if (n >= 0 && n < size)
		    break;
	    }
	}
	size = strlen((char*)ptr);

	if (!mac->justsize) {
	    while (stream->data.stream.idx + size >= stream->data.stream.size) {
		unsigned char *tmp = realloc(stream->data.stream.source.str,
					     stream->data.stream.size + pagesize);

		if (tmp == NULL) {
		    if (ptr != stk)
			free(ptr);
		    LispDestroy(mac, "out of memory");
		}
		stream->data.stream.source.str = tmp;
		stream->data.stream.size += pagesize;
	    }
	    strcpy((char*)stream->data.stream.source.str +
		   stream->data.stream.idx, (char*)ptr);
	    stream->data.stream.idx += size;
	}
	if (ptr != stk)
	    free(ptr);
    }
    va_end(ap);

    return (size);
}

int
LispPrintObj(LispMac *mac, LispObj *stream, LispObj *obj, int paren)
{
    int len = 0;

    switch (obj->type) {
	case LispNil_t:
	    len += LispPrintf(mac, stream, "NIL");
	    break;
	case LispTrue_t:
	    len += LispPrintf(mac, stream, "T");
	    break;
	case LispOpaque_t:
	    len += LispPrintf(mac, stream, "#0x%08x-%s",
			      (int)obj->data.opaque.data,
			      LispIntToOpaqueType(mac, obj->data.opaque.type));
	    break;
	case LispAtom_t:
	    len += LispPrintf(mac, stream, "%s", obj->data.atom);
	    break;
	case LispString_t:
	    if (mac->princ)
		len += LispPrintf(mac, stream, "%s", obj->data.atom);
	    else
		len += LispPrintf(mac, stream, "\"%s\"", obj->data.atom);
	    break;
	case LispReal_t:
	    len += LispPrintf(mac, stream, "%g", obj->data.real);
	    break;
	case LispCons_t: {
	    LispObj *car, *cdr;

	    car = CAR(obj);
	    cdr = CDR(obj);
	    if (!cdr || cdr->type == LispNil_t) {
		if (paren)
		    len += LispPrintf(mac, stream, "(");
		len += LispPrintObj(mac, stream, car, car->type == LispCons_t);
		if (paren)
		    len += LispPrintf(mac, stream, ")");
	    }
	    else {
		if (paren)
		    len += LispPrintf(mac, stream, "(");
		LispPrintObj(mac, stream, car, car->type == LispCons_t);
		if (cdr->type == LispQuote_t) {
		    len += LispPrintf(mac, stream, " QUOTE ");
		    len += LispPrintObj(mac, stream, cdr->data.quote, 0);
		}
		else if (cdr->type != LispCons_t) {
		    len += LispPrintf(mac, stream, " . ");
		    len += LispPrintObj(mac, stream, cdr, 0);
		}
		else {
		    len += LispPrintf(mac, stream, " ");
		    len += LispPrintObj(mac, stream, cdr,
					car->type != LispCons_t &&
					cdr->type != LispCons_t);
		}
		if (paren)
		    len += LispPrintf(mac, stream, ")");
	    }
	}    break;
	case LispQuote_t:
	    len += LispPrintf(mac, stream, "'");
	    len += LispPrintObj(mac, stream, obj->data.quote, 1);
	    break;
	case LispArray_t:
	    if (obj->data.array.rank == 1)
		len += LispPrintf(mac, stream, "#(");
	    else
		len += LispPrintf(mac, stream, "#%dA(", obj->data.array.rank);

	    if (!obj->data.array.zero) {
		if (obj->data.array.rank == 1) {
		    LispObj *ary;
		    long count;

		    for (ary = obj->data.array.dim, count = 1;
			 ary != NIL; ary = CDR(ary))
			count *= (int)CAR(ary)->data.real;
		    for (ary = obj->data.array.list; count > 0;
			 ary = CDR(ary), count--) {
			len += LispPrintObj(mac, stream, CAR(ary), 0);
			if (count - 1 > 0)
			    len += LispPrintf(mac, stream, " ");
		    }
		}
		else {
		    LispObj *ary;
		    int i, k, rank, *dims, *loop;

		    rank = obj->data.array.rank;
		    dims = LispMalloc(mac, sizeof(int) * rank);
		    loop = LispCalloc(mac, 1, sizeof(int) * (rank - 1));

		    /* fill dim */
		    for (i = 0, ary = obj->data.array.dim; ary != NIL;
			 i++, ary = CDR(ary))
			dims[i] = (int)CAR(ary)->data.real;

		    i = 0;
		    ary = obj->data.array.list;
		    while (loop[0] < dims[0]) {
			for (; i < rank - 1; i++)
			    len += LispPrintf(mac, stream, "(");
			--i;
			for (;;) {
			    ++loop[i];
			    if (i && loop[i] >= dims[i])
				loop[i] = 0;
			    else
				break;
			    --i;
			}
			for (k = 0; k < dims[rank - 1] - 1; k++, ary = CDR(ary)) {
			    len += LispPrintObj(mac, stream, CAR(ary), 1);
			    len += LispPrintf(mac, stream, " ");
			}
			len += LispPrintObj(mac, stream, CAR(ary), 0);
			ary = CDR(ary);
			for (k = rank - 1; k > i; k--)
			    len += LispPrintf(mac, stream, ")");
			if (loop[0] < dims[0])
			    len += LispPrintf(mac, stream, " ");
		    }
		    LispFree(mac, dims);
		    LispFree(mac, loop);
		}
	    }
	    len += LispPrintf(mac, stream, ")");
	    break;
	case LispStruct_t: {
	    LispObj *def = obj->data.struc.def;
	    LispObj *field = obj->data.struc.fields;

	    len += LispPrintf(mac, stream, "S#(%s", CAR(def)->data.atom);
	    def = CDR(def);
	    for (; def != NIL; def = CDR(def), field = CDR(field)) {
		len += LispPrintf(mac, stream, " :%s ",
				  CAR(def)->type == LispAtom_t ?
				      CAR(def)->data.atom :
				      CAR(CAR(def))->data.atom);
		len += LispPrintObj(mac, stream, CAR(field), 1);
	    }
	    len += LispPrintf(mac, stream, ")");
	}   break;
	case LispSymbol_t:
	    len += LispPrintf(mac, stream, "<#SYMBOL# %s ",
			      obj->data.symbol.name);
	    len += LispPrintObj(mac, stream, obj->data.symbol.obj, paren);
	    len += LispPrintf(mac, stream, ">");
	    break;
	case LispLambda_t:
	    switch (obj->data.lambda.type) {
		case LispLambda:
		    len += LispPrintf(mac, stream, "<#LAMBDA# ");
		    break;
		case LispFunction:
		    len += LispPrintf(mac, stream, "<#FUNCTION# %s ",
				      obj->data.lambda.name->data.atom);
		    break;
		case LispMacro:
		    len += LispPrintf(mac, stream, "<#MACRO# %s ",
				      obj->data.lambda.name->data.atom);
		    break;
	    }
	    len += LispPrintObj(mac, stream, obj->data.lambda.code, 1);
	    len += LispPrintf(mac, stream, ">");
	    break;
	case LispStream_t:
	    if (obj->data.stream.size < 0)
		len += LispPrintf(mac, stream, "<#STREAM# 0x%8x>",
				  (int)obj->data.stream.source.fp);
	    else {
		if (mac->princ)
		    len += LispPrintf(mac, stream, "%s",
				      obj->data.stream.source.str);
		else
		    len += LispPrintf(mac, stream, "\"%s\"",
				      obj->data.stream.source.str ?
					(char*)obj->data.stream.source.str : "");
	    }
	    break;
    }

    return (len);
}

void
LispPrint(LispMac *mac, LispObj *obj, int newline)
{
    if (!obj)
	LispDestroy(mac, "internal error, at INTERNAL:PRINT");
    if (newline && !mac->newline) {
	LispPrintf(mac, NIL, "\n");
	mac->column = 0;
    }
    /* XXX maybe should check for newlines in object */
    mac->column = LispPrintObj(mac, NIL, obj, 1);
    mac->newline = 0;
    fflush(lisp_stdout);
}

void
LispUpdateResults(LispMac *mac, LispObj *cod, LispObj *res)
{
    GCProtect();
    LispSetVar(mac, RUN[2]->data.atom,
	       LispGetVar(mac, RUN[1]->data.atom, 0), 0);
    LispSetVar(mac, RUN[1]->data.atom,
	       LispGetVar(mac, RUN[0]->data.atom, 0), 0);
    LispSetVar(mac, RUN[0]->data.atom, cod, 0);

    LispSetVar(mac, RES[2]->data.atom,
	       LispGetVar(mac, RES[1]->data.atom, 0), 0);
    LispSetVar(mac, RES[1]->data.atom,
	       LispGetVar(mac, RES[0]->data.atom, 0), 0);
    LispSetVar(mac, RES[0]->data.atom, res, 0);
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
	LispDestroy(global_mac, "Floating point exception");
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
	LispDestroy(global_mac, "Floating point exception");
}
#endif

void
LispMachine(LispMac *mac)
{
    LispObj *cod, *obj;

    /*CONSTCOND*/
    while (1) {
	if (setjmp(mac->jmp) == 0) {
	    global_mac = mac;
	    mac->sigint = signal(SIGINT, LispAbortSignal);
	    mac->sigfpe = signal(SIGFPE, LispFPESignal);
	    if (mac->interactive && mac->prompt) {
		fprintf(lisp_stdout, "%s", mac->prompt);
		fflush(lisp_stdout);
	    }
	    mac->level = 0;
	    if ((cod = LispRun(mac)) != NULL) {
		obj = EVAL(cod);
		if (mac->interactive) {
		    LispPrint(mac, obj, 1);
		    LispUpdateResults(mac, cod, obj);
		    if (!mac->newline) {
			LispPrintf(mac, NIL, "\n");
			mac->newline = 1;
			mac->column = 0;
		    }
		}
	    }
	    global_mac = NULL;
	    signal(SIGINT, mac->sigint);
	    signal(SIGFPE, mac->sigfpe);
	    mac->sigint = NULL;
	    LispTopLevel(mac);
	    if (mac->tok == EOF)
		break;
	}
    }
}

void
LispExecute(LispMac *mac, char *str)
{
    int level;
    LispObj *obj;

    if (str == NULL || *str == '\0')
	return;

    if (mac->stream.stream_level + 1 >= mac->stream.stream_size) {
	LispStream *stream = (LispStream*)
	    realloc(mac->stream.stream, sizeof(LispStream) *
		    (mac->stream.stream_size + 1));

	if (stream == NULL) {
	    fprintf(lisp_stderr, "out of memory");
	    return;
	}

	mac->stream.stream = stream;
	++mac->stream.stream_size;
    }
    mac->stream.stream[mac->stream.stream_level].fp = mac->fp;
    mac->stream.stream[mac->stream.stream_level].st = mac->st;
    mac->stream.stream[mac->stream.stream_level].cp = mac->cp;
    mac->stream.stream[mac->stream.stream_level].tok = mac->tok;
    ++mac->stream.stream_level;
    memset(mac->stream.stream + mac->stream.stream_level, 0, sizeof(LispStream));
    mac->stream.stream[mac->stream.stream_level].fp = NULL;
    mac->fp = NULL;
    mac->st = mac->cp = LispStrdup(mac, str);
    mac->tok = 0;

    level = mac->level;
    mac->level = 0;

    if (setjmp(mac->jmp) == 0) {
	/*CONSTCOND*/
	while (1) {
	    if ((obj = LispRun(mac)) != NULL) {
		GCProtect();
		(void)EVAL(obj);
		GCUProtect();
	    }
	    if (mac->tok == EOF)
		break;
	}

	LispFree(mac, mac->st);
	mac->level = level;
	--mac->stream.stream_level;

	mac->fp = mac->stream.stream[mac->stream.stream_level].fp;
	mac->st = mac->stream.stream[mac->stream.stream_level].st;
	mac->cp = mac->stream.stream[mac->stream.stream_level].cp;
	mac->tok = mac->stream.stream[mac->stream.stream_level].tok;
    }
}

LispMac *
LispBegin(int argc, char *argv[])
{
    int i;
    char results[4];
    char *fname = "INTERNAL:BEGIN";
    LispMac *mac = malloc(sizeof(LispMac));

    if (mac == NULL)
	return (NULL);

    if (lisp_stdin == NULL)
	lisp_stdin = fdopen(0, "r");
    if (lisp_stdout == NULL)
	lisp_stdout = fdopen(1, "w");
    if (lisp_stderr == NULL)
	lisp_stderr = fdopen(2, "w");

    pagesize = getpagesize();
    segsize = pagesize / sizeof(LispObj);
    bzero(mac, sizeof(LispMac));
    MOD = ENV = GLB = SYM = LEX = COD = FUN = FRM = STR = DBG = BRK = NIL;
    LispAllocSeg(mac);

    /* initialize stream management */
    mac->stream.stream = (LispStream*)calloc(1, sizeof(LispStream));
    if (argc > 1) {
	i = 1;

	if (strcmp(argv[1], "-d") == 0) {
	    mac->debugging = 1;
	    mac->debug_level = -1;
	    ++i;
	}
	if (i < argc &&
	    (mac->stream.stream[0].fp = mac->fp = fopen(argv[i], "r")) == NULL) {
	    fprintf(lisp_stderr, "Cannot open %s.\n", argv[i]);
	    exit(1);
	}
    }
    if (mac->fp == NULL) {
	mac->stream.stream[0].fp = mac->fp = lisp_stdin;
	mac->interactive = 1;
    }
    mac->stream.stream_size = 1;

    /* initialize memory management */
    mac->mem.mem = (void**)calloc(mac->mem.mem_size = 16, sizeof(void*));
    mac->mem.mem_level = 0;

    mac->prompt = "> ";
    mac->newline = 1;
    mac->column = 0;

    mac->errexit = !mac->interactive;

    if (mac->interactive) {
	/* add +, ++, +++, *, **, and *** */
	for (i = 0; i < 3; i++) {
	    results[i] = '+';
	    results[i + 1] = '\0';
	    RUN[i] = ATOM2(results);
	    _LispSet(mac, RUN[i], NIL, fname, 0);
	}
	for (i = 0; i < 3; i++) {
	    results[i] = '*';
	    results[i + 1] = '\0';
	    RES[i] = ATOM2(results);
	    _LispSet(mac, RES[i], NIL, fname, 0);
	}
    }
    else
	RUN[0] = RUN[1] = RUN[2] = RES[0] = RES[1] = RES[2] = NIL;

    return (mac);
}

void
LispEnd(LispMac *mac)
{
    if (mac->fp != lisp_stdin)
	fclose(mac->fp);
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

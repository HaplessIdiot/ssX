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

/* $XFree86: xc/programs/xedit/lisp/lisp.c,v 1.4 2001/09/09 23:03:47 paulo Exp $ */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>

#include "private.h"

#include "core.h"
#include "format.h"
#include "require.h"
#include "struct.h"
#include "time.h"

/*
 * Prototypes
 */
LispObj *LispRunFunMac(LispMac*, LispObj*, LispObj*, LispFunType);
void LispTopLevel(LispMac*);

static int LispGet(LispMac*);
static int LispUnget(LispMac*);
static int LispSkipComment(LispMac*);
static int LispSkipWhiteSpace(LispMac*);
static char *LispIntToOpaqueType(LispMac*, int);

void LispSnprint(LispMac*, LispObj*, char*, int);
void LispSnprintObj(LispMac*, LispObj*, char**, int*, int);

void LispCheckMemLevel(LispMac*);

void LispAllocSeg(LispMac*);
void LispMark(LispObj*);
LispObj *LispAddVar(LispMac*, char*, LispObj*);

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
#if 0
    while (mac->mem.mem_level)
	free(mac->mem.mem[--mac->mem.mem_level]);
#endif
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
    gcpro = 0;
    mac->block.block_level = 0;
    mac->setf = NULL;
    mac->cdr = mac->princ = mac->justsize = 0;
    if (mac->stream.stream_level) {
	if (mac->stream.stream[mac->stream.stream_level].fp) {
	    /* i.e. if not called from LispExecute */
	    free(mac->st);
	    fclose(mac->stream.stream[mac->stream.stream_level].fp);
	}
	--mac->stream.stream_level;	/* the current stream buffer is in mac->st */
	while (mac->stream.stream_level) {
	    fclose(mac->stream.stream[mac->stream.stream_level].fp);
	    free(mac->stream.stream[mac->stream.stream_level].st);
	    --mac->stream.stream_level;
	}
	mac->fp = mac->stream.stream[0].fp;
	mac->st = mac->stream.stream[0].st;
	mac->cp = mac->stream.stream[0].cp;
	mac->tok = mac->stream.stream[0].tok;
    }
    mac->mem.mem_level = 0;

    fflush(lisp_stdout);
    fflush(lisp_stderr);
}

void
LispGC(LispMac *mac, LispObj *car, LispObj *cdr)
{
    LispObj *entry;
    unsigned i, j;
#ifdef DEBUG
    struct timeval start, end;
    long sec, msec;
    int count = nfree;
#endif

    if (gcpro)
	return;

#ifdef DEBUG
    fprintf(lisp_stdout, "gc: ");
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
    LispMark(car);
    LispMark(cdr);

    for (j = 0; j < numseg; j++)
	for (i = 0, entry = objseg[j]; i < segsize; i++, entry++) {
	    if (entry->prot)
		LispMark(entry);
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
#ifdef DEBUG
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    msec = end.tv_usec - start.tv_usec;
    if (msec < 0) {
	--sec;
	msec += 1000000;
    }
    fprintf(lisp_stdout, "%ld sec, %ld msec, ", sec, msec);
    fprintf(lisp_stdout, "%d recovered, %d free, %d protected, %d total\n", nfree - count, nfree, nobjs - nfree, nobjs);
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
	mac->mem.mem[mac->mem.mem_level++] = pointer;
    else
	mac->mem.mem[i] = pointer;

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
	memcpy(mac->mem.mem + + i, mac->mem.mem + i + 1,
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
    opaque->desc = LispGetString(mac, desc);
    opaque->next = mac->opqs[ii];
    mac->opqs[ii] = opaque;

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

    return ("nil");
}

char *
LispGetString(LispMac *mac, char *str)
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
	if (strcmp(string->string, str) == 0)
	    return (string->string);
    string = (LispString*)LispMalloc(mac, sizeof(LispString));
    string->string = LispStrdup(mac, str);
    string->next = mac->strs[ii];
    mac->strs[ii] = string;

    return (string->string);
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
		LispDestroy(mac, "internal error, at internal:new");
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
    LispObj *atom = LispNew(mac, NIL, NIL);

    atom->type = LispAtom_t;
    atom->data.atom = LispGetString(mac, str);

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
LispNewLambda(LispMac *mac, char *name, LispObj *args, LispObj *code, 
	      unsigned num_args, LispFunType type)
{
    LispObj *fun = LispNew(mac, args, code);

    fun->type = LispLambda_t;
    fun->data.lambda.name = name;
    GCProtect();
    fun->data.lambda.code = CONS(args, code);
    GCUProtect();
    fun->data.lambda.num_args = num_args;
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
    LispObj *old_env, *old_sym, *env, *res, *list, *pair;

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
	if (!refs) {
	    pair = CONS(var, EVAL(val));
	    GCProtect();
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
	    LispAddVar(mac, var->data.atom, EVAL(val));
    }

    if (!refs && list != NIL) {
	list = LispReverse(list);
	for (; list != NIL; list = CDR(list)) {
	    pair = CAR(list);
	    LispAddVar(mac, CAR(pair)->data.atom, CDR(pair));
	}
	FRM = CDR(FRM);
    }

    res = fn(mac, CDR(args), fname);

    SYM = old_sym;
    ENV = old_env;

    return (res);
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
		str[len++] = ch;
		if (dquote) {
		    if (ch == '\\')
			escape = !escape;
		    else if (ch == '"' && !escape)
			dquote = escape = 0;
		    else
			escape = 0;
		}
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
	    else if (isdigit(str[0]) || str[0] == '-' ||
		     (str[0] == '.' && isdigit(str[1])) || str[0] == '+') {
		double value;
		char *cp;

		value = strtod(str, &cp);
		if (cp && *cp)
		    res = ATOM(str);
		else
		    res = REAL(value);
	    }
	    else {
		if (strcmp(str, "nil") == 0)
		    res = NIL;
		else if (strcmp(str, "t") == 0)
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
    char *name = NULL, stk[32];
    LispObj *fun, *cons = NIL, *frm, *res;
    LispObj *car, *cdr;
    LispBuiltin *fn;
    unsigned num_objs;

    if (!obj)
	LispDestroy(mac, "internal error, at internal:eval");

    switch (obj->type) {
	case LispNil_t:
	case LispTrue_t:
	case LispReal_t:
	case LispString_t:
	case LispOpaque_t:
	    return (obj);
	case LispAtom_t:
	    if (obj->data.atom[0] == ':')
		return (obj);
	    else if ((obj = LispGetVar(mac, name = obj->data.atom, 0)) != NULL)
		return (obj);
	    LispDestroy(mac, "the variable %s is unbound", name);
	    /*NOTREACHED*/
	case LispQuote_t:
	    return (obj->data.quote);
	case LispSymbol_t:
	    return (EVAL(obj->data.symbol.obj));
	case LispCons_t:
	    cons = obj;
	    break;
    }
    car = CAR(cons);
    fun = NIL;
    switch (car->type) {
	case LispAtom_t:
	    name = car->data.atom;
	    break;
	case LispReal_t:
	    (void)snprintf(stk, sizeof(stk), "%g", car->data.real);
	    name = stk;
	    break;
	case LispNil_t:
	    name = "nil";
	    break;
	case LispTrue_t:
	    name = "t";
	    break;
	case LispLambda_t:
	    name = "<lambda>";
	    fun = car;
	    break;
	default:
	    LispDestroy(mac, "%s is invalid as a function, at internal:eval",
			LispStrObj(mac, car));
	    /*NOTREACHED*/
    }

    ++mac->level;
    frm = FRM;

    if (fun == NIL) {
	int len = strlen(name);

	if ((fn = LispFindBuiltin(name, len)) == NULL) {
	    LispModule *module = mac->module;

	    while (module != NULL) {
		if (module->data->find_fun &&
		    (fn = (module->data->find_fun)(name, len)) != NULL)
		    break;
		module = module->next;
	    }

	    if (fn == NULL) {
		LispObj *str;

		/* first check for structure functions */
		if (strncmp("make-", name, 5) == 0) {
		    for (str = STR; str != NIL; str = CDR(str)) {
			if (strcmp(CAR(CAR(str))->data.atom, name + 5) == 0) {
			    mac->struc = CAR(str);
			    fn = &LispMakeStructDef;
			    break;
			}
		    }
		}
		else if (strchr(name, '-')) {
		    char *sname, *ptr;

		    /* check if it is a structure field access function */
		    for (str = STR; str != NIL; str = CDR(str)) {
			ptr = name;
			sname = CAR(CAR(str))->data.atom;
			while (*ptr == *sname) {
			    ++ptr;
			    ++sname;
			}
			if (*sname == '\0' && *ptr == '-') {
			    LispObj *res;

			    ++ptr;
			    if (ptr[0] == 'p' && ptr[1] == '\0') {
				/* Just checking type */
				if (CDR(cons) == NIL)
				    LispDestroy(mac, "too few arguments to %s",
						name);
				else if (CDR(CDR(cons)) != NIL)
				    LispDestroy(mac, "too many arguments to %s",
						name);
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
	    while (cdr != NIL) {
		++num_objs;
		cdr = CDR(cdr);
	    }
	    if ((fn->max_args && num_objs > fn->max_args)
		|| (fn->min_args && num_objs < fn->min_args))
		LispDestroy(mac, "bad number of arguments to %s", name);

	    if (fn->eval && num_objs) {
		LispObj *arg = CDR(cons);

		res = EVAL(CAR(arg));
		args = cdr = CONS(res, NIL);
		FRM = CONS(args, FRM);
		arg = CDR(arg);
		while (arg != NIL) {
		    CDR(cdr) = CONS(EVAL(CAR(arg)), NIL);
		    cdr = CDR(cdr);
		    arg = CDR(arg);
		}
	    }
	    else
		args = CDR(cons);

	    mac->setf = NULL;
	    mac->cdr = 0;
	    res = fn->fn(mac, args, name);
	    FRM = frm;
	    --mac->level;
	    return (res);
	}

	for (fun = FUN; fun != NIL; fun = CDR(fun))
	    if (CAR(fun)->data.lambda.name == name) {
		fun = CAR(fun);
		break;
	    }
    }

    if (fun != NIL) {
	LispObj *args;

	cdr = CDR(cons);
	num_objs = 0;
	while (cdr != NIL) {
	    ++num_objs;
	    cdr = CDR(cdr);
	}
	if (num_objs != fun->data.lambda.num_args)
	    LispDestroy(mac, "%u is a bad number of arguments to %s, needs %u",
			num_objs, name, fun->data.lambda.num_args);

	if (fun->data.lambda.type == LispMacro)
	    args = CDR(cons);
	else if (num_objs) {
	    LispObj *arg = CDR(cons);

	    res = EVAL(CAR(arg));
	    args = cdr = CONS(res, NIL);
	    FRM = CONS(args, FRM);
	    arg = CDR(arg);
	    while (arg != NIL) {
		CDR(cdr) = CONS(EVAL(CAR(arg)), NIL);
		cdr = CDR(cdr);
		arg = CDR(arg);
	    }
	}
	else
	    args = NIL;

	res = LispRunFunMac(mac, fun->data.lambda.code, args,
			    fun->data.lambda.type);
	FRM = frm;
	--mac->level;
	return (res);
    }

    LispDestroy(mac, "the function %s is not defined", name);
    /*NOTREACHED*/

    return (NIL);
}

LispObj *
LispRunFunMac(LispMac *mac, LispObj *fun, LispObj *list, LispFunType type)
{
    LispObj *old_env, *old_sym, *old_lex, *args, *code, *res;

    old_env = ENV;
    old_sym = SYM;
    old_lex = LEX;

    args = CAR(fun);
    code = CDR(fun);

    LEX = ENV;
    SYM = CONS(LEX, SYM);

    for (; args != NIL; args = CDR(args), list = CDR(list))
	LispAddVar(mac, CAR(args)->data.atom, CAR(list));

    res = Lisp_Progn(mac, code,
		     type == LispLambda ? "#<lambda>" : fun->data.lambda.name);
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
	    sz = snprintf(*str, *len, "nil");
	    *len -= sz;
	    *str += sz;
	    break;
	case LispTrue_t:
	    sz = snprintf(*str, *len, "t");
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
		    sz = snprintf(*str, *len, " quote ");
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
	LispDestroy(mac, "internal error, at internal:sprint");
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
	n = vsnprintf(stk, size, fmt, ap);
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
		n = vsnprintf(ptr, size, fmt, ap);
		if (n >= 0 && n < size)
		    break;
	    }
	}
	size = strlen(ptr);

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
	    strcpy(stream->data.stream.source.str + stream->data.stream.idx, ptr);
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
	    len += LispPrintf(mac, stream, "nil");
	    break;
	case LispTrue_t:
	    len += LispPrintf(mac, stream, "t");
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
		    len += LispPrintf(mac, stream, " quote ");
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
	    len += LispPrintf(mac, stream, "<#symbol# %s ",
			      obj->data.symbol.name);
	    len += LispPrintObj(mac, stream, obj->data.symbol.obj, paren);
	    len += LispPrintf(mac, stream, ">");
	    break;
	case LispLambda_t:
	    switch (obj->data.lambda.type) {
		case LispLambda:
		    len += LispPrintf(mac, stream, "<#lambda# ");
		    break;
		case LispFunction:
		    len += LispPrintf(mac, stream, "<#function# %s ",
				      obj->data.lambda.name);
		    break;
		case LispMacro:
		    len += LispPrintf(mac, stream, "<#macro# %s ",
				      obj->data.lambda.name);
		    break;
	    }
	    len += LispPrintObj(mac, stream, obj->data.lambda.code, 1);
	    len += LispPrintf(mac, stream, ">");
	    break;
	case LispStream_t:
	    if (obj->data.stream.size < 0)
		len += LispPrintf(mac, stream, "<#stream# 0x%8x>",
				  (int)obj->data.stream.source.fp);
	    else {
		if (mac->princ)
		    len += LispPrintf(mac, stream, "%s",
				      obj->data.stream.source.str);
		else
		    len += LispPrintf(mac, stream, "\"%s\"",
				      obj->data.stream.source.str ?
					obj->data.stream.source.str : "");
	    }
	    break;
    }

    return (len);
}

void
LispPrint(LispMac *mac, LispObj *obj)
{
    if (!obj)
	LispDestroy(mac, "internal error, at internal:print");
    if (!mac->newline) {
	LispPrintf(mac, NIL, "\n");
	mac->column = 0;
    }
    /* XXX maybe should check for newlines in object */
    mac->column = LispPrintObj(mac, NIL, obj, 1);
    mac->newline = 0;
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
    LispObj *obj;

    /*CONSTCOND*/
    while (1) {
	if (setjmp(mac->jmp) == 0) {
	    global_mac = mac;
	    mac->sigint = signal(SIGINT, LispAbortSignal);
	    mac->sigfpe = signal(SIGFPE, LispFPESignal);
	    if (mac->interactive && mac->prompt)
		fprintf(lisp_stdout, "%s", mac->prompt);
	    mac->level = 0;
	    if ((obj = LispRun(mac)) != NULL) {
		obj = EVAL(obj);
		if (mac->interactive)
		    LispPrint(mac, obj);
		if (!mac->newline) {
		    LispPrintf(mac, NIL, "\n");
		    mac->newline = 1;
		    mac->column = 0;
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
    mac->st = mac->cp = str;
    mac->tok = 0;

    level = mac->level;
    mac->level = 0;

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

    mac->level = level;
    --mac->stream.stream_level;

    mac->fp = mac->stream.stream[mac->stream.stream_level].fp;
    mac->st = mac->stream.stream[mac->stream.stream_level].st;
    mac->cp = mac->stream.stream[mac->stream.stream_level].cp;
    mac->tok = mac->stream.stream[mac->stream.stream_level].tok;
}

LispMac *
LispBegin(int argc, char *argv[])
{
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
    MOD = ENV = GLB = SYM = LEX = COD = FUN = FRM = STR = NIL;
    LispAllocSeg(mac);

    /* initialize stream management */
    mac->stream.stream = (LispStream*)calloc(1, sizeof(LispStream));
    if (argc > 1) {
	if ((mac->stream.stream[0].fp = mac->fp = fopen(argv[1], "r")) == NULL) {
	    fprintf(lisp_stderr, "Cannot open %s.\n", argv[1]);
	    exit(1);
	}
    }
    else {
	mac->stream.stream[0].fp = mac->fp = lisp_stdin;
	mac->interactive = 1;
    }
    mac->stream.stream_size = 1;

    /* initialize memory management */
    mac->mem.mem = (void**)calloc(mac->mem.mem_size = 16, sizeof(void*));
    mac->mem.mem_level = 0;

    mac->prompt = ">";
    mac->newline = 1;
    mac->column = 0;

    mac->errexit = !mac->interactive;

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

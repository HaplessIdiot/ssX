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

/* $XFree86: xc/programs/xedit/lisp/debugger.c,v 1.18 2002/06/03 21:39:23 paulo Exp $ */

#include <ctype.h>
#include "io.h"
#include "debugger.h"
#include "write.h"

#ifdef DEBUGGER
#define DebuggerHelp		0
#define DebuggerAbort		1
#define DebuggerBacktrace	2
#define DebuggerContinue	3
#define DebuggerFinish		4
#define DebuggerFrame		5
#define DebuggerNext		6
#define DebuggerPrint		7
#define DebuggerStep		8
#define DebuggerBreak		9
#define DebuggerDelete		10
#define DebuggerDown		11
#define DebuggerUp		12
#define DebuggerInfo		13
#define DebuggerWatch		14

#define DebuggerInfoBreakpoints	0
#define DebuggerInfoBacktrace	1

/*
 * Prototypes
 */
static char *format_integer(int);
static void LispDebuggerCommand(LispMac*, LispObj *obj);

/*
 * Initialization
 */
static struct {
    char *name;
    int action;
} commands[] = {
    {"help",		DebuggerHelp},
    {"abort",		DebuggerAbort},
    {"backtrace",	DebuggerBacktrace},
    {"b",		DebuggerBreak},
    {"break",		DebuggerBreak},
    {"bt",		DebuggerBacktrace},
    {"continue",	DebuggerContinue},
    {"d",		DebuggerDelete},
    {"delete",		DebuggerDelete},
    {"down",		DebuggerDown},
    {"finish",		DebuggerFinish},
    {"frame",		DebuggerFrame},
    {"info",		DebuggerInfo},
    {"n",		DebuggerNext},
    {"next",		DebuggerNext},
    {"print",		DebuggerPrint},
    {"run",		DebuggerContinue},
    {"s",		DebuggerStep},
    {"step",		DebuggerStep},
    {"up",		DebuggerUp},
    {"watch",		DebuggerWatch},
};

static struct {
    char *name;
    int subaction;
} info_commands[] = {
    {"breakpoints",	DebuggerInfoBreakpoints},
    {"stack",		DebuggerInfoBacktrace},
    {"watchpoints",	DebuggerInfoBreakpoints},
};

static char debugger_help[] =
"Available commands are:\n\
\n\
help		- This message.\n\
abort		- Abort the current execution, and return to toplevel.\n\
backtrace, bt	- Print backtrace.\n\
b, break	- Set breakpoint at function name argument.\n\
continue	- Continue execution.\n\
d, delete	- Delete breakpoint(s), all breakpoint if no arguments given.\n\
down		- Set environment to frame called by the current one.\n\
finish		- Executes until current form is finished.\n\
frame		- Set environment to selected frame.\n\
info		- Prints information about the debugger state.\n\
n, next		- Evaluate next form.\n\
print		- Print value of variable name argument.\n\
run		- Continue execution.\n\
s, step		- Evaluate next form, stopping on any subforms.\n\
up		- Set environment to frame that called the current one.\n\
\n\
Commands may be abbreviated.\n";

static char debugger_info_help[] =
"Available subcommands are:\n\
\n\
breakpoints	- List and prints status of breakpoints, and watchpoints.\n\
stack		- Backtrace of stack.\n\
watchpoints	- List and prints status of watchpoints, and breakpoints.\n\
\n\
Subcommands may be abbreviated.\n";

/* Debugger variables layout (if you change it, update description):
 *
 * DBG
 *	is a macro for mac->dbglist
 *	is a NIL terminated list
 *	every element is a list in the format (NOT NIL terminated):
 *	(list* NAM ARG ENV HED LEX)
 *	where
 *		NAM is an ATOM for the function/macro name
 *		    or NIL for lambda expressions
 *		ARG is NAM arguments (a LIST)
 *		ENV is the value of mac->stack.base (a INTEGER)
 *		LEN is the value of mac->env.length (a INTEGER)
 *		LEX is the value of mac->env.lex (a INTEGER)
 *	new elements are added to the beggining of the DBG list
 *
 * BRK
 *	is macro for mac->brklist
 *	is a NIL terminated list
 *	every element is a list in the format (NIL terminated):
 *	(list NAM IDX TYP HIT VAR VAL FRM)
 *	where
 *		NAM is an ATOM for the name of the object at
 *		    wich the breakpoint was added
 *		IDX is a INTEGER, the breakpoint number
 *		    must be stored, as breakpoints may be deleted
 *		TYP is a INTEGER that must be an integer of enum LispBreakType
 *		HIT is a INTEGER, with the number of times this breakpoint was
 *		    hitted.
 *		VAR variable to watch a SYMBOL	(not needed for breakpoints)
 *		VAL value of watched variable	(not needed for breakpoints)
 *		FRM frame where variable started being watched
 *						(not needed for breakpoints)
 *	new elements are added to the end of the list
 */

/*
 * Implementation
 */
void
LispDebugger(LispMac *mac, LispDebugCall call, LispObj *name, LispObj *arg)
{
    int force = 0;
    LispObj *obj, *prev;

    switch (call) {
	case LispDebugCallBegin:
	    ++mac->debug_level;
	    GCProtect();
	    DBG = CONS(CONS(name, CONS(arg, CONS(SMALLINT(mac->stack.base),
		       CONS(SMALLINT(mac->env.length),
			    SMALLINT(mac->env.lex))))), DBG);
	    GCUProtect();
	    for (obj = BRK; obj != NIL; obj = CDR(obj))
		if (ATOMID(CAR(CAR(obj))) == ATOMID(name) &&
		    CAR(CDR(CDR(CAR(obj))))->data.integer == LispDebugBreakFunction)
		    break;
	    if (obj != NIL) {
		/* if not at a fresh line */
		if (LispGetColumn(mac, NIL))
		    LispFputc(Stdout, '\n');
		LispFputs(Stdout, "BREAK #");
		LispDoWriteObject(mac, NIL, CAR(CDR(CAR(obj))), 1);
		LispFputs(Stdout, "> (");
		LispDoWriteObject(mac, NIL, CAR(CAR(DBG)), 1);
		LispFputc(Stdout, ' ');
		LispDoWriteObject(mac, NIL, CAR(CDR(CAR(DBG))), 0);
		LispFputs(Stdout, ")\n");
		force = 1;
		/* update hits counter */
		CAR(CDR(CDR(CDR(CAR(obj)))))->data.integer += 1;
	    }
	    break;
	case LispDebugCallEnd:
	    DBG = CDR(DBG);
	    if (mac->debug_level < mac->debug_step)
		mac->debug_step = mac->debug_level;
	    --mac->debug_level;
	    break;
	case LispDebugCallFatal:
	    LispDebuggerCommand(mac, NIL);
	    return;
	case LispDebugCallWatch:
	    break;
    }

    /* didn't return, check watchpoints */
    if (call == LispDebugCallEnd || call == LispDebugCallWatch) {
watch_again:
	for (prev = obj = BRK; obj != NIL; prev = obj, obj = CDR(obj)) {
	    if (CAR(CDR(CDR(CAR(obj))))->data.integer == LispDebugBreakVariable) {
		/* the variable */
		LispObj *wat = CAR(CDR(CDR(CDR(CDR(CAR(obj))))));
		void *sym = LispGetVarAddr(mac, CAAR(obj));
		LispObj *frm = CAR(CDR(CDR(CDR(CDR(CDR(CDR(CAR(obj))))))));

		if ((sym == NULL && mac->debug_level <= 0) ||
		    (sym != wat->data.opaque.data &&
		     frm->data.integer > mac->debug_level)) {
		    LispFputs(Stdout, "WATCH #");
		    LispFputs(Stdout, format_integer(CAR(CDR(CAR(obj)))->data.integer));
		    LispFputs(Stdout, "> ");
		    LispFputs(Stdout, STRPTR(CAR(CAR(obj))));
		    LispFputs(Stdout, " deleted. Variable does not exist anymore.\n");
		    /* force debugger to stop */
		    force = 1;
		    if (obj == prev) {
			BRK = CDR(BRK);
			goto watch_again;
		    }
		    else
			CDR(prev) = CDR(obj);
		    obj = prev;
		}
		else {
		    /* current value */
		    LispObj *cur = *(LispObj**)wat->data.opaque.data;
		    /* last value */
		    LispObj *val = CAR(CDR(CDR(CDR(CDR(CDR(CAR(obj)))))));
		    if (XEQUAL(val, cur) == NIL) {
			LispFputs(Stdout, "WATCH #");
			LispFputs(Stdout, format_integer(CAR(CDR(CAR(obj)))->data.integer));
			LispFputs(Stdout, "> ");
			LispFputs(Stdout, STRPTR(CAR(CAR(obj))));
			LispFputc(Stdout, '\n');

			LispFputs(Stdout, "OLD: ");
			LispDoWriteObject(mac, NIL, val, 1);

			LispFputs(Stdout, "\nNEW: ");
			LispDoWriteObject(mac, NIL, cur, 1);
			LispFputc(Stdout, '\n');

			/* update current value */
			CAR(CDR(CDR(CDR(CDR(CDR(CAR(obj))))))) = cur;
			/* update hits counter */
			CAR(CDR(CDR(CDR(CAR(obj)))))->data.integer += 1;
			/* force debugger to stop */
			force = 1;
		    }
		}
	    }
	}

	if (call == LispDebugCallWatch)
	    /* special call, just don't keep gc protected variables that may be
	     * using a lot of memory... */
	    return;
    }

    switch (mac->debug) {
	case LispDebugUnspec:
	    LispDebuggerCommand(mac, NIL);
	    goto debugger_done;
	case LispDebugRun:
	    if (force)
		LispDebuggerCommand(mac, NIL);
	    goto debugger_done;
	case LispDebugFinish:
	    if (!force &&
		(call != LispDebugCallEnd || mac->debug_level != mac->debug_step))
		goto debugger_done;
	    break;
	case LispDebugNext:
	    if (call == LispDebugCallBegin) {
		if (!force && mac->debug_level != mac->debug_step)
		    goto debugger_done;
	    }
	    else if (call == LispDebugCallEnd) {
		if (!force && mac->debug_level >= mac->debug_step)
		    goto debugger_done;
	    }
	    break;
	case LispDebugStep:
	    break;
    }

    if (call == LispDebugCallBegin) {
	LispFputc(Stdout, '#');
	LispFputs(Stdout, format_integer(mac->debug_level));
	LispFputs(Stdout, "> (");
	LispDoWriteObject(mac, NIL, CAR(CAR(DBG)), 1);
	LispFputc(Stdout, ' ');
	LispDoWriteObject(mac, NIL, CAR(CDR(CAR(DBG))), 0);
	LispFputs(Stdout, ")\n");
	LispDebuggerCommand(mac, NIL);
    }
    else if (call == LispDebugCallEnd) {
	LispFputc(Stdout, '#');
	LispFputs(Stdout, format_integer(mac->debug_level + 1));
	LispFputs(Stdout, "= ");
	LispDoWriteObject(mac, NIL, arg, 1);
	LispFputc(Stdout, '\n');
	LispDebuggerCommand(mac, NIL);
    }
    else if (force)
	LispDebuggerCommand(mac, arg);

debugger_done:
    return;
}

static void
LispDebuggerCommand(LispMac *mac, LispObj *args)
{
    LispObj *obj, *frm, *curframe;
    int i = 0, frame, matches, action = -1, subaction = 0;
    char *cmd, *arg, *ptr, line[256];

    int envbase = mac->stack.base, envlen = mac->env.length, envlex = mac->env.lex;

    frame = mac->debug_level;
    curframe = CAR(DBG);

    line[0] = '\0';
    arg = line;
    for (;;) {
	LispFputs(Stdout, DBGPROMPT);
	LispFflush(Stdout);
	if (LispFgets(Stdin, line, sizeof(line)) == NULL) {
	    LispFputc(Stdout, '\n');
	    return;
	}
	/* get command */
	ptr = line;
	while (*ptr && isspace(*ptr))
	    ++ptr;
	cmd = ptr;
	while (*ptr && !isspace(*ptr))
	    ++ptr;
	if (*ptr)
	    *ptr++ = '\0';

	if (*cmd) {	/* if *cmd is nul, then arg may be still set */
	    /* get argument(s) */
	    while (*ptr && isspace(*ptr))
		++ptr;
	    arg = ptr;
	    /* goto end of line */
	    if (*ptr) {
		while (*ptr)
		    ++ptr;
		--ptr;
		while (*ptr && isspace(*ptr))
		    --ptr;
		if (*ptr)
		    *++ptr = '\0';
	    }
	}

	if (*cmd == '\0') {
	    if (action < 0) {
		if (mac->debug == LispDebugFinish)
		    action = DebuggerFinish;
		else if (mac->debug == LispDebugNext)
		    action = DebuggerNext;
		else if (mac->debug == LispDebugStep)
		    action = DebuggerStep;
		else if (mac->debug == LispDebugRun)
		    action = DebuggerContinue;
		else
		    continue;
	    }
	}
	else {
	    for (i = matches = 0; i < sizeof(commands) / sizeof(commands[0]);
		 i++) {
		char *str = commands[i].name;

		ptr = cmd;
		while (*ptr && *ptr == *str) {
		    ++ptr;
		    ++str;
		}
		if (*ptr == '\0') {
		    action = commands[i].action;
		    if (*str == '\0') {
			matches = 1;
			break;
		    }
		    ++matches;
		}
	    }
	    if (matches == 0) {
		LispFputs(Stdout, "* Command unknown: ");
		LispFputs(Stdout, cmd);
		LispFputs(Stdout, ". Type help for help.\n");
		continue;
	    }
	    else if (matches > 1) {
		LispFputs(Stdout, "* Command is ambiguous: ");
		LispFputs(Stdout, cmd);
		LispFputs(Stdout, ". Type help for help.\n");
		continue;
	    }
	}

	switch (action) {
	    case DebuggerHelp:
		LispFputs(Stdout, debugger_help);
		break;
	    case DebuggerInfo:
		if (*arg == '\0') {
		    LispFputs(Stdout, debugger_info_help);
		    break;
		}

		for (i = matches = 0;
		     i < sizeof(info_commands) / sizeof(info_commands[0]);
		     i++) {
		    char *str = info_commands[i].name;

		    ptr = arg;
		    while (*ptr && *ptr == *str) {
			++ptr;
			++str;
		    }
		    if (*ptr == '\0') {
			subaction = info_commands[i].subaction;
			if (*str == '\0') {
			    matches = 1;
			    break;
			}
			++matches;
		    }
		}
		if (matches == 0) {
		    LispFputs(Stdout, "* Command unknown: ");
		    LispFputs(Stdout, arg);
		    LispFputs(Stdout, ". Type info for help.\n");
		    continue;
		}
		else if (matches > 1) {
		    LispFputs(Stdout, "* Command is ambiguous: ");
		    LispFputs(Stdout, arg);
		    LispFputs(Stdout, ". Type info for help.\n");
		    continue;
		}

		switch (subaction) {
		    case DebuggerInfoBreakpoints:
			LispFputs(Stdout, "Num\tHits\tType\t\tWhat\n");
			for (obj = BRK; obj != NIL; obj = CDR(obj)) {
			    /* breakpoint number */
			    LispFputc(Stdout, '#');
			    LispDoWriteObject(mac, NIL, CAR(CDR(CAR(obj))), 1);

			    /* number of hits */
			    LispFputc(Stdout, '\t');
			    LispDoWriteObject(mac, NIL,
					     CAR(CDR(CDR(CDR(CAR(obj))))), 1);

			    /* breakpoint type */
			    LispFputc(Stdout, '\t');
			    switch ((int)CAR(CDR(CDR(CAR(obj))))->data.integer) {
				case LispDebugBreakFunction:
				    LispFputs(Stdout, "Function");
				    break;
				case LispDebugBreakVariable:
				    LispFputs(Stdout, "Variable");
				    break;
			    }

			    /* breakpoint object */
			    LispFputc(Stdout, '\t');
			    LispDoWriteObject(mac, NIL, CAR(CAR(obj)), 1);
			    LispFputc(Stdout, '\n');
			}
			break;
		    case DebuggerInfoBacktrace:
			goto debugger_print_backtrace;
		}
		break;
	    case DebuggerAbort:
		while (mac->mem.level) {
		    --mac->mem.level;
		    if (mac->mem.mem[mac->mem.level])
			free(mac->mem.mem[mac->mem.level]);
		}
		mac->mem.index = 0;
		LispTopLevel(mac);
		if (!mac->running) {
		    LispMessage(mac, "*** Fatal: nowhere to longjmp.");
		    abort();
		}
		siglongjmp(mac->jmp, 1);/* don't need to restore environment */
		/*NOTREACHED*/
		break;
	    case DebuggerBreak:
		for (ptr = arg; *ptr; ptr++) {
		    if (isspace(*ptr))
			break;
		    else
			*ptr = toupper(*ptr);
		}

		if (!*arg || *ptr || strchr(arg, '(') || strchr(arg, '(') ||
		    strchr(arg, ';')) {
		    LispFputs(Stdout, "* Bad function name '");
		    LispFputs(Stdout, arg);
		    LispFputs(Stdout, "' specified.\n");
		}
		else {
		    for (obj = frm = BRK; obj != NIL; frm = obj, obj = CDR(obj))
			;
		    i = mac->debug_break;
		    ++mac->debug_break;
		    GCProtect();
		    obj = CONS(ATOM(arg),
			       CONS(SMALLINT(i),
				    CONS(SMALLINT(LispDebugBreakFunction),
					 CONS(SMALLINT(0), NIL))));
		    if (BRK == NIL)
			BRK = CONS(obj, NIL);
		    else
			CDR(frm) = CONS(obj, NIL);
		    GCUProtect();
		}
		break;
	    case DebuggerWatch: {
		void *sym;
		int vframe;
		LispObj *val, *atom;

		/* make variable name uppercase, an ATOM */
		ptr = arg;
		while (*ptr) {
		    *ptr = toupper(*ptr);
		    ++ptr;
		}
		atom = ATOM(arg);
		val = LispGetVar(mac, atom);
		if (val == NULL) {
		    LispFputs(Stdout, "* No variable named '");
		    LispFputs(Stdout, arg);
		    LispFputs(Stdout, "' in the selected frame.\n");
		    break;
		}

		/* variable is available at the current frame */
		sym = LispGetVarAddr(mac, atom);

		/* find the lowest frame where the variable is visible */
		vframe = 0;
		if (frame > 0) {
		    for (; vframe < frame; vframe++) {
			for (frm = DBG, i = mac->debug_level; i > vframe;
			     frm = CDR(frm), i--)
			    ;
			obj = CAR(frm);
			mac->stack.base = CAR(CDR(CDR(obj)))->data.integer;
			mac->env.length = CAR(CDR(CDR(CDR(obj))))->data.integer;
			mac->env.lex = CDR(CDR(CDR(CDR(obj))))->data.integer;

			if (LispGetVarAddr(mac, atom) == sym)
			    /* got variable initial frame */
			    break;
		    }
		    vframe = i;
		    if (vframe != frame) {
			/* restore environment */
			for (frm = DBG, i = mac->debug_level; i > frame;
			     frm = CDR(frm), i--)
			    ;
			obj = CAR(frm);
			mac->stack.base = CAR(CDR(CDR(obj)))->data.integer;
			mac->env.length = CAR(CDR(CDR(CDR(obj))))->data.integer;
			mac->env.lex = CDR(CDR(CDR(CDR(obj))))->data.integer;
		    }
		}

		i = mac->debug_break;
		++mac->debug_break;
		for (obj = frm = BRK; obj != NIL; frm = obj, obj = CDR(obj))
		    ;

		GCProtect();
		obj = CONS(atom,					  /* NAM */
			   CONS(SMALLINT(i),				  /* IDX */
				CONS(SMALLINT(LispDebugBreakVariable),	  /* TYP */
				     CONS(SMALLINT(0),			  /* HIT */
					  CONS(OPAQUE(sym, 0),		  /* VAR */
					       CONS(val,		  /* VAL */
						    CONS(SMALLINT(vframe),/* FRM */
							      NIL)))))));

		/* add watchpoint */
		if (BRK == NIL)
		    BRK = CONS(obj, NIL);
		else
		    CDR(frm) = CONS(obj, NIL);
		GCUProtect();
	    }	break;
	    case DebuggerDelete:
		if (*arg == 0) {
		    int confirm = 0;

		    for (;;) {
			int ch;

			LispFputs(Stdout, "* Delete all breakpoints? (y or n) ");
			LispFflush(Stdout);
			if ((ch = LispFgetc(Stdin)) == '\n')
			    continue;
			while ((i = LispFgetc(Stdin)) != '\n' && i != EOF)
			    ;
			if (tolower(ch) == 'n')
			    break;
			else if (tolower(ch) == 'y') {
			    confirm = 1;
			    break;
			}
		    }
		    if (confirm)
			BRK = NIL;
		}
		else {
		    for (ptr = arg; *ptr;) {
			while (*ptr && isdigit(*ptr))
			    ++ptr;
			if (*ptr && !isspace(*ptr)) {
			    *ptr = '\0';
			    LispFputs(Stdout, "* Bad breakpoint number '");
			    LispFputs(Stdout, arg);
			    LispFputs(Stdout, "' specified.\n");
			    break;
			}
			i = atoi(arg);
			for (obj = frm = BRK; frm != NIL;
			     obj = frm, frm = CDR(frm))
			    if (CAR(CDR(CAR(frm)))->data.integer == i)
				break;
			if (frm == NIL) {
			    LispFputs(Stdout, "* No breakpoint number ");
			    LispFputs(Stdout, arg);
			    LispFputs(Stdout, " available.\n");
			    break;
			}
			if (obj == frm)
			    BRK = CDR(BRK);
			else
			    CDR(obj) = CDR(frm);
			while (*ptr && isspace(*ptr))
			    ++ptr;
			arg = ptr;
		    }
		}
		break;
	    case DebuggerFrame:
		i = -1;
		ptr = arg;
		if (*ptr) {
		    i = 0;
		    while (*ptr && isdigit(*ptr)) {
			i *= 10;
			i += *ptr - '0';
			++ptr;
		    }
		    if (*ptr) {
			LispFputs(Stdout, "* Frame identifier must "
				"be a positive number.\n");
			break;
		    }
		}
		else
		    goto debugger_print_frame;
		if (i >= 0 && i <= mac->debug_level)
		    goto debugger_new_frame;
		LispFputs(Stdout, "* No such frame ");
		LispFputs(Stdout, format_integer(i));
		LispFputs(Stdout, ".\n");
		break;
	    case DebuggerDown:
		if (frame + 1 > mac->debug_level) {
		    LispFputs(Stdout, "* Cannot go down.\n");
		    break;
		}
		i = frame + 1;
		goto debugger_new_frame;
		break;
	    case DebuggerUp:
		if (frame == 0) {
		    LispFputs(Stdout, "* Cannot go up.\n");
		    break;
		}
		i = frame - 1;
		goto debugger_new_frame;
		break;
	    case DebuggerPrint:
		ptr = arg;
		while (*ptr) {
		    *ptr = toupper(*ptr);
		    ++ptr;
		}
		obj = LispGetVar(mac, ATOM(arg));
		if (obj != NULL) {
		    LispDoWriteObject(mac, NIL, obj, 1);
		    LispFputc(Stdout, '\n');
		}
		else {
		    LispFputs(Stdout, "* No variable named '");
		    LispFputs(Stdout, arg);
		    LispFputs(Stdout, "' in the selected frame.\n");
		}
		break;
	    case DebuggerBacktrace:
debugger_print_backtrace:
		if (DBG == NIL) {
		    LispFputs(Stdout, "* No stack.\n");
		    break;
		}
		DBG = LispReverse(DBG);
		for (obj = DBG, i = 0; obj != NIL; obj = CDR(obj), i++) {
		    frm = CAR(obj);
		    LispFputc(Stdout, '#');
		    LispFputs(Stdout, format_integer(i));
		    LispFputs(Stdout, "> (");
		    LispDoWriteObject(mac, NIL, CAR(frm), 1);
		    LispFputc(Stdout, ' ');
		    LispDoWriteObject(mac, NIL, CAR(CDR(frm)), 0);
		    LispFputs(Stdout, ")\n");
		}
		DBG = LispReverse(DBG);
		break;
	    case DebuggerContinue:
		mac->debug = LispDebugRun;
		goto debugger_command_done;
	    case DebuggerFinish:
		if (mac->debug != LispDebugFinish) {
		    mac->debug_step = mac->debug_level - 2;
		    mac->debug = LispDebugFinish;
		}
		else
		    mac->debug_step = mac->debug_level - 1;
		goto debugger_command_done;
	    case DebuggerNext:
		if (mac->debug != LispDebugNext) {
		    mac->debug = LispDebugNext;
		    mac->debug_step = mac->debug_level + 1;
		}
		goto debugger_command_done;
	    case DebuggerStep:
		mac->debug = LispDebugStep;
		goto debugger_command_done;
	}
	continue;

debugger_new_frame:
	/* goto here with i as the new frame value, after error checking */
	if (i != frame) {
	    frame = i;
	    for (frm = DBG, i = mac->debug_level; i > frame; frm = CDR(frm), i--)
		;
	    curframe = CAR(frm);
	    mac->stack.base = CAR(CDR(CDR(curframe)))->data.integer;
	    mac->env.length = CAR(CDR(CDR(CDR(curframe))))->data.integer;
	    mac->env.lex = CDR(CDR(CDR(CDR(curframe))))->data.integer;
	}
debugger_print_frame:
	LispFputc(Stdout, '#');
	LispFputs(Stdout, format_integer(frame));
	LispFputs(Stdout, "> (");
	LispDoWriteObject(mac, NIL, CAR(curframe), 1);
	LispFputc(Stdout, ' ');
	LispDoWriteObject(mac, NIL, CAR(CDR(curframe)), 0);
	LispFputs(Stdout, ")\n");
    }

debugger_command_done:
    mac->stack.base = envbase;
    mac->env.length = envlen;
    mac->env.lex = envlex;
}

static char *
format_integer(int integer)
{
    static char buffer[16];

    sprintf(buffer, "%d", integer);

    return (buffer);
}

#endif /* DEBUGGER */

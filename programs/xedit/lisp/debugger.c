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

/* $XFree86$ */

#include <ctype.h>
#include "debugger.h"

#define DebuggerHelp		0
#define DebuggerAbort		1
#define DebuggerBacktrace	2
#define DebuggerContinue	3
#define DebuggerFinish		4
#define DebuggerFrame		5
#define DebuggerNext		6
#define DebuggerPrint		7
#define DebuggerStep		8

/*
 * Prototypes
 */
static void LispDebuggerCommand(LispMac*);

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
    {"bt",		DebuggerBacktrace},
    {"continue",	DebuggerContinue},
    {"finish",		DebuggerFinish},
    {"frame",		DebuggerFrame},
    {"next",		DebuggerNext},
    {"print",		DebuggerPrint},
    {"step",		DebuggerStep},
};

static char debugger_help[] =
"Available commands are:\n\
\n\
help	  - This message.\n\
abort	  - Abort the current execution, and return to toplevel.\n\
backtrace - Print backtrace.\n\
bt	  - Print backtrace.\n\
continue  - Continue execution.\n\
finish	  - Executes until current form is finished.\n\
frame	  - Set environment to selected frame.\n\
next	  - Evaluate next form.\n\
print	  - Print value of variable name argument.\n\
step	  - Evaluate next form, stopping on any subforms.\n\
\n\
Commands may be abbreviated.\n";

/*
 * Implementation
 */
void
LispDebugger(LispMac *mac, LispDebugCall call, LispObj *name, LispObj *arg)
{
    int i;

    switch (call) {
	case LispDebugCallBegin:
	    ++mac->debug_level;
	    GCProtect();
	    DBG = CONS(CONS(name, CONS(arg, CONS(ENV, LEX))), DBG);
	    GCUProtect();
	    break;
	case LispDebugCallEnd:
	    DBG = CDR(DBG);
	    if (mac->debug_level <= mac->debug_step)
		mac->debug_step = mac->debug_level;
	    --mac->debug_level;
	    break;
	case LispDebugCallFatal:
	    /* just a remind... */
	    fprintf(lisp_stdout, "%sfatal errors not yet implemented.\n",
		    DBGPROMPT);
	    mac->debug = LispDebugStep;
	    return;
    }

    switch (mac->debug) {
	case LispDebugUnspec:
	    LispDebuggerCommand(mac);
	    goto debugger_done;
	case LispDebugRun:
	    goto debugger_done;
	case LispDebugFinish:
	    if (call != LispDebugCallEnd || mac->debug_level != mac->debug_step)
		goto debugger_done;
	    break;
	case LispDebugNext:
	    if (call == LispDebugCallBegin) {
		if (mac->debug_level != mac->debug_step)
		    goto debugger_done;
	    }
	    else if (call == LispDebugCallEnd) {
		if (mac->debug_level + 1 != mac->debug_step)
		    goto debugger_done;
	    }
	    break;
	case LispDebugStep:
	    break;
    }

    if (call == LispDebugCallBegin) {
	if (!mac->newline)
	    fputc('\n', lisp_stdout);
	for (i = 0; i < mac->debug_level; i++)
	    fputc(' ', lisp_stdout);
	fprintf(lisp_stdout, "#%d> ", mac->debug_level);

	fputc('(', lisp_stdout);
	LispPrintObj(mac, NIL, CAR(CAR(DBG)), 1);
	fputc(' ', lisp_stdout);
	LispPrintObj(mac, NIL, CAR(CDR(CAR(DBG))), 0);
	fprintf(lisp_stdout, ")\n");
	LispDebuggerCommand(mac);
    }
    else if (call == LispDebugCallEnd) {
	if (!mac->newline)
	    fputc('\n', lisp_stdout);
	for (i = 0; i <= mac->debug_level; i++)
	    fputc(' ', lisp_stdout);
	fprintf(lisp_stdout, "#%d= ", mac->debug_level + 1);

	LispPrintObj(mac, NIL, arg, 1);
	fputc('\n', lisp_stdout);
	mac->newline = 1;
	if (mac->debug == LispDebugFinish || mac->debug == LispDebugStep)
	    LispDebuggerCommand(mac);
    }

debugger_done:
}

static void
LispDebuggerCommand(LispMac *mac)
{
    LispObj *obj, *frm,
	    *old_frm = FRM, *old_env = ENV, *old_sym = SYM, *old_lex = LEX;
    int i, j, frame, matches, action = -1;
    char *cmd, *arg, *ptr, line[256];

    for (;;) {
	fprintf(lisp_stdout, "%s", DBGPROMPT);
	if (fgets(line, sizeof(line), lisp_stdin) == NULL) {
	    fputc('\n', lisp_stdout);
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

	if (*cmd == '\0') {
	    if (action < 0) {
		if (mac->debug == LispDebugFinish)
		    action = DebuggerFinish;
		else if (mac->debug == LispDebugNext)
		    action = DebuggerNext;
		else if (mac->debug == LispDebugStep)
		    action = DebuggerStep;
		else
		    continue;
	    }
	    else {
		if (action == DebuggerFrame || action == DebuggerPrint)
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
		    ++matches;
		}
	    }
	    if (matches == 0) {
		fprintf(lisp_stdout, "* Command unknown: %s. "
			"Type help for help.\n", cmd);
		continue;
	    }
	    else if (matches > 1) {
		fprintf(lisp_stdout, "* Command is ambiguous: %s. "
			"Type help for help.\n", cmd);
		continue;
	    }
	}

	switch (action) {
	    case DebuggerHelp:
		fprintf(lisp_stdout, debugger_help);
		break;
	    case DebuggerAbort:
		/* Almost the same code as LispDestroy */
		while (mac->mem.mem_level)
		    free(mac->mem.mem[--mac->mem.mem_level]);

		LispTopLevel(mac);
		if (mac->st) {
		    mac->cp = &(mac->st[strlen(mac->st)]);
		    mac->tok = 0;
		}
		mac->column = 0;
		mac->newline = 1;
		longjmp(mac->jmp, 0);	/* don't need to restore environment */
		/*NOTREACHED*/
		break;
	    case DebuggerFrame:
		ptr = arg;
		frame = 0;
		while (*ptr && isdigit(*ptr)) {
		    frame *= 10;
		    frame += *ptr - '0';
		    ++ptr;
		}
		if (*ptr) {
		    fprintf(lisp_stdout, "* Frame identifier must "
			    "be a number.\n");
		    break;
		}
		if (frame < mac->debug_level) {
		    DBG = LispReverse(DBG);
		    for (obj = DBG, i = 0; i < frame; obj = CDR(obj), i++)
			;
		    frm = CAR(obj);
		    DBG = LispReverse(DBG);
		    if (FRM == old_frm) {
			/* if first time selecting a new frame */
			GCProtect();
			FRM = CONS(CONS(ENV, SYM), old_frm);
			GCUProtect();
		    }
		    ENV = SYM = CAR(CDR(CDR(frm)));
		    LEX = CDR(CDR(CDR(frm)));
		}
		else if (frame > mac->debug_level)
		    fprintf(lisp_stdout, "* No such frame: %d.\n", frame);
		break;
	    case DebuggerPrint:
		ptr = arg;
		while (*ptr) {
		    *ptr = toupper(*ptr);
		    ++ptr;
		}
		obj = LispGetVar(mac, LispGetString(mac, arg), 0);
		if (obj) {
		    LispPrintObj(mac, NIL, obj, 1);
		    fputc('\n', lisp_stdout);
		}
		else
		    fprintf(lisp_stdout, "* No variable named %s "
			    "in the selected frame.\n", arg);
		break;
	    case DebuggerBacktrace:
		DBG = LispReverse(DBG);
		for (obj = DBG, i = 0; obj != NIL; obj = CDR(obj), i++) {
		    if (!mac->newline)
			fputc('\n', lisp_stdout);
		    frm = CAR(obj);
		    for (j = 0; j < i; j++)
			fputc(' ', lisp_stdout);
		    fprintf(lisp_stdout, "#%d> (", i);
		    LispPrintObj(mac, NIL, CAR(frm), 1);
		    fputc(' ', lisp_stdout);
		    LispPrintObj(mac, NIL, CAR(CDR(frm)), 0);
		    fprintf(lisp_stdout, ")\n");
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
    }

debugger_command_done:
    FRM = old_frm;
    ENV = old_env;
    SYM = old_sym;
    LEX = old_lex;
}

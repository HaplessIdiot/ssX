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

/* $XFree86$ */

#define VARIABLE_USED		0x0001

/*
 * Prototypes
 */
static void ComPredicate(LispCom*, LispBuiltin*, LispBytePredicate);
static void ComReturnFrom(LispCom*, LispBuiltin*, int);

static int ComConstantp(LispCom*, LispObj*);
static void ComAddVariable(LispCom*, LispObj*, LispObj*);
static int ComGetVariable(LispCom*, LispObj*);
static void ComVariableUsed(LispCom*, LispAtom*);

static int FindIndex(void*, void**, int);
static int compare(const void*, const void*);
static int BuildTablePointer(LispMac*, void*, void***, int*);

static void ComLabel(LispCom*, LispObj*);
static void ComPush(LispCom*, LispObj*, LispObj*, int, int, int);
static int ComCall(LispCom*, LispArgList*, LispObj*, LispObj*, int, int, int);
static void ComFuncall(LispCom*, LispObj*, LispObj*, int);
static void ComProgn(LispCom*, LispObj*);
static void ComEval(LispCom*, LispObj*);

static void ComRecursiveCall(LispCom*, LispArgList*, LispObj*, LispObj*);
static void ComInlineCall(LispCom*, LispArgList*, LispObj*, LispObj*, LispObj*);

static void ComMacroBackquote(LispCom*, LispObj*);
static void ComMacroCall(LispCom*, LispArgList*, LispObj*, LispObj*, LispObj*);
static LispObj *ComMacroExpandBackquote(LispCom*, LispObj*);
static LispObj *ComMacroExpand(LispCom*, LispObj*);
static LispObj *ComMacroExpandFuncall(LispCom*, LispObj*, LispObj*);
static LispObj *ComMacroExpandEval(LispCom*, LispObj*);

/*
 * Implementation
 */
void
Com_And(LispCom *com, LispBuiltin *builtin)
/*
 and &rest args
 */
{
    LispMac *mac = com->mac;
    LispObj *args;

    args = ARGUMENT(0);

    if (CONS_P(args)) {
	/* Evaluate first argument */
	ComEval(com, CAR(args));
	args = CDR(args);

	/* If more than one argument, create jump list */
	if (CONS_P(args)) {
	    CodeTree *tree = NULL, *group;

	    group = NEW_TREE(CodeTreeJumpIf);
	    group->code = XBC_JUMPNIL;

	    for (; CONS_P(args); args = CDR(args)) {
		ComEval(com, CAR(args));
		tree = NEW_TREE(CodeTreeJumpIf);
		tree->code = XBC_JUMPNIL;
		group->group = tree;
		group = tree;
	    }
	    /*  Finish form the last CodeTree code is changed to sign the
	     * end of the AND list */
	    group->code = XBC_NOOP;
	    if (group)
		group->group = tree;
	}
    }
    else
	/* Identity of AND is T */
	com_Bytecode(com, XBC_T);
}

void
Com_Block(LispCom *com, LispBuiltin *builtin)
/*
 block name &rest body
 */
{

    LispMac *mac = com->mac;
    LispObj *name, *body;

    body = ARGUMENT(1);
    name = ARGUMENT(0);

    if (name != NIL && name != T && !SYMBOL_P(name))
	LispDestroy(mac, "%s: %s cannot name a block",
		    STRFUN(builtin), STROBJ(name));
    if (CONS_P(body)) {
	CompileIniBlock(com, LispBlockTag, name);
	ComProgn(com, body);
	CompileFiniBlock(com);
    }
    else
	/* Just load NIL without starting an empty block */
	com_Bytecode(com, XBC_NIL);
}

void
Com_C_r(LispCom *com, LispBuiltin *builtin)
/*
 c[ad]{1,4}r list
 */
{
    LispMac *mac = com->mac;
    LispObj *list;
    char *desc;

    list = ARGUMENT(0);

    desc = STRFUN(builtin);
    if (*desc == 'F')		/* FIRST */
	desc = "CAR";
    else if (*desc == 'R')	/* REST */
	desc = "CDR";

    /* Check if it is a list of constants */
    while (desc[1] != 'R')
	desc++;
    ComEval(com, list);
    while (*desc != 'C') {
	com_Bytecode(com, *desc == 'A' ? XBC_CAR : XBC_CDR);
	--desc;
    }
}

void
Com_Cond(LispCom *com, LispBuiltin *builtin)
/*
 cond &rest body
 */
{
    LispMac *mac = com->mac;
    int count;
    LispObj *code, *body;
    CodeTree *group, *tree;

    body = ARGUMENT(0);

    count = 0;
    group = NULL;
    if (CONS_P(body)) {
	int constant;

	for (; CONS_P(body); body = CDR(body)) {
	    code = CAR(body);
	    ERROR_CHECK_LIST(code);
	    /* If a constant NIL test condition. This does not prevent
	     * things like (OR) or (AND NIL), etc. Should really try
	     * to remove this sort of things here? */
	    if (CAR(code) == NIL)
		continue;
	    ++count;
	    /* If test known to be true */
	    constant = ComConstantp(com, CAR(code));
	    if (!constant) {
		ComEval(com, CAR(code));
		tree = NEW_TREE(CodeTreeCond);
		if (group)
		    group->group = tree;
		tree->code = XBC_JUMPNIL;
		group = tree;
	    }
	    /* The code to execute if the test is true */
	    ComProgn(com, CDR(code));
	    /* Add a node signaling the end of the PROGN code */
	    tree = NEW_TREE(CodeTreeCond);
	    tree->code = XBC_JUMPT;
	    if (group)
		group->group = tree;
	    group = tree;
	    /* If test known to be true */
	    if (constant)
		break;
	}
    }
    if (!count)
	com_Bytecode(com, XBC_NIL);
    else
	/* Where to jump after T progn */
	group->code = XBC_NOOP;
}

void
Com_Cons(LispCom *com, LispBuiltin *builtin)
/*
 cons car cdr
 */
{
    LispMac *mac = com->mac;
    LispObj *car, *cdr;

    cdr = ARGUMENT(1);
    car = ARGUMENT(0);

    if (ComConstantp(com, car) && ComConstantp(com, cdr))
	com_BytecodeCons(com, XBC_CCONS, car, cdr);
    else {
	++com->stack.cpstack;
	if (com->stack.pstack < com->stack.cpstack)
	    com->stack.pstack = com->stack.cpstack;
	ComEval(com, car);
	com_Bytecode(com, XBC_CSTAR);
	ComEval(com, cdr);
	com_Bytecode(com, XBC_CFINI);
	--com->stack.cpstack;
    }
}

void
Com_Consp(LispCom *com, LispBuiltin *builtin)
/*
 consp object
 */
{
    ComPredicate(com, builtin, XBP_CONSP);
}

void
Com_Dolist(LispCom *com, LispBuiltin *builtin)
/*
 dolist init &rest body
 */
{
    int unbound, item;
    LispMac *mac = com->mac;
    LispObj *symbol, *list, *result;
    LispObj *init, *body;
    CodeTree *group, *tree;

    body = ARGUMENT(1);
    init = ARGUMENT(0);

    ERROR_CHECK_LIST(init);
    symbol = CAR(init);
    ERROR_CHECK_SYMBOL(symbol);
    ERROR_CHECK_CONSTANT(symbol);
    init = CDR(init);
    if (CONS_P(init)) {
	list = CAR(init);
	init = CDR(init);
    }
    else
	list = NIL;
    if (CONS_P(init)) {
	result = CAR(init);
	if (CONS_P(CDR(init)))
	    LispDestroy(mac, "%s: too many arguments %s",
			STRFUN(builtin), STROBJ(CDR(init)));
    }
    else
	result = NIL;

    /*	Generate code for the body of the form.
     *	The generated code uses two objects unavailable to user code,
     * in the format:
     *	(block NIL
     *	    (let ((? list) (item NIL))
     *		(tagbody
     *		    .			    ; the DOT object as a label
     *		    (when (consp list)
     *			(setq item (car ?))
     *			@body		    ; code to be executed
     *			(setq ? (cdr ?))
     *			(go .)
     *		    )
     *		)
     *		(setq item nil)
     *		result
     *	    )
     *	)
     */

    /* XXX All of the logic below should be simplified at some time
     * by adding more opcodes for compound operations ... */

    /* Relative offsets the locally added variables will have at run time */
    unbound = mac->env.length - mac->env.lex;
    item = unbound + 1;

    /* Start BLOCK NIL */
    FORM_ENTER();
    CompileIniBlock(com, LispBlockTag, NIL);

    /* Add the <?> variable */
    ComPush(com, UNBOUND, list, 1, 0, 0);
    /* Add the <item> variable */
    ComPush(com, symbol, NIL, 0, 0, 0);
    /* Stack length is increased */
    CompileStackEnter(com, 2, 0);
    /* Bind variables */
    com_Bind(com, 2);
    com->block->bind += 2;
    mac->env.head += 2;

    /* Initialize the TAGBODY */
    FORM_ENTER();
    CompileIniBlock(com, LispBlockBody, NIL);

    /* Create the <.> label */
    ComLabel(com, DOT);

    /* Load <?> variable */
    com_BytecodeShort(com, XBC_LOAD, unbound);
    /* Check if <?> is a list */
    com_BytecodeChar(com, XBC_PRED, XBP_CONSP);

    /* Start WHEN block */
    group = NEW_TREE(CodeTreeJumpIf);
    group->code = XBC_JUMPNIL;
    /* Load <?> again */
    com_BytecodeShort(com, XBC_LOAD, unbound);
    /* Get CAR of <?> */
    com_Bytecode(com, XBC_CAR);
    /* Store it in <item> */
    com_BytecodeShort(com, XBC_SET, item);
    /* Execute @BODY */
    ComProgn(com, body);

    /* Load <?> again */
    com_BytecodeShort(com, XBC_LOAD, unbound);
    /* Get CDR of <?> */
    com_Bytecode(com, XBC_CDR);
    /* Change value of <?> */
    com_BytecodeShort(com, XBC_SET, unbound);

    /* GO back to <.> */
    tree = NEW_TREE(CodeTreeGo);
    tree->data.object = DOT;

    /* Finish WHEN block */
    tree = NEW_TREE(CodeTreeJumpIf);
    tree->code = XBC_NOOP;
    group->group = tree;

    /* Finish the TAGBODY */
    CompileFiniBlock(com);
    FORM_LEAVE();

    /* Set <item> to NIL, in case result references it...
     * Loaded value is NIL as the CONSP predicate */
    com_BytecodeShort(com, XBC_SET, item);

    /* Evaluate <result> */
    ComEval(com, result);

    /* Unbind variables */
    mac->env.head -= 2;
    mac->env.length -= 2;
    com->block->bind -= 2;
    com_Unbind(com, 2);
    /* Stack length is reduced. */
    CompileStackLeave(com, 2, 0);

    /* Finish BLOCK NIL */
    CompileFiniBlock(com);
    FORM_LEAVE();
}

void
Com_Eq(LispCom *com, LispBuiltin *builtin)
/*
 eq left right
 eql left right
 equal left right
 equalp left right
 */
{
    LispMac *mac = com->mac;
    LispObj *left, *right;
    LispByteOpcode code;
    char *name;

    right = ARGUMENT(1);
    left = ARGUMENT(0);

    CompileStackEnter(com, 1, 1);
    /* Just like preparing to call a builtin function */
    ComEval(com, left);
    com_Bytecode(com, XBC_PUSH);
    /* The second argument is now loaded */
    ComEval(com, right);

    /* Compare arguments and restore builtin stack */
    name = STRFUN(builtin);
    switch (name[3]) {
	case 'L':
	    code = XBC_EQL;
	    break;
	case 'U':
	    code = name[5] == 'P' ? XBC_EQUALP : XBC_EQUAL;
	    break;
	default:
	    code = XBC_EQ;
	    break;
    }
    com_Bytecode(com, code);

    CompileStackLeave(com, 1, 1);
}

void
Com_Go(LispCom *com, LispBuiltin *builtin)
/*
 go tag
 */
{
    LispMac *mac = com->mac;
    LispObj *tag;
    CodeTree *tree;

    tag = ARGUMENT(0);

    if (com->block->type != LispBlockBody)
	LispDestroy(com->mac, "%s called not within a block", STRFUN(builtin));

    /* If inside a LET block */
    com_Unbind(com, com->block->bind);
    tree = NEW_TREE(CodeTreeGo);
    tree->data.object = tag;
}

void
Com_If(LispCom *com, LispBuiltin *builtin)
/*
 if test then &optional else
 */
{
    LispMac *mac = com->mac;
    CodeTree *group, *tree;
    LispObj *test, *then, *oelse;

    oelse = ARGUMENT(2);
    then = ARGUMENT(1);
    test = ARGUMENT(0);

    /* Build code to execute test */
    ComEval(com, test);

    /* Add jump node to use if test is NIL */
    group = NEW_TREE(CodeTreeJumpIf);
    group->code = XBC_JUMPNIL;

    /* Build T code */
    ComEval(com, then);

    if (oelse != NIL) {
	/* Remember start of NIL code */
	tree = NEW_TREE(CodeTreeJump);
	tree->code = XBC_JUMP;
	group->group = tree;
	group = tree;
	/* Build NIL code */
	ComEval(com, oelse);
    }

    /* Remember jump of T code */
    tree = NEW_TREE(CodeTreeJumpIf);
    tree->code = XBC_NOOP;
    group->group = tree;
}

void
Com_Last(LispCom *com, LispBuiltin *builtin)
/*
 last list &optional count
 */
{
    LispMac *mac = com->mac;
    LispObj *list, *count;

    count = ARGUMENT(1);
    list = ARGUMENT(0);

    ComEval(com, list);
    CompileStackEnter(com, 1, 1);
    com_Bytecode(com, XBC_PUSH);
    ComEval(com, count);
    CompileStackLeave(com, 1, 1);
    com_Bytecode(com, XBC_LAST);
}

void
Com_Length(LispCom *com, LispBuiltin *builtin)
/*
 length sequence
 */
{
    LispMac *mac = com->mac;
    LispObj *sequence;

    sequence = ARGUMENT(0);

    ComEval(com, sequence);
    com_Bytecode(com, XBC_LENGTH);
}

void
Com_Let(LispCom *com, LispBuiltin *builtin)
/*
 let init &rest body
 */
{
    LispMac *mac = com->mac;
    LispObj *init, *body;

    body = ARGUMENT(1);
    init = ARGUMENT(0);

    if (init == NIL)
	/* If no local variables */
	ComProgn(com, body);
    else ERROR_CHECK_LIST(init);
    else {
	/* Could optimize if the body is empty and the
	 * init form is known to have no side effects */
	int count;
	LispObj *symbol, *value, *pair;

	for (count = 0; CONS_P(init); init = CDR(init), count++) {
	    pair = CAR(init);
	    if (CONS_P(pair)) {
		symbol = CAR(pair);
		pair = CDR(pair);
		if (CONS_P(pair)) {
		    value = CAR(pair);
		    if (CDR(pair) != NIL)
			LispDestroy(mac, "%s: too much arguments to initialize %s",
				    STRFUN(builtin), STROBJ(symbol));
		}
		else
		    value = NIL;
	    }
	    else {
		symbol = pair;
		value = NIL;
	    }
	    ERROR_CHECK_SYMBOL(symbol);
	    ERROR_CHECK_CONSTANT(symbol);

	    /* Add the variable */
	    ComPush(com, symbol, value, 1, 0, 0);
	}

	/* Stack length is increased */
	CompileStackEnter(com, count, 0);
	/* Bind the added variables */
	com_Bind(com, count);
	com->block->bind += count;
	mac->env.head += count;
	/* Generate code for the body of the form */
	ComProgn(com, body);
	/* Unbind the added variables */
	mac->env.head -= count;
	mac->env.length -= count;
	com->block->bind -= count;
	com_Unbind(com, count);
	/* Stack length is reduced. */
	CompileStackLeave(com, count, 0);
    }
}

void
Com_Letx(LispCom *com, LispBuiltin *builtin)
/*
 let* init &rest body
 */
{
    LispMac *mac = com->mac;
    LispObj *init, *body;

    body = ARGUMENT(1);
    init = ARGUMENT(0);

    if (init == NIL)
	/* If no local variables */
	ComProgn(com, body);
    else ERROR_CHECK_LIST(body);
    else {
	/* Could optimize if the body is empty and the
	 * init form is known to have no side effects */
	int count;
	LispObj *symbol, *value, *pair;

	for (count = 0; CONS_P(init); init = CDR(init), count++) {
	    pair = CAR(init);
	    if (CONS_P(pair)) {
		symbol = CAR(pair);
		pair = CDR(pair);
		if (CONS_P(pair)) {
		    value = CAR(pair);
		    if (CDR(pair) != NIL)
			LispDestroy(mac, "%s: too much arguments to initialize %s",
				    STRFUN(builtin), STROBJ(symbol));
		}
		else
		    value = NIL;
	    }
	    else {
		symbol = pair;
		value = NIL;
	    }
	    ERROR_CHECK_SYMBOL(symbol);
	    ERROR_CHECK_CONSTANT(symbol);

	    /* LET* is identical to &AUX arguments, just bind the symbol */
	    ComPush(com, symbol, value, 1, 0, 0);
	    /* Every added variable is binded */
	    com_Bind(com, 1);
	    /* Must be binded at compile time also */
	    ++mac->env.head;
	    ++com->block->bind;
	}

	/* Generate code for the body of the form */
	CompileStackEnter(com, count, 0);
	ComProgn(com, body);
	com_Unbind(com, count);
	com->block->bind -= count;
	mac->env.head -= count;
	mac->env.length -= count;
	CompileStackLeave(com, count, 0);
    }
}

void
Com_Listp(LispCom *com, LispBuiltin *builtin)
/*
 listp object
 */
{
    ComPredicate(com, builtin, XBP_LISTP);
}

void
Com_Loop(LispCom *com, LispBuiltin *builtin)
/*
 loop &rest body
 */
{
    LispMac *mac = com->mac;
    CodeTree *tree, *group;
    LispObj *body;

    body = ARGUMENT(0);

    /* Start NIL block */
    CompileIniBlock(com, LispBlockTag, NIL);

    /* Insert node to mark LOOP start */
    tree = NEW_TREE(CodeTreeJump);
    tree->code = XBC_NOOP;

    /* Execute @BODY */
    if (CONS_P(body))
	ComProgn(com, body);
    else
	/* XXX bytecode.c code require that blocks have at least one opcode */
	com_Bytecode(com, XBC_NIL);

    /* Insert node to jump of start of LOOP */
    group = NEW_TREE(CodeTreeJump);
    group->code = XBC_JUMP;
    group->group = tree;

    /* Finish NIL block */
    CompileFiniBlock(com);
}

void
Com_Nthcdr(LispCom *com, LispBuiltin *builtin)
/*
 nthcdr index list
 */
{
    LispMac *mac = com->mac;
    LispObj *oindex, *list;

    list = ARGUMENT(1);
    oindex = ARGUMENT(0);

    ComEval(com, oindex);
    CompileStackEnter(com, 1, 1);
    com_Bytecode(com, XBC_PUSH);
    ComEval(com, list);
    CompileStackLeave(com, 1, 1);
    com_Bytecode(com, XBC_NTHCDR);
}

void
Com_Null(LispCom *com, LispBuiltin *builtin)
/*
 null list
 */
{
    LispMac *mac = com->mac;
    LispObj *list;

    list = ARGUMENT(0);

    if (list == NIL)
	com_Bytecode(com, XBC_T);
    else if (ComConstantp(com, list))
	com_Bytecode(com, XBC_NIL);
    else {
	ComEval(com, list);
	com_Bytecode(com, XBC_INV);
    }
}

void
Com_Numberp(LispCom *com, LispBuiltin *builtin)
/*
 numberp object
 */
{
    ComPredicate(com, builtin, XBP_NUMBERP);
}

void
Com_Or(LispCom *com, LispBuiltin *builtin)
/*
 or &rest args
 */
{
    LispMac *mac = com->mac;
    LispObj *args;

    args = ARGUMENT(0);

    if (CONS_P(args)) {
	/* Evaluate first argument */
	ComEval(com, CAR(args));
	args = CDR(args);

	/* If more than one argument, create jump list */
	if (CONS_P(args)) {
	    CodeTree *tree = NULL, *group;

	    group = NEW_TREE(CodeTreeJumpIf);
	    group->code = XBC_JUMPT;

	    for (; CONS_P(args); args = CDR(args)) {
		ComEval(com, CAR(args));
		tree = NEW_TREE(CodeTreeJumpIf);
		tree->code = XBC_JUMPT;
		group->group = tree;
		group = tree;
	    }
	    /*  Finish form the last CodeTree code is changed to sign the
	     * end of the AND list */
	    group->code = XBC_NOOP;
	    group->group = tree;
	}
    }
    else
	/* Identity of OR is NIL */
	com_Bytecode(com, XBC_NIL);
}

void
Com_Progn(LispCom *com, LispBuiltin *builtin)
/*
 progn &rest body
 */
{
    LispMac *mac = com->mac;
    LispObj *body;

    body = ARGUMENT(0);

    ComProgn(com, body);
}

void
Com_Return(LispCom *com, LispBuiltin *builtin)
/*
 return &optional result
 */
{
    ComReturnFrom(com, builtin, 0);
}

void
Com_ReturnFrom(LispCom *com, LispBuiltin *builtin)
/*
 return-from name &optional result
 */
{
    ComReturnFrom(com, builtin, 1);
}

void
Com_Rplac_(LispCom *com, LispBuiltin *builtin)
/*
 rplac[ad] place value
 */
{
    LispMac *mac = com->mac;
    LispObj *place, *value;

    value = ARGUMENT(1);
    place = ARGUMENT(0);

    CompileStackEnter(com, 1, 1);
    ComEval(com, place);
    com_Bytecode(com, XBC_PUSH);
    ComEval(com, value);
    com_Bytecode(com, STRFUN(builtin)[5] == 'A' ? XBC_RPLACA : XBC_RPLACD);
    CompileStackLeave(com, 1, 1);
}

void
Com_Setq(LispCom *com, LispBuiltin *builtin)
/*
 setq &rest form
 */
{
    LispMac *mac = com->mac;
    int offset;
    LispObj *form, *symbol, *value;

    form = ARGUMENT(0);

    for (; CONS_P(form); form = CDR(form)) {
	symbol = CAR(form);
	ERROR_CHECK_SYMBOL(symbol);
	ERROR_CHECK_CONSTANT(symbol);
	form = CDR(form);
	if (!CONS_P(form))
	    LispDestroy(mac, "%s: odd number of arguments", STRFUN(builtin));
	value = CAR(form);
	/* Generate code to load value */
	ComEval(com, value);
	offset = ComGetVariable(com, symbol);
	if (offset >= 0)
	    com_Set(com, offset);
	else
	    com_SetSym(com, symbol->data.atom);
    }
}

void
Com_Tagbody(LispCom *com, LispBuiltin *builtin)
/*
 tagbody &rest body
 */
{
    LispMac *mac = com->mac;
    LispObj *body;

    body = ARGUMENT(0);

    if (CONS_P(body)) {
	CompileIniBlock(com, LispBlockBody, NIL);
	ComProgn(com, body);
	/* Tagbody returns NIL */
	com_Bytecode(com, XBC_NIL);
	CompileFiniBlock(com);
    }
    else
	/* Tagbody always returns NIL */
	com_Bytecode(com, XBC_NIL);
}

void
Com_Unless(LispCom *com, LispBuiltin *builtin)
/*
 unless test &rest body
 */
{
    LispMac *mac = com->mac;
    CodeTree *group, *tree;
    LispObj *test, *body;

    body = ARGUMENT(1);
    test = ARGUMENT(0);

    /* Generate code to evaluate test */
    ComEval(com, test);
    /* Add node after test */
    group = NEW_TREE(CodeTreeJumpIf);
    group->code = XBC_JUMPT;
    /* Generate NIL code */
    ComProgn(com, body);
    /* Insert node to know where to jump if test is T */
    tree = NEW_TREE(CodeTreeJumpIf);
    tree->code = XBC_NOOP;
    group->group = tree;
}

void
Com_Until(LispCom *com, LispBuiltin *builtin)
/*
 until test &rest body
 */
{
    LispMac *mac = com->mac;
    CodeTree *tree, *group, *ltree, *lgroup;
    LispObj *test, *body;

    body = ARGUMENT(1);
    test = ARGUMENT(0);

    /* Insert node to mark LOOP start */
    ltree = NEW_TREE(CodeTreeJump);
    ltree->code = XBC_NOOP;

    /* Build code for test */
    ComEval(com, test);
    group = NEW_TREE(CodeTreeJumpIf);
    group->code = XBC_JUMPT;

    /* Execute @BODY */
    ComProgn(com, body);

    /* Insert node to jump to test again */
    lgroup = NEW_TREE(CodeTreeJump);
    lgroup->code = XBC_JUMP;
    lgroup->group = ltree;

    /* Insert node to know where to jump if test is T */
    tree = NEW_TREE(CodeTreeJumpIf);
    tree->code = XBC_NOOP;
    group->group = tree;
}

void
Com_When(LispCom *com, LispBuiltin *builtin)
/*
 when test &rest body
 */
{
    LispMac *mac = com->mac;
    CodeTree *group, *tree;
    LispObj *test, *body;

    body = ARGUMENT(1);
    test = ARGUMENT(0);

    /* Generate code to evaluate test */
    ComEval(com, test);
    /* Add node after test */
    group = NEW_TREE(CodeTreeJumpIf);
    group->code = XBC_JUMPNIL;
    /* Generate T code */
    ComProgn(com, body);
    /* Insert node to know where to jump if test is NIL */
    tree = NEW_TREE(CodeTreeJumpIf);
    tree->code = XBC_NOOP;
    group->group = tree;
}

void
Com_While(LispCom *com, LispBuiltin *builtin)
/*
 while test &rest body
 */
{
    LispMac *mac = com->mac;
    CodeTree *tree, *group, *ltree, *lgroup;
    LispObj *test, *body;

    body = ARGUMENT(1);
    test = ARGUMENT(0);

    /* Insert node to mark LOOP start */
    ltree = NEW_TREE(CodeTreeJump);
    ltree->code = XBC_NOOP;

    /* Build code for test */
    ComEval(com, test);
    group = NEW_TREE(CodeTreeJumpIf);
    group->code = XBC_JUMPNIL;

    /* Execute @BODY */
    ComProgn(com, body);

    /* Insert node to jump to test again */
    lgroup = NEW_TREE(CodeTreeJump);
    lgroup->code = XBC_JUMP;
    lgroup->group = ltree;

    /* Insert node to know where to jump if test is NIL */
    tree = NEW_TREE(CodeTreeJumpIf);
    tree->code = XBC_NOOP;
    group->group = tree;
}


/***********************************************************************
 * Com_XXX helper functions
 ***********************************************************************/
static void
ComPredicate(LispCom *com, LispBuiltin *builtin, LispBytePredicate predicate)
{
    LispMac *mac = com->mac;
    LispObj *object;

    object = ARGUMENT(0);

    if (ComConstantp(com, object)) {
	switch (predicate) {
	    case XBP_CONSP:
		com_Bytecode(com, CONS_P(object) ? XBC_T : XBC_NIL);
		break;
	    case XBP_LISTP:
		com_Bytecode(com, CONS_P(object) || object == NIL ?
			     XBC_T : XBC_NIL);
		break;
	    case XBP_NUMBERP:
		com_Bytecode(com, NUMBER_P(object) ? XBC_T : XBC_NIL);
		break;
	}
    }
    else {
	ComEval(com, object);
	com_BytecodeChar(com, XBC_PRED, predicate);
    }
}

/* XXX Could receive an argument telling if is the last statement in the
 * block(s), i.e. if a jump opcode should be generated or just the
 * evaluation of the returned value. Probably this is better done in
 * an optimization step. */
static void
ComReturnFrom(LispCom *com, LispBuiltin *builtin, int from)
{
    LispMac *mac = com->mac;
    int bind;
    CodeTree *tree;
    LispObj *name, *result;
    CodeBlock *block = com->block;

    if (from) {
	result = ARGUMENT(1);
	name = ARGUMENT(0);
    }
    else {
	result = ARGUMENT(0);
	name = NIL;
    }

    bind = block->bind;
    while (block) {
	if (block->type == LispBlockClosure)
	    /* A function call */
	    break;
	else if (block->type == LispBlockTag && block->tag == name)
	    break;
	block = block->prev;
	bind += block->bind;
    }

    if (!block || block->tag != name)
	LispDestroy(mac, "%s: no visible %s block",
		    STRFUN(builtin), STROBJ(name));

    /* Generate code to load result */
    ComEval(com, result);

    /* Check for added variables that the jump is skiping the unbind opcode */
    com_Unbind(com, bind);

    tree = NEW_TREE(CodeTreeReturn);
    tree->data.block = block;
}

/***********************************************************************
 * Helper functions
 ***********************************************************************/
static int
ComConstantp(LispCom *com, LispObj *object)
{
    switch (object->type) {
	case LispAtom_t:
	    /* Keywords are guaranteed to evaluate to itself */
	    if (object->data.atom->package == com->mac->keyword)
		break;
	    return (0);

	    /* Function call */
	case LispCons_t:

	    /* Need macro expansion, these are special abstract objects */
	case LispQuote_t:
	case LispBackquote_t:
	case LispComma_t:
	    return (0);

	    /* Anything else is a literal constant */
	default:
	    break;
    }

    return (1);
}

static int
FindIndex(void *item, void **table, int length)
{
    long cmp;
    int left, right, i;

    left = 0;
    right = length - 1;
    while (left <= right) {
	i = (left + right) >> 1;
	cmp = (char*)item - (char*)table[i];
	if (cmp == 0)
	    return (i);
	else if (cmp < 0)
	    right = i - 1;
	else
	    left = i + 1;
    }

    return (-1);
}

static int
compare(const void *left, const void *right)
{
    long cmp = *(char**)left - *(char**)right;

    return (cmp < 0 ? -1 : 1);
}

static int
BuildTablePointer(LispMac *mac,
		  void *pointer, void ***pointers, int *num_pointers)
{
    int i;

    if ((i = FindIndex(pointer, *pointers, *num_pointers)) < 0) {
	*pointers = LispRealloc(mac, *pointers,
				sizeof(void*) * (*num_pointers + 1));
	(*pointers)[*num_pointers] = pointer;
	if (++*num_pointers > 1)
	    qsort(*pointers, *num_pointers, sizeof(void*), compare);
	i = FindIndex(pointer, *pointers, *num_pointers);
    }

    return (i);
}

static void
ComAddVariable(LispCom *com, LispObj *symbol, LispObj *value)
{
    LispAtom *atom = symbol->data.atom;

    if (atom && atom->string && !com->macro) {
	int i, length = com->block->variables.length;

	i = BuildTablePointer(com->mac, atom,
			      (void***)&com->block->variables.symbols,
			      &com->block->variables.length);

	if (com->block->variables.length != length) {
	    com->block->variables.flags =
		LispRealloc(com->mac, com->block->variables.flags,
			    com->block->variables.length * sizeof(int));
	    com->block->variables.flags[i] = 0;
	}
    }

    LispAddVar(com->mac, symbol, value);
}

static int
ComGetVariable(LispCom *com, LispObj *symbol)
{
    LispMac *mac;
    LispAtom *name;
    int i, base, offset;
    Atom_id id;

    mac = com->mac;
    name = symbol->data.atom;
    if (name->constant) {
	if (name->package == mac->keyword)
	    /*	Just load <symbol> from the byte stream, keywords are
	     * guaranteed to evaluate to itself. */
	    return (SYMBOL_KEYWORD);
	return (SYMBOL_CONSTANT);
    }

    offset = name->offset;
    id = name->string;
    base = mac->env.lex;
    i = mac->env.head - 1;

    /* If variable is local */
    if (offset <= i && offset >= base && mac->env.names[offset] == id) {
	ComVariableUsed(com, name);
	/* Relative offset */
	return (offset - base);
    }

    /* name->offset may have been changed in a macro expansion */
    for (; i >= base; i--)
	if (mac->env.names[i] == id) {
	    name->offset = i;
	    ComVariableUsed(com, name);
	    return (i - base);
	}

    if (!name->a_object) {
	++com->warnings;
	LispWarning(mac, "variable %s is neither declared nor bound",
		    name->string);
    }

    /* Not found, resolve <symbol> at run time */
    return (SYMBOL_UNBOUND);
}

static void
ComVariableUsed(LispCom *com, LispAtom *atom)
{
    int i;
    CodeBlock *block = com->block;

    while (block) {
	i = FindIndex(atom, (void**)block->variables.symbols,
		      block->variables.length);
	if (i >= 0) {
	    block->variables.flags[i] |= VARIABLE_USED;
	    break;
	}
	block = block->prev;
    }
}

/***********************************************************************
 * Bytecode compiler functions
 ***********************************************************************/
static void
ComLabel(LispCom *com, LispObj *label)
{
    int i;
    CodeTree *tree;
    LispMac *mac = com->mac;

    for (i = 0; i < com->block->tagbody.length; i++)
	if (XEQL(label, com->block->tagbody.labels[i]) == T)
	    LispDestroy(mac, "TAGBODY: tag %s specified more than once",
			STROBJ(label));

    if (com->block->tagbody.length >= com->block->tagbody.space) {
	com->block->tagbody.labels =
	    LispRealloc(com->mac, com->block->tagbody.labels,
			sizeof(LispObj*) * (com->block->tagbody.space + 8));
	/*  Reserve space, will be used at link time when
	 * resolving GO jumps. */
	com->block->tagbody.codes =
	    LispRealloc(com->mac, com->block->tagbody.codes,
			sizeof(CodeTree*) * (com->block->tagbody.space + 8));
	com->block->tagbody.space += 8;
    }

    com->block->tagbody.labels[com->block->tagbody.length++] = label;
    tree = NEW_TREE(CodeTreeLabel);
    tree->data.object = label;
}

static void
ComPush(LispCom *com, LispObj *symbol, LispObj *value,
	int eval, int builtin, int compile)
{
    LispMac *mac = com->mac;

    /*  If <compile> is set, it is pushing an argument to one of
     * Com_XXX functions. */
    if (compile) {
	if (builtin)
	    mac->stack.values[mac->stack.length++] = value;
	else
	    ComAddVariable(com, symbol, value);
	return;
    }

    /*  If <com->macro> is set, it is expanding a macro, just add the local
     * variable <symbol> bounded to <value>, so that it will be available
     * when calling the interpreter to expand the macro. */
    else if (com->macro) {
	ComAddVariable(com, symbol, value);
	return;
    }

    /*  If <eval> is set, it must generate the opcodes to evaluate <value>.
     * If <value> is a constant, just generate the opcodes to load it. */
    else if (eval && !ComConstantp(com, value)) {
	switch (value->type) {
	    case LispAtom_t: {
		int offset = ComGetVariable(com, value);

		if (offset >= 0) {
		    /* Load <value> from user stack at the relative offset */
		    if (builtin)
			com_LoadPush(com, offset);
		    else
			com_LoadLet(com, offset, symbol->data.atom);
		}
		/* ComConstantp() does not return true for this, as the
		 * current value must be computed. */
		else if (offset == SYMBOL_CONSTANT) {
		    value = value->data.atom->property->value;
		    if (builtin)
			com_LoadConPush(com, value);
		    else
			com_LoadConLet(com, value, symbol->data.atom);
		}
		else {
		    /* Load value bound to <value> at run time */
		    if (builtin)
			com_LoadSymPush(com, value->data.atom);
		    else
			com_LoadSymLet(com, value->data.atom,
				       symbol->data.atom);
		}
	    }	break;

	    default:
		/* Generate code to evaluate <value> */
		ComEval(com, value);
		if (builtin)
		    com_Bytecode(com, XBC_PUSH);
		else
		    com_Let(com, symbol->data.atom);
		break;
	}
	/*  Remember <symbol> will be bound, <value> only matters for
	 * the Com_XXX  functions */
	if (builtin)
	    mac->stack.values[mac->stack.length++] = value;
	else
	    ComAddVariable(com, symbol, value);
	return;
    }

    /* XXX all builtin macros should be implemented as bytecode, but
     * do this for now, just to remember variable is used, in case it
     * is only referenced as an argument to a builtin macro */
    if (SYMBOL_P(value))
	ComVariableUsed(com, value->data.atom);

    if (builtin) {
	/* Load <value> as a constant in builtin stack */
	com_LoadConPush(com, value);
	mac->stack.values[mac->stack.length++] = value;
    }
    else {
	/* Load <value> as a constant in stack */
	com_LoadConLet(com, value, symbol->data.atom);
	/* Remember <symbol> will be bound */
	ComAddVariable(com, symbol, value);
    }
}

/*  This function does almost the same job as LispMakeEnvironment, but
 * it is not optimized for speed, as it is not building argument lists
 * to user code, but to Com_XXX functions, or helping in generating the
 * opcodes to load arguments at bytecode run time. */
static int
ComCall(LispCom *com, LispArgList *alist,
	LispObj *name, LispObj *values,
	int eval, int builtin, int compile)
{
    char *desc;
    LispMac *mac;
    int i, count, base;
    LispObj **symbols, **defaults, **sforms;

    mac = com->mac;
    if (builtin) {
	base = mac->stack.length;
	/* This should never be executed, but make the check for safety */
	if (base + alist->num_arguments > mac->stack.space) {
	    do
		LispMoreStack(mac);
	    while (base + alist->num_arguments > mac->stack.space);
	}
    }
    else
	base = mac->env.length;

    desc = alist->description;
    switch (*desc++) {
	case '.':
	    goto normal_label;
	case 'o':
	    goto optional_label;
	case 'k':
	    goto key_label;
	case 'r':
	    goto rest_label;
	case 'a':
	    goto aux_label;
	default:
	    goto done_label;
    }


    /* Normal arguments */
normal_label:
    i = 0;
    symbols = alist->normals.symbols;
    count = alist->normals.num_symbols;
    for (; i < count && CONS_P(values); i++, values = CDR(values)) {
	ComPush(com, symbols[i], CAR(values), eval, builtin, compile);
    }
    if (i < count)
	LispDestroy(mac, "%s: too few arguments", STROBJ(name));

    switch (*desc++) {
	case 'o':
	    goto optional_label;
	case 'k':
	    goto key_label;
	case 'r':
	    goto rest_label;
	case 'a':
	    goto aux_label;
	default:
	    goto done_label;
    }


    /* &OPTIONAL */
optional_label:
    i = 0;
    count = alist->optionals.num_symbols;
    symbols = alist->optionals.symbols;
    defaults = alist->optionals.defaults;
    sforms = alist->optionals.sforms;
    for (; i < count && CONS_P(values); i++, values = CDR(values)) {
	ComPush(com, symbols[i], CAR(values), eval, builtin, compile);
	if (sforms[i])
	    ComPush(com, sforms[i], T, 0, builtin, compile);
    }
    for (; i < count; i++) {
	if (!builtin) {
	    /* Keep track of lexical environment */
	    int head = mac->env.head;
	    int lex = mac->env.lex;

	    mac->env.lex = base;
	    mac->env.head = mac->env.length;
	    ComPush(com, symbols[i], defaults[i], eval, 0, compile);
	    mac->env.head = head;
	    mac->env.lex = lex;
	}
	else
	    ComPush(com, symbols[i], defaults[i], eval, 1, compile);
	if (sforms[i])
	    ComPush(com, sforms[i], NIL, 0, builtin, compile);
    }

    switch (*desc++) {
	case 'k':
	    goto key_label;
	case 'r':
	    goto rest_label;
	case 'a':
	    goto aux_label;
	default:
	    goto done_label;
    }


    /* &KEY */
key_label:
    {
	int varset;
	LispObj *val, *karg, **keys;

	count = alist->keys.num_symbols;
	symbols = alist->keys.symbols;
	defaults = alist->keys.defaults;
	sforms = alist->keys.sforms;
	keys = alist->keys.keys;

	/* Check if arguments are correctly specified */
	for (karg = values; CONS_P(karg); karg = CDR(karg)) {
	    val = CAR(karg);
	    if (SYMBOL_P(val) && KEYWORD_P(val)) {
		for (i = 0; i < alist->keys.num_symbols; i++)
		    if (!keys[i] && symbols[i] == val)
			break;
	    }

	    else if (!builtin &&
		     val->type == LispQuote_t && SYMBOL_P(val->data.quote)) {
		for (i = 0; i < alist->keys.num_symbols; i++)
		    if (keys[i] && ATOMID(keys[i]) == ATOMID(val->data.quote))
			break;
	    }

	    else
		/* Just make the error test true */
		i = alist->keys.num_symbols;

	    if (i == alist->keys.num_symbols) {
		/* If not in argument specification list... */
		char function_name[36];

		strcpy(function_name, STROBJ(name));
		LispDestroy(mac, "%s: invalid keyword %s",
			    function_name, STROBJ(val));
	    }

	    karg = CDR(karg);
	    if (!CONS_P(karg))
		LispDestroy(mac, "%s: &KEY needs arguments as pairs",
			    STROBJ(name));
	}

	/* Add variables */
	for (i = 0; i < alist->keys.num_symbols; i++) {
	    val = defaults[i];
	    varset = 0;
	    if (!builtin && keys[i]) {
		Atom_id atom = ATOMID(keys[i]);

		/* Special keyword specification, need to compare ATOMID
		 * and keyword specification must be a quoted object */
		for (karg = values; CONS_P(karg); karg = CDR(karg)) {
		    val = CAR(karg);
		    if (val->type == LispQuote_t &&
			atom == ATOMID(val->data.quote)) {
			val = CADR(karg);
			varset = 1;
			break;
		    }
		    karg = CDR(karg);
		}
	    }

	    else {
		/* Normal keyword specification, can compare object pointers,
		 * as they point to the same object in the keyword package */
		for (karg = values; CONS_P(karg); karg = CDR(karg)) {
		    /* Don't check if argument is a valid keyword or
		     * special quoted keyword */
		    if (symbols[i] == CAR(karg)) {
			val = CADR(karg);
			varset = 1;
			break;
		    }
		    karg = CDR(karg);
		}
	    }

	    /* Add the variable to environment */
	    if (varset) {
		ComPush(com, symbols[i], val, eval, builtin, compile);
		if (sforms[i])
		    ComPush(com, sforms[i], T, 0, builtin, compile);
	    }
	    else {
		if (eval && !builtin) {
		    /* Keep track of lexical environment */
		    int head = mac->env.head;
		    int lex = mac->env.lex;

		    mac->env.lex = base;
		    mac->env.head = mac->env.length;
		    ComPush(com, symbols[i], val, eval, 0, compile);
		    mac->env.head = head;
		    mac->env.lex = lex;
		}
		else
		    ComPush(com, symbols[i], val, eval, builtin, compile);
		if (sforms[i])
		    ComPush(com, sforms[i], NIL, 0, builtin, compile);
	    }
	}
    }

    if (*desc == 'a') {
	/* &KEY uses all remaining arguments */
	values = NIL;
	goto aux_label;
    }
    goto finished_label;


    /* &REST */
rest_label:
    if (!eval || !CONS_P(values) || (compile && !builtin))
	ComPush(com, alist->rest, values, eval, builtin, compile);
    else {
	char *string;
	LispObj *list;
	int count, constantp;

	/* Count number of arguments and check if it is a list of constants */
	for (count = 0, constantp = 1, list = values;
	     CONS_P(list);
	     list = CDR(list), count++)
	    if (constantp && !ComConstantp(com, CAR(values)))
		constantp = 0;

	string = builtin ? STRPTR(name) : NULL;
	/* XXX FIXME should have a flag indicating if function call
	 * change the &REST arguments even if it is a constant list
	 * (or if the returned value may be changed). */
	if (string && (count < MAX_BCONS || constantp) &&
	    strcmp(string, "LIST") &&
	    strcmp(string, "APPLY") &&	/* XXX depends on function argument */
	    strcmp(string, "VECTOR")) {
	    if (constantp) {
		/* If the builtin function changes the &REST parameters, must
		 * define a Com_XXX function for it. */
		ComPush(com, alist->rest, values, 0, builtin, compile);
	    }
	    else {
		CompileStackEnter(com, count - 1, 1);
		for (; CONS_P(CDR(values)); values = CDR(values)) {
		    /* Evaluate this argument */
		    ComEval(com, CAR(values));
		    /* Save result in builtin stack */
		    com_Bytecode(com, XBC_PUSH);
		}
		CompileStackLeave(com, count - 1, 1);
		/* The last argument is not saved in the stack */
		ComEval(com, CAR(values));
		values = NIL;
		com_Bytecode(com, XBC_BCONS + (count - 1));
	    }
	}
	else {
	    /* Allocate a fresh list of cons */

	    /* Generate code to evaluate object */
	    ComEval(com, CAR(values));

	    com->stack.cpstack += 2;
	    if (com->stack.pstack < com->stack.cpstack)
		com->stack.pstack = com->stack.cpstack;
	    /* Start building a gc protected list, with the loaded value */
	    com_Bytecode(com, XBC_LSTAR);

	    values = CDR(values);
	    for (; CONS_P(values); values = CDR(values)) {
		/* Generate code to evaluate object */
		ComEval(com, CAR(values));

		/* Add loaded value to gc protected list */
		com_Bytecode(com, XBC_LCONS);
	    }

	    /* Finish gc protected list */
	    com_Bytecode(com, XBC_LFINI);

	    /* Push loaded value */
	    if (builtin)
		com_Bytecode(com, XBC_PUSH);
	    else {
		com_Let(com, alist->rest->data.atom);

		/* Remember this symbol will be bound */
		ComAddVariable(com, alist->rest, values);
	    }
	    com->stack.cpstack -= 2;
	}
    }
    if (*desc != 'a')
	goto finished_label;


    /* &AUX */
aux_label:
    i = 0;
    count = alist->auxs.num_symbols;
    symbols = alist->auxs.symbols;
    defaults = alist->auxs.initials;
    if (!builtin && !compile && eval) {
	/* Keep track of lexical environment */
	int lex = mac->env.lex;

	mac->env.lex = base;
	mac->env.head = mac->env.length;

	/* Make the variables added so far available in the bytecode */
	com_Bind(com, mac->env.length - base);

	for (; i < count; i++) {
	    ComPush(com, symbols[i], defaults[i], 1, 0, compile);
	    /* Make this variable available */
	    com_Bind(com, 1);
	    ++mac->env.head;
	}
	mac->env.lex = lex;

	/* XXX Do this to avoid confusing the interpreter. Should find a
	 * way to avoid this... */
	com_Unbind(com, mac->env.length - base);
    }
    else {
	for (; i < count; i++)
	    ComPush(com, symbols[i], defaults[i], eval, builtin, compile);
    }

done_label:
    if (CONS_P(values))
	LispDestroy(mac, "%s: too many arguments", STROBJ(name));

finished_label:
    if (builtin)
	mac->stack.base = base;
    else
	mac->env.head = mac->env.length;

    return (base);
}

static void
ComFuncall(LispCom *com, LispObj *function, LispObj *arguments, int eval)
{
    int base, compile;
    LispAtom *atom;
    LispArgList *alist;
    LispBuiltin *builtin;
    LispObj *lambda;
    LispMac *mac = com->mac;

    switch (function->type) {
	case LispAtom_t:
	    atom = function->data.atom;
	    alist = atom->property->alist;

	    if (atom->a_builtin) {
		builtin = atom->property->fun.builtin;
		compile = builtin->compile != NULL;

		/*  If one of:
		 * 	o expanding a macro
		 *	o calling a builtin special form
		 *	o builtin function is a macro
		 * don't evaluate arguments. */
		if (com->macro || compile || builtin->type == LispMacro)
		    eval = 0;

		FORM_ENTER();
		if (!compile && !com->macro)
		    CompileStackEnter(com, alist->num_arguments, 1);

		/* Build argument list in the interpreter stacks */
		base = ComCall(com, alist, function, arguments,
			       eval, 1, compile);

		/* If <compile> is set, it is a special form */
		if (compile)
		    builtin->compile(com, builtin);

		/* Else, generate opcodes to call builtin function */
		else {
		    com_Call(com, alist->num_arguments, builtin);
		    CompileStackLeave(com, alist->num_arguments, 1);
		}
		mac->stack.base = mac->stack.length = base;
		FORM_LEAVE();
	    }
	    else if (atom->a_function) {
		int macro;

		lambda = atom->property->fun.function;
		macro = lambda->data.lambda.type == LispMacro;

		/* If <macro> is set, expand macro */
		if (macro)
		    ComMacroCall(com, alist, function, lambda, arguments);

		else {
		    if (com->block->type == LispBlockClosure &&
			com->block->tag == function)
			ComRecursiveCall(com, alist, function, arguments);
		    else
			ComInlineCall(com, alist, function, arguments,
				      CDR(lambda->data.lambda.code));
		}
	    }
	    else if (atom->a_defstruct &&
		     atom->property->structure.function != STRUCT_NAME &&
		     atom->property->structure.function != STRUCT_CONSTRUCTOR) {
		LispObj *definition = atom->property->structure.definition;

		if (!CONS_P(arguments) || CONS_P(CDR(arguments)))
		    LispDestroy(mac, "%s: too %s arguments", atom->string,
				CONS_P(arguments) ? "many" : "few");

		ComEval(com, CAR(arguments));
		if (atom->property->structure.function == STRUCT_CHECK)
		    com_Structp(com, definition);
		else
		    com_Struct(com,
			       atom->property->structure.function, definition);
	    }
	    else {
		/* Not yet defined function/macro. */
		++com->warnings;
		LispWarning(mac, "call to undefined function %s", atom->string);
		com_Funcall(com, function, arguments);
	    }
	    break;

	case LispLambda_t:
	    /* XXX TODO compile inline */
	    com_Funcall(com, function, arguments);
	    break;

	case LispCons_t:
	    /* XXX TODO compile inline */
	    com_Funcall(com, function, arguments);
	    break;

	default:
	    /*  XXX If bytecode objects are made available, should
	     * handle it here. */
	    LispDestroy(mac, "EVAL: %s is invalid as a function",
			STROBJ(function));
	    /*NOTREACHED*/
	    break;
    }
}

/* Generate opcodes for an implicit PROGN */
static void
ComProgn(LispCom *com, LispObj *code)
{
    if (CONS_P(code)) {
	for (; CONS_P(code); code = CDR(code))
	    ComEval(com, CAR(code));
    }
    else
	/* If no code to execute, empty PROGN returns NIL */
	com_Bytecode(com, XBC_NIL);
}

/* Generate opcodes to evaluate <object>. */
static void
ComEval(LispCom *com, LispObj *object)
{
    int offset;
    LispObj *form;

    switch (object->type) {
	case LispAtom_t:
	    if (IN_TAGBODY())
		ComLabel(com, object);
	    else {
		offset = ComGetVariable(com, object);
		if (offset >= 0)
		    /* Load from user stack at relative offset */
		    com_Load(com, offset);
		else if (offset == SYMBOL_KEYWORD)
		    com_LoadCon(com, object);
		else if (offset == SYMBOL_CONSTANT)
		    /* Symbol defined as constant, just load it's value */
		    com_LoadCon(com, LispGetVar(com->mac, object));
		else
		    /* Load value bound to symbol at run time */
		    com_LoadSym(com, object->data.atom);
	    }
	    break;

	case LispCons_t: {
	    /* Macro expansion may be done in the object form */
	    form = com->form;
	    com->form = object;
	    ComFuncall(com, CAR(object), CDR(object), 1);
	    com->form = form;
	}   break;

	case LispQuote_t:
	    com_LoadCon(com, object->data.quote);
	    break;

	case LispBackquote_t:
	    /* Macro expansion is stored in the current value of com->form */
	    ComMacroBackquote(com, object);
	    break;

	case LispComma_t:
	    LispDestroy(com->mac, "EVAL: comma outside of backquote");
	    break;

	case LispInteger_t:
	    if (IN_TAGBODY()) {
		ComLabel(com, object);
		break;
	    }
	    /*FALLTROUGH*/

	default:
	    /* Constant object */
	    com_LoadCon(com, object);
	    break;
    }
}

/***********************************************************************
 * Lambda expansion helper functions
 ***********************************************************************/
static void
ComRecursiveCall(LispCom *com, LispArgList *alist,
		 LispObj *name, LispObj *arguments)
{
    LispMac *mac = com->mac;
    int base, lex;

    /* Save state */
    lex = mac->env.lex;

    FORM_ENTER();

    /* Generate code to push function arguments in the stack */
    base = ComCall(com, alist, name, arguments, 1, 0, 0);

    /* Stack will grow this amount */
    CompileStackEnter(com, alist->num_arguments, 0);

    /* Make the variables available at run time */
    com_Bind(com, alist->num_arguments);
    com->block->bind += alist->num_arguments;

    com_BytecodeChar(com, XBC_LETREC, alist->num_arguments);

    /* The variables are now unbound */
    com_Unbind(com, alist->num_arguments);
    com->block->bind -= alist->num_arguments;

    /* Stack length is reduced */
    CompileStackLeave(com, alist->num_arguments, 0);
    FORM_LEAVE();

    /* Restore state */
    mac->env.lex = lex;
    mac->env.head = mac->env.length = base;
}

static void
ComInlineCall(LispCom *com, LispArgList *alist,
	      LispObj *name, LispObj *arguments, LispObj *lambda)
{
    LispMac *mac = com->mac;
    int base, lex;

    /* Save state */
    lex = mac->env.lex;

    FORM_ENTER();
    /* Start the inline function block */
    CompileIniBlock(com, LispBlockClosure, name);

    /* Generate code to push function arguments in the stack */
    base = ComCall(com, alist, name, arguments, 1, 0, 0);

    /* Stack will grow this amount */
    CompileStackEnter(com, alist->num_arguments, 0);

    /* Make the variables available at run time */
    com_Bind(com, alist->num_arguments);
    com->block->bind += alist->num_arguments;

    /* Expand the lambda list */
    ComProgn(com, lambda);

    /* The variables are now unbound */
    com_Unbind(com, alist->num_arguments);
    com->block->bind -= alist->num_arguments;

    /* Stack length is reduced */
    CompileStackLeave(com, alist->num_arguments, 0);

    /* Finish the inline function block */
    CompileFiniBlock(com);
    FORM_LEAVE();

    /* Restore state */
    mac->env.lex = lex;
    mac->env.head = mac->env.length = base;
}

/***********************************************************************
 * Macro expansion helper functions.
 ***********************************************************************/
static LispObj *
ComMacroExpandBackquote(LispCom *com, LispObj *object)
{
    return (LispEvalBackquote(com->mac, object->data.quote, 1));
}

static LispObj *
ComMacroExpandFuncall(LispCom *com, LispObj *function, LispObj *arguments)
{
    return (LispFuncall(com->mac, function, arguments, 1));
}

static LispObj *
ComMacroExpandEval(LispCom *com, LispObj *object)
{
    LispMac *mac = com->mac;
    LispObj *result;

    switch (object->type) {
	case LispAtom_t:
	    result = LispGetVar(mac, object);

	    /* Macro expansion requires bounded symbols */
	    if (result == NULL)
		LispDestroy(mac, "EVAL: the variable %s is unbound",
			    STROBJ(object));
	    break;

	case LispCons_t:
	    result = ComMacroExpandFuncall(com, CAR(object), CDR(object));
	    break;

	case LispQuote_t:
	    result = object->data.quote;
	    break;

	case LispBackquote_t:
	    result = ComMacroExpandBackquote(com, object);
	    break;

	case LispComma_t:
	    LispDestroy(com->mac, "EVAL: comma outside of backquote");

	default:
	    result = object;
	    break;
    }

    return (result);
}

static LispObj *
ComMacroExpand(LispCom *com, LispObj *lambda)
{
    LispObj *result, **presult = &result, **plambda;
    int jumped, *pjumped = &jumped, backquote, *pbackquote = &backquote;
    LispMac *mac = com->mac;
    LispBlock *block;

    int interpreter_lex, interpreter_head, interpreter_base;

    /* Save interpreter state */
    interpreter_base = mac->stack.length;
    interpreter_head = mac->env.length;
    interpreter_lex = mac->env.lex;

    /* Use the variables */
    plambda = &lambda;
    *presult = NIL;
    *pjumped = 1;
    *pbackquote = !CONS_P(lambda);

    block = LispBeginBlock(mac, NIL, LispBlockProtect);
    if (setjmp(block->jmp) == 0) {
	if (!backquote) {
	    for (; CONS_P(lambda); lambda = CDR(lambda))
		result = ComMacroExpandEval(com, CAR(lambda));
	}
	else
	    result = ComMacroExpandBackquote(com, lambda);

	*pjumped = 0;
    }
    LispEndBlock(mac, block);

    /* If tried to jump out of the macro expansion block */
    if (!mac->destroyed && jumped)
	LispDestroy(mac, "*** EVAL: bad jump in macro expansion");

    /* Macro expansion did something wrong */
    if (mac->destroyed) {
	LispMessage(mac, "*** EVAL: aborting macro expansion");
	LispDestroy(mac, NULL);
    }

    /* Restore interpreter state */
    mac->env.lex = interpreter_lex;
    mac->stack.length = interpreter_base;
    mac->env.head = mac->env.length = interpreter_head;

    return (result);
}

static void
ComMacroCall(LispCom *com, LispArgList *alist,
	     LispObj *name, LispObj *lambda, LispObj *arguments)
{
    int base;
    LispObj *body;
    LispMac *mac = com->mac;

    ++com->macro;
    base = ComCall(com, alist, name, arguments, 0, 0, 0);
    body = CDR(lambda->data.lambda.code);
    body = ComMacroExpand(com, body);
    --com->macro;
    mac->env.head = mac->env.length = base;

    /* Macro is expanded, store the result */
    CAR(com->form) = body;
    ComEval(com, body);
}

static void
ComMacroBackquote(LispCom *com, LispObj *lambda)
{
    LispObj *body;

    ++com->macro;
    body = ComMacroExpand(com, lambda);
    --com->macro;

    /* Macro is expanded, store the result */
    CAR(com->form) = body;

    com_LoadCon(com, body);
}

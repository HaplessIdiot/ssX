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

/* $XFree86: xc/programs/xedit/lisp/package.c,v 1.4 2002/02/27 06:56:36 paulo Exp $ */

#include "package.h"
#include "private.h"

/*
 * Prototypes
 */
static int LispDoSymbol(LispMac*, LispObj*, LispAtom*, int, int);
static LispObj *LispReallyDoSymbols(LispMac*, LispBuiltin*, int, int);
static LispObj *LispDoSymbols(LispMac*, LispBuiltin*, int, int);
static LispObj *LispFindPackageOrDie(LispMac*, LispBuiltin*, LispObj*);
static void LispDoExport(LispMac*, LispBuiltin*, LispObj*, LispObj*, int);
static void LispDoImport(LispMac*, LispBuiltin*, LispObj*);

/*
 * Initialization
 */
extern LispProperty *NOPROPERTY;
static LispObj *Kinternal, *Kexternal, *Kinherited;

/*
 * Implementation
 */
void
LispPackageInit(LispMac *mac)
{
    Kinternal	= KEYWORD("INTERNAL");
    Kexternal	= KEYWORD("EXTERNAL");
    Kinherited	= KEYWORD("INHERITED");
}

LispObj *
LispFindPackageFromString(LispMac *mac, char *string)
{
    LispObj *list, *package, *nick;

    for (list = PACK; CONS_P(list); list = CDR(list)) {
	package = CAR(list);
	if (strcmp(THESTR(package->data.package.name), string) == 0)
	    return (package);
	for (nick = package->data.package.nicknames;
	     CONS_P(nick); nick = CDR(nick))
	    if (strcmp(THESTR(CAR(nick)), string) == 0)
		return (package);
    }

    return (NIL);
}

LispObj *
LispFindPackage(LispMac *mac, LispObj *name)
{
    char *string = NULL;

    if (PACKAGE_P(name))
	return (name);

    if (SYMBOL_P(name))
	string = STRPTR(name);
    else if (STRING_P(name))
	string = THESTR(name);
    else
	LispDestroy(mac, "FIND-PACKAGE: %s is not a string or symbol",
		    STROBJ(name));

    return (LispFindPackageFromString(mac, string));
}

/*   This function is used to avoid some namespace polution caused by the
 * way builtin functions are created, all function name arguments enter
 * the current package, but most of them do not have a property */
static int
LispDoSymbol(LispMac *mac, LispObj *package, LispAtom *atom,
	     int if_extern, int all_packages)
{
    int dosymbol;

    /* condition 1: atom package is current package */
    dosymbol = !all_packages || atom->package == package;
    if (dosymbol) {
	/* condition 2: intern and extern symbols or symbol is extern */
	dosymbol = !if_extern || atom->ext;
	if (dosymbol) {
	    /* condition 3: atom has properties or is in
	     * the current package */
	    dosymbol = atom->property != NOPROPERTY ||
		       package == mac->keyword ||
		       package == PACKAGE;
	}
    }

    return (dosymbol);
}

static LispObj *
LispFindPackageOrDie(LispMac *mac, LispBuiltin *builtin, LispObj *name)
{
    LispObj *package;

    package = LispFindPackage(mac, name);

    if (package == NIL)
	LispDestroy(mac, "%s: package %s is not available",
		    STRFUN(builtin), STROBJ(name));

    return (package);
}

/* package must be of type LispPackage_t, symbol type is checked
   bypass lisp.c:LispExportSymbol() */
static void
LispDoExport(LispMac *mac, LispBuiltin *builtin,
	     LispObj *package, LispObj *symbol, int export)
{
    ERROR_CHECK_SYMBOL(symbol);
    if (!export) {
	if (package == mac->keyword ||
	    symbol->data.atom->package == mac->keyword)
	    LispDestroy(mac, "%s: symbol %s cannot be unexported",
			STRFUN(builtin), STROBJ(symbol));
    }

    if (package == PACKAGE)
	symbol->data.atom->ext = export ? LispTrue_t : LispNil_t;
    else {
	int i;
	char *string;
	LispAtom *atom;
	LispPackage *pack;

	string = STRPTR(symbol);
	pack = package->data.package.package;

	for (i = 0; i < STRTBLSZ; i++) {
	    atom = pack->atoms[i];
	    while (atom) {
		if (strcmp(atom->string, string) == 0) {
		    atom->ext = export ? LispTrue_t : LispNil_t;
		    return;
		}

		atom = atom->next;
	    }
	}

	LispDestroy(mac, "%s: the symbol %s is not available in package %s",
		    STRFUN(builtin), STROBJ(symbol),
		    THESTR(package->data.package.name));
    }
}

static void
LispDoImport(LispMac *mac, LispBuiltin *builtin, LispObj *symbol)
{
    ERROR_CHECK_SYMBOL(symbol);
    LispImportSymbol(mac, symbol);
}

static LispObj *
LispReallyDoSymbols(LispMac *mac, LispBuiltin *builtin,
		    int only_externs, int all_symbols)
{
    int i;
    LispPackage *pack = NULL;
    LispAtom *atom, *next_atom;
    LispObj *variable, *package = NULL, *list, *code, *result_form;

    LispObj *init, *body;

    body = ARGUMENT(1);
    init = ARGUMENT(0);
    MACRO_ARGUMENT2();

    /* Prepare for loop */
    ERROR_CHECK_LIST(init);
    variable = CAR(init);
    ERROR_CHECK_SYMBOL(variable);

    if (!all_symbols) {
	/* if all_symbols, a package name is not specified in the init form */

	init = CDR(init);
	if (!CONS_P(init))
	    LispDestroy(mac, "%s: missing package name");

	/* Evaluate package specification */
	package = EVAL(CAR(init));
	if (!PACKAGE_P(package))
	    package = LispFindPackageOrDie(mac, builtin, package);

	pack = package->data.package.package;
    }

    result_form = NIL;

    init = CDR(init);
    if (CONS_P(init))
	result_form = init;

    /* Initialize iteration variable */
    LispAddVar(mac, variable, NIL);
    mac->env.head += 2;

    for (list = PACK; CONS_P(list); list = CDR(list)) {
	if (all_symbols) {
	    package = CAR(list);
	    pack = package->data.package.package;
	}

	/* Traverse the symbol list, executing body */
	for (i = 0; i < STRTBLSZ; i++) {
	    atom = pack->atoms[i];
	    while (atom) {
		/* Save pointer to next atom. If variable is removed,
		 * predicatable result is only guaranteed if the bound
		 * variable is removed. */
		next_atom = atom->next;

		if (LispDoSymbol(mac, package, atom, only_externs, all_symbols)) {
		    LispSetVar(mac, variable, atom->object);
		    for (code = body; CONS_P(code); code = CDR(code))
			EVAL(CAR(code));
		}

		atom = next_atom;
	    }
	}

	if (!all_symbols)
	    break;
    }

    /* Variable is still bound */
    for (code = result_form; CONS_P(code); code = CDR(code))
	EVAL(CAR(code));

    return (NIL);
}

static LispObj *
LispDoSymbols(LispMac *mac, LispBuiltin *builtin,
	      int only_externs, int all_symbols)
{
    int did_jump, *pdid_jump = &did_jump;
    LispObj *result, **presult = &result;
    LispBlock *block;

    *presult = NIL;
    *pdid_jump = 1;
    block = LispBeginBlock(mac, NIL, LispBlockTag);
    if (setjmp(block->jmp) == 0) {
	*presult = LispReallyDoSymbols(mac, builtin, only_externs, all_symbols);
	*pdid_jump = 0;
    }
    LispEndBlock(mac, block);
    if (*pdid_jump)
	*presult = mac->block.block_ret;

    return (*presult);
}

LispObj *
Lisp_DoAllSymbols(LispMac *mac, LispBuiltin *builtin)
/*
 do-all-symbols init &rest body
 */
{
    return (LispDoSymbols(mac, builtin, 0, 1));
}

LispObj *
Lisp_DoExternalSymbols(LispMac *mac, LispBuiltin *builtin)
/*
 do-external-symbols init &rest body
 */
{
    return (LispDoSymbols(mac, builtin, 1, 0));
}

LispObj *
Lisp_DoSymbols(LispMac *mac, LispBuiltin *builtin)
/*
 do-symbols init &rest body
 */
{
    return (LispDoSymbols(mac, builtin, 0, 0));
}

LispObj *
Lisp_FindAllSymbols(LispMac *mac, LispBuiltin *builtin)
/*
 find-all-symbols string-or-symbol
 */
{
    char *string = NULL;
    LispAtom *atom;
    LispPackage *pack;
    LispObj *list, *package, *result;
    int i, protect = mac->protect.length;

    LispObj *string_or_symbol;

    string_or_symbol = ARGUMENT(0);

    if (STRING_P(string_or_symbol))
	string = THESTR(string_or_symbol);
    else if (SYMBOL_P(string_or_symbol))
	string = STRPTR(string_or_symbol);
    else
	LispDestroy(mac, "%s: %s is not a string or symbol",
		    STRFUN(builtin), STROBJ(string_or_symbol));

    result = NIL;

    /* Will find at least one symbol matching string-or-symbol,
     * the value of string-or-symbol itself :-) */
    if (protect + 1 >= mac->protect.space)
	LispMoreProtects(mac);

    /* Traverse all packages, searching for symbols matching specified string */
    for (list = PACK; CONS_P(list); list = CDR(list)) {
	package = CAR(list);
	pack = package->data.package.package;

	for (i = 0; i < STRTBLSZ; i++) {
	    atom = pack->atoms[i];
	    while (atom) {

		if (LispDoSymbol(mac, package, atom, 0, 1)) {
		    /* Return only one pointer to a matching symbol */

		    if (strcmp(atom->string, string) == 0) {
			if (result == NIL) {
			    result = CONS(atom->object, NIL);
			    mac->protect.objects[mac->protect.length++] = result;
			}
			else {
			    /* Put symbols defined first in the
			     * beginning of the result list */
			    CDR(result) = CONS(CAR(result), CDR(result));
			    CAR(result) = atom->object;
			}
		    }
		}
		atom = atom->next;
	    }
	}
    }

    return (result);
}

LispObj *
Lisp_FindPackage(LispMac *mac, LispBuiltin *builtin)
/*
 find-package name
 */
{
    LispObj *name;

    name = ARGUMENT(0);

    return (LispFindPackage(mac, name));
}

LispObj *
Lisp_Export(LispMac *mac, LispBuiltin *builtin)
/*
 export symbols &optional package
 */
{
    LispObj *list;

    LispObj *symbols, *package;

    package = ARGUMENT(1);
    symbols = ARGUMENT(0);

    /* If specified, make sure package is available */
    if (package != NIL)
	package = LispFindPackageOrDie(mac, builtin, package);
    else
	package = PACKAGE;

    /* Export symbols */
    if (CONS_P(symbols)) {
	for (list = symbols; CONS_P(list); list = CDR(list))
	    LispDoExport(mac, builtin, package, CAR(list), 1);
    }
    else
	LispDoExport(mac, builtin, package, symbols, 1);

    return (T);
}

LispObj *
Lisp_Import(LispMac *mac, LispBuiltin *builtin)
/*
 import symbols &optional package
 */
{
    int restore_package;
    LispPackage *savepack = NULL;
    LispObj *list, *savepackage = NULL;

    LispObj *symbols, *package;

    package = ARGUMENT(1);
    symbols = ARGUMENT(0);

    /* If specified, make sure package is available */
    if (package != NIL)
	package = LispFindPackageOrDie(mac, builtin, package);
    else
	package = PACKAGE;

    restore_package = package != PACKAGE;
    if (restore_package) {
	/* Save package environment */
	savepackage = PACKAGE;
	savepack = mac->pack;

	/* Change package environment */
	PACKAGE = package;
	mac->pack = package->data.package.package;
    }

    /* Export symbols */
    if (CONS_P(symbols)) {
	for (list = symbols; CONS_P(list); list = CDR(list))
	    LispDoImport(mac, builtin, CAR(list));
    }
    else
	LispDoImport(mac, builtin, symbols);

    if (restore_package) {
	/* Restore package environment */
	PACKAGE = savepackage;
	mac->pack = savepack;
    }

    return (T);
}

LispObj *
Lisp_InPackage(LispMac *mac, LispBuiltin *builtin)
/*
 in-package name
 */
{
    LispObj *package;

    LispObj *name;

    name = ARGUMENT(0);

    package = LispFindPackageOrDie(mac, builtin, name);

    /* Update pointer to package symbol table */
    mac->pack = package->data.package.package;
    PACKAGE = package;

    LispSetVar(mac, PACKNAM, package);

    return (package);
}

LispObj *
Lisp_Intern(LispMac *mac, LispBuiltin *builtin)
/*
 intern string &optional package
 */
{
    int i;
    char *ptr;
    LispAtom *atom;
    LispObj *symbol;
    LispPackage *pack;

    LispObj *string, *package;

    package = ARGUMENT(1);
    string = ARGUMENT(0);

    ERROR_CHECK_STRING(string);
    if (package != NIL)
	package = LispFindPackageOrDie(mac, builtin, package);
    else
	package = PACKAGE;

    /* If got here, package is a LispPackage_t */
    pack = package->data.package.package;

    /* Search symbol in specified package */
    ptr = THESTR(string);
    for (i = 0, symbol = NULL; i < STRTBLSZ && symbol == NULL; i++) {
	atom = pack->atoms[i];
	while (atom) {
	    if (strcmp(atom->string, ptr) == 0) {
		symbol = atom->object;
		break;
	    }
	    atom = atom->next;
	}
    }

    RETURN_CHECK(1);
    if (symbol == NULL) {
	/* symbol does not exist in the specified package, create a new
	 * internal symbol */
	int unreadable;

	if (package == PACKAGE)
	    symbol = ATOM(ptr);
	else {
	    LispPackage *savepack;
	    LispObj *savepackage;

	    /* Save package environment */
	    savepackage = PACKAGE;
	    savepack = mac->pack;

	    /* Change package environment */
	    PACKAGE = package;
	    mac->pack = package->data.package.package;

	    symbol = ATOM(ptr);

	    /* Restore package environment */
	    PACKAGE = savepackage;
	    mac->pack = savepack;
	}

	/* Check for zero length string */
	unreadable = *ptr == '\0';

	/* Check if string can be safely read back */
	for (; *ptr; ptr++)
	    if (islower(*ptr) || *ptr == '"' || *ptr == '\\' || *ptr == ';' ||
		*ptr == '#' || *ptr == ',' || *ptr == '@' || *ptr == '(' ||
		*ptr == ')' || *ptr == '`' || *ptr == '\'' || *ptr == '|' ||
		*ptr == ':') {
		unreadable = 1;
		break;
	    }

	symbol->data.atom->unreadable = unreadable;
	/* If symbol being create in the keyword package, make it external */
	if (package == mac->keyword)
	    symbol->data.atom->ext = LispTrue_t;
	RETURN(0) = NIL;
    }
    else {
	if (symbol->data.atom->package == package)
	    RETURN(0) = symbol->data.atom->ext ? Kexternal : Kinternal;
	else
	    RETURN(0) = Kinherited;
    }

    RETURN_COUNT = 1;

    return (symbol);
}

LispObj *
Lisp_ListAllPackages(LispMac *mac, LispBuiltin *builtin)
/*
 list-all-packages
 */
{
    /*   Maybe this should be read-only or a copy of the package list.
     *   But, if properly implemented, it should be possible to (rplaca)
     * this variable from lisp code with no problems. Don't do it at home. */

    return (PACK);
}

LispObj *
Lisp_MakePackage(LispMac *mac, LispBuiltin *builtin)
/*
 make-package package-name &key nicknames (use '(lisp ext))
 */
{
    int protect;
    LispObj *list, *package, *nicks, *cons, *savepackage;

    LispObj *package_name, *nicknames, *use;

    use = ARGUMENT(2);
    nicknames = ARGUMENT(1);
    package_name = ARGUMENT(0);

    protect = mac->protect.length;
    if (protect + 2 >= mac->protect.space)	
	LispMoreProtects(mac);

    /* Check if package already exists */
    package = LispFindPackage(mac, package_name);
    if (package != NIL)
	LispDestroy(mac, "%s: package %s already defined",
		    STRFUN(builtin), STROBJ(package_name));

    /* Error checks done, package_name is either a symbol or string */
    if (!STRING_P(package_name))
	package_name = STRING(STRPTR(package_name));

    mac->protect.objects[mac->protect.length++] = package_name;

    /* Check nicknames */
    nicks = cons = NIL;
    for (list = nicknames; CONS_P(list); list = CDR(list)) {
	package = LispFindPackage(mac, CAR(list));
	if (package != NIL)
	    LispDestroy(mac, "%s: nickname %s matches package %s",
			STRFUN(builtin), STROBJ(CAR(list)),
			THESTR(package->data.package.name));
	/* Store all nicknames as strings */
	package = CAR(list);
	if (!STRING_P(package))
	    package = STRING(STRPTR(package));
	if (nicks == NIL) {
	    nicks = cons = CONS(package, NIL);
	    mac->protect.objects[mac->protect.length++] = nicks;
	}
	else {
	    CDR(cons) = CONS(package, NIL);
	    cons = CDR(cons);
	}
    }

    /* Check use list */
    for (list = use; CONS_P(list); list = CDR(list))
	(void)LispFindPackageOrDie(mac, builtin, CAR(list));

    /* No errors, create new package */
    package = LispNewPackage(mac, package_name, nicks);

    /* Update list of packages */
    PACK = CONS(package, PACK);

    /* No need for gc protection anymore */
    mac->protect.length = protect;

    /* Import symbols from use list */
    savepackage = PACKAGE;
    PACKAGE = package;
    LispSetVar(mac, PACKNAM, PACKAGE);
    for (list = use; CONS_P(list); list = CDR(list))
	LispUsePackage(mac, LispFindPackage(mac, CAR(list)));
    PACKAGE = savepackage;
    LispSetVar(mac, PACKNAM, PACKAGE);

    return (package);
}

LispObj *
Lisp_PackageName(LispMac *mac, LispBuiltin *builtin)
/*
 package-name package
 */
{
    LispObj *package;

    package = ARGUMENT(0);

    package = LispFindPackageOrDie(mac, builtin, package);

    return (package->data.package.name);
}

LispObj *
Lisp_PackageNicknames(LispMac *mac, LispBuiltin *builtin)
/*
 package-nicknames package
 */
{
    LispObj *package;

    package = ARGUMENT(0);

    package = LispFindPackageOrDie(mac, builtin, package);

    return (package->data.package.nicknames);
}

LispObj *
Lisp_PackageUseList(LispMac *mac, LispBuiltin *builtin)
/*
 package-use-list package
 */
{
    /*  If the variable returned by this function is expected to be changeable,
     * need to change the layout of the LispPackage structure. */

    LispPackage *pack;
    LispObj *package, *use, *cons;

    package = ARGUMENT(0);

    package = LispFindPackageOrDie(mac, builtin, package);

    use = cons = NIL;
    pack = package->data.package.package;

    if (pack->use.length) {
	int i = pack->use.length - 1, protect = mac->protect.length;

	use = cons = CONS(pack->use.pairs[i], NIL);
	if (protect + 1 >= mac->protect.space)
	    LispMoreProtects(mac);
	mac->protect.objects[mac->protect.length++] = use;
	for (--i; i >= 0; i--) {
	    CDR(cons) = CONS(pack->use.pairs[i], NIL);
	    cons = CDR(cons);
	}
	mac->protect.length = protect;
    }

    return (use);
}

LispObj *
Lisp_PackageUsedByList(LispMac *mac, LispBuiltin *builtin)
/*
 package-used-by-list package
 */
{
    int i, protect;
    LispPackage *pack;
    LispObj *package, *other, *used, *cons, *list;

    package = ARGUMENT(0);

    package = LispFindPackageOrDie(mac, builtin, package);

    used = cons = NIL;

    protect = mac->protect.length;
    if (protect + 1 >= mac->protect.space)
	LispMoreProtects(mac);

    for (list = PACK; CONS_P(list); list = CDR(list)) {
	other = CAR(list);
	if (package == other)
	    /* Surely package uses itself */
	    continue;

	pack = other->data.package.package;

	for (i = 0; i < pack->use.length; i++) {
	    if (pack->use.pairs[i] == package) {
		if (used == NIL) {
		    used = cons = CONS(other, NIL);
		    mac->protect.objects[mac->protect.length++] = used;
		}
		else {
		    CDR(cons) = CONS(other, NIL);
		    cons = CDR(cons);
		}
	    }
	}
    }

    mac->protect.length = protect;

    return (used);
}

LispObj *
Lisp_Unexport(LispMac *mac, LispBuiltin *builtin)
/*
 unexport symbols &optional package
 */
{
    LispObj *list;

    LispObj *symbols, *package;

    package = ARGUMENT(1);
    symbols = ARGUMENT(0);

    /* If specified, make sure package is available */
    if (package != NIL)
	package = LispFindPackageOrDie(mac, builtin, package);
    else
	package = PACKAGE;

    /* Export symbols */
    if (CONS_P(symbols)) {
	for (list = symbols; CONS_P(list); list = CDR(list))
	    LispDoExport(mac, builtin, package, CAR(list), 0);
    }
    else
	LispDoExport(mac, builtin, package, symbols, 0);

    return (T);
}

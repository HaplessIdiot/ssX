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

/* $XFree86: xc/programs/xedit/lisp/struct.c,v 1.7 2002/01/31 04:33:28 paulo Exp $ */

#include "struct.h"

/*
 * Initialization
 */
LispObj *Omake_struct, *Ostruct_access, *Ostruct_store, *Ostruct_type;

Atom_id Smake_struct, Sstruct_access, Sstruct_store, Sstruct_type;

/*
 * Implementation
 */
LispObj *
Lisp_Defstruct(LispMac *mac, LispBuiltin *builtin)
/*
 defstruct name &rest description
 */
{
    int intern;
    LispAtom *atom;
    int i, size, length, slength;
    char *name, *strname, *sname;
    LispObj *list, *object, *definition, *documentation;

    LispObj *oname, *description;

    description = ARGUMENT(1);
    oname = ARGUMENT(0);
    MACRO_ARGUMENT2();

    /* get structure name */
    if (!SYMBOL_P(oname) ||
	/* reserved name(s) */
	ATOMID(oname) == Sarray)
	LispDestroy(mac, "%s: %s cannot be a structure name",
		    STRFUN(builtin), STROBJ(oname));

    strname = STRPTR(oname);
    length = strlen(strname);

    if (CONS_P(description) && STRING_P(CAR(description))) {
	documentation = CAR(description);
	description = CDR(description);
    }
    else
	documentation = NIL;

    /* get structure fields and default values */
    for (list = description; CONS_P(list); list = CDR(list)) {
	object = CAR(list);

	if (CONS_P(object)) {
	    if ((CONS_P(CDR(object)) && CDR(CDR(object)) != NIL) ||
		(!CONS_P(CDR(object)) && CDR(object) != NIL))
	    LispDestroy(mac, "%s: bad initialization %s",
			STRFUN(builtin), STROBJ(object));
	    object = CAR(object);
	}
	if (!SYMBOL_P(object) || strcmp(STRPTR(object), "P") == 0)
	    /* p is invalid as a field name due to `type'-p */
	    LispDestroy(mac, "%s: %s cannot be a field for %s",
			LispStrObj(mac, CAR(list)), STRPTR(oname));

	/* check for repeated field names */
	for (object = description; object != list; object = CDR(object)) {
	    LispObj *left = CAR(object), *right = CAR(list);

	    if (CONS_P(left))
		left = CAR(left);
	    if (CONS_P(right))
		right = CAR(right);

	    if (ATOMID(left) == ATOMID(right))
		LispDestroy(mac, "%s: only one slot named :%s allowed",
			    STRFUN(builtin), STRPTR(left));
	}
    }

    /* XXX any memory allocation failure below should be a fatal error */

    definition = CONS(oname, description);
    atom = oname->data.atom;
    if (atom->a_defstruct)
	LispWarning(mac, "%s: structure %s is being redefined",
		    STRFUN(builtin), strname);
    LispSetAtomStructProperty(mac, atom, definition, STRUCT_NAME);

    /* check if symbol is exported */
    intern = !atom->ext;

	    /* MAKE- */
    size = length + 6;
    name = LispMalloc(mac, size);

    sprintf(name, "MAKE-%s", strname);
    atom = (object = ATOM(name))->data.atom;
    LispSetAtomStructProperty(mac, atom, definition, STRUCT_CONSTRUCTOR);
    if (!intern)
	LispExportSymbol(mac, object);

    sprintf(name, "%s-P", strname);
    atom = (object = ATOM(name))->data.atom;
    LispSetAtomStructProperty(mac, atom, definition, STRUCT_CHECK);
    if (!intern)
	LispExportSymbol(mac, object);

    for (i = 0, list = description; CONS_P(list); i++, list = CDR(list)) {
	if (CONS_P(CAR(list)))
	    sname = STRPTR(CAR(CAR(list)));
	else
	    sname = STRPTR(CAR(list));
	slength = strlen(sname);
	if (length + slength + 2 > size) {
	    size = length + slength + 2;
	    name = LispRealloc(mac, name, size);
	}
	sprintf(name, "%s-%s", strname, sname);
	atom = (object = ATOM(name))->data.atom;
	LispSetAtomStructProperty(mac, atom, definition, i);
	if (!intern)
	    LispExportSymbol(mac, object);
    }

    LispFree(mac, name);

    if (documentation != NIL)
	LispAddDocumentation(mac, oname, documentation, LispDocStructure);

    return (oname);
}

/* helper functions
 *	DONT explicitly call them. Non standard functions.
 */
LispObj *
Lisp_XeditMakeStruct(LispMac *mac, LispBuiltin *builtin)
/*
 xedit::make-struct atom &rest init
 */
{
    int nfld, ncvt, length = mac->protect.length;
    LispAtom *atom = NULL;

    LispObj *definition, *object, *field, *fields, *value, *cons, *list;
    LispObj *struc, *init;

    init = ARGUMENT(1);
    struc = ARGUMENT(0);

    field = cons = NIL;		/* fix gcc warning */

    if (!SYMBOL_P(struc) ||
	(atom = struc->data.atom)->a_defstruct == 0 ||
	 atom->property->structure.function != STRUCT_CONSTRUCTOR)
	LispDestroy(mac, "%s: invalid constructor %s",
		    STRFUN(builtin), STROBJ(struc));
    definition = atom->property->structure.definition;

    ncvt = nfld = 0;
    fields = NIL;

    /* check for errors in argument list */
    for (list = init, nfld = 0; CONS_P(list); list = CDR(list)) {
	if (!KEYWORD_P(CAR(list)))
	    LispDestroy(mac, "%s: %s is a invalid field for %s",
			STRPTR(struc), STROBJ(field),
			STRPTR(CAR(definition)));
	if (!CONS_P(CDR(list)))
	    LispDestroy(mac, "%s: values must be provided as pairs",
			STRPTR(struc));
	nfld++;
	list = CDR(list);
    }

    /* create structure, CAR(definition) is structure name */
    for (list = CDR(definition); CONS_P(list); list = CDR(list)) {
	Atom_id id;
	LispObj *defvalue = NIL;

	++nfld;
	field = CAR(list);
	if (CONS_P(field)) {
	    /* if default value provided */
	    if (CONS_P(CDR(field)))
		defvalue = CAR(CDR(field));
	    field = CAR(field);
	}
	id = ATOMID(field);

	for (object = init; CONS_P(object); object = CDR(object)) {
	    /* field is a keyword, test above checked it */
	    field = CAR(object);
	    if (id == ATOMID(field->data.quote)) {
		/* value provided */
		value = CAR(CDR(object));
		ncvt++;
		break;
	    }
	    object = CDR(object);
	}

	/* if no initialization given */
	if (!CONS_P(object)) {
	    /* if default value in structure definition */
	    if (defvalue != NIL)
		value = EVAL(defvalue);
	    else
		value = NIL;
	}

	if (fields == NIL) {
	    fields = cons = CONS(value, NIL);
	    if (length + 1 >= mac->protect.space)
		LispMoreProtects(mac);
	    mac->protect.objects[mac->protect.length++] = fields;
	}
	else {
	    CDR(cons) = CONS(value, NIL);
	    cons = CDR(cons);
	}
    }

    /* if not enough arguments were converted, need to check because
     * it is acceptable to set a field more than once, but in that case,
     * only the first value will be used. */
    if (nfld > ncvt) {
	for (list = init; CONS_P(list); list = CDR(list)) {
	    Atom_id id = ATOMID(CAR(list)->data.quote);

	    for (object = CDR(definition); CONS_P(object);
		 object = CDR(object)) {
		field = CAR(object);
		if (CONS_P(field))
		    field = CAR(field);
		if (ATOMID(field) == id)
		    break;
	    }
	    if (!CONS_P(object))
		LispDestroy(mac, "%s: %s is not a field for %s",
			    STRPTR(struc), STROBJ(CAR(list)),
			    STRPTR(CAR(definition)));
	    list = CDR(list);
	}
    }

    mac->protect.length = length;

    return (STRUCT(fields, definition));
}

LispObj *
Lisp_XeditStructAccess(LispMac *mac, LispBuiltin *builtin)
/*
 xedit::struct-access atom struct
 */
{
    int offset = 0;
    LispAtom *atom = NULL;

    LispObj *definition, *struc, *name, *list;

    struc = ARGUMENT(1);
    name = ARGUMENT(0);

    if (!SYMBOL_P(name) ||
	(atom = name->data.atom)->a_defstruct == 0 ||
	(offset = atom->property->structure.function) < 0)
	LispDestroy(mac, "%s: invalid argument %s",
		    STRFUN(builtin), STROBJ(name));
    definition = atom->property->structure.definition;

    /* check if the object is of the required type */
    if (struc->type != LispStruct_t || struc->data.struc.def != definition)
	LispDestroy(mac, "%s: %s is not a %s",
		    STRPTR(name), STROBJ(struc), STRPTR(CAR(definition)));

    for (list = struc->data.struc.fields; offset; list = CDR(list), offset--)
	;

    return (CAR(list));
}

LispObj *
Lisp_XeditStructStore(LispMac *mac, LispBuiltin *builtin)
/*
 xedit::struct-store atom struct value
 */
{
    int offset = 0;
    LispAtom *atom = NULL;

    LispObj *definition, *value, *struc, *name, *list;

    value = ARGUMENT(2);
    struc = ARGUMENT(1);
    name = ARGUMENT(0);

    if (!SYMBOL_P(name) ||
	(atom = name->data.atom)->a_defstruct == 0 ||
	(offset = atom->property->structure.function) < 0)
	LispDestroy(mac, "%s: invalid argument %s",
		    STRFUN(builtin), STROBJ(name));
    definition = atom->property->structure.definition;

    /* check if the object is of the required type */
    if (struc->type != LispStruct_t || struc->data.struc.def != definition)
	LispDestroy(mac, "%s: %s is not a %s",
		    STRPTR(name), STROBJ(struc), STRPTR(CAR(definition)));

    for (list = struc->data.struc.fields; offset; list = CDR(list), offset--)
	;

    return (CAR(list) = value);
}

LispObj *
Lisp_XeditStructType(LispMac *mac, LispBuiltin *builtin)
/*
 xedit::struct-type atom struct
 */
{
    LispAtom *atom = NULL;

    LispObj *definition, *struc, *name;

    struc = ARGUMENT(1);
    name = ARGUMENT(0);

    if (!SYMBOL_P(name) ||
	(atom = name->data.atom)->a_defstruct == 0 ||
	(atom->property->structure.function != STRUCT_CHECK))
	LispDestroy(mac, "%s: invalid argument %s",
		    STRFUN(builtin), STROBJ(name));
    definition = atom->property->structure.definition;

    /* check if the object is of the required type */
    if (struc->type == LispStruct_t && struc->data.struc.def == definition)
	return (T);

    return (NIL);
}

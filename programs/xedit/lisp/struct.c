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

/* $XFree86: xc/programs/xedit/lisp/struct.c,v 1.2 2001/09/21 05:08:43 paulo Exp $ */

#include "struct.h"

/*
 * Initialization
 */
LispBuiltin LispMakeStructDef = {
    "MAKE-STRUCT",
    Lisp_MakeStruct,
    1,
    0,
    0
};

LispBuiltin LispStructAccessDef = {
    "STRUCT-FIELD",
    Lisp_StructAccess,
    0,
    1,
    1
};

/*
 * Implementation
 */
LispObj *
Lisp_MakeStruct(LispMac *mac, LispObj *list, char *fname)
{
    int count;
    LispObj *obj, *str, *fld, *nam, *val;

    count = 0;
    str = mac->struc;
    for (obj = CDR(str); obj != NIL; obj = CDR(obj))
	++count;
    fld = NIL;

    /* create structure fields, using default initial values */
    for (obj = CDR(str); obj != NIL; obj = CDR(obj)) {
	if (CAR(obj)->type == LispAtom_t)
	    val = NIL;
	else
	    val = EVAL(CAR(CDR(CAR(obj))));

	if (fld == NIL) {
	    fld = CONS(val, NIL);
	    FRM = CONS(fld, FRM);	/* GC protect fld linking to FRM */
	}
	else {
	    CDR(fld) = CONS(CAR(fld), CDR(fld));
	    CAR(fld) = val;
	}
    }
    fld = CAR(FRM) = LispReverse(fld);

    for (; list != NIL; list = CDR(list)) {
	if ((nam = EVAL(CAR(list)))->type != LispAtom_t ||
	    STRPTR(nam)[0] != ':')
	    LispDestroy(mac, "%s is a illegal field for %s",
			LispStrObj(mac, nam), fname);

	/* check if field name is a valid field name */
	for (count = 0, obj = CDR(str); obj != NIL; ++count, obj = CDR(obj)) {
	    if ((CAR(obj)->type == LispAtom_t &&
		 strcmp(STRPTR(CAR(obj)), STRPTR(nam) + 1) == 0) ||
		(CAR(obj)->type == LispCons_t &&
		 strcmp(STRPTR(CAR(CAR(obj))), STRPTR(nam) + 1) == 0))
		break;
	}

	/* check if structure has named field */
	if (obj == NIL)
	    LispDestroy(mac, ":%s is not a %s field, at %s",
			STRPTR(nam), STRPTR(CAR(str)), fname);

	/* value supplied? */
	if ((list = CDR(list)) == NIL)
	    LispDestroy(mac, "expecting value for field, at %s", fname);

	/* set structure field value */
	for (obj = fld; count; obj = CDR(obj))
	    --count;
	if (obj->prot == LispNil_t) {
	    CAR(obj) = CAR(list);
	    /* set value only if the first time */
	    obj->prot = LispTrue_t;
	}
    }

    /* clean protect flag */
    for (obj = fld; obj != NIL; obj = CDR(obj))
	obj->prot = LispNil_t;

    FRM = CDR(FRM);	/* GC Uprotect fld */

    return (STRUCT(fld, str));
}

LispObj *
Lisp_StructAccess(LispMac *mac, LispObj *list, char *fname)
{
    int len = mac->struc_field;
    LispObj *str = mac->struc, *obj = CAR(list), *res;

    len = mac->struc_field;
    obj = EVAL(obj);
    /* check if the object is of the required type */
    if (obj->type != LispStruct_t || obj->data.struc.def != str)
	LispDestroy(mac, "%s is not a %s",
		    LispStrObj(mac, obj), STRPTR(CAR(str)));

    for (res = CAR(obj); len; res = CDR(res))
	--len;

    mac->setf = res;
    mac->setflag = SETFCAR;

    return (CAR(res));
}

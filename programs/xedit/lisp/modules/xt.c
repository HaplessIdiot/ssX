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

/* $XFree86: xc/programs/xedit/lisp/modules/xt.c,v 1.1 2001/08/31 15:00:14 paulo Exp $ */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <stdio.h>
#include "internal.h"

/*
 * Types
 */
typedef struct {
    XrmQuark qname;
    XrmQuark qtype;
    Cardinal size;
} ResourceInfo;

typedef struct {
    WidgetClass widget_class;
    ResourceInfo **resources;
    Cardinal num_resources;
    Cardinal num_cons_resources;
} ResourceList;

typedef struct {
    Arg *args;
    Cardinal num_args;
} Resources;

typedef struct {
    LispMac *mac;
    char *callback;
    LispObj *argument;		/* XXX must be gc protected outside here */
} CallbackArgs;

/*
 * Prototypes
 */
int xtLoadModule(LispMac*);

void _LispXtCallback(Widget, XtPointer, XtPointer);
LispObj *Lisp_XtAddCallback(LispMac*, LispObj*, char*);
LispObj *Lisp_XtAppInitialize(LispMac*, LispObj*, char*);
LispObj *Lisp_XtAppMainLoop(LispMac*, LispObj*, char*);
LispObj *Lisp_XtCreateWidget(LispMac*, LispObj*, char*);
LispObj *Lisp_XtCreateManagedWidget(LispMac*, LispObj*, char*);
LispObj *_LispXtCreateWidget(LispMac*, LispObj*, char*, int);
LispObj *Lisp_XtGetValues(LispMac*, LispObj*, char*);
LispObj *Lisp_XtRealizeWidget(LispMac*, LispObj*, char*);
LispObj *Lisp_XtSetValues(LispMac*, LispObj*, char*);

static Resources *LispConvertResources(LispMac*, LispObj*, Widget,
				       ResourceList*, ResourceList*);
static void LispFreeResources(Resources*);

static int bcmp_action_resource(_Xconst void*, _Xconst void*);
static ResourceInfo *GetResourceInfo(char*, ResourceList*, ResourceList*);
static ResourceList *GetResourceList(WidgetClass);
static int bcmp_action_resource_list(_Xconst void*, _Xconst void*);
static ResourceList *FindResourceList(WidgetClass);
static int qcmp_action_resource_list(_Xconst void*, _Xconst void*);
static ResourceList *CreateResourceList(WidgetClass);
static int qcmp_action_resource(_Xconst void*, _Xconst void*);
static void BindResourceList(ResourceList*);

/*
 * Initialization
 */
#include "xttable.c"

LispModuleData xtLispModuleData = {
    xtFindFun,
    xtLoadModule
};

static ResourceList **resource_list;
static Cardinal num_resource_list;

static int xtAppContext_t, xtWidget_t, xtWidgetClass_t;

/*
 * Implementation
 */
int
xtLoadModule(LispMac *mac)
{
    char *fname = "INTERNAL:XT-LOAD-MODULE";

    xtAppContext_t = LispRegisterOpaqueType(mac, "XtAppContext");
    xtWidget_t = LispRegisterOpaqueType(mac, "Widget");
    xtWidgetClass_t = LispRegisterOpaqueType(mac, "WidgetClass");
    GCPRO();
    (void)LispSetVariable(mac, ATOM(LispGetString(mac, "coreWidgetClass")),
			  OPAQUE(coreWidgetClass, xtWidgetClass_t),
			  fname, 0);
    GCUPRO();

    return (1);
}

void
_LispXtCallback(Widget w, XtPointer user_data, XtPointer call_data)
{
    CallbackArgs *args = (CallbackArgs*)user_data;
    LispMac *mac = args->mac;
    LispObj *code;

    GCPRO();
    code = CONS(QUOTE(ATOM(args->callback)), CONS(OPAQUE(w, xtWidget_t),
		CONS(args->argument, CONS(OPAQUE(call_data, 0), NIL))));

    (void)Lisp_Funcall(mac, code, "FUNCALL");
    GCUPRO();
}

LispObj *
Lisp_XtAddCallback(LispMac *mac, LispObj *list, char *fname)
{
    CallbackArgs *arguments;
    LispObj *widget, *name, *callback, *args;

    widget = CAR(list);
    if (!CHECKO(widget, xtWidget_t))
	LispDestroy(mac,
		    "cannot convert %s to Widget, at %s",
		    LispStrObj(mac, CAR(list)), fname);
    list = CDR(list);

    name = CAR(list);
    if (name->type != LispString_t)
	LispDestroy(mac, "expecting string, at %s", fname);
    list = CDR(list);

    callback = CAR(list);
    if (callback->type != LispAtom_t)
	LispDestroy(mac, "expecting atom, at %s", fname);
    list = CDR(list);

    if (list == NIL)
	args = list;
    else {
	args = CAR(list);
	PROTECT(args);
    }

    arguments = XtNew(CallbackArgs);
    arguments->mac = mac;
    arguments->callback = LispGetString(mac, callback->data.atom);
    arguments->argument = args;

    XtAddCallback((Widget)(widget->data.opaque.data), name->data.atom,
		  _LispXtCallback, (XtPointer)arguments);

    return (NIL);
}

LispObj *
Lisp_XtAppInitialize(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *obj;
    XtAppContext appcon;
    char *app, *cname;
    Widget shell;
    int zero = 0;
    Resources *resources = NULL;

    if (CAR(list)->type != LispAtom_t)
	LispDestroy(mac, "expecting atom, at %s", fname);
    app = CAR(list)->data.atom;
    list = CDR(list);

    if (CAR(list)->type != LispString_t) {
	LispDestroy(mac, "cannot convert %s to string, at %s",
		    LispStrObj(mac, CAR(list)), fname);
    }
    cname = CAR(list)->data.atom;

    shell = XtAppInitialize(&appcon, cname, NULL, 0, &zero, NULL, NULL, NULL, 0);
    (void)LispSetVariable(mac, ATOM(app), OPAQUE(appcon, xtAppContext_t),
			  fname, 0);

    list = CDR(list);
    if (list == NIL || CAR(list) == NIL)
	resources = NULL;
    else if (CAR(list)->type != LispCons_t)
	LispDestroy(mac, "expecting argument list, at %s", fname);
    else {
	resources = LispConvertResources(mac, CAR(list), shell,
					 GetResourceList(XtClass(shell)),
					 NULL);
	if (resources) {
	    XtSetValues(shell, resources->args, resources->num_args);
	    LispFreeResources(resources);
	}
    }

    return (OPAQUE(shell, xtWidget_t));
}

LispObj *
Lisp_XtAppMainLoop(LispMac *mac, LispObj *list, char *fname)
{
    if (!CHECKO(CAR(list), xtAppContext_t))
	LispDestroy(mac,
		    "cannot convert %s to XtAppContext, at %s",
		    LispStrObj(mac, CAR(list)), fname);

    XtAppMainLoop((XtAppContext)(CAR(list)->data.opaque.data));

    return (NIL);
}

LispObj *
Lisp_XtRealizeWidget(LispMac *mac, LispObj *list, char *fname)
{
    if (!CHECKO(CAR(list), xtWidget_t))
	LispDestroy(mac,
		    "cannot convert %s to Widget, at %s",
		    LispStrObj(mac, CAR(list)), fname);

    XtRealizeWidget((Widget)(CAR(list)->data.opaque.data));

    return (NIL);
}

LispObj *
Lisp_XtCreateWidget(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispXtCreateWidget(mac, list, fname, 0));
}

LispObj *
Lisp_XtCreateManagedWidget(LispMac *mac, LispObj *list, char *fname)
{
    return (_LispXtCreateWidget(mac, list, fname, 1));
}


LispObj *
_LispXtCreateWidget(LispMac *mac, LispObj *list, char *fname, int managed)
{
    char *name;
    WidgetClass widget_class;
    Widget widget, parent;
    LispObj *arg, *val;
    Resources *resources = NULL;

    if (CAR(list)->type != LispString_t)
	LispDestroy(mac, "cannot convert %s to char*, at %s",
		    LispStrObj(mac, CAR(list)), fname);
    name = CAR(list)->data.atom;
    list = CDR(list);

    if (!CHECKO(CAR(list), xtWidgetClass_t))
	LispDestroy(mac, "cannot convert %s to WidgetClass, at %s",
		    LispStrObj(mac, CAR(list)), fname);
    widget_class = (WidgetClass)(CAR(list)->data.opaque.data);
    list = CDR(list);

    if (!CHECKO(CAR(list), xtWidget_t))
	LispDestroy(mac, "cannot convert %s to Widget, at %s",
		    LispStrObj(mac, CAR(list)), fname);
    parent = (Widget)(CAR(list)->data.opaque.data);
    list = CDR(list);

    widget = XtCreateWidget(name, widget_class, parent, NULL, 0);

    if (list == NIL || CAR(list) == NIL)
	resources = NULL;
    else if (CAR(list)->type != LispCons_t)
	LispDestroy(mac, "expecting argument list, at %s", fname);
    else {
	resources = LispConvertResources(mac, CAR(list), widget,
					 GetResourceList(widget_class),
					 GetResourceList(XtClass(parent)));
	XtSetValues(widget, resources->args, resources->num_args);
    }
    if (managed)
	XtManageChild(widget);
    if (resources)
	LispFreeResources(resources);

    return (OPAQUE(widget, xtWidget_t));
}

LispObj *
Lisp_XtGetValues(LispMac *mac, LispObj *list, char *fname)
{
    Arg args[1];
    Widget widget;
    ResourceList *rlist, *plist;
    ResourceInfo *resource;
    LispObj *obj, *res, *ptr;
    char c1;
    short c2;
    int c4;
#ifdef LONG64
    long c8;
#endif

    if (!CHECKO(CAR(list), xtWidget_t))
	LispDestroy(mac, "cannot convert %s to Widget, at %s",
		    LispStrObj(mac, CAR(list)), fname);
    widget = (Widget)(CAR(list)->data.opaque.data);
    list = CDR(list);
    if (CAR(list)->type != LispCons_t)
	LispDestroy(mac, "expecting string list, at %s", fname);

    rlist = GetResourceList(XtClass(widget));
    plist =  XtParent(widget) ? GetResourceList(XtClass(XtParent(widget))) : NULL;

    res = NIL;
    for (list = CAR(list); list != NIL; list = CDR(list)) {
	if (CAR(list)->type != LispString_t)
	    LispDestroy(mac, "%s is not a string, at %s",
			LispStrObj(mac, CAR(list)), fname);
	if ((resource = GetResourceInfo(CAR(list)->data.atom, rlist, plist))
	     == NULL) {
	    int i;
	    Widget child;
	    XrmQuark qwidget;

	    qwidget = XrmPermStringToQuark(XtRWidget);
	    for (i = 0; i < rlist->num_resources; i++) {
		if (rlist->resources[i]->qtype == qwidget) {
		    XtSetArg(args[0],
			     XrmQuarkToString(rlist->resources[i]->qname),
			     &child);
		    XtGetValues(widget, args, 1);
		    if (child && XtParent(child) == widget) {
			resource =
			    GetResourceInfo(CAR(list)->data.atom,
					    GetResourceList(XtClass(child)),
					    NULL);
			if (resource)
			    break;
		    }
		}
	    }
	    if (resource == NULL) {
		fprintf(stderr, "resource %s not available.\n",
			CAR(list)->data.atom);
		continue;
	    }
	}
	switch (resource->size) {
	    case 1:
		XtSetArg(args[0], CAR(list)->data.atom, &c1);
		break;
	    case 2:
		XtSetArg(args[0], CAR(list)->data.atom, &c2);
		break;
	    case 4:
		XtSetArg(args[0], CAR(list)->data.atom, &c4);
		break;
#ifdef LONG64
	    case 1:
		XtSetArg(args[0], CAR(list)->data.atom, &c8);
		break;
#endif
	}
	XtGetValues(widget, args, 1);
	switch (resource->size) {
	    case 1:
		obj = CONS(CAR(list), OPAQUE(c1, 0));
		break;
	    case 2:
		obj = CONS(CAR(list), OPAQUE(c2, 0));
		break;
	    case 4:
		obj = CONS(CAR(list), OPAQUE(c4, 0));
		break;
#ifdef LONG64
	    case 8:
		obj = CONS(CAR(list), OPAQUE(c8, 0));
		break;
#endif
	}
	if (res == NIL)
	    res = ptr = CONS(obj, NIL);
	else {
	    CDR(ptr) = CONS(obj, NIL);
	    ptr = CDR(ptr);
	}
    }

    return (res);
}

LispObj *
Lisp_XtSetValues(LispMac *mac, LispObj *list, char *fname)
{
    Widget widget;
    Resources *resources;

    if (!CHECKO(CAR(list), xtWidget_t))
	LispDestroy(mac, "cannot convert %s to Widget, at %s",
		    LispStrObj(mac, CAR(list)), fname);
    widget = (Widget)(CAR(list)->data.opaque.data);
    list = CDR(list);
    if (CAR(list)->type != LispCons_t)
	LispDestroy(mac, "expecting string list, at %s", fname);

    resources = LispConvertResources(mac, CAR(list), widget,
				     GetResourceList(XtClass(widget)),
				     XtParent(widget) ?
					GetResourceList(XtClass(XtParent(widget))) :
					NULL);
    XtSetValues(widget, resources->args, resources->num_args);
    LispFreeResources(resources);

    return (NIL);
}

static Resources *
LispConvertResources(LispMac *mac, LispObj *list, Widget widget,
		     ResourceList *rlist, ResourceList *plist)
{
    char c1;
    short c2;
    int c4;   
#ifdef LONG64
    long c8;
#endif
    XrmValue from, to;
    LispObj *arg, *val;
    ResourceInfo *resource;
    char *fname = "XT-INTERNAL:CONVERT-RESOURCES";
    Resources *resources = (Resources*)XtCalloc(1, sizeof(Resources));
    static ResourceInfo info;

    for (; list != NIL; list = CDR(list)) {
	if (list->type != LispCons_t || CAR(list)->type != LispCons_t) {
	    XtFree((XtPointer)resources);
	    LispDestroy(mac, "expecting cons, at %s", fname);
	}
	arg = CAR(CAR(list));
	val = CDR(CAR(list));

	if (arg->type != LispString_t) {
	    XtFree((XtPointer)resources);
	    LispDestroy(mac, "resource name must be a string, at %s", fname);
	}

	if ((resource = GetResourceInfo(arg->data.atom, rlist, plist)) == NULL) {
	    int i;
	    Arg args[1];
	    Widget child;
	    XrmQuark qwidget;

	    qwidget = XrmPermStringToQuark(XtRWidget);
	    for (i = 0; i < rlist->num_resources; i++) {
		if (rlist->resources[i]->qtype == qwidget) {
		    XtSetArg(args[0],
			     XrmQuarkToString(rlist->resources[i]->qname),
			     &child);
		    XtGetValues(widget, args, 1);
		    if (child && XtParent(child) == widget) {
			resource =
			    GetResourceInfo(arg->data.atom,
					    GetResourceList(XtClass(child)),
					    NULL);
			if (resource)
			    break;
		    }
		}
	    }
	    if (resource == NULL) {
		fprintf(stderr, "resource %s not available.\n", arg->data.atom);
		continue;
	    }
	}

	if (val->type == LispReal_t || val->type == LispOpaque_t) {
	    resources->args = (Arg*)
		XtRealloc((XtPointer)resources->args,
			  sizeof(Arg) * (resources->num_args + 1));
	    if (val->type == LispReal_t)
		XtSetArg(resources->args[resources->num_args],
			 XrmQuarkToString(resource->qname), (int)val->data.real);
	    else
		XtSetArg(resources->args[resources->num_args],
			 XrmQuarkToString(resource->qname), val->data.opaque.data);
	    ++resources->num_args;
	    continue;
	}
	else if (val->type != LispString_t) {
	    XtFree((XtPointer)resources);
	    LispDestroy(mac,
			"resource value must be string, number or opaque, not %s at %s",
			LispStrObj(mac, val), fname);
	}

	from.size = val == NIL ? 1 : strlen(val->data.atom) + 1;
	from.addr = val == NIL ? "" : val->data.atom;
	switch (to.size = resource->size) {
	    case 1:
		to.addr = (XtPointer)&c1;
		break;
	    case 2:
		to.addr = (XtPointer)&c2;
		break;
	    case 4:
		to.addr = (XtPointer)&c4;
		break;
#ifdef LONG64
	    case 8:
		to.addr = (XtPointer)&c8;
		break;
#endif
	    default:
		fprintf(stderr, "bad resource size %d, for %s.\n",
			to.size, arg->data.atom);
		continue;
	}

	if (strcmp(XtRString, XrmQuarkToString(resource->qtype)) == 0)
#ifdef LONG64
	    c8 = (long)from.addr;
#else
	    c4 = (long)from.addr;
#endif
	else if (!XtConvertAndStore(widget, XtRString, &from,
				    XrmQuarkToString(resource->qtype), &to))
	    /* The type converter already have printed an error message */
	    continue;

	resources->args = (Arg*)
	    XtRealloc((XtPointer)resources->args,
		      sizeof(Arg) * (resources->num_args + 1));
	switch (to.size) {
	    case 1:
		XtSetArg(resources->args[resources->num_args],
			 XrmQuarkToString(resource->qname), c1);
		break;
	    case 2:
		XtSetArg(resources->args[resources->num_args],
			 XrmQuarkToString(resource->qname), c2);
		break;
	    case 4:
		XtSetArg(resources->args[resources->num_args],
			 XrmQuarkToString(resource->qname), c4);
		break;
#ifdef LONG64
	    case 8:
		XtSetArg(resources->args[resources->num_args],
			 XrmQuarkToString(resource->qname), c8);
		break;
#endif
	}
	++resources->num_args;
    }

    return (resources);
}

static void
LispFreeResources(Resources *resources)
{
    if (resources) {
	XtFree((XtPointer)resources->args);
	XtFree((XtPointer)resources);
    }
}

static int
bcmp_action_resource(_Xconst void *string, _Xconst void *resource)
{
    return (strcmp((String)string,
		   XrmQuarkToString((*(ResourceInfo**)resource)->qname)));
}   

static ResourceInfo *
GetResourceInfo(char *name, ResourceList *rlist, ResourceList *plist)
{
    ResourceInfo **resource = NULL;

    if (rlist->resources)
	resource = (ResourceInfo**)
	    bsearch(name, rlist->resources, rlist->num_resources,
		    sizeof(ResourceInfo*), bcmp_action_resource);

    if (resource == NULL && plist) {
	resource = (ResourceInfo**)
	  bsearch(name, &plist->resources[plist->num_resources],
		  plist->num_cons_resources, sizeof(ResourceInfo*),
		  bcmp_action_resource);
    }

    return (resource ? *resource : NULL);
}

static ResourceList *
GetResourceList(WidgetClass wc)
{
    ResourceList *list;

    if ((list = FindResourceList(wc)) == NULL)
	list = CreateResourceList(wc);

    return (list);
}

static int
bcmp_action_resource_list(_Xconst void *wc, _Xconst void *list)
{
    return ((char*)wc - (char*)((*(ResourceList**)list)->widget_class));
}

static ResourceList *
FindResourceList(WidgetClass wc)
{  
    ResourceList **list;

    if (!resource_list)
	return (NULL);

    list = (ResourceList**)
	bsearch(wc, resource_list, num_resource_list,
		sizeof(ResourceList*),  bcmp_action_resource_list);

    return (list ? *list : NULL);
}

static int
qcmp_action_resource_list(_Xconst void *left, _Xconst void *right)
{
    return ((char*)((*(ResourceList**)left)->widget_class) -
	    (char*)((*(ResourceList**)right)->widget_class));
}

static ResourceList *
CreateResourceList(WidgetClass wc)
{
    ResourceList *list;

    list = (ResourceList*)XtMalloc(sizeof(ResourceList));
    list->widget_class = wc;
    list->num_resources = list->num_cons_resources = 0;
    list->resources = NULL;

    resource_list = (ResourceList**)
	XtRealloc((XtPointer)resource_list, sizeof(ResourceList*) *
		  (num_resource_list + 1));
    resource_list[num_resource_list++] = list;
    BindResourceList(list);

    return (list);
}

static int
qcmp_action_resource(_Xconst void *left, _Xconst void *right)
{
    return (strcmp(XrmQuarkToString((*(ResourceInfo**)left)->qname),
		   XrmQuarkToString((*(ResourceInfo**)right)->qname)));
}

static void
BindResourceList(ResourceList *list)
{
    XtResourceList xt_list, cons_list;
    Cardinal i, num_xt, num_cons;

    XtGetResourceList(list->widget_class, &xt_list, &num_xt);
    XtGetConstraintResourceList(list->widget_class, &cons_list, &num_cons);
    list->num_resources = num_xt;
    list->num_cons_resources = num_cons;

    list->resources = (ResourceInfo**)
	XtMalloc(sizeof(ResourceInfo*) * (num_xt + num_cons));

    for (i = 0; i < num_xt; i++) {
	list->resources[i] = (ResourceInfo*)XtMalloc(sizeof(ResourceInfo));
	list->resources[i]->qname =
	    XrmPermStringToQuark(xt_list[i].resource_name);
	list->resources[i]->qtype =
	    XrmPermStringToQuark(xt_list[i].resource_type);
	list->resources[i]->size = xt_list[i].resource_size;
    }

    for (; i < num_xt + num_cons; i++) {
	list->resources[i] = (ResourceInfo*)XtMalloc(sizeof(ResourceInfo));
	list->resources[i]->qname =
	    XrmPermStringToQuark(cons_list[i - num_xt].resource_name);
	list->resources[i]->qtype =
	    XrmPermStringToQuark(cons_list[i - num_xt].resource_type);
	list->resources[i]->size = cons_list[i - num_xt].resource_size;
    }

    XtFree((XtPointer)xt_list);
    if (cons_list)
	XtFree((XtPointer)cons_list);

    qsort(list->resources, list->num_resources, sizeof(ResourceInfo*),
	  qcmp_action_resource);
    if (num_cons)
	qsort(&list->resources[num_xt], list->num_cons_resources,
	      sizeof(ResourceInfo*), qcmp_action_resource);
}

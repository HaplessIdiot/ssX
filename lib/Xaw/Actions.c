/*
 * Copyright (c) 1998 by The XFree86 Project, Inc.
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
 */

/* $XFree86: xc/lib/Xaw/Actions.c,v 3.3 1998/06/04 16:43:07 hohndel Exp $ */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Constraint.h>
#include "Private.h"

#ifdef __EMX__
#define strcasecmp stricmp
#endif

/*
 * Definitions
 */
#define ERROR   -2
#define END     -1
#define BOOLEAN 0
#define AND     '&'
#define OR      '|'
#define XOR     '^'
#define NOT     '~'
#define LP      '('
#define RP      ')'

/*
 * Types
 */
/* boolean expressions */
typedef struct _XawEvalInfo {
  Widget widget;
  XawActionResList *rlist;
  XawActionVarList *vlist;
  XawParseBooleanProc parse_proc;
  XEvent *event;
  char *cp, *lp;
  int token;
  Boolean value;
} XawEvalInfo;

/* resources */
typedef struct _XawActionRes {
  XrmQuark qname;
  XrmQuark qtype;
  Cardinal size;
} XawActionRes;

struct _XawActionResList {
  WidgetClass widget_class;
  XawActionRes **resources;
  Cardinal num_common_resources;
  Cardinal num_constraint_resources;
};

/* variables */
typedef struct _XawActionVar {
  XrmQuark qname;
  String value;
} XawActionVar;

struct _XawActionVarList {
  Widget widget;
  Cardinal num_variables;
  XawActionVar **variables;
};

/*
 * Private methods
 */
/* expressions */
static int get_token(XawEvalInfo*);
static Boolean expr(XawEvalInfo*);
static Boolean prim(XawEvalInfo*);

/* resources */
static String XawConvertActionRes(XawActionResList*, Widget w, String);

static XawActionResList *_XawCreateActionResList(WidgetClass);
static XawActionResList *_XawFindActionResList(WidgetClass);
static void _XawBindActionResList(XawActionResList*);
static XawActionRes *_XawFindActionRes(XawActionResList*, Widget, String);
static int qcmp_action_resource_list(_Xconst void*, _Xconst void*);
static int bcmp_action_resource_list(_Xconst void*, _Xconst void*);
static int qcmp_action_resource(_Xconst void*, _Xconst void*);
static int bcmp_action_resource(_Xconst void*, _Xconst void*);

/* variables */
static String XawConvertActionVar(XawActionVarList*, String);
static void XawDeclareActionVar(XawActionVarList*, String, String);

static XawActionVarList *_XawCreateActionVarList(Widget);
static XawActionVarList *_XawFindActionVarList(Widget);
static XawActionVar *_XawCreateActionVar(XawActionVarList*, String);
static XawActionVar *_XawFindActionVar(XawActionVarList*, String);
static void _XawDestroyActionVarList(Widget, XtPointer, XtPointer);

/*
 * Initialization
 */
/* resources */
static XawActionResList **resource_list;
static Cardinal num_resource_list;

/* variables */
static XawActionVarList **variable_list;
static Cardinal num_variable_list;

/*
 * Implementation
 */
/*
 * Start of Boolean Expression Evaluation Implementation Code
 */
Boolean
XawParseBoolean(Widget w, String param, XEvent *event, Boolean *succed)
{
  char *tmp = param;
  int value;

  value = (int)strtod(param, &tmp);
  if (*tmp == '\0')
    return (value);

  if (strcasecmp(param, "true") == 0
      || strcasecmp(param, "yes") == 0
      || strcasecmp(param, "on") == 0
      || strcasecmp(param, "in") == 0
      || strcasecmp(param, "up") == 0)
    return (True);
  else if (strcasecmp(param, "false") == 0
	  || strcasecmp(param, "no") == 0
	  || strcasecmp(param, "off") == 0
	  || strcasecmp(param, "out") == 0
	  || strcasecmp(param, "down") == 0)
      ;
  else if (strcasecmp(param, "my") == 0
	   || strcasecmp(param, "mine") == 0)
    return (event->xany.window == XtWindow(w));
  else if (strcasecmp(param, "faked") == 0)
    return (event->xany.send_event != 0);
  else
    *succed = False;

  return (False);
}

Boolean
XawBooleanExpression(Widget w, String param, XEvent *event)
{
  XawEvalInfo info;
  Boolean retval;

  info.widget = w;

  info.rlist = XawGetActionResList(XtClass(w));
  info.vlist = XawGetActionVarList(w);

  /*
   * Verify widget class, in case we will allow the parse proc procedure
   * as a widget class element, or if we allow overriding the default
   * parse boolean proc.
   */
  info.parse_proc = XawParseBoolean;

  info.event = event;
  info.cp = info.lp = param;

#ifdef DIAGNOSTIC
  fprintf(stderr, "(*) Parsing expression \"%s\"\n", param);
#endif

  (void)get_token(&info);
  if (info.token == ERROR)
    return (False);
  retval = expr(&info);

  return (info.token != ERROR ? retval : False);
}

int
get_token(XawEvalInfo *info)
{
  int ch;
  char *p, name[256];

  info->lp = info->cp;

  /* eat white spaces */
  while (1)
    {
      ch = *info->cp++;
      if (isspace(ch))
	continue;
      break;
    }

  switch (ch)
    {
    case AND: case OR: case XOR: case NOT: case LP: case RP:
      return (info->token = ch);
    }

  /* It's a symbol name, resolve it. */
  if (ch == XAW_PRIV_VAR_PREFIX || isalnum(ch) || ch == '_')
    {
      Boolean succed = True;

      p = info->cp - 1;

      while ((ch = *info->cp) && (isalnum(ch) || ch == '_'))
	++info->cp;

      strncpy(name, p, XawMin(sizeof(name) - 1, info->cp - p));
      name[XawMin(sizeof(name) -1, info->cp - p)] = '\0';

      if (name[0] == XAW_PRIV_VAR_PREFIX)
	{
	  String value = XawConvertActionVar(info->vlist, name);

	  info->value = info->parse_proc(info->widget, value, info->event,
					 &succed) & 1;
	}
      else
	{
	  info->value = info->parse_proc(info->widget, name, info->event,
					 &succed) & 1;
	  if (!succed)
	    {
	      String value =
		XawConvertActionRes(info->rlist, info->widget, name);

	      info->value = info->parse_proc(info->widget, value, info->event,
					     &succed) & 1;
	      if (!succed)
		{
		  /* not a numeric value or boolean string */
		  info->value = True;
		  succed = True;
		}
	    }
	}
      if (succed)
	return (info->token = BOOLEAN);
    }
  else if (ch == '\0')
    return (info->token = END);

  {
    char msg[256];

    snprintf(msg, sizeof(msg),
	     "evaluate(): bad token \"%c\" at \"%s\"", ch, info->cp - 1);

    XtAppWarning(XtWidgetToApplicationContext(info->widget), msg);
  }

  return (info->token = ERROR);
}

Boolean
expr(XawEvalInfo *info)
{
  Boolean left = prim(info);

  for (;;)
    switch (info->token)
      {
      case AND:
	(void)get_token(info);
	left &= prim(info);
	break;
      case OR:
	(void)get_token(info);
	left |= prim(info);
	break;
      case XOR:
	(void)get_token(info);
	left ^= prim(info);
	break;
      default:
	return (left);
      }
  /* NOTREACHED */
}

Boolean
prim(XawEvalInfo *info)
{
  Boolean e;

  switch (info->token)
    {
    case BOOLEAN:
      (void)get_token(info);
      return (info->value);
    case NOT:
      (void)get_token(info);
      return (!prim(info));
    case LP:
      (void)get_token(info);
      e = expr(info);
      if (info->token != RP)
	{
	  char msg[256];

	  info->token = ERROR;
	  snprintf(msg, sizeof(msg),
		   "evaluate(): expecting ), at \"%s\"", info->lp);
	  XtAppWarning(XtWidgetToApplicationContext(info->widget), msg);
	  return (False);
	}
      return (e);
    case END:
      return (True);
    default:
      {
	char msg[256];

	info->token = ERROR;
	snprintf(msg, sizeof(msg),
		 "evaluate(): sintax error, at \"%s\"", info->lp);
	XtAppWarning(XtWidgetToApplicationContext(info->widget), msg);
      } return (False);
    }
  /* NOTREACHED */
}

/*
 * Start of Resources Implementation Code
 */
void
XawSetValuesAction(Widget w, XEvent *event,
		   String *params, Cardinal *num_params)
{
  Arg *arglist;
  Cardinal num_args, count;
  XawActionResList *rlist;
  XawActionVarList *vlist;
  XawActionRes *resource;
  XrmValue from, to;
  String value;
  char  c_1;
  short c_2;
  int   c_4;
#ifdef LONG_64
  long  c_8;
#endif

  if (!(*num_params & 1))
    {
      XawPrintActionErrorMsg("set-values", w, params, num_params);
      return;
    }

  if (!XawBooleanExpression(w, params[0], event))
    return;

  rlist = XawGetActionResList(XtClass(w));
  vlist = XawGetActionVarList(w);

  num_args = 0;
  arglist = (Arg *)XtMalloc(sizeof(Arg) * (*num_params)>>1);

  for (count = 1; count < *num_params; count += 2)
    {
      if ((resource = _XawFindActionRes(rlist, w, params[count])) == NULL)
	{
	  char msg[256];

	  snprintf(msg, sizeof(msg), "set-values(): bad resource name \"%s\"",
		   params[count]);
	  XtAppWarning(XtWidgetToApplicationContext(w), msg);
          continue;
	}
      value = XawConvertActionVar(vlist, params[count + 1]);
      from.size = strlen(value) + 1;
      from.addr = value;
      to.size = resource->size;
      switch (to.size)
	{
	case 1: to.addr = (XPointer)&c_1; break;
	case 2: to.addr = (XPointer)&c_2; break;
	case 4: to.addr = (XPointer)&c_4; break;
#ifdef LONG_64
	case 8: to.addr = (XPointer)&c_8; break;
#endif
	default:
	  {
	    char msg[256];

	    snprintf(msg, sizeof(msg),
		     "set-values(): bad resource size for \"%s\"",
		     params[count]);
	    XtAppWarning(XtWidgetToApplicationContext(w), msg);
	  } continue;
	}

      if (strcmp(XtRString, XrmQuarkToString(resource->qtype)) == 0)
#ifdef LONG_64
	c_8 = (int)from.addr;
#else
	c_4 = (int)from.addr;
#endif
      else if (!XtConvertAndStore(w, XtRString, &from,
				  XrmQuarkToString(resource->qtype), &to))
	continue;

      switch (to.size)
	{
	case 1:
	  XtSetArg(arglist[num_args], XrmQuarkToString(resource->qname), c_1);
	  break;
	case 2:
	  XtSetArg(arglist[num_args], XrmQuarkToString(resource->qname), c_2);
	  break;
	case 4:
	  XtSetArg(arglist[num_args], XrmQuarkToString(resource->qname), c_4);
	  break;
#ifdef LONG_64
	case 8:
	  XtSetArg(arglist[num_args], XrmQuarkToString(resource->qname), c_8);
	  break;
#endif
	}
      ++num_args;
    }

  XtSetValues(w, arglist, num_args);
  XtFree((char *)arglist);
}

void
XawGetValuesAction(Widget w, XEvent *event,
		   String *params, Cardinal *num_params)
{     
  XawActionResList *rlist;
  XawActionVarList *vlist;
  String value;
  Cardinal count;

  if (!(*num_params & 1)) 
    {
      XawPrintActionErrorMsg("get-values", w, params, num_params);
      return;
    }
  if (!XawBooleanExpression(w, params[0], event))
    return;

  rlist = XawGetActionResList(XtClass(w));
  vlist = XawGetActionVarList(w);

  for (count = 1; count < *num_params; count += 2)
    {
      if ((value = XawConvertActionRes(rlist, w, params[count + 1])) == NULL)
	continue;
      XawDeclareActionVar(vlist, params[count], value);
    }
}

void
XawDeclareAction(Widget w, XEvent *event,
		 String *params, Cardinal *num_params)
{     
  XawActionVarList *vlist;
  Cardinal count;

  if (!(*num_params & 1))
    {
      XawPrintActionErrorMsg("declare", w, params, num_params);
      return;
    }
  if (!XawBooleanExpression(w, params[0], event))
    return;

  vlist = XawGetActionVarList(w);

  for (count = 1; count < *num_params; count += 2)
    XawDeclareActionVar(vlist, params[count], params[count + 1]);
}

void
XawCallProcAction(Widget w, XEvent *event,
		  String *params, Cardinal *num_params)
{
  String *args;
  Cardinal num_args;

  if (*num_params < 2)
    {
      XawPrintActionErrorMsg("call-proc", w, params, num_params);
      return;
    }

  if (*num_params && !XawBooleanExpression(w, params[0], event))
    return;

  if (*num_params > 2)
    {
      args = &params[2];
      num_args = *num_params - 2;
    }
  else
    {
      args = NULL;
      num_args = 0;
    }

  XtCallActionProc(w, params[1], event, args, num_args);
}

String
XawConvertActionRes(XawActionResList *list, Widget w, String name)
{
  XawActionRes *resource;
  XrmValue from, to;
  Arg arg;
  char  c_1;
  short c_2;
  int   c_4;   
#ifdef LONG_64
  long  c_8;
#endif

  if ((resource = _XawFindActionRes(list, w, name)) == NULL)
    {
      char msg[256];

      snprintf(msg, sizeof(msg), "convert(): bad resource name \"\%s\"", name);
      XtAppWarning(XtWidgetToApplicationContext(w), msg);
      return (NULL);
    }

  from.size = resource->size;
  switch (from.size)
    {
    case 1:
      XtSetArg(arg, XrmQuarkToString(resource->qname),
	       from.addr = (XPointer)&c_1);
      break;
    case 2:
      XtSetArg(arg, XrmQuarkToString(resource->qname),
               from.addr = (XPointer)&c_2);
      break;
    case 4:
      XtSetArg(arg, XrmQuarkToString(resource->qname),
               from.addr = (XPointer)&c_4);
      break;
    default:
      {
        char msg[256];

        snprintf(msg, sizeof(msg), "convert(): bad resource size for \"%s\"",
		 name);
	XtAppWarning(XtWidgetToApplicationContext(w), name);
      } return (NULL);
    }

  XtGetValues(w, &arg, 1);
  to.size = sizeof(String);
  to.addr = (XPointer)NULL;

  if (strcmp(XtRString, XrmQuarkToString(resource->qtype)) == 0)
    to.addr = *(char **)from.addr;
  else if (!XtConvertAndStore(w, XrmQuarkToString(resource->qtype),
			      &from, XtRString, &to))
    return (NULL);

  return ((String)to.addr);
}

void
XawPrintActionErrorMsg(String action_name, Widget w,
		       String *params, Cardinal *num_params)
{
  char msg[1024];
  int size, index;

  size = snprintf(msg, sizeof(msg), "%s(): bad number of parameters.\n\t(",
		  action_name, action_name);

  index = 0;
  while (index < *num_params - 1 && size < sizeof(msg))
    size += snprintf(&msg[size], sizeof(msg) - size, "%s, ",
		     params[index++]); 
  if (*num_params)
    snprintf(&msg[size], sizeof(msg) - size, "%s)", params[index]);
  else
    snprintf(&msg[size], sizeof(msg) - size, ")");
  XtAppWarning(XtWidgetToApplicationContext(w), msg);
}

XawActionResList *
XawGetActionResList(WidgetClass wc)
{
  XawActionResList *list;

  list = _XawFindActionResList(wc);

  if (!list)
    list = _XawCreateActionResList(wc);

  return (list);
}

int
qcmp_action_resource_list(register _Xconst void *left,
			  register _Xconst void *right)
{   
  return ((int)((*(XawActionResList **)left)->widget_class) -
          (int)((*(XawActionResList **)right)->widget_class));
}

XawActionResList *
_XawCreateActionResList(WidgetClass wc)
{
  XawActionResList *list;

  list = (XawActionResList *)XtMalloc(sizeof(XawActionResList));
  list->widget_class = wc;
  list->num_common_resources = list->num_constraint_resources = 0;
  list->resources = NULL;

  if (!resource_list)
    {
      num_resource_list = 1;
      resource_list = (XawActionResList **)XtMalloc(sizeof(XawActionResList*));
      resource_list[0] = list;
    }
  else
    {
      ++num_resource_list;
      resource_list = (XawActionResList **)XtRealloc((char *)resource_list,
						     sizeof(XawActionResList*)
						     * num_resource_list);
      resource_list[num_resource_list - 1] = list;
      qsort(resource_list, num_resource_list, sizeof(XawActionResList*),
	    qcmp_action_resource_list);
    }

  _XawBindActionResList(list);

  return (list);
}

int
bcmp_action_resource_list(register _Xconst void *wc,
			  register _Xconst void *list)
{
  return ((int)wc - (int)((*(XawActionResList **)list)->widget_class));
}

XawActionResList *
_XawFindActionResList(WidgetClass wc)
{  
  XawActionResList **list;

  if (!resource_list)
    return (NULL);

  list = (XawActionResList **)bsearch(wc, resource_list,
				      num_resource_list,
				      sizeof(XawActionResList*),
				      bcmp_action_resource_list);

  return (list ? *list : NULL);
}

int
qcmp_action_resource(register _Xconst void *left,
		     register _Xconst void *right)
{
  return (strcmp(XrmQuarkToString((*(XawActionRes **)left)->qname),
		 XrmQuarkToString((*(XawActionRes **)right)->qname)));
}

void
_XawBindActionResList(XawActionResList *list)
{
  XtResourceList xt_list, cons_list;
  Cardinal i, num_xt, num_cons;

#ifdef DIAGNOSTIC
  fprintf(stderr, "(*) Creating resource list for class \'%s\'\n---------\n",
	  list->widget_class->core_class.class_name);
#endif

  XtGetResourceList(list->widget_class, &xt_list, &num_xt);
  XtGetConstraintResourceList(list->widget_class, &cons_list, &num_cons);
  list->num_common_resources = num_xt;
  list->num_constraint_resources = num_cons;

  list->resources = (XawActionRes **)
    XtMalloc(sizeof(XawActionRes*) * (num_xt + num_cons));

#ifdef DIAGNOSTIC
  fprintf(stderr, "Common resources\n---\n");
#endif

  for (i = 0; i < num_xt; i++)
    {
      list->resources[i] = (XawActionRes *)XtMalloc(sizeof(XawActionRes));
      list->resources[i]->qname =
	XrmPermStringToQuark(xt_list[i].resource_name);
      list->resources[i]->qtype =
	XrmPermStringToQuark(xt_list[i].resource_type);
      list->resources[i]->size = xt_list[i].resource_size;

#ifdef DIAGNOSTIC
      fprintf(stderr, "%-20s\t%-20s\t(%d)\n",
	      xt_list[i].resource_name,
	      xt_list[i].resource_type,
	      xt_list[i].resource_size);
#endif
    }

#ifdef DIAGNOSTIC
  fprintf(stderr, "---\nContraint resources\n---");
#endif

  for (; i < num_xt + num_cons; i++)
    {
      list->resources[i] = (XawActionRes *)XtMalloc(sizeof(XawActionRes));
      list->resources[i]->qname =
	XrmPermStringToQuark(cons_list[i - num_xt].resource_name);
      list->resources[i]->qtype =
	XrmPermStringToQuark(cons_list[i - num_xt].resource_type);
      list->resources[i]->size = cons_list[i - num_xt].resource_size;

#ifdef DIAGNOSTIC
      fprintf(stderr, "%-20s\t%-20s\t(%d)\n",
	      cons_list[i - num_xt].resource_name,
	      cons_list[i - num_xt].resource_type,
	      cons_list[i - num_xt].resource_size);
#endif
    }

#ifdef DIAGNOSTIC
  fprintf(stderr, "---\n");
#endif

  XtFree((char *)xt_list);
  if (cons_list)
    XtFree((char *)cons_list);

  qsort(list->resources, list->num_common_resources, sizeof(XawActionRes*),
	qcmp_action_resource);
  if (num_cons)
    qsort(&list->resources[num_xt], list->num_constraint_resources,
	  sizeof(XawActionRes*), qcmp_action_resource);
}

int
bcmp_action_resource(register _Xconst void *string,
		     register _Xconst void *resource)
{
  return (strcmp((String)string,
		 XrmQuarkToString((*(XawActionRes **)resource)->qname)));
}   
    
XawActionRes *
_XawFindActionRes(XawActionResList *list, Widget detail, String name)
{
  XawActionRes **res;

  if (!list->resources)
    return (NULL);

  res = (XawActionRes **)bsearch(name, list->resources,
				 list->num_common_resources,
				 sizeof(XawActionRes*), bcmp_action_resource);

  if (!res && XtParent(detail)
      && XtIsSubclass(XtParent(detail), constraintWidgetClass))
    {
      XawActionResList *cons = XawGetActionResList(XtClass(XtParent(detail)));

      if (cons)
	res = (XawActionRes **)
	  bsearch(name, &cons->resources[cons->num_common_resources],
		  cons->num_constraint_resources,
		  sizeof(XawActionRes*), bcmp_action_resource);
    }

  return (res ? *res : NULL);
}

/*
 * Start of Variables Implementation Code
 */
void
XawDeclareActionVar(XawActionVarList *list, String name, String value)
{
  XawActionVar *variable;

  if (name[0] != XAW_PRIV_VAR_PREFIX)
    {
      char msg[256];

      snprintf(msg, sizeof(msg), "declare(): variable name must begin with "
	       "\'%c\', at %s = %s", XAW_PRIV_VAR_PREFIX, name, value);
      XtAppWarning(XtWidgetToApplicationContext(list->widget), msg);
    }
  variable = _XawFindActionVar(list, name);
  if (!variable)
    variable = _XawCreateActionVar(list, name);
  if (variable->value)
    {
      if (strcmp(variable->value, value) == 0)
	return;
      XtFree(variable->value);
    }
  variable->value = XtNewString(value);
}

String
XawConvertActionVar(XawActionVarList *list, String name)
{
  XawActionVar *variable;

  if (name[0] != XAW_PRIV_VAR_PREFIX)
    return (name);

  variable = _XawFindActionVar(list, name);
  if (!variable)
    return (name);
  return (variable->value);
}

XawActionVarList *
XawGetActionVarList(Widget w)
{
  XawActionVarList *list;

  list = _XawFindActionVarList(w);
  if (!list)
    list = _XawCreateActionVarList(w);

  return (list);
}

static int
qcmp_action_variable_list(register _Xconst void *left,
			  register _Xconst void *right)
{
  return ((int)((*(XawActionVarList **)left)->widget) -
	  (int)((*(XawActionVarList **)right)->widget));
}

XawActionVarList *
_XawCreateActionVarList(Widget w)
{
  XawActionVarList *list;

#ifdef DIAGNOSTIC
  fprintf(stderr, "(*) Creating action variable list for widget %s (%p)\n",
	  XtName(w), w);
#endif

  list = (XawActionVarList *)XtMalloc(sizeof(XawActionVarList));
  list->widget = w;
  list->num_variables = 0;
  list->variables = NULL;

  if (!variable_list)
    {
      num_variable_list = 1;
      variable_list = (XawActionVarList **)
	XtMalloc(sizeof(XawActionVarList *));
      variable_list[0] = list;
    }
  else
    {
      ++num_variable_list;
      variable_list = (XawActionVarList **)
	XtRealloc((char *)variable_list,
		  sizeof(XawActionVarList *) * num_variable_list);
      variable_list[num_variable_list - 1] = list;
      qsort(variable_list, num_variable_list, sizeof(XawActionVarList*),
	    qcmp_action_variable_list);
    }

  XtAddCallback(w, XtNdestroyCallback, _XawDestroyActionVarList,
		(XtPointer)list);

  return (list);
}

static int
bcmp_action_variable_list(register _Xconst void *widget,
			  register _Xconst void *list)
{
  return ((int)widget - (int)((*(XawActionVarList **)list)->widget));
}

XawActionVarList *
_XawFindActionVarList(Widget w)
{
  XawActionVarList **list;

  if (!num_variable_list)
    return ((XawActionVarList *)NULL);

  list = (XawActionVarList **)bsearch(w, variable_list, num_variable_list,
				      sizeof(XawActionVarList*),
				      bcmp_action_variable_list);

  return (list ? *list : NULL);
}

static int
qcmp_action_variable(register _Xconst void *left,
		     register _Xconst void *right)
{
  return (strcmp(XrmQuarkToString((*(XawActionVar **)left)->qname),
		 XrmQuarkToString((*(XawActionVar **)right)->qname)));
}

XawActionVar *
_XawCreateActionVar(XawActionVarList *list, String name)
{
  XawActionVar *variable;

#ifdef DIAGNOSTIC
  fprintf(stderr, "(*) Creating action variable '%s' for widget %s (%p)\n",
	  name, XtName(list->widget), list->widget);
#endif

  variable = (XawActionVar *)XtMalloc(sizeof(XawActionVar));
  variable->qname = XrmStringToQuark(name);
  variable->value = (String)NULL;

  if (!list->variables)
    {
      list->num_variables = 1;
      list->variables = (XawActionVar **)XtMalloc(sizeof(XawActionVar*));
      list->variables[0] = variable;
    }
  else
    {
      ++list->num_variables;
      list->variables = (XawActionVar **)XtRealloc((char *)list->variables,
						   sizeof(XawActionVar *) *
						   list->num_variables);
      list->variables[list->num_variables - 1] = variable;
      qsort(list->variables, list->num_variables, sizeof(XawActionVar*),
	    qcmp_action_variable);
    }
  return (variable);
}

static int
bcmp_action_variable(register _Xconst void *string,
		     register _Xconst void *variable)
{
  return (strcmp((String)string,
		 XrmQuarkToString((*(XawActionVar **)variable)->qname)));
}

XawActionVar *
_XawFindActionVar(XawActionVarList *list, String name)
{
  XawActionVar **var;

  if (!list->variables)
    return (NULL);

  var = (XawActionVar **)bsearch(name, list->variables, list->num_variables,
				 sizeof(XawActionVar*), bcmp_action_variable);

  return (var ? *var : NULL);
}

/* ARGSUSED */
void
_XawDestroyActionVarList(Widget w, XtPointer client_data, XtPointer call_data)
{
  XawActionVarList *list = (XawActionVarList *)client_data;
  Cardinal i;

  for (i = 0; i < num_variable_list; i++)
    if (variable_list[i] == list)
      break;
  if (i >= num_variable_list || list->widget != w
      ||variable_list[i]->widget != w)
    {
      XtWarning("destroy-variable-list(): Bad widget argument.");
      return;
    }
  if (--num_variable_list > 0)
    {
      bcopy(&variable_list[i + 1], &variable_list[i],
	    (num_variable_list - i) * sizeof(XawActionVarList *));
      variable_list = (XawActionVarList **)
	XtRealloc((char *)variable_list, sizeof(XawActionVarList *) *
		  num_variable_list);
    }
  else
    {
      XtFree((char *)variable_list);
      variable_list = NULL;
    }

  for (i = 0; i < list->num_variables; i++)
    XtFree(list->variables[i]->value);

  XtFree((char *)list->variables);
  XtFree((char *)list);
}

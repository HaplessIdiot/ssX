/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Option.c,v 1.3 1998/12/05 14:40:07 dawes Exp $ */

/*
 * Copyright (c) 1998 by The XFree86 Project, Inc.
 *
 * Author: David Dawes <dawes@xfree86.org>
 *
 *
 * This file includes public option handling functions.
 */

#include <stdlib.h>
#include <ctype.h>
#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Optrec.h"

static Bool ParseOptionValue(int scrnIndex, pointer options, OptionInfoPtr p);

/*
 * xf86CollectOptions collects the options from each of the config file
 * sections used by the screen and puts the combined list in pScrn->options.
 * This function requires that the following have been initialised:
 *
 *	pScrn->confScreen
 *	pScrn->device
 *	pScrn->display
 *	pScrn->monitor
 *
 * The extraOpts parameter may optionally contain a list of additional options
 * to include.
 *
 * The order of precedence for options is:
 *
 *   extraOpts, display, confScreen, monitor, device
 */

void
xf86CollectOptions(ScrnInfoPtr pScrn, pointer extraOpts)
{
    XF86OptionPtr tmp;
    XF86OptionPtr extras = (XF86OptionPtr)extraOpts;

    pScrn->options = NULL;
    if (pScrn->device->options) {
	pScrn->options = OptionListDup(pScrn->device->options);
    }
    if (pScrn->monitor->options) {
	tmp = OptionListDup(pScrn->monitor->options);
	if (pScrn->options)
	    OptionListMerge(pScrn->options, tmp);
	else
	    pScrn->options = tmp;
    }
    if (pScrn->confScreen->options) {
	tmp = OptionListDup(pScrn->confScreen->options);
	if (pScrn->options)
	    OptionListMerge(pScrn->options, tmp);
	else
	    pScrn->options = tmp;
    }
    if (pScrn->display->options) {
	tmp = OptionListDup(pScrn->display->options);
	if (pScrn->options)
	    OptionListMerge(pScrn->options, tmp);
	else
	    pScrn->options = tmp;
    }
    if (extras) {
	tmp = OptionListDup(extras);
	if (pScrn->options)
	    OptionListMerge(pScrn->options, tmp);
	else
	    pScrn->options = tmp;
    }
}

/* Created for new XInput stuff -- essentially extensions to the parser	*/

/* These xf86Set* functions are intended for use by non-screen specific code */

int
xf86SetIntOption(pointer optlist, const char *name, int deflt)
{
    OptionInfoRec o;

    o.name = name;
    o.type = OPTV_INTEGER;
    if (ParseOptionValue(-1, optlist, &o))
	deflt = o.value.num;
    return deflt;
}


char *
xf86SetStrOption(pointer optlist, const char *name, char *deflt)
{
    OptionInfoRec o;

    o.name = name;
    o.type = OPTV_STRING;
    if (ParseOptionValue(-1, optlist, &o))
        deflt = o.value.str;
    return xstrdup(deflt);
}


int
xf86SetBoolOption(pointer optlist, const char *name, int deflt)
{
    OptionInfoRec o;

    o.name = name;
    o.type = OPTV_TRI;
    if (ParseOptionValue(-1, optlist, &o))
	deflt = o.value.bool;
    return deflt;
}


pointer
xf86AddNewOption(pointer head, char *name, char *val)
{
    return addNewOption(head, name, val);
}


pointer
xf86NewOption(char *name, char *value)
{
    return NewOption(name, value);
}


pointer
xf86NextOption(pointer list)
{
    return NextOption(list);
}

pointer
xf86OptionListCreate(char **options, int count)
{
	return OptionListCreate(options, count);
}

pointer
xf86OptionListMerge(pointer head, pointer tail)
{
	return OptionListMerge(head, tail);
}

void
xf86OptionListFree(pointer opt)
{
	OptionListFree(opt);
}

char *
xf86OptionName(pointer opt)
{
	return OptionName(opt);
}

char *
xf86OptionValue(pointer opt)
{
	return OptionValue(opt);
}

void
xf86OptionListReport(pointer parm)
{
    XF86OptionPtr opts = parm;

    while(opts) {
	if (OptionValue(opts))
	    xf86ErrorFVerb(5, "\tOption \"%s\" \"%s\"\n",
			    OptionName(opts), OptionValue(opts));
	else
	    xf86ErrorFVerb( 5, "\tOption \"%s\"\n", OptionName(opts));
	opts = NextOption(opts);
    }
}

/* End of XInput-caused section	*/

pointer
xf86FindOption(pointer options, const char *name)
{
    return FindOption(options, name);
}


char *
xf86FindOptionValue(pointer options, const char *name)
{
    return FindOptionValue(options, name);
}


void
xf86MarkOptionUsed(pointer option)
{
    if (option != NULL)
	((XF86OptionPtr)option)->opt_used = TRUE;
}


void
xf86MarkOptionUsedByName(pointer options, const char *name)
{
    XF86OptionPtr opt;

    opt = FindOption(options, name);
    if (opt != NULL)
	opt->opt_used = TRUE;
}


void
xf86ShowUnusedOptions(int scrnIndex, pointer options)
{
    XF86OptionPtr opt = options;

    while (opt) {
	if (opt->opt_name && !opt->opt_used) {
	    xf86DrvMsg(scrnIndex, X_WARNING, "Option \"%s\" is not used\n",
			opt->opt_name);
	}
	opt = opt->list.next;
    }
}


static Bool
ParseOptionValue(int scrnIndex, pointer options, OptionInfoPtr p)
{
    char *s, *end;
    if ((s = FindOptionValue(options, p->name)) != NULL) {
	xf86MarkOptionUsedByName(options, p->name);
	switch (p->type) {
	case OPTV_INTEGER:
	    if (*s == '\0') {
		xf86DrvMsg(scrnIndex, X_WARNING,
			   "Option \"%s\" requires an integer value\n",
			   p->name);
		p->found = FALSE;
	    } else {
		p->value.num = strtoul(s, &end, 0);
		if (*end == '\0') {
		    p->found = TRUE;
		} else {
		    xf86DrvMsg(scrnIndex, X_WARNING,
			       "Option \"%s\" requires an integer value\n",
			        p->name);
		    p->found = FALSE;
		}
	    }
	    break;
	case OPTV_STRING:
	    if (*s == '\0') {
		xf86DrvMsg(scrnIndex, X_WARNING,
			   "Option \"%s\" requires an string value\n",
			   p->name);
		p->found = FALSE;
	    } else {
		p->value.str = s;
		p->found = TRUE;
	    }
	    break;
	case OPTV_ANYSTR:
	    p->value.str = s;
	    p->found = TRUE;
	    break;
	case OPTV_REAL:	
	    if (*s == '\0') {
		xf86DrvMsg(scrnIndex, X_WARNING,
			   "Option \"%s\" requires a floating point value\n",
			   p->name);
		p->found = FALSE;
	    } else {
		p->value.num = strtod(s, &end);
		if (*end == '\0') {
		    p->found = TRUE;
		} else {
		    xf86DrvMsg(scrnIndex, X_WARNING,
			    "Option \"%s\" requires a floating point value\n",
			    p->name);
		    p->found = FALSE;
		}
	    }
	    break;
	case OPTV_BOOLEAN:
	    p->found = TRUE;
	    break;
	case OPTV_TRI:
	    if (*s == '\0') {
		p->found = TRUE;
		p->value.bool = TRUE;
	    } else {
		p->found = TRUE;
		if (xf86NameCmp(s, "1") == 0)
		    p->value.bool = TRUE;
		else if (xf86NameCmp(s, "on") == 0)
		    p->value.bool = TRUE;
		else if (xf86NameCmp(s, "true") == 0)
		    p->value.bool = TRUE;
		else if (xf86NameCmp(s, "yes") == 0)
		    p->value.bool = TRUE;
		else if (xf86NameCmp(s, "0") == 0)
		    p->value.bool = FALSE;
		else if (xf86NameCmp(s, "off") == 0)
		    p->value.bool = FALSE;
		else if (xf86NameCmp(s, "false") == 0)
		    p->value.bool = FALSE;
		else if (xf86NameCmp(s, "no") == 0)
		    p->value.bool = FALSE;
		else {
		    xf86DrvMsg(scrnIndex, X_WARNING,
			       "Option \"%s\" requires a boolean value\n",
			       p->name);
		    p->found = FALSE;
		}
	    }
	    break;
	case OPTV_NONE:
	    /* Should never get here */
	    p->found = FALSE;
	    break;
	}
	if (p->found) {
	    xf86DrvMsgVerb(scrnIndex, X_CONFIG, 2, "Option \"%s\"", p->name);
	    if (p->type != OPTV_BOOLEAN && !(p->type == OPTV_TRI && *s == 0)) {
		xf86ErrorFVerb(2, " \"%s\"", s);
	    }
	    xf86ErrorFVerb(2, "\n");
	}
    } else {
	p->found = FALSE;
    }
    return p->found;
}


void
xf86ProcessOptions(int scrnIndex, pointer options, OptionInfoPtr optinfo)
{
    OptionInfoPtr p;

    for (p = optinfo; p->name != NULL; p++) {
	ParseOptionValue(scrnIndex, options, p);
    }
}


OptionInfoPtr
xf86TokenToOptinfo(OptionInfoPtr table, int token)
{
    OptionInfoPtr p;

    for (p = table; p->token >= 0 && p->token != token; p++)
	;

    if (p->token < 0)
	return NULL;
    else
	return p;
}


Bool
xf86IsOptionSet(OptionInfoPtr table, int token)
{
    OptionInfoPtr p;

    p = xf86TokenToOptinfo(table, token);
    return (p && p->found);
}


char *
xf86GetOptValString(OptionInfoPtr table, int token)
{
    OptionInfoPtr p;

    p = xf86TokenToOptinfo(table, token);
    if (p && p->found)
	return p->value.str;
    else
	return NULL;
}


Bool
xf86GetOptValInteger(OptionInfoPtr table, int token, int *value)
{
    OptionInfoPtr p;

    p = xf86TokenToOptinfo(table, token);
    if (p && p->found) {
	*value = p->value.num;
	return TRUE;
    } else
	return FALSE;
}


Bool
xf86GetOptValULong(OptionInfoPtr table, int token, unsigned long *value)
{
    OptionInfoPtr p;

    p = xf86TokenToOptinfo(table, token);
    if (p && p->found) {
	*value = p->value.num;
	return TRUE;
    } else
	return FALSE;
}


Bool
xf86GetOptValReal(OptionInfoPtr table, int token, double *value)
{
    OptionInfoPtr p;

    p = xf86TokenToOptinfo(table, token);
    if (p && p->found) {
	*value = p->value.realnum;
	return TRUE;
    } else
	return FALSE;
}


Bool
xf86GetOptValBool(OptionInfoPtr table, int token, Bool *value)
{
    OptionInfoPtr p;

    p = xf86TokenToOptinfo(table, token);
    if (p && p->found) {
	*value = p->value.bool;
	return TRUE;
    } else
	return FALSE;
}


int
xf86NameCmp(const char *s1, const char *s2)
{
    return NameCompare(s1, s2);
}

char *
xf86NormalizeName(const char *s)
{
    char *ret, *q;
    const char *p;

    if (s == NULL)
	return NULL;

    ret = xalloc(strlen(s) + 1);
    for (p = s, q = ret; *p != 0; p++) {
	switch (*p) {
	case '_':
	case ' ':
	case '\t':
	    continue;
	default:
	    if (isupper(*p))
		*q++ = tolower(*p);
	    else
		*q++ = *p;
	}
    }
    *q = '\0';
    return ret;
}

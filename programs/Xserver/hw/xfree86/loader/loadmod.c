/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loadmod.c,v 1.30 1998/12/13 05:32:55 dawes Exp $ */

/*
 *
 * Copyright 1995-1998 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "os.h"
/* For stat() and related stuff */
#define NO_OSLIB_PROTOTYPES
#define NO_COMPILER_H
#include "xf86_OSlib.h"
#if defined(SVR4)
#include <sys/stat.h>
#endif
#define LOADERDECLARATIONS
#include "loaderProcs.h"
#include "misc.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Xinput.h"
#include "loader.h"
#include "xf86Optrec.h"

extern int check_unresolved_sema;

#ifdef GLXEXT
extern void (*GlxExtensionInitPtr) (void);
typedef Bool (*GlxInitVisualsType) (
   VisualPtr * /* visualp */ ,
   DepthPtr * /* depthp */ ,
   int * /* nvisualp */ ,
   int * /* ndepthp */ ,
   int * /* rootDepthp */ ,
   VisualID * /* defaultVisp */ ,
   unsigned long /* sizes */ ,
   int	/* bitsPerRGB */
);
GlxInitVisualsType GlxInitVisualsPtr;
#endif

/* Prototypes for static functions */
static char *FindModule (const char *, const char *);
static void CheckVersion (const char *, XF86ModuleVersionInfo *);
static void UnloadModuleOrDriver (ModuleDescPtr mod);

ModuleVersions LoaderVersionInfo = {
	XF86_VERSION_CURRENT,
	ABI_ANSIC_VERSION,
	ABI_VIDEODRV_VERSION,
	ABI_XINPUT_VERSION,
	ABI_EXTENSION_VERSION,
	ABI_FONT_VERSION
};

void
LoaderFixups (void)
{
	/* Need to call LRS here because the frame buffers get loaded last,
	 * and the drivers depend on them. */

	LoaderResolveSymbols ();
}

static char *subdirs[] =
{
	"",
	"drivers/",
	"extensions/",
	"fonts/",
	"internal/",
};
static char *prefixes[] =
{
	"",
	"lib",
};
static char *suffixes[] =
{
	"",
	".so",
	"_drv.so",
	".o",
	"_drv.o",
	".a",
};

static char *
FindModule (module, dir)
const char *module;
const char *dir;
{
	char *buf;
	int d, p, s;
	struct stat stat_buf;

	buf = (char *) xcalloc (1, strlen (module) + strlen (dir) + 40);
	for (d = 0; d < sizeof (subdirs) / sizeof (char *); d++)
		for (p = 0; p < sizeof (prefixes) / sizeof (char *); p++)
			for (s = 0; s < sizeof (suffixes) / sizeof (char *); s++)
			{
				/* 
				 * put together a possible filename
				 */
				strcpy (buf, dir);
				strcat (buf, subdirs[d]);
				strcat (buf, prefixes[p]);
				strcat (buf, module);
				strcat (buf, suffixes[s]);
#ifdef __EMX__
				strcpy (buf, (char *) __XOS2RedirRoot (buf));
#endif
				if ((stat (buf, &stat_buf) == 0) &&
					S_ISREG(stat_buf.st_mode))
				{
					return (buf);
				}
			}
	xfree (buf);
	return (NULL);
}

static void
CheckVersion (const char *module, XF86ModuleVersionInfo *data)
{
	int vercode[3];
	char verstr[4];
	long ver = data->xf86version;
	char *abiname;

	xf86Msg (X_INFO, "Module %s: vendor=\"%s\"\n",
			data->modname ? data->modname : "UNKNOWN!",
			data->vendor ? data->vendor : "UNKNOWN!");

	verstr[1] = verstr[3] = 0;
	verstr[2] = (ver & 0x1f) ? (ver & 0x1f) + 'a' - 1 : 0;
	ver >>= 5;
	verstr[0] = (ver & 0x1f) ? (ver & 0x1f) + 'A' - 1 : 0;
	ver >>= 5;
	vercode[2] = ver & 0x7f;
	ver >>= 7;
	vercode[1] = ver & 0x7f;
	ver >>= 7;
	vercode[0] = ver;
	xf86ErrorF("\tcompiled for %d.%d", vercode[0], vercode[1]);
	if (vercode[2] != 0)
		xf86ErrorF(".%d", vercode[2]);
	xf86ErrorF("%s%s, module version = %d.%d.%d\n", verstr, verstr + 2,
		   data->majorversion, data->minorversion, data->patchlevel);
	switch (data->abiclass)
	{
	case ABI_CLASS_NONE:
		abiname = NULL;
		break;
	case ABI_CLASS_ANSIC:
		abiname = "ANSI C Emulation";
		ver = LoaderVersionInfo.ansicVersion;
		break;
	case ABI_CLASS_VIDEODRV:
		abiname = "XFree86 Video Driver";
		ver = LoaderVersionInfo.videodrvVersion;
		break;
	case ABI_CLASS_XINPUT:
		abiname = "XInput Driver";
		ver = LoaderVersionInfo.xinputVersion;
		break;
	case ABI_CLASS_EXTENSION:
		abiname = "Server Extension";
		ver = LoaderVersionInfo.extensionVersion;
		break;
	case ABI_CLASS_FONT:
		abiname = "Font Renderer";
		ver = LoaderVersionInfo.fontVersion;
		break;
	default:
		/* XXX This should be an error condition */
		abiname = NULL;
		xf86MsgVerb(X_WARNING, 0,
			    "Unknown ABI class (%d) for module \"%s\"\n",
			    data->abiclass, data->modname);
	}
	if (abiname) {
		xf86ErrorFVerb(2, "\tABI class: %s, version %d.%d\n",
			       abiname, data->abiversion >> 16,
			       data->abiversion & 0xFFFF);
		if ((data->abiversion >> 16) != (ver >> 16)) {
			/* XXX This should be an error condition */
			xf86MsgVerb(X_WARNING, 0,
				"module ABI major version (%d) doesn't"
				" match the server's version (%d)\n",
				data->abiversion >> 16, ver >> 16);
		} else if ((data->abiversion & 0xFFFF) > (ver & 0xFFFF)) {
			/* XXX This should be an error condition */
			xf86MsgVerb(X_WARNING, 0,
				"module ABI minor version (%d) is "
				"newer than the server's version "
				"(%d)\n", data->abiversion & 0xFFFF,
				ver & 0xFFFF);
		}
	}
#if NOTYET
	if (data->checksum)
	{
		/* verify the checksum field */
		/* TO BE DONE */
	}
	else
	{
		ErrorF ("\t*** Checksum field is 0 - this module is untrusted!\n");
	}
#endif
}

ModuleDescPtr
LoadSubModule(ModuleDescPtr parent, const char *module, const char *path,
	      pointer options, int *errmaj, int *errmin)
{
	ModuleDescPtr submod;

	xf86MsgVerb(X_INFO, 3, "Loading sub module \"%s\"\n", module);

	if (path == NULL)
		path = parent->path;

	submod = LoadModule (module, path, options, errmaj, errmin);
	if (submod)
		parent->child = AddSibling (parent->child, submod);
	return submod;
}

void
LoadExtension (ExtensionModule *e)
{
	int i;

	if (e == NULL)
		return;
	xf86MsgVerb(X_INFO, 2, "Loading extension %s\n", e->name);

	for (i = 0; extension[i].name != NULL; i++)
	{
		if (strcmp (extension[i].name, e->name) == 0)
		{
			extension[i].initFunc = e->initFunc;
			extension[i].disablePtr = e->disablePtr;
			if (e->setupFunc != NULL)
				e->setupFunc();
			break;
		}
	}
	if (extension[i].name == NULL)
	{
		xf86MsgVerb(X_WARNING, 0,
			"Extension \"%s\" is not recognised\n", e->name);
	}
}

ModuleDescPtr
DuplicateModule(ModuleDescPtr mod)
{
	ModuleDescPtr ret;

	if (!mod)
		return NULL;

	ret = NewModuleDesc(mod->name);
	if (ret == NULL)
		return NULL;

	if (LoaderHandleOpen(mod->handle) == -1)
		return NULL;

	ret->identifier = mod->identifier;
	ret->client_id = mod->client_id;
	ret->in_use = mod->in_use;
	ret->handle = mod->handle;
	ret->SetupProc = mod->SetupProc;
	ret->TearDownProc = mod->TearDownProc;
	ret->TearDownData = NULL;
	ret->path = mod->path;
	ret->child = DuplicateModule(mod->child);
	ret->sib = DuplicateModule(mod->sib);

	return ret;
}

ModuleDescPtr
LoadModule (const char *module, const char *path, pointer options,
	    int *errmaj, int *errmin)
{
	ModuleInitProc initfunc = NULL;
	char *dir_elem = NULL;
	char *found = NULL;
	char *keep = NULL;
	char *name = NULL;
	char *path_elem = NULL;
	char *p = NULL;
	ModuleDescPtr ret = NULL;
	int wasLoaded = 0;

	xf86MsgVerb(X_INFO, 3, "LoadModule: \"%s\"\n", module);

	name = LoaderGetCanonicalName(module);
	if (!name)
		goto LoadModule_fail;
	ret = NewModuleDesc (module);
	if (!ret)
		goto LoadModule_fail;
	keep = dir_elem = (char *) xcalloc (1, strlen (path) + 2);
	if (!keep)
		goto LoadModule_fail;
	path_elem = (char *) xcalloc (1, strlen (path) + 2);
	if (!path_elem)
		goto LoadModule_fail;
	*dir_elem = ',';
	strcpy (dir_elem + 1, path);

	/* 
	 * if the module name is not a full pathname, we need to
	 * check the elements in the path
	 */
#ifndef __EMX__
	if (module[0] == '/')
		found = (char *) module;
#else
	/* accept a drive name here */
	if (isalpha (module[0]) && module[1] == ':' && module[2] == '/')
		found = (char *) module;
#endif
	dir_elem = strtok (dir_elem, ",");
	while ((!found) && (dir_elem != NULL))
	{
		/* 
		 * only allow fully specified path 
		 */
#ifndef __EMX__
		if (*dir_elem == '/')
#else
		if (*dir_elem == '/' ||
		(isalpha (dir_elem[0]) && dir_elem[1] == ':' && dir_elem[2] == '/'))
#endif
		{
			strcpy (path_elem, dir_elem);
			if (dir_elem[strlen (dir_elem) - 1] != '/')
			{
				path_elem[strlen (dir_elem)] = '/';
				path_elem[strlen (dir_elem) + 1] = '\0';
			}
			found = FindModule (module, path_elem);
		}
		dir_elem = strtok (NULL, ",");
	}

	/* 
	 * did we find the module?
	 */
	if (!found)
	{
		xf86Msg (X_WARNING, "Warning, couldn't open module %s\n",
				module);
		goto LoadModule_fail;
	}
	ret->handle = LoaderOpen (found, 0, errmaj, errmin, &wasLoaded);
	if (ret->handle < 0)
		goto LoadModule_fail;

	/* 
	 * now check if there's the special function
	 * <modulename>ModuleInit
	 * and if yes, call it.
	 */
	p = (char *) xalloc (strlen (name) + strlen ("ModuleInit") + 1);
	if (!p)
		goto LoadModule_fail;
	strcpy (p, name);
	strcat (p, "ModuleInit");
	initfunc = (ModuleInitProc) LoaderSymbol (p);

	if (initfunc)
	{
		ModuleSetupProc setup;
		ModuleTearDownProc teardown;
		XF86ModuleVersionInfo *vers;
		initfunc(&vers, &setup, &teardown);
		if (!wasLoaded) {
			if (vers) {
				CheckVersion (module, vers);
			} else {
				xf86Msg(X_WARNING,
					"LoadModule: Module %s does not supply"
					" version information\n", module);
			}
		}
		if (setup)
			ret->SetupProc = setup;
		if (teardown)
			ret->TearDownProc = teardown;
		ret->path = path;

#if 0
			/* Kept for reference */
			case MAGIC_DONT_CHECK_UNRESOLVED:
				check_unresolved_sema++;
				break;

#ifdef GLXEXT
			case MAGIC_GLX_VISUALS_INIT:
				GlxInitVisualsPtr = (GlxInitVisualsType) data;
				break;
#endif
#endif

	}
	else
	{
		/* no initfunc, complain */
		xf86Msg (X_WARNING, "LoadModule: Module %s does not have a %s "
				"routine.\n", module, p);
	}
	if (ret->SetupProc)
	{
		ret->TearDownData = ret->SetupProc (ret, options, errmaj, errmin);
		if (!ret->TearDownData)
		{
			goto LoadModule_fail;
		}
	}
	else if (options)
	{
		xf86Msg (X_WARNING, "Module Options present, but no SetupProc "
				"available for %s\n", module);
	}
	goto LoadModule_exit;

  LoadModule_fail:
	UnloadModule (ret);
	ret = NULL;

  LoadModule_exit:
	TestFree (found);
	TestFree (name);
	TestFree (p);
	TestFree (path_elem);
	TestFree (keep);
	return ret;
}

ModuleDescPtr
LoadDriver (const char *module, const char *path, int handle, pointer options,
	    int *errmaj, int *errmin)
{
return LoadModule (module, path, options, errmaj, errmin);
}

void
UnloadModule (ModuleDescPtr mod)
{
	UnloadModuleOrDriver (mod);
}

void
UnloadDriver (ModuleDescPtr mod)
{
	UnloadModuleOrDriver (mod);
}

static void
UnloadModuleOrDriver (ModuleDescPtr mod)
{
    char *n;

    if (mod == NULL || mod->name == NULL)
	return;

    if ((n = LoaderGetCanonicalName(mod->name)) == NULL)
	return;	/* XXX */

    xf86MsgVerb(X_INFO, 3, "UnloadModule: \"%s\"\n", n);

    if ((mod->TearDownProc) && (mod->TearDownData))
        mod->TearDownProc (mod->TearDownData);
    LoaderUnload (mod->handle);

    if (mod->child)
        UnloadModuleOrDriver (mod->child);
    if (mod->sib)
        UnloadModuleOrDriver (mod->sib);
    TestFree (mod->name);
    TestFree (n);
    xfree (mod);
}

void
FreeModuleDesc (ModuleDescPtr head)
{
	ModuleDescPtr sibs, prev;

	/*
	 * only free it if it's not marked as in use. In use means that it may
	 * be unloaded someday, and UnloadModule or UnloadDriver will free it
	 */
	if (head->in_use)
		return;
	if (head->child)
		FreeModuleDesc (head->child);
	sibs = head;
	while (sibs)
	{
		prev = sibs;
		sibs = sibs->sib;
		TestFree (prev->name);
		xfree (prev);
	}
}

ModuleDescPtr
NewModuleDesc (const char *name)
{
	ModuleDescPtr mdp = (ModuleDescPtr) xalloc (sizeof (ModuleDesc));

	if (mdp)
	{
		mdp->child = NULL;
		mdp->sib = NULL;
		mdp->demand_next = NULL;
		mdp->name = xstrdup (name);
		mdp->identifier = NULL;
		mdp->client_id = 0;
		mdp->in_use = 0;
		mdp->handle = -1;
		mdp->SetupProc = NULL;
		mdp->TearDownProc = NULL;
		mdp->TearDownData = NULL;
	}

	return (mdp);
}

ModuleDescPtr
AddSibling (ModuleDescPtr head, ModuleDescPtr new)
{
    new->sib = head;
    return (new);

}

void
LoaderErrorMsg(const char *name, const char *modname, int errmaj, int errmin)
{
	const char *msg;

	switch (errmaj) {
	case LDR_NOERROR:
		msg = "no error";
		break;
	case LDR_NOMEM:
		msg = "out of memory";
		break;
	case LDR_NOENT:
		msg = "module does not exist";
		break;
	case LDR_NOSUBENT:
		msg = "submodule does not exist";
		break;
	case LDR_NOSPACE:
		msg = "too many modules";
		break;
	case LDR_NOMODOPEN:
		msg = "open failed";
		break;
	case LDR_UNKTYPE:
		msg = "unknown module type";
		break;
	case LDR_NOLOAD:
		msg = "loader failed";
		break;
	case LDR_ONCEONLY:
		msg = "once-only module";
		break;
	case LDR_NOPORTOPEN:
		msg = "port open failed";
		break;
	case LDR_NOHARDWARE:
		msg = "no hardware found";
		break;
	default:
		msg = "uknown error";
	}
	xf86Msg(X_ERROR, "%s: Failed to load module \"%s\" (%s, %d)\n",
		name, modname, msg, errmin);
}


/* Given a module path or file name, return the module's canonical name */
char *
LoaderGetCanonicalName(const char *modname)
{
	char *str;
	const char *s, *e;

	/* Strip off any leading path */
	s = strrchr(modname, '/');
	if (s == NULL)
		s = modname;
	else
		s++;

	/* Strip off a leading "lib" */
	if (strncmp(s, "lib", 3) == 0)
		s += 3;
    
	/* Strip off a file suffix */
	e = strrchr(s, '.');
	if (e == NULL)
		e = s + strlen(s);

	/* Strip off a trailing "_drv" */
	if (e - s > 4 && strncmp(e - 4, "_drv", 4) == 0)
		e -= 4;

	str = (char *)xalloc(e - s + 1);
	if (str == NULL)
		return NULL;

	strncpy(str, s, e - s);
	str[e - s] = '\0';
	return str;
}

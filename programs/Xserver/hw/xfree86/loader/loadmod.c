/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loadmod.c,v 1.20 *
 * 1997/11/22 00:00:18 hohndel Exp $ */

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

#define NO_OSLIB_PROTOTYPES
#include "os.h"
#include "xf86_OSlib.h"
#include "xf86_ansic.h"
#if defined(SVR4)
#include <sys/stat.h>
#endif
#define LOADERDECLARATIONS
#include "misc.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "vga.h"
#include "xf86_Config.h"
#include "xf86Xinput.h"
#include "xf86_ldext.h"
#include "loader.h"
#include "loaderProcs.h"
#include "xf86Optrec.h"

int *xf86ccdScreenPrivateIndex = NULL;
void *(*xf86xaacfbfuncs) () = NULL;

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
static void CheckVersion (const char *, XF86ModuleVersionInfo *, int);
static void UnloadModuleOrDriver (ModuleDescPtr mod);

void
LoaderFixups (void)
{
	/* Need to call LRS here because the frame buffers get loaded last, * and 
	 * 
	 * * the drivers depend on them. */
	LoaderResolveSymbols ();
}

static char *subdirs[] =
{
	"",
	"drivers/",
	"extensions/",
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
					((S_IFMT & stat_buf.st_mode) == S_IFREG))
				{
					return (buf);
				}
			}
	xfree (buf);
	return (NULL);
}

static void
CheckVersion (module, data, cnt)
const char *module;
XF86ModuleVersionInfo *data;
int cnt;
{
	switch (cnt)
	{
	case 0:
		/* okay */
		break;
	case -1:
		/* no initfunc */
		ErrorF ("LoadModule: Module %s does not have a ModuleInit routine. Please fix\n",
				module);
		return;
	default:
		/* not first item */
		ErrorF ("LoadModule: ModuleInit of %s doesn't return MAGIC_VERSION as first data item. Please fix module!\n",
				module);
	}

	if (xf86Verbose)
	{
		int vercode[3];
		char verstr[4];
		int modcode[2];
		long ver = data->xf86version;
		long mod = data->modversion;

		ErrorF ("\tModule %s: vendor=\"%s\"\n",
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
		modcode[1] = mod & 0xffff;
		mod >>= 16;
		modcode[0] = mod;
		ErrorF ("\t  compiled for %d.%d.%d%s%s, module version = %d.%d\n",
				vercode[0], vercode[1], vercode[2],
				verstr, verstr + 2,
				modcode[0], modcode[1]);

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
}

void
LoadExtension (e)
ExtensionModule *e;
{
	int i;

	if (e == NULL)
		return;
	ErrorF ("loading extension %s\n", e->name);

	for (i = 0; extension[i].name != NULL; i++)
	{
		if (strcmp (extension[i].name, e->name) == 0)
		{
			extension[i].initFunc = e->initFunc;
			extension[i].disablePtr = e->disablePtr;
			break;
		}
	}
}

ModuleDescPtr
LoadModule (module, path, options, errmaj, errmin)
const char *module;
const char *path;
XF86OptionPtr options;
int *errmaj;
int *errmin;
{
	void (*initfunc) () = NULL;
	pointer data;
	INT32 magic;
	int version, cnt;

	char *dir_elem = NULL;
	char *found = NULL;
	char *keep = NULL;
	char *name = NULL;
	char *path_elem = NULL;
	char *p = NULL;
	xf86ccdDoBitbltProcPtr *pccddbb;
	xf86ccdXAAScreenInitProcPtr *pccdxaasi;
	ModuleDescPtr ret;
	ModuleDescPtr submod;

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
		ErrorF ("Warning, couldn't open module %s\n", module);
		goto LoadModule_fail;
	}
	ret->handle = LoaderOpen (found, 0, errmaj, errmin);
	if (ret->handle < 0)
		goto LoadModule_fail;

	/* 
	 * now check if there's the special function
	 * <modulename>ModuleInit
	 * and if yes, call it.
	 */
	name = (char *) xalloc (strlen (found) + strlen ("ModuleInit") + 1);
	if (!name)
		goto LoadModule_fail;
	strcpy (name, found);
	p = strrchr (name, '.');	/* get rid of the .o/.a/.so */
	if (p)
		*p = '\0';
	strcat (name, "ModuleInit");
	p = strrchr (name, '/');
	if (p)
		p++;
	else
		p = name;
	initfunc = (void (*)()) LoaderSymbol (p);

	version = 0;
	if (initfunc)
	{
		do
		{
			cnt = 0;

			initfunc (&data, &magic);
			switch (magic)
			{
			case MAGIC_DONE:
				break;
			case MAGIC_ADD_VIDEO_CHIP_REC:
				addChipRec ((void *) data);
				break;
			case MAGIC_LOAD:
				submod = LoadModule ((char *) data, path, options, errmaj, errmin);
				if (submod)
					ret->child = AddSibling (ret->child, submod);
				else
					goto LoadModule_fail;
				break;
			case MAGIC_CCD_DO_BITBLT:
				pccddbb = LoaderSymbol("xf86ccdDoBitblt");
				if (pccddbb)
					*pccddbb = (xf86ccdDoBitbltProcPtr)data;
				break;
			case MAGIC_CCD_SCREEN_PRIV_IDX:
				xf86ccdScreenPrivateIndex = data;
				break;
			case MAGIC_CCD_XAA_SCREEN_INIT:
				pccddbb = LoaderSymbol("xf86ccdXAAScreenInit");
				if (pccddbb)
					*pccddbb = (xf86ccdXAAScreenInitProcPtr)data;
				break;
			case MAGIC_LOAD_EXTENSION:
				LoadExtension ((ExtensionModule *) data);
				break;
			case MAGIC_VERSION:
				version++;
				CheckVersion (module, (XF86ModuleVersionInfo *) data, cnt);
				break;

#ifdef GLXEXT
			case MAGIC_GLX_VISUALS_INIT:
				GlxInitVisualsPtr = (GlxInitVisualsType) data;
				break;
#endif
			case MAGIC_DONT_CHECK_UNRESOLVED:
				check_unresolved_sema++;
				break;

			case MAGIC_SETUP_PROC:
				ret->SetupProc = (ModuleSetupProc) data;
				break;

			case MAGIC_TEARDOWN_PROC:
				ret->TearDownProc = (ModuleTearDownProc) data;
				break;

			default:
				ErrorF ("Unknown magic action %d\n", magic);
				break;
			}
			cnt++;
		}
		while (magic != MAGIC_DONE);
	}
	else
	{
		/* no initfunc, complain */
		ErrorF ("LoadModule: Module %s does not have a %s routine.\n",
				module, p);
	}
	if (ret->SetupProc)
	{
		ret->TearDownData = ret->SetupProc (options, errmaj, errmin);
		if (!ret->TearDownData)
		{
			goto LoadModule_fail;
		}
	}
	else if (options)
	{
		ErrorF ("Module Options present, ");
		ErrorF ("but no SetupProc available for %s\n", module);
	}
	goto LoadModule_exit;

  LoadModule_fail:
	UnloadModule (ret);
	ret = NULL;

  LoadModule_exit:
	TestFree (found);
	TestFree (name);
	TestFree (path_elem);
	TestFree (keep);
	return ret;
}

ModuleDescPtr
LoadDriver (const char *module, const char *path, int handle,
			XF86OptionPtr options, int *errmaj, int *errmin)
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
    if ((mod->TearDownProc) && (mod->TearDownData))
        mod->TearDownProc (mod->TearDownData);
    LoaderUnload (mod->handle);

    if (mod->child)
        UnloadModuleOrDriver (mod->child);
    if (mod->sib)
        UnloadModuleOrDriver (mod->sib);
	TestFree (mod->name);
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
		mdp->name = strdup (name);
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

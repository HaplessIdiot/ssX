/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
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
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/xf86cfg/loader.c,v 1.8 2001/07/06 07:37:10 paulo Exp $
 */

#include "config.h"
#include "options.h"
#include "loader.h"
#include <X11/Xresource.h>

#ifdef USE_MODULES
#include <stdio.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif
#include <string.h>
#include <setjmp.h>
#include <sys/signal.h>

#ifndef SIGNALRETURNSINT
void sig_handler(int);
#else
int sig_handler(int);
#endif	/* SIGNALRETURNSINT */

static Bool EnumDatabase(XrmDatabase, XrmBindingList, XrmQuarkList,
			 XrmRepresentation*, XrmValue*, XPointer);
extern void CheckChipsets(xf86cfgModuleOptions*, int*);

static jmp_buf jmp;
int signal_caught;
int error_level;
char *loaderPath, **loaderList, **ploaderList;
extern XrmDatabase options_xrm;
extern int noverify;
extern ModuleType module_type;
static OptionInfoPtr option;

extern FontModule *font_module;
extern int numFontModules;

#ifndef SIGNALRETURNSINT
void
#else
int
#endif
sig_handler(int sig)
{
    char *str;

    switch (sig) {
	case SIGTRAP:
	    str = "TRAP";
	    break;
	case SIGBUS:
	    str = "BUS";
	    break;
	case SIGSEGV:
	    str = "SEGV";
	    break;
	case SIGILL:
	    str = "ILL";
	    break;
	case SIGFPE:
	    str = "FPE";
	    break;
	default:
	    str = "???";
	    break;
    }

    if (signal_caught == 1) {
	ErrorF("  ERROR I am dead.\n");
	exit(1);
    }
    else if (signal_caught == 2)
	abort();
    ++signal_caught;
    ErrorF("  ERROR SIG%s caught!\n", str);
    error_level += 50;
    longjmp(jmp, 1);
    /*NOTREACHED*/
}

static Bool
EnumDatabase(XrmDatabase db, XrmBindingList bindings, XrmQuarkList quarks,
	     XrmRepresentation *type, XrmValue *value, XPointer closure)
{
    char *res = XrmQuarkToString(quarks[1]);

    option = module_options->option;
    while (option->name) {
	if (strcmp(option->name, res) == 0)
	    return (False);
	++option;
    }
    ErrorF("    ERROR %s.%s is not used\n",
	   XrmQuarkToString(quarks[0]), res);
    ++error_level;

    return (False);
}

Bool
LoaderInitializeOptions(void)
{
    static int first = 1;
    static char *modules = "lib/modules";
    Bool options_ok = False;
    char query[256];
    char *type;
    XrmValue value;
    XrmQuark names[2];
    XrmQuark classes[2];
    int i;
    static ModuleType module_types[] = {
	GenericModule, FontRendererModule, InputModule, VideoModule, NullModule
    };

    /* The offset in this vector must match loader.h:enum ModuleType values */
    static char *module_strs[] = {
	"Null Module", "Video Module", "Input Module", "Generic Module", "Font Module"
    };

    if (first) {
	xf86cfgLoaderInit();
	first = 0;
    }

    if (XF86Module_path == NULL) {
	XF86Module_path = malloc(strlen(XFree86Dir) + strlen(modules) + 2);
	sprintf(XF86Module_path, "%s/%s", XFree86Dir, modules);
    }

    if (loaderPath == NULL || strcmp(XF86Module_path, loaderPath))
	loaderPath = strdup(XF86Module_path);
    else
	/* nothing new */
	return (True);

    if (!noverify) {
	options_ok = InitializeOptionsDatabase();
	InitializePciInfo();
    }

    for (i = 0; module_types[i] != NullModule; i++) {
	xf86cfgLoaderInitList(module_types[i]);
	if (!noverify)
	    ErrorF("================= Checking modules of type \"%s\" =================\n",
		   module_strs[module_types[i]]);

	if (loaderList) {
	    for (ploaderList = loaderList; *ploaderList; ploaderList++) {
		if (!noverify) {
		    if (setjmp(jmp) == 0) {
			int ok, nfont_modules;

			nfont_modules = numFontModules;
			error_level = 0;
			signal_caught = 0;
			signal(SIGTRAP, sig_handler);
			signal(SIGBUS, sig_handler);
			signal(SIGSEGV, sig_handler);
			signal(SIGILL, sig_handler);
			signal(SIGFPE, sig_handler);
			ErrorF("CHECK MODULE %s\n", *ploaderList);
			if ((ok = xf86cfgCheckModule()) == 0) {
			    ErrorF("  ERROR Failed to load module.\n");
			    error_level += 50;
			}
			else if (module_type != module_types[i]) {
			    ErrorF("  WARNING %s recognized as a \"%s\"\n", *ploaderList,
				   module_strs[module_type]);
			    ++error_level;
			}
			signal(SIGTRAP, SIG_DFL);
			signal(SIGBUS, SIG_DFL);
			signal(SIGSEGV, SIG_DFL);
			signal(SIGILL, SIG_DFL);
			signal(SIGFPE, SIG_DFL);
			if (ok) {
			    if (options_ok) {
				if ((module_options == NULL || module_options->option == NULL) &&
				    module_type != GenericModule) {
				    ErrorF("  WARNING Not a generic module, but no options available.\n");
				    ++error_level;
				}
				else if (module_options && strcmp(module_options->name, *ploaderList) == 0) {
				    ErrorF("  CHECK OPTIONS\n");
				    option = module_options->option;

				    while (option->name) {
					XmuSnprintf(query, sizeof(query), "%s.%s", *ploaderList, option->name);
					if (!XrmGetResource(options_xrm, query, "Module.Option", &type, &value) ||
					    value.addr == NULL) {
					    ErrorF("    ERROR no description for %s\n", query);
					    ++error_level;
					}
					++option;
				    }

				    /* now do a linear search for Options file entries that are not
				     * in the driver.
				     */
				    names[0] = XrmPermStringToQuark(module_options->name);
				    classes[0] = XrmPermStringToQuark("Option");
				    names[1] = classes[1] = NULLQUARK;
				    (void)XrmEnumerateDatabase(options_xrm, &names, &classes, XrmEnumOneLevel,
							       EnumDatabase, NULL);
				}
			    }
			    else {
				ErrorF("  ERROR Options file missing.\n");
				error_level += 10;
			    }

			    if (module_type == VideoModule &&
				(module_options == NULL || module_options->vendor < 0 ||
				 module_options->chipsets == NULL)) {
				ErrorF("  WARNING No vendor/chipset information available.\n");
				++error_level;
			    }
			    else if (module_type == VideoModule) {
				if (module_options == NULL) {
				    ErrorF("  ERROR No module_options!?!\n");
				    error_level += 50;
				}
				else {
				    ErrorF("  CHECK CHIPSETS\n");
				    CheckChipsets(module_options, &error_level);
				}
			    }

			    /* font modules check */
			    if (module_type == FontRendererModule) {
				if (strcmp(*ploaderList, font_module->name)) {
				    /* not an error */
				    ErrorF("  NOTICE FontModule->name specification mismatch: \"%s\" \"%s\"\n",
					   *ploaderList, font_module->name);
				    ++error_level;
				}
				if (nfont_modules + 1 != numFontModules) {
				    /* not an error */
				    ErrorF("  NOTICE font module \"%s\" loaded more than one font renderer.\n",
					   *ploaderList);
				    ++error_level;
				}
			    }
			    else if (nfont_modules != numFontModules) {
				ErrorF("  WARNING number of font modules changed from %d to %d.\n",
				       nfont_modules, numFontModules);
				++error_level;
			    }
			}
		    }
		    ErrorF("  SUMMARY error_level set to %d.\n\n", error_level);
		}
		else
		    (void)xf86cfgCheckModule();
	    }
	    xf86cfgLoaderFreeList();
	}
	else
	    ErrorF("  ERROR Failed to initialize module list.\n");
    }

    return (True);
}
#endif

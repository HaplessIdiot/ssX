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

/* $XFree86: xc/programs/xedit/lisp/require.c,v 1.3 2001/09/21 05:08:43 paulo Exp $ */

#include "require.h"

/*
 * Initialization
 */
static char *BadArgumentAt = "bad argument %s, at %s";

/*
 * Implementation
 */
LispObj *
Lisp_Load(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *file = CAR(list);
    int verbose = 0;
    int print = 0;
    int ifdoesnotexist = 0;

    if (file->type != LispString_t)
	LispDestroy(mac, BadArgumentAt, LispStrObj(mac, file), fname);

    for (list = CDR(list); list != NIL; list = CDR(list)) {
	if (CAR(list)->type != LispAtom_t)
	    LispDestroy(mac, BadArgumentAt, LispStrObj(mac, CAR(list)), fname);
	else {
	    if (strcmp(CAR(list)->data.atom, ":VERBOSE") == 0) {
		if ((list = CDR(list)) == NIL)
		    LispDestroy(mac, "expecting :VERBOSE, at %s", fname);
		verbose = CAR(list) != NIL;
	    }
	    else if (strcmp(CAR(list)->data.atom, ":PRINT") == 0) {
		if ((list = CDR(list)) == NIL)
		    LispDestroy(mac, "expecting :PRINT, at %s", fname);
		print = CAR(list) != NIL;
	    }
	    else if (strcmp(CAR(list)->data.atom, ":IF-DOES-NOT-EXIST") == 0) {
		if ((list = CDR(list)) == NIL)
		    LispDestroy(mac, "expecting :IF-DOES-NOT-EXIST, at %s",
				fname);
		ifdoesnotexist = CAR(list) != NIL;
	    }
	}
    }

    return (_LispLoadFile(mac, file->data.atom, fname,
			  verbose, print, ifdoesnotexist));
}

LispObj *
Lisp_Require(LispMac *mac, LispObj *list, char *fname)
{
    LispObj *feat = CAR(list), *nam, *obj;
    char filename[1024], *ext;
    int len;

    if (feat->type != LispString_t)
	LispDestroy(mac, BadArgumentAt, LispStrObj(mac, feat), fname);

    if (CDR(list) != NIL) {
	nam = CAR(CDR(list));

	if (nam->type != LispString_t)
	    LispDestroy(mac, "%s is not of type string, at %s",
			LispStrObj(mac, nam), fname);
    }
    else
	nam = feat;

    for (obj = MOD; obj != NIL; obj = CDR(obj)) {
	if (CAR(obj)->data.atom == feat->data.atom)
	    return (feat);
    }

    if (nam->data.atom[0] != '/') {
#ifdef LISPDIR
	snprintf(filename, sizeof(filename), "%s", LISPDIR);
#else
	getcwd(filename, sizeof(filename));
#endif
    }
    else
	filename[0] = '\0';
    *(filename + sizeof(filename) - 5) = '\0';	/* make sure there is place for ext */
    len = strlen(filename);
    if (!len || filename[len - 1] != '/') {
	strcat(filename, "/");
	++len;
    }

    snprintf(filename + len, sizeof(filename) - len - 5, "%s", nam->data.atom);

    ext = filename + strlen(filename);

#ifdef SHARED_MODULES
    strcpy(ext, ".so");
    if (access(filename, R_OK) == 0) {
	LispModule *module;
	char data[64];
	int len;

	if (mac->module == NULL) {
	    /* export our own symbols */
	    if (dlopen(NULL, RTLD_LAZY | RTLD_GLOBAL) == NULL)
		LispDestroy(mac, dlerror());
	}

#if 0
	if (mac->interactive)
	    fprintf(lisp_stderr, "; Loading %s\n", filename);
#endif
	module = (LispModule*)LispMalloc(mac, sizeof(LispModule));
	if ((module->handle = dlopen(filename, RTLD_LAZY | RTLD_GLOBAL)) == NULL)
	    LispDestroy(mac, dlerror());
	snprintf(data, sizeof(data), "%sLispModuleData", nam->data.atom);
	if ((module->data = (LispModuleData*)dlsym(module->handle, data)) == NULL) {
	    dlclose(module->handle);
	    LispDestroy(mac, "cannot find LispModuleData for %s",
			LispStrObj(mac, feat));
	}
	LispMused(mac, module);
	module->next = mac->module;
	mac->module = module;
	if (module->data->load)
	    (module->data->load)(mac);

	return (Lisp_Provide(mac, CONS(feat, NIL), "PROVIDE"));
    }
#endif

    strcpy(ext, ".lsp");
    (void)_LispLoadFile(mac, filename, fname, 0, 0, 0);

    return (feat);
}

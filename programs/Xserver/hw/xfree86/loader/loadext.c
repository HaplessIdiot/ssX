/* $XFree86$ */

/* Maybe this file belongs elsewhere? */

#define LOADERDECLARATIONS
#include "loaderProcs.h"
#include "misc.h"
#include "xf86.h"

ExtensionModule *ExtensionModuleList = NULL;
static int numExtensionModules = 0;


static ExtensionModule *
NewExtensionModule(void)
{
	ExtensionModule *save = ExtensionModuleList;
	int n;

	/* Sanity check */
	if (!ExtensionModuleList)
		numExtensionModules = 0;

	n = numExtensionModules + 1;
	ExtensionModuleList = xrealloc(ExtensionModuleList,
				       (n + 1) * sizeof(ExtensionModule));
	if (ExtensionModuleList == NULL) {
		ExtensionModuleList = save;
		return NULL;
	} else {
		numExtensionModules++;
		ExtensionModuleList[numExtensionModules].name = NULL;
		return ExtensionModuleList + (numExtensionModules - 1);
	}
}

void
LoadExtension(ExtensionModule *e, Bool builtin)
{
	ExtensionModule *newext;

	if (e == NULL || e->name == NULL)
		return;

	if (!(newext = NewExtensionModule()))
		return;

	if (builtin)
		xf86MsgVerb(X_INFO, 2, "Initializing built-in extension %s\n",
				e->name);
	else
		xf86MsgVerb(X_INFO, 2, "Loading extension %s\n", e->name);

	newext->name = e->name;
	newext->initFunc = e->initFunc;
	newext->disablePtr = e->disablePtr;
	newext->setupFunc = e->setupFunc;
	newext->dependencies = e->dependencies;

	if (e->setupFunc != NULL)
		e->setupFunc();
}


/* $XFree86: xc/lib/font/include/fontmod.h,v 1.3 1998/12/13 05:32:33 dawes Exp $ */

#ifndef _FONTMOD_H_
#define _FONTMOD_H_

#ifndef _XF86MODULE_H
typedef struct module_desc *ModuleDescPtr;
#endif

typedef void (*InitFont)(void);

typedef struct font_module {
    InitFont		initFunc;
    char *		name;
    ModuleDescPtr	module;
} FontModule;

extern FontModule *FontModuleList;

#endif /* _FONTMOD_H_ */

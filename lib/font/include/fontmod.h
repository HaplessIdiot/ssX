/* $XFree86: xc/lib/font/include/fontmod.h,v 1.1.2.1 1998/07/05 14:36:07 dawes Exp $ */

#ifndef _FONTMOD_H_
#define _FONTMOD_H_

typedef void (*InitFont)(void);

typedef struct {
    InitFont	initFunc;
    char *	name;
} FontModule;

extern FontModule FontModuleList[];

#endif /* _FONTMOD_H_ */

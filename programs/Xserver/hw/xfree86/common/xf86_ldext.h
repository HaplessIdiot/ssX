/*
 * Copyright (C) 1997 Matthieu Herrb
 *
 * $Source: /vol1/history/xf86/xc/programs/Xserver/hw/xfree86/common/Attic/xf86_ldext.h,v $
 * $Revision: 3.1 $
 * $Date: 1997/04/08 10:11:54 $
 * 
 */
#ifndef _XF86_LDEXT_H
#define _XF86_LDEXT_H

#if NeedFunctionPrototypes
#define INITARGS void
#else
#define INITARGS /*nothing*/
#endif
typedef void (*InitExtension)(INITARGS);

typedef struct ExtensionModule {
    InitExtension initFunc;
    char *name;
    Bool *disablePtr;
} ExtensionModule;

extern ExtensionModule extension[];

#endif

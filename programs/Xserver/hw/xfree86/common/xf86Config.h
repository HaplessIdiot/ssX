/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Config.h,v 1.2 1998/07/25 16:54:59 dawes Exp $ */
/*
 * Copyright 1997 by The XFree86 Project, Inc
 */

#ifndef _xf86_config_h
#define _xf86_config_h

#ifdef HAVE_PARSER_DECLS
/*
 * global structure that holds the result of parsing the config file
 */
extern XF86ConfigPtr xf86configptr;
#endif

/*
 * prototypes
 */
char ** xf86ModulelistFromConfig(pointer **);
char ** xf86DriverlistFromConfig(void);
Bool xf86HandleConfigFile(void);

#endif /* _xf86_config_h */

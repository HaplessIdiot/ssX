/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Config.h,v 1.3 1999/04/05 07:13:07 dawes Exp $ */
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
char ** xf86InputDriverlistFromConfig(void);
Bool xf86BuiltinInputDriver(const char *);
Bool xf86HandleConfigFile(void);

#endif /* _xf86_config_h */

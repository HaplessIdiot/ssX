/* $XFree86$ */

#ifndef _xf86InPriv_h
#define _xf86InPriv_h

/* xf86Globals.c */
#ifdef XFree86LOADER
extern InputDriverPtr *xf86InputDriverList;
#else
extern InputDriverPtr xf86InputDriverList[];
#endif

/* xf86Mouse.c */
extern InputDriverRec xf86MOUSE;

#endif /* _xf86InPriv_h */

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86InPriv.h,v 1.2 1999/05/09 06:06:19 dawes Exp $ */

#ifndef _xf86InPriv_h
#define _xf86InPriv_h

/* xf86Globals.c */
#ifdef XFree86LOADER
extern InputDriverPtr *xf86InputDriverList;
#else
extern InputDriverPtr xf86InputDriverList[];
#endif
extern int xf86NumInputDrivers;

/* xf86Mouse.c */
extern InputDriverRec xf86MOUSE;

/* xf86Xinput.c */
void xf86ActivateDevice(InputInfoPtr pInfo);

#endif /* _xf86InPriv_h */

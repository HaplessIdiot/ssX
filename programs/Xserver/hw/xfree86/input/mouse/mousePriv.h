/* $XFree86: xc/programs/Xserver/hw/xfree86/input/mouse/mouse.h,v 1.7 1999/06/05 15:55:27 dawes Exp $ */

/*
 * Copyright (c) 1997-1999 by The XFree86 Project, Inc.
 */

#ifndef _X_MOUSEPRIV_H
#define _X_MOUSEPRIV_H

/* Private interface for the mouse driver. */

typedef struct {
    const char *	name;
    int			class;
    const char **	defaults;
    MouseProtocolID	id;
} MouseProtocolRec, *MouseProtocolPtr;

/* mouse proto flags */
#define MPF_NONE		0x00
#define MPF_SAFE		0x01

/* pnp.c */
int MouseGetPnpProtocol(InputInfoPtr pInfo);

#endif /* _X_MOUSE_H */

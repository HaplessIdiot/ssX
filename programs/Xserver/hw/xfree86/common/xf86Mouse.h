/* $XFree86$ */

/*
 * Copyright (c) 1997-1999 by The XFree86 Project, Inc.
 */

#ifndef _XF86MOUSE_H
#define _XF86MOUSE_H

#include "xisb.h"

/* Mouse device private record */

typedef struct _MouseDevRec {
    DeviceProc		mseProc;	/* procedure for initializing */
    void		(*mseEvents)(struct _MouseDevRec *);
					/* proc for processing events */
    DeviceIntPtr	device;
    int			mseFd;
    const char *	mseDevice;
    const char *	protocol;
    MouseProtocol	mseType;
    int			mseModel;
    int			baudRate;
    int			oldBaudRate;
    int			sampleRate;
    int			lastButtons;
    int			threshold;	/* acceleration */
    int			num;
    int			den;
    int			buttons;	/* # of buttons */
    int			emulateState;	/* automata state for 2 button mode */
    Bool		emulate3Buttons;
    int			emulate3Timeout;/* Timeout for 3 button emulation */
    Bool		chordMiddle;
    Bool		clearDTR;
    Bool		clearRTS;
    int			mouseFlags;	/* Flags to Clear after opening
					 * mouse dev */
    int			truebuttons;	/* Arg to maintain before
					 * emulate3buttons timer callback */
    int			resolution;
    int			negativeZ;
    int			positiveZ;
    XISBuffer *		buffer;
#ifndef MOUSE_PROTOCOL_IN_KERNEL
    int			protoBufTail;
#ifndef WSCONS_SUPPORT
    unsigned char	protoBuf[8];
#else
    unsigned char	protoBuf[32];
#endif
    unsigned char	protoPara[7];
    unsigned char	inSync;		/* driver in sync with datastream */
#endif
    /* xqueue part */
    int			xquePending;	/* was xqueFd, but nothing uses that */
    int			xqueSema;
#ifdef XINPUT
    struct _LocalDeviceRec *local;
#endif
} MouseDevRec, *MouseDevPtr;

#ifndef XINPUT
#define MOUSE_DEV(dev) (MouseDevPtr) (dev)->public.devicePrivate
#endif

/* Mouse device private record */

#define MSE_MAPTOX	-1
#define MSE_MAPTOY	-2
#define MSE_MAXBUTTONS	12
#define MSE_DFLTBUTTONS	 3

#endif /* _XF86MOUSE_H */

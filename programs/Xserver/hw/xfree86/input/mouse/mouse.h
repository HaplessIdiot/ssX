/* $XFree86: xc/programs/Xserver/hw/xfree86/input/mouse/mouse.h,v 1.3 1999/05/16 06:55:53 dawes Exp $ */

/*
 * Copyright (c) 1997-1999 by The XFree86 Project, Inc.
 */

#ifndef _X_MOUSE_H
#define _X_MOUSE_H

#include "xisb.h"

/* Protocol IDs.  These are for internal use only. */
typedef enum {
    PROT_UNKNOWN = -2,
    PROT_UNSUP = -1,		/* protocol is not supported */
    PROT_MS = 0,
    PROT_MSC,
    PROT_MM,
    PROT_LOGI,
    PROT_LOGIMAN,
    PROT_MMHIT,
    PROT_GLIDE,
    PROT_IMSERIAL,
    PROT_THINKING,
    PROT_ACECAD,
    PROT_PS2,
    PROT_IMPS2,
    PROT_THINKPS2,
    PROT_MMPS2,
    PROT_GLIDEPS2,
    PROT_NETPS2,
    PROT_NETSCPS2,
    PROT_BM,
    PROT_AUTO,
    PROT_SYSMOUSE,
    PROT_NUMPROTOS	/* This must always be last. */
} ProtocolID;

/*
 * Mouse device record.  This is shared by the mouse driver and the OSMouse
 * layer.
 */

typedef void (*PostEventProc)(InputInfoPtr pInfo, int buttons, int dx, int dy);

typedef struct _MouseDevRec {
    PtrCtrlProcPtr	Ctrl;
    PostEventProc	PostEvent;
    DeviceIntPtr	device;
    int			mseFd;
    const char *	mseDevice;
    const char *	protocol;
    int			protocolID;
    int			class;
    Bool		automatic;
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
    int			protoBufTail;
    unsigned char	protoBuf[8];
    unsigned char	protoPara[8];
    unsigned char	inSync;		/* driver in sync with datastream */
    pointer		mousePriv;	/* private area */
    InputInfoPtr	pInfo;
} MouseDevRec, *MouseDevPtr;

/* Mouse device private record */

#define MSE_MAPTOX	-1
#define MSE_MAPTOY	-2
#define MSE_MAXBUTTONS	12
#define MSE_DFLTBUTTONS	 3

typedef struct {
    const char *	name;
    int			class;
    const char **	defaults;
    ProtocolID		id;
} MouseProtocolRec, *MouseProtocolPtr;

/* mouse proto flags */
#define MPF_NONE		0x00
#define MPF_SAFE		0x01

#endif /* _X_MOUSE_H */

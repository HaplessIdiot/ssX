/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bsd/bsd_mouse.c,v 1.11 1999/12/06 02:50:22 robin Exp $ */

/*
 * Copyright 1999 by The XFree86 Project, Inc.
 */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86Xinput.h"
#include "xf86OSmouse.h"
#include "xisb.h"
#include "mipointer.h"
#ifdef WSCONS_SUPPORT
#include <dev/wscons/wsconsio.h>
#endif

static void wsconsSigioReadInput (int fd, void *closure);

static int
SupportedInterfaces(void)
{
#if defined(__NetBSD__)
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_AUTO;
#elif defined(__FreeBSD__)
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_XPS2 | MSE_AUTO | MSE_MISC;
#else
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_XPS2 | MSE_AUTO;
#endif
}

/* Names of protocols that are handled internally here. */
static const char *internalNames[] = {
#if defined(WSCONS_SUPPORT)
	"WSMouse",
#endif
	NULL
};

/*
 * Names of MSC_MISC protocols that the OS supports.  These are decoded by
 * main "mouse" driver.
 */
static const char *miscNames[] = {
#if defined(__FreeBSD__)
	"SysMouse",
#endif
	NULL
};

static const char **
BuiltinNames(void)
{
    return internalNames;
}

static Bool
CheckProtocol(const char *protocol)
{
    int i;

    for (i = 0; internalNames[i]; i++)
	if (xf86NameCmp(protocol, internalNames[i]) == 0)
	    return TRUE;
    for (i = 0; miscNames[i]; i++)
	if (xf86NameCmp(protocol, miscNames[i]) == 0)
	    return TRUE;
    return FALSE;
}

static const char *
DefaultProtocol(void)
{
#if defined(__FreeBSD__)
    return "Auto";
#else
    return NULL;
#endif
}

#if defined(__FreeBSD__) && defined(MOUSE_PROTO_SYSMOUSE)
static struct {
	int dproto;
	const char *name;
} devproto[] = {
	{ MOUSE_PROTO_MS,		"Microsoft" },
	{ MOUSE_PROTO_MSC,		"MouseSystems" },
	{ MOUSE_PROTO_LOGI,		"Logitech" },
	{ MOUSE_PROTO_MM,		"MMSeries" },
	{ MOUSE_PROTO_LOGIMOUSEMAN,	"MouseMan" },
	{ MOUSE_PROTO_BUS,		"BusMouse" },
	{ MOUSE_PROTO_INPORT,		"BusMouse" },
	{ MOUSE_PROTO_PS2,		"PS/2" },
	{ MOUSE_PROTO_HITTAB,		"MMHitTab" },
	{ MOUSE_PROTO_GLIDEPOINT,	"GlidePoint" },
	{ MOUSE_PROTO_INTELLI,		"Intellimouse" },
	{ MOUSE_PROTO_THINK,		"ThinkingMouse" },
	{ MOUSE_PROTO_SYSMOUSE,		"SysMouse" }
};
	
static const char *
SetupAuto(InputInfoPtr pInfo, int *protoPara)
{
    int i;
    mousehw_t hw;
    mousemode_t mode;

    if (pInfo->fd == -1)
	return NULL;

    /* set the driver operation level, if applicable */
    i = 1;
    ioctl(pInfo->fd, MOUSE_SETLEVEL, &i);
    
    /* interrogate the driver and get some intelligence on the device. */
    hw.iftype = MOUSE_IF_UNKNOWN;
    hw.model = MOUSE_MODEL_GENERIC;
    ioctl(pInfo->fd, MOUSE_GETHWINFO, &hw);
    if (ioctl(pInfo->fd, MOUSE_GETMODE, &mode) == 0) {
	for (i = 0; i < sizeof(devproto)/sizeof(devproto[0]); ++i) {
	    if (mode.protocol == devproto[i].dproto) {
		/* override some parameters */
		if (protoPara) {
		    protoPara[4] = mode.packetsize;
		    protoPara[0] = mode.syncmask[0];
		    protoPara[1] = mode.syncmask[1];
		}
		return devproto[i].name;
	    }
	}
    }
    return NULL;
}

static void
SetSysMouseRes(InputInfoPtr pInfo, const char *protocol, int rate, int res)
{
    mousemode_t mode;

    mode.rate = rate > 0 ? rate : -1;
    mode.resolution = res > 0 ? res : -1;
    mode.accelfactor = -1;
    mode.level = -1;
    ioctl(pInfo->fd, MOUSE_SETMODE, &mode);
}
#endif

#if defined(WSCONS_SUPPORT)
#define NUMEVENTS 64

static int
wsconsMouseProc(DeviceIntPtr pPointer, int what)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    unsigned char map[MSE_MAXBUTTONS + 1];
    int nbuttons;

    pInfo = pPointer->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device = pPointer;

    switch (what) {
    case DEVICE_INIT: 
	pPointer->public.on = FALSE;

	for (nbuttons = 0; nbuttons < MSE_MAXBUTTONS; ++nbuttons)
	    map[nbuttons + 1] = nbuttons + 1;

	InitPointerDeviceStruct((DevicePtr)pPointer, 
				map, 
				min(pMse->buttons, MSE_MAXBUTTONS),
				miPointerGetMotionEvents, 
				pMse->Ctrl,
				miPointerGetMotionBufferSize());

	/* X valuator */
	xf86InitValuatorAxisStruct(pPointer, 0, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pPointer, 0);
	/* Y valuator */
	xf86InitValuatorAxisStruct(pPointer, 1, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pPointer, 1);
	xf86MotionHistoryAllocate(pInfo);
	break;

    case DEVICE_ON:
	pInfo->fd = xf86OpenSerial(pInfo->options);
	if (pInfo->fd == -1)
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    pMse->buffer = XisbNew(pInfo->fd,
			      NUMEVENTS * sizeof(struct wscons_event));
	    if (!pMse->buffer) {
		xfree(pMse);
		xf86CloseSerial(pInfo->fd);
		pInfo->fd = -1;
	    } else {
		xf86FlushInput(pInfo->fd);
		if (!xf86InstallSIGIOHandler (pInfo->fd, wsconsSigioReadInput, pInfo))
		    AddEnabledDevice(pInfo->fd);
	    }
	}
	pMse->lastButtons = 0;
	pMse->emulateState = 0;
	pPointer->public.on = TRUE;
	break;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
	if (pInfo->fd != -1) {
	    RemoveEnabledDevice(pInfo->fd);
	    if (pMse->buffer) {
		XisbFree(pMse->buffer);
		pMse->buffer = NULL;
	    }
	    xf86CloseSerial(pInfo->fd);
	    pInfo->fd = -1;
	}
	pPointer->public.on = FALSE;
	usleep(300000);
	break;
    }
    return Success;
}


static void
wsconsReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    static struct wscons_event eventList[NUMEVENTS];
    int n, c; 
    struct wscons_event *event = eventList;
    unsigned char *pBuf;

    pMse = pInfo->private;

    XisbBlockDuration(pMse->buffer, -1);
    pBuf = (unsigned char *)eventList;
    n = 0;
    while ((c = XisbRead(pMse->buffer)) >= 0 && n < sizeof(eventList)) {
	pBuf[n++] = (unsigned char)c;
    }

    if (n == 0)
	return;

    n /= sizeof(struct wscons_event);
    while( n-- ) {
	int buttons = pMse->lastButtons;
	int dx = 0, dy = 0, dz = 0, dw = 0;
	switch (event->type) {
	case WSCONS_EVENT_MOUSE_UP:
#define BUTBIT (1 << (event->value <= 2 ? 2 - event->value : event->value))
	    buttons &= ~BUTBIT;
	    break;
	case WSCONS_EVENT_MOUSE_DOWN:
	    buttons |= BUTBIT;
	    break;
	case WSCONS_EVENT_MOUSE_DELTA_X:
	    dx = event->value;
	    break;
	case WSCONS_EVENT_MOUSE_DELTA_Y:
	    dy = -event->value;
	    break;
#ifdef WSCONS_EVENT_MOUSE_DELTA_Z
	case WSCONS_EVENT_MOUSE_DELTA_Z:
	    dz = event->value;
	    break;
#endif
	default:
	    xf86Msg(X_WARNING, "%s: bad wsmouse event type=%d\n", pInfo->name,
		    event->type);
	    continue;
	}

	pMse->PostEvent(pInfo, buttons, dx, dy, dz, dw);
	++event;
    }
    return;
}

static void
wsconsSigioReadInput (int fd, void *closure)
{
    wsconsReadInput ((InputInfoPtr) closure);
}


/* This function is called when the protocol is "wsmouse". */
static Bool
wsconsPreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse = pInfo->private;

    pMse->protocol = protocol;
    xf86Msg(X_CONFIG, "%s: Protocol: %s\n", pInfo->name, protocol);

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, NULL, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

    /* Check if the device can be opened. */
    pInfo->fd = xf86OpenSerial(pInfo->options);
    if (pInfo->fd == -1) {
	if (xf86GetAllowMouseOpenFail())
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    xf86Msg(X_ERROR, "%s: cannot open input device\n", pInfo->name);
	    xfree(pMse);
	    return FALSE;
	}
    }
    xf86CloseSerial(pInfo->fd);
    pInfo->fd = -1;

    /* Process common mouse options (like Emulate3Buttons, etc). */
    pMse->CommonOptions(pInfo);

    /* Setup the local procs. */
    pInfo->device_control = wsconsMouseProc;
    pInfo->read_input = wsconsReadInput;

    pInfo->flags |= XI86_CONFIGURED;
    return TRUE;
}
#endif

OSMouseInfoPtr
xf86OSMouseInit(int flags)
{
    OSMouseInfoPtr p;

    p = xcalloc(sizeof(OSMouseInfoRec), 1);
    if (!p)
	return NULL;
    p->SupportedInterfaces = SupportedInterfaces;
    p->BuiltinNames = BuiltinNames;
    p->DefaultProtocol = DefaultProtocol;
    p->CheckProtocol = CheckProtocol;
#if defined(__FreeBSD__) && defined(MOUSE_PROTO_SYSMOUSE)
    p->SetupAuto = SetupAuto;
    p->SetPS2Res = SetSysMouseRes;
    p->SetBMRes = SetSysMouseRes;
    p->SetMiscRes = SetSysMouseRes;
#endif
#if defined(WSCONS_SUPPORT)
    p->PreInit = wsconsPreInit;
#endif
    return p;
}

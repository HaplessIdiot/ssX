/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sysv/xqueue.c,v 3.15 1999/05/22 08:40:18 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: xqueue.c /main/8 1996/10/19 18:08:11 kaleb $ */

#include "X.h"
#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#ifdef NEW_INPUT
#include "xqueue.h"
#endif

#ifdef XQUEUE

static xqEventQueue      *XqueQaddr;
static int xqueFd = -1;
#ifndef XQUEUE_ASYNC
static int xquePipe[2];
#endif

#ifdef XKB
#include "inputstr.h"
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBsrv.h>
extern Bool noXkbExtension;
#endif

#include "xf86Xinput.h"
#include "mipointer.h"

#ifdef NEW_INPUT
typedef struct {
	int		xquePending;	/* XXX not needed? */
	int		xqueSema;
} XqInfoRec, *XqInfoPtr;

InputInfoPtr XqMouse = NULL;
InputInfoPtr XqKeyboard = NULL;
#endif

#ifndef XQUEUE_ASYNC
/*
 * xf86XqueSignal --
 *	Trap the signal from xqueue and let it be known that events are
 *	ready for collection
 */

static void
xf86XqueSignal(int signum)
{
#ifndef NEW_INPUT
  xf86Info.mouseDev->xquePending = 1;
#endif
  /*
   * This is a hack, but it is the only reliable way I can find of letting
   * the main select() loop know that there is more input waiting.  Receiving
   * a signal will interrupt select(), but there is no way I can find of
   * dealing with events that come in between the end of processing the
   * last set and when select() gets called.
   *
   * XXX Maybe the XqBlock() handler will take care of that.  If we remove
   * the pipe, we need a Wakeup handler too (and need to put back xquePending).
   *
   * Suggestions for better ways of dealing with this without going back to
   * asynchronous event processing are welcome.
   */
  write(xquePipe[1], "X", 1);
  signal(SIGUSR2, xf86XqueSignal);
}
#endif
  

#ifndef NEW_INPUT
/*
 * xf86XqueRequest --
 *      Notice an i/o request from the xqueue.
 */

void
xf86XqueRequest()
{
  xqEvent  *XqueEvents;
  int       XqueHead;
  char      buf[100];

  if (xqueFd < 0)
    return;

  XqueEvents = XqueQaddr->xq_events;
  XqueHead = XqueQaddr->xq_head;

  while (XqueHead != XqueQaddr->xq_tail)
    {

      switch(XqueEvents[XqueHead].xq_type) {
	
      case XQ_BUTTON:
	xf86PostMseEvent(xf86Info.pMouse,
			~(XqueEvents[XqueHead].xq_code) & 0x07, 0, 0);
	break;

      case XQ_MOTION: {
	signed char dx, dy;

	dx = (signed char)XqueEvents[XqueHead].xq_x;
	dy = (signed char)XqueEvents[XqueHead].xq_y;
	xf86PostMseEvent(xf86Info.pMouse,
			~(XqueEvents[XqueHead].xq_code) & 0x07,
			(int)dx, (int)dy);
	break;
      }

      case XQ_KEY:
	xf86PostKbdEvent(XqueEvents[XqueHead].xq_code);
	break;
	
      default:
	xf86Msg(X_WARNING, "Unknown Xque Event: 0x%02x\n",
		XqueEvents[XqueHead].xq_type);
      }
      
      if ((++XqueHead) == XqueQaddr->xq_size) XqueHead = 0;
    }

  /* reenable the signal-processing */
  xf86Info.inputPending = TRUE;
#ifdef XQUEUE_ASYNC
  signal(SIGUSR2, (void (*)()) xf86XqueRequest);
#else
#if 0
  signal(SIGUSR2, (void (*)()) xf86XqueSignal);
#endif
#endif

#ifndef XQUEUE_ASYNC
  {
    int rval;

    while ((rval = read(xquePipe[0], buf, sizeof(buf))) > 0)
#ifdef DEBUG
      ErrorF("Read %d bytes from xquePipe[0]\n", rval);
#else
      ;
#endif
  }
#endif

  XqueQaddr->xq_head = XqueQaddr->xq_tail;
  xf86Info.mouseDev->xquePending = 0;
  XqueQaddr->xq_sigenable = 1; /* UNLOCK */
}



/*
 * xf86XqueEnable --
 *      Enable the handling of the Xque
 */

static int
xf86XqueEnable()
{
  static struct kd_quemode xqueMode;
  static Bool              was_here = FALSE;

  if (!was_here) {
    if ((xqueFd = open("/dev/mouse", O_RDONLY|O_NDELAY)) < 0)
      {
	if (xf86GetAllowMouseOpenFail()) {
	  xf86Msg(X_WARNING, "Cannot open /dev/mouse (%s) - Continuing...\n",
		  strerror(errno));
	  return (Success);
	} else {
	  Error ("Cannot open /dev/mouse");
	  return (!Success);
	}
      }
#ifndef XQUEUE_ASYNC
    pipe(xquePipe);
    fcntl(xquePipe[0],F_SETFL,fcntl(xquePipe[0],F_GETFL,0)|O_NDELAY);
    fcntl(xquePipe[1],F_SETFL,fcntl(xquePipe[1],F_GETFL,0)|O_NDELAY);
#endif
    was_here = TRUE;
  }

  if (xf86Info.mouseDev->xqueSema++ == 0) 
    {
#ifdef XQUEUE_ASYNC
      (void) signal(SIGUSR2, (void (*)()) xf86XqueRequest);
#else
      (void) signal(SIGUSR2, (void (*)()) xf86XqueSignal);
#endif
      xqueMode.qsize = 64;    /* max events */
      xqueMode.signo = SIGUSR2;
      ioctl(xf86Info.consoleFd, KDQUEMODE, NULL);
      
      if (ioctl(xf86Info.consoleFd, KDQUEMODE, &xqueMode) < 0) {
	Error ("Cannot set KDQUEMODE");
	/* CONSTCOND */
	return (!Success);
      }
      
      XqueQaddr = (xqEventQueue *)xqueMode.qaddr;
      XqueQaddr->xq_sigenable = 1; /* UNLOCK */
    }

  return(Success);
}



/*
 * xf86XqueDisable --
 *      disable the handling of the Xque
 */

static int
xf86XqueDisable()
{
  if (xf86Info.mouseDev->xqueSema-- == 1)
    {
      
      XqueQaddr->xq_sigenable = 0; /* LOCK */
      
      if (ioctl(xf86Info.consoleFd, KDQUEMODE, NULL) < 0) {
	Error ("Cannot unset KDQUEMODE");
	/* CONSTCOND */
	return (!Success);
      }
    }

  return(Success);
}



/*
 * xf86XqueMseProc --
 *      Handle the initialization, etc. of a mouse
 */

int
xf86XqueMseProc(DeviceIntPtr pPointer, int what)
{
  MouseDevPtr	mouse = MOUSE_DEV(pPointer);
  unchar        map[4];
  int ret;
 
  mouse->device = pPointer;

  switch (what)
    {
    case DEVICE_INIT: 
      
      pPointer->public.on = FALSE;
      
      map[1] = 1;
      map[2] = 2;
      map[3] = 3;
      InitPointerDeviceStruct((DevicePtr)pPointer, 
			      map, 
			      3, 
			      miPointerGetMotionEvents, 
			      (PtrCtrlProcPtr)xf86MseCtrl, 
			      miPointerGetMotionBufferSize());
#ifdef XINPUT
      InitValuatorAxisStruct(pPointer,
			     0,
			     0, /* min val */
			     screenInfo.screens[0]->width, /* max val */
			     1, /* resolution */
			     0, /* min_res */
			     1); /* max_res */
      InitValuatorAxisStruct(pPointer,
			     1,
			     0, /* min val */
			     screenInfo.screens[0]->height, /* max val */
			     1, /* resolution */
			     0, /* min_res */
			     1); /* max_res */
      /*
       * Initialize valuator values in synch
       * with dix/event.c DefineInitialRootWindow
       */
      *pPointer->valuator->axisVal = screenInfo.screens[0]->width / 2;
      *(pPointer->valuator->axisVal+1) = screenInfo.screens[0]->height / 2;
#endif

      break;
      
    case DEVICE_ON:
      mouse->lastButtons = 0;
      mouse->emulateState = 0;
      pPointer->public.on = TRUE;
      ret = xf86XqueEnable();
#ifndef XQUEUE_ASYNC
      if (xquePipe[0] != -1)
	AddEnabledDevice(xquePipe[0]);
#endif
      return(ret);
      
    case DEVICE_CLOSE:
    case DEVICE_OFF:
      pPointer->public.on = FALSE;
      ret = xf86XqueDisable();
#ifndef XQUEUE_ASYNC
      if (xquePipe[0] != -1)
	RemoveEnabledDevice(xquePipe[0]);
#endif
      return(ret);
    }
  
  return Success;
}
#endif /* !NEW_INPUT */



/*
 * xf86XqueKbdProc --
 *	Handle the initialization, etc. of a keyboard.
 */

int
xf86XqueKbdProc(DeviceIntPtr pKeyboard, int what)
{
  KeySymsRec  keySyms;
  CARD8       modMap[MAP_LENGTH];

  switch (what) {
      
  case DEVICE_INIT:
    
    xf86KbdGetMapping(&keySyms, modMap);
    
    /*
     * Get also the initial led settings
     */
    ioctl(xf86Info.consoleFd, KDGETLED, &xf86Info.leds);

    /*
     * Perform final initialization of the system private keyboard
     * structure and fill in various slots in the device record
     * itself which couldn't be filled in before.
     */
    pKeyboard->public.on = FALSE;

#ifdef XKB
    if (noXkbExtension) {
#endif
    InitKeyboardDeviceStruct((DevicePtr)xf86Info.pKeyboard,
			     &keySyms,
			     modMap,
			     xf86KbdBell,
			     (KbdCtrlProcPtr)xf86KbdCtrl);
#ifdef XKB
    } else {
	XkbComponentNamesRec names;
	if (XkbInitialMap) {
	    if ((xf86Info.xkbkeymap = strchr(XkbInitialMap, '/')) != NULL)
		xf86Info.xkbkeymap++;
	    else
		xf86Info.xkbkeymap = XkbInitialMap;
	}
	if (xf86Info.xkbkeymap) {
	    names.keymap = xf86Info.xkbkeymap;
	    names.keycodes = NULL;
	    names.types = NULL;
	    names.compat = NULL;
	    names.symbols = NULL;
	    names.geometry = NULL;
	} else {
	    names.keymap = NULL;
	    names.keycodes = xf86Info.xkbkeycodes;
	    names.types = xf86Info.xkbtypes;
	    names.compat = xf86Info.xkbcompat;
	    names.symbols = xf86Info.xkbsymbols;
	    names.geometry = xf86Info.xkbgeometry;
	}
	if ((xf86Info.xkbkeymap || xf86Info.xkbcomponents_specified)
	   && (xf86Info.xkbmodel == NULL || xf86Info.xkblayout == NULL)) {
		xf86Info.xkbrules = NULL;
	}
	XkbSetRulesDflts(xf86Info.xkbrules, xf86Info.xkbmodel,
			 xf86Info.xkblayout, xf86Info.xkbvariant,
			 xf86Info.xkboptions);
	XkbInitKeyboardDeviceStruct(pKeyboard, 
				    &names,
				    &keySyms, 
				    modMap, 
				    xf86KbdBell,
				    (KbdCtrlProcPtr)xf86KbdCtrl);
    }
#endif

    xf86InitKBD(TRUE);
    break;
    
  case DEVICE_ON:
    pKeyboard->public.on = TRUE;
    xf86InitKBD(FALSE);
    return(xf86XqueEnable());
    
  case DEVICE_CLOSE:
  case DEVICE_OFF:
    pKeyboard->public.on = FALSE;
    return(xf86XqueDisable());
  }
  
  return (Success);
}


/*
 * xf86XqueEvents --
 *      Get some events from our queue. Nothing to do here ...
 */

void
xf86XqueEvents()
{
}

#ifdef NEW_INPUT

static void XqDoInput(int signum);

void
XqReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    xqEvent *XqueEvents;
    int XqueHead;
    char buf[100];
    signed char dx, dy;

    if (xqueFd < 0)
	return;

    pMse = pInfo->private;

    XqueEvents = XqueQaddr->xq_events;
    XqueHead = XqueQaddr->xq_head;

    while (XqueHead != XqueQaddr->xq_tail) {
	switch (XqueEvents[XqueHead].xq_type) {
	case XQ_BUTTON:
	    pMse->PostEvent(pInfo, ~(XqueEvents[XqueHead].xq_code) & 0x07,
			    0, 0, 0);
	    break;

	case XQ_MOTION:
	    dx = (signed char)XqueEvents[XqueHead].xq_x;
	    dy = (signed char)XqueEvents[XqueHead].xq_y;
	    pMse->PostEvent(pInfo, ~(XqueEvents[XqueHead].xq_code) & 0x07,
			    (int)dx, (int)dy, 0);
	    break;

	case XQ_KEY:
	    /* XXX Need to deal with the keyboard part nicely. */
	    xf86PostKbdEvent(XqueEvents[XqueHead].xq_code);
	    break;
	default:
	    xf86Msg(X_WARNING, "Unknown Xque Event: 0x%02x\n",
		    XqueEvents[XqueHead].xq_type);
	}
      
	if ((++XqueHead) == XqueQaddr->xq_size) XqueHead = 0;
    }

    /* reenable the signal-processing */
    xf86Info.inputPending = TRUE;
#ifdef XQUEUE_ASYNC
    signal(SIGUSR2, XqDoInput);
#endif

#ifndef XQUEUE_ASYNC
    {
	int rval;

	while ((rval = read(xquePipe[0], buf, sizeof(buf))) > 0)
#ifdef DEBUG
	    ErrorF("Read %d bytes from xquePipe[0]\n", rval);
#else
	    ;
#endif
    }
#endif

    XqueQaddr->xq_head = XqueQaddr->xq_tail;
    XqueQaddr->xq_sigenable = 1; /* UNLOCK */
}

static void
XqDoInput(int signum)
{
    if (XqMouse)
	XqReadInput(XqMouse);
}

static void
XqBlock(pointer blockData, OSTimePtr pTimeout, pointer pReadmask)
{
    /*
     * On MP SVR4 boxes, a race condition exists because the XQUEUE does
     * not have anyway to lock it for exclusive access. This results in one
     * processor putting something on the queue at the same time the other
     * processor is taking it something off. The count of items in the queue
     * can get off by 1. This just goes and checks to see if an extra event
     * was put in the queue a during this period. The signal for this event
     * was ignored while processing the previous event.
     */

    XqReadInput((InputInfoPtr)blockData);
}

/*
 * XqEnable --
 *      Enable the handling of the Xque
 */

static int
XqEnable(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    XqInfoPtr pXq;
    static struct kd_quemode xqueMode;
    static Bool was_here = FALSE;

    pMse = pInfo->private;
    pXq = pMse->mousePriv;

    if (xqueFd < 0) {
	if ((xqueFd = open("/dev/mouse", O_RDONLY | O_NDELAY)) < 0) {
	    if (xf86GetAllowMouseOpenFail()) {
		xf86Msg(X_WARNING,
		    "%s: Cannot open /dev/mouse (%s) - Continuing...\n",
		    pInfo->name, strerror(errno));
		return Success;
	    } else {
		xf86Msg(X_ERROR, "%s: Cannot open /dev/mouse", pInfo->name);
		return !Success;
	    }
	}
    }
#ifndef XQUEUE_ASYNC
    if (!was_here) {
	pipe(xquePipe);
	fcntl(xquePipe[0], F_SETFL, fcntl(xquePipe[0], F_GETFL, 0) | O_NDELAY);
	fcntl(xquePipe[1], F_SETFL, fcntl(xquePipe[1], F_GETFL, 0) | O_NDELAY);
	was_here = TRUE;
    }
#endif

    if (pXq->xqueSema++ == 0) {
#ifdef XQUEUE_ASYNC
	(void) signal(SIGUSR2, XqDoInput);
#else
	(void) signal(SIGUSR2, xf86XqueSignal);
#endif
	xqueMode.qsize = 64;    /* max events */
	xqueMode.signo = SIGUSR2;
	ioctl(xf86Info.consoleFd, KDQUEMODE, NULL);

	if (ioctl(xf86Info.consoleFd, KDQUEMODE, &xqueMode) < 0) {
	    xf86Msg(X_ERROR, "%s: Cannot set KDQUEMODE", pInfo->name);
	    return !Success;
	}
	XqueQaddr = (xqEventQueue *)xqueMode.qaddr;
	XqueQaddr->xq_sigenable = 1; /* UNLOCK */
    }

    return Success;
}



/*
 * xf86XqueDisable --
 *      disable the handling of the Xque
 */

static int
XqDisable(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    XqInfoPtr pXq;

    pMse = pInfo->private;
    pXq = pMse->mousePriv;

    if (pXq->xqueSema-- == 1)
    {
	XqueQaddr->xq_sigenable = 0; /* LOCK */
      
	if (ioctl(xf86Info.consoleFd, KDQUEMODE, NULL) < 0) {
	    xf86Msg(X_ERROR, "%s: Cannot unset KDQUEMODE", pInfo->name);
	    return !Success;
    }

    if (xqueFd >= 0) {
	fclose(xqueFd);
	xqueFd = -1;
    }

    return Success;
}

/*
 * XqMouseProc --
 *      Handle the initialization, etc. of a mouse
 */

static int
XqMouseProc(DeviceIntPtr pPointer, int what)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    unchar        map[4];
    int ret;
 
    pInfo = pPointer->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device = pPointer;

    switch (what) {
    case DEVICE_INIT: 
	pPointer->public.on = FALSE;

	map[1] = 1;
	map[2] = 2;
	map[3] = 3;

	InitPointerDeviceStruct((DevicePtr)pPointer, 
				map, 
				3, 
				miPointerGetMotionEvents, 
				pMse->Ctrl,
				miPointerGetMotionBufferSize());
	/* X valuator */
	xf86InitValuatorAxisStruct(device, 0, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(device, 0);
	/* Y valuator */
	xf86InitValuatorAxisStruct(device, 1, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(device, 1);
	xf86MotionHistoryAllocate(pInfo);
	RegisterBlockAndWakeupHandlers(XqBlock, (WakeupHandlerProcPtr)NoopDDA,
					pInfo);
	break;
      
    case DEVICE_ON:
	pMse->lastButtons = 0;
	pMse->emulateState = 0;
	pPointer->public.on = TRUE;
	ret = XqEnable(pInfo);
#ifndef XQUEUE_ASYNC
	if (xquePipe[0] != -1) {
	    pInfo->fd = xquePipe[0];
	    AddEnabledDevice(xquePipe[0]);
	}
#endif
	return ret;
      
    case DEVICE_CLOSE:
    case DEVICE_OFF:
	pPointer->public.on = FALSE;
	ret = XqDisable(pInfo);
#ifndef XQUEUE_ASYNC
	if (xquePipe[0] != -1) {
	    RemoveEnabledDevice(xquePipe[0]);
	    pInfo->fd = -1;
	}
#endif
	return ret;
    }
    return Success;
}

Bool
XqueueMousePreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse;
    XqInfoPtr pXq;

    pMse->protocol = protocol;
    pXq = pMse->mousePriv = xnfcalloc(sizeof(XqInfoRec));

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, NULL, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

    /* Process common mouse options (like Emulate3Buttons, etc). */
    pMse->CommonOptions(pInfo);

    /* Setup the local procs. */
    pInfo->device_control = XqMouseProc;
#ifdef XQUEUE_ASYNC
    pInfo->read_input = NULL;
#else
    pInfo->read_input = XqReadInput();
#endif
    pInfo->fd = -1;

    XqMouse = pInfo;

    pInfo->flags |= XI86_CONFIGURED;
    return TRUE;
}
#endif

#endif /* XQUEUE */

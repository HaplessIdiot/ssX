/* $XConsortium: xf86Xinput.h /main/11 1996/10/27 11:05:29 kaleb $ */
/*
 * Copyright 1995-1999 by Frederic Lepied, France. <Lepied@XFree86.org>
 *                                                                            
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Frederic   Lepied not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Frederic  Lepied   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.                   
 *                                                                            
 * FREDERIC  LEPIED DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL FREDERIC  LEPIED BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Xinput.h,v 3.21 1999/04/04 05:47:05 dawes Exp $ */

#ifndef _xf86Xinput_h
#define _xf86Xinput_h

#ifndef NEED_EVENTS
#define NEED_EVENTS
#endif
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "XI.h"
#include "XIproto.h"
#include "XIstubs.h"

#define XI86_NO_OPEN_ON_INIT    1 /* open the device only when needed */
#define XI86_CONFIGURED         2 /* the device has been configured */
#define XI86_ALWAYS_CORE	4 /* the device always controls the pointer */
/* the device sends Xinput and core pointer events */
#define XI86_SEND_CORE_EVENTS	4
/* if the device is the core pointer or is sending core events, and
 * SEND_DRAG_EVENTS is false, and a buttons is done, then no motion events
 * (mouse drag action) are sent. This is mainly to allow a touch screen to be
 * used with netscape and other browsers which do strange things if the mouse
 * moves between button down and button up. With a touch screen, this motion
 * is common due to the user's finger moving slightly.
 */
#define XI86_SEND_DRAG_EVENTS	8

#define XI_PRIVATE(dev) \
	(((LocalDevicePtr)((dev)->public.devicePrivate))->private)

#define MOUSE_DEV(dev) (MouseDevPtr) XI_PRIVATE(dev)

#ifdef DBG
#undef DBG
#endif
#define DBG(lvl, f) {if ((lvl) <= xf86GetVerbosity()) f;}

#ifdef HAS_MOTION_HISTORY
#undef HAS_MOTION_HISTORY
#endif
#define HAS_MOTION_HISTORY(local) ((local)->dev->valuator && (local)->dev->valuator->numMotionEvents)

typedef struct _LocalDeviceRec {  
    char		*name;
    int			flags;
    
    Bool (*device_control)(DeviceIntPtr /*device*/, int /*what*/);
    void (*read_input)(struct _LocalDeviceRec* /*local*/);
    int (*control_proc)(struct _LocalDeviceRec* /*local*/, xDeviceCtl* /* control */);
    void (*close_proc)(struct _LocalDeviceRec* /*local*/);
    int (*switch_mode)(ClientPtr /*client*/, DeviceIntPtr /*dev*/, int /*mode*/);
    Bool (*conversion_proc)(struct _LocalDeviceRec* /*local*/, int /* first */,
			    int /* num */, int /* v0 */, int /* v1 */, int /* v2 */,
			    int /* v3 */, int /* v4 */, int /* v5 */, int* /* x */,
			    int* /* y */);
    
    int			fd;
    Atom		atom;
    DeviceIntPtr	dev;
    pointer		private;
    int			private_flags;
    pointer		motion_history;
    ValuatorMotionProcPtr motion_history_proc;
    unsigned int	history_size;	/* only for configuration purpose */
    unsigned int	first;
    unsigned int	last;
    int			old_x;
    int			old_y;
    char		*type_name;
    IntegerFeedbackPtr	always_core_feedback;
    Bool (*reverse_conversion_proc)(struct _LocalDeviceRec* /*local*/,
				    int /* x */, int /* y */,
				    int* /* valuators */);
    pointer		options;
    struct _LocalDeviceRec *next;
} LocalDeviceRec, *LocalDevicePtr;

typedef struct _DeviceAssocRec 
{
  char                  *config_section_name;
  LocalDevicePtr        (*device_allocate)(void);
} DeviceAssocRec, *DeviceAssocPtr;

int
xf86IsCorePointer(DeviceIntPtr /*dev*/);

int
xf86IsCoreKeyboard(DeviceIntPtr /*dev*/);

void
xf86XInputSetSendCoreEvents(LocalDevicePtr /*local*/, Bool /*always*/);
#define xf86AlwaysCore(a,b) xf86XInputSetSendCoreEvents(a,b)

void
InitExtInput(void);

Bool
xf86eqInit(DevicePtr /* pKbd */, DevicePtr /* pPtr */);

void
xf86eqEnqueue (struct _xEvent * /*event */);

void
xf86eqProcessInputEvents (void);

void
xf86eqSwitchScreen(ScreenPtr /* pScreen */, Bool /* fromDIX */);

void
xf86PostMotionEvent(DeviceIntPtr	/*device*/,
		    int			/*is_absolute*/,
		    int			/*first_valuator*/,
		    int			/*num_valuators*/,
		    ...);

void
xf86PostProximityEvent(DeviceIntPtr	/*device*/,
		       int		/*is_in*/,
		       int		/*first_valuator*/,
		       int		/*num_valuators*/,
		       ...);

void
xf86PostButtonEvent(DeviceIntPtr	/*device*/,
		    int			/*is_absolute*/,
		    int			/*button*/,
		    int			/*is_down*/,
		    int			/*first_valuator*/,
		    int			/*num_valuators*/,
		    ...);

void
xf86PostKeyEvent(DeviceIntPtr	/* device */,
		 unsigned int	/* key_code */,
		 int		/* is_down */,
		 int		/* is_absolute */,
		 int		/* first_valuator */,
		 int		/* num_valuators */,
		 ...);

void
xf86MotionHistoryAllocate(LocalDevicePtr /* local */);

int
xf86GetMotionEvents(DeviceIntPtr	/* dev */,
		    xTimecoord		*/* buff */,
		    unsigned long	/* start */,
		    unsigned long	/* stop */,
		    ScreenPtr		/* pScreen */);

void
xf86XinputFinalizeInit(DeviceIntPtr /* dev */);

Bool
xf86CheckButton(int /* button */, int /* down */);

void
xf86SwitchCoreDevice(LocalDevicePtr /* device */, DeviceIntPtr /* core */);

void
xf86AddLocalDevice( LocalDevicePtr	/* device */,
		    pointer		/* options */);

Bool
xf86RemoveLocalDevice(LocalDevicePtr	/* device */);

LocalDevicePtr
xf86FirstLocalDevice(void);

int
xf86ScaleAxis(int	/* Cx */,
	      int	/* Sxhigh */,
	      int	/* Sxlow */,
	      int	/* Rxhigh */,
	      int	/* Rxlow */);

void
xf86XInputSetScreen(LocalDevicePtr	/* local */,
		    int			/* screen_number */,
		    int			/* x */,
		    int			/* y */);

#endif /* _xf86Xinput_h */

/* Id: xf86Xinput.h,v 1.1 1995/12/20 14:01:23 lepied Exp */
/*
 * Copyright 1995 by Frederic Lepied, France. <fred@sugix.frmug.fr.net>       
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

/* $XFree86$ */

#ifndef _xf86Xinput_h
#define _xf86Xinput_h

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"

#define XI86_NO_OPEN_ON_INIT    1 /* open the device only when needed */
#define XI86_CONFIGURED         2 /* the device has been configured */

#define PRIVATE(dev) (((LocalDevicePtr)((dev)->public.devicePrivate))->private)

typedef struct _LocalDeviceRec {  
  char		*name;
  char		*config_section_name;
  int           flags;
  Bool		(*device_config)(
#if NeedNestedFunctionPrototypes
    struct _LocalDeviceRec* /*device*/,
    void* /*LexPtr*/ /*val*/
#endif
    );
  Bool		(*device_control)(
#if NeedNestedFunctionPrototypes
    DeviceIntPtr /*device*/,
    int /*what*/
#endif
    );
  void		(*read_input)(
#if NeedNestedFunctionPrototypes
    struct _LocalDeviceRec* /*local*/
#endif
    );
  int           (*control_proc)(
#if NeedNestedFunctionPrototypes
    struct _LocalDeviceRec* /*local*/
#endif
    );
  void          (*close_proc)(
#if NeedNestedFunctionPrototypes
    struct _LocalDeviceRec* /*local*/
#endif
    );
  int           (*switch_mode)(
#if NeedNestedFunctionPrototypes
    ClientPtr /*client*/,
    DeviceIntPtr /*dev*/,
    int /*mode*/
#endif
    );
  int		fd;
  Atom		atom;
  DeviceIntPtr	dev;
  pointer	private;
  int		private_flags;
} LocalDeviceRec, *LocalDevicePtr;

extern	int		DeviceButtonPress;
extern	int		DeviceButtonRelease;
extern	int		DeviceMotionNotify;
extern	int		DeviceValuator;

extern int
IsCorePointer(
#ifdef NeedFunctionPrototypes
	      DeviceIntPtr /*dev*/
#endif
);

extern int
IsCoreKeyboard(
#ifdef NeedFunctionPrototypes
	       DeviceIntPtr /*dev*/
#endif
);

void
PostMotionEvent(
#ifdef NeedVarargsPrototypes
		DeviceIntPtr	/*device*/,
		int		/*is_absolute*/,
		int		/*first_valuator*/,
		int		/*num_valuators*/,
		...
#endif		
);

void
PostProximityEvent(
#ifdef NeedVarargsPrototypes
		   DeviceIntPtr	/*device*/,
		   int		/*is_in*/,
		   int		/*first_valuator*/,
		   int		/*num_valuators*/,
		   ...
#endif
);

void
PostButtonEvent(
#ifdef NeedVarargsPrototypes
		DeviceIntPtr	/*device*/,
		int		/*button*/,
		int		/*is_down*/,
		int		/*first_valuator*/,
		int		/*num_valuators*/,
		...
#endif
);

#endif /* _xf86Xinput_h */

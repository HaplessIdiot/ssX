/* $XConsortium: xf86Xinput.h /main/3 1996/01/14 19:01:46 kaleb $ */
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

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Xinput.h,v 3.5 1996/02/18 03:42:55 dawes Exp $ */

#ifndef _xf86Xinput_h
#define _xf86Xinput_h

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "XI.h"
#include "XIproto.h"

#define XI86_NO_OPEN_ON_INIT    1 /* open the device only when needed */
#define XI86_CONFIGURED         2 /* the device has been configured */

#ifdef PRIVATE
#undef PRIVATE
#endif
#define PRIVATE(dev) (((LocalDevicePtr)((dev)->public.devicePrivate))->private)

typedef struct _LocalDeviceRec {  
  char		*name;
  int           flags;
  Bool		(*device_config)(
#if NeedNestedPrototypes
    struct _LocalDeviceRec** /*array*/,
    int /*index*/,
    int /*max*/,
    LexPtr /*val*/
#endif
    );
  Bool		(*device_control)(
#if NeedNestedPrototypes
    DeviceIntPtr /*device*/,
    int /*what*/
#endif
    );
  void		(*read_input)(
#if NeedNestedPrototypes
    struct _LocalDeviceRec* /*local*/
#endif
    );
  int           (*control_proc)(
#if NeedNestedPrototypes
    struct _LocalDeviceRec* /*local*/,
    xDeviceCtl* /* control */
#endif
    );
  void          (*close_proc)(
#if NeedNestedPrototypes
    struct _LocalDeviceRec* /*local*/
#endif
    );
  int           (*switch_mode)(
#if NeedNestedPrototypes
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

typedef struct _DeviceAssocRec 
{
  char                  *config_section_name;
  LocalDevicePtr        (*device_allocate)(
#if NeedNestedPrototypes
    void
#endif
);
} DeviceAssocRec, *DeviceAssocPtr;

extern	int		DeviceButtonPress;
extern	int		DeviceButtonRelease;
extern	int		DeviceMotionNotify;
extern	int		DeviceValuator;
extern	int		ProximityIn;
extern	int		ProximityOut;

extern int
IsCorePointer(
#if NeedFunctionPrototypes
	      DeviceIntPtr /*dev*/
#endif
);

extern int
IsCoreKeyboard(
#if NeedFunctionPrototypes
	       DeviceIntPtr /*dev*/
#endif
);

void
configExtendedInputSection(
#ifdef NeedFunctionPrototypes
		LexPtr          /* val */
#endif
);

void
AddDeviceAssoc(
#ifdef NeedFunctionPrototypes
		DeviceAssocPtr	/* assoc */
#endif
);

void
InitExtInput(
#ifdef NeedFunctionPrototypes
		void
#endif
);

void
OpenInputDevice (
#ifdef NeedFunctionPrototypes
		DeviceIntPtr    /* dev */,
		ClientPtr       /* client */,
		int *           /* status */
#endif
);

int
ChangeKeyboardDevice (
#ifdef NeedFunctionPrototypes
		DeviceIntPtr	/* old_dev */,
		DeviceIntPtr	/* new_dev */
#endif
);

int
ChangePointerDevice (
#ifdef NeedFunctionPrototypes
		DeviceIntPtr	/* old_dev */,
		DeviceIntPtr	/* new_dev */,
		unsigned char	/* x */,
		unsigned char	/* y */
#endif
);

void
CloseInputDevice (
#ifdef NeedFunctionPrototypes
		DeviceIntPtr    /* d */,
		ClientPtr       /* client */
#endif
);

void
AddOtherInputDevices (
#ifdef NeedFunctionPrototypes
		void
#endif
);

int
SetDeviceMode (
#ifdef NeedFunctionPrototypes
		ClientPtr       /* client */,
		DeviceIntPtr    /* dev */,
		int             /* mode */
#endif
);

int
SetDeviceValuators (
#ifdef NeedFunctionPrototypes
		ClientPtr       /* client */,
		DeviceIntPtr    /* dev */,
		int *           /* valuators */,
		int             /* first_valuator */,
		int             /* num_valuators */
#endif
);

int
ChangeDeviceControl (
#ifdef NeedFunctionPrototypes
		ClientPtr       /* client */,
		DeviceIntPtr    /* dev */,
		xDeviceCtl *    /* control */
#endif
);

Bool
xf86eqInit (
#ifdef NeedFunctionPrototypes
		DevicePtr       /* pKbd */,
		DevicePtr       /* pPtr */
#endif
);

void
xf86eqEnqueue (
#ifdef NeedFunctionPrototypes
		struct _xEvent * /*event */
#endif
);

void
xf86eqProcessInputEvents (
#ifdef NeedFunctionPrototypes
		void
#endif
);

void
PostMotionEvent(
#if NeedVarargsPrototypes
		DeviceIntPtr	/*device*/,
		int		/*is_absolute*/,
		int		/*first_valuator*/,
		int		/*num_valuators*/,
		...
#endif		
);

void
PostProximityEvent(
#if NeedVarargsPrototypes
		   DeviceIntPtr	/*device*/,
		   int		/*is_in*/,
		   int		/*first_valuator*/,
		   int		/*num_valuators*/,
		   ...
#endif
);

void
PostButtonEvent(
#if NeedVarargsPrototypes
		DeviceIntPtr	/*device*/,
		int		/*button*/,
		int		/*is_down*/,
		int		/*first_valuator*/,
		int		/*num_valuators*/,
		...
#endif
);

void
AddDeviceAssoc(
#if NeedFunctionPrototypes
		DeviceAssocPtr	/*assoc*/
#endif
);

#endif /* _xf86Xinput_h */

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Events.c,v 3.75 1999/06/27 09:20:16 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
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
/* $XConsortium: sol8PostKbdEvents.c /main/46 1996/10/25 11:36:30 kaleb $ */

/* [JCH-96/01/21] Extended std reverse map to four buttons. */

#include "X.h"
#include "Xproto.h"
#include "misc.h"

#include "compiler.h"

#include "Xpoll.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSlib.h"


#ifdef XINPUT
#include "XI.h"
#include "XIproto.h"
#include "xf86Xinput.h"
#else
#include "inputstr.h"
#endif

#include <sys/vuid_event.h>
#include "sol8_keynames.h"

#define KeyPressed(a) 1

#ifdef XKB
extern Bool noXkbExtension;
#endif

#define XE_POINTER  1
#define XE_KEYBOARD 2

#ifdef XTESTEXT1

#define	XTestSERVER_SIDE
#include "xtestext1.h"
extern short xtest_mousex;
extern short xtest_mousey;
extern int   on_steal_input;          
extern Bool  XTestStealKeyData();
extern void  XTestStealMotionData();

#ifdef XINPUT
#define ENQUEUE(ev, code, direction, dev_type) \
  (ev)->u.u.detail = (code); \
  (ev)->u.u.type   = (direction); \
  if (!on_steal_input ||  \
      XTestStealKeyData((ev)->u.u.detail, (ev)->u.u.type, dev_type, \
			xtest_mousex, xtest_mousey)) \
  xf86eqEnqueue((ev))
#else
#define ENQUEUE(ev, code, direction, dev_type) \
  (ev)->u.u.detail = (code); \
  (ev)->u.u.type   = (direction); \
  if (!on_steal_input ||  \
      XTestStealKeyData((ev)->u.u.detail, (ev)->u.u.type, dev_type, \
			xtest_mousex, xtest_mousey)) \
  mieqEnqueue((ev))
#endif

#define MOVEPOINTER(dx, dy, time) \
  if (on_steal_input) \
    XTestStealMotionData(dx, dy, XE_POINTER, xtest_mousex, xtest_mousey); \
  miPointerDeltaCursor (dx, dy, time)

#else /* ! XTESTEXT1 */

#ifdef XINPUT
#define ENQUEUE(ev, code, direction, dev_type) \
  (ev)->u.u.detail = (code); \
  (ev)->u.u.type   = (direction); \
  xf86eqEnqueue((ev))
#else
#define ENQUEUE(ev, code, direction, dev_type) \
  (ev)->u.u.detail = (code); \
  (ev)->u.u.type   = (direction); \
  mieqEnqueue((ev))
#endif
#define MOVEPOINTER(dx, dy, time) \
  miPointerDeltaCursor (dx, dy, time)

#endif


/*
 * sol8PostKbdEvent --
 *	Translate the raw hardware Firm_event into an XEvent, and tell DIX
 *	about it. KeyCode preprocessing and so on is done ...
 * 
 * Most of the Solaris 8 stuff has whacked Panix/PC98 support in the
 * interests of simplicity - DWH 8/30/99
 *
 */

void sol8PostKbdEvent(Firm_event	*event)
{
  int         specialkey;
  Bool        down;
  KeyClassRec *keyc = ((DeviceIntPtr)xf86Info.pKeyboard)->key;
  Bool        updateLeds = FALSE;
  Bool        UsePrefix = FALSE;
  Bool        Direction = FALSE;
  xEvent      kevent;
  KeySym      *keysym;
  int         keycode;
  static int  lockkeys = 0;

/* 
 * Give down a value 
 */

 if(event->value == VKEY_DOWN) 
	down = TRUE;
 else 
	down = FALSE;

  /*
   * and now get some special keysequences
   */

  specialkey = event->id;

  if ((ModifierDown(ControlMask | AltMask)) ||
      (ModifierDown(ControlMask | AltLangMask)))
    {
      
      switch (specialkey) {
	
      case KEY_BackSpace:
	if (!xf86Info.dontZap) {
#ifdef XFreeXDGA
	 DGAShutdown();
#endif
	 GiveUp(0);
        }
	break;
	
	/*
	 * The idea here is to pass the scancode down to a list of
	 * registered routines. There should be some standard conventions
	 * for processing certain keys.
	 */
      case KEY_KP_Minus:   /* Keypad - */
	if (!xf86Info.dontZoom) {
	  if (down) xf86ZoomViewport(xf86Info.currentScreen, -1);
	  return;
	}
	break;
	
      case KEY_KP_Plus:   /* Keypad + */
	if (!xf86Info.dontZoom) {
	  if (down) xf86ZoomViewport(xf86Info.currentScreen,  1);
	  return;
	}
	break;
      } 
    }


  /*
   * Now map the scancodes to real X-keycodes ...
   */
  keycode = event->id + MIN_KEYCODE;
  keysym = (keyc->curKeySyms.map +
		keyc->curKeySyms.mapWidth *
		(keycode - keyc->curKeySyms.minKeyCode));

#ifdef XKB
  if (noXkbExtension) {
#endif
  /*
   * Filter autorepeated caps/num/scroll lock keycodes.
   */
#define CAPSFLAG 0x01
#define NUMFLAG 0x02
#define SCROLLFLAG 0x04
#define MODEFLAG 0x08
  if( down ) {
    switch( keysym[0] ) {
        case XK_Caps_Lock :
          if (lockkeys & CAPSFLAG)
              return;
	  else
	      lockkeys |= CAPSFLAG;
          break;

        case XK_Num_Lock :
          if (lockkeys & NUMFLAG)
              return;
	  else
	      lockkeys |= NUMFLAG;
          break;

        case XK_Scroll_Lock :
          if (lockkeys & SCROLLFLAG)
              return;
	  else
	      lockkeys |= SCROLLFLAG;
          break;
    }
  }
  else {
    switch( keysym[0] ) {
        case XK_Caps_Lock :
            lockkeys &= ~CAPSFLAG;
            break;

        case XK_Num_Lock :
            lockkeys &= ~NUMFLAG;
            break;

        case XK_Scroll_Lock :
            lockkeys &= ~SCROLLFLAG;
            break;
    }
  }

  /*
   * LockKey special handling:
   * ignore releases, toggle on & off on presses.
   * Don't deal with the Caps_Lock keysym directly, but check the lock modifier
   */

  if (keyc->modifierMap[keycode] & LockMask ||
      keysym[0] == XK_Scroll_Lock ||
      keysym[0] == XK_Num_Lock)
    {
      Bool flag;

      if (!down) return;
      if (KeyPressed(keycode)) {
	down = !down;
	flag = FALSE;
      }
      else
	flag = TRUE;

      if (keyc->modifierMap[keycode] & LockMask)   xf86Info.capsLock   = flag;
      if (keysym[0] == XK_Num_Lock)    xf86Info.numLock    = flag;
      if (keysym[0] == XK_Scroll_Lock) xf86Info.scrollLock = flag;
      updateLeds = TRUE;
    }


  if (updateLeds) xf86KbdLeds();
#ifdef XKB
  }
#endif

  /*
   * check for an autorepeat-event
   */
  if ((down && KeyPressed(keycode)) &&
      (xf86Info.autoRepeat != AutoRepeatModeOn || keyc->modifierMap[keycode]))
    return;

  xf86Info.lastEventTime = kevent.u.keyButtonPointer.time = GetTimeInMillis();

  /*
   * And now send these prefixes ...
   * NOTE: There cannot be multiple Mode_Switch keys !!!!
   */

   ENQUEUE(&kevent, keycode, (down ? KeyPress : KeyRelease), XE_KEYBOARD);
}

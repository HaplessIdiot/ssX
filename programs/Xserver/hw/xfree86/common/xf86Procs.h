/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Procs.h,v 3.10.4.7 1998/07/18 17:53:27 dawes Exp $ */
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
/* $XConsortium: xf86Procs.h /main/9 1996/10/19 17:59:14 kaleb $ */

#ifndef _XF86PROCS_H
#define _XF86PROCS_H

#if 0
#include <X11/Xfuncproto.h>
#include "xf86.h"
#include "xf86Priv.h"

_XFUNCPROTOBEGIN

/* xf86Config.c */
extern void xf86CheckBeta(int extraDays, char *key);

/* xf86Events.c */
extern void ProcessInputEvents(void);
extern void xf86PostKbdEvent(unsigned key);
extern void xf86PostMseEvent(DeviceIntPtr device, int buttons, int dx, int dy);
extern void xf86Block(pointer blockData, OSTimePtr pTimeout,
			pointer pReadmask);
extern void xf86Wakeup(pointer blockData, int err, pointer pReadmask);
extern void xf86VTRequest(int signo);
extern void xf86SigHandler(int signo);

/* xf86Io.c */
extern void xf86KbdLeds(void);
extern void xf86InitKBD(Bool init);
#ifdef NEED_EVENTS
extern void xf86KbdBell(int loudness, DeviceIntPtr pKeyboard, pointer ctrl,
			int);
#endif
extern void xf86KbdCtrl(DevicePtr pKeyboard, KeybdCtrl *ctrl);
extern int  xf86KbdProc(DeviceIntPtr pKeyboard, int what);
extern void xf86KbdEvents(void);
extern void xf86MseCtrl(DevicePtr pPointer, PtrCtrl *ctrl);
extern int  xf86MseProc(DeviceIntPtr pPointer, int what);
extern int  xf86MseProcAux(DeviceIntPtr pPointer, int what, MouseDevPtr mouse,
				int* fd, PtrCtrlProcPtr ctrl);
extern void xf86MseEvents(MouseDevPtr mouse);

/* xf86_Mouse.c */
extern Bool xf86MouseSupported(int mousetype);
extern void xf86SetupMouse(MouseDevPtr);
extern void xf86MouseProtocol(DeviceIntPtr device, unsigned char *rBuf,
				int nBytes);

/* xf86Kbd.c */
extern void xf86KbdGetMapping(KeySymsRec *pKeySyms, CARD8 *pModMap);

/* xf86XKB.c */
extern void xf86InitXkb(void);
#endif

_XFUNCPROTOEND

#endif /* _XF86PROCS_H */


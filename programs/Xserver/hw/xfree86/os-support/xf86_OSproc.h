/*
 * Copyright 1990, 1991 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1992 by David Dawes <dawes@XFree86.org>
 * Copyright 1992 by Jim Tsillas <jtsilla@damon.ccs.northeastern.edu>
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1992 by Robert Baron <Robert.Baron@ernst.mach.cs.cmu.edu>
 * Copyright 1992 by Orest Zborowski <obz@eskimo.com>
 * Copyright 1993 by Vrije Universiteit, The Netherlands
 * Copyright 1993 by David Wexelblat <dwex@XFree86.org>
 * Copyright 1994, 1996 by Holger Veit <Holger.Veit@gmd.de>
 * Copyright 1994-1998 by The XFree86 Project, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holders 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holders make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * The ARM32 code here carries the following copyright:
 *
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in 
 * accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * this software in source and/or binary form. No title or ownership is
 * transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and retain
 *    this copyright notice and list of conditions as they appear in the
 *    source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital 
 *    Equipment Corporation. Neither the "Digital Equipment Corporation"
 *    name nor any trademark or logo of Digital Equipment Corporation may be
 *    used to endorse or promote products derived from this software without
 *    the prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties,
 *    including but not limited to, any implied warranties of merchantability,
 *    fitness for a particular purpose, or non-infringement are disclaimed.
 *    In no event shall DIGITAL be liable for any damages whatsoever, and in
 *    particular, DIGITAL shall not be liable for special, indirect,
 *    consequential, or incidental damages or damages for lost profits, loss
 *    of revenue or loss of use, whether such damages arise in contract, 
 *    negligence, tort, under statute, in equity, at law or otherwise, even
 *    if advised of the possibility of such damage. 
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86_OSproc.h,v 3.18 1998/07/26 09:56:18 dawes Exp $ */

#ifndef _XF86_OSPROC_H
#define _XF86_OSPROC_H

/*
 * The actual prototypes have been pulled into this seperate file so
 * that they can can be used without pulling in all of the OS specific
 * stuff like sys/stat.h, etc. This casues problem for loadable modules.
 */ 

/*
 * Flags for xf86MapVidMem().  Multiple flags can be or'd together.  The
 * flags may be used as hints.  For example it would be permissible to
 * enable write combining for memory marked only for framebuffer use.
 */

typedef enum {
    VIDMEM_FRAMEBUFFER = 0x01,	/* memory for framebuffer use */
    VIDMEM_MMIO        = 0x02	/* memory for I/O use */
} VidMemFlags;

typedef unsigned long xf86size_t;	/* added for Xinput	(Metrolink merge) */
typedef signed long xf86ssize_t;	/* added for Xinput	(Metrolink merge) */

#ifdef XF86_OS_PRIVS
extern void xf86WrapperInit(void);
#endif

#ifndef NO_OSLIB_PROTOTYPES
/*
 * This is to prevent re-entrancy to FatalError() when aborting.
 * Anything that can be called as a result of AbortDDX() should use this
 * instead of FatalError(). (xf86Exiting gets set to TRUE the first time
 * AbortDDX() is called.)
 */

#define xf86FatalError(a, b) \
	if (xf86Exiting) { \
		ErrorF(a, b); \
		return; \
	} else FatalError(a, b)

/***************************************************************************/
/* Prototypes                                                              */
/***************************************************************************/

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

/* public functions */
extern Bool xf86LinearVidMem(void);
extern pointer xf86MapVidMem(int, int, pointer, unsigned long);
extern void xf86UnMapVidMem(int, pointer, unsigned long);
extern int xf86ReadBIOS(unsigned long, unsigned long, unsigned char *, int);
extern void xf86EnableIO(void);
extern void xf86DisableIO(void);
extern Bool xf86DisableInterrupts(void);
extern void xf86EnableInterrupts(void);
extern void xf86SetTVOut(int);
extern void xf86SetRGBOut(void);
extern void xf86BusToMem(unsigned char *, unsigned char *, int);
extern void xf86MemToBus(unsigned char *, unsigned char *, int);
extern void xf86IODelay(void);
extern void xf86SlowBcopy(unsigned char *, unsigned char *, int);
extern int xf86OpenSerial(pointer options);
extern int xf86SetSerial(int fd, pointer options);
extern int xf86ReadSerial(int fd, void *buf, int count);
extern int xf86WriteSerial(int fd, void *buf, int count);
extern int xf86CloseSerial(int fd);
extern int xf86FlushInput(int fd);
/* Merged from Metrolink tree for Xinput */
extern int xf86WaitForInput( int, int );
extern int xf86SerialSendBreak( int, int );
/* End merged section	*/


#if defined(__alpha__)
/* entry points for SPARSE memory access routines */
extern pointer xf86MapVidMemSparse(int, int, pointer, unsigned long);
extern void xf86UnMapVidMemSparse(int, pointer, unsigned long);
extern int xf86ReadSparse8(pointer, unsigned long);
extern int xf86ReadSparse16(pointer, unsigned long);
extern int xf86ReadSparse32(pointer, unsigned long);
extern void xf86WriteSparse8(int, pointer, unsigned long);
extern void xf86WriteSparse16(int, pointer, unsigned long);
extern void xf86WriteSparse32(int, pointer, unsigned long);
extern void xf86JensenMemToBus(char *, long, long, int);
extern void xf86JensenBusToMem(char *, char *, unsigned long, int);
extern void xf86SlowBCopyFromBus(unsigned char *, unsigned char *, int);
extern void xf86SlowBCopyToBus(unsigned char *, unsigned char *, int);
#endif /* __alpha__ */


#ifdef XF86_OS_PRIVS
extern void xf86OpenConsole(void);
extern void xf86CloseConsole(void);
extern Bool xf86VTSwitchPending(void);
extern Bool xf86VTSwitchAway(void);
extern Bool xf86VTSwitchTo(void);
extern void xf86VTRequest(int sig);
extern int xf86ProcessArgument(int, char **, int);
extern void xf86UseMsg(void);
extern void xf86SoundKbdBell(int, int, int);
extern void xf86SetKbdLeds(int);
extern int xf86GetKbdLeds(void);
extern void xf86SetKbdRepeat(char);
extern void xf86KbdInit(void);
extern int xf86KbdOn(void);
extern int xf86KbdOff(void);
extern void xf86KbdEvents(void);
extern void xf86SetMouseSpeed(MouseDevPtr, int, int, unsigned);
extern void xf86MouseInit(MouseDevPtr);
extern int xf86MouseOn(MouseDevPtr);
extern int xf86MouseOff(MouseDevPtr, Bool);
extern void xf86MouseEvents(MouseDevPtr);
extern int  xf86XqueKbdProc(DeviceIntPtr, int);
extern int  xf86XqueMseProc(DeviceIntPtr, int);
extern void xf86XqueEvents(void);
extern int  xf86OsMouseProc(DeviceIntPtr, int);
extern void xf86OsMouseEvents(void);
extern void xf86OsMouseOption(int, pointer);

#endif /* XF86_OS_PRIVS */


_XFUNCPROTOEND
#endif /* NO_OSLIB_PROTOTYPES */

#endif /* _XF86_OSPROC_H */

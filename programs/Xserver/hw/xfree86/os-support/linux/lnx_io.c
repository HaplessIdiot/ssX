/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_io.c,v 3.8 1999/07/04 06:39:15 dawes Exp $ */
/*
 * Copyright 1992 by Orest Zborowski <obz@Kodak.com>
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Orest Zborowski and David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Orest Zborowski
 * and David Dawes make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * OREST ZBOROWSKI AND DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL OREST ZBOROWSKI OR DAVID DAWES BE LIABLE 
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: lnx_io.c /main/8 1996/10/19 18:06:28 kaleb $ */

#define NEED_EVENTS
#include "X.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

void
xf86SoundKbdBell(int loudness, int pitch, int duration)
{
	if (loudness && pitch)
	{
		ioctl(xf86Info.consoleFd, KDMKTONE,
		      ((1193190 / pitch) & 0xffff) |
		      (((unsigned long)duration *
			loudness / 50) << 16));
	}
}

void
xf86SetKbdLeds(int leds)
{
	ioctl(xf86Info.consoleFd, KDSETLED, leds);
}

int
xf86GetKbdLeds()
{
	int leds;

	ioctl(xf86Info.consoleFd, KDGETLED, &leds);
	return(leds);
}

void
xf86SetKbdRepeat(char rad)
{
	return;
}

static int kbdtrans;
static struct termios kbdtty;

void
xf86KbdInit()
{
	ioctl (xf86Info.consoleFd, KDGKBMODE, &kbdtrans);
	tcgetattr (xf86Info.consoleFd, &kbdtty);
}

int
xf86KbdOn()
{
	struct termios nTty;

#ifdef __powerpc__
	ioctl(xf86Info.consoleFd, KDSKBMODE, K_MEDIUMRAW);
#else
	ioctl(xf86Info.consoleFd, KDSKBMODE, K_RAW);
#endif
	nTty = kbdtty;
	nTty.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
	nTty.c_oflag = 0;
	nTty.c_cflag = CREAD | CS8;
	nTty.c_lflag = 0;
	nTty.c_cc[VTIME]=0;
	nTty.c_cc[VMIN]=1;
	cfsetispeed(&nTty, 9600);
	cfsetospeed(&nTty, 9600);
	tcsetattr(xf86Info.consoleFd, TCSANOW, &nTty);
	return(xf86Info.consoleFd);
}

int
xf86KbdOff()
{
	ioctl(xf86Info.consoleFd, KDSKBMODE, kbdtrans);
	tcsetattr(xf86Info.consoleFd, TCSANOW, &kbdtty);
	return(xf86Info.consoleFd);
}


/* $XConsortium: sysv_tty.c,v 1.1 94/03/28 21:32:14 dpw Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Thomas Roell and David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission. Thomas Roell and
 * David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THOMAS ROELL AND DAVID DAWES DISCLAIM ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL THOMAS ROELL OR DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */



#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "compiler.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"

static Bool not_a_tty = FALSE;

void xf86SetMouseSpeed(old, new, cflag)
int old;
int new;
unsigned cflag;
{
	struct termio tty;
	char *c;

	if (not_a_tty)
		return;

	if (ioctl(xf86Info.mseFd, TCGETA, &tty) < 0)
	{
		not_a_tty = TRUE;
		ErrorF("Warning: unable to get status of mouse fd (%s)\n",
		       strerror(errno));
		return;
	}

	tty.c_iflag = IGNBRK | IGNPAR;
	tty.c_oflag = 0;
	tty.c_lflag = 0;
	tty.c_cflag = cflag;
	tty.c_line = 0;
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 1;

	switch (old)
	{
	case 9600:
		tty.c_cflag |= B9600;
		break;
	case 4800:
		tty.c_cflag |= B4800;
		break;
	case 2400:
		tty.c_cflag |= B2400;
		break;
	case 1200:
	default:
		tty.c_cflag |= B1200;
	}

	if (ioctl(xf86Info.mseFd, TCSETAW, &tty) < 0)
	{
		FatalError("Unable to set status of mouse fd (%s)\n",
			   strerror(errno));
	}

	switch (new)
	{
	case 9600:
		c = "*q";
		tty.c_cflag |= B9600;
		break;
	case 4800:
		c = "*p";
		tty.c_cflag |= B4800;
		break;
	case 2400:
		c = "*o";
		tty.c_cflag |= B2400;
		break;
	case 1200:
	default:
		c = "*n";
		tty.c_cflag |= B1200;
	}

	if (write(xf86Info.mseFd, c, 2) != 2)
	{
		FatalError("Unable to write to mouse fd (%s)\n",
			   strerror(errno));
	}
	usleep(100000);

	if (ioctl(xf86Info.mseFd, TCSETAW, &tty) < 0)
	{
		FatalError("Unable to set status of mouse fd (%s)\n",
			   strerror(errno));
	}
#ifdef TCMOUSE
	ioctl(xf86Info.mseFd, TCMOUSE, 1);
#endif
}


/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/std_mouse.c,v 3.6.4.5 1998/06/05 16:23:23 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Thomas Roell and David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Thomas Roell and
 * David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THOMAS ROELL AND DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL THOMAS ROELL OR DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: std_mouse.c /main/5 1996/03/11 10:47:40 kaleb $ */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

int
xf86MouseOff(MouseDevPtr mouse, Bool doclose)
{
	int oldfd;

	if ((oldfd = mouse->mseFd) >= 0)
	{
		if (mouse->mseType == PROT_LOGI)
		{
			write(mouse->mseFd, "U", 1);
		}
		if (mouse->oldBaudRate > 0) {
		    xf86SetMouseSpeed(mouse,
				      mouse->baudRate,
				      mouse->oldBaudRate,
				      xf86MouseCflags[mouse->mseType]);
		}
		close(mouse->mseFd);
		oldfd = mouse->mseFd;
		mouse->mseFd = -1;
	}
	return(oldfd);
}

/* $XConsortium: mnx_init.c,v 1.1 94/03/28 21:29:45 dpw Exp $ */
/*
 * Copyright 1993 by Vrije Universiteit, The Netherlands
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of The Vrije Universiteit and David 
 * Wexelblat not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission.
 * The Vrije Universiteit and David Wexelblat make no representations about 
 * the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE VRIJE UNIVERSITEIT AND DAVID WEXELBLAT DISCLAIM ALL WARRANTIES WITH 
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THE VRIJE UNIVERSITEIT OR
 * DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR 
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"

#define VIDEO_SIZE	0x20000
#define VIDEO_ALIGN	 0x1000

void xf86OpenConsole()
{
    int fd, r, align_diff;
    char *video_base_raw, *video_base;
    struct mio_map mio_map;

    if (serverGeneration == 1)
    {
	/* check if we're run with euid==0 */
	if (geteuid() != 0)
	{
	    FatalError("xf86OpenConsole: Server must be suid root\n");
	}

	xf86Config(FALSE); /* Read Xconfig */

	fd = xf86Info.screenFd = open("/dev/vga", O_RDWR);
	if (fd == -1)
	{
	    FatalError("xf86OpenConsole: Can't open /dev/vga: %s\n", 
		       strerror(errno));
	}
	video_base_raw = xf86Info.screenPtrRaw = Xalloc(VIDEO_SIZE+VIDEO_ALIGN);
	if (video_base_raw == 0)
	{
	    FatalError("xf86OpenConsole: Out of memory\n");
	}
	align_diff = (int)video_base_raw;
	align_diff = VIDEO_ALIGN-(((align_diff-1) & (VIDEO_ALIGN-1))+1);
	assert(align_diff >= 0 && align_diff < VIDEO_ALIGN);
	xf86Info.screenPtr = video_base = video_base_raw + align_diff;
	mio_map.mm_base = (u32_t)video_base;
	mio_map.mm_size = VIDEO_SIZE;
	r = ioctl(fd, MIOCMAP, &mio_map);
	if (r == -1)
	{
	    FatalError("xf86OpenConsole: MIOCMAP failed: %s\n", 
		       strerror(errno));
	}
    }
    return;
}

void xf86CloseConsole()
{
	return;
}

/* ARGSUSED */
int xf86ProcessArgument (argc, argv, i)
int argc;
char *argv[];
int i;
{
	return(0);
}

void xf86UseMsg()
{
	return;
}

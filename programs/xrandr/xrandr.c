/*
 * $XFree86$
 *
 * Copyright © 2001 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

int
main (int argc, char **argv)
{
    Display *dpy;
    Time    t, nt;
    XRRScreenSize   *sizes;
    int		    nsize;
    Window	    root;
    int		    n;

    dpy = XOpenDisplay (0);
    if (!dpy)
    {
	fprintf (stderr, "Can't open display\n");
	exit (1);
    }
    root = DefaultRootWindow (dpy);
    for (;;)
    {
	t = XRRGetScreenInfo (dpy, root, &sizes, &nsize);
	if (t == CurrentTime)
	{
	    fprintf (stderr, "XRRGetScreenInfo failed\n");
	    exit (1);
	}
	for (n = 0; n < nsize; n++)
	{
	    printf ("%d: %dx%d (%dmmx%dmm)\n",
		    n, sizes[n].width, sizes[n].height,
		    sizes[n].mwidth, sizes[n].mheight);
	}
	printf ("Select one -> ");
	scanf ("%d", &n);
	if (0 <= n && n < nsize)
	{
	    nt = XRRSetScreenConfig (dpy, DefaultRootWindow (dpy), n, 0, 0, t);
	    if (nt == t)
		break;
	}
    }
    return 0;
}   

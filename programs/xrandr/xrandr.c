/*
 * $XFree86: xc/programs/xrandr/xrandr.c,v 1.3 2001/06/03 21:52:47 keithp Exp $
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
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>

int
main (int argc, char **argv)
{
    Display *dpy;
    XRRScreenSize   *sizes;
    XRRScreenConfiguration *sc;
    int		    nsize;
    Window	    root;
    int		    n;
    Status	    status;
    int		    rot;
    Rotation	    rotation, current_rotation, rotations;
    XRRScreenChangeNotifyEvent event;

    dpy = XOpenDisplay (NULL);
    if (dpy == NULL)
    {
	fprintf (stderr, "Can't open display\n");
	exit (1);
    }
    root = DefaultRootWindow (dpy);
    for (;;)    {
	sc = XRRGetScreenInfo (dpy, root);
	sizes = XRRSizes(sc, &nsize);
	for (n = 0; n < nsize; n++)
	{
	    printf ("%d: %dx%d (%dmmx%dmm)\n",
		    n, sizes[n].width, sizes[n].height,
		    sizes[n].mwidth, sizes[n].mheight);
	}
	rotations = XRRRotations(sc, &current_rotation);
	printf ("Rotations Possible %d, current %d\n", 
		rotations, current_rotation);
	printf ("Select one -> ");
	scanf ("%d", &n);
	printf ("Rotation -> ");
	scanf ("%d", &rot);
	rotation = rot;
	
	XRRScreenChangeSelectInput (dpy, root, True);
        do {
	  printf("calling XRRSetScreenConfig\n");
	  status = XRRSetScreenConfig (dpy, sc, DefaultRootWindow (dpy), 
			       (SizeID) n, 0, rotation, CurrentTime);
	    if (!status) {
	      fprintf(stderr, "status = %d\n", status);
	    }
	    
	}   while (status == False);
	printf("operation succeeded!\n");
	XNextEvent(dpy, (XEvent *) &event);
	printf("Got and event!\n");
	printf("timestamp = %ld, config_timestamp = %ld\n",
	       event.timestamp, event.config_timestamp);
	printf("root = %ld, size_index = %d, rotation, %d\n", 
		root, event.size_index, event.rotation);
	printf("%dX%d pixels, %dX%d mm, group = %d\n",
	       event.new.width, event.new.height,
	       event.new.mwidth, event.new.mheight,
	       event.new.group);
	XRRFreeScreenInfo(sc);
    }
}


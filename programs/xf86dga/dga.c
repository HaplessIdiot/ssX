/* $XFree86: xc/programs/xf86dga/dga.c,v 3.2 1995/12/16 08:22:37 dawes Exp $ */

#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xmu/StdSel.h>
#include <X11/Xmd.h>
#include <X11/extensions/xf86dga.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>



#define MINMAJOR 0
#define MINMINOR 0


main(int argc, char *argv[])
{
    int MajorVersion, MinorVersion;
    int EventBase, ErrorBase;
    Display *dis;
    int i;
    char *addr;
    int width, bank, banks, ram;
    XEvent event;
    Colormap cmap;
    Visual *vis;
    Window root;
    XSetWindowAttributes xswa;

    if ( (dis = XOpenDisplay(NULL)) == NULL )
     {
        (void) fprintf( stderr, " cannot connect to X server %s\n",
                       XDisplayName(NULL));
        exit( -1 );
     }


    if (!XF86DGAQueryVersion(dis, &MajorVersion, &MinorVersion))
 { 
        fprintf(stderr, "Unable to query video extension version\n");
        return 2;
 }

    if (!XF86DGAQueryExtension(dis, &EventBase, &ErrorBase)) {
        fprintf(stderr, "Unable to query video extension information\n");
        return 2;
    }

    /* Fail if the extension version in the server is too old */
    if (MajorVersion < MINMAJOR ||
        (MajorVersion == MINMAJOR && MinorVersion < MINMINOR)) {
        fprintf(stderr,
                "Xserver is running an old XFree86-DGA version"
                " (%d.%d)\n", MajorVersion, MinorVersion);
        fprintf(stderr, "Minimum required version is %d.%d\n",
                MINMAJOR, MINMINOR);
        exit(2);
    }
   /* override redirect tells the window manger to just ignore us :-) */
   xswa.override_redirect = True;

   xswa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                         PointerMotionMask | ButtonPressMask |
                         ButtonReleaseMask;

   /* we want to get everything including colormap */
   root = XCreateWindow(dis, DefaultRootWindow(dis), 0, 0,
			WidthOfScreen(ScreenOfDisplay(dis, 0)),
		        HeightOfScreen(ScreenOfDisplay(dis, 0)), 0,
			CopyFromParent, InputOutput, CopyFromParent,
			CWEventMask|CWOverrideRedirect, &xswa);


   XMapWindow(dis, root);
   XRaiseWindow(dis, root);
   /* We want all the key presses */

   XGrabKeyboard(dis, root, True, GrabModeAsync, 
		 GrabModeAsync,	 CurrentTime);

   /* and all the mouse moves */
   XGrabPointer(dis, root, True, PointerMotionMask |
		ButtonPressMask | ButtonReleaseMask,
   GrabModeAsync, GrabModeAsync, None,  None, CurrentTime);
   /* we want _our_ cmap */
   vis = DefaultVisual(dis, DefaultScreen(dis));
   cmap = XCreateColormap(dis, root, vis,
		   AllocAll);


   for (i = 0; i < 256; i++) {
       XColor xcol;

       xcol.pixel = i;
       xcol.red = (256 - i) * 255;
       xcol.green = i * 255;
       xcol.blue = ((128 - i)%256) * 255;
       xcol.flags = DoBlue | DoGreen | DoRed;
       XStoreColor(dis, cmap, &xcol);
   }
   XInstallColormap(dis, cmap);
   XSetWindowColormap(dis, root, cmap);
   /*
    * Lets go live
    */

   XF86DGAGetVideo(dis, DefaultScreen(dis), &addr, &width, &bank, &ram);
   fprintf(stderr, "%x addr:%X, width %d, bank size %d\n", True,
	   addr, width, bank);

   XF86DGADirectVideo(dis, DefaultScreen(dis),
			   XF86DGADirectGraphics|
			   XF86DGADirectMouse|
			   XF86DGADirectKeyb);


   XF86DGASetViewPort(dis, DefaultScreen(dis), 0, 0);

   banks = (ram * 1024)/bank;
#ifdef DEBUG
   fprintf(stderr, "%x ram:%x, addr %x, banks %d\n", True,
	   ram, addr, banks);
#endif
   while (1) {
      XMotionEvent *mevent = (XMotionEvent *) &event;
      XButtonEvent *bevent = (XButtonEvent *) &event;
      int n_chars = 0;
      char buf[21];
      KeySym ks = 0;
      
      XNextEvent(dis, &event);
      switch (event.type) {
       case KeyPress:
	 n_chars = XLookupString(&event.xkey, buf, 20, &ks, NULL);
         buf[n_chars] = '\0';
	 fprintf(stderr,"KeyPress [%d]: %s\n", event.xkey.keycode, buf);

         for (i = 0; i < banks; i++) {
		XF86DGASetVidPage(dis, DefaultScreen(dis), i);
		memset(addr, buf[0], bank);
#ifdef DEBUG
   fprintf(stderr, "XF86DGASetVidPage(dis, DefaultScreen(dis), %d);\n",i);
   fprintf(stderr, "memset(addr:%x, buf[0]:%d, bank:%d);\n",addr,buf[0],bank);
#endif
	 }
	 break;
       case KeyRelease:
	 n_chars = XLookupString(&event.xkey, buf, 20, &ks, NULL);
         buf[n_chars] = '\0';
	 fprintf(stderr,"KeyRelease[%d]: %s\n", event.xkey.keycode, buf);
	 break;
       case MotionNotify:
 	 fprintf(stderr,"Relative Motion: %d %d\n", mevent->x_root, mevent->y_root);
         break;
       case ButtonPress:
	 fprintf(stderr,"Button %d pressed\n", bevent->button);
	 break;
       case ButtonRelease:
	 fprintf(stderr,"Button %d released\n", bevent->button);
	 break;
       case Expose:
	 /* maybe we should RaiseWindow? */
	 break;
       default:
	 fprintf(stderr,"%X\n", event.type);
	 break;
      }
      if (n_chars && (buf[0] == 'q' || buf[0] == 'Q')) {
	 fprintf(stderr,"EXITTING\n");
         break;
      }
   }
   /*
    * back to the X server
    */
   XF86DGADirectVideo(dis, DefaultScreen(dis), 0);
   fprintf(stderr, "back now in X\n");

   /* and give back control */
   XUngrabPointer(dis, CurrentTime);
   XUngrabKeyboard(dis, CurrentTime);
   fprintf(stderr, "Thats all folks\n");
}



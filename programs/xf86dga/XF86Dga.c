/* $XFree86: xc/programs/xf86dga/XF86Dga.c,v 3.2 1995/12/16 08:22:36 dawes Exp $ */

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
#include <stdlib.h>
#include <stdio.h>
#include <sys/fcntl.h>
#if defined(ISC) 
# define HAS_SVR3_MMAP
# include <sys/types.h>
# include <errno.h>

# include <sys/at_ansi.h>
# include <sys/kd.h>

# include <sys/sysmacros.h>
# include <sys/immu.h>
# include <sys/region.h>

# include <sys/mmap.h>
#else
#include <sys/mman.h>
#endif
#include <sys/wait.h>
#include <signal.h>
extern int errno;

#if defined(ISC) && defined(HAS_SVR3_MMAP)
struct kd_memloc XFree86mloc;
#endif
static char * _XFree86addr = NULL;
static int    _XFree86size = 0;

XF86DGADirectVideo(dis, screen, enable)
Display *dis;
int screen;
int enable;
{
   if (enable&XF86DGADirectGraphics) {
	 fprintf(stderr, "video memory unprotecting\n");
#if !defined(ISC) && !defined(HAS_SVR3_MMAP)
      if (_XFree86addr && _XFree86size)
         if (mprotect(_XFree86addr,_XFree86size, PROT_READ|PROT_WRITE)) {
         fprintf(stderr, "XF86DGADirectVideo: mprotect (%s)\n",
                           strerror(errno));
		exit (-3);
	 }
#endif
   } else {
      if (_XFree86addr && _XFree86size)
	 fprintf(stderr, "video memory protecting\n");
#if !defined(ISC) && !defined(HAS_SVR3_MMAP)
         if (mprotect(_XFree86addr,_XFree86size, PROT_READ)) {
         fprintf(stderr, "XF86DGADirectVideo: mprotect (%s)\n",
                           strerror(errno));
		exit (-4);
	 }
#endif
   }
   XF86DGADirectVideoLL(dis, screen, enable);
}


static void cleanup()
{
        Display *disp;
	disp = XOpenDisplay(NULL);
	XF86DGADirectVideo(disp, 0, 0);
	XSync(disp,False);
        _exit(3);
}

XF86DGAGetVideo(dis, screen, addr, width, bank, ram)
Display *dis;
int screen;
char **addr;
int *width, *bank, *ram;
{
   int offset, fd;
   int pid, status;

   XF86DGAGetVideoLL(dis, screen , &offset, width, bank, ram);

#if defined(ISC) && defined(HAS_SVR3_MMAP)
   if ((fd = open("/dev/mmap", O_RDWR)) < 0)
   {
        fprintf(stderr, "XF86DGAGetVideo: failed to open /dev/mmap (%s)\n",
                           strerror(errno));
#else
   if ((fd = open("/dev/mem", O_RDWR)) < 0)
   {
        fprintf(stderr, "XF86DGAGetVideo: failed to open /dev/mem (%s)\n",
                           strerror(errno));
#endif
        exit (-1);
   }

#if defined(ISC) && defined(HAS_SVR3_MMAP)
   XFree86mloc.vaddr=(char *) 0;
   XFree86mloc.physaddr=(char *)offset;
   XFree86mloc.length=*bank;
   XFree86mloc.ioflg=1;

#define DEBUG_MMAP(state)     fprintf(stderr,"\
        XF86DGAGetVideo: (%s) \n\
            vaddr=%x/%x physaddr=%x/%x length=%d/%d\n",\
        (state) ,\
        XFree86mloc.vaddr,*addr,\
        XFree86mloc.physaddr,offset,\
        XFree86mloc.length,*bank)

   DEBUG_MMAP("before MMAP");
   if ((*addr = (char *) ioctl( fd, MAP, &XFree86mloc)) != (char *)-1) {
     offset=(int)XFree86mloc.physaddr;
     *bank=(int)XFree86mloc.length;
     DEBUG_MMAP("after MMAP");
   } else {
     DEBUG_MMAP("after MMAP");
     fprintf(stderr, "XF86DGAGetVideo: failed to mmap /dev/mmap (%s)\n",
                           strerror(errno));
#else

   /* This requires linux-0.99.pl10 or above */
   *addr = (void *)mmap(NULL, *bank, PROT_READ,
                            MAP_SHARED, fd, (off_t)offset);
#ifdef DEBUG
   fprintf(stderr, "XF86DGAGetVideo: physaddr: 0x%08x, size: %d\n",
	   (long)offset, *bank);
#endif
   if (*addr == (char *) -1) {
        fprintf(stderr, "XF86DGAGetVideo: failed to mmap /dev/mem (%s)\n",
                           strerror(errno));
#endif
        exit (-2);
   }
   _XFree86size = *bank;
   _XFree86addr = *addr;

#if FORK
   if ((pid = fork())) {
        Display *disp;
	waitpid(pid, &status, 0);
	disp = XOpenDisplay(NULL);
	XF86DGADirectVideo(disp, screen, 0);
	XSync(disp,False);
        if (WIFEXITED(status))
	    _exit(0);
	else
	    _exit(-1);
   }
#else
#ifdef linux
    on_exit(cleanup, 0);
#else
    atexit(cleanup);
#endif
    /* one shot cleanup attempts */
    signal(SIGSEGV, cleanup);
    signal(SIGBUS, cleanup);
    signal(SIGHUP, cleanup);
    signal(SIGFPE, cleanup);  
#endif
}

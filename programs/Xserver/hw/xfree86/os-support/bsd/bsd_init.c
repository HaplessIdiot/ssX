/* $XConsortium: bsd_init.c,v 1.1 94/03/28 21:28:00 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bsd/bsd_init.c,v 3.0 1994/09/23 10:23:58 dawes Exp $ */
/*
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Rich Murphey and David Wexelblat 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Rich Murphey and
 * David Wexelblat make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * RICH MURPHEY AND DAVID WEXELBLAT DISCLAIM ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL RICH MURPHEY OR DAVID WEXELBLAT BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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

extern void xf86VTRequest(
#if NeedFunctionPrototypes
	int
#endif
);

static Bool KeepTty = FALSE;
static int devConsoleFd = -1;
static int VTnum = -1;
static int initialVT = -1;

#ifdef PCCONS_SUPPORT
/* Stock 0.1 386bsd pccons console driver interface */
#define PCCONS_CONSOLE_DEV1 "/dev/ttyv0"
#define PCCONS_CONSOLE_DEV2 "/dev/vga"
#define PCCONS_CONSOLE_MODE O_RDWR|O_NDELAY
#endif

#ifdef CODRV_SUPPORT
/* Holger Veit's codrv console driver */
#define CODRV_CONSOLE_DEV "/dev/kbd"
#define CODRV_CONSOLE_MODE O_RDONLY|O_NDELAY
#endif

#ifdef SYSCONS_SUPPORT
/* The FreeBSD 1.1 version syscons driver uses /dev/ttyv0 */
#define SYSCONS_CONSOLE_DEV1 "/dev/ttyv0"
#define SYSCONS_CONSOLE_DEV2 "/dev/vga"
#define SYSCONS_CONSOLE_MODE O_RDWR|O_NDELAY
#endif

#define CHECK_DRIVER_MSG \
  "Check your kernel's console driver configuration and /dev entries"

static char *supported_drivers[] = {
#ifdef PCCONS_SUPPORT
	"pccons (with X support)",
#endif
#ifdef CODRV_SUPPORT
	"codrv",
#endif
#ifdef SYSCONS_SUPPORT
	"syscons",
#endif
#ifdef PCVT_SUPPORT
	"pcvt",
#endif
};

void xf86OpenConsole()
{
    int i, fd = -1, onoff;
    vtmode_t vtmode;
    char vtname[12];
    struct stat status;
    long syscons_version;
#ifdef PCVT_SUPPORT
    struct pcvtid pcvt_version;
#endif

    if (serverGeneration == 1)
    {
	/* check if we're run with euid==0 */
	if (geteuid() != 0)
	{
	    FatalError("xf86OpenConsole: Server must be suid root\n");
	}

	if (!KeepTty)
	{
	    /*
	     * detaching the controlling tty solves problems of kbd character
	     * loss.  This is not interesting for CO driver, because it is 
	     * exclusive.
	     */
	    setpgrp(0, getpid());
	    if ((i = open("/dev/tty",O_RDWR)) >= 0)
	    {
		ioctl(i,TIOCNOTTY,(char *)0);
		close(i);
	    }
	}

#ifdef SYSCONS_SUPPORT
	/* First check for syscons */
	if (((fd = open(SYSCONS_CONSOLE_DEV1, SYSCONS_CONSOLE_MODE, 0)) >= 0
	    || (fd = open(SYSCONS_CONSOLE_DEV2, SYSCONS_CONSOLE_MODE, 0)) >= 0)
	    && (ioctl(fd, VT_GETMODE, &vtmode) >= 0))
	{
#ifdef PCVT_SUPPORT
	    /* Check if PCVT */
	    if (ioctl(fd, VGAPCVTID, &pcvt_version) >= 0)
	    {
		syscons_version = -1;
	    }
	    else
#endif
	    /* Get syscons version */
	    {
		if (ioctl(fd, CONS_GETVERS, &syscons_version) < 0)
		{
		    syscons_version = 0;
		}
	    }

	    xf86Info.vtno = VTnum;

#ifdef VT_GETACTIVE
	    if (ioctl(fd, VT_GETACTIVE, &initialVT) < 0)
		initialVT = -1;
#endif
	    if (xf86Info.vtno == -1)
	    {
		/*
		 * For old syscons versions (<0x100), VT_OPENQRY returns
		 * the current VT rather than the next free VT.  In this
		 * case, the server gets started on the current VT instead
		 * of the next free VT.
		 */

#if 0
		/* check for the fixed VT_OPENQRY */
		if (syscons_version >= 0x100)
		{
#endif
		    if (ioctl(fd, VT_OPENQRY, &xf86Info.vtno) < 0)
		    {
			/* No free VTs */
			xf86Info.vtno = -1;
		    }
#if 0
		}
#endif

		if (xf86Info.vtno == -1)
		{
		    /*
		     * All VTs are in use.  If initialVT was found, use it.
		     * Otherwise, if stdin is a VT, use that one.
		     */
		    if (initialVT != -1)
		    {
			xf86Info.vtno = initialVT;
		    }
		    else if ((fstat(0, &status) >= 0) && S_ISCHR(status.st_mode)
		        && (ioctl(0, VT_GETMODE, &vtmode) >= 0))
		    {
			/* stdin is a VT */
			xf86Info.vtno = minor(status.st_rdev) + 1;
		    }
		    else
		    {
			if (syscons_version >= 0x100)
			{
			    FatalError("%s: Cannot find a free VT\n",
				       "xf86OpenConsole");
			}
			/* Should no longer reach here */
			FatalError("%s: %s %s\n\t%s %s\n",
				   "xf86OpenConsole",
				   "syscons versions prior to 1.0 require",
				   "either the",
				   "server's stdin be a VT",
				   "or the use of the vtxx server option");
		    }
		}
	    }

	    /* Don't need stdin any more */
	    fclose(stdin);

	    close(fd);
	    sprintf(vtname, "/dev/ttyv%01x", xf86Info.vtno - 1);
	    if ((fd = open(vtname, SYSCONS_CONSOLE_MODE, 0)) < 0)
	    {
		FatalError("xf86OpenConsole: Cannot open %s (%s)\n",
			   vtname, strerror(errno));
	    }
	    if (ioctl(fd, VT_GETMODE, &vtmode) < 0)
	    {
		FatalError("xf86OpenConsole: VT_GETMODE failed\n");
	    }
	    xf86Info.consType = SYSCONS;
	    if (xf86Verbose)
	    {
#ifdef PCVT_SUPPORT
		if (syscons_version == -1)
		{
		    ErrorF("Using pcvt driver (version %d.%d)\n",
			   pcvt_version.rmajor, pcvt_version.rminor);
		}
		else
#endif
		{
		    ErrorF("Using syscons driver with X support");
		    if (syscons_version >= 0x100)
		    {
	                ErrorF(" (version %d.%d)\n", syscons_version >> 8,
			       syscons_version & 0xFF);
		    }
		    else
		    {
	                ErrorF(" (version 0.x)\n");
		    }
		}

		ErrorF("(using VT number %d)\n\n", xf86Info.vtno);
	    }
	}
        else
#endif /* SYSCONS_SUPPORT */
	{
	    fd = -1;
#ifdef CODRV_SUPPORT
	    /* Try codrv next */
	    if ((fd = open(CODRV_CONSOLE_DEV, CODRV_CONSOLE_MODE, 0)) >= 0) 
	    {
	        int onoff = X_MODE_OFF;
     
	        if (ioctl(fd, CONSOLE_X_MODE, &onoff) < 0)
	        {
		    FatalError("%s: CONSOLE_X_MODE on %s failed (%s)\n%s\n%s\n",
			       "xf86OpenConsole",
			       CODRV_CONSOLE_DEV, strerror(errno),
			       "Was expecting codrv driver",
			       CHECK_DRIVER_MSG);
	        }
	        xf86Info.consType = CODRV011;
	    }
	    else
#endif /* CODRV_SUPPORT */
	    {
#ifdef CODRV_SUPPORT
	        if (errno == EBUSY)
	        {
		   FatalError("xf86OpenConsole: %s is already in use (codrv)\n",
			      CODRV_CONSOLE_DEV);
	        }
#endif /* CODRV_SUPPORT */
#ifdef PCCONS_SUPPORT
	        /* Now try pccons */
	        if ((fd = open(PCCONS_CONSOLE_DEV1, PCCONS_CONSOLE_MODE, 0))
		    >= 0 ||
		    (fd = open(PCCONS_CONSOLE_DEV2, PCCONS_CONSOLE_MODE, 0))
		    >= 0)
	        {
		    if (ioctl(fd, CONSOLE_X_MODE_OFF, 0) < 0)
		    {
		        FatalError(
			   "%s: CONSOLE_X_MODE_OFF failed (%s)\n%s\n%s\n",
			    "xf86OpenConsole",
			    strerror(errno),
			    "Was expecting pccons driver with X support",
			    CHECK_DRIVER_MSG);
		    }
		    xf86Info.consType = PCCONS;
		    if (xf86Verbose)
		    {
#ifdef PCVT_SUPPORT
			/* Check if PCVT */
			if (ioctl(fd, VGAPCVTID, &pcvt_version) >= 0)
			{
			    ErrorF("Using pcvt driver (version %d.%d)\n",
			           pcvt_version.rmajor, pcvt_version.rminor);
			}
			else
#endif
		            ErrorF("Using pccons driver with X support\n");
		    }
	        }
#endif /* PCCONS_SUPPORT */
	    }
     	}
	/* Check that a supported console driver was found */
        if (fd < 0)
	{
		char cons_drivers[80] = {0, };
		for (i = 0; i < sizeof(supported_drivers) / sizeof(char *); i++)
		{
			if (i)
			{
				strcat(cons_drivers, ", ");
			}
			strcat(cons_drivers, supported_drivers[i]);
		}
		FatalError(
		 "%s: No console driver found\n\tSupported drivers: %s\n\t%s\n",
		 "xf86OpenConsole", cons_drivers, CHECK_DRIVER_MSG);
	}
     	fclose(stdin);
	xf86Info.consoleFd = fd;
	xf86Info.screenFd = fd;

#ifdef CODRV_SUPPORT
	if (xf86Info.consType == CODRV011)
	{
	    /* 
	     * analyse whether this kernel has sufficient capabilities for 
	     * this xserver, if not don't proceed: it won't work.  Also 
	     * find out which codrv version.
	     */
#define NECESSARY	(CONS_HASKBD|CONS_HASKEYNUM|CONS_HASPX386)
	    struct oldconsinfo ci;
	    if ((ioctl(fd, OLDCONSGINFO, &ci) < 0 ||
	        (ci.info1 & NECESSARY) != NECESSARY))
	    {
		FatalError("xf86OpenConsole: %s\n%s\n%s\n",
		    "This Xserver has detected the codrv driver, but your",
		    "kernel doesn't appear to have the required facilities",
		    CHECK_DRIVER_MSG);
	    }
	    /* Check for codrv 0.1.2 or later */
	    if (ci.info1 & CONS_CODRV2)
	    {
		xf86Info.consType = CODRV01X;
		if (xf86Verbose)
		{
		    ErrorF("Using codrv 0.1.2 (or later)\n");
		}
	    }
	    else
	    {
		if (xf86Verbose)
		{
		    ErrorF("Using codrv 0.1.1\n");
		}
	    }
#undef NECESSARY
	}
#endif /* CODRV_SUPPORT */

	xf86Config(FALSE); /* Read XF86Config */

	switch (xf86Info.consType)
	{
#ifdef CODRV_SUPPORT
	case CODRV011:
	case CODRV01X:
	    onoff = X_MODE_ON;
	    if (ioctl (xf86Info.consoleFd, CONSOLE_X_MODE, &onoff) < 0)
	    {
		FatalError("%s: CONSOLE_X_MODE ON failed (%s)\n%s\n", 
			   "xf86OpenConsole", strerror(errno),
			   CHECK_DRIVER_MSG);
	    }
	    if (xf86Info.consType == CODRV01X)
		ioctl(xf86Info.consoleFd, VGATAKECTRL, 0);
	    break;
#endif
#ifdef PCCONS_SUPPORT
	case PCCONS:
	    if (ioctl (xf86Info.consoleFd, CONSOLE_X_MODE_ON, 0) < 0)
	    {
		FatalError("%s: CONSOLE_X_MODE_ON failed (%s)\n%s\n", 
			   "xf86OpenConsole", strerror(errno),
			   CHECK_DRIVER_MSG);
	    }
	    /*
	     * Hack to prevent keyboard hanging when syslogd closes
	     * /dev/console
	     */
	    if ((devConsoleFd = open("/dev/console", O_WRONLY,0)) < 0)
	    {
		ErrorF("Warning: couldn't open /dev/console (%s)\n",
		       strerror(errno));
	    }
	    break;
#endif
#ifdef SYSCONS_SUPPORT
	case SYSCONS:
	    /*
	     * now get the VT
	     */
	    if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno) != 0)
	    {
    	        ErrorF("xf86OpenConsole: VT_ACTIVATE failed\n");
	    }
	    if (ioctl(xf86Info.consoleFd, VT_WAITACTIVE, xf86Info.vtno) != 0)
	    {
	        ErrorF("xf86OpenConsole: VT_WAITACTIVE failed\n");
	    }

	    signal(SIGUSR1, xf86VTRequest);

	    vtmode.mode = VT_PROCESS;
	    vtmode.relsig = SIGUSR1;
	    vtmode.acqsig = SIGUSR1;
	    vtmode.frsig = SIGUSR1;
	    if (ioctl(xf86Info.consoleFd, VT_SETMODE, &vtmode) < 0) 
	    {
	        FatalError("xf86OpenConsole: VT_SETMODE VT_PROCESS failed\n");
	    }
	    if (ioctl(xf86Info.consoleFd, KDENABIO, 0) < 0)
	    {
	        FatalError("xf86OpenConsole: KDENABIO failed (%s)\n",
		           strerror(errno));
	    }
	    if (ioctl(xf86Info.consoleFd, KDSETMODE, KD_GRAPHICS) < 0)
	    {
	        FatalError("xf86OpenConsole: KDSETMODE KD_GRAPHICS failed\n");
	    }
   	    break; 
#endif /* SYSCONS_SUPPORT */
        }
    }
    else 
    {
	/* serverGeneration != 1 */
#ifdef SYSCONS_SUPPORT
    	if (xf86Info.consType == SYSCONS)
    	{
	    if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno) != 0)
	    {
	        ErrorF("xf86OpenConsole: VT_ACTIVATE failed\n");
	    }
        }
#endif /* SYSCONS_SUPPORT */
    }
    return;
}

void xf86CloseConsole()
{
    int onoff;
    struct vt_mode   VT;

    switch (xf86Info.consType)
    {
#ifdef CODRV_SUPPORT
    case CODRV011:
    case CODRV01X:
	onoff = X_MODE_OFF;
	if (xf86Info.consType == CODRV01X)
	{
	    ioctl (xf86Info.consoleFd, VGAGIVECTRL, 0);
	}
	ioctl (xf86Info.consoleFd, CONSOLE_X_MODE, &onoff);
        break;
#endif /* CODRV_SUPPORT */
#ifdef PCCONS_SUPPORT
    case PCCONS:
	ioctl (xf86Info.consoleFd, CONSOLE_X_MODE_OFF, 0);
	break;
#endif /* PCCONS_SUPPORT */
#ifdef SYSCONS_SUPPORT
    case SYSCONS:
        ioctl(xf86Info.consoleFd, KDSETMODE, KD_TEXT);  /* Back to text mode */
        if (ioctl(xf86Info.consoleFd, VT_GETMODE, &VT) != -1)
        {
	    VT.mode = VT_AUTO;
	    ioctl(xf86Info.consoleFd, VT_SETMODE, &VT); /* dflt vt handling */
        }
        if (ioctl(xf86Info.consoleFd, KDDISABIO, 0) < 0)
        {
            xf86FatalError("xf86OpenConsole: KDDISABIO failed (%s)\n",
	                   strerror(errno));
        }
	if (initialVT != -1)
		ioctl(xf86Info.consoleFd, VT_ACTIVATE, initialVT);
        break;
#endif /* SYSCONS_SUPPORT */
    }

    if (xf86Info.screenFd != xf86Info.consoleFd)
    {
	close(xf86Info.screenFd);
	close(xf86Info.consoleFd);
	if ((xf86Info.consoleFd = open("/dev/console",O_RDONLY,0)) <0)
	{
	    xf86FatalError("xf86CloseConsole: Cannot open /dev/console (%s)\n",
			   strerror(errno));
	}
    }
    close(xf86Info.consoleFd);
    if (devConsoleFd >= 0)
	close(devConsoleFd);
    return;
}

int xf86ProcessArgument (argc, argv, i)
int argc;
char *argv[];
int i;
{
	/*
	 * Keep server from detaching from controlling tty.  This is useful 
	 * when debugging (so the server can receive keyboard signals.
	 */
	if (!strcmp(argv[i], "-keeptty"))
	{
		KeepTty = TRUE;
		return(1);
	}
#ifdef SYSCONS_SUPPORT
	if ((argv[i][0] == 'v') && (argv[i][1] == 't'))
	{
		if (sscanf(argv[i], "vt%2d", &VTnum) == 0 ||
		    VTnum < 1 || VTnum > 12)
		{
			UseMsg();
			VTnum = -1;
			return(0);
		}
		return(1);
	}
#endif /* SYSCONS_SUPPORT */
	return(0);
}

void xf86UseMsg()
{
#ifdef SYSCONS_SUPPORT
	ErrorF("vtXX                   use the specified VT number (1-12)\n");
#endif /* SYSCONS_SUPPORT */
	ErrorF("-keeptty               ");
	ErrorF("don't detach controlling tty (for debugging only)\n");
	return;
}

/*
 * Copyright 1993,1994 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Dawes not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Dawes makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID DAWES BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: mit/server/ddx/x386/SuperProbe/OS_386BSD.c,v 2.8 1994/04/10 05:50:10 dawes Exp $ */

#include "Probe.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>

#ifdef __bsdi__
# include <i386/isa/pcconsioctl.h>
# define CONSOLE_X_MODE_ON PCCONIOCRAW
# define CONSOLE_X_MODE_OFF PCCONIOCCOOK
#else
  /* This header is part of codrv */
# if defined(__FreeBSD__) || defined(__NetBSD__)
#  include <machine/ioctl_pc.h>
/* both, Free and NetBSD have syscons */
#  include <machine/console.h>
# else
#  include <sys/ioctl_pc.h>
# endif
# undef CONSOLE_X_MODE_ON
# define CONSOLE_X_MODE_ON _IO('t',121)
# undef CONSOLE_X_MODE_OFF
# define CONSOLE_X_MODE_OFF _IO('t',122)
#endif
#if defined(__NetBSD__) && !defined(MAP_FILE)
#define MAP_FILE 0
#endif

static int CONS_fd = -1;
static int BIOS_fd = -1;

static Bool HasCodrv = FALSE;
static Bool HasUslVt = FALSE;

/*
 * OpenVideo --
 *
 * Enable access to the installed video hardware.  For 386BSD, we disable
 * IO protection, since this is currently the only way to access any
 * IO registers.
 */
int OpenVideo()
{
	if (geteuid() != 0)
	{
		fprintf(stderr, 
			"%s: Must be run as root or installed suid-root\n", 
			MyName);
		return(-1);
	}
	/*
	 * Attempt to open /dev/kbd.  If this fails, the driver is either
	 * pccons, or it is codrv and something else has it open.  errno
	 * will tell us which case it is.
	 */
	if ((CONS_fd = open("/dev/kbd", O_RDONLY|O_NDELAY, 0)) < 0)
	{
		if (errno == EBUSY)
		{
			/*
			 * Codrv, but something else is using the console
			 * in "X" mode
			 */
			fprintf(stderr,
			    "%s: Cannot be run while an X server is running\n",
			    MyName);
			return(-1);
		}
#ifndef __bsdi__
		/*
		 * pccons.  To guess if we're being run from within an
		 * X session, check $DISPLAY, and fail if it is set.
		 * This check is not foolproof -- just a guide.
		 */
		if (getenv("DISPLAY"))
		{
			fprintf(stderr,
			    "%s: Cannot be run while an X server is running\n",
			    MyName);
			fprintf(stderr,
			    "%s: If an X server is not running, unset $DISPLAY",
			    MyName);
			fprintf(stderr,
			    " and try again\n");
			return(-1);
		}
		if ((CONS_fd = open("/dev/vga", O_RDWR, 0)) < 0
		    && (CONS_fd = open("/dev/ttyv0", O_RDWR, 0)) < 0)
		{
			fprintf(stderr,
				"%s: Cannot open /dev/vga nor /dev/ttyv0\n", 
				MyName);
			return(-1);
		}
#endif
	}
	else
	{
#ifndef __bsdi__
		struct oldconsinfo tmp;

		if (ioctl(CONS_fd, OLDCONSGINFO, &tmp) < 0)
		{
			fprintf(stderr, "%s: Unknown console driver\n",
				MyName);
			return(-1);
		}
#endif
		HasCodrv = TRUE;
	}
#ifndef __bsdi__
	if (HasCodrv)
	{
		int onoff = X_MODE_ON;

		if (ioctl(CONS_fd, CONSOLE_X_MODE, &onoff) < 0)
		{
			fprintf(stderr, "%s: CONSOLE_X_MODE ON failed\n",
				MyName);
			return(-1);
		}
		ioctl(CONS_fd, VGATAKECTRL, 0);
	}
	else
	{
		/*
		 * not codrv and not BSDI; first look if we have a console
		 * driver that understands USL-style VT commands
		 */
		int vt_num;
		
		if (ioctl(CONS_fd, VT_GETACTIVE, &vt_num) == 0)
		{
			/* yes, so we don't use the old stuff */
			HasUslVt = TRUE;
			if (ioctl(CONS_fd, KDENABIO, 0) < 0)
			{
				fprintf(stderr, "%s: KDENABIO failed\n",
					MyName);
				return(-1);
			}
		}
		else
#endif
			if (ioctl(CONS_fd, CONSOLE_X_MODE_ON, 0) < 0)
		{
			fprintf(stderr, "%s: CONSOLE_X_MODE_ON failed\n",
				MyName);
			return(-1);
		}
#ifdef __bsdi__
	ioctl(CONS_fd, PCCONENABIOPL, 0);
#else
	}
#endif
	return(CONS_fd);
}

/*
 * CloseVideo --
 *
 * Disable access to the video hardware.  For 386BSD, we re-enable
 * IO protection.
 */
void CloseVideo()
{
	if (CONS_fd != -1)
	{
#ifndef __bsdi__
		int onoff = X_MODE_OFF;

		if (HasCodrv)
		{
			ioctl(CONS_fd, VGAGIVECTRL, 0);
			ioctl(CONS_fd, CONSOLE_X_MODE, &onoff);
		}
		else if(HasUslVt)
			ioctl(CONS_fd, KDDISABIO, 0);
		else
#endif
			ioctl(CONS_fd, CONSOLE_X_MODE_OFF, 0);
	}
#ifdef __bsdi__
	ioctl(CONS_fd, PCCONDISABIOPL, 0);
#endif
	if (CONS_fd > 0)
	{
		close(CONS_fd);
	}
}

/*
 * MapVGA --
 *
 * Map the VGA memory window (0xA0000-0xAFFFF) as read/write memory for
 * the process for use in probing memory.
 */
Byte *MapVGA()
{
	int fd;
	Byte *base;

	if ((fd = open("/dev/vga", O_RDWR)) < 0)
	{
		fprintf(stderr, "%s: Failed to open /dev/vga\n", MyName);
		return((Byte *)0);
	}
	base = (Byte *)mmap((caddr_t)0, 0x10000, PROT_READ|PROT_WRITE,
			    MAP_FILE, fd, 0);
	close(fd);
	if ((long)base == -1)
	{
		fprintf(stderr, "%s: Failed to mmap framebuffer\n", MyName);
		return((Byte *)0);
	}
	return(base);
}

/*
 * UnMapVGA --
 *
 * Unmap the VGA memory window.
 */
void UnMapVGA(base)
Byte *base;
{
	munmap((caddr_t)base, 0x10000);
}

/*
 * ReadBIOS --
 *
 * Read 'Len' bytes from the video BIOS at address 'Bios_Base'+'Offset' into 
 * buffer 'Buffer'.
 */
int ReadBIOS(Offset, Buffer, Len)
unsigned Offset;
Byte *Buffer;
int Len;
{
	Word tmp;
	Byte *Base = Bios_Base + Offset;

	if (BIOS_fd == -1)
	{
		if ((BIOS_fd = open("/dev/mem", O_RDONLY, 0)) < 0)
		{
			fprintf(stderr, "%s: cannot open /dev/mem\n", MyName);
			return(-1);
		}
	}
	if ((off_t)((off_t)Base & 0x7FFF) != (off_t)0)
	{
		/*
	 	 * Sanity check...
	 	 */
		(void)lseek(BIOS_fd, (off_t)((off_t)Base & 0xF8000), SEEK_SET);
		(void)read(BIOS_fd, &tmp, 2);
		if (tmp != (Word)0xAA55)
		{
			fprintf(stderr, 
				"%s: BIOS sanity check failed, addr=%x\n",
				MyName, (int)Base);
			return(-1);
		}
	}
	if (lseek(BIOS_fd, (off_t)Base, SEEK_SET) < 0)
	{
		fprintf(stderr, "%s: BIOS seek failed\n", MyName);
		return(-1);
	}
	if (read(BIOS_fd, Buffer, Len) != Len)
	{
		fprintf(stderr, "%s: BIOS read failed\n", MyName);
		return(-1);
	}
	return(Len);
}

/*
 * EnableIOPort --
 *
 * Enable access to 'NumPorts' IO ports listed in array 'Ports'.  For 386BSD, 
 * we've disabled IO protections so this is a no-op.
 */
/*ARGSUSED*/
int EnableIOPorts(NumPorts, Ports)
CONST int NumPorts;
CONST Word *Ports;
{
	return(0);
}

/*
 * DisableIOPort --
 *
 * Disable access to 'NumPorts' IO ports listed in array  'Ports'.  For 386BSD, 
 * we've disabled IO protections so this is a no-op.
 */
/*ARGSUSED*/
/*ARGSUSED*/
int DisableIOPorts(NumPorts, Port)
CONST int NumPorts;
CONST Word *Port;
{
	return(0);
}

/*
 * ShortSleep --
 *
 * Sleep for the number of milliseconds specified in 'Delay'.
 */
void ShortSleep(Delay)
int Delay;
{
	usleep(Delay * 1000);
}


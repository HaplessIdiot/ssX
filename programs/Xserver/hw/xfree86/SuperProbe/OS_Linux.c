/*
 * Copyright 1993,1994 by Orest Zborowski <obz@Kodak.COM>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Orest Zborowski not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Orest Zborowski makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * OREST ZBOROWSKI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OREST ZBOROWSKI BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: mit/server/ddx/x386/SuperProbe/OS_Linux.c,v 2.3 1994/02/28 14:09:37 dawes Exp $ */

#include "Probe.h"

#include <fcntl.h>
#include <sys/kd.h>
#include <sys/vt.h>
#include <sys/mman.h>

static int VT_fd = -1;
static int VT_num = -1;
static int VT_cur = -1;
static int BIOS_fd = -1;

/*
 * OpenVideo --
 *
 * Enable access to the installed video hardware.  For Linux, open a new
 * VT, and disable IO protection, since we may need to get at extended 
 * registers (full 16-bit decoding).
 */
int OpenVideo()
{
	int fd;
	char fn[20];
	struct vt_stat vts;

	if (geteuid() != 0)
	{
		fprintf(stderr,
			"%s: Must be run as root or installed suid-root\n",
			MyName);
		return(-1);
	}

	if ((fd = open("/dev/console", O_WRONLY, 0)) < 0)
	{
		fprintf(stderr, "%s: Cannot open /dev/console\n", MyName);
		return(-1);
	}
	/*
	 * The Linux kernel doesn't keep track of the VT to return
	 * to, so we have to maintain that ourselves. (Hmm... would
	 * it work like a stack [chain of vt parents])?
	 */
	if (ioctl(fd, VT_GETSTATE, &vts) == 0)
	{
		VT_cur = vts.v_active;
	}
	else
	{
		VT_cur = -1;
	}
	if ((ioctl(fd, VT_OPENQRY, &VT_num) < 0) || (VT_num == -1))
	{
		fprintf(stderr, "%s: No free VTs\n", MyName);
		return(-1);
	}
	close(fd);
	sprintf(fn, "/dev/tty%d", VT_num);
	if ((VT_fd = open(fn, O_RDWR|O_NDELAY, 0)) < 0)
	{
		fprintf(stderr, "%s: Could not open VT %s\n", MyName, fn);
		return(-1);
	}
	/*
	 * Hmmm... I thought WAITACTIVE didn't activate, only
	 * waited for the vt to activate.  Am I wrong in that
	 * assumption?
	 */
	if (ioctl(VT_fd, VT_ACTIVATE, VT_num) != 0)
	{
		fprintf(stderr, "%s: VT_ACTIVATE failed!\n", MyName);
		return(-1);
	}
	if (ioctl(VT_fd, VT_WAITACTIVE, VT_num) != 0)
	{
		fprintf(stderr, "%s: VT_WAITACTIVE failed!\n", MyName);
		return(-1);
	}
	/*
	 * Give the other VT time to release.
	 */
	sleep(1);
	/*
	 * Get IOPL so we can get at all the I/O ports.
	 */
	iopl(3);
	return(VT_fd);
}

/*
 * CloseVideo --
 *
 * Disable access to the video hardware.  For Linux, close the VT, and
 * re-enable IO protection.
 */
void CloseVideo()
{
	int fd;

	iopl(0);
	if (VT_fd > 0)
	{
		close(VT_fd);
		if (VT_cur >= 0)
		{
			if ((fd = open("/dev/console", O_WRONLY, 0)) >= 0)
			{
				ioctl(fd, VT_ACTIVATE, VT_cur);
				close(fd);
			}
		}
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

	if ((fd = open("/dev/mem", O_RDWR)) < 0)
	{
		fprintf(stderr, "%s: Failed to open /dev/mem\n", MyName);
		return((Byte *)0);
	}
	base = (Byte *)mmap((caddr_t)0, 0x10000, PROT_READ|PROT_WRITE,
			    MAP_SHARED, fd, (off_t)0xA0000);
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
	return;
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
 * Enable access to 'NumPorts' IO ports listed in array 'Ports'.  For Linux, 
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
 * Disable access to 'NumPorts' IO ports listed in array  'Ports'.  For Linux, 
 * we've disabled IO protections so this is a no-op.
 */
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

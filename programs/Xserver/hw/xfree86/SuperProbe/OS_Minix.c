/*
 * Copyright 1993,1994 by Philip Homburg <philip@cs.vu.nl>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Philip Homburg not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Philip Homburg makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * PHILIP HOMBURG DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL PHILIP HOMBURG BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: mit/server/ddx/x386/SuperProbe/OS_Minix.c,v 2.1 1994/02/28 14:09:40 dawes Exp $ */

#include "Probe.h"

#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/memio.h>

static int CONS_fd = -1;
static int BIOS_fd = -1;

/*
 * OpenVideo --
 *
 * Enable access to the installed video hardware.  This also gives us 
 * access to all I/O registers and the interrupt enable/disable bit.
 */
int OpenVideo()
{
	void *base;
	u32_t aligned_base;
	struct mio_map mio_map;

	if (geteuid() != 0)
	{
		fprintf(stderr, 
			"%s: Must be run as root or installed suid-root\n", 
			MyName);
		return(-1);
	}
	base= malloc(0x21000);
	if (base == NULL)
	{
		fprintf(stderr, 
		"%s: Cannot allocate enough memory to map /dev/vga\n", 
			MyName);
		return(-1);
	}
	aligned_base= (((unsigned long)base) + 0xFFF) & ~0xFFF;

	if ((CONS_fd = open("/dev/vga", O_RDWR, 0)) == -1)
	{
		fprintf(stderr, "%s: Cannot open /dev/vga\n", 
			MyName);
		return(-1);
	}
	mio_map.mm_base= aligned_base;
	mio_map.mm_size= 0x20000;	/* 128K */
	if (ioctl(CONS_fd, MIOCMAP, &mio_map) == -1)
	{
		fprintf(stderr, "%s: Cannot map /dev/vga\n", 
			MyName);
		close(CONS_fd);
		CONS_fd= -1;
		return(-1);
	}
	return(CONS_fd);
}

/*
 * CloseVideo --
 *
 * Disable access to the video hardware.  For Minix, this removes access
 * to I/O registers.
 */
void CloseVideo()
{
	if (CONS_fd > 0)
	{
		close(CONS_fd);
		CONS_fd= -1;
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
        return((Byte *)0);
}

/*
 * UnMapVGA --
 *
 * Unmap the VGA memory window.
 */
void UnMapVGA(base)
Byte *base;
{
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
 * Enable access to 'NumPorts' IO ports listed in array 'Ports'.  For Minix,
 * we've disabled IO protections so this is a no-op.
 */
/*ARGSUSED*/
int EnableIOPorts(NumPorts, Ports)
CONST int NumPorts;
CONST Word *Ports;
{
	if (CONS_fd == -1) abort();

	return(0);
}

/*
 * DisableIOPort --
 *
 * Disable access to 'NumPorts' IO ports listed in array  'Ports'.  For Minix, 
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

/*
 * Copyright 1993,1994 by Robert V. Baron <Robert.Baron@ernst.mach.cs.cmu.edu>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Robert V. Baron not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Robert V. Baron makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ROBERT V. BARON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ROBERT V. BARON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: mit/server/ddx/x386/SuperProbe/OS_Mach.c,v 2.4 1994/02/28 14:09:39 dawes Exp $ */

#include "Probe.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

/* These can be Mach 2.5 or 3.0 headers */
#include <mach.h>
#include <mach_error.h>
#include <mach/message.h>
#ifdef MACH_PORT_NULL
#define MACH3		/* they must have been 3.0 headers beckerd@cs.unc.edu */
#endif
#undef task_self

static int IOPL_fd = -1;

/*
 * OpenVideo --
 *
 * Enable access to the installed video hardware.  For Mach, we disable
 * IO protection, since this is currently the only way to access any
 * IO registers.
 */
int screen_addr;

int OpenVideo()
{
	int ret;
#define C /*Bios_Base*/ 0xc0000
#define S 0x40000

	if ((IOPL_fd = open("/dev/iopl", O_RDWR, 0)) < 0) {
		fprintf(stderr, "Failed to open /dev/iopl\n");
		return -1;
	}
#define KERN_SUCESS 0
	if (KERN_SUCESS != vm_allocate(task_self(), &screen_addr, S, TRUE)) {
		fprintf(stderr, "Failed vmallocate %x\n", S);
		close(IOPL_fd);
		return -1;
	}
	if (mmap(screen_addr+C, S, 3, 1, IOPL_fd, C) < 0) {
		fprintf(stderr, "Failed to mmap %x bytes at %x\n", S, C);
		vm_deallocate(task_self(), screen_addr, S);
		close(IOPL_fd);
		return -1;
	}
	return(IOPL_fd);
}

/*
 * CloseVideo --
 *
 * Disable access to the video hardware.  For Mach, we re-enable
 * IO protection.
 */
void CloseVideo()
{
	if (KERN_SUCESS != vm_deallocate(task_self(), screen_addr, S)) {
		fprintf(stderr, "Failed vmdeallocate %x\n", S);
	}
	close(IOPL_fd);
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
	Byte *Base = (Byte *)(screen_addr + Bios_Base + Offset);

#ifdef	DEBUG
	{int i;
		fprintf(stderr, "ReadBIOS(Offset %x, Buffer %x, Len %x) .. ",
			Offset, Buffer, Len);
		for (i=0;i<Len;i++)
			fprintf(stderr," [%c](%x)|", *(Base+i), *(Base+i));
		fprintf(stderr,"\n");
	}
#endif
	bcopy(Base, Buffer, Len);
	return (Len);
}

/*
 * EnableIOPort --
 *
 * Enable access to 'NumPorts' IO ports listed in array 'Ports'.  For Mach, 
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
 * Disable access to 'NumPorts' IO ports listed in array  'Ports'.  For Mach, 
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
#ifdef MACH3
	struct trial {
		mach_msg_header_t	h;
		mach_msg_type_t	t;
		int		d;
	} msg_rcv;
	mach_port_t		my_port;
#else
	/* This does Mach 2.5 IPC */	
	struct trial {
		msg_header_t	h;
		msg_type_t	t;
		int		d;
	} msg_rcv;
	port_t		my_port;
#endif
	kern_return_t	error;
	int		ret;

	if ((error = port_allocate(task_self(), &my_port)) != KERN_SUCCESS) {
		printf("ShortSleep: port_allocate failed with %d: %s\n", 
			error, mach_error_string(error));
		return;
	}
	
#ifdef MACH3
	msg_rcv.h.msgh_size = sizeof(msg_rcv);
	if ((ret = msg_receive(&msg_rcv.h, MACH_RCV_TIMEOUT,	Delay)) != MACH_RCV_TIMED_OUT) {
#else
	if ((ret = msg_receive(&msg_rcv.h, RCV_TIMEOUT,	Delay)) != RCV_TIMED_OUT) {
	msg_rcv.h.msg_local_port = my_port;
#endif
		mach_error("ShortSleep: msg_receive returned ", ret);
		return;
	}

	if ((error = port_deallocate(task_self(), my_port)) != KERN_SUCCESS) {
		printf("ShortSleep: port_deallocate failed with %d: %s\n", 
			error, mach_error_string(error));
		return;
	}
}

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/posix_tty.c,v 3.10 1998/01/24 16:58:36 hohndel Exp $ */
/*
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.
 * David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*
 *
 * Copyright (c) 1997  Metro Link Incorporated
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 *
 */

/* $XConsortium: posix_tty.c /main/7 1996/10/19 18:07:47 kaleb $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"

#define SYSCALL(call) while(((call) == -1) && (errno == EINTR))
static Bool not_a_tty = FALSE;

void 
xf86SetMouseSpeed (mouse, old, new, cflag)
MouseDevPtr mouse;
int old;
int new;
unsigned cflag;
{
	struct termios tty;
	char *c;

	if (not_a_tty)
		return;

	if (tcgetattr(mouse->mseFd, &tty) < 0)
	{
		not_a_tty = TRUE;
		ErrorF("Warning: %s unable to get status of mouse fd (%s)\n",
		       mouse->mseDevice, strerror(errno));
		return;
	}

	/* this will query the initial baudrate only once */
	if (mouse->oldBaudRate < 0) { 
	   switch (cfgetispeed(&tty)) 
	      {
	      case B9600: 
		 mouse->oldBaudRate = 9600;
		 break;
	      case B4800: 
		 mouse->oldBaudRate = 4800;
		 break;
	      case B2400: 
		 mouse->oldBaudRate = 2400;
		 break;
	      case B1200: 
	      default:
		 mouse->oldBaudRate = 1200;
		 break;
	      }
	}

	tty.c_iflag = IGNBRK | IGNPAR;
	tty.c_oflag = 0;
	tty.c_lflag = 0;
	tty.c_cflag = (tcflag_t)cflag;
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 1;

	switch (old)
	{
	case 9600:
		cfsetispeed(&tty, B9600);
		cfsetospeed(&tty, B9600);
		break;
	case 4800:
		cfsetispeed(&tty, B4800);
		cfsetospeed(&tty, B4800);
		break;
	case 2400:
		cfsetispeed(&tty, B2400);
		cfsetospeed(&tty, B2400);
		break;
	case 1200:
	default:
		cfsetispeed(&tty, B1200);
		cfsetospeed(&tty, B1200);
	}

	if (tcsetattr(mouse->mseFd, TCSANOW, &tty) < 0)
	{
		if (xf86AllowMouseOpenFail) {
			ErrorF("Unable to set status of mouse fd (%s) - Continuing...\n",
			       strerror(errno));
			return;
		}
		xf86FatalError("Unable to set status of mouse fd (%s)\n",
			       strerror(errno));
	}

	switch (new)
	{
	case 9600:
		c = "*q";
		cfsetispeed(&tty, B9600);
		cfsetospeed(&tty, B9600);
		break;
	case 4800:
		c = "*p";
		cfsetispeed(&tty, B4800);
		cfsetospeed(&tty, B4800);
		break;
	case 2400:
		c = "*o";
		cfsetispeed(&tty, B2400);
		cfsetospeed(&tty, B2400);
		break;
	case 1200:
	default:
		c = "*n";
		cfsetispeed(&tty, B1200);
		cfsetospeed(&tty, B1200);
	}

	if (mouse->mseType == P_LOGIMAN || mouse->mseType == P_LOGI)
	{
		if (write(mouse->mseFd, c, 2) != 2)
		{
			if (xf86AllowMouseOpenFail) {
				ErrorF("Unable to write to mouse fd (%s) - Continuing...\n",
				       strerror(errno));
				return;
			}
			xf86FatalError("Unable to write to mouse fd (%s)\n",
				       strerror(errno));
		}
	}
	usleep(100000);

	if (tcsetattr(mouse->mseFd, TCSANOW, &tty) < 0)
	{
		if (xf86AllowMouseOpenFail) {
			ErrorF("Unable to set status of mouse fd (%s) - Continuing...\n",
			       strerror(errno));
			return;
		}
		xf86FatalError ("Unable to set status of mouse fd (%s)\n",
						strerror (errno));
	}
}

static int 
GetBaud (int baudrate)
{
#ifdef B300
	if (baudrate == 300)
		return B300;
#endif
#ifdef B1200
	if (baudrate == 1200)
		return B1200;
#endif
#ifdef B2400
	if (baudrate == 2400)
		return B2400;
#endif
#ifdef B4800
	if (baudrate == 4800)
		return B4800;
#endif
#ifdef B9600
	if (baudrate == 9600)
		return B9600;
#endif
#ifdef B19200
	if (baudrate == 19200)
		return B19200;
#endif
#ifdef B38400
	if (baudrate == 38400)
		return B38400;
#endif
#ifdef B57600
	if (baudrate == 57600)
		return B57600;
#endif
#ifdef B115200
	if (baudrate == 115200)
		return B115200;
#endif
#ifdef B230400
	if (baudrate == 230400)
		return B230400;
#endif
#ifdef B460800
	if (baudrate == 460800)
		return B460800;
#endif
	return (0);
}

int
xf86OpenSerial (XF86OptionPtr options)
{
#ifdef LYNX
	struct sgttyb ms_sgtty;
#endif
	struct termios t;
	int fd, i;
	char *dev;

	dev = xf86FindOptionValue (options, "Device");
	if (!dev)
	{
		ErrorF ("xf86OpenSerial: No Device specified.\n");
		return (-1);
	}

	SYSCALL (fd = open (dev, O_RDWR | O_NONBLOCK | O_EXCL));
	if (fd == -1)
	{
		ErrorF ("xf86OpenSerial: Cannot open device %s\n\t%s.\n", dev,
			strerror (errno));
		return (-1);
	}

	if (!isatty (fd))
	{
		ErrorF ("xf86OpenSerial: Specified device %s is not a tty\n", dev);
		SYSCALL (close (fd));
		errno = EINVAL;
		return (-1);
	}

#ifdef LYNX
	/* LynxOS does not assert DTR without this */
	ioctl (control, TIOCGETP, (char *) &ms_sgtty);
	ioctl (control, TIOCSDTR, (char *) &ms_sgtty);
#endif

	/* set up default port parameters */
	SYSCALL (tcgetattr (fd, &t));
	t.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR\
						|IGNCR|ICRNL|IXON);
	t.c_oflag &= ~OPOST;
	t.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	t.c_cflag &= ~(CSIZE|PARENB);
	t.c_cflag |= CS8;

	cfsetispeed (&t, B9600);
	cfsetospeed (&t, B9600);
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;

	SYSCALL (tcsetattr (fd, TCSANOW, &t));

	if (xf86SetSerial (fd, options) == -1)
	{
		SYSCALL (close (fd));
		return (-1);
	}

	SYSCALL (i = fcntl (fd, F_GETFL, 0));
	if (i == -1)
	{
		SYSCALL (close (fd));
		return (-1);
	}
	i &= ~O_NONBLOCK;
	SYSCALL (i = fcntl (fd, F_SETFL, i));
	if (i == -1)
	{
		SYSCALL (close (fd));
		return (-1);
	}
	return (fd);
}

int
xf86SetSerial (int fd, XF86OptionPtr options)
{
	struct termios t;
	char *s;
	int baud, r;
    XF86OptionPtr tmp = options;

	SYSCALL (tcgetattr (fd, &t));

	if (s = xf86FindOptionValue (options, "BaudRate"))
	{
		if (baud = GetBaud (atoi (s)))
		{
			cfsetispeed (&t, baud);
			cfsetospeed (&t, baud);
		}
		else
		{
			ErrorF ("Invalid Option BaudRate value: %s\n", s);
			return (-1);
		}
	}

	if (s = xf86FindOptionValue (options, "StopBits"))
	{
		switch (atoi (s))
		{
		case 1:
			t.c_cflag &= ~(CSTOPB);
			break;
		case 2:
			t.c_cflag |= CSTOPB;
			break;
		default:
			ErrorF ("Invalid Option StopBits value: %s\n", s);
			return (-1);
			break;
		}
	}

	if (s = xf86FindOptionValue (options, "DataBits"))
	{
		switch (atoi (s))
		{
		case 5:
			t.c_cflag &= ~(CSIZE);
			t.c_cflag |= CS5;
			break;
		case 6:
			t.c_cflag &= ~(CSIZE);
			t.c_cflag |= CS6;
			break;
		case 7:
			t.c_cflag &= ~(CSIZE);
			t.c_cflag |= CS7;
			break;
		case 8:
			t.c_cflag &= ~(CSIZE);
			t.c_cflag |= CS8;
			break;
		default:
			ErrorF ("Invalid Option DataBits value: %s\n", s);
			return (-1);
			break;
		}
	}

	if (s = xf86FindOptionValue (options, "Parity"))
	{
		if (StrCaseCmp (s, "Odd") == 0)
		{
			t.c_cflag |= PARENB | PARODD;
		}
		else if (StrCaseCmp (s, "Even") == 0)
		{
			t.c_cflag |= PARENB;
			t.c_cflag &= ~(PARODD);
		}
		else if (StrCaseCmp (s, "None") == 0)
		{
			t.c_cflag &= ~(PARENB);
		}
		else
		{
			ErrorF ("Invalid Option Party value: %s\n", s);
			return (-1);
		}
	}

	if (s = xf86FindOptionValue (options, "Vmin"))
	{
		t.c_cc[VMIN] = atoi (s);
	}
	if (s = xf86FindOptionValue (options, "Vtime"))
	{
		t.c_cc[VTIME] = atoi (s);
	}

	if (s = xf86FindOptionValue (options, "FlowControl"))
	{
		if (StrCaseCmp (s, "Xon") == 0)
		{
			t.c_iflag |= IXON | IXOFF;
		}
		else if (StrCaseCmp (s, "None") == 0)
		{
			t.c_iflag &= ~(IXON | IXOFF);
		}
		else
		{
			ErrorF ("Invalid Option FlowControl value: %s\n", s);
			return (-1);
		}
	}

	if (s = xf86FindOptionValue (options, "ClearDTR"))
	{
#ifdef CLEARDTR_SUPPORT
          int val = TIOCM_DTR;
          SYSCALL (ioctl(fd, TIOCMBIC, &val));
#else
		ErrorF ("Option ClearDTR not supported on this OS\n");
			return (-1);
#endif
	}

	if (s = xf86FindOptionValue (options, "ClearRTS"))
	{
#ifdef CLEARRTS_SUPPORT
          int val = TIOCM_RTS;
          SYSCALL (ioctl(fd, TIOCMBIC, &val));
#else
		ErrorF ("Option ClearRTS not supported on this OS\n");
			return (-1);
#endif
	}

	SYSCALL (r = tcsetattr (fd, TCSANOW, &t));
	return (r);
}

xf86ssize_t
xf86ReadSerial (int fd, void *buf, xf86size_t count)
{
	int r;

	SYSCALL (r = read (fd, buf, count));
	return (r);
}

xf86ssize_t
xf86WriteSerial (int fd, void *buf, xf86size_t count)
{
	int r;

	SYSCALL (r = write (fd, buf, count));
	return (r);
}

int
xf86CloseSerial (int fd)
{
	int r;

	SYSCALL (r = close (fd));
	return (r);
}

int
xf86FlushInput(fd)
int fd;
{
	return tcflush(fd, TCIFLUSH);
}


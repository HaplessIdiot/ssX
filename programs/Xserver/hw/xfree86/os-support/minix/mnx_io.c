/* $XConsortium: mnx_io.c,v 1.1 94/03/28 21:29:50 dpw Exp $ */
/*
 * Copyright 1993 by Vrije Universiteit, The Netherlands
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the Vrije Universiteit and David 
 * Dawes not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission.
 * The Vrije Universiteit and David Dawes make no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE VRIJE UNIVERSITEIT AND DAVID DAWES DISCLAIM ALL WARRANTIES WITH 
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THE VRIJE UNIVERSITEIT OR 
 * DAVID DAWES BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR 
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */


#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"

void xf86SoundKbdBell(loudness, pitch, duration)
int loudness;
int pitch;
int duration;
{
	kio_bell_t kio_bell;
	int r;

	kio_bell.kb_pitch = pitch;
	kio_bell.kb_volume = 1000000 / 100 * loudness;
	kio_bell.kb_duration.tv_sec = duration / 1000;
	kio_bell.kb_duration.tv_usec = (duration % 1000) * 1000;
	r = ioctl(xf86Info.kbdFd, KIOCBELL, &kio_bell);
	if (r != 0)
	{
		ErrorF("(warning) unable to ring keyboard bell: %s\n",
		       strerror(errno));
	}
}

void xf86SetKbdLeds(leds)
int leds;
{
	kio_leds_t kio_leds;
	int r;

	kio_leds.kl_bits= leds;
	r = ioctl(xf86Info.kbdFd, KIOCSLEDS, &kio_leds);
	if (r != 0)
	{
		ErrorF("(warning) unable to set keyboard leds: %s\n",
		       strerror(errno));
	}
}

int xf86GetKbdLeds()
{
	return(0);
}

#if __STDC__
void xf86SetKbdRepeat(char rad)
#else
void xf86SetKbdRepeat(rad)
char rad;
#endif
{
	return;
}

void xf86KbdInit()
{
	static int kbd_fd = -1;
	int flags, r;

	/* Open the keyboard device if not already done so */
	if (kbd_fd < 0)
	{
		kbd_fd = open("/dev/kbd", O_RDONLY);
		if (kbd_fd == -1)
			FatalError("Unable to open keyboard\n");
		else
			xf86Info.kbdFd = kbd_fd;

		/* Mark the keyboard as asynchronous */
		flags= fcntl(xf86Info.kbdFd, F_GETFD);
		if (flags == -1)
			FatalError("Unable to get keyboard flags\n");
		r = fcntl(xf86Info.kbdFd, F_SETFD, flags | FD_ASYNCHIO);
		if (r == -1)
			FatalError("Unable to set keyboard flags\n");
		xf86Info.kbdAvail= 0;
		xf86Info.kbdInprogress= FALSE;
	}
}

int xf86KbdOn()
{
	if (!xf86Info.kbdInprogress)
	{
		/* Get rid of old data */
		for (;;)
		{
			int r;
			r = read(xf86Info.kbdFd, xf86Info.kbdBuf,
				 sizeof(xf86Info.kbdBuf));
			if (r > 0)
				continue;
			if (r == -1 && errno == EINPROGRESS)
			{
				xf86Info.kbdInprogress = TRUE;
				break;
			}
			FatalError("unable to read from keyboard (%s)\n",
				   strerror(errno));
		}
	}
	return(xf86Info.kbdFd);
}

int xf86KbdOff()
{
	/* Should RemoveEnabledDevice() be done for Minix?? */
	/* If it shouldn't be done, we should return -1 here */
	return(xf86Info.kbdFd);
}

void xf86KbdEvents()
{
	int i, r;

	for (;;)
	{
		if (xf86Info.kbdInprogress)
		{
			/* Nothing to do */
			return;
		}
		if (xf86Info.kbdAvail > 0)
		{
			for (i = 0; i < xf86Info.kbdAvail; i++)
				xf86PostKbdEvent(xf86Info.kbdBuf[i]);
			xf86Info.kbdAvail = 0;
		}
		r = read(xf86Info.kbdFd, xf86Info.kbdBuf,
			 sizeof(xf86Info.kbdBuf));
		if (r > 0)
		{
			xf86Info.kbdAvail = r;
			continue;
		}
		if (r == -1 && errno == EINPROGRESS)
		{
			xf86Info.kbdInprogress = TRUE;
			AddEnabledDevice(xf86Info.kbdFd);
			return;
		}
		FatalError("unable to read from keyboard (%s)\n",
			   r == 0 ? "eof" : strerror(errno));
	}
}

void xf86MouseInit()
{
	return;
}

int xf86MouseOn()
{
	int r, flags;

	if ((xf86Info.mseFd = open(xf86Info.mseDevice, O_RDWR)) < 0)
	{
		FatalError("Cannot open mouse (%s)\n", strerror(errno));
	}

	xf86SetupMouse();

	/* Mark the mouse as asynchronous */
	flags = fcntl(xf86Info.mseFd, F_GETFD);
	if (flags == -1)
		FatalError("Unable to get mouse flags (%s)\n", strerror(errno));
	r = fcntl(xf86Info.mseFd, F_SETFD, flags | FD_ASYNCHIO);
	if (r == -1)
		FatalError("Unable to set mouse flags (%s)\n", strerror(errno));
	xf86Info.mseAvail = 0;
	xf86Info.mseInprogress = FALSE;

	/* Get rid of old data */
	for (;;)
	{
		r = read(xf86Info.mseFd, xf86Info.mseBuf,
			 sizeof(xf86Info.mseBuf));
		if (r > 0)
			continue;
		if (r == -1 && errno == EINPROGRESS)
		{
			xf86Info.mseInprogress= TRUE;
			break;
		}
		FatalError("unable to read from mouse (%s)\n",
			   strerror(errno));
	}
	return(xf86Info.mseFd);
}

void xf86MouseEvents()
{
	int r;

	for (;;)
	{
		if (xf86Info.mseInprogress)
		{
			/* Nothing to do */
			return;
		}
		if (xf86Info.mseAvail == 0)
		{
			r = read(xf86Info.mseFd, xf86Info.mseBuf,
				 sizeof(xf86Info.mseBuf));
			if (r > 0)
			{
				xf86Info.mseAvail = r;
			}
			else if (r == -1 && errno == EINPROGRESS)
			{
				xf86Info.mseInprogress = TRUE;
				AddEnabledDevice(xf86Info.mseFd);
				return;
			}
			else
			{
				FatalError("unable to read from mouse: %s\n",
					   r == 0 ? "eof" : strerror(errno));
			}
		}
		xf86MouseProtocol((unsigned char *)xf86Info.mseBuf,
				  xf86Info.mseAvail);
		xf86Info.mseAvail = 0;
	}
}

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_video.c,v 3.14.2.4 1998/06/05 16:23:12 dawes Exp $ */
/*
 * Copyright 1992 by Orest Zborowski <obz@Kodak.com>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Orest Zborowski and David Wexelblat
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  Orest Zborowski
 * and David Wexelblat make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * OREST ZBOROWSKI AND DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL OREST ZBOROWSKI OR DAVID WEXELBLAT BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: lnx_video.c /main/9 1996/10/19 18:06:34 kaleb $ */

#include "X.h"
#include "input.h"
#include "scrnintstr.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

static Bool ExtendedEnabled = FALSE;

#ifdef __alpha__

/*
 * The Jensen lacks dense memory, thus we have to address the bus via
 * the sparse addressing scheme.
 *
 * Martin Ostermann (ost@comnets.rwth-aachen.de) - Apr.-Sep. 1996
 */

#ifdef TEST_JENSEN_CODE /* define to test the Sparse addressing on a non-Jensen */
#define SPARSE (5)
#define isJensen (1)
#else
#define isJensen (!_bus_base())
#define SPARSE (7)
#endif

#define BUS_BASE (isJensen ? _bus_base_sparse() : _bus_base())
#define JENSEN_SHIFT(x) (isJensen ? ((long)x<<SPARSE) : (long)x)
#else /* ! __alpha__ */
#define BUS_BASE 0
#define JENSEN_SHIFT(x) (x)
#endif /* ! __alpha__ */

/***************************************************************************/
/* Video Memory Mapping section                                            */
/***************************************************************************/

pointer
xf86MapVidMem(int ScreenNum, int Flags, pointer Base, unsigned long Size)
{
	pointer base;
      	int fd;

	if ((fd = open("/dev/mem", O_RDWR)) < 0)
	{
		FatalError("xf86MapVidMem: failed to open /dev/mem (%s)\n",
			   strerror(errno));
	}
	/* This requires linux-0.99.pl10 or above */
	base = (pointer)mmap((caddr_t)0, JENSEN_SHIFT(Size),
			     PROT_READ|PROT_WRITE,
			     MAP_SHARED, fd,
			     (off_t)(JENSEN_SHIFT((off_t)Base) + BUS_BASE));
	close(fd);
	if ((long)base == -1)
	{
		FatalError("xf86MapVidMem: Could not mmap framebuffer (%s)\n",
			   strerror(errno));
	}
	return base;
}

void
xf86UnMapVidMem(int ScreenNum, pointer Base, unsigned long Size)
{
    munmap((caddr_t)JENSEN_SHIFT(Base), JENSEN_SHIFT(Size));
}

Bool
xf86LinearVidMem()
{
	return(TRUE);
}

/***************************************************************************/
/* I/O Permissions section                                                 */
/***************************************************************************/

void
xf86EnableIO(void)
{
	if (ExtendedEnabled)
		return;

#ifndef __mc68000__
	if (iopl(3))
		FatalError("%s: Failed to set IOPL for I/O\n",
			   "xf86EnableIOPorts");
#endif
	ExtendedEnabled = TRUE;

	return;
}

void
xf86DisableIO(void)
{
	if (!ExtendedEnabled)
		return;

#ifndef __mc68000__
	iopl(0);
#endif
	ExtendedEnabled = FALSE;

	return;
}


/***************************************************************************/
/* Interrupt Handling section                                              */
/***************************************************************************/

Bool
xf86DisableInterrupts()
{
	if (!ExtendedEnabled)
#ifndef __mc68000__
		if (iopl(3))
			return (FALSE);
#endif
#if defined(__alpha__) || defined(__mc68000__)
#else
#ifdef __GNUC__
	__asm__ __volatile__("cli");
#else
	asm("cli");
#endif
#endif
#ifndef __mc68000__
	if (!ExtendedEnabled)
		iopl(0);
#endif
	return (TRUE);
}

void
xf86EnableInterrupts()
{
	if (!ExtendedEnabled)
#ifndef __mc68000__
		if (iopl(3))
			return;
#endif
#if defined(__alpha__) || defined(__mc68000__)
#else
#ifdef __GNUC__
	__asm__ __volatile__("sti");
#else
	asm("sti");
#endif
#endif
#ifndef __mc68000__
	if (!ExtendedEnabled)
		iopl(0);
#endif
	return;
}

#if defined(__alpha__)

static int xf86SparseShift = 5; /* default to all but JENSEN */

pointer
xf86MapVidMemSparse(int ScreenNum, int Flags, pointer Base, unsigned long Size)
{
	pointer base;
      	int fd;

	if (!_bus_base()) xf86SparseShift = 7; /* Uh, oh, JENSEN... */

	Size <<= xf86SparseShift;
	Base = (pointer)((unsigned long)Base << xf86SparseShift);

	if ((fd = open("/dev/mem", O_RDWR)) < 0)
	{
		FatalError("xf86MapVidMem: failed to open /dev/mem (%s)\n",
			   strerror(errno));
	}
	/* This requirers linux-0.99.pl10 or above */
	base = (pointer)mmap((caddr_t)0, Size,
			     PROT_READ | PROT_WRITE,
			     MAP_SHARED, fd,
			     (off_t)Base + _bus_base_sparse());
	close(fd);
	if ((long)base == -1)
	{
		FatalError("xf86MapVidMem: Could not mmap framebuffer (%s)\n",
			   strerror(errno));
	}
	return base;
}

void
xf86UnMapVidMemSparse(int ScreenNum, pointer Base, unsigned long Size)
{
	Size <<= xf86SparseShift;

	munmap((caddr_t)Base, Size);
}

#define vuip    volatile unsigned int *

extern void sethae(unsigned long hae);

int
xf86ReadSparse8(pointer Base, unsigned long Offset)
{
    unsigned long result, shift;
    unsigned long msb = 0;

    shift = (Offset & 0x3) * 8;
    if (xf86SparseShift != 7) { /* if not JENSEN, we may need HAE */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000UL;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    result = *(vuip) ((unsigned long)Base + (Offset << xf86SparseShift));
    if (msb)
      sethae(0);
    result >>= shift;
    return 0xffUL & result;
}

int
xf86ReadSparse16(pointer Base, unsigned long Offset)
{
    unsigned long result, shift;
    unsigned long msb = 0;

    shift = (Offset & 0x2) * 8;
    if (xf86SparseShift != 7) { /* if not JENSEN, we may need HAE */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000UL;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    result = *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(1<<(xf86SparseShift-2)));
    if (msb)
      sethae(0);
    result >>= shift;
    return 0xffffUL & result;
}

int
xf86ReadSparse32(pointer Base, unsigned long Offset)
{
    unsigned long result;
    unsigned long msb = 0;

    if (xf86SparseShift != 7) { /* if not JENSEN, we may need HAE */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000UL;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    result = *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(3<<(xf86SparseShift-2)));
    if (msb)
      sethae(0);
    return result;
}

void
xf86WriteSparse8(int Value, pointer Base, unsigned long Offset)
{
    unsigned long msb = 0;
    unsigned int b = Value & 0xffU;

    if (xf86SparseShift != 7) { /* not JENSEN */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    *(vuip) ((unsigned long)Base + (Offset << xf86SparseShift)) = b * 0x01010101;
    if (msb)
      sethae(0);
}

void
xf86WriteSparse16(int Value, pointer Base, unsigned long Offset)
{
    unsigned long msb = 0;
    unsigned int w = Value & 0xffffU;

    if (xf86SparseShift != 7) { /* not JENSEN */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(1<<(xf86SparseShift-2))) =
      w * 0x00010001;
    if (msb)
      sethae(0);
}

void
xf86WriteSparse32(int Value, pointer Base, unsigned long Offset)
{
    unsigned long msb = 0;

    if (xf86SparseShift != 7) { /* not JENSEN */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000;
        Offset -= msb;
        if (msb) {
	  sethae(msb);
        }
      }
    }
    *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(3<<(xf86SparseShift-2))) = Value;
    if (msb)
      sethae(0);
}
#endif /* __alpha__ */

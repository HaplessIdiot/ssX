/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_video.c,v 3.24 1999/04/18 04:08:52 dawes Exp $ */
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
#include "xf86OSpriv.h"

#include "compiler.h"

#ifdef HAS_MTRR_SUPPORT
#include <asm/mtrr.h>
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

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

static pointer mapVidMem(int, unsigned long, unsigned long);
static void unmapVidMem(int, pointer, unsigned long);
#ifdef __alpha__
static pointer mapVidMemSparse(int, unsigned long, unsigned long);
static void unmapVidMemSparse(int, pointer, unsigned long);
#endif

#ifdef HAS_MTRR_SUPPORT
static pointer setWC(int, unsigned long, unsigned long, Bool, MessageType);
static void undoWC(int, pointer);

/* The file desc for /proc/mtrr. Once opened, left opened, and the mtrr
   driver will clean up when we exit. */
#define MTRR_FD_UNOPENED (-1)	/* We have yet to open /proc/mtrr */
#define MTRR_FD_PROBLEM (-2)	/* We tried to open /proc/mtrr, but had
				   a problem. */
static int mtrr_fd = MTRR_FD_UNOPENED;

/* Open /proc/mtrr. FALSE on failure. Will always fail on Linux 2.0, 
   and will fail on Linux 2.2 with MTRR support configured out,
   so verbosity should be chosen appropriately. */
static Bool
mtrr_open(int verbosity)
{
	/* Only report absence of /proc/mtrr once. */
	static Bool warned = FALSE;

	char **fn;
	static char *mtrr_files[] = {
		"/dev/cpu/mtrr",	/* Possible future name */
		"/proc/mtrr",		/* Current name */
		NULL
	};

	if (mtrr_fd == MTRR_FD_UNOPENED) { 
		/* So open it. */
		for (fn = mtrr_files; mtrr_fd < 0 && *fn; fn++)
			mtrr_fd = open(*fn, O_WRONLY);

		if (mtrr_fd < 0)
			mtrr_fd = MTRR_FD_PROBLEM;
	}

	if (mtrr_fd == MTRR_FD_PROBLEM) {
		/* To make sure we only ever warn once, need to check
		   verbosity outside xf86MsgVerb */
		if (!warned && verbosity <= xf86GetVerbosity()) {
			xf86MsgVerb(X_WARNING, verbosity,
				  "System lacks support for changing MTRRs\n");
			warned = TRUE;
		}

		return FALSE;
	}
	else
		return TRUE;
}

/*
 * We maintain a list of WC regions for each physical mapping so they can
 * be undone when unmapping.
 */

struct mtrr_wc_region {
	struct mtrr_sentry	sentry;
	Bool			added;		/* added WC or removed it */
	struct mtrr_wc_region *	next;
};

static struct mtrr_wc_region *
mtrr_cull_wc_region(int screenNum, unsigned long base, unsigned long size,
		      MessageType from)
{
	/* Some BIOS writers thought that setting wc over the mmio
	   region of a graphics devices was a good idea. Try to fix
	   it. */

	struct mtrr_gentry gent;
	char buf[20];
	struct mtrr_wc_region *wcreturn = NULL, *wcr;

	/* Linux 2.0 users should not get a warning without -verbose */
	if (!mtrr_open(2))
		return NULL;

	for (gent.regnum = 0; 
	     ioctl(mtrr_fd, MTRRIOC_GET_ENTRY, &gent) >= 0;
	     gent.regnum++) {
		if (gent.type != MTRR_TYPE_WRCOMB
		    || gent.base + gent.size <= base
		    || base + size <= gent.base)
			continue;

		/* Found an overlapping region. Delete it. */
		
		wcr = xalloc(sizeof(*wcr));
		if (!wcr)
			return NULL;
		wcr->sentry.base = gent.base;
		wcr->sentry.size = gent.size;
		wcr->sentry.type = MTRR_TYPE_WRCOMB;
		wcr->added = FALSE;
		
		/* There is now a nicer ioctl-based way to do this,
		   but it isn't in current kernels. */
		snprintf(buf, sizeof(buf), "disable=%u\n", gent.regnum);

		if (write(mtrr_fd, buf, strlen(buf)) >= 0) {
			xf86DrvMsg(screenNum, from,
				   "Removed MMIO write-combining range "
				   "(0x%lx,0x%lx)\n",
				   gent.base, gent.size);
			wcr->next = wcreturn;
			wcreturn = wcr;
		} else {
			xfree(wcr);
			xf86DrvMsgVerb(screenNum, X_WARNING, 0,
				   "Failed to remove MMIO "
				   "write-combining range (0x%lx,0x%lx)\n",
				   gent.base, gent.size);
		}
	}
	return wcreturn;
}


static struct mtrr_wc_region *
mtrr_add_wc_region(int screenNum, unsigned long base, unsigned long size,
		   MessageType from)
{
	struct mtrr_wc_region *wcr;

	/* Linux 2.0 should not warn, unless the user explicitly asks for
	   WC. */
	if (!mtrr_open(from == X_CONFIG ? 0 : 2))
		return NULL;

	wcr = xalloc(sizeof(*wcr));
	if (!wcr)
		return NULL;

	wcr->sentry.base = base;
	wcr->sentry.size = size;
	wcr->sentry.type = MTRR_TYPE_WRCOMB;
	wcr->added = TRUE;
	wcr->next = NULL;

	if (ioctl(mtrr_fd, MTRRIOC_ADD_ENTRY, &wcr->sentry) >= 0) {
		/* Avoid printing on every VT switch */
		if (xf86ServerIsInitialising()) {
			xf86DrvMsg(screenNum, from,
				   "Write-combining range (0x%lx,0x%lx)\n",
				   base, size);
		}
		return wcr;
	}
	else {
		xfree(wcr);
		
		/* Don't complain about the VGA region: MTRR fixed
		   regions aren't currently supported, but might be in
		   the future. */
		if ((unsigned long)base >= 0x100000) {
			xf86DrvMsgVerb(screenNum, X_WARNING, 0,
				"Failed to set up write-combining range "
				"(0x%lx,0x%lx)\n", base, size);
		}
		return NULL;
	}
}

static void
mtrr_undo_wc_region(int screenNum, struct mtrr_wc_region *wcr)
{
	struct mtrr_wc_region *p, *prev;

	if (mtrr_fd > 0) {
		p = wcr;
		while (p) {
			if (p->added)
				ioctl(mtrr_fd, MTRRIOC_DEL_ENTRY, &p->sentry);
			prev = p;
			p = p->next;
			xfree(prev);
		}
	}
}

static pointer
setWC(int screenNum, unsigned long base, unsigned long size, Bool enable,
      MessageType from)
{
	if (enable)
		return mtrr_add_wc_region(screenNum, base, size, from);
	else
		return mtrr_cull_wc_region(screenNum, base, size, from);
}

static void
undoWC(int screenNum, pointer regioninfo)
{
	mtrr_undo_wc_region(screenNum, regioninfo);
}

#endif /* HAS_MTRR_SUPPORT */


void
xf86OSInitVidMem(VidMemInfoPtr pVidMem)
{
	pVidMem->linearSupported = TRUE;
	pVidMem->mapMem = mapVidMem;
	pVidMem->unmapMem = unmapVidMem;
#ifdef __alpha__
	pVidMem->mapMemSparse = mapVidMemSparse;
	pVidMem->unmapMemSparse = unmapVidMemSparse;
#endif
#ifdef HAS_MTRR_SUPPORT
	pVidMem->setWC = setWC;
	pVidMem->undoWC = undoWC;
#endif
	pVidMem->initialised = TRUE;
}

static pointer
mapVidMem(int ScreenNum, unsigned long Base, unsigned long Size)
{
	pointer base;
      	int fd;

	if ((fd = open("/dev/mem", O_RDWR)) < 0)
	{
		FatalError("xf86MapVidMem: failed to open /dev/mem (%s)\n",
			   strerror(errno));
	}
	/* This requires linux-0.99.pl10 or above */
	base = mmap((caddr_t)0, JENSEN_SHIFT(Size),
		     PROT_READ|PROT_WRITE,
		     MAP_SHARED, fd,
		     (off_t)(JENSEN_SHIFT((off_t)Base) + BUS_BASE));
	close(fd);
	if (base == MAP_FAILED)
	{
		FatalError("xf86MapVidMem: Could not mmap framebuffer"
			   " (0x%08x,0x%x) (%s)\n", Base, Size,
			   strerror(errno));
	}

	return base;
}

static void
unmapVidMem(int ScreenNum, pointer Base, unsigned long Size)
{
	munmap((caddr_t)JENSEN_SHIFT(Base), JENSEN_SHIFT(Size));
}

/***************************************************************************/
/* I/O Permissions section                                                 */
/***************************************************************************/

#if defined(__powerpc__)
/* FIXME: init this... */
volatile unsigned char *ioBase = MAP_FAILED;

void
ppc_flush_icache(char *addr)
{
	/* cut+paste from linux glibc */
	__asm__ volatile (" dcbf 0,3");
	__asm__ volatile (" icbi 0,3");
	__asm__ volatile (" addi 3,3,32");
	__asm__ volatile (" dcbf 0,3");
	__asm__ volatile (" icbi 0,3");
}
#endif

void
xf86EnableIO(void)
{
	if (ExtendedEnabled)
		return;

#if !defined(__mc68000__) && !defined(__powerpc__) && !defined(__sparc__)
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

#if !defined(__mc68000__) && !defined(__powerpc__) && !defined(__sparc__)
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
#if !defined(__mc68000__) && !defined(__powerpc__) && !defined(__sparc__)
		if (iopl(3))
			return (FALSE);
#endif
#if defined(__alpha__) || defined(__mc68000__) || defined(__powerpc__) || defined(__sparc__)
#else
#ifdef __GNUC__
	__asm__ __volatile__("cli");
#else
	asm("cli");
#endif
#endif
#if !defined(__mc68000__) && !defined(__powerpc__) && !defined(__sparc__)
	if (!ExtendedEnabled)
		iopl(0);
#endif
	return (TRUE);
}

void
xf86EnableInterrupts()
{
	if (!ExtendedEnabled)
#if !defined(__mc68000__) && !defined(__powerpc__) && !defined(__sparc__)
		if (iopl(3))
			return;
#endif
#if defined(__alpha__) || defined(__mc68000__) || defined(__powerpc__) || defined(__sparc__)
#else
#ifdef __GNUC__
	__asm__ __volatile__("sti");
#else
	asm("sti");
#endif
#endif
#if !defined(__mc68000__) && !defined(__powerpc__) && !defined(__sparc__)
	if (!ExtendedEnabled)
		iopl(0);
#endif
	return;
}

#if defined(__alpha__)

static int xf86SparseShift = 5; /* default to all but JENSEN */

static pointer
mapVidMemSparse(int ScreenNum, unsigned long Base, unsigned long Size)
{
	pointer base;
      	int fd;

	if (!_bus_base()) xf86SparseShift = 7; /* Uh, oh, JENSEN... */
	if (!_bus_base_sparse()) xf86SparseShift = 0; /* no sparse  */
	
	Size <<= xf86SparseShift;
	Base = Base << xf86SparseShift;

	if ((fd = open("/dev/mem", O_RDWR)) < 0)
	{
		FatalError("xf86MapVidMem: failed to open /dev/mem (%s)\n",
			   strerror(errno));
	}
	/* This requirers linux-0.99.pl10 or above */
	base = mmap((caddr_t)0, Size,
		     PROT_READ | PROT_WRITE,
		     MAP_SHARED, fd,
		     (off_t)Base + _bus_base_sparse());
	close(fd);
	if (base == MAP_FAILED)
	{
		FatalError("xf86MapVidMem: Could not mmap framebuffer (%s)\n",
			   strerror(errno));
	}
	return base;
}

static void
unmapVidMemSparse(int ScreenNum, pointer Base, unsigned long Size)
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
        msb = Offset & 0xf8000000UL;
        if (msb) {
	    Offset -= msb;
	    sethae(msb);
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
        msb = Offset & 0xf8000000UL;
        if (msb) {
	    Offset -= msb;
	    sethae(msb);
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
        msb = Offset & 0xf8000000UL;
        if (msb) {
	    Offset -= msb;
	    sethae(msb);
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
        msb = Offset & 0xf8000000;
        if (msb) {
	    Offset -= msb;
	    sethae(msb);
        }
    }

    *(vuip) ((unsigned long)Base + (Offset << xf86SparseShift)) = b * 0x01010101;
    if (msb)
      sethae(0);

#if defined(__alpha__)
    /* CAUTION: if you make changes inside this block you need to */
    /* make them to the other two identical blocks in this file   */
    mem_barrier();
#endif

}

void
xf86WriteSparse16(int Value, pointer Base, unsigned long Offset)
{
    unsigned long msb = 0;
    unsigned int w = Value & 0xffffU;

    if (xf86SparseShift != 7) { /* not JENSEN */
        msb = Offset & 0xf8000000;
        if (msb) {
	    Offset -= msb;
	    sethae(msb);
        }
    }

    *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(1<<(xf86SparseShift-2))) =
      w * 0x00010001;
    if (msb)
      sethae(0);

#if defined(__alpha__)
    /* CAUTION: if you make changes inside this block you need to */
    /* make them to the other two identical blocks in this file   */
    mem_barrier();
#endif

}

void
xf86WriteSparse32(int Value, pointer Base, unsigned long Offset)
{
    unsigned long msb = 0;

    if (xf86SparseShift != 7) { /* not JENSEN */
        msb = Offset & 0xf8000000;
        if (msb) {
	    Offset -= msb;
	    sethae(msb);
        }
    }
    
    *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(3<<(xf86SparseShift-2))) = Value;
    if (msb)
      sethae(0);

#if defined(__alpha__)
    /* CAUTION: if you make changes inside this block you need to */
    /* make them to the other two identical blocks in this file   */
    mem_barrier();
#endif
}
#endif /* __alpha__ */

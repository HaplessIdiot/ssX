/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_video.c,v 3.18 1999/02/19 21:27:05 hohndel Exp $ */
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

#include "compiler.h"

#ifdef HAS_MTRR_SUPPORT
#include <asm/mtrr.h>
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

/*
 * Get a piece of the ScrnInfoRec.  At the moment, this is only used to hold
 * the MTRR option information, but it is likely to be expanded if we do
 * auto unmapping of memory at VT switch.
 *
 * XXX This might be better located in an OS-independent front-end to the
 * VidMap functions.
 */

static int vidMapIndex = -1;
typedef struct {
	Bool mtrrEnabled;
	MessageType mtrrFrom;
	Bool mtrrOptChecked;
} VidMapRec, *VidMapPtr;

#define VIDMAPPTR(p) ((VidMapPtr)((p)->privates[vidMapIndex].ptr))

static VidMapPtr
getVidMapRec(int scrnIndex)
{
	VidMapPtr vp;

	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	if (vidMapIndex < 0)
		vidMapIndex = xf86AllocateScrnInfoPrivateIndex();

	if (VIDMAPPTR(pScrn) != NULL)
		return VIDMAPPTR(pScrn);

	vp = pScrn->privates[vidMapIndex].ptr = xnfcalloc(sizeof(VidMapRec), 1);
	vp->mtrrEnabled = TRUE;	/* default to enabled */
	vp->mtrrFrom = X_DEFAULT;
	vp->mtrrOptChecked = FALSE;
	return vp;
}

#ifdef HAS_MTRR_SUPPORT
/* The file desc for /proc/mtrr. Once opened, left opened, and the mtrr
   driver will clean up when we exit. */
#define MTRR_FD_UNOPENED (-1)	/* We have yet to open /proc/mtrr */
#define MTRR_FD_PROBLEM (-2)	/* We tried to open /proc/mtrr, but had
				   a problem. */
static int mtrr_fd = MTRR_FD_UNOPENED;

static void
mtrr_write_combining_region(ScrnInfoPtr scrn, pointer base, unsigned long size)
{
	static const char *mtrr_files[] = {
		"/dev/cpu/mtrr",	/* Possible future name */
		"/proc/mtrr",		/* Current name */
		NULL
	};

	struct mtrr_sentry sentry;
	const char **fn;
	VidMapPtr vp;

	switch (mtrr_fd) {
	case MTRR_FD_PROBLEM:
		break;
		
	case MTRR_FD_UNOPENED:
		/* So open it. */
		for (fn = mtrr_files; mtrr_fd < 0 && *fn; fn++) {
			mtrr_fd = open(*fn, O_WRONLY, 0);
			if (mtrr_fd < 0 && errno != ENOENT) {
				xf86MsgVerb(X_WARNING, 0
					    "Error opening %s: %s\n"
					    *fn, strerror(errno));
				break;
			}
		}

		if (mtrr_fd < 0) {
			if (errno == ENOENT)
				xf86MsgVerb(X_WARNING, 0,
				    "System lacks write combining support\n");
			
			mtrr_fd = MTRR_FD_PROBLEM;
			break;
		}
		
		/* FALL THROUGH */
	default:
		sentry.base = (unsigned long) base;
		sentry.size = size;
		sentry.type = MTRR_TYPE_WRCOMB;
		
		if (ioctl(mtrr_fd, MTRRIOC_ADD_ENTRY, &sentry) < 0) {
			xf86DrvMsgVerb(scrn->scrnIndex, X_WARNING, 0,
				"Failed to set up write-combining range "
				"(0x%lx,0x%lx)\n", (long)base, (long)size);
		} else {
			vp = VIDMAPPTR(scrn);
			xf86DrvMsg(scrn->scrnIndex, vp->mtrrFrom,
				"Write-combining range (0x%lx,0x%lx)\n",
				(long)base, (long)size);
		}
	}
}
#endif


pointer
xf86MapVidMem(int ScreenNum, int Flags, pointer Base, unsigned long Size)
{
	pointer base;
      	int fd;
	VidMapPtr vp;

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

	/*
	 * Check the "mtrr" option even when MTRR isn't supported to avoid
	 * warnings about unrecognised options.
	 */
	/*
	 * XXX Maybe some functions like this should be divided into
	 * OS-dependent and OS-independent parts.  The option checking could
	 * go into the OS-independent part.
	 */
	vp = getVidMapRec(ScreenNum);
	if (!vp->mtrrOptChecked && xf86Screens[ScreenNum]->options != NULL)
	{
		enum { OPTION_MTRR };
		static OptionInfoRec opts[] =
		{
			{ OPTION_MTRR, "mtrr", OPTV_TRI, {0}, FALSE },
			{ -1, NULL, OPTV_NONE, {0}, FALSE }
		};
		/*
		 * We get called once for each screen, so reset
		 * the OptionInfoRecs.
		 */
		opts[0].found = FALSE;

		xf86ProcessOptions(ScreenNum, xf86Screens[ScreenNum]->options,
				   opts);
		if (xf86GetOptValBool(opts, OPTION_MTRR, &vp->mtrrEnabled))
			vp->mtrrFrom = X_CONFIG;
		vp->mtrrOptChecked = TRUE;
	}

#ifdef HAS_MTRR_SUPPORT
	if (vp->mtrrEnabled)
	{
		if (Flags & VIDMEM_FRAMEBUFFER)
			mtrr_write_combining_region(xf86Screens[ScreenNum],
						    Base, Size);
		/* XXX add code to make sure WC is off for VIDMEM_MMIO */
	}
#endif

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

#if defined(__powerpc__)
/* FIXME: init this... */
volatile unsigned char *ioBase = MAP_FAILED;

/* FIXME: dummy function ... */
void ppc_flush_icache(char *addr) {};
#endif

void
xf86EnableIO(void)
{
	if (ExtendedEnabled)
		return;

#if !defined(__mc68000__) && !defined(__powerpc__)
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

#if !defined(__mc68000__) && !defined(__powerpc__)
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
#if !defined(__mc68000__) && !defined(__powerpc__)
		if (iopl(3))
			return (FALSE);
#endif
#if defined(__alpha__) || defined(__mc68000__) || defined(__powerpc__)
#else
#ifdef __GNUC__
	__asm__ __volatile__("cli");
#else
	asm("cli");
#endif
#endif
#if !defined(__mc68000__) && !defined(__powerpc__)
	if (!ExtendedEnabled)
		iopl(0);
#endif
	return (TRUE);
}

void
xf86EnableInterrupts()
{
	if (!ExtendedEnabled)
#if !defined(__mc68000__) && !defined(__powerpc__)
		if (iopl(3))
			return;
#endif
#if defined(__alpha__) || defined(__mc68000__) || defined(__powerpc__)
#else
#ifdef __GNUC__
	__asm__ __volatile__("sti");
#else
	asm("sti");
#endif
#endif
#if !defined(__mc68000__) && !defined(__powerpc__)
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

#if defined(__alpha__)
    /* CAUTION: if you make changes inside this block you need to */
    /* make them to the other two identical blocks in this file   */
    mem_barrier();
#endif
}
#endif /* __alpha__ */

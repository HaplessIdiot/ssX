/* $XFree86: xc/programs/Xserver/hw/xfree86/vgahw/vgaHW.c,v 1.10 1998/11/01 12:36:05 dawes Exp $ */

/*
 *
 * Copyright 1991-1998 by The XFree86 Project, Inc.
 *
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 */

#if !defined(AMOEBA) && !defined(MINIX)
#define _NEED_SYSI86
#endif

#include "X.h"
#include "misc.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "vgaHW.h"

#include "xf86cmap.h"

/*
 * XXX The PC98 bits have been removed for now.  The structure of the
 * code here has been reorganised to the point where they need to be
 * redone anyway.  In the meantime the older version can be found in
 * xfree86/olddrivers/vgahw/.
 */


#ifndef SAVE_FONT1
#define SAVE_FONT1
#endif

#if defined(Lynx) || defined(CSRG_BASED) || defined(MACH386) || defined(linux) || defined(AMOEBA) || defined(MINIX)
#ifndef NEED_SAVED_CMAP
#define NEED_SAVED_CMAP
#endif
#ifndef SAVE_TEXT
#define SAVE_TEXT
#endif
#ifndef SAVE_FONT2
#define SAVE_FONT2
#endif
#endif

/* bytes per plane to save for text */
#define TEXT_AMOUNT 16384

/* bytes per plane to save for font data */
#define FONT_AMOUNT 8192

#if 0
/* Override all of these for now */
#undef SAVE_FONT1
#define SAVE_FONT1
#undef SAVE_FONT2
#define SAVE_FONT2
#undef SAVE_TEST
#define SAVE_TEST
#undef FONT_AMOUNT
#define FONT_AMOUNT 65536
#undef TEXT_AMOUNT
#define TEXT_AMOUNT 65536
#endif

/* DAC indices for white and black */
#define WHITE_VALUE 0x3F
#define BLACK_VALUE 0x00
#define OVERSCAN_VALUE 0x01


/* Use a private definition of this here */
#undef VGAHWPTR
#define VGAHWPTRLVAL(p) (p)->privates[vgaHWPrivateIndex].ptr
#define VGAHWPTR(p) ((vgaHWPtr)(VGAHWPTRLVAL(p)))

static int vgaHWPrivateIndex = -1;

#define DAC_TEST_MASK 0x3F

#ifdef NEED_SAVED_CMAP
/* This default colourmap is used only when it can't be read from the VGA */

static CARD8 defaultDAC[768] =
{
     0,  0,  0,    0,  0, 42,    0, 42,  0,    0, 42, 42,
    42,  0,  0,   42,  0, 42,   42, 21,  0,   42, 42, 42,
    21, 21, 21,   21, 21, 63,   21, 63, 21,   21, 63, 63,
    63, 21, 21,   63, 21, 63,   63, 63, 21,   63, 63, 63,
     0,  0,  0,    5,  5,  5,    8,  8,  8,   11, 11, 11,
    14, 14, 14,   17, 17, 17,   20, 20, 20,   24, 24, 24,
    28, 28, 28,   32, 32, 32,   36, 36, 36,   40, 40, 40,
    45, 45, 45,   50, 50, 50,   56, 56, 56,   63, 63, 63,
     0,  0, 63,   16,  0, 63,   31,  0, 63,   47,  0, 63,
    63,  0, 63,   63,  0, 47,   63,  0, 31,   63,  0, 16,
    63,  0,  0,   63, 16,  0,   63, 31,  0,   63, 47,  0,
    63, 63,  0,   47, 63,  0,   31, 63,  0,   16, 63,  0,
     0, 63,  0,    0, 63, 16,    0, 63, 31,    0, 63, 47,
     0, 63, 63,    0, 47, 63,    0, 31, 63,    0, 16, 63,
    31, 31, 63,   39, 31, 63,   47, 31, 63,   55, 31, 63,
    63, 31, 63,   63, 31, 55,   63, 31, 47,   63, 31, 39,
    63, 31, 31,   63, 39, 31,   63, 47, 31,   63, 55, 31,
    63, 63, 31,   55, 63, 31,   47, 63, 31,   39, 63, 31,
    31, 63, 31,   31, 63, 39,   31, 63, 47,   31, 63, 55,
    31, 63, 63,   31, 55, 63,   31, 47, 63,   31, 39, 63,
    45, 45, 63,   49, 45, 63,   54, 45, 63,   58, 45, 63,
    63, 45, 63,   63, 45, 58,   63, 45, 54,   63, 45, 49,
    63, 45, 45,   63, 49, 45,   63, 54, 45,   63, 58, 45,
    63, 63, 45,   58, 63, 45,   54, 63, 45,   49, 63, 45,
    45, 63, 45,   45, 63, 49,   45, 63, 54,   45, 63, 58,
    45, 63, 63,   45, 58, 63,   45, 54, 63,   45, 49, 63,
     0,  0, 28,    7,  0, 28,   14,  0, 28,   21,  0, 28,
    28,  0, 28,   28,  0, 21,   28,  0, 14,   28,  0,  7,
    28,  0,  0,   28,  7,  0,   28, 14,  0,   28, 21,  0,
    28, 28,  0,   21, 28,  0,   14, 28,  0,    7, 28,  0,
     0, 28,  0,    0, 28,  7,    0, 28, 14,    0, 28, 21,
     0, 28, 28,    0, 21, 28,    0, 14, 28,    0,  7, 28,
    14, 14, 28,   17, 14, 28,   21, 14, 28,   24, 14, 28,
    28, 14, 28,   28, 14, 24,   28, 14, 21,   28, 14, 17,
    28, 14, 14,   28, 17, 14,   28, 21, 14,   28, 24, 14,
    28, 28, 14,   24, 28, 14,   21, 28, 14,   17, 28, 14,
    14, 28, 14,   14, 28, 17,   14, 28, 21,   14, 28, 24,
    14, 28, 28,   14, 24, 28,   14, 21, 28,   14, 17, 28,
    20, 20, 28,   22, 20, 28,   24, 20, 28,   26, 20, 28,
    28, 20, 28,   28, 20, 26,   28, 20, 24,   28, 20, 22,
    28, 20, 20,   28, 22, 20,   28, 24, 20,   28, 26, 20,
    28, 28, 20,   26, 28, 20,   24, 28, 20,   22, 28, 20,
    20, 28, 20,   20, 28, 22,   20, 28, 24,   20, 28, 26,
    20, 28, 28,   20, 26, 28,   20, 24, 28,   20, 22, 28,
     0,  0, 16,    4,  0, 16,    8,  0, 16,   12,  0, 16,
    16,  0, 16,   16,  0, 12,   16,  0,  8,   16,  0,  4,
    16,  0,  0,   16,  4,  0,   16,  8,  0,   16, 12,  0,
    16, 16,  0,   12, 16,  0,    8, 16,  0,    4, 16,  0,
     0, 16,  0,    0, 16,  4,    0, 16,  8,    0, 16, 12,
     0, 16, 16,    0, 12, 16,    0,  8, 16,    0,  4, 16,
     8,  8, 16,   10,  8, 16,   12,  8, 16,   14,  8, 16,
    16,  8, 16,   16,  8, 14,   16,  8, 12,   16,  8, 10,
    16,  8,  8,   16, 10,  8,   16, 12,  8,   16, 14,  8,
    16, 16,  8,   14, 16,  8,   12, 16,  8,   10, 16,  8,
     8, 16,  8,    8, 16, 10,    8, 16, 12,    8, 16, 14,
     8, 16, 16,    8, 14, 16,    8, 12, 16,    8, 10, 16,
    11, 11, 16,   12, 11, 16,   13, 11, 16,   15, 11, 16,
    16, 11, 16,   16, 11, 15,   16, 11, 13,   16, 11, 12,
    16, 11, 11,   16, 12, 11,   16, 13, 11,   16, 15, 11,
    16, 16, 11,   15, 16, 11,   13, 16, 11,   12, 16, 11,
    11, 16, 11,   11, 16, 12,   11, 16, 13,   11, 16, 15,
    11, 16, 16,   11, 15, 16,   11, 13, 16,   11, 12, 16,
     0,  0,  0,    0,  0,  0,    0,  0,  0,    0,  0,  0,
     0,  0,  0,    0,  0,  0,    0,  0,  0,    0,  0,  0,
};
#endif /* NEED_SAVED_CMAP */

/*
 * With Intel, the version in os-support/misc/SlowBcopy.s is used.
 * This avoids port I/O during the copy (which causes problems with
 * some hardware).
 */
#ifdef __alpha__
#define slowbcopy_tobus(src,dst,count) xf86SlowBCopyToBus(src,dst,count)
#define slowbcopy_frombus(src,dst,count) xf86SlowBCopyFromBus(src,dst,count)
#else /* __alpha__ */
#define slowbcopy_tobus(src,dst,count) xf86SlowBcopy(src,dst,count)
#define slowbcopy_frombus(src,dst,count) xf86SlowBcopy(src,dst,count)
#endif /* __alpha__ */


/*
 * Standard VGA versions of the register access functions.
 */
static void
stdWriteCrtc(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    outb(hwp->IOBase + VGA_CRTC_INDEX_OFFSET, index);
    outb(hwp->IOBase + VGA_CRTC_DATA_OFFSET, value);
}

static CARD8
stdReadCrtc(vgaHWPtr hwp, CARD8 index)
{
    outb(hwp->IOBase + VGA_CRTC_INDEX_OFFSET, index);
    return inb(hwp->IOBase + VGA_CRTC_DATA_OFFSET);
}

static void
stdWriteGr(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    outb(VGA_GRAPH_INDEX, index);
    outb(VGA_GRAPH_DATA, value);
}

static CARD8
stdReadGr(vgaHWPtr hwp, CARD8 index)
{
    outb(VGA_GRAPH_INDEX, index);
    return inb(VGA_GRAPH_DATA);
}

static void
stdWriteSeq(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    outb(VGA_SEQ_INDEX, index);
    outb(VGA_SEQ_DATA, value);
}

static CARD8
stdReadSeq(vgaHWPtr hwp, CARD8 index)
{
    outb(VGA_SEQ_INDEX, index);
    return inb(VGA_SEQ_DATA);
}

static void
stdWriteAttr(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    CARD8 tmp;

    if (hwp->paletteEnabled)
	index &= ~0x20;
    else
	index |= 0x20;

    tmp = inb(hwp->IOBase + VGA_IN_STAT_1_OFFSET);
    outb(VGA_ATTR_INDEX, index);
    outb(VGA_ATTR_DATA_W, value);
}

static CARD8
stdReadAttr(vgaHWPtr hwp, CARD8 index)
{
    CARD8 tmp;

    if (hwp->paletteEnabled)
	index &= ~0x20;
    else
	index |= 0x20;

    tmp = inb(hwp->IOBase + VGA_IN_STAT_1_OFFSET);
    outb(VGA_ATTR_INDEX, index);
    return inb(VGA_ATTR_DATA_R);
}

static void
stdWriteMiscOut(vgaHWPtr hwp, CARD8 value)
{
    outb(VGA_MISC_OUT_W, value);
}

static CARD8
stdReadMiscOut(vgaHWPtr hwp)
{
    return inb(VGA_MISC_OUT_R);
}

static void
stdEnablePalette(vgaHWPtr hwp)
{
    CARD8 tmp;

    tmp = inb(hwp->IOBase + VGA_IN_STAT_1_OFFSET);
    outb(VGA_ATTR_INDEX, 0x00);
    hwp->paletteEnabled = TRUE;
}

static void
stdDisablePalette(vgaHWPtr hwp)
{
    CARD8 tmp;

    tmp = inb(hwp->IOBase + VGA_IN_STAT_1_OFFSET);
    outb(VGA_ATTR_INDEX, 0x20);
    hwp->paletteEnabled = FALSE;
}

static void
stdWriteDacMask(vgaHWPtr hwp, CARD8 value)
{
    outb(VGA_DAC_MASK, value);
}

static CARD8
stdReadDacMask(vgaHWPtr hwp)
{
    return inb(VGA_DAC_MASK);
}

static void
stdWriteDacReadAddr(vgaHWPtr hwp, CARD8 value)
{
    outb(VGA_DAC_READ_ADDR, value);
}

static void
stdWriteDacWriteAddr(vgaHWPtr hwp, CARD8 value)
{
    outb(VGA_DAC_WRITE_ADDR, value);
}

static void
stdWriteDacData(vgaHWPtr hwp, CARD8 value)
{
    outb(VGA_DAC_DATA, value);
}

static CARD8
stdReadDacData(vgaHWPtr hwp)
{
    return inb(VGA_DAC_DATA);
}

void
vgaHWSetStdFuncs(vgaHWPtr hwp)
{
    hwp->writeCrtc		= stdWriteCrtc;
    hwp->readCrtc		= stdReadCrtc;
    hwp->writeGr		= stdWriteGr;
    hwp->readGr			= stdReadGr;
    hwp->writeAttr		= stdWriteAttr;
    hwp->readAttr		= stdReadAttr;
    hwp->writeSeq		= stdWriteSeq;
    hwp->readSeq		= stdReadSeq;
    hwp->writeMiscOut		= stdWriteMiscOut;
    hwp->readMiscOut		= stdReadMiscOut;
    hwp->enablePalette		= stdEnablePalette;
    hwp->disablePalette		= stdDisablePalette;
    hwp->writeDacMask		= stdWriteDacMask;
    hwp->readDacMask		= stdReadDacMask;
    hwp->writeDacWriteAddr	= stdWriteDacWriteAddr;
    hwp->writeDacReadAddr	= stdWriteDacReadAddr;
    hwp->writeDacData		= stdWriteDacData;
    hwp->readDacData		= stdReadDacData;
}

/*
 * MMIO versions of the register access functions.  These require
 * hwp->MemBase to be set in such a way that when the standard VGA port
 * adderss is added the correct memory address results.
 */

#ifndef __alpha__
#define minb(p) *(volatile CARD8 *)(hwp->MMIOBase + hwp->MMIOOffset + (p))
#define moutb(p,v) \
	*(volatile CARD8 *)(hwp->MMIOBase + hwp->MMIOOffset + (p)) = (v)
#else
#define minb(p) xf86ReadSparse8(hwp->MMIOBase, hwp->MMIOOffset + (p))
#define moutb(p,v) \
	xf86WriteSparse8((v), hwp->MMIOBase, hwp->MMIOOffset + (p))
#endif

static void
mmioWriteCrtc(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    moutb(hwp->IOBase + VGA_CRTC_INDEX_OFFSET, index);
    moutb(hwp->IOBase + VGA_CRTC_DATA_OFFSET, value);
}

static CARD8
mmioReadCrtc(vgaHWPtr hwp, CARD8 index)
{
    moutb(hwp->IOBase + VGA_CRTC_INDEX_OFFSET, index);
    return minb(hwp->IOBase + VGA_CRTC_DATA_OFFSET);
}

static void
mmioWriteGr(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    moutb(VGA_GRAPH_INDEX, index);
    moutb(VGA_GRAPH_DATA, value);
}

static CARD8
mmioReadGr(vgaHWPtr hwp, CARD8 index)
{
    moutb(VGA_GRAPH_INDEX, index);
    return minb(VGA_GRAPH_DATA);
}

static void
mmioWriteSeq(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    moutb(VGA_SEQ_INDEX, index);
    moutb(VGA_SEQ_DATA, value);
}

static CARD8
mmioReadSeq(vgaHWPtr hwp, CARD8 index)
{
    moutb(VGA_SEQ_INDEX, index);
    return minb(VGA_SEQ_DATA);
}

static void
mmioWriteAttr(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    CARD8 tmp;

    if (hwp->paletteEnabled)
	index &= ~0x20;
    else
	index |= 0x20;

    tmp = minb(hwp->IOBase + VGA_IN_STAT_1_OFFSET);
    moutb(VGA_ATTR_INDEX, index);
    moutb(VGA_ATTR_DATA_W, value);
}

static CARD8
mmioReadAttr(vgaHWPtr hwp, CARD8 index)
{
    CARD8 tmp;

    if (hwp->paletteEnabled)
	index &= ~0x20;
    else
	index |= 0x20;

    tmp = minb(hwp->IOBase + VGA_IN_STAT_1_OFFSET);
    moutb(VGA_ATTR_INDEX, index);
    return minb(VGA_ATTR_DATA_R);
}

static void
mmioWriteMiscOut(vgaHWPtr hwp, CARD8 value)
{
    moutb(VGA_MISC_OUT_W, value);
}

static CARD8
mmioReadMiscOut(vgaHWPtr hwp)
{
    return minb(VGA_MISC_OUT_R);
}

static void
mmioEnablePalette(vgaHWPtr hwp)
{
    CARD8 tmp;

    tmp = minb(hwp->IOBase + VGA_IN_STAT_1_OFFSET);
    moutb(VGA_ATTR_INDEX, 0x00);
    hwp->paletteEnabled = TRUE;
}

static void
mmioDisablePalette(vgaHWPtr hwp)
{
    CARD8 tmp;

    tmp = minb(hwp->IOBase + VGA_IN_STAT_1_OFFSET);
    moutb(VGA_ATTR_INDEX, 0x20);
    hwp->paletteEnabled = FALSE;
}

static void
mmioWriteDacMask(vgaHWPtr hwp, CARD8 value)
{
    moutb(VGA_DAC_MASK, value);
}

static CARD8
mmioReadDacMask(vgaHWPtr hwp)
{
    return minb(VGA_DAC_MASK);
}

static void
mmioWriteDacReadAddr(vgaHWPtr hwp, CARD8 value)
{
    moutb(VGA_DAC_READ_ADDR, value);
}

static void
mmioWriteDacWriteAddr(vgaHWPtr hwp, CARD8 value)
{
    moutb(VGA_DAC_WRITE_ADDR, value);
}

static void
mmioWriteDacData(vgaHWPtr hwp, CARD8 value)
{
    moutb(VGA_DAC_DATA, value);
}

static CARD8
mmioReadDacData(vgaHWPtr hwp)
{
    return minb(VGA_DAC_DATA);
}

void
vgaHWSetMmioFuncs(vgaHWPtr hwp, CARD8 *base, int offset)
{
    hwp->writeCrtc		= mmioWriteCrtc;
    hwp->readCrtc		= mmioReadCrtc;
    hwp->writeGr		= mmioWriteGr;
    hwp->readGr			= mmioReadGr;
    hwp->writeAttr		= mmioWriteAttr;
    hwp->readAttr		= mmioReadAttr;
    hwp->writeSeq		= mmioWriteSeq;
    hwp->readSeq		= mmioReadSeq;
    hwp->writeMiscOut		= mmioWriteMiscOut;
    hwp->readMiscOut		= mmioReadMiscOut;
    hwp->enablePalette		= mmioEnablePalette;
    hwp->disablePalette		= mmioDisablePalette;
    hwp->writeDacMask		= mmioWriteDacMask;
    hwp->readDacMask		= mmioReadDacMask;
    hwp->writeDacWriteAddr	= mmioWriteDacWriteAddr;
    hwp->writeDacReadAddr	= mmioWriteDacReadAddr;
    hwp->writeDacData		= mmioWriteDacData;
    hwp->readDacData		= mmioReadDacData;
    hwp->MMIOBase		= base;
    hwp->MMIOOffset		= offset;
}

/*
 * vgaHWProtect --
 *	Protect VGA registers and memory from corruption during loads.
 */

void
vgaHWProtect(ScrnInfoPtr pScrn, Bool on)
{
  vgaHWPtr hwp = VGAHWPTR(pScrn);
  
  unsigned char tmp;
  
  if (pScrn->vtSema) {
    if (on) {
      /*
       * Turn off screen and disable sequencer.
       */
      tmp = hwp->readSeq(hwp, 0x01);

      vgaHWSeqReset(hwp, TRUE);			/* start synchronous reset */
      hwp->writeSeq(hwp, 0x01, tmp | 0x20);	/* disable the display */

      hwp->enablePalette(hwp);
    } else {
      /*
       * Reenable sequencer, then turn on screen.
       */
  
      tmp = hwp->readSeq(hwp, 0x01);

      hwp->writeSeq(hwp, 0x01, tmp & ~0x20);	/* reenable display */
      vgaHWSeqReset(hwp, FALSE);		/* clear synchronousreset */

      hwp->disablePalette(hwp);
    }
  }
}


/*
 * vgaHWBlankScreen -- blank the screen.
 */

void
vgaHWBlankScreen(ScrnInfoPtr pScrn, Bool on)
{
  vgaHWPtr hwp = VGAHWPTR(pScrn);
  unsigned char scrn;

  scrn = hwp->readSeq(hwp, 0x01);

  if (on) {
    scrn &= ~0x20;			/* enable screen */
  } else {
    scrn |= 0x20;			/* blank screen */
  }

  vgaHWSeqReset(hwp, TRUE);
  hwp->writeSeq(hwp, 0x01, scrn);	/* change mode */
  vgaHWSeqReset(hwp, FALSE);
}


/*
 * vgaHWSaveScreen -- blank the screen.
 */

Bool
vgaHWSaveScreen(ScreenPtr pScreen, Bool on)
{
   ScrnInfoPtr pScrn = NULL;

   if (pScreen != NULL)
      pScrn = xf86Screens[pScreen->myNum];

   if (on)
      SetTimeSinceLastInputEvent();

   if ((pScrn != NULL) && pScrn->vtSema) {
     vgaHWBlankScreen(pScrn, on);
   }
   return (TRUE);
}


/*
 * vgaHWDPMSSet -- Sets VESA Display Power Management Signaling (DPMS) Mode
 *
 * This generic VGA function can only set the Off and On modes.  If the
 * Standby and Suspend modes are to be supported, a chip specific replacement
 * for this function must be written.
 */

void
vgaHWDPMSSet(ScrnInfoPtr pScrn, int PowerManagementMode, int flags)
{
#ifdef DPMSExtension
  unsigned char seq1 = 0, crtc17 = 0;
  vgaHWPtr hwp = VGAHWPTR(pScrn);

  if (!pScrn->vtSema) return;

  switch (PowerManagementMode) {
  case DPMSModeOn:
    /* Screen: On; HSync: On, VSync: On */
    seq1 = 0x00;
    crtc17 = 0x80;
    break;
  case DPMSModeStandby:
    /* Screen: Off; HSync: Off, VSync: On -- Not Supported */
    seq1 = 0x20;
    crtc17 = 0x80;
    break;
  case DPMSModeSuspend:
    /* Screen: Off; HSync: On, VSync: Off -- Not Supported */
    seq1 = 0x20;
    crtc17 = 0x80;
    break;
  case DPMSModeOff:
    /* Screen: Off; HSync: Off, VSync: Off */
    seq1 = 0x20;
    crtc17 = 0x00;
    break;
  }
  hwp->writeSeq(hwp, 0x00, 0x01);		/* Synchronous Reset */
  seq1 |= hwp->readSeq(hwp, 0x01) & ~0x20;
  hwp->writeSeq(hwp, 0x01, seq1);
  crtc17 |= hwp->readCrtc(hwp, 0x17) & ~0x80;
  usleep(10000);
  hwp->writeCrtc(hwp, 0x17, crtc17);
  hwp->writeSeq(hwp, 0x00, 0x03);		/* End Reset */
#endif
}


/*
 * vgaHWSeqReset
 *      perform a sequencer reset.
 */

void
vgaHWSeqReset(vgaHWPtr hwp, Bool start)
{
  if (start)
    hwp->writeSeq(hwp, 0x00, 0x01);		/* Synchronous Reset */
  else
    hwp->writeSeq(hwp, 0x00, 0x03);		/* End Reset */
}


void
vgaHWRestoreFonts(ScrnInfoPtr scrninfp, vgaRegPtr restore)
{
    vgaHWPtr hwp = VGAHWPTR(scrninfp);
    int savedIOBase;
    unsigned char miscOut, attr10, gr1, gr3, gr4, gr5, gr6, gr8, seq2, seq4;
    Bool doMap = FALSE;

#if defined(SAVE_TEXT) || defined(SAVE_FONT1) || defined(SAVE_FONT2)

    /* If nothing to do, return now */
    if (!hwp->FontInfo1 && !hwp->FontInfo2 && !hwp->TextInfo)
	return;

    if (hwp->Base == NULL) {
	doMap = TRUE;
	if (!vgaHWMapMem(scrninfp)) {
	    xf86DrvMsg(scrninfp->scrnIndex, X_ERROR,
		       "vgaHWRestoreFonts: vgaHWMapMem() failed\n");
	    return;
	}
    }

    /* save the registers that are needed here */
    miscOut = hwp->readMiscOut(hwp);
    attr10 = hwp->readAttr(hwp, 0x10);
    gr1 = hwp->readGr(hwp, 0x01);
    gr3 = hwp->readGr(hwp, 0x03);
    gr4 = hwp->readGr(hwp, 0x04);
    gr5 = hwp->readGr(hwp, 0x05);
    gr6 = hwp->readGr(hwp, 0x06);
    gr8 = hwp->readGr(hwp, 0x08);
    seq2 = hwp->readSeq(hwp, 0x02);
    seq4 = hwp->readSeq(hwp, 0x04);

    /* save hwp->IOBase and temporarily set it for colour mode */
    savedIOBase = hwp->IOBase;
    hwp->IOBase = VGA_IOBASE_COLOR;

    /* Force into colour mode */
    hwp->writeMiscOut(hwp, miscOut | 0x01);

    vgaHWBlankScreen(scrninfp, FALSE);

    /*
     * here we temporarily switch to 16 colour planar mode, to simply
     * copy the font-info and saved text.
     *
     * BUG ALERT: The (S)VGA's segment-select register MUST be set correctly!
     */

    hwp->writeAttr(hwp, 0x10, 0x01);	/* graphics mode */
    if (scrninfp->depth == 4) {
	/* GJA */
	hwp->writeGr(hwp, 0x03, 0x00);	/* don't rotate, write unmodified */
	hwp->writeGr(hwp, 0x08, 0xFF);	/* write all bits in a byte */
	hwp->writeGr(hwp, 0x01, 0x00);	/* all planes come from CPU */
    }

#ifdef SAVE_FONT1
    if (hwp->FontInfo1) {
	hwp->writeSeq(hwp, 0x02, 0x04);	/* write to plane 2 */
	hwp->writeSeq(hwp, 0x04, 0x06);	/* enable plane graphics */
	hwp->writeGr(hwp, 0x04, 0x02);	/* read plane 2 */
	hwp->writeGr(hwp, 0x05, 0x00);	/* write mode 0, read mode 0 */
	hwp->writeGr(hwp, 0x06, 0x05);	/* set graphics */
	slowbcopy_tobus(hwp->FontInfo1, hwp->Base, FONT_AMOUNT);
    }
#endif

#ifdef SAVE_FONT2
    if (hwp->FontInfo2) {
	hwp->writeSeq(hwp, 0x02, 0x08);	/* write to plane 3 */
	hwp->writeSeq(hwp, 0x04, 0x06);	/* enable plane graphics */
	hwp->writeGr(hwp, 0x04, 0x03);	/* read plane 3 */
	hwp->writeGr(hwp, 0x05, 0x00);	/* write mode 0, read mode 0 */
	hwp->writeGr(hwp, 0x06, 0x05);	/* set graphics */
	slowbcopy_tobus(hwp->FontInfo2, hwp->Base, FONT_AMOUNT);
    }
#endif

#ifdef SAVE_TEXT
    if (hwp->TextInfo) {
	hwp->writeSeq(hwp, 0x02, 0x01);	/* write to plane 0 */
	hwp->writeSeq(hwp, 0x04, 0x06);	/* enable plane graphics */
	hwp->writeGr(hwp, 0x04, 0x00);	/* read plane 0 */
	hwp->writeGr(hwp, 0x05, 0x00);	/* write mode 0, read mode 0 */
	hwp->writeGr(hwp, 0x06, 0x05);	/* set graphics */
	slowbcopy_tobus(hwp->TextInfo, hwp->Base, TEXT_AMOUNT);
	hwp->writeSeq(hwp, 0x02, 0x02);	/* write to plane 1 */
	hwp->writeSeq(hwp, 0x04, 0x06);	/* enable plane graphics */
	hwp->writeGr(hwp, 0x04, 0x01);	/* read plane 1 */
	hwp->writeGr(hwp, 0x05, 0x00);	/* write mode 0, read mode 0 */
	hwp->writeGr(hwp, 0x06, 0x05);	/* set graphics */
	slowbcopy_tobus((unsigned char *)hwp->TextInfo + TEXT_AMOUNT,
			hwp->Base, TEXT_AMOUNT);
    }
#endif

    vgaHWBlankScreen(scrninfp, TRUE);

    /* restore the registers that were changed */
    hwp->writeMiscOut(hwp, miscOut);
    hwp->writeAttr(hwp, 0x10, attr10);
    hwp->writeGr(hwp, 0x01, gr1);
    hwp->writeGr(hwp, 0x03, gr3);
    hwp->writeGr(hwp, 0x04, gr4);
    hwp->writeGr(hwp, 0x05, gr5);
    hwp->writeGr(hwp, 0x06, gr6);
    hwp->writeGr(hwp, 0x08, gr8);
    hwp->writeSeq(hwp, 0x02, seq2);
    hwp->writeSeq(hwp, 0x04, seq4);
    hwp->IOBase = savedIOBase;

    if (doMap)
	vgaHWUnmapMem(scrninfp);

#endif /* defined(SAVE_TEXT) || defined(SAVE_FONT1) || defined(SAVE_FONT2) */
}


void
vgaHWRestoreMode(ScrnInfoPtr scrninfp, vgaRegPtr restore)
{
    vgaHWPtr hwp = VGAHWPTR(scrninfp);
    int i;

    if (restore->MiscOutReg & 0x01)
	hwp->IOBase = VGA_IOBASE_COLOR;
    else
	hwp->IOBase = VGA_IOBASE_MONO;

    hwp->writeMiscOut(hwp, restore->MiscOutReg);

    for (i = 1; i < 5; i++)
	hwp->writeSeq(hwp, i, restore->Sequencer[i]);
  
    /* Ensure CRTC registers 0-7 are unlocked by clearing bit 7 or CRTC[17] */

    hwp->writeCrtc(hwp, 17, restore->CRTC[17] & ~0x80);

    for (i = 0; i < 25; i++)
	hwp->writeCrtc(hwp, i, restore->CRTC[i]);

    for (i = 0; i < 9; i++)
	hwp->writeGr(hwp, i, restore->Graphics[i]);

    hwp->enablePalette(hwp);
    for (i = 0; i < 21; i++)
	hwp->writeAttr(hwp, i, restore->Attribute[i]);
    hwp->disablePalette(hwp);
}


void
vgaHWRestoreColormap(ScrnInfoPtr scrninfp, vgaRegPtr restore)
{
    vgaHWPtr hwp = VGAHWPTR(scrninfp);
    int i;

    hwp->enablePalette(hwp);

    hwp->writeDacMask(hwp, 0xFF);
    hwp->writeDacWriteAddr(hwp, 0x00);
    for (i = 0; i < 768; i++) {
	hwp->writeDacData(hwp, restore->DAC[i]);
	DACDelay(hwp);
    }

    hwp->disablePalette(hwp);
}


/*
 * vgaHWRestore --
 *      restore the VGA state
 */

void
vgaHWRestore(ScrnInfoPtr scrninfp, vgaRegPtr restore, int flags)
{
    if (flags & VGA_SR_FONTS)
	vgaHWRestoreFonts(scrninfp, restore);

    if (flags & VGA_SR_MODE)
	vgaHWRestoreMode(scrninfp, restore);

    if (flags & VGA_SR_CMAP)
	vgaHWRestoreColormap(scrninfp, restore);
}

void
vgaHWSaveFonts(ScrnInfoPtr scrninfp, vgaRegPtr save)
{
    vgaHWPtr hwp = VGAHWPTR(scrninfp);
    int savedIOBase;
    unsigned char miscOut, attr10, gr4, gr5, gr6, seq2, seq4;
    Bool doMap = FALSE;

#if defined(SAVE_TEXT) || defined(SAVE_FONT1) || defined(SAVE_FONT2)

    if (hwp->Base == NULL) {
	doMap = TRUE;
	if (!vgaHWMapMem(scrninfp)) {
	    xf86DrvMsg(scrninfp->scrnIndex, X_ERROR,
		       "vgaHWSaveFonts: vgaHWMapMem() failed\n");
	    return;
	}
    }

    /* If in graphics mode, don't save anything */
    attr10 = hwp->readAttr(hwp, 0x10);
    if (attr10 & 0x01)
	return;

    /* save the registers that are needed here */
    miscOut = hwp->readMiscOut(hwp);
    gr4 = hwp->readGr(hwp, 0x04);
    gr5 = hwp->readGr(hwp, 0x05);
    gr6 = hwp->readGr(hwp, 0x06);
    seq2 = hwp->readSeq(hwp, 0x02);
    seq4 = hwp->readSeq(hwp, 0x04);

    /* save hwp->IOBase and temporarily set it for colour mode */
    savedIOBase = hwp->IOBase;
    hwp->IOBase = VGA_IOBASE_COLOR;

    /* Force into colour mode */
    hwp->writeMiscOut(hwp, miscOut | 0x01);

    vgaHWBlankScreen(scrninfp, FALSE);
  
    /*
     * get the character sets, and text screen if required
     */
    /*
     * Here we temporarily switch to 16 colour planar mode, to simply
     * copy the font-info
     *
     * BUG ALERT: The (S)VGA's segment-select register MUST be set correctly!
     */

    hwp->writeAttr(hwp, 0x10, 0x01);	/* graphics mode */

#ifdef SAVE_FONT1
    if (hwp->FontInfo1 || (hwp->FontInfo1 = (pointer)xalloc(FONT_AMOUNT))) {
	hwp->writeSeq(hwp, 0x02, 0x04);	/* write to plane 2 */
	hwp->writeSeq(hwp, 0x04, 0x06);	/* enable plane graphics */
	hwp->writeGr(hwp, 0x04, 0x02);	/* read plane 2 */
	hwp->writeGr(hwp, 0x05, 0x00);	/* write mode 0, read mode 0 */
	hwp->writeGr(hwp, 0x06, 0x05);	/* set graphics */
	slowbcopy_frombus(hwp->Base, hwp->FontInfo1, FONT_AMOUNT);
    }
#endif /* SAVE_FONT1 */
#ifdef SAVE_FONT2
    if (hwp->FontInfo2 || (hwp->FontInfo2 = (pointer)xalloc(FONT_AMOUNT))) {
	hwp->writeSeq(hwp, 0x02, 0x08);	/* write to plane 3 */
	hwp->writeSeq(hwp, 0x04, 0x06);	/* enable plane graphics */
	hwp->writeGr(hwp, 0x04, 0x03);	/* read plane 3 */
	hwp->writeGr(hwp, 0x05, 0x00);	/* write mode 0, read mode 0 */
	hwp->writeGr(hwp, 0x06, 0x05);	/* set graphics */
	slowbcopy_frombus(hwp->Base, hwp->FontInfo2, FONT_AMOUNT);
    }
#endif /* SAVE_FONT2 */
#ifdef SAVE_TEXT
    if (hwp->TextInfo || (hwp->TextInfo = (pointer)xalloc(2 * TEXT_AMOUNT))) {
	hwp->writeSeq(hwp, 0x02, 0x01);	/* write to plane 0 */
	hwp->writeSeq(hwp, 0x04, 0x06);	/* enable plane graphics */
	hwp->writeGr(hwp, 0x04, 0x00);	/* read plane 0 */
	hwp->writeGr(hwp, 0x05, 0x00);	/* write mode 0, read mode 0 */
	hwp->writeGr(hwp, 0x06, 0x05);	/* set graphics */
	slowbcopy_frombus(hwp->Base, hwp->TextInfo, TEXT_AMOUNT);
	hwp->writeSeq(hwp, 0x02, 0x02);	/* write to plane 1 */
	hwp->writeSeq(hwp, 0x04, 0x06);	/* enable plane graphics */
	hwp->writeGr(hwp, 0x04, 0x01);	/* read plane 1 */
	hwp->writeGr(hwp, 0x05, 0x00);	/* write mode 0, read mode 0 */
	hwp->writeGr(hwp, 0x06, 0x05);	/* set graphics */
	slowbcopy_frombus(hwp->Base,
		(unsigned char *)hwp->TextInfo + TEXT_AMOUNT, TEXT_AMOUNT);
    }
#endif /* SAVE_TEXT */

    /* Restore clobbered registers */
    hwp->writeAttr(hwp, 0x10, attr10);
    hwp->writeSeq(hwp, 0x02, seq2);
    hwp->writeSeq(hwp, 0x04, seq4);
    hwp->writeGr(hwp, 0x04, gr4);
    hwp->writeGr(hwp, 0x05, gr5);
    hwp->writeGr(hwp, 0x06, gr6);
    hwp->writeMiscOut(hwp, miscOut);
    hwp->IOBase = savedIOBase;

    vgaHWBlankScreen(scrninfp, TRUE);

    if (doMap)
	vgaHWUnmapMem(scrninfp);

#endif /* defined(SAVE_TEXT) || defined(SAVE_FONT1) || defined(SAVE_FONT2) */
}

void
vgaHWSaveMode(ScrnInfoPtr scrninfp, vgaRegPtr save)
{
    vgaHWPtr hwp = VGAHWPTR(scrninfp);
    int i;

    save->MiscOutReg = hwp->readMiscOut(hwp);
    if (save->MiscOutReg & 0x01)
	hwp->IOBase = VGA_IOBASE_COLOR;
    else
	hwp->IOBase = VGA_IOBASE_MONO;

    for (i = 0; i < 25; i++) {
	save->CRTC[i] = hwp->readCrtc(hwp, i);
#ifdef DEBUG
	ErrorF("CRTC[0x%02x] = 0x%02x\n", i, save->CRTC[i]);
#endif
    }

    hwp->enablePalette(hwp);
    for (i = 0; i < 21; i++) {
	save->Attribute[i] = hwp->readAttr(hwp, i);
#ifdef DEBUG
	ErrorF("Attribute[0x%02x] = 0x%02x\n", i, save->Attribute[i]);
#endif
    }
    hwp->disablePalette(hwp);

    for (i = 0; i < 9; i++) {
	save->Graphics[i] = hwp->readGr(hwp, i);
#ifdef DEBUG
	ErrorF("Graphics[0x%02x] = 0x%02x\n", i, save->Graphics[i]);
#endif
    }

    for (i = 0; i < 5; i++) {
	save->Sequencer[i] = hwp->readSeq(hwp, i);
#ifdef DEBUG
	ErrorF("Sequencer[0x%02x] = 0x%02x\n", i, save->Sequencer[i]);
#endif
    }
}


void
vgaHWSaveColormap(ScrnInfoPtr scrninfp, vgaRegPtr save)
{
    vgaHWPtr hwp = VGAHWPTR(scrninfp);
    Bool readError = FALSE;
    int i;

#ifdef NEED_SAVED_CMAP
    /*
     * Some ET4000 chips from 1991 have a HW bug that prevents the reading
     * of the color lookup table.  Mask rev 9042EAI is known to have this bug.
     *
     * If the colourmap is not readable, we set the saved map to a default
     * map (taken from Ferraro's "Programmer's Guide to the EGA and VGA
     * Cards" 2nd ed).
     */

    /* Only save it once */
    if (hwp->cmapSaved)
	return;

    hwp->enablePalette(hwp);

    hwp->writeDacMask(hwp, 0xFF);
    /*
     * check if we can read the lookup table
     */
    hwp->writeDacReadAddr(hwp, 0x00);
    for (i = 0; i < 3; i++) {
	save->DAC[i] = hwp->readDacData(hwp);
#ifdef DEBUG
	switch (i % 3) {
	case 0:
	    ErrorF("DAC[0x%02x] = 0x%02x, ", i / 3, save->DAC[i]);
	    break;
	case 1:
	    ErrorF("0x%02x, ", save->DAC[i]);
	    break;
	case 2:
	    ErrorF("0x%02x\n", save->DAC[i]);
	}
#endif
    }
    hwp->writeDacWriteAddr(hwp, 0x00);
    for (i = 0; i < 3; i++)
	hwp->writeDacData(hwp, ~save->DAC[i] & DAC_TEST_MASK);
    hwp->writeDacReadAddr(hwp, 0x00);
    for (i = 0; i < 3; i++) {
	if (hwp->readDacData(hwp) != (~save->DAC[i] & DAC_TEST_MASK))
	    readError = TRUE;
    }
  
    if (readError) {
	/*			 
	 * save the default lookup table
	 */
	memmove(save->DAC, defaultDAC, 768);
	xf86DrvMsg(scrninfp->scrnIndex, X_WARNING,
	   "Cannot read colourmap from VGA.  Will restore with default\n");
    } else {
	/* save the colourmap */
	hwp->writeDacReadAddr(hwp, 0x01);
	for (i = 3; i < 768; i++) {
	    save->DAC[i] = hwp->readDacData(hwp);
	    DACDelay(hwp);
#ifdef DEBUG
	    switch (i % 3) {
	    case 0:
		ErrorF("DAC[0x%02x] = 0x%02x, ", i / 3, save->DAC[i]);
		break;
	    case 1:
		ErrorF("0x%02x, ", save->DAC[i]);
		break;
	    case 2:
		ErrorF("0x%02x\n", save->DAC[i]);
	    }
#endif
	}
    }
    hwp->disablePalette(hwp);
    hwp->cmapSaved = TRUE;
#endif
}

/*
 * vgaHWSave --
 *      save the current VGA state
 */

void
vgaHWSave(ScrnInfoPtr scrninfp, vgaRegPtr save, int flags)
{
    if (save == NULL)
	return;

   if (flags & VGA_SR_CMAP)
	vgaHWSaveColormap(scrninfp, save);

   if (flags & VGA_SR_MODE)
	vgaHWSaveMode(scrninfp, save);

   if (flags & VGA_SR_FONTS)
	vgaHWSaveFonts(scrninfp, save);
}


/*
 * vgaHWInit --
 *      Handle the initialization, etc. of a screen.
 *      Return FALSE on failure.
 */

Bool
vgaHWInit(ScrnInfoPtr scrninfp, DisplayModePtr mode)
{
    unsigned int       i;
    vgaHWPtr hwp;
    vgaRegPtr regp;
    int depth = scrninfp->depth;

    /*
     * make sure the vgaHWRec is allocated
     */
    if (!vgaHWGetHWRec(scrninfp))
	return FALSE;
    hwp = VGAHWPTR(scrninfp);
    regp = &hwp->ModeReg;
    
    /*
     * compute correct Hsync & Vsync polarity 
     */
    if ((mode->Flags & (V_PHSYNC | V_NHSYNC))
        && (mode->Flags & (V_PVSYNC | V_NVSYNC)))
    {
        regp->MiscOutReg = 0x23;
        if (mode->Flags & V_NHSYNC) regp->MiscOutReg |= 0x40;
        if (mode->Flags & V_NVSYNC) regp->MiscOutReg |= 0x80;
    }
    else
    {
        int VDisplay = mode->VDisplay;
        if (mode->Flags & V_DBLSCAN)
            VDisplay *= 2;
        if (mode->VScan > 1)
            VDisplay *= mode->VScan;
        if      (VDisplay < 400)
            regp->MiscOutReg = 0xA3;		/* +hsync -vsync */
        else if (VDisplay < 480)
            regp->MiscOutReg = 0x63;		/* -hsync +vsync */
        else if (VDisplay < 768)
            regp->MiscOutReg = 0xE3;		/* -hsync -vsync */
        else
            regp->MiscOutReg = 0x23;		/* +hsync +vsync */
    }
    
    regp->MiscOutReg |= (mode->ClockIndex & 0x03) << 2;

    /*
     * Time Sequencer
     */
    if (depth == 4)
        regp->Sequencer[0] = 0x02;
    else
        regp->Sequencer[0] = 0x00;
    if (mode->Flags & V_CLKDIV2) 
        regp->Sequencer[1] = 0x09;
    else
        regp->Sequencer[1] = 0x01;
    if (depth == 1)
        regp->Sequencer[2] = 1 << BIT_PLANE;
    else
        regp->Sequencer[2] = 0x0F;
    regp->Sequencer[3] = 0x00;                             /* Font select */
    if (depth < 8)
        regp->Sequencer[4] = 0x06;                             /* Misc */
    else
        regp->Sequencer[4] = 0x0E;                             /* Misc */

    /*
     * CRTC Controller
     */
    regp->CRTC[0]  = (mode->CrtcHTotal >> 3) - 5;
    regp->CRTC[1]  = (mode->CrtcHDisplay >> 3) - 1;
    regp->CRTC[2]  = (mode->CrtcHBlankStart >> 3) - 1;
    regp->CRTC[3]  = (((mode->CrtcHBlankEnd >> 3) - 1 - 1) & 0x1F ) | 0x80;
    i = (((mode->CrtcHSkew << 2) + 0x10) & ~0x1F);
    if (i < 0x80)
        regp->CRTC[3] |= i;
    regp->CRTC[4]  = (mode->CrtcHSyncStart >> 3);
    regp->CRTC[5]  = ((((mode->CrtcHBlankEnd >> 3) - 1 - 1) & 0x20 ) << 2 )
        | (((mode->CrtcHSyncEnd >> 3)) & 0x1F);
    regp->CRTC[6]  = (mode->CrtcVTotal - 2) & 0xFF;
    regp->CRTC[7]  = (((mode->CrtcVTotal -2) & 0x100) >> 8 )
        | (((mode->CrtcVDisplay -1) & 0x100) >> 7 )
        | ((mode->CrtcVSyncStart & 0x100) >> 6 )
	| (((mode->CrtcVBlankStart-1) & 0x100) >> 5 )
        | 0x10
        | (((mode->CrtcVTotal -2) & 0x200)   >> 4 )
        | (((mode->CrtcVDisplay -1) & 0x200) >> 3 )
        | ((mode->CrtcVSyncStart & 0x200) >> 2 );
    regp->CRTC[8]  = 0x00;
    regp->CRTC[9]  = (((mode->CrtcVBlankStart-1) & 0x200) >>4) | 0x40;
    if (mode->Flags & V_DBLSCAN)
        regp->CRTC[9] |= 0x80;
    if (mode->VScan >= 32)
        regp->CRTC[9] |= 0x1F;
    else if (mode->VScan > 1)
        regp->CRTC[9] |= mode->VScan - 1;
    regp->CRTC[10] = 0x00;
    regp->CRTC[11] = 0x00;
    regp->CRTC[12] = 0x00;
    regp->CRTC[13] = 0x00;
    regp->CRTC[14] = 0x00;
    regp->CRTC[15] = 0x00;
    regp->CRTC[16] = mode->CrtcVSyncStart & 0xFF;
    regp->CRTC[17] = (mode->CrtcVSyncEnd & 0x0F) | 0x20;
    regp->CRTC[18] = (mode->CrtcVDisplay -1) & 0xFF;
    regp->CRTC[19] = scrninfp->displayWidth >> 4;  /* just a guess */
    regp->CRTC[20] = 0x00;
    regp->CRTC[21] = (mode->CrtcVBlankStart-1) & 0xFF; 
    regp->CRTC[22] = (mode->CrtcVBlankEnd-1 - 1) & 0xFF;
    if (depth < 8)
        regp->CRTC[23] = 0xE3;
    else
        regp->CRTC[23] = 0xC3;
    regp->CRTC[24] = 0xFF;

    /*
     * Graphics Display Controller
     */
    regp->Graphics[0] = 0x00;
    regp->Graphics[1] = 0x00;
    regp->Graphics[2] = 0x00;
    regp->Graphics[3] = 0x00;
    if (depth == 1) {
        regp->Graphics[4] = BIT_PLANE;
        regp->Graphics[5] = 0x00;
    } else {
        regp->Graphics[4] = 0x00;
        if (depth == 4)
            regp->Graphics[5] = 0x02;
        else
            regp->Graphics[5] = 0x40;
    }
    regp->Graphics[6] = 0x05;   /* only map 64k VGA memory !!!! */
    regp->Graphics[7] = 0x0F;
    regp->Graphics[8] = 0xFF;
  
    if (depth == 1) {
        /* Initialise the Mono map according to which bit-plane gets used */
        for (i=0; i<16; i++)
            if (i & (1<<BIT_PLANE))
                regp->Attribute[i] = WHITE_VALUE;
            else
                regp->Attribute[i] = BLACK_VALUE;

        regp->Attribute[16] = 0x01;  /* -VGA2- */ /* wrong for the ET4000 */
	if (!hwp->ShowOverscan)
            regp->Attribute[17] = OVERSCAN_VALUE;  /* -VGA2- */
    } else {
        regp->Attribute[0]  = 0x00; /* standard colormap translation */
        regp->Attribute[1]  = 0x01;
        regp->Attribute[2]  = 0x02;
        regp->Attribute[3]  = 0x03;
        regp->Attribute[4]  = 0x04;
        regp->Attribute[5]  = 0x05;
        regp->Attribute[6]  = 0x06;
        regp->Attribute[7]  = 0x07;
        regp->Attribute[8]  = 0x08;
        regp->Attribute[9]  = 0x09;
        regp->Attribute[10] = 0x0A;
        regp->Attribute[11] = 0x0B;
        regp->Attribute[12] = 0x0C;
        regp->Attribute[13] = 0x0D;
        regp->Attribute[14] = 0x0E;
        regp->Attribute[15] = 0x0F;
        if (depth == 4)
            regp->Attribute[16] = 0x81; /* wrong for the ET4000 */
        else
            regp->Attribute[16] = 0x41; /* wrong for the ET4000 */
        /* Attribute[17] (overscan) initialised in vgaHWGetHWRec() */
    }
    regp->Attribute[18] = 0x0F;
    regp->Attribute[19] = 0x00;
    regp->Attribute[20] = 0x00;

    return(TRUE);
}



/*
 * these are some more hardware specific helpers, formerly in vga.c
 */
static void
vgaHWGetHWRecPrivate(void)
{
    if (vgaHWPrivateIndex < 0)
	vgaHWPrivateIndex = xf86AllocateScrnInfoPrivateIndex();
    return;
}

Bool
vgaHWGetHWRec(ScrnInfoPtr scrp)
{
    vgaRegPtr regp;
    vgaHWPtr hwp;
    int i;
    
    /*
     * Let's make sure that the private exists and allocate one.
     */
    vgaHWGetHWRecPrivate();
    /*
     * New privates are always set to NULL, so we can check if the allocation
     * has already been done.
     */
    if (VGAHWPTR(scrp))
	return TRUE;
    hwp = VGAHWPTRLVAL(scrp) = xnfcalloc(sizeof(vgaHWRec), 1);
    regp = &VGAHWPTR(scrp)->ModeReg;
    if (scrp->bitsPerPixel == 1) {
        /*
         * initialize default colormap for monochrome
         */
        for (i=0; i<3;   i++) regp->DAC[i] = 0x00;
        for (i=3; i<768; i++) regp->DAC[i] = 0x3F;
        /* The old blackColour/whiteColour handling needs to be redone XXX */
        i = BLACK_VALUE * 3;
#if 0
        regp->DAC[i++] = scrp->blackColour.red;
        regp->DAC[i++] = scrp->blackColour.green;
        regp->DAC[i] = scrp->blackColour.blue;
#else
        regp->DAC[i++] = 0;
        regp->DAC[i++] = 0;
        regp->DAC[i] = 0;
#endif
        i = WHITE_VALUE * 3;
#if 0
        regp->DAC[i++] = scrp->whiteColour.red;
        regp->DAC[i++] = scrp->whiteColour.green;
        regp->DAC[i] = scrp->whiteColour.blue;
#else
        regp->DAC[i++] = 0x3F;
        regp->DAC[i++] = 0x3F;
        regp->DAC[i] = 0x3F;
#endif
        i = OVERSCAN_VALUE * 3;
        regp->DAC[i++] = 0x00;
        regp->DAC[i++] = 0x00;
        regp->DAC[i] = 0x00;
    } else {
	/* Set all colours to black */
        for (i=0; i<768; i++) regp->DAC[i] = 0x00;
        /* ... and the overscan */
        if (scrp->depth == 8)
            regp->Attribute[17] = 0xFF;
    }
    if (xf86FindOption(scrp->confScreen->options, "ShowOverscan")) {
	xf86MarkOptionUsedByName(scrp->confScreen->options, "ShowOverscan");
	xf86DrvMsg(scrp->scrnIndex, X_CONFIG, "Showing overscan area\n");
	regp->DAC[765] = 0x3F; 
	regp->DAC[766] = 0x00; 
	regp->DAC[767] = 0x3F; 
	regp->Attribute[17] = 0xFF;
	hwp->ShowOverscan = TRUE;
    } else
	hwp->ShowOverscan = FALSE;

    hwp->paletteEnabled = FALSE;
    hwp->cmapSaved = FALSE;
    hwp->MapSize = 0;
    hwp->pScrn = scrp;

    /* Initialise the function pointers with the standard VGA versions */
    vgaHWSetStdFuncs(hwp);

    return TRUE;
}


void
vgaHWFreeHWRec(ScrnInfoPtr scrp)
{
    if (vgaHWPrivateIndex >= 0) {
	vgaHWPtr hwp = VGAHWPTR(scrp);

	xfree(hwp->FontInfo1);
	xfree(hwp->FontInfo2);
	xfree(hwp->TextInfo);

	xfree(hwp);
	VGAHWPTRLVAL(scrp) = NULL;
    }
}



Bool
vgaHWMapMem(ScrnInfoPtr scrp)
{
    vgaHWPtr hwp = VGAHWPTR(scrp);
    int scr_index = scrp->scrnIndex;
    
    /* If not set, initialise with the defaults */
    if (hwp->MapSize == 0)
	hwp->MapSize = VGA_DEFAULT_MEM_SIZE;
    if (hwp->MapPhys == 0)
	hwp->MapPhys = VGA_DEFAULT_PHYS_ADDR;

    hwp->Base = xf86MapVidMem(scr_index, VIDMEM_FRAMEBUFFER,
			      (pointer)hwp->MapPhys, hwp->MapSize);
    return hwp->Base != NULL;
}


void
vgaHWUnmapMem(ScrnInfoPtr scrp)
{
    vgaHWPtr hwp = VGAHWPTR(scrp);
    int scr_index = scrp->scrnIndex;

    if (hwp->Base == NULL)
	return;

    xf86UnMapVidMem(scr_index, hwp->Base, hwp->MapSize);
    hwp->Base = NULL;
}

int
vgaHWGetIndex()
{
    return vgaHWPrivateIndex;
}


void
vgaHWGetIOBase(vgaHWPtr hwp)
{
    hwp->IOBase = (hwp->readMiscOut(hwp) & 0x01) ?
				VGA_IOBASE_COLOR : VGA_IOBASE_MONO;
}



void
vgaHWLock(vgaHWPtr hwp)
{
    /* Protect CRTC[0-7] */
    hwp->writeCrtc(hwp, 0x11, hwp->readCrtc(hwp, 0x11) & ~0x80);
}

void
vgaHWUnlock(vgaHWPtr hwp)
{
    /* Unprotect CRTC[0-7] */
     hwp->writeCrtc(hwp, 0x11, hwp->readCrtc(hwp, 0x11) | 0x80);
}


static void
vgaHWLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors,
		 short visualClass)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    int i, index;

    hwp->enablePalette(hwp);

    for (i = 0; i < numColors; i++) {
	index = indices[i];
	hwp->writeDacWriteAddr(hwp, index);
	DACDelay(hwp);
	hwp->writeDacData(hwp, colors[index].red);
	DACDelay(hwp);
	hwp->writeDacData(hwp, colors[index].green);
	DACDelay(hwp);
	hwp->writeDacData(hwp, colors[index].blue);
	DACDelay(hwp);
    }

    hwp->disablePalette(hwp);
}

Bool
vgaHWHandleColormaps(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    if (pScrn->depth > 1 && pScrn->depth <= 8) {
	return xf86HandleColormaps(pScreen, 1 << pScrn->depth,
				   pScrn->rgbBits, vgaHWLoadPalette,
				   CMAP_RELOAD_ON_MODE_SWITCH);
    }
    return TRUE;
}

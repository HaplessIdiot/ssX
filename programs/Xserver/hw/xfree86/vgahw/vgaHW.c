/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/vgahw/vgaHW.c,v 1.4 1998/08/19 12:48:33 dawes Exp $
 *
 * Copyright 1991-1998 by The XFree86 Project, Inc.
 *
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 */
/* $XConsortium: vgaHW.c /main/19 1996/10/28 04:55:33 kaleb $ */

#if !defined(AMOEBA) && !defined(MINIX)
#define _NEED_SYSI86
#endif

#include "X.h"
#include "misc.h"

#ifndef VGA_MMIO
#include "compiler.h"
#define VGANAME(x) x
#else 
#define VGANAME(x) x##MMIO
#endif /* VGA_MMIO */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#ifndef VGA_MMIO
#include "vgaHW.h"
#else
#include "vgaHWmmio.h"
#endif

/* XXX The PC98 bits here need to be cleaned up */

#ifdef PC98_EGC
/* I/O port address define for extended EGC */
#define		EGC_READ	0x4a2	/* EGC FGC,EGC,Read Plane  */
#define		EGC_MASK	0x4a8	/* EGC Mask register       */
#define		EGC_ADD		0x4ac	/* EGC Dest/Source address */
#define		EGC_LENGTH	0x4ae	/* EGC Bit length          */
#endif

#if !defined(PC98_PEGC) && !defined(PC98_EGC)
#if !defined(MONOVGA) && !defined(SCO)
#ifndef SAVE_FONT1
#define SAVE_FONT1
#endif
#endif

#if defined(Lynx) || defined(CSRG_BASED) || defined(MACH386) || defined(linux) || defined(AMOEBA) || defined(MINIX)
#ifndef NEED_SAVED_CMAP
#define NEED_SAVED_CMAP
#endif
#ifndef MONOVGA
#ifndef SAVE_TEXT
#define SAVE_TEXT
#endif
#endif
#ifndef SAVE_FONT2
#define SAVE_FONT2
#endif
#endif

/* bytes per plane to save for text */
#if defined(Lynx) || defined(linux) || defined(MINIX)
#define TEXT_AMOUNT 16384
#else
#define TEXT_AMOUNT 4096
#endif

/* bytes per plane to save for font data */
#define FONT_AMOUNT 8192
#endif /* !defined(PC98_PEGC) && !defined(PC98_EGC) */

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

#if defined(CSRG_BASED) || defined(MACH386)
#include <sys/time.h>
#endif

#ifdef MACH386
#define WEXITSTATUS(x) (x.w_retcode)
#define WTERMSIG(x) (x.w_termsig)
#define WSTOPSIG(x) (x.w_stopsig)
#endif

/* DAC indices for white and black */
#define WHITE_VALUE 0x3F
#define BLACK_VALUE 0x00
#define OVERSCAN_VALUE 0x01


/* Use a private definition of this here */
#undef VGAHWPTR
#define VGAHWPTRLVAL(p) (p)->privates[vgaHWPrivateIndex].ptr
#define VGAHWPTR(p) ((vgaHWPtr)(VGAHWPTRLVAL(p)))

#ifndef VGA_MMIO
int vgaRamdacMask = 0x3F;
int vgaHWPrivateIndex = -1;

#ifdef NEED_SAVED_CMAP
/* This default colourmap is used only when it can't be read from the VGA */

unsigned char defaultDAC[768] =
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
#endif /* VGA_MMIO */

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
 * vgaHWProtect --
 *	Protect VGA registers and memory from corruption during loads.
 */
void
VGANAME(vgaHWProtect)(ScrnInfoPtr pScrn, Bool on)
{
#if !defined(PC98_PEGC) && !defined(PC98_EGC)
  vgaHWPtr hwp = VGAHWPTR(pScrn);
  
  unsigned char tmp;
  
  if (pScrn->vtSema) {
    if (on) {
      /*
       * Turn off screen and disable sequencer.
       */
      outb(0x3C4, 0x01);
      tmp = inb(0x3C5);

      VGANAME(vgaHWSeqReset)(hwp, TRUE);	/* start synchronous reset */
      outw(0x3C4, ((tmp | 0x20) << 8) | 0x01);	/* disable the display */

      tmp = inb(hwp->IOBase + 0x0A);
      outb(0x3C0, 0x00);			/* enable pallete access */
    }
    else {
      /*
       * Reenable sequencer, then turn on screen.
       */
      outb(0x3C4, 0x01);
      tmp = inb(0x3C5);

      outw(0x3C4, ((tmp & 0xDF) << 8) | 0x01);	/* reenable display */
      VGANAME(vgaHWSeqReset)(hwp, FALSE);	/* clear synchronousreset */

      tmp = inb(hwp->IOBase + 0x0A);
      outb(0x3C0, 0x20);			/* disable pallete access */
    }
  }
#endif /* !defined(PC98_PEGC) && !defined(PC98_EGC) */
}

/*
 * vgaHWBlankScreen -- blank the screen.
 */

void
VGANAME(vgaHWBlankScreen)(ScrnInfoPtr pScrn, Bool on)
{
  vgaHWPtr hwp = VGAHWPTR(pScrn);
#if !defined(PC98_EGC) && !defined(PC98_PEGC)
  unsigned char scrn;

  outb(0x3C4,1);
  scrn = inb(0x3C5);

  if(on) {
    scrn &= 0xDF;			/* enable screen */
  }else {
    scrn |= 0x20;			/* blank screen */
  }

  VGANAME(vgaHWSeqReset)(hwp, TRUE);
  outw(0x3C4, (scrn << 8) | 0x01); /* change mode */
  VGANAME(vgaHWSeqReset)(hwp, FALSE);
#else
  if(on) 
    outb(0xa2, 0xd);
  else
    outb(0xa2, 0xc);
#endif
}

/*
 * vgaHWSaveScreen -- blank the screen.
 */

Bool
VGANAME(vgaHWSaveScreen)(ScreenPtr pScreen, Bool on)
{
   ScrnInfoPtr pScrn = NULL;

   if (pScreen != NULL)
      pScrn = xf86Screens[pScreen->myNum];

   if (on)
      SetTimeSinceLastInputEvent();

   if ((pScrn != NULL) && pScrn->vtSema) {
     VGANAME(vgaHWBlankScreen)(pScrn, on);
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
VGANAME(vgaHWDPMSSet)(ScrnInfoPtr pScrn, int PowerManagementMode, int flags)
{
#ifdef DPMSExtension
  unsigned char seq1, crtc17;
  vgaHWPtr hwp = VGAHWPTR(pScrn);

  if (!pScrn->vtSema) return;
  switch (PowerManagementMode)
  {
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
  outw(0x3C4, 0x0100);	/* Synchronous Reset */
  outb(0x3C4, 0x01);	/* Select SEQ1 */
  seq1 |= inb(0x3C5) & ~0x20;
  outb(0x3C5, seq1);
  outb(hwp->IOBase+4, 0x17); /* Select CRTC17 */
  crtc17 |= inb(hwp->IOBase+5) & ~0x80;
  usleep(10000);
  outb(hwp->IOBase+5, crtc17);
  outw(0x3C4, 0x0300);	/* End Reset */
#endif
}

/*
 * vgaHWSeqReset
 *      perform a sequencer reset.
 */

void
VGANAME(vgaHWSeqReset)(vgaHWPtr hwp, Bool start)
{
#if !defined(PC98_PEGC) && !defined(PC98_EGC)
  if (start == TRUE)
    outw(0x3C4, 0x0100);        /* synchronous reset */
  else
    outw(0x3C4, 0x0300);        /* end reset */
#endif
}

/*
 * vgaHWRestore --
 *      restore a video mode
 */

void
VGANAME(vgaHWRestore)(ScrnInfoPtr scrninfp, vgaRegPtr restore, Bool restoreFonts)
{
    int i,tmp;
    vgaHWPtr hwp = VGAHWPTR(scrninfp);
  
#if !defined(PC98_PEGC) && !defined(PC98_EGC)
  tmp = inb(hwp->IOBase + 0x0A);		/* Reset flip-flop */
  outb(0x3C0, 0x00);			/* Enables pallete access */

  outb(0x3C2, restore->MiscOutReg | 0x01);      /* Force to colour emulation */
#endif

  /*
   * This here is a workaround a bug in the kd-driver. We MUST explicitely
   * restore the font we got, when we entered graphics mode.
   * The bug was seen on ESIX, and ISC 2.0.2 when using a monochrome
   * monitor. 
   *
   * BTW, also GIO_FONT seems to have a bug, so we cannot use it, to get
   * a font.
   */
  
  VGANAME(vgaHWBlankScreen)(scrninfp, FALSE);
#if defined(SAVE_TEXT) || defined(SAVE_FONT1) || defined(SAVE_FONT2)
  if (restoreFonts)
  if(hwp->FontInfo1 || hwp->FontInfo2 || hwp->TextInfo) {
    /*
     * here we temporarily switch to 16 colour planar mode, to simply
     * copy the font-info and saved text
     *
     * BUG ALERT: The (S)VGA's segment-select register MUST be set correctly!
     */
    tmp = inb(0x3D0 + 0x0A); /* reset flip-flop */
    outb(0x3C0,0x30); outb(0x3C0, 0x01); /* graphics mode */
    if (scrninfp->depth == 4) {
      outw(0x3CE,0x0003); /* GJA - don't rotate, write unmodified */
      outw(0x3CE,0xFF08); /* GJA - write all bits in a byte */
      outw(0x3CE,0x0001); /* GJA - all planes come from CPU */
    }
#ifdef SAVE_FONT1
    if (hwp->FontInfo1) {
      outw(0x3C4, 0x0402);    /* write to plane 2 */
      outw(0x3C4, 0x0604);    /* enable plane graphics */
      outw(0x3CE, 0x0204);    /* read plane 2 */
      outw(0x3CE, 0x0005);    /* write mode 0, read mode 0 */
      outw(0x3CE, 0x0506);    /* set graphics */
      slowbcopy_tobus(hwp->FontInfo1, hwp->Base, FONT_AMOUNT);
    }
#endif
#ifdef SAVE_FONT2
    if (hwp->FontInfo2) {
      outw(0x3C4, 0x0802);    /* write to plane 3 */
      outw(0x3C4, 0x0604);    /* enable plane graphics */
      outw(0x3CE, 0x0304);    /* read plane 3 */
      outw(0x3CE, 0x0005);    /* write mode 0, read mode 0 */
      outw(0x3CE, 0x0506);    /* set graphics */
      slowbcopy_tobus(hwp->FontInfo2, hwp->Base, FONT_AMOUNT);
    }
#endif
#ifdef SAVE_TEXT
    if (hwp->TextInfo) {
      outw(0x3C4, 0x0102);    /* write to plane 0 */
      outw(0x3C4, 0x0604);    /* enable plane graphics */
      outw(0x3CE, 0x0004);    /* read plane 0 */
      outw(0x3CE, 0x0005);    /* write mode 0, read mode 0 */
      outw(0x3CE, 0x0506);    /* set graphics */
      slowbcopy_tobus(hwp->TextInfo, hwp->Base, TEXT_AMOUNT);
      outw(0x3C4, 0x0202);    /* write to plane 1 */
      outw(0x3C4, 0x0604);    /* enable plane graphics */
      outw(0x3CE, 0x0104);    /* read plane 1 */
      outw(0x3CE, 0x0005);    /* write mode 0, read mode 0 */
      outw(0x3CE, 0x0506);    /* set graphics */
      slowbcopy_tobus((unsigned char *)hwp->TextInfo + TEXT_AMOUNT,
        hwp->Base, TEXT_AMOUNT);
    }
#endif
  }
#endif /* defined(SAVE_TEXT) || defined(SAVE_FONT1) || defined(SAVE_FONT2) */

  VGANAME(vgaHWBlankScreen)(scrninfp, TRUE);

#if !defined(PC98_PEGC) && !defined(PC98_EGC)

  tmp = inb(0x3D0 + 0x0A);			/* Reset flip-flop */
  outb(0x3C0, 0x00);				/* Enables pallete access */

  if (restore->MiscOutReg & 0x01)
    hwp->IOBase = 0x3D0;
  else
    hwp->IOBase = 0x3B0;

  outb(0x3C2, restore->MiscOutReg);

  for (i=1; i<5;  i++) outw(0x3C4, (restore->Sequencer[i] << 8) | i);
  
  /* Ensure CRTC registers 0-7 are unlocked by clearing bit 7 or CRTC[17] */

  outw(hwp->IOBase + 4, ((restore->CRTC[17] & 0x7F) << 8) | 17);

  for (i=0; i<25; i++) outw(hwp->IOBase + 4,(restore->CRTC[i] << 8) | i);

  for (i=0; i<9;  i++) outw(0x3CE, (restore->Graphics[i] << 8) | i);

  for (i=0; i<21; i++) {
    tmp = inb(hwp->IOBase + 0x0A);
    outb(0x3C0,i); outb(0x3C0, restore->Attribute[i]);
  }
  
#if 0
  if (clgd6225Lcd)
  {
    for (i= 0; i<768; i++)
    {
      /* The LCD doesn't like white */
      if (restore->DAC[i] == 63) restore->DAC[i]= 62;
    }
  }
#endif

  outb(0x3C6,0xFF);
  outb(0x3C8,0x00);
  for (i=0; i<768; i++)
  {
     outb(0x3C9, restore->DAC[i]);
     DACDelay(hwp);
  }

  /* Turn on PAS bit */
  tmp = inb(hwp->IOBase + 0x0A);
  outb(0x3C0, 0x20);

#endif /* !defined(PC98_PEGC) && !defined(PC98_EGC) */

}

/*
 * vgaHWSave --
 *      save the current video mode
 */

void
VGANAME(vgaHWSave)(ScrnInfoPtr scrninfp, vgaRegPtr save, Bool saveFonts)
{
  static Bool	first_time = TRUE;
  int           i,tmp;
  vgaHWPtr      hwp = VGAHWPTR(scrninfp);

  if (save == NULL) {
    return;
  }

#if !defined(PC98_PEGC) && !defined(PC98_EGC)
  save->MiscOutReg = inb(0x3CC);
#ifdef PC98
  save->MiscOutReg |= 0x01;
#endif
  hwp->IOBase = (save->MiscOutReg & 0x01) ? 0x3D0 : 0x3B0;

  tmp = inb(hwp->IOBase + 0x0A); /* reset flip-flop */
  outb(0x3C0, 0x00);

#ifdef NEED_SAVED_CMAP
  /*
   * Some recent (1991) ET4000 chips have a HW bug that prevents the reading
   * of the color lookup table.  Mask rev 9042EAI is known to have this bug.
   *
   * XF86 already keeps track of the contents of the color lookup table so
   * reading the HW isn't needed.  Therefore, as a workaround for this HW
   * bug, the following (correct) code has been #ifdef'ed out.  This is also
   * a valid change for ET4000 chips that don't have the HW bug.  The code
   * is really just being retained for reference.  MWS 22-Aug-91
   *
   * This is *NOT* true for 386BSD, Mach -- the initial colour map must be
   * restored.  When saving the text mode, we check if the colourmap is
   * readable.  If so we read it.  If not, we set the saved map to a
   * default map (taken from Ferraro's "Programmer's Guide to the EGA and
   * VGA Cards" 2nd ed).
   */

  if (first_time)
  {
    int read_error = 0;

    outb(0x3C6,0xFF);
    /*
     * check if we can read the lookup table
     */
    outb(0x3C7,0x00);
    for (i=0; i<3; i++) save->DAC[i] = inb(0x3C9);
#if defined(PC98_PW) || defined(PC98_XKB) || defined(PC98_NEC) || defined(PC98_PWLB)
    for (i=0; i<300; i++) inb(hwp->IOBase + 4);
#endif
    outb(0x3C8,0x00);
    for (i=0; i<3; i++) outb(0x3C9, ~save->DAC[i] & vgaRamdacMask);
#if defined(PC98_PW) || defined(PC98_XKB) || defined(PC98_NEC) || defined(PC98_PWLB)
    for (i=0; i<300; i++) inb(hwp->IOBase + 4);
#endif
    outb(0x3C7,0x00);
    for (i=0; i<3; i++)
    {
      unsigned char tmp2 = inb(0x3C9);
#if defined(PC98_PW) || defined(PC98_XKB) || defined(PC98_NEC) || defined(PC98_PWLB)
      if (tmp2 != (~save->DAC[i]&0xFF))
#endif
      if (tmp2 != (~save->DAC[i] & vgaRamdacMask)) read_error++;
    }
  
    if (read_error)
    {
      /*			 
       * save the default lookup table
       */
      memmove(save->DAC, defaultDAC, 768);
      ErrorF("%s: Cannot read colourmap from VGA.",scrninfp->name);
      ErrorF("  Will restore with default\n");
    }
    else
    {
      /*			 
       * save the colorlookuptable 
       */
      outb(0x3C7,0x01);
      for (i=3; i<768; i++)
      {
	save->DAC[i] = inb(0x3C9); 
	DACDelay(hwp);
      }
    }
  }
  first_time = FALSE;
#endif /* NEED_SAVED_CMAP */

  for (i=0; i<25; i++) { outb(hwp->IOBase + 4,i);
			 save->CRTC[i] = inb(hwp->IOBase + 5); }

  for (i=0; i<21; i++) {
    tmp = inb(hwp->IOBase + 0x0A);
    outb(0x3C0,i);
    save->Attribute[i] = inb(0x3C1);
  }

  for (i=0; i<9;  i++) { outb(0x3CE,i); save->Graphics[i]  = inb(0x3CF); }

  for (i=0; i<5;  i++) { outb(0x3C4,i); save->Sequencer[i]   = inb(0x3C5); }

#endif /* !defined(PC98_PEGC) && !defined(PC98_EGC) */

  VGANAME(vgaHWBlankScreen)(scrninfp, FALSE);
  
#if !defined(PC98_PEGC) && !defined(PC98_EGC)
  outb(0x3C2, save->MiscOutReg | 0x01);		/* shift to colour emulation */
  /* Since forced to colour mode, must use 0x3Dx instead of (vgaIOBase + x) */

#if defined(SAVE_TEXT) || defined(SAVE_FONT1) || defined(SAVE_FONT2)
  /*
   * get the character sets, and text screen if required
   */
  if (saveFonts && !(save->Attribute[0x10] & 0x01)) {
#ifdef SAVE_FONT1
#if defined(BIT_PLANE) && (BIT_PLANE != 2)
    if (scrninfp->depth > 1)
#endif
    if (hwp->FontInfo1 || (hwp->FontInfo1 = (pointer)xalloc(FONT_AMOUNT))) {
      /*
       * Here we temporarily switch to 16 colour planar mode, to simply
       * copy the font-info
       *
       * BUG ALERT: The (S)VGA's segment-select register MUST be set correctly!
       */
      tmp = inb(0x3D0 + 0x0A); /* reset flip-flop */
      outb(0x3C0,0x30); outb(0x3C0, 0x01); /* graphics mode */
      outw(0x3C4, 0x0402);    /* write to plane 2 */
      outw(0x3C4, 0x0604);    /* enable plane graphics */
      outw(0x3CE, 0x0204);    /* read plane 2 */
      outw(0x3CE, 0x0005);    /* write mode 0, read mode 0 */
      outw(0x3CE, 0x0506);    /* set graphics */
      slowbcopy_frombus(hwp->Base, hwp->FontInfo1, FONT_AMOUNT);
    }
#endif /* SAVE_FONT1 */
#ifdef SAVE_FONT2
#if defined(BIT_PLANE) && (BIT_PLANE != 3)
    if (scrninfp->depth > 1)
#endif
    if (hwp->FontInfo2 || (hwp->FontInfo2 = (pointer)xalloc(FONT_AMOUNT))) {
      tmp = inb(0x3D0 + 0x0A); /* reset flip-flop */
      outb(0x3C0,0x30); outb(0x3C0, 0x01); /* graphics mode */
      outw(0x3C4, 0x0802);    /* write to plane 3 */
      outw(0x3C4, 0x0604);    /* enable plane graphics */
      outw(0x3CE, 0x0304);    /* read plane 3 */
      outw(0x3CE, 0x0005);    /* write mode 0, read mode 0 */
      outw(0x3CE, 0x0506);    /* set graphics */
      slowbcopy_frombus(hwp->Base, hwp->FontInfo2, FONT_AMOUNT);
    }
#endif /* SAVE_FONT2 */
#ifdef SAVE_TEXT
#if defined(BIT_PLANE) && (BIT_PLANE != 0) && (BIT_PLANE != 1)
    if (scrninfp->depth > 1)
#endif
    if (hwp->TextInfo || (hwp->TextInfo = (pointer)xalloc(2 * TEXT_AMOUNT))) {
      tmp = inb(0x3D0 + 0x0A); /* reset flip-flop */
      outb(0x3C0,0x30); outb(0x3C0, 0x01); /* graphics mode */
      /*
       * This is a quick hack to save the text screen for system that don't
       * restore it automatically.
       */
      outw(0x3C4, 0x0102);    /* write to plane 0 */
      outw(0x3C4, 0x0604);    /* enable plane graphics */
      outw(0x3CE, 0x0004);    /* read plane 0 */
      outw(0x3CE, 0x0005);    /* write mode 0, read mode 0 */
      outw(0x3CE, 0x0506);    /* set graphics */
      slowbcopy_frombus(hwp->Base, hwp->TextInfo, TEXT_AMOUNT);
      outw(0x3C4, 0x0202);    /* write to plane 1 */
      outw(0x3C4, 0x0604);    /* enable plane graphics */
      outw(0x3CE, 0x0104);    /* read plane 1 */
      outw(0x3CE, 0x0005);    /* write mode 0, read mode 0 */
      outw(0x3CE, 0x0506);    /* set graphics */
      slowbcopy_frombus(hwp->Base,
        (unsigned char *)hwp->TextInfo + TEXT_AMOUNT, TEXT_AMOUNT);
    }
#endif /* SAVE_TEXT */

    /* Restore clobbered registers */
    tmp = inb(0x3D0 + 0x0A);
    outb(0x3C0, 0x30);  outb(0x3C0, save->Attribute[0]);
    outw(0x3C4, (save->Sequencer[2] << 8) | 0x02);
    outw(0x3C4, (save->Sequencer[4] << 8) | 0x04);
    outw(0x3CE, (save->Graphics[4] << 8) | 0x04);
    outw(0x3CE, (save->Graphics[5] << 8) | 0x05);
    outw(0x3CE, (save->Graphics[6] << 8) | 0x06);
  }
#endif /* defined(SAVE_TEXT) || defined(SAVE_FONT1) || defined(SAVE_FONT2) */

  outb(0x3C2, save->MiscOutReg);		/* back to original setting */
#endif /* !defined(PC98_PEGC) && !defined(PC98_EGC) */
  
  VGANAME(vgaHWBlankScreen)(scrninfp, TRUE);

#if !defined(PC98_PEGC) && !defined(PC98_EGC)
  /* Turn on PAS bit */
  tmp = inb(hwp->IOBase + 0x0A);
  outb(0x3C0, 0x20);
#endif /* !defined(PC98_PEGC) && !defined(PC98_EGC) */
  
}



/*
 * vgaHWInit --
 *      Handle the initialization, etc. of a screen.
 *      Return FALSE on failure.
 */

#ifndef VGA_MMIO
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
        /* Initialise the Mono map according to which bit-plane gets written to */

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
#if 0
#ifdef PC98_EGC
    /* There should be no writing of registers in this function */
    outb (0x7c, 0x00);
    /* set to 16color mode */
    outb(0x6a, 0x01);
    /* set EGC enable */
    outb(0x7c, 0xc0); /* GRCG enable, RMW mode */
    outb(0x6a, 0x07);
    outb(0x6a, 0x05);
    outb(0x6a, 0x06);

    for (i = 0; i < 4 ; i++)
        {
            outb (0x7e,0xff);
        }

    /* set up VGA registers */

    outw (EGC_READ, 0x0000) ;
    outw (EGC_MASK, 0xffff) ;
    outw (EGC_READ ,0x40ff) ;
    outw (EGC_ADD, 0x0000) ;
    outw (EGC_LENGTH, 0x000f) ;
#endif /* PC98_EGC */
#endif
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
    VGAHWPTRLVAL(scrp) = xnfcalloc(sizeof(vgaHWRec), 1);
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
	VGAHWPTR(scrp)->ShowOverscan = TRUE;
    } else
	VGAHWPTR(scrp)->ShowOverscan = FALSE;
	
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
    
#if defined(PC98_WAB) || defined(PC98_WABEP)
    hwp->Base = xf86MapVidMem(scr_index, VIDMEM_FRAMEBUFFER, (pointer)0xE0000,
			    hwp->MapSize);
#elif defined(PC98_GANB_WAP) || defined(PC98_WSNA) || defined(PC98_NKVNEC)
    hwp->Base = xf86MapVidMem(scr_index, VIDMEM_FRAMEBUFFER, (pointer)0xF00000,
			    hwp->MapSize);
#elif defined(PC98_TGUI)
    if(!vgaUseLinearAddressing && pc98PvramBase != NULL)
        hwp->Base = xf86MapVidMem(scr_index, VIDMEM_FRAMEBUFFER,
                                pc98PvramBase, hwp->MapSize);
#elif defined(PC98_EGC) || defined(PC98_PEGC)
    hwp->Base = xf86MapVidMem(scr_index, VIDMEM_FRAMEBUFFER, (pointer)0xA8000,
			    hwp->MapSize);
#else    
    hwp->Base = xf86MapVidMem(scr_index, VIDMEM_FRAMEBUFFER, (pointer)0xA0000,
			    hwp->MapSize);
#endif
    return hwp->Base != NULL;
}


void
vgaHWUnmapMem(ScrnInfoPtr scrp)
{
    vgaHWPtr hwp = VGAHWPTR(scrp);
    int scr_index = scrp->scrnIndex;

    xf86UnMapVidMem(scr_index, (pointer)hwp->Base, hwp->MapSize);
}

int
vgaHWGetIndex()
{
    return vgaHWPrivateIndex;
}


#endif /* VGA_MMIO */
 
void
VGANAME(vgaHWGetIOBase)(vgaHWPtr hwp)
{
    hwp->IOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
}



void
VGANAME(vgaHWLock)(vgaHWPtr hwp)
{
    unsigned char temp;

#ifndef PC98_EGC
    /* Protect CRTC[0-7] */
    outb(hwp->IOBase + 4, 0x11);
    temp = inb(hwp->IOBase + 5);
    outb(hwp->IOBase + 5, temp & 0x7F);
#endif
}

void
VGANAME(vgaHWUnlock)(vgaHWPtr hwp)
{
    unsigned char temp;

#ifndef PC98_EGC
    /* Unprotect CRTC[0-7] */
    outb(hwp->IOBase + 4, 0x11);
    temp = inb(hwp->IOBase + 5);
    outb(hwp->IOBase + 5, (temp & 0x7F) | 0x80);
#endif
}



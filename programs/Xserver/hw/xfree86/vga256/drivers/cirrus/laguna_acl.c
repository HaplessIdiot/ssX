/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/laguna_acl.c,v 3.2 1997/01/08 20:35:45 dawes Exp $ */

/*
 * New-style acceleration for the Laguna-family (CL-GD5462/5464).
 */

#include "vga256.h"
#include "xf86.h"
#include "vga.h"

#include "xf86xaa.h"

#include "cir_driver.h"
#include "cir_blitLG.h"

#include <stdio.h>

/* Hardware 8x8 mono color expansion patterns are now working in XAA (3.2q),
   but I haven't been able to debug the code for the Laguna yet.  This
   probably will be the case when the 3.2A binaries are released.  Ah, well.
*/
#define USE_PAT_COLEXP 0
#define DEBUG_MONOPAT 0

/* Do we really want to check the command FIFO on the laguna part?  Doing
   so produces a PCI read, which is a real performance pinch-point.  And 
   the PCI bus will just keep the extra requests floating around until the
   FIFO has drained, so it _should_ be safe.  OTOH, if the PCI bus is busy
   with a flood of Laguna commands with no where to write them, then no one
   _else_ can use the PCI bus (like sound cards, _network_ cards, etc).

   With limited testing, it appears that setting ALWAYS_CHECK_FIFO to 0
   is safe.  The PCI bus never locked during a large number of screen-screen
   copies, colexp fills, etc.  For now, we'll stick with this setting.
   (3.2p)
*/
#define ALWAYS_CHECK_FIFO   0


void LagunaSync();
void LagunaWaitQAvail();
void LagunaSetupForFillRectSolid();
void LagunaSubsequentFillRectSolid();
void LagunaSetupForScreenToScreenCopy();
void LagunaSubsequentScreenToScreenCopy();
void LagunaSetupForCPUToScreenColorExpand();
void LagunaSubsequentCPUToScreenColorExpand();
void LagunaSetupForScreenToScreenColorExpand();
void LagunaSubsequentScreenToScreenColorExpand();
void LagunaSetupForFill8x8Pattern();
void LagunaSubsequentFill8x8Pattern();
#if USE_PAT_COLEXP
void LagunaSetupFor8x8PatternColorExpand();
void LagunaSubsequent8x8PatternColorExpand();
#endif
void LagunaImageWrite();



/* Cirrus raster operations. */
int lgCirrusRop[16] = {
	LGROP_0,		/* GXclear */
	LGROP_AND,		/* GXand */
	LGROP_SRC_AND_NOT_DST,	/* GXandReverse */
	LGROP_SRC,		/* GXcopy */
	LGROP_NOT_SRC_AND_DST,	/* GXandInverted */
	LGROP_DST,		/* GXnoop */
	LGROP_XOR,		/* GXxor */
	LGROP_OR,		/* GXor */
	LGROP_NOR,		/* GXnor */
	LGROP_XNOR,		/* GXequiv */
	LGROP_NOT_DST,		/* GXinvert */
	LGROP_SRC_OR_NOT_DST,	/* GXorReverse */
	LGROP_NOT_SRC,		/* GXcopyInverted */
	LGROP_NOT_SRC_OR_DST,	/* GXorInverted */
	LGROP_NAND,		/* GXnand */
	LGROP_1			/* GXset */
};


/* Cirrus raster operations.  These are ROPs for the Pattern fetch unit. */
int lgCirrusPatRop[16] = {
  LGPATROP_0,			/* GXclear */
  LGPATROP_AND,			/* GXand */
  LGPATROP_SRC_AND_NOT_DST,	/* GXandReverse */
  LGPATROP_SRC,			/* GXcopy */
  LGPATROP_NOT_SRC_AND_DST,	/* GXandInverted */
  LGPATROP_DST,			/* GXnoop */
  LGPATROP_XOR,			/* GXxor */
  LGPATROP_OR,			/* GXor */
  LGPATROP_NOR,			/* GXnor */
  LGPATROP_XNOR,		/* GXequiv */
  LGPATROP_NOT_DST,		/* GXinvert */
  LGPATROP_SRC_OR_NOT_DST,	/* GXorReverse */
  LGPATROP_NOT_SRC,		/* GXcopyInverted */
  LGPATROP_NOT_SRC_OR_DST,	/* GXorInverted */
  LGPATROP_NAND,		/* GXnand */
  LGPATROP_1			/* GXset */
};


/* These varibles are used by several of the laguna functions.  They're
   used to remember properties of the blit mode. */
static int blitxdir, blitydir, blittransparent;


void LagunaAccelInit() {

  /* General flags, functions */
  xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS | PIXMAP_CACHE
    | ONLY_LEFT_TO_RIGHT_BITBLT | HARDWARE_PATTERN_SCREEN_ORIGIN |
      HARDWARE_PATTERN_PROGRAMMED_ORIGIN | HARDWARE_PATTERN_TRANSPARENCY | 
	HARDWARE_PATTERN_MONO_TRANSPARENCY | HARDWARE_PATTERN_PROGRAMMED_BITS;
  if (cirrusChip == CLGD5464 && vga256InfoRec.bitsPerPixel == 24)
    xf86AccelInfoRec.Flags |= NO_TRANSPARENCY;
  xf86AccelInfoRec.Sync = LagunaSync;


  /* Solid Color fills */
/*  xf86GCInfoRec.PolyFillRectSolidFlags |= NO_PLANEMASK;
*/
  xf86AccelInfoRec.SetupForFillRectSolid = LagunaSetupForFillRectSolid;
  xf86AccelInfoRec.SubsequentFillRectSolid = LagunaSubsequentFillRectSolid;


  /* Screen-to-screen copies */
/*  xf86GCInfoRec.CopyAreaFlags |= NO_PLANEMASK;
*/
  xf86AccelInfoRec.SetupForScreenToScreenCopy =
    LagunaSetupForScreenToScreenCopy;
  xf86AccelInfoRec.SubsequentScreenToScreenCopy =
    LagunaSubsequentScreenToScreenCopy;


  /* Color expansion. */
  xf86AccelInfoRec.ColorExpandFlags =
    SCANLINE_PAD_DWORD | CPU_TRANSFER_PAD_DWORD |
      BIT_ORDER_IN_BYTE_LSBFIRST | VIDEO_SOURCE_GRANULARITY_DWORD;

  xf86AccelInfoRec.SetupForCPUToScreenColorExpand =
    LagunaSetupForCPUToScreenColorExpand;
  xf86AccelInfoRec.SubsequentCPUToScreenColorExpand =
    LagunaSubsequentCPUToScreenColorExpand;

  xf86AccelInfoRec.SetupForScreenToScreenColorExpand =
    LagunaSetupForScreenToScreenColorExpand;
  xf86AccelInfoRec.SubsequentScreenToScreenColorExpand =
    LagunaSubsequentScreenToScreenColorExpand;

  xf86AccelInfoRec.CPUToScreenColorExpandBase = (unsigned int *)
    (cirrusMMIOBase + HOSTDATA);
  xf86AccelInfoRec.CPUToScreenColorExpandRange = HOSTDATASIZE;


  /* 8x8 Color Pattern Fills */
  if (!(vga256InfoRec.bitsPerPixel == 24 ||
	vga256InfoRec.bitsPerPixel == 32)) {
    /* Only 8 and 16bpp pattern fills are supported.  24 and 32 bpp
       pattern fills require their patterns to be on _two_ adjacent
       lines in video memory.  XAA currently doesn't support this
       mode for pattern caching (as of 3.2i). */
    xf86AccelInfoRec.SetupForFill8x8Pattern = 
      LagunaSetupForFill8x8Pattern;
    xf86AccelInfoRec.SubsequentFill8x8Pattern = 
      LagunaSubsequentFill8x8Pattern;
  }
#if USE_PAT_COLEXP
  /* According to Harm's NOTES file, 8x8 colexp pattern fills aren't
     working yet (12/17/96) */
  xf86AccelInfoRec.SetupFor8x8PatternColorExpand =
    LagunaSetupFor8x8PatternColorExpand;
  xf86AccelInfoRec.Subsequent8x8PatternColorExpand =
    LagunaSubsequent8x8PatternColorExpand;
#endif


  /* PixMap caching and CPU-to-screen transfers. */
  /* THe '62 had some host-to-screen transfer problems. */
  if (cirrusChip != CLGD5462)
    xf86AccelInfoRec.ImageWrite = LagunaImageWrite;

  xf86InitPixmapCache(&vga256InfoRec, vga256InfoRec.virtualY *
      vga256InfoRec.displayWidth * vga256InfoRec.bitsPerPixel / 8,
      vga256InfoRec.videoRam * 1024 - 1024);
}


/* Laguna chip status query routines */

int LgReady(void)
{
  volatile unsigned char status;

  status = *(unsigned char *)(cirrusMMIOBase + STATUS);
  if (status & 0x07)
    return 0;
  else
    return 1;
}

void LgSetBitmask(unsigned int m)
{
  static unsigned int oldMask = 0xFFFFFFFF;

  if (m != oldMask) {
    LgSETBITMASK(m);
    oldMask = m;
  }
}


void LagunaSync() {
    while (!LgReady());
}

void LagunaWaitQAvail(int n) {
#if ALWAYS_CHECK_FIFO
  /* Wait until n entries are open in the command queue */

  volatile unsigned char qfree;

  do
    qfree = *(unsigned char *)(cirrusMMIOBase + QFREE);
  while (qfree < n);
#endif
}
  

/* Solid color rectangle fill */
void LagunaSetupForFillRectSolid(color, rop, planemask)
    int color, rop, planemask;
{
    switch (vga256InfoRec.bitsPerPixel) {
    case 8 :
        color &= 0xFF;
        color |= (color << 8) | (color << 16) | (color << 24);
        break;
    case 16 :
        color &= 0xFFFF;
        color |= (color << 16);
        break;
    default :
        break;
    }

    /* Yes, solid color fills take their colors from the _background_
       color. */
    LgSETBACKGROUND(color);
    LgSETROP(lgCirrusRop[rop]);
    LgSETMODE(SCR2SCR | COLORFILL);
    LgSetBitmask(planemask);
}

void LagunaSubsequentFillRectSolid(x, y, w, h)
    int x, y, w, h;
{
  /* Wait for room in the command queue. */
  LagunaWaitQAvail(2);

  LgSETDSTXY(x, y);
  LgSETEXTENTS(w, h);
}


/* Screen-to-screen transfers */ 
void LagunaSetupForScreenToScreenCopy(xdir, ydir, rop, planemask,
				      transparency_color)
    int xdir, ydir;
    int rop;
    unsigned planemask;
    int transparency_color;
{
    int bltmode;
    LagunaSync();

    blittransparent = (transparency_color != -1);
    blitxdir = xdir;
    blitydir = ydir;
    bltmode = 0;
    if (ydir < 0)
      bltmode |= BLITUP;
    if (blittransparent) {
      bltmode |= COLORTRANS;
      if (cirrusChip != CLGD5462)
	bltmode |= PATeqSRC;
    }
    LgSETROP(lgCirrusRop[rop]);
    LgSETMODE(SCR2SCR | COLORSRC | bltmode);
    LgSetBitmask(planemask);
}

void LagunaSubsequentScreenToScreenCopy(x1, y1, x2, y2, w, h)
    int x1, y1, x2, y2, w, h;
{
    /*
     * We have set the flag indicating that xdir must be one,
     * so we can assume that here.
     */
    if (blitydir == -1) {
        y1 += h - 1;
        y2 += h - 1;
    }

    if (blittransparent) {
      /* We're doing a transparent blit.  We'll need to point
	 OP2 to the color compare mask. */
      LagunaWaitQAvail(4);
      LgSETTRANSMASK(x1, y1);
    } else {
      LagunaWaitQAvail(3);
    }
    LgSETSRCXY(x1, y1);
    LgSETDSTXY(x2, y2);
    LgSETEXTENTS(w, h);
}



/*
 * CPU-to-screen color expansion.
 */
void LagunaSetupForCPUToScreenColorExpand(bg, fg, rop, planemask)
    int bg, fg, rop;
    unsigned int planemask;
{
    int trans = (bg == -1)?TRANSBG:TRANSNONE;

    switch (vga256InfoRec.bitsPerPixel) {
    case 8:
      fg &= 0xFF;
      fg |= (fg << 8) | (fg << 16) | (fg << 24);
      if (bg != -1) {
	bg &= 0xFF;
	bg |= (bg << 8) | (bg << 16) | (bg << 24);
      }
      break;

    case 16:
      fg &= 0xFFFF;
      fg |= (fg << 16);
      if (bg != -1) {
	bg &= 0xFFFF;
	bg |= (bg << 16);
      }
      break;
    }

    if (trans == TRANSNONE) {
        LgSETBACKGROUND(bg);
    }
    LgSETFOREGROUND(fg);

    /* We can do transparency blits using the Pattern fetch unit. */
    LgSETROP(lgCirrusPatRop[rop] | trans);
    LgSETPHASE2(0);
    LgSETMODE(HOST2PAT | MONOPAT);
    LgSetBitmask(planemask);
}

void LagunaSubsequentCPUToScreenColorExpand(x, y, w, h, skipleft)
    int x, y, w, h, skipleft;
{
  /*
   * I assume the higher level code has called Sync(), which is
   * a reasonable assumption since it transferred the CPU data.
   */


  LgSETDSTXY(x, y);
  LgSETEXTENTS(w, h);
}


/* Screen-to-screen color expansion. */

void LagunaSetupForScreenToScreenColorExpand(bg, fg, rop, planemask)
    int bg, fg, rop;
    unsigned int planemask;
{
  int trans = (bg == -1)?TRANSBG:TRANSNONE;
  int bltmode = 0;
  
  blittransparent = (trans == TRANSBG);

  switch (vga256InfoRec.bitsPerPixel) {
  case 8 :
    fg &= 0xFF;
    fg |= (fg << 8) | (fg << 16) | (fg << 24);
    if (bg != -1) {
      bg &= 0xFF;
      bg |= (bg << 8) | (bg << 16) | (bg << 24);
    }
    break;

  case 16:
    fg &= 0xFFFF;
    fg |= (fg << 16);
    if (bg != -1) {
      bg &= 0xFFFF;
      bg |= (bg << 16);
    }
    break;
  }

  if (trans == TRANSNONE) {
    LgSETBACKGROUND(bg);
  } else {
    bltmode |= COLORTRANS;
    if (cirrusChip != CLGD5462)
      bltmode |= PATeqSRC;
  }

  LgSETFOREGROUND(fg);
  LgSETROP(lgCirrusRop[rop] | trans);
  LgSETMODE(SCR2SCR | MONOSRC | bltmode);
  LgSetBitmask(planemask);
}

void LagunaSubsequentScreenToScreenColorExpand(srcx, srcy, x, y, w, h)
    int srcx, srcy, x, y, w, h;
{

  if (blittransparent) {
    LagunaWaitQAvail(4);
    LgSETMTRANSMASK(srcx, srcy);
  } else {
    LagunaWaitQAvail(3);
  }

  LgSETDSTXY(x, y);
  /* 'Monochrome' coordinates */
  LgSETMSRCXY(srcx, srcy);
  LgSETEXTENTS(w, h);
}




/* 8x8 color pattern fills */

void LagunaSetupForFill8x8Pattern(patternx, patterny, rop, planemask,
				  transparency_color)
     int patternx, patterny, rop, planemask, transparency_color;
{
  int bltmode = 0;
  int trans = TRANSNONE;


  blittransparent = (transparency_color != -1);

  if (transparency_color != -1) {
    int c = transparency_color;

    switch (vga256InfoRec.bitsPerPixel) {
    case 8:
      c &= 0xFF;
      c |= (c << 8) | (c << 16) | (c << 24);
      break;

    case 16:
      c &= 0xFFFF;
      c |= (c << 16);
      break;
    }

    /* Color transparency_color is transparent */
    LgSETBACKGROUND(c);
    trans = TRANSEQ;
  }
    
  LgSETROP(lgCirrusPatRop[rop] | trans);
  LgSETMODE(PAT2SCR | COLORPAT);
  LgSETPATXY(patternx, patterny);
  LgSetBitmask(planemask);
}

void LagunaSubsequentFill8x8Pattern(patternx, patterny, x, y, w, h)
     int patternx, patterny, x, y, w, h;
{
  /* The pattern source (patternx, patterny) won't change, because
     we've set HARDWARE_PATTERN_SOURCE_ORIGIN.  Although the Laguna
     chips can adjust the pattern origin (by writing the pattern
     offset into register PATOFF), XAA doesn't take advantage of
     this capability yet (3.2i). */

  LagunaWaitQAvail(3);
  LgSETPATOFF(patternx, patterny);
  LgSETDSTXY(x, y);
  LgSETEXTENTS(w, h);
}


#if USE_PAT_COLEXP
/* 8x8 mono pattern color expansion fills */

void LagunaLoadMonoPattern(int bits0_31, int bits32_63)
{
  /* Idea:  Load the pattern into the on-chip SRAM (SRAM2, for those
     following along in the book).  In subsequent pattern colexp's, 
     we'll just source SRAM2, and all will be well. */

  volatile unsigned long *pHOSTDATA = 
    (unsigned long *)(cirrusMMIOBase + HOSTDATA);

  LagunaSync();
  LagunaWaitQAvail(7);

  LgSetBitmask(0xFFFFFFFF);


#if 0
  LgSETFOREGROUND(0x00000000);
  LgSETBACKGROUND(0x01010101);

  LgSETMODE(0x6020);
  LgSETROP(0x00CC);
  LgSETSRAMDST(0);
  LgSETPHASE1(0);
  LgSETMEXTENTS(8, 1);
  *pHOSTDATA = 0x40404000;
  *pHOSTDATA = 0x00484878;
#else
  LgSETMODE(HOST2SRAM2);      /* Transfer into SRAM2 */
  LgSETROP(LGROP_SRC);        /* Copy bits directly */
  LgSETPHASE1(0);             /* Byte phase */
  LgSETSRAMDST(0);            /* Offset 0 into SRAM2 */
  
  LgSETMEXTENTS(8, 1);        /* 8 bytes (64 bits), 1 row */
  *pHOSTDATA = bits0_31;
  *pHOSTDATA = bits32_63;
#endif

  LagunaSync();

  LgSETMODE(0x110C);
  LgSETROP(0x00F0);
  LgSETSRAM2OFFSET(0);
  LgSETDSTXY(0x40, 0x40);
  LgSETEXTENTS(200, 200);


#if DEBUG_MONOPAT
  fprintf(stderr, "bits0_31 = 0x%08X  bits32_63 = 0x%08X\n", 
	  bits0_31, bits32_63);
#endif
}



void LagunaSetupFor8x8PatternColorExpand(patternx, patterny, bg, fg, rop,
					 planemask)
     int patternx, patterny, bg, fg, rop, planemask;
{
  int trans = (bg == -1)?TRANSBG:TRANSNONE;


  /* Load the mono pattern into SRAM */
  LagunaLoadMonoPattern(patternx, patterny);


  switch (vga256InfoRec.bitsPerPixel) {
  case 8 :
    fg &= 0xFF;
    fg |= (fg << 8) | (fg << 16) | (fg << 24);
    if (bg != -1) {
      bg &= 0xFF;
      bg |= (bg << 8) | (bg << 16) | (bg << 24);
    }
    break;

  case 16:
    fg &= 0xFFFF;
    fg |= (fg << 16);
    if (bg != -1) {
      bg &= 0xFFFF;
      bg |= (bg << 16);
    }
    break;
    
  }

  if (trans == TRANSNONE)
    LgSETBACKGROUND(bg);
  LgSETFOREGROUND(fg);

  /* 'Monochrome' source coordinates */
/*
  LgSETMPATXY(patternx, patterny);
*/
  LgSETSRAM2OFFSET(0);
  LgSETROP(lgCirrusPatRop[rop] | trans);
  LgSETMODE(SRAM2PAT2SCR | MONOPAT);
  LgSetBitmask(planemask);


#if DEBUG_MONOPAT
  fprintf(stderr, 
	  "fg = 0x%08X  bg = 0x%08X  patx = %d = 0x%08X  paty = %d = 0x%08X\n",
	 fg, bg, patternx, patternx, patterny, patterny);
#endif


}

void LagunaSubsequent8x8PatternColorExpand(patternx, patterny, x, y, w, h)
     int patternx, patterny, x, y, w, h;
{
  LagunaWaitQAvail(3);

  LgSETPATOFF(patternx, patterny);
  LgSETDSTXY(x, y);
  LgSETEXTENTS(w, h);
}
#endif /* USE_PAT_COLEXP */



/* ImageWrite.  Copy an area w pixels wide, h pixels high from src (on 
   the CPU) to the frame buffer, at point (x,y).  The source pixmap
   is srcwidth bytes wide.  The srcwidth may be larger than the 
   destination copy area. */

void LagunaImageWrite(x, y, w, h, src, srcwidth, rop, planemask)
    int x;
    int y;
    int w;
    int h;
    void *src;
    int srcwidth;
    int rop;
    unsigned planemask;
{
  volatile unsigned long *pHOSTDATA;
  unsigned long *pdSrc;
  unsigned char *pbSrc;
  int dwords, dwordTotal;
  int burst;
  
#if DEBUG_MONOPAT
  fprintf(stderr, "x = %d  y = %d  w = %d  h = %x  src = 0x%08X\n", 
	  x, y, w, h, src);
#endif

  /* Don't try any funny stuff */
  if (h == 0 || w == 0)
    return;

  /* Wait until the chip is idle. */
  LagunaSync();

  /* Setup the Laguna chip for a Host-to-screen copy */
  LgSETDSTXY(x, y);                  /* Destination */
  LgSETPHASE1(0);                    /* Assume only byte-alignment */
  LgSETMODE(HOST2SCR | COLORSRC);    /* Host-to-screen, color copy */
  LgSETROP(lgCirrusRop[rop]);        /* Straight copy, no frills */
  LgSetBitmask(planemask);

  LgSETEXTENTS(w, h);                /* Start watching HOSTDATA */

  pbSrc = (unsigned char *)src;

  dwordTotal = (w * vga256InfoRec.bitsPerPixel / 8 + 3) >> 2;
  burst = (dwordTotal < HOSTDATASIZE * 4);
  if (!burst)
    pHOSTDATA = (unsigned long *)(cirrusMMIOBase + HOSTDATA);

  while (h--) {
    /* Each scanline must be DWORD padded.  */
    dwords = dwordTotal;

    pdSrc = (unsigned long *)pbSrc;
    if (burst) {
      pHOSTDATA = (unsigned long *)(cirrusMMIOBase + HOSTDATA);
      while (dwords--)
	*pHOSTDATA++ = *pdSrc++;
    } else {
      while (dwords--)
	*pHOSTDATA = *pdSrc++;
    }
    pbSrc += srcwidth;
  }
}

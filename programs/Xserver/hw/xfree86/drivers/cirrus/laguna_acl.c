/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/laguna_acl.c,v 1.5 1997/09/15 07:18:50 hohndel Exp $ */

/*
 * New-style acceleration for the Laguna-family (CL-GD5462/5464).
 */
#include "compiler.h"

#include "vga256.h"
#include "xf86.h"
#include "vga.h"

#include "xf86xaa.h"

#include "cir_driver.h"
#include "cir_blitLG.h"


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
void LagunaSetupFor8x8PatternColorExpand();
void LagunaSubsequent8x8PatternColorExpand();
void LagunaSetupForImageWrite();
void LagunaSubsequentImageWrite();



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
    | ONLY_LEFT_TO_RIGHT_BITBLT;
  xf86AccelInfoRec.PatternFlags = HARDWARE_PATTERN_SCREEN_ORIGIN |
      HARDWARE_PATTERN_PROGRAMMED_ORIGIN | HARDWARE_PATTERN_TRANSPARENCY | 
	HARDWARE_PATTERN_MONO_TRANSPARENCY;
  if (cirrusChip == CLGD5464 && vga256InfoRec.bitsPerPixel == 24)
    xf86AccelInfoRec.Flags |= NO_TRANSPARENCY;
  xf86AccelInfoRec.Sync = LagunaSync;


  /* Solid Color fills */
  xf86AccelInfoRec.SetupForFillRectSolid = LagunaSetupForFillRectSolid;
  xf86AccelInfoRec.SubsequentFillRectSolid = LagunaSubsequentFillRectSolid;


  /* Screen-to-screen copies */
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

  xf86AccelInfoRec.CPUToScreenColorExpandBase = CIR_MMIO_READ32(HOSTDATA);
  xf86AccelInfoRec.CPUToScreenColorExpandRange = HOSTDATASIZE;


  /* 8x8 Color Pattern Fills */
  if (!(vga256InfoRec.bitsPerPixel == 24 ||
	vga256InfoRec.bitsPerPixel == 32)) {
    /* Only 8 and 16bpp pattern fills are supported.  24 and 32 bpp
       pattern fills require their patterns to be on _two_ adjacent
       lines in video memory.  XAA currently doesn't support this
       mode for pattern caching (as of 3.2t). */
    xf86AccelInfoRec.SetupForFill8x8Pattern = 
      LagunaSetupForFill8x8Pattern;
    xf86AccelInfoRec.SubsequentFill8x8Pattern = 
      LagunaSubsequentFill8x8Pattern;
  }

  xf86AccelInfoRec.SetupFor8x8PatternColorExpand =
    LagunaSetupFor8x8PatternColorExpand;
  xf86AccelInfoRec.Subsequent8x8PatternColorExpand =
    LagunaSubsequent8x8PatternColorExpand;


  /* ImageWrite acceleration */
  /* I'm not sure if the Lg can do transparency on host-to-screen
     blits.  We can worry about that later... */
  if (cirrusChip != CLGD5462) {
    xf86AccelInfoRec.ImageWriteFlags = NO_TRANSPARENCY;
    xf86AccelInfoRec.SetupForImageWrite = LagunaSetupForImageWrite;
    xf86AccelInfoRec.SubsequentImageWrite = LagunaSubsequentImageWrite;
    xf86AccelInfoRec.ImageWriteBase = 
      xf86AccelInfoRec.CPUToScreenColorExpandBase;
    xf86AccelInfoRec.ImageWriteRange = 
      xf86AccelInfoRec.CPUToScreenColorExpandRange;
  }


  /* PixMap caching and CPU-to-screen transfers. */
  xf86InitPixmapCache(&vga256InfoRec, vga256InfoRec.virtualY *
      vga256InfoRec.displayWidth * vga256InfoRec.bitsPerPixel / 8,
      vga256InfoRec.videoRam * 1024 - 1024);
}


/* Laguna chip status query routines */

int LgReady(void)
{
  volatile unsigned char status;

  status = CIR_MMIO_READ8(STATUS);
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
    qfree = CIR_MMIO_READ8(QFREE);
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

    LagunaWaitQAvail(4);

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

    LagunaWaitQAvail(3);

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
      LagunaWaitQAvail(1);
      LgSETBACKGROUND(bg);
    }
    LagunaWaitQAvail(5);
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

  LagunaWaitQAvail(2);

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
    LagunaWaitQAvail(1);
    LgSETBACKGROUND(bg);
  } else {
    bltmode |= COLORTRANS;
    if (cirrusChip != CLGD5462)
      bltmode |= PATeqSRC;
  }

  LagunaWaitQAvail(4);
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
    LagunaWaitQAvail(1);
    LgSETBACKGROUND(c);
    trans = TRANSEQ;
  }
    
  LagunaWaitQAvail(4);
  LgSETROP(lgCirrusPatRop[rop] | trans);
  LgSETMODE(PAT2SCR | COLORPAT);
  LgSETPATXY(patternx, patterny);
  LgSetBitmask(planemask);
}

void LagunaSubsequentFill8x8Pattern(patternx, patterny, x, y, w, h)
     int patternx, patterny, x, y, w, h;
{
  LagunaWaitQAvail(3);

  LgSETPATOFF(patternx, patterny);
  LgSETDSTXY(x, y);
  LgSETEXTENTS(w, h);
}



/* 8x8 mono pattern color expansion fills */
void LagunaSetupFor8x8PatternColorExpand(patternx, patterny, bg, fg, rop,
					 planemask)
     int patternx, patterny, bg, fg, rop, planemask;
{
  int trans = (bg == -1)?TRANSBG:TRANSNONE;


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
    LagunaWaitQAvail(1);
    LgSETBACKGROUND(bg);
  }

  LagunaWaitQAvail(5);

  LgSETFOREGROUND(fg);

  /* 'Monochrome' source coordinates */
  LgSETMPATXY(patternx, patterny);
  LgSETROP(lgCirrusPatRop[rop] | trans);
  LgSETMODE(PAT2SCR | MONOPAT);
  
  LgSetBitmask(planemask);


}

void LagunaSubsequent8x8PatternColorExpand(patternx, patterny, x, y, w, h)
     int patternx, patterny, x, y, w, h;
{
  LagunaWaitQAvail(3);

  LgSETPATOFF(patternx, patterny);
  LgSETDSTXY(x, y);
  LgSETEXTENTS(w, h);
}



void LagunaSetupForImageWrite(int rop, unsigned int planemask, 
			      int transparency_color) {

  LagunaWaitQAvail(4);

  LgSETPHASE1(0);                   /* Assume that the data starts on 
				       the DWORD boundary. */
  LgSETROP(lgCirrusRop[rop]);       /* We support all the raster ops */
  LgSETMODE(HOST2SCR | COLORSRC);   /* Host-to-screen, color copy */
  LgSetBitmask(planemask);          /* Is this ever _not_ 0xFFFFFFFF? */
}


void LagunaSubsequentImageWrite(int x, int y, int w, int h, int skipleft) {
  /* We don't support left edge clipping, so we can ignore skipleft. */
  
  LagunaWaitQAvail(2);

  LgSETDSTXY(x, y);
  LgSETEXTENTS(w, h);

  /* Boy, I hope that I get the right number of DWORDS now... */
}

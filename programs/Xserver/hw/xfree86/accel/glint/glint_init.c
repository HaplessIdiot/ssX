/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/glint_init.c,v 1.12 1997/11/01 15:04:32 hohndel Exp $ */
/*
 * Copyright 1997 by Alan Hourihane <alanh@fairlite.demon.co.uk>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *           Dirk Hohndel,   <hohndel@suse.de>
 *	     Stefan Dirsch,  <sndirsch@suse.de>
 *	     Helmut Fahrion, <hf@suse.de>
 *
 * this work is sponsored by S.u.S.E. GmbH, Fuerth, Elsa GmbH, Aachen and
 * Siemens Nixdorf Informationssysteme
 */

#include "glint_regs.h"
#define GLINT_SERVER
#include "IBMRGB.h"
#include "glint.h"
#include "xf86_Config.h"

#if DEBUG
#define MEMDEBUG 1
#endif

typedef struct {
	unsigned long glintRegs[0x200];
	unsigned long DacRegs[0x100];
} glintRegisters;
static glintRegisters SR;

#ifndef SUSE_3D
#define MEMSize glintInfoRec.virtualX*glintInfoRec.virtualY*glintInfoRec.bitsPerPixel/8
#else
#define MEMSize glintInfoRec.videoRam * 1024
#endif

/* if < graphicsmem then framebuffer is not correct !? */
/* #define  VGASize glintInfoRec.videoRam*1024 */
#define VGASize MEMSize 

static Bool glintInitialized = FALSE;
static Bool LUTInited = FALSE;
static LUTENTRY oldlut[256];
int VBlank;
int glintInitCursorFlag = TRUE;
int glintHDisplay;
extern struct glintmem glintmem;
extern int glintWeight;
extern int glintDisplayWidth;
extern Bool glintDoubleBufferMode;
void glintDumpRegs(void);
unsigned int glintSetLUT(int , unsigned int );
int pprod;
extern int coprotype;


#define PARTPROD(a,b,c) (((a)<<6) | ((b)<<3) | (c))

static int partprod500TX[] = {
	-1,
	PARTPROD(0,0,1), PARTPROD(0,0,2), PARTPROD(0,1,2), PARTPROD(0,0,3),
	PARTPROD(0,1,3), PARTPROD(0,2,3), PARTPROD(1,2,3), PARTPROD(0,0,4),
	PARTPROD(0,1,4), PARTPROD(0,2,4), PARTPROD(1,2,4), PARTPROD(0,3,4),
	PARTPROD(1,3,4), PARTPROD(2,3,4),              -1, PARTPROD(0,0,5), 
	PARTPROD(0,1,5), PARTPROD(0,2,5), PARTPROD(1,2,5), PARTPROD(0,3,5), 
	PARTPROD(1,3,5), PARTPROD(2,3,5),              -1, PARTPROD(0,4,5), 
	PARTPROD(1,4,5), PARTPROD(2,4,5), PARTPROD(3,4,5),              -1,
	             -1,              -1,              -1, PARTPROD(0,0,6), 
	PARTPROD(0,1,6), PARTPROD(0,2,6), PARTPROD(1,2,6), PARTPROD(0,3,6), 
	PARTPROD(1,3,6), PARTPROD(2,3,6),              -1, PARTPROD(0,4,6), 
	PARTPROD(1,4,6), PARTPROD(2,4,6),              -1, PARTPROD(3,4,6),
	             -1,              -1,              -1, PARTPROD(0,5,6), 
	PARTPROD(1,5,6), PARTPROD(2,5,6),              -1, PARTPROD(3,5,6), 
	             -1,              -1,              -1, PARTPROD(4,5,6), 
	             -1,              -1,              -1,              -1,
		     -1,              -1,              -1, PARTPROD(0,0,7), 
	             -1, PARTPROD(0,2,7), PARTPROD(1,2,7), PARTPROD(0,3,7), 
	PARTPROD(1,3,7), PARTPROD(2,3,7),              -1, PARTPROD(0,4,7),
	PARTPROD(1,4,7), PARTPROD(2,4,7),              -1, PARTPROD(3,4,7), 
	             -1,              -1,              -1, PARTPROD(0,5,7),
	PARTPROD(1,5,7), PARTPROD(2,5,7),              -1, PARTPROD(3,5,7), 
	             -1,              -1,              -1, PARTPROD(4,5,7),
	             -1,              -1,              -1,              -1,
		     -1,              -1,              -1, PARTPROD(0,6,7), 
	PARTPROD(1,6,7), PARTPROD(2,6,7),              -1, PARTPROD(3,6,7),
	             -1,              -1,              -1, PARTPROD(4,6,7), 
	             -1,              -1,              -1,              -1,
		     -1,              -1,              -1, PARTPROD(5,6,7), 
	             -1,              -1,              -1,              -1,
		     -1,              -1,              -1,              -1,
		     -1,              -1,              -1,              -1,
		     -1,              -1,              -1, PARTPROD(0,7,7)};
static int partprodPermedia[] = {
	-1,
	PARTPROD(0,0,1), PARTPROD(0,1,1), PARTPROD(1,1,1), PARTPROD(1,1,2),
	PARTPROD(1,2,2), PARTPROD(1,2,2), PARTPROD(1,2,3), PARTPROD(2,2,3),
	PARTPROD(1,3,3), PARTPROD(2,3,3),              -1, PARTPROD(3,3,3),
	PARTPROD(1,3,4), PARTPROD(2,3,4),              -1, PARTPROD(3,3,4), 
	PARTPROD(1,4,4), PARTPROD(2,4,4),              -1, PARTPROD(3,4,4), 
	             -1,              -1,              -1, PARTPROD(4,4,4), 
	PARTPROD(1,4,5), PARTPROD(2,4,5), PARTPROD(3,4,5),              -1,
	             -1,              -1,              -1, PARTPROD(4,4,5), 
	PARTPROD(1,5,5), PARTPROD(2,5,5),              -1, PARTPROD(3,5,5), 
	             -1,              -1,              -1, PARTPROD(4,5,5), 
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1, PARTPROD(5,5,5), 
	PARTPROD(1,5,6), PARTPROD(2,5,6),              -1, PARTPROD(3,5,6),
	             -1,              -1,              -1, PARTPROD(4,5,6),
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1, PARTPROD(5,5,6),
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1,
	             -1,              -1,              -1,              -1};

int
Shiftbpp(int value)
{
    /* shift horizontal timings for 64bit VRAM's or 32bit SGRAMs */
	
    int logbytesperaccess;
	
    if (IS_3DLABS_TX_MX_CLASS(coprotype))
	logbytesperaccess = 3;
    else if (IS_3DLABS_PM_FAMILY(coprotype))
	logbytesperaccess = 2;

    switch (glintInfoRec.bitsPerPixel) {
    case 8:
	value >>= logbytesperaccess;
	break;
    case 16:
	if (glintDoubleBufferMode)
	    value >>= (logbytesperaccess-2);
	else
	    value >>= (logbytesperaccess-1);
	break;
    case 32:
	value >>= (logbytesperaccess-2);
	break;
    }
    return (value);
}

void
glintCalcCRTCRegs(glintCRTCRegPtr crtcRegs, DisplayModePtr mode)
{
    int h_front_porch, v_front_porch;
    int h_sync_width, v_sync_width;

    h_front_porch = mode->CrtcHSyncStart - mode->CrtcHDisplay;
    v_front_porch = mode->CrtcVSyncStart - mode->CrtcVDisplay;
    h_sync_width  = mode->CrtcHSyncEnd - mode->CrtcHSyncStart;
    v_sync_width  = mode->CrtcVSyncEnd - mode->CrtcVSyncStart;

    crtcRegs->h_limit	= Shiftbpp(mode->CrtcHTotal);
    crtcRegs->h_sync_end	= Shiftbpp(h_front_porch + h_sync_width);
    crtcRegs->h_sync_start	= Shiftbpp(h_front_porch);
    crtcRegs->h_blank_end	= Shiftbpp(mode->CrtcHTotal-mode->CrtcHDisplay);

    crtcRegs->v_limit	= mode->CrtcVTotal;
    crtcRegs->v_sync_end	= v_front_porch + v_sync_width + 1;
    crtcRegs->v_sync_start	= v_front_porch + 1;
    crtcRegs->v_blank_end	= mode->CrtcVTotal - mode->CrtcVDisplay;
    VBlank = crtcRegs->v_blank_end;

    crtcRegs->pitch		= mode->CrtcHDisplay;

    if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
	crtcRegs->vtgpolarity = 
	    (((mode->Flags & V_PHSYNC) ? 0 : 2) << 2) | 
	    ((mode->Flags & V_PVSYNC) ? 0 : 2) | (0xb0);
    }
    else if (IS_3DLABS_PM_FAMILY(coprotype)) {
	crtcRegs->vtgpolarity = 
 	    (((mode->Flags & V_PHSYNC) ? 0x1 : 0x3) << 3) |  
 	    (((mode->Flags & V_PVSYNC) ? 0x1 : 0x3) << 5) | 1; 
	if (IS_3DLABS_PM2_CLASS(coprotype) && (glintInfoRec.bitsPerPixel > 8)) {
	    /* 64 bit pixel bus */
	    crtcRegs->vtgpolarity |= (1 << 16);
	}
    }

    crtcRegs->clock_sel = glintInfoRec.clock[mode->Clock];
    if (IS_3DLABS_PM_FAMILY(coprotype)) {
      /* crtcRegs->vclkctl = 0x03 | 
	 (((33*10000*6 + (crtcRegs->clock_sel-1)) / crtcRegs->clock_sel) << 2); */
      /* GL 1000 none Recovery time */
      crtcRegs->vclkctl = 0x03;
    }
    else {
	/*
	 * tell DAC to use the ICD chip clock 0 as ref clock 
	 * and set up some more video timining generator registers
	 */
	crtcRegs->vclkctl = 0x00;
	crtcRegs->vtgserialclk = 0x05;
	/*
	 * Different settings for Fire GL 3000 and Elsa Gloria cards
	 */
	if (OFLG_ISSET(OPTION_FIREGL3000, &glintInfoRec.options))
	  {
	    crtcRegs->fbmodesel = 0x907; /* ?way interleave */
	    crtcRegs->vtgmodectl = 0x00;
	  }
	else
	  {
	    crtcRegs->fbmodesel = 0xa07; /* 4way interleave */
	    crtcRegs->vtgmodectl = 0x44;
	  }
    }
}

void
glintSetCRTCRegs(glintCRTCRegPtr crtcRegs)
{
    unsigned long usData;

    if (IS_3DLABS_PM_FAMILY(coprotype)) {
	pprod = partprodPermedia[crtcRegs->pitch >> 5];
    } else {
	pprod = partprod500TX[crtcRegs->pitch >> 5];
    }
  
    /*
     * in order to set up a mode we need to do a few more
     * things here than just set up the CRTC regs...
     */
    GLINT_WAIT (2);
    if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
	GLINT_WRITE_REG(crtcRegs->vtgpolarity,	VTGPolarity);
    }
    else if (IS_3DLABS_PERMEDIA_CLASS(coprotype)) {
	GLINT_WRITE_REG(crtcRegs->vtgpolarity,	PMVideoControl);
    }

    if (pprod == -1) {
	/*
	 * Houston, we have a problem
	 *
	 * we need to figure out how to handle pitches we 
	 * can't handle. Later, comma, much.
	 */
	FatalError("Can't handle pitch %d\n",crtcRegs->pitch);
    }

    if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
      GLINT_WAIT (2);
	GLINT_WRITE_REG(pprod | 0x600,	LBReadMode);
	GLINT_WRITE_REG(0x01,		LBWriteMode);
   }
    GLINT_WAIT (3);
    GLINT_WRITE_REG(1,			FBWriteMode);
    GLINT_WRITE_REG(pprod,		FBReadMode);
    GLINT_WRITE_REG(glintInfoRec.displayWidth |
		    ((((glintInfoRec.videoRam * 1024) / 
			glintInfoRec.displayWidth))<<16),	ScreenSize);
    GLINT_WRITE_REG(2,			ScissorMode);

    /*
     * this one depends on the color depth
     */  
    if (IS_3DLABS_TX_MX_CLASS(coprotype)) {

      GLINT_WAIT (1);

	switch (glintInfoRec.bitsPerPixel) {
	case 8:
	    GLINT_WRITE_REG(0x400 | (0x0e << 2) | 0,DitherMode);
	    break;
	case 16:
	    GLINT_WRITE_REG(0x400 | (0x01 << 2) | 0,DitherMode);
	    break;
	case 32:
	    GLINT_WRITE_REG(0x400 | (0x00 << 2) | 0,DitherMode);
	    break;
	}
	GLINT_WAIT (31);
	GLINT_WRITE_REG(0x3000,		AlphaBlendMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	ColorDDAMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	TextureColorMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	TextureAddressMode);
	GLINT_WRITE_REG(UNIT_DISABLE,   TextureReadMode);
	GLINT_WRITE_REG(UNIT_DISABLE,   GLINTWindow);
	GLINT_WRITE_REG(UNIT_DISABLE,   AlphaBlendMode);
	GLINT_WRITE_REG(UNIT_DISABLE,   LogicalOpMode);
	GLINT_WRITE_REG(UNIT_DISABLE,   DepthMode);
	GLINT_WRITE_REG(UNIT_DISABLE,   RouterMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	FogMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	AntialiasMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	AlphaTestMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	StencilMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	AreaStippleMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	LineStippleMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	LogicalOpMode);
	GLINT_WRITE_REG(/*0x7b*/0,	DepthMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	StatisticMode);
	GLINT_WRITE_REG(0xc00,		FilterMode);
	GLINT_WRITE_REG(0xffffffff,	FBHardwareWriteMask);
	GLINT_WRITE_REG(0xffffffff,	FBSoftwareWriteMask);
	GLINT_WRITE_REG(UNIT_DISABLE,	RasterizerMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	GLINTDepth);
	GLINT_WRITE_REG(UNIT_DISABLE,	FBSourceOffset);
	GLINT_WRITE_REG(UNIT_DISABLE,	FBPixelOffset);
	GLINT_WRITE_REG(UNIT_DISABLE,	LBSourceOffset);
	GLINT_WRITE_REG(UNIT_DISABLE,	WindowOrigin);
	GLINT_WRITE_REG(UNIT_DISABLE,	FBWindowBase);
	GLINT_WRITE_REG(UNIT_DISABLE,	LBWindowBase);

	GLINT_WAIT (1);
	switch (glintInfoRec.bitsPerPixel) {
	case 8:
	    GLINT_WRITE_REG(0x2,	PixelSize);
	    break;
	case 16:
	    GLINT_WRITE_REG(0x1,	PixelSize);
	    break;
	case 32:
	    GLINT_WRITE_REG(0x0,	PixelSize);
	    break;
	}
    }
    else if (IS_3DLABS_PM_FAMILY(coprotype)) {
      GLINT_WAIT (1);
	switch (glintInfoRec.bitsPerPixel) {
	case 8:
	  GLINT_WRITE_REG(0x0, FBReadPixel); /* 8 Bits */
	  break;
	case 16:
	  GLINT_WRITE_REG(0x1, FBReadPixel); /* 16 Bits */
	  break;
	case 32:
	  GLINT_WRITE_REG(0x2, FBReadPixel); /* 32 Bits */
	  break;
	}

	GLINT_WAIT (31);

	GLINT_WRITE_REG(GWIN_DisableLBUpdate,   GLINTWindow); /* for performance */
	/*  Framebufferorganisation */
	GLINT_WRITE_REG(0x0,		DitherMode);
	GLINT_WRITE_REG(0x3000,		AlphaBlendMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	ColorDDAMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	TextureColorMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	TextureAddressMode);
	GLINT_WRITE_REG(UNIT_DISABLE,   TextureReadMode);
	GLINT_WRITE_REG(UNIT_DISABLE,   AlphaBlendMode);
	GLINT_WRITE_REG(UNIT_DISABLE,   LogicalOpMode);
	GLINT_WRITE_REG(UNIT_DISABLE,   DepthMode);
	GLINT_WRITE_REG(UNIT_DISABLE,   RouterMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	FogMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	AntialiasMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	AlphaTestMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	StencilMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	AreaStippleMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	LogicalOpMode);
	GLINT_WRITE_REG(/*0x7b*/0,	DepthMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	StatisticMode);
	GLINT_WRITE_REG(0xc00,		FilterMode);
	GLINT_WRITE_REG(0xffffffff,	FBHardwareWriteMask);
	GLINT_WRITE_REG(0xffffffff,	FBSoftwareWriteMask);
	GLINT_WRITE_REG(UNIT_DISABLE,	RasterizerMode);
	GLINT_WRITE_REG(UNIT_DISABLE,	GLINTDepth);
	GLINT_WRITE_REG(UNIT_DISABLE,	FBSourceOffset);
	GLINT_WRITE_REG(UNIT_DISABLE,	FBPixelOffset);
	GLINT_WRITE_REG(UNIT_DISABLE,	LBSourceOffset);
	GLINT_WRITE_REG(UNIT_DISABLE,	WindowOrigin);
	GLINT_WRITE_REG(UNIT_DISABLE,	FBWindowBase);
	GLINT_WRITE_REG(UNIT_DISABLE,	LBWindowBase);
    }

    if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
      GLINT_WAIT (24);
	GLINT_WRITE_REG(0x0,		TextureAddressMode);
	GLINT_WRITE_REG(0x0,		TextureReadMode);
	GLINT_WRITE_REG(0x0,		RouterMode);
	GLINT_WRITE_REG(0x0,		PatternRamMode);

	/*
	 * this sets the GLINT to do FIFO Disconnects (aka PCI retry)
	 *
	 * this is in general a bad idea, but we do this now to make
	 * sure we are not losing writes to the register file
	 */
	GLINT_WRITE_REG(1,			DFIFODis);
	GLINT_WRITE_REG(3,			FIFODis);
	GLINT_WRITE_REG(crtcRegs->vclkctl,	VClkCtl);
	GLINT_WRITE_REG(pprod,			FBReadMode);

	/* in their infinite wisdom, 3DLabs changed the video timing
	   registers between the 500TX and PerMedia... */
	GLINT_WRITE_REG(0x01,			VTGSerialClk);
	GLINT_WRITE_REG(0x00,			VTGModeCtl);
	GLINT_WRITE_REG(crtcRegs->vtgserialclk,	VTGSerialClk);
	GLINT_WRITE_REG(crtcRegs->vtgmodectl,	VTGModeCtl);

	GLINT_WRITE_REG(crtcRegs->h_limit,      VTGHLimit);
	GLINT_WRITE_REG(crtcRegs->h_sync_start, VTGHSyncStart);
	GLINT_WRITE_REG(crtcRegs->h_sync_end,	VTGHSyncEnd);
	GLINT_WRITE_REG(crtcRegs->h_blank_end,	VTGHBlankEnd);
	GLINT_WRITE_REG(crtcRegs->v_limit,	VTGVLimit);
	GLINT_WRITE_REG(crtcRegs->v_sync_start, VTGVSyncStart);
	GLINT_WRITE_REG(crtcRegs->v_sync_end,	VTGVSyncEnd);
	GLINT_WRITE_REG(crtcRegs->v_blank_end,	VTGVBlankEnd);
if (OFLG_ISSET(OPTION_FIREGL3000, &glintInfoRec.options))
  {
    GLINT_WRITE_REG(crtcRegs->h_blank_end-1/*2*/,VTGHGateStart);
    GLINT_WRITE_REG(crtcRegs->h_limit-1/*2*/,	VTGHGateEnd);
  }
else
  {
    GLINT_WRITE_REG(crtcRegs->h_blank_end-2,VTGHGateStart);
    GLINT_WRITE_REG(crtcRegs->h_limit-2,	VTGHGateEnd);
  }
GLINT_WRITE_REG(crtcRegs->v_blank_end-1,VTGVGateStart);
	GLINT_WRITE_REG(crtcRegs->v_blank_end,	VTGVGateEnd);
    } 
    else if (IS_3DLABS_PM_FAMILY(coprotype)) {
      GLINT_WAIT (16);
      GLINT_WRITE_REG(0x0,			TextureAddressMode);
      GLINT_WRITE_REG(0x0,			TextureReadMode);
      GLINT_WRITE_REG(1,			DFIFODis);
      GLINT_WRITE_REG(3,			FIFODis);
      /*
       * this is the Permedia version of crtc registers
       */
      GLINT_WRITE_REG(crtcRegs->h_blank_end,	PMHgEnd);

	GLINT_WRITE_REG(crtcRegs->vclkctl,	VClkCtl);
	GLINT_WRITE_REG(0,			PMScreenBase);

	GLINT_WRITE_REG((crtcRegs->h_limit - crtcRegs->h_blank_end)>>1, PMScreenStride);
	GLINT_WRITE_REG(crtcRegs->h_limit-1,    PMHTotal);
	GLINT_WRITE_REG(crtcRegs->h_blank_end,	PMHbEnd);
	GLINT_WRITE_REG(crtcRegs->h_sync_start-1,   PMHsStart);
	GLINT_WRITE_REG(crtcRegs->h_sync_end-1,	PMHsEnd);

	GLINT_WRITE_REG(crtcRegs->v_limit-1,	PMVTotal);
	GLINT_WRITE_REG(crtcRegs->v_blank_end,	PMVbEnd);
	GLINT_WRITE_REG(crtcRegs->v_sync_start-1,	PMVsStart);
	GLINT_WRITE_REG(crtcRegs->v_sync_end-1,	PMVsEnd);
	if (IS_3DLABS_PM2_CLASS(coprotype)) {
	  /* set SClk Src to MClk/2 */
	  int cc = GLINT_READ_REG(ChipConfig);
#ifdef DEBUG
	  ErrorF("SClk was 0x%x\n",cc & SCLK_SEL_MASK);
#endif
	  cc &= ~ SCLK_SEL_MASK;
	  GLINT_WRITE_REG(cc | SCLK_SEL_MCLK_HALF, ChipConfig);
	}
    }

    if (IS_3DLABS_TX_MX_CLASS(coprotype)|| 
        IS_3DLABS_PERMEDIA_CLASS(coprotype)) {
	IBMRGB52x_Init_Stdmode(crtcRegs->clock_sel);
    }
    else if (IS_3DLABS_PM2_CLASS(coprotype) ) {
    	PM2DACInit(crtcRegs->clock_sel);
    }
  
    if (IS_3DLABS_TX_MX_CLASS(coprotype))
    {
      GLINT_WAIT (1);
	GLINT_WRITE_REG(crtcRegs->fbmodesel, FBModeSel);
    } 
}

void
saveGLINTstate()
{
    int i;

    if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
	SR.glintRegs[0] = GLINT_READ_REG(VTGHLimit);
	SR.glintRegs[1] = GLINT_READ_REG(VTGHSyncStart);
	SR.glintRegs[2] = GLINT_READ_REG(VTGHSyncEnd);
	SR.glintRegs[3] = GLINT_READ_REG(VTGHBlankEnd);
	SR.glintRegs[4] = GLINT_READ_REG(VTGVLimit);
	SR.glintRegs[5] = GLINT_READ_REG(VTGVSyncStart);
	SR.glintRegs[6] = GLINT_READ_REG(VTGVSyncEnd);
	SR.glintRegs[7] = GLINT_READ_REG(VTGVBlankEnd);
	SR.glintRegs[8] = GLINT_READ_REG(VTGPolarity);

    } else if (IS_3DLABS_PM_FAMILY(coprotype)) {
	SR.glintRegs[5] = GLINT_READ_REG(PMScreenStride);
	SR.glintRegs[6] = GLINT_READ_REG(PMHTotal);
	SR.glintRegs[7] = GLINT_READ_REG(PMHbEnd);
	SR.glintRegs[8] = GLINT_READ_REG(PMHgEnd);
	SR.glintRegs[9] = GLINT_READ_REG(PMHsStart);
	SR.glintRegs[10] = GLINT_READ_REG(PMHsStart);
	SR.glintRegs[11] = GLINT_READ_REG(PMHsEnd);
	SR.glintRegs[12] = GLINT_READ_REG(PMVTotal);
	SR.glintRegs[13] = GLINT_READ_REG(PMVbEnd);
	SR.glintRegs[14] = GLINT_READ_REG(PMVsStart);
	SR.glintRegs[15]= GLINT_READ_REG(PMVsEnd);
	SR.glintRegs[16]= GLINT_READ_REG(PMScreenBase);
	SR.glintRegs[17]= GLINT_READ_REG(PMVideoControl);
	SR.glintRegs[18]= GLINT_READ_REG(VClkCtl);
    }

    for (i=0; i<0x100; i++)
	SR.DacRegs[i] = glintInIBMRGBIndReg(i);
}


void
restoreGLINTstate(void)
{
    int i;
    unsigned short usData;

#if DEBUG
    glintDumpRegs();
#endif

    if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
	GLINT_WRITE_REG(SR.glintRegs[0], VTGHLimit);
	GLINT_WRITE_REG(SR.glintRegs[1], VTGHSyncStart);
	GLINT_WRITE_REG(SR.glintRegs[2], VTGHSyncEnd);
	GLINT_WRITE_REG(SR.glintRegs[3], VTGHBlankEnd);
	GLINT_WRITE_REG(SR.glintRegs[4], VTGVLimit);
	GLINT_WRITE_REG(SR.glintRegs[5], VTGVSyncStart);
	GLINT_WRITE_REG(SR.glintRegs[6], VTGVSyncEnd);
	GLINT_WRITE_REG(SR.glintRegs[7], VTGVBlankEnd);
	GLINT_WRITE_REG(SR.glintRegs[8], VTGPolarity);
    
	for (i=0; i<0x100; i++) 
	    glintOutIBMRGBIndReg(i, 0, SR.DacRegs[i]);

    } else if (IS_3DLABS_PM_FAMILY(coprotype)) {
	    GLINT_WRITE_REG(SR.glintRegs[8],  PMHgEnd);
	    GLINT_WRITE_REG(SR.glintRegs[18], VClkCtl);
	    GLINT_WRITE_REG(SR.glintRegs[16], PMScreenBase);
	    
	    GLINT_WRITE_REG(SR.glintRegs[5], PMScreenStride);
	    GLINT_WRITE_REG(SR.glintRegs[6], PMHTotal);
	    GLINT_WRITE_REG(SR.glintRegs[7], PMHbEnd);
	    
	    GLINT_WRITE_REG(SR.glintRegs[9],  PMHsStart);
	    GLINT_WRITE_REG(SR.glintRegs[10], PMHsStart);
	    GLINT_WRITE_REG(SR.glintRegs[11], PMHsEnd);
	    GLINT_WRITE_REG(SR.glintRegs[12], PMVTotal);
	    GLINT_WRITE_REG(SR.glintRegs[13], PMVbEnd);
	    GLINT_WRITE_REG(SR.glintRegs[14], PMVsStart);
	    GLINT_WRITE_REG(SR.glintRegs[15], PMVsEnd);
	    
	    GLINT_WRITE_REG(SR.glintRegs[17], PMVideoControl);
	    GLINT_WRITE_REG(SR.glintRegs[19], ChipConfig);

	    GLINT_WRITE_REG(SR.glintRegs[0], Aperture0);
	    GLINT_WRITE_REG(SR.glintRegs[1], Aperture1);
	    GLINT_WRITE_REG(SR.glintRegs[2], PMFramebufferWriteMask);
	    GLINT_WRITE_REG(SR.glintRegs[3], PMBypassWriteMask);
	    GLINT_WRITE_REG(SR.glintRegs[4], FIFODis);

	    GLINT_WRITE_REG(SR.glintRegs[20], PMInterruptLine);
	    
	    /* GLINT_WRITE_REG(SR.glintRegs[21], PMMemConfig); */
	    /* GLINT_WRITE_REG(SR.glintRegs[22], PMBootAddress); */
	    /* GLINT_WRITE_REG(SR.glintRegs[23], PMRomControl); */

	    if (IS_3DLABS_PM_FAMILY(coprotype)) {

		/* restore ramdac */
		for (i=0; i<0x100; i++) 
		    glintOutIBMRGBIndReg(i, 0, SR.DacRegs[i]);

		/* restore colors */
		GLINT_SLOW_WRITE_REG(0x00, IBMRGB_WRITE_ADDR);
		for (i=0; i<256; i++) {
		    GLINT_SLOW_WRITE_REG(oldlut[i].r,IBMRGB_RAMDAC_DATA);
		    GLINT_SLOW_WRITE_REG(oldlut[i].g,IBMRGB_RAMDAC_DATA);
		    GLINT_SLOW_WRITE_REG(oldlut[i].b,IBMRGB_RAMDAC_DATA);
		}

		/* switch to VGA */
		GLINT_WRITE_REG((unsigned char)PERMEDIA_VGA_CTRL_INDEX, 
		                PERMEDIA_MMVGA_INDEX_REG);

		usData = GLINT_READ_REG(PERMEDIA_MMVGA_DATA_REG);
		usData |= 
		    ((PERMEDIA_VGA_ENABLE | PERMEDIA_VGA_MEMORYACCESS) << 8) | 
		    PERMEDIA_VGA_CTRL_INDEX;
		GLINT_WRITE_REG(usData, PERMEDIA_MMVGA_INDEX_REG);
	    }

	}
}

void
glintCleanUp(void)
{
    unsigned short usData;

    if (!glintInitialized)
	return;
  
    restoreGLINTstate();

    glintInitialized = FALSE;
}

void permediapreinit(void)
{
  /* SR.glintRegs[21]= GLINT_READ_REG(PMMemConfig); */
  /* SR.glintRegs[22]= GLINT_READ_REG(PMBootAddress); */
  /* SR.glintRegs[23]= GLINT_READ_REG(PMRomControl); */

  GLINT_WAIT (8);
#if 0
  /* set the memconfig Register to found memory */
  switch (glintInfoRec.videoRam)
    {
      /* MCLK = 50 Mhz and 4 Mbytes */
    case 4069:
      GLINT_WRITE_REG(Burst1Cycle4 | NumberBanks4 | RefreshCount4 | BankDelay4 | 
		      DeadCycle4 | CAS3Latency4 | TimeRASMin4 | RowCharge4 | 
		      TimeRCD4 | TimeRC4 | TimeRP4, PMMemConfig);
      GLINT_WRITE_REG(BootAdress4, PMBootAddress);
      GLINT_WRITE_REG(SDRAM4, PMRomControl);
      break;
    case 6144:
      /* MCLK = 66 Mhz and 6 Mbytes */
      GLINT_WRITE_REG(Burst1Cycle6 | NumberBanks6 | RefreshCount6 | BankDelay6 | 
		      DeadCycle6 | CAS3Latency6 | TimeRASMin6 | RowCharge6 | 
		      TimeRCD6 | TimeRC6 | TimeRP6, PMMemConfig);
      GLINT_WRITE_REG(BootAdress6, PMBootAddress);
      GLINT_WRITE_REG(SDRAM6, PMRomControl);
      break;
    case 8192:
      /* MCLK = 66 Mhz and 8 Mbytes */
      GLINT_WRITE_REG(Burst1Cycle8 | NumberBanks8 | RefreshCount8 | BankDelay8 | 
		      DeadCycle8 | CAS3Latency8 | TimeRASMin8 | RowCharge8 | 
		      TimeRCD8 | TimeRC8 | TimeRP8, PMMemConfig);
      GLINT_WRITE_REG(BootAdress8, PMBootAddress);
      GLINT_WRITE_REG(SDRAM8, PMRomControl);
      break;
    }
#endif

  GLINT_WRITE_REG(0x01,       FIFODis); 
  GLINT_WRITE_REG(0x00,       Aperture0);
  GLINT_WRITE_REG(0x00,       Aperture1);
  GLINT_WRITE_REG(0xffffffff, PMFramebufferWriteMask);
  GLINT_WRITE_REG(0xffffffff, PMBypassWriteMask);
}

Bool
glintInit(DisplayModePtr mode)
{
    int i,j;
    unsigned short usData;

    if (IS_3DLABS_TX_MX_CLASS(coprotype)) {
	    if (!glintInitialized)
		saveGLINTstate();
#ifdef COMPATIBELMODE
	    if (glintVideoMemSave && glintVideoMem)
		xf86memcpy(glintVideoMemSave, (unsigned char *)glintVideoMem, 
			   MEMSize/* glintInfoRec.virtualX * glintInfoRec.virtualY * */
/* 			   glintInfoRec.bitsPerPixel / 8 */);
	    IBMRGB52x_Init(mode);
#endif

	    /*
	     * this is hardwired for setting the FB and LB memory control
	     */
	    GLINT_WRITE_REG(0xdc000017,  LBMemoryEDO);
	    GLINT_WRITE_REG(0x60400800,  FBMemoryCtl);
	    GLINT_WRITE_REG(0xFFFFFFFF,  FBWrMaskk);
	    GLINT_WRITE_REG(0x00000002,  FBTXMemCtl);
    } 


    if (IS_3DLABS_PM_FAMILY(coprotype)) {
	  permediapreinit();

	  /* switch to graphics Mode */
	  GLINT_WRITE_REG((unsigned char)PERMEDIA_VGA_CTRL_INDEX, PERMEDIA_MMVGA_INDEX_REG);
	  usData = (unsigned short)GLINT_READ_REG(PERMEDIA_MMVGA_DATA_REG);
	  usData &= ~PERMEDIA_VGA_ENABLE;
	  usData = (usData << 8) | PERMEDIA_VGA_CTRL_INDEX;
	  GLINT_WRITE_REG((unsigned short)usData, PERMEDIA_MMVGA_INDEX_REG);
    }

    /*
     * for some reason, the server cannot access the framebuffer
     * when using some motherboards. At this point all we can do is
     * detect this :-(
     */
    if (!glintInitialized)
	{
	    if(*(unsigned char*)glintVideoMem != 0)    
		ErrorF("Framebuffer Access Problem!!\n");
	    *(unsigned char*)glintVideoMem = 0xff;
	    if(*(unsigned char*)glintVideoMem != 0xff) 
		ErrorF("Framebuffer Access Problem!!\n");
	    *(unsigned char*)glintVideoMem = 0x0;
	    if(*(unsigned char*)glintVideoMem != 0)    
		ErrorF("Framebuffer Access Problem!!\n");
	}

    glintInitialized = TRUE;
    glintInitCursorFlag = TRUE;

    return(TRUE);
}

static void
InitLUT(void)
{
    int i;

    GLINT_SLOW_WRITE_REG(0xFF, IBMRGB_PIXEL_MASK);
    GLINT_SLOW_WRITE_REG(0x00, IBMRGB_READ_ADDR);

    /*
     * we should make sure that we don't overrun the RAMDAC, so let's
     * pause for a moment every time after we've written to it
     */
    for (i=0; i<256; i++) {
	oldlut[i].r = GLINT_READ_REG(IBMRGB_RAMDAC_DATA);
	oldlut[i].g = GLINT_READ_REG(IBMRGB_RAMDAC_DATA);
	oldlut[i].b = GLINT_READ_REG(IBMRGB_RAMDAC_DATA);
    }

    GLINT_SLOW_WRITE_REG(0x00, IBMRGB_WRITE_ADDR);
    for (i=0; i<256; i++) {
	GLINT_SLOW_WRITE_REG(0x00,IBMRGB_RAMDAC_DATA);
	GLINT_SLOW_WRITE_REG(0x00,IBMRGB_RAMDAC_DATA);
	GLINT_SLOW_WRITE_REG(0x00,IBMRGB_RAMDAC_DATA);
    }

    if (glintInfoRec.bitsPerPixel > 8) {
	int r,g,b;
	int mr,mg,mb;
	int nr = 5, ng = 5, nb = 5;
	extern unsigned char xf86rGammaMap[], xf86gGammaMap[], xf86bGammaMap[];
	extern LUTENTRY currentglintdac[];
	
	if (!LUTInited) {
	    if (glintInfoRec.bitsPerPixel == 32) {
		for (i=0;i<256;i++) {
		    currentglintdac[i].r = xf86rGammaMap[i];
		    currentglintdac[i].g = xf86gGammaMap[i];
		    currentglintdac[i].b = xf86bGammaMap[i];
		}
	    } else {
		mr = (1<<nr)-1;
		mg = (1<<ng)-1;
		mb = (1<<nb)-1;
		
		for (i=0;i<256;i++) {
		    r = (i >> (6-nr)) & mr;
		    /* g = (i >> (6-ng)) & mg; */
		    /* b = (i >> (6-nb)) & mb; */
		    currentglintdac[i].r = xf86rGammaMap[(r*255+mr/2)/mr];
		    currentglintdac[i].g = xf86gGammaMap[(r*255+mg/2)/mg];
		    currentglintdac[i].b = xf86bGammaMap[(r*255+mb/2)/mb];
		}
	    }
	}

   	GLINT_SLOW_WRITE_REG(0x00, IBMRGB_WRITE_ADDR);
   	for (i=0; i<256; i++) {
	    GLINT_SLOW_WRITE_REG(currentglintdac[i].r,IBMRGB_RAMDAC_DATA);
	    GLINT_SLOW_WRITE_REG(currentglintdac[i].g,IBMRGB_RAMDAC_DATA);
	    GLINT_SLOW_WRITE_REG(currentglintdac[i].b,IBMRGB_RAMDAC_DATA);
   	}
    }

    LUTInited = TRUE;
}


void
glintInitEnvironment(void)
{
    InitLUT();
}


void
glintInitAperture(int screen_idx)
{
    unsigned short usData;
  
    if (IS_3DLABS_PM_FAMILY(coprotype)) {
	    if (!glintInitialized)
		{
		    SR.glintRegs[0]  = GLINT_READ_REG(Aperture0);
		    SR.glintRegs[1]  = GLINT_READ_REG(Aperture1);
		    SR.glintRegs[2]  = GLINT_READ_REG(PMFramebufferWriteMask);
		    SR.glintRegs[3]  = GLINT_READ_REG(PMBypassWriteMask);
		    SR.glintRegs[4]  = GLINT_READ_REG(FIFODis);
		    SR.glintRegs[20] = GLINT_READ_REG(PMInterruptLine);
		    SR.glintRegs[19] = GLINT_READ_REG(ChipConfig);
	  
		    permediapreinit();

		    saveGLINTstate(); 
		}
	}

    if (!glintVideoMem) 
	{
	    glintVideoMem = xf86MapVidMem(screen_idx, LINEAR_REGION,
					  (pointer)(glintInfoRec.MemBase),
					  glintInfoRec.videoRam * 1024);
	}

#ifdef XFreeXDGA
    glintInfoRec.physBase = glintInfoRec.MemBase;
    glintInfoRec.physSize = glintInfoRec.virtualX * glintInfoRec.virtualY
 	* glintInfoRec.bitsPerPixel / 8;
#endif
}	


void
glintDumpRegs()
{
    int i;
    unsigned int v;

    /*
     * simply walk the registers and dump them
     */
    ErrorF("GLINT Register Dump\n");
    for( i=0; i<0x70; i+=8 )
	{
	    if( i % 32 == 0 )
		ErrorF("\n 0x%04x\t",i);
	    v = GLINT_READ_REG(i);
	    if( v < 0x10000 )
		/*
		 * let's do hex and dec
		 */
		ErrorF("0x%08x %5d ",v,v);
	    else
		ErrorF("0x%08x ----- ",v);
	}
    ErrorF("\n");
    for( i=0x1000; i<0x1838; i+=8 )
	{
	    switch(i)
		{
		case 0x1010:
		    i = 0x1800;
		    ErrorF("\n 0x%04x\t",i);
		    break;
		case 0x1810:
		    i = 0x1820;
		case 0x1000:
		    ErrorF("\n 0x%04x\t",i);
		    break;
		}
	    v = GLINT_READ_REG(i);
	    if( v < 0x10000 )
		/*
		 * let's do hex and dec
		 */
		ErrorF("0x%08x %5d ",v,v);
	    else
		ErrorF("0x%08x ----- ",v);
	}
    ErrorF("\n");
    for( i=0x3000; i<0x3088; i+=8 )
	{
	    if( i % 32 == 0 )
		ErrorF("\n 0x%04x\t",i);
	    v = GLINT_READ_REG(i);
	    if( v < 0x10000 )
		/*
		 * let's do hex and dec
		 */
		ErrorF("0x%08x %5d ",v,v);
	    else
		ErrorF("0x%08x       ",v);
	}
    ErrorF("\n 0x4800\t");
    v = GLINT_READ_REG(0x4800);
    if( v < 0x10000 )
	/*
	 * let's do hex and dec
	 */
	ErrorF("0x%08x %5d ",v,v);
    else
	ErrorF("0x%08x       ",v);
    ErrorF("\n 0x6000\t");
    v = GLINT_READ_REG(0x6000);
    if( v < 0x10000 )
	/*
	 * let's do hex and dec
	 */
	ErrorF("0x%08x %5d ",v,v);
    else
	ErrorF("0x%08x       ",v);
    ErrorF("\n\n");
    if (IS_3DLABS_TX_MX_CLASS(coprotype) ||
	IS_3DLABS_PERMEDIA_CLASS(coprotype)) {
	ErrorF("IBM RAMDAC  Register Dump\n");
	for (i=0; i<0x100; i++)
	{
	    if( i % 8 == 0 )
		ErrorF("\n 0x%04x\t",i);
	    ErrorF("0x%02x ",glintInIBMRGBIndReg(i));
	}
	ErrorF("\n\n");
    }
}

void
glintDumpCoreDrawRegs()
{
    int i;
    unsigned int v;

    /*
     * simply walk the registers and dump them
     */
    for( i=0x8000; i<0x8038; i+=8 )
	{
	    if( i % 32 == 0 )
		ErrorF("\n 0x%04x\t",i);
	    v = GLINT_READ_REG(i);
	    if( v < 0x10000 )
		/*
		 * let's do hex and dec
		 */
		ErrorF("0x%08x %5d ",v,v);
	    else
		ErrorF("0x%08x ----- ",v);
	}
    ErrorF("\n");
}




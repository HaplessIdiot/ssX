/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/glint_init.c,v 1.1 1997/06/17 08:17:55 hohndel Exp $ */
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
 *           Dirk Hohndel, <hohndel@suse.de>
 *	     Stefan Dirsch, <sndirsch@suse.de>
 *
 * this work is sponsored by S.u.S.E. GmbH, Fuerth, Elsa GmbH, Aachen and
 * Siemens Nixdorf Informationssysteme
 */

#include "glint_regs.h"
#define GLINT_SERVER
#include "IBMRGB.h"
#include "glint.h"
#include "xf86_Config.h"

typedef struct {
	unsigned long glintRegs[0x200];
	unsigned long DacRegs[0x100];
} glintRegisters;
static glintRegisters SR;

unsigned char *glintVideoMemSave=NULL;

static int glintInitialized = 0;
static Bool LUTInited = FALSE;
static LUTENTRY oldlut[256];
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

static int partprod[] = {
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
		     -1,              -1,              -1, PARTPROD(0,7,7) };

int
Shiftbpp(int value)
{
	/* shift horizontal timings for 64bit VRAM's or 32bit SGRAMs */
	
	int logbytesperaccess;
	
	if (coprotype == PCI_CHIP_3DLABS_500TX)
		logbytesperaccess = 3;
	else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA)
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

	crtcRegs->pitch		= mode->CrtcHDisplay;

	if (coprotype == PCI_CHIP_3DLABS_500TX) {
		crtcRegs->vtgpolarity = 0x80; /* CBlank active low */

		if (mode->Flags & V_PHSYNC)
			crtcRegs->vtgpolarity |= 0x02;
		else
			crtcRegs->vtgpolarity |= 0x00;
		if (mode->Flags & V_PVSYNC)
			crtcRegs->vtgpolarity |= 0x08;
		else
			crtcRegs->vtgpolarity |= 0x00;
	}
	else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
		crtcRegs->vtgpolarity = 0x01; /* GP video enable */

		if (mode->Flags & V_PHSYNC)
			crtcRegs->vtgpolarity |= 0x18;
		else
			crtcRegs->vtgpolarity |= 0x08;
		if (mode->Flags & V_PVSYNC)
			crtcRegs->vtgpolarity |= 0x60;
		else
			crtcRegs->vtgpolarity |= 0x20;
	}
		

	crtcRegs->clock_sel = glintInfoRec.clock[mode->Clock];

	/*
	 * tell DAC to use the ICD chip clock 0 as ref clock 
	 * and set up some more video timining generator registers
	 */
	crtcRegs->vclkctl = 0x00;
	crtcRegs->vtgserialclk = 0x05;
	crtcRegs->fbmodesel = 0xa07; /* 4way interleave */
	crtcRegs->vtgmodectl = 0x44;
}

void
glintSetCRTCRegs(glintCRTCRegPtr crtcRegs)
{
	pprod = partprod[crtcRegs->pitch >> 5];

	/*
	 * in order to set up a mode we need to do a few more
	 * things here than just set up the CRTC regs...
	 */
	if (coprotype == PCI_CHIP_3DLABS_500TX) {
		crtcRegs->vtgpolarity |= 0x30;	/* ELSA Gloria-L */
		GLINT_WRITE_REG(crtcRegs->vtgpolarity,	VTGPolarity);
	}
	else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
#if 0
		/*
		 * first thing we want to make sure on a PerMedia card
		 * is that the VGA part is turned off
		 */
		GLINT_WRITE_REG(0x05, 0x63c4);
		GLINT_WRITE_REG(0x00, 0x63c4);
#endif
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

	GLINT_WRITE_REG(1,			FBWriteMode);
	GLINT_WRITE_REG(pprod | 0x600,		LBReadMode);
	GLINT_WRITE_REG(0x01,			LBWriteMode);
	GLINT_WRITE_REG(0x8842,			LBReadFormat);
	GLINT_WRITE_REG(0x8842,			LBWriteFormat);
	GLINT_WRITE_REG(pprod,			FBReadMode);
	GLINT_WRITE_REG(0x0,			ScissorMode);

	/*
	 * this one depends on the color depth
	 */
	switch (glintInfoRec.bitsPerPixel) {
	case 8:
		GLINT_WRITE_REG(0x400 | (0x0e << 2) | 1,DitherMode);
		break;
	case 16:
		GLINT_WRITE_REG(0x400 | (0x01 << 2) | 1,DitherMode);
		break;
	case 32:
		GLINT_WRITE_REG(0x400 | (0x00 << 2) | 1,DitherMode);
		break;
	}
	GLINT_WRITE_REG(0x3000,			AlphaBlendMode);
	GLINT_WRITE_REG(0x0,			ColorDDAMode);
	GLINT_WRITE_REG(0x0,			TextureColorMode);
	GLINT_WRITE_REG(0x0,			FogMode);
	GLINT_WRITE_REG(0x0,			AntialiasMode);
	GLINT_WRITE_REG(0x0,			AlphaTestMode);
	GLINT_WRITE_REG(0x0,			StencilMode);
	GLINT_WRITE_REG(0x0,			AreaStippleMode);
	GLINT_WRITE_REG(0x0,			LineStippleMode);
	GLINT_WRITE_REG(0x0,			LogicalOpMode);
	GLINT_WRITE_REG(/*0x7b*/0,			DepthMode);
	GLINT_WRITE_REG(0x0,			StatisticMode);
	GLINT_WRITE_REG(0xc00,			FilterMode);
	GLINT_WRITE_REG(0xffffffff,		FBHardwareWriteMask);
	GLINT_WRITE_REG(0xffffffff,		FBSoftwareWriteMask);
	GLINT_WRITE_REG(0x0,			RasterizerMode);
	GLINT_WRITE_REG(0x0,			Depth);
	GLINT_WRITE_REG(0x0,			FBSourceOffset);
	GLINT_WRITE_REG(0x0,			FBPixelOffset);
	GLINT_WRITE_REG(0x0,			LBSourceOffset);
	GLINT_WRITE_REG(0x0,			WindowOrigin);
	GLINT_WRITE_REG(0x0,			FBWindowBase);
	GLINT_WRITE_REG(0x0,			LBWindowBase);
	GLINT_WRITE_REG(0x0,			RasterizerMode);
	if (coprotype == PCI_CHIP_3DLABS_500TX) {
		switch (glintInfoRec.bitsPerPixel) {
		case 8:
			GLINT_WRITE_REG(0x2,			PixelSize);
			break;
		case 16:
			GLINT_WRITE_REG(0x1,			PixelSize);
			break;
		case 32:
			GLINT_WRITE_REG(0x0,			PixelSize);
			break;
		}
	}
	else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
		switch (glintInfoRec.bitsPerPixel) {
		case 8:
			GLINT_WRITE_REG(0x0,			FBReadPixel);
			break;
		case 16:
			GLINT_WRITE_REG(0x1,			FBReadPixel);
			break;
		case 32:
			GLINT_WRITE_REG(0x2,			FBReadPixel);
			break;
		}
	}
	GLINT_WRITE_REG(0x0,			TextureAddressMode);
	GLINT_WRITE_REG(0x0,			TextureReadMode);
	GLINT_WRITE_REG(0x0,			RouterMode);
	GLINT_WRITE_REG(0x0,			PatternRamMode);
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
	if (coprotype == PCI_CHIP_3DLABS_500TX) {
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
		GLINT_WRITE_REG(crtcRegs->h_blank_end-2,VTGHGateStart);
		GLINT_WRITE_REG(crtcRegs->h_limit-2,	VTGHGateEnd);
		GLINT_WRITE_REG(crtcRegs->v_blank_end-1,VTGVGateStart);
		GLINT_WRITE_REG(crtcRegs->v_blank_end,	VTGVGateEnd);
	} 
	else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
		GLINT_WRITE_REG((crtcRegs->h_limit - crtcRegs->h_blank_end)<<1,
							PMScreenStride);
		GLINT_WRITE_REG(crtcRegs->h_limit-1,	PMHTotal);
		GLINT_WRITE_REG(crtcRegs->h_blank_end,	PMHbEnd);
		GLINT_WRITE_REG(crtcRegs->h_blank_end,	PMHgEnd);
		GLINT_WRITE_REG(crtcRegs->h_sync_start, PMHsStart);
		GLINT_WRITE_REG(crtcRegs->h_sync_end,	PMHsEnd);
		GLINT_WRITE_REG(crtcRegs->v_limit-1,	PMVTotal);
		GLINT_WRITE_REG(crtcRegs->v_blank_end,	PMVbEnd);
		GLINT_WRITE_REG(crtcRegs->v_sync_start,	PMVsStart);
		GLINT_WRITE_REG(crtcRegs->v_sync_end,	PMVsEnd);
		GLINT_WRITE_REG(0,			PMScreenBase);
		
	}

	IBMRGBClockSelect(crtcRegs->clock_sel);

	if (coprotype == PCI_CHIP_3DLABS_500TX) {
		GLINT_WRITE_REG(crtcRegs->fbmodesel,	FBModeSel);
	} 
	else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
		GLINT_WRITE_REG(0x4005,0x63c4);
	} 
}

void
saveGLINTstate()
{
	int i;

	if (coprotype == PCI_CHIP_3DLABS_500TX) {
		SR.glintRegs[0] = GLINT_READ_REG(VTGHLimit);
		SR.glintRegs[1] = GLINT_READ_REG(VTGHSyncStart);
		SR.glintRegs[2] = GLINT_READ_REG(VTGHSyncEnd);
		SR.glintRegs[3] = GLINT_READ_REG(VTGHBlankEnd);
		SR.glintRegs[4] = GLINT_READ_REG(VTGVLimit);
		SR.glintRegs[5] = GLINT_READ_REG(VTGVSyncStart);
		SR.glintRegs[6] = GLINT_READ_REG(VTGVSyncEnd);
		SR.glintRegs[7] = GLINT_READ_REG(VTGVBlankEnd);
		SR.glintRegs[8] = GLINT_READ_REG(VTGPolarity);
	} else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
		SR.glintRegs[0] = GLINT_READ_REG(PMScreenStride);
		SR.glintRegs[1] = GLINT_READ_REG(PMHTotal);
		SR.glintRegs[2] = GLINT_READ_REG(PMHbEnd);
		SR.glintRegs[3] = GLINT_READ_REG(PMHgEnd);
		SR.glintRegs[4] = GLINT_READ_REG(PMHsStart);
		SR.glintRegs[5] = GLINT_READ_REG(PMHsStart);
		SR.glintRegs[6] = GLINT_READ_REG(PMHsEnd);
		SR.glintRegs[7] = GLINT_READ_REG(PMVTotal);
		SR.glintRegs[8] = GLINT_READ_REG(PMVbEnd);
		SR.glintRegs[9] = GLINT_READ_REG(PMVsStart);
		SR.glintRegs[10]= GLINT_READ_REG(PMVsEnd);
		SR.glintRegs[11]= GLINT_READ_REG(PMScreenBase);
		SR.glintRegs[12]= GLINT_READ_REG(0x634c);
	}

	for (i=0; i<0x100; i++)
		SR.DacRegs[i] = glintInIBMRGBIndReg(i);
}


void
restoreGLINTstate(void)
{
	int i;

#if DEBUG
	glintDumpRegs();
#endif

	if (coprotype == PCI_CHIP_3DLABS_500TX) {
		GLINT_WRITE_REG(SR.glintRegs[0], VTGHLimit);
		GLINT_WRITE_REG(SR.glintRegs[1], VTGHSyncStart);
		GLINT_WRITE_REG(SR.glintRegs[2], VTGHSyncEnd);
		GLINT_WRITE_REG(SR.glintRegs[3], VTGHBlankEnd);
		GLINT_WRITE_REG(SR.glintRegs[4], VTGVLimit);
		GLINT_WRITE_REG(SR.glintRegs[5], VTGVSyncStart);
		GLINT_WRITE_REG(SR.glintRegs[6], VTGVSyncEnd);
		GLINT_WRITE_REG(SR.glintRegs[7], VTGVBlankEnd);
		GLINT_WRITE_REG(SR.glintRegs[8], VTGPolarity);
	} else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
		GLINT_WRITE_REG(SR.glintRegs[0], PMScreenStride);
		GLINT_WRITE_REG(SR.glintRegs[1], PMHTotal);
		GLINT_WRITE_REG(SR.glintRegs[2], PMHbEnd);
		GLINT_WRITE_REG(SR.glintRegs[3], PMHgEnd);
		GLINT_WRITE_REG(SR.glintRegs[4], PMHsStart);
		GLINT_WRITE_REG(SR.glintRegs[5], PMHsStart);
		GLINT_WRITE_REG(SR.glintRegs[6], PMHsEnd);
		GLINT_WRITE_REG(SR.glintRegs[7], PMVTotal);
		GLINT_WRITE_REG(SR.glintRegs[8], PMVbEnd);
		GLINT_WRITE_REG(SR.glintRegs[9], PMVsStart);
		GLINT_WRITE_REG(SR.glintRegs[10],PMVsEnd);
		GLINT_WRITE_REG(SR.glintRegs[11],PMScreenBase);
		GLINT_WRITE_REG(SR.glintRegs[12],0x634c);
	}

	for (i=0; i<0x100; i++)
		glintOutIBMRGBIndReg(i, 0, SR.DacRegs[i]);

	usleep(5000);
}


void
glintCleanUp(void)
{
	if (!glintInitialized)
		return;

	restoreGLINTstate();

	if (glintVideoMemSave && glintVideoMem)
	  xf86memcpy(glintVideoMemSave, (unsigned char *)glintVideoMem, 
		     glintInfoRec.virtualX * glintInfoRec.virtualY *
		     glintInfoRec.bitsPerPixel / 8);
}


Bool
glintInit(DisplayModePtr mode)
{
	int i,j;

	if (!glintInitialized)
		saveGLINTstate();

	if (glintVideoMemSave && glintVideoMem)
	  xf86memcpy((unsigned char *)glintVideoMem, glintVideoMemSave, 
		     glintInfoRec.virtualX * glintInfoRec.virtualY *
		     glintInfoRec.bitsPerPixel / 8);

	IBMRGB52x_Init(mode);
#if 0
	/*
	 * this is hardwired for setting the FB and LB memory control
	 */
	if (coprotype == PCI_CHIP_3DLABS_500TX) {
		GLINT_WRITE_REG(0xdc000017,      LBMemoryEDO);
		GLINT_WRITE_REG(0x60400800,      FBMemoryCtl);
		GLINT_WRITE_REG(0xFFFFFFFF,	 FBWrMaskk);
		GLINT_WRITE_REG(
			GLINT_READ_REG(FBTXMemCtl)&0xfffffffe, FBTXMemCtl);
	} else if (coprotype == PCI_CHIP_3DLABS_PERMEDIA) {
		GLINT_WRITE_REG(0xFFFFFFFF,	 PMFramebufferWriteMask);
	}
#endif
	glintInitialized = 1;
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
				g = (i >> (6-ng)) & mg;
				b = (i >> (6-nb)) & mb;
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

        if (!glintVideoMem) {
	  glintVideoMem = xf86MapVidMem(screen_idx, LINEAR_REGION,
					(pointer)(glintInfoRec.MemBase),
					glintInfoRec.videoRam * 1024);
	  xf86memset(glintVideoMem, 0, glintInfoRec.virtualX *
			glintInfoRec.virtualY * glintInfoRec.bitsPerPixel / 8);
	  /*
	   * for some reason, the server cannot access the framebuffer
	   * when using some motherboards. At this point all we can do is
	   * detect this :-(
	   */
	  if(*(unsigned char*)glintVideoMem != 0) 
	  	ErrorF("Framebuffer Access Problem!!\n");
	  *(unsigned char*)glintVideoMem = 0xff;
	  if(*(unsigned char*)glintVideoMem != 0xff) 
	  	ErrorF("Framebuffer Access Problem!!\n");
	  *(unsigned char*)glintVideoMem = 0x0;
	  if(*(unsigned char*)glintVideoMem != 0) 
	  	ErrorF("Framebuffer Access Problem!!\n");

	}
	
	if (!glintVideoMemSave) {
	  glintVideoMemSave = (unsigned char *)xalloc(glintInfoRec.virtualX*
			glintInfoRec.virtualY*glintInfoRec.bitsPerPixel/8);
	  if (glintVideoMemSave == NULL)
	    FatalError("Unable to allocate save/restore buffer for %d "
		       "bytes, aborting.....\n");
	}

#ifdef XFreeXDGA
	glintInfoRec.physBase = (glintInfoRec.MemBase);
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
	ErrorF("IBM RAMDAC  Register Dump\n");
	for (i=0; i<0x100; i++)
	{
		if( i % 8 == 0 )
			ErrorF("\n 0x%04x\t",i);
		ErrorF("0x%02x ",glintInIBMRGBIndReg(i));
	}
	ErrorF("\n\n");
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

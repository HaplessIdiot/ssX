/*
 * Copyright 1997,1998 by Alan Hourihane <alanh@fairlite.demon.co.uk>
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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm2_dac.c,v 1.7 1998/09/05 06:36:48 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "glint_regs.h"
#include "glint.h"

#define INITIALFREQERR 100000
#define MINCLK 110000		/* VCO frequency range */
#define MAXCLK 250000

static int
Shiftbpp(ScrnInfoPtr pScrn, int value)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    /* shift horizontal timings for 64bit VRAM's or 32bit SGRAMs */
    int logbytesperaccess = 2;
	
    switch (pScrn->bitsPerPixel) {
    case 8:
	value >>= logbytesperaccess;
	pGlint->BppShift = logbytesperaccess;
	break;
    case 16:
	if (pGlint->DoubleBuffer) {
	    value >>= (logbytesperaccess-2);
	    pGlint->BppShift = logbytesperaccess-2;
	} else {
	    value >>= (logbytesperaccess-1);
	    pGlint->BppShift = logbytesperaccess-1;
	}
	break;
    case 24:
	value *= 3;
	value >>= logbytesperaccess;
	pGlint->BppShift = logbytesperaccess;
	break;
    case 32:
	value >>= (logbytesperaccess-2);
	pGlint->BppShift = logbytesperaccess-2;
	break;
    }
    return (value);
}

static unsigned long
PM2DAC_CalculateMNPCForClock
(
 unsigned long reqclock,		/* In kHz units */
 unsigned long refclock,		/* In kHz units */
 unsigned char *rm, 			/* M Out */
 unsigned char *rn, 			/* N Out */
 unsigned char *rp			/* P Out */
 )
{
    unsigned char	m, n, p;
    unsigned long	f;
    long		freqerr, lowestfreqerr = INITIALFREQERR;
    unsigned long  	clock,actualclock = 0;

    for (n = 2; n <= 14; n++) {
        for (m = 2; m != 0; m++) { /* this is a char, so this counts to 255 */
	    f = refclock * m / n;
	    if ( (f < MINCLK) || (f > MAXCLK) )
	    	continue;
	    for (p = 0; p <= 4; p++) {
	    	clock = f >> p;
		freqerr = reqclock - clock;
		if (freqerr < 0)
		    freqerr = -freqerr;
		if (freqerr < lowestfreqerr) {
		    *rn = n;
		    *rm = m;
		    *rp = p;
		    lowestfreqerr = freqerr;
		    actualclock = clock;
#ifdef DEBUG
	ErrorF("Best %ld diff %ld\n",actualclock,freqerr);
#endif
		}
	    }
	}
    }

    return(actualclock);
}

Bool
Permedia2Init(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINTRegPtr pReg = &pGlint->ModeReg;
    CARD32 temp1, temp2, temp3, temp4;

    pReg->glintRegs[Aperture0 >> 3] = 0;
    pReg->glintRegs[Aperture1 >> 3] = 0;
    pReg->glintRegs[PMFramebufferWriteMask >> 3] = 0xFFFFFFFF;
    pReg->glintRegs[PMBypassWriteMask >> 3] = 0xFFFFFFFF;

    if (pGlint->UsePCIRetry) {
	pReg->glintRegs[DFIFODis >> 3] = 1;
	pReg->glintRegs[FIFODis >> 3] = 3;
    } else {
	pReg->glintRegs[DFIFODis >> 3] = 0;
	pReg->glintRegs[FIFODis >> 3] = 1;
    }

    if (pGlint->UseBlockWrite)
	pReg->glintRegs[PMMemConfig >> 3] = GLINT_READ_REG(PMMemConfig) | 1<<21;


    temp1 = mode->CrtcHSyncStart - mode->CrtcHDisplay;
    temp2 = mode->CrtcVSyncStart - mode->CrtcVDisplay;
    temp3 = mode->CrtcHSyncEnd - mode->CrtcHSyncStart;
    temp4 = mode->CrtcVSyncEnd - mode->CrtcVSyncStart;

    pReg->glintRegs[PMHTotal >> 3] = Shiftbpp(pScrn,mode->CrtcHTotal);
    pReg->glintRegs[PMHsEnd >> 3] = Shiftbpp(pScrn, temp1 + temp3);
    pReg->glintRegs[PMHsStart >> 3] = Shiftbpp(pScrn, temp1);
    pReg->glintRegs[PMHbEnd >> 3] = 
			Shiftbpp(pScrn,mode->CrtcHTotal-mode->CrtcHDisplay);
    pReg->glintRegs[PMScreenStride >> 3] = 
			Shiftbpp(pScrn,pScrn->displayWidth>>1);

    pReg->glintRegs[PMVTotal >> 3] = mode->CrtcVTotal;
    pReg->glintRegs[PMVsEnd >> 3] = temp2 + temp4;
    pReg->glintRegs[PMVsStart >> 3] = temp2;
    pReg->glintRegs[PMVbEnd >> 3] = mode->CrtcVTotal - mode->CrtcVDisplay;

    pReg->glintRegs[PMVideoControl >> 3] = 
 	    (((mode->Flags & V_PHSYNC) ? 0x1 : 0x3) << 3) |  
 	    (((mode->Flags & V_PVSYNC) ? 0x1 : 0x3) << 5) | 1; 

    if (pScrn->bitsPerPixel > 8) {
	/* When != 8bpp then we stick the RAMDAC into 64bit mode */
	/* And reduce the horizontal timings by half */
	pReg->glintRegs[PMVideoControl >> 3] |= 1<<16;
    	pReg->glintRegs[PMHTotal >> 3] >>= 1;
	pReg->glintRegs[PMHsEnd >> 3] >>= 1;
	pReg->glintRegs[PMHsStart >> 3] >>= 1;
	pReg->glintRegs[PMHbEnd >> 3] >>= 1;
    }

    pReg->glintRegs[VClkCtl >> 3] = (GLINT_READ_REG(VClkCtl) & 0xFFFFFFFC);
    pReg->glintRegs[PMScreenBase >> 3] = 0;
    pReg->glintRegs[PMHTotal >> 3] -= 1;
    pReg->glintRegs[PMHsStart >> 3] -= 1;
    pReg->glintRegs[PMVTotal >> 3] -= 1;

    pReg->glintRegs[ChipConfig >> 3] = GLINT_READ_REG(ChipConfig) & 0xFFFFFFDD;
    
    pReg->DacRegs[PM2DACIndexMDCR] = 0x00; /* Disable Overlay */
  
    {
	/* Get the programmable clock values */
    	unsigned char m,n,p;
    	unsigned long clockused;
    	unsigned long fref = 14318;
	
    	clockused = PM2DAC_CalculateMNPCForClock(mode->Clock,fref,&m,&n,&p);
	pReg->DacRegs[PM2DACIndexClockAM] = m;
	pReg->DacRegs[PM2DACIndexClockAN] = n;
	pReg->DacRegs[PM2DACIndexClockAP] = p|0x08;
    }

    if (pGlint->MemClock) {
	/* Get the memory clock values */
    	unsigned char m,n,p;
    	unsigned long clockused;
    	unsigned long fref = 14318;
	
    	clockused = PM2DAC_CalculateMNPCForClock(pGlint->MemClock*1000,
								fref,&m,&n,&p);
	pReg->DacRegs[PM2DACIndexMemClockM] = m;
	pReg->DacRegs[PM2DACIndexMemClockN] = n;
	pReg->DacRegs[PM2DACIndexMemClockP] = p;
    }

    if (pScrn->rgbBits == 8)
	pReg->DacRegs[PM2DACIndexMCR] = 0x02; /* 8bit DAC */
    else
        pReg->DacRegs[PM2DACIndexMCR] = 0x00; /* 6bit DAC */

    switch (pScrn->bitsPerPixel)
    {
    case 8:
	pReg->DacRegs[PM2DACIndexCMR] = PM2DAC_RGB | PM2DAC_GRAPHICS |
					PM2DAC_CI8;
    	break;
    case 16:
	if (pScrn->depth == 15) {
	      pReg->DacRegs[PM2DACIndexCMR] = PM2DAC_RGB | PM2DAC_TRUECOLOR|
				        PM2DAC_GRAPHICS | PM2DAC_5551;
	} else {
	    pReg->DacRegs[PM2DACIndexCMR] = PM2DAC_RGB | PM2DAC_TRUECOLOR|
				 	PM2DAC_GRAPHICS | PM2DAC_565;
	}
    	break;
    case 24:
	pReg->DacRegs[PM2DACIndexCMR] = PM2DAC_RGB | PM2DAC_TRUECOLOR|
				 	PM2DAC_GRAPHICS | PM2DAC_PACKED;
    	break;
    case 32:
	pReg->DacRegs[PM2DACIndexCMR] = PM2DAC_RGB | PM2DAC_TRUECOLOR|
				 	PM2DAC_GRAPHICS | PM2DAC_8888;
    	break;
    }

    return(TRUE);
}

void
Permedia2Save(ScrnInfoPtr pScrn, GLINTRegPtr glintReg)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int i;

    glintReg->glintRegs[Aperture0 >> 3] = GLINT_READ_REG(Aperture0);
    glintReg->glintRegs[Aperture1 >> 3] = GLINT_READ_REG(Aperture1);
    glintReg->glintRegs[PMFramebufferWriteMask >> 3] = 
					GLINT_READ_REG(PMFramebufferWriteMask);
    glintReg->glintRegs[PMBypassWriteMask >> 3]  = GLINT_READ_REG(PMBypassWriteMask);
    glintReg->glintRegs[DFIFODis >> 3]  = GLINT_READ_REG(DFIFODis);
    glintReg->glintRegs[FIFODis >> 3]  = GLINT_READ_REG(FIFODis);
    /* We only muck about with PMMemConfig, if user wants to */
    if (pGlint->UseBlockWrite)
	glintReg->glintRegs[PMMemConfig >> 3] = GLINT_READ_REG(PMMemConfig);
    glintReg->glintRegs[PMHTotal >> 3] = GLINT_READ_REG(PMHTotal);
    glintReg->glintRegs[PMHbEnd >> 3] = GLINT_READ_REG(PMHbEnd);
    glintReg->glintRegs[PMHbEnd >> 3] = GLINT_READ_REG(PMHgEnd);
    glintReg->glintRegs[PMScreenStride >> 3] = GLINT_READ_REG(PMScreenStride);
    glintReg->glintRegs[PMHsStart >> 3] = GLINT_READ_REG(PMHsStart);
    glintReg->glintRegs[PMHsEnd >> 3] = GLINT_READ_REG(PMHsEnd);
    glintReg->glintRegs[PMVTotal >> 3] = GLINT_READ_REG(PMVTotal);
    glintReg->glintRegs[PMVbEnd >> 3] = GLINT_READ_REG(PMVbEnd);
    glintReg->glintRegs[PMVsStart >> 3] = GLINT_READ_REG(PMVsStart);
    glintReg->glintRegs[PMVsEnd >> 3] = GLINT_READ_REG(PMVsEnd);
    glintReg->glintRegs[PMScreenBase >> 3] = GLINT_READ_REG(PMScreenBase);
    glintReg->glintRegs[PMVideoControl >> 3] = GLINT_READ_REG(PMVideoControl);
    glintReg->glintRegs[VClkCtl >> 3] = GLINT_READ_REG(VClkCtl);
    glintReg->glintRegs[ChipConfig >> 3] = GLINT_READ_REG(ChipConfig);

    Permedia2ReadAddress(pScrn, 0x00);
    for (i=0;i<768;i++)
	glintReg->cmap[i] = Permedia2ReadData(pScrn);

    glintReg->DacRegs[PM2DACIndexMCR] = 
				Permedia2InIndReg(pScrn, PM2DACIndexMCR);
    glintReg->DacRegs[PM2DACIndexMDCR] =
				Permedia2InIndReg(pScrn, PM2DACIndexMDCR);
    glintReg->DacRegs[PM2DACIndexCMR] = 
				Permedia2InIndReg(pScrn, PM2DACIndexCMR);

    glintReg->DacRegs[PM2DACIndexClockAM] = 
				Permedia2InIndReg(pScrn, PM2DACIndexClockAM);
    glintReg->DacRegs[PM2DACIndexClockAN] = 
				Permedia2InIndReg(pScrn, PM2DACIndexClockAN);
    glintReg->DacRegs[PM2DACIndexClockAP] =
				Permedia2InIndReg(pScrn, PM2DACIndexClockAP);

#if 0 /* In theory we should restore the memory clock, we can't as the register
       * is write only. This code is here for completeness and possible
       * restoring to a default clock */
    if (pGlint->MemClock) {
        glintReg->DacRegs[PM2DACIndexMemClockM] = 
				Permedia2InIndReg(pScrn, PM2DACIndexMemClockM);
        glintReg->DacRegs[PM2DACIndexMemClockN] = 
				Permedia2InIndReg(pScrn, PM2DACIndexMemClockN);
        glintReg->DacRegs[PM2DACIndexMemClockP] =
				Permedia2InIndReg(pScrn, PM2DACIndexMemClockP);
    }
#endif
}

void
Permedia2Restore(ScrnInfoPtr pScrn, GLINTRegPtr glintReg)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int i;

#if 0
    GLINT_SLOW_WRITE_REG(0, ResetStatus);
    while(GLINT_READ_REG(ResetStatus) != 0) {
	xf86MsgVerb(X_INFO, 2, "Resetting Engine - Please Wait.\n");
    };
#endif

    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[Aperture0 >> 3], Aperture0);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[Aperture1 >> 3], Aperture1);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMFramebufferWriteMask >> 3], 
							PMFramebufferWriteMask);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMBypassWriteMask >> 3], 
							PMBypassWriteMask);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[DFIFODis >> 3], DFIFODis);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[FIFODis >> 3], FIFODis);
    /* We only muck about with PMMemConfig, if user wants to */
    if (pGlint->UseBlockWrite)
    	GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMMemConfig >> 3],PMMemConfig);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMVideoControl >> 3], 
								PMVideoControl);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMHbEnd >> 3], PMHgEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMScreenBase >> 3], PMScreenBase);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[VClkCtl >> 3], VClkCtl);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMScreenStride >> 3], 
								PMScreenStride);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMHTotal >> 3], PMHTotal);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMHbEnd >> 3], PMHbEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMHsStart >> 3], PMHsStart);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMHsEnd >> 3], PMHsEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMVTotal >> 3], PMVTotal);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMVbEnd >> 3], PMVbEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMVsStart >> 3], PMVsStart);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMVsEnd >> 3], PMVsEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[ChipConfig >> 3], ChipConfig);

    Permedia2WriteAddress(pScrn, 0x00);
    for (i=0;i<768;i++)
        Permedia2WriteData(pScrn, glintReg->cmap[i]);

    Permedia2OutIndReg(pScrn, PM2DACIndexMCR, 0x00, 
					glintReg->DacRegs[PM2DACIndexMCR]);
    Permedia2OutIndReg(pScrn, PM2DACIndexMDCR, 0x00, 
					glintReg->DacRegs[PM2DACIndexMDCR]);
    Permedia2OutIndReg(pScrn, PM2DACIndexCMR, 0x00, 
					glintReg->DacRegs[PM2DACIndexCMR]);

    Permedia2OutIndReg(pScrn, PM2DACIndexClockAM, 0x00, 
					glintReg->DacRegs[PM2DACIndexClockAM]);
    Permedia2OutIndReg(pScrn, PM2DACIndexClockAN, 0x00, 
					glintReg->DacRegs[PM2DACIndexClockAN]);
    Permedia2OutIndReg(pScrn, PM2DACIndexClockAP, 0x00, 
					glintReg->DacRegs[PM2DACIndexClockAP]);

    if (pGlint->MemClock) {
        Permedia2OutIndReg(pScrn, PM2DACIndexMemClockM, 0x00, 
				glintReg->DacRegs[PM2DACIndexMemClockM]);
        Permedia2OutIndReg(pScrn, PM2DACIndexMemClockN, 0x00, 
				glintReg->DacRegs[PM2DACIndexMemClockN]);
        Permedia2OutIndReg(pScrn, PM2DACIndexMemClockP, 0x00, 
				glintReg->DacRegs[PM2DACIndexMemClockP]);
    }
}

static void
Permedia2ShowCursor(ScrnInfoPtr pScrn)
{
    /* Enable cursor - X11 mode */
    Permedia2OutIndReg(pScrn, PM2DACCursorControl, 0xBC, 0x43);
}

static void
Permedia2HideCursor(ScrnInfoPtr pScrn)
{
    /* Disable cursor */
    Permedia2OutIndReg(pScrn, PM2DACCursorControl, 0xFC, 0x00);
}

static void
Permedia2LoadCursorImage(
    ScrnInfoPtr pScrn, 
    unsigned char *src
)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int i;
       
    Permedia2OutIndReg(pScrn, PM2DACCursorControl, 0xB0, 0x43);
    GLINT_SLOW_WRITE_REG(0x00, PM2DACWriteAddress);
    for (i=0; i<1024; i++) {
	GLINT_SLOW_WRITE_REG(*(src++), PM2DACCursorData);
    }
}

static void
Permedia2SetCursorPosition(
   ScrnInfoPtr pScrn, 
   int x, int y
)
{
    x += 64;
    y += 64;

    /* Output position - "only" 12 bits of location documented */
   
    Permedia2OutIndReg(pScrn, PM2DACCursorXLsb, 0x00, x & 0xFF);
    Permedia2OutIndReg(pScrn, PM2DACCursorXMsb, 0x00, (x>>8) & 0x0F);
    Permedia2OutIndReg(pScrn, PM2DACCursorYLsb, 0x00, y & 0xFF);
    Permedia2OutIndReg(pScrn, PM2DACCursorYMsb, 0x00, (y>>8) & 0x0F);
}

static void
Permedia2SetCursorColors(
   ScrnInfoPtr pScrn, 
   int bg, int fg
)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    /* The Permedia2 cursor is always 8 bits so shift 8, not 10 */

    GLINT_SLOW_WRITE_REG(1, PM2DACCursorColorAddress);
    /* Background color */
    GLINT_SLOW_WRITE_REG(bg >> 0, PM2DACCursorColorData);
    GLINT_SLOW_WRITE_REG(bg >> 8, PM2DACCursorColorData);
    GLINT_SLOW_WRITE_REG(bg >> 16, PM2DACCursorColorData);

    /* Foreground color */
    GLINT_SLOW_WRITE_REG(fg >> 0, PM2DACCursorColorData);
    GLINT_SLOW_WRITE_REG(fg >> 8, PM2DACCursorColorData);
    GLINT_SLOW_WRITE_REG(fg >> 16, PM2DACCursorColorData);
}

static Bool 
Permedia2UseHWCursor(ScreenPtr pScr, CursorPtr pCurs)
{
    return TRUE;
}

Bool 
Permedia2HWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    xf86CursorInfoPtr infoPtr;

    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) return FALSE;
    
    pGlint->CursorInfoRec = infoPtr;

    infoPtr->MaxWidth = 64;
    infoPtr->MaxHeight = 64;
    infoPtr->Flags = HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
		HARDWARE_CURSOR_SOURCE_MASK_NOT_INTERLEAVED |
		HARDWARE_CURSOR_BIT_ORDER_MSBFIRST;
    infoPtr->SetCursorColors = Permedia2SetCursorColors;
    infoPtr->SetCursorPosition = Permedia2SetCursorPosition;
    infoPtr->LoadCursorImage = Permedia2LoadCursorImage;
    infoPtr->HideCursor = Permedia2HideCursor;
    infoPtr->ShowCursor = Permedia2ShowCursor;
    infoPtr->UseHWCursor = Permedia2UseHWCursor;

    return(xf86InitCursor(pScreen, infoPtr));
}

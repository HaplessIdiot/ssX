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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm2v_dac.c,v 1.2 1998/07/25 16:55:48 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "glint_regs.h"
#include "glint.h"

#define PERMEDIA2V_REF_CLOCK 14318

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
PM2VDAC_CalculateClock
(
 unsigned long reqclock,		/* In kHz units */
 unsigned long refclock,		/* In kHz units */
 unsigned char *prescale,		/* ClkPreScale */
 unsigned char *feedback, 		/* ClkFeedBackScale */
 unsigned char *postscale		/* ClkPostScale */
 )
{
    int			f, pre, post;
    unsigned long	freq;
    long		freqerr = 1000;
    unsigned long  	actualclock = 0;
    unsigned char	divide[5] = { 1, 2, 4, 8, 16 };

    for (f=1;f<256;f++) {
	for (pre=1;pre<256;pre++) {
	    for (post=0;post<2;post++) { 
	    	freq = ((refclock * f) / (pre * (1 << divide[post])));
		if ((reqclock > freq - freqerr)&&(reqclock < freq + freqerr)){
		    freqerr = (reqclock > freq) ? 
					reqclock - freq : freq - reqclock;
		    *feedback = f;
		    *prescale = pre;
		    *postscale = post;
		    actualclock = freq;
		}
	    }
	}
    }

    return(actualclock);
}

Bool
Permedia2VInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINTRegPtr pReg = &pGlint->ModeReg;

    pReg->glintRegs[0x00] = 0;
    pReg->glintRegs[0x01] = 0;
    pReg->glintRegs[0x02] = 0xFFFFFFFF;
    pReg->glintRegs[0x03] = 0xFFFFFFFF;

    if (pGlint->UsePCIRetry) {
	pReg->glintRegs[0x04] = 1;
	pReg->glintRegs[0x05] = 3;
    } else {
	pReg->glintRegs[0x04] = 0;
	pReg->glintRegs[0x05] = 1;
    }

    if (pGlint->UseBlockWrite)
	pReg->glintRegs[0x06] = GLINT_READ_REG(PMMemConfig) | 1<<21;

    pReg->glintRegs[0x50] = mode->CrtcHSyncStart - mode->CrtcHDisplay;
    pReg->glintRegs[0x51] = mode->CrtcVSyncStart - mode->CrtcVDisplay;
    pReg->glintRegs[0x52] = mode->CrtcHSyncEnd - mode->CrtcHSyncStart;
    pReg->glintRegs[0x53] = mode->CrtcVSyncEnd - mode->CrtcVSyncStart;

    pReg->glintRegs[0x54] = Shiftbpp(pScrn,mode->CrtcHTotal);
    pReg->glintRegs[0x55] = Shiftbpp(pScrn,pReg->glintRegs[0x50] + 
				     		pReg->glintRegs[0x52]);
    pReg->glintRegs[0x56] = Shiftbpp(pScrn,pReg->glintRegs[0x50]);
    pReg->glintRegs[0x57] = Shiftbpp(pScrn,mode->CrtcHTotal-mode->CrtcHDisplay);
    pReg->glintRegs[0x58] = Shiftbpp(pScrn,pScrn->displayWidth>>1);

    pReg->glintRegs[0x59] = mode->CrtcVTotal;
    pReg->glintRegs[0x5A] = pReg->glintRegs[0x51] + pReg->glintRegs[0x53];
    pReg->glintRegs[0x5B] = pReg->glintRegs[0x51];
    pReg->glintRegs[0x5C] = mode->CrtcVTotal - mode->CrtcVDisplay;

    pReg->glintRegs[0x5D] = 
 	    (((mode->Flags & V_PHSYNC) ? 0x1 : 0x3) << 3) |  
 	    (((mode->Flags & V_PVSYNC) ? 0x1 : 0x3) << 5) | 1; 

    /* We stick the RAMDAC into 64bit mode */
    /* And reduce the horizontal timings and clock by half */
    pReg->glintRegs[0x5D] |= 1<<16;
    pReg->glintRegs[0x54] >>= 1;
    pReg->glintRegs[0x55] >>= 1;
    pReg->glintRegs[0x56] >>= 1;
    pReg->glintRegs[0x57] >>= 1;

    pReg->glintRegs[0x5E] = (GLINT_READ_REG(VClkCtl) & 0xFFFFFFFC);
    pReg->glintRegs[0x5F] = 0; /* PMScreenBase */
    pReg->glintRegs[0x54] -= 1; /* PMHTotal */
    pReg->glintRegs[0x56] -= 1; /* PMHsStart */
    pReg->glintRegs[0x59] -= 1; /* PMVTotal */

    pReg->glintRegs[0x60] = (GLINT_READ_REG(ChipConfig) & 0xFFFFFFFD);
    pReg->glintRegs[0x80] = 0x00;
  
    {
	/* Get the programmable clock values */
    	unsigned char m,n,p;
    	unsigned long clockused;
    	unsigned long fref = PERMEDIA2V_REF_CLOCK;
	
    	clockused = PM2VDAC_CalculateClock(mode->Clock/2,fref,&m,&n,&p);
	pReg->glintRegs[0x70] = m;
	pReg->glintRegs[0x71] = n;
	pReg->glintRegs[0x72] = p;
    }

    switch (pScrn->depth)
    {
    case 8:
    	if (pScrn->rgbBits == 8)
            pReg->glintRegs[0x61] = 0x01; /* 8bit DAC */
    	else
	    pReg->glintRegs[0x61] = 0x00; /* 6bit DAC */
	pReg->glintRegs[0x81] = 0x00;
	pReg->glintRegs[0x82] = 0x2E;
    	break;
    case 15:
        pReg->glintRegs[0x61] = 0x09; /* 8bit DAC */
	pReg->glintRegs[0x81] = 0x01;
	pReg->glintRegs[0x82] = 0x61;
    	break;
    case 16:
        pReg->glintRegs[0x61] = 0x09; /* 8bit DAC */
	pReg->glintRegs[0x81] = 0x01;
	pReg->glintRegs[0x82] = 0x70;
    	break;
    case 24:
        pReg->glintRegs[0x61] = 0x09; /* 8bit DAC */
	pReg->glintRegs[0x81] = 0x04;
	pReg->glintRegs[0x82] = 0x20;
    	break;
    case 32:
        pReg->glintRegs[0x61] = 0x09; /* 8bit DAC */
	pReg->glintRegs[0x81] = 0x02;
	pReg->glintRegs[0x82] = 0x20;
    	break;
    }

    return(TRUE);
}

void
Permedia2VSave(ScrnInfoPtr pScrn, GLINTRegPtr glintReg)
{
    int i;
    GLINTPtr pGlint = GLINTPTR(pScrn);

    glintReg->glintRegs[0x60] = GLINT_READ_REG(ChipConfig);
    glintReg->glintRegs[0x00]  = GLINT_READ_REG(Aperture0);
    glintReg->glintRegs[0x01]  = GLINT_READ_REG(Aperture1);
    glintReg->glintRegs[0x02]  = GLINT_READ_REG(PMFramebufferWriteMask);
    glintReg->glintRegs[0x03]  = GLINT_READ_REG(PMBypassWriteMask);
    glintReg->glintRegs[0x04]  = GLINT_READ_REG(DFIFODis);
    glintReg->glintRegs[0x05]  = GLINT_READ_REG(FIFODis);
    /* We only muck about with PMMemConfig, if user wants to */
    if (pGlint->UseBlockWrite)
	glintReg->glintRegs[0x06] = GLINT_READ_REG(PMMemConfig);
    glintReg->glintRegs[0x54] = GLINT_READ_REG(PMHTotal);
    glintReg->glintRegs[0x57] = GLINT_READ_REG(PMHbEnd);
    glintReg->glintRegs[0x57] = GLINT_READ_REG(PMHgEnd);
    glintReg->glintRegs[0x58] = GLINT_READ_REG(PMScreenStride);
    glintReg->glintRegs[0x56] = GLINT_READ_REG(PMHsStart);
    glintReg->glintRegs[0x55] = GLINT_READ_REG(PMHsEnd);
    glintReg->glintRegs[0x59] = GLINT_READ_REG(PMVTotal);
    glintReg->glintRegs[0x5C] = GLINT_READ_REG(PMVbEnd);
    glintReg->glintRegs[0x5B] = GLINT_READ_REG(PMVsStart);
    glintReg->glintRegs[0x5A] = GLINT_READ_REG(PMVsEnd);
    glintReg->glintRegs[0x5F] = GLINT_READ_REG(PMScreenBase);
    glintReg->glintRegs[0x5D] = GLINT_READ_REG(PMVideoControl);
    glintReg->glintRegs[0x5E] = GLINT_READ_REG(VClkCtl);

    /* save colors */
    GLINT_SLOW_WRITE_REG(0xFF, PM2DACReadMask);
    GLINT_SLOW_WRITE_REG(0x00, PM2DACReadAddress);
    for (i=0; i<768; i++)
	glintReg->DacRegs[i] = GLINT_READ_REG(PM2DACData);

    GLINT_SLOW_WRITE_REG(0, PM2VDACIndexRegHigh);

    GLINT_SLOW_WRITE_REG(PM2VDACRDMiscControl, PM2VDACIndexRegLow);
    glintReg->glintRegs[0x61] = GLINT_READ_REG(PM2VDACIndexData);

    GLINT_SLOW_WRITE_REG(PM2VDACRDDACControl, PM2VDACIndexRegLow);
    glintReg->glintRegs[0x80] = GLINT_READ_REG(PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(PM2VDACRDPixelSize, PM2VDACIndexRegLow);
    glintReg->glintRegs[0x81] = GLINT_READ_REG(PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(PM2VDACRDColorFormat, PM2VDACIndexRegLow);
    glintReg->glintRegs[0x82] = GLINT_READ_REG(PM2VDACIndexData);

    GLINT_SLOW_WRITE_REG(PM2VDACRDDClk0PreScale >> 8, PM2VDACIndexRegHigh);
    GLINT_SLOW_WRITE_REG(PM2VDACRDDClk0PreScale, PM2VDACIndexRegLow);
    glintReg->glintRegs[0x70] = GLINT_READ_REG(PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(PM2VDACRDDClk0FeedbackScale, PM2VDACIndexRegLow);
    glintReg->glintRegs[0x71] = GLINT_READ_REG(PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(PM2VDACRDDClk0PostScale, PM2VDACIndexRegLow);
    glintReg->glintRegs[0x72] = GLINT_READ_REG(PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(0, PM2VDACIndexRegHigh);
}

void
Permedia2VRestore(ScrnInfoPtr pScrn, GLINTRegPtr glintReg)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    CARD32 temp;

    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x60], ChipConfig);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x00], Aperture0);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x01], Aperture1);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x02], PMFramebufferWriteMask);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x03], PMBypassWriteMask);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x04], DFIFODis);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x05], FIFODis);
    /* We only muck about with PMMemConfig, if user wants to */
    if (pGlint->UseBlockWrite)
    	GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x06], PMMemConfig);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5D], PMVideoControl);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x57], PMHgEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5F], PMScreenBase);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5E], VClkCtl);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x58], PMScreenStride);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x54], PMHTotal);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x57], PMHbEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x56], PMHsStart);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x55], PMHsEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x59], PMVTotal);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5C], PMVbEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5B], PMVsStart);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5A], PMVsEnd);

    GLINT_SLOW_WRITE_REG(0, PM2VDACIndexRegHigh);

    GLINT_SLOW_WRITE_REG(PM2VDACRDMiscControl, PM2VDACIndexRegLow);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x61],PM2VDACIndexData);

    GLINT_SLOW_WRITE_REG(PM2VDACRDDACControl, PM2VDACIndexRegLow);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x80],PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(PM2VDACRDPixelSize, PM2VDACIndexRegLow);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x81],PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(PM2VDACRDColorFormat, PM2VDACIndexRegLow);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x82],PM2VDACIndexData);

    GLINT_SLOW_WRITE_REG(PM2VDACIndexClockControl >> 8, PM2VDACIndexRegHigh);
    GLINT_SLOW_WRITE_REG(PM2VDACIndexClockControl, PM2VDACIndexRegLow);
    temp = GLINT_READ_REG(PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(temp & 0xFC, PM2VDACIndexData);

    GLINT_SLOW_WRITE_REG(PM2VDACRDDClk0PreScale, PM2VDACIndexRegLow);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x70], PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(PM2VDACRDDClk0FeedbackScale, PM2VDACIndexRegLow);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x71], PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(PM2VDACRDDClk0PostScale, PM2VDACIndexRegLow);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x72], PM2VDACIndexData);

    GLINT_SLOW_WRITE_REG(PM2VDACIndexClockControl, PM2VDACIndexRegLow);
    GLINT_SLOW_WRITE_REG(temp | 0x03, PM2VDACIndexData);
    GLINT_SLOW_WRITE_REG(0, PM2VDACIndexRegHigh);

}

static void 
Permedia2vShowCursor(ScrnInfoPtr pScrn)
{
    /* Enable cursor - X11 mode */
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorMode, 0x00, 0x11);
}

static void
Permedia2vHideCursor(ScrnInfoPtr pScrn)
{
    /* Disable cursor - X11 mode */
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorMode, 0x00, 0x10);
}

static void
Permedia2vLoadCursorImage(
    ScrnInfoPtr pScrn, 
    unsigned char *src
)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int i;
       
    GLINT_SLOW_WRITE_REG((PM2VDACRDCursorPattern>>8)&0xff, PM2VDACIndexRegHigh);
    for (i=0; i<1024; i++) 
    	Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPattern+i, 0x00, *(src++));
    GLINT_SLOW_WRITE_REG(0, PM2VDACIndexRegHigh);
}

static void
Permedia2vSetCursorPosition(
   ScrnInfoPtr pScrn, 
   int x, int y
)
{
    x += 64;
    y += 64;

    /* Output position - "only" 12 bits of location documented */
   
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorHotSpotX, 0x00, 0x00);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorHotSpotY, 0x00, 0x00);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorXLow, 0x00, x & 0xFF);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorXHigh, 0x00, (x>>8) & 0x0F);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorYLow, 0x00, y & 0xFF);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorYHigh, 0x00, (y>>8) & 0x0F);
}

static void
Permedia2vSetCursorColors(
   ScrnInfoPtr pScrn, 
   int bg, int fg
)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    /* The Permedia2v cursor is always 8 bits so shift 8, not 10 */

    GLINT_SLOW_WRITE_REG((PM2VDACRDCursorPalette>>8)&0xff, PM2VDACIndexRegHigh);
    /* Background color */
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+6, 0x00, bg >> 16);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+7, 0x00, bg >> 8);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+8, 0x00, bg);

    /* Foreground color */
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+9, 0x00, fg >> 16);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+10, 0x00, fg >> 8);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+11, 0x00, fg);
   
    GLINT_SLOW_WRITE_REG(0, PM2VDACIndexRegHigh);
}

static Bool 
Permedia2vUseHWCursor(ScreenPtr pScr, CursorPtr pCurs)
{
    return TRUE;
}

Bool 
Permedia2vHWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    XAACursorInfoPtr infoPtr;

    infoPtr = XAACreateCursorInfoRec();
    if(!infoPtr) return FALSE;
    
    pGlint->CursorInfoRec = infoPtr;

    infoPtr->MaxWidth = 64;
    infoPtr->MaxHeight = 64;
    infoPtr->Flags = 0;
    infoPtr->SetCursorColors = Permedia2vSetCursorColors;
    infoPtr->SetCursorPosition = Permedia2vSetCursorPosition;
    infoPtr->LoadCursorImage = Permedia2vLoadCursorImage;
    infoPtr->HideCursor = Permedia2vHideCursor;
    infoPtr->ShowCursor = Permedia2vShowCursor;
    infoPtr->UseHWCursor = Permedia2vUseHWCursor;

    return(XAAInitCursor(pScreen, infoPtr));
}

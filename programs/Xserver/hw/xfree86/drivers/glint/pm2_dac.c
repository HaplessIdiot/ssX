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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm2_dac.c,v 1.1.2.5 1998/07/18 17:53:37 dawes Exp $ */

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

    if (pScrn->bitsPerPixel > 8) {
	/* When != 8bpp then we stick the RAMDAC into 64bit mode */
	/* And reduce the horizontal timings by half */
	pReg->glintRegs[0x5D] |= 1<<16;
    	pReg->glintRegs[0x54] >>= 1;
	pReg->glintRegs[0x55] >>= 1;
	pReg->glintRegs[0x56] >>= 1;
	pReg->glintRegs[0x57] >>= 1;
    }

    pReg->glintRegs[0x5E] = (GLINT_READ_REG(VClkCtl) & 0xFFFFFFFC);
    pReg->glintRegs[0x5F] = 0; /* PMScreenBase */
    pReg->glintRegs[0x54] -= 1; /* PMHTotal */
    pReg->glintRegs[0x56] -= 1; /* PMHsStart */
    pReg->glintRegs[0x59] -= 1; /* PMVTotal */

    pReg->glintRegs[0x60] = (GLINT_READ_REG(ChipConfig) & 0xFFFFFFFD);
    
    pReg->glintRegs[0x62] = 0x00; /* Disable Overlay */
  
    {
	/* Get the programmable clock values */
    	unsigned char m,n,p;
    	unsigned long clockused;
    	unsigned long fref = 14318;
	
    	clockused = PM2DAC_CalculateMNPCForClock(mode->Clock,fref,&m,&n,&p);
	pReg->glintRegs[0x70] = m;
	pReg->glintRegs[0x71] = n;
	pReg->glintRegs[0x72] = p|0x08;
    }

    switch (pScrn->depth)
    {
    case 8:
	pReg->glintRegs[0x80] = PM2DAC_RGB|PM2DAC_GRAPHICS|PM2DAC_CI8;
	if (pScrn->rgbBits == 8)
            pReg->glintRegs[0x61] = 0x02; /* 8bit DAC */
	else
            pReg->glintRegs[0x61] = 0x00; /* 6bit DAC */
    	break;
    case 15:
	pReg->glintRegs[0x80] = PM2DAC_RGB|PM2DAC_TRUECOLOR|
				 PM2DAC_GRAPHICS|PM2DAC_5551;
        pReg->glintRegs[0x61] = 0x02; /* 8bit DAC */
    	break;
    case 16:
	pReg->glintRegs[0x80] = PM2DAC_RGB|PM2DAC_TRUECOLOR|
				 PM2DAC_GRAPHICS|PM2DAC_565;
        pReg->glintRegs[0x61] = 0x02; /* 8bit DAC */
    	break;
    case 24:
	pReg->glintRegs[0x80] = PM2DAC_RGB|PM2DAC_TRUECOLOR|
				 PM2DAC_GRAPHICS|PM2DAC_PACKED;
        pReg->glintRegs[0x61] = 0x02; /* 8bit DAC */
    	break;
    case 32:
	pReg->glintRegs[0x80] = PM2DAC_RGB|PM2DAC_TRUECOLOR|
				 PM2DAC_GRAPHICS|PM2DAC_8888;
        pReg->glintRegs[0x61] = 0x02; /* 8bit DAC */
    	break;
    }

    return(TRUE);
}

void
Permedia2Save(ScrnInfoPtr pScrn, GLINTRegPtr glintReg)
{
    int i;
    GLINTPtr pGlint = GLINTPTR(pScrn);

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
    glintReg->glintRegs[0x60] = GLINT_READ_REG(ChipConfig);

    /* save colors */
    GLINT_SLOW_WRITE_REG(0xFF, PM2DACReadMask);
    GLINT_SLOW_WRITE_REG(0x00, PM2DACReadAddress);
    for (i=0; i<768; i++)
	glintReg->DacRegs[i] = GLINT_READ_REG(PM2DACData);

    GLINT_SLOW_WRITE_REG(PM2DACIndexMCR, PM2DACIndexReg);
    glintReg->glintRegs[0x61] = GLINT_READ_REG(PM2DACIndexData); 
    GLINT_SLOW_WRITE_REG(PM2DACIndexMDCR, PM2DACIndexReg);
    glintReg->glintRegs[0x62] = GLINT_READ_REG(PM2DACIndexData); 

    GLINT_SLOW_WRITE_REG(PM2DACIndexCMR, PM2DACIndexReg);
    glintReg->glintRegs[0x80] = GLINT_READ_REG(PM2DACIndexData);

    GLINT_SLOW_WRITE_REG(PM2DACIndexClockAM, PM2DACIndexReg);
    glintReg->glintRegs[0x70] = GLINT_READ_REG(PM2DACIndexData);
    GLINT_SLOW_WRITE_REG(PM2DACIndexClockAN, PM2DACIndexReg);
    glintReg->glintRegs[0x71] = GLINT_READ_REG(PM2DACIndexData);
    GLINT_SLOW_WRITE_REG(PM2DACIndexClockAP, PM2DACIndexReg);
    glintReg->glintRegs[0x72] = GLINT_READ_REG(PM2DACIndexData);
}

void
Permedia2Restore(ScrnInfoPtr pScrn, GLINTRegPtr glintReg)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

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
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x60], ChipConfig);

    GLINT_SLOW_WRITE_REG(PM2DACIndexMCR, PM2DACIndexReg);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x61], PM2DACIndexData); 
    GLINT_SLOW_WRITE_REG(PM2DACIndexMDCR, PM2DACIndexReg);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x62], PM2DACIndexData); 

    GLINT_SLOW_WRITE_REG(PM2DACIndexCMR, PM2DACIndexReg);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x80],PM2DACIndexData);

    GLINT_SLOW_WRITE_REG(PM2DACIndexClockAM, PM2DACIndexReg);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x70], PM2DACIndexData);
    GLINT_SLOW_WRITE_REG(PM2DACIndexClockAN, PM2DACIndexReg);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x71], PM2DACIndexData);
    GLINT_SLOW_WRITE_REG(PM2DACIndexClockAP, PM2DACIndexReg);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x72], PM2DACIndexData);
}

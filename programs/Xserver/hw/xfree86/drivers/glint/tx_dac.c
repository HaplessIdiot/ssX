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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/tx_dac.c,v 1.1.2.2 1998/07/18 17:53:39 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "IBM.h"
#include "glint_regs.h"
#include "glint.h"

#define GLINT_DELTA_REF_CLOCK 40000

static int
Shiftbpp(ScrnInfoPtr pScrn, int value)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int logbytesperaccess = 3;
	
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

Bool
TXInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINTRegPtr pReg = &pGlint->ModeReg;
    RamDacHWRecPtr pIBM = RAMDACHWPTR(pScrn);
    RamDacRegRecPtr ramdacReg = &pIBM->ModeReg;

    pReg->glintRegs[0x00] = 0;
    pReg->glintRegs[0x01] = 0;

    if (pGlint->UsePCIRetry) {
	pReg->glintRegs[0x04] = 1;
	pReg->glintRegs[0x05] = 3;
    } else {
	pReg->glintRegs[0x04] = 0;
	pReg->glintRegs[0x05] = 1;
    }

    pReg->glintRegs[0x50] = mode->CrtcHSyncStart - mode->CrtcHDisplay;
    pReg->glintRegs[0x51] = mode->CrtcVSyncStart - mode->CrtcVDisplay;
    pReg->glintRegs[0x52] = mode->CrtcHSyncEnd - mode->CrtcHSyncStart;
    pReg->glintRegs[0x53] = mode->CrtcVSyncEnd - mode->CrtcVSyncStart;

    pReg->glintRegs[0x54] = Shiftbpp(pScrn,mode->CrtcHTotal);
    pReg->glintRegs[0x55] = Shiftbpp(pScrn,pReg->glintRegs[0x50] + 
					   pReg->glintRegs[0x52]);
    pReg->glintRegs[0x56] = Shiftbpp(pScrn,pReg->glintRegs[0x50]);
    pReg->glintRegs[0x57] = Shiftbpp(pScrn,mode->CrtcHTotal-mode->CrtcHDisplay);

    pReg->glintRegs[0x59] = mode->CrtcVTotal;
    pReg->glintRegs[0x5A] = pReg->glintRegs[0x51] + pReg->glintRegs[0x53];
    pReg->glintRegs[0x5B] = pReg->glintRegs[0x51];
    pReg->glintRegs[0x5C] = mode->CrtcVTotal - mode->CrtcVDisplay;

    pReg->glintRegs[0x5D] = (((mode->Flags & V_PHSYNC) ? 0 : 2) << 2) |
			     ((mode->Flags & V_PVSYNC) ? 0 : 2) | (0xb0);

    pReg->glintRegs[0x5E] = 0; /* VClkCtl */
    pReg->glintRegs[0x5F] = pReg->glintRegs[0x5C] - 1; /* VTGVGateStart */
    pReg->glintRegs[0x61] = pReg->glintRegs[0x5C]; /* VTGVGateEnd */
    /*
     * tell DAC to use the ICD chip clock 0 as ref clock 
     * and set up some more video timining generator registers
     */
    pReg->glintRegs[0x62] = 0x05; /* VTGSerialClk */

    /* This is ugly */
    if (pGlint->UseFireGL3000) {
	pReg->glintRegs[0x60] = pReg->glintRegs[0x57] - 1; /* VTGHGateStart */
	pReg->glintRegs[0x65] = pReg->glintRegs[0x54] - 1; /* VTGHGateEnd */
	pReg->glintRegs[0x63] = 0x907; /* FBModeSel */
	pReg->glintRegs[0x64] = 0x00; /* VTGModeCtl */
    } else {
	pReg->glintRegs[0x60] = pReg->glintRegs[0x57] - 2; /* VTGHGateStart */
	pReg->glintRegs[0x65] = pReg->glintRegs[0x54] - 2; /* VTGHGateEnd */
	pReg->glintRegs[0x63] = 0x0A07; /* FBModeSel */
	pReg->glintRegs[0x64] = 0x44; /* VTGModeCtl */
    }

    {
	/* Get the programmable clock values */
    	unsigned long m=0,n=0,p=0,c=0;
    	unsigned long clock;
	unsigned long refclock;

	refclock = GLINT_DELTA_REF_CLOCK;
	
    	clock = IBMramdac526CalculateMNPCForClock(refclock, mode->Clock, 1,
			pGlint->MinClock, pGlint->MaxClock, &m, &n, &p, &c);
			
	ramdacReg->DacRegs[IBMRGB_m0] = m;
	ramdacReg->DacRegs[IBMRGB_n0] = n;
	ramdacReg->DacRegs[IBMRGB_p0] = p;
	ramdacReg->DacRegs[IBMRGB_c0] = c;

	ramdacReg->DacRegs[IBMRGB_pll_ctrl1] = 0x05;
	ramdacReg->DacRegs[IBMRGB_pll_ctrl2] = 0x00;

    	clock = IBMramdac526CalculateMNPCForClock(refclock, mode->Clock, 0,
			pGlint->MinClock, pGlint->MaxClock, &m, &n, &p, &c);

	ramdacReg->DacRegs[IBMRGB_sysclk] = 0x05;
	ramdacReg->DacRegs[IBMRGB_sysclk_m] = m;
	ramdacReg->DacRegs[IBMRGB_sysclk_n] = n;
	ramdacReg->DacRegs[IBMRGB_sysclk_p] = p;
	ramdacReg->DacRegs[IBMRGB_sysclk_c] = c;
    }

    ramdacReg->DacRegs[IBMRGB_misc1] = SENS_DSAB_DISABLE | VRAM_SIZE_64;
    ramdacReg->DacRegs[IBMRGB_misc2] = COL_RES_8BIT | PORT_SEL_VRAM | PCLK_SEL_PLL;
    ramdacReg->DacRegs[IBMRGB_misc3] = 0;
    ramdacReg->DacRegs[IBMRGB_misc_clock] = 1;
    ramdacReg->DacRegs[IBMRGB_sync] = 0;
    ramdacReg->DacRegs[IBMRGB_hsync_pos] = 0;
    ramdacReg->DacRegs[IBMRGB_pwr_mgmt] = 0;
    ramdacReg->DacRegs[IBMRGB_dac_op] = 0;
    ramdacReg->DacRegs[IBMRGB_pal_ctrl] = 0;

    IBMramdacSetBpp(pScrn, ramdacReg);

    return(TRUE);
}

void
TXSave(ScrnInfoPtr pScrn, GLINTRegPtr glintReg)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    glintReg->glintRegs[0x00]  = GLINT_READ_REG(Aperture0);
    glintReg->glintRegs[0x01]  = GLINT_READ_REG(Aperture1);

    glintReg->glintRegs[0x04]  = GLINT_READ_REG(DFIFODis);
    glintReg->glintRegs[0x05]  = GLINT_READ_REG(FIFODis);

    glintReg->glintRegs[0x5E] = GLINT_READ_REG(VClkCtl);
    glintReg->glintRegs[0x5D] = GLINT_READ_REG(VTGPolarity);
    glintReg->glintRegs[0x54] = GLINT_READ_REG(VTGHLimit);
    glintReg->glintRegs[0x57] = GLINT_READ_REG(VTGHBlankEnd);
    glintReg->glintRegs[0x56] = GLINT_READ_REG(VTGHSyncStart);
    glintReg->glintRegs[0x55] = GLINT_READ_REG(VTGHSyncEnd);
    glintReg->glintRegs[0x59] = GLINT_READ_REG(VTGVLimit);
    glintReg->glintRegs[0x5C] = GLINT_READ_REG(VTGVBlankEnd);
    glintReg->glintRegs[0x5B] = GLINT_READ_REG(VTGVSyncStart);
    glintReg->glintRegs[0x5A] = GLINT_READ_REG(VTGVSyncEnd);
    glintReg->glintRegs[0x5F] = GLINT_READ_REG(VTGVGateStart);
    glintReg->glintRegs[0x61] = GLINT_READ_REG(VTGVGateEnd);
    glintReg->glintRegs[0x62] = GLINT_READ_REG(VTGSerialClk);
    glintReg->glintRegs[0x63] = GLINT_READ_REG(FBModeSel);
    glintReg->glintRegs[0x64] = GLINT_READ_REG(VTGModeCtl);
    glintReg->glintRegs[0x60] = GLINT_READ_REG(VTGHGateStart);
    glintReg->glintRegs[0x65] = GLINT_READ_REG(VTGHGateEnd);
}

void
TXRestore(ScrnInfoPtr pScrn, GLINTRegPtr glintReg)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);

    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x00], Aperture0);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x01], Aperture1);

    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x04], DFIFODis);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x05], FIFODis);

    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5D], VTGPolarity);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5E], VClkCtl);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x62], VTGSerialClk);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x64], VTGModeCtl);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x54], VTGHLimit);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x56], VTGHSyncStart);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x55], VTGHSyncEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x57], VTGHBlankEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x59], VTGVLimit);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5B], VTGVSyncStart);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5A], VTGVSyncEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5C], VTGVBlankEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x5F], VTGVGateStart);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x61], VTGVGateEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x63], FBModeSel);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x60], VTGHGateStart);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[0x65], VTGHGateEnd);
}

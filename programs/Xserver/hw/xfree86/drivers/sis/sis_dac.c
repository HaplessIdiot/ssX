/*
 * Copyright 1998,1999 by Alan Hourihane, Wigan, England.
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
 * Authors:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *           Mike Chapman <mike@paranoia.com>, 
 *           Juanjo Santamarta <santamarta@ctv.es>, 
 *           Mitani Hiroshi <hmitani@drl.mei.co.jp> 
 *           David Thomas <davtom@dream.org.uk>. 
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_dac.c,v 1.1 1999/01/23 09:55:53 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Version.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "vgaHW.h"

#include "sis.h"
#include "sis_regs.h"

static void
SiSCalcClock(int clock, int max_VLD, unsigned int *vclk)
{
    int M, N, P, PSN, VLD, PSNx;
    int bestM, bestN, bestP, bestPSN, bestVLD;
    double bestError, abest = 42.0, bestFout;
    double target;
    double Fvco, Fout;
    double error, aerror;

    /*
     *	fd = fref*(Numerator/Denumerator)*(Divider/PostScaler)
     *
     *	M 	= Numerator [1:128] 
     *  N 	= DeNumerator [1:32]
     *  VLD	= Divider (Vco Loop Divider) : divide by 1, 2
     *  P	= Post Scaler : divide by 1, 2, 3, 4
     *  PSN     = Pre Scaler (Reference Divisor Select) 
     * 
     * result in vclk[]
     */
#define Midx 	0
#define Nidx 	1
#define VLDidx 	2
#define Pidx 	3
#define PSNidx 	4
#define Fref 14318180
/* stability constraints for internal VCO -- MAX_VCO also determines 
 * the maximum Video pixel clock */
#define MIN_VCO Fref
#define MAX_VCO 135000000
#define MAX_VCO_5597 353000000
#define MAX_PSN 0 /* no pre scaler for this chip */
#define TOLERANCE 0.01	/* search smallest M and N in this tolerance */
  
  int M_min = 2;
  int M_max = 128;
  
/*  abest=10000.0; */
 
  target = clock * 1000;
 
#if 0 /* Implied at the moment */
     if (SISchipset == SIS5597 || SISchipset == SIS6326){
#endif
    {
 	int low_N = 2;
 	int high_N = 5;
 	int PSN = 1;
 
 	P = 1;
 	if (target < MAX_VCO_5597 / 2)
 	    P = 2;
 	if (target < MAX_VCO_5597 / 3)
 	    P = 3;
 	if (target < MAX_VCO_5597 / 4)
 	    P = 4;
 	if (target < MAX_VCO_5597 / 6)
 	    P = 6;
 	if (target < MAX_VCO_5597 / 8)
 	    P = 8;
 
 	Fvco = P * target;
 
 	for (N = low_N; N <= high_N; N++){
 	    double M_desired = Fvco / Fref * N;
 	    if (M_desired > M_max * max_VLD)
 		continue;
 
 	    if ( M_desired > M_max ) {
 		M = M_desired / 2 + 0.5;
 		VLD = 2;
 	    } else {
 		M = Fvco / Fref * N + 0.5;
 		VLD = 1;
 	    };
 
 	    Fout = (double)Fref * (M * VLD)/(N * P);
 
 	    error = (target - Fout) / target;
 	    aerror = (error < 0) ? -error : error;
/* 	    if (aerror < abest && abest > TOLERANCE) {*/
 	    if (aerror < abest) {
 	        abest = aerror;
 	        bestError = error;
 	        bestM = M;
 	        bestN = N;
 	        bestP = P;
 	        bestPSN = PSN;
 	        bestVLD = VLD;
 	        bestFout = Fout;
 	    }
 	}
     }
#if 0
     else {
         for (PSNx = 0; PSNx <= MAX_PSN ; PSNx++) {
 	    int low_N, high_N;
 	    double FrefVLDPSN;
 
 	    PSN = !PSNx ? 1 : 4;
 
 	    low_N = 2;
 	    high_N = 32;
 
 	    for ( VLD = 1 ; VLD <= max_VLD ; VLD++ ) {
 
 	        FrefVLDPSN = (double)Fref * VLD / PSN;
 	        for (N = low_N; N <= high_N; N++) {
 		    double tmp = FrefVLDPSN / N;
 
 		    for (P = 1; P <= 4; P++) {	
 		        double Fvco_desired = target * ( P );
 		        double M_desired = Fvco_desired / tmp;
 
 		        /* Which way will M_desired be rounded?  
 		         *  Do all three just to be safe.  
 		         */
 		        int M_low = M_desired - 1;
 		        int M_hi = M_desired + 1;
 
 		        if (M_hi < M_min || M_low > M_max)
 			    continue;
 
 		        if (M_low < M_min)
 			    M_low = M_min;
 		        if (M_hi > M_max)
 			    M_hi = M_max;
 
 		        for (M = M_low; M <= M_hi; M++) {
 			    Fvco = tmp * M;
 			    if (Fvco <= MIN_VCO)
 			        continue;
 			    if (Fvco > MAX_VCO)
 			        break;
 
 			    Fout = Fvco / ( P );
 
 			    error = (target - Fout) / target;
 			    aerror = (error < 0) ? -error : error;
 			    if (aerror < abest) {
 			        abest = aerror;
 			        bestError = error;
 			        bestM = M;
 			        bestN = N;
 			        bestP = P;
 			        bestPSN = PSN;
 			        bestVLD = VLD;
 			        bestFout = Fout;
 			    }
#ifdef DEBUG1
 			ErrorF("Freq. selected: %.2f MHz, M=%d, N=%d, VLD=%d,"
 			       " P=%d, PSN=%d\n",
 			       (float)(clock / 1000.), M, N, P, VLD, PSN);
 			ErrorF("Freq. set: %.2f MHz\n", Fout / 1.0e6);
#endif
 		        }
 		    }
 	        }
 	    }
         }
  }
#endif
  vclk[Midx]    = bestM;
  vclk[Nidx]    = bestN;
  vclk[VLDidx]  = bestVLD;
  vclk[Pidx]    = bestP;
  vclk[PSNidx]  = bestPSN;
#ifdef DEBUG
     ErrorF("Freq. selected: %.2f MHz, M=%d, N=%d, VLD=%d, P=%d, PSN=%d\n",
 	(float)(clock / 1000.), vclk[Midx], vclk[Nidx], vclk[VLDidx], 
 	   vclk[Pidx], vclk[PSNidx]);
     ErrorF("Freq. set: %.2f MHz\n", bestFout / 1.0e6);
     ErrorF("VCO Freq.: %.2f MHz\n", bestFout*bestP / 1.0e6);
#endif
#ifdef DEBUG1
                       ErrorF("abest=%f\n",
                               abest);
#endif
}

Bool
SiSInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    SISPtr pSiS = SISPTR(pScrn);
    SISRegPtr pReg = &pSiS->ModeReg;
    int vgaIOBase;
    unsigned char temp;
    int offset;
    int clock = mode->Clock;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    offset = pScrn->displayWidth >> (mode->Flags & V_INTERLACE ? 2 : 3);

    outb(0x3C4, 0x05); /* Unlock Registers */
    temp = inb(0x3C5);
    outw(0x3C4, 0x8605);

    outb(0x3C4, BankReg);
    pReg->sisRegs3C4[BankReg] |= 0x82;			/* Enable Linear */
    pReg->sisRegs3C4[CPUThreshold] = 0x0F;
    pReg->sisRegs3C4[CRTThreshold] = 0x0F;

    switch (pScrn->depth) {
	case 8:
    	    pReg->sisRegs3C4[CPUThreshold] |= 0x40;
	    break;
	case 15:
	    offset <<= 1;
	    pReg->sisRegs3C4[BankReg] |= 0x04;
    	    pReg->sisRegs3C4[CPUThreshold] |= 0x80;
	    break;
	case 16:
	    offset <<= 1;
	    pReg->sisRegs3C4[BankReg] |= 0x08;
    	    pReg->sisRegs3C4[CPUThreshold] |= 0x80;
	    break;
	case 24:
	    offset += (offset << 1);
	    pReg->sisRegs3C4[BankReg] |= 0x80;
    	    pReg->sisRegs3C4[CPUThreshold] |= 0xC0;
	    break;
    }

    pReg->sisRegs3C4[LinearAdd0] = (pSiS->FbAddress & 0x07F80000) >> 19;
    pReg->sisRegs3C4[LinearAdd1] = ((pSiS->FbAddress & 0xF8000000) >> 27) | 0x60;
    pReg->sisRegs3x4[Offset] = offset & 0xFF;
    pReg->sisRegs3C4[CRTCOff] = ((offset & 0xF00) >> 4) | 
				(((mode->CrtcVTotal-2) & 0x400) >> 10 ) |
				(((mode->CrtcVDisplay-1) & 0x400) >> 9 ) |
				((mode->CrtcVSyncStart & 0x400) >> 8 ) |
				(((mode->CrtcVSyncStart) & 0x400) >> 7 ) ;
	
    {
    	unsigned int vclk[5];
    	unsigned char xr2a, xr2b;

    	SiSCalcClock(clock, 2, vclk);

	xr2a = (vclk[Midx] - 1) & 0x7f ;
	xr2a |= ((vclk[VLDidx] == 2 ) ? 1 : 0 ) << 7 ;
	xr2b = (vclk[Nidx] -1) & 0x1f ;	/* bits [4:0] contain denumerator -MC */

	outb(0x3C4, ClockBase);
    	pReg->sisRegs3C4[ClockBase] = inb(0x3C5);
 	if (vclk[Pidx] <= 4){
 	    xr2b |= (vclk[Pidx] -1 ) << 5 ; /* postscale 1,2,3,4 */
    	    pReg->sisRegs3C4[ClockBase] &= 0xBF;
 	} else {
	    xr2b |= ((vclk[Pidx] / 2) -1 ) << 5 ;  /* postscale 6,8 */
    	    pReg->sisRegs3C4[ClockBase] |= 0x40;
 	};
 	xr2b |= 0x80 ;   /* gain for high frequency */
 
	pReg->sisRegs3C4[XR2A] = xr2a;
	pReg->sisRegs3C4[XR2B] = xr2b;
	pReg->sisRegs3C2[0x00] = inb(0x3CC) | 0x0C;
    }

    if (!pSiS->NoAccel) {
    	outb(0x3C4, GraphEng);
    	pReg->sisRegs3C4[GraphEng] = inb(0x3C5) | 0x40;
	if (pSiS->TurboQueue) {
    	    pReg->sisRegs3C4[GraphEng] |= 0x80;
	    outb(0x3C4, ExtMiscCont9);
    	    pReg->sisRegs3C4[ExtMiscCont9] = inb(0x3C5) & 0xFC; /* All Queue for 2D */
    	    pReg->sisRegs3C4[TurboQueueBase] = (pScrn->videoRam/32) - 1;
	}
	outb(0x3C4, Mode64);
    	pReg->sisRegs3C4[Mode64] = inb(0x3C5) | 0x80;
	outb(0x3C4, MMIOEnable);
    	pReg->sisRegs3C4[MMIOEnable] = inb(0x3C5) | 0x60; /* At PCI base */
    }

    outw(0x3C4, (temp << 8) | 0x05); /* Relock Registers */
    return(TRUE);
}

void
SiSRestore(ScrnInfoPtr pScrn, SISRegPtr sisReg)
{
    SISPtr pSiS = SISPTR(pScrn);
    int i;
    int vgaIOBase;
    unsigned char temp;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    outb(0x3C4, 0x05); /* Unlock Registers */
    temp = inb(0x3C5);
    outw(0x3C4, 0x8605);

    if (!pSiS->NoAccel) {
    	outw(0x3C4, (sisReg->sisRegs3C4[GraphEng] << 8) | GraphEng);
    	outw(0x3C4, (sisReg->sisRegs3C4[Mode64] << 8) | Mode64);
    	outw(0x3C4, (sisReg->sisRegs3C4[MMIOEnable] << 8) | MMIOEnable);
	if (pSiS->TurboQueue) {
    	    outw(0x3C4, (sisReg->sisRegs3C4[ExtMiscCont9] << 8) | ExtMiscCont9);
    	    outw(0x3C4, (sisReg->sisRegs3C4[TurboQueueBase] << 8) | TurboQueueBase);
	}
    }

    outw(0x3C4, (sisReg->sisRegs3C4[BankReg] << 8) | BankReg);
    outw(0x3C4, (sisReg->sisRegs3C4[LinearAdd0] << 8) | LinearAdd0);
    outw(0x3C4, (sisReg->sisRegs3C4[LinearAdd1] << 8) | LinearAdd1);
    outw(0x3C4, (sisReg->sisRegs3C4[CPUThreshold] << 8) | CPUThreshold);
    outw(0x3C4, (sisReg->sisRegs3C4[CRTThreshold] << 8) | CRTThreshold);
    outw(vgaIOBase + 4, (sisReg->sisRegs3x4[Offset] << 8) | Offset);
    outw(0x3C4, (sisReg->sisRegs3C4[CRTCOff] << 8) | CRTCOff);

    outw(0x3C4, (sisReg->sisRegs3C4[XR2A] << 8) | XR2A);
    outw(0x3C4, (sisReg->sisRegs3C4[XR2B] << 8) | XR2B);
    outw(0x3C4, (sisReg->sisRegs3C4[ClockBase] << 8) | ClockBase);
    outb(0x3C2, sisReg->sisRegs3C2[0x00]);

    outw(0x3C4, (temp << 8) | 0x05); /* Relock Registers */
}

void
SiSSave(ScrnInfoPtr pScrn, SISRegPtr sisReg)
{
    SISPtr pSiS = SISPTR(pScrn);
    int i;
    int vgaIOBase;
    unsigned char temp;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    outb(0x3C4, 0x05); /* Unlock Registers */
    temp = inb(0x3C5);
    outw(0x3C4, 0x8605);
    outb(0x3C4, BankReg);
    sisReg->sisRegs3C4[BankReg] = inb(0x3C5);
    outb(0x3C4, LinearAdd0);
    sisReg->sisRegs3C4[LinearAdd0] = inb(0x3C5);
    outb(0x3C4, LinearAdd1);
    sisReg->sisRegs3C4[LinearAdd1] = inb(0x3C5);
    outb(0x3C4, CPUThreshold);
    sisReg->sisRegs3C4[CPUThreshold] = inb(0x3C5);
    outb(0x3C4, CRTThreshold);
    sisReg->sisRegs3C4[CRTThreshold] = inb(0x3C5);
    outb(vgaIOBase + 4, Offset);
    sisReg->sisRegs3x4[Offset] = inb(0x3C5);
    outb(0x3C4, CRTCOff);
    sisReg->sisRegs3C4[CRTCOff] = inb(0x3C5);

    sisReg->sisRegs3C2[0x00] = inb(0x3CC);
    outb(0x3C4, ClockBase);
    sisReg->sisRegs3C4[ClockBase] = inb(0x3C5);
    outb(0x3C4, XR2A);
    sisReg->sisRegs3C4[XR2A] = inb(0x3C5);
    outb(0x3C4, XR2B);
    sisReg->sisRegs3C4[XR2B] = inb(0x3C5);

    if (!pSiS->NoAccel) {
    	outb(0x3C4, GraphEng);
    	sisReg->sisRegs3C4[GraphEng] = inb(0x3C5);
	outb(0x3C4, Mode64);
    	sisReg->sisRegs3C4[Mode64] = inb(0x3C5);
	outb(0x3C4, MMIOEnable);
    	sisReg->sisRegs3C4[MMIOEnable] = inb(0x3C5);
	if (pSiS->TurboQueue) {
    	    outb(0x3C4, ExtMiscCont9);
    	    sisReg->sisRegs3C4[ExtMiscCont9] = inb(0x3C5);
    	    outb(0x3C4, TurboQueueBase);
    	    sisReg->sisRegs3C4[TurboQueueBase] = inb(0x3C5);
	}
    }

    outw(0x3C4, (temp << 8) | 0x05); /* Relock Registers */
}

#if 0
static void 
TridentShowCursor(ScrnInfoPtr pScrn) 
{
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    /* 64x64 */
    outw(vgaIOBase + 4, 0xC150);
}

static void 
TridentHideCursor(ScrnInfoPtr pScrn) {
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    outw(vgaIOBase + 4, 0x4150);
}

static void 
TridentSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    if (x < 0) {
    	outw(vgaIOBase + 4, (-x)<<8 | 0x46);
	x = 0;
    } else
    	outw(vgaIOBase + 4, 0x0046);
 
    if (y < 0) {
    	outw(vgaIOBase + 4, (-y)<<8 | 0x47);
	y = 0;
    } else
    	outw(vgaIOBase + 4, 0x0047);

    outw(vgaIOBase + 4, (x&0xFF)<<8 | 0x40);
    outw(vgaIOBase + 4, (x&0xFF00)  | 0x41);
    outw(vgaIOBase + 4, (y&0xFF)<<8 | 0x42);
    outw(vgaIOBase + 4, (y&0xFF00)  | 0x43);
}

static void
TridentSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    outw(vgaIOBase + 4, (fg & 0x000000FF)<<8  | 0x48);
    outw(vgaIOBase + 4, (fg & 0x0000FF00)     | 0x49);
    outw(vgaIOBase + 4, (fg & 0x00FF0000)>>8  | 0x4A);
    outw(vgaIOBase + 4, (fg & 0xFF000000)>>16 | 0x4B);
    outw(vgaIOBase + 4, (bg & 0x000000FF)<<8  | 0x4C);
    outw(vgaIOBase + 4, (bg & 0x0000FF00)     | 0x4D);
    outw(vgaIOBase + 4, (bg & 0x00FF0000)>>8  | 0x4E);
    outw(vgaIOBase + 4, (bg & 0xFF000000)>>16 | 0x4F);
}

static void
TridentLoadCursorImage(
    ScrnInfoPtr pScrn, 
    unsigned char *src
)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    memcpy((unsigned char *)pTrident->FbBase + (pScrn->videoRam * 1024) - 4096,
			src, pTrident->CursorInfoRec->MaxWidth * 
			pTrident->CursorInfoRec->MaxHeight / 4);

    outw(vgaIOBase + 4, (((pScrn->videoRam-4) & 0xFF) << 8) | 0x44);
    outw(vgaIOBase + 4, ((pScrn->videoRam-4) & 0xFF00) | 0x45);
}

static Bool 
TridentUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    
    if (pTrident->MUX && pScrn->bitsPerPixel == 8) return FALSE;

    return TRUE;
}

Bool 
TridentHWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    xf86CursorInfoPtr infoPtr;
    int memory = pScrn->displayWidth * pScrn->virtualY * pScrn->bitsPerPixel/8;

    if (memory > (pScrn->videoRam * 1024 - 4096)) return FALSE;
    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) return FALSE;

    pTrident->CursorInfoRec = infoPtr;

    infoPtr->MaxWidth = 64;
    infoPtr->MaxHeight = 64;
    infoPtr->Flags = HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
		HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK |
		HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_32;
    infoPtr->SetCursorColors = TridentSetCursorColors;
    infoPtr->SetCursorPosition = TridentSetCursorPosition;
    infoPtr->LoadCursorImage = TridentLoadCursorImage;
    infoPtr->HideCursor = TridentHideCursor;
    infoPtr->ShowCursor = TridentShowCursor;
    infoPtr->UseHWCursor = TridentUseHWCursor;

    return(xf86InitCursor(pScreen, infoPtr));
}

unsigned int
Tridentddc1Read(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    unsigned char temp;

    /* New mode */
    outb(0x3C4, 0x0B); inb(0x3C5);

    outb(vgaIOBase + 4, NewMode1);
    temp = inb(vgaIOBase + 5);
    outb(vgaIOBase + 5, temp | 0x80);

    /* Define SDA as input */
    outw(vgaIOBase + 4, (0x04 << 8) | I2C);

    outw(vgaIOBase + 4, (temp << 8) | NewMode1);

    /* Wait until vertical retrace is in progress. */
    while (inb(vgaIOBase + 0xA) & 0x08);
    while (!(inb(vgaIOBase + 0xA) & 0x08));

    /* Get the result */
    outb(vgaIOBase + 4, I2C);
    return ( inb(vgaIOBase + 5) & 0x01 );
}
#endif

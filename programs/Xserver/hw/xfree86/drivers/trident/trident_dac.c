/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident_dac.c,v 1.4 1999/01/03 03:58:38 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Version.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "vgaHW.h"

#include "trident.h"
#include "trident_regs.h"

Bool
TridentInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    TRIDENTRegPtr pReg = &pTrident->ModeReg;
    int vgaIOBase;
    int offset;
    int clock = mode->Clock;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    pReg->tridentRegs3x4[PixelBusReg] = 0x00;

    switch (pTrident->Chipset) {
	case PROVIDIA9682:
	    pReg->tridentRegs3x4[PixelBusReg] |= 0x40; /* Late Sync */
	    break;
    }

    switch (pScrn->bitsPerPixel) {
	case 1:
	case 4:
	    outb(0x3CE, MiscExtFunc);
	    pReg->tridentRegs3CE[MiscExtFunc] = (inb(0x3CF) & 0xB7) | 0x04;
    	    offset = pScrn->displayWidth >> 3;
    	    pReg->tridentRegsDAC[0x00] = 0x00;
	    break;
	case 8:
	    outb(0x3CE, MiscExtFunc);
	    pReg->tridentRegs3CE[MiscExtFunc] = (inb(0x3CF) & 0xB7) | 0x02;
    	    offset = pScrn->displayWidth >> 3;
    	    pReg->tridentRegsDAC[0x00] = 0x00;
	    break;
	case 16:
	    outb(0x3CE, MiscExtFunc);
	    pReg->tridentRegs3CE[MiscExtFunc] = (inb(0x3CF) & 0xB7) | 0x02;
    	    offset = pScrn->displayWidth >> 2;
	    if (pScrn->depth == 15)
    	    	pReg->tridentRegsDAC[0x00] = 0x10;
	    else
	    	pReg->tridentRegsDAC[0x00] = 0x30;
    	    pReg->tridentRegs3x4[PixelBusReg] |= 0x04;
	    if (pTrident->Chipset > CYBER9320) 
		pReg->tridentRegs3x4[PixelBusReg] |= 0x01;
	    break;
	case 24:
	    outb(0x3CE, MiscExtFunc);
	    pReg->tridentRegs3CE[MiscExtFunc] = (inb(0x3CF) & 0xB7) | 0x02;
#if 0
    	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x08; /* Clock Division by 2*/
	    clock *= 2;	/* Double the clock */
#endif
    	    offset = (pScrn->displayWidth * 3) >> 3;
    	    pReg->tridentRegs3x4[PixelBusReg] |= 0x29;
	    pReg->tridentRegsDAC[0x00] = 0xD0;
	    break;
	case 32:
	    outb(0x3CE, MiscExtFunc);
	    pReg->tridentRegs3CE[MiscExtFunc] = (inb(0x3CF) & 0xB7) | 0x02;
    	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x08; /* Clock Division by 2*/
	    clock *= 2;	/* Double the clock */
    	    offset = pScrn->displayWidth >> 1;
    	    pReg->tridentRegs3x4[PixelBusReg] |= 0x09;
	    pReg->tridentRegsDAC[0x00] = 0xD0;
	    break;
    }
    pReg->tridentRegs3x4[Offset] = offset & 0xFF;

    {
	unsigned char a, b;

	TGUISetClock(pScrn, clock, &a, &b);
	pReg->tridentRegsClock[0x00] = (inb(0x3CC) & 0xF3) | 0x08;
	pReg->tridentRegsClock[0x01] = a;
	pReg->tridentRegsClock[0x02] = b;
    }

    pReg->tridentRegs3C4[NewMode1] = 0xC0;

    pReg->tridentRegs3x4[LinearAddReg] = ((pTrident->FbAddress >> 24) << 6)  |
					 ((pTrident->FbAddress >> 20) & 0x0F)|
					 0x20;
    pReg->tridentRegs3x4[CRTHiOrd] = (((mode->CrtcVBlankStart-1)& 0x400) >> 4) |
 			(((mode->CrtcVTotal - 2) & 0x400) >> 3) |
 			((mode->CrtcVSyncStart & 0x400) >> 5) |
 			(((mode->CrtcVDisplay - 1) & 0x400) >> 6) |
 			0x08;
    pReg->tridentRegs3x4[CRTCModuleTest] = (mode->Flags & V_INTERLACE ? 0x84 : 0x80);
    outb(vgaIOBase+ 4, InterfaceSel);
    pReg->tridentRegs3x4[InterfaceSel] = inb(vgaIOBase + 5) | 0x40;
    outb(vgaIOBase+ 4, Performance);
    pReg->tridentRegs3x4[Performance] = inb(vgaIOBase + 5) | 0x10;
    outb(vgaIOBase+ 4, DRAMControl);
    pReg->tridentRegs3x4[DRAMControl] = inb(vgaIOBase + 5) | 0x10;
    outb(vgaIOBase+ 4, AddColReg);
    pReg->tridentRegs3x4[AddColReg] = (inb(vgaIOBase + 5) & 0xCF) |
				      ((offset & 0x300) >> 4);
    if (pTrident->NoMMIO)
	pReg->tridentRegs3x4[GraphEngReg] = 0x80; 
    else
	pReg->tridentRegs3x4[GraphEngReg] = 0x82; 

    outb(vgaIOBase+ 4, PCIReg);
    pReg->tridentRegs3x4[PCIReg] = inb(vgaIOBase + 5) & 0xF9; 
    /* Enable PCI Bursting on capable chips */
    if (pTrident->Chipset > CYBER9320) pReg->tridentRegs3x4[PCIReg] |= 0x06;

    outb(0x3CE, MiscIntContReg);
    pReg->tridentRegs3CE[MiscIntContReg] = inb(0x3CF) | 0x04;

    if (pTrident->UsePCIRetry) {
        outb(vgaIOBase + 4, PCIRetry);
        pReg->tridentRegs3x4[PCIRetry] = inb(vgaIOBase + 5) | 0xDF;
    	outb(vgaIOBase + 4, Enhancement0);
	pReg->tridentRegs3x4[Enhancement0] = inb(vgaIOBase + 5) | 0x50;
    }

    return(TRUE);
}

void
TridentRestore(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int i;
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    /* Goto New Mode */
    outb(0x3C4, 0x0B);
    inb(0x3C5);

    /* Unprotect registers */
    outw(0x3C4, ((0xC0 ^ 0x02) << 8) | NewMode1);

    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[CRTCModuleTest] << 8) | CRTCModuleTest);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[LinearAddReg] << 8) | 
								LinearAddReg);

    outb(0x3C2, tridentReg->tridentRegsClock[0x00]);
    if (Is3Dchip) {
	outw(0x3C4, (tridentReg->tridentRegsClock[0x01])<<8 | ClockLow);
	outw(0x3C4, (tridentReg->tridentRegsClock[0x02])<<8 | ClockHigh);
    } else {
	outb(0x43C8, tridentReg->tridentRegsClock[0x01]);
	outb(0x43C9, tridentReg->tridentRegsClock[0x02]);
    }

    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[CRTHiOrd] << 8) | CRTHiOrd);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[AddColReg] << 8) | AddColReg);
    outw(0x3CE, (tridentReg->tridentRegs3CE[MiscExtFunc] << 8) | MiscExtFunc);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[GraphEngReg] << 8) | GraphEngReg);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[Performance] << 8) | Performance);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[InterfaceSel] << 8) | InterfaceSel);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[DRAMControl] << 8) | DRAMControl);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[PixelBusReg] << 8) | PixelBusReg);
    outw(0x3CE, (tridentReg->tridentRegs3CE[MiscIntContReg] << 8) | MiscIntContReg);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[PCIReg] << 8) | PCIReg);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[Offset] << 8) | Offset);

    if (pTrident->UsePCIRetry) {
        outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[PCIRetry] << 8) | PCIRetry);
    	outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[Enhancement0] << 8) | Enhancement0);
    }
 
    /* restore cursor registers */
    for (i=CursorXLow;i<=CursorControl;i++)
    	outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[i] << 8) | i);

    inb(0x3C8);
    inb(0x3C6);
    inb(0x3C6);
    inb(0x3C6);
    inb(0x3C6);
    outb(0x3C6, tridentReg->tridentRegsDAC[0x00]);
    inb(0x3C8);

    outw(0x3C4, ((tridentReg->tridentRegs3C4[NewMode1] ^ 0x02) << 8) | NewMode1);
}

void
TridentSave(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int i;
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    outb(vgaIOBase + 4, Offset);
    tridentReg->tridentRegs3x4[Offset] = inb(vgaIOBase + 5);

    /* Goto New Mode */
    outb(0x3C4, 0x0B);
    inb(0x3C5);

    outb(0x3C4, NewMode1);
    tridentReg->tridentRegs3C4[NewMode1] = inb(0x3C5);

    outb(vgaIOBase + 4, LinearAddReg);
    tridentReg->tridentRegs3x4[LinearAddReg] = inb(vgaIOBase + 5);
    outb(vgaIOBase + 4, CRTCModuleTest);
    tridentReg->tridentRegs3x4[CRTCModuleTest] = inb(vgaIOBase + 5);
    outb(vgaIOBase + 4, CRTHiOrd);
    tridentReg->tridentRegs3x4[CRTHiOrd] = inb(vgaIOBase + 5);
    outb(vgaIOBase + 4, Performance);
    tridentReg->tridentRegs3x4[Performance] = inb(vgaIOBase + 5);
    outb(vgaIOBase + 4, InterfaceSel);
    tridentReg->tridentRegs3x4[InterfaceSel] = inb(vgaIOBase + 5);
    outb(vgaIOBase + 4, DRAMControl);
    tridentReg->tridentRegs3x4[DRAMControl] = inb(vgaIOBase + 5);
    outb(vgaIOBase + 4, AddColReg);
    tridentReg->tridentRegs3x4[AddColReg] = inb(vgaIOBase + 5);
    outb(vgaIOBase + 4, PixelBusReg);
    tridentReg->tridentRegs3x4[PixelBusReg] = inb(vgaIOBase + 5);
    outb(vgaIOBase + 4, GraphEngReg);
    tridentReg->tridentRegs3x4[GraphEngReg] = inb(vgaIOBase + 5);
    outb(vgaIOBase + 4, PCIReg);
    tridentReg->tridentRegs3x4[PCIReg] = inb(vgaIOBase + 5);

    if (pTrident->UsePCIRetry) {
        outb(vgaIOBase + 4, PCIRetry);
        tridentReg->tridentRegs3x4[PCIRetry] = inb(vgaIOBase + 5);
	outb(vgaIOBase + 4, Enhancement0);
	tridentReg->tridentRegs3x4[Enhancement0] = inb(vgaIOBase + 5);
    }

    /* save cursor registers */
    for (i=CursorXLow;i<=CursorControl;i++) {
    	outb(vgaIOBase + 4, i);
    	tridentReg->tridentRegs3x4[i] = inb(vgaIOBase + 5);
    }

    outb(0x3CE, MiscExtFunc);
    tridentReg->tridentRegs3CE[MiscExtFunc] = inb(0x3CF);
    outb(0x3CE, MiscIntContReg);
    tridentReg->tridentRegs3CE[MiscIntContReg] = inb(0x3CF);

    inb(0x3C8);
    inb(0x3C6);
    inb(0x3C6);
    inb(0x3C6);
    inb(0x3C6);
    tridentReg->tridentRegsDAC[0x00] = inb(0x3C6);
    inb(0x3C8);

    tridentReg->tridentRegsClock[0x00] = inb(0x3CC);
    if (Is3Dchip) {
	outb(0x3C4, ClockLow);
	tridentReg->tridentRegsClock[0x01] = inb(0x3C5);
	outb(0x3C4, ClockHigh);
	tridentReg->tridentRegsClock[0x02] = inb(0x3C5);
    } else {
	tridentReg->tridentRegsClock[0x01] = inb(0x43C8);
	tridentReg->tridentRegsClock[0x02] = inb(0x43C9);
    }
}

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
TridentUseHWCursor(ScreenPtr pScr, CursorPtr pCurs)
{
    return TRUE;
}

Bool 
TridentHWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    xf86CursorInfoPtr infoPtr;

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

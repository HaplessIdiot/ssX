/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident_dac.c,v 1.7 1999/04/15 06:39:02 dawes Exp $ */

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
    int offset = 0;
    int clock = mode->Clock;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    pReg->tridentRegs3x4[PixelBusReg] = 0x00;
    pReg->tridentRegsDAC[0x00] = 0x00;
    MMIO_OUTB(vgaIOBase + 4, NewMode2);
    pReg->tridentRegs3C4[NewMode2] = 0x20;
    MMIO_OUTB(0x3CE, MiscExtFunc);
    pReg->tridentRegs3CE[MiscExtFunc] = MMIO_INB(0x3CF);

    /* Enable Chipset specific options */
    switch (pTrident->Chipset) {
	case BLADE3D:
	case CYBER9520:
	case CYBER9397:
	case CYBER939A:
	case CYBER9525:
	case IMAGE975:
	case IMAGE985:
    	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x10;
	    /* Fall Through */
	case PROVIDIA9685:
	    if (pTrident->UsePCIRetry) {
		pTrident->UseGERetry = TRUE;
		MMIO_OUTB(vgaIOBase + 4, Enhancement0);
	    	pReg->tridentRegs3x4[Enhancement0] = 0x50;
	    } else {
		pTrident->UseGERetry = FALSE;
	    }
	    /* Fall Through */
	case TGUI9660:
	case TGUI9680:
	case PROVIDIA9682:
	    if (pTrident->MUX && pScrn->bitsPerPixel == 8 && mode->CrtcHAdjusted) {
	    	pReg->tridentRegs3x4[PixelBusReg] |= 0x01; /* 16bit bus */
	    	pReg->tridentRegs3C4[NewMode2] |= 0x02; /* half clock */
    		pReg->tridentRegsDAC[0x00] |= 0x20;	/* mux mode */
	    }	
	    break;
    }

    /* Defaults for all trident chipsets follows */
    switch (pScrn->bitsPerPixel) {
	case 1:
	case 4:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x04;
    	    offset = pScrn->displayWidth >> 3;
	    break;
	case 8:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    offset = pScrn->displayWidth >> 3;
	    break;
	case 16:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    offset = pScrn->displayWidth >> 2;
	    if (pScrn->depth == 15)
    	    	pReg->tridentRegsDAC[0x00] = 0x10;
	    else
	    	pReg->tridentRegsDAC[0x00] = 0x30;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x04;
	    if (pTrident->Chipset > CYBER9320) 
		pReg->tridentRegs3x4[PixelBusReg] |= 0x01;
	    break;
	case 24:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    offset = (pScrn->displayWidth * 3) >> 3;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x29;
	    pReg->tridentRegsDAC[0x00] = 0xD0;
	    break;
	case 32:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x08; /* Clock Division by 2*/
	    clock *= 2;	/* Double the clock */
    	    offset = pScrn->displayWidth >> 1;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x09;
	    pReg->tridentRegsDAC[0x00] = 0xD0;
	    break;
    }
    pReg->tridentRegs3x4[Offset] = offset & 0xFF;

    {
	unsigned char a, b;

	TGUISetClock(pScrn, clock, &a, &b);
	pReg->tridentRegsClock[0x00] = (MMIO_INB(0x3CC) & 0xF3) | 0x08;
	pReg->tridentRegsClock[0x01] = a;
	pReg->tridentRegsClock[0x02] = b;
	if (pTrident->MCLK > 0) {
	    TGUISetMCLK(pScrn, pTrident->MCLK, &a, &b);
	    pReg->tridentRegsClock[0x03] = a;
	    pReg->tridentRegsClock[0x04] = b;
	}
    }

    pReg->tridentRegs3C4[NewMode1] = 0xC0;

    pReg->tridentRegs3x4[LinearAddReg] = ((pTrident->FbAddress >> 24) << 6)  |
					 ((pTrident->FbAddress >> 20) & 0x0F)|
					 0x20;
    pReg->tridentRegs3x4[CRTHiOrd] = (((mode->CrtcVBlankEnd-1) & 0x400) >> 4) |
 			(((mode->CrtcVTotal - 2) & 0x400) >> 3) |
 			((mode->CrtcVSyncStart & 0x400) >> 5) |
 			(((mode->CrtcVDisplay - 1) & 0x400) >> 6) |
 			0x08;
    pReg->tridentRegs3x4[CRTCModuleTest] = (mode->Flags & V_INTERLACE ? 0x84 : 0x80);
    MMIO_OUTB(vgaIOBase+ 4, InterfaceSel);
    pReg->tridentRegs3x4[InterfaceSel] = MMIO_INB(vgaIOBase + 5) | 0x40;
    MMIO_OUTB(vgaIOBase+ 4, Performance);
    pReg->tridentRegs3x4[Performance] = MMIO_INB(vgaIOBase + 5) | 0x10;
    MMIO_OUTB(vgaIOBase+ 4, DRAMControl);
    pReg->tridentRegs3x4[DRAMControl] = MMIO_INB(vgaIOBase + 5) | 0x10;
    MMIO_OUTB(vgaIOBase+ 4, AddColReg);
    pReg->tridentRegs3x4[AddColReg] = (MMIO_INB(vgaIOBase + 5) & 0xCF) |
				      ((offset & 0x300) >> 4);
   
    pReg->tridentRegs3x4[GraphEngReg] = 0x80; 

    MMIO_OUTB(0x3CE, MiscIntContReg);
    pReg->tridentRegs3CE[MiscIntContReg] = MMIO_INB(0x3CF) | 0x04;

    MMIO_OUTB(vgaIOBase+ 4, PCIReg);
    pReg->tridentRegs3x4[PCIReg] = MMIO_INB(vgaIOBase + 5) & 0xF9; 
    /* Enable PCI Bursting on capable chips */
    if (pTrident->Chipset > CYBER9320) pReg->tridentRegs3x4[PCIReg] |= 0x06;

    return(TRUE);
}

void
TridentRestore(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int i;
    unsigned char temp;
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    /* Goto New Mode */
    MMIO_OUTB(0x3C4, 0x0B);
    temp = MMIO_INB(0x3C5);

    /* Unprotect registers */
    MMIO_OUTW(0x3C4, ((0xC0 ^ 0x02) << 8) | NewMode1);

    /* Select 25MHz clock, program the programmable, and set it */
    MMIO_OUTB(0x3C2, MMIO_INB(0x3CC) & 0xF3);

    MMIO_OUTW_3x4(CursorControl);
    MMIO_OUTW_3x4(CRTCModuleTest);
    MMIO_OUTW_3x4(LinearAddReg);
    MMIO_OUTW_3C4(NewMode2);
    MMIO_OUTW_3x4(CRTHiOrd);
    MMIO_OUTW_3x4(AddColReg);
    MMIO_OUTW_3CE(MiscExtFunc);
    MMIO_OUTW_3x4(GraphEngReg);
    MMIO_OUTW_3x4(Performance);
    MMIO_OUTW_3x4(InterfaceSel);
    MMIO_OUTW_3x4(DRAMControl);
    MMIO_OUTW_3x4(PixelBusReg);
    MMIO_OUTW_3CE(MiscIntContReg);
    MMIO_OUTW_3x4(PCIReg);
    MMIO_OUTW_3x4(Offset);

    if (pTrident->UsePCIRetry)
        MMIO_OUTW_3x4(Enhancement0);
 
    if (Is3Dchip) {
	MMIO_OUTW(0x3C4, (tridentReg->tridentRegsClock[0x01])<<8 | ClockLow);
	MMIO_OUTW(0x3C4, (tridentReg->tridentRegsClock[0x02])<<8 | ClockHigh);
	if (pTrident->MCLK > 0) {
	    MMIO_OUTW(0x3C4,(tridentReg->tridentRegsClock[0x03])<<8 | MCLKLow);
	    MMIO_OUTW(0x3C4,(tridentReg->tridentRegsClock[0x04])<<8 | MCLKHigh);
	}
    } else {
	MMIO_OUTB(0x43C8, tridentReg->tridentRegsClock[0x01]);
	MMIO_OUTB(0x43C9, tridentReg->tridentRegsClock[0x02]);
	if (pTrident->MCLK > 0) {
	    MMIO_OUTB(0x43C6, tridentReg->tridentRegsClock[0x03]);
	    MMIO_OUTB(0x43C7, tridentReg->tridentRegsClock[0x04]);
	}
    }
    MMIO_OUTB(0x3C2, tridentReg->tridentRegsClock[0x00]);

    temp = MMIO_INB(0x3C8);
    temp = MMIO_INB(0x3C6);
    temp = MMIO_INB(0x3C6);
    temp = MMIO_INB(0x3C6);
    temp = MMIO_INB(0x3C6);
    MMIO_OUTB(0x3C6, tridentReg->tridentRegsDAC[0x00]);
    temp = MMIO_INB(0x3C8);

    MMIO_OUTW(0x3C4, ((tridentReg->tridentRegs3C4[NewMode1] ^ 0x02) << 8) | NewMode1);
}

void
TridentSave(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int i;
    unsigned char temp;
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    MMIO_OUTB(vgaIOBase + 4, Offset);
    tridentReg->tridentRegs3x4[Offset] = MMIO_INB(vgaIOBase + 5);

    /* Goto New Mode */
    MMIO_OUTB(0x3C4, 0x0B);
    temp = MMIO_INB(0x3C5);

    MMIO_INB_3C4(NewMode1);

    /* Unprotect registers */
    MMIO_OUTW(0x3C4, ((0xC0 ^ 0x02) << 8) | NewMode1);

    MMIO_INB_3x4(LinearAddReg);
    MMIO_INB_3x4(CRTCModuleTest);
    MMIO_INB_3x4(CRTHiOrd);
    MMIO_INB_3x4(Performance);
    MMIO_INB_3x4(InterfaceSel);
    MMIO_INB_3x4(DRAMControl);
    MMIO_INB_3x4(AddColReg);
    MMIO_INB_3x4(PixelBusReg);
    MMIO_INB_3x4(GraphEngReg);
    MMIO_INB_3x4(PCIReg);

    if (pTrident->UsePCIRetry) {
        MMIO_INB_3x4(Enhancement0);
    }

    /* save cursor registers */
    MMIO_INB_3x4(CursorControl);

    MMIO_INB_3CE(MiscExtFunc);
    MMIO_INB_3CE(MiscIntContReg);

    temp = MMIO_INB(0x3C8);
    temp = MMIO_INB(0x3C6);
    temp = MMIO_INB(0x3C6);
    temp = MMIO_INB(0x3C6);
    temp = MMIO_INB(0x3C6);
    tridentReg->tridentRegsDAC[0x00] = MMIO_INB(0x3C6);
    temp = MMIO_INB(0x3C8);

    tridentReg->tridentRegsClock[0x00] = MMIO_INB(0x3CC);
    if (Is3Dchip) {
	MMIO_OUTB(0x3C4, ClockLow);
	tridentReg->tridentRegsClock[0x01] = MMIO_INB(0x3C5);
	MMIO_OUTB(0x3C4, ClockHigh);
	tridentReg->tridentRegsClock[0x02] = MMIO_INB(0x3C5);
	if (pTrident->MCLK > 0) {
	    MMIO_OUTB(0x3C4, MCLKLow);
	    tridentReg->tridentRegsClock[0x03] = MMIO_INB(0x3C5);
	    MMIO_OUTB(0x3C4, MCLKHigh);
	    tridentReg->tridentRegsClock[0x04] = MMIO_INB(0x3C5);
	}
    } else {
	tridentReg->tridentRegsClock[0x01] = MMIO_INB(0x43C8);
	tridentReg->tridentRegsClock[0x02] = MMIO_INB(0x43C9);
	if (pTrident->MCLK > 0) {
	    tridentReg->tridentRegsClock[0x03] = MMIO_INB(0x43C6);
	    tridentReg->tridentRegsClock[0x04] = MMIO_INB(0x43C7);
	}
    }
    MMIO_INB_3C4(NewMode2);

#if 0
for (i=0;i<0xffff;i++)
ErrorF("0x%x 0x%x\n",i,MMIO_INB(i));
#endif

    /* Protect registers */
    MMIO_OUTW_3C4(NewMode1);
}

static void 
TridentShowCursor(ScrnInfoPtr pScrn) 
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    /* 64x64 */
    MMIO_OUTW(vgaIOBase + 4, 0xC150);
}

static void 
TridentHideCursor(ScrnInfoPtr pScrn) {
    int vgaIOBase;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    MMIO_OUTW(vgaIOBase + 4, 0x4150);
}

static void 
TridentSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    int vgaIOBase;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    if (x < 0) {
    	MMIO_OUTW(vgaIOBase + 4, (-x)<<8 | 0x46);
	x = 0;
    } else
    	MMIO_OUTW(vgaIOBase + 4, 0x0046);
 
    if (y < 0) {
    	MMIO_OUTW(vgaIOBase + 4, (-y)<<8 | 0x47);
	y = 0;
    } else
    	MMIO_OUTW(vgaIOBase + 4, 0x0047);

    MMIO_OUTW(vgaIOBase + 4, (x&0xFF)<<8 | 0x40);
    MMIO_OUTW(vgaIOBase + 4, (x&0x0F00)  | 0x41);
    MMIO_OUTW(vgaIOBase + 4, (y&0xFF)<<8 | 0x42);
    MMIO_OUTW(vgaIOBase + 4, (y&0x0F00)  | 0x43);
}

static void
TridentSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    int vgaIOBase;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    MMIO_OUTW(vgaIOBase + 4, (fg & 0x000000FF)<<8  | 0x48);
    MMIO_OUTW(vgaIOBase + 4, (fg & 0x0000FF00)     | 0x49);
    MMIO_OUTW(vgaIOBase + 4, (fg & 0x00FF0000)>>8  | 0x4A);
    MMIO_OUTW(vgaIOBase + 4, (fg & 0xFF000000)>>16 | 0x4B);
    MMIO_OUTW(vgaIOBase + 4, (bg & 0x000000FF)<<8  | 0x4C);
    MMIO_OUTW(vgaIOBase + 4, (bg & 0x0000FF00)     | 0x4D);
    MMIO_OUTW(vgaIOBase + 4, (bg & 0x00FF0000)>>8  | 0x4E);
    MMIO_OUTW(vgaIOBase + 4, (bg & 0xFF000000)>>16 | 0x4F);
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

    MMIO_OUTW(vgaIOBase + 4, (((pScrn->videoRam-4) & 0xFF) << 8) | 0x44);
    MMIO_OUTW(vgaIOBase + 4, ((pScrn->videoRam-4) & 0xFF00) | 0x45);
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
    MMIO_OUTB(0x3C4, 0x0B); temp = MMIO_INB(0x3C5);

    MMIO_OUTB(vgaIOBase + 4, NewMode1);
    temp = MMIO_INB(vgaIOBase + 5);
    MMIO_OUTB(vgaIOBase + 5, temp | 0x80);

    /* Define SDA as input */
    MMIO_OUTW(vgaIOBase + 4, (0x04 << 8) | I2C);

    MMIO_OUTW(vgaIOBase + 4, (temp << 8) | NewMode1);

    /* Wait until vertical retrace is in progress. */
    while (MMIO_INB(vgaIOBase + 0xA) & 0x08);
    while (!(MMIO_INB(vgaIOBase + 0xA) & 0x08));

    /* Get the result */
    MMIO_OUTB(vgaIOBase + 4, I2C);
    return ( MMIO_INB(vgaIOBase + 5) & 0x01 );
}

void TridentLoadPalette(
    ScrnInfoPtr pScrn, 
    int numColors, 
    int *indicies,
    LOCO *colors,
    short visualClass
){
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int i, index;

    for(i = 0; i < numColors; i++) {
	index = indicies[i];
    	MMIO_OUTB(0x3C6, 0xFF);
        MMIO_OUTB(0x3c8, index);
        MMIO_OUTB(0x3c9, colors[index].red);
        MMIO_OUTB(0x3c9, colors[index].green);
        MMIO_OUTB(0x3c9, colors[index].blue);
    }
}


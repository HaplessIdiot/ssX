/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident_dac.c,v 1.1 1998/09/06 13:47:59 dawes Exp $ */

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
    int shift;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    shift = (mode->Flags & V_INTERLACE ? 3 : 4);

    outb(0x3CE, MiscExtFunc);
    pReg->tridentRegs3CE[MiscExtFunc] = (inb(0x3CF) & 0xB7) | 0x02;

    switch (pScrn->bitsPerPixel) {
	case 8:
    	    offset = pScrn->displayWidth >> shift;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x00;
    	    pReg->tridentRegsDAC[0x00] = 0x00;
	    break;
	case 16:
    	    offset = pScrn->displayWidth >> (shift - 1);
	    if (pScrn->depth == 15) {
    	    	pReg->tridentRegs3x4[PixelBusReg] = 0x05;
    	    	pReg->tridentRegsDAC[0x00] = 0x10;
	    } else {
    	    	pReg->tridentRegs3x4[PixelBusReg] = 0x05;
	    	pReg->tridentRegsDAC[0x00] = 0x30;
	    }
	    break;
	case 24:
    	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x08; /* Clock Division by 2*/
	    clock *= 2;	/* Double the clock */
    	    offset = (pScrn->displayWidth * 3) >> shift;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x29;
	    pReg->tridentRegsDAC[0x00] = 0xD0;
	    break;
	case 32:
    	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x08; /* Clock Division by 2*/
	    clock *= 2;	/* Double the clock */
    	    offset = pScrn->displayWidth >> (shift - 2);
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x09;
	    pReg->tridentRegsDAC[0x00] = 0xD0;
	    break;
    }

    {
	unsigned char a, b;

	TGUISetClock(pScrn, clock, &a, &b);
	pReg->tridentRegsClock[0x00] = (inb(0x3CC) & 0xF3) | 0x08;
	pReg->tridentRegsClock[0x01] = a;
	pReg->tridentRegsClock[0x02] = b;
    }

    pReg->tridentRegs3C4[NewMode1] = 0xC2;

    pReg->tridentRegs3x4[LinearAddReg] = ((pTrident->FbAddress >> 24) << 6)  |
					 ((pTrident->FbAddress >> 20) & 0x0F)|
					 0x20;
    pReg->tridentRegs3x4[CRTHiOrd] = ((mode->CrtcVSyncStart & 0x400) >> 4) |
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
    pReg->tridentRegs3x4[AddColReg] = inb(vgaIOBase + 5) | ((offset & 0x300) >> 4);
    pReg->tridentRegs3x4[GraphEngReg] = 0x80; 
    outb(vgaIOBase+ 4, PCIReg);
    pReg->tridentRegs3x4[PCIReg] = inb(vgaIOBase + 5) | 0x06; 

    outb(0x3CE, MiscIntContReg);
    pReg->tridentRegs3CE[MiscIntContReg] = inb(0x3CF) | 0x04;

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
    outw(0x3C4, 0xC200 | NewMode1);

    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[CRTCModuleTest] << 8) | CRTCModuleTest);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[LinearAddReg] << 8) | 
								LinearAddReg);

    inb(0x3C8);
    inb(0x3C6);
    inb(0x3C6);
    inb(0x3C6);
    inb(0x3C6);
    outb(0x3C6, tridentReg->tridentRegsDAC[0x00]);
    inb(0x3C8);

    outb(0x3C2, tridentReg->tridentRegsClock[0x00]);
    outb(0x43C8, tridentReg->tridentRegsClock[0x01]);
    outb(0x43C9, tridentReg->tridentRegsClock[0x02]);


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

    outw(0x3C4, (tridentReg->tridentRegs3C4[NewMode1] << 8) | NewMode1);
}

void
TridentSave(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

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
    tridentReg->tridentRegsClock[0x01] = inb(0x43C8);
    tridentReg->tridentRegsClock[0x02] = inb(0x43C9);
}

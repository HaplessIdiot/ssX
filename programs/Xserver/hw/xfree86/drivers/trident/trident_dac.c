/* $XFree86$ */

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
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    IsClearTV(pScrn);

    pTrident->NewClockCode = FALSE;

    {
	unsigned char a, b;

	TGUISetClock(pScrn, mode->Clock, &a, &b);
	pReg->tridentRegsClock[0x00] = (inb(0x3CC) & 0xF3) | 0x08;
	pReg->tridentRegsClock[0x01] = a;
	pReg->tridentRegsClock[0x02] = b;
    }

    pReg->tridentRegs3C4[NewMode1] = 0xC2;

    pReg->tridentRegs3x4[LinearAddReg] = ((pTrident->FbAddress >> 24) << 6)  |
					 ((pTrident->FbAddress >> 20) & 0x0F)|
					 0x20;
    outb(vgaIOBase+ 4, VLBusReg);
    pReg->tridentRegs3x4[VLBusReg] = inb(vgaIOBase + 5) | 0x40;

    outb(0x3CE, MiscExtFunc);
    pReg->tridentRegs3CE[MiscExtFunc] = (inb(0x3CF) & 0xB7) | 0x07;

    return(TRUE);
}

void
TridentRestore(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    /* Goto New Mode */
    outb(0x3C4, 0x0B);
    inb(0x3C5);

    outw(0x3C4, (tridentReg->tridentRegs3C4[NewMode1] << 8) | NewMode1);

    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[LinearAddReg] << 8) | 
								LinearAddReg);
    outw(vgaIOBase + 4, (tridentReg->tridentRegs3x4[VLBusReg] << 8) | VLBusReg);

    outw(0x3CE, (tridentReg->tridentRegs3CE[MiscExtFunc] << 8) | MiscExtFunc);

    outb(0x3C2, tridentReg->tridentRegsClock[0x00]);
    outb(0x43C8, tridentReg->tridentRegsClock[0x01]);
    outb(0x43C9, tridentReg->tridentRegsClock[0x02]);
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
    outb(vgaIOBase + 4, VLBusReg);
    tridentReg->tridentRegs3x4[VLBusReg] = inb(vgaIOBase + 5);

    outb(0x3CE, MiscExtFunc);
    tridentReg->tridentRegs3CE[MiscExtFunc] = inb(0x3CF);

    tridentReg->tridentRegsClock[0x00] = inb(0x3CC);
    tridentReg->tridentRegsClock[0x01] = inb(0x43C8);
    tridentReg->tridentRegsClock[0x02] = inb(0x43C9);
}

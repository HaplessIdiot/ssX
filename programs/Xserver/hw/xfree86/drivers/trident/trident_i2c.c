/* $XFree86$ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "trident.h"
#include "trident_regs.h"

/*
 * Switch between internal I2C bus and external (DDC) bus.
 * There is one I2C port controlled by SR08 and the programmable
 * outputs control a multiplexer.
 */
static Bool
TRIDENTI2CSwitchToBus(I2CBusPtr b) {
    TRIDENTPtr pTrident = ((TRIDENTPtr)b->DriverPrivate.ptr);
    int vgaIOBase;
    vgaIOBase = 0x3D0; /* VGAHWPTR(pScrn)->IOBase; */
    if (b == pTrident->I2CPtr1) {
        if ((pTrident->ModeReg.tridentRegs3x4[I2C] & 0x04) == 0)
		return TRUE;
	pTrident->ModeReg.tridentRegs3x4[I2C] &= ~0x04;
    }
    else if(b == pTrident->I2CPtr2) {
        if ((pTrident->ModeReg.tridentRegs3x4[I2C] & 0x04) != 0)
		return TRUE;
        pTrident->ModeReg.tridentRegs3x4[I2C] |= 0x04;
    }
    else return FALSE;

    ErrorF("TRIDENTI2CSwitchToBus: \"%s\"\n", b->BusName);
    outw(vgaIOBase + 4, (pTrident->ModeReg.tridentRegs3x4[I2C] << 8) | I2C);
    return TRUE;
}

static Bool
TRIDENTI2CPutBits(I2CBusPtr b, int clock,  int data) {
    unsigned int reg = 4;
    int vgaIOBase;
    vgaIOBase = 0x3D0; /* VGAHWPTR(pScrn)->IOBase; */
    
#if 0
    if(!TRIDENTI2CSwitchToBus(b))
        return FALSE;
#endif

    if(clock) reg |= 2;
    if(data)  reg |= 1;
    outw(vgaIOBase + 4, (reg << 8) | 0x08);
    /* ErrorF("TRIDENTI2CPutBits: %d %d\n", clock, data); */
    return TRUE;
}

static Bool
TRIDENTI2CGetBits(I2CBusPtr b, int *clock, int *data) {
    unsigned int reg;
    int vgaIOBase;
    vgaIOBase = 0x3D0; /* VGAHWPTR(pScrn)->IOBase; */

#if 0
    if(!TRIDENTI2CSwitchToBus(b))
        return FALSE;
#endif

    outb(vgaIOBase + 4, I2C);
    reg = inb(vgaIOBase + 5);
    *clock = (reg & 0x02) != 0;
    *data  = (reg & 0x01) != 0;
    /* ErrorF("TRIDENTI2CGetBits: %d %d\n", *clock, *data); */
    return TRUE;
}

Bool 
TRIDENTI2CInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    I2CBusPtr I2CPtr;

    ErrorF("TRIDENTI2CInit\n");

    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    pTrident->I2CPtr1 = I2CPtr;

    I2CPtr->BusName    = "I2C bus 1";
    I2CPtr->I2CPutBits = TRIDENTI2CPutBits;
    I2CPtr->I2CGetBits = TRIDENTI2CGetBits;
    I2CPtr->DriverPrivate.ptr = pTrident;
    
    if(!xf86I2CBusInit(I2CPtr))
       return FALSE;

    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    pTrident->I2CPtr2 = I2CPtr;

    I2CPtr->BusName    = "I2C bus 2";
    I2CPtr->I2CPutBits = TRIDENTI2CPutBits;
    I2CPtr->I2CGetBits = TRIDENTI2CGetBits;
    I2CPtr->DriverPrivate.ptr = pTrident;
    
    if(!xf86I2CBusInit(I2CPtr))
       return FALSE;

    return TRUE;
}

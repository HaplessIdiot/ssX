/* (c) Itai Nahshon */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir_i2c.c,v 1.5 1998/12/06 06:08:30 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "cir.h"

/*
 * Switch between internal I2C bus and external (DDC) bus.
 * There is one I2C port controlled bu SR08 and the programmable
 * outputs control a multiplexer.
 */
static Bool
CIRI2CSwitchToBus(I2CBusPtr b) {
    CIRPtr pCir = ((CIRPtr)b->DriverPrivate.ptr);
    vgaHWPtr hwp = VGAHWPTR(pCir->pScrn);
    if (b == pCir->I2CPtr1) {
        if ((pCir->ModeReg.ExtVga[GR17] & 0x60) == 0)
		return TRUE;
	pCir->ModeReg.ExtVga[GR17] &= ~0x60;
    }
    else if(b == pCir->I2CPtr2) {
        if ((pCir->ModeReg.ExtVga[GR17] & 0x60) != 0)
		return TRUE;
        pCir->ModeReg.ExtVga[GR17] |= 0x60;
    }
    else return FALSE;

    /* ErrorF("CIRI2CSwitchToBus: \"%s\"\n", b->BusName); */
    hwp->writeGr(hwp, 0x17, pCir->ModeReg.ExtVga[GR17]);
    return TRUE;
}

static void
CIRI2CPutBits(I2CBusPtr b, int clock,  int data) {
    unsigned int reg = 0xfc;
    CIRPtr pCir = ((CIRPtr)b->DriverPrivate.ptr);
    vgaHWPtr hwp = VGAHWPTR(pCir->pScrn);
    
    if(!CIRI2CSwitchToBus(b))
        return;

    if(clock) reg |= 1;
    if(data)  reg |= 2;
    hwp->writeSeq(hwp, 0x08, reg);
    /* ErrorF("CIRI2CPutBits: %d %d\n", clock, data); */
}

static void
CIRI2CGetBits(I2CBusPtr b, int *clock, int *data) {
    unsigned int reg;
    CIRPtr pCir = ((CIRPtr)b->DriverPrivate.ptr);
    vgaHWPtr hwp = VGAHWPTR(pCir->pScrn);

    if(!CIRI2CSwitchToBus(b))
        return;

    reg = hwp->readSeq(hwp, 0x08);
    *clock = (reg & 0x04) != 0;
    *data  = (reg & 0x80) != 0;
    /* ErrorF("CIRI2CGetBits: %d %d\n", *clock, *data); */
}

Bool 
CIRI2CInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    CIRPtr pCir = CIRPTR(pScrn);
    I2CBusPtr I2CPtr;

#ifdef CIR_DEBUG
    ErrorF("CIRI2CInit\n");
#endif

    switch(pCir->Chipset) {
    case PCI_CHIP_GD5446:
    case PCI_CHIP_GD5480:
        break;
    default:
        return FALSE;
    }


    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    pCir->I2CPtr1 = I2CPtr;

    I2CPtr->BusName    = "I2C bus 1";
    I2CPtr->scrnIndex  = pScrn->scrnIndex;
    I2CPtr->I2CPutBits = CIRI2CPutBits;
    I2CPtr->I2CGetBits = CIRI2CGetBits;
    I2CPtr->DriverPrivate.ptr = pCir;
    
    if(!xf86I2CBusInit(I2CPtr))
       return FALSE;

    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    pCir->I2CPtr2 = I2CPtr;

    I2CPtr->BusName    = "I2C bus 2";
    I2CPtr->scrnIndex  = pScrn->scrnIndex;
    I2CPtr->I2CPutBits = CIRI2CPutBits;
    I2CPtr->I2CGetBits = CIRI2CGetBits;
    I2CPtr->DriverPrivate.ptr = pCir;
    
    if(!xf86I2CBusInit(I2CPtr))
       return FALSE;

    return TRUE;
}

/* $XFree86$ */


#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "apm.h"
#include "apm_regs.h"

#if 0
/*
 * Switch between internal I2C bus and external (DDC) bus.
 * There is one I2C port controlled bu SR08 and the programmable
 * outputs control a multiplexer.
 */
static Bool
ApmI2CSwitchToBus(I2CBusPtr b)
{
    ApmPtr pApm = ((ApmPtr)b->DriverPrivate.ptr);
    vgaHWPtr hwp = VGAHWPTR(xf86Screens[pApm->pScreen->myNum]);
    if (b == pApm->I2CBus) {
        if ((pApm->ModeReg.ExtVga[GR17] & 0x60) == 0)
		return TRUE;
	pApm->ModeReg.ExtVga[GR17] &= ~0x60;
    }
    else if(b == pApm->I2CPtr2) {
        if ((pApm->ModeReg.ExtVga[GR17] & 0x60) != 0)
		return TRUE;
        pApm->ModeReg.ExtVga[GR17] |= 0x60;
    }
    else return FALSE;

    hwp->writeGr(hwp, 0x17, pApm->ModeReg.ExtVga[GR17]);
    return TRUE;
}
#endif

/* Inline functions */
static __inline__ void
WaitForFifo(ApmPtr pApm, int slots)
{
  if (!pApm->UsePCIRetry) {
    volatile int i;
#define MAXLOOP 1000000

    for(i = 0; i < MAXLOOP; i++) {
      if ((STATUS() & STATUS_FIFO) >= slots)
	break;
    }
    if (i == MAXLOOP) {
      FatalError("Hung in WaitForFifo() (Status = 0x%08X)\n", STATUS());
    }
  }
}

static void
ApmI2CPutBits(I2CBusPtr b, int clock,  int data)
{
    unsigned int reg;
    ApmPtr pApm = ((ApmPtr)b->DriverPrivate.ptr);

    WaitForFifo(pApm, 2);
    reg = (RDXB(0xD0) & 0x07) | 0x60;
    if(clock) reg |= 0x08;
    if(data)  reg |= 0x10;
    WRXB(0xD0, reg);
}

static void
ApmI2CGetBits(I2CBusPtr b, int *clock, int *data)
{
    unsigned int reg;
    ApmPtr pApm = ((ApmPtr)b->DriverPrivate.ptr);

    WaitForFifo(pApm, 2);
    WRXB(0xD0, RDXB(0xD0) & 0x07);
    reg = STATUS();
    *clock = (reg & STATUS_SCL) != 0;
    *data  = (reg & STATUS_SDA) != 0;
}

Bool 
ApmI2CInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    APMDECL(pScrn);
    I2CBusPtr I2CPtr;

    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    pApm->I2CPtr	= I2CPtr;

    I2CPtr->BusName    = "Alliance bus";
    I2CPtr->scrnIndex  = pScrn->scrnIndex;
    I2CPtr->I2CPutBits = ApmI2CPutBits;
    I2CPtr->I2CGetBits = ApmI2CGetBits;
    I2CPtr->DriverPrivate.ptr = pApm;
    
    if(!xf86I2CBusInit(I2CPtr))
       return FALSE;

#if 0
    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    pApm->I2CPtr2 = I2CPtr;

    I2CPtr->BusName    = "I2C bus 2";
    I2CPtr->scrnIndex  = pScrn->scrnIndex;
    I2CPtr->I2CPutBits = ApmI2CPutBits;
    I2CPtr->I2CGetBits = ApmI2CGetBits;
    I2CPtr->DriverPrivate.ptr = pApm;
    
    if(!xf86I2CBusInit(I2CPtr))
       return FALSE;
#endif

    return TRUE;
}

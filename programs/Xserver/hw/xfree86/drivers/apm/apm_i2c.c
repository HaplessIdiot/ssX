/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_i2c.c,v 1.1 1999/03/21 07:35:04 dawes Exp $ */


#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "apm.h"
#include "apm_regs.h"

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

    return TRUE;
}

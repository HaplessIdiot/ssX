/* (c) Itai Nahshon */

/* $XFree86$ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "lg.h"

static void
LGI2CPutBits(I2CBusPtr b, int clock,  int data) {
    unsigned int regval, regno;
    LgPtr pLg = ((LgPtr)b->DriverPrivate.ptr);
    if (b == pLg->I2CPtr1)
        regno = 0x280;
    else if(b == pLg->I2CPtr2)
        regno = 0x282;
    else return;
    
    regval = 0xff7e;
    if(clock) regval |= 0x0080;
    if(data)  regval |= 0x0001;
    *((volatile CARD16 *)(pLg->IOBase + regno)) = regval;
    /* ErrorF("LGI2CPutBits: %d %d\n", clock, data); */
}

static void
LGI2CGetBits(I2CBusPtr b, int *clock, int *data) {
    unsigned int regval, regno;
    LgPtr pLg = ((LgPtr)b->DriverPrivate.ptr);
    if (b == pLg->I2CPtr1)
        regno = 0x280;
    else if(b == pLg->I2CPtr2)
        regno = 0x282;
    else return;

    regval = *((volatile CARD16 *)(pLg->IOBase + regno));
    *clock = (regval & 0x8000) != 0;
    *data  = (regval & 0x0100) != 0;
    /* ErrorF("LGI2CGetBits: %d %d\n", *clock, *data); */
}

Bool 
LGI2CInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    LgPtr pLg = LGPTR(pScrn);
    I2CBusPtr I2CPtr;

#ifdef CIR_DEBUG
    ErrorF("LGI2CInit\n");
#endif

    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    pLg->I2CPtr1 = I2CPtr;

    I2CPtr->BusName    = "I2C bus 1";
    I2CPtr->scrnIndex  = pScrn->scrnIndex;
    I2CPtr->I2CPutBits = LGI2CPutBits;
    I2CPtr->I2CGetBits = LGI2CGetBits;
    I2CPtr->DriverPrivate.ptr = pLg;
    
    if(!xf86I2CBusInit(I2CPtr))
       return FALSE;

    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    pLg->I2CPtr2 = I2CPtr;

    I2CPtr->BusName    = "I2C bus 2";
    I2CPtr->scrnIndex  = pScrn->scrnIndex;
    I2CPtr->I2CPutBits = LGI2CPutBits;
    I2CPtr->I2CGetBits = LGI2CGetBits;
    I2CPtr->DriverPrivate.ptr = pLg;
    
    if(!xf86I2CBusInit(I2CPtr))
       return FALSE;

    return TRUE;
}

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/s3virge/s3v_i2c.c,v 1.1 1999/08/21 13:48:39 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "s3v.h"

static void
s3v_I2CPutBits(I2CBusPtr b, int clock,  int data)
{
    S3VPtr ps3v = S3VPTR(xf86Screens[b->scrnIndex]);
    unsigned int reg = 0x10;

    if(clock) reg |= 0x1;
    if(data)  reg |= 0x2;

    OUTREG(DDC_REG,reg);
    /*ErrorF("s3v_I2CPutBits: %d %d\n", clock, data); */
}

static void
s3v_I2CGetBits(I2CBusPtr b, int *clock, int *data)
{
    S3VPtr ps3v = S3VPTR(xf86Screens[b->scrnIndex]);
    unsigned int reg;

    reg = (INREG(DDC_REG));

    *clock = reg & 0x4;
    *data = reg & 0x8;
    
    /*ErrorF("s3v_I2CGetBits: %d %d\n", *clock, *data);*/
}

Bool 
S3V_I2CInit(ScrnInfoPtr pScrn)
{
    S3VPtr ps3v = S3VPTR(pScrn);
    I2CBusPtr I2CPtr;


    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    ps3v->I2C = I2CPtr;

    I2CPtr->BusName    = "I2C bus";
    I2CPtr->scrnIndex  = pScrn->scrnIndex;
    I2CPtr->I2CPutBits = s3v_I2CPutBits;
    I2CPtr->I2CGetBits = s3v_I2CGetBits;

    if (!xf86I2CBusInit(I2CPtr))
      return FALSE;

    return TRUE;
}




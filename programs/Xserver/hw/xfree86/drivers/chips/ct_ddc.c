/* $XFree86$ */

/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* All drivers using the vgahw module need this */
#include "vgaHW.h"

#include "ct_driver.h"
#include "xf86DDC.h"

static Bool chips_TestI2C(int scrnIndex);
static Bool chips_setI2CBits(I2CBusPtr I2CPtr, ScrnInfoPtr pScrn);

static unsigned int
chips_ddc1Read(ScrnInfoPtr pScrn)
{
    unsigned char ddc_mask = ((CHIPSPtr)pScrn->driverPrivate)->ddc_mask;
    register unsigned int ST01reg = ((CHIPSPtr)pScrn->driverPrivate)->IOBase 
	+ 0x0A;
    register unsigned int tmp;

    while(inb(ST01reg)&0x8){};
    while(!(inb(ST01reg)&0x8)){};
    read_xr(0x63,tmp);
    return (tmp & ddc_mask);
}

void
chips_ddc1(ScrnInfoPtr pScrn)
{
    unsigned char FR0B, FR0C, XR62;
    unsigned char mask_c = 0x00;
    unsigned char val, tmp_val;
    int i;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);    

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Probing for DDC1\n");	

    read_fr(0x0C,FR0C);
    read_xr(0x62,XR62);
    switch (cPtr->Chipset) {
    case CHIPS_CT65550:
	cPtr->ddc_mask = 0x1F;         /* GPIO 0-4 */
	read_fr(0x0B,FR0B);
	if (!(FR0B & 0x10))      /* GPIO 2 is used as 32 kHz input */
	    cPtr->ddc_mask &= 0xFB;      
	if (cPtr->Bus == ChipsVLB) /* GPIO 3-7 are used as address bits */
	    cPtr->ddc_mask &= 0x07;
	break;
    case CHIPS_CT65554:
    case CHIPS_CT65555:
	cPtr->ddc_mask = 0x0F;        /* GPIO 0-3 */
	break;
    case CHIPS_CT69000:
	cPtr->ddc_mask = 0x9F;        /* GPIO 0-4,7? */
	break;
    default:
	cPtr->ddc_mask = 0x0C;       /* GPIO 2,3 */
	break;
    }
    if (!(FR0C & 0x80)) {       /* GPIO 1 is not available */
	mask_c |= 0xC0;
	cPtr->ddc_mask &= 0xFE;
    }
    if (!(FR0C & 0x10)) {       /* GPIO 0 is not available */
	mask_c |= 0x18;
	cPtr->ddc_mask &= 0xFD;
    }

    /* set GPIO 0,1 to read if available */
    write_fr(0x0C, (FR0C & mask_c) | (~mask_c & 0x90));
    /* set remaining GPIO to read */
    write_xr(0x62,0);

    val = chips_ddc1Read(pScrn);
    for (i = 0; i < 70; i++) {
	tmp_val = chips_ddc1Read(pScrn);
	if (tmp_val != val)
	    break;
    }
    cPtr->ddc_mask = val ^ tmp_val;
    if (cPtr->ddc_mask)
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DDC1 found\n");	
    else return;

    xf86PrintEDID(xf86DoEDID_DDC1(pScrn->scrnIndex,vgaHWddc1SetSpeed,
				  chips_ddc1Read));

    /* restore */
    write_fr(0x0C, FR0C);
    write_xr(0x62, XR62);
}

static void
chips_I2CGetBits(I2CBusPtr b, int *clock, int *data) 
{
    CHIPSI2CPtr pI2C_c = (CHIPSI2CPtr) (b->DriverPrivate.ptr);  
    unsigned char FR0C, XR62, XR63, val;

    read_fr(0x0C,FR0C);
    if (pI2C_c->i2cDataBit & 0x01 || pI2C_c->i2cClockBit & 0x01)
	FR0C = (FR0C & 0xE7) | 0x10;
    if (pI2C_c->i2cDataBit & 0x02 || pI2C_c->i2cClockBit & 0x02)
	FR0C = (FR0C & 0x3F) | 0x80;
    read_xr(0x62,XR62);
    XR62 &= (~pI2C_c->i2cDataBit) & (~pI2C_c->i2cClockBit);
    write_fr(0x0C,FR0C);
    write_xr(0x62,XR62);
    read_xr(0x63,val);
    *clock = (val & pI2C_c->i2cClockBit) != 0;
    *data  = (val & pI2C_c->i2cDataBit) != 0;
}

static void
chips_I2CPutBits(I2CBusPtr b, int clock, int data)
{
    CHIPSI2CPtr pI2C_c = (CHIPSI2CPtr) (b->DriverPrivate.ptr);  
    unsigned char FR0C, XR62, XR63, val;

    read_fr(0x0C,FR0C);
    if (pI2C_c->i2cDataBit & 0x01 || pI2C_c->i2cClockBit & 0x01)
	FR0C |=  0x18;
    if (pI2C_c->i2cDataBit & 0x02 || pI2C_c->i2cClockBit & 0x02)
	FR0C |=  0xC0;
    read_xr(0x62,XR62);
    XR62 |= (pI2C_c->i2cDataBit | pI2C_c->i2cClockBit);
    write_fr(0x0C,FR0C);
    write_xr(0x62,XR62);
    read_xr(0x63,val);
    val = (val & ~pI2C_c->i2cClockBit) | (clock ? pI2C_c->i2cClockBit : 0);
    val = (val & ~pI2C_c->i2cDataBit) | (data ? pI2C_c->i2cDataBit : 0);
    write_xr(0x63,val);
}


Bool
chips_i2cInit(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    I2CBusPtr I2CPtr;

    I2CPtr = xf86CreateI2CBusRec();
    if(!I2CPtr) return FALSE;

    cPtr->I2C = I2CPtr;

    I2CPtr->BusName    = "DDC";
    I2CPtr->scrnIndex  = pScrn->scrnIndex;
    I2CPtr->I2CPutBits = chips_I2CPutBits;
    I2CPtr->I2CGetBits = chips_I2CGetBits;
    I2CPtr->DriverPrivate.ptr = (pointer)xalloc(sizeof(CHIPSI2CRec));

    if (!xf86I2CBusInit(I2CPtr))
	return FALSE;
    
    if (!chips_setI2CBits(I2CPtr, pScrn))
	return FALSE;

    return TRUE;
}

static Bool
chips_setI2CBits(I2CBusPtr b, ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);    
    CHIPSI2CPtr pI2C_c = (CHIPSI2CPtr) (b->DriverPrivate.ptr);  
    unsigned char FR0B, FR0C, XR62;
    unsigned char bits, data_bits, clock_bits;
    int i,j;

    switch (cPtr->Chipset) {
    case CHIPS_CT65550:
	bits = 0x1F;         /* GPIO 0-4 */
	read_fr(0x0B,FR0B);
	if (!(FR0B & 0x10))      /* GPIO 2 is used as 32 kHz input */
	    bits &= 0xFB;      
	pI2C_c->i2cDataBit = 0x01;
	pI2C_c->i2cClockBit = 0x02;
	if (cPtr->Bus == ChipsVLB) /* GPIO 3-7 are used as address bits */
	    bits &= 0x07;
	break;
    case CHIPS_CT65554:
    case CHIPS_CT65555:
	bits = 0x0F;        /* GPIO 0-3 */
	pI2C_c->i2cDataBit = 0x04;
	pI2C_c->i2cClockBit = 0x08;
	break;
    case CHIPS_CT69000:
	bits = 0x9F;        /* GPIO 0-4,7? */
	pI2C_c->i2cDataBit = 0x04;
	pI2C_c->i2cClockBit = 0x08;
	break;
    default:
	bits = 0x0C;       /* GPIO 2,3 */
	pI2C_c->i2cDataBit = 0x04;
	pI2C_c->i2cClockBit = 0x08;
	break;
    }
    if (!(FR0C & 0x80)) {       /* GPIO 1 is not available */
	bits &= 0xFE;
    }
    if (!(FR0C & 0x10)) {       /* GPIO 0 is not available */
	bits &= 0xFD;
    }
    pI2C_c->i2cClockBit &= bits;
    pI2C_c->i2cDataBit &= bits;
    /*
     * first we test out the "favorite" GPIO bits ie. the ones suggested
     * by the data book; if we don't succeed test all other combinations
     * of possible GPIO pins as data/clock lines as the manufacturer might
     * have its own ideas.
     */
    if (chips_TestI2C(pScrn->scrnIndex)) return TRUE;

    data_bits = bits;
    pI2C_c->i2cDataBit = 0x01;
    for (i = 0; i<8; i++) {
	if (data_bits & 0x01) {
	    clock_bits = bits;
	    pI2C_c->i2cClockBit = 0x01;
	    for (j = 0; j<8; j++) {
		if (clock_bits & 0x01)
		    if (chips_TestI2C(pScrn->scrnIndex)) return TRUE;
		clock_bits >>= 1;
		pI2C_c->i2cClockBit <<= 1;
	    }
	}
	data_bits >>= 1;
	pI2C_c->i2cDataBit <<= 1;
    }
    /* 
     * We haven't found a valid clock/data line combination - that
     * doesn't mean there aren't any. We just haven't received an
     * answer from the relevant DDC I2C addresses. We'll have to wait
     * and see, if this is too restrictive (eg one wants to use I2C
     * for something else than DDC we might have to probe more addresses
     * or just fall back to the "favorite" GPIO lines.
     */
    return FALSE;
}

static Bool
chips_TestI2C(int scrnIndex)
{
    int i;
    I2CBusPtr b;

    b = xf86I2CFindBus(scrnIndex, "DDC");
    if (b == NULL) return FALSE;
    else {
	for(i = 0xA0; i < 0xA8; i += 2)
	    if(xf86I2CProbeAddress(b, i))
		return TRUE;
    }
    return FALSE;
}



/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/* $XFree86$ */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "via_driver.h"

/*
 * DDC2 support requires DDC_SDA_MASK and DDC_SCL_MASK
 */
#define DDC_SDA_READ_MASK  (1 << 2)
#define DDC_SCL_READ_MASK  (1 << 3)
#define DDC_SDA_WRITE_MASK (1 << 4)
#define DDC_SCL_WRITE_MASK (1 << 5)

/* I2C Function for DDC2 */
static void
VIAI2C1PutBits(I2CBusPtr b, int clock,	int data)
{
    CARD8 reg;

    outb(0x3c4, 0x26);
    reg = inb(0x3c5);
    reg &= 0xF0;
    reg |= 0x01;		    /* Enable DDC */

    if (clock)
	reg |= DDC_SCL_WRITE_MASK;
    else
	reg &= ~DDC_SCL_WRITE_MASK;

    if (data)
	reg |= DDC_SDA_WRITE_MASK;
    else
	reg &= ~DDC_SDA_WRITE_MASK;

    outb(0x3c4, 0x26);
    outb(0x3c5, reg);
}

static void
VIAI2C1GetBits(I2CBusPtr b, int *clock, int *data)
{
    CARD8 reg;

    outb(0x3c4, 0x26);
    reg = inb(0x3c5);

    *clock = (reg & DDC_SCL_READ_MASK) != 0;
    *data  = (reg & DDC_SDA_READ_MASK) != 0;
}

/* I2C Function for DVI DDC2 */
static void
VIAI2C2PutBits(I2CBusPtr b, int clock,	int data)
{
    CARD8 reg;

    outb(0x3c4, 0x31);
    reg = inb(0x3c5);
    reg &= 0xF0;
    reg |= 0x01;		    /* Enable DDC */

    if (clock)
	reg |= DDC_SCL_WRITE_MASK;
    else
	reg &= ~DDC_SCL_WRITE_MASK;

    if (data)
	reg |= DDC_SDA_WRITE_MASK;
    else
	reg &= ~DDC_SDA_WRITE_MASK;

    outb(0x3c4, 0x31);
    outb(0x3c5, reg);
}

static void
VIAI2C2GetBits(I2CBusPtr b, int *clock, int *data)
{
    CARD8 reg;

    outb(0x3c4, 0x31);
    reg = inb(0x3c5);

    *clock = (reg & DDC_SCL_READ_MASK) != 0;
    *data  = (reg & DDC_SDA_READ_MASK) != 0;
}


Bool
VIAI2CInit(ScrnInfoPtr pScrn)
{
    VIAPtr pVia = VIAPTR(pScrn);
    I2CBusPtr I2CPtr1, I2CPtr2;

    I2CPtr1 = xf86CreateI2CBusRec();
    I2CPtr2 = xf86CreateI2CBusRec();
    if (!I2CPtr1 || !I2CPtr2)
	return FALSE;

    pVia->I2C_Port1 = I2CPtr1;
    pVia->I2C_Port2 = I2CPtr2;

    I2CPtr1->BusName	= "I2C bus 1";
    I2CPtr1->scrnIndex	= pScrn->scrnIndex;
    I2CPtr1->I2CPutBits = VIAI2C1PutBits;
    I2CPtr1->I2CGetBits = VIAI2C1GetBits;

    I2CPtr2->BusName	= "I2C bus 2";
    I2CPtr2->scrnIndex	= pScrn->scrnIndex;
    I2CPtr2->I2CPutBits = VIAI2C2PutBits;
    I2CPtr2->I2CGetBits = VIAI2C2GetBits;

    if (!xf86I2CBusInit(I2CPtr1) || !xf86I2CBusInit(I2CPtr2))
	return FALSE;

    return TRUE;
}


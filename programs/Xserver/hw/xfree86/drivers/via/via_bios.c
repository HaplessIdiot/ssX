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
#include "via_driver.h"
#include "via_mode.h"
#include "via_tv2.h"
#include "via_refresh.h"

/*
 * BIOS Support 4F14 Funciton Calls
 *
 * int VIACheckTVExist(VIABIOSInfoPtr) - Check TV Endcoder
 *
 * Return Type:	   int
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   0 - None
 *		   1 - TV2+
 *		   2 - TV3
 */

int VIACheckTVExist(VIABIOSInfoPtr pBIOSInfo)
{
    I2CDevPtr	    dev;
    unsigned char   W_Buffer[1];
    unsigned char   R_Buffer[1];

    if (xf86I2CProbeAddress(pBIOSInfo->I2C_Port2, 0x40)) {
	dev = xf86CreateI2CDevRec();
	dev->DevName = "TV";
	dev->SlaveAddr = 0x40;
	dev->pI2CBus = pBIOSInfo->I2C_Port2;

	if (xf86I2CDevInit(dev)) {
	    W_Buffer[0] = 0x1B;
	    xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,1);
	    xf86DestroyI2CDevRec(dev,TRUE);
	    switch (R_Buffer[0]) {
		case 2:
		    pBIOSInfo->TVEncoder = VIA_TV2PLUS;
		    break;
		case 3:
		    pBIOSInfo->TVEncoder = VIA_TV3;
		    break;
		default:
		    pBIOSInfo->TVEncoder = VIA_NONETV;
		    break;
	    }
	}
	else
	    xf86DestroyI2CDevRec(dev,TRUE);
    }
    else
	pBIOSInfo->TVEncoder = VIA_NONETV;

    return pBIOSInfo->TVEncoder;
}


/*
 * log VIAQueryChipInfo(VIABIOSInfoPtr) - Query Chip Infomation
 *
 * Return Type:	   log
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   [31:24] Major BIOS Version Number
 *		   [31:24] Minor BIOS Version Number
 *		   [15:8] Type of External TV Encoder
 *		       0 - None
 *		       1 - VT1621
 *		       2 - VT1622
 *		   [7:6] Reserved
 *		   [5] DVI Display Supported
 *		   [4] Reserved
 *		   [3] PAL TV Display Supported
 *		   [2] NTSC TV Display Supported
 *		   [1] LCD Display Supported
 *		   [0] CRT Display Supported
 */

long VIAQueryChipInfo(VIABIOSInfoPtr pBIOSInfo)
{
    VIAModeTablePtr pViaModeTable;
    long	    tmp;
    unsigned char   support = 0;

    pViaModeTable = pBIOSInfo->pModeTable;

    tmp = ((long)(pViaModeTable->BIOSVer)) << 16;
    tmp |= ((long)(VIACheckTVExist(pBIOSInfo))) << 8;

    if (VIA_CRT_SUPPORT)
	support |= VIA_CRT_SUPPORT_BIT;
    if (VIA_LCD_SUPPORT)
	support |= VIA_LCD_SUPPORT_BIT;
    if (VIA_NTSC_SUPPORT)
	support |= VIA_NTSC_SUPPORT_BIT;
    if (VIA_PAL_SUPPORT)
	support |= VIA_PAL_SUPPORT_BIT;
    if (VIA_DVI_SUPPORT)
	support |= VIA_DVI_SUPPORT_BIT;

    tmp |= (long)(support);

    return tmp;
}


/*
 * char* VIAGetBIOSInfo(VIABIOSInfoPtr) - Get BIOS Release Date
 *
 * Return Type:	   string pointer
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   BIOS release date string pointer
 */

char* VIAGetBIOSInfo(VIABIOSInfoPtr pBIOSInfo)
{
    return pBIOSInfo->pModeTable->BIOSDate;
}


/*
 * Bool VIASetActiveDisplay(VIABIOSInfoPtr, unsigned char)
 *
 *     - Set Active Display Device
 *
 * Return Type:	   Bool
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 *		   Bit[7] 2nd Path
 *		   Bit[6] 1/0 MHS Enable/Disable
 *		   Bit[5] 0 = Bypass Callback, 1 = Enable Callback
 *		   Bit[4] 0 = Hot-Key Sequence Control (OEM Specific)
 *		   Bit[3] LCD
 *		   Bit[2] TV
 *		   Bit[1] CRT
 *		   Bit[0] DVI
 *
 * The Definition of Return Value:
 *
 *		   Success - TRUE
 *		   Not Success - FALSE
 */

Bool VIASetActiveDisplay(VIABIOSInfoPtr pBIOSInfo, unsigned char display)
{
    VIABIOSInfoPtr  pVia;
    unsigned char tmp;

    pVia = pBIOSInfo;

    switch (display & 0x0F) {
	case 0x07:	/* WCRTON + WTVON + WDVION */
	case 0x0E:	/* WCRTON + WTVON + WLCDON */
	    return FALSE;
	default:
	    break;
    }

    VGAOUT8(0x3D4, 0x3E);
    tmp = VGAIN8(0x3D5) & 0xF0;
    tmp |= (display & 0x0F);
    VGAOUT8(0x3D5, tmp);

    if ((display & 0xC0) == 0x80)
	return FALSE;

    VGAOUT8(0x3D4, 0x3B);
    tmp = VGAIN8(0x3D5) & 0xE7;
    tmp |= ((display & 0xC0) >> 3);
    VGAOUT8(0x3D5, tmp);

    return TRUE;
}


/*
 * unsigned char VIAGetActiveDisplay(VIABIOSInfoPtr, unsigned char)
 *
 *     - Get Active Display Device
 *
 * Return Type:	   unsigned char
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   Bit[7] 2nd Path
 *		   Bit[6] 1/0 MHS Enable/Disable
 *		   Bit[5] 0 = Bypass Callback, 1 = Enable Callback
 *		   Bit[4] 0 = Hot-Key Sequence Control (OEM Specific)
 *		   Bit[3] LCD
 *		   Bit[2] TV
 *		   Bit[1] CRT
 *		   Bit[0] DVI
 */

unsigned char VIAGetActiveDisplay(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    unsigned char tmp;

    pVia = pBIOSInfo;

    VGAOUT8(0x3D4, 0x3E);
    tmp = (VGAIN8(0x3D5) & 0xF0) >> 4;

    VGAOUT8(0x3D4, 0x3B);
    tmp |= ((VGAIN8(0x3D5) & 0x18) << 3);

    return tmp;
}


/*
 * unsigned char VIASensorTV2(VIABIOSInfoPtr pBIOSInfo)
 *
 *     - Sense TV2+ Encoder
 *
 * Return Type:	   unsigned char
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   Bit[2] Bit[0]
 *		   0	  0	 - Composite + S-Video
 *		   0	  1	 - S-Video
 *		   1	  0	 - Composite
 *		   1	  1	 - None
 */

unsigned char VIASensorTV2(VIABIOSInfoPtr pBIOSInfo)
{
    I2CDevPtr	    dev;
    unsigned char   save, tmp = 0x05;
    unsigned char   W_Buffer[2];
    unsigned char   R_Buffer[1];

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIASensorTV2\n"));
    dev = xf86CreateI2CDevRec();
    dev->DevName = "VT1621";
    dev->SlaveAddr = 0x40;
    dev->pI2CBus = pBIOSInfo->I2C_Port2;

    if (xf86I2CDevInit(dev)) {
	W_Buffer[0] = 0x0E;
	xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,1);
	save = R_Buffer[0];
	W_Buffer[1] = 0x08;
	xf86I2CWriteRead(dev, W_Buffer,2, NULL,0);
	W_Buffer[1] = 0;
	xf86I2CWriteRead(dev, W_Buffer,2, NULL,0);
	W_Buffer[0] = 0x0F;
	xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,1);
	tmp = R_Buffer[0] & 0x0F;
	W_Buffer[0] = 0x0E;
	W_Buffer[1] = save;
	xf86I2CWriteRead(dev, W_Buffer,2, NULL,0);
	xf86DestroyI2CDevRec(dev,TRUE);
    }
    else {
	xf86DestroyI2CDevRec(dev,TRUE);
    }

    return tmp;
}


/*
 * unsigned char VIASensorTV3(VIABIOSInfoPtr pBIOSInfo)
 *
 *     - Sense TV3 Encoder
 *
 * Return Type:	   unsigned char
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   Bit[3] Bit[2] Bit[1] Bit[0]
 *		   1	  1	 1	1      - None
 *		   0	  1	 1	1      - Composite
 *		   1	  1	 1	0      - Composite
 *					       - Others: S-Video
 */

unsigned char VIASensorTV3(VIABIOSInfoPtr pBIOSInfo)
{
    I2CDevPtr	    dev;
    unsigned char   save, tmp = 0;
    unsigned char   W_Buffer[2];
    unsigned char   R_Buffer[1];

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIASensorTV3\n"));
    dev = xf86CreateI2CDevRec();
    dev->DevName = "VT1622";
    dev->SlaveAddr = 0x40;
    dev->pI2CBus = pBIOSInfo->I2C_Port2;

    if (xf86I2CDevInit(dev)) {
	W_Buffer[0] = 0x0E;
	xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,1);
	save = R_Buffer[0];
	W_Buffer[1] = 0;
	xf86I2CWriteRead(dev, W_Buffer,2, NULL,0);
	W_Buffer[1] = 0x80;
	xf86I2CWriteRead(dev, W_Buffer,2, NULL,0);
	W_Buffer[1] = 0;
	xf86I2CWriteRead(dev, W_Buffer,2, NULL,0);
	W_Buffer[0] = 0x0F;
	xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,1);
	tmp = R_Buffer[0] & 0x0F;
	W_Buffer[0] = 0x0E;
	W_Buffer[1] = save;
	xf86I2CWriteRead(dev, W_Buffer,2, NULL,0);
	xf86DestroyI2CDevRec(dev,TRUE);
    }
    else {
	xf86DestroyI2CDevRec(dev,TRUE);
    }

    return tmp;
}


/*
 * Bool VIASensorDVI(pBIOSInfo)
 *
 *     - Sense DVI Connector
 *
 * Return Type:	   Bool
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   DVI Attached	 - TRUE
 *		   DVI Not Attached - FALSE
 */

Bool VIASensorDVI(VIABIOSInfoPtr pBIOSInfo)
{
    I2CDevPtr	    dev;
    unsigned char   W_Buffer[1];
    unsigned char   R_Buffer[1];
    unsigned char   misc;
    VIABIOSInfoPtr  pVia;
    VIAModeTablePtr pViaModeTable;
    int		    port, offset, mask, data;
    int		    i, j, k;


    pVia = pBIOSInfo;
    pViaModeTable = pBIOSInfo->pModeTable;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIASensorDVI\n"));
    /* Find Panel Size Index */
    for (i = 0; i < VIA_BIOS_NUM_PANEL; i++) {
	if (pViaModeTable->lcdTable[i].fpSize == pBIOSInfo->PanelSize)
	    break;
    };

    /* Enable LCD */
    for (j = 0; j < pViaModeTable->NumPowerOn; j++) {
	if (pViaModeTable->lcdTable[i].powerSeq ==
	    pViaModeTable->powerOn[j].powerSeq)
	    break;
    }

    for (k = 0; k < pViaModeTable->powerOn[j].numEntry; k++) {
	port = pViaModeTable->powerOn[j].port[k];
	offset = pViaModeTable->powerOn[j].offset[k];
	mask = pViaModeTable->powerOn[j].mask[k];
	data = pViaModeTable->powerOn[j].data[k] & mask;
	VGAOUT8(0x300+port, offset);
	VGAOUT8(0x301+port, data);
	usleep(pViaModeTable->powerOn[j].delay[k]);
    }

    VGAOUT8(0x3d4, 0x6A);
    misc = VGAIN8(0x3d5);
    VGAOUT8(0x3d5, 0x48);
    usleep(1);

    if (xf86I2CProbeAddress(pBIOSInfo->I2C_Port2, 0x70)) {

	dev = xf86CreateI2CDevRec();
	dev->DevName = "TMDS";
	dev->SlaveAddr = 0x70;
	dev->pI2CBus = pBIOSInfo->I2C_Port2;

	if (xf86I2CDevInit(dev)) {
	    W_Buffer[0] = 0x09;
	    xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,1);
	    xf86DestroyI2CDevRec(dev,TRUE);
	    for (k = 0; k < pViaModeTable->powerOff[j].numEntry; k++) {
		port = pViaModeTable->powerOff[j].port[k];
		offset = pViaModeTable->powerOff[j].offset[k];
		mask = pViaModeTable->powerOff[j].mask[k];
		data = pViaModeTable->powerOff[j].data[k] & mask;
		VGAOUT8(0x300+port, offset);
		VGAOUT8(0x301+port, data);
		usleep(pViaModeTable->powerOff[j].delay[k]);
	    }
	    VGAOUT8(0x3d4, 0x6A);
	    VGAOUT8(0x3d5, misc);
	    if (R_Buffer[0] & 0x04) {
		return TRUE;
	    }
	    else {
		return FALSE; /* DVI Not Attached */
	    }
	}
	else {
	    xf86DestroyI2CDevRec(dev,TRUE);
	    for (k = 0; k < pViaModeTable->powerOff[j].numEntry; k++) {
		port = pViaModeTable->powerOff[j].port[k];
		offset = pViaModeTable->powerOff[j].offset[k];
		mask = pViaModeTable->powerOff[j].mask[k];
		data = pViaModeTable->powerOff[j].data[k] & mask;
		VGAOUT8(0x300+port, offset);
		VGAOUT8(0x301+port, data);
		usleep(pViaModeTable->powerOff[j].delay[k]);
	    }
	    VGAOUT8(0x3d4, 0x6A);
	    VGAOUT8(0x3d5, misc);
	    return FALSE;
	}
    }

    else if (xf86I2CProbeAddress(pBIOSInfo->I2C_Port2, 0x10)) {
	dev = xf86CreateI2CDevRec();
	dev->DevName = "TMDS";
	dev->SlaveAddr = 0x10;
	dev->pI2CBus = pBIOSInfo->I2C_Port2;

	if (xf86I2CDevInit(dev)) {
	    W_Buffer[0] = 0x09;
	    xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,1);
	    xf86DestroyI2CDevRec(dev,TRUE);
	    for (k = 0; k < pViaModeTable->powerOff[j].numEntry; k++) {
		port = pViaModeTable->powerOff[j].port[k];
		offset = pViaModeTable->powerOff[j].offset[k];
		mask = pViaModeTable->powerOff[j].mask[k];
		data = pViaModeTable->powerOff[j].data[k] & mask;
		VGAOUT8(0x300+port, offset);
		VGAOUT8(0x301+port, data);
		usleep(pViaModeTable->powerOff[j].delay[k]);
	    }
	    VGAOUT8(0x3d4, 0x6A);
	    VGAOUT8(0x3d5, misc);
	    if (R_Buffer[0] & 0x04) {
		return TRUE;
	    }
	    else {
		return FALSE; /* DVI Not Attached */
	    }
	}
	else {
	    xf86DestroyI2CDevRec(dev,TRUE);
	    for (k = 0; k < pViaModeTable->powerOff[j].numEntry; k++) {
		port = pViaModeTable->powerOff[j].port[k];
		offset = pViaModeTable->powerOff[j].offset[k];
		mask = pViaModeTable->powerOff[j].mask[k];
		data = pViaModeTable->powerOff[j].data[k] & mask;
		VGAOUT8(0x300+port, offset);
		VGAOUT8(0x301+port, data);
		usleep(pViaModeTable->powerOff[j].delay[k]);
	    }
	    VGAOUT8(0x3d4, 0x6A);
	    VGAOUT8(0x3d5, misc);
	    return FALSE;
	}
    }
    else {
	for (k = 0; k < pViaModeTable->powerOff[j].numEntry; k++) {
	    port = pViaModeTable->powerOff[j].port[k];
	    offset = pViaModeTable->powerOff[j].offset[k];
	    mask = pViaModeTable->powerOff[j].mask[k];
	    data = pViaModeTable->powerOff[j].data[k] & mask;
	    VGAOUT8(0x300+port, offset);
	    VGAOUT8(0x301+port, data);
	    usleep(pViaModeTable->powerOff[j].delay[k]);
	}
	VGAOUT8(0x3d4, 0x6A);
	VGAOUT8(0x3d5, misc);
	return FALSE; /* Device: TMDS not found */
    }
}

Bool VIAPostDVI(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia = pBIOSInfo;
    VIAModeTablePtr pViaModeTable = pBIOSInfo->pModeTable;
    int		    port, offset, mask, data;
    int		    i, j, k;
    unsigned char   misc;
    Bool	    ret = FALSE;
    I2CDevPtr	    dev;
    unsigned char   W_Buffer[2];
    unsigned char   R_Buffer[2];
    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAPostDVI\n"));

    /* Find Panel Size Index */
    for (i = 0; i < VIA_BIOS_NUM_PANEL; i++) {
	if (pViaModeTable->lcdTable[i].fpSize == pBIOSInfo->PanelSize)
	    break;
    };
    /* Enable LCD */
    for (j = 0; j < pViaModeTable->NumPowerOn; j++) {
	if (pViaModeTable->lcdTable[i].powerSeq ==
	    pViaModeTable->powerOn[j].powerSeq)
	    break;
    }

    for (k = 0; k < pViaModeTable->powerOn[j].numEntry; k++) {
	port = pViaModeTable->powerOn[j].port[k];
	offset = pViaModeTable->powerOn[j].offset[k];
	mask = pViaModeTable->powerOn[j].mask[k];
	data = pViaModeTable->powerOn[j].data[k] & mask;
	VGAOUT8(0x300+port, offset);
	VGAOUT8(0x301+port, data);
	usleep(pViaModeTable->powerOn[j].delay[k]);
    }
    /* Enable LCD first, Before Post and Sensor */
    VGAOUT8(0x3d4, 0x6A);
    misc = VGAIN8(0x3d5);
    VGAOUT8(0x3d5, 0x48);
    usleep(1);

    if (xf86I2CProbeAddress(pBIOSInfo->I2C_Port2, 0x70)) {
	dev = xf86CreateI2CDevRec();
	dev->DevName = "TMDS";
	dev->SlaveAddr = 0x70;
	dev->pI2CBus = pBIOSInfo->I2C_Port2;
	if (xf86I2CDevInit(dev)) {
	    W_Buffer[0] = 0x00;
	    xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,2);
	    if (R_Buffer[0] == 0x06 && R_Buffer[1] == 0x11) {		     DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_PROBED, "Found LVDS Transmiter VT3191.\n"));
		pBIOSInfo->DVIEncoder = VIA_VT3191;
		W_Buffer[0] = 0x02;
		xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,2);
		if (R_Buffer[0] == 0x91 && R_Buffer[1] == 0x31) {
		    W_Buffer[0] = 0x08;
					if (pBIOSInfo->LCDDualEdge)
			W_Buffer[1] = 0x0D;
		    else
			W_Buffer[1] = 0x09;
		    xf86I2CWriteRead(dev, W_Buffer,2, R_Buffer,0);
		    xf86DestroyI2CDevRec(dev,TRUE);
		    ret = TRUE;
		}
		else {
		    xf86DestroyI2CDevRec(dev,TRUE);
		}
	    }
	    else if (R_Buffer[0] == 0x01 && R_Buffer[1] == 0x00) {  /* Sil164 */
		DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_PROBED, "Found TMDS Transmiter Silicon164.\n"));
		pBIOSInfo->DVIEncoder = VIA_SIL164;
		W_Buffer[0] = 0x02;
		xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,2);
		if (R_Buffer[0] && R_Buffer[1]) {
		    W_Buffer[0] = 0x08;
		    W_Buffer[1] = 0x37;
		    xf86I2CWriteRead(dev, W_Buffer,2, R_Buffer,0);
		    W_Buffer[0] = 0x0C;
		    W_Buffer[1] = 0x09;
		    xf86I2CWriteRead(dev, W_Buffer,2, R_Buffer,0);
		    xf86DestroyI2CDevRec(dev,TRUE);
		    ret = TRUE;
		}
		else {
		    xf86DestroyI2CDevRec(dev,TRUE);
		}
	    }
	    else {
		xf86DestroyI2CDevRec(dev,TRUE);
	    }
	}
	else {
	    xf86DestroyI2CDevRec(dev,TRUE);
	}
	for (k = 0; k < pViaModeTable->powerOff[j].numEntry; k++) {
	    port = pViaModeTable->powerOff[j].port[k];
	    offset = pViaModeTable->powerOff[j].offset[k];
	    mask = pViaModeTable->powerOff[j].mask[k];
	    data = pViaModeTable->powerOff[j].data[k] & mask;
	    VGAOUT8(0x300+port, offset);
	    VGAOUT8(0x301+port, data);
	    usleep(pViaModeTable->powerOff[j].delay[k]);
	}
	VGAOUT8(0x3d4, 0x6A);	    /* Restore CR6A status back */
	VGAOUT8(0x3d5, misc);
	return ret;
    }
    else if (xf86I2CProbeAddress(pBIOSInfo->I2C_Port2, 0x10)) {
	dev = xf86CreateI2CDevRec();
	dev->DevName = "TMDS";
	dev->SlaveAddr = 0x10;
	dev->pI2CBus = pBIOSInfo->I2C_Port2;
	if (xf86I2CDevInit(dev)) {
	    W_Buffer[0] = 0x00;
	    xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,2);
	    if (R_Buffer[0] == 0x06 && R_Buffer[1] == 0x11) {
		DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_PROBED, "Found TMDS Transmiter VT3192.\n"));
		pBIOSInfo->DVIEncoder = VIA_VT3192;
		W_Buffer[0] = 0x02;
		xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,2);
		if (R_Buffer[0] == 0x92 && R_Buffer[1] == 0x31) {
		    W_Buffer[0] = 0x08;
		    W_Buffer[1] = 0x37;
		    xf86I2CWriteRead(dev, W_Buffer,2, R_Buffer,0);
		    xf86DestroyI2CDevRec(dev,TRUE);
		    ret = TRUE;
		}
		else {
		    xf86DestroyI2CDevRec(dev,TRUE);
		}
	    }
	    else {
		xf86DestroyI2CDevRec(dev,TRUE);
	    }
	}
	else {
	    xf86DestroyI2CDevRec(dev,TRUE);
	}
	for (k = 0; k < pViaModeTable->powerOff[j].numEntry; k++) {
	    port = pViaModeTable->powerOff[j].port[k];
	    offset = pViaModeTable->powerOff[j].offset[k];
	    mask = pViaModeTable->powerOff[j].mask[k];
	    data = pViaModeTable->powerOff[j].data[k] & mask;
	    VGAOUT8(0x300+port, offset);
	    VGAOUT8(0x301+port, data);
	    usleep(pViaModeTable->powerOff[j].delay[k]);
	}
	VGAOUT8(0x3d4, 0x6A);	    /* Restore CR6A status back */
	VGAOUT8(0x3d5, misc);
	return ret;
    }
    for (k = 0; k < pViaModeTable->powerOff[j].numEntry; k++) {
	port = pViaModeTable->powerOff[j].port[k];
	offset = pViaModeTable->powerOff[j].offset[k];
	mask = pViaModeTable->powerOff[j].mask[k];
	data = pViaModeTable->powerOff[j].data[k] & mask;
	VGAOUT8(0x300+port, offset);
	VGAOUT8(0x301+port, data);
	usleep(pViaModeTable->powerOff[j].delay[k]);
    }
    VGAOUT8(0x3d4, 0x6A);	    /* Restore CR6A status back */
    VGAOUT8(0x3d5, misc);

    return ret;
}

/*
 * unsigned char VIAGetDeviceDetect(VIABIOSInfoPtr)
 *
 *     - Get Display Device Attched
 *
 * Return Type:	   unsigned char
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   Bit[7] Reserved ------------ 2nd TV Connector
 *		   Bit[6] Reserved ------------ 1st TV Connector
 *		   Bit[5] Reserved
 *		   Bit[4] CRT2
 *		   Bit[3] DFP
 *		   Bit[2] TV
 *		   Bit[1] LCD
 *		   Bit[0] CRT
 */

unsigned char VIAGetDeviceDetect(VIABIOSInfoPtr pBIOSInfo)
{
    unsigned char tmp, sense;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAGetDeviceDetect\n"));

    tmp = VIA_DEVICE_CRT1; /* Default assume color CRT attached */
/*
    if (VIA_LCD_SUPPORT) {
	if (VIA_UNCOVERD_LCD_PANEL)
	    tmp |= 0x08;
    }
*/
	if (pBIOSInfo->DVIEncoder == VIA_VT3191)
		tmp |= VIA_DEVICE_LCD;

    switch (pBIOSInfo->TVEncoder) {
	case VIA_NONETV:
	    pBIOSInfo->TVAttach = FALSE;
	    break;
	case VIA_TV2PLUS:
/*	      if (pBIOSInfo->TVOutput == TVOUTPUT_NONE || pBIOSInfo->TVOutput >= 0x04) {*/
		sense = VIASensorTV2(pBIOSInfo);
		if (sense == 0x05) {
		    pBIOSInfo->TVAttach = FALSE; /* No TV Output Attached */
		}
		else {
		    tmp |= VIA_DEVICE_TV;
		    pBIOSInfo->TVAttach = TRUE;
		    if (!pBIOSInfo->TVOutput) {
			if (sense == 0) {
			    /*tmp |= 0xC0;  Connect S_Video + Composite */
			    pBIOSInfo->TVOutput = TVOUTPUT_SC;
			}
			if (sense == 0x01) {
			    /*tmp |= 0x80;  Connect S_Video */
			    pBIOSInfo->TVOutput = TVOUTPUT_SVIDEO;
			}
			else {
			    /*tmp |= 0x40;  Connect Composite */
			    pBIOSInfo->TVOutput = TVOUTPUT_COMPOSITE;
			}
		    }
		    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_PROBED, "TV2 sense = %d ,TVOutput = %d\n", sense, pBIOSInfo->TVOutput));
		}
/*	      }*/
	    break;
	case VIA_TV3:
/*	      if (pBIOSInfo->TVOutput == TVOUTPUT_NONE || pBIOSInfo->TVOutput >= 0x04) {*/
		sense = VIASensorTV3(pBIOSInfo);
		if (sense == 0x0F) {
		    pBIOSInfo->TVAttach = FALSE; /* No TV Output Attached */
		}
		else {
		    tmp |= VIA_DEVICE_TV;
		    pBIOSInfo->TVAttach = TRUE;
		    if (!pBIOSInfo->TVOutput) {
			if (sense == 0x07 || sense == 0x0E) {
			    /*tmp |= 0x40;  Connect Composite */
			    pBIOSInfo->TVOutput = TVOUTPUT_COMPOSITE;
			}
			else {
			    /*tmp |= 0x80;  Connect S_Video */
			    pBIOSInfo->TVOutput = TVOUTPUT_SVIDEO;
			}
		    }
		    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_PROBED, "TV3 sense = %d ,TVOutput = %d\n", sense, pBIOSInfo->TVOutput));
		}
/*	      }*/
	    break;
	default:
	    break;
    }

    if (VIASensorDVI(pBIOSInfo) || (pBIOSInfo->DVIEncoder == VIA_VT3191)) {
	tmp |= VIA_DEVICE_DFP;
	pBIOSInfo->DVIAttach = TRUE;
	DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_PROBED, "DVI has Attachment.\n"));
    }
    else {
	pBIOSInfo->DVIAttach = FALSE;
	DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_PROBED, "DVI hasn't Attachment.\n"));
    }
    return tmp;
}


/*
 * Bool VIASetPanelState(VIABIOSInfoPtr, unsigned char)
 *
 *     - Set Flat Panel Expaension/Centering State
 *
 * Return Type:	   Bool
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 *		   Bit[7:1] Reserved
 *		   Bit[0] 0/1 = Centering/Expansion
 *
 * The Definition of Return Value:
 *
 *		   Success - TRUE
 *		   Not Success - FALSE
 */

Bool VIASetPanelState(VIABIOSInfoPtr pBIOSInfo, unsigned char state)
{
    VIABIOSInfoPtr  pVia;
    unsigned char tmp;

    pVia = pBIOSInfo;

    tmp = state & 0x01;
    VGAOUT8(0x3D4, 0x3B);
    tmp |= (VGAIN8(0x3D5) & 0xFE);
    VGAOUT8(0x3D5, tmp);

    return TRUE;
}


/*
 * unsigned char VIAGetPanelState(VIABIOSInfoPtr)
 *
 *     - Get Flat Panel Expaension/Centering State
 *
 * Return Type:	   unsigend char
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 *		   Bit[7:1] Reserved
 *		   Bit[0] 0/1 = Centering/Expansion
 *
 * The Definition of Return Value:
 *
 *		   Bit[7:1] Reserved
 *		   Bit[0] 0/1 = Centering/Expansion
 */

unsigned char VIAGetPanelState(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    unsigned char tmp;

    pVia = pBIOSInfo;

    VGAOUT8(0x3D4, 0x3B);
    tmp = VGAIN8(0x3D5) & 0x01;

    return tmp;
}


/*
 * int VIAQueryDVIEDID(void)
 *
 *     - Query Flat Panel's EDID Table Version Through DVI Connector
 *
 * Return Type:	   int
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   0 - Found No EDID Table
 *		   1 - Found EDID1 Table
 *		   2 - Found EDID2 Table
 */

int VIAQueryDVIEDID(VIABIOSInfoPtr pBIOSInfo)
{
    I2CDevPtr	    dev;
    unsigned char   W_Buffer[1];
    unsigned char   R_Buffer[2];

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAQueryDVIEDID\n"));
    if (xf86I2CProbeAddress(pBIOSInfo->I2C_Port2, 0xA0)) {
	pBIOSInfo->dev = xf86CreateI2CDevRec();
	dev = pBIOSInfo->dev;
	dev->DevName = "EDID1";
	dev->SlaveAddr = 0xA0;
	dev->ByteTimeout = 2200; /* VESA DDC spec 3 p. 43 (+10 %) */
	dev->StartTimeout = 550;
	dev->BitTimeout = 40;
	dev->ByteTimeout = 40;
	dev->AcknTimeout = 40;
	dev->pI2CBus = pBIOSInfo->I2C_Port2;

	if (xf86I2CDevInit(dev)) {
	    W_Buffer[0] = 0;
	    xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,2);
	    if ((R_Buffer[0] == 0) && (R_Buffer[1] == 0xFF))
		return 1; /* Found EDID1 Table */
	    else
		xf86DestroyI2CDevRec(dev,TRUE);
	}
	else {
	    xf86DestroyI2CDevRec(dev,TRUE);
	}
    }

    if (xf86I2CProbeAddress(pBIOSInfo->I2C_Port2, 0xA2)) {
	pBIOSInfo->dev = xf86CreateI2CDevRec();
	dev = pBIOSInfo->dev;
	dev->DevName = "EDID2";
	dev->SlaveAddr = 0xA2;
	dev->ByteTimeout = 2200; /* VESA DDC spec 3 p. 43 (+10 %) */
	dev->StartTimeout = 550;
	dev->BitTimeout = 40;
	dev->ByteTimeout = 40;
	dev->AcknTimeout = 40;
	dev->pI2CBus = pBIOSInfo->I2C_Port2;

	if (xf86I2CDevInit(dev)) {
	    W_Buffer[0] = 0;
	    xf86I2CWriteRead(dev, W_Buffer,1, R_Buffer,1);
	    if (R_Buffer[0] == 0x20)
		return 2; /* Found EDID2 Table */
	    else {
		xf86DestroyI2CDevRec(dev,TRUE);
		return 0; /* Found No EDID Table */
	    }
	}
	else {
	    xf86DestroyI2CDevRec(dev,TRUE);
	    return 0; /* Found No EDID Table */
	}
    }
    else
	return 0;
}


/*
 * unsigned char VIAGetPanelSizeFromDDCv1(VIABIOSInfoPtr pBIOSInfo)
 *
 *     - Get Panel Size Using EDID1 Table
 *
 * Return Type:	   unsigned char
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   0 - 640x480
 *		   1 - 800x600
 *		   2 - 1024x768
 *		   3 - 1280x768
 *		   4 - 1280x1024
 *		   5 - 1400x1050
 *		   6 - 1600x1200
 *		   0xFF - Not Supported Panel Size
 */

unsigned char VIAGetPanelSizeFromDDCv1(VIABIOSInfoPtr pBIOSInfo)
{
    ScrnInfoPtr	    pScrn = xf86Screens[pBIOSInfo->scrnIndex];
    xf86MonPtr	    pMon;
    int		    i, max = 0;
    unsigned char   W_Buffer[1];
    unsigned char   R_Buffer[1];

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAGetPanelSizeFromDDCv1\n"));
    for (i = 0x23; i < 0x35; i++) {
	switch (i) {
	    case 0x23:
		W_Buffer[0] = i;
		xf86I2CWriteRead(pBIOSInfo->dev, W_Buffer,1, R_Buffer,1);
		if (R_Buffer[0] & 0x3C)
		    max = 640;
		if (R_Buffer[0] & 0xC0)
		    max = 720;
		if (R_Buffer[0] & 0x03)
		    max = 800;
		break;
	    case 0x24:
		W_Buffer[0] = i;
		xf86I2CWriteRead(pBIOSInfo->dev, W_Buffer,1, R_Buffer,1);
		if (R_Buffer[0] & 0xC0)
		    max = 800;
		if (R_Buffer[0] & 0x1E)
		    max = 1024;
		if (R_Buffer[0] & 0x01)
		    max = 1280;
		break;
	    case 0x26:
	    case 0x28:
	    case 0x2A:
	    case 0x2C:
	    case 0x2E:
	    case 0x30:
	    case 0x32:
	    case 0x34:
		W_Buffer[0] = i;
		xf86I2CWriteRead(pBIOSInfo->dev, W_Buffer,1, R_Buffer,1);
		if (R_Buffer[0] == 1)
		    break;
		R_Buffer[0] += 31;
		R_Buffer[0] = R_Buffer[0] << 3; /* data = (data + 31) * 8 */
		if (R_Buffer[0] > max)
		    max = R_Buffer[0];
		break;
	    default:
		break;
	}
    }

    xf86DestroyI2CDevRec(pBIOSInfo->dev,TRUE);

    pMon = xf86DoEDID_DDC2(pScrn->scrnIndex, pBIOSInfo->I2C_Port2);
    if (pMon) {
	pBIOSInfo->DDC2 =  pMon;
	xf86PrintEDID(pMon);
	xf86SetDDCproperties(pScrn, pMon);
	for (i = 0; i < 8; i++) {
	    if (pMon->timings2[i].hsize > max) {
		max = pMon->timings2[i].hsize;
	    }
	}
	if (pBIOSInfo->DDC1) {
	    xf86SetDDCproperties(pScrn, pBIOSInfo->DDC1);
	}
    }
    switch (max) {
	case 640:
	    pBIOSInfo->PanelSize = VIA_PANEL6X4;
	    break;
	case 800:
	    pBIOSInfo->PanelSize = VIA_PANEL8X6;
	    break;
	case 1024:
	    pBIOSInfo->PanelSize = VIA_PANEL10X7;
	    break;
	case 1280:
	    pBIOSInfo->PanelSize = VIA_PANEL12X10;
	    break;
	case 1400:
	    pBIOSInfo->PanelSize = VIA_PANEL14X10;
	    break;
	case 1600:
	    pBIOSInfo->PanelSize = VIA_PANEL16X12;
	    break;
	default:
	    pBIOSInfo->PanelSize = VIA_PANEL_INVALID;
	    break;
    }
    return pBIOSInfo->PanelSize;
}


/*
 * unsigned char VIAGetPanelSizeFromDDCv2(VIABIOSInfoPtr pBIOSInfo)
 *
 *     - Get Panel Size Using EDID2 Table
 *
 * Return Type:	   unsigned char
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   0 - 640x480
 *		   1 - 800x600
 *		   2 - 1024x768
 *		   3 - 1280x768
 *		   4 - 1280x1024
 *		   5 - 1400x1050
 *		   6 - 1600x1200
 *		   0xFF - Not Supported Panel Size
 */

unsigned char VIAGetPanelSizeFromDDCv2(VIABIOSInfoPtr pBIOSInfo)
{
    int		    data = 0;
    unsigned char   W_Buffer[1];
    unsigned char   R_Buffer[1];

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAGetPanelSizeFromDDCv2\n"));
    W_Buffer[0] = 0x77;
    xf86I2CWriteRead(pBIOSInfo->dev, W_Buffer,1, R_Buffer,1);
    data = R_Buffer[0];
    data = data << 8;
    W_Buffer[0] = 0x76;
    xf86I2CWriteRead(pBIOSInfo->dev, W_Buffer,1, R_Buffer,1);
    data |= R_Buffer[0];

    xf86DestroyI2CDevRec(pBIOSInfo->dev,TRUE);

    switch (data) {
	case 640:
	    pBIOSInfo->PanelSize = VIA_PANEL6X4;
	    break;
	case 800:
	    pBIOSInfo->PanelSize = VIA_PANEL8X6;
	    break;
	case 1024:
	    pBIOSInfo->PanelSize = VIA_PANEL10X7;
	    break;
	case 1280:
	    pBIOSInfo->PanelSize = VIA_PANEL12X10;
	    break;
	case 1400:
	    pBIOSInfo->PanelSize = VIA_PANEL14X10;
	    break;
	case 1600:
	    pBIOSInfo->PanelSize = VIA_PANEL16X12;
	    break;
	default:
	    pBIOSInfo->PanelSize = VIA_PANEL_INVALID;
	    break;
    }
    return pBIOSInfo->PanelSize;
}


/*
 * unsigned char VIAGetPanelInfo(VIABIOSInfoPtr pBIOSInfo)
 *
 *     - Get Panel Size
 *
 * Return Type:	   unsigned char
 *
 * The Definition of Input Value:
 *
 *		   VIABIOSInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   0 - 640x480
 *		   1 - 800x600
 *		   2 - 1024x768
 *		   3 - 1280x768
 *		   4 - 1280x1024
 *		   5 - 1400x1050
 *		   6 - 1600x1200
 *		   0xFF - Not Supported Panel Size
 */

unsigned char VIAGetPanelInfo(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia = pBIOSInfo;
    unsigned char   misc;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAGetPanelInfo\n"));

    if (pBIOSInfo->PanelSize == VIA_PANEL_INVALID) {
	/* Software Power Sequence On (Hard Code when Panal Size Unknow) */
		VGAOUT8(0x3d4, 0x91);
	    VGAOUT8(0x3d5, 0x10);
		usleep(25);
		VGAOUT8(0x3d4, 0x91);
	    VGAOUT8(0x3d5, 0x8);
		usleep(100);

	    /* Enable LCD */
	    VGAOUT8(0x3d4, 0x6A);
	    misc = VGAIN8(0x3d5);
	    VGAOUT8(0x3d5, 0x48);
	    usleep(1);

	switch (VIAQueryDVIEDID(pBIOSInfo)) {
	    case 1:
		VIAGetPanelSizeFromDDCv1(pBIOSInfo);
		break;
	    case 2:
		VIAGetPanelSizeFromDDCv2(pBIOSInfo);
		break;
	    default:
		break;
	}

	    /* Software Power Sequence Off (Hard Code when Panal Size Unknow) */
		VGAOUT8(0x3d4, 0x91);
	    VGAOUT8(0x3d5, 0x0);
		usleep(1);

	    /* Disable LCD */
	    VGAOUT8(0x3d4, 0x6A);
	    VGAOUT8(0x3d5, misc);
    }

	DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "PanelSize = %d\n", pBIOSInfo->PanelSize));
	return (unsigned char)(pBIOSInfo->PanelSize);
}

Bool VIAGetBIOSTable(VIABIOSInfoPtr pBIOSInfo)
{
    VIAModeTablePtr pViaModeTable = pBIOSInfo->pModeTable;
    int		    i, j;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAGetBIOSTable\n"));

    pViaModeTable->BIOSVer = BIOSVer;
    pViaModeTable->BIOSDate = BIOSDate;
    pViaModeTable->NumModes = NumModes;
    pViaModeTable->NumPowerOn = NumPowerOn;
    pViaModeTable->NumPowerOff = NumPowerOff;
    pViaModeTable->Modes = Modes;
    pViaModeTable->commExtTable = commExtTable;
    pViaModeTable->stdModeExtTable = stdModeExtTable;
    for (i = 0; i < VIA_BIOS_NUM_RES; i++) {
	for (j = 0; j < VIA_BIOS_NUM_REFRESH; j++) {
	    pViaModeTable->refreshTable[i][j] = refreshTable[i][j];
	}
    }
    for (i = 0; i < VIA_BIOS_NUM_PANEL; i++) {
	pViaModeTable->lcdTable[i] = lcdTable[i];
    }
    for (i = 0; i < VIA_BIOS_NUM_LCD_POWER_SEQ; i++) {
	pViaModeTable->powerOn[i] = powerOn[i];
	pViaModeTable->powerOff[i] = powerOff[i];
    }
    pViaModeTable->modeFix = modeFix;
    pViaModeTable->tv2MaskTable = tv2MaskTable;
    for (i = 0; i < VIA_BIOS_NUM_TV2; i++) {
	pViaModeTable->tv2Table[i] = tv2Table[i];
	pViaModeTable->tv2OverTable[i] = tv2OverTable[i];
    }
    pViaModeTable->tv3MaskTable = tv3MaskTable;
    for (i = 0; i < VIA_BIOS_NUM_TV3; i++) {
	pViaModeTable->tv3Table[i] = tv3Table[i];
	pViaModeTable->tv3OverTable[i] = tv3OverTable[i];
    }
    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "BIOS Version: %x.%x\n",
	    (pViaModeTable->BIOSVer >> 8), (pViaModeTable->BIOSVer & 0xFF)));
    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "BIOS Release Date: %s\n", pViaModeTable->BIOSDate));
    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "Get mode table from via_mode.h\n"));
    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "-- VIAGetBIOSTable Done!\n"));

    DEBUG(xf86Msg(pBIOSInfo->scrnIndex, X_INFO, "VIAGetBIOSTable Done\n"));
    return TRUE;
}


/* Find supported mode */
int VIAFindSupportRefreshRate(VIABIOSInfoPtr pBIOSInfo,
			      int resIndex)
{
    VIABIOSInfoPtr  pVia;
    int		    bppIndex, refIndex;
    int		    needRefresh;
    int		    *supRefTab;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAFindSupportRefreshRate\n"));
    pVia = pBIOSInfo;
    bppIndex = 0;
    supRefTab = NULL;
    needRefresh = pBIOSInfo->Refresh;

    if (needRefresh <= supportRef[0]) {
	refIndex = 0;
    }
    else if (needRefresh >= supportRef[VIA_NUM_REFRESH_RATE - 1]) {
	refIndex = VIA_NUM_REFRESH_RATE - 1;
    }
    else {
	for (refIndex = 0; refIndex < VIA_NUM_REFRESH_RATE; refIndex++) {
	    if (needRefresh < supportRef[refIndex + 1]) {
		break;
	    }
	}
    }

    switch (pBIOSInfo->bitsPerPixel) {
	case 8:
	    bppIndex = 0;
	    break;
	case 16:
	    bppIndex = 1;
	    break;
	case 24:
	case 32:
	    bppIndex = 2;
	    break;
    }

    VGAOUT8(0x3D4, 0x3D);
    switch ((VGAIN8(0x3D5) & 0x70) >> 4) {
	case 0:
	case 1:
	    supRefTab = SDR100[bppIndex][resIndex];
	    break;
	case 2:
	    supRefTab = SDR133[bppIndex][resIndex];
	    break;
	case 3:
	    supRefTab = DDR200[bppIndex][resIndex];
	    break;
	case 4:
	    supRefTab = DDR266[bppIndex][resIndex];
	    break;
    }

    for ( ; refIndex >= 0; refIndex--) {
	if (supRefTab[refIndex]) {
	    needRefresh = supportRef[refIndex];
	    break;
	}
    }

    pBIOSInfo->FoundRefresh = needRefresh;
    return refIndex;
}


Bool VIAFindModeUseBIOSTable(VIABIOSInfoPtr pBIOSInfo)
{
    VIAModeTablePtr pViaModeTable;
    int		    refresh, maxRefresh, needRefresh, refreshMode;
    int		    refIndex;
    int		    i, j, k;
    int		    modeNum, tmp;
    Bool	    setVirtual = FALSE;
    unsigned char	ActiveDevice;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAFindModeUseBIOSTable\n"));
    pViaModeTable = pBIOSInfo->pModeTable;

    i = 0;
    j = VIA_RES_INVALID;

    if (pBIOSInfo->ActiveDevice & VIA_DEVICE_DFP) {
	    VIAGetPanelInfo(pBIOSInfo);
	}
	VIAPostDVI(pBIOSInfo);
    ActiveDevice = VIAGetDeviceDetect(pBIOSInfo);

    if (!pBIOSInfo->ActiveDevice) {
	pBIOSInfo->ActiveDevice = ActiveDevice;
    }
    /* TV + LCD/DVI has no simultaneous, block it */
    if ((pBIOSInfo->ActiveDevice & VIA_DEVICE_TV)
     && (pBIOSInfo->ActiveDevice & (VIA_DEVICE_LCD | VIA_DEVICE_DFP))) {
	pBIOSInfo->ActiveDevice = VIA_DEVICE_TV;
    }

	if ((pBIOSInfo->ActiveDevice & VIA_DEVICE_TV) && (pBIOSInfo->CrtcHDisplay > 1024)
	 && (pBIOSInfo->TVEncoder == VIA_TV3)) {
	for (i = 0; i < pViaModeTable->NumModes; i++) {
	    if ((pViaModeTable->Modes[i].Bpp == pBIOSInfo->bitsPerPixel) &&
		(pViaModeTable->Modes[i].Width == 1024) &&
		(pViaModeTable->Modes[i].Height == 768))
		break;
	}

	if (i == pViaModeTable->NumModes) {
	    ErrorF("\nVIASetModeUseBIOSTable: Cannot find suitable mode!!\n");
	    ErrorF("Mode setting in XF86Config(-4) is not supported!!\n");
	    return FALSE;
	}

	j = VIA_RES_1024X768;
	pBIOSInfo->frameX1 = 1023;
	pBIOSInfo->frameY1 = 767;
	pBIOSInfo->HDisplay = 1024;
	pBIOSInfo->VDisplay = 768;
	pBIOSInfo->CrtcHDisplay = 1024;
	pBIOSInfo->CrtcVDisplay = 768;
    }
	else if ((pBIOSInfo->ActiveDevice & VIA_DEVICE_TV) && (pBIOSInfo->CrtcHDisplay > 800)
	 && (pBIOSInfo->TVEncoder == VIA_TV2PLUS)) {
	for (i = 0; i < pViaModeTable->NumModes; i++) {
	    if ((pViaModeTable->Modes[i].Bpp == pBIOSInfo->bitsPerPixel) &&
		(pViaModeTable->Modes[i].Width == 800) &&
		(pViaModeTable->Modes[i].Height == 600))
		break;
	}

	if (i == pViaModeTable->NumModes) {
	    ErrorF("\nVIASetModeUseBIOSTable: Cannot find suitable mode!!\n");
	    ErrorF("Mode setting in XF86Config(-4) is not supported!!\n");
	    return FALSE;
	}

	j = VIA_RES_800X600;
	pBIOSInfo->frameX1 = 799;
	pBIOSInfo->frameY1 = 599;
	pBIOSInfo->HDisplay = 800;
	pBIOSInfo->VDisplay = 600;
	pBIOSInfo->CrtcHDisplay = 800;
	pBIOSInfo->CrtcVDisplay = 600;
    }
    else {
	for (i = 0; i < pViaModeTable->NumModes; i++) {
	    if ((pViaModeTable->Modes[i].Bpp == pBIOSInfo->bitsPerPixel) &&
		(pViaModeTable->Modes[i].Width == pBIOSInfo->CrtcHDisplay) &&
		(pViaModeTable->Modes[i].Height == pBIOSInfo->CrtcVDisplay))
		break;
	}

	if (i == pViaModeTable->NumModes) {
	    ErrorF("\nVIASetModeUseBIOSTable: Cannot find suitable mode!!\n");
	    ErrorF("Mode setting in XF86Config(-4) is not supported!!\n");
	    return FALSE;
	}

	modeNum = (int)pViaModeTable->Modes[i].Mode;

	if (pBIOSInfo->ActiveDevice & (VIA_DEVICE_DFP | VIA_DEVICE_LCD)) {
	    switch (pBIOSInfo->PanelSize) {
		case VIA_PANEL6X4:
		    pBIOSInfo->panelX = 640;
		    pBIOSInfo->panelY = 480;
		    j = VIA_RES_640X480;
		    break;
		case VIA_PANEL8X6:
		    pBIOSInfo->panelX = 800;
		    pBIOSInfo->panelY = 600;
		    j = VIA_RES_800X600;
		    break;
		case VIA_PANEL10X7:
		    pBIOSInfo->panelX = 1024;
		    pBIOSInfo->panelY = 768;
		    j = VIA_RES_1024X768;
		    break;
		case VIA_PANEL12X7:
		    pBIOSInfo->panelX = 1280;
		    pBIOSInfo->panelY = 768;
		    j = VIA_RES_1280X768;
		    break;
		case VIA_PANEL12X10:
		    pBIOSInfo->panelX = 1280;
		    pBIOSInfo->panelY = 1024;
		    j = VIA_RES_1280X1024;
		    break;
		case VIA_PANEL14X10:
		    pBIOSInfo->panelX = 1400;
		    pBIOSInfo->panelY = 1050;
		    j = VIA_RES_1400X1050;
		    break;
		case VIA_PANEL16X12:
		    pBIOSInfo->panelX = 1600;
		    pBIOSInfo->panelY = 1200;
		    j = VIA_RES_1600X1200;
		    break;
		default:
		    pBIOSInfo->PanelSize = VIA_PANEL10X7;
		    pBIOSInfo->panelX = 1024;
		    pBIOSInfo->panelY = 768;
		    j = VIA_RES_1024X768;
		    break;
	    }

	    /* Find Panel Size Index */
	    for (k = 0; k < VIA_BIOS_NUM_PANEL; k++) {
		if (pViaModeTable->lcdTable[k].fpSize == pBIOSInfo->PanelSize)
		    break;
	    };

	    tmp = 0x1;
	    tmp = tmp << (modeNum & 0xF);
	    if ((CARD16)(tmp) &
		pViaModeTable->lcdTable[k].SuptMode[(modeNum >> 4)]) {
	    }
	    else {
		if ((pBIOSInfo->CrtcHDisplay > pBIOSInfo->panelX) &&
		    (pBIOSInfo->CrtcVDisplay > pBIOSInfo->panelY)) {
		    setVirtual = TRUE;
		    pBIOSInfo->frameX1 = pBIOSInfo->panelX - 1;
		    pBIOSInfo->frameY1 = pBIOSInfo->panelY - 1;
		    pBIOSInfo->HDisplay = pBIOSInfo->panelX;
		    pBIOSInfo->VDisplay = pBIOSInfo->panelY;
		    pBIOSInfo->CrtcHDisplay = pBIOSInfo->panelX;
		    pBIOSInfo->CrtcVDisplay = pBIOSInfo->panelY;
		}
		else {
		    pBIOSInfo->DVIAttach = FALSE;
		    pBIOSInfo->scaleY = FALSE;
		    pBIOSInfo->panelX = 0;
		    pBIOSInfo->panelY = 0;
		}
	    }
	}

	if (setVirtual) {
	    for (i = 0; i < pViaModeTable->NumModes; i++) {
		if ((pViaModeTable->Modes[i].Bpp == pBIOSInfo->bitsPerPixel) &&
		    (pViaModeTable->Modes[i].Width == pBIOSInfo->HDisplay) &&
		    (pViaModeTable->Modes[i].Height == pBIOSInfo->VDisplay))
		    break;
	    }

	    if (i == pViaModeTable->NumModes) {
		ErrorF("\nVIASetModeUseBIOSTable: Cannot find suitable mode!!\n");
		ErrorF("Mode setting in XF86Config(-4) is not supported!!\n");
		return FALSE;
	    }
	}

	switch (pBIOSInfo->CrtcVDisplay) {
	    case 480:
		switch (pBIOSInfo->CrtcHDisplay) {
		    case 640:
			j = VIA_RES_640X480;
			break;
		    case 720:
			j = VIA_RES_720X480;
			break;
		    case 848:
			j = VIA_RES_848X480;
			break;
		    case 856:
			j = VIA_RES_856X480;
			break;
		    default:
			break;
		}
		break;
	    case 512:
		j = VIA_RES_1024X512;
		break;
	    case 576:
		j = VIA_RES_720X576;
		break;
	    case 600:
		j = VIA_RES_800X600;
		break;
	    case 768:
		switch (pBIOSInfo->CrtcHDisplay) {
		    case 1024:
			j = VIA_RES_1024X768;
			break;
		    case 1280:
			j = VIA_RES_1280X768;
				break;
			    default:
				break;
			}
			break;
	    case 864:
		j = VIA_RES_1152X864;
		break;
	    case 960:
		j = VIA_RES_1280X960;
		break;
	    case 1024:
		j = VIA_RES_1280X1024;
		break;
	    case 1050:
		switch (pBIOSInfo->CrtcHDisplay) {
		    case 1440:
			j = VIA_RES_1440X1050;
			break;
		    case 1400:
			j = VIA_RES_1400X1050;
				break;
			    default:
				break;
			}
			break;
	    case 1200:
		j = VIA_RES_1600X1200;
			break;
		    default:
		j = VIA_RES_INVALID;
		break;
	}
    }

    k = 0;

    if (j != VIA_RES_INVALID) {
	if (pBIOSInfo->OptRefresh) {
	    pBIOSInfo->Refresh = pBIOSInfo->OptRefresh;
	    refIndex = VIAFindSupportRefreshRate(pBIOSInfo, j);
	    needRefresh = pBIOSInfo->FoundRefresh;
	    if (refIndex < 0) {
		xf86DrvMsg(pBIOSInfo->scrnIndex, X_ERROR, "Mode setting in XF86Config(-4) is not supported!!\n");
		xf86DrvMsg(pBIOSInfo->scrnIndex, X_ERROR, "Please use lower bpp or resolution.\n");
		return FALSE;
	    }
	}
	else {
	    /* use the monitor information */
	    /* needRefresh = (pBIOSInfo->Clock * 1000) / (pBIOSInfo->HTotal * pBIOSInfo->VTotal); */
	    /* Do rounding */
	    needRefresh = ((pBIOSInfo->Clock * 10000) / (pBIOSInfo->HTotal * pBIOSInfo->VTotal) + 5) / 10;
	    pBIOSInfo->Refresh = needRefresh;
	    refIndex = VIAFindSupportRefreshRate(pBIOSInfo, j);
	    needRefresh = pBIOSInfo->FoundRefresh;
	    if (refIndex < 0) {
		xf86DrvMsg(pBIOSInfo->scrnIndex, X_ERROR, "Mode setting in XF86Config(-4) is not supported!!\n");
		xf86DrvMsg(pBIOSInfo->scrnIndex, X_ERROR, "Please use lower bpp or resolution.\n");
		return FALSE;
	    }
	}

	/* Cannot support refresh > 60 in 1600x1200 because HTotoal don't have enough bits.
	 */
	/*
	if (j == VIA_RES_1600X1200) {
	    needRefresh = 60;
	}
	*/

	refreshMode = 0xFF;
	maxRefresh = 0;

	while (pViaModeTable->refreshTable[j][k].refresh != 0x0) {
	    refresh = pViaModeTable->refreshTable[j][k].refresh;
	    if (refresh != 0xFF) {
		if ((refresh <= needRefresh) && (refresh > maxRefresh)) {
		    refreshMode = k;
		    maxRefresh = refresh;
		}
	    }
	    k++;
	}

	if ((refreshMode == 0xFF) && (needRefresh < 60)) {
	    xf86DrvMsg(pBIOSInfo->scrnIndex, X_ERROR, "VIASetModeUseBIOSTable: Cannot Find suitable refresh!!\n");
	    return FALSE;
	}

	k = refreshMode;
    }

    pBIOSInfo->mode = i;
    pBIOSInfo->resMode = j;
    pBIOSInfo->refresh = k;
    /* pBIOSInfo->widthByQWord = (pBIOSInfo->displayWidth * (pBIOSInfo->bitsPerPixel >> 3)) >> 3; */
    pBIOSInfo->offsetWidthByQWord = (pBIOSInfo->displayWidth * (pBIOSInfo->bitsPerPixel >> 3)) >> 3;
    pBIOSInfo->countWidthByQWord = (pBIOSInfo->CrtcHDisplay * (pBIOSInfo->bitsPerPixel >> 3)) >> 3;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "pBISOInfo->FoundRefresh: %d\n", pBIOSInfo->FoundRefresh));
#ifdef DebugInfo
    xf86Msg(X_DEFAULT, "pBIOSInfo->mode = %d\n", pBIOSInfo->mode);
    xf86Msg(X_DEFAULT, "pBIOSInfo->resMode = %d\n", pBIOSInfo->resMode);
    xf86Msg(X_DEFAULT, "pBIOSInfo->refresh = %d\n", pBIOSInfo->refresh);
    xf86Msg(X_DEFAULT, "pBIOSInfo->offsetWidthByQWord = %d\n", pBIOSInfo->offsetWidthByQWord);
    xf86Msg(X_DEFAULT, "pBIOSInfo->countWidthByQWord = %d\n", pBIOSInfo->countWidthByQWord);
#endif
    return TRUE;
}

void VIASetLCDMode(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    VIAModeTablePtr pViaModeTable;
    CARD8	    modeNum;
    int		    resIdx;
    int		    port, offset, mask, data;
    int		    resMode;
    int		    i, j, k, misc;


    pVia = pBIOSInfo;
    pViaModeTable = pBIOSInfo->pModeTable;
    resMode = pBIOSInfo->resMode;
    modeNum = (CARD8)(pViaModeTable->Modes[pBIOSInfo->mode].Mode);

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIASetLCDMode\n"));
    /* Panel Size 1600x1200 Not Supported Now*/
    if (pBIOSInfo->PanelSize == VIA_PANEL16X12) {
	xfree(pViaModeTable->BIOSDate);
	xfree(pViaModeTable->Modes);
	xf86DrvMsg(pBIOSInfo->scrnIndex, X_ERROR, "VIASetModeUseBIOSTable: Panel Size Not Support!!\n");
    }

    /* Find Panel Size Index */
    for (i = 0; i < VIA_BIOS_NUM_PANEL; i++) {
	if (pViaModeTable->lcdTable[i].fpSize == pBIOSInfo->PanelSize)
	    break;
    };

    if (i == VIA_BIOS_NUM_PANEL) {
	xfree(pViaModeTable->BIOSDate);
	xfree(pViaModeTable->Modes);
	xf86DrvMsg(pBIOSInfo->scrnIndex, X_ERROR, "VIASetModeUseBIOSTable: Panel Size Not Support!!\n");
    }

    switch (pBIOSInfo->PanelSize) {
	case VIA_PANEL6X4:
	case VIA_PANEL8X6:
	    if (pBIOSInfo->IsSecondary) {
		VGAOUT8(0x3c4, 0x16);
		VGAOUT8(0x3c5, 0x08);
	    }
	    else {
		VGAOUT8(0x3c4, 0x16);
		VGAOUT8(0x3c5, 0x48);
	    }
	    VGAOUT8(0x3c4, 0x17);
	    VGAOUT8(0x3c5, 0x1F);
	    VGAOUT8(0x3c4, 0x18);
	    VGAOUT8(0x3c5, 0x0E);
	    break;
	case VIA_PANEL10X7:
	case VIA_PANEL12X7:
	case VIA_PANEL12X10:
	case VIA_PANEL14X10:
	    if (pBIOSInfo->IsSecondary) {
		VGAOUT8(0x3c4, 0x16);
		VGAOUT8(0x3c5, 0x0C);
	    }
	    else {
		VGAOUT8(0x3c4, 0x16);
		VGAOUT8(0x3c5, 0x4C);
	    }
	    VGAOUT8(0x3c4, 0x17);
	    VGAOUT8(0x3c5, 0x1F);
	    VGAOUT8(0x3c4, 0x18);
	    VGAOUT8(0x3c5, 0x0C);
	    break;
	case VIA_PANEL16X12:
	    if (pBIOSInfo->IsSecondary) {
		VGAOUT8(0x3c4, 0x16);
		VGAOUT8(0x3c5, 0x0F);
	    }
	    else {
		VGAOUT8(0x3c4, 0x16);
		VGAOUT8(0x3c5, 0x4F);
	    }
	    VGAOUT8(0x3c4, 0x17);
	    VGAOUT8(0x3c5, 0x1F);
	    VGAOUT8(0x3c4, 0x18);
	    VGAOUT8(0x3c5, 0x0F);
	    break;
	default:
	    if (pBIOSInfo->IsSecondary) {
		VGAOUT8(0x3c4, 0x16);
		VGAOUT8(0x3c5, 0x0C);
	    }
	    else {
		VGAOUT8(0x3c4, 0x16);
		VGAOUT8(0x3c5, 0x4C);
	    }
	    VGAOUT8(0x3c4, 0x17);
	    VGAOUT8(0x3c5, 0x1F);
	    VGAOUT8(0x3c4, 0x18);
	    VGAOUT8(0x3c5, 0x0C);
	    break;
    }

    if (pBIOSInfo->PanelSize == VIA_PANEL12X10) {
	VGAOUT8(0x3d4, 0x89);
	VGAOUT8(0x3d5, 0x07);
    }

    /* LCD Expand Mode Y Scale Flag */
    pBIOSInfo->scaleY = FALSE;

    /* Set LCD InitTb Regs */

    /* Set LClk */
    VGAOUT8(0x3d4, 0x17);
    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

    VGAOUT8(0x3c4, 0x44);
    VGAOUT8(0x3c5, (pViaModeTable->lcdTable[i].InitTb.LCDClk >> 8));
    VGAOUT8(0x3c4, 0x45);
    VGAOUT8(0x3c5, (pViaModeTable->lcdTable[i].InitTb.LCDClk & 0xFF));

    VGAOUT8(0x3d4, 0x17);
    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

    VGAOUT8(0x3c4, 0x40);
    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x04);
    VGAOUT8(0x3c4, 0x40);
    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFB);

    if (!pBIOSInfo->IsSecondary) {
	/* Set VClk */
	VGAOUT8(0x3d4, 0x17);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	VGAOUT8(0x3c4, 0x46);
	VGAOUT8(0x3c5, (pViaModeTable->lcdTable[i].InitTb.VClk >> 8));
	VGAOUT8(0x3c4, 0x47);
	VGAOUT8(0x3c5, (pViaModeTable->lcdTable[i].InitTb.VClk & 0xFF));

	VGAOUT8(0x3d4, 0x17);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	VGAOUT8(0x3c4, 0x40);
	VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x02);
	VGAOUT8(0x3c4, 0x40);
	VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFD);
    }

    /* Use external clock */
    data = VGAIN8(0x3cc) | 0x0C;
    VGAOUT8(0x3c2, data);

    for (j = 0;
	 j < pViaModeTable->lcdTable[i].InitTb.numEntry;
	 j++)
    {
	port = pViaModeTable->lcdTable[i].InitTb.port[j];
	offset = pViaModeTable->lcdTable[i].InitTb.offset[j];
	data = pViaModeTable->lcdTable[i].InitTb.data[j];
	VGAOUT8(0x300+port, offset);
	VGAOUT8(0x301+port, data);
    }

    if ((pBIOSInfo->CrtcHDisplay == pBIOSInfo->panelX) &&
	(pBIOSInfo->CrtcVDisplay == pBIOSInfo->panelY)) {
	/* Set LClk */
	VGAOUT8(0x3d4, 0x17);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	VGAOUT8(0x3c4, 0x44);
	VGAOUT8(0x3c5, (pViaModeTable->lcdTable[i].InitTb.LCDClk >> 8));
	VGAOUT8(0x3c4, 0x45);
	VGAOUT8(0x3c5, (pViaModeTable->lcdTable[i].InitTb.LCDClk & 0xFF));

	VGAOUT8(0x3d4, 0x17);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	VGAOUT8(0x3c4, 0x40);
	VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x04);
	VGAOUT8(0x3c4, 0x40);
	VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFB);

	if (!pBIOSInfo->IsSecondary) {
	    /* Set VClk */
	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	    VGAOUT8(0x3c4, 0x46);
	    VGAOUT8(0x3c5, (pViaModeTable->lcdTable[i].InitTb.VClk >> 8));
	    VGAOUT8(0x3c4, 0x47);
	    VGAOUT8(0x3c5, (pViaModeTable->lcdTable[i].InitTb.VClk & 0xFF));

	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x02);
	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFD);
	}

	/* Use external clock */
	data = VGAIN8(0x3cc) | 0x0C;
	VGAOUT8(0x3c2, data);
    }
    else {
	if (!(pBIOSInfo->Center)) {
	    /* LCD Expand Mode Y Scale Flag */
	    pBIOSInfo->scaleY = TRUE;
	}

	resIdx = VIA_RES_INVALID;

	/* Find MxxxCtr & MxxxExp Index and
	 * HWCursor Y Scale (PanelSize Y / Res. Y) */
	pBIOSInfo->resY = pBIOSInfo->CrtcVDisplay;
	switch (resMode) {
	    case VIA_RES_640X480:
		resIdx = 0;
		break;
	    case VIA_RES_800X600:
		resIdx = 1;
		break;
	    case VIA_RES_1024X768:
		resIdx = 2;
		break;
	    case VIA_RES_1152X864:
		resIdx = 3;
		break;
	    case VIA_RES_1280X768:
	    case VIA_RES_1280X960:
	    case VIA_RES_1280X1024:
		if (pBIOSInfo->PanelSize == VIA_PANEL12X10)
		    resIdx = VIA_RES_INVALID;
		else
		    resIdx = 4;
		break;
	    default:
		resIdx = VIA_RES_INVALID;
		break;
	}

	if ((pBIOSInfo->CrtcHDisplay == 640) &&
	    (pBIOSInfo->CrtcVDisplay == 400))
	    resIdx = 0;

	if (pBIOSInfo->Center) {
	    if (resIdx != VIA_RES_INVALID) {
	    /* Set LCD MxxxCtr Regs */
	    for (j = 0;
		 j < pViaModeTable->lcdTable[i].MCtr[resIdx].numEntry;
		 j++)
	    {
		port = pViaModeTable->lcdTable[i].MCtr[resIdx].port[j];
		offset = pViaModeTable->lcdTable[i].MCtr[resIdx].offset[j];
		data = pViaModeTable->lcdTable[i].MCtr[resIdx].data[j];
		VGAOUT8(0x300+port, offset);
		VGAOUT8(0x301+port, data);
	    }

	    /* Set LClk */
	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	    VGAOUT8(0x3c4, 0x44);
	    VGAOUT8(0x3c5,
		    (pViaModeTable->lcdTable[i].MCtr[resIdx].LCDClk >> 8));
	    VGAOUT8(0x3c4, 0x45);
	    VGAOUT8(0x3c5,
		    (pViaModeTable->lcdTable[i].MCtr[resIdx].LCDClk & 0xFF));

	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x04);
	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFB);

	    if (!pBIOSInfo->IsSecondary) {
		/* Set VClk */
		VGAOUT8(0x3d4, 0x17);
		VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

		VGAOUT8(0x3c4, 0x46);
		VGAOUT8(0x3c5,
			(pViaModeTable->lcdTable[i].MCtr[resIdx].VClk >> 8));
		VGAOUT8(0x3c4, 0x47);
		VGAOUT8(0x3c5,
			(pViaModeTable->lcdTable[i].MCtr[resIdx].VClk & 0xFF));

		VGAOUT8(0x3d4, 0x17);
		VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

		VGAOUT8(0x3c4, 0x40);
		VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x02);
		VGAOUT8(0x3c4, 0x40);
		VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFD);
	    }
	    }

	    for (j = 0; j < pViaModeTable->modeFix.numEntry; j++) {
		if (pViaModeTable->modeFix.reqMode[j] == modeNum) {
		    modeNum = pViaModeTable->modeFix.fixMode[j];
		    break;
		}
	    }

	    for (j = 0; j < pViaModeTable->lcdTable[i].numMPatchDP2Ctr; j++) {
		if (pViaModeTable->lcdTable[i].MPatchDP2Ctr[j].Mode == modeNum)
		    break;
	    }

	    if (j != pViaModeTable->lcdTable[i].numMPatchDP2Ctr) {
		/* Set LCD MPatchDP2Ctr Regs */
		for (k = 0;
		     k < pViaModeTable->lcdTable[i].MPatchDP2Ctr[j].numEntry;
		     k++)
		{
		    port = pViaModeTable->lcdTable[i].MPatchDP2Ctr[j].port[k];
		    offset = pViaModeTable->lcdTable[i].MPatchDP2Ctr[j].offset[k];
		    data = pViaModeTable->lcdTable[i].MPatchDP2Ctr[j].data[k];
		VGAOUT8(0x300+port, offset);
		VGAOUT8(0x301+port, data);
	    }

	    /* Set LClk */
	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	    VGAOUT8(0x3c4, 0x44);
	    VGAOUT8(0x3c5,
		    (pViaModeTable->lcdTable[i].MPatchDP2Ctr[j].LCDClk >> 8));
	    VGAOUT8(0x3c4, 0x45);
	    VGAOUT8(0x3c5,
		    (pViaModeTable->lcdTable[i].MPatchDP2Ctr[j].LCDClk & 0xFF));

	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x04);
	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFB);

	    if (!pBIOSInfo->IsSecondary) {
		/* Set VClk */
		VGAOUT8(0x3d4, 0x17);
		VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

		VGAOUT8(0x3c4, 0x46);
		VGAOUT8(0x3c5,
			(pViaModeTable->lcdTable[i].MPatchDP2Ctr[j].VClk >> 8));
		VGAOUT8(0x3c4, 0x47);
		VGAOUT8(0x3c5,
			(pViaModeTable->lcdTable[i].MPatchDP2Ctr[j].VClk & 0xFF));

		VGAOUT8(0x3d4, 0x17);
		VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

		VGAOUT8(0x3c4, 0x40);
		VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x02);
		VGAOUT8(0x3c4, 0x40);
		VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFD);
	    }
	    }

	    for (j = 0; j < pViaModeTable->lcdTable[i].numMPatchDP1Ctr; j++) {
		if (pViaModeTable->lcdTable[i].MPatchDP1Ctr[j].Mode == modeNum)
		    break;
	    }

	    if ((j != pViaModeTable->lcdTable[i].numMPatchDP1Ctr) &&
		pBIOSInfo->IsSecondary) {
		/* Set LCD MPatchDP1Ctr Regs */
		for (k = 0;
		     k < pViaModeTable->lcdTable[i].MPatchDP1Ctr[j].numEntry;
		     k++)
		{
		    port = pViaModeTable->lcdTable[i].MPatchDP1Ctr[j].port[k];
		    offset = pViaModeTable->lcdTable[i].MPatchDP1Ctr[j].offset[k];
		    data = pViaModeTable->lcdTable[i].MPatchDP1Ctr[j].data[k];
		    VGAOUT8(0x300+port, offset);
		    VGAOUT8(0x301+port, data);
		}
	    }

	    /* Use external clock */
	    data = VGAIN8(0x3cc) | 0x0C;
	    VGAOUT8(0x3c2, data);

	}
	else {
	    if (resIdx != VIA_RES_INVALID) {
		/* Set LCD MxxxExp Regs */
		for (j = 0;
		     j < pViaModeTable->lcdTable[i].MExp[resIdx].numEntry;
		     j++)
		{
		    port = pViaModeTable->lcdTable[i].MExp[resIdx].port[j];
		    offset = pViaModeTable->lcdTable[i].MExp[resIdx].offset[j];
		    data = pViaModeTable->lcdTable[i].MExp[resIdx].data[j];
		    VGAOUT8(0x300+port, offset);
		    VGAOUT8(0x301+port, data);
		}

		/* Set LClk */
		VGAOUT8(0x3d4, 0x17);
		VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

		VGAOUT8(0x3c4, 0x44);
		VGAOUT8(0x3c5,
			(pViaModeTable->lcdTable[i].MExp[resIdx].LCDClk >> 8));
		VGAOUT8(0x3c4, 0x45);
		VGAOUT8(0x3c5,
			(pViaModeTable->lcdTable[i].MExp[resIdx].LCDClk & 0xFF));

		VGAOUT8(0x3d4, 0x17);
		VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

		VGAOUT8(0x3c4, 0x40);
		VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x04);
		VGAOUT8(0x3c4, 0x40);
		VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFB);

		if (!pBIOSInfo->IsSecondary) {
		    /* Set VClk */
		    VGAOUT8(0x3d4, 0x17);
		    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

		    VGAOUT8(0x3c4, 0x46);
		    VGAOUT8(0x3c5,
			    (pViaModeTable->lcdTable[i].MExp[resIdx].VClk >> 8));
		    VGAOUT8(0x3c4, 0x47);
		    VGAOUT8(0x3c5,
			    (pViaModeTable->lcdTable[i].MExp[resIdx].VClk & 0xFF));

		    VGAOUT8(0x3d4, 0x17);
		    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

		    VGAOUT8(0x3c4, 0x40);
		    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x02);
		    VGAOUT8(0x3c4, 0x40);
		    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFD);
		}
	    }

	    for (j = 0; j < pViaModeTable->modeFix.numEntry; j++) {
		if (pViaModeTable->modeFix.reqMode[j] == modeNum) {
		    modeNum = pViaModeTable->modeFix.fixMode[j];
		    break;
		}
	    }

	    for (j = 0; j < pViaModeTable->lcdTable[i].numMPatchDP2Exp; j++) {
		if (pViaModeTable->lcdTable[i].MPatchDP2Exp[j].Mode == modeNum)
		    break;
	    }

	    if (j != pViaModeTable->lcdTable[i].numMPatchDP2Exp) {
		if (pBIOSInfo->CrtcHDisplay == pBIOSInfo->panelX)
		    pBIOSInfo->scaleY = FALSE;
		/* Set LCD MPatchExp Regs */
		for (k = 0;
		     k < pViaModeTable->lcdTable[i].MPatchDP2Exp[j].numEntry;
		     k++)
		{
		    port = pViaModeTable->lcdTable[i].MPatchDP2Exp[j].port[k];
		    offset = pViaModeTable->lcdTable[i].MPatchDP2Exp[j].offset[k];
		    data = pViaModeTable->lcdTable[i].MPatchDP2Exp[j].data[k];
		    VGAOUT8(0x300+port, offset);
		    VGAOUT8(0x301+port, data);
		}

		/* Set LClk */
		VGAOUT8(0x3d4, 0x17);
		VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

		VGAOUT8(0x3c4, 0x44);
		VGAOUT8(0x3c5,
			(pViaModeTable->lcdTable[i].MPatchDP2Exp[j].LCDClk >> 8));
		VGAOUT8(0x3c4, 0x45);
		VGAOUT8(0x3c5,
			(pViaModeTable->lcdTable[i].MPatchDP2Exp[j].LCDClk & 0xFF));

		VGAOUT8(0x3d4, 0x17);
		VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

		VGAOUT8(0x3c4, 0x40);
		VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x04);
		VGAOUT8(0x3c4, 0x40);
		VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFB);

		if (!pBIOSInfo->IsSecondary) {
		    /* Set VClk */
		    VGAOUT8(0x3d4, 0x17);
		    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

		    VGAOUT8(0x3c4, 0x46);
		    VGAOUT8(0x3c5,
			    (pViaModeTable->lcdTable[i].MPatchDP2Exp[j].VClk >> 8));
		    VGAOUT8(0x3c4, 0x47);
		    VGAOUT8(0x3c5,
			    (pViaModeTable->lcdTable[i].MPatchDP2Exp[j].VClk & 0xFF));

		    VGAOUT8(0x3d4, 0x17);
		    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

		    VGAOUT8(0x3c4, 0x40);
		    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x02);
		    VGAOUT8(0x3c4, 0x40);
		    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFD);
		}
	    }

	    for (j = 0; j < pViaModeTable->lcdTable[i].numMPatchDP1Exp; j++) {
		if (pViaModeTable->lcdTable[i].MPatchDP1Exp[j].Mode == modeNum)
		    break;
	    }

	    if ((j != pViaModeTable->lcdTable[i].numMPatchDP1Exp) &&
		pBIOSInfo->IsSecondary) {
		/* Set LCD MPatchDP1Ctr Regs */
		for (k = 0;
		     k < pViaModeTable->lcdTable[i].MPatchDP1Exp[j].numEntry;
		     k++)
		{
		    port = pViaModeTable->lcdTable[i].MPatchDP1Exp[j].port[k];
		    offset = pViaModeTable->lcdTable[i].MPatchDP1Exp[j].offset[k];
		    data = pViaModeTable->lcdTable[i].MPatchDP1Exp[j].data[k];
		    VGAOUT8(0x300+port, offset);
		    VGAOUT8(0x301+port, data);
		}
	    }

	    /* Use external clock */
	    data = VGAIN8(0x3cc) | 0x0C;
	    VGAOUT8(0x3c2, data);
	}
    }

    /* CRT Display Source Bit 6 - 0: CRT, 1: LCD */
    /*
    VGAOUT8(0x3c4, 0x16);
    VGAOUT8(0x3c5, 0x50);
    */

    /* LCD patch 3D5.02 */
    VGAOUT8(0x3d4, 0x01);
    misc = VGAIN8(0x3d5);
    VGAOUT8(0x3d4, 0x02);
    VGAOUT8(0x3d5, misc);

    if (!pBIOSInfo->IsSecondary) {
	/* Enable Simultaneous */
	VGAOUT8(0x3d4, 0x6B);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x08);

	/* Delay 1 Millisecond for CRT+LCD Simultaneous
	 * If there is no delay, LCD will not receive signal when user press
	 * Ctrl+Alt++(or +-) to switch mode. */
	usleep(1);

	/* Enable LCD */
	for (j = 0; j < pViaModeTable->NumPowerOn; j++) {
	    if (pViaModeTable->lcdTable[i].powerSeq ==
		pViaModeTable->powerOn[j].powerSeq)
		break;
	}

	for (k = 0; k < pViaModeTable->powerOn[j].numEntry; k++) {
	    port = pViaModeTable->powerOn[j].port[k];
	    offset = pViaModeTable->powerOn[j].offset[k];
	    mask = pViaModeTable->powerOn[j].mask[k];
	    data = pViaModeTable->powerOn[j].data[k] & mask;
	    VGAOUT8(0x300+port, offset);
	    VGAOUT8(0x301+port, data);
	    usleep(pViaModeTable->powerOn[j].delay[k]);
	}
	usleep(1);

	VGAOUT8(0x3d4, 0x6A);
	VGAOUT8(0x3d5, 0x48);
    }
    else {
	/* Delay 1 Millisecond for CRT+LCD Simultaneous
	 * If there is no delay, LCD will not receive signal when user press
	 * Ctrl+Alt++(or +-) to switch mode. */
	usleep(100);

	/* Enable LCD */
	for (j = 0; j < pViaModeTable->NumPowerOn; j++) {
	    if (pViaModeTable->lcdTable[i].powerSeq ==
		pViaModeTable->powerOn[j].powerSeq)
		break;
	}

	for (k = 0; k < pViaModeTable->powerOn[j].numEntry; k++) {
	    port = pViaModeTable->powerOn[j].port[k];
	    offset = pViaModeTable->powerOn[j].offset[k];
	    mask = pViaModeTable->powerOn[j].mask[k];
	    data = pViaModeTable->powerOn[j].data[k] & mask;
	    VGAOUT8(0x300+port, offset);
	    VGAOUT8(0x301+port, data);
	    usleep(pViaModeTable->powerOn[j].delay[k]);
	}

	usleep(1);

	VGAOUT8(0x3d4, 0x6A);
	VGAOUT8(0x3d5, 0xC8);
	VGAOUT8(0x3d4, 0x6B);
	VGAOUT8(0x3d5, 0);
    }
}

void VIAPreSetTV2Mode(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    VIAModeTablePtr pViaModeTable;
    CARD8	    *TV;
    CARD16	    *DotCrawl, *Patch2;
    int		    tvIndx;
    int		    i, j;
    unsigned char   W_Buffer[9*16];
    unsigned char   W_Other[2];
    int		    w_bytes;
    I2CDevPtr	    dev;
    unsigned char   R_Buffer[1];

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAPreSetTV2Mode\n"));
    pVia = pBIOSInfo;
    pViaModeTable = pBIOSInfo->pModeTable;
    tvIndx = pBIOSInfo->resMode;
    TV = NULL;
    DotCrawl = NULL;
    Patch2 = NULL;
    W_Buffer[0] = 0;
    for (i = 0; i < VIA_BIOS_MAX_NUM_TV_REG; i++) {
	W_Buffer[i+1] = pBIOSInfo->TVRegs[i];
    }

    if (pBIOSInfo->TVType == TVTYPE_PAL) {
	switch (pBIOSInfo->TVVScan) {
	    case VIA_TVNORMAL:
		Patch2 = pViaModeTable->tv2Table[tvIndx].PatchPAL2;
		if (pBIOSInfo->TVOutput == TVOUTPUT_COMPOSITE)
		    TV = pViaModeTable->tv2Table[tvIndx].TVPALC;
		else
		    TV = pViaModeTable->tv2Table[tvIndx].TVPALS;
		break;
	    case VIA_TVOVER:
		Patch2 = pViaModeTable->tv2OverTable[tvIndx].PatchPAL2;
		if (pBIOSInfo->TVOutput == TVOUTPUT_COMPOSITE)
		    TV = pViaModeTable->tv2OverTable[tvIndx].TVPALC;
		else
		    TV = pViaModeTable->tv2OverTable[tvIndx].TVPALS;
		break;
	}
    }
    else {
	switch (pBIOSInfo->TVVScan) {
	    case VIA_TVNORMAL:
		Patch2 = pViaModeTable->tv2Table[tvIndx].PatchNTSC2;
		DotCrawl = pViaModeTable->tv2Table[tvIndx].DotCrawlNTSC;
		if (pBIOSInfo->TVOutput == TVOUTPUT_COMPOSITE)
		    TV = pViaModeTable->tv2Table[tvIndx].TVNTSCC;
		else
		    TV = pViaModeTable->tv2Table[tvIndx].TVNTSCS;
		break;
	    case VIA_TVOVER:
		Patch2 = pViaModeTable->tv2OverTable[tvIndx].PatchNTSC2;
		DotCrawl = pViaModeTable->tv2OverTable[tvIndx].DotCrawlNTSC;
		if (pBIOSInfo->TVOutput == TVOUTPUT_COMPOSITE)
		    TV = pViaModeTable->tv2OverTable[tvIndx].TVNTSCC;
		else
		    TV = pViaModeTable->tv2OverTable[tvIndx].TVNTSCS;
		break;
	}
    }

    /* Set TV mode */
    for (i = 0, j = 0; i < pViaModeTable->tv2MaskTable.numTV; j++) {
	if (pViaModeTable->tv2MaskTable.TV[j] == 0xFF) {
	    W_Buffer[j+1] = TV[j];
	    i++;
	}
    }

    w_bytes = j + 1;

    dev = xf86CreateI2CDevRec();
    dev->DevName = "VT1621";
    dev->SlaveAddr = 0x40;
    dev->pI2CBus = pBIOSInfo->I2C_Port2;

    xf86I2CDevInit(dev);

    xf86I2CWriteRead(dev, W_Buffer,w_bytes, NULL,0);

    /* Turn on all Composite and S-Video output */
    W_Other[0] = 0x0E;
    W_Other[1] = 0;
    xf86I2CWriteRead(dev, W_Other,2, NULL,0);

    if (pBIOSInfo->TVDotCrawl && (pBIOSInfo->TVType == TVTYPE_NTSC)) {
	int numReg = (int)(DotCrawl[0]);

	for (i = 1; i < (numReg + 1); i++) {
	    W_Other[0] = (unsigned char)(DotCrawl[i] & 0xFF);
	    if (W_Other[0] == 0x11) {
		xf86I2CWriteRead(dev, W_Other,1, R_Buffer,1);
		W_Other[1] = R_Buffer[0] | (unsigned char)(DotCrawl[i] >> 8);
	    }
	    else {
		W_Other[1] = (unsigned char)(DotCrawl[i] >> 8);
	    }
	    xf86I2CWriteRead(dev, W_Other,2, NULL,0);
	}
    }

    if (pBIOSInfo->IsSecondary) {
	int numPatch;

	/* Patch as setting 2nd path */
	numPatch = (int)(pViaModeTable->tv2MaskTable.misc2 >> 5);
	for (i = 0; i < numPatch; i++) {
	    W_Other[0] = (unsigned char)(Patch2[i] & 0xFF);
	    W_Other[1] = (unsigned char)(Patch2[i] >> 8);
	    xf86I2CWriteRead(dev, W_Other,2, NULL,0);
	}
    }

    xf86DestroyI2CDevRec(dev,TRUE);
}


void VIAPostSetTV2Mode(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    VIAModeTablePtr pViaModeTable;
    CARD8	    *CRTC1, *CRTC2, *Misc1, *Misc2;
    int		    tvIndx;
    int		    i, j, data;


    pVia = pBIOSInfo;
    pViaModeTable = pBIOSInfo->pModeTable;
    tvIndx = pBIOSInfo->resMode;
    CRTC1 = NULL;
    CRTC2 = NULL;
    Misc1 = NULL;
    Misc2 = NULL;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAPostSetTV2Mode\n"));
    if (pBIOSInfo->TVType == TVTYPE_PAL) {
	switch (pBIOSInfo->TVVScan) {
	    case VIA_TVNORMAL:
		CRTC1 = pViaModeTable->tv2Table[tvIndx].CRTCPAL1;
		switch (pBIOSInfo->bitsPerPixel) {
		    case 8:
			CRTC2 = pViaModeTable->tv2Table[tvIndx].CRTCPAL2_8BPP;
			break;
		    case 16:
			CRTC2 = pViaModeTable->tv2Table[tvIndx].CRTCPAL2_16BPP;
			break;
		    case 24:
		    case 32:
			CRTC2 = pViaModeTable->tv2Table[tvIndx].CRTCPAL2_32BPP;
			break;
		}
		Misc1 = pViaModeTable->tv2Table[tvIndx].MiscPAL1;
		Misc2 = pViaModeTable->tv2Table[tvIndx].MiscPAL2;
		break;
	    case VIA_TVOVER:
		CRTC1 = pViaModeTable->tv2OverTable[tvIndx].CRTCPAL1;
		switch (pBIOSInfo->bitsPerPixel) {
		    case 8:
			CRTC2 = pViaModeTable->tv2OverTable[tvIndx].CRTCPAL2_8BPP;
			break;
		    case 16:
			CRTC2 = pViaModeTable->tv2OverTable[tvIndx].CRTCPAL2_16BPP;
			break;
		    case 24:
		    case 32:
			CRTC2 = pViaModeTable->tv2OverTable[tvIndx].CRTCPAL2_32BPP;
			break;
		}
		Misc1 = pViaModeTable->tv2OverTable[tvIndx].MiscPAL1;
		Misc2 = pViaModeTable->tv2OverTable[tvIndx].MiscPAL2;
		break;
	}
    }
    else {
	switch (pBIOSInfo->TVVScan) {
	    case VIA_TVNORMAL:
		CRTC1 = pViaModeTable->tv2Table[tvIndx].CRTCNTSC1;
		switch (pBIOSInfo->bitsPerPixel) {
		    case 8:
			CRTC2 = pViaModeTable->tv2Table[tvIndx].CRTCNTSC2_8BPP;
			break;
		    case 16:
			CRTC2 = pViaModeTable->tv2Table[tvIndx].CRTCNTSC2_16BPP;
			break;
		    case 24:
		    case 32:
			CRTC2 = pViaModeTable->tv2Table[tvIndx].CRTCNTSC2_32BPP;
			break;
		}
		Misc1 = pViaModeTable->tv2Table[tvIndx].MiscNTSC1;
		Misc2 = pViaModeTable->tv2Table[tvIndx].MiscNTSC2;
		break;
	    case VIA_TVOVER:
		CRTC1 = pViaModeTable->tv2OverTable[tvIndx].CRTCNTSC1;
		switch (pBIOSInfo->bitsPerPixel) {
		    case 8:
			CRTC2 = pViaModeTable->tv2OverTable[tvIndx].CRTCNTSC2_8BPP;
			break;
		    case 16:
			CRTC2 = pViaModeTable->tv2OverTable[tvIndx].CRTCNTSC2_16BPP;
			break;
		    case 24:
		    case 32:
			CRTC2 = pViaModeTable->tv2OverTable[tvIndx].CRTCNTSC2_32BPP;
			break;
		}
		Misc1 = pViaModeTable->tv2OverTable[tvIndx].MiscNTSC1;
		Misc2 = pViaModeTable->tv2OverTable[tvIndx].MiscNTSC2;
		break;
	}
    }

    if (pBIOSInfo->IsSecondary) {
	for (i = 0, j = 0; i < pViaModeTable->tv2MaskTable.numCRTC2; j++) {
	    if (pViaModeTable->tv2MaskTable.CRTC2[j] == 0xFF) {
		VGAOUT8(0x3d4, j + 0x50);
		VGAOUT8(0x3d5, CRTC2[j]);
		i++;
	    }
	}

	j = 3;

	if (pViaModeTable->tv2MaskTable.misc2 & 0x18) {
	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	    VGAOUT8(0x3c4, 0x44);
	    VGAOUT8(0x3c5, Misc2[j++]);
	    VGAOUT8(0x3c4, 0x45);
	    VGAOUT8(0x3c5, Misc2[j++]);

	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x04);
	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFB);

	    /* Use external clock */
	    data = VGAIN8(0x3cc) | 0x0C;
	    VGAOUT8(0x3c2, data);
	}

	VGAOUT8(0x3d4, 0x6A);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0xC0);
	VGAOUT8(0x3d4, 0x6B);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x01);
	VGAOUT8(0x3d4, 0x6C);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x01);

	/* Disable LCD Scaling */
	if (!pBIOSInfo->SAMM || pBIOSInfo->FirstInit) {
		VGAOUT8(0x3d4, 0x79);
		VGAOUT8(0x3d5, 0);
	    }
    }
    else {
	for (i = 0, j = 0; i < pViaModeTable->tv2MaskTable.numCRTC1; j++) {
	    if (pViaModeTable->tv2MaskTable.CRTC1[j] == 0xFF) {
		VGAOUT8(0x3d4, j);
		VGAOUT8(0x3d5, CRTC1[j]);
		i++;
	    }
	}

	j = 0;

	VGAOUT8(0x3d4, 0x33);
	if (Misc1[j] & 0x20) {
	    VGAOUT8(0x3d5, (VGAIN8(0x3d5) | 0x20));
	    j++;
	}
	else {
	    VGAOUT8(0x3d5, (VGAIN8(0x3d5) & 0xdf));
	    j++;
	}

	VGAOUT8(0x3d4, 0x6A);
	VGAOUT8(0x3d5, Misc1[j++]);
	VGAOUT8(0x3d4, 0x6B);
	VGAOUT8(0x3d5, Misc1[j++] | 0x01);
	VGAOUT8(0x3d4, 0x6C);
	VGAOUT8(0x3d5, Misc1[j++] | 0x01);

	if (pViaModeTable->tv2MaskTable.misc1 & 0x30) {
	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	    VGAOUT8(0x3c4, 0x46);
	    VGAOUT8(0x3c5, Misc1[j++]);
	    VGAOUT8(0x3c4, 0x47);
	    VGAOUT8(0x3c5, Misc1[j++]);

	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x02);
	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFD);

	    /* Use external clock */
	    data = VGAIN8(0x3cc) | 0x0C;
	    VGAOUT8(0x3c2, data);
	}
    }
}


void VIAPreSetTV3Mode(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    VIAModeTablePtr pViaModeTable;
    CARD8	    *TV;
    CARD16	    *DotCrawl, *RGB, *YCbCr, *Patch2;
    int		    tvIndx, tvType;
    int		    i, j;
    unsigned char   W_Buffer[9*16];
    unsigned char   W_Other[2];
    int		    w_bytes;
    I2CDevPtr	    dev;
    unsigned char   R_Buffer[1];

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAPreSetTV3Mode\n"));
    pVia = pBIOSInfo;
    pViaModeTable = pBIOSInfo->pModeTable;
    DotCrawl = NULL;
    RGB = NULL;
    YCbCr = NULL;
    Patch2 = NULL;

    /* for hardware limited and New mode support */
    switch (pBIOSInfo->resMode) {
	case VIA_RES_640X480 :
	    tvIndx = VIA_TVRES_640X480;
	    tvType = pBIOSInfo->TVType;
	    break;
	case VIA_RES_800X600 :
	    tvIndx = VIA_TVRES_800X600;
	    tvType = pBIOSInfo->TVType;
	    break;
	case VIA_RES_1024X768 :
	    tvIndx = VIA_TVRES_1024X768;
	    tvType = pBIOSInfo->TVType;
	    break;
	case VIA_RES_848X480 :
	    tvIndx = VIA_TVRES_848X480;
	    tvType = pBIOSInfo->TVType;
	    break;
	case VIA_RES_720X480 :
	    tvIndx = VIA_TVRES_720X480;
	    tvType = TVTYPE_NTSC;
	    break;
	case VIA_RES_720X576 :
	    tvIndx = VIA_TVRES_720X576;
	    tvType = TVTYPE_PAL;
	    break;
	default :
	    tvIndx = pBIOSInfo->resMode;
	    tvType = pBIOSInfo->TVType;
	    break;
    }

    TV = NULL;
    W_Buffer[0] = 0;
    for (i = 0; i < VIA_BIOS_MAX_NUM_TV_REG; i++) {
	W_Buffer[i+1] = pBIOSInfo->TVRegs[i];
    }

    if (tvType == TVTYPE_PAL) {
	switch (pBIOSInfo->TVVScan) {
	    case VIA_TVNORMAL:
		TV = pViaModeTable->tv3Table[tvIndx].TVPAL;
		RGB = pViaModeTable->tv3Table[tvIndx].RGBPAL;
		YCbCr = pViaModeTable->tv3Table[tvIndx].YCbCrPAL;
		Patch2 = pViaModeTable->tv3Table[tvIndx].PatchPAL2;
		break;
	    case VIA_TVOVER:
		TV = pViaModeTable->tv3OverTable[tvIndx].TVPAL;
		RGB = pViaModeTable->tv3OverTable[tvIndx].RGBPAL;
		YCbCr = pViaModeTable->tv3OverTable[tvIndx].YCbCrPAL;
		Patch2 = pViaModeTable->tv3OverTable[tvIndx].PatchPAL2;
		break;
	}
    }
    else {
	switch (pBIOSInfo->TVVScan) {
	    case VIA_TVNORMAL:
		TV = pViaModeTable->tv3Table[tvIndx].TVNTSC;
		DotCrawl = pViaModeTable->tv3Table[tvIndx].DotCrawlNTSC;
		RGB = pViaModeTable->tv3Table[tvIndx].RGBNTSC;
		YCbCr = pViaModeTable->tv3Table[tvIndx].YCbCrNTSC;
		Patch2 = pViaModeTable->tv3Table[tvIndx].PatchNTSC2;
		break;
	    case VIA_TVOVER:
		TV = pViaModeTable->tv3OverTable[tvIndx].TVNTSC;
		DotCrawl = pViaModeTable->tv3OverTable[tvIndx].DotCrawlNTSC;
		RGB = pViaModeTable->tv3OverTable[tvIndx].RGBNTSC;
		YCbCr = pViaModeTable->tv3OverTable[tvIndx].YCbCrNTSC;
		Patch2 = pViaModeTable->tv3OverTable[tvIndx].PatchNTSC2;
		break;
	}
    }

    for (i = 0, j = 0; i < pViaModeTable->tv3MaskTable.numTV; j++) {
	if (pViaModeTable->tv3MaskTable.TV[j] == 0xFF) {
	    W_Buffer[j+1] = TV[j];
	    i++;
	}
    }

    w_bytes = j + 1;

    dev = xf86CreateI2CDevRec();
    dev->DevName = "VT1622";
    dev->SlaveAddr = 0x40;
    dev->pI2CBus = pBIOSInfo->I2C_Port2;

    xf86I2CDevInit(dev);

    /* TV Reset */
    W_Other[0] = 0x1D;
    W_Other[1] = 0;
    xf86I2CWriteRead(dev, W_Other,2, NULL,0);
    W_Other[0] = 0x1D;
    W_Other[1] = 0x80;
    xf86I2CWriteRead(dev, W_Other,2, NULL,0);

    W_Other[0] = 0x30;
    xf86I2CWriteRead(dev, W_Other,1, (W_Buffer + 0x31),26);

    xf86I2CWriteRead(dev, W_Buffer,w_bytes, NULL,0);

    /* Turn on all Composite and S-Video output */
    W_Other[0] = 0x0E;
    W_Other[1] = 0;
    xf86I2CWriteRead(dev, W_Other,2, NULL,0);

    if (pBIOSInfo->TVDotCrawl && (tvType == TVTYPE_NTSC)) {
	int numReg = (int)(DotCrawl[0]);

	for (i = 1; i < (numReg + 1); i++) {
	    W_Other[0] = (unsigned char)(DotCrawl[i] & 0xFF);
	    if (W_Other[0] == 0x11) {
		xf86I2CWriteRead(dev, W_Other,1, R_Buffer,1);
		W_Other[1] = R_Buffer[0] | (unsigned char)(DotCrawl[i] >> 8);
	    }
	    else {
		W_Other[1] = (unsigned char)(DotCrawl[i] >> 8);
	    }
	    xf86I2CWriteRead(dev, W_Other,2, NULL,0);
	}
    }

    if (pBIOSInfo->TVOutput == TVOUTPUT_RGB) {
	int numReg = (int)(RGB[0]);

	for (i = 1; i < (numReg + 1); i++) {
	    W_Other[0] = (unsigned char)(RGB[i] & 0xFF);
	    W_Other[1] = (unsigned char)(RGB[i] >> 8);
	    xf86I2CWriteRead(dev, W_Other,2, NULL,0);
	}

    }

    if (pBIOSInfo->TVOutput == TVOUTPUT_YCBCR) {
	int numReg = (int)(YCbCr[0]);

	for (i = 1; i < (numReg + 1); i++) {
	    W_Other[0] = (unsigned char)(YCbCr[i] & 0xFF);
	    W_Other[1] = (unsigned char)(YCbCr[i] >> 8);
	    xf86I2CWriteRead(dev, W_Other,2, NULL,0);
	}

    }

    if (pBIOSInfo->IsSecondary) {
	int numPatch;

	/* Patch as setting 2nd path */
	numPatch = (int)(pViaModeTable->tv2MaskTable.misc2 >> 5);
	for (i = 0; i < numPatch; i++) {
	    W_Other[0] = (unsigned char)(Patch2[i] & 0xFF);
	    W_Other[1] = (unsigned char)(Patch2[i] >> 8);
	    xf86I2CWriteRead(dev, W_Other,2, NULL,0);
	}
    }

    xf86DestroyI2CDevRec(dev,TRUE);
}


void VIAPostSetTV3Mode(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    VIAModeTablePtr pViaModeTable;
    CARD8	    *CRTC1, *CRTC2, *Misc1, *Misc2;
    int		    tvIndx, tvType;
    int		    i, j, data;


    pVia = pBIOSInfo;
    pViaModeTable = pBIOSInfo->pModeTable;
    CRTC1 = NULL;
    CRTC2 = NULL;
    Misc1 = NULL;
    Misc2 = NULL;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIAPostSetTV3Mode\n"));
    /*	for hardware limited and New mode support */
    switch (pBIOSInfo->resMode) {
	case VIA_RES_640X480 :
	    tvIndx = VIA_TVRES_640X480;
	    tvType = pBIOSInfo->TVType;
	    break;
	case VIA_RES_800X600 :
	    tvIndx = VIA_TVRES_800X600;
	    tvType = pBIOSInfo->TVType;
	    break;
	case VIA_RES_1024X768 :
	    tvIndx = VIA_TVRES_1024X768;
	    tvType = pBIOSInfo->TVType;
	    break;
	case VIA_RES_848X480 :
	    tvIndx = VIA_TVRES_848X480;
	    tvType = pBIOSInfo->TVType;
	    break;
	case VIA_RES_720X480 :
	    tvIndx = VIA_TVRES_720X480;
	    tvType = TVTYPE_NTSC;
	    break;
	case VIA_RES_720X576 :
	    tvIndx = VIA_TVRES_720X576;
	    tvType = TVTYPE_PAL;
	    break;
	default :
	    tvIndx = pBIOSInfo->resMode;
	    tvType = pBIOSInfo->TVType;
	    break;
    }
    if (((tvType == TVTYPE_PAL) && (tvIndx == VIA_TVRES_720X480)) ||
	((tvType == TVTYPE_NTSC) && (tvIndx == VIA_TVRES_720X576))){
	xf86DrvMsg(pBIOSInfo->scrnIndex, X_DEFAULT, "Not TV Support Resolution or Mode!!\n");
	return;
    }

    if (tvType == TVTYPE_PAL) {
	switch (pBIOSInfo->TVVScan) {
	    case VIA_TVNORMAL:
		CRTC1 = pViaModeTable->tv3Table[tvIndx].CRTCPAL1;
		switch (pBIOSInfo->bitsPerPixel) {
		    case 8:
			CRTC2 = pViaModeTable->tv3Table[tvIndx].CRTCPAL2_8BPP;
			break;
		    case 16:
			CRTC2 = pViaModeTable->tv3Table[tvIndx].CRTCPAL2_16BPP;
			break;
		    case 24:
		    case 32:
			CRTC2 = pViaModeTable->tv3Table[tvIndx].CRTCPAL2_32BPP;
			break;
		}
		Misc1 = pViaModeTable->tv3Table[tvIndx].MiscPAL1;
		Misc2 = pViaModeTable->tv3Table[tvIndx].MiscPAL2;
		break;
	    case VIA_TVOVER:
		CRTC1 = pViaModeTable->tv3OverTable[tvIndx].CRTCPAL1;
		switch (pBIOSInfo->bitsPerPixel) {
		    case 8:
			CRTC2 = pViaModeTable->tv3OverTable[tvIndx].CRTCPAL2_8BPP;
			break;
		    case 16:
			CRTC2 = pViaModeTable->tv3OverTable[tvIndx].CRTCPAL2_16BPP;
			break;
		    case 24:
		    case 32:
			CRTC2 = pViaModeTable->tv3OverTable[tvIndx].CRTCPAL2_32BPP;
			break;
		}
		Misc1 = pViaModeTable->tv3OverTable[tvIndx].MiscPAL1;
		Misc2 = pViaModeTable->tv3OverTable[tvIndx].MiscPAL2;
		break;
	}
    }
    else {
	switch (pBIOSInfo->TVVScan) {
	    case VIA_TVNORMAL:
		CRTC1 = pViaModeTable->tv3Table[tvIndx].CRTCNTSC1;
		switch (pBIOSInfo->bitsPerPixel) {
		    case 8:
			CRTC2 = pViaModeTable->tv3Table[tvIndx].CRTCNTSC2_8BPP;
			break;
		    case 16:
			CRTC2 = pViaModeTable->tv3Table[tvIndx].CRTCNTSC2_16BPP;
			break;
		    case 24:
		    case 32:
			CRTC2 = pViaModeTable->tv3Table[tvIndx].CRTCNTSC2_32BPP;
			break;
		}
		Misc1 = pViaModeTable->tv3Table[tvIndx].MiscNTSC1;
		Misc2 = pViaModeTable->tv3Table[tvIndx].MiscNTSC2;
		break;
	    case VIA_TVOVER:
		CRTC1 = pViaModeTable->tv3OverTable[tvIndx].CRTCNTSC1;
		switch (pBIOSInfo->bitsPerPixel) {
		    case 8:
			CRTC2 = pViaModeTable->tv3OverTable[tvIndx].CRTCNTSC2_8BPP;
			break;
		    case 16:
			CRTC2 = pViaModeTable->tv3OverTable[tvIndx].CRTCNTSC2_16BPP;
			break;
		    case 24:
		    case 32:
			CRTC2 = pViaModeTable->tv3OverTable[tvIndx].CRTCNTSC2_32BPP;
			break;
		}
		Misc1 = pViaModeTable->tv3OverTable[tvIndx].MiscNTSC1;
		Misc2 = pViaModeTable->tv3OverTable[tvIndx].MiscNTSC2;
		break;
	}
    }

    if (pBIOSInfo->IsSecondary) {
	for (i = 0, j = 0; i < pViaModeTable->tv3MaskTable.numCRTC2; j++) {
	    if (pViaModeTable->tv3MaskTable.CRTC2[j] == 0xFF) {
		VGAOUT8(0x3d4, j + 0x50);
		VGAOUT8(0x3d5, CRTC2[j]);
		i++;
	    }
	}

	j = 3;

	if (pViaModeTable->tv3MaskTable.misc2 & 0x18) {
	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	    VGAOUT8(0x3c4, 0x44);
	    VGAOUT8(0x3c5, Misc2[j++]);
	    VGAOUT8(0x3c4, 0x45);
	    VGAOUT8(0x3c5, Misc2[j++]);

	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x04);
	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFB);

	    /* Use external clock */
	    data = VGAIN8(0x3cc) | 0x0C;
	    VGAOUT8(0x3c2, data);
	}

	VGAOUT8(0x3d4, 0x6A);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0xC0);
	VGAOUT8(0x3d4, 0x6B);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x01);
	VGAOUT8(0x3d4, 0x6C);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x01);

	/* Disable LCD Scaling */
	if (!pBIOSInfo->SAMM || pBIOSInfo->FirstInit) {
		VGAOUT8(0x3d4, 0x79);
		VGAOUT8(0x3d5, 0);
	    }
    }
    else {
	for (i = 0, j = 0; i < pViaModeTable->tv3MaskTable.numCRTC1; j++) {
	    if (pViaModeTable->tv3MaskTable.CRTC1[j] == 0xFF) {
		VGAOUT8(0x3d4, j);
		VGAOUT8(0x3d5, CRTC1[j]);
		i++;
	    }
	}

	j = 0;

	VGAOUT8(0x3d4, 0x33);
	if (Misc1[j] & 0x20) {
	    VGAOUT8(0x3d5, (VGAIN8(0x3d5) | 0x20));
	    j++;
	}
	else {
	    VGAOUT8(0x3d5, (VGAIN8(0x3d5) & 0xdf));
	    j++;
	}
	VGAOUT8(0x3d4, 0x6A);
	VGAOUT8(0x3d5, Misc1[j++]);
	VGAOUT8(0x3d4, 0x6B);
	VGAOUT8(0x3d5, Misc1[j++] | 0x01);

	if (!pBIOSInfo->A2) {
	    VGAOUT8(0x3d4, 0x6C);
	    VGAOUT8(0x3d5, (Misc1[j++] & 0xE1) | 0x01);
	}
	else {
	    VGAOUT8(0x3d4, 0x6C);
	    VGAOUT8(0x3d5, Misc1[j++] | 0x01);
	}

	if (pViaModeTable->tv3MaskTable.misc1 & 0x30) {
	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	    VGAOUT8(0x3c4, 0x46);
	    VGAOUT8(0x3c5, Misc1[j++]);
	    VGAOUT8(0x3c4, 0x47);
	    VGAOUT8(0x3c5, Misc1[j++]);

	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x02);
	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFD);

	    /* Use external clock */
	    data = VGAIN8(0x3cc) | 0x0C;
	    VGAOUT8(0x3c2, data);
	}
    }
}


void VIADisableExtendedFIFO(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    CARD32	    dwTemp;

    pVia = pBIOSInfo;

    dwTemp = (CARD32)VIAGETREG(0x298);
    dwTemp |= 0x20000000;
    VIASETREG(0x298, dwTemp);

    dwTemp = (CARD32)VIAGETREG(0x230);
    dwTemp &= ~0x00200000;
    VIASETREG(0x230, dwTemp);

    dwTemp = (CARD32)VIAGETREG(0x298);
    dwTemp &= ~0x20000000;
    VIASETREG(0x298, dwTemp);
}


void VIAEnableExtendedFIFO(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    CARD32	    dwTemp;
    CARD8	    bTemp;

    pVia = pBIOSInfo;

    dwTemp = (CARD32)VIAGETREG(0x298);
    dwTemp |= 0x20000000;
    VIASETREG(0x298, dwTemp);

    dwTemp = (CARD32)VIAGETREG(0x230);
    dwTemp |= 0x00200000;
    VIASETREG(0x230, dwTemp);

    dwTemp = (CARD32)VIAGETREG(0x298);
    dwTemp &= ~0x20000000;
    VIASETREG(0x298, dwTemp);

    VGAOUT8(0x3C4, 0x17);
    bTemp = VGAIN8(0x3C5);
    bTemp &= ~0x7F;
    bTemp |= 0x2F;
    VGAOUT8(0x3C5, bTemp);

    VGAOUT8(0x3C4, 0x16);
    bTemp = VGAIN8(0x3C5);
    bTemp &= ~0x3F;
    bTemp |= 0x17;
    VGAOUT8(0x3C5, bTemp);

    VGAOUT8(0x3C4, 0x18);
    bTemp = VGAIN8(0x3C5);
    bTemp &= ~0x3F;
    bTemp |= 0x17;
    bTemp |= 0x40; /* force the preq always higher than treq */
    VGAOUT8(0x3C5, bTemp);
}


Bool VIASetModeUseBIOSTable(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    VIAModeTablePtr pViaModeTable;
    BOOL	    setTV = FALSE;
    int		    mode, resMode, refresh;
    int		    port, offset, mask, data;
    int		    countWidthByQWord, offsetWidthByQWord;
    int		    i, j, k, m, n;
    unsigned char   cr13, cr35, sr1c, sr1d;
    CARD8	    misc;


    pVia = pBIOSInfo;
    mode = pBIOSInfo->mode;
    resMode = pBIOSInfo->resMode;
    refresh = pBIOSInfo->refresh;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIASetModeUseBIOSTable\n"));
    /* Turn off Screen */
    VGAOUT8(0x3d4, 17);
    VGAOUT8(0x3d5, (VGAIN8(0x3d5) & 0x7f));

    /* Clean Second Path Status */
    if (pBIOSInfo->SAMM) {
	if (pBIOSInfo->FirstInit) {
	    VGAOUT8(0x3d4, 0x6A);
	    VGAOUT8(0x3d5, 0x40);
	    VGAOUT8(0x3d4, 0x6B);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x3E);
	    VGAOUT8(0x3d4, 0x6C);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0xFE);
	}
    }
    else {
	VGAOUT8(0x3d4, 0x6A);
	VGAOUT8(0x3d5, 0x0);
	VGAOUT8(0x3d4, 0x6B);
	VGAOUT8(0x3d5, 0x0);
	VGAOUT8(0x3d4, 0x6C);
	VGAOUT8(0x3d5, 0x0);
    }

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "Active Device is %d\n", pBIOSInfo->ActiveDevice));

	if (pBIOSInfo->ActiveDevice & VIA_DEVICE_TV) {
	if ((pBIOSInfo->TVEncoder == VIA_TV3) &&
	    ((resMode == VIA_RES_640X480) || (resMode == VIA_RES_800X600) ||
	     (resMode == VIA_RES_1024X768) || (resMode == VIA_RES_720X480) ||
	     (resMode == VIA_RES_720X576) || (resMode == VIA_RES_848X480)))
	    setTV = TRUE;
	if ((pBIOSInfo->TVEncoder == VIA_TV2PLUS) &&
	    ((resMode == VIA_RES_640X480) || (resMode == VIA_RES_800X600)))
	    setTV = TRUE;
    }

    if (!setTV && !pBIOSInfo->SAMM) {
	if (pBIOSInfo->TVEncoder == VIA_TV3) {
	    I2CDevPtr	    dev;
	    unsigned char   W_Buffer[2];

	    dev = xf86CreateI2CDevRec();
	    dev->DevName = "VT1622";
	    dev->SlaveAddr = 0x40;
	    dev->pI2CBus = pBIOSInfo->I2C_Port2;
	    if (xf86I2CDevInit(dev)) {
		W_Buffer[0] = 0x0E;
		W_Buffer[1] = 0x0F;
		xf86I2CWriteRead(dev, W_Buffer,2, NULL,0);
		xf86DestroyI2CDevRec(dev,TRUE);
	    }
	    else
		xf86DestroyI2CDevRec(dev,TRUE);
	}
	else if (pBIOSInfo->TVEncoder == VIA_TV2PLUS) {
	    I2CDevPtr	    dev;
	    unsigned char   W_Buffer[2];

	    dev = xf86CreateI2CDevRec();
	    dev->DevName = "VT1621";
	    dev->SlaveAddr = 0x40;
	    dev->pI2CBus = pBIOSInfo->I2C_Port2;
	    if (xf86I2CDevInit(dev)) {
		W_Buffer[0] = 0x0E;
		W_Buffer[1] = 0x03;
		xf86I2CWriteRead(dev, W_Buffer,2, NULL,0);
		xf86DestroyI2CDevRec(dev,TRUE);
	    }
	    else
		xf86DestroyI2CDevRec(dev,TRUE);
	}
    }

    pViaModeTable = pBIOSInfo->pModeTable;

    if (setTV) {
	if (pBIOSInfo->TVEncoder == VIA_TV3) {
	    VIAPreSetTV3Mode(pBIOSInfo);
	}
	else if (pBIOSInfo->TVEncoder == VIA_TV2PLUS) {
	    VIAPreSetTV2Mode(pBIOSInfo);
	}
    }

    i = mode;

    /* Get Standard VGA Regs */
    /* Set Sequences regs */
    for (j = 1; j < 5; j++) {
	VGAOUT8(0x3c4, j);
	VGAOUT8(0x3c5, pViaModeTable->Modes[i].stdVgaTable.SR[j]);
    }

    /* Set Misc reg */
    VGAOUT8(0x3c2, pViaModeTable->Modes[i].stdVgaTable.misc);

    /* Set CRTC regs */
    for (j = 0; j < 25; j++) {
	VGAOUT8(0x3d4, j);
	VGAOUT8(0x3d5, pViaModeTable->Modes[i].stdVgaTable.CR[j]);
    }

    /* Set attribute regs */
    for (j = 0; j < 20; j++) {
	misc = VGAIN8(0x3da);
	VGAOUT8(0x3c0, j);
	VGAOUT8(0x3c0, pViaModeTable->Modes[i].stdVgaTable.AR[j]);
    }

    for (j = 0; j < 9; j++) {
	VGAOUT8(0x3ce, j);
	VGAOUT8(0x3cf, pViaModeTable->Modes[i].stdVgaTable.GR[j]);
    }

    /* Unlock Extended regs */
    VGAOUT8(0x3c4, 0x10);
    VGAOUT8(0x3c5, 0x01);

    /* Set Commmon Ext. Regs */
    for (j = 0; j < pViaModeTable->commExtTable.numEntry; j++) {
	port = pViaModeTable->commExtTable.port[j];
	offset = pViaModeTable->commExtTable.offset[j];
	mask = pViaModeTable->commExtTable.mask[j];
	data = pViaModeTable->commExtTable.data[j] & mask;
	VGAOUT8(0x300+port, offset);
	VGAOUT8(0x301+port, data);
    }

    if (pViaModeTable->Modes[i].Mode <= 0x13) {
	/* Set Standard Mode-Spec. Extend Regs */
	for (j = 0; j < pViaModeTable->stdModeExtTable.numEntry; j++) {
	    port = pViaModeTable->stdModeExtTable.port[j];
	    offset = pViaModeTable->stdModeExtTable.offset[j];
	    mask = pViaModeTable->stdModeExtTable.mask[j];
	    data = pViaModeTable->stdModeExtTable.data[j] & mask;
	    VGAOUT8(0x300+port, offset);
	    VGAOUT8(0x301+port, data);
	}
    }
    else {
	/* Set Extended Mode-Spec. Extend Regs */
	for (j = 0; j < pViaModeTable->Modes[i].extModeExtTable.numEntry; j++) {
	    port = pViaModeTable->Modes[i].extModeExtTable.port[j];
	    offset = pViaModeTable->Modes[i].extModeExtTable.offset[j];
	    mask = pViaModeTable->Modes[i].extModeExtTable.mask[j];
	    data = pViaModeTable->Modes[i].extModeExtTable.data[j] & mask;
	    VGAOUT8(0x300+port, offset);
	    VGAOUT8(0x301+port, data);
	}
    }

    j = resMode;

    if ((j != VIA_RES_INVALID) && (refresh != 0xFF) && !setTV) {
	k = refresh;

	VGAOUT8(0x3d4, 0x17);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	VGAOUT8(0x3c4, 0x46);
	VGAOUT8(0x3c5, (pViaModeTable->refreshTable[j][k].VClk >> 8));
	VGAOUT8(0x3c4, 0x47);
	VGAOUT8(0x3c5, (pViaModeTable->refreshTable[j][k].VClk & 0xFF));

	VGAOUT8(0x3d4, 0x17);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	VGAOUT8(0x3c4, 0x40);
	VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x02);
	VGAOUT8(0x3c4, 0x40);
	VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFD);

	m = 0;
	VGAOUT8(0x3d4, 0x0);
	VGAOUT8(0x3d5, pViaModeTable->refreshTable[j][k].CR[m++]);

	for (n = 2; n <= 7; n++) {
	    VGAOUT8(0x3d4, n);
	    VGAOUT8(0x3d5, pViaModeTable->refreshTable[j][k].CR[m++]);
	}

	for (n = 16; n <= 17; n++) {
	    VGAOUT8(0x3d4, n);
	    VGAOUT8(0x3d5, pViaModeTable->refreshTable[j][k].CR[m++]);
	}

	for (n = 21; n <= 22; n++) {
	    VGAOUT8(0x3d4, n);
	    VGAOUT8(0x3d5, pViaModeTable->refreshTable[j][k].CR[m++]);
	}

	/* Use external clock */
	data = VGAIN8(0x3cc) | 0x0C;
	VGAOUT8(0x3c2, data);

    }
    else {
	if (pViaModeTable->Modes[i].VClk != 0) {
	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) & 0x7F);

	    VGAOUT8(0x3c4, 0x46);
	    VGAOUT8(0x3c5, (pViaModeTable->Modes[i].VClk >> 8));
	    VGAOUT8(0x3c4, 0x47);
	    VGAOUT8(0x3c5, (pViaModeTable->Modes[i].VClk & 0xFF));

	    VGAOUT8(0x3d4, 0x17);
	    VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x80);

	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) | 0x02);
	    VGAOUT8(0x3c4, 0x40);
	    VGAOUT8(0x3c5, VGAIN8(0x3c5) & 0xFD);

	    /* Use external clock */
	    data = VGAIN8(0x3cc) | 0x0C;
	    VGAOUT8(0x3c2, data);
	}
    }

#ifdef DBG_PRINT_BIOSMODE
    xf86Msg(X_DEFAULT,
	    "mode: %x resMode: %x refresh: %x\n", mode, resMode, refresh);

    VGAOUT8(0x3c4, 0x46);
    xf86Msg(X_DEFAULT, "Refresh VCk D Reg. Read: %x %x %x %x\n",
	    0x3c4, 0x3c5, 0x46, VGAIN8(0x3c5));
    VGAOUT8(0x3c4, 0x47);
    xf86Msg(X_DEFAULT, "Refresh VCk N Reg. Read: %x %x %x %x\n",
	    0x3c4, 0x3c5, 0x47, VGAIN8(0x3c5));

    VGAOUT8(0x3d4, 0x00);
    xf86Msg(X_DEFAULT, "Refresh Reg. Read: %x %x %x %x\n",
	    0x3d4, 0x3d5, 0x00, VGAIN8(0x3d5));

    for (i = 2; i <= 7; i++) {
	VGAOUT8(0x3d4, i);
	xf86Msg(X_DEFAULT, "Refresh Reg. Read: %x %x %x %x\n",
		0x3d4, 0x3d5, i, VGAIN8(0x3d5));
    }

    for (i = 16; i <= 17; i++) {
	VGAOUT8(0x3d4, i);
	xf86Msg(X_DEFAULT, "Refresh Reg. Read: %x %x %x %x\n",
		0x3d4, 0x3d5, i, VGAIN8(0x3d5));
    }

    for (i = 21; i <= 22; i++) {
	VGAOUT8(0x3d4, i);
	xf86Msg(X_DEFAULT, "Refresh Reg. Read: %x %x %x %x\n",
		0x3d4, 0x3d5, i, VGAIN8(0x3d5));
    }
#endif

    /* Set Quadword offset and counter */
    /* Fix bandwidth problem when using virtual desktop */
    countWidthByQWord = pBIOSInfo->countWidthByQWord;
    offsetWidthByQWord = pBIOSInfo->offsetWidthByQWord;

    VGAOUT8(0x3d4, 0x35);
    cr35 = VGAIN8(0x3d5) & 0x1f;

    cr13 = offsetWidthByQWord & 0xFF;
    cr35 |= (offsetWidthByQWord & 0x700) >> 3;	      /* bit 7:5: offset 10:8 */

    VGAOUT8(0x3c4, 0x1d);
    sr1d = VGAIN8(0x3c5);

    /* Enable Refresh to avoid data lose when enter screen saver */
    /* Refresh disable & 128-bit alignment */
    sr1d = (sr1d & 0xfc) | (countWidthByQWord >> 9);
    sr1c = ((countWidthByQWord >> 1) + 1) & 0xff;
    /* sr1d = ((sr1d & 0xfc) | (widthByQWord >> 8)) | 0x80; */
    /* sr1c = widthByQWord & 0xff; */


    VGAOUT8(0x3d4, 0x13);
    VGAOUT8(0x3d5, cr13);
    VGAOUT8(0x3d4, 0x35);
    VGAOUT8(0x3d5, cr35);
    VGAOUT8(0x3c4, 0x1c);
    VGAOUT8(0x3c5, sr1c);
    VGAOUT8(0x3c4, 0x1d);
    VGAOUT8(0x3c5, sr1d);

    /* Enable MMIO & PCI burst (1 wait state) */
    VGAOUT8(0x3c4, 0x1a);
    data = VGAIN8(0x3c5);
    /*VGAOUT8(0x3c5, data | 0x6B); */
    /*VGAOUT8(0x3c5, data | 0xEF); */
    /* PCI burst 1 wait state */
    /*VGAOUT8(0x3c5, data | 0x04);*/
    VGAOUT8(0x3c5, data | 0x06);

    /* Enable modify CRTC starting address */
    VGAOUT8(0x3d4, 0x11);
    misc = VGAIN8(0x3d5);
    VGAOUT8(0x3d5, misc & 0x7f);

    if (pBIOSInfo->FirstInit) {
	/* Clear Memory */
	memset(pBIOSInfo->FBBase, 0x00, pBIOSInfo->videoRambytes);
    }

    /* Patch for horizontal blanking end bit6 */
    VGAOUT8(0x3d4, 0x02);
    data = VGAIN8(0x3d5); /* Save Blanking Start */

    VGAOUT8(0x3d4, 0x03);
    misc = VGAIN8(0x3d5) & 0x1f; /* Save Blanking End bit[4:0] */

    VGAOUT8(0x3d4, 0x05);
    misc |= ((VGAIN8(0x3d5) & 0x80) >> 2); /* Blanking End bit[5:0] */

    if ((data & 0x3f) > misc) { /* Is Blanking End overflow ? */
	if (data & 0x40) { /* Blanking Start bit6 = ? */
	    VGAOUT8(0x3d4, 0x33); /* bit6 = 1, Blanking End bit6 = 0 */
	    VGAOUT8(0x3d5, (VGAIN8(0x3d5) & 0xdf));
	}
	else {
	    VGAOUT8(0x3d4, 0x33); /* bit6 = 0, Blanking End bit6 = 1 */
	    VGAOUT8(0x3d5, (VGAIN8(0x3d5) | 0x20));
	}
    }
    else {
	if (data & 0x40) { /* Blanking Start bit6 = ? */
	    VGAOUT8(0x3d4, 0x33); /* bit6 = 1, Blanking End bit6 = 1 */
	    VGAOUT8(0x3d5, (VGAIN8(0x3d5) | 0x20));
	}
	else {
	    VGAOUT8(0x3d4, 0x33); /* bit6 = 0, Blanking End bit6 = 0 */
	    VGAOUT8(0x3d5, (VGAIN8(0x3d5) & 0xdf));
	}
    }

    /* LCD Simultaneous Set Mode */
	if (pBIOSInfo->ActiveDevice & (VIA_DEVICE_DFP | VIA_DEVICE_LCD))
	VIASetLCDMode(pBIOSInfo);

    if (setTV) {
	if (pBIOSInfo->TVEncoder == VIA_TV3) {
	    VIAPostSetTV3Mode(pBIOSInfo);
	}
	else if (pBIOSInfo->TVEncoder == VIA_TV2PLUS) {
	    VIAPostSetTV2Mode(pBIOSInfo);
	}
    }

    /* Enable extended FIFO if the resolution > 1024x768
     * or MHS */
    if (pBIOSInfo->CrtcHDisplay > 1024) {
	VIAEnableExtendedFIFO(pBIOSInfo);
    }
    else {
	VIADisableExtendedFIFO(pBIOSInfo);
    }

    /* Fix system crash problem when using VGA BIOS
     * version 00.06. Enable CRT Controller (3D5.17 Hardware Reset) */
    VGAOUT8(0x3d4, 0x17);
    misc = VGAIN8(0x3d5);
    VGAOUT8(0x3d5, (misc | 0x80));

    /* if user doesn't want crt on */
    if (!(pBIOSInfo->ActiveDevice & VIA_DEVICE_CRT1)) {
	VGAOUT8(0x3d4, 0x36);
	VGAOUT8(0x3d5, VGAIN8(0x3d5) | 0x30);
    }

    /* Open Screen */
    misc = VGAIN8(0x3da);
    VGAOUT8(0x3c0, 0x20);

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "-- VIASetModeUseBIOSTable Done!\n"));
    return TRUE;
}

Bool VIASetModeForMHS(VIABIOSInfoPtr pBIOSInfo)
{
    VIABIOSInfoPtr  pVia;
    VIAModeTablePtr pViaModeTable;
    BOOL	    setTV = FALSE;
    int		    mode, resMode, refresh;
    int		    countWidthByQWord, offsetWidthByQWord;
    unsigned char	cr65, cr66, cr67;
    CARD8	    misc;


    pVia = pBIOSInfo;
    mode = pBIOSInfo->mode;
    resMode = pBIOSInfo->resMode;
    refresh = pBIOSInfo->refresh;
    pBIOSInfo->SetTV = FALSE;

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "VIASetModeForMHS\n"));
    /* Turn off Screen */
    VGAOUT8(0x3d4, 17);
    VGAOUT8(0x3d5, (VGAIN8(0x3d5) & 0x7f));

	if (pBIOSInfo->ActiveDevice & VIA_DEVICE_TV) {
	if ((pBIOSInfo->TVEncoder == VIA_TV3) &&
	    ((resMode == VIA_RES_640X480) || (resMode == VIA_RES_800X600) ||
	     (resMode == VIA_RES_1024X768) || (resMode == VIA_RES_720X480) ||
	     (resMode == VIA_RES_720X576))) {
	    setTV = TRUE;
	    pBIOSInfo->SetTV = TRUE;
	}
	if ((pBIOSInfo->TVEncoder == VIA_TV2PLUS) &&
	    ((resMode == VIA_RES_640X480) || (resMode == VIA_RES_800X600))) {
	    setTV = TRUE;
	    pBIOSInfo->SetTV = TRUE;
	}
    }

    pViaModeTable = pBIOSInfo->pModeTable;

    if (setTV) {
	if (pBIOSInfo->TVEncoder == VIA_TV3) {
	    VIAPreSetTV3Mode(pBIOSInfo);
	    VIAPostSetTV3Mode(pBIOSInfo);
	    if (!pBIOSInfo->A2) {
		VGAOUT8(0x3d4, 0x6C);
		VGAOUT8(0x3d5, (VGAIN8(0x3d5) & 0xE1));
	    }
	}
	else if (pBIOSInfo->TVEncoder == VIA_TV2PLUS) {
	    VIAPreSetTV2Mode(pBIOSInfo);
	    VIAPostSetTV2Mode(pBIOSInfo);
	}
    }

	if (pBIOSInfo->ActiveDevice & (VIA_DEVICE_DFP | VIA_DEVICE_LCD)) {
	pBIOSInfo->SetDVI = TRUE;
	VIASetLCDMode(pBIOSInfo);
    }

    /* Set Quadword offset and counter */
    countWidthByQWord = pBIOSInfo->countWidthByQWord;
    offsetWidthByQWord = pBIOSInfo->offsetWidthByQWord;

    VGAOUT8(0x3d4, 0x67);
    cr67 = VGAIN8(0x3d5) & 0xFC;

    cr66 = offsetWidthByQWord & 0xFF;
    cr67 |= (offsetWidthByQWord & 0x300) >> 8;

    VGAOUT8(0x3d4, 0x66);
    VGAOUT8(0x3d5, cr66);
    VGAOUT8(0x3d4, 0x67);
    VGAOUT8(0x3d5, cr67);

    VGAOUT8(0x3d4, 0x67);
    cr67 = VGAIN8(0x3d5) & 0xF3;

    cr67 |= (countWidthByQWord & 0x600) >> 7;
    cr65 = (countWidthByQWord >> 1) & 0xff;

    VGAOUT8(0x3d4, 0x65);
    VGAOUT8(0x3d5, cr65);
    VGAOUT8(0x3d4, 0x67);
    VGAOUT8(0x3d5, cr67);

    VGAOUT8(0x3d4, 0x67);
    cr67 = VGAIN8(0x3d5);

    switch (pBIOSInfo->bitsPerPixel) {
	case 8:
	    cr67 &= 0x3F;
	    VGAOUT8(0x3d5, cr67);
	    break;
	case 16:
	    cr67 &= 0x3F;
	    cr67 |= 0x40;
	    VGAOUT8(0x3d5, cr67);
	    break;
	case 24:
	case 32:
	    cr67 |= 0x80;
	    VGAOUT8(0x3d5, cr67);
	    break;
    }

    /* Open Screen */
    misc = VGAIN8(0x3da);
    VGAOUT8(0x3c0, 0x20);

    DEBUG(xf86DrvMsg(pBIOSInfo->scrnIndex, X_INFO, "-- VIASetModeForMHS Done!\n"));
    return TRUE;
}

Bool VIASavePalette(ScrnInfoPtr pScrn, LOCO *colors) {
    VIAPtr pVia = VIAPTR(pScrn);
    int i, sr1a, sr1b, cr67, cr6a, sr16;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "VIASavePalette\n"));
    VGAOUT8(0x3C4, 0x16);
    sr16 = VGAIN8(0x3C5);
    VGAOUT8(0x3C4, 0x1A);
    sr1a = VGAIN8(0x3C5);
    VGAOUT8(0x3C4, 0x1B);
    sr1b = VGAIN8(0x3C5);
    VGAOUT8(0x3D4, 0x67);
    cr67 = VGAIN8(0x3D5);
    VGAOUT8(0x3D4, 0x6A);
    cr6a = VGAIN8(0x3D5);
    VGAOUT8(0x3C4, 0x16);
    if (pScrn->bitsPerPixel == 8)
	VGAOUT8(0x3C5, sr16 & ~0x80);
    else
	VGAOUT8(0x3C5, sr16 | 0x80);
    if (pVia->IsSecondary) {
	    VGAOUT8(0x3C4, 0x1A);
	VGAOUT8(0x3C5, sr1a | 0x01);
	    VGAOUT8(0x3C4, 0x1B);
	VGAOUT8(0x3C5, sr1b | 0x80);
	VGAOUT8(0x3D4, 0x67);
	VGAOUT8(0x3D5, cr67 & 0x3F);
	VGAOUT8(0x3D4, 0x6A);
	VGAOUT8(0x3D5, cr6a | 0xC0);
    }
    else {
	    VGAOUT8(0x3C4, 0x1A);
	VGAOUT8(0x3C5, sr1a & 0xFE);
	    VGAOUT8(0x3C4, 0x1B);
	VGAOUT8(0x3C5, sr1b | 0x20);
    }

    VGAOUT8(0x3c7, 0);
    for (i = 0; i < 256; i++) {
	colors[i].red = VGAIN8(0x3c9);
	colors[i].green = VGAIN8(0x3c9);
	colors[i].blue = VGAIN8(0x3c9);
	DEBUG(xf86Msg(X_INFO, "%d, %d, %d\n", colors[i].red, colors[i].green, colors[i].blue));
    }
    WaitIdle();

    VGAOUT8(0x3C4, 0x1A);
    VGAOUT8(0x3C5, sr1a);
    VGAOUT8(0x3C4, 0x1B);
    VGAOUT8(0x3C5, sr1b);

    if (pVia->IsSecondary) {
	VGAOUT8(0x3D4, 0x67);
	VGAOUT8(0x3D5, cr67);
	VGAOUT8(0x3D4, 0x6A);
	VGAOUT8(0x3D5, cr6a);
	}
    return TRUE;
}

Bool VIARestorePalette(ScrnInfoPtr pScrn, LOCO *colors) {
    VIAPtr pVia = VIAPTR(pScrn);
    int i, sr1a, sr1b, cr67, cr6a, sr16;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "VIARestorePalette\n"));
    VGAOUT8(0x3C4, 0x16);
    sr16 = VGAIN8(0x3C5);
    VGAOUT8(0x3C4, 0x1A);
    sr1a = VGAIN8(0x3C5);
    VGAOUT8(0x3C4, 0x1B);
    sr1b = VGAIN8(0x3C5);
    VGAOUT8(0x3D4, 0x67);
    cr67 = VGAIN8(0x3D5);
    VGAOUT8(0x3D4, 0x6A);
    cr6a = VGAIN8(0x3D5);
    VGAOUT8(0x3C4, 0x16);
    if (pScrn->bitsPerPixel == 8)
	VGAOUT8(0x3C5, sr16 & ~0x80);
    else
	VGAOUT8(0x3C5, sr16 | 0x80);
    if (pVia->IsSecondary) {
	    VGAOUT8(0x3C4, 0x1A);
	VGAOUT8(0x3C5, sr1a | 0x01);
	    VGAOUT8(0x3C4, 0x1B);
	VGAOUT8(0x3C5, sr1b | 0x80);
	VGAOUT8(0x3D4, 0x67);
	VGAOUT8(0x3D5, cr67 & 0x3F);
	VGAOUT8(0x3D4, 0x6A);
	VGAOUT8(0x3D5, cr6a | 0xC0);
    }
    else {
	    VGAOUT8(0x3C4, 0x1A);
	VGAOUT8(0x3C5, sr1a & 0xFE);
	    VGAOUT8(0x3C4, 0x1B);
	VGAOUT8(0x3C5, sr1b | 0x20);
    }

    VGAOUT8(0x3c8, 0);
    for (i = 0; i < 256; i++) {
	VGAOUT8(0x3c9, colors[i].red);
	VGAOUT8(0x3c9, colors[i].green);
	VGAOUT8(0x3c9, colors[i].blue);
    }
    WaitIdle();
    if (pScrn->bitsPerPixel == 8) {
	    VGAOUT8(0x3C4, 0x16);
	    VGAOUT8(0x3C5, sr16);
	}
    VGAOUT8(0x3C4, 0x1A);
    VGAOUT8(0x3C5, sr1a);
    VGAOUT8(0x3C4, 0x1B);
    VGAOUT8(0x3C5, sr1b);

    if (pVia->IsSecondary) {
	VGAOUT8(0x3D4, 0x67);
	VGAOUT8(0x3D5, cr67);
	VGAOUT8(0x3D4, 0x6A);
	VGAOUT8(0x3D5, cr6a);
	}
    return TRUE;
}

#ifdef CallBIOS
unsigned char BIOS_GetActiveDevice(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    VIABIOSInfoPtr  pBIOSInfo = pVia->pBIOSInfo;
    int RealOff;
    pointer page = NULL;
    unsigned char	ret = 0x0;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS_GetActiveDevice\n"));
    page = xf86Int10AllocPages(pVia->pInt10, 1, &RealOff);
    if (!page)
	return 0xFF;
    pVia->pInt10->ax = 0x4F14;
    pVia->pInt10->bx = 0x0103;
    pVia->pInt10->cx = 0;
    pVia->pInt10->num = 0x10;
    xf86ExecX86int10(pVia->pInt10);

    if ((pVia->pInt10->ax & 0xFF) != 0x4F) {
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "BIOS Get Active Device fail!\n"));
	if (page)
	    xf86Int10FreePages(pVia->pInt10, page, 1);
	return 0xFF;
    }

    if (page)
	xf86Int10FreePages(pVia->pInt10, page, 1);
	if (pBIOSInfo->BIOSMajorVersion < 0x01) {
		if (pVia->pInt10->cx & 0x01)
			ret = VIA_DEVICE_DFP;
		if (pVia->pInt10->cx & 0x02)
			ret |= VIA_DEVICE_CRT1;
		if (pVia->pInt10->cx & 0x04)
			ret |= VIA_DEVICE_TV;
		if (pVia->pInt10->cx & 0x08)
			ret |= VIA_DEVICE_LCD;
	}
	else {
		if (pVia->pInt10->cx & 0x01)
			ret = VIA_DEVICE_CRT1;
		if (pVia->pInt10->cx & 0x02)
			ret |= VIA_DEVICE_LCD;
		if (pVia->pInt10->cx & 0x04)
			ret |= VIA_DEVICE_TV;
		if (pVia->pInt10->cx & 0x20)
			ret |= VIA_DEVICE_DFP;
	}
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Return Value Is: %u\n", ret));
    return ret;
}

/*
 *  Linux Call BIOS Funciton
 *
 * unsigned char BIOS_GetDisplayDeviceInfo(pScrn, *numDevice)
 *
 *     - Get Display Device Information
 *
 * Return Type:	   unsigned int
 *
 * The Definition of Input Value:
 *
 *		   ScrnInfoPtr
 *		   point of int	   0-CRT
 *				   1-DVI
 *				   2-LCD Panel
 *
 * The Definition of Return Value:
 *
 *		   Bit[15:0]	Max. vertical resolution
 */

unsigned int BIOS_GetDisplayDeviceInfo(ScrnInfoPtr pScrn, unsigned char *numDevice)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    int RealOff;
    pointer page = NULL;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS_GetDisplayDeviceInfo\n"));
    page = xf86Int10AllocPages(pVia->pInt10, 1, &RealOff);
    if (!page)
	return 0xFFFF;
    pVia->pInt10->ax = 0x4F14;
    pVia->pInt10->bx = 0x0806;
    pVia->pInt10->cx = *numDevice;
    pVia->pInt10->di = 0x00;
    pVia->pInt10->num = 0x10;
    xf86ExecX86int10(pVia->pInt10);

    if ((pVia->pInt10->ax & 0xFF) != 0x4F) {
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "BIOS Get Device Info fail!\n"));
	if (page)
	    xf86Int10FreePages(pVia->pInt10, page, 1);
	return 0xFFFF;
    }

    if (page)
	xf86Int10FreePages(pVia->pInt10, page, 1);
	*numDevice = pVia->pInt10->cx;
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Vertical Resolution Is: %u\n", pVia->pInt10->di & 0xFFFF));
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Panel ID Is: %u\n", *numDevice));
    return (pVia->pInt10->di & 0xFFFF);
}

/*
 * Linux Call BIOS Funciton
 *
 * unsigned char BIOS_GetDisplayDeviceAttached(pScrn)
 *
 *     - Get Display Device Information
 *
 * Return Type:	   unsigned char
 *
 * The Definition of Input Value:
 *
 *		   ScrnInfoPtr
 *
 * The Definition of Return Value:
 *
 *		   Bit[4]    CRT2
 *		   Bit[3]    DFP
 *		   Bit[2]    TV
 *		   Bit[1]    LCD
 *		   Bit[0]    CRT
 */

unsigned char BIOS_GetDisplayDeviceAttached(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    VIABIOSInfoPtr  pBIOSInfo = pVia->pBIOSInfo;
    int RealOff;
    pointer page = NULL;
    unsigned char	ret = 0;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS_GetDisplayDeviceAttached\n"));
    page = xf86Int10AllocPages(pVia->pInt10, 1, &RealOff);
    if (!page)
	return 0xFF;
    pVia->pInt10->ax = 0x4F14;
    pVia->pInt10->bx = 0x0004;
    pVia->pInt10->cx = 0x00;
    pVia->pInt10->num = 0x10;
    xf86ExecX86int10(pVia->pInt10);

    if ((pVia->pInt10->ax & 0xFF) != 0x4F) {
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "BIOS Get Display Device Attached fail!\n"));
	if (page)
	    xf86Int10FreePages(pVia->pInt10, page, 1);
	return 0xFF;
    }

    if (page)
	xf86Int10FreePages(pVia->pInt10, page, 1);
	if (pBIOSInfo->BIOSMajorVersion < 0x01) {
		if (pVia->pInt10->cx & 0x01)
			ret = VIA_DEVICE_DFP;
		if (pVia->pInt10->cx & 0x02)
			ret |= VIA_DEVICE_CRT1;
		if (pVia->pInt10->cx & 0x04)
			ret |= VIA_DEVICE_TV;
		if (pVia->pInt10->cx & 0x08)
			ret |= VIA_DEVICE_LCD;
	}
	else {
		if (pVia->pInt10->cx & 0x01)
			ret = VIA_DEVICE_CRT1;
		if (pVia->pInt10->cx & 0x02)
			ret |= VIA_DEVICE_LCD;
		if (pVia->pInt10->cx & 0x04)
			ret |= VIA_DEVICE_TV;
		if (pVia->pInt10->cx & 0x20)
			ret |= VIA_DEVICE_DFP;
	}

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Connected Device Is: %d\n", ret));
    return ret;
}

Bool BIOS_GetBIOSDate(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    VIABIOSInfoPtr  pBIOSInfo = pVia->pBIOSInfo;
    int RealOff;
    pointer page = NULL;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS_GetBIOSDate\n"));
    page = xf86Int10AllocPages(pVia->pInt10, 1, &RealOff);
    if (!page)
	return FALSE;
    pVia->pInt10->ax = 0x4F14;
    pVia->pInt10->bx = 0x0100;
    pVia->pInt10->cx = 0;
    pVia->pInt10->dx = 0;
    pVia->pInt10->si = 0;
    pVia->pInt10->num = 0x10;
    xf86ExecX86int10(pVia->pInt10);

    if ((pVia->pInt10->ax & 0xff) != 0x4f) {
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Get BIOS Date fail!\n"));
	if (page)
	    xf86Int10FreePages(pVia->pInt10, page, 1);
	return FALSE;
    }

    pBIOSInfo->BIOSDateYear = ((pVia->pInt10->bx >> 8) - 48) + ((pVia->pInt10->bx & 0xFF) - 48)*10;
    pBIOSInfo->BIOSDateMonth = ((pVia->pInt10->cx >> 8) - 48) + ((pVia->pInt10->cx & 0xFF) - 48)*10;
    pBIOSInfo->BIOSDateDay = ((pVia->pInt10->dx >> 8) - 48) + ((pVia->pInt10->dx & 0xFF) - 48)*10;

    if (page)
	xf86Int10FreePages(pVia->pInt10, page, 1);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS Release Date Is: %d/%d/%d\n"
     , pBIOSInfo->BIOSDateYear + 2000, pBIOSInfo->BIOSDateMonth, pBIOSInfo->BIOSDateDay));
    return TRUE;
}

unsigned int BIOS_GetBIOSVersion(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    int RealOff;
    pointer page = NULL;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS_GetBIOSVersion\n"));
    page = xf86Int10AllocPages(pVia->pInt10, 1, &RealOff);
    if (!page)
	return 0xFFFF;
    pVia->pInt10->ax = 0x4F14;
    pVia->pInt10->bx = 0x0000;
    pVia->pInt10->cx = 0;
    pVia->pInt10->num = 0x10;
    xf86ExecX86int10(pVia->pInt10);

    if ((pVia->pInt10->ax & 0xff) != 0x4f) {
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Get BIOS Version fail!\n"));
	if (page)
	    xf86Int10FreePages(pVia->pInt10, page, 1);
	return 0xFFFF;
    }

    if (page)
	xf86Int10FreePages(pVia->pInt10, page, 1);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Return Value Is: %u\n", pVia->pInt10->bx & 0xFFFF));
    return (pVia->pInt10->bx & 0xFFFF);
}

unsigned char BIOS_GetFlatPanelInfo(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    int RealOff;
    pointer page = NULL;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS_GetFlatPanelInfo\n"));
    page = xf86Int10AllocPages(pVia->pInt10, 1, &RealOff);
    if (!page)
	return 0xFF;
    pVia->pInt10->ax = 0x4F14;
    pVia->pInt10->bx = 0x0006;
    pVia->pInt10->cx = 0x00;
    pVia->pInt10->num = 0x10;
    xf86ExecX86int10(pVia->pInt10);

    if ((pVia->pInt10->ax & 0xFF) != 0x4F) {
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "BIOS Get Flat Panel Info fail!\n"));
	if (page)
	    xf86Int10FreePages(pVia->pInt10, page, 1);
	return 0xFF;
    }

    if (page)
	xf86Int10FreePages(pVia->pInt10, page, 1);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Panel ID Is: %u\n", pVia->pInt10->cx & 0x0F));
    return (pVia->pInt10->cx & 0x0F);
}

unsigned int BIOS_GetTVConfiguration(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    int RealOff;
    pointer page = NULL;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS_GetTVConfiguration\n"));
    page = xf86Int10AllocPages(pVia->pInt10, 1, &RealOff);
    if (!page)
	return 0xFFFF;
    pVia->pInt10->ax = 0x4F14;
    pVia->pInt10->bx = 0x8107;
    pVia->pInt10->cx = 0;
    pVia->pInt10->num = 0x10;
    xf86ExecX86int10(pVia->pInt10);

    if ((pVia->pInt10->ax & 0xff) != 0x4f) {
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Get TV Configuration fail!\n"));
	if (page)
	    xf86Int10FreePages(pVia->pInt10, page, 1);
	return 0xFFFF;
    }

    if (page)
	xf86Int10FreePages(pVia->pInt10, page, 1);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Return Value Is: %u\n", pVia->pInt10->cx & 0xFFFF));
    return (pVia->pInt10->cx & 0xFFFF);
}

unsigned char BIOS_GetTVEncoderType(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    int RealOff;
    pointer page = NULL;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS_GetTVEncoderType\n"));
    page = xf86Int10AllocPages(pVia->pInt10, 1, &RealOff);
    if (!page)
	return 0xFF;
    pVia->pInt10->ax = 0x4F14;
    pVia->pInt10->bx = 0x0000;
    pVia->pInt10->cx = 0;
    pVia->pInt10->num = 0x10;
    xf86ExecX86int10(pVia->pInt10);

    if ((pVia->pInt10->ax & 0xFF) != 0x4F) {
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Get TV Encoder Type fail!\n"));
	if (page)
	    xf86Int10FreePages(pVia->pInt10, page, 1);
	return 0xFF;
    }

    if (page)
	xf86Int10FreePages(pVia->pInt10, page, 1);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Return Value Is: %u\n", pVia->pInt10->cx >> 8));
    return (pVia->pInt10->cx >> 8);
}

Bool BIOS_SetActiveDevice(ScrnInfoPtr pScrn, unsigned short cx)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    int RealOff;
    pointer page = NULL;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS_SetActiveDevice\n"));
    page = xf86Int10AllocPages(pVia->pInt10, 1, &RealOff);
    if (!page)
	return FALSE;
    pVia->pInt10->ax = 0x4F14;
    pVia->pInt10->bx = 0x0103;
    pVia->pInt10->cx = cx;
    pVia->pInt10->num = 0x10;
    xf86ExecX86int10(pVia->pInt10);

    if ((pVia->pInt10->ax & 0xFF) != 0x4F) {
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "BIOS Set Active Device fail!\n"));
	if (page)
	    xf86Int10FreePages(pVia->pInt10, page, 1);
	return FALSE;
    }

    if (page)
	xf86Int10FreePages(pVia->pInt10, page, 1);

    return TRUE;
}
#endif

/* $XFree86$ */

#include "xf86.h"
#include "xf86_ansic.h"
#include "compiler.h"
#include "xf86PciInfo.h"

#include "sis.h"
#include "sis_regs.h"
#include "sis_tv.h"



static	Bool	SIS301TVInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

static	int	XYToRes(int x, int y);
static	int	BppToColor(int bpp);


static Bool
SIS301TVInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	SISPtr pSiS = SISPTR(pScrn);
	SISRegPtr	sisReg = &pSiS->ModeReg;
	int	res, color;
	unsigned short	offset, Threshold_Low, Threshold_High;

	if (mode->Flags & V_INTERLACE)
		res = XYToRes(mode->CrtcHDisplay, mode->CrtcVDisplay*2);
	else
		res = XYToRes(mode->CrtcHDisplay, mode->CrtcVDisplay);
	color = BppToColor(pScrn->bitsPerPixel);
	if ((res == -1) || (color == -1))
		return FALSE;

	switch (pSiS->VBFlags & TV_TYPE)  {
	case TV_PAL:
		if (mode->CrtcHDisplay > 800)
			return FALSE;
		memcpy(sisReg->VBPart1, sis301_PAL[res].VBPart1, 0x29);
		memcpy(sisReg->VBPart2, sis301_PAL[res].VBPart2, 0x46);
		memcpy(sisReg->VBPart3, sis301_PAL[res].VBPart3, 0x3F);
		memcpy(sisReg->VBPart4, sis301_PAL[res].VBPart4, 0x1C);
		break;
	case TV_HIVISION:
		if (mode->CrtcHDisplay > 1280)
			return FALSE;
/*
		memcpy(sisReg->VBPart1, sis301_2[res].VBPart1, 0x29);
		memcpy(sisReg->VBPart2, sis301_2[res].VBPart2, 0x46);
		memcpy(sisReg->VBPart3, sis301_2[res].VBPart3, 0x3F);
		memcpy(sisReg->VBPart4, sis301_2[res].VBPart4, 0x1C);
*/
		break;
	case TV_NTSC:
	default:
		if (mode->CrtcHDisplay > 800)
			return FALSE;
		memcpy(sisReg->VBPart1, sis301_NTSC[res].VBPart1, 0x29);
		memcpy(sisReg->VBPart2, sis301_NTSC[res].VBPart2, 0x46);
		memcpy(sisReg->VBPart3, sis301_NTSC[res].VBPart3, 0x3F);
		memcpy(sisReg->VBPart4, sis301_NTSC[res].VBPart4, 0x1C);
		break;
	}
	sisReg->VBPart1[0x00] &= ~GENMASK(4:0);
	sisReg->VBPart4[0x0d] &= ~GENMASK(4:3);
	switch (pScrn->bitsPerPixel)  {
	case 8:
		sisReg->VBPart4[0x0d] |=0x10;
		sisReg->VBPart1[0] |= 0x10;
		break;
	case 16:
		if (pScrn->depth==15)
			sisReg->VBPart1[0] |= 0x08;
		else
			sisReg->VBPart1[0] |= 0x04;
		break;
	case 24:
		sisReg->VBPart1[0] |= 0x02;
		break;
	case 32:
		sisReg->VBPart1[0] |= 0x01;
		break;
	default:
		return FALSE;
	}
	/* Set TV Interface */
	sisReg->VBPart2[0] &= GENMASK(4:4) | GENMASK(0:0);
	switch (pSiS->VBFlags & TV_INTERFACE)  {
	case TV_SVIDEO:
		sisReg->VBPart2[0] |= 0x08;
		break;
	case TV_SCART:
		sisReg->VBPart2[0] |= 0x02;
		break;
	case TV_AVIDEO:	/* Composite */
	default:
		sisReg->VBPart2[0] |= 0x04;
	}

	offset = pSiS->scrnOffset >> 3;		/* Scrn Offset */
	sisReg->VBPart1[0x07] = GETVAR8(offset);	
	sisReg->VBPart1[0x09] &= 0xF0;
	sisReg->VBPart1[0x09] |= GETBITS(offset, 11:8);

	sisReg->VBPart1[3] = (offset >> 3)+1;	/* CRT2 FIFO Stop */

	(*pSiS->SetThreshold2)(pScrn, mode, &Threshold_Low, &Threshold_High);
	sisReg->VBPart1[1] &= ~GENMASK(4:0);
	sisReg->VBPart1[1] |= GETBITS(Threshold_High, 4:0);
	sisReg->VBPart1[2] &= ~GENMASK(4:0);
	sisReg->VBPart1[2] |= GETBITS(Threshold_Low, 4:0);

	sisReg->sisRegs3D4[0x30] |= 0x01;	/* Set Needed Scratch Regs */
	sisReg->sisRegs3D4[0x31] &= ~0x02;
	sisReg->sisRegs3D4[0x31] |= 0x40;

	return TRUE;
}

static int
BppToColor(int bpp)
{
	if (bpp == 8)	return 0;
	if (bpp == 15)	return 1;
	if (bpp == 16)	return 2;
	if (bpp == 24)	return 3;
	if (bpp == 32)	return 4;
	return -1;
}

static int
XYToRes(int x, int y)
{
	if (x==640 && y==480)  {
		return 0;
	}
	if (x==800 && y==600)  {
		return 1;
	}
	if (x==1024 && y==768)  {
		return 2;
	}
	if (x==1280 && y==1024)  {
		return 3;
	}
	return -1;
}

static void
SIS300_TVPreInit(ScrnInfoPtr pScrn)
{
	SISPtr	pSiS = SISPTR(pScrn);
	int	temp;

	inSISIDXREG(pSiS->RelIO+SROFFSET, 0x38, temp);
	if (!(temp & 0x20))
		return;

	inSISIDXREG(pSiS->RelIO+CROFFSET, 0x30, temp);
	if (temp & 0x10)
		pSiS->VBFlags = CRT2_TV | TV_SCART;
	else if (temp & 0x08)
		pSiS->VBFlags = CRT2_TV | TV_SVIDEO;
	else if (temp & 0x04)
		pSiS->VBFlags = CRT2_TV | TV_AVIDEO;
	else
		return;

	inSISIDXREG(pSiS->RelIO+CROFFSET, 0x31, temp);
	if (temp & 0x01)
		pSiS->VBFlags |= TV_PAL;
	else
		pSiS->VBFlags |= TV_NTSC;
	pSiS->ModeInit2 = SIS301TVInit;
}

void
SISTVPreInit(ScrnInfoPtr pScrn)
{
	SISPtr	pSiS = SISPTR(pScrn);

	switch (pSiS->Chipset)  {
	case PCI_CHIP_SIS300:
	case PCI_CHIP_SIS630:
	case PCI_CHIP_SIS540:
		SIS300_TVPreInit(pScrn);
		break;
	}
}

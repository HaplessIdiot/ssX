/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/alp_dga.c,v 1.3 1998/09/05 06:36:44 dawes Exp $ */

/* (c) Itai Nahshon */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "cir.h"
#include "alp.h"

static Bool
AlpDGAGetParams(int scrnIndex, unsigned long *offset, int *banksize,
				int *memsize)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	CIRPtr pCir = &ALPPTR(pScrn)->CirRec;

	*offset = pCir->FbAddress;
	*banksize = pScrn->videoRam * 1024;
	*memsize = pScrn->videoRam * 1024;

#ifdef ALP_DEBUG
	ErrorF("AlpDGAGetParams %d = 0x%08x, %d, %d\n",
		scrnIndex, *banksize, *memsize);
#endif
	return TRUE;
}

static Bool
AlpDGASetDirect(int scrnIndex, Bool enable)
{
	return TRUE;
}

static Bool
AlpDGASetBank(int scrnIndex, int bank, int flags)
{
	return TRUE;
}

static Bool
AlpDGASetViewport(int scrnIndex, int x, int y, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	(*pScrn->AdjustFrame)(scrnIndex, x, y, 0);
	return TRUE;
}

static  Bool
AlpDGAViewportChanged(int scrnIndex, int n, int flags)
{
	return TRUE;
}

Bool
AlpDGAInit(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	CIRPtr pCir = &ALPPTR(pScrn)->CirRec;
	DGAInfoPtr pDGAInfo;

	pDGAInfo = DGACreateInfoRec();
	if(pDGAInfo == NULL)
		return FALSE;

	pCir->DGAInfo = pDGAInfo;

	pDGAInfo->GetParams = AlpDGAGetParams;
	pDGAInfo->SetDirectMode = AlpDGASetDirect;
	pDGAInfo->SetBank = AlpDGASetBank;
	pDGAInfo->SetViewport = AlpDGASetViewport;
	pDGAInfo->ViewportChanged = AlpDGAViewportChanged;;

	return DGAInit(pScreen, pDGAInfo, 0);
}

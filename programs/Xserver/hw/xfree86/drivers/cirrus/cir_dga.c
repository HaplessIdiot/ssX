/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir_dga.c,v 1.2 1998/08/29 07:39:51 dawes Exp $ */

/* (c) Itai Nahshon */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "cir.h"

static Bool
CIRDGAGetParams(int scrnIndex, unsigned long *offset,
                       int *banksize, int *memsize) {
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    CIRPtr pCir = CIRPTR(pScrn);
   
    *offset = pCir->FbAddress;
    *banksize = pScrn->videoRam * 1024;
    *memsize = pScrn->videoRam * 1024;

#ifdef CIR_DEBUG
    ErrorF("CIRDGAGetParams %d = 0x%08x, %d, %d\n",
           scrnIndex, *banksize, *memsize);
#endif
    return TRUE;
}

static Bool
CIRDGASetDirect(int scrnIndex, Bool enable) {
    return TRUE;
}

static Bool
CIRDGASetBank(int scrnIndex, int bank, int flags) {
    return TRUE;
}

static Bool
CIRDGASetViewport(int scrnIndex, int x, int y, int flags) {
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    (*pScrn->AdjustFrame)(scrnIndex, x, y, 0);
    return TRUE;
}

static  Bool
CIRDGAViewportChanged(int scrnIndex, int n, int flags) {
    return TRUE;
}

Bool
CIRDGAInit(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    CIRPtr pCir = CIRPTR(pScrn);
    DGAInfoPtr pDGAInfo;

    pDGAInfo = DGACreateInfoRec();
    if(pDGAInfo == NULL)
        return FALSE;

    pCir->DGAInfo = pDGAInfo;

    pDGAInfo->GetParams = CIRDGAGetParams;
    pDGAInfo->SetDirectMode = CIRDGASetDirect;
    pDGAInfo->SetBank = CIRDGASetBank;
    pDGAInfo->SetViewport = CIRDGASetViewport;
    pDGAInfo->ViewportChanged = CIRDGAViewportChanged;;

    return DGAInit(pScreen, pDGAInfo, 0);
}

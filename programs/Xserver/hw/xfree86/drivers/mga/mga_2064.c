/* $XFree86$ */

#include "compiler.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "vgaHW.h"

#include "mga_bios.h"
#include "mga_reg.h"
#include "mga.h"

static void MGA2064PreInit(ScrnInfoPtr pScrn);
static void MGA2064Save(ScrnInfoPtr pScrn);
static void MGA2064Restore(ScrnInfoPtr pScrn);
static Bool MGA2064ModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);


void
MGA2064SetupChipFuncs(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);
    MGARamdacPtr MGAdac = &pMga->Dac;

    
    pMga->PreInit = MGA2064PreInit;
    pMga->Save = MGA2064Save;
    pMga->Restore = MGA2064Restore;
    pMga->ModeInit = MGA2064ModeInit;

    switch(pMga->Bios.RamdacType) {
    default:
	MGAdac->PreInit = MGA3026PreInit;
	MGAdac->Save = MGA3026Save;
	MGAdac->Restore = MGA3026Restore;
	MGAdac->Init = MGA3026Init;
	MGAdac->StoreColors = MGA3026StoreColors;
	break;
    }
}


static void
MGA2064PreInit(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);
    MGARamdacPtr MGAdac = &pMga->Dac;

    (*MGAdac->PreInit)(pScrn);
}


static void
MGA2064Save(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);
    MGARamdacPtr MGAdac = &pMga->Dac;
    vgaRegPtr vgaReg = &VGAHWPTR(pScrn)->SavedReg;
    MGARegPtr mgaReg = &pMga->SavedReg;
    int i;

     /* Code is needed to get back to bank zero. */
    outw(0x3DE, 0x0004);  /* get rid of this PIO */

    vgaHWSave(pScrn, vgaReg, TRUE); /* get rid of this PIO */

    for (i = 0; i < 6; i++) {	    /* get rid of this PIO */
	outb(0x3DE, i);
	mgaReg->ExtVga[i] = inb(0x3DF);
    }

    (*MGAdac->Save)(pScrn);
}


static void
MGA2064Restore(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);
    MGARamdacPtr MGAdac = &pMga->Dac;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg = &hwp->SavedReg;
    MGARegPtr mgaReg = &pMga->SavedReg;

    MGAStormSync(pScrn);

    vgaHWProtect(pScrn, TRUE);	
    (*MGAdac->Restore)(pScrn, vgaReg, mgaReg, TRUE);
    vgaHWProtect(pScrn, FALSE);
}

static Bool
MGA2064ModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg;
    MGAPtr pMga;
    MGARegPtr mgaReg;
    MGARamdacPtr MGAdac;

    vgaHWUnlock(hwp);

    /* Initialise the ModeReg values */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;

    pMga = MGAPTR(pScrn);
    MGAdac = &pMga->Dac;

    if(!(*MGAdac->Init)(pScrn, mode))
	return FALSE;

    /* Program the registers */
    vgaHWProtect(pScrn, TRUE);
    vgaReg = &hwp->ModeReg;
    mgaReg = &pMga->ModeReg;
    (*MGAdac->Restore)(pScrn, vgaReg, mgaReg, FALSE);
    MGAStormEngineInit(pScrn);
    vgaHWProtect(pScrn, FALSE);

    return TRUE;
}

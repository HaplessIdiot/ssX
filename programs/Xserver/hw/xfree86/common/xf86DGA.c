/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86DGA.c,v 1.1.2.11 1998/07/18 17:53:22 dawes Exp $ */

/*
 * Copyright (c) 1997-1998 by The XFree86 Project, Inc.
 */

/*
 * This file contains the DGA functions required by the extension.
 * These have been added to avoid the need for the higher level extension
 * code to access the private XFree86 data structures directly.
 */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#ifdef XFreeXDGA
#include "dgaproc.h"
#endif


#ifdef XFreeXDGA
static int DGAGeneration = 0;
static int DGAIndex = -1;
static int DGACount = 0;
static Bool DGAClose(int i, ScreenPtr pScreen);

#define DGAPTR(p) ((DGAPtr)(p)->devPrivates[DGAIndex].ptr)
#endif

DGAInfoPtr
DGACreateInfoRec()
{
#ifdef XFreeXDGA
    return (DGAInfoPtr)xcalloc(1, sizeof(DGAInfoRec));
#else
    return NULL;
#endif
}


void
DGADestroyInfoRec(DGAInfoPtr pDGAInfo)
{
#ifdef XFreeXDGA
    if (!pDGAInfo)
	return;

    xfree(pDGAInfo);
    pDGAInfo = NULL;
#endif
}


Bool
DGAInit(ScreenPtr pScreen, DGAInfoPtr pDGAInfo, int flags)
{
#ifdef XFreeXDGA
    DGAPtr pDGA;
    
    /* Check that the essential DGAInfoRec fields are set */
    if (!pDGAInfo->GetParams || !pDGAInfo->SetDirectMode)
	return FALSE;

    if (serverGeneration != DGAGeneration) {
	if ((DGAIndex = AllocateScreenPrivateIndex()) < 0)
	    return FALSE;
	DGAGeneration = serverGeneration;
    }

    if (!(pScreen->devPrivates[DGAIndex].ptr = xcalloc(sizeof(DGARec), 1)))
	return FALSE;

    pDGA = DGAPTR(pScreen);
    pDGA->pDGAInfo = pDGAInfo;
    pDGA->Flags = 0;
    pDGA->Active = FALSE;
    pDGA->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = DGAClose;
    DGACount++;
    return TRUE;
#else
    return FALSE;
#endif
}


#ifdef XFreeXDGA

static Bool
DGAClose(int i, ScreenPtr pScreen)
{
    DGAPtr pDGA = DGAPTR(pScreen);

    /* This shouldn't happen */
    if (!pDGA)
	return FALSE;

    pScreen->CloseScreen = pDGA->CloseScreen;

    /* The client must free the DGAInfoRec */
    xfree((pointer)pDGA);
    pDGA = NULL;
    if (--DGACount == 0)
	DGAIndex = -1;
    return pScreen->CloseScreen(i, pScreen);
}


/*
 * NOTE: DGAAvailable() should be called before calling any of the other
 * functions below because they don't make any similar checks.  The extension
 * code does this as a matter or course, but code in the common layer that
 * uses some of these functions needs to take care.
 */

Bool
DGAAvailable(int scrnIndex)
{
    DGAPtr pDGA;

    if (DGAIndex < 0)
	return FALSE;

    pDGA = DGAPTR(screenInfo.screens[scrnIndex]);

    if (pDGA && pDGA->pDGAInfo)
	return TRUE;
    else
	return FALSE;
}

Bool
DGAGetParameters(int scrnIndex, CARD32 *offset, CARD32 *bankSize,
		 CARD32 *memSize, CARD32 *width)
{
    int bs, mem;
    unsigned long off;
    DGAPtr pDGA;

    if (DGAIndex < 0)
	return FALSE;

    if ((pDGA = DGAPTR(screenInfo.screens[scrnIndex])) == NULL)
	return FALSE;

    if (pDGA->pDGAInfo->GetParams(scrnIndex, &off, &bs, &mem)) {
	*offset = off;
	*bankSize = bs;
	*memSize = mem;
	*width = xf86Screens[scrnIndex]->displayWidth;
	return TRUE;
    } else {
	return FALSE;
    }
}

Bool
DGAScreenActive(int scrnIndex)
{
    return xf86Screens[scrnIndex]->vtSema;
}

Bool
DGAGetDirectMode(int scrnIndex)
{
    DGAPtr pDGA = DGAPTR(screenInfo.screens[scrnIndex]);

    if (pDGA)
	return pDGA->Active;
    else
	return FALSE;
}

void
DGASetFlags(int scrnIndex, int flags)
{
    DGAPtr pDGA = DGAPTR(screenInfo.screens[scrnIndex]);

    if (pDGA)
	pDGA->Flags = flags;
}

int
DGAGetFlags(int scrnIndex)
{
    DGAPtr pDGA = DGAPTR(screenInfo.screens[scrnIndex]);

    if (pDGA)
	return pDGA->Flags;
    else
	return 0;
}

Bool
DGAEnableDirectMode(int scrnIndex)
{
    DGAPtr pDGA = DGAPTR(screenInfo.screens[scrnIndex]);

    /*
     * Do most of the stuff that usually happens at VT switch, but
     * just for one screen.
     */

    xf86SaveRestoreImage(scrnIndex, SaveImage);
    pDGA->pDGAInfo->SetDirectMode(scrnIndex, TRUE);
    pDGA->Active = TRUE;
    xf86Screens[scrnIndex]->vtSema = FALSE;
    return TRUE;
}

void
DGADisableDirectMode(int scrnIndex)
{
    DGAPtr pDGA = DGAPTR(screenInfo.screens[scrnIndex]);

    /*
     * Do most of the stuff that usually happens at VT switch, but
     * just for one screen.
     */
    xf86Screens[scrnIndex]->vtSema = TRUE;
    pDGA->Active = FALSE;
    pDGA->pDGAInfo->SetDirectMode(scrnIndex, FALSE);
    xf86SaveRestoreImage(scrnIndex, RestoreImage);
}

void
DGAGetViewPortSize(int scrnIndex, int *width, int *height)
{
    *width = xf86Screens[scrnIndex]->currentMode->HDisplay;
    *height = xf86Screens[scrnIndex]->currentMode->VDisplay;
    return;
}

Bool
DGASetViewPort(int scrnIndex, int x, int y)
{
    DGAPtr pDGA = DGAPTR(screenInfo.screens[scrnIndex]);

    if (pDGA->pDGAInfo->SetViewport)
	return pDGA->pDGAInfo->SetViewport(scrnIndex, x, y, 0);
    else
	return FALSE;
}

int
DGAGetVidPage(int scrnIndex)
{
    xf86DrvMsg(scrnIndex, X_WARNING, "DGAGetVidPage not yet implemented\n");
    return -1;
}

Bool
DGASetVidPage(int scrnIndex, int bank)
{
    DGAPtr pDGA = DGAPTR(screenInfo.screens[scrnIndex]);

    if (pDGA->pDGAInfo->SetBank)
	return pDGA->pDGAInfo->SetBank(scrnIndex, bank, 0);
    else
	return FALSE;
}

Bool
DGAViewPortChanged(int scrnIndex, int n)
{
    DGAPtr pDGA = DGAPTR(screenInfo.screens[scrnIndex]);

    if (pDGA->pDGAInfo->ViewportChanged)
	return pDGA->pDGAInfo->ViewportChanged(scrnIndex, n, 0);
    else
	return TRUE;
}

#endif /* XFreeXDGA */

/* $XFree86$ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "xf86str.h"

#include "X.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "xf86str.h"
#include "cursorstr.h"
#include "colormapst.h"
#include "mi.h"
#include "xf86Cursor.h"

int XAACursorScreenIndex = -1;
static unsigned long XAACursorGeneration = 0;

/* sprite functions */

static Bool XAACursorRealizeCursor(ScreenPtr, CursorPtr);
static Bool XAACursorUnrealizeCursor(ScreenPtr, CursorPtr);
static void XAACursorSetCursor(ScreenPtr, CursorPtr, int, int);
static void XAACursorMoveCursor(ScreenPtr, int, int);

static miPointerSpriteFuncRec XAACursorSpriteFuncs = {
   XAACursorRealizeCursor,
   XAACursorUnrealizeCursor,
   XAACursorSetCursor,
   XAACursorMoveCursor
};

/* Screen functions */

static void XAACursorInstallColormap(ColormapPtr);
static void XAACursorRecolorCursor(ScreenPtr, CursorPtr, Bool);
static Bool XAACursorCloseScreen(int, ScreenPtr);
static void XAACursorQueryBestSize(int, unsigned short*, unsigned short*, 
				ScreenPtr);

/* ScrnInfoRec functions */

static Bool XAACursorSwitchMode(int, DisplayModePtr,int);
static Bool XAACursorEnterVT(int, int);
static void XAACursorLeaveVT(int, int);


Bool 
XAAInitCursor(
   ScreenPtr pScreen, 
   XAACursorInfoPtr infoPtr
)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    XAACursorScreenPtr ScreenPriv;
    miPointerScreenPtr PointPriv;
 
    if(XAACursorGeneration != serverGeneration) {
	if((XAACursorScreenIndex = AllocateScreenPrivateIndex()) < 0)
		return FALSE;
	XAACursorGeneration = serverGeneration; 	
    }

    if(!XAAInitHardwareCursor(pScreen, infoPtr))
	return FALSE;

    ScreenPriv = (XAACursorScreenPtr)xcalloc(1, sizeof(XAACursorScreenRec));
    if(!ScreenPriv) return FALSE;

    pScreen->devPrivates[XAACursorScreenIndex].ptr = (pointer)ScreenPriv;

    ScreenPriv->SWCursor = TRUE;
    ScreenPriv->isUp = FALSE;
    ScreenPriv->CurrentCursor = NULL;
    ScreenPriv->CursorInfoPtr = infoPtr;
    ScreenPriv->PalettedCursor = FALSE;
    ScreenPriv->pInstalledMap = NULL;

    ScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = XAACursorCloseScreen;
    ScreenPriv->QueryBestSize = pScreen->QueryBestSize;
    pScreen->QueryBestSize = XAACursorQueryBestSize;
    ScreenPriv->RecolorCursor = pScreen->RecolorCursor;
    pScreen->RecolorCursor = XAACursorRecolorCursor;

    if((infoPtr->pScrn->bitsPerPixel == 8) &&
    		!(infoPtr->Flags & HARDWARE_CURSOR_TRUECOLOR_AT_8BPP)) {
	ScreenPriv->InstallColormap = pScreen->InstallColormap;
	pScreen->InstallColormap = XAACursorInstallColormap;
	ScreenPriv->PalettedCursor = TRUE;
    }

    PointPriv = 
	(miPointerScreenPtr)pScreen->devPrivates[miPointerScreenIndex].ptr;

    ScreenPriv->spriteFuncs = PointPriv->spriteFuncs;
    PointPriv->spriteFuncs = &XAACursorSpriteFuncs; 

    ScreenPriv->SwitchMode = pScrn->SwitchMode;
    pScrn->SwitchMode = XAACursorSwitchMode;
    ScreenPriv->EnterVT = pScrn->EnterVT;
    pScrn->EnterVT = XAACursorEnterVT; 
    ScreenPriv->LeaveVT = pScrn->LeaveVT;
    pScrn->LeaveVT = XAACursorLeaveVT;

    return TRUE;
}

/***** Screen functions *****/

static Bool
XAACursorCloseScreen(int i, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    miPointerScreenPtr PointPriv = 
	(miPointerScreenPtr)pScreen->devPrivates[miPointerScreenIndex].ptr;
    XAACursorScreenPtr ScreenPriv = 
	(XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;

    pScreen->CloseScreen = ScreenPriv->CloseScreen;
    pScreen->QueryBestSize = ScreenPriv->QueryBestSize;
    pScreen->RecolorCursor = ScreenPriv->RecolorCursor;
    if(ScreenPriv->InstallColormap)
	pScreen->InstallColormap = ScreenPriv->InstallColormap;

    PointPriv->spriteFuncs = ScreenPriv->spriteFuncs;

    pScrn->SwitchMode = ScreenPriv->SwitchMode;
    pScrn->EnterVT = ScreenPriv->EnterVT; 
    pScrn->LeaveVT = ScreenPriv->LeaveVT; 

    xfree ((pointer) ScreenPriv);

    return (*pScreen->CloseScreen) (i, pScreen);
}

static void
XAACursorQueryBestSize(
   int class, 
   unsigned short *width,
   unsigned short *height,
   ScreenPtr pScreen
){
    XAACursorScreenPtr ScreenPriv = 
	(XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;

    if(class == CursorShape) {
	*width = ScreenPriv->CursorInfoPtr->MaxWidth;
	*height = ScreenPriv->CursorInfoPtr->MaxHeight;
    } else
	(*ScreenPriv->QueryBestSize)(class, width, height, pScreen);
}
   
static void 
XAACursorInstallColormap(ColormapPtr pMap)
{
    XAACursorScreenPtr ScreenPriv = 	
     (XAACursorScreenPtr)pMap->pScreen->devPrivates[XAACursorScreenIndex].ptr;

    ScreenPriv->pInstalledMap = pMap;
    
    (*ScreenPriv->InstallColormap)(pMap);
}


static void 
XAACursorRecolorCursor(
    ScreenPtr pScreen, 
    CursorPtr pCurs, 
    Bool displayed )
{
    XAACursorScreenPtr ScreenPriv = 
	(XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;

    if(!displayed) return;

    if(ScreenPriv->SWCursor)
	(*ScreenPriv->RecolorCursor)(pScreen, pCurs, displayed);
    else 
	XAARecolorCursor(pScreen, pCurs, displayed);
}

/***** ScrnInfoRec functions *********/

static Bool 
XAACursorSwitchMode(int index, DisplayModePtr mode, int flags)
{
    Bool ret;
    ScreenPtr pScreen = screenInfo.screens[index];
    XAACursorScreenPtr ScreenPriv = 
     (XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;

    if(ScreenPriv->isUp) {
	XAASetCursor(pScreen, 0, ScreenPriv->x, ScreenPriv->y);
	ScreenPriv->isUp = FALSE;
    }

    ret = (*ScreenPriv->SwitchMode)(index, mode, flags);

    if(ScreenPriv->CurrentCursor)
	XAACursorSetCursor(pScreen, ScreenPriv->CurrentCursor, 
					ScreenPriv->x, ScreenPriv->y);

    return ret;
}


static Bool 
XAACursorEnterVT(int index, int flags)
{
    Bool ret;
    ScreenPtr pScreen = screenInfo.screens[index];
    XAACursorScreenPtr ScreenPriv = 
     (XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;

    ret = (*ScreenPriv->EnterVT)(index, flags);

    if(ScreenPriv->CurrentCursor)
	XAACursorSetCursor(pScreen, ScreenPriv->CurrentCursor, 
			ScreenPriv->x, ScreenPriv->y);
    return ret;
}

static void 
XAACursorLeaveVT(int index, int flags)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    XAACursorScreenPtr ScreenPriv = 
     (XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;

    if(ScreenPriv->isUp) {   
	XAASetCursor(pScreen, 0, ScreenPriv->x, ScreenPriv->y);
	ScreenPriv->isUp = FALSE;
    }
    ScreenPriv->SWCursor = TRUE;

    (*ScreenPriv->LeaveVT)(index, flags);
}

    
/****** miPointerSpriteFunctions *******/


static Bool
XAACursorRealizeCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
    XAACursorScreenPtr ScreenPriv = 
	(XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;

    if(pCurs->refcnt <= 1)
	pCurs->devPriv[pScreen->myNum] = NULL;

    return((*ScreenPriv->spriteFuncs->RealizeCursor)(pScreen, pCurs));
}

static Bool
XAACursorUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
    XAACursorScreenPtr ScreenPriv = 
	(XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;
    
    if(pCurs->refcnt <= 1) {
	pointer privData = pCurs->devPriv[pScreen->myNum];
	if(privData) xfree(privData);
	pCurs->devPriv[pScreen->myNum] = NULL;
    }

    return((*ScreenPriv->spriteFuncs->UnrealizeCursor)(pScreen, pCurs));
}


static void
XAACursorSetCursor(ScreenPtr pScreen, CursorPtr pCurs, int x, int y)
{
    XAACursorScreenPtr ScreenPriv = 
	(XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;
    XAACursorInfoPtr infoPtr = ScreenPriv->CursorInfoPtr;

    ScreenPriv->CurrentCursor = pCurs;
    ScreenPriv->x = x;
    ScreenPriv->y = y;

    if(!pCurs) {  /* means we're supposed to remove the cursor */
	if(ScreenPriv->SWCursor)
	    (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, 0, x, y);
	else {
	    if(ScreenPriv->isUp) XAASetCursor(pScreen, 0, x, y);
	    ScreenPriv->isUp = FALSE;
	}
	return;
    } 

    ScreenPriv->HotX = pCurs->bits->xhot;
    ScreenPriv->HotY = pCurs->bits->yhot;
	
    if( infoPtr->pScrn->vtSema &&
	(pCurs->bits->height <= infoPtr->MaxHeight) &&
	(pCurs->bits->width <= infoPtr->MaxWidth) &&
	(!infoPtr->UseHWCursor || (*infoPtr->UseHWCursor)(pScreen, pCurs))){

	if(ScreenPriv->SWCursor) /* remove the SW cursor */
	      (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, 0, x, y);

	XAASetCursor(pScreen, pCurs, x, y);
	ScreenPriv->SWCursor = FALSE;
	ScreenPriv->isUp = TRUE;
	return; 
    }

    if(ScreenPriv->isUp) {
	/* remove the HW cursor */
	XAASetCursor(pScreen, 0, x, y);
	ScreenPriv->isUp = FALSE;
    }

    ScreenPriv->SWCursor = TRUE;

    (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, pCurs, x, y);
}

static void
XAACursorMoveCursor(ScreenPtr pScreen, int x, int y)
{
    XAACursorScreenPtr ScreenPriv = 
	(XAACursorScreenPtr)pScreen->devPrivates[XAACursorScreenIndex].ptr;

    ScreenPriv->x = x;
    ScreenPriv->y = y;

    if(ScreenPriv->SWCursor)
	(*ScreenPriv->spriteFuncs->MoveCursor)(pScreen, x, y);
    else if(ScreenPriv->isUp)	
	XAAMoveCursor(pScreen, x, y); 
}



XAACursorInfoPtr 
XAACreateCursorInfoRec(void)
{
    return((XAACursorInfoPtr)xcalloc(1,sizeof(XAACursorInfoRec)));
}


void 
XAADestroyCursorInfoRec(XAACursorInfoPtr infoPtr)
{
    if(infoPtr) xfree(infoPtr);
}









/* $TOG: panoramiX.c /main/5 1998/02/27 12:22:22 barstow $ */
/****************************************************************
*                                                               *
*    Copyright (c) Digital Equipment Corporation, 1991, 1997    *
*                                                               *
*   All Rights Reserved.  Unpublished rights  reserved  under   *
*   the copyright laws of the United States.                    *
*                                                               *
*   The software contained on this media  is  proprietary  to   *
*   and  embodies  the  confidential  technology  of  Digital   *
*   Equipment Corporation.  Possession, use,  duplication  or   *
*   dissemination of the software and media is authorized only  *
*   pursuant to a valid written license from Digital Equipment  *
*   Corporation.                                                *
*                                                               *
*   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
*   by the U.S. Government is subject to restrictions  as  set  *
*   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
*   or  in  FAR 52.227-19, as applicable.                       *
*                                                               *
*****************************************************************/
/* $XFree86: xc/programs/Xserver/Xext/panoramiX.c,v 3.7 1999/06/27 14:07:39 dawes Exp $ */

#define NEED_REPLIES
#include <stdio.h>
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "cursor.h"
#include "cursorstr.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "gc.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "window.h"
#include "windowstr.h"
#include "pixmapstr.h"
#if 0
#include <sys/workstation.h>
#include <X11/Xserver/ws.h> 
#endif
#include "panoramiX.h"
#include "panoramiXproto.h"
#include "panoramiXsrv.h"
#include "globals.h"

static unsigned char PanoramiXReqCode = 0;
/*
 *	PanoramiX data declarations
 */

int 		PanoramiXPixWidth;
int 		PanoramiXPixHeight;
int 		PanoramiXNumScreens;

PanoramiXData 	*panoramiXdataPtr;
PanoramiXWindow *PanoramiXWinRoot;
PanoramiXGC 	*PanoramiXGCRoot;
PanoramiXCmap 	*PanoramiXCmapRoot;
PanoramiXPmap 	*PanoramiXPmapRoot;

/*
 * Free list flags added as pre-test before running through list to free ids
 */
Bool PanoramiXWinRootFreeable = FALSE;
Bool PanoramiXGCRootFreeable = FALSE;
Bool PanoramiXCmapRootFreeable = FALSE;
Bool PanoramiXPmapRootFreeable = FALSE;

PanoramiXEdge   panoramiXEdgePtr[MAXSCREENS];
RegionRec   	PanoramiXScreenRegion[MAXSCREENS];

int		PanoramiXNumDepths;
DepthPtr	PanoramiXDepths;
int		PanoramiXNumVisuals;
VisualPtr	PanoramiXVisuals;
/* We support at most 256 visuals */
XID		PanoramiXVisualTable[256][MAXSCREENS];


int (* SavedProcVector[256]) ();
ScreenInfo *GlobalScrInfo;

static int panoramiXGeneration;
static int ProcPanoramiXDispatch(); 
/*
 *	Function prototypes
 */

static void locate_neighbors(int);
static void PanoramiXResetProc(ExtensionEntry*);

/*
 *	External references for data variables
 */

extern int SProcPanoramiXDispatch();
extern WindowPtr *WindowTable;
#if 0
extern ScreenArgsRec screenArgs[MAXSCREENS];
#endif
extern int defaultBackingStore;
extern char *ConnectionInfo;
extern int connBlockScreenStart;
extern int (* ProcVector[256]) ();
extern xConnSetupPrefix connSetupPrefix;

/*
 *	Server dispatcher function replacements
 */

int PanoramiXCreateWindow(), 	PanoramiXChangeWindowAttributes();
int PanoramiXDestroyWindow(),	PanoramiXDestroySubwindows();
int PanoramiXChangeSaveSet(), 	PanoramiXReparentWindow();
int PanoramiXMapWindow(), 	PanoramiXMapSubwindows();
int PanoramiXUnmapWindow(), 	PanoramiXUnmapSubwindows();
int PanoramiXConfigureWindow(), PanoramiXCirculateWindow();
int PanoramiXGetGeometry(), 	PanoramiXChangeProperty();
int PanoramiXDeleteProperty(), 	PanoramiXSendEvent();
int PanoramiXCreatePixmap(), 	PanoramiXFreePixmap();
int PanoramiXCreateGC(), 	PanoramiXChangeGC();
int PanoramiXCopyGC();
int PanoramiXSetDashes(), 	PanoramiXSetClipRectangles();
int PanoramiXFreeGC(), 		PanoramiXClearToBackground();
int PanoramiXCopyArea(),	PanoramiXCopyPlane();
int PanoramiXPolyPoint(),	PanoramiXPolyLine();
int PanoramiXPolySegment(),	PanoramiXPolyRectangle();
int PanoramiXPolyArc(),		PanoramiXFillPoly();
int PanoramiXPolyFillArc(),	PanoramiXPolyFillRectangle();
int PanoramiXPutImage(), 	PanoramiXGetImage();
int PanoramiXPolyText8(),	PanoramiXPolyText16();	
int PanoramiXImageText8(),	PanoramiXImageText16();
int PanoramiXCreateColormap(),	PanoramiXFreeColormap();
int PanoramiXInstallColormap(),	PanoramiXUninstallColormap();
int PanoramiXAllocColor(),	PanoramiXAllocNamedColor();
int PanoramiXAllocColorCells();
int PanoramiXFreeColors(), 	PanoramiXStoreColors();

static int PanoramiXGCIndex = -1;
static int PanoramiXScreenIndex = -1;

typedef struct {
  DDXPointRec clipOrg;
  DDXPointRec patOrg;
  GCFuncs *wrapFuncs;
} PanoramiXGCRec, *PanoramiXGCPtr;

typedef struct {
  CreateGCProcPtr	CreateGC;
  CloseScreenProcPtr	CloseScreen;
} PanoramiXScreenRec, *PanoramiXScreenPtr;

static void XineramaValidateGC(GCPtr, unsigned long, DrawablePtr);
static void XineramaChangeGC(GCPtr, unsigned long);
static void XineramaCopyGC(GCPtr, unsigned long, GCPtr);
static void XineramaDestroyGC(GCPtr);
static void XineramaChangeClip(GCPtr, int, pointer, int);
static void XineramaDestroyClip(GCPtr);
static void XineramaCopyClip(GCPtr, GCPtr);

GCFuncs XineramaGCFuncs = {
    XineramaValidateGC, XineramaChangeGC, XineramaCopyGC, XineramaDestroyGC,
    XineramaChangeClip, XineramaDestroyClip, XineramaCopyClip
};

#define Xinerama_GC_FUNC_PROLOGUE(pGC)\
    PanoramiXGCPtr  pGCPriv = \
		(PanoramiXGCPtr) (pGC)->devPrivates[PanoramiXGCIndex].ptr;\
    (pGC)->funcs = pGCPriv->wrapFuncs;

#define Xinerama_GC_FUNC_EPILOGUE(pGC)\
    pGCPriv->wrapFuncs = (pGC)->funcs;\
    (pGC)->funcs = &XineramaGCFuncs;


static Bool
XineramaCloseScreen (int i, ScreenPtr pScreen)
{
    PanoramiXScreenPtr pScreenPriv = 
        (PanoramiXScreenPtr) pScreen->devPrivates[PanoramiXScreenIndex].ptr;

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->CreateGC = pScreenPriv->CreateGC;

    xfree ((pointer) pScreenPriv);

    return (*pScreen->CloseScreen) (i, pScreen);
}

Bool
XineramaCreateGC(GCPtr pGC)
{
    ScreenPtr pScreen = pGC->pScreen;
    PanoramiXScreenPtr pScreenPriv = 
        (PanoramiXScreenPtr) pScreen->devPrivates[PanoramiXScreenIndex].ptr;
    Bool ret;

    pScreen->CreateGC = pScreenPriv->CreateGC;
    if((ret = (*pScreen->CreateGC)(pGC))) {
	PanoramiXGCPtr pGCPriv = 
		(PanoramiXGCPtr) pGC->devPrivates[PanoramiXGCIndex].ptr;

	pGCPriv->wrapFuncs = pGC->funcs;
        pGC->funcs = &XineramaGCFuncs;

	pGCPriv->clipOrg.x = pGC->clipOrg.x; 
	pGCPriv->clipOrg.y = pGC->clipOrg.y;
	pGCPriv->patOrg.x = pGC->patOrg.x;
	pGCPriv->patOrg.y = pGC->patOrg.y;
    }
    pScreen->CreateGC = XineramaCreateGC;

    return ret;
}

static void
XineramaValidateGC(
   GCPtr         pGC,
   unsigned long changes,
   DrawablePtr   pDraw 
){
    Xinerama_GC_FUNC_PROLOGUE (pGC);

    if((pDraw->type == DRAWABLE_WINDOW) && !(((WindowPtr)pDraw)->parent)) {
	/* the root window */
	int x_off = panoramiXdataPtr[pGC->pScreen->myNum].x;
	int y_off = panoramiXdataPtr[pGC->pScreen->myNum].y;
	int new_val;

	new_val = pGCPriv->clipOrg.x - x_off;
	if(pGC->clipOrg.x != new_val) {
	    pGC->clipOrg.x = new_val;
	    changes |= GCClipXOrigin;
	}
	new_val = pGCPriv->clipOrg.y - y_off;
	if(pGC->clipOrg.y != new_val) {
	    pGC->clipOrg.y = new_val;
	    changes |= GCClipYOrigin;
	}
	new_val = pGCPriv->patOrg.x - x_off;
	if(pGC->patOrg.x != new_val) {
	    pGC->patOrg.x = new_val;
	    changes |= GCTileStipXOrigin;
	}
	new_val = pGCPriv->patOrg.y - y_off;
	if(pGC->patOrg.y != new_val) {
	    pGC->patOrg.y = new_val;
	    changes |= GCTileStipYOrigin;
	}
    } else {
	if(pGC->clipOrg.x != pGCPriv->clipOrg.x) {
	    pGC->clipOrg.x = pGCPriv->clipOrg.x;
	    changes |= GCClipXOrigin;
	}
	if(pGC->clipOrg.y != pGCPriv->clipOrg.y) {
	    pGC->clipOrg.y = pGCPriv->clipOrg.y;
	    changes |= GCClipYOrigin;
	}
	if(pGC->patOrg.x != pGCPriv->patOrg.x) {
	    pGC->patOrg.x = pGCPriv->patOrg.x;
	    changes |= GCTileStipXOrigin;
	}
	if(pGC->patOrg.y != pGCPriv->patOrg.y) {
	    pGC->patOrg.y = pGCPriv->patOrg.y;
	    changes |= GCTileStipYOrigin;
	}
    }
  
    (*pGC->funcs->ValidateGC)(pGC, changes, pDraw);
    Xinerama_GC_FUNC_EPILOGUE (pGC);
}

static void
XineramaDestroyGC(GCPtr pGC)
{
    Xinerama_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->DestroyGC)(pGC);
    Xinerama_GC_FUNC_EPILOGUE (pGC);
}

static void
XineramaChangeGC (
    GCPtr	    pGC,
    unsigned long   mask
){
    Xinerama_GC_FUNC_PROLOGUE (pGC);

    if(mask & GCTileStipXOrigin)
	pGCPriv->patOrg.x = pGC->patOrg.x;
    if(mask & GCTileStipYOrigin)
	pGCPriv->patOrg.y = pGC->patOrg.y;
    if(mask & GCClipXOrigin)
	pGCPriv->clipOrg.x = pGC->clipOrg.x; 
    if(mask & GCClipYOrigin)
	pGCPriv->clipOrg.y = pGC->clipOrg.y;

    (*pGC->funcs->ChangeGC) (pGC, mask);
    Xinerama_GC_FUNC_EPILOGUE (pGC);
}

static void
XineramaCopyGC (
    GCPtr	    pGCSrc, 
    unsigned long   mask,
    GCPtr	    pGCDst
){
    Xinerama_GC_FUNC_PROLOGUE (pGCDst);
    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    Xinerama_GC_FUNC_EPILOGUE (pGCDst);
}
static void
XineramaChangeClip (
    GCPtr   pGC,
    int		type,
    pointer	pvalue,
    int		nrects 
){
    Xinerama_GC_FUNC_PROLOGUE (pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    Xinerama_GC_FUNC_EPILOGUE (pGC);
}

static void
XineramaCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    Xinerama_GC_FUNC_PROLOGUE (pgcDst);
    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);
    Xinerama_GC_FUNC_EPILOGUE (pgcDst);
}

static void
XineramaDestroyClip(GCPtr pGC)
{
    Xinerama_GC_FUNC_PROLOGUE (pGC);
    (* pGC->funcs->DestroyClip)(pGC);
    Xinerama_GC_FUNC_EPILOGUE (pGC);
}



/*
 *	PanoramiXExtensionInit():
 *		Called from InitExtensions in main().  
 *		Register PanoramiXeen Extension
 *		Initialize global variables.
 */ 

void PanoramiXExtensionInit(int argc, char *argv[])
{
    int 	     	i, PhyScrNum;
    Bool	     	success = FALSE;
    ExtensionEntry 	*extEntry, *AddExtension();
    PanoramiXData    	*panoramiXtempPtr;
    ScreenPtr		pScreen;
    PanoramiXScreenPtr	pScreenPriv;
    
    if (noPanoramiXExtension) 
	return;

    GlobalScrInfo = &screenInfo;		/* For debug visibility */
    PanoramiXNumScreens = screenInfo.numScreens;
    if (PanoramiXNumScreens == 1) {		/* Only 1 screen 	*/
	noPanoramiXExtension = TRUE;
	return;
    }

    while (panoramiXGeneration != serverGeneration) {
	extEntry = AddExtension(PANORAMIX_PROTOCOL_NAME, 0,0, 
				ProcPanoramiXDispatch,
				SProcPanoramiXDispatch, PanoramiXResetProc, 
				StandardMinorOpcode);
	if (!extEntry) {
	    ErrorF("PanoramiXExtensionInit(): failed to AddExtension\n");
	    break;
 	}
	PanoramiXReqCode = (unsigned char)extEntry->base;

	/*
	 *	First make sure all the basic allocations succeed.  If not,
	 *	run in non-PanoramiXeen mode.
	 */

	panoramiXdataPtr = (PanoramiXData *) Xcalloc(PanoramiXNumScreens * sizeof(PanoramiXData));
	PanoramiXWinRoot = (PanoramiXWindow *) Xcalloc(sizeof(PanoramiXWindow));
	PanoramiXGCRoot  =  (PanoramiXGC *) Xcalloc(sizeof(PanoramiXGC));
	PanoramiXCmapRoot = (PanoramiXCmap *) Xcalloc(sizeof(PanoramiXCmap));
	PanoramiXPmapRoot = (PanoramiXPmap *) Xcalloc(sizeof(PanoramiXPmap));
	BREAK_IF(!(panoramiXdataPtr && PanoramiXWinRoot && PanoramiXGCRoot &&
		   PanoramiXCmapRoot && PanoramiXPmapRoot));

	BREAK_IF((PanoramiXGCIndex = AllocateGCPrivateIndex()) < 0);
	BREAK_IF((PanoramiXScreenIndex = AllocateScreenPrivateIndex()) < 0);

	
	for (i = 0; i < PanoramiXNumScreens; i++) {
	   pScreen = screenInfo.screens[i];
	   if(!AllocateGCPrivate(pScreen, PanoramiXGCIndex, 
						sizeof(PanoramiXGCRec))) {
		noPanoramiXExtension = TRUE;
		return;
	   }

	   pScreenPriv = xalloc(sizeof(PanoramiXScreenRec));
	   pScreen->devPrivates[PanoramiXScreenIndex].ptr = 
						(pointer)pScreenPriv;
	   if(!pScreenPriv) {
		noPanoramiXExtension = TRUE;
		return;
	   }
	
	   pScreenPriv->CreateGC = pScreen->CreateGC;
	   pScreenPriv->CloseScreen = pScreen->CloseScreen;
	
	   pScreen->CreateGC = XineramaCreateGC;
	   pScreen->CloseScreen = XineramaCloseScreen;
	}

	panoramiXGeneration = serverGeneration;
	success = TRUE;
    }

    if (!success) {
	noPanoramiXExtension = TRUE;
	ErrorF("%s Extension failed to initialize\n", PANORAMIX_PROTOCOL_NAME);
	return;
    }
   
    /* Set up a default configuration base on horizontal ordering */
    for (i = PanoramiXNumScreens -1; i >= 0 ; i--) {
	panoramiXdataPtr[i].above = panoramiXdataPtr[i].below = -1;
	panoramiXdataPtr[i].left = panoramiXdataPtr[i].right = -1;
	panoramiXEdgePtr[i].no_edges = TRUE;
    }
    for (i = PanoramiXNumScreens - 1; i >= 0; i--) {
	 panoramiXdataPtr[i].left = i - 1;
	 panoramiXdataPtr[i].right = i + 1;
    }
    panoramiXdataPtr[PanoramiXNumScreens - 1].right = -1;

    /*
     *	Position the screens relative to each other based on
     *  command line options. 
     */

#if 0
    for (PhyScrNum = PanoramiXNumScreens - 1; PhyScrNum >= 0; PhyScrNum--) {
	if (wsRemapPhysToLogScreens)
	    i = wsPhysToLogScreens[PhyScrNum];
	else 
	    i = PhyScrNum;
	if (i < 0)
	    continue;
	panoramiXdataPtr[i].width = (screenInfo.screens[i])->width;
	panoramiXdataPtr[i].height = (screenInfo.screens[i])->height;
	if (screenArgs[i].flags & ARG_EDGE_L) {
	    ArgScrNum = screenArgs[PhyScrNum].edge_left;
	    if (ArgScrNum < 0)
	       j = -1;
	    else { 
	       if (wsRemapPhysToLogScreens)
	           j = wsPhysToLogScreens[ArgScrNum];
	       else 
		   j = ArgScrNum;
	    }
	    panoramiXdataPtr[i].left = j;
	    panoramiXEdgePtr[i].no_edges = FALSE;
	    if ( j >= 0) 
	        panoramiXdataPtr[j].right = i;
	    else {
		if ( i >= 1 ) 
		   panoramiXdataPtr[i - 1].right = -1;
	    }
	}
	if (screenArgs[i].flags & ARG_EDGE_R) {
	    ArgScrNum = screenArgs[PhyScrNum].edge_right;
	    if (ArgScrNum < 0)
	       j = -1;
	    else { 
	       if (wsRemapPhysToLogScreens)
	           j = wsPhysToLogScreens[ArgScrNum];
	       else 
		   j = ArgScrNum;
	    }
	    panoramiXdataPtr[i].right = j;
	    panoramiXEdgePtr[i].no_edges = FALSE;
	    if ( j >= 0) 
	        panoramiXdataPtr[j].left = i;
	}
	if (screenArgs[i].flags & ARG_EDGE_T) {
	    ArgScrNum = screenArgs[PhyScrNum].edge_top;
	    if (ArgScrNum < 0)
	       j = -1;
	    else { 
	       if (wsRemapPhysToLogScreens)
	           j = wsPhysToLogScreens[ArgScrNum];
	       else 
		   j = ArgScrNum;
	    }
	    panoramiXdataPtr[i].above = j;
	    panoramiXEdgePtr[i].no_edges = FALSE;
	    if ( j >= 0)
	        panoramiXdataPtr[j].below = i;
	}
	if (screenArgs[i].flags & ARG_EDGE_B) {
	    ArgScrNum = screenArgs[PhyScrNum].edge_bottom;
	    if (ArgScrNum < 0)
	       j = -1;
	    else { 
	       if (wsRemapPhysToLogScreens)
	           j = wsPhysToLogScreens[ArgScrNum];
	       else 
		   j = ArgScrNum;
	    }
	    panoramiXdataPtr[i].below = j;
	    panoramiXEdgePtr[i].no_edges = FALSE;
	    if ( j >= 0)
	        panoramiXdataPtr[j].above = i;
	}
    }
#else
    for (PhyScrNum = PanoramiXNumScreens - 1; PhyScrNum >= 0; PhyScrNum--) {
	i = PhyScrNum;
	if (i < 0)
	    continue;
	panoramiXdataPtr[i].width = (screenInfo.screens[i])->width;
	panoramiXdataPtr[i].height = (screenInfo.screens[i])->height;
    }
#endif
    
    /*
     *	Find the upper-left screen and then locate all the others
     */
    panoramiXtempPtr = panoramiXdataPtr;
    for (i = PanoramiXNumScreens; i; i--, panoramiXtempPtr++)
        if (panoramiXtempPtr->above == -1 && panoramiXtempPtr->left == -1)
            break;
    locate_neighbors(PanoramiXNumScreens - i);

    /*
     *	Put our processes into the ProcVector
     */

    for (i = 256; i--; )
	SavedProcVector[i] = ProcVector[i];

    ProcVector[X_CreateWindow] = PanoramiXCreateWindow;
    ProcVector[X_ChangeWindowAttributes] = PanoramiXChangeWindowAttributes;
    ProcVector[X_DestroyWindow] = PanoramiXDestroyWindow;
    ProcVector[X_DestroySubwindows] = PanoramiXDestroySubwindows;
    ProcVector[X_ChangeSaveSet] = PanoramiXChangeSaveSet;
    ProcVector[X_ReparentWindow] = PanoramiXReparentWindow;
    ProcVector[X_MapWindow] = PanoramiXMapWindow;
    ProcVector[X_MapSubwindows] = PanoramiXMapSubwindows;
    ProcVector[X_UnmapWindow] = PanoramiXUnmapWindow;
    ProcVector[X_UnmapSubwindows] = PanoramiXUnmapSubwindows;
    ProcVector[X_ConfigureWindow] = PanoramiXConfigureWindow;
    ProcVector[X_CirculateWindow] = PanoramiXCirculateWindow;
    ProcVector[X_GetGeometry] = PanoramiXGetGeometry;
    ProcVector[X_ChangeProperty] = PanoramiXChangeProperty;
    ProcVector[X_DeleteProperty] = PanoramiXDeleteProperty;
    ProcVector[X_SendEvent] = PanoramiXSendEvent;
    ProcVector[X_CreatePixmap] = PanoramiXCreatePixmap;
    ProcVector[X_FreePixmap] = PanoramiXFreePixmap;
    ProcVector[X_CreateGC] = PanoramiXCreateGC;
    ProcVector[X_ChangeGC] = PanoramiXChangeGC;
    ProcVector[X_CopyGC] = PanoramiXCopyGC;
    ProcVector[X_SetDashes] = PanoramiXSetDashes;
    ProcVector[X_SetClipRectangles] = PanoramiXSetClipRectangles;
    ProcVector[X_FreeGC] = PanoramiXFreeGC;
    ProcVector[X_ClearArea] = PanoramiXClearToBackground;
    ProcVector[X_CopyArea] = PanoramiXCopyArea;;
    ProcVector[X_CopyPlane] = PanoramiXCopyPlane;;
    ProcVector[X_PolyPoint] = PanoramiXPolyPoint;
    ProcVector[X_PolyLine] = PanoramiXPolyLine;
    ProcVector[X_PolySegment] = PanoramiXPolySegment;
    ProcVector[X_PolyRectangle] = PanoramiXPolyRectangle;
    ProcVector[X_PolyArc] = PanoramiXPolyArc;
    ProcVector[X_FillPoly] = PanoramiXFillPoly;
    ProcVector[X_PolyFillRectangle] = PanoramiXPolyFillRectangle;
    ProcVector[X_PolyFillArc] = PanoramiXPolyFillArc;
    ProcVector[X_PutImage] = PanoramiXPutImage;
    ProcVector[X_GetImage] = PanoramiXGetImage;
    ProcVector[X_PolyText8] = PanoramiXPolyText8;
    ProcVector[X_PolyText16] = PanoramiXPolyText16;
    ProcVector[X_ImageText8] = PanoramiXImageText8;
    ProcVector[X_ImageText16] = PanoramiXImageText16;
    ProcVector[X_CreateColormap] = PanoramiXCreateColormap;
    ProcVector[X_FreeColormap] = PanoramiXFreeColormap;
    ProcVector[X_InstallColormap] = PanoramiXInstallColormap;
    ProcVector[X_UninstallColormap] = PanoramiXUninstallColormap;
    ProcVector[X_AllocColor] = PanoramiXAllocColor;
    ProcVector[X_AllocNamedColor] = PanoramiXAllocNamedColor;
    ProcVector[X_AllocColorCells] = PanoramiXAllocColorCells;
    ProcVector[X_FreeColors] = PanoramiXFreeColors;
    ProcVector[X_StoreColors] = PanoramiXStoreColors;    

    return;
}
extern 
Bool PanoramiXCreateConnectionBlock(void)
{
    int i, j, length;
    Bool disableBackingStore = FALSE;
    Bool disableSaveUnders = FALSE;
    int old_width, old_height;
    int width_mult, height_mult;
    xWindowRoot *root;
    xConnSetup *setup;
    xVisualType *visual;
    xDepth *depth;
    VisualPtr pVisual;
    ScreenPtr pScreen;

    /*
     *	Do normal CreateConnectionBlock but faking it for only one screen
     */

    if(!PanoramiXNumDepths) {
	ErrorF("PanoramiX error: Incompatible screens. No common visuals\n");
	return FALSE;
    }

    for(i = 1; i < screenInfo.numScreens; i++) {
	pScreen = screenInfo.screens[i];
	if(pScreen->rootDepth != screenInfo.screens[0]->rootDepth) {
	    ErrorF("PanoramiX error: Incompatible screens. Root window depths differ\n");
	    return FALSE;
	}
	if(pScreen->backingStoreSupport != screenInfo.screens[0]->backingStoreSupport)
	     disableBackingStore = TRUE;
	if(pScreen->saveUnderSupport != screenInfo.screens[0]->saveUnderSupport)
	     disableSaveUnders = TRUE;
    }

    if(disableBackingStore || disableSaveUnders) {
    	for(i = 0; i < screenInfo.numScreens; i++) {
	    pScreen = screenInfo.screens[i];
	    if(disableBackingStore)
		pScreen->backingStoreSupport = NotUseful;
	    if(disableSaveUnders)
		pScreen->saveUnderSupport = NotUseful;
	}
    }

    i = screenInfo.numScreens;
    screenInfo.numScreens = 1;
    if (!CreateConnectionBlock()) {
	screenInfo.numScreens = i;
	return FALSE;
    }

    screenInfo.numScreens = i;
    
    setup = (xConnSetup *) ConnectionInfo;
    root = (xWindowRoot *) (ConnectionInfo + connBlockScreenStart);
    length = connBlockScreenStart + sizeof(xWindowRoot);

    /* overwrite the connection block */
    root->nDepths = PanoramiXNumDepths;

    for (i = 0; i < PanoramiXNumDepths; i++) {
	depth = (xDepth *) (ConnectionInfo + length);
	depth->depth = PanoramiXDepths[i].depth;
	depth->nVisuals = PanoramiXDepths[i].numVids;
	length += sizeof(xDepth);
	visual = (xVisualType *)(ConnectionInfo + length);
	
	for (j = 0; j < depth->nVisuals; j++, visual++) {
	    visual->visualID = PanoramiXDepths[i].vids[j];

	    for (pVisual = PanoramiXVisuals;
		 pVisual->vid != visual->visualID;
		 pVisual++)
	         ;

	    visual->class = pVisual->class;
	    visual->bitsPerRGB = pVisual->bitsPerRGBValue;
	    visual->colormapEntries = pVisual->ColormapEntries;
	    visual->redMask = pVisual->redMask;
	    visual->greenMask = pVisual->greenMask;
	    visual->blueMask = pVisual->blueMask;
	}

	length += (depth->nVisuals * sizeof(xVisualType));
    }

    connSetupPrefix.length = length >> 2;

    xfree(PanoramiXVisuals);
    for (i = 0; i < PanoramiXNumDepths; i++)
	xfree(PanoramiXDepths[i].vids);
    xfree(PanoramiXDepths);

    /*
     *  OK, change some dimensions so it looks as if it were one big screen
     */
    
    old_width = root->pixWidth;
    old_height = root->pixHeight;
    for (i = PanoramiXNumScreens - 1; i >= 0; i--) {
        if (panoramiXdataPtr[i].right == -1 )
            root->pixWidth = panoramiXdataPtr[i].x + panoramiXdataPtr[i].width;
        if (panoramiXdataPtr[i].below == -1)
            root->pixHeight = panoramiXdataPtr[i].y + panoramiXdataPtr[i].height;
    }
    PanoramiXPixWidth = root->pixWidth;
    PanoramiXPixHeight = root->pixHeight;
    width_mult = root->pixWidth / old_width;
    height_mult = root->pixHeight / old_height;
    root->mmWidth *= width_mult;
    root->mmHeight *= height_mult;
    return TRUE;
}

extern 
Bool PanoramiXCreateScreenRegion(WindowPtr pWin)
{
   ScreenPtr   pScreen;
   BoxRec      box;
   int         i;
   Bool	       ret;
   
   pScreen = pWin->drawable.pScreen;
   for (i = 0; i < PanoramiXNumScreens; i++) {
        box.x1 = 0 - panoramiXdataPtr[i].x;
        box.x2 = box.x1 + PanoramiXPixWidth;
        box.y1 = 0 - panoramiXdataPtr[i].y;
        box.y2 = box.y1 + PanoramiXPixHeight;
        REGION_INIT(pScreen, &PanoramiXScreenRegion[i], &box, 1);
        ret = REGION_NOTEMPTY(pScreen, &PanoramiXScreenRegion[i]);
        if (!ret)
           return ret;
   }
   return ret;
}

extern
void PanoramiXDestroyScreenRegion(WindowPtr pWin)
{
   ScreenPtr   pScreen;
   int         i;

   pScreen = pWin->drawable.pScreen;
   for (i = 0; i < PanoramiXNumScreens; i++) 
        REGION_DESTROY(pScreen, &PanoramiXScreenRegion[i]);
}

/* 
 *  Assign the Root window and colormap ID's in the PanoramiXScreen Root
 *  linked lists. Note: WindowTable gets setup in dix_main by 
 *  InitRootWindow, and GlobalScrInfo is screenInfo which gets setup
 *  by InitOutput.
 */
extern
void PanoramiXConsolidate(void)
{
    int 	i, j, k, l;
    VisualPtr   pVisual, pVisual2;
    ScreenPtr   pScreen, pScreen2;

    /* initialize the visual table */
    for (i = 0; i < 256; i++) {
	for (j = 0; j < MAXSCREENS - 1; j++) 
	 PanoramiXVisualTable[i][j] = 0;
    }

    pScreen = screenInfo.screens[0];
    pVisual = pScreen->visuals; 

    PanoramiXNumDepths = 0;
    PanoramiXDepths = xcalloc(pScreen->numDepths,sizeof(DepthRec));
    PanoramiXNumVisuals = 0;
    PanoramiXVisuals = xcalloc(pScreen->numVisuals,sizeof(VisualRec));

    for (i = 0; i < pScreen->numVisuals; i++, pVisual++) {
	PanoramiXVisualTable[pVisual->vid][0] = pVisual->vid;

	/* check if the visual exists on all screens */
	for (j = 1; j < PanoramiXNumScreens; j++) {
	    pScreen2 = screenInfo.screens[j];
	    pVisual2 = pScreen2->visuals;

	    for (k = 0; k < pScreen2->numVisuals; k++, pVisual2++) {
		if ((pVisual->class == pVisual2->class) &&
		    (pVisual->ColormapEntries == pVisual2->ColormapEntries) &&
		    (pVisual->nplanes == pVisual2->nplanes) &&
		    (pVisual->redMask == pVisual2->redMask) &&
		    (pVisual->greenMask == pVisual2->greenMask) &&
		    (pVisual->blueMask == pVisual2->blueMask) &&
		    (pVisual->offsetRed == pVisual2->offsetRed) &&
		    (pVisual->offsetGreen == pVisual2->offsetGreen) &&
		    (pVisual->offsetBlue == pVisual2->offsetBlue))
		{
		    Bool AlreadyUsed = FALSE;
		    for (l = 0; l < 256; l++) {
			if (pVisual2->vid == PanoramiXVisualTable[l][j]) {	
			    AlreadyUsed = TRUE;
			    break;
			}
		    }
		    if (!AlreadyUsed) {
			PanoramiXVisualTable[pVisual->vid][j] = pVisual2->vid;
			break;
		    }
		}
	    }
	}
	
	/* if it doesn't exist on all screens we can't use it */
	for (j = 0; j < PanoramiXNumScreens; j++) {
	    if (!PanoramiXVisualTable[pVisual->vid][j]) {
		PanoramiXVisualTable[pVisual->vid][0] = 0;
		break;
	    }
	}

	/* if it does, make sure it's in the list of supported depths and visuals */
	if(PanoramiXVisualTable[pVisual->vid][0]) {
	    Bool GotIt = FALSE;

	    PanoramiXVisuals[PanoramiXNumVisuals].vid = pVisual->vid;
	    PanoramiXVisuals[PanoramiXNumVisuals].class = pVisual->class;
	    PanoramiXVisuals[PanoramiXNumVisuals].bitsPerRGBValue = pVisual->bitsPerRGBValue;
	    PanoramiXVisuals[PanoramiXNumVisuals].ColormapEntries = pVisual->ColormapEntries;
	    PanoramiXVisuals[PanoramiXNumVisuals].nplanes = pVisual->nplanes;
	    PanoramiXVisuals[PanoramiXNumVisuals].redMask = pVisual->redMask;
	    PanoramiXVisuals[PanoramiXNumVisuals].greenMask = pVisual->greenMask;
	    PanoramiXVisuals[PanoramiXNumVisuals].blueMask = pVisual->blueMask;
	    PanoramiXVisuals[PanoramiXNumVisuals].offsetRed = pVisual->offsetRed;
	    PanoramiXVisuals[PanoramiXNumVisuals].offsetGreen = pVisual->offsetGreen;
	    PanoramiXVisuals[PanoramiXNumVisuals].offsetBlue = pVisual->offsetBlue;
	    PanoramiXNumVisuals++;	

	    for (j = 0; j < PanoramiXNumDepths; j++) {
	        if (PanoramiXDepths[j].depth == pVisual->nplanes) {
		    PanoramiXDepths[j].vids[PanoramiXDepths[j].numVids] = pVisual->vid;
		    PanoramiXDepths[j].numVids++;
		    GotIt = TRUE;
		    break;
		}	
	    }   

	    if (!GotIt) {
		PanoramiXDepths[PanoramiXNumDepths].depth = pVisual->nplanes;
		PanoramiXDepths[PanoramiXNumDepths].numVids = 1;
		PanoramiXDepths[PanoramiXNumDepths].vids = xalloc(sizeof(VisualID) * 256);
	        PanoramiXDepths[PanoramiXNumDepths].vids[0] = pVisual->vid;
		PanoramiXNumDepths++;
	    } 
	}
    } 

    for (i =  0; i < PanoramiXNumScreens; i++) {
	PanoramiXWinRoot->info[i].id = WindowTable[i]->drawable.id;
	PanoramiXCmapRoot->info[i].id = (screenInfo.screens[i])->defColormap;
    }
}

/* Since locate_neighbors is recursive, a quick simple example 
   is in order.This mostly so you can see what the initial values are. 

   Given 3 screens:
   upperleft screen[0]
	panoramiXdataPtr[0].x = 0
	panoramiXdataPtr[0].y = 0
	panoramiXdataPtr[0].width  = 640
	panoramiXdataPtr[0].height = 480
	panoramiXdataPtr[0].below = -1
	panoramiXdataPtr[0].right = 1
	panoramiXdataPtr[0].above = -1
	panoramiXdataPtr[0].left = -1
   middle screen[1]
	panoramiXdataPtr[1].x = 0
	panoramiXdataPtr[1].y = 0
	panoramiXdataPtr[1].width  = 640
	panoramiXdataPtr[1].height = 480
	panoramiXdataPtr[1].below = -1
	panoramiXdataPtr[1].right = 2
	panoramiXdataPtr[1].above = -1
	panoramiXdataPtr[1].left = 0
   last right screen[2]
	panoramiXdataPtr[2].x = 0
	panoramiXdataPtr[2].y = 0
	panoramiXdataPtr[2].width  = 640
	panoramiXdataPtr[2].height = 480
	panoramiXdataPtr[2].below = -1
	panoramiXdataPtr[2].right = -1 
	panoramiXdataPtr[2].above = -1
	panoramiXdataPtr[2].left = 1
	
   Calling locate_neighbors(0) results in: 
	panoramiXdataPtr[0].x = 0
	panoramiXdataPtr[0].y = 0
	panoramiXdataPtr[1].x = 640
	panoramiXdataPtr[1].y = 0
	panoramiXdataPtr[2].x = 1280 
	panoramiXdataPtr[2].y = 0
*/

static void locate_neighbors(int i)
{
    int j;
    
    j = panoramiXdataPtr[i].right;
    if ((j != -1) && !panoramiXdataPtr[j].x && !panoramiXdataPtr[j].y) {
	panoramiXdataPtr[j].x = panoramiXdataPtr[i].x + panoramiXdataPtr[i].width;
	panoramiXdataPtr[j].y = panoramiXdataPtr[i].y;
	locate_neighbors(j);
    }
    j = panoramiXdataPtr[i].below;
    if ((j != -1) && !panoramiXdataPtr[j].x && !panoramiXdataPtr[j].y) {
	panoramiXdataPtr[j].y = panoramiXdataPtr[i].y + panoramiXdataPtr[i].height;
	panoramiXdataPtr[j].x = panoramiXdataPtr[i].x;
	locate_neighbors(j);
    }
}



/*
 *	PanoramiXResetProc()
 *		Exit, deallocating as needed.
 */

static void PanoramiXResetProc(ExtensionEntry* extEntry)
{
    int		i;
    PanoramiXList *pPanoramiXList;
    PanoramiXList *tempList;

    for (pPanoramiXList = PanoramiXPmapRoot; pPanoramiXList; pPanoramiXList = tempList){
	tempList = pPanoramiXList->next;
	Xfree(pPanoramiXList);
    }
    for (pPanoramiXList = PanoramiXCmapRoot; pPanoramiXList; pPanoramiXList = tempList){
	tempList = pPanoramiXList->next;
	Xfree(pPanoramiXList);
    }
    for (pPanoramiXList = PanoramiXGCRoot; pPanoramiXList; pPanoramiXList = tempList) {
	tempList = pPanoramiXList->next;
	Xfree(pPanoramiXList);
    }
    for (pPanoramiXList = PanoramiXWinRoot; pPanoramiXList; pPanoramiXList = tempList) {
	tempList = pPanoramiXList->next;
	Xfree(pPanoramiXList);
    }
    screenInfo.numScreens = PanoramiXNumScreens;
    for (i = 256; i--; )
	ProcVector[i] = SavedProcVector[i];
    Xfree(panoramiXdataPtr);
    
}


int
ProcPanoramiXQueryVersion (ClientPtr client)
{
    REQUEST(xPanoramiXQueryVersionReq);
    xPanoramiXQueryVersionReply		rep;
    register 	int			n;

    REQUEST_SIZE_MATCH (xPanoramiXQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = PANORAMIX_MAJOR_VERSION;
    rep.minorVersion = PANORAMIX_MINOR_VERSION;   
    if (client->swapped) { 
        swaps(&rep.sequenceNumber, n);
        swapl(&rep.length, n);     
    }
    WriteToClient(client, sizeof (xPanoramiXQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

int
ProcPanoramiXGetState(ClientPtr client)
{
	REQUEST(xPanoramiXGetStateReq);
    	WindowPtr			pWin;
	xPanoramiXGetStateReply		rep;
	register int			n;
	
	REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
	pWin = LookupWindow (stuff->window, client);
	if (!pWin)
	     return BadWindow;
	rep.type = X_Reply;
	rep.length = 0;
	rep.sequenceNumber = client->sequence;
	rep.state = !noPanoramiXExtension;
    	if (client->swapped) {
	    swaps (&rep.sequenceNumber, n);
	    swapl (&rep.length, n);
	    swaps (&rep.state, n);
	}	
	WriteToClient (client, sizeof (xPanoramiXGetStateReply), (char *) &rep);
	return client->noClientException;

}

int 
ProcPanoramiXGetScreenCount(ClientPtr client)
{
	REQUEST(xPanoramiXGetScreenCountReq);
    	WindowPtr			pWin;
	xPanoramiXGetScreenCountReply	rep;
	register int			n;

	REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
	pWin = LookupWindow (stuff->window, client);
	if (!pWin)
	     return BadWindow;
	rep.type = X_Reply;
	rep.length = 0;
	rep.sequenceNumber = client->sequence;
	rep.ScreenCount = PanoramiXNumScreens;
    	if (client->swapped) {
	    swaps (&rep.sequenceNumber, n);
	    swapl (&rep.length, n);
	    swaps (&rep.ScreenCount, n);
	}	
	WriteToClient (client, sizeof (xPanoramiXGetScreenCountReply), (char *) &rep);
	return client->noClientException;
}

int 
ProcPanoramiXGetScreenSize(ClientPtr client)
{
	REQUEST(xPanoramiXGetScreenSizeReq);
    	WindowPtr			pWin;
	xPanoramiXGetScreenSizeReply	rep;
	register int			n;
	
	REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);
	pWin = LookupWindow (stuff->window, client);
	if (!pWin)
	     return BadWindow;
	rep.type = X_Reply;
	rep.length = 0;
	rep.sequenceNumber = client->sequence;
		/* screen dimensions */
	rep.width  = panoramiXdataPtr[stuff->screen].width; 
	rep.height = panoramiXdataPtr[stuff->screen].height; 
    	if (client->swapped) {
	    swaps (&rep.sequenceNumber, n);
	    swapl (&rep.length, n);
	    swaps (rep.width, n);
	    swaps (rep.height, n);
	}	
	WriteToClient (client, sizeof (xPanoramiXGetScreenSizeReply), (char *) &rep);
	return client->noClientException;
}


void PrintList(PanoramiXList *head)
{
    int i = 0;

    for (; head; i++, head = head->next)
	fprintf(stderr, "%2d  next = 0x%010lx,   id[0] = 0x%08x,   id[1] = 0x%08x\n",
	    i, head->next, head->info[0].id, head->info[1].id);
}
static int
ProcPanoramiXDispatch (ClientPtr client)
{   REQUEST(xReq);
    switch (stuff->data)
    {
	case X_PanoramiXQueryVersion:
	     return ProcPanoramiXQueryVersion(client);
	case X_PanoramiXGetState:
	     return ProcPanoramiXGetState(client);
	case X_PanoramiXGetScreenCount:
	     return ProcPanoramiXGetScreenCount(client);
	case X_PanoramiXGetScreenSize:
	     return ProcPanoramiXGetScreenSize(client);
    }
    return BadRequest;
}

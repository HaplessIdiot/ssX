/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86fbman.h,v 1.5 1998/11/28 10:43:02 dawes Exp $ */

#ifndef _XF86FBMAN_H
#define _XF86FBMAN_H


#include "scrnintstr.h"
#include "regionstr.h"


typedef struct _FBArea {
   ScreenPtr    pScreen;
   BoxRec   	box;
   int 		granularity;
   void 	(*MoveAreaCallback)(struct _FBArea*);
   void 	(*RemoveAreaCallback)(struct _FBArea*);
   DevUnion 	devPrivate;
} FBArea, *FBAreaPtr;

typedef struct _FBLink {
  FBArea area;
  struct _FBLink *next;  
} FBLink, *FBLinkPtr;

typedef void (*FreeBoxCallbackProcPtr)(ScreenPtr, RegionPtr, pointer);
typedef void (*MoveAreaCallbackProcPtr)(FBAreaPtr);
typedef void (*RemoveAreaCallbackProcPtr)(FBAreaPtr);

typedef struct {
   ScreenPtr	pScreen;
   RegionPtr	InitialBoxes;
   RegionPtr	FreeBoxes;
   FBLinkPtr 	UsedAreas;
   int		NumUsedAreas;
   CloseScreenProcPtr 		CloseScreen;
   int				NumCallbacks;
   FreeBoxCallbackProcPtr	*FreeBoxesUpdateCallback;
   DevUnion			*devPrivates;
} FBManager, *FBManagerPtr;



Bool
xf86InitFBManagerRegion(
    ScreenPtr pScreen, 
    RegionPtr ScreenRegion
);

Bool
xf86InitFBManager(
    ScreenPtr pScreen, 
    BoxPtr FullBox
);

Bool 
xf86FBManagerRunning(
    ScreenPtr pScreen
);

FBAreaPtr 
xf86AllocateOffscreenArea (
   ScreenPtr pScreen, 
   int w, int h,
   int granularity,
   MoveAreaCallbackProcPtr moveCB,
   RemoveAreaCallbackProcPtr removeCB,
   pointer privData
);

void xf86FreeOffscreenArea(FBAreaPtr area);

Bool 
xf86ResizeOffscreenArea(
   FBAreaPtr resize,
   int w, int h
);

Bool
xf86RegisterFreeBoxCallback(
    ScreenPtr pScreen,  
    FreeBoxCallbackProcPtr FreeBoxCallback,
    pointer devPriv
);

#endif /* _XF86FBMAN_H */

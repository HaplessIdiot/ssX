/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86fbman.h,v 1.3 1998/08/29 05:43:03 dawes Exp $ */

#ifndef _XF86FBMAN_H
#define _XF86FBMAN_H


#include "scrnintstr.h"
#include "regionstr.h"


typedef struct _FBArea {
   ScreenPtr    pScreen;
   BoxRec   	box;
   int 		granularity;
   void 	(*MoveAreaCallback)(struct _FBArea*);
   DevUnion 	devPrivate;
} FBArea, *FBAreaPtr;

typedef struct _FBLink {
  FBArea area;
  struct _FBLink *next;  
} FBLink, *FBLinkPtr;

typedef void (*FreeBoxCallbackProcPtr)(ScreenPtr, RegionPtr, pointer);
typedef void (*MoveAreaCallbackProcPtr)(FBAreaPtr);

typedef struct {
   ScreenPtr	pScreen;
   RegionPtr	InitialBoxes;
   RegionPtr	FreeBoxes;
   FBLinkPtr 	UsedAreas;
   int		NumUsedAreas;
   CloseScreenProcPtr 		CloseScreen;
   FreeBoxCallbackProcPtr	FreeBoxesUpdateCallback;
   DevUnion	devPrivate;
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

void
xf86RegisterFreeBoxCallback(
    ScreenPtr pScreen,  
    FreeBoxCallbackProcPtr FreeBoxCallback,
    pointer devPriv
);

typedef FBAreaPtr (*AllocateOffscreenAreaProcPtr) (
   ScreenPtr pScreen, 
   int w, int h,
   int granularity,
   MoveAreaCallbackProcPtr MoveAreaCallback,
   pointer privData
);

typedef Bool (*ResizeOffscreenAreaProcPtr) (
   ScreenPtr pScreen, 
   int w, int h,
   FBAreaPtr area
);


typedef void (*FreeOffscreenAreaProcPtr) (FBAreaPtr area);


#endif /* _XF86FBMAN_H */

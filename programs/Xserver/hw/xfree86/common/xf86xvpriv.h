/* $XFree86$ */

#ifndef _XF86XVPRIV_H_
#define _XF86XVPRIV_H_

#include "xf86xv.h"

/*** These are DDX layer privates ***/

extern int XF86XvScreenIndex;

typedef struct {
   DestroyWindowProcPtr		DestroyWindow;
   ClipNotifyProcPtr		ClipNotify;
   WindowExposuresProcPtr	WindowExposures;
   void                         (*AdjustFrame)(int, int, int, int);
   Bool                         (*EnterVT)(int, int);
   void                         (*LeaveVT)(int, int);
   GCPtr			videoGC;
} XF86XVScreenRec, *XF86XVScreenPtr;

typedef struct {
  int flags;  
  PutVideoFuncPtr PutVideo;
  PutStillFuncPtr PutStill;
  GetVideoFuncPtr GetVideo;
  GetStillFuncPtr GetStill;
  StopVideoFuncPtr StopVideo;
  SetPortAttributeFuncPtr SetPortAttribute;
  GetPortAttributeFuncPtr GetPortAttribute;
  QueryBestSizeFuncPtr QueryBestSize;
  PutImageFuncPtr PutImage;
  ReputImageFuncPtr ReputImage;
  QueryImageAttributesFuncPtr QueryImageAttributes;
} XvAdaptorRecPrivate, *XvAdaptorRecPrivatePtr;

typedef struct {
   ScrnInfoPtr pScrn;
   DrawablePtr pDraw;
   unsigned char type;
   unsigned int subWindowMode;
   DDXPointRec clipOrg;
   RegionPtr clientClip;
   RegionPtr pCompositeClip;
   Bool FreeCompositeClip;
   XvAdaptorRecPrivatePtr AdaptorRec;
   XvStatus isOn;
   Bool moved;
   int vid_x, vid_y, vid_w, vid_h;
   int drw_x, drw_y, drw_w, drw_h;
   DevUnion DevPriv;
} XvPortRecPrivate, *XvPortRecPrivatePtr;

typedef struct _XF86XVWindowRec{
   XvPortRecPrivatePtr PortRec;
   struct _XF86XVWindowRec *next;
} XF86XVWindowRec, *XF86XVWindowPtr;

#endif  /* _XF86XVPRIV_H_ */

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86xv.h,v 1.4 1999/03/28 15:32:31 dawes Exp $ */

#ifndef _XVDIX_H_
#define _XVDIX_H_

#include "xvdix.h"
#include "xf86str.h"

#define VIDEO_NO_CLIPPING			0x00000001
#define VIDEO_INVERT_CLIPLIST			0x00000002
#define VIDEO_EXPOSE				0x00000004


typedef  int (* PutVideoFuncPtr)( ScrnInfoPtr pScrn, 
	short vid_x, short vid_y, short drw_x, short drw_y,
	short vid_w, short vid_h, short drw_w, short drw_h,
	RegionPtr clipBoxes, pointer data );
typedef  int (* PutStillFuncPtr)( ScrnInfoPtr pScrn, 
	short vid_x, short vid_y, short drw_x, short drw_y,
	short vid_w, short vid_h, short drw_w, short drw_h,
	RegionPtr clipBoxes, pointer data );
typedef int (* GetVideoFuncPtr)( ScrnInfoPtr pScrn, 
	short vid_x, short vid_y, short drw_x, short drw_y,
	short vid_w, short vid_h, short drw_w, short drw_h,
	RegionPtr clipBoxes, pointer data );
typedef int (* GetStillFuncPtr)( ScrnInfoPtr pScrn, 
	short vid_x, short vid_y, short drw_x, short drw_y,
	short vid_w, short vid_h, short drw_w, short drw_h,
	RegionPtr clipBoxes, pointer data );
typedef void (* StopVideoFuncPtr)(ScrnInfoPtr pScrn, pointer data, Bool exit);
typedef int (* SetPortAttributeFuncPtr)(ScrnInfoPtr pScrn, Atom attribute,
	INT32 value, pointer data);
typedef int (* GetPortAttributeFuncPtr)(ScrnInfoPtr pScrn, Atom attribute,
	INT32 *value, pointer data);
typedef void (* QueryBestSizeFuncPtr)(ScrnInfoPtr pScrn, Bool motion,
	short vid_w, short vid_h, short drw_w, short drw_h, 
	unsigned int *p_w, unsigned int *p_h, pointer data);


/*** this is what the driver needs to fill out ***/

typedef struct {
  int id;
  char *name;
  unsigned short width, height;
  XvRationalRec rate;
} XF86VideoEncodingRec, *XF86VideoEncodingPtr;


typedef struct {
  char 	depth;  
  short class;
} XF86VideoFormatRec, *XF86VideoFormatPtr;

typedef struct {
  int 	number;  
  int   *flags;
  char  **names;
} XF86AttributeListRec, *XF86AttributeListPtr;

typedef struct {
  unsigned char type; 
  int flags;
  char *name;
  int nEncodings;
  XF86VideoEncodingPtr pEncodings;  
  int nFormats;
  XF86VideoFormatPtr pFormats;  
  int nPorts;
  XF86AttributeListPtr pAttributes;
  DevUnion *pPortPrivates;
  PutVideoFuncPtr PutVideo;
  PutStillFuncPtr PutStill;
  GetVideoFuncPtr GetVideo;
  GetStillFuncPtr GetStill;
  StopVideoFuncPtr StopVideo;
  SetPortAttributeFuncPtr SetPortAttribute;
  GetPortAttributeFuncPtr GetPortAttribute;
  QueryBestSizeFuncPtr QueryBestSize;
} XF86VideoAdaptorRec, *XF86VideoAdaptorPtr;


Bool
xf86XVScreenInit(
   ScreenPtr pScreen, 
   XF86VideoAdaptorPtr 	*Adaptors,
   int num
);

int
xf86XVRegisterGenericAdaptor(
   XF86VideoAdaptorPtr 	*Adaptors,
   int num
);

int
xf86XVListGenericAdaptors(
   XF86VideoAdaptorPtr 	**Adaptors
);


/*** These are DDX layer privates ***/


typedef struct {
   CreateWindowProcPtr		CreateWindow;
   ClipNotifyProcPtr		ClipNotify;
   UnrealizeWindowProcPtr	UnrealizeWindow;
   WindowExposuresProcPtr	WindowExposures;
   CopyWindowProcPtr		CopyWindow;
   Bool                         (*EnterVT)(int, int);
   void                         (*LeaveVT)(int, int);
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
   Bool isOn;
   int vid_x, vid_y, vid_w, vid_h;
   int drw_x, drw_y, drw_w, drw_h;
   DevUnion DevPriv;
} XvPortRecPrivate, *XvPortRecPrivatePtr;

typedef struct _XF86XVWindowRec{
   XvPortRecPrivatePtr PortRec;
   struct _XF86XVWindowRec *next; 
} XF86XVWindowRec, *XF86XVWindowPtr;

#endif  /* _XVDIX_H_ */
 

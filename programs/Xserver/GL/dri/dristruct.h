/* $XFree86: xc/programs/Xserver/GL/dri/dristruct.h,v 1.5 2000/02/14 06:27:14 martin Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Jens Owen <jens@precisioninsight.com>
 *
 */

#ifndef DRI_STRUCT_H
#define DRI_STRUCT_H

#include "xf86drm.h"


#define DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin) \
    ((DRIWindowPrivIndex < 0) ? \
     NULL : \
     ((DRIDrawablePrivPtr)((pWin)->devPrivates[DRIWindowPrivIndex].ptr)))

#define DRI_DRAWABLE_PRIV_FROM_PIXMAP(pPix) \
    ((DRIPixmapPrivIndex < 0) ? \
     NULL : \
     ((DRIDrawablePrivPtr)((pPix)->devPrivates[DRIWindowPrivIndex].ptr)))

typedef struct _DRIDrawablePrivRec
{
    drmDrawable		hwDrawable;
    int			drawableIndex;
    ScreenPtr		pScreen;
    int 		refCount;
} DRIDrawablePrivRec, *DRIDrawablePrivPtr;

struct _DRIContextPrivRec
{
    drmContext		hwContext;
    ScreenPtr		pScreen;
    Bool     		valid3D;
    DRIContextFlags     flags;
    void**     		pContextStore;
};

#define DRI_SCREEN_PRIV(pScreen) \
    ((DRIScreenPrivIndex < 0) ? \
     NULL : \
     ((DRIScreenPrivPtr)((pScreen)->devPrivates[DRIScreenPrivIndex].ptr)))

#define DRI_SCREEN_PRIV_FROM_INDEX(screenIndex) ((DRIScreenPrivPtr) \
    (screenInfo.screens[screenIndex]->devPrivates[DRIScreenPrivIndex].ptr))


typedef struct _DRIScreenPrivRec
{
    Bool		directRenderingSupport;
    int			drmFD;	      /* File descriptor for /dev/video/?   */
    drmHandle   	hSAREA;	      /* Handle to SAREA, for mapping       */
    XF86DRISAREAPtr	pSAREA;	      /* Mapped pointer to SAREA            */
    drmHandle   	hFrameBuffer; /* Handle to framebuffer, for mapping */
    drmContext          myContext;    /* DDX Driver's context               */
    DRIContextPrivPtr   myContextPriv;/* Pointer to server's private area   */
    DRIContextPrivPtr   lastPartial3DContext;  /* last one partially saved  */
    void**		hiddenContextStore;    /* hidden X context          */
    void**		partial3DContextStore; /* parital 3D context        */
    DRIInfoPtr		pDriverInfo;
    void		(*WakeupHandler)(int screenNum,
					 pointer wakeupData,
					 unsigned long result,
					 pointer pReadmask);
    void		(*BlockHandler)(int screenNum,
					pointer blockData,
					struct timeval **pTimeout,
					pointer pReadmask);
    void		(*PaintWindowBackground)(WindowPtr pWin,
						 RegionPtr prgn,
						 int what);
    void		(*PaintWindowBorder)(WindowPtr pWin,
					     RegionPtr prgn,
					     int what);
    void		(*CopyWindow)(WindowPtr pWin,
				      DDXPointRec ptOldOrg,
				      RegionPtr prgnSrc);
    int			(*ValidateTree)(WindowPtr pParent,
				        WindowPtr pChild,
				        VTKind    kind);
    void		(*PostValidateTree)(WindowPtr pParent,
					    WindowPtr pChild,
					    VTKind    kind);
    DrawablePtr		DRIDrawables[SAREA_MAX_DRAWABLES];
} DRIScreenPrivRec, *DRIScreenPrivPtr;

#endif /* DRI_STRUCT_H */

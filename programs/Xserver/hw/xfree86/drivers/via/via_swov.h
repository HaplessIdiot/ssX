/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_swov.h,v 1.1tsi Exp $ */
/*#define   XV_DEBUG	  1*/	  /* write log msg to /var/log/XFree86.0.log */

#ifdef XV_DEBUG
#  define DBG_DD(x) (x)
#else
#  define DBG_DD(x)
#endif

#include "ddmpeg.h"
#include "via_xvpriv.h"

/* Definition for VideoStatus */
#define VIDEO_NULL		0x00000000

void PassViaInfo(ScrnInfoPtr pScrnOV, viaPortPrivPtr pPrivOV);

CARD32 VIAVidCreateSurface(LPSURFACEPARAM lpSurfaceParam);
CARD32 VIAVidLockSurface(LPLOCKPARAM lpLock);
CARD32 VIAVidUpdateOverlay(VIAPtr pVia, LPUPDATEOVERLAYREC lpUpdate);
CARD32 VIAVidDestroySurface( LPSURFACEPARAM lpSurfaceParam);



/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i810_dri.h,v 1.1 1999/08/29 12:21:03 dawes Exp $ */

#ifndef _I810_DRI_
#define _I810_DRI_

#include <xf86drm.h>
#include "i810_sarea.h"

#define I810_MAX_DRAWABLES 256

typedef struct {
   drmHandle regs;
   drmSize regsSize;
   drmAddress regsMap;
   int deviceID;
   int width;
   int height;
   int mem;
   int cpp;
   int bitsPerPixel;
   int fbOffset;
   int fbStride;

   int backOffset;
   int depthOffset;

   int auxPitch;
   int auxPitchBits;

   int logTextureGranularity;
   int textureOffset;
   int textureSize;

   /* For non-dma direct rendering.
    */
   int ringOffset;
   int ringSize;

   drmBufMapPtr drmBufs;
   int irq;

} I810DRIRec, *I810DRIPtr;



typedef struct {
  /* Nothing here yet */
  int dummy;
} I810ConfigPrivRec, *I810ConfigPrivPtr;

typedef struct {
  /* Nothing here yet */
  int dummy;
} I810DRIContextRec, *I810DRIContextPtr;


#endif

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tdfx/tdfx_dri.h,v 1.1 1999/08/29 12:21:03 dawes Exp $ */

#ifndef _TDFX_DRI_
#define _TDFX_DRI_

#include <xf86drm.h>

typedef struct {
  drmHandle regs;
  drmSize regsSize;
  drmAddress regsMap;
  int deviceID;
  int width;
  int height;
  int mem;
  int cpp;
  int stride;
  int priv1;
  int priv2;
  int fbOffset;
  int backOffset;
  int depthOffset;
  int textureOffset;
  int textureSize;
} TDFXDRIRec, *TDFXDRIPtr;

#endif

/* $XFree86$ */

#ifndef _TDFX_DRI_
#define _TDFX_DRI_

#include <xf86drm.h>

typedef struct {
  drmHandle regs;
  drmSize regsSize;
  drmAddress regsMap;
  int deviceID;
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

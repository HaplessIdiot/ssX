/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i810_dripriv.h,v 1.1 1999/08/29 12:21:03 dawes Exp $ */

#ifndef _I810_DRIPRIV_H_
#define _I810_DRIPRIV_H_

#define I810_MAX_DRAWABLES 256

extern void GlxSetVisualConfigs(
    int nconfigs,
    __GLXvisualConfig *configs,
    void **configprivs
);

typedef struct {
  /* Nothing here yet */
  int dummy;
} I810ConfigPrivRec, *I810ConfigPrivPtr;

typedef struct {
  /* Nothing here yet */
  int dummy;
} I810DRIContextRec, *I810DRIContextPtr;

#endif

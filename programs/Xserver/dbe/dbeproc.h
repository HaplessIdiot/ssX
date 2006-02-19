/* $XFree86: xc/programs/Xserver/dbe/dbeproc.h,v 1.1tsi Exp $ */

#ifndef DBEPROC_H
#define DBEPROC_H 1

#ifndef DBE_EXT_INIT_ONLY

#include "dbestruct.h"

typedef Bool (*DbeInitFuncPtr)(ScreenPtr pScreen,
			       DbeScreenPrivPtr pDbeScreenPriv);

/* DbeValidateBuffer declaration moved to dix.h */

void DbeRegisterFunction(ScreenPtr pScreen, DbeInitFuncPtr funct);

#endif /* DBE_EXT_INIT_ONLY */

void DbeExtensionInit(void);

#endif /* DBEPROC_H */

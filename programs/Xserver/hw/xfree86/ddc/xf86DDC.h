/* $XFree86$ */

/* xf86DDC.h
 *
 * This file contains all information to interpret a standard EDIC block 
 * transmitted by a display device via DDC (Display Data Channel). So far 
 * there is no information to deal with optional EDID blocks.  
 * DDC is a Trademark of VESA (Video Electronics Standard Association).
 *
 * Copyright 1998 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */


#ifndef XF86_DDC_H
# define XF86_DDC_H

#include "edid.h"
#include "xf86i2c.h"

/* speed up / slow down */
typedef enum {
  DDC_SLOW,
  DDC_FAST
} xf86ddcSpeed;

extern xf86MonPtr xf86DoEDID_DDC1(
#if NeedFunctionPrototypes
    int scrnIndex, 
    void (*DDC1SetSpeed)(ScrnInfoPtr, xf86ddcSpeed),
    unsigned int (*DDC1Read)(ScrnInfoPtr)
#endif
);

extern xf86MonPtr xf86DoEDID_DDC2(
#if NeedFunctionPrototypes
   int scrnIndex,
   I2CBusPtr pBus
#endif
);

extern void xf86PrintEDID(
#if NeedFunctionPrototypes
    xf86MonPtr 
#endif
);

#endif



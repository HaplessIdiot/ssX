/* **********************************************************
 * Copyright (C) 1998-2002 VMware, Inc.
 * All Rights Reserved
 * **********************************************************/
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/vmware/ 1.7 2002/10/16 22:12:53 alanh Exp $ */

#ifndef OFFSCREEN_MANAGER_H
#define OFFSCREEN_MANAGER_H

struct _Heap;
typedef struct _Heap Heap;

extern Heap* Heap_Create(CARD8* ptr, CARD32 size, CARD32 maxSlots,
                         CARD32 startOffset, CARD32 sWidth, CARD32 sHeight,
                         CARD32 sBPP, CARD32 sPitch, CARD32 sFbOffset);
extern void Heap_Destroy(Heap* heap);

extern void Heap_Clear(Heap* heap);

extern SVGASurface* Heap_GetFrontBuffer(Heap* heap);

extern SVGASurface* Heap_AllocSurface(Heap* heap, CARD32 width, CARD32 height,
                                      CARD32 pitch, CARD32 bpp);

#endif

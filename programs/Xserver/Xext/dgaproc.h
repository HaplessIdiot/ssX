/* $XFree86: xc/programs/Xserver/Xext/dgaproc.h,v 1.1.2.1 1998/06/11 16:01:07 dawes Exp $ */

/* Prototypes for DGA functions that the DDX must provide */

#ifndef _DGAPROC_H_
#define _DGAPROC_H_

Bool DGAGetParameters(int scrnIndex, CARD32 *offset, CARD32 *bankSize,
		      CARD32 *memSize, CARD32 *width);
Bool DGAAvailable(int scrnIndex);
Bool DGAScreenActive(int scrnIndex);
Bool DGAGetDirectMode(int scrnIndex);
void DGASetFlags(int scrnIndex, int flags);
int DGAGetFlags(int scrnIndex);
Bool DGAEnableDirectMode(int scrnIndex);
void DGADisableDirectMode(int scrnIndex);
void DGAGetViewPortSize(int scrnIndex, int *width, int *height);
Bool DGASetViewPort(int scrnIndex, int x, int y);
int DGAGetVidPage(int scrnIndex);
Bool DGASetVidPage(int scrnIndex, int bank);
Bool DGAViewPortChanged(int scrnIndex, int n);

#endif

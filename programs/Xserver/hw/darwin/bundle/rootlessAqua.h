/*
 * Rootless setup for Aqua
 *
 * Greg Parker     gparker@cs.stanford.edu
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/rootlessAqua.h,v 1.1 2001/06/26 23:29:12 torrey Exp $ */

#ifndef _ROOTLESSAQUA_H
#define _ROOTLESSAQUA_H

Bool AquaAddScreen(int index, ScreenPtr pScreen);
Bool AquaSetupScreen(int index, ScreenPtr pScreen);
void AquaDisplayInit(void);

#endif /* _ROOTLESSAQUA_H */

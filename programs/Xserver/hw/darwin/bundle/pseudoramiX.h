/*
 * Minimal implementation of PanoramiX/Xinerama
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/pseudoramiX.h,v 1.1 2001/12/22 05:28:34 torrey Exp $ */

extern int noPseudoramiXExtension;

void PseudoramiXAddScreen(int x, int y, int w, int h);
void PseudoramiXExtensionInit(int argc, char *argv[]);

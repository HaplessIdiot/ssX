/*
 * Minimal implementation of PanoramiX/Xinerama
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/pseudoramiX.h,v 1.2 2001/12/30 03:56:40 torrey Exp $ */

extern int noPseudoramiXExtension;
extern int aquaNumScreens;

void PseudoramiXAddScreen(int x, int y, int w, int h);
void PseudoramiXExtensionInit(int argc, char *argv[]);

/*
 * Rootless implementation for Mac OS X Aqua environment
 *
 * Greg Parker     gparker@cs.stanford.edu
 */
/* $XFree86: $ */

#ifndef _ROOTLESSAQUAIMP_H
#define _ROOTLESSAQUAIMP_H

// This struct is byte-compatible with X11's BoxRec, for use in 
// code that can't include X headers.
typedef struct _fakeBox {
    short x1;
    short y1;
    short x2;
    short y2;
} fakeBoxRec;


void AquaDisplayInit(int *width, int *height, int *rowBytes, 
                     unsigned long *bps, unsigned long *spp, int *bpp);

void *AquaNewWindow(void *upperw, int x, int y, int w, int h, int isRoot);

void AquaDestroyWindow(void *rw);

void AquaMoveWindow(void *rw, int x, int y);

void AquaStartResizeWindow(void *rw, int x, int y, int w, int h);

void AquaFinishResizeWindow(void *rw, int x, int y, int w, int h);

void AquaUpdateRects(void *rw, fakeBoxRec *rects, int count);

void AquaRestackWindow(void *rw, void *lowerw);

void AquaReshapeWindow(void *rw, fakeBoxRec *rects, int count);

void AquaStartDrawing(void *rw, char **bits, 
                      int *rowBytes, int *depth, int *bpp);

void AquaStopDrawing(void *rw);

#endif /* _ROOTLESSAQUAIMP_H */

/* $XFree86$ */

#ifndef _SHADOWFB_H
#define _SHADOWFB_H

typedef void (*RefreshAreaFuncPtr)(ScrnInfoPtr, int, BoxPtr);

Bool
ShadowFBInit (
    ScreenPtr		pScreen,
    RefreshAreaFuncPtr  refreshArea
);

#endif /* _SHADOWFB_H */





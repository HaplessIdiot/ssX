/*
 * Cocoa rootless implementation initialization
 */
/*
 * Copyright (c) 2001 Greg Parker. All Rights Reserved.
 * Copyright (c) 2002-2003 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/quartz/cr/crScreen.m,v 1.1 2003/06/07 05:49:07 torrey Exp $ */

#include "quartzCommon.h"
#include "cr.h"

#undef BOOL
#define BOOL xBOOL
#include "darwin.h"
#include "rootlessAqua.h"
#include "rootless.h"
#include "aqua.h"
#include "pseudoramiX.h"

#include "regionstr.h"
#include "scrnintstr.h"
#include "picturestr.h"
#undef BOOL

// Name of GLX bundle using AGL framework
const char *quartzOpenGLBundle = "glxAGL.bundle";


/*
 * AquaDisplayInit
 *  Find all screens.
 *
 *  Multihead note: When rootless mode uses PseudoramiX, the
 *  X server only sees one screen; only PseudoramiX itself knows
 *  about all of the screens.
 */
void
AquaDisplayInit(void)
{
    if (noPseudoramiXExtension) {
        darwinScreensFound = [[NSScreen screens] count];
    } else {
        darwinScreensFound = 1; // only PseudoramiX knows about the rest
    }
}


/*
 * AquaScreenParams
 *  Set the basic screen parameters.
 */
static void
AquaScreenParams(int index, DarwinFramebufferPtr dfb)
{
    dfb->bitsPerComponent = CGDisplayBitsPerSample(kCGDirectMainDisplay);
    dfb->bitsPerPixel = CGDisplayBitsPerPixel(kCGDirectMainDisplay);
    dfb->colorBitsPerPixel = 3 * dfb->bitsPerComponent;

    if (noPseudoramiXExtension) {
        NSScreen *screen = [[NSScreen screens] objectAtIndex:index];
        NSRect frame = [screen frame];

        // set x, y so (0,0) is top left of main screen
        dfb->x = NSMinX(frame);
        dfb->y = NSHeight([[NSScreen mainScreen] frame]) -
                 NSHeight(frame) - NSMinY(frame);

        dfb->width =  NSWidth(frame);
        dfb->height = NSHeight(frame);
        dfb->pitch = (dfb->width) * (dfb->bitsPerPixel) / 8;

        // Shift the usable part of main screen down to avoid the menu bar.
        if (NSEqualRects(frame, [[NSScreen mainScreen] frame])) {
            dfb->y      += aquaMenuBarHeight;
            dfb->height -= aquaMenuBarHeight;
        }

    } else {
        int i;
        NSRect unionRect = NSMakeRect(0, 0, 0, 0);
        NSArray *screens = [NSScreen screens];

        // Get the union of all screens (minus the menu bar on main screen)
        for (i = 0; i < [screens count]; i++) {
            NSScreen *screen = [screens objectAtIndex:i];
            NSRect frame = [screen frame];
            frame.origin.y = [[NSScreen mainScreen] frame].size.height -
                             frame.size.height - frame.origin.y;
            if (NSEqualRects([screen frame], [[NSScreen mainScreen] frame])) {
                frame.origin.y    += aquaMenuBarHeight;
                frame.size.height -= aquaMenuBarHeight;
            }
            unionRect = NSUnionRect(unionRect, frame);
        }

        // Use unionRect as the screen size for the X server.
        dfb->x = unionRect.origin.x;
        dfb->y = unionRect.origin.y;
        dfb->width = unionRect.size.width;
        dfb->height = unionRect.size.height;
        dfb->pitch = (dfb->width) * (dfb->bitsPerPixel) / 8;

        // Tell PseudoramiX about the real screens.
        // InitOutput() will move the big screen to (0,0),
        // so compensate for that here.
        for (i = 0; i < [screens count]; i++) {
            NSScreen *screen = [screens objectAtIndex:i];
            NSRect frame = [screen frame];
            int j;

            // Skip this screen if it's a mirrored copy of an earlier screen.
            for (j = 0; j < i; j++) {
                if (NSEqualRects(frame, [[screens objectAtIndex:j] frame])) {
                    ErrorF("PseudoramiX screen %d is a mirror of screen %d.\n",
                           i, j);
                    break;
                }
            }
            if (j < i) continue; // this screen is a mirrored copy

            frame.origin.y = [[NSScreen mainScreen] frame].size.height -
                             frame.size.height - frame.origin.y;

            if (NSEqualRects([screen frame], [[NSScreen mainScreen] frame])) {
                frame.origin.y    += aquaMenuBarHeight;
                frame.size.height -= aquaMenuBarHeight;
            }

            ErrorF("PseudoramiX screen %d added: %dx%d @ (%d,%d).\n", i,
                   (int)frame.size.width, (int)frame.size.height,
                   (int)frame.origin.x, (int)frame.origin.y);

            frame.origin.x -= unionRect.origin.x;
            frame.origin.y -= unionRect.origin.y;

            ErrorF("PseudoramiX screen %d placed at X11 coordinate (%d,%d).\n",
                   i, (int)frame.origin.x, (int)frame.origin.y);

            PseudoramiXAddScreen(frame.origin.x, frame.origin.y,
                                 frame.size.width, frame.size.height);
        }
    }
}


/*
 * AquaAddScreen
 *  Init the framebuffer and record pixmap parameters for the screen.
 */
Bool
AquaAddScreen(int index, ScreenPtr pScreen)
{
    DarwinFramebufferPtr dfb = SCREEN_PRIV(pScreen);
    QuartzScreenPtr displayInfo = QUARTZ_PRIV(pScreen);
    CGRect cgRect;
    CGDisplayCount numDisplays;
    CGDisplayCount allocatedDisplays = 0;
    CGDirectDisplayID *displays = NULL;
    CGDisplayErr cgErr;

    AquaScreenParams(index, dfb);

    dfb->colorType = TrueColor;

    // No frame buffer - it's all in window pixmaps.
    dfb->framebuffer = NULL; // malloc(dfb.pitch * dfb.height);

    // Get all CoreGraphics displays covered by this X11 display.
    cgRect = CGRectMake(dfb->x, dfb->y, dfb->width, dfb->height);
    do {
        cgErr = CGGetDisplaysWithRect(cgRect, 0, NULL, &numDisplays);
        if (cgErr) break;
        allocatedDisplays = numDisplays;
        displays = xrealloc(displays,
                            numDisplays * sizeof(CGDirectDisplayID));
        cgErr = CGGetDisplaysWithRect(cgRect, allocatedDisplays, displays,
                                      &numDisplays);
        if (cgErr != CGDisplayNoErr) break;
    } while (numDisplays > allocatedDisplays);

    if (cgErr != CGDisplayNoErr || numDisplays == 0) {
        ErrorF("Could not find CGDirectDisplayID(s) for X11 screen %d: %dx%d @ %d,%d.\n",
               index, dfb->width, dfb->height, dfb->x, dfb->y);
        return FALSE;
    }

    // This X11 screen covers all CoreGraphics displays we just found.
    // If there's more than one CG display, then video mirroring is on
    // or PseudoramiX is on.
    displayInfo->displayCount = allocatedDisplays;
    displayInfo->displayIDs = displays;

    return TRUE;
}


/*
 * AquaSetupScreen
 *  Setup the screen for rootless access.
 */
Bool
AquaSetupScreen(int index, ScreenPtr pScreen)
{
    // Add Aqua specific replacements for fb screen functions
    pScreen->PaintWindowBackground = AquaPaintWindow;
    pScreen->PaintWindowBorder = AquaPaintWindow;

#ifdef RENDER
    {
        PictureScreenPtr ps = GetPictureScreen(pScreen);
        ps->Composite = AquaComposite;
    }
#endif /* RENDER */

    // Initialize generic rootless code
    return CRInit(pScreen);
}


/*
 * AquaInitInput
 *  Finalize CR specific setup.
 */
void
AquaInitInput(int argc, char **argv)
{
    rootlessGlobalOffsetX = darwinMainScreenX;
    rootlessGlobalOffsetY = darwinMainScreenY;
}

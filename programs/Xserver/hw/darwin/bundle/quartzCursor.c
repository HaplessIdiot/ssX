/**************************************************************
 *
 * Support for using the Quartz Window Manager cursor
 *
 **************************************************************/
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/quartzCursor.c,v 1.1 2001/04/02 05:18:50 torrey Exp $ */

#include "mi.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "mipointrst.h"

#include "quartzCursor.h"

#undef AllocCursor
#define Cursor QD_Cursor
#define WindowPtr QD_WindowPtr
#include <ApplicationServices/ApplicationServices.h>

// Size of the QuickDraw cursor
#define CURSORWIDTH 16
#define CURSORHEIGHT 16

typedef struct {
    int                     qdCursorMode;
    int                     qdCursorVisible;
    int                     serverVisible;
    CursorPtr               latentCursor;
    QueryBestSizeProcPtr    QueryBestSize;
    miPointerSpriteFuncPtr  spriteFuncs;
} QuartzCursorScreenRec, *QuartzCursorScreenPtr;

static int darwinCursorScreenIndex = -1;
static unsigned long darwinCursorGeneration = 0;

#define CURSOR_PRIV(pScreen) \
    ((QuartzCursorScreenPtr)pScreen->devPrivates[darwinCursorScreenIndex].ptr)
#define HIDE_QD_CURSOR(display, visible) \
    if (visible) { CGDisplayHideCursor(display); visible = FALSE; }
#define SHOW_QD_CURSOR(display, visible) \
    if (! visible) { CGDisplayShowCursor(display); visible = TRUE; }

/*
===========================================================================

 Pointer sprite functions

===========================================================================
*/

/*
 * QuartzRealizeCursor
 * Convert the X cursor representation to QuickDraw format if possible.
 */
Bool
QuartzRealizeCursor(
    ScreenPtr pScreen,
    CursorPtr pCursor )
{
    int i, width, height;
    unsigned short rowMask;
    QD_Cursor *curs;
    QuartzCursorScreenPtr ScreenPriv = CURSOR_PRIV(pScreen);

    if(!pCursor || !pCursor->bits)
        return FALSE;

    // if the cursor is too big we use a software cursor
    if ((pCursor->bits->height > CURSORHEIGHT) ||
        (pCursor->bits->width > CURSORWIDTH))
        return (*ScreenPriv->spriteFuncs->RealizeCursor)(pScreen, pCursor);

    // allocate memory for new cursor image
    curs = xalloc( sizeof(QD_Cursor) );
    if (!curs)
        return FALSE;

    // X cursor max size is 32x32 (rowbytes 4).
    // Copy top left 16x16 for now.
    memset(curs->data, 0, 16*16 / 8);
    memset(curs->mask, 0, 16*16 / 8);
    width = min(pCursor->bits->width,  CURSORWIDTH);
    height = min(pCursor->bits->height, CURSORHEIGHT);
    // rowMask: set to 1 for the bits in each row that are in the X cursor
    rowMask = ~((1 << (16 - width)) - 1);

    for (i = 0; i < height; i++) {
        curs->data[i] = rowMask & ((pCursor->bits->source[i*4] << 8) |
                        pCursor->bits->source[i*4+1]);
        curs->mask[i] = rowMask & ((pCursor->bits->mask[i*4] << 8) |
                        pCursor->bits->mask[i*4+1]);
    }
    curs->hotSpot.h = pCursor->bits->xhot;
    if(curs->hotSpot.h > 16)
        curs->hotSpot.h = 16;
    curs->hotSpot.v = pCursor->bits->yhot;
    if(curs->hotSpot.v > 16)
        curs->hotSpot.v = 16;

    // save the result
    pCursor->devPriv[pScreen->myNum] = (pointer) curs;
    return TRUE;
}


/*
 * QuartzUnrealizeCursor
 * Free the storage space associated with a realized cursor.
 */
Bool
QuartzUnrealizeCursor(
    ScreenPtr pScreen,
    CursorPtr pCursor )
{
    QuartzCursorScreenPtr ScreenPriv = CURSOR_PRIV(pScreen);

    if ((pCursor->bits->height > CURSORHEIGHT) ||
        (pCursor->bits->width > CURSORWIDTH)) {
        return (*ScreenPriv->spriteFuncs->UnrealizeCursor)(pScreen, pCursor);
    } else {
        xfree( pCursor->devPriv[pScreen->myNum] );
        return TRUE;
    }
}


/*
 * QuartzSetCursor
 * Set the cursor sprite and position.
 * Use QuickDraw cursor if possible.
 */
static void
QuartzSetCursor(
    ScreenPtr       pScreen,
    CursorPtr       pCursor,
    int             x,
    int             y)
{
    QuartzCursorScreenPtr ScreenPriv = CURSOR_PRIV(pScreen);

    ScreenPriv->latentCursor = pCursor;

    // Don't touch Mac OS cursor if X is hidden!
    if (! ScreenPriv->serverVisible)
        return;

    if (!pCursor) {
        // Remove the cursor completely.
        HIDE_QD_CURSOR(kCGDirectMainDisplay, ScreenPriv->qdCursorVisible);
        if (! ScreenPriv->qdCursorMode)
            (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, 0, x, y);
    }
    else if ((pCursor->bits->height <= CURSORHEIGHT) &&
             (pCursor->bits->width <= CURSORWIDTH)) {
        // Cursor is small enough to use QuickDraw directly.
        QD_Cursor *curs;

        if (! ScreenPriv->qdCursorMode)    // remove the X cursor
            (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, 0, x, y);
        ScreenPriv->qdCursorMode = TRUE;

        curs = (QD_Cursor *) pCursor->devPriv[pScreen->myNum];
        SetCursor(curs);
        SHOW_QD_CURSOR(kCGDirectMainDisplay, ScreenPriv->qdCursorVisible);
    }
    else {
        // Cursor is too big for QuickDraw. Use X software cursor.
        HIDE_QD_CURSOR(kCGDirectMainDisplay, ScreenPriv->qdCursorVisible);
        ScreenPriv->qdCursorMode = FALSE;
        (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, pCursor, x, y);
    }
}


/*
 * QuartzMoveCursor
 * Move the cursor. This is a noop for QuickDraw.
 */
static void
QuartzMoveCursor(
    ScreenPtr   pScreen,
    int         x,
    int         y)
{
    QuartzCursorScreenPtr ScreenPriv = CURSOR_PRIV(pScreen);

    // only the X cursor needs to be explicitly moved
    if (!ScreenPriv->qdCursorMode)
        (*ScreenPriv->spriteFuncs->MoveCursor)(pScreen, x, y);
}


static miPointerSpriteFuncRec quartzSpriteFuncsRec = {
    QuartzRealizeCursor,
    QuartzUnrealizeCursor,
    QuartzSetCursor,
    QuartzMoveCursor
};

/*
===========================================================================

 Pointer screen functions

===========================================================================
*/

/*
 * QuartzCursorOffScreen
 */
static Bool QuartzCursorOffScreen(ScreenPtr *pScreen, int *x, int *y)
{
    return FALSE;
}


/*
 * QuartzCrossScreen
 */
static void QuartzCrossScreen(ScreenPtr pScreen, Bool entering)
{
    return;
}


/*
 * QuartzWarpCursor
 *  Change the cursor position without generating an event or motion history
 */
static void
QuartzWarpCursor(
    ScreenPtr               pScreen,
    int                     x,
    int                     y)
{
    CGDisplayErr            cgErr;
    CGPoint                 cgPoint;
    int                     neverMoved = TRUE;

    if (neverMoved) {
        // Don't move the cursor the first time. This is the jump-to-center 
        // initialization, and it's annoying because we may still be in MacOS.
        neverMoved = FALSE;
        return;
    }

    if (CURSOR_PRIV(pScreen)->serverVisible) {
        cgPoint = CGPointMake(x, y);
        cgErr = CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, cgPoint);
        if (cgErr != CGDisplayNoErr) {
            ErrorF("Could not set cursor position with error code 0x%x.\n",
                    cgErr);
        }
    }
    miPointerWarpCursor(pScreen, x, y);
}


static miPointerScreenFuncRec quartzScreenFuncsRec = {
    QuartzCursorOffScreen,
    QuartzCrossScreen,
    QuartzWarpCursor,
};

/*
===========================================================================

 Other screen functions

===========================================================================
*/

/*
 * QuartzCursorQueryBestSize
 * Handle queries for best cursor size
 */
static void
QuartzCursorQueryBestSize(
   int              class, 
   unsigned short   *width,
   unsigned short   *height,
   ScreenPtr        pScreen)
{
    QuartzCursorScreenPtr ScreenPriv = CURSOR_PRIV(pScreen);

    if (class == CursorShape) {
        *width = CURSORWIDTH;
        *height = CURSORHEIGHT;
    } else {
        (*ScreenPriv->QueryBestSize)(class, width, height, pScreen);
    }
}


/*
 * QuartzInitCursor
 * Initialize cursor support
 */
Bool 
QuartzInitCursor(
    ScreenPtr   pScreen )
{
    QuartzCursorScreenPtr   ScreenPriv;
    miPointerScreenPtr      PointPriv;

    // initialize software cursor handling (always needed as backup)
    if (!miDCInitialize(pScreen, &quartzScreenFuncsRec)) {
        return FALSE;
    }

    // allocate private storage for this screen's QuickDraw cursor info
    if (darwinCursorGeneration != serverGeneration) {
        if ((darwinCursorScreenIndex = AllocateScreenPrivateIndex()) < 0)
            return FALSE;
        darwinCursorGeneration = serverGeneration;
    }

    ScreenPriv = xcalloc( 1, sizeof(QuartzCursorScreenRec) );
    if (!ScreenPriv) return FALSE;

    CURSOR_PRIV(pScreen) = ScreenPriv;

    // override some screen procedures
    ScreenPriv->QueryBestSize = pScreen->QueryBestSize;
    pScreen->QueryBestSize = QuartzCursorQueryBestSize;

    // initialize QuickDraw cursor handling
    PointPriv = (miPointerScreenPtr)
                    pScreen->devPrivates[miPointerScreenIndex].ptr;

    ScreenPriv->spriteFuncs = PointPriv->spriteFuncs;
    PointPriv->spriteFuncs = &quartzSpriteFuncsRec; 

    ScreenPriv->qdCursorMode = 1;
    ScreenPriv->qdCursorVisible = TRUE;
    ScreenPriv->latentCursor = NULL;
    ScreenPriv->serverVisible = FALSE; 
    return TRUE;
}


// X server is hiding. Restore the Aqua cursor.
void QuartzSuspendXCursor(
    ScreenPtr pScreen )
{
    QuartzCursorScreenPtr ScreenPriv = CURSOR_PRIV(pScreen);

    InitCursor();	// restore QuickDraw arrow cursor
    SHOW_QD_CURSOR(kCGDirectMainDisplay, ScreenPriv->qdCursorVisible);

    ScreenPriv->serverVisible = FALSE;
}


// X server is showing. Restore the X cursor.
void QuartzResumeXCursor(
    ScreenPtr pScreen,
    int x,
    int y )
{
    QuartzCursorScreenPtr ScreenPriv = CURSOR_PRIV(pScreen);

    ScreenPriv->serverVisible = TRUE;

    QuartzSetCursor(pScreen, ScreenPriv->latentCursor, x, y);
}

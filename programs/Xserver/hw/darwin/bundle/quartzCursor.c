/**************************************************************
 *
 * Support for using the Quartz Window Manager cursor
 *
 **************************************************************/
/* $XFree86: $ */

#include "mi.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "mipointrst.h"

#undef AllocCursor
#define Cursor QD_Cursor
#define WindowPtr QD_WindowPtr
#include <ApplicationServices/ApplicationServices.h>

// Size of the QuickDraw cursor
#define CURSORWIDTH 16
#define CURSORHEIGHT 16

typedef struct {
    int                     cursorMode;
    QueryBestSizeProcPtr    QueryBestSize;
    miPointerSpriteFuncPtr  spriteFuncs;
} QuartzCursorScreenRec, *QuartzCursorScreenPtr;

static int darwinCursorScreenIndex = -1;
static unsigned long darwinCursorGeneration = 0;

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
    int i;
    QD_Cursor *curs;
    QuartzCursorScreenPtr ScreenPriv = (QuartzCursorScreenPtr)
        pScreen->devPrivates[darwinCursorScreenIndex].ptr;

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
    for (i = 0; i < 16; i++)
    {
        curs->data[i] = (pCursor->bits->source[i*4]<<8) |
                        pCursor->bits->source[i*4+1];
        curs->mask[i] = (pCursor->bits->mask[i*4]<<8) |
                        pCursor->bits->mask[i*4+1];
    }
    curs->hotSpot.h = pCursor->bits->xhot;
    if(curs->hotSpot.h >= 16)
        curs->hotSpot.h = 15;
    curs->hotSpot.v = pCursor->bits->yhot;
    if(curs->hotSpot.v >= 16)
        curs->hotSpot.v = 15;

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
    QuartzCursorScreenPtr ScreenPriv = (QuartzCursorScreenPtr)
        pScreen->devPrivates[darwinCursorScreenIndex].ptr;

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
    QD_Cursor *curs;
    QuartzCursorScreenPtr ScreenPriv = (QuartzCursorScreenPtr)
        pScreen->devPrivates[darwinCursorScreenIndex].ptr;

    // are we supposed to remove the cursor?
    if (!pCursor) {
        if (ScreenPriv->cursorMode == 0)
            (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, 0, x, y);
        else
            CGDisplayHideCursor(kCGDirectMainDisplay);
        return;
    }

    // can we use QuickDraw cursor?
    if ((pCursor->bits->height <= CURSORHEIGHT) &&
        (pCursor->bits->width <= CURSORWIDTH)) {

        if (ScreenPriv->cursorMode == 0)    // remove the X cursor
            (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, 0, x, y);

        ScreenPriv->cursorMode = 1;
        curs = (QD_Cursor *) pCursor->devPriv[pScreen->myNum];
        SetCursor(curs);
        return;
    }

    // otherwise we use a software cursor
    if (ScreenPriv->cursorMode) {
        // remove the QuickDraw cursor
        QuartzSetCursor(pScreen, 0, x, y);
    }

    ScreenPriv->cursorMode = 0;
    (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, pCursor, x, y);
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
    QuartzCursorScreenPtr ScreenPriv = (QuartzCursorScreenPtr)
        pScreen->devPrivates[darwinCursorScreenIndex].ptr;

    // only the X cursor needs to be explicitly moved
    if (!ScreenPriv->cursorMode)
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

    cgPoint = CGPointMake(x, y);
    cgErr = CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, cgPoint);
    if (cgErr != CGDisplayNoErr) {
        ErrorF("Could not set cursor position with error code 0x%x.\n", cgErr);
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
    QuartzCursorScreenPtr ScreenPriv = (QuartzCursorScreenPtr)
        pScreen->devPrivates[darwinCursorScreenIndex].ptr;

    if (class == CursorShape) {
        *width = CURSORWIDTH;
        *height = CURSORHEIGHT;
    } else
        (*ScreenPriv->QueryBestSize)(class, width, height, pScreen);
}


/*
 * QuartzInitCursor
 * Initialize cursor support
 */
Bool 
QuartzInitCursor(
    ScreenPtr	pScreen )
{
    QuartzCursorScreenPtr   ScreenPriv;
    miPointerScreenPtr	    PointPriv;

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

    pScreen->devPrivates[darwinCursorScreenIndex].ptr = (pointer) ScreenPriv;

    // override some screen procedures
    ScreenPriv->QueryBestSize = pScreen->QueryBestSize;
    pScreen->QueryBestSize = QuartzCursorQueryBestSize;

    // initialize QuickDraw cursor handling
    PointPriv = (miPointerScreenPtr)
                    pScreen->devPrivates[miPointerScreenIndex].ptr;

    ScreenPriv->spriteFuncs = PointPriv->spriteFuncs;
    PointPriv->spriteFuncs = &quartzSpriteFuncsRec; 

    ScreenPriv->cursorMode = 1;
    return TRUE;
}

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_hwcurs.c,v 1.5 1998/07/25 16:55:53 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "xf86Cursor.h"

#include "vgaHWmmio.h"

#include "mga_bios.h"
#include "mga.h"
#include "mga_reg.h"
#include "mga_map.h"
#include "mga_macros.h"

Bool 
MGAHWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    MGAPtr pMga = MGAPTR(pScrn);
    MGARamdacPtr MGAdac = &pMga->Dac;
    XAACursorInfoPtr infoPtr;

    if (!MGAdac->isHwCursor) 
        return FALSE;

    infoPtr = XAACreateCursorInfoRec();
    if(!infoPtr) return FALSE;
    
    pMga->CursorInfoRec = infoPtr;

    infoPtr->MaxWidth = MGAdac->CursorMaxWidth;
    infoPtr->MaxHeight = MGAdac->CursorMaxHeight;
    infoPtr->Flags = MGAdac->CursorFlags;
    infoPtr->SetCursorColors = MGAdac->SetCursorColors;
    infoPtr->SetCursorPosition = MGAdac->SetCursorPosition;
    infoPtr->LoadCursorImage = MGAdac->LoadCursorImage;
    infoPtr->HideCursor = MGAdac->HideCursor;
    infoPtr->ShowCursor = MGAdac->ShowCursor;
    infoPtr->UseHWCursor = MGAdac->UseHWCursor;

    return(XAAInitCursor(pScreen, infoPtr));
}

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_cursor.c,v 1.6 2001/09/28 07:47:22 alanh Exp $ */
/*
 * Copyright 1998,1999 by Alan Hourihane, Wigan, England.
 * Parts Copyright 2001, 2002 by Thomas Winischhofer, Vienna, Austria.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *           Mike Chapman <mike@paranoia.com>,
 *           Juanjo Santamarta <santamarta@ctv.es>,
 *           Mitani Hiroshi <hmitani@drl.mei.co.jp>
 *           David Thomas <davtom@dream.org.uk>.
 *	     Thomas Winischhofer <thomas@winischhofer.net>:
 */

#include "xf86.h"
#include "xf86PciInfo.h"
#include "cursorstr.h"
#include "vgaHW.h"

#include "sis.h"
#include "sis_regs.h"
#include "sis_cursor.h"

static void
SiSShowCursor(ScrnInfoPtr pScrn)
{
    SISPtr  pSiS = SISPTR(pScrn);

#ifdef UNLOCK_ALWAYS
    sisSaveUnlockExtRegisterLock(pSiS, NULL);
#endif

    orSISIDXREG(SISSR, 0x06, 0x40);
}

static void
SiS300ShowCursor(ScrnInfoPtr pScrn)
{
    SISPtr  pSiS = SISPTR(pScrn);

#ifdef SISDUALHEAD
    if (pSiS->DualHeadMode) {
	if (pSiS->SecondHead) {
		/* TW: Head 2 is always CRT1 */
		sis300EnableHWCursor()
        } else {
		/* TW: Head 1 is always CRT2 */
		sis301EnableHWCursor()
	}
    } else {
#endif
    	sis300EnableHWCursor()
    	if (pSiS->VBFlags & CRT2_ENABLE)  {
        	sis301EnableHWCursor()
      	}
#ifdef SISDUALHEAD
    }
#endif
}

/* TW: 310/325 series */
static void
SiS310ShowCursor(ScrnInfoPtr pScrn)
{
    SISPtr  pSiS = SISPTR(pScrn);

#ifdef SISDUALHEAD
    if (pSiS->DualHeadMode) {
	if (pSiS->SecondHead) {
		/* TW: Head 2 is always CRT1 */
		sis310EnableHWCursor()
        } else {
		/* TW: Head 1 is always CRT2 */
		sis301EnableHWCursor310()
	}
    } else {
#endif
    	sis310EnableHWCursor()
    	if (pSiS->VBFlags & CRT2_ENABLE)  {
        	sis301EnableHWCursor310()
      	}
#ifdef SISDUALHEAD
    }
#endif
}

static void
SiSHideCursor(ScrnInfoPtr pScrn)
{
    SISPtr  pSiS = SISPTR(pScrn);

#ifdef UNLOCK_ALWAYS
    sisSaveUnlockExtRegisterLock(pSiS, NULL);
#endif

    andSISIDXREG(SISSR, 0x06, 0xBF);
}

static void
SiS300HideCursor(ScrnInfoPtr pScrn)
{
    SISPtr  pSiS = SISPTR(pScrn);
    
#ifdef SISDUALHEAD
    if (pSiS->DualHeadMode) {
	if (pSiS->SecondHead) {
		/* TW: Head 2 is always CRT1 */
		sis300DisableHWCursor()
		sis300SetCursorPositionY(2000, 0)
        } else {
		/* TW: Head 1 is always CRT2 */
		sis301DisableHWCursor()
		sis301SetCursorPositionY(2000, 0)
	}
    } else {
#endif
    	sis300DisableHWCursor()
	sis300SetCursorPositionY(2000, 0)
    	if (pSiS->VBFlags & CRT2_ENABLE)  {
        	sis301DisableHWCursor()
		sis301SetCursorPositionY(2000, 0)
    	}
#ifdef SISDUALHEAD
    }
#endif
}

/* TW: 310/325 series */
static void
SiS310HideCursor(ScrnInfoPtr pScrn)
{
    SISPtr  pSiS = SISPTR(pScrn);

#ifdef SISDUALHEAD
    if (pSiS->DualHeadMode) {
	if (pSiS->SecondHead) {
		/* TW: Head 2 is always CRT1 */
		sis310DisableHWCursor()
		sis310SetCursorPositionY(2000, 0)
        } else {
		/* TW: Head 1 is always CRT2 */
		sis301DisableHWCursor310()
		sis301SetCursorPositionY310(2000, 0)
	}
    } else {
#endif
    	sis310DisableHWCursor()
	sis310SetCursorPositionY(2000, 0)
    	if (pSiS->VBFlags & CRT2_ENABLE)  {
        	sis301DisableHWCursor310()
		sis301SetCursorPositionY310(2000, 0)
    	}
#ifdef SISDUALHEAD
    }
#endif
}

static void
SiSSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    SISPtr        pSiS     = SISPTR(pScrn);
    unsigned char x_preset = 0;
    unsigned char y_preset = 0;
    int           temp;

#ifdef UNLOCK_ALWAYS
    sisSaveUnlockExtRegisterLock(pSiS,NULL);
#endif

    if (x < 0) {
        x_preset = (-x);
        x = 0;
    }

    if (y < 0) {
        y_preset = (-y);
        y = 0;
    }

    /* are we in interlaced/doublescan mode? */
    if (pScrn->currentMode->Flags & V_INTERLACE)
        y /= 2;
    else if (pScrn->currentMode->Flags & V_DBLSCAN)
        y *= 2;

    outSISIDXREG(SISSR, 0x1A, x & 0xff);
    outSISIDXREG(SISSR, 0x1B, (x & 0xff00) >> 8);
    outSISIDXREG(SISSR, 0x1D, y & 0xff);

    inSISIDXREG(SISSR, 0x1E, temp);
    temp &= 0xF8;
    outSISIDXREG(SISSR, 0x1E, temp | ((y >> 8) & 0x07));

    outSISIDXREG(SISSR, 0x1C, x_preset);
    outSISIDXREG(SISSR, 0x1F, y_preset);
}

static void
SiS300SetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    SISPtr  pSiS = SISPTR(pScrn);

    unsigned char   x_preset = 0;
    unsigned char   y_preset = 0;

    if (x < 0) {
        x_preset = (-x);
        x = 0;
    }
    if (y < 0) {
        y_preset = (-y);
        y = 0;
    }

    /* are we in interlaced/doublescan mode? */
    if (pScrn->currentMode->Flags & V_INTERLACE)
        y /= 2;
    else if (pScrn->currentMode->Flags & V_DBLSCAN)
        y *= 2;

#ifdef SISDUALHEAD
    if (pSiS->DualHeadMode) {
	if (pSiS->SecondHead) {
		/* TW: Head 2 is always CRT1 */
		sis300SetCursorPositionX(x, x_preset)
    		sis300SetCursorPositionY(y, y_preset)
        } else {
		/* TW: Head 1 is always CRT2 */
		sis301SetCursorPositionX(x+13, x_preset)
      		sis301SetCursorPositionY(y, y_preset)
	}
    } else {
#endif
    	sis300SetCursorPositionX(x, x_preset)
    	sis300SetCursorPositionY(y, y_preset)
    	if (pSiS->VBFlags & CRT2_ENABLE) {
      		sis301SetCursorPositionX(x+13, x_preset)
      		sis301SetCursorPositionY(y, y_preset)
    	}
#ifdef SISDUALHEAD
    }
#endif
}

static void
SiS310SetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    SISPtr  pSiS = SISPTR(pScrn);

    unsigned char   x_preset = 0;
    unsigned char   y_preset = 0;
    int             vbxoffs;

    if (x < 0) {
        x_preset = (-x);
        x = 0;
    }
    if (y < 0) {
        y_preset = (-y);
        y = 0;
    }

    /* are we in interlaced/doublescan mode? */
    if (pScrn->currentMode->Flags & V_INTERLACE)
        y /= 2;
    else if (pScrn->currentMode->Flags & V_DBLSCAN)
        y *= 2;

    switch(pSiS->VBFlags & VB_VIDEOBRIDGE) {
    case VB_30xLV:
    case VB_30xLVX:
        vbxoffs = 17;
        break;
    default:
        vbxoffs = 13;
    }

#ifdef SISDUALHEAD
    if (pSiS->DualHeadMode) {
	if (pSiS->SecondHead) {
		/* TW: Head 2 is always CRT1 */
		sis310SetCursorPositionX(x, x_preset)
    		sis310SetCursorPositionY(y, y_preset)
        } else {
		/* TW: Head 1 is always CRT2 */
		sis301SetCursorPositionX310(x + vbxoffs, x_preset)
      		sis301SetCursorPositionY310(y, y_preset)
	}
    } else {
#endif
    	sis310SetCursorPositionX(x, x_preset)
    	sis310SetCursorPositionY(y, y_preset)
    	if (pSiS->VBFlags & CRT2_ENABLE) {
      		sis301SetCursorPositionX310(x + vbxoffs, x_preset)
      		sis301SetCursorPositionY310(y, y_preset)
    	}
#ifdef SISDUALHEAD
    }
#endif
}

static void
SiSSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    SISPtr pSiS = SISPTR(pScrn);
    unsigned char f_red, f_green, f_blue;
    unsigned char b_red, b_green, b_blue;

#ifdef UNLOCK_ALWAYS
    sisSaveUnlockExtRegisterLock(pSiS, NULL);
#endif

    f_red   = (fg & 0x00FF0000) >> (16+2);
    f_green = (fg & 0x0000FF00) >> (8+2);
    f_blue  = (fg & 0x000000FF) >> 2;
    b_red   = (bg & 0x00FF0000) >> (16+2);
    b_green = (bg & 0x0000FF00) >> (8+2);
    b_blue  = (bg & 0x000000FF) >> 2;

    outSISIDXREG(SISSR, 0x14, b_red);
    outSISIDXREG(SISSR, 0x15, b_green);
    outSISIDXREG(SISSR, 0x16, b_blue);
    outSISIDXREG(SISSR, 0x17, f_red);
    outSISIDXREG(SISSR, 0x18, f_green);
    outSISIDXREG(SISSR, 0x19, f_blue);
}

static void
SiS300SetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    SISPtr pSiS = SISPTR(pScrn);

#ifdef SISDUALHEAD
    if (pSiS->DualHeadMode) {
	if (pSiS->SecondHead) {
		/* TW: Head 2 is always CRT1 */
    		sis300SetCursorBGColor(bg)
    		sis300SetCursorFGColor(fg)
        } else {
		/* TW: Head 1 is always CRT2 */
        	sis301SetCursorBGColor(bg)
        	sis301SetCursorFGColor(fg)
	}
    } else {
#endif
    	sis300SetCursorBGColor(bg)
    	sis300SetCursorFGColor(fg)
    	if (pSiS->VBFlags & CRT2_ENABLE)  {
        	sis301SetCursorBGColor(bg)
        	sis301SetCursorFGColor(fg)
    	}
#ifdef SISDUALHEAD
    }
#endif
}

static void
SiS310SetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    SISPtr pSiS = SISPTR(pScrn);

#ifdef SISDUALHEAD
    if (pSiS->DualHeadMode) {
	if (pSiS->SecondHead) {
		/* TW: Head 2 is always CRT1 */
    		sis310SetCursorBGColor(bg)
    		sis310SetCursorFGColor(fg)
        } else {
		/* TW: Head 1 is always CRT2 */
        	sis301SetCursorBGColor310(bg)
        	sis301SetCursorFGColor310(fg)
	}
    } else {
#endif
    	sis310SetCursorBGColor(bg)
    	sis310SetCursorFGColor(fg)
    	if (pSiS->VBFlags & CRT2_ENABLE)  {
        	sis301SetCursorBGColor310(bg)
        	sis301SetCursorFGColor310(fg)
    	}
#ifdef SISDUALHEAD
    }
#endif
}

static void
SiSLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
    SISPtr pSiS = SISPTR(pScrn);
    int cursor_addr;
    unsigned char   temp;

#ifdef UNLOCK_ALWAYS
    sisSaveUnlockExtRegisterLock(pSiS,NULL);
#endif

    cursor_addr = pScrn->videoRam - 1;
    if(pSiS->CurrentLayout.mode->Flags & V_DBLSCAN) {
      int i;
      for(i = 0; i < 32; i++) {
         memcpy((unsigned char *)pSiS->FbBase + (cursor_addr * 1024) + (32 * i),
	            src + (16 * i), 16);
         memcpy((unsigned char *)pSiS->FbBase + (cursor_addr * 1024) + (32 * i) + 16,
	 	    src + (16 * i), 16);
      }
    } else {
      memcpy((unsigned char *)pSiS->FbBase + (cursor_addr * 1024), src, 1024);
    }

    /* copy bits [21:18] into the top bits of SR38 */
    inSISIDXREG(SISSR, 0x38, temp);
    temp &= 0x0F;
    outSISIDXREG(SISSR, 0x38, temp | ((cursor_addr & 0xF00) >> 4));

    if(pSiS->Chipset == PCI_CHIP_SIS530) {
       /* store the bit [22] to SR3E */
       if(cursor_addr & 0x1000) {
          orSISIDXREG(SISSR, 0x3E, 0x04);
       } else {
          andSISIDXREG(SISSR, 0x3E, ~0x04);
       }
    }

    /* set HW cursor pattern, use pattern 0xF */
    orSISIDXREG(SISSR, 0x1E, 0xF0);

    /* disable the hardware cursor side pattern */
    andSISIDXREG(SISSR, 0x1E, 0xF7);
}

static void
SiS300LoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
    SISPtr pSiS = SISPTR(pScrn);
    int cursor_addr;

#ifdef SISDUALHEAD
    SISEntPtr pSiSEnt = NULL;
#endif

#ifdef SISDUALHEAD
    pSiSEnt = pSiS->entityPrivate;
#endif

    cursor_addr = pScrn->videoRam - pSiS->cursorOffset - (pSiS->CursorSize/1024);  /* 1K boundary */

#ifdef SISDUALHEAD
    if (pSiS->DualHeadMode) {
    	/* TW: Use the global (real) FbBase in DHM */
    	memcpy((unsigned char *)pSiSEnt->FbBase + (cursor_addr * 1024), src, 1024);
    } else
#endif
      if(pSiS->CurrentLayout.mode->Flags & V_DBLSCAN) {
        int i;
        for(i = 0; i < 32; i++) {
           memcpy((unsigned char *)pSiS->FbBase + (cursor_addr * 1024) + (32 * i),
	            src + (16 * i), 16);
           memcpy((unsigned char *)pSiS->FbBase + (cursor_addr * 1024) + (32 * i) + 16,
	 	    src + (16 * i), 16);
        }
    } else {
    	memcpy((unsigned char *)pSiS->FbBase + (cursor_addr * 1024), src, 1024);
    }


    /* TW: No special handling for DUAL HEAD needed here */
    sis300SetCursorAddress(cursor_addr)
    sis300SetCursorPatternSelect(0)
    if (pSiS->VBFlags & CRT2_ENABLE)  {
        sis301SetCursorAddress(cursor_addr)
        sis301SetCursorPatternSelect(0)
    }
}

static void
SiS310LoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
    SISPtr pSiS = SISPTR(pScrn);
    int cursor_addr;

#ifdef SISDUALHEAD
    SISEntPtr pSiSEnt = NULL;
#endif

#ifdef SISDUALHEAD
    pSiSEnt = pSiS->entityPrivate;
#endif

    cursor_addr = pScrn->videoRam - pSiS->cursorOffset - (pSiS->CursorSize/1024); /* 1K boundary */

#ifdef SISDUALHEAD
    if (pSiS->DualHeadMode)
    	/* TW: Use the global (real) FbBase in DHM */
    	memcpy((unsigned char *)pSiSEnt->FbBase + (cursor_addr * 1024), src, 1024);
    else
#endif
      if(pSiS->CurrentLayout.mode->Flags & V_DBLSCAN) {
        int i;
        for(i = 0; i < 32; i++) {
           memcpy((unsigned char *)pSiS->FbBase + (cursor_addr * 1024) + (32 * i),
	            src + (16 * i), 16);
           memcpy((unsigned char *)pSiS->FbBase + (cursor_addr * 1024) + (32 * i) + 16,
	 	    src + (16 * i), 16);
        }
    } else {
    	memcpy((unsigned char *)pSiS->FbBase + (cursor_addr * 1024), src, 1024);
    }

    /* TW: No special handling for DUAL HEAD needed here  */
    sis310SetCursorAddress(cursor_addr)
    sis310SetCursorPatternSelect(0)
    if (pSiS->VBFlags & (CRT2_ENABLE | VB_VIDEOBRIDGE))  {  /* @@@ */
        sis301SetCursorAddress310(cursor_addr)
        sis301SetCursorPatternSelect310(0)
    }
}

static Bool
SiSUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DisplayModePtr  mode = pScrn->currentMode;
    SISPtr  pSiS = SISPTR(pScrn);

    if(pSiS->Chipset != PCI_CHIP_SIS6326) return TRUE;
    if(!(pSiS->SiS6326Flags & SIS6326_TVDETECTED)) return TRUE;
    if((strcmp(mode->name, "PAL800x600U") == 0) ||
       (strcmp(mode->name, "NTSC640x480U") == 0))
      return FALSE;
    else
      return TRUE;
}

static Bool
SiS300UseHWCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DisplayModePtr  mode = pScrn->currentMode;
    SISPtr  pSiS = SISPTR(pScrn);

    switch (pSiS->Chipset)  {
      case PCI_CHIP_SIS300:
      case PCI_CHIP_SIS630:
      case PCI_CHIP_SIS540:
        if(mode->Flags & V_INTERLACE)
            return FALSE;
        break;
      case PCI_CHIP_SIS550:
      case PCI_CHIP_SIS650:
      case PCI_CHIP_SIS315:
      case PCI_CHIP_SIS315H:
      case PCI_CHIP_SIS315PRO:
      case PCI_CHIP_SIS330:
        if(mode->Flags & V_INTERLACE)
            return FALSE;
        break;
    }
    return TRUE;
}

Bool
SiSHWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    SISPtr pSiS = SISPTR(pScrn);
    xf86CursorInfoPtr infoPtr;

    PDEBUG(ErrorF("HW Cursor Init\n"));
    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) 
        return FALSE;

    pSiS->CursorInfoPtr = infoPtr;

    switch (pSiS->Chipset)  {
      case PCI_CHIP_SIS300:
      case PCI_CHIP_SIS630:
      case PCI_CHIP_SIS540:
        infoPtr->MaxWidth  = 64;
        infoPtr->MaxHeight = (pSiS->CurrentLayout.mode->Flags & V_DBLSCAN) ? 32 : 64;
        infoPtr->ShowCursor = SiS300ShowCursor;
        infoPtr->HideCursor = SiS300HideCursor;
        infoPtr->SetCursorPosition = SiS300SetCursorPosition;
        infoPtr->SetCursorColors = SiS300SetCursorColors;
        infoPtr->LoadCursorImage = SiS300LoadCursorImage;
        infoPtr->UseHWCursor = SiS300UseHWCursor;
        infoPtr->Flags =
            HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
            HARDWARE_CURSOR_INVERT_MASK |
            HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
            HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
            HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK |
            HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64;
        break;
      case PCI_CHIP_SIS550:
      case PCI_CHIP_SIS650:
      case PCI_CHIP_SIS315:
      case PCI_CHIP_SIS315H:
      case PCI_CHIP_SIS315PRO:
      case PCI_CHIP_SIS330:
        infoPtr->MaxWidth  = 64;   
        infoPtr->MaxHeight = (pSiS->CurrentLayout.mode->Flags & V_DBLSCAN) ? 32 : 64;
        infoPtr->ShowCursor = SiS310ShowCursor;
        infoPtr->HideCursor = SiS310HideCursor;
        infoPtr->SetCursorPosition = SiS310SetCursorPosition;
        infoPtr->SetCursorColors = SiS310SetCursorColors;
        infoPtr->LoadCursorImage = SiS310LoadCursorImage;
        infoPtr->UseHWCursor = SiS300UseHWCursor;
        infoPtr->Flags =
            HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
            HARDWARE_CURSOR_INVERT_MASK |
            HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
            HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
            HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK |
            HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64;
        break;
      default:
        infoPtr->MaxWidth  = 64;
	infoPtr->MaxHeight = (pSiS->CurrentLayout.mode->Flags & V_DBLSCAN) ? 32 : 64;
        infoPtr->SetCursorPosition = SiSSetCursorPosition;
        infoPtr->ShowCursor = SiSShowCursor;
        infoPtr->HideCursor = SiSHideCursor;
        infoPtr->SetCursorColors = SiSSetCursorColors;
        infoPtr->LoadCursorImage = SiSLoadCursorImage;
        infoPtr->UseHWCursor = SiSUseHWCursor;
        infoPtr->Flags =
            HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
            HARDWARE_CURSOR_INVERT_MASK |
            HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
            HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
            HARDWARE_CURSOR_NIBBLE_SWAPPED |
            HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1;
        break;
    }

    return(xf86InitCursor(pScreen, infoPtr));
}

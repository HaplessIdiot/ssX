/* $XFree86: $ */

/*
 * Copyright 1994  The XFree86 Project
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * DAVID WEXELBLAT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Hardware Cursor for Trident utilizing XAA Cursor code.
 * Written by Alan Hourihane <alanh@fairlite.demon.co.uk>
 * Modified for Chips and Technologies by David Bateman <dbateman@eng.uts.edu.au>
 */

#include "vga.h"
#include "scrnintstr.h"
#include "xf86_OSlib.h"
#include "xf86cursor.h"
#include "ct_driver.h"
#include <xf86Priv.h>

extern vgaHWCursorRec vgaHWCursor;
Bool ctHWcursorShown = FALSE;
Bool ctDBLSCAN = FALSE;
extern Bool XAACursorInit();
extern void XAARestoreCursor();
extern void XAAWarpCursor();
extern void XAAQueryBestSize();

void CHIPSShowCursor() {
    unsigned char tmp;
    /* turn the cursor on */
    if (ctisHiQV32) {
        outb(0x3D6, 0xA0);
        tmp = inb(0x3D7);
	outb(0x3D7, ((tmp & 0xF8) | 5));
    } else
      if(!ctUseMMIO) {
	HW_DEBUG(0x8);
	outw(DR(0x8), 0x21);
      } else {
	HW_DEBUG(DR(8));
	MMIOmemw(MR(8)) = 0x21;
      }
    ctHWcursorShown = TRUE;
}

void CHIPSHideCursor() {
    unsigned char tmp;
  
    /* turn the cursor off */
    if (ctisHiQV32) {
        outb(0x3D6, 0xA0);
        tmp = inb(0x3D7);
	outb(0x3D7, (tmp & 0xF8));
    } else
      if(!ctUseMMIO) {
	HW_DEBUG(0x8);
	outw(DR(0x8), 0x20);
      } else {
	HW_DEBUG(DR(0x8));
	MMIOmemw(DR(0x8)) = 0x20;
      }
    ctHWcursorShown = FALSE;
}

void CHIPSSetCursorPosition(x, y, xorigin, yorigin)
	int x, y, xorigin, yorigin;
{

    if (xorigin != 0)
      x = ~(-xorigin-1) | 0x8000;
    if (yorigin != 0)
      y = ~(-yorigin-1) | 0x8000;
    
    if (ctDBLSCAN)
	y *= 2;

    /* Program the cursor origin (offset into the cursor bitmap). */
    if (ctisHiQV32) {
	outw(0x3D6, ((x & 0xFF) << 8) | 0xA4);
	outw(0x3D6, (x & 0x8700) | 0xA5);
	outw(0x3D6, ((y & 0xFF) << 8) | 0xA6);
	outw(0x3D6, (y & 0x8700) | 0xA7);
    } else {
	unsigned long xy;

	xy = y;
	xy = (xy << 16) | x;
	if(!ctUseMMIO) {
	  HW_DEBUG(0xB);
	  outl(DR(0xB), xy);
	} else {
	  HW_DEBUG(MR(0xB));
	  MMIOmeml(MR(0xB)) = xy;
	}
    }
}

void
CHIPSSetCursorColors(bg, fg)
	int fg, bg;
{
    unsigned long packedcolfg, packedcolbg;
    
    if (ctisHiQV32) {
	unsigned char xr80;

	outb(0x3D6, 0x80);
	xr80 = inb(0x3D7);
	outb(0x3D7, xr80 | 0x1);       /* Enable extended palette addressing */

	/* Write the new colours to the extended VGA palette. Palette
	 * index is incremented after each write, so only write index
	 * once 
	 */
	outb(0x3C8, 0x4);
	outb(0x3C9, (bg >> 18) & 0x2F);
	outb(0x3C9, (bg >> 10) & 0x2F);
	outb(0x3C9, (bg >> 2) & 0x2F);
	outb(0x3C9, (fg >> 18) & 0x2F);
	outb(0x3C9, (fg >> 10) & 0x2F);
	outb(0x3C9, (fg >> 2) & 0x2F);
	outb(0x3D6, 0x80);
	outb(0x3D7, xr80);	       /* Enable normal palette addressing */
    } else if (ctisWINGINE) {
	outl(DR(0xA), (bg & 0xFFFFFF));
	outl(DR(0x9), (fg & 0xFFFFFF));
    } else {
	packedcolfg =  ((fg & 0xF80000) >> 8) | ((fg & 0xFC00) >> 5)
	    | ((fg & 0xF8) >> 3);
	packedcolbg =  ((bg & 0xF80000) >> 8) | ((bg & 0xFC00) >> 5)
	    | ((bg & 0xF8) >> 3);
	packedcolfg = (packedcolfg << 16) | packedcolbg;
	if(!ctUseMMIO) {
	  HW_DEBUG(0x9);
	  outl(DR(0x9), packedcolfg);
	} else {
	  MMIOmeml(MR(0x9)) = packedcolfg;
	  HW_DEBUG(MR(0x9));
	}
    }
}

void
CHIPSLoadCursorImage(bits, xorigin, yorigin)
	unsigned long *bits;
	int xorigin, yorigin;
{
    int i;

    if (ctisWINGINE) {
      unsigned long *tmp;

      /* The Wingine expected the mask and source data to not be intereleaved.
       * The XAA code isn't currently setup to do this, and as far as I know
       * only the Wingine wants the data in this format. Hence use a 32 bit
       * interleave while realising the cursor and reverse it at this point
       */
      outl(DR(0x8),0x20);
      tmp = bits;
      for (i=0;i<32;i++) {
	outl(DR(0xC),*(unsigned long *)tmp);
	tmp += 2;
      }
      tmp = bits + 1;
      for (i=0;i<32;i++) {
	outl(DR(0xC),*(unsigned long *)tmp);
	tmp += 2;
      }
    } else {
      if (ctLinearSupport) {
	xf86memcpy((unsigned char *)vgaLinearBase + ctCursorAddress,
	    bits, XAACursorInfoRec.MaxWidth * XAACursorInfoRec.MaxHeight / 4);
      } else {
	/*
	 * The cursor can only be in the last 16K of video memory,
	 * which fits in the last banking window.
	 */
	vgaSaveBank();
	if (ctisHiQV32)
	  CHIPSHiQVSetWrite(ctCursorAddress >> 16);
	else
	  CHIPSSetWrite(ctCursorAddress >> 16);
	xf86memcpy((unsigned char *)vgaBase + (ctCursorAddress & 0xFFFF),
	     bits, XAACursorInfoRec.MaxWidth * XAACursorInfoRec.MaxWidth / 4);
	vgaRestoreBank();
      }
    }

    /* set cursor address here or we loose the cursor on video mode change */
    if (ctisHiQV32) {
      outb(0x3D6, 0xA2);
      outb(0x3D7, (ctCursorAddress >> 8) & 0xFF);
      outb(0x3D6, 0xA3);
      outb(0x3D7, (ctCursorAddress >> 16) & 0x3F);
    } else if (!ctisWINGINE) {
      if (!ctUseMMIO) {
	HW_DEBUG(0xC);
	outl(DR(0xC), ctCursorAddress);
      } else {
	HW_DEBUG(MR(0xC));
	MMIOmeml(MR(0xC)) = ctCursorAddress;
      }
    }
}


Bool CHIPSUseHWCursor(pScr)
     ScreenPtr pScr;
{
    if (XF86SCRNINFO(pScr)->modes->Flags & V_DBLSCAN)
        ctDBLSCAN = TRUE;
    else
        ctDBLSCAN = FALSE;

    if (ctHWCursorAlways) return TRUE;

    return ctHWCursor;
}

void CHIPSInitCursor() 
{
    XAACursorInfoRec.Flags = USE_HARDWARE_CURSOR |
      HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
      HARDWARE_CURSOR_PROGRAMMED_BITS |
      HARDWARE_CURSOR_PROGRAMMED_ORIGIN |
      HARDWARE_CURSOR_INVERT_MASK |
      HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
      HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
      HARDWARE_CURSOR_SYNC_NEEDED;

    if (ctisHiQV32) {
      XAACursorInfoRec.Flags |= HARDWARE_CURSOR_INT64_BIT_FORMAT;
      XAACursorInfoRec.MaxHeight = 64;
      XAACursorInfoRec.MaxWidth = 64;
    } else if (ctisWINGINE) {
      XAACursorInfoRec.Flags |= HARDWARE_CURSOR_LONG_BIT_FORMAT;
      XAACursorInfoRec.MaxHeight = 32;
      XAACursorInfoRec.MaxWidth = 32;      
    } else {
      XAACursorInfoRec.Flags |= HARDWARE_CURSOR_CHAR_BIT_FORMAT;
      XAACursorInfoRec.MaxHeight = 32;
      XAACursorInfoRec.MaxWidth = 32;
    }
    
    vgaHWCursor.Init = XAACursorInit;
    vgaHWCursor.Initialized = TRUE;
    vgaHWCursor.Restore = XAARestoreCursor;
    vgaHWCursor.Warp = XAAWarpCursor;
    vgaHWCursor.QueryBestSize = XAAQueryBestSize;
    XAACursorInfoRec.ShowCursor = CHIPSShowCursor;
    XAACursorInfoRec.HideCursor = CHIPSHideCursor;
    XAACursorInfoRec.SetCursorColors = CHIPSSetCursorColors;
    XAACursorInfoRec.SetCursorPosition = CHIPSSetCursorPosition;
    XAACursorInfoRec.LoadCursorImage = CHIPSLoadCursorImage;
    XAACursorInfoRec.UseHWCursor = CHIPSUseHWCursor;
    XAACursorInfoRec.GetInstalledColormaps = vgaGetInstalledColormaps;

    /* 1kB alignment. Note this allocation is now done in FbInit. */
    /* load address to card when cursor is loaded to card */
    if (ctisHiQV32) {
	outb(0x3D6, 0xA2);
	outb(0x3D7, (ctCursorAddress >> 8) & 0xFF);
	outb(0x3D6, 0xA3);
	outb(0x3D7, (ctCursorAddress >> 16) & 0x3F);
    } else if (!ctisWINGINE) {
      if(!ctUseMMIO) {
	HW_DEBUG(0xC);
	outl(DR(0xC), ctCursorAddress);
      } else {
	HW_DEBUG(MR(0xC));
	MMIOmeml(MR(0xC)) = ctCursorAddress;
      }
    }
}

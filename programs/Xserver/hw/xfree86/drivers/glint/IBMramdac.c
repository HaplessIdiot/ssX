/*
 * Copyright 1998 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * glintOutIBMRGBIndReg() and glintInIBMRGBIndReg() are used to access 
 * the indirect IBM RAMDAC registers only.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/IBMramdac.c,v 1.2 1998/07/25 16:55:45 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "IBM.h"
#include "glint_regs.h"
#include "glint.h"

#define IBMRGB_WRITE_ADDR           0x4000  
#define IBMRGB_RAMDAC_DATA          0x4008 
#define IBMRGB_PIXEL_MASK           0x4010 
#define IBMRGB_READ_ADDR            0x4018  
#define IBMRGB_INDEX_LOW            0x4020 
#define IBMRGB_INDEX_HIGH           0x4028 
#define IBMRGB_INDEX_DATA           0x4030  
#define IBMRGB_INDEX_CONTROL        0x4038 

void
glintOutIBMRGBIndReg(ScrnInfoPtr pScrn,
		     CARD32 reg, unsigned char mask, unsigned char data)
{
  GLINTPtr pGlint = GLINTPTR(pScrn);
  unsigned char tmp = 0x00;

  GLINT_SLOW_WRITE_REG (reg & 0xFF, IBMRGB_INDEX_LOW);
  GLINT_SLOW_WRITE_REG((reg>>8) & 0xff, IBMRGB_INDEX_HIGH);

  if (mask != 0x00)
    tmp = GLINT_READ_REG (IBMRGB_INDEX_DATA) & mask;

  GLINT_SLOW_WRITE_REG (tmp | data, IBMRGB_INDEX_DATA);
}

unsigned char
glintInIBMRGBIndReg (ScrnInfoPtr pScrn, CARD32 reg)
{
  GLINTPtr pGlint = GLINTPTR(pScrn);
  unsigned char ret;

  GLINT_SLOW_WRITE_REG(reg & 0xFF, IBMRGB_INDEX_LOW);
  GLINT_SLOW_WRITE_REG((reg>>8) & 0xff, IBMRGB_INDEX_HIGH);
  ret = GLINT_READ_REG(IBMRGB_INDEX_DATA);
  return (ret);
}

void
glintIBMWriteAddress (ScrnInfoPtr pScrn, CARD32 index)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    
    GLINT_SLOW_WRITE_REG(index, IBMRGB_WRITE_ADDR);
}

void
glintIBMWriteData (ScrnInfoPtr pScrn, unsigned char data)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    
    GLINT_SLOW_WRITE_REG(data, IBMRGB_RAMDAC_DATA);
}

void
glintIBMReadAddress (ScrnInfoPtr pScrn, CARD32 index)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    
    GLINT_SLOW_WRITE_REG(0xFF, IBMRGB_PIXEL_MASK);
    GLINT_SLOW_WRITE_REG(index, IBMRGB_READ_ADDR);
}

unsigned char
glintIBMReadData (ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    
    return(GLINT_READ_REG(IBMRGB_RAMDAC_DATA));
}

void 
glintIBM526ShowCursor(ScrnInfoPtr pScrn)
{
   /* Enable cursor - X11 mode */
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs, 0x00, 0x27);
}

void 
glintIBM640ShowCursor(ScrnInfoPtr pScrn)
{
   /* Enable cursor - mode2 (x11 mode) */
   glintOutIBMRGBIndReg(pScrn, RGB640_CURSOR_CONTROL, 0x00, 0x0B);
}

void
glintIBM526HideCursor(ScrnInfoPtr pScrn)
{
   /* Disable cursor - X11 mode */
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs, 0x00, 0x24);
}

void
glintIBM640HideCursor(ScrnInfoPtr pScrn)
{
   /* Disable cursor - mode2 (x11 mode) */
   glintOutIBMRGBIndReg(pScrn, RGB640_CURSOR_CONTROL, 0x00, 0x08);
}

void
glintIBM526SetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
   x += 64;
   y += 64;
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_hot_x, 0x00, 0x3f);
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_hot_y, 0x00, 0x3f);
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_xl, 0x00, x & 0xff);
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_xh, 0x00, (x>>8) & 0xf);
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_yl, 0x00, y & 0xff);
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_yh, 0x00, (y>>8) & 0xf);
}

void
glintIBM640SetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
   x += 64;
   y += 64;
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_OFFSETX, 0x00, 0x3f);
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_OFFSETY, 0x00, 0x3f);
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_X_LOW, 0x00, x & 0xff);
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_X_HIGH, 0x00, (x>>8) & 0xf);
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_Y_LOW, 0x00, y & 0xff);
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_Y_HIGH, 0x00, (y>>8) & 0xf);
}

void
glintIBM526SetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_col1_r, 0x00, bg >> 16);
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_col1_g, 0x00, bg >> 8);
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_col1_b, 0x00, bg);
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_col2_r, 0x00, fg >> 16);
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_col2_g, 0x00, fg >> 8);
   glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_col2_b, 0x00, fg);
}

void
glintIBM640SetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_COL2, 0x00, bg>>16);
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_COL2, 0x00, bg>>8);
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_COL2, 0x00, bg);
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_COL3, 0x00, fg>>16);
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_COL3, 0x00, fg>>8);
   glintOutIBMRGBIndReg(pScrn, RGB640_CURS_COL3, 0x00, fg);
}

void 
glintIBM526LoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
    int i;
   /* 
    * Output the cursor data.  The realize function has put the planes into
    * their correct order, so we can just blast this out.
    */
   for (i = 0; i < 1024; i++)
      glintOutIBMRGBIndReg(pScrn, IBMRGB_curs_array + i, 0x00, (*src++));
}

void 
glintIBM640LoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
    int i;
   /* 
    * Output the cursor data.  The realize function has put the planes into
    * their correct order, so we can just blast this out.
    */
   for (i = 0; i < 1024; i++)
      glintOutIBMRGBIndReg(pScrn, RGB640_CURS_WRITE + i, 0x00, (*src++));
}

static Bool 
glintIBM526UseHWCursor(ScreenPtr pScr, CursorPtr pCurs)
{
    return TRUE;
}

static Bool 
glintIBM640UseHWCursor(ScreenPtr pScr, CursorPtr pCurs)
{
    return TRUE;
}

Bool 
glintIBM526HWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    XAACursorInfoPtr infoPtr;

    infoPtr = XAACreateCursorInfoRec();
    if(!infoPtr) return FALSE;
    
    pGlint->CursorInfoRec = infoPtr;

    infoPtr->MaxWidth = 64;
    infoPtr->MaxHeight = 64;
    infoPtr->Flags = HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
		     HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
		     HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1;
    infoPtr->SetCursorColors = glintIBM526SetCursorColors;
    infoPtr->SetCursorPosition = glintIBM526SetCursorPosition;
    infoPtr->LoadCursorImage = glintIBM526LoadCursorImage;
    infoPtr->HideCursor = glintIBM526HideCursor;
    infoPtr->ShowCursor = glintIBM526ShowCursor;
    infoPtr->UseHWCursor = glintIBM526UseHWCursor;

    return(XAAInitCursor(pScreen, infoPtr));
}

glintIBM640HWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    XAACursorInfoPtr infoPtr;

    infoPtr = XAACreateCursorInfoRec();
    if(!infoPtr) return FALSE;
    
    pGlint->CursorInfoRec = infoPtr;

    infoPtr->MaxWidth = 64;
    infoPtr->MaxHeight = 64;
    infoPtr->Flags = HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
		     HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
		     HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1;
    infoPtr->SetCursorColors = glintIBM640SetCursorColors;
    infoPtr->SetCursorPosition = glintIBM640SetCursorPosition;
    infoPtr->LoadCursorImage = glintIBM640LoadCursorImage;
    infoPtr->HideCursor = glintIBM640HideCursor;
    infoPtr->ShowCursor = glintIBM640ShowCursor;
    infoPtr->UseHWCursor = glintIBM640UseHWCursor;

    return(XAAInitCursor(pScreen, infoPtr));
}

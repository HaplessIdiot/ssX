/* $XFree86: $ */
/*
 * Copyright 1996 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Robin Cutshaw not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Robin Cutshaw makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ROBIN CUTSHAW DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ROBIN CUTSHAW BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Modified by Alan Hourihane <alanh@fairlite.demon.co.uk> 
 * for the 3Dlabs GLINT chipset
 */

#include "servermd.h"

#include "glint_regs.h"
#include "glint.h"
#define GLINT_SERVER
#include "IBMRGB.h"

#define MAX_CURS_HEIGHT 64   /* 64 scan lines */
#define MAX_CURS_WIDTH  64   /* 64 pixels     */

extern struct glintmem glintmem;
extern Bool glintBlockCursor;

/*
 * Convert the cursor from server-format to hardware-format.  The IBMRGB
 * has two planes, plane 0 selects cursor color 0 or 1 and plane 1
 * selects transparent or display cursor.  The bits of these planes
 * are packed together so that one byte has 4 pixels. The organization
 * looks like:
 *             Byte 0x000 - 0x00F    top scan line, left to right
 *                  0x010 - 0x01F
 *                    .       .
 *                  0x3F0 - 0x3FF    bottom scan line
 *
 *             Byte/bit map - D7D6,D5D4,D3D2,D1D0  four pixels, two planes each
 *             Pixel/bit map - P1P0  (plane 1) == 1 maps to cursor color
 *                                   (plane 1) == 0 maps to transparent
 *                                   (plane 0) maps to cursor colors 0 and 1
 */

Bool
glintIBMRealizeCursor(ScreenPtr pScr, CursorPtr pCurs)
{
   register int i, j;
   unsigned char *pServMsk;
   unsigned char *pServSrc;
   pointer *pPriv;
   int   wsrc, h;
   unsigned char *ram;

   if (pCurs->bits->refcnt > 1)
      return TRUE;

   ram = (unsigned char *)xalloc(1024);  /* 64x64x2 bits */
   pPriv = &pCurs->bits->devPriv[pScr->myNum];
   *pPriv = (pointer) ram;

   if (!ram)
      return FALSE;

   pServSrc = (unsigned char *)pCurs->bits->source;
   pServMsk = (unsigned char *)pCurs->bits->mask;

   h = pCurs->bits->height;
   if (h > MAX_CURS_HEIGHT)
      h = MAX_CURS_HEIGHT;

   wsrc = PixmapBytePad(pCurs->bits->width, 1);	/* bytes per line */

   for (i = 0; i < MAX_CURS_HEIGHT; i++,ram+=16) {
      for (j = 0; j < MAX_CURS_WIDTH / 8; j++) {
	 register unsigned char mask, source;

	 if (i < h && j < wsrc) {
	    /*
	     * mask byte ABCDEFGH and source byte 12345678 map to two byte
	     * cursor data H8G7F6E5 D4C3B2A1
	     */
	    mask = *pServMsk++;
	    source = *pServSrc++ & mask;

	    /* map 1 byte source and mask into two byte cursor data */
	    ram[j*2] =     ((mask&0x01) << 7) | ((source&0x01) << 6) |
		           ((mask&0x02) << 4) | ((source&0x02) << 3) |
		           ((mask&0x04) << 1) | (source&0x04)        |
		           ((mask&0x08) >> 2) | ((source&0x08) >> 3) ;
	    ram[(j*2)+1] = ((mask&0x10) << 3) | ((source&0x10) << 2) |
		           (mask&0x20)        | ((source&0x20) >> 1) |
		           ((mask&0x40) >> 3) | ((source&0x40) >> 4) |
		           ((mask&0x80) >> 6) | ((source&0x80) >> 7) ;
	 } else {
	    ram[j*2]     = 0x00;
	    ram[(j*2)+1] = 0x00;
	 }
      }
      /*
       * if we still have more bytes on this line (j < wsrc),
       * we have to ignore the rest of the line.
       */
       while (j++ < wsrc) pServMsk++,pServSrc++;
   }
   return TRUE;
}

void 
glintIBMCursorOn()
{
   unsigned char tmp;

   /* Enable cursor - X11 mode */
   tmp = GLINT_READ_REG(IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(0x27, IBMRGB_INDEX_DATA);

   GLINT_SLOW_WRITE_REG(tmp, IBMRGB_INDEX_LOW);

   return;
}

void
glintIBMCursorOff()
{
   unsigned char tmp, tmp1;

   tmp = GLINT_READ_REG(IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs, IBMRGB_INDEX_LOW);
   tmp1 = GLINT_READ_REG(IBMRGB_INDEX_DATA) & ~3;
   GLINT_SLOW_WRITE_REG(tmp1, IBMRGB_INDEX_DATA);

   GLINT_SLOW_WRITE_REG(tmp, IBMRGB_INDEX_LOW);

   return;
}

void
glintIBMMoveCursor(ScreenPtr pScr, int x, int y)
{
   unsigned char tmp;
   extern int glintAdjustCursorXPos, glinthotX, glinthotY;

   if (glintBlockCursor)
      return;
   
   x -= glintInfoRec.frameX0 - glintAdjustCursorXPos;
   if (x < 0)
      return;

   y -= glintInfoRec.frameY0;
   if (y < 0)
      return;

   tmp = GLINT_READ_REG(IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_hot_x, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(glinthotX & 0xFF, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_hot_y, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(glinthotY & 0xFF, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_xl, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(x & 0xFF, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_xh, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG((x >> 8) & 0x0F, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_yl, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(y & 0xFF, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_yh, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG((y >> 8) & 0x0F, IBMRGB_INDEX_DATA);

   GLINT_SLOW_WRITE_REG(tmp, IBMRGB_INDEX_LOW);
   return;
}

void
glintIBMRecolorCursor(ScreenPtr pScr, CursorPtr pCurs, Bool displayed)
{
   unsigned char tmp;

   if (!xf86VTSema) {
      miRecolorCursor(pScr, pCurs, displayed);
      return;
   }

   tmp = GLINT_READ_REG(IBMRGB_INDEX_LOW);

   /* Background color */
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_col1_r, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG((pCurs->backRed >> 8) & 0xFF, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_col1_g, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG((pCurs->backGreen >> 8) & 0xFF, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_col1_b, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG((pCurs->backBlue >> 8) & 0xFF, IBMRGB_INDEX_DATA);

   /* Foreground color */
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_col2_r, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG((pCurs->foreRed >> 8) & 0xFF, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_col2_g, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG((pCurs->foreGreen >> 8) & 0xFF, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_col2_b, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG((pCurs->foreBlue >> 8) & 0xFF, IBMRGB_INDEX_DATA);

   GLINT_SLOW_WRITE_REG(tmp, IBMRGB_INDEX_LOW);

   return;
}

void 
glintIBMLoadCursor(ScreenPtr pScr, CursorPtr pCurs, int x, int y)
{
   extern int glinthotX, glinthotY;
   int   index = pScr->myNum;
   register int   i;
   unsigned char *ram, *p, tmp, tmp1, tmpcurs;
   extern int glintInitCursorFlag;

   if (!xf86VTSema)
      return;

   if (!pCurs)
      return;

   tmp = GLINT_READ_REG(IBMRGB_INDEX_LOW);

   /* turn the cursor off */
   GLINT_SLOW_WRITE_REG(IBMRGB_curs, IBMRGB_INDEX_LOW);
   if ((tmpcurs = GLINT_READ_REG(IBMRGB_INDEX_DATA) & 0x03))
      glintIBMCursorOff();

   /* load colormap */
   glintIBMRecolorCursor(pScr, pCurs, TRUE);

   ram = (unsigned char *)pCurs->bits->devPriv[index];

   glintBlockCursor = TRUE;

   GLINT_SLOW_WRITE_REG(IBMRGB_curs_hot_x, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(0x00, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_hot_y, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(0x00, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_xl, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(0xFF, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_xh, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(0x7F, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_yl, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(0xFF, IBMRGB_INDEX_DATA);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_yh, IBMRGB_INDEX_LOW);
   GLINT_SLOW_WRITE_REG(0x7F, IBMRGB_INDEX_DATA);

   tmp1 = GLINT_READ_REG(IBMRGB_INDEX_CONTROL) & 0xFE;
   GLINT_SLOW_WRITE_REG(tmp1 | 1, IBMRGB_INDEX_CONTROL); /* enable auto-inc */

   GLINT_SLOW_WRITE_REG((IBMRGB_curs_array >> 8) & 0xFF, IBMRGB_INDEX_HIGH);
   GLINT_SLOW_WRITE_REG(IBMRGB_curs_array & 0xFF, IBMRGB_INDEX_LOW);

   /* 
    * Output the cursor data.  The realize function has put the planes into
    * their correct order, so we can just blast this out.
    */
   p = ram;
   for (i = 0; i < 1024; i++,p++) {
      GLINT_SLOW_WRITE_REG(*p, IBMRGB_INDEX_DATA);
   }

   if (glinthotX >= MAX_CURS_WIDTH)
      glinthotX = MAX_CURS_WIDTH - 1;
   else if (glinthotX < 0)
      glinthotX = 0;
   if (glinthotY >= MAX_CURS_HEIGHT)
      glinthotY = MAX_CURS_HEIGHT - 1;
   else if (glinthotY < 0)
      glinthotY = 0;

   GLINT_SLOW_WRITE_REG(0, IBMRGB_INDEX_HIGH);
   GLINT_SLOW_WRITE_REG(tmp1, IBMRGB_INDEX_CONTROL);
   GLINT_SLOW_WRITE_REG(tmp, IBMRGB_INDEX_LOW);

   glintBlockCursor = FALSE;

   /* position cursor */
   glintIBMMoveCursor(0, x, y);

   /* turn the cursor on */
   if ((tmpcurs & 0x03) || glintInitCursorFlag)
      glintIBMCursorOn();

   if (glintInitCursorFlag)
      glintInitCursorFlag = FALSE;

   return;
}

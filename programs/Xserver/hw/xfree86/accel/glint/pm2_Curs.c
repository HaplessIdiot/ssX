/* $XFree86: $ */
/*
 * Copyright 1996 by Alan Hourihane <alanh@fairlite.demon.co.uk>
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
 * Written by Alan Hourihane <alanh@fairlite.demon.co.uk>
 * for the 3Dlabs Permedia2 chipset
 */
#include "glint_regs.h"
#include "glint.h"
#define GLINT_SERVER

void 
PM2DACShowCursor()
{
   /* Enable cursor - X11 mode */
   GLINT_WRITE_REG(PM2DACCursorControl, PM2DACIndexReg);
   GLINT_WRITE_REG(0x4B, PM2DACIndexData);
}

void
PM2DACHideCursor()
{
   GLINT_WRITE_REG(PM2DACCursorControl, PM2DACIndexReg);
   GLINT_WRITE_REG(0x48, PM2DACIndexData);
}

void
PM2DACSetCursorPosition(x, y, xorigin, yorigin)
{
   GLINT_WRITE_REG((x >> 8) & 0x0F, PM2DACCursorXMsb);
   GLINT_WRITE_REG(x & 0xFF, PM2DACCursorXLsb);
   GLINT_WRITE_REG((y >> 8) & 0x0F, PM2DACCursorYMsb);
   GLINT_WRITE_REG(y & 0xFF, PM2DACCursorYLsb);
}

void
PM2DACSetCursorColors(bg, fg)
	int bg, fg;
{
   GLINT_SLOW_WRITE_REG(0x01, PM2DACCursorColorAddress);
   GLINT_SLOW_WRITE_REG((bg & 0xFF0000)>>16, PM2DACCursorColorData);
   GLINT_SLOW_WRITE_REG((bg & 0xFF00)>>8, PM2DACCursorColorData);
   GLINT_SLOW_WRITE_REG((bg & 0xFF), PM2DACCursorColorData);
   GLINT_SLOW_WRITE_REG((fg & 0xFF0000)>>16, PM2DACCursorColorData);
   GLINT_SLOW_WRITE_REG((fg & 0xFF00)>>8, PM2DACCursorColorData);
   GLINT_SLOW_WRITE_REG((fg & 0xFF), PM2DACCursorColorData);
}

void 
PM2DACLoadCursorImage(bits, xorigin, yorigin)
	unsigned char *bits;
	int xorigin, yorigin;
{
   unsigned char tmp, tmp1, tmpcurs;
   int i;

   GLINT_WRITE_REG(0, PM2DACWriteAddress);
   /* 
    * Output the cursor data.  The realize function has put the planes into
    * their correct order, so we can just blast this out.
    */
   for (i = 0; i < 1024; i++) {
      GLINT_WRITE_REG(*bits++, PM2DACCursorData);
   }
}

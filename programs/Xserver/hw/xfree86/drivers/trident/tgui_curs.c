/* $XConsortium: tgui_curs.c /main/10 1996/10/25 15:39:18 kaleb $ */
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
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/tgui_curs.c,v 1.5 1997/10/13 17:16:44 hohndel Exp $ */



#include "vga.h"
#include "xf86_OSlib.h"

#include "trident_driver.h"
#include "tgui_ger.h"
#include "xf86cursor.h"

extern int tridentHWCursorType;
extern int TridentCursorAddress;

void TridentShowCursor() {
	if (tridentHWCursorType == 2)
	{
		outb(GER_INDEX, 0x34);
		outb(GER_BYTE2, 0x01);	/* Enable Cursor in GER */
		outb(GER_INDEX, 0x78);
		/* Max 64x64 cursor */
		outb(GER_BYTE0, XAACursorInfoRec.MaxWidth - 1); 
		outb(GER_BYTE2, XAACursorInfoRec.MaxHeight - 1);
	} else {
		if (IsCyber) 
			/* 128x128 */
			wrinx(vgaIOBase + 4, 0x50, 0xC2);
		else
			/* 64x64 */
			wrinx(vgaIOBase + 4, 0x50, 0xC1);
	}

	if (tridentHWCursorType == 2)
	{
		outb(GER_INDEX, 0x74);
		outb(GER_BYTE0, TridentCursorAddress & 0x000000FF);
		outb(GER_BYTE1, (TridentCursorAddress & 0x0000FF00) >> 8);
		outb(GER_BYTE2, (TridentCursorAddress & 0x00FF0000) >> 16);
		outb(GER_BYTE3, (TridentCursorAddress & 0xFF000000) >> 24);
	}
	else
	{
		outw(vgaIOBase + 4,
			(((TridentCursorAddress/1024) & 0xFF) << 8) | 0x44);
		outw(vgaIOBase + 4,
			((TridentCursorAddress/1024) & 0xFF00) | 0x45);
	}
}

void TridentHideCursor() {
	if (tridentHWCursorType == 2)
	{
		outb(GER_INDEX, 0x34);
		outb(GER_BYTE2, 0x00);	/* Disable Cursor in GER */
	} else {
		if (IsCyber)
			wrinx(vgaIOBase + 4, 0x50, 0x42);
		else
			wrinx(vgaIOBase + 4, 0x50, 0x41);
	}
}

void TridentSetCursorPosition(x, y, xorigin, yorigin)
	int x, y, xorigin, yorigin;
{
	/* Program the cursor origin (offset into the cursor bitmap). */
	if (tridentHWCursorType == 1)
	{
		wrinx(vgaIOBase + 4, 0x46, xorigin);
		wrinx(vgaIOBase + 4, 0x47, yorigin);
	}

	/* Program the new cursor position. */
	if (tridentHWCursorType == 2)
	{
		outb(GER_INDEX, 0x30);
		outb(GER_BYTE0, x);
		outb(GER_BYTE1, x >> 8);
		outb(GER_BYTE3, y);
		outb(GER_INDEX, 0x34);
		outb(GER_BYTE0, y >> 8);
	}
	else
	{
		wrinx(vgaIOBase + 4, 0x40, x);		/* Low byte. */
		wrinx(vgaIOBase + 4, 0x41, x >> 8);	/* High byte. */
		wrinx(vgaIOBase + 4, 0x42, y);		/* Low byte. */
		wrinx(vgaIOBase + 4, 0x43, y >> 8);	/* High byte. */
	}
}

void
TridentSetCursorColors(bg, fg)
	int fg, bg;
{
#if 0
   outb(0x3c8, 0x00);		/* DAC color 0 */
   outb(0x3c9, fred>>shift);
   outb(0x3c9, fgreen>>shift);
   outb(0x3c9, fblue>>shift);

   outb (0x3c8, 0xFF);		/* DAC color 255 */
   outb (0x3c9, bred>>shift);
   outb (0x3c9, bgreen>>shift);
   outb (0x3c9, bblue>>shift);
#endif

   if (tridentHWCursorType == 2)
   {
	outb(GER_INDEX, 0x38);
	outb(GER_BYTE0, 0x00);
	outb(GER_BYTE1, 0xFF);
   }
   else
   if (TVGAchipset >= TGUI96xx)
   {
	/* We've got specific colours now for the cursor */

	wrinx(vgaIOBase + 4, 0x48, fg & 0x000000FF);
	wrinx(vgaIOBase + 4, 0x49, (fg & 0x0000FF00) >> 8);
	wrinx(vgaIOBase + 4, 0x4A, (fg & 0x00FF0000) >> 16);
	wrinx(vgaIOBase + 4, 0x4B, (fg & 0xFF000000) >> 24);
	wrinx(vgaIOBase + 4, 0x4C, bg & 0x000000FF);
	wrinx(vgaIOBase + 4, 0x4D, (bg & 0x0000FF00) >> 8);
	wrinx(vgaIOBase + 4, 0x4E, (bg & 0x00FF0000) >> 16);
	wrinx(vgaIOBase + 4, 0x4F, (bg & 0xFF000000) >> 24);
   }
}

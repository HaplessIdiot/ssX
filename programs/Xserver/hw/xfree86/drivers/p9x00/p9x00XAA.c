/*
 * Copyright 1996-1997  Joerg Knura (knura@imst.de)
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
 * JOERG KNURA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/p9x00/p9x00XAA.c,v 1.1 1998/01/11 03:36:48 dawes Exp $ */

#include "vga256.h"
#include "xf86.h"
#include "vga.h"

#include "../../xaa/xf86xaa.h"

#if 0
#include "p9x00Regs.h"

extern int p9x00type;
extern type_p9x00_registers p9x00regs;
extern ScrnInfoRec P9X00;

/*extern int *p9x00raster;
 */
#define P9X00SETFORECOLOR(color) p9x00regs.drawing_ctrl->color0=\
   (vga256InfoRec.bitsPerPixel==32)? (CARD32)color:\
   (vga256InfoRec.bitsPerPixel==24)? (CARD32)(((color&0xFFL)<<24)&color):\
   (vga256InfoRec.bitsPerPixel>8)?   (CARD32)((color<<16)|color):\
                  (CARD32)color*0x1010101L;
#define P9X00SETBACKCOLOR(color) p9x00regs.drawing_ctrl->color1=\
   (vga256InfoRec.bitsPerPixel==32)? (CARD32)color:\
   (vga256InfoRec.bitsPerPixel==24)? (CARD32)(((color&0xFFL)<<24)&color):\
   (vga256InfoRec.bitsPerPixel>8)?   (CARD32)((color<<16)|color):\
                  (CARD32)color*0x1010101L;

/* #define P9X00SETRASTEROP(rop) p9x00regs.drawing_ctrl->raster=\
   p9x00raster[rop];
*/
#define P9X00SETPLANEMASK(pm) p9x00regs.drawing_ctrl->plane_mask=(pm)
     
void p9x00Sync();
void p9x00SetupForFillRectSolid();
void p9x00SubsequentFillRectSolid();
void p9x00SetupForScreenToScreenCopy();
void p9x00SubsequentScreenToScreenCopy();
/*void p9x00SubsequentTwoPointLine();
void p9x00SetClippingRectangle();
*/

/*
 * The following function sets up the supported acceleration. Call it
 * from the FbInit() function in the SVGA driver, or before ScreenInit
 * in a monolithic server.
 */
void p9x00AccelInit() {
    xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS | PIXMAP_CACHE;
    xf86AccelInfoRec.Sync = p9x00Sync;

    /*
     * We want to set up the FillRectSolid primitive for filling a solid
     * rectangle. First we set up the flags for the graphics operation.
     * It may include GXCOPY_ONLY, NO_PLANEMASK, and RGB_EQUAL.
     */
    xf86GCInfoRec.PolyFillRectSolidFlags = 0;

    xf86AccelInfoRec.SetupForFillRectSolid = p9x00SetupForFillRectSolid;
    xf86AccelInfoRec.SubsequentFillRectSolid = p9x00SubsequentFillRectSolid;

    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY;
    
    xf86AccelInfoRec.SetupForScreenToScreenCopy =
        p9x00SetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
        p9x00SubsequentScreenToScreenCopy;

    /*
     * Set a pointer to the server InfoRec so that XAA knows how to
     * format its start-up messages. If you are not using the SVGA
     * server, you would provide a pointer to the ScrnInfoRec defined
     * for the server. The SVGA server correctly initializes this
     * field itself, so this line can be omitted when using the SVGA
     * server.
     */
    xf86AccelInfoRec.ServerInfoRec = &P9X00;

    /*
     * Finally, we set up the video memory space available to the pixmap
     * cache. In this case, all memory from the end of the virtual screen
     * to the end of video memory minus 1K, can be used. If you haven't
     * enabled the PIXMAP_CACHE flag, then these lines can be omitted.
     */
     xf86AccelInfoRec.PixmapCacheMemoryStart =
         vga256InfoRec.virtualY * vga256InfoRec.virtualX
         * (vga256InfoRec.bitsPerPixel+1) / 8;
     xf86AccelInfoRec.PixmapCacheMemoryEnd =
        vga256InfoRec.videoRam * 1024 - 1024;
     ErrorF("PIXMAP: 0x%08lX 0x%08lX\n",
       xf86AccelInfoRec.PixmapCacheMemoryStart,
       xf86AccelInfoRec.PixmapCacheMemoryEnd);
}

/*
 * This is the implementation of the Sync() function.
 */
void p9x00Sync() {
   while (p9x00regs.command->status&(QUAD_BUSY|DRAW_BUSY)){};
}

/*
 * This is the implementation of the SetupForFillRectSolid function
 * that sets up the coprocessor for a subsequent batch of solid
 * rectangle fills.
 */
void p9x00SetupForFillRectSolid(int color, int rop, unsigned planemask)
{
   p9x00Sync();
   P9X00SETFORECOLOR(color);
/*   P9X00SETRASTEROP(rop);
*/   P9X00SETPLANEMASK(planemask);
}

/*
 * This is the implementation of the SubsequentForFillRectSolid function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 */
void p9x00SubsequentFillRectSolid(int x, int y, int w, int h)
{
  CARD32 temp;
  
  p9x00Sync();
  p9x00regs.meta_coord->type[RECTANGLE].relative_to_win.xy_coord=
    (CARD32)(0xFFFFFFFFL&(CARD32)(x<<16)|((CARD32)y&0xFFFFL));
  p9x00regs.meta_coord->type[RECTANGLE].relative_to_prev.xy_coord=
    (CARD32)(0xFFFFFFFFL&(CARD32)(w<<16)|((CARD32)h&0xFFFFL));
  temp=p9x00regs.command->quad;
}

/*
 * This is the implementation of the SetupForScreenToScreenCopy function
 * that sets up the coprocessor for a subsequent batch of
 * screen-to-screen copies. Remember, we don't handle transparency,
 * so the transparency color is ignored.
 */
void p9x00SetupForScreenToScreenCopy(int xdir, int ydir, int rop, 
                                    unsigned planemask, int transparency_color)
{
  p9x00Sync();
/*  P9X00SETRASTEROP(rop);
*/  P9X00SETPLANEMASK(planemask);
}

/*
 * This is the implementation of the SubsequentForScreenToScreenCopy
 * that sends commands to the coprocessor to perform a screen-to-screen
 * copy of the specified areas, with the parameters from the SetUp call.
 * In this sample implementation, the direction must be taken into
 * account when calculating the addresses (with coordinates, it might be
 * a little easier).
 */
void p9x00SubsequentScreenToScreenCopy(int x1, int y1, int x2, int y2, 
                                       int w, int h)
{
  CARD32 bpp=vga256InfoRec.bitsPerPixel;
  CARD32 temp;
  
  p9x00Sync();
  p9x00regs.device_coord->edge[0].absolute.xy_coord=
    (CARD32)(0xFFFFFFFFL&(CARD32)((x1*bpp-1)<<16)|((CARD32)y1&0xFFFFL));
  p9x00regs.device_coord->edge[1].absolute.xy_coord=
    (CARD32)(0xFFFFFFFFL&(CARD32)(((x1+w)*bpp-1)<<16)|((CARD32)(y1+h-1)&0xFFFFL));
  p9x00regs.device_coord->edge[2].absolute.xy_coord=
    (CARD32)(0xFFFFFFFFL&(CARD32)((x2*bpp-1)<<16)|((CARD32)y2&0xFFFFL));
  p9x00regs.device_coord->edge[3].absolute.xy_coord=
    (CARD32)(0xFFFFFFFFL&(CARD32)(((x2+w)*bpp-1)<<16)|((CARD32)(y2+h-1)&0xFFFFL));
  temp=p9x00regs.command->blit;
}
#endif

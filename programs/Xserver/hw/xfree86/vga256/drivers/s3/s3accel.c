/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/s3/s3accel.c,v 3.3 1997/01/14 22:21:59 dawes Exp $ */

/*
 *
 * Copyright 1996-1997 The XFree86 Project, Inc.
 *
 */

/*
 * This is a sample driver implementation template for the new acceleration
 * interface.
 */

#define PSZ 8

#include "vga256.h"
#include "xf86.h"
#include "vga.h"

#include "../../../xaa/xf86xaa.h"

#include "s3.h"
#include "s3reg.h"

void S3Sync();
void S3SetupForScreenToScreenCopy();
void S3SubsequentScreenToScreenCopy();
void S3SetupForFillRectSolid();
void S3SubsequentFillRectSolid();
#if 0
void S3SubsequentBresenhamLine();
#endif
void S3SetClippingRectangle();
void S3PolySegment();
void S3PolyLine();
void S3SetupForFill8x8Pattern();
void S3SubsequentFill8x8Pattern();
void S3SubsequentTwoPointLine();

void S3AccelInit() {

    /* PIXMAP_CACHE is broken */
    xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS | /* PIXMAP_CACHE |	*/
				TWO_POINT_LINE_NOT_LAST | HARDWARE_CLIP_LINE;


    xf86AccelInfoRec.Sync = S3Sync;

    /* copy area */
    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY;
    xf86AccelInfoRec.SetupForScreenToScreenCopy =
        S3SetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
        S3SubsequentScreenToScreenCopy;

    /* filled rects */
    xf86GCInfoRec.PolyFillRectSolidFlags = 0;
    xf86AccelInfoRec.SetupForFillRectSolid = S3SetupForFillRectSolid;
    xf86AccelInfoRec.SubsequentFillRectSolid = S3SubsequentFillRectSolid;


    /* lines */
    xf86AccelInfoRec.SubsequentTwoPointLine = S3SubsequentTwoPointLine; 
    xf86AccelInfoRec.SetClippingRectangle = S3SetClippingRectangle;
    xf86GCInfoRec.PolyLineSolidZeroWidth = S3PolyLine;
    xf86GCInfoRec.PolySegmentSolidZeroWidth = S3PolySegment;


    /* 8x8 pattern fills */
    if(S3_801_928_SERIES(s3ChipId)) {
	/* vgadoc4b says this is only for 80x and newer (MArk) */
       xf86AccelInfoRec.SetupForFill8x8Pattern = S3SetupForFill8x8Pattern;
       xf86AccelInfoRec.SubsequentFill8x8Pattern = S3SubsequentFill8x8Pattern;
    }
   

    /* pixmap cache */    
    xf86AccelInfoRec.PixmapCacheMemoryStart =
	(s3CursorStartY + s3CursorLines) * s3BppDisplayWidth;
    xf86AccelInfoRec.PixmapCacheMemoryEnd =
        vga256InfoRec.videoRam * 1024;


}

		/******************\
		|	Sync	   |
		\******************/

void S3Sync() {
    UNBLOCK_CURSOR;
    WaitIdle();
}


	/***************************************\
	|	Screen-to-Screen Copy		|
	\***************************************/


static unsigned short BltDirection;

void S3SetupForScreenToScreenCopy(xdir, ydir, rop, planemask,
transparency_color)
    int xdir, ydir;
    int rop;
    unsigned planemask;
    int transparency_color;
{
    BltDirection = CMD_BITBLT | DRAW | PLANAR | WRTDATA;

    if(xdir == 1) BltDirection |= INC_X;
    if(ydir == 1) BltDirection |= INC_Y;
   
    BLOCK_CURSOR;
    WaitQueue16_32(3,4);
    SET_MIX(FSS_BITBLT | s3alu[rop],BSS_BKGDCOL | MIX_SRC);
    SET_WRT_MASK(planemask);
}


void S3SubsequentScreenToScreenCopy(srcX, srcY, destX, destY, w, h)
    int srcX, srcY, destX, destY, w, h;
{
    w--; h--;
	
    if(!(BltDirection & INC_Y)) {
        srcY += h;
	destY += h;
    } 

    if(!(BltDirection & INC_X)) {
        srcX += w;
	destX += w;
    } 
 
    WaitQueue(7); 
    SET_CURPT((short)srcX,(short)srcY);              
    SET_DESTSTP((short)destX, (short)destY);
    SET_AXIS_PCNT((short)w,(short)h);
    SET_CMD(BltDirection);
}

	/***********************\
	|	Solid Rects	|
	\***********************/


void S3SetupForFillRectSolid(color, rop, planemask)
    int color, rop;
    unsigned planemask;
{
    BLOCK_CURSOR;
    WaitQueue16_32(3,5);
    SET_FRGD_COLOR(color);
    SET_FRGD_MIX(FSS_FRGDCOL | s3alu[rop]);
    SET_WRT_MASK(planemask);
}

void S3SubsequentFillRectSolid(x, y, w, h)
    int x, y, w, h;
{
    WaitQueue(5);
    SET_CURPT((short)x, (short)y);
    SET_AXIS_PCNT(w - 1, h - 1);
    SET_CMD(CMD_RECT | DRAW | INC_X | INC_Y | PLANAR | WRTDATA);
}

	/***********************\
	|	Lines		|
	\***********************/

#include "miline.h"

/* We override lines at the GC level so this will only be used
	for vertical lines in xf86PolyRectangleSolidZeroWidth and 
	in DGA.  We are very biased toward vertical lines. We don't
	clip here (MArk) */

void S3SubsequentTwoPointLine(x1, y1, x2, y2, bias)
    int x1, y1, x2, y2, bias;
{
    short length;

    if(x1 == x2) {
        unsigned short cmd;

   	if(y2 > y1) {	/* going down */
	   cmd = CMD_LINE | DRAW | LINETYPE | PLANAR | WRTDATA | (6 << 5);
	   length = (short)(y2 - y1);
    	} else {	/* going up   */
	   cmd = CMD_LINE | DRAW | LINETYPE | PLANAR | WRTDATA | (2 << 5);
	   length = (short)(y1 - y2); 
        }

	if(bias & 0x100) length--;
        
        if(length >= 0) {
     	    WaitQueue(4);
    	    SET_CURPT((short)x1, (short)y1);
    	    SET_MAJ_AXIS_PCNT(length);
    	    SET_CMD(cmd);
        }	
    } else if (y1 == y2) {
	if(x2 >= x1) {  /* going right */
	    length = x2 - x1;
	    if(bias & 0x100)
		length--;
        } else {       /* going left  */
     	    length = x1 - x2;
	    if(bias & 0x100) {
		length--;
		x1 = x2 + 1;
            } else x1 = x2;
        }

        if(length >= 0) {
    	    WaitQueue(5);
    	    SET_CURPT((short)x1, (short)y1);
    	    SET_AXIS_PCNT(length,0);
    	    SET_CMD(CMD_RECT | INC_Y | INC_X | DRAW | PLANAR | WRTDATA);
	}
    } else {
	 int e, e1, e2, adx, ady;
	 short octant = 0;
   	 unsigned short cmd = CMD_LINE | DRAW | PLANAR | WRTDATA | LASTPIX;

	 if ((adx = x2 - x1) >= 0) {
	    cmd |= INC_X;
	 } else {
	    adx = -adx;
	    octant |= XDECREASING;
         }

	 if ((ady = y2 - y1) >= 0) {
	    cmd |= INC_Y;
	 } else {
	    ady = -ady;
	    octant |= YDECREASING;
         }

	 if (adx > ady) {
	    e1 = ady << 1;
	    e2 = e1 - (adx << 1);
	    e = e1 - adx;
	 } else {
	    e1 = adx << 1;
	    e2 = e1 - (ady << 1);
	    e = e1 - ady;
	    cmd |= YMAJAXIS;
	    octant |= YMAJOR;
	 }

	 FIXUP_ERROR(e, octant, bias & 0xff);
         
         length = (cmd & YMAJAXIS) ? (short)ady : (short)adx;
	 if(!(bias & 0x100)) length++;

/* Did I do this correctly?  (MArk) */

	 WaitQueue(7);
	 SET_CURPT((short)x1, (short)y1);
	 SET_ERR_TERM((short)e);
	 SET_DESTSTP((short)e2, (short)e1);
	 SET_MAJ_AXIS_PCNT(length);
	 SET_CMD(cmd);
    }

}

void S3SetClippingRectangle(x1, y1, x2, y2)
    int x1, y1, x2, y2;
{
    SET_SCISSORS((short)x1,(short)y1,(short)x2,(short)y2);
}


#if 0

/* No longer used. It's nice and clean but is significantly slower */

void S3SubsequentBresenhamLine(x1, y1, octant, err, e1, e2, length)
    int x1, y1, octant, err, e1, e2, length;
{
    unsigned short cmd = CMD_LINE | DRAW | PLANAR | WRTDATA | LASTPIX;

    if(octant & YMAJOR) cmd |= YMAJAXIS;
    if(!(octant & XDECREASING)) cmd |= INC_X;
    if(!(octant & YDECREASING)) cmd |= INC_Y;

    WaitQueue(7);
    SET_CURPT((short)x1, (short)y1);
    SET_ERR_TERM((short)err);
    SET_DESTSTP((short)e2, (short)e1);
    SET_MAJ_AXIS_PCNT((short)length);
    SET_CMD(cmd);
}

#endif


	/*******************************\
	| 	8x8 Fill Patterns	|
	\*******************************/

/* this is broken? (MArk) */
void S3SetupForFill8x8Pattern(patternx, patterny, rop, planemask, trans_col)
    int patternx, patterny, rop, planemask, trans_col;
{
    BLOCK_CURSOR;
    WaitQueue16_32(3,4);
    SET_MIX(FSS_BITBLT | s3alu[rop],BSS_BKGDCOL | MIX_SRC);
    SET_WRT_MASK(planemask);
}

void S3SubsequentFill8x8Pattern(patternx, patterny, x, y, w, h)
    int patternx, patterny, x, y, w, h;
{
    WaitQueue(7);
/* I don't know about this (MArk) */
    SET_CURPT((short)patternx, (short)patterny); 
    SET_DESTSTP((short)x, (short)y);
    SET_AXIS_PCNT(w - 1, h - 1);
    SET_CMD(CMD_PFILL | DRAW | INC_Y | INC_X | PLANAR | WRTDATA);
}



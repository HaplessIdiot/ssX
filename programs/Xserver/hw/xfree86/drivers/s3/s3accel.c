/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/s3/s3accel.c,v 1.2 1997/03/10 10:12:09 hohndel Exp $ */

/*
 *
 * Copyright 1996-1997 The XFree86 Project, Inc.
 *
 */


#define PSZ 8

#include "vga256.h"
#include "xf86.h"
#include "vga.h"

#include "xf86xaa.h"

#include "s3.h"
#include "s3reg.h"

void S3Sync();
void S3SetupForScreenToScreenCopy();
void S3SubsequentScreenToScreenCopy();
void S3SetupForFillRectSolid();
void S3SubsequentFillRectSolid();
void S3SubsequentBresenhamLine();
void S3SetupForFill8x8Pattern();
void S3SubsequentFill8x8Pattern();
void S3SetupForScanlineScreenToScreenColorExpand();
void S3SubsequentScanlineScreenToScreenColorExpand16();
void S3SubsequentScanlineScreenToScreenColorExpand32();

static Bool Transfer32 = FALSE;

static unsigned char ScratchBuffer[512];

void S3AccelInit() {

    if(S3_x64_SERIES(s3ChipId))
	Transfer32 = TRUE; 


    xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS  | PIXMAP_CACHE |
				 HARDWARE_PATTERN_NOT_LINEAR |
				COP_FRAMEBUFFER_CONCURRENCY; 
	
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
    xf86AccelInfoRec.SubsequentBresenhamLine = S3SubsequentBresenhamLine; 
    xf86AccelInfoRec.ErrorTermBits = 11;


    /* 8x8 pattern fills */
    if(S3_801_928_SERIES(s3ChipId)) {
       xf86AccelInfoRec.SetupForFill8x8Pattern = S3SetupForFill8x8Pattern;
       xf86AccelInfoRec.SubsequentFill8x8Pattern = S3SubsequentFill8x8Pattern;
    }

    /* Image Write */
    xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand = 
			S3SetupForScanlineScreenToScreenColorExpand;
    if(Transfer32) {
	xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
			S3SubsequentScanlineScreenToScreenColorExpand32;

    } else {
	xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
			S3SubsequentScanlineScreenToScreenColorExpand16;

    }

    xf86AccelInfoRec.ColorExpandFlags = SCANLINE_PAD_DWORD |
					BIT_ORDER_IN_BYTE_MSBFIRST|
					VIDEO_SOURCE_GRANULARITY_DWORD;
    xf86AccelInfoRec.ScratchBufferAddr = 1;
    xf86AccelInfoRec.ScratchBufferSize = 512;
    xf86AccelInfoRec.ScratchBufferBase = (void*)ScratchBuffer;
    xf86AccelInfoRec.PingPongBuffers = 1;


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
    BltDirection = CMD_BITBLT | DRAW | WRTDATA;

    if(xdir == 1) BltDirection |= INC_X;
    if(ydir == 1) BltDirection |= INC_Y;
   
    BLOCK_CURSOR;
    WaitQueue16_32(3,4);
    SET_PIX_CNTL(0);
    SET_FRGD_MIX(FSS_BITBLT | s3alu[rop]);
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
    WaitQueue16_32(4,6);
    SET_PIX_CNTL(0);
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
    SET_CMD(CMD_RECT | DRAW | INC_X | INC_Y  | WRTDATA);
}

	/***********************\
	|	Lines		|
	\***********************/

#include "miline.h"

void S3SubsequentBresenhamLine(x1, y1, octant, err, e1, e2, length)
    int x1, y1, octant, err, e1, e2, length;
{
    unsigned short cmd;

    if(e1) { 
	cmd = CMD_LINE | DRAW | WRTDATA | LASTPIX;

    	if(octant & YMAJOR) cmd |= YMAJAXIS;
    	if(!(octant & XDECREASING)) cmd |= INC_X;
    	if(!(octant & YDECREASING)) cmd |= INC_Y;

   	WaitQueue(7);
   	SET_CURPT((short)x1, (short)y1);
   	SET_ERR_TERM((short)err);
    	SET_DESTSTP((short)e2, (short)e1);
    	SET_MAJ_AXIS_PCNT((short)length);
   	SET_CMD(cmd);
    } else { /* vertical line */
   	if(octant & YDECREASING)   
   	    cmd = CMD_LINE | DRAW | LINETYPE | WRTDATA | VECDIR_090;
        else 
	    cmd = CMD_LINE | DRAW | LINETYPE | WRTDATA | VECDIR_270;
   
     	WaitQueue(4);
    	SET_CURPT((short)x1, (short)y1);
    	SET_MAJ_AXIS_PCNT((short)length - 1);
    	SET_CMD(cmd);
    }
}



	/*******************************\
	| 	8x8 Fill Patterns	|
	\*******************************/


void S3SetupForFill8x8Pattern(patternx, patterny, rop, planemask, trans_col)
    int patternx, patterny, rop, planemask, trans_col;
{
    BLOCK_CURSOR;
    WaitQueue16_32(3,4);
    SET_PIX_CNTL(0);
    SET_FRGD_MIX(FSS_BITBLT | s3alu[rop]);
    SET_WRT_MASK(planemask);
}

void S3SubsequentFill8x8Pattern(patternx, patterny, x, y, w, h)
    int patternx, patterny, x, y, w, h;
{
    WaitQueue(7);
    SET_CURPT((short)patternx, (short)patterny); 
    SET_DESTSTP((short)x, (short)y);
    SET_AXIS_PCNT(w - 1, h - 1);
    SET_CMD(CMD_PFILL | DRAW | INC_Y | INC_X | WRTDATA);
}


	/***************************************\
	|	Scanline Color Expand Hack  	|
	\***************************************/

static int ScanlineWordCount;

void S3SetupForScanlineScreenToScreenColorExpand(x, y, w, h, bg, fg,
						 rop, planemask)
   int x, y, w, h, bg, fg, rop, planemask;
{
    BLOCK_CURSOR;
    WaitQueue16_32(5,8);
    SET_FRGD_COLOR(fg);
    SET_FRGD_MIX(FSS_FRGDCOL | s3alu[rop]); 
    if(bg == -1) {  
    	SET_BKGD_MIX(BSS_BKGDCOL | MIX_XOR); 
    	SET_BKGD_COLOR(0);
    } else {
   	SET_BKGD_MIX(BSS_BKGDCOL | s3alu[rop]); 
    	SET_BKGD_COLOR(bg);
    }
    SET_WRT_MASK(planemask);

    WaitQueue(5);
    SET_CURPT((short)x, (short)y); 
    SET_PIX_CNTL(MIXSEL_EXPPC);
    SET_AXIS_PCNT(w - 1, h - 1);

    if(Transfer32) {
    	ScanlineWordCount = (w + 31) >> 5; 

    	WaitIdle();
    	SET_CMD(CMD_RECT | BYTSEQ | _32BIT | PCDATA | DRAW | PLANAR |
					INC_Y | INC_X | WRTDATA);
    } else {
    	ScanlineWordCount = (w + 15) >> 4;

    	WaitIdle();
    	SET_CMD(CMD_RECT | BYTSEQ | _16BIT | PCDATA | DRAW | PLANAR |
					INC_Y | INC_X | WRTDATA);
    }
}

void S3SubsequentScanlineScreenToScreenColorExpand16(int srcaddr)
{
    register unsigned short *ptr = (unsigned short*)ScratchBuffer;
    register int count = ScanlineWordCount;

    while(count--)
	SET_PIX_TRANS_W(*(ptr++)); 
}

void S3SubsequentScanlineScreenToScreenColorExpand32(int srcaddr)
{
    register unsigned long *ptr = (unsigned long*)ScratchBuffer;
    register int count = ScanlineWordCount;

    while(count--)
	SET_PIX_TRANS_L(*(ptr++)); 
}

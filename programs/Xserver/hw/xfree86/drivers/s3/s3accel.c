/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/s3/s3accel.c,v 1.9 1997/06/10 12:30:29 hohndel Exp $ */

/*
 *
 * Copyright 1996-1997 The XFree86 Project, Inc.
 *
 *
 *	Written by Mark Vojkovich (mvojkovi@ucsd.edu)
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
void S3SetupForDashedLine();
void S3SubsequentDashedBresenhamLine();
void S3SetupForCPUToScreenColorExpand();
#ifdef S3_NEWMMIO
 void S3SubsequentCPUToScreenColorExpand32();
#endif
void S3FillRectStippledCPUToScreenColorExpand();
void S3WriteBitmapCPUToScreenColorExpand32();
void S3WriteBitmapCPUToScreenColorExpand16();

static Bool Transfer32 = FALSE;
static Bool ColorExpandBug = FALSE;

static unsigned char ScratchBuffer[512];
static unsigned char SwappedBytes[256];

#define MAX_LINE_PATTERN_LENGTH 512
#define LINE_PATTERN_START	((MAX_LINE_PATTERN_LENGTH >> 5) - 1)
static CARD32 DashPattern[MAX_LINE_PATTERN_LENGTH >> 5];


void S3AccelInit() 
{
#ifndef S3_NEWMMIO
    if(S3_x64_SERIES(s3ChipId))
#endif
	Transfer32 = TRUE; 

    if(S3_x68_SERIES(s3ChipId))
	ColorExpandBug = TRUE;

    xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS  | PIXMAP_CACHE |
				COP_FRAMEBUFFER_CONCURRENCY |
				LINE_PATTERN_MSBFIRST_INCREASING |
				DELAYED_SYNC;
 
    xf86AccelInfoRec.PatternFlags = HARDWARE_PATTERN_NOT_LINEAR;
	
    xf86AccelInfoRec.Sync = S3Sync;

    /* copy area */
    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY;
    xf86AccelInfoRec.SetupForScreenToScreenCopy =
        			S3SetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
        			S3SubsequentScreenToScreenCopy;

    /* filled rects */
    xf86AccelInfoRec.SetupForFillRectSolid = S3SetupForFillRectSolid;
    xf86AccelInfoRec.SubsequentFillRectSolid = S3SubsequentFillRectSolid; 


    /* lines */
    xf86AccelInfoRec.SubsequentBresenhamLine = S3SubsequentBresenhamLine;
    if(S3_911_SERIES(s3ChipId)) 
    	xf86AccelInfoRec.ErrorTermBits = 11;
    else
    	xf86AccelInfoRec.ErrorTermBits = 12;

    /* dashed lines */
    if(s3Bpp != 3) {
    	xf86AccelInfoRec.SetupForDashedLine = S3SetupForDashedLine;
    	xf86AccelInfoRec.LinePatternBuffer = (void*)DashPattern;     
    	xf86AccelInfoRec.LinePatternMaxLength = MAX_LINE_PATTERN_LENGTH;
       	xf86AccelInfoRec.SubsequentDashedBresenhamLine = 
				S3SubsequentDashedBresenhamLine; 
    }

    /* 8x8 pattern fills */
    if(!S3_911_SERIES(s3ChipId)) {
       xf86AccelInfoRec.SetupForFill8x8Pattern = S3SetupForFill8x8Pattern;
       xf86AccelInfoRec.SubsequentFill8x8Pattern = S3SubsequentFill8x8Pattern;
    }


    /* Color Expand */
#ifdef S3_NEWMMIO 
    xf86AccelInfoRec.SetupForCPUToScreenColorExpand =  
				S3SetupForCPUToScreenColorExpand;
    xf86AccelInfoRec.SubsequentCPUToScreenColorExpand =
				S3SubsequentCPUToScreenColorExpand32;

    xf86AccelInfoRec.CPUToScreenColorExpandBase = (void*) &IMG_TRANS;
    xf86AccelInfoRec.CPUToScreenColorExpandRange = 0x8000;

    xf86AccelInfoRec.ColorExpandFlags = CPU_TRANSFER_PAD_DWORD |
				       	CPU_TRANSFER_BASE_FIXED |
					BIT_ORDER_IN_BYTE_MSBFIRST |
					SCANLINE_PAD_DWORD;

#else
    xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand = 
			S3SetupForScanlineScreenToScreenColorExpand;
    if(Transfer32) {
	xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
			S3SubsequentScanlineScreenToScreenColorExpand32;
    } else {
	xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
			S3SubsequentScanlineScreenToScreenColorExpand16;
    }

    xf86AccelInfoRec.ColorExpandFlags = BIT_ORDER_IN_BYTE_MSBFIRST;

    xf86AccelInfoRec.ScratchBufferAddr = 1;
    xf86AccelInfoRec.ScratchBufferSize = 512;
    xf86AccelInfoRec.ScratchBufferBase = (void*)ScratchBuffer;
    xf86AccelInfoRec.PingPongBuffers = 1;

#endif


    xf86AccelInfoRec.FillRectOpaqueStippled =
                S3FillRectStippledCPUToScreenColorExpand;
    xf86AccelInfoRec.FillRectStippled =
                S3FillRectStippledCPUToScreenColorExpand;

    if(Transfer32)
     	xf86AccelInfoRec.WriteBitmap =
		S3WriteBitmapCPUToScreenColorExpand32;
    else
      	xf86AccelInfoRec.WriteBitmap =
		S3WriteBitmapCPUToScreenColorExpand16;

    /* pixmap cache */    
    xf86AccelInfoRec.PixmapCacheMemoryStart =
	(s3CursorStartY + s3CursorLines) * s3BppDisplayWidth;
    xf86AccelInfoRec.PixmapCacheMemoryEnd =
        vga256InfoRec.videoRam * 1024;

    {
	int i,j;
	register unsigned char newbyte;
	register unsigned char oldbyte;

    	for(i = 0; i < 256; i++) {
	    oldbyte = i;
	    newbyte = 0;
	    j = 8;
	    while(j--) {
	        newbyte <<= 1;	
		if(oldbyte & 0x01) newbyte++;
		oldbyte >>= 1;
	    }
	    SwappedBytes[i] = newbyte;
    	}
    }

}

		/******************\
		|	Sync	   |
		\******************/

void S3Sync() {
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

    /* Note: 
	We rely on the fact that XAA will never send us a horizontal line
    */
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
	|    CPU to Screen Color Expansion 	|
	\***************************************/


void S3SetupForCPUToScreenColorExpand(bg, fg, rop, planemask)
    int bg, fg, rop;
    unsigned planemask;
{
    WaitQueue16_32(3, 4);
    if(bg == -1) {  
	if(ColorExpandBug) {
    	  SET_MIX(FSS_FRGDCOL | s3alu[rop], BSS_BKGDCOL | MIX_XOR); 
    	  SET_BKGD_COLOR(0);
	} else
    	  SET_MIX(FSS_FRGDCOL | s3alu[rop], BSS_BKGDCOL | MIX_DST); 
    } else {
   	SET_MIX(FSS_FRGDCOL | s3alu[rop], BSS_BKGDCOL | s3alu[rop]); 
    	SET_BKGD_COLOR(bg);
    }

    WaitQueue16_32(3, 5);
    SET_FRGD_COLOR(fg);
    SET_WRT_MASK(planemask);
    SET_PIX_CNTL(MIXSEL_EXPPC);
}


#ifdef S3_NEWMMIO

void S3SubsequentCPUToScreenColorExpand32(x, y, w, h, skipleft)
    int x, y, w, h, skipleft;
{
    WaitQueue(4);
    SET_CURPT((short)x, (short)y); 
    SET_AXIS_PCNT((short)w - 1, (short)h - 1);

    WaitIdle();
    SET_CMD(CMD_RECT | BYTSEQ | _32BIT | PCDATA | DRAW | PLANAR |
					INC_Y | INC_X | WRTDATA);
}

#endif

	/***********************************************\
	|	Indirect Color Expansion Hack		|
	\***********************************************/

static int ScanlineWordCount;

void S3SetupForScanlineScreenToScreenColorExpand(x, y, w, h, bg, fg,
						 rop, planemask)
   int x, y, w, h, bg, fg, rop, planemask;
{
    S3SetupForCPUToScreenColorExpand(bg, fg, rop, planemask);

    WaitQueue(4);
    SET_CURPT((short)x, (short)y); 
    SET_AXIS_PCNT((short)w - 1, (short)h - 1);

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
    register CARD32 *ptr = (CARD32*)ScratchBuffer;
    register int count = ScanlineWordCount;

    while(count--)
	SET_PIX_TRANS_L(*(ptr++)); 
}


	/***********************\
	|	Dashed Lines	|
	\***********************/


static int DashPatternSize;
static Bool NicePattern;

void S3SetupForDashedLine(fg, bg, rop, planemask, size)
    int fg, bg, rop, planemask, size;
{
    register CARD32 scratch = DashPattern[LINE_PATTERN_START];

    S3SetupForCPUToScreenColorExpand(bg, fg, rop, planemask);

    NicePattern = FALSE;

    if(size <= 32) {
	if(size & (size - 1)) {
	  	while(size < 16) {
		   scratch |= (scratch >> size);
		   size <<= 1;
	  	}
		scratch |= (scratch >> size);
	 	DashPattern[LINE_PATTERN_START] = scratch;
	 } else { 
          	switch(size) {
	  	   case 2:	scratch |= scratch >> 2;
	  	   case 4:	scratch |= scratch >> 4;
	  	   case 8:	scratch |= scratch >> 8;
	  	   case 16:	scratch |= scratch >> 16;
	 			DashPattern[LINE_PATTERN_START] = scratch;
		   case 32:	NicePattern = TRUE;
	  	   default:	break;	
		}		
        }
    }
    DashPatternSize = size;
}

void S3SubsequentDashedBresenhamLine(x1, y1, octant, err, e1, e2, length,
							start)
    int x1, y1, octant, err, e1, e2, length, start;
{
    register int count;
    register CARD32 pattern;
    unsigned short cmd;
    Bool plus_one;

    if(Transfer32) {
	cmd = _32BIT | PLANAR | WRTDATA | DRAW | PCDATA;
	count = (length + 31) >> 5;
    } else {
	cmd = _16BIT | PLANAR | WRTDATA | DRAW | PCDATA;
	count = (length + 15) >> 4;
	plus_one = (count & 0x01);
	count >>= 1;
    }

    if(e1) {
	cmd |= LASTPIX | CMD_LINE;

       	if(octant & YMAJOR) cmd |= YMAJAXIS;
    	if(!(octant & XDECREASING)) cmd |= INC_X;
    	if(!(octant & YDECREASING)) cmd |= INC_Y;

    	WaitQueue(7);
    	SET_CURPT((short)x1, (short)y1);
    	SET_ERR_TERM((short)err);
    	SET_DESTSTP((short)e2, (short)e1);
    	SET_MAJ_AXIS_PCNT((short)length);
    	SET_CMD(cmd);
    } else {
	if (octant & YMAJOR){
     	    WaitQueue(4);
    	    SET_CURPT((short)x1, (short)y1);
    	    SET_MAJ_AXIS_PCNT((short)length - 1);

   	    if(octant & YDECREASING) {   
    	    	SET_CMD(cmd | CMD_LINE | LINETYPE | VECDIR_090);
            } else {
    	    	SET_CMD(cmd | CMD_LINE | LINETYPE | VECDIR_270);
	    }
	} else {
    	    if(octant & XDECREASING) { 
 		WaitQueue(4);
    	    	SET_CURPT((short)x1, (short)y1);
    	    	SET_MAJ_AXIS_PCNT((short)length - 1);
    	    	SET_CMD(cmd | CMD_LINE | LINETYPE | VECDIR_180);
            } else { 	/* he he */
    		WaitQueue(4);
    		SET_CURPT((short)x1, (short)y1); 
    		SET_AXIS_PCNT((short)length - 1, 0);

    		WaitIdle();
    		SET_CMD(cmd | CMD_RECT | INC_Y | INC_X);
	    } 
	}
    }

    if(NicePattern) {
	pattern = (start) ? 
	     (DashPattern[LINE_PATTERN_START] << start) |
 		(DashPattern[LINE_PATTERN_START] >> (32 - start)) :
			DashPattern[LINE_PATTERN_START];

	if(Transfer32) {
	   while(count--)
	   	SET_PIX_TRANS_L(pattern);
	} else {
	   while(count--) {
	   	SET_PIX_TRANS_W(pattern >> 16);
	   	SET_PIX_TRANS_W(pattern);
	   }
	   if(plus_one)
	   	SET_PIX_TRANS_W(pattern >> 16);
	}
    } else if(DashPatternSize < 32) {
	register int offset = start;

	if(Transfer32) {
	    while(count--) {
	    	SET_PIX_TRANS_L((DashPattern[LINE_PATTERN_START] << offset) | 
	   	 (DashPattern[LINE_PATTERN_START] >> (DashPatternSize-offset)));
		offset += 32;
		while(offset > DashPatternSize)
		    offset -= DashPatternSize;
	    }
	} else {
	    while(count--) {
		pattern = (DashPattern[LINE_PATTERN_START] << offset) | 
		 (DashPattern[LINE_PATTERN_START] >> (DashPatternSize-offset));
	    	SET_PIX_TRANS_W(pattern >> 16);
		offset += 32;
		while(offset > DashPatternSize)
		    offset -= DashPatternSize;
	    	SET_PIX_TRANS_W(pattern);
	    }
 	    if(plus_one) 
		SET_PIX_TRANS_W(((DashPattern[LINE_PATTERN_START] << offset) | 
			(DashPattern[LINE_PATTERN_START] >> 
				(DashPatternSize - offset))) >> 16);
	}
    } else if(Transfer32) { 
	int offset = start;
	register unsigned char* srcp = (unsigned char*)(DashPattern) + 
					(MAX_LINE_PATTERN_LENGTH >> 3) - 1;
	register CARD32* scratch;
	int scratch2, shift;
	   	   	
	while(count--) {
	   	shift = DashPatternSize - offset;
		scratch = (CARD32*)(srcp - (offset >> 3) - 3);
		scratch2 = offset & 0x07;

		if(shift & ~31) {
		   if(scratch2) {
		      pattern =	(*scratch << scratch2) |
				(*(scratch - 1) >> (32 - scratch2));
		   } else 
		       pattern = *scratch; 
		} else {
		    pattern = (*((CARD32*)(srcp - 3)) >> shift) | 
			((*scratch & ~(0xFFFFFFFF >> shift)) << scratch2);

		}
	   	SET_PIX_TRANS_L(pattern);
		offset += 32;
		while(offset >= DashPatternSize) 
		    offset -= DashPatternSize;
	}	
    } else {
	int offset = start;
	register unsigned char* srcp = (unsigned char*)(DashPattern) + 
					(MAX_LINE_PATTERN_LENGTH >> 3) - 1;
	register CARD32* scratch;
	int scratch2, shift;

	while(count--) {
	   	shift = DashPatternSize - offset;
		scratch = (CARD32*)(srcp - (offset >> 3) - 3);
		scratch2 = offset & 0x07;

		if(shift & ~31) {
		   if(scratch2) {
		      pattern =	(*scratch << scratch2) |
				(*(scratch - 1) >> (32 - scratch2));
		   } else 
		       pattern = *scratch; 
		} else {
		    pattern = (*((CARD32*)(srcp - 3)) >> shift) | 
			((*scratch & ~(0xFFFFFFFF >> shift)) << scratch2);

		}
	   	SET_PIX_TRANS_W(pattern >> 16);
		offset += 32;
		while(offset >= DashPatternSize) 
		    offset -= DashPatternSize;
	   	SET_PIX_TRANS_W(pattern);
	}
	if(plus_one) {	
	   	shift = DashPatternSize - offset;
		scratch = (CARD32*)(srcp - (offset >> 3) - 3);
		scratch2 = offset & 0x07;

		if(shift & ~31) {
		   if(scratch2) {
		      pattern =	(*scratch << scratch2) |
				(*(scratch - 1) >> (32 - scratch2));
		   } else 
		       pattern = *scratch; 
		} else {
		    pattern = (*((CARD32*)(srcp - 3)) >> shift) | 
			((*scratch & ~(0xFFFFFFFF >> shift)) << scratch2);

		}
	   	SET_PIX_TRANS_W(pattern >> 16);
	}
    }
}


#if defined(__GNUC__) && defined(__i386__)
static __inline__ CARD32 reverse_bitorder(CARD32 data) {
#if defined(Lynx) || (defined(SYSV) || defined(SVR4)) && !defined(ACK_ASSEMBLER) || (defined(linux) || defined (__OS2ELF__)) && defined(__ELF__)
	__asm__(
		"movl $0,%%ecx\n"
		"movb %%al,%%cl\n"
		"movb SwappedBytes(%%ecx),%%al\n"
		"movb %%ah,%%cl\n"
		"movb SwappedBytes(%%ecx),%%ah\n"
		"roll $16,%%eax\n"
		"movb %%al,%%cl\n"
		"movb SwappedBytes(%%ecx),%%al\n"
		"movb %%ah,%%cl\n"
		"movb SwappedBytes(%%ecx),%%ah\n"
		"roll $16,%%eax\n"
		: "=a" (data) : "0" (data)
		: "cx"
		);
#else
	__asm__(
		"movl $0,%%ecx\n"
		"movb %%al,%%cl\n"
		"movb _SwappedBytes(%%ecx),%%al\n"
		"movb %%ah,%%cl\n"
		"movb _SwappedBytes(%%ecx),%%ah\n"
		"roll $16,%%eax\n"
		"movb %%al,%%cl\n"
		"movb _SwappedBytes(%%ecx),%%al\n"
		"movb %%ah,%%cl\n"
		"movb _SwappedBytes(%%ecx),%%ah\n"
		"roll $16,%%eax\n"
		: "=a" (data) : "0" (data)
		: "cx"
		);
#endif
	return data;
}
#else
static __inline__ CARD32 reverse_bitorder(CARD32 data) {
    unsigned char* kludge = (unsigned char*)&data;

    kludge[0] = SwappedBytes[kludge[0]];
    kludge[1] = SwappedBytes[kludge[1]];
    kludge[2] = SwappedBytes[kludge[2]];
    kludge[3] = SwappedBytes[kludge[3]];
	
    return data;	
}
#endif

static void
S3FillStippledCPUToScreenColorExpand(x, y, w, h, src, srcwidth,
stipplewidth, stippleheight, srcx, srcy)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int stipplewidth, stippleheight;
    int srcx, srcy;
{
    unsigned char *srcp = (srcwidth * srcy) + src;
    int dwords, count; 
    Bool PlusOne;

    WaitQueue(4);
    SET_CURPT((short)x, (short)y); 
    SET_AXIS_PCNT((short)w - 1, (short)h - 1);

    WaitIdle();	
    if(Transfer32) {
    	SET_CMD(CMD_RECT | BYTSEQ | _32BIT | PCDATA | DRAW | PLANAR |
					INC_Y | INC_X | WRTDATA);
	dwords = (w + 31) >> 5;    
    } else {
    	SET_CMD(CMD_RECT | BYTSEQ | _16BIT | PCDATA | DRAW | PLANAR |
					INC_Y | INC_X | WRTDATA);
	dwords = (w + 15) >> 4;  
	PlusOne = (dwords & 0x01);
	dwords >>= 1;  
    }

    if(!((stipplewidth > 32) || (stipplewidth & (stipplewidth - 1)))) { 
    	CARD32 pattern;
    	register unsigned char* kludge = (unsigned char*)(&pattern);

	while(h--) {
	   switch(stipplewidth) {
		case 32:
	   	    pattern = *((CARD32*)srcp);  
		    break;
	      	case 16:
		    kludge[0] = kludge[2] = srcp[0];
		    kludge[1] = kludge[3] = srcp[1];
		    break;
	      	case 8:
		    kludge[0] = kludge[1] = kludge[2] = kludge[3] = srcp[0];
		    break;
	      	case 4:
		    kludge[0] = kludge[1] = kludge[2] = kludge[3] = 
				(srcp[0] & 0x0F);
		    pattern |= (pattern << 4);
		case 2:
		    kludge[0] = kludge[1] = kludge[2] = kludge[3] = 
				(srcp[0] & 0x03);
		    pattern |= (pattern << 2);
		    pattern |= (pattern << 4);
		default:	/* case 1: */
		    if(srcp[0] & 0x01) 
			pattern = 0xffffffff;
		    else
			pattern = 0x00000000;
		    break;
	   }

	   if(srcx) 
		pattern = (pattern >> srcx) | (pattern << (32 - srcx));           
	   pattern = reverse_bitorder(pattern);

	   count = dwords;
	   if(Transfer32) {
	      while(count--)
	   	SET_PIX_TRANS_L(pattern);
	   } else {
	      while(count--) {
	   	SET_PIX_TRANS_W(pattern);
	   	SET_PIX_TRANS_W(*((unsigned short*)(&kludge[2])));
	      }
	      if(PlusOne)
	   	SET_PIX_TRANS_W(pattern);
	   }

	   srcy++;
	   srcp += srcwidth;
	   if (srcy >= stippleheight) {
	     srcy = 0;
	     srcp = src;
	   }
	}
    } else if(stipplewidth < 32) {
	int width;
	int offset;
	register CARD32 pattern2;
	register CARD32 pattern;

	while(h--) {
	   width = stipplewidth;
	   pattern = *((CARD32*)srcp) & (0xFFFFFFFF >> (32 - width));  
	   while(!(width & ~15)) {
		pattern |= (pattern << width);
		width <<= 1;	
	   }
	   pattern |= (pattern << width);
 

	   offset = srcx;

	   count = dwords;
	   if(Transfer32) {
	     while(count--) {
	   	SET_PIX_TRANS_L(reverse_bitorder((pattern >> offset) | 
				(pattern << (width - offset))));
		offset += 32;
		while(offset >= width) 
		    offset -= width;
	     }
	   } else {
	     while(count--) {
		pattern2 = reverse_bitorder((pattern >> offset) | 
				(pattern << (width - offset)));
	   	SET_PIX_TRANS_W(pattern2); 
		offset += 32;
		while(offset >= width) 
		    offset -= width;
	   	SET_PIX_TRANS_W(pattern2 >> 16);
	     }
	     if(PlusOne) 
	   	SET_PIX_TRANS_W(reverse_bitorder((pattern >> offset) | 
				(pattern << (width - offset))));
	   }

	   srcy++;
	   srcp += srcwidth;
	   if (srcy >= stippleheight) {
	     srcy = 0;
	     srcp = src;
	   }
	}
    } else if(Transfer32){
	register CARD32* scratch;
	register CARD32 pattern;
	int shift, offset, scratch2;

	while(h--) {
	   count = dwords;
	   offset = srcx;
	   	   	
	   while(count--) {
	   	shift = stipplewidth - offset;
		scratch = (CARD32*)(srcp + (offset >> 3));
		scratch2 = offset & 0x07;

		if(shift & ~31) {
		   if(scratch2) {
		      pattern = (*scratch >> scratch2) |
			(*(scratch + 1) << (32 - scratch2));
		   } else 
		       pattern = *scratch; 
		} else {
		    pattern = (*((CARD32*)srcp) << shift) |
			((*scratch >> scratch2) & ((1 << shift) - 1));
		}
	   	SET_PIX_TRANS_L(reverse_bitorder(pattern));
		offset += 32;
		while(offset >= stipplewidth) 
		    offset -= stipplewidth;
	   }	

	   srcy++;
	   srcp += srcwidth;
	   if (srcy >= stippleheight) {
	     srcy = 0;
	     srcp = src;
	   }
	}
    } else {
	register CARD32* scratch;
	register CARD32 pattern;
	int shift, offset, scratch2;

	while(h--) {
	   count = dwords;
	   offset = srcx;
	   	   	
	   while(count--) {
	   	shift = stipplewidth - offset;
		scratch = (CARD32*)(srcp + (offset >> 3));
		scratch2 = offset & 0x07;

		if(shift & ~31) {
		   if(scratch2) {
		      pattern = (*scratch >> scratch2) |
			(*(scratch + 1) << (32 - scratch2));
		   } else 
		       pattern = *scratch; 
		} else {
		    pattern = (*((CARD32*)srcp) << shift) |
			((*scratch >> scratch2) & ((1 << shift) - 1));
		}
		pattern = reverse_bitorder(pattern);
	   	SET_PIX_TRANS_W(pattern);
		offset += 32;
		while(offset >= stipplewidth) 
		    offset -= stipplewidth;
	   	SET_PIX_TRANS_W(pattern >> 16);
	   }
	   if(PlusOne) {
	   	shift = stipplewidth - offset;
		scratch = (CARD32*)(srcp + (offset >> 3));
		scratch2 = offset & 0x07;

		if(shift & ~31) {
		   if(scratch2) {
		      pattern = (*scratch >> scratch2) |
			(*(scratch + 1) << (32 - scratch2));
		   } else 
		       pattern = *scratch; 
		} else {
		    pattern = (*((CARD32*)srcp) << shift) |
			((*scratch >> scratch2) & ((1 << shift) - 1));
		}
	   	SET_PIX_TRANS_W(reverse_bitorder(pattern));
	   }	

	   srcy++;
	   srcp += srcwidth;
	   if (srcy >= stippleheight) {
	     srcy = 0;
	     srcp = src;
	   }
	}
    } 

    SET_SYNC_FLAG;
}



void
S3FillRectStippledCPUToScreenColorExpand(pDrawable, pGC, nBoxInit, pBoxInit)
    DrawablePtr pDrawable;
    GCPtr pGC;
    int nBoxInit;		/* number of rectangles to fill */
    BoxPtr pBoxInit;		/* Pointer to first rectangle to fill */
{
    PixmapPtr pPixmap;		/* Pixmap of the area to draw */
    int rectWidth;		/* Width of the rect to be drawn */
    int rectHeight;		/* Height of the rect to be drawn */
    BoxPtr pBox;		/* current rectangle to fill */
    int nBox;			/* Number of rectangles to fill */
    int xoffset, yoffset;
    Bool AlreadySetup = FALSE;

    pPixmap = pGC->stipple;

    for (nBox = nBoxInit, pBox = pBoxInit; nBox > 0; nBox--, pBox++) {

	rectWidth = pBox->x2 - pBox->x1;
	rectHeight = pBox->y2 - pBox->y1;

	if ((rectWidth > 0) && (rectHeight > 0)) {
	    if(!AlreadySetup) {
    		S3SetupForCPUToScreenColorExpand(
	    		(pGC->fillStyle == FillStippled) ? -1 : pGC->bgPixel, 
			pGC->fgPixel, pGC->alu, pGC->planemask);
		AlreadySetup = TRUE;
	    }

	    xoffset = (pBox->x1 - (pGC->patOrg.x + pDrawable->x))
	        % pPixmap->drawable.width;
	    if (xoffset < 0)
	        xoffset += pPixmap->drawable.width;
	    yoffset = (pBox->y1 - (pGC->patOrg.y + pDrawable->y))
	        % pPixmap->drawable.height;
	    if (yoffset < 0)
	        yoffset += pPixmap->drawable.height;
	    S3FillStippledCPUToScreenColorExpand(
	        pBox->x1, pBox->y1, rectWidth, rectHeight,
	        pPixmap->devPrivate.ptr, pPixmap->devKind,
	        pPixmap->drawable.width, pPixmap->drawable.height,
	        xoffset, yoffset);
	}
    }	/* end for loop through each rectangle to draw */
    return;
}



void S3WriteBitmapCPUToScreenColorExpand32(x, y, w, h, src, srcwidth, srcx,
srcy, bg, fg, rop, planemask)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int srcx, srcy;
    int bg, fg;
    int rop;
    unsigned int planemask;
{
    unsigned char *srcp = (srcwidth * srcy) + (srcx >> 3) + src;
    int shift = srcx & 0x07;
    int dwords;
    register int count; 
    register CARD32* pattern;

    S3SetupForCPUToScreenColorExpand(bg, fg, rop, planemask);

    WaitQueue(4);
    SET_CURPT((short)x, (short)y); 
    SET_AXIS_PCNT((short)w - 1, (short)h - 1);

    dwords = (w + 31) >> 5;    
    
    WaitIdle();	
    SET_CMD(CMD_RECT | BYTSEQ | _32BIT | PCDATA | DRAW | PLANAR |
					INC_Y | INC_X | WRTDATA);

    if(shift) {
    	while(h--) {
	    count = dwords;
	    pattern = (CARD32*)srcp;
	    while(count--) {
	   	SET_PIX_TRANS_L(reverse_bitorder((*pattern >> shift) | 
				(*(pattern + 1) << (32 - shift))));
		pattern++;
	    }
	    srcp += srcwidth;
    	}    
    } else {
    	while(h--) {
	    count = dwords;
	    pattern = (CARD32*)srcp;
	    while(count--) 
	   	SET_PIX_TRANS_L(reverse_bitorder(*(pattern++)));
	    srcp += srcwidth;
    	}    
    }

    SET_SYNC_FLAG;	
}

void S3WriteBitmapCPUToScreenColorExpand16(x, y, w, h, src, srcwidth, srcx,
srcy, bg, fg, rop, planemask)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int srcx, srcy;
    int bg, fg;
    int rop;
    unsigned int planemask;
{
    unsigned char *srcp = (srcwidth * srcy) + (srcx >> 3) + src;
    int shift = srcx & 0x07;
    int dwords, count; 
    register CARD32* pattern;
    register CARD32 pattern2;
    Bool PlusOne;

    S3SetupForCPUToScreenColorExpand(bg, fg, rop, planemask);

    WaitQueue(4);
    SET_CURPT((short)x, (short)y); 
    SET_AXIS_PCNT((short)w - 1, (short)h - 1);

    dwords = (w + 15) >> 4;  
    PlusOne = (dwords & 0x01);
    dwords >>= 1;  

    WaitIdle();	
    SET_CMD(CMD_RECT | BYTSEQ | _16BIT | PCDATA | DRAW | PLANAR |
					INC_Y | INC_X | WRTDATA);

    if(shift) {
    	while(h--) {
	    count = dwords;
	    pattern = (CARD32*)srcp;
	    while(count--) {
		pattern2 = reverse_bitorder((*pattern >> shift) | 
				(*(pattern + 1) << (32 - shift)));
	   	SET_PIX_TRANS_W(pattern2);
		pattern++;
	   	SET_PIX_TRANS_W(pattern2 >> 16);
	    }
	    if(PlusOne) 
	   	SET_PIX_TRANS_W(reverse_bitorder((*pattern >> shift) | 
				(*(pattern + 1) << (32 - shift))));
	    srcp += srcwidth;
    	}    
    } else {
    	while(h--) {
	    count = dwords;
	    pattern = (CARD32*)srcp;
	    while(count--) { 
		pattern2 = reverse_bitorder(*(pattern++));
	   	SET_PIX_TRANS_W(pattern2);
	   	SET_PIX_TRANS_W(pattern2 >> 16);
	    }
	    if(PlusOne)
	   	SET_PIX_TRANS_W(reverse_bitorder(*(pattern)));
	    srcp += srcwidth;
    	}    
    }

    SET_SYNC_FLAG;	
}


/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/agx/agxIm.c,v 3.4 1994/09/07 15:47:26 dawes Exp $ */
/*
 * Copyright 1992,1993 by Kevin E. Martin, Chapel Hill, North Carolina.
 * Copyright 1994 by Henry A. Worth, Sunnyvale, California.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Kevin E. Martin not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Kevin E. Martin makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * KEVIN E. MARTIN, RICKARD E. FAITH, TIAGO GONS AND HENRY A. WORTH
 * DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL 
 * THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA 
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER 
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
 * Modified for the Mach32 by Kevin E. Martin (martin@cs.unc.edu)
 * Rik Faith (faith@cs.unc.edu), Mon Jul  5 20:00:01 1993:
 *   Added 16-bit and some 64kb aperture optimizations.
 *   Waffled in stipple optimization by Jon Tombs (from s3/s3im.c)
 *   Added outsw code.
 * Modified for the AGX by Henry A. Worth (haw30@eng.amdahl.com)
 *
 */

#include "X.h"
#include "misc.h"
#include "xf86.h"
#include "xf86_HWlib.h"
#include "agx.h"
#include "regagx.h"
#include "agxIm.h"

void	(*agxImageReadFunc)();
void	(*agxImageWriteFunc)();
void	(*agxImageFillFunc)();

static	void	agxImageFillBank();
static	void	agxImageFillNoMem();
static	void	agxImageReadBank();
static	void	agxImageWriteBank();
static	void	agxImageWriteNoMem();

static unsigned long PMask;
static int BytesPerPixelShift;
static int screenStride;
unsigned char agxVideoMapFormat;

#define MAP_MNG 1

#if 1
#define agxSetVGAPage(bank) outb( agxApIdxReg, bank );
#else
#ifdef __GNUC__

#ifdef NO_INLINE
#define __inline__ /**/
#endif

static __inline__ void agxSetVGAPage(unsigned char bank)
{
	outb( agxApIdxReg, bank );
}

#else /* ! __GNUC__ */

#ifdef __STDC__
static void agxSetVGAPage(unsigned char bank)
#else
static void agxSetVGAPage(bank)
unsigned char bank;
#endif
{
	outb( agxApIdxReg, bank );
}

#endif /* __GNUC__ */
#endif

void
agxImageInit()
{
    int i;

    PMask = (1UL << agxInfoRec.depth) - 1;
    for( BytesPerPixelShift = 0, i = 1; 
         i < agxInfoRec.bitsPerPixel;
         BytesPerPixelShift++, i <<= 1   );
    agxVideoMapFormat = (BytesPerPixelShift & 0x07) | GE_MF_MOTO_FORMAT;
    BytesPerPixelShift -= 3;
    screenStride = agxVirtX << BytesPerPixelShift;


#if 0
    if (xgaUse4MbAperture) {
	/* This code does not need to deal with bank select at all */
	agxImageReadFunc = agxImageRead;
	agxImageWriteFunc = agxImageWrite;
	agxImageFillFunc = agxImageFill;
    }
    else {
#else
    {
#endif

	/* This code does need to deal with bank select */
	agxImageReadFunc = agxImageReadBank;
	agxImageWriteFunc = agxImageWriteBank;
	agxImageFillFunc = agxImageFillBank;
    }
}

/*
 * agxBytePadScratchMapPow2()
 *
 * Determine the map byte width, subject to the constraint that it be
 * a power of 2 and padded out to a byte boundary. Return the amount
 * that a one needs to be shifted by to get that width.
 */ 
int
agxBytePadScratchMapPow2( w, n )
   int  w;        /* map width in pixels */
   int  n;        /* 2^n == bytes per pixel, may be 0 or even neg */
{
   int pw;   /* pixel width */
   int size; 

   for( pw=0, size=1; size < w; size<<=1, pw++ );  /* padded width in pixels */
   pw += n;    /* adjust to bytes */
   return  pw >= 0 ? pw : 0;
}

/*
 * agxMemToVid()
 * 
 */
void
agxMemToVid( dst, dstWidth, src, srcWidth, h )
   unsigned int  dst;
   unsigned int  dstWidth;
   unsigned char *src;
   unsigned int  srcWidth;
   unsigned int  h;
{
    char *curvm;
    unsigned int offset;
    unsigned int bank;
    unsigned int cpyWidth;
    int left;
    int count;

    offset = dst & (agxBankSize-1);
    bank   = (dst >> 16) & 0x7F;
    agxSetVGAPage(bank);
    curvm = (char*)agxVideoMem + offset;
    cpyWidth = dstWidth < srcWidth ? dstWidth : srcWidth;

    while(h) {
        /*
         * calc number of line before need to switch banks
         */
        count = (agxBankSize - offset) / dstWidth;
        if (count >= h) {
                count = h;
                h = 0;
        }
        else {
           offset += count * dstWidth;
           if (offset + cpyWidth  < agxBankSize) {
              count++;
              offset += dstWidth;
           }
           h -= count;
        }

        /*
         * Output line till back switch
         */
        if( srcWidth == dstWidth ) {
           unsigned int size = count * dstWidth;
           MemToBus( curvm, src, size );
           count = 0;
           curvm += size;
           src += size;
        }
        else {
           while(count--) {
              MemToBus( curvm, src, cpyWidth);
              curvm += dstWidth;
              src += srcWidth;
           }
        }

        if (h) {
           if (offset < agxBankSize) {
                h--;
                left = agxBankSize - offset;
                MemToBus( curvm, src, left);
                bank++;
                agxSetVGAPage(bank);

                MemToBus( agxVideoMem, src+left, dstWidth-left );
                src += srcWidth;
                offset += dstWidth;
            } else {
                bank++;
                agxSetVGAPage(bank);
            }
        offset &= (agxBankSize-1);
        curvm = (char*)agxVideoMem + offset;
        }
    }
}

/*
 * agxPartMemToVid()
 * 
 */
void
agxPartMemToVid( dst, dstWidth, src, srcWidth, w, h )
   unsigned int  dst;
   unsigned int  dstWidth;
   unsigned char *src;
   unsigned int  srcWidth;
   unsigned int  w;
   unsigned int  h;
{
    char *curvm;
    unsigned int offset;
    unsigned int bank;
    int left;
    int count;
    Bool multiCopy = (srcWidth == w) && (dstWidth == w);

    offset = dst & (agxBankSize-1);
    bank   = (dst >> 16) & 0x7F;
    agxSetVGAPage(bank);
    curvm = (char*)agxVideoMem + offset;

    while(h) {
        /*
         * calc number of line before need to switch banks
         */
        count = (agxBankSize - offset) / dstWidth;
        if (count >= h) {
                count = h;
                h = 0;
        }
        else {
           offset += count * dstWidth;
           if (offset + w  < agxBankSize) {
              count++;
              offset += dstWidth;
           }
           h -= count;
        }

        /*
         * Output line till back switch
         */
        if( FALSE && multiCopy ) {
           unsigned int size = count * w;
           MemToBus( curvm, src, size );
           count = 0;
           curvm += size;
           src += size;
        }
        else {
           while(count--) {
              MemToBus( curvm, src, w);
              curvm += dstWidth;
              src += srcWidth;
           }
        }

        if (h) {
           if ( offset < agxBankSize) {
                h--;
                left = agxBankSize - offset;
                if( left < w )
                   MemToBus( curvm, src, left);
                else
                   MemToBus( curvm, src, w);
                bank++;
                agxSetVGAPage(bank);

                if( w > left )
                   MemToBus( agxVideoMem, src+left, w-left );
                src += srcWidth;
                offset += dstWidth;
            } else {
                bank++;
                agxSetVGAPage(bank);
            }
        offset &= (agxBankSize-1);
        curvm = (char*)agxVideoMem + offset;
        }
    }
}

#if 0
static void
agxImageWrite(x, y, w, h, psrc, pwidth, px, py, alu, planemask)
    unsigned int        x;
    unsigned int	y;
    unsigned int	w;
    unsigned int	h;
    unsigned char	*psrc;
    unsigned int	pwidth;
    unsigned int	px;
    unsigned int	py;
    unsigned int        alu;
    unsigned int  	planemask;
{
    pointer curvm;

    if ((w == 0) || (h == 0))
	return;

    if (alu == MIX_DST)
	return;

    GE_WAIT_IDLE();

    if ((alu != MIX_SRC) || ((planemask & PMask) != PMask)) {
#if 0
	vgaImageWrite( agxVideoMem, psrc, 
                       pwidth, agxVirtX,
                       px, py, x, y, w, h, 
                       1, 1, alu, planemask );
#else
        agxImageWriteNoMem(x, y, w, h, psrc, pwidth, px, py, alu, planemask);
#endif
	return;
    }
	
    psrc += pwidth * py + px;
    curvm = (agxVideoMem + x) + y * agxVirtX;

    while(h--) {
	MemToBus(curvm, psrc, w);
	
	curvm += agxVirtX; 
	psrc += pwidth;
    }
}
#endif

static void
agxImageWriteBank(x, y, w, h, psrc, pwidth, px, py, alu, planemask)
    unsigned int	x;
    unsigned int	y;
    unsigned int	w;
    unsigned int	h;
    unsigned char	*psrc;
    unsigned int	pwidth;
    unsigned int	px;
    unsigned int	py;
    unsigned int        alu;
    unsigned int	planemask;
{
    pointer curvm;
    int offset;
    int bank;
    int left;
    int count;

    if ((w == 0) || (h == 0))
	return;

    if (alu == MIX_DST)
	  return;

    GE_WAIT_IDLE();

    if ((alu != MIX_SRC) || ((planemask & PMask) != PMask)) {
#if 0
	vgaImageWrite( agxVideoMem, psrc, 
                       pwidth, agxVirtX,
                       px, py, x, y, w, h, 
                       1, 1, alu, planemask );
#else
        agxImageWriteNoMem(x, y, w, h, psrc, pwidth, px, py, alu, planemask);
#endif
	return;
    }
	
    psrc += (pwidth * py + px) << BytesPerPixelShift;
    offset = (x + y * agxVirtX) << BytesPerPixelShift;
    bank = offset / agxBankSize;
    offset &= (agxBankSize-1);
    curvm = (pointer)&((char*)agxVideoMem)[offset];
    agxSetVGAPage(bank);


    while(h) {
	/*
	 * calc number of line before need to switch banks
	 */
	count = (agxBankSize - offset) / screenStride;
	if (count >= h) {
		count = h;
		h = 0;
	} 
        else {
 	   offset += (count * agxVirtX);
	   if (offset + (w<<BytesPerPixelShift) < agxBankSize) {
	      count++;
	      offset += agxVirtX;
	   }
           h -= count;
	}

	/*
	 * Output line till back switch
	 */
	while(count--) {
		MemToBus(curvm, psrc, w<<BytesPerPixelShift);
		curvm = (void *)((unsigned char *)curvm + agxVirtX);
		psrc += pwidth;
	}

	if (h) {
	   if (offset < agxBankSize) {
		h--;
	        left = agxBankSize - offset;
		MemToBus(curvm, psrc, left);
		bank++;
		agxSetVGAPage(bank);
		
		MemToBus(agxVideoMem, psrc+left, (w<<BytesPerPixelShift)-left);
		psrc += pwidth;
		offset += screenStride;
	    } else {
		bank++;
		agxSetVGAPage(bank);
	    }
	offset &= (agxBankSize-1);
	curvm = (pointer)&((char*)agxVideoMem)[offset];
	}
    }
}


static void
agxImageWriteNoMem(x, y, w, h, psrc, pwidth, alu, planemask)
    unsigned int	x;
    unsigned int	y;
    unsigned int	w;
    unsigned int	h;
    unsigned char	*psrc;
    unsigned int	pwidth;
    unsigned int	alu;
    unsigned int	planemask;
{
    unsigned short      srcPWidth;
    unsigned short      srcBWidth;
    short  		srcBWidthShift;
    unsigned short      srcStripHeight;
    unsigned int        srcMaxLines;
    unsigned short      srcLine;
    unsigned short      numVertStrips;
    unsigned short      lastVStripHeight;
    unsigned short      strip;

    /*
     * Unlike the 8514 clan, the XGA architecture does not support
     * bitblt's with the src being the input port. Instead
     * the src will need to be copied into a scratchpad map in
     * unused videomem and then used as the source of the bitblt.
     * There are a couple of complications:
     *    1) The scratchpad may not be large enough
     *    2) The AGX implementations require that the source map width
     *       be a power of 2 (there are some exceptions that I'm not
     *       exploiting yet). 
     * This results in 2 major cases:
     *    1) Fits in scratchpad.
     *    2) Doesn't fit in scratchpad.
     *       Source split into multiple horizontal strips.
     *   
     * An optimization to be considered in the future, would be 
     * to split the scratchpad into double buffers, and for all cases
     * that are not trivially small, overlap the copies and 
     * the bitblt to maximize concurrency.
     */

    srcBWidthShift = agxBytePadScratchMapPow2( w, BytesPerPixelShift );
    srcBWidth = 1 << srcBWidthShift;
    srcPWidth = srcBWidth >> BytesPerPixelShift;
    srcMaxLines = agxScratchSize >> srcBWidthShift;

    if( h <= srcMaxLines ) {
       numVertStrips = 1 ;
       lastVStripHeight = h ;
       srcStripHeight = h;
    }
    else {
       numVertStrips = h / srcMaxLines;
       lastVStripHeight = h % srcMaxLines;
       srcStripHeight = srcMaxLines;
       if( lastVStripHeight )
          numVertStrips++;
       else
          lastVStripHeight = srcStripHeight;
    }

    MAP_INIT( GE_MS_MAP_B,
              agxVideoMapFormat,
              agxMemBase + agxScratchOffset,
              srcPWidth-1,
              srcStripHeight-1,
              FALSE, FALSE, FALSE );

    GE_WAIT_IDLE();

    MAP_SET_DST( GE_MS_MAP_A );
    MAP_SET_SRC( GE_MS_MAP_B );

#ifdef MAP_MNG
    GE_SET_MAP( GE_MS_MAP_B );
#else
    GE_OUT_B( GE_PIXEL_MAP_SEL, GE_MS_MAP_B );
    GE_OUT_D( GE_PIXEL_MAP_BASE, agxMemBase + agxScratchOffset );
    GE_OUT_W( GE_PIXEL_MAP_WIDTH,  srcPWidth - 1 );
    GE_OUT_W( GE_PIXEL_MAP_HEIGHT, srcStripHeight - 1 );
    GE_OUT_B( GE_PIXEL_MAP_FORMAT, agxVideoMapFormat );
#endif

    GE_OUT_B( GE_FRGD_MIX, alu );
    GE_OUT_D( GE_PIXEL_BIT_MASK, planemask );
#ifndef NO_MULTI_IO
    GE_OUT_D( GE_OP_DIM_WIDTH, (srcStripHeight-1 << 16) | w-1 );
#else
    GE_OUT_W( GE_OP_DIM_HEIGHT, srcStripHeight - 1 );
    GE_OUT_W( GE_OP_DIM_WIDTH, w - 1);
#endif
    GE_OUT_W( GE_PIXEL_OP, 
              GE_OP_PAT_FRGD
              | GE_OP_MASK_DISABLED
              | GE_OP_INC_X
              | GE_OP_INC_Y         );


    srcLine = 0; 

    for( strip = 1; strip <= numVertStrips; strip++ ) {

       GE_WAIT_IDLE(); /* need to double buffer! */

       if( strip == numVertStrips ) {
          srcStripHeight = lastVStripHeight;
          GE_OUT_W( GE_OP_DIM_HEIGHT, srcStripHeight - 1 );
       }
          
       /*
        * Load map B with the current strip. 
        */
       if( numVertStrips == 1 ) { 
          agxMemToVid( agxScratchOffset, srcPWidth,
                       psrc, pwidth,
                       h               ); 
       }
       else {
          agxMemToVid( agxScratchOffset, srcBWidth, 
                       psrc + (srcLine<< srcBWidthShift), pwidth,
                       srcStripHeight );
       }

#ifndef NO_MULTI_IO
       GE_OUT_D( GE_SRC_MAP_X, 0 );
       GE_OUT_D( GE_DEST_MAP_X, (srcLine << 16) | x );
#else
       GE_OUT_W( GE_SRC_MAP_X, 0 );
       GE_OUT_W( GE_SRC_MAP_Y, 0 );
       GE_OUT_W( GE_DEST_MAP_X, x );
       GE_OUT_W( GE_DEST_MAP_Y, srcLine );
#endif
       GE_START_CMDW( GE_OPW_BITBLT
                      | GE_OPW_FRGD_SRC_MAP
                      | GE_OPW_SRC_MAP_B
                      | GE_OPW_DEST_MAP_A   );

       srcLine += srcStripHeight;
    }
    GE_WAIT_IDLE_EXIT();
} 

#if 0
static void
agxImageRead(x, y, w, h, psrc, pwidth, px, py, planemask)
    unsigned int	x;
    unsigned int	y;
    unsigned int	w;
    unsigned int	h;
    unsigned char	*psrc;
    unsigned int	pwidth;
    unsigned int	px;
    unsigned int	py;
    unsigned int	planemask;
{
    int j;
    pointer curvm;

    if ((w == 0) || (h == 0))
	return;

    GE_WAIT_IDLE();

    if ((planemask & PMask) != PMask) {
	return;
    }

    psrc += pwidth * py + px;
    curvm = agxVideoMem + x;
    
    for (j = y; j < y+h; j++) {
	BusToMem(psrc, curvm + j*agxVirtX, w);
	psrc += pwidth;
    }
}
#endif

static void
agxImageReadBank(x, y, w, h, psrc, pwidth, px, py, planemask)
    unsigned int	x;
    unsigned int	y;
    unsigned int	w;
    unsigned int	h;
    unsigned char	*psrc;
    unsigned int	pwidth;
    unsigned int	px;
    unsigned int	py;
    unsigned int	planemask;
{
    pointer curvm;
    int offset;
    int bank;
    int left;

    if ((w == 0) || (h == 0))
	return;

    GE_WAIT_IDLE();

#if 0
    if ((planemask & 0xff) != 0xff) {
        /* ??? */
    }
#endif

    psrc += pwidth * py + (px << BytesPerPixelShift);
    offset = x + (y << BytesPerPixelShift) * screenStride;
    bank = offset / agxBankSize;
    offset &= (agxBankSize-1);
    curvm = (pointer)&((char*)agxVideoMem)[offset];
    agxSetVGAPage(bank);

    while(h--) {
	if (offset + w > agxBankSize) {
	    if (offset < agxBankSize) {
		left = agxBankSize - offset;
		BusToMem(psrc, curvm, left);
		bank++;
		agxSetVGAPage(bank);
		
		BusToMem( psrc+left, 
                          agxVideoMem, 
                          (w << BytesPerPixelShift) - left );
		psrc += pwidth;
		offset = (offset + screenStride) & (agxBankSize-1);
		curvm = (pointer)&((char*)agxVideoMem)[offset];
	    } else {
		bank++;
		agxSetVGAPage(bank);
		offset &= (agxBankSize-1);
		curvm = (pointer)&((char*)agxVideoMem)[offset];
		BusToMem(psrc, curvm, (w << BytesPerPixelShift));
		offset += screenStride;
		curvm = (void *)((unsigned char *)curvm + screenStride);
		psrc += pwidth;
	    }
	} else {
	    BusToMem(psrc, curvm, (w << BytesPerPixelShift) );
	    offset += screenStride;
	    curvm = (void *)((unsigned char *)curvm + screenStride);
	    psrc += pwidth;
	}
    }
}

#if 0
static void
agxImageFill(x, y, w, h, psrc, pwidth, pw, ph, pox, poy, alu, planemask)
    int			x;
    int			y;
    int			w;
    int			h;
    unsigned char	*psrc;
    int			pwidth;
    int			pw;
    int			ph;
    int			pox;
    int			poy;
    unsigned int	alu;
    unsigned int	planemask;
{
    unsigned int 	i,j;
    unsigned char	*pline;
    int                 mod, ymod;
    unsigned char       *curvm;
    int                 count;

    if ((w == 0) || (h == 0))
	return;

    if (alu == MIX_DST)
	  return;

    if ((alu != MIX_SRC) || ((planemask&PMask) != PMask))
    {
	agxImageFillNoMem(x, y, w, h, psrc, pwidth, pw, ph, pox,
			     poy, alu, planemask);
	return;
    }

    GE_WAIT_IDLE();
    modulus(y-poy,ph,ymod);

    for (j = y; j < y+h; j++) {
	curvm = agxVideoMem + x + j*agxVirtX;
	
	pline = psrc + pwidth*ymod;
	if (++ymod >= ph)
	    ymod -= ph;
	modulus(x-pox,pw,mod);
	for (i = 0, count = 0; i < w; i += count ) {
	    count = pw - mod;
	    if (i + count > w)
		count = w - i;
	    MemToBus( curvm, pline + mod, count);
	    curvm += count;
	    mod += count;
	    while(mod >= pw)
		mod -= pw;
	}
    }
}
#endif

static void
agxImageFillBank(x, y, w, h, psrc, pwidth, pw, ph, pox, poy, alu, planemask)
    int                 x;
    int                 y;
    int                 w;
    int                 h;
    unsigned char       *psrc;
    int                 pwidth;
    int                 pw;
    int                 ph;
    int                 pox;
    int                 poy;
    int                 alu;
    int                 planemask;
{
    int			xrot;
    int			yrot;

    if ((w == 0) || (h == 0))
        return;

    if (alu == MIX_DST)
        return;

    modulus( x-pox, pw, xrot );
    modulus( y-poy, ph, yrot );

    if ( (alu != MIX_SRC) 
         || ((planemask&PMask) != PMask)
         || w + xrot > pw
         || h + yrot > ph ) {
        agxImageFillNoMem(x, y, w, h, psrc, pwidth, pw, ph, pox,
                             poy, alu, planemask);
    }
    else {

       GE_WAIT_IDLE();
   
       agxPartMemToVid( (x + y * agxVirtX) << BytesPerPixelShift, 
                        agxVirtX,
                        psrc + ((xrot + yrot * pwidth) << BytesPerPixelShift),
                        pwidth,
                        w << BytesPerPixelShift, h );
    }
}

static void
agxImageFillNoMem(x, y, w, h, psrc, pwidth, pw, ph, pox, poy, alu, planemask)
    int			x;
    int			y;
    int			w;
    int			h;
    unsigned char	*psrc;
    int			pwidth;
    int			pw;
    int			ph;
    int			pox;
    int			poy;
    unsigned int	alu;
    unsigned int 	planemask;
{
    unsigned short      srcPWidth;
    unsigned short      srcBWidth;
    short  		srcBWidthShift;
    unsigned short      srcStripHeight;
    unsigned int        srcMaxLines;
    unsigned short      srcLine;
    unsigned short      numHorizTiles;
    unsigned short      firstHTileWidth = 0;
    unsigned short      lastHTileWidth = 0;
    unsigned short      firstBWidth = 0;
    unsigned short      lastBWidth = 0;
    unsigned short      numVertTiles;
    unsigned int        lastVTileHeight;
    unsigned short      numLastVTileStrips;
    unsigned int        lastVTileLastStripHeight;
    unsigned short      numVertStrips;
    unsigned int        lastVStripHeight;
    unsigned short      strip;
    unsigned short      vStripFirstY;         
    int			xrot;
    int			yrot;
    unsigned short      newWidth;
    unsigned short      firstHeight;
    unsigned short      secondHeight;
    unsigned short      vTile;
    unsigned short      dstY;         

    modulus( x-pox, pw, xrot );
    modulus( y-poy, ph, yrot );

    /*
     * Unlike the 8514 clan, the XGA architecture does not support
     * bitblt's with the src being the input port. Instead
     * the src will need to be copied into a scratchpad map in
     * unused videomem and then used as the source of the bitblt.
     * There are a couple of complications:
     *    1) The scratchpad may not be large enough
     *    2) The AGX implementations require that the source map width
     *       be a power of 2. This means the bitblt'er can only
     *       do tiling directly if the source width is a power of two.
     * This results in 4 major cases:
     *    1) Fits in scratchpad and it's width is a power of 2.
     *       The bitblt can be done directly including any tiling.
     *    2) Fits in scratchpad and it's width is not a power of 2.
     *       Any horizontal tiling will have to be done with multiple
     *       bitblt's.
     *    3) Doesn't fit in scratchpad and it's width is a power of 2.
     *       Source split into multiple horizontal strips. The bitblt'r 
     *       does horizontal tiling but vertical tiling is done
     *       with multiple Bitblt's. 
     *    4) Doesn't fit in scratchpad and it's width is not a power of 2.
     *       Source split into multiple horizontal strips, The bitblt'r
     *       does no tiling.
     *
     * This is further complicated by the pox and poy offsets that
     * need to be considered in the cases that the bitblt'r is unable
     * to handle tiling on its own.
     *
     * An optimization to be considered in the future, would be 
     * to split the scratchpad into two buffers, and for all cases
     * that are not trivially small, or for which hardware tiling cannot
     * be explioted in both directions, overlap the copies and 
     * the bitblt to maximize concurrency (whether the vidoe memory
     * has the bandwidh to do this without an overall loss in performance
     * has not been tested).
     *
     * Another optimization would be to seperate-out common cases that
     * don't requires SW tiling so they don't incur the computational 
     * and loop overhead of this routine.
     */

    newWidth = w > pw ? pw : w;
    srcBWidthShift = agxBytePadScratchMapPow2( newWidth, BytesPerPixelShift );
    srcBWidth = 1 << srcBWidthShift;
    srcPWidth = srcBWidth >> BytesPerPixelShift;
    srcMaxLines = agxScratchSize >> srcBWidthShift; 

    if( w == newWidth ) {
       firstHTileWidth = w; 
       lastHTileWidth = 0; 
       numHorizTiles = 1;
    }
    else if( pw == srcPWidth ) {
       firstHTileWidth = w; 
       lastHTileWidth = 0;
       numHorizTiles = 1;
    }
    else {
       firstHTileWidth = pw;
       numHorizTiles  = w / pw;
       lastHTileWidth = w % pw; 
       if( lastHTileWidth )
          numHorizTiles++;
    } 

    if( xrot == 0 ) {
       firstBWidth = 0;
       lastBWidth = w << BytesPerPixelShift;
    }
    else { 
       int right = pw - xrot;
       if( w < ph ) {
          if( right < w ) {
             firstBWidth = right << BytesPerPixelShift;
             lastBWidth = (w-right) << BytesPerPixelShift;
          }
          else {
             firstBWidth = w << BytesPerPixelShift;
             lastBWidth = 0; 
          }
       }
       else {
          firstBWidth = right << BytesPerPixelShift;
          lastBWidth = xrot << BytesPerPixelShift; 
       }
    }
    
    if( h < ph 
        && h <= srcMaxLines ) {
       numVertStrips = 1 ;
       lastVStripHeight = h ;
       srcStripHeight = h;
    }
    else if( ph <= srcMaxLines ) {
       numVertStrips = 1 ;
       lastVStripHeight = ph ;
       srcStripHeight = ph;
    }
    else {
       numVertStrips = ph / srcMaxLines;
       lastVStripHeight = ph % srcMaxLines;
       srcStripHeight = srcMaxLines;
       if( lastVStripHeight )
          numVertStrips++;
       else
          lastVStripHeight = srcStripHeight;
    }

    if( numVertStrips == 1 ) { 
       numVertTiles = 1;
       numLastVTileStrips = 1;
       lastVTileLastStripHeight = h;
    }
    else { 
       numVertTiles = h / ph;
       lastVTileHeight = h % ph;
       if( lastVTileHeight ) 
           numVertTiles++;
       else
          lastVTileHeight = ph; 

       numLastVTileStrips = lastVTileHeight / srcStripHeight; 
       lastVTileLastStripHeight = lastVTileHeight % srcStripHeight; 
       if( lastVTileLastStripHeight )
          numLastVTileStrips++;
       else
          lastVTileLastStripHeight = srcStripHeight;
    }

    MAP_INIT( GE_MS_MAP_B,
              agxVideoMapFormat,
              agxMemBase + agxScratchOffset,
              srcPWidth-1,
              srcStripHeight-1,
              FALSE, FALSE, FALSE );
   
    GE_WAIT_IDLE();

    MAP_SET_DST( GE_MS_MAP_A );
    MAP_SET_SRC( GE_MS_MAP_B );

#ifdef MAP_MNG
    GE_SET_MAP( GE_MS_MAP_B );
#else
    GE_OUT_B( GE_PIXEL_MAP_SEL, GE_MS_MAP_B );
    GE_OUT_D( GE_PIXEL_MAP_BASE, agxMemBase + agxScratchOffset );
    GE_OUT_W( GE_PIXEL_MAP_WIDTH,  srcPWidth - 1 );
    GE_OUT_W( GE_PIXEL_MAP_HEIGHT, srcStripHeight - 1 );
    GE_OUT_B( GE_PIXEL_MAP_FORMAT, agxVideoMapFormat );
#endif

    GE_OUT_B( GE_FRGD_MIX, alu );
    GE_OUT_D( GE_PIXEL_BIT_MASK, planemask );
    GE_OUT_W( GE_PIXEL_OP, 
              GE_OP_PAT_FRGD
              | GE_OP_MASK_DISABLED
              | GE_OP_INC_X
              | GE_OP_INC_Y         );

    srcLine = yrot; 
    vStripFirstY = y;

    for( strip = 1; strip <= numVertStrips; strip++ ) {

       dstY = vStripFirstY;

       GE_WAIT_IDLE();

       /*
        * Load map B with the current strip. 
        */
       firstHeight = ph - srcLine;
       if( firstHeight <= srcStripHeight && firstHeight > (unsigned)h ) {
          firstHeight = h; 
          secondHeight = 0;
       }
       else if( firstHeight < srcStripHeight ) {
          secondHeight = srcStripHeight - firstHeight;
       }
       else {
          firstHeight = srcStripHeight;
          secondHeight = 0;
       }

       agxPartMemToVid( agxScratchOffset + firstBWidth, srcBWidth, 
                        psrc + srcLine * pwidth, pwidth,
                        lastBWidth, firstHeight );
       if( firstBWidth > 0 ) {
          agxPartMemToVid( agxScratchOffset, srcBWidth, 
                           psrc + srcLine * pwidth + xrot, pwidth,
                           firstBWidth, firstHeight );
       }
     
       if( secondHeight ) {
          agxPartMemToVid( agxScratchOffset + firstHeight*srcBWidth 
                             + firstBWidth, srcBWidth,
                            psrc, pwidth,
                            lastBWidth, secondHeight );
          if( firstBWidth > 0 ) {
             agxPartMemToVid( agxScratchOffset + firstHeight*srcBWidth,
                              srcBWidth,
                              psrc + xrot, pwidth,
                              firstBWidth, secondHeight );
          }
       }

       if( strip == numVertStrips ) {
          srcStripHeight = lastVStripHeight;
          GE_OUT_B( GE_PIXEL_MAP_SEL, GE_MS_MAP_B );
          GE_OUT_W( GE_PIXEL_MAP_HEIGHT, srcStripHeight - 1 );
       }
          
       for( vTile = 1; vTile <= numVertTiles; vTile++ ) {
          unsigned short hTile;
          unsigned short dstX;
          unsigned short dstHeight;

          if( vTile == numVertTiles )
             if( strip < numLastVTileStrips )
                dstHeight = srcStripHeight - 1;
             else if( strip == numLastVTileStrips )
                dstHeight = lastVTileLastStripHeight - 1;
             else
                continue;
          else
             dstHeight = srcStripHeight - 1;

          dstX = x;

          GE_WAIT_IDLE();
          GE_OUT_W( GE_OP_DIM_HEIGHT, dstHeight );

          for( hTile = 1; hTile <= numHorizTiles; hTile++ ) {
             unsigned short dstWidth;

             if( hTile == 1 ) {
                dstWidth = firstHTileWidth - 1;
             }
             else if( hTile == numHorizTiles ) {
                dstWidth = lastHTileWidth - 1;
             }
             else {
                dstWidth = pw - 1; 
             }
      
            GE_WAIT_IDLE();
#ifndef NO_MULTI_IO
            GE_OUT_D( GE_SRC_MAP_X,  0 );
            GE_OUT_D( GE_DEST_MAP_X, (dstY << 16) | dstX );
#else
            GE_OUT_D( GE_SRC_MAP_X, 0 );
            GE_OUT_W( GE_DEST_MAP_X, dstX );
            GE_OUT_W( GE_DEST_MAP_Y, dstY );
#endif
            GE_OUT_W( GE_OP_DIM_WIDTH, dstWidth );
            GE_START_CMDW( GE_OPW_BITBLT
                           | GE_OPW_FRGD_SRC_MAP
                           | GE_OPW_SRC_MAP_B
                           | GE_OPW_DEST_MAP_A   );
            dstX += dstWidth + 1;    
          }
          dstY += ph;
       }
       srcLine += srcStripHeight;
       if( srcLine > (unsigned)ph )
          srcLine -= ph; 
       vStripFirstY += srcStripHeight;
    }
    GE_WAIT_IDLE_EXIT();
}


void
agxImageStipple( x, y, w, h, psrc, pwidth, pw, ph, pox, poy, 
                 fgPixel, bgPixel, fgAlu, bgAlu, planemask)
    int			x;
    int			y;
    int			w;
    int			h;
    unsigned char	*psrc;
    int			pwidth;
    int			pw;
    int			ph;
    int			pox;
    int			poy;
    unsigned int	fgPixel;
    unsigned int	bgPixel;
    unsigned int	fgAlu;
    unsigned int	bgAlu;
    unsigned int	planemask;
{
    unsigned short      srcPWidth = 0;
    unsigned short      srcBWidth;
    short  		srcBWidthShift;
    unsigned short      srcStripHeight;
    unsigned int        srcMaxLines;
    unsigned short      srcLine;
    unsigned short      numHorizTiles;
    unsigned short      firstHTileWidth = 0;
    unsigned short      lastHTileWidth;
    unsigned short      firstBWidth;
    unsigned short      lastBWidth;
    unsigned short      numLastVTileStrips;
    unsigned short      lastVTileLastStripHeight;
    unsigned short      numVertTiles;
    unsigned short      lastVTileHeight;
    unsigned short      numVertStrips;
    unsigned short      lastVStripHeight;
    unsigned short      strip;
    unsigned short      vStripFirstY; 
    Bool                autoHTiling;
    int			xrot;
    int			yrot;
    int                 xOff;
    unsigned short      vTile;
    unsigned short      dstY;
    unsigned short      firstHeight;
    unsigned short      secondHeight;
    unsigned short      firstWidth;
    unsigned short      remWidth;

    modulus( x-pox, pw, xrot );
    modulus( y-poy, ph, yrot );

    /*
     * Unlike the 8514 clan, the XGA architecture does not support
     * bitblt's with the src being the input port. Instead
     * the src will need to be copied into a scratchpad map in
     * unused videomem and then used as the source of the bitblt.
     * There are a couple of complications:
     *    1) The scratchpad may not be large enough
     *    2) The AGX implementations require that the source map width
     *       be a power of 2. This means the bitblt'er can only
     *       do tiling directly if the source width is a power of two.
     * This results in 4 major cases:
     *    1) Fits in scratchpad and it's width is a power of 2.
     *       The bitblt can be done directly including any tiling.
     *    2) Fits in scratchpad and it's width is not a power of 2.
     *       Any horizontal tiling will have to be done with multiple
     *       bitblt's.
     *    3) Doesn't fit in scratchpad and it's width is a power of 2.
     *       Source split into multiple horizontal strips. The bitblt'r 
     *       does horizontal tiling but vertical tiling is done
     *       with multiple Bitblt's. 
     *    4) Doesn't fit in scratchpad and it's width is not a power of 2.
     *       Source split into multiple horizontal strips, The bitblt'r
     *       does no tiling.
     *
     * This is further complicated by the pox and poy offsets that
     * need to be considered in the cases that the bitblt'r is unable
     * to handle tiling on its own.
     *
     * An optimization to be considered in the future, would be 
     * to split the scratchpad into two buffers, and for all cases
     * that are not trivially small, overlap the copies and 
     * the bitblt to maximize concurrency.
     */

    if( pw == srcPWidth && pw < w ) {
       numHorizTiles = 0;
       lastHTileWidth = pw;
       autoHTiling = TRUE;
    }
    else {
       firstWidth = pw - xrot;
       firstHTileWidth = firstWidth <= (unsigned)w ? firstWidth : w; 
       remWidth = w - firstHTileWidth;
       numHorizTiles  = remWidth / (unsigned)pw;
       lastHTileWidth = remWidth % (unsigned)pw;
       if( firstHTileWidth )
          numHorizTiles++;
       if( lastHTileWidth )
          numHorizTiles++;
       else
          lastHTileWidth = pw;
       autoHTiling = FALSE;
    } 
    firstBWidth = firstHTileWidth >> 3;
    lastBWidth = lastHTileWidth >> 3;
    xOff = xrot >> 3;

    srcBWidthShift = agxBytePadScratchMapPow2( pw, -3 );
    srcBWidth = 1 << srcBWidthShift;
    srcPWidth = srcBWidth << 3;
    srcMaxLines = agxScratchSize >> srcBWidthShift;

    if( ph <= srcMaxLines ) {
       numVertStrips = 1 ;
       lastVStripHeight = ph ;
       srcStripHeight = ph;
    }
    else {
       numVertStrips = ph / srcMaxLines;
       lastVStripHeight = ph % srcMaxLines;
       srcStripHeight = srcMaxLines;
       if( lastVStripHeight )
          numVertStrips++;
       else
          lastVStripHeight = srcStripHeight;
    }
 
    if( numVertStrips == 1 ) {
       numVertTiles = 1;
       numLastVTileStrips = 1;
       lastVTileLastStripHeight = h;
    }
    else {
       numVertTiles = h / ph;
       lastVTileHeight = h % ph;
       if( lastVTileHeight )
           numVertTiles++;
       else
          lastVTileHeight = ph;

       numLastVTileStrips = lastVTileHeight / srcStripHeight;
       lastVTileLastStripHeight = lastVTileHeight % srcStripHeight;
       if( lastVTileLastStripHeight )
          numLastVTileStrips++;
       else
          lastVTileLastStripHeight = srcStripHeight;
    }

    MAP_INIT( GE_MS_MAP_B, 
              GE_MF_1BPP | GE_MF_MOTO_FORMAT,
              agxMemBase + agxScratchOffset, 
              srcPWidth-1, 
              srcStripHeight-1,
              FALSE, FALSE, FALSE );
   
    GE_WAIT_IDLE();

    MAP_SET_DST( GE_MS_MAP_A );
    MAP_SET_SRC( GE_MS_MAP_B );

#ifdef MAP_MNG
    GE_SET_MAP( GE_MS_MAP_B );   
#else
    GE_OUT_B( GE_PIXEL_MAP_SEL, GE_MS_MAP_B );
    GE_OUT_D( GE_PIXEL_MAP_BASE, agxMemBase + agxScratchOffset );
    GE_OUT_W( GE_PIXEL_MAP_WIDTH,  srcPWidth-1 );
    GE_OUT_W( GE_PIXEL_MAP_HEIGHT, srcStripHeight-1 );
    GE_OUT_B( GE_PIXEL_MAP_FORMAT, GE_MF_1BPP | GE_MF_MOTO_FORMAT );
#endif

#ifndef NO_MULTI_IO
    GE_OUT_W(GE_FRGD_MIX, (bgAlu << 8) | fgAlu );  /* both fg & bg */
#else
    GE_OUT_B(GE_FRGD_MIX, fgAlu );
    GE_OUT_B(GE_BKGD_MIX, bgAlu );
#endif
    GE_OUT_D(GE_FRGD_CLR, fgPixel );
    GE_OUT_D(GE_BKGD_CLR, bgPixel );
    GE_OUT_D(GE_PIXEL_BIT_MASK, planemask);
    GE_OUT_W( GE_PIXEL_OP,
              GE_OP_PAT_MAP_B
              | GE_OP_MASK_DISABLED
              | GE_OP_INC_X
              | GE_OP_INC_Y         );

    srcLine = yrot; 
    vStripFirstY = y;

    for( strip = 1; strip <= numVertStrips; strip++ ) {

       dstY = vStripFirstY;

       GE_WAIT_IDLE();

       if( strip == numVertStrips ) {
          srcStripHeight = lastVStripHeight;
          GE_OUT_B( GE_PIXEL_MAP_SEL, GE_MS_MAP_B );
          GE_OUT_W( GE_PIXEL_MAP_HEIGHT, srcStripHeight-1 );
       }

          
       /*
        * Load map B with the current strip. 
        */
       if( autoHTiling && numVertStrips == 1 ) { 
          agxMemToVid( agxScratchOffset, srcBWidth,
                       psrc, pwidth,
                       ph               ); 
       }
       else
       {
          firstHeight = ph - srcLine;
          secondHeight =  0;

          if( firstHeight <= srcStripHeight && firstHeight > (unsigned)h ) {
             firstHeight = h;
             secondHeight = 0;
          }
          else if( firstHeight < srcStripHeight )
             secondHeight = srcStripHeight - firstHeight;
          else
             firstHeight = srcStripHeight;
          if( firstHTileWidth > 0  && numHorizTiles == 0 ) {
             /* load only what we need */
             agxPartMemToVid( agxScratchOffset + xOff, srcBWidth, 
                              psrc + srcLine*pwidth + xOff, pwidth,
                              firstBWidth, firstHeight );
             if( lastHTileWidth > 0 ) {
                agxPartMemToVid( agxScratchOffset, srcBWidth, 
                                 psrc + srcLine*pwidth, pwidth,
                                 lastBWidth, firstHeight );
             }
          }
          else {
             agxMemToVid( agxScratchOffset, srcBWidth, 
                          psrc + srcLine*pwidth, pwidth,
                          firstHeight );
          }
          if( secondHeight ) {
             if( firstHTileWidth > 0  && numHorizTiles == 0 ) {
                /* load only what we need */
                agxPartMemToVid( agxScratchOffset+firstHeight*srcBWidth+xOff,
                                    srcBWidth, 
                                 psrc + srcLine*pwidth + xOff, pwidth,
                                 firstBWidth, firstHeight );
                if( lastHTileWidth > 0 ) {
                   agxPartMemToVid( agxScratchOffset + firstHeight*srcBWidth,
                                       srcBWidth, 
                                    psrc + srcLine*pwidth, pwidth,
                                    lastBWidth, firstHeight );
                }
             }
             else {
                agxMemToVid( agxScratchOffset + firstHeight*srcBWidth, 
                                 srcBWidth,
                                 psrc, pwidth,
                                 secondHeight );
             }
          }
       }

       for( vTile = 1; vTile <= numVertTiles; vTile++ ) {
          unsigned short hTile;
          unsigned short dstX;
          unsigned short srcX, srcY;
          unsigned short dstHeight;

          if( vTile == numVertTiles )
             if( strip < numLastVTileStrips )
                dstHeight = srcStripHeight - 1;
             else if( strip == numLastVTileStrips )
                dstHeight = lastVTileLastStripHeight - 1;
             else  
                continue;  
          else
             dstHeight = srcStripHeight - 1;

          dstX = x;

          GE_WAIT_IDLE();
          GE_OUT_W( GE_OP_DIM_HEIGHT, dstHeight );

          for( hTile = 1; hTile <= numHorizTiles; hTile++ ) {
             unsigned short dstWidth;

            if( autoHTiling ) {
               dstWidth = w - 1;
               srcX = xrot;
               if( numVertStrips == 1 )
                  srcY = yrot;
               else
                  srcY = 0;
            }
            else {
                if( hTile == 1 ) {
                   dstWidth = firstHTileWidth - 1;
                   srcX = xrot;
                   srcY = 0;
                }
                else if( hTile == numHorizTiles ) {
                   dstWidth = lastHTileWidth - 1;
                   srcX = 0;
                   srcY = 0;
                }
                else {
                   dstWidth = pw - 1; 
                   srcX = 0;
                   srcY = 0;
                }
            }
      
            GE_WAIT_IDLE();
#ifndef NO_MULTI_IO

            GE_OUT_D( GE_PAT_MAP_X,  (srcY << 16) | srcX );
            GE_OUT_D( GE_DEST_MAP_X, (dstY << 16) | dstX );
#else
            GE_OUT_W( GE_PAT_MAP_X, srcX );
            GE_OUT_W( GE_PAT_MAP_Y, srcY );
            GE_OUT_W( GE_DEST_MAP_X, dstX );
            GE_OUT_W( GE_DEST_MAP_Y, dstY );
#endif
            GE_OUT_W( GE_OP_DIM_WIDTH, dstWidth );
            GE_START_CMDW( GE_OPW_BITBLT
                           | GE_OPW_FRGD_SRC_CLR
                           | GE_OPW_BKGD_SRC_CLR
                           | GE_OPW_DEST_MAP_A   );
            dstX += dstWidth + 1;    
          }
          dstY += ph; 
       }
       srcLine += srcStripHeight;
       if( srcLine > (unsigned)ph )
       srcLine -= ph; 
       vStripFirstY += srcStripHeight;
    }
    GE_WAIT_IDLE_EXIT();
}


void
agxFillBoxStipple( pDrawable, nBox, pBox, 
                   stipple, pox, poy,
                   fgpixel, bgpixel, 
                   fgAlu, bgAlu,
                   planemask )
    DrawablePtr     pDrawable;
    int             nBox;
    BoxPtr          pBox;
    PixmapPtr       stipple;
    int             pox;
    int             poy;
    unsigned int    fgpixel;
    unsigned int    bgpixel;
    unsigned int    fgAlu;
    unsigned int    bgAlu;
    unsigned int    planemask;
{
    short          w, h;
    unsigned int   srcX, srcY;
    unsigned int   pixBWidth;
    unsigned short srcPWidth;
    unsigned short srcBWidth;
    short          srcBWidthShift;
    unsigned int   srcMaxLines;
    unsigned int   width = stipple->drawable.width;
    unsigned int   height = stipple->drawable.height;
    
    srcBWidthShift = agxBytePadScratchMapPow2( width, -3 );
    srcBWidth = 1 << srcBWidthShift;
    srcPWidth = srcBWidth << 3;
    srcMaxLines = agxScratchSize >> srcBWidthShift;
    pixBWidth = BitmapBytePad( width );

    if( (width != srcPWidth && width != 2 && width !=4)
        || width * height > 64 * 64
        || srcMaxLines < height ) { 

       for (; nBox; nBox--, pBox++) {
          h = pBox->y2 - pBox->y1 - 1;
          w = pBox->x2 - pBox->x1 - 1;

          if ((w >= 0) && (h >= 0)) {
             agxImageStipple( pBox->x1, pBox->y1,
                              pBox->x2 - pBox->x1,
                              pBox->y2 - pBox->y1,
                              stipple->devPrivate.ptr, pixBWidth,
                              width, height, 
                              pox, poy,
                              fgpixel, bgpixel,
                              fgAlu, bgAlu,
                              planemask );
          }
       }
    }
    else {
       unsigned int widthMask = width - 1;
       unsigned int bwidth;

       bwidth = width >> 3;
       if( bwidth == 0 )
          bwidth = 1;

       MAP_INIT( GE_MS_MAP_B,
                 agxVideoMapFormat,
                 agxMemBase + agxScratchOffset,
                 srcPWidth-1,
                 height-1,
                 FALSE, FALSE, FALSE );

       GE_WAIT_IDLE();

       agxMemToVid( agxScratchOffset, srcBWidth,
                    stipple->devPrivate.ptr, 
                    pixBWidth, height );
 
       MAP_SET_DST( GE_MS_MAP_A );
       MAP_SET_SRC( GE_MS_MAP_B );
       GE_SET_MAP( GE_MS_MAP_B );
 
#ifndef NO_MULTI_IO
       GE_OUT_W(GE_FRGD_MIX, bgAlu << 8) | fgAlu );  /* both fg & bg */
#else
       GE_OUT_B(GE_FRGD_MIX, fgAlu );
       GE_OUT_B(GE_BKGD_MIX, bgAlu );
#endif
       GE_OUT_D(GE_FRGD_CLR, fgpixel );
       GE_OUT_D(GE_BKGD_CLR, bgpixel );
       GE_OUT_D(GE_PIXEL_BIT_MASK, planemask);

 
       GE_OUT_W( GE_PIXEL_OP,
                 GE_OP_PAT_MAP_B
                 | GE_OP_MASK_DISABLED
                 | GE_OP_INC_X
                 | GE_OP_INC_Y         );
 
       for (; nBox; nBox--, pBox++) {
          h = pBox->y2 - pBox->y1 - 1;
          w = pBox->x2 - pBox->x1 - 1;
 
          if ((w >= 0) && (h >= 0)) {
             srcX = (pBox->x1 - pox) & widthMask;
             srcY = (pBox->y1 - poy) % height;
 
             GE_WAIT_IDLE();
#ifndef NO_MULTI_IO
             GE_OUT_D( GE_PAT_MAP_X,  (srcY << 16) | srcX );
             GE_OUT_D( GE_DEST_MAP_X, pBox->y1 << 16
                                      | pBox->x1 );
             GE_OUT_D( GE_OP_DIM_WIDTH, h << 16 | w );
#else
             GE_OUT_W( GE_PAT_MAP_X, srcX );
             GE_OUT_W( GE_PAT_MAP_Y, srcY );
             GE_OUT_W( GE_DEST_MAP_X, (short)(pBox->x1) );
             GE_OUT_W( GE_DEST_MAP_Y, (short)(pBox->y1) );
             GE_OUT_W( GE_OP_DIM_WIDTH, w );
             GE_OUT_W( GE_OP_DIM_HEIGHT, h );
#endif
             GE_START_CMDW( GE_OPW_BITBLT
                            | GE_OPW_FRGD_SRC_CLR
                            | GE_OPW_BKGD_SRC_CLR
                            | GE_OPW_DEST_MAP_A   );
          }
      }
      GE_WAIT_IDLE_EXIT();
   }
}
 

void
agxFillBoxTile( pDrawable, nBox, pBox, tile, pox, poy, alu, planemask )
    DrawablePtr     pDrawable;
    int             nBox;
    BoxPtr          pBox;
    PixmapPtr       tile;
    int             pox;
    int             poy;
    unsigned int    alu;
    unsigned int    planemask;
{
    short          w, h;
    unsigned int   width, height;
    unsigned int   srcX, srcY;
    unsigned int   pixBWidth;
    unsigned short srcPWidth;
    unsigned short srcBWidth;
    short          srcBWidthShift;
    unsigned int   srcMaxLines;

    width = tile->drawable.width;
    height = tile->drawable.height;
    
    srcBWidthShift = agxBytePadScratchMapPow2( width, BytesPerPixelShift );
    srcBWidth = 1 << srcBWidthShift;
    srcPWidth = srcBWidth >> BytesPerPixelShift;
    srcMaxLines = agxScratchSize >> srcBWidthShift;
    pixBWidth = PixmapBytePad( width, tile->drawable.depth );
 
    if( srcPWidth != width
        || width * height > 64 * 64
        || srcMaxLines < height ) { 

       for (; nBox; nBox--, pBox++) {
          h = pBox->y2 - pBox->y1 - 1;
          w = pBox->x2 - pBox->x1 - 1;

          if ((w >= 0) && (h >= 0)) {
              (*agxImageFillFunc)( pBox->x1, pBox->y1,
                                   pBox->x2 - pBox->x1,
                                   pBox->y2 - pBox->y1,
                                   tile->devPrivate.ptr, pixBWidth,
                                   width, height, 
                                   pox, poy,
                                   alu, planemask );
          }
       }
    }
    else {
       unsigned int widthMask = width - 1;

       MAP_INIT( GE_MS_MAP_B,
                 agxVideoMapFormat,
                 agxMemBase + agxScratchOffset,
                 srcPWidth-1,
                 height-1,
                 FALSE, FALSE, FALSE );

       GE_WAIT_IDLE();

       agxMemToVid( agxScratchOffset, srcBWidth,
                       tile->devPrivate.ptr, pixBWidth, height );
 
       MAP_SET_DST( GE_MS_MAP_A );
       MAP_SET_SRC( GE_MS_MAP_B );
       GE_SET_MAP( GE_MS_MAP_B );
 
       GE_OUT_B( GE_FRGD_MIX, alu );
       GE_OUT_D( GE_PIXEL_BIT_MASK, planemask );
 
       GE_OUT_W( GE_PIXEL_OP,
                 GE_OP_PAT_FRGD
                 | GE_OP_MASK_DISABLED
                 | GE_OP_INC_X
                 | GE_OP_INC_Y         );
 
       for (; nBox; nBox--, pBox++) {
          h = pBox->y2 - pBox->y1 - 1;
          w = pBox->x2 - pBox->x1 - 1;
 
          if ((w >= 0) && (h >= 0)) {
             srcX = (pBox->x1 - pox) & widthMask;
             srcY = (pBox->y1 - poy) % height;
 
             GE_WAIT_IDLE();
#ifndef NO_MULTI_IO
             GE_OUT_D( GE_SRC_MAP_X,  (srcY << 16) | srcX );
             GE_OUT_D( GE_DEST_MAP_X, pBox->y1 << 16 | pBox->x1 );
             GE_OUT_D( GE_OP_DIM_WIDTH, h << 16 | w );
#else
             GE_OUT_W( GE_SRC_MAP_X, srcX );
             GE_OUT_W( GE_SRC_MAP_Y, srcY );
             GE_OUT_W( GE_DEST_MAP_X, (short)(pBox->x1) );
             GE_OUT_W( GE_DEST_MAP_Y, (short)(pBox->y1) );
             GE_OUT_W( GE_OP_DIM_WIDTH, w );
             GE_OUT_W( GE_OP_DIM_HEIGHT, h );
#endif
             GE_START_CMDW( GE_OPW_BITBLT
                            | GE_OPW_FRGD_SRC_MAP
                            | GE_OPW_SRC_MAP_B
                            | GE_OPW_DEST_MAP_A   );
          }
      }
      GE_WAIT_IDLE_EXIT();
   }
}

 
void
agxFSpansTile( pDrawable, nSpans, ppts, pwidth,
               tile, pox, poy,
               alu, planemask )
    DrawablePtr     pDrawable;
    int             nSpans;
    DDXPointPtr     ppts; 
    int            *pwidth;
    PixmapPtr       tile;
    int             pox;
    int             poy;
    unsigned int    alu;
    unsigned int    planemask;
{
    unsigned int   width  = tile->drawable.width;
    unsigned int   height = tile->drawable.height;
    unsigned int   pixBWidth;
    unsigned int   srcX, srcY;
    unsigned short srcPWidth;
    unsigned short srcBWidth;
    short          srcBWidthShift;
    unsigned int   srcMaxLines;
    unsigned int   firstLine;
    unsigned int   lastLine;
    int            xOff = 0;
    DDXPointPtr    pPts = ppts;
    int            *pWidth = pwidth;
    int            n = nSpans;
    int            minX;
    int            maxX;
    Bool           wrap = FALSE;
    int            minLine;
    int            maxLine;
    unsigned int   newWidth;
    unsigned int   firstBWidth;
    unsigned int   secondBWidth;
    char           *pixStart;
    char           *pix2Start;

    modulus(pPts->x-pox,width,minX);
    modulus(pPts->x+*pWidth-pox-1,width,maxX);
    modulus(pPts->y-poy,height,minLine);
    maxLine = minLine;
    while( n-- > 0 ) {
       int tmp;
       modulus(pPts->x-pox,width,tmp);
       if( tmp < minX ) minX = tmp;
       tmp += *pWidth - 1 ;
       if( tmp >= width ) {
          wrap = TRUE;
          modulus(tmp,width,maxX);
       }
       else if( wrap && tmp > maxX && tmp < minX ) {
          maxX = tmp;
       }
       else if( tmp > maxX ) {
          maxX = tmp;
       }
       modulus(pPts->y-poy,height,tmp);
       if( tmp < minLine ) minLine = tmp;
       if( tmp > maxLine ) maxLine = tmp;
       pWidth++;
       pPts++;
    }
    pPts = ppts;
    pWidth = pwidth;
    n = nSpans;

    if( wrap && maxX > minX ) {
       if( minX > 0 ) {
          maxX = minX - 1;
       }
       else {
          maxX = width - 1;
          wrap = FALSE;
       }
    }

    if( wrap ) {
       int firstWidth = width - minX;
       newWidth = firstWidth + maxX + 1;
       firstBWidth = firstWidth << BytesPerPixelShift;
       secondBWidth = (maxX + 1) << BytesPerPixelShift;
    }
    else {
       newWidth = maxX - minX + 1;
       firstBWidth = PixmapBytePad( newWidth, tile->drawable.depth );
       secondBWidth = 0;
    }
    if( newWidth >= width ) {
       newWidth = width;
       firstBWidth = PixmapBytePad( newWidth, tile->drawable.depth );
       secondBWidth = 0;
       minX = 0;
       maxX = width - 1;
       wrap = FALSE;
    }

    pox += minX; 
    xOff = minX << BytesPerPixelShift;
    pixStart = (char *)tile->devPrivate.ptr + xOff;
    pix2Start = (char *)tile->devPrivate.ptr;
    srcBWidthShift = agxBytePadScratchMapPow2( width, BytesPerPixelShift );
    srcBWidth = 1 << srcBWidthShift;
    srcPWidth = srcBWidth >> BytesPerPixelShift;
    srcMaxLines = agxScratchSize >> srcBWidthShift;
    pixBWidth = PixmapBytePad( width, tile->drawable.depth );

    firstLine = minLine;
    if( (maxLine - minLine + 1) > srcMaxLines ) {
       lastLine = firstLine + srcMaxLines - 1;
    }
    else {
       lastLine = maxLine ;
    }

    MAP_INIT( GE_MS_MAP_B,
              agxVideoMapFormat,
              agxMemBase + agxScratchOffset,
              srcPWidth-1,
              height-1,
              FALSE, FALSE, FALSE );

    GE_WAIT_IDLE();

    agxPartMemToVid( agxScratchOffset, srcBWidth,
                     pixStart + (firstLine * pixBWidth), pixBWidth,
                     firstBWidth, lastLine - firstLine + 1 );
    if( wrap ) {
       agxPartMemToVid( agxScratchOffset+firstBWidth, srcBWidth,
                        pix2Start + (firstLine * pixBWidth), pixBWidth,
                        secondBWidth, lastLine - firstLine + 1 );
    }
 
    MAP_SET_DST( GE_MS_MAP_A );
    MAP_SET_SRC( GE_MS_MAP_B );
    GE_SET_MAP( GE_MS_MAP_B );
 
    GE_OUT_B(GE_FRGD_MIX, alu );
    GE_OUT_D(GE_PIXEL_BIT_MASK, planemask);

    GE_OUT_W( GE_PIXEL_OP,
              GE_OP_PAT_FRGD 
              | GE_OP_MASK_DISABLED
              | GE_OP_INC_X
              | GE_OP_INC_Y         );

    GE_OUT_W( GE_OP_DIM_HEIGHT, 0 );

    while( nSpans-- ) {
       int firstWidth = *pwidth;   
       int lastWidth = 0; 
       int numMids = 0;
       int nextX = ppts->x;

       if (firstWidth > 0) {

          modulus(ppts->x-pox,width,srcX);
          modulus(ppts->y-poy,height,srcY);

          /* 
           * If the GE can't handle the wrap (width ! a pow of 2)
           * we'll need to handle the horz tiling ourselves.
           */
          if( srcPWidth != width && (srcX + firstWidth) > width ) {
             /* we have to handle the wrap */
             int remains; 
             firstWidth = width - srcX;
             remains = *pwidth - firstWidth;
             numMids = remains / width;
             lastWidth = remains % width;
          }

          if( srcY > lastLine || srcY < firstLine ) {
             firstLine = srcY;
             lastLine = maxLine;
             if( lastLine - firstLine >= srcMaxLines ) {
                lastLine = firstLine + srcMaxLines - 1;
             }
             GE_WAIT_IDLE();

             agxPartMemToVid( agxScratchOffset, srcBWidth,
                              pixStart + (firstLine * pixBWidth), pixBWidth,
                              firstBWidth, lastLine - firstLine + 1 );
             if( wrap ) {
                agxPartMemToVid( agxScratchOffset+firstBWidth, srcBWidth,
                                 pix2Start + (firstLine * pixBWidth),
                                    pixBWidth,
                                 secondBWidth, lastLine - firstLine + 1 );
             }
          }
          srcY -= firstLine;

          GE_WAIT_IDLE();
#ifndef NO_MULTI_IO
          GE_OUT_D( GE_SRC_MAP_X,  (srcY << 16) | srcX );
          GE_OUT_D( GE_DEST_MAP_X, ppts->y << 16 | nextX );
#else
          GE_OUT_W( GE_SRC_MAP_X, srcX );
          GE_OUT_W( GE_SRC_MAP_Y, srcY );
          GE_OUT_W( GE_DEST_MAP_X, nextX );
          GE_OUT_W( GE_DEST_MAP_Y, ppts->y );
#endif
          GE_OUT_W( GE_OP_DIM_WIDTH, firstWidth - 1 );
          GE_OUT_W( GE_OP_DIM_HEIGHT, 0 );
          GE_START_CMDW( GE_OPW_BITBLT
                         | GE_OPW_FRGD_SRC_MAP
                         | GE_OPW_SRC_MAP_B
                         | GE_OPW_DEST_MAP_A   );

          nextX += firstWidth;
          while( numMids-- > 0 ) { 
             GE_WAIT_IDLE();
#ifndef NO_MULTI_IO
             GE_OUT_D( GE_SRC_MAP_X, srcY << 16 );
             GE_OUT_D( GE_DEST_MAP_X, ppts->y << 16 | nextX );
#else
             GE_OUT_W( GE_SRC_MAP_X, 0 );
             GE_OUT_W( GE_SRC_MAP_Y, srcY );
             GE_OUT_W( GE_DEST_MAP_X, nextX );
             GE_OUT_W( GE_DEST_MAP_Y, ppts->y );
#endif
             GE_OUT_W( GE_OP_DIM_WIDTH, width - 1 );
             GE_OUT_W( GE_OP_DIM_HEIGHT, 0 );
             GE_START_CMDW( GE_OPW_BITBLT
                            | GE_OPW_FRGD_SRC_MAP
                            | GE_OPW_SRC_MAP_B
                            | GE_OPW_DEST_MAP_A   );
             nextX += width;
          }

          if( lastWidth > 0 ) {
             GE_WAIT_IDLE();
#ifndef NO_MULTI_IO
             GE_OUT_D( GE_SRC_MAP_X, srcY << 16 );
             GE_OUT_D( GE_DEST_MAP_X, ppts->y << 16 | nextX );
#else
             GE_OUT_W( GE_SRC_MAP_X, 0 );
             GE_OUT_W( GE_SRC_MAP_Y, srcY );
#endif
             GE_OUT_W( GE_OP_DIM_WIDTH, lastWidth - 1 );
             GE_OUT_W( GE_OP_DIM_HEIGHT, 0 );
             GE_OUT_W( GE_DEST_MAP_X, nextX );
             GE_OUT_W( GE_DEST_MAP_Y, ppts->y );
             GE_START_CMDW( GE_OPW_BITBLT
                            | GE_OPW_FRGD_SRC_MAP
                            | GE_OPW_SRC_MAP_B
                            | GE_OPW_DEST_MAP_A   );
          }
       }
       ppts++;
       pwidth++;
    }
    GE_WAIT_IDLE_EXIT();
}
    

 
void
agxFSpansStipple( pDrawable, nSpans, ppts, pwidth,
                  stipple, pox, poy,
                  fgpixel, bgpixel, 
                  fgAlu, bgAlu,
                  planemask )
    DrawablePtr     pDrawable;
    int             nSpans;
    DDXPointPtr     ppts; 
    int            *pwidth;
    PixmapPtr       stipple;
    int             pox;
    int             poy;
    unsigned int    fgpixel;
    unsigned int    bgpixel;
    unsigned int    fgAlu;
    unsigned int    bgAlu;
    unsigned int    planemask;
{
    unsigned int   width  = stipple->drawable.width;
    unsigned int   height = stipple->drawable.height;
    unsigned int   pixBWidth;
    unsigned int   srcX, srcY;
    unsigned short srcPWidth;
    unsigned short srcBWidth;
    short          srcBWidthShift;
    unsigned int   srcMaxLines;
    unsigned int   firstLine;
    unsigned int   lastLine;
    DDXPointPtr    pPts = ppts; 
    int            *pWidth = pwidth;
    int            n = nSpans;
    int            minX;
    int            maxX;
    int            minLine;
    int            maxLine;
    unsigned int   newWidth;
    unsigned int   newBWidth;
    char           *pixStart;
    
    modulus(pPts->x-pox,width,minX);
    modulus(pPts->x+*pWidth-pox-1,width,maxX);
    modulus(pPts->y-poy,height,minLine);
    maxLine = minLine;
    /*
     * Try to load as little of the bitmap as possible,
     * but if we wrap it's not worth while as we have
     * to realign bits. Fortunately bitmaps are generally
     * small.
     */
    while( n-- > 0 ) {
       int tmp;
       modulus(pPts->x-pox,width,tmp);
       if( tmp < minX ) minX = tmp;
       tmp += *pWidth;
       if( tmp > width ) { 
          minX = 0;
          maxX = width - 1;
       }
       else if( tmp > maxX ) {
          maxX = tmp - 1; 
       }
       modulus(pPts->y-poy,height,tmp);
       if( tmp < minLine ) minLine = tmp;
       if( tmp > maxLine ) maxLine = tmp;
       pWidth++;
       pPts++;
    }
    pPts = ppts; 
    pWidth = pwidth;
    n = nSpans;

    minX &= 0xFFF8;
    newWidth = maxX - minX + 1;
    newBWidth = BitmapBytePad( newWidth );

    pox += minX;
    pixStart = (char *)stipple->devPrivate.ptr + (minX >> 3);
    srcBWidthShift = agxBytePadScratchMapPow2( newWidth, -3 );
    srcBWidth = 1 << srcBWidthShift;
    srcPWidth = srcBWidth << 3;
    srcMaxLines = agxScratchSize >> srcBWidthShift;
    pixBWidth = BitmapBytePad( width );

    firstLine = minLine; 
    if( (maxLine - minLine + 1) > srcMaxLines ) {
       lastLine = firstLine + srcMaxLines - 1;
    }
    else {
       lastLine = maxLine ;
    }

    MAP_INIT( GE_MS_MAP_B,
              GE_MF_1BPP,
              agxMemBase + agxScratchOffset,
              srcPWidth-1,
              height-1,
              FALSE, FALSE, FALSE );

    GE_WAIT_IDLE();

    agxPartMemToVid( agxScratchOffset, srcBWidth,
                     pixStart + (firstLine * pixBWidth), pixBWidth, 
                     newBWidth, lastLine - firstLine + 1 );
 
    MAP_SET_SRC_AND_DST( GE_MS_MAP_A );
    GE_SET_MAP( GE_MS_MAP_B );
 
#ifndef NO_MULTI_IO
    GE_OUT_W(GE_FRGD_MIX, bgAlu << 8) | fgAlu );  /* both fg & bg */
#else
    GE_OUT_B(GE_FRGD_MIX, fgAlu );
    GE_OUT_B(GE_BKGD_MIX, bgAlu );
#endif
    GE_OUT_D(GE_FRGD_CLR, fgpixel );
    GE_OUT_D(GE_BKGD_CLR, bgpixel );
    GE_OUT_D(GE_PIXEL_BIT_MASK, planemask);

    GE_OUT_W( GE_PIXEL_OP,
              GE_OP_PAT_MAP_B
              | GE_OP_MASK_DISABLED
              | GE_OP_INC_X
              | GE_OP_INC_Y         );

    while( nSpans-- ) {
       int firstWidth = *pwidth;   
       int lastWidth = 0; 
       int numMids = 0;
       int nextX = ppts->x;

       if (firstWidth > 0) {

          modulus(ppts->x-pox,width,srcX);
          modulus(ppts->y-poy,height,srcY);

          /* 
           * If the GE can't handle the wrap (width ! a pow of 2)
           * we'll need to handle the horz tiling ourselves.
           */
          if( srcPWidth != width && (srcX + firstWidth) > width ) {
             /* we have to handle the wrap */
             int remains; 
             firstWidth = width - srcX;
             remains = *pwidth - firstWidth;
             numMids = remains / width;
             lastWidth = remains % width;
          }
   
          if( srcY > lastLine || srcY < firstLine ) {
             firstLine = srcY;
             lastLine = maxLine;
             if( lastLine - firstLine >= srcMaxLines ) {
                lastLine = firstLine + srcMaxLines - 1;
             }
             GE_WAIT_IDLE();

             agxPartMemToVid( agxScratchOffset, srcBWidth,
                              pixStart + (firstLine * pixBWidth), pixBWidth, 
                              newBWidth, lastLine - firstLine + 1 );
          } 
          srcY -= firstLine;

          GE_WAIT_IDLE();
#ifndef NO_MULTI_IO
          GE_OUT_D( GE_PAT_MAP_X,  (srcY << 16) | srcX );
          GE_OUT_D( GE_DEST_MAP_X, ppts->y << 16 | nextX );
#else
          GE_OUT_W( GE_PAT_MAP_X, srcX );
          GE_OUT_W( GE_PAT_MAP_Y, srcY );
          GE_OUT_W( GE_DEST_MAP_X, nextX );
          GE_OUT_W( GE_DEST_MAP_Y, ppts->y );
#endif
          GE_OUT_W( GE_OP_DIM_WIDTH, firstWidth - 1 );
          GE_OUT_W( GE_OP_DIM_HEIGHT, 0 );
          GE_START_CMDW( GE_OPW_BITBLT
                         | GE_OPW_FRGD_SRC_CLR
                         | GE_OPW_BKGD_SRC_CLR
                         | GE_OPW_DEST_MAP_A   );

          nextX += firstWidth;
          while( numMids-- > 0 ) {
             GE_WAIT_IDLE();
#ifndef NO_MULTI_IO
             GE_OUT_D( GE_PAT_MAP_X, srcY << 16 );
             GE_OUT_D( GE_DEST_MAP_X, ppts->y << 16 | nextX );
#else
             GE_OUT_W( GE_PAT_MAP_X, 0 );
             GE_OUT_W( GE_PAT_MAP_Y, srcY );
             GE_OUT_W( GE_DEST_MAP_X, nextX );
             GE_OUT_W( GE_DEST_MAP_Y, ppts->y );
#endif
             GE_OUT_W( GE_OP_DIM_WIDTH, width - 1 );
             GE_OUT_W( GE_OP_DIM_HEIGHT, 0 );
             GE_START_CMDW( GE_OPW_BITBLT
                            | GE_OPW_FRGD_SRC_CLR
                            | GE_OPW_BKGD_SRC_CLR
                            | GE_OPW_DEST_MAP_A   );
             nextX += width;
          }

          if( lastWidth > 0 ) {
             GE_WAIT_IDLE();
#ifndef NO_MULTI_IO
             GE_OUT_D( GE_PAT_MAP_X, srcY << 16 );
             GE_OUT_D( GE_DEST_MAP_X, ppts->y << 16 | nextX );
#else
             GE_OUT_W( GE_PAT_MAP_X, 0 );
             GE_OUT_W( GE_PAT_MAP_Y, srcY );
#endif
             GE_OUT_W( GE_OP_DIM_WIDTH, lastWidth - 1 );
             GE_OUT_W( GE_OP_DIM_HEIGHT, 0 );
             GE_OUT_W( GE_DEST_MAP_X, nextX );
             GE_OUT_W( GE_DEST_MAP_Y, ppts->y );
             GE_START_CMDW( GE_OPW_BITBLT
                            | GE_OPW_FRGD_SRC_CLR
                            | GE_OPW_BKGD_SRC_CLR
                            | GE_OPW_DEST_MAP_A   );
          }
       }
       ppts++;
       pwidth++;
    }
    GE_WAIT_IDLE_EXIT();
}
    

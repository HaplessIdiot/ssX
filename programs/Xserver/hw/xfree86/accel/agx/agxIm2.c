/* $XFree86$ */
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

extern void     agxMemToVid();

#define MODULUS(a,b,d)  if (((d) = (a) % (b)) < 0) (d) += (b)


void
agxImageOpStipple(x, y, w, h, psrc, pwidth, pw, ph, pox, poy, fgPixel, bgPixel, alu, planemask)
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
    unsigned int	alu;
    unsigned int	planemask;
{
    unsigned short      srcPWidth;
    unsigned short      srcBWidth;
    short  		srcBWidthShift;
    unsigned short      srcStripHeight;
    unsigned int        srcMaxLines;
    unsigned short      srcMapLines;
    unsigned short      srcMapSize;
    unsigned short      srcLine;
    unsigned short      ppWidth;
    unsigned short      numHorizTiles;
    unsigned short      firstHTileWidth;
    unsigned short      lastHTileWidth;
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
    unsigned int        temp;

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

    srcBWidthShift = agxBytePadScratchMapPow2( pw, -3 );
    srcBWidth = 1 << srcBWidthShift;
    srcPWidth = srcBWidth << 3;
    srcMaxLines = agxScratchSize >> srcBWidthShift;

    if( pw == srcPWidth ) {
       numHorizTiles = 1;
       lastHTileWidth = pw;
       autoHTiling = TRUE;
    }
    else {
       unsigned short firstWidth = pw - xrot;
       unsigned short remWidth;
       firstHTileWidth = firstWidth <= w ? firstWidth : w; 
       remWidth = w - firstHTileWidth;
       numHorizTiles  = remWidth / pw;
       lastHTileWidth = remWidth % pw;
       if( firstHTileWidth )
          numHorizTiles++;
       if( lastHTileWidth )
          numHorizTiles++;
       else
          lastHTileWidth = pw;
       autoHTiling = FALSE;
    } 

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
   
    GE_WAIT_IDLE();

    if( agx256WidthAdjust ) {
       unsigned char byteData;
       outb(agxIdxReg,IR_M3_MODE_REG_3);
       byteData = inb(agxByteData);
       byteData &= ~IR_M3_256_SRC_ADJUST;
       byteData |= IR_M3_256_DST_ADJUST;
       outb(agxByteData,byteData);
    }
 
    GE_OUT_B( GE_PIXEL_MAP_SEL, GE_MS_MAP_B );
    GE_OUT_D( GE_PIXEL_MAP_BASE, agxMemBase + agxScratchOffset );
    GE_OUT_W( GE_PIXEL_MAP_WIDTH,  srcPWidth-1 );
    GE_OUT_W( GE_PIXEL_MAP_HEIGHT, srcStripHeight-1 );
    GE_OUT_B( GE_PIXEL_MAP_FORMAT, GE_MF_1BPP | GE_MF_MOTO_FORMAT );

    GE_OUT_D(GE_PIXEL_BIT_MASK, planemask);
    GE_OUT_W(GE_FRGD_MIX, (alu << 8) | alu );
    /*GE_OUT_B(GE_BKGD_MIX, alu );*/
    GE_OUT_D(GE_FRGD_CLR, fgPixel );
    GE_OUT_D(GE_BKGD_CLR, bgPixel );
/*
    GE_OUT_W( GE_PIXEL_OP,
              GE_OP_PAT_MAP_B
              | GE_OP_MASK_DISABLED
              | GE_OP_INC_X
              | GE_OP_INC_Y         );
*/

    srcLine = yrot; 
    vStripFirstY = y;

    for( strip = 1; strip <= numVertStrips; strip++ ) {
       unsigned short vTile;
       unsigned short dstY;

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
          unsigned short firstHeight = ph - srcLine;
          unsigned short secondHeight =  0;
          if( firstHeight < srcStripHeight )
             secondHeight = srcStripHeight - firstHeight;
          else
             firstHeight = srcStripHeight;
          agxMemToVid( agxScratchOffset, srcBWidth, 
                       psrc + srcLine*pwidth, pwidth,
                       firstHeight );
          if( secondHeight ) 
             agxMemToVid( agxScratchOffset + firstHeight*srcBWidth, 
                          srcBWidth,
                          psrc, pwidth,
                          secondHeight );
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
            GE_OUT_W( GE_PAT_MAP_X, srcX );
            GE_OUT_W( GE_PAT_MAP_Y, srcY );
            GE_OUT_W( GE_DEST_MAP_X, dstX );
            GE_OUT_W( GE_DEST_MAP_Y, dstY );
            GE_OUT_W( GE_OP_DIM_WIDTH, dstWidth );
            GE_START_CMD( GE_OP_BITBLT
                          | GE_OP_PAT_MAP_B
                          | GE_OP_MASK_DISABLED
                          | GE_OP_INC_X
                          | GE_OP_INC_Y 
                          | GE_OP_FRGD_SRC_CLR
                          | GE_OP_BKGD_SRC_CLR
                          | GE_OP_SRC_MAP_B
                          | GE_OP_DEST_MAP_A   );
            dstX += dstWidth + 1;    
          }
          dstY += ph; 
       }
       srcLine += srcStripHeight;
       if( srcLine > ph )
       srcLine -= ph; 
       vStripFirstY += srcStripHeight;
    }
    GE_WAIT_IDLE();
}

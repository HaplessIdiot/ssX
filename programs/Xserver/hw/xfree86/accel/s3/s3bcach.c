/* $XConsortium: s3bcach.c,v 1.1 94/03/28 21:14:19 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3bcach.c,v 3.2 1994/08/01 12:12:11 dawes Exp $ */
/*
 * Copyright 1993 by Jon Tombs. Oxford University
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Jon Tombs or Oxford University shall
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. The authors  make no
 * representations about the suitability of this software for any purpose. It
 * is provided "as is" without express or implied warranty.
 * 
 * JON TOMBS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 */

/*
 * Id: s3bcach.c,v 2.3 1993/07/24 13:16:56 jon Exp
 */


#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"cfb.h"
#include	"misc.h"
#include        "xf86.h"
#include	"windowstr.h"
#include	"gcstruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"xf86bcache.h"
#include	"s3.h"
#include	"regs3.h"

void
s3CacheMoveBlock(srcx, srcy, dstx, dsty, h, w, id)
int srcx, srcy, dstx, dsty, h, w;
unsigned int id;
{
   BLOCK_CURSOR;
   WaitQueue(8);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (s3DisplayWidth - 1));
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | s3ScissB);
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX);  
   S3_OUTW(FRGD_MIX, FSS_BITBLT | MIX_SRC);
   S3_OUTW(BKGD_MIX, BSS_BITBLT | MIX_SRC);
   S3_OUTW(WRT_MASK, 1 << id);
#ifdef S3_32BPP
   if (s3InfoRec.bitsPerPixel == 32)
      S3_OUTW(WRT_MASK, (short)(1 << id));
#endif

   WaitQueue(8);		/* now shift the cache */
   S3_OUTW(RD_MASK, 1 << id);
#ifdef S3_32BPP
   if (s3InfoRec.bitsPerPixel == 32)
      S3_OUTW(RD_MASK, (short)(1 << id));
#endif
   S3_OUTW(CUR_Y, srcy);
   S3_OUTW(CUR_X, srcx);
   S3_OUTW(DESTX_DIASTP, dstx);
   S3_OUTW(DESTY_AXSTP, dsty);
   S3_OUTW(MAJ_AXIS_PCNT, w - 1);
   S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (h - 1));
   S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

   WaitQueue(4);		/* sanity returns */
   S3_OUTW(RD_MASK, ~0);
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
   S3_OUTW(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);
   UNBLOCK_CURSOR;
}

/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/i128/i128accel.c,v 3.15 1998/04/09 02:35:56 robin Exp $ */

/*
 * Copyright 1997-1998 by Robin Cutshaw <robin@XFree86.Org>
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
 */

#include "cfb.h"
#include "gc.h"
#include "gcstruct.h"

#include "xf86.h"
#include "xf86xaa.h"
#include "xf86local.h"

#include "i128.h"
#include "i128reg.h"

#define ENG_PIPELINE_READY() { while (eng_cur[BUSY] & BUSY_BUSY) ; }
#define ENG_DONE() { while (eng_cur[FLOW] & (FLOW_DEB | FLOW_MCB | FLOW_PRV)) ;}

extern struct i128io i128io;
extern struct i128mem i128mem;
extern int i128DisplayWidth;
extern int i128DisplayOffset;
extern int i128DeviceType;
extern int i128MemoryType;
static volatile CARD32 *eng_a;
static volatile CARD32 *eng_b;
static volatile CARD32 *eng_cur;
static volatile i128dlpu *i128dl1p,  *i128dl2p;
static CARD32 i128dl1o, i128dl2o;
static CARD32 i128blitdir, i128cmd, i128rop, i128clptl, i128clpbr;
static CARD32 i128DashPattern, i128pctrlsize;
static i128dlpu dlpb[256];

/* pre-shift rops and just or in as needed */

CARD32 i128alu[16] =
{
   CR_CLEAR<<8,
   CR_AND<<8,
   CR_AND_REV<<8,
   CR_COPY<<8,
   CR_AND_INV<<8,
   CR_NOOP<<8,
   CR_XOR<<8,
   CR_OR<<8,
   CR_NOR<<8,
   CR_EQUIV<<8,
   CR_INVERT<<8,
   CR_OR_REV<<8,
   CR_COPY_INV<<8,
   CR_OR_INV<<8,
   CR_NAND<<8,
   CR_SET<<8
};
                        /*  8bpp   16bpp  32bpp unused */
static int min_size[]   = { 0x62,  0x32,  0x1A, 0x00 };
static int max_size[]   = { 0x80,  0x40,  0x20, 0x00 };
static int split_size[] = { 0x20,  0x10,  0x08, 0x00 };


void
i128EngineDone()
{
	ENG_DONE();
}


void
i128BitBlit(int x1, int y1, int x2, int y2, int w, int h)
{
	ENG_PIPELINE_READY();

	eng_cur[CMD] = i128cmd;
	/*eng_cur[XY3_DIR] = i128blitdir;*/

	if (i128blitdir & DIR_RL_TB) {
		x1 += w; x1--;
		x2 += w; x2--;
	}
	if (i128blitdir & DIR_LR_BT) {
		y1 += h; y1--;
		y2 += h; y2--;
	}



	if (i128DeviceType == I128_DEVICE_ID1) {
		int bppi;
		int origx2 = x2;
		int origy2 = y2;

		static int first_time_through = 1;

		/* The I128-1 has a nasty bitblit bug
		 * that occurs when dest is exactly 8 pages wide
		 */
		
		bppi = (eng_cur[BUF_CTRL] & BC_PSIZ_MSK) >> 24;

		if ((w >= min_size[bppi]) && (w <= max_size[bppi])) {
			if (first_time_through) {
            			ErrorF("%s: Using I128-1 workarounds.\n",
					i128InfoRec.name);
				first_time_through = 0;
			}

			bppi = split_size[bppi];
#if 1
			/* split method */

			eng_cur[XY2_WH] = (bppi<<16) | h;
			eng_cur[XY0_SRC] = (x1<<16) | y1;		MB;
			eng_cur[XY1_DST] = (x2<<16) | y2;		MB;

			ENG_PIPELINE_READY();

			w -= bppi;

			if (i128blitdir & DIR_RL_TB) {
				/* right to left blit */
				x1 -= bppi;
				x2 -= bppi;
			} else {
				/* left to right blit */
				x1 += bppi;
				x2 += bppi;
			}
#else
			/* clip method */
			eng_cur[CLPTL] = (origx2<<16) | origy2;
			eng_cur[CLPBR] = ((origx2+w)<<16) | (origy2+h);
			w += bppi;
#endif
		}
	}

	eng_cur[XY2_WH] = (w<<16) | h;
	eng_cur[XY0_SRC] = (x1<<16) | y1;				MB;
	eng_cur[XY1_DST] = (x2<<16) | y2;				MB;
}


void
i128SetupForScreenToScreenCopy(int xdir, int ydir, int rop, unsigned planemask,
	int transparency_color)
{

	ENG_PIPELINE_READY();

	eng_cur[MASK] = planemask;

	eng_cur[CLPTL] = 0x00000000;
	eng_cur[CLPBR] = (4095<<16) | 2047;

	if (transparency_color != -1)
		eng_cur[BACK] = transparency_color;


	if (xdir == -1) {
		if (ydir == -1) i128blitdir = DIR_RL_BT;
		else            i128blitdir = DIR_RL_TB;
	} else {
		if (ydir == -1) i128blitdir = DIR_LR_BT;
		else            i128blitdir = DIR_LR_TB;
	}
	eng_cur[XY3_DIR] = i128blitdir;

	i128rop = i128alu[rop];
	i128cmd = (transparency_color != -1 ? (CS_TRNSP<<16) : 0) |
		  i128rop | CO_BITBLT;
	eng_cur[CMD] = i128cmd;
}


void
i128SubsequentScreenToScreenCopy(int x1, int y1, int x2, int y2, int w, int h)
{
	i128BitBlit(x1, y1, x2, y2, w, h);
}


void
i128SetupForFillRectSolid(int color, int rop, unsigned planemask)
{

	ENG_PIPELINE_READY();
#if 0
ErrorF("SFFRS color 0x%x rop 0x%x (i128rop 0x%x) pmask 0x%x\n", color, rop, i128alu[rop]>>8, planemask);
#endif

	if (planemask == -1)
		eng_cur[MASK] = -1;
	else switch (xf86bpp) {
		case 8:
			eng_cur[MASK] = planemask |
					(planemask<<8) |
					(planemask<<16) |
					(planemask<<24);
			break;
		case 16:
			eng_cur[MASK] = planemask | (planemask<<16);
			break;
		case 24:
		case 32:
		default:
			eng_cur[MASK] = planemask;
			break;
	}

	eng_cur[FORE] = color;

	i128clptl = eng_cur[CLPTL] = 0x00000000;
	i128clpbr = eng_cur[CLPBR] = (4095<<16) | 2047 ;

	eng_cur[XY3_DIR] = i128blitdir = DIR_LR_TB;

	i128rop = i128alu[rop];
	i128cmd = (CS_SOLID<<16) | i128rop | CO_BITBLT;
	eng_cur[CMD] = i128cmd;
}


void
i128SubsequentFillRectSolid(int x, int y, int w, int h)
{
#if 0
ErrorF("SFRS %d,%d %d,%d\n", x, y, w, h);
#endif
	i128BitBlit(0, 0, x, y, w, h);
}


void
i128SubsequentTwoPointLine(int x1, int y1, int x2, int y2, int bias)
{
	ENG_PIPELINE_READY();
#if 0
ErrorF("STPL i128rop 0x%x  %d,%d %d,%d   clip %d,%d %d,%d\n", i128rop, x1, y1, x2, y2, i128clptl>>16, i128clptl&0xffff, (i128clpbr>>16)&0xffff, i128clpbr&0xffff);
#endif

	eng_cur[CMD] =
			((bias&0x0100) ? (CP_NLST<<24) : 0) |
			(CC_CLPRECI<<21) |
			(CS_SOLID<<16) |
			i128rop |
			CO_LINE;

	eng_cur[CLPTL] = i128clptl;
	eng_cur[CLPBR] = i128clpbr;

	eng_cur[XY0_SRC] = (x1<<16) | y1;				MB;
	eng_cur[XY1_DST] = (x2<<16) | y2;				MB;

}

#if 0
static int sdlprint;
#endif

void
i128SetupForDashedLine(int fg, int bg, int rop, unsigned planemask, int size)
{
	ENG_PIPELINE_READY();
#if 0
ErrorF("SFDL fg 0x%x bg 0x%x rop 0x%x(0x%x) pm 0x%x sz %d ", fg, bg, rop, i128alu[rop]>>8, planemask, size);
{ int i;
for (i=31; i>=0; i--) ErrorF("%d", (i128DashPattern>>i)&1);
ErrorF("\n");
sdlprint = 1;
}
#endif

	if (planemask == -1)
		eng_cur[MASK] = -1;
	else switch (xf86bpp) {
		case 8:
			eng_cur[MASK] = planemask |
					(planemask<<8) |
					(planemask<<16) |
					(planemask<<24);
			break;
		case 16:
			eng_cur[MASK] = planemask | (planemask<<16);
			break;
		case 24:
		case 32:
		default:
			eng_cur[MASK] = planemask;
			break;
	}
	i128DashPattern =
			(byte_reversed[i128DashPattern&0xFF]<<24) |
			(byte_reversed[(i128DashPattern>>8)&0xFF]<<16) |
			(byte_reversed[(i128DashPattern>>16)&0xFF]<<8) |
			 byte_reversed[(i128DashPattern>>24)&0xFF];

	eng_cur[FORE] = fg;
	if (bg != -1)
		eng_cur[BACK] = bg;
	i128pctrlsize = size & 0x1f;
	eng_cur[LPAT] = i128DashPattern;

	i128clptl = eng_cur[CLPTL] = 0x00000000;
	i128clpbr = eng_cur[CLPBR] = (4095<<16) | 2047 ;

	i128rop = i128alu[rop];
	i128cmd =
		(CP_PRST<<24) | (CC_CLPRECI<<21) |
		(bg == -1 ? (CS_TRNSP<<16) : (CS_SOLID<<16)) |
		i128rop |
		CO_LINE;
}


void
i128SubsequentDashedTwoPointLine(int x1, int y1, int x2, int y2,
				 int bias, int offset)
{
	ENG_PIPELINE_READY();
#if 0
if (sdlprint)
ErrorF("SDTPL %d,%d %d,%d   bias 0x%x offset 0x%x pctrl 0x%08x\n", x1, y1, x2, y2, bias, offset, i128pctrlsize | ((offset&0x1f)<<8));
sdlprint = 0;
#endif

	eng_cur[CMD] =
			((bias&0x0100) ? (CP_NLST<<24) : 0) |
			i128cmd;

	eng_cur[CLPTL] = i128clptl;
	eng_cur[CLPBR] = i128clpbr;

	eng_cur[PCTRL] = i128pctrlsize | ((offset&0x1f)<<8);
	eng_cur[XY0_SRC] = (x1<<16) | y1;				MB;
	eng_cur[XY1_DST] = (x2<<16) | y2;				MB;
}


void
i128SetClippingRectangle(int x1, int y1, int x2, int y2)
{
	int tmp;
#if 0
ErrorF("SCR  %d,%d %d,%d\n", x1, y1, x2, y2);
#endif

	if (x1 > x2) { tmp = x2; x2 = x1; x1 = tmp; }
	if (y1 > y2) { tmp = y2; y2 = y1; y1 = tmp; }

	i128clptl = (x1<<16) | y1;
	i128clpbr = (x2<<16) | y2;
}


void
i128FillRectSolid(DrawablePtr pDraw, GCPtr pGC, int nBox, register BoxPtr pBoxI)
{
	register int w, h, planemask;

	ENG_PIPELINE_READY();
#if 0
ErrorF("FRS color 0x%x rop 0x%x (i128rop 0x%x) pmask 0x%x\n", pGC->fgPixel, pGC->alu, i128alu[pGC->alu]>>8, pGC->planemask);
#endif

	planemask = pGC->planemask;

	if (planemask != -1) {
		if (xf86bpp == 8) {
			planemask |= (planemask<<8)  |
				     (planemask<<16) |
				     (planemask<<24);
		} else if (xf86bpp == 16)
			planemask |= planemask<<16;
	}

	eng_cur[MASK] = planemask;
	eng_cur[FORE] = pGC->fgPixel;
	eng_cur[CMD] = (CS_SOLID<<16) | i128alu[pGC->alu] | CO_BITBLT;
	eng_cur[CLPTL] = 0x00000000;
	eng_cur[CLPBR] = (4095<<16) | 2047;

	eng_cur[XY3_DIR] = DIR_LR_TB;
	eng_cur[XY0_SRC] = 0x00000000;

	while (nBox > 0) {
		w = pBoxI->x2 - pBoxI->x1;
		h = pBoxI->y2 - pBoxI->y1;
		if (w > 0 && h > 0) {
			eng_cur[XY2_WH] = (w<<16) | h;			MB;
			eng_cur[XY1_DST] = (pBoxI->x1<<16) | pBoxI->y1;	MB;
			ENG_PIPELINE_READY();
		}
		pBoxI++;
		nBox--;
	}

	ENG_DONE();
}


void
i128T2RFillRectSolid(DrawablePtr pDraw, GCPtr pGC, int nBox, BoxPtr pBoxI)
{
	register int planemask;
	register CARD32 *idp, *sdp;
	register i128dlpu *dp;
	static i128dlpu staticdp[2] = {
		{
		(MASK*4)&0xff,			/* aad */
		(FORE*4)&0xff,			/* bad */
		(CMD*4)&0xff,			/* cad */
		0x0c |
		(((MASK*4)&0x100)>>4) |
		(((FORE*4)&0x100)>>3) |
		(((CMD*4)&0x100)>>2),		/* control */
		0x00000000,			/* rad */
		0x00000000,			/* rbd */
		0x00000000,			/* rcd */
		},
		{
		(CLPTL*4)&0xff,			/* aad */
		(CLPBR*4)&0xff,			/* bad */
		0x00,				/* cad */
		0x08 |
		(((CLPTL*4)&0x100)>>4) |
		(((CLPBR*4)&0x100)>>3),		/* control */
		0x00000000,			/* rad */
		(4095<<16) | 2047,		/* rbd */
		0x00000000,			/* rcd */
		}
	};

	ENG_PIPELINE_READY();

	planemask = pGC->planemask;

	if (planemask != -1) {
		if (xf86bpp == 8) {
			planemask |= (planemask<<8)  |
				     (planemask<<16) |
				     (planemask<<24);
		} else if (xf86bpp == 16)
			planemask |= planemask<<16;
	}

	if (nBox == 1) {
		register int w, h;

		eng_cur[MASK] = planemask;
		eng_cur[FORE] = pGC->fgPixel;
		eng_cur[CMD] = (CS_SOLID<<16) | i128alu[pGC->alu] | CO_BITBLT;
		eng_cur[CLPTL] = 0x00000000;
		eng_cur[CLPBR] = (4095<<16) | 2047;

		eng_cur[XY3_DIR] = DIR_LR_TB;
		eng_cur[XY0_SRC] = 0x00000000;

		w = pBoxI->x2 - pBoxI->x1;
		h = pBoxI->y2 - pBoxI->y1;
		if (w > 0 && h > 0) {
			eng_cur[XY2_WH] = (w<<16) | h;			MB;
			eng_cur[XY1_DST] = (pBoxI->x1<<16) | pBoxI->y1;	MB;
		}
		ENG_DONE();
		return;
	}

	staticdp[0].f0.rad = planemask;
	staticdp[0].f0.rbd = pGC->fgPixel;
	staticdp[0].f0.rcd = (CS_SOLID<<16) | i128alu[pGC->alu] | CO_BITBLT;

	/* copy and trigger the transfer */

	idp = (CARD32 *)i128dl2p;
	sdp = (CARD32 *)staticdp;
	*idp++ = *sdp++;
	*idp++ = *sdp++;
	*idp++ = *sdp++;
	*idp++ = *sdp++;
	*idp++ = *sdp++;
	*idp++ = *sdp++;
	*idp++ = *sdp++;
	*idp++ = *sdp++;

	while (eng_cur[DL_ADR] & 0x40000000) ;

	eng_cur[DL_ADR] = i128dl2o;					MB;
	eng_cur[DL_CNTRL] = i128dl2o + 32;				MB;

	while (nBox > 0) {
		register int w, h, n, ndp, dlpslotsleft;

		dp = dlpb;
		ndp = 0;

		/* fill in up to 16 slots in the dlp1 buffer */

		dlpslotsleft = 16;
		while ((nBox > 0) && (dlpslotsleft > 0)) {
			w = pBoxI->x2 - pBoxI->x1;
			h = pBoxI->y2 - pBoxI->y1;
			if (w > 0 && h > 0) {
				dp->f1.xy0 = 0x0000000;
				dp->f1.xy2 = (w<<16) | h;
				dp->f1.xy3 = DIR_LR_TB; /* == 0 */
				dp->f1.xy1 = (pBoxI->x1<<16) | pBoxI->y1;
				dp++; ndp++; dlpslotsleft--;
			}
			pBoxI++;
			nBox--;
		}
		if (ndp == 0)  /* no valid rects, we're done */
			break;

		/* copy the words into the frame buffer dlp1 cache */

		idp = (CARD32 *)i128dl1p;
		sdp = (CARD32 *)dlpb;
		n = ndp;

		while (n-- > 0) {
			*idp++ = *sdp++;
			*idp++ = *sdp++;
			*idp++ = *sdp++;
			*idp++ = *sdp++;
		}

		/* trigger the dlp op */

		while (eng_cur[DL_ADR] & 0x40000000) ;

		eng_cur[DL_ADR] = i128dl1o;				MB;
		eng_cur[DL_CNTRL] = (i128dl1o + 16*ndp) | 0x20000000;	MB;

		/* we do our own multi-threading here */

		dp = dlpb;
		ndp = 0;

		/* fill in up to 128 slots in the dlp2 buffer */

		dlpslotsleft = 128;
		while ((nBox > 0) && (dlpslotsleft > 0)) {
			w = pBoxI->x2 - pBoxI->x1;
			h = pBoxI->y2 - pBoxI->y1;
			if (w > 0 && h > 0) {
				dp->f1.xy0 = 0x0000000;
				dp->f1.xy2 = (w<<16) | h;
				dp->f1.xy3 = DIR_LR_TB; /* == 0 */
				dp->f1.xy1 = (pBoxI->x1<<16) | pBoxI->y1;
				dp++; ndp++; dlpslotsleft--;
			}
			pBoxI++;
			nBox--;
		}
		if (ndp == 0)  /* no valid rects, we're done */
			break;

		/* copy the words into the frame buffer dlp2 cache */

		idp = (CARD32 *)i128dl2p;
		sdp = (CARD32 *)dlpb;
		n = ndp;

		while (n-- > 0) {
			*idp++ = *sdp++;
			*idp++ = *sdp++;
			*idp++ = *sdp++;
			*idp++ = *sdp++;
		}

		/* trigger the dlp op */

		while (eng_cur[DL_ADR] & 0x40000000) ;

		eng_cur[DL_ADR] = i128dl2o;				MB;
		eng_cur[DL_CNTRL] = (i128dl2o + 16*ndp) | 0x20000000;	MB;
	}

	while (!(eng_cur[DL_CNTRL] & 0x00000400)) ;

	ENG_DONE();
}


void
i128ScreenToScreenBitBlt(int nbox, DDXPointPtr pptSrc, BoxPtr pbox,
			 int xdir, int ydir, int alu, unsigned planemask)
{
        i128SetupForScreenToScreenCopy(xdir, ydir, alu, planemask, -1);
        for (; nbox; pbox++, pptSrc++, nbox--)
            i128SubsequentScreenToScreenCopy(pptSrc->x, pptSrc->y,
                pbox->x1, pbox->y1, pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
	ENG_DONE();
}


void
i128AccelInit()
{
	CARD32 buf_ctrl;

	xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS |
				 HARDWARE_CLIP_LINE |
				 USE_TWO_POINT_LINE |
				 LINE_PATTERN_MSBFIRST_MSBJUSTIFIED |
				 TWO_POINT_LINE_NOT_LAST |
				 MICROSOFT_ZERO_LINE_BIAS |
				 PIXMAP_CACHE
#ifdef DELAYED_SYNC
				 | DELAYED_SYNC
#endif
				 ;

	if (i128DeviceType == I128_DEVICE_ID3)
		xf86AccelInfoRec.Flags |= ONLY_LEFT_TO_RIGHT_BITBLT;

	xf86AccelInfoRec.ServerInfoRec = &i128InfoRec;

	xf86AccelInfoRec.Sync = i128EngineDone;

	xf86AccelInfoRec.SetupForScreenToScreenCopy =
		i128SetupForScreenToScreenCopy;
	xf86AccelInfoRec.SubsequentScreenToScreenCopy =
		i128SubsequentScreenToScreenCopy;

	xf86GCInfoRec.CopyAreaFlags = 0;

	xf86AccelInfoRec.SetupForFillRectSolid =
		i128SetupForFillRectSolid;
	xf86AccelInfoRec.SubsequentFillRectSolid =
		i128SubsequentFillRectSolid;

	xf86AccelInfoRec.SubsequentTwoPointLine =
		i128SubsequentTwoPointLine;

	xf86AccelInfoRec.SetupForDashedLine =
		i128SetupForDashedLine;
	xf86AccelInfoRec.SubsequentDashedTwoPointLine =
		i128SubsequentDashedTwoPointLine;
	xf86AccelInfoRec.LinePatternBuffer =
		(void *)&i128DashPattern;
	xf86AccelInfoRec.LinePatternMaxLength = 32;

	xf86AccelInfoRec.SetClippingRectangle =
		i128SetClippingRectangle;

	xf86GCInfoRec.PolyFillRectSolidFlags = 0;

	xf86AccelInfoRec.PixmapCacheMemoryStart = i128InfoRec.virtualY *
		i128DisplayWidth * i128InfoRec.bitsPerPixel / 8;

	xf86AccelInfoRec.PixmapCacheMemoryEnd =
		(i128InfoRec.videoRam * 1024) - 1024;

	/*/
	 * If there is sufficient memory, we create two blocks of
	 * 128 diplay list instruction words (16 bytes each).
	/*/

	if ((xf86AccelInfoRec.PixmapCacheMemoryEnd -
	     xf86AccelInfoRec.PixmapCacheMemoryStart) > 16*256) {
		xf86AccelInfoRec.PixmapCacheMemoryEnd -= 16*256;
		i128dl1o = xf86AccelInfoRec.PixmapCacheMemoryEnd;
		i128dl1p = (i128dlpu *)&i128mem.mw0_ad[i128dl1o];
		i128dl2o = i128dl1o + 16*128;
		i128dl2p = (i128dlpu *)&i128mem.mw0_ad[i128dl2o];
	} else {
		i128dl1o = i128dl2o = 0;
		i128dl1p = i128dl2p = (i128dlpu *)0;
	}

	xf86GCInfoRec.PolyFillRectSolid = xf86PolyFillRect;
	if ((i128DeviceType == I128_DEVICE_ID3) && (i128dl1o != 0))
		xf86AccelInfoRec.FillRectSolid = i128T2RFillRectSolid;
	else
		xf86AccelInfoRec.FillRectSolid = i128FillRectSolid;


	i128InfoRec.displayWidth = i128DisplayWidth;

	eng_a = i128mem.rbase_a;
	eng_b = i128mem.rbase_b;
	eng_cur = eng_a;

	switch (xf86bpp) {
		case 8:  buf_ctrl = BC_PSIZ_8B;  break;
		case 16: buf_ctrl = BC_PSIZ_16B; break;
		case 24:
		case 32: buf_ctrl = BC_PSIZ_32B; break;
		default: buf_ctrl = 0;           break; /* error */
	}
	if (i128DeviceType == I128_DEVICE_ID3) {
		if (i128MemoryType == I128_MEMORY_SGRAM)
			buf_ctrl |= BC_MDM_PLN;
		else
			buf_ctrl |= BC_BLK_ENA;
	}
	eng_cur[BUF_CTRL] = buf_ctrl;

	eng_cur[DE_PGE] = 0x00;
	eng_cur[DE_SORG] = i128DisplayOffset;
	eng_cur[DE_DORG] = i128DisplayOffset;
	eng_cur[DE_MSRC] = 0x00;
	eng_cur[DE_WKEY] = 0x00;
	eng_cur[DE_SPTCH] = i128mem.rbase_g[DB_PTCH];
	eng_cur[DE_DPTCH] = i128mem.rbase_g[DB_PTCH];
	eng_cur[RMSK] = 0x00000000;
	eng_cur[XY4_ZM] = ZOOM_NONE;
	eng_cur[LPAT] = 0xffffffff;  /* for lines */
	eng_cur[PCTRL] = 0x00000000; /* for lines */
	eng_cur[CLPTL] = 0x00000000;
	eng_cur[CLPBR] = (4095<<16) | 2047 ;
}

/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/i128/i128accel.c,v 3.10 1997/08/12 12:02:00 hohndel Exp $ */

/*
 * Copyright 1997 by Robin Cutshaw <robin@XFree86.Org>
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

#include "i128.h"
#include "i128reg.h"

extern struct i128io i128io;
extern struct i128mem i128mem;
extern int i128DisplayWidth;
extern int i128DisplayOffset;
extern int i128DeviceType;
extern int i128MemoryType;
static volatile CARD32 *eng_a;
static volatile CARD32 *eng_b;
static volatile CARD32 *eng_cur;
static int i128blitdir, i128rop, i128cmd, i128clptl, i128clpbr;

short i128alu[16] =
{
   CR_CLEAR,
   CR_AND,
   CR_AND_REV,
   CR_COPY,
   CR_AND_INV,
   CR_NOOP,
   CR_XOR,
   CR_OR,
   CR_NOR,
   CR_EQUIV,
   CR_INVERT,
   CR_OR_REV,
   CR_COPY_INV,
   CR_OR_INV,
   CR_NAND,
   CR_SET
};
                        /*  8bpp   16bpp  32bpp unused */
static int min_size[]   = { 0x62,  0x32,  0x1A, 0x00 };
static int max_size[]   = { 0x80,  0x40,  0x20, 0x00 };
static int split_size[] = { 0x20,  0x10,  0x08, 0x00 };

unsigned char byte_reversed[256] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};


void
i128EngineDone()
{
	int flow;

	do {
		flow = eng_a[FLOW];
	} while (flow & (FLOW_DEB | FLOW_MCB | FLOW_PRV));

#if 0
	do {
		flow = eng_b[FLOW];
	} while (flow & (FLOW_DEB | FLOW_MCB));
#endif
}

void
i128EngineReady()
{
	int busy;

	do {
		busy = eng_cur[BUSY];
	} while (busy & BUSY_BUSY) ;
}

void
i128BitBlit(int x1, int y1, int x2, int y2, int w, int h, int cmd, int dir)
{
	int origx2 = x2;
	int origy2 = y2;

	i128EngineReady();

	eng_cur[CMD] = cmd;
	eng_cur[XY3_DIR] = dir;
	eng_cur[XY4_ZM] = ZOOM_NONE;

	if (i128blitdir & DIR_RL_TB) {
		x1 += w - 1;
		x2 += w - 1;
	}
	if (i128blitdir & DIR_LR_BT) {
		y1 += h - 1;
		y2 += h - 1;
	}


	if (i128DeviceType == I128_DEVICE_ID1) {
		int bppi;
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

			i128EngineReady();

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
	static CARD32 buf_ctrl = 1;

	i128EngineReady();

	if (buf_ctrl == 1) {
		switch (xf86bpp) {
			case 8:
				buf_ctrl = BC_PSIZ_8B;
				break;
			case 16:
				buf_ctrl = BC_PSIZ_16B;
				break;
			case 24:
			case 32:
				buf_ctrl = BC_PSIZ_32B;
				break;
			default:
				/* programming error */
				return;
		}
		if (i128DeviceType == I128_DEVICE_ID3) {
			buf_ctrl |= BC_BLK_ENA;
			if (i128MemoryType == I128_MEMORY_SGRAM)
				buf_ctrl |= BC_MDM_PLN;
		}
		eng_cur[BUF_CTRL] = buf_ctrl;
	}

	eng_cur[DE_PGE] = 0x00;
	eng_cur[DE_SORG] = i128DisplayOffset;
	eng_cur[DE_DORG] = i128DisplayOffset;
	eng_cur[DE_MSRC] = 0x00;
	eng_cur[DE_WKEY] = 0x00;
	eng_cur[DE_SPTCH] = i128mem.rbase_g[DB_PTCH];
	eng_cur[DE_DPTCH] = i128mem.rbase_g[DB_PTCH];
	eng_cur[MASK] = planemask;
	eng_cur[RMSK] = 0x00000000;

	eng_cur[CLPTL] = 0x00000000;
	eng_cur[CLPBR] = (4095<<16) | 2047;

	if (transparency_color != -1)
		eng_cur[BACK] = transparency_color;


	if (xdir == -1) {
		if (ydir == -1)
			i128blitdir = DIR_RL_BT;
		else
			i128blitdir = DIR_RL_TB;
	} else {
		if (ydir == -1)
			i128blitdir = DIR_LR_BT;
		else
			i128blitdir = DIR_LR_TB;
	}

	i128rop = i128alu[rop];
	i128cmd = (transparency_color != -1 ? (CS_TRNSP<<16) : 0) |
		  (i128rop<<8) | CO_BITBLT;
}

void
i128SubsequentScreenToScreenCopy(int x1, int y1, int x2, int y2, int w, int h)
{
	i128BitBlit(x1, y1, x2, y2, w, h, i128cmd, i128blitdir);
}

void
i128SetupForFillRectSolid(int color, int rop, unsigned planemask)
{
	CARD32 buf_ctrl;

	i128EngineReady();
#if 0
ErrorF("SFFRS color 0x%x rop 0x%x (i128rop 0x%x) pmask 0x%x\n", color, rop, i128alu[rop], planemask);
#endif

	switch (xf86bpp) {
		case 8:
			buf_ctrl = BC_PSIZ_8B;
			if (planemask != -1)
				eng_cur[MASK] = planemask |
						(planemask<<8) |
						(planemask<<16) |
						(planemask<<24);
			else
				eng_cur[MASK] = planemask;
			break;
		case 16:
			buf_ctrl = BC_PSIZ_16B;
			if (planemask != -1)
				eng_cur[MASK] = planemask | (planemask<<16);
			else
				eng_cur[MASK] = planemask;
			break;
		case 24:
		case 32:
			buf_ctrl = BC_PSIZ_32B;
			eng_cur[MASK] = planemask;
			break;
		default:
			/* programming error */
			return;
	}

	if (i128DeviceType == I128_DEVICE_ID3) {
		buf_ctrl |= BC_BLK_ENA;
		if (i128MemoryType == I128_MEMORY_SGRAM)
			buf_ctrl |= BC_MDM_PLN;
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
	eng_cur[FORE] = color;

	i128clptl = eng_cur[CLPTL] = 0x00000000;
	i128clpbr = eng_cur[CLPBR] = (4095<<16) | 2047 ;

	eng_cur[LPAT] = 0xffffffff;  /* for lines */
	eng_cur[PCTRL] = 0x00000000; /* for lines */


	i128blitdir = DIR_LR_TB;

	i128rop = i128alu[rop];
	i128cmd = (CS_SOLID<<16) | (i128rop<<8) | CO_BITBLT;
}

void
i128SubsequentFillRectSolid(int x, int y, int w, int h)
{
#if 0
ErrorF("SFRS %d,%d %d,%d  cmd 0x%x dir 0x%x\n", x, y, w, h, i128cmd, i128blitdir);
#endif
	i128BitBlit(0, 0, x, y, w, h, i128cmd, i128blitdir);
}

void
i128SubsequentTwoPointLine(int x1, int y1, int x2, int y2, int bias)
{
	i128EngineReady();
#if 0
ErrorF("STPL i128rop 0x%x  %d,%d %d,%d   clip %d,%d %d,%d\n", i128rop, x1, y1, x2, y2, i128clptl>>16, i128clptl&0xffff, (i128clpbr>>16)&0xffff, i128clpbr&0xffff);
#endif

	eng_cur[CMD] =
			((bias&0x0100) ? (CP_NLST<<24) : 0) |
			(CC_CLPRECI<<21) |
			(CS_SOLID<<16) |
			(i128rop<<8) |
			CO_LINE;

	eng_cur[CLPTL] = i128clptl;
	eng_cur[CLPBR] = i128clpbr;

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
i128AccelInit()
{
	xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS |
				 HARDWARE_CLIP_LINE |
				 USE_TWO_POINT_LINE |
				 TWO_POINT_LINE_NOT_LAST |
				 PIXMAP_CACHE |
				 DELAYED_SYNC;

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

	xf86AccelInfoRec.SetClippingRectangle =
		i128SetClippingRectangle;

	xf86GCInfoRec.PolyFillRectSolidFlags = 0;

	xf86AccelInfoRec.PixmapCacheMemoryStart = i128InfoRec.virtualY *
		i128DisplayWidth * i128InfoRec.bitsPerPixel / 8;

	xf86AccelInfoRec.PixmapCacheMemoryEnd =
		i128InfoRec.videoRam * 1024-1024;

	i128InfoRec.displayWidth = i128DisplayWidth;

	eng_a = i128mem.rbase_a;
	eng_b = i128mem.rbase_b;
	eng_cur = eng_a;

	eng_cur[CLPTL] = 0x00000000;
	eng_cur[CLPBR] = (4095<<16) | 2047 ;
}

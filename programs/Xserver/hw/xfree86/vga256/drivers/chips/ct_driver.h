/*
 *
 * Modified 1996 by Egbert Eich <Egbert.Eich@Physik.TH-Darmstadt.DE>
 * Modified 1996 by David Bateman <dbateman@ee.uts.edu.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/chips/ct_driver.h,v 3.0 1996/08/11 13:02:48 dawes Exp $ */

extern Bool ctLinearSupport;	       /*linear addressing enable */
extern Bool ctAccelSupport;	       /*acceleration enable */
extern Bool ctisHiQV32;		       /*New architecture used in 65550 and 65554 */
extern Bool ctHDepth;		       /*Chip has 16/24bpp */
extern Bool ctDSTN;
extern Bool ctLCD;
extern Bool ctCRT;

extern unsigned int ctCursorAddress;   /* The address in video ram of cursor */

/* The adress in video ram of the tile pattern.  */
extern unsigned int ctBLTPatternAddress;
extern Bool ctUseMMIO;
extern unsigned char *ctMMIOBase;

extern int ctAluConv[];		       /* Map Alu to Chips ROP */

extern unsigned long ctFrameBufferSize;		/* Frame buffer size */

/* 
 * Definitions for IO access to 32 bit ports
 */
extern unsigned CHIPS_ExtPorts32[];
#define DR(x) CHIPS_ExtPorts32[x]

/*
 * Forward definitions for the functions that make up the driver.    See
 * the definitions of these functions for the real scoop.
 */

/* in ct_blitter.c */
extern void ctBitBlt();
extern void ctMMIOBitBlt();

/* in ct_BitBlt.c */
extern void ctcfbDoBitbltCopy();
extern void ctcfbFillBoxSolid();

/* in ct_solid.c */
extern void ctcfbFillRectSolid();
extern void ctcfbFillSolidSpansGeneral();
extern void ctMMIOFillRectSolid();
extern void ctMMIOFillSolidSpansGeneral();

/* in ct_blt16.c */
extern RegionPtr ctcfb16CopyArea();
extern RegionPtr ctcfb24CopyArea();

/* in ct_pci.c */
extern int ctPCIMemBase();
extern int ctPCIIOBase();

/* in ct_FillRct.c */
extern void ctcfbPolyFillRect();

/* in ct_line.c */
#ifdef CHIPS_SUPPORT_MMIO
extern void ctMMIOLineSS();
extern void ctMMIOSegmentSS();

#endif

#define MMIOmem(x) *(unsigned int *)(ctMMIOBase + x)

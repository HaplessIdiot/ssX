/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir_imbltc.c,v 1.2 1997/10/26 15:46:40 hohndel Exp $ */

/*
 * A few dummies to resolve symbols on the PowerPC
 * until we translate the x86 assembler back to C code
 *
 * NOTE: Most (if not all) of these functions appear to be used
 * only with the older chips that do not support linear mode, so
 * are probably never neede on a powerpc....
 */

/*
 * This low-level routine copies bitmap data to video memory for the
 * blitter, which must be setup for system-memory-to-video-memory BLT.
 * The video address where the data is written doesn't matter. Each bitmap
 * scanline transmitted is padded to a multiple of 4 bytes; the bitmap is
 * transfered in dwords (this is effectively 16-bit words because of the
 * 16-bit host interface of the 5426/28).
 *
 * This function is used by the 5426 and 5428.
 *
 * width is the bitmap width in bytes.
 * height is the height of the bitmap.
 * srcaddr is the address of the bitmap data.
 * srcwidth is the length of a bitmap scanline in bytes.
 * vaddr is a video memory address (doesn't really matter).
 */
void
CirrusImageWriteTransfer(int width, int height, void *srcaddr,
			 int srcwidth, void *vaddr)
{
	/*
	 * XXX - CirrusImageWriteTransfer: Not implemented
	 */
 	ErrorF("CirrusImageWriteTransfer: Not implemented!!!\n");
}


/* 
 * This may be wrong. For image reads, scanlines are not padded two a multiple
 * of 4 bytes (if I understand the databook correctly).
 */

/* 
 * This is the equivalent function for image read blits.
 *
 * width is the bitmap width in bytes.
 * height is the height of the bitmap.
 * destaddr is the address of the bitmap data.
 * destwidth is the length of a bitmap scanline in bytes.
 * vaddr is a video memory address (doesn't really matter).
 */
void
CirrusImageReadTransfer(int width, int height, void *destaddr,
			int destwidth, void *vaddr)
{
	/*
	 * XXX - CirrusImageReadTransfer: Not implemented
	 */
 	ErrorF("CirrusImageReadTransfer: Not implemented!!!\n");
}

/* 
 * Function to transfer monochrome bitmap data to the blitter.
 * Has to reverse the per-byte bit order. 
 * Because the BitBLT engine wants only dword transfers and cannot accept
 * scanline padding, bytes from different scanlines have to be combined.
 *
 * bytewidth is the number of bytes to be transferred per scanline.
 * h is the number of bitmap scanlines to be transfered.
 * bwidth is the total (padded) width of a bitmap scanline.
 * srcaddr is the address of the bitmap data.
 * vaddr is a video memory address (doesn't really matter).
 */
void
CirrusBitmapTransfer(int bytewidth, int h, int bwidth, void *srcaddr,
		     void *vaddr)
{
	/*
	 * XXX - CirrusBitmapTransfer: Not implemented
	 */
 	ErrorF("CirrusBitmapTransfer: Not implemented!!!\n");
}

#if 0  /* Not currently used by driver - see cir_im.c for details */

/* 
 * Function to transfer monochrome bitmap data to the blitter.
 * Has to reverse the per-byte bit order.
 * Assumes source data can be transfered as-is, i.e. scanlines are in a
 * format suitable for the BitBLT engine (not padded).
 *
 * count is the number of 32-bit words to transfer.
 * srcaddr is the address of the bitmap data.
 * vaddr is a video memory address (doesn't really matter).
 */
void
CirrusAlignedBitmapTransfer(int count, void *srcaddr, void *vaddr)
{
	/*
	 * XXX - CirrusAlignedBitmapTransfer: Not implemented
	 */
 	ErrorF("CirrusAlignedBitmapTransfer: Not implemented!!!\n");
}

#endif

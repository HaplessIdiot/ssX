/* $XConsortium: ct_driver.h /main/3 1996/10/27 11:49:29 kaleb $ */
/*
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

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/chips/ct_driver.h,v 1.7 1998/03/28 06:35:58 hohndel Exp $ */

/*#define DEBUG
#define CT_HW_DEBUG */
#define CT_DEBUG_WAIT 500000

extern Bool ctLinearSupport;	       /*linear addressing enable */
extern Bool ctAccelSupport;	       /*acceleration enable */
extern Bool ctisHiQV32;		       /*New architecture used in 65550 and 65554 */
extern Bool ctisWINGINE;
extern Bool ctHDepth;		       /*Chip has 16/24bpp */
extern Bool ctDSTN;
extern Bool ctHWCursor;
extern Bool ctHWCursorAlways;

extern unsigned int ctCursorAddress;   /* The address in video ram of cursor */

/* The adress in video ram of the tile pattern.  */
extern unsigned int ctBLTPatternAddress;
extern Bool ctUseMMIO;
extern Bool ctAvoidImageBLT;
extern Bool ctColorTransparency;
extern unsigned char *ctMMIOBase;
extern unsigned char *ctBltDataWindow;

extern int ctAluConv[];		       /* Map Alu to Chips ROP source data  */
extern int ctAluConv2[];       	       /* Map Alu to Chips ROP pattern data */
extern int ctAluConv3[];       	       /* Map Alu to Chips ROP source data with
					  pattern as planemask */

extern unsigned long ctFrameBufferSize;		/* Frame buffer size */
extern unsigned int ctCacheEnd;			/* Pixmap Cache End */

/* Byte reversal functions */
extern unsigned int byte_reversed3[];

/* 
 * Definitions for IO access to ports
 */
#define write_xr(num,val) {outb(0x3D6, num);outb(0x3D7, val);}
#define read_xr(num,var) {outb(0x3D6, num);var=inb(0x3D7);}
#define write_fr(num,val) {outb(0x3D0, num);outb(0x3D1, val);}
#define read_fr(num,var) {outb(0x3D0, num);var=inb(0x3D1);}

/* 
 * Definitions for IO access to 32 bit ports
 */
extern int ctReg32MMIO[];
extern int ctReg32HiQV[];
#define MR(x) ctReg32MMIO[x]
#define BR(x) ctReg32HiQV[x]

extern unsigned int CHIPS_ExtPorts32[];
#define DR(x) CHIPS_ExtPorts32[x] 

/*
 * Forward definitions for the functions that make up the driver.    See
 * the definitions of these functions for the real scoop.
 */

/* ct_accel.c */
extern void ctAccelInit(void);
/* ct_accelhi.c */
extern void ctHiQVAccelInit(void);
/* ct_accelmm.c */
extern void ctMMIOAccelInit(void);
/* ct_alloc.c */
extern int ctInitializeAllocator(void);
extern int ctAllocate(int, unsigned int);
extern void ctFree(int);
/* ct_config.c */
extern void ctConfig(void);
/* ct_cursor.c */
extern Bool CHIPSCursorInit(char *, ScreenPtr);
extern void CHIPSRestoreCursor(ScreenPtr);
extern void CHIPSWarpCursor(ScreenPtr, int, int);
extern void CHIPSQueryBestSize(int, unsigned short *, unsigned short *, ScreenPtr);
/* ct_driver.c */
extern void ModuleInit(pointer *, INT32 *);
extern int ctGetHWClock(unsigned char);
extern int ctVideoMode(int, int, int, int);
/* ct_pci.c */
extern int ctPCIMemBase(Bool);
extern int ctPCIChipset(void);

/* Bank select functions. */
extern void CHIPSSetRead(int);
extern void CHIPSSetWrite(int);
extern void CHIPSSetReadWrite(int);
extern void CHIPSWINSetRead(int);
extern void CHIPSWINSetWrite(int);
extern void CHIPSWINSetReadWrite(int);
extern void CHIPSHiQVSetRead(int);
extern void CHIPSHiQVSetWrite(int);
extern void CHIPSHiQVSetReadWrite(int);

/* Bank select functions for 1 and 4bpp */
extern void CHIPSSetReadPlanar(int);
extern void CHIPSSetWritePlanar(int);
extern void CHIPSSetReadWritePlanar(int);
extern void CHIPSWINSetReadPlanar(int);
extern void CHIPSWINSetWritePlanar(int);
extern void CHIPSWINSetReadWritePlanar(int);
extern void CHIPSHiQVSetReadPlanar(int);
extern void CHIPSHiQVSetWritePlanar(int);
extern void CHIPSHiQVSetReadWritePlanar(int);

/* in ct_cursor.c */
extern Bool  CHIPSInitCursor();

#define MMIOmeml(x) *(unsigned int *)(ctMMIOBase + (x))
#define MMIOmemw(x) *(unsigned short *)(ctMMIOBase + (x))

/* To aid debugging of 32 bit register access we make the following defines */
#if defined(DEBUG) & defined(CT_HW_DEBUG)
extern void ctHWDebug(int);
#define HW_DEBUG(x) ctHWDebug((x))
#else
#define HW_DEBUG(x)
#endif

/* A generalised blitter wait function for use in compiled once code. This
 * might be need in the CHIPSSave, CHIPSRestore and HW cursor functions with
 * changes to XAA introduced in 32Ar */
#define ctBLTWAITGENERAL { \
    if (ctUseMMIO) { \
	if (ctisHiQV32) { \
	    outb(0x3D6,0x20); {int timeout; \
        	timeout = 0; \
		for (;;) { \
		    if (!(inb(0x3D7)&0x1)) break; \
		    timeout++; \
		    if (timeout == 10000000) { \
			unsigned char tmp; \
			ErrorF("CHIPS: BitBlt Engine Timeout\n"); \
			tmp = inb(0x3D7); \
			outb(0x3D7, ((tmp & 0xFD) | 0x2)); \
			break; \
		    } \
		} \
	    } \
	} else { \
	   while(*(volatile unsigned int *)(ctMMIOBase + MR(0x4)) & \
   		0x00100000){}; \
	} \
    } else { \
	while(inw(DR(0x4)+2)&0x10){}; \
    } \
}

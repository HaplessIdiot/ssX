/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/s3virge/s3v.h,v 1.1 1998/11/01 12:36:00 dawes Exp $ */

#ifndef _S3V_H
#define _S3V_H

/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

/* All drivers need this */
#include "xf86_ansic.h"

/* This is used for module versioning */
/*#include "xf86Version.h"*/

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

#include "vgaHW.h"

#ifndef _S3V_VGAHWMMIO_H
#define _S3V_VGAHWMMIO_H
#define INREG8(addr) *(volatile CARD8 *)(hwp->MMIOBase + hwp->MMIOOffset + (addr))
#define INREG16(addr) *(volatile CARD16 *)(hwp->MMIOBase + hwp->MMIOOffset + (addr))
#define INREG(addr) *(volatile CARD32 *)(hwp->MMIOBase + hwp->MMIOOffset + (addr))
#define OUTREG8(addr, val) *(volatile CARD8 *)(hwp->MMIOBase + hwp->MMIOOffset + (addr)) = (val)
#define OUTREG16(addr, val) *(volatile CARD16 *)(hwp->MMIOBase + hwp->MMIOOffset + (addr)) = (val)
#define OUTREG(addr, val) *(volatile CARD32 *)(hwp->MMIOBase + hwp->MMIOOffset + (addr)) = (val)
/* End of copy ... {KF}*/
#endif

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

#if 0
/* Drivers using the mi banking wrapper need this */
#include "mibank.h"
#endif

/* All drivers using the mi colormap manipulation need this */
#include "micmap.h"

/* Drivers using cfb need this */

#define PSZ 8
#include "cfb.h"
#undef PSZ

/* Drivers supporting bpp 16, 24 or 32 with cfb need these */

#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"

/* Drivers using the XAA interface ... */
#include "xaa.h"


/*********************************************/
/* locals */

/* Some S3 ViRGE structs */
#include "newmmio.h"

/* More ViRGE defines */
#include "regs3v.h"

/*********************************************/


/* PCI data */
#define PCI_S3_VENDOR_ID	0x5333
#define PCI_ViRGE		0x5631
#define PCI_ViRGE_VX		0x883D
#define PCI_ViRGE_DXGX 		0x8A01
#define PCI_ViRGE_GX2 		0x8A10
#define PCI_ViRGE_MX		0x8C01

/* Chip tags */
#define S3_UNKNOWN		 0
#define S3_ViRGE		 1
#define S3_ViRGE_VX		 2
#define S3_ViRGE_DXGX		 3
#define S3_ViRGE_GX2		 4
#define S3_ViRGE_MX		 5

	      
/* Driver data structure; this should contain all needed info for a mode */
/* used to be in s3v_driver.h for pre 4.0 */
typedef struct {      
	/* Want to use MMIO only, need to replace vgaHWRec */
   /* vgaHWRec std; */
   unsigned char SR10, SR11, SR12, SR13, SR15, SR18; /* SR9-SR1C, ext seq. */
   unsigned char Clock;
   unsigned char s3DacRegs[0x101];
   unsigned char CR31, CR33, CR34, CR36, CR3A, CR3B, CR3C;
   unsigned char CR42, CR43;
   unsigned char CR51, CR53, CR54, CR58, CR5D, CR5E;
   unsigned char CR63, CR65, CR66, CR67, CR68, CR69, CR6D; /* Video attrib. */
   unsigned char CR86;
   unsigned char CR90;
   unsigned char ColorStack[8]; /* S3 hw cursor color stack CR4A/CR4B */
   unsigned int  STREAMS[22];   /* Streams regs */
   unsigned int  MMPR0, MMPR1, MMPR2, MMPR3;   /* MIU regs */
} S3VRegRec, *S3VRegPtr;


/* S3VRec  original from tseng */
typedef struct {
    /* ViRGE specifics -start- */   
    					/* Is STREAMS processor needed for */
					/* this mode? */
    Bool 		NeedSTREAMS;
    					/* Is STREAMS running now ? */
    Bool 		STREAMSRunning;
    					/* Compatibility variables */
    int vgaCRIndex, vgaCRReg;
    int Width, Bpp,Bpl, ScissB;   
    					/* XAA */
    unsigned PlaneMask;
    int bltbug_width1, bltbug_width2;
    					/* Flag used by MapMem/UnMapMem */
					/* to determine if matched pair */
					/* is called */
    /*Bool		MemMapped;*/
    					/* In units as noted, set in PreInit */
    int			videoRambytes;
    int			videoRamKbytes;
    					/* In Kbytes, set in PreInit */
    int			MemOffScreen;
    /*unsigned char *	PciMapBase;*/
    					/* Holds the virtual memory address */
					/* returned when the MMIO registers */
					/* are mapped with xf86MapPciMem    */
    unsigned char *	MapBase;
    					/* MapBase + 0x8000 */
    unsigned char *	IOBase;
    					/* Same as MapBase, except framebuffer*/
    unsigned char *	FBBase;
    					/* Current visual FB starting location */
    unsigned char *	FBStart;
    				     	/* Saved CR53 value */
    unsigned char	EnableMmioCR53;
    					/* Extended reg unlock storage */
    unsigned char	CR38,CR39,CR40;
    					/* Flag indicating if vgaHWMapMem was */
					/* used successfully for this screen */
    Bool		PrimaryVidMapped;
    					/* Clock value */
    int			dacSpeedBpp;
    int			minClock;
    					/* Maximum clock for present bpp */
    int			maxClock;
    int			HorizScaleFactor;
    Bool		bankedMono;
    					/* Memory Clock */
    int 		MCLK;
    					/* ViRGE options -start- */
					
    					/* Enable PCI burst mode for reads? */
    Bool 		pci_burst_on;
    					/* Diasable PCI retries */
    Bool		NoPCIRetry;
    					/* Adjust fifo for acceleration? */
    Bool 		fifo_conservative;
    Bool 		fifo_moderate;
    Bool 		fifo_aggressive;
    					/* Set memory options */
    Bool 		slow_edodram;
    Bool 		fast_dram;
    Bool 		fpm_vram;
    					/* Disable Acceleration */
    Bool		NoAccel;
    					/* Adjust memory ras precharge */ 
					/* timing */
    Bool 		early_ras_precharge;
    Bool 		late_ras_precharge;
    					/* ViRGE options -end- */
    /* ViRGE specifics -end- */
    
    /* Used by ViRGE driver, but generic */
    
    CloseScreenProcPtr	CloseScreen;
    					/* XAA info Rec */
    XAAInfoRecPtr 	AccelInfoRec;
    
    /* Used by ViRGE driver, but generic -end- */
    
    
    
    
    /* we'll put variables that we want to access _fast_ at the beginning (just a hunch) */
    unsigned char cache_SegSelL, cache_SegSelH;  /* for tseng_bank.c */
    int Bytesperpixel;		       /* a shorthand for the XAA code */
    Bool need_wait_acl;		       /* always need a full "WAIT" for ACL finish */
    int line_width;		       /* framebuffer width in bytes per scanline */
    int planemask_mask;		       /* mask for active bits in planemask */
    int neg_x_pixel_offset;
    int powerPerPixel;		       /* power-of-2 version of bytesperpixel */
    /* normal stuff starts here */
    pciVideoPtr PciInfo;
    PCITAG PciTag;
    int			Chipset;
    int			ChipRev;
    int Save_Divide;
    Bool UsePCIRetry;		       /* Do we use PCI-retry or busy-waiting */
    Bool UseAccel;		       /* Do we use the XAA acceleration architecture */
    Bool HWCursor;		       /* Do we use the hardware cursor (if supported) */
    Bool Linmem_1meg;		       /* Is this a card limited to 1Mb of linear memory */
    Bool UseLinMem;
    Bool SlowDram;
    Bool FastDram;
    Bool MedDram;
    Bool SetPCIBurst;
    Bool PCIBurst;
    Bool SetW32Interleave;
    Bool W32Interleave;
    Bool ShowCache;
    Bool Legend;		       /* Sigma Legend clock select method */
    Bool NoClockchip;		       /* disable clockchip programming clockchip (=use set-of-clocks) */
  #if 0
    TsengRegRec SavedReg;	       /* saved Tseng registers at server start */
    TsengRegRec ModeReg;
  #endif
    S3VRegRec SavedReg;	       	       /* saved Tseng registers at server start */
    S3VRegRec ModeReg;
  #if 0
    unsigned long icd2061_dwv;	       /* To hold the clock data between Init and Restore */
    t_tseng_bus Bustype;	       /* W32 bus type (currently used for lin mem on W32i) */
    t_tseng_type ChipType;	       /* "Chipset" causes confusion with pScrn->chipset */
    int ChipRev;
    CARD32 LinFbAddress;
    unsigned char *FbBase;
    CARD32 LinFbAddressMask;
    long FbMapSize;
    miBankInfoRec BankInfo;
    CARD32 IOAddress;		       /* PCI config space base address for ET6000 */
    CARD32 MMIOBase;
    int MinClock;
    int MaxClock;
    ClockRangePtr clockRange[2];
    TsengDacInfoRec DacInfo;
    TsengMClkInfoRec MClkInfo;
    t_clockchip_type ClockChip;
    CloseScreenProcPtr CloseScreen;
    int save_divide;
    XAAInfoRecPtr AccelInfoRec;
    XAACursorInfoPtr CursorInfoRec;
    CARD32 AccelColorBufferOffset;     /* offset in video memory where FG and BG colors will be stored */
    CARD32 AccelColorExpandBufferOffsets[3];   /* offset in video memory for ColorExpand buffers */
    unsigned char * XAAColorExpandBuffers[3];  /* pointers to colorexpand buffers */
    CARD32 AccelImageWriteBufferOffsets[2];    /* offset in video memory for ImageWrite Buffers */
    unsigned char * XAAScanlineImageWriteBuffers[2];   /* pointers to ImageWrite Buffers */
    CARD32 HWCursorBufferOffset;
    unsigned char *XAAHWCursorBuffer;
    unsigned char save_ExtCRTC36;      /* save CRTC 0x36 during sequencer resets */
  #endif
    
} S3VRec, *S3VPtr;

#define S3VPTR(p) ((S3VPtr)((p)->driverPrivate))
      

/* #define S3V_DEBUG */
		 
#ifdef S3V_DEBUG
#define PVERB5(arg) ErrorF(arg)
#define VERBLEV	1
#else
#define PVERB5(arg) xf86ErrorFVerb(2, arg)
#define VERBLEV	2
#endif


/******************* s3v_accel ****************************/

/*static*/ void S3VGEReset(ScrnInfoPtr pScrn);







/******************* compat. macros **********************/

#ifdef COMPMACROS3X
#include "s3v_comp.h"
#endif


/******************* regs3v *******************************/

/* #ifndef MetroLink */ 
#if !defined (MetroLink) && !defined (VertDebug)
#define VerticalRetraceWait() do { \
   outb(vgaCRIndex, 0x17); \
   if ( inb(vgaCRReg) & 0x80 ) { \
       while ((inb(vgaIOBase + 0x0A) & 0x08) == 0x00) ; \
       while ((inb(vgaIOBase + 0x0A) & 0x08) == 0x08) ; \
       while ((inb(vgaIOBase + 0x0A) & 0x08) == 0x00) ; \
       }\
} while (0)
/*
#define VerticalRetraceWait() do { \
   outb(vgaCRIndex, 0x17); \
   if ( inb(vgaCRReg) & 0x80 ) { \
       while ((inb( 0x3CA) & 0x08) == 0x00) ; \
       while ((inb( 0x3CA) & 0x08) == 0x08) ; \
       while ((inb( 0x3CA) & 0x08) == 0x00) ; \
       }\
} while (0)
*/

#else
#define SPIN_LIMIT 1000000
#define VerticalRetraceWait() do { \
   outb(vgaCRIndex, 0x17); \
   if ( inb(vgaCRReg) & 0x80 ) { \
	volatile unsigned long _spin_me; \
	for (_spin_me = 0; \
	 ((inb(vgaIOBase + 0x0A) & 0x08) == 0x00) && _spin_me <= SPIN_LIMIT; \
	 _spin_me++) ; \
	if (_spin_me > SPIN_LIMIT) \
	    ErrorF("s3v: warning: VerticalRetraceWait timed out.\n"); \
	for (_spin_me = 0; \
	 ((inb(vgaIOBase + 0x0A) & 0x08) == 0x08) && _spin_me <= SPIN_LIMIT; \
	 _spin_me++) ; \
	if (_spin_me > SPIN_LIMIT) \
	    ErrorF("s3v: warning: VerticalRetraceWait timed out.\n"); \
	for (_spin_me = 0; \
	 ((inb(vgaIOBase + 0x0A) & 0x08) == 0x00) && _spin_me <= SPIN_LIMIT; \
	 _spin_me++) ; \
	if (_spin_me > SPIN_LIMIT) \
	    ErrorF("s3v: warning: VerticalRetraceWait timed out.\n"); \
   } \
} while (0)
#endif

#if 0

/* Wait until GP is idle and queue is empty */
#define	WaitIdleEmpty()  do { mem_barrier(); while ((IN_SUBSYS_STAT() & 0x3f00) != 0x3000); } while (0)

/* Wait until GP is idle */
#define WaitIdle()       do { mem_barrier(); while (!(IN_SUBSYS_STAT() & 0x2000)); } while (0)

/* Wait until Command FIFO is empty */
#define WaitCommandEmpty()       do { mem_barrier(); while (!(((((mmtr)s3vMmioMem)->subsys_regs.regs.adv_func_cntl)) & 0x200)); } while (0)

#endif

/*********************************************************/


/* Various defines which are used to pass flags between the Setup and 
 * Subsequent functions. 
 */

#define NO_MONO_FILL      0x00
#define NEED_MONO_FILL    0x01
#define MONO_TRANSPARENCY 0x02


/* This function was taken from accel/s3v.h. It adjusts the width
 * of transfers for mono images to works around some bugs.
 */

static __inline__ int S3VCheckLSPN(S3VPtr ps3v, int w, int dir)
{
   int lspn = (w * ps3v->Bpp) & 63;  /* scanline width in bytes modulo 64*/

   if (ps3v->Bpp == 1) {
      if (lspn <= 8*1)
	 w += 16;
      else if (lspn <= 16*1)
	 w += 8;
   } else if (ps3v->Bpp == 2) {
      if (lspn <= 4*2)
	 w += 8;
      else if (lspn <= 8*2)
	 w += 4;
   } else {  /* ps3v->Bpp == 3 */
      if (lspn <= 3*3) 
	 w += 6;
      else if (lspn <= 6*3)
	 w += 3;
   }
   if (dir && w >= ps3v->bltbug_width1 && w <= ps3v->bltbug_width2) {
      w = ps3v->bltbug_width2 + 1;
   }

   return w;
}

/* And this adjusts color bitblts widths to work around GE bugs */

static __inline__ int S3VCheckBltWidth(S3VPtr ps3v, int w)
{
   if (w >= ps3v->bltbug_width1 && w <= ps3v->bltbug_width2) {
      w = ps3v->bltbug_width2 + 1;
   }
   return w;
}

/* This next function determines if the Source operand is present in the
 * given ROP. The rule is that both the lower and upper nibble of the rop
 * have to be neither 0x00, 0x05, 0x0a or 0x0f. If a CPU-Screen blit is done
 * with a ROP which does not contain the source, the virge will hang when
 * data is written to the image transfer area. 
 */

static __inline__ Bool S3VROPHasSrc(int shifted_rop)
{
    int rop = (shifted_rop & (0xff << 17)) >> 17;

    if ((((rop & 0x0f) == 0x0a) | ((rop & 0x0f) == 0x0f) 
        | ((rop & 0x0f) == 0x05) | ((rop & 0x0f) == 0x00)) &
       (((rop & 0xf0) == 0xa0) | ((rop & 0xf0) == 0xf0) 
        | ((rop & 0xf0) == 0x50) | ((rop & 0xf0) == 0x00)))
            return FALSE;
    else 
            return TRUE;
}

/* This next function determines if the Destination operand is present in the
 * given ROP. The rule is that both the lower and upper nibble of the rop
 * have to be neither 0x00, 0x03, 0x0c or 0x0f. 
 */

static __inline__ Bool S3VROPHasDst(int shifted_rop)
{
    int rop = (shifted_rop & (0xff << 17)) >> 17;

    if ((((rop & 0x0f) == 0x0c) | ((rop & 0x0f) == 0x0f) 
        | ((rop & 0x0f) == 0x03) | ((rop & 0x0f) == 0x00)) &
       (((rop & 0xf0) == 0xc0) | ((rop & 0xf0) == 0xf0) 
        | ((rop & 0xf0) == 0x30) | ((rop & 0xf0) == 0x00)))
            return FALSE;
    else 
            return TRUE;
}




#endif  /*_S3V_H*/


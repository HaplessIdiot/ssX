/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/s3/s3fbinit.c,v $ */
/*
 *
 * Copyright 1995-1997 The XFree86 Project, Inc.
 *
 */
#define USE_VGAHWINIT

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "site.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "vga.h"

#include "s3.h"
#include "s3reg.h"
#include "s3Bt485.h"
#define S3_SERVER
#include "Ti302X.h"
#include "IBMRGB.h"
#define XCONFIG_FLAGS_ONLY 
#include "xf86_Config.h"


extern int nonMuxMaxClock;
extern int pixMuxMinClock;

extern vgaHWCursorRec vgaHWCursor;


/*
 *   S3FbInit --
 *
 *	This is typical of what is done in the mga server. ie. the
 *	registers are memory mapped, the linear base is determined
 *	and XAA and hardware cursor are set up.  Note that X -probeonly
 *	will get into this function so hardware initialization is not
 *	possible this early on.
 *	  This function is different in that some stuff seemingly more
 *	suitable for the Probe function is in here, but that is only 
 *	because it had to wait until after the modelines were validated
 *	or the width was determined etc...
 *
 *				MArk.
 *
 */


void S3FbInit(void)
{
   int i;

#ifdef S3_DEBUG
    ErrorF("In S3FbInit()\n");
#endif
    
      /**********************************************************************\
      | Adjust vga256InfoRec.clock[] when not using a programable clock chip |
      \**********************************************************************/
	/* This had to wait until after the clocks were setup (MArk). */

   if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions)) {
      Bool clocksChanged = FALSE;
      Bool numClocksChanged = FALSE;
      int newNumClocks = vga256InfoRec.clocks;


      /* New rules if we don't have a programmable clock ?*/
      if (S3_864_SERIES(s3ChipId))
	 nonMuxMaxClock = 95000;
      else if (S3_805_I_SERIES(s3ChipId))
	 nonMuxMaxClock = 90000;  

      /* Clock Modifications for some RAMDACs */
      for (i = 0; i < vga256InfoRec.clocks; i++) {
	 switch(s3RamdacType) {
	  case ATT20C498_DAC:
	  case ATT22C498_DAC:
	  case STG1700_DAC:
	  case STG1703_DAC:
	    switch (s3Bpp) {
	     case 1:
	        if (!numClocksChanged) {
		  newNumClocks = 32;
		  numClocksChanged = TRUE;
		  clocksChanged = TRUE;
		  for(i = vga256InfoRec.clocks; i < newNumClocks; i++)
		     vga256InfoRec.clock[i] = 0;  
		  if (vga256InfoRec.clocks > 16) 
		     vga256InfoRec.clocks = 16;
	        }
	        if (vga256InfoRec.clock[i] * 2 > pixMuxMinClock &&
		   vga256InfoRec.clock[i] * 2 <= vga256InfoRec.dacSpeed)
		  vga256InfoRec.clock[i + 16] = vga256InfoRec.clock[i] * 2;
	        else
		  vga256InfoRec.clock[i + 16] = 0;
	        if (vga256InfoRec.clock[i] > nonMuxMaxClock)
		  vga256InfoRec.clock[i] = 0;
	        break;
	     case 4:
	        vga256InfoRec.clock[i] /= 2;
	        clocksChanged = TRUE;
	        break;
	    }
	    break;
	  case ATT20C490_DAC:
	  case SS2410_DAC:
	  case SC1148x_DAC:
	  case TI3020_DAC:
	  case SC15025_DAC:
	    if (s3Bpp > 1) {
	       vga256InfoRec.clock[i] /= s3Bpp;
	       clocksChanged = TRUE;
	    }
 	    break;
 	  default:
	    break;
	 }
      }
      if (numClocksChanged)
	 vga256InfoRec.clocks = newNumClocks;

      if (xf86Verbose && clocksChanged) {
	 ErrorF("%s %s: Effective pixel clocks available for depth %d:\n",
		XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.depth);
	 for (i = 0; i < vga256InfoRec.clocks; i++) {
	    if ((i % 8) == 0) {
	       if (i != 0)
		  ErrorF("\n");
               ErrorF("%s %s: pixel clocks:", XCONFIG_PROBED, 
						vga256InfoRec.name);
	    }
	    ErrorF(" %6.2f", (double)vga256InfoRec.clock[i] / 1000.0);
         }
         ErrorF("\n");
      }
   }

   /* At this point, the vga256InfoRec.clock[] values are pixel clocks */

	/*******************************\
	|	Locate Cursor Storage	|
	\*******************************/


   if (OFLG_ISSET(OPTION_BT485_CURS, &vga256InfoRec.options) ||
       OFLG_ISSET(OPTION_TI3020_CURS, &vga256InfoRec.options) ||
       OFLG_ISSET(OPTION_TI3026_CURS, &vga256InfoRec.options) ||
       OFLG_ISSET(OPTION_IBMRGB_CURS, &vga256InfoRec.options) ||
       OFLG_ISSET(OPTION_SW_CURSOR, &vga256InfoRec.options)) {
      		s3CursorStartY = vga256InfoRec.virtualY;
   		s3CursorLines = 0;
   } else {
      int st_addr, CursorStartX;

      st_addr = (vga256InfoRec.virtualY * s3BppDisplayWidth + 1023) & ~1023;
      CursorStartX = st_addr % s3BppDisplayWidth;
      s3CursorStartY = st_addr / s3BppDisplayWidth;
      s3CursorLines = ((CursorStartX + 1023) / s3BppDisplayWidth) + 1;
   }


  /*
    * Reduce the videoRam value if necessary to prevent Y coords exceeding
    * the 12-bit (4096) limit when small display widths are used on cards
    * with a lot of memory
    */
/* should this go somewhere else? (MArk) */

   if (vga256InfoRec.videoRam * 1024 / s3BppDisplayWidth > 4096) {
      vga256InfoRec.videoRam = s3BppDisplayWidth * 4096 / 1024;
      ErrorF("%s %s: videoram usage reduced to %dk to avoid co-ord overflow\n",
	     XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.videoRam);
   }
  

   if (S3_964_SERIES(s3ChipId) &&
	  !OFLG_ISSET(OPTION_NUMBER_NINE, &vga256InfoRec.options))
         s3SAM256 = 0x40;
   else if ((OFLG_ISSET(OPTION_SPEA_MERCURY, &vga256InfoRec.options) &&
               S3_928_ONLY(s3ChipId)) ||
	       OFLG_ISSET(OPTION_STB_PEGASUS, &vga256InfoRec.options))
         s3SAM256 = 0x80; /* set 6 MCLK cycles for R/W time on Mercury */
   else
         s3SAM256 = 0x00;


    if(xf86Verbose)
      ErrorF("%s %s: Framebuffer aligned on %i pixel width\n",
	 	XCONFIG_PROBED, vga256InfoRec.name, s3DisplayWidth);



	/* I should probably stick this somewhere else ? */
   s3ScissB = ((vga256InfoRec.videoRam * 1024) / s3BppDisplayWidth) - 1;
   s3ScissR = s3DisplayWidth - 1;



	/*******************************\
	|	Set Linear Base	 	|
	\*******************************/

    s3BankSize = 0x10000;
    s3LinApOpt = 0x14;


      /* determine if we are linear addressing */
 
   if (S3_801_928_SERIES(s3ChipId) && s3Localbus && xf86LinearVidMem()
	  && !OFLG_ISSET(OPTION_NOLINEAR_MODE, &vga256InfoRec.options)
	  && !OFLG_ISSET(OPTION_NO_MEM_ACCESS, &vga256InfoRec.options)) {
	 if (S3_x64_SERIES(s3ChipId)) { 
	    if (vga256InfoRec.MemBase) {
#ifdef S3_NEWMMIO
	       if (vga256InfoRec.MemBase & 0x3ffffff) {
		  ErrorF("%s %s: base address not correctly aligned to 64MB\n",
			 XCONFIG_PROBED, vga256InfoRec.name);
		  ErrorF("\t\tbase address changed from 0x%08lx to 0x%08lx\n",
		   vga256InfoRec.MemBase, vga256InfoRec.MemBase & ~0x3ffffff);
		  vga256InfoRec.MemBase &= ~0x3ffffff;
	       }
#endif /* S3_NEWMMIO */
	       s3InfoRec.ChipLinearBase = vga256InfoRec.MemBase;
	    }
#ifdef S3_NEWMMIO
	    else {
	       unsigned long OrigBase;

	       outb(vgaCRIndex, 0x59);
	       s3InfoRec.ChipLinearBase = inb(vgaCRReg) << 24;
	       outb(vgaCRIndex, 0x5a);
	       s3InfoRec.ChipLinearBase |= inb(vgaCRReg) << 16;
	       OrigBase = s3InfoRec.ChipLinearBase;
	       s3InfoRec.ChipLinearBase &= 0xfc000000;
	       if (!s3InfoRec.ChipLinearBase || 
			s3InfoRec.ChipLinearBase != OrigBase) {
		  /* the aligned address may clash with other devices,
		     so use a pretty random base address... */
		  s3InfoRec.ChipLinearBase = 0xb4000000;  /* last resort */
		  ErrorF("%s %s: PCI: base address not correctly aligned\n",
			 XCONFIG_PROBED, vga256InfoRec.name);
		  ErrorF("\t\tbase address changed from 0x%08lx to 0x%08lx\n",
			 OrigBase, s3InfoRec.ChipLinearBase);
	       }
	    }
#else
	    else 
	       s3InfoRec.ChipLinearBase = 0xf3000000; 
#endif /* S3_NEWMMIO */
	 } else {  /* not x64 series */
#ifdef PC98_PWLB
	    if (pc98BoardType == PWLB)
	       s3InfoRec.ChipLinearBase = 0x0;
	    else
#endif
	    s3InfoRec.ChipLinearBase = 0x03000000;
	 }
   } 


   /* s3Port59/s3Port5A need to be checked/initialized
	 before s3Init() is called the first time */
   if(s3InfoRec.ChipLinearBase) {
        s3InfoRec.ChipUseLinearAddressing = TRUE;   
    	s3Port59 = s3InfoRec.ChipLinearBase >> 24;
   	s3Port5A = s3InfoRec.ChipLinearBase >> 16;
   } else {   /* this is what it worked out to in the S3 server. If
	it really isn't important what these are when you aren't linear
	addressing, then remove this. (MArk) */
    	s3Port59 = 0xA0000 >> 24;
   	s3Port5A = 0xA0000 >> 16;
   }

   if(xf86Verbose) {
  	if(s3InfoRec.ChipUseLinearAddressing) 
	    ErrorF("%s %s: Linear mapped framebuffer at 0x%08lx\n",
		 XCONFIG_PROBED, vga256InfoRec.name,s3InfoRec.ChipLinearBase);
	else
	    ErrorF("%s %s: Bank switching... Bank size %i\n",
			 XCONFIG_PROBED, vga256InfoRec.name,s3BankSize);
    }


	/***************************************\
	|	Memory map the registers 	|
	\***************************************/

#if defined(S3_MMIO) || defined(S3_NEWMMIO)

/*  I'm a little intimidated by this stuff.  Harald? */

   s3MmioMem = xf86MapVidMem(Your ad here);

#endif
	/*******************************\
	|	Setup XAA	 	|
	\*******************************/

    if(!OFLG_ISSET(OPTION_NOACCEL, &vga256InfoRec.options)) 
	S3AccelInit();
    

	/*******************************\
      	|	Hardware Cursor		|
	\*******************************/

/* only these cursors work yet (MArk) */

    if( OFLG_ISSET(OPTION_BT485_CURS, &vga256InfoRec.options) ||
        OFLG_ISSET(OPTION_TI3020_CURS, &vga256InfoRec.options) ||
	OFLG_ISSET(OPTION_TI3026_CURS, &vga256InfoRec.options) ||
	OFLG_ISSET(OPTION_IBMRGB_CURS, &vga256InfoRec.options)) {
#ifdef S3_DEBUG
   ErrorF("Using Hardware Cursor\n");
#endif

    	vgaHWCursor.Initialized = TRUE;
   	vgaHWCursor.Init = S3CursorInit;
   	vgaHWCursor.Restore = S3RestoreCursor;
    	vgaHWCursor.Warp = S3WarpCursor;
    	vgaHWCursor.QueryBestSize = S3QueryBestSize;
    }
    

}


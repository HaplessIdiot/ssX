
/* $XConsortium: s3misc.c,v 1.1 94/03/28 21:16:11 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3misc.c,v 3.9 1994/08/20 07:34:20 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Thomas Roell makes no
 * representations about the suitability of this software for any purpose. It
 * is provided "as is" without express or implied warranty.
 * 
 * THOMAS ROELL AND KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THOMAS ROELL OR KEVIN E. MARTIN BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 * 
 * Rewritten for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)
 * 
 * Header: /home/src/xfree86/mit/server/ddx/xf86/accel/s3/RCS/s3.c,v 2.0
 * 1993/02/22 05:58:13 jon Exp
 * 
 * Modified by Amancio Hasty and Jon Tombs
 * 
 * Id: s3.c,v 2.6 1993/08/09 06:17:57 jon Exp jon
 */


#include "cfb.h"
#include "cfb16.h"
#include "cfb32.h"
#include "pixmapstr.h"
#include "fontstruct.h"
#include "s3.h"
#include "regs3.h"
#include "xf86_HWlib.h"
#include "ICD2061A.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "s3linear.h"

extern char s3Mbanks;
extern Bool s3Mmio928;

extern miPointerScreenFuncRec xf86PointerScreenFuncs;

static Bool s3TryAddress();
extern ScreenPtr s3savepScreen;
static PixmapPtr ppix = NULL;
extern Bool  s3Localbus;
extern Bool  s3LinearAperture;
extern int s3BankSize;
extern int s3DisplayWidth;
extern pointer vgaBase;
extern pointer s3VideoMem;
extern unsigned char s3Port59;
extern unsigned char s3Port5A;
extern unsigned char s3Port31;
extern Bool s3PowerSaver;

extern Bool xf86Exiting, xf86Resetting, xf86ProbeFailed, xf86Verbose;

static Bool AlreadyInited = FALSE;
static Bool s3ModeSwitched = FALSE;


/*
 * s3Initialize -- Attempt to find and initialize a VGA framebuffer Most of
 * the elements of the ScreenRec are filled in.  The video is enabled for the
 * frame buffer...
 */

Bool
s3Initialize(scr_index, pScreen, argc, argv)
     int   scr_index;		/* The index of pScreen in the ScreenInfo */
     ScreenPtr pScreen;		/* The Screen to initialize */
     int   argc;		/* The number of the Server's arguments. */
     char **argv;		/* The arguments themselves. Don't change! */
{
   int displayResolution = 75;	/* default to 75dpi */
   extern int monitorResolution;

   s3Unlock();	  /* for restarts */
   
   /* for clips */
   s3ScissB = ((s3InfoRec.videoRam * 1024) / s3BppDisplayWidth) - 1;

 /*
  * Initialize the screen, saving the original state for Save/Restore
  * function
  */

   if (serverGeneration == 1) {
      s3BankSize = 0x10000;
      s3LinApOpt = 0x14;

      /* First, map the vga window -- it is always required */
      vgaBase = xf86MapVidMem(scr_index, VGA_REGION, (pointer)0xA0000,
			      s3BankSize);
      s3Init(s3InfoRec.modes);
      /*
       * The meaning of NOLINEAR_MODE has changed a little, and was
       * used inconsistently here.  It now means don't attempt to map
       * the framebuffer at high memory.  Some code here implied that at
       * one stage it meant it was OK to use high memory, but only use a 64k
       * aperture.
       */
      if (xf86LinearVidMem() &&
	  !OFLG_ISSET(OPTION_NOLINEAR_MODE, &s3InfoRec.options)) {
	 /* Now, see if we can map a high buffer */
	 if (s3Localbus && !S3_924_ONLY(s3ChipId) &&
	     !OFLG_ISSET(OPTION_NO_MEM_ACCESS, &s3InfoRec.options)) {
	    long i;
	    long *poker;
	    unsigned long pVal;
	    unsigned char reg53tmp = 0;
	    Bool CachedFrameBuffer = FALSE;

	    pVal = 0x12345678;

	    s3InitEnvironment();
	    s3ImageWriteNoMem(0, 0, 4, 1, (char *) &pVal, 4, 0, 0,
			      (short) s3alu[GXcopy], ~0);	 

	    if (S3_801_928_SERIES (s3ChipId)) {
	       int j;

	       if (s3Mmio928) { /* Due to S3 bugs we must disable mmio */
		  outb(vgaCRIndex, 0x53);
		  reg53tmp = inb(vgaCRReg);
		  outb(vgaCRReg, reg53tmp & ~0x10); /* save parallel bit */
	       }
	       /* begin 801 sequence for going in to linear mode */
	       outb (vgaCRIndex, 0x40);
	       /* enable fast write buffer and disable 8514/a mode */
	       j = (s3Port40 & 0xf6) | 0x0a;
	       outb (vgaCRReg, (unsigned char) j);
	       outb(vgaCRIndex, 0x59);
	       if (S3_x64_SERIES(s3ChipId)) 
		  if (s3InfoRec.MemBase != 0) 
		     outb(vgaCRReg, 0x03 | (s3InfoRec.MemBase>>24));
		  else
		     outb(vgaCRReg, 0x03 | 0xf0);
	       else
		  outb(vgaCRReg, 0x03);
	       outb(vgaCRIndex, 0x5a);
	       outb(vgaCRReg, 0x00);
	       outb (vgaCRIndex, 0x58);
	       if (s3InfoRec.videoRam <= 1024) {
		  s3LinApOpt=0x15;
	       } else if (s3InfoRec.videoRam <= 2048) {
		  s3LinApOpt=0x16;
	       } else {
		  s3LinApOpt=0x17;
	       }
       	       s3BankSize = s3InfoRec.videoRam * 1024;
	       /* go on to linear mode */
	       outb (vgaCRReg, s3LinApOpt | s3SAM256);
	       /* end  801 sequence to go into linear mode */
	    }
	 
	    /*
	     * XXXX This is for debugging only.  It attempts to find
	     * which values of LAW are decoded (see s3TryAddress() below).
	     */
	    if (OFLG_ISSET(OPTION_FB_DEBUG, &s3InfoRec.options)) {
	       for (i = 0xfc; i >= 0; i-=4) {
		  unsigned long addr = (i << 24) + (0x3<<24);

	          s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
				          (pointer)addr, 4096);
	          poker = (long *) s3VideoMem; 
	          s3TryAddress(poker, pVal, addr, 0);
	          xf86UnMapVidMem(scr_index, LINEAR_REGION, s3VideoMem, 4096);
	       }
	    }
		  
	    /*
	     * If a MemBase value was given in the Xconfig, skip the LAW
	     * probe and use the high 6 bits for the hw part of LAW.
	     */
	    if (s3InfoRec.MemBase != 0) {
	       unsigned long addr;

	       /* Software part of LAW is 60MB = 0x3c00000 */
	       addr = (s3InfoRec.MemBase & 0xfc000000) + 0x3c00000;
	       s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
					  (pointer)addr, s3BankSize);
	       outb(vgaCRIndex, 0x5a);
	       outb(vgaCRReg, 0xC0);
	       s3LinearAperture = TRUE;
	       ErrorF("%s %s: Local bus LAW31-26 is %X\n", 
		      XCONFIG_GIVEN, s3InfoRec.name, (addr >> 24) & 0xfc);
	    } else {
	       if (S3_x64_SERIES(s3ChipId)) {
		  /* So far, only tested for the PCI ELSA W2000Pro */
		  unsigned long addr;

	          outb(vgaCRIndex, 0x59);
	          addr = inb(vgaCRReg) << 8;
	          outb(vgaCRIndex, 0x5a);
	          addr |= inb(vgaCRReg);
	          addr <<= 16;
                  if (OFLG_ISSET(OPTION_FB_DEBUG, &s3InfoRec.options)) {
		     ErrorF("Read LAW as 0x%08X \n", addr);
	          }
		  s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
                                             (pointer)addr, s3BankSize);
		  s3LinearAperture = TRUE;
                  ErrorF("%s %s: Local bus LAW is 0x%08lX\n", 
                         XCONFIG_PROBED, s3InfoRec.name, addr);
	       } else {
	          for (i = 0xfc; i >= 0; i-=4) {
		     unsigned long addr;
	    
		     /* Start at LAW(hw) + 48MB */
		     addr = (i << 24) + (0x3<<24);
		     s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
						(pointer)addr, s3BankSize);
		     poker = (long *) s3VideoMem; 

		     if (s3TryAddress(poker, pVal, addr, 1)) {
			/* We found some ram, but is it ours? */
	       
			/* move it up by 12MB to LAW(hw) + 60MB */
			outb(vgaCRIndex, 0x5a);
			outb(vgaCRReg, 0xC0);
			if (!s3TryAddress(poker, pVal, addr, 2)) {
			   addr += (0x0C<<20);
			   xf86UnMapVidMem(scr_index, LINEAR_REGION,
					   s3VideoMem, s3BankSize);
			   s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
						      (pointer)addr,
						      s3BankSize);
		     
			   if (s3TryAddress((long *)s3VideoMem, pVal,
					    addr, 3)) {
			      ErrorF("%s %s: Local bus LAW31-26 is %X\n", 
				     XCONFIG_PROBED, s3InfoRec.name, i);
			      s3LinearAperture = TRUE;
			      break;
			   }
			} else {
			   ErrorF("%s %s: linear framebuffer found, but",
				  XCONFIG_PROBED, s3InfoRec.name);
			   ErrorF(" it appears to be in a cachable\n");
			   ErrorF("\t address range.\n");
			   CachedFrameBuffer = TRUE;
			}
		     }
		     xf86UnMapVidMem(scr_index, LINEAR_REGION, s3VideoMem,
				     s3BankSize);
		  }
	       }
	    }

            /* The UnMap can unmap all mapped regions, so make sure vgaBase is
	       still mapped */
	    xf86MapDisplay(scr_index, VGA_REGION);

	    if (!s3LinearAperture) {
	       ErrorF("%s %s: %s local bus framebuffer -",
		      XCONFIG_PROBED, s3InfoRec.name,
	              (CachedFrameBuffer? "Can't use": "Couldn't find"));
	       ErrorF(" linear access disabled\n");
	       s3BankSize = 0x10000;
	       s3VideoMem = NULL;
	    }
	    if (S3_801_928_SERIES (s3ChipId)) {
	       /* begin 801  sequence to go into enhanced mode */
	       outb (vgaCRIndex, 0x58);
	       outb (vgaCRReg, s3SAM256);
	       outb (vgaCRIndex, 0x40);
	       outb (vgaCRReg, s3Port40);
	       /* end 801 sequence to go into enhanced mode */
	    }
	    if (s3Mmio928) { /* Now re-enable mmio if required */
	       outb(vgaCRIndex, 0x53);
	       outb(vgaCRReg, reg53tmp | 0x10);
	    }
         }
      }

      /* No linear mapping */
      if (!s3VideoMem) {
	 s3VideoMem = vgaBase;
	 /* If using VGA aperture, set it up */
	 if (s3BankSize == 0x10000) {
	    outb(vgaCRIndex, 0x59);
	    outb(vgaCRReg, 0x00);
	    outb(vgaCRIndex, 0x5a);
	    outb(vgaCRReg, 0x0a);
	    s3LinApOpt = 0x14;
	 }
      }
	   
      /* Save CR59, CR5A for future calls to s3Init() */
      outb(vgaCRIndex, 0x59);
      s3Port59 = inb(vgaCRReg);
      outb(vgaCRIndex, 0x5A);
      s3Port5A = inb(vgaCRReg);
   } else
      s3Init(s3InfoRec.modes);

   s3InitEnvironment();
   AlreadyInited = TRUE;

   s3ImageInit();
   xf86InitCache(s3CacheMoveBlock);
   s3FontCache8Init();

   /*
    * Take display resolution from the -dpi flag if specified
    */

   if (monitorResolution)
      displayResolution = monitorResolution;

   if (!s3ScreenInit(pScreen,
		     (pointer) s3VideoMem,
		     s3InfoRec.virtualX, s3InfoRec.virtualY,
		     displayResolution, displayResolution,
		     s3DisplayWidth))
      return (FALSE);

   pScreen->CloseScreen = s3CloseScreen;
   pScreen->SaveScreen = s3SaveScreen;


#if 0
	miDCInitialize (pScreen, &xf86PointerScreenFuncs);
#else
 	switch (s3InfoRec.bitsPerPixel) {
	case 8:
	    pScreen->InstallColormap = s3InstallColormap;
	    pScreen->UninstallColormap = s3UninstallColormap;
	    pScreen->ListInstalledColormaps = s3ListInstalledColormaps;
	    pScreen->StoreColors = s3StoreColors;
	    break;       
	case 16:
        case 32:
	    pScreen->InstallColormap = cfbInstallColormap;
	    pScreen->UninstallColormap = cfbUninstallColormap;
	    pScreen->ListInstalledColormaps = cfbListInstalledColormaps;
	    pScreen->StoreColors = (void (*)())NoopDDA;
	}
	pScreen->QueryBestSize = s3QueryBestSize;
	xf86PointerScreenFuncs.WarpCursor = s3WarpCursor;
	(void)s3CursorInit(0, pScreen);
#endif
   s3savepScreen = pScreen;
   return (cfbCreateDefColormap(pScreen));

}


/*
 * s3EnterLeaveVT -- grab/ungrab the current VT completely.
 */

void
s3EnterLeaveVT(enter, screen_idx)
     Bool  enter;
     int screen_idx;
{
   PixmapPtr pspix;
   ScreenPtr pScreen = s3savepScreen;

   if (!xf86Exiting && !xf86Resetting) {
      /* cfbGetScreenPixmap(pScreen) */
        switch (s3InfoRec.bitsPerPixel) {
        case 8:
            pspix = (PixmapPtr)pScreen->devPrivate;
            break;
        case 16:
            pspix =
                  (PixmapPtr)pScreen->devPrivates[cfb16ScreenPrivateIndex].ptr;
            break;
        case 32:
            pspix =
                  (PixmapPtr)pScreen->devPrivates[cfb32ScreenPrivateIndex].ptr;
            break;
	}
   }

   if (enter) {
      xf86MapDisplay(screen_idx, VGA_REGION);
      if (s3VideoMem != vgaBase)
	 xf86MapDisplay(screen_idx, LINEAR_REGION);

      if (!xf86Resetting) {
	 ScrnInfoPtr pScr = XF86SCRNINFO(pScreen);

         s3Unlock();
         s3Init(s3InfoRec.modes);
         s3InitEnvironment();
         AlreadyInited = TRUE;
	 s3RestoreDACvalues();
	 s3ImageInit();
   	 s3FontCache8Init();
	 s3RestoreColor0(pScreen);
	 (void)s3CursorInit(0, pScreen); 
	 s3RestoreCursor(pScreen);
	 s3AdjustFrame(pScr->frameX0, pScr->frameY0);

	 if ((pointer)pspix->devPrivate.ptr != s3VideoMem && ppix) {
	    pspix->devPrivate.ptr = s3VideoMem;
	    (*s3ImageWriteFunc)(0, 0, pScreen->width, pScreen->height,
			        ppix->devPrivate.ptr,
			        PixmapBytePad(s3DisplayWidth, 
					      pScreen->rootDepth),
			        0, 0, s3alu[GXcopy], ~0);
	 }
      }
      if (ppix) {
	 (pScreen->DestroyPixmap)(ppix);
	 ppix = NULL;
      }
   } else {
      /* Make sure that another driver hasn't disabled IO */
      xf86MapDisplay(screen_idx, VGA_REGION);
      if (s3VideoMem != vgaBase)
	 xf86MapDisplay(screen_idx, LINEAR_REGION);

      if (!xf86Exiting) {
	 ppix = (pScreen->CreatePixmap)(pScreen, s3DisplayWidth,
					pScreen->height, pScreen->rootDepth);
	 if (ppix) {
	    (*s3ImageReadFunc)(0, 0, pScreen->width, pScreen->height,
			       ppix->devPrivate.ptr,
			       PixmapBytePad(s3DisplayWidth, 
					     pScreen->rootDepth),
			       0, 0, ~0);
	    pspix->devPrivate.ptr = ppix->devPrivate.ptr;
	 }
      }
      if (AlreadyInited) {
	  s3CleanUp();
	  AlreadyInited = FALSE;
      }
      xf86UnMapDisplay(screen_idx, VGA_REGION);
      if (s3VideoMem != vgaBase)
	 xf86UnMapDisplay(screen_idx, LINEAR_REGION);
   }
}

/*
 * s3CloseScreen -- called to ensure video is enabled when server exits.
 */

Bool
s3CloseScreen(screen_idx, pScreen)
     int screen_idx;
     ScreenPtr pScreen;
{

 /*
  * Hmm... The server may shut down even if it is not running on the current
  * vt. Let's catch this case here.
  */
   xf86Exiting = TRUE;
   if (xf86VTSema)
      s3EnterLeaveVT(LEAVE, screen_idx);
   else if (ppix) {
      /*
       * 7-Jan-94 CEG: The server is not running on the current vt.
       * Free the screen snapshot taken when the server vt was left.
       */
      (s3savepScreen->DestroyPixmap)(ppix);
      ppix = NULL;
   }
   return (TRUE);
}

/*
 * s3SaveScreen -- blank the screen.
 */

Bool
s3SaveScreen(pScreen, on)
     ScreenPtr pScreen;
     Bool  on;
{
   unsigned char scrn, sync;

   if (on)
      SetTimeSinceLastInputEvent();

   if (xf86VTSema) {
      /* the server is running on the current vt */
      /* so just go for it */

      outb(0x3C4,1);
      scrn = inb(0x3C5);
      outb(vgaIOBase + 4, 0x17);
      sync = inb(vgaIOBase + 5);

      if (on) {
	 scrn &= 0xDF;			/* enable screen */
	 sync |= 0x80;			/* enable sync   */
      } else {
	 scrn |= 0x20;			/* blank screen */
	 sync &= ~0x80;			/* disable sync */
      }

      outw(0x3C4, 0x0100);              /* syncronous reset */
      outw(0x3C4, (scrn << 8) | 0x01); /* change mode */

      if (s3PowerSaver) {
         outb(vgaCRReg, sync);
      }
   }
   return (TRUE);
}

static debugcache = 0;

Bool
s3SwitchMode(mode)
     DisplayModePtr mode;
{
  WaitIdle();  /* wait till idle */

   if (OFLG_ISSET(OPTION_SHOWCACHE, &s3InfoRec.options)) {
      debugcache++;
      if (debugcache & 1) {
        mode = mode->prev;
        s3InfoRec.modes=mode;
      }
   }

   if (s3Init(mode)) {
      s3ModeSwitched = TRUE;
#if notyet
      s3Restore(vgaNewVideoState);
#endif
      s3ImageInit(); /* mode switching changes the memory bank */
      return (TRUE);
   } else {
      ErrorF("Mode switch failed because of hardware initialisation error\n");
      s3ImageInit();
      return (FALSE);
   }
}

void
s3AdjustFrame(int x, int y)
{
   int   Base;
   unsigned char tmp;

   if (OFLG_ISSET(OPTION_SHOWCACHE, &s3InfoRec.options)) {
      if ( debugcache & 1)
         y += 512;
   }
      
   Base = ((y * s3DisplayWidth + x) * s3Bpp) >> 2;

   /* Elsa Winner 1000 has display errors with 16bpp and (Base&0x0f) == 0x0f */
   if (s3Bpp>1 && DAC_IS_SC15025 && S3_928_SERIES(s3ChipId)) {
      if ((Base&0x0f) == 0x0f) Base--;
   }
   else if (DAC_IS_SC15025 && S3_928_SERIES(s3ChipId)) {
      if ((Base&0x3f) == 0x3f) Base--;
   }

   outb(vgaCRIndex, 0x31);
   outb(vgaCRReg, ((Base & 0x030000) >> 12) | s3Port31);
   s3Port51 &= ~0x03;
   s3Port51 |= ((Base & 0x0c0000) >> 18);
   outb(vgaCRIndex, 0x51);
   /* Don't override current bank selection */
   tmp = (inb(vgaCRReg) & ~0x03) | (s3Port51 & 0x03);
   outb(vgaCRReg, tmp);

   outw(vgaCRIndex, (Base & 0x00FF00) | 0x0C);
   outw(vgaCRIndex, ((Base & 0x00FF) << 8) | 0x0D);

   if (s3ModeSwitched) {
      s3ModeSwitched = FALSE;
      s3RestoreCursor(s3savepScreen);
   } else {
      s3RepositionCursor(s3savepScreen);
   }

}

/*
 * Force optimising compilers to read *addr
 */
static Bool
s3TryAddress(addr, value, physaddr, stage)
     long *addr;
     long value;
     long physaddr;
     int stage;
{
   long tmp;

   tmp = *addr;

   if (OFLG_ISSET(OPTION_FB_DEBUG, &s3InfoRec.options)) {
      if (stage == 0 && tmp != 0xFFFFFFFF) {
	 ErrorF("Memory found at 0x%08X (read 0x%08X)\n", physaddr, tmp);
      } else {
	 ErrorF("Stage %d: address = 0x%08X, read 0x%08X\n", stage, physaddr,
		tmp);
      }
   }
   if (tmp == value)
      return TRUE;
   else
      return FALSE;
}

/* $XConsortium: newmmio.c /main/5 1996/10/24 07:11:34 kaleb $ */




/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/drivers/newmmio/newmmio.c,v 3.10 1997/04/08 10:11:25 hohndel Exp $ */
/*
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.
 * David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include "s3.h"
#include "regs3.h"

#if defined(XFree86LOADER)
#include "xf86Version.h"

extern char *xf86ModulePath;

XF86ModuleVersionInfo newmmioVersRec =
{
	"newmmio.o", 
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00010001,
	{0,0,0,0}	/* signature, to be patched into the file by a tool */
};


/*
 * this function returns the vgaVideoChipPtr for this driver
 *
 * it name has to be ModuleInit()
 */
void
ModuleInit(data,magic)
    pointer	* data;
    INT32	* magic;
{
    extern vgaVideoChipRec MGA;
    static int cnt = 0;

    switch(cnt++)
    {
    default:
        * magic= MAGIC_DONE;
        break;
    }
    return;
}
#endif  /* XFree86LOADER */


static Bool NEWMMIO_Probe();
static char *NEWMMIO_Ident();
extern void newmmio_s3EnterLeaveVT();
extern Bool newmmio_s3Initialize();
extern void newmmio_s3AdjustFrame();
extern Bool newmmio_s3SwitchMode();
extern Bool s3NewMmio;

/*
 * If 'volatile' isn't available, we disable the MMIO code 
 */

#if defined(__STDC__) || defined(__GNUC__)
#define HAVE_VOLATILE
#endif

s3VideoChipRec NEWMMIO = {
  NEWMMIO_Probe,
  NEWMMIO_Ident,
#ifdef HAVE_VOLATILE
  newmmio_s3EnterLeaveVT,
  newmmio_s3Initialize,
  newmmio_s3AdjustFrame,
  newmmio_s3SwitchMode,
#else
  NULL,
  NULL,
  NULL,
  NULL,
#endif
};

static char *
NEWMMIO_Ident(n)
     int n;
{
#ifdef HAVE_VOLATILE
   static char *chipsets[] = {"newmmio"};

   if (n + 1 > sizeof(chipsets) / sizeof(char *))
      return(NULL);
   else
      return(chipsets[n]);
#else
   return(NULL);
#endif
}


static Bool
NEWMMIO_Probe()
{
#ifdef HAVE_VOLATILE
   if (s3InfoRec.chipset) {
      if (StrCaseCmp(s3InfoRec.chipset, NEWMMIO_Ident(0)))
	 return(FALSE);
      else {
	 /* don't allow "newmmio" for S3 chips which don't support it 
	    even when specified in the config file, so fall through
	    for more sanity checks... */
      }
   }

   if ((S3_x68_SERIES(s3ChipId) ||
       S3_TRIO64V_SERIES(s3ChipId) ||
       S3_TRIO64V2_SERIES(s3ChipId))
       && xf86LinearVidMem()
       && !OFLG_ISSET(OPTION_NOLINEAR_MODE, &s3InfoRec.options)) {
      s3InfoRec.chipset = NEWMMIO_Ident(0);
      s3NewMmio = TRUE;
#if defined(XFree86LOADER)
#ifndef PC98
      LoadModule("libs3newmmio.a", xf86ModulePath);
#else
#ifdef PC98_NEC
      LoadModule("libs3necnewmmio.a", xf86ModulePath);
#endif
#ifdef PC98_XKB
      LoadModule("libs3pwskbnewmmio.a", xf86ModulePath);
#endif
#ifdef PC98_PWLB
      LoadModule("libs3pwlbnewmmio.a", xf86ModulePath);
#endif
#ifdef PC98_GA968
      LoadModule("libs3ga968newmmio.a", xf86ModulePath);
#endif
#endif

      if (LoaderCheckUnresolved(xf86bpp, LD_RESOLV_NOW))
	 ErrorF("Warning: Some symbols couldn't be resolved!\n");
#endif
      return(TRUE);
   } else {
      return(FALSE);
   }
#else
   return(FALSE);
#endif
}

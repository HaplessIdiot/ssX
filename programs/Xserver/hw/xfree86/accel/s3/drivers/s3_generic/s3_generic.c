/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/drivers/s3_generic/s3_generic.c,v 3.10 1997/04/08 10:11:28 hohndel Exp $ */
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
/* $XConsortium: s3_generic.c /main/5 1996/02/21 17:34:05 kaleb $ */

#include "s3.h"

#if defined(XFree86LOADER)
#include "xf86Version.h"

extern char *xf86ModulePath;

XF86ModuleVersionInfo s3_genericVersRec =
{
	"s3_generic.o", 
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


static Bool S3_GENERICProbe();
static char *S3_GENERICIdent();
extern void s3EnterLeaveVT();
extern Bool s3Initialize();
extern void s3AdjustFrame();
extern Bool s3SwitchMode();

s3VideoChipRec S3_GENERIC = {
  S3_GENERICProbe,
  S3_GENERICIdent,
  s3EnterLeaveVT,
  s3Initialize,
  s3AdjustFrame,
  s3SwitchMode,
};

static char *
S3_GENERICIdent(n)
     int n;
{
   static char *chipsets[] = {"s3_generic"};

   if (n + 1 > sizeof(chipsets) / sizeof(char *))
      return(NULL);
   else
      return(chipsets[n]);
}


static Bool
S3_GENERICProbe()
{
   /*
    * We don't even get called unless the card is identified as S3, so just
    * return TRUE, unless the chipset is specified as something other than
    * "s3_generic" in XF86Config.
    */

   if (s3InfoRec.chipset) {
      if (StrCaseCmp(s3InfoRec.chipset, S3_GENERICIdent(0)))
	 return(FALSE);
   }
   else
      s3InfoRec.chipset = S3_GENERICIdent(0);

#if defined(XFree86LOADER)
#ifndef PC98
   LoadModule("libs3pio.a", xf86ModulePath);
#else
#ifdef PC98_NEC
   LoadModule("libs3necpio.a", xf86ModulePath);
#endif
#ifdef PC98_XKB
   LoadModule("libs3pwskbpio.a", xf86ModulePath);
#endif
#ifdef PC98_PWLB
   LoadModule("libs3pwlbpio.a", xf86ModulePath);
#endif
#ifdef PC98_GA968
   LoadModule("libs3ga968pio.a", xf86ModulePath);
#endif
#endif

   if (LoaderCheckUnresolved(xf86bpp, LD_RESOLV_NOW))
      ErrorF("Warning: Some symbols couldn't be resolved!\n");
#endif
   return(TRUE);
}

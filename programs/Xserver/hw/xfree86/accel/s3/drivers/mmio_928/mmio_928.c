/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/drivers/mmio_928/mmio_928.c,v 3.9 1996/12/23 06:42:28 dawes Exp $ */
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
/* $XConsortium: mmio_928.c /main/5 1996/02/21 17:33:58 kaleb $ */

#include "s3.h"
#include "regs3.h"
#include "xf86Version.h"

extern char *xf86ModulePath;

XF86ModuleVersionInfo mmio_928VersRec =
{
	"mmio_928.o", 
	"The XFree86 Project",
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


static Bool MMIO_928Probe();
static char *MMIO_928Ident();
extern void mmio928_s3EnterLeaveVT();
extern Bool mmio928_s3Initialize();
extern void mmio928_s3AdjustFrame();
extern Bool mmio928_s3SwitchMode();
extern Bool s3Mmio928;

/*
 * If 'volatile' isn't available, we disable the MMIO code 
 */

#if defined(__STDC__) || defined(__GNUC__)
#define HAVE_VOLATILE
#endif

s3VideoChipRec MMIO_928 = {
  MMIO_928Probe,
  MMIO_928Ident,
#ifdef HAVE_VOLATILE
  mmio928_s3EnterLeaveVT,
  mmio928_s3Initialize,
  mmio928_s3AdjustFrame,
  mmio928_s3SwitchMode,
#else
  NULL,
  NULL,
  NULL,
  NULL,
#endif
};

static char *
MMIO_928Ident(n)
     int n;
{
#ifdef HAVE_VOLATILE
   static char *chipsets[] = {"mmio_928"};

   if (n + 1 > sizeof(chipsets) / sizeof(char *))
      return(NULL);
   else
      return(chipsets[n]);
#else
   return(NULL);
#endif
}


static Bool
MMIO_928Probe()
{
#ifdef HAVE_VOLATILE
   if (s3InfoRec.chipset) {
      if (StrCaseCmp(s3InfoRec.chipset, MMIO_928Ident(0)))
	 return(FALSE);
   }

   if (S3_928_SERIES(s3ChipId)) {
      s3InfoRec.chipset = MMIO_928Ident(0);
   } else {
      return(FALSE);
   }

   s3Mmio928 = TRUE;
   LoadModule("libs3mmio.a", xf86ModulePath);
   return(TRUE);

#else
   return(FALSE);
#endif
}

/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/xf86sym.c,v 1.6 1997/02/18 17:51:44 hohndel Exp $ */




/*
 *
 * Copyright 1995,96 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include "sym.h"
#include "misc.h"
#include "mi.h"
#include "cursor.h"
#include "mipointer.h"
#include "xf86.h"
#include "xf86Procs.h"
#include "accel/cache/xf86bcache.h"
#include "accel/cache/xf86fcache.h"
#include "accel/cache/xf86text.h"
#include "xf86_HWlib.h"
#include "xf86_OSlib.h"
#include "xf86_PCI.h"
#include "CirrusClk.h"
#include "vga.h"

extern Bool xf86Resetting;
extern Bool xf86ProbeFailed;
extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern char **xf86VisualNames;
extern vgaVideoChipPtr vgaDrivers[];
extern unsigned char (* dacInTi3026IndReg)(unsigned char);
extern void (* dacOutTi3026IndReg)(unsigned char,unsigned char,unsigned char);
extern int LoaderDefaultFunc(void);
extern void STG1703magic(int);
extern unsigned char STG1703getIndex(unsigned int);
extern void STG1703setIndex(unsigned int, unsigned char);

extern int vgaIOBase,vgaCRReg,vgaCRIndex;
extern unsigned char *xf86rGammaMap,*xf86gGammaMap,*xf86bGammaMap;

/* XFree86 things */

LOOKUP xfree86LookupTab[] = {
   SYMFUNC(xf86ReadBIOS)
   SYMFUNC(xf86ClearIOPortList)
   SYMFUNC(xf86AddIOPorts)
   SYMFUNC(xf86EnableIOPorts)
   SYMFUNC(xf86DisableIOPorts)
   SYMFUNC(xf86StringToToken)
   SYMFUNC(xf86LinearVidMem)
   SYMFUNC(xf86DeleteMode)
   SYMFUNC(xf86GetClocks)
   SYMFUNC(xf86LookupMode)
   SYMFUNC(xf86FlipPixels)
   SYMFUNC(xf86UnMapDisplay)
   SYMFUNC(xf86MapDisplay)
   SYMFUNC(xf86MapVidMem)
   SYMFUNC(xf86VerifyOptions)
   SYMFUNC(xf86ccdDoBitblt)
   SYMFUNC(xf86ccdXAAScreenInit)
   SYMFUNC(SlowBcopy)
   SYMFUNC(BusToMem)
   SYMFUNC(MemToBus)
   SYMFUNC(StrCaseCmp)
   SYMFUNC(SetTimeSinceLastInputEvent)

   SYMFUNC(xf86dactopel)
   SYMFUNC(xf86dactocomm)
   SYMFUNC(xf86getdaccomm)
   SYMFUNC(xf86setdaccomm)
   SYMFUNC(xf86setdaccommbit)
   SYMFUNC(xf86clrdaccommbit)
   SYMFUNC(xf86TokenToString)
   SYMFUNC(s3IBMRGB_Probe)
   SYMFUNC(s3IBMRGB_Init)
   SYMFUNC(s3InIBMRGBIndReg)
   SYMFUNC(Ti3025SetClock)
   SYMFUNC(Ti3026SetClock)
   SYMFUNC(Ti3030SetClock)
   SYMFUNC(AltICD2061SetClock)
   SYMFUNC(SC11412SetClock)
   SYMFUNC(ICS2595SetClock)
   SYMFUNC(Att409SetClock)
   SYMFUNC(Chrontel8391SetClock)
   SYMFUNC(IBMRGBSetClock)
   SYMFUNC(ICS5342SetClock)
   SYMFUNC(S3TrioSetClock)
   SYMFUNC(S3gendacSetClock)
   SYMFUNC(STG1703SetClock)
   SYMFUNC(ET6000SetClock)
   SYMFUNC(commonCalcClock)
   SYMFUNC(xf86scanpci)
   SYMFUNC(xf86writepci)
   SYMFUNC(xf86cleanpci)
   SYMFUNC(xf86UnMapVidMem)
   SYMFUNC(dacOutTi3026IndReg)
   SYMFUNC(dacInTi3026IndReg)
   SYMFUNC(s3OutIBMRGBIndReg)
   SYMFUNC(CirrusFindClock)
   SYMFUNC(CirrusSetClock)
   SYMFUNC(GlennsIODelay)
   SYMFUNC(STG1703getIndex)
   SYMFUNC(STG1703setIndex)
   SYMFUNC(STG1703magic)
   SYMFUNC(gendacMNToClock)
   SYMFUNC(Et4000AltICD2061SetClock)
   SYMFUNC(ET4000stg1703SetClock)
   SYMFUNC(ET4000gendacSetClock)


   SYMFUNC(pciWriteWord)
   SYMFUNC(pcibusWrite)
   SYMFUNC(pcibusRead)
   SYMFUNC(pcibusTag)
   SYMFUNC(AllocatePixmapPrivateIndex)
   SYMFUNC(AllocatePixmapPrivate)
   SYMFUNC(LoaderDefaultFunc)

/*
 * these here are our own interfaces to libc functions
 */
   SYMFUNC(xf86memmove)
   SYMFUNC(xf86memset)
   SYMFUNC(xf86memcpy)
   SYMFUNC(xf86memcmp)
   SYMFUNC(xf86strcat)
   SYMFUNC(xf86strcpy)
   SYMFUNC(xf86strncmp)
   SYMFUNC(xf86strcmp)
   SYMFUNC(xf86strlen)
   SYMFUNC(xf86exp)
   SYMFUNC(xf86log)
   SYMFUNC(xf86pow)
   SYMFUNC(xf86sqrt)
   SYMFUNC(xf86cos)
   SYMFUNC(xf86usleep)
   SYMFUNC(xf86bzero)
   SYMFUNC(xf86getbitsperpixel)
   SYMFUNC(xf86sprintf)
   SYMFUNC(xf86strerror)
   SYMFUNC(xf86setexternclock)
   SYMFUNC(xf86execl)

/*
 * these are our own interfaces where libc functions
 * would pass structures around
 */
   SYMFUNC(xf86getsecs)

/*
 * these are only needed for the /tmp/accelswitch in the mga driver
 */
   SYMFUNC(fscanf)
   SYMFUNC(fopen)

/*
 * not sure yet about these here
 */
   SYMFUNC(ffs)

/*
 * and now some variables
 */
   SYMVAR(xf86bpp)
   SYMVAR(xf86weight)
   SYMVAR(xf86Verbose)
   SYMVAR(xf86PointerScreenFuncs)
   SYMVAR(xf86VTSema)
   SYMVAR(xf86Exiting)
   SYMVAR(xf86Resetting)
   SYMVAR(xf86VisualNames)
   SYMVAR(xf86Info)
   SYMVAR(xf86ProbeFailed)
   SYMVAR(xf86ScreenIndex)
   SYMVAR(vgaCRIndex)
   SYMVAR(vgaCRReg)
   SYMVAR(vgaIOBase)
   SYMVAR(xf86rGammaMap)
   SYMVAR(xf86gGammaMap)
   SYMVAR(xf86bGammaMap)
   SYMVAR(xf86ccdScreenPrivateIndex)
   SYMVAR(xf86xaaloaded)
   SYMVAR(xf86ProbeOnly)
   SYMVAR(vgaDrivers)

  { 0, 0 },

};

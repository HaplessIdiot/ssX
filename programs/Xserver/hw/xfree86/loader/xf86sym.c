/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/xf86sym.c,v 1.24 1997/07/06 05:30:59 dawes Exp $ */




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
#include <fcntl.h>
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
#include "xf86_Config.h"
#include "xf86Xinput.h"
#include "CirrusClk.h"
#include "vga.h"

#define DONT_DEFINE_WRAPPERS
#include "xf86_libc.h"
#include "xf86_ansic.h"

extern Bool xf86Resetting;
extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern char **xf86VisualNames;
extern vgaVideoChipPtr videoDrivers[];
extern unsigned char (* dacInTi3026IndReg)(unsigned char);
extern void (* dacOutTi3026IndReg)(unsigned char,unsigned char,unsigned char);
extern int LoaderDefaultFunc(void);
extern void STG1703magic(int);
extern unsigned char STG1703getIndex(unsigned int);
extern void STG1703setIndex(unsigned int, unsigned char);

extern int vgaIOBase,vgaCRReg,vgaCRIndex;
extern unsigned char *xf86rGammaMap,*xf86gGammaMap,*xf86bGammaMap;
extern char *xf86ModulePath;
extern LoadModule();
extern int LoaderCheckUnresolved();
#ifdef DPMSExtension
extern void DPMSSet(CARD16);
#endif

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
   SYMFUNC(xf86InitViewport)
   SYMFUNC(xf86VerifyOptions)
   SYMFUNC(xf86ccdDoBitblt)
   SYMFUNC(xf86ccdXAAScreenInit)
   SYMFUNC(SlowBcopy)
   SYMFUNC(BusToMem)
   SYMFUNC(MemToBus)
   SYMFUNC(StrCaseCmp)
   SYMFUNC(SetTimeSinceLastInputEvent)
   SYMFUNC(GetTimeInMillis)
   SYMFUNC(xf86CheckMode)
   SYMFUNC(xf86ZoomViewport)
   SYMFUNC(xf86LockZoom)
   SYMFUNC(xf86MouseInit)
   SYMFUNC(xf86SetKbdRepeat)
   SYMFUNC(xf86GetToken)
   SYMFUNC(xf86ConfigError)
#ifdef XINPUT
   SYMFUNC(xf86AlwaysCore)
   SYMFUNC(xf86PostMotionEvent)
   SYMFUNC(xf86PostButtonEvent)
   SYMFUNC(xf86GetMotionEvents)
   SYMFUNC(xf86MotionHistoryAllocate)
#endif
#ifdef DPMSExtension
   SYMFUNC(DPMSSet)
#endif

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
   SYMFUNC(S3Trio64V2SetClock)
   SYMFUNC(S3gendacSetClock)
   SYMFUNC(STG1703SetClock)
   SYMFUNC(ET6000SetClock)
   SYMFUNC(S3AuroraSetClock)
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
   SYMFUNC(xf86DisableInterrupts)
   SYMFUNC(xf86EnableInterrupts)
   SYMFUNC(xf86GetNearestClock)

   SYMFUNC(pciWriteByte)
   SYMFUNC(pciWriteWord)
   SYMFUNC(pcibusWrite)
   SYMFUNC(pciReadByte)
   SYMFUNC(pciReadWord)
   SYMFUNC(pcibusRead)
   SYMFUNC(pcibusTag)
   SYMFUNC(AllocatePixmapPrivateIndex)
   SYMFUNC(AllocatePixmapPrivate)
   SYMFUNC(LoaderDefaultFunc)
   SYMFUNC(LoadModule)
   SYMFUNC(xf86ModulePath)
   SYMFUNC(LoaderCheckUnresolved)

/*
 * these here are our own interfaces to libc functions
 */
  SYMFUNC(xf86abort)
  SYMFUNC(xf86abs)
  SYMFUNC(xf86acos)
  SYMFUNC(xf86asin)
  SYMFUNC(xf86atan)
  SYMFUNC(xf86atan2)
  SYMFUNC(xf86atof)
  SYMFUNC(xf86atoi)
  SYMFUNC(xf86atol)
  SYMFUNC(xf86ceil)
  SYMFUNC(xf86calloc)
  SYMFUNC(xf86clearerr)
  SYMFUNC(xf86cos)
  SYMFUNC(xf86exit)
  SYMFUNC(xf86exp)
  SYMFUNC(xf86fabs)
  SYMFUNC(xf86fclose)
  SYMFUNC(xf86feof)
  SYMFUNC(xf86ferror)
  SYMFUNC(xf86fflush)
  SYMFUNC(xf86fgetc)
  SYMFUNC(xf86fgetpos)
  SYMFUNC(xf86fgets)
  SYMFUNC(xf86floor)
  SYMFUNC(xf86fmod)
  SYMFUNC(xf86fopen)
  SYMFUNC(xf86fprintf)
  SYMFUNC(xf86fputc)
  SYMFUNC(xf86fputs)
  SYMFUNC(xf86fread)
  SYMFUNC(xf86free)
  SYMFUNC(xf86freopen)
  SYMFUNC(xf86fscanf)
  SYMFUNC(xf86fseek)
  SYMFUNC(xf86fsetpos)
  SYMFUNC(xf86ftell)
  SYMFUNC(xf86fwrite)
  SYMFUNC(xf86getenv)
  SYMFUNC(xf86isalnum)
  SYMFUNC(xf86isalpha)
  SYMFUNC(xf86iscntrl)
  SYMFUNC(xf86isdigit)
  SYMFUNC(xf86isgraph)
  SYMFUNC(xf86islower)
  SYMFUNC(xf86isprint)
  SYMFUNC(xf86ispunct)
  SYMFUNC(xf86isspace)
  SYMFUNC(xf86isupper)
  SYMFUNC(xf86isxdigit)
  SYMFUNC(xf86labs)
  SYMFUNC(xf86log)
  SYMFUNC(xf86log10)
  SYMFUNC(xf86malloc)
  SYMFUNC(xf86memchr)
  SYMFUNC(xf86memcmp)
  SYMFUNC(xf86memcpy)
  SYMFUNC(xf86memmove)
  SYMFUNC(xf86memset)
  SYMFUNC(xf86modf)
  SYMFUNC(xf86perror)
  SYMFUNC(xf86pow)
  SYMFUNC(xf86realloc)
  SYMFUNC(xf86remove)
  SYMFUNC(xf86rename)
  SYMFUNC(xf86rewind)
  SYMFUNC(xf86setbuf)
  SYMFUNC(xf86setvbuf)
  SYMFUNC(xf86sin)
  SYMFUNC(xf86sprintf)
  SYMFUNC(xf86sqrt)
  SYMFUNC(xf86sscanf)
  SYMFUNC(xf86strcat)
  SYMFUNC(xf86strcmp)
  SYMFUNC(xf86strcpy)
  SYMFUNC(xf86strcspn)
  SYMFUNC(xf86strerror)
  SYMFUNC(xf86strlen)
  SYMFUNC(xf86strncmp)
  SYMFUNC(xf86strncpy)
  SYMFUNC(xf86strpbrk)
  SYMFUNC(xf86strrchr)
  SYMFUNC(xf86strspn)
  SYMFUNC(xf86strstr)
  SYMFUNC(xf86strtod)
  SYMFUNC(xf86strtok)
  SYMFUNC(xf86strtol)
  SYMFUNC(xf86strtoul)
  SYMFUNC(xf86tan)
  SYMFUNC(xf86tmpfile)
  SYMFUNC(xf86tmpnam)
  SYMFUNC(xf86tolower)
  SYMFUNC(xf86toupper)
  SYMFUNC(xf86ungetc)
  SYMFUNC(xf86vfprintf)
  SYMFUNC(xf86vsprintf)
  
/* non-ANSI C functions */
   SYMFUNC(xf86opendir)
   SYMFUNC(xf86closedir)
   SYMFUNC(xf86readdir)
   SYMFUNC(xf86rewinddir)
   SYMFUNC(xf86ffs)
   SYMFUNC(xf86strdup)
   SYMFUNC(xf86bzero)
   SYMFUNC(xf86usleep)
   SYMFUNC(xf86execl)

   SYMFUNC(xf86getbitsperpixel)
   SYMFUNC(xf86setexternclock)
   SYMFUNC(xf86getsecs)
   SYMFUNC(xf86fpossize)      /* for returning sizeof(fpos_t) */
  
   SYMVAR(xf86stdin)
   SYMVAR(xf86stdout)
   SYMVAR(xf86stderr)

/*
 * and now some variables
 */
#ifdef XF86MISC
   SYMVAR(xf86AllowMouseOpenFail)
   SYMVAR(xf86MiscModInDevAllowNonLocal)
   SYMVAR(xf86MiscModInDevEnabled)
#endif
#ifdef XF86VIDMODE
   SYMVAR(xf86VidModeEnabled)
   SYMVAR(xf86VidModeAllowNonLocal)
#endif
   SYMVAR(xf86BestRefresh)
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
   SYMVAR(xf86issvgatype)
   SYMVAR(xf86ismonotype)
   SYMVAR(xf86ProbeOnly)
   SYMVAR(videoDrivers)
   SYMVAR(TimingTab)
#ifdef DPMSExtension
   SYMVAR(DPMSEnabledSwitch)
   SYMVAR(DPMSDisabledSwitch)
   SYMVAR(defaultDPMSEnabled)
#endif

  { 0, 0 },

};

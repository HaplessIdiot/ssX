/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/xf86sym.c,v 1.32 1998/03/21 22:40:13 robin Exp $ */




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
#include "resource.h"
#include "extnsionst.h"
#include "scrnintstr.h"
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

extern void xf86Bzero(void *, unsigned int);
extern int xf86GetBitsPerPixel(int);
extern void *xf86Memset(void *, int, unsigned int);
extern void *xf86Memmove(void *, const void *, unsigned int);
extern Bool xf86SetExternClock(char *, int, int);
extern int xf86Sprintf(char *, const char *, ...);
extern char *xf86Strerror(int);
extern int xf86Strncmp(const char *, const char *, unsigned int);
extern char *xf86Strncpy(char *, const char *, unsigned int);
extern void xf86Usleep(unsigned long);
extern int xf86Execl(char *, char *, char *, char *, char *);

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
extern char *xf86ModulePath;
extern LoadModule();
extern int LoaderCheckUnresolved();
#ifdef DPMSExtension
extern void DPMSSet(CARD16);
#endif
extern Bool (*GlxInitVisualsPtr)();

#if defined (PowerMAX_OS)
#undef inb
#undef inw
#undef inl
#undef outb
#undef outw
#undef outl

void outb(unsigned int a, unsigned char b);
void outw(unsigned int a, unsigned short w); 
void outl(unsigned int a, unsigned long l); 
extern unsigned char  inb(unsigned int a);
extern unsigned short inw(unsigned int a);
extern unsigned long  inl(unsigned int a);

#endif

#if defined(__alpha__)
extern void _outb(char val, unsigned short port);
extern void _outw(short val, unsigned short port);
extern void _outl(int val, unsigned short port);
extern unsigned int _inb(unsigned short port);
extern unsigned int _inw(unsigned short port);
extern unsigned int _inl(unsigned short port);

extern void xf86WriteSparse16(int, pointer, unsigned long);

extern void SlowBCopyToBus(char *src, char *dst, int count);
extern void SlowBCopyFromBus(char *src, char *dst, int count);

extern void* __divl(long, long);
extern void* __reml(long, long);
extern void* __divlu(long, long);
extern void* __remlu(long, long);
extern void* __divq(long, long);
extern void* __divqu(long, long);
#endif

#if defined(__powerpc__) && defined(Lynx)
void eieio();
void _restf23();
void _restf24();
void _restf25();
void _restf26();
void _restf27();
void _restf28();
void _restf29();
void _savef23();
void _savef24();
void _savef25();
void _savef26();
void _savef27();
void _savef28();
void _savef29();
#endif

/* XFree86 things */

LOOKUP xfree86LookupTab[] = {
   SYMFUNC(xf86ReadBIOS)
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
   SYMFUNC(xf86MapPciMem)
   SYMFUNC(xf86InitViewport)
   SYMFUNC(xf86VerifyOptions)
#if 0
   SYMFUNC(xf86ccdDoBitblt)
   SYMFUNC(xf86ccdXAAScreenInit)
#endif
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
#ifdef XQUEUE
   SYMFUNC(xf86XqueKbdProc)
   SYMFUNC(xf86XqueMseProc)
   SYMFUNC(xf86XqueEvents)
#endif
   SYMFUNC(xf86SetCurrentScreen)
   SYMFUNC(xf86GetCurrentScreen)
   SYMFUNC(xf86GetConsoleFD)

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
   SYMFUNC(Ti3025CalcNMP)
   SYMFUNC(Ti3026SetClock)
   SYMFUNC(Ti3030SetClock)
   SYMFUNC(AltICD2061CalcClock)
   SYMFUNC(AltICD2061SetClock)
   SYMFUNC(et4000_init_clock)
   SYMFUNC(SC11412SetClock)
   SYMFUNC(ICS2595SetClock)
   SYMFUNC(Att409SetClock)
   SYMFUNC(Chrontel8391CalcClock)
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
	   
   /* Public PCI access functions */
   SYMFUNC(pciInit)
   SYMFUNC(pciFindFirst)
   SYMFUNC(pciFindNext)
   SYMFUNC(pciEnableIO)
   SYMFUNC(pciDisableIO)
   SYMFUNC(pciReadLong)
   SYMFUNC(pciReadWord)
   SYMFUNC(pciReadByte)
   SYMFUNC(pciWriteLong)
   SYMFUNC(pciWriteWord)
   SYMFUNC(pciWriteByte)
   SYMFUNC(pciBusAddrToHostAddr)
   SYMFUNC(pciHostAddrToBusAddr)
   SYMFUNC(pciTag)
	   
   SYMFUNC(xf86scanpci)
   SYMFUNC(xf86writepci)
   SYMFUNC(xf86cleanpci)
   SYMFUNC(xf86lockpci)

   SYMFUNC(AllocatePixmapPrivateIndex)
   SYMFUNC(AllocatePixmapPrivate)
   SYMFUNC(LoaderDefaultFunc)
   SYMFUNC(LoadModule)
   SYMFUNC(xf86ModulePath)
   SYMFUNC(LoaderCheckUnresolved)

#undef abs
   SYMFUNC(abs)
	   
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
  SYMFUNC(xf86close)
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
  SYMFUNC(xf86ioctl)
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
#if defined(__powerpc__) && defined(Lynx)
  /*
   * The LynxOS compilers generate calls to memcpy to handle structure copies.
   * This causes a problem both here and in shared libraries as there is no
   * way to map the name of the call to the correct function.
   */
  SYMFUNC(memcpy)
#endif
  SYMFUNC(xf86memmove)
  SYMFUNC(xf86memset)
  SYMFUNC(xf86modf)
  SYMFUNC(xf86open)
  SYMFUNC(xf86perror)
  SYMFUNC(xf86pow)
  SYMFUNC(xf86read)
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

/* helpful routines from the parser */
   SYMFUNC(xf86FindDevice)
   SYMFUNC(xf86FindLayout)
   SYMFUNC(xf86FindMonitor)
   SYMFUNC(xf86FindModeLine)
   SYMFUNC(xf86FindScreen)
   SYMFUNC(xf86FindDisplay)
   SYMFUNC(xf86FindVendor)
   SYMFUNC(xf86FindOption)
   SYMFUNC(xf86FindOptionValue)
   SYMFUNC(xf86OptionListCreate)
   SYMFUNC(xf86OptionListMerge)
   SYMFUNC(xf86OptionListFree)

#if 0
/* Serial access routines */
   SYMFUNC(xf86OpenSerial)
   SYMFUNC(xf86SetSerial)
   SYMFUNC(xf86ReadSerial)
   SYMFUNC(xf86WriteSerial)
   SYMFUNC(xf86CloseSerial)
#endif

   SYMFUNC(xf86getbitsperpixel)
   SYMFUNC(xf86setexternclock)
   SYMFUNC(xf86getsecs)
   SYMFUNC(xf86fpossize)      /* for returning sizeof(fpos_t) */

#if defined(__alpha__)
   SYMFUNC(__divl)
   SYMFUNC(__reml)
   SYMFUNC(__divlu)
   SYMFUNC(__remlu)
   SYMFUNC(__divq)
   SYMFUNC(__divqu)

   SYMFUNC(_outw)
   SYMFUNC(_outb)
   SYMFUNC(_outl)
   SYMFUNC(_inb)
   SYMFUNC(_inw)
   SYMFUNC(_inl)
   SYMFUNC(SlowBCopyToBus)
   SYMFUNC(xf86MapVidMemSparse)
   SYMFUNC(xf86ReadSparse32)
   SYMFUNC(xf86ReadSparse16)
   SYMFUNC(xf86ReadSparse8)
   SYMFUNC(xf86WriteSparse32)
   SYMFUNC(xf86WriteSparse16)
   SYMFUNC(xf86WriteSparse8)
   SYMFUNC(SlowBCopyFromBus)
   SYMFUNC(memcpy)
#endif
#if defined(__powerpc__)	   
   SYMFUNC(inb)
   SYMFUNC(inw)
   SYMFUNC(inl)
   SYMFUNC(outb)
   SYMFUNC(outw)
   SYMFUNC(outl)
#if defined(NO_INLINE)
   SYMFUNC(mem_barrier)
   SYMFUNC(ldl_u)
   SYMFUNC(eieio)
   SYMFUNC(ldl_brx)
   SYMFUNC(ldw_brx)
   SYMFUNC(stl_brx)
   SYMFUNC(stw_brx)
   SYMFUNC(ldq_u)
   SYMFUNC(ldw_u)
   SYMFUNC(stl_u)
   SYMFUNC(stq_u)
   SYMFUNC(stw_u)
   SYMFUNC(write_mem_barrier)
#endif
   SYMFUNC(rdinx)
   SYMFUNC(wrinx)
   SYMFUNC(modinx)
   SYMFUNC(testrg)
   SYMFUNC(testinx2)
   SYMFUNC(testinx)
#if defined(Lynx)
   SYMFUNC(_restf23)
   SYMFUNC(_restf24)
   SYMFUNC(_restf25)
   SYMFUNC(_restf26)
   SYMFUNC(_restf27)
   SYMFUNC(_restf28)
   SYMFUNC(_restf29)
   SYMFUNC(_savef23)
   SYMFUNC(_savef24)
   SYMFUNC(_savef25)
   SYMFUNC(_savef26)
   SYMFUNC(_savef27)
   SYMFUNC(_savef28)
   SYMFUNC(_savef29)
#endif	   
#if PPCIO_DEBUG   
   SYMFUNC(debug_inb)
   SYMFUNC(debug_inw)
   SYMFUNC(debug_inl)
   SYMFUNC(debug_outb)
   SYMFUNC(debug_outw)
   SYMFUNC(debug_outl)
#endif	   
#endif	   

/*
 * and now some variables
 */

   SYMVAR(xf86stdin)
   SYMVAR(xf86stdout)
   SYMVAR(xf86stderr)
   SYMVAR(xf86errno)

   SYMVAR(byte_reversed)
   SYMVAR(InstalledMaps)
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
   SYMVAR(xf86Screens)
   SYMVAR(xf86ScreenIndex)
   SYMVAR(xf86PixmapIndex)
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
#ifdef OLD_PARSER
   SYMVAR(TimingTab)
#endif
#ifdef DPMSExtension
   SYMVAR(DPMSEnabledSwitch)
   SYMVAR(DPMSDisabledSwitch)
   SYMVAR(defaultDPMSEnabled)
#endif
#if defined(__powerpc__) && !defined(NO_INLINE)	   
   SYMVAR(ioBase)
#endif	   
  { 0, 0 },

};

/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/xf86sym.c,v 1.63 1999/02/07 06:18:47 dawes Exp $ */

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
#include "loaderProcs.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "xf86Parser.h"
#include "xf86Config.h"
#ifdef XINPUT
#include "xf86Xinput.h"
#endif
#include "xisb.h"
#include "xf86xv.h"
#include "xf86cmap.h"
#include "xf86fbman.h"
#include "dgaproc.h"
#include "loader.h"

#define DONT_DEFINE_WRAPPERS
#include "xf86_ansic.h"

/* XXX Should get all of these from elsewhere */
#if defined (PowerMAX_OS)
#undef inb
#undef inw
#undef inl
#undef outb
#undef outw
#undef outl

extern void outb(unsigned int a, unsigned char b);
extern void outw(unsigned int a, unsigned short w);
extern void outl(unsigned int a, unsigned long l);
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
   
extern void* __divl(long, long);
extern void* __reml(long, long);
extern void* __divlu(long, long);
extern void* __remlu(long, long);
extern void* __divq(long, long);
extern void* __divqu(long, long);
extern void* __remq(long, long);
extern void* __remqu(long, long);
#endif

#if defined(__powerpc__) && defined(Lynx)
void eieio();
void _restf17();
void _restf23();
void _restf24();
void _restf25();
void _restf26();
void _restf27();
void _restf28();
void _restf29();
void _savef17();
void _savef23();
void _savef24();
void _savef25();
void _savef26();
void _savef27();
void _savef28();
void _savef29();

/* even if we compile without -DNO_INLINE we still provide
 * the usual port i/o functions for module use
 */

extern volatile unsigned char *ioBase;

/* XXX Should get all of these from elsewhere */

extern void outb(unsigned short, unsigned char);
extern void outw(unsigned short, unsigned short);
extern void outl(unsigned short, unsigned int);
extern unsigned int inb(unsigned short);
extern unsigned int inw(unsigned short);
extern unsigned int inl(unsigned short);
extern unsigned long ldq_u(void *);
extern unsigned long ldl_u(void *);
extern unsigned short ldw_u(void *);
extern void stl_u(unsigned long, void *);
extern void stq_u(unsigned long, void *);
extern void stw_u(unsigned short, void *);
extern void mem_barrier(void);
extern void write_mem_barrier(void);
extern void stl_brx(unsigned long, volatile unsigned char *, int);
extern void stw_brx(unsigned short, volatile unsigned char *, int);
extern unsigned long ldl_brx(volatile unsigned char *, int);
extern unsigned short ldw_brx(volatile unsigned char *, int);
extern unsigned char rdinx(unsigned short, unsigned char);
extern void wrinx(unsigned short, unsigned char, unsigned char);
extern void modinx(unsigned short, unsigned char, unsigned char, unsigned char);
extern int testrg(unsigned short, unsigned char);
extern int testinx2(unsigned short, unsigned char, unsigned char);
extern int testinx(unsigned short, unsigned char);
#endif

/* XXX This needs to be cleaned up for the new design */

#ifdef DPMSExtension
extern void DPMSSet(CARD16);
#endif

/* XFree86 things */

LOOKUP xfree86LookupTab[] = {

   /* Public OSlib functions */
   SYMFUNC(xf86ReadBIOS)
   SYMFUNC(xf86EnableIO)
   SYMFUNC(xf86DisableIO)
   SYMFUNC(xf86DisableInterrupts)
   SYMFUNC(xf86EnableInterrupts)
   SYMFUNC(xf86LinearVidMem)
   SYMFUNC(xf86MapVidMem)
   SYMFUNC(xf86UnMapVidMem)
   SYMFUNC(xf86IODelay)
   SYMFUNC(xf86SlowBcopy)
#ifdef __alpha__
   SYMFUNC(xf86SlowBCopyToBus)
   SYMFUNC(xf86SlowBCopyFromBus)
#endif
   SYMFUNC(xf86BusToMem)
   SYMFUNC(xf86MemToBus)
   SYMFUNC(xf86OpenSerial)
   SYMFUNC(xf86SetSerial)
   SYMFUNC(xf86ReadSerial)
   SYMFUNC(xf86WriteSerial)
   SYMFUNC(xf86CloseSerial)
/* Next three merged in from Metrolink tree	(posix_tty.c) */
   SYMFUNC(xf86GetErrno)
   SYMFUNC(xf86WaitForInput)
   SYMFUNC(xf86SerialSendBreak)

#ifdef XINPUT
/* XISB routines  (Merged from Metrolink tree) */
   SYMFUNC(XisbNew)
   SYMFUNC(XisbFree)
   SYMFUNC(XisbRead)
   SYMFUNC(XisbWrite)
   SYMFUNC(XisbTrace)
   SYMFUNC(XisbBlockDuration)
#endif

   /* xf86Bus.c */
   SYMFUNC(xf86CheckPciSlot)
   SYMFUNC(xf86ClaimPciSlot)
   SYMFUNC(xf86ReleasePciSlot)
   SYMFUNC(xf86GetPciVideoInfo)
   SYMFUNC(xf86GetPciInfoForScreen)
   SYMFUNC(xf86CheckIsaSlot)
   SYMFUNC(xf86ClaimIsaSlot)
   SYMFUNC(xf86ReleaseIsaSlot)
   SYMFUNC(xf86FreeBusSlots)
   SYMFUNC(xf86ParsePciBusString)
   SYMFUNC(xf86ComparePciBusString)
   SYMFUNC(xf86ParseIsaBusString)
   SYMFUNC(xf86IsPciBus)
   SYMFUNC(xf86IsIsaBus)
   SYMFUNC(xf86FindChipsetsForScreen)
   SYMFUNC(xf86AddControlledResource)
   SYMFUNC(xf86DelControlledResource)
   SYMFUNC(xf86EnableAccess)
   SYMFUNC(xf86IsPrimaryPci)
   SYMFUNC(xf86IsPrimaryIsa)
   SYMFUNC(xf86CheckPciGAType)

   /* xf86Cursor.c  XXX not all of these should be exported */
   SYMFUNC(xf86LockZoom)
   SYMFUNC(xf86SetScreenLayout)
   SYMFUNC(xf86SetViewport)
   SYMFUNC(xf86ZoomLocked)
   SYMFUNC(xf86ZoomViewport)
   SYMFUNC(xf86GetPointerScreenFuncs)

   /* xf86DGA.c */
   /* For drivers */
   SYMFUNC(DGACreateInfoRec)
   SYMFUNC(DGADestroyInfoRec)
   SYMFUNC(DGAInit)
   /* For extmod */
   SYMFUNC(DGAAvailable)
   SYMFUNC(DGAGetParameters)
   SYMFUNC(DGAScreenActive)
   SYMFUNC(DGASetViewPort)
   SYMFUNC(DGAEnableDirectMode)
   SYMFUNC(DGADisableDirectMode)
   SYMFUNC(DGAGetViewPortSize)
   SYMFUNC(DGAGetDirectMode)
   SYMFUNC(DGAGetVidPage)
   SYMFUNC(DGASetVidPage)
   SYMFUNC(DGAGetFlags)
   SYMFUNC(DGASetFlags)
   SYMFUNC(DGAViewPortChanged)

   /* xf86DPMS.c */
   SYMFUNC(xf86DPMSInit)

   /* xf86Events.c */
   SYMFUNC(SetTimeSinceLastInputEvent)

   /* xf86Helper.c */
   SYMFUNC(xf86AddDriver)
   SYMFUNC(xf86DeleteDriver)
   SYMFUNC(xf86AllocateScreen)
   SYMFUNC(xf86DeleteScreen)
   SYMFUNC(xf86AllocateScrnInfoPrivateIndex)
   SYMFUNC(xf86AddPixFormat)
   SYMFUNC(xf86SetDepthBpp)
   SYMFUNC(xf86PrintDepthBpp)
   SYMFUNC(xf86SetWeight)
   SYMFUNC(xf86SetDefaultVisual)
   SYMFUNC(xf86SetGamma)
   SYMFUNC(xf86SetDpi)
   SYMFUNC(xf86SetBlackWhitePixels)
   SYMFUNC(xf86SaveRestoreImage)
   SYMFUNC(xf86VDrvMsgVerb)
   SYMFUNC(xf86DrvMsgVerb)
   SYMFUNC(xf86DrvMsg)
   SYMFUNC(xf86MsgVerb)
   SYMFUNC(xf86Msg)
   SYMFUNC(xf86ErrorFVerb)
   SYMFUNC(xf86ErrorF)
   SYMFUNC(xf86TokenToString)
   SYMFUNC(xf86StringToToken)
   SYMFUNC(xf86ShowClocks)
   SYMFUNC(xf86PrintChipsets)
   SYMFUNC(xf86MatchDevice)
   SYMFUNC(xf86MatchPciInstances)
   SYMFUNC(xf86FindPciResource)
   SYMFUNC(xf86MatchIsaInstances)
   SYMFUNC(xf86FindIsaResource)
   SYMFUNC(xf86GetVerbosity)
   SYMFUNC(xf86GetVisualName)
   SYMFUNC(xf86GetPix24)
   SYMFUNC(xf86GetDepth)
   SYMFUNC(xf86GetWeight)
   SYMFUNC(xf86GetGamma)
   SYMFUNC(xf86GetFlipPixels)
   SYMFUNC(xf86GetServerName)
   SYMFUNC(xf86ServerIsExiting)
   SYMFUNC(xf86ServerIsResetting)
   SYMFUNC(xf86CaughtSignal)
   SYMFUNC(xf86GetClocks)
   SYMFUNC(xf86LoadSubModule)
   SYMFUNC(xf86LoaderReqSymLists)
   SYMFUNC(xf86LoaderReqSymbols)
   SYMFUNC(xf86Break1)
   SYMFUNC(xf86Break2)
   SYMFUNC(xf86Break3)
   SYMFUNC(xf86SetBackingStore)
   SYMFUNC(xf86NewSerialNumber)

   /* xf86Init.c */
   SYMFUNC(xf86GetPixFormat)
   SYMFUNC(xf86GetBppFromDepth)
   SYMFUNC(xf86ScanPciRegister)

   /* xf86Mode.c */
   SYMFUNC(xf86GetNearestClock)
   SYMFUNC(xf86ModeStatusToString)
   SYMFUNC(xf86LookupMode)
   SYMFUNC(xf86CheckModeForMonitor)
   SYMFUNC(xf86InitialCheckModeForDriver)
   SYMFUNC(xf86CheckModeForDriver)
   SYMFUNC(xf86ValidateModes)
   SYMFUNC(xf86DeleteMode)
   SYMFUNC(xf86PruneDriverModes)
   SYMFUNC(xf86PruneMonitorModes)
   SYMFUNC(xf86SetCrtcForModes)
   SYMFUNC(xf86PrintModes)
   SYMFUNC(xf86ShowClockRanges)

   /* xf86Option.c */
   SYMFUNC(xf86CollectOptions)
   /* Merging of XInput stuff	*/
   SYMFUNC(xf86AddNewOption)
   SYMFUNC(xf86SetBoolOption)
   SYMFUNC(xf86NewOption)
   SYMFUNC(xf86NextOption)
   SYMFUNC(xf86OptionListCreate)
   SYMFUNC(xf86OptionListMerge)
   SYMFUNC(xf86OptionListFree)
   SYMFUNC(xf86OptionName)
   SYMFUNC(xf86OptionValue)
   SYMFUNC(xf86OptionListReport)
   SYMFUNC(xf86SetIntOption)
   SYMFUNC(xf86SetStrOption)
   SYMFUNC(xf86FindOption)
   SYMFUNC(xf86FindOptionValue)
   SYMFUNC(xf86MarkOptionUsed)
   SYMFUNC(xf86MarkOptionUsedByName)
   SYMFUNC(xf86ShowUnusedOptions)
   SYMFUNC(xf86ProcessOptions)
   SYMFUNC(xf86TokenToOptinfo)
   SYMFUNC(xf86IsOptionSet)
   SYMFUNC(xf86GetOptValString)
   SYMFUNC(xf86GetOptValInteger)
   SYMFUNC(xf86GetOptValULong)
   SYMFUNC(xf86GetOptValReal)
   SYMFUNC(xf86GetOptValBool)
   SYMFUNC(xf86NameCmp)

   /* xf86fbman.c */
   SYMFUNC(xf86InitFBManager)
   SYMFUNC(xf86RegisterFreeBoxCallback)
   SYMFUNC(xf86FreeOffscreenArea)
   SYMFUNC(xf86AllocateOffscreenArea)
   SYMFUNC(xf86ResizeOffscreenArea)
   SYMFUNC(xf86FBManagerRunning)

   /* xf86cmap.c */
   SYMFUNC(xf86HandleColormaps)

   /* xf86xv.c */
   SYMFUNC(xf86XVScreenInit)

   /* Misc */
   SYMFUNC(GetTimeInMillis)

   /* XXX Check these */
   SYMFUNC(xf86MouseInit)
   SYMFUNC(xf86SetKbdRepeat)

#ifdef XINPUT
   SYMFUNC(xf86AlwaysCore)
   SYMFUNC(xf86PostMotionEvent)
   SYMFUNC(xf86PostProximityEvent)		/* Merged from Metrolink tree */
   SYMFUNC(xf86PostButtonEvent)
   SYMFUNC(xf86GetMotionEvents)
   SYMFUNC(xf86MotionHistoryAllocate)

/* The following segment merged from Metrolink tree */
   SYMFUNC(xf86AddLocalDevice)
   SYMFUNC(xf86RemoveLocalDevice)
   SYMFUNC(xf86XInputSetScreen)
   SYMFUNC(xf86ScaleAxis)
   SYMFUNC(xf86XInputProcessOptions)
   SYMFUNC(xf86XInputSetSendCoreEvents)
/* End merged segment */
#endif
#ifdef DPMSExtension
   SYMFUNC(DPMSSet)
#endif

#if 0 /* we want to move the hw stuff in a module */
   SYMFUNC(xf86dactopel)
   SYMFUNC(xf86dactocomm)
   SYMFUNC(xf86getdaccomm)
   SYMFUNC(xf86setdaccomm)
   SYMFUNC(xf86setdaccommbit)
   SYMFUNC(xf86clrdaccommbit)
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
   SYMFUNC(dacOutTi3026IndReg)
   SYMFUNC(dacInTi3026IndReg)
   SYMFUNC(s3OutIBMRGBIndReg)
   SYMFUNC(CirrusFindClock)
   SYMFUNC(CirrusSetClock)
   SYMFUNC(STG1703getIndex)
   SYMFUNC(STG1703setIndex)
   SYMFUNC(STG1703magic)
   SYMFUNC(gendacMNToClock)
   SYMFUNC(Et4000AltICD2061SetClock)
   SYMFUNC(ET4000stg1703SetClock)
   SYMFUNC(ET4000gendacSetClock)

#endif
   
   SYMFUNC(pciFindFirst)
   SYMFUNC(pciFindNext)
   SYMFUNC(pciWriteByte)
   SYMFUNC(pciWriteWord)
   SYMFUNC(pciWriteLong)
   SYMFUNC(pciReadByte)
   SYMFUNC(pciReadWord)
   SYMFUNC(pciReadLong)
   SYMFUNC(pciSetBitsLong)
   SYMFUNC(pciTag)
   SYMFUNC(pciBusAddrToHostAddr)
   SYMFUNC(pciHostAddrToBusAddr)
   SYMFUNC(xf86MapPciMem)
   SYMFUNC(xf86scanpci)
#ifdef __alpha__
   SYMFUNC(xf86MapPciMemSparse)
#endif
   SYMFUNC(xf86ReadPciBIOS)
   SYMFUNC(AllocatePixmapPrivateIndex)
   SYMFUNC(AllocatePixmapPrivate)

   /* Loader functions */
   SYMFUNC(LoaderDefaultFunc)
   SYMFUNC(LoadSubModule)
   SYMFUNC(DuplicateModule)
   SYMFUNC(LoaderErrorMsg)
   SYMFUNC(LoaderCheckUnresolved)
   SYMFUNC(LoadExtension)
   SYMFUNC(LoadFont)
   SYMFUNC(LoaderReqSymbols)
   SYMFUNC(LoaderReqSymLists)
   SYMFUNC(LoaderRefSymLists)
   SYMFUNC(UnloadSubModule)
   SYMFUNC(LoaderSymbol)
   SYMFUNC(LoaderListDirs)
   SYMFUNC(LoaderFreeDirList)

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
   SYMFUNC(xf86printf)
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
   SYMFUNC(xf86hypot)
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
   SYMFUNC(xf86strcasecmp)
   SYMFUNC(xf86strcpy)
   SYMFUNC(xf86strcspn)
   SYMFUNC(xf86strerror)
   SYMFUNC(xf86strlen)
   SYMFUNC(xf86strncmp)
   SYMFUNC(xf86strncasecmp)
   SYMFUNC(xf86strncpy)
   SYMFUNC(xf86strpbrk)
   SYMFUNC(xf86strchr)
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

#if 0
/* Serial access routines */
   SYMFUNC(xf86OpenSerial)
   SYMFUNC(xf86SetSerial)
   SYMFUNC(xf86ReadSerial)
   SYMFUNC(xf86WriteSerial)
   SYMFUNC(xf86CloseSerial)
#endif

#if 0
   SYMFUNC(xf86setexternclock)
#endif
   SYMFUNC(xf86getsecs)
   SYMFUNC(xf86fpossize)      /* for returning sizeof(fpos_t) */
  
#if defined(__alpha__)
   SYMFUNC(__divl)
   SYMFUNC(__reml)
   SYMFUNC(__divlu)
   SYMFUNC(__remlu)
   SYMFUNC(__divq)
   SYMFUNC(__divqu)
   SYMFUNC(__remq)
   SYMFUNC(__remqu)

   SYMFUNC(_outw)
   SYMFUNC(_outb)
   SYMFUNC(_outl)
   SYMFUNC(_inb)
   SYMFUNC(_inw)
   SYMFUNC(_inl)
   SYMFUNC(xf86MapVidMemSparse)
   SYMFUNC(xf86ReadSparse32)
   SYMFUNC(xf86ReadSparse16)
   SYMFUNC(xf86ReadSparse8)
   SYMFUNC(xf86WriteSparse32)
   SYMFUNC(xf86WriteSparse16)
   SYMFUNC(xf86WriteSparse8)
   SYMFUNC(memcpy)
#endif
#if defined(__powerpc__)
   SYMFUNC(inb)
   SYMFUNC(inw)
   SYMFUNC(inl)
   SYMFUNC(outb)
   SYMFUNC(outw)
   SYMFUNC(outl)
#if defined(NO_INLINE) || defined(Lynx)
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
   SYMFUNC(_restf17)
   SYMFUNC(_restf23)
   SYMFUNC(_restf24)
   SYMFUNC(_restf25)
   SYMFUNC(_restf26)
   SYMFUNC(_restf27)
   SYMFUNC(_restf28)
   SYMFUNC(_restf29)
   SYMFUNC(_savef17)
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
   SYMVAR(xf86HUGE_VAL)

#ifdef XF86MISC
   SYMVAR(xf86AllowMouseOpenFail)
   SYMVAR(xf86MiscModInDevAllowNonLocal)
   SYMVAR(xf86MiscModInDevEnabled)
#endif
#ifdef XF86VIDMODE
   SYMVAR(xf86VidModeEnabled)
   SYMVAR(xf86VidModeAllowNonLocal)
#endif
   /* General variables (from xf86.h) */
   SYMVAR(xf86ScreenIndex)
   SYMVAR(xf86PixmapIndex)
   SYMVAR(xf86Screens)
   SYMVAR(byte_reversed)

#if defined(__powerpc__) && (!defined(NO_INLINE) || defined(Lynx))
   SYMVAR(ioBase)
#endif

  { 0, 0 },

};

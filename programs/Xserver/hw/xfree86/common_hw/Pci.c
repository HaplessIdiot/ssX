/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/Pci.c,v 3.4 1998/03/20 21:06:24 hohndel Exp $ */
/*
 * Pci.c - New server PCI access functions
 *
 * The Xfree server PCI access functions have been reimplemented as a
 * framework that allows each supported platform/OS to have their own
 * platform/OS specific pci driver.
 *
 * All of the public PCI access functions exported to the other parts of
 * the server are declared in Pci.h and defined herein.  These include:
 * 	pciInit()      - Initialize PCI access functions
 * 	pciFindFirst() - Find a PCI device by dev/vend id
 * 	pciFindNext()  - Find another PCI device by dev/vend id
 *	pciEnableIO()  - Enable/Map resources needed to R/W PCI cfg space
 *	pciDisableIO() - Disable/Unmap resources needed to R/W PCI cfg space
 * 	pciReadLong()  - Read a 32 bit value from a PCI devices cfg space
 * 	pciReadWord()  - Read a 16 bit value from a PCI devices cfg space
 * 	pciReadByte()  - Read an 8 bit value from a PCI devices cfg space
 * 	pciWriteLong() - Write a 32 bit value to a PCI devices cfg space
 * 	pciWriteWord() - Write a 16 bit value to a PCI devices cfg space
 * 	pciWriteByte() - Write an 8 bit value to a PCI devices cfg space
 * 	pciTag()       - Return tag for a given PCI bus, device, & function
 * 	pciBusAddrToHostAddr() - Convert a PCI address to a host address
 * 	pciHostAddrToBusAddr() - Convert a host address to a PCI address
 *      xf86MapPciMem() - Like xf86MapVidMem() expect function expects
 *                        a PCI address and PCITAG (identifies PCI domain)
 *
 * For compatibility with older drivers, the following functions are
 * also provided
 *	xf86scanpci()  - Return info about all PCI devices
 *      xf86writepci() - Write to a devices PCI cfg space
 * 	xf86cleanpci() - Free data structures allocated by xf86scanpci()
 * 
 * The actual PCI backend driver is selected by the pciInit() function
 * (see below)  using either compile time definitions, run-time checks,
 * or both.
 *
 * Certain generic functions are provided that make the implementation 
 * of certain well behaved platforms (e.g. those supporting PCI config
 * mechanism 1 or some thing close to it) very easy.
 *
 * Less well behaved platforms/OS's can roll their own functions.
 *
 * To add support for another platform/OS, add a call to fooPciInit() within
 * pciInit() below under the correct compile time definition or run-time
 * conditional.
 *
 *
 * The fooPciInit() procedure must do three things:
 * 	1) Initialize the pciBusTable[] for all primary PCI buses including
 *	   the per domain PCI access functions (readLong, writeLong,
 *         addrBusToHost, and addrHostToBus).  
 *
 *	2) Add entries to pciBusTable[] for configured secondary buses.  This
 *	   step may be skipped if a platform is using the generic findFirst/
 *	   findNext functions because these procedures will automatically
 *	   discover and add secondary buses dynamically.
 *
 *      3) Overide default settings for global PCI access functions if
 *         required. These include pciFindFirstFP, pciFindNextFP, pciIOEnableFP,
 *         pciIODisableFP.  Of course, if you choose not to use one of the generic
 *         functions, you will need to provide a platform specifc replacement.
 *
 * See xc/programs/Xserver/hw/xfree86/os-support/pmax/pmax_pci.c for an example
 * of how to extend this framework to other platforms/OSes.
 *
 * Gary Barton
 * Concurrent Computer Corporation
 * garyb@gate.net
 *
 */

/*
 * Copyright 1998 by Concurrent Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Concurrent Computer
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Concurrent Computer Corporation makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 * 
 * CONCURRENT COMPUTER CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CONCURRENT COMPUTER CORPORATION BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 * Copyright 1998 by Metro Link Incorporated
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Metro Link
 * Incorporated not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Metro Link Incorporated makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 * 
 * METRO LINK INCORPORATED DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL METRO LINK INCORPORATED BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 * This software is derived from the original XFree86 PCI code
 * which includes the following copyright notices as well:
 *
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holder(s)
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holder(s) make(s) no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM(S) ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * This code is also based heavily on the code in FreeBSD-current, which was
 * written by Wolfgang Stanglmeier, and contains the following copyright:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <errno.h>
#include <signal.h>
#include "Xarch.h"
#include "os.h"
#include "compiler.h"
#include "input.h"
#include "xf86Procs.h"
#include "xf86_ansic.h"
#include "Pci.h"

#define PCI_MFDEV_SUPPORT   1 /* Include PCI multifunction device support */
#define PCI_BRIDGE_SUPPORT  1 /* Include support for PCI-to-PCI bridges */

#ifdef PC98
#define outb(port,data) _outb(port,data)
#define outl(port,data) _outl(port,data)
#define inb(port) _inb(port)
#define inl(port) _inl(port)
#endif

/*
 * Global data
 */
static int    pciInitialized = 0;

CARD32 pciDevid;            /* Requested device/vendor ID (after mask) */
CARD32 pciDevidMask;        /* Bit mask applied (AND) before comparison */
				   /* of real devid's with requested           */

int    pciBusNum;           /* Bus Number of current device */
int    pciDevNum;           /* Device number of current device */
int    pciFuncNum;          /* Function number of current device */
PCITAG pciDeviceTag;        /* Tag for current device */

pciBusFuncs_t pciNOOPFuncs = {
	pciReadLongNULL,
	pciWriteLongNULL,
	pciAddrNOOP,
	pciAddrNOOP
};
		
pciBusInfo_t  *pciBusInfo[MAX_PCI_BUSES];
int            pciNumBuses = 0;     /* Actual number of PCI buses */

static pciConfigPtr pci_devp[MAX_PCI_DEVICES + 1] = {NULL, };
static pciConfigPtr pci_locked_devp[MAX_PCI_DEVICES + 1] = {NULL, };

/*
 * Platform specific PCI function pointers.
 *
 * NOTE: A platform/OS specific pci init procedure can override these defaults
 *       by setting them to the appropriate platform dependent functions.
 */
PCITAG     (*pciFindFirstFP)(void) = pciGenFindFirst;
PCITAG     (*pciFindNextFP)(void) = pciGenFindNext;
void       (*pciIOEnableFP)(int) = NULL;
void       (*pciIODisableFP)(int) = NULL;

/*
 * Select architecure specific PCI init function
 */
#if defined(__powerpc__)
# define ARCH_PCI_INIT ppcPciInit
# define INCLUDE_OLD_PCI_FUNCS
# if !defined(PowerMAX_OS)
#  define INCLUDE_XF86_MAP_PCI_MEM
# endif 
#elif defined(__alpha__)
# define ARCH_PCI_INIT axpPciInit
# define INCLUDE_OLD_PCI_FUNCS
# define INCLUDE_XF86_MAP_PCI_MEM
#else
# define ARCH_PCI_INIT ix86PciInit
# define INCLUDE_OLD_PCI_FUNCS
# define INCLUDE_XF86_MAP_PCI_MEM
#endif

#if NeedFunctionPrototypes
extern void ARCH_PCI_INIT(void);
#else
extern void ARCH_PCI_INIT();
#endif

/*
 * pciInit - choose correct platform/OS specific PCI init routine
 */
void
pciInit()
{
	if (pciInitialized)
		return;

	pciInitialized = 1;

#if defined(DEBUGPCI)
	if (DEBUGPCI >= xf86Verbose)
		xf86Verbose = DEBUGPCI;
#endif

	ARCH_PCI_INIT();	
}

  
PCITAG
pciFindFirst(unsigned long id, unsigned long mask)
{
#ifdef DEBUGPCI
ErrorF("pciFindFirst(0x%lx, 0x%lx), pciInit = %d\n", id, mask, pciInitialized);
#endif
  if (!pciInitialized)
    pciInit();

  pciDevid = id & mask;
  pciDevidMask = mask;

  return((*pciFindFirstFP)());
}

PCITAG
pciFindNext(void)
{
#ifdef DEBUGPCI
ErrorF("pciFindNext(), pciInit = %d\n", pciInitialized);
#endif
  if (!pciInitialized)
    pciInit();
    
  return((*pciFindNextFP)());
}

void
pciEnableIO(int scrNdx)
{
  if (!pciInitialized)
    pciInit();

  if (pciIOEnableFP)
    (*pciIOEnableFP)(scrNdx);
}	

void
pciDisableIO(int scrNdx)
{
  if (!pciInitialized)
    pciInit();
    
  if (pciIODisableFP)
    (*pciIODisableFP)(scrNdx);
}	

CARD32
pciReadLong(PCITAG tag, int offset)
{
  int bus = PCI_BUS_FROM_TAG(tag);
  int dev = PCI_DEV_FROM_TAG(tag);
  int func = PCI_FUNC_FROM_TAG(tag);

#ifdef DEBUGPCI
ErrorF("pciReadLong(0x%lx, %d)\n", tag, offset);
#endif
  if (!pciInitialized)
    pciInit();

  if (bus < pciNumBuses && pciBusInfo[bus] && pciBusInfo[bus]->funcs.pciReadLong) {
    CARD32 rv = (*pciBusInfo[bus]->funcs.pciReadLong)(tag, offset);

    PCITRACE(1, ("pciReadLong: tag=0x%x [b=%d,d=%d,f=%d] returns 0x%08x\n",
		 tag, bus, dev, func, rv));
    return(rv);
   }

  return(PCI_NOT_FOUND);
}

CARD16
pciReadWord(PCITAG tag, int offset)
{
  CARD32 tmp;
  int    shift = (offset & 3) * 8;
  int    aligned_offset = offset & ~3;
  
  if (shift != 0 && shift != 16)
	  FatalError("pciReadWord: Alignment error: Cannot read 16 bits at offset %d\n", offset);
  
  tmp = pciReadLong(tag, aligned_offset);
  
  return((CARD16)((tmp >> shift) & 0xffff));
}

CARD8
pciReadByte(PCITAG tag, int offset)
{
  CARD32 tmp;
  int    shift = (offset & 3) * 8;
  int    aligned_offset = offset & ~3;
  
  tmp = pciReadLong(tag, aligned_offset);
  
  return((CARD8)((tmp >> shift) & 0xff));
}

void
pciWriteLong(PCITAG tag, int offset, CARD32 val)
{
  int bus = PCI_BUS_FROM_TAG(tag);
	
  if (!pciInitialized)
    pciInit();
  
  if (bus < pciNumBuses && pciBusInfo[bus] && pciBusInfo[bus]->funcs.pciWriteLong)
	  (*pciBusInfo[bus]->funcs.pciWriteLong)(tag, offset, val);
}
  
void
pciWriteWord(PCITAG tag, int offset, CARD16 val)
{ 
  CARD32 tmp;
  int    aligned_offset = offset & ~3;
  int    shift = (offset & 3) * 8;

  if (shift != 0 && shift != 16)
	  FatalError("pciWriteWord: Alignment Error: Cannot read 16 bits from offset %d\n", offset);
  
  tmp = pciReadLong(tag, aligned_offset);
  
  tmp &= ~(0xffffL << shift);
  tmp |= (((CARD32)val) << shift);
  
  pciWriteLong(tag, offset, tmp);
}

void
pciWriteByte(PCITAG tag, int offset, CARD8 val)
{ 
  CARD32 tmp;
  int    aligned_offset = offset & ~3;
  int    shift = (offset & 3) *8 ;
  
  tmp = pciReadLong(tag, aligned_offset);
  
  tmp &= ~(0xffL << shift);
  tmp |= (((CARD32)val) << shift);
  
  pciWriteLong(tag, offset, tmp);
} 
  
ADDRESS
pciBusAddrToHostAddr(PCITAG tag, ADDRESS addr)
{
  int bus = PCI_BUS_FROM_TAG(tag);
	
  if (!pciInitialized)
    pciInit();
  
  if (bus < pciNumBuses && pciBusInfo[bus] && pciBusInfo[bus]->funcs.pciAddrBusToHost)
	  (*pciBusInfo[bus]->funcs.pciAddrBusToHost)(tag, addr);
  else
	  return(addr);
}

ADDRESS
pciHostAddrToBusAddr(PCITAG tag, ADDRESS addr)
{
  int bus = PCI_BUS_FROM_TAG(tag);
	
  if (!pciInitialized)
    pciInit();
  
  if (bus < pciNumBuses && pciBusInfo[bus] && pciBusInfo[bus]->funcs.pciAddrHostToBus)
	  (*pciBusInfo[bus]->funcs.pciAddrHostToBus)(tag, addr);
  else
	  return(addr);
}

PCITAG
pciTag(int busnum, int devnum, int funcnum)
{
	return(PCI_MAKE_TAG(busnum,devnum,funcnum));
}

Bool
pciMfDev(int busnum, int devnum)
{
    PCITAG tag0, tag1;
    unsigned long id0, id1;
 
    /* Detect a multi-function device that complies to the PCI 2.0 spec */
 
    tag0 = PCI_MAKE_TAG(busnum, devnum, 0);
    id0  = pciReadLong(tag0, PCI_ID_REG);
    if (id0 == 0xffffffff)
        return FALSE;

    if (pciReadLong(tag0, PCI_HEADER_MISC) & PCI_HEADER_MULTIFUNCTION)
        return TRUE;
 
    /*
     * Now, to find non-compliant devices...
     * If there is a valid ID for function 1 and the ID for func 0 and 1
     * are different, or the base0 values of func 0 and 1 are differend,
     * then assume there is a multi-function device.
     */
    tag1 = PCI_MAKE_TAG(busnum, devnum, 1);
    id1  = pciReadLong(tag1, PCI_ID_REG);
    if (id1 = 0xffffffff)
	return FALSE;
 
    if ((id0 != id1) || 
        (pciReadLong(tag0, PCI_MAP_REG_START) != pciReadLong(tag1, PCI_MAP_REG_START)))
        return TRUE;

    return FALSE;
}

/*
 * Generic find/read/write functions
 */
PCITAG
pciGenFindNext(void)
{
  unsigned long devid, tmp;
  unsigned char base_class, sub_class, sec_bus, pri_bus;
  
#ifdef DEBUGPCI
ErrorF("pciGenFindNext\n");
#endif

  for (;;) {

#ifdef DEBUGPCI
ErrorF("pciGenFindNext: pciBusNum %d\n", pciBusNum);
#endif
    if (pciBusNum == -1) {
	    /*
	     * Start at top of the order
	     */
	    pciBusNum = 0;
	    pciFuncNum = 0;
	    pciDevNum = 0;
    }
    else {
#ifdef PCI_MFDEV_SUPPORT
#ifdef DEBUGPCI
ErrorF("pciGenFindNext: pciFuncNum %d\n", pciFuncNum);
#endif
	    /*
	     * Somewhere in middle of order.  Determine who's
	     * next up
	     */
	    if (pciFuncNum == 0) {
		    /*
		     * Is current dev a multifunction device?
		     */
		    if (pciMfDev(pciBusNum, pciDevNum))
			    /* Probe for other functions */
			    pciFuncNum = 1;
		    else
			    /* No more functions this device. Next device please */
			    pciDevNum ++;
	    }
	    else if (++pciFuncNum >= 8) {
		    /* No more functions for this device. Next device please */
		    pciFuncNum = 0;
		    pciDevNum ++;
	    }
#else
	    pciDevNum ++;
#endif
	    if (pciDevNum >= 32 ||
		!pciBusInfo[pciBusNum] ||
		pciDevNum >= pciBusInfo[pciBusNum]->numDevices) {
#ifdef DEBUGPCI
ErrorF("pciGenFindNext: next bus\n");
#endif
		    /*
		     * No more devices for this bus. Next bus please
		     */
		    if (++pciBusNum >= pciNumBuses) {
#ifdef DEBUGPCI
ErrorF("pciGenFindNext: out of buses\n");
#endif
			    /* No more buses.  All done for now */
			    return(PCI_NOT_FOUND);
		    }
		    
		    pciDevNum = 0;
	    }
    }

#ifdef DEBUGPCI
ErrorF("pciGenFindNext: pciBusInfo[%d] = 0x%lx\n", pciBusNum, pciBusInfo[pciBusNum]);
#endif
    if (!pciBusInfo[pciBusNum])
	    continue;	    /* Bus not defined, next please */
    
    /*
     * At this point, pciBusNum, pciDevNum, and pciFuncNum have been
     * advanced to the next device.  Compute the tag, and read the
     * device/vendor ID field.
     */
#ifdef DEBUGPCI
ErrorF("pciGenFindNext: [%d, %d, %d]\n", pciBusNum, pciDevNum, pciFuncNum);
#endif
    pciDeviceTag = PCI_MAKE_TAG(pciBusNum, pciDevNum, pciFuncNum);
    devid = pciReadLong(pciDeviceTag, 0);
#ifdef DEBUGPCI
ErrorF("pciGenFindNext: pciDeviceTag = 0x%lx, devid = 0x%lx\n", pciDeviceTag, devid);
#endif
    if (devid == 0xffffffff)
	    continue; /* Nobody home.  Next device please */

    /*
     * Before checking for a specific devid, look for enabled
     * PCI to PCI bridge devices.  If one is found, create and
     * initialize a bus info record (if one does not already exist).
     */
#ifdef PCI_BRIDGE_SUPPORT    
    tmp = pciReadLong(pciDeviceTag, PCI_CLASS_REG);
    base_class = PCI_CLASS_EXTRACT(tmp);
    sub_class = PCI_SUBCLASS_EXTRACT(tmp); 
    if (base_class == PCI_CLASS_BRIDGE && sub_class == PCI_SUBCLASS_BRIDGE_PCI) {
	    tmp = pciReadLong(pciDeviceTag, PCI_PCI_BRIDGE_BUS_REG);
	    sec_bus = PCI_SECONDARY_BUS_EXTRACT(tmp);
	    pri_bus = PCI_PRIMARY_BUS_EXTRACT(tmp);
#ifdef DEBUGPCI
ErrorF("pciGenFindNext: pri_bus %d sec_bus %d\n", pri_bus, sec_bus);
#endif
	    if (sec_bus > 0 && sec_bus < MAX_PCI_BUSES && pciBusInfo[pri_bus]) {
		    /*
		     * Found a secondary PCI bus
		     */
		    if (!pciBusInfo[sec_bus]) {
			    pciBusInfo[sec_bus] = (pciBusInfo_t *)Xalloc(sizeof(pciBusInfo_t));

			    if (!pciBusInfo[sec_bus])
				    FatalError("pciGenFindNext: Xalloc failed\n!!!");
		    }

		    /* Copy parents settings... */
		    *pciBusInfo[sec_bus] = *pciBusInfo[pri_bus];

		    /* ...but not everything same as parent */
		    pciBusInfo[sec_bus]->primary_bus = pri_bus;
		    pciBusInfo[sec_bus]->secondary = TRUE;
		    pciBusInfo[sec_bus]->numDevices = 32;

		    if (pciNumBuses <= sec_bus)
			    pciNumBuses = sec_bus+1;
	    }
    }
#endif
    
    /*
     * Does this device match the requested devid after
     * applying mask?
     */
#ifdef DEBUGPCI
ErrorF("pciGenFindNext: pciDevidMask = 0x%lx, pciDevid = 0x%lx\n", pciDevidMask, pciDevid);
#endif
    if ((devid & pciDevidMask) == pciDevid)
	  /* Yes - Return it.  Otherwise, next device */
	  return(pciDeviceTag); /* got a match */

  } /* for */
  /*NOTREACHED*/
}

PCITAG
pciGenFindFirst(void)
{
  /* Reset PCI bus number to start from top */	
  pciBusNum = -1;
  
  return pciGenFindNext();
}

static int buserr_detected;

static 
void buserr(int sig)
{
	buserr_detected = 1;
}

unsigned long
pciCfgMech1Read(PCITAG tag, int offset)
{
  unsigned long rv = 0xffffffff;

#if defined(__powerpc__)
  signal(SIGBUS, buserr);
  buserr_detected = 0;
#endif

  outl(0xCF8, PCI_EN | tag | (offset & 0xfc));
  rv = inl(0xCFC);

#if defined(__powerpc__)
  signal(SIGBUS,SIG_DFL);
  if (buserr_detected)
    return(0xffffffff);
  else
#endif
    return(rv);
}

void
pciCfgMech1Write(PCITAG tag, int offset, unsigned long val)
{
#if defined(__powerpc__)
  signal(SIGBUS, SIG_IGN);
#endif

  outl(0xCF8, PCI_EN | tag | (offset & 0xfc));
  outl(0xCFC, val);

#if defined(__powerpc__)
  signal(SIGBUS, SIG_DFL);
#endif
}

unsigned long
pciByteSwap(unsigned long u)
{
#if BYTE_ORDER == BIG_ENDIAN
# if defined(__powerpc__) && defined(PowerMAX_OS)
  unsigned long tmp;

  __inst_stwbrx(u, &tmp, 0);

  return(tmp);
	
# else /* !PowerMAX_OS */

  return lswapl(u);

# endif /* !PowerMAX_OS */

#else /* !BIG_ENDIAN */
  
  return(u);
  
#endif
}

/*
 * Dummy functions to noop the PCI services
 */
unsigned long
pciReadLongNULL(PCITAG tag, int offset)
{
	return(0xffffffff);
}

void
pciWriteLongNULL(PCITAG tag, int offset, unsigned long val)
{
}

ADDRESS
pciAddrNOOP(PCITAG tag, ADDRESS addr)
{
	return(addr);
}


#if defined(INCLUDE_OLD_PCI_FUNCS)

pciConfigPtr *
xf86scanpci(int scrnIndex)
{
    int idx = 0;
    PCITAG tag;

    if (pci_devp[0])
	return pci_devp;

    pciInit();

    pciEnableIO(scrnIndex);

    tag = pciFindFirst(0,0);  /* 0 mask means match any valid device */

#ifdef DEBUGPCI
ErrorF("xf86scanpci: tag = 0x%lx\n", tag);
#endif
    while (idx < MAX_PCI_DEVICES && tag != PCI_NOT_FOUND) {
	    pciConfigPtr devp;
	    int          i;
	    
	    devp = (pciConfigPtr)xalloc(sizeof(pciDevice));
	    if (!devp) {
		    ErrorF("xf86scanpci: Out of memory after %d devices!!\n", idx);
		    pciDisableIO(scrnIndex);
		    xf86cleanpci();
		    return (pciConfigPtr *)NULL;
	    }

	    /* Identify pci device by bus, dev, func, and tag */
	    devp->tag = tag;
	    devp->busnum = PCI_BUS_FROM_TAG(tag);
	    devp->devnum = PCI_DEV_FROM_TAG(tag);
	    devp->funcnum = PCI_FUNC_FROM_TAG(tag);
	    
	    /* Read config space for this device */
	    for (i = 0; i < 17; i++)  /* PCI hdr plus 1st dev spec dword */
		    devp->cfgspc.dwords[i] = pciReadLong(tag, i * sizeof(CARD32));

	    if (xf86Verbose > 1) {
		    ErrorF("PCI: Tag 0x%08x [b=%d,d=%d,f=%d], ID 0x%04x,"
			   "0x%04x Rev 0x%02x Class 0x%02x,0x%02x\n",
			   devp->tag, devp->busnum, devp->devnum, devp->funcnum,
			   devp->_vendor, devp->_device, devp->_rev_id,
			   devp->_base_class, devp->_sub_class);
	    }

	    pci_devp[idx++] = devp;
	    tag = pciFindNext();
#ifdef DEBUGPCI
ErrorF("xf86scanpci: tag = pciFindNext = 0x%lx\n", tag);
#endif
    }

    /* pciDisableIO(scrnIndex); */
    return pci_devp;
}

void
xf86writepci(int scrnIndex, int bus, int dev, int func, int reg,
	     CARD32 mask, CARD32 value)
{
    PCITAG tag;
    CARD32 data;

    pciInit();

    pciEnableIO(scrnIndex);
    
    tag = pciTag(bus, dev, func);
    data = (pciReadLong(tag, reg) & ~mask) | (value & mask);
    pciWriteLong(tag, reg, data);

    if (xf86Verbose > 2) {
	ErrorF("PCI: xf86writepci: Tag=0x%08x [B=%d,D=%d,F=%d] Reg=0x%02x "
	       "Mask=0x%08x Val=0x%08x\n",
	       tag, bus, dev, func, reg, mask, value);
    }

    pciDisableIO(scrnIndex);
}

void
xf86cleanpci()
{
	int i;

#ifdef DEBUGPCI
ErrorF("xf86cleanpci\n");
#endif
	for (i = 0; i < MAX_PCI_DEVICES; i++) {
		if (pci_devp[i]) {
			xfree((pointer)pci_devp[i]);
			pci_devp[i] = NULL;
		}
		
		if (pci_locked_devp[i]) {
			xfree((pointer)pci_locked_devp[i]);
			pci_devp[i] = NULL;
		}
	}
}

int
xf86lockpci(int bus, int device, int function)
{
    pciConfigPtr *p, *q;
    int i;
    int ret = -1;

#ifdef DEBUGPCI
ErrorF("xf86lockpci(%d, %d, %d)\n", bus, device, function);
#endif

    for (i = 0, p = pci_devp; *p; p++, i++) {
	if ((*p)->busnum == bus
	    && (*p)->devnum == device
	    && (*p)->funcnum == function) {
	    /* put on locked list */
	    for (q = pci_locked_devp; *q; q++)
		;
	    *q = *p;
	    memmove(&(pci_devp[i]), &(pci_devp[i+1]),
		    (MAX_PCI_DEVICES - i) * sizeof (pciConfigPtr));
	    pci_devp[MAX_PCI_DEVICES] = 0;
	    ret = 0;
	    break;
	}
    }
    return ret;
}

#endif /* INCLUDE_OLD_PCI_FUNCS */

#if defined(INCLUDE_XF86_MAP_PCI_MEM)

#ifndef MAP_FAILED
#define MAP_FAILED (pointer)(-1)
#endif

pointer
xf86MapPciMem(int ScreenNum, int Region, PCITAG Tag, pointer Base, unsigned long Size)
{
	pointer hostbase = pciBusAddrToHostAddr(Tag, Base);
	pointer base;

	base = xf86MapVidMem(ScreenNum, Region, hostbase, Size);
	if (base == MAP_FAILED)	{
		FatalError("%s: Could not mmap PCI memory [base=0x%x,hostbase=0x%x,size=%x] (%s)\n",
			   "xf86MapPciMem", Base, hostbase, Size, strerror(errno));
	}
	return((pointer)base);
}

int
xf86ReadPciBIOS(unsigned long Base, unsigned long Offset, PCITAG Tag, unsigned char *Buf, int Len)
{
    pointer hostbase = pciBusAddrToHostAddr(Tag, (ADDRESS)Base);
    
    return(xf86ReadBIOS(Base, Offset, Buf, Len));
}

#endif /* INCLUDE_XF86_MAP_PCI_MEM */


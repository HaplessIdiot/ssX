/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/sparcPci.c,v 1.5.2.1 2001/11/28 22:35:38 tsi Exp $ */
/*
 * Copyright (C) 2001 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "Pci.h"
#include "xf86sbusBus.h"

extern char *apertureDevName;
static int  apertureFd = -1;

/*
 * A version of xf86MapVidMem() that allows for 64-bit displacements (but not
 * sizes).  Areas thus mapped can be unmapped by xf86UnMapVidMem().
 */
static pointer
sparcMapAperture(int iScreen, int Flags,
		 unsigned long long Base, unsigned long Size)
{
    pointer result;

    /* Assume both Base & Size are multiples of the page size */

    if (apertureFd < 0) {
	apertureFd = open(apertureDevName,
	    (Flags & VIDMEM_READONLY) ? O_RDONLY : O_RDWR);
	if (apertureFd < 0)
	    FatalError("sparcMapAperture:  open failure:  %s\n",
		       strerror(errno));
    }

    result = mmap(NULL, Size,
		  (Flags & VIDMEM_READONLY) ?
		      PROT_READ : (PROT_READ | PROT_WRITE),
		  MAP_SHARED, apertureFd, (off_t)Base);

    if (result == MAP_FAILED)
	FatalError("sparcMapAperture:  mmap failure:  %s\n", strerror(errno));

    return result;
}

/*
 * Platform-specific bus privates.
 */
typedef struct _sparcDomainRec {
    unsigned long long io_addr, io_size;
    unsigned long long mem_addr, mem_size;
    pointer pci, io;
    int bus_min, bus_max;
    unsigned char dfn_mask[256 / 8];
} sparcDomainRec, *sparcDomainPtr;

#define SetBitInMap(bit, map) \
    do { \
	int _bit = (bit); \
	(map)[_bit >> 3] |= 1 << (_bit & 7); \
    } while (0)

#define IsBitSetInMap(bit, map) \
    ((map)[(bit) >> 3] & (1 << ((bit) & 7)))

/*
 * Domain 0 is reserved for the one that represents the system as a whole, i.e.
 * the one without any resource relocations.
 */
#define MAX_DOMAINS (MAX_PCI_BUSES / 256)
static sparcDomainPtr xf86DomainInfo[MAX_DOMAINS];
static int            pciNumDomains = 1;

/* Variables that are assigned this must be declared volatile */
#define PciReg(base, tag, off, type) \
    *(volatile type *)(pointer)((char *)(base) + \
	(PCI_TAG_NO_DOMAIN(tag) | (off)))

/* Generic SPARC PCI access functions */
static CARD32
sparcPciCfgRead32(PCITAG tag, int off)
{
    pciBusInfo_t    *pBusInfo;
    sparcDomainPtr  pDomain;
    volatile CARD32 result = (CARD32)(-1);	/* Must be volatile */
    int             bus;

    if ((off >= 0) && (off <= 252) && !(off & 3) &&
	((bus = PCI_BUS_FROM_TAG(tag)) < pciNumBuses) &&
	(pBusInfo = pciBusInfo[bus]) && (pDomain = pBusInfo->pciBusPriv) &&
	(bus >= pDomain->bus_min) && (bus < pDomain->bus_max) &&
	((bus > pDomain->bus_min) ||
	 IsBitSetInMap(PCI_DFN_FROM_TAG(tag), pDomain->dfn_mask))) {
	result = PciReg(pDomain->pci, tag, off, CARD32);

	result = PCI_CPU(result);
    }

    return result;
}

static void
sparcPciCfgWrite32(PCITAG tag, int off, CARD32 val)
{
    pciBusInfo_t   *pBusInfo;
    sparcDomainPtr pDomain;
    int            bus;

    if ((off < 0) || (off > 252) || (off & 3) ||
	((bus = PCI_BUS_FROM_TAG(tag)) >= pciNumBuses) ||
	!(pBusInfo = pciBusInfo[bus]) || !(pDomain = pBusInfo->pciBusPriv) ||
	(bus < pDomain->bus_min) || (bus >= pDomain->bus_max) ||
	((bus == pDomain->bus_min) &&
	 !IsBitSetInMap(PCI_DFN_FROM_TAG(tag), pDomain->dfn_mask)))
	return;

    val = PCI_CPU(val);
    PciReg(pDomain->pci, tag, off, CARD32) = val;
}

static void
sparcPciCfgSetBits32(PCITAG tag, int off, CARD32 mask, CARD32 bits)
{
    CARD32 PciVal;

    PciVal = sparcPciCfgRead32(tag, off);
    PciVal &= ~mask;
    PciVal |= bits;
    sparcPciCfgWrite32(tag, off, PciVal);
}

static pciBusFuncs_t sparcPCIFunctions =
{
    sparcPciCfgRead32,
    sparcPciCfgWrite32,
    sparcPciCfgSetBits32,
    pciAddrNOOP,
    pciAddrNOOP
};

/*
 * Sabre-specific versions of the above because of its peculiar access size
 * requirements.
 */
static CARD32
sabrePciCfgRead32(PCITAG tag, int off)
{
    pciBusInfo_t    *pBusInfo;
    sparcDomainPtr  pDomain;
    volatile CARD32 result;			/* Must be volatile */
    int             bus;

    if (PCI_BDEV_FROM_TAG(tag))
	return sparcPciCfgRead32(tag, off);

    if (PCI_FUNC_FROM_TAG(tag) || (off < 0) || (off > 252) || (off & 3) ||
	((bus = PCI_BUS_FROM_TAG(tag)) >= pciNumBuses) ||
	!(pBusInfo = pciBusInfo[bus]) || !(pDomain = pBusInfo->pciBusPriv) ||
	(bus != pDomain->bus_min))
	return (CARD32)(-1);

    if (off < 8) {
	result = (PciReg(pDomain->pci, tag, off, CARD16) << 16) |
		  PciReg(pDomain->pci, tag, off + 2, CARD16);
	return PCI_CPU(result);
    }

    result = (PciReg(pDomain->pci, tag, off + 3, CARD8) << 24) |
	     (PciReg(pDomain->pci, tag, off + 2, CARD8) << 16) |
	     (PciReg(pDomain->pci, tag, off + 1, CARD8) <<  8) |
	     (PciReg(pDomain->pci, tag, off    , CARD8)      );
    return result;
}

static void
sabrePciCfgWrite32(PCITAG tag, int off, CARD32 val)
{
    pciBusInfo_t   *pBusInfo;
    sparcDomainPtr pDomain;
    int            bus;

    if (PCI_BDEV_FROM_TAG(tag))
	sparcPciCfgWrite32(tag, off, val);
    else if (!PCI_FUNC_FROM_TAG(tag) &&
	     (off >= 0) && (off <= 252) && !(off & 3) &&
	     ((bus = PCI_BUS_FROM_TAG(tag)) < pciNumBuses) &&
	     (pBusInfo = pciBusInfo[bus]) &&
	     (pDomain = pBusInfo->pciBusPriv) &&
	     (bus == pDomain->bus_min)) {
	if (off < 8) {
	    val = PCI_CPU(val);
	    PciReg(pDomain->pci, tag, off    , CARD16) = val >> 16;
	    PciReg(pDomain->pci, tag, off + 2, CARD16) = val;
	} else {
	    PciReg(pDomain->pci, tag, off    , CARD8) = val;
	    PciReg(pDomain->pci, tag, off + 1, CARD8) = val >> 8;
	    PciReg(pDomain->pci, tag, off + 2, CARD8) = val >> 16;
	    PciReg(pDomain->pci, tag, off + 3, CARD8) = val >> 24;
	}
    }
}

static void
sabrePciCfgSetBits32(PCITAG tag, int off, CARD32 mask, CARD32 bits)
{
    CARD32 PciVal;

    PciVal = sabrePciCfgRead32(tag, off);
    PciVal &= ~mask;
    PciVal |= bits;
    sabrePciCfgWrite32(tag, off, PciVal);
}

static pciBusFuncs_t sabrePCIFunctions =
{
    sabrePciCfgRead32,
    sabrePciCfgWrite32,
    sabrePciCfgSetBits32,
    pciAddrNOOP,
    pciAddrNOOP
};

static int pagemask;

/* Scan PROM for all PCI host bridges in the system */
void
sparcPciInit(void)
{
    int node, node2;

    if (!xf86LinearVidMem())
	return;

    apertureFd = open(apertureDevName, O_RDWR);
    if (apertureFd < 0) {
	xf86Msg(X_ERROR,
	    "sparcPciInit:  open failure:  %s\n", strerror(errno));
	return;
    }

    sparcPromInit();
    pagemask = xf86getpagesize() - 1;

    for (node = promGetChild(promRootNode);
	 node;
	 node = promGetSibling(node)) {
	unsigned long long pci_addr;
	sparcDomainRec     domain;
	sparcDomainPtr     pDomain;
	pciBusFuncs_t      *pFunctions;
	char               *prop_val;
	int                prop_len, bus;

	prop_val = promGetProperty("name", &prop_len);
	/* Some PROMs include the trailing null;  some don't */
	if (!prop_val || (prop_len < 3) || (prop_len > 4) ||
	    strcmp(prop_val, "pci"))
	    continue;

	prop_val = promGetProperty("model", &prop_len);
	if (!prop_val || (prop_len <= 0)) {
	    prop_val = promGetProperty("compatible", &prop_len);
	    if (!prop_val || (prop_len <= 0))
		continue;
	}

	pFunctions = &sparcPCIFunctions;
	(void)memset(&domain, 0, sizeof(domain));

	if (!strncmp("SUNW,sabre",   prop_val, prop_len) ||
	    !strncmp("pci108e,a000", prop_val, prop_len) ||
	    !strncmp("pci108e,a001", prop_val, prop_len)) {
	    /*
	     * There can only be one "Sabre" bridge in a system.  It provides
	     * PCI configuration space, a 24-bit I/O space and a 32-bit memory
	     * space, all three of which are at fixed physical CPU addresses.
	     */
	    static Bool sabre_seen = FALSE;

	    xf86Msg(X_INFO,
		"Sabre or Hummingbird PCI host bridge found (\"%s\")\n",
		prop_val);

	    /* There can only be one Sabre */
	    if (sabre_seen)
		continue;
	    sabre_seen = TRUE;

	    /* Get "bus-range" property */
	    prop_val = promGetProperty("bus-range", &prop_len);
	    if (!prop_val || (prop_len != 8) ||
		(((unsigned int *)prop_val)[0]) ||
		(((unsigned int *)prop_val)[1] >= 256))
		continue;

	    pci_addr         = 0x01fe01000000ull;
	    domain.io_addr   = 0x01fe02000000ull;
	    domain.io_size   = 0x000001000000ull;
	    domain.mem_addr  = 0x01ff00000000ull;
	    domain.mem_size  = 0x000100000000ull;
	    domain.bus_min   = 0;			/* Always */
	    domain.bus_max   = ((int *)prop_val)[1];

	    pFunctions = &sabrePCIFunctions;
	} else
	if (!strncmp("SUNW,psycho",  prop_val, prop_len) ||
	    !strncmp("pci108e,8000", prop_val, prop_len)) {
	    /*
	     * A "Psycho" host bridge provides two PCI interfaces, each with
	     * its own 16-bit I/O and 31-bit memory spaces.  Both share the
	     * same PCI configuration space.  Here, they are assigned separate
	     * domain numbers to prevent unintentional I/O and/or memory
	     * resource conflicts.
	     */
	    xf86Msg(X_INFO,
		"Psycho PCI host bridge found (\"%s\")\n", prop_val);

	    /* Get "bus-range" property */
	    prop_val = promGetProperty("bus-range", &prop_len);
	    if (!prop_val || (prop_len != 8) ||
		(((unsigned int *)prop_val)[1] >= 256) ||
		(((unsigned int *)prop_val)[0] > ((unsigned int *)prop_val)[1]))
		continue;

	    domain.bus_min = ((int *)prop_val)[0];
	    domain.bus_max = ((int *)prop_val)[1];

	    /* Get "ranges" property */
	    prop_val = promGetProperty("ranges", &prop_len);
	    if (!prop_val || (prop_len != 112) ||
		prop_val[0] || (prop_val[28] != 0x01u) ||
		(prop_val[56] != 0x02u) || (prop_val[84] != 0x03u) ||
		(((unsigned int *)prop_val)[4] != 0x01000000u) ||
		((unsigned int *)prop_val)[5] ||
		((unsigned int *)prop_val)[12] ||
		(((unsigned int *)prop_val)[13] != 0x00010000u) ||
		((unsigned int *)prop_val)[19] ||
		(((unsigned int *)prop_val)[20] != 0x80000000u) ||
		((((unsigned int *)prop_val)[11] & ~0x00010000u) !=
		 0x02000000u) ||
		(((unsigned int *)prop_val)[18] & ~0x80000000u) ||
		(((unsigned int *)prop_val)[3] !=
		 ((unsigned int *)prop_val)[10]) ||
		(((unsigned int *)prop_val)[17] !=
		 ((unsigned int *)prop_val)[24]) ||
		(((unsigned int *)prop_val)[18] !=
		 ((unsigned int *)prop_val)[25]) ||
		(((unsigned int *)prop_val)[19] !=
		 ((unsigned int *)prop_val)[26]) ||
		(((unsigned int *)prop_val)[20] !=
		 ((unsigned int *)prop_val)[27]))
		continue;

	    /* Use memcpy() to avoid alignment issues */
	    (void)memcpy(&pci_addr, prop_val + 12,
			 sizeof(pci_addr));
	    (void)memcpy(&domain.io_addr, prop_val + 40,
			 sizeof(domain.io_addr));
	    (void)memcpy(&domain.mem_addr, prop_val + 68,
			 sizeof(domain.mem_addr));

	    domain.io_size  = 0x000000010000ull;
	    domain.mem_size = 0x000080000000ull;
	} else
	if (!strncmp("SUNW,schizo",  prop_val, prop_len) ||
	    !strncmp("pci108e,8001", prop_val, prop_len)) {
	    /*
	     * I have no docs on the "Schizo", but judging from the Linux
	     * kernel, it also provides two PCI domains.  Each PCI
	     * configuration space is the usual 16M in size, followed by a
	     * variable-length I/O space.  Each domain also provides a
	     * variable-length memory space.  The kernel seems to think the I/O
	     * spaces are 16M long, and the memory spaces, 2G, but these
	     * assumptions are actually only present in source code comments.
	     * Sun has, however, confirmed to me the validity of these
	     * assumptions.
	     */
	    volatile unsigned long long mem_match, mem_mask, io_match, io_mask;
	    unsigned long Offset;
	    pointer pSchizo;

	    xf86Msg(X_INFO,
		"Schizo PCI host bridge found (\"%s\")\n", prop_val);

	    /* Get "bus-range" property */
	    prop_val = promGetProperty("bus-range", &prop_len);
	    if (!prop_val || (prop_len != 8) ||
		(((unsigned int *)prop_val)[1] >= 256) ||
		(((unsigned int *)prop_val)[0] > ((unsigned int *)prop_val)[1]))
		continue;

	    domain.bus_min = ((int *)prop_val)[0];
	    domain.bus_max = ((int *)prop_val)[1];

	    /* Get "reg" property */
	    prop_val = promGetProperty("reg", &prop_len);
	    if (!prop_val || (prop_len != 48))
		continue;

	    /* Temporarily map some of Schizo's registers */
	    pSchizo = sparcMapAperture(-1, VIDMEM_MMIO,
		((unsigned long long *)prop_val)[2] - 0x000000010000ull,
		0x00010000ul);

	    /* Determine where PCI config, I/O and memory spaces reside */
	    if ((((unsigned long long *)prop_val)[0] & 0x000000700000ull) ==
		0x000000600000ull)
		Offset = 0x0040;
	    else
		Offset = 0x0060;

	    mem_match = PciReg(pSchizo, 0, Offset, unsigned long long);
	    mem_mask  = PciReg(pSchizo, 0, Offset + 8, unsigned long long);
	    io_match  = PciReg(pSchizo, 0, Offset + 16, unsigned long long);
	    io_mask   = PciReg(pSchizo, 0, Offset + 24, unsigned long long);

	    /* Unmap Schizo registers */
	    xf86UnMapVidMem(-1, pSchizo, 0x00010000ul);

	    /* Calculate sizes */
	    mem_mask = (((mem_mask - 1) ^ mem_mask) >> 1) + 1;
	    io_mask  = (((io_mask  - 1) ^ io_mask ) >> 1) + 1;

	    if (io_mask <= 0x000001000000ull)	/* Nothing left for I/O */
		continue;

	    domain.mem_addr = mem_match & ~0x8000000000000000ull;
	    domain.mem_size = mem_mask;
	    pci_addr        = io_match  & ~0x8000000000000000ull;
	    domain.io_addr  = pci_addr  +  0x0000000001000000ull;
	    domain.io_size  = io_mask   -  0x0000000001000000ull;
	} else {
	    xf86Msg(X_WARNING, "Unknown PCI host bridge: \"%s\"\n", prop_val);
	    continue;
	}

	/* Only map as much PCI configuration as we need */
	domain.pci = (char *)sparcMapAperture(-1, VIDMEM_MMIO,
	    pci_addr + PCI_MAKE_TAG(domain.bus_min, 0, 0),
	    PCI_MAKE_TAG(domain.bus_max - domain.bus_min + 1, 0, 0)) -
	    PCI_MAKE_TAG(domain.bus_min, 0, 0);

	/* Allocate a domain record */
	pDomain = xnfalloc(sizeof(sparcDomainRec));
	*pDomain = domain;

	/*
	 * Allocate and prime pciBusInfo records.  These are allocated one at a
	 * time because those for empty buses are eventually released.
	 */
	bus = pDomain->bus_min =
	    PCI_MAKE_BUS(pciNumDomains, domain.bus_min);
	pciNumBuses = pDomain->bus_max =
	    PCI_MAKE_BUS(pciNumDomains, domain.bus_max) + 1;

	pciBusInfo[bus] = xnfcalloc(1, sizeof(pciBusInfo_t));
	pciBusInfo[bus]->configMech = PCI_CFG_MECH_OTHER;
	pciBusInfo[bus]->numDevices = 32;
	pciBusInfo[bus]->funcs = *pFunctions;
	pciBusInfo[bus]->pciBusPriv = pDomain;
	while (++bus < pciNumBuses) {
	    pciBusInfo[bus] = xnfalloc(sizeof(pciBusInfo_t));
	    *(pciBusInfo[bus]) = *(pciBusInfo[bus - 1]);
	    pciBusInfo[bus]->funcs = sparcPCIFunctions;
	}

	/* Next domain, please... */
	xf86DomainInfo[pciNumDomains++] = pDomain;

	/*
	 * OK, enough of the straight-forward stuff.  Time to deal with some
	 * brokenness...
	 *
	 * The PCI specs require that when a bus transaction remains unclaimed
	 * for too long, the master entity on that bus is to cancel the
	 * transaction it issued or passed on with a master abort.  Two
	 * outcomes are possible:
	 *
	 * - the master abort can be treated as an error that is propogated
	 *   back through the bus tree to the entity that ultimately originated
	 *   the transaction; or
	 * - the transaction can be allowed to complete normally, which means
	 *   that writes are ignored and reads return all ones.
	 *
	 * In the first case, if the CPU happens to be at the tail end of the
	 * tree path through one of its host bridges, it will be told there is
	 * a hardware mal-function, despite being generated by software.
	 *
	 * For a software function (be it firmware, OS or userland application)
	 * to determine how a PCI bus tree is populated, it must be able to
	 * detect when master aborts occur.  Obviously, PCI discovery is much
	 * simpler when master aborts are allowed to complete normally.
	 *
	 * Unfortunately, a number of non-Intel PCI implementations have chosen
	 * to treat master aborts as severe errors.  The net effect is to
	 * cripple PCI discovery algorithms in userland.
	 *
	 * On SPARCs, master aborts cause a number of different behaviours,
	 * including delivering a signal to the userland application, rebooting
	 * the system, "dropping down" to firmware, or, worst of all, bus
	 * lockouts.  Even in the first case, the SIGBUS signal that is
	 * eventually generated isn't delivered in a timely enough fashion to
	 * allow an application to reliably detect the master abort that
	 * ultimately caused it.
	 *
	 * This can be somewhat mitigated.  On all architectures, master aborts
	 * that occur on secondary buses can be forced to complete normally
	 * because the PCI-to-PCI bridges that serve them are governed by an
	 * industry-wide specification.  (This is just another way of saying
	 * that whatever justification there might be for erroring out master
	 * aborts is deemed by the industry as insufficient to generate more
	 * PCI non-compliance than there already is...)
	 *
	 * This leaves us with master aborts that occur on primary buses.
	 * There is no specification for host-to-PCI bridges.  Bridges used in
	 * SPARCs can be told to ignore all PCI errors, but not specifically
	 * master aborts.  Not only is this too coarse-grained, but
	 * master-aborted read transactions on the primary bus end up returning
	 * garbage rather than all ones.
	 *
	 * I have elected to work around this the only way I can think of doing
	 * so right now.  The following scans an additional PROM level and
	 * builds a device/function map for the primary bus.  I can only hope
	 * this PROM information represents all devices on the primary bus,
	 * rather than only a subset of them.
	 *
	 * Master aborts are useful in other ways too, that are not addressed
	 * here.  These include determining whether or not a domain provides
	 * VGA, or if a PCI device actually implements PCI disablement.
	 *
	 * ---  TSI @ UQV  2001.09.19
	 */
	for (node2 = promGetChild(node);
	     node2;
	     node2 = promGetSibling(node2)) {
	    /* Get "reg" property */
	    prop_val = promGetProperty("reg", &prop_len);
	    if (!prop_val || (prop_len % 20))
		continue;

	    /*
	     * It's unnecessary to scan the entire "reg" property, but I'll do
	     * so anyway.
	     */
	    prop_len /= 20;
	    for (;  prop_len--;  prop_val += 20)
		SetBitInMap(PCI_DFN_FROM_TAG(*(PCITAG *)prop_val),
		    pDomain->dfn_mask);
	}

	/* Assume the host bridge is device 0, function 0 on its bus */
	SetBitInMap(0, pDomain->dfn_mask);
    }

    sparcPromClose();

    close(apertureFd);
    apertureFd = -1;
}

#ifndef INCLUDE_XF86_NO_DOMAIN

int
xf86GetPciDomain(PCITAG Tag)
{
    return PCI_DOM_FROM_TAG(Tag);
}

pointer
xf86MapDomainMemory(int ScreenNum, int Flags, PCITAG Tag,
		    ADDRESS Base, unsigned long Size)
{
    sparcDomainPtr pDomain;
    pointer        result;
    int            domain = PCI_DOM_FROM_TAG(Tag);

    if ((domain <= 0) || (domain >= pciNumDomains) ||
	!(pDomain = xf86DomainInfo[domain]) ||
	(((unsigned long long)Base + (unsigned long long)Size) >
	 pDomain->mem_size))
	FatalError("xf86MapDomainMemory() called with invalid parameters.\n");

    result = sparcMapAperture(ScreenNum, Flags, pDomain->mem_addr + Base, Size);

    if (apertureFd >= 0) {
	close(apertureFd);
	apertureFd = -1;
    }

    return result;
}

IOADDRESS
xf86MapDomainIO(int ScreenNum, int Flags, PCITAG Tag,
		IOADDRESS Base, unsigned long Size)
{
    sparcDomainPtr pDomain;
    int            domain = PCI_DOM_FROM_TAG(Tag);

    if ((domain <= 0) || (domain >= pciNumDomains) ||
	!(pDomain = xf86DomainInfo[domain]) ||
	(((unsigned long long)Base + (unsigned long long)Size) >
	 pDomain->io_size))
	FatalError("xf86MapDomainIO() called with invalid parameters.\n");

    /* Permanently map all of I/O space */
    if (!pDomain->io) {
	pDomain->io = sparcMapAperture(ScreenNum, Flags,
	    pDomain->io_addr, pDomain->io_size);

	if (apertureFd >= 0) {
	    close(apertureFd);
	    apertureFd = -1;
	}
    }

    return (IOADDRESS)pDomain->io + Base;
}

int
xf86ReadDomainMemory(PCITAG Tag, ADDRESS Base, int Len, unsigned char *Buf)
{
    unsigned char *ptr, *src;
    ADDRESS offset;
    unsigned long size;
    int len;

    /* Ensure page boundaries */
    offset = Base & ~pagemask;
    size = ((Base + Len + pagemask) & ~pagemask) - offset;

    ptr = xf86MapDomainMemory(-1, VIDMEM_READONLY, Tag, offset, size);

    /* Using memcpy() here hangs the system */
    src = ptr + (Base - offset);
    for (len = Len;  len-- > 0;)
	    *Buf++ = *src++;

    xf86UnMapVidMem(-1, ptr, size);

    return Len;
}

resPtr
xf86BusAccWindowsFromOS(void)
{
    sparcDomainPtr pDomain;
    resPtr         pRes = NULL;
    resRange       range;
    int            domain;

    for (domain = 1;  domain < pciNumDomains;  domain++) {
	if (!(pDomain = xf86DomainInfo[domain]))
	    continue;

	RANGE(range, 0, pDomain->mem_size - 1,
	      RANGE_TYPE(ResExcMemBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);

	RANGE(range, 0, pDomain->io_size - 1,
	      RANGE_TYPE(ResExcIoBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);
    }

    return pRes;
}

resPtr
xf86PciBusAccWindowsFromOS(void)
{
    sparcDomainPtr pDomain;
    resPtr         pRes = NULL;
    resRange       range;
    int            domain;

    for (domain = 1;  domain < pciNumDomains;  domain++) {
	if (!(pDomain = xf86DomainInfo[domain]))
	    continue;

	RANGE(range, 0, pDomain->mem_size - 1,
	      RANGE_TYPE(ResExcMemBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);

	RANGE(range, 0, pDomain->io_size - 1,
	      RANGE_TYPE(ResExcIoBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);
    }

    return pRes;
}

resPtr
xf86AccResFromOS(resPtr pRes)
{
    sparcDomainPtr pDomain;
    resRange       range;
    int            domain;

    for (domain = 1;  domain < pciNumDomains;  domain++) {
	if (!(pDomain = xf86DomainInfo[domain]))
	    continue;

	/*
	 * At minimum, the top and bottom resources must be claimed, so that
	 * resources that are (or appear to be) unallocated can be relocated.
	 */
	RANGE(range, 0x00000000u, 0x0009ffffu,
	      RANGE_TYPE(ResExcMemBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);
	RANGE(range, 0x000c0000u, 0x000effffu,
	      RANGE_TYPE(ResExcMemBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);
	RANGE(range, 0x000f0000u, 0x000fffffu,
	      RANGE_TYPE(ResExcMemBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);

	RANGE(range, pDomain->mem_size - 1, pDomain->mem_size - 1,
	      RANGE_TYPE(ResExcMemBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);

	RANGE(range, 0x00000000u, 0x00000000u,
	      RANGE_TYPE(ResExcIoBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);
	RANGE(range, pDomain->io_size - 1, pDomain->io_size - 1,
	      RANGE_TYPE(ResExcIoBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);
    }

    return pRes;
}

#endif /* !INCLUDE_XF86_NO_DOMAIN */

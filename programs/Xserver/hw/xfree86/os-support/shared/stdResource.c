/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/stdResource.c,v 1.3 1999/04/04 10:59:50 dawes Exp $ */

/* Standard resource information code */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Pci.h"
#define NEED_OS_RAC_PROTOS
#include "xf86_OSlib.h"

#ifdef USESTDRES
#define xf86StdMemWindowFromOS xf86MemWindowFromOS
#define xf86StdIoWindowFromOS xf86IoWindowFromOS
#define xf86StdMemResFromOS xf86MemResFromOS
#define xf86StdIoResFromOS xf86IoResFromOS
#define xf86StdInitOSPciAllocator xf86InitOSPciAllocator
#endif

void
xf86StdMemWindowFromOS(unsigned long *begin, unsigned long *end)
{
    /* Fallback is to allow addressing of all memory space */
    *begin = 0;
    *end = 0xffffffff;
}


void
xf86StdIoWindowFromOS(unsigned long *begin, unsigned long *end)
{
    /* Fallback is to allow addressing of all I/O space */
    *begin = 0;
    *end = 0xffff;
}


resPtr
xf86StdMemResFromOS()
{
    resPtr ret = NULL;

    /*
     * Fallback it to claim the following areas:
     *
     * 0x00000000 - 0x0009ffff	low 640k host memory
     * 0x000f0000 - 0x000fffff	system BIOS
     * 0x00100000 - 0x7fffffff	low 2G - 1MB host memory
     * 0xfec00000 - 0xfecfffff	default I/O APIC config space
     * 0xfee00000 - 0xfeefffff	default Local APIC config space
     * 0xffe00000 - 0xffffffff	high BIOS area (should this be included?)
     *
     * reference: Intel 440BX AGP specs
     */

    /* Fallback is to claim 0x0 - 0x9ffff and 0x100000 - 0x7fffffff */
    ret = xf86AddResToList(ret, 0, 0x9ffff, ResExcMemBlock, -1);
    ret = xf86AddResToList(ret, 0xf0000, 0xfffff, ResExcMemBlock, -1);
    ret = xf86AddResToList(ret, 0x100000, 0x7fffffff, ResExcMemBlock, -1);
    ret = xf86AddResToList(ret, 0xfec00000, 0xfecfffff, ResExcMemBlock, -1);
    ret = xf86AddResToList(ret, 0xfee00000, 0xfeefffff, ResExcMemBlock, -1);
    ret = xf86AddResToList(ret, 0xffe00000, 0xffffffff, ResExcMemBlock, -1);
    return ret;
}


resPtr
xf86StdIoResFromOS()
{
    resPtr ret = NULL;

    /* Fallback is to claim well known ports in the 0x0 - 0x3ff range */
    /* Possibly should claim some of them as sparse ranges */

    ret = xf86AddResToList(ret, 0, 0x1ff, ResExcIoBlock, -1);
    /* XXX add others */
    return ret;
}

typedef struct {
    int brbus, brdev, brfunc;	/* ID of the bridge to this bus */
    int subclass;	/* bridge type */
    resPtr io;		/* I/O range */
    resPtr mem;		/* non-prefetchable memory range */
    resPtr pmem;	/* prefetchable memory range */
    int brcontrol;	/* bridge_control byte */
} PciBusRec, *PciBusPtr;

static int NumBusses = 0;
static PciBusPtr *PciBusList = NULL;
static resPtr *pSysMem = NULL;
static resPtr *pSysIo = NULL;
static resPtr PciMem = NULL;
static resPtr PciIo = NULL;

void
xf86StdInitOSPciAllocator(const pciConfigPtr *pciInfo, resPtr *sysMem,
			  resPtr *sysIo, const resPtr pciMem,
			  const resPtr pciIo)
{
    const pciConfigPtr *pcrpp;
    pciConfigPtr pcrp;
    int i;
    unsigned long begin, end;

    /* Initialise the pointers to the system exclusive lists */
    pSysMem = sysMem;
    pSysIo = sysIo;

    if (!pciInfo)
	return;

    /* Make a local copy of the current non-system PCI allocations */
    PciMem = xf86DupResList(pciMem);
    PciIo = xf86DupResList(pciIo);

    /* Get the current address ranges for each additional PCI bus */
    /* Bus zero is a little special */
    NumBusses = 1;
    PciBusList = xnfalloc(sizeof(PciBusPtr));
    PciBusList[0] = xnfcalloc(1, sizeof(PciBusRec));
    PciBusList[0]->subclass = PCI_SUBCLASS_BRIDGE_HOST;
    /* Add each PCI-PCI bridge */
    /* XXX What about secondary host bridges?? */
    for (pcrpp = pciInfo, pcrp = *pcrpp; pcrp; pcrp = *(++pcrpp)) {
	if (pcrp->pci_base_class == PCI_CLASS_BRIDGE &&
	    pcrp->pci_sub_class == PCI_SUBCLASS_BRIDGE_PCI) {
	    i = pcrp->pci_secondary_bus_number;
	    if (i + 1 > NumBusses) {
		NumBusses = i + 1;
		PciBusList = xnfrealloc(PciBusList,
					NumBusses * sizeof(PciBusPtr));
	    }
	    PciBusList[i] = xnfcalloc(1, sizeof(PciBusRec));
	    PciBusList[i]->brbus = pcrp->busnum;
	    PciBusList[i]->brdev = pcrp->devnum;
	    PciBusList[i]->brfunc = pcrp->funcnum;
	    PciBusList[i]->subclass = pcrp->pci_sub_class;
	    PciBusList[i]->brcontrol = pcrp->pci_bridge_control;
	    begin = pcrp->pci_io_base << 8;
	    end = (pcrp->pci_io_limit << 8) | 0xfff;
	    PciBusList[i]->io = xf86AddResToList(NULL, begin, end,
					ResIo | ResBlock | ResMinimised, -1);
	    begin = pcrp->pci_mem_base << 16;
	    end = (pcrp->pci_mem_limit << 16) | 0xfffff;
	    PciBusList[i]->mem = xf86AddResToList(NULL, begin, end,
					ResMem | ResBlock | ResMinimised, -1);
	    begin = pcrp->pci_prefetch_mem_base << 16;
	    end = (pcrp->pci_prefetch_mem_limit << 16) | 0xfffff;
	    PciBusList[i]->pmem = xf86AddResToList(NULL, begin, end,
					ResMem | ResBlock | ResMinimised, -1);
	    xf86MsgVerb(X_INFO, 3, "Bus %d: bridge is at (%d:%d:%d), "
			"BCTRL: 0x%02x (VGA_EN is %s)\n",
			i, PciBusList[i]->brbus, PciBusList[i]->brdev,
			PciBusList[i]->brfunc, PciBusList[i]->brcontrol,
			(PciBusList[i]->brcontrol & 0x08) ? "set" : "cleared");
	    xf86MsgVerb(X_INFO, 3, "Bus %d I/O range:\n", i);
	    xf86PrintResList(3, PciBusList[i]->io);
	    xf86MsgVerb(X_INFO, 3,
			"Bus %d non-prefetchable memory range:\n", i);
	    xf86PrintResList(3, PciBusList[i]->mem);
	    xf86MsgVerb(X_INFO, 3, "Bus %d prefetchable memory range:\n", i);
	    xf86PrintResList(3, PciBusList[i]->pmem);
	}
    }
}

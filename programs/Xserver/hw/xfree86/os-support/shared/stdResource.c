/* $XFree86: /X11R6/x-cvs/xc/programs/Xserver/hw/xfree86/os-support/shared/stdResource.c,v 1.10 1999/09/25 14:38:04 dawes Exp $ */

/* Standard resource information code */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Privstr.h"
#include "xf86Pci.h"
#define NEED_OS_RAC_PROTOS
#include "xf86_OSlib.h"

#ifdef USESTDRES
#define xf86AccWindowsFromOS xf86StdAccWindowsFromOS
#define xf86AccResFromOS xf86StdAccResFromOS
#define xf86InitOSPciAllocator xf86StdInitOSPciAllocator
#endif

resPtr
xf86StdAccWindowsFromOS(void)
{
    /* Fallback is to allow addressing of all memory space */
    resPtr ret = NULL;
    resRange range;

    RANGE(range,0,0xffffffff,ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    /* Fallback is to allow addressing of all I/O space */
    RANGE(range,0,0xffff,ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

resPtr
xf86StdAccResFromOS(resPtr ret)
{
    resRange range;

    /*
     * Fallback is to claim the following areas:
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
    RANGE(range,0,0x9ffff,ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range,0xf0000,0xfffff,ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range,0x100000,0x7fffffff,ResExcMemBlock | ResBios | ResEstimated);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range,0xfec00000,0xfecfffff,ResExcMemBlock | ResBios);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range,0xfee00000,0xfeefffff,ResExcMemBlock | ResBios);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range,0xffe00000,0xffffffff,ResExcMemBlock | ResBios);
    ret = xf86AddResToList(ret, &range, -1);

    /* Fallback is to claim well known ports in the 0x0 - 0x3ff range */
    /* Possibly should claim some of them as sparse ranges */

    RANGE(range,0,0x1ff,ResExcIoBlock | ResEstimated);
    ret = xf86AddResToList(ret, &range, -1);
    /* XXX add others */
    return ret;
}

static resPtr *pSysRes = NULL;
static resPtr PciRes = NULL;

static PciBusPtr
xf86FindPciBridgeInfo(const pciConfigPtr *pciInfo)
{
    const pciConfigPtr *pcrpp;
    pciConfigPtr pcrp;
    resRange range;
    PciBusPtr PciBus, PciBusBase;
    
    /* Get the current address ranges for each additional PCI bus */
    /* Bus zero is a little special */
    PciBus = PciBusBase = xnfcalloc(1, sizeof(PciBusRec));
    PciBus->subclass = PCI_SUBCLASS_BRIDGE_HOST;
    PciBus->primary = -1;
    PciBus->secondary = 0;
    /* for the primary host bridge assume: io: 0-0xffff mem: 0-0xffffffff */
    /* prefetchable range is unknown therefore we don't set it            */
    RANGE(range, 0, 0xFFFF, ResIo | ResBlock | ResExclusive);
    PciBus->io = xf86AddResToList(NULL, &range, -1);
    RANGE(range, 0, ~(memType)0, ResMem | ResBlock | ResExclusive);
    PciBus->mem = xf86AddResToList(NULL, &range, -1);
    /* Add each PCI-PCI bridge */
    /* XXX What about secondary host bridges?? */
    for (pcrpp = pciInfo, pcrp = *pcrpp; pcrp; pcrp = *(++pcrpp)) {
	if (pcrp->pci_base_class == PCI_CLASS_BRIDGE &&
	    pcrp->pci_sub_class == PCI_SUBCLASS_BRIDGE_PCI) {
	    PciBus->next = xnfcalloc(1, sizeof(PciBusRec));
	    PciBus = PciBus->next;
	    PciBus->secondary = pcrp->pci_secondary_bus_number;
	    PciBus->primary = pcrp->pci_primary_bus_number;
	    PciBus->subordinate = pcrp->pci_subordinate_bus_number;
	    PciBus->brbus = pcrp->busnum;
	    PciBus->brdev = pcrp->devnum;
	    PciBus->brfunc = pcrp->funcnum;
	    PciBus->subclass = pcrp->pci_sub_class;
	    PciBus->brcontrol = pcrp->pci_bridge_control;
	    if (pcrp->pci_io_base <= pcrp->pci_io_limit) {
		RANGE(range,pcrp->pci_io_base << 8,
		      (pcrp->pci_io_limit << 8) | 0xfff,
		      ResIo | ResBlock | ResExclusive);
		PciBus->io = xf86AddResToList(NULL, &range, -1);
	    }
	    if (pcrp->pci_mem_base <= pcrp->pci_mem_limit) {
		RANGE(range,pcrp->pci_mem_base << 16,
		      (pcrp->pci_mem_limit << 16) | 0xfffff,
		      ResMem | ResBlock | ResExclusive);
		PciBus->mem = xf86AddResToList(NULL, &range, -1);
	    }
	    if (pcrp->pci_prefetch_mem_base <= pcrp->pci_prefetch_mem_limit) {
		RANGE(range,pcrp->pci_prefetch_mem_base << 16,
		      (pcrp->pci_prefetch_mem_limit << 16) | 0xfffff,
		      ResMem | ResBlock | ResExclusive);
		PciBus->pmem = xf86AddResToList(NULL, &range, -1);
	    }
	    xf86MsgVerb(X_INFO, 3, "Bus %d: bridge is at (%d:%d:%d), "
			"(%d,%d,%d), BCTRL: 0x%02x (VGA_EN is %s)\n",
			PciBus->secondary, PciBus->brbus, PciBus->brdev,
			PciBus->brfunc, PciBus->primary,
			PciBus->secondary, PciBus->subordinate,
			PciBus->brcontrol,
			(PciBus->brcontrol & PCI_PCI_BRIDGE_VGA_EN)
			? "set" : "cleared");
	    xf86MsgVerb(X_INFO, 3, "Bus %d I/O range:\n", PciBus->secondary);
	    xf86PrintResList(3, PciBus->io);
	    xf86MsgVerb(X_INFO, 3,
			"Bus %d non-prefetchable memory range:\n",
			PciBus->secondary);
	    xf86PrintResList(3, PciBus->mem);
	    xf86MsgVerb(X_INFO, 3, "Bus %d prefetchable memory range:\n",
			PciBus->secondary);
	    xf86PrintResList(3, PciBus->pmem);
	}
	if (pcrp->pci_base_class == PCI_CLASS_BRIDGE &&
	    pcrp->pci_sub_class == PCI_SUBCLASS_BRIDGE_ISA) {
	    PciBus->next = xnfcalloc(1, sizeof(PciBusRec));
	    PciBus = PciBus->next;
	    PciBus->primary = 0;
	    PciBus->secondary = -1;
	    PciBus->brbus = pcrp->busnum;
	    PciBus->brdev = pcrp->devnum;
	    PciBus->brfunc = pcrp->funcnum;
	    PciBus->subclass = pcrp->pci_sub_class;
	}
    }
    return PciBusBase;
    
}

PciBusPtr
xf86StdInitOSPciAllocator(const pciConfigPtr *pciInfo, resPtr *sysRes,
			  const resPtr pciRes)
{
    /* Initialise the pointers to the system exclusive lists */
    pSysRes = sysRes;

    if (!pciInfo)
	return NULL;

    /* Make a local copy of the current non-system PCI allocations */
    PciRes = xf86DupResList(pciRes);

    /* resources assigned by bios should be avoided */
    *sysRes = xf86JoinResLists(pciRes, *sysRes);

    return xf86FindPciBridgeInfo(pciInfo);
}

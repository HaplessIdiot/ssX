/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Bus.c,v 1.38 1999/09/25 14:37:09 dawes Exp $ */
#define DEBUG
/*
 * Copyright (c) 1997-1999 by The XFree86 Project, Inc.
 */

/*
 * This file contains the interfaces to the bus-specific code
 */

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "X.h"
#include "os.h"
#include "xf86Pci.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Resources.h"

/* Bus-specific headers */

#include "xf86Bus.h"
#define DECLARE_CARD_DATASTRUCTURES TRUE
#include "xf86PciInfo.h"

#define XF86_OS_PRIVS
#define NEED_OS_RAC_PROTOS
#include "xf86_OSproc.h"

#include "xf86RAC.h"

/* Bus-specific globals */
pciConfigPtr *xf86PciInfo = NULL;		/* Full PCI probe info */
pciVideoPtr *xf86PciVideoInfo = NULL;		/* PCI probe for video hw */
pciAccPtr * xf86PciAccInfo = NULL;              /* PCI access related */
BusAccPtr xf86PciBusAccInfo = NULL;

/* PCI buses */
static PciBusPtr xf86PciBus = NULL;

/* Entity data */
static EntityPtr *xf86Entities = NULL;	/* Bus slots claimed by drivers */
static int xf86NumEntities = 0;

static xf86AccessRec AccessNULL = {NULL,NULL,NULL};

xf86CurrentAccessRec xf86CurrentAccess = {NULL,NULL};

static BusRec primaryBus = { BUS_NONE, {{0}}};

Bool xf86ResAccessEnter = FALSE;

/* new RAC */

/* resource lists */
static resPtr Acc =  NULL;

/* Resources that temporarily conflict with estimated resources */
static resPtr AccReducers = NULL;

/* allocatable ranges */
static resPtr ResRange = NULL;

/* predefined special resources */
resRange resVgaExclusive[] = {_VGA_EXCLUSIVE, _END};
resRange resVgaShared[] = {_VGA_SHARED, _END};
resRange resVgaUnusedExclusive[] = {_VGA_EXCLUSIVE_UNUSED, _END};
resRange resVgaUnusedShared[] = {_VGA_SHARED_UNUSED, _END};
resRange resVgaSparseExclusive[] = {_VGA_EXCLUSIVE_SPARSE, _END};
resRange resVgaSparseShared[] = {_VGA_SHARED_SPARSE, _END};
resRange res8514Exclusive[] = {_8514_EXCLUSIVE, _END};
resRange res8514Shared[] = {_8514_SHARED, _END};
resRange PciAvoid[] = {_PCI_AVOID, _END};

/* Flag: do we need RAC ? */
static Bool needRAC = FALSE;

/* Bus-specific probe/sorting functions */

/* PCI classes that get included in xf86PciVideoInfo */
#define PCIINFOCLASSES(b,s)						      \
    (((b) == PCI_CLASS_PREHISTORIC) ||					      \
     ((b) == PCI_CLASS_DISPLAY) ||					      \
     ((b) == PCI_CLASS_MULTIMEDIA && (s) == PCI_SUBCLASS_MULTIMEDIA_VIDEO) || \
     ((b) == PCI_CLASS_PROCESSOR && (s) == PCI_SUBCLASS_PROCESSOR_COPROC))

/*
 * PCI classes that have messages printed always.  The others are only
 * have a message printed when the vendor/dev IDs are recognised.
 */
#define PCIALWAYSPRINTCLASSES(b,s)					      \
    (((b) == PCI_CLASS_PREHISTORIC && (s) == PCI_SUBCLASS_PREHISTORIC_VGA) || \
     ((b) == PCI_CLASS_DISPLAY) ||					      \
     ((b) == PCI_CLASS_MULTIMEDIA && (s) == PCI_SUBCLASS_MULTIMEDIA_VIDEO))

/*
 * PCI classes for which potentially destructive checking of the map sizes
 * may be done.  Any classes where this may be unsafe should be omitted
 * from this list.
 */
#define PCINONSYSTEMCLASSES(b,s) PCIALWAYSPRINTCLASSES(b,s)

/* 
 * PCI classes that use RAC 
 */
#define PCISHAREDIOCLASSES(b,s)					      \
    (((b) == PCI_CLASS_PREHISTORIC && (s) == PCI_SUBCLASS_PREHISTORIC_VGA) || \
     ((b) == PCI_CLASS_DISPLAY && (s) == PCI_SUBCLASS_DISPLAY_VGA))

/* XXX Fixme: Hack: so far we only support 32 bit PCI addresses */
#define PCI_MEMBASE_LENGTH_TYPE(x) 0xFFFFFFFF
#define PCI_MEMBASE_LENGTH_MAX 0xFFFFFFFF
#define PCI_IOBASE_LENGTH_MAX 0xFFFF
#undef MIN
#define MIN(x,y) ((x<y)?x:y)

SymTabPtr xf86PCIVendorNameInfo;
pciVendorCardInfo *xf86PCICardInfo;
pciVendorDeviceInfo * xf86PCIVendorInfo;

static void
xf86FindPCIVideoInfo(void)
{
    pciConfigPtr pcrp, *pcrpp;
    int i = 0, j, k;
    int num = 0;
    pciVideoPtr info;
    Bool mem64 = FALSE;

    pcrpp = xf86PciInfo = xf86scanpci(0);
   
    if (pcrpp == NULL) {
	xf86PciVideoInfo = NULL;
	return;
    }
    
    while ((pcrp = pcrpp[i])) {
	if (PCIINFOCLASSES(pcrp->pci_base_class, pcrp->pci_sub_class)) {
	    num++;
	    xf86PciVideoInfo = xnfrealloc(xf86PciVideoInfo,
					  sizeof(pciVideoPtr) * (num + 1));
	    xf86PciVideoInfo[num] = NULL;
	    info = xf86PciVideoInfo[num - 1] = xnfalloc(sizeof(pciVideoRec));
	    info->validate = FALSE;
	    info->vendor = pcrp->pci_vendor;
	    info->chipType = pcrp->pci_device;
	    info->chipRev = pcrp->pci_rev_id;
	    info->subsysVendor = pcrp->pci_subsys_vendor;
	    info->subsysCard = pcrp->pci_subsys_card;
	    info->bus = pcrp->busnum;
	    info->device = pcrp->devnum;
	    info->func = pcrp->funcnum;
	    info->class = pcrp->pci_base_class;
	    info->subclass = pcrp->pci_sub_class;
	    info->interface = pcrp->pci_prog_if;
	    info->biosBase = pcrp->pci_baserom;
	    info->biosSize = pciGetBaseSize(pcrp->tag, 6, TRUE, NULL);
	    info->thisCard = pcrp;
	    if ((PCISHAREDIOCLASSES(pcrp->pci_base_class, pcrp->pci_sub_class))
		&& (pcrp->pci_command & PCI_CMD_IO_ENABLE) &&
		(pcrp->pci_prog_if == 0)) {
		/* assumption: primary bus is always VGA */
	            primaryBus.type = BUS_PCI;
	            primaryBus.id.pci.bus = pcrp->busnum;
		    primaryBus.id.pci.device = pcrp->devnum;
		    primaryBus.id.pci.func = pcrp->funcnum;
	    }
	    
	    for (j = 0; j < 6; j++) {
		info->memBase[j] = 0;
		info->ioBase[j] = 0;
		if (PCINONSYSTEMCLASSES(info->class, info->subclass)) {
		    info->size[j]  = pciGetBaseSize(pcrp->tag, j, TRUE, NULL);
		    info->validate = TRUE;
		}
		else
		  info->size[j] = pcrp->basesize[j];
                  /* pciGetBaseSize(pcrp->tag, j, FALSE, NULL) */
		info->type[j] = 0;
	    }

	    /*
	     * 64-bit base addresses are checked for and avoided.
	     * XXX Should deal with them on platforms that support them.
	     */

	    if (pcrp->pci_base0) {
		if (pcrp->pci_base0 & PCI_MAP_IO) {
		    info->ioBase[0] = (memType)PCIGETIO(pcrp->pci_base0);
		    info->type[0] = pcrp->pci_base0 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base0))
			mem64 = TRUE;
		    else {
			info->memBase[0] = (memType)PCIGETMEMORY(pcrp->pci_base0);
			info->type[0] = pcrp->pci_base0 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    }

	    if (pcrp->pci_base1 && !mem64) {
		if (pcrp->pci_base1 & PCI_MAP_IO) {
		    info->ioBase[1] = (memType)PCIGETIO(pcrp->pci_base1);
		    info->type[1] = pcrp->pci_base1 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base1))
			mem64 = TRUE;
		    else {
			info->memBase[1] = (memType)PCIGETMEMORY(pcrp->pci_base1);
			info->type[1] = pcrp->pci_base1 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->pci_base2 && !mem64) {
		if (pcrp->pci_base2 & PCI_MAP_IO) {
		    info->ioBase[2] = (memType)PCIGETIO(pcrp->pci_base2);
		    info->type[2] = pcrp->pci_base2 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base2))
			mem64 = TRUE;
		    else {
			info->memBase[2] = (memType)PCIGETMEMORY(pcrp->pci_base2);
			info->type[2] = pcrp->pci_base2 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->pci_base3 && !mem64) {
		if (pcrp->pci_base3 & PCI_MAP_IO) {
		    info->ioBase[3] = (memType)PCIGETIO(pcrp->pci_base3);
		    info->type[3] = pcrp->pci_base3 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base3))
			mem64 = TRUE;
		    else {
			info->memBase[3] = (memType)PCIGETMEMORY(pcrp->pci_base3);
			info->type[3] = pcrp->pci_base3 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->pci_base4 && !mem64) {
		if (pcrp->pci_base4 & PCI_MAP_IO) {
		    info->ioBase[4] = (memType)PCIGETIO(pcrp->pci_base4);
		    info->type[4] = pcrp->pci_base4 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base4))
			mem64 = TRUE;
		    else {
			info->memBase[4] = (memType)PCIGETMEMORY(pcrp->pci_base4);
			info->type[4] = pcrp->pci_base4 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->pci_base5 && !mem64) {
		if (pcrp->pci_base5 & PCI_MAP_IO) {
		    info->ioBase[5] = (memType)PCIGETIO(pcrp->pci_base5);
		    info->type[5] = pcrp->pci_base5 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base5))
			mem64 = TRUE;
		    else {
			info->memBase[5] = (memType)PCIGETMEMORY(pcrp->pci_base5);
			info->type[5] = pcrp->pci_base5 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    }
	}
	i++;
    }

    /* Print a summary of the video devices found */
    {
	for (k = 0; k < num; k++) {
	    char *vendorname = NULL, *chipname = NULL;
	    Bool memdone = FALSE, iodone = FALSE;

	    i = 0; 
	    info = xf86PciVideoInfo[k];
	    while (xf86PCIVendorNameInfo[i].token) {
		if (xf86PCIVendorNameInfo[i].token == info->vendor) 
		    vendorname = (char *)xf86PCIVendorNameInfo[i].name;
		i++;
	    }
	    i = 0;
	    while(xf86PCIVendorInfo[i].VendorID) {
		if (xf86PCIVendorInfo[i].VendorID == info->vendor) {
		    j = 0;
		    while (xf86PCIVendorInfo[i].Device[j].DeviceName) {
			if (xf86PCIVendorInfo[i].Device[j].DeviceID ==
			    info->chipType) {
			    chipname =
				xf86PCIVendorInfo[i].Device[j].DeviceName;
			    break;
			}
			j++;
		    }
		    break;
		}
		i++;
	    }
	    if ((!vendorname || !chipname) &&
		!PCIALWAYSPRINTCLASSES(info->class, info->subclass))
		continue;
	    xf86Msg(X_PROBED, "PCI: (%d:%d:%d) ", info->bus, info->device,
		    info->func);
	    if (vendorname)
		ErrorF("%s ", vendorname);
	    else
		ErrorF("unknown vendor (0x%04x) ", info->vendor);
	    if (chipname)
		ErrorF("%s ", chipname);
	    else
		ErrorF("unknown chipset (0x%04x) ", info->chipType);
	    ErrorF("rev %d", info->chipRev);
	    for (i = 0; i < 6; i++) {
		if (info->memBase[i]) {
		    if (!memdone) {
			ErrorF(", Mem @ ");
			memdone = TRUE;
		    } else
			ErrorF(", ");
		    ErrorF("0x%08x/%d", info->memBase[i], info->size[i]);
		}
	    }
	    for (i = 0; i < 6; i++) {
		if (info->ioBase[i]) {
		    if (!iodone) {
			ErrorF(", I/O @ ");
			iodone = TRUE;
		    } else
			ErrorF(", ");
		    ErrorF("0x%04x/%d", info->ioBase[i], info->size[i]);
		}
	    }
	    ErrorF("\n");
	}
    }
}

/*
 * Call the bus probes relevant to the architecture.
 *
 * The only one available so far is for PCI
 */

void
xf86BusProbe(void)
{

#ifdef XFree86LOADER
    /* 
     * we need to get the pointer to the pci data structures initialized
     */
    xf86PCIVendorNameInfo =
      (SymTabPtr)LoaderSymbol("xf86PCIVendorNameInfoData");
    xf86PCIVendorInfo =
      (pciVendorDeviceInfo*)LoaderSymbol("xf86PCIVendorInfoData");
    xf86PCICardInfo =
      (pciVendorCardInfo*)LoaderSymbol("xf86PCICardInfoData");
#else
    xf86PCIVendorNameInfo = xf86PCIVendorNameInfoData; 
    xf86PCIVendorInfo = xf86PCIVendorInfoData; 
    xf86PCICardInfo = xf86PCICardInfoData;
#endif
    xf86FindPCIVideoInfo();
}

/*
 * Functions to access the bus list.  The data structures are opaque to
 * the video drivers.
 */

static int
xf86AllocateEntity(void)
{
    xf86NumEntities++;
    xf86Entities = xnfrealloc(xf86Entities, sizeof(EntityPtr) * xf86NumEntities);
    xf86Entities[xf86NumEntities - 1] = xnfcalloc(1,sizeof(EntityRec));
    return (xf86NumEntities - 1);
}

/*
 * Check if the slot requested is free.  If it is already in use, return FALSE.
 */

Bool
xf86CheckPciSlot(int bus, int device, int func)
{
    int i;
    EntityPtr p;

    for (i = 0; i < xf86NumEntities; i++) {
	p = xf86Entities[i];
	/* Check if this PCI slot is taken */
	if (p->busType == BUS_PCI && p->pciBusId.bus == bus &&
	    p->pciBusId.device == device && p->pciBusId.func == func)
	    return FALSE;
    }
    
    return TRUE;
}

/*
 * fixPciSizeInfo() -- fix pci size info by testing it destructively
 * (if not already done), fix pciVideoInfo and entry in the resource
 * list.
 */
/*
 * Note: once we have OS support to read the sizes GetBaseSize() will
 * have to be wrapped by the OS layer. fixPciSizeInfo() should also
 * be wrapped by the OS layer to do nothing if the size is always
 * returned correctly by GetBaseSize(). It should however set validate
 * in pciVideoRec if validation is required. PciValidate() also needs
 * to be wrapped by the OS layer. This may do nothing if the OS has
 * already taken care of validation. fixPciResource() may be moved to
 * OS layer with minimal changes. Once the wrapping layer is in place
 * the common level and drivers should not reference these functions
 * directly but thru the OS layer.
 */

static void
fixPciSizeInfo(int entityIndex)
{
    pciVideoPtr pvp;
    resPtr pAcc;
    PCITAG tag;
    int j;
    
    if (! (pvp = xf86GetPciInfoForEntity(entityIndex))) return;
    if (pvp->validate) return;

    tag = pciTag(pvp->bus,pvp->device,pvp->func);
    
    for (j = 0; j < 6; j++) {
	pAcc = Acc;
	if (pvp->memBase[j]) 
	    while (pAcc) {
		if (((pAcc->res_type & (ResMem | ResBlock))
		     == (ResMem | ResBlock))
		    && (pAcc->block_begin == pvp->memBase[j])
		    && (pAcc->block_end == pvp->memBase[j]
		    + (1 << (pvp->size[j])) - 1)) break;
		pAcc = pAcc->next;
	    } else if (pvp->ioBase[j])
	    while (pAcc) {
		if (((pAcc->res_type & (ResIo | ResBlock)) ==
		     (ResIo | ResBlock))
		    && (pAcc->block_begin == pvp->ioBase[j])
		    && (pAcc->block_end == pvp->ioBase[j]
		    + (1 << (pvp->size[j])) - 1)) break;
		pAcc = pAcc->next;
	    } else continue;
	pvp->size[j]  = pciGetBaseSize(tag, j, TRUE, NULL);
	if (pAcc)
	    pAcc->block_end = pAcc->block_begin + (1 << (pvp->size[j])) - 1;
    }
    if (pvp->biosBase) {
	pAcc = Acc;
	while (pAcc) {
	    if (((pAcc->res_type & (ResMem | ResBlock)) == (ResMem | ResBlock))
		&& (pAcc->block_begin == pvp->biosBase)
		    && (pAcc->block_end == pvp->biosBase
		    + (1 << (pvp->biosSize)) - 1)) break;
	    pAcc = pAcc->next;
	}
	pvp->biosSize = pciGetBaseSize(tag, 6, TRUE, NULL);
	if (pAcc)
	    pAcc->block_end = pAcc->block_begin + (1 << (pvp->biosSize))- 1;
    }
    
    pvp->validate = TRUE;
}

/*
 * If the slot requested is already in use, return -1.
 * Otherwise, claim the slot for the screen requesting it.
 */
static void disablePciBios(PCITAG tag);

int
xf86ClaimPciSlot(int bus, int device, int func, DriverPtr drvp,
		 int chipset, GDevPtr dev, Bool active)
{
    EntityPtr p;
    pciAccPtr *ppaccp = xf86PciAccInfo;
    BusAccPtr pbap = xf86PciBusAccInfo;
    
    int num;
    
    if (xf86CheckPciSlot(bus, device, func)) {
	num = xf86AllocateEntity();
	p = xf86Entities[num];
	p->driver = drvp;
	p->chipset = chipset;
	p->busType = BUS_PCI;
 	p->device = dev;
	p->pciBusId.bus = bus;
	p->pciBusId.device = device;
	p->pciBusId.func = func;
	p->active = active;
	p->inUse = FALSE;
	/* Here we initialize the access structure */
	p->access = xnfcalloc(1,sizeof(EntityAccessRec));
	while (ppaccp && *ppaccp) {
	    if ((*ppaccp)->busnum == bus
		&& (*ppaccp)->devnum == device
		&& (*ppaccp)->funcnum == func) {
		p->access->fallback = &(*ppaccp)->io_memAccess;
		p->access->pAccess = &(*ppaccp)->io_memAccess;
 		(*ppaccp)->ctrl = TRUE; /* mark control if not already */
		break;
	    }
	    ppaccp++;
	}
	if (!ppaccp || !*ppaccp) {
	    p->access->fallback = &AccessNULL;
	    p->access->pAccess = &AccessNULL;
	}
	
	p->busAcc = NULL;
	while (pbap) {
	    if (pbap->type == BUS_PCI && pbap->busdep.pci.bus == bus)
		p->busAcc = pbap;
	    pbap = pbap->next;
	}
	fixPciSizeInfo(num);

	/* in case bios is enabled disable it */
	disablePciBios(pciTag(bus,device,func));
	
 	return num;
    } else
 	return -1;
}

/*
 * Get xf86PciVideoInfo for a driver.
 */
pciVideoPtr *
xf86GetPciVideoInfo(void)
{
    return xf86PciVideoInfo;
}

/* --- Used by ATI driver, but also more generally useful */

/*
 * Get the full xf86scanpci data.
 */
pciConfigPtr *
xf86GetPciConfigInfo(void)
{
    return xf86PciInfo;
}

/*
 * Enable a device and route VGA to it.  This is intended for a driver's
 * Probe(), before creating EntityRec's.  Only one device can be thus enabled
 * at any one time, and should be disabled when the driver is done with it.
 *
 * The following special calls are also available:
 *
 * pvp == NULL && rt == NONE    disable previously enabled device
 * pvp != NULL && rt == NONE    ensure device is disabled
 * pvp == NULL && rt != NONE    disable >all< subsequent calls to this function
 *                              (done from xf86PostProbe())
 *
 * The device represented by pvp may not have been previously claimed.
 */
void
xf86SetPciVideo(pciVideoPtr pvp, resType rt)
{
    static BusAccPtr pbap = NULL;
    static xf86AccessPtr pAcc = NULL;
    static Bool DoneProbes = FALSE;
    pciAccPtr pcaccp;
    int i;

    if (DoneProbes)
	return;

    /* Disable previous access */
    if (pAcc) {
	if (pAcc->AccessDisable)
	    (*pAcc->AccessDisable)(pAcc->arg);
	pAcc = NULL;
    }
    if (pbap) {
	while (pbap->primary) {
	    if (pbap->disable_f)
		(*pbap->disable_f)(pbap);
	    pbap->primary->current = NULL;
	    pbap = pbap->primary;
	}
	pbap = NULL;
    }

    /* Check for xf86PostProbe's magic combo */
    if (!pvp) {
	if (rt != NONE)
	    DoneProbes = TRUE;
	return;
    }

    /* Validate device */
    if (!xf86PciVideoInfo || !xf86PciAccInfo || !xf86PciBusAccInfo)
	return;

    for (i = 0; pvp != xf86PciVideoInfo[i]; i++)
	if (!xf86PciVideoInfo[i])
	    return;

    /* Ignore request for claimed adapters */
    if (!xf86CheckPciSlot(pvp->bus, pvp->device, pvp->func))
	return;

    /* Find pciAccRec structure */
    for (i = 0; ; i++) {
	if (!(pcaccp = xf86PciAccInfo[i]))
	    return;
	if ((pvp->bus == pcaccp->busnum) &&
	    (pvp->device == pcaccp->devnum) &&
	    (pvp->func == pcaccp->funcnum))
	    break;
    }

    if (rt == NONE) {
	/* This is a call to ensure the adapter is disabled */
	if (pcaccp->io_memAccess.AccessDisable)
	    (*pcaccp->io_memAccess.AccessDisable)(pcaccp->io_memAccess.arg);
	return;
    }

    /* Find BusAccRec structure */
    for (pbap = xf86PciBusAccInfo; ; pbap = pbap->next) {
	if (!pbap)
	    return;
	if (pvp->bus == pbap->busdep.pci.bus)
	    break;
    }

    /* Route VGA */
    if (pbap->set_f)
	(*pbap->set_f)(pbap);

    /* Enable device */
    switch (rt) {
    case IO:
	pAcc = &pcaccp->ioAccess;
	break;
    case MEM_IO:
	pAcc = &pcaccp->io_memAccess;
	break;
    case MEM:
	pAcc = &pcaccp->memAccess;
	break;
    default:	/* no compiler noise */
	break;
    }

    if (pAcc && pAcc->AccessEnable)
	(*pAcc->AccessEnable)(pAcc->arg);
}

/* --- */

/*
 * Determine what bus type the busID string represents.  The start of the
 * bus-dependent part of the string is returned as retID.
 */

static BusType
StringToBusType(const char* busID, const char **retID)
{
    char *p, *s;
    BusType ret = BUS_NONE;

    /* If no type field, Default to PCI */
    if (isdigit(busID[0])) {
	if (retID)
	    *retID = busID;
	return BUS_PCI;
    }

    s = xstrdup(busID);
    p = strtok(s, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return BUS_NONE;
    }
    if (!xf86NameCmp(p, "pci") || !xf86NameCmp(p, "agp"))
	ret = BUS_PCI; 
    if (!xf86NameCmp(p, "isa"))
	ret = BUS_ISA;
    if (ret != BUS_NONE)
	if (retID)
	    *retID = busID + strlen(p) + 1;
    xfree(s);
    return ret;
}


/*
 * Parse a BUS ID string, and return the PCI bus parameters if it was
 * in the correct format for a PCI bus id.
 */

Bool
xf86ParsePciBusString(const char *busID, int *bus, int *device, int *func)
{
    /*
     * The format is assumed to be "bus:device:func", where bus, device
     * and func are decimal integers.  func may be omitted and assumed to
     * be zero, although it doing this isn't encouraged.
     */

    char *p, *s;
    const char *id;
    int i;

    if (StringToBusType(busID, &id) != BUS_PCI)
	return FALSE;

    s = xstrdup(id);
    p = strtok(s, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return FALSE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    xfree(s);
	    return FALSE;
	}
    }
    *bus = atoi(p);
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return FALSE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    xfree(s);
	    return FALSE;
	}
    }
    *device = atoi(p);
    *func = 0;
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return TRUE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    xfree(s);
	    return FALSE;
	}
    }
    *func = atoi(p);
    xfree(s);
    return TRUE;
}

/*
 * Compare a BUS ID string with a PCI bus id.  Return TRUE if they match.
 */

Bool
xf86ComparePciBusString(const char *busID, int bus, int device, int func)
{
    int ibus, idevice, ifunc;

    if (xf86ParsePciBusString(busID, &ibus, &idevice, &ifunc)) {
	return bus == ibus && device == idevice && func == ifunc;
    } else {
	return FALSE;
    }
}

/*
 * If the slot requested is already in use, return FALSE.
 * Otherwise, claim the slot for the screen requesting it.
 */

int
xf86ClaimIsaSlot(DriverPtr drvp, int chipset, GDevPtr dev, Bool active)
{
    EntityPtr p;
    BusAccPtr pbap = xf86PciBusAccInfo;
    int num;
    
    num = xf86AllocateEntity();
    p = xf86Entities[num];
    p->driver = drvp;
    p->chipset = chipset;
    p->busType = BUS_ISA;
    p->device = dev;
    p->active = active;
    p->inUse = FALSE;
    p->access = xnfcalloc(1,sizeof(EntityAccessRec));
    p->access->fallback = &AccessNULL;
    p->access->pAccess = &AccessNULL;
    p->busAcc = NULL;
    while (pbap) {
	if (pbap->type == BUS_ISA)
	    p->busAcc = pbap;
	pbap = pbap->next;
    }
    return num;
}

/*
 * Get the list of ISA "slots" claimed by a screen
 *
 * Note: The ISA implementation here assumes that only one ISA "slot" type
 * can be claimed by any one screen.  That means a return value other than
 * 0 or 1 isn't useful.
 */
int
xf86GetIsaInfoForScreen(int scrnIndex)
{
    int num = 0;
    int i;
    EntityPtr p;
    
    for (i = 0; i < xf86Screens[scrnIndex]->numEntities; i++) {
	p = xf86Entities[xf86Screens[scrnIndex]->entityList[i]];
  	if (p->busType == BUS_ISA) {
  	    num++;
  	}
    }
    return num;
}

/*
 * Parse a BUS ID string, and return True if it is a ISA bus id.
 */

Bool
xf86ParseIsaBusString(const char *busID)
{
    /*
     * The format assumed to be "isa" or "isa:"
     */
    return (StringToBusType(busID,NULL) == BUS_ISA);
}

/*
 * IO enable/disable related routines for PCI
 */
#define SETBITS PCI_CMD_IO_ENABLE
static void
pciIoAccessEnable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIoAccessEnable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG,
			 SETBITS, SETBITS);
}

static void
pciIoAccessDisable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIoAccessDisable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG, SETBITS, 0);
}

#undef SETBITS
#define SETBITS (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE)
static void
pciIo_MemAccessEnable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIo_MemAccessEnable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG,
			 SETBITS, SETBITS);
}

static void
pciIo_MemAccessDisable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIo_MemAccessDisable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG, SETBITS, 0);
}

#undef SETBITS
#define SETBITS (PCI_CMD_MEM_ENABLE)
static void
pciMemAccessEnable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciMemAccessEnable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG,
			 SETBITS, SETBITS);
}

static void
pciMemAccessDisable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciMemAccessDisable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG,SETBITS, 0);
}
#undef SETBITS

#define PCI_PCI_BRDG_CTRL_BASE (PCI_PCI_BRIDGE_CONTROL_REG & 0xFC)
#define SHIFT_BITS ((PCI_PCI_BRIDGE_CONTROL_REG & 0x3) << 3)
#define SETBITS (CARD32)((PCI_PCI_BRIDGE_VGA_EN) << SHIFT_BITS)
static void
pciBusAccessEnable(BusAccPtr ptr)
{
    ptr->busdep.pci.func(ptr->busdep.pci.acc,PCI_PCI_BRDG_CTRL_BASE,
			 SETBITS,SETBITS);
}

static void
pciBusAccessDisable(BusAccPtr ptr)
{
    ptr->busdep.pci.func(ptr->busdep.pci.acc,PCI_PCI_BRDG_CTRL_BASE,SETBITS,0);
}
#undef SETBITS
#undef SHIFT_BITS
static void
pciSetBusAccess(BusAccPtr ptr)
{
#ifdef DEBUG
    ErrorF("pciSetBusAccess: route VGA to bus %d\n", ptr->busdep.pci.bus);
#endif

    if (!ptr->primary && !ptr->current)
	return;
    
    if (ptr->current && ptr->current->disable_f)
	ptr->current->disable_f(ptr->current);
    ptr->current = NULL;
    
    /* walk down */
    while (ptr->primary) { /* no enable for top bus */
	if (ptr->primary->current != ptr) {
	    if (ptr->primary->current && ptr->primary->current->disable_f)
		ptr->primary->current->disable_f(ptr->primary->current);
	    if (ptr->enable_f) ptr->enable_f(ptr);
	    ptr->primary->current = ptr;
	}
	ptr = ptr->primary;
    }
}

static void
savePciState(PCITAG tag, pciSavePtr ptr)
{
    int i;
    
    ptr->command = pciReadLong(tag,PCI_CMD_STAT_REG);
    for (i=0; i < 6; i++) 
        ptr->base[i] = pciReadLong(tag,PCI_CMD_BASE_REG + i*4 );
    ptr->biosBase = pciReadLong(tag,PCI_CMD_BIOS_REG);
}

static void
restorePciState(PCITAG tag, pciSavePtr ptr)
{
    int i;
    
    /* disable card before setting anything */
    pciSetBitsLong(tag, PCI_CMD_STAT_REG, PCI_CMD_MEM_ENABLE
		   | PCI_CMD_IO_ENABLE , 0);
    pciWriteLong(tag,PCI_CMD_BIOS_REG,ptr->biosBase);
    for (i=0; i<6; i++)
        pciWriteLong(tag,PCI_CMD_BASE_REG + i*4, ptr->base[i]);        
    pciWriteLong(tag,PCI_CMD_STAT_REG,ptr->command);
}

static void
savePciBusState(BusAccPtr ptr)
{
    ptr->busdep.pci.save.io =
		pciReadWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_IO_REG);
    ptr->busdep.pci.save.mem =
		pciReadWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_MEM_REG);
    ptr->busdep.pci.save.pmem =
		pciReadWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_PMEM_REG);
    ptr->busdep.pci.save.control =
		pciReadByte(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_CONTROL_REG);
}

static void
restorePciBusState(BusAccPtr ptr)
{
    pciWriteWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_IO_REG,
		 ptr->busdep.pci.save.io);
    pciWriteWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_MEM_REG,
		 ptr->busdep.pci.save.mem);
    pciWriteWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_PMEM_REG,
		 ptr->busdep.pci.save.pmem);
    pciWriteByte(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_CONTROL_REG,
		 ptr->busdep.pci.save.control);
}

/*-----------------------------------------------------------------*/
static void
initPciState(void)
{
    int i = 0;
    int j = 0;
    pciVideoPtr pvp; 
    pciAccPtr pcaccp;

    if (xf86PciAccInfo != NULL)
	return;
  
    if (xf86PciVideoInfo == NULL)
	return;

    while ((pvp = xf86PciVideoInfo[i]) != NULL) {
  	i++;
  	    j++;
  	    xf86PciAccInfo = xnfrealloc(xf86PciAccInfo,
  					sizeof(pciAccPtr) * (j + 1));
  	    xf86PciAccInfo[j] = NULL;
  	    pcaccp = xf86PciAccInfo[j - 1] = xnfalloc(sizeof(pciAccRec));
	    pcaccp->busnum = pvp->bus; 
 	    pcaccp->devnum = pvp->device; 
 	    pcaccp->funcnum = pvp->func;
 	    pcaccp->arg.tag = pciTag(pvp->bus, pvp->device, pvp->func);
 	    pcaccp->arg.func =
			(SetBitsProcPtr)pciLongFunc(pcaccp->arg.tag,SET_BITS);
  	    pcaccp->ioAccess.AccessDisable = pciIoAccessDisable;
  	    pcaccp->ioAccess.AccessEnable = pciIoAccessEnable;
  	    pcaccp->ioAccess.arg = &pcaccp->arg;
	    pcaccp->io_memAccess.AccessDisable = pciIo_MemAccessDisable;
	    pcaccp->io_memAccess.AccessEnable = pciIo_MemAccessEnable;
	    pcaccp->io_memAccess.arg = &pcaccp->arg;
	    pcaccp->memAccess.AccessDisable = pciMemAccessDisable;
	    pcaccp->memAccess.AccessEnable = pciMemAccessEnable;
	    pcaccp->memAccess.arg = &pcaccp->arg;
 	    if (PCISHAREDIOCLASSES(pvp->class, pvp->subclass))
 		pcaccp->ctrl = TRUE;
 	    else
 		pcaccp->ctrl = FALSE;
 	    savePciState(pcaccp->arg.tag, &pcaccp->save);
    }
}

/*
 * initPciBusState() - fill out the BusAccRec for a PCI bus.
 * Theory: each bus is associated with one bridge connecting it
 * to its parent bus. The address of a bridge is therefore stored
 * in the BusAccRec of the bus it connects to. Each bus can
 * have several bridges connecting secondary buses to it. Only one
 * of these bridges can be open. Therefore the status of a bridge
 * associated with a bus is stored in the BusAccRec of the parent
 * the bridge connects to. The first member of the structure is
 * a pointer to a function that open access to this bus. This function
 * receives a pointer to the structure itself as argument. This
 * design should be common to BusAccRecs of any type of buses we
 * support. The remeinder of the structure is bus type specific.
 * In this case it contains a pointer to the structure of the
 * parent bus. Thus enabling access to a specific bus is simple:
 * 1. Close any bridge going to secondary buses.
 * 2. Climb down the ladder and enable any bridge on buses
 *    on the path from the CPU to this bus.
 */
 
static void
initPciBusState(void)
{
    BusAccPtr *ppbap_next = &xf86PciBusAccInfo;
    BusAccPtr pbap, pbap_tmp;
    PciBusPtr pbp = xf86PciBus;

    while (pbp) {
	(*ppbap_next) = pbap = xnfcalloc(1,sizeof(BusAccRec));
	pbap->busdep.pci.bus = pbp->secondary;
#if 0
	pbap->save_f = NULL;
	pbap->restore_f = NULL;
	pbap->set_f = NULL;
	pbap->enable_f = NULL;
	pbap->disable_f = NULL;
	pbap->current = NULL;
	pbap->next = NULL;
#endif
	pbap->busdep.pci.acc = PCITAG_SPECIAL;
	switch (pbp->subclass) {
	case PCI_SUBCLASS_BRIDGE_HOST:
#ifdef DEBUG
	    ErrorF("setting up HOST: %i\n",pbap->busdep.pci.bus);
#endif
	    pbap->type = BUS_PCI;
	    pbap->set_f = pciSetBusAccess;
	    break;
	case PCI_SUBCLASS_BRIDGE_PCI:
#ifdef DEBUG
	    ErrorF("setting up PCI: %i\n",pbap->busdep.pci.bus);
#endif
	    pbap->type = BUS_PCI;
	    pbap->save_f = savePciBusState;
	    pbap->restore_f = restorePciBusState;
	    pbap->set_f = pciSetBusAccess;
	    pbap->enable_f = pciBusAccessEnable;
	    pbap->disable_f = pciBusAccessDisable;
	    pbap->busdep.pci.acc = pciTag(pbp->brbus,pbp->brdev,pbp->brfunc);
	    pbap->busdep.pci.func =
		(SetBitsProcPtr)pciLongFunc(pbap->busdep.pci.acc,SET_BITS);
	    savePciBusState(pbap);
	    break;
	case PCI_SUBCLASS_BRIDGE_ISA:
#ifdef DEBUG
	    ErrorF("setting up ISA: %i\n",pbap->busdep.pci.bus);
#endif
	    pbap->type = BUS_ISA;
	    pbap->set_f = pciSetBusAccess;
	    break;
	}
	pbap->next = NULL;
	ppbap_next = &pbap->next;
	pbp = pbp->next;
    }
    
    pbp = xf86PciBus;
    pbap = xf86PciBusAccInfo;
    
    while (pbp) {
	pbap->primary = NULL;
	if (pbp->primary > -1) {
	    pbap_tmp = xf86PciBusAccInfo;
	    while (pbap_tmp) {
		if (pbap_tmp->busdep.pci.bus == pbp->primary) {
		    pbap->primary = pbap_tmp;
		    break;
		}
		pbap_tmp = pbap_tmp->next;
	    }
	    /* set initial state */
#if 0  /* before doing anything we close all bridges */
	    if (pbap->primary
		&& pbp->subclass == PCI_SUBCLASS_BRIDGE_PCI
		&& pbp->brcontrol & 0x08)
		pbap->primary->current = pbap;
#endif
	}
	pbap = pbap->next;
	pbp = pbp->next;
    }
}

/*--------------------------------------------------------------------*/

static void 
PciStateEnter(void)
{
    pciAccPtr paccp;
    int i = 0;
    
    if (xf86PciAccInfo == NULL) 
	return;

    while ((paccp = xf86PciAccInfo[i]) != NULL) {
	i++;
 	if (!paccp->ctrl)
 	    continue;
	savePciState(paccp->arg.tag, &paccp->save);
	restorePciState(paccp->arg.tag, &paccp->restore);
    }
}

static void
PciBusStateEnter(void)
{
    BusAccPtr pbap = xf86PciBusAccInfo;

    while (pbap) {
	if (pbap->save_f)
	    pbap->save_f(pbap);
	pbap = pbap->next;
    }
}

static void 
PciStateLeave(void)
{
    pciAccPtr paccp;
    int i = 0;

    if (xf86PciAccInfo == NULL) 
	return;

    while ((paccp = xf86PciAccInfo[i]) != NULL) {
	i++;
	if (!paccp->ctrl)
	    continue;
	savePciState(paccp->arg.tag, &paccp->restore);
	restorePciState(paccp->arg.tag, &paccp->save);
    }
}

static void
PciBusStateLeave(void)
{
    BusAccPtr pbap = xf86PciBusAccInfo;

    while (pbap) {
	if (pbap->restore_f)
	    pbap->restore_f(pbap);
	pbap = pbap->next;
    }
}

static void 
DisablePciAccess(void)
{
    int i = 0;
    pciAccPtr paccp;
    if (xf86PciAccInfo == NULL)
	return;

    while ((paccp = xf86PciAccInfo[i]) != NULL) {
	i++;
	if (!paccp->ctrl) /* disable devices that are under control initially*/
	    continue;
	pciIo_MemAccessDisable(paccp->io_memAccess.arg);
    }
}

static void
disablePciBios(PCITAG tag)
{
    pciSetBitsLong(tag, PCI_CMD_BIOS_REG, PCI_CMD_BIOS_ENABLE, 0);
}

static void
DisablePciBusAccess(void)
{
    BusAccPtr pbap = xf86PciBusAccInfo;

    while (pbap) {
	if (pbap->disable_f)
	    pbap->disable_f(pbap);
	if (pbap->primary)
	    pbap->primary->current = NULL;
	pbap = pbap->next;
    }
}

/*
 * xf86IsPrimaryPci() -- return TRUE if primary device
 * is PCI and bus, dev and func numbers match.
 */
 
Bool
xf86IsPrimaryPci(pciVideoPtr pPci)
{
    if (primaryBus.type != BUS_PCI) return FALSE;
    return (pPci->bus == primaryBus.id.pci.bus &&
	    pPci->device == primaryBus.id.pci.device &&
	    pPci->func == primaryBus.id.pci.func);
}

/*
 * xf86IsPrimaryIsa() -- return TRUE if primary device
 * is ISA.
 */
 
Bool
xf86IsPrimaryIsa(void)
{
    return ( primaryBus.type == BUS_ISA );
}

/*
 * Generic VGA IO part - add other buses here
 */

void
xf86EntityInit(void)
{
    int i;
    resPtr *pprev_next;
    resPtr res;
    xf86AccessPtr pacc;
    
    for (i = 0; i < xf86NumEntities; i++)
	if (xf86Entities[i]->entityInit) {
	    if (xf86Entities[i]->access->busAcc)
		((BusAccPtr)xf86Entities[i]->access->busAcc)->set_f
		    (xf86Entities[i]->access->busAcc);
	    pacc = xf86Entities[i]->access->fallback;
	    if (pacc->AccessEnable)
		pacc->AccessEnable(pacc->arg);
	    xf86Entities[i]->entityInit(i,xf86Entities[i]->private);
	    if (pacc->AccessDisable)
		pacc->AccessDisable(pacc->arg);
	    /* remove init resources after init is processed */
	    pprev_next = &Acc;
	    res = Acc;
	    while (res) {  
		if (res->res_type & ResInit && (res->entityIndex == i)) {
		    (*pprev_next) = res->next;
		    xfree(res);
		} else 
		    pprev_next = &(res->next);
		res = (*pprev_next);
	    }
	}
}

static void
EntityEnter(void)
{
    int i;
    xf86AccessPtr pacc;
    
    for (i = 0; i < xf86NumEntities; i++)
	if (xf86Entities[i]->entityEnter) {
	    if (xf86Entities[i]->access->busAcc)
		((BusAccPtr)xf86Entities[i]->access->busAcc)->set_f
		    (xf86Entities[i]->access->busAcc);
	    pacc = xf86Entities[i]->access->fallback;
	    if (pacc->AccessEnable)
		pacc->AccessEnable(pacc->arg);
	    xf86Entities[i]->entityEnter(i,xf86Entities[i]->private);
	    if (pacc->AccessDisable)
		pacc->AccessDisable(pacc->arg);
	}
}

static void
EntityLeave(void)
{
    int i;
    xf86AccessPtr pacc;

    for (i = 0; i < xf86NumEntities; i++)
	if (xf86Entities[i]->entityLeave) {
	    if (xf86Entities[i]->access->busAcc)
		((BusAccPtr)xf86Entities[i]->access->busAcc)->set_f
		    (xf86Entities[i]->access->busAcc);
	    pacc = xf86Entities[i]->access->fallback;
	    if (pacc->AccessEnable)
		pacc->AccessEnable(pacc->arg);
	    xf86Entities[i]->entityLeave(i,xf86Entities[i]->private);
	    if (pacc->AccessDisable)
		pacc->AccessDisable(pacc->arg);
	}
}

static void
disableAccess(void)
{
    int i;
    xf86AccessPtr pacc;
    EntityAccessPtr peacc;
    
    /* call disable funcs and reset current access pointer */
    /* the entity specific access funcs are in an enabled  */
    /* state - driver must restore their state explicitely */
    for (i = 0; i < xf86NumScreens; i++) {
	peacc = xf86Screens[i]->CurrentAccess->pIoAccess;
	while (peacc) {
	    if (peacc->pAccess->AccessDisable)
		peacc->pAccess->AccessDisable(peacc->pAccess->arg);
	    peacc = peacc->next;
	}
	xf86Screens[i]->CurrentAccess->pIoAccess = NULL;
	peacc = xf86Screens[i]->CurrentAccess->pMemAccess;
	while (peacc) {
	    if (peacc->pAccess->AccessDisable)
		peacc->pAccess->AccessDisable(peacc->pAccess->arg);
	    peacc = peacc->next;
	}
	xf86Screens[i]->CurrentAccess->pMemAccess = NULL;
    }
    /* then call the generic entity disable funcs */
    for (i = 0; i < xf86NumEntities; i++) {
	pacc = xf86Entities[i]->access->fallback; 
	if (pacc->AccessDisable)
	    pacc->AccessDisable(pacc->arg);
    }
}

/*
 * xf86AccessInit() - set up everything needed for access control
 * called only once on first server generation.
 */

void
xf86AccessInit(void)
{
    initPciState();
    initPciBusState();
    DisablePciBusAccess();
    DisablePciAccess();
    
    xf86ResAccessEnter = TRUE;
}

/*
 * xf86AccessEnter() -- gets called to save the text mode VGA IO 
 * resources when reentering the server after a VT switch.
 */ 

void
xf86AccessEnter(void)
{
    if (xf86ResAccessEnter) 
	return;

    /*
     * on enter we simply disable routing of special resources
     * to any bus and let the RAC code to "open" the right bridges.
     */
    PciBusStateEnter();
    DisablePciBusAccess();
    PciStateEnter();
    disableAccess();
    EntityEnter();
    xf86EnterServerState(SETUP);
    xf86ResAccessEnter = TRUE;
}

/*
 * xf86AccessLeave() -- prepares access for and calls the
 * entityLeave() functions.
 * xf86AccessLeaveState() --- gets called to restore the
 * access to the VGA IO resources when switching VT or on
 * server exit.
 * This was split to call xf86AccessLeaveState() from
 * ddxGiveUp().
 */

void
xf86AccessLeave(void)
{
    if (!xf86ResAccessEnter)
	return;
    disableAccess();
    DisablePciBusAccess();
    EntityLeave();
}

void
xf86AccessLeaveState(void)
{
    if (!xf86ResAccessEnter)
	return;
    xf86ResAccessEnter = FALSE;
    PciStateLeave();
    PciBusStateLeave();
}

/*
 * xf86EnableAccess() -- enable access to controlled resources.
 * To reduce latency when switching access the ScrnInfoRec has
 * a linked list of the EntityAccPtr of all screen entities.
 */
/*
 * switching access needs to be done in te following oder:
 * disable
 * 1. disable old entity
 * 2. reroute bus
 * 3. enable new entity
 * Otherwise resources needed for access control might be shadowed
 * by other resources!
 */
void
xf86EnableAccess(ScrnInfoPtr pScrn)
{
    register EntityAccessPtr peAcc = (EntityAccessPtr) pScrn->access;
    register EntityAccessPtr pceAcc;
    register xf86AccessPtr pAcc;
    EntityAccessPtr tmp;
#ifdef DEBUG
    ErrorF("Enable access %i\n",pScrn->scrnIndex);
#endif

    /* Entity is not under access control or currently enabled */
    if (!pScrn->access) {
	if (pScrn->busAccess)
	    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
	return;
    }
    
    switch (pScrn->resourceType) {
    case IO:
	pceAcc = pScrn->CurrentAccess->pIoAccess;
	if (peAcc == pceAcc) {
#if 0
	    if (pScrn->busAccess)
		((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
#endif
	    return;
	}
	if (pScrn->CurrentAccess->pMemAccess == pceAcc)
	    pScrn->CurrentAccess->pMemAccess = NULL;
	while (pceAcc) {
	    pAcc = pceAcc->pAccess;
	    if ( pAcc && pAcc->AccessDisable) 
		(*pAcc->AccessDisable)(pAcc->arg);
	    pceAcc = pceAcc->next;
	}
	if (pScrn->busAccess)
	    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
	while (peAcc) {
	    pAcc = peAcc->pAccess;
	    if (pAcc && pAcc->AccessEnable) 
		(*pAcc->AccessEnable)(pAcc->arg);
	    peAcc = peAcc->next;
	}
	pScrn->CurrentAccess->pIoAccess = (EntityAccessPtr) pScrn->access;
	return;
	
    case MEM_IO:
	pceAcc = pScrn->CurrentAccess->pIoAccess;
	if (peAcc != pceAcc) { /* current Io != pAccess */
	    tmp = pceAcc;
	    while (pceAcc) {
		pAcc = pceAcc->pAccess;
		if (pAcc && pAcc->AccessDisable)
		    (*pAcc->AccessDisable)(pAcc->arg);
		pceAcc = pceAcc->next;
	    }
	    pceAcc = pScrn->CurrentAccess->pMemAccess;
	    if (peAcc != pceAcc /* current Mem != pAccess */
		&& tmp !=pceAcc) {
		while (pceAcc) {
		    pAcc = pceAcc->pAccess;
		    if (pAcc && pAcc->AccessDisable)
			(*pAcc->AccessDisable)(pAcc->arg);
		    pceAcc = pceAcc->next;
		}
	    }
	} else {    /* current Io == pAccess */
	    pceAcc = pScrn->CurrentAccess->pMemAccess;
	    if (pceAcc == peAcc) { /* current Mem == pAccess */
#if 0
		if (pScrn->busAccess)
		    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
#endif
		return;
	    }
	    while (pceAcc) {  /* current Mem != pAccess */
		pAcc = pceAcc->pAccess;
		if (pAcc && pAcc->AccessDisable) 
		    (*pAcc->AccessDisable)(pAcc->arg);
		pceAcc = pceAcc->next;
	    }
	}
	if (pScrn->busAccess)
	    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
	while (peAcc) {
	    pAcc = peAcc->pAccess;
	    if (pAcc && pAcc->AccessEnable) 
		(*pAcc->AccessEnable)(pAcc->arg);
		peAcc = peAcc->next;
	}
	pScrn->CurrentAccess->pMemAccess =
	    pScrn->CurrentAccess->pIoAccess = (EntityAccessPtr) pScrn->access;
	return;
	
    case MEM:
	pceAcc = pScrn->CurrentAccess->pMemAccess;
	if (peAcc == pceAcc) {
#if 0
	    if (pScrn->busAccess)
		((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
#endif
	    return;
	}
	if (pScrn->CurrentAccess->pIoAccess == pceAcc)
	    pScrn->CurrentAccess->pIoAccess = NULL;
	while (pceAcc) {
	    pAcc = pceAcc->pAccess;
	    if ( pAcc && pAcc->AccessDisable) 
		(*pAcc->AccessDisable)(pAcc->arg);
	    pceAcc = pceAcc->next;
	}
	if (pScrn->busAccess)
	    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
	while (peAcc) {
	    pAcc = peAcc->pAccess;
	    if (pAcc && pAcc->AccessEnable) 
		(*pAcc->AccessEnable)(pAcc->arg);
	    peAcc = peAcc->next;
	}
	pScrn->CurrentAccess->pMemAccess = (EntityAccessPtr) pScrn->access;
	return;

    case NONE:
	if (pScrn->busAccess)
	    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
	return;
    }
}

void
xf86SetAccessFuncs(EntityInfoPtr pEnt, xf86AccessPtr p_io, xf86AccessPtr p_mem,
		   xf86AccessPtr p_io_mem, xf86AccessPtr *ppAccessOld)
{
    AccessFuncPtr rac;

    if (!xf86Entities[pEnt->index]->rac)
	xf86Entities[pEnt->index]->rac = xnfcalloc(1,sizeof(AccessFuncRec));

    rac = xf86Entities[pEnt->index]->rac;

    if (p_mem == p_io_mem && p_mem && p_io)
	xf86Entities[pEnt->index]->entityProp |= NO_SEPARATE_MEM_FROM_IO;
    if (p_io == p_io_mem && p_mem && p_io)
	xf86Entities[pEnt->index]->entityProp |= NO_SEPARATE_IO_FROM_MEM;
    
    rac->mem_new = p_mem;
    rac->io_new = p_io;
    rac->io_mem_new = p_io_mem;
    
    rac->old = ppAccessOld;
}

/*------------------------------------------------------------*/
static void CheckGenericGA(void);

/*
 * xf86FindPrimaryDevice() - Find the display device which
 * was active when the server was started. Side effects:
 * - disable IO access to all VGA/8514 display adaptors 
 *   if possible.
 */
void
xf86FindPrimaryDevice()
{
    /* if no VGA device is found check for primary PCI device */
    if (primaryBus.type == BUS_NONE)
        CheckGenericGA();
}

#include "vgaHW.h"
#include "compiler.h"

/*
 * CheckGenericGA() - Check for presence of a VGA device.
 */
static void
CheckGenericGA()
{
#if !defined(__sparc__) && !defined(__powerpc__) /* FIXME ?? */
    CARD16 GenericIOBase = VGAHW_GET_IOBASE();
    CARD8 CurrentValue, TestValue;

    /* VGA CRTC registers are not used here, so don't bother unlocking them */

    /* VGA has one more read/write attribute register than EGA */
    (void) inb(GenericIOBase + 0x0AU);  /* Reset flip-flop */
    outb(0x3C0, 0x14 | 0x20);
    CurrentValue = inb(0x3C1);
    outb(0x3C0, CurrentValue ^ 0x0F);
    outb(0x3C0, 0x14 | 0x20);
    TestValue = inb(0x3C1);
    outb(0x3C0, CurrentValue);

    if ((CurrentValue ^ 0x0F) == TestValue) {
	primaryBus.type = BUS_ISA;
    }
#endif
}

/*
 * xf86CheckPciGAType() -- return type of PCI graphics adapter.
 */
int
xf86CheckPciGAType(pciVideoPtr pPci)
{
    int i = 0;
    pciConfigPtr pcp;
    
    while ((pcp = xf86PciInfo[i]) != NULL) { 
	if (pPci->bus == pcp->busnum && pPci->device == pcp->devnum
	    && pPci->func == pcp->funcnum) {
	    if (pcp->pci_base_class == PCI_CLASS_PREHISTORIC &&
		pcp->pci_sub_class == PCI_SUBCLASS_PREHISTORIC_VGA)
		return PCI_CHIP_VGA ;
	    if (pcp->pci_base_class == PCI_CLASS_DISPLAY &&
		pcp->pci_sub_class == PCI_SUBCLASS_DISPLAY_VGA) {
		if (pcp->pci_prog_if == 0)
		    return PCI_CHIP_VGA ; 
		if (pcp->pci_prog_if == 1)
		    return PCI_CHIP_8514;
	    }
	    return -1;
	}
    i++;
    }
    return -1;
}

/* new RAC */

static memType
getMask(memType val)
{
    memType mask = 0;
    memType tmp = 0;
    
    mask=~mask;
    tmp = ~((~tmp) >> 1);
    
    while (!(val & tmp)) {
	mask = mask >> 1;
	val = val << 1;
    }
    return mask;
}

/*
 * checkConflictBlock() -- check for conflicts of a block resource range.
 * If conflict is found return end of conflicting range. Else return 0.
 */
static memType
checkConflictBlock(resRange *range, resPtr pRes)
{
    memType val,tmp,prev;
    int i;
    
    switch (pRes->res_type & ResExtMask) {
    case ResBlock:
	if (range->rBegin < pRes->block_end &&
	    range->rEnd > pRes->block_begin)
	    return pRes->block_end < range->rEnd ?
					pRes->block_end : range->rEnd;
	return 0;
    case ResSparse:
	if (pRes->sparse_base > range->rEnd) return 0;
	
	val = (~pRes->sparse_mask | pRes->sparse_base) & getMask(range->rEnd);
#ifdef DEBUG
	ErrorF("base = 0x%lx, mask = 0x%lx, begin = 0x%lx, end = 0x%lx ,"
	       "val = 0x%lx\n",
		pRes->sparse_base, pRes->sparse_mask, range->rBegin,
		range->rEnd, val);
#endif
	i = sizeof(memType) * 8;
	tmp = prev = pRes->sparse_base;
	
	while (i) {
#ifdef DEBUG
	    ErrorF("tmp = 0x%lx\n",tmp);
#endif
	    tmp |= 1<< (--i) & val;
	    if (tmp > range->rEnd)
		tmp = prev;
	    else
		prev = tmp;
	}
	if (tmp >= range->rBegin) {
#ifdef DEBUG
	    ErrorF("conflict found at: 0x%lx\n",tmp);
#endif
	    return tmp;
	}
	else
	    return 0;
    }
    return 0;
}

/*
 * checkConflictSparse() -- check for conflicts of a sparse resource range.
 * If conflict is found return base of conflicting region. Else return 0.
 */
#define mt_max ~(memType)0
#define length sizeof(memType) * 8
static memType
checkConflictSparse(resRange *range, resPtr pRes)
{
    memType val, tmp, prev;
    int i;
    
    switch (pRes->res_type & ResExtMask) {
    case ResSparse:
	tmp = pRes->sparse_mask & range->rMask;
	if ((tmp & pRes->sparse_base) == (tmp & range->rBase))
	    return pRes->sparse_mask;
	return 0;

    case ResBlock:
	if (pRes->block_end < range->rBase) return 0;
	
	val = (~range->rMask | range->rBase) & getMask(pRes->block_end);
	i = length;
	tmp = prev = range->rBase;
	
	while (i) {
#ifdef DEBUG
	    ErrorF("tmp = 0x%lx\n",tmp);
#endif
	    tmp |= 1<< (--i) & val;
	    if (tmp > pRes->block_end)
		tmp = prev;
	    else
		prev = tmp;
	}
	if (tmp < pRes->block_begin) 
	    return 0;
	else {
	    /*
	     * now we subdivide the block region in sparse regions
	     * with base values = 2^n and find the smallest mask.
	     * This might be done in a simpler way....
	     */
	    memType mask, m_mask = 0, base = pRes->block_begin;
	    int i;	    
	    while (base < pRes->block_end) {
		for (i = 1; i < length; i++)
		    if ( base != (base & (mt_max << i))) break;
		mask = mt_max >> (length - i);
		do mask >>= 1;
		while ((mask + base + 1) > pRes->block_end);
		/* m_mask and are _inverted_ sparse masks */ 
		m_mask = mask > m_mask ? mask : m_mask;
		base = base + mask + 1;
	    }
#ifdef DEBUG
	    ErrorF("conflict found at: 0x%lx\n",tmp);
#endif
	    return ~m_mask; 
	}
    }
    return 0;
}
#undef mt_max
#undef length

/*
 * needCheck() -- this function decides whether to check for conflicts
 * depending on the types of the resource ranges and their locations
 */
static Bool
needCheck(resPtr pRes, long type, int entityIndex, xf86State state)
{
    /* the same entity shouldn't conflict with itself */
    ScrnInfoPtr pScrn;
    int i;
    BusType loc = BUS_NONE;
    BusType r_loc = BUS_NONE;
    
    if (!(pRes->res_type & type & ResPhysMask))
	return FALSE;

    /*
     * Resources set by BIOS (ResBios) are allowed to conflict
     * with resources marked (ResBios).
     */
    if (pRes->res_type & type & ResBios)
	return FALSE;

    /* If requested, skip over estimated resources */
    if (pRes->res_type & type & ResEstimated)
	return FALSE;
    
    if (type & pRes->res_type & ResUnused)
	return FALSE;

    if (state == OPERATING) {
	if (type & ResDisableOpr || pRes->res_type & ResDisableOpr)
	    return FALSE;
	if (type & pRes->res_type & ResUnusedOpr) return FALSE;
	/*
	 * Maybe we should have ResUnused set The resUnusedOpr
	 * bit, too. This way we could avoid this confusion
	 */
	if ((type & ResUnusedOpr && pRes->res_type & ResUnused) ||
	    (type & ResUnused && pRes->res_type & ResUnusedOpr))
	    return FALSE;
    }
    
    if (entityIndex > -1)
	loc = xf86Entities[entityIndex]->busType;
    if (pRes->entityIndex > -1)
	r_loc = xf86Entities[pRes->entityIndex]->busType;

    switch (type & ResAccMask) {
    case ResExclusive:
	switch (pRes->res_type & ResAccMask) {
	case ResExclusive:
	    break;
	case ResShared:
	    /* ISA buses are only locally exclusive on a PCI system */
	    if (loc == BUS_ISA && r_loc == BUS_PCI)
		return FALSE;
	    break;
	}
	break;
    case ResShared:
	switch (pRes->res_type & ResAccMask) {
	case ResExclusive:
	    /* ISA buses are only locally exclusive on a PCI system */
	    if (loc == BUS_PCI && r_loc == BUS_ISA) 
		return FALSE;
	    break;
	case ResShared:
	    return FALSE;
	}
	break;
    case ResAny:
	break;
    }
    
    if (pRes->entityIndex == entityIndex) return FALSE;

    if (pRes->entityIndex > -1 &&
	(pScrn = xf86FindScreenForEntity(entityIndex))) {
	for (i = 0; i < pScrn->numEntities; i++)
	    if (pScrn->entityList[i] == pRes->entityIndex) return FALSE;
    }
    return TRUE;
}

/*
 * checkConflict() - main conflict checking function which all other
 * function call.
 */
static memType
checkConflict(resRange *rgp, resPtr pRes, int entityIndex, xf86State state)
{
    memType ret;
    
    while(pRes) {
	if (!needCheck(pRes,rgp->type, entityIndex ,state)) { 
	    pRes = pRes->next;                    
	    continue;                             
	}
	switch (rgp->type & ResExtMask) {
	case ResBlock:
	    if (rgp->rEnd < rgp->rBegin) {
		xf86Msg(X_ERROR,"end of block range 0x%lx < begin 0x%lx\n",
			rgp->rEnd,rgp->rBegin);
		return 0;
	    }
	    if ((ret = checkConflictBlock(rgp, pRes)))
		return ret;
	    break;
	case ResSparse:
	    if ((rgp->rBase & rgp->rMask) != rgp->rBase) {
		xf86Msg(X_ERROR,"sparse io range (base: 0x%lx  mask: 0x%lx)"
			"doesn't satisfy (base & mask = mask)\n",
			rgp->rBase, rgp->rMask);
		return 0;
	    }
	    if ((ret = checkConflictSparse(rgp, pRes)))
		return ret;
	    break;
	}
	pRes = pRes->next;
    }
    return 0;
}

/*
 * ChkConflict() -- used locally ; find conflict with any location.
 */
static memType
ChkConflict(resRange *rgp, resPtr res, xf86State state)
{
    return checkConflict(rgp, res, -2, state);
}

/*
 * xf86ChkConflict() - This function is the low level interface to
 * the resource broker that gets exported. Tests all resources ie.
 * performs test with SETUP flag.
 */
memType
xf86ChkConflict(resRange *rgp, int entityIndex)
{
    return checkConflict(rgp, Acc, entityIndex, SETUP);
}

resPtr
xf86JoinResLists(resPtr rlist1, resPtr rlist2)
{
    resPtr pRes;

    if (!rlist1)
	return rlist2;

    if (!rlist2)
	return rlist1;

    for (pRes = rlist1; pRes->next; pRes = pRes->next)
	;
    pRes->next = rlist2;
    return rlist1;
}

resPtr
xf86AddResToList(resPtr rlist, resRange *range, int entityIndex)
{
    resPtr new;

    switch (range->type & ResExtMask) {
    case ResBlock:
	if (range->rEnd < range->rBegin) {
		xf86Msg(X_ERROR,"end of block range 0x%lx < begin 0x%lx\n",
			range->rEnd,range->rBegin);
		return rlist;
	}
	break;
    case ResSparse:
	if ((range->rBase & range->rMask) != range->rBase) {
	    xf86Msg(X_ERROR,"sparse io range (base: 0x%lx  mask: 0x%lx)"
		    "doesn't satisfy (base & mask = mask)\n",
		    range->rBase, range->rMask);
	    return rlist;
	}
	break;
    }
    
    new = xnfalloc(sizeof(resRec));
    new->val = *range;
    new->entityIndex = entityIndex;
    new->next = rlist;
    return new;
}

void
xf86FreeResList(resPtr rlist)
{
    resPtr pRes;

    if (!rlist)
	return;

    for (pRes = rlist->next; pRes; rlist = pRes, pRes = pRes->next)
	xfree(rlist);
    xfree(rlist);
}

resPtr
xf86DupResList(const resPtr rlist)
{
    resPtr pRes, ret, prev, new;

    if (!rlist)
	return NULL;

    ret = xnfalloc(sizeof(resRec));
    *ret = *rlist;
    prev = ret;
    for (pRes = rlist->next; pRes; pRes = pRes->next) {
	new = xnfalloc(sizeof(resRec));
	*new = *pRes;
	prev->next = new;
	prev = new;
    }
    return ret;
}

void
xf86PrintResList(int verb, resPtr list)
{
    int i = 0;
    const char *s, *r;
    resPtr tmp = list;
    long type;
    
    if (!list)
	return;

    type = ResMem;
    r = "M";
    while (1) {
	while (list) {
	    if ((list->res_type & ResPhysMask) == type) {
		switch (list->res_type & ResExtMask) {
		case ResBlock:
		    xf86ErrorFVerb(verb, "\t[%d] %d\t0x%08x - 0x%08x (0x%x)",
				   i, list->entityIndex, list->block_begin,
				   list->block_end,
				   list->block_end - list->block_begin + 1);
		    break;
		case ResSparse:
		    xf86ErrorFVerb(verb, "\t[%d] %d\t0x%08x - 0x%08x ",
				   i, list->entityIndex,
				   list->sparse_base,list->sparse_mask);
		    break;
		default:
		    list = list->next;
		    continue;
		}
		xf86ErrorFVerb(verb, " %s", r);
		switch (list->res_type & ResAccMask) {
		case ResExclusive:
		    if (list->res_type & ResUnused)
			s = "x";
		    else
			s = "X";
		    break;
		case ResShared:
		    if (list->res_type & ResUnused)
			s = "s";
		    else
			s = "S";
		    break;
		default:
		    s = "?";
		}
		xf86ErrorFVerb(verb, "%s", s);
		switch (list->res_type & ResExtMask) {
		case ResBlock:
		    s = "B";
		    break;
		case ResSparse:
		    s = "S";
		    break;
		default:
		    s = "?";
		}
		xf86ErrorFVerb(verb, "%s", s);
		if (list->res_type & ResEstimated)
		    xf86ErrorFVerb(verb, "E");
		if (list->res_type & ResInit)
		    xf86ErrorFVerb(verb, "t");
		if (list->res_type & ResBios)
		    xf86ErrorFVerb(verb, "(B)");
		xf86ErrorFVerb(verb, "\n");
		i++;
	    }
	    list = list->next;
	}
	if (type == ResIo) break;
	type = ResIo;
	r = "I";
	list = tmp;
    }
}

#define MEM_ALIGN (1024 * 1024)

/*
 * RemoveOverlaps() -- remove overlaps between resources of the
 * same kind.
 * Beware: This function doesn't check for access attributes.
 * At resource broker initialization this is no problem as this
 * only deals with exclusive resources.
 */
static void
RemoveOverlaps(resPtr target, resPtr list, Bool pciAlignment)
{
    resPtr pRes;
    memType size, newsize, adjust;

    for (pRes = list; pRes; pRes = pRes->next) {
	if (pRes != target
	    && ((pRes->res_type & ResPhysMask) ==
		(target->res_type & ResPhysMask))
	    && pRes->block_begin <= target->block_end
	    && pRes->block_end >= target->block_begin) {
	    /*
	     * target should be a larger region than pRes.  If pRes fully
	     * contains target, don't do anything.
	     */
	    if (pRes->block_begin <= target->block_begin &&
		pRes->block_end >= target->block_end)
		continue;
	    /*
	     * cases where the target and pRes have the same starting address
	     * cannot be resolved, so skip them (with a warning).
	     */
	    if (pRes->block_begin == target->block_begin) {
		xf86MsgVerb(X_WARNING, 3, "Unresolvable overlap at 0x%08x\n",
			    pRes->block_begin);
		continue;
	    }
	    /* Otherwise, trim target to remove the overlap */
	    if (pRes->block_begin <= target->block_end) {
		target->block_end = pRes->block_begin - 1;
	    } else if (!pciAlignment &&
		       pRes->block_end >= target->block_begin) {
		target->block_begin = pRes->block_end + 1;
	    }
	    if (pciAlignment) {
		/*
		 * Align to a power of two.  This requires finding the
		 * largest power of two that is smaller than the adjusted
		 * size.
		 */
		size = target->block_end - target->block_begin + 1;
		newsize = 1UL << (sizeof(memType) * 8 - 1);
		while (!(newsize & size))
		    newsize >>= 1;
		target->block_end = target->block_begin + newsize - 1;
	    } else if (target->block_end > MEM_ALIGN) {
		/* Align the end to MEM_ALIGN */
		if ((adjust = (target->block_end + 1) % MEM_ALIGN))
		    target->block_end -= adjust;
	    }
	}
    }
}

static void
xf86GetPciRes(resPtr *sysRes, resPtr *nonsysRes)
{
    pciConfigPtr pcrp, *pcrpp;
    pciVideoPtr pvp, *pvpp;
    CARD32 *basep;
    int i;
    resPtr pRes;
    resRange range;

    if (sysRes)
	*sysRes = NULL;
    if (nonsysRes)
	*nonsysRes = NULL;

    if (!sysRes || !nonsysRes || !xf86PciInfo)
	return;

    if (xf86PciVideoInfo)
	for (pvpp = xf86PciVideoInfo, pvp = *pvpp; pvp; pvp = *(++pvpp)) {
	    resPtr *res;

	    if (PCIINFOCLASSES(pvp->class, pvp->subclass))
		res = nonsysRes;
	    else
		res = sysRes;
	    for (i = 0; i < 6; i++) {
		if (pvp->ioBase[i]) {
		    RANGE(range, pvp->ioBase[i],
			  pvp->ioBase[i] + (1 << pvp->size[i]) - 1,
			  ResExcIoBlock | ResBios);
		    *res = xf86AddResToList(*res, &range, -1);
		} else if (pvp->memBase[i]) {
		    RANGE(range, pvp->memBase[i],
			  pvp->memBase[i] + (1 << pvp->size[i]) - 1,
			  ResExcMemBlock | ResBios);
		    *res = xf86AddResToList(*res, &range, -1);
		}
	    }
	    if (pvp->biosBase) {
		RANGE(range, pvp->biosBase,
		      pvp->biosBase + (1 << pvp->biosSize) - 1,
		      ResExcMemBlock | ResBios);
		*res = xf86AddResToList(*res, &range, -1);
	    }
	}
    
    /* XXX Needs to be updated for 64 bit mappings */
    for (pcrpp = xf86PciInfo, pcrp = *pcrpp; pcrp; pcrp = *++(pcrpp)) {
	if (PCIINFOCLASSES(pcrp->pci_base_class, pcrp->pci_sub_class))
	    continue;
	/* Only process devices with type 0 headers */
	if ((pcrp->pci_header_type & 0x7f) != 0)
	    continue;

	basep = &pcrp->pci_base0;
	for (i = 0; i < 6; i++) {
	    if (basep[i]) {
		if (PCI_MAP_IS_IO(basep[i])) {
		    RANGE(range,PCIGETIO(basep[i]),
			  range.rBegin + (1 << pcrp->basesize[i]) - 1,
			  ResExcIoBlock | ResEstimated);
		    *sysRes = xf86AddResToList(*sysRes, &range, -1);
		} else {
		    RANGE(range,PCIGETMEMORY(basep[i]),
			  range.rBegin + (1 << pcrp->basesize[i]) - 1,
			  ResExcMemBlock | ResEstimated);
		    *sysRes = xf86AddResToList(*sysRes, &range, -1);
		}
	    }
	}
	if (pcrp->pci_baserom) {
	    RANGE(range,PCIGETROM(pcrp->pci_baserom),
		  range.rBegin	+ (1 << pcrp->basesize[6]) - 1,
		  ResExcMemBlock | ResEstimated);
	    *sysRes = xf86AddResToList(*sysRes, &range, -1);
	}
    }

    if (*nonsysRes) {
	xf86MsgVerb(X_INFO, 3, "Non-system PCI resource ranges:\n");
	xf86PrintResList(3, *nonsysRes);
    }

    /*
     * Adjust ranges based on the assumption that there are no real
     * overlaps in the PCI base allocations.  This assumption should be
     * reasonable in most cases.  It may be possible to refine the
     * approximated PCI base sizes by considering bus mapping information
     * from PCI-PCI bridges.
     */
    if (*sysRes) {
	xf86MsgVerb(X_INFO, 3, "System PCI resource ranges:\n");
	xf86PrintResList(3, *sysRes);

	/* Check for overlaps */
	for (pRes = *sysRes; pRes; pRes = pRes->next) {
	    if (ResIsEstimated(&pRes->val)) {
		range = pRes->val;

		RemoveOverlaps(pRes, *nonsysRes, TRUE);
		RemoveOverlaps(pRes, *sysRes, TRUE);

		if (range.rEnd > pRes->block_end)
		    xf86MsgVerb(X_INFO, 3,
			"PCI %s resource overlap reduced 0x%08x"
			" from 0x%08x to 0x%08x\n",
			(pRes->res_type & ResMem) ? "Memory" : "I/O",
			range.rBegin, range.rEnd, pRes->block_end);
	    }
	}
	xf86MsgVerb(X_INFO, 3,
	    "System PCI resource ranges after removing overlaps:\n");
	xf86PrintResList(3, *sysRes);
    }
}

static Bool fixPciResource(int prt, memType alignment,
			   pciVideoPtr pvp, long type);

static void
ValidatePci(void)
{
    pciVideoPtr pvp, pvp1;
    PciBusPtr pbp, pbp1;
    resPtr res_mp = NULL, res_m_io = NULL;
    resPtr tmp, avoid;
    resPtr Sys;
    resPtr Fix;
    resRange range;
    int n = 0, m, i;

    if (!xf86PciVideoInfo) return;
    
    /*
     * The order the video devices are listed in is
     * just right: the lower buses come first.
     * This way we attempt to fix a conflict of
     * a lower bus device with a higher bus device
     * where we have more room to find different
     * resources.
     */
    while ((pvp = xf86PciVideoInfo[n++])) {
	if (!pvp->validate) continue;
	Sys = NULL;
	m = n;
	while ((pvp1 = xf86PciVideoInfo[m++])) {
	    if (!pvp1->validate) continue;
	    for (i = 0; i<6; i++) {
		if (pvp1->ioBase[i]) {
		    RANGE(range,pvp1->ioBase[i],
			  pvp1->ioBase[i] + (1 << pvp1->size[i]) - 1,
			  ResExcIoBlock);
		    Sys = xf86AddResToList(Sys,&range,-1);
		} else if (pvp1->memBase[i]) {
		    RANGE(range,pvp1->memBase[i],
			  pvp1->memBase[i] + (1 << pvp1->size[i]) - 1,
			  ResExcMemBlock);
		    Sys = xf86AddResToList(Sys,&range,-1);
		}
	    }
	}
#ifdef DEBUG
	xf86MsgVerb(X_INFO, 3,"Sys:\n");
	xf86PrintResList(3,Sys);
#endif
	avoid = NULL;
	pbp = pbp1 = xf86PciBus;
	while (pbp) {
	    if (pbp->secondary == pvp->bus) {
		if (pbp->pmem) {
		    /* keep prefetchable separate */
		    res_mp = findIntersectOfLists(pbp->pmem,ResRange);
		}
		if (pbp->mem) {
		    res_m_io = findIntersectOfLists(pbp->mem,ResRange);
		}
		if (pbp->io) {
		    res_m_io = xf86JoinResLists(res_m_io,
			findIntersectOfLists(pbp->io,ResRange));
		}
	    }
	    while (pbp1) {
		if (pbp1->primary == pvp->bus) {
		    tmp = xf86DupResList(pbp1->pmem);
		    avoid = xf86JoinResLists(avoid,tmp);
		    tmp = xf86DupResList(pbp1->mem);
		    avoid = xf86JoinResLists(avoid,tmp);
		    tmp = xf86DupResList(pbp1->io);
		    avoid = xf86JoinResLists(avoid,tmp);
		}
		pbp1 = pbp1->next;
	    }	
	    pbp = pbp->next;
	}
	if (res_m_io == NULL)
	   res_m_io = xf86DupResList(ResRange);
#ifdef DEBUG
	xf86MsgVerb(X_INFO, 3,"avoid:\n");
	xf86PrintResList(3,avoid);
	xf86MsgVerb(X_INFO, 3,"prefetchable Memory:\n");
	xf86PrintResList(3,res_mp);
	xf86MsgVerb(X_INFO, 3,"MEM/IO:\n");
	xf86PrintResList(3,res_m_io);
#endif
	Fix = NULL;
	for (i = 0; i < 6; i++) {
	    int j;
	    resPtr own = NULL;
	    for (j = i+1; j < 6; j++) {
		if (pvp->ioBase[j]) {
		    RANGE(range,pvp->ioBase[j],
			  pvp->ioBase[j] + (1 << pvp->size[j]) - 1,
			  ResExcIoBlock);
		    own = xf86AddResToList(own,&range,-1);
		} else if (pvp->memBase[j]) {
		    RANGE(range,pvp->memBase[j],
			  pvp->memBase[j] + (1 << pvp->size[j]) - 1,
			  ResExcMemBlock);
		    own = xf86AddResToList(own,&range,-1);
		}
	    }
#ifdef DEBUG
	xf86MsgVerb(X_INFO, 3,"own:\n");
	xf86PrintResList(3,own);
#endif
	    if (pvp->ioBase[i]) {
		RANGE(range,pvp->ioBase[i],
		      pvp->ioBase[i] + (1 << pvp->size[i]) - 1,
		      ResExcIoBlock);
		if (isSubsetOf(range,res_m_io)
		    && ! ChkConflict(&range,own,SETUP)
		    && ! ChkConflict(&range,avoid,SETUP)
		    && ! ChkConflict(&range,Sys,SETUP))
		    continue;
		xf86MsgVerb(X_WARNING, 0,
			"****INVALID IO ALLOCATION**** b: 0x%lx e: 0x%lx "
			"correcting\a\n", range.rBegin,range.rEnd);
#ifdef DEBUG
		sleep(2);
#endif
		fixPciResource(i, 0, pvp, range.type);
	    } else if (pvp->memBase[i]) {
		RANGE(range,pvp->memBase[i],
		      pvp->memBase[i] + (1 << pvp->size[i]) - 1,
		      ResExcMemBlock);
		if (pvp->type[i] & PCI_MAP_MEMORY_CACHABLE) {
		    if (isSubsetOf(range,res_mp)
			&& ! ChkConflict(&range,own,SETUP)
			&& ! ChkConflict(&range,avoid,SETUP)
			&& ! ChkConflict(&range,Sys,SETUP))
			continue;
		}
		if (isSubsetOf(range,res_m_io)
		    && ! ChkConflict(&range,own,SETUP)
		    && ! ChkConflict(&range,avoid,SETUP)
		    && ! ChkConflict(&range,Sys,SETUP))
		    continue;
		xf86MsgVerb(X_WARNING, 0,
			"****INVALID MEM ALLOCATION**** b: 0x%lx e: 0x%lx "
			"correcting\a\n", range.rBegin,range.rEnd);
#ifdef DEBUG
		sleep(2);
#endif
		fixPciResource(i, 0, pvp, range.type);
	    }
	    xf86FreeResList(own);
	}
	xf86FreeResList(avoid);
	xf86FreeResList(Sys);
	xf86FreeResList(res_mp);
	xf86FreeResList(res_m_io);
    }
    return;
}

    
static resList
GetImplicitPciResources(int entityIndex)
{
    pciVideoPtr pvp;
    int i;
    resList list = NULL;

    int num = 0;
    
    if (! (pvp = xf86GetPciInfoForEntity(entityIndex))) return NULL;

    for (i = 0; i < 6; i++) {
	if (pvp->ioBase[i]) {
	    list = xnfrealloc(list,sizeof(resRange) * (++num));
	    RANGE(list[num - 1],pvp->ioBase[i],
		  pvp->ioBase[i] + (1 << pvp->size[i]) - 1,
		  ResShrIoBlock | ResBios);
	} else if (pvp->memBase[i]) {
	    list = xnfrealloc(list,sizeof(resRange) * (++num));
	    RANGE(list[num - 1],pvp->memBase[i],
		  pvp->memBase[i] + (1 << pvp->size[i]) - 1,
		  ResShrMemBlock | ResBios);
	}
    }
    if (pvp->biosBase) {
	list = xnfrealloc(list,sizeof(resRange) * (++num));
	RANGE(list[num - 1], pvp->biosBase,
	      pvp->biosBase + (1 << pvp->biosSize) - 1,
	      ResShrMemBlock | ResBios);
    }
    list = xnfrealloc(list,sizeof(resRange) * (++num));
    list[num - 1].type = ResEnd;
    
    return list;
}

void
xf86ResourceBrokerInit(void)
{
    resPtr sysRes, nonsysRes;
    resPtr osRes = NULL, tmp;

    /* Get the addressable ranges */
    ResRange = xf86AccWindowsFromOS();
    xf86MsgVerb(X_INFO, 3, "Addressable resource ranges are\n");
    xf86PrintResList(3, ResRange);

    /* Get the ranges used exclusively by the system */
    osRes = xf86AccResFromOS(osRes);

    /* Get bus-specific system resources (PCI) */
    xf86GetPciRes(&sysRes, &nonsysRes);

    /*
     * Adjust OS-reported resource ranges based on the assumption that there
     * are no overlaps with the PCI base allocations.  This should be a good
     * assumption because writes to PCI address space won't be routed directly
     * host memory.
     */
    xf86MsgVerb(X_INFO, 3, "OS-reported resource ranges:\n");
    xf86PrintResList(3, osRes);

    for (tmp = osRes; tmp; tmp = tmp->next) {
	RemoveOverlaps(tmp, sysRes, FALSE);
	RemoveOverlaps(tmp, nonsysRes, FALSE);
    }

    xf86MsgVerb(X_INFO, 3, "OS-reported resource ranges after removing"
		" overlaps with PCI:\n");
    xf86PrintResList(3, osRes);

    Acc = xf86JoinResLists(sysRes, osRes);
    xf86MsgVerb(X_INFO, 3, "All system resource ranges:\n");
    xf86PrintResList(3, Acc);

    /* Initialise the OS-layer PCI allocator */
    xf86PciBus = xf86InitOSPciAllocator(xf86PciInfo, &Acc, nonsysRes);

    xf86MsgVerb(X_INFO, 3, "Resource ranges after broker init:\n");
    xf86PrintResList(3, Acc);
}

/*
 * ProcessEstimatedConflicts() -- Do something about driver-registered
 * resources that conflict with estimated resources.  For now, just register
 * them with a logged warning.
 */
static void
ProcessEstimatedConflicts(void)
{
    if (!AccReducers)
	return;

    /* Temporary */
    xf86MsgVerb(X_WARNING, 3,
		"Registering the following despite conflicts with estimated"
		" resources:\n");
    xf86PrintResList(3, AccReducers);
    Acc = xf86JoinResLists(Acc, AccReducers);
    AccReducers = NULL;
}

/*
 * xf86ClaimFixedResources() -- This function gets called from the
 * driver Probe() function to claim fixed resources.
 */
static void
resError(resList list)
{
    FatalError("A driver tried to allocate the %s %sresource at \n"
	       "0x%x:0x%x which conflicted with another resource.  Send the\n"
	       "output of the server to xfree86@xfree86.org.  Please \n"
	       "specify your computer hardware as closely as possible.\n",
	       ResIsBlock(list)?"Block":"Sparse",
	       ResIsMem(list)?"Mem":"Io",
	       ResIsBlock(list)?list->rBegin:list->rBase,
	       ResIsBlock(list)?list->rEnd:list->rMask);
}

/*
 * xf86ClaimFixedResources() is used to allocate non-relocatable resources.
 * This should only be done by a driver's Probe() function.
 */
void
xf86ClaimFixedResources(resList list, int entityIndex)
{
    resPtr ptr = NULL;
    resRange range;
	
    if (!list) return;
    
    while (list->type !=ResEnd) {
	range = *list;
	range.type &= ~ResEstimated;	/* Not allowed for drivers */
	switch (range.type & ResAccMask) {
	case ResExclusive:
	    if (!xf86ChkConflict(&range, entityIndex)) {
		range.type &= ~ResBios;
		Acc = xf86AddResToList(Acc, &range, entityIndex);
	    } else {
		range.type |= ResEstimated;
		if (!xf86ChkConflict(&range, entityIndex) &&
		    !checkConflict(&range, AccReducers, entityIndex, SETUP)) {
		    range.type &= ~(ResEstimated | ResBios);
		    AccReducers =
			xf86AddResToList(AccReducers, &range, entityIndex);
		} else
		    resError(&range);	/* No return */
	    }
	    break;
	case ResShared:
	    /* at this stage the resources are just added to the
	     * EntityRec. After the Probe() phase this list is checked by
	     * xf86PostProbe(). All resources which don't
	     * conflict with already allocated ones are allocated
	     * and removed from the EntityRec. Thus a non-empty resource
	     * list in the EntityRec indicates resource conflicts the
	     * driver should either handle or fail.
	     */
	    if (xf86Entities[entityIndex]->active)
		ptr = xf86AddResToList(ptr, &range, entityIndex);
	    break;
	}
	list++;
    }
    xf86Entities[entityIndex]->resources =
	xf86JoinResLists(xf86Entities[entityIndex]->resources,ptr);
    xf86MsgVerb(X_INFO, 3,
	"resource ranges after xf86ClaimFixedResources() call:\n");
    xf86PrintResList(3,Acc);
    ProcessEstimatedConflicts();
#ifdef DEBUG
    if (ptr) {
	xf86MsgVerb(X_INFO, 3, "to be registered later:\n");
	xf86PrintResList(3,ptr);
    }
#endif
}

static void
checkRoutingForScreens(xf86State state)
{
    resList list = resVgaUnusedExclusive;
    resPtr pResVGA = NULL;
    pointer vga = NULL;
    int i,j;
    int entityIndex;
    EntityPtr pEnt;
    resPtr pAcc;
    resRange range;

    /*
     * find devices that need VGA routed: ie the ones that have
     * registered VGA resources without ResUnused. ResUnused
     * doesn't conflict with itself therefore use it here.
     */
    while (list->type != ResEnd) {
	range = *list;
	range.type &= ~(ResBios | ResEstimated);
	pResVGA = xf86AddResToList(pResVGA, &range, -1);
	list++;
    }

    for (i = 0; i < xf86NumScreens; i++) {
	for (j = 0; j < xf86Screens[i]->numEntities; j++) {
	    entityIndex = xf86Screens[i]->entityList[j];
	    pEnt = xf86Entities[entityIndex];
	    pAcc = Acc;
	    vga = NULL;
	    while (pAcc) {
		if (pAcc->entityIndex == entityIndex)
		    if (checkConflict(&pAcc->val,pResVGA,
				      entityIndex,state)) {
			if (vga && vga != pEnt->busAcc) {
			    xf86Msg(X_ERROR, "Screen %i needs vga routed to"
				    "different buses - deleting\n",i);
			    xf86DeleteScreen(i--,0);
			}
			vga = pEnt->busAcc;
			pEnt->entityProp |= (state == SETUP
			    ? NEED_VGA_ROUTED_SETUP : NEED_VGA_ROUTED);
			break;
		    }
		pAcc = pAcc->next;
	    }
	    if (vga)
		xf86MsgVerb(X_INFO, 3,"Setting vga for screen %i.\n",i);
	}
    }
    xf86FreeResList(pResVGA);
}


/*
 * xf86PostProbe() -- Allocate all non conflicting resources
 * This function gets called by xf86Init().
 */
void
xf86PostProbe(void)
{
    memType val;
    int i,j;
    resPtr resp, acc, tmp, resp_x, *pprev_next;

    xf86SetPciVideo(NULL, MEM_IO);

    /* don't compare against ResInit - remove it from clone.*/
    acc = tmp = xf86DupResList(Acc);
    pprev_next = &acc;
    while (tmp) {
	if (tmp->res_type & ResInit) {
	    (*pprev_next) = tmp->next;
	    xfree(tmp);
	} else 
	    pprev_next = &(tmp->next);
	tmp = (*pprev_next);
    }

    for (i=0; i<xf86NumEntities; i++) {
	resp = xf86Entities[i]->resources;
	xf86Entities[i]->resources = NULL;
	resp_x = NULL;
	while (resp) {
	    if (!(val = checkConflict(&resp->val, acc, i, SETUP))) {
		resp->res_type &= ~ResBios;
		tmp = resp_x;
		resp_x = resp;
		resp = resp->next;
		resp_x->next = tmp;
	    } else {
		resp->res_type |= ResEstimated;
		if (!checkConflict(&resp->val, acc, i, SETUP)) {
		    resp->res_type &= ~(ResEstimated | ResBios);
		    tmp = AccReducers;
		    AccReducers = resp;
		    resp = resp->next;
		    AccReducers->next = tmp;
		} else {
		    xf86MsgVerb(X_INFO, 3, "Found conflict at: 0x%lx\n",val);
		    resp->res_type &= ~ResEstimated;
		    tmp = xf86Entities[i]->resources;
		    xf86Entities[i]->resources = resp;
		    resp = resp->next;
		    xf86Entities[i]->resources->next = tmp;
		}
	    }
	}
	xf86JoinResLists(Acc,resp_x);
	ProcessEstimatedConflicts();
    }
    xf86FreeResList(acc);

    ValidatePci();
    
    xf86MsgVerb(X_INFO, 3, "resource ranges after probing:\n");
    xf86PrintResList(3, Acc);
    checkRoutingForScreens(SETUP);

    for (i = 0; i < xf86NumScreens; i++) {
	for (j = 0; j<xf86Screens[i]->numEntities; j++) {
	    EntityPtr pEnt = xf86Entities[xf86Screens[i]->entityList[j]];
	    if ((pEnt->entityProp & NEED_VGA_ROUTED_SETUP) &&
		((xf86Screens[i]->busAccess = pEnt->busAcc)))
		break;
	}
    }
}

void
xf86AddEntityToScreen(ScrnInfoPtr pScrn, int entityIndex)
{
    if (entityIndex == -1)
	return;
    if (xf86Entities[entityIndex]->inUse)
	FatalError("Requested Entity already in use!\n");

    pScrn->numEntities++;
    pScrn->entityList = xnfrealloc(pScrn->entityList,
				    pScrn->numEntities * sizeof(int));
    pScrn->entityList[pScrn->numEntities - 1] = entityIndex;
    xf86Entities[entityIndex]->access->next = pScrn->access;
    pScrn->access = xf86Entities[entityIndex]->access;
    xf86Entities[entityIndex]->inUse = TRUE;
}

Bool
xf86SetEntityFuncs(int entityIndex, EntityProc init, EntityProc enter,
		   EntityProc leave, pointer private)
{
    if (entityIndex >= xf86NumEntities)
	return FALSE;
    xf86Entities[entityIndex]->entityInit = init;
    xf86Entities[entityIndex]->entityEnter = enter;
    xf86Entities[entityIndex]->entityLeave = leave;
    xf86Entities[entityIndex]->private = private;
    return TRUE;
}

void
xf86RemoveEntityFromScreen(ScrnInfoPtr pScrn, int entityIndex)
{
    int i;
    EntityAccessPtr *ptr = (EntityAccessPtr *)&pScrn->access;
    EntityAccessPtr peacc;
    
    for (i = 0; i < pScrn->numEntities; i++) {
	if (pScrn->entityList[i] == entityIndex) {
	    peacc = xf86Entities[pScrn->entityList[i]]->access;
	    (*ptr) = peacc->next;
	    /* disable entity: call disable func */
	    if (peacc->pAccess && peacc->pAccess->AccessDisable)
		peacc->pAccess->AccessDisable(peacc->pAccess->arg);
	    /* also disable fallback - just in case */
	    if (peacc->fallback && peacc->fallback->AccessDisable)
		peacc->fallback->AccessDisable(peacc->fallback->arg);
	    for (i++; i < pScrn->numEntities; i++)
		pScrn->entityList[i-1] = pScrn->entityList[i];
	    pScrn->numEntities--;
	    xf86Entities[entityIndex]->inUse = FALSE;
	    break;
	}
	ptr = &(xf86Entities[pScrn->entityList[i]]->access->next);
    }
}

void
xf86DeallocateResourcesForEntity(int entityIndex, long type)
{
    resPtr *pprev_next = &Acc;
    resPtr res = Acc;

    while (res) {
	if (res->entityIndex == entityIndex &&
	    (type & ResAccMask & res->res_type))
	{
	    (*pprev_next) = res->next;
	    xfree(res);
	} else 
	    pprev_next = &(res->next);
	res = (*pprev_next);
    }
}

/*
 * xf86ClearEntitiesForScreen() - called when a screen is deleted
 * to mark it's entities unused. Called by xf86DeleteScreen().
 */
void
xf86ClearEntityListForScreen(int scrnIndex)
{
    int i;
    EntityAccessPtr peacc;
    
    if (xf86Screens[scrnIndex]->entityList == NULL
	|| xf86Screens[scrnIndex]->numEntities == 0) return;
	
    for (i=0; i<xf86Screens[scrnIndex]->numEntities; i++) {
	xf86Entities[xf86Screens[scrnIndex]->entityList[i]]->inUse = FALSE;
	/* disable resource: call the disable function */
	peacc = xf86Entities[xf86Screens[scrnIndex]->entityList[i]]->access;
	if (peacc->pAccess && peacc->pAccess->AccessDisable)
	    peacc->pAccess->AccessDisable(peacc->pAccess->arg);
	/* and the fallback function */
	if (peacc->fallback && peacc->fallback->AccessDisable)
	    peacc->fallback->AccessDisable(peacc->fallback->arg);
	/* shared resources are only needed when entity is active: remove */
	xf86DeallocateResourcesForEntity(i, ResShared);
    }
    xfree(xf86Screens[scrnIndex]->entityList);
    if (xf86Screens[scrnIndex]->CurrentAccess->pIoAccess
	== (EntityAccessPtr) xf86Screens[scrnIndex]->access)
	xf86Screens[scrnIndex]->CurrentAccess->pIoAccess = NULL;
    if (xf86Screens[scrnIndex]->CurrentAccess->pMemAccess
	== (EntityAccessPtr) xf86Screens[scrnIndex]->access)
	xf86Screens[scrnIndex]->CurrentAccess->pMemAccess = NULL;
    xf86Screens[scrnIndex]->entityList = NULL;
}

/*
 * xf86GetEntityInfo() -- This function hands information from the
 * EntityRec struct to the drivers. The EntityRec structure itself
 * remains invisible to the driver.
 */
EntityInfoPtr
xf86GetEntityInfo(int entityIndex)
{
    EntityInfoPtr pEnt;
    
    if (entityIndex >= xf86NumEntities)
	return NULL;
    
    pEnt = xnfcalloc(1,sizeof(EntityInfoRec));
    pEnt->index = entityIndex;
    pEnt->location = xf86Entities[entityIndex]->bus;
    pEnt->active = xf86Entities[entityIndex]->active;
    pEnt->chipset = xf86Entities[entityIndex]->chipset;
    pEnt->resources = xf86Entities[entityIndex]->resources;
    pEnt->device = xf86Entities[entityIndex]->device;
    
    return pEnt;
}

/*
 * xf86GetPciInfoForEntity() -- Get the pciVideoRec of entity.
 */
pciVideoPtr
xf86GetPciInfoForEntity(int entityIndex)
{
    pciVideoPtr *ppPci;
    EntityPtr p = xf86Entities[entityIndex];
    
    if (entityIndex >= xf86NumEntities
	|| p->busType != BUS_PCI) return NULL;
    
    for (ppPci = xf86PciVideoInfo; *ppPci != NULL; ppPci++) {
	if (p->pciBusId.bus == (*ppPci)->bus &&
	    p->pciBusId.device == (*ppPci)->device &&
	    p->pciBusId.func == (*ppPci)->func) 
	    return (*ppPci);
    }
    return NULL;
}

ScrnInfoPtr
xf86FindScreenForEntity(int entityIndex)
{
    int i,j;

    if (entityIndex == -1) return NULL;
    
    if (xf86Screens) {
	for (i = 0; i < xf86NumScreens; i++) {
	    for (j = 0; j < xf86Screens[i]->numEntities; j++) {
		if ( xf86Screens[i]->entityList[j] == entityIndex )
		    return (xf86Screens[i]);
	    }
	}
    }
    return NULL;
}

#define ALIGN(x,a) ((x) + a) &~(a)

resRange 
xf86GetBlock(long type, memType size,
	 memType window_start, memType window_end,
	 memType align_mask, resPtr avoid)
{
    memType min, max, tmp;
    resRange r = {ResEnd,0,0};
    resPtr res_range = ResRange;
    
    if (!size) return r;
    if (window_end < window_start || (window_end - window_start) < (size - 1)) {
	ErrorF("Requesting insufficient memory window!:"
	       " start: 0x%lx end: 0x%lx size 0x%lx\n",
	       window_start,window_end,size);
	return r;
    }
    type = (type & ~(ResExtMask | ResNoAvoid | ResEstimated)) | ResBlock;
    
    while (res_range) {
	if (type & res_range->res_type & ResPhysMask) {
	    if (res_range->block_begin > window_start)
		min = res_range->block_begin;
	    else
		min = window_start;
	    if (res_range->block_end < window_end)
		max = res_range->block_end;
	    else
		max = window_end;
	    min = ALIGN(min,align_mask);
	    /* do not produce an overflow! */
	    while (min < max && (max - min) >= (size - 1)) {
		RANGE(r,min,min + size - 1,type);
		tmp = ChkConflict(&r,Acc,SETUP);
		if (!tmp) {
		    tmp = ChkConflict(&r,avoid,SETUP);
		    if (!tmp) {
			return r;
		    } 
		}
		min = ALIGN(tmp,align_mask);
	    }
	}
	res_range = res_range->next;
    }
    RANGE(r,0,0,ResEnd);
    return r;
}

#define mt_max ~(memType)0
#define length sizeof(memType) * 8
/*
 * make_base() -- assign the lowest bits to the bits set in mask.
 *                example: mask 011010 val 0000110 -> 011000 
 */
static memType
make_base(memType val, memType mask)
{
    int i,j = 0;
    memType ret = 0
	;
    for (i = 0;i<length;i++) {
	if ((1 << i) & mask) {
	    ret |= (((val >> j) & 1) << i);
	    j++;
	}
    }
    return ret;
}

/*
 * make_base() -- assign the bits set in mask to the lowest bits.
 *                example: mask 011010 , val 010010 -> 000011
 */
static memType
unmake_base(memType val, memType mask)
{
    int i,j = 0;
    memType ret = 0;
    
    for (i = 0;i<length;i++) {
	if ((1 << i) & mask) {
	    ret |= (((val >> i) & 1) << j);
	    j++;
	}
    }
    return ret;
}

static memType
fix_counter(memType val, memType old_mask, memType mask)
{
    mask = old_mask & mask;
    
    val = make_base(val,old_mask);
    return unmake_base(val,mask);
}

resRange
xf86GetSparse(long type,  memType fixed_bits,
	  memType decode_mask, memType address_mask, resPtr avoid)
{
    resRange r = {ResEnd,0,0};
    memType new_mask;
    memType mask1;
    memType base;
    memType bits;
    memType counter = 0;
    memType counter1;
    memType max_counter = ~(memType)0;
    memType max_counter1;
    memType conflict = 0;
    
    /* for sanity */
    type = (type & ~(ResExtMask | ResNoAvoid | ResEstimated)) | ResSparse;

    /*
     * a sparse address consists of 3 parts:
     * fixed_bits:   F bits which hard decoded by the hardware
     * decode_bits:  D bits which are used to decode address
     *                 but which may be set by software
     * address_bits: A bits which are used to address the
     *                 sparse range.
     * the decode_mask marks all decode bits while the address_mask
     * masks out all address_bits:
     *                F D A
     * decode_mask:   0 1 0
     * address_mask:  1 1 0
     */
    decode_mask &= address_mask;
    new_mask = decode_mask;

    /*
     * We start by setting the decode_mask bits to different values
     * when a conflict is found the address_mask of the conflicting
     * resource is returned. We remove those bits from decode_mask
     * that are also set in the returned address_mask as they always
     * conflict with resources which use them as address masks.
     * The resoulting mask is stored in new_mask.
     * We continue until no conflict is found or until we have
     * tried all possible settings of new_mask.
     */
    while (1) {
	base = make_base(counter,new_mask) | fixed_bits;
	RANGE(r,base,address_mask,type);
	conflict = ChkConflict(&r,Acc,SETUP);
	if (!conflict) {
	    conflict = ChkConflict(&r,avoid,SETUP);
	    if (!conflict) {
		return r;
	    }
	}
	counter = fix_counter(counter,new_mask,conflict);
	max_counter = fix_counter(max_counter,new_mask,conflict);
	new_mask &= conflict;
	counter ++;
	if (counter > max_counter) break;
    }
    if (!new_mask && (new_mask == decode_mask)) {
	RANGE(r,0,0,ResEnd);
	return r;
    }
    /*
     * if we haven't been successful we also try to modify those
     * bits in decode_mask that are not at the same time set in
     * new mask. These bits overlap with address_bits of some
     * resources. If a conflict with a resource of this kind is
     * found (ie. returned_mask & mask1 != mask1) with
     * mask1 = decode_mask & ~new_mask we cannot
     * use our choice of bits in the new_mask part. We try
     * another choice.
     */
    max_counter = fix_counter(mt_max,mt_max,new_mask);
    mask1 = decode_mask & ~new_mask;
    max_counter1 = fix_counter(mt_max,mt_max,mask1);
    counter = 0;
    
    while (1) {
	bits = make_base(counter,new_mask) | fixed_bits; 
	counter1 = 0;
	while (1) {
	    base = make_base(counter1,mask1);
	    RANGE(r,base,address_mask,type);
	    conflict = ChkConflict(&r,Acc,SETUP);
	    if (!conflict) {
		conflict = ChkConflict(&r,avoid,SETUP);
		if (!conflict) {
		    return r;
		}
	    }
	    counter1 ++;
	    if ((mask1 & conflict) != mask1 || counter1 > max_counter1)
		break;
	}
	counter ++;
	if (counter > max_counter) break;
    }
    RANGE(r,0,0,ResEnd);
    return r;
}

#undef length
#undef mt_max

static resList
xf86GetResourcesImplicitly(int entityIndex)
{
    if (entityIndex >= xf86NumEntities) return NULL;
    
    switch (xf86Entities[entityIndex]->bus.type) {
    case BUS_ISA:
    case BUS_NONE:
	return NULL;
    case BUS_PCI:
	return GetImplicitPciResources(entityIndex);
    }
    return NULL;
}

/*
 * xf86RegisterResources() -- attempts to register listed resources.
 * If list is NULL it tries to obtain resources implicitly. Function
 * returns a resPtr listing all resources not successfully registered.
 */
resPtr
xf86RegisterResources(int entityIndex, resList list, int access)
{
    resPtr res = NULL;
    resRange range;
    
    if (!list) {
	list = xf86GetResourcesImplicitly(entityIndex);
	if (!list) return NULL;
    }
    while(list->type != ResEnd) {
	range = *list;
	if ((access != ResNone) && (access & ResAccMask)) {
	    range.type = (range.type & ~ResAccMask) | (access & ResAccMask);
	}
	range.type &= ~ResEstimated;	/* Not allowed for drivers */
	if(xf86ChkConflict(&range,entityIndex)) 
	    res = xf86AddResToList(res,&range,entityIndex);
	else {
	    range.type &= ~ResBios;
	    Acc = xf86AddResToList(Acc,&range,entityIndex);
	}
	list++;
    }
#ifdef DEBUG
    xf86MsgVerb(X_INFO, 3,"Resources after driver initialization\n");
    xf86PrintResList(3, Acc);
    if (res) xf86MsgVerb(X_INFO, 3,
			 "Failed Resources after driver initialization\n");
    xf86PrintResList(3, res);
#endif
    return res;
    
}

static void
checkRequiredResources(int entityIndex)
{
    resRange range;
    resPtr pAcc = Acc;
    const EntityPtr pEnt = xf86Entities[entityIndex];
    while (pAcc) {
	if (pAcc->entityIndex == entityIndex) {
	    range = pAcc->val;
	    /*  ResAny to find conflicts with anything. */
	    range.type = (range.type & ~ResAccMask) | ResAny | ResBios;
	    if (checkConflict(&range,Acc,entityIndex,OPERATING))
		switch (pAcc->res_type & ResPhysMask) {
		case ResMem:
		    pEnt->entityProp |= NEED_MEM_SHARED;
		    break;
		case ResIo:
		    pEnt->entityProp |= NEED_IO_SHARED;
		    break;
		}
	    if (!(pAcc->res_type & ResOprMask)) {
		switch (pAcc->res_type & ResPhysMask) {
		case ResMem:
		    pEnt->entityProp |= NEED_MEM;
		    break;
		case ResIo:
		    pEnt->entityProp |= NEED_IO;
		    break;
		}
	    }
	}
	pAcc = pAcc->next;
    }
    
    /* check if we can separately enable mem/io resources */
    /* XXX we still need to find out how to set this yet  */
    if ( ((pEnt->entityProp & NO_SEPARATE_MEM_FROM_IO)
	  && (pEnt->entityProp & NEED_MEM_SHARED))
	 || ((pEnt->entityProp & NO_SEPARATE_IO_FROM_MEM)
	     && (pEnt->entityProp & NEED_IO_SHARED)) )
	pEnt->entityProp |= NEED_SHARED;
    /*
     * After we have checked all resources of an entity agains any
     * other resource we know if the entity need this resource type
     * (ie. mem/io) at all. if not we can disable this type completely,
     * so no need to share it either. 
     */
    if ((pEnt->entityProp & NEED_MEM_SHARED)
	&& (!(pEnt->entityProp & NEED_MEM))
	&& (!(pEnt->entityProp & NO_SEPARATE_MEM_FROM_IO)))
	pEnt->entityProp &= ~(unsigned long)NEED_MEM_SHARED;

    if ((pEnt->entityProp & NEED_IO_SHARED)
	&& (!(pEnt->entityProp & NEED_IO))
	&& (!(pEnt->entityProp & NO_SEPARATE_IO_FROM_MEM)))
	pEnt->entityProp &= ~(unsigned long)NEED_IO_SHARED;
}

static xf86AccessPtr
busTypeSpecific(EntityPtr pEnt, xf86State state)
{
    pciAccPtr *ppaccp;

    switch (pEnt->bus.type) {
    case BUS_ISA:
	switch (state) {
	case SETUP:
	    return &AccessNULL;
	    break;
	case OPERATING:
	    if (pEnt->entityProp & NEED_SHARED)
		return &AccessNULL;
	    else  /* no conflicts at all */
		return NULL; /* remove from RAC */
	    break;
	}
	break;
    case BUS_PCI:
	ppaccp = xf86PciAccInfo;
	while (*ppaccp) {
	    if ((*ppaccp)->busnum == pEnt->pciBusId.bus
		&& (*ppaccp)->devnum == pEnt->pciBusId.device
		&& (*ppaccp)->funcnum == pEnt->pciBusId.func) {
		switch (state) {
		case SETUP:
		    (*ppaccp)->io_memAccess.AccessDisable((*ppaccp)->
							  io_memAccess.arg);
		    return &(*ppaccp)->io_memAccess;
		    break;
		case OPERATING:
		    if (!(pEnt->entityProp & NEED_MEM_SHARED)){
			if (pEnt->entityProp & NEED_MEM)
			    (*ppaccp)->memAccess.AccessEnable((*ppaccp)->
							      memAccess.arg);
			else 
			    (*ppaccp)->memAccess.AccessDisable((*ppaccp)->memAccess.arg);
		    }
		    if (!(pEnt->entityProp & NEED_IO_SHARED)) {
			if (pEnt->entityProp & NEED_IO)
			    (*ppaccp)->ioAccess.AccessEnable((*ppaccp)->
							     ioAccess.arg);
			else 
			    (*ppaccp)->ioAccess.AccessDisable((*ppaccp)->
							      ioAccess.arg);
		    }
		    switch(pEnt->entityProp & NEED_SHARED) {
		    case NEED_IO_SHARED:
			return &(*ppaccp)->ioAccess;
		    case NEED_MEM_SHARED:
			return &(*ppaccp)->memAccess;
		    case NEED_SHARED:
			return &(*ppaccp)->io_memAccess;
		    default: /* no conflicts at all */
			return NULL; /* remove from RAC */
		    }
		    break;
		}
	    }
	    ppaccp++;
	}
	break;
    default:
	return NULL;
    }
    return NULL;
}

/*
 * setAccess() -- sets access functions according to resources
 * required. 
 */
static void
setAccess(EntityPtr pEnt, xf86State state)
{
    xf86AccessPtr new = NULL;
    
    /* set access funcs and access state according to resource requirements */
    pEnt->access->pAccess = busTypeSpecific(pEnt,state);
    
    if (state == OPERATING) {
	switch(pEnt->entityProp & NEED_SHARED) {
	case NEED_SHARED:
	    pEnt->access->rt = MEM_IO;
	    break;
	case NEED_IO_SHARED:
	    pEnt->access->rt = IO;
	    break;
	case NEED_MEM_SHARED:
	    pEnt->access->rt = MEM;
	    break;
	default:
	    pEnt->access->rt = NONE;
	}
    } else 
	pEnt->access->rt = MEM_IO;
    
    /* disable shared resources */
    if (pEnt->access->pAccess 
	&& pEnt->access->pAccess->AccessDisable)
	pEnt->access->pAccess->AccessDisable(pEnt->access->pAccess->arg);

    /*
     * If device is not under access control it is enabled.
     * If it needs bus routing do it here as it isn't bus
     * type specific. Any conflicts should be checked at this
     * stage
     */
    if (!pEnt->access->pAccess
	&& (pEnt->entityProp & (state == SETUP ? NEED_VGA_ROUTED_SETUP :
				NEED_VGA_ROUTED))) 
	((BusAccPtr)pEnt->busAcc)->set_f(pEnt->busAcc);
    
    /* do we have a driver replacement for the generic access funcs ?*/
    if (pEnt->rac) {
	/* XXX should we use rt here? */
	switch (pEnt->access->rt) {
	case MEM_IO:
	    new = pEnt->rac->io_mem_new;
	    break;
	case IO:
	    new = pEnt->rac->io_new;
	    break;
	case MEM:
	    new = pEnt->rac->mem_new;
	    break;
	default:
	    new = NULL;
	    break;
	}
    }
    if (new) {
	/* does the driver want the old access func? */
	if (pEnt->rac->old) {
	    /* give it to the driver, leave state disabled */
	    (*pEnt->rac->old) = pEnt->access->pAccess;
	} else if ((pEnt->access->rt != NONE) && pEnt->access->pAccess
		   && pEnt->access->pAccess->AccessEnable) {
	    /* driver doesn't want it - enable generic access */
	    pEnt->access->pAccess->AccessEnable(pEnt->access->pAccess->arg);
	}
	/* now replace access funcs */
	pEnt->access->pAccess = new;
	/* call the new disable func just to be shure */
	/* XXX should we do this only if rt != NONE? */
	if (pEnt->access->pAccess && pEnt->access->pAccess->AccessDisable)
	    pEnt->access->pAccess->AccessDisable(pEnt->access->pAccess->arg);
	/* The replacement function needs to handle _all_ shared resources */
	/* unless they are handeled locally and disabled otherwise         */
    } 
} 


void
xf86EnterServerState(xf86State state)
{
    EntityPtr pEnt;
    ScrnInfoPtr pScrn;
    int i,j;
    resType rt;

#ifdef DEBUG
    if (state == SETUP)
	ErrorF("Entering SETUP state\n");
    else
	ErrorF("Entering OPERATING state\n");
#endif

    for (i=0; i<xf86NumScreens; i++) {
	pScrn = xf86Screens[i];
	j = pScrn->entityList[pScrn->numEntities - 1];
	pScrn->access = xf86Entities[j]->access;
	
 	for (j = 0; j<xf86Screens[i]->numEntities; j++) {
 	    pEnt = xf86Entities[xf86Screens[i]->entityList[j]];
 	    if (pEnt->entityProp & (state == SETUP ? NEED_VGA_ROUTED_SETUP
 				    : NEED_VGA_ROUTED))
		xf86Screens[i]->busAccess = pEnt->busAcc;
 	}
    }
    
    /*
     * if we just have one screen we don't have RAC.
     * Therefore just enable the screen and return.
     */
    if (!needRAC) {
	xf86EnableAccess(xf86Screens[0]);
	return;
    }
    
    disableAccess();
    
    for (i=0; i<xf86NumScreens;i++) {

	rt = NONE;
	
	for (j = 0; j<xf86Screens[i]->numEntities; j++) {
	    pEnt = xf86Entities[xf86Screens[i]->entityList[j]];

	    setAccess(pEnt,state);

	    if (pEnt->access->rt != NONE) {
		if (rt != NONE && rt != pEnt->access->rt)
		    rt = MEM_IO;
		else
		    rt = pEnt->access->rt;
	    }
	}
	xf86Screens[i]->resourceType = rt;
	if (rt == NONE) {
	    xf86Screens[i]->access = NULL;
	    xf86Screens[i]->busAccess = NULL;
	}
	
#ifdef DEBUG
	if (xf86Screens[i]->busAccess)
	    ErrorF("Screen %i setting vga route\n",i);
#endif
	switch (rt) {
	case MEM_IO:
	    xf86MsgVerb(X_INFO, 3, "Screen %i shares mem & io resources\n",i);
	    break;
	case IO:
	    xf86MsgVerb(X_INFO, 3, "Screen %i shares io resources\n",i);
	    break;
	case MEM:
	    xf86MsgVerb(X_INFO, 3, "Screen %i shares mem resources\n",i);
	    break;
	default:
	    xf86MsgVerb(X_INFO, 3, "Entity %i shares no resources\n",i);
	    break;
	}
    }
}

void
xf86PostPreInit()
{
    if (xf86NumScreens > 1)
	needRAC = TRUE;

#ifdef XFree86LOADER
    ErrorF("do I need RAC?\n");
    
    if (needRAC) {
	char *list[] = { "rac",NULL };
	ErrorF("  Yes, I do.\n");
	
	if (!xf86LoadModules(list, NULL))
	    FatalError("Cannot load RAC module\n");
    } else
	ErrorF("  No, I don't.\n");
#endif    
 	
    xf86MsgVerb(X_INFO, 3, "resource ranges after preInit:\n");
    xf86PrintResList(3, Acc);
}

void
xf86PostScreenInit(void)
{
    int i,j;
    ScreenPtr pScreen;
    unsigned int flags;
    int nummem = 0, numio = 0;

#ifdef XFree86LOADER
	pointer xf86RACInit = NULL;
	if (needRAC) {
	    xf86RACInit = LoaderSymbol("xf86RACInit");
	    if (!xf86RACInit)
		FatalError("Cannot resolve symbol \"xf86RACInit\"\n");
	}
#endif
#ifdef DEBUG
    ErrorF("PostScreenInit  generation: %i\n",serverGeneration);
#endif
    if (serverGeneration == 1) {
	checkRoutingForScreens(OPERATING);
	for (i=0; i<xf86NumEntities; i++) {
	    checkRequiredResources(i);
	}
	
	/*
	 * after removing NEED_XXX_SHARED from entities that
	 * don't need need XXX resources at all we might have
	 * a single entity left that has NEED_XXX_SHARED set.
	 * In this case we can delete that, too.
	 */
	for (i = 0; i < xf86NumEntities; i++) {
	    if (xf86Entities[i]->entityProp & NEED_MEM_SHARED)
		nummem++;
	    if (xf86Entities[i]->entityProp & NEED_IO_SHARED)
		numio++;
	}
	for (i = 0; i < xf86NumEntities; i++) {
	    if (nummem < 2)
		xf86Entities[i]->entityProp &= ~NEED_MEM_SHARED;
	    if (numio < 2)
		xf86Entities[i]->entityProp &= ~NEED_IO_SHARED;
	}
    }
    
    if (xf86Screens && needRAC) {
	for (i = 0; i < xf86NumScreens; i++) {
	    Bool needRACforMem = FALSE, needRACforIo = FALSE;
	    
	    for (j = 0; j < xf86Screens[i]->numEntities; j++) {
		if (xf86Entities[xf86Screens[i]->entityList[j]]->entityProp
		    & NEED_MEM_SHARED)
		    needRACforMem = TRUE;
		if (xf86Entities[xf86Screens[i]->entityList[j]]->entityProp
		    & NEED_IO_SHARED)
		    needRACforIo = TRUE;
	    }
	    
	    pScreen = xf86Screens[i]->pScreen;
	    flags = 0;
	    if (needRACforMem) {
		flags |= xf86Screens[i]->racMemFlags;
#ifdef DEBUG
		ErrorF("Screen %d is using RAC for mem\n", i);
#endif
	    }
	    if (needRACforIo) {
		flags |= xf86Screens[i]->racIoFlags;
#ifdef DEBUG
		ErrorF("Screen %d is using RAC for io\n", i);
#endif
	    }
	    
#ifdef XFree86LOADER
		((Bool(*)(ScreenPtr,unsigned int))xf86RACInit)
		    (pScreen,flags);
#else
	    xf86RACInit(pScreen,flags);
#endif
	}
    }
    
    xf86EnterServerState(OPERATING);
    
}

/*
 * xf86CheckPciMemBase() checks that the memory base value matches one of the
 * PCI base address register values for the given PCI device.
 */
Bool
xf86CheckPciMemBase(pciVideoPtr pPci, memType base)
{
    int i;

    /* XXX Doesn't handle 64-bit base addresses */
    for (i = 0; i < 6; i++)
	if (base == pPci->memBase[i])
	    return TRUE;
    return FALSE;
}

Bool
xf86IsEntityPrimary(int entityIndex)
{
    EntityPtr pEnt = xf86Entities[entityIndex];
    
    if (primaryBus.type != pEnt->busType) return FALSE;

    switch (pEnt->busType) {
    case BUS_PCI:
	return (pEnt->pciBusId.bus == primaryBus.id.pci.bus &&
		pEnt->pciBusId.device == primaryBus.id.pci.device &&
		pEnt->pciBusId.func == primaryBus.id.pci.func);
    case BUS_ISA:
	return ( primaryBus.type == BUS_ISA );
    default:
	return FALSE;
    }
}

static Bool
fixPciResource(int prt, memType alignment, pciVideoPtr pvp, long type)
{
    int  res_n;
    memType *p_base;
    int *p_size;
    unsigned char p_type;
    resPtr AccTmp = NULL;
    resPtr *pAcc = &AccTmp;
    resPtr avoid = NULL;
    resList p_avoid = PciAvoid;
    resRange range;
    resPtr resBios = 0;
    resPtr w_tmp, w = NULL, w_2nd = NULL;
    PCITAG tag;
    PciBusPtr pbp = xf86PciBus, pbp1 = xf86PciBus;
    resPtr tmp;
    
    if (!pvp) return FALSE;

    type &= ResAccMask;
    if (!type) type = ResShared;
    if (prt < 6) {
	if (pvp->memBase[prt]) {
	    type |= ResMem;
	    res_n = prt;
	    p_base = &(pvp->memBase[res_n]);
	    p_size = &(pvp->size[res_n]);
	    p_type = pvp->type[res_n];
	} else if (pvp->ioBase[prt]){
	    type |= ResIo;
	    res_n = prt;
	    p_base = &(pvp->ioBase[res_n]);
	    p_size = &(pvp->size[res_n]);
	    p_type = pvp->type[res_n];
	} else return FALSE;
    } else if (prt == 6) {
	type |= ResMem;
	res_n = 0xff;	/* special flag for bios rom */
	p_base = &(pvp->biosBase);
	p_size = &(pvp->biosSize);
	/* XXX This should also include the PCI_MAP_MEMORY_TYPE_MASK part */
	p_type = 0;
	RANGE(range,0,0xffffffff,ResExcMemBlock);
	resBios = xf86AddResToList(resBios,&range,-1);
    } else return FALSE;

    if (! *p_base) return FALSE;
    
    type |= ResBlock;
    
    /* setup avoid */
    avoid = addRangesToList(avoid,p_avoid,-1);

    while (pbp) {
	if (pbp->secondary == pvp->bus) {
	    if (type & ResMem) {
		if (((p_type & PCI_MAP_MEMORY_CACHABLE)
#if 0 /*EE*/
		     || (res_n == 0xff)/* bios should also be prefetchable */
#endif
		     ) 
		    && pbp->pmem) {
		    w = findIntersectOfLists(pbp->pmem,ResRange);
		    if (pbp->mem) {
			w_2nd = findIntersectOfLists(pbp->mem,ResRange);
		    }
		} else if (pbp->mem) {
		    w = findIntersectOfLists(pbp->mem,ResRange);
		} 
	    } else if (pbp->io) {
		    w = findIntersectOfLists(pbp->io,ResRange);
	    }
	    
	    while (pbp1) {
		if (pbp1->primary == pvp->bus) {
		    if (type & ResMem) {
			tmp = xf86DupResList(pbp1->pmem);
			avoid = xf86JoinResLists(avoid,tmp);
			tmp = xf86DupResList(pbp1->mem);
			avoid = xf86JoinResLists(avoid,tmp);
		    } else {
			tmp = xf86DupResList(pbp1->io);
			avoid = xf86JoinResLists(avoid,tmp);
		    }
		}	
		pbp1 = pbp1->next;
	    }
	    break;
	}
	pbp = pbp->next;
    }

    if (!w)
	w = xf86DupResList(ResRange);
    
    if (resBios) {
	w_tmp = w;
	w = findIntersectOfLists(w,resBios);
	xf86FreeResList(w_tmp);
	xf86FreeResList(resBios);
    }

    if (!alignment)
	alignment = (1 << (*p_size)) - 1;

    /* Access list holds bios resources -- remove this one */
#ifdef NOTYET
    AccTmp = xf86DupResList(Acc);
    while ((*pAcc)) {
	if ((((*pAcc)->res_type & (type & ~ResAccMask))
	     == (type & ~ResAccMask))
	    && ((*pAcc)->block_begin == (*p_base))
	    && ((*pAcc)->block_end == (*p_base) + (1 << (*p_size)) - 1)) {
	    resPtr acc_tmp = (*pAcc)->next;
	    xfree((*pAcc));
	    (*pAcc) = acc_tmp;
	    break;
	} else
	    pAcc = &((*pAcc)->next);
    }
    /* check if we really need to fix anything */
    RANGE(range, (*p_base), (*p_base) + (1 << (*p_size)) - 1, type);
    if (!ChkConflict(&range,avoid,SETUP)
	&& !ChkConflict(&range,AccTmp,SETUP)
	&& (((*p_base) & alignment) == (*p_base)->block_begin)
	&& ((isSubsetOf(range,w)
	    || (w_2nd && isSubsetOf(range,w_2n))))) {
#ifdef DEBUG
	    ErrorF("nothing to fix\n");
#endif
	xf86FreeResList(AccTmp);
	xf86FreeResList(w);
	xf86FreeResList(w_2nd);
	xf86FreeResList(avoid);
	return TRUE;
    } else {
#ifdef DEBUG
	    ErrorF("removing old resource\n");
#endif
	xf86FreeResList(Acc);
	Acc = AccTmp;
    }
#else
    pAcc = &Acc;
    while ((*pAcc)) {
	if ((((*pAcc)->res_type & (type & ~ResAccMask))
	     == (type & ~ResAccMask))
	    && ((*pAcc)->block_begin == (*p_base))
	    && ((*pAcc)->block_end == (*p_base) + (1 << (*p_size)) - 1)) {
#ifdef DEBUG
	    ErrorF("removing old resource\n");
#endif
	    (*pAcc) = (*pAcc)->next;
	    break;
	} else
	    pAcc = &((*pAcc)->next);
    }
#endif
    
#ifdef DEBUG
    ErrorF("base: 0x%lx alignment: 0x%lx size[bit]: 0x%x\n",
	   (*p_base),alignment,(*p_size));
    xf86MsgVerb(X_INFO, 3, "window:\n");
    xf86PrintResList(3, w);
    if (w_2nd)
	xf86MsgVerb(X_INFO, 3, "2nd window:\n");
    xf86PrintResList(3, w_2nd);
    xf86ErrorFVerb(3,"avoid:\n");
    xf86PrintResList(3,avoid);
#endif
    w_tmp = w;
    while (w) {
	if (type & w->res_type & ResPhysMask) {
	    range = xf86GetBlock(type,alignment + 1, w->block_begin,
				 w->block_end, alignment,avoid);
	    if (range.type != ResEnd)
		break;
	}
	w = w->next;
    }
    xf86FreeResList(w_tmp);
    /* if unsuccessful and memory prefetchable try non-prefetchable */
    if (range.type == ResEnd && w_2nd) {
	w_tmp = w_2nd;
	while (w_2nd) {
	    if (type & w_2nd->res_type & ResPhysMask) {
		range = xf86GetBlock(type,alignment + 1, w_2nd->block_begin,
				     w_2nd->block_end,alignment,avoid);
		if (range.type != ResEnd)
		    break;
	    }
	w_2nd = w_2nd->next;
	}
	xf86FreeResList(w_tmp);
    }
    xf86FreeResList(avoid);

    if (range.type == ResEnd)
	return FALSE;
    
    (*p_size) = 0;
    while (alignment >> (*p_size))
	(*p_size)++;
    (*p_base) = range.rBegin;
    
#ifdef DEBUG
    ErrorF("New PCI res %i base: 0x%lx, size: 0x%lx, type %s\n",
	   res_n,(*p_base),(1 << (*p_size)),type | ResMem ? "Mem" : "Io");
#endif
    tag = pciTag(pvp->bus,pvp->device,pvp->func);

    if (res_n != 0xff) /*XXX fix for 64 bit */
	pciWriteLong(tag,PCI_CMD_BASE_REG + res_n * sizeof(CARD32),
		     (CARD32)(*p_base) | (CARD32)(p_type));
    else {
	CARD32 val = pciReadLong(tag,PCI_CMD_BIOS_REG) & 0x01;
	pciWriteLong(tag,PCI_CMD_BIOS_REG,(CARD32)(*p_base) | val);
    }
    /* fake BIOS allocated resource */
    range.type |= ResBios;
    Acc = xf86AddResToList(Acc, &range,-1);
    
    return TRUE;
    
}

Bool
xf86FixPciResource(int entityIndex, int prt, memType alignment,
		    long type)
{
    pciVideoPtr pvp = xf86GetPciInfoForEntity(entityIndex);
    return fixPciResource(prt, alignment, pvp, type);
}

resPtr
xf86ReallocatePciResources(int entityIndex, resPtr pRes)
{
    pciVideoPtr pvp = xf86GetPciInfoForEntity(entityIndex);
    resPtr pBad = NULL,pResTmp;
    unsigned int prt = 0;
    int i;
    
    if (!pvp) return pRes;

    while (pRes) {
	switch (pRes->res_type & ResPhysMask) {
	case ResMem:
	    if (pRes->block_begin == pvp->biosBase &&
		pRes->block_end == pvp->biosBase + (1 << (pvp->biosSize)) - 1)
		prt = 6;
	    else for (i = 0 ; i < 6; i++) 
		if (pRes->block_begin == pvp->memBase[i]
		    && pRes->block_end
		    == pvp->memBase[i] + (1 << (pvp->size[i])) - 1) {
		    prt = i;
		    break;
		}
	    break;
	case ResIo:
	    for (i = 0 ; i < 6; i++) 
		if (pRes->block_begin == pvp->ioBase[i]
		    && pRes->block_end
		    == pvp->ioBase[i] + (1 << (pvp->size[i])) - 1) {
		    prt = i;
		    break;
		}
	    break;
	}

	if (!prt) return pRes;

	pResTmp = pRes->next;
	if (! fixPciResource(prt, 0, pvp, pRes->res_type)) {
	    pRes->next = pBad;
	    pBad = pRes;
	} else
	    xfree(pRes);
	
	pRes = pResTmp;
    }
    return pBad;
}

	
resPtr
xf86SetOperatingState(resList list, int entityIndex, int mask)
{
    resPtr acc;
    resPtr r_fail = NULL;

    while (list->type != ResEnd) {
	acc = Acc;
	while (acc) {
#define MASK (ResPhysMask | ResExtMask)
	    if (acc->entityIndex == entityIndex
		&& acc->val.a == list->a && acc->val.b == list->b
		&& (acc->val.type & MASK) == (list->type & MASK))
		break;
#undef MASK
	    acc = acc->next;
	}
	if (acc)
	    acc->val.type = (acc->val.type & ~ResOprMask)
		| (mask & ResOprMask);
	else
	    r_fail = xf86AddResToList(r_fail,list,entityIndex);

	list ++;
    }
    
    return r_fail;
}

memType
getValidBIOSBase(PCITAG tag, int num)
{
    pciVideoPtr pvp;
    PciBusPtr pbp, pbp1;
    memType start_mp = 0, start_m = 0;
    memType end_mp, end_m;
    resPtr tmp, avoid;
    resRange range;
    int n = 0;

    if (!xf86PciVideoInfo) return 0;
    
    while ((pvp = xf86PciVideoInfo[n++])) {
	if (pciTag(pvp->bus,pvp->device,pvp->func) == tag)
	    break;
    }
    if (!pvp) return 0;
    avoid = NULL;
    end_m = PCI_MEMBASE_LENGTH_MAX;
    end_mp = PCI_MEMBASE_LENGTH_MAX;
    pbp = pbp1 = xf86PciBus;
    while (pbp) {
	if (pbp->secondary == pvp->bus) {
	    if (pbp->pmem) {
		start_mp = pbp->pmem->block_begin;
		end_mp = MIN(end_mp,pbp->pmem->block_end);
	    }
	    if (pbp->mem) {
		start_m = pbp->mem->block_begin;
		end_m = MIN(end_m,pbp->mem->block_end);
	    }
	}
	while (pbp1) {
	    if (pbp1->primary == pvp->bus) {
		tmp = xf86DupResList(pbp1->pmem);
		avoid = xf86JoinResLists(avoid,tmp);
		tmp = xf86DupResList(pbp1->mem);
		avoid = xf86JoinResLists(avoid,tmp);
	    }
	    pbp1 = pbp1->next;
	}	
	pbp = pbp->next;
    }	

    if (pvp->biosBase) { /* try biosBase first */
	RANGE(range, pvp->biosBase,
	      pvp->biosBase + (1 << pvp->biosSize) - 1,
	      ResExcMemBlock);
	if (((range.rBegin >= start_m && range.rEnd <= end_m) ||
	     (range.rBegin >= start_mp && range.rEnd <= end_mp))
	    && ! ChkConflict(&range,avoid,SETUP)) {
	    xf86FreeResList(avoid);
	    return pvp->biosBase;
	}
    }
#if 0
    if (num >= 0 && num <= 5 && pvp->memBase[num]) {
    /* then try suggested memBase */
	RANGE(range, pvp->memBase[num],
	      pvp->memBase[num] + (1 << pvp->biosSize) - 1,
	      ResExcMemBlock);  /* keep bios size ! */
	if (((range.rBegin >= start_m && range.rEnd <= end_m) ||
	     (range.rBegin >= start_mp && range.rEnd <= end_mp))
	    && ! ChkConflict(&range,avoid,SETUP)) {
	    xf86FreeResList(avoid);
	    return pvp->memBase[num];
	}
    }
#endif
    range = xf86GetBlock(ResExcMemBlock, (1 << pvp->biosSize),start_m,end_m,
			 (1 << pvp->biosSize) -1, avoid);
    xf86FreeResList(avoid);
    return range.rBase;
}

static resPtr
addRangesToList(resPtr list, resRange *pRange, int entityIndex)
{
    while(pRange && pRange->type != ResEnd) {
	list = xf86AddResToList(list,pRange,entityIndex);
	pRange++;
    }
    return list;
}

/*
 * not yet used. FIXME: add support for sparse
 */
static Bool
isSubsetOf(resRange range, resPtr list)
{
    while (list) {
	if (range.type & list->res_type & ResPhysMask) {
	    switch (range.type & ResExtMask) {
	    case ResBlock:
		switch (list->res_type & ResExtMask) {
		case ResBlock:
		    if (range.rBegin >= list->block_begin
			&& range.rEnd <= list->block_end)
			return TRUE;
			break;
		}
		break;
	    }
	}
	list = list->next;
    }
    return FALSE;
}

static Bool
isListSubsetOf(resPtr list, resPtr BaseList)
{
    while (list) {
	if (! isSubsetOf(list->val,BaseList))
	    return FALSE;
	list = list->next;
    }
    return TRUE;
}

static resPtr
findIntersect(resRange Range, resPtr list)
{
    resRange range;
    resPtr new = NULL;
    
    while (list) {
	    if (Range.type & list->res_type & ResPhysMask) {
		switch (Range.type & ResExtMask) {
		case ResBlock:
		    switch (list->res_type & ResExtMask) {
		    case ResBlock:
			if (Range.rBegin >= list->block_begin)
			    range.rBegin = Range.rBegin;
			else
			    range.rBegin = list->block_begin;
			if (range.rEnd <= list->block_end)
			    range.rEnd = Range.rEnd;
			else 
			    range.rEnd = Range.rEnd;
			if (range.rEnd > range.rBegin) {
			    range.type = Range.type;
			    xf86AddResToList(new,&range,-1);
			}
			break;
		    }
		    break;
		}
	    }
	list = list->next;
    }
    return new;
}

static resPtr
findIntersectOfLists(resPtr l1, resPtr l2)
{
    resPtr ret = NULL;

    while (l1) {
	ret = xf86JoinResLists(ret,findIntersect(l1->val,l2));
	l1 = l1->next;
    }
    return ret;
}

Bool
xf86IsPciDevPresent(int bus, int dev, int func)
{
    int i = 0;
    pciConfigPtr pcp;
    
    while ((pcp = xf86PciInfo[i]) != NULL) {
	if ((pcp->busnum == bus)
	    && (pcp->devnum == dev)
	    && (pcp->funcnum == func))
	    return TRUE;
	i++;
    }
    return FALSE;
}

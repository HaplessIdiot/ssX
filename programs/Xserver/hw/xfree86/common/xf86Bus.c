/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Bus.c,v 1.15 1999/02/19 21:27:00 hohndel Exp $ */

/*
 * Copyright (c) 1997-1999 by The XFree86 Project, Inc.
 */

/*
 * This file contains the interfaces to the bus-specific code
 */

#include <ctype.h>
#include <stdlib.h>
#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/* Bus-specific headers */

#include "xf86Pci.h"
#include "xf86Bus.h"
#define DECLARE_CARD_DATASTRUCTURES TRUE
#include "xf86PciInfo.h"

/* Bus-specific globals */

pciConfigPtr *xf86PciInfo = NULL;		/* Full PCI probe info */
pciVideoPtr *xf86PciVideoInfo = NULL;		/* PCI probe for video hw */
pciAccPtr * xf86PciAccInfo = NULL;              /* PCI access related */
/* Local bus data */

static BusPtr *xf86BusSlots = NULL;	/* Bus slots claimed by drivers */
static int xf86NumBusSlots = 0;

static xf86AccessRec AccessNULL = {NULL,NULL,NULL};

static xf86CurrentAccessRec xf86CurrentAccessVga = {&AccessNULL,&AccessNULL};
static xf86CurrentAccessRec xf86CurrentAccess8514 = {&AccessNULL,&AccessNULL};
static xf86CurrentAccessRec xf86CurrentAccessMono = {&AccessNULL,&AccessNULL};

static BusType primaryBus = BUS_NONE;
static struct {
    int bus;
    int dev;
    int func;
} primaryPciDev = {0x7fff, 0x7fff, 0x7fff}; /*if invalid primary dev not PCI*/

static Bool ResAccessEnter = FALSE;

struct {
    unsigned int exclusive_Vga :1,
	exclusive_Vga_taken :1,
	exclusive_8514 :1,
	exclusive_8514_taken :1,
	exclusive_mono :1,
	exclusive_mono_taken :1;
} Resources = {0,0,0,0,0,0};

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
	if (PCIINFOCLASSES(pcrp->_base_class, pcrp->_sub_class)) {
	    num++;
	    xf86PciVideoInfo = xnfrealloc(xf86PciVideoInfo,
					  sizeof(pciVideoPtr) * (num + 1));
	    xf86PciVideoInfo[num] = NULL;
	    info = xf86PciVideoInfo[num - 1] = xnfalloc(sizeof(pciVideoRec));
	    info->vendor = pcrp->_vendor;
	    info->chipType = pcrp->_device;
	    info->chipRev = pcrp->_rev_id;
	    info->subsysVendor = pcrp->_subsys_vendor;
	    info->subsysCard = pcrp->_subsys_card;
	    info->bus = pcrp->busnum;
	    info->device = pcrp->devnum;
	    info->func = pcrp->funcnum;
	    info->class = pcrp->_base_class;
	    info->subclass = pcrp->_sub_class;
	    info->interface = pcrp->_prog_if;
	    info->biosBase = pcrp->_baserom;
	    info->biosSize = pciGetBaseSize(pcrp->tag, 6, TRUE);
	    info->thisCard = pcrp;
	    for (j = 0; j < 6; j++) {
		info->memBase[j] = 0;
		info->ioBase[j] = 0;
		if (PCINONSYSTEMCLASSES(info->class, info->subclass))
		    info->size[j] = pciGetBaseSize(pcrp->tag, j, TRUE);
		else
		    info->size[j] = pciGetBaseSize(pcrp->tag, j, FALSE);
		info->type[j] = 0;
	    }

	    /*
	     * 64-bit base addresses are checked for and avoided.
	     * XXX Should deal with them on platforms that support them.
	     */

	    if (pcrp->_base0) {
		if (pcrp->_base0 & PCI_MAP_IO) {
		    info->ioBase[0] = PCIGETIO(pcrp->_base0);
		    info->type[0] = pcrp->_base0 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base0))
			mem64 = TRUE;
		    else {
			info->memBase[0] = PCIGETMEMORY(pcrp->_base0);
			info->type[0] = pcrp->_base0 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    }

	    if (pcrp->_base1 && !mem64) {
		if (pcrp->_base1 & PCI_MAP_IO) {
		    info->ioBase[1] = PCIGETIO(pcrp->_base1);
		    info->type[1] = pcrp->_base0 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base1))
			mem64 = TRUE;
		    else {
			info->memBase[1] = PCIGETMEMORY(pcrp->_base1);
			info->type[1] = pcrp->_base0 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->_base2 && !mem64) {
		if (pcrp->_base2 & PCI_MAP_IO) {
		    info->ioBase[2] = PCIGETIO(pcrp->_base2);
		    info->type[2] = pcrp->_base0 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base2))
			mem64 = TRUE;
		    else {
			info->memBase[2] = PCIGETMEMORY(pcrp->_base2);
			info->type[2] = pcrp->_base0 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->_base3 && !mem64) {
		if (pcrp->_base3 & PCI_MAP_IO) {
		    info->ioBase[3] = PCIGETIO(pcrp->_base3);
		    info->type[3] = pcrp->_base0 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base3))
			mem64 = TRUE;
		    else {
			info->memBase[3] = PCIGETMEMORY(pcrp->_base3);
			info->type[3] = pcrp->_base0 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->_base4 && !mem64) {
		if (pcrp->_base4 & PCI_MAP_IO) {
		    info->ioBase[4] = PCIGETIO(pcrp->_base4);
		    info->type[4] = pcrp->_base0 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base4))
			mem64 = TRUE;
		    else {
			info->memBase[4] = PCIGETMEMORY(pcrp->_base4);
			info->type[4] = pcrp->_base0 & PCI_MAP_MEMORY_ATTR_MASK;
		    }
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->_base5 && !mem64) {
		if (pcrp->_base5 & PCI_MAP_IO) {
		    info->ioBase[5] = PCIGETIO(pcrp->_base5);
		    info->type[5] = pcrp->_base0 & PCI_MAP_IO_ATTR_MASK;
		} else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base5))
			mem64 = TRUE;
		    else {
			info->memBase[5] = PCIGETMEMORY(pcrp->_base5);
			info->type[5] = pcrp->_base0 & PCI_MAP_MEMORY_ATTR_MASK;
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

static BusPtr
xf86AllocateBusSlot(void)
{
    xf86NumBusSlots++;
    xf86BusSlots = xnfrealloc(xf86BusSlots, sizeof(BusPtr) * xf86NumBusSlots);
    xf86BusSlots[xf86NumBusSlots - 1] = xnfalloc(sizeof(BusRec));
    return xf86BusSlots[xf86NumBusSlots - 1];
}

static void
xf86FreeBusSlot(int slot)
{
    int i;

    if (xf86NumBusSlots == 0 || xf86BusSlots == NULL)
	return;

    if (slot > xf86NumBusSlots - 1)
	return;

    if (xf86BusSlots[slot] == NULL)
	return;

    xfree(xf86BusSlots[slot]);
    xf86NumBusSlots--;
    for (i = slot; i < xf86NumBusSlots; i++)
	xf86BusSlots[i] = xf86BusSlots[i + 1];
}

/*
 * Check if the slot requested is free.  If it is already in use, return FALSE.
 */

Bool
xf86CheckPciSlot(int bus, int device, int func, BusResource res)
{
    int i;
    BusPtr p;

    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	/* Check if this PCI slot is taken */
	if (p->busType == BUS_PCI && p->pciBusId.bus == bus &&
	    p->pciBusId.device == device && p->pciBusId.func == func)
	    return FALSE;
    }
	/* If VGA access is requested, check if that is alreay taken */
    switch (res) {
    case RES_SHARED_VGA:
	if (Resources.exclusive_Vga) return FALSE;
	break;
    case RES_SHARED_8514:
	if (Resources.exclusive_8514) return FALSE;
	break;
    case RES_VGA:
	if (!Resources.exclusive_Vga || Resources.exclusive_Vga_taken) 
	    return FALSE;
	Resources.exclusive_Vga_taken = 1;
	break;
    case RES_8514:
	if (!Resources.exclusive_8514 || Resources.exclusive_8514_taken) 
	    return FALSE;
	Resources.exclusive_8514_taken = 1;
	break;
    case RES_MONO:
	xf86Msg(X_WARNING,"Resource Mono is not supported for PCI devices\n");
	return FALSE;
    default:
	return TRUE;
    }
    return TRUE;
}

/*
 * If the slot requested is already in use, return FALSE.
 * Otherwise, claim the slot for the screen requesting it.
 */

Bool
xf86ClaimPciSlot(int bus, int device, int func, BusResource res, 
		 DriverPtr drvp, int chipset, int scrnIndex)
{
    BusPtr p;

    if (xf86CheckPciSlot(bus, device, func, res)) {
	p = xf86AllocateBusSlot();
	p->driver = drvp;
	p->chipset = chipset;
	p->busType = BUS_PCI;
	p->scrnIndex = scrnIndex;
	p->pciBusId.bus = bus;
	p->pciBusId.device = device;
	p->pciBusId.func = func;
	p->resource = res;
	return TRUE;
    } else
	return FALSE;
}

/*
 * Release the slot for a particular PCI ID
 */

void
xf86ReleasePciSlot(int bus, int device, int func)
{
    int i;
    BusPtr p;

    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	if (p->busType == BUS_PCI && p->pciBusId.bus == bus &&
	    p->pciBusId.device == device && p->pciBusId.func == func) {
	    xf86FreeBusSlot(i);
	    return;
	}
    }
}

/*
 * Get xf86PciVideoInfo for a driver.
 */
pciVideoPtr *
xf86GetPciVideoInfo()
{
    return xf86PciVideoInfo;
}

/*
 * Get the list of pciVideoRecs claimed by a screen.
 */
int
xf86GetPciInfoForScreen(int scrnIndex, pciVideoPtr **pPciList,
			BusResource **pRes)
{
    int num = 0;
    int i;
    BusPtr p;
    pciVideoPtr *ppPci;

    if (xf86PciVideoInfo == NULL)
	return 0;

    if (pPciList != NULL)
	*pPciList = NULL;
    if (pRes != NULL)
	*pRes = NULL;
    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	if (p->scrnIndex == scrnIndex && p->busType == BUS_PCI) {
	    for (ppPci = xf86PciVideoInfo; *ppPci != NULL; ppPci++) {
		if (p->pciBusId.bus == (*ppPci)->bus &&
		    p->pciBusId.device == (*ppPci)->device &&
		    p->pciBusId.func == (*ppPci)->func) {
		    break;
		}
	    }
	    if (*ppPci != NULL) {
		num++;
		if (pPciList != NULL) {
		    *pPciList = xnfrealloc(*pPciList,
					   num * sizeof(pciVideoPtr));
		    (*pPciList)[num - 1] = *ppPci;
		}
		if (pRes != NULL) {
		    *pRes = xnfrealloc(*pRes, num * sizeof(BusResource));
		    (*pRes)[num - 1] = p->resource;
		}
	    } else {
		/* This shouldn't happen */
		xf86DrvMsgVerb(scrnIndex, X_WARNING, 0, "There is no entry "
			       "for PCI slot %d:%d:%d in the list of PCI "
			       "video cards\n", p->pciBusId.bus,
			       p->pciBusId.device, p->pciBusId.func);
	    }
	}
    }
    return num;
}


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
 * Check if the slot requested is free.  If it is already in use, return FALSE.
 */

Bool
xf86CheckIsaSlot(BusResource res)
{
    switch (res) {
    case RES_SHARED_VGA:
    case RES_SHARED_8514:
	return FALSE;
    case RES_VGA:
	if (!Resources.exclusive_Vga || Resources.exclusive_Vga_taken) 
	    return FALSE;
	Resources.exclusive_Vga_taken = 1;
	break;
    case RES_8514:
	if (!Resources.exclusive_8514 || Resources.exclusive_8514_taken) 
	    return FALSE;
	Resources.exclusive_8514_taken = 1;
	break;
    case RES_MONO:
	if (!Resources.exclusive_mono || Resources.exclusive_mono_taken) 
	    return FALSE;
	Resources.exclusive_mono_taken = 1;
	break;
    default:
	return TRUE;
    }
    return TRUE;
}

/*
 * If the slot requested is already in use, return FALSE.
 * Otherwise, claim the slot for the screen requesting it.
 */

Bool
xf86ClaimIsaSlot(BusResource res, DriverPtr drvp, int chipset, int scrnIndex)
{
    BusPtr p;

    if (xf86CheckIsaSlot(res)) {
	p = xf86AllocateBusSlot();
	p->driver = drvp;
	p->chipset = chipset;
	p->busType = BUS_ISA;
	p->scrnIndex = scrnIndex;
	p->resource = res;
	return TRUE;
    } else
	return FALSE;
}

/*
 * Release the slot for a particular ISA type
 */

void
xf86ReleaseIsaSlot(BusResource res)
{
    int i;
    BusPtr p;

    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	if (p->busType == BUS_ISA && p->resource == res) {
	    xf86FreeBusSlot(i);
	    return;
	}
    }
}

/*
 * Get the list of ISA "slots" claimed by a screen
 *
 * Note: The ISA implementation here assumes that only one ISA "slot" type
 * can be claimed by any one screen.  That means a return value other than
 * 0 or 1 isn't useful.
 */
int
xf86GetIsaInfoForScreen(int scrnIndex, BusResource **pRes)
{
    int num = 0;
    int i;
    BusPtr p;

    if (pRes != NULL)
	*pRes = NULL;
    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	if (p->scrnIndex == scrnIndex && p->busType == BUS_ISA) {
	    num++;
	    if (pRes != NULL) {
		*pRes = xnfrealloc(*pRes, num * sizeof(BusResource));
		(*pRes)[num - 1] = p->resource;
	    }
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
 * Release all slots claimed by a screen.
 */

void
xf86FreeBusSlots(int scrnIndex)
{
    int i;

    for (i = 0; i < xf86NumBusSlots; i++) {
	if (xf86BusSlots[i]->scrnIndex == scrnIndex)
	    xf86FreeBusSlot(i);
    }
}

/*
 * Change the scrnIndex in bus slots to reflect the new indices after a
 * screen has been removed.
 */

void
xf86ChangeBusIndex(int oldIndex, int newIndex)
{
    int i;

    for (i = 0; i < xf86NumBusSlots; i++) {
	if (xf86BusSlots[i]->scrnIndex == oldIndex)
	    xf86BusSlots[i]->scrnIndex = newIndex;
    }
}

/*
 * xf86DeleteBusSlotsForScreen() -- deletes all bus slots 
 * assigned to a specific screen.
 */
void
xf86DeleteBusSlotsForScreen(int scrnIndex)
{
    int i;
    
    for (i = 0; i < xf86NumBusSlots; i++) {
	if (xf86BusSlots[i]->scrnIndex == scrnIndex){
	    xf86FreeBusSlot(i);
	    i--;
	}
    }
}

static Bool
CompBusTypeForScreen(int scrnIndex, BusType bt)
{
    int i;
    BusPtr p;

    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	if (p->scrnIndex == scrnIndex && p->busType == bt)
	    return TRUE;
	}
    return FALSE;
}

Bool
xf86IsPciBus(int scrnIndex)
{
    return CompBusTypeForScreen(scrnIndex,BUS_PCI);
}

Bool
xf86IsIsaBus(int scrnIndex)
{
    return CompBusTypeForScreen(scrnIndex,BUS_ISA);
}

int
xf86FindChipsetsForScreen(int scrnIndex, DriverPtr drv, int **chipsets)
{
    int num = 0;
    int i;
    BusPtr p;
    
    *chipsets = NULL;
    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	if (p->scrnIndex == scrnIndex && p->driver == drv){
	    num++;
	    *chipsets = xnfrealloc(*chipsets,num * sizeof(int));
	    (*chipsets)[num - 1] = p->chipset;
	}
    }
    return num;
}
	
/*
 * IO enable/disable related routines for PCI
 */
#define SETBITS PCI_CMD_IO_ENABLE
static void
pciIoAccessEnable(void* arg)
{
    pciSetBitsLong((*(PCITAG *)arg), PCI_CMD_STAT_REG, SETBITS, SETBITS);
}

static void
pciIoAccessDisable(void* arg)
{
    pciSetBitsLong((*(PCITAG *)arg), PCI_CMD_STAT_REG, SETBITS, 0);
}

#undef SETBITS
#define SETBITS (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE)
static void
pciIo_MemAccessEnable(void* arg)
{
    pciSetBitsLong((*(PCITAG *)arg), PCI_CMD_STAT_REG, SETBITS, SETBITS);
}

static void
pciIo_MemAccessDisable(void* arg)
{
    pciSetBitsLong((*(PCITAG *)arg), PCI_CMD_STAT_REG, SETBITS, 0);
}

#undef SETBITS
#define SETBITS (PCI_CMD_MEM_ENABLE)
static void
pciMemAccessEnable(void* arg)
{
    pciSetBitsLong((*(PCITAG *)arg), PCI_CMD_STAT_REG, SETBITS, SETBITS);
}

static void
pciMemAccessDisable(void* arg)
{
    pciSetBitsLong((*(PCITAG *)arg), PCI_CMD_STAT_REG,SETBITS, 0);
}
#undef SETBITS

static void
savePciAccess(PCITAG tag, CARD32 *ptr)
{
    *ptr = pciReadLong(tag,PCI_CMD_STAT_REG);
}

static void
restorePciAccess(PCITAG tag, CARD32 *ptr)
{
    pciWriteLong(tag,PCI_CMD_STAT_REG,*ptr);
}

#define PCISHAREDIOCLASSES(b,s)					      \
    (((b) == PCI_CLASS_PREHISTORIC && (s) == PCI_SUBCLASS_PREHISTORIC_VGA) || \
     ((b) == PCI_CLASS_DISPLAY && (s) == PCI_SUBCLASS_DISPLAY_VGA))

static void
setupPciAccess(void)
{
    int i = 0;
    int j = 0;
    pciConfigPtr pcp; 
    pciAccPtr pcaccp;

    if (xf86PciAccInfo != NULL)
	return;
  
    if (xf86PciInfo == NULL)
	return;

    while ((pcp = xf86PciInfo[i]) != NULL) {
	i++;
	if (PCISHAREDIOCLASSES(pcp->_base_class, pcp->_sub_class)) {
	    j++;
	    xf86PciAccInfo = xnfrealloc(xf86PciAccInfo,
					sizeof(pciAccPtr) * (j + 1));
	    xf86PciAccInfo[j] = NULL;
	    pcaccp = xf86PciAccInfo[j - 1] = xnfalloc(sizeof(pciAccRec));
	    pcaccp->busnum = pcp->busnum; 
	    pcaccp->devnum = pcp->devnum; 
	    pcaccp->funcnum = pcp->funcnum;
	    pcaccp->tag = pciTag(pcp->busnum, pcp->devnum, pcp->funcnum);
	    pcaccp->ioAccess.AccessDisable = pciIoAccessDisable;
	    pcaccp->ioAccess.AccessEnable = pciIoAccessEnable;
	    pcaccp->ioAccess.arg = &pcaccp->tag;
	    pcaccp->io_memAccess.AccessDisable = pciIo_MemAccessDisable;
	    pcaccp->io_memAccess.AccessEnable = pciIo_MemAccessEnable;
	    pcaccp->io_memAccess.arg = &pcaccp->tag;
	    pcaccp->memAccess.AccessDisable = pciMemAccessDisable;
	    pcaccp->memAccess.AccessEnable = pciMemAccessEnable;
	    pcaccp->memAccess.arg = &pcaccp->tag;
	    savePciAccess(pcaccp->tag, &pcaccp->saveio);
	}
    }
}


static void 
EnterPciAccess(void)
{
    pciAccPtr paccp;
    int i = 0;

    if (xf86PciAccInfo == NULL) 
	return;

    while ((paccp = xf86PciAccInfo[i]) != NULL) {
	i++;
	savePciAccess(paccp->tag, &paccp->saveio);
	restorePciAccess(paccp->tag, &paccp->restoreio);
    }
}

static void 
LeavePciAccess(void)
{
    pciAccPtr paccp;
    int i = 0;

    if (xf86PciAccInfo == NULL) 
	return;

    while ((paccp = xf86PciAccInfo[i]) != NULL) {
	i++;
	savePciAccess(paccp->tag, &paccp->restoreio);
	restorePciAccess(paccp->tag, &paccp->saveio);
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
	pciIo_MemAccessDisable(paccp->io_memAccess.arg);
    }
}

static void
FindPciPrimaryDevice(void)
{  
    int i = 0;
    int j;
    pciConfigPtr pcp;
    pciAccPtr paccp;
  
    if (!xf86PciInfo) return;

    if (!xf86PciAccInfo) return;

    while ((pcp = xf86PciInfo[i]) != NULL) { 
	if (pcp->_command & PCI_CMD_IO_ENABLE) {
	    j = 0;
	    while ((paccp = xf86PciAccInfo[j]) != NULL) {
		if (paccp->busnum == pcp->busnum
		    && paccp->devnum == pcp->devnum
		    && paccp->funcnum == pcp->funcnum) {
		    if (PCISHAREDIOCLASSES(pcp->_base_class,pcp->_sub_class))
			if (pcp->_prog_if == 0) {
			    primaryPciDev.bus = pcp->busnum;
			    primaryPciDev.dev = pcp->devnum;
			    primaryPciDev.func = pcp->funcnum;
			    primaryBus = BUS_PCI;
			    /* prefer VGA */
			    return;
			} else if (pcp->_prog_if == 1){
			    primaryPciDev.bus = pcp->busnum;
			    primaryPciDev.dev = pcp->devnum;
			    primaryPciDev.func = pcp->funcnum;
			    primaryBus = BUS_PCI;
			}
		}
		j++;
	    }
	}
	i++;
    }
}

/*
 * xf86IsPrimaryPci() -- return TRUE if primary device
 * is PCI and bus, dev and func numbers match.
 */
 
Bool
xf86IsPrimaryPci(pciVideoPtr pPci)
{
    if (primaryBus != BUS_PCI) return FALSE;
    return (pPci->bus == primaryPciDev.bus &&
	    pPci->device == primaryPciDev.dev &&
	    pPci->func == primaryPciDev.func);
}

/*
 * xf86IsPrimaryIsa() -- return TRUE if primary device
 * is ISA.
 */
 
Bool
xf86IsPrimaryIsa(void)
{
    return ( primaryBus == BUS_ISA );
}

/*
 * Generic VGA IO part - add other buses here
 */

/*
 * xf86AccessEnter() -- gets called to save the text mode VGA IO 
 * resources when reentering the server after a VT switch.
 */ 

void
xf86AccessEnter(void)
{
    if (ResAccessEnter) 
	return;
    EnterPciAccess();
    ResAccessEnter = TRUE;
}

/*
 * xf86AccessLeave() -- gets called to restore the access to the 
 * VGA IO resources when switching VT or on server exit.
 */

void
xf86AccessLeave(void)
{
    if (!ResAccessEnter)
	return;
    LeavePciAccess();
    ResAccessEnter = FALSE;
}

/*
 * xf86AccessSetup() - set up everything needed for access control
 * called only once on first server generation.
 */

void
xf86AccessSetup(void)
{
    setupPciAccess();
    ResAccessEnter = TRUE;
}

/*
 * xf86AddControlledResource() -- Find and add pointer 
 * to Access structure to ScreenRec.
 */
static int
GetRBTypesForScreen(int scrnIndex, BusType **ppBt, BusResource **ppRes)
{
    int num = 0;
    int i;
    BusPtr p;

    *ppRes = NULL;
    *ppBt  = NULL;
    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	if (p->scrnIndex == scrnIndex) {
	    num++;
	    *ppRes = xnfrealloc(*ppRes, num * sizeof(BusResource));
	    (*ppRes)[num - 1] = p->resource;
	    *ppBt = xnfrealloc(*ppBt, num * sizeof(BusType));
	    (*ppBt)[num - 1] = p->busType;
	}
    }
    return num;
}

#define RETURN xfree(bt);\
               xfree(br);\
	       return
void
xf86AddControlledResource(ScrnInfoPtr pScreen, resType rt)
{
    int num,i,j;
    int k = 0;
    pciVideoPtr *ppvp;
    pciAccPtr paccp;
    BusType *bt;
    BusResource *br;
    if (pScreen->Access.pAccess)
	xf86DelControlledResource(&pScreen->Access,FALSE);
    num = GetRBTypesForScreen(pScreen->scrnIndex,&bt,&br);
    pScreen->Access.CurrentAccess = NULL;
    for (i = 0; i < num; i++) {
	switch (br[i]) {
	case RES_VGA:
	    pScreen->Access.CurrentAccess = &xf86CurrentAccessVga;
	    pScreen->Access.pAccess = &AccessNULL;
	    pScreen->Access.rt = rt;
	    RETURN;
	case RES_8514:
	    pScreen->Access.CurrentAccess = &xf86CurrentAccess8514;
	    pScreen->Access.pAccess = &AccessNULL;
	    pScreen->Access.rt = rt;
	    RETURN;
	case RES_MONO:
	    pScreen->Access.CurrentAccess = &xf86CurrentAccessMono;
	    pScreen->Access.pAccess = &AccessNULL;
	    pScreen->Access.rt = rt;
	    RETURN;
	case RES_SHARED_VGA:
	    pScreen->Access.CurrentAccess = &xf86CurrentAccessVga;
	    break;
	case RES_SHARED_8514:
	    pScreen->Access.CurrentAccess = &xf86CurrentAccess8514;
	    break;
	default:
	    continue;
	}
	break;
    }
    /* 
     * Screen doesn't require any generic interface
     * for sanity set resources type to none.
     */
    if ( pScreen->Access.CurrentAccess == NULL ) 
      rt = NONE;
    pScreen->Access.rt = rt;

    if (i < num) {
	switch (bt[i]) {
	case BUS_ISA:
	case BUS_NONE:
	    pScreen->Access.pAccess = &AccessNULL;
	    RETURN;
	case BUS_PCI:
	    num = xf86GetPciInfoForScreen(pScreen->scrnIndex, &ppvp,NULL);
	    for (j = 0; j < num; j++) {
		k = 0;
		while ((paccp = xf86PciAccInfo[k]) != NULL) {
		    if (paccp->busnum == ppvp[j]->bus
		       && paccp->devnum == ppvp[j]->device
		       && paccp->funcnum == ppvp[j]->func){
			switch (rt) {
			case IO:
			    pScreen->Access.pAccess = &paccp->ioAccess;
			    (*paccp->io_memAccess.AccessEnable)
				(paccp->io_memAccess.arg);
			    (*paccp->ioAccess.AccessDisable)
				(paccp->ioAccess.arg);
			    break;
			case MEM_IO:
			    pScreen->Access.pAccess = &paccp->io_memAccess;
			    (*paccp->io_memAccess.AccessDisable)
				(paccp->io_memAccess.arg);
			    break;
			case MEM:
			    pScreen->Access.pAccess = &paccp->memAccess;
			    (*paccp->memAccess.AccessDisable)
				(paccp->memAccess.arg);
			    break;
			case NONE:
			    pScreen->Access.pAccess = NULL;
			    (*paccp->io_memAccess.AccessEnable)
				(paccp->io_memAccess.arg);
			    break;
			default:
			    break;
			}
			RETURN;
		    }
		    k++;
		}
	    }
	    pScreen->Access.pAccess = NULL;
	    RETURN;
	default:
	    pScreen->Access.pAccess = NULL;
	    RETURN;
	}
    } else {
	pScreen->Access.CurrentAccess = NULL;
	pScreen->Access.pAccess = NULL;
    }
    RETURN;
}
#undef RETURN

/* 
 * xf86DelControlledResource() -- remove device from access 
 * control, enable access to resources if second argument is 
 * true otherwise disable it.
 */

void
xf86DelControlledResource(xf86ScrnAccessPtr pScAcc, Bool enable)
{
    xf86AccessPtr pAcc = pScAcc->pAccess;
    if(pAcc) {
	if(enable)
	    if (pAcc->AccessEnable)
		(*pAcc->AccessEnable)(pAcc->arg);
	    else
		if (pAcc->AccessDisable)
		    (*pAcc->AccessDisable)(pAcc->arg);
	switch (pScAcc->rt) {
	case IO:
	    pScAcc->CurrentAccess->pIoAccess = NULL;
	    break;
	case MEM:
	    pScAcc->CurrentAccess->pMemAccess = NULL;
	    break;
	case MEM_IO:
	    pScAcc->CurrentAccess->pIoAccess =
		pScAcc->CurrentAccess->pMemAccess = NULL;
	    break;
	case NONE:
	    break;
	}
	pScAcc->pAccess = NULL;
    }
    pScAcc->CurrentAccess = NULL;
    pScAcc->rt = RES_NONE;
}

/*
 * xf86EnableAccess() -- enable access to controlled resources.
 */
void
xf86EnableAccess(xf86ScrnAccessPtr pScAcc)
{
    register xf86AccessPtr pAcc;
    /* Screen is not under access control or currently enabled */
    if (!pScAcc->pAccess) return;
    switch (pScAcc->rt) {
    case IO:
	pAcc = pScAcc->CurrentAccess->pIoAccess;
	if (pScAcc->pAccess == pAcc)  return;
	if (pAcc && pAcc->AccessDisable) 
	    (*pAcc->AccessDisable)(pAcc->arg);
	if (pScAcc->CurrentAccess->pMemAccess == pAcc)
	    pScAcc->CurrentAccess->pMemAccess = NULL;
	pAcc = pScAcc->pAccess;
	if (pAcc && pAcc->AccessEnable) 
	    (*pAcc->AccessEnable)(pAcc->arg);
	pScAcc->CurrentAccess->pIoAccess = pAcc;
	return;
	
    case MEM_IO:
	pAcc = pScAcc->CurrentAccess->pIoAccess;
	if (pScAcc->pAccess == pAcc)  return;
	if (pAcc && pAcc->AccessDisable) 
	    (*pAcc->AccessDisable)(pAcc->arg);
	pAcc = pScAcc->CurrentAccess->pIoAccess;
	if (pAcc && pAcc->AccessDisable) 
	    (*pAcc->AccessDisable)(pAcc->arg);
	pAcc = pScAcc->pAccess;
	if (pAcc && pAcc->AccessEnable) 
	    (*pAcc->AccessEnable)(pAcc->arg);
	pScAcc->CurrentAccess->pIoAccess = pAcc;
	pScAcc->CurrentAccess->pMemAccess = pAcc;
	return;
	
    case MEM:
	pAcc = pScAcc->CurrentAccess->pMemAccess;
	if (pScAcc->pAccess == pAcc)  return;
	if (pAcc && pAcc->AccessDisable) 
	    (*pAcc->AccessDisable)(pAcc->arg);
	if (pScAcc->CurrentAccess->pIoAccess == pAcc)
	    pScAcc->CurrentAccess->pIoAccess = NULL;
	pAcc = pScAcc->pAccess;
	if (pAcc && pAcc->AccessEnable) 
	    (*pAcc->AccessEnable)(pAcc->arg);
	pScAcc->CurrentAccess->pMemAccess = pAcc;
	return;

    case NONE:
	break;
    }
}

/*
 * xf86DisableAccess() -- Disable access to VGA resources of _all_
 * video devices on buses which allow this.
 */
static void 
xf86DisableAccess(void)
{
#if 0 /* kraxel */
    return;
#endif
    DisablePciAccess();
}

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
    xf86DisableAccess();

    /* if no VGA device is found check for primary PCI device */
    CheckGenericGA();
    if (primaryBus == BUS_NONE)
	FindPciPrimaryDevice();
}

#include "vgaHW.h"
#include "compiler.h"

/*
 * CheckGenericGA() - Check for presence of a VGA device.
 */
static void
CheckGenericGA()
{
#ifndef __powerpc__ /* FIXME ?? */
    CARD16 GenericIOBase = VGAHW_GET_IOBASE();
    CARD8 CurrentValue, TestValue;

    /* Unlock VGA registers */
    VGAHW_UNLOCK(GenericIOBase);

    /* VGA has one more read/write attribute register than EGA */
    (void) inb(GenericIOBase + 0x0AU);  /* Reset flip-flop */
    outb(0x3C0, 0x14 | 0x20);
    CurrentValue = inb(0x3C1);
    outb(0x3C0, CurrentValue ^ 0x0F);
    outb(0x3C0, 0x14 | 0x20);
    TestValue = inb(0x3C1);
    outb(0x3C0, CurrentValue);

    /* XXX:  This should restore lock state, rather than relock */
    VGAHW_LOCK(GenericIOBase);

    if ((CurrentValue ^ 0x0F) == TestValue) {
        Resources.exclusive_Vga = 1;
	primaryBus = BUS_ISA;
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
	    if (pcp->_base_class == PCI_CLASS_PREHISTORIC &&
		pcp->_sub_class == PCI_SUBCLASS_PREHISTORIC_VGA)
		return PCI_CHIP_VGA ;
	    if (pcp->_base_class == PCI_CLASS_DISPLAY &&
		pcp->_sub_class == PCI_SUBCLASS_DISPLAY_VGA) {
		if (pcp->_prog_if == 0)
		    return PCI_CHIP_VGA ; 
		if (pcp->_prog_if == 1)
		    return PCI_CHIP_8514;
	    }
	    return -1;
	}
    i++;
    }
    return -1;
}

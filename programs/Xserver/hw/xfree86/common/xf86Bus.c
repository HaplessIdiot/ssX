/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Bus.c,v 1.1.2.21 1998/07/19 13:21:50 dawes Exp $ */

/*
 * Copyright (c) 1997,1998 by The XFree86 Project, Inc.
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

#include "xf86Bus.h"
#include "xf86Pci.h"
#define INIT_PCI_VENDOR_INFO
#include "xf86PciInfo.h"

/* Bus-specific globals */

pciConfigPtr *xf86PciInfo = NULL;		/* Full PCI probe info */
pciVideoPtr *xf86PciVideoInfo = NULL;		/* PCI probe for video hw */

/* Local bus data */

static BusPtr *xf86BusSlots = NULL;	/* Bus slots claimed by drivers */
static int xf86NumBusSlots = 0;

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
	    xf86PciVideoInfo = (pciVideoPtr *)xnfrealloc(xf86PciVideoInfo,
					      sizeof(pciVideoPtr) * (num + 1));
	    xf86PciVideoInfo[num] = NULL;
	    info = xf86PciVideoInfo[num - 1] =
		(pciVideoPtr)xnfalloc(sizeof(pciVideoRec));
	    info->vendor = pcrp->_vendor;
	    info->chipType = pcrp->_device;
	    info->chipRev = pcrp->_rev_id;
	    info->bus = pcrp->busnum;
	    info->device = pcrp->devnum;
	    info->func = pcrp->funcnum;
	    info->class = pcrp->_base_class;
	    info->subclass = pcrp->_sub_class;
	    info->interface = pcrp->_prog_if;
	    info->biosBase = pcrp->_baserom;
	    info->thisCard = pcrp;
	    for (j = 0; j < 6; j++) {
		info->memBase[j] = 0;
		info->ioBase[j] = 0;
	    }

	    /*
	     * 64-bit base addresses are checked for and avoided.
	     * XXX Should deal with them on platforms that support them.
	     */

	    if (pcrp->_base0) {
		if (pcrp->_base0 & PCI_MAP_IO)
		    info->ioBase[0] = PCIGETIO(pcrp->_base0);
		else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base0))
			mem64 = TRUE;
		    else
			info->memBase[0] = PCIGETMEMORY(pcrp->_base0);
	    }

	    if (pcrp->_base1 && !mem64) {
		if (pcrp->_base1 & PCI_MAP_IO)
		    info->ioBase[1] = PCIGETIO(pcrp->_base1);
		else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base1))
			mem64 = TRUE;
		    else
			info->memBase[1] = PCIGETMEMORY(pcrp->_base1);
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->_base2 && !mem64) {
		if (pcrp->_base2 & PCI_MAP_IO)
		    info->ioBase[2] = PCIGETIO(pcrp->_base2);
		else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base2))
			mem64 = TRUE;
		    else
			info->memBase[2] = PCIGETMEMORY(pcrp->_base2);
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->_base3 && !mem64) {
		if (pcrp->_base3 & PCI_MAP_IO)
		    info->ioBase[3] = PCIGETIO(pcrp->_base3);
		else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base3))
			mem64 = TRUE;
		    else
			info->memBase[3] = PCIGETMEMORY(pcrp->_base3);
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->_base4 && !mem64) {
		if (pcrp->_base4 & PCI_MAP_IO)
		    info->ioBase[4] = PCIGETIO(pcrp->_base4);
		else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base4))
			mem64 = TRUE;
		    else
			info->memBase[4] = PCIGETMEMORY(pcrp->_base4);
	    } else if (mem64)
		mem64 = FALSE;

	    if (pcrp->_base5 && !mem64) {
		if (pcrp->_base5 & PCI_MAP_IO)
		    info->ioBase[5] = PCIGETIO(pcrp->_base5);
		else
		    if (PCI_MAP_IS64BITMEM(pcrp->_base5))
			mem64 = TRUE;
		    else
			info->memBase[5] = PCIGETMEMORY(pcrp->_base5);
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
	    while(xf86PCIVendorInfo[i].VendorName) {
		if (xf86PCIVendorInfo[i].VendorID == info->vendor) {
		    j = 0;
		    vendorname = xf86PCIVendorInfo[i].VendorName;
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
		    ErrorF("0x%08x", info->memBase[i]);
		}
	    }
	    for (i = 0; i < 6; i++) {
		if (info->ioBase[i]) {
		    if (!iodone) {
			ErrorF(", I/O @ ");
			iodone = TRUE;
		    } else
			ErrorF(", ");
		    ErrorF("0x%04x", info->ioBase[i]);
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
    xf86FindPCIVideoInfo();
}

/*
 * Functions to access the bus list.  Perhaps it might be best to keep
 * the data structures opaque to the video drivers, requiring that they
 * manipulate them via functions here.
 */

static BusPtr
xf86AllocateBusSlot(void)
{
    xf86NumBusSlots++;
    xf86BusSlots = (BusPtr *)xnfrealloc(xf86BusSlots,
					sizeof(BusPtr) * xf86NumBusSlots);
    xf86BusSlots[xf86NumBusSlots - 1] = (BusPtr)xnfalloc(sizeof(BusRec));
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
xf86CheckPciSlot(int bus, int device, int func, PciBusType type)
{
    int i;
    BusPtr p;

    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	/* Check if this PCI slot is taken */
	if (p->busType == BUS_PCI && p->pciBusId.bus == bus &&
	    p->pciBusId.device == device && p->pciBusId.func == func)
	    return FALSE;
	/* If VGA access is requested, check if that is alreay taken */
	if (type == PCI_VGA) {
	    if (p->busType == BUS_ISA && p->isaBusId == ISA_COLOR)
		return FALSE;
	    if (p->busType == BUS_PCI &&
		(p->pciBusId.type == PCI_VGA ||
		 p->pciBusId.type == PCI_SHARED_VGA))
		return FALSE;
	} else if (type == PCI_SHARED_VGA) {
	    if (p->busType == BUS_ISA && p->isaBusId == ISA_COLOR)
		return FALSE;
	    if (p->busType == BUS_PCI && p->pciBusId.type == PCI_VGA)
		return FALSE;
	}

    }
    return TRUE;
}

/*
 * If the slot requested is already in use, return FALSE.
 * Otherwise, claim the slot for the screen requesting it.
 */

Bool
xf86ClaimPciSlot(int bus, int device, int func, PciBusType type, int scrnIndex)
{
    BusPtr p;

    if (xf86CheckPciSlot(bus, device, func, type)) {
	p = xf86AllocateBusSlot();
	p->busType = BUS_PCI;
	p->scrnIndex = scrnIndex;
	p->pciBusId.bus = bus;
	p->pciBusId.device = device;
	p->pciBusId.func = func;
	p->pciBusId.type = type;
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
			PciBusType **pType)
{
    int num = 0;
    int i;
    BusPtr p;
    pciVideoPtr *ppPci;

    if (xf86PciVideoInfo == NULL)
	return 0;

    if (pPciList != NULL)
	*pPciList = NULL;
    if (pType != NULL)
	*pType = NULL;
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
		    *pPciList = (pciVideoPtr *)xnfrealloc(*pPciList,
						num * sizeof(pciVideoPtr));
		    (*pPciList)[num - 1] = *ppPci;
		}
		if (pType != NULL) {
		    *pType = (PciBusType *)xnfrealloc(*pType,
						num * sizeof(PciBusType));
		    (*pType)[num - 1] = p->pciBusId.type;
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
    if (xf86NameCmp(p, "pci") || xf86NameCmp(p, "agp"))
	ret = BUS_PCI;
    if (xf86NameCmp(p, "isa"))
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
xf86CheckIsaSlot(IsaBusType type)
{
    int i;
    BusPtr p;

    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	if (type == ISA_COLOR && p->busType == BUS_PCI &&
	    (p->pciBusId.type == PCI_VGA || p->pciBusId.type == PCI_SHARED_VGA))
	    return FALSE;
	if (p->busType == BUS_ISA && type == p->isaBusId)
	    return FALSE;
    }
    return TRUE;
}

/*
 * If the slot requested is already in use, return FALSE.
 * Otherwise, claim the slot for the screen requesting it.
 */

Bool
xf86ClaimIsaSlot(IsaBusType type, int scrnIndex)
{
    BusPtr p;

    if (xf86CheckIsaSlot(type)) {
	p = xf86AllocateBusSlot();
	p->busType = BUS_ISA;
	p->scrnIndex = scrnIndex;
	p->isaBusId = type;
	return TRUE;
    } else
	return FALSE;
}

/*
 * Release the slot for a particular ISA type
 */

void
xf86ReleaseIsaSlot(IsaBusType type)
{
    int i;
    BusPtr p;

    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	if (p->busType == BUS_ISA && p->isaBusId == type) {
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
xf86GetIsaInfoForScreen(int scrnIndex, IsaBusType **pType)
{
    int num = 0;
    int i;
    BusPtr p;

    if (pType != NULL)
	*pType = NULL;
    for (i = 0; i < xf86NumBusSlots; i++) {
	p = xf86BusSlots[i];
	if (p->scrnIndex == scrnIndex && p->busType == BUS_ISA) {
	    num++;
	    if (pType != NULL) {
		*pType = (IsaBusType *)xnfrealloc(*pType,
						num * sizeof(IsaBusType));
		(*pType)[num - 1] = p->isaBusId;
	    }
	}
    }
    return num;
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


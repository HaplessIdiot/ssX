/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/vga/vgaPCI.c,v 3.17 1997/10/25 13:50:55 hohndel Exp $ */
/*
 * PCI Probe
 *
 * Copyright 1995  The XFree86 Project, Inc.
 *
 * A lot of this comes from Robin Cutshaw's scanpci
 *
 */
/* $XConsortium: vgaPCI.c /main/10 1996/10/25 10:34:22 kaleb $ */

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "xf86_Config.h"
#include "vga.h"

#define INIT_PCI_VENDOR_INFO
#include "vgaPCI.h"

vgaPCIInformation *
vgaGetPCIInfo()
{
    vgaPCIInformation *info = NULL;
    pciConfigPtr pcrp, *pcrpp;
    Bool found = FALSE;
    int i = 0;
    CARD32 membase2;

    pcrpp = xf86scanpci(vga256InfoRec.scrnIndex);

    if (!pcrpp)
	return NULL;

    while ((pcrp = pcrpp[i])) {
#if !defined(PC98_TGUI) && !defined(PC98_MGA)
	if ((pcrp->_base_class == PCI_CLASS_PREHISTORIC &&
	     pcrp->_sub_class == PCI_SUBCLASS_PREHISTORIC_VGA) ||
	    (pcrp->_base_class == PCI_CLASS_DISPLAY &&
	     pcrp->_sub_class == PCI_SUBCLASS_DISPLAY_VGA)) {
#else /* PC98 */
#ifdef PC98_TGUI
	if (pcrp->_base_class == PCI_CLASS_DISPLAY &&
	    pcrp->_sub_class == PCI_SUBCLASS_DISPLAY_VGA &&
	    pcrp->_vendor == PCI_VENDOR_TRIDENT) {
#endif
#ifdef PC98_MGA
	if (pcrp->_base_class == PCI_CLASS_DISPLAY &&
	    pcrp->_sub_class == PCI_SUBCLASS_DISPLAY_MISC &&
	    pcrp->_vendor == PCI_VENDOR_MATROX) {
#endif
#endif /* PC98 */
	    found = TRUE;
	    if ((info = (vgaPCIInformation *)
		 xalloc(sizeof(vgaPCIInformation))) == NULL)
		return NULL;
	    info->Vendor = pcrp->_vendor;
	    info->ChipType = pcrp->_device;
	    info->ChipRev = pcrp->_rev_id;
	    info->Tag = pcrp->tag;
	    info->Bus = pcrp->_bus;
	    info->Card = pcrp->_cardnum;
	    info->Func = pcrp->_func;
	    info->AllCards = pcrpp;
	    info->ThisCard = pcrp;
	    info->MemBase = 0;
	    info->MMIOBase = 0;
	    info->IOBase = 0;
	    membase2 = 0;

	    if (pcrp->_base0) {
		if (pcrp->_base0 & 1)
		    info->IOBase = pcrp->_base0 & 0xFFFFFFFC;
		else if (pcrp->_base0 & 0x3FFF0)
		    info->MMIOBase = pcrp->_base0 & 0xFFFFFFF0;
		else
		    info->MemBase = pcrp->_base0 & 0xFFFFFFF0;
	    }
	    if (pcrp->_base1) {
		if (pcrp->_base1 & 1) {
		    if (!info->IOBase)
			info->IOBase = pcrp->_base1 & 0xFFFFFFFC;
		} else if (pcrp->_base1 & 0x3FFF0) {
		    if (!info->MMIOBase)
			info->MMIOBase = pcrp->_base1 & 0xFFFFFFF0;
		} else {
		    if (!info->MemBase)
			info->MemBase = pcrp->_base1 & 0xFFFFFFF0;
		    else
			membase2 = pcrp->_base1 & 0xFFFFFFF0;
		}
	    }
	    if (pcrp->_base2) {
		if (pcrp->_base2 & 1) {
		    if (!info->IOBase)
			info->IOBase = pcrp->_base2 & 0xFFFFFFFC;
		} else if (pcrp->_base2 &0x3FFF0) {
		    if (!info->MMIOBase)
			info->MMIOBase = pcrp->_base2 & 0xFFFFFFF0;
		} else {
		    if (!info->MemBase)
			info->MemBase = pcrp->_base2 & 0xFFFFFFF0;
		    else if (!membase2)
			membase2 = pcrp->_base2 & 0XFFFFFFF0;
		}
	    }
	    if (pcrp->_base3) {
		if (pcrp->_base3 & 1) {
		    if (!info->IOBase)
			info->IOBase = pcrp->_base3 & 0xFFFFFFFC;
		} else if (pcrp->_base3 &0x3FFF0) {
		    if (!info->MMIOBase)
			info->MMIOBase = pcrp->_base3 & 0xFFFFFFF0;
		} else {
		    if (!info->MemBase)
			info->MemBase = pcrp->_base3 & 0xFFFFFFF0;
		    else if (!membase2)
			membase2 = pcrp->_base3 & 0XFFFFFFF0;
		}
	    }
	    if (pcrp->_base4) {
		if (pcrp->_base4 & 1) {
		    if (!info->IOBase)
			info->IOBase = pcrp->_base4 & 0xFFFFFFFC;
		} else if (pcrp->_base4 &0x3FFF0) {
		    if (!info->MMIOBase)
			info->MMIOBase = pcrp->_base4 & 0xFFFFFFF0;
		} else {
		    if (!info->MemBase)
			info->MemBase = pcrp->_base4 & 0xFFFFFFF0;
		    else if (!membase2)
			membase2 = pcrp->_base4 & 0XFFFFFFF0;
		}
	    }
	    if (pcrp->_base5) {
		if (pcrp->_base5 & 1) {
		    if (!info->IOBase)
			info->IOBase = pcrp->_base5 & 0xFFFFFFFC;
		} else if (pcrp->_base5 &0x3FFF0) {
		    if (!info->MMIOBase)
			info->MMIOBase = pcrp->_base5 & 0xFFFFFFF0;
		} else {
		    if (!info->MemBase)
			info->MemBase = pcrp->_base5 & 0xFFFFFFF0;
		    else if (!membase2)
			membase2 = pcrp->_base5 & 0XFFFFFFF0;
		}
	    }
	    break;
	}
	i++;
    }
    if (found && xf86Verbose) {
	int i = 0, j;
	char *vendorname = NULL, *chipname = NULL;

	while (xf86PCIVendorInfo[i].VendorName) {
	    if (xf86PCIVendorInfo[i].VendorID == info->Vendor) {
		j = 0;
		vendorname = xf86PCIVendorInfo[i].VendorName;
		while (xf86PCIVendorInfo[i].Device[j].DeviceName) {
		    if (xf86PCIVendorInfo[i].Device[j].DeviceID ==
			info->ChipType) {
			chipname = xf86PCIVendorInfo[i].Device[j].DeviceName;
			break;
		    }
		    j++;
		}
		break;
	    }
	    i++;
	}

	ErrorF("%s %s: PCI: ", XCONFIG_PROBED, vga256InfoRec.name);
	if (vendorname)
	    ErrorF("%s ", vendorname);
	else
	    ErrorF("Unknown vendor (0x%04x) ", info->Vendor);
	if (chipname)
	    ErrorF("%s ", chipname);
	else
	    ErrorF("Unknown chipset (0x%04x) ", info->ChipType);
	ErrorF("rev 0x%x", info->ChipRev);

	if (info->MemBase)
	    ErrorF(", Memory @ 0x%08x", info->MemBase);
	if (membase2)
	    ErrorF(", 0x%08x", membase2);
	if (info->MMIOBase)
	    ErrorF(", MMIO @ 0x%08x", info->MMIOBase);
	if (info->IOBase)
	    ErrorF(", I/O @ 0x%04x", info->IOBase);
	ErrorF("\n");
    }
    return info;
}

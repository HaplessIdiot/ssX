/* $XFree86: $ */
/*
 * Display the Subsystem Vendor Id and Subsystem Id in order to identify
 * the cards installed in this computer
 *
 * Copyright 1995-1998 by The XFree86 Project, Inc.
 *
 * A lot of this comes from Robin Cutshaw's scanpci
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Pci.h"

#define INIT_PCI_CARD_INFO TRUE
#ifdef XFree86LOADER
#define INIT_PCI_VENDOR_INFO TRUE
#endif
#include "xf86PciInfo.h"

/*
 * PCI classes that have messages printed always.  The others are only
 * have a message printed when the vendor/dev IDs are recognised.
 */
#define PCIALWAYSPRINTCLASSES(b,s)					      \
    (((b) == PCI_CLASS_PREHISTORIC && (s) == PCI_SUBCLASS_PREHISTORIC_VGA) || \
     ((b) == PCI_CLASS_DISPLAY) ||					      \
     ((b) == PCI_CLASS_MULTIMEDIA && (s) == PCI_SUBCLASS_MULTIMEDIA_VIDEO))


MODULESETUPPROTO(scanPciSetup);
void xf86DisplayPCICardInfo(void);


#ifdef XFree86LOADER

#include "xf86Module.h"

static XF86ModuleVersionInfo scanPciVersRec = {
	"scanpci",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0, 1, 0,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	NULL,
	{0, 0, 0, 0}
};

XF86ModuleData scanpciModuleData = { &scanPciVersRec, scanPciSetup, NULL };

pointer
scanPciSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    xf86ScanPciRegister(xf86DisplayPCICardInfo);

    return (pointer)1;
}

#endif /* XFree86LOADER */


void
xf86DisplayPCICardInfo(void)
{
    pciConfigPtr pcrp, *pcrpp;
    int i = 0, j,k;
    int verbosity = 2;
    pciVendorCardInfo *info;
    Bool mem64 = FALSE;

    xf86EnableIO();
    pcrpp = xf86scanpci(0);
   
    if (pcrpp == NULL) {
	return;
    }
    while ((pcrp = pcrpp[i])) {
	char *vendorname = NULL, *cardname = NULL;
	char *chipvendorname = NULL, *chipname = NULL;

	/* first let's look for the card itself */
	k = 0;
	while(xf86PCICardInfo[k].VendorName) {
	    if (xf86PCICardInfo[k].VendorID == pcrp->_subsys_vendor) {
		j = 0;
		vendorname = xf86PCICardInfo[k].VendorName;
		while (xf86PCICardInfo[k].Device[j].CardName) {
		    if (xf86PCICardInfo[k].Device[j].SubsystemID ==
			pcrp->_subsys_card) {
			cardname =
			  xf86PCICardInfo[k].Device[j].CardName;
			break;
		    }
		    j++;
		}
		break;
	    }
	    k++;
	}
#if DONT_PRINT_ALL_DEVICES
	if ((!vendorname || !cardname) &&
	    !PCIALWAYSPRINTCLASSES(pcrp->_base_class, 
				   pcrp->_sub_class)) {
	    i++;
	    continue;
	}
#endif /* DONT_PRINT_ALL_DEVICES */
	if (!vendorname || !cardname)
	  verbosity = 0;
	xf86Msg(X_PROBED, "PCI: (%d:%d:%d) ", pcrp->busnum, pcrp->devnum,
		pcrp->funcnum);
	if (vendorname)
	  xf86Msg( X_NONE,"%s ", vendorname);
	if (cardname)
	  xf86Msg( X_NONE,"%s ", cardname);
	else
	  xf86Msg( X_NONE,"unknown card (0x%04x/0x%04x) ", pcrp->_subsys_vendor,
			  pcrp->_subsys_card);

	/* now check for the chipset used */
	k = 0;
	while(xf86PCIVendorInfo[k].VendorName) {
	    if (xf86PCIVendorInfo[k].VendorID == pcrp->_vendor) {
		j = 0;
		chipvendorname = xf86PCIVendorInfo[k].VendorName;
		while (xf86PCIVendorInfo[k].Device[j].DeviceName) {
		    if (xf86PCIVendorInfo[k].Device[j].DeviceID ==
			pcrp->_device) {
			chipname =
			  xf86PCIVendorInfo[k].Device[j].DeviceName;
			break;
		    }
		    j++;
		}
		break;
	    }
	    k++;
	}
	if (chipvendorname && chipname)
	  xf86MsgVerb(X_NONE,verbosity,"using a %s %s",
		      chipvendorname,chipname);
	else if (chipvendorname)
	  xf86MsgVerb(X_NONE,verbosity,
		      "\n\tusing an unknown chip (DeviceId 0x%04x) from %s",
		      pcrp->_device,chipvendorname);
	else
	  xf86MsgVerb(X_NONE,verbosity,
		      "\n\tusing an unknown chipset(0x%04x/0x%04x)",
		      pcrp->_vendor,pcrp->_device);
	xf86Msg(X_NONE,"\n");
	i++;
    }
}



/* $XFree86: xc/programs/Xserver/hw/xfree86/scanpci/xf86ScanPci.c,v 1.2 1999/02/13 16:44:59 hohndel Exp $ */
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
#define DECLARE_CARD_DATASTRUCTURES TRUE
#define INIT_PCI_VENDOR_INFO TRUE
#define VENDOR_INCLUDE_NONVIDEO TRUE
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
void xf86DisplayPCICardInfo(int);


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

XF86ModuleData scanpciModuleData = { &scanPciVersRec, NULL, NULL };

#else

/*
 * setup the pci data structures for the static server
 */
void
xf86InitPciData(void)
{
    xf86PCIVendorInfo = xf86PCIVendorInfoData;
    xf86PCICardInfo = xf86PCICardInfoData;
}

#endif /* XFree86LOADER */


void
xf86DisplayPCICardInfo(int verbosity)
{
    pciConfigPtr pcrp, *pcrpp;
    int i = 0, j,k;
    pciVendorCardInfo *info;
    Bool noCard = FALSE;

    xf86EnableIO();
    pcrpp = xf86scanpci(0);
   
    if (pcrpp == NULL) {
        xf86MsgVerb(X_NONE,0,"No PCI info available\n");
	return;
    }
    xf86MsgVerb(X_NONE,0,"Probing for PCI devices (Bus:Device:Function)\n\n");
    while ((pcrp = pcrpp[i])) {
	char *vendorname = NULL, *cardname = NULL;
	char *chipvendorname = NULL, *chipname = NULL;
	Bool noCard = FALSE;

	xf86MsgVerb(X_NONE,-verbosity,
		    "(%d:%d:%d) ", pcrp->busnum, pcrp->devnum,
		pcrp->funcnum);

	/* first let's look for the card itself, but only if information
	 * is available
	 */
	if ( pcrp->_subsys_vendor || pcrp->_subsys_card ) {
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
	    if (vendorname)
		xf86MsgVerb(X_NONE,-verbosity,"%s ", vendorname);
	    if (cardname)
		xf86MsgVerb(X_NONE,-verbosity,"%s ", cardname);
	    if (vendorname && !cardname)
		xf86MsgVerb(X_NONE,-verbosity,"unknown card ");
	}
	if (!(cardname || vendorname)) {
	    /*
	     * we didn't find text representation of the information 
	     * about the card
	     */
	    if ( pcrp->_subsys_vendor || pcrp->_subsys_card ) {
		/*
		 * if there was information and we just couldn't interpret
		 * it, print it out as unknown, anyway
		 */
		xf86MsgVerb(X_NONE,-verbosity,
			    "unknown card (0x%04x/0x%04x) \n\t",
			    pcrp->_subsys_vendor, pcrp->_subsys_card);
	    }
	    else {
		/*
		 * if there was no info to begin with, only print in
		 * verbose mode
		 */
		if (verbosity > 1)
		    xf86MsgVerb(X_NONE,-verbosity,
				"unknown card (0x%04x/0x%04x) \n\t",
				pcrp->_subsys_vendor, pcrp->_subsys_card);
		else
		    noCard = TRUE;
	    }
	}
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
	if (noCard) {
	  if (chipvendorname && chipname)
	    xf86MsgVerb(X_NONE,-verbosity,"%s %s",
			chipvendorname,chipname);
	  else if (chipvendorname)
	    xf86MsgVerb(X_NONE,-verbosity,
			"unknown chip (DeviceId 0x%04x) from %s",
			pcrp->_device,chipvendorname);
	  else
	    xf86MsgVerb(X_NONE,-verbosity,
			"unknown chipset(0x%04x/0x%04x)",
			pcrp->_vendor,pcrp->_device);
	  xf86MsgVerb(X_NONE,-verbosity,"\n");
	}
	else
	{
	  if (chipvendorname && chipname)
	    xf86MsgVerb(X_NONE,-verbosity,"using a %s %s",
			chipvendorname,chipname);
	  else if (chipvendorname)
	    xf86MsgVerb(X_NONE,-verbosity,
			"using an unknown chip (DeviceId 0x%04x) from %s",
			pcrp->_device,chipvendorname);
	  else
	    xf86MsgVerb(X_NONE,-verbosity,
			"using an unknown chipset(0x%04x/0x%04x)",
			pcrp->_vendor,pcrp->_device);
	  xf86MsgVerb(X_NONE,-verbosity,"\n");
	}
	i++;
    }
}



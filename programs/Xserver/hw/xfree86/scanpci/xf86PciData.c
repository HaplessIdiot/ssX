/* $XFree86: xc/programs/Xserver/hw/xfree86/scanpci/xf86ScanPci.c,v 1.1 1999/02/12 22:52:12 hohndel Exp $ */
/*
 * the PCI data structures
 *
 * this module only includes the data that is relevant for video boards
 * the non-video data is included in the scanpci module
 *
 * Copyright 1995-1999 by The XFree86 Project, Inc.
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
#ifdef XFree86LOADER
#define INIT_PCI_VENDOR_INFO TRUE
#endif
#include "xf86PciInfo.h"

#ifdef XFree86LOADER

#include "xf86Module.h"

static XF86ModuleVersionInfo pciDataVersRec = {
	"pcidata",
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

XF86ModuleData pcidataModuleData = { &pciDataVersRec, NULL, NULL };

#endif /* XFree86LOADER */


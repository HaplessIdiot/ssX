/* $XFree86: $ */
#ifndef _XF86_PCI_H_
#define _XF86_PCI_H_

/*
 * xf86_PCI.h has been superceded by Pci.h.   This stub should remain
 * in place until all the drivers have been to including "Pci.h" directly
 */
#include "Pci.h"

#define _bus     busnum
#define _cardnum devnum
#define _func    funcnum

#define pciConfigRec pciDevice

#endif

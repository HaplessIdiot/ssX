/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/chips/ct_pci.c,v 3.0 1996/08/11 13:02:52 dawes Exp $ */

#include "X.h"
#include "input.h"
#include "screenint.h"

/*
 * These are XFree86-specific header files
 */
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "xf86_PCI.h"

#include "vga.h"
#include "vgaPCI.h"
#include "xf86_Config.h"

int 
ctPCIMemBase(ctisHiQV32)
    Bool ctisHiQV32;
{

    if (vgaPCIInfo && vgaPCIInfo->Vendor == PCI_VENDOR_CHIPSTECH) {
	if (vgaPCIInfo->MemBase != 0) {
	    return vgaPCIInfo->MemBase & 0xFF800000;
	} else {
	    ErrorF("%s %s: %s: Can't find valid PCI "
		"Base Address\n", XCONFIG_PROBED,
		vga256InfoRec.name, vga256InfoRec.chipset);
	    return -1;
	}
    } else {
	ErrorF("%s %s: %s: Can't find PCI device in "
	    "configuration space\n", XCONFIG_PROBED,
	    vga256InfoRec.name, vga256InfoRec.chipset);
	return -1;
    }
}

unsigned int 
ctPCIIOBase()
{
  if (vgaPCIInfo->IOBase != 0) {
    return vgaPCIInfo->IOBase & 0xFFFFFC00;
  } else {
    ErrorF("%s %s: %s: Can't find valid PCI "
	   "IOBase Address\n", XCONFIG_PROBED,
	   vga256InfoRec.name, vga256InfoRec.chipset);
    return 0;
  }
}

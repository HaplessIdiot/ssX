/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/ix86Pci.c,v 1.5 1998/09/27 04:43:41 dawes Exp $ */
/*
 * ix86Pci.c - x86 PCI driver
 *
 * The Xfree server PCI access functions have been reimplemented as a
 * framework that allows each supported platform/OS to have their own
 * platform/OS specific PCI driver.
 *
 * Most of the code of these functions was simply lifted from the
 * Intel architecture specifric portion of the original Xfree86
 * PCI code in hw/xfree86/common_hw/xf86_PCI.C...
 *
 * Gary Barton
 * Concurrent Computer Corporation
 * garyb@gate.net
 */

/*
 * Copyright 1998 by Concurrent Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Concurrent Computer
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Concurrent Computer Corporation makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * CONCURRENT COMPUTER CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CONCURRENT COMPUTER CORPORATION BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Copyright 1998 by Metro Link Incorporated
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Metro Link
 * Incorporated not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Metro Link Incorporated makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * METRO LINK INCORPORATED DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL METRO LINK INCORPORATED BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * This software is derived from the original XFree86 PCI code
 * which includes the following copyright notices as well:
 *
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holder(s)
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holder(s) make(s) no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM(S) ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * This code is also based heavily on the code in FreeBSD-current, which was
 * written by Wolfgang Stanglmeier, and contains the following copyright:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "Pci.h"

#ifdef PC98
#define outb(port,data) _outb(port,data)
#define outl(port,data) _outl(port,data)
#define inb(port) _inb(port)
#define inl(port) _inl(port)
#endif

typedef union {
    CARD32 cfg1;
    struct {
	CARD8  enable;
	CARD8  forward;
	CARD16 port;
    } cfg2;
    CARD32 tag;
} pciTagRec;

#define	PCI_CFGMECH2_ENABLE_REG		0xCF8
#ifdef PC98
#define	PCI_CFGMECH2_FORWARD_REG	0xCF9
#else
#define	PCI_CFGMECH2_FORWARD_REG	0xCFA
#endif

#define PCI_CFGMECH2_MAXDEV	16

/*
 * Intel x86 platform specific PCI access functions
 */
CARD32 ix86PciReadLong(PCITAG tag, int off);
void ix86PciWriteLong(PCITAG, int off, CARD32 val);
void ix86PciSetBitsLong(PCITAG, int off, CARD32 mask, CARD32 val);

pciBusInfo_t ix86Pci0 = {
/* configMech  */	  PCI_CFG_MECH_UNKNOWN, /* Set by ix86PciInit() */
/* numDevices  */	  0,                    /* Set by ix86PciInit() */
/* secondary   */	  FALSE,
/* primary_bus */	  0,
/* ppc_io_base */	  0,
/* ppc_io_size */	  0,		  
/* funcs       */	  {
	                    ix86PciReadLong,
			    ix86PciWriteLong,
			    ix86PciSetBitsLong,
			    pciAddrNOOP,
			    pciAddrNOOP
		          },
/* pciBusPriv  */	  NULL
};

static Bool
ix86PciBusCheck(void)
{
    CARD8 device;

    for (device = 0; device < ix86Pci0.numDevices; device++) {
	CARD32 id;
	id = ix86PciReadLong(PCI_MAKE_TAG(0, device, 0), 0);
	if (id && id != 0xffffffff) {
	    return TRUE;
	}
    }
    return 0;
}

static
void ix86PciSelectCfgmech(void)
{
    static Bool beenhere = FALSE;
    CARD32 mode1Res1 = 0, mode1Res2 = 0, oldVal1 = 0;
    CARD8  mode2Res1 = 0, mode2Res2 = 0, oldVal2 = 0;
    int stages = 0;

    if (beenhere)
	return; /* Been there, done that */

    beenhere = TRUE;

    /*
     * Determine if motherboard chipset supports PCI Config Mech 1 or 2
     * We rely on xf86Info.pciFlags to tell which mechanisms to try....
     */
    switch (xf86Info.pciFlags) {

    case PCIProbe1: /* { */

      xf86MsgVerb(X_INFO, 2, "PCI: Probing config type using method 1\n");
      oldVal1 = inl(PCI_CFGMECH1_ADDRESS_REG);

#ifdef DEBUGPCI
      if (xf86Verbose > 2) {
	ErrorF("Checking config type 1:\n"
		"\tinitial value of MODE1_ADDR_REG is 0x%08x\n", oldVal1);
	ErrorF("\tChecking that all bits in mask 0x7f000000 are clear\n");
      }
#endif

      /* Assuming config type 1 to start with */
      if ((oldVal1 & 0x7f000000) == 0) {

	stages |= 0x01;

#ifdef DEBUGPCI
	if (xf86Verbose > 2) {
	    ErrorF("\tValue indicates possibly config type 1\n");
	    ErrorF("\tWriting 32-bit value 0x%08x to MODE1_ADDR_REG\n", PCI_EN);
#if 0
	    ErrorF("\tWriting 8-bit value 0x00 to MODE1_ADDR_REG + 3\n");
#endif
	}
#endif

	ix86Pci0.configMech = PCI_CFG_MECH_1;
	ix86Pci0.numDevices = PCI_CFGMECH1_MAXDEV;

	outl(PCI_CFGMECH1_ADDRESS_REG, PCI_EN);

#if 0
	/*
	 * This seems to cause some Neptune-based PCI machines to switch
	 * from config type 1 to config type 2
	 */
	outb(PCI_CFGMECH1_ADDRESS_REG + 3, 0);
#endif
	mode1Res1 = inl(PCI_CFGMECH1_ADDRESS_REG);

#ifdef DEBUGPCI
	if (xf86Verbose > 2) {
	    ErrorF("\tValue read back from MODE1_ADDR_REG is 0x%08x\n",
			mode1Res1);
	    ErrorF("\tRestoring original contents of MODE1_ADDR_REG\n");
	}
#endif

	outl(PCI_CFGMECH1_ADDRESS_REG, oldVal1);

	if (mode1Res1) {

	    stages |= 0x02;

#ifdef DEBUGPCI
	    if (xf86Verbose > 2) {
		ErrorF("\tValue read back is non-zero, and indicates possible"
			" config type 1\n");
	    }
#endif

	    if (ix86PciBusCheck()) {

#ifdef DEBUGPCI
		if (xf86Verbose > 2)
		    ErrorF("\tBus check Confirms this: ");
#endif

		xf86MsgVerb(X_INFO, 2, "PCI: Config type is 1\n");
		xf86MsgVerb(X_INFO, 3,
			"PCI: stages = 0x%02x, oldVal1 = 0x%08x, mode1Res1"
			" = 0x%08x\n", stages, oldVal1, mode1Res1);
		return;
	    }

#ifdef DEBUGPCI
	    if (xf86Verbose > 2) {
		ErrorF("\tBus check fails to confirm this, continuing type 1"
			" check ...\n");
	    }
#endif

	}

	stages |= 0x04;

#ifdef DEBUGPCI
	if (xf86Verbose > 2) {
	    ErrorF("\tWriting 0xff000001 to MODE1_ADDR_REG\n");
	}
#endif
	outl(PCI_CFGMECH1_ADDRESS_REG, 0xff000001);
	mode1Res2 = inl(PCI_CFGMECH1_ADDRESS_REG);

#ifdef DEBUGPCI
	if (xf86Verbose > 2) {
	    ErrorF("\tValue read back from MODE1_ADDR_REG is 0x%08x\n",
			mode1Res2);
	    ErrorF("\tRestoring original contents of MODE1_ADDR_REG\n");
	}
#endif

	outl(PCI_CFGMECH1_ADDRESS_REG, oldVal1);

	if ((mode1Res2 & 0x80000001) == 0x80000000) {

	    stages |= 0x08;

#ifdef DEBUGPCI
	    if (xf86Verbose > 2) {
		ErrorF("\tValue read back has only the msb set\n"
			"\tThis indicates possible config type 1\n");
	    }
#endif

	    if (ix86PciBusCheck()) {

#ifdef DEBUGPCI
		if (xf86Verbose > 2)
		    ErrorF("\tBus check Confirms this: ");
#endif

		xf86MsgVerb(X_INFO, 2, "PCI: Config type is 1\n");
		xf86MsgVerb(X_INFO, 3,
			"PCI: stages = 0x%02x, oldVal1 = 0x%08x,\n"
			"\tmode1Res1 = 0x%08x, mode1Res2 = 0x%08x\n",
			stages, oldVal1, mode1Res1, mode1Res2);
		return;
	    }

#ifdef DEBUGPCI
	    if (xf86Verbose > 2) {
		ErrorF("\tBus check fails to confirm this.\n");
	    }
#endif

	}
      }
   
      xf86MsgVerb(X_INFO, 3, "PCI: Standard check for type 1 failed.\n");
      xf86MsgVerb(X_INFO, 3, "PCI: stages = 0x%02x, oldVal1 = 0x%08x,\n"
	       "\tmode1Res1 = 0x%08x, mode1Res2 = 0x%08x\n",
	       stages, oldVal1, mode1Res1, mode1Res2);
 
      /* Try config type 2 */
      oldVal2 = inb(PCI_CFGMECH2_ENABLE_REG);
      if ((oldVal2 & 0xf0) == 0) {
	ix86Pci0.configMech = PCI_CFG_MECH_2;
	ix86Pci0.numDevices = PCI_CFGMECH2_MAXDEV;

	outb(PCI_CFGMECH2_ENABLE_REG, 0x0e);
	mode2Res1 = inb(PCI_CFGMECH2_ENABLE_REG);
	outb(PCI_CFGMECH2_ENABLE_REG, oldVal2);

	if (mode2Res1 == 0x0e) {
	    if (ix86PciBusCheck()) {
		xf86MsgVerb(X_INFO, 2, "PCI: Config type is 2\n");
		return;
	    }
	}
      }
      break; /* } */

    case PCIProbe2: /* { */

      /* The scanpci-style detection method */

      xf86MsgVerb(X_INFO, 2, "PCI: Probing config type using method 2\n");

      outb(PCI_CFGMECH2_ENABLE_REG, 0x00);
      outb(PCI_CFGMECH2_FORWARD_REG, 0x00);
      mode2Res1 = inb(PCI_CFGMECH2_ENABLE_REG);
      mode2Res2 = inb(PCI_CFGMECH2_FORWARD_REG);

      if (mode2Res1 == 0 && mode2Res2 == 0) {
	xf86MsgVerb(X_INFO, 2, "PCI: Config type is 2\n");
	ix86Pci0.configMech = PCI_CFG_MECH_2;
	ix86Pci0.numDevices = PCI_CFGMECH2_MAXDEV;
	return;
      }

      oldVal1 = inl(PCI_CFGMECH1_ADDRESS_REG);
      outl(PCI_CFGMECH1_ADDRESS_REG, PCI_EN);
      mode1Res1 = inl(PCI_CFGMECH1_ADDRESS_REG);
      outl(PCI_CFGMECH1_ADDRESS_REG, oldVal1);
      if (mode1Res1 == PCI_EN) {
	xf86MsgVerb(X_INFO, 2, "PCI: Config type is 1\n");
	ix86Pci0.configMech = PCI_CFG_MECH_1;
	ix86Pci0.numDevices = PCI_CFGMECH1_MAXDEV;
	return;
      }
      break; /* } */

    case PCIForceConfig1:

      xf86MsgVerb(X_INFO, 2, "PCI: Forcing config type 1\n");

      ix86Pci0.configMech = PCI_CFG_MECH_1;
      ix86Pci0.numDevices = PCI_CFGMECH1_MAXDEV;
      return;

    case PCIForceConfig2:

      xf86MsgVerb(X_INFO, 2, "PCI: Forcing config type 2\n");

      ix86Pci0.configMech = PCI_CFG_MECH_2;
      ix86Pci0.numDevices = PCI_CFGMECH2_MAXDEV;
      return;

    }

    /* No PCI found */
    xf86MsgVerb(X_INFO, 2, "PCI: No PCI bus found\n");
}

static pciTagRec
ix86PcibusTag(CARD8 bus, CARD8 cardnum, CARD8 func)
{
    pciTagRec tag;

    tag.cfg1 = 0;

    if (func > 7 || cardnum >= pciBusInfo[bus]->numDevices)
	return tag;

    switch (ix86Pci0.configMech) {
    case PCI_CFG_MECH_1:
	    tag.cfg1 = PCI_EN | ((CARD32)bus << 16) |
		       ((CARD32)cardnum << 11) |
		       ((CARD32)func << 8);
	    break;
	    
    case PCI_CFG_MECH_2:
	    tag.cfg2.port    = 0xc000 | ((CARD16)cardnum << 8);
	    tag.cfg2.enable  = 0xf0 | (func << 1);
	    tag.cfg2.forward = bus;
	    break;
    }
    
    return tag;
}

CARD32
ix86PciReadLong(PCITAG Tag, int reg)
{
    pciTagRec tag;
    CARD32    addr, data = 0;
    int       bus, dev, func;

#ifdef DEBUGPCI
ErrorF("ix86PciReadLong 0x%lx, %d\n", Tag, reg);
#endif

    ix86PciSelectCfgmech();

    bus  = PCI_BUS_FROM_TAG(Tag);
    dev  = PCI_DEV_FROM_TAG(Tag);
    func = PCI_FUNC_FROM_TAG(Tag);

    tag = ix86PcibusTag(bus,dev,func);

    switch (ix86Pci0.configMech) {
    case PCI_CFG_MECH_1:
	addr = tag.cfg1 | (reg & 0xfc);
	outl(PCI_CFGMECH1_ADDRESS_REG, addr);
	data = inl(PCI_CFGMECH1_DATA_REG);
	outl(PCI_CFGMECH1_ADDRESS_REG, 0);
	break;
    case PCI_CFG_MECH_2:
	addr = tag.cfg2.port | (reg & 0xfc);
	outb(PCI_CFGMECH2_ENABLE_REG, tag.cfg2.enable);
	outb(PCI_CFGMECH2_FORWARD_REG, tag.cfg2.forward);
	data = inl((CARD16)addr);
	outb(PCI_CFGMECH2_ENABLE_REG, 0);
	outb(PCI_CFGMECH2_FORWARD_REG, 0);
	break;
    }

#ifdef DEBUGPCI
ErrorF("ix86PciReadLong 0x%lx\n", data);
#endif

    return data;
}

void
ix86PciWriteLong(PCITAG Tag, int reg, CARD32 data)
{
    pciTagRec tag;
    CARD32    addr;
    int       bus, dev, func;

    ix86PciSelectCfgmech();

    bus  = PCI_BUS_FROM_TAG(Tag);
    dev  = PCI_DEV_FROM_TAG(Tag);
    func = PCI_FUNC_FROM_TAG(Tag);

    tag = ix86PcibusTag(bus,dev,func);
    
    switch (ix86Pci0.configMech) {
    case PCI_CFG_MECH_1:
	addr = tag.cfg1 | (reg & 0xfc);
	outl(PCI_CFGMECH1_ADDRESS_REG, addr);
	outl(PCI_CFGMECH1_DATA_REG, data);
	outl(PCI_CFGMECH1_ADDRESS_REG, 0);
	break;
    case PCI_CFG_MECH_2:
	addr = tag.cfg2.port | (reg & 0xfc);
	outb(PCI_CFGMECH2_ENABLE_REG, tag.cfg2.enable);
	outb(PCI_CFGMECH2_FORWARD_REG, tag.cfg2.forward);
	outl((CARD16)addr, data);
	outb(PCI_CFGMECH2_ENABLE_REG, 0);
	outb(PCI_CFGMECH2_FORWARD_REG, 0);
	break;
    }
}

void
ix86PciSetBitsLong(PCITAG Tag, int reg, CARD32 mask, CARD32 val)
{
    pciTagRec tag;
    CARD32    addr, data = 0;
    int       bus, dev, func;

#ifdef DEBUGPCI
    ErrorF("ix86PciSetBitsLong 0x%lx, %d\n", Tag, reg);
#endif

    ix86PciSelectCfgmech();

    bus  = PCI_BUS_FROM_TAG(Tag);
    dev  = PCI_DEV_FROM_TAG(Tag);
    func = PCI_FUNC_FROM_TAG(Tag);

    tag = ix86PcibusTag(bus,dev,func);

    switch (ix86Pci0.configMech) {
	case PCI_CFG_MECH_1:
	    addr = tag.cfg1 | (reg & 0xfc);
	    outl(PCI_CFGMECH1_ADDRESS_REG, addr);
	    data = inl(PCI_CFGMECH1_DATA_REG);
	    data = (data & ~mask) | (val & mask);
	    outl(PCI_CFGMECH1_DATA_REG, data);
	    outl(PCI_CFGMECH1_ADDRESS_REG, 0);
	    break;
	case PCI_CFG_MECH_2:
	    addr = tag.cfg2.port | (reg & 0xfc);
	    outb(PCI_CFGMECH2_ENABLE_REG, tag.cfg2.enable);
	    outb(PCI_CFGMECH2_FORWARD_REG, tag.cfg2.forward);
	    data = inl((CARD16)addr);
	    data = (data & ~mask) | (val & mask);
	    outl((CARD16)addr, data);
	    outb(PCI_CFGMECH2_ENABLE_REG, 0);
	    outb(PCI_CFGMECH2_FORWARD_REG, 0);
	    break;
    }
}
void
ix86PciInit()
{
    /* Initialize pciBusInfo[] array and function pointers */
    pciNumBuses    = 1;
    pciBusInfo[0]  = &ix86Pci0;
    pciFindFirstFP = pciGenFindFirst;
    pciFindNextFP  = pciGenFindNext;

    /* Make sure that there is a PCI bus present. */
    ix86PciSelectCfgmech();
    if (ix86Pci0.configMech == PCI_CFG_MECH_UNKNOWN) {
	pciNumBuses    = 0;
	pciBusInfo[0]  = NULL;
    }
}

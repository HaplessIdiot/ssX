/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/xf86_PCI.c,v 3.8 1996/03/10 12:04:45 dawes Exp $ */
/*
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
 */
/* $XConsortium: xf86_PCI.c /main/5 1996/01/10 10:21:13 kaleb $ */

/*#define DEBUGPCI  1 */

#include <stdio.h>
#include "os.h"
#include "compiler.h"
#include "input.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_PCI.h"

#ifdef PC98
#define outb(port,data) _outb(port,data)
#define outl(port,data) _outl(port,data)
#define inb(port) _inb(port)
#define inl(port) _inl(port)
#endif

struct pci_config_reg *pci_devp[MAX_PCI_DEVICES + 1] = {NULL, };

void
xf86scanpci(scrnIndex)
int scrnIndex;
{
    unsigned long tmplong1, tmplong2, config_cmd;
    unsigned char tmp1, tmp2;
    unsigned int i, j, idx = 0;
    struct pci_config_reg pcr;
#ifdef PC98
    unsigned PCI_CtrlIOPorts[] = { 0xCF8, 0xCF9, 0xCFC };
#else
    unsigned PCI_CtrlIOPorts[] = { 0xCF8, 0xCFA, 0xCFC };
#endif
    int Num_PCI_CtrlIOPorts = 3;
    unsigned PCI_DevIOAddrPorts[16*16];
    int Num_PCI_DevIOAddrPorts = 16*16;

    if (pci_devp[0])
	return;

    for (i=0; i<16; i++)
        for (j=0; j<16; j++)
            PCI_DevIOAddrPorts[(i*16)+j] = 0xC000 + (i*0x0100) + (j*4);

    xf86ClearIOPortList(scrnIndex);
    xf86AddIOPorts(scrnIndex, Num_PCI_CtrlIOPorts, PCI_CtrlIOPorts);
    xf86AddIOPorts(scrnIndex, Num_PCI_DevIOAddrPorts, PCI_DevIOAddrPorts);

    /* Enable I/O access */
    xf86EnableIOPorts(scrnIndex);

    outb(PCI_MODE2_ENABLE_REG, 0x00);
    outb(PCI_MODE2_FORWARD_REG, 0x00);
    tmp1 = inb(PCI_MODE2_ENABLE_REG);
    tmp2 = inb(PCI_MODE2_FORWARD_REG);
    if ((tmp1 == 0x00) && (tmp2 == 0x00)) {
	pcr._configtype = 2;
#ifdef DEBUGPCI
        printf("PCI says configuration type 2\n");
#endif
    } else {
        tmplong1 = inl(PCI_MODE1_ADDRESS_REG);
        outl(PCI_MODE1_ADDRESS_REG, PCI_EN);
        tmplong2 = inl(PCI_MODE1_ADDRESS_REG);
        outl(PCI_MODE1_ADDRESS_REG, tmplong1);
        if (tmplong2 == PCI_EN) {
	    pcr._configtype = 1;
#ifdef DEBUGPCI
            printf("PCI says configuration type 1\n");
#endif
	} else {
	    pcr._configtype = 0;
#ifdef DEBUGPCI
            printf("No PCI !\n");
#endif
            xf86DisableIOPorts(scrnIndex);
            xf86ClearIOPortList(scrnIndex);
	    return;
	}
    }

    /* Try pci config 1 probe first */

#ifdef DEBUGPCI
    printf("\nPCI probing configuration type 1\n");
#endif

    pcr._pcibuses[0] = 0;
    pcr._pcinumbus = 1;
    pcr._pcibusidx = 0;

#ifndef DEBUGPCI
    if (pcr._configtype == 1)
#endif
    do {
        for (pcr._cardnum = 0x0; pcr._cardnum < 0x20; pcr._cardnum += 0x1) {
	    config_cmd = PCI_EN | (pcr._pcibuses[pcr._pcibusidx]<<16) |
                                  (pcr._cardnum<<11);

            outl(PCI_MODE1_ADDRESS_REG, config_cmd);         /* ioreg 0 */
            pcr._device_vendor = inl(PCI_MODE1_DATA_REG);

            if (pcr._vendor == 0xFFFF)   /* nothing there */
                continue;

#ifdef DEBUGPCI
	    printf("\npci bus 0x%x cardnum 0x%02x, vendor 0x%04x device 0x%04x\n",
	        pcr._pcibuses[pcr._pcibusidx], pcr._cardnum, pcr._vendor,
                pcr._device);
#endif

            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x04);
	    pcr._status_command  = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x08);
	    pcr._class_revision  = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x0C);
	    pcr._bist_header_latency_cache = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x10);
	    pcr._base0  = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x14);
	    pcr._base1  = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x18);
	    pcr._base2  = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x1C);
	    pcr._base3  = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x20);
	    pcr._base4  = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x24);
	    pcr._base5  = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x30);
	    pcr._baserom = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x3C);
	    pcr._max_min_ipin_iline = inl(PCI_MODE1_DATA_REG);
            outl(PCI_MODE1_ADDRESS_REG, config_cmd | 0x40);
	    pcr._user_config = inl(PCI_MODE1_DATA_REG);

            /* check for pci-pci bridges (currently we only know Digital) */
            if ((pcr._vendor == 0x1011) && (pcr._device == 0x0001))
                if (pcr._secondary_bus_number > 0)
                    pcr._pcibuses[pcr._pcinumbus++] = pcr._secondary_bus_number;

	    if (idx >= MAX_PCI_DEVICES)
	        continue;

	    if ((pci_devp[idx] = (struct pci_config_reg *)xalloc(sizeof(
		 struct pci_config_reg))) == (struct pci_config_reg *)NULL) {
                outl(PCI_MODE1_ADDRESS_REG, 0x00);
                xf86DisableIOPorts(scrnIndex);
                xf86ClearIOPortList(scrnIndex);
		return;
	    }

	    memcpy(pci_devp[idx++], &pcr, sizeof(struct pci_config_reg));
	    pci_devp[idx] = NULL;
        }
    } while (++pcr._pcibusidx < pcr._pcinumbus);

#ifndef DEBUGPCI
    if (pcr._configtype == 1) {
        outl(PCI_MODE1_ADDRESS_REG, 0x00);
	xf86DisableIOPorts(scrnIndex);
	xf86ClearIOPortList(scrnIndex);
	return;
    }
#endif
    /* Now try pci config 2 probe (deprecated) */

    outb(PCI_MODE2_ENABLE_REG, 0xF1);
    outb(PCI_MODE2_FORWARD_REG, 0x00); /* bus 0 for now */

#ifdef DEBUGPCI
    printf("\nPCI probing configuration type 2\n");
#endif

    pcr._pcibuses[0] = 0;
    pcr._pcinumbus = 1;
    pcr._pcibusidx = 0;

    do {
        for (pcr._ioaddr = 0xC000; pcr._ioaddr < 0xD000; pcr._ioaddr += 0x0100){
	    outb(PCI_MODE2_FORWARD_REG, pcr._pcibuses[pcr._pcibusidx]); /* bus 0 for now */
            pcr._device_vendor = inl(pcr._ioaddr);
	    outb(PCI_MODE2_FORWARD_REG, 0x00); /* bus 0 for now */
	    pcr._cardnum = (pcr._ioaddr >> 8) & 0x0f;

            if (pcr._vendor == 0xFFFF)   /* nothing there */
                continue;
	    /* opti chipsets that use config type 1 look like this on type 2 */
            if ((pcr._vendor == 0xFF00) && (pcr._device == 0xFFFF))
                continue;

#ifdef DEBUGPCI
	    printf("\npci bus 0x%x slot at 0x%04x, vendor 0x%04x device 0x%04x\n",
	        pcr._pcibuses[pcr._pcibusidx], pcr._ioaddr, pcr._vendor,
                pcr._device);
#endif

	    outb(PCI_MODE2_FORWARD_REG, pcr._pcibuses[pcr._pcibusidx]); /* bus 0 for now */
            pcr._status_command = inl(pcr._ioaddr + 0x04);
            pcr._class_revision = inl(pcr._ioaddr + 0x08);
            pcr._bist_header_latency_cache = inl(pcr._ioaddr + 0x0C);
            pcr._base0 = inl(pcr._ioaddr + 0x10);
            pcr._base1 = inl(pcr._ioaddr + 0x14);
            pcr._base2 = inl(pcr._ioaddr + 0x18);
            pcr._base3 = inl(pcr._ioaddr + 0x1C);
            pcr._base4 = inl(pcr._ioaddr + 0x20);
            pcr._base5 = inl(pcr._ioaddr + 0x24);
            pcr._baserom = inl(pcr._ioaddr + 0x30);
            pcr._max_min_ipin_iline = inl(pcr._ioaddr + 0x3C);
            pcr._user_config = inl(pcr._ioaddr + 0x40);
	    outb(PCI_MODE2_FORWARD_REG, 0x00); /* bus 0 for now */

            /* check for pci-pci bridges (currently we only know Digital) */
            if ((pcr._vendor == 0x1011) && (pcr._device == 0x0001))
                if (pcr._secondary_bus_number > 0)
                    pcr._pcibuses[pcr._pcinumbus++] = pcr._secondary_bus_number;

	    if (idx >= MAX_PCI_DEVICES)
	        continue;

	    if ((pci_devp[idx] = (struct pci_config_reg *)xalloc(sizeof(
		 struct pci_config_reg))) == (struct pci_config_reg *)NULL) {
                outb(PCI_MODE2_ENABLE_REG, 0x00);
                outb(PCI_MODE2_FORWARD_REG, 0x00);
                xf86DisableIOPorts(scrnIndex);
                xf86ClearIOPortList(scrnIndex);
		return;
	    }

	    memcpy(pci_devp[idx++], &pcr, sizeof(struct pci_config_reg));
	    pci_devp[idx] = NULL;
	}
    } while (++pcr._pcibusidx < pcr._pcinumbus);

    outb(PCI_MODE2_ENABLE_REG, 0x00);
    outb(PCI_MODE2_FORWARD_REG, 0x00);

    xf86DisableIOPorts(scrnIndex);
    xf86ClearIOPortList(scrnIndex);
}

void
xf86writepci(scrnIndex, cardnum, reg, mask, value)
    int scrnIndex;
    int cardnum;
    int reg;
    unsigned long mask;
    unsigned long value;
{
    unsigned char tmp1, tmp2;
    unsigned long tmplong1, tmplong2, tmp, config_cmd;
    unsigned int i, j;
#ifdef PC98
    unsigned PCI_CtrlIOPorts[] = { 0xCF8, 0xCF9, 0xCFC };
#else
    unsigned PCI_CtrlIOPorts[] = { 0xCF8, 0xCFA, 0xCFC };
#endif
    int configtype;
    int Num_PCI_CtrlIOPorts = 3;
    unsigned PCI_DevIOAddrPorts[16*16];
    int Num_PCI_DevIOAddrPorts = 16*16;

    for (i=0; i<16; i++)
        for (j=0; j<16; j++)
            PCI_DevIOAddrPorts[(i*16)+j] = 0xC000 + (i*0x0100) + (j*4);

    xf86ClearIOPortList(scrnIndex);
    xf86AddIOPorts(scrnIndex, Num_PCI_CtrlIOPorts, PCI_CtrlIOPorts);
    xf86AddIOPorts(scrnIndex, Num_PCI_DevIOAddrPorts, PCI_DevIOAddrPorts);

    /* Enable I/O access */
    xf86EnableIOPorts(scrnIndex);

    outb(PCI_MODE2_ENABLE_REG, 0x00);
    outb(PCI_MODE2_FORWARD_REG, 0x00);
    tmp1 = inb(PCI_MODE2_ENABLE_REG);
    tmp2 = inb(PCI_MODE2_FORWARD_REG);
    if ((tmp1 == 0x00) && (tmp2 == 0x00)) {
	configtype = 2;
#ifdef DEBUGPCI
        printf("PCI says configuration type 2\n");
#endif
    } else {
        tmplong1 = inl(PCI_MODE1_ADDRESS_REG);
        outl(PCI_MODE1_ADDRESS_REG, PCI_EN);
        tmplong2 = inl(PCI_MODE1_ADDRESS_REG);
        outl(PCI_MODE1_ADDRESS_REG, tmplong1);
        if (tmplong2 == PCI_EN) {
	    configtype = 1;
#ifdef DEBUGPCI
            printf("PCI says configuration type 1\n");
#endif
	} else {
	    configtype = 0;
#ifdef DEBUGPCI
            printf("No PCI !\n");
#endif
            xf86DisableIOPorts(scrnIndex);
            xf86ClearIOPortList(scrnIndex);
	    return;
	}
    }

    /* Try pci config 1 probe first */

#ifdef DEBUGPCI
    printf("\nPCI probing configuration type 1\n");
#endif

#ifndef DEBUGPCI
    if (configtype == 1)
#endif
    {
	config_cmd = PCI_EN | (0<<16) | (cardnum<<11);

        outl(PCI_MODE1_ADDRESS_REG, config_cmd | reg);
	tmp = inl(PCI_MODE1_DATA_REG) & ~mask;
	outl(PCI_MODE1_DATA_REG, tmp | (value & mask));
    }
#ifndef DEBUGPCI
    if (configtype == 1) {
        outl(PCI_MODE1_ADDRESS_REG, 0x00);
	xf86DisableIOPorts(scrnIndex);
	xf86ClearIOPortList(scrnIndex);
	return;
    }
#endif
    /* Now try pci config 2 probe (deprecated) */

    outb(PCI_MODE2_ENABLE_REG, 0xF1);
    outb(PCI_MODE2_FORWARD_REG, 0x00); /* bus 0 for now */

#ifdef DEBUGPCI
    printf("\nPCI probing configuration type 2\n");
#endif

    {
	int ioaddr = 0xC000 + (cardnum * 0x100);

	outb(PCI_MODE2_FORWARD_REG, 0x00); /* bus 0 for now */
        tmp = inl(ioaddr + reg) & ~mask;
	outb(PCI_MODE2_FORWARD_REG, 0x00); /* bus 0 for now */
        outl(ioaddr + reg, tmp | (value & mask));
	outb(PCI_MODE2_FORWARD_REG, 0x00); /* bus 0 for now */

    }

    outb(PCI_MODE2_ENABLE_REG, 0x00);
    outb(PCI_MODE2_FORWARD_REG, 0x00);

    xf86DisableIOPorts(scrnIndex);
    xf86ClearIOPortList(scrnIndex);
    return;
}

void
xf86cleanpci()
{
    int idx = 0;

    while (pci_devp[idx])
	xfree(pci_devp[idx++]);

    pci_devp[0] = NULL;
}

/* $XConsortium: nv_driver.c /main/3 1996/10/28 05:13:37 kaleb $ */
/*
 * Copyright 1996-1997  David J. McKay
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DAVID J. MCKAY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Hacked together from mga driver and 3.3.4 NVIDIA driver by Jarno Paananen
   <jpaana@s2.org> */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_setup.c,v 1.1 1999/08/01 07:20:58 dawes Exp $ */

#include "nv_include.h"

#include "nvreg.h"
#include "nvvga.h"

/*
 * Lock and unlock VGA and SVGA registers.
 */
void RivaEnterLeave(ScrnInfoPtr pScrn, Bool enter)
{
    unsigned char tmp;
    CARD16 vgaIOBase = VGAHW_GET_IOBASE();
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    NVPtr pNv = NVPTR(pScrn);
     
    if (enter)
    {
        vgaHWUnlock(hwp);
        outb(vgaIOBase + 4, 0x11);
        tmp = inb(vgaIOBase + 5);
        outb(vgaIOBase + 5, tmp & 0x7F);
        outb(pNv->riva.LockUnlockIO, pNv->riva.LockUnlockIndex);
        outb(pNv->riva.LockUnlockIO + 1, 0x57);
    }
    else
    {
        outb(vgaIOBase + 4, 0x11);
        tmp = inb(vgaIOBase + 5);
        outb(vgaIOBase + 5, (tmp & 0x7F) | 0x80);
        outb(pNv->riva.LockUnlockIO, pNv->riva.LockUnlockIndex);
        outb(pNv->riva.LockUnlockIO + 1, 0x99);
        vgaHWLock(hwp);
    }
}

static void
NVCommonSetup(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    CARD32 regBase = pNv->IOAddress;
    int mmioFlags;
    
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NV4CommonSetup\n"));
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Regbase %x\n", regBase));
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- riva %x\n", &pNv->riva));

    pNv->PreInit = NVRamdacInit;
    pNv->Save = NVDACSave;
    pNv->Restore = NVDACRestore;
    pNv->ModeInit = NVDACInit;

    pNv->Dac.LoadPalette = NVDACLoadPalette;

    /*
     * No IRQ in use.
     */
    pNv->riva.EnableIRQ = 0;
    /*
     * Map remaining registers. This MUST be done in the OS specific driver code.
     */
    pNv->riva.IO      = VGAHW_GET_IOBASE();

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- IO %x\n", pNv->riva.IO));

    mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT;

    pNv->riva.PRAMDAC = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00680000, 0x00001000);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- PRAMDAC %x\n", pNv->riva.PRAMDAC));
    pNv->riva.PFB     = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00100000, 0x00001000);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- PFB %x\n", pNv->riva.PFB));
    pNv->riva.PFIFO   = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00002000, 0x00002000);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- PFIFO %x\n", pNv->riva.PFIFO));
    pNv->riva.PGRAPH  = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00400000, 0x00002000);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- PGRAPH %x\n", pNv->riva.PGRAPH));
    pNv->riva.PEXTDEV = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00101000, 0x00001000);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- PEXTDEV %x\n", pNv->riva.PEXTDEV));
    pNv->riva.PTIMER  = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00009000, 0x00001000);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- PTIMER %x\n", pNv->riva.PTIMER));
    pNv->riva.PMC     = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00000000, 0x00001000);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- PMC %x\n", pNv->riva.PMC));
    pNv->riva.FIFO    = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00800000, 0x00010000);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- FIFO %x\n", pNv->riva.FIFO));

    RivaGetConfig(&pNv->riva);

    pNv->Dac.maxPixelClock = pNv->riva.MaxVClockFreqKHz;

    RivaEnterLeave(pScrn, TRUE);
}

void
NV1Setup(ScrnInfoPtr pScrn)
{
}

void
NV3Setup(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    CARD32 frameBase = pNv->FbAddress;
    int mmioFlags;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NV3Setup\n"));

    /*
     * Record chip architecture based in PCI probe.
     */
    pNv->riva.Architecture = 3;
    /*
     * Map chip-specific memory-mapped registers. This MUST be done in the OS specific driver code.
     */
    mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT;
    pNv->riva.PRAMIN = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     frameBase+0x00C00000, 0x00008000);
            
    NVCommonSetup(pScrn);
}

void
NV4Setup(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    CARD32 regBase = pNv->IOAddress;
    int mmioFlags;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NV4Setup\n"));

    pNv->riva.Architecture = 4;
    /*
     * Map chip-specific memory-mapped registers. This MUST be done in the OS specific driver code.
     */
    mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT;
    pNv->riva.PRAMIN = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00710000, 0x00010000);
    pNv->riva.PCRTC  = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00600000, 0x00001000);

    NVCommonSetup(pScrn);
}


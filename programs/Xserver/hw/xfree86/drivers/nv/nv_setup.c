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

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_setup.c,v 1.33 2003/05/05 22:19:49 mvojkovi Exp $ */

#include "nv_include.h"

/*
 * Override VGA I/O routines.
 */
static void NVWriteCrtc(vgaHWPtr pVga, CARD8 index, CARD8 value)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PCIO, pVga->IOBase + VGA_CRTC_INDEX_OFFSET, index);
    VGA_WR08(pNv->riva.PCIO, pVga->IOBase + VGA_CRTC_DATA_OFFSET,  value);
}
static CARD8 NVReadCrtc(vgaHWPtr pVga, CARD8 index)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PCIO, pVga->IOBase + VGA_CRTC_INDEX_OFFSET, index);
    return (VGA_RD08(pNv->riva.PCIO, pVga->IOBase + VGA_CRTC_DATA_OFFSET));
}
static void NVWriteGr(vgaHWPtr pVga, CARD8 index, CARD8 value)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PVIO, VGA_GRAPH_INDEX, index);
    VGA_WR08(pNv->riva.PVIO, VGA_GRAPH_DATA,  value);
}
static CARD8 NVReadGr(vgaHWPtr pVga, CARD8 index)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PVIO, VGA_GRAPH_INDEX, index);
    return (VGA_RD08(pNv->riva.PVIO, VGA_GRAPH_DATA));
}
static void NVWriteSeq(vgaHWPtr pVga, CARD8 index, CARD8 value)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PVIO, VGA_SEQ_INDEX, index);
    VGA_WR08(pNv->riva.PVIO, VGA_SEQ_DATA,  value);
}
static CARD8 NVReadSeq(vgaHWPtr pVga, CARD8 index)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PVIO, VGA_SEQ_INDEX, index);
    return (VGA_RD08(pNv->riva.PVIO, VGA_SEQ_DATA));
}
static void NVWriteAttr(vgaHWPtr pVga, CARD8 index, CARD8 value)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    volatile CARD8 tmp;

    tmp = VGA_RD08(pNv->riva.PCIO, pVga->IOBase + VGA_IN_STAT_1_OFFSET);
    if (pVga->paletteEnabled)
        index &= ~0x20;
    else
        index |= 0x20;
    VGA_WR08(pNv->riva.PCIO, VGA_ATTR_INDEX,  index);
    VGA_WR08(pNv->riva.PCIO, VGA_ATTR_DATA_W, value);
}
static CARD8 NVReadAttr(vgaHWPtr pVga, CARD8 index)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    volatile CARD8 tmp;

    tmp = VGA_RD08(pNv->riva.PCIO, pVga->IOBase + VGA_IN_STAT_1_OFFSET);
    if (pVga->paletteEnabled)
        index &= ~0x20;
    else
        index |= 0x20;
    VGA_WR08(pNv->riva.PCIO, VGA_ATTR_INDEX, index);
    return (VGA_RD08(pNv->riva.PCIO, VGA_ATTR_DATA_R));
}
static void NVWriteMiscOut(vgaHWPtr pVga, CARD8 value)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PVIO, VGA_MISC_OUT_W, value);
}
static CARD8 NVReadMiscOut(vgaHWPtr pVga)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    return (VGA_RD08(pNv->riva.PVIO, VGA_MISC_OUT_R));
}
static void NVEnablePalette(vgaHWPtr pVga)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    volatile CARD8 tmp;

    tmp = VGA_RD08(pNv->riva.PCIO, pVga->IOBase + VGA_IN_STAT_1_OFFSET);
    VGA_WR08(pNv->riva.PCIO, VGA_ATTR_INDEX, 0x00);
    pVga->paletteEnabled = TRUE;
}
static void NVDisablePalette(vgaHWPtr pVga)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    volatile CARD8 tmp;

    tmp = VGA_RD08(pNv->riva.PCIO, pVga->IOBase + VGA_IN_STAT_1_OFFSET);
    VGA_WR08(pNv->riva.PCIO, VGA_ATTR_INDEX, 0x20);
    pVga->paletteEnabled = FALSE;
}
static void NVWriteDacMask(vgaHWPtr pVga, CARD8 value)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PDIO, VGA_DAC_MASK, value);
}
static CARD8 NVReadDacMask(vgaHWPtr pVga)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    return (VGA_RD08(pNv->riva.PDIO, VGA_DAC_MASK));
}
static void NVWriteDacReadAddr(vgaHWPtr pVga, CARD8 value)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PDIO, VGA_DAC_READ_ADDR, value);
}
static void NVWriteDacWriteAddr(vgaHWPtr pVga, CARD8 value)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PDIO, VGA_DAC_WRITE_ADDR, value);
}
static void NVWriteDacData(vgaHWPtr pVga, CARD8 value)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    VGA_WR08(pNv->riva.PDIO, VGA_DAC_DATA, value);
}
static CARD8 NVReadDacData(vgaHWPtr pVga)
{
    NVPtr pNv = (NVPtr)pVga->MMIOBase;
    return (VGA_RD08(pNv->riva.PDIO, VGA_DAC_DATA));
}

static Bool 
NVIsConnected (ScrnInfoPtr pScrn, int output)
{
    NVPtr pNv = NVPTR(pScrn);
    volatile U032 *PRAMDAC = pNv->riva.PRAMDAC0;
    CARD32 reg52C, reg608;
    Bool present;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
               "Probing for analog device on output %s...\n", 
                output ? "B" : "A");

    if(output) PRAMDAC += 0x800;

    reg52C = PRAMDAC[0x052C/4];
    reg608 = PRAMDAC[0x0608/4];

    PRAMDAC[0x0608/4] = reg608 & ~0x00010000;

    PRAMDAC[0x052C/4] = reg52C & 0x0000FEEE;
    usleep(1000);
    PRAMDAC[0x052C/4] |= 1;

    pNv->riva.PRAMDAC0[0x0610/4] = 0x94050140;
    pNv->riva.PRAMDAC0[0x0608/4] |= 0x00001000;

    usleep(1000);

    present = (PRAMDAC[0x0608/4] & (1 << 28)) ? TRUE : FALSE;

    if(present)
       xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "  ...found one\n");
    else
       xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "  ...can't find one\n");

    pNv->riva.PRAMDAC0[0x0608/4] &= 0x0000EFFF;

    PRAMDAC[0x052C/4] = reg52C;
    PRAMDAC[0x0608/4] = reg608;

    return present;
}

static void
NVSelectHeadRegisters(ScrnInfoPtr pScrn, int head)
{
    NVPtr pNv = NVPTR(pScrn);

    if(head) {
       pNv->riva.PCIO = pNv->riva.PCIO0 + 0x2000;
       pNv->riva.PCRTC = pNv->riva.PCRTC0 + 0x800;
       pNv->riva.PRAMDAC = pNv->riva.PRAMDAC0 + 0x800;
       pNv->riva.PDIO = pNv->riva.PDIO0 + 0x2000;
    } else {
       pNv->riva.PCIO = pNv->riva.PCIO0;
       pNv->riva.PCRTC = pNv->riva.PCRTC0;
       pNv->riva.PRAMDAC = pNv->riva.PRAMDAC0;
       pNv->riva.PDIO = pNv->riva.PDIO0;
    }
}



static xf86MonPtr 
NVProbeDDC (ScrnInfoPtr pScrn, int bus)
{
    NVPtr pNv = NVPTR(pScrn);
    xf86MonPtr MonInfo = NULL;

    if(!pNv->I2C) return NULL;

    pNv->DDCBase = bus ? 0x36 : 0x3e;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
               "Probing for EDID on I2C bus %s...\n", bus ? "B" : "A");

    if ((MonInfo = xf86DoEDID_DDC2(pScrn->scrnIndex, pNv->I2C))) {
       xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                  "DDC detected a %s:\n", MonInfo->features.input_type ?
                  "DFP" : "CRT");
       xf86PrintEDID( MonInfo );
    } else {
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
                  "  ... none found\n");
    }

    return MonInfo;
}

static void
NVCommonSetup(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    vgaHWPtr pVga = VGAHWPTR(pScrn);
    CARD32 regBase = pNv->IOAddress;
    CARD16 implementation = pNv->Chipset & 0x0ff0;
    xf86MonPtr monitorA, monitorB;
    Bool mobile = FALSE;
    Bool tvA = FALSE;
    Bool tvB = FALSE;
    int FlatPanel = -1;   /* really means the CRTC is slaved */
    Bool Television = FALSE;
    int mmioFlags;
    
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVCommonSetup\n"));
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Regbase %x\n", regBase));
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- riva %x\n", &pNv->riva));

    pNv->Save = NVDACSave;
    pNv->Restore = NVDACRestore;
    pNv->ModeInit = NVDACInit;

    pNv->Dac.LoadPalette = NVDACLoadPalette;

    /*
     * Override VGA I/O routines.
     */
    pVga->writeCrtc         = NVWriteCrtc;
    pVga->readCrtc          = NVReadCrtc;
    pVga->writeGr           = NVWriteGr;
    pVga->readGr            = NVReadGr;
    pVga->writeAttr         = NVWriteAttr;
    pVga->readAttr          = NVReadAttr;
    pVga->writeSeq          = NVWriteSeq;
    pVga->readSeq           = NVReadSeq;
    pVga->writeMiscOut      = NVWriteMiscOut;
    pVga->readMiscOut       = NVReadMiscOut;
    pVga->enablePalette     = NVEnablePalette;
    pVga->disablePalette    = NVDisablePalette;
    pVga->writeDacMask      = NVWriteDacMask;
    pVga->readDacMask       = NVReadDacMask;
    pVga->writeDacWriteAddr = NVWriteDacWriteAddr;
    pVga->writeDacReadAddr  = NVWriteDacReadAddr;
    pVga->writeDacData      = NVWriteDacData;
    pVga->readDacData       = NVReadDacData;
    /*
     * Note: There are different pointers to the CRTC/AR and GR/SEQ registers.
     * Bastardize the intended uses of these to make it work.
     */
    pVga->MMIOBase   = (CARD8 *)pNv;
    pVga->MMIOOffset = 0;
    
    /*
     * No IRQ in use.
     */
    pNv->riva.EnableIRQ = 0;
    pNv->riva.IO      = VGA_IOBASE_COLOR;

    mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT;

    pNv->riva.PRAMDAC0 = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00680000, 0x00003000);
    pNv->riva.PFB     = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00100000, 0x00001000);
    pNv->riva.PFIFO   = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00002000, 0x00002000);
    pNv->riva.PGRAPH  = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00400000, 0x00002000);
    pNv->riva.PEXTDEV = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00101000, 0x00001000);
    pNv->riva.PTIMER  = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00009000, 0x00001000);
    pNv->riva.PMC     = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00000000, 0x00009000);
    pNv->riva.FIFO    = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00800000, 0x00010000);

    /*
     * These registers are read/write as 8 bit values.  Probably have to map
     * sparse on alpha.
     */
    pNv->riva.PCIO0 = (U008 *)xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
                                           pNv->PciTag, regBase+0x00601000,
                                           0x00003000);
    pNv->riva.PDIO0 = (U008 *)xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
                                           pNv->PciTag, regBase+0x00681000,
                                           0x00003000);
    pNv->riva.PVIO = (U008 *)xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
                                           pNv->PciTag, regBase+0x000C0000,
                                           0x00001000);

    if(pNv->riva.Architecture == 3)
       pNv->riva.PCRTC0 = pNv->riva.PGRAPH;

    pNv->twoHeads =  (implementation >= 0x0110) &&
                     (implementation != 0x0150) &&
                     (implementation != 0x01A0) &&
                     (implementation != 0x0200);

    /* look for known laptop chips */
    switch(pNv->Chipset & 0xffff) {
    case 0x0112:
    case 0x0174:
    case 0x0175:
    case 0x0176:
    case 0x0177:
    case 0x0179:
    case 0x017C:
    case 0x017D:
    case 0x0186:
    case 0x0187:
    case 0x0286:
    case 0x028C:
    case 0x0316:
    case 0x0317:
    case 0x031A:
    case 0x031B:
    case 0x031C:
    case 0x031D:
    case 0x031E:
    case 0x031F:
    case 0x0324:
    case 0x0325:
    case 0x0328:
    case 0x0329:
    case 0x032C:
    case 0x032D:
        mobile = TRUE;
        break;
    default:
        break;
    }
 
    RivaGetConfig(pNv);

    NVSelectHeadRegisters(pScrn, 0);

    pNv->riva.LockUnlock(&pNv->riva, 0);

    NVI2CInit(pScrn);

    pNv->Television = FALSE;

    if(!pNv->twoHeads) {
       pNv->CRTCnumber = 0;
       if((monitorA = NVProbeDDC(pScrn, 0))) {
           FlatPanel = monitorA->features.input_type ? 1 : 0;

           /* NV3 and NV4 don't support FlatPanels */
           if((pNv->Chipset & 0x0fff) <= 0x0020)
              FlatPanel = 0;
       } else {
           VGA_WR08(pNv->riva.PCIO, 0x03D4, 0x28);
           if(VGA_RD08(pNv->riva.PCIO, 0x03D5) & 0x80) {
              VGA_WR08(pNv->riva.PCIO, 0x03D4, 0x33);
              if(!(VGA_RD08(pNv->riva.PCIO, 0x03D5) & 0x01)) 
                 Television = TRUE;
              FlatPanel = 1;
           } else {
              FlatPanel = 0;
           }
           xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                         "HW is currently programmed for %s\n",
                          FlatPanel ? (Television ? "TV" : "DFP") : "CRT");
       } 

       if(pNv->FlatPanel == -1) {
           pNv->FlatPanel = FlatPanel;
           pNv->Television = Television;
       } else {
           xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                      "Forcing display type to %s as specified\n", 
                       pNv->FlatPanel ? "DFP" : "CRT");
       }
    } else {
       CARD8 outputAfromCRTC, outputBfromCRTC;
       int CRTCnumber = -1;
       CARD8 slaved_on_A, slaved_on_B;
       Bool analog_on_A, analog_on_B;
       CARD32 oldhead;
       CARD8 cr44;
      
       if(implementation != 0x0110) {
           if(pNv->riva.PRAMDAC0[0x0000052C/4] & 0x100)
               outputAfromCRTC = 1;
           else            
               outputAfromCRTC = 0;
           if(pNv->riva.PRAMDAC0[0x0000252C/4] & 0x100)
               outputBfromCRTC = 1;
           else
               outputBfromCRTC = 0;
          analog_on_A = NVIsConnected(pScrn, 0);
          analog_on_B = NVIsConnected(pScrn, 1);
       } else {
          outputAfromCRTC = 0;
          outputBfromCRTC = 1;
          analog_on_A = FALSE;
          analog_on_B = FALSE;
       }


       VGA_WR08(pNv->riva.PCIO, 0x03D4, 0x44);
       cr44 = VGA_RD08(pNv->riva.PCIO, 0x03D5);

       VGA_WR08(pNv->riva.PCIO, 0x03D5, 3);
       NVSelectHeadRegisters(pScrn, 1);
       pNv->riva.LockUnlock(&pNv->riva, 0);

       VGA_WR08(pNv->riva.PCIO, 0x03D4, 0x28);
       slaved_on_B = VGA_RD08(pNv->riva.PCIO, 0x03D5) & 0x80;
       if(slaved_on_B) {
           VGA_WR08(pNv->riva.PCIO, 0x03D4, 0x33);
           tvB = !(VGA_RD08(pNv->riva.PCIO, 0x03D5) & 0x01);
       }

       VGA_WR08(pNv->riva.PCIO, 0x03D4, 0x44);
       VGA_WR08(pNv->riva.PCIO, 0x03D5, 0);
       NVSelectHeadRegisters(pScrn, 0);
       pNv->riva.LockUnlock(&pNv->riva, 0);

       VGA_WR08(pNv->riva.PCIO, 0x03D4, 0x28);
       slaved_on_A = VGA_RD08(pNv->riva.PCIO, 0x03D5) & 0x80; 
       if(slaved_on_A) {
           VGA_WR08(pNv->riva.PCIO, 0x03D4, 0x33);
           tvA = !(VGA_RD08(pNv->riva.PCIO, 0x03D5) & 0x01);
       }

       oldhead = pNv->riva.PCRTC0[0x00000860/4];
       pNv->riva.PCRTC0[0x00000860/4] = oldhead | 0x00000010;

       monitorA = NVProbeDDC(pScrn, 0);
       monitorB = NVProbeDDC(pScrn, 1);

       if(slaved_on_A) {
          CRTCnumber = 0;
          FlatPanel = 1;
          Television = tvA;
          xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                    "CRTC 0 is currently programmed for %s\n",
                     Television ? "TV" : "DFP");
       } else 
       if(slaved_on_B) {
          CRTCnumber = 1;
          FlatPanel = 1;
          Television = tvB;
          xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                    "CRTC 1 is currently programmed for %s\n",
                     Television ? "TV" : "DFP");
       } else
       if(analog_on_A) {
          CRTCnumber = outputAfromCRTC;
          FlatPanel = 0;
          xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                    "CRTC %i appears to have a CRT attached\n", CRTCnumber);
       } else
       if(analog_on_B) {
           CRTCnumber = outputBfromCRTC;
           FlatPanel = 0;
           xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
                    "CRTC %i appears to have a CRT attached\n", CRTCnumber);
       } else if(monitorA) {
           FlatPanel = monitorA->features.input_type ? 1 : 0;
       } else if(monitorB) {
           FlatPanel = monitorB->features.input_type ? 1 : 0;
       }

       if(pNv->FlatPanel == -1) {
          if(FlatPanel != -1) {
             pNv->FlatPanel = FlatPanel;
             pNv->Television = Television;
          } else {
             xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Unable to detect display type...\n");
             if(mobile) {
                 xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT,
                            "...On a laptop, assuming DFP\n");
                 pNv->FlatPanel = 1;
             } else {
                 xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT,
                            "...Using default of CRT\n");
                 pNv->FlatPanel = 0;
             }
          }
       } else {
           xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                      "Forcing display type to %s as specified\n", 
                       pNv->FlatPanel ? "DFP" : "CRT");
       }

       if(pNv->CRTCnumber == -1) {
          if(CRTCnumber != -1) pNv->CRTCnumber = CRTCnumber;
          else {
             xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Unable to detect which CRTCNumber...\n");
             if(pNv->FlatPanel) pNv->CRTCnumber = 1;
             else pNv->CRTCnumber = 0;
             xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT,
                        "...Defaulting to CRTCNumber %i\n", pNv->CRTCnumber);
          }
       } else {
           xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                      "Forcing CRTCNumber %i as specified\n", pNv->CRTCnumber);
       }
     
       if(monitorA) {
           if((monitorA->features.input_type && pNv->FlatPanel) ||
              (!monitorA->features.input_type && !pNv->FlatPanel))
           {
               if(monitorB) { 
                  xfree(monitorB);
                  monitorB = NULL;
               }
           } else {
              xfree(monitorA);
              monitorA = NULL;
           }
       }

       if(monitorB) {
           if((monitorB->features.input_type && !pNv->FlatPanel) ||
              (!monitorB->features.input_type && pNv->FlatPanel)) 
           {
              xfree(monitorB);
           } else {
              monitorA = monitorB;
           }
           monitorB = NULL;
       }

       if(implementation == 0x0110)
           cr44 = pNv->CRTCnumber * 0x3;

       pNv->riva.PCRTC0[0x00000860/4] = oldhead;

       VGA_WR08(pNv->riva.PCIO, 0x03D4, 0x44);
       VGA_WR08(pNv->riva.PCIO, 0x03D5, cr44);
       NVSelectHeadRegisters(pScrn, pNv->CRTCnumber);
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
              "Using %s on CRTC %i\n",
              pNv->FlatPanel ? (pNv->Television ? "TV" : "DFP") : "CRT", 
              pNv->CRTCnumber);

    if(monitorA)
      xf86SetDDCproperties(pScrn, monitorA);

    pNv->Dac.maxPixelClock = pNv->riva.MaxVClockFreqKHz;

    pNv->riva.flatPanel = pNv->FlatPanel ? FP_ENABLE : 0;
    if(pNv->riva.flatPanel && pNv->FPDither && (pScrn->depth == 24))
       pNv->riva.flatPanel |= FP_DITHER;
}

void
NV3Setup(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    CARD32 frameBase = pNv->FbAddress;
    int mmioFlags;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NV3Setup\n"));

    pNv->riva.Architecture = 3;
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
    mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT;
    pNv->riva.PRAMIN = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00710000, 0x00010000);
    pNv->riva.PCRTC0 = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00600000, 0x00001000);

    NVCommonSetup(pScrn);
}

void
NV10Setup(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    CARD32 regBase = pNv->IOAddress;
    int mmioFlags;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NV10Setup\n"));

    pNv->riva.Architecture = 0x10;
    mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT;
    pNv->riva.PRAMIN = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00710000, 0x00010000);
    pNv->riva.PCRTC0 = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00600000, 0x00003000);

    NVCommonSetup(pScrn);
}

void
NV20Setup(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    CARD32 regBase = pNv->IOAddress;
    int mmioFlags;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NV20Setup\n"));

    pNv->riva.Architecture = 0x20;
    mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT;
    pNv->riva.PRAMIN = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00710000, 0x00010000);
    pNv->riva.PCRTC0 = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00600000, 0x00003000);

    NVCommonSetup(pScrn);
}


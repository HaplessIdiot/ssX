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

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_dac.c,v 1.1 1999/08/01 07:20:55 dawes Exp $ */

#include "nv_include.h"

#include "nvreg.h"
#include "nvvga.h"

Bool
NVDACInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    int i;
    int horizDisplay = (mode->CrtcHDisplay/8)   - 1;
    int horizStart   = (mode->CrtcHSyncStart/8) - 1;
    int horizEnd     = (mode->CrtcHSyncEnd/8)   - 1;
    int horizTotal   = (mode->CrtcHTotal/8)     - 1;
    int vertDisplay  =  mode->CrtcVDisplay      - 1;
    int vertStart    =  mode->CrtcVSyncStart    - 1;
    int vertEnd      =  mode->CrtcVSyncEnd      - 1;
    int vertTotal    =  mode->CrtcVTotal        - 2;

    NVPtr pNv = NVPTR(pScrn);
    NVRegPtr nvReg = &pNv->ModeReg;
    NVFBLayout *pLayout = &pNv->CurrentLayout;
    vgaRegPtr   pVga;
    
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVDACInit\n"));
    
    /*
     * This will initialize all of the generic VGA registers.
     */
    if (!vgaHWInit(pScrn, mode))
        return(FALSE);

    pVga = &VGAHWPTR(pScrn)->ModeReg;

    /*
     * Set all CRTC values.
     */

    pVga->CRTC[0x0]  = Set8Bits(horizTotal - 4);
    pVga->CRTC[0x1]  = Set8Bits(horizDisplay);
    pVga->CRTC[0x2]  = Set8Bits(horizDisplay);
    pVga->CRTC[0x3]  = SetBitField(horizTotal,4:0,4:0) 
                                                 | SetBit(7);
    pVga->CRTC[0x4]  = Set8Bits(horizStart);
    pVga->CRTC[0x5]  = SetBitField(horizTotal,5:5,7:7)
        | SetBitField(horizEnd,4:0,4:0);
    pVga->CRTC[0x6]  = SetBitField(vertTotal,7:0,7:0);
    pVga->CRTC[0x7]  = SetBitField(vertTotal,8:8,0:0)
        | SetBitField(vertDisplay,8:8,1:1)
        | SetBitField(vertStart,8:8,2:2)
        | SetBitField(vertDisplay,8:8,3:3)
        | SetBit(4)
        | SetBitField(vertTotal,9:9,5:5)
        | SetBitField(vertDisplay,9:9,6:6)
        | SetBitField(vertStart,9:9,7:7);
    pVga->CRTC[0x9]  = SetBitField(vertDisplay,9:9,5:5)
        | SetBit(6)
        | ((mode->Flags & V_DBLSCAN) ? 0x80 : 0x00);
    pVga->CRTC[0x10] = Set8Bits(vertStart);
    pVga->CRTC[0x11] = SetBitField(vertEnd,3:0,3:0) | SetBit(5);
    pVga->CRTC[0x12] = Set8Bits(vertDisplay);
    pVga->CRTC[0x13] = 0xFF &  
	((pLayout->displayWidth/8)*(pLayout->bitsPerPixel/8));
    pVga->CRTC[0x15] = Set8Bits(vertDisplay);
    pVga->CRTC[0x16] = Set8Bits(vertTotal + 1);

    /*
     * Initialize DAC palette.
     */
    if(pLayout->bitsPerPixel != 8 )
    {
        if (pNv->riva.Architecture == 3)
            for (i = 0; i < 256; i++)
            {
                pVga->DAC[i*3]     = i >> 2;
                pVga->DAC[(i*3)+1] = i >> 2;
                pVga->DAC[(i*3)+2] = i >> 2;
            }
        else
            for (i = 0; i < 256; i++)
            {
                pVga->DAC[i*3]     = i;
                pVga->DAC[(i*3)+1] = i;
                pVga->DAC[(i*3)+2] = i;
            }
    }
    
    /*
     * Calculate the extended registers.
     */

    if(pLayout->depth < 24) 
	i = pLayout->depth;
    else i = 32;

    pNv->riva.CalcStateExt(&pNv->riva, 
                           nvReg,
                           i,
                           pLayout->displayWidth,
                           mode->CrtcHDisplay,
                           horizDisplay,
                           horizStart,
                           horizEnd,
                           horizTotal,
                           pScrn->virtualY,
                           vertDisplay,
                           vertStart,
                           vertEnd,
                           vertTotal,
                           mode->Clock);

    return (TRUE);
}

void 
NVDACRestore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, NVRegPtr nvReg,
             Bool restoreFonts)
{
    NVPtr pNv = NVPTR(pScrn);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVDACRestore\n"));
    vgaHWRestore(pScrn, vgaReg, VGA_SR_CMAP | VGA_SR_MODE | 
			(restoreFonts? VGA_SR_FONTS : 0));
    pNv->riva.LoadStateExt(&pNv->riva, nvReg);
}

/*
 * NVRamdacInit
 */
void
NVRamdacInit(ScrnInfoPtr pScrn)
{
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVRamdacInit\n"));
}

/*
 * NVDACSave
 *
 * This function saves the video state.
 */
void
NVDACSave(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, NVRegPtr nvReg,
          Bool saveFonts)
{
    NVPtr pNv = NVPTR(pScrn);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVDACSave\n"));
    vgaHWSave(pScrn, vgaReg, VGA_SR_MODE | (saveFonts? VGA_SR_FONTS : 0));
    pNv->riva.UnloadStateExt(&pNv->riva, nvReg);
}

void
NVDACLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors,
                 VisualPtr pVisual )
{
    int i, index;
    vgaRegPtr   pVga;

    pVga = &VGAHWPTR(pScrn)->ModeReg;

    if(pVisual->nplanes != 8) return;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVDACLoadPalette\n"));
    for(i = 0; i < numColors; i++) {
        index = indices[i];
	pVga->DAC[index*3]     = colors[index].red;
	pVga->DAC[(index*3)+1] = colors[index].green;
	pVga->DAC[(index*3)+2] = colors[index].blue;
    }
    vgaHWRestore(pScrn, pVga, VGA_SR_CMAP);
}


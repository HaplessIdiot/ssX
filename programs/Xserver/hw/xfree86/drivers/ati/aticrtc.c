/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/aticrtc.c,v 1.4tsi Exp $ */
/*
 * Copyright 1997 through 1999 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "ati.h"
#include "atiadapter.h"
#include "atichip.h"
#include "atidac.h"
#include "atidsp.h"
#include "atimach64.h"
#include "atiprint.h"
#include "ativga.h"
#include "atiwonder.h"

/*
 * ATICopyVGAMemory --
 *
 * This function is called to copy one or all banks of a VGA plane.
 */
static void
ATICopyVGAMemory
(
    ATIPtr   pATI,
    ATIHWPtr pATIHW,
    pointer  *saveptr,
    pointer  *from,
    pointer  *to
)
{
    unsigned int iBank;

    for (iBank = 0;  iBank < pATIHW->nBank;  iBank++)
    {
        (*pATIHW->SetBank)(pATI, iBank);
        (void) memcpy(*to, *from, 0x00010000U);
        *saveptr = (char *)(*saveptr) + 0x00010000U;
    }
}

/*
 * ATISwap --
 *
 * This function saves/restores video memory contents during video mode
 * switches.
 */
static void
ATISwap
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI,
    ATIHWPtr    pATIHW,
    Bool        ToFB
)
{
    pointer save, *from, *to;
    unsigned int iPlane = 0, PlaneMask = 1;
    CARD8 seq2, seq4, gra1, gra3, gra4, gra5, gra6, gra8;

    /*
     * This is only done for non-accelerator modes.  If the video state on
     * server entry was an accelerator mode, the application that relinquished
     * the console had better do the Right Thing (tm) anyway by saving and
     * restoring its own video memory contents.
     */
    if (pATIHW->crtc != ATI_CRTC_VGA)
        return;

    if (ToFB)
    {
        if (!pATIHW->frame_buffer)
            return;

        from = &save;
        to = &pATI->pBank;
    }
    else
    {
        /* Allocate the memory */
        if (!pATIHW->frame_buffer)
        {
            pATIHW->frame_buffer =
                (pointer)xalloc(pATIHW->nBank * pATIHW->nPlane * 0x00010000U);
            if (!pATIHW->frame_buffer)
            {
                xf86DrvMsg(pScreenInfo->scrnIndex, X_WARNING,
                    "Temporary frame buffer could not be allocated.\n");
                return;
            }
        }

        from = &pATI->pBank;
        to = &save;
    }

    /* Turn off screen */
    ATIVGASaveScreen(pATI, FALSE);

    /* Save register values to be modified */
    seq2 = GetReg(SEQX, 0x02U);
    seq4 = GetReg(SEQX, 0x04U);
    gra1 = GetReg(GRAX, 0x01U);
    gra3 = GetReg(GRAX, 0x03U);
    gra4 = GetReg(GRAX, 0x04U);
    gra5 = GetReg(GRAX, 0x05U);
    gra6 = GetReg(GRAX, 0x06U);
    gra8 = GetReg(GRAX, 0x08U);

    save = pATIHW->frame_buffer;

    /* Temporarily normalize the mode */
    if (gra1 != 0x00U)
        PutReg(GRAX, 0x01U, 0x00U);
    if (gra3 != 0x00U)
        PutReg(GRAX, 0x03U, 0x00U);
    if (gra6 != 0x05U)
        PutReg(GRAX, 0x06U, 0x05U);
    if (gra8 != 0xFFU)
        PutReg(GRAX, 0x08U, 0xFFU);

    if (seq4 & 0x08U)
    {
        /* Setup packed mode memory */
        if (seq2 != 0x0FU)
            PutReg(SEQX, 0x02U, 0x0FU);
        if (seq4 != 0x0AU)
            PutReg(SEQX, 0x04U, 0x0AU);
        if (pATI->Chip < ATI_CHIP_264CT)
        {
            if (gra5 != 0x00U)
                PutReg(GRAX, 0x05U, 0x00U);
        }
        else
        {
            if (gra5 != 0x40U)
                PutReg(GRAX, 0x05U, 0x40U);
        }

        ATICopyVGAMemory(pATI, pATIHW, &save, from, to);

        if (seq2 != 0x0FU)
            PutReg(SEQX, 0x02U, seq2);
        if (seq4 != 0x0AU)
            PutReg(SEQX, 0x04U, seq4);
        if (pATI->Chip < ATI_CHIP_264CT)
        {
            if (gra5 != 0x00U)
                PutReg(GRAX, 0x05U, gra5);
        }
        else
        {
            if (gra5 != 0x40U)
                PutReg(GRAX, 0x05U, gra5);
        }
    }
    else
    {
        gra4 = GetReg(GRAX, 0x04U);

        /* Setup planar mode memory */
        if (seq4 != 0x06U)
            PutReg(SEQX, 0x04U, 0x06U);
        if (gra5 != 0x00U)
            PutReg(GRAX, 0x05U, 0x00U);

        for (;  iPlane < pATIHW->nPlane;  iPlane++)
        {
            PutReg(SEQX, 0x02U, PlaneMask);
            PutReg(GRAX, 0x04U, iPlane);
            ATICopyVGAMemory(pATI, pATIHW, &save, from, to);
            PlaneMask <<= 1;
        }

        PutReg(SEQX, 0x02U, seq2);
        if (seq4 != 0x06U)
            PutReg(SEQX, 0x04U, seq4);
        PutReg(GRAX, 0x04U, gra4);
        if (gra5 != 0x00U)
            PutReg(GRAX, 0x05U, gra5);
    }

    /* Restore registers */
    if (gra1 != 0x00U)
        PutReg(GRAX, 0x01U, gra1);
    if (gra3 != 0x00U)
        PutReg(GRAX, 0x03U, gra3);
    if (gra6 != 0x05U)
        PutReg(GRAX, 0x06U, gra6);
    if (gra8 != 0xFFU)
        PutReg(GRAX, 0x08U, gra8);

    /* Back to bank 0 */
    (*pATIHW->SetBank)(pATI, 0);

    /*
     * If restoring video memory for a server video mode, free the frame buffer
     * save area.
     */
    if (ToFB && (pATIHW == &pATI->NewHW))
    {
        xfree(pATIHW->frame_buffer);
        pATIHW->frame_buffer = NULL;
    }
}

/*
 * ATICRTCPreInit --
 *
 * This function initializes an ATIHWRec with information common to all video
 * states generated by the driver.
 */
void
ATICRTCPreInit
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI,
    ATIHWPtr    pATIHW
)
{
    if (pATI->VGAAdapter != ATI_ADAPTER_NONE)
    {
        /* Fill in VGA data */
        ATIVGAPreInit(pScreenInfo, pATI, pATIHW);

        /* Fill in VGA Wonder data */
        if (pATI->CPIO_VGAWonder)
            ATIVGAWonderPreInit(pScreenInfo, pATI, pATIHW);
    }

    /* Fill in Mach64 data */
    if (pATI->Chip >= ATI_CHIP_88800GXC)
        ATIMach64PreInit(pScreenInfo, pATI, pATIHW);

    /* For now disable extended reference and feedback dividers */
    if (pATI->Chip >= ATI_CHIP_264LT)
        pATIHW->pll_ext_vpll_cntl = ATIGetMach64PLLReg(PLL_EXT_VPLL_CNTL) &
            ~(PLL_EXT_VPLL_EN | PLL_EXT_VPLL_VGA_EN | PLL_EXT_VPLL_INSYNC);

    /* Set RAMDAC data */
    ATIDACPreInit(pScreenInfo, pATIHW);
}

/*
 * ATICRTCSave --
 *
 * This function saves the curent video state.
 */
void
ATICRTCSave
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI,
    ATIHWPtr    pATIHW
)
{
    /* Get bank to bank 0 */
    (*pATIHW->SetBank)(pATI, 0);

    /* Save clock data */
    ATIClockSave(pScreenInfo, pATI, pATIHW);
    if (pATI->Chip >= ATI_CHIP_264LT)
        pATIHW->pll_ext_vpll_cntl = ATIGetMach64PLLReg(PLL_EXT_VPLL_CNTL);

    if (pATI->VGAAdapter != ATI_ADAPTER_NONE)
    {
        /* Save VGA data */
        ATIVGASave(pATI, pATIHW);

        /* Save VGA Wonder data */
        if (pATI->CPIO_VGAWonder)
            ATIVGAWonderSave(pATI, pATIHW);
    }

    /* Save Mach64 data */
    if (pATI->Chip >= ATI_CHIP_88800GXC)
        ATIMach64Save(pATI, pATIHW);

    /* Save DSP data */
    if ((pATI->Chip >= ATI_CHIP_264VTB) && (pATI->CPIODecoding == BLOCK_IO))
        ATIDSPSave(pATI, pATIHW);

    /*
     * For some unknown reason, CLKDIV2 needs to be turned off to save the
     * DAC's LUT reliably on VGA Wonder VLB adapters.
     */
    if ((pATI->Adapter == ATI_ADAPTER_NONISA) && (pATIHW->seq[1] & 0x08U))
        PutReg(SEQX, 0x01U, pATIHW->seq[1] & ~0x08U);

    /* Save RAMDAC state */
    ATIDACSave(pATI, pATIHW);

    if ((pATI->Adapter == ATI_ADAPTER_NONISA) && (pATIHW->seq[1] & 0x08U))
        PutReg(SEQX, 0x01U, pATIHW->seq[1]);

    /*
     * The server has already saved video memory contents when switching out of
     * its virtual console, so don't do it again.
     */
    if (pATIHW != &pATI->NewHW)
    {
        pATIHW->FeedbackDivider = 0;    /* Don't programme clock */

        ATISwap(pScreenInfo, pATI, pATIHW, FALSE);      /* Save video memory */
    }

    if (pATI->VGAAdapter != ATI_ADAPTER_NONE)
        ATIVGASaveScreen(pATI, TRUE);                   /* Turn on screen */
}

/*
 * ATICRTCCalculate --
 *
 * This function fills in the an ATIHWRec with all register values needed to
 * enable a video state.  It's important that this be done without modifying
 * the current video state.
 */
Bool
ATICRTCCalculate
(
    ScrnInfoPtr    pScreenInfo,
    ATIPtr         pATI,
    ATIHWPtr       pATIHW,
    DisplayModePtr pMode
)
{
    switch (pATIHW->crtc)
    {
        case ATI_CRTC_VGA:
            /* Fill in VGA data */
            ATIVGACalculate(pATI, pATIHW, pMode);

            /* Fill in VGA Wonder data */
            if (pATI->CPIO_VGAWonder)
                ATIVGAWonderCalculate(pScreenInfo, pATI, pATIHW, pMode);

            if (pATI->Chip >= ATI_CHIP_88800GXC)
            {
                pATIHW->crtc_gen_cntl = inl(pATI->CPIO_CRTC_GEN_CNTL) &
                    ~(CRTC_DBL_SCAN_EN | CRTC_INTERLACE_EN |
                      CRTC_HSYNC_DIS | CRTC_VSYNC_DIS | CRTC_CSYNC_EN |
                      CRTC_PIX_BY_2_EN | CRTC_DISPLAY_DIS |
                      CRTC_VGA_XOVERSCAN | CRTC_VGA_128KAP_PAGING |
                      CRTC_VFC_SYNC_TRISTATE |
                      CRTC_LOCK_REGS |  /* Already off, but ... */
                      CRTC_SYNC_TRISTATE | CRTC_EXT_DISP_EN |
                      CRTC_DISP_REQ_EN | CRTC_VGA_LINEAR | CRTC_VGA_TEXT_132 |
                      CRTC_CUR_B_TEST);
#if 0           /* This isn't needed, but is kept for reference */
                if (pMode->Flags & V_DBLSCAN)
                    pATIHW->crtc_gen_cntl |= CRTC_DBL_SCAN_EN;
#endif
                if (pMode->Flags & V_INTERLACE)
                    pATIHW->crtc_gen_cntl |= CRTC_INTERLACE_EN;
                if ((pMode->Flags & (V_CSYNC | V_PCSYNC)) || pATI->OptionCSync)
                    pATIHW->crtc_gen_cntl |= CRTC_CSYNC_EN;
                if (pScreenInfo->depth <= 4)
                    pATIHW->crtc_gen_cntl |= CRTC_EN | CRTC_CNT_EN;
                else
                    pATIHW->crtc_gen_cntl |=
                        CRTC_EN | CRTC_VGA_LINEAR | CRTC_CNT_EN;
            }
            break;

        case ATI_CRTC_MACH64:
            /*
             * Fill in Mach64 data.
             */
            ATIMach64Calculate(pScreenInfo, pATI, pATIHW, pMode);
            break;

        default:
            break;
    }

    /* Fill in clock data */
    return ATIClockCalculate(pScreenInfo, pATI, pATIHW, pMode);
}

/*
 * ATICRTCSet --
 *
 * This function sets a video mode.  It writes out all video state data that
 * has been previously calculated or saved.
 */
void
ATICRTCSet
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI,
    ATIHWPtr    pATIHW
)
{
    /* Get back to bank 0 */
    (*pATIHW->SetBank)(pATI, 0);

    if (pATI->Chip >= ATI_CHIP_264LT)
        ATIPutMach64PLLReg(PLL_EXT_VPLL_CNTL, pATIHW->pll_ext_vpll_cntl);

    switch (pATIHW->crtc)
    {
        case ATI_CRTC_VGA:
            /* Stop CRTC */
            if (pATI->Chip >= ATI_CHIP_88800GXC)
                outl(pATI->CPIO_CRTC_GEN_CNTL,
                    pATIHW->crtc_gen_cntl & ~CRTC_EN);

            /* Start sequencer reset */
            PutReg(SEQX, 0x00U, 0x00U);

            /* Set pixel clock */
            if ((pATIHW->FeedbackDivider > 0) &&
                (pATI->ProgrammableClock != ATI_CLOCK_FIXED))
                ATIClockSet(pATI, pATIHW);

            /* Load VGA Wonder */
            if (pATI->CPIO_VGAWonder)
                ATIVGAWonderSet(pATI, pATIHW);

            /* Load VGA device */
            ATIVGASet(pATI, pATIHW);

            /* Load Mach64 registers */
            if (pATI->Chip >= ATI_CHIP_88800GXC)
            {
                outl(pATI->CPIO_CRTC_GEN_CNTL, pATIHW->crtc_gen_cntl);
                outl(pATI->CPIO_MEM_VGA_WP_SEL, pATIHW->mem_vga_wp_sel);
                outl(pATI->CPIO_MEM_VGA_RP_SEL, pATIHW->mem_vga_rp_sel);
                if (pATI->Chip >= ATI_CHIP_264CT)
                {
                    outl(pATI->CPIO_CRTC_OFF_PITCH, pATIHW->crtc_off_pitch);
                    outl(pATI->CPIO_BUS_CNTL, pATIHW->bus_cntl);
                    outl(pATI->CPIO_DAC_CNTL, pATIHW->dac_cntl);
                    outl(pATI->CPIO_CONFIG_CNTL, pATIHW->config_cntl);
                }
            }

            break;

        case ATI_CRTC_MACH64:
            /* Load Mach64 CRTC regsiters */
            ATIMach64Set(pATI, pATIHW);

            if (pATI->UseSmallApertures)
            {
                /* Oddly enough, thses need to be set also, maybe others */
                PutReg(SEQX, 0x02U, pATIHW->seq[2]);
                PutReg(SEQX, 0x04U, pATIHW->seq[4]);
                PutReg(GRAX, 0x06U, pATIHW->gra[6]);
                if (pATI->CPIO_VGAWonder)
                    ATIModifyExtReg(pATI, 0xB6, -1, 0x00U, pATIHW->b6);
            }

            break;

        default:
            break;
    }

    /*
     * Set DSP registers.  Note that sequencer resets clear the DSP_CONFIG
     * register, for some reason.
     */
    if ((pATI->Chip >= ATI_CHIP_264VTB) && (pATI->CPIODecoding == BLOCK_IO))
        ATIDSPSet(pATI, pATIHW);

    /* Load RAMDAC */
    ATIDACSet(pATI, pATIHW);

    /* Restore video memory */
    ATISwap(pScreenInfo, pATI, pATIHW, TRUE);

    if (pATI->VGAAdapter != ATI_ADAPTER_NONE)
        ATIVGASaveScreen(pATI, TRUE);           /* Turn screen back on */

    if ((xf86GetVerbosity() > 3) && (pATIHW == &pATI->NewHW))
    {
        xf86ErrorFVerb(4, "\n After setting mode \"%s\":\n\n",
            pScreenInfo->currentMode->name);
        ATIPrintMode(pScreenInfo->currentMode);
        ATIPrintRegisters(pATI);
    }
}

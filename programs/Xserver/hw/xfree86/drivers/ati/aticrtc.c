/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/aticrtc.c,v 1.8 1999/09/25 14:37:20 dawes Exp $ */
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
    CARD32 lcd_index;

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

    /* Ensure proper VCLK source */
    if (pATI->Chip >= ATI_CHIP_264CT)
    {
        pATIHW->pll_vclk_cntl = ATIGetMach64PLLReg(PLL_VCLK_CNTL) |
            (PLL_VCLK_SRC_SEL | PLL_VCLK_RESET);

        /* For now disable extended reference and feedback dividers */
        if (pATI->Chip >= ATI_CHIP_264LT)
            pATIHW->pll_ext_vpll_cntl = ATIGetMach64PLLReg(PLL_EXT_VPLL_CNTL) &
                ~(PLL_EXT_VPLL_EN | PLL_EXT_VPLL_VGA_EN | PLL_EXT_VPLL_INSYNC);
    }

    /* Initialize CRTC data for LCD panels */
    if (pATI->LCDPanelID >= 0)
    {
        int HDisplay, VDisplay;

        if (pATI->Chip == ATI_CHIP_264LT)
        {
            pATIHW->lcd_gen_ctrl = inl(pATI->CPIO_LCD_GEN_CTRL);
            pATIHW->power_management = inl(pATI->CPIO_POWER_MANAGEMENT);
        }
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            lcd_index = inl(pATI->CPIO_LCD_INDEX);
            pATIHW->lcd_index = (lcd_index &
                ~(LCD_REG_INDEX | LCD_DISPLAY_DIS | LCD_SRC_SEL)) |
                (LCD_SRC_SEL_CRTC1 | LCD_CRTC2_DISPLAY_DIS);
            pATIHW->config_panel =
                ATIGetLTProLCDReg(LCD_CONFIG_PANEL) | DONT_SHADOW_HEND;
            pATIHW->lcd_gen_ctrl = ATIGetLTProLCDReg(LCD_GEN_CNTL);
            pATIHW->power_management = ATIGetLTProLCDReg(LCD_POWER_MANAGEMENT);
            outl(pATI->CPIO_LCD_INDEX, lcd_index);
        }

        pATIHW->lcd_gen_ctrl &=
            ~(CRT_ON | LCD_ON | CRTC_RW_SELECT |
              HORZ_DIVBY2_EN | DIS_HOR_CRT_DIVBY2 |
              USE_SHADOWED_VEND | USE_SHADOWED_ROWCUR |
              SHADOW_EN | SHADOW_RW_EN);
        pATIHW->lcd_gen_ctrl |= DONT_SHADOW_VPAR | LOCK_8DOT;

        /* Disable power management */
        pATIHW->power_management &= ~PWR_MGT_ON;

        if (pATI->OptionCRT)
        {
            /*
             * Use primary CRTC to drive the CRT.  Turn off panel interface.
             */
            pATIHW->lcd_gen_ctrl |= CRT_ON;
        }
        else
        {
            /*
             * Use primary CRTC to drive the panel.  Turn off CRT interface.
             */
            pATIHW->lcd_gen_ctrl |= LCD_ON;

            /*
             * XXX
             *
             * Determine porch data.  This is ugly and will be removed when the
             * common layer's mode validation is changed to better deal with
             * panels.  The intent here is to produce stretched modes that
             * approximate the horizontal sync and vertical refresh rates of
             * the mode on server entry (which, BTW, hasn't been saved yet).
             * The following is inaccurate (but still good enough) when BIOS
             * initialization has set things up so that the registers read here
             * are not the ones actually in use by the panel.
             */
            if (inl(pATI->CPIO_CRTC_GEN_CNTL) & CRTC_EXT_DISP_EN)
            {
                pATIHW->crtc_h_total_disp = inl(pATI->CPIO_CRTC_H_TOTAL_DISP);
                pATIHW->crtc_h_sync_strt_wid =
                    inl(pATI->CPIO_CRTC_H_SYNC_STRT_WID);
                pATIHW->crtc_v_total_disp = inl(pATI->CPIO_CRTC_V_TOTAL_DISP);
                pATIHW->crtc_v_sync_strt_wid =
                    inl(pATI->CPIO_CRTC_V_SYNC_STRT_WID);

                HDisplay = GetBits(pATIHW->crtc_h_total_disp, CRTC_H_DISP);
                VDisplay = GetBits(pATIHW->crtc_v_total_disp, CRTC_V_DISP);

                pATI->LCDHSyncStart =
                    (GetBits(pATIHW->crtc_h_sync_strt_wid,
                             CRTC_H_SYNC_STRT_HI) *
                     (MaxBits(CRTC_H_SYNC_STRT) + 1)) +
                    GetBits(pATIHW->crtc_h_sync_strt_wid, CRTC_H_SYNC_STRT) -
                    HDisplay;
                pATI->LCDHSyncWidth =
                    GetBits(pATIHW->crtc_h_sync_strt_wid, CRTC_H_SYNC_WID);
                pATI->LCDHBlankWidth =
                    GetBits(pATIHW->crtc_h_total_disp, CRTC_H_TOTAL) -
                    HDisplay;
                pATI->LCDVSyncStart =
                    GetBits(pATIHW->crtc_v_sync_strt_wid, CRTC_V_SYNC_STRT) -
                    VDisplay;
                pATI->LCDVSyncWidth =
                    GetBits(pATIHW->crtc_v_sync_strt_wid, CRTC_V_SYNC_WID);
                pATI->LCDVBlankWidth =
                    GetBits(pATIHW->crtc_v_total_disp, CRTC_V_TOTAL) -
                    VDisplay;
            }
            else
            {
                pATIHW->crt[0] = GetReg(CRTX(pATI->CPIO_VGABase), 0x00U);
                pATIHW->crt[1] = GetReg(CRTX(pATI->CPIO_VGABase), 0x01U);
                pATIHW->crt[4] = GetReg(CRTX(pATI->CPIO_VGABase), 0x04U);
                pATIHW->crt[5] = GetReg(CRTX(pATI->CPIO_VGABase), 0x05U);
                pATIHW->crt[6] = GetReg(CRTX(pATI->CPIO_VGABase), 0x06U);
                pATIHW->crt[7] = GetReg(CRTX(pATI->CPIO_VGABase), 0x07U);
                pATIHW->crt[16] = GetReg(CRTX(pATI->CPIO_VGABase), 0x10U);
                pATIHW->crt[17] = GetReg(CRTX(pATI->CPIO_VGABase), 0x11U);
                pATIHW->crt[18] = GetReg(CRTX(pATI->CPIO_VGABase), 0x12U);

                HDisplay = pATIHW->crt[1] + 1;
                VDisplay = (((pATIHW->crt[7] << 3) & 0x0200U) |
                            ((pATIHW->crt[7] << 7) & 0x0100U) |
                            pATIHW->crt[18]) + 1;

                pATI->LCDHSyncStart = pATIHW->crt[4] - HDisplay;
                pATI->LCDHSyncWidth =
                    (pATIHW->crt[5] - pATIHW->crt[4]) & 0x1FU;
                pATI->LCDHBlankWidth = pATIHW->crt[0] + 5 - HDisplay;
                pATI->LCDVSyncStart = (((pATIHW->crt[7] << 2) & 0x0200U) |
                                       ((pATIHW->crt[7] << 6) & 0x0100U) |
                                       pATIHW->crt[16]) - VDisplay;
                pATI->LCDVSyncWidth =
                    (pATIHW->crt[17] - pATIHW->crt[16]) & 0x0FU;
                pATI->LCDVBlankWidth = (((pATIHW->crt[7] << 4) & 0x0200U) |
                                        ((pATIHW->crt[7] << 8) & 0x0100U) |
                                        pATIHW->crt[6]) + 2 - VDisplay;
            }

            pATI->LCDHSyncStart <<= 3;
            pATI->LCDHSyncWidth <<= 3;
            pATI->LCDHBlankWidth <<= 3;
        }
    }

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
    int Index;

    /* Get bank to bank 0 */
    (*pATIHW->SetBank)(pATI, 0);

    /* Save clock data */
    ATIClockSave(pScreenInfo, pATI, pATIHW);

    if (pATI->Chip >= ATI_CHIP_264CT)
    {
        pATIHW->pll_vclk_cntl = ATIGetMach64PLLReg(PLL_VCLK_CNTL) |
            PLL_VCLK_RESET;
        if (pATI->Chip >= ATI_CHIP_264LT)
            pATIHW->pll_ext_vpll_cntl = ATIGetMach64PLLReg(PLL_EXT_VPLL_CNTL);
    }

    /* Save LCD registers */
    if (pATI->LCDPanelID >= 0)
    {
        if (pATI->Chip == ATI_CHIP_264LT)
        {
            pATIHW->horz_stretching = inl(pATI->CPIO_HORZ_STRETCHING);
            pATIHW->vert_stretching = inl(pATI->CPIO_VERT_STRETCHING);
            pATIHW->lcd_gen_ctrl = inl(pATI->CPIO_LCD_GEN_CTRL);
            pATIHW->power_management = inl(pATI->CPIO_POWER_MANAGEMENT);

            /* Set up to save non-shadow registers */
            outl(pATI->CPIO_LCD_GEN_CTRL, pATIHW->lcd_gen_ctrl &
                ~(CRTC_RW_SELECT | SHADOW_EN | SHADOW_RW_EN));
        }
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            pATIHW->lcd_index = inl(pATI->CPIO_LCD_INDEX);
            pATIHW->config_panel = ATIGetLTProLCDReg(LCD_CONFIG_PANEL);
            pATIHW->lcd_gen_ctrl = ATIGetLTProLCDReg(LCD_GEN_CNTL);
            pATIHW->horz_stretching = ATIGetLTProLCDReg(LCD_HORZ_STRETCHING);
            pATIHW->vert_stretching = ATIGetLTProLCDReg(LCD_VERT_STRETCHING);
            pATIHW->ext_vert_stretch = ATIGetLTProLCDReg(LCD_EXT_VERT_STRETCH);
            pATIHW->power_management = ATIGetLTProLCDReg(LCD_POWER_MANAGEMENT);

            /* Set up to save non-shadow registers */
            ATIPutLTProLCDReg(LCD_GEN_CNTL, pATIHW->lcd_gen_ctrl &
                ~(CRTC_RW_SELECT | SHADOW_EN | SHADOW_RW_EN));
        }
    }

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

    if (pATI->LCDPanelID >= 0)
    {
        if (!pATI->OptionCRT)
        {
            /* Switch to shadow registers */
            if (pATI->Chip == ATI_CHIP_264LT)
                outl(pATI->CPIO_LCD_GEN_CTRL,
                    (pATIHW->lcd_gen_ctrl & ~CRTC_RW_SELECT) |
                    (SHADOW_EN | SHADOW_RW_EN));
            else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                        (pATI->Chip == ATI_CHIP_264XL) ||
                        (pATI->Chip == ATI_CHIP_MOBILITY)) */
                ATIPutLTProLCDReg(LCD_GEN_CNTL,
                    (pATIHW->lcd_gen_ctrl & ~CRTC_RW_SELECT) |
                    (SHADOW_EN | SHADOW_RW_EN));

            /* Save shadow VGA CRTC registers */
            for (Index = 0;  Index < NumberOf(pATIHW->shadow_vga);  Index++)
                pATIHW->shadow_vga[Index] =
                    GetReg(CRTX(pATI->CPIO_VGABase), Index);

            /* Save shadow Mach64 CRTC registers */
            pATIHW->shadow_h_total_disp = inl (pATI->CPIO_CRTC_H_TOTAL_DISP);
            pATIHW->shadow_h_sync_strt_wid =
                inl(pATI->CPIO_CRTC_H_SYNC_STRT_WID);
            pATIHW->shadow_v_total_disp = inl (pATI->CPIO_CRTC_V_TOTAL_DISP);
            pATIHW->shadow_v_sync_strt_wid =
                inl(pATI->CPIO_CRTC_V_SYNC_STRT_WID);
        }

        /* Restore CRTC selection and shadow state */
        if (pATI->Chip == ATI_CHIP_264LT)
            outl(pATI->CPIO_LCD_GEN_CTRL, pATIHW->lcd_gen_ctrl);
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            ATIPutLTProLCDReg(LCD_GEN_CNTL, pATIHW->lcd_gen_ctrl);
            outl(pATI->CPIO_LCD_INDEX, pATIHW->lcd_index);
        }
    }

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
    CARD32 lcd_index;
    int Index;

    /* XXX Clobber mode timings */
    if ((pATI->LCDPanelID >= 0) && !pATI->OptionCRT &&
        !pMode->CrtcHAdjusted && !pMode->CrtcVAdjusted)
    {
        int VScan;

        pMode->Flags &= ~(V_DBLSCAN | V_INTERLACE | V_CLKDIV2);

        pMode->HSyncStart = pMode->HDisplay + pATI->LCDHSyncStart;
        pMode->HSyncEnd = pMode->HSyncStart + pATI->LCDHSyncWidth;
        pMode->HTotal = pMode->HDisplay + pATI->LCDHBlankWidth;

        /*
         * Use doublescanning or multiscanning to get around vertical blending
         * limitations.
         */
        VScan = pATI->LCDVertical / pMode->VDisplay;
        switch (pATIHW->crtc)
        {
            case ATI_CRTC_VGA:
                if (VScan > 64)
                    VScan = 64;
                pMode->VScan = VScan;
                break;

            case ATI_CRTC_MACH64:
                pMode->VScan = 0;
                if (VScan <= 1)
                    break;
                VScan = 2;
                pMode->Flags |= V_DBLSCAN;
                break;

            default:
                break;
        }
        pMode->VSyncStart = pMode->VDisplay +
            ATIDivide(pATI->LCDVSyncStart, VScan, 0, 0);
        pMode->VSyncEnd = pMode->VSyncStart +
            ATIDivide(pATI->LCDVSyncWidth, VScan, 0, 1);
        pMode->VTotal = pMode->VDisplay +
            ATIDivide(pATI->LCDVBlankWidth, VScan, 0, 0);
    }

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

    /* Set up LCD register values */
    if (pATI->LCDPanelID >= 0)
    {
        int VDisplay = pMode->VDisplay;

        if (pMode->Flags & V_DBLSCAN)
            VDisplay <<= 1;
        if (pMode->VScan > 1)
            VDisplay *= pMode->VScan;

        if (pATI->Chip == ATI_CHIP_264LT)
        {
            pATIHW->horz_stretching = inl(pATI->CPIO_HORZ_STRETCHING);
            pATIHW->vert_stretching = inl(pATI->CPIO_VERT_STRETCHING);
        }
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            lcd_index = inl(pATI->CPIO_LCD_INDEX);
            pATIHW->horz_stretching = ATIGetLTProLCDReg(LCD_HORZ_STRETCHING);
            pATIHW->vert_stretching = ATIGetLTProLCDReg(LCD_VERT_STRETCHING);
            pATIHW->ext_vert_stretch =
                ATIGetLTProLCDReg(LCD_EXT_VERT_STRETCH) &
                ~(AUTO_VERT_RATIO | VERT_STRETCH_MODE);

            /*
             * Don't use vertical blending if the mode is too wide, too short,
             * or not vertically stretched.
             */
            if (!pATI->OptionCRT &&
                (pMode->HDisplay <= pATI->LCDVBlendFIFOSize) &&
                (VDisplay < pATI->LCDVertical) &&
                (VDisplay >= (pATI->LCDVertical / 2)))
                pATIHW->ext_vert_stretch |= VERT_STRETCH_MODE;

            outl(pATI->CPIO_LCD_INDEX, lcd_index);
        }

        pATIHW->horz_stretching &=
            ~(HORZ_STRETCH_RATIO | HORZ_STRETCH_LOOP | AUTO_HORZ_RATIO |
              HORZ_STRETCH_MODE | HORZ_STRETCH_EN);
        if (!pATI->OptionCRT && (pMode->HDisplay < pATI->LCDHorizontal))
            pATIHW->horz_stretching |= (HORZ_STRETCH_MODE | HORZ_STRETCH_EN) |
                SetBits(((pMode->HDisplay & ~7) *
                         (MaxBits(HORZ_STRETCH_BLEND) + 1)) /
                        pATI->LCDHorizontal, HORZ_STRETCH_BLEND);

        if (pATI->OptionCRT || (VDisplay >= pATI->LCDVertical))
            pATIHW->vert_stretching &= ~VERT_STRETCH_EN;
        else
        {
            pATIHW->vert_stretching &= ~VERT_STRETCH_RATIO0;
            pATIHW->vert_stretching |= (VERT_STRETCH_USE0 | VERT_STRETCH_EN) |
                SetBits((VDisplay * (MaxBits(VERT_STRETCH_RATIO0) + 1)) /
                        pATI->LCDVertical, VERT_STRETCH_RATIO0);
        }

        if (!pATI->OptionCRT)
        {
            /* Copy non-shadow CRTC register values to the shadow set */
            for (Index = 0;  Index < NumberOf(pATIHW->shadow_vga);  Index++)
                pATIHW->shadow_vga[Index] = pATIHW->crt[Index];

            pATIHW->shadow_h_total_disp = pATIHW->crtc_h_total_disp;
            pATIHW->shadow_h_sync_strt_wid = pATIHW->crtc_h_sync_strt_wid;
            pATIHW->shadow_v_total_disp = pATIHW->crtc_v_total_disp;
            pATIHW->shadow_v_sync_strt_wid = pATIHW->crtc_v_sync_strt_wid;
        }
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
    int Index;

    /* Get back to bank 0 */
    (*pATIHW->SetBank)(pATI, 0);

    if (pATI->Chip >= ATI_CHIP_264CT)
    {
        ATIPutMach64PLLReg(PLL_VCLK_CNTL, pATIHW->pll_vclk_cntl);
        ATIPutMach64PLLReg(PLL_VCLK_CNTL,
            pATIHW->pll_vclk_cntl & ~PLL_VCLK_RESET);
        if (pATI->Chip >= ATI_CHIP_264LT)
            ATIPutMach64PLLReg(PLL_EXT_VPLL_CNTL, pATIHW->pll_ext_vpll_cntl);
    }

    /* Load LCD registers */
    if (pATI->LCDPanelID >= 0)
    {
        /* Stop CRTC */
        outl(pATI->CPIO_CRTC_GEN_CNTL, pATIHW->crtc_gen_cntl &
            ~(CRTC_EXT_DISP_EN | CRTC_EN));

        if (pATI->Chip == ATI_CHIP_264LT)
        {
            /* Update non-shadow registers first */
            outl(pATI->CPIO_LCD_GEN_CTRL, pATIHW->lcd_gen_ctrl &
                ~(CRTC_RW_SELECT | SHADOW_EN | SHADOW_RW_EN));

            /* Temporarily disable stretching */
            outl(pATI->CPIO_HORZ_STRETCHING, pATIHW->horz_stretching &
                ~(HORZ_STRETCH_MODE | HORZ_STRETCH_EN));
            outl(pATI->CPIO_VERT_STRETCHING, pATIHW->vert_stretching &
                ~(VERT_STRETCH_RATIO1 | VERT_STRETCH_RATIO2 |
                  VERT_STRETCH_USE0 | VERT_STRETCH_EN));
        }
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            /* Update non-shadow registers first */
            ATIPutLTProLCDReg(LCD_CONFIG_PANEL, pATIHW->config_panel);
            ATIPutLTProLCDReg(LCD_GEN_CNTL, pATIHW->lcd_gen_ctrl &
                ~(CRTC_RW_SELECT | SHADOW_EN | SHADOW_RW_EN));

            /* Temporarily disable stretching */
            ATIPutLTProLCDReg(LCD_HORZ_STRETCHING, pATIHW->horz_stretching &
                ~(HORZ_STRETCH_MODE | HORZ_STRETCH_EN));
            ATIPutLTProLCDReg(LCD_VERT_STRETCHING, pATIHW->vert_stretching &
                ~(VERT_STRETCH_RATIO1 | VERT_STRETCH_RATIO2 |
                  VERT_STRETCH_USE0 | VERT_STRETCH_EN));
        }
    }

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
            /* Load Mach64 CRTC registers */
            ATIMach64Set(pATI, pATIHW);

            if (pATI->UseSmallApertures)
            {
                /* Oddly enough, these need to be set also, maybe others */
                PutReg(SEQX, 0x02U, pATIHW->seq[2]);
                PutReg(SEQX, 0x04U, pATIHW->seq[4]);
                PutReg(GRAX, 0x06U, pATIHW->gra[6]);
                if (pATI->CPIO_VGAWonder)
                    ATIModifyExtReg(pATI, 0xB6U, -1, 0x00U, pATIHW->b6);
            }

            break;

        default:
            break;
    }

    if (pATI->LCDPanelID >= 0)
    {
        if (!pATI->OptionCRT &&
            (!pATI->OptionDevel || (pATIHW == &pATI->OldHW)))
        {
            /* Switch to shadow registers */
            if (pATI->Chip == ATI_CHIP_264LT)
                outl(pATI->CPIO_LCD_GEN_CTRL,
                    (pATIHW->lcd_gen_ctrl & ~CRTC_RW_SELECT) |
                    (SHADOW_EN | SHADOW_RW_EN));
            else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                        (pATI->Chip == ATI_CHIP_264XL) ||
                        (pATI->Chip == ATI_CHIP_MOBILITY)) */
                ATIPutLTProLCDReg(LCD_GEN_CNTL,
                    (pATIHW->lcd_gen_ctrl & ~CRTC_RW_SELECT) |
                    (SHADOW_EN | SHADOW_RW_EN));

            /* Restore shadow registers */
            switch (pATIHW->crtc)
            {
                case ATI_CRTC_VGA:
                    for (Index = 0;
                         Index < NumberOf(pATIHW->shadow_vga);
                         Index++)
                        PutReg(CRTX(pATI->CPIO_VGABase), Index,
                            pATIHW->shadow_vga[Index]);
                    break;

                case ATI_CRTC_MACH64:
                    outl(pATI->CPIO_CRTC_H_TOTAL_DISP,
                        pATIHW->shadow_h_total_disp);
                    outl(pATI->CPIO_CRTC_H_SYNC_STRT_WID,
                        pATIHW->shadow_h_sync_strt_wid);
                    outl(pATI->CPIO_CRTC_V_TOTAL_DISP,
                        pATIHW->shadow_v_total_disp);
                    outl(pATI->CPIO_CRTC_V_SYNC_STRT_WID,
                        pATIHW->shadow_v_sync_strt_wid);
                    break;

                default:
                    break;
            }
        }

        /* Restore CRTC selection & shadow state and enable stretching */
        if (pATI->Chip == ATI_CHIP_264LT)
        {
            outl(pATI->CPIO_LCD_GEN_CTRL, pATIHW->lcd_gen_ctrl);
            outl(pATI->CPIO_HORZ_STRETCHING, pATIHW->horz_stretching);
            outl(pATI->CPIO_VERT_STRETCHING, pATIHW->vert_stretching);
            outl(pATI->CPIO_POWER_MANAGEMENT, pATIHW->power_management);
        }
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            ATIPutLTProLCDReg(LCD_GEN_CNTL, pATIHW->lcd_gen_ctrl);
            ATIPutLTProLCDReg(LCD_HORZ_STRETCHING, pATIHW->horz_stretching);
            ATIPutLTProLCDReg(LCD_VERT_STRETCHING, pATIHW->vert_stretching);
            ATIPutLTProLCDReg(LCD_EXT_VERT_STRETCH, pATIHW->ext_vert_stretch);
            ATIPutLTProLCDReg(LCD_POWER_MANAGEMENT, pATIHW->power_management);
            outl(pATI->CPIO_LCD_INDEX, pATIHW->lcd_index);
        }
    }

    /*
     * Set DSP registers.  Note that, for some reason, sequencer resets clear
     * the DSP_CONFIG register on early integrated controllers.
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

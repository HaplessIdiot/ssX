/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atividmem.c,v 1.3 1999/07/06 11:38:40 dawes Exp $ */
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
#include "atistruct.h"
#include "atividmem.h"

/* Memory types for 68800's and 88800GX's */
const char *ATIMemoryTypeNames_Mach[] =
{
    "DRAM (256Kx4)",
    "VRAM (256Kx4, x8, x16)",
    "VRAM (256Kx16 with short shift register)",
    "DRAM (256Kx16)",
    "Graphics DRAM (256Kx16)",
    "Enhanced VRAM (256Kx4, x8, x16)",
    "Enhanced VRAM (256Kx16 with short shift register)",
    "Unknown video memory type"
};

/* Memory types for 88800CX's */
const char *ATIMemoryTypeNames_88800CX[] =
{
    "DRAM (256Kx4, x8, x16)",
    "EDO DRAM (256Kx4, x8, x16)",
    "Unknown video memory type",
    "DRAM (256Kx16 with assymetric RAS/CAS)",
    "Unknown video memory type",
    "Unknown video memory type",
    "Unknown video memory type",
    "Unknown video memory type"
};

/* Memory types for 264xT's */
const char *ATIMemoryTypeNames_264xT[] =
{
    "Disabled video memory",
    "DRAM",
    "EDO DRAM",
    "Pseudo-EDO DRAM",
    "SDRAM",
    "SGRAM",
    "Unknown video memory type",
    "Unknown video memory type"
};

/*
 * ATIMapApertures --
 *
 * This function maps all apertures used by the driver.
 */
Bool
ATIMapApertures
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    if (pATI->Mapped)
        return TRUE;

    /* Map VGA aperture */
    if (pATI->VGAAdapter != ATI_ADAPTER_VGA)
    {
        pATI->pBank = xf86MapVidMem(pScreenInfo->scrnIndex, VIDMEM_MMIO,
            0x000A0000U, 0x00010000U);
        if (!pATI->pBank)
            return FALSE;
        pATI->pMemory =
            pATI->BankInfo.pBankA =
            pATI->BankInfo.pBankB = pATI->pBank;
    }

    /* Map linear aperture */
    if (pATI->LinearBase)
    {
        if (pATI->PCIInfo)
            pATI->pMemory = xf86MapPciMem(pScreenInfo->scrnIndex,
                VIDMEM_FRAMEBUFFER,
                ((pciConfigPtr)(pATI->PCIInfo->thisCard))->tag,
                pATI->LinearBase, pATI->LinearSize);
        else
            pATI->pMemory = xf86MapVidMem(pScreenInfo->scrnIndex,
                VIDMEM_FRAMEBUFFER, pATI->LinearBase, pATI->LinearSize);
        if (!pATI->pMemory)
        {
            if (pATI->pBank)
            {
                xf86UnMapVidMem(pScreenInfo->scrnIndex, pATI->pBank,
                    0x00010000U);
                pATI->pBank = NULL;
            }
            return FALSE;
        }
    }

    pATI->Mapped = TRUE;
    return TRUE;
}

/*
 * ATIUnmapApertures --
 *
 * This function unmaps all apertures used by the driver.
 */
void
ATIUnmapApertures
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI
)
{
    if (!pATI->Mapped)
        return;
    pATI->Mapped = FALSE;

    /* Unmap linear aperture */
    if (pATI->pMemory != pATI->pBank)
        xf86UnMapVidMem(pScreenInfo->scrnIndex, pATI->pMemory,
            pATI->LinearSize);

    /* Unmap VGA aperture */
    if (pATI->pBank)
        xf86UnMapVidMem(pScreenInfo->scrnIndex, pATI->pBank, 0x00010000U);

    pATI->pMemory = pATI->pBank = NULL;
    return;
}

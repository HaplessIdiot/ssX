/* $XFree86$ */
/*
 * Copyright 1994 by Marc Aurele La France (TSI @ UQV), tsi@gpu.srv.ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Marc Aurele La France not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Marc Aurele La France makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
 
/*************************************************************************/
 
/*
 * Author:  Marc Aurele La France (TSI @ UQV), tsi@gpu.srv.ualberta.ca
 *
 * This is a rewrite of the ATI VGA Wonder driver included with XFree86 2.0,
 * 2.1 and 3.0.  Contributions to the previous versions of this driver by the
 * following people are hereby acknowledged:
 *
 * Thomas Roell, roell@informatik.tu-muenchen.de
 * Per Lindqvist, pgd@compuram.bbt.se
 * Doug Evans, dje@cygnus.com
 * Rik Faith, faith@cs.unc.edu
 * Arthur Tateishi, ruhtra@turing.toronto.edu
 * Alain Hebert, aal@broue.rot.qc.ca
 *
 * ... and, doubtless, many others whom I do not know about.
 *
 * Additional acknowledgements are due to:
 *
 * Ton van Rosmalen, ton@stack.urc.tue.nl, for debugging assistance on V3 and
 *    V5 boards.
 * David Chambers, davidc@netcom.com, for providing an old V3 board.
 *
 * This rewrite exists to fix interlacing, virtual console switching, virtual
 * window scrolling and clock selection, implement clock probing, allow for an
 * external clock selection program, enhance support for V3 boards, add support
 * for the monochrome and 16-colour servers and lots of other nitpicky reasons
 * I don't recall right now.
 *
 * The driver is intended to support ATI VGA WONDER V3, V4, V5, PLUS, XL and
 * XL24 boards.  It will also work with Mach8 and Mach32 boards but will not
 * use their accelerated features.
 *
 * The ATI x8800 chips use special registers for their extended features.
 * These registers are accessible through an index I/O port and a data I/O
 * port.  The index port number is found in the 16-bit integer at real address
 * 0x000C0010 (which is usually 0x01CE) and the data port is one more
 * (usually 0x01CF).  These ports differ in their I/O behaviour from the
 * normal VGA ones:
 *
 *      write:  outw(0x01CE, (data << 8) | index);
 *      read:   outb(0x01CE, index);  data = inb(0x01CF);
 *
 * Two consecutive byte-writes are NOT allowed.  Furthermore an index
 * written to 0x01CE is only usable ONCE!  Note also that the setting of ATI
 * extended registers (especially those with clock selection bits) should be
 * bracketed by a sequencer reset.
 *
 * Boards prior to V5 use 4 crystals.  Boards V5 and later use a clock
 * generator chip.  V3 and V4 boards differ when it comes to choosing clock
 * frequencies.
 *
 *      V3/V4 Board Clock Frequencies
 *      R E G I S T E R S
 *      1CE(*)     3C2      3C2         Frequency
 *      B2h/BEh
 *      Bit 6/4   Bit 3    Bit 2        (MHz)
 *      -------  -------  -------       -------
 *         0        0        0          50.175
 *         0        0        1          56.644
 *         0        1        0          Spare 1
 *         0        1        1          44.900
 *         1        0        0          44.900
 *         1        0        1          50.175
 *         1        1        0          Spare 2
 *         1        1        1          36.000
 *
 *      (*):  V3 uses index B2h, bit 6;  V4 uses index BEh, bit 4
 *
 * V5, PLUS, XL and XL24 usually have an ATI 18810 clock generator chip, but
 * some have an ATI 18811-0, and it's quite conceivable that some exist with
 * ATI 18811-1's or ATI 18811-2's.  ATI says there is no reliable way for the
 * driver to determine which clock generator is on the board (their BIOS's are
 * tailored to the board).
 *
 *      V5/PLUS/XL/XL24 Board Clock Frequencies
 *      R E G I S T E R S
 *        1CE      1CE      3C2      3C2        Frequency
 *        B9h      BEh                          (MHz)             18811-1
 *       Bit 1    Bit 4    Bit 3    Bit 2        18810   18811-0  18811-2
 *      -------  -------  -------  -------      -------  -------  -------
 *         0        0        0        0          30.240   30.240  135.000
 *         0        0        0        1          32.000   32.000   32.000
 *         0        0        1        0          37.500  110.000  110.000
 *         0        0        1        1          39.000   80.000   80.000
 *         0        1        0        0          42.954   42.954  100.000
 *         0        1        0        1          48.771   48.771  126.000
 *         0        1        1        0            (*1)   92.400   92.400
 *         0        1        1        1          36.000   36.000   36.000
 *         1        0        0        0          40.000   39.910   39.910
 *         1        0        0        1          56.644   44.900   44.900
 *         1        0        1        0          75.000   75.000   75.000
 *         1        0        1        1          65.000   65.000   65.000
 *         1        1        0        0          50.350   50.350   50.350
 *         1        1        0        1          56.640   56.640   56.640
 *         1        1        1        0            (*2)     (*3)     (*3)
 *         1        1        1        1          44.900   44.900   44.900
 *
 *  (*1) External 0 (supposedly 16.657 Mhz)
 *  (*2) External 1 (supposedly 28.322 MHz)
 *  (*3) This setting doesn't seem to generate anything
 *
 * For all boards, these frequencies can be divided by 1, 2, 3 or 4.
 *
 *      Register 1CE, index B8h
 *       Bit 7    Bit 6
 *      -------  -------
 *         0        0           Divide by 1
 *         0        1           Divide by 2
 *         1        0           Divide by 3
 *         1        1           Divide by 4
 *
 * Because 36 MHz is the only (undivided) frequency available at the same index
 * on all boards, it will be used for calibration during clock probing.  This
 * replaces vgaGetClocks's assumption that clock 1 is 28.322 MHz.
 *
 * There is some question as to whether or not bit 1 of index 0xB9 can
 * be used for clock selection on a V4 board.  This driver makes it
 * available only if the "undocumented_clocks" option (itself
 * undocumented :-)) is specified in Xconfig.
 *
 * Also it appears that bit 0 of index 0xB9 can also be used for clock
 * selection on some boards.  It is also only available under Xconfig
 * option "undocumented_clocks".
 */
 
/*************************************************************************/
 
/*
 * These are X and server generic header files.
 */
#include "X.h"
#include "input.h"
#include "screenint.h"
 
/*
 * These are XFree86-specific header files.
 */
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "vga.h"
 
/*
 * Driver data structures.
 */
typedef struct {
        vgaHWRec std;               /* good old IBM VGA */
 
        unsigned char a3, a6, a7, ac, ad, ae,
                      b0, b1, b2, b3, b4, b5, b6, b8, b9, ba, bd, be;
} vgaATIRec, *vgaATIPtr;
 
/*
 * Forward definitions for the functions that make up the driver.  See
 * the definitions of these functions for the real scoop.
 */
static Bool     ATIProbe();
static char *   ATIIdent();
static Bool     ATIClockSelect();
static void     ATIEnterLeave();
static Bool     ATIInit();
static void *   ATISave();
static void     ATIRestore();
static void     ATIAdjust();
/*
 * These are the bank select functions.  They are defined in bank.s.
 */
extern void     ATISetRead();
extern void     ATISetWrite();
extern void     ATISetReadWrite();
/*
 * Bank selection functions for V3 boards.  These are also defined in bank.s.
 */
extern void     ATIV3SetRead();
extern void     ATIV3SetWrite();
extern void     ATIV3SetReadWrite();
 
/*
 * This data structure defines the driver itself.  The data structure is
 * initialized with the functions that make up the driver and some data
 * that defines how the driver operates.
 */
vgaVideoChipRec ATI = {
        ATIProbe,               /* Probe */
        ATIIdent,               /* Ident */
        ATIEnterLeave,          /* EnterLeave */
        ATIInit,                /* Init */
        ATISave,                /* Save */
        ATIRestore,             /* Restore */
        ATIAdjust,              /* Adjust */
        (void (*)())NoopDDA,    /* SaveScreen */
        (void (*)())NoopDDA,    /* GetMode */
        (void (*)())NoopDDA,    /* FbInit */
        ATISetRead,             /* SetRead */
        ATISetWrite,            /* SetWrite */
        ATISetReadWrite,        /* SetReadWrite */
        0x10000,                /* Mapped memory window size (64k) */
        0x10000,                /* Video memory bank size (64k) */
        16,                     /* Shift factor to get bank number */
        0xFFFF,                 /* Bit mask for address within a bank */
        0x00000, 0x10000,       /* Boundaries for reads within a bank */
        0x00000, 0x10000,       /* Boundaries for writes within a bank */
        TRUE,                   /* Uses two banks */
        VGA_DIVIDE_VERT,        /* Divide interlaced vertical timings */
        {0,},                   /* Options are set by ATIProbe */
        16,                     /* Virtual X rounding */
};
 
/*
 * This is a convenience macro, so that entries in the driver structure
 * can simply be dereferenced with 'new->xxx'.
 */
#define new ((vgaATIPtr)vgaNewVideoState)
 
/*
 * This driver needs two non-standard I/O ports.  These are determined
 * by ATIProbe and are initialized to their most probable values here.
 */
static unsigned ATI_IOPorts[2] = {0x01CE, 0x01CF};
static int Num_ATI_IOPorts =
        (sizeof(ATI_IOPorts)/sizeof(ATI_IOPorts[0]));
short ATIExtReg = 0x01CE;       /* Used by bank.s;  Must be short */
 
/*
 * Handy macros to read and write extended registers.
 */
#define ATIGetExtReg(Index)             \
        (                               \
                outb(ATIExtReg, Index), \
                inb(ATIExtReg + 1)      \
        )
#define ATIPutExtReg(Index, Register_Value)     \
        outw(ATIExtReg, ((Register_Value) << 8) | Index)
 
static unsigned char ATIBoard;
#define ATI_BOARD_V3    0       /* ATIBoard values;  Keep chronological */
#define ATI_BOARD_V4    1
#define ATI_BOARD_V5    2
#define ATI_BOARD_PLUS  3
#define ATI_BOARD_XL    4
#define ATI_BOARD_XL24  5
 
static unsigned char saved_b9_bits_0_and_1 = 0;
 
/*
 * ATIIdent --
 *
 * Returns a string name for this driver or NULL.
 */
static char *
ATIIdent(n)
int n;
{
        static char *chipsets[] = {"ati", "vgawonder"};
 
        if (n >= (sizeof(chipsets) / sizeof(char *)))
                return (NULL);
        else
                return (chipsets[n]);
}

#if 1   /* For now */
/*
 * ATIV3Delay --
 *
 * A utility routine for ATIV3SetB2 below.
 */
static void
ATIV3Delay()
{
        int counter;
        for (counter = 0;  counter < 512;  counter++)
                /* (void) inb(vgaIOBase + 0x0A) */;
}

/*
 * ATIV3SetB2 --
 *
 * This is taken from ATI's programmer's reference manual which says that this
 * is needed to "ensure the proper state of the 8/16 bit ROM toggle".  I
 * suspect that a timing glitch appeared in the 18800 after its die was cast.
 * 18800-1 and later chips do not exhibit this problem. 
 *
 * This routine is called for a V3 board just before changing the 0x40 bit of
 * extended register 0xB2 from a one to a zero.
 */
static void
ATIV3SetB2(old_b2, new_b2)
unsigned char old_b2, new_b2;
{
        if ((new_b2 ^ 0x40) & old_b2 & 0x40)
        {
                unsigned char misc = inb(0x3CC);
		unsigned char tmp = ATIGetExtReg(0xBB);
                outb(0x3C2, (misc & 0xF3) | 0x04 |
                        ((tmp & 0x10) >> 1));
                ATIPutExtReg(0xB2, old_b2 & 0xBF);
                ATIV3Delay();
                outb(0x3C2, misc);
                ATIV3Delay();
        }
        ATIPutExtReg(0xB2, new_b2);
}
#else
#define ATIV3SetB2(old_b2, new_b2)      ATIPutExtReg(0xB2, new_b2)
#endif
 
/*
 * ATIClockSelect --
 *
 * This function selects the dot-clock with index 'no'.  In most cases
 * this is done my setting the correct bits in various registers (generic
 * VGA uses two bits in the Miscellaneous Output Register to select from
 * 4 clocks).  Care must be taken to protect any other bits in these
 * registers by fetching their values and masking off the other bits.
 */
static Bool
ATIClockSelect(no)
int no;
{
        static unsigned char saved_misc,
                saved_b2, saved_b5, saved_b8, saved_b9, saved_be;
        unsigned char misc, b9;
        static unsigned char previous_b2;
 
        switch(no)
        {
                case CLK_REG_SAVE:
                        /*
                         * Here all of the registers that can be affected by
                         * clock setting are saved into static variables.
                         */
                        saved_misc = inb(0x3CC);
                        saved_b5 = ATIGetExtReg(0xB5);
                        saved_b8 = ATIGetExtReg(0xB8);
                        saved_b9 = ATIGetExtReg(0xB9);

                        /*
                         * Ensure clock divider is enabled.
                         */
                        ATIPutExtReg(0xB5, (saved_b5 & 0x7F)       );

                        if (ATIBoard == ATI_BOARD_V3)
                        {
                                saved_b2 = ATIGetExtReg(0xB2);
                                previous_b2 = saved_b2;
                        }
                        else
                                saved_be = ATIGetExtReg(0xBE);
                        break;
                case CLK_REG_RESTORE:
                        /*
                         * Here all the previously saved registers are
                         * restored.
                         */
                        outw(0x3C4, 0x0100);    /* Start synchronous reset */
 
                        if (ATIBoard == ATI_BOARD_V3)
                                ATIV3SetB2(previous_b2, saved_b2);
                        else
                                ATIPutExtReg(0xBE, saved_be);
                        ATIPutExtReg(0xB5, saved_b5);
                        ATIPutExtReg(0xB9, saved_b9);
                        ATIPutExtReg(0xB8, saved_b8);
 
                        outb(0x3C2, saved_misc);
                        outw(0x3C4, 0x0300);    /* End synchronous reset */
 
                        break;
                default:
                        /*
                         * Set the generic two low-order bits of the clock
                         * select.
                         */
                        misc = (inb(0x3CC) & 0xF3) | ((no << 2) & 0x0C);
 
                        b9 = ATIGetExtReg(0xB9);
 
                        /*
                         * Set the high order bits.
                         */
                        if (ATIBoard == ATI_BOARD_V3)
                        {
                                unsigned char new_b2 = 
                                        (previous_b2 & 0xBF) |
                                                ((no << 4) & 0x40);
                                ATIV3SetB2(previous_b2, new_b2);
                                previous_b2 = new_b2;
                        }
                        else
                        {
				unsigned char tmp = ATIGetExtReg(0xBE);
                                ATIPutExtReg(0xBE,
                                        (tmp & 0xEF) | ((no << 2) & 0x10));
                                if ((ATIBoard != ATI_BOARD_V4) ||
                                    (OFLG_ISSET(OPTION_UNDOC_CLKS,
                                        &vga256InfoRec.options)))
                                {
                                        b9 = (b9 & 0xFD) | ((no >> 2) & 0x02);
                                        no >>= 1;
                                }
                        }
                        if (OFLG_ISSET(OPTION_UNDOC_CLKS,
                                &vga256InfoRec.options))
                        {
                                b9 = (b9 & 0xFE) | ((no >> 3) & 0x01);
                                b9 ^= saved_b9_bits_0_and_1;
                                no >>= 1;
                        }
                        ATIPutExtReg(0xB9, b9);
 
                        /*
                         * Set clock divider bits.
                         */
                        ATIPutExtReg(0xB8, (no << 3) & 0xC0);
 
                        /*
                         * Must set miscellaneous output register last.
                         */
                        outb(0x3C2, misc);
 
                        break;
        }
        return (TRUE);
}
 
/*
 * ATIProbe --
 *
 * This is the function that makes a yes/no decision about whether or not
 * a chipset supported by this driver is present or not.  The server will
 * call each driver's probe function in sequence, until one returns TRUE
 * or they all fail.
 */
static Bool
ATIProbe()
{
#       define BIOS_DATA_SIZE   0x50
        unsigned char BIOS_Data[BIOS_DATA_SIZE];
#       define Signature        "761295520"
#       define Signature_Offset 0x31
#       define BIOS_Signature   &BIOS_Data[Signature_Offset]
#       define Signature_Size   9
 
        /*
         * Get out if this isn't the driver the user wants.
         */
        if (vga256InfoRec.chipset)
                if (!StrCaseCmp(vga256InfoRec.chipset, ATIIdent(0)))
                {
                        ErrorF(
            "Driver specification changed from \"ati\" to \"vgawonder\"\n");
                        OFLG_CLR(XCONFIG_CHIPSET, &vga256InfoRec.xconfigFlag);
                        if (vga256InfoRec.clocks)
                        {
                                vga256InfoRec.clocks = 0;
                                OFLG_CLR(XCONFIG_CLOCKS,
                                        &vga256InfoRec.xconfigFlag);
                                if (!OFLG_ISSET(OPTION_PROBE_CLKS,
                                        &vga256InfoRec.options))
                                        ErrorF("Clocks will be probed\n");
                        }
                }
                else if (StrCaseCmp(vga256InfoRec.chipset, ATIIdent(1)))
                        return (FALSE);
 
        /*
         * Get BIOS data this driver will use.
         */
        if (xf86ReadBIOS(vga256InfoRec.BIOSbase, 0, BIOS_Data,
                         sizeof(BIOS_Data)) != sizeof(BIOS_Data))
                return (FALSE);
 
        /*
         * Get out if this is the wrong driver for installed chipset.
         */
        if (strncmp(Signature, (char *)BIOS_Signature, Signature_Size))
                return (FALSE);
 
        ErrorF("ATI BIOS Information:\n");
        ErrorF("   Chip version: %c = ", BIOS_Data[0x43]);
        switch (BIOS_Data[0x43])
        {
                case '1':
                        ErrorF("ATI 18800");
                        ATIBoard = ATI_BOARD_V3;
                        /* Reset a few things for V3 boards */
                        ATI.ChipSetRead = ATIV3SetRead;
                        ATI.ChipSetWrite = ATIV3SetWrite;
                        ATI.ChipSetReadWrite = ATIV3SetReadWrite;
                        ATI.ChipUse2Banks = FALSE;
                        break;
                case '2':
                        ErrorF("ATI 18800-1");
                        /* Is a clock chip present? */
                        if (BIOS_Data[0x42] & 0x10)
                                ATIBoard = ATI_BOARD_V5;
                        else
                                ATIBoard = ATI_BOARD_V4;
                        break;
                case '3':
                        ErrorF("ATI 28800-2");
                        ATIBoard = ATI_BOARD_PLUS;
                        break;
                case '4':
                        ErrorF("ATI 28800-4");
                        ATIBoard = ATI_BOARD_PLUS;
                        break;
                case '5':
                        ErrorF("ATI 28800-5");
                        /* Is there a 32K colour DAC on board? */
                        if (BIOS_Data[0x44] & 0x80)
                                ATIBoard = ATI_BOARD_XL;
                        else
                                ATIBoard = ATI_BOARD_PLUS;
                        break;
                case '6':
                        ErrorF("ATI 28800-6");
                        ATIBoard = ATI_BOARD_XL24;
                        break;
                case 'a':
                        ErrorF("Mach-32");
                        /* Is there a 32K colour DAC on board? */
                        if (BIOS_Data[0x44] & 0x80)
                                ATIBoard = ATI_BOARD_XL;
                        else
                                ATIBoard = ATI_BOARD_PLUS;
                        break;
                default:
                        ErrorF("Unknown!");
                        /* Is there a 32K colour DAC on board? */
                        if (BIOS_Data[0x44] & 0x80)
                                ATIBoard = ATI_BOARD_XL;
                        else
                                ATIBoard = ATI_BOARD_PLUS;
                        break;
        }
 
        ErrorF("\n   This video adapter is a VGA WONDER ");
        switch (ATIBoard)
        {
                case ATI_BOARD_V3:
                        ErrorF("V3");
                        break;
                case ATI_BOARD_V4:
                        ErrorF("V4");
                        break;
                case ATI_BOARD_V5:
                        ErrorF("V5");
                        break;
                case ATI_BOARD_PLUS:
                        ErrorF("PLUS (V6)");
                        break;
                case ATI_BOARD_XL:
                        ErrorF("XL (V7)");
                        break;
                case ATI_BOARD_XL24:
                        ErrorF("XL24 (V8)");  /* sic */
                        break;
                default:
                        ErrorF("Unknown");
                        break;
        }
        ErrorF("\n");
 
        /*
         * Set up extended register addressing.
         */
        ATIExtReg = *((short int *)BIOS_Data + 0x08);
        ATI_IOPorts[0] = ATIExtReg;
        ATI_IOPorts[1] = ATIExtReg + 1;
 
        /*
         * Set up I/O ports to be used by this driver.
         */
        xf86ClearIOPortList(vga256InfoRec.scrnIndex);
        xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
        xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_ATI_IOPorts, ATI_IOPorts);
 
        ATIEnterLeave(ENTER);           /* Unlock registers */
 
        if (OFLG_ISSET(OPTION_UNDOC_CLKS, &vga256InfoRec.options))
        {
                /*
                 * Remember initial settings of undocumented clock selection
                 * bits.
                 */
                saved_b9_bits_0_and_1 = ATIGetExtReg(0xB9);
                if (ATIBoard == ATI_BOARD_V4)
                        saved_b9_bits_0_and_1 &= 0x03;
                else
                        saved_b9_bits_0_and_1 &= 0x01;
        }
 
        /*
         * If the user has specified the amount of memory in the Xconfig
         * file, we respect that setting.
         */
        if (!vga256InfoRec.videoRam)
        {
                /*
                 * Otherwise, probe for the value.
                 */
                if (ATIBoard < ATI_BOARD_PLUS)
                        vga256InfoRec.videoRam =
                                (ATIGetExtReg(0xBB) & 0x20) ? 512 : 256;
                else switch (ATIGetExtReg(0xB0) & 0x18)
                {
                        case 0x00:
                                vga256InfoRec.videoRam = 256;
                                break;
                        case 0x10:
                                vga256InfoRec.videoRam = 512;
                                break;
                        default:
                                vga256InfoRec.videoRam = 1024;
                                break;
                }
        }
 
        /*
         * Again, if the user has specified the clock values in the Xconfig
         * file, we respect those choices.
         */
        if ((!vga256InfoRec.clocks) ||
            (OFLG_ISSET(OPTION_PROBE_CLKS, &vga256InfoRec.options)))
        {
                int Number_Of_Clocks;
 
                /*
                 * Determine the number of clock values to probe for.
                 */
                if (ATIBoard <= ATI_BOARD_V4)
                        Number_Of_Clocks = 8*4;
                else
                        Number_Of_Clocks = 16*4;
                if (OFLG_ISSET(OPTION_UNDOC_CLKS, &vga256InfoRec.options))
                        Number_Of_Clocks <<= 1 + (ATIBoard == ATI_BOARD_V4);
                if (Number_Of_Clocks > MAXCLOCKS)
                        Number_Of_Clocks = MAXCLOCKS;
 
                /*
                 * Probe the board for clock values.  Note that vgaGetClocks
                 * cannot be used for this purpose because it assumes clock
                 * 1 is 28.322 MHz.  Instead call xf86GetClocks directly
                 * passing it slighly different parameters.
                 */
                xf86GetClocks(Number_Of_Clocks, ATIClockSelect,
                        vgaProtect, (void (*)())vgaSaveScreen,
                        (vgaIOBase + 0x0A), 0x08,
                        7, 36000,
                        &vga256InfoRec);
        }
 
        /*
         * Set the maximum allowable dot-clock frequency (in kHz).
         */
        vga256InfoRec.maxClock = 80000;
 
        /*
         * Set chipset name.
         */
        vga256InfoRec.chipset = ATIIdent(1);
 
        /*
         * Tell monochrome and 16-colour servers banked operation is
         * supported.
         */
        vga256InfoRec.bankedMono = TRUE;
 
        /*
         * Indicate supported options.
         */
        OFLG_SET(OPTION_PROBE_CLKS, &ATI.ChipOptionFlags);
        OFLG_SET(OPTION_UNDOC_CLKS, &ATI.ChipOptionFlags);
 
        /*
         * Return success.
         */
        return (TRUE);
}
 
/*
 * ATIEnterLeave --
 *
 * This function is called when the virtual terminal on which the server
 * is running is entered or left, as well as when the server starts up
 * and is shut down.  Its function is to obtain and relinquish I/O
 * permissions for the SVGA device.  This includes unlocking access to
 * any registers that may be protected on the chipset, and locking those
 * registers again on exit.
 */
static void
ATIEnterLeave(enter)
Bool enter;
{
        static unsigned char saved_b8;
 
        if (enter)
        {
		unsigned char tmp;

                xf86EnableIOPorts(vga256InfoRec.scrnIndex);
 
                vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
 
                /* Unprotect CRTC[0-7] */
                outb(vgaIOBase + 4, 0x11);
		tmp = inb(vgaIOBase + 5);
                outb(vgaIOBase + 5, tmp & 0x7F);
 
                /* Unprotect ATI extended registers */
                saved_b8 = ATIGetExtReg(0xB8);
                ATIPutExtReg(0xB8, saved_b8 & 0xC0);
        }
        else
        {
		unsigned char tmp;
                /* Protect CRTC[0-7] */
                outb(vgaIOBase + 4, 0x11);
		tmp = inb(vgaIOBase + 5);
                outb(vgaIOBase + 5, (tmp & 0x7F) | 0x80);
 
                /* Protect ATI extended registers */
		tmp = ATIGetExtReg(0xB8);
                ATIPutExtReg(0xB8,
                        (saved_b8 & 0x3F) | (tmp & 0xC0));
 
                xf86DisableIOPorts(vga256InfoRec.scrnIndex);
        }
 
}
 
/*
 * ATIRestore --
 *
 * This function restores a video mode.  It basically writes out all of
 * the registers that have previously been saved in the vgaATIRec data
 * structure.
 *
 * Note that "Restore" is slightly incorrect.  This function is also used when
 * the server enters/changes video modes.  The mode definitions have previously
 * been initialized by the Init() function, below.
 */
static void
ATIRestore(restore)
vgaATIPtr restore;
{
        unsigned char b2;
	unsigned char tmp;
        /*
         * Unlock ATI extended registers.
         */
	tmp = ATIGetExtReg(0xB8);
        ATIPutExtReg(0xB8, tmp & 0xC0);
 
        /*
         * Get (back) to bank 0.
         */
        if (ATIBoard == ATI_BOARD_V3)
        {
                b2 = ATIGetExtReg(0xB2);
                ATIPutExtReg(0xB2, b2 & 0xE1);
        }
        else
                ATIPutExtReg(0xB2, 0);
 
        /*
         * Restore ATI registers.
         *
         * A special case - when using an external clock-setting program,
         * clock selection bits must not be changed.  This condition can
         * be checked by the condition:
         *
         *      if (restore->std.NoClock >= 0)
         *              restore clock-select bits.
         */
 
        if (restore->std.NoClock < 0)
        {
                unsigned char b9 = ATIGetExtReg(0xB9);
 
                /*
                 * Retrieve current setting of clock select bits.
                 */
                restore->b8 = (restore->b8 & 0x3F) |
                        (ATIGetExtReg(0xB8) & 0xC0);
                if (ATIBoard == ATI_BOARD_V3)
                        restore->b2 = (restore->b2 & 0xBF) | (b2 & 0x40);
                else
                {
                        restore->be = (restore->be & 0xEF) |
                                (ATIGetExtReg(0xBE) & 0x10);
                        if ((ATIBoard != ATI_BOARD_V4) ||
                            (OFLG_ISSET(OPTION_UNDOC_CLKS,
                                &vga256InfoRec.options)))
                        {
                                restore->b9 = (restore->b9 & 0xFD) |
                                        (b9 & 0x02);
                        }
                }
                if (OFLG_ISSET(OPTION_UNDOC_CLKS, &vga256InfoRec.options))
                        restore->b9 = (restore->b9 & 0xFE) | (b9 & 0x01);
        }
 
        outw(0x3C4, 0x0100);    /* Start synchronous reset */
 
        if (ATIBoard == ATI_BOARD_V3)
                ATIV3SetB2(b2, restore->b2);
        else
        {
                ATIPutExtReg(0xB2, restore->b2);
                ATIPutExtReg(0xBE, restore->be);
                if (ATIBoard > ATI_BOARD_V5)
                {
                        ATIPutExtReg(0xA3, restore->a3);
                        ATIPutExtReg(0xA6, restore->a6);
                        ATIPutExtReg(0xA7, restore->a7);
                        ATIPutExtReg(0xAC, restore->ac);
                        ATIPutExtReg(0xAD, restore->ad);
                        ATIPutExtReg(0xAE, restore->ae);
                }
        }
        ATIPutExtReg(0xB0, restore->b0);
        ATIPutExtReg(0xB1, restore->b1);
        ATIPutExtReg(0xB3, restore->b3);
        ATIPutExtReg(0xB4, restore->b4);
        ATIPutExtReg(0xB5, restore->b5);
        ATIPutExtReg(0xB6, restore->b6);
        ATIPutExtReg(0xB9, restore->b9);
        ATIPutExtReg(0xBA, restore->ba);
        ATIPutExtReg(0xBD, restore->bd);
        ATIPutExtReg(0xB8, restore->b8);        /* Must be last */
 
        /*
         * Restore the generic VGA registers.
         */
        vgaHWRestore((vgaHWPtr)restore);
 
        outb(0x3C2, restore->std.MiscOutReg);
        outw(0x3C4, 0x0300);    /* End synchronous reset */
}
 
/*
 * ATISave --
 *
 * This function saves the video state.  It reads all of the SVGA registers
 * into the vgaATIRec data structure.  There is in general no need to
 * mask out bits here - just read the registers.
 */
static void *
ATISave(save)
vgaATIPtr save;
{
        unsigned char b2, b8;   /* The oddballs */
 
        /*
         * Unlock ATI extended registers.
         */
        b8 = ATIGetExtReg(0xB8);
        ATIPutExtReg(0xB8, b8 & 0xC0);
 
        /*
         * Get back to bank zero.
         */
        b2 = ATIGetExtReg(0xB2);
        if (ATIBoard == ATI_BOARD_V3)
                ATIPutExtReg(0xB2, b2 & 0xE1);
        else
                ATIPutExtReg(0xB2, 0);
 
        /*
         * This function will handle creating the data structure and filling
         * in the generic VGA portion.
         */
        save = (vgaATIPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaATIRec));
 
        /*
         * Save ATI-specific registers.
         */
        save->b0 = ATIGetExtReg(0xB0);
        save->b1 = ATIGetExtReg(0xB1);
        save->b2 = b2;
        save->b3 = ATIGetExtReg(0xB3);
        save->b4 = ATIGetExtReg(0xB4);
        save->b5 = ATIGetExtReg(0xB5);
        save->b6 = ATIGetExtReg(0xB6);
        save->b8 = b8;
        save->b9 = ATIGetExtReg(0xB9);
        save->ba = ATIGetExtReg(0xBA);
        save->bd = ATIGetExtReg(0xBD);
        if (ATIBoard != ATI_BOARD_V3)
        {
                save->be = ATIGetExtReg(0xBE);
                if (ATIBoard > ATI_BOARD_V5)
                {
                        save->a3 = ATIGetExtReg(0xA3);
                        save->a6 = ATIGetExtReg(0xA6);
                        save->a7 = ATIGetExtReg(0xA7);
                        save->ac = ATIGetExtReg(0xAC);
                        save->ad = ATIGetExtReg(0xAD);
                        save->ae = ATIGetExtReg(0xAE);
                }
        }
        return ((void *) save);
}
 
/*
 * ATIInit --
 *
 * This is the most important function (after the Probe) function.  This
 * function fills in the vgaATIRec with all of the register values needed
 * to enable a video mode.
 */
static Bool
ATIInit(mode)
DisplayModePtr mode;
{
        /*
         * The VGA Wonder boards have a bit that multiplies all vertical
         * timing values by 2.  This feature is only used if it's actually
         * needed (i.e. when VTotal > 1024).  If the feature is needed, fake
         * out an interlaced mode and let vgaHWInit divide things by two.
         * Note that this prevents the (incorrect) use of this feature with
         * interlaced modes.
         */
        int saved_mode_flags = mode->Flags;
        if (mode->VTotal > 1024)
                mode->Flags |= V_INTERLACE;
 
        /*
         * This will allocate the data structure and initialize all of the
         * generic VGA registers.
         */
        if (!vgaHWInit(mode,sizeof(vgaATIRec)))
        {
                mode->Flags = saved_mode_flags;
                return (FALSE);
        }
 
        /*
         * Override a few things.
         */
#if !defined(MONOVGA) && !defined(XF86VGA16)
        new->std.Sequencer[4] = 0x0A;
        new->std.Graphics[5] = 0x00;
        new->std.Attribute[16] = 0x01;
        if (ATIBoard == ATI_BOARD_V3)
              new->std.CRTC[19] = vga256InfoRec.displayWidth >> 3;
        new->std.CRTC[23] = 0xE3;
#endif
        if (saved_mode_flags != mode->Flags)
        {
		/* Use "double vertical timings" bit */
                new->std.CRTC[23] |= 0x04;
                mode->Flags = saved_mode_flags;
        }
 
        /*
         * Set up ATI registers.
         */
#if defined(MONOVGA) || defined(XF86VGA16)
	if (ATIBoard < ATI_BOARD_PLUS)
		new->b0 = 0x00;
	else
		new->b0 = (ATIGetExtReg(0xB0) & 0x98) | 0x01;
#else
	if (ATIBoard < ATI_BOARD_PLUS)
		new->b0 = 0x26;
	else
		new->b0 = (ATIGetExtReg(0xB0) & 0x98) | 0x21;
#endif
        new->b1 = (ATIGetExtReg(0xB1) & 0x84)       ;
        new->b3 = (ATIGetExtReg(0xB3) & 0x2F)       ;
        new->b4 = 0;
        new->b5 = (ATIGetExtReg(0xB5) & 0x3F)       ;
#if defined(MONOVGA) || defined(XF86VGA16)
        new->b6 = 0x41;
#else
        new->b6 = 0x45;
#endif
        new->b8 = (ATIGetExtReg(0xB8) & 0xC0)       ;
        new->b9 = (ATIGetExtReg(0xB9) & 0x3F)       ;
        new->ba = (ATIGetExtReg(0xBA) & 0xCF)       ;
        new->bd = (ATIGetExtReg(0xBD) & 0x0B)       ;
        if (ATIBoard == ATI_BOARD_V3)
                new->b2 = (ATIGetExtReg(0xB2) & 0xC0)       ;
        else
        {
                new->b2 = 0;
                new->be = (ATIGetExtReg(0xBE) & 0x34) | 0x09;
                if (ATIBoard > ATI_BOARD_V5)
                {
                        new->a3 = (ATIGetExtReg(0xA3) & 0xE7)       ;
                        new->a6 = (ATIGetExtReg(0xA6)       ) | 0x01;
                        new->a7 = (ATIGetExtReg(0xA7) & 0xFE)       ;
                        new->ac = (ATIGetExtReg(0xAC) & 0xBE)       ;
                        new->ad = (ATIGetExtReg(0xAD) & 0xF0)       ;
                        new->ae = (ATIGetExtReg(0xAE) & 0xF0)       ;
                }
        }
        if (mode->Flags & V_INTERLACE)  /* Enable interlacing */
        {
                if (ATIBoard == ATI_BOARD_V3)
                        new->b2 |= 0x01;
                else
                        new->be |= 0x02;
        }
 
        /*
         * Set clock select bits.
         */
        if (new->std.NoClock >= 0)
        {
                int Clock = mode->Clock;
 
                /*
                 * Set generic clock select bits just in case.
                 */
                new->std.MiscOutReg = (new->std.MiscOutReg & 0xF3) |
                        ((Clock << 2) & 0x0C);
 
                /*
                 * Set ATI clock select bits.
                 */
                if (ATIBoard == ATI_BOARD_V3)
                        new->b2 = (new->b2 & 0xBF) | ((Clock << 4) & 0x40);
                else
                {
                        new->be = (new->be & 0xEF) | ((Clock << 2) & 0x10);
                        if ((ATIBoard != ATI_BOARD_V4) ||
                            (OFLG_ISSET(OPTION_UNDOC_CLKS,
                                &vga256InfoRec.options)))
                        {
                                new->b9 = (new->b9 & 0xFD) |
                                        ((Clock >> 2) & 0x02);
                                Clock >>= 1;
                        }
                }
                if (OFLG_ISSET(OPTION_UNDOC_CLKS, &vga256InfoRec.options))
                {
                        new->b9 = (new->b9 & 0xFE) | ((Clock >> 3) & 0x01);
                        new->b9 ^= saved_b9_bits_0_and_1;
                        Clock >>= 1;
                }
 
                /*
                 * Set clock divider bits.
                 */
                new->b8 = (new->b8 & 0x3F) | ((Clock << 3) & 0xC0);
        }
        return (TRUE);
}
 
/*
 * ATIAdjust --
 *
 * This function is used to initialize the SVGA Start Address - the first
 * displayed location in the video memory.  This is used to implement the
 * virtual window.
 */
static void
ATIAdjust(x, y)
int x, y;
{
        int Base = (y * vga256InfoRec.displayWidth + x) >> 3;
	unsigned char tmp;
 
        outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
        outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);
 
        if (ATIBoard < ATI_BOARD_PLUS)
	{
		tmp = ATIGetExtReg(0xB0);
                ATIPutExtReg(0xB0, (tmp & 0x3F) | ((Base & 0x030000) >> 10));
	}
        else
        {
		tmp = ATIGetExtReg(0xB0);
                ATIPutExtReg(0xB0, (tmp & 0xBF) | ((Base & 0x010000) >> 10));
		tmp = ATIGetExtReg(0xA3);
                ATIPutExtReg(0xA3, (tmp & 0xEF) | ((Base & 0x020000) >> 13));
	}
}


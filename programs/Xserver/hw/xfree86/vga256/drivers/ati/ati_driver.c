/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/ati/ati_driver.c,v 3.4 1994/08/01 12:15:05 dawes Exp $ */
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
 * William Shubert, wms@ssd.intel.com, for work in supporting Mach64 boards.
 * ATI, Robert Wolff, David Dawes and Mark Weaver, for a Mach32 memory probe.
 *
 * This rewrite exists to fix interlacing, virtual console switching, virtual
 * window scrolling and clock selection, implement clock probing, allow for an
 * external clock selection program, enhance support for V3, Mach8, Mach32 and
 * Mach64 boards, add support for up to 4M of video memory and for the
 * monochrome and 16-colour servers and lots of other nitpicky reasons I don't
 * recall right now.
 *
 * The driver is intended to support ATI VGA Wonder V3, V4, V5, PLUS, XL and
 * XL24 boards.  It will also work with Mach8, Mach32 and Mach64 boards but
 * will not use their accelerated features.
 *
 * The ATI x8800 chips use special registers for their extended features.
 * These registers are accessible through an index I/O port and a data I/O
 * port.  On most boards, BIOS initialization stores the index port number in
 * the Graphics register bank (0x03CE), indices 0x50 and 0x51.  In a few of
 * these cases, this value has been lost by the time the operating system kicks
 * in, presumably due to a bug in the BIOS's mode select code.  On many boards
 * (Mach64 boards being notable exceptions), the index port number can also be
 * found in the short integer at offset 0x10 in the BIOS.  If both assumptions
 * fail, this driver will use 0x01CE as the index port number, which happens to
 * be the correct port on all known ATI video adapters.  The data port number
 * is one more than the index port number (i.e. 0x01CF).  These ports differ in
 * their I/O behaviour from the normal VGA ones:
 *
 *    write:  outw(0x01CE, (data << 8) | index);
 *    read:   outb(0x01CE, index);  data = inb(0x01CF);
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
 * V3/V4 Board Clock Frequencies
 * R E G I S T E R S
 * 1CE(*)    3C2     3C2    Frequency
 * B2h/BEh
 * Bit 6/4  Bit 3   Bit 2   (MHz)
 * ------- ------- -------  -------
 *    0       0       0     50.175
 *    0       0       1     56.644
 *    0       1       0     Spare 1
 *    0       1       1     44.900
 *    1       0       0     44.900
 *    1       0       1     50.175
 *    1       1       0     Spare 2
 *    1       1       1     36.000
 *
 * (*):  V3 uses index B2h, bit 6;  V4 uses index BEh, bit 4
 *
 * V5, PLUS, XL and XL24 usually have an ATI 18810 clock generator chip, but
 * some have an ATI 18811-0, and it's quite conceivable that some exist with
 * ATI 18811-1's or ATI 18811-2's.  Mach8 and Mach32 boards are known to use
 * any one of these clock generators.  The possibilities for Mach64 boards also
 * include two different flavours of the newer 18818 chips.  ATI says there is
 * no reliable way for the driver to determine which clock generator is on the
 * board (their BIOS's are tailored to the board).
 *
 * V5/PLUS/XL/XL24 Board Clock Frequencies
 * R E G I S T E R S
 *   1CE     1CE     3C2     3C2    Frequency
 *   B9h     BEh                    (MHz)             18811-1
 *  Bit 1   Bit 4   Bit 3   Bit 2    18810   18811-0  18811-2  18818-?  18818-?
 * ------- ------- ------- -------  -------  -------  -------  -------  -------
 *    0       0       0       0      30.240   30.240  135.000     (*3)     (*3)
 *    0       0       0       1      32.000   32.000   32.000  110.000  110.000
 *    0       0       1       0      37.500  110.000  110.000  126.000  126.000
 *    0       0       1       1      39.000   80.000   80.000  135.000  135.000
 *    0       1       0       0      42.954   42.954  100.000   50.350   25.175
 *    0       1       0       1      48.771   48.771  126.000   56.644   28.322
 *    0       1       1       0        (*1)   92.400   92.400   63.000   31.500
 *    0       1       1       1      36.000   36.000   36.000   72.000   36.000
 *    1       0       0       0      40.000   39.910   39.910     (*3)     (*3)
 *    1       0       0       1      56.644   44.900   44.900   80.000   80.000
 *    1       0       1       0      75.000   75.000   75.000   75.000   75.000
 *    1       0       1       1      65.000   65.000   65.000   65.000   65.000
 *    1       1       0       0      50.350   50.350   50.350   40.000   40.000
 *    1       1       0       1      56.640   56.640   56.640   44.900   44.900
 *    1       1       1       0        (*2)     (*3)     (*3)   49.500   49.500
 *    1       1       1       1      44.900   44.900   44.900   50.000   50.000
 *
 * (*1) External 0 (supposedly 16.657 Mhz)
 * (*2) External 1 (supposedly 28.322 MHz)
 * (*3) This setting doesn't seem to generate anything
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
#include "regati.h"

/*
 * Driver data structures.
 */
typedef struct
{
        vgaHWRec std;               /* good old IBM VGA */

        unsigned char             a3,         a6, a7,
                                  ab, ac, ad, ae,
                      b0, b1, b2, b3, b4, b5, b6,
                      b8, b9, ba,         bd, be, bf;
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
 * Bank selection functions for V4 and V5 boards.  Defined in bank.s.
 */
extern void     ATIV4V5SetRead();
extern void     ATIV4V5SetWrite();
extern void     ATIV4V5SetReadWrite();
/*
 * This data structure defines the driver itself.  The data structure is
 * initialized with the functions that make up the driver and some data
 * that defines how the driver operates.
 */
vgaVideoChipRec ATI =
{
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
	FALSE,			/* No linear fb */
	0,			/* Linear fb base address */
	0,			/* Linear fb size */
};

/*
 * This is a convenience macro, so that entries in the driver structure
 * can simply be dereferenced with 'new->xxx'.
 */
#define new ((vgaATIPtr)vgaNewVideoState)

#define DAC_Mask        0x03C6
#define DAC_Read_Index  0x03C7
#define DAC_Write_Index 0x03C8
#define DAC_Data        0x03C9

/*
 * This driver needs non-standard I/O ports.  The first (or only) two are
 * determined by ATIProbe and are initialized to their most probable values
 * here.
 */
static unsigned ATI_IOPorts[] =
{
        0x01CE, 0x01CF
};
static int Num_ATI_IOPorts =
        (sizeof(ATI_IOPorts) / sizeof(ATI_IOPorts[0]));
short ATIExtReg = 0x01CE;       /* Used by bank.s;  Must be short */
extern unsigned char ATIB2Reg;  /* The B2 mirror in bank.s */

/*
 * I/O port list needed by ATIProbe.
 */
static unsigned Probe_IOPorts[] =
{
        /* VGA Graphics registers */
        0x03CE, 0x03CF,

        /* 8514/A registers */
        ERR_TERM, GP_STAT,
        RD_MASK, CUR_X, CUR_Y, PIX_TRANS, FRGD_COLOR,

        /* Mach8 registers */
        ROM_ADDR_1, DESTX_DIASTP, CONFIG_STATUS_1, EXT_FIFO_STATUS,
        CLOCK_SEL, GE_OFFSET_LO, GE_OFFSET_HI, GE_PITCH, DP_CONFIG,

        /* Mach32 registers */
        READ_SRC_X, CHIP_ID, MISC_OPTIONS,
        MEM_BNDRY, R_EXT_GE_CONFIG, EXT_GE_CONFIG,
        DEST_X_START, DEST_X_END, DEST_Y_END, ALU_FG_FN,

        /* Mach64 registers */
        SCRATCH_REG0, MEM_INFO, CONFIG_STATUS_0
};
static int Num_Probe_IOPorts =
        (sizeof(Probe_IOPorts) / sizeof(Probe_IOPorts[0]));

/*
 * Handy macros to read and write registers.
 */
#define GetReg(Port, Index)        \
        (                          \
                outb(Port, Index), \
                inb(Port + 1)      \
        )
#define ATIGetExtReg(Index)        GetReg(ATIExtReg, Index)
#define ATIPutExtReg(Index, Register_Value)     \
        outw(ATIExtReg, ((Register_Value) << 8) | Index)

#define ATI_CHIP_NONE      0
#define ATI_CHIP_18800     1
#define ATI_CHIP_18800_1   2
#define ATI_CHIP_28800_2   3
#define ATI_CHIP_28800_4   4
#define ATI_CHIP_28800_5   5
#define ATI_CHIP_28800_6   6
#define ATI_CHIP_68800     7    /* Mach32 */
#define ATI_CHIP_68800_3   8    /* Mach32 */
#define ATI_CHIP_68800_6   9    /* Mach32 */
#define ATI_CHIP_68800LX  10    /* Mach32 */
#define ATI_CHIP_68800AX  11    /* Mach32 */
#define ATI_CHIP_88800    12    /* Mach64 */
static unsigned char ATIChip = ATI_CHIP_NONE;
static const char *ChipNames[] =
{
        "Unknown!",
        "ATI 18800",
        "ATI 18800-1",
        "ATI 28800-2",
        "ATI 28800-4",
        "ATI 28800-5",
        "ATI 28800-6",
        "ATI 68800",
        "ATI 68800-3",
        "ATI 68800-6",
        "ATI 68800LX",
        "ATI 68800AX",
        "ATI 88800"
};

#define ATI_BOARD_NONE    0
#define ATI_BOARD_V3      1
#define ATI_BOARD_V4      2
#define ATI_BOARD_V5      3
#define ATI_BOARD_PLUS    4
#define ATI_BOARD_XL      5
#define ATI_BOARD_MACH8   6
#define ATI_BOARD_MACH32  7
#define ATI_BOARD_MACH64  8
static unsigned char ATIBoard = ATI_BOARD_NONE;
static const char *BoardNames[] =
{
        "Unknown!",
        "VGA Wonder V3",
        "VGA Wonder V4",
        "VGA Wonder V5",
        "VGA Wonder+",
        "VGA Wonder XL or XL24",
        "Mach8",
        "Mach32",
        "Mach64"
};

#define ATI_DAC_ATI68830 0
#define ATI_DAC_SC11483  1
#define ATI_DAC_ATI68875 2
#define ATI_DAC_GENERIC  3
#define ATI_DAC_BT481    4
#define ATI_DAC_ATI68860 5
#define ATI_DAC_STG1700  6
#define ATI_DAC_SC15021  7
static unsigned char ATIDac = ATI_DAC_GENERIC;
static const char *DACNames[] =
{
        "ATI 68830",
        "Sierra 11483",
        "ATI 68875",
        "Brooktree 476",
        "Brooktree 481",
        "ATI 68860",
        "STG 1700",
        "Sierra 15021"
};

static unsigned char saved_b9_bits_0_and_1 = 0;

/*
 * Because the only practical standard C library is an inadequate lowest
 * common denominator...
 */
static void *
findsubstring(needle, needle_length, haystack, haystack_length)
const void * needle;
int needle_length;
const void * haystack;
int haystack_length;
{
        const unsigned char * haystack_pointer;
        const unsigned char * needle_pointer = needle;
        int compare_length;

        haystack_length -= needle_length;
        for (haystack_pointer = haystack;
             haystack_length >= 0;
             haystack_pointer++, haystack_length--)
                for (compare_length = 0; ; compare_length++)
                {
                        if (compare_length >= needle_length)
                                return (void *) haystack_pointer;
                        if (haystack_pointer[compare_length] !=
                            needle_pointer[compare_length])
                                break;
                }

        return (void *) 0;
}

/*
 * ATIIdent --
 *
 * Returns a string name for this driver or NULL.
 */
static char *
ATIIdent(n)
int n;
{
        static char *chipsets[] = {"vgawonder"};

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
                unsigned char misc = inb(0x03CC);
                unsigned char bb = ATIGetExtReg(0xBB);
                outb(0x03C2, (misc & 0xF3) | 0x04 | ((bb & 0x10) >> 1));
                ATIPutExtReg(0xB2, old_b2 & 0xBF);
                ATIV3Delay();
                outb(0x03C2, misc);
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
                        saved_misc = inb(0x03CC);
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
                        outw(0x03C4, 0x0100);   /* Start synchronous reset */

                        if (ATIBoard == ATI_BOARD_V3)
                                ATIV3SetB2(previous_b2, saved_b2);
                        else
                                ATIPutExtReg(0xBE, saved_be);
                        ATIPutExtReg(0xB5, saved_b5);
                        ATIPutExtReg(0xB9, saved_b9);
                        ATIPutExtReg(0xB8, saved_b8);

                        outb(0x03C2, saved_misc);
                        outw(0x03C4, 0x0300);   /* End synchronous reset */

                        break;
                default:
                        /*
                         * Set the generic two low-order bits of the clock
                         * select.
                         */
                        misc = (inb(0x03CC) & 0xF3) | ((no << 2) & 0x0C);

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
                                unsigned char be = ATIGetExtReg(0xBE);
                                ATIPutExtReg(0xBE,
                                        (be & 0xEF) | ((no << 2) & 0x10));
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
                        outb(0x03C2, misc);

                        break;
        }
        return (TRUE);
}

typedef unsigned short Colour;  /* The correct spelling should be OK :-) */

/*
 * Bit patterns which are extremely unlikely to show up when reading from
 * nonexistant memory (which normally shows up as either all bits set or all
 * bits clear).
 */
static const Colour Test_Pixel[] = {0x5aa5, 0x55aa, 0xa55a, 0xca53};

#define NUMBER_OF_TEST_PIXELS (sizeof(Test_Pixel) / sizeof(Test_Pixel[0]))

static const struct
{
        int videoRamSize;
        int Miscellaneous_Options_Setting;
        struct
        {
                short x, y;
        }
        Coordinates[NUMBER_OF_TEST_PIXELS + 1];
}
Test_Case[] =
{
        /*
         * Given the engine settings used, only a 4M card will have enough
         * memory to back up the 1025th line of the display.  Since the pixel
         * coordinates are zero-based, line 1024 will be the first one which
         * is only backed on 4M cards.
         *
         * <Mark_Weaver@brown.edu>:
         * In case memory is being wrapped, (0,0) and (0,1024) to make sure
         * they can each hold a unique value.
         */
        {4096, MEM_SIZE_4M, {{0,0}, {0,1024}, {-1,-1}}},

        /*
         * We know this card has 2M or less. On a 1M card, the first 2M of the
         * card's memory will have even doublewords backed by physical memory
         * and odd doublewords unbacked.
         *
         * Pixels 0 and 1 of a row will be in the zeroth doubleword, while
         * pixels 2 and 3 will be in the first.  Check both pixels 2 and 3 in
         * case this is a pseudo-1M card (one chip pulled to turn a 2M card
         * into a 1M card).
         *
         * <Mark_Weaver@brown.edu>:
         * I don't have a 1M card, so I'm taking a stab in the dark.  Maybe
         * memory wraps every 512 lines, or maybe odd doublewords are aliases
         * of their even doubleword counterparts.  I try everything here.
         */
        {2048, MEM_SIZE_2M, {{0,0}, {0,512}, {2,0}, {3,0}, {-1,-1}}},

        /*
         * This is a either a 1M card or a 512k card.  Test pixel 1, since it
         * is an odd word in an even doubleword.
         *
         * <Mark_Weaver@brown.edu>:
         * This is the same idea as the test above.
         */
        {1024, MEM_SIZE_1M, {{0,0}, {0,256}, {1,0}, {-1,-1}}},

        /*
         * We assume it is a 512k card by default, since that is the minimum
         * configuration.
         */
        {512, MEM_SIZE_512K, {{-1,-1}}}
};

#define NUMBER_OF_TEST_CASES (sizeof(Test_Case) / sizeof(Test_Case[0]))

/*
 * ATIMach32ReadPixel --
 *
 * Return the colour of the specified screen location.  Called from
 * ATIMach32videoRam routine below.
 */
static Colour
ATIMach32ReadPixel(X, Y)
short X, Y;
{
        Colour Pixel_Colour;

        /* Wait for idle engine */
        WaitIdleEmpty();

        /* Set up engine for pixel read */
        ATIWaitQueue(7);
        outw(RD_MASK, (unsigned short)(~0));
        outw(DP_CONFIG, FG_COLOR_SRC_BLIT | DATA_WIDTH | DRAW | DATA_ORDER);
        outw(CUR_X, X);
        outw(CUR_Y, Y);
        outw(DEST_X_START, X);
        outw(DEST_X_END, X + 1);
        outw(DEST_Y_END, Y + 1);

        /* Wait for data to become ready */
        ATIWaitQueue(16);
        WaitDataReady();

        /* Read pixel colour */
        Pixel_Colour = inw(PIX_TRANS);
        WaitIdleEmpty();
        return Pixel_Colour;
}

/*
 * ATIMach32WritePixel --
 *
 * Set the colour of the specified screen location.  Called from
 * ATIMach32videoRam routine below.
 */
static void
ATIMach32WritePixel(X, Y, Pixel_Colour)
short X, Y;
Colour Pixel_Colour;
{
        /* Set up engine for pixel write */
        ATIWaitQueue(8);
        outw(DP_CONFIG, FG_COLOR_SRC_FG | DRAW | READ_WRITE);
        outw(ALU_FG_FN, MIX_FN_PAINT);
        outw(FRGD_COLOR, Pixel_Colour);
        outw(CUR_X, X);
        outw(CUR_Y, Y);
        outw(DEST_X_START, X);
        outw(DEST_X_END, X + 1);
        outw(DEST_Y_END, Y + 1);
}

/*
 * ATIMach32videoRam --
 *
 * Determine the amount of video memory installed on an 68800-6 based adapter.
 * This is done because these chips exhibit a bug that causes their
 * MISC_OPTIONS register to report 1M rather than the true amount of memory.
 *
 * This routine is adapted from a similar routine in mach32mem.c written by
 * Robert Wolff, David Dawes and Mark Weaver.
 */
static int
ATIMach32videoRam(void)
{
        unsigned short saved_CLOCK_SEL, saved_MEM_BNDRY,
                saved_MISC_OPTIONS, saved_EXT_GE_CONFIG;
        Colour saved_Pixel[NUMBER_OF_TEST_PIXELS];
        int Case_Number, Pixel_Number;
        Bool AllPixelsOK;

        /* Enable accelerator */
        saved_CLOCK_SEL = inw(CLOCK_SEL);
        outw(CLOCK_SEL, saved_CLOCK_SEL | DISABPASSTHRU);

        /*
         * Set up a 512k VGA boundary so "blue screen" writes that happen when
         * in accelerator mode won't show up in the wrong place.
         */
        saved_MEM_BNDRY = inw(MEM_BNDRY);
        outw(MEM_BNDRY, 0);

        /* Prevent video memory wrap */
        saved_MISC_OPTIONS = inw(MISC_OPTIONS) & ~MEM_SIZE_ALIAS;
        outw(MISC_OPTIONS, saved_MISC_OPTIONS | MEM_SIZE_4M);

        /*
         * Set up the drawing engine for a pitch of 1024 at 16 bits per pixel.
         * No need to mess with the CRT because the results of this test are
         * not intended to be seen.
         */
        saved_EXT_GE_CONFIG = inw(R_EXT_GE_CONFIG);
        outw(EXT_GE_CONFIG, PIX_WIDTH_16BPP | ORDER_16BPP_565 | 0x000A);
        outw(GE_PITCH, 1024 >> 3);
        outw(GE_OFFSET_HI, 0);
        outw(GE_OFFSET_LO, 0);

        for (Case_Number = 0;
             Case_Number < (NUMBER_OF_TEST_CASES - 1);
             Case_Number++)
        {
                /* Reduce redundancy as per Mark_Weaver@brown.edu */
#               define TestPixel               \
                        Test_Case[Case_Number].Coordinates[Pixel_Number]
#               define ForEachTestPixel        \
                        for (Pixel_Number = 0; \
                             TestPixel.x >= 0; \
                             Pixel_Number++)

                /* Save pixel colours that will be clobbered */
                ForEachTestPixel
                        saved_Pixel[Pixel_Number] =
                                ATIMach32ReadPixel(TestPixel.x, TestPixel.y);

                /* Write test patterns */
                ForEachTestPixel
                        ATIMach32WritePixel(TestPixel.x, TestPixel.y,
                                Test_Pixel[Pixel_Number]);

                /* Test for lost pixels */
                AllPixelsOK = TRUE;
                ForEachTestPixel
                        if (ATIMach32ReadPixel(TestPixel.x, TestPixel.y) !=
                            saved_Pixel[Pixel_Number])
                        {
                                AllPixelsOK = FALSE;
                                break;
                        }

                /* Restore clobbered pixels */
                ForEachTestPixel
                        ATIMach32WritePixel(TestPixel.x, TestPixel.y,
                                saved_Pixel[Pixel_Number]);

                /* End test on success */
                if (AllPixelsOK)
                        break;

                /* Completeness */
#               undef ForEachTestPixel
#               undef TestPixel
        }

        /* Restore what was changed and correct MISC_OPTIONS register */
        outw(EXT_GE_CONFIG, saved_EXT_GE_CONFIG);
        outw(MISC_OPTIONS, saved_MISC_OPTIONS |
                Test_Case[Case_Number].Miscellaneous_Options_Setting);
        outw(MEM_BNDRY, saved_MEM_BNDRY);

        /* Re-enable VGA passthrough */
        outw(CLOCK_SEL, saved_CLOCK_SEL & ~DISABPASSTHRU);

        /* Tell ATIProbe the REAL story */
        return Test_Case[Case_Number].videoRamSize;
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
#       define Signature        " 761295520"
#       define Signature_Size   10
#       define BIOS_DATA_SIZE   (0x80 + Signature_Size)
        unsigned char BIOS_Data[BIOS_DATA_SIZE];
#       define Signature_Offset 0x30
#       define BIOS_Signature   (BIOS_Data + Signature_Offset)
        unsigned char *Signature_Found;
        unsigned int IO_Value, Index;
        int MachvideoRam = 0;
        int VGAWondervideoRam = 0;
        const int videoRamSizes[] =
                {0, 256, 512, 1024, 2*1024, 4*1024, 6*1024, 8*1024, 0, 0};

        /*
         * Get out if this isn't the driver the user wants.
         */
        if (vga256InfoRec.chipset &&
            StrCaseCmp(vga256InfoRec.chipset, ATIIdent(0)))
        {
                static char *chipsets[] = {"ati", "mach8", "mach32", "mach64"};
                int i;

                /* Check for some other chipset names that need changing */
                for (i = 0;  StrCaseCmp(vga256InfoRec.chipset, chipsets[i]);  )
                        if (++i >= (sizeof(chipsets) / sizeof(chipsets[0])))
                                return (FALSE);
                ErrorF("ChipSet specification changed from \"%s%s",
                       chipsets[i], "\" to \"vgawonder\"\n");
                ErrorF("See README.ati for more information");
                OFLG_CLR(XCONFIG_CHIPSET, &vga256InfoRec.xconfigFlag);
                if (vga256InfoRec.clocks)
                {
                        vga256InfoRec.clocks = 0;
                        OFLG_CLR(XCONFIG_CLOCKS, &vga256InfoRec.xconfigFlag);
                        if (!OFLG_ISSET(OPTION_PROBE_CLKS,
                                        &vga256InfoRec.options))
                                ErrorF("Clocks will be probed\n");
                }
        }

        /*
         * Get BIOS data this driver will use.
         */
        if (xf86ReadBIOS(vga256InfoRec.BIOSbase, 0, BIOS_Data,
                         sizeof(BIOS_Data)) != sizeof(BIOS_Data))
                return (FALSE);

        /*
         * Get out if this is the wrong driver for installed chipset.
         */
        Signature_Found =
                findsubstring(Signature, Signature_Size,
                        BIOS_Data, sizeof(BIOS_Data));
        if (!Signature_Found)
                return (FALSE);

        /*
         * Enable the I/O ports needed for probing.
         */
        xf86ClearIOPortList(vga256InfoRec.scrnIndex);
        xf86AddIOPorts(vga256InfoRec.scrnIndex,
                Num_Probe_IOPorts, Probe_IOPorts);
        xf86EnableIOPorts(vga256InfoRec.scrnIndex);

        /*
         * First determine if a Mach64 is present.
         */
        IO_Value = inl(SCRATCH_REG0);
        outl(SCRATCH_REG0, 0x55555555);          /* Test odd bits */
        if (inl(SCRATCH_REG0) == 0x55555555)
        {
                outl(SCRATCH_REG0, 0xAAAAAAAA);  /* Test even bits */
                if (inl(SCRATCH_REG0) == 0xAAAAAAAA)
                {
                        /* Mach64 detected */
                        ATIChip = ATI_CHIP_88800;
                        ATIBoard = ATI_BOARD_MACH64;
                        ATIDac = (inl(CONFIG_STATUS_0) >> 9) & 0x0007;
                        MachvideoRam =
                                videoRamSizes[(inl(MEM_INFO) & 0x0007) + 2];
                }
        }
        outl(SCRATCH_REG0, IO_Value);

        if (ATIBoard == ATI_BOARD_NONE)
        {
                /*
                 * Get out if signature is not in the right spot.
                 */
                if (Signature_Found != BIOS_Signature)
                {
                        xf86DisableIOPorts(vga256InfoRec.scrnIndex);
                        return (FALSE);
                }

                if (!(BIOS_Data[0x44] & 0x40))
                {
                        /* An accelerator is present */
                        IO_Value = inw(ERR_TERM);
                        outw(ERR_TERM, 0x5A5A);
                        ProbeWaitIdleEmpty();
                        if (inw(ERR_TERM) == 0x5A5A)
                        {
                                outw(ERR_TERM, 0x2525);
                                ProbeWaitIdleEmpty();
                                if (inw(ERR_TERM) == 0x2525)
                                        ATIBoard = ATI_BOARD_MACH8;
                        }
                        outw(ERR_TERM, IO_Value);

                        if (ATIBoard != ATI_BOARD_NONE)
                        {
                                /* Some kind of 8514/A detected */
                                ATIBoard = ATI_BOARD_NONE;

                                IO_Value = inw(ROM_ADDR_1);
                                outw(ROM_ADDR_1, 0x5555);
                                ProbeWaitIdleEmpty();
                                if (inw(ROM_ADDR_1) == 0x5555)
                                {
                                        outw(ROM_ADDR_1, 0x2A2A);
                                        ProbeWaitIdleEmpty();
                                        if (inw(ROM_ADDR_1) == 0x2A2A)
                                                ATIBoard = ATI_BOARD_MACH8;
                                }
                                outw(ROM_ADDR_1, IO_Value);
                        }

                        if (ATIBoard != ATI_BOARD_NONE)
                        {
                                /* ATI accelerator detected */
                                outw(DESTX_DIASTP, 0xAAAA);
                                WaitIdleEmpty();
                                if (inw(READ_SRC_X) == 0x02AA)
                                        ATIBoard = ATI_BOARD_MACH32;

                                outw(DESTX_DIASTP, 0x5555);
                                WaitIdleEmpty();
                                if (inw(READ_SRC_X) == 0x0555)
                                {
                                        if (ATIBoard != ATI_BOARD_MACH32)
                                                ATIBoard = ATI_BOARD_NONE;
                                }
                                else
                                {
                                        if (ATIBoard != ATI_BOARD_MACH8)
                                                ATIBoard = ATI_BOARD_NONE;
                                }
                        }

                        if (ATIBoard == ATI_BOARD_NONE)
                        {
                                xf86DisableIOPorts(vga256InfoRec.scrnIndex);
                                return (FALSE);
                        }

                        if (ATIBoard == ATI_BOARD_MACH32)
                        {
                                switch (inw(CHIP_ID) & 0x3FF)
                                {
                                      case 0x0000:
                                              ATIChip = ATI_CHIP_68800_3;
                                              break;

                                      case 0x02F7:
                                              ATIChip = ATI_CHIP_68800_6;
                                              break;

                                      case 0x0177:
                                              ATIChip = ATI_CHIP_68800LX;
                                              break;

                                      case 0x0017:
                                              ATIChip = ATI_CHIP_68800AX;
                                              break;

                                      default:
                                              ATIChip = ATI_CHIP_68800;
                                              break;
                                }

                                ATIDac = (inw(CONFIG_STATUS_1) & DACTYPE) >> 9;
                                MachvideoRam =
                                        videoRamSizes[((inw(MISC_OPTIONS) &
                                                MEM_SIZE_ALIAS) >> 2) + 2];

                                /*
                                 * The 68800-6 doesn't necessarily report the
                                 * correct video memory size.
                                 */
                                if ((ATIChip == ATI_CHIP_68800_6) &&
                                    (MachvideoRam == 1024))
                                        MachvideoRam = ATIMach32videoRam();

                        }
                }

                if (ATIBoard != ATI_BOARD_MACH32)
                {
                        unsigned char ATIVGABoard;

                        /* This is a Mach8 or VGA Wonder board of some kind */
                        if ((BIOS_Data[0x43] >= '1') &&
                                (BIOS_Data[0x43] <= '6'))
                                ATIChip = BIOS_Data[0x43] - '0';

                        switch (ATIChip)
                        {
                                case ATI_CHIP_18800:
                                        ATIVGABoard = ATI_BOARD_V3;
                                        /* Reset a few things for V3 boards */
                                        ATI.ChipSetRead = ATIV3SetRead;
                                        ATI.ChipSetWrite = ATIV3SetWrite;
                                        ATI.ChipSetReadWrite =
                                                ATIV3SetReadWrite;
                                        ATI.ChipUse2Banks = FALSE;
#if !defined(MONOVGA) && !defined(XF86VGA16)
                                        ATI.ChipRounding = 8;
#endif
                                        break;

                                case ATI_CHIP_18800_1:
                                        if (BIOS_Data[0x42] & 0x10)
                                                ATIVGABoard = ATI_BOARD_V5;
                                        else
                                                ATIVGABoard = ATI_BOARD_V4;
                                        /* Reset a few things for V4 and V5
                                           boards */
                                        ATI.ChipSetRead = ATIV4V5SetRead;
                                        ATI.ChipSetWrite = ATIV4V5SetWrite;
                                        ATI.ChipSetReadWrite =
                                                ATIV4V5SetReadWrite;
                                        break;

                                case ATI_CHIP_28800_2:
                                case ATI_CHIP_28800_4:
                                case ATI_CHIP_28800_5:
                                case ATI_CHIP_28800_6:
                                        ATIVGABoard = ATI_BOARD_PLUS;
                                        if (BIOS_Data[0x44] & 0x80)
                                        {
                                                ATIVGABoard = ATI_BOARD_XL;
                                                ATIDac = ATI_DAC_SC11483;
                                        }
                                        break;
                        }

                        if (ATIBoard == ATI_BOARD_NONE)
                                ATIBoard = ATIVGABoard;
                }
        }

        /*
         * Set up extended register addressing.
         */
        ATIExtReg = ((GetReg(0x03CE, 0x51) & 0x03) << 8) |
                GetReg(0x03CE, 0x50);
        if (ATIExtReg == 0)
        {
                ATIExtReg = *((short *)(BIOS_Data + 0x10));
                if (ATIExtReg == 0)
                        ATIExtReg = 0x01CE;
        }
        ATI_IOPorts[0] = ATIExtReg;
        ATI_IOPorts[1] = ATIExtReg + 1;

        /*
         * Probe I/O ports are no longer needed.
         */
        xf86DisableIOPorts(vga256InfoRec.scrnIndex);

        /*
         * Set up I/O ports to be used by this driver.
         */
        xf86ClearIOPortList(vga256InfoRec.scrnIndex);
        xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
        xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_ATI_IOPorts, ATI_IOPorts);

        ATIEnterLeave(ENTER);           /* Unlock registers */

        /*
         * Sometimes, the BIOS lies about the chip.
         */
        if ((ATIChip >= ATI_CHIP_28800_4) &&
                (ATIChip <= ATI_CHIP_28800_6))
        {
                IO_Value = ATIGetExtReg(0xAA) & 0x0F;
                if ((IO_Value < 7) && (IO_Value > ATIChip))
                        ATIChip = IO_Value;
        }

        ErrorF("%s graphics controller and %s or compatible RAMDAC detected\n",
                ChipNames[ATIChip], DACNames[ATIDac]);
        ErrorF("This is a %s video adapter\n", BoardNames[ATIBoard]);

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
         * Normalize any Xconfig videoRam value.
         */
        for (Index = 0; videoRamSizes[++Index]; )
                if (vga256InfoRec.videoRam < videoRamSizes[Index])
                        break;
        vga256InfoRec.videoRam = videoRamSizes[Index - 1];

        /*
         * The default videoRam value is what the accelerator (if any) thinks
         * it has.  Also, allow the user to override the accelerator's value.
         */
        if (vga256InfoRec.videoRam == 0)
        {
                /* Normalization might have zeroed Xconfig videoRam value */
                OFLG_CLR(XCONFIG_VIDEORAM, &vga256InfoRec.xconfigFlag);
                vga256InfoRec.videoRam = MachvideoRam;
        }
        else
                MachvideoRam = vga256InfoRec.videoRam;

        /*
         * Find out how much video memory the VGA Wonder side thinks it has.
         */
        if (ATIBoard < ATI_BOARD_PLUS)
        {
                IO_Value = ATIGetExtReg(0xBB);
                if (IO_Value & 0x20)
                        VGAWondervideoRam = 512;
                else
                        VGAWondervideoRam = 256;
                if (MachvideoRam > 512)
                        MachvideoRam = 512;
        }
        else
        {
                IO_Value = ATIGetExtReg(0xB0);
                if (IO_Value & 0x08)
                        VGAWondervideoRam = 1024;
                else if (IO_Value & 0x10)
                        VGAWondervideoRam = 512;
                else
                        VGAWondervideoRam = 256;
                if (MachvideoRam > 1024)
                        MachvideoRam = 1024;
        }

        /*
         * If there's no accelerator, default videoRam to what the VGA Wonder
         * side believes.
         */
        if (!vga256InfoRec.videoRam)
                vga256InfoRec.videoRam = VGAWondervideoRam;
        else

        /*
         * Ensure any accelerator and VGA Wonder side agree on video memory
         * size.  In particular, on a Mach64, they won't always agree (a bug in
         * its BIOS, I suppose).
         */
        if (MachvideoRam > VGAWondervideoRam)
                if (ATIBoard < ATI_BOARD_PLUS)
                        ATIPutExtReg(0xBB, IO_Value | 0x20);
                else
                {
                        IO_Value &= 0xE7;
                        if (MachvideoRam >= 1024)
                                IO_Value |= 0x08;
                        else
                                IO_Value |= 0x10;
                        ATIPutExtReg(0xB0, IO_Value);
                }

        /*
         * If the user has specified the clock values in the Xconfig file, we
         * respect those choices.
         */
        if ((!vga256InfoRec.clocks) ||
            (OFLG_ISSET(OPTION_PROBE_CLKS, &vga256InfoRec.options)))
        {
                int Number_Of_Clocks;
                int Calibration_Clock_Number, Calibration_Clock_Value;

                /*
                 * Determine the number of clock values to probe for and the
                 * dot-clock to use for calibration.
                 */
                if (ATIBoard <= ATI_BOARD_V4)
                {
                        Number_Of_Clocks = 8*4;
                        /* Actually, any undivided clock will do */
                        Calibration_Clock_Number = 7;
                        Calibration_Clock_Value = 36000;
                }
                else
                {
                        Number_Of_Clocks = 16*4;
                        Calibration_Clock_Number = 10 /* or 11 */;
                        Calibration_Clock_Value = 75000 /* or 65000 */;
                }
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
                        Calibration_Clock_Number, Calibration_Clock_Value,
                        &vga256InfoRec);
        }

        /*
         * Set the maximum allowable dot-clock frequency (in kHz).
         */
        vga256InfoRec.maxClock = 80000;

        /*
         * Set chipset name.
         */
        vga256InfoRec.chipset = ATIIdent(0);

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
        static unsigned char saved_ab,
                saved_b4, saved_b8, saved_b9, saved_be;
        static Bool entered = LEAVE;
        unsigned char tmp;

        if (enter == entered)
                return;
        entered == enter;

        if (enter == ENTER)
        {
                xf86EnableIOPorts(vga256InfoRec.scrnIndex);

                vgaIOBase = (inb(0x03CC) & 0x01) ? 0x03D0 : 0x03B0;

                /* Clear protection bits in ATI extended registers */
                saved_b4 = ATIGetExtReg(0xB4);
                ATIPutExtReg(0xB4, saved_b4 & 0x03);
                saved_b8 = ATIGetExtReg(0xB8);
                ATIPutExtReg(0xB8, saved_b8 & 0xC0);
                saved_b9 = ATIGetExtReg(0xB9);
                ATIPutExtReg(0xB9, saved_b9 & 0x7F);
                if (ATIBoard != ATI_BOARD_V3)
                {
                        saved_be = ATIGetExtReg(0xBE);
                        ATIPutExtReg(0xBE, saved_be | 0x01);
                        if (ATIBoard > ATI_BOARD_V5)
                        {
                                saved_ab = ATIGetExtReg(0xAB);
                                ATIPutExtReg(0xAB, saved_ab & 0xE7);
                        }
                }

                /* Unprotect CRTC[0-7] */
                outb(vgaIOBase + 4, 0x11);
                tmp = inb(vgaIOBase + 5);
                outb(vgaIOBase + 5, tmp & 0x7F);
        }
        else
        {
                /* Protect CRTC[0-7] */
                outb(vgaIOBase + 4, 0x11);
                tmp = inb(vgaIOBase + 5);
                outb(vgaIOBase + 5, (tmp & 0x7F) | 0x80);

                /* Restore protection bits in ATI extended registers */
                tmp = ATIGetExtReg(0xB4);
                ATIPutExtReg(0xB4, (saved_b4 & 0xFC) | (tmp & 0x03));
                tmp = ATIGetExtReg(0xB8);
                ATIPutExtReg(0xB8, (saved_b8 & 0x3F) | (tmp & 0xC0));
                tmp = ATIGetExtReg(0xB9);
                ATIPutExtReg(0xB9, (saved_b9 & 0x80) | (tmp & 0x7F));
                if (ATIBoard != ATI_BOARD_V3)
                {
                        tmp = ATIGetExtReg(0xBE);
                        ATIPutExtReg(0xBE, (saved_be & 0x01) | (tmp & 0xFE));
                        if (ATIBoard > ATI_BOARD_V5)
                        {
                                tmp = ATIGetExtReg(0xAB);
                                ATIPutExtReg(0xAB,
                                        (saved_ab & 0x18) | (tmp & 0xE7));
                        }
                }

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

        /*
         * Unlock registers.
         */
        ATIEnterLeave(ENTER);

        /*
         * Get (back) to bank 0.
         */
        if (ATIBoard == ATI_BOARD_V3)
        {
                b2 = ATIGetExtReg(0xB2);
                ATIPutExtReg(0xB2, b2 & 0xE1);
        }
        else
        {
                ATIPutExtReg(0xB2, 0);
                ATIB2Reg = 0;
                if (ATIBoard > ATI_BOARD_V5)
                {
                        unsigned char ae = ATIGetExtReg(0xAE);
                        ATIPutExtReg(0xAE, ae & 0xF0);
                }
        }

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
                restore->b8 = (restore->b8 & 0x3F) | ATIGetExtReg(0xB8);
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

        outw(0x03C4, 0x0100);   /* Start synchronous reset */

        if (ATIBoard == ATI_BOARD_V3)
                ATIV3SetB2(b2, restore->b2);
        else
        {
                ATIPutExtReg(0xB2, restore->b2);
                ATIPutExtReg(0xBE, restore->be);
                if (ATIBoard > ATI_BOARD_V5)
                {
                        ATIPutExtReg(0xBF, restore->bf);
                        ATIPutExtReg(0xA3, restore->a3);
                        ATIPutExtReg(0xA6, restore->a6);
                        ATIPutExtReg(0xA7, restore->a7);
                        ATIPutExtReg(0xAB, restore->ab);
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
        ATIPutExtReg(0xB8, restore->b8);
        ATIPutExtReg(0xB9, restore->b9);
        ATIPutExtReg(0xBA, restore->ba);
        ATIPutExtReg(0xBD, restore->bd);

        /*
         * Restore the generic VGA registers.
         */
        vgaHWRestore((vgaHWPtr)restore);
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
        unsigned char ae, b2;   /* The oddballs */

        /*
         * Unlock registers.
         */
        ATIEnterLeave(ENTER);

        /*
         * Get back to bank zero.
         */
        b2 = ATIGetExtReg(0xB2);
        if (ATIBoard == ATI_BOARD_V3)
                ATIPutExtReg(0xB2, b2 & 0xE1);
        else
        {
                ATIPutExtReg(0xB2, 0);
                ATIB2Reg = 0;
                if (ATIBoard > ATI_BOARD_V5)
                {
                        ae = ATIGetExtReg(0xAE);
                        ATIPutExtReg(0xAE, ae & 0xF0);
                }
        }

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
        save->b8 = ATIGetExtReg(0xB8);
        save->b9 = ATIGetExtReg(0xB9);
        save->ba = ATIGetExtReg(0xBA);
        save->bd = ATIGetExtReg(0xBD);
        if (ATIBoard != ATI_BOARD_V3)
        {
                save->be = ATIGetExtReg(0xBE);
                if (ATIBoard > ATI_BOARD_V5)
                {
                        save->bf = ATIGetExtReg(0xBF);
                        save->a3 = ATIGetExtReg(0xA3);
                        save->a6 = ATIGetExtReg(0xA6);
                        save->a7 = ATIGetExtReg(0xA7);
                        save->ab = ATIGetExtReg(0xAB);
                        save->ac = ATIGetExtReg(0xAC);
                        save->ad = ATIGetExtReg(0xAD);
                        save->ae = ae;
                }
        }
        return ((void *) save);
}

/*
 * ATIInit --
 *
 * This is the most important function (after the Probe function).  This
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
        {
                if (vga256InfoRec.videoRam <= 256)
                        new->b0 = 0x26;
                else
                        new->b0 = 0x30;
        }
        else
                new->b0 = (ATIGetExtReg(0xB0) & 0x98) | 0x21;
#endif
        new->b1 = (ATIGetExtReg(0xB1) & 0x04)       ;
        new->b3 = (ATIGetExtReg(0xB3) & 0x23)       ;
        new->b4 = 0;
        new->b5 = 0;
#if defined(MONOVGA) || defined(XF86VGA16)
        new->b6 = 0x41;
#else
        new->b6 = 0x45;
#endif
        new->b8 = (ATIGetExtReg(0xB8) & 0xC0)       ;
        new->b9 = (ATIGetExtReg(0xB9) & 0x7F)       ;
        new->ba = (ATIGetExtReg(0xBA) & 0xC0)       ;
        new->bd = (ATIGetExtReg(0xBD) & 0x02)       ;
        if (ATIBoard == ATI_BOARD_V3)
                new->b2 = (ATIGetExtReg(0xB2) & 0xC0)       ;
        else
        {
                new->b2 = 0;
                new->be = (ATIGetExtReg(0xBE) & 0x30) | 0x09;
                if (ATIBoard > ATI_BOARD_V5)
                {
                        new->bf = (ATIGetExtReg(0xBF) & 0x70)       ;
                        new->a3 = (ATIGetExtReg(0xA3) & 0xC7)       ;
                        new->a6 = (ATIGetExtReg(0xA6) & 0x7A) | 0x05;
                        new->a7 = (ATIGetExtReg(0xA7) & 0xFE)       ;
                        new->ab = (ATIGetExtReg(0xAB) & 0xE7)       ;
                        new->ac = (ATIGetExtReg(0xAC) & 0xBE)       ;
                        new->ad = (ATIGetExtReg(0xAD) & 0xF0)       ;
                        new->ae = (ATIGetExtReg(0xAE) & 0xF0)       ;
                }
        }
        if (mode->Flags & V_INTERLACE)  /* Enable interlacing */
                if (ATIBoard == ATI_BOARD_V3)
                        new->b2 |= 0x01;
                else
                        new->be |= 0x02;

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
        unsigned char tmp = ATIGetExtReg(0xB0);

        outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
        outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);

        if (ATIBoard < ATI_BOARD_PLUS)
                ATIPutExtReg(0xB0, (tmp & 0x3F) | ((Base & 0x030000) >> 10));
        else
        {
                ATIPutExtReg(0xB0, (tmp & 0xBF) | ((Base & 0x010000) >> 10));
                tmp = ATIGetExtReg(0xA3);
                ATIPutExtReg(0xA3, (tmp & 0xEF) | ((Base & 0x020000) >> 13));

                /*
                 * I don't know if this also applies to Mach64's, but give it
                 * a shot...
                 */
                if (ATIBoard >= ATI_BOARD_MACH32)
                {
                        tmp = ATIGetExtReg(0xAD);
                        ATIPutExtReg(0xAD,
                                (tmp & 0xFB) | ((Base & 0x040000) >> 12));
                }
        }
}

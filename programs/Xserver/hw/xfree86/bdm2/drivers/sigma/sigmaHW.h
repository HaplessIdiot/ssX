/* $XConsortium: sigmaHW.h,v 1.1 94/03/28 21:20:07 dpw Exp $ */
/*
 * BDM2: Banked dumb monochrome driver
 * Pascal Haible 9/93, haible@izfm.uni-stuttgart.de
 *
 * bdm2/driver/sigma/sigmaHW.c
 * Register definitions for Sigma L-View and Sigma LaserView PLUS
 *
 * see bdm2/COPYRIGHT for copyright and disclaimers.
 */

/* Many thanks to Rich Murphey (rich@rice.edu) who sent me the docs */


#include "sigmaPorts.h"

/* Memory Base Address */
/* According to the docs, this can be anywhere (on a 64k boundary)
 * in the second half MB, but show me the PC running *N*X where
 * 0x8000, 0x9000 or 0xF0000 is available!
 * On Systems without VGA, 0xA0000, 0xB0000 and 0xC0000 is available */
/* So: valid choices for bdm2 are 0x{ABCDE}0000L */
/* 0xE0000 is the default, other values are selected in Xconfig
 * with ' membase "xxxx" ' */
#define SIGMA_DEF_MEM_BASE	(0xE0000L)

#if 0
#define SIGMA_DEF_MEM_BASE_BIT16    ((SIGMA_DEF_MEM_BASE & 0x10000L) ? 1 : 0)
#define SIGMA_DEF_MEM_BASE_BIT17    ((SIGMA_DEF_MEM_BASE & 0x20000L) ? 1 : 0)
#define SIGMA_DEF_MEM_BASE_BIT18    ((SIGMA_DEF_MEM_BASE & 0x40000L) ? 1 : 0)
#endif

/* Sigma LaserView [PLUS] has its video mem 'high address aligned',
 * the first line is line 848. Unfortunately, this is not on a bank boundary.
 * Following trick to cope with this:
 * first two banking windows (out of four) for the first 'logical' bank:
 *       16k from SIGMA_MEM_BASE+4k to SIGMA_MEM_BASE+20k.
 * Quite ugly: this way I have to bank two times 16k to have once 16k;
 * 'logical' banks do not match real banks.
 */

#define SIGMA_BANK_SIZE		(0x4000L)	/* 16k */

#define SIGMA_DEF_MAP_BASE	(SIGMA_DEF_MEM_BASE)

#define SIGMA_MAP_SIZE		(4*SIGMA_BANK_SIZE)

/* alignement gap at the beginning of the line, in bytes */
#define SIGMA_X_GAP		(48)

/* alignement gap between the beginning of the 'logical' bank 0
 *	(real bank 13) and the first visible line (16 lines), in bytes
 */
#define SIGMA_Y_GAP		(0x1000L)	/* 4k */

/* rel. to SIGMA_MAP_BASE */
#define SIGMA_MEM_BASE_BANK1	(SIGMA_Y_GAP + SIGMA_X_GAP)
#define SIGMA_MEM_BASE_BANK2	(2*SIGMA_BANK_SIZE+SIGMA_Y_GAP+SIGMA_X_GAP)
#define SIGMA_BANK1_BOTTOM	(SIGMA_MEM_BASE_BANK1)
#define SIGMA_BANK1_TOP		(SIGMA_BANK1_BOTTOM+SIGMA_BANK_SIZE)
#define SIGMA_BANK2_BOTTOM	(SIGMA_MEM_BASE_BANK2)
#define SIGMA_BANK2_TOP		(SIGMA_BANK2_BOTTOM+SIGMA_BANK_SIZE)

#define SIGMA_SEGMENT_SIZE	(SIGMA_BANK_SIZE)
#define SIGMA_SEGMENT_SHIFT	(14)		/* 16k */
#define SIGMA_SEGMENT_MASK	(0x3FFFL)

#define SIGMA_HDISPLAY		(1664)
#define SIGMA_VDISPLAY		(1200)

#define SIGMA_SCAN_LINE_WIDTH	(2048)

#define sigma_Textmode		(1409)
#define sigma_Graphmode		(1410)

/* $XFree86: xc/programs/Xserver/hw/xfree86/etc/ati.test.c,v 3.1 1994.06.23 Exp $ */
/* ati.test.c -- Gather information about ATI VGA WONDER cards
 * Created: Sun Aug  9 10:15:01 1992
 * Author: Rickard E. Faith, faith@cs.unc.edu
 * Copyright 1992 Rickard E. Faith; All Rights Reserved.
 * May be distributed freely for any purpose as long as the
 * copyright notice and the warranty disclaimer are kept.
 * This programme comes with ABSOLUTELY NO WARRANTY.
 *
 * Log
 * - Heavily modified in 1994 by Marc A. La France, tsi@gpu.srv.ualberta.ca
 *
 * Please see main driver (ati_driver.c) for acknowledgements.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>

#ifndef linux
#define iopl(Level)     /* Nothing */
#endif

/* These are from the x386 compile.h for Linux */
static inline void outb(unsigned short port, char value)
{
   __asm__ volatile ("outb %0,%1"
		     ::"a" ((char) value),"d" ((unsigned short) port));
}
static inline void outw(unsigned short port, short value)
{
   __asm__ volatile ("outw %0,%1"
		     ::"a" ((short) value),"d" ((unsigned short) port));
}
static inline void outl(unsigned short port, long value)
{
   __asm__ volatile ("outl %0,%1"
		     ::"a" ((long) value),"d" ((unsigned short) port));
}

static inline unsigned char inb(unsigned short port)
{
   unsigned char _v;
   __asm__ volatile ("inb %1,%0"
		     :"=a" (_v):"d" ((unsigned short) port));
   return _v;
}
static inline unsigned short inw(unsigned short port)
{
   unsigned short _v;
   __asm__ volatile ("inw %1,%0"
		     :"=a" (_v):"d" ((unsigned short) port));
   return _v;
}
static inline unsigned long inl(unsigned short port)
{
   unsigned long _v;
   __asm__ volatile ("inl %1,%0"
		     :"=a" (_v):"d" ((unsigned short) port));
   return _v;
}

static int ATIExtReg;

/* 8514/A VESA approved register definitions */
#define DISP_STAT		0x02e8		/* Read */
#define SENSE				0x0001	/* Presumably belong here */
#define VBLANK				0x0002
#define HORTOG				0x0004
#define H_TOTAL			0x02e8		/* Write */
#define DAC_MASK		0x02ea
#define DAC_R_INDEX		0x02eb
#define DAC_W_INDEX		0x02ec
#define DAC_DATA		0x02ed
#define H_DISP			0x06e8
#define H_SYNC_STRT		0x0ae8
#define H_SYNC_WID		0x0ee8
#define HSYNCPOL_POS			0x0000
#define HSYNCPOL_NEG			0x0020
#define H_POLARITY_POS			HSYNCPOL_POS	/* Sigh */
#define H_POLARITY_NEG			HSYNCPOL_NEG	/* Sigh */
#define V_TOTAL			0x12e8
#define V_DISP			0x16e8
#define V_SYNC_STRT		0x1ae8
#define V_SYNC_WID		0x1ee8
#define VSYNCPOL_POS			0x0000
#define VSYNCPOL_NEG			0x0020
#define V_POLARITY_POS			VSYNCPOL_POS	/* Sigh */
#define V_POLARITY_NEG			VSYNCPOL_NEG	/* Sigh */
#define DISP_CNTL		0x22e8
#define ODDBNKENAB			0x0001
#define MEMCFG_2			0x0000
#define MEMCFG_4			0x0002
#define MEMCFG_6			0x0004
#define MEMCFG_8			0x0006
#define DBLSCAN				0x0008
#define INTERLACE			0x0010
#define DISPEN_NC			0x0000
#define DISPEN_ENAB			0x0020
#define DISPEN_DISAB			0x0040
#define R_H_TOTAL		0x26e8		/* Read */
/*	?			0x2ae8 */
/*	?			0x2ee8 */
/*	?			0x32e8 */
/*	?			0x36e8 */
/*	?			0x3ae8 */
/*	?			0x3ee8 */
#define SUBSYS_STAT		0x42e8		/* Read */
#define VBLNKFLG			0x0001
#define PICKFLAG			0x0002
#define INVALIDIO			0x0004
#define GPIDLE				0x0008
#define MONITORID_MASK			0x0070
#define MONITORID_8507				0x0010
#define MONITORID_8514				0x0020
#define MONITORID_8503				0x0050
#define MONITORID_8512				0x0060
#define MONITORID_8513				0x0060
#define MONITORID_NONE				0x0070
#define _8PLANE				0x0080
#define SUBSYS_CNTL		0x42e8		/* Write */
#define RVBLNKFLG			0x0001
#define RPICKFLAG			0x0002
#define RINVALIDIO			0x0004
#define IVBLNKFLG			0x0100
#define IPICKFLAG			0x0200
#define IINVALIDIO			0x0400
#define IGPIDLE				0x0800
#define CHPTEST_NC			0x0000
#define CHPTEST_NORMAL			0x1000
#define CHPTEST_ENAB			0x2000
#define GPCTRL_NC			0x0000
#define GPCTRL_ENAB			0x4000
#define GPCTRL_RESET			0x8000
#define ROM_PAGE_SEL		0x46e8
#define ADVFUNC_CNTL		0x4ae8
#define DISABPASSTHRU			0x0001
#define CLOKSEL				0x0004
/*	?			0x4ee8 */
/*	?			0x52e8 */
/*	?			0x56e8 */
/*	?			0x5ae8 */
/*	?			0x5ee8 */
/*	?			0x62e8 */
/*	?			0x66e8 */
/*	?			0x6ae8 */
/*	?			0x6ee8 */
/*	?			0x72e8 */
/*	?			0x76e8 */
/*	?			0x7ae8 */
/*	?			0x7ee8 */
#define CUR_Y			0x82e8
#define CUR_X			0x86e8
#define DESTY_AXSTP		0x8ae8
#define DESTX_DIASTP		0x8ee8
#define ERR_TERM		0x92e8
#define MAJ_AXIS_PCNT		0x96e8
#define GP_STAT			0x9ae8		/* Read */
#define GE_STAT			0x9ae8		/* Alias */
#define DATARDY				0x0100
#define DATA_READY			DATARDY	/* Alias */
#define GPBUSY				0x0200
#define CMD			0x9ae8		/* Write */
#define WRTDATA				0x0001
#define PLANAR				0x0002
#define LASTPIX				0x0004
#define LINETYPE			0x0008
#define DRAW				0x0010
#define INC_X				0x0020
#define YMAJAXIS			0x0040
#define INC_Y				0x0080
#define PCDATA				0x0100
#define _16BIT				0x0200
#define CMD_NOP				0x0000
#define CMD_OP_MSK			0xf000
#define BYTSEQ					0x1000
#define CMD_LINE				0x2000
#define CMD_RECT				0x4000
#define CMD_RECTV1				0x6000
#define CMD_RECTV2				0x8000
#define CMD_LINEAF				0xa000
#define CMD_BITBLT				0xc000
#define SHORT_STROKE		0x9ee8
#define SSVDRAW				0x0010
#define VECDIR_000			0x0000
#define VECDIR_045			0x0020
#define VECDIR_090			0x0040
#define VECDIR_135			0x0060
#define VECDIR_180			0x0080
#define VECDIR_225			0x00a0
#define VECDIR_270			0x00c0
#define VECDIR_315			0x00e0
#define BKGD_COLOR		0xa2e8
#define FRGD_COLOR		0xa6e8
#define WRT_MASK		0xaae8
#define RD_MASK			0xaee8
#define COLOR_CMP		0xb2e8
#define BKGD_MIX		0xb6e8
/*					0x001f	See MIX_* definitions below */
#define BSS_BKGDCOL			0x0000
#define BSS_FRGDCOL			0x0020
#define BSS_PCDATA			0x0040
#define BSS_BITBLT			0x0060
#define FRGD_MIX		0xbae8
/*					0x001f	See MIX_* definitions below */
#define FSS_BKGDCOL			0x0000
#define FSS_FRGDCOL			0x0020
#define FSS_PCDATA			0x0040
#define FSS_BITBLT			0x0060
#define MULTIFUNC_CNTL		0xbee8
#define MIN_AXIS_PCNT			0x0000
#define SCISSORS_T			0x1000
#define SCISSORS_L			0x2000
#define SCISSORS_B			0x3000
#define SCISSORS_R			0x4000
#define MEM_CNTL			0x5000
#define HORCFG_4				0x0000
#define HORCFG_5				0x0001
#define HORCFG_8				0x0002
#define HORCFG_10				0x0003
#define VRTCFG_2				0x0000
#define VRTCFG_4				0x0004
#define VRTCFG_6				0x0008
#define VRTCFG_8				0x000c
#define BUFSWP					0x0010
#define PATTERN_L			0x8000
#define PATTERN_H			0x9000
#define PIX_CNTL			0xa000
#define PLANEMODE				0x0004
#define COLCMPOP_F				0x0000
#define COLCMPOP_T				0x0008
#define COLCMPOP_GE				0x0010
#define COLCMPOP_LT				0x0018
#define COLCMPOP_NE				0x0020
#define COLCMPOP_EQ				0x0028
#define COLCMPOP_LE				0x0030
#define COLCMPOP_GT				0x0038
#define	MIXSEL_FRGDMIX				0x0000
#define	MIXSEL_PATT				0x0040
#define	MIXSEL_EXPPC				0x0080
#define	MIXSEL_EXPBLT				0x00c0
/*	?			0xc2e8 */
/*	?			0xc6e8 */
/*	?			0xcae8 */
/*	?			0xcee8 */
/*	?			0xd2e8 */
/*	?			0xd6e8 */
/*	?			0xdae8 */
/*	?			0xdee8 */
#define PIX_TRANS		0xe2e8
/*	?			0xe6e8 */
/*	?			0xeae8 */
/*	?			0xeee8 */
/*	?			0xf2e8 */
/*	?			0xf6e8 */
/*	?			0xfae8 */
/*	?			0xfee8 */

/* ATI Mach8 & Mach32 register definitions */
/*	?			0x02ee */
/*	?			0x06ee */
#define CURSOR_OFFSET_LO	0x0aee
#define CURSOR_OFFSET_HI	0x0eee
#define CONFIG_STATUS_1		0x12ee		/* Read */
#define CLK_MODE			0x0001			/* Mach8 */
#define BUS_16				0x0002			/* Mach8 */
#define MC_BUS				0x0004			/* Mach8 */
#define EEPROM_ENA			0x0008			/* Mach8 */
#define DRAM_ENA			0x0010			/* Mach8 */
#define MEM_INSTALLED			0x0060			/* Mach8 */
#define ROM_ENA				0x0080			/* Mach8 */
#define ROM_PAGE_ENA			0x0100			/* Mach8 */
#define ROM_LOCATION			0xfe00			/* Mach8 */
#define _8514_ONLY			0x0001			/* Mach32 */
#define BUS_TYPE			0x000e			/* Mach32 */
#define ISA_16_BIT				0x0000		/* Mach32 */
#define EISA					0x0002		/* Mach32 */
#define MICRO_C_16_BIT				0x0004		/* Mach32 */
#define MICRO_C_8_BIT				0x0006		/* Mach32 */
#define LOCAL_386SX				0x0008		/* Mach32 */
#define LOCAL_386DX				0x000a		/* Mach32 */
#define LOCAL_486				0x000c		/* Mach32 */
#define PCI					0x000e		/* Mach32 */
#define MEM_TYPE			0x0070			/* Mach32 */
#define CHIP_DIS			0x0080			/* Mach32 */
#define TST_VCTR_ENA			0x0100			/* Mach32 */
#define DACTYPE				0x0e00			/* Mach32 */
#define MC_ADR_DECODE			0x1000			/* Mach32 */
#define CARD_ID				0xe000			/* Mach32 */
#define HORZ_CURSOR_POSN	0x12ee		/* Write */	/* Mach32 */
#define CONFIG_STATUS_2		0x16ee		/* Read */
#define SHARE_CLOCK			0x0001			/* Mach8 */
#define HIRES_BOOT			0x0002			/* Mach8 */
#define EPROM_16_ENA			0x0004			/* Mach8 */
#define WRITE_PER_BIT			0x0008			/* Mach8 */
#define FLASH_ENA			0x0010			/* Mach8 */
#define SLOW_SEQ_EN			0x0001			/* Mach32 */
#define MEM_ADDR_DIS			0x0002			/* Mach32 */
#define ISA_16_ENA			0x0004			/* Mach32 */
#define KOR_TXT_MODE_ENA		0x0008			/* Mach32 */
#define LOCAL_BUS_SUPPORT		0x0030			/* Mach32 */
#define LOCAL_BUS_CONFIG_2		0x0040			/* Mach32 */
#define LOCAL_BUS_RD_DLY_ENA		0x0080			/* Mach32 */
#define LOCAL_DAC_EN			0x0100			/* Mach32 */
#define LOCAL_RDY_EN			0x0200			/* Mach32 */
#define EEPROM_ADR_SEL			0x0400			/* Mach32 */
#define GE_STRAP_SEL			0x0800			/* Mach32 */
#define VESA_RDY			0x1000			/* Mach32 */
#define Z4GB				0x2000			/* Mach32 */
#define LOC2_MDRAM			0x4000			/* Mach32 */
#define VERT_CURSOR_POSN	0x16ee		/* Write */	/* Mach32 */
#define CURSOR_COLOR_0		0x1aee				/* Mach32 */
#define CURSOR_COLOR_1		0x1aef				/* Mach32 */
#define HORZ_CURSOR_OFFSET	0x1eee				/* Mach32 */
#define VERT_CURSOR_OFFSET	0x1eef				/* Mach32 */
/*	?			0x22ee */
#define CRT_PITCH		0x26ee
#define CRT_OFFSET_LO		0x2aee
#define CRT_OFFSET_HI		0x2eee
/*	?			0x32ee */
#define MISC_OPTIONS		0x36ee				/* Mach32 */
#define W_STATE_ENA			0x0000			/* Mach32 */
#define HOST_8_ENA			0x0001			/* Mach32 */
#define MEM_SIZE_ALIAS			0x000c			/* Mach32 */
#define MEM_SIZE_512K				0x0000		/* Mach32 */
#define MEM_SIZE_1M				0x0004		/* Mach32 */
#define MEM_SIZE_2M				0x0008		/* Mach32 */
#define MEM_SIZE_4M				0x000c		/* Mach32 */
#define DISABLE_VGA			0x0010			/* Mach32 */
#define _16_BIT_IO			0x0020			/* Mach32 */
#define DISABLE_DAC			0x0040			/* Mach32 */
#define DLY_LATCH_ENA			0x0080			/* Mach32 */
#define TEST_MODE			0x0100			/* Mach32 */
#define BLK_WR_ENA			0x0400			/* Mach32 */
#define _64_DRAW_ENA			0x0800			/* Mach32 */
#define EXT_CURSOR_COLOR_0	0x3aee				/* Mach32 */
#define EXT_CURSOR_COLOR_1	0x3eee				/* Mach32 */
#define MEM_BNDRY		0x42ee				/* Mach32 */
#define SHADOW_CTL		0x46ee
#define CLOCK_SEL		0x4aee
/*	DISABPASSTHRU			0x0001	See ADVFUNC_CNTL */
#define VFIFO_DEPTH_1			0x0100			/* Mach32 */
#define VFIFO_DEPTH_2			0x0200			/* Mach32 */
#define VFIFO_DEPTH_3			0x0300			/* Mach32 */
#define VFIFO_DEPTH_4			0x0400			/* Mach32 */
#define VFIFO_DEPTH_5			0x0500			/* Mach32 */
#define VFIFO_DEPTH_6			0x0600			/* Mach32 */
#define VFIFO_DEPTH_7			0x0700			/* Mach32 */
#define VFIFO_DEPTH_8			0x0800			/* Mach32 */
#define VFIFO_DEPTH_9			0x0900			/* Mach32 */
#define VFIFO_DEPTH_A			0x0a00			/* Mach32 */
#define VFIFO_DEPTH_B			0x0b00			/* Mach32 */
#define VFIFO_DEPTH_C			0x0c00			/* Mach32 */
#define VFIFO_DEPTH_D			0x0d00			/* Mach32 */
#define VFIFO_DEPTH_E			0x0e00			/* Mach32 */
#define VFIFO_DEPTH_F			0x0f00			/* Mach32 */
#define COMPOSITE_SYNC			0x1000
/*	?			0x4eee */
#define ROM_ADDR_1		0x52ee
/*	?			0x56ee */
#define SHADOW_SET		0x5aee
#define MEM_CFG			0x5eee				/* Mach32 */
#define EXT_GE_STATUS		0x62ee		/* Read */	/* Mach32 */
#define HORZ_OVERSCAN		0x62ee		/* Write */	/* Mach32 */
#define VERT_OVERSCAN		0x66ee				/* Mach32 */
/*	?			0x6aee */
#define GE_OFFSET_LO		0x6eee
#define GE_OFFSET_HI		0x72ee
#define GE_PITCH		0x76ee
#define EXT_GE_CONFIG		0x7aee				/* Mach32 */
#define PIXEL_WIDTH_4			0x0000			/* Mach32 */
#define PIXEL_WIDTH_8			0x0010			/* Mach32 */
#define PIXEL_WIDTH_16			0x0020			/* Mach32 */
#define PIXEL_WIDTH_24			0x0030			/* Mach32 */
#define RGB16_555			0x0000			/* Mach32 */
#define RGB16_565			0x0040			/* Mach32 */
#define RGB16_655			0x0080			/* Mach32 */
#define RGB16_664			0x00c0			/* Mach32 */
#define MULTIPLEX_PIXELS		0x0100			/* Mach32 */
#define RGB24				0x0000			/* Mach32 */
#define RGBx24				0x0200			/* Mach32 */
#define BGR24				0x0400			/* Mach32 */
#define xBGR24				0x0600			/* Mach32 */
#define DAC_8_BIT_EN			0x4000			/* Mach32 */
#define PIX_WIDTH_16BPP			PIXEL_WIDTH_16		/* Mach32 */
#define ORDER_16BPP_565			RGB16_565		/* Mach32 */
#define MISC_CNTL		0x7eee				/* Mach32 */
/*	?			0x82ee */
/*	?			0x86ee */
/*	?			0x8aee */
#define R_EXT_GE_CONFIG		0x8eee				/* Mach32 */
#define R_MISC_CNTL		0x92ee				/* Mach32 */
#define BRES_COUNT		0x96ee				/* Mach32 */
#define EXT_FIFO_STATUS		0x9aee		/* Read */
#define LINEDRAW_INDEX		0x9aee		/* Write */
/*	?			0x9eee */
#define LINEDRAW_OPT		0xa2ee
#define BOUNDS_RESET			0x0100
#define CLIP_MODE_0			0x0000	/* Clip exception disabled */
#define CLIP_MODE_1			0x0200	/* Line segments */
#define CLIP_MODE_2			0x0400	/* Polygon boundary lines */
#define CLIP_MODE_3			0x0600	/* Patterned lines */
#define DEST_X_START		0xa6ee				/* Mach32 */
#define DEST_X_END		0xaaee				/* Mach32 */
#define DEST_Y_END		0xaeee				/* Mach32 */
/*	?			0xb2ee */
/*	?			0xb6ee */
#define ALU_FG_FN		0xbaee				/* Mach32 */
/*	?			0xbeee */
/*	?			0xc2ee */
/*	?			0xc6ee */
/*	?			0xcaee */
#define DP_CONFIG		0xceee
#define READ_WRITE			0x0001
#define DATA_WIDTH			0x0200
#define DATA_ORDER			0x1000
#define FG_COLOR_SRC_FG			0x2000
#define FG_COLOR_SRC_BLIT		0x6000
/*	?			0xd2ee */
/*	?			0xd6ee */
#define READ_SRC_X		0xdaee		/* Read */	/* Mach32 */
#define EXT_SCISSOR_L		0xdaee		/* Write */	/* Mach32 */
#define EXT_SCISSOR_T		0xdeee				/* Mach32 */
#define EXT_SCISSOR_R		0xe2ee				/* Mach32 */
#define EXT_SCISSOR_B		0xe6ee				/* Mach32 */
/*	?			0xeaee */
/*	?			0xeeee */
/*	?			0xf2ee */
/*	?			0xf6ee */
#define CHIP_ID			0xfaee				/* Mach32 */
#define LINEDRAW		0xfeee

/* ATI Mach64 register definitions */
/*	?			0x12ec */
/*	?			0x22ec */
/*	?			0x32ec */
#define SCRATCH_REG0		0x42ec
#define MEM_INFO		0x52ec
/*	?			0x62ec */
#define CONFIG_STATUS_0		0x72ec
/*	?			0x82ec */
/*	?			0x92ec */
/*	?			0xa2ec */
/*	?			0xb2ec */
/*	?			0xc2ec */
/*	?			0xd2ec */
/*	?			0xe2ec */
/*	?			0xf2ec */

/* Miscellaneous */

/* Current X, Y & Dest X, Y mask */
#define COORD_MASK	0x07ff

/* The Mixes */
#define	MIX_MASK			0x001f

#define	MIX_NOT_DST			0x0000
#define	MIX_0				0x0001
#define	MIX_1				0x0002
#define	MIX_DST				0x0003
#define	MIX_NOT_SRC			0x0004
#define	MIX_XOR				0x0005
#define	MIX_XNOR			0x0006
#define	MIX_SRC				0x0007
#define	MIX_NAND			0x0008
#define	MIX_NOT_SRC_OR_DST		0x0009
#define	MIX_SRC_OR_NOT_DST		0x000a
#define	MIX_OR				0x000b
#define	MIX_AND				0x000c
#define	MIX_SRC_AND_NOT_DST		0x000d
#define	MIX_NOT_SRC_AND_DST		0x000e
#define	MIX_NOR				0x000f

#define	MIX_MIN				0x0010
#define	MIX_DST_MINUS_SRC		0x0011
#define	MIX_SRC_MINUS_DST		0x0012
#define	MIX_PLUS			0x0013
#define	MIX_MAX				0x0014
#define	MIX_HALF__DST_MINUS_SRC		0x0015
#define	MIX_HALF__SRC_MINUS_DST		0x0016
#define	MIX_AVERAGE			0x0017
#define	MIX_DST_MINUS_SRC_SAT		0x0018
#define	MIX_SRC_MINUS_DST_SAT		0x001a
#define	MIX_HALF__DST_MINUS_SRC_SAT	0x001c
#define	MIX_HALF__SRC_MINUS_DST_SAT	0x001e
#define	MIX_AVERAGE_SAT			0x001f
#define MIX_FN_PAINT			MIX_SRC

/* Wait until "n" queue entries are free */
#define ibm8514WaitQueue(n)						\
	{								\
		while (inw(GP_STAT) & (0x0100 >> (n)));			\
	}
#define ATIWaitQueue(n)							\
	{								\
		while (inw(EXT_FIFO_STATUS) & (0x10000 >> (n)));	\
	}

/* Wait until GP is idle and queue is empty */
#define WaitIdleEmpty()							\
	{								\
		while (inw(GP_STAT) & (GPBUSY | 1));			\
	}
#define ProbeWaitIdleEmpty()						\
	{								\
		int i;							\
		for (i = 0; i < 100000; i++)				\
			if (!(inw(GP_STAT) & (GPBUSY | 1)))		\
				break;					\
	}

/* Wait until GP has data available */
#define WaitDataReady()							\
	{								\
		while (!(inw(GP_STAT) & DATARDY));			\
	}

#define GetReg(Register, Index)		\
	(				\
		outb(Register, Index),	\
		inb(Register + 1)	\
	)
#define GetATIReg(Index)		GetReg(ATIExtReg, Index)

#define FALSE 0
#define TRUE  1

#define ATI_CHIP_NONE      0
#define ATI_CHIP_18800     1
#define ATI_CHIP_18800_1   2
#define ATI_CHIP_28800_2   3
#define ATI_CHIP_28800_4   4
#define ATI_CHIP_28800_5   5
#define ATI_CHIP_28800_6   6
#define ATI_CHIP_38800     7    /* Mach8 */
#define ATI_CHIP_68800     8    /* Mach32 */
#define ATI_CHIP_68800_3   9    /* Mach32 */
#define ATI_CHIP_68800_6  10    /* Mach32 */
#define ATI_CHIP_68800LX  11    /* Mach32 */
#define ATI_CHIP_68800AX  12    /* Mach32 */
#define ATI_CHIP_88800    13    /* Mach64 */
static const char *ChipNames[] =
{
	"Unknown",
	"ATI 18800",
	"ATI 18800-1",
	"ATI 28800-2",
	"ATI 28800-4",
	"ATI 28800-5",
	"ATI 28800-6",
	"ATI 38800",
	"ATI 68800",
	"ATI 68800-3",
	"ATI 68800-6",
	"ATI 68800LX",
	"ATI 68800AX",
	"ATI 88800"
};

#define ATI_BOARD_NONE    0
#define ATI_BOARD_EGA     1
#define ATI_BOARD_BASIC   2
#define ATI_BOARD_V3      3
#define ATI_BOARD_V4      4
#define ATI_BOARD_V5      5
#define ATI_BOARD_PLUS    6
#define ATI_BOARD_XL      7
#define ATI_BOARD_XL24    8
#define ATI_BOARD_MACH8   9
#define ATI_BOARD_MACH32 10
#define ATI_BOARD_MACH64 11
static const char *BoardNames[] =
{
	"Unknown",
	"EGA Wonder800+",
	"VGA Basic16",
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
	unsigned char AllPixelsOK;

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
#		define TestPixel               \
			Test_Case[Case_Number].Coordinates[Pixel_Number]
#		define ForEachTestPixel        \
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
#		undef ForEachTestPixel
#		undef TestPixel
	}

	/* Restore what was changed and correct MISC_OPTIONS register */
	outw(EXT_GE_CONFIG, saved_EXT_GE_CONFIG);
	outw(MISC_OPTIONS, saved_MISC_OPTIONS |
		Test_Case[Case_Number].Miscellaneous_Options_Setting);
	outw(MEM_BNDRY, saved_MEM_BNDRY);

	/* Re-enable VGA passthrough */
	outw(CLOCK_SEL, saved_CLOCK_SEL & ~DISABPASSTHRU);

	/* Tell caller the REAL story */
	return Test_Case[Case_Number].videoRamSize;
}

static void
PrintRegisters(int Port, int Number, unsigned char * Name)
{
	int Index;

	printf("\n\n Current %s register values:", Name);
	for (Index = 0;  Index < Number;  Index++)
	{
		if((Index % 4) == 0)
		{
			if ((Index % 16) == 0)
				printf("\n 0x%02X:", Index);
			printf(" ");
		}
		printf("%02X", GetReg(Port, Index));
	}
}

int
main(void)
{
#	define Signature	" 761295520"
#	define Signature_Size	10
#	define BIOS_DATA_SIZE	(0x80 + Signature_Size)
	unsigned char		BIOS_Data[BIOS_DATA_SIZE];
#	define Signature_Offset	0x30
#	define BIOS_Signature	(BIOS_Data + Signature_Offset)
	unsigned char * Signature_Found;
	int fd;
	int Index, IO_Value;
	int MachvideoRam = 0;
	int VGAWondervideoRam = 0;
	const int videoRamSizes[] =
		{0, 256, 512, 1024, 2*1024, 4*1024, 6*1024, 8*1024, 0, 0};
	unsigned char * ptr;
	unsigned char misc;
	unsigned char ATIChip = ATI_CHIP_NONE;
	unsigned char ATIBoard = ATI_BOARD_NONE;
	unsigned char ATIDac = ATI_DAC_GENERIC;

	if ((fd = open("/dev/mem", O_RDONLY)) < 0 ||
	    lseek(fd, 0xc0000, SEEK_SET) < 0 ||
	    read(fd, BIOS_Data, BIOS_DATA_SIZE) != BIOS_DATA_SIZE)
	{
		if (fd >= 0)
			close(fd);
		printf(" Failed to read VGA BIOS.  Cannot detect ATI card.\n");
		return 2;
	}
	close(fd);

	/* Look for signature in BIOS */
	Signature_Found =
		findsubstring(Signature, Signature_Size,
			BIOS_Data, BIOS_DATA_SIZE);
	if (!Signature_Found)
		printf(" ATI Signature not found in BIOS\n");
	else if (Signature_Found != BIOS_Signature)
		printf(" ATI Signature found at offset 0x%04x in BIOS\n",
			Signature_Found - BIOS_Data);

	iopl(3);		/* Enable I/O ports */

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

	if ((ATIBoard == ATI_BOARD_NONE) &&
	    (Signature_Found == BIOS_Signature))
	{
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

			if (ATIBoard == ATI_BOARD_MACH8)
			{
				ATIChip = ATI_CHIP_38800;
				if (BIOS_Data[0x44] & 0x80)
					ATIDac = ATI_DAC_SC11483;
				MachvideoRam =
					videoRamSizes[((inw(CONFIG_STATUS_1) &
						MEM_INSTALLED) >> 5) + 2];
			}
			else if (ATIBoard == ATI_BOARD_MACH32)
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
		else if (BIOS_Data[0x40] == '3')
		if (BIOS_Data[0x41] == '2')
			ATIBoard = ATI_BOARD_EGA;
		else if (BIOS_Data[0x41] == '3')
			ATIBoard = ATI_BOARD_BASIC;
		else if (BIOS_Data[0x41] == '1')
		{
			/* This is a VGA Wonder board of some kind */
			if ((BIOS_Data[0x43] >= '1') &&
				(BIOS_Data[0x43] <= '6'))
				ATIChip = BIOS_Data[0x43] - '0';

			switch (ATIChip)
			{
				case ATI_CHIP_18800:
					ATIBoard = ATI_BOARD_V3;
					break;

				case ATI_CHIP_18800_1:
					if (BIOS_Data[0x42] & 0x10)
						ATIBoard = ATI_BOARD_V5;
					else
						ATIBoard = ATI_BOARD_V4;
					break;

				case ATI_CHIP_28800_2:
				case ATI_CHIP_28800_4:
				case ATI_CHIP_28800_5:
				case ATI_CHIP_28800_6:
					ATIBoard = ATI_BOARD_PLUS;
					if (BIOS_Data[0x44] & 0x80)
					{
						ATIBoard = ATI_BOARD_XL;
						ATIDac = ATI_DAC_SC11483;
					}
					break;
			}
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

	/*
	 * Sometimes, the BIOS lies about the chip.
	 */
	if ((ATIChip == ATI_CHIP_28800_5) ||
		(ATIChip == ATI_CHIP_28800_6))
	{
		IO_Value = GetReg(ATIExtReg, 0xAA) & 0x0F;
		if ((IO_Value < 7) && (IO_Value > ATIChip))
			ATIChip = IO_Value;
	}

	printf("%s graphics controller and %s or compatible RAMDAC detected\n",
		ChipNames[ATIChip], DACNames[ATIDac]);
	printf("This is a %s video adapter\n\n", BoardNames[ATIBoard]);
	if (ATIBoard < ATI_BOARD_MACH64)
	{
		printf("   Signature code:                \"%c%c\"\n",
			BIOS_Data[0x40], BIOS_Data[0x41]);
		printf("\n   BIOS version:                  %d.%d\n",
		       BIOS_Data[0x4C], BIOS_Data[0x4D]);

		printf("\n   Byte at offset 0x42 =          0x%02X\n",
		       BIOS_Data[0x42]);
		printf("   8 and 16 bit ROM supported:    %s\n",
		       BIOS_Data[0x42] & 0x01 ? "Yes" : "No");
		printf("   Mouse chip present:            %s\n",
		       BIOS_Data[0x42] & 0x02 ? "Yes" : "No");
		printf("   Inport compatible mouse port:  %s\n",
		       BIOS_Data[0x42] & 0x04 ? "Yes" : "No");
		printf("   Micro Channel supported:       %s\n",
		       BIOS_Data[0x42] & 0x08 ? "Yes" : "No");
		printf("   Clock chip present:            %s\n",
		       BIOS_Data[0x42] & 0x10 ? "Yes" : "No");
		printf("   Uses C000:0000 to D000:FFFF:   %s\n",
		       BIOS_Data[0x42] & 0x80 ? "Yes" : "No");

		printf("\n   Byte at offset 0x44 =          0x%02X\n",
		       BIOS_Data[0x44]);
		printf("   Supports 70Hz non-interlaced:  %s\n",
		       BIOS_Data[0x44] & 0x01 ? "No" : "Yes");
		printf("   Supports Korean characters:    %s\n",
		       BIOS_Data[0x44] & 0x02 ? "Yes" : "No");
		printf("   Uses 45Mhz memory clock:       %s\n",
		       BIOS_Data[0x44] & 0x04 ? "Yes" : "No");
		printf("   Supports zero wait states:     %s\n",
		       BIOS_Data[0x44] & 0x08 ? "Yes" : "No");
		printf("   Uses paged ROMs:               %s\n",
		       BIOS_Data[0x44] & 0x10 ? "Yes" : "No");
		printf("   8514/A hardware on board:      %s\n",
		       BIOS_Data[0x44] & 0x40 ? "No" : "Yes");
		printf("   32K colour DAC on board:       %s\n\n",
		       BIOS_Data[0x44] & 0x80 ? "Yes" : "No");
	}

	/*
	 * Find out how much video memory the VGA Wonder side thinks it has.
	 */
	if (ATIBoard < ATI_BOARD_PLUS)
	{
		IO_Value = GetReg(ATIExtReg, 0xBB);
		if (IO_Value & 0x20)
			VGAWondervideoRam = 512;
		else
			VGAWondervideoRam = 256;
	}
	else
	{
		IO_Value = GetReg(ATIExtReg, 0xB0);
		if (IO_Value & 0x08)
			VGAWondervideoRam = 1024;
		else if (IO_Value & 0x10)
			VGAWondervideoRam = 512;
		else
			VGAWondervideoRam = 256;
	}

	if (MachvideoRam)
		printf("Accelerator video RAM:  %dkB\n", MachvideoRam);
	printf("VGA video RAM:  %dkB\n", VGAWondervideoRam);

	printf("The extended registers (ATIExtReg) are at 0x%04X\n", ATIExtReg);

	ptr = BIOS_Data;
	printf("\n BIOS data at 0xC0000:");
	for (Index = 0;  Index < BIOS_DATA_SIZE;  Index++, ptr++)
	{
		if ((Index % 4) == 0)
		{
			if ((Index % 16) == 0)
				printf("\n 0xC00%02X:", Index);
			printf(" ");
		}
		printf("%02x", *ptr);
	}

	PrintRegisters(ATIExtReg, 256, "extended");
	/* PrintRegisters(0x3C0, 32, "attribute controller"); * Not yet */
	PrintRegisters(0x3C4, 8, "sequencer");

	misc = inb(0x3CC);
	printf("\n\n Current miscellaneous output register value: 0x%02X", misc);

	PrintRegisters(0x3CE, 256, "graphics controller");

	if (misc & 0x01)
		PrintRegisters(0x3D4, 64, "colour CRT controller");
	else
		PrintRegisters(0x3B4, 64, "monochrome CRT controller");

	printf("\n\n");

	iopl(0);

	return 0;

}

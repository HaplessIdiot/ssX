/* $XConsortium: hgc1280Port.h,v 1.1 94/03/28 21:19:43 dpw Exp $ */
/*
 * BDM2: Banked dumb monochrome driver
 * Pascal Haible 8/93, 3/94 haible@izfm.uni-stuttgart.de
 *
 * bdm2/driver/hgc1280/hgc1280Ports.h
 *
 * see bdm2/COPYRIGHT for copyright and disclaimers.
 */


/* Primary I/O Base		0x3B0
 * Secondary I/O Base		0x390
 */

#define HGC_PRIM_PORT_BASE           0x3B0
#define HGC_PRIM_PORT_INDEX          0x3B0
#define HGC_PRIM_PORT_DATA           0x3B1
#define HGC_PRIM_PORT_CONTROL        0x3B8
#define HGC_PRIM_PORT_CRT_STATUS     0x3BA
#define HGC_PRIM_PORT_CONFIG         0x3BF

#define HGC_SEC_PORT_BASE           0x390
#define HGC_SEC_PORT_INDEX          0x390
#define HGC_SEC_PORT_DATA           0x391
#define HGC_SEC_PORT_CONTROL        0x398
#define HGC_SEC_PORT_CRT_STATUS     0x39A
#define HGC_SEC_PORT_CONFIG         0x39F

#define HGC_REG_BANK1           0x22
#define HGC_REG_BANK2           0x24
#define HGC_REG_SHIFT_DISPLAY   0x2D
#define HGC_REG_LEFT_BORDER     0x2A
#define HGC_REG_RIGHT_BORDER    0x2B

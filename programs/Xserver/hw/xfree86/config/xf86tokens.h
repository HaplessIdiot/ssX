/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/xf86tokens.h,v 1.1.2.3 1997/07/22 13:50:55 dawes Exp $ */
/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
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
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */

#ifndef _xf86_tokens_h
#define _xf86_tokens_h

/* 
 * We use the convention that tokens >= 1000 or < 0 do not need to be
 * present in a screen's list of valid tokens in order to be valid.
 * Also, each token should have a unique value regardless of the section
 * it is used in.
 */

#define EOF_TOKEN   -4
#define LOCK_TOKEN  -3
#define ERROR_TOKEN -2

/* 
 * These tokens are not restricted to a particular section of the file
 */

#define NUMBER		10000
#define STRING		10001
#define SECTION		10002
#define SUBSECTION	10003
#define ENDSECTION	10004
#define ENDSUBSECTION	10005
#define IDENTIFIER	10006
#define VENDOR		10007
#define DASH		10008
#define COMMA		10009

#define HRZ	1001				/* Silly name to avoid conflict with
								 * linux/param.h */
#define KHZ	1002
#define MHZ	1003

/* Driver tokens */

#define SVGA	1010
#define VGA2	1011
#define MONO	1012
#define VGA16	1013
#define ACCEL	1014
#define FBDEV	1015
#define XF86	1016

/* Mouse tokens */

#define MICROSOFT	1020
#define MOUSESYS	1021
#define MMSERIES	1022
#define LOGITECH	1023
#define BUSMOUSE	1024
#define LOGIMAN		1025
#define PS_2		1026
#define MMHITTAB	1027
#define GLIDEPOINT	1028
#define INTELLIMOUSE    1029
#define XQUE      	1030
#define OSMOUSE   	1031

/* File tokens */
#define FONTPATH	1040
#define RGBPATH		1041
#define MODULEPATH	1042
#define LOGFILEPATH	1043

/* Server Flag tokens */
#define NOTRAPSIGNALS		1050
#define DONTZAP			1051
#define DONTZOOM		1052
#define DISABLEVIDMODE		1054
#define ALLOWNONLOCAL		1055
#define DISABLEMODINDEV		1056
#define MODINDEVALLOWNONLOCAL	1057
#define ALLOWMOUSEOPENFAIL	1058

/* Monitor tokens */
#define DISPLAYSIZE	1060
#define MODELINE	1061
#define MODEL		1062
#define BANDWIDTH	1063
#define HORIZSYNC	1064
#define VERTREFRESH	1065
#define MODE		1066
#define GAMMA		1067
#define DIMENSIONS	1068

/* Mode tokens */
#define DOTCLOCK	1070
#define HTIMINGS	1071
#define VTIMINGS	1072
#define FLAGS		1073
#define HSKEW		1074
#define ENDMODE		1075

/* Screen tokens */
#define DRIVER		1080
#define MDEVICE		1081
#define MONITOR		1082
#define SCREENNO	1083
#define BLANKTIME	1084
#define STANDBYTIME	1085
#define SUSPENDTIME	1086
#define OFFTIME		1087
#define DEFBPP		1088
#define OBSDRIVER	1089

/* Mode timing tokens */
#define TT_INTERLACE	1090
#define TT_PHSYNC	1091
#define TT_NHSYNC	1092
#define TT_PVSYNC	1093
#define TT_NVSYNC	1094
#define TT_CSYNC	1095
#define TT_PCSYNC	1096
#define TT_NCSYNC	1097
#define TT_DBLSCAN	1098
#define TT_HSKEW	1099
#define TT_CUSTOM	1100

/* Module tokens */
#define LOAD		2000
#define LOAD_DRIVER	2001
#define DEFAULTCOLORDEPTH	2002	/* Temp */

/* Device tokens */
#define CHIPSET		10
#define CLOCKS		11
#define OPTION		12
#define VIDEORAM	13
#define BOARD		14
#define IOBASE		15
#define DACBASE		16
#define COPBASE		17
#define POSBASE		18
#define INSTANCE	19
#define RAMDAC		20
#define DACSPEED	21
#define SPEEDUP		22
#define NOSPEEDUP	23
#define CLOCKPROG	24
#define BIOSBASE	25
#define MEMBASE		26
#define CLOCKCHIP	27
#define S3MNADJUST	28
#define S3MCLK		29
#define CHIPID		30
#define CHIPREV		31
#define MEMCLOCK    32
#define CARD        33
#define VGABASEADDR    100
#define S3REFCLK       101
#define S3BLANKDELAY   102
#define TEXTCLOCKFRQ   103
#define PCI_TAG        104

/* Keyboard tokens */
#define AUTOREPEAT	30
#define SERVERNUM	31
#define XLEDS		32
#define VTINIT		33
#define LEFTALT		34
#define RIGHTALT	35
#define SCROLLLOCK	36
#define RIGHTCTL	37
#define VTSYSREQ	38
#define KPROTOCOL	39
#define XKBKEYMAP	40
#define XKBCOMPAT	41
#define XKBTYPES	42
#define XKBKEYCODES	43
#define XKBGEOMETRY	44
#define XKBSYMBOLS	45
#define XKBDISABLE	46
#define PANIX106	47
#define XKBRULES	110
#define XKBMODEL	111
#define XKBLAYOUT	112
#define XKBVARIANT	113
#define XKBOPTIONS	114

/* Pointer tokens */
#define P_MS		0			/* Microsoft */
#define P_MSC		1			/* Mouse Systems Corp */
#define P_MM		2			/* MMseries */
#define P_LOGI		3			/* Logitech */
#define P_BM		4			/* BusMouse ??? */
#define P_LOGIMAN	5			/* MouseMan / TrackMan * * * * *
								 * [CHRIS-211092] */
#define P_PS2		6			/* PS/2 mouse */
#define P_MMHIT		7			/* MM_HitTab */
#define P_GLIDEPOINT	8		/* ALPS GlidePoint */
#define P_MSINTELLIMOUSE  9		/* Microsoft IntelliMouse */

#define EMULATE3	50
#define BAUDRATE	51
#define SAMPLERATE	52
#define CLEARDTR	53
#define CLEARRTS	54
#define CHORDMIDDLE	55
#define PROTOCOL	56
#define PDEVICE		57
#define EM3TIMEOUT	58
/* This should be removed soon */
#define REPEATEDMIDDLE	59
#define DEVICE_NAME	60
#define ALWAYSCORE	61
#define BUTTONS		62

/* Display tokens */
/* OPTION is defined to 12 above */
#define MODES		70
#define VIRTUAL		71
#define VIEWPORT	72
#define VISUAL		73
#define BLACK		74
#define WHITE		75
#define DEPTH		76
#define WEIGHT		77
#define INVERTVCLK	78
#define BLANKDELAY	79
#define EARLYSC		80

/* Visual Tokens */
#define STATICGRAY	90
#define GRAYSCALE	91
#define STATICCOLOR	92
#define PSEUDOCOLOR	93
#define TRUECOLOR	94
#define DIRECTCOLOR	95

/* Layout Tokens */
#define	SCREEN		100

/* Vendor Tokens */
#define VENDORNAME	200

#endif /* _xf86_tokens_h */

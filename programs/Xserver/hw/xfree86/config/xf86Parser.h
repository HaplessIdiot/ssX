/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/xf86Parser.h,v 1.1.2.6 1997/07/22 13:50:55 dawes Exp $ */
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

/* 
 * This file contains the external interfaces for the XFree86 configuration
 * file parser.
 */
#ifndef _xf86Parser_h_
#define _xf86Parser_h_

#include "xf86Optrec.h"

typedef struct
{
	char *file_logfile;
	char *file_rgbpath;
	char *file_modulepath;
	char *file_fontpath;
}
XF86ConfFilesRec, *XF86ConfFilesPtr;

/* Values for load_type */
#define XF86_LOAD_MODULE	0
#define XF86_LOAD_DRIVER	1

typedef struct
{
	GenericListRec list;
	int load_type;
	char *load_name;
	XF86OptionPtr load_opt;
}
XF86LoadRec, *XF86LoadPtr;

typedef struct
{
	XF86LoadPtr mod_load_lst;
}
XF86ConfModuleRec, *XF86ConfModulePtr;

/* Indexes for the specialKeyMap array */
#define CONF_K_INDEX_LEFTALT		0
#define CONF_K_INDEX_RIGHTALT		1
#define CONF_K_INDEX_SCROLLLOCK		2
#define CONF_K_INDEX_RIGHTCTL		3

/* Values for the specialKeyMap array */
#define CONF_KM_META		0
#define CONF_KM_COMPOSE		1
#define CONF_KM_MODESHIFT	2
#define CONF_KM_MODELOCK	3
#define CONF_KM_SCROLLLOCK	4
#define CONF_KM_CONTROL		5

/* Device tokens */
typedef struct
{
	char *keyb_protocol;
	int keyb_kbdDelay;
	int keyb_kbdRate;
	int keyb_serverNumLock;
	long keyb_xleds;
	int keyb_specialKeyMap[4];
	char *keyb_vtinit;
	int keyb_vtSysreq;
	int keyb_xkbDisable;
	char *keyb_xkbkeymap;
	char *keyb_xkbcompat;
	char *keyb_xkbtypes;
	char *keyb_xkbkeycodes;
	char *keyb_xkbgeometry;
	char *keyb_xkbsymbols;
	char *keyb_xkbrules;
	char *keyb_xkbmodel;
	char *keyb_xkblayout;
	char *keyb_xkbvariant;
	char *keyb_xkboptions;
	int keyb_panix106;
}
XF86ConfKeyboardRec, *XF86ConfKeyboardPtr;

typedef struct
{
	char *pntr_protocol;
	char *pntr_device;
	int pntr_buttons;
	int pntr_emulate3buttons;
	int pntr_baudrate;
	int pntr_samplerate;
	int pntr_emulate3Buttons;
	int pntr_emulate3Timeout;
	int pntr_chordMiddle;
	int pntr_clearDtr;
	int pntr_clearRts;
	int pntr_alwaysCore;
}
XF86ConfPointerRec, *XF86ConfPointerPtr;

#define XF86CONF_PHSYNC    0x0001
#define XF86CONF_NHSYNC    0x0002
#define XF86CONF_PVSYNC    0x0004
#define XF86CONF_NVSYNC    0x0008
#define XF86CONF_INTERLACE 0x0010
#define XF86CONF_DBLSCAN   0x0020
#define XF86CONF_CSYNC     0x0040
#define XF86CONF_PCSYNC    0x0080
#define XF86CONF_NCSYNC    0x0100
#define XF86CONF_HSKEW     0x0200	/* hskew provided */
#define XF86CONF_CUSTOM    0x0400	/* timing numbers customized by editor */

typedef struct
{
	GenericListRec list;
	char *ml_identifier;
	int ml_clock;
	int ml_hdisplay;
	int ml_hsyncstart;
	int ml_hsyncend;
	int ml_htotal;
	int ml_vdisplay;
	int ml_vsyncstart;
	int ml_vsyncend;
	int ml_vtotal;
	int ml_flags;
	int ml_hskew;
}
XF86ConfModeLineRec, *XF86ConfModeLinePtr;

#define CONF_MAX_HSYNC 8
#define CONF_MAX_VREFRESH 8

typedef struct
{
	float hi, lo;
}
parser_range;

typedef struct
{
	int red, green, blue;
}
parser_rgb;

typedef struct
{
	GenericListRec list;
	char *mon_identifier;
	char *mon_vendor;
	char *mon_modelname;
	int mon_width;				/* in mm */
	int mon_height;				/* in mm */
	XF86ConfModeLinePtr mon_modeline_lst;
	int mon_n_hsync;
	parser_range mon_hsync[CONF_MAX_HSYNC];
	int mon_n_vrefresh;
	parser_range mon_vrefresh[CONF_MAX_VREFRESH];
	float mon_gamma_red;
	float mon_gamma_green;
	float mon_gamma_blue;
}
XF86ConfMonitorRec, *XF86ConfMonitorPtr;

#define CONF_MAXDACSPEEDS 4
#define CONF_MAXCLOCKS    128

typedef struct
{
	GenericListRec list;
	char *dev_identifier;
	char *dev_vendor;
	char *dev_board;
	char *dev_chipset;
	char *dev_card;
	char *dev_driver;
	char *dev_ramdac;
	char *dev_speedup;
	int dev_dacSpeeds[CONF_MAXDACSPEEDS];
	int dev_videoram;
	char *dev_clockprog;
	int dev_textclockvalue;
	int dev_bios_base;
	int dev_mem_base;
	int dev_io_base;
	int dev_dac_base;
	int dev_cop_base;
	int dev_pos_base;
	int dev_instance;
        unsigned long dev_pci_tag;
	char *dev_clockchip;
	int dev_clocks;
	int dev_clock[CONF_MAXCLOCKS];
	XF86OptionPtr dev_option_lst;
}
XF86ConfDeviceRec, *XF86ConfDevicePtr;

typedef struct
{
	GenericListRec list;
	char *mode_name;
}
XF86ModeRec, *XF86ModePtr;

typedef struct
{
	GenericListRec list;
	int disp_frameX0;
	int disp_frameY0;
	int disp_virtualX;
	int disp_virtualY;
	int disp_depth;
	char *disp_visual;
	parser_rgb disp_weight;
	XF86ModePtr disp_mode_lst;
}
XF86ConfDisplayRec, *XF86ConfDisplayPtr;

typedef struct
{
	XF86OptionPtr flg_option_lst;
}
XF86ConfFlagsRec, *XF86ConfFlagsPtr;

typedef struct
{
	GenericListRec list;
	char *scrn_identifier;
	char *scrn_obso_driver;
	int scrn_defaultcolordepth;
	int scrn_blanktime;
	int scrn_standbytime;
	int scrn_suspendtime;
	int scrn_offtime;
	char *scrn_monitor_str;
	XF86ConfMonitorPtr scrn_monitor;
	char *scrn_device_str;
	XF86ConfDevicePtr scrn_device;
	XF86ConfDisplayPtr scrn_display_lst;
}
XF86ConfScreenRec, *XF86ConfScreenPtr;

typedef struct
{
	GenericListRec list;
	XF86ConfScreenPtr adj_screen;
	char *adj_screen_str;
	XF86ConfScreenPtr adj_top;
	char *adj_top_str;
	XF86ConfScreenPtr adj_bottom;
	char *adj_bottom_str;
	XF86ConfScreenPtr adj_left;
	char *adj_left_str;
	XF86ConfScreenPtr adj_right;
	char *adj_right_str;
}
XF86ConfAdjacencyRec, *XF86ConfAdjacencyPtr;

typedef struct
{
	GenericListRec list;
	char *lay_identifier;
	XF86ConfAdjacencyPtr lay_adjacency_lst;
}
XF86ConfLayoutRec, *XF86ConfLayoutPtr;

typedef struct
{
	GenericListRec list;
	char *vnd_identifier;
	XF86OptionPtr vnd_option_lst;
}
XF86ConfVendorRec, *XF86ConfVendorPtr;

typedef struct
{
	XF86ConfFilesPtr conf_files;
	XF86ConfModulePtr conf_modules;
	XF86ConfFlagsPtr conf_flags;
	XF86ConfKeyboardPtr conf_keyboard;
	XF86ConfPointerPtr conf_pointer;
	XF86ConfMonitorPtr conf_monitor_lst;
	XF86ConfDevicePtr conf_device_lst;
	XF86ConfScreenPtr conf_screen_lst;
	XF86ConfLayoutPtr conf_layout_lst;
	XF86ConfVendorPtr conf_vendor_lst;
}
XF86ConfigRec, *XF86ConfigPtr;

typedef struct
{
	int token;			/* id of the token */
	char *name;			/* pointer to the LOWERCASED name */
}
xf86ConfigSymTabRec, *xf86ConfigSymTabPtr;

/*
 * prototypes
 */
extern int xf86OpenConfigFile (char *);

extern XF86ConfigPtr xf86ReadConfigFile (void);

extern void xf86CloseConfigFile (void);

void XF86FreeConfig (XF86ConfigPtr p);

extern int xf86WriteConfigFile (const char *, XF86ConfigPtr);

XF86ConfDevicePtr xf86FindDevice(char *ident, XF86ConfDevicePtr p);
XF86ConfLayoutPtr xf86FindLayout(XF86ConfLayoutPtr list, char *name);
XF86ConfMonitorPtr xf86FindMonitor(char *ident, XF86ConfMonitorPtr p);
XF86ConfModeLinePtr xf86FindModeLine(char *ident, XF86ConfModeLinePtr p);
XF86ConfScreenPtr xf86FindScreen(char *ident, XF86ConfScreenPtr p);
XF86ConfDisplayPtr xf86FindDisplay(int depth, XF86ConfDisplayPtr p);
XF86ConfVendorPtr xf86FindVendor(XF86ConfVendorPtr list, char *name);

#endif /* _xf86Parser_h_ */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_driver.h,v 1.0 2001/11/30 12:12:00 eich Exp $ */
/*
 * Global data and definitions
 *
 * Copyright 2002, 2003 by Thomas Winischhofer, Vienna, Austria
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holder not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holder makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:
 *      	Thomas Winischhofer <thomas@winischhofer.net>
 *
 */

/* Mode numbers for 300/315/330 series */
const UShort  ModeIndex_320x200[]      = {0x59, 0x41, 0x00, 0x4f};
const UShort  ModeIndex_320x240[]      = {0x50, 0x56, 0x00, 0x53};
const UShort  ModeIndex_320x240_FSTN[] = {0x5a, 0x5b, 0x00, 0x00};  /* FSTN */
const UShort  ModeIndex_400x300[]      = {0x51, 0x57, 0x00, 0x54};
const UShort  ModeIndex_512x384[]      = {0x52, 0x58, 0x00, 0x5c};
const UShort  ModeIndex_640x400[]      = {0x2f, 0x5d, 0x00, 0x5e};
const UShort  ModeIndex_640x480[]      = {0x2e, 0x44, 0x00, 0x62};
const UShort  ModeIndex_720x480[]      = {0x31, 0x33, 0x00, 0x35};
const UShort  ModeIndex_720x576[]      = {0x32, 0x34, 0x00, 0x36};
const UShort  ModeIndex_768x576[]      = {0x5f, 0x60, 0x00, 0x61};
const UShort  ModeIndex_800x480[]      = {0x70, 0x7a, 0x00, 0x76};
const UShort  ModeIndex_800x600[]      = {0x30, 0x47, 0x00, 0x63};
const UShort  ModeIndex_848x480[]      = {0x39, 0x3b, 0x00, 0x3e};
const UShort  ModeIndex_856x480[]      = {0x3f, 0x42, 0x00, 0x45};
const UShort  ModeIndex_1024x768[]     = {0x38, 0x4a, 0x00, 0x64};
const UShort  ModeIndex_1024x576[]     = {0x71, 0x74, 0x00, 0x77};
const UShort  ModeIndex_1024x600[]     = {0x20, 0x21, 0x00, 0x22};  /* 300 series only */
const UShort  ModeIndex_1280x1024[]    = {0x3a, 0x4d, 0x00, 0x65};
const UShort  ModeIndex_1280x960[]     = {0x7c, 0x7d, 0x00, 0x7e};
const UShort  ModeIndex_1152x768[]     = {0x23, 0x24, 0x00, 0x25};  /* 300 series only */
const UShort  ModeIndex_1152x864[]     = {0x29, 0x2a, 0x00, 0x2b};
const UShort  ModeIndex_300_1280x768[] = {0x55, 0x5a, 0x00, 0x5b};
const UShort  ModeIndex_310_1280x768[] = {0x23, 0x24, 0x00, 0x25};
const UShort  ModeIndex_1280x720[]     = {0x79, 0x75, 0x00, 0x78};
const UShort  ModeIndex_1360x768[]     = {0x48, 0x4b, 0x00, 0x4e};
const UShort  ModeIndex_1400x1050[]    = {0x26, 0x27, 0x00, 0x28};  /* 315 series only */
const UShort  ModeIndex_1600x1200[]    = {0x3c, 0x3d, 0x00, 0x66};
const UShort  ModeIndex_1920x1440[]    = {0x68, 0x69, 0x00, 0x6b};
const UShort  ModeIndex_300_2048x1536[]= {0x6c, 0x6d, 0x00, 0x00};
const UShort  ModeIndex_310_2048x1536[]= {0x6c, 0x6d, 0x00, 0x6e};
 
/* VESA */
/*     The following is included because there are BIOSes out there that
 *     report incomplete mode lists. These are 630 BIOS versions <2.01.2x
 *
 *     -) VBE 3.0 on SiS300 and 315 series do not support 24 fpp modes
 *     -) Only SiS315 series support 1920x1440x32
 */
				             /*     8      16    (24)    32   */
static const UShort  VESAModeIndex_320x200[]   = {0x138, 0x10e, 0x000, 0x000};
static const UShort  VESAModeIndex_320x240[]   = {0x132, 0x135, 0x000, 0x000};
static const UShort  VESAModeIndex_400x300[]   = {0x133, 0x136, 0x000, 0x000};
static const UShort  VESAModeIndex_512x384[]   = {0x134, 0x137, 0x000, 0x000};
static const UShort  VESAModeIndex_640x400[]   = {0x100, 0x139, 0x000, 0x000};
static const UShort  VESAModeIndex_640x480[]   = {0x101, 0x111, 0x000, 0x13a};
static const UShort  VESAModeIndex_800x600[]   = {0x103, 0x114, 0x000, 0x13b};
static const UShort  VESAModeIndex_1024x768[]  = {0x105, 0x117, 0x000, 0x13c};
static const UShort  VESAModeIndex_1280x1024[] = {0x107, 0x11a, 0x000, 0x13d};
static const UShort  VESAModeIndex_1600x1200[] = {0x130, 0x131, 0x000, 0x13e};
static const UShort  VESAModeIndex_1920x1440[] = {0x13f, 0x140, 0x000, 0x141};

/* For calculating refresh rate index (CR33) */
static const struct _sis_vrate {
    CARD16 idx;
    CARD16 xres;
    CARD16 yres;
    CARD16 refresh;
    BOOLEAN SiS730valid32bpp;
} sisx_vrate[] = {
	{1,  320,  200,  70,  TRUE},
	{1,  320,  240,  60,  TRUE},
	{1,  400,  300,  60,  TRUE},
        {1,  512,  384,  60,  TRUE},
	{1,  640,  400,  72,  TRUE},
	{1,  640,  480,  60,  TRUE}, {2,  640,  480,  72,  TRUE}, {3,  640,  480,  75,  TRUE}, 
	{4,  640,  480,  85,  TRUE}, {5,  640,  480, 100,  TRUE}, {6,  640,  480, 120,  TRUE}, 
	{7,  640,  480, 160,  TRUE}, {8,  640,  480, 200,  TRUE},
	{1,  720,  480,  60,  TRUE},
	{1,  720,  576,  58,  TRUE},
	{1,  800,  480,  60,  TRUE}, {2,  800,  480,  75,  TRUE}, {3,  800,  480,  85,  TRUE},
	{1,  800,  600,  56,  TRUE}, {2,  800,  600,  60,  TRUE}, {3,  800,  600,  72,  TRUE}, 
	{4,  800,  600,  75,  TRUE}, {5,  800,  600,  85,  TRUE}, {6,  800,  600, 105,  TRUE}, 
	{7,  800,  600, 120,  TRUE}, {8,  800,  600, 160,  TRUE},
	{1,  848,  480,  39,  TRUE}, {2,  848,  480,  60,  TRUE},
	{1,  856,  480,  39,  TRUE}, {2,  856,  480,  60,  TRUE},
	{1, 1024,  576,  60,  TRUE}, {2, 1024,  576,  75,  TRUE}, {3, 1024,  576,  85,  TRUE},
	{1, 1024,  600,  60,  TRUE},
	{1, 1024,  768,  43,  TRUE}, {2, 1024,  768,  60,  TRUE}, {3, 1024,  768,  70, FALSE},
	{4, 1024,  768,  75, FALSE}, {5, 1024,  768,  85,  TRUE}, {6, 1024,  768, 100,  TRUE},
	{7, 1024,  768, 120,  TRUE},
	{1, 1152,  768,  60,  TRUE},
	{1, 1152,  864,  75,  TRUE}, {2, 1152,  864,  84,  TRUE},
	{1, 1280,  720,  60,  TRUE}, {2, 1280,  720,  75,  TRUE}, {3, 1280,  720,  85,  TRUE},
	{1, 1280,  768,  60,  TRUE},
	{1, 1280,  960,  60,  TRUE}, {2, 1280,  960,  85,  TRUE},
	{1, 1280, 1024,  43,  TRUE}, {2, 1280, 1024,  60,  TRUE}, {3, 1280, 1024,  75,  TRUE},
	{4, 1280, 1024,  85,  TRUE},
	{1, 1360,  768,  60,  TRUE},
	{1, 1400, 1050,  60,  TRUE}, {2, 1400, 1050,  75,  TRUE},
	{1, 1600, 1200,  60,  TRUE}, {2, 1600, 1200,  65,  TRUE}, {3, 1600, 1200,  70,  TRUE}, 
	{4, 1600, 1200,  75,  TRUE}, {5, 1600, 1200,  85,  TRUE}, {6, 1600, 1200, 100,  TRUE}, 
	{7, 1600, 1200, 120,  TRUE},
	{1, 1920, 1440,  60,  TRUE}, {2, 1920, 1440,  65,  TRUE}, {3, 1920, 1440,  70,  TRUE}, 
	{4, 1920, 1440,  75,  TRUE}, {5, 1920, 1440,  85,  TRUE}, {6, 1920, 1440, 100,  TRUE},
	{1, 2048, 1536,  60,  TRUE}, {2, 2048, 1536,  65,  TRUE}, {3, 2048, 1536,  70,  TRUE}, 
	{4, 2048, 1536,  75,  TRUE}, {5, 2048, 1536,  85,  TRUE},
	{0,    0,    0,   0, FALSE}
};

/*     Some 300-series laptops have a badly designed BIOS and make it
 *     impossible to detect the correct panel delay compensation. This
 *     table used to detect such machines by their PCI subsystem IDs;
 *     however, I don't know how reliable this method is. (With Asus
 *     machines, it is to general, ASUS uses the same ID for different
 *     boxes)
 */
static const pdctable mypdctable[] = {
        { 0x1071, 0x7522, 32, "Mitac", "7521T" },
	{ 0,      0,       0, ""     , ""      }
};

static const chswtable mychswtable[] = {
        { 0x1631, 0x1002, "Mitachi", "0x1002" },
	{ 0,      0,      ""       , ""       }
};

/*     Our TV modes for the 6326. The data in these structures
 *     is mainly correct, but since we use our private CR and
 *     clock values anyway, small errors do no matter.
 */
static DisplayModeRec SiS6326PAL800x600Mode = {
	NULL, NULL,     /* prev, next */
	"PAL800x600",   /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	36000,		/* Clock frequency */
	800,		/* HDisplay */
	848,		/* HSyncStart */
	912,		/* HSyncEnd */
	1008,		/* HTotal */
	0,		/* HSkew */
	600,		/* VDisplay */
	600,		/* VSyncStart */
	602,		/* VSyncEnd */
	625,		/* VTotal */
	0,		/* VScan */
	V_PHSYNC | V_PVSYNC,	/* Flags */
	-1,		/* ClockIndex */
	36000,		/* SynthClock */
	800,		/* CRTC HDisplay */
	808,            /* CRTC HBlankStart */
	848,            /* CRTC HSyncStart */
	912,            /* CRTC HSyncEnd */
	1008,           /* CRTC HBlankEnd */
	1008,           /* CRTC HTotal */
	0,              /* CRTC HSkew */
	600,		/* CRTC VDisplay */
	600,		/* CRTC VBlankStart */
	600,		/* CRTC VSyncStart */
	602,		/* CRTC VSyncEnd */
	625,		/* CRTC VBlankEnd */
	625,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};

/*     Due to the scaling method this mode uses, the vertical data here
 *     does not match the CR data. But this does not matter, we use our
 *     private CR data anyway.
 */
static DisplayModeRec SiS6326PAL800x600UMode = {
	NULL,           /* prev */
	&SiS6326PAL800x600Mode, /* next */
	"PAL800x600U",  /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	37120,		/* Clock frequency */
	800,		/* HDisplay */
	872,		/* HSyncStart */
	984,		/* HSyncEnd */
	1088,		/* HTotal */
	0,		/* HSkew */
	600,		/* VDisplay (548 due to scaling) */
	600,		/* VSyncStart (584) */
	602,		/* VSyncEnd (586) */
	625,		/* VTotal */
	0,		/* VScan */
	V_PHSYNC | V_PVSYNC,	/* Flags */
	-1,		/* ClockIndex */
	37120,		/* SynthClock */
	800,		/* CRTC HDisplay */
	808,            /* CRTC HBlankStart */
	872,            /* CRTC HSyncStart */
	984,            /* CRTC HSyncEnd */
	1024,           /* CRTC HBlankEnd */
	1088,           /* CRTC HTotal */
	0,              /* CRTC HSkew */
	600,		/* CRTC VDisplay (548 due to scaling) */
	600,		/* CRTC VBlankStart (600) */
	600,		/* CRTC VSyncStart (584) */
	602,		/* CRTC VSyncEnd (586) */
	625,		/* CRTC VBlankEnd */
	625,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};

static DisplayModeRec SiS6326PAL720x540Mode = {
	NULL,      	/* prev */
	&SiS6326PAL800x600UMode, /* next */
	"PAL720x540",   /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	36000,		/* Clock frequency */
	720,		/* HDisplay */
	816,		/* HSyncStart */
	920,		/* HSyncEnd */
	1008,		/* HTotal */
	0,		/* HSkew */
	540,		/* VDisplay */
	578,		/* VSyncStart */
	580,		/* VSyncEnd */
	625,		/* VTotal */
	0,		/* VScan */
	V_PHSYNC | V_PVSYNC,	/* Flags */
	-1,		/* ClockIndex */
	36000,		/* SynthClock */
	720,		/* CRTC HDisplay */
	736,            /* CRTC HBlankStart */
	816,            /* CRTC HSyncStart */
	920,            /* CRTC HSyncEnd */
	1008,           /* CRTC HBlankEnd */
	1008,           /* CRTC HTotal */
	0,              /* CRTC HSkew */
	540,		/* CRTC VDisplay */
	577,		/* CRTC VBlankStart */
	578,		/* CRTC VSyncStart */
	580,		/* CRTC VSyncEnd */
	625,		/* CRTC VBlankEnd */
	625,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};

static DisplayModeRec SiS6326PAL640x480Mode = {
	NULL,      	/* prev */
	&SiS6326PAL720x540Mode, /* next */
	"PAL640x480",   /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	36000,		/* Clock frequency */
	640,		/* HDisplay */
	768,		/* HSyncStart */
	920,		/* HSyncEnd */
	1008,		/* HTotal */
	0,		/* HSkew */
	480,		/* VDisplay */
	532,		/* VSyncStart */
	534,		/* VSyncEnd */
	625,		/* VTotal */
	0,		/* VScan */
	V_NHSYNC | V_NVSYNC,	/* Flags */
	-1,		/* ClockIndex */
	36000,		/* SynthClock */
	640,		/* CRTC HDisplay */
	648,            /* CRTC HBlankStart */
	768,            /* CRTC HSyncStart */
	920,            /* CRTC HSyncEnd */
	944,            /* CRTC HBlankEnd */
	1008,           /* CRTC HTotal */
	0,              /* CRTC HSkew */
	480,		/* CRTC VDisplay */
	481,		/* CRTC VBlankStart */
	532,		/* CRTC VSyncStart */
	534,		/* CRTC VSyncEnd */
	561,		/* CRTC VBlankEnd */
	625,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};

static DisplayModeRec SiS6326NTSC640x480Mode = {
	NULL, NULL,	/* prev, next */
	"NTSC640x480",  /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	27000,		/* Clock frequency */
	640,		/* HDisplay */
	664,		/* HSyncStart */
	760,		/* HSyncEnd */
	800,		/* HTotal */
	0,		/* HSkew */
	480,		/* VDisplay */
	489,		/* VSyncStart */
	491,		/* VSyncEnd */
	525,		/* VTotal */
	0,		/* VScan */
	V_NHSYNC | V_NVSYNC,	/* Flags */
	-1,		/* ClockIndex */
	27000,		/* SynthClock */
	640,		/* CRTC HDisplay */
	648,            /* CRTC HBlankStart */
	664,            /* CRTC HSyncStart */
	760,            /* CRTC HSyncEnd */
	792,            /* CRTC HBlankEnd */
	800,            /* CRTC HTotal */
	0,              /* CRTC HSkew */
	480,		/* CRTC VDisplay */
	488,		/* CRTC VBlankStart */
	489,		/* CRTC VSyncStart */
	491,		/* CRTC VSyncEnd */
	517,		/* CRTC VBlankEnd */
	525,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};

/*     Due to the scaling method this mode uses, the vertical data here
 *     does not match the CR data. But this does not matter, we use our
 *     private CR data anyway.
 */
static DisplayModeRec SiS6326NTSC640x480UMode = {
	NULL, 		/* prev */
	&SiS6326NTSC640x480Mode, /* next */
	"NTSC640x480U", /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	32215,		/* Clock frequency */
	640,		/* HDisplay */
	696,		/* HSyncStart */
	840,		/* HSyncEnd */
	856,		/* HTotal */
	0,		/* HSkew */
	480,		/* VDisplay (439 due to scaling) */
	489,		/* VSyncStart (473) */
	491,		/* VSyncEnd (475) */
	525,		/* VTotal */
	0,		/* VScan */
	V_NHSYNC | V_NVSYNC,	/* Flags */
	-1,		/* ClockIndex */
	32215,		/* SynthClock */
	640,		/* CRTC HDisplay */
	656,            /* CRTC HBlankStart */
	696,            /* CRTC HSyncStart */
	840,            /* CRTC HSyncEnd */
	856,            /* CRTC HBlankEnd */
	856,            /* CRTC HTotal */
	0,              /* CRTC HSkew */
	480,		/* CRTC VDisplay */
	488,		/* CRTC VBlankStart */
	489,		/* CRTC VSyncStart */
	491,		/* CRTC VSyncEnd */
	517,		/* CRTC VBlankEnd */
	525,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};


static DisplayModeRec SiS6326NTSC640x400Mode = {
	NULL, 	     	/* prev */
	&SiS6326NTSC640x480UMode, /* next */
	"NTSC640x400",  /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	27000,		/* Clock frequency */
	640,		/* HDisplay */
	664,		/* HSyncStart */
	760,		/* HSyncEnd */
	800,		/* HTotal */
	0,		/* HSkew */
	400,		/* VDisplay */
	459,		/* VSyncStart */
	461,		/* VSyncEnd */
	525,		/* VTotal */
	0,		/* VScan */
	V_NHSYNC | V_NVSYNC,	/* Flags */
	-1,		/* ClockIndex */
	27000,		/* SynthClock */
	640,		/* CRTC HDisplay */
	648,            /* CRTC HBlankStart */
	664,            /* CRTC HSyncStart */
	760,            /* CRTC HSyncEnd */
	792,            /* CRTC HBlankEnd */
	800,            /* CRTC HTotal */
	0,              /* CRTC HSkew */
	400,		/* CRTC VDisplay */
	407,		/* CRTC VBlankStart */
	459,		/* CRTC VSyncStart */
	461,		/* CRTC VSyncEnd */
	490,		/* CRTC VBlankEnd */
	525,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};

/*     Built-in hi-res modes for the 6326.
 *     For some reason, our default mode lines and the
 *     clock calculation functions in sis_dac.c do no
 *     good job on higher clocks. It seems, the hardware
 *     needs some tricks so make mode with higher clock
 *     rates than ca. 120MHz work. I didn't bother trying
 *     to find out what exactly is going wrong, so I
 *     implemented two special modes instead for 1280x1024
 *     and 1600x1200. These two are automatically added
 *     to the list if they are supported with the current
 *     depth.
 *     The data in the strucures below is a proximation,
 *     in sis_vga.c the register contents are fetched from
 *     fixed tables anyway.
 */
static DisplayModeRec SiS6326SIS1280x1024_75Mode = {
	NULL, 	       	/* prev */
	NULL,           /* next */
	"SIS1280x1024-75",  /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	135000,		/* Clock frequency */
	1280,		/* HDisplay */
	1296,		/* HSyncStart */
	1440,		/* HSyncEnd */
	1688,		/* HTotal */
	0,		/* HSkew */
	1024,		/* VDisplay */
	1025,		/* VSyncStart */
	1028,		/* VSyncEnd */
	1066,		/* VTotal */
	0,		/* VScan */
	V_PHSYNC | V_PVSYNC,	/* Flags */
	-1,		/* ClockIndex */
	135000,		/* SynthClock */
	1280,		/* CRTC HDisplay */
	1280,           /* CRTC HBlankStart */
	1296,           /* CRTC HSyncStart */
	1440,           /* CRTC HSyncEnd */
	1680,           /* CRTC HBlankEnd */
	1688,           /* CRTC HTotal */
	0,              /* CRTC HSkew */
	1024,		/* CRTC VDisplay */
	1024,		/* CRTC VBlankStart */
	1025,		/* CRTC VSyncStart */
	1028,		/* CRTC VSyncEnd */
	1065,		/* CRTC VBlankEnd */
	1066,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};

static DisplayModeRec SiS6326SIS1600x1200_60Mode = {
	NULL, 	       	/* prev */
	NULL,           /* next */
	"SIS1600x1200-60",  /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	162000,		/* Clock frequency */
	1600,		/* HDisplay */
	1664,		/* HSyncStart */
	1856,		/* HSyncEnd */
	2160,		/* HTotal */
	0,		/* HSkew */
	1200,		/* VDisplay */
	1201,		/* VSyncStart */
	1204,		/* VSyncEnd */
	1250,		/* VTotal */
	0,		/* VScan */
	V_PHSYNC | V_PVSYNC,	/* Flags */
	-1,		/* ClockIndex */
	162000,		/* SynthClock */
	1600,		/* CRTC HDisplay */
	1600,           /* CRTC HBlankStart */
	1664,           /* CRTC HSyncStart */
	1856,           /* CRTC HSyncEnd */
	2152,            /* CRTC HBlankEnd */
	2160,            /* CRTC HTotal */
	0,              /* CRTC HSkew */
	1200,		/* CRTC VDisplay */
	1200,		/* CRTC VBlankStart */
	1201,		/* CRTC VSyncStart */
	1204,		/* CRTC VSyncEnd */
	1249,		/* CRTC VBlankEnd */
	1250,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};

USHORT 	      SiS_CalcModeIndex(ScrnInfoPtr pScrn, DisplayModePtr mode, BOOLEAN hcm);
USHORT        SiS_CheckCalcModeIndex(ScrnInfoPtr pScrn, DisplayModePtr mode, unsigned long VBFlags, BOOLEAN hcm);
unsigned char SiS_GetSetBIOSScratch(ScrnInfoPtr pScrn, USHORT offset, unsigned char value);

void          SISMergePointerMoved(int scrnIndex, int x, int y);



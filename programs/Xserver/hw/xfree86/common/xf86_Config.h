/* $XConsortium: xf86_Config.h,v 1.1 94/03/28 21:23:53 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86_Config.h,v 3.9 1994/09/13 15:09:38 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Thomas Roell and David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Thomas Roell and
 * David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THOMAS ROELL AND DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL THOMAS ROELL OR DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#ifndef XCONFIG_FLAGS_ONLY

#ifdef BLACK
#undef BLACK
#endif
#ifdef WHITE
#undef WHITE
#endif
#ifdef SCROLLLOCK
#undef SCROLLLOCK
#endif
#ifdef MONO
#undef MONO
#endif

typedef struct {
  int           num;                  /* returned number */
  char          *str;                 /* private copy of the return-string */
  double        realnum;              /* returned number as a real */
} LexRec, *LexPtr;

#ifndef OLD_XCONFIG

/* Monitor and device records. */
#define MAX_HSYNC 8
#define MAX_VREFRESH 8

typedef struct { float hi, lo; } range ;

typedef struct {
   char *id;
   char *vendor;
   char *model;
   float bandwidth;
   int n_hsync;
   range hsync[MAX_HSYNC];
   int n_vrefresh;
   range vrefresh[MAX_VREFRESH];
   DisplayModePtr Modes, Last; /* Start and end of monitor's mode list */
} MonRec, *MonPtr ;

typedef struct {
   char *identifier;
   char *vendor;
   char *board;
   char *chipset;
   char *ramdac;
   int dacSpeed;
   int clocks;
   int clock[MAXCLOCKS];
   OFlagSet options;
   OFlagSet clockOptions;
   OFlagSet xconfigFlag;
   int videoRam;
   unsigned long speedup;
   char *clockprog;
   int textClockValue;
   int BIOSbase;                 /* Base address of video BIOS */
   unsigned long MemBase;                  /* Frame buffer base address */
   int IObase;
   int DACbase;
   int COPbase;
   int POSbase;
   int instance;
} GDevRec, *GDevPtr;

typedef struct {
   int depth;
   xrgb weight;
   int frameX0;
   int frameY0;
   int virtualX;
   int virtualY;
   DisplayModePtr modes;
   xrgb whiteColour;
   xrgb blackColour;
   int defaultVisual;
   OFlagSet options;
   OFlagSet xconfigFlag;
} DispRec, *DispPtr;

/*
 * We use the convention that tokens >= 1000 or < 0 do not need to be
 * present in a screen's list of valid tokens in order to be valid.
 * Also, each token should have a unique value regardless of the section
 * it is used in.
 */

#define LOCK_TOKEN  -3
#define ERROR_TOKEN -2
#define NUMBER		10000                  
#define STRING		10001

/* GJA -- gave those high numbers since they occur in many sections. */
#define SECTION		10002
#define SUBSECTION	10003  /* Only used at one place now. */
#define ENDSECTION	10004
#define ENDSUBSECTION	10005
#define IDENTIFIER	10006
#define VENDOR		10007
#define DASH		10008
#define COMMA		10009

#ifdef INIT_CONFIG
static SymTabRec TopLevelTab[] = {
  { SECTION,   "section" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */

#define HRZ	1001	/* Silly name to avoid conflict with linux/param.h */
#define KHZ	1002
#define MHZ	1003

#ifdef INIT_CONFIG
static SymTabRec UnitTab[] = {
  { HRZ,	"hz" },
  { KHZ,	"khz" },
  { MHZ,	"mhz" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */

#define SVGA	1010
#define VGA2	1011
#define MONO	1012
#define VGA16	1013
#define ACCEL	1014

#ifdef INIT_CONFIG
static SymTabRec DriverTab[] = {
  { SVGA,	"svga" },
  { SVGA,	"vga256" },
  { VGA2,	"vga2" },
  { MONO,	"mono" },
  { VGA16,	"vga16" },
  { ACCEL,	"accel" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */

#define MICROSOFT	1020
#define MOUSESYS	1021
#define MMSERIES	1022
#define LOGITECH	1023
#define BUSMOUSE	1024
#define LOGIMAN		1025
#define PS_2		1026
#define MMHITTAB	1027
#define XQUE      	1030
#define OSMOUSE   	1031

#ifdef INIT_CONFIG
static SymTabRec MouseTab[] = {
  { MICROSOFT,	"microsoft" },
  { MOUSESYS,	"mousesystems" },
  { MMSERIES,	"mmseries" },
  { LOGITECH,	"logitech" },
  { BUSMOUSE,	"busmouse" },
  { LOGIMAN,	"mouseman" },
  { PS_2,	"ps/2" },
  { MMHITTAB,	"mmhittab" },
  { XQUE,	"xqueue" },
  { OSMOUSE,	"osmouse" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */

#define FONTPATH	1040
#define RGBPATH		1041

#ifdef INIT_CONFIG
static SymTabRec FilesTab[] = {
  { ENDSECTION,	"endsection"},
  { FONTPATH,	"fontpath" },
  { RGBPATH,	"rgbpath" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */

#define NOTRAPSIGNALS	1050
#define DONTZAP		1051

#ifdef INIT_CONFIG
static SymTabRec ServerFlagsTab[] = {
  { ENDSECTION, "endsection"},
  { NOTRAPSIGNALS, "notrapsignals" },
  { DONTZAP,	"dontzap" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */

#define DISPLAYSIZE	1060
#define MODELINE	1061
#define MODEL		1062
#define BANDWIDTH	1063
#define HORIZSYNC	1064
#define VERTREFRESH	1065
#define MODE		1066
#define GAMMA		1067

#ifdef INIT_CONFIG
static SymTabRec MonitorTab[] = {
  { ENDSECTION,	"endsection"},
  { IDENTIFIER,	"identifier"}, 
  { VENDOR,	"vendorname"},
  { MODEL,	"modelname"}, 
  { MODELINE,	"modeline"},
  { DISPLAYSIZE,"displaysize" },
  { BANDWIDTH,	"bandwidth" },
  { HORIZSYNC,	"horizsync" },
  { VERTREFRESH,"vertrefresh" },
  { MODE,	"mode" },
  { GAMMA,	"gamma" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */

#define DOTCLOCK	1070
#define HTIMINGS	1071
#define VTIMINGS	1072
#define FLAGS		1073
#define ENDMODE		1074

#ifdef INIT_CONFIG
static SymTabRec ModeTab[] = {
  { DOTCLOCK,	"dotclock"},
  { HTIMINGS,	"htimings"}, 
  { VTIMINGS,	"vtimings"},
  { FLAGS,	"flags"}, 
  { ENDMODE,	"endmode"},
  { -1,		"" },
};
#endif /* INIT_CONFIG */

#define DRIVER		1080
#define MDEVICE		1081
#define MONITOR		1082
#define SCREENNO	1083

#ifdef INIT_CONFIG
static SymTabRec ScreenTab[] = {
  { ENDSECTION, "endsection"},
  { DRIVER,	"driver" },
  { MDEVICE,	"device" },
  { MONITOR,	"monitor" },
  { SCREENNO,	"screenno" },
  { SUBSECTION,	"subsection" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */

/* Mode timing keywords */
#define INTERLACE	1090
#define PHSYNC		1091
#define NHSYNC		1092
#define PVSYNC		1093
#define NVSYNC		1094
#define CSYNC		1095

#ifdef INIT_CONFIG
static SymTabRec TimingTab[] = {
  { INTERLACE,	"interlace"},
  { PHSYNC,	"+hsync"},
  { NHSYNC,	"-hsync"},
  { PVSYNC,	"+vsync"},
  { NVSYNC,	"-vsync"},
  { CSYNC,	"composite"},
  { -1,		"" },
};
#endif /* INIT_CONFIG */

/* Indexes for the specialKeyMap array */
#define K_INDEX_LEFTALT		0
#define K_INDEX_RIGHTALT	1
#define K_INDEX_SCROLLLOCK	2
#define K_INDEX_RIGHTCTL	3

/* Values for the specialKeyMap array */
#define K_META		0
#define K_COMPOSE	1
#define K_MODESHIFT	2
#define K_MODELOCK	3
#define K_SCROLLLOCK	4
#define K_CONTROL	5

#ifdef INIT_CONFIG
static SymTabRec KeyMapTab[] = {
  { K_META,		"meta" },
  { K_COMPOSE,		"compose" },
  { K_MODESHIFT,	"modeshift" },
  { K_MODELOCK,		"modelock" },
  { K_SCROLLLOCK,	"scrolllock" },
  { K_CONTROL,		"control" },
  { -1,			"" },
};
#endif /* INIT_CONFIG */

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

#ifdef INIT_CONFIG
static SymTabRec DeviceTab[] = {
  { ENDSECTION, "endsection"},
  { IDENTIFIER, "identifier"},
  { VENDOR, 	"vendorname"},
  { BOARD, 	"boardname"},
  { CHIPSET,	"chipset" },
  { RAMDAC,	"ramdac" },
  { DACSPEED,	"dacspeed"},
  { CLOCKS,	"clocks" },
  { OPTION,	"option" },
  { VIDEORAM,	"videoram" },
  { SPEEDUP,	"speedup" },
  { NOSPEEDUP,	"nospeedup" },
  { CLOCKPROG,	"clockprog" },
  { BIOSBASE,	"biosbase" },
  { MEMBASE,	"membase" },
  { IOBASE,	"iobase" },
  { DACBASE,	"dacbase" },
  { COPBASE,	"copbase" },
  { POSBASE,	"posbase" },
  { INSTANCE,	"instance" },
  { CLOCKCHIP,	"clockchip" },
  { S3MNADJUST,	"s3mnadjust" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */

/* Keyboard keywords */
#define AUTOREPEAT	30
#define SERVERNUM	31
#define XLEDS		32
#define VTINIT		33
#define LEFTALT		34
#define RIGHTALT	35
#define SCROLLLOCK	36
#define RIGHTCTL	37
#define VTSYSREQ	38
#define DEVICE		39
#define KPROTOCOL	39

#ifdef INIT_CONFIG
static SymTabRec KeyboardTab[] = {
  { ENDSECTION,	"endsection"},
  { DEVICE,	"device" },
  { KPROTOCOL,	"protocol" },
  { AUTOREPEAT,	"autorepeat" },
  { SERVERNUM,	"servernumlock" },
  { XLEDS,	"xleds" },
  { VTINIT,	"vtinit" },
  { LEFTALT,	"leftalt" },
  { RIGHTALT,	"rightalt" },
  { RIGHTALT,	"altgr" },
  { SCROLLLOCK,	"scrolllock" },
  { RIGHTCTL,	"rightctl" },
  { VTSYSREQ,	"vtsysreq" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */

#define P_MS		0			/* Microsoft */
#define P_MSC		1			/* Mouse Systems Corp */
#define P_MM		2			/* MMseries */
#define P_LOGI		3			/* Logitech */
#define P_BM		4			/* BusMouse ??? */
#define P_LOGIMAN	5			/* MouseMan / TrackMan
						   [CHRIS-211092] */
#define P_PS2		6			/* PS/2 mouse */
#define P_MMHIT		7			/* MM_HitTab */

#define EMULATE3	50
#define BAUDRATE	51
#define SAMPLERATE	52
#define CLEARDTR	53
#define CLEARRTS	54
#define CHORDMIDDLE	55
#define PROTOCOL	56
#define PDEVICE		57

#ifdef INIT_CONFIG
static SymTabRec PointerTab[] = {
  { PDEVICE,	"device"},
  { ENDSECTION,	"endsection"},
  { PROTOCOL,	"protocol" },
  { BAUDRATE,	"baudrate" },
  { EMULATE3,	"emulate3buttons" },
  { SAMPLERATE,	"samplerate" },
  { CLEARDTR,	"cleardtr" },
  { CLEARRTS,	"clearrts" },
  { CHORDMIDDLE,"chordmiddle" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */

/* OPTION is defined to 12 above */
#define MODES		70
#define VIRTUAL		71
#define VIEWPORT	72
#define VISUAL		73
#define BLACK		74
#define WHITE		75
#define DEPTH		76
#define WEIGHT		77

#ifdef INIT_CONFIG
static SymTabRec DisplayTab[] = {
  { ENDSUBSECTION,	"endsubsection" },
  { MODES,		"modes" },
  { VIEWPORT,		"viewport" },
  { VIRTUAL,		"virtual" },
  { VISUAL,		"visual" },
  { BLACK,		"black" },
  { WHITE,		"white" },
  { DEPTH,		"depth" },
  { WEIGHT,		"weight" },
  { OPTION,		"option" },
  { -1,			"" },
};
#endif /* INIT_CONFIG */

/* Graphics keywords */
#define STATICGRAY	90
#define GRAYSCALE	91
#define STATICCOLOR	92
#define PSEUDOCOLOR	93
#define TRUECOLOR	94
#define DIRECTCOLOR	95

#ifdef INIT_CONFIG
static SymTabRec VisualTab[] = {
  { STATICGRAY,	"staticgray" },
  { GRAYSCALE,	"grayscale" },
  { STATICCOLOR,"staticcolor" },
  { PSEUDOCOLOR,"pseudocolor" },
  { TRUECOLOR,	"truecolor" },
  { DIRECTCOLOR,"directcolor" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */

#else /* OLD_XCONFIG */
/* This stuff will be removed soon */

#define LOCK_TOKEN  -3
#define ERROR_TOKEN -2
#define NUMBER      10000                  
#define STRING      10001

/* XConfig sections */
#define FONTPATH   0
#define RGBPATH    1
#define SHAREDMON  2
#define NOTRAPSIGNALS 3

#define KEYBOARD   10

#define MICROSOFT  20
#define MOUSESYS   21
#define MMSERIES   22
#define LOGITECH   23
#define BUSMOUSE   24
#define LOGIMAN    25
#define PS_2       26
#define MMHITTAB   27
#define XQUE       30
#define OSMOUSE    31

#define VGA256     40
#define SVGA       40
#define VGA2       41
#undef MONO /* used on Linux in /usr/include/linux/kd.h */
#define MONO       42
#define VGA16      43
#define ACCEL      44

#define MODEDB     60

#ifdef INIT_CONFIG
static SymTabRec SymTab[] = {
  { FONTPATH,   "fontpath" },
  { RGBPATH,    "rgbpath" },
  { SHAREDMON,  "sharedmonitor" },
  { NOTRAPSIGNALS, "notrapsignals" },

  { KEYBOARD,   "keyboard" },

  { MICROSOFT,  "microsoft" },
  { MOUSESYS,   "mousesystems" },
  { MMSERIES,   "mmseries" },
  { LOGITECH,   "logitech" },
  { BUSMOUSE,   "busmouse" },
  { LOGIMAN,    "mouseman" },
  { PS_2,       "ps/2" },
  { MMHITTAB,   "mmhittab" },
  { XQUE,       "xqueue" },
  { OSMOUSE,    "osmouse" },

  { VGA256,     "vga256" },
  { SVGA,       "svga" },
  { VGA2,       "vga2" },
  { MONO,       "mono" },
  { VGA16,      "vga16" },
  { ACCEL,      "accel" },


  { MODEDB,     "modedb" },

  { -1,         "" },
};
#endif /* INIT_CONFIG */

#define P_MS		0			/* Microsoft */
#define P_MSC		1			/* Mouse Systems Corp */
#define P_MM		2			/* MMseries */
#define P_LOGI		3			/* Logitech */
#define P_BM		4			/* BusMouse ??? */
#define P_LOGIMAN	5			/* MouseMan / TrackMan
						   [CHRIS-211092] */
#define P_PS2		6			/* PS/2 mouse */
#define P_MMHIT		7			/* MM_HitTab */

/* Keyboard keywords */
#define AUTOREPEAT 0
#define DONTZAP    1
#define SERVERNUM  2
#define XLEDS      3
#define VTINIT     4
#define LEFTALT    5
#define RIGHTALT   6
#define SCROLLLOCK 7
#define RIGHTCTL   8
#define VTSYSREQ   9

#ifdef INIT_CONFIG
static SymTabRec KeyboardTab[] = {
  { AUTOREPEAT, "autorepeat" },
  { DONTZAP,    "dontzap" },
  { SERVERNUM,  "servernumlock" },
  { XLEDS,      "xleds" },
  { VTINIT,     "vtinit" },
  { LEFTALT,    "leftalt" },
  { RIGHTALT,   "rightalt" },
  { RIGHTALT,   "altgr" },
  { SCROLLLOCK, "scrolllock" },
  { RIGHTCTL,   "rightctl" },
  { VTSYSREQ,   "vtsysreq" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */

/* Indexes for the specialKeyMap array */

#define K_INDEX_LEFTALT    0
#define K_INDEX_RIGHTALT   1
#define K_INDEX_SCROLLLOCK 2
#define K_INDEX_RIGHTCTL   3

/* Values for the specialKeyMap array */
#define K_META             0
#define K_COMPOSE          1
#define K_MODESHIFT        2
#define K_MODELOCK         3
#define K_SCROLLLOCK       4
#define K_CONTROL          5

#ifdef INIT_CONFIG
static SymTabRec KeyMapTab[] = {
  { K_META,       "meta" },
  { K_COMPOSE,    "compose" },
  { K_MODESHIFT,  "modeshift" },
  { K_MODELOCK,   "modelock" },
  { K_SCROLLLOCK, "scrolllock" },
  { K_CONTROL,    "control" },
  { -1,           "" },
};
#endif /* INIT_CONFIG */

/* Mouse keywords */
#define EMULATE3   0
#define BAUDRATE   1
#define SAMPLERATE 2
#define CLEARDTR   3
#define CLEARRTS   4
#define CHORDMIDDLE 5

#ifdef INIT_CONFIG
static SymTabRec MouseTab[] = {
  { BAUDRATE,   "baudrate" },
  { EMULATE3,   "emulate3buttons" },
  { SAMPLERATE, "samplerate" },
  { CLEARDTR,   "cleardtr" },
  { CLEARRTS,   "clearrts" },
  { CHORDMIDDLE,"chordmiddle" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */

/* Graphics keywords */
#define STATICGRAY  0
#define GRAYSCALE   1
#define STATICCOLOR 2
#define PSEUDOCOLOR 3
#define TRUECOLOR   4
#define DIRECTCOLOR 5

#define CHIPSET     10
#define CLOCKS      11
#define DISPLAYSIZE 12
#define MODES       13
#define SCREENNO    14
#define OPTION      15
#define VIDEORAM    16
#define VIEWPORT    17
#define VIRTUAL     18
#define SPEEDUP     19
#define NOSPEEDUP   20
#define CLOCKPROG   21
#define BIOSBASE    22
#define BLACK       23
#define WHITE       24
#define MEMBASE     25
#define IOBASE      26
#define DACBASE     27
#define COPBASE     28
#define POSBASE     29
#define INSTANCE    30
#define RAMDAC      31
#define DACSPEED    32


#ifdef INIT_CONFIG
static SymTabRec GraphicsTab[] = {
  { STATICGRAY, "staticgray" },
  { GRAYSCALE,  "grayscale" },
  { STATICCOLOR,"staticcolor" },
  { PSEUDOCOLOR,"pseudocolor" },
  { TRUECOLOR,  "truecolor" },
  { DIRECTCOLOR,"directcolor" },

  { CHIPSET,    "chipset" },
  { CLOCKS,     "clocks" },
  { DISPLAYSIZE,"displaysize" },
  { MODES,      "modes" },
  { SCREENNO,   "screenno" },
  { OPTION,     "option" },
  { VIDEORAM,   "videoram" },
  { VIEWPORT,   "viewport" },
  { VIRTUAL,    "virtual" },
  { SPEEDUP,    "speedup" },
  { NOSPEEDUP,  "nospeedup" },
  { CLOCKPROG,  "clockprog" },
  { BIOSBASE,   "biosbase" },
  { BLACK,	"black" },
  { WHITE,	"white" },
  { MEMBASE,	"membase" },
  { IOBASE,     "iobase" },
  { DACBASE,    "dacbase" },
  { COPBASE,    "copbase" },
  { POSBASE,    "posbase" },
  { INSTANCE,   "instance" },
  { RAMDAC,     "ramdac" },
  { DACSPEED,   "dacspeed" },

  { -1,         "" },
};
#endif /* INIT_CONFIG */

/* Mode timing keywords */
#define INTERLACE 0
#define PHSYNC    1
#define NHSYNC    2
#define PVSYNC    3
#define NVSYNC    4
#define CSYNC     5

#ifdef INIT_CONFIG
static SymTabRec TimingTab[] = {
  { INTERLACE,  "interlace"},
  { PHSYNC,     "+hsync"},
  { NHSYNC,     "-hsync"},
  { PVSYNC,     "+vsync"},
  { NVSYNC,     "-vsync"},
  { CSYNC,      "composite"},
  { -1,         "" },
};
#endif /* INIT_CONFIG */

#endif /* OLD_XCONFIG */

#endif /* XCONFIG_FLAGS_ONLY */

/* 
 *   Xconfig flags to record which options were defined in the Xconfig file
 */
#define XCONFIG_FONTPATH        1       /* Commandline/Xconfig or default  */
#define XCONFIG_RGBPATH         2       /* Xconfig or default */
#define XCONFIG_CHIPSET         3       /* Xconfig or probed */
#define XCONFIG_CLOCKS          4       /* Xconfig or probed */
#define XCONFIG_DISPLAYSIZE     5       /* Xconfig or default/calculated */
#define XCONFIG_VIDEORAM        6       /* Xconfig or probed */
#define XCONFIG_VIEWPORT        7       /* Xconfig or default */
#define XCONFIG_VIRTUAL         8       /* Xconfig or default/calculated */
#define XCONFIG_SPEEDUP         9       /* Xconfig or default/calculated */
#define XCONFIG_NOMEMACCESS     10      /* set if forced on */
#define XCONFIG_INSTANCE        11      /* Xconfig or default */
#define XCONFIG_RAMDAC          12      /* Xconfig or default */
#define XCONFIG_DACSPEED        13      /* Xconfig or default */
#define XCONFIG_BIOSBASE        14      /* Xconfig or default */
#define XCONFIG_MEMBASE         15      /* Xconfig or default */
#define XCONFIG_IOBASE          16      /* Xconfig or default */
#define XCONFIG_DACBASE         17      /* Xconfig or default */
#define XCONFIG_COPBASE         18      /* Xconfig or default */
#define XCONFIG_POSBASE         19      /* Xconfig or default */

#define XCONFIG_GIVEN		"(**)"
#define XCONFIG_PROBED		"(--)"

#ifdef INIT_CONFIG

OFlagSet  GenericXconfigFlag;

#else

extern OFlagSet  GenericXconfigFlag;

#endif  /* INIT_CONFIG */

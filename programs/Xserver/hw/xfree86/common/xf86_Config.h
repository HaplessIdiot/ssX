/* $XConsortium: xf86_Config.h,v 1.1 94/03/28 21:23:53 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86_Config.h,v 3.5 1994/09/03 02:51:50 dawes Exp $ */
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

typedef struct {
  int           num;                  /* returned number */
  char          *str;                 /* private copy of the return-string */
  double        realnum;              /* returned number as a real */
} LexRec, *LexPtr;

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

#define LOCK_TOKEN  -3
#define ERROR_TOKEN -2
#define NUMBER      10000                  
#define STRING      10001

/* GJA -- gave those high numbers since they occur in many sections. */
#define SECTION       10002
#define SUBSECTION    10003  /* Only used at one place now. */
#define ENDSECTION    10004
#define ENDSUBSECTION 10005
#define IDENTIFIER    10006
#define VENDOR        10007
#define DASH          10008
#define COMMA         10009

#ifdef INIT_CONFIG
static SymTabRec GlobalTab[] = {
  { SECTION,	"section" } /* endsection is in the sections. */
};
#endif /* INIT_CONFIG */

/* XConfig sections */
#define FONTPATH   0
#define RGBPATH    1
#define SHAREDMON  2
#define NOTRAPSIGNALS 3

#ifdef OLD_XCONFIG
#define KEYBOARD   10
#endif

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

#define SVGA       40
#define VGA2       41
#undef MONO /* used on Linux in /usr/include/linux/kd.h */
#define MONO       42
#define VGA16      43
#define ACCEL      44

#define MODEDB     60

#ifndef OLD_XCONFIG

#define HRZ 1	/* Silly name to avoid conflict with linux/param.h */
#define KHZ 2
#define MHZ 3
#ifdef INIT_CONFIG
static SymTabRec UnitTab[] = {
  { HRZ,	"hz" },
  { KHZ,	"khz" },
  { MHZ,	"mhz" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */

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

#ifdef INIT_CONFIG
static SymTabRec MouseTab[] = {
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
  { -1,         "" },
};
#endif /* INIT_CONFIG */

#ifdef INIT_CONFIG
static SymTabRec FilesTab[] = {
  { ENDSECTION, "endsection"},
  { FONTPATH,   "fontpath" },
  { RGBPATH,    "rgbpath" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */

#ifdef INIT_CONFIG
static SymTabRec TopLevelTab[] = {
  { SECTION,   "section" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */

#ifdef INIT_CONFIG
static SymTabRec ServerFlagsTab[] = {
  { ENDSECTION, "endsection"},
  { NOTRAPSIGNALS, "notrapsignals" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */

#define CHIPSET     10
#define CLOCKS      11
#define OPTION      12
#define VIDEORAM    13
#define BOARD       14
#define IOBASE      15
#define DACBASE     16
#define COPBASE     17
#define POSBASE     18
#define INSTANCE    19
#define RAMDAC      20
#define DACSPEED    21
#define SPEEDUP     22
#define NOSPEEDUP   23
#define CLOCKPROG   24
#define BIOSBASE    25
#define MEMBASE     26

#ifdef INIT_CONFIG
static SymTabRec DeviceTab[] = {
  { ENDSECTION, "endsection"},
  { IDENTIFIER, "identifier"},
  { VENDOR, 	"vendorname"},
  { BOARD, 	"boardname"},
  { CHIPSET,    "chipset" },
  { RAMDAC,	"ramdac" },
  { DACSPEED,   "dacspeed"},
  { CLOCKS,     "clocks" },
  { OPTION,     "option" },
  { VIDEORAM,   "videoram" },
  { IOBASE,	"iobase" },
  { DACBASE,	"dacbase" },
  { COPBASE,	"copbase" },
  { POSBASE,	"posbase" },
  { INSTANCE,	"instance" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */
#else /* OLD_XCONFIG */
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
  { VGA2,       "vga2" },
  { MONO,       "mono" },
  { VGA16,      "vga16" },
  { ACCEL,      "accel" },


  { MODEDB,     "modedb" },

  { -1,         "" },
};
#endif /* INIT_CONFIG */
#endif /* OLD_XCONFIG */

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
#define DEVICE    10

#ifdef INIT_CONFIG
static SymTabRec KeyboardTab[] = {
#ifndef OLD_XCONFIG
  { ENDSECTION, "endsection"},
  { DEVICE, "device" },
#endif
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
#define EMULATE3    0
#define BAUDRATE    1
#define SAMPLERATE  2
#define CLEARDTR    3
#define CLEARRTS    4
#define CHORDMIDDLE 5
#define PROTOCOL    6
#define PDEVICE     7

#ifndef OLD_XCONFIG
#ifdef INIT_CONFIG
static SymTabRec PointerTab[] = {
  { PDEVICE, "device"},
  { ENDSECTION, "endsection"},
  { PROTOCOL,   "protocol" },
  { BAUDRATE,   "baudrate" },
  { EMULATE3,   "emulate3buttons" },
  { SAMPLERATE, "samplerate" },
  { CLEARDTR,   "cleardtr" },
  { CLEARRTS,   "clearrts" },
  { CHORDMIDDLE,"chordmiddle" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */
#else
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
#endif

#ifndef OLD_XCONFIG
#define DISPLAYSIZE 12
#define SCREENNO    14
#define MODELINE    26
#define MODEL       27
#define BANDWIDTH   28
#define HORIZSYNC   29
#define VERTREFRESH 30
#define MODE	    31

#ifdef INIT_CONFIG
static SymTabRec MonitorTab[] = {
  { ENDSECTION,  "endsection"},
  { IDENTIFIER,  "identifier"}, 
  { VENDOR, 	 "vendorname"},
  { MODEL, 	 "modelname"}, 
  { MODELINE,    "modeline"},
  { DISPLAYSIZE, "displaysize" },
  { SCREENNO,    "screenno" },
  { SPEEDUP,     "speedup" },
  { NOSPEEDUP,   "nospeedup" },
  { CLOCKPROG,   "clockprog" },
  { BIOSBASE,    "biosbase" },
  { MEMBASE,	 "membase" },
  { BANDWIDTH,   "bandwidth" },
  { HORIZSYNC,   "horizsync" },
  { VERTREFRESH, "vertrefresh" },
  { MODE, "mode" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */

#define DOTCLOCK 1
#define HTIMINGS 2
#define VTIMINGS 3
#define FLAGS    4
#define ENDMODE  5

#ifdef INIT_CONFIG
static SymTabRec ModeTab[] = {
  { DOTCLOCK,  "dotclock"},
  { HTIMINGS,  "htimings"}, 
  { VTIMINGS,  "vtimings"},
  { FLAGS,     "flags"}, 
  { ENDMODE,   "endmode"},
  { -1,         "" },
};
#endif /* INIT_CONFIG */

/* OPTION is defined to 12 above */
#define MODES       13
#define VIRTUAL     18
#define VIEWPORT    17
#define VISUAL	    19 /* GJA -- ?? */
#define BLACK       23
#define WHITE       24
#define DEPTH       25
#define WEIGHT      26

#ifdef INIT_CONFIG
static SymTabRec DisplayTab[] = {
  { ENDSUBSECTION, "endsubsection" },
  { MODES,      "modes" },
  { VIEWPORT,   "viewport" },
  { VIRTUAL,    "virtual" },
  { VISUAL,	"visual" },
  { BLACK,	"black" },
  { WHITE,	"white" },
  { DEPTH,	"depth" },
  { WEIGHT,	"weight" },
  { OPTION,	"option" },
  { -1,         "" },
};
#endif /* INIT_CONFIG */

#define DRIVER 1
#define MDEVICE 2
#define MONITOR 3

/* Graphics keywords */
#define STATICGRAY  10 	/* Enum! */
#define GRAYSCALE   11 	/* Enum! */
#define STATICCOLOR 12 	/* Enum! */
#define PSEUDOCOLOR 13 	/* Enum! */
#define TRUECOLOR   14 	/* Enum! */
#define DIRECTCOLOR 15 	/* Enum! */

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

#ifdef INIT_CONFIG
static SymTabRec ScreenTab[] = {
  { ENDSECTION, "endsection"},

  { DRIVER,	"driver" },
  { MDEVICE,	"device" },
  { MONITOR,	"monitor" },
  { SUBSECTION,	"subsection" },
  { -1,		"" },
};
#endif /* INIT_CONFIG */
#else /* OLD_XCONFIG */
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
#endif

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

#define XCONFIG_GIVEN		"(**)"
#define XCONFIG_PROBED		"(--)"

#ifdef INIT_CONFIG

OFlagSet  GenericXconfigFlag;

#else

extern OFlagSet  GenericXconfigFlag;

#endif  /* INIT_CONFIG */

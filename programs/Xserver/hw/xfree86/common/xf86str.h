/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86str.h,v 1.21 1999/03/14 03:21:53 dawes Exp $ */

/*
 * Copyright (c) 1997 by The XFree86 Project, Inc.
 */

/*
 * This file contains definitions of the public XFree86 data structures/types.
 * Any data structures that video drivers need to access should go here.
 */

#ifndef _XF86STR_H
#define _XF86STR_H

#include "misc.h"
#include "input.h"
#include "scrnintstr.h"
#include "xf86Module.h"

/* Video mode flags */

typedef enum {
    V_PHSYNC	= 0x0001,
    V_NHSYNC	= 0x0002,
    V_PVSYNC	= 0x0004,
    V_NVSYNC	= 0x0008,
    V_INTERLACE	= 0x0010,
    V_DBLSCAN	= 0x0020,
    V_CSYNC	= 0x0040,
    V_PCSYNC	= 0x0080,
    V_NCSYNC	= 0x0100,
    V_HSKEW	= 0x0200,	/* hskew provided */
    V_PIXMUX	= 0x1000,
    V_DBLCLK	= 0x2000,
    V_CLKDIV2	= 0x4000
} ModeFlags;

typedef enum {
    INTERLACE_HALVE_V	= 0x0001	/* Halve V values for interlacing */
} CrtcAdjustFlags;


/* These are possible return values for xf86CheckMode() and ValidMode() */
typedef enum {
    MODE_OK	= 0,	/* Mode OK */
    MODE_HSYNC,		/* hsync out of range */
    MODE_VSYNC,		/* vsync out of range */
    MODE_H_ILLEGAL,	/* mode has illegal horizontal timings */
    MODE_V_ILLEGAL,	/* mode has illegal horizontal timings */
    MODE_BAD_WIDTH,	/* requires an unsupported linepitch */
    MODE_NOMODE,	/* no mode with a maching name */
    MODE_NO_INTERLACE,	/* interlaced mode not supported */
    MODE_NO_DBLESCAN,	/* doublescan mode not supported */
    MODE_MEM,		/* insufficient video memory */
    MODE_VIRTUAL_X,	/* mode width too large for specified virtual size */
    MODE_VIRTUAL_Y,	/* mode height too large for specified virtual size */
    MODE_MEM_VIRT,	/* insufficient video memory given virtual size */
    MODE_NOCLOCK,	/* no fixed clock available */
    MODE_CLOCK_HIGH,	/* clock required is too high */
    MODE_CLOCK_LOW,	/* clock required is too low */
    MODE_CLOCK_RANGE,	/* clock/mode isn't in a ClockRange */
    MODE_BAD_HVALUE,	/* horizontal timing was out of range */
    MODE_BAD_VVALUE,	/* vertical timing was out of range */
    MODE_BAD,		/* unspecified reason */
    MODE_ERROR	= -1	/* error condition */
} ModeStatus;

# define M_T_BUILTIN 0x01        /* built-in mode */
# define M_T_CLOCK_C (0x02 | M_T_BUILTIN) /* built-in mode - configure clock */
# define M_T_CRTC_C  (0x04 | M_T_BUILTIN) /* built-in mode - configure CRTC  */
# define M_T_CLOCK_CRTC_C  (M_T_CLOCK_C | M_T_CRTC_C)
                               /* built-in mode - configure CRTC and clock */

/* Video mode */

typedef struct _DisplayModeRec {
    struct _DisplayModeRec *	prev;
    struct _DisplayModeRec *	next;
    char *			name;		/* identifier for the mode */
    ModeStatus			status;
    int                         type;
    
    /* These are the values that the user sees/provides */
    int				Clock;		/* pixel clock freq */
    int				HDisplay;	/* horizontal timing */
    int				HSyncStart;
    int				HSyncEnd;
    int				HTotal;
    int				HSkew;
    int				VDisplay;	/* vertical timing */
    int				VSyncStart;
    int				VSyncEnd;
    int				VTotal;
    int				VScan;
    int				Flags;

  /* These are the values the hardware uses */
    int				ClockIndex;
    int				SynthClock;	/* Actual clock freq to
					  	 * be programmed */
    int				CrtcHDisplay;
    int				CrtcHBlankStart;
    int				CrtcHSyncStart;
    int				CrtcHSyncEnd;
    int				CrtcHBlankEnd;
    int				CrtcHTotal;
    int				CrtcHSkew;
    int				CrtcVDisplay;
    int				CrtcVBlankStart;
    int				CrtcVSyncStart;
    int				CrtcVSyncEnd;
    int				CrtcVBlankEnd;
    int				CrtcVTotal;
    Bool			CrtcHAdjusted;
    Bool			CrtcVAdjusted;
    int				PrivSize;
    INT32 *			Private;
    int				PrivFlags;
} DisplayModeRec, *DisplayModePtr;

/* The monitor description */

#define MAX_HSYNC 8
#define MAX_VREFRESH 8

typedef struct { float hi, lo; } range;

typedef struct { CARD32 red, green, blue; } rgb;

typedef struct { float red, green, blue; } Gamma;

/* The permitted gamma range is 1 / GAMMA_MAX <= g <= GAMMA_MAX */
#define GAMMA_MAX	10.0
#define GAMMA_MIN	(1.0 / GAMMA_MAX)
#define GAMMA_ZERO	(GAMMA_MIN / 100.0)

typedef struct {
    char *		id;
    char *		vendor;
    char *		model;
    int			nHsync;
    range		hsync[MAX_HSYNC];       
    int			nVrefresh;                  
    range		vrefresh[MAX_VREFRESH];
    DisplayModePtr	Modes;		/* Start of the monitor's mode list */
    DisplayModePtr	Last;		/* End of the monitor's mode list */
    Gamma		gamma;		/* Gamma of the monitor */
    int			widthmm;
    int			heightmm;
    pointer		options;
} MonRec, *MonPtr;

/* the list of clock ranges */
typedef struct x_ClockRange {
    struct x_ClockRange *next;
    int			minClock;
    int			maxClock;
    int			clockIndex;	/* -1 for programmable clocks */
    Bool		interlaceAllowed;
    Bool		doubleScanAllowed;
    int			ClockMulFactor;
    int			ClockDivFactor;
    int			PrivFlags;
} ClockRange, *ClockRangePtr;

/* Public bus-related types */
typedef enum {
    NONE,
    IO,
    MEM_IO,
    MEM
} resType;

typedef enum {
    RES_UNDEFINED = -1,
    RES_NONE,
    RES_VGA,
    RES_SHARED_VGA,
    RES_8514,
    RES_SHARED_8514,
    RES_MONO
} BusResource;

typedef struct {
    int numChipset;
    BusResource Resource;
} IsaChipsets;

typedef int (*FindIsaDevProc)(void);

typedef struct { 
    int numChipset;
    int PCIid;
    BusResource Resource;
} PciChipsets;

#define MAXCLOCKS   128
typedef enum {
    DAC_BPP8 = 0,
    DAC_BPP16,
    DAC_BPP24,
    DAC_BPP32,
    MAXDACSPEEDS
} DacSpeedIndex;
 
typedef struct {
   char *			identifier;
   char *			vendor;
   char *			board;
   char *			chipset;
   char *			ramdac;
   char *			driver;
   struct _confscreenrec *	myScreenSection;
   Bool				claimed;
   int				dacSpeeds[MAXDACSPEEDS];
   int				numclocks;
   int				clock[MAXCLOCKS];
   char *			clockchip;
   char *			busID;
   int				videoRam;
   int				textClockFreq;
   unsigned long		BiosBase;	/* Base address of video BIOS */
   unsigned long		MemBase;	/* Frame buffer base address */
   unsigned long		IOBase;
   int				chipID;
   int				chipRev;
   int				MemClk;		/* General flag used for
						   memory clocking */
   pointer			options;
} GDevRec, *GDevPtr;

typedef struct {
    int			vendor;
    int			chipType;
    int			chipRev;
    int			subsysVendor;
    int			subsysCard;
    int			bus;
    int			device;
    int			func;
    int			class;
    int			subclass;
    int			interface;
    unsigned long	memBase[6];
    unsigned long	ioBase[6];
    int			size[6];
    unsigned char	type[6];
    unsigned long	biosBase;
    int			biosSize;
    pointer		thisCard;
} pciVideoRec, *pciVideoPtr;

typedef struct {
    int			frameX0;
    int			frameY0;
    int			virtualX;
    int			virtualY;
    int			depth;
    int			fbbpp;
    rgb			weight;
    int			defaultVisual;
    char **		modes;
    pointer		options;
} DispRec, *DispPtr;

typedef struct _confscreenrec {
    char *		id;
    int			defaultdepth;
    int			defaultbpp;
    int			defaultfbbpp;
    MonPtr		monitor;
    GDevPtr		device;
    int			numdisplays;
    DispPtr		displays;
    pointer		options;
} confScreenRec, *confScreenPtr;

typedef struct _screenlayoutrec {
    confScreenPtr	screen;
    confScreenPtr	top;
    confScreenPtr	bottom;
    confScreenPtr	left;
    confScreenPtr	right;
} screenLayoutRec, *screenLayoutPtr;

typedef struct _serverlayoutrec {
    char *		id;
    screenLayoutPtr	screens;
    GDevPtr		inactives;
    pointer		options;
} serverLayoutRec, *serverLayoutPtr;
    
/* These values should be adjusted when new fields are added to ScrnInfoRec */
#define NUM_RESERVED_INTS		16
#define NUM_RESERVED_POINTERS		16
#define NUM_RESERVED_FUNCS		16

typedef pointer (*funcPointer)(void);

/* Flags for driver messages */
typedef enum {
    X_PROBED,			/* Value was probed */
    X_CONFIG,			/* Value was given in the config file */
    X_DEFAULT,			/* Value is a default */
    X_CMDLINE,			/* Value was given on the command line */
    X_NOTICE,			/* Notice */
    X_ERROR,			/* Error message */
    X_WARNING,			/* Warning message */
    X_INFO,			/* Informational message */
    X_NONE			/* No prefix */
} MessageType;

/* flags for depth 24 pixmap options */
typedef enum {
    Pix24DontCare = 0,
    Pix24Use24,
    Pix24Use32
} Pix24Flags;

/* flags for SaveRestoreImage */
typedef enum {
    SaveImage,
    RestoreImage,
    FreeImage
} SaveRestoreFlags;

/*
 * The driver list struct.  This contains the information required for each
 * driver before a ScrnInfoRec has been allocated.
 */
typedef struct _DriverRec {
    int			driverVersion;
    char *		driverName;
    void		(*Identify)(int flags);
    Bool		(*Probe)(struct _DriverRec *drv, int flags);
    pointer		module;
    int			refCount;
} DriverRec, *DriverPtr;

/*
 * The IO access enabler struct. This contains the address for 
 * the IOEnable/IODisable funcs for their specific bus along
 * with a pointer to data needed by them
 */
typedef struct _AccessRec {
    void (*AccessDisable)(void *arg);
    void (*AccessEnable)(void *arg);
    void *arg;
} xf86AccessRec, *xf86AccessPtr;

typedef struct _CurrAccRec {
    xf86AccessPtr pMemAccess;
    xf86AccessPtr pIoAccess;
} xf86CurrentAccessRec, *xf86CurrentAccessPtr;

typedef struct _ScrnAccessRec {
    xf86AccessPtr pAccess;
    xf86CurrentAccessPtr CurrentAccess;
    resType rt;
} xf86ScrnAccessRec, *xf86ScrnAccessPtr;

/* DGA */

typedef struct {
   int num;		/* A unique identifier for the mode (num > 0) */
   DisplayModePtr mode;
   int flags;		/* DGA_CONCURRENT_ACCESS, etc... */
   int imageWidth;	/* linear accessible portion (pixels) */
   int imageHeight;
   int pixmapWidth;	/* Xlib accessible portion (pixels) */
   int pixmapHeight;	/* both fields ignored if no concurrent access */
   int bytesPerScanline; 
   int byteOrder;	/* MSBFirst, LSBFirst */
   int depth;		
   int bitsPerPixel;
   unsigned long red_mask;
   unsigned long green_mask;
   unsigned long blue_mask;
   int viewportWidth;
   int viewportHeight;
   int xViewportStep;	/* viewport position granularity */
   int yViewportStep;
   int maxViewportX;	/* max viewport origin */
   int maxViewportY;
   int viewportFlags;	/* types of page flipping possible */
   unsigned char* memBase;
   int reserved1;
   int reserved2;
} DGAModeRec, *DGAModePtr;

typedef struct {
   DGAModePtr mode;
   PixmapPtr pPix;
} DGADeviceRec, *DGADevicePtr;


/*
 * ScrnInfoRec
 *
 * There is one of these for each screen, and it holds all the screen-specific
 * information.
 *
 * Note: the size and layout must be kept the same across versions.  New
 * fields are to be added in place of the "reserved*" fields.  No fields
 * are to be dependent on compile-time defines.
 */


typedef struct _ScrnInfoRec {
    int			driverVersion;
    char *              driverName;             /* canonical name used in */
                                                /* the config file */   
    ScreenPtr		pScreen;		/* Pointer to the ScreenRec */
    int			scrnIndex;		/* Number of this screen */
    Bool		configured;		/* Is this screen valid */ 
    int			origIndex;		/* initial number assigned to
						 * this screen before
						 * finalising the number of
						 * available screens */

    /* Display-wide screenInfo values needed by this screen */
    int			imageByteOrder;
    int			bitmapScanlineUnit;
    int			bitmapScanlinePad;
    int			bitmapBitOrder;
    int			numFormats;
    PixmapFormatRec	formats[MAXFORMATS];
    PixmapFormatRec	fbFormat;

    int			bitsPerPixel;		/* fb bpp */
    Pix24Flags		pixmap24;		/* pixmap pref for depth 24 */
    int			depth;			/* depth of default visual */
    MessageType		depthFrom;		/* set from config? */
    MessageType		bitsPerPixelFrom;	/* set from config? */
    rgb			weight;			/* r/g/b weights */
    rgb			mask;			/* rgb masks */
    rgb			offset;			/* rgb offsets */
    int			rgbBits;		/* Number of bits in r/g/b */
    Gamma		gamma;			/* Gamma of the monitor */
    int			defaultVisual;		/* default visual class */
    int			maxHValue;		/* max horizontal timing */
    int			maxVValue;		/* max vertical timing  value */
    int			virtualX;		/* Virtual width */
    int			virtualY; 		/* Virtual height */
    MessageType		virtualFrom;		/* set from config? */
    int			displayWidth;		/* memory pitch */
    int			frameX0;		/* viewport position */
    int			frameY0;
    int			frameX1;
    int			frameY1;
    int			zoomLocked;		/* Disallow mode changes */
    DisplayModePtr	modePool;		/* list of compatible modes */
    DisplayModePtr	modes;			/* list of actual modes */
    DisplayModePtr	currentMode;		/* current mode
						 * This was previously
						 * overloaded with the modes
						 * field, which is a pointer
						 * into a circular list */
    confScreenPtr	confScreen;		/* Screen config info */
    MonPtr		monitor;		/* Monitor information */
    GDevPtr		device;			/* device information */
    DispPtr		display;		/* Display information */
    int			widthmm;		/* physical display dimensions
						 * in mm */
    int			heightmm;
    int			xDpi;			/* width DPI */
    int			yDpi;			/* height DPI */
    char *		name;			/* Name to prefix messages */
    pointer		driverPrivate;		/* Driver private area */
    DevUnion *		privates;		/* Other privates can hook in
						 * here */
    DriverPtr		drv;			/* xf86DriverList[] entry */
    pointer		module;			/* Pointer to module head */
    int			colorKey;
    int			overlayFlags;

    /* Some of these may be moved out of here into the driver private area */

    char *		chipset;		/* chipset name */
    char *		ramdac;			/* ramdac name */
    char *		clockchip;		/* clock name */
    Bool		progClock;		/* clock is programmable */
    int			dacSpeeds[MAXDACSPEEDS];/* list of clock limits */
    int			numClocks;		/* number of clocks */
    int			clock[MAXCLOCKS];	/* list of clock frequencies */
    int			videoRam;		/* amount of video ram (kb) */
    unsigned long	biosBase;		/* Base address of video BIOS */
    unsigned long	memBase;		/* Frame buffer base address */
    unsigned long	ioBase;			/* I/O or MMIO base adderss */
    int			memClk;			/* memory clock */
    int			textClockFreq;		/* clock of text mode */
    Bool		flipPixels;		/* swap default black/white */
    pointer		options;

    int			chipID;
    int			chipRev;
    xf86ScrnAccessRec   Access;
    /* Allow screens to be enabled/disabled individually */
    Bool		vtSema;

    /*
     * These can be used when the minor ABI version is incremented.
     * The NUM_* parameters must be reduced appropriately to keep the
     * structure size and alignment unchanged.
     */
    int			reservedInt[NUM_RESERVED_INTS];
    pointer		reservedPtr[NUM_RESERVED_POINTERS];

    /*
     * Driver entry points.
     *
     */

    Bool		(*Probe)(DriverPtr drv, int flags);
    Bool		(*PreInit)(struct _ScrnInfoRec *pScrn, int flags);
    Bool		(*ScreenInit)(int scrnIndex, ScreenPtr pScreen,
				      int argc, char **argv);
    Bool		(*SwitchMode)(int scrnIndex, DisplayModePtr mode,
				      int flags);
    void		(*AdjustFrame)(int scrnIndex, int x, int y, int flags);
    Bool		(*EnterVT)(int scrnIndex, int flags);
    void		(*LeaveVT)(int scrnIndex, int flags);
    void		(*FreeScreen)(int scrnIndex, int flags);
    int			(*ValidMode)(int scrnIndex, DisplayModePtr mode,
				     Bool verbose, int flags);
    Bool		(*SaveRestoreImage)(int scrnIndex,
					    SaveRestoreFlags what);
    int			(*SetDGAMode)(int scrnIndex, int num, 
					DGADevicePtr devRet);
    /*
     * This can be used when the minor ABI version is incremented.
     * The NUM_* parameter must be reduced appropriately to keep the
     * structure size and alignment unchanged.
     */
    funcPointer		reservedFuncs[NUM_RESERVED_FUNCS];

} ScrnInfoRec, *ScrnInfoPtr;


typedef struct {
   Bool (*SetMode)(ScrnInfoPtr, DGAModePtr);
   void (*SetViewport)(ScrnInfoPtr, int, int, int);
   int  (*GetViewport)(ScrnInfoPtr, int);
   void (*Flush)(ScrnInfoPtr);
   void (*FillRect)(
	ScrnInfoPtr pScrn, 
	int x, int y, int w, int h, 
	unsigned long color
   );
   void (*BlitRect)(
	ScrnInfoPtr pScrn, 
	int srcx, int srcy, 
	int w, int h, 
	int dstx, int dsty
   );
   void (*BlitTransRect)(
	ScrnInfoPtr pScrn, 
	int srcx, int srcy, 
	int w, int h, 
	int dstx, int dsty,
	unsigned long color
   );
} DGAFunctionRec, *DGAFunctionPtr;

typedef struct {
    int			token;		/* id of the token */
    const char *	name;		/* token name */
} SymTabRec, *SymTabPtr;


/* These are the possible flags for ValidMode */
typedef enum {
    MODE_USED,		/* this mode is really being used in the */
			/* modes line of the Display Subsection  */
    MODE_SUGGESTED,	/* this mode is included in the available*/
			/* modes in the Monitor Section */
    MODE_VID		/* this is called from the VidMode extension */
} ValidModeFlags;

/* flags for xf86LookupMode */
typedef enum {
    LOOKUP_DEFAULT		= 0,	/* Use default mode lookup method */
    LOOKUP_BEST_REFRESH,		/* Pick modes with best refresh */
    LOOKUP_CLOSEST_CLOCK,		/* Pick modes with the closest clock */
    LOOKUP_LIST_ORDER,			/* Pick first useful mode in list */
    LOOKUP_CLKDIV2		= 0x0100 /* Allow half clocks */
} LookupModeFlags;

/* flags for indicating depth 24 support */
typedef enum {
    NoDepth24Support		= 0x00,
    Support24bppFb		= 0x01,	/* 24bpp framebuffer supported */
    Support32bppFb		= 0x02,	/* 32bpp framebuffer supported */
    SupportConvert24to32	= 0x04,	/* Can convert 24bpp pixmap to 32bpp */
    SupportConvert32to24	= 0x08,	/* Can convert 32bpp pixmap to 24bpp */
    PreferConvert24to32		= 0x10, /* prefer 24bpp pixmap to 32bpp conv */
    PreferConvert32to24		= 0x20	/* prefer 32bpp pixmap to 24bpp conv */
} Depth24Flags;

/*
 * mouse protocol types
 */
typedef enum {
    PROT_OSMOUSE = -1,
    PROT_MS			= 0,	/* Microsoft */
    PROT_MSC,				/* Mouse Systems Corp */
    PROT_MM,				/* MMseries */
    PROT_LOGI,				/* Logitech */
    PROT_BM,				/* BusMouse ??? */
    PROT_LOGIMAN,			/* MouseMan / TrackMan */
    PROT_PS2,				/* PS/2 mouse */
    PROT_MMHIT,				/* MM_HitTab */
    PROT_GLIDEPOINT,			/* ALPS serial GlidePoint */
    PROT_IMSERIAL,			/* Microsoft serial IntelliMouse */
    PROT_THINKING,			/* Kensington serial ThinkingMouse */
    PROT_IMPS2,				/* Microsoft PS/2 IntelliMouse */
    PROT_THINKINGPS2,			/* Kensington PS/2 ThinkingMouse */
    PROT_MMANPLUSPS2,			/* Logitech PS/2 MouseMan+ */
    PROT_GLIDEPOINTPS2,			/* ALPS PS/2 GlidePoint */
    PROT_NETPS2,			/* Genius PS/2 NetMouse */
    PROT_NETSCROLLPS2,			/* Genius PS/2 NetScroll */
    PROT_SYSMOUSE,			/* SysMouse */
    PROT_WSMOUSE,			/* wsmouse (NetBSD) */
    PROT_AUTO,				/* automatic */
    PROT_ACECAD,			/* Acecad tablets */
    NUM_PROTOCOLS			/* MUST BE LAST */
} MouseProtocol;

/*
 * keyboard specialKeyMap paramters
 */
typedef enum {
    K_INDEX_LEFTALT	= 0,
    K_INDEX_RIGHTALT,
    K_INDEX_SCROLLLOCK,
    K_INDEX_RIGHTCTL,
    NUM_KEYMAP_TYPES
} KeymapIndex;

typedef enum {
    KM_META		= 0,
    KM_COMPOSE,
    KM_MODESHIFT,
    KM_MODELOCK,
    KM_SCROLLLOCK,
    KM_CONTROL
} KeymapKey;


/* For DPMS */
typedef void (*DPMSSetProcPtr)(ScrnInfoPtr, int, int);

/* These are used by xf86GetClocks */
#define CLK_REG_SAVE		-1
#define CLK_REG_RESTORE		-2

/*
 * misc constants
 */
#define INTERLACE_REFRESH_WEIGHT	1.5
#define SYNC_TOLERANCE		0.01	/* 1 percent */
#define CLOCK_TOLERANCE		2000	/* Clock matching tolerance (2MHz) */


#define OVERLAY_8_32_DUALFB	0x00000001
#define OVERLAY_8_24_DUALFB	0x00000002
#define OVERLAY_8_16_DUALFB	0x00000004
#define OVERLAY_8_32_PLANAR	0x00000008

#if 0
#define LD_RESOLV_IFDONE		0	/* only check if no more 
						   delays pending */
#define LD_RESOLV_NOW			1	/* finish one delay step */
#define LD_RESOLV_FORCE			2	/* force checking... */
#endif

#endif /* _XF86STR_H */

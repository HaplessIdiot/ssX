/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
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
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/xf86cfg/loader.h,v 1.5 2001/07/06 02:04:10 paulo Exp $
 */
#ifdef USE_MODULES
#include "config.h"
#include "stubs.h"

#ifndef _xf86cfg_loader_h
#define _xf86cfg_loader_h

void xf86cfgLoaderInit(void);
void xf86cfgLoaderInitList(int);
void xf86cfgLoaderFreeList(void);
int xf86cfgCheckModule(void);

#ifdef LOADER_PRIVATE
#include <sym.h>

#define XFree86LOADER		/* not really */
#include <xf86_ansic.h>

/* common/xf86Module.h */
pointer LoadModule(const char *, const char *, const char **,
                   const char **, pointer, const pointer *,
                   int *, int *);
pointer LoadSubModule(pointer, const char *, const char **,
                      const char **, pointer, const pointer *,
                      int *, int *);
void UnloadSubModule(pointer);
void LoadFont(pointer);
void UnloadModule (pointer);
pointer LoaderSymbol(const char *);
char **LoaderListDirs(const char **, const char **);
void LoaderFreeDirList(char **);
void LoaderErrorMsg(const char *, const char *, int, int);
void LoadExtension(pointer, Bool);
void LoaderRefSymLists(const char **, ...);
void LoaderRefSymbols(const char *, ...);
void LoaderReqSymLists(const char **, ...);
void LoaderReqSymbols(const char *, ...);
int LoaderCheckUnresolved(int);
void LoaderGetOS(const char **name, int *major, int *minor, int *teeny);

typedef pointer (*ModuleSetupProc)(pointer, pointer, int *, int *);
typedef void (*ModuleTearDownProc)(pointer);

/*
 * Some common module classes.  The moduleclass can be used to identify
 * that modules loaded are of the correct type.  This is a finer
 * classification than the ABI classes even though the default set of
 * classes have the same names.  For example, not all modules that require
 * the video driver ABI are themselves video drivers.
 */
#define MOD_CLASS_NONE		NULL
#define MOD_CLASS_VIDEODRV	"XFree86 Video Driver"
#define MOD_CLASS_XINPUT	"XFree86 XInput Driver"
#define MOD_CLASS_FONT		"XFree86 Font Renderer"
#define MOD_CLASS_EXTENSION	"XFree86 Server Extension"

/* This structure is expected to be returned by the initfunc */
typedef struct {
    const char * modname;	/* name of module, e.g. "foo" */
    const char * vendor;	/* vendor specific string */
    CARD32	 _modinfo1_;	/* constant MODINFOSTRING1/2 to find */
    CARD32	 _modinfo2_;	/* infoarea with a binary editor or sign tool */
    CARD32	 xf86version;	/* contains XF86_VERSION_CURRENT */
    CARD8	 majorversion;	/* module-specific major version */
    CARD8	 minorversion;	/* module-specific minor version */
    CARD16	 patchlevel;	/* module-specific patch level */
    const char * abiclass;	/* ABI class that the module uses */
    CARD32	 abiversion;	/* ABI version */
    const char * moduleclass;	/* module class description */
    CARD32	 checksum[4];	/* contains a digital signature of the */
				/* version info structure */
} XF86ModuleVersionInfo;

typedef struct {
    XF86ModuleVersionInfo *	vers;
    ModuleSetupProc		setup;
    ModuleTearDownProc		teardown;
} XF86ModuleData;

/* loader/loader.h */
void LoaderDefaultFunc(void);

/* loader/loaderProcs.h */
typedef struct module_desc {
    struct module_desc *child;
    struct module_desc *sib;
    struct module_desc *parent;
    struct module_desc *demand_next;
    char *name;
    char *filename;
    char *identifier;
    XID client_id;
    int in_use;
    int handle;
    ModuleSetupProc SetupProc;
    ModuleTearDownProc TearDownProc;
    void *TearDownData; /* returned from SetupProc */
    const char *path;
} ModuleDesc, *ModuleDescPtr;

void LoaderInit(void);

ModuleDescPtr LoadDriver(const char *, const char *, int, pointer, int *,
                         int *);
ModuleDescPtr DuplicateModule(ModuleDescPtr mod, ModuleDescPtr parent);
void UnloadDriver (ModuleDescPtr);
void FreeModuleDesc (ModuleDescPtr mod);
ModuleDescPtr NewModuleDesc (const char *);
ModuleDescPtr AddSibling (ModuleDescPtr head, ModuleDescPtr new);
void LoaderSetPath(const char *path);
void LoaderSortExtensions(void);
#endif /* LOADER_PRIVATE */

/* common/xf86Opt.h */
typedef struct {
    double freq;
    int units;
} OptFrequency;

typedef union {
    unsigned long       num;
    char *              str;
    double              realnum;
    Bool		bool;
    OptFrequency	freq;
} ValueUnion;

typedef enum {
    OPTV_NONE = 0,
    OPTV_INTEGER,
    OPTV_STRING,                /* a non-empty string */
    OPTV_ANYSTR,                /* Any string, including an empty one */
    OPTV_REAL,
    OPTV_BOOLEAN,
    OPTV_FREQ
} OptionValueType;

typedef enum {
    OPTUNITS_HZ = 1,
    OPTUNITS_KHZ,
    OPTUNITS_MHZ
} OptFreqUnits;

typedef struct {
    int                 token;
    const char*         name;
    OptionValueType     type;
    ValueUnion          value;
    Bool                found;
} OptionInfoRec, *OptionInfoPtr;

/* fontmod.h */
typedef void (*InitFont)(void);

typedef struct {
    InitFont	initFunc;
    char *	name;
    void	*module;
} FontModule;

extern FontModule *FontModuleList;

#ifdef LOADER_PRIVATE
#define PROBE_DETECT	0x01

/* common/xf86str.h */
typedef struct _DriverRec {
    int			driverVersion;
    char *		driverName;
    void		(*Identify)(int flags);
    Bool		(*Probe)(struct _DriverRec *drv, int flags);
    OptionInfoPtr	(*AvailableOptions)(int chipid, int bustype);
    void *		module;
    int			refCount;
} DriverRec, *DriverPtr;

typedef struct _InputDriverRec {
    int			    driverVersion;
    char *		    driverName;
    void		    (*Identify)(int flags);
    struct _LocalDeviceRec *(*PreInit)(struct _InputDriverRec *drv,
				       void *dev, int flags);
    void		    (*UnInit)(struct _InputDriverRec *drv,
				      void *pInfo,
				      int flags);
    pointer		    module;
    int			    refCount;
} InputDriverRec, *InputDriverPtr;

typedef struct _loader_item *itemPtr;
typedef struct _loader_item {
	char	*name ;
	void	*address ;
	itemPtr	next ;
	int	handle ;
	int	module ;
	itemPtr	exports;
#if defined(__powerpc__)
	/*
	 * PowerPC file formats require special routines in some circumstances
	 * to assist in the linking process. See the specific loader for
	 * more details.
	 */
	union {
		unsigned short	plt[8];		/* ELF */
		unsigned short	glink[14];	/* XCOFF */
	} code ;
#endif
	} itemRec ;

typedef struct _ModuleInfoRec {
    int			moduleVersion;
    char *		moduleName;
    pointer		module;
    int			refCount;
    OptionInfoRec *	(*AvailableOptions)(void *unused);
    pointer		unused[2];	/* leave some space for more fields */
} ModuleInfoRec, *ModuleInfoPtr;

typedef unsigned long memType;

typedef struct {
    long type;     /* shared, exclusive, unused etc. */
    memType a;
    memType b;
} resRange, *resList;

typedef struct { 
    int numChipset;
    int PCIid;
    resRange *resList;
} PciChipsets;

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
    memType  	        memBase[6];
    memType  	        ioBase[6];
    int			size[6];
    unsigned char	type[6];
    memType   	        biosBase;
    int			biosSize;
    pointer		thisCard;
    Bool                validSize;
    Bool                validate;
    CARD32              listed_class;
} pciVideoRec, *pciVideoPtr;

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
   Bool				active;
   Bool				inUse;
   int				videoRam;
   int				textClockFreq;
   unsigned long		BiosBase;	/* Base address of video BIOS */
   unsigned long		MemBase;	/* Frame buffer base address */
   unsigned long		IOBase;
   int				chipID;
   int				chipRev;
   pointer			options;
   int                          irq;
   int                          screen;         /* For multi-CRTC cards */
} GDevRec, *GDevPtr;
#endif /* LOADER_PRIVATE */

typedef struct {
    int                 token;          /* id of the token */
    const char *        name;           /* token name */
} SymTabRec, *SymTabPtr;

typedef enum {
    NullModule = 0,
    VideoModule,
    InputModule,
    GenericModule,
    FontRendererModule
} ModuleType;

typedef struct _xf86cfgModuleOptions {
    char *name;
    ModuleType type;
    OptionInfoPtr option;
    int vendor;
    SymTabPtr chipsets;
    struct _xf86cfgModuleOptions *next;
} xf86cfgModuleOptions;

extern xf86cfgModuleOptions *module_options;

#ifndef LOADER_PRIVATE
int LoaderInitializeOptions(void);
#endif
#endif /* USE_MODULES */

#endif /* _xf86cfg_loader_h */

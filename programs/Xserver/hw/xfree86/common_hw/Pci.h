/* $XFree86: $ */
/*
 * Copyright 1998 by Concurrent Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Concurrent Computer
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Concurrent Computer Corporation makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * CONCURRENT COMPUTER CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CONCURRENT COMPUTER CORPORATION BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Copyright 1998 by Metro Link Incorporated
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Metro Link
 * Incorporated not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Metro Link Incorporated makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * METRO LINK INCORPORATED DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL METRO LINK INCORPORATED BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * This file is derived in part from the original xf86_PCI.h that included
 * following copyright message:
 *
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holder(s)
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holder(s) make(s) no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM(S) ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef _PCI_H
#define _PCI_H 1
/*** this file doesn't exist (dhh)
#include "Xarch.h"
***/
#include "Xfuncproto.h"

/*
 * Global Definitions
 */
#define MAX_PCI_DEVICES 64	/* Max number of devices accomodated */
				/* by xf86scanpci		     */
#define MAX_PCI_BUSES   32	/* Max number of PCI buses           */

#define PCI_NOT_FOUND   0xffffffff

/*
 *   
 */
#define PCI_MAKE_TAG(b,d,f)  ((((b) & 0xff) << 16) | \
			      (((d) & 0x1f) << 11) | \
			      (((f) & 0x7) << 8))

#define PCI_BUS_FROM_TAG(tag)  (((tag) & 0x00ff0000) >> 16)
#define PCI_DEV_FROM_TAG(tag)  (((tag) & 0x0000f800) >> 11)
#define PCI_FUNC_FROM_TAG(tag) (((tag) & 0x00000700) >> 8)

#define PCI_DFN_FROM_TAG(tag) (((tag) & 0x0000ff00) >> 8)

/*
 * Debug Macros/Definitions
 */
/* #define DEBUGPCI  2 */    /* Disable/enable trace in PCI code */

#if defined(DEBUGPCI)

# define PCITRACE(lvl,printfargs) \
	if (lvl > xf86Verbose) { \
		ErrorF printfargs; \
	}

#else /* !defined(DEBUGPCI) */

# define PCITRACE(lvl,printfargs)

#endif /* !defined(DEBUGPCI) */

/*
 * PCI Config mechanism definitions
 */
#define PCI_EN 0x80000000

#define	PCI_CFGMECH1_ADDRESS_REG	0xCF8
#define	PCI_CFGMECH1_DATA_REG		0xCFC

#define PCI_CFGMECH1_MAXDEV	32

#define PCI_CFGMECH1_TYPE1_CFGADDR(b,d,f,o) (PCI_EN | PCI_MAKE_TAG(b,d,f) | ((o) & 0xff) | 1)
#define PCI_CFGMECH1_TYPE0_CFGADDR(d,f,o) (PCI_EN | PCI_MAKE_TAG(0,d,f) | ((o) & 0xff))

/*
 * PCI cfg space definitions (e.g. stuff right out of the PCI spec
 */
/* Device identification register */
#define PCI_ID_REG			0x00

/* Command and status register */
#define	PCI_CMD_STAT_REG		0x04
#define	PCI_CMD_MASK			0xffff
#define	PCI_CMD_IO_ENABLE		0x01
#define	PCI_CMD_MEM_ENABLE		0x02
#define	PCI_CMD_MASTER_ENABLE		0x04
#define PCI_CMD_SPECIAL_ENABLE		0x08
#define PCI_CMD_INVALIDATE_ENABLE 	0x10
#define PCI_CMD_PALETTE_ENABLE 		0x20
#define	PCI_CMD_PARITY_ENABLE		0x40
#define	PCI_CMD_STEPPING_ENABLE		0x80
#define	PCI_CMD_SERR_ENABLE		0x100
#define	PCI_CMD_BACKTOBACK_ENABLE	0x200

/* base class */
#define PCI_CLASS_REG          		0x08
#define PCI_CLASS_MASK          	0xff000000
#define PCI_CLASS_SHIFT      		24
#define PCI_CLASS_EXTRACT(x) 		(((x) & PCI_CLASS_MASK) >> PCI_CLASS_SHIFT)

/* base class values */
#define PCI_CLASS_PREHISTORIC           0x00
#define PCI_CLASS_MASS_STORAGE          0x01
#define PCI_CLASS_NETWORK               0x02
#define PCI_CLASS_DISPLAY               0x03
#define PCI_CLASS_MULTIMEDIA            0x04
#define PCI_CLASS_MEMORY                0x05
#define PCI_CLASS_BRIDGE                0x06
#define PCI_CLASS_UNDEFINED             0xff

/* sub class */
#define PCI_SUBCLASS_MASK       0x00ff0000
#define PCI_SUBCLASS_SHIFT      16
#define PCI_SUBCLASS_EXTRACT(x) (((x) & PCI_SUBCLASS_MASK) >> PCI_SUBCLASS_SHIFT)

/* Sub class values */
/* 0x00 prehistoric subclasses */
#define PCI_SUBCLASS_PREHISTORIC_MISC   0x00
#define PCI_SUBCLASS_PREHISTORIC_VGA    0x01

/* 0x01 mass storage subclasses */
#define PCI_SUBCLASS_MASS_STORAGE_SCSI  	0x00
#define PCI_SUBCLASS_MASS_STORAGE_IDE   	0x01
#define PCI_SUBCLASS_MASS_STORAGE_FLOPPY        0x02
#define PCI_SUBCLASS_MASS_STORAGE_IPI   	0x03
#define PCI_SUBCLASS_MASS_STORAGE_MISC  	0x80

/* 0x02 network subclasses */
#define PCI_SUBCLASS_NETWORK_ETHERNET   	0x00
#define PCI_SUBCLASS_NETWORK_TOKENRING  	0x01
#define PCI_SUBCLASS_NETWORK_FDDI       	0x02
#define PCI_SUBCLASS_NETWORK_MISC       	0x80

/* 0x03 display subclasses */
#define PCI_SUBCLASS_DISPLAY_VGA        0x00
#define PCI_SUBCLASS_DISPLAY_XGA        0x01
#define PCI_SUBCLASS_DISPLAY_MISC       0x80

/* 0x04 multimedia subclasses */
#define PCI_SUBCLASS_MULTIMEDIA_VIDEO   0x00
#define PCI_SUBCLASS_MULTIMEDIA_AUDIO   0x01
#define PCI_SUBCLASS_MULTIMEDIA_MISC    0x80

/* 0x05 memory subclasses */
#define PCI_SUBCLASS_MEMORY_RAM         0x00
#define PCI_SUBCLASS_MEMORY_FLASH       0x01
#define PCI_SUBCLASS_MEMORY_MISC        0x80

/* 0x06 bridge subclasses */
#define PCI_SUBCLASS_BRIDGE_HOST        0x00
#define PCI_SUBCLASS_BRIDGE_ISA         0x01
#define PCI_SUBCLASS_BRIDGE_EISA        0x02
#define PCI_SUBCLASS_BRIDGE_MC          0x03
#define PCI_SUBCLASS_BRIDGE_PCI         0x04
#define PCI_SUBCLASS_BRIDGE_PCMCIA      0x05
#define PCI_SUBCLASS_BRIDGE_MISC        0x80

/* Header */
#define PCI_HEADER_MISC			0x0c
#define PCI_HEADER_MULTIFUNCTION	0x00800000

/* Interrupt configration register */
#define PCI_INTERRUPT_REG		0x3c
#define PCI_INTERRUPT_PIN_MASK          0x0000ff00
#define PCI_INTERRUPT_PIN_EXTRACT(x)    ((((x) & PCI_INTERRUPT_PIN_MASK) >> 8) & 0xff)
#define PCI_INTERRUPT_PIN_NONE          0x00
#define PCI_INTERRUPT_PIN_A             0x01
#define PCI_INTERRUPT_PIN_B             0x02
#define PCI_INTERRUPT_PIN_C             0x03
#define PCI_INTERRUPT_PIN_D             0x04

#define PCI_INTERRUPT_LINE_MASK         0x000000ff
#define PCI_INTERRUPT_LINE_EXTRACT(x)   ((((x) & PCI_INTERRUPT_LINE_MASK) >> 0) & 0xff) 
#define PCI_INTERRUPT_LINE_INSERT(x,v)  (((x) & ~PCI_INTERRUPT_LINE_MASK) | ((v) << 0))

/* Base registers */
#define PCI_MAP_REG_START               0x10
#define PCI_MAP_REG_END                 0x28
#define PCI_MAP_ROM_REG			0x30

#define PCI_MAP_MEMORY                  0x00000000
#define PCI_MAP_IO                      0x00000001  

#define PCI_MAP_MEMORY_TYPE_32BIT       0x00000000
#define PCI_MAP_MEMORY_TYPE_32BIT_1M    0x00000002
#define PCI_MAP_MEMORY_TYPE_64BIT       0x00000004
#define PCI_MAP_MEMORY_TYPE_MASK        0x00000006
#define PCI_MAP_MEMORY_CACHABLE         0x00000008
#define PCI_MAP_MEMORY_ADDRESS_MASK     0xfffffff0

#define PCI_MAP_IO_ADDRESS_MASK         0xfffffffc

/* PCI-PCI bridge mapping registers */
#define PCI_PCI_BRIDGE_BUS_REG          0x18
#define PCI_SUBORDINATE_BUS_MASK        0x00ff0000
#define PCI_SECONDARY_BUS_MASK          0x0000ff00
#define PCI_PRIMARY_BUS_MASK            0x000000ff

#define PCI_SUBORDINATE_BUS_EXTRACT(x)  (((x) >> 16) & 0xff)
#define PCI_SECONDARY_BUS_EXTRACT(x)    (((x) >>  8) & 0xff)
#define PCI_PRIMARY_BUS_EXTRACT(x)      (((x)      ) & 0xff)

#define PCI_PRIMARY_BUS_INSERT(x, y)    (((x) & ~PCI_PRIMARY_BUS_MASK) | ((y) << 0))
#define PCI_SECONDARY_BUS_INSERT(x, y)  (((x) & ~PCI_SECONDARY_BUS_MASK) | ((y) <<  8)) 
#define PCI_SUBORDINATE_BUS_INSERT(x, y) (((x) & ~PCI_SUBORDINATE_BUS_MASK) | (( y) << 16))

#define PCI_PCI_BRIDGE_IO_REG           0x1c
#define PCI_PCI_BRIDGE_MEM_REG          0x20
#define PCI_PCI_BRIDGE_PMEM_REG         0x24

#define PCI_PPB_IOBASE_EXTRACT(x)       (((x) << 8) & 0xFF00)
#define PCI_PPB_IOLIMIT_EXTRACT(x)      (((x) << 0) & 0xFF00)

#define PCI_PPB_MEMBASE_EXTRACT(x)      (((x) << 16) & 0xFFFF0000)
#define PCI_PPB_MEMLIMIT_EXTRACT(x)     (((x) <<  0) & 0xFFFF0000)

/* User defined cfg space regs */
#define PCI_REG_USERCONFIG 		0x40
#define PCI_OPTION_REG	 		0x40

/*
 * Typedefs, etc...
 */

/* Primitive Types */
typedef void * ADDRESS; /* Memory/PCI address          */

/*
 * Table of functions used to access a specific PCI bus domain
 * (e.g. a primary PCI bus and all of its secondaries)
 */
typedef struct pci_bus_funcs {
	CARD32  (*pciReadLong)(PCITAG, int);
	void    (*pciWriteLong)(PCITAG, int, CARD32);
	ADDRESS (*pciAddrHostToBus)(PCITAG, ADDRESS);
	ADDRESS (*pciAddrBusToHost)(PCITAG, ADDRESS);
} pciBusFuncs_t;

/*
 * pciBusInfo_t - One structure per defined PCI bus 
 */
typedef struct pci_bus_info {
	unsigned char  configMech;   /* PCI config type to use      */
	unsigned char  numDevices;   /* Range of valid devnums      */
	unsigned char  secondary;    /* Boolean: bus is a secondary */
	unsigned char  primary_bus;  /* Top level (primary) parent  */
	unsigned long  ppc_io_base;  /* PowerPC I/O spc membase     */
	unsigned long  ppc_io_size;  /* PowerPC I/O spc size        */
	pciBusFuncs_t  funcs;        /* PCI access functions        */
	void          *pciBusPriv;   /* Implementation private data */
} pciBusInfo_t;

/* configMech values */
#define PCI_CFG_MECH_UNKNOWN 0 /* Not yet known  */
#define PCI_CFG_MECH_1       1 /* Most machines  */
#define PCI_CFG_MECH_2       2 /* Older PC's     */
#define PCI_CFG_MECH_OTHER   3 /* Something else */

/*
 * PCI configuration space
 */
typedef struct pci_cfg_regs {
    /* start of official PCI config space header */
    union { 	/* Offset 0x0 - 0x3 */
	CARD32 device_vendor;
	struct {
#if BYTE_ORDER == BIG_ENDIAN		
	    CARD16 device;
	    CARD16 vendor;
#else	    
	    CARD16 vendor;
	    CARD16 device;
#endif	    
	} dv;
    } dv_id;
    
    union {    /* Offset 0x4 - 0x8 */
        CARD32 status_command;
	struct {
#if BYTE_ORDER == BIG_ENDIAN
	CARD16 status;
	CARD16 command;
#else
	CARD16 command;
	CARD16 status;
#endif		
	} sc;
    } stat_cmd;
    
    union {   /* Offset 0x8 - 0xb */
        CARD32 class_revision;
	struct {
#if BYTE_ORDER == BIG_ENDIAN		
	    CARD8 base_class;
	    CARD8 sub_class;
	    CARD8 prog_if;
	    CARD8 rev_id;
#else
	    CARD8 rev_id;
	    CARD8 prog_if;
	    CARD8 sub_class;
	    CARD8 base_class;
#endif	    
	} cr;
    } class_rev;
    
    union {	/* Offset 0xc - 0xf */
        CARD32 bist_header_latency_cache;
	struct {
#if BYTE_ORDER == BIG_ENDIAN		
	    CARD8 bist;
	    CARD8 header_type;
	    CARD8 latency_timer;
	    CARD8 cache_line_size;
#else
	    CARD8 cache_line_size;
	    CARD8 latency_timer;
	    CARD8 header_type;
	    CARD8 bist;
#endif	    
	} bhlc;
    } bhlc;
    
    union {	/* Offset 0x10 - 0x27 */ 
	struct {
	    CARD32 dv_base0;
	    CARD32 dv_base1;
	    CARD32 dv_base2;
	    CARD32 dv_base3;
	    CARD32 dv_base4;
	    CARD32 dv_base5;
	} dv;
	struct {
	    CARD32 bg_rsrvd[2];
#if BYTE_ORDER == BIG_ENDIAN	    
	    CARD8 secondary_latency_timer;
	    CARD8 subordinate_bus_number;
	    CARD8 secondary_bus_number;
	    CARD8 primary_bus_number;

	    CARD16 secondary_status;
	    CARD8 io_limit;
	    CARD8 io_base;
	    
	    CARD16 mem_limit;
	    CARD16 mem_base;

	    CARD16 prefetch_mem_limit;
	    CARD16 prefetch_mem_base;
#else
	    CARD8 primary_bus_number;
	    CARD8 secondary_bus_number;
	    CARD8 subordinate_bus_number;
	    CARD8 secondary_latency_timer;
	    
	    CARD8 io_base;
	    CARD8 io_limit;
	    CARD16 secondary_status;
	    
	    CARD16 mem_base;
	    CARD16 mem_limit;
	    
	    CARD16 prefetch_mem_base;
	    CARD16 prefetch_mem_limit;
#endif	    
	} bg;
    } bc;
    CARD32 rsvd1;	/* Offset 0x28 - 0x2b */
    CARD32 rsvd2;	/* Offset 0x2c - 0x2f */
    CARD32 _baserom;	/* Offset 0x30 - 0x33 */
    CARD32 rsvd3;	/* Offset 0x34 - 0x37 */
    CARD32 rsvd4;	/* Offset 0x38 - 0x3b */
    union {	/* Offset 0x3c - 0x3f */
        CARD32 max_min_ipin_iline;
	struct {
#if BYTE_ORDER == BIG_ENDIAN		
	    CARD8 max_lat;
	    CARD8 min_gnt;
	    CARD8 int_pin;
	    CARD8 int_line;
#else
	    CARD8 int_line;
	    CARD8 int_pin;
	    CARD8 min_gnt;
	    CARD8 max_lat;
#endif	    
	} mmii;
    } mmii;
    union {    /* Offset 0x40 - 0xff */
	    CARD32 dwords[48];
	    CARD32 bytes[192];
    } devspf;
} pciCfgRegs;

typedef union pci_cfg_spc {
	pciCfgRegs regs;
	CARD32     dwords[256/sizeof(CARD32)];  
	CARD8      bytes[256/sizeof(CARD8)];
} pciCfgSpc;

/*
 * Data structure returned by xf86scanpci including contents of
 * PCI config space header
 */
typedef struct pci_device {
    PCITAG    tag;
    int       busnum;
    int       devnum;
    int       funcnum;
    pciCfgSpc cfgspc;
} pciDevice, *pciConfigPtr;

#define _device_vendor             cfgspc.regs.dv_id.device_vendor
#define _vendor                    cfgspc.regs.dv_id.dv.vendor
#define _device                    cfgspc.regs.dv_id.dv.device
#define _status_command            cfgspc.regs.stat_cmd.status_command
#define _command                   cfgspc.regs.stat_cmd.sc.command
#define _status                    cfgspc.regs.stat_cmd.sc.status
#define _class_revision            cfgspc.regs.class_rev.class_revision
#define _rev_id                    cfgspc.regs.class_rev.cr.rev_id
#define _prog_if                   cfgspc.regs.class_rev.cr.prog_if
#define _sub_class                 cfgspc.regs.class_rev.cr.sub_class
#define _base_class                cfgspc.regs.class_rev.cr.base_class
#define _bist_header_latency_cache cfgspc.regs.bhlc.bist_header_latency_cache
#define _cache_line_size           cfgspc.regs.bhlc.bhlc.cache_line_size
#define _latency_timer             cfgspc.regs.bhlc.bhlc.latency_timer
#define _header_type               cfgspc.regs.bhlc.bhlc.header_type
#define _bist                      cfgspc.regs.bhlc.bhlc.bist
#define	_base0                     cfgspc.regs.bc.dv.dv_base0
#define	_base1			   cfgspc.regs.bc.dv.dv_base1
#define	_base2			   cfgspc.regs.bc.dv.dv_base2
#define	_base3			   cfgspc.regs.bc.dv.dv_base3
#define	_base4			   cfgspc.regs.bc.dv.dv_base4
#define	_base5			   cfgspc.regs.bc.dv.dv_base5
#define	_baserom		   cfgspc.regs._baserom
#define	_primary_bus_number	   cfgspc.regs.bc.bg.primary_bus_number
#define	_secondary_bus_number	   cfgspc.regs.bc.bg.secondary_bus_number
#define	_subordinate_bus_number	   cfgspc.regs.bc.bg.subordinate_bus_number
#define	_secondary_latency_timer   cfgspc.regs.bc.bg.secondary_latency_timer
#define _io_base		   cfgspc.regs.bc.bg.io_base
#define _io_limit		   cfgspc.regs.bc.bg.io_limit
#define _secondary_status	   cfgspc.regs.bc.bg.secondary_status
#define _mem_base		   cfgspc.regs.bc.bg.mem_base
#define _mem_limit		   cfgspc.regs.bc.bg.mem_limit
#define _prefetch_mem_base	   cfgspc.regs.bc.bg.prefetch_mem_base
#define _prefetch_mem_limit	   cfgspc.regs.bc.bg.prefetch_mem_limit
#define _rsvd1			   cfgspc.regs.rsvd1
#define _rsvd2			   cfgspc.regs.rsvd2
#define _int_line                  cfgspc.regs.mmii.mmii.int_line
#define _int_pin                   cfgspc.regs.mmii.mmii.int_pin
#define _min_gnt                   cfgspc.regs.mmii.mmii.min_gnt
#define _max_lat                   cfgspc.regs.mmii.mmii.max_lat
#define _max_min_ipin_iline        cfgspc.regs.mmii.max_min_ipin_iline
#define _user_config               cfgspc.regs.devspf.dwords[0]
#define _user_config_0		   cfgspc.regs.devspf.bytes[0]
#define _user_config_1		   cfgspc.regs.devspf.bytes[1]
#define _user_config_2		   cfgspc.regs.devspf.bytes[2]
#define _user_config_3		   cfgspc.regs.devspf.bytes[3]

/*
 * Function declarations
 */
#if NeedFunctionPrototypes

/* Public PCI access functions */
void          pciInit(void);
PCITAG        pciFindFirst(CARD32 id, CARD32 mask);
PCITAG        pciFindNext(void);
void          pciEnableIO(int);
void          pciDisableIO(int);
CARD32        pciReadLong(PCITAG tag, int offset);
CARD16        pciReadWord(PCITAG tag, int offset);
CARD8         pciReadByte(PCITAG tag, int offset);
void          pciWriteLong(PCITAG, int offset, CARD32 val);
void          pciWriteWord(PCITAG, int offset, CARD16 val);
void          pciWriteByte(PCITAG, int offset, CARD8 val);
ADDRESS       pciBusAddrToHostAddr(PCITAG tag, ADDRESS addr);
ADDRESS       pciHostAddrToBusAddr(PCITAG tag, ADDRESS addr);
PCITAG        pciTag(int busnum, int devnum, int funcnum);
pointer       xf86MapPciMem(int ScreenNum, int Region, PCITAG Tag, pointer Base, unsigned long Size);

/* Old sytle PCI access functions (for compatibility) */
pciConfigPtr *xf86scanpci(int);
void          xf86writepci(int, int, int, int, int, CARD32, CARD32);
void          xf86cleanpci(void);
int           xf86lockpci(int, int, int);

/* Generic PCI service functions and helpers */
PCITAG        pciGenFindFirst(void);
PCITAG        pciGenFindNext(void);
CARD32        pciCfgMech1Read(PCITAG tag, int offset);
void          pciCfgMech1Write(PCITAG tag, int offset, CARD32 val);
CARD32        pciByteSwap(CARD32);
Bool          pciMfDev(int, int);
CARD32        pciReadLongNULL(PCITAG tag, int offset);
void          pciWriteLongNULL(PCITAG tag, int offset, CARD32 val);
ADDRESS       pciAddrNOOP(PCITAG tag, ADDRESS);

extern PCITAG (*pciFindFirstFP)(void);
extern PCITAG (*pciFindNextFP)(void);
extern void   (*pciIOEnableFP)(int);
extern void   (*pciIODisableFP)(int);

#else /* !NeedFunctionPrototypes */

/* Public PCI access functions */
void          pciInit();
PCITAG        pciFindFirst();
PCITAG        pciFindNext();
void          pciEnableIO();
void          pciDisableIO();
CARD32        pciReadLong();
CARD16        pciReadWord();
CARD8         pciReadByte();
void          pciWriteLong();
void          pciWriteWord();
void          pciWriteByte();
ADDRESS       pciBusAddrToHostAddr();
ADDRESS       pciHostAddrToBusAddr();
PCITAG        pciTag();
pointer       xf86MapPciMem();

/* Old sytle PCI access functions (for compatibility) */
pciConfigPtr *xf86scanpci();
void          xf86writepci();
void          xf86cleanpci();
int           xf86lockpci();

/* Generic PCI service functions and helpers */
PCITAG        pciGenFindFirst();
PCITAG        pciGenFindNext();
CARD32        pciCfgMech1Read();
void          pciCfgMech1Write();
CARD32        pciByteSwap();
Bool          pciMfDev();
CARD32        pciReadLongNULL();
void          pciWriteLongNULL();
ADDRESS       pciAddrNOOP();

extern PCITAG (*pciFindFirstFP)();
extern PCITAG (*pciFindNextFP)();
extern void   (*pciIOEnableFP)();
extern void   (*pciIODisableFP)();

#endif /* !NeedFunctionPrototypes */

extern CARD32 pciDevid;    
extern CARD32 pciDevidMask;

extern int    pciBusNum;
extern int    pciDevNum;
extern int    pciFuncNum;
extern PCITAG pciDeviceTag;

extern pciBusInfo_t  *pciBusInfo[];
extern int            pciNumBuses;

extern pciBusFuncs_t pciNOOPFuncs;

#endif /* _PCI_H */

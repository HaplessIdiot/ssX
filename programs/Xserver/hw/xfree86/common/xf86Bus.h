/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Bus.h,v 1.5 1999/06/12 07:18:38 dawes Exp $ */

/*
 * Copyright (c) 1997 by The XFree86 Project, Inc.
 */

/*
 * This file contains definitions of the bus-related data structures/types.
 * Everything contained here is private to xf86Bus.c.  In particular the
 * video drivers must not include this file.
 */

#ifndef _XF86_BUS_H
#define _XF86_BUS_H

typedef struct racInfo {
    xf86AccessPtr mem_new;
    xf86AccessPtr io_new;
    xf86AccessPtr io_mem_new;
    xf86AccessPtr *old;
} AccessFuncRec, *AccessFuncPtr;

#define PCITAG_SPECIAL pciTag(0xFF,0xFF,0xFF)

typedef struct {
    DriverPtr                   driver;
    int                         chipset;
    int                         entityProp;
    EntityProc                  entityInit;
    EntityProc                  entityEnter;
    EntityProc                  entityLeave;
    pointer                     private;
    GDevPtr                     device;
    resPtr                      resources;
    Bool                        active;
    Bool                        inUse;
    BusRec                      bus;
    EntityAccessPtr             access;
    AccessFuncPtr               rac;
    pointer                     busAcc;
} EntityRec, *EntityPtr;

#define NO_SEPARATE_IO_FROM_MEM 0x0001
#define NO_SEPARATE_MEM_FROM_IO 0x0002
#define NEED_VGA_ROUTED 0x0004
#define NEED_VGA_ROUTED_SETUP 0x0008
#define NEED_MEM 0x0010
#define NEED_IO  0x0020
#define NEED_MEM_SHARED 0x0040
#define NEED_IO_SHARED 0x0080

#define NEED_SHARED (NEED_MEM_SHARED | NEED_IO_SHARED)

#define busType bus.type
#define pciBusId bus.id.pci
#define isaBusId bus.id.isa

typedef struct {
    CARD32       command;
    CARD32       base[6];
    CARD32       biosBase;
} pciSave, *pciSavePtr;

typedef void (*SetBitsProcPtr)(PCITAG, int, CARD32, CARD32);

typedef struct {
    PCITAG tag;
    SetBitsProcPtr func;
} pciArg;

typedef struct pci_io {
    int    busnum;
    int    devnum;
    int    funcnum;
    pciArg arg;
    xf86AccessRec ioAccess;
    xf86AccessRec io_memAccess;
    xf86AccessRec memAccess;
    pciSave save;
    pciSave restore;
} pciAccRec, *pciAccPtr;

typedef struct {
    CARD16      io;
    CARD32      mem;
    CARD32      pmem;
    CARD8       control;
} pciBridgeSave, *pciBridgeSavePtr;

struct x_BusAccRec;
typedef void (*BusAccProcPtr)(struct x_BusAccRec *ptr);

typedef struct x_BusAccRec {
    BusAccProcPtr set_f;
    BusAccProcPtr enable_f;
    BusAccProcPtr disable_f;
    BusAccProcPtr save_f;
    BusAccProcPtr restore_f;
    struct x_BusAccRec *current; /* pointer to bridge open on this bus */
    struct x_BusAccRec *primary; /* pointer to the bus connecting to this */
    struct x_BusAccRec *next;    /* this links the different buses together */
    BusType type;
    /* Bus-specific fields */
    union {
	struct {
	    int bus;
	    PCITAG acc;
	    pciBridgeSave save;
	    void (*func)(PCITAG,int,CARD32,CARD32);
	} pci;
    } busdep;
} BusAccRec, *BusAccPtr;

#endif /* _XF86_BUS_H */







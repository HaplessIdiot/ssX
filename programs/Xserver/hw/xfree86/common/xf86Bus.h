/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Bus.h,v 1.2 1998/07/25 16:54:57 dawes Exp $ */

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

/*
 * These are the private bus types.  New types can be added here.  Types
 * required for the public interface should be added to xf86str.h, with
 * function prototypes added to xf86.h.
 */

typedef enum {
    BUS_NONE,
    BUS_ISA,
    BUS_PCI
} BusType;

typedef struct {
    int		bus;
    int		device;
    int		func;
} PciBusId;
    
typedef struct {
    unsigned int dummy;
} IsaBusId;

/*
 * The general Bus description record.  As new types are added, new fields
 * should be added to the x_busId field.
 */

typedef struct {
    BusType			busType;
    int				scrnIndex;
    DriverPtr                   driver;
    int                         chipset;
    BusResource                 resource;
    union {
	PciBusId x_pciBusId;
	IsaBusId x_isaBusId;
    }				x_busId;
} BusRec, *BusPtr;

typedef struct pci_io {
    int    busnum;
    int    devnum;
    int    funcnum;
    PCITAG tag;
    xf86AccessRec ioAccess;
    xf86AccessRec io_memAccess;
    xf86AccessRec memAccess;
    CARD32 saveio;
    CARD32 restoreio;
} pciAccRec, *pciAccPtr;

typedef enum {
    DEV_NONE,
    DEV_VGA,
    DEV_8514
} devType;

#define pciBusId        x_busId.x_pciBusId
#define isaBusId        x_busId.x_isaBusId

#endif /* _XF86_BUS_H */

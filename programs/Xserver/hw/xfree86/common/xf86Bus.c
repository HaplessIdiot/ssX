/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Bus.c,v 1.39 1999/09/27 06:29:27 dawes Exp $ */
/*
 * Copyright (c) 1997-1999 by The XFree86 Project, Inc.
 */
#define REDUCER
/*
 * This file contains the interfaces to the bus-specific code
 */

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Resources.h"

/* Bus-specific headers */

#include "xf86Bus.h"

#define XF86_OS_PRIVS
#define NEED_OS_RAC_PROTOS
#include "xf86_OSproc.h"

#include "xf86RAC.h"

/* Entity data */
EntityPtr *xf86Entities = NULL;	/* Bus slots claimed by drivers */
int xf86NumEntities = 0;
BusAccPtr xf86BusAccInfo = NULL;

xf86AccessRec AccessNULL = {NULL,NULL,NULL};

xf86CurrentAccessRec xf86CurrentAccess = {NULL,NULL};

BusRec primaryBus = { BUS_NONE, {{0}}};

Bool xf86ResAccessEnter = FALSE;

int xf86EstimateSizesAggressively = 0;

#ifdef REDUCER
/* Resources that temporarily conflict with estimated resources */
static resPtr AccReducers = NULL;
#endif

/* resource lists */
resPtr Acc =  NULL;

/* allocatable ranges */
resPtr ResRange = NULL;

/* predefined special resources */
resRange resVgaExclusive[] = {_VGA_EXCLUSIVE, _END};
resRange resVgaShared[] = {_VGA_SHARED, _END};
resRange resVgaMemShared[] = {_VGA_SHARED_MEM,_END};
resRange resVgaIoShared[] = {_VGA_SHARED_IO,_END};
resRange resVgaUnusedExclusive[] = {_VGA_EXCLUSIVE_UNUSED, _END};
resRange resVgaUnusedShared[] = {_VGA_SHARED_UNUSED, _END};
resRange resVgaSparseExclusive[] = {_VGA_EXCLUSIVE_SPARSE, _END};
resRange resVgaSparseShared[] = {_VGA_SHARED_SPARSE, _END};
resRange res8514Exclusive[] = {_8514_EXCLUSIVE, _END};
resRange res8514Shared[] = {_8514_SHARED, _END};

/* Flag: do we need RAC ? */
static Bool needRAC = FALSE;

#undef MIN
#define MIN(x,y) ((x<y)?x:y)


/*
 * Call the bus probes relevant to the architecture.
 *
 * The only one available so far is for PCI
 */

void
xf86BusProbe(void)
{
    xf86PciProbe();
}

/*
 * Determine what bus type the busID string represents.  The start of the
 * bus-dependent part of the string is returned as retID.
 */

BusType
StringToBusType(const char* busID, const char **retID)
{
    char *p, *s;
    BusType ret = BUS_NONE;

    /* If no type field, Default to PCI */
    if (isdigit(busID[0])) {
	if (retID)
	    *retID = busID;
	return BUS_PCI;
    }

    s = xstrdup(busID);
    p = strtok(s, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return BUS_NONE;
    }
    if (!xf86NameCmp(p, "pci") || !xf86NameCmp(p, "agp"))
	ret = BUS_PCI; 
    if (!xf86NameCmp(p, "isa"))
	ret = BUS_ISA;
    if (ret != BUS_NONE)
	if (retID)
	    *retID = busID + strlen(p) + 1;
    xfree(s);
    return ret;
}

/*
 * Entity related code.
 */

void
xf86EntityInit(void)
{
    int i;
    resPtr *pprev_next;
    resPtr res;
    xf86AccessPtr pacc;
    
    for (i = 0; i < xf86NumEntities; i++)
	if (xf86Entities[i]->entityInit) {
	    if (xf86Entities[i]->access->busAcc)
		((BusAccPtr)xf86Entities[i]->access->busAcc)->set_f
		    (xf86Entities[i]->access->busAcc);
	    pacc = xf86Entities[i]->access->fallback;
	    if (pacc->AccessEnable)
		pacc->AccessEnable(pacc->arg);
	    xf86Entities[i]->entityInit(i,xf86Entities[i]->private);
	    if (pacc->AccessDisable)
		pacc->AccessDisable(pacc->arg);
	    /* remove init resources after init is processed */
	    pprev_next = &Acc;
	    res = Acc;
	    while (res) {  
		if (res->res_type & ResInit && (res->entityIndex == i)) {
		    (*pprev_next) = res->next;
		    xfree(res);
		} else 
		    pprev_next = &(res->next);
		res = (*pprev_next);
	    }
	}
}

int
xf86AllocateEntity(void)
{
    xf86NumEntities++;
    xf86Entities = xnfrealloc(xf86Entities,
			      sizeof(EntityPtr) * xf86NumEntities);
    xf86Entities[xf86NumEntities - 1] = xnfcalloc(1,sizeof(EntityRec));
    return (xf86NumEntities - 1);
}

static void
EntityEnter(void)
{
    int i;
    xf86AccessPtr pacc;
    
    for (i = 0; i < xf86NumEntities; i++)
	if (xf86Entities[i]->entityEnter) {
	    if (xf86Entities[i]->access->busAcc)
		((BusAccPtr)xf86Entities[i]->access->busAcc)->set_f
		    (xf86Entities[i]->access->busAcc);
	    pacc = xf86Entities[i]->access->fallback;
	    if (pacc->AccessEnable)
		pacc->AccessEnable(pacc->arg);
	    xf86Entities[i]->entityEnter(i,xf86Entities[i]->private);
	    if (pacc->AccessDisable)
		pacc->AccessDisable(pacc->arg);
	}
}

static void
EntityLeave(void)
{
    int i;
    xf86AccessPtr pacc;

    for (i = 0; i < xf86NumEntities; i++)
	if (xf86Entities[i]->entityLeave) {
	    if (xf86Entities[i]->access->busAcc)
		((BusAccPtr)xf86Entities[i]->access->busAcc)->set_f
		    (xf86Entities[i]->access->busAcc);
	    pacc = xf86Entities[i]->access->fallback;
	    if (pacc->AccessEnable)
		pacc->AccessEnable(pacc->arg);
	    xf86Entities[i]->entityLeave(i,xf86Entities[i]->private);
	    if (pacc->AccessDisable)
		pacc->AccessDisable(pacc->arg);
	}
}

Bool
xf86IsEntityPrimary(int entityIndex)
{
    EntityPtr pEnt = xf86Entities[entityIndex];
    
    if (primaryBus.type != pEnt->busType) return FALSE;

    switch (pEnt->busType) {
    case BUS_PCI:
	return (primaryBus.type == BUS_PCI &&
		pEnt->pciBusId.bus == primaryBus.id.pci.bus &&
		pEnt->pciBusId.device == primaryBus.id.pci.device &&
		pEnt->pciBusId.func == primaryBus.id.pci.func);
    case BUS_ISA:
	return ( primaryBus.type == BUS_ISA );
    default:
	return FALSE;
    }
}
	
Bool
xf86SetEntityFuncs(int entityIndex, EntityProc init, EntityProc enter,
		   EntityProc leave, pointer private)
{
    if (entityIndex >= xf86NumEntities)
	return FALSE;
    xf86Entities[entityIndex]->entityInit = init;
    xf86Entities[entityIndex]->entityEnter = enter;
    xf86Entities[entityIndex]->entityLeave = leave;
    xf86Entities[entityIndex]->private = private;
    return TRUE;
}

void
xf86AddEntityToScreen(ScrnInfoPtr pScrn, int entityIndex)
{
    if (entityIndex == -1)
	return;
    if (xf86Entities[entityIndex]->inUse)
	FatalError("Requested Entity already in use!\n");

    pScrn->numEntities++;
    pScrn->entityList = xnfrealloc(pScrn->entityList,
				    pScrn->numEntities * sizeof(int));
    pScrn->entityList[pScrn->numEntities - 1] = entityIndex;
    xf86Entities[entityIndex]->access->next = pScrn->access;
    pScrn->access = xf86Entities[entityIndex]->access;
    xf86Entities[entityIndex]->inUse = TRUE;
}

ScrnInfoPtr
xf86FindScreenForEntity(int entityIndex)
{
    int i,j;

    if (entityIndex == -1) return NULL;
    
    if (xf86Screens) {
	for (i = 0; i < xf86NumScreens; i++) {
	    for (j = 0; j < xf86Screens[i]->numEntities; j++) {
		if ( xf86Screens[i]->entityList[j] == entityIndex )
		    return (xf86Screens[i]);
	    }
	}
    }
    return NULL;
}

void
xf86RemoveEntityFromScreen(ScrnInfoPtr pScrn, int entityIndex)
{
    int i;
    EntityAccessPtr *ptr = (EntityAccessPtr *)&pScrn->access;
    EntityAccessPtr peacc;
    
    for (i = 0; i < pScrn->numEntities; i++) {
	if (pScrn->entityList[i] == entityIndex) {
	    peacc = xf86Entities[pScrn->entityList[i]]->access;
	    (*ptr) = peacc->next;
	    /* disable entity: call disable func */
	    if (peacc->pAccess && peacc->pAccess->AccessDisable)
		peacc->pAccess->AccessDisable(peacc->pAccess->arg);
	    /* also disable fallback - just in case */
	    if (peacc->fallback && peacc->fallback->AccessDisable)
		peacc->fallback->AccessDisable(peacc->fallback->arg);
	    for (i++; i < pScrn->numEntities; i++)
		pScrn->entityList[i-1] = pScrn->entityList[i];
	    pScrn->numEntities--;
	    xf86Entities[entityIndex]->inUse = FALSE;
	    break;
	}
	ptr = &(xf86Entities[pScrn->entityList[i]]->access->next);
    }
}

/*
 * xf86ClearEntitiesForScreen() - called when a screen is deleted
 * to mark it's entities unused. Called by xf86DeleteScreen().
 */
void
xf86ClearEntityListForScreen(int scrnIndex)
{
    int i;
    EntityAccessPtr peacc;
    
    if (xf86Screens[scrnIndex]->entityList == NULL
	|| xf86Screens[scrnIndex]->numEntities == 0) return;
	
    for (i=0; i<xf86Screens[scrnIndex]->numEntities; i++) {
	xf86Entities[xf86Screens[scrnIndex]->entityList[i]]->inUse = FALSE;
	/* disable resource: call the disable function */
	peacc = xf86Entities[xf86Screens[scrnIndex]->entityList[i]]->access;
	if (peacc->pAccess && peacc->pAccess->AccessDisable)
	    peacc->pAccess->AccessDisable(peacc->pAccess->arg);
	/* and the fallback function */
	if (peacc->fallback && peacc->fallback->AccessDisable)
	    peacc->fallback->AccessDisable(peacc->fallback->arg);
	/* shared resources are only needed when entity is active: remove */
	xf86DeallocateResourcesForEntity(i, ResShared);
    }
    xfree(xf86Screens[scrnIndex]->entityList);
    if (xf86Screens[scrnIndex]->CurrentAccess->pIoAccess
	== (EntityAccessPtr) xf86Screens[scrnIndex]->access)
	xf86Screens[scrnIndex]->CurrentAccess->pIoAccess = NULL;
    if (xf86Screens[scrnIndex]->CurrentAccess->pMemAccess
	== (EntityAccessPtr) xf86Screens[scrnIndex]->access)
	xf86Screens[scrnIndex]->CurrentAccess->pMemAccess = NULL;
    xf86Screens[scrnIndex]->entityList = NULL;
}

void
xf86DeallocateResourcesForEntity(int entityIndex, long type)
{
    resPtr *pprev_next = &Acc;
    resPtr res = Acc;

    while (res) {
	if (res->entityIndex == entityIndex &&
	    (type & ResAccMask & res->res_type))
	{
	    (*pprev_next) = res->next;
	    xfree(res);
	} else 
	    pprev_next = &(res->next);
	res = (*pprev_next);
    }
}

/*
 * xf86GetEntityInfo() -- This function hands information from the
 * EntityRec struct to the drivers. The EntityRec structure itself
 * remains invisible to the driver.
 */
EntityInfoPtr
xf86GetEntityInfo(int entityIndex)
{
    EntityInfoPtr pEnt;
    
    if (entityIndex >= xf86NumEntities)
	return NULL;
    
    pEnt = xnfcalloc(1,sizeof(EntityInfoRec));
    pEnt->index = entityIndex;
    pEnt->location = xf86Entities[entityIndex]->bus;
    pEnt->active = xf86Entities[entityIndex]->active;
    pEnt->chipset = xf86Entities[entityIndex]->chipset;
    pEnt->resources = xf86Entities[entityIndex]->resources;
    pEnt->device = xf86Entities[entityIndex]->device;
    
    return pEnt;
}

/*
 * general generic disable function.
 */
static void
disableAccess(void)
{
    int i;
    xf86AccessPtr pacc;
    EntityAccessPtr peacc;
    
    /* call disable funcs and reset current access pointer */
    /* the entity specific access funcs are in an enabled  */
    /* state - driver must restore their state explicitely */
    for (i = 0; i < xf86NumScreens; i++) {
	peacc = xf86Screens[i]->CurrentAccess->pIoAccess;
	while (peacc) {
	    if (peacc->pAccess->AccessDisable)
		peacc->pAccess->AccessDisable(peacc->pAccess->arg);
	    peacc = peacc->next;
	}
	xf86Screens[i]->CurrentAccess->pIoAccess = NULL;
	peacc = xf86Screens[i]->CurrentAccess->pMemAccess;
	while (peacc) {
	    if (peacc->pAccess->AccessDisable)
		peacc->pAccess->AccessDisable(peacc->pAccess->arg);
	    peacc = peacc->next;
	}
	xf86Screens[i]->CurrentAccess->pMemAccess = NULL;
    }
    /* then call the generic entity disable funcs */
    for (i = 0; i < xf86NumEntities; i++) {
	pacc = xf86Entities[i]->access->fallback; 
	if (pacc->AccessDisable)
	    pacc->AccessDisable(pacc->arg);
    }
}

/*
 * Generic interface to bus specific code - add other buses here
 */

/*
 * xf86AccessInit() - set up everything needed for access control
 * called only once on first server generation.
 */
void
xf86AccessInit(void)
{
    initPciState();
    initPciBusState();
    DisablePciBusAccess();
    DisablePciAccess();
    
    xf86ResAccessEnter = TRUE;
}

/*
 * xf86AccessEnter() -- gets called to save the text mode VGA IO 
 * resources when reentering the server after a VT switch.
 */
void
xf86AccessEnter(void)
{
    if (xf86ResAccessEnter) 
	return;

    /*
     * on enter we simply disable routing of special resources
     * to any bus and let the RAC code to "open" the right bridges.
     */
    PciBusStateEnter();
    DisablePciBusAccess();
    PciStateEnter();
    disableAccess();
    EntityEnter();
    xf86EnterServerState(SETUP);
    xf86ResAccessEnter = TRUE;
}

/*
 * xf86AccessLeave() -- prepares access for and calls the
 * entityLeave() functions.
 * xf86AccessLeaveState() --- gets called to restore the
 * access to the VGA IO resources when switching VT or on
 * server exit.
 * This was split to call xf86AccessLeaveState() from
 * ddxGiveUp().
 */
void
xf86AccessLeave(void)
{
    if (!xf86ResAccessEnter)
	return;
    disableAccess();
    DisablePciBusAccess();
    EntityLeave();
}

void
xf86AccessLeaveState(void)
{
    if (!xf86ResAccessEnter)
	return;
    xf86ResAccessEnter = FALSE;
    PciStateLeave();
    PciBusStateLeave();
}

/*
 * xf86EnableAccess() -- enable access to controlled resources.
 * To reduce latency when switching access the ScrnInfoRec has
 * a linked list of the EntityAccPtr of all screen entities.
 */
/*
 * switching access needs to be done in te following oder:
 * disable
 * 1. disable old entity
 * 2. reroute bus
 * 3. enable new entity
 * Otherwise resources needed for access control might be shadowed
 * by other resources!
 */
void
xf86EnableAccess(ScrnInfoPtr pScrn)
{
    register EntityAccessPtr peAcc = (EntityAccessPtr) pScrn->access;
    register EntityAccessPtr pceAcc;
    register xf86AccessPtr pAcc;
    EntityAccessPtr tmp;
#ifdef DEBUG
    ErrorF("Enable access %i\n",pScrn->scrnIndex);
#endif

    /* Entity is not under access control or currently enabled */
    if (!pScrn->access) {
	if (pScrn->busAccess)
	    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
	return;
    }
    
    switch (pScrn->resourceType) {
    case IO:
	pceAcc = pScrn->CurrentAccess->pIoAccess;
	if (peAcc == pceAcc) {
	    return;
	}
	if (pScrn->CurrentAccess->pMemAccess == pceAcc)
	    pScrn->CurrentAccess->pMemAccess = NULL;
	while (pceAcc) {
	    pAcc = pceAcc->pAccess;
	    if ( pAcc && pAcc->AccessDisable) 
		(*pAcc->AccessDisable)(pAcc->arg);
	    pceAcc = pceAcc->next;
	}
	if (pScrn->busAccess)
	    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
	while (peAcc) {
	    pAcc = peAcc->pAccess;
	    if (pAcc && pAcc->AccessEnable) 
		(*pAcc->AccessEnable)(pAcc->arg);
	    peAcc = peAcc->next;
	}
	pScrn->CurrentAccess->pIoAccess = (EntityAccessPtr) pScrn->access;
	return;
	
    case MEM_IO:
	pceAcc = pScrn->CurrentAccess->pIoAccess;
	if (peAcc != pceAcc) { /* current Io != pAccess */
	    tmp = pceAcc;
	    while (pceAcc) {
		pAcc = pceAcc->pAccess;
		if (pAcc && pAcc->AccessDisable)
		    (*pAcc->AccessDisable)(pAcc->arg);
		pceAcc = pceAcc->next;
	    }
	    pceAcc = pScrn->CurrentAccess->pMemAccess;
	    if (peAcc != pceAcc /* current Mem != pAccess */
		&& tmp !=pceAcc) {
		while (pceAcc) {
		    pAcc = pceAcc->pAccess;
		    if (pAcc && pAcc->AccessDisable)
			(*pAcc->AccessDisable)(pAcc->arg);
		    pceAcc = pceAcc->next;
		}
	    }
	} else {    /* current Io == pAccess */
	    pceAcc = pScrn->CurrentAccess->pMemAccess;
	    if (pceAcc == peAcc) { /* current Mem == pAccess */
		return;
	    }
	    while (pceAcc) {  /* current Mem != pAccess */
		pAcc = pceAcc->pAccess;
		if (pAcc && pAcc->AccessDisable) 
		    (*pAcc->AccessDisable)(pAcc->arg);
		pceAcc = pceAcc->next;
	    }
	}
	if (pScrn->busAccess)
	    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
	while (peAcc) {
	    pAcc = peAcc->pAccess;
	    if (pAcc && pAcc->AccessEnable) 
		(*pAcc->AccessEnable)(pAcc->arg);
		peAcc = peAcc->next;
	}
	pScrn->CurrentAccess->pMemAccess =
	    pScrn->CurrentAccess->pIoAccess = (EntityAccessPtr) pScrn->access;
	return;
	
    case MEM:
	pceAcc = pScrn->CurrentAccess->pMemAccess;
	if (peAcc == pceAcc) {
	    return;
	}
	if (pScrn->CurrentAccess->pIoAccess == pceAcc)
	    pScrn->CurrentAccess->pIoAccess = NULL;
	while (pceAcc) {
	    pAcc = pceAcc->pAccess;
	    if ( pAcc && pAcc->AccessDisable) 
		(*pAcc->AccessDisable)(pAcc->arg);
	    pceAcc = pceAcc->next;
	}
	if (pScrn->busAccess)
	    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
	while (peAcc) {
	    pAcc = peAcc->pAccess;
	    if (pAcc && pAcc->AccessEnable) 
		(*pAcc->AccessEnable)(pAcc->arg);
	    peAcc = peAcc->next;
	}
	pScrn->CurrentAccess->pMemAccess = (EntityAccessPtr) pScrn->access;
	return;

    case NONE:
	if (pScrn->busAccess)
	    ((BusAccPtr)pScrn->busAccess)->set_f(pScrn->busAccess);
	return;
    }
}

void
xf86SetCurrentAccess(Bool Enable, ScrnInfoPtr pScrn)
{
    EntityAccessPtr pceAcc2 = NULL;
    register EntityAccessPtr pceAcc = NULL;
    register xf86AccessPtr pAcc;

    
    switch(pScrn->resourceType) {
    case IO:
	pceAcc = pScrn->CurrentAccess->pIoAccess;
	break;
    case MEM:
	pceAcc = pScrn->CurrentAccess->pMemAccess;
	break;
    case MEM_IO:
	pceAcc = pScrn->CurrentAccess->pMemAccess;
	pceAcc2 = pScrn->CurrentAccess->pIoAccess;
	break;
    default:
	break;
    }

    while (pceAcc) {
	pAcc = pceAcc->pAccess;
	if ( pAcc) {
	    if (!Enable) {
		if (pAcc->AccessDisable) 
		    (*pAcc->AccessDisable)(pAcc->arg);
	    } else {
		if (pAcc->AccessEnable) 
		    (*pAcc->AccessEnable)(pAcc->arg);
	    }
	}
	pceAcc = pceAcc->next;
	if (!pceAcc) {
	    pceAcc = pceAcc2;
	    pceAcc2 = NULL;
	}
    }
}

void
xf86SetAccessFuncs(EntityInfoPtr pEnt, xf86AccessPtr p_io, xf86AccessPtr p_mem,
		   xf86AccessPtr p_io_mem, xf86AccessPtr *ppAccessOld)
{
    AccessFuncPtr rac;

    if (!xf86Entities[pEnt->index]->rac)
	xf86Entities[pEnt->index]->rac = xnfcalloc(1,sizeof(AccessFuncRec));

    rac = xf86Entities[pEnt->index]->rac;

    if (p_mem == p_io_mem && p_mem && p_io)
	xf86Entities[pEnt->index]->entityProp |= NO_SEPARATE_MEM_FROM_IO;
    if (p_io == p_io_mem && p_mem && p_io)
	xf86Entities[pEnt->index]->entityProp |= NO_SEPARATE_IO_FROM_MEM;
    
    rac->mem_new = p_mem;
    rac->io_new = p_io;
    rac->io_mem_new = p_io_mem;
    
    rac->old = ppAccessOld;
}

/*
 * Conflict checking
 */

static memType
getMask(memType val)
{
    memType mask = 0;
    memType tmp = 0;
    
    mask=~mask;
    tmp = ~((~tmp) >> 1);
    
    while (!(val & tmp)) {
	mask = mask >> 1;
	val = val << 1;
    }
    return mask;
}

/*
 * checkConflictBlock() -- check for conflicts of a block resource range.
 * If conflict is found return end of conflicting range. Else return 0.
 */
static memType
checkConflictBlock(resRange *range, resPtr pRes)
{
    memType val,tmp,prev;
    int i;
    
    switch (pRes->res_type & ResExtMask) {
    case ResBlock:
	if (range->rBegin < pRes->block_end &&
	    range->rEnd > pRes->block_begin)
	    return pRes->block_end < range->rEnd ?
					pRes->block_end : range->rEnd;
	return 0;
    case ResSparse:
	if (pRes->sparse_base > range->rEnd) return 0;
	
	val = (~pRes->sparse_mask | pRes->sparse_base) & getMask(range->rEnd);
#ifdef DEBUG
	ErrorF("base = 0x%lx, mask = 0x%lx, begin = 0x%lx, end = 0x%lx ,"
	       "val = 0x%lx\n",
		pRes->sparse_base, pRes->sparse_mask, range->rBegin,
		range->rEnd, val);
#endif
	i = sizeof(memType) * 8;
	tmp = prev = pRes->sparse_base;
	
	while (i) {
	    tmp |= 1<< (--i) & val;
	    if (tmp > range->rEnd)
		tmp = prev;
	    else
		prev = tmp;
	}
	if (tmp >= range->rBegin) {
#ifdef DEBUG
	    ErrorF("conflict found at: 0x%lx\n",tmp);
#endif
	    return tmp;
	}
	else
	    return 0;
    }
    return 0;
}

/*
 * checkConflictSparse() -- check for conflicts of a sparse resource range.
 * If conflict is found return base of conflicting region. Else return 0.
 */
#define mt_max ~(memType)0
#define length sizeof(memType) * 8
static memType
checkConflictSparse(resRange *range, resPtr pRes)
{
    memType val, tmp, prev;
    int i;
    
    switch (pRes->res_type & ResExtMask) {
    case ResSparse:
	tmp = pRes->sparse_mask & range->rMask;
	if ((tmp & pRes->sparse_base) == (tmp & range->rBase))
	    return pRes->sparse_mask;
	return 0;

    case ResBlock:
	if (pRes->block_end < range->rBase) return 0;
	
	val = (~range->rMask | range->rBase) & getMask(pRes->block_end);
	i = length;
	tmp = prev = range->rBase;
	
	while (i) {
#ifdef DEBUG
	    ErrorF("tmp = 0x%lx\n",tmp);
#endif
	    tmp |= 1<< (--i) & val;
	    if (tmp > pRes->block_end)
		tmp = prev;
	    else
		prev = tmp;
	}
	if (tmp < pRes->block_begin) 
	    return 0;
	else {
	    /*
	     * now we subdivide the block region in sparse regions
	     * with base values = 2^n and find the smallest mask.
	     * This might be done in a simpler way....
	     */
	    memType mask, m_mask = 0, base = pRes->block_begin;
	    int i;	    
	    while (base < pRes->block_end) {
		for (i = 1; i < length; i++)
		    if ( base != (base & (mt_max << i))) break;
		mask = mt_max >> (length - i);
		do mask >>= 1;
		while ((mask + base + 1) > pRes->block_end);
		/* m_mask and are _inverted_ sparse masks */ 
		m_mask = mask > m_mask ? mask : m_mask;
		base = base + mask + 1;
	    }
#ifdef DEBUG
	    ErrorF("conflict found at: 0x%lx\n",tmp);
#endif
	    return ~m_mask; 
	}
    }
    return 0;
}
#undef mt_max
#undef length

/*
 * needCheck() -- this function decides whether to check for conflicts
 * depending on the types of the resource ranges and their locations
 */
static Bool
needCheck(resPtr pRes, long type, int entityIndex, xf86State state)
{
    /* the same entity shouldn't conflict with itself */
    ScrnInfoPtr pScrn;
    int i;
    BusType loc = BUS_NONE;
    BusType r_loc = BUS_NONE;
    
    if (!(pRes->res_type & type & ResPhysMask)) 
        return FALSE;

    /*
     * Resources set by BIOS (ResBios) are allowed to conflict
     * with resources marked (ResBios).
     */
    if (pRes->res_type & type & ResBios)
	return FALSE;
    
    /*If requested, skip over estimated resources */
    if (pRes->res_type & type & ResEstimated)
 	return FALSE;
      
    if (type & pRes->res_type & ResUnused)
 	return FALSE;

    if (state == OPERATING) {
	if (type & ResDisableOpr || pRes->res_type & ResDisableOpr)
	    return FALSE;
	if (type & pRes->res_type & ResUnusedOpr) return FALSE;
	/*
	 * Maybe we should have ResUnused set The resUnusedOpr
	 * bit, too. This way we could avoid this confusion
	 */
	if ((type & ResUnusedOpr && pRes->res_type & ResUnused) ||
	    (type & ResUnused && pRes->res_type & ResUnusedOpr))
	    return FALSE;
    }
    
    if (entityIndex > -1)
	loc = xf86Entities[entityIndex]->busType;
    if (pRes->entityIndex > -1)
	r_loc = xf86Entities[pRes->entityIndex]->busType;

    switch (type & ResAccMask) {
    case ResExclusive:
	switch (pRes->res_type & ResAccMask) {
	case ResExclusive:
	    break;
	case ResShared:
	    /* ISA buses are only locally exclusive on a PCI system */
	    if (loc == BUS_ISA && r_loc == BUS_PCI)
		return FALSE;
	    break;
	}
	break;
    case ResShared:
	switch (pRes->res_type & ResAccMask) {
	case ResExclusive:
	    /* ISA buses are only locally exclusive on a PCI system */
	    if (loc == BUS_PCI && r_loc == BUS_ISA) 
		return FALSE;
	    break;
	case ResShared:
	    return FALSE;
	}
	break;
    case ResAny:
	break;
    }
    
    if (pRes->entityIndex == entityIndex) return FALSE;

    if (pRes->entityIndex > -1 &&
	(pScrn = xf86FindScreenForEntity(entityIndex))) {
	for (i = 0; i < pScrn->numEntities; i++)
	    if (pScrn->entityList[i] == pRes->entityIndex) return FALSE;
    }
    return TRUE;
}

/*
 * checkConflict() - main conflict checking function which all other
 * function call.
 */
static memType
checkConflict(resRange *rgp, resPtr pRes, int entityIndex, xf86State state)
{
    memType ret;
    
    while(pRes) {
	if (!needCheck(pRes,rgp->type, entityIndex ,state)) { 
	    pRes = pRes->next;                    
	    continue;                             
	}
	switch (rgp->type & ResExtMask) {
	case ResBlock:
	    if (rgp->rEnd < rgp->rBegin) {
		xf86Msg(X_ERROR,"end of block range 0x%lx < begin 0x%lx\n",
			rgp->rEnd,rgp->rBegin);
		return 0;
	    }
	    if ((ret = checkConflictBlock(rgp, pRes)))
		return ret;
	    break;
	case ResSparse:
	    if ((rgp->rBase & rgp->rMask) != rgp->rBase) {
		xf86Msg(X_ERROR,"sparse io range (base: 0x%lx  mask: 0x%lx)"
			"doesn't satisfy (base & mask = mask)\n",
			rgp->rBase, rgp->rMask);
		return 0;
	    }
	    if ((ret = checkConflictSparse(rgp, pRes)))
		return ret;
	    break;
	}
	pRes = pRes->next;
    }
    return 0;
}

/*
 * ChkConflict() -- used within xxxBus ; find conflict with any location.
 */
memType
ChkConflict(resRange *rgp, resPtr res, xf86State state)
{
    return checkConflict(rgp, res, -2, state);
}

/*
 * xf86ChkConflict() - This function is the low level interface to
 * the resource broker that gets exported. Tests all resources ie.
 * performs test with SETUP flag.
 */
memType
xf86ChkConflict(resRange *rgp, int entityIndex)
{
    return checkConflict(rgp, Acc, entityIndex, SETUP);
}

/*
 * Resources List handling
 */

resPtr
xf86JoinResLists(resPtr rlist1, resPtr rlist2)
{
    resPtr pRes;

    if (!rlist1)
	return rlist2;

    if (!rlist2)
	return rlist1;

    for (pRes = rlist1; pRes->next; pRes = pRes->next)
	;
    pRes->next = rlist2;
    return rlist1;
}

resPtr
xf86AddResToList(resPtr rlist, resRange *range, int entityIndex)
{
    resPtr new;

    switch (range->type & ResExtMask) {
    case ResBlock:
	if (range->rEnd < range->rBegin) {
		xf86Msg(X_ERROR,"end of block range 0x%lx < begin 0x%lx\n",
			range->rEnd,range->rBegin);
		return rlist;
	}
	break;
    case ResSparse:
	if ((range->rBase & range->rMask) != range->rBase) {
	    xf86Msg(X_ERROR,"sparse io range (base: 0x%lx  mask: 0x%lx)"
		    "doesn't satisfy (base & mask = mask)\n",
		    range->rBase, range->rMask);
	    return rlist;
	}
	break;
    }
    
    new = xnfalloc(sizeof(resRec));
    /* 
     * Only background resources may be registered with ResBios 
     * and ResEstimated set. Other resources only set it for
     * testing.
     */
    if (entityIndex != (-1)) 
        range->type &= ~(ResBios | ResEstimated);
    new->val = *range;
    new->entityIndex = entityIndex;
    new->next = rlist;
    return new;
}

void
xf86FreeResList(resPtr rlist)
{
    resPtr pRes;

    if (!rlist)
	return;

    for (pRes = rlist->next; pRes; rlist = pRes, pRes = pRes->next)
	xfree(rlist);
    xfree(rlist);
}

resPtr
xf86DupResList(const resPtr rlist)
{
    resPtr pRes, ret, prev, new;

    if (!rlist)
	return NULL;

    ret = xnfalloc(sizeof(resRec));
    *ret = *rlist;
    prev = ret;
    for (pRes = rlist->next; pRes; pRes = pRes->next) {
	new = xnfalloc(sizeof(resRec));
	*new = *pRes;
	prev->next = new;
	prev = new;
    }
    return ret;
}

void
xf86PrintResList(int verb, resPtr list)
{
    int i = 0;
    const char *s, *r;
    resPtr tmp = list;
    long type;
    
    if (!list)
	return;

    type = ResMem;
    r = "M";
    while (1) {
	while (list) {
	    if ((list->res_type & ResPhysMask) == type) {
		switch (list->res_type & ResExtMask) {
		case ResBlock:
		    xf86ErrorFVerb(verb, "\t[%d] %d\t0x%08x - 0x%08x (0x%x)",
				   i, list->entityIndex, list->block_begin,
				   list->block_end,
				   list->block_end - list->block_begin + 1);
		    break;
		case ResSparse:
		    xf86ErrorFVerb(verb, "\t[%d] %d\t0x%08x - 0x%08x ",
				   i, list->entityIndex,
				   list->sparse_base,list->sparse_mask);
		    break;
		default:
		    list = list->next;
		    continue;
		}
		xf86ErrorFVerb(verb, " %s", r);
		switch (list->res_type & ResAccMask) {
		case ResExclusive:
		    if (list->res_type & ResUnused)
			s = "x";
		    else
			s = "X";
		    break;
		case ResShared:
		    if (list->res_type & ResUnused)
			s = "s";
		    else
			s = "S";
		    break;
		default:
		    s = "?";
		}
		xf86ErrorFVerb(verb, "%s", s);
		switch (list->res_type & ResExtMask) {
		case ResBlock:
		    s = "B";
		    break;
		case ResSparse:
		    s = "S";
		    break;
		default:
		    s = "?";
		}
		xf86ErrorFVerb(verb, "%s", s);
		if (list->res_type & ResEstimated)
		    xf86ErrorFVerb(verb, "E");
		if (list->res_type & ResInit)
		    xf86ErrorFVerb(verb, "t");
		if (list->res_type & ResBios)
		    xf86ErrorFVerb(verb, "(B)");
		xf86ErrorFVerb(verb, "\n");
		i++;
	    }
	    list = list->next;
	}
	if (type == ResIo) break;
	type = ResIo;
	r = "I";
	list = tmp;
    }
}

resPtr
xf86AddRangesToList(resPtr list, resRange *pRange, int entityIndex)
{
    while(pRange && pRange->type != ResEnd) {
	list = xf86AddResToList(list,pRange,entityIndex);
	pRange++;
    }
    return list;
}

void
xf86ResourceBrokerInit(void)
{
    resPtr resPci;
    resPtr osRes = NULL;

    /* Get the addressable ranges */
    ResRange = xf86AccWindowsFromOS();
    xf86MsgVerb(X_INFO, 3, "Addressable resource ranges are\n");
    xf86PrintResList(3, ResRange);

    /* Get the ranges used exclusively by the system */
    osRes = xf86AccResFromOS(osRes);
    xf86MsgVerb(X_INFO, 3, "OS-reported resource ranges:\n");
    xf86PrintResList(3, osRes);

    /* Bus dep initialization */
    resPci = ResourceBrokerInitPci(&osRes);
    Acc = xf86JoinResLists(osRes, resPci);

    
    xf86MsgVerb(X_INFO, 3, "All system resource ranges:\n");
    xf86PrintResList(3, Acc);

}

#define MEM_ALIGN (1024 * 1024)

/*
 * RemoveOverlaps() -- remove overlaps between resources of the
 * same kind.
 * Beware: This function doesn't check for access attributes.
 * At resource broker initialization this is no problem as this
 * only deals with exclusive resources.
 */
void
RemoveOverlaps(resPtr target, resPtr list, Bool pow2Alignment)
{
    resPtr pRes;
    memType size, newsize, adjust;

    for (pRes = list; pRes; pRes = pRes->next) {
	if (pRes != target
	    && ((pRes->res_type & ResPhysMask) ==
		(target->res_type & ResPhysMask))
	    && pRes->block_begin <= target->block_end
	    && pRes->block_end >= target->block_begin) {
	    /*
	     * target should be a larger region than pRes.  If pRes fully
	     * contains target, don't do anything.
	     */
	    if (pRes->block_begin <= target->block_begin &&
		pRes->block_end >= target->block_end)
		continue;
	    /*
	     * cases where the target and pRes have the same starting address
	     * cannot be resolved, so skip them (with a warning).
	     */
	    if (pRes->block_begin == target->block_begin) {
		xf86MsgVerb(X_WARNING, 3, "Unresolvable overlap at 0x%08x\n",
			    pRes->block_begin);
		continue;
	    }
	    /* Otherwise, trim target to remove the overlap */
	    if (pRes->block_begin <= target->block_end) {
		target->block_end = pRes->block_begin - 1;
	    } else if (!pow2Alignment &&
		       pRes->block_end >= target->block_begin) {
		target->block_begin = pRes->block_end + 1;
	    }
	    if (pow2Alignment) {
		/*
		 * Align to a power of two.  This requires finding the
		 * largest power of two that is smaller than the adjusted
		 * size.
		 */
		size = target->block_end - target->block_begin + 1;
		newsize = 1UL << (sizeof(memType) * 8 - 1);
		while (!(newsize & size))
		    newsize >>= 1;
		target->block_end = target->block_begin + newsize - 1;
	    } else if (target->block_end > MEM_ALIGN) {
		/* Align the end to MEM_ALIGN */
		if ((adjust = (target->block_end + 1) % MEM_ALIGN))
		    target->block_end -= adjust;
	    }
	}
    }
}

/*
 * Resource request code
 */

#define ALIGN(x,a) ((x) + a) &~(a)

resRange 
xf86GetBlock(long type, memType size,
	 memType window_start, memType window_end,
	 memType align_mask, resPtr avoid)
{
    memType min, max, tmp;
    resRange r = {ResEnd,0,0};
    resPtr res_range = ResRange;
    
    if (!size) return r;
    if (window_end < window_start || (window_end - window_start) < (size - 1)) {
	ErrorF("Requesting insufficient memory window!:"
	       " start: 0x%lx end: 0x%lx size 0x%lx\n",
	       window_start,window_end,size);
	return r;
    }
    type = (type & ~(ResExtMask | ResBios | ResEstimated)) | ResBlock;
    
    while (res_range) {
	if (type & res_range->res_type & ResPhysMask) {
	    if (res_range->block_begin > window_start)
		min = res_range->block_begin;
	    else
		min = window_start;
	    if (res_range->block_end < window_end)
		max = res_range->block_end;
	    else
		max = window_end;
	    min = ALIGN(min,align_mask);
	    /* do not produce an overflow! */
	    while (min < max && (max - min) >= (size - 1)) {
		RANGE(r,min,min + size - 1,type);
		tmp = ChkConflict(&r,Acc,SETUP);
		if (!tmp) {
		    tmp = ChkConflict(&r,avoid,SETUP);
		    if (!tmp) {
			return r;
		    } 
		}
		min = ALIGN(tmp,align_mask);
	    }
	}
	res_range = res_range->next;
    }
    RANGE(r,0,0,ResEnd);
    return r;
}

#define mt_max ~(memType)0
#define length sizeof(memType) * 8
/*
 * make_base() -- assign the lowest bits to the bits set in mask.
 *                example: mask 011010 val 0000110 -> 011000 
 */
static memType
make_base(memType val, memType mask)
{
    int i,j = 0;
    memType ret = 0
	;
    for (i = 0;i<length;i++) {
	if ((1 << i) & mask) {
	    ret |= (((val >> j) & 1) << i);
	    j++;
	}
    }
    return ret;
}

/*
 * make_base() -- assign the bits set in mask to the lowest bits.
 *                example: mask 011010 , val 010010 -> 000011
 */
static memType
unmake_base(memType val, memType mask)
{
    int i,j = 0;
    memType ret = 0;
    
    for (i = 0;i<length;i++) {
	if ((1 << i) & mask) {
	    ret |= (((val >> i) & 1) << j);
	    j++;
	}
    }
    return ret;
}

static memType
fix_counter(memType val, memType old_mask, memType mask)
{
    mask = old_mask & mask;
    
    val = make_base(val,old_mask);
    return unmake_base(val,mask);
}

resRange
xf86GetSparse(long type,  memType fixed_bits,
	  memType decode_mask, memType address_mask, resPtr avoid)
{
    resRange r = {ResEnd,0,0};
    memType new_mask;
    memType mask1;
    memType base;
    memType bits;
    memType counter = 0;
    memType counter1;
    memType max_counter = ~(memType)0;
    memType max_counter1;
    memType conflict = 0;
    
    /* for sanity */
    type = (type & ~(ResExtMask | ResBios | ResEstimated)) | ResSparse;

    /*
     * a sparse address consists of 3 parts:
     * fixed_bits:   F bits which hard decoded by the hardware
     * decode_bits:  D bits which are used to decode address
     *                 but which may be set by software
     * address_bits: A bits which are used to address the
     *                 sparse range.
     * the decode_mask marks all decode bits while the address_mask
     * masks out all address_bits:
     *                F D A
     * decode_mask:   0 1 0
     * address_mask:  1 1 0
     */
    decode_mask &= address_mask;
    new_mask = decode_mask;

    /*
     * We start by setting the decode_mask bits to different values
     * when a conflict is found the address_mask of the conflicting
     * resource is returned. We remove those bits from decode_mask
     * that are also set in the returned address_mask as they always
     * conflict with resources which use them as address masks.
     * The resoulting mask is stored in new_mask.
     * We continue until no conflict is found or until we have
     * tried all possible settings of new_mask.
     */
    while (1) {
	base = make_base(counter,new_mask) | fixed_bits;
	RANGE(r,base,address_mask,type);
	conflict = ChkConflict(&r,Acc,SETUP);
	if (!conflict) {
	    conflict = ChkConflict(&r,avoid,SETUP);
	    if (!conflict) {
		return r;
	    }
	}
	counter = fix_counter(counter,new_mask,conflict);
	max_counter = fix_counter(max_counter,new_mask,conflict);
	new_mask &= conflict;
	counter ++;
	if (counter > max_counter) break;
    }
    if (!new_mask && (new_mask == decode_mask)) {
	RANGE(r,0,0,ResEnd);
	return r;
    }
    /*
     * if we haven't been successful we also try to modify those
     * bits in decode_mask that are not at the same time set in
     * new mask. These bits overlap with address_bits of some
     * resources. If a conflict with a resource of this kind is
     * found (ie. returned_mask & mask1 != mask1) with
     * mask1 = decode_mask & ~new_mask we cannot
     * use our choice of bits in the new_mask part. We try
     * another choice.
     */
    max_counter = fix_counter(mt_max,mt_max,new_mask);
    mask1 = decode_mask & ~new_mask;
    max_counter1 = fix_counter(mt_max,mt_max,mask1);
    counter = 0;
    
    while (1) {
	bits = make_base(counter,new_mask) | fixed_bits; 
	counter1 = 0;
	while (1) {
	    base = make_base(counter1,mask1);
	    RANGE(r,base,address_mask,type);
	    conflict = ChkConflict(&r,Acc,SETUP);
	    if (!conflict) {
		conflict = ChkConflict(&r,avoid,SETUP);
		if (!conflict) {
		    return r;
		}
	    }
	    counter1 ++;
	    if ((mask1 & conflict) != mask1 || counter1 > max_counter1)
		break;
	}
	counter ++;
	if (counter > max_counter) break;
    }
    RANGE(r,0,0,ResEnd);
    return r;
}

#undef length
#undef mt_max

/*
 * Resource registrarion
 */

static resList
xf86GetResourcesImplicitly(int entityIndex)
{
    if (entityIndex >= xf86NumEntities) return NULL;
    
    switch (xf86Entities[entityIndex]->bus.type) {
    case BUS_ISA:
    case BUS_NONE:
	return NULL;
    case BUS_PCI:
	return GetImplicitPciResources(entityIndex);
    }
    return NULL;
}

/*
 * xf86RegisterResources() -- attempts to register listed resources.
 * If list is NULL it tries to obtain resources implicitly. Function
 * returns a resPtr listing all resources not successfully registered.
 */
resPtr
xf86RegisterResources(int entityIndex, resList list, int access)
{
    resPtr res = NULL;
    resRange range;
    
    if (!list) {
	list = xf86GetResourcesImplicitly(entityIndex);
	if (!list) return NULL;
    }
    while(list->type != ResEnd) {
	range = *list;
	if ((access != ResNone) && (access & ResAccMask)) {
	    range.type = (range.type & ~ResAccMask) | (access & ResAccMask);
	}
 	range.type &= ~ResEstimated;	/* Not allowed for drivers */
	if(xf86ChkConflict(&range,entityIndex)) 
	    res = xf86AddResToList(res,&range,entityIndex);
	else {
	    Acc = xf86AddResToList(Acc,&range,entityIndex);
	}
	list++;
    }
#ifdef DEBUG
    xf86MsgVerb(X_INFO, 3,"Resources after driver initialization\n");
    xf86PrintResList(3, Acc);
    if (res) xf86MsgVerb(X_INFO, 3,
			 "Failed Resources after driver initialization "
			 "for Entity: %i\n",entityIndex);
    xf86PrintResList(3, res);
#endif
    return res;
    
}

/*
 * Server State 
 */

static xf86AccessPtr
busTypeSpecific(EntityPtr pEnt, xf86State state)
{
    pciAccPtr *ppaccp;

    switch (pEnt->bus.type) {
    case BUS_ISA:
	switch (state) {
	case SETUP:
	    return &AccessNULL;
	    break;
	case OPERATING:
	    if (pEnt->entityProp & NEED_SHARED)
		return &AccessNULL;
	    else  /* no conflicts at all */
		return NULL; /* remove from RAC */
	    break;
	}
	break;
    case BUS_PCI:
	ppaccp = xf86PciAccInfo;
	while (*ppaccp) {
	    if ((*ppaccp)->busnum == pEnt->pciBusId.bus
		&& (*ppaccp)->devnum == pEnt->pciBusId.device
		&& (*ppaccp)->funcnum == pEnt->pciBusId.func) {
		switch (state) {
		case SETUP:
		    (*ppaccp)->io_memAccess.AccessDisable((*ppaccp)->
							  io_memAccess.arg);
		    return &(*ppaccp)->io_memAccess;
		    break;
		case OPERATING:
		    if (!(pEnt->entityProp & NEED_MEM_SHARED)){
			if (pEnt->entityProp & NEED_MEM)
			    (*ppaccp)->memAccess.AccessEnable((*ppaccp)->
							      memAccess.arg);
			else 
			    (*ppaccp)->memAccess.AccessDisable((*ppaccp)->memAccess.arg);
		    }
		    if (!(pEnt->entityProp & NEED_IO_SHARED)) {
			if (pEnt->entityProp & NEED_IO)
			    (*ppaccp)->ioAccess.AccessEnable((*ppaccp)->
							     ioAccess.arg);
			else 
			    (*ppaccp)->ioAccess.AccessDisable((*ppaccp)->
							      ioAccess.arg);
		    }
		    switch(pEnt->entityProp & NEED_SHARED) {
		    case NEED_IO_SHARED:
			return &(*ppaccp)->ioAccess;
		    case NEED_MEM_SHARED:
			return &(*ppaccp)->memAccess;
		    case NEED_SHARED:
			return &(*ppaccp)->io_memAccess;
		    default: /* no conflicts at all */
			return NULL; /* remove from RAC */
		    }
		    break;
		}
	    }
	    ppaccp++;
	}
	break;
    default:
	return NULL;
    }
    return NULL;
}

/*
 * setAccess() -- sets access functions according to resources
 * required. 
 */
static void
setAccess(EntityPtr pEnt, xf86State state)
{
    xf86AccessPtr new = NULL;
    
    /* set access funcs and access state according to resource requirements */
    pEnt->access->pAccess = busTypeSpecific(pEnt,state);
    
    if (state == OPERATING) {
	switch(pEnt->entityProp & NEED_SHARED) {
	case NEED_SHARED:
	    pEnt->access->rt = MEM_IO;
	    break;
	case NEED_IO_SHARED:
	    pEnt->access->rt = IO;
	    break;
	case NEED_MEM_SHARED:
	    pEnt->access->rt = MEM;
	    break;
	default:
	    pEnt->access->rt = NONE;
	}
    } else 
	pEnt->access->rt = MEM_IO;
    
    /* disable shared resources */
    if (pEnt->access->pAccess 
	&& pEnt->access->pAccess->AccessDisable)
	pEnt->access->pAccess->AccessDisable(pEnt->access->pAccess->arg);

    /*
     * If device is not under access control it is enabled.
     * If it needs bus routing do it here as it isn't bus
     * type specific. Any conflicts should be checked at this
     * stage
     */
    if (!pEnt->access->pAccess
	&& (pEnt->entityProp & (state == SETUP ? NEED_VGA_ROUTED_SETUP :
				NEED_VGA_ROUTED))) 
	((BusAccPtr)pEnt->busAcc)->set_f(pEnt->busAcc);
    
    /* do we have a driver replacement for the generic access funcs ?*/
    if (pEnt->rac) {
	/* XXX should we use rt here? */
	switch (pEnt->access->rt) {
	case MEM_IO:
	    new = pEnt->rac->io_mem_new;
	    break;
	case IO:
	    new = pEnt->rac->io_new;
	    break;
	case MEM:
	    new = pEnt->rac->mem_new;
	    break;
	default:
	    new = NULL;
	    break;
	}
    }
    if (new) {
	/* does the driver want the old access func? */
	if (pEnt->rac->old) {
	    /* give it to the driver, leave state disabled */
	    (*pEnt->rac->old) = pEnt->access->pAccess;
	} else if ((pEnt->access->rt != NONE) && pEnt->access->pAccess
		   && pEnt->access->pAccess->AccessEnable) {
	    /* driver doesn't want it - enable generic access */
	    pEnt->access->pAccess->AccessEnable(pEnt->access->pAccess->arg);
	}
	/* now replace access funcs */
	pEnt->access->pAccess = new;
	/* call the new disable func just to be shure */
	/* XXX should we do this only if rt != NONE? */
	if (pEnt->access->pAccess && pEnt->access->pAccess->AccessDisable)
	    pEnt->access->pAccess->AccessDisable(pEnt->access->pAccess->arg);
	/* The replacement function needs to handle _all_ shared resources */
	/* unless they are handeled locally and disabled otherwise         */
    } 
} 

/*
 * xf86EnterServerState() -- set state the server is in.
 */
void
xf86EnterServerState(xf86State state)
{
    EntityPtr pEnt;
    ScrnInfoPtr pScrn;
    int i,j;
    resType rt;

#ifdef DEBUG
    if (state == SETUP)
	ErrorF("Entering SETUP state\n");
    else
	ErrorF("Entering OPERATING state\n");
#endif

    for (i=0; i<xf86NumScreens; i++) {
	pScrn = xf86Screens[i];
	j = pScrn->entityList[pScrn->numEntities - 1];
	pScrn->access = xf86Entities[j]->access;
	
 	for (j = 0; j<xf86Screens[i]->numEntities; j++) {
 	    pEnt = xf86Entities[xf86Screens[i]->entityList[j]];
 	    if (pEnt->entityProp & (state == SETUP ? NEED_VGA_ROUTED_SETUP
 				    : NEED_VGA_ROUTED))
		xf86Screens[i]->busAccess = pEnt->busAcc;
 	}
    }
    
    /*
     * if we just have one screen we don't have RAC.
     * Therefore just enable the screen and return.
     */
    if (!needRAC) {
	xf86EnableAccess(xf86Screens[0]);
	return;
    }
    
    disableAccess();
    
    for (i=0; i<xf86NumScreens;i++) {

	rt = NONE;
	
	for (j = 0; j<xf86Screens[i]->numEntities; j++) {
	    pEnt = xf86Entities[xf86Screens[i]->entityList[j]];

	    setAccess(pEnt,state);

	    if (pEnt->access->rt != NONE) {
		if (rt != NONE && rt != pEnt->access->rt)
		    rt = MEM_IO;
		else
		    rt = pEnt->access->rt;
	    }
	}
	xf86Screens[i]->resourceType = rt;
	if (rt == NONE) {
	    xf86Screens[i]->access = NULL;
	    xf86Screens[i]->busAccess = NULL;
	}
	
#ifdef DEBUG
	if (xf86Screens[i]->busAccess)
	    ErrorF("Screen %i setting vga route\n",i);
#endif
	switch (rt) {
	case MEM_IO:
	    xf86MsgVerb(X_INFO, 3, "Screen %i shares mem & io resources\n",i);
	    break;
	case IO:
	    xf86MsgVerb(X_INFO, 3, "Screen %i shares io resources\n",i);
	    break;
	case MEM:
	    xf86MsgVerb(X_INFO, 3, "Screen %i shares mem resources\n",i);
	    break;
	default:
	    xf86MsgVerb(X_INFO, 3, "Entity %i shares no resources\n",i);
	    break;
	}
    }
}

/*
 * xf86SetOperatingState() -- Set ResOperMask for resources listed.
 */
resPtr
xf86SetOperatingState(resList list, int entityIndex, int mask)
{
    resPtr acc;
    resPtr r_fail = NULL;

    while (list->type != ResEnd) {
	acc = Acc;
	while (acc) {
#define MASK (ResPhysMask | ResExtMask)
	    if ((acc->entityIndex == entityIndex) 
		&& (acc->val.a == list->a) && (acc->val.b == list->b)
		&& ((acc->val.type & MASK) == (list->type & MASK)))
		break;
#undef MASK
	    acc = acc->next;
	}
	if (acc)
	    acc->val.type = (acc->val.type & ~ResOprMask)
		| (mask & ResOprMask);
	else {
	    r_fail = xf86AddResToList(r_fail,list,entityIndex);
	}
	list ++;
    }
    
    return r_fail;
}

/*
 * Stage specific code
 */
 /*
  * ProcessEstimatedConflicts() -- Do something about driver-registered
  * resources that conflict with estimated resources.  For now, just register
  * them with a logged warning.
  */
#ifdef REDUCER
static void
ProcessEstimatedConflicts(void)
{
    if (!AccReducers)
	return;

    /* Temporary */
    xf86MsgVerb(X_WARNING, 3,
		"Registering the following despite conflicts with estimated"
		" resources:\n");
    xf86PrintResList(3, AccReducers);
    Acc = xf86JoinResLists(Acc, AccReducers);
    AccReducers = NULL;
}
#endif

/*
 * xf86ClaimFixedResources() -- This function gets called from the
 * driver Probe() function to claim fixed resources.
 */
static void
resError(resList list)
{
    FatalError("A driver tried to allocate the %s %sresource at \n"
	       "0x%x:0x%x which conflicted with another resource. Send the\n"
	       "output of the server to xfree86@xfree86.org. Please \n"
	       "specify your computer hardware as closely as possible.\n",
	       ResIsBlock(list)?"Block":"Sparse",
	       ResIsMem(list)?"Mem":"Io",
	       ResIsBlock(list)?list->rBegin:list->rBase,
	       ResIsBlock(list)?list->rEnd:list->rMask);
}

/*
 * xf86ClaimFixedResources() is used to allocate non-relocatable resources.
 * This should only be done by a driver's Probe() function.
 */
void
xf86ClaimFixedResources(resList list, int entityIndex)
{
    resPtr ptr = NULL;
    resRange range;	

    if (!list) return;
    
    while (list->type !=ResEnd) {
 	range = *list;
 	range.type &= ~ResEstimated;	/* Not allowed for drivers */
 	switch (range.type & ResAccMask) {
  	case ResExclusive:
 	    if (!xf86ChkConflict(&range, entityIndex)) {
 		Acc = xf86AddResToList(Acc, &range, entityIndex);
#ifdef REDUCER
	    } else {
 		range.type |= ResEstimated;
 		if (!xf86ChkConflict(&range, entityIndex) &&
 		    !checkConflict(&range, AccReducers, entityIndex, SETUP)) {
 		    range.type &= ~(ResEstimated | ResBios);
 		    AccReducers =
 			xf86AddResToList(AccReducers, &range, entityIndex);
#endif
		} else resError(&range); /* no return */
#ifdef REDUCER
	    }
#endif
	    break;
	case ResShared:
	    /* at this stage the resources are just added to the
	     * EntityRec. After the Probe() phase this list is checked by
	     * xf86PostProbe(). All resources which don't
	     * conflict with already allocated ones are allocated
	     * and removed from the EntityRec. Thus a non-empty resource
	     * list in the EntityRec indicates resource conflicts the
	     * driver should either handle or fail.
	     */
	    if (xf86Entities[entityIndex]->active)
		ptr = xf86AddResToList(ptr,&range,entityIndex);
	    break;
	}
	list++;
    }
    xf86Entities[entityIndex]->resources =
	xf86JoinResLists(xf86Entities[entityIndex]->resources,ptr);
    xf86MsgVerb(X_INFO, 3,
	"resource ranges after xf86ClaimFixedResources() call:\n");
    xf86PrintResList(3,Acc);
#ifdef REDUCER
    ProcessEstimatedConflicts();
#endif
#ifdef DEBUG
    if (ptr) {
	xf86MsgVerb(X_INFO, 3, "to be registered later:\n");
	xf86PrintResList(3,ptr);
    }
#endif
}

static void
checkRoutingForScreens(xf86State state)
{
    resList list = resVgaUnusedExclusive;
    resPtr pResVGA = NULL;
    pointer vga = NULL;
    int i,j;
    int entityIndex;
    EntityPtr pEnt;
    resPtr pAcc;
    resRange range;

    /*
     * find devices that need VGA routed: ie the ones that have
     * registered VGA resources without ResUnused. ResUnused
     * doesn't conflict with itself therefore use it here.
     */
    while (list->type != ResEnd) { /* create resPtr from resList for VGA */
	range = *list;
	range.type &= ~(ResBios | ResEstimated); /* if set remove them */
	pResVGA = xf86AddResToList(pResVGA, &range, -1);
	list++;
    }

    for (i = 0; i < xf86NumScreens; i++) {
	for (j = 0; j < xf86Screens[i]->numEntities; j++) {
	    entityIndex = xf86Screens[i]->entityList[j];
	    pEnt = xf86Entities[entityIndex];
	    pAcc = Acc;
	    vga = NULL;
	    while (pAcc) {
		if (pAcc->entityIndex == entityIndex)
		    if (checkConflict(&pAcc->val,pResVGA,
				      entityIndex,state)) {
			if (vga && vga != pEnt->busAcc) {
			    xf86Msg(X_ERROR, "Screen %i needs vga routed to"
				    "different buses - deleting\n",i);
			    xf86DeleteScreen(i--,0);
			}
			vga = pEnt->busAcc;
			pEnt->entityProp |= (state == SETUP
			    ? NEED_VGA_ROUTED_SETUP : NEED_VGA_ROUTED);
			break;
		    }
		pAcc = pAcc->next;
	    }
	    if (vga)
		xf86MsgVerb(X_INFO, 3,"Setting vga for screen %i.\n",i);
	}
    }
    xf86FreeResList(pResVGA);
}

/*
 * xf86PostProbe() -- Allocate all non conflicting resources
 * This function gets called by xf86Init().
 */
void
xf86PostProbe(void)
{
    memType val;
    int i,j;
    resPtr resp, acc, tmp, resp_x, *pprev_next;

    /* don't compare against ResInit - remove it from clone.*/
    acc = tmp = xf86DupResList(Acc);
    pprev_next = &acc;
    while (tmp) {
	if (tmp->res_type & ResInit) {
	    (*pprev_next) = tmp->next;
	    xfree(tmp);
	} else 
	    pprev_next = &(tmp->next);
	tmp = (*pprev_next);
    }

    for (i=0; i<xf86NumEntities; i++) {
	resp = xf86Entities[i]->resources;
	xf86Entities[i]->resources = NULL;
	resp_x = NULL;
	while (resp) {
	    if (! (val = checkConflict(&resp->val,acc,i,SETUP)))  {
 	        resp->res_type &= ~(ResBios); /* just used for chkConflict() */
		tmp = resp_x;
		resp_x = resp;
		resp = resp->next;
		resp_x->next = tmp;
#ifdef REDUCER
	    } else {
		resp->res_type |= ResEstimated;
 		if (!checkConflict(&resp->val, acc, i, SETUP)) {
 		    resp->res_type &= ~(ResEstimated | ResBios);
 		    tmp = AccReducers;
 		    AccReducers = resp;
 		    resp = resp->next;
 		    AccReducers->next = tmp;
#endif
		} else {
		    xf86MsgVerb(X_INFO, 3, "Found conflict at: 0x%lx\n",val);
 		    resp->res_type &= ~ResEstimated;
		    tmp = xf86Entities[i]->resources;
		    xf86Entities[i]->resources = resp;
		    resp = resp->next;
		    xf86Entities[i]->resources->next = tmp;
		}
#ifdef REDUCER
	    }
#endif
	}
	xf86JoinResLists(Acc,resp_x);
#ifdef REDUCER
	ProcessEstimatedConflicts();
#endif
    }
    xf86FreeResList(acc);

    ValidatePci();
    
    xf86MsgVerb(X_INFO, 3, "resource ranges after probing:\n");
    xf86PrintResList(3, Acc);
    checkRoutingForScreens(SETUP);

    for (i = 0; i < xf86NumScreens; i++) {
	for (j = 0; j<xf86Screens[i]->numEntities; j++) {
	    EntityPtr pEnt = xf86Entities[xf86Screens[i]->entityList[j]];
 	    if ((pEnt->entityProp & NEED_VGA_ROUTED_SETUP) &&
 		((xf86Screens[i]->busAccess = pEnt->busAcc)))
		break;
	}
    }
}

static void
checkRequiredResources(int entityIndex)
{
    resRange range;
    resPtr pAcc = Acc;
    const EntityPtr pEnt = xf86Entities[entityIndex];
    while (pAcc) {
	if (pAcc->entityIndex == entityIndex) {
	    range = pAcc->val;
	    /*  ResAny to find conflicts with anything. */
	    range.type = (range.type & ~ResAccMask) | ResAny | ResBios;
	    if (checkConflict(&range,Acc,entityIndex,OPERATING))
		switch (pAcc->res_type & ResPhysMask) {
		case ResMem:
		    pEnt->entityProp |= NEED_MEM_SHARED;
		    break;
		case ResIo:
		    pEnt->entityProp |= NEED_IO_SHARED;
		    break;
		}
	    if (!(pAcc->res_type & ResOprMask)) {
		switch (pAcc->res_type & ResPhysMask) {
		case ResMem:
		    pEnt->entityProp |= NEED_MEM;
		    break;
		case ResIo:
		    pEnt->entityProp |= NEED_IO;
		    break;
		}
	    }
	}
	pAcc = pAcc->next;
    }
    
    /* check if we can separately enable mem/io resources */
    /* XXX we still need to find out how to set this yet  */
    if ( ((pEnt->entityProp & NO_SEPARATE_MEM_FROM_IO)
	  && (pEnt->entityProp & NEED_MEM_SHARED))
	 || ((pEnt->entityProp & NO_SEPARATE_IO_FROM_MEM)
	     && (pEnt->entityProp & NEED_IO_SHARED)) )
	pEnt->entityProp |= NEED_SHARED;
    /*
     * After we have checked all resources of an entity agains any
     * other resource we know if the entity need this resource type
     * (ie. mem/io) at all. if not we can disable this type completely,
     * so no need to share it either. 
     */
    if ((pEnt->entityProp & NEED_MEM_SHARED)
	&& (!(pEnt->entityProp & NEED_MEM))
	&& (!(pEnt->entityProp & NO_SEPARATE_MEM_FROM_IO)))
	pEnt->entityProp &= ~(unsigned long)NEED_MEM_SHARED;

    if ((pEnt->entityProp & NEED_IO_SHARED)
	&& (!(pEnt->entityProp & NEED_IO))
	&& (!(pEnt->entityProp & NO_SEPARATE_IO_FROM_MEM)))
	pEnt->entityProp &= ~(unsigned long)NEED_IO_SHARED;
}

void
xf86PostPreInit()
{
    if (xf86NumScreens > 1)
	needRAC = TRUE;

#ifdef XFree86LOADER
    ErrorF("do I need RAC?\n");
    
    if (needRAC) {
	char *list[] = { "rac",NULL };
	ErrorF("  Yes, I do.\n");
	
	if (!xf86LoadModules(list, NULL))
	    FatalError("Cannot load RAC module\n");
    } else
	ErrorF("  No, I don't.\n");
#endif    
 	
    xf86MsgVerb(X_INFO, 3, "resource ranges after preInit:\n");
    xf86PrintResList(3, Acc);
}

void
xf86PostScreenInit(void)
{
    int i,j;
    ScreenPtr pScreen;
    unsigned int flags;
    int nummem = 0, numio = 0;

#ifdef XFree86LOADER
	pointer xf86RACInit = NULL;
	if (needRAC) {
	    xf86RACInit = LoaderSymbol("xf86RACInit");
	    if (!xf86RACInit)
		FatalError("Cannot resolve symbol \"xf86RACInit\"\n");
	}
#endif
#ifdef DEBUG
    ErrorF("PostScreenInit  generation: %i\n",serverGeneration);
#endif
    if (serverGeneration == 1) {
	checkRoutingForScreens(OPERATING);
	for (i=0; i<xf86NumEntities; i++) {
	    checkRequiredResources(i);
	}
	
	/*
	 * after removing NEED_XXX_SHARED from entities that
	 * don't need need XXX resources at all we might have
	 * a single entity left that has NEED_XXX_SHARED set.
	 * In this case we can delete that, too.
	 */
	for (i = 0; i < xf86NumEntities; i++) {
	    if (xf86Entities[i]->entityProp & NEED_MEM_SHARED)
		nummem++;
	    if (xf86Entities[i]->entityProp & NEED_IO_SHARED)
		numio++;
	}
	for (i = 0; i < xf86NumEntities; i++) {
	    if (nummem < 2)
		xf86Entities[i]->entityProp &= ~NEED_MEM_SHARED;
	    if (numio < 2)
		xf86Entities[i]->entityProp &= ~NEED_IO_SHARED;
	}
    }
    
    if (xf86Screens && needRAC) {
	for (i = 0; i < xf86NumScreens; i++) {
	    Bool needRACforMem = FALSE, needRACforIo = FALSE;
	    
	    for (j = 0; j < xf86Screens[i]->numEntities; j++) {
		if (xf86Entities[xf86Screens[i]->entityList[j]]->entityProp
		    & NEED_MEM_SHARED)
		    needRACforMem = TRUE;
		if (xf86Entities[xf86Screens[i]->entityList[j]]->entityProp
		    & NEED_IO_SHARED)
		    needRACforIo = TRUE;
	    }
	    
	    pScreen = xf86Screens[i]->pScreen;
	    flags = 0;
	    if (needRACforMem) {
		flags |= xf86Screens[i]->racMemFlags;
#ifdef DEBUG
		ErrorF("Screen %d is using RAC for mem\n", i);
#endif
	    }
	    if (needRACforIo) {
		flags |= xf86Screens[i]->racIoFlags;
#ifdef DEBUG
		ErrorF("Screen %d is using RAC for io\n", i);
#endif
	    }
	    
#ifdef XFree86LOADER
		((Bool(*)(ScreenPtr,unsigned int))xf86RACInit)
		    (pScreen,flags);
#else
	    xf86RACInit(pScreen,flags);
#endif
	}
    }
    
    xf86EnterServerState(OPERATING);
    
}

/*
 * Sets
 */

 /* not yet used. FIXME: add support for sparse */

Bool
isSubsetOf(resRange range, resPtr list)
{
    while (list) {
	if (range.type & list->res_type & ResPhysMask) {
	    switch (range.type & ResExtMask) {
	    case ResBlock:
		switch (list->res_type & ResExtMask) {
		case ResBlock:
		    if (range.rBegin >= list->block_begin
			&& range.rEnd <= list->block_end)
			return TRUE;
			break;
		}
		break;
	    }
	}
	list = list->next;
    }
    return FALSE;
}

Bool
isListSubsetOf(resPtr list, resPtr BaseList)
{
    while (list) {
	if (! isSubsetOf(list->val,BaseList))
	    return FALSE;
	list = list->next;
    }
    return TRUE;
}

resPtr
findIntersect(resRange Range, resPtr list)
{
    resRange range;
    resPtr new = NULL;
    
    while (list) {
	    if (Range.type & list->res_type & ResPhysMask) {
		switch (Range.type & ResExtMask) {
		case ResBlock:
		    switch (list->res_type & ResExtMask) {
		    case ResBlock:
			if (Range.rBegin >= list->block_begin)
			    range.rBegin = Range.rBegin;
			else
			    range.rBegin = list->block_begin;
			if (Range.rEnd <= list->block_end)
			    range.rEnd = Range.rEnd;
			else 
			    range.rEnd = list->block_end;
			if (range.rEnd > range.rBegin) {
			    range.type = Range.type;
			    new = xf86AddResToList(new,&range,-1);
			}
			break;
		    }
		    break;
		}
	    }
	list = list->next;
    }
    return new;
}
    
resPtr
xf86FindIntersectOfLists(resPtr l1, resPtr l2)
{
    resPtr ret = NULL;

    while (l1) {
	ret = xf86JoinResLists(ret,findIntersect(l1->val,l2));
	l1 = l1->next;
    }
    return ret;
}


/*------------------------------------------------------------*/
static void CheckGenericGA(void);

/*
 * xf86FindPrimaryDevice() - Find the display device which
 * was active when the server was started.
 */
void
xf86FindPrimaryDevice()
{
    /* if no VGA device is found check for primary PCI device */
    if (primaryBus.type == BUS_NONE)
        CheckGenericGA();
}

#include "vgaHW.h"
#include "compiler.h"

/*
 * CheckGenericGA() - Check for presence of a VGA device.
 */
static void
CheckGenericGA()
{
#if !defined(__sparc__) && !defined(__powerpc__) /* FIXME ?? */
    CARD16 GenericIOBase = VGAHW_GET_IOBASE();
    CARD8 CurrentValue, TestValue;

    /* VGA CRTC registers are not used here, so don't bother unlocking them */

    /* VGA has one more read/write attribute register than EGA */
    (void) inb(GenericIOBase + 0x0AU);  /* Reset flip-flop */
    outb(0x3C0, 0x14 | 0x20);
    CurrentValue = inb(0x3C1);
    outb(0x3C0, CurrentValue ^ 0x0F);
    outb(0x3C0, 0x14 | 0x20);
    TestValue = inb(0x3C1);
    outb(0x3C0, CurrentValue);

    if ((CurrentValue ^ 0x0F) == TestValue) {
	primaryBus.type = BUS_ISA;
    }
#endif
}

Bool
xf86NoSharedMem(int screenIndex)
{
    int j;
    
    if (screenIndex > xf86NumScreens)
	return TRUE;

    for (j = 0; j < xf86Screens[screenIndex]->numEntities; j++) {
	if ( xf86Entities[xf86Screens[screenIndex]->entityList[j]]->entityProp
	     & NEED_MEM_SHARED)
	    return FALSE;
    }
    return TRUE;
}


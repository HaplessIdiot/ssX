/* $XConsortium: xkbUtils.c /main/16 1995/12/07 21:23:15 kaleb $ */
/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be 
used in advertising or publicity pertaining to distribution 
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability 
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#define NEED_EVENTS 1
#include <X11/X.h>
#include <X11/Xproto.h>
#define	XK_CYRILLIC
#include <X11/keysym.h>
#include "misc.h"
#include "inputstr.h"

#define	XKBSRV_NEED_FILE_FUNCS
#include "XKBsrv.h"
#include "extensions/XKBgeom.h"

#ifdef MODE_SWITCH
extern Bool noKME; /* defined in os/utils.c */
#endif

	int	XkbDisableLockActions = 0;

/***====================================================================***/

#ifndef RETURN_SHOULD_REPEAT
#if (defined(__osf__) && defined(__alpha))
#define RETURN_SHOULD_REPEAT 1
#else
#define	RETURN_SHOULD_REPEAT 0
#endif
#endif

/***====================================================================***/

DeviceIntPtr
_XkbLookupAnyDevice(id,why_rtrn)
    int id;
    int *why_rtrn;
{
DeviceIntPtr dev = NULL;

    dev= (DeviceIntPtr)LookupKeyboardDevice();
    if ((id==XkbUseCoreKbd)||(dev->id==id))
	return dev;

    dev= (DeviceIntPtr)LookupPointerDevice();
    if ((id==XkbUseCorePtr)||(dev->id==id))
	return dev;

    if (id&(~0xff))
	 dev = NULL;
#ifdef XINPUT
    else dev = (DeviceIntPtr)LookupDeviceIntRec(id);
#endif
    if ((!dev)&&(why_rtrn))
	*why_rtrn= XkbErr_BadDevice;
    return dev;
}

DeviceIntPtr
_XkbLookupKeyboard(id,why_rtrn)
    int id;
    int *why_rtrn;
{
DeviceIntPtr dev = NULL;

    if ((dev= _XkbLookupAnyDevice(id,why_rtrn))==NULL)
	return NULL;
    else if ((!dev->key)||(!dev->key->xkbInfo)) {
	if (why_rtrn)
	   *why_rtrn= XkbErr_BadClass;
	return NULL;
    }
    return dev;
}

DeviceIntPtr
_XkbLookupBellDevice(id,why_rtrn)
    int id;
    int *why_rtrn;
{
DeviceIntPtr dev = NULL;

    if ((dev= _XkbLookupAnyDevice(id,why_rtrn))==NULL)
	return NULL;
    else if ((!dev->kbdfeed)&&(!dev->bell)) {
	if (why_rtrn)
	   *why_rtrn= XkbErr_BadClass;
	return NULL;
    }
    return dev;
}

DeviceIntPtr
_XkbLookupLedDevice(id,why_rtrn)
    int id;
    int *why_rtrn;
{
DeviceIntPtr dev = NULL;

    if ((dev= _XkbLookupAnyDevice(id,why_rtrn))==NULL)
	return NULL;
    else if ((!dev->kbdfeed)&&(!dev->leds)) {
	if (why_rtrn)
	   *why_rtrn= XkbErr_BadClass;
	return NULL;
    }
    return dev;
}

DeviceIntPtr
_XkbLookupButtonDevice(id,why_rtrn)
    int id;
    int *why_rtrn;
{
DeviceIntPtr dev = NULL;

    if ((dev= _XkbLookupAnyDevice(id,why_rtrn))==NULL)
	return NULL;
    else if (!dev->button) {
	if (why_rtrn)
	   *why_rtrn= XkbErr_BadClass;
	return NULL;
    }
    return dev;
}

static void
XkbSetActionKeyMods(xkb,act,mods)
    XkbDescPtr	xkb;
    XkbAction *	act;
    unsigned	mods;
{
register unsigned	tmp;

    switch (act->type) {
	case XkbSA_SetMods: case XkbSA_LatchMods: case XkbSA_LockMods:
	    if (act->mods.flags&XkbSA_UseModMapMods)
		act->mods.real_mods= act->mods.mask= mods;
	    if ((tmp= XkbModActionVMods(&act->mods))!=0)
		act->mods.mask|= XkbMaskForVMask(xkb,tmp);
	    break;
	case XkbSA_ISOLock:
	    if (act->iso.flags&XkbSA_UseModMapMods)
		act->iso.real_mods= act->iso.mask= mods;
	    if ((tmp= XkbModActionVMods(&act->iso))!=0)
		act->iso.mask|= XkbMaskForVMask(xkb,tmp);
	    break;
    }
    return;
}

/***====================================================================***/

	/*
	 * These two functions provide more-or-less the same functionality
	 * via slightly different interfaces.   XkbVirtualModsToReal
	 * is a identical to the XKB X library function with the same
	 * name; we need a copy in the server because some of the functions
	 * in xkmread.c use it.   XkbVirtualModsToReal reports failure if
	 * you pass in an XkbDescRec that is so incomplete that makes it
	 * impossible to determine the correct mask.  The XkbMaskToVMask
	 * interface is a little more efficient and convenient and is
	 * used only inside the server.
	 */
Bool
XkbVirtualModsToReal(xkb,virtual_mask,mask_rtrn)
    XkbDescPtr		xkb;
    unsigned		virtual_mask;
    unsigned *		mask_rtrn;
{
register int i,bit;
register unsigned mask;

    if (xkb==NULL)
        return False;
    if (virtual_mask==0) {
        *mask_rtrn= 0;
        return True;
    }
    if (xkb->server==NULL)
        return False;
    for (i=mask=0,bit=1;i<XkbNumVirtualMods;i++,bit<<=1) {
        if (virtual_mask&bit)
	    mask|= xkb->server->vmods[i];
    }
    *mask_rtrn= mask;
    return True;
}

unsigned
XkbMaskForVMask(xkb,vmask)
    XkbDescPtr	xkb;
    unsigned	vmask;
{
register int i,bit;
register unsigned mask;
    
    for (mask=i=0,bit=1;i<XkbNumVirtualMods;i++,bit<<=1) {
	if (vmask&bit)
	    mask|= xkb->server->vmods[i];
    }
    return mask;
}

static Bool
#if NeedFunctionPrototypes
XkbVMUpdateKeyType(XkbDescPtr xkb,int ndx,XkbChangesPtr changes)
#else
XkbVMUpdateKeyType(xkb,ndx,changes)
    XkbDescPtr		xkb;
    int			ndx;
    XkbChangesPtr	changes;
#endif
{
register unsigned i;
XkbKeyTypePtr	type;
CARD8		mask;

#ifdef DEBUG
    if (xkbDebugFlags>0)
	ErrorF("Updating key type %d due to virtual modifier change\n",ndx);
#endif
    type= &xkb->map->types[ndx];
    type->mods.mask= type->mods.real_mods|XkbMaskForVMask(xkb,type->mods.vmods);
    if ((type->map_count>0)&&(type->mods.vmods!=0)) {
	XkbKTMapEntryPtr entry;
	for (i=0,entry=type->map;i<type->map_count;i++,entry++) {
	    if (entry->mods.vmods!=0) {
		unsigned tmp;
		tmp= XkbMaskForVMask(xkb,entry->mods.vmods);
		entry->mods.mask=entry->mods.real_mods|tmp;
		/* entry is active if vmods are bound*/
	    	entry->active= (tmp!=0);
	    }
	    else entry->active= 1;
	}
    }
    if (changes) {
	if (changes->map.changed&XkbKeyTypesMask) {
	    unsigned first,last;
	    first= last= i;
	    if (changes->map.first_type<first)
		first= changes->map.first_type;
	    if ((changes->map.first_type+changes->map.num_types-1)>last)
		last= changes->map.first_type+changes->map.num_types-1;
	    changes->map.first_type= first;
	    changes->map.num_types= last-first+1;
	}
	else {
	    changes->map.changed|= XkbKeyTypesMask;
	    changes->map.first_type= i;
	    changes->map.num_types= 1;
	}
    }
    return TRUE;
}

Bool
XkbApplyVirtualModChanges(xkbi,changed,changes,needChecksRtrn)
    XkbSrvInfoPtr	 xkbi;
    unsigned		 changed;
    XkbChangesPtr	 changes;
    unsigned *		 needChecksRtrn;
{
register unsigned 	i,n,bit;
int 			lowChange,highChange;
XkbDescPtr		xkb;
XkbCompatMapPtr		compat;
unsigned		check;

    xkb= xkbi->desc;
    check= 0;
#ifdef DEBUG
    for (i=0,bit=1;i<XkbNumVirtualMods;i++,bit<<=1) {
	if ((changed&bit)==0)
	    continue;
	if (xkbDebugFlags)
	    ErrorF("Should be applying: change vmod %d to 0x%x\n",i,
					xkb->server->vmods[i]);
    }
#endif
    for (i=0;i<xkb->map->num_types;i++) {
	if (xkb->map->types[i].mods.vmods & changed)
	    XkbVMUpdateKeyType(xkb,i,changes);
    }

    if (changed&xkb->ctrls->internal.vmods) {
	CARD8 newMask;
	XkbControlsPtr ctrl= xkb->ctrls;

	newMask= XkbMaskForVMask(xkb,ctrl->internal.vmods);
	newMask|= ctrl->internal.real_mods;
	if (newMask!=ctrl->internal.mask) {
	    ctrl->internal.mask= newMask;
	    changes->ctrls.changed_ctrls|= XkbInternalModsMask;
	    check|= XkbStateNotifyMask;
	}
    }
    if (changed&xkb->ctrls->ignore_lock.vmods) {
	CARD8 newMask;
	XkbControlsPtr ctrl= xkb->ctrls;

	newMask= XkbMaskForVMask(xkb,ctrl->ignore_lock.vmods);
	newMask|= ctrl->ignore_lock.real_mods;
	if (newMask!=ctrl->ignore_lock.mask) {
	    ctrl->ignore_lock.mask= newMask;
	    changes->ctrls.changed_ctrls|= XkbIgnoreLockModsMask;
	    check|= XkbStateNotifyMask;
	}
    }
    for (i=0;i<XkbNumIndicators;i++) {
	if (xkb->indicators->maps[i].mods.vmods&changed) {
	    CARD8 newMask;
	    XkbIndicatorMapPtr map= &xkb->indicators->maps[i];

	    newMask= XkbMaskForVMask(xkb,map->mods.vmods)|map->mods.real_mods;
	    if (newMask!=map->mods.mask)
		map->mods.mask= newMask;
	    changes->indicators.map_changes|= (1<<i);
	}
	check|= XkbIndicatorStateNotify;
    }
    compat= xkb->compat;
    for (i=0;i<XkbNumKbdGroups;i++) {
	if (compat->groups[i].vmods&changed) {
	    CARD8 newMask;
	    newMask= XkbMaskForVMask(xkb,compat->groups[i].vmods);
	    compat->groups[i].mask= (newMask|compat->groups[i].real_mods);
	    changes->compat.changed_groups|= (1<<i);
	}
	check|= XkbIndicatorStateNotify;
	/* 7/12/95 (ef) - XXX! If group compatibility maps change, don't */
	/*               forget to change the compatibility state        */
    }
    lowChange= -1;
    for (i=xkb->min_key_code;i<=xkb->max_key_code;i++) {
	if (XkbKeyHasActions(xkb,i)) {
	    register XkbAction *pAct;
	    pAct= XkbKeyActionsPtr(xkb,i);
	    for (n=XkbKeyNumActions(xkb,i);n>0;n--,pAct++) {
		register unsigned tmp;
		switch (pAct->type) {
		    case XkbSA_SetMods: case XkbSA_LatchMods: case XkbSA_LockMods:
			if (((tmp= XkbModActionVMods(&pAct->mods))!=0)&&
							((tmp&changed)!=0)) {
			    pAct->mods.mask= pAct->mods.real_mods;
			    pAct->mods.mask|= XkbMaskForVMask(xkb,tmp);
			    if (lowChange<0)	lowChange= i;
			    highChange= i;
			}
			break;
		    case XkbSA_ISOLock:
			if (((tmp= XkbModActionVMods(&pAct->iso))!=0)&&
							((tmp&changed)!=0)) {
			    pAct->iso.mask= pAct->iso.real_mods;
			    pAct->iso.mask|= XkbMaskForVMask(xkb,tmp);
			    if (lowChange<0)	lowChange= i;
			    highChange= i;
			}
			break;
		}
	    }
	}
    }
    if (lowChange>0) {	/* something changed */
	if (changes->map.changed&XkbKeyActionsMask) {
	    i= changes->map.first_key_act;
	    if (i<lowChange)
		lowChange= i;
	    i+= changes->map.num_key_acts-1;
	    if (i>highChange)
		highChange= i;
	}
	changes->map.changed|= XkbKeyActionsMask;
	changes->map.first_key_act= lowChange;
	changes->map.num_key_acts= (highChange-lowChange)+1;
    }
    if (check&XkbStateNotifyMask)
	check|= XkbIndicatorStateNotifyMask;
    if (needChecksRtrn!=NULL) 
	*needChecksRtrn= check;
    else {
	/* 7/12/95 (ef) -- XXX check compatibility and/or indicator state */
    }
    return 1;
}

/***====================================================================***/

unsigned
XkbIndicatorsToUpdate(keybd,modsChanged)
    DeviceIntRec		*keybd;
    unsigned long	 	 modsChanged;
{
register unsigned	update=	0;
XkbSrvInfoPtr		xkbi = keybd->key->xkbInfo;

    if (modsChanged&(XkbModifierStateMask|XkbGroupStateMask))
	update|= xkbi->iAccel.usesEffective;
    if (modsChanged&(XkbModifierBaseMask|XkbGroupBaseMask))
	update|= xkbi->iAccel.usesBase;
    if (modsChanged&(XkbModifierLatchMask|XkbGroupLatchMask))
	update|= xkbi->iAccel.usesLatched;
    if (modsChanged&(XkbModifierLockMask|XkbGroupLockMask))
	update|= xkbi->iAccel.usesLocked;
    if (modsChanged&XkbCompatStateMask)
	update|= xkbi->iAccel.usesCompat;
    return update;
}

Bool
XkbApplyLEDChangeToKeyboard(xkbi,map,on,change)
    XkbSrvInfoPtr	xkbi;
    XkbIndicatorMapPtr	map;
    Bool		on;
    XkbChangesPtr	change;
{
Bool		ctrlChange,stateChange;
XkbStatePtr	state;

    if ((map->flags&XkbIM_NoExplicit)||((map->flags&XkbIM_LEDDrivesKB)==0))
	return False;
    ctrlChange= stateChange= False;
    if (map->ctrls) {
	XkbControlsPtr	ctrls= xkbi->desc->ctrls;
	unsigned 	old;

	old= ctrls->enabled_ctrls;
	if (on)	ctrls->enabled_ctrls|= map->ctrls;
	else	ctrls->enabled_ctrls&= ~map->ctrls;
	if (old!=ctrls->enabled_ctrls) {
	    change->ctrls.changed_ctrls= XkbControlsEnabledMask;
	    change->ctrls.enabled_ctrls_changes= old^ctrls->enabled_ctrls;
	    ctrlChange= True;
	}
    }
    state= &xkbi->state;
    if ((map->groups)&&((map->which_groups&(~XkbIM_UseBase))!=0)) {
	register int i;
	register unsigned bit,match;

	if (on)	match= (map->groups)&XkbAllGroupsMask;
	else 	match= (~map->groups)&XkbAllGroupsMask;
	if (map->which_groups&(XkbIM_UseLocked|XkbIM_UseEffective)) {
	    for (i=0,bit=1;i<XkbNumKbdGroups;i++,bit<<=1) {
		if (bit&match)
		    break;
	    }
	    if (map->which_groups&XkbIM_UseLatched)
		XkbLatchGroup(xkbi->device,0); /* unlatch group */
	    state->locked_group= i;
	    stateChange= True;
	}
	else if (map->which_groups&(XkbIM_UseLatched|XkbIM_UseEffective)) {
	    for (i=0,bit=1;i<XkbNumKbdGroups;i++,bit<<=1) {
		if (bit&match)
		    break;
	    }
	    state->locked_group= 0;
	    XkbLatchGroup(xkbi->device,i);
	    stateChange= True;
	}
    }
    if ((map->mods.mask)&&((map->which_mods&(~XkbIM_UseBase))!=0)) {
	if (map->which_mods&(XkbIM_UseLocked|XkbIM_UseEffective)) {
	    register unsigned long old;
	    old= state->locked_mods;
	    if (on)	state->locked_mods|= map->mods.mask;
	    else	state->locked_mods&= ~map->mods.mask;
	    if (state->locked_mods!=old)
		stateChange= True;
	}
	if (map->which_mods&(XkbIM_UseLatched|XkbIM_UseEffective)) {
	    register unsigned long newmods;
	    newmods= state->latched_mods;
	    if (on)	newmods|=  map->mods.mask;
	    else	newmods&= ~map->mods.mask;
	    if (newmods!=state->locked_mods) {
		newmods&= map->mods.mask;
		XkbLatchModifiers(xkbi->device,map->mods.mask,newmods);
		stateChange= True;
	    }
	}
    }
    return (stateChange || ctrlChange);
}

void
XkbSetIndicators(keybd,affect,values,pChanges,pCause)
    DeviceIntPtr	keybd;
    CARD32		affect;
    CARD32		values;
    XkbChangesPtr	pChanges;
    XkbEventCausePtr	pCause;
{
XkbSrvInfoPtr		xkbi=	keybd->key->xkbInfo;
XkbChangesRec		changes;
XkbEventCauseRec	cause;
register CARD32		led_changes;

    if (pChanges==NULL) {
	pChanges= &changes;
	bzero((char *)pChanges,sizeof(XkbChangesRec));
    }
    if (pCause==NULL) {
	pCause= &cause;
	XkbSetCauseCoreReq(pCause,X_ChangeKeyboardMapping);
    }
    xkbi->iStateExplicit&= ~affect;
    xkbi->iStateExplicit|= (affect&values);

    led_changes= (xkbi->iStateEffective&affect);
    led_changes^= (affect&values);
    if (led_changes) {
	register int 		i;
	register unsigned	bit;
	CARD8			flags;
	XkbStateRec		old;
	XkbIndicatorPtr	map;
	old= xkbi->state;
	map= xkbi->desc->indicators;
	for (i=0,bit=1;i<XkbNumIndicators;i++,bit<<=1) {
	    Bool on;
	    if ((led_changes&bit)==0)
		continue;
	    flags= map->maps[i].flags;
	    if ((flags&XkbIM_NoExplicit)||((flags&XkbIM_LEDDrivesKB)==0))
		continue;
	    on= ((values&bit)!=0);
	    if (XkbApplyLEDChangeToKeyboard(xkbi,&map->maps[i],on,pChanges)) {
		XkbComputeDerivedState(xkbi);
		pChanges->state_changes|= 
					XkbStateChangedFlags(&old,&xkbi->state);
	    }
	    xkbi->iStateExplicit&= ~bit;
	}
    }
    XkbUpdateIndicators(keybd,affect,&pChanges->indicators);
    if (pChanges==&changes)
	XkbSendNotification(keybd,pChanges,pCause);
    return;
}

void
XkbUpdateIndicators(keybd,update,pChanges)
    DeviceIntPtr		keybd;
    register CARD32		update;
    XkbIndicatorChangesPtr	pChanges;
{
register int	i,bit;
XkbSrvInfoPtr	xkbi= keybd->key->xkbInfo;
XkbStatePtr	kbdState = &xkbi->state;
CARD32		oldState;
unsigned	changedLEDs;


    oldState= xkbi->iStateEffective;
    for (i=0,bit=1;update;i++,bit<<=1) {
	if (update&bit) {
	    int on;
	    CARD8 		mods,group;
	    XkbIndicatorMapPtr	map= &xkbi->desc->indicators->maps[i];
	    on= mods= group= 0;
	    if (map->which_mods&XkbIM_UseBase)
		mods|= kbdState->base_mods;
	    if (map->which_groups&XkbIM_UseBase)
		group|= (1L << kbdState->base_group);

	    if (map->which_mods&XkbIM_UseLatched)
		mods|= kbdState->latched_mods;
	    if (map->which_groups&XkbIM_UseLatched)
		group|= (1L << kbdState->latched_group);

	    if (map->which_mods&XkbIM_UseLocked)
		mods|= kbdState->locked_mods;
	    if (map->which_groups&XkbIM_UseLocked)
		group|= (1L << kbdState->locked_group);

	    if (map->which_mods&XkbIM_UseEffective)
		mods|= kbdState->mods;
	    if (map->which_groups&XkbIM_UseEffective)
		group|= (1L << kbdState->group);

	    if (map->which_mods&XkbIM_UseCompat)
		mods|= kbdState->compat_state;

	    if ((map->which_mods|map->which_groups)&XkbIM_UseAnyMods) {
		on = ((map->mods.mask&mods)!=0);
		on = on||((mods==0)&&(map->mods.mask==0)&&(map->mods.vmods==0));
		on = on&&(((map->groups&group)!=0)||(map->groups==0));
	    }
	    if (map->ctrls)
		on = on || (xkbi->desc->ctrls->enabled_ctrls&map->ctrls);

	    if (on)	xkbi->iStateAuto|= bit;
	    else	xkbi->iStateAuto&= ~bit;

	    if ((map->flags&XkbIM_NoExplicit)==0) {
		if ((map->flags&XkbIM_NoAutomatic)!=0)
		    on= FALSE;
		on= on||((xkbi->iStateExplicit&bit)!=0);
	    }

	    if (on)	xkbi->iStateEffective|= bit;
	    else	xkbi->iStateEffective&= ~bit;
	    update&= ~bit;
	}
    }
    changedLEDs= xkbi->iStateEffective^oldState;
    if (changedLEDs!=0) {
#ifdef DEBUG
	if (xkbDebugFlags&0x2) {
	    ErrorF("changedLEDs= 0x%x, effective= 0x%x\n",changedLEDs,
	    						xkbi->iStateEffective);
	}
#endif
	XkbDDXUpdateIndicators(keybd,oldState,xkbi->iStateEffective);
	if (pChanges)
	    pChanges->state_changes|= changedLEDs;
	else {
	    xkbIndicatorNotify	in;
	    in.changed= changedLEDs;
	    in.state = xkbi->iStateEffective;
	    XkbSendIndicatorNotify(keybd,XkbIndicatorStateNotify,&in);
	}
	if (XkbAX_NeedFeedback(xkbi->desc->ctrls,XkbAX_IndicatorFBMask)) {
	    unsigned on,off,beepType;
	    on= changedLEDs&xkbi->iStateEffective;
	    off= changedLEDs&(~on);

	    if (on && off)	beepType= _BEEP_LED_CHANGE;
	    else if (on)	beepType= _BEEP_LED_ON;
	    else if (off)	beepType= _BEEP_LED_OFF;
	    else		beepType= _BEEP_NONE;
	    if (beepType!=_BEEP_NONE)
		XkbDDXAccessXBeep(keybd,beepType,0);
	}
    }
    if (xkbi->device->kbdfeed)
	xkbi->device->kbdfeed->ctrl.leds= xkbi->iStateEffective;
    return;
}

void
XkbCheckIndicatorMaps(xkbi,which)
    XkbSrvInfoPtr	xkbi;
    unsigned		which;
{
register unsigned	i,bit;
XkbIndicatorPtr		leds;

    leds= xkbi->desc->indicators;
    for (i=0,bit=1;i<XkbNumIndicators;i++,bit<<=1) {
	if (which&bit) {
	    CARD8		what;
	    XkbIndicatorMapPtr	map= &leds->maps[i];

	    what= (map->which_mods|map->which_groups);
	    if (what&XkbIM_UseBase)
		 xkbi->iAccel.usesBase|= bit;
	    else xkbi->iAccel.usesBase&= ~bit;
	    if (what&XkbIM_UseLatched)
		 xkbi->iAccel.usesLatched|= bit;
	    else xkbi->iAccel.usesLatched&= ~bit;
	    if (what&XkbIM_UseLocked)
		 xkbi->iAccel.usesLocked|= bit;
	    else xkbi->iAccel.usesLocked&= ~bit;
	    if (what&XkbIM_UseEffective)
		 xkbi->iAccel.usesEffective|= bit;
	    else xkbi->iAccel.usesEffective&= ~bit;
	    if (what&XkbIM_UseCompat)
		 xkbi->iAccel.usesCompat|= bit;
	    else xkbi->iAccel.usesCompat&= ~bit;
	    if (map->ctrls)
		 xkbi->iAccel.usesControls|= bit;
	    else xkbi->iAccel.usesControls&= ~bit;

	    if (map->ctrls || 
		(map->which_groups && map->groups) ||
		(map->which_mods && map->mods.real_mods))
		 xkbi->iAccel.haveMap|= bit;
	    else xkbi->iAccel.haveMap&= ~bit;
	    if (map->mods.vmods!=0) {
		map->mods.mask= map->mods.real_mods;
		map->mods.mask|= XkbMaskForVMask(xkbi->desc,map->mods.vmods);
	    }
	}
    }
    xkbi->iAccel.usedComponents= 0;
    if (xkbi->iAccel.usesBase)
	xkbi->iAccel.usedComponents|= XkbModifierBaseMask|XkbGroupBaseMask;
    if (xkbi->iAccel.usesLatched)
	xkbi->iAccel.usedComponents|= XkbModifierLatchMask|XkbGroupLatchMask;
    if (xkbi->iAccel.usesLocked)
	xkbi->iAccel.usedComponents|= XkbModifierLockMask|XkbGroupLockMask;
    if (xkbi->iAccel.usesEffective)
	xkbi->iAccel.usedComponents|= XkbModifierStateMask|XkbGroupStateMask;
    if (xkbi->iAccel.usesCompat)
	xkbi->iAccel.usedComponents|= XkbCompatStateMask;
    return;
}

#if NeedFunctionPrototypes
void
XkbUpdateKeyTypesFromCore(	DeviceIntPtr	pXDev,
				KeyCode	 	first,
				CARD8	 	num,
				XkbChangesPtr	pChanges)
#else
void
XkbUpdateKeyTypesFromCore(pXDev,first,num,pChanges)
    DeviceIntPtr pXDev;
    KeyCode first;
    CARD8 num;
    XkbChangesPtr pChanges;
#endif
{
XkbDescPtr	xkb;
XkbSymMapPtr	map;
unsigned	key,nGroups,nLevels[XkbNumKbdGroups],width;
KeySymsPtr	pCore;
KeySym		*pSym,*newSyms;
KeySym		core[4];
int		i;

    xkb= pXDev->key->xkbInfo->desc;
    if (first<xkb->min_key_code) {
	if (first>=XkbMinLegalKeyCode) {
	    xkb->min_key_code= first;
#ifdef NOTYET
	    /* 1/12/95 (ef) -- XXX! should zero out the new maps */
	    pChanges->map.changed|= XkbKeycodesMask;
generate a NewKeyboard notify here?
#endif
	}
    }
    if (first+num-1>xkb->max_key_code) {
	/* 1/12/95 (ef) -- XXX! should allow XKB structures to grow */
	num= xkb->max_key_code-first+1;
    }

    pCore= &pXDev->key->curKeySyms;
    map= &xkb->map->key_sym_map[first];
    pSym= &pCore->map[(first-pCore->minKeyCode)*pCore->mapWidth];
    for (key=first; key<(first+num); key++,map++,pSym+= pCore->mapWidth) {
	core[0]= pSym[0];
	if (pCore->mapWidth>1)	core[1]= pSym[1];
	else			core[1]= NoSymbol;
	if (pCore->mapWidth>2)	core[2]= pSym[2];
	else			core[2]= NoSymbol;
	if (pCore->mapWidth>3)	core[3]= pSym[3];
	else			core[3]= NoSymbol;

	/* 3/24/94 (ef) -- XXX! Horrible kludge to fake core capitalization */
	if ((core[1]==NoSymbol)&&(core[0]!=NoSymbol)) {
	    KeySym upper,lower;
	    XkbConvertCase(core[0],&lower,&upper);
	    if ((core[0]==upper)&&(upper!=lower)) {
		core[1]= upper;
		core[0]= lower;
	    }
	}
	if ((core[3]==NoSymbol)&&(core[2]!=NoSymbol)) {
	    KeySym upper,lower;
	    XkbConvertCase(core[2],&lower,&upper);
	    if ((core[2]==upper)&&(upper!=lower)) {
		core[3]= upper;
		core[2]= lower;
	    }
	}

	bzero(nLevels,XkbNumKbdGroups*sizeof(unsigned));
	if (core[1]==NoSymbol)	nLevels[XkbGroup1Index]= 1;
	else 			nLevels[XkbGroup1Index]= 2;
	if (core[3]==NoSymbol)	nLevels[XkbGroup2Index]= 1;
	else 			nLevels[XkbGroup2Index]= 2;

	if ((core[2]==NoSymbol)&&(core[3]==NoSymbol)) {
	     nLevels[XkbGroup2Index]= 0;
	     if ((core[0]==NoSymbol)&&(core[1]==NoSymbol)) {
		  nGroups= 0;
		  nLevels[XkbGroup1Index]= 0;
	     }
	     else nGroups= 1;
	}
	else nGroups= 2;

	for (width=i=0;i<XkbNumKbdGroups;i++) {
	    if (xkb->server->explicit[key]&(1<<i)) {
		nLevels[i]= XkbKeyGroupWidth(xkb,key,i);
		if  (i>nGroups)
		    nGroups= i+1;
	    }
	    if (nLevels[i]>width)
		width= nLevels[i];
	}
	if (nGroups>0) {
	    KeySym syms[XkbMaxSymsPerKey];

	    if (XkbKeyNumSyms(xkb,key)>0) {
		memcpy(syms,XkbKeySymsPtr(xkb,key),
					XkbKeyNumSyms(xkb,key)*sizeof(KeySym));
	    }
	    newSyms= XkbResizeKeySyms(xkb,key,nGroups*width);
	    newSyms[0]= core[0];
	    if (nLevels[XkbGroup1Index]>1)
		newSyms[1]= core[1];
	    if (nGroups>1) {
		newSyms[width]= core[2];
		if (nLevels[XkbGroup2Index]>=2)
		    newSyms[width+1]= core[3];
		for (i=XkbGroup3Index;i<XkbNumKbdGroups;i++) {
		    memcpy(&newSyms[width*i],
				&syms[XkbKeyGroupsWidth(xkb,key)*i],
		    		nLevels[i]*sizeof(KeySym));
		}
	    }
	    if (XkbKeyHasActions(xkb,key)) {
		XkbAction *newActs,acts[XkbMaxSymsPerKey];
		newActs= XkbResizeKeyActions(xkb,key,nGroups*width);
		if (nGroups>1) {
		    register int g,oldWidth;
		    oldWidth= XkbKeyGroupsWidth(xkb,key)*i;
		    memcpy(acts,newActs,
				XkbKeyNumActions(xkb,key)*sizeof(XkbAction));
		    bzero(newActs,(nGroups*width)*sizeof(XkbAction));
		    for (g=0;g<nGroups;g++) {
			int ncopy;
			if (nLevels[g]>=XkbKeyGroupWidth(xkb,key,g))
			     ncopy= XkbKeyGroupWidth(xkb,key,g);
			else ncopy= nLevels[g];
			memcpy(&newActs[width*g],&acts[oldWidth*g],
						    ncopy*sizeof(XkbAction));
		    }
		}
	    }
	}
	if ((nGroups>1)&&(xkb->ctrls->num_groups<2)) {
	    xkb->ctrls->num_groups= nGroups;
	    pChanges->ctrls.num_groups_changed= True;
	}
	for (i=0;i<nGroups;i++) {
	    if ((xkb->server->explicit[key]&(1<<i))==0) {
		if (nLevels[i]<2)
		     map->kt_index[i]= XkbOneLevelIndex;
		else if (XkbKSIsLower(newSyms[i*width])&&
			 XkbKSIsUpper(newSyms[i*width+1]))
		     map->kt_index[i]= XkbAlphabeticIndex;
		else if (XkbKSIsKeypad(newSyms[i*width])||
			 XkbKSIsKeypad(newSyms[i*width+1]))
		     map->kt_index[i]= XkbKeypadIndex;
		else map->kt_index[i]= XkbTwoLevelIndex;
	    }
	}
	map->width= width;
	map->group_info = XkbSetNumGroups(map->group_info,nGroups);
    }
    if (pChanges->map.changed&XkbKeySymsMask) {
	CARD8 oldLast,newLast;
	oldLast = pChanges->map.first_key_sym+pChanges->map.num_key_syms-1;
	newLast = first+num-1;

	if (first<pChanges->map.first_key_sym)
	    pChanges->map.first_key_sym = first;
	if (oldLast>newLast)
	    newLast= oldLast;
	pChanges->map.num_key_syms = newLast-pChanges->map.first_key_sym+1;
    }
    else {
	pChanges->map.changed|= XkbKeySymsMask;
	pChanges->map.first_key_sym = first;
	pChanges->map.num_key_syms = num;
    }
    return;
}

/***====================================================================***/

static XkbSymInterpretPtr
_XkbFindMatchingInterp(xkb,sym,real_mods,level)
    XkbDescPtr	xkb;
    KeySym	sym;
    CARD8	real_mods;
    CARD8	level;
{
register unsigned	 i;
XkbSymInterpretPtr	 interp,rtrn;
CARD8			 mods;

    rtrn= NULL;
    interp= xkb->compat->sym_interpret;
    for (i=0;i<xkb->compat->num_si;i++,interp++) {
	if ((interp->sym==NoSymbol)||(sym==interp->sym)) {
	    int match;
	    if ((level==0)||((interp->match&XkbSI_LevelOneOnly)==0))
		 mods= real_mods;
	    else mods= 0;
	    switch (interp->match&XkbSI_OpMask) {
		case XkbSI_NoneOf:
		    match= ((interp->mods&mods)==0);
		    break;
		case XkbSI_AnyOfOrNone:
		    match= ((mods==0)||((interp->mods&mods)!=0));
		    break;
		case XkbSI_AnyOf:
		    match= ((interp->mods&mods)!=0);
		    break;
		case XkbSI_AllOf:
		    match= ((interp->mods&mods)==interp->mods);
		    break;
		case XkbSI_Exactly:
		    match= (interp->mods==mods);
		    break;
		default:
		    ErrorF("Illegal match in FindMatchingInterp\n");
		    match= 0;
		    break;
	    }
	    if (match) {
		if (interp->sym!=NoSymbol) {
		    return interp;
		}
		else if (rtrn==NULL) {
		    rtrn= interp;
		}
	    }
	}
    }
    return rtrn;
}

#if NeedFunctionPrototypes
void
XkbUpdateActions(	DeviceIntPtr	 pXDev,
			KeyCode		 first,
			CARD8		 num,
			XkbChangesPtr	 pChanges,
			unsigned *	 needChecksRtrn)
#else
void
XkbUpdateActions(pXDev,first,num,pChanges,needChecksRtrn)
    DeviceIntPtr 	pXDev;
    KeyCode 		first;
    CARD8 		num;
    XkbChangesPtr 	pChanges;
    unsigned *		needChecksRtrn;
#endif
{
XkbSrvInfoPtr		xkbi;
XkbDescPtr		xkb;
register unsigned	i,key,n,interpSize;
unsigned	 	nSyms,found;
KeySym	*		pSym;
XkbSymInterpretPtr	*interps,ibuf[8]; 
CARD8		 	mods,*repeat,explicit;
unsigned char		origVMods[XkbNumVirtualMods];
CARD16		 	changedVMods;

    if (needChecksRtrn)
	*needChecksRtrn= 0;
    xkbi= pXDev->key->xkbInfo;
    xkb= xkbi->desc;
    repeat= xkb->ctrls->per_key_repeat;
    changedVMods= 0;
    memcpy(origVMods,xkb->server->vmods,XkbNumVirtualMods);
    if (pXDev->kbdfeed)
	memcpy(repeat,pXDev->kbdfeed->ctrl.autoRepeats,32);
    interpSize= 8;
    interps= ibuf;
    for (key=first;key<(first+num);key++) {
	if ((xkb->server->explicit[key]&XkbExplicitInterpretMask)!=0) {
	    if (key==first) {
		first++; num--;	/* don't report unchanged keys */
	    }
	    continue;
	}
	explicit= xkb->server->explicit[key];
	mods= pXDev->key->modifierMap[key];
	pSym= XkbKeySymsPtr(xkb,key);
	nSyms= XkbKeyNumSyms(xkb,key);
	found= 0;
	if (nSyms>interpSize) {
	    if (interps==ibuf) {
		 interps= (XkbSymInterpretPtr *)
			Xcalloc(nSyms*sizeof(XkbSymInterpretPtr));
	    }
	    else interps= (XkbSymInterpretPtr *)
			Xrealloc(interps,nSyms*sizeof(XkbSymInterpretPtr));
	    if (interps==NULL) {
		ErrorF("Couldn't allocate room for symbol interps\n");
		ErrorF("Some actions may be incorrect\n");
		nSyms= interpSize= 8;
		interps= ibuf;
	    }
	}
	for (n=0;n<nSyms;n++,pSym++) {
	    CARD8 level= (n%XkbKeyGroupsWidth(xkb,key));
	    interps[n]= NULL;
	    if (*pSym!=NoSymbol) {
		interps[n]= _XkbFindMatchingInterp(xkb,*pSym,mods,level);
		if (interps[n]&&interps[n]->act.type!=XkbSA_NoAction)
		     found++;
		else interps[n]= NULL;
	    }
	}
	if (!found) {
	    xkb->server->key_acts[key]= 0;
	}
	else {
	    XkbAction *pActs;
	    pActs= XkbResizeKeyActions(xkb,key,nSyms);
	    for (i=0;i<nSyms;i++) {
		if (interps[i]) {
		    unsigned effMods;

		    pActs[i]= *((XkbAction *)&interps[i]->act);
		    if ((i==0)||((interps[i]->match&XkbSI_LevelOneOnly)==0))
			 effMods= mods;
		    else effMods= 0;
		    XkbSetActionKeyMods(xkb,&pActs[i],effMods);
		}
		else pActs[i].type= XkbSA_NoAction;
	    }
	    if ((explicit&XkbExplicitVModMapMask)==0) {
		changedVMods|= xkb->server->vmodmap[key];
		xkb->server->vmodmap[key]= 0;
	    }
	    if (interps[0]) {
		if ((interps[0]->flags&XkbSI_LockingKey)&&
		    ((explicit&XkbExplicitBehaviorMask)==0)) {
		    xkb->server->behaviors[key].type= XkbKB_Lock;
		}
		if ((explicit&XkbExplicitAutoRepeatMask)==0) {
		    if (interps[0]->flags&XkbSI_AutoRepeat)
			 repeat[key/8]|= (1<<(key%8));
		    else repeat[key/8]&= ~(1<<(key%8));
		}
		if ((interps[0]->virtual_mod!=XkbNoModifier)&&
				((explicit&XkbExplicitVModMapMask)==0)) {
		    if (pChanges) {
			if (pChanges->map.changed&XkbVirtualModMapMask) {
			    CARD8 oldLast,newLast;
			    oldLast= pChanges->map.first_vmodmap_key+
					pChanges->map.num_vmodmap_keys-1;
			    newLast = key;

			    if (key<pChanges->map.first_vmodmap_key)
				pChanges->map.first_vmodmap_key = key;
			    if (newLast>oldLast)
				newLast= oldLast;
			    pChanges->map.num_vmodmap_keys= 
				newLast-pChanges->map.first_vmodmap_key+1;
			}
			else {
			    pChanges->map.changed|= XkbVirtualModMapMask;
			    pChanges->map.first_vmodmap_key= key;
			    pChanges->map.num_vmodmap_keys= 1;
			}
		    }
		    changedVMods|= (1<<interps[0]->virtual_mod);
		    xkb->server->vmodmap[key]|= (1<<interps[0]->virtual_mod);
		}
	    }
	}
	if ((!found)||(interps[0]==NULL)) {
	    if ((explicit&XkbExplicitAutoRepeatMask)==0) {
#if RETURN_SHOULD_REPEAT
		if (*XkbKeySymsPtr(xkb,key) != XK_Return)
#endif
		repeat[key/8]|= (1<<(key%8));
	    }
	    if (((explicit&XkbExplicitBehaviorMask)==0)&&
		(xkb->server->behaviors[key].type==XkbKB_Lock)) {
		xkb->server->behaviors[key].type= XkbKB_Default;
	    }
	}
    }

    if (pXDev->kbdfeed) {
        memcpy(pXDev->kbdfeed->ctrl.autoRepeats,repeat, 32);
	(*pXDev->kbdfeed->CtrlProc)(pXDev, &pXDev->kbdfeed->ctrl);
    }
    if (changedVMods) {
	register unsigned bit;
	bzero(xkb->server->vmods,XkbNumVirtualMods);
	if (xkb->server->vmodmap) {
	    for (key=xkb->min_key_code;key<=xkb->max_key_code;key++) {
		if ((xkb->server->vmodmap[key]==0)||(xkb->map->modmap[key]==0))
		    continue;
		for (i=0,bit=1;i<XkbNumVirtualMods;i++,bit<<=1) {
		    if (bit&xkb->server->vmodmap[key])
			xkb->server->vmods[i]|= xkb->map->modmap[key];
		}
	    }
	}
	changedVMods= 0;
	for (i=0,bit=1;i<XkbNumVirtualMods;i++,bit<<=1) {
	    if (origVMods[i]!=xkb->server->vmods[i])
		changedVMods|= bit;
	}
	if (changedVMods)
	   XkbApplyVirtualModChanges(xkbi,changedVMods,pChanges,needChecksRtrn);
    }
    if (pChanges->map.changed&XkbKeyActionsMask) {
	CARD8 oldLast,newLast;
	oldLast= pChanges->map.first_key_act+pChanges->map.num_key_acts-1;
	newLast = first+num-1;

	if (first<pChanges->map.first_key_act)
	    pChanges->map.first_key_act = first;
	if (newLast>oldLast)
	    newLast= oldLast;
	pChanges->map.num_key_acts= newLast-pChanges->map.first_key_act+1;
    }
    else {
	pChanges->map.changed|= XkbKeyActionsMask;
	pChanges->map.first_key_act = first;
	pChanges->map.num_key_acts = num;
    }
    if (interps!=ibuf)
	Xfree(interps);
    return;
}

void
XkbUpdateCoreDescription(keybd)
    DeviceIntPtr keybd;
{
register int		key,tmp;
int			maxSymsPerKey,maxKeysPerMod;
int			first,last,firstCommon,lastCommon;
XkbDescPtr		xkb;
KeyClassPtr		keyc;
CARD8			keysPerMod[XkbNumModifiers];

    if (!keybd || !keybd->key || !keybd->key->xkbInfo)
	return;
    xkb= keybd->key->xkbInfo->desc;
    keyc= keybd->key;
    maxSymsPerKey= maxKeysPerMod= 0;
    bzero(keysPerMod,sizeof(keysPerMod));
    memcpy(keyc->modifierMap,xkb->map->modmap,xkb->max_key_code+1);
    if (xkb->min_key_code<keyc->curKeySyms.minKeyCode) {
	 first= xkb->min_key_code;
	 firstCommon= keyc->curKeySyms.minKeyCode;
    }
    else {
	firstCommon= xkb->min_key_code;
	first= keyc->curKeySyms.minKeyCode;
    }
    if (xkb->max_key_code>keyc->curKeySyms.maxKeyCode) {
	 lastCommon= keyc->curKeySyms.maxKeyCode;
	 last= xkb->max_key_code;
    }
    else {
	lastCommon= xkb->max_key_code;
	last= keyc->curKeySyms.maxKeyCode;
    }

    /* determine sizes */
    for (key=first;key<=last;key++) {
	if (XkbKeycodeInRange(xkb,key)) {
	    tmp= XkbKeyNumSyms(xkb,key);
	    if (tmp>maxSymsPerKey)
		maxSymsPerKey= tmp;
	}
	if (_XkbCoreKeycodeInRange(keyc,key)) {
	    if (keyc->modifierMap[key]!=0) {
		register unsigned bit,i,mask;
		mask= keyc->modifierMap[key];
		for (i=0,bit=1;i<XkbNumModifiers;i++,bit<<=1) {
		    if (mask&bit) {
			keysPerMod[i]++;
			if (keysPerMod[i]>maxKeysPerMod)
			    maxKeysPerMod= keysPerMod[i];
		    }
		}
	    }
	}
    }

    if (maxKeysPerMod>0) {
	tmp= maxKeysPerMod*XkbNumModifiers;
	if (keyc->modifierKeyMap==NULL)
	    keyc->modifierKeyMap= (KeyCode *)Xcalloc(tmp);
	else if (keyc->maxKeysPerModifier<maxKeysPerMod)
	    keyc->modifierKeyMap= (KeyCode *)Xrealloc(keyc->modifierKeyMap,tmp);
	if (keyc->modifierKeyMap==NULL)
	    FatalError("Couldn't allocate modifierKeyMap in UpdateCore\n");
	bzero(keyc->modifierKeyMap,tmp);
    }
    else if ((keyc->maxKeysPerModifier>0)&&(keyc->modifierKeyMap!=NULL)) {
	Xfree(keyc->modifierKeyMap);
	keyc->modifierKeyMap= NULL;
    }
    keyc->maxKeysPerModifier= maxKeysPerMod;

    if (maxSymsPerKey>0) {
	tmp= maxSymsPerKey*_XkbCoreNumKeys(keyc);
	keyc->curKeySyms.map= _XkbTypedRealloc(keyc->curKeySyms.map,tmp,KeySym);
	if (keyc->curKeySyms.map==NULL)
	    FatalError("Couldn't allocate symbols map in UpdateCore\n");
    }
    else if ((keyc->curKeySyms.mapWidth>0)&&(keyc->curKeySyms.map!=NULL)) {
	_XkbFree(keyc->curKeySyms.map);
	keyc->curKeySyms.map= NULL;
    }
    keyc->curKeySyms.mapWidth= maxSymsPerKey;

    bzero(keysPerMod,sizeof(keysPerMod));
    for (key=firstCommon;key<=lastCommon;key++) {
	if (keyc->curKeySyms.map!=NULL) {
	    KeySym *pCore,*pXKB;
	    unsigned nGroups,groupWidth;

	    nGroups= XkbKeyNumGroups(xkb,key);
	    groupWidth= XkbKeyGroupsWidth(xkb,key);
	    tmp= (key-keyc->curKeySyms.minKeyCode)*maxSymsPerKey;
	    pCore= &keyc->curKeySyms.map[tmp];
	    bzero(pCore,maxSymsPerKey*sizeof(KeySym));
	    pXKB= XkbKeySymsPtr(xkb,key);
	    for (tmp=0;tmp<nGroups;tmp++) {
		*pCore++= *pXKB++;
		if (groupWidth>1) {
		    *pCore++= *pXKB++;
		    pXKB+= groupWidth-2;
		}
		else if (maxSymsPerKey>1)
		    *pCore++= NoSymbol;
	    }
	    if (groupWidth>2) {
		register unsigned level;
		pXKB= XkbKeySymsPtr(xkb,key);
		for (tmp=0;tmp<nGroups;tmp++) {
		    for (level=2;level<groupWidth;level++) {
			*pCore++= pXKB[level];
		    }
		    pXKB+= groupWidth;
		}
	    }
	}
	if (keyc->modifierMap[key]!=0) {
	    register unsigned bit,i,mask;
	    mask= keyc->modifierMap[key];
	    for (i=0,bit=1;i<XkbNumModifiers;i++,bit<<=1) {
		if (mask&bit) {
		    tmp= i*maxKeysPerMod+keysPerMod[i];
		    keyc->modifierKeyMap[tmp]= key;
		    keysPerMod[i]++;
		}
	    }
	}
    }
#ifdef MODE_SWITCH
    /* Fix up any of the KME stuff if we changed the core description.
     */
    if (!noKME)
	HandleKeyBinding(keyc, &keyc->curKeySyms);
#endif
    return;
}

void
XkbSetRepeatKeys(pXDev,key,onoff)
    DeviceIntPtr pXDev;
    int		 key;
    int		 onoff;
{
    if (pXDev && pXDev->key && pXDev->key->xkbInfo) {
	xkbControlsNotify	cn;
	XkbControlsPtr		ctrls = pXDev->key->xkbInfo->desc->ctrls;
	XkbControlsRec 		old;
	old = *ctrls;

	if (key== -1) {	/* global autorepeat setting changed */
	    if (onoff)	ctrls->enabled_ctrls |= XkbRepeatKeysMask;
	    else	ctrls->enabled_ctrls &= ~XkbRepeatKeysMask;
	}
	else if (pXDev->kbdfeed) {
	    ctrls->per_key_repeat[key/8] = 
		pXDev->kbdfeed->ctrl.autoRepeats[key/8];
	}
	
	if (XkbComputeControlsNotify(pXDev,&old,ctrls,&cn,True))
	    XkbSendControlsNotify(pXDev,&cn);
    }
    return;
}

#if NeedFunctionPrototypes
void
XkbApplyMappingChange(	DeviceIntPtr	kbd,
			CARD8		 request,
			KeyCode		 firstKey,
			CARD8		 num)
#else
void
XkbApplyMappingChange(kbd,request,firstKey,num)
    DeviceIntPtr kbd;
    CARD8 request;
    KeyCode firstKey;
    CARD8 num;
#endif
{
XkbEventCauseRec	cause;
XkbChangesRec	 	changes;
int		 	req;
unsigned	 	check;

    if (kbd->key->xkbInfo==NULL)
	XkbInitDevice(kbd);
    bzero(&changes,sizeof(XkbChangesRec));
    check= 0;
    if (request==MappingKeyboard) {
	XkbUpdateKeyTypesFromCore(kbd,firstKey,num,&changes);
	XkbUpdateActions(kbd,firstKey,num,&changes,&check);
	if (check)
	    XkbCheckSecondaryEffects(kbd->key->xkbInfo,check,&changes);
	req= X_ChangeKeyboardMapping;
    }
    else if (request==MappingModifier) {
	XkbDescPtr	xkb= kbd->key->xkbInfo->desc;
	num = xkb->max_key_code-xkb->min_key_code+1;
	memcpy(xkb->map->modmap,kbd->key->modifierMap,xkb->max_key_code+1);

	changes.map.changed|= XkbModifierMapMask;
	changes.map.first_modmap_key= xkb->min_key_code;
	changes.map.num_modmap_keys= num;
	XkbUpdateActions(kbd,xkb->min_key_code,num,&changes,&check);
	if (check)
	    XkbCheckSecondaryEffects(kbd->key->xkbInfo,check,&changes);
	req= X_SetModifierMapping;
    }
    /* 3/26/94 (ef) -- XXX! Doesn't deal with input extension requests */
    XkbSetCauseCoreReq(&cause,req);
    XkbSendNotification(kbd,&changes,&cause);
    return;
}

void
XkbDisableComputedAutoRepeats(dev,key)
    DeviceIntPtr	dev;
    unsigned		key;
{
XkbSrvInfoPtr	xkbi = dev->key->xkbInfo;
xkbMapNotify	mn;

    xkbi->desc->server->explicit[key]|= XkbExplicitAutoRepeatMask;
    bzero(&mn,sizeof(mn));
    mn.changed= XkbExplicitComponentsMask;
    mn.firstKeyExplicit= key;
    mn.nKeyExplicit= 1;
    XkbSendMapNotify(dev,&mn);
    return;
}

unsigned
XkbStateChangedFlags(old,new)
    XkbStatePtr	old;
    XkbStatePtr	new;
{
int		groupUnlatch,changed;

    changed=(old->group!=new->group?XkbGroupStateMask:0);
    changed|=(old->base_group!=new->base_group?XkbGroupBaseMask:0);
    changed|=(old->latched_group!=new->latched_group?XkbGroupLatchMask:0);
    changed|=(old->locked_group!=new->locked_group?XkbGroupLockMask:0);
    changed|=(old->mods!=new->mods?XkbModifierStateMask:0);
    changed|=(old->base_mods!=new->base_mods?XkbModifierBaseMask:0);
    changed|=(old->latched_mods!=new->latched_mods?XkbModifierLatchMask:0);
    changed|=(old->locked_mods!=new->locked_mods?XkbModifierLockMask:0);
    changed|=(old->compat_state!=new->compat_state?XkbCompatStateMask:0);
    changed|=(old->grab_mods!=new->grab_mods?XkbGrabModsMask:0);
    if (old->compat_grab_mods!=new->compat_grab_mods)
	changed|= XkbCompatGrabModsMask;
    changed|=(old->lookup_mods!=new->lookup_mods?XkbLookupModsMask:0);
    if (old->compat_lookup_mods!=new->compat_lookup_mods)
	changed|= XkbCompatLookupModsMask;
    changed|=(old->ptr_buttons!=new->ptr_buttons?XkbPointerButtonMask:0);
    return changed;
}

void
XkbComputeCompatState(xkbi)
    XkbSrvInfoPtr	xkbi;
{
register int 	i,bit;
CARD16 		grp_mask;
XkbStatePtr	state= &xkbi->state;
XkbCompatMapPtr	map;

    map= xkbi->desc->compat;
    grp_mask= map->groups[state->group].mask;
    state->compat_state = state->mods|grp_mask;
    state->compat_lookup_mods= state->lookup_mods|grp_mask;

    if (xkbi->desc->ctrls->enabled_ctrls&XkbIgnoreGroupLockMask)
	 grp_mask= map->groups[state->base_group].mask;
    state->compat_grab_mods= state->grab_mods|grp_mask;
    return;
}

static void
#if NeedFunctionPrototypes
_XkbAdjustGroup(CARD8 *pGroup,unsigned nGroups,unsigned act,unsigned newGroup)
#else
_XkbAdjustGroup(pGroup,nGroups,act,newGroup)
    CARD8 *	pGroup;
    unsigned	nGroups;
    unsigned	act;
    unsigned	newGroup;
#endif
{
register int group = XkbCharToInt(*pGroup);

    if (group<0) {
	while ( group < 0 )  {
	    if (act==XkbClampIntoRange) {
		group= 0;
	    }
	    else if (act==XkbRedirectIntoRange) {
		if (newGroup>=nGroups)
		     group= 0;
		else group= newGroup;
	    }
	    else {
		group+= nGroups;
	    }
	}
    }
    else if (((unsigned)group)>=nGroups) {
	if (act==XkbClampIntoRange) {
	    group= nGroups-1;
	}
	else if (act==XkbRedirectIntoRange) {
	    if (newGroup>=nGroups)
		 group= 0;
	    else group= newGroup;
	}
	else {
	    group%= nGroups;
	}
    }
    *pGroup= group;
    return;
}

void
XkbComputeDerivedState(xkbi)
    XkbSrvInfoPtr	xkbi;
{
XkbStatePtr	state= &xkbi->state;
XkbControlsPtr	ctrls= xkbi->desc->ctrls;
unsigned	act,newGroup;

    act= XkbOutOfRangeGroupAction(ctrls->groups_wrap);
    newGroup= XkbOutOfRangeGroupNumber(ctrls->groups_wrap);
    state->mods= (state->base_mods|state->latched_mods);
    state->mods|= state->locked_mods;
    state->lookup_mods= state->mods&(~ctrls->internal.mask);
    state->grab_mods= state->lookup_mods&(~ctrls->ignore_lock.mask);
    state->grab_mods|= 
		(state->base_mods|state->latched_mods&ctrls->ignore_lock.mask);

    _XkbAdjustGroup(&state->locked_group,ctrls->num_groups,act,newGroup);

    state->group = state->base_group+state->latched_group;
    state->group+= state->locked_group;
    _XkbAdjustGroup(&state->group,ctrls->num_groups,act,newGroup);

    XkbComputeCompatState(xkbi);
    return;
}

/***====================================================================***/

void
XkbCheckSecondaryEffects(xkbi,which,changes)
    XkbSrvInfoPtr	xkbi;
    unsigned		which;
    XkbChangesPtr	changes;
{
    if (which&XkbStateNotifyMask) {
	XkbStateRec old;
	old= xkbi->state;
	changes->state_changes|= XkbStateChangedFlags(&old,&xkbi->state);
	XkbComputeDerivedState(xkbi);
    }
    if (which&XkbIndicatorStateNotifyMask)
	XkbUpdateIndicators(xkbi->device,XkbAllIndicatorsMask,
							&changes->indicators);
    return;
}

/***====================================================================***/

void
XkbSetPhysicalLockingKey(dev,key)
   DeviceIntPtr	dev;
   unsigned	key;
{
XkbDescPtr	xkb;

    xkb= dev->key->xkbInfo->desc;
    if ((key>=xkb->min_key_code) && (key<=xkb->max_key_code)) {
	XkbAction	*pActs;
	xkb->server->behaviors[key].type= XkbKB_Lock|XkbKB_Permanent;
    }
    else ErrorF("Internal Error!  Bad XKB info in SetPhysicalLockingKey\n");
    return;
}

/***====================================================================***/

Bool
XkbChangeEnabledControls(xkbi,change,newValues,cause,changes)
    XkbSrvInfoPtr	xkbi;
    unsigned long	change;
    unsigned long	newValues;
    XkbEventCausePtr	cause;
    XkbChangesPtr	changes;
{
XkbControlsPtr		ctrls;
unsigned 		old;

    ctrls= xkbi->desc->ctrls;
    old= ctrls->enabled_ctrls;
    ctrls->enabled_ctrls&= ~change;
    ctrls->enabled_ctrls|= (change&newValues);
    if (old==ctrls->enabled_ctrls)
	return False;
    if (cause!=NULL) {
	xkbControlsNotify cn;
	cn.numGroups= ctrls->num_groups;
	cn.changedControls|= XkbControlsEnabledMask;
	cn.enabledControls= ctrls->enabled_ctrls;
	cn.enabledControlChanges= (ctrls->enabled_ctrls^old);
	cn.keycode= cause->kc;
	cn.eventType= cause->event;
	cn.requestMajor= cause->mjr;
	cn.requestMinor= cause->mnr;
	XkbSendControlsNotify(xkbi->device,&cn);
    }
    else {
	/* Yes, this really should be an XOR.  If ctrls->enabled_ctrls_changes*/
	/* is non-zero, the controls in question changed already in "this" */
	/* request and this change merely undoes the previous one.  By the */
	/* same token, we have to figure out whether or not ControlsEnabled */
	/* should be set or not in the changes structure */
	changes->ctrls.enabled_ctrls_changes^= (ctrls->enabled_ctrls^old);
	if (changes->ctrls.enabled_ctrls_changes)
	     changes->ctrls.changed_ctrls|= XkbControlsEnabledMask;
	else changes->ctrls.changed_ctrls&= ~XkbControlsEnabledMask;
    }
    if (xkbi->iAccel.usesControls) {
	XkbIndicatorChangesPtr	ic;
	ic= (changes?(&changes->indicators):NULL);
	XkbUpdateIndicators(xkbi->device,xkbi->iAccel.usesControls,ic);
    }
    return True;
}

/***====================================================================***/

#define	MAX_TOC	16

XkbGeometryPtr 
XkbLookupNamedGeometry(dev,name,shouldFree)
    DeviceIntPtr	dev;
    Atom		name;
    Bool *		shouldFree;
{
XkbSrvInfoPtr	xkbi=	dev->key->xkbInfo;
XkbDescPtr	xkb=	xkbi->desc;

    *shouldFree= 0;
    if (name==None) {
	if (xkb->geom!=NULL)
	    return xkb->geom;
	name= xkb->names->geometry;
    }
    if ((xkb->geom!=NULL)&&(xkb->geom->name==name))
	return xkb->geom;
    else if ((name==xkb->names->geometry)&&(xkb->geom==NULL)) {
	FILE *file= XkbDDXOpenConfigFile(XkbInitialMap,NULL,0);
	if (file!=NULL) {
	    XkbFileInfo		xkbFInfo;
	    xkmFileInfo		finfo;
	    xkmSectionInfo	toc[MAX_TOC],*entry;
	    bzero(&xkbFInfo,sizeof(xkbFInfo));
	    xkbFInfo.xkb= xkb;
	    if (XkmReadTOC(file,&finfo,MAX_TOC,toc)) {
		entry= XkmFindTOCEntry(&finfo,toc,XkmGeometryIndex);
		if (entry!=NULL)
		    XkmReadFileSection(file,entry,&xkbFInfo,NULL);
	    }
	    fclose(file);
	    if (xkb->geom) {
		*shouldFree= 0;
		return xkb->geom;
	    }
	}
    }
    *shouldFree= 1;
    return NULL;
}

void
XkbConvertCase(sym, lower, upper)
    register KeySym sym;
    KeySym *lower;
    KeySym *upper;
{
    *lower = sym;
    *upper = sym;
    switch(sym >> 8) {
    case 0: /* Latin 1 */
	if ((sym >= XK_A) && (sym <= XK_Z))
	    *lower += (XK_a - XK_A);
	else if ((sym >= XK_a) && (sym <= XK_z))
	    *upper -= (XK_a - XK_A);
	else if ((sym >= XK_Agrave) && (sym <= XK_Odiaeresis))
	    *lower += (XK_agrave - XK_Agrave);
	else if ((sym >= XK_agrave) && (sym <= XK_odiaeresis))
	    *upper -= (XK_agrave - XK_Agrave);
	else if ((sym >= XK_Ooblique) && (sym <= XK_Thorn))
	    *lower += (XK_oslash - XK_Ooblique);
	else if ((sym >= XK_oslash) && (sym <= XK_thorn))
	    *upper -= (XK_oslash - XK_Ooblique);
	break;
    case 1: /* Latin 2 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym == XK_Aogonek)
	    *lower = XK_aogonek;
	else if (sym >= XK_Lstroke && sym <= XK_Sacute)
	    *lower += (XK_lstroke - XK_Lstroke);
	else if (sym >= XK_Scaron && sym <= XK_Zacute)
	    *lower += (XK_scaron - XK_Scaron);
	else if (sym >= XK_Zcaron && sym <= XK_Zabovedot)
	    *lower += (XK_zcaron - XK_Zcaron);
	else if (sym == XK_aogonek)
	    *upper = XK_Aogonek;
	else if (sym >= XK_lstroke && sym <= XK_sacute)
	    *upper -= (XK_lstroke - XK_Lstroke);
	else if (sym >= XK_scaron && sym <= XK_zacute)
	    *upper -= (XK_scaron - XK_Scaron);
	else if (sym >= XK_zcaron && sym <= XK_zabovedot)
	    *upper -= (XK_zcaron - XK_Zcaron);
	else if (sym >= XK_Racute && sym <= XK_Tcedilla)
	    *lower += (XK_racute - XK_Racute);
	else if (sym >= XK_racute && sym <= XK_tcedilla)
	    *upper -= (XK_racute - XK_Racute);
	break;
    case 2: /* Latin 3 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Hstroke && sym <= XK_Hcircumflex)
	    *lower += (XK_hstroke - XK_Hstroke);
	else if (sym >= XK_Gbreve && sym <= XK_Jcircumflex)
	    *lower += (XK_gbreve - XK_Gbreve);
	else if (sym >= XK_hstroke && sym <= XK_hcircumflex)
	    *upper -= (XK_hstroke - XK_Hstroke);
	else if (sym >= XK_gbreve && sym <= XK_jcircumflex)
	    *upper -= (XK_gbreve - XK_Gbreve);
	else if (sym >= XK_Cabovedot && sym <= XK_Scircumflex)
	    *lower += (XK_cabovedot - XK_Cabovedot);
	else if (sym >= XK_cabovedot && sym <= XK_scircumflex)
	    *upper -= (XK_cabovedot - XK_Cabovedot);
	break;
    case 3: /* Latin 4 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Rcedilla && sym <= XK_Tslash)
	    *lower += (XK_rcedilla - XK_Rcedilla);
	else if (sym >= XK_rcedilla && sym <= XK_tslash)
	    *upper -= (XK_rcedilla - XK_Rcedilla);
	else if (sym == XK_ENG)
	    *lower = XK_eng;
	else if (sym == XK_eng)
	    *upper = XK_ENG;
	else if (sym >= XK_Amacron && sym <= XK_Umacron)
	    *lower += (XK_amacron - XK_Amacron);
	else if (sym >= XK_amacron && sym <= XK_umacron)
	    *upper -= (XK_amacron - XK_Amacron);
	break;
    case 6: /* Cyrillic */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Serbian_DJE && sym <= XK_Serbian_DZE)
	    *lower -= (XK_Serbian_DJE - XK_Serbian_dje);
	else if (sym >= XK_Serbian_dje && sym <= XK_Serbian_dze)
	    *upper += (XK_Serbian_DJE - XK_Serbian_dje);
	else if (sym >= XK_Cyrillic_YU && sym <= XK_Cyrillic_HARDSIGN)
	    *lower -= (XK_Cyrillic_YU - XK_Cyrillic_yu);
	else if (sym >= XK_Cyrillic_yu && sym <= XK_Cyrillic_hardsign)
	    *upper += (XK_Cyrillic_YU - XK_Cyrillic_yu);
        break;
    case 7: /* Greek */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Greek_ALPHAaccent && sym <= XK_Greek_OMEGAaccent)
	    *lower += (XK_Greek_alphaaccent - XK_Greek_ALPHAaccent);
	else if (sym >= XK_Greek_alphaaccent && sym <= XK_Greek_omegaaccent &&
		 sym != XK_Greek_iotaaccentdieresis &&
		 sym != XK_Greek_upsilonaccentdieresis)
	    *upper -= (XK_Greek_alphaaccent - XK_Greek_ALPHAaccent);
	else if (sym >= XK_Greek_ALPHA && sym <= XK_Greek_OMEGA)
	    *lower += (XK_Greek_alpha - XK_Greek_ALPHA);
	else if (sym >= XK_Greek_alpha && sym <= XK_Greek_omega &&
		 sym != XK_Greek_finalsmallsigma)
	    *upper -= (XK_Greek_alpha - XK_Greek_ALPHA);
        break;
    }
}



/* $XConsortium: XKBMAlloc.c /main/5 1996/01/14 16:43:37 kaleb $ */
/* $XFree86: xc/lib/X11/XKBMAlloc.c,v 3.0 1996/01/11 10:33:13 dawes Exp $ */
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

#ifndef XKB_IN_SERVER

#include <stdio.h>
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xlibint.h"
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"

#else 

#include <stdio.h>
#include "X.h"
#define	NEED_EVENTS
#define	NEED_REPLIES
#include "Xproto.h"
#include "misc.h"
#include "inputstr.h"
#include "XKBsrv.h"

#endif /* XKB_IN_SERVER */

/***====================================================================***/

#define	mapSize(m)	(sizeof(m)/sizeof(XkbKTMapEntryRec))
static  XkbKTMapEntryRec map2Level[]= { 
	{ True, ShiftMask, 1, ShiftMask, 0 }
};

static  XkbKTMapEntryRec mapAlpha[]= { 
	{ True, ShiftMask, 1, ShiftMask, 0 },
	{ True,	LockMask,  0,  LockMask, 0 }
};

static	XkbModsRec preAlpha[]= {
	{        0,        0, 0 },
	{ LockMask, LockMask, 0 }
};

#define	NL_VMOD_MASK	0
static  XkbKTMapEntryRec mapKeypad[]= { 
	{ True,	ShiftMask, 1, ShiftMask,            0 },
	{ False,        0, 1,         0, NL_VMOD_MASK }
};

static	XkbKeyTypeRec	canonicalTypes[XkbNumRequiredTypes] = {
	{ { 0, 0, 0 }, 
	  1,	/* num_levels */
	  0,	/* map_count */
	  NULL,		NULL,
	  None,		NULL
	},
	{ { ShiftMask, ShiftMask, 0 }, 
	  2,	/* num_levels */
	  mapSize(map2Level),	/* map_count */
	  map2Level,	NULL,
	  None,		NULL
	},
	{ { ShiftMask|LockMask, ShiftMask|LockMask, 0 }, 
	  2,				/* num_levels */
	  mapSize(mapAlpha),		/* map_count */
	  mapAlpha,	preAlpha,
	  None,		NULL
	},
	{ { ShiftMask, ShiftMask, NL_VMOD_MASK },
	  2,				/* num_levels */
	  mapSize(mapKeypad),		/* map_count */
	  mapKeypad,	NULL,
	  None,		NULL
	}
};

Status
#if NeedFunctionPrototypes
XkbInitCanonicalKeyTypes(XkbDescPtr xkb,unsigned which,int keypadVMod)
#else
XkbInitCanonicalKeyTypes(xkb,which,keypadVMod)
    XkbDescPtr		xkb;
    unsigned		which;
    int			keypadVMod;
#endif
{
XkbClientMapPtr	map;
XkbKeyTypePtr	from,to;
Status		rtrn;

    if (!xkb)
	return BadMatch;
    rtrn= XkbAllocClientMap(xkb,XkbKeyTypesMask,XkbNumRequiredTypes);
    if (rtrn!=Success)
	return rtrn;
    map= xkb->map;
    if ((which&XkbAllRequiredTypes)==0)
	return Success;
    rtrn= Success;
    from= canonicalTypes;
    to= map->types;
    if (which&XkbOneLevelMask)
	rtrn= XkbCopyKeyType(&from[XkbOneLevelIndex],&to[XkbOneLevelIndex]);
    if ((which&XkbTwoLevelMask)&&(rtrn==Success))
	rtrn= XkbCopyKeyType(&from[XkbTwoLevelIndex],&to[XkbTwoLevelIndex]);
    if ((which&XkbAlphabeticMask)&&(rtrn==Success))
	rtrn= XkbCopyKeyType(&from[XkbAlphabeticIndex],&to[XkbAlphabeticIndex]);
    if ((which&XkbKeypadMask)&&(rtrn==Success)) {
	XkbKeyTypePtr type;
	rtrn= XkbCopyKeyType(&from[XkbKeypadIndex],&to[XkbKeypadIndex]);
	type= &to[XkbKeypadIndex];
	if ((keypadVMod>=0)&&(keypadVMod<XkbNumVirtualMods)&&(rtrn==Success)) {
	    type->mods.vmods= (1<<keypadVMod);
	    type->map[0].active= True;
	    type->map[0].mods.mask= ShiftMask;
	    type->map[0].mods.real_mods= ShiftMask;
	    type->map[0].mods.vmods= 0;
	    type->map[0].level= 1;
	    type->map[1].active= False;
	    type->map[1].mods.mask= 0;
	    type->map[1].mods.real_mods= 0;
	    type->map[1].mods.vmods= (1<<keypadVMod);
	    type->map[1].level= 1;
	}
    }
    return Success;
}

/***====================================================================***/

Status
#if NeedFunctionPrototypes
XkbAllocClientMap(XkbDescPtr xkb,unsigned which,unsigned nTotalTypes)
#else
XkbAllocClientMap(xkb,which,nTotalTypes)
    XkbDescPtr		xkb;
    unsigned		which;
    unsigned		nTotalTypes;
#endif
{
register int	i;
XkbClientMapPtr map;

    if ((xkb==NULL)||((nTotalTypes>0)&&(nTotalTypes<XkbNumRequiredTypes)))
	return BadValue;
    if ((which&XkbKeySymsMask)&&
	((!XkbIsLegalKeycode(xkb->min_key_code))||
	 (!XkbIsLegalKeycode(xkb->max_key_code))||
	 (xkb->max_key_code<xkb->min_key_code))) {
#ifdef DEBUG
fprintf(stderr,"bad keycode (%d,%d) in XkbAllocClientMap\n",
				xkb->min_key_code,xkb->max_key_code);
#endif
	return BadValue;
    }

    if (xkb->map==NULL) {
	map= _XkbTypedCalloc(1,XkbClientMapRec);
	if (map==NULL)
	    return BadAlloc;
	xkb->map= map;
    }
    else map= xkb->map;

    if ((which&XkbKeyTypesMask)&&(nTotalTypes>0)) {
	if (map->types==NULL) {
	    map->types= _XkbTypedCalloc(nTotalTypes,XkbKeyTypeRec);
	    if (map->types==NULL)
		return BadAlloc;
	    map->num_types= 0;
	    map->size_types= nTotalTypes;
	}
	else if (map->size_types<nTotalTypes) {
	    map->types= _XkbTypedRealloc(map->types,nTotalTypes,XkbKeyTypeRec);
	    if (map->types==NULL) {
		map->num_types= map->size_types= 0;
		return BadAlloc;
	    }
	    map->size_types= nTotalTypes;
	    bzero(&map->types[map->num_types], 
		  ((map->size_types-map->num_types)*sizeof(XkbKeyTypeRec)));
	}
    }
    if (which&XkbKeySymsMask) {
	int nKeys= XkbNumKeys(xkb);
	if (map->syms==NULL) {
	    map->size_syms= (nKeys*15)/10;
	    map->syms= _XkbTypedCalloc(map->size_syms,KeySym);
	    if (!map->syms) {
		map->size_syms= 0;
		return BadAlloc;
	    }
	    map->num_syms= 1;
	    map->syms[0]= NoSymbol;
	}
	if (map->key_sym_map==NULL) {
	    i= xkb->max_key_code+1;
	    map->key_sym_map= _XkbTypedCalloc(i,XkbSymMapRec);
	    if (map->key_sym_map==NULL)
		return BadAlloc;
	}
    }
    if (which&XkbModifierMapMask) {
	if ((!XkbIsLegalKeycode(xkb->min_key_code))||
	    (!XkbIsLegalKeycode(xkb->max_key_code))||
	    (xkb->max_key_code<xkb->min_key_code))
	    return BadMatch;
	if (map->modmap==NULL) {
	    i= xkb->max_key_code+1;
	    map->modmap= _XkbTypedCalloc(i,unsigned char);
	    if (map->modmap==NULL)
		return BadAlloc;
	}
    }
    return Success;
}

Status
#if NeedFunctionPrototypes
XkbAllocServerMap(XkbDescPtr xkb,unsigned which,unsigned nNewActions)
#else
XkbAllocServerMap(xkb,which,nNewActions)
    XkbDescPtr		xkb;
    unsigned		which;
    unsigned		nNewActions;
#endif
{
register int	i;
XkbServerMapPtr map;

    if (xkb==NULL)
	return BadMatch;
    if (xkb->server==NULL) {
	map= _XkbTypedCalloc(1,XkbServerMapRec);
	if (map==NULL)
	    return BadAlloc;
	for (i=0;i<XkbNumVirtualMods;i++) {
	    map->vmods[i]= XkbNoModifierMask;
	}
	xkb->server= map;
    }
    else map= xkb->server;
    if (which&XkbExplicitComponentsMask) {
	if ((!XkbIsLegalKeycode(xkb->min_key_code))||
	    (!XkbIsLegalKeycode(xkb->max_key_code))||
	    (xkb->max_key_code<xkb->min_key_code))
	    return BadMatch;
	if (map->explicit==NULL) {
	    i= xkb->max_key_code+1;
	    map->explicit= _XkbTypedCalloc(i,unsigned char);
	    if (map->explicit==NULL)
		return BadAlloc;
	}
    }
    if (which&XkbKeyActionsMask) {
	if ((!XkbIsLegalKeycode(xkb->min_key_code))||
	    (!XkbIsLegalKeycode(xkb->max_key_code))||
	    (xkb->max_key_code<xkb->min_key_code))
	    return BadMatch;
        if (nNewActions<1)
	    nNewActions= 1;
	if (map->acts==NULL) {
	    map->acts= _XkbTypedCalloc((nNewActions+1),XkbAction);
	    if (map->acts==NULL)
		return BadAlloc;
	    map->num_acts= 1;
	    map->size_acts= nNewActions+1;
	}
	else if ((map->size_acts-map->num_acts)<nNewActions) {
	    unsigned need;
	    need= map->num_acts+nNewActions;
	    map->acts= _XkbTypedRealloc(map->acts,need,XkbAction);
	    if (map->acts==NULL) {
	        map->num_acts= map->size_acts= 0;
	        return BadAlloc;
	    }
	    map->size_acts= need;
	    bzero(&map->acts[map->num_acts], 
		    ((map->size_acts-map->num_acts)*sizeof(XkbAction)));
	}
	if (map->key_acts==NULL) {
	    i= xkb->max_key_code+1;
	    map->key_acts= _XkbTypedCalloc(i,unsigned short);
	    if (map->key_acts==NULL)
		return BadAlloc;
	}
    }
    if (which&XkbKeyBehaviorsMask) {
	if ((!XkbIsLegalKeycode(xkb->min_key_code))||
	    (!XkbIsLegalKeycode(xkb->max_key_code))||
	    (xkb->max_key_code<xkb->min_key_code))
	    return BadMatch;
	if (map->behaviors==NULL) {
	    i= xkb->max_key_code+1;
	    map->behaviors= _XkbTypedCalloc(i,XkbBehavior);
	    if (map->behaviors==NULL)
		return BadAlloc;
	}
    }
    if (which&XkbVirtualModMapMask) {
	if ((!XkbIsLegalKeycode(xkb->min_key_code))||
	    (!XkbIsLegalKeycode(xkb->max_key_code))||
	    (xkb->max_key_code<xkb->min_key_code))
	    return BadMatch;
	if (map->vmodmap==NULL) {
	    i= xkb->max_key_code+1;
	    map->vmodmap= _XkbTypedCalloc(i,unsigned short);
	    if (map->vmodmap==NULL)
		return BadAlloc;
	}
    }
    return Success;
}

/***====================================================================***/

Status
#if NeedFunctionPrototypes
XkbCopyKeyType(XkbKeyTypePtr from,XkbKeyTypePtr into)
#else
XkbCopyKeyType(from,into)
    XkbKeyTypePtr	from;
    XkbKeyTypePtr	into;
#endif
{
    if ((!from)||(!into))
	return BadMatch;
    if (into->map) {
	Xfree(into->map);
	into->map= NULL;
    }
    if (into->preserve) {
	Xfree(into->preserve);
	into->preserve= NULL;
    }
    if (into->level_names) {
	Xfree(into->level_names);
	into->level_names= NULL;
    }
    *into= *from;
    if ((from->map)&&(into->map_count>0)) {
	into->map= _XkbTypedCalloc(into->map_count,XkbKTMapEntryRec);
	if (!into->map)
	    return BadAlloc;
	memcpy(into->map,from->map,into->map_count*sizeof(XkbKTMapEntryRec));
    }
    if ((from->preserve)&&(into->map_count>0)) {
	into->preserve= _XkbTypedCalloc(into->map_count,XkbModsRec);
	if (!into->preserve)
	    return BadAlloc;
	memcpy(into->preserve,from->preserve,
				into->map_count*sizeof(XkbModsRec));
    }
    if ((from->level_names)&&(into->num_levels>0)) {
	into->level_names= _XkbTypedCalloc(into->num_levels,Atom);
	if (!into->level_names)
	    return BadAlloc;
	memcpy(into->level_names,from->level_names,
				 into->num_levels*sizeof(Atom));
    }
    return Success;
}

Status
#if NeedFunctionPrototypes
XkbCopyKeyTypes(XkbKeyTypePtr from,XkbKeyTypePtr into,int num_types)
#else
XkbCopyKeyTypes(from,into,num_types)
    XkbKeyTypePtr	from;
    XkbKeyTypePtr	into;
    int			num_types;
#endif
{
register int i,rtrn;

    if ((!from)||(!into)||(num_types<0))
	return BadMatch;
    for (i=0;i<num_types;i++) {
	if ((rtrn= XkbCopyKeyType(from++,into++))!=Success)
	    return rtrn;
    }
    return Success;
}

XkbKeyTypePtr
#if NeedFunctionPrototypes
XkbAddKeyType(	XkbDescPtr	xkb,
		Atom 		name,
		int 		map_count,
		Bool 		want_preserve,
		int		num_lvls)
#else
XkbAddKeyType(xkb,name,map_count,want_preserve,num_lvls)
    XkbDescPtr	xkb;
    Atom	name;
    int		map_count;
    Bool	want_preserve;
    int		num_lvls;
#endif
{
register int 	i;
unsigned	tmp;
XkbKeyTypePtr	type;
XkbClientMapPtr	map;

    if ((!xkb)||(num_lvls<1))
	return NULL;
    map= xkb->map;
    if ((map)&&(map->types)) {
	for (i=0;i<map->num_types;i++) {
	    if (map->types[i].name==name) {
		Status status;
		status=XkbResizeKeyType(xkb,i,map_count,want_preserve,num_lvls);
		return (status==Success?type:NULL);
	    }
	}
    }
    if ((!map)||(!map->types)||(!map->num_types<XkbNumRequiredTypes)) {
	tmp= XkbNumRequiredTypes+1;
	if (XkbAllocClientMap(xkb,XkbKeyTypesMask,tmp)!=Success)
	    return NULL;
	tmp= 0;
	if (map->num_types<=XkbKeypadIndex)
	    tmp|= XkbKeypadMask;
	if (map->num_types<=XkbAlphabeticIndex)
	    tmp|= XkbAlphabeticMask;
	if (map->num_types<=XkbTwoLevelIndex)
	    tmp|= XkbTwoLevelMask;
	if (map->num_types<=XkbOneLevelIndex)
	    tmp|= XkbOneLevelMask;
	if (XkbInitCanonicalKeyTypes(xkb,tmp,XkbNoModifier)==Success) {
	    for (i=0;i<map->num_types;i++) {
		Status status;
		if (map->types[i].name!=name)
		    continue;
		status=XkbResizeKeyType(xkb,i,map_count,want_preserve,num_lvls);
		return (status==Success?type:NULL);
	    }
	}
    }
    if ((map->num_types<=map->size_types)&&
	(XkbAllocClientMap(xkb,XkbKeyTypesMask,map->num_types+1)!=Success)) {
	return NULL;
    }
    type= &map->types[map->num_types];
    map->num_types++;
    bzero((char *)type,sizeof(XkbKeyTypeRec));
    type->num_levels=	num_lvls;
    type->map_count=	map_count;
    type->name=		name;
    if (map_count>0) {
	type->map=	_XkbTypedCalloc(map_count,XkbKTMapEntryRec);
	if (!type->map) {
	    map->num_types--;
	    return NULL;
	}
	if (want_preserve) {
	    type->preserve=	_XkbTypedCalloc(map_count,XkbModsRec);
	    if (!type->preserve) {
		_XkbFree(type->map);
		map->num_types--;
		return NULL;
	    }
	}
    }
    return type;
}

Status
#if NeedFunctionPrototypes
XkbResizeKeyType(	XkbDescPtr	xkb,
			int		type_ndx,
			int		map_count,
			Bool		want_preserve,
			int		new_num_lvls)
#else
XkbResizeKeyType(xkb,type_ndx,map_count,want_preserve,new_num_lvls)
    XkbDescPtr		xkb;
    int			type_ndx;
    int			map_count;
    Bool		want_preserve;
    int			new_num_lvls;
#endif
{
XkbKeyTypePtr	type;
KeyCode		matchingKeys[XkbMaxKeyCount],nMatchingKeys;

    if ((type_ndx<0)||(type_ndx>=xkb->map->num_types)||(map_count<0)||
    							(new_num_lvls<1))
	return BadValue;
    switch (type_ndx) {
	case XkbOneLevelIndex:
	    if (new_num_lvls!=1)
		return BadMatch;
	    break;
	case XkbTwoLevelIndex:
	case XkbAlphabeticIndex:
	case XkbKeypadIndex:
	    if (new_num_lvls!=2)
		return BadMatch;
	    break;
    }
    type= &xkb->map->types[type_ndx];
    if (map_count==0) {
	if (type->map!=NULL)
	    Xfree(type->map);
	type->map= NULL;
	if (type->preserve!=NULL)
	    Xfree(type->preserve);
	type->preserve= NULL;
	type->map_count= 0;
    }
    else {
	if ((map_count>type->map_count)||(type->map==NULL))
	    type->map=_XkbTypedRealloc(type->map,map_count,XkbKTMapEntryRec);
	if (!type->map)
	    return BadAlloc;
	if (want_preserve) {
	    if ((map_count>type->map_count)||(type->preserve==NULL)) {
		type->preserve= _XkbTypedRealloc(type->preserve,map_count,
	     						    XkbModsRec);
	    }
	    if (!type->preserve)
		return BadAlloc;
	}
	else if (type->preserve!=NULL) {
	    Xfree(type->preserve);
	    type->preserve= NULL;
	}
	type->map_count= map_count;
    }

    if ((new_num_lvls>type->num_levels)||(type->level_names==NULL)) {
	type->level_names=_XkbTypedRealloc(type->level_names,new_num_lvls,Atom);
	if (!type->level_names)
	    return BadAlloc;
    }
    /*
     * Here's the theory:
     *    If the width of the type changed, we might have to resize the symbol
     * maps for any keys that use the type for one or more groups.  This is
     * expensive, so we'll try to cull out any keys that are obviously okay:
     * In any case:
     *    - keys that have a group width <= the old width are okay (because
     *      they could not possibly have been associated with the old type)
     * If the key type increased in size:
     *    - keys that already have a group width >= to the new width are okay
     *    + keys that have a group width >= the old width but < the new width
     *      might have to be enlarged.
     * If the key type decreased in size:
     *    - keys that have a group width > the old width don't have to be
     *      resized (because they must have some other wider type associated 
     *      with some group).
     *    + keys that have a group width == the old width might have to be
     *      shrunk.
     * The possibilities marked with '+' require us to examine the key types
     * associated with each group for the key.
     */
    bzero(matchingKeys,XkbMaxKeyCount*sizeof(KeyCode));
    nMatchingKeys= 0;
    if (new_num_lvls>type->num_levels) {
	int	 		nTotal;
	KeySym	*		newSyms;
	int			width,match,nResize;
	register int		i,g,nSyms;

	nResize= 0;
	for (nTotal=1,i=xkb->min_key_code;i<=xkb->max_key_code;i++) {
	    width= XkbKeyGroupsWidth(xkb,i);
	    if (width<type->num_levels)
		continue;
	    for (match=0,g=XkbKeyNumGroups(xkb,i)-1;(g>=0)&&(!match);g--) {
		if (XkbKeyKeyTypeIndex(xkb,i,g)==type_ndx) {
		    matchingKeys[nMatchingKeys++]= i;
		    match= 1;
		}
	    }
	    if ((!match)||(width>=new_num_lvls))
		nTotal+= XkbKeyNumSyms(xkb,i);
	    else {
		nTotal+= XkbKeyNumGroups(xkb,i)*new_num_lvls;
		nResize++;
	    }
	}
	if (nResize>0) {
	    int nextMatch;
	    xkb->map->size_syms= (nTotal*12)/10;
	    newSyms = _XkbTypedCalloc(xkb->map->size_syms,KeySym);
	    if (newSyms==NULL)
		return BadAlloc;
	    nextMatch= 0;
	    nSyms= 1;
	    for (i=xkb->min_key_code;i<=xkb->max_key_code;i++) {
		if (matchingKeys[nextMatch]==i) {
		    KeySym *pOld;
		    nextMatch++;
		    width= XkbKeyGroupsWidth(xkb,i);
		    pOld= XkbKeySymsPtr(xkb,i);
		    for (g=XkbKeyNumGroups(xkb,i)-1;g>=0;g--) {
			memcpy(&newSyms[nSyms+(new_num_lvls*g)],&pOld[width*g],
							width*sizeof(KeySym));
		    }
		    xkb->map->key_sym_map[i].offset= nSyms;
		    nSyms+= XkbKeyNumGroups(xkb,i)*new_num_lvls;
		}
		else {
		    memcpy(&newSyms[nSyms],XkbKeySymsPtr(xkb,i),
					XkbKeyNumSyms(xkb,i)*sizeof(KeySym));
		    xkb->map->key_sym_map[i].offset= nSyms;
		    nSyms+= XkbKeyNumSyms(xkb,i);
		}
	    }
	    type->num_levels= new_num_lvls;
	    _XkbFree(xkb->map->syms);
	    xkb->map->syms= newSyms;
	    xkb->map->num_syms= nSyms;
	    return Success;
	}
    }
    else if (new_num_lvls<type->num_levels) {
	int 		width,match;
	register int	g,i;
	for (i=xkb->min_key_code;i<=xkb->max_key_code;i++) {
	    width= XkbKeyGroupsWidth(xkb,i);
	    if (width<type->num_levels)
		continue;
	    for (match=0,g=XkbKeyNumGroups(xkb,i)-1;(g>=0)&&(!match);g--) {
		if (XkbKeyKeyTypeIndex(xkb,i,g)==type_ndx) {
		    matchingKeys[nMatchingKeys++]= i;
		    match= 1;
		}
	    }
	}
    }
    if (nMatchingKeys>0) {
	int 		key,firstClear;
	register int	i,g;
	if (new_num_lvls>type->num_levels)
	     firstClear= type->num_levels;
	else firstClear= new_num_lvls;
	for (i=0;i<nMatchingKeys;i++) {
	    KeySym *	pSyms;
	    int		width,nClear;

	    key= matchingKeys[i];
	    width= XkbKeyGroupsWidth(xkb,key);
	    nClear= width-firstClear;
	    pSyms= XkbKeySymsPtr(xkb,key);
	    for (g=XkbKeyNumGroups(xkb,key)-1;g>=0;g--) {
		if (XkbKeyKeyTypeIndex(xkb,key,g)==type_ndx) {
		    if (nClear>0)
			bzero(&pSyms[g*width+firstClear],nClear*sizeof(KeySym));
		}
	    }
	}
    }
    type->num_levels= new_num_lvls;
    return Success;
}

KeySym *
#if NeedFunctionPrototypes
XkbResizeKeySyms(XkbDescPtr xkb,int key,int needed)
#else
XkbResizeKeySyms(xkb,key,needed)
    XkbDescPtr	xkb;
    int 	key;
    int 	needed;
#endif
{
register int i,nSyms,nKeySyms;
unsigned nOldSyms;
KeySym	*newSyms;

    if (needed==0) {
	xkb->map->key_sym_map[key].offset= 0;
	return xkb->map->syms;
    }
    nOldSyms= XkbKeyNumSyms(xkb,key);
    if (nOldSyms>=(unsigned)needed) {
	return XkbKeySymsPtr(xkb,key);
    }
    if (xkb->map->size_syms-xkb->map->num_syms>=(unsigned)needed) {
	if (nOldSyms>0) {
	    memcpy(&xkb->map->syms[xkb->map->num_syms],XkbKeySymsPtr(xkb,key),
						nOldSyms*sizeof(KeySym));
	}
	if ((needed-nOldSyms)>0) {
	    bzero(&xkb->map->syms[xkb->map->num_syms+XkbKeyNumSyms(xkb,key)],
					(needed-nOldSyms)*sizeof(KeySym));
	}
	xkb->map->key_sym_map[key].offset = xkb->map->num_syms;
	xkb->map->num_syms+= needed;
	return &xkb->map->syms[xkb->map->key_sym_map[key].offset];
    }
    xkb->map->size_syms+= (needed>32?needed:32);
    newSyms = _XkbTypedCalloc(xkb->map->size_syms,KeySym);
    if (newSyms==NULL)
	return NULL;
    newSyms[0]= NoSymbol;
    nSyms = 1;
    for (i=xkb->min_key_code;i<=(int)xkb->max_key_code;i++) {
	int nCopy;

	nCopy= nKeySyms= XkbKeyNumSyms(xkb,i);
	if ((nKeySyms==0)&&(i!=key))
	    continue;
	if (i==key)
	    nKeySyms= needed;
	if (nCopy!=0)
	   memcpy(&newSyms[nSyms],XkbKeySymsPtr(xkb,i),nCopy*sizeof(KeySym));
	if (nKeySyms>nCopy)
	    bzero(&newSyms[nSyms+nCopy],(nKeySyms-nCopy)*sizeof(KeySym));
	xkb->map->key_sym_map[i].offset = nSyms;
	nSyms+= nKeySyms;
    }
    _XkbFree(xkb->map->syms);
    xkb->map->syms = newSyms;
    xkb->map->num_syms = nSyms;
    return &xkb->map->syms[xkb->map->key_sym_map[key].offset];
}

XkbAction *
#if NeedFunctionPrototypes
XkbResizeKeyActions(XkbDescPtr xkb,int key,int needed)
#else
XkbResizeKeyActions(xkb,key,needed)
    XkbDescPtr	xkb;
    int 	key;
    int 	needed;
#endif
{
register int i,nActs;
XkbAction *newActs;

    if (needed==0) {
	xkb->server->key_acts[key]= 0;
	return NULL;
    }
    if (XkbKeyHasActions(xkb,key)&&(XkbKeyNumSyms(xkb,key)>=(unsigned)needed))
	return XkbKeyActionsPtr(xkb,key);
    if (xkb->server->size_acts-xkb->server->num_acts>=(unsigned)needed) {
	xkb->server->key_acts[key]= xkb->server->num_acts;
	xkb->server->num_acts+= needed;
	return &xkb->server->acts[xkb->server->key_acts[key]];
    }
    xkb->server->size_acts= xkb->server->num_acts+needed+8;
    newActs = _XkbTypedCalloc(xkb->server->size_acts,XkbAction);
    if (newActs==NULL)
	return NULL;
    newActs[0].type = XkbSA_NoAction;
    nActs = 1;
    for (i=xkb->min_key_code;i<=(int)xkb->max_key_code;i++) {
	int nKeyActs,nCopy;

	if ((xkb->server->key_acts[i]==0)&&(i!=key))
	    continue;

	nCopy= nKeyActs= XkbKeyNumActions(xkb,i);
	if (i==key) {
	    nKeyActs= needed;
	    if (needed<nCopy)
		nCopy= needed;
	}

	if (nCopy>0)
	    memcpy(&newActs[nActs],XkbKeyActionsPtr(xkb,i),
						nCopy*sizeof(XkbAction));
	if (nCopy<nKeyActs)
	    bzero(&newActs[nActs+nCopy],(nKeyActs-nCopy)*sizeof(XkbAction));
	xkb->server->key_acts[i]= nActs;
	nActs+= nKeyActs;
    }
    _XkbFree(xkb->server->acts);
    xkb->server->acts = newActs;
    xkb->server->num_acts= nActs;
    return &xkb->server->acts[xkb->server->key_acts[key]];
}

Status
#if NeedFunctionPrototypes
XkbChangeTypesOfKey(	XkbDescPtr		 xkb,
			int		 	 key,
			int			 nGroups,
			unsigned	 	 groups,
			int	* 	 	 newTypesIn,
			XkbMapChangesPtr	 pChanges)
#else
XkbChangeTypesOfKey(xkb,key,nGroups,groups,newTypesIn,pChanges)
    XkbDescPtr		 xkb;
    int		 	 key;
    int			 nGroups;
    unsigned	 	 groups;
    int	* 	 	 newTypesIn;
    XkbMapChangesPtr	 pChanges;
#endif
{
XkbKeyTypePtr	pOldType,pNewType;
register int	i;
int		width,nOldGroups,oldWidth,newTypes[XkbNumKbdGroups];

    if ((!xkb) || (!XkbKeycodeInRange(xkb,key)) || (!xkb->map) ||
	(!xkb->map->types)||(!newTypes)||
	(nGroups>XkbNumKbdGroups)||((groups&XkbAllGroupsMask)==0)) {
	return BadMatch;
    }
    if (nGroups==0) {
	for (i=0;i<XkbNumKbdGroups;i++) {
	    xkb->map->key_sym_map[key].kt_index[i]= XkbOneLevelIndex;
	}
	i= xkb->map->key_sym_map[key].group_info;
	i= XkbSetNumGroups(i,0);
	xkb->map->key_sym_map[key].group_info= i;
	XkbResizeKeySyms(xkb,key,0);
	return Success;
    }

    nOldGroups= XkbKeyNumGroups(xkb,key);
    oldWidth= XkbKeyGroupsWidth(xkb,key);
    for (width=i=0;i<nGroups;i++) {
	if (groups&(1<<i))
	     newTypes[i]=  newTypesIn[i];
	else if (i<nOldGroups)
	     newTypes[i]= XkbKeyKeyTypeIndex(xkb,key,i);
	else if (nOldGroups>0)
	     newTypes[i]= XkbKeyKeyTypeIndex(xkb,key,XkbGroup1Index);
	else newTypes[i]= XkbTwoLevelIndex;
	if (newTypes[i]>xkb->map->num_types)
	    return BadMatch;
	pNewType= &xkb->map->types[newTypes[i]];
	if (pNewType->num_levels>width)
	    width= pNewType->num_levels;
    }
    if ((width!=oldWidth)||(nGroups!=nOldGroups)) {
	KeySym		oldSyms[XkbMaxSymsPerKey],*pSyms;
	int		nCopy;

	if (nOldGroups==0) {
	    pSyms= XkbResizeKeySyms(xkb,key,width*nGroups);
	    return ((pSyms!=NULL)?Success:BadAlloc);
	}
	pSyms= XkbKeySymsPtr(xkb,key);
	memcpy(oldSyms,pSyms,XkbKeyNumSyms(xkb,key)*sizeof(KeySym));
	pSyms= XkbResizeKeySyms(xkb,key,width*nGroups);
	if (pSyms==NULL)
	    return BadAlloc;
	bzero(pSyms,width*nGroups*sizeof(KeySym));
	for (i=0;(i<nGroups)&&(i<nOldGroups);i++) {
	    pOldType= XkbKeyKeyType(xkb,key,i);
	    pNewType= &xkb->map->types[newTypes[i]];
	    if (pNewType->num_levels>pOldType->num_levels)
		 nCopy= pOldType->num_levels;
	    else nCopy= pNewType->num_levels;
	    memcpy(&pSyms[i*width],&oldSyms[i*oldWidth],nCopy*sizeof(KeySym));
	}
	if (XkbKeyHasActions(xkb,key)) {
	    XkbAction	oldActs[XkbMaxSymsPerKey],*pActs;
	    pActs= XkbKeyActionsPtr(xkb,key);
	    memcpy(oldActs,pActs,XkbKeyNumSyms(xkb,key)*sizeof(XkbAction));
	    pActs= XkbResizeKeyActions(xkb,key,width*nGroups);
	    if (pActs==NULL)
		return BadAlloc;
	    bzero(pActs,width*nGroups*sizeof(XkbAction));
	    for (i=0;(i<nGroups)&&(i<nOldGroups);i++) {
		pOldType= XkbKeyKeyType(xkb,key,i);
		pNewType= &xkb->map->types[newTypes[i]];
		if (pNewType->num_levels>pOldType->num_levels)
		     nCopy= pOldType->num_levels;
		else nCopy= pNewType->num_levels;
		memcpy(&pActs[i*width],&oldActs[i*oldWidth],
						nCopy*sizeof(XkbAction));
	    }
	}
    }
    for (i=0;i<nGroups;i++) {
	xkb->map->key_sym_map[key].kt_index[i]= newTypes[i];
    }
    if (pChanges!=NULL) {
	if (pChanges->changed&XkbKeySymsMask) {
	    int first,last;
	    first= pChanges->first_key_sym;
	    last= pChanges->first_key_sym+pChanges->num_key_syms-1;
	    if (key<first)	first= key;
	    if (key>last)	last= key;
	    pChanges->first_key_sym = first;
	    pChanges->num_key_syms = (last-first)+1;
	}
	else {
	    pChanges->changed|= XkbKeySymsMask;
	    pChanges->first_key_sym= key;
	    pChanges->num_key_syms= 1;
	}
    }
    return Success;
}

void
#if NeedFunctionPrototypes
XkbFreeClientMap(XkbDescPtr xkb,unsigned what,Bool freeMap)
#else
XkbFreeClientMap(xkb,what,freeMap)
    XkbDescPtr	xkb;
    unsigned	what;
    Bool	freeMap;
#endif
{
XkbClientMapPtr	map;

    if ((xkb==NULL)||(xkb->map==NULL))
	return;
    if (freeMap)
	what= XkbAllClientInfoMask;
    map= xkb->map;
    if (what&XkbKeyTypesMask) {
	if (map->types!=NULL) {
	    if (map->num_types>0) {
		register int 	i;
		XkbKeyTypePtr	type;
		for (i=0,type=map->types;i<map->num_types;i++,type++) {
		    if (type->map!=NULL) {
			_XkbFree(type->map);
			type->map= NULL;
		    }
		    if (type->preserve!=NULL) {
			_XkbFree(type->preserve);
			type->preserve= NULL;
		    }
		    type->map_count= 0;
		    if (type->level_names!=NULL) {
			_XkbFree(type->level_names);
			type->level_names= NULL;
		    }
		}
	    }
	    _XkbFree(map->types);
	    map->num_types= map->size_types= 0;
	    map->types= NULL;
	}
    }
    if (what&XkbKeySymsMask) {
	if (map->key_sym_map!=NULL) {
	    _XkbFree(map->key_sym_map);
	    map->key_sym_map= NULL;
	}
	if (map->syms!=NULL) {
	    _XkbFree(map->syms);
	    map->size_syms= map->num_syms= 0;
	    map->syms= NULL;
	}
    }
    if ((what&XkbModifierMapMask)&&(map->modmap!=NULL)) {
	_XkbFree(map->modmap);
	map->modmap= NULL;
    }
    if (freeMap) {
	_XkbFree(xkb->map);
	xkb->map= NULL;
    }
    return;
}

void
#if NeedFunctionPrototypes
XkbFreeServerMap(XkbDescPtr xkb,unsigned what,Bool freeMap)
#else
XkbFreeServerMap(xkb,what,freeMap)
    XkbDescPtr	xkb;
    unsigned	what;
    Bool	freeMap;
#endif
{
XkbServerMapPtr	map;

    if ((xkb==NULL)||(xkb->server==NULL))
	return;
    if (freeMap)
	what= XkbAllServerInfoMask;
    map= xkb->server;
    if ((what&XkbExplicitComponentsMask)&&(map->explicit!=NULL)) {
	_XkbFree(map->explicit);
	map->explicit= NULL;
    }
    if (what&XkbKeyActionsMask) {
	if (map->key_acts!=NULL) {
	    _XkbFree(map->key_acts);
	    map->key_acts= NULL;
	}
	if (map->acts!=NULL) {
	    _XkbFree(map->acts);
	    map->num_acts= map->size_acts= 0;
	    map->acts= NULL;
	}
    }
    if ((what&XkbKeyBehaviorsMask)&&(map->behaviors!=NULL)) {
	_XkbFree(map->behaviors);
	map->behaviors= NULL;
    }
    if ((what&XkbVirtualModMapMask)&&(map->vmodmap!=NULL)) {
	_XkbFree(map->vmodmap);
	map->vmodmap= NULL;
    }

    if (freeMap) {
	_XkbFree(xkb->server);
	xkb->server= NULL;
    }
    return;
}

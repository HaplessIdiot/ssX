/* $XConsortium: cout.c /main/2 1995/12/07 21:18:35 kaleb $ */
/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

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
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include "XKMformat.h"
#include "XKBfileInt.h"

#define	lowbit(x)	((x) & (-(x)))

static Bool
WriteCHdrVMods(file,dpy,xkb)
    FILE *	file;
    Display *	dpy;
    XkbDescPtr	xkb;
{
register int i,nOut;

    if ((!xkb)||(!xkb->names)||(!xkb->names->vmods))
	return False;
    for (i=nOut=0;i<XkbNumVirtualMods;i++) {
	if (xkb->names->vmods[i]!=None) {
	    fprintf(file,"%s#define	vmod_%s	%d\n",(nOut<1?"\n":""),
				XkbAtomText(dpy,xkb->names->vmods[i],XkbCFile),
				i);
	    nOut++;
	}
    }
    for (i=nOut=0;i<XkbNumVirtualMods;i++) {
	if (xkb->names->vmods[i]!=None) {
	    fprintf(file,"%s#define	vmod_%sMask	(1<<%d)\n",
				(nOut<1?"\n":""),
				XkbAtomText(dpy,xkb->names->vmods[i],XkbCFile)
				,i);
	    nOut++;
	}
    }
    if (nOut>0)
	fprintf(file,"\n");
    return True;
}

static Bool
WriteCHdrKeycodes(file,xkb)
    FILE *	file;
    XkbDescPtr	xkb;
{
Atom			kcName;
register unsigned 	i;
char 			buf[8],buf2[8];

    if ((!xkb)||(!xkb->names)||(!xkb->names->keys)) {
	_XkbLibError(_XkbErrMissingNames,"WriteCHdrKeycodes",0);
	return False;
    }
    kcName= xkb->names->keycodes;
    buf[4]= '\0';
    if (xkb->names->keycodes!=None)
	 fprintf(file,"/* keycodes name is \"%s\" */\n",
 				XkbAtomText(xkb->dpy,kcName,XkbMessage));
    fprintf(file,"static XkbKeyNameRec	keyNames[NUM_KEYS]= {\n");
    for (i=0;i<=xkb->max_key_code;i++) {
	memcpy(buf,xkb->names->keys[i].name,4);
	sprintf(buf2,"\"%s\"",buf);
	if (i!=xkb->max_key_code)  {
	    fprintf(file,"    {  %6s  },",buf2);
	    if ((i&3)==3)
		fprintf(file,"\n");
	}
	else {
	    fprintf(file,"    {  %6s  }\n",buf2);
	}
    }
    fprintf(file,"};\n");
    return True;
}

static void
WriteTypePreserve(file,dpy,prefix,xkb,type)
    FILE *		file;
    Display *		dpy;
    char *		prefix;
    XkbDescPtr		xkb;
    XkbKeyTypePtr	type;
{
register unsigned	i;
XkbModsPtr		pre;

    fprintf(file,"static XkbModsRec preserve_%s[%d]= {\n",prefix,
							type->map_count);
    for (i=0,pre=type->preserve;i<type->map_count;i++,pre++) {
	if (i!=0)
	    fprintf(file,",\n");
	fprintf(file,"    {   %15s, ",XkbModMaskText(pre->mask,XkbCFile));
	fprintf(file,"%15s, ",XkbModMaskText(pre->real_mods,XkbCFile));
	fprintf(file,"%15s }",XkbVModMaskText(dpy,xkb,0,pre->vmods,XkbCFile));
    }
    fprintf(file,"\n};\n");
    return;
}

static void
WriteTypeInitFunc(file,dpy,xkb)
    FILE *	file;
    Display *	dpy;
    XkbDescPtr	xkb;
{
register unsigned 	i,n;
XkbKeyTypePtr		type;
Atom *			names;
char			prefix[32];

    fprintf(file,"\n\nstatic void\n#if NeedFunctionPrototypes\n");
    fprintf(file,"initTypeNames(DPYTYPE dpy)\n#else\n");
    fprintf(file,"initTypeNames(dpy)\nDPYTYPE dpy;\n#endif\n{\n");
    for (i=0,type=xkb->map->types;i<xkb->map->num_types;i++,type++) {
	strcpy(prefix,XkbAtomText(dpy,type->name,XkbCFile));
	if (type->name!=None)
	    fprintf(file,"    dflt_types[%d].name= GET_ATOM(dpy,\"%s\");\n",i,
					XkbAtomGetString(dpy,type->name));
	names= type->level_names;
	if (names!=NULL) {
	    char *tmp;
	    for (n=0;n<type->num_levels;n++) {
		if (names[n]==None)
		    continue;
		tmp= XkbAtomGetString(dpy,names[n]);
		if (tmp==NULL)
		    continue;
		fprintf(file,"    lnames_%s[%d]=	",prefix,n);
		fprintf(file,"GET_ATOM(dpy,\"%s\");\n",tmp);
	    }
	}
    }
    fprintf(file,"}\n");
    return;
}

static Bool
WriteCHdrKeyTypes(file,dpy,xkb)
    FILE *	file;
    Display *	dpy;
    XkbDescPtr	xkb;
{
register unsigned	i,n;
XkbClientMapPtr		map;
XkbKeyTypePtr		type;
char 			prefix[32];

    if ((!xkb)||(!xkb->map)||(!xkb->map->types)) {
	_XkbLibError(_XkbErrMissingTypes,"WriteCHdrKeyTypes",0);
	return False;
    }
    if (xkb->map->num_types<XkbNumRequiredTypes) {
	_XkbLibError(_XkbErrMissingReqTypes,"WriteCHdrKeyTypes",0);
	return 0;
    }
    map= xkb->map;
    if ((xkb->names!=NULL)&&(xkb->names->types!=None)) {
	fprintf(file,"/* types name is \"%s\" */\n",
				XkbAtomGetString(dpy,xkb->names->types));
    }
    for (i=0,type=map->types;i<map->num_types;i++,type++) {
	strcpy(prefix,XkbAtomText(dpy,type->name,XkbCFile));

	if (type->map_count>0) {
	    XkbKTMapEntryPtr	entry;
	    entry= type->map;
	    fprintf(file,"static XkbKTMapEntryRec map_%s[%d]= {\n",prefix,
							type->map_count);
	    for (n=0;n<(unsigned)type->map_count;n++,entry++) {
		if (n!=0)
		    fprintf(file,",\n");
		fprintf(file,"    { %d, %6d, { %15s, %15s, %15s } }",
			entry->active,
			entry->level,
			XkbModMaskText(entry->mods.mask,XkbCFile),
			XkbModMaskText(entry->mods.real_mods,XkbCFile),
			XkbVModMaskText(dpy,xkb,0,entry->mods.vmods,XkbCFile));
	    }
	    fprintf(file,"\n};\n");

	    if (type->preserve)
		WriteTypePreserve(file,dpy,prefix,xkb,type);
	}
	if (type->level_names!=NULL) {
	    fprintf(file,"static Atom lnames_%s[%d];\n",prefix,
							 type->num_levels);
	}
	fprintf(file,"\n");
    }
    fprintf(file,"static XkbKeyTypeRec dflt_types[]= {\n");
    for (i=0,type=map->types;i<(unsigned)map->num_types;i++,type++) {
	strcpy(prefix,XkbAtomText(dpy,type->name,XkbCFile));
	if (i!=0)	fprintf(file,",\n");
	fprintf(file,"    {\n	{ %15s, %15s, %15s },\n",
			XkbModMaskText(type->mods.mask,XkbCFile),
			XkbModMaskText(type->mods.real_mods,XkbCFile),
			XkbVModMaskText(dpy,xkb,0,type->mods.vmods,XkbCFile));
	fprintf(file,"	%d,\n",type->num_levels);
	fprintf(file,"	%d,",type->map_count);
	if (type->map_count>0)
	     fprintf(file,"	map_%s,",prefix);
	else fprintf(file,"	NULL,");
	if (type->preserve)
	     fprintf(file,"	preserve_%s,\n",prefix);
	else fprintf(file,"	NULL,\n");
	if (type->level_names!=NULL)
	     fprintf(file,"	None,	lnames_%s\n    }",prefix);
	else fprintf(file,"	None,	NULL\n    }",prefix);
    }
    fprintf(file,"\n};\n");
    fprintf(file,"#define num_dflt_types (sizeof(dflt_types)/sizeof(XkbKeyTypeRec))\n");
    WriteTypeInitFunc(file,dpy,xkb);
    return True;
}

static Bool
WriteCHdrCompatMap(file,dpy,xkb)
    FILE *	file;
    Display *	dpy;
    XkbDescPtr	xkb;
{
register unsigned	i;
XkbCompatMapPtr		compat;
XkbSymInterpretPtr	interp;

    if ((!xkb)||(!xkb->compat)||(!xkb->compat->sym_interpret)) {
	_XkbLibError(_XkbErrMissingSymInterps,"WriteCHdrInterp",0);
	return False;
    }
    compat= xkb->compat;
    if ((xkb->names!=NULL)&&(xkb->names->compat!=None)) {
	fprintf(file,"/* compat name is \"%s\" */\n",
				XkbAtomGetString(dpy,xkb->names->compat));
    }
    fprintf(file,"static XkbSymInterpretRec dfltSI[%d]= {\n",
						compat->num_si);
    interp= compat->sym_interpret;
    for (i=0;i<compat->num_si;i++,interp++) {
	XkbAction *act;
	act= (XkbAction *)&interp->act;
	if (i!=0)	fprintf(file,",\n");
	fprintf(file,"    {    %s, ",XkbKeysymText(interp->sym,XkbCFile));
	fprintf(file,"0x%04x,\n",interp->flags);
	fprintf(file,"         %s, ",XkbSIMatchText(interp->match,XkbCFile));
	fprintf(file,"%s,\n",XkbModMaskText(interp->mods,XkbCFile));
	fprintf(file,"         %d,\n",interp->virtual_mod);
	fprintf(file,"       %s }",
		XkbActionText(dpy,xkb,act,XkbCFile));
    }
    fprintf(file,"\n};\n");
    fprintf(file,
	   "#define num_dfltSI (sizeof(dfltSI)/sizeof(XkbSymInterpretRec))\n");
    fprintf(file,"\nstatic XkbCompatMapRec compatMap= {\n");
    fprintf(file,"    dfltSI,\n");
    fprintf(file,"    {   /* group compatibility */\n        ");
    for (i=0;i<XkbNumKbdGroups;i++) {
	XkbModsPtr gc;
	gc= &xkb->compat->groups[i];
	fprintf(file,"%s{ %12s, %12s, %12s }",
			((i==0)?"":",\n        "),
			XkbModMaskText(gc->mask,XkbCFile),
			XkbModMaskText(gc->real_mods,XkbCFile),
			XkbVModMaskText(xkb->dpy,xkb,0,gc->vmods,XkbCFile));
    }
    fprintf(file,"\n    },\n");
    fprintf(file,"    num_dfltSI, num_dfltSI\n");
    fprintf(file,"};\n\n");
    return True;
}

static Bool
WriteCHdrSymbols(file,xkb)
    FILE *	file;
    XkbDescPtr	xkb;
{
register unsigned i;

    if ((!xkb)||(!xkb->map)||(!xkb->map->syms)||(!xkb->map->key_sym_map)) {
	_XkbLibError(_XkbErrMissingSymbols,"WriteCHdrSymbols",0);
	return False;
    }
    fprintf(file,"#define NUM_SYMBOLS	%d\n",xkb->map->num_syms);
    if (xkb->map->num_syms>0) {
	register KeySym *sym;
	sym= xkb->map->syms;
	fprintf(file,"static KeySym	symCache[NUM_SYMBOLS]= {\n");
	for (i=0;i<xkb->map->num_syms;i++,sym++) {
	    if (i==0)		fprintf(file,"    ");
	    else if (i%4==0)	fprintf(file,",\n    ");
	    else		fprintf(file,", ");
	    fprintf(file,"%15s",XkbKeysymText(*sym,XkbCFile));
	}
	fprintf(file,"\n};\n");
    }
    if (xkb->max_key_code>0) {
	register XkbSymMapPtr	map;
	map= xkb->map->key_sym_map;
	fprintf(file,"static XkbSymMapRec	symMap[NUM_KEYS]= {\n");
	for (i=0;i<=xkb->max_key_code;i++,map++) {
	    if (i==0)		fprintf(file,"    ");
	    else if ((i&3)==0)	fprintf(file,",\n    ");
	    else			fprintf(file,", ");
	    fprintf(file,"{ %2d, 0x%x, %3d }",map->kt_index,map->group_info,
							 map->offset);
	}
	fprintf(file,"\n};\n");
    }
    return True;
}

static Bool
WriteCHdrClientMap(file,dpy,xkb)
    FILE *	file;
    Display *	dpy;
    XkbDescPtr	xkb;
{
    if ((!xkb)||(!xkb->map)||(!xkb->map->syms)||(!xkb->map->key_sym_map)) {
	_XkbLibError(_XkbErrMissingSymbols,"WriteCHdrClientMap",0);
	return False;
    }
    if (!WriteCHdrKeyTypes(file,dpy,xkb))
	return False;
    if (!WriteCHdrSymbols(file,xkb))
	return False;
    fprintf(file,"static XkbClientMapRec clientMap= {\n");
    fprintf(file,"    NUM_TYPES,   NUM_TYPES,   types, \n");
    fprintf(file,"    NUM_SYMBOLS, NUM_SYMBOLS, symCache, symMap\n");
    fprintf(file,"};\n\n");
    return True;
}

static Bool
WriteCHdrServerMap(file,dpy,xkb)
    FILE *	file;
    Display *	dpy;
    XkbDescPtr	xkb;
{
register unsigned i;

    if ((!xkb)||(!xkb->map)||(!xkb->map->syms)||(!xkb->map->key_sym_map)) {
	_XkbLibError(_XkbErrMissingSymbols,"WriteCHdrServerMap",0);
	return False;
    }
    if (xkb->server->num_acts>0) {
	register XkbAnyAction *act;
	act= (XkbAnyAction *)xkb->server->acts;
	fprintf(file,"#define NUM_ACTIONS	%d\n",xkb->server->num_acts);
	fprintf(file,"static XkbAnyAction 	actionCache[NUM_ACTIONS]= {\n");
	for (i=0;i<xkb->server->num_acts;i++,act++) {
	    if (i==0)	fprintf(file,"    ");
	    else	fprintf(file,",\n    ");
#ifdef NOLONGER
	    fprintf(file,"{ %20s, ",XkbActionTypeText(act->type,XkbCFile));
	    fprintf(file," 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x }",
			act->data[0], act->data[1], act->data[2], act->data[3],
			act->data[4], act->data[5], act->data[6]);
#else
	    fprintf(file,"%s",XkbActionText(dpy,xkb,(XkbAction *)act,XkbCFile));
#endif
	}
	fprintf(file,"\n};\n");
    }
    fprintf(file,"static unsigned short	keyActions[NUM_KEYS]= {\n");
    for (i=0;i<=xkb->max_key_code;i++) {
	if (i==0)		fprintf(file,"    ");
	else if ((i&0xf)==0)	fprintf(file,",\n    ");
	else			fprintf(file,", ");
	fprintf(file,"%2d",xkb->server->key_acts[i]);
    }
    fprintf(file,"\n};\n");
    fprintf(file,"static XkbBehavior behaviors[NUM_KEYS]= {\n");
    for (i=0;i<=xkb->max_key_code;i++) {
	if (i==0)		fprintf(file,"    ");
	else if ((i&0x3)==0)	fprintf(file,",\n    ");
	else			fprintf(file,", ");
	if (xkb->server->behaviors) {
	     fprintf(file,"%s",
		XkbBehaviorText(xkb,&xkb->server->behaviors[i],XkbCFile));
	}
	else fprintf(file,"{    0,    0 }");
    }
    fprintf(file,"\n};\n");
    fprintf(file,"static unsigned char explicit[NUM_KEYS]= {\n");
    for (i=0;i<=xkb->max_key_code;i++) {
	if (i==0)		fprintf(file,"    ");
	else if ((i&0x7)==0)	fprintf(file,",\n    ");
	else			fprintf(file,", ");
	if ((xkb->server->explicit==NULL)||(xkb->server->explicit[i]==0))
	     fprintf(file,"   0");
	else fprintf(file,"0x%02x",xkb->server->explicit[i]);
    }
    fprintf(file,"\n};\n");
    fprintf(file,"static unsigned short vmodmap[NUM_KEYS]= {\n");
    for (i=0;i<xkb->max_key_code;i++) {
	if (i==0)		fprintf(file,"    ");
	else if ((i&0x7)==0)	fprintf(file,",\n    ");
	else			fprintf(file,", ");
	if ((xkb->server->vmodmap==NULL)||(xkb->server->vmodmap[i]==0))
	     fprintf(file,"     0");
	else fprintf(file,"0x%04x",xkb->server->vmodmap[i]);
    }
    fprintf(file,"};\n");
    fprintf(file,"static XkbServerMapRec serverMap= {\n");
    fprintf(file,"    %d, %d, (XkbAction *)actionCache,\n",
				xkb->server->num_acts,xkb->server->num_acts);
    fprintf(file,"    behaviors, keyActions, explicit,\n");
    for (i=0;i<XkbNumVirtualMods;i++) {
	if (i==0)	fprintf(file,"    { ");
	else if (i==8)	fprintf(file,",\n      ");
	else		fprintf(file,", ");
	fprintf(file,"%3d",xkb->server->vmods[i]);
    }
    fprintf(file," },\n");
    fprintf(file,"    vmodmap\n");
    fprintf(file,"};\n\n");
    return True;
}

static Bool
WriteCHdrIndicators(file,dpy,xkb)
    FILE *	file;
    Display *	dpy;
    XkbDescPtr	xkb;
{
register int 		i,nNames;
XkbIndicatorMapPtr	imap;

    if (xkb->indicators==NULL)
	return True;
    fprintf(file,"static XkbIndicatorRec indicators= {\n");
    fprintf(file,"    0x%x,\n    {\n",xkb->indicators->phys_indicators);
    for (imap=xkb->indicators->maps,i=nNames=0;i<XkbNumIndicators;i++,imap++) {
	fprintf(file,"%s        { 0x%02x, %s, 0x%02x, %s, %s, ",
			(i!=0?",\n":""),
			imap->flags,
			XkbIMWhichStateMaskText(imap->which_groups,XkbCFile),
			imap->groups,
			XkbIMWhichStateMaskText(imap->which_mods,XkbCFile),
			XkbModMaskText(imap->mods.mask,XkbCFile));
	fprintf(file," %s, %s, %s }",
			XkbModMaskText(imap->mods.real_mods,XkbCFile),
			XkbVModMaskText(dpy,xkb,0,imap->mods.vmods,XkbCFile),
			XkbControlsMaskText(imap->ctrls,XkbCFile));
	if (xkb->names && (xkb->names->indicators[i]!=None))
	    nNames++;
    }
    fprintf(file,"\n    }\n};\n");
    if (nNames>0) {
	fprintf(file,"static void\n#if NeedFunctionPrototypes\n");
	fprintf(file,"initIndicatorNames(DPYTYPE dpy,XkbDescPtr xkb)\n");
	fprintf(file,"#else\ninitIndicatorNames(dpy,xkb)\n");
	fprintf(file,"    DPYTYPE dpy;\n");
	fprintf(file,"    XkbDescPtr xkb;\n#endif\n{\n");
	for (i=0;i<XkbNumIndicators;i++) {
	    Atom name;
	    if (xkb->names->indicators[i]==None)
		continue;
	    name= xkb->names->indicators[i];
	    fprintf(file,"    xkb->names->indicators[%2d]=	",i);
	    fprintf(file,"GET_ATOM(dpy,\"%s\");\n",XkbAtomGetString(dpy,name));
	}
	fprintf(file,"}\n");
    }
    return True;
}

static Bool
WriteCHdrLayout(file,result)
    FILE *		file;
    XkbFileInfo *	result;
{
Bool		ok;
XkbDescPtr	xkb;

    xkb= result->xkb;
    ok= WriteCHdrVMods(file,xkb->dpy,xkb);
    ok= WriteCHdrKeycodes(file,xkb)&&ok;
    ok= WriteCHdrSymbols(file,xkb)&&ok;
#ifdef NOTDEF
    ok= WriteCHdrGeometry(file,xkb->dpy,xkb)&&ok;
#endif
    return ok;
}

static Bool
WriteCHdrSemantics(file,result)
    FILE *		file;
    XkbFileInfo *	result;
{
Bool		ok;
XkbDescPtr	xkb;

    xkb= result->xkb;
    ok= WriteCHdrVMods(file,xkb->dpy,xkb);
    ok= WriteCHdrKeyTypes(file,xkb->dpy,xkb)&&ok;
    ok= WriteCHdrCompatMap(file,xkb->dpy,xkb)&&ok;
    ok= WriteCHdrIndicators(file,xkb->dpy,xkb)&&ok;
    return ok;
}

static Bool
WriteCHdrKeymap(file,result)
    FILE *		file;
    XkbFileInfo *	result;
{
Bool		ok;
XkbDescPtr	xkb;

    xkb= result->xkb;
    ok= WriteCHdrVMods(file,xkb->dpy,xkb);
    ok= ok&&WriteCHdrKeycodes(file,xkb);
    ok= ok&&WriteCHdrClientMap(file,xkb->dpy,xkb);
    ok= ok&&WriteCHdrServerMap(file,xkb->dpy,xkb);
    ok= ok&&WriteCHdrCompatMap(file,xkb->dpy,xkb);
    ok= WriteCHdrIndicators(file,xkb->dpy,xkb)&&ok;
#ifdef NOTDEF
    ok= ok&&WriteCHdrGeometry(file,xkb->dpy,xkb);
#endif
    return ok;
}

Bool
XkbWriteCFile(out,name,result)
    FILE *		out;
    char *		name;
    XkbFileInfo *	result;
{
Bool	 		ok;
Bool			(*func)();
XkbDescPtr		xkb;

    switch (result->type) {
	case XkmSemanticsFile:
	    func= WriteCHdrSemantics;
	    break;
	case XkmLayoutFile:
	    func= WriteCHdrLayout;
	    break;
	case XkmKeymapFile:
	    func= WriteCHdrKeymap;
	    break;
	default:
	    _XkbLibError(_XkbErrIllegalContents,"XkbWriteCFile",result->type);
	    return False;
    }
    xkb= result->xkb;
    if (out==NULL) {
	_XkbLibError(_XkbErrFileCannotOpen,"XkbWriteCFile",0);
	ok= False;
    }
    else {
	char *tmp,*hdrdef;
	tmp= (char *)strrchr(name,'/');
	if (tmp==NULL)
	     tmp= name;
	else tmp++;
	hdrdef= (char *)calloc(strlen(tmp+1),sizeof(char));
	if (hdrdef) {
	    strcpy(hdrdef,tmp);
	    tmp= hdrdef;
	    while (*tmp) {
		if (islower(*tmp))		*tmp= toupper(*tmp);
		else if (!isalnum(*tmp))	*tmp= '_';
		tmp++;
	    }
	    fprintf(out,"/* This file generated automatically by xkbcomp */\n");
	    fprintf(out,"/* DO  NOT EDIT */\n");
	    fprintf(out,"#ifndef %s\n",hdrdef);
	    fprintf(out,"#define %s 1\n\n",hdrdef);
	}
	fprintf(out,"#ifndef XKB_IN_SERVER\n");
	fprintf(out,"#define GET_ATOM(d,s)	XInternAtom(d,s,0)\n");
	fprintf(out,"#define DPYTYPE	Display *\n");
	fprintf(out,"#else\n");
	fprintf(out,"#define GET_ATOM(d,s)	MakeAtom(s,strlen(s),1)\n");
	fprintf(out,"#define DPYTYPE	char *\n");
	fprintf(out,"#endif\n");
	fprintf(out,"#define NUM_KEYS	%d\n",xkb->max_key_code+1);
	ok= (*func)(out,result);
	if (hdrdef)
	    fprintf(out,"#endif /* %s */\n",hdrdef);
    }

    if (!ok) {
	return False;
    }
    return True;
}

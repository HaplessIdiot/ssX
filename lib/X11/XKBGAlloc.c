/* $XConsortium $ */
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

#define	NEED_EVENTS
#define	NEED_REPLIES

#ifndef XKB_IN_SERVER

#include <stdio.h>
#include "Xlibint.h"
#include "XKBlibint.h"
#include <X11/extensions/XKBgeom.h>
#include <X11/extensions/XKBproto.h>

#else 

#include <stdio.h>
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "inputstr.h"
#include "XKBsrv.h"
#include "XKBgeom.h"

#endif /* XKB_IN_SERVER */

#ifdef X_NOT_POSIX
#define Size_t unsigned int
#else
#define Size_t size_t
#endif

/***====================================================================***/

void
XkbFreeGeomProperties(geom,first,count,freeAll)
    XkbGeometryPtr	geom;
    int			first;
    int			count;
    Bool		freeAll;
{
XkbPropertyPtr	prop;
register int	i;

    if (freeAll) {
	first= 0;
	count= geom->num_properties;
    }
    else if ((first>=geom->num_properties)||(first<0)||(count<1))
	return;
    else if (first+count>geom->num_properties)
	count= geom->num_properties-first;

    for (i=0,prop=&geom->properties[first];i<count;i++,prop++) {
	if (prop->name) {
	    _XkbFree(prop->name);
	    prop->name= NULL;
	}
	if (prop->value) {
	    _XkbFree(prop->value);
	    prop->value= NULL;
	}
    }
    if (freeAll) {
	_XkbFree(geom->properties);
	geom->properties= NULL;
	geom->sz_properties= geom->num_properties= 0;
    }
    else {
	for (i=0,prop=geom->properties;i<geom->num_properties;i++) {
	    register int n;
	    if (prop->name==NULL) {
		Bool match= False;
		for (n=i+1;(n<geom->num_properties)&&(!match);n++) {
		    if (geom->properties[n].name!=NULL) {
			prop->name= geom->properties[n].name;
			prop->value= geom->properties[n].value;
			geom->properties[n].name= NULL;
			geom->properties[n].value= NULL;
			match= True;
		    }
		}
		if (!match)
		    geom->num_properties= i+1;
	    }
	}
    }
    return;
}

void
XkbFreeGeomKeyAliases(geom,first,count,freeAll)
    XkbGeometryPtr	geom;
    int			first;
    int			count;
    Bool		freeAll;
{
    if (freeAll) {
	if (geom->key_aliases)
	    _XkbFree(geom->key_aliases);
	geom->key_aliases= NULL;
	geom->num_key_aliases= geom->sz_key_aliases= 0;
	return;
    }
    if ((first<0)||(first>=geom->num_key_aliases)||(geom->key_aliases==NULL))
	return;
    else if (first+count>geom->num_key_aliases)
	count= geom->num_colors-first;
    if (count<1)
	return;

    if (first+count<geom->num_key_aliases) {
	int extra;
	extra= geom->num_key_aliases-(first+count);
	memmove(&geom->key_aliases[first],&geom->key_aliases[first+count],
						extra*sizeof(XkbKeyAliasRec));
    }
    geom->num_key_aliases-= count;
    return;
}

void
XkbFreeGeomColors(geom,first,count,freeAll)
    XkbGeometryPtr	geom;
    int			first;
    int			count;
    Bool		freeAll;
{
XkbColorPtr	color;
register int	i;

    if (freeAll) {
	first= 0;
	count= geom->num_colors;
    }
    else if ((first>=geom->num_colors)||(first<0)||(count<1))
	return;
    else if (first+count>geom->num_colors)
	count= geom->num_colors-first;

    if (geom->colors==NULL)
	return;

    for (i=0,color=&geom->colors[first];i<count;i++,color++) {
	if (color->spec)
	    _XkbFree(color->spec);
	color->spec= NULL;
    }
    if (freeAll) {
	_XkbFree(geom->colors);
	geom->colors= NULL;
	geom->sz_colors= geom->num_colors= 0;
    }
    else {
	for (i=0,color=geom->colors;i<geom->num_colors;i++) {
	    register int n;
	    if (color->spec==NULL) {
		Bool match= False;
		for (n=i+1;(n<geom->num_colors)&&(!match);n++) {
		    if (geom->colors[n].spec!=NULL) {
			color->spec= geom->colors[n].spec;
			geom->colors[n].spec= NULL;
			match= True;
		    }
		}
		if (!match)
		    geom->num_colors= i+1;
	    }
	}
    }
    return;
}

void
XkbFreeGeomShapes(geom,first,count,freeAll)
    XkbGeometryPtr	geom;
    int			first;
    int			count;
    Bool		freeAll;
{
XkbShapePtr	shape;
register int	i;

    if (freeAll) {
	first= 0;
	count= geom->num_shapes;
    }
    else if ((first>=geom->num_shapes)||(first<0)||(count<1))
	return;
    else if (first+count>geom->num_shapes)
	count= geom->num_shapes-first;

    if (geom->shapes==NULL)
	return;

    for (i=0,shape=&geom->shapes[first];i<count;i++,shape++) {
	if (shape->outlines) {
	    register int 	o;
	    XkbOutlinePtr	ol;
	    for (o=0,ol=shape->outlines;o<shape->num_outlines;o++,ol++) {
		if (ol->points)
		    _XkbFree(ol->points);
		ol->points= NULL;
		ol->num_points= ol->sz_points= 0;
	    }
	    _XkbFree(shape->outlines);
	}
	bzero(shape,sizeof(XkbShapeRec));
    }
    if (freeAll) {
	_XkbFree(geom->shapes);
	geom->shapes= NULL;
	geom->num_shapes= geom->sz_shapes= 0;
    }
    return;
}

void
XkbFreeGeomSections(geom,first,count,freeAll)
    XkbGeometryPtr	geom;
    int			first;
    int			count;
    Bool		freeAll;
{
XkbSectionPtr	section;
register int	i;

    if (freeAll) {
	first= 0;
	count= geom->num_sections;
    }
    else if ((first>=geom->num_sections)||(first<0)||(count<1))
	return;
    else if (first+count>geom->num_sections)
	count= geom->num_sections-first;

    if (geom->sections==NULL)
	return;

    for (i=0,section=&geom->sections[first];i<count;i++,section++) {
	if (section->rows) {
	    register int	r;
	    XkbRowPtr	row;
	    for (r=0,row=section->rows;r<section->num_rows;r++,row++) {
		if (row->keys)
		    _XkbFree(row->keys);
		row->keys= NULL;
		row->num_keys= row->sz_keys= 0;
	    }
	    _XkbFree(section->rows);
	}
	section->rows= NULL;
	section->num_rows= section->sz_rows= 0;
	if (section->doodads) {
	    XkbFreeGeomDoodads(section->doodads,section->num_doodads,True);
	}
	section->doodads= NULL;
	section->num_doodads= section->sz_doodads= 0;
    }
    if (freeAll) {
	_XkbFree(geom->sections);
	geom->sections= NULL;
	geom->num_sections= geom->sz_sections= 0;
    }
    return;
}

void
XkbFreeGeomDoodads(doodads,nDoodads,freeAll)
    XkbDoodadPtr	doodads;
    int			nDoodads;
    Bool		freeAll;
{
register int 		i;
register XkbDoodadPtr	doodad;

    if (doodads) {
	for (i=0,doodad= doodads;i<nDoodads;i++,doodad++) {
	    if (doodad->any.type==XkbTextDoodad) {
		if (doodad->text.text!=NULL) {
		    _XkbFree(doodad->text.text);
		    doodad->text.text= NULL;
		}
		if (doodad->text.font!=NULL) {
		    _XkbFree(doodad->text.font);
		    doodad->text.font= NULL;
		}
	    }
	}
	if (freeAll)
	    _XkbFree(doodads);
    }
    return;
}


void
XkbFreeGeometry(geom,which,freeMap)
    XkbGeometryPtr	geom;
    unsigned		which;
    Bool		freeMap;
{
    if (geom==NULL)
	return;
    if (freeMap)
	which= XkbGeomAllMask;
    if ((which&XkbGeomPropertiesMask)&&(geom->properties!=NULL))
	XkbFreeGeomProperties(geom,0,geom->num_properties,True);
    if ((which&XkbGeomColorsMask)&&(geom->colors!=NULL))
	XkbFreeGeomColors(geom,0,geom->num_colors,True);
    if ((which&XkbGeomShapesMask)&&(geom->shapes!=NULL))
	XkbFreeGeomShapes(geom,0,geom->num_shapes,True);
    if ((which&XkbGeomSectionsMask)&&(geom->sections!=NULL))
	XkbFreeGeomSections(geom,0,geom->num_sections,True);
    if ((which&XkbGeomDoodadsMask)&&(geom->doodads!= NULL)) {
	XkbFreeGeomDoodads(geom->doodads,geom->num_doodads,True);
	geom->doodads= NULL;
	geom->num_doodads= geom->sz_doodads= 0;
    }
    if ((which&XkbGeomKeyAliasesMask)&&(geom->key_aliases!=NULL))
	XkbFreeGeomKeyAliases(geom,0,geom->num_key_aliases,True);
    if (freeMap) {
	if (geom->label_font!=NULL)
	    _XkbFree(geom->label_font);
	_XkbFree(geom);
    }
    return;
}

/***====================================================================***/

static Status
_XkbGeomAlloc(old,num,total,num_new,sz_elem)
    XPointer *		old;
    unsigned short *	num;
    unsigned short *	total;
    int			num_new;
    Size_t		sz_elem;
{
    if (num_new<1)
	return Success;
    if ((*old)==NULL)
	*num= *total= 0;

    if ((*num)+num_new<=(*total))
	return Success;

    *total= (*num)+num_new;
    if ((*old)!=NULL)
	 (*old)= (XPointer)_XkbRealloc((*old),(*total)*sz_elem);
    else (*old)= (XPointer)_XkbCalloc((*total),sz_elem);
    if ((*old)==NULL) {
	*total= *num= 0;
	return BadAlloc;
    }

    if (*num>0) {
	char *tmp= (char *)(*old);
	bzero(&tmp[sz_elem*(*num)],(num_new*sz_elem));
    }
    return Success;
}

#define	_XkbAllocProps(g,n) _XkbGeomAlloc((XPointer *)&(g)->properties,\
				&(g)->num_properties,&(g)->sz_properties,\
				(n),sizeof(XkbPropertyRec))
#define	_XkbAllocColors(g,n) _XkbGeomAlloc((XPointer *)&(g)->colors,\
				&(g)->num_colors,&(g)->sz_colors,\
				(n),sizeof(XkbColorRec))
#define	_XkbAllocShapes(g,n) _XkbGeomAlloc((XPointer *)&(g)->shapes,\
				&(g)->num_shapes,&(g)->sz_shapes,\
				(n),sizeof(XkbShapeRec))
#define	_XkbAllocSections(g,n) _XkbGeomAlloc((XPointer *)&(g)->sections,\
				&(g)->num_sections,&(g)->sz_sections,\
				(n),sizeof(XkbSectionRec))
#define	_XkbAllocDoodads(g,n) _XkbGeomAlloc((XPointer *)&(g)->doodads,\
				&(g)->num_doodads,&(g)->sz_doodads,\
				(n),sizeof(XkbDoodadRec))
#define	_XkbAllocKeyAliases(g,n) _XkbGeomAlloc((XPointer *)&(g)->key_aliases,\
				&(g)->num_key_aliases,&(g)->sz_key_aliases,\
				(n),sizeof(XkbKeyAliasRec))

#define	_XkbAllocOutlines(s,n) _XkbGeomAlloc((XPointer *)&(s)->outlines,\
				&(s)->num_outlines,&(s)->sz_outlines,\
				(n),sizeof(XkbOutlineRec))
#define	_XkbAllocRows(s,n) _XkbGeomAlloc((XPointer *)&(s)->rows,\
				&(s)->num_rows,&(s)->sz_rows,\
				(n),sizeof(XkbRowRec))
#define	_XkbAllocPoints(o,n) _XkbGeomAlloc((XPointer *)&(o)->points,\
				&(o)->num_points,&(o)->sz_points,\
				(n),sizeof(XkbPointRec))
#define	_XkbAllocKeys(r,n) _XkbGeomAlloc((XPointer *)&(r)->keys,\
				&(r)->num_keys,&(r)->sz_keys,\
				(n),sizeof(XkbKeyRec))
#define	_XkbAllocOverlays(s,n) _XkbGeomAlloc((XPointer *)&(s)->overlays,\
				&(s)->num_overlays,&(s)->sz_overlays,\
				(n),sizeof(XkbOverlayRec))
#define	_XkbAllocOverlayRows(o,n) _XkbGeomAlloc((XPointer *)&(o)->rows,\
				&(o)->num_rows,&(o)->sz_rows,\
				(n),sizeof(XkbOverlayRowRec))
#define	_XkbAllocOverlayKeys(r,n) _XkbGeomAlloc((XPointer *)&(r)->keys,\
				&(r)->num_keys,&(r)->sz_keys,\
				(n),sizeof(XkbOverlayKeyRec))
    
Status
XkbAllocGeomProps(geom,nProps)
    XkbGeometryPtr	geom;
    int			nProps;
{
    return _XkbAllocProps(geom,nProps);
}

Status
XkbAllocGeomColors(geom,nColors)
    XkbGeometryPtr	geom;
    int			nColors;
{
    return _XkbAllocColors(geom,nColors);
}

Status
XkbAllocGeomKeyAliases(geom,nKeyAliases)
    XkbGeometryPtr	geom;
    int			nKeyAliases;
{
    return _XkbAllocKeyAliases(geom,nKeyAliases);
}

Status
XkbAllocGeomShapes(geom,nShapes)
    XkbGeometryPtr	geom;
    int			nShapes;
{
    return _XkbAllocShapes(geom,nShapes);
}

Status
XkbAllocGeomSections(geom,nSections)
    XkbGeometryPtr	geom;
    int			nSections;
{
    return _XkbAllocSections(geom,nSections);
}

Status
XkbAllocGeomDoodads(geom,nDoodads)
    XkbGeometryPtr	geom;
    int			nDoodads;
{
    return _XkbAllocDoodads(geom,nDoodads);
}

Status
XkbAllocGeomSectionDoodads(section,nDoodads)
    XkbSectionPtr	section;
    int			nDoodads;
{
    return _XkbAllocDoodads(section,nDoodads);
}

Status
XkbAllocGeomOutlines(shape,nOL)
    XkbShapePtr		shape;
    int			nOL;
{
    return _XkbAllocOutlines(shape,nOL);
}

Status
XkbAllocGeomRows(section,nRows)
    XkbSectionPtr	section;
    int			nRows;
{
    return _XkbAllocRows(section,nRows);
}

Status
XkbAllocGeomPoints(ol,nPts)
    XkbOutlinePtr	ol;
    int			nPts;
{
    return _XkbAllocPoints(ol,nPts);
}

Status
XkbAllocGeomKeys(row,nKeys)
    XkbRowPtr		row;
    int			nKeys;
{
    return _XkbAllocKeys(row,nKeys);
}

Status
XkbAllocGeometry(xkb,sizes)
    XkbDescPtr		xkb;
    XkbGeometrySizesPtr	sizes;
{
XkbGeometryPtr	geom;
Status		rtrn;

    if (xkb->geom==NULL) {
	xkb->geom= _XkbTypedCalloc(1,XkbGeometryRec);
	if (!xkb->geom)
	    return BadAlloc;
    }
    geom= xkb->geom;
    if ((sizes->which&XkbGeomPropertiesMask)&&
	((rtrn=_XkbAllocProps(geom,sizes->num_properties))!=Success)) {
	goto BAIL;
    }
    if ((sizes->which&XkbGeomColorsMask)&&
	((rtrn=_XkbAllocColors(geom,sizes->num_colors))!=Success)) {
	goto BAIL;
    }
    if ((sizes->which&XkbGeomShapesMask)&&
	((rtrn=_XkbAllocShapes(geom,sizes->num_shapes))!=Success)) {
	goto BAIL;
    }
    if ((sizes->which&XkbGeomSectionsMask)&&
	((rtrn=_XkbAllocSections(geom,sizes->num_sections))!=Success)) {
	goto BAIL;
    }
    if ((sizes->which&XkbGeomDoodadsMask)&&
	((rtrn=_XkbAllocDoodads(geom,sizes->num_doodads))!=Success)) {
	goto BAIL;
    }
    if ((sizes->which&XkbGeomKeyAliasesMask)&&
	((rtrn=_XkbAllocKeyAliases(geom,sizes->num_key_aliases))!=Success)) {
	goto BAIL;
    }
    return Success;
BAIL:
    XkbFreeGeometry(geom,XkbGeomAllMask,True);
    xkb->geom= NULL;
    return rtrn;
}

/***====================================================================***/

XkbPropertyPtr
XkbAddGeomProperty(geom,name,value)
    XkbGeometryPtr	geom;
    char *		name;
    char *		value;
{
register int i;
register XkbPropertyPtr prop;

    if ((!geom)||(!name)||(!value))
	return NULL;
    for (i=0,prop=geom->properties;i<geom->num_properties;i++,prop++) {
	if ((prop->name)&&(strcmp(name,prop->name)==0)) {
	    if (prop->value)
		Xfree(prop->value);
	    prop->value= (char *)_XkbAlloc(strlen(value)+1);
	    if (prop->value)
		strcpy(prop->value,value);
	    return prop;
	}    
    }
    if ((geom->num_properties>=geom->sz_properties)&&
					(_XkbAllocProps(geom,1)!=Success)) {
	return NULL;
    }
    prop= &geom->properties[geom->num_properties];
    prop->name= (char *)_XkbAlloc(strlen(name)+1);
    if (!name)
	return NULL;
    strcpy(prop->name,name);
    prop->value= (char *)_XkbAlloc(strlen(value)+1);
    if (!value) {
	Xfree(prop->name);
	prop->name= NULL;
	return NULL;
    }
    strcpy(prop->value,value);
    geom->num_properties++;
    return prop;
}

XkbKeyAliasPtr
XkbAddGeomKeyAlias(geom,aliasStr,realStr)
    XkbGeometryPtr	geom;
    char *		aliasStr;
    char *		realStr;
{
register int i;
register XkbKeyAliasPtr alias;

    if ((!geom)||(!aliasStr)||(!realStr)||(!aliasStr[0])||(!realStr[0]))
	return NULL;
    for (i=0,alias=geom->key_aliases;i<geom->num_key_aliases;i++,alias++) {
	if (strncmp(alias->alias,aliasStr,XkbKeyNameLength)==0) {
	    bzero(alias->real,XkbKeyNameLength);
	    strncpy(alias->real,realStr,XkbKeyNameLength);
	    return alias;
	}
    }
    if ((geom->num_key_aliases>=geom->sz_key_aliases)&&
				(_XkbAllocKeyAliases(geom,1)!=Success)) {
	return NULL;
    }
    alias= &geom->key_aliases[geom->num_key_aliases];
    bzero(alias,sizeof(XkbKeyAliasRec));
    strncpy(alias->alias,aliasStr,XkbKeyNameLength);
    strncpy(alias->real,realStr,XkbKeyNameLength);
    geom->num_key_aliases++;
    return alias;
}

XkbColorPtr
XkbAddGeomColor(geom,spec)
    XkbGeometryPtr	geom;
    char *		spec;
{
register int i;
register XkbColorPtr color;

    if ((!geom)||(!spec))
	return NULL;
    for (i=0,color=geom->colors;i<geom->num_colors;i++,color++) {
	if ((color->spec)&&(strcmp(color->spec,spec)==0))
	    return color;
    }
    if ((geom->num_colors>=geom->sz_colors)&&
					(_XkbAllocColors(geom,1)!=Success)) {
	return NULL;
    }
    color= &geom->colors[geom->num_colors];
    color->pixel= geom->num_colors;
    color->spec= (char *)_XkbAlloc(strlen(spec)+1);
    if (!color->spec)
	return NULL;
    strcpy(color->spec,spec);
    geom->num_colors++;
    return color;
}

XkbOutlinePtr
XkbAddGeomOutline(geom,shape,sz_points)
    XkbGeometryPtr	geom;
    XkbShapePtr		shape;
    int			sz_points;
{
XkbOutlinePtr	outline;

    if ((!geom)||(!shape)||(sz_points<0))
	return NULL;
    if ((shape->num_outlines>=shape->sz_outlines)&&
					(_XkbAllocOutlines(shape,1)!=Success)) {
	return NULL;
    }
    outline= &shape->outlines[shape->num_outlines];
    bzero(outline,sizeof(XkbOutlineRec));
    if ((sz_points>0)&&(_XkbAllocPoints(outline,sz_points)!=Success))
	return NULL;
    shape->num_outlines++;
    return outline;
}

XkbShapePtr
XkbAddGeomShape(geom,name,sz_outlines)
    XkbGeometryPtr	geom;
    Atom		name;
    int			sz_outlines;
{
XkbShapePtr	shape;
register int	i;

    if ((!geom)||(!name)||(sz_outlines<0))
	return NULL;
    if (geom->num_shapes>0) {
	for (shape=geom->shapes,i=0;i<geom->num_shapes;i++,shape++) {
	    if (name==shape->name)
		return shape;
	}
    }
    if ((geom->num_shapes>=geom->sz_shapes)&&
					(_XkbAllocShapes(geom,1)!=Success))
	return NULL;
    shape= &geom->shapes[geom->num_shapes];
    bzero(shape,sizeof(XkbShapeRec));
    if ((sz_outlines>0)&&(_XkbAllocOutlines(shape,sz_outlines)!=Success))
	return NULL;
    shape->name= name;
    shape->primary= shape->approx= NULL;
    geom->num_shapes++;
    return shape;
}

XkbKeyPtr
XkbAddGeomKey(geom,row)
    XkbGeometryPtr	geom;
    XkbRowPtr		row;
{
XkbKeyPtr	key;
    if ((!geom)||(!row))
	return NULL;
    if ((row->num_keys>=row->sz_keys)&&(_XkbAllocKeys(row,1)!=Success))
	return NULL;
    key= &row->keys[row->num_keys++];
    bzero(key,sizeof(XkbKeyRec));
    return key;
}

XkbRowPtr
XkbAddGeomRow(geom,section,sz_keys)
    XkbGeometryPtr	geom;
    XkbSectionPtr	section;
    int			sz_keys;
{
XkbRowPtr	row;

    if ((!geom)||(!section)||(sz_keys<0))
	return NULL;
    if ((section->num_rows>=section->sz_rows)&&
    					(_XkbAllocRows(section,1)!=Success))
	return NULL;
    row= &section->rows[section->num_rows];
    bzero(row,sizeof(XkbRowRec));
    if ((sz_keys>0)&&(_XkbAllocKeys(row,sz_keys)!=Success))
	return NULL;
    section->num_rows++;
    return row;
}

XkbSectionPtr
XkbAddGeomSection(geom,name,sz_rows,sz_doodads,sz_over)
    XkbGeometryPtr	geom;
    Atom		name;
    int			sz_rows;
    int			sz_doodads;
    int			sz_over;
{
register int	i;
XkbSectionPtr	section;

    if ((!geom)||(name==None)||(sz_rows<0))
	return NULL;
    for (i=0,section=geom->sections;i<geom->num_sections;i++,section++) {
	if (section->name!=name)
	    continue;
	if (((sz_rows>0)&&(_XkbAllocRows(section,sz_rows)!=Success))||
	    ((sz_doodads>0)&&(_XkbAllocDoodads(section,sz_doodads)!=Success))||
	    ((sz_over>0)&&(_XkbAllocOverlays(section,sz_over)!=Success)))
	    return NULL;
	return section;
    }
    if ((geom->num_sections>=geom->sz_sections)&&
					(_XkbAllocSections(geom,1)!=Success))
	return NULL;
    section= &geom->sections[geom->num_sections];
    if ((sz_rows>0)&&(_XkbAllocRows(section,sz_rows)!=Success))
	return NULL;
    if ((sz_doodads>0)&&(_XkbAllocDoodads(section,sz_doodads)!=Success)) {
	if (section->rows) {
	    Xfree(section->rows);
	    section->rows= NULL;
	    section->sz_rows= section->num_rows= 0;
	}
	return NULL;
    }
    section->name= name;
    geom->num_sections++;
    return section;
}

XkbDoodadPtr
XkbAddGeomDoodad(geom,section,name)
    XkbGeometryPtr	geom;
    XkbSectionPtr	section;
    Atom		name;
{
XkbDoodadPtr	old,doodad;
register int	i,nDoodads;

    if ((!geom)||(name==None))
	return NULL;
    if ((section!=NULL)&&(section->num_doodads>0)) {
	old= section->doodads;
	nDoodads= section->num_doodads;
    }
    else {
	old= geom->doodads;
	nDoodads= geom->num_doodads;
    }
    for (i=0,doodad=old;i<nDoodads;i++,doodad++) {
	if (doodad->any.name==name)
	    return doodad;
    }
    if (section) {
	if ((section->num_doodads>=geom->sz_doodads)&&
	    (_XkbAllocDoodads(section,1)!=Success)) {
	    return NULL;
	}
	doodad= &section->doodads[section->num_doodads++];
    }
    else {
	if ((geom->num_doodads>=geom->sz_doodads)&&
					(_XkbAllocDoodads(geom,1)!=Success))
	    return NULL;
	doodad= &geom->doodads[geom->num_doodads++];
    }
    bzero(doodad,sizeof(XkbDoodadRec));
    doodad->any.name= name;
    return doodad;
}

XkbOverlayRowPtr
XkbAddGeomOverlayRow(geom,overlay,sz_keys)
    XkbGeometryPtr	geom;
    XkbOverlayPtr	overlay;
    int			sz_keys;
{
XkbOverlayRowPtr	row;

    if ((!geom)||(!overlay)||(sz_keys<0))
	return NULL;
    if ((overlay->num_rows>=overlay->sz_rows)&&
				(_XkbAllocOverlayRows(overlay,1)!=Success))
	return NULL;
    row= &overlay->rows[overlay->num_rows];
    bzero(row,sizeof(XkbOverlayRowRec));
    if ((sz_keys>0)&&(_XkbAllocOverlayKeys(row,sz_keys)!=Success))
	return NULL;
    overlay->num_rows++;
    return row;
}

XkbOverlayPtr
XkbAddGeomOverlay(geom,section,name,sz_rows)
    XkbGeometryPtr	geom;
    XkbSectionPtr	section;
    Atom		name;
    int			sz_rows;
{
register int	i;
XkbOverlayPtr	overlay;

    if ((!geom)||(!section)||(name==None)||(sz_rows==0))
	return NULL;

    for (i=0,overlay=section->overlays;i<section->num_overlays;i++,overlay++) {
	if (overlay->name==name) {
	    if ((sz_rows>0)&&(_XkbAllocOverlayRows(overlay,sz_rows)!=Success))
		return NULL;
	    return overlay;
	}
    }
    if ((section->num_overlays>=section->sz_overlays)&&
				(_XkbAllocOverlays(section,1)!=Success))
	return NULL;
    overlay= &section->overlays[section->num_overlays];
    if ((sz_rows>0)&&(_XkbAllocOverlayRows(overlay,sz_rows)!=Success))
	return NULL;
    overlay->name= name;
    section->num_overlays++;
    return overlay;
}

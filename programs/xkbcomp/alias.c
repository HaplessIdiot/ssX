/* $XConsortium: alias.c /main/2 1995/12/07 21:24:47 kaleb $ */
/************************************************************
 Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

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

#include "xkbcomp.h"
#include "misc.h"

#include "alias.h"

static void
HandleCollision(old,new)
    AliasInfo *		old;
    AliasInfo *		new;
{
    if (strncmp(new->real,old->real,XkbKeyNameLength)==0) {
	if (((new->def.fileID==old->def.fileID)&&(warningLevel>0))||
							(warningLevel>9)) {
	    uWarning("Alias of %s for %s declared more than once\n",
					XkbKeyNameText(new->alias,XkbMessage),
					XkbKeyNameText(new->real,XkbMessage));
	    uAction("First definition ignored\n");
	}
    }
    else {
	char *use,*ignore;
	if (new->def.merge==MergeAugment) {
	    use= old->real;
	    ignore= new->real;
	}
	else {
	    use= old->real;
	    ignore= new->real;
	}
	if (((old->def.fileID==new->def.fileID)&&(warningLevel>0))||
							(warningLevel>9)){
	    uWarning("Multiple definitions for alias %s\n",
					XkbKeyNameText(old->alias,XkbMessage));
	    uWarning("Using %s, ignoring %s\n",
					XkbKeyNameText(use,XkbMessage),
					XkbKeyNameText(ignore,XkbMessage));
	}
	if (use!=old->real)
	    memcpy(old->real,use,XkbKeyNameLength);
    }
    old->def.fileID= new->def.fileID;
    old->def.merge= new->def.merge;
    return;
}

static void
#if NeedFunctionPrototypes
InitAliasInfo(	AliasInfo *	info,
		unsigned	merge,
		unsigned	file_id,
		char *		alias,
		char *		real)
#else
InitAliasInfo(info,merge,file_id,alias,real)
    AliasInfo *		info;
    unsigned		merge;
    unsigned		file_id;
    char *		alias;
    char *		real;
#endif
{
    bzero(info,sizeof(AliasInfo));
    info->def.merge= merge;
    info->def.fileID= file_id;
    strncpy(info->alias,alias,XkbKeyNameLength);
    strncpy(info->real,real,XkbKeyNameLength);
    return;
}

int 
HandleAliasDef(def,merge,file_id,info_in)
    KeyAliasDef *	def;
    unsigned		merge;
    unsigned		file_id;
    AliasInfo **	info_in;
{
AliasInfo *	info;

    for (info= *info_in;info!=NULL;info= (AliasInfo *)info->def.next) {
	if (strncmp(info->alias,def->alias,XkbKeyNameLength)==0) {
	    AliasInfo new;
	    InitAliasInfo(&new,merge,file_id,def->alias,def->real);
	    HandleCollision(info,&new);
	    return True;
	}
    }
    info= uTypedCalloc(1,AliasInfo);
    if (info==NULL) {
	uInternalError("Allocation failure in HandleAliasDef\n");
	return False;
    }
    info->def.fileID= file_id;
    info->def.merge= merge;
    info->def.next= (CommonInfo *)*info_in;
    memcpy(info->alias,def->alias,XkbKeyNameLength);
    memcpy(info->real,def->real,XkbKeyNameLength);
    *info_in= (AliasInfo *)AddCommonInfo(&(*info_in)->def,&info->def);
    return True;
}

void
ClearAliases(info_in)
    AliasInfo **	info_in;
{
    if ((info_in)&&(*info_in))
	ClearCommonInfo(&(*info_in)->def);
    return;
}

Bool
MergeAliases(into,merge)
    AliasInfo **	into;
    AliasInfo **	merge;
{
AliasInfo *	tmp;
KeyAliasDef 	def;

    if ((*merge)==NULL)
	return True;
    if ((*into)==NULL) {
	*into= *merge;
	*merge= NULL;
	return True;
    }	
    bzero((char *)&def,sizeof(KeyAliasDef));
    for (tmp= *merge;tmp!=NULL;tmp= (AliasInfo *)tmp->def.next) {
	def.merge= tmp->def.merge;
	memcpy(def.alias,tmp->alias,XkbKeyNameLength);
	memcpy(def.real,tmp->real,XkbKeyNameLength);
	if (!HandleAliasDef(&def,def.merge,tmp->def.fileID,into))
	    return False;
    }
    return True;
}

int
ApplyAliases(xkb,info_in)
    XkbDescPtr		xkb;
    AliasInfo **	info_in;
{
register int 	i;
XkbKeyAliasPtr	a;
XkbNamesPtr	names;
AliasInfo *	info;
int		nNew,nOld;

    if (*info_in==NULL)
	return True;
    names= xkb->names;
    nOld= ((names!=NULL)?names->num_key_aliases:0);
    for (nNew=0,info= *info_in;info!=NULL;info= (AliasInfo *)info->def.next) {
	nNew++;
	if ( names && names->key_aliases ) {
	    for (i=0,a=names->key_aliases;i<names->num_key_aliases;i++,a++) {
		if (strncmp(a->alias,info->alias,XkbKeyNameLength)==0) {
		    AliasInfo old;
		    InitAliasInfo(&old,MergeAugment,0,a->alias,a->real);
		    HandleCollision(&old,info);
		    memcpy(old.real,a->real,XkbKeyNameLength);
		    info->alias[0]= '\0';
		    nNew--;
		}
	    }
	}
    }
    if (nNew==0) {
	ClearCommonInfo(&(*info_in)->def);
	return True;
    }
    if (XkbAllocNames(xkb,XkbKeyAliasesMask,0,nOld+nNew)!=Success) {
	uInternalError("Allocation failure in ApplyAliases\n");
	return False;
    }
    names= xkb->names;
    a= &names->key_aliases[nOld];
    for (info= *info_in;info!=NULL;info= (AliasInfo *)info->def.next) {
	if (info->alias[0]!='\0') {
	    strncpy(a->alias,info->alias,XkbKeyNameLength);
	    strncpy(a->real,info->real,XkbKeyNameLength);
	    a++;
	}
    }
#ifdef DEBUG
    if ((a-names->key_aliases)!=(nOld+nNew)) {
	uInternalError("Expected %d aliases total but created %d\n",nOld+nNew,
							a-names->key_aliases);
    }
#endif
    ClearCommonInfo(&(*info_in)->def);
    *info_in= NULL;
    return True;
}

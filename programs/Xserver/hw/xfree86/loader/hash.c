/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/hash.c,v 1.3 1997/02/25 14:21:11 hohndel Exp $ */




/*
 *
 * Copyright 1995-1998 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#if defined(Lynx)
#define MAXINT	32000
#else
#include <values.h>
#endif

#include "os.h"
#include "Xos.h"
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
extern void free();
#endif
#include "sym.h"
#include "loader.h"
#include "hash.h"

/* Prototypes for static functions. */
static unsigned int hashFunc(const char *);
static itemPtr LoaderHashFindNearest(
#if NeedFunctionPrototypes
int
#endif
);

static itemPtr LoaderhashTable[ HASHSIZE ] ;

#ifdef DEBUG
static int hashhits[ HASHSIZE ] ;

void
DumpHashHits(void)
{
    int	i;
    int	depth=0;
    int	dev=0;

    for(i=0;i<HASHSIZE;i++) {
	ErrorF("hashhits[%d]=%d\n", i, hashhits[i] );
	depth += hashhits[i];
    }

    depth /= HASHSIZE;
    ErrorF("Average hash depth=%d\n", depth );

    for(i=0;i<HASHSIZE;i++) {
	if( hashhits[i] < depth )
	    dev += depth-hashhits[i];
	else
	    dev += hashhits[i]-depth;
    }

    dev /=HASHSIZE;
    ErrorF("Average hash deviation=%d\n", dev );
}
#endif


static unsigned int
hashFunc(string)
const char *string;
{
    int i=0; 

    while ( i < 10 && string[i] )
      i ++ ;

    if ( i < 5 ) {
#ifdef DEBUG
	hashhits[i]++;
#endif
	return i ;
    }

/*
 * Original has function
#define HASH ((string[ i-4 ] * string[i-3] + string[i-2] ) & (HASHSIZE-1))
 */

#define HASH ((string[i-5] * string[ i-4 ] + string[i-3] * string[i-2] ) & (HASHSIZE-1))

#ifdef DEBUG
    hashhits[HASH]++;
#endif

    return HASH;
}

void
LoaderHashAdd( entry )
    itemPtr entry ;
{
  int bucket = hashFunc( entry->name ) ;
  itemPtr oentry;

  if ((oentry = LoaderHashFind(entry->name)) != NULL)
	LoaderDuplicateSymbol(entry->name, oentry->handle);

  entry->next = LoaderhashTable[ bucket ] ;
  LoaderhashTable[ bucket ] = entry ;
  return;
}

void
LoaderAddSymbols(handle, module, list)
int	handle;
int	module;
LOOKUP	*list ;
{
    LOOKUP	*l = list;
    itemPtr	i;
    char		*p;
    char		*modname;
    char		*newmodname;

    if (!list)
	return;
    /* Visit every symbol in the lookup table,
     * and add it to the given namespace.
     */
    while ( l->symName ) {
	i = (itemPtr) xalloc( sizeof( itemRec )) ;
	i->name = l->symName ;
	if( strcmp(i->name,"ModuleInit") == 0
#if defined(__powerpc__) && defined(Lynx)
	  || strcmp(i->name,".ModuleInit") == 0
#endif
	  )
	    {
		char *origname=i->name;
		/*
		 * special handling for symbol name "ModuleInit"
		 */
		modname = _LoaderHandleToName(handle);
		if( modname )
		    {
			newmodname = strdup(modname);
			p = strrchr(newmodname,'.');
			if( p )
			    *p = '\0';
			p = strrchr(newmodname,'/');
			if( p )
			    p++;
			else
			    p = newmodname;

			i->name = (char*)xalloc(strlen(p)+11);
			if( i->name )
			    {
				strcpy(i->name,p);
				strcat(i->name,origname);
			    }
			free(newmodname);
		    }
#ifdef DEBUG
		ErrorF("Add module init function %s \n",i->name);
#endif
	    }
	i->address = (char *) l->offset ;
	i->handle = handle ;
	i->module = module ;
	LoaderHashAdd( i );
	l ++ ;
    }
}

itemPtr
LoaderHashDelete(string)
const char *string;
{
  int bucket = hashFunc( string ) ;
  itemPtr entry;
  itemPtr *entry2;

  entry = LoaderhashTable[ bucket ] ;
  entry2= &(LoaderhashTable[ bucket ]);
  while ( entry ) {
    if (! strcmp( entry->name, string )) {
      *entry2=entry->next;
      xfree(entry->name);
      xfree( entry ) ;
      return 0 ;
    }
    entry2 = &(entry->next) ;
    entry = entry->next ;
  }
  return 0 ;
}

itemPtr
LoaderHashFind(string)
const char *string;
{
    int bucket = hashFunc( string ) ;
    itemPtr entry ;
	entry = LoaderhashTable[ bucket ] ;
	while ( entry ) {
	    if (!strcmp(entry->name, string))
		return entry;
	    entry = entry->next;
	}
    return 0;
}

static itemPtr
LoaderHashFindNearest(address)
int address;
{
  int i ;
  itemPtr entry, best_entry = 0 ;
  int best_difference = MAXINT;

  for ( i = 0 ; i < HASHSIZE ; i ++ ) {
    entry = LoaderhashTable[ i ] ;
    while ( entry ) {
      int difference = (int) address - (int) entry->address ;
      if ( difference >= 0 ) {
	if ( best_entry ) {
	  if ( difference < best_difference ) {
	    best_entry = entry ;
	    best_difference = difference ;
	  }
	}
	else {
	  best_entry = entry ;
	  best_difference = difference ;
	}
      }
      entry = entry->next ;
    }
  }
  return best_entry ;
}

void
LoaderPrintSymbol(address)
unsigned long address;
{
    itemPtr	entry;
    entry=LoaderHashFindNearest(address);
    if (entry) {
	ErrorF("0x%x %s+%x\n", entry->address, entry->name,
		   address - (unsigned long) entry->address);
    } else {
	ErrorF("(null)\n");
    }
}

void
LoaderPrintItem(itemPtr pItem)
{
    if (pItem)
	ErrorF("0x%x %s\n", pItem->address, pItem->name);
    else
	ErrorF("(null)\n");
}
	
void
LoaderPrintAddress(symbol)
const char *symbol;
{
    itemPtr	entry;
    entry = LoaderHashFind(symbol);
    LoaderPrintItem(entry);
}

void
LoaderHashTraverse(card, fnp)
    void *card;
    int (*fnp)(void *, itemPtr);
{
  int i ;
  itemPtr entry, last_entry = 0 ;

  for ( i = 0 ; i < HASHSIZE ; i ++ ) {
    last_entry = 0 ;
    entry = LoaderhashTable[ i ] ;
    while ( entry ) {
      if (( * fnp )( card, entry )) {
	if ( last_entry ) {
	  last_entry->next = entry->next ;
	  xfree( entry->name ) ;
	  xfree( entry ) ;
	  entry = last_entry->next ;
	}
	else {
	  LoaderhashTable[ i ] = entry->next ;
	  xfree( entry->name ) ;
	  xfree( entry ) ;
	  entry = LoaderhashTable[ i ] ;
	}
      }
      else {
	last_entry = entry ;
	entry = entry->next ;
      }
    }
  }
}

void
LoaderDumpSymbols()
{
	itemPtr       entry;
	int           i,j;
		
	for (j=0; j<HASHSIZE; j++) {
		entry = LoaderhashTable[j];
		while (entry) {
			LoaderPrintItem(entry);
			entry = entry->next;
		}
	}
		
}

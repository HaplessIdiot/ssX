/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/hash.c,v $ */




/*
 *
 * Copyright 1995,96 by Metro Link, Inc.
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

#include <sys/types.h>

#include "sym.h"
#include "loader.h"

#define HASHDIV 8
#define HASHSIZE (1<<HASHDIV)

static itemPtr LoaderhashTable[ HASHSIZE ] ;

#ifdef DEBUG
static int hashhits[ HASHSIZE ] ;

DumpHashHits()
{
int	i;
int	depth=0;
int	dev=0;

for(i=0;i<HASHSIZE;i++) {
	ErrorF("hashists[%d]=%d\n", i, hashhits[i] );
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
  hashFunc( string )
char * string ;
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

LoaderHashAdd( entry )
     itemPtr entry ;
{
  int bucket = hashFunc( entry->name ) ;
  itemPtr oentry;

  if( oentry=LoaderHashFind( entry->name ) )
	LoaderDuplicateSymbol(entry->name, oentry->handle);

  entry->next = LoaderhashTable[ bucket ] ;
  LoaderhashTable[ bucket ] = entry ;
  return 0 ;
}

void
LoaderAddSymbols( handle, list )
int	handle;
LOOKUP	*list ;
{
  LOOKUP * l = list ;
  itemPtr i ;
  while ( l->symName ) {
    i = (itemPtr) malloc( sizeof( itemRec )) ;
    i->name = l->symName ;
    i->address = (char *) l->offset ;
#ifdef HANDLE_IN_HASH_ENTRY
    i->handle = handle ;
#endif
    LoaderHashAdd( i ) ;
    l ++ ;
  }
}

itemPtr
  LoaderHashDelete( string )
char * string ;
{
  int bucket = hashFunc( string ) ;
  itemPtr entry;
  itemPtr *entry2;

  entry = LoaderhashTable[ bucket ] ;
  entry2= &(LoaderhashTable[ bucket ]);
  while ( entry ) {
    if (! strcmp( entry->name, string )) {
      *entry2=entry->next;
      free( entry->name ) ; /* strdup */
      free( entry ) ;
      return 0 ;
    }
    entry2 = &(entry->next) ;
    entry = entry->next ;
  }
  return 0 ;
}

itemPtr
  LoaderHashFind( string )
char * string ;
{
  int bucket = hashFunc( string ) ;
  itemPtr entry ;

  entry = LoaderhashTable[ bucket ] ;
  while ( entry ) {
    if (! strcmp( entry->name, string ))
      return entry ;
    entry = entry->next ;
  }
  return 0 ;
}

itemPtr
  LoaderHashFindNearset( address )
int address ;
{
  int i ;
  itemPtr entry, best_entry = 0 ;
  int best_difference ;

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

LoaderPrintSymbol( address )
int address ;
{
itemPtr	entry;

entry=LoaderHashFindNearset(address);
ErrorF("0x%x %s+%x\n", entry->address, entry->name,(address-(int)entry->address) );
}

LoaderPrintAddress( symbol )
char *symbol ;
{
itemPtr	entry;

entry=LoaderHashFind(symbol);
ErrorF("0x%x %s\n", entry->address, entry->name );
}

void
  LoaderHashTraverse( card, fnp )
void *card ;
int (* fnp ) () ;
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
	  free( entry->name ) ;
	  free( entry ) ;
	  entry = last_entry->next ;
	}
	else {
	  LoaderhashTable[ i ] = entry->next ;
	  free( entry->name ) ;
	  free( entry ) ;
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

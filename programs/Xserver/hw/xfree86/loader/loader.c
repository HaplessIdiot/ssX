/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loader.c,v 1.11 1997/03/07 07:44:18 hohndel Exp $ */




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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#if UseMMAP
#include <sys/mman.h>
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "ar.h"
#include "elf.h"
#include "coff.h"

#include "os.h"
#include "sym.h"
#include "loader.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
#define DEBUG
#define DEBUGAR
#define DEBUGLIST
#define DEBUGMEM
*/

int check_unresolved_sema = 0;

#if defined(Lynx) && defined(sun)
/* Cross build machine doesn;t have strerror() */
#define strerror(err) "strerror unsupported"
#endif

#ifdef __EMX__
void * os2loader_calloc(size_t,size_t);
#endif

#ifdef HANDLE_IN_HASH_ENTRY
#define MAX_HANDLE 256
#define HANDLE_FREE 0
#define HANDLE_USED 1
static char freeHandles[MAX_HANDLE] ;
#endif

void
dummy()
{
return;
}

int 
dummy0(dum)
int dum;
{
return 0;
}

/*
 * Array containing entry points for different formats.
 */

static loader_funcs funcs[] = {
	/* LD_ARCHIVE */
	{ARCHIVELoadModule,dummy,(int(*)())dummy0,dummy},
	/* LD_ELFOBJECT */
	{ELFLoadModule,ELFResolveSymbols,ELFCheckForUnresolved,ELFUnloadModule},
	/* LD_COFFOBJECT */
	{COFF2LoadModule,COFF2ResolveSymbols,COFF2CheckForUnresolved,COFF2UnloadModule},
	/* LD_XCOFFOBJECT */
	{COFF2LoadModule,COFF2ResolveSymbols,COFF2CheckForUnresolved,COFF2UnloadModule},
	/* LD_AOUTOBJECT */
	{AOUTLoadModule,AOUTResolveSymbols,AOUTCheckForUnresolved,AOUTUnloadModule},
#ifdef DLOPEN_SUPPORT
	/* LD_DLOPEN */
	{DLLoadModule,DLResolveSymbols,DLCheckForUnresolved,DLUnloadModule},
#endif
	};

int	numloaders=sizeof(funcs)/sizeof(loader_funcs);


/*
 * Determine what type of object is being loaded.
 * This function is responsible for restoring the offset.
 * The fd and offset are used here so that when Archive processing
 * is enabled, individual elements of an archive can be evaluated
 * so the correct loader_funcs can be determined.
 */
static int
_GetModuleType(fd,offset)
int	fd;
long	offset;
{
unsigned char	buf[10]; /* long enough for the largest magic type */

if( read(fd,buf,sizeof(buf)) < 0 ) {
	return -1;
	}

lseek(fd,offset,SEEK_SET);

if( strncmp(buf,ARMAG,SARMAG) == 0 ) {
	return LD_ARCHIVE;
	}

if( strncmp(buf,ELFMAG,SELFMAG) == 0 ) {
	return LD_ELFOBJECT;
	}

if( buf[0] == 0x4c && buf[1] == 0x01 ) {
	/* I386MAGIC */
	return LD_COFFOBJECT;
	}
if( buf[0] == 0x01 && buf[1] == 0xdf ) {
	/* XCOFFMAGIC */
	return LD_COFFOBJECT;
	}
if( buf[0] == 0x0d && buf[1] == 0x01 ) {
	/* ZCOFFMAGIC (LynxOS) */
	return LD_COFFOBJECT;
	}
if( buf[0] == 0x00 && buf[1] == 0x86 && buf[2] == 0x01 && buf[3] == 0x07) {
        /* AOUTMAGIC */
        return LD_AOUTOBJECT;
        }
if( buf[0] == 0x07 && buf[1] == 0x01 && (buf[2] == 0x64 || buf[2] == 0x86)) {
        /* AOUTMAGIC, (Linux OMAGIC, old impure format, also used by OS/2 */
        return LD_AOUTOBJECT;
        }
return LD_UNKNOWN;
}


static int	offsetbias=0; /* offset into archive */
/*
 * _LoaderFileToMem() loads the contents of a file into memory using
 * the most efficient method for a platform.
 */
void *
_LoaderFileToMem(fd,offset,size,label)
int	fd;
unsigned long	offset;
int	size;
char	*label; /* Only used for Debugging */
{
#if UseMMAP
long ret;	
#define MMAP_PROT	(PROT_READ|PROT_WRITE|PROT_EXEC)

#ifdef DEBUGMEM
ErrorF("_LoaderFileToMem(%d,%u(%u),%d,%s)",fd,offset,offsetbias,size,label);
#endif

ret=(long)mmap(0,size,MMAP_PROT,MAP_PRIVATE,fd,offset+offsetbias);

if(ret == -1)
	FatalError("mmap() failed: %s\n", strerror(errno) );

return (void *)ret;
#else
char *ptr;

#ifdef DEBUGMEM
ErrorF("_LoaderFileToMem(%d,%u(%u),%d,%s)",fd,offset,offsetbias,size,label);
#endif

if(size == 0){
#ifdef DEBUGMEM
ErrorF("=NULL\n",ptr);
#endif
	return NULL;
	}

#ifndef __EMX__
if( (ptr=(char *)calloc(size,1)) == NULL )
	FatalError("_LoaderFileToMem() malloc failed\n" );
#else
if( (ptr=(char *)os2loader_calloc(size,1)) == NULL )
	FatalError("_LoaderFileToMem() malloc failed\n" );
#endif

if(lseek(fd,offset+offsetbias,SEEK_SET)<0)
	FatalError("\n_LoaderFileToMem() lseek() failed: %s\n",strerror(errno));

if(read(fd,ptr,size)!=size)
	FatalError("\n_LoaderFileToMem() read() failed: %s\n",strerror(errno));

#ifdef DEBUGMEM
ErrorF("=%x\n",ptr);
#endif

return (void *)ptr;
#endif
}

/*
 * _LoaderFreeFileMem() free the memory in which a file was loaded.
 */
void
_LoaderFreeFileMem(addr,size)
void	*addr;
int	size;
{
#if UseMMAP
munmap(addr,size);
#else
if(size == 0)
	return;

free(addr);
#endif

return;
}

int
_LoaderFileRead(fd,offset,buf,size)
int	fd;
unsigned int offset;
void	*buf;
int	size;
{
if(lseek(fd,offset+offsetbias,SEEK_SET)<0)
	FatalError("_LoaderFileRead() lseek() failed: %s\n", strerror(errno) );

if(read(fd,buf,size)!=size)
	FatalError("_LoaderFileRead() read() failed: %s\n", strerror(errno) );

return size;
}

static loaderPtr listHead = (loaderPtr) 0 ;

static loaderPtr
_LoaderListPush()
{
  loaderPtr item = (loaderPtr) Xcalloc( sizeof(struct _loader)) ;
  item->next = listHead ;
  listHead = item;

  return item;
}

static loaderPtr
_LoaderListPop(handle)
int	handle;
{
  loaderPtr item=listHead;
  loaderPtr *bptr=&listHead; /* pointer to previous node */

  while(item) {
	if( item->handle == handle ) {
	    *bptr=item->next;	/* remove this from the list */
	    return item;
	    }
	bptr=&(item->next);
	item=item->next;
	}

  return 0;
}

/*
 * _LoaderHandleToName() will return the name of the first module with a
 * given handle. This requires getting the last module on the LIFO with
 * the given handle.
 */
char *
_LoaderHandleToName(handle)
int	handle;
{
  loaderPtr item=listHead;
  loaderPtr aritem=NULL;
  loaderPtr lastitem=NULL;

  if ( handle < 0 ) {
	return "(built-in)";
	}
  while(item) {
	if( item->handle == handle ) {
	    if( strchr(item->name,':') == NULL )
		aritem=item;
	    else
		lastitem=item;
	    }
	item=item->next;
	}

  if( aritem )
    return aritem->name;

  if( lastitem )
    return lastitem->name;

  return 0;
}

/* 
 * _LoaderHandleUnresolved() decides what to do with an unresolved
 * symbol. Right now, it will ignore cfb* symbols whose color depth
 * does not match that of the current server. This has to be checked for
 * the vga16, and xaa servers. Other unresolved will 
 * always be printed out. 
 */

int
_LoaderHandleUnresolved(symbol, module, color_depth)
char *symbol;
char *module;
int color_depth;
{
int fatalsym = 0;

     switch (color_depth){
          case 4:  /* Don't know how to handle yet */
	       break;
          case 8:
	       if (!strncmp(symbol,"cfb16", 5)) break;
	       if (!strncmp(symbol,"cfb24", 5)) break;
	       if (!strncmp(symbol,"cfb32", 5)) break;
	       ErrorF("Symbol %s from module %s is unresolved!\n",
	           symbol, module);
	       fatalsym = 1;
	       break;
	  case 15:
          case 16:
	       if (!strncmp(symbol,"cfb24", 5)) break;
	       if (!strncmp(symbol,"cfb32", 5)) break;
	       ErrorF("Symbol %s from module %s is unresolved!\n",
	           symbol, module);
	       fatalsym = 1;
	       break;
          case 24:
	       if (!strncmp(symbol,"cfb16", 5)) break;
	       if (!strncmp(symbol,"cfb32", 5)) break;
	       ErrorF("Symbol %s from module %s is unresolved!\n",
	           symbol, module);
	       fatalsym = 1;
	       break;
	  case 32:
	       if (!strncmp(symbol,"cfb16", 5)) break;
	       if (!strncmp(symbol,"cfb24", 5)) break;
	       ErrorF("Symbol %s from module %s is unresolved!\n",
	           symbol, module);
	       fatalsym = 1;
	       break;
	  }
     if (xf86ShowUnresolved && !fatalsym){
          ErrorF("Symbol %s from module %s is unresolved!\n",
	       symbol, module);
          }
return(fatalsym);
}

/*
 * Handle an archive.
 */
void *
ARCHIVELoadModule(modtype,modname,handle,arfd)
int	modtype; /* Always LD_ARCHIVE */
char	*modname;
int	handle;
int	arfd;
{
loaderPtr tmp ;
unsigned char	magic[SARMAG];
struct ar_hdr	hdr;
unsigned int	size;
unsigned int	offset = 0;
int	arnamesize, modnamesize;
char	*slash;

if( modtype != LD_ARCHIVE ) {
	ErrorF( "ARCHIVELoadModule(): modtype != ARCHIVE\n" );
	return NULL;
	}

arnamesize=strlen(modname);

read(arfd,magic,SARMAG);

if(strncmp((const char *)magic,ARMAG,SARMAG) != 0 ) {
	ErrorF("ARCHIVELoadModule: wrong magic!!\n" );
	return NULL;
	}

/* Skip the symbol table */
read(arfd,&hdr,sizeof(struct ar_hdr));

if( hdr.ar_name[0] == '/' || strncmp(hdr.ar_name, "__.SYMDEF", 9) == 0) {
	/* If the file name is NULL, then it is a symbol table */
	sscanf(hdr.ar_size,"%d",&size);
#ifdef DEBUGAR
	ErrorF("Member '%16.16s', size %d, offset %d\n",
			hdr.ar_name, size, offset );
	ErrorF("Symbol table size %d\n", size );
#endif
	offset=lseek(arfd,size,SEEK_CUR);
	if( offset&0x1 ) /* odd value */
		offset=lseek(arfd,1,SEEK_CUR); /* make it an even boundary */
	}
else	{
	/* No symbol table - reset the file position to the first file */
	offset=lseek(arfd,SARMAG,SEEK_SET);
	}

/* Skip the string table */
read(arfd,&hdr,sizeof(struct ar_hdr));

if( hdr.ar_name[0] == '/' && hdr.ar_name[1] == '/') {
	/* If the file name is '/', then it is a string table */
	sscanf(hdr.ar_size,"%d",&size);
#ifdef DEBUGAR
	ErrorF("Member '%16.16s', size %d, offset %d\n",
			hdr.ar_name, size, offset );
	ErrorF("String table size %d\n", size );
#endif
	offset=lseek(arfd,size,SEEK_CUR);
	if( offset&0x3 ) /* needs to be long word aligned */
		offset=lseek(arfd,4-(offset&0x3),SEEK_CUR);
	}
else	{
	/* No string table - reset the file position to the first file */
	offset=lseek(arfd,offset,SEEK_SET);
	}

while( read(arfd,&hdr,sizeof(struct ar_hdr)) ) {

	sscanf(hdr.ar_size,"%d",&size);
	offset=lseek(arfd,0,SEEK_CUR);

#ifdef DEBUGAR
	ErrorF("Member '%16.16s', size %d, offset %d\n",
			hdr.ar_name, size, offset );
#endif

	slash=strchr(hdr.ar_name,'/');
	if (slash == NULL) {
	    /* BSD format without trailing slash */
	    slash = strchr(hdr.ar_name,' ');
	} 
	/* XXX lots to do with long name in the form #1/ */
        /* SM: Make sure we do not overwrite other parts of struct */
        
	if((slash - hdr.ar_name) > sizeof(hdr.ar_name)) 
                slash = hdr.ar_name + sizeof(hdr.ar_name) -1;
	*slash='\000';

	if( (modtype=_GetModuleType(arfd,offset)) < 0 ) {
		ErrorF( "%s is an unrecognized module type\n", hdr.ar_name ) ;
		offsetbias=0;
		return NULL;
		}

	tmp=_LoaderListPush();

	tmp->handle = handle;
	tmp->funcs=funcs[modtype];
	modnamesize=strlen(hdr.ar_name);
	tmp->name=(char *)malloc(arnamesize+modnamesize+2 );
	strcpy(tmp->name,modname);
	strcat(tmp->name,":");
	strcat(tmp->name,hdr.ar_name);

	offsetbias=offset;

	if( (tmp->private=funcs[modtype].LoadModule(modtype,tmp->name,handle,
						arfd)) == NULL ) {
		ErrorF( "Failed to load %s\n", hdr.ar_name ) ;
		offsetbias=0;
		return NULL;
	}

	offset=lseek(arfd,offset+size,SEEK_SET);
	if( offset&0x1 ) /* odd value */
		lseek(arfd,1,SEEK_CUR); /* make it an even boundary */
	}
offsetbias=0;

return tmp->private;
}

/*
 * Public Interface to the loader.
 */

int
LoaderOpen( module)
char * module ;
{
loaderPtr tmp ;
static int been_here = 0 ;
int new_handle, modtype ;
int fd;

#if defined(DEBUG)
ErrorF("LoaderOpen(%s)\n", module );
#endif
  
if ( ! been_here ) {
	extern LOOKUP miLookupTab[] ;
	extern LOOKUP xfree86LookupTab[] ;
	extern LOOKUP dixLookupTab[] ;
	been_here = 1 ;
	LoaderAddSymbols( -1, miLookupTab ) ;
	LoaderAddSymbols( -1, xfree86LookupTab ) ;
	LoaderAddSymbols( -1, dixLookupTab ) ;
	}
  
  /*
   * Find a free handle.
   */
  new_handle = 0 ;
  while ( freeHandles[new_handle] && new_handle < MAX_HANDLE )
    new_handle ++ ;
    
  if ( new_handle == MAX_HANDLE ) {
    ErrorF( "Out of loader space\n" ) ; /* XXX */
    return -1 ;
  }
  else
    freeHandles[new_handle] = HANDLE_USED ;

  /*
   * Check to see if the module is already loaded.
   */
  tmp = listHead ;
  while ( tmp ) {
#ifdef DEBUGLIST
    ErrorF("strcmp(%x(%s),{%x} %x(%s))\n", module,module,&(tmp->name),
			tmp->name,tmp->name );
#endif
    if ( ! strcmp( module, tmp->name ))
      return tmp->handle ;
    tmp = tmp->next ;
  }

  /*
   * OK, it's a new one. Add it.
   */
  ErrorF( "Loading %s\n", module ) ;


  if( (fd=open(module, O_RDONLY)) < 0 ) {
    ErrorF( "Unable to open %s\n", module );
    freeHandles[new_handle] = HANDLE_FREE ;
    return -1 ;
  }

  if( (modtype=_GetModuleType(fd,0)) < 0 ) {
	ErrorF( "%s is an unrecognized module type\n", module ) ;
        freeHandles[new_handle] = HANDLE_FREE ;
	return -1;
	}

  tmp=_LoaderListPush();
  tmp->name = strdup( module ) ;
  tmp->handle = new_handle;
  tmp->funcs=funcs[modtype];

  if( (tmp->private=funcs[modtype].LoadModule(modtype,tmp->name,new_handle,fd)) == NULL ) {
	ErrorF( "Failed to load %s\n", module ) ;
	_LoaderListPop(new_handle);
        freeHandles[new_handle] = HANDLE_FREE ;
	return -1;
	}

  close(fd);

  return new_handle ;
}

void *
  LoaderSymbol( sym )
char * sym ;
{
  int i;
  itemPtr item ;
    
  for(i=0;i<numloaders;i++)
	funcs[i].ResolveSymbols();

  item = (itemPtr) LoaderHashFind( sym ) ;

  if ( item )
    return item->address ;
  else
    return 0 ;

}

int
LoaderResolveSymbols( )
{
  int i;

  for(i=0;i<numloaders;i++)
	funcs[i].ResolveSymbols();

  return 0;
}

int
LoaderCheckUnresolved( color_depth, delay_flag )
int color_depth, delay_flag;
{
  int i,ret=0;

  LoaderResolveSymbols();
    
  if (delay_flag == LD_RESOLV_NOW) {
     if (check_unresolved_sema > 0) 
	check_unresolved_sema--;
     else 
	ErrorF("LoaderCheckUnresolved: not enough MAGIC_DONT_CHECK_UNRESOLVED \n");
  }

  if (!check_unresolved_sema ||  delay_flag == LD_RESOLV_FORCE)
     for(i=0;i<numloaders;i++)
	if( funcs[i].CheckForUnresolved( color_depth ) )
		ret=1;

  return ret;
}

int
LoaderDefaultFunc( )
{
	ErrorF("\n\n\tThis should not happen!\n\tAn unresolved function was called!\n");
	FatalError("\n");
}

int
LoaderUnload( handle)
     int handle ;
{
  loaderRec fakeHead ;
  loaderPtr tmp = & fakeHead ;

  if ( handle < 0 || handle > MAX_HANDLE )
	return -1;
 /*
  * find the loaderRecs associated with this handle.
  */

  while( (tmp=_LoaderListPop(handle)) != NULL )
	{
	if( strchr(tmp->name,':') == NULL ) {
		/* It is not a member of an archive */
		ErrorF( "Unloading %s\n", tmp->name ) ;
		}
	tmp->funcs.LoaderUnload(tmp->private);
	free(tmp->name);
	free(tmp);
	}
  
  freeHandles[handle] = HANDLE_FREE ;

return 0;
}

void
LoaderDuplicateSymbol(symbol,handle)
char	*symbol;
int	handle;
{
ErrorF("Duplicate symbol %s in %s\n", symbol, listHead->name );
ErrorF("Also defined in %s\n", _LoaderHandleToName(handle) );
FatalError("\n");
}


/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loader.c,v 1.18 1998/03/21 11:08:48 dawes Exp $ */




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
#include "loaderProcs.h"
#include "xf86.h"
#include "xf86Priv.h"

extern LOOKUP miLookupTab[];
extern LOOKUP xfree86LookupTab[];
extern LOOKUP dixLookupTab[];

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
/*
 * handles are used to identify files that are loaded. Even archives
 * are counted as a single file.
 */
#define MAX_HANDLE 256
#define HANDLE_FREE 0
#define HANDLE_USED 1
static char freeHandles[MAX_HANDLE] ;
static int refCount[MAX_HANDLE] ;
#endif

/*
 * modules are used to identify compilation units (ie object modules).
 * Archives contain multiple modules, each of which is treated seperately.
 */
static int moduleseq = 0;

/* Prototypes for static functions. */
static int _GetModuleType(int, long);
static loaderPtr _LoaderListPush(void);
static loaderPtr _LoaderListPop(int);
/*ARGSUSED*/
static void ARCHIVEResolveSymbols(void *unused) {}
/*ARGSUSED*/
static int ARCHIVECheckForUnresolved(int foo, void *v) { return 0; }
/*ARGSUSED*/
static void ARCHIVEUnload(void *unused2) {}

/*
 * Array containing entry points for different formats.
 */

static loader_funcs funcs[] = {
	/* LD_ARCHIVE */
	{ARCHIVELoadModule,
	 ARCHIVEResolveSymbols,
	 ARCHIVECheckForUnresolved,
	 ARCHIVEUnload, {0,0,0,0,0}},
	/* LD_ELFOBJECT */
	{ELFLoadModule,
	 ELFResolveSymbols,
	 ELFCheckForUnresolved,
	 ELFUnloadModule, {0,0,0,0,0}},
	/* LD_COFFOBJECT */
	{COFFLoadModule,
	 COFFResolveSymbols,
	 COFFCheckForUnresolved,
	 COFFUnloadModule, {0,0,0,0,0}},
	/* LD_XCOFFOBJECT */
	{COFFLoadModule,
	 COFFResolveSymbols,
	 COFFCheckForUnresolved,
	 COFFUnloadModule, {0,0,0,0,0}},
	/* LD_AOUTOBJECT */
	{AOUTLoadModule,
	 AOUTResolveSymbols,
	 AOUTCheckForUnresolved,
	 AOUTUnloadModule, {0,0,0,0,0}},
#ifdef DLOPEN_SUPPORT
	/* LD_AOUTDLOBJECT */
	{DLLoadModule,
	 DLResolveSymbols,
	 DLCheckForUnresolved,
	 DLUnloadModule, {0,0,0,0,0}},
#endif
	};

int	numloaders=sizeof(funcs)/sizeof(loader_funcs);


void
LoaderInit(void)
{
    LoaderAddSymbols(-1, -1, miLookupTab ) ;
    LoaderAddSymbols(-1, -1, xfree86LookupTab ) ;
    LoaderAddSymbols(-1, -1, dixLookupTab ) ;
}

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

#ifdef DEBUG
    ErrorF("Checking module type %10s\n", buf );
    ErrorF("Checking module type %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3] );
#endif

    lseek(fd,offset,SEEK_SET);

    if (strncmp((char *) buf, ARMAG, SARMAG) == 0) {
	return LD_ARCHIVE;
    }

#if defined(AIAMAG)
    /* LynxOS PPC style archives */
    if (strncmp((char *) buf, AIAMAG, SAIAMAG) == 0) {
	return LD_ARCHIVE;
    }
#endif

    if (strncmp((char *) buf, ELFMAG, SELFMAG) == 0) {
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
    if (buf[0] == 0x07 && buf[1] == 0x01 && (buf[2] == 0x64 || buf[2] == 0x86))
    {
        /* AOUTMAGIC, (Linux OMAGIC, old impure format, also used by OS/2 */
        return LD_AOUTOBJECT;
    }
    if (buf[0] == 0x07 && buf[1] == 0x01 && buf[2] == 0x00 && buf[3] == 0x00)
    {
        /* AOUTMAGIC, BSDI */
        return LD_AOUTOBJECT;
    }
#ifdef DLOPEN_SUPPORT
    if ((buf[0] == 0xc0 && buf[1] == 0x86) ||
	(buf[3] == 0xc0 && buf[2] == 0x86))
    {
        /* i386 shared object */
        return LD_AOUTDLOBJECT;
    }
#endif

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
    unsigned long ret;	
#define MMAP_PROT	(PROT_READ|PROT_WRITE|PROT_EXEC)

#ifdef DEBUGMEM
    ErrorF("_LoaderFileToMem(%d,%u(%u),%d,%s)",fd,offset,offsetbias,size,label);
#endif

    ret = (unsigned long) mmap(0,size,MMAP_PROT,MAP_PRIVATE,
			       fd,offset+offsetbias);

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
    if( (ptr=(char *)xf86loadercalloc(size,1)) == NULL )
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
    ErrorF("=%lx\n",ptr);
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
#ifdef DEBUGMEM
    ErrorF("_LoaderFreeFileMem(%x,%d)\n",addr,size);
#endif
#if UseMMAP
    munmap(addr,size);
#else
    if(size == 0)
	return;

    xf86loaderfree(addr);
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
  loaderPtr item = (loaderPtr) xf86loadercalloc(1, sizeof (struct _loader));
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
ARCHIVELoadModule(modrec, arfd, ppLookup)
loaderPtr	modrec;
int	arfd;
LOOKUP **ppLookup;
{
    loaderPtr tmp = NULL;
    unsigned char	magic[SARMAG];
    struct ar_hdr	hdr;
#if defined(__powerpc__) && defined(Lynx)
    struct fl_hdr	fhdr;
    char		name[255];
    int			namlen;
#endif
    unsigned int	size;
    unsigned int	offset;
    int	arnamesize, modnamesize;
    char	*slash;
    LOOKUP *lookup_ret, *p;
    LOOKUP *myLookup = NULL; /* Does realloc behave if ptr == 0? */
    int modtype;
    int i;
    int numsyms = 0;
    int resetoff;

    /* lookup_ret = (LOOKUP **) xf86loadermalloc(sizeof (LOOKUP *)); */

    arnamesize=strlen(modrec->name);

#if !(defined(__powerpc__) && defined(Lynx))
    read(arfd,magic,SARMAG);

    if(strncmp((const char *)magic,ARMAG,SARMAG) != 0 ) {
	ErrorF("ARCHIVELoadModule: wrong magic!!\n" );
	return NULL;
    }
    resetoff=SARMAG;
#else
    read(arfd,&fhdr,FL_HSZ);

    if(strncmp(fhdr.fl_magic,AIAMAG,SAIAMAG) != 0 ) {
	ErrorF("ARCHIVELoadModule: wrong magic!!\n" );
	return NULL;
    }
    resetoff=FL_HSZ;
#endif /* __powerpc__ && Lynx */

#ifdef DEBUGAR
    ErrorF("Looking for archive members starting at offset %o\n", offset );
#endif

    while( read(arfd,&hdr,sizeof(struct ar_hdr)) ) {

	sscanf(hdr.ar_size,"%d",&size);
#if defined(__powerpc__) && defined(Lynx)
	sscanf(hdr.ar_namlen,"%d",&namlen);
	name[0]=hdr.ar_name[0];
	name[1]=hdr.ar_name[1];
	read(arfd,&name[2],namlen);
	name[namlen]='\0';
	offset=lseek(arfd,0,SEEK_CUR);
	if( offset&0x1 ) /* odd value */
		offset=lseek(arfd,1,SEEK_CUR); /* make it an even boundary */
#endif
	offset=lseek(arfd,0,SEEK_CUR);

	/* Check for a Symbol Table */
	if( (hdr.ar_name[0] == '/' && hdr.ar_name[1] == ' ') ||
#if defined(__powerpc__) && defined(Lynx)
	    namlen == 0 ||
#endif
	    strncmp(hdr.ar_name, "__.SYMDEF", 9) == 0 ) {
	    /* If the file name is NULL, then it is a symbol table */
#ifdef DEBUGAR
	    ErrorF("Symbol Table Member '%16.16s', size %d, offset %d\n",
	           hdr.ar_name, size, offset );
	    ErrorF("Symbol table size %d\n", size );
#endif
	    offset=lseek(arfd,offset+size,SEEK_SET);
	    if( offset&0x1 ) /* odd value */
	        offset=lseek(arfd,1,SEEK_CUR); /* make it an even boundary */
	    continue;
	}

	/* Check for a String Table */
	if( hdr.ar_name[0] == '/' && hdr.ar_name[1] == '/') { 
	    /* If the file name is '/', then it is a string table */
#ifdef DEBUGAR
	    ErrorF("String Table Member '%16.16s', size %d, offset %d\n",
	           hdr.ar_name, size, offset );
	    ErrorF("String table size %d\n", size );
#endif
	    offset=lseek(arfd,offset+size,SEEK_SET);
	    if( offset&0x1 ) /* odd value */
		    offset=lseek(arfd,1,SEEK_CUR); /* make it an even boundary */
	    continue;
	}

	/* Regular archive member */
#ifdef DEBUGAR
	ErrorF("Member '%16.16s', size %d, offset %x\n",
#if !(defined(__powerpc__) && defined(Lynx))
		hdr.ar_name,
#else
		name,
#endif
		size, offset );
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

	tmp->handle = modrec->handle;
	tmp->module = moduleseq++;
	tmp->funcs=&funcs[modtype];
	modnamesize=strlen(hdr.ar_name);
	tmp->name=(char *)xf86loadermalloc(arnamesize+modnamesize+2 );
	strcpy(tmp->name,modrec->name);
	strcat(tmp->name,":");
	strcat(tmp->name,hdr.ar_name);

	offsetbias=offset;

	if((tmp->private = funcs[modtype].LoadModule(tmp, arfd,
						     &lookup_ret))
	   == NULL) {
	    ErrorF( "Failed to load %s\n", hdr.ar_name ) ;
	    offsetbias=0;
	    return NULL;
	}

	offset=lseek(arfd,offset+size,SEEK_SET);
	if( offset&0x1 ) /* odd value */
		lseek(arfd,1,SEEK_CUR); /* make it an even boundary */

	/* Add the lookup table returned from funcs.LoadModule to the
	 * one we're going to return.
	 */
	for (i = 0, p = lookup_ret; p && p->symName; i++, p++)
	    ;
	if (i) {
	    myLookup = (LOOKUP *) xf86loaderrealloc(myLookup, (numsyms + i + 1)
					   * sizeof (LOOKUP));
	    if (!myLookup)
		continue; /* Oh well! */

	    memcpy(&(myLookup[numsyms]), lookup_ret, i * sizeof (LOOKUP));
	    numsyms += i;
	    myLookup[numsyms].symName = 0;
	}
	xf86loaderfree(lookup_ret);
    }
    /* xf86loaderfree(lookup_ret); */
    offsetbias=0;

    *ppLookup = myLookup;

    if (tmp)
	return tmp->private;
    else
	return 0;
}

/*
 * Relocation list manipulation routines
 */

/*
 * _LoaderGetRelocations() Return the list of outstanding relocations
 */
LoaderRelocPtr
_LoaderGetRelocations(mod)
void *mod;
{
loader_funcs	*formatrec = (loader_funcs *)mod;

return  &(formatrec->pRelocs);
}

/*
 * Public Interface to the loader.
 */

int
LoaderOpen(module, handle, errmaj, errmin)
const char *module;
int handle;
int *errmaj; int *errmin;
{
    loaderPtr tmp ;
    int new_handle, modtype ;
    int fd;
    LOOKUP *pLookup;
    int i;

#if defined(DEBUG)
    ErrorF("LoaderOpen(%s)\n", module );
#endif

    /*
     * Check to see if the module is already loaded.
     * Only if we are loading it into an existing namespace.
     * If it is to be loaded into a new namespace, don't check.
     */
    if (handle >= 0) {
	tmp = listHead;
	while ( tmp ) {
#ifdef DEBUGLIST
	    ErrorF("strcmp(%x(%s),{%x} %x(%s))\n", module,module,&(tmp->name),
		   tmp->name,tmp->name );
#endif
	    if ( ! strcmp( module, tmp->name )) {
		return tmp->handle;
	    }
	    tmp = tmp->next ;
	}
    }

    /*
     * OK, it's a new one. Add it.
     */
    ErrorF( "Loading %s\n", module ) ;

    /*
     * Find a free handle.
     */
    new_handle = 1;
    while ( freeHandles[new_handle] && new_handle < MAX_HANDLE )
	new_handle ++ ;

    if ( new_handle == MAX_HANDLE ) {
	ErrorF( "Out of loader space\n" ) ; /* XXX */
	if(errmaj) *errmaj = LDR_NOSPACE;
	return -1 ;
    }
    else
	freeHandles[new_handle] = HANDLE_USED ;
	refCount[new_handle] = 1;



    if( (fd=open(module, O_RDONLY)) < 0 ) {
	ErrorF( "Unable to open %s\n", module );
	freeHandles[new_handle] = HANDLE_FREE ;
	if(errmaj) *errmaj = LDR_NOMODOPEN;
	if(errmin) *errmin = errno;
	return -1 ;
    }

    if( (modtype=_GetModuleType(fd,0)) < 0 ) {
	ErrorF( "%s is an unrecognized module type\n", module ) ;
        freeHandles[new_handle] = HANDLE_FREE ;
	if(errmaj) *errmaj = LDR_UNKTYPE;
	return -1;
    }

    tmp=_LoaderListPush();
    tmp->name = (char *) xf86loadermalloc(strlen(module) + 1);
    strcpy(tmp->name, module);
    tmp->handle = new_handle;
    tmp->module = moduleseq++;
    tmp->funcs=&funcs[modtype];

    if((tmp->private = funcs[modtype].LoadModule(tmp,fd, &pLookup)) == NULL) {
	ErrorF( "Failed to load %s\n", module ) ;
	_LoaderListPop(new_handle);
        freeHandles[new_handle] = HANDLE_FREE ;
	if(errmaj) *errmaj = LDR_NOLOAD;
	return -1;
    }

    LoaderAddSymbols(new_handle, tmp->module, pLookup);
    xf86loaderfree(pLookup);

    close(fd);

    return new_handle;
}

void *
LoaderSymbol(sym)
const char *sym;
{
  int i;
  itemPtr item = NULL;
  for (i = 0; i < numloaders; i++)
	funcs[i].ResolveSymbols(&funcs[i]);

      item = (itemPtr) LoaderHashFind(sym);

  if ( item )
    return item->address ;
  else
#ifdef DLOPEN_SUPPORT
    return(DLFindSymbol(sym));
#else
    return NULL;
#endif
}

int
LoaderResolveSymbols(void)
{
    int i;
    for(i=0;i<numloaders;i++)
	funcs[i].ResolveSymbols(&funcs[i]);
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
	   if (funcs[i].CheckForUnresolved(color_depth, &funcs[i]))
		ret=1;

  return ret;
}

void
LoaderDefaultFunc(void)
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
  * check the reference count, only free it if it goes to zero
  */
	if (--refCount[handle])
		return 0;
 /*
  * find the loaderRecs associated with this handle.
  */

  while( (tmp=_LoaderListPop(handle)) != NULL )
	{
	if( strchr(tmp->name,':') == NULL ) {
		/* It is not a member of an archive */
		ErrorF( "Unloading %s\n", tmp->name ) ;
		}
	tmp->funcs->LoaderUnload(tmp->private);
	xf86loaderfree(tmp->name);
	xf86loaderfree(tmp);
	}
  
  freeHandles[handle] = HANDLE_FREE ;

return 0;
}

void
LoaderDuplicateSymbol(const char *symbol, const int handle)
{
    ErrorF("Duplicate symbol %s in %s\n", symbol, listHead->name);
    ErrorF("Also defined in %s\n", _LoaderHandleToName(handle));
    FatalError("\n");
}

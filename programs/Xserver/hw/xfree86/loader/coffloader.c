/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/coffloader.c,v $ */



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
#include <unistd.h>
#include <stdlib.h>
#ifdef QNX
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/stat.h>
#include "coff.h"

#include "sym.h"
#include "loader.h"

/*
#ifndef LDTEST
#define COFF2DEBUG ErrorF
#endif
*/

/*
 * This structure contains all of the information about a module
 * that has been loaded.
 */

typedef	struct {
	int	handle;
	int	fd;
	FILHDR 	*header;	/* file header */
	AOUTHDR  *optheader;	/* optional file header */
	unsigned short	numsh;
	SCNHDR	*sections;	/* Start address of the section table */
	int	secsize;	/* size of the section table */
	unsigned char	**saddr;/* Start addresss of the sections table */
	unsigned char	**reladdr;/* Start addresss of the relocation table */
	unsigned char *strtab;	/* Start address of the string table */
	int	strsize;	/* size of the string table */
	unsigned char *text;	/* Start address of the .text section */
	int	txtndx;		/* index of the .text section */
	int	txtsize;	/* size of the .text section */
	int	txtrelsize;	/* size of the .rel.text section */
	unsigned char *data;	/* Start address of the .data section */
	int	datndx;		/* index of the .data section */
	int	datsize;	/* size of the .data section */
	int	datrelsize;	/* size of the .rel.data section */
	unsigned char *bss;	/* Start address of the .bss section */
	int	bssndx;		/* index of the .bss section */
	int	bsssize;	/* size of the .bss section */
	SYMENT *symtab;		/* Start address of the .symtab section */
	int	symndx;		/* index of the .symtab section */
	int	symsize;	/* size of the .symtab section */
	unsigned char *common;	/* Start address of the .common section */
	int	comsize;	/* size of the .common section */
	}	COFF2ModuleRec, *COFF2ModulePtr;

/*
 * If an relocation is unable to be satisfied, then put it on a list
 * to try later after more odules have been loaded.
 */
typedef struct _coff_reloc {
	COFF2ModulePtr	file;
	RELOC		*rel;
	int		secndx;
	struct _coff_reloc	*next;
	} coff_reloc ;

static	coff_reloc *listResolve = NULL;

/*
 * Symbols with a section number of 0 (N_UNDEF) but a value of non-zero
 * need to have space allocated for them.
 *
 * Gather all of these symbols together, and allocate one chunk when we
 * are done.
 */

typedef struct _coff_COMMON {
	SYMENT	*sym;
	int	index;
	struct _coff_COMMON	*next;
	} coff_COMMON;

static coff_COMMON *listCOMMON = NULL;

/*
 * Utility Functions
 */


#ifdef HANDLE_IN_HASH_ENTRY
static int
  COFF2hashCleanOut( module, item )
COFF2ModulePtr module ;
itemPtr item ;
{
  return ( module->handle == item->handle ) ;
}
#endif

/*
 * Get symbol name
 */
char *
COFF2GetSymbolName(cofffile, index)
COFF2ModulePtr	cofffile;
int		index;
{
char	*name;
SYMENT	*sym;

sym=(SYMENT *)(((unsigned char *)cofffile->symtab)+(index*SYMESZ));

#ifdef COFF2DEBUG
COFF2DEBUG("COFF2GetSymbolName(%x,%x) %x",cofffile, index, sym->n_zeroes );
#endif

if( sym->n_zeroes )
	{
	if( (name=malloc(SYMNMLEN+1)) == NULL ) /* +1 for NULL */
		FatalError("COFF2GetSymbolNameIndex() Can't malloc space\n" );
	strncpy(name,sym->n_name,SYMNMLEN);
	name[SYMNMLEN]='\000';
	}
else
	{
	name=(char *)strdup((const char *)&cofffile->strtab[(int)sym->n_offset-4]);
	}
#ifdef COFF2DEBUG
COFF2DEBUG(" %s\n", name );
#endif
return name;
}

/*
 * Manage listCOMMON
 */

static void
COFF2AddCOMMON(sym,index)
SYMENT	*sym;
{
coff_COMMON	*common;

if( (common=(coff_COMMON*)malloc(sizeof(coff_COMMON))) == NULL ) {
	ErrorF( "COFF2AddCOMMON() Unable to allocate memory!!!!\n" );
	return;
	}
common->sym=sym;
common->index=index;
common->next=listCOMMON;
listCOMMON=common;

return;
}

static void
COFF2CreateCOMMON(cofffile)
COFF2ModulePtr	cofffile;
{
int	numsyms=0,size=0,l=0;
int	offset=0;
coff_COMMON	*common;
LOOKUP		*lookup;

if( listCOMMON == NULL )
	return;

common=listCOMMON;
while(common) {
	/* Ensure long word alignment */
	if( common->sym->n_value%4 != 0 )
		common->sym->n_value+= 4-(common->sym->n_value%4);

	/* accumulate the sizes */
	size+=common->sym->n_value;
	numsyms++;
	common=common->next;
	}

#ifdef COFF2DEBUG
COFF2DEBUG("COFF2CreateCOMMON() %d entries (%d bytes) of COMMON data\n",
				numsyms, size );
#endif

if( (lookup=malloc((numsyms+1)*sizeof(LOOKUP))) == NULL ) {
        ErrorF( "COFF2CreateCOMMON() Unable to allocate memory!!!!\n" );
        return;
        }

cofffile->comsize=size;
if( (cofffile->common=(unsigned char *)calloc(1,size)) == NULL ) {
        ErrorF( "COFF2CreateCOMMON() Unable to allocate memory!!!!\n" );
        return;
        }

while(listCOMMON) {
        common=listCOMMON;
        lookup[l].symName=COFF2GetSymbolName(cofffile,common->index);
        lookup[l].offset=(void (*)())(cofffile->common+offset);
#ifdef COFF2DEBUG
        COFF2DEBUG("Adding %x %s\n", lookup[l].offset, lookup[l].symName );
#endif
        listCOMMON=common->next;
        offset+=common->sym->n_value;
        free(common);
        l++;
        }

lookup[l].symName=NULL; /* Terminate the list */
LoaderAddSymbols(cofffile->handle,lookup);
free(lookup);

return;

}

/*
 * Manage listResolv
 */
static void
COFF2DelayRelocation(cofffile,secndx,rel)
COFF2ModulePtr	cofffile;
int		secndx;
RELOC		*rel;
{
coff_reloc	*reloc;

if( (reloc=(coff_reloc *)malloc(sizeof(coff_reloc))) == NULL ) {
	ErrorF( "COFF2DelayRelocation() Unable to allocate memory!!!!\n" );
	return;
	}

reloc->file=cofffile;
reloc->secndx=secndx;
reloc->rel=rel;
reloc->next=listResolve;
listResolve=reloc;

return;
}


/*
 * Symbol Table
 */

SYMENT *
COFF2GetSymbol(file, index)
COFF2ModulePtr	file;
int index;
{
return (SYMENT *)(((unsigned char *)file->symtab)+(index*SYMESZ));
}

unsigned char *
COFF2GetSymbolValue(cofffile, index)
COFF2ModulePtr	cofffile;
int index;
{
unsigned char *symval=0;	/* value of the indicated symbol */
itemPtr symbol;		/* name/value of symbol */
char	*name;

name=COFF2GetSymbolName(cofffile, index);

#ifdef COFF2DEBUG
COFF2DEBUG("COFF2GetSymbolValue() for %s=", name );
#endif

symbol = LoaderHashFind( name ) ;

if( symbol )
	symval=(unsigned char *)symbol->address;

#ifdef COFF2DEBUG
COFF2DEBUG("%x\n", symval );
#endif

free(name);
return symval;
}

/*
 * Fix all of the relocation for the given section.
 */
static void
COFF2_RelocateEntry(cofffile,secndx,rel)
COFF2ModulePtr	cofffile;
int		secndx;	/* index of the target section */
RELOC		*rel;
{
SYMENT	*symbol;	/* value of the indicated symbol */
unsigned long *destl;	/* address of the place being modified */
unsigned char *symval;	/* value of the indicated symbol */

/*
 * Note: Section numbers are 1 biased, while the cofffile->saddr[] array
 * of pointer is 0 biased, so alway have to account for the difference.
 */

/* 
 * Reminder: secndx is the section to which the relocation is applied.
 *           symbol->n_scnum is the section in which the symbol value resides.
 */

#ifdef COFF2DEBUG
	COFF2DEBUG("%x %d %o ",
		rel->r_vaddr,rel->r_symndx,rel->r_type );
#endif
symbol=COFF2GetSymbol(cofffile,rel->r_symndx);
#ifdef COFF2DEBUG
	COFF2DEBUG("%d %x %d-%d\n", symbol->n_sclass, symbol->n_value, symbol->n_scnum, secndx );
#endif

/*
 * Check to see if the relocation offset is part of the .text segment.
 * If not, we must change the offset to be relative to the .data section
 * which is NOT contiguous.
 */ 
if( (long)rel->r_vaddr < (long)(cofffile->txtsize) ) {
	if( secndx != N_TEXT-1 )
		ErrorF("secndx != N_TEXT-1\n" );
	destl=(unsigned long *)((long)(cofffile->saddr[secndx])+rel->r_vaddr);
} else {
	if( (long)rel->r_vaddr < (long)(cofffile->txtsize+cofffile->datsize) ){
		if( secndx != N_DATA-1 )
			ErrorF("secndx != N_DATA-1\n" );
		destl=(unsigned long *)((long)(cofffile->saddr[N_DATA-1])+
			rel->r_vaddr-cofffile->txtsize);
	} else {
		if( secndx != N_BSS-1 )
			ErrorF("secndx != N_BSS-1\n" );
		destl=(unsigned long *)((long)(cofffile->saddr[N_DATA-1])+
			rel->r_vaddr-(cofffile->txtsize+cofffile->datsize));
		}
	}

if( symbol->n_sclass == 0 )
	{
	symval=(unsigned char *)(symbol->n_value+(*destl)-symbol->n_type);
#ifdef COFF2DEBUG
	COFF2DEBUG( "symbol->n_sclass==0\n" );
	COFF2DEBUG( "destl=%x\t", destl );
	COFF2DEBUG( "symval=%x\t", symval );
	COFF2DEBUG( "*destl=%8.8x\t", *destl );
#endif
	*destl=(unsigned long)symval;
	return;
	}

switch( rel->r_type )
	{
	case R_DIR32:
#ifdef COFF2DEBUG
#endif
		symval=COFF2GetSymbolValue(cofffile,rel->r_symndx);
		if( symval ) {
#ifdef COFF2DEBUG
			char *namestr;
			COFF2DEBUG( "R_DIR32 %s\n",
			namestr=COFF2GetSymbolName(cofffile,rel->r_symndx) );
			free(namestr);
			COFF2DEBUG( "txtsize=%x\t", cofffile->txtsize );
			COFF2DEBUG( "destl=%x\t", destl );
			COFF2DEBUG( "symval=%x\t", symval );
			COFF2DEBUG( "*destl=%8.8x\t", *destl );
#endif
			*destl=(unsigned long)(symval+(*destl)-symbol->n_value);
		} else {
			switch( symbol->n_scnum ) {
			case N_UNDEF:
#ifdef COFF2DEBUG
				COFF2DEBUG( "R_DIR32 N_UNDEF\n" );
#endif
				COFF2DelayRelocation(cofffile,secndx,rel);
				return;
			case N_ABS:
#ifdef COFF2DEBUG
				COFF2DEBUG( "R_DIR32 N_ABS\n" );
#endif
				return;
			case N_DEBUG:
#ifdef COFF2DEBUG
				COFF2DEBUG( "R_DIR32 N_DEBUG\n" );
#endif
				return;
			case N_COMMENT:
#ifdef COFF2DEBUG
				COFF2DEBUG( "R_DIR32 N_COMMENT\n" );
#endif
				return;
			case N_TEXT:
#ifdef COFF2DEBUG
				COFF2DEBUG( "R_DIR32 N_TEXT\n" );
				COFF2DEBUG( "destl=%x\t", destl );
				COFF2DEBUG( "symval=%x\t", symval );
				COFF2DEBUG( "*destl=%8.8x\t", *destl );
#endif
				*destl=(unsigned long)((*destl)+
				 (unsigned long)(cofffile->saddr[N_TEXT-1]));
				break;
			case N_DATA:
#ifdef COFF2DEBUG
				COFF2DEBUG( "R_DIR32 N_DATA\n" );
				COFF2DEBUG( "txtsize=%x\t", cofffile->txtsize );
				COFF2DEBUG( "destl=%x\t", destl );
				COFF2DEBUG( "symval=%x\t", symval );
				COFF2DEBUG( "*destl=%8.8x\t", *destl );
#endif
				*destl=(unsigned long)((*destl)+
				 ((unsigned long)(cofffile->saddr[N_DATA-1]))-
				 cofffile->txtsize);
				break;
			case N_BSS:
#ifdef COFF2DEBUG
				COFF2DEBUG( "R_DIR32 N_BSS\n" );
				COFF2DEBUG( "destl=%x\t", destl );
				COFF2DEBUG( "symval=%x\t", symval );
				COFF2DEBUG( "*destl=%8.8x\t", *destl );
#endif
				*destl=(unsigned long)((*destl)+
				 (unsigned long)(cofffile->saddr[N_BSS-1])-
				 (cofffile->txtsize+cofffile->datsize));
				break;
			default:
				ErrorF("R_DIR32 with unexpected section %d\n",
							symbol->n_scnum );
			}

		}
#ifdef COFF2DEBUG
COFF2DEBUG( "*destl=%8.8x\n", *destl );
#endif
		break;
	case R_PCRLONG:
		if( symbol->n_scnum == N_TEXT )
			break;

		symval=COFF2GetSymbolValue(cofffile,rel->r_symndx);
#ifdef COFF2DEBUG
COFF2DEBUG( "R_PCRLONG ");
COFF2DEBUG( "destl=%x\t", destl );
COFF2DEBUG( "symval=%x\t", symval );
COFF2DEBUG( "*destl=%8.8x\t", *destl );
#endif
		if( symval == 0 ) {
#ifdef COFF2DEBUG
			char *name;
			COFF2DEBUG( "***Unable to resolve symbol %s\n",
			     name=COFF2GetSymbolName(cofffile,rel->r_symndx) );
			free(name);
#endif
			COFF2DelayRelocation(cofffile,secndx,rel);
			break;
			}
		*destl=(unsigned long)(symval-((long)destl+sizeof(long)));

#ifdef COFF2DEBUG
COFF2DEBUG( "*destl=%8.8x\n", *destl );
#endif
		break;
	case R_ABS:
		/*
		 * Nothing to really do here.
		 * Usually, a dummy relocation for .file
		 */
		break;
#if defined(__powerpc__)
	case R_LEN:
		/*
		 */
		break;
#endif
	default:
		ErrorF(
		 "COFF2_RelocateEntry() Unsupported relocation type %o\n",
				rel->r_type );
		break;
	}
}

void
COFF2_DoRelocations(cofffile)
COFF2ModulePtr	cofffile;
{
unsigned short	i,j;
RELOC	*rel;
SCNHDR	*sec;

for(i=0; i<cofffile->numsh; i++ ) {
	if( cofffile->saddr[i] == NULL )
		continue; /* Section not loaded!! */
	sec=&(cofffile->sections[i]);
	for(j=0;j<sec->s_nreloc;j++) {
		rel=(RELOC *)(cofffile->reladdr[i]+(j*RELSZ));
		COFF2DelayRelocation(cofffile,i,rel);
		}
	}

return;
}

/*
 * COFF2_GetSymbols()
 *
 * add the symbols to the symbol table maintained by the loader.
 */

static void
COFF2_GetSymbols(cofffile)
COFF2ModulePtr	cofffile;
{
SYMENT	*sym;
int	i, l, numsyms;
LOOKUP	*lookup;
char	*symname;

/*
 * Load the symbols into memory
 */
numsyms=cofffile->header->f_nsyms;

#ifdef COFF2DEBUG
COFF2DEBUG("COFF2_GetSymbols(): %d symbols\n", numsyms );
#endif

cofffile->symsize=(numsyms*SYMESZ);
cofffile->symtab=(SYMENT *)_LoaderFileToMem(cofffile->fd,cofffile->header->f_symptr,
				(numsyms*SYMESZ),"symbols");

if( (lookup=malloc((numsyms+1)*sizeof(LOOKUP))) == NULL )
	return;

for(i=0,l=0; i<numsyms; i++)
	{
	sym=(SYMENT *)(((unsigned char *)cofffile->symtab)+(i*SYMESZ));
	symname=COFF2GetSymbolName(cofffile,i);
#ifdef COFF2DEBUG
	COFF2DEBUG("\t%d %d %x %x %d %d %s\n",
		i, sym->n_scnum, sym->n_value, sym->n_type,
		sym->n_sclass, sym->n_numaux, symname );
#endif
	i+=sym->n_numaux;
	switch( sym->n_scnum )
		{
		case N_UNDEF:
			if( sym->n_value != 0 ) {
#ifdef COFF2DEBUG
				{
				char *name;
				COFF2DEBUG("Adding COMMON space for %s\n",
					name=COFF2GetSymbolName(cofffile,i));
				free(name);
				}
#endif
				if( !LoaderHashFind(
						COFF2GetSymbolName(cofffile,i)))
					COFF2AddCOMMON(sym,i);
			}
			free(symname);
			break;
		case N_ABS:
		case N_DEBUG:
		case N_COMMENT:
#ifdef COFF2DEBUG
			COFF2DEBUG("Freeing %s, section %d\n",
				symname, sym->n_scnum );
#endif
			free(symname);
			break;
		case N_TEXT:
			if(sym->n_sclass == C_EXT &&
			   cofffile->saddr[sym->n_scnum-1]) {
				lookup[l].symName=symname;
				lookup[l].offset=(void (*)())
					    (cofffile->saddr[sym->n_scnum-1]+
							sym->n_value);
#ifdef COFF2DEBUG
			COFF2DEBUG("Adding %x %s\n",
				lookup[l].offset, lookup[l].symName );
#endif
			l++;
			}
			else {
#ifdef COFF2DEBUG
			  COFF2DEBUG( "Section not loaded %d\n",
							sym->n_scnum-1 );
#endif
			  free(symname);
			}
			break;
		case N_DATA:
			/*
			 * Note: COFF expects .data to be contiguous with
			 * .data, so that offsets for .data are relative to
			 * .text. We need to adjust for this, and make them
			 * relative to .data so that the relocation can be
			 * properly applied. This is needed becasue we allocate
			 * .data seperately from .text.
			 */
			if(sym->n_sclass == C_EXT &&
			   cofffile->saddr[sym->n_scnum-1]) {
			  lookup[l].symName=symname;
			  lookup[l].offset=(void (*)())
					    (cofffile->saddr[sym->n_scnum-1]+
					    sym->n_value-cofffile->txtsize);
#ifdef COFF2DEBUG
			COFF2DEBUG("Adding %x %s\n",
				lookup[l].offset, lookup[l].symName );
#endif
			l++;
			}
			else {
#ifdef COFF2DEBUG
			  COFF2DEBUG( "Section not loaded %d\n",
							sym->n_scnum-1 );
#endif
			  free(symname);
			}
			break;
		case N_BSS:
			/*
			 * Note: COFF expects .bss to be contiguous with
			 * .data, so that offsets for .bss are relative to
			 * .text. We need to adjust for this, and make them
			 * relative to .bss so that the relocation can be
			 * properly applied. This is needed becasue we allocate
			 * .bss seperately from .text and .data.
			 */
			if(sym->n_sclass == C_EXT &&
			   cofffile->saddr[sym->n_scnum-1]) {
			  lookup[l].symName=symname;
			  lookup[l].offset=(void (*)())
					    (cofffile->saddr[sym->n_scnum-1]+
					    sym->n_value-(cofffile->txtsize+
						cofffile->datsize));
#ifdef COFF2DEBUG
			COFF2DEBUG("Adding %x %s\n",
				lookup[l].offset, lookup[l].symName );
#endif
			l++;
			}
			else {
#ifdef COFF2DEBUG
			  COFF2DEBUG( "Section not loaded %d\n",
							sym->n_scnum-1 );
#endif
			  free(symname);
			}
			break;
		default:
			ErrorF("Unknown Section number %d\n", sym->n_scnum );
			free(symname);
			break;
		}
	}

lookup[l].symName=NULL; /* Terminate the list */
LoaderAddSymbols(cofffile->handle,lookup);
free(lookup);

COFF2CreateCOMMON(cofffile);

/*
 * remove the COFF symbols that will show up in every module
 */
LoaderHashDelete(".text");
LoaderHashDelete(".data");
LoaderHashDelete(".bss");
}

#define SecOffset(index) cofffile->sections[index].s_scnptr
#define SecSize(index) cofffile->sections[index].s_size
#define RelOffset(index) cofffile->sections[index].s_relptr
#define RelSize(index) (cofffile->sections[index].s_nreloc*RELSZ)

/*
 * COFF2CollectSections
 *
 * Do the work required to load each section into memory.
 */
static void
COFF2CollectSections(cofffile)
COFF2ModulePtr	cofffile;
{
unsigned short	i;

/*
 * Find and identify all of the Sections
 */

#ifdef COFF2DEBUG
COFF2DEBUG("COFF2CollectSections(): %d sections\n", cofffile->numsh );
#endif

for( i=0; i<cofffile->numsh; i++) {
#ifdef COFF2DEBUG
	COFF2DEBUG("%d %s\n", i, cofffile->sections[i].s_name );
#endif
	/* .text */
	if( strcmp(cofffile->sections[i].s_name,
							".text" ) == 0 ) {
		cofffile->text=_LoaderFileToMem(cofffile->fd,
					SecOffset(i),SecSize(i),".text");
		cofffile->saddr[i]=cofffile->text;
		cofffile->txtndx=i;
		cofffile->txtsize=SecSize(i);
		cofffile->txtrelsize=RelSize(i);
		cofffile->reladdr[i]=_LoaderFileToMem(cofffile->fd,
					RelOffset(i), RelSize(i),".rel.text");
#ifdef COFF2DEBUG
COFF2DEBUG(".text starts at %x (%x bytes)\n", cofffile->text, cofffile->txtsize );
#endif
		continue;
		}
	/* .data */
	if( strcmp(cofffile->sections[i].s_name,
							".data" ) == 0 ) {
		cofffile->data=_LoaderFileToMem(cofffile->fd,
					SecOffset(i),SecSize(i),".data");
		cofffile->saddr[i]=cofffile->data;
		cofffile->datndx=i;
		cofffile->datsize=SecSize(i);
		cofffile->datrelsize=RelSize(i);
		cofffile->reladdr[i]=_LoaderFileToMem(cofffile->fd,
					RelOffset(i), RelSize(i),".rel.data");
#ifdef COFF2DEBUG
COFF2DEBUG(".data starts at %x (%x bytes)\n", cofffile->data, cofffile->datsize );
#endif
		continue;
		}
	/* .bss */
	if( strcmp(cofffile->sections[i].s_name,
							".bss" ) == 0 ) {
		if( SecSize(i) )
			cofffile->bss=calloc(1,SecSize(i));
		else
			cofffile->bss=NULL;
		cofffile->saddr[i]=cofffile->bss;
		cofffile->bssndx=i;
		cofffile->bsssize=SecSize(i);
#ifdef COFF2DEBUG
COFF2DEBUG(".bss starts at %x (%x bytes)\n", cofffile->bss, cofffile->bsssize );
#endif
		continue;
		}
	/* .comment */
	if( strcmp(cofffile->sections[i].s_name,
							".comment" ) == 0 ) {
		continue;
		}
#ifdef COFF2DEBUG
COFF2DEBUG("Not loading %s\n", cofffile->sections[i].s_name );
#endif
	}
}

/*
 * Public API for the COFF2 implementation of the loader.
 */
void *
COFF2LoadModule(modtype,modname,handle,cofffd)
int	modtype; /* Always LD_COFF2OBJECT */
char	*modname;
int	handle;
int	cofffd;
{
COFF2ModulePtr	cofffile;
FILHDR *header;
int stroffset; /* offset of string table */

#ifdef COFF2DEBUG
COFF2DEBUG("COFF2LoadModule(%x,%s,%x,%x)\n",modtype,modname,
					handle,cofffd );
#endif

if( modtype != LD_COFFOBJECT ) {
	ErrorF( "COFF2LoadModule(): modtype != COFF\n" );
	return NULL;
	}

if( (cofffile=(COFF2ModulePtr)calloc(1,sizeof(COFF2ModuleRec))) == NULL ) {
	ErrorF( "Unable to allocate COFF2ModuleRec\n" );
	return NULL;
	}

cofffile->handle=handle;
cofffile->fd=cofffd;

/*
 *  Get the COFF2 header
 */
cofffile->header=(FILHDR *)_LoaderFileToMem(cofffd,0,sizeof(FILHDR),"header");
header=(FILHDR *)cofffile->header;

/*
 * Get the section table
 */
cofffile->numsh=header->f_nscns;
cofffile->secsize=(header->f_nscns*SCNHSZ);
cofffile->sections=(SCNHDR *)_LoaderFileToMem(cofffd,FILHSZ+header->f_opthdr,
					cofffile->secsize, "sections");
cofffile->saddr=calloc(cofffile->numsh,sizeof(unsigned char *));
cofffile->reladdr=calloc(cofffile->numsh,sizeof(unsigned char *));

/*
 * Load the optional header if we need it ?????
 */

/*
 * Load the rest of the desired sections
 */
COFF2CollectSections(cofffile);

/*
 * load the string table (must be done before we process symbols).
 */
stroffset=header->f_symptr+(header->f_nsyms*SYMESZ);

_LoaderFileRead(cofffd,stroffset,&(cofffile->strsize),sizeof(int));

stroffset+=4; /* Move past the size */
cofffile->strsize-=sizeof(int);	/* size includes itself, so reduce by 4 */
cofffile->strtab=_LoaderFileToMem(cofffd,stroffset,cofffile->strsize,"strings");

/*
 * add symbols
 */
COFF2_GetSymbols(cofffile);

/*
 * Do relocations
 */
COFF2_DoRelocations(cofffile);

return (void *)cofffile;
}

void
COFF2ResolveSymbols()
{
coff_reloc	*rel,*oldlist;

#ifdef COFF2DEBUG
COFF2DEBUG("COFF2ResolvSymbols()\n" );
#endif

/* avoid looping */
oldlist=listResolve;
listResolve=NULL;

while( (rel=oldlist) != NULL )
	{
	oldlist=rel->next;
	COFF2_RelocateEntry(rel->file,rel->secndx,rel->rel);
	free(rel);
	}
}

int
COFF2CheckForUnresolved()
{
char	*name;
coff_reloc	*crel;

if( (crel=listResolve) == NULL )
	return 0;

while( crel )
	{
	ErrorF("Unresolved Symbol %s from %s\n",
		name=COFF2GetSymbolName(crel->file, crel->rel->r_symndx),
			_LoaderHandleToName(crel->file->handle));
	free(name);
	crel=crel->next;
	}
return 1;
}

void
COFF2UnloadModule( modptr )
void *modptr;
{
COFF2ModulePtr	cofffile = (COFF2ModulePtr)modptr;
coff_reloc	*relptr, *reltptr, **brelptr;

/*
 * Delete any unresolved relocations
 */

relptr=listResolve;
brelptr=&listResolve;

while(relptr) {
	if( relptr->file == cofffile ) {
		*brelptr=relptr->next;	/* take it out of the list */
		reltptr=relptr;		/* save pointer to this node */
		relptr=relptr->next;	/* advance the pointer */
		free(reltptr);		/* free the node */
		}
	else
		relptr=relptr->next;	/* advance the pointer */
	}

/*
 * Delete any symbols in the symbols table.
 */

LoaderHashTraverse( (void *)cofffile, COFF2hashCleanOut ) ;

/*
 * Free the sections that were allocated.
 */
#define CheckandFree(ptr,size)	if(ptr) _LoaderFreeFileMem(ptr,size);

CheckandFree(cofffile->strtab,cofffile->strsize)
CheckandFree(cofffile->symtab,cofffile->symsize)
CheckandFree(cofffile->text,cofffile->txtsize)
CheckandFree(cofffile->reladdr[cofffile->txtndx],cofffile->txtrelsize)
CheckandFree(cofffile->data,cofffile->datsize)
CheckandFree(cofffile->reladdr[cofffile->datndx],cofffile->datrelsize)
CheckandFree(cofffile->bss,cofffile->bsssize)
if( cofffile->common )
	free(cofffile->common);
/*
 * Free the section table, and section pointer array
 */
_LoaderFreeFileMem(cofffile->sections,cofffile->secsize);
free(cofffile->saddr);
free(cofffile->reladdr);
_LoaderFreeFileMem(cofffile->header,sizeof(FILHDR));
/*
 * Free the COFF2ModuleRec
 */
free(cofffile);

return;
}

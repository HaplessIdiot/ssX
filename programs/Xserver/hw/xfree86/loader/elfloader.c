/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/elfloader.c,v 1.9 1997/09/25 16:13:59 hohndel Exp $ */




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
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef QNX
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/stat.h>

#ifdef DBMALLOC
#include <debug/malloc.h>
#define Xalloc(size) malloc(size)
#define Xcalloc(size) calloc(1,(size))
#define Xfree(size) free(size)
#endif

#include "Xos.h"
#include "os.h"
#include "elf.h"

#include "sym.h"
#include "loader.h"
#include "elfloader.h"

#include "xf86.h"
#include "xf86Priv.h"
/*
#ifndef LDTEST
#define ELFDEBUG ErrorF
#endif
*/

/*
 * This structure contains all of the information about a module
 * that has been loaded.
 */

typedef	struct {
	int	handle;
	int	module;
	int	fd;
	loader_funcs	*funcs;
	Elf32_Ehdr 	*header;/* file header */
	int	numsh;
	Elf32_Shdr	*sections;/* Start address of the section table */
	int	secsize;	/* size of the section table */
	unsigned char	**saddr;/* Start addresss of the sections table */
	unsigned char	*shstraddr; /* Start address of the section header string table */
	int	shstrndx;	/* index of the section header string table */
	int	shstrsize;	/* size of the section header string table */
	unsigned char *straddr;	/* Start address of the string table */
	int	strndx;		/* index of the string table */
	int	strsize;	/* size of the string table */
	unsigned char *text;	/* Start address of the .text section */
	int	txtndx;		/* index of the .text section */
	int	txtsize;	/* size of the .text section */
	unsigned char *data;	/* Start address of the .data section */
	int	datndx;		/* index of the .data section */
	int	datsize;	/* size of the .data section */
	unsigned char *data1;	/* Start address of the .data section */
	int	dat1ndx;		/* index of the .data section */
	int	dat1size;	/* size of the .data section */
	unsigned char *bss;	/* Start address of the .bss section */
	int	bssndx;		/* index of the .bss section */
	int	bsssize;	/* size of the .bss section */
	unsigned char *rodata;	/* Start address of the .rodata section */
	int	rodatndx;	/* index of the .rodata section */
	int	rodatsize;	/* size of the .rodata section */
	unsigned char *rodata1;	/* Start address of the .rodata section */
	int	rodat1ndx;	/* index of the .rodata section */
	int	rodat1size;	/* size of the .rodata section */
	Elf32_Sym *symtab;	/* Start address of the .symtab section */
	int	symndx;		/* index of the .symtab section */
	int	symsize;	/* size of the .symtab section */
	unsigned char *reltext;	/* Start address of the .rel.text section */
	int	reltxtndx;	/* index of the .rel.text section */
	int	reltxtsize;	/* size of the .rel.text section */
	unsigned char *reldata;	/* Start address of the .rel.data section */
	int	reldatndx;	/* index of the .rel.data section */
	int	reldatsize;	/* size of the .rel.data section */
	unsigned char *relrodata;/* Start address of the .rel.rodata section */
	int	relrodatndx;	/* index of the .rel.rodata section */
	int	relrodatsize;	/* size of the .rel.rodata section */
	unsigned char *common;	/* Start address of the SHN_COMMON space */
	int	comsize;	/* size of the SHN_COMMON space */
	}	ELFModuleRec, *ELFModulePtr;

/*
 * If a relocation is unable to be satisfied, then put it on a list
 * to try later after more modules have been loaded.
 */
typedef struct _elf_reloc {
#if defined(i386) || defined(__alpha__)
	Elf32_Rel	*rel;
#endif
#if defined(__powerpc__) || defined(__mc68000__)
	Elf32_Rela	*rel;
#endif
	ELFModulePtr	file;
	unsigned char	*secp;
	struct _elf_reloc	*next;
} ELFRelocRec;

/*
 * symbols with a st_shndx of COMMON need to have space allocated for them.
 *
 * Gather all of these symbols together, and allocate one chunk when we
 * are done.
 */
typedef struct _elf_COMMON {
	Elf32_Sym	   *sym;
	struct _elf_COMMON *next;
} ELFCommonRec;

static	ELFCommonPtr listCOMMON;

/* Prototypes for static functions */
static int ELFhashCleanOut(void *, itemPtr);
static char *ElfGetStringIndex(ELFModulePtr, int, int);
static char *ElfGetString(ELFModulePtr, int);
static char *ElfGetSectionName(ELFModulePtr, int);
#if defined(__powerpc__) || defined(__mc68000__)
static ELFRelocPtr ElfDelayRelocation(ELFModulePtr, unsigned char *, Elf32_Rela *);
#else
static ELFRelocPtr ElfDelayRelocation(ELFModulePtr, unsigned char *, Elf32_Rel *);
#endif
static ELFCommonPtr ElfAddCOMMON(Elf32_Sym *);
static LOOKUP *ElfCreateCOMMON(ELFModulePtr);
static char *ElfGetSymbolNameIndex(ELFModulePtr, int, int);
static char *ElfGetSymbolName(ELFModulePtr, int);
static Elf32_Addr ElfGetSymbolValue(ELFModulePtr, int);
#if defined(__powerpc__) || defined(__mc68000__)
static ELFRelocPtr Elf_RelocateEntry(ELFModulePtr, unsigned char *, Elf32_Rela *);
#else
static ELFRelocPtr Elf_RelocateEntry(ELFModulePtr, unsigned char *, Elf32_Rel *);
#endif
static ELFRelocPtr ELFCollectRelocations(ELFModulePtr, int);
static LOOKUP *ELF_GetSymbols(ELFModulePtr);
static void ELFCollectSections(ELFModulePtr);

/*
 * Utility Functions
 */


static int
ELFhashCleanOut(voidptr, item)
void *voidptr;
itemPtr item ;
{
    ELFModulePtr module = (ELFModulePtr) voidptr;
    return (module->handle == item->handle);
}

/*
 * Manage listResolv
 */
static ELFRelocPtr
ElfDelayRelocation(elffile,secp,rel)
ELFModulePtr	elffile;
unsigned char	*secp;
#if defined(i386) || defined(__alpha__)
Elf32_Rel	*rel;
#endif
#if defined(__powerpc__) || defined(__mc68000__)
Elf32_Rela	*rel;
#endif
{
    ELFRelocPtr	reloc;

    if ((reloc = (ELFRelocPtr) xalloc(sizeof(ELFRelocRec))) == NULL) {
	ErrorF( "ElfDelayRelocation() Unable to allocate memory!!!!\n" );
	return 0;
    }
    reloc->file=elffile;
    reloc->secp=secp;
    reloc->rel=rel;
    reloc->next=0;
#ifdef ELFDEBUG
ErrorF("DelayReloc %x: file %x, sec %x, r_offset 0x%x, r_info 0x%x\n", reloc, elffile, secp, rel->r_offset, rel->r_info);
#endif
    return reloc;
}

/*
 * Manage listCOMMON
 */
static ELFCommonPtr
ElfAddCOMMON(sym)
Elf32_Sym	*sym;
{
    ELFCommonPtr common;

    if ((common = (ELFCommonPtr) xalloc(sizeof(ELFCommonRec))) == NULL) {
	ErrorF( "ElfAddCOMMON() Unable to allocate memory!!!!\n" );
	return 0;
    }
    common->sym=sym;
    common->next=0;
    return common;
}

static LOOKUP *
ElfCreateCOMMON(elffile)
ELFModulePtr	elffile;
{
    int	numsyms=0,size=0,l=0;
    int	offset=0;
    LOOKUP	*lookup;
    ELFCommonPtr common;

    if (listCOMMON == NULL)
	return NULL;

    for (common = listCOMMON; common; common = common->next) {
	size+=common->sym->st_size;
	numsyms++;
    }

#ifdef ELFDEBUG
    ELFDEBUG("ElfCreateCOMMON() %d entries (%d bytes) of COMMON data\n",
	     numsyms, size );
#endif

    if((lookup = (LOOKUP *) xalloc((numsyms+1)*sizeof(LOOKUP))) == NULL) {
	ErrorF( "ElfCreateCOMMON() Unable to allocate memory!!!!\n" );
	return 0;
    }

    elffile->comsize=size;
    if((elffile->common = (unsigned char *) xcalloc(1,size)) == NULL) {
	ErrorF( "ElfCreateCOMMON() Unable to allocate memory!!!!\n" );
	return 0;
    }

    /* Traverse the common list and create a lookup table with all the
     * common symbols.  Destroy the common list in the process.
     * See also ResolveSymbols.
     */
    while(listCOMMON) {
	common=listCOMMON;
	/* this is xf86strdup because is should be more efficient. it is freed
	 * with xfree
	 */
	lookup[l].symName = (char *)xf86strdup(ElfGetString(elffile,common->sym->st_name));
	lookup[l].offset = (void (*)())(elffile->common + offset);
#ifdef ELFDEBUG
	ELFDEBUG("Adding common %x %s\n", lookup[l].offset, lookup[l].symName );
#endif
	listCOMMON=common->next;
	offset+=common->sym->st_size;
	xfree(common);
	l++;
    }
    /* listCOMMON == 0 */
    lookup[l].symName=NULL; /* Terminate the list. */
    return lookup;
}


/*
 * String Table
 */
static char *
ElfGetStringIndex(file, offset, index)
ELFModulePtr	file;
int offset;
int index;
{
    if( !offset || !index )
        return "";

    return (char *)(file->saddr[index]+offset);
}

static char *
ElfGetString(file, offset)
ELFModulePtr	file;
int offset;
{
    return ElfGetStringIndex( file, offset, file->strndx );
}

static char *
ElfGetSectionName(file, offset)
ELFModulePtr	file;
int offset;
{
    return (char *)(file->shstraddr+offset);
}



/*
 * Symbol Table
 */

/*
 * Get symbol name
 */
static char *
ElfGetSymbolNameIndex(elffile, index, secndx)
ELFModulePtr	elffile;
int index;
int secndx;
{
    Elf32_Sym	*syms;

#ifdef ELFDEBUG
    ELFDEBUG("ElfGetSymbolNameIndex(%x,%x) ",index, secndx );
#endif

    syms=(Elf32_Sym *)elffile->saddr[secndx];

#ifdef ELFDEBUG
    ELFDEBUG("%s ",ElfGetString(elffile, syms[index].st_name));
    ELFDEBUG("%x %x ",ELF32_ST_BIND(syms[index].st_info),
	     ELF32_ST_TYPE(syms[index].st_info));
    ELFDEBUG("%lx\n",syms[index].st_value);
#endif

    return ElfGetString(elffile,syms[index].st_name );
}

static char *
ElfGetSymbolName(elffile, index)
ELFModulePtr	elffile;
int index;
{
    char	*name,*symname;
    symname=ElfGetSymbolNameIndex( elffile, index, elffile->symndx );
    if( symname == NULL )
	return NULL;
   
    name=(char *) xalloc(strlen(symname)+1);
    if (!name)
	FatalError("ELFGetSymbolName: Out of memory\n");

    strcpy(name,symname);

    return name;
}

static Elf32_Addr
ElfGetSymbolValue(elffile, index)
ELFModulePtr	elffile;
int index;
{
    Elf32_Sym	*syms;
    Elf32_Addr symval=0;	/* value of the indicated symbol */
    char *symname = NULL;		/* name of symbol in relocation */
    itemPtr symbol = NULL;		/* name/value of symbol */

    syms=(Elf32_Sym *)elffile->saddr[elffile->symndx];

    switch( ELF32_ST_TYPE(syms[index].st_info) )
	{
	case STT_NOTYPE:
	case STT_OBJECT:
	case STT_FUNC:
	    switch( ELF32_ST_BIND(syms[index].st_info) )
		{
		case STB_LOCAL:
		    symval=(Elf32_Addr)(
					elffile->saddr[syms[index].st_shndx]+
					syms[index].st_value);
		    break;
		case STB_GLOBAL:
		case STB_WEAK: /* STB_WEAK seems like a hack to cover for
					some other problem */
		    symname=
			ElfGetString(elffile,syms[index].st_name);
		    symbol = LoaderHashFind(symname);
		    if( symbol == 0 ) {
			return 0;
			}
		    symval=(Elf32_Addr)symbol->address;
		    symname=NULL; /* ElfGetString() does not return
					a malloc string */
		    break;
		default:
		    symval=0;
		    ErrorF(
			   "ElfGetSymbolValue(), unhandled symbol scope %x\n",
			   ELF32_ST_BIND(syms[index].st_info) );
		    break;
		}
#ifdef ELFDEBUG
	    ELFDEBUG( "%x\t", symbol );
	    ELFDEBUG( "%lx\t", symval );
	    ELFDEBUG( "%s\n", symname );
#endif
	    break;
	case STT_SECTION:
	    symval=(Elf32_Addr)elffile->saddr[syms[index].st_shndx];
#ifdef ELFDEBUG
	    ELFDEBUG( "ST_SECTION %lx\n", symval );
#endif
	    break;
	case STT_FILE:
	case STT_LOPROC:
	case STT_HIPROC:
	default:
	    symval=0;
	    ErrorF( "ElfGetSymbolValue(), unhandled symbol type %x\n",
		    ELF32_ST_TYPE(syms[index].st_info) );
	    break;
	}
    if( symname ) xfree(symname);
    return symval;
}

#if defined(__powerpc__)
/*
 * This function returns the address of a pseudo PLT routine which can
 * be used to compute a function offset. This is needed because loaded
 * modules have an offset from the .text section of greater than 24 bits.
 * The code generated makes the assumption that all function entry points
 * will be within a 24 bit offset (non-PIC code).
 */
static Elf32_Addr
ElfGetPltAddr(elffile, index)
ELFModulePtr	elffile;
int index;
{
    Elf32_Sym	*syms;
    Elf32_Addr symval=0;	/* value of the indicated symbol */
    char *symname;		/* name of symbol in relocation */
    itemPtr symbol;		/* name/value of symbol */

    syms=(Elf32_Sym *)elffile->saddr[elffile->symndx];

    switch( ELF32_ST_TYPE(syms[index].st_info) )
	{
	case STT_NOTYPE:
	case STT_OBJECT:
	case STT_FUNC:
	    switch( ELF32_ST_BIND(syms[index].st_info) )
		{
		case STB_GLOBAL:
		    symname=
			ElfGetString(elffile,syms[index].st_name);
		    symbol=LoaderHashFind(symname);
		    if( symbol == 0 )
			return 0;
/*
 * Here we are building up a pseudo Plt function that can make a call to
 * a function that has an offset greater than 24 bits. The following code
 * is being used to implement this.

     1  00000000                                .extern realfunc
     2  00000000                                .global pltfunc
     3  00000000                        pltfunc:
     4  00000000  3d 80 00 00                   lis     r12,hi16(realfunc)
     5  00000004  61 8c 00 00                   ori     r12,r12,lo16(realfunc)
     6  00000008  7d 89 03 a6                   mtctr   r12
     7  0000000c  4e 80 04 20                   bctr

 */

		    symbol->code.plt[0]=0x3d80; /* lis     r12 */
		    symbol->code.plt[1]=(((Elf32_Addr)symbol->address)&0xffff0000)>>16;
		    symbol->code.plt[2]=0x618c; /* ori     r12,r12 */
		    symbol->code.plt[3]=(((Elf32_Addr)symbol->address)&0xffff);
		    symbol->code.plt[4]=0x7d89; /* mtcr    r12 */
		    symbol->code.plt[5]=0x03a6;
		    symbol->code.plt[6]=0x4e80; /* bctr */
		    symbol->code.plt[7]=0x0420;
		    symbol->address=(char *)&symbol->code.plt[0];
		    symval=(Elf32_Addr)symbol->address;
		    ppc_flush_icache(&symbol->code.plt[0]);
		    ppc_flush_icache(&symbol->code.plt[6]);
		    break;
		default:
		    symval=0;
		    ErrorF(
			   "ElfGetPltAddr(), unhandled symbol scope %x\n",
			   ELF32_ST_BIND(syms[index].st_info) );
		    break;
		}
#ifdef ELFDEBUG
	    ELFDEBUG( "ElfGetPlt: symbol=%x\t", symbol );
	    ELFDEBUG( "newval=%x\t", symval );
	    ELFDEBUG( "name=\"%s\"\n", symname );
#endif
	    break;
	case STT_SECTION:
	case STT_FILE:
	case STT_LOPROC:
	case STT_HIPROC:
	default:
	    symval=0;
	    ErrorF( "ElfGetPltAddr(), Unexpected symbol type %x",
		    ELF32_ST_TYPE(syms[index].st_info) );
	    ErrorF( "for a Plt request\n" );
	    break;
	}
    if( symname ) xfree(symname);
    return symval;
}
#endif /* __powerpc__ */

/*
 * Fix all of the relocations for the given section.
 */
static ELFRelocPtr
Elf_RelocateEntry(elffile, secp, rel)
ELFModulePtr	elffile;
unsigned char *secp;	/* Begining of the target section */
#if defined(i386) || defined(__alpha__)
Elf32_Rel	*rel;
#endif
#if defined(__powerpc__) || defined(__mc68000__)
Elf32_Rela	*rel;
#endif
{
    unsigned long *dest32;	/* address of the 32 bit place being modified */
#if defined(__powerpc__) || defined(__mc68000__)
    unsigned short *dest16;	/* address of the 16 bit place being modified */
#endif
    Elf32_Addr symval;	/* value of the indicated symbol */

#ifdef ELFDEBUG
#if defined(i386) || defined(__alpha__)
	ELFDEBUG( "%x %d %d\n", rel->r_offset,
		ELF32_R_SYM(rel->r_info),ELF32_R_TYPE(rel->r_info) );
#endif
#if defined(__powerpc__) || defined(__mc68000__)
	ELFDEBUG( "%x %d %d %x\n", rel->r_offset,
		ELF32_R_SYM(rel->r_info),ELF32_R_TYPE(rel->r_info),
		rel->r_addend );
#endif
#endif

    switch( ELF32_R_TYPE(rel->r_info) )
	{
#if defined(i386) || defined(__alpha__)
	case R_386_32:
	    dest32=(unsigned long *)(secp+rel->r_offset);
	    symval=ElfGetSymbolValue(elffile,
				     ELF32_R_SYM(rel->r_info));
	    if( symval == 0 ) {
#ifdef ELFDEBUG
		char *namestr;
		ELFDEBUG( "***Unable to resolve symbol %s\n",
			  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
		xfree(namestr);
#endif
		return ElfDelayRelocation(elffile,secp,rel);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "R_386_32\t");
	    ELFDEBUG( "dest32=%x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8lx\t", *dest32 );
#endif
	    *dest32=symval+(*dest32); /* S + A */
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8lx\n", *dest32 );
#endif
	    break;
	case R_386_PC32:
	    dest32=(unsigned long *)(secp+rel->r_offset);
	    symval=ElfGetSymbolValue(elffile,
				     ELF32_R_SYM(rel->r_info));
	    if( symval == 0 ) {
#ifdef ELFDEBUG
		char *namestr;
		ELFDEBUG( "***Unable to resolve symbol %s\n",
			  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
		xfree(namestr);
#endif
		return ElfDelayRelocation(elffile,secp,rel);
	    }

#ifdef ELFDEBUG
	    char *namestr;
	    ELFDEBUG( "R_386_PC32 %s\t",
			namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
	    xfree(namestr);
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%lx\t", symval );
	    ELFDEBUG( "dest32=%x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8lx\t", *dest32 );
#endif

	    *dest32=symval+(*dest32)-(Elf32_Addr)dest32; /* S + A - P */

#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8lx\n", *dest32 );
#endif

		break;
#endif /* i386/alpha */
#if defined(__mc68000__)
	case R_68K_32:
		dest32=(unsigned long *)(secp+rel->r_offset);
		symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
		if( symval == 0 ) {
#ifdef ELFDEBUG
			char *namestr;
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
			xfree(namestr);
#endif
			return ElfDelayRelocation(elffile,secp,rel);
			break;
			}
#ifdef ELFDEBUG
ELFDEBUG( "R_68K_32\t", dest32 );
ELFDEBUG( "dest32=%x\t", dest32 );
ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		*dest32=symval+(*dest32); /* S + A */
#ifdef ELFDEBUG
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
		break;
	case R_68K_PC32:
		dest32=(unsigned long *)(secp+rel->r_offset);
		symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
		if( symval == 0 ) {
#ifdef ELFDEBUG
			char *namestr;
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
			xfree(namestr);
#endif
			return ElfDelayRelocation(elffile,secp,rel);
			break;
			}

#ifdef ELFDEBUG
char *namestr;
ELFDEBUG( "R_68K_PC32 %s\t",
		  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
xfree(namestr);
ELFDEBUG( "secp=%x\t", secp );
ELFDEBUG( "symval=%x\t", symval );
ELFDEBUG( "dest32=%x\t", dest32 );
ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif

		*dest32=symval+(*dest32)-(Elf32_Addr)dest32; /* S + A - P */

#ifdef ELFDEBUG
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif

		break;
#endif /* __mc68000__ */
#if defined(__powerpc__)
	case R_PPC_DISP24: /* 11 */
	    dest32=(unsigned long *)(secp+rel->r_offset);
	    symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
	    if( symval == 0 ) {
#ifdef ELFDEBUG
		char *namestr;
		ELFDEBUG( "***Unable to resolve symbol %s\n",
		  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
		xfree(namestr);
#endif
		return ElfDelayRelocation(elffile,secp,rel);
	    }

#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_DISP24 %s\t", ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "dest32=%x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif

	    {
		unsigned long val;
		
		/* S + A - P >> 2 */
		val=((symval+(rel->r_addend)-(Elf32_Addr)dest32));
#ifdef ELFDEBUG
		ELFDEBUG("S+A-P=%x\t",val);
#endif
		val = val>>2;
		if( (val & 0x3f000000) != 0x3f000000 &&
		    (val & 0x3f000000) != 0x00000000 )  {
#ifdef ELFDEBUG
		    ELFDEBUG("R_PPC_DISP24 offset %x too large\n", val<<2);
#endif
		    symval = ElfGetPltAddr(elffile,ELF32_R_SYM(rel->r_info));
		    val=((symval+(rel->r_addend)-(Elf32_Addr)dest32));
#ifdef ELFDEBUG
		    ELFDEBUG("PLT offset is %x\n", val);
#endif
		    val=val>>2;
		    if( (val & 0x3f000000) != 0x3f000000 &&
		         (val & 0x3f000000) != 0x00000000 )
			   FatalError("R_PPC_DISP24 PLT offset %x too large\n", val<<2);
		}
		val &= 0x00ffffff;
		(*dest32)|=(val<<2);	/* The address part is always 0 */
		ppc_flush_icache(dest32);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_16HU: /* 31 */
	    dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    dest32=(unsigned long *)(dest16-1);

#endif
	    symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
	    if( symval == 0 ) {
#ifdef ELFDEBUG
		char *namestr;
		ELFDEBUG( "***Unable to resolve symbol %s\n",
		  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
		xfree(namestr);
#endif
		return ElfDelayRelocation(elffile,secp,rel);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_16HU\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest16=%x\t", dest16 );
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned short val;
		/* S + A */
		val=((symval+(rel->r_addend))&0xffff0000)>>16;
#ifdef ELFDEBUG
		ELFDEBUG("uhi16(S+A)=%x\t",val);
#endif
		*dest16=val; /* S + A */
		ppc_flush_icache(dest16);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_32: /* 32 */
	    dest32=(unsigned long *)(secp+rel->r_offset);
	    symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
	    if( symval == 0 ) {
#ifdef ELFDEBUG
		char *namestr;
		ELFDEBUG( "***Unable to resolve symbol %s\n",
		  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
		xfree(namestr);
#endif
		return ElfDelayRelocation(elffile,secp,rel);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_32\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned long val;
		/* S + A */
		val=symval+(rel->r_addend);
#ifdef ELFDEBUG
		ELFDEBUG("S+A=%x\t",val);
#endif
		*dest32=val; /* S + A */
		ppc_flush_icache(dest32);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_32UA: /* 33 */
	    dest32=(unsigned long *)(secp+rel->r_offset);
	    symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
	    if( symval == 0 ) {
#ifdef ELFDEBUG
		char *namestr;
		ELFDEBUG( "***Unable to resolve symbol %s\n",
		  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
		xfree(namestr);
#endif
		return ElfDelayRelocation(elffile,secp,rel);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_32UA\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned long val;
		unsigned char *dest8 = (unsigned char *)dest32;
		/* S + A */
		val=symval+(rel->r_addend);
#ifdef ELFDEBUG
		ELFDEBUG("S+A=%x\t",val);
#endif
		*dest8++=(val&0xff000000)>>24;
		*dest8++=(val&0x00ff0000)>>16;
		*dest8++=(val&0x0000ff00)>> 8;
		*dest8++=(val&0x000000ff);
		ppc_flush_icache(dest32);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_16H: /* 34 */
	    dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    dest32=(unsigned long *)(dest16-1);
#endif
	    symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
	    if( symval == 0 ) {
#ifdef ELFDEBUG
		char *namestr;
		ELFDEBUG( "***Unable to resolve symbol %s\n",
		  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
		xfree(namestr);
#endif
		return ElfDelayRelocation(elffile,secp,rel);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_16H\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symbol=%s\t", ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest16=%x\t", dest16 );
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned short val;
		unsigned short loval;
		/* S + A */
		val=((symval+(rel->r_addend))&0xffff0000)>>16;
		loval=(symval+(rel->r_addend))&0xffff;
		if( loval & 0x8000 ) {
		    /*
		     * This is hi16(), instead of uhi16(). Because of this,
		     * if the lo16() will produce a negative offset, then
		     * we have to increment this part of the address to get
		     * the correct final result.
		     */
		    val++;
		}
#ifdef ELFDEBUG
		ELFDEBUG("hi16(S+A)=%x\t",val);
#endif
		*dest16=val; /* S + A */
		ppc_flush_icache(dest16);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_16L: /* 35 */
	    dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    dest32=(unsigned long *)(dest16-1);
#endif
	    symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
	    if( symval == 0 ) {
#ifdef ELFDEBUG
		char *namestr;
		ELFDEBUG( "***Unable to resolve symbol %s\n",
		  namestr=ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
		xfree(namestr);
#endif
		return ElfDelayRelocation(elffile,secp,rel);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_16L\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest16=%x\t", dest16 );
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned short val;
		/* S + A */
		val=(symval+(rel->r_addend))&0xffff;
#ifdef ELFDEBUG
		ELFDEBUG("lo16(S+A)=%x\t",val);
#endif
		*dest16=val; /* S + A */
		ppc_flush_icache(dest16);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
#endif /* __powerpc__ */
	default:
	    ErrorF(
		   "Elf_RelocateEntry() Unsupported relocation type %d\n",
		   ELF32_R_TYPE(rel->r_info) );
	    break;
	}
    return 0;
}

static ELFRelocPtr
ELFCollectRelocations(elffile, index)
ELFModulePtr	elffile;
int	index; /* The section to use as relocation data */
{
    int	i, numrel;
    Elf32_Shdr	*sect=&(elffile->sections[index]);
#if defined(i386) || defined(__alpha__)
    Elf32_Rel	*rel=(Elf32_Rel *)elffile->saddr[index];
#endif
#if defined(__powerpc__) || defined(__mc68000__)
    Elf32_Rela	*rel=(Elf32_Rela *)elffile->saddr[index];
#endif
    Elf32_Sym	*syms;
    unsigned char *secp;	/* Begining of the target section */
    ELFRelocPtr reloc_head = NULL;
    ELFRelocPtr tmp;

    secp=(unsigned char *)elffile->saddr[sect->sh_info];
    syms = (Elf32_Sym *) elffile->saddr[elffile->symndx];

    numrel=sect->sh_size/sect->sh_entsize;

    for(i=0; i<numrel; i++ ) {
	tmp = ElfDelayRelocation(elffile,secp,&(rel[i]));
	tmp->next = reloc_head;
	reloc_head = tmp;
    }

    return reloc_head;
}

/*
 * ELF_GetSymbols()
 *
 * add the symbols to the symbol table maintained by the loader.
 */

static LOOKUP *
ELF_GetSymbols(elffile)
ELFModulePtr	elffile;
{
    Elf32_Sym	*syms;
    Elf32_Shdr	*sect;
    int		i, l, numsyms;
    LOOKUP	*lookup, *lookup_common, *p;
    ELFCommonPtr tmp;

    syms=elffile->symtab;
    sect=&(elffile->sections[elffile->symndx]);
    numsyms=sect->sh_size/sect->sh_entsize;

    if ((lookup = (LOOKUP *) xalloc((numsyms+1)*sizeof(LOOKUP))) == NULL)
	return 0;

    for(i=0,l=0; i<numsyms; i++)
	{
#ifdef ELFDEBUG
	    ELFDEBUG("value=%lx\tsize=%lx\tBIND=%x\tTYPE=%x\tndx=%x\t%s\n",
		     syms[i].st_value,syms[i].st_size,
		     ELF32_ST_BIND(syms[i].st_info),ELF32_ST_TYPE(syms[i].st_info),
		     syms[i].st_shndx,ElfGetString(elffile,syms[i].st_name) );
#endif

	    if( ELF32_ST_BIND(syms[i].st_info) == STB_LOCAL )
		/* Don't add static symbols to the symbol table */
		continue;

	    switch( ELF32_ST_TYPE(syms[i].st_info) )
		{
		case STT_OBJECT:
		case STT_FUNC:
		case STT_SECTION:
		case STT_NOTYPE:
		    switch(syms[i].st_shndx)
			{
			case SHN_ABS:
			    ErrorF("ELF_GetSymbols() Don't know how to handle SHN_ABS\n" );
			    break;
			case SHN_COMMON:
#ifdef ELFDEBUG
			    ELFDEBUG("Adding COMMON space for %s\n",
				     ElfGetString(elffile,syms[i].st_name) );
#endif
			    if (!LoaderHashFind(ElfGetString(elffile,
							syms[i].st_name))) {
				tmp = ElfAddCOMMON(&(syms[i]));
				if (tmp) {
				    tmp->next = listCOMMON;
				    listCOMMON = tmp;
				}
			    }			    
			    break;
			case SHN_UNDEF:
			    /*
			     * UNDEF will get resolved later, so the value
			     * doesn't really matter here.
			     */
			    /* since we don't know the value don't advertise the symbol */
			    break;
			default:
			    /* this is xf86strdup because is should be more
			     * efficient. it is freed with xfree
			     */
			    lookup[l].symName=(char *)xf86strdup(ElfGetString(elffile,syms[i].st_name));
			    lookup[l].offset=(void (*)())
					(elffile->saddr[syms[i].st_shndx]+
					syms[i].st_value);
#ifdef ELFDEBUG
			    ELFDEBUG("Adding symbol %x %s\n",
				     lookup[l].offset, lookup[l].symName );
#endif
			    l++;
			    break;
			}
		    break;
		case STT_FILE:
		case STT_LOPROC:
		case STT_HIPROC:
		    /* Skip this type */
#ifdef ELFDEBUG
		    ELFDEBUG("Skipping TYPE %d %s\n",
			     ELF32_ST_TYPE(syms[i].st_info),
			     ElfGetString(elffile,syms[i].st_name));
#endif
		    break;
		default:
		    ErrorF("ELF_GetSymbols(): Unepected symbol type %d\n",
			   ELF32_ST_TYPE(syms[i].st_info) );
		    break;
		}
	}

    lookup[l].symName=NULL; /* Terminate the list */

    lookup_common = ElfCreateCOMMON(elffile);
    if (lookup_common) {
	for (i = 0, p = lookup_common; p->symName; i++, p++)
	    ;
	memcpy(&(lookup[l]), lookup_common, i * sizeof (LOOKUP));

	xfree(lookup_common);
	l += i;
	lookup[l].symName = NULL;
    }


/*
 * Remove the ELF symbols that will show up in every object module.
 */
    for (i = 0, p = lookup; p->symName; i++, p++) {
	while (!strcmp(lookup[i].symName, ".text")
	       || !strcmp(lookup[i].symName, ".data")
	       || !strcmp(lookup[i].symName, ".bss")
	       || !strcmp(lookup[i].symName, ".comment")
	       || !strcmp(lookup[i].symName, ".note")
	       ) {
	    memmove(&(lookup[i]), &(lookup[i+1]), (l-- - i) * sizeof (LOOKUP));
	}
    }
    return lookup;
}

#define SecOffset(index) elffile->sections[index].sh_offset
#define SecSize(index) elffile->sections[index].sh_size

/*
 * ELFCollectSections
 *
 * Do the work required to load each section into memory.
 */
static void
ELFCollectSections(elffile)
ELFModulePtr	elffile;
{
    int	i;

/*
 * Find and identify all of the Sections
 */

#ifdef ELFDEBUG
    ELFDEBUG("%d sections\n", elffile->numsh );
#endif

    for( i=1; i<elffile->numsh; i++) {
#ifdef ELFDEBUG
	ELFDEBUG("%d %s\n", i, ElfGetSectionName(elffile, elffile->sections[i].sh_name) );
#endif
	/* .text */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".text" ) == 0 ) {
	    elffile->text=_LoaderFileToMem(elffile->fd,
					   SecOffset(i),SecSize(i),".text");
	    elffile->saddr[i]=elffile->text;
	    elffile->txtndx=i;
	    elffile->txtsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".text starts at %x\n", elffile->text );
#endif
	    continue;
	}
	/* .data */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".data" ) == 0 ) {
	    elffile->data=_LoaderFileToMem(elffile->fd,
					   SecOffset(i),SecSize(i),".data");
	    elffile->saddr[i]=elffile->data;
	    elffile->datndx=i;
	    elffile->datsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".data starts at %x\n", elffile->data );
#endif
		continue;
		}
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
							".data1" ) == 0 ) {
		elffile->data1=_LoaderFileToMem(elffile->fd,
					SecOffset(i),SecSize(i),".data1");
		elffile->saddr[i]=elffile->data1;
		elffile->dat1ndx=i;
		elffile->dat1size=SecSize(i);
#ifdef ELFDEBUG
ELFDEBUG(".data1 starts at %x\n", elffile->data1 );
#endif
		continue;
		}
	/* .bss */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".bss" ) == 0 ) {
	    if( SecSize(i) )
		elffile->bss = (unsigned char *) xcalloc(1, SecSize(i));
	    else
		elffile->bss=NULL;
	    elffile->saddr[i]=elffile->bss;
	    elffile->bssndx=i;
	    elffile->bsssize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".bss starts at %x\n", elffile->bss );
#endif
	    continue;
	}
	/* .rodata */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rodata" ) == 0 ) {
	    elffile->rodata=_LoaderFileToMem(elffile->fd,
					     SecOffset(i),SecSize(i),".rodata");
	    elffile->saddr[i]=elffile->rodata;
	    elffile->rodatndx=i;
	    elffile->rodatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rodata starts at %x\n", elffile->rodata );
#endif
		continue;
		}
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
							".rodata1" ) == 0 ) {
		elffile->rodata1=_LoaderFileToMem(elffile->fd,
					SecOffset(i),SecSize(i),".rodata1");
		elffile->saddr[i]=elffile->rodata1;
		elffile->rodat1ndx=i;
		elffile->rodat1size=SecSize(i);
#ifdef ELFDEBUG
ELFDEBUG(".rodata1 starts at %x\n", elffile->rodata1 );
#endif
		continue;
		}
	/* .symtab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".symtab" ) == 0 ) {
	    elffile->symtab=(Elf32_Sym *)_LoaderFileToMem(elffile->fd,
							  SecOffset(i),SecSize(i),".symtab");
	    elffile->saddr[i]=(unsigned char *)elffile->symtab;
	    elffile->symndx=i;
	    elffile->symsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".symtab starts at %x\n", elffile->symtab );
#endif
	    continue;
	}
	/* .strtab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".strtab" ) == 0 ) {
	    elffile->straddr=_LoaderFileToMem(elffile->fd,
					      SecOffset(i),SecSize(i),".strtab");
	    elffile->saddr[i]=(unsigned char *)elffile->straddr;
	    elffile->strndx=i;
	    elffile->strsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".strtab starts at %x\n", elffile->straddr );
#endif
		continue;
		}
#if defined(i386) || defined(__alpha__)
	/* .rel.text */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.text" ) == 0 ) {
	    elffile->reltext=_LoaderFileToMem(elffile->fd,
					      SecOffset(i),SecSize(i),".rel.text");
	    elffile->saddr[i]=(unsigned char *)elffile->reltext;
	    elffile->reltxtndx=i;
	    elffile->reltxtsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rel.text starts at %x\n", elffile->reltext );
#endif
	    continue;
	}
	/* .rel.data */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.data" ) == 0 ) {
	    elffile->reldata=_LoaderFileToMem(elffile->fd,
					      SecOffset(i),SecSize(i),".rel.data");
	    elffile->saddr[i]=(unsigned char *)elffile->reldata;
	    elffile->reldatndx=i;
	    elffile->reldatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rel.data starts at %x\n", elffile->reldata );
#endif
	    continue;
	}
	/* .rel.rodata */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.rodata" ) == 0 ) {
	    elffile->relrodata=_LoaderFileToMem(elffile->fd,
						SecOffset(i),SecSize(i),".rel.rodata");
	    elffile->saddr[i]=(unsigned char *)elffile->relrodata;
	    elffile->relrodatndx=i;
	    elffile->relrodatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rel.rodata starts at %x\n", elffile->relrodata );
#endif
		continue;
		}
#endif /* i386/alpha */
#if defined(__powerpc__) || defined(__mc68000__)
	/* .rela.text */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.text" ) == 0 ) {
	    elffile->reltext=_LoaderFileToMem(elffile->fd,
					      SecOffset(i),SecSize(i),".rela.text");
	    elffile->saddr[i]=(unsigned char *)elffile->reltext;
	    elffile->reltxtndx=i;
	    elffile->reltxtsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rela.text starts at %x\n", elffile->reltext );
#endif
	    continue;
	}
	/* .rela.data */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.data" ) == 0 ) {
	    elffile->reldata=_LoaderFileToMem(elffile->fd,
					      SecOffset(i),SecSize(i),".rela.data");
	    elffile->saddr[i]=(unsigned char *)elffile->reldata;
	    elffile->reldatndx=i;
	    elffile->reldatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rela.data starts at %x\n", elffile->reldata );
#endif
	    continue;
	}
	/* .rela.rodata */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.rodata" ) == 0 ) {
	    elffile->relrodata=_LoaderFileToMem(elffile->fd,
						SecOffset(i),SecSize(i),".rela.rodata");
	    elffile->saddr[i]=(unsigned char *)elffile->relrodata;
	    elffile->relrodatndx=i;
	    elffile->relrodatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rela.rodata starts at %x\n", elffile->relrodata );
#endif
	    continue;
	}
#endif /* __powerpc__ || __mc68000__ */
	/* .shstrtab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".shstrtab" ) == 0 ) {
	    continue;
	}
	/* .comment */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".comment" ) == 0 ) {
	    continue;
	}
	/* .debug */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".debug" ) == 0 ) {
	    continue;
	}
	/* .line */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".line" ) == 0 ) {
	    continue;
	}
	/* .note */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".note" ) == 0 ) {
	    continue;
	}
	/* .rel.debug */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.debug" ) == 0 ) {
	    continue;
	}
	/* .rel.line */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.line" ) == 0 ) {
	    continue;
	}
	/* .stab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".stab" ) == 0 ) {
	    continue;
	}
	/* .rel.stab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.stab" ) == 0 ) {
	    continue;
	}
	/* .stabstr */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".stabstr" ) == 0 ) {
	    continue;
	}
#if defined(__powerpc__) || defined(__mc68000__)
	/* .rela.tdesc */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.tdesc" ) == 0 ) {
	    continue;
	}
	/* .rela.debug_line */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.debug_line" ) == 0 ) {
	    continue;
	}
	/* .tdesc */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".tdesc" ) == 0 ) {
	    continue;
	}
	/* .debug_line */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".debug_line" ) == 0 ) {
	    continue;
	}
	/* $0001300 */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   "$0001300" ) == 0 ) {
	    continue;
	}
#endif
	ErrorF("Not loading %s\n",
	       ElfGetSectionName(elffile, elffile->sections[i].sh_name) );
    }

/* Done with this, so free it */
    _LoaderFreeFileMem(elffile->shstraddr,elffile->shstrsize);
    elffile->shstraddr=NULL;
}

/*
 * Public API for the ELF implementation of the loader.
 */
void *
ELFLoadModule(modrec, elffd, ppLookup)
loaderPtr	modrec;
int	elffd;
LOOKUP **ppLookup;
{
    ELFModulePtr elffile;
    Elf32_Ehdr   *header;
    ELFRelocPtr  elf_reloc, tail;
    LOOKUP *p;
    void	*v;

    if ((elffile = (ELFModulePtr) xcalloc(1, sizeof(ELFModuleRec))) == NULL) {
	ErrorF( "Unable to allocate ELFModuleRec\n" );
	return NULL;
    }

    elffile->handle=modrec->handle;
    elffile->module=modrec->module;
    elffile->fd=elffd;
    v=elffile->funcs=modrec->funcs;

/*
 *  Get the ELF header
 */
    elffile->header=(Elf32_Ehdr*)_LoaderFileToMem(elffd,0,sizeof(Elf32_Ehdr),"header");
    header=(Elf32_Ehdr *)elffile->header;

/*
 * Get the section table
 */
    elffile->numsh=header->e_shnum;
    elffile->secsize=(header->e_shentsize*header->e_shnum);
    elffile->sections=(Elf32_Shdr *)_LoaderFileToMem(elffd,header->e_shoff,
						     elffile->secsize, "sections");
    elffile->saddr=(unsigned char **) xcalloc(elffile->numsh,
					      sizeof(unsigned char *));

/*
 * Get the section header string table
 */
    elffile->shstrsize = SecSize(header->e_shstrndx);
    elffile->shstraddr = _LoaderFileToMem(elffd,SecOffset(header->e_shstrndx),
					  SecSize(header->e_shstrndx),".shstrtab");
    elffile->shstrndx = header->e_shstrndx;

/*
 * Load the rest of the desired sections
 */
    ELFCollectSections(elffile);

/*
 * add symbols
 */
    *ppLookup = ELF_GetSymbols(elffile);

/*
 * Do relocations
 */
    if (elffile->reltxtndx) {
	elf_reloc = ELFCollectRelocations(elffile,elffile->reltxtndx);
	if (elf_reloc) {
	    for (tail = elf_reloc; tail->next; tail = tail->next)
		;
	    tail->next = _LoaderGetRelocations(v)->elf_reloc;
	    _LoaderGetRelocations(v)->elf_reloc = elf_reloc;
	}
    }
    if (elffile->reldatndx) {
	elf_reloc = ELFCollectRelocations(elffile,elffile->reldatndx);
	if (elf_reloc) {
	    for (tail = elf_reloc; tail->next; tail = tail->next)
		;
	    tail->next = _LoaderGetRelocations(v)->elf_reloc;
	    _LoaderGetRelocations(v)->elf_reloc = elf_reloc;
	}
    }
    if (elffile->relrodatndx) {
	elf_reloc = ELFCollectRelocations(elffile,elffile->relrodatndx);
	if (elf_reloc) {
	    for (tail = elf_reloc; tail->next; tail = tail->next)
		;
	    tail->next = _LoaderGetRelocations(v)->elf_reloc;
	    _LoaderGetRelocations(v)->elf_reloc = elf_reloc;
	}
    }

    return (void *)elffile;
}

void
ELFResolveSymbols(mod)
void *mod;
{
    ELFRelocPtr newlist, p, tmp;

    /* Try to relocate everything.  Build a new list containing entries
     * which we failed to relocate.  Destroy the old list in the process.
     */
    newlist = 0;
    for (p = _LoaderGetRelocations(mod)->elf_reloc; p; ) {
#ifdef ELFDEBUG
	ErrorF("ResolveSymbols: file %x, sec %x, r_offset 0x%x, r_info 0x%x\n",
	       p->file, p->secp, p->rel->r_offset, p->rel->r_info);
#endif
	tmp = Elf_RelocateEntry(p->file, p->secp, p->rel);
	if (tmp) {
	    /* Failed to relocate.  Keep it in the list. */
	    tmp->next = newlist;
	    newlist = tmp;
	}
	tmp = p;
	p = p->next;
	xfree(tmp);
    }
    _LoaderGetRelocations(mod)->elf_reloc=newlist;
}

int
ELFCheckForUnresolved(color_depth, mod)
int	color_depth;
void	*mod;
{
    ELFRelocPtr	erel;
    char	*name, **addr;
    int flag, fatalsym=0;

    if ((erel = _LoaderGetRelocations(mod)->elf_reloc) == NULL)
	return 0;

    while( erel )
	{
	addr = (char**)(erel->secp+erel->rel->r_offset);
	*addr += ((int) &LoaderDefaultFunc - (int)addr);
	name = ElfGetSymbolName(erel->file, ELF32_R_SYM(erel->rel->r_info));
        flag = _LoaderHandleUnresolved(name,
			_LoaderHandleToName(erel->file->handle), color_depth);
        if(flag) fatalsym = 1;
	xfree(name);
	erel=erel->next;
	}
    return fatalsym;
}

void
ELFUnloadModule(modptr)
void *modptr;
{
    ELFModulePtr elffile = (ELFModulePtr)modptr;
    ELFRelocPtr  relptr, reltptr, *brelptr;

/*
 * Delete any unresolved relocations
 */

    relptr=_LoaderGetRelocations(elffile->funcs)->elf_reloc;
    brelptr=&(relptr);

    while(relptr) {
	if( relptr->file == elffile ) {
	    *brelptr=relptr->next;	/* take it out of the list */
	    reltptr=relptr;		/* save pointer to this node */
	    relptr=relptr->next;	/* advance the pointer */
	    xfree(reltptr);		/* free the node */
	}
	else
	    relptr=relptr->next;	/* advance the pointer */
    }

/*
 * Delete any symbols in the symbols table.
 */

    LoaderHashTraverse((void *)elffile, ELFhashCleanOut);

/*
 * Free the sections that were allocated.
 */
#define CheckandFree(ptr,size)  if(ptr) _LoaderFreeFileMem((ptr),(size))

    CheckandFree(elffile->straddr,elffile->strsize);
    CheckandFree(elffile->symtab,elffile->symsize);
    CheckandFree(elffile->text,elffile->txtsize);
    CheckandFree(elffile->data,elffile->datsize);
    CheckandFree(elffile->bss,elffile->bsssize);
    CheckandFree(elffile->rodata,elffile->rodatsize);
    CheckandFree(elffile->reltext,elffile->reltxtsize);
    CheckandFree(elffile->reldata,elffile->reldatsize);
    CheckandFree(elffile->relrodata,elffile->relrodatsize);
    if( elffile->common )
	xfree(elffile->common);
/*
 * Free the section table, and section pointer array
 */
    _LoaderFreeFileMem(elffile->sections,elffile->secsize);
    xfree(elffile->saddr);
    _LoaderFreeFileMem(elffile->header,sizeof(Elf32_Ehdr));
/*
 * Free the ELFModuleRec
 */
    xfree(elffile);

    return;
}

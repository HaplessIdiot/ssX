/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/elfloader.c,v $ */




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
#include "elf.h"

#include "sym.h"
#include "loader.h"

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
	int	fd;
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
	unsigned char *bss;	/* Start address of the .bss section */
	int	bssndx;		/* index of the .bss section */
	int	bsssize;	/* size of the .bss section */
	unsigned char *rodata;	/* Start address of the .rodata section */
	int	rodatndx;	/* index of the .rodata section */
	int	rodatsize;	/* size of the .rodata section */
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
 * If an relocation is unable to be satisfied, then put it on a list
 * to try later after more odules have been loaded.
 */
typedef struct _elf_reloc {
#if defined(i386)
	Elf32_Rel	*rel;
#endif
#if defined(__powerpc__)
	Elf32_Rela	*rel;
#endif
	ELFModulePtr	file;
	char		*secp;
	struct _elf_reloc	*next;
	} elf_reloc ;

static	elf_reloc *listResolve = NULL;

/*
 * symbols with a st_shndx of COMMON need to have space allocated for them.
 *
 * Gather all of these symbols together, and allocate one chunk when we
 * are done.
 */
typedef struct _elf_COMMON {
	Elf32_Sym	*sym;
	struct _elf_COMMON	*next;
	} elf_COMMON ;

static	elf_COMMON *listCOMMON = NULL;

/*
 * Utility Functions
 */


#ifdef HANDLE_IN_HASH_ENTRY
static int
  ELFhashCleanOut( module, item )
ELFModulePtr module ;
itemPtr item ;
{
  return ( module->handle == item->handle ) ;
}
#endif


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
 * Manage listResolv
 */
static void
ElfDelayRelocation(elffile,secp,rel)
ELFModulePtr	elffile;
char		*secp;
#if defined(i386)
Elf32_Rel	*rel;
#endif
#if defined(__powerpc__)
Elf32_Rela	*rel;
#endif
{
elf_reloc	*reloc;

if( (reloc=(elf_reloc *)malloc(sizeof(elf_reloc))) == NULL ) {
	ErrorF( "ElfDelayRelocation() Unable to allocate memory!!!!\n" );
	return;
	}
reloc->file=elffile;
reloc->secp=secp;
reloc->rel=rel;
reloc->next=listResolve;
listResolve=reloc;

return;
}

/*
 * Manage listCOMMON
 */
static void
ElfAddCOMMON(sym)
Elf32_Sym	*sym;
{
elf_COMMON	*common;

if( (common=(elf_COMMON*)malloc(sizeof(elf_COMMON))) == NULL ) {
	ErrorF( "ElfAddCOMMON() Unable to allocate memory!!!!\n" );
	return;
	}
common->sym=sym;
common->next=listCOMMON;
listCOMMON=common;

return;
}

static void
ElfCreateCOMMON(elffile)
ELFModulePtr	elffile;
{
int	numsyms=0,size=0,l=0;
int	offset=0;
elf_COMMON	*common;
LOOKUP		*lookup;

if( listCOMMON == NULL )
	return;

common=listCOMMON;
while(common) {
	size+=common->sym->st_size;
	numsyms++;
	common=common->next;
	}

#ifdef ELFDEBUG
ELFDEBUG("ElfCreateCOMMON() %d entries (%d bytes) of COMMON data\n",
				numsyms, size );
#endif

if( (lookup=malloc((numsyms+1)*sizeof(LOOKUP))) == NULL ) {
	ErrorF( "ElfCreateCOMMON() Unable to allocate memory!!!!\n" );
	return;
	}

elffile->comsize=size;
if( (elffile->common=(unsigned char *)calloc(1,size)) == NULL ) {
	ErrorF( "ElfCreateCOMMON() Unable to allocate memory!!!!\n" );
	return;
	}

while(listCOMMON) {
	common=listCOMMON;
	lookup[l].symName=(char *)strdup(ElfGetString(elffile,common->sym->st_name));
	lookup[l].offset=(void (*)())(elffile->common+offset);
#ifdef ELFDEBUG
	ELFDEBUG("Adding %x %s\n", lookup[l].offset, lookup[l].symName );
#endif
	listCOMMON=common->next;
	offset+=common->sym->st_size;
	free(common);
	l++;
	}

lookup[l].symName=NULL; /* Terminate the list */
LoaderAddSymbols(elffile->handle,lookup);
free(lookup);

return;
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
ELFDEBUG("%x %x ",ELF32_ST_BIND(syms[index].st_info),ELF32_ST_TYPE(syms[index].st_info));
ELFDEBUG("%x\n",syms[index].st_value);
#endif

return ElfGetString(elffile,syms[index].st_name );
}

static char *
ElfGetSymbolName(elffile, index)
ELFModulePtr	elffile;
int index;
{
return ElfGetSymbolNameIndex( elffile, index, elffile->symndx );
}

static Elf32_Addr
ElfGetSymbolValue(elffile, index)
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
			case STB_LOCAL:
				symval=(Elf32_Addr)(
					elffile->saddr[syms[index].st_shndx]+
					syms[index].st_value);
				break;
			case STB_GLOBAL:
				symname=
				    ElfGetString(elffile,syms[index].st_name);
				symbol=LoaderHashFind( symname );
				if( symbol == 0 )
					return 0;
				symval=(Elf32_Addr)symbol->address;
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
ELFDEBUG( "%x\t", symval );
ELFDEBUG( "%s\n", symname );
#endif
		break;
	case STT_SECTION:
		symval=(Elf32_Addr)elffile->saddr[syms[index].st_shndx];
#ifdef ELFDEBUG
ELFDEBUG( "ST_SECTION %x\n", symval );
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
return symval;
}

/*
 * This function is similar to ElfGetSymbolValue, but returns an address that
 * can be used to emulate a GOT entry location.
 */
static Elf32_Addr
ElfGetSymbolAddr(elffile, index)
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
				symbol=LoaderHashFind( symname );
				if( symbol == 0 )
					return 0;
				symval=(Elf32_Addr)&symbol->address;
				break;
			default:
				symval=0;
				ErrorF(
			     "ElfGetSymbolAddr(), unhandled symbol scope %x\n",
					ELF32_ST_BIND(syms[index].st_info) );
				break;
			}
#ifdef ELFDEBUG
ELFDEBUG( "%x\t", symbol );
ELFDEBUG( "%x\t", symval );
ELFDEBUG( "%s\n", symname );
#endif
		break;
	case STT_SECTION:
	case STT_FILE:
	case STT_LOPROC:
	case STT_HIPROC:
	default:
		symval=0;
		ErrorF( "ElfGetSymbolAddr(), unhandled symbol type %x\n",
				ELF32_ST_TYPE(syms[index].st_info) );
		break;
	}
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
				symbol=LoaderHashFind( symname );
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

				symbol->plt[0]=0x3d80; /* lis     r12 */
				symbol->plt[1]=(((Elf32_Addr)symbol->address)&0xffff0000)>>16;
				symbol->plt[2]=0x618c; /* ori     r12,r12 */
				symbol->plt[3]=(((Elf32_Addr)symbol->address)&0xffff);
				symbol->plt[4]=0x7d89; /* mtcr    r12 */
				symbol->plt[5]=0x03a6;
				symbol->plt[6]=0x4e80; /* bctr */
				symbol->plt[7]=0x0420;
				symbol->address=(char *)&symbol->plt[0];
				symval=(Elf32_Addr)symbol->address;
				break;
			default:
				symval=0;
				ErrorF(
			     "ElfGetPltAddr(), unhandled symbol scope %x\n",
					ELF32_ST_BIND(syms[index].st_info) );
				break;
			}
#ifdef ELFDEBUG
ELFDEBUG( "%x\t", symbol );
ELFDEBUG( "%x\t", symval );
ELFDEBUG( "%s\n", symname );
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
return symval;
}
#endif /* __powerpc__ */

/*
 * Fix all of the relocations for the given section.
 */
static void
Elf_RelocateEntry(elffile,secp,rel)
ELFModulePtr	elffile;
unsigned char *secp;	/* Begining of the target section */
#if defined(i386)
Elf32_Rel	*rel;
#endif
#if defined(__powerpc__)
Elf32_Rela	*rel;
#endif
{
unsigned long *dest32;	/* address of the 32 bit place being modified */
#if defined(__powerpc__)
unsigned short *dest16;	/* address of the 16 bit place being modified */
#endif
Elf32_Addr symval;	/* value of the indicated symbol */

#ifdef ELFDEBUG
#if defined(i386)
	ELFDEBUG( "%x %d %d\n", rel->r_offset,
		ELF32_R_SYM(rel->r_info),ELF32_R_TYPE(rel->r_info) );
#endif
#if defined(__powerpc__)
	ELFDEBUG( "%x %d %d %x\n", rel->r_offset,
		ELF32_R_SYM(rel->r_info),ELF32_R_TYPE(rel->r_info),
		rel->r_addend );
#endif
#endif

switch( ELF32_R_TYPE(rel->r_info) )
	{
#if defined(i386)
	case R_386_32:
		dest32=(unsigned long *)(secp+rel->r_offset);
		symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
		if( symval == 0 ) {
#ifdef ELFDEBUG
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			   ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
			}
#ifdef ELFDEBUG
ELFDEBUG( "R_386_32\t", dest32 );
ELFDEBUG( "dest32=%x\t", dest32 );
ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		*dest32=symval+(*dest32); /* S + A */
#ifdef ELFDEBUG
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
		break;
	case R_386_PC32:
		dest32=(unsigned long *)(secp+rel->r_offset);
		symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
		if( symval == 0 ) {
#ifdef ELFDEBUG
			ELFDEBUG( "***Unable to resolve symbol %s\n", ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
			}

#ifdef ELFDEBUG
ELFDEBUG( "R_386_PC32 %s\t", ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
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
#endif /* i386 */
#if defined(__powerpc__)
	case R_PPC_DISP24: /* 11 */
		dest32=(unsigned long *)(secp+rel->r_offset);
		symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
		if( symval == 0 ) {
#ifdef ELFDEBUG
			ELFDEBUG( "***Unable to resolve symbol %s\n", ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
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

		if( (val & 0xff000000) != 0xff000000 &&
		    (val & 0xff000000) != 0x00000000) {
#ifdef ELFDEBUG
			ELFDEBUG("R_PPC_DISP24 offset %x too large\n", val );
#endif
			symval = ElfGetPltAddr(elffile,ELF32_R_SYM(rel->r_info));
			val=((symval+(rel->r_addend)-(Elf32_Addr)dest32));
#ifdef ELFDEBUG
			ELFDEBUG("New value is %x\n",val );
#endif
			}

		val=val>>2;
		val=val&0x00ffffff;
		if( val > 0x00ffffff ) {
			ErrorF("R_PPC_DISP24 offset too large\n" );
			}
			
		(*dest32)|=(val<<2);
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
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			   ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
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
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			   ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
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
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			   ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
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
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			   ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
			}
#ifdef ELFDEBUG
ELFDEBUG( "R_PPC_16H\t" );
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
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			   ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
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
#if 0
/*
 * These relocation types are used only for loading -KPIC code. These are
 * not used by MetroX.
 */
	case R_PPC_ABDIFF_16HU:
		dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
		dest32=(unsigned long *)(dest16-1);
#endif
		symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
		if( symval == 0 ) {
#ifdef ELFDEBUG
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			   ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
			}
#ifdef ELFDEBUG
ELFDEBUG( "R_PPC_ABDIFF_16HU\t" );
ELFDEBUG( "secp=%x\t", secp );
ELFDEBUG( "symval=%x\t", symval );
ELFDEBUG( "dest16=%x\t", dest16 );
ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
ELFDEBUG( "dest32=%8.8x\t", dest32 );
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
		{
		unsigned short val;
		/* BA - S + A */
		val=(((unsigned long)secp-symval+(rel->r_addend))&0xffff0000)>>16;
		ErrorF("uhi16(BA-S+A)=%x\t",val);
		*dest16=val; /* BA - S + A */
		}
#ifdef ELFDEBUG
ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
		break;
	case R_PPC_ABDIFF_16L:
		dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
		dest32=(unsigned long *)(dest16-1);
#endif
		symval=ElfGetSymbolValue(elffile,ELF32_R_SYM(rel->r_info));
		if( symval == 0 ) {
#ifdef ELFDEBUG
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			   ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
			}
#ifdef ELFDEBUG
ELFDEBUG( "R_PPC_ABDIFF_16L\t" );
ELFDEBUG( "secp=%x\t", secp );
ELFDEBUG( "symval=%x\t", symval );
ELFDEBUG( "dest16=%x\t", dest16 );
ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
ELFDEBUG( "dest32=%8.8x\t", dest32 );
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
		{
		unsigned short val;
		/* BA - S + A */
		val=((unsigned long)secp-symval+(rel->r_addend))&0xffff;
		ErrorF("lo16(BA-S+A)=%x\t",val);
		*dest16=val; /* BA - S + A */
		}
#ifdef ELFDEBUG
ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
		break;
	case R_PPC_GOT_ABREL_16H:
		dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
		dest32=(unsigned long *)(dest16-1);
#endif
		symval=ElfGetSymbolAddr(elffile,ELF32_R_SYM(rel->r_info));
		if( symval == 0 ) {
#ifdef ELFDEBUG
			ELFDEBUG( "***Unable to resolve symbol %s\n",
			   ElfGetSymbolName(elffile,ELF32_R_SYM(rel->r_info)) );
#endif
			ElfDelayRelocation(elffile,secp,rel);
			break;
			}
#ifdef ELFDEBUG
ELFDEBUG( "R_PPC_GOT_ABREL_16H\t" );
ELFDEBUG( "secp=%x\t", secp );
ELFDEBUG( "symval=%x\t", symval );
ELFDEBUG( "dest16=%x\t", dest16 );
ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
ELFDEBUG( "dest32=%8.8x\t", dest32 );
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
		{
		unsigned short val;
		/* G + A - AB */
		val=(symval+(rel->r_addend)-(unsigned long)secp)&0xffff;
		ErrorF("lo16(G+A-AB)=%x\t",val);
		*dest16=val; /* G + A - AB */
		}
#ifdef ELFDEBUG
ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
		break;
#endif /* 0 */
#endif /* __powerpc__ */
	default:
		ErrorF(
		 "Elf_RelocateEntry() Unsupported relocation type %d\n",
				ELF32_R_TYPE(rel->r_info) );
		break;
	}
}

static void
ELF_DoRelocations(elffile, index)
ELFModulePtr	elffile;
int	index; /* The section to use as relocation data */
{
int	i, numrel;
Elf32_Shdr	*sect=&(elffile->sections[index]);
#if defined(i386)
Elf32_Rel	*rel=(Elf32_Rel *)elffile->saddr[index];
#endif
#if defined(__powerpc__)
Elf32_Rela	*rel=(Elf32_Rela *)elffile->saddr[index];
#endif
unsigned char *secp;	/* Begining of the target section */

secp=(unsigned char *)elffile->saddr[sect->sh_info];

numrel=sect->sh_size/sect->sh_entsize;

for(i=0; i<numrel; i++ ) {
	ElfDelayRelocation(elffile,secp,&(rel[i]));
	}

return;
}

/*
 * ELF_GetSymbols()
 *
 * add the symbols to the symbol table maintained by the loader.
 */

static void
ELF_GetSymbols(elffile)
ELFModulePtr	elffile;
{
Elf32_Sym	*syms;
Elf32_Shdr	*sect;
int		i, l, numsyms;
LOOKUP		*lookup;

syms=elffile->symtab;
sect=&(elffile->sections[elffile->symndx]);
numsyms=sect->sh_size/sect->sh_entsize;

if( (lookup=malloc((numsyms+1)*sizeof(LOOKUP))) == NULL )
	return;

for(i=0,l=0; i<numsyms; i++)
	{
#ifdef ELFDEBUG
	ELFDEBUG("value=%x\tsize=%x\tBIND=%x\tTYPE=%x\tndx=%x\t%s\n",
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
			if( !LoaderHashFind(
					ElfGetString(elffile,syms[i].st_name)))
				ElfAddCOMMON(&(syms[i]));
			break;
		    case SHN_UNDEF:
			/*
			 * UNDEF will get resolved later, so the value
			 * doesn't really matter here.
			 */
			/* since we don't know the value don't advertise the symbol */
			break;
		    default:
			lookup[l].symName=(char *)strdup(ElfGetString(elffile,syms[i].st_name));
			lookup[l].offset=(void (*)())(
				elffile->saddr[syms[i].st_shndx]+
				syms[i].st_value);
#ifdef ELFDEBUG
			ELFDEBUG("Adding %x %s\n",
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
LoaderAddSymbols(elffile->handle,lookup);
free(lookup);

ElfCreateCOMMON(elffile);

/*
 * Remove the ELF symbols that will show up in every object module.
 */
LoaderHashDelete(".text");
LoaderHashDelete(".data");
LoaderHashDelete(".bss");
LoaderHashDelete(".comment");
LoaderHashDelete(".note");
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
	/* .bss */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
							".bss" ) == 0 ) {
		if( SecSize(i) )
			elffile->bss=calloc(1,SecSize(i));
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
#if defined(i386)
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
#endif /* i386 */
#if defined(__powerpc__)
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
#endif /* __powerpc__ */
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
#if defined(__powerpc__)
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
ELFLoadModule(modtype,modname,handle,elffd)
int	modtype; /* Always LD_ELFOBJECT */
char	*modname;
int	handle;
int	elffd;
{
ELFModulePtr	elffile;
Elf32_Ehdr *header;

if( modtype != LD_ELFOBJECT ) {
	ErrorF( "ELFLoadModule(): modtype != ELF\n" );
	return NULL;
	}

if( (elffile=(ELFModulePtr)calloc(1,sizeof(ELFModuleRec))) == NULL ) {
	ErrorF( "Unable to allocate ELFModuleRec\n" );
	return NULL;
	}

elffile->handle=handle;
elffile->fd=elffd;

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
elffile->saddr=calloc(elffile->numsh,sizeof(unsigned char *));

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
ELF_GetSymbols(elffile);

/*
 * Do relocations
 */
if( elffile->reltxtndx )
	ELF_DoRelocations(elffile,elffile->reltxtndx);
if( elffile->reldatndx )
	ELF_DoRelocations(elffile,elffile->reldatndx);
if( elffile->relrodatndx )
	ELF_DoRelocations(elffile,elffile->relrodatndx);

return (void *)elffile;
}

void
ELFResolveSymbols()
{
	elf_reloc	*rel,*oldlist;

#ifdef ELFDEBUG
	ELFDEBUG("ELFResolvSymbols()\n" );
#endif

	/* avoid looping */
	oldlist=listResolve;
	listResolve=NULL;

	while( (rel=oldlist) != NULL )
	{
		oldlist=rel->next;
		Elf_RelocateEntry(rel->file,rel->secp,rel->rel);
		free(rel);
	}
}

int
ELFCheckForUnresolved()
{
	elf_reloc	*erel;
	char		**addr;
	extern int	LoaderDefaultFunc();

	if( (erel=listResolve) == NULL )
		return 0;

	while( erel )
	{
		addr = (char**)(erel->secp+erel->rel->r_offset);
		*addr += ((int) &LoaderDefaultFunc - (int)addr);

		if (xf86ShowUnresolved)
		{
			ErrorF("Unresolved Symbol %s from %s\n",
				ElfGetSymbolName(erel->file, 
					ELF32_R_SYM(erel->rel->r_info)),
				_LoaderHandleToName(erel->file->handle));
		}
		erel=erel->next;
	}
	return 1;
}

void
ELFUnloadModule( modptr )
void *modptr;
{
ELFModulePtr	elffile = (ELFModulePtr)modptr;
elf_reloc	*relptr, *reltptr, **brelptr;

/*
 * Delete any unresolved relocations
 */

relptr=listResolve;
brelptr=&listResolve;

while(relptr) {
	if( relptr->file == elffile ) {
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

LoaderHashTraverse( (void *)elffile, ELFhashCleanOut ) ;

/*
 * Free the sections that were allocated.
 */
#define CheckandFree(ptr,size)  if(ptr) _LoaderFreeFileMem(ptr,size);

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
	free(elffile->common);
/*
 * Free the section table, and section pointer array
 */
_LoaderFreeFileMem(elffile->sections,elffile->secsize);
free(elffile->saddr);
_LoaderFreeFileMem(elffile->header,sizeof(Elf32_Ehdr));
/*
 * Free the ELFModuleRec
 */
free(elffile);

return;
}

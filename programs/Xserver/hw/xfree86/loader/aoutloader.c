/* $XFree86$ */





/*
 *
 * Copyright (c) 1997 Matthieu Herrb
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
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "aout.h"
#include "sym.h"
#include "loader.h"


/* #define AOUTDEBUG ErrorF */


#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

/*
 * This structure contains all of the information about a module
 * that has been loaded.
 */

typedef struct {
    int     handle;
    int     fd;
    AOUTHDR      *header;/* file header */
    unsigned char *text;       /* Start address of the text section */
    unsigned char *data;       /* Start address of the data section */
    unsigned char *bss;        /* Start address of the bss data */
    struct relocation_info *txtrel;    /* Start address of the text relocation table */
    struct relocation_info *datarel;    /* Start address of the data relocation table */
    AOUT_nlist *symtab;        /* Start address of the symbol table */
    unsigned char *strings;    /* Start address of the string table */
    unsigned long strsize;     /* size of string table */
    unsigned char *common;	/* Start address of the common data */
    unsigned long comsize;	/* size of common data */
} AOUTModuleRec, *AOUTModulePtr;

/*
 * If an relocation is unable to be satisfied, then put it on a list
 * to try later after more odules have been loaded.
 */
typedef struct AOUT_RELOC {
    AOUTModulePtr file;
    struct relocation_info *rel;
    int type;			/* AOUT_TEXT or AOUT_DATA */
    struct AOUT_RELOC *next;
} AOUT_RELOC ;

static AOUT_RELOC *listResolve = NULL;

/*
 * Symbols with a section number of 0 (N_UNDF) but a value of non-zero
 * need to have space allocated for them.
 *
 * Gather all of these symbols together, and allocate one chunk when we
 * are done.
 */

typedef struct AOUT_COMMON {
    struct AOUT_nlist *sym;
    int index;
    struct AOUT_COMMON *next;
} AOUT_COMMON;

static AOUT_COMMON *listCOMMON = NULL;

/*
 * Return 1 if the symbol in item belongs to aoutfile
 */
static int
AOUTHashCleanOut(AOUTModulePtr aoutfile, itemPtr item)
{
    return (aoutfile->handle == item->handle);
}

/*
 * Return the name of a symbol 
 */
static char *
AOUTGetSymbolName(AOUTModulePtr aoutfile, struct AOUT_nlist *sym)
{
    char *symname =  &(aoutfile->strings[sym->n_un.n_strx]);

    if (symname[0] == '_') {
	return symname + 1;
    } else {
	return symname;
    }
}

/*
 * Return the value of a symbol in the loader's symbol table
 */
unsigned long
AOUTGetSymbolValue(AOUTModulePtr aoutfile, int index)
{
    unsigned long symval = NULL; /* value of the indicated symbol */
    itemPtr symbol;		/* name/value of symbol */

    symbol = LoaderHashFind(AOUTGetSymbolName(aoutfile, aoutfile->symtab 
					      + index)) ;
    if (symbol)
	symval = (unsigned long)(symbol->address);

    return symval;
}

/*
 * Manage listCOMMON
 */

static void
AOUTAddCommon(struct AOUT_nlist *sym)
{
    AOUT_COMMON	*common;

    if ((common = (AOUT_COMMON*)malloc(sizeof(AOUT_COMMON))) == NULL) {
	ErrorF( "AOUTAddCommon() Unable to allocate memory\n" );
	return;
    }
    common->sym = sym;
    common->next = listCOMMON;
    listCOMMON = common;
}

static void
AOUTCreateCommon(AOUTModulePtr aoutfile)
{
    int	numsyms = 0, size = 0, l = 0;
    int	offset = 0;
    AOUT_COMMON *common;
    LOOKUP *lookup;

    if (listCOMMON == NULL)
	return;

    common = listCOMMON;
    while (common) {
	/* Ensure long word alignment */
	if( common->sym->n_value%4 != 0 )
	    common->sym->n_value+= 4-(common->sym->n_value%4);

	/* accumulate the sizes */
	size += common->sym->n_value;
	numsyms++;
	common = common->next;
    } /* while */

#ifdef AOUTDEBUG
    AOUTDEBUG("AOUTCreateCommon() %d entries (%d bytes) of COMMON data\n",
	      numsyms, size );
#endif
    
    if ((lookup = malloc((numsyms+1)*sizeof(LOOKUP))) == NULL) {
        ErrorF( "AOUTCreateCommon() Unable to allocate memory\n" );
        return;
    }
    
    aoutfile->comsize = size;
    if ((aoutfile->common = (unsigned char *)calloc(1,size)) == NULL) {
        ErrorF( "AOUTCreateCommon() Unable to allocate memory\n" );
        return;
    }
    
    while (listCOMMON) {
        common = listCOMMON;
        lookup[l].symName= AOUTGetSymbolName(aoutfile, common->sym);
        lookup[l].offset = (void (*)())(aoutfile->common+offset);
#ifdef AOUTDEBUG
        AOUTDEBUG("Adding %x %s\n", lookup[l].offset, lookup[l].symName );
#endif
        listCOMMON = common->next;
        offset += common->sym->n_value;
        free(common);
        l++;
    } /* while */

    lookup[l].symName=NULL; /* Terminate the list */
    LoaderAddSymbols(aoutfile->handle, lookup);
    free(lookup);

}

/*
 * AOUT_GetSymbols()
 * 
 * add the symbols to the loader's symbol table
 */
static void 
AOUT_GetSymbols(AOUTModulePtr aoutfile)
{
    int fd = aoutfile->fd;
    AOUTHDR *header = aoutfile->header;
    int nsyms, soff, i, l;
    char *symname;
    AOUT_nlist *s;
    LOOKUP *lookup;

    aoutfile->symtab = (AOUT_nlist *)_LoaderFileToMem(fd,
						      AOUT_SYMOFF(header),
						      header->a_syms,
						      "symbols");
    nsyms = header->a_syms/sizeof(AOUT_nlist);
    lookup = (LOOKUP *)malloc(nsyms * sizeof(LOOKUP));
    if (lookup == NULL) {
	ErrorF("AOUT_GetSymbols(): can't allocate memory\n");
	return;
    }
    for (i = 0, l = 0; i < nsyms; i++) {
	s = aoutfile->symtab + i;
	soff = s->n_un.n_strx;

	symname = &(aoutfile->strings[soff]);
	/* strip leading underscore */
	if (symname[0] == '_') {
	    symname++;
	}
#ifdef AOUTDEBUG
	AOUTDEBUG("AOUT_GetSymbols(): %s %02x %02x %08x\n",
		  symname, s->n_type,
		  s->n_other, s->n_value);
#endif
	if (soff == 0 || (s->n_type & AOUT_STAB) != 0) 
	    continue;
	switch (s->n_type & AOUT_TYPE) {
	  case AOUT_UNDF:
	    if (s->n_value != 0) {
		if (!LoaderHashFind(symname)) {
#ifdef AOUTDEBUG
		    AOUTDEBUG("Adding common %s\n", symname);
#endif
		    AOUTAddCommon(s);
		}
	    } else {
#ifdef AOUTDEBUG
		AOUTDEBUG("Adding undef %s\n", symname);
#endif
	    }
	    break;
	  case AOUT_TEXT:
	    if (s->n_type & AOUT_EXT) {
		lookup[l].symName = symname;
		/* text symbols start at 0 */
		lookup[l].offset = (void(*)())(aoutfile->text + s->n_value);
#ifdef AOUTDEBUG
		AOUTDEBUG("Adding text %s %08x\n", symname, lookup[l].offset);
#endif
		l++;
	    }
	    break;
	  case AOUT_DATA :
	    if (s->n_type & AOUT_EXT) {
		lookup[l].symName = symname;
		/* data symbols are following text */
		lookup[l].offset = (void(*)())(aoutfile->data + 
					       s->n_value - header->a_text);
#ifdef AOUTDEBUG
		AOUTDEBUG("Adding data %s %08x\n", symname, lookup[l].offset);
#endif
		l++;
	    }
	    break;
	  case AOUT_BSS:
	    if (s->n_type & AOUT_EXT) {
		lookup[l].symName = symname;
		/* bss symbols follow both text and data */
		lookup[l].offset = (void(*)())(aoutfile->bss + s->n_value 
					       - (header->a_data 
						  + header->a_text));
#ifdef AOUTDEBUG
		AOUTDEBUG("Adding bss %s %08x\n", symname, lookup[l].offset);
#endif
		l++;
	    }
	    break;
	} /* switch */
    } /* for */
    lookup[l].symName = NULL;
    LoaderAddSymbols(aoutfile->handle, lookup);
    free(lookup);
    
    AOUTCreateCommon(aoutfile);
    
} /* AOUT_GetSymbols */

/*
 * Manage listResolv 
 */
static void
AOUTDelayRelocation(AOUTModulePtr aoutfile, int type, 
			struct relocation_info *rel)
{
    AOUT_RELOC *reloc;

    if ((reloc = (AOUT_RELOC *) malloc(sizeof(AOUT_RELOC))) == NULL) {
	ErrorF("AOUTDelayRelocation() Unable to allocate memory\n");
	return;
    }
    if ((unsigned long)rel < 0x200) {
	ErrorF("bug");
    }
    reloc->file = aoutfile;
    reloc->type = type;
    reloc->rel = rel;
    reloc->next = listResolve;
    listResolve = reloc;
}
	

static void
AOUT_DoRelocations(AOUTModulePtr aoutfile)
{
    AOUTHDR *header = aoutfile->header;
    int i, nreloc;
    struct relocation_info *rel;

    /* Text relocations */
    if (aoutfile->text != NULL && aoutfile->txtrel != NULL) {
	nreloc = header->a_trsize/sizeof(struct relocation_info);

	for (i = 0; i < nreloc; i++) {
	    rel = aoutfile->txtrel + i;
	    AOUTDelayRelocation(aoutfile, AOUT_TEXT, rel);
	} /* for */
    }
    /* Data relocations */
    if (aoutfile->data != NULL && aoutfile->datarel != NULL) {
	nreloc = header->a_drsize/sizeof(struct relocation_info);
    
	for (i = 0; i < nreloc; i++) {
	    rel = aoutfile->datarel + i;
	    AOUTDelayRelocation(aoutfile, AOUT_DATA, rel);
	} /* for */
    }
} /* AOUT_DoRelocations */

/*
 * Perform the actual relocation 
 */
static void
AOUT_Relocate(unsigned long *destl, unsigned long val, int type)
{
#ifdef AOUTDEBUG
    AOUTDEBUG("AOUT_Relocate %08x -> %08x %s\n",
	      *destl, val, type == 1 ? "rel" : "abs");
#endif
    if (type == 1) {
	/* relative to PC */
	*destl -= val;
    } else {
	/* absolute */
	*destl = val;
    }
}


/*
 * Fix the relocation for text or data section
 */
static void
AOUT_RelocateEntry(AOUTModulePtr aoutfile, int type, 
		   struct relocation_info *rel) 
{
    AOUTHDR *header = aoutfile->header;
    AOUT_nlist *symtab = aoutfile->symtab;
    int symnum;
    unsigned long symval;
    unsigned long *destl;	/* address of the location to be modified */

    symnum = rel->r_symbolnum;
#ifdef AOUTDEBUG 
    if (rel->r_extern) {
	AOUTDEBUG("AOUT_RelocateEntry: extern %s\n", 
		  AOUTGetSymbolName(aoutfile, aoutfile->symtab+symnum));
    } else {
	AOUTDEBUG("AOUT_RelocateEntry: intern\n");
    }
    AOUTDEBUG("  pcrel: %d", rel->r_pcrel);
    AOUTDEBUG("  length: %d", rel->r_length);
    AOUTDEBUG("  baserel: %d", rel->r_baserel);
    AOUTDEBUG("  jmptable: %d", rel->r_jmptable);
    AOUTDEBUG("  relative: %d", rel->r_relative);
    AOUTDEBUG("  copy: %d\n", rel->r_copy);    
#endif /* AOUTDEBUG */

    if (rel->r_length != 2) {
	ErrorF("AOUT_ReloateEntry: length != 2\n");
    }
    /* 
     * First find the address to modify
     */
    switch (type) {
      case AOUT_TEXT:
	/* Check that the relocation offset is in the text segment */
	if (rel->r_address > header->a_text) {
	    ErrorF("AOUT_RelocateEntry(): "
		   "text relocation out of text section\n");
	} 
	destl = (unsigned long *)(aoutfile->text + rel->r_address);
	break;
      case AOUT_DATA:
	/* Check that the relocation offset is in the data segment */
	if (rel->r_address > header->a_data) {
	    ErrorF("AOUT_RelocateEntry():"
		   "data relocation out of data section\n");
	}
	destl = (unsigned long *)(aoutfile->data + rel->r_address);
	break;
      default:
	ErrorF("AOUT_RelocateEntry(): unknown section type %d\n", type);
	return;
    } /* switch */

    /*
     * Now handle the relocation 
     */
    if (rel->r_extern) {
	/* Lookup the symbol in the loader's symbol table */
	symval = AOUTGetSymbolValue(aoutfile, symnum);
	if (symval != 0) {
	    /* we've got the value */
	    AOUT_Relocate(destl, symval, rel->r_pcrel);
	} else {
	    /* The symbol should be undefined */
	    switch (symtab[symnum].n_type & AOUT_TYPE) {
	      case AOUT_UNDF:
#ifdef AOUTDEBUG
		AOUTDEBUG("  extern AOUT_UNDEF\n");
#endif
		/* Add this relocation back to the global list */
		AOUTDelayRelocation(aoutfile,type,rel);
		return;

	      default:
		ErrorF("AOUT_RelocateEntry():"
		       " impossible intern relocation type: %d\n", 
		       symtab[symnum].n_type);
		return;
	    } /* switch */
	}
    } else {
	/* intern */
	switch (rel->r_symbolnum) {
	  case AOUT_TEXT:
#ifdef AOUTDEBUG
	    AOUTDEBUG("  AOUT_TEXT\n");
#endif
	    AOUT_Relocate(destl, (unsigned long)aoutfile->text+*destl, 
			  rel->r_pcrel);
	    return;
	  case AOUT_DATA:
#ifdef AOUTDEBUG
	    AOUTDEBUG("  AOUT_DATA\n");
#endif
	    AOUT_Relocate(destl, (unsigned long)aoutfile->data
			   + *destl - header->a_text, 
			  rel->r_pcrel);
	    return;
	  case AOUT_BSS:
#ifdef AOUTDEBUG
	    AOUTDEBUG("  AOUT_BSS\n");
#endif
	    AOUT_Relocate(destl, (unsigned long)aoutfile->bss
			  + *destl - header->a_text - header->a_data, 
			  rel->r_pcrel);
	    return;
	  default:
	    ErrorF("AOUT_RelocateEntry():"
		   " unknown intern relocation type: %d\n", rel->r_symbolnum);
	    return;
	} /* switch */
    }
	
} /* AOUT_RelocateEntry */
    
/*
 * Public API for the a.out implementation of the loader
 */
void *
AOUTLoadModule(int modtype,
	       char *modname,
	       int handle,
	       int aoutfd)
{
    AOUTModulePtr aoutfile = NULL;
    AOUTHDR *header;

#ifdef AOUTDEBUG
    AOUTDEBUG("AOUTLoadModule(%d, %s, %d, %d)\n",
	      modtype, modname, handle, aoutfd);
#endif
    if (modtype != LD_AOUTOBJECT) {
	ErrorF( "AOUTLoadModule(): modtype != a.out\n" );
	return NULL;
    }
    if ((aoutfile=(AOUTModulePtr)calloc(1,sizeof(AOUTModuleRec))) == NULL ) {
	ErrorF( "Unable to allocate AOUTModuleRec\n" );
	return NULL;
    }

    aoutfile->handle=handle;
    aoutfile->fd=aoutfd;

    /*
     *  Get the a.out header
     */
    aoutfile->header=(AOUTHDR *)_LoaderFileToMem(aoutfd,0,sizeof(AOUTHDR),
						 "header");
    header= (AOUTHDR *)aoutfile->header;

    /* 
     * Load the 6 other sections 
     */
    /* text */
    if (header->a_text != 0) {
	aoutfile->text = _LoaderFileToMem(aoutfile->fd, 
					  AOUT_TXTOFF(header),
					  header->a_text, "text");
    } else {
	aoutfile->text = NULL;
    }
    /* data */
    if (header->a_data != 0) {
	aoutfile->data = _LoaderFileToMem(aoutfile->fd,
					  AOUT_DATOFF(header),
					  header->a_data, "data");
    } else {
	aoutfile->data = NULL;
    }
    /* bss */
    if (header->a_bss != 0) {
	aoutfile->bss = calloc(1, header->a_data);
    } else {
	aoutfile->bss = NULL;
    }
    /* Text Relocations */
    if (header->a_trsize != 0) {
	aoutfile->txtrel = _LoaderFileToMem(aoutfile->fd,
					    AOUT_TRELOFF(header),
					    header->a_trsize, "txtrel");
    } else {
	aoutfile->txtrel = NULL;
    }
    /* Data Relocations */
    if (header->a_drsize != 0) {
	aoutfile->datarel = _LoaderFileToMem(aoutfile->fd,
					    AOUT_DRELOFF(header),
					    header->a_drsize, "datarel");
    } else {
	aoutfile->datarel = NULL;
    }
    /* String table */
    _LoaderFileRead(aoutfile->fd, AOUT_STROFF(header), 
		    &(aoutfile->strsize), sizeof(int));
    if (aoutfile->strsize != 0) {
	aoutfile->strings = _LoaderFileToMem(aoutfile->fd,
					     AOUT_STROFF(header),
					     aoutfile->strsize, "strings");
    } else {
	aoutfile->strings = NULL;
    }
    /* load symbol table */
    AOUT_GetSymbols(aoutfile);

    /* Do relocations */
    AOUT_DoRelocations(aoutfile);

    return (void *)aoutfile;
}

void
AOUTResolveSymbols(void)
{
    AOUT_RELOC *rel, *oldlist;

#ifdef AOUTDEBUG
    AOUTDEBUG("AOUTResolveSymbols()\n");
#endif
    oldlist = listResolve;
    listResolve = NULL;

    while ((rel = oldlist) != NULL) {
	oldlist = rel->next;
	AOUT_RelocateEntry(rel->file, rel->type, rel->rel);
	free(rel);
    } /* while */

} /* AOUTResolveSymbols */

int
AOUTCheckForUnresolved(void)
{
    int symnum;
    AOUT_RELOC *crel = listResolve;

#ifdef AOUTDEBUG
    AOUTDEBUG("AOUTCheckForUnResolved()\n");
#endif
    if (crel == NULL) {
	return 0;
    }
    while (crel) {
	symnum = crel->rel->r_symbolnum;
	ErrorF("Unresolved Symbol %s from %s\n",
	       AOUTGetSymbolName(crel->file, crel->file->symtab + symnum),
	       _LoaderHandleToName(crel->file->handle));
	crel = crel->next;
    }
    return 1;
}

void
AOUTUnloadModule(void *modptr)
{
    AOUTModulePtr aoutfile = (AOUTModulePtr)modptr;
    AOUTHDR *header = aoutfile->header;
    AOUT_RELOC *relptr, **prevptr;
#ifdef AOUTDEBUG
    AOUTDEBUG("AOUTUnLoadModule(0x%p)\n", modptr);
#endif
    /* Free pending relocations */
    prevptr = &listResolve;
    relptr = listResolve;
    while (relptr) {
	if (relptr->file == aoutfile) {
	    *prevptr = relptr->next;
	    free(relptr);
	    relptr = *prevptr;
	} else {
	    prevptr = &(relptr->next);
	    relptr = relptr->next;
	}
    } /* while */
	    
    /* clean the symbols table */
    LoaderHashTraverse((void *)aoutfile, AOUTHashCleanOut);

    if (aoutfile->strings != NULL) {
	_LoaderFreeFileMem(aoutfile->strings, aoutfile->strsize);
    }
    if (aoutfile->symtab != NULL) {
	_LoaderFreeFileMem(aoutfile->symtab, header->a_syms);
    }
    /* Free relocations */
    if (aoutfile->datarel != NULL) {
	_LoaderFreeFileMem(aoutfile->datarel, header->a_drsize);
    }
    if (aoutfile->txtrel != NULL) {
	_LoaderFreeFileMem(aoutfile->txtrel, header->a_trsize);
    }
    /* Free symbol table */
    /* Free allocated sections */
    if (aoutfile->bss != NULL) {
	free(aoutfile->bss);
    }
    if (aoutfile->data != NULL) {
	_LoaderFreeFileMem(aoutfile->data, header->a_data);
    }
    if (aoutfile->text != NULL) {
	_LoaderFreeFileMem(aoutfile->text, header->a_text);
    }
    /* Free header */
    _LoaderFreeFileMem(aoutfile->header, sizeof(AOUTHDR));
    /* Free the module structure itself */
    free(aoutfile);
}

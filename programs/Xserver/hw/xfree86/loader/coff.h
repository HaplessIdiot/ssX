/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/coff.h,v 1.2 1997/02/16 10:27:20 hohndel Exp $ */





/* This file was implemented from the information in the book
   Understanding and Using COFF
   Gintaras R. Gircys
   O'Reilly, 1988
   and by looking at the Linux kernel code.

   It is therefore most likely free to use...

   If the file format changes in the COFF object, this file should be
   subsequently updated to reflect the changes.

   The actual loader module only uses a few of the COFF structures. 
   Only those are included here.  If you wish more information about 
   COFF, thein check out the book mentioned above.
*/

#define  E_SYMNMLEN  8   /* Number of characters in a symbol name         */
/*
 * Intel 386/486  
 */

/*
 * FILE HEADER 
 */

typedef struct COFF_filehdr 
{
	unsigned short	  f_magic;	/* magic number			*/
	unsigned short	  f_nscns;	/* number of sections		*/
	long		  f_timdat;	/* time & date stamp		*/
	long		  f_symptr;	/* file pointer to symtab	*/
	long		  f_nsyms;	/* number of symtab entries	*/
	unsigned short	  f_opthdr;	/* sizeof(optional hdr)		*/
	unsigned short	  f_flags;	/* flags			*/
} FILHDR;

#define	FILHSZ	sizeof(FILHDR)

/*
 * SECTION HEADER 
 */

typedef struct COFF_scnhdr 
{
	char		  s_name[8];	/* section name			*/
	long		  s_paddr;	/* physical address		*/
	long		  s_vaddr;	/* virtual address		*/
	long		  s_size;	/* section size			*/
	long		  s_scnptr;	/* raw data for section		*/
	long		  s_relptr;	/* relocation			*/
	long		  s_lnnoptr;	/* line numbers	   		*/
	unsigned short	  s_nreloc;	/* number of relocation entries */
	unsigned short	  s_nlnno;	/* number of line number entries*/
	long		  s_flags;	/* flags			*/
} SCNHDR;

#define	COFF_SCNHDR	struct COFF_scnhdr
#define	COFF_SCNHSZ	sizeof(COFF_SCNHDR)
#define SCNHSZ		COFF_SCNHSZ

/*
 * the optional COFF header as used by Linux COFF
 */

typedef struct 
{
  char	  magic[2];			/* type of file			 */
  char	  vstamp[2];			/* version stamp		 */
  char	  tsize[4];			/* text size in bytes 		 */
  char	  dsize[4];			/* initialized data		 */
  char	  bsize[4];			/* uninitialized data  		 */
  char	  entry[4];			/* entry point			 */
  char 	  text_start[4];		/* base of text 	  	 */
  char 	  data_start[4];		/* base of data 		 */
} AOUTHDR;


/*
 * SYMBOLS 
 */

typedef struct COFF_syment 
{
	union 
	{
	char	  _n_name[E_SYMNMLEN];	/* Symbol name (first 8 chars)	*/
	struct 
	{
		long	  _n_zeroes;	 /* Leading zeros		*/
		long	  _n_offset;	 /* Offset for a header section  */
	} _n_n;
	char	*_n_nptr[2];		 /* allows for overlaying       */
	} _n;

	long		  n_value;	/* address of the segment	*/
	short		  n_scnum;	/* Section number		*/
	unsigned short	  n_type;	/* Type of section		*/
	char		  n_sclass;	/* Loader class			*/
	char		  n_numaux;	/* Number of aux entries following */
} SYMENT;

#define n_name		_n._n_name
#define n_nptr		_n._n_nptr[1]
#define n_zeroes	_n._n_n._n_zeroes
#define n_offset	_n._n_n._n_offset

#define COFF_E_SYMNMLEN	 8	/* characters in a short symbol name	*/
#define COFF_E_FILNMLEN	14	/* characters in a file name		*/
#define COFF_E_DIMNUM	 4	/* array dimensions in aux entry        */
#define SYMNMLEN	COFF_E_SYMNMLEN
#define SYMESZ		18	/* not really sizeof(SYMENY) due to padding */

/* Special section number found in the symbol section */
#define	N_UNDEF	0
#define	N_ABS	-1
#define	N_DEBUG	-2

#define C_EXT	2

/*
 * RELOCATION DIRECTIVES
 */

typedef struct COFF_reloc 
{
	long	  r_vaddr;		/* Virtual address of item    */
	long	  r_symndx;		/* Symbol index in the symtab */
#if defined(__powerpc__)
	union
	{
	unsigned short	  _r_type;	/* old style coff relocation type */
	struct 
	{
		char	  _r_rsize;	/* sign and reloc bit len */
		char	  _r_rtype;	/* toc relocation type */
	} _r_r;
	} _r;
#define r_type  _r._r_type		/* old style reloc - original name */
#define r_rsize _r._r_r._r_rsize	/* extract sign and bit len    */
#define r_rtype _r._r_r._r_rtype	/* extract toc relocation type */
#else
	unsigned short	  r_type;	/* Relocation type             */
#endif
} RELOC;

#define COFF_RELOC	struct COFF_reloc
#define COFF_RELSZ	10
#define RELSZ		COFF_RELSZ

/*
 * x86 Relocation types 
 */
#define	R_ABS		000
#define	R_DIR32		006
#define	R_PCRLONG	024


/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/elf.h,v 1.1 1997/02/14 12:17:36 hohndel Exp $ */





typedef unsigned long	Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef unsigned long	Elf32_Off;
typedef long		Elf32_Sword;
typedef unsigned long	Elf32_Word;

/* These constants are for the segment types stored in the image headers */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7fffffff

/* These constants define the different elf file types */
#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOPROC 5
#define ET_HIPROC 6

/* These constants define the various ELF target machines */
#define EM_NONE  0
#define EM_M32   1
#define EM_SPARC 2
#define EM_386   3
#define EM_68K   4
#define EM_88K   5
#define EM_486   6   /* Perhaps disused */
#define EM_860   7

/* This is the info that is needed to parse the dynamic section of the file */
#define DT_NULL		0
#define DT_NEEDED	1
#define DT_PLTRELSZ	2
#define DT_PLTGOT	3
#define DT_HASH		4
#define DT_STRTAB	5
#define DT_SYMTAB	6
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT	9
#define DT_STRSZ	10
#define DT_SYMENT	11
#define DT_INIT		12
#define DT_FINI		13
#define DT_SONAME	14
#define DT_RPATH 	15
#define DT_SYMBOLIC	16
#define DT_REL	        17
#define DT_RELSZ	18
#define DT_RELENT	19
#define DT_PLTREL	20
#define DT_DEBUG	21
#define DT_TEXTREL	22
#define DT_JMPREL	23
#define DT_LOPROC	0x70000000
#define DT_HIPROC	0x7fffffff

/* This info is needed when parsing the symbol table */
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2

#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_LOPROC  13
#define STT_HIPROC  15

#define ELF32_ST_BIND(x) ((x) >> 4)
#define ELF32_ST_TYPE(x) (((unsigned int) x) & 0xf)



typedef struct dynamic{
  Elf32_Sword d_tag;
  union{
    Elf32_Sword	d_val;
    Elf32_Addr	d_ptr;
  } d_un;
} Elf32_Dyn;

extern Elf32_Dyn _DYNAMIC [];

/* The following are used with relocations */
#define ELF32_R_SYM(x) ((x) >> 8)
#define ELF32_R_TYPE(x) ((x) & 0xff)

/* x86 Relocation Types */
#define R_386_NONE	0
#define R_386_32	1
#define R_386_PC32	2
#define R_386_GOT32	3
#define R_386_PLT32	4
#define R_386_COPY	5
#define R_386_GLOB_DAT	6
#define R_386_JMP_SLOT	7
#define R_386_RELATIVE	8
#define R_386_GOTOFF	9
#define R_386_GOTPC	10
#define R_386_NUM	11

/* m68k Relocation Types */
#define R_68K_NONE	0		/* No reloc */
#define R_68K_32	1		/* Direct 32 bit  */
#define R_68K_16	2		/* Direct 16 bit  */
#define R_68K_8		3		/* Direct 8 bit  */
#define R_68K_PC32	4		/* PC relative 32 bit */
#define R_68K_PC16	5		/* PC relative 16 bit */
#define R_68K_PC8	6		/* PC relative 8 bit */
#define R_68K_GOT32	7		/* 32 bit PC relative GOT entry */
#define R_68K_GOT16	8		/* 16 bit PC relative GOT entry */
#define R_68K_GOT8	9		/* 8 bit PC relative GOT entry */
#define R_68K_GOT32O	10		/* 32 bit GOT offset */
#define R_68K_GOT16O	11		/* 16 bit GOT offset */
#define R_68K_GOT8O	12		/* 8 bit GOT offset */
#define R_68K_PLT32	13		/* 32 bit PC relative PLT address */
#define R_68K_PLT16	14		/* 16 bit PC relative PLT address */
#define R_68K_PLT8	15		/* 8 bit PC relative PLT address */
#define R_68K_PLT32O	16		/* 32 bit PLT offset */
#define R_68K_PLT16O	17		/* 16 bit PLT offset */
#define R_68K_PLT8O	18		/* 8 bit PLT offset */
#define R_68K_COPY	19		/* Copy symbol at runtime */
#define R_68K_GLOB_DAT	20		/* Create GOT entry */
#define R_68K_JMP_SLOT	21		/* Create PLT entry */
#define R_68K_RELATIVE	22		/* Adjust by program base */

/* Alpha Relocation Types */
#define R_ALPHA_NONE		0	/* No reloc */
#define R_ALPHA_REFLONG		1	/* Direct 32 bit */
#define R_ALPHA_REFQUAD		2	/* Direct 64 bit */
#define R_ALPHA_GPREL32		3	/* GP relative 32 bit */
#define R_ALPHA_LITERAL		4	/* GP relative 16 bit w/optimization */
#define R_ALPHA_LITUSE		5	/* Optimization hint for LITERAL */
#define R_ALPHA_GPDISP		6	/* Add displacement to GP */
#define R_ALPHA_BRADDR		7	/* PC+4 relative 23 bit shifted */
#define R_ALPHA_HINT		8	/* PC+4 relative 16 bit shifted */
#define R_ALPHA_SREL16		9	/* PC relative 16 bit */
#define R_ALPHA_SREL32		10	/* PC relative 32 bit */
#define R_ALPHA_SREL64		11	/* PC relative 64 bit */
#define R_ALPHA_OP_PUSH		12	/* OP stack push */
#define R_ALPHA_OP_STORE	13	/* OP stack pop and store */
#define R_ALPHA_OP_PSUB		14	/* OP stack subtract */
#define R_ALPHA_OP_PRSHIFT	15	/* OP stack right shift */
#define R_ALPHA_GPVALUE		16
#define R_ALPHA_GPRELHIGH	17
#define R_ALPHA_GPRELLOW	18
#define R_ALPHA_IMMED_GP_16	19
#define R_ALPHA_IMMED_GP_HI32	20
#define R_ALPHA_IMMED_SCN_HI32	21
#define R_ALPHA_IMMED_BR_HI32	22
#define R_ALPHA_IMMED_LO32	23
#define R_ALPHA_COPY		24	/* Copy symbol at runtime */
#define R_ALPHA_GLOB_DAT	25	/* Create GOT entry */
#define R_ALPHA_JMP_SLOT	26	/* Create PLT entry */
#define R_ALPHA_RELATIVE	27	/* Adjust by program base */

/* PPC Relocation Types */
#define R_PPC_NONE              0
#define R_PPC_COPY              1
#define R_PPC_GOTP_ENT          2
#define R_PPC_8                 4
#define R_PPC_8S                5
#define R_PPC_16S               7
#define R_PPC_14                8
#define R_PPC_DISP14            9
#define R_PPC_24                10
#define R_PPC_DISP24            11
#define R_PPC_PLT_DISP24        14
#define R_PPC_BBASED_16HU       15
#define R_PPC_BBASED_32         16
#define R_PPC_BBASED_32UA       17
#define R_PPC_BBASED_16H        18
#define R_PPC_BBASED_16L        19
#define R_PPC_ABDIFF_16HU       23
#define R_PPC_ABDIFF_32         24
#define R_PPC_ABDIFF_32UA       25
#define R_PPC_ABDIFF_16H        26
#define R_PPC_ABDIFF_16L        27
#define R_PPC_ABDIFF_16         28
#define R_PPC_16HU              31
#define R_PPC_32                32
#define R_PPC_32UA              33
#define R_PPC_16H               34
#define R_PPC_16L               35
#define R_PPC_16                36
#define R_PPC_GOT_16HU          39
#define R_PPC_GOT_32            40
#define R_PPC_GOT_32UA          41
#define R_PPC_GOT_16H           42
#define R_PPC_GOT_16L           43
#define R_PPC_GOT_16            44
#define R_PPC_GOTP_16HU         47
#define R_PPC_GOTP_32           48
#define R_PPC_GOTP_32UA         49
#define R_PPC_GOTP_16H          50
#define R_PPC_GOTP_16L          51
#define R_PPC_GOTP_16           52
#define R_PPC_PLT_16HU          55
#define R_PPC_PLT_32            56
#define R_PPC_PLT_32UA          57
#define R_PPC_PLT_16H           58
#define R_PPC_PLT_16L           59
#define R_PPC_PLT_16            60
#define R_PPC_ABREL_16HU        63
#define R_PPC_ABREL_32          64
#define R_PPC_ABREL_32UA        65
#define R_PPC_ABREL_16H         66
#define R_PPC_ABREL_16L         67
#define R_PPC_ABREL_16          68
#define R_PPC_GOT_ABREL_16HU    71
#define R_PPC_GOT_ABREL_32      72
#define R_PPC_GOT_ABREL_32UA    73
#define R_PPC_GOT_ABREL_16H     74
#define R_PPC_GOT_ABREL_16L     75
#define R_PPC_GOT_ABREL_16      76
#define R_PPC_GOTP_ABREL_16HU   79
#define R_PPC_GOTP_ABREL_32     80
#define R_PPC_GOTP_ABREL_32UA   81
#define R_PPC_GOTP_ABREL_16H    82
#define R_PPC_GOTP_ABREL_16L    83
#define R_PPC_GOTP_ABREL_16     84
#define R_PPC_PLT_ABREL_16HU    87
#define R_PPC_PLT_ABREL_32      88
#define R_PPC_PLT_ABREL_32UA    89
#define R_PPC_PLT_ABREL_16H     90
#define R_PPC_PLT_ABREL_16L     91
#define R_PPC_PLT_ABREL_16      92
#define R_PPC_SREL_16HU         95
#define R_PPC_SREL_32           96
#define R_PPC_SREL_32UA         97
#define R_PPC_SREL_16H          98
#define R_PPC_SREL_16L          99

typedef struct elf32_rel {
  Elf32_Addr	r_offset;
  Elf32_Word	r_info;
} Elf32_Rel;

typedef struct elf32_rela{
  Elf32_Addr	r_offset;
  Elf32_Word	r_info;
  Elf32_Sword	r_addend;
} Elf32_Rela;

typedef struct elf32_sym{
  Elf32_Word	st_name;
  Elf32_Addr	st_value;
  Elf32_Word	st_size;
  unsigned char	st_info;
  unsigned char	st_other;
  Elf32_Half	st_shndx;
} Elf32_Sym;


#define EI_NIDENT	16

typedef struct elfhdr{
  unsigned char	e_ident[EI_NIDENT];
  Elf32_Half	e_type;
  Elf32_Half	e_machine;
  Elf32_Word	e_version;
  Elf32_Addr	e_entry;  /* Entry point */
  Elf32_Off	e_phoff;
  Elf32_Off	e_shoff;
  Elf32_Word	e_flags;
  Elf32_Half	e_ehsize;
  Elf32_Half	e_phentsize;
  Elf32_Half	e_phnum;
  Elf32_Half	e_shentsize;
  Elf32_Half	e_shnum;
  Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

/* These constants define the permissions on sections in the program
   header, p_flags. */
#define PF_R		0x4
#define PF_W		0x2
#define PF_X		0x1

typedef struct elf_phdr{
  Elf32_Word	p_type;
  Elf32_Off	p_offset;
  Elf32_Addr	p_vaddr;
  Elf32_Addr	p_paddr;
  Elf32_Word	p_filesz;
  Elf32_Word	p_memsz;
  Elf32_Word	p_flags;
  Elf32_Word	p_align;
} Elf32_Phdr;

/* sh_type */
#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_DYNAMIC	6
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB	10
#define SHT_DYNSYM	11
#define SHT_NUM		12
#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

/* sh_flags */
#define SHF_WRITE	0x1
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4
#define SHF_MASKPROC	0xf0000000

/* special section indexes */
#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff
 
typedef struct {
  Elf32_Word	sh_name;
  Elf32_Word	sh_type;
  Elf32_Word	sh_flags;
  Elf32_Addr	sh_addr;
  Elf32_Off	sh_offset;
  Elf32_Word	sh_size;
  Elf32_Word	sh_link;
  Elf32_Word	sh_info;
  Elf32_Word	sh_addralign;
  Elf32_Word	sh_entsize;
} Elf32_Shdr;

#define	EI_MAG0		0		/* e_ident[] indexes */
#define	EI_MAG1		1
#define	EI_MAG2		2
#define	EI_MAG3		3
#define	EI_CLASS	4
#define	EI_DATA		5
#define	EI_VERSION	6
#define	EI_PAD		7

#define	ELFMAG0		0x7f		/* EI_MAG */
#define	ELFMAG1		'E'
#define	ELFMAG2		'L'
#define	ELFMAG3		'F'
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

#define	ELFCLASSNONE	0		/* EI_CLASS */
#define	ELFCLASS32	1
#define	ELFCLASS64	2
#define	ELFCLASSNUM	3

#define ELFDATANONE	0		/* e_ident[EI_DATA] */
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

#define EV_NONE		0		/* e_version, EI_VERSION */
#define EV_CURRENT	1
#define EV_NUM		2

/* Notes used in ET_CORE */
#define NT_PRSTATUS	1
#define NT_PRFPREG	2
#define NT_PRPSINFO	3
#define NT_TASKSTRUCT	4

/* Note header in a PT_NOTE section */
typedef struct elf_note {
  Elf32_Word	n_namesz;	/* Name size */
  Elf32_Word	n_descsz;	/* Content size */
  Elf32_Word	n_type;		/* Content type */
} Elf32_Nhdr;

#define ELF_START_MMAP 0x80000000

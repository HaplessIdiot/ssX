/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/int10/linux.c,v 1.4 1999/12/03 19:17:43 eich Exp $ */
/*
 * linux specific part of the int10 module
 * Copyright 1999 Egbert Eich
 */
#include "xf86.h"
#include "xf86str.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "compiler.h"
#define _INT10_PRIVATE
#include "xf86int10.h"
#include "int10Defines.h"
#define DEV_MEM "/dev/mem"
#ifndef XFree86LOADER
#include <sys/mman.h>
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif
#endif
static int base_addr = 0x100000;
#define ALLOC_ENTRIES(x) ((V_RAM / x) - 1)
#define REG pInt

static CARD8 read_b(xf86Int10InfoPtr pInt,int addr);
static CARD16 read_w(xf86Int10InfoPtr pInt,int addr);
static CARD32 read_l(xf86Int10InfoPtr pInt,int addr);
static void write_b(xf86Int10InfoPtr pInt,int addr, CARD8 val);
static void write_w(xf86Int10InfoPtr pInt,int addr, CARD16 val);
static void write_l(xf86Int10InfoPtr pInt,int addr, CARD32 val);

int10MemRec linuxMem = {
    read_b,
    read_w,
    read_l,
    write_b,
    write_w,
    write_l
};

typedef struct {
    int base;
    void* vidMem;
    int screen;
    char* alloc;
} linuxInt10Priv;

xf86Int10InfoPtr
xf86InitInt10(int entityIndex)
{
    xf86Int10InfoPtr pInt;
    int screen;
    int fd;
    void* vidMem;
    int pagesize;
    int alloc_entries;
    legacyVGARec vga;
    
    screen = (xf86FindScreenForEntity(entityIndex))->scrnIndex;
    if (int10skip(xf86Screens[screen],entityIndex))
	return NULL;
    vidMem = xf86MapVidMem(screen,VIDMEM_FRAMEBUFFER,V_RAM,VRAM_SIZE);
    pInt = (xf86Int10InfoPtr)xnfcalloc(1,sizeof(xf86Int10InfoRec));
    pInt->entityIndex = entityIndex;
    if (!xf86Int10ExecSetup(pInt))
	goto error0;
    pInt->mem = &linuxMem;
    pagesize = getpagesize();
    alloc_entries = ALLOC_ENTRIES(pagesize);
    pInt->private = (pointer)xnfcalloc(1,sizeof(linuxInt10Priv));
    ((linuxInt10Priv*)pInt->private)->base = base_addr;
    ((linuxInt10Priv*)pInt->private)->vidMem = vidMem;
    ((linuxInt10Priv*)pInt->private)->screen = screen;
    ((linuxInt10Priv*)pInt->private)->alloc = 
	(pointer)xnfcalloc(1,sizeof(ALLOC_ENTRIES(pagesize)));

#ifdef DEBUG
    ErrorF("Mapping 1 M area\n");
#endif
    if ((fd = open("/dev/zero",O_RDWR,0)) < 0
	|| mmap((void *)base_addr, SYS_SIZE, PROT_READ | PROT_WRITE
		| PROT_EXEC, MAP_PRIVATE | MAP_FIXED, fd, 0)
	== MAP_FAILED) {
	xf86Msg(X_ERROR,"Cannot map 1 M area\n");
	close(fd);
	goto error1;
    }
    close(fd);
#ifdef DEBUG
    ErrorF("Mapping sys bios area\n");
#endif
    if ((fd = open(DEV_MEM,O_RDWR,0)) < 0
	|| mmap((void *)(base_addr + SYS_BIOS),BIOS_SIZE,PROT_READ
		| PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_FIXED,
		fd, SYS_BIOS) == MAP_FAILED) {
	xf86Msg(X_ERROR,"Cannot map SYS BIOS\n");
	close(fd);
	goto error2;
    }
    close(fd);
#ifdef DEBUG
    ErrorF("Mapping int area\n");
#endif
    if (xf86ReadBIOS(0,0,(unsigned char *)base_addr,LOW_PAGE_SIZE) < 0) {
	xf86Msg(X_ERROR,"Cannot read int vect\n");
	goto error3;
    }
#ifdef DEBUG
    ErrorF("Mapping VRAM area\n");
#endif
    if ((fd = open("/proc/self/mem",O_RDWR,0)) < 0
	|| mmap((void *)(base_addr + V_RAM),VRAM_SIZE,PROT_READ
		| PROT_WRITE, MAP_SHARED | MAP_FIXED,
		fd, (size_t)vidMem) == MAP_FAILED) {
	xf86Msg(X_ERROR,"Cannot map V_RAM\n");
	close(fd);
	goto error3;
    }
    close(fd);

#ifdef DEBUG
    dprint(base_addr,0x20);
    dprint(base_addr + 0xa0000,0x20);
    dprint(base_addr + 0xf0000,0x20);
#endif
    
    if (xf86IsEntityPrimary(entityIndex)) {
	int cs = ((CARD16*)base_addr)[(0x10<<1)+1];
	CARD8 *bios_base = (unsigned char *)base_addr + (cs << 4);
	int size;
	
	xf86Msg(X_INFO,"Primary V_BIOS segment is: 0x%x\n",cs);
	if (xf86ReadBIOS(cs << 4,0,bios_base, 0x10) < 0) {
	    xf86Msg(X_ERROR,"Cannot read V_BIOS\n");
	    goto error4;
	}
	if (!(*bios_base == 0x55 && *(bios_base + 1) == 0xAA)) {
	    xf86Msg(X_ERROR,"No V_BIOS found\n");
	    goto error4;
	}
	size = *(bios_base + 2) * 512;
	if (xf86ReadBIOS(cs << 4,0,bios_base, size) < 0) {
	    xf86Msg(X_ERROR,"Cannot read V_BIOS\n");
	    goto error4;
	}
	if (bios_checksum(bios_base,size)) {
	    xf86Msg(X_ERROR,"Bad checksum of V_BIOS \n");
	    goto error4;
	}
	pInt->BIOSseg = cs;
	MapCurrentInt10(pInt);  /* required for set_return_trap() */
	set_return_trap(pInt);
    } else {
	if (!mapPciRom(pInt,(unsigned char *)(base_addr + V_BIOS))) {
	    xf86Msg(X_ERROR,"Cannot read V_BIOS\n");
	    goto error4;
	}

	pInt->BIOSseg = V_BIOS >> 4;
	pInt->num = 0xe6;
	MapCurrentInt10(pInt);  /* required for reset_int_vect() */
	reset_int_vect(pInt);
	set_return_trap(pInt);
	LockLegacyVGA(screen, &vga);
	xf86ExecX86int10(pInt);
	UnlockLegacyVGA(screen, &vga);
    }
#ifdef DEBUG
    dprint(base_addr + 0xc0000,0x20);
#endif
    base_addr += SYS_SIZE;
    
    return pInt;

 error4:
    munmap((void *)(base_addr + V_RAM),VRAM_SIZE);
 error3:
    munmap((void *)(base_addr + SYS_BIOS),BIOS_SIZE);
 error2:
    munmap((void *)base_addr,SYS_SIZE);
 error1:
    xf86UnMapVidMem(((linuxInt10Priv*)pInt->private)->screen,
		    ((linuxInt10Priv*)pInt->private)->vidMem,
		    VRAM_SIZE);
    xfree(((linuxInt10Priv*)pInt->private)->alloc);
    xfree(pInt->private);
 error0:
    xfree(pInt);
    return NULL;
}

void
MapCurrentInt10(xf86Int10InfoPtr pInt)
{
    int fd;

    if (Int10Current)
	munmap((void*)((linuxInt10Priv *)Int10Current->private)->base,
	       SYS_SIZE);
    if ((fd = open("/proc/self/mem",O_RDWR,0)) < 0)
	return;
    mmap(0,SYS_SIZE,PROT_READ | PROT_WRITE,
	 MAP_SHARED | MAP_FIXED, fd,
	 ((linuxInt10Priv *)pInt->private)->base);
    close(fd);
}

void
xf86FreeInt10(xf86Int10InfoPtr pInt)
{
    if (!pInt)
        return;
    if (Int10Current == pInt) {
	munmap((void*)(((linuxInt10Priv *)Int10Current->private)->base),
	       SYS_SIZE);
	Int10Current = NULL;
    }
    munmap((void *)(((linuxInt10Priv *)pInt->private)->base+V_RAM),VRAM_SIZE);
    munmap((void *)(((linuxInt10Priv *)pInt->private)->base+SYS_BIOS),
	   BIOS_SIZE);
    munmap((void *)((linuxInt10Priv *)pInt->private)->base,SYS_SIZE);
    xf86UnMapVidMem(((linuxInt10Priv*)pInt->private)->screen,
		    ((linuxInt10Priv*)pInt->private)->vidMem,
		    VRAM_SIZE);
    xfree(((linuxInt10Priv*)pInt->private)->alloc);
	    xfree(pInt->private);
	    xfree(pInt);
}

void *
xf86Int10AllocPages(xf86Int10InfoPtr pInt,int num, int *off)
{
    int pagesize = getpagesize();
    int num_pages = ALLOC_ENTRIES(pagesize);
    int i,j;

    for (i=0;i<num_pages - num;i++) {
	if (((linuxInt10Priv*)pInt->private)->alloc[i] == 0) {
	    for (j=i;j < num + i;j++)
		if ((((linuxInt10Priv*)pInt->private)->alloc[j] != 0))
		    break;
	    if (j == num + i)
		break;
	    else
		i = i + num;
	}
    }
    if (i == num_pages - num)
	return NULL;
    
    for (j = i; j < i + num; j++)
	((linuxInt10Priv*)pInt->private)->alloc[j] = 1;

    *off = (i + 1) * pagesize;
    
    return (void *)
	(((linuxInt10Priv*)pInt->private)->base + (i + 1) * pagesize);
}

void
xf86Int10FreePages(xf86Int10InfoPtr pInt, void *pbase, int num)
{
    int pagesize = getpagesize();
    int first = ((CARD32)pbase - ((linuxInt10Priv*)pInt->private)->base)
	/ pagesize - 1;
    int i;

    for (i = first; i < first + num; i++)
	((linuxInt10Priv*)pInt->private)->alloc[i] = 0;
}

static CARD8
read_b(xf86Int10InfoPtr pInt,int addr)
{
    return *((CARD8 *)(addr));
}

static CARD16
read_w(xf86Int10InfoPtr pInt,int addr)
{
    return *((CARD16 *)(addr));
}

static CARD32
read_l(xf86Int10InfoPtr pInt,int addr)
{
    return *((CARD32 *)(addr));
}

static void
write_b(xf86Int10InfoPtr pInt,int addr, CARD8 val)
{
    *((CARD8 *)(addr)) = (CARD8)val;
}

static void
write_w(xf86Int10InfoPtr pInt,int addr, CARD16 val)
{
    *((CARD16 *)(addr)) = (CARD16)val;
}

static
void write_l(xf86Int10InfoPtr pInt,int addr, CARD32 val)
{
    *((CARD32 *)(addr)) = (CARD32)val;
}

pointer
xf86int10Addr(xf86Int10InfoPtr pInt, CARD32 addr)
{
    return  (pointer)(((linuxInt10Priv*)pInt->private)->base + addr);
}

#ifdef _VM86_LINUX

static int vm86_rep(struct vm86_struct *ptr);

Bool
xf86Int10ExecSetup(xf86Int10InfoPtr pInt)
{
#define VM86S ((struct vm86_struct *)pInt->cpuRegs)

    pInt->cpuRegs = (pointer) xnfcalloc(1,sizeof(struct vm86_struct));
    VM86S->flags = 0;
    VM86S->screen_bitmap = 0;
    VM86S->cpu_type = CPU_586;
    memset(&VM86S->int_revectored, 0xff,sizeof(VM86S->int_revectored)) ;
    memset(&VM86S->int21_revectored, 0xff,sizeof(VM86S->int21_revectored)) ;
    return TRUE;
}
    
static int
do_vm86(xf86Int10InfoPtr pInt)
{
    int retval;
    
    retval = vm86_rep(VM86S);
    
    switch (VM86_TYPE(retval)) {
    case VM86_UNKNOWN:
	if (!vm86_GP_fault(pInt)) return 0;
	break;
    case VM86_STI:
	xf86Msg(X_ERROR,"vm86_sti :-((\n");
	stack_trace(pInt);
	dump_code(pInt);
	return 0;
    case VM86_INTx:
	pInt->num = VM86_ARG(retval);
	if (!int_handler(pInt)) {
	    xf86Msg(X_ERROR,"Unknown vm86_int: 0x%X\n\n",VM86_ARG(retval));
	    dump_registers(pInt);
	    return 0;
	}
	/* I'm not sure yet what to do if we can handle ints */
	break;
    case VM86_SIGNAL:
	return 1;
	/*
	 * we used to warn here and bail out - but now the sigio stuff
	 * always fires signals at us. So we just ignore them for now.
	 */
	xf86Msg(X_WARNING,"received signal\n");
	return 0;
    default:
	xf86Msg(X_ERROR,"unknown type(0x%x)=0x%x\n",
		VM86_ARG(retval),VM86_TYPE(retval));
	dump_registers(pInt);
	dump_code(pInt);
	stack_trace(pInt);
	return 0;
    }
    
    return 1;
}

void
xf86ExecX86int10(xf86Int10InfoPtr pInt)
{
    setup_int(pInt);

    if (int_handler(pInt))
	while(do_vm86(pInt)) {};

    finish_int(pInt);
}

static int
vm86_rep(struct vm86_struct *ptr) 
{
    int __res;

    __asm__ __volatile__("int $0x80\n"
			 :"=a" (__res):"a" ((int)113),
			 "b" ((struct vm86_struct *)ptr));

	    if ((__res) < 0) {
		errno = -__res;
		__res=-1;
	    }
	    else errno = 0;
	    return __res;
}

#endif

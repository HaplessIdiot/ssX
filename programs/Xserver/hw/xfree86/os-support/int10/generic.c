/* $XFree86$ */

#include "xf86.h"
#include "xf86str.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#define _INT10_PRIVATE
#include "xf86int10.h"
#include "defines.h"

static CARD8 read_b(xf86Int10InfoPtr pInt,int addr);
static CARD16 read_w(xf86Int10InfoPtr pInt,int addr);
static CARD32 read_l(xf86Int10InfoPtr pInt,int addr);
static void write_b(xf86Int10InfoPtr pInt,int addr, CARD8 val);
static void write_w(xf86Int10InfoPtr pInt,int addr, CARD16 val);
static void write_l(xf86Int10InfoPtr pInt,int addr, CARD32 val);

/*
 * the emulator cannot pass a pointer to the current xf86Int10InfoRec
 * to the memory access functions therefore store it here.
 */

typedef struct {
    int screen;
    int shift;
    int pagesize_1;
    int entries;
    memType *alloc_rec;
} genericInt10Priv;

int10MemRec genericMem = {
    read_b,
    read_w,
    read_l,
    write_b,
    write_w,
    write_l
};

static void setupTable(xf86Int10InfoPtr pInt, memType address,
		       int loc,int size);

xf86Int10InfoPtr
xf86InitInt10(int entityIndex)
{
    xf86Int10InfoPtr pInt;
    int screen;
    void* vidMem;
    void* sysMem;
    void* intMem;
    void* vbiosMem;
    int pagesize;
    int entries;
    int shift;
    
    screen = (xf86FindScreenForEntity(entityIndex))->scrnIndex;
    if (int10skip(xf86Screens[screen]))
	return NULL;

    pInt = (xf86Int10InfoPtr)xnfcalloc(1,sizeof(xf86Int10InfoRec));
    pInt->entityIndex = entityIndex;
    if (!xf86Int10ExecSetup(pInt))
	goto error0;
    pInt->mem = &genericMem;
    pagesize = xf86getpagesize();
    pInt->private = (pointer)xnfcalloc(1,sizeof(genericInt10Priv));
    entries = SYS_SIZE / pagesize;

    ((genericInt10Priv*)pInt->private)->screen = screen;
    ((genericInt10Priv*)pInt->private)->pagesize_1 = pagesize - 1;
    ((genericInt10Priv*)pInt->private)->entries = entries;
    ((genericInt10Priv*)pInt->private)->alloc_rec =
	xnfcalloc(1,sizeof(memType) * entries);
    for (shift = 0 ; (pagesize >> shift) ; shift++) {};
    shift -= 1;
    ((genericInt10Priv*)pInt->private)->shift = shift;

    vidMem = xf86MapVidMem(screen,VIDMEM_FRAMEBUFFER,V_RAM,VRAM_SIZE);
    setupTable(pInt,(memType)vidMem,V_RAM,VRAM_SIZE);
    intMem = xnfalloc(pagesize);
    setupTable(pInt,(memType)intMem,0,pagesize);
    vbiosMem = xnfalloc(V_BIOS_SIZE);

#ifdef _PC
    sysMem = xf86MapVidMem(screen,VIDMEM_FRAMEBUFFER,SYS_BIOS,BIOS_SIZE);
    setupTable(pInt,(memType)sysMem,SYS_BIOS,BIOS_SIZE);
    if (xf86ReadBIOS(0,0,(unsigned char *)intMem,LOW_PAGE_SIZE) < 0) {
	xf86Msg(X_ERROR,"Cannot read int vect\n");
	goto error1;
    }
    if (xf86IsEntityPrimary(entityIndex)) {
	int cs = MEM_RW(pInt,((0x10<<2)+2));
	xf86Msg(X_INFO,"Primary V_BIOS segmant is: 0x%x\n",cs);
	if (xf86ReadBIOS(cs << 8,0,(unsigned char *)vbiosMem,
			 V_BIOS_SIZE) < 0) {
	    xf86Msg(X_ERROR,"Cannot read V_BIOS\n");
	    goto error1;
	}
	setupTable(pInt,(memType)vbiosMem,cs<<4,V_BIOS_SIZE);
	pInt->BIOSseg = cs;
    } else {
	reset_int_vect(pInt);
	if (!mapPciRom(pInt,(unsigned char *)(vbiosMem))) {
	    xf86Msg(X_ERROR,"Cannot read V_BIOS\n");
	    goto error1;
	}
	setupTable(pInt,(memType)vbiosMem,V_BIOS,V_BIOS_SIZE);
	pInt->BIOSseg = V_BIOS >> 4;
	pInt->num = 0xe6;
		   
	xf86ExecX86int10(pInt);
    }
#else
    sysMem = xnfalloc(BIOS_SIZE);
    setup_system_bios((unsigned long)sysMem);
    setupTable(pInt,(memType)sysMem,SYS_BIOS,BIOS_SIZE);
    setup_int_vect(pInt);
    if (!mapPciRom(pInt,(unsigned char *)(vbiosMem))) {
	xf86Msg(X_ERROR,"Cannot read V_BIOS\n");
	goto error1;
    }
    setupTable(pInt,(memType)vbiosMem,V_BIOS,V_BIOS_SIZE);
    pInt->BIOSseg = V_BIOS >> 4;
    pInt->num = 0xe6;
    xf86ExecX86int10(pInt);
#endif
    return pInt;

 error1:
#ifndef _PC
    xfree(sysMem);
#else
    xf86UnMapVidMem(((genericInt10Priv*)pInt->private)->screen,
		    (pointer)((genericInt10Priv*)pInt->private)->alloc_rec[
			SYS_BIOS/pagesize],BIOS_SIZE);
#endif
    xfree(vbiosMem);
    xfree(intMem);
    xf86UnMapVidMem(((genericInt10Priv*)pInt->private)->screen,
		    (pointer)((genericInt10Priv*)pInt->private)->alloc_rec[
			V_RAM/pagesize],VRAM_SIZE);
    xfree(((genericInt10Priv*)pInt->private)->alloc_rec);
    xfree(pInt->private);
 error0:
    xfree(pInt);
    
    return NULL;
}

void
MapCurrentInt10(xf86Int10InfoPtr pInt)
{
}

void
xf86FreeInt10(xf86Int10InfoPtr pInt)
{
    int pagesize = ((genericInt10Priv*)pInt->private)->pagesize_1 + 1;
    if (Int10Current == pInt) 
	Int10Current = NULL;
#ifndef _PC
    xfree(((genericInt10Priv*)pInt->private)->alloc_rec[SYS_BIOS/pagesize]);
#else
    xf86UnMapVidMem(((genericInt10Priv*)pInt->private)->screen,
		    (pointer)((genericInt10Priv*)pInt->private)->alloc_rec[
			SYS_BIOS/pagesize],BIOS_SIZE);
#endif
    xfree(((genericInt10Priv*)pInt->private)->alloc_rec[V_BIOS/pagesize]);
    xfree(((genericInt10Priv*)pInt->private)->alloc_rec[0]);
    xf86UnMapVidMem(((genericInt10Priv*)pInt->private)->screen,
		    (pointer)((genericInt10Priv*)pInt->private)->alloc_rec[
			V_RAM/pagesize],VRAM_SIZE);
    xfree(((genericInt10Priv*)pInt->private)->alloc_rec);
    xfree(pInt->private);
    xfree(pInt);
}

void *
xf86Int10AllocPages(xf86Int10InfoPtr pInt,int num, int *off)
{
    void* addr;
    int pagesize = ((genericInt10Priv*)pInt->private)->pagesize_1 + 1;
    int num_pages = ((genericInt10Priv*)pInt->private)->entries;
    int i,j;

    for (i=0;i<num_pages - num;i++) {
	if (((genericInt10Priv*)pInt->private)->alloc_rec[i] == 0) {
	    for (j=i;j < num + i;j++)
		if ((((genericInt10Priv*)pInt->private)->alloc_rec[j] != 0))
		    break;
	    if (j == num + i)
		break;
	    else
		i = i + num;
	}
    }
    if (i == num_pages - num)
	return NULL;

    *off = i * pagesize;
    addr = xnfalloc(pagesize * num);
    setupTable(pInt,(memType)addr,*off,pagesize * num);
    
    return addr;
}

void
xf86Int10FreePages(xf86Int10InfoPtr pInt, void *pbase, int num)
{
    int num_pages = ((genericInt10Priv*)pInt->private)->entries;
    int i,j;
    for (i = 0;i<num_pages - num; i++)
	if (((genericInt10Priv*)pInt->private)->alloc_rec[i]==(memType)pbase) {
	    for (j = 0; j < num; j++)
		((genericInt10Priv*)pInt->private)->alloc_rec[i] = 0;
	    break;
	}
    xfree(pbase);
    return;
}

static void
setupTable(xf86Int10InfoPtr pInt, memType address,int loc,int size)
{
    int pagesize = ((genericInt10Priv*)pInt->private)->pagesize_1 + 1;
    int i,j,num;

    i = loc / pagesize;
    num = (size + pagesize - 1)/ pagesize; /* round up to the nearest page */
                                           /* boudary if size is not       */
                                           /* multiple of pagesize         */
    for (j = 0; j<num; j++) {
	((genericInt10Priv*)pInt->private)->alloc_rec[i+j] = address;
	address += pagesize;
    }
}

#define OFF(addr) \
          ((addr) & (((genericInt10Priv*)pInt->private)->pagesize_1))
#define SHIFT \
          (((genericInt10Priv*)pInt->private)->shift)
#define BASE(addr,shift) \
          (((genericInt10Priv*)pInt->private)->alloc_rec[addr >> shift])
#define V_ADDR(addr,shift,off) \
          (BASE(addr,shift) + (off))

static CARD8
read_b(xf86Int10InfoPtr pInt, int addr)
{
    return  *(CARD8*) V_ADDR(addr,SHIFT,OFF(addr));
}

static CARD16
read_w(xf86Int10InfoPtr pInt, int addr)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
	return ((*(CARD8*) V_ADDR(addr,shift,off))
		|| ((*(CARD8*) V_ADDR(addr,shift,off + 1)) << 8));
#else
    if (OFF(addr + 1) > 0) {
	return *(CARD16*) V_ADDR(addr,SHIFT,OFF(addr));
    } else {
	int shift = SHIFT;
	int off = OFF(addr);

	return ((*(CARD8*) V_ADDR(addr,shift,off + 1))
		|| ((*(CARD8*) V_ADDR(addr,shift,off)) << 8));
    }
#endif
}

static CARD32
read_l(xf86Int10InfoPtr pInt, int addr)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
    return ((*(CARD8*) V_ADDR(addr,shift,off))
	    || ((*(CARD8*) V_ADDR(addr,shift,off + 1)) << 8)
	    || ((*(CARD8*) V_ADDR(addr,shift,off + 2)) << 16)
	    || ((*(CARD8*) V_ADDR(addr,shift,off + 3)) << 24));
#else
    if (OFF(addr + 3) > 2) {
	return *(CARD32*) V_ADDR(addr,SHIFT,OFF(addr));
    } else {
	int shift = SHIFT;
	int off = OFF(addr);
	
	return ((*(CARD8*) V_ADDR(addr,shift,off + 3))
		|| ((*(CARD8*) V_ADDR(addr,shift,off + 2)) << 8)
		|| ((*(CARD8*) V_ADDR(addr,shift,off + 1)) << 16)
		|| ((*(CARD8*) V_ADDR(addr,shift,off)) << 24));
    }
#endif
}

static void
write_b(xf86Int10InfoPtr pInt, int addr, CARD8 val)
{
    *(CARD8*) V_ADDR(addr,SHIFT,OFF(addr)) = val;
}

static void
write_w(xf86Int10InfoPtr pInt, int addr, CARD16 val)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
    (*(CARD8*) V_ADDR(addr,shift,off)) = val;
    (*(CARD8*) V_ADDR(addr,shift,off + 1)) = val >> 8;
#else
    if (OFF(addr + 1) > 0) {
	*(CARD16*) V_ADDR(addr,SHIFT,OFF(addr)) = val;
    } else {
	int shift = SHIFT;
	int off = OFF(addr);

	(*(CARD8*) V_ADDR(addr,shift,off + 1)) = val;
	(*(CARD8*) V_ADDR(addr,shift,off)) = val >> 8;
    }
#endif
}

static void
write_l(xf86Int10InfoPtr pInt, int addr, CARD32 val)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
    (*(CARD8*) V_ADDR(addr,shift,off)) = val;
    (*(CARD8*) V_ADDR(addr,shift,off + 1)) = val >> 8;
    (*(CARD8*) V_ADDR(addr,shift,off + 2)) = val >> 16;
    (*(CARD8*) V_ADDR(addr,shift,off + 3))= val >> 24;
#else
    if (OFF(addr + 3) > 2) {
	*(CARD32*) V_ADDR(addr,SHIFT,OFF(addr)) = val;
    } else {
	int shift = SHIFT;
	int off = OFF(addr);
	
	(*(CARD8*) V_ADDR(addr,shift,off + 3)) = val;
	(*(CARD8*) V_ADDR(addr,shift,off + 2)) = val >> 8;
	(*(CARD8*) V_ADDR(addr,shift,off + 1)) = val >> 16;
	(*(CARD8*) V_ADDR(addr,shift,off))= val >> 24;
    }
#endif
}


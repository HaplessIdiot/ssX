/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/vboard.c,v 1.6 1999/11/19 13:54:46 hohndel Exp $ */
/*
 * includes
 */

#include "rendition.h"
#include "v1krisc.h"
#include "vboard.h"
#include "vloaduc.h"
#include "vos.h"



/* 
 * global data
 */

#include "cscode.h"

/* Global imported during compile-time */
char MICROCODE_DIR [PATH_MAX] = MODULEDIR;



/*
 * local function prototypes
 */


/*
 * functions
 */
int v_initboard(ScrnInfoPtr pScreenInfo)
{
  renditionPtr pRendition = RENDITIONPTR(pScreenInfo);

  vu16 iob=pRendition->board.io_base;
  vu8 *vmb;
  vu32 offset;
  int c;

  /* write "monitor" program to memory */
  v1k_stop(pScreenInfo);
  pRendition->board.csucode_base=0x800;
  v_out8(iob+MEMENDIAN, MEMENDIAN_NO);

  /* Note that CS ucode must wait on address in csucode_base
   * when initialized for later context switch code to work. */
  vmb=pRendition->board.vmem_base;
  offset=pRendition->board.csucode_base;
  for (c=0; c<sizeof(csrisc)/sizeof(vu32); c++, offset+=sizeof(vu32))
    v_write_memory32(vmb, offset, csrisc[c]);

  if (V1000_DEVICE == pRendition->board.chip){
    c=v_load_ucfile(pScreenInfo, strcat ((char *)MICROCODE_DIR,"v10002d.uc"));
  }
  else {
    /* V2x00 chip */
    c=v_load_ucfile(pScreenInfo, strcat ((char *)MICROCODE_DIR,"v20002d.uc"));
  }

  if (c == -1) {
    ErrorF( "RENDITION: Microcode loading failed !!!\n");
    return 1;
  }

  pRendition->board.ucode_entry=c;

  ErrorF("UCode_Entry == 0x%x\n",pRendition->board.ucode_entry);

  /* Everything's OK */
  return 0;
}


int v_resetboard(ScrnInfoPtr pScreenInfo)
{
  /*
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
  */
  v1k_softreset(pScreenInfo);
  return 0;
}



int v_getmemorysize(ScrnInfoPtr pScreenInfo)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);

#define PATTERN  0xf5faaf5f
#define START    0x12345678
#define ONEMEG   (1024L*1024L)
    vu32 offset;
    vu32 pattern;
    vu32 start;
    vu8 memendian;
#ifdef XSERVER
    vu8 modereg;

    modereg=v_in8(pRendition->board.io_base+MODEREG);
    v_out8(pRendition->board.io_base+MODEREG, NATIVE_MODE);
#endif

    /* no byte swapping */
    memendian=v_in8(pRendition->board.io_base+MEMENDIAN);
    v_out8(pRendition->board.io_base+MEMENDIAN, MEMENDIAN_NO);

    /* it looks like the v1000 wraps the memory; but for I'm not sure,
     * let's test also for non-writable offsets */
    start=v_read_memory32(pRendition->board.vmem_base, 0);
    v_write_memory32(pRendition->board.vmem_base, 0, START);
    for (offset=ONEMEG; offset<16*ONEMEG; offset+=ONEMEG) {
#ifdef DEBUG
        ErrorF( "Testing %d MB: ", offset/ONEMEG);
#endif
        pattern=v_read_memory32(pRendition->board.vmem_base, offset);
        if (START == pattern) {
#ifdef DEBUG
            ErrorF( "Back at the beginning\n");
#endif
            break;    
        }
        
        pattern^=PATTERN;
        v_write_memory32(pRendition->board.vmem_base, offset, pattern);
        
#ifdef DEBUG
        ErrorF( "%x <-> %x\n", (int)pattern, 
                    (int)v_read_memory32(pRendition->board.vmem_base, offset));
#endif

        if (pattern != v_read_memory32(pRendition->board.vmem_base, offset)) {
            offset-=ONEMEG;
            break;    
        }
        v_write_memory32(pRendition->board.vmem_base, offset, pattern^PATTERN);
    }
    v_write_memory32(pRendition->board.vmem_base, 0, start);

    if (16*ONEMEG <= offset)
        pRendition->board.mem_size=4*ONEMEG;
    else 
	    pRendition->board.mem_size=offset;

    /* restore default byte swapping */
    v_out8(pRendition->board.io_base+MEMENDIAN, MEMENDIAN_NO);

#ifdef XSERVER
    v_out8(pRendition->board.io_base+MODEREG, modereg);
#endif

    return pRendition->board.mem_size;
#undef PATTERN
#undef ONEMEG
}



/*
 * local functions
 */



/*
 * end of file vboard.c
 */

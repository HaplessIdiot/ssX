/*
 * file vos.h
 *
 * Operating system dependant functions.
 */



/*
 * includes
 */

#include "vos.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>



/*
 * functions
 */

/*
 * void v_enableio(void)
 *
 * Enable access to all 65536 ports.
 */
void v_enableio(void) 
{
    if (iopl(3)) {
        fprintf(stderr, "vlib: Failed to set iopl for io\n");
        exit(1);
    }
}



/*
 * void v_disableio(void)
 *
 * Disable access to the ports.
 */
void v_disableio(void) 
{
    iopl(0);
}


#ifdef __alpha__

void v_unmapmemory(vu8 *vmembase, vu32 size)
{
  xf86UnMapVidMemSparse(0, 0, vmembase, size);
}

vu8 *v_mapmemory(vu8 *membase, vu32 size)
{
  return (vu8 *)xf86MapVidMemSparse(0, 0, membase, size);
}

#else

/*
 * void v_unmapmemory(vu8 *vmembase, vu32 size)
 *
 * Unmap memory.
 */
void v_unmapmemory(vu8 *vmembase, vu32 size)
{
    munmap((caddr_t)vmembase, size);
}



vu8 *v_mapmemory(vu8 *membase, vu32 size)
{
    int fd;
    caddr_t vaddr;    

    if ((fd=open("/dev/mem", O_RDWR)) < 0) {
        fprintf(stderr, "vlib: Couldn't open /dev/mem\n");
        exit(1);
    }

    vaddr=mmap((caddr_t)0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd,
			   (off_t)membase);
    close(fd);

    if (-1 == (long)vaddr) {
        fprintf(stderr, "vlib: Couldn't mmap memory at %p\n", membase);
        exit(1);
    }

    return (vu8 *)vaddr;
}

#endif



/*
 * struct v_board_t *v_findverite(void)
 * 
 * Tries to find a Verite card and returns a handle for that card. If a
 * card could not be found, NULL is returned.
 */
struct v_board_t *v_findverite(void)
{
  FILE *f;
  unsigned int number; /* bus number */
  unsigned int devfn;
  unsigned short vendor;
  unsigned short device;
  unsigned int irq;
  unsigned long base[6];
  unsigned long mem_base;
  unsigned int io_base;
  int i;
  struct v_board_t *board;

  /* 
   * routine for kernel 2.0.x 
   */
  if (0 == access("/proc/pci", R_OK)) {
    f=popen("fgrep -A 3 Rendition /proc/pci | fgrep memory", "r");
    if (1 != fscanf(f, "%*s %*s %*s %*s %*s %lx", &mem_base))
      mem_base=0;
    fclose(f);

    f=popen("fgrep -A 3 Rendition /proc/pci | fgrep I/O", "r");
    if (1 != fscanf(f, "%*s %*s %x", &io_base))
      io_base=0;
    fclose(f);

    f=popen("fgrep Rendition /proc/pci", "r");
    if (1 != fscanf(f, "%*s %*s %*s %*s %*s %hd", &device))
      device=2000;
    fclose(f);

    if (0==mem_base || 0==io_base) {
      /* some pci drivers do not recognize Rendition/Verite */
      f=popen("fgrep -A 3 id=1163 /proc/pci | fgrep memory", "r");
      if (1 != fscanf(f, "%*s %*s %*s %*s %*s %lx", &mem_base))
        mem_base=0;
      fclose(f);

      f=popen("fgrep -A 3 id=1163 /proc/pci | fgrep I/O", "r");
      if (1 != fscanf(f, "%*s %*s %x", &io_base))
        io_base=0;
      fclose(f);

      f=popen("fgrep id=1163 /proc/pci", "r");
      if (1 != fscanf(f, "%*s %*s %*s %*c%*c%*c %hd", &device))
        device=2000;
      if (1 == device)
        device=1000;
      fclose(f);
    }

    if (0==mem_base || 0==io_base) {
      return NULL;
    }

    board=(struct v_board_t *)calloc(1, sizeof(struct v_board_t));
    board->mem_base=(vu8 *)mem_base;
    board->io_base=io_base;
    board->chip=(device==1000 ? V1000_DEVICE : V2000_DEVICE);

    fprintf(stderr, "found Rendition Verite %s at 0x%lx I/O 0x%x\n",
            (board->chip==V1000_DEVICE ? "V1000" : "V2x00"),
             (long)board->mem_base, board->io_base);

    return board;
  }

  /* routine for kernel 2.1.x */
  if ((f=fopen("/proc/bus/pci/devices", "rt"))==NULL) {
    fprintf(stderr, "Couldn't open proc pci file\n");
    exit(1);
  }

  while (!feof(f)) {
    fprintf(stderr, "checking device...\n");
    fscanf(f, "%02x%02x\t%04hx%04hx\t%x", &number, &devfn, &vendor, &device, &irq);
    for (i=0; i<6; i++)
      fscanf(f,
#if __alpha__
	     "\t%016lx",
#else
	     "\t%08lx",
#endif
	     &(base[i]));
    if (vendor == PCI_VENDOR_ID_RENDITION) {
      board=(struct v_board_t *)calloc(1, sizeof(struct v_board_t));
      board->chip = device;
      board->mem_base = (void *)(base[0] & PCI_BASE_ADDRESS_MEM_MASK);
      board->io_base = base[1] & PCI_BASE_ADDRESS_IO_MASK;
      board->mmio_base = base[2];
      return board;
    }
  }
  fclose(f);
  fprintf(stderr, "vlib: Couldn't find Rendition card\n");
  exit(1);
}



/*
 * end of file vos.c
 */

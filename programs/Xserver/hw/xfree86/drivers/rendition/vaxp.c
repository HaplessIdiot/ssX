#ifdef __alpha__
#include "vtypes.h"
#include "vaxp.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <asm/io.h>
/*
#include <sys/io.h>
*/


static int xf86SparseShift = 5; /* default to all but JENSEN */

vu8 *xf86MapVidMemSparse(vu8 *Base, vu64 Size)
{
	vu8 *base;
      	int fd;

	if (!_bus_base()) xf86SparseShift = 7; /* Uh, oh, JENSEN... */

	Size <<= xf86SparseShift;
	Base = (vu8 *)((unsigned long)Base << xf86SparseShift);

	if ((fd = open("/dev/mem", O_RDWR)) < 0)
	{
		perror("xf86MapVidMem: failed to open /dev/mem");
		exit(1);
	}
	/* This requirers linux-0.99.pl10 or above */
	base = (vu8 *)mmap((caddr_t)0, Size,
			     PROT_READ | PROT_WRITE,
			     MAP_SHARED, fd,
			     (off_t)Base + _bus_base_sparse());
	close(fd);
	if ((long)base == -1)
	{
		perror("xf86MapVidMem: Could not mmap framebuffer");
		exit(1);
	}
	return base;
}

void xf86UnMapVidMemSparse(vu8 *Base, vu64 Size)
{
	Size <<= xf86SparseShift;

	munmap((caddr_t)Base, Size);
}

#define vuip    volatile unsigned int *

vu8 xf86ReadSparse8(vu8 *Base, vu64 Offset)
{
    unsigned long result, shift;
    unsigned long msb = 0;

    shift = (Offset & 0x3) * 8;
    if (xf86SparseShift != 7) { /* if not JENSEN, we may need HAE */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000UL;
        Offset -= msb;
        if (msb) {
	  _sethae(msb);
        }
      }
    }
    result = *(vuip) ((unsigned long)Base + (Offset << xf86SparseShift));
    if (msb)
      _sethae(0);
    result >>= shift;
    return 0xffUL & result;
}

vu16 xf86ReadSparse16(vu8 *Base, vu64 Offset)
{
    unsigned long result, shift;
    unsigned long msb = 0;

    shift = (Offset & 0x2) * 8;
    if (xf86SparseShift != 7) { /* if not JENSEN, we may need HAE */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000UL;
        Offset -= msb;
        if (msb) {
	  _sethae(msb);
        }
      }
    }
    result = *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(1<<(xf86SparseShift-2)));
    if (msb)
      _sethae(0);
    result >>= shift;
    return 0xffffUL & result;
}

vu32 xf86ReadSparse32(vu8 *Base, vu64 Offset)
{
    unsigned long result;
    unsigned long msb = 0;

    if (xf86SparseShift != 7) { /* if not JENSEN, we may need HAE */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000UL;
        Offset -= msb;
        if (msb) {
	  _sethae(msb);
        }
      }
    }
    result = *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(3<<(xf86SparseShift-2)));
    if (msb)
      _sethae(0);
    return result;
}

void xf86WriteSparse8(vu8 Value, vu8 *Base, vu64 Offset)
{
    unsigned long msb = 0;
    unsigned int b = Value & 0xffU;

    if (xf86SparseShift != 7) { /* not JENSEN */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000;
        Offset -= msb;
        if (msb) {
	  _sethae(msb);
        }
      }
    }
    *(vuip) ((unsigned long)Base + (Offset << xf86SparseShift)) = b * 0x01010101;
    if (msb)
      _sethae(0);
}

void xf86WriteSparse16(vu16 Value, vu8 *Base, vu64 Offset)
{
    unsigned long msb = 0;
    unsigned int w = Value & 0xffffU;

    if (xf86SparseShift != 7) { /* not JENSEN */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000;
        Offset -= msb;
        if (msb) {
	  _sethae(msb);
        }
      }
    }
    *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(1<<(xf86SparseShift-2))) =
      w * 0x00010001;
    if (msb)
      _sethae(0);
}

void xf86WriteSparse32(vu32 Value, vu8 *Base, vu64 Offset)
{
    unsigned long msb = 0;

    if (xf86SparseShift != 7) { /* not JENSEN */
      if (Offset >= (1UL << 24)) {
        msb = Offset & 0xf8000000;
        Offset -= msb;
        if (msb) {
	  _sethae(msb);
        }
      }
    }
    *(vuip)((unsigned long)Base+(Offset<<xf86SparseShift)+(3<<(xf86SparseShift-2))) = Value;
    if (msb)
      _sethae(0);
}

#endif

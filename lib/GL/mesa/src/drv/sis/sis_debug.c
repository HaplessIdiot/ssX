/* $XFree86$ */

/* 
 * dump HW states, set environment variable SIS_DEBUG
 * to enable these functions 
 */

#include <fcntl.h>
#include <assert.h>

#include "sis_ctx.h"
#include "sis_mesa.h"

/* for SiS 300/630/540 */
#define MMIOLength (0x8FFF-0x8800+1)
#define MMIO3DOffset (0x8800)
#define FILE_NAME "300.dump"

char *IOBase4Debug = 0;

char *prevLockFile = NULL;
int prevLockLine = 0;

DWORD _empty[0x10000];

void
dump_agp (void *addr, int dword_count)
{
  if (!getenv ("SIS_DEBUG"))
    return;

  {
    int i;
    FILE *file = fopen ("300agp.dump", "w");

    if (file)
      {
	for (i = 0; i < dword_count; i++)
	  {
	    fprintf (file, "%f\n", *(float *) addr);
	    ((unsigned char *) addr) += 4;
	  }
	fclose (file);
      }
  }
}

void
d2f_once (GLcontext * ctx)
{
  XMesaContext xmesa = (XMesaContext) ctx->DriverCtx;
  __GLSiScontext *hwcx = (__GLSiScontext *) xmesa->private;

  static int serialNumber = -1;

  if (serialNumber == hwcx->serialNumber)
    return;
  else
    serialNumber = hwcx->serialNumber;

  d2f();
}

void
d2f (void)
{
  if (!getenv ("SIS_DEBUG"))
    return;

  /* dump 0x8800 - 0x8AFF */
  {
    int fh;
    int rval;
    void *addr = IOBase4Debug + MMIO3DOffset;

    assert (IOBase4Debug);

    if ((fh = open (FILE_NAME, O_WRONLY | O_CREAT, S_IREAD | S_IWRITE)) != -1)
      {
	rval = write (fh, addr, MMIOLength);
	assert (rval != -1);
	close (fh);
      }
  }
}

/* dump to HW */
void
d2h (char *file_name)
{
  int fh;
  int rval;
  void *addr[MMIOLength];

  if (!getenv ("SIS_DEBUG"))
    return;

  if ((fh = open (file_name, O_CREAT, S_IREAD | S_IWRITE)) != -1)
    {
      rval = read (fh, addr, MMIOLength);
      assert (rval != -1);
      close (fh);
    }
  memcpy (IOBase4Debug + MMIO3DOffset, addr, MMIOLength);

}

/* dump video memory to file  */
void
dvidmem (unsigned char *addr, int size)
{
  int fh;
  int rval;
  static char *file_name = "vidmem.dump";

  if (!getenv ("SIS_DEBUG"))
    return;

  if ((fh = open (file_name, O_WRONLY | O_CREAT, S_IREAD | S_IWRITE)) != -1)
    {
      rval = write (fh, addr, size);
      assert (rval != -1);
      close (fh);
    }
}

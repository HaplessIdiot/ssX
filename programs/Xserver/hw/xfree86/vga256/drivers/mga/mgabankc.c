/*
 */

/* $XFree86$ */

#define PSZ 8
#include "vga256.h"
#include "mgareg.h"
#include "mga.h"
void MGASetRead(int bank)
{
  outw(0x3de, ((bank & 0xff)<<8)|0x04);
}
void MGASetWrite(int bank)
{
  outw(0x3de, ((bank & 0xff)<<8)|0x04);
}
void MGASetReadWrite(int bank)
{
  outw(0x3de, ((bank & 0xff)<<8)|0x04);
}
int MGAWaitForBlitter()
{
#define max_time 10000000
  int i = max_time;
  do {
    if (!(INREG8(0x1e16) & 0x01))
      break;
  } while (--i);
  return i;
}

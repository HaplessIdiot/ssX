/* $XFree86$ */
/*
 * Written by Erik Nygren
 *
 * This code may be freely incorporated in any program without royalty, as
 * long as the copyright notice stays intact.
 *
 * ERIK NYGREN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIK NYGREN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "X.h"
#include "input.h"

#include "xf86_OSlib.h"
#include "xf86.h"

#include "p9000.h"
#include "p9000reg.h"
#include "p9000Bt485.h"

/* Virtual Terminal / VGA values to be saved and restored */
static short vga_dac_mask = 0xff;
static LUTENTRY vga_lut[256];

/* Saved font data */
static unsigned char *vga_font_buf1 = NULL;
static unsigned char *vga_font_buf2;

pointer vgaBase;
int vgaIOBase;

static void slowcpy(
#if NeedFunctionPrototypes
   unsigned char *, unsigned char *, unsigned
#endif
);


/*
 * slowcpy --
 *     Copiess memory from one location to another
 */
void slowcpy(unsigned char *dest, unsigned char *src, unsigned bytes)
{
  while (bytes-- > 0)
    {
      *(dest++) = *(src++);
    }
}


/* 
 * p9000SaveVT --
 *    Restores the LUT and other parts of the VT's state for a virtual terminal
 */
void p9000SaveVT()
{
  /* Save old values */
  p9000ReadLUT(vga_lut);
  vga_dac_mask = inb(BT_PIXEL_MASK);

#ifdef DO_FONT_RESTORE
  if (!vga_font_buf1)
    {
      vga_font_buf1 = (unsigned char *)Xcalloc(FONT_SIZE*2);
      vga_font_buf2 = vga_font_buf1 + FONT_SIZE;
    }
  /* save font data in plane 2 */
  outb(GRA_I, 0x04);
  outb(GRA_D, 0x02);
  slowcpy(vga_font_buf1, vgaBase, FONT_SIZE);

  /* save font data in plane 3 */
  outb(GRA_I, 0x04);
  outb(GRA_D, 0x03);
  slowcpy(vga_font_buf2, vgaBase, FONT_SIZE);
#endif /* DO_FONT_RESTORE */
}


/* 
 * p9000RestoreVT --
 *    Restores the LUT and other things for a virtual terminal
 */
void p9000RestoreVT()
{
  /* Restore old values */
  p9000WriteLUT(vga_lut);
  outb(BT_PIXEL_MASK,vga_dac_mask);

#ifdef DO_FONT_RESTORE
  /* disable Set/Reset register */
  outb(GRA_I, 0x01);
  outb(GRA_D, 0x00);
  
  /* restore font data from plane 2 */
  outb(GRA_I, 0x02);
  outb(GRA_D, 0x04);
  slowcpy(vgaBase, vga_font_buf1, FONT_SIZE);

  /* restore font data from plane 3 */
  outb(GRA_I, 0x02);
  outb(GRA_D, 0x08);
  slowcpy(vgaBase, vga_font_buf2, FONT_SIZE);
#endif /* DO_FONT_RESTORE */
}






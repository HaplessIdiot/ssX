/* $XFree86$ */

#include "s3v.h"

/* protos */

static void S3VLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src);
static void S3VShowCursor(ScrnInfoPtr pScrn);
static void S3VHideCursor(ScrnInfoPtr pScrn);
static void S3VSetCursorPosition(ScrnInfoPtr pScrn, int x, int y);
static void S3VSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg);

/*
 * implementation
 */
 
#define DACREGSIZE 0x50
    
/*
 * Only change bits shown in this mask.  Ideally reserved bits should be
 * zeroed here.  Also, don't change the vgaioen bit here since it is
 * controlled elsewhere.
 *
 * XXX These settings need to be checked.
 */
#define OPTION1_MASK	0xFFFFFEFF
#define OPTION2_MASK	0xFFFFFFFF


/*
 * Read/write to the DAC via MMIO 
 */

/*
 * These were functions.  Use macros instead to avoid the need to
 * pass pMga to them.
 */

#define inCRReg(reg) (VGAHWPTR(pScrn))->readCrtc( VGAHWPTR(pScrn), reg )
#define outCRReg(reg, val) (VGAHWPTR(pScrn))->writeCrtc( VGAHWPTR(pScrn), reg, val )



/****
 ***  HW Cursor
 */
static void
S3VLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
  S3VPtr ps3v = S3VPTR(pScrn);
  CARD32 *dst = (CARD32*)(ps3v->FBCursorStart);
  int i = 128;
  
    /*PVERB5("	S3VLoadCursorImage\n");*/
    
    /* swap words */
    while( i-- ) {
        *dst++ = (src[0] << 16 ) | (src[1] << 24) | (src[2] ) | (src[3] << 8);
        *dst++ = (src[4] << 16 ) | (src[5] << 24) | (src[6] ) | (src[7] << 8);
        src += 8;
    }

}


static void 
S3VShowCursor(ScrnInfoPtr pScrn)
{
  char tmp;

  tmp = inCRReg(EXT_RAMDAC_CNTL_CR55);
  /* Enable X11 decoding */
  outCRReg(EXT_RAMDAC_CNTL_CR55, tmp | 0x10 );

  tmp = inCRReg(HWCURSOR_MODE_CR45);
    /* Enable cursor */
  outCRReg(HWCURSOR_MODE_CR45, tmp | 1 );
}


static void
S3VHideCursor(ScrnInfoPtr pScrn)
{
  char tmp;
  
  tmp = inCRReg(HWCURSOR_MODE_CR45);
   /* Disable cursor */
  outCRReg(HWCURSOR_MODE_CR45, tmp & ~1 );
}


static void
S3VSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
   unsigned char xoff, yoff;

   /*
   if (!xf86VTSema)
      return;
    */

   /*
   x -= pScrn->frameX0;
   y -= pScrn->frameY0;
   */

   /*
   x -= s3vHotX;
   y -= s3vHotY;
    */

   if (pScrn->bitsPerPixel > 16)
      x &= ~1;

   /*
    * Make these even when used.  There is a bug/feature on at least
    * some chipsets that causes a "shadow" of the cursor in interlaced
    * mode.  Making this even seems to have no visible effect, so just
    * do it for the generic case.
    */
   if (x < 0) {
     xoff = ((-x) & 0xFE);
     x = 0;
   } else {
     xoff = 0;
   }

   if (y < 0) {
      yoff = ((-y) & 0xFE);
      y = 0;
   } else {
      yoff = 0;
   }

   /* WaitIdle(); */

   /* This is the recomended order to move the cursor */

   outCRReg( 0x46, (x & 0xff00)>>8 );
   outCRReg( 0x47, (x & 0xff) );
   outCRReg( 0x49, (y & 0xff) );
   outCRReg( 0x4e, xoff );
   outCRReg( 0x4f, yoff );
   outCRReg( 0x48, (y & 0xff00)>>8 );
}


static void
S3VSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    S3VPtr ps3v = S3VPTR(pScrn);

    /*PVERB5("	S3VSetCursorColors\n");*/

	switch( pScrn->bitsPerPixel) {
	  case 8:
	  			/* Init set depth 8 as not truecolor, so fg & bg are */
	  			/* already properly adjusted. */
	    break;
	  case 16:
				/* adjust colors to 16 bits */
		if (pScrn->weight.green == 5 && ps3v->Chipset != S3_ViRGE_VX) {
		  fg = ((fg & 0xf80000) >> 9) |
			((fg & 0xf800) >> 6) |
			((fg & 0xf8) >> 3);
		  bg = ((bg & 0xf80000) >> 9) |
			((bg & 0xf800) >> 6) |
			((bg & 0xf8) >> 3);
		  
		} else {
		  fg = ((fg & 0xf80000) >> 8) |
			((fg & 0xfc00) >> 5) |
			((fg & 0xf8) >> 3);
		  bg = ((bg & 0xf80000) >> 8) |
			((bg & 0xfc00) >> 5) |
			((bg & 0xf8) >> 3);
		}
			
	    break;

	  case 24:
	  case 32:
			/* Do it straight, full 24 bit color. */
        break;
    }
      
	/* Reset the cursor color stack pointer */
    inCRReg( 0x45 );
    /* Write low, mid, high bytes - foreground */
    outCRReg( 0x4a, (fg & 0x000000FF));
    outCRReg( 0x4a, (fg & 0x0000FF00) >> 8);
    outCRReg( 0x4a, (fg & 0x00FF0000) >> 16);
	/* Reset the cursor color stack pointer */
    inCRReg( 0x45 );
    /* Write low, mid, high bytes - background */
    outCRReg( 0x4b, (bg & 0x000000FF));
    outCRReg( 0x4b, (bg & 0x0000FF00) >> 8);
    outCRReg( 0x4b, (bg & 0x00FF0000) >> 16);
}


Bool 
S3VHWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    S3VPtr ps3v = S3VPTR(pScrn);
    xf86CursorInfoPtr infoPtr;

    PVERB5("	S3VHWCursorInit\n");

    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) return FALSE;
    
    ps3v->CursorInfoRec = infoPtr;

    infoPtr->MaxWidth = 64;
    infoPtr->MaxHeight = 64;
    infoPtr->Flags = 
    				HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_16 |
        			HARDWARE_CURSOR_BIT_ORDER_MSBFIRST;
/*    				HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 |*/
/*    				HARDWARE_CURSOR_TRUECOLOR_AT_8BPP;*/
/*				HARDWARE_CURSOR_SOURCE_MASK_NOT_INTERLEAVED;*/

    infoPtr->SetCursorColors = S3VSetCursorColors;
    infoPtr->SetCursorPosition = S3VSetCursorPosition;
    infoPtr->LoadCursorImage = S3VLoadCursorImage;
    infoPtr->HideCursor = S3VHideCursor;
    infoPtr->ShowCursor = S3VShowCursor;
    infoPtr->UseHWCursor = NULL;

    return(xf86InitCursor(pScreen, infoPtr));
}

/*EOF*/

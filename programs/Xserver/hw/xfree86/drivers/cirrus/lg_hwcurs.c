/*
 * Hardware cursor support for CL-GD546x -- The Laugna family
 *
 * lg_hwcurs.c
 *
 * (c) 1998 Corin Anderson.
 *          corina@the4cs.com
 *          Tukwila, WA
 *
 * Much of this code is inspired by the HW cursor code from XFree86
 * 3.3.3.
 */
/* $XFree86$ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "lg.h"
#include "lg_xaa.h" /* For BitBLT engine macros */

/*
#define LG_CURSOR_DEBUG
*/

#define CURSORWIDTH	64
#define CURSORHEIGHT	64
#define CURSORSIZE      (CURSORWIDTH*CURSORHEIGHT/8)

/* Some registers used for the HW cursor. */
enum {
  PALETTE_READ_ADDR  = 0x00A4,
  PALETTE_WRITE_ADDR = 0x00A8,
  PALETTE_DATA       = 0x00AC,

  PALETTE_STATE      = 0x00B0,
  CURSOR_X_POS       = 0x00E0,
  CURSOR_Y_POS       = 0x00E2,
  CURSOR_PRESET      = 0x00E4,
  MISC_CONTROL       = 0x00E6,
  CURSOR_ADDR        = 0x00E8
};
  

static void
LgFindCursorTile(ScrnInfoPtr pScrn, int *x, int *y, int *width, int *height,
		 CARD32 *curAddr);


/*
 * Set the FG and BG colors of the HW cursor.
 */
static void LgSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg) {
  LgPtr pLg = LGPTR(pScrn);

  volatile CARD8 *pPaletteState = (CARD8 *)(pLg->IOBase + PALETTE_STATE);
  volatile CARD8 *pPaletteAddr = (CARD8 *)(pLg->IOBase + PALETTE_WRITE_ADDR);
  volatile CARD8 *pPaletteData = (CARD8 *)(pLg->IOBase + PALETTE_DATA);
  
  

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgSetCursorColors\n");
#endif

  /* Enable access to cursor colors in palette */
  *pPaletteState |= (1<<3);

  /* Slam in the color */
  *pPaletteAddr = 0x00;
  *pPaletteData = (bg >> 16); /* The 5446 & 5480 code shifted an extra 2 */
  *pPaletteData = (bg >>  8); /* positions right, and masked everything */
  *pPaletteData = (bg >>  0); /* with 0x3F.  Why? */
  *pPaletteAddr = 0x0F;
  *pPaletteData = (fg >> 16);
  *pPaletteData = (fg >>  8);
  *pPaletteData = (fg >>  0);

  /* Disable access to cursor colors */
  *pPaletteState &= ~(1<<3);
}


/*
 * Set the (x,y) position of the pointer.
 *
 * Note:  (x,y) are /frame/ relative, not /framebuffer/ relative.
 * That is, if the virtual desktop has been panned all the way to 
 * the right, and the pointer is to be in the upper-right hand corner 
 * of the viewable screen, the pointer coords are (0,0) (even though
 * the pointer is on, say (550,0) wrt the frame buffer).  This is, of
 * course, a /good/ thing -- we don't want to have to deal with where
 * the virtual display is, etc, in the cursor code. 
 *
 */
static void LgSetCursorPosition(ScrnInfoPtr pScrn, int x, int y) {
  LgPtr pLg = LGPTR(pScrn);
  volatile CARD16 *pXPos = (CARD16 *)(pLg->IOBase + CURSOR_X_POS);
  volatile CARD16 *pYPos = (CARD16 *)(pLg->IOBase + CURSOR_Y_POS);
  volatile CARD16 *pPreset = (CARD16 *)(pLg->IOBase + CURSOR_PRESET);

#if 0
#ifdef LG_CURSOR_DEBUG
  ErrorF("LgSetCursorPosition %d %d\n", x, y);
#endif
#endif

  if (x < 0 || y < 0) {
    CARD16 oldPreset = *pPreset;
    CARD16 newPreset = 0x8080 & oldPreset; /* Reserved bits */

    if (x < 0) {
      newPreset |= ((-x & 0x7F) << 8);
      x = 0;
    }

    if (y < 0) {
      newPreset |= ((-y & 0x7F) << 0);
      y = 0;
    }

    *pPreset = newPreset;
    pLg->CursorIsSkewed = TRUE;

  } else if (pLg->CursorIsSkewed) {

    /* Reset the hotspot location. */
    *pPreset &= 0x8080;
    pLg->CursorIsSkewed = FALSE;
  }

  /* Commit the new position to the card. */
  *pXPos = x;
  *pYPos = y;
}


/*
 * Load the cursor image to the card.  The cursor image is given in
 * bits.  The format is:  ???
 */
static void LgLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *bits) {
  LgPtr pLg = LGPTR(pScrn);
  volatile CARD32 *pXCursorBits = (CARD32 *)bits;

  volatile CARD32 *pHOSTDATA = (CARD32 *)(pLg->IOBase + HOSTDATA);
  volatile CARD8 *pQFREE = (CARD8 *)(pLg->IOBase + QFREE);
  int l, w;

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgLoadCursorImage\n");
#endif

  /* All ("all") we have to do is a simple CPU-to-screen copy of the
     cursor image to the frame buffer. */

  while (!LgREADY())
    ;

  /* Wait until there's ample room in the chip's queue */
  while (*pQFREE < 10)
    ;

  LgSETMODE(HOST2SCR);       /* Host-to-screen blit */
  LgSETROP(0x00CC);          /* Source copy */

  /* First, copy our transparent cursor image to the next 1/2 tile boundry */
  /* Destination */
  LgSETMDSTXY(pLg->HWCursorImageX+pLg->HWCursorTileWidth, pLg->HWCursorImageY);

  /* Set the source pitch.  0 means that, worst case, the source is 
     alligned only on a byte boundry */
  LgSETMPHASE1(0);

  LgSETMEXTENTSNOMONOQW(pLg->HWCursorTileWidth, pLg->HWCursorTileHeight);

  for (l = 0; l < CURSORHEIGHT; l++) {
    /* Plane 0 */
    for (w = 0; w < CURSORWIDTH >> 5; w++) 
      *pHOSTDATA = 0x00000000;
    /* Plane 1 */
    for (w = 0; w < CURSORWIDTH >> 5; w++) 
      *pHOSTDATA = 0x00000000;
  }

  /* Now, copy the real cursor image */
    
  /* Set the destination */
  LgSETMDSTXY(pLg->HWCursorImageX, pLg->HWCursorImageY);

  /* Set the source pitch.  0 means that, worst case, the source is 
     alligned only on a byte boundry */
  LgSETMPHASE1(0);

  /* Always copy an entire cursor image to the card. */
  LgSETMEXTENTSNOMONOQW(pLg->HWCursorTileWidth, pLg->HWCursorTileHeight);

  for (l = 0; l < CURSORHEIGHT; l++) {
    /* Plane 0 */
    for (w = 0; w < CURSORWIDTH >> 5; w++) 
      *pHOSTDATA = *pXCursorBits++;
    /* Plane 1 */
    for (w = 0; w < CURSORWIDTH >> 5; w++) 
      *pHOSTDATA = *pXCursorBits++;
  }

  while (!LgREADY())
    ;
}



/*
 * LgFindCursorTile() finds the tile of display memory that will be
 * used to load the pointer image into.  The tile chosen will be the
 * last tile in the last line of the frame buffer. 
 */
static void
LgFindCursorTile(ScrnInfoPtr pScrn, int *x, int *y, int *width, int *height,
		 CARD32 *curAddr) {
  LgPtr pLg = LGPTR(pScrn);

  int videoRam = pScrn->videoRam; /* in K */
  int tileHeight = LgLineData[pLg->lineDataIndex].width?8:16;
  int tileWidth = LgLineData[pLg->lineDataIndex].width?256:128;
  int tilesPerLine = LgLineData[pLg->lineDataIndex].tilesPerLine;
  int filledOutTileLines, leftoverMem;
  int yTile, xTile;
  int tileNumber;
    
  filledOutTileLines = videoRam / (tilesPerLine * 2); /* tiles are 2K */
  leftoverMem = videoRam - filledOutTileLines*tilesPerLine*2;
    
  if (leftoverMem > 0) {
    yTile = filledOutTileLines;
  } else {
    /* There is no incomplete row of tiles.  Then just use the last
       tile in the last line */
    yTile = filledOutTileLines - 1;
  }
  xTile = 0;   /* Always use the first tile in the determined tile row */

  /* The (x,y) coords of the pointer image. */
  if (x)
    *x = xTile * tileWidth;
  if (y)
    *y = yTile * tileHeight;

  if (width)
    *width = tileWidth;
  if (height)
    *height = tileHeight / 2;

  /* Now, compute the linear address of the cursor image.  This process
     is unpleasant because the memory is tiled, and we essetially have
     to undo the tiling computation. */
  if (curAddr) {
    unsigned int nIL;  /* Interleaving */
    nIL = pLg->memInterleave==0x00? 1 : (pLg->memInterleave==0x40 ? 2 : 4);

    if (PCI_CHIP_GD5465 == pLg->Chipset) {
      /* The Where's The Cursor formula changed for the 5465.  It's really 
 	 kinda wierd now. */
      unsigned long page, bank;
      unsigned int nX, nY;
      
      nX = xTile * tileWidth;
      nY = yTile * tileHeight;
      
      page = (nY / (tileHeight * nIL)) * tilesPerLine + nX / tileWidth;
      bank = (nX/tileWidth + nY/tileHeight) % nIL + page/(512*nIL);
      page = page & 0x1FF;
      *curAddr = bank*1024*1024L + page*2048 + (nY%tileHeight)*tileWidth;
      
    } else {
       tileNumber = (tilesPerLine*nIL) * (yTile/nIL) + yTile % nIL;
       *curAddr = tileNumber * 2048;
    }
  }
}




/*
 * Hide/disable the HW cursor.
 */
void LgHideCursor(ScrnInfoPtr pScrn) {
  LgPtr pLg = LGPTR(pScrn);
  volatile CARD16 *pMiscControl = (CARD16 *)(pLg->IOBase + MISC_CONTROL);

  /* To hide the cursor, we kick it off into the corner, and then set the
     cursor image to be a transparent bitmap.  That way, if X continues
     to move the cursor while it is hidden, there is no way that the user
     can move the cursor back on screen!

     We don't just clear the cursor enable bit because doesn't work in some
     cases (like when switching back to text mode).
     */
  
#ifdef LG_CURSOR_DEBUG
  ErrorF("LgHideCursor\n");
#endif

  *pMiscControl &= 0xFFFE;
}

void LgShowCursor(ScrnInfoPtr pScrn) {
  LgPtr pLg = LGPTR(pScrn);
  volatile CARD16 *pMiscControl = (CARD16 *)(pLg->IOBase + MISC_CONTROL);
  volatile CARD16 *pCursorAddr = (CARD16 *)(pLg->IOBase + CURSOR_ADDR);

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgShowCursor\n");
#endif

  *pMiscControl |= (1<<0);
  *pCursorAddr = pLg->HWCursorAddr & 0x7FFC;
}


/*
 * Can the HW cursor be used?
 */
static Bool LgUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs) {
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgUseHWCursor\n");
#endif

  if(pScrn->bitsPerPixel < 8)
    return FALSE;

  return TRUE;
}


/*
 * Initialize all the fun HW cursor code.
 */
Bool LgHWCursorInit(ScreenPtr pScreen) {
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  LgPtr pLg = LGPTR(pScrn);
  xf86CursorInfoPtr infoPtr;

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgHWCursorInit\n");
#endif

  infoPtr = xf86CreateCursorInfoRec();
  if(!infoPtr) return FALSE;
  
  pLg->CursorInfoRec = infoPtr;
  LgFindCursorTile(pScrn, 
		   &pLg->HWCursorImageX, &pLg->HWCursorImageY,
		   &pLg->HWCursorTileWidth, &pLg->HWCursorTileHeight,
		   &pLg->HWCursorAddr);
  /* Keep only bits 22:10 of the address. */
  pLg->HWCursorAddr = (pLg->HWCursorAddr >> 8) & 0x7FFC;
  
  pLg->CursorIsSkewed = FALSE;

  infoPtr->MaxWidth = CURSORWIDTH;
  infoPtr->MaxHeight = CURSORHEIGHT;
  infoPtr->Flags = 
    HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
    HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
    HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64;
  infoPtr->SetCursorColors = LgSetCursorColors;
  infoPtr->SetCursorPosition = LgSetCursorPosition;
  infoPtr->LoadCursorImage = LgLoadCursorImage;
  infoPtr->HideCursor = LgHideCursor;
  infoPtr->ShowCursor = LgShowCursor;
  infoPtr->UseHWCursor = LgUseHWCursor;
  /* RealizeCursor is how the cursor bits are converted from an infoPtr
     to the bits to slam out to the card.  Can we use one of the 
     pre-defined realize'rs?  Look in ramdac/xf86HWCurs.c. */
  infoPtr->RealizeCursor = NULL;

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgHWCursorInit before xf86InitCursor\n");
#endif

  return(xf86InitCursor(pScreen, infoPtr));
}

#ifndef __DGAPROC_H
#define __DGAPROC_H


/********* This section belongs in the client header file **********/

#define DGA_CONCURRENT_ACCESS	0x00000001
#define DGA_FILL_RECT		0x00000002
#define DGA_BLIT_RECT		0x00000004
#define DGA_BLIT_RECT_TRANS	0x00000008
#define DGA_PIXMAP_AVAILABLE	0x00000010

#define DGA_INTERLACED		0x00010000
#define DGA_DOUBLESCAN		0x00020000

#define DGA_FLIP_IMMEDIATE	0x00000001
#define DGA_FLIP_RETRACE	0x00000002

#define DGA_COMPLETED		0x00000000
#define DGA_PENDING		0x00000001

typedef struct {
   int num;		/* A unique identifier for the mode (num > 0) */
   char *name;		/* name of mode given in the XF86Config */
   float verticalRefresh;
   int flags;		/* DGA_CONCURRENT_ACCESS, etc... */
   int imageWidth;	/* linear accessible portion (pixels) */
   int imageHeight;
   int pixmapWidth;	/* Xlib accessible portion (pixels) */
   int pixmapHeight;	/* both fields ignored if no concurrent access */
   int bytesPerScanline; 
   int byteOrder;	/* MSBFirst, LSBFirst */
   int depth;		
   int bitsPerPixel;
   unsigned long red_mask;
   unsigned long green_mask;
   unsigned long blue_mask;
   int viewportWidth;
   int viewportHeight;
   int xViewportStep;	/* viewport position granularity */
   int yViewportStep;
   int maxViewportX;	/* max viewport origin */
   int maxViewportY;
   int viewportFlags;	/* types of page flipping possible */
   int reserved1;
   int reserved2;
} XDGAModeRec, *XDGAModePtr;


typedef struct {
   XDGAModeRec mode;
   unsigned char *data;
   PixmapPtr pPix;
} XDGADeviceRec, *XDGADevicePtr;


/*********************************************************************/

void XFree86DGAExtensionInit(void);

/* DDX interface */

int
DGASetMode(
   int index,
   int num,
   XDGADevicePtr device
);

void 
DGASelectInput(
   int index,
   long mask
);

Bool DGAAvailable(int index);
int DGAGetInput(int index);
void DGAShutdown(void);
void DGAInstallColormap(ColormapPtr cmap);
int DGAGetViewportStatus(int index, int flags); 
int DGAFlush(int index);

int
DGAFillRect(
   int index,
   int x, int y, int w, int h,
   unsigned long color
);

int
DGABlitRect(
   int index,
   int srcx, int srcy, 
   int w, int h, 
   int dstx, int dsty
);

int
DGABlitTransRect(
   int index,
   int srcx, int srcy, 
   int w, int h, 
   int dstx, int dsty,
   unsigned long color
);

int
DGASetViewport(
   int index,
   int x, int y,
   int mode
); 

int DGAGetModes(int index);
int DGAGetOldDGAMode(int index);

int DGAGetDeviceInfo(int index, XDGADevicePtr dev, int mode);

Bool DGAVTSwitch(int index);
Bool DGAStealEvent(int index, xEvent *event);



#endif /* __DGAPROC_H */

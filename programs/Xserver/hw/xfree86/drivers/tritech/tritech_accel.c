#include "vga256.h"
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
/* #include "xf86_OSlib.h" */
#include "xf86_HWlib.h"
#include "xf86Version.h"
#include "vga.h"
#include "../../xaa/xf86xaa.h"
#include "xf86_PCI.h"
#include "vgaPCI.h"

#include "tritech.h"

static void TRITECHSync();
static void TRITECHSetupForFillRectSolid();
static void TRITECHSubsequentFillRectSolid();
static int CurrentRop = -1;

void TRITECHAccelInit(void)
{   
    xf86AccelInfoRec.Flags = 	BACKGROUND_OPERATIONS | 
			     	COP_FRAMEBUFFER_CONCURRENCY |
				DELAYED_SYNC;

    xf86AccelInfoRec.Sync = TRITECHSync;

    xf86GCInfoRec.PolyFillRectSolidFlags =  NO_PLANEMASK;
    xf86AccelInfoRec.SetupForFillRectSolid = TRITECHSetupForFillRectSolid;
    xf86AccelInfoRec.SubsequentFillRectSolid = TRITECHSubsequentFillRectSolid;
}


	/************************\
	|    Shade Programs      |
	\************************/

/* We follow the convention:
	Coef0 = fgcolor
	Coef1 = full planemask
	Coef2 = planemask (not supported yet)
*/

void P3D_GXclear()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_COEF1 | P3D_AADDR_COEF1 | P3D_END | (13 << 14));
}

void P3D_GXand()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_COEF0 | P3D_END); 
}


void P3D_GXandReverse()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_COEF0 | P3D_END | (1 << 14)); 
}


void P3D_GXcopy()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_COEF1 | P3D_AADDR_COEF0 | P3D_END); 
}


void P3D_GXandInverted()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_COEF0 | P3D_END | (1 << 14)); 
}


void P3D_GXnoop()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_TMP1 | P3D_END); 
}


void P3D_GXxor()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_COEF0 | P3D_END | (4 << 14)); 
}


void P3D_GXor()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_COEF0 | P3D_END | (19 << 14)); 
}


void P3D_GXnor()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_COEF0 | P3D_END | (16 << 14)); 
}


void P3D_GXequiv()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_LOGIC_OP | P3D_DEST_TMP1 | 
		P3D_BADDR_COEF1 | P3D_AADDR_COEF0 | (2 << 14)); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_CREAD | P3D_DEST_TMP2); 
    OUTREG(P3D_SHADE_PROG_MEM + 8, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_TMP2 | P3D_END | (4 << 14)); 
}


void P3D_GXinvert()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_COEF0 | P3D_END | (1 << 14)); 
}


void P3D_GXorReverse()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_COEF0 | P3D_END | (18 << 14));
}


void P3D_GXcopyInverted()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_COEF1 | P3D_AADDR_COEF0 | P3D_END | (2 << 14)); 
}


void P3D_GXorInverted()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_COEF0 | P3D_END | (17 << 14)); 
}


void P3D_GXnand()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_CREAD | P3D_DEST_TMP1); 
    OUTREG(P3D_SHADE_PROG_MEM + 4, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_TMP1 | P3D_AADDR_COEF0 | P3D_END | (16 << 14)); 
}


void P3D_GXset()
{
    OUTREG(P3D_SHADE_PROG_MEM, P3D_LOGIC_OP | P3D_DEST_FB | 
		P3D_BADDR_COEF1 | P3D_AADDR_COEF1 | P3D_END);
}



void (*P3DRasterOps[16])() = {
  P3D_GXclear		/* 0 */,
  P3D_GXand 		/* src & dst */,
  P3D_GXandReverse	/* src & !dst */,
  P3D_GXcopy 		/* src */,
  P3D_GXandInverted	/* !src & dst */,
  P3D_GXnoop 		/* dst */,
  P3D_GXxor 		/* src ^ dst */,
  P3D_GXor		/* (src | dst) = !(!src & !dst)*/,
  P3D_GXnor		/* (!src & !dst) = !(src | dst) */,
  P3D_GXequiv 		/* !src ^ dst */,
  P3D_GXinvert		/* !dst */,
  P3D_GXorReverse	/* (src | !dst) = !(!src & dst) */,
  P3D_GXcopyInverted	/* !src */,
  P3D_GXorInverted	/* (!src | dst)  = !(src & !dst) */,
  P3D_GXnand 		/* (!src | !dst) = !(src & dst) */,
  P3D_GXset             /* 1 */
};




/* This is the accelerator initialization part of the Init code.
	We stick this here just to keep all graphics engine writes
	in the same place */

void TRITECHInitializeAccelerator()
{
   WAIT_IDLE

   OUTREG(P3D_RASTER_EXT, 1);  /* Soft reset primitive processor */
   OUTREG(P3D_BASE_ADDR, 0); 
   OUTREG(P3D_EDGE_ORDER, 0x50); /* Rectangle Edge Order */
   OUTREG(P3D_EDGE0_DY, 0); 
   OUTREG(P3D_EDGE0_DX, 8); 
   OUTREG(P3D_EDGE0_INIT, 0); 
   OUTREG(P3D_EDGE1_DY, 0); 
   OUTREG(P3D_EDGE1_DX, -8); 
   OUTREG(P3D_GRID_REG, (vga256InfoRec.virtualY + 31) >> 5); 
   OUTREG(P3D_PPU_CODE_START, 0); 
   OUTREG(P3D_COEF_REG1, 0xFFFFFF);
   OUTREG(P3D_FRAME_MODE, 0); 
   OUTREG(P3D_PPU_MODE, 0); 
   P3D_GXcopy();
   CurrentRop = GXcopy;
}

void TRITECHSync(void)
{
    WAIT_IDLE
}


	/*****************************\
	|    Solid Filled  Rects      |
	\*****************************/


void TRITECHSetupForFillRectSolid(color, rop, planemask)
int color, rop;
unsigned planemask;
{
    WAIT_IDLE

    if(xf86bpp == 32) {
	OUTREG(P3D_COEF_REG0, color & 0xFFFFFF);
    } else { 	/* 16bpp */
	OUTREG(P3D_COEF_REG0, CONVERT_TO_888(color) & 0xFFFFFF);
    }

    /* load shading program */
    if(CurrentRop != rop) {
	(*P3DRasterOps[rop])();
	CurrentRop = rop;
    }
    OUTREG(P3D_EDGE_ORDER, 0x50); 
}

void TRITECHSubsequentFillRectSolid(x, y, w, h)
int x, y, w, h;
{
   WAIT_IDLE

   OUTREG(P3D_Y_INIT, y); 
   OUTREG(P3D_X_INIT, x); 
   OUTREG(P3D_EDGE1_INIT, w << 6);	/* why ? */ 
   OUTREG(P3D_Y_END, y + h); 
}

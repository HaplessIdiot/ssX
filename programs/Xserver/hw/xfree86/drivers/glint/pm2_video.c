/*
 *  Permedia 2 Xv Driver 
 *  Copyright (C) 1998-1999 Michael Schimek
 */

/* $XFree86$ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xf86fbman.h"
#include "xf86i2c.h"
#include "xf86xv.h"
#include "Xv.h"

#include "glint_regs.h"
#include "glint.h"

#define PCI_SUBSYSTEM_ID_WINNER_2000_A		0x0a311048
#define PCI_SUBSYSTEM_ID_GLORIA_SYNERGY_A	0x0a321048
#define PCI_SUBSYSTEM_ID_GLORIA_SYNERGY_B	0x0a351048
#define PCI_SUBSYSTEM_ID_WINNER_2000_B		0x0a441048

#define SAA7111_SLAVE_ADDRESS 0x48
#define SAA7125_SLAVE_ADDRESS 0x88

typedef struct {
    CARD32			xy, wh;			/* 16.0 16.0 dw */
    CARD32			u, v;			/* 12.20 sfp */
} CookieRec, *CookiePtr;

typedef struct _PortPrivRec {
    struct _AdaptorPrivRec *    pAdaptor;
    I2CDevRec                   I2CDev;
    int                         Plug;
    INT32			Attribute[8];		/* Brig, Con, Sat, Hue, Int, Filt */
    FBAreaPtr			pFBArea[2];
    RegionRec	                Region;
    CARD32			BufferBase[2];		/* x1 */
    CARD32			BufferStride;		/* x1 */
    CARD32			BufferPProd;		/* PProd(BufferStride / 2) */
    short			vx, vy, vw, vh;
    short			dx, dy, dw, dh;
    CookiePtr			pCookies;
    int				nCookies;
    CARD32			dS, dT;			/* 12.20 sfp */
    short			fw, fh;
    int				APO;
    unsigned int		Frames, FrameAcc;
    int				VideoOn;		/* No, Once, Yes */
    Bool			StreamOn;
    Bool			BlackOut;
} PortPrivRec, *PortPrivPtr;

typedef struct _AdaptorPrivRec {
    struct _AdaptorPrivRec *	Next;
    ScrnInfoPtr			pScrn;
    void *			VDID;
    CARD32			FifoControl;
    OsTimerPtr			Timer;
    int                         VideoStd;
    unsigned int		FramesPerSec;
    int				FrameLines;
    int				IntLine;		/* frame */
    int				LinePer;		/* nsec */
    PortPrivRec                 Port[2];
} AdaptorPrivRec, *AdaptorPrivPtr;

static AdaptorPrivPtr AdaptorPrivList = NULL;

#define PORTNUM(p) (((p) == &pAPriv->Port[0]) ? 0 : 1)
#define BPPSHIFT(g) (2 - (g)->BppShift)			/* BytesPerPixel = 1 << BPPSHIFT(pGlint) */

#define COLORBARS
#define DEBUG(x) 

/* forward */
static CARD32 TimerCallback(OsTimerPtr pTim, CARD32 now, pointer p);
static void DelayedStopVideo(PortPrivPtr pPPriv);

#define XV_ENCODING	"XV_ENCODING"
#define XV_BRIGHTNESS  	"XV_BRIGHTNESS"
#define XV_CONTRAST 	"XV_CONTRAST"
#define XV_SATURATION  	"XV_SATURATION"
#define XV_HUE		"XV_HUE"
/* proprietary */
#define XV_INTERLACE	"XV_INTERLACE"			/* Boolean */
#define XV_FILTER	"XV_FILTER"			/* Boolean */

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvEncoding, xvBrightness, xvContrast, xvSaturation, xvHue;
static Atom xvInterlace, xvFilter;

static XF86VideoEncodingRec
InputVideoEncodings[] =
{
    { 0, "pal-composite",		704, 576, { 1, 50 }},
    { 1, "pal-composite_adaptor",	704, 576, { 1, 50 }},
    { 2, "pal-svideo",			704, 576, { 1, 50 }},
    { 3, "ntsc-composite",		704, 480, { 1001, 60000 }},
    { 4, "ntsc-composite_adaptor",	704, 480, { 1001, 60000 }},
    { 5, "ntsc-svideo",			704, 480, { 1001, 60000 }},
    { 6, "secam-composite",		704, 576, { 1, 50 }},
    { 7, "secam-composite_adaptor",	704, 576, { 1, 50 }},
    { 8, "secam-svideo",		704, 576, { 1, 50 }},
};

static XF86VideoEncodingRec
OutputVideoEncodings[] =
{
    { 1, "pal-composite_adaptor",	704, 576, { 1, 50 }},
    { 2, "pal-svideo",			704, 576, { 1, 50 }},
    { 4, "ntsc-composite_adaptor",	704, 480, { 1001, 60000 }},
    { 5, "ntsc-svideo",			704, 480, { 1001, 60000 }},
};

static XF86VideoFormatRec
InputVideoFormats[] = {
    { 8,  TrueColor }, /* dithered */
    { 15, TrueColor },
    { 16, TrueColor },
    { 24, TrueColor },
};

static XF86VideoFormatRec
OutputVideoFormats[] = {
    { 8,  TrueColor },
    { 8,  PseudoColor }, /* using .. */
    { 8,  StaticColor },
    { 8,  GrayScale },
    { 8,  StaticGray }, /* .. TexelLUT */
    { 15, TrueColor },
    { 16, TrueColor },
    { 24, TrueColor },
};

static I2CByte
DecInitVec[] =
{
    0x11, 0x00,
    0x02, 0xC1, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00,
    0x06, 0x00, 0x07, 0x00, 0x08, 0x00, 0x09, 0x4A,
    0x0A, 0x80, 0x0B, 0x40, 0x0C, 0x40, 0x0D, 0x00,
    0x0E, 0x01, 0x10, 0xC8, 0x12, 0x20,
    0x13, 0x00, 0x15, 0x00, 0x16, 0x00, 0x17, 0x00,
};

static I2CByte
EncInitVec[] =
{
    0x3A, 0x83, 0x61, 0xC2,
    0x5A, 119,  0x5B, 0x7D,
    0x5C, 0xAF, 0x5D, 0x3C, 0x5E, 0x3F, 0x5F, 0x3F,
    0x60, 0x70, 0x62, 0x4B, 0x67, 0x00,
    0x68, 0x00, 0x69, 0x00, 0x6A, 0x00, 0x6B, 0x20,
    0x6C, 0x03, 0x6D, 0x30, 0x6E, 0xA0, 0x6F, 0x00,
    0x70, 0x80, 0x71, 0xE8, 0x72, 0x10,
    0x7A, 0x13, 0x7B, 0xFB, 0x7C, 0x00, 0x7D, 0x00,
};

static I2CByte Dec02[3] = { 0xC1, 0xC0, 0xC4 };
static I2CByte Dec09[3] = { 0x4A, 0x4A, 0xCA };
static I2CByte Enc3A[3] = { 0x03, 0x03, 0x23 };
static I2CByte Enc61[3] = { 0x06, 0x01, 0xC2 };

static I2CByte
DecVS[3][8] =
{
    { 0x06, 108, 0x07, 108, 0x08, 0x09, 0x0E, 0x01 },
    { 0x06, 107, 0x07, 107, 0x08, 0x49, 0x0E, 0x31 },
    { 0x06, 108, 0x07, 108, 0x08, 0x01, 0x0E, 0x51 }
};

#define FSC(n) ((CARD32)((n) / 27e6 * 4294967296.0 + .5))
#define SUBCARRIER_FREQ_PAL  (4.433619e6)
#define SUBCARRIER_FREQ_NTSC (3.579545e6)

static I2CByte
EncVS[2][14] =
{
    { 0x62, 0x4B, 0x6B, 0x28, 0x6E, 0xA0,
      0x63, (I2CByte)(FSC(SUBCARRIER_FREQ_PAL) >> 0),
      0x64, (I2CByte)(FSC(SUBCARRIER_FREQ_PAL) >> 8),
      0x65, (I2CByte)(FSC(SUBCARRIER_FREQ_PAL) >> 16),
      0x66, (I2CByte)(FSC(SUBCARRIER_FREQ_PAL) >> 24) },
    { 0x62, 0x6A, 0x6B, 0x20, 0x6E, 0x20,
      0x63, (I2CByte)(FSC(SUBCARRIER_FREQ_NTSC) >> 0),
      0x64, (I2CByte)(FSC(SUBCARRIER_FREQ_NTSC) >> 8),
      0x65, (I2CByte)(FSC(SUBCARRIER_FREQ_NTSC) >> 16),
      0x66, (I2CByte)(FSC(SUBCARRIER_FREQ_NTSC) >> 24) }
};

#undef CLAMP
#define CLAMP(value, min, max)  \
(                               \
    ((value) < (min)) ? (min) : \
    ((value) > (max)) ? (max) : \
                        (value) \
)

#undef ABS
#define ABS(n) (((n) < 0) ? -(n) : (n))

static int
SetAttr(PortPrivPtr pPPriv, int i, I2CByte s, I2CByte v)
{
	return xf86I2CWriteByte(&pPPriv->I2CDev, s, v) ?
	    Success : XvBadAlloc;
}

static Bool
SetPlug(PortPrivPtr pPPriv)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    {
	if (pPPriv == &pAPriv->Port[0]) {
	    xf86I2CWriteByte(&pPPriv->I2CDev, 0x02, Dec02[pPPriv->Plug]);
	    xf86I2CWriteByte(&pPPriv->I2CDev, 0x09, Dec09[pPPriv->Plug]);
	} else {
	    if (pPPriv->StreamOn)
		xf86I2CWriteByte(&pPPriv->I2CDev, 0x3A, Enc3A[pPPriv->Plug]);
#ifdef COLORBARS
	    else
		xf86I2CWriteByte(&pPPriv->I2CDev, 0x3A, 0x83);
#endif
	}
    }

    return TRUE;
}

static Bool
SetVideoStd(AdaptorPrivPtr pAPriv)
{
    {
	if (pAPriv->VideoStd >= 2)
	    xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x61, 0xC2);

	xf86I2CWriteVec(&pAPriv->Port[0].I2CDev, &DecVS[pAPriv->VideoStd][0], 4);

	if (pAPriv->VideoStd < 2)
	    xf86I2CWriteVec(&pAPriv->Port[1].I2CDev, &EncVS[pAPriv->VideoStd][0], 7);

	if (pAPriv->VideoStd == 1) {
	    pAPriv->FramesPerSec = 30;
	    pAPriv->FrameLines = 525;
	    pAPriv->IntLine = 513;
	    pAPriv->LinePer = 63555;
	} else {
	    pAPriv->FramesPerSec = 25;
	    pAPriv->FrameLines = 625;
	    pAPriv->IntLine = 613;
	    pAPriv->LinePer = 64000;
	}

	pAPriv->Port[0].Frames = pAPriv->FramesPerSec;
	pAPriv->Port[1].Frames = pAPriv->FramesPerSec;
    }

    return TRUE;
}

/* FIXME 2048 */

static Bool
AllocOffRec(PortPrivPtr pPPriv, int i)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int width, height;

    height = (pAPriv->VideoStd == 1) ? 512 : 608;

    if (!pPPriv->Attribute[4])
	height >>= 1;

    width = (704 << 4) / pScrn->bitsPerPixel;

    pPPriv->BufferStride = pScrn->displayWidth << BPPSHIFT(pGlint);

    if (pPPriv->pFBArea[i] != NULL) {
	if (xf86ResizeOffscreenArea(pPPriv->pFBArea[i], width, height))
	    return TRUE;

	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 2,
	    "Xv/ROB couldn't resize buffer %d,%d-%d,%d to %dx%d\n",
	    pPPriv->pFBArea[i]->box.x1, pPPriv->pFBArea[i]->box.y1,
	    pPPriv->pFBArea[i]->box.x2, pPPriv->pFBArea[i]->box.y2,
	    width, height));

	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
	    "Xv/ROB free buffer 0x%08x\n", pPPriv->BufferBase[i]));
	xf86FreeOffscreenArea(pPPriv->pFBArea[i]);
	pPPriv->pFBArea[i] = NULL;
    }

    pPPriv->pFBArea[i] = xf86AllocateOffscreenArea(pScrn->pScreen,
        width, height, 2, NULL, NULL, NULL);

    if (pPPriv->pFBArea[i] != NULL)
        return TRUE;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 2,
        "Xv/ROB couldn't allocate buffer %dx%d\n",
	width, height));

    return FALSE;
}

static Bool
AllocOffLin(PortPrivPtr pPPriv, int i)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int width, height;

    height = (pAPriv->VideoStd == 1) ? 512 : 608;
    width = pScrn->displayWidth << BPPSHIFT(pGlint);
    height = (height * (704 << 1) + width - 1) / width;
    width = pScrn->displayWidth;

    pPPriv->BufferStride = 704 << 1;

    if (pPPriv->pFBArea[i] != NULL) {
	if (xf86ResizeOffscreenArea(pPPriv->pFBArea[i], width, height))
	    return TRUE;

	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 2,
	    "Xv/ROB couldn't resize buffer %d,%d-%d,%d to %dx%d\n",
	    pPPriv->pFBArea[i]->box.x1, pPPriv->pFBArea[i]->box.y1,
	    pPPriv->pFBArea[i]->box.x2, pPPriv->pFBArea[i]->box.y2,
	    width, height));

	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
	    "Xv/ROB free buffer 0x%08x\n", pPPriv->BufferBase[i]));
	xf86FreeOffscreenArea(pPPriv->pFBArea[i]);
	pPPriv->pFBArea[i] = NULL;
    }

    pPPriv->pFBArea[i] = xf86AllocateOffscreenArea(pScrn->pScreen,
        width, height, 2, NULL, NULL, NULL);

    if (pPPriv->pFBArea[i] != NULL)
	return TRUE;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 2,
	"Xv/ROB couldn't allocate buffer %dx%d\n",
	width, height));

    return FALSE;
}

static Bool
ReallocateOffscreenBuffer(PortPrivPtr pPPriv, Bool new, Bool two)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    ScrnInfoPtr pScrn = pAPriv->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int i;

    if (!new) {
	for (i = 0; i < 2; i++)
	    if (pPPriv->pFBArea[i]) {
		DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
		    "Xv/ROB free buffer 0x%08x\n", pPPriv->BufferBase[i]));
		xf86FreeOffscreenArea(pPPriv->pFBArea[i]);
		pPPriv->pFBArea[i] = NULL;
	    }

	return FALSE;
    }

    if (pAPriv->VDID != NULL) return FALSE;

    while (1) { /* retry */
	PortPrivPtr pPPrivN;
/*
	if (AllocOffRec(pPPriv, 0)) {
	    if (!two || AllocOffRec(pPPriv, 1))
		break;
	    xf86FreeOffscreenArea(pPPriv->pFBArea[0]);
	    pPPriv->pFBArea[0] = NULL;
	}
*/
	if (AllocOffLin(pPPriv, 0)) {
	    if (two) AllocOffLin(pPPriv, 1);
	    break;
	}

	if (AllocOffRec(pPPriv, 0))
	    break;

	pPPrivN = &pAPriv->Port[1 - PORTNUM(pPPriv)];

	if (pPPrivN->VideoOn <= 0 && pPPrivN->APO >= 0)
	    DelayedStopVideo(pPPrivN);
	else
	    return FALSE;
    }

    pPPriv->BufferBase[0] = ((pPPriv->pFBArea[0]->box.y1 * pScrn->displayWidth) +
			     pPPriv->pFBArea[0]->box.x1) << BPPSHIFT(pGlint);

    if (pPPriv->pFBArea[1])
	pPPriv->BufferBase[1] = ((pPPriv->pFBArea[1]->box.y1 * pScrn->displayWidth) +
			         pPPriv->pFBArea[1]->box.x1) << BPPSHIFT(pGlint);

    pPPriv->BufferPProd = partprodPermedia[(pPPriv->BufferStride / 2) >> 5];

    pPPriv->fw = 704;
    pPPriv->fh = (pAPriv->VideoStd == 1) ? 480 : 576;
    if (!pPPriv->Attribute[4])
	pPPriv->fh >>= 1;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
	"Xv/ROB new buffer addr 0x%08x, 0x%08x str %d\n",
	pPPriv->BufferBase[0], pPPriv->BufferBase[1], pPPriv->BufferStride));

    return TRUE;
}

static void
DrawYUV(PortPrivPtr pPPriv, int i)
{
    ScrnInfoPtr pScrn = pPPriv->pAdaptor->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int nBox = REGION_NUM_RECTS(&pPPriv->Region);
    BoxPtr pBox = REGION_RECTS(&pPPriv->Region);
    int vy, vh;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
	"Xv/DrawYUV %d,%d,%d,%d -> %d,%d,%d,%d\n",
	pPPriv->vx, pPPriv->vy, pPPriv->vw, pPPriv->vh,
	pPPriv->dx, pPPriv->dy, pPPriv->dw, pPPriv->dh));

    vy = pPPriv->vy;
    vh = pPPriv->vh;

    if (!pPPriv->Attribute[4]) {
	vy >>= 1;
	vh >>= 1;
    }

    pPPriv->dS = (pPPriv->vw * (1 << 20)) / pPPriv->dw;
    pPPriv->dT = (vh * (1 << 20)) / pPPriv->dh;

    /* Setup */

    GLINT_WAIT(25);

    CHECKCLIPPING;

    switch (pScrn->depth) {
	case 8:
	    GLINT_WRITE_REG((0 << 10) | 	/* BGR */
			    (1 << 1) | 		/* Dither */
			    ((5 & 0x10)<<12) |
			    ((5 & 0x0F)<<2) | 	/* 3:3:2f */
			    UNIT_ENABLE, DitherMode);
	    break;
	case 15:
	    GLINT_WRITE_REG((1 << 10) | 	/* RGB */
			    ((1 & 0x10)<<12)|
			    ((1 & 0x0F)<<2) | 	/* 5:5:5:1f */
			    UNIT_ENABLE, DitherMode);
	    break;
	case 16:
	    GLINT_WRITE_REG((1 << 10) | 	/* RGB */
			    ((16 & 0x10)<<12)|
			    ((16 & 0x0F)<<2) | 	/* 5:6:5f */
			    UNIT_ENABLE, DitherMode);
	    break;
	case 24:
	    GLINT_WRITE_REG((1 << 10) |		/* RGB */
			    ((0 & 0x10)<<12)|
			    ((0 & 0x0F)<<2) | 	/* 8:8:8:8 */
			    UNIT_ENABLE, DitherMode);
	    break;
	default:
	    return; /* Oops */
    }

    GLINT_WRITE_REG(1 << 16, dY);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(UNIT_DISABLE, AreaStippleMode);
    if (!pPPriv->pFBArea[1]) i = 0;
    GLINT_WRITE_REG(pPPriv->BufferBase[i] / 2, PMTextureBaseAddress);
    GLINT_WRITE_REG((1 << 19) /* 16 */ | pPPriv->BufferPProd,
		    PMTextureMapFormat);
    GLINT_WRITE_REG((1 << 4) /* No alpha */ |
	            ((19 & 0x10) << 2) |
		    ((19 & 0x0F) << 0) /* YUYV */,
		    PMTextureDataFormat);
    GLINT_WRITE_REG((pPPriv->Attribute[5] << 17) | /* FilterMode */
		    (11 << 13) | (11 << 9) | /* TextureSize log2 */ 
		    UNIT_ENABLE, PMTextureReadMode);
    GLINT_WRITE_REG((0 << 4) /* RGB */ |
		    (3 << 1) /* Copy */ |
		    UNIT_ENABLE, TextureColorMode);
    GLINT_WRITE_REG(UNIT_ENABLE, YUVMode);
    GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    GLINT_WRITE_REG(UNIT_DISABLE, LogicalOpMode);
    GLINT_WRITE_REG(UNIT_ENABLE, TextureAddressMode);
    GLINT_WRITE_REG(pPPriv->dS, dSdx);
    GLINT_WRITE_REG(0, dSdyDom);
    GLINT_WRITE_REG(0, dTdx);
    GLINT_WRITE_REG(pPPriv->dT, dTdyDom);

    /* Subsequent */

    for (; nBox--; pBox++) {
        int w = pBox->x2 - pBox->x1;
	int h = pBox->y2 - pBox->y1;
	int x = (pPPriv->vx << 20) + (pBox->x1 - pPPriv->dx) * pPPriv->dS;
	int y = (vy << 20) + (pBox->y1 - pPPriv->dy) * pPPriv->dT;

	GLINT_WAIT(5);
	GLINT_WRITE_REG((pBox->y1 << 16) | pBox->x1, RectangleOrigin);
	GLINT_WRITE_REG((h << 16) | w, RectangleSize);
	GLINT_WRITE_REG(x, SStart);
	GLINT_WRITE_REG(y, TStart);
        GLINT_WRITE_REG(PrimitiveRectangle |
			XPositive |
			YPositive |
			TextureEnable, Render);
    }

    /* Cleanup */

    pGlint->x = pGlint->y = -1;
    pGlint->w = pGlint->h = -1;
    pGlint->ROP = 0xFF;
    GLINT_WAIT(6);
    GLINT_WRITE_REG(pGlint->TexMapFormat, PMTextureMapFormat);
    GLINT_WRITE_REG(UNIT_DISABLE, DitherMode);
    GLINT_WRITE_REG(UNIT_DISABLE, TextureColorMode);
    GLINT_WRITE_REG(UNIT_DISABLE, TextureAddressMode);
    GLINT_WRITE_REG(UNIT_DISABLE, PMTextureReadMode);
    GLINT_WRITE_REG(UNIT_DISABLE, YUVMode);
}

static void
BlackOut(PortPrivPtr pPPriv)
{
    ScrnInfoPtr pScrn = pPPriv->pAdaptor->pScrn;
    ScreenPtr pScreen = pScrn->pScreen;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    RegionRec DRegion;
    BoxRec DBox;
    BoxPtr pBox;
    int nBox;
    int vy, vh;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
	"Xv/BlackOut %d,%d,%d,%d -- %d,%d,%d,%d\n",
	pPPriv->vx, pPPriv->vy, pPPriv->vw, pPPriv->vh,
	pPPriv->dx, pPPriv->dy, pPPriv->dw, pPPriv->dh));

    vy = pPPriv->vy;
    vh = pPPriv->vh;

    if (!pPPriv->Attribute[4]) {
	vy >>= 1;
	vh >>= 1;
    }

    DBox.x1 = pPPriv->dx - (pPPriv->vx * pPPriv->dw) / pPPriv->vw;
    DBox.y1 = pPPriv->dy - (vy * pPPriv->dh) / vh;
    DBox.x2 = DBox.x1 + (pPPriv->fw * pPPriv->dw) / pPPriv->vw;
    DBox.y2 = DBox.y1 + (pPPriv->fh * pPPriv->dh) / vh;

    REGION_INIT(pScreen, &DRegion, &DBox, 1);
    REGION_SUBTRACT(pScreen, &DRegion, &DRegion, &pPPriv->Region);

    nBox = REGION_NUM_RECTS(&DRegion);
    pBox = REGION_RECTS(&DRegion);

    GLINT_WAIT(15);

    CHECKCLIPPING;

    GLINT_WRITE_REG(UNIT_DISABLE, ColorDDAMode);
    GLINT_WRITE_REG(pPPriv->BufferPProd, FBReadMode);
    GLINT_WRITE_REG(0x1, FBReadPixel); /* 16 */
    GLINT_WRITE_REG(0 /* -1 */, FBBlockColor);
    GLINT_WRITE_REG(pPPriv->BufferBase[0] >> 1 /* 16 */, FBWindowBase);
    GLINT_WRITE_REG(UNIT_DISABLE, LogicalOpMode);

    for (; nBox--; pBox++) {
        int w = ((pBox->x2 - pBox->x1) * pPPriv->vw + pPPriv->dw) / pPPriv->dw;
	int h = ((pBox->y2 - pBox->y1) * vh + pPPriv->dh) / pPPriv->dh;
	int x = ((pBox->x1 - DBox.x1) * pPPriv->vw + (pPPriv->dw >> 1)) / pPPriv->dw;
	int y = ((pBox->y1 - DBox.y1) * vh + (pPPriv->dh >> 1)) / pPPriv->dh;

	if ((x + w) > pPPriv->fw)
	    w = pPPriv->fw - x;
	if ((y + h) > pPPriv->fh)
	    h = pPPriv->fh - y;

	GLINT_WAIT(3);
	GLINT_WRITE_REG((y << 16) | x, RectangleOrigin);
	GLINT_WRITE_REG((h << 16) | w, RectangleSize);
	GLINT_WRITE_REG(PrimitiveRectangle |
	    XPositive | YPositive | FastFillEnable, Render);
    }

    REGION_UNINIT(pScreen, &DRegion);

    pGlint->x = pGlint->y = -1;
    pGlint->w = pGlint->h = -1;
    pGlint->ROP = 0xFF;
    GLINT_WAIT(3);
    GLINT_WRITE_REG(0, FBWindowBase);
    GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    GLINT_WRITE_REG(pGlint->PixelWidth, FBReadPixel);
}
				
#define FreeCookies(pPPriv)		\
do {					\
    if ((pPPriv)->pCookies) {		\
        xfree((pPPriv)->pCookies);	\
	(pPPriv)->pCookies = NULL;	\
    }					\
} while (0)

static void
GrabYUV(PortPrivPtr pPPriv)
{
    ScrnInfoPtr pScrn = pPPriv->pAdaptor->pScrn;
    GLINTPtr pGlint = GLINTPTR(pScrn);
    CookiePtr pCookie = pPPriv->pCookies;
    int nCookies = pPPriv->nCookies;
    int vy, vh;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 4,
	"Xv/GrabYUV %d,%d,%d,%d <- %d,%d,%d,%d\n",
	pPPriv->vx, pPPriv->vy, pPPriv->vw, pPPriv->vh,
	pPPriv->dx, pPPriv->dy, pPPriv->dw, pPPriv->dh));

    vy = pPPriv->vy;
    vh = pPPriv->vh;

    if (!pPPriv->Attribute[4]) {
	vy >>= 1;
	vh >>= 1;
    }

    if (!pCookie) {
	BoxPtr pBox = REGION_RECTS(&pPPriv->Region);
	int nBox = REGION_NUM_RECTS(&pPPriv->Region);
	int dw1 = pPPriv->dw - 1;
	int dh1 = pPPriv->dh - 1;

	if (!(pPPriv->pCookies = (CookiePtr) xalloc(nBox * sizeof(CookieRec))))
	    return;

	pPPriv->dS = (pPPriv->dw << 20) / pPPriv->vw;
	pPPriv->dT = (pPPriv->dh << 20) / vh;

	for (pCookie = pPPriv->pCookies; nBox--; pBox++) {
	    int n1, n2;

	    n1 = ((pBox->x1 - pPPriv->dx) * pPPriv->vw + dw1) / pPPriv->dw;
	    n2 = ((pBox->x2 - pPPriv->dx) * pPPriv->vw - 1) / pPPriv->dw;
	    if (n1 > n2) continue; /* clip is subpixel */
	    pCookie->xy = n1 + pPPriv->vx;
	    pCookie->wh = n2 - n1 + 1;
	    pCookie->u = (n1 * pPPriv->dw / pPPriv->vw + pPPriv->dx) << 20;
	    pCookie->u = n1 * pPPriv->dS + (pPPriv->dx << 20);

	    n1 = ((pBox->y1 - pPPriv->dy) * pPPriv->vh + dh1) / pPPriv->dh;
	    n2 = ((pBox->y2 - pPPriv->dy) * vh - 1) / pPPriv->dh;
	    if (n1 > n2) continue;
	    pCookie->xy |= (n1 + vy) << 16;
	    pCookie->wh |= (n2 - n1 + 1) << 16;
	    pCookie->v = (n1 * pPPriv->dh / vh + pPPriv->dy) << 20; 
	    pCookie->v = n1 * pPPriv->dT + (pPPriv->dy << 20);

	    pCookie++;
	}

	nCookies = pPPriv->nCookies = pCookie - pPPriv->pCookies;
	pCookie = pPPriv->pCookies;
    }

    if (!nCookies)
	return;

    /* Setup */

    GLINT_WAIT(25);
    CHECKCLIPPING;

    switch (pScrn->depth) {
	case 8:
	    GLINT_WRITE_REG(UNIT_ENABLE, TexelLUTMode);
	    GLINT_WRITE_REG((1 << 4) | /* No alpha */
	        	    ((14 & 0x10) << 2) |
			    ((14 & 0x0F) << 0), /* CI8 */
			    PMTextureDataFormat);
	    break;

	case 15:	
	    GLINT_WRITE_REG((1 << 5) | /* RGB */
			    (1 << 4) |
			    ((1 & 0x10) << 2) |
			    ((1 & 0x0F) << 0), 	/* 5:5:5:1f */
			    PMTextureDataFormat);
	    break;
	case 16:
	    GLINT_WRITE_REG((1 << 5) |
			    (1 << 4) |
			    ((16 & 0x10) << 2) |
			    ((16 & 0x0F) << 0), /* 5:6:5f */
			    PMTextureDataFormat);
	    break;
	case 24:
	    GLINT_WRITE_REG((1 << 5) |
			    (1 << 4) |
			    ((0 & 0x10) << 2) |
			    ((0 & 0x0F) << 0),	/* 8:8:8:8 */
			    PMTextureDataFormat);
	    break;
	default:       
	    return;
    }

    GLINT_WRITE_REG(UNIT_DISABLE, AreaStippleMode);
    GLINT_WRITE_REG(0, PMTextureBaseAddress);
    GLINT_WRITE_REG((1 << 10) | /* RGB */
		    ((16 & 0x10)<<12)|
		    ((16 & 0x0F)<<2) | 	/* 5:6:5f */
		    UNIT_ENABLE, DitherMode);
    GLINT_WRITE_REG((pPPriv->Attribute[5] << 17) | /* FilterMode */
		    (11 << 13) | (11 << 9) | /* TextureSize log2 */
		    UNIT_ENABLE, PMTextureReadMode);
    GLINT_WRITE_REG((0 << 4) /* RGB */ |
		    (3 << 1) /* Copy */ |
		    UNIT_ENABLE, TextureColorMode);
    GLINT_WRITE_REG(0x1, FBReadPixel); /* 16 */
    GLINT_WRITE_REG(UNIT_DISABLE, YUVMode);
    GLINT_WRITE_REG(pPPriv->BufferPProd, FBReadMode);
    GLINT_WRITE_REG(UNIT_DISABLE, LogicalOpMode);
    GLINT_WRITE_REG(UNIT_ENABLE, TextureAddressMode);
    GLINT_WRITE_REG(pPPriv->dS, dSdx);
    GLINT_WRITE_REG(0, dSdyDom);
    GLINT_WRITE_REG(0, dTdx);
    GLINT_WRITE_REG(pPPriv->dT, dTdyDom);
    GLINT_WRITE_REG(1 << 16, dY);
    GLINT_WRITE_REG(0, RasterizerMode);
    GLINT_WRITE_REG(pPPriv->BufferBase[0] >> 1 /* 16 */, FBWindowBase);

    /* Subsequent */
#if 0
    for (; nCookies--; pCookie++) {
	GLINT_WAIT(5);
	GLINT_WRITE_REG(pCookie->xy, RectangleOrigin);
	GLINT_WRITE_REG(pCookie->wh, RectangleSize);
	GLINT_WRITE_REG(pCookie->u, SStart);
	GLINT_WRITE_REG(pCookie->v, TStart);
        GLINT_WRITE_REG(PrimitiveRectangle |
			XPositive |
			YPositive |
			TextureEnable, Render);
    }
#else /* test w/o clipping */
    {
        int w = ((pPPriv->dw) * pPPriv->vw) / pPPriv->dw;
	int h = ((pPPriv->dh) * vh) / pPPriv->dh;
	int x = pPPriv->vx;
	int y = vy;

	GLINT_WAIT(5);
	GLINT_WRITE_REG((y << 16) | x, RectangleOrigin);
	GLINT_WRITE_REG((h << 16) | w, RectangleSize);
	GLINT_WRITE_REG(pPPriv->dx << 20, SStart);
	GLINT_WRITE_REG(pPPriv->dy << 20, TStart);
        GLINT_WRITE_REG(PrimitiveRectangle |
			XPositive |
			YPositive |
			TextureEnable, Render);
    }
#endif

    /* Cleanup */

    pGlint->x = pGlint->y = -1;
    pGlint->w = pGlint->h = -1;
    pGlint->ROP = 0xFF;
    GLINT_WAIT(10);
    GLINT_WRITE_REG(0, FBWindowBase);
    GLINT_WRITE_REG(pGlint->pprod, FBReadMode);
    GLINT_WRITE_REG(pGlint->PixelWidth, FBReadPixel);
    GLINT_WRITE_REG(UNIT_DISABLE, DitherMode);
    GLINT_WRITE_REG(UNIT_DISABLE, TextureColorMode);
    GLINT_WRITE_REG(UNIT_DISABLE, TextureAddressMode);
    GLINT_WRITE_REG(pGlint->TexMapFormat, PMTextureMapFormat);
    GLINT_WRITE_REG(UNIT_DISABLE, TexelLUTMode);
    GLINT_WRITE_REG(UNIT_DISABLE, PMTextureReadMode);
    GLINT_WRITE_REG(UNIT_DISABLE, YUVMode);
}

static Bool
WaitFrame(PortPrivPtr pPPriv)
{
    usleep(80000);
    return TRUE;
}

static void
StopVideoStream(AdaptorPrivPtr pAPriv, int which)
{
    GLINTPtr pGlint = GLINTPTR(pAPriv->pScrn);

    if (which & 1) pAPriv->Port[0].VideoOn = 0;
    if (which & 2) pAPriv->Port[1].VideoOn = 0;

    /* on? */

    if (which & 2) {
    	xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x3A, 0x83);
#ifndef COLORBARS
	xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x61, 0xC2);
#endif
	GLINT_WRITE_REG(0, VSBBase + VSControl);
	pAPriv->Port[1].StreamOn = FALSE;
    }

    if (which & 1) {
	GLINT_WRITE_REG(0, VSABase + VSControl);
	pAPriv->Port[0].StreamOn = FALSE;
	WaitFrame(&pAPriv->Port[0]);
    }

    if (!(pAPriv->Port[0].StreamOn || pAPriv->Port[1].StreamOn)) {
	if (which & 4)
	    xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x61, 0xC2);
	xf86I2CWriteByte(&pAPriv->Port[0].I2CDev, 0x11, 0x00);
	TimerCancel(pAPriv->Timer);
    }
}

static Bool
StartVideoStream(PortPrivPtr pPPriv)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    GLINTPtr pGlint = GLINTPTR(pAPriv->pScrn);

    pPPriv->APO = -1;

    if (pPPriv->StreamOn)
	return TRUE;

    {
	CARD32 Base = (pPPriv == &pAPriv->Port[0]) ? VSABase : VSBBase;

	if (pPPriv->pFBArea[0] == NULL) {
	    if (!ReallocateOffscreenBuffer(pPPriv, TRUE, pPPriv == &pAPriv->Port[0]))
		return FALSE;
	}

	GLINT_WRITE_REG(pPPriv->BufferBase[0] / 8, Base + VSVideoAddress0);
	if (pPPriv->pFBArea[1])
	    GLINT_WRITE_REG(pPPriv->BufferBase[1] / 8, Base + VSVideoAddress1);
	else
	    GLINT_WRITE_REG(pPPriv->BufferBase[0] / 8, Base + VSVideoAddress1);
	GLINT_WRITE_REG(pPPriv->BufferStride / 8, Base + VSVideoStride);

	GLINT_WRITE_REG(0, Base + VSCurrentLine);

	if (pAPriv->VideoStd == 1) {
	    /* NTSC FIXME */
	    GLINT_WRITE_REG(16, Base + VSVideoStartLine);
	    GLINT_WRITE_REG(16 + 240, Base + VSVideoEndLine);
	    GLINT_WRITE_REG(288 + (8 & ~3) * 2, Base + VSVideoStartData);
	    GLINT_WRITE_REG(288 + ((8 & ~3) + 704) * 2, Base + VSVideoEndData);
	} else {
	    /* PAL, SECAM (SECAM untested) */
	    GLINT_WRITE_REG(16, Base + VSVideoStartLine);
	    GLINT_WRITE_REG(16 + 288, Base + VSVideoEndLine);
	    GLINT_WRITE_REG(288 + (8 & ~3) * 2, Base + VSVideoStartData);
	    GLINT_WRITE_REG(288 + ((8 & ~3) + 704) * 2, Base + VSVideoEndData);
	}

	GLINT_WRITE_REG(2, Base + VSVideoAddressHost);
	GLINT_WRITE_REG(0, Base + VSVideoAddressIndex);

	if (pPPriv == &pAPriv->Port[0]) {
	    xf86I2CWriteByte(&pAPriv->Port[0].I2CDev, 0x11, 0x0D);
	    GLINT_WRITE_REG(VSA_Video |
			    (pPPriv->Attribute[4] ? 
				VSA_CombineFields : VSA_Discard_FieldTwo),
			    VSABase + VSControl);
#ifdef COLORBARS
	    if (!pAPriv->Port[1].StreamOn) {
		xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x3A, 0x83);
		xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x61, Enc61[pAPriv->VideoStd]);
	    }
#endif
	} else {
	    GLINT_WRITE_REG(VSB_Video |
			    (pPPriv->Attribute[4] ? VSB_CombineFields : 0) |
			  /* VSB_GammaCorrect | */
			    (16 << 4) | /* 5:6:5 FIXME */
			    (1 << 9) | /* 16 */
			    VSB_RGBOrder, VSBBase + VSControl);
	    xf86I2CWriteByte(&pAPriv->Port[0].I2CDev, 0x11, 0x0D);
	    xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x3A, Enc3A[pPPriv->Plug]);
	    xf86I2CWriteByte(&pAPriv->Port[1].I2CDev, 0x61, Enc61[pAPriv->VideoStd]);
	}

	TimerSet(pAPriv->Timer, 0, 80, TimerCallback, pAPriv);

	return pPPriv->StreamOn = TRUE;
    }

    return FALSE;
}

/* Pseudo interrupt - better than nothing */

static CARD32
TimerCallback(OsTimerPtr pTim, CARD32 now, pointer p)
{
    AdaptorPrivPtr pAPriv = (AdaptorPrivPtr) p;
    GLINTPtr pGlint = GLINTPTR(pAPriv->pScrn);
    PortPrivPtr pPPriv;
    int delay;

    pPPriv = &pAPriv->Port[0];

    if (pPPriv->VideoOn >= 2) {
	pPPriv->FrameAcc += pPPriv->Frames;
	if (pPPriv->FrameAcc >= pAPriv->FramesPerSec) {
	    pPPriv->FrameAcc -= pAPriv->FramesPerSec;
	    DrawYUV(pPPriv, 1 - GLINT_READ_REG(VSABase + VSVideoAddressIndex));
	}
    } else if (pPPriv->APO >= 0 && !(pPPriv->APO--))
	DelayedStopVideo(pPPriv);

    pPPriv = &pAPriv->Port[1];

    if (pPPriv->VideoOn >= 1) {
	pPPriv->FrameAcc += pPPriv->Frames;
	if (pPPriv->FrameAcc >= pAPriv->FramesPerSec) {
	    pPPriv->FrameAcc -= pAPriv->FramesPerSec;
    	    if (pPPriv->BlackOut) {
		BlackOut(pPPriv);
		pPPriv->BlackOut = FALSE;
	    }
	    GrabYUV(pPPriv);
	    if (pPPriv->VideoOn == 1)
		pPPriv->VideoOn = 0;
	}
    } else if (pPPriv->APO >= 0 && !(pPPriv->APO--))
	DelayedStopVideo(pPPriv);

    if (pAPriv->Port[0].StreamOn) {
	delay = GLINT_READ_REG(VSABase + VSCurrentLine);
	if (!(GLINT_READ_REG(VSStatus) & VS_FieldOne0A))
	    delay += pAPriv->FrameLines >> 1;
    } else if (pAPriv->Port[1].StreamOn) {
	delay = GLINT_READ_REG(VSBBase + VSCurrentLine);
	if (!(GLINT_READ_REG(VSStatus) & VS_FieldOne0B))
	    delay += pAPriv->FrameLines >> 1;
    } else
	    delay = pAPriv->IntLine;

    if (delay > (pAPriv->IntLine - 16))
	delay -= pAPriv->FrameLines;

    /* FIXME adap */

    return (((pAPriv->IntLine - delay) * pAPriv->LinePer) + 999999) / 1000000;
}

static int
Permedia2PutVideo(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3, "Xv/PV\n"));

    if (pPPriv == &pAPriv->Port[0]) {
	if ((vid_x + vid_w) > InputVideoEncodings[pAPriv->VideoStd * 3].width ||
	    (vid_y + vid_h) > InputVideoEncodings[pAPriv->VideoStd * 3].height)
	    return BadValue;

	pPPriv->VideoOn = 0;

        pPPriv->vx = vid_x;
	pPPriv->vy = vid_y;
        pPPriv->vw = vid_w;
	pPPriv->vh = vid_h;

        pPPriv->dx = drw_x;
	pPPriv->dy = drw_y;
        pPPriv->dw = drw_w;
	pPPriv->dh = drw_h;

	{
	    pPPriv->FrameAcc = pAPriv->FramesPerSec;

	    if (!StartVideoStream(pPPriv))
		return XvBadAlloc;

	    pPPriv->VideoOn = 2;
	    pPPriv->APO = -1;

	    return Success;
	}
    }

    return XvBadAlloc;
}

static int
Permedia2PutStill(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
    GLINTPtr pGlint = GLINTPTR(pScrn);

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3, "Xv/PS\n"));

    if (pPPriv == &pAPriv->Port[0]) {
	if ((vid_x + vid_w) > InputVideoEncodings[pAPriv->VideoStd * 3].width ||
	    (vid_y + vid_h) > InputVideoEncodings[pAPriv->VideoStd * 3].height)
	    return BadValue;

	pPPriv->VideoOn = 0;

        pPPriv->vx = vid_x;
	pPPriv->vy = vid_y;
        pPPriv->vw = vid_w;
	pPPriv->vh = vid_h;

        pPPriv->dx = drw_x;
	pPPriv->dy = drw_y;
        pPPriv->dw = drw_w;
	pPPriv->dh = drw_h;

	{
	    Bool r;

	    if (!StartVideoStream(pPPriv))
		return XvBadAlloc;

	    r = WaitFrame(pPPriv);

	    if (r)
		DrawYUV(pPPriv, 1 - GLINT_READ_REG(VSABase + VSVideoAddressIndex));

	    pPPriv->APO = 125; /* Delayed stop: consider PutStill staccato */

	    if (r)
		return Success;
	}
    }

    return XvBadAlloc;
}

static int
Permedia2GetVideo(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3, "Xv/GV\n"));

    if (pPPriv == &pAPriv->Port[1]) {
	if ((vid_x + vid_w) > InputVideoEncodings[pAPriv->VideoStd * 3].width ||
	    (vid_y + vid_h) > InputVideoEncodings[pAPriv->VideoStd * 3].height)
	    return BadValue;

	pPPriv->VideoOn = 0;

        pPPriv->vx = vid_x;
	pPPriv->vy = vid_y;
        pPPriv->vw = vid_w;
	pPPriv->vh = vid_h;

        pPPriv->dx = drw_x;
	pPPriv->dy = drw_y;
        pPPriv->dw = drw_w;
	pPPriv->dh = drw_h;

	{
	    if (pPPriv->pCookies) {
		xfree(pPPriv->pCookies);
		pPPriv->pCookies = NULL;
	    }

	    pPPriv->FrameAcc = pAPriv->FramesPerSec;

	    if (!StartVideoStream(pPPriv))
		return XvBadAlloc;

	    pPPriv->BlackOut = TRUE;
	    pPPriv->VideoOn = 2;

	    if (REGION_NIL(&pPPriv->Region))
		pPPriv->VideoOn = 1; /* one shot */

	    return Success;
	}
    }

    return XvBadAlloc;
}

static int
Permedia2GetStill(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3, "Xv/GS\n"));

    if (pPPriv == &pAPriv->Port[1]) {
	if ((vid_x + vid_w) > InputVideoEncodings[pAPriv->VideoStd * 3].width ||
	    (vid_y + vid_h) > InputVideoEncodings[pAPriv->VideoStd * 3].height)
	    return BadValue;

	pPPriv->VideoOn = 0;

        pPPriv->vx = vid_x;
	pPPriv->vy = vid_y;
        pPPriv->vw = vid_w;
	pPPriv->vh = vid_h;

        pPPriv->dx = drw_x;
	pPPriv->dy = drw_y;
        pPPriv->dw = drw_w;
	pPPriv->dh = drw_h;

	{
	    Bool r;

	    if (!StartVideoStream(pPPriv))
		return XvBadAlloc;

	    r = WaitFrame(pPPriv);

	    if (r) {
		if (pPPriv->pCookies) {
		    xfree(pPPriv->pCookies);
		    pPPriv->pCookies = NULL;
		}

		BlackOut(pPPriv);
		GrabYUV(pPPriv);
	    }

	    if (r) return Success;
	}
    }

    return XvBadAlloc;
}

static void
Permedia2StopVideo(ScrnInfoPtr pScrn, pointer data, Bool exit)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3, "Xv/StopVideo port=%d, exit=%d\n",
	PORTNUM(pPPriv), exit));

    pPPriv->VideoOn = 0;

    if (exit) {
    	StopVideoStream(pAPriv, 1 << PORTNUM(pPPriv));
	ReallocateOffscreenBuffer(pPPriv, FALSE, FALSE);
    } else
	pPPriv->APO = 750; /* delay */
}

static void
DelayedStopVideo(PortPrivPtr pPPriv)
{
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pAPriv->pScrn->scrnIndex, X_INFO, 3, "Xv/DelayedStopVideo\n"));

    pPPriv->APO = -1;
    Permedia2StopVideo(pAPriv->pScrn, (pointer) pPPriv, TRUE);
}

static void
Permedia2ReclipVideo(ScrnInfoPtr pScrn, RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    int VideoOn = pPPriv->VideoOn; /* paranoia, must be off */

    DEBUG(
	AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 3, "Xv/Reclip port=%d\n",
	    PORTNUM(pPPriv));
    )

    pPPriv->VideoOn = 0;
    REGION_COPY(pScrn->pScreen, &pPPriv->Region, clipBoxes);
    pPPriv->VideoOn = VideoOn;
    /* GetVideo one-shot in case region empty */
}

static void
AdjustVideoH(PortPrivPtr pPPriv, int num, int denom)
{
    pPPriv->vy = ((int) pPPriv->vy * num) / denom;
    pPPriv->vh = ((int) pPPriv->vh * num) / denom;
}

static int
Permedia2SetPortAttribute(ScrnInfoPtr pScrn,
    Atom attribute, INT32 value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data; 
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/SPA %d, %d, %d\n",
	attribute, value, PORTNUM(pPPriv)));

    if (attribute == xvFilter) {
	pPPriv->Attribute[5] = !!value;
	return Success;
    } else if (attribute == xvInterlace) {
	value = !!value;

	if (value != pPPriv->Attribute[4]) {
	    int VideoOn = ABS(pPPriv->VideoOn);

    	    StopVideoStream(pAPriv, 1 << PORTNUM(pPPriv));
	    ReallocateOffscreenBuffer(pPPriv, FALSE, FALSE);
	    FreeCookies(pPPriv);
	    pPPriv->Attribute[4] = value;

	    if (VideoOn)
		if (StartVideoStream(pPPriv))
		    pPPriv->VideoOn = VideoOn;
		else {
		    pPPriv->VideoOn = -VideoOn;
		    return XvBadAlloc;
		}
	}
	
	return Success;
    }
    
    if (PORTNUM(pPPriv) == 0) {
	if (attribute == xvEncoding) {
	    if (value < 0 || value > 8) {
		return XvBadEncoding;
	    }
	    /* fall thru */
	} else if (attribute == xvBrightness) {
	    pPPriv->Attribute[0] = value = CLAMP(value, 0, +1000);
	    return SetAttr(&pAPriv->Port[0], 0, 0x0A, (value * 255) / 1000);
	} else if (attribute == xvContrast) {
	    pPPriv->Attribute[1] = value = CLAMP(value, -1000, +1000);
	    return SetAttr(&pAPriv->Port[0], 1, 0x0B, (value * 127) / 1000);
   	} else if (attribute == xvSaturation) {
	    pPPriv->Attribute[2] = value = CLAMP(value, -1000, +1000);
	    return SetAttr(&pAPriv->Port[0], 2, 0x0C, (value * 127) / 1000);
	} else if (attribute == xvHue) {
	    pPPriv->Attribute[3] = value = CLAMP(value, -1000, +1000);
	    return SetAttr(&pAPriv->Port[0], 3, 0x0D, (value * 127) / 1000);
	} else
	    return BadMatch;
    } else { /* Output */
	if (attribute == xvEncoding) {
	    if ((value % 3) == 0 || value > 5)
		return XvBadEncoding;
	} else
    	    return Success;
    }

    if (attribute == xvEncoding) {
	pPPriv->Plug = value % 3;

	if (!SetPlug(pPPriv))
	    return XvBadAlloc;

	value /= 3;

	if (value != pAPriv->VideoStd) {
	    int VideoOn0 = ABS(pAPriv->Port[0].VideoOn);
	    int	VideoOn1 = ABS(pAPriv->Port[1].VideoOn);

	    StopVideoStream(pAPriv, 1 | 2);

	    if (value == 1) {
		AdjustVideoH(&pAPriv->Port[0], 576, 480);
		AdjustVideoH(&pAPriv->Port[1], 576, 480);
	    } else if (pAPriv->VideoStd == 1) {
		AdjustVideoH(&pAPriv->Port[0], 480, 576);
		AdjustVideoH(&pAPriv->Port[1], 480, 576);
	    }
	    
	    if (value == 1 || pAPriv->VideoStd == 1) {
		ReallocateOffscreenBuffer(&pAPriv->Port[0], FALSE, FALSE);
		ReallocateOffscreenBuffer(&pAPriv->Port[1], FALSE, FALSE);
		FreeCookies(&pAPriv->Port[0]);
		FreeCookies(&pAPriv->Port[1]);
	    }

	    pAPriv->VideoStd = value;

	    if (VideoOn0)
		pAPriv->Port[0].VideoOn = (StartVideoStream(&pAPriv->Port[0])) ?
		    VideoOn0 : -VideoOn0;

	    if (VideoOn1)
		pAPriv->Port[1].VideoOn = (StartVideoStream(&pAPriv->Port[1])) ?
		    VideoOn1 : -VideoOn1;

	    if (pAPriv->Port[0].VideoOn < 0 || pAPriv->Port[1].VideoOn < 0)
		return XvBadAlloc;
	}
    }

    return Success;
}

static int
Permedia2GetPortAttribute(ScrnInfoPtr pScrn, 
    Atom attribute, INT32 *value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    AdaptorPrivPtr pAPriv = pPPriv->pAdaptor;

    if (attribute == xvEncoding)
	*value = pAPriv->VideoStd * 3 + pPPriv->Plug;
    else if (attribute == xvBrightness)
	*value = pPPriv->Attribute[0];
    else if (attribute == xvContrast)
	*value = pPPriv->Attribute[1];
    else if (attribute == xvSaturation)
	*value = pPPriv->Attribute[2];
    else if (attribute == xvHue)
	*value = pPPriv->Attribute[3];
    else if (attribute == xvInterlace)
	*value = pPPriv->Attribute[4];
    else if (attribute == xvFilter)
	*value = pPPriv->Attribute[5];
    else
	return BadMatch;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/GPA %d, %d, %d\n",
	attribute, *value, PORTNUM(pPPriv)));

    return Success;
}

static void
Permedia2QueryBestSize(ScrnInfoPtr pScrn, Bool motion,
    short vid_w, short vid_h, short drw_w, short drw_h,
    unsigned int *p_w, unsigned int *p_h, pointer data)
{
    *p_w = drw_w;
    *p_h = drw_h;
}

static void
AdaptorPrivUninit(AdaptorPrivPtr pAPriv)
{
    GLINTPtr pGlint = GLINTPTR(pAPriv->pScrn);

    {
	TimerFree(pAPriv->Timer);
	xf86DestroyI2CDevRec(&pAPriv->Port[0].I2CDev, FALSE);
	xf86DestroyI2CDevRec(&pAPriv->Port[1].I2CDev, FALSE);
	GLINT_MASK_WRITE_REG(VS_UnitMode_ROM, ~VS_UnitMode_Mask, VSConfiguration);
	FreeCookies(&pAPriv->Port[0]);
	FreeCookies(&pAPriv->Port[1]);
    }

    GLINT_WRITE_REG(pAPriv->FifoControl, PMFifoControl);

    xfree(pAPriv);
}

static AdaptorPrivPtr
AdaptorPrivInit(ScrnInfoPtr pScrn)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    AdaptorPrivPtr pAPriv = (AdaptorPrivPtr) xcalloc(1, sizeof(AdaptorPrivRec));
    int i;

    if (pAPriv) {
	pAPriv->pScrn = pScrn;
	pAPriv->Port[0].pAdaptor = pAPriv;
	pAPriv->Port[1].pAdaptor = pAPriv;

	pAPriv->Port[0].Attribute[0] = 500;	/* Brightness (0..1000) */
	pAPriv->Port[0].Attribute[1] = 500; 	/* Contrast (-1000..+1000) */
	pAPriv->Port[0].Attribute[2] = 500; 	/* Color saturation (-1000..+1000) */
	pAPriv->Port[0].Attribute[3] = 0;	/* Hue (-1000..+1000) */
	pAPriv->Port[0].Attribute[4] = 1;	/* Interlaced (Bool) */
	pAPriv->Port[0].Attribute[5] = 0;	/* Bilinear Filter (Bool) */

	pAPriv->Port[1].Attribute[4] = 1;	/* Interlaced (Bool) */
	pAPriv->Port[1].Attribute[5] = 0;	/* Bilinear Filter (Bool) */

	{
	    GLINT_WRITE_REG(0, VSABase + VSControl);
	    GLINT_WRITE_REG(0, VSBBase + VSControl);
#if 0
	    GLINT_MASK_WRITE_REG(0, ~(VSAIntFlag | VSBIntFlag), IntEnable);
	    GLINT_WRITE_REG(VSAIntFlag | VSBIntFlag, IntFlags); /* Reset */
#endif
    	    for (i = 0x0018; i <= 0x00B0; i += 8) {
		GLINT_WRITE_REG(0, VSABase + i);
		GLINT_WRITE_REG(0, VSBBase + i);
	    }

	    GLINT_WRITE_REG((0 << 8) | (132 << 0), VSABase + VSFifoControl);
	    GLINT_WRITE_REG((0 << 8) | (132 << 0), VSBBase + VSFifoControl);

	    GLINT_MASK_WRITE_REG(
		VS_UnitMode_AB8 |
		VS_GPBusMode_A |
	     /* VS_HRefPolarityA | */
		VS_VRefPolarityA |
		VS_VActivePolarityA |
	     /* VS_UseFieldA | */
		VS_FieldPolarityA |
	     /* VS_FieldEdgeA | */
	     /* VS_VActiveVBIA | */
        	VS_InterlaceA |
		VS_ReverseDataA |

	     /* VS_HRefPolarityB | */
		VS_VRefPolarityB |
		VS_VActivePolarityB |
	     /* VS_UseFieldB | */
		VS_FieldPolarityB |
	     /* VS_FieldEdgeB | */
	     /* VS_VActiveVBIB | */
		VS_InterlaceB |
	     /* VS_ColorSpaceB_RGB | */
	     /* VS_ReverseDataB | */
	     /* VS_DoubleEdgeB | */
		0, ~0x1FFFFE0F, VSConfiguration);

	    pAPriv->FifoControl = GLINT_READ_REG(PMFifoControl);
	    GLINT_WRITE_REG((12 << 8) | 8, PMFifoControl); /* FIXME */

	    if (!xf86I2CProbeAddress(pGlint->VSBus, SAA7111_SLAVE_ADDRESS))
		goto failed;

	    pAPriv->Port[0].I2CDev.DevName = "Decoder";
	    pAPriv->Port[0].I2CDev.SlaveAddr = SAA7111_SLAVE_ADDRESS;
	    pAPriv->Port[0].I2CDev.pI2CBus = pGlint->VSBus;

	    if (!xf86I2CDevInit(&pAPriv->Port[0].I2CDev))
		goto failed;

	    if (!xf86I2CWriteVec(&pAPriv->Port[0].I2CDev, DecInitVec,
		(sizeof(DecInitVec) / sizeof(I2CByte)) / 2))
		goto failed;

	    if (!xf86I2CProbeAddress(pGlint->VSBus, SAA7125_SLAVE_ADDRESS))
		goto failed;

	    pAPriv->Port[1].I2CDev.DevName = "Encoder";
	    pAPriv->Port[1].I2CDev.SlaveAddr = SAA7125_SLAVE_ADDRESS;
	    pAPriv->Port[1].I2CDev.pI2CBus = pGlint->VSBus;

	    if (!xf86I2CDevInit(&pAPriv->Port[1].I2CDev))
		goto failed;

	    if (!xf86I2CWriteVec(&pAPriv->Port[1].I2CDev, EncInitVec,
		(sizeof(EncInitVec) / sizeof(I2CByte)) / 2))
		goto failed;

	    SetVideoStd(pAPriv);

	    pAPriv->Port[0].APO = -1;

     	    pAPriv->Timer = TimerSet(NULL, 0, 0,
		TimerCallback, pAPriv);

	    if (!pAPriv->Timer)
		goto failed;
	}

	return pAPriv;

failed:
	AdaptorPrivUninit(pAPriv);
    }

    return NULL;
}

void
Permedia2VideoUninit(ScrnInfoPtr pScrn)
{
    AdaptorPrivPtr pAPriv, *ppAPriv;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1, "Xv driver cleanup\n"));

    for (ppAPriv = &AdaptorPrivList; (pAPriv = *ppAPriv); ppAPriv = &(pAPriv->Next))
	if (pAPriv->pScrn == pScrn) {
	    *ppAPriv = pAPriv->Next;
	    StopVideoStream(pAPriv, 1 | 2 | 4);
	    AdaptorPrivUninit(pAPriv);
	    break;
	}
}

/* Required to retire delayed stop */

void
Permedia2VideoReset(ScrnInfoPtr pScrn)
{
    AdaptorPrivPtr pAPriv;

    for (pAPriv = AdaptorPrivList; pAPriv != NULL; pAPriv = pAPriv->Next)
	if (pAPriv->pScrn == pScrn) {
	    StopVideoStream(pAPriv, 1 | 2 | 4);
	    break;
	}

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1, "Xv driver halted\n"));
}

void
Permedia2VideoInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    AdaptorPrivPtr pAPriv;
    DevUnion Private[2];
    XF86VideoAdaptorRec VAR[2];
    XF86VideoInfoRec VIR;
    int i;

    pGlint->VideoIO = FALSE; /* FIXME */

    if (!pGlint->VideoIO) {
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1, "Xv driver disabled\n");
	return;
    }

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1, "Initializing Xv driver\n");

    switch (pGlint->Chipset) {
	case PCI_VENDOR_TI_CHIP_PERMEDIA2:
	case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2:
	case PCI_VENDOR_3DLABS_CHIP_PERMEDIA2V:
	    break;
	default:
	    pGlint->VideoIO = FALSE;
    }

    switch (pciReadLong(pGlint->PciTag, PCI_SUBSYSTEM_ID_REG)) {
	case PCI_SUBSYSTEM_ID_WINNER_2000_A:
	case PCI_SUBSYSTEM_ID_WINNER_2000_B:
	case PCI_SUBSYSTEM_ID_GLORIA_SYNERGY_A:
	case PCI_SUBSYSTEM_ID_GLORIA_SYNERGY_B:
	    break;
	default:
	    pGlint->VideoIO = FALSE;
    }

    if (!pGlint->VideoIO) {
	xf86DrvMsgVerb(pScrn->scrnIndex, X_PROBED, 1, "No Xv support for this board\n");
	return;
    }

    if (!(pAPriv = AdaptorPrivInit(pScrn))) {
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 1, "Xv init failed at %d\n", __LINE__));
	pGlint->VideoIO = FALSE;
	return;
    }

    if (pGlint->NoAccel) {
	BoxRec AvailFBArea;

	xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Xv driver overrides NoAccel option\n");

	Permedia2InitializeEngine(pScrn);

	AvailFBArea.x1 = 0;
	AvailFBArea.y1 = 0;
	AvailFBArea.x2 = pScrn->displayWidth;
	AvailFBArea.y2 = pGlint->FbMapSize /
	    (pScrn->displayWidth * pScrn->bitsPerPixel / 8);

	/* if (AvailFBArea.y2 > 2047)
	    AvailFBArea.y2 = 2047; */

	xf86InitFBManager(pScreen, &AvailFBArea);
    }

    memset(VAR, 0, sizeof(VAR));

    for (i = 0; i < 2; i++) {
	Private[i].ptr = (pointer) &pAPriv->Port[i];

	VAR[i].type = (i == 0) ? XvInputMask : XvOutputMask;
	VAR[i].name = "Permedia 2";

	if (i == 0) {
	    VAR[i].nEncodings = sizeof(InputVideoEncodings) / sizeof(InputVideoEncodings[0]);
	    VAR[i].pEncodings = InputVideoEncodings;
	    VAR[i].nFormats = sizeof(InputVideoFormats) / sizeof(InputVideoFormats[0]);
	    VAR[i].pFormats = InputVideoFormats;
	} else {
	    VAR[i].nEncodings = sizeof(OutputVideoEncodings) / sizeof(OutputVideoEncodings[0]);
	    VAR[i].pEncodings = OutputVideoEncodings;
	    VAR[i].nFormats = sizeof(OutputVideoFormats) / sizeof(OutputVideoFormats[0]);
	    VAR[i].pFormats = OutputVideoFormats;
	}

	VAR[i].nPorts = 1;
	VAR[i].pPortPrivates = &Private[i];
	VAR[i].PutVideo = Permedia2PutVideo;
	VAR[i].PutStill = Permedia2PutStill;
	VAR[i].GetVideo = Permedia2GetVideo;
	VAR[i].GetStill = Permedia2GetStill;
	VAR[i].StopVideo = Permedia2StopVideo;
	VAR[i].ReclipVideo = Permedia2ReclipVideo;
	VAR[i].SetPortAttribute = Permedia2SetPortAttribute;
	VAR[i].GetPortAttribute = Permedia2GetPortAttribute;
	VAR[i].QueryBestSize = Permedia2QueryBestSize;
    }

    VIR.NumAdaptors = 2;
    VIR.Adaptors = &VAR[0];

    if (!xf86XVScreenInit(pScreen, &VIR)) {
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_ERROR, 1, "Xv init failed at %d\n", __LINE__));
	pGlint->VideoIO = FALSE;
	return;
    }

    xvEncoding   = MAKE_ATOM(XV_ENCODING);
    xvHue        = MAKE_ATOM(XV_HUE);
    xvSaturation = MAKE_ATOM(XV_SATURATION);
    xvBrightness = MAKE_ATOM(XV_BRIGHTNESS);
    xvContrast   = MAKE_ATOM(XV_CONTRAST);
    xvInterlace  = MAKE_ATOM(XV_INTERLACE);
    xvFilter	 = MAKE_ATOM(XV_FILTER);

    pAPriv->Next = AdaptorPrivList;
    AdaptorPrivList = pAPriv;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Xv driver enabled\n");
}

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_type.h,v 1.5 1999/09/27 06:29:54 dawes Exp $ */

#ifndef __NV_STRUCT_H__
#define __NV_STRUCT_H__

#include "riva_hw.h"

#define SetBitField(value,from,to) SetBF(to, GetBF(value,from))
#define SetBit(n) (1<<(n))
#define Set8Bits(value) ((value)&0xff)

#define MAX_CURS            32

typedef RIVA_HW_STATE* NVRegPtr;

typedef struct {
    Bool        isHwCursor;
    int         CursorMaxWidth;
    int         CursorMaxHeight;
    int         CursorFlags;
    int         CursorOffscreenMemSize;
    Bool        (*UseHWCursor)(ScreenPtr, CursorPtr);
    void        (*LoadCursorImage)(ScrnInfoPtr, unsigned char*);
    void        (*ShowCursor)(ScrnInfoPtr);
    void        (*HideCursor)(ScrnInfoPtr);
    void        (*SetCursorPosition)(ScrnInfoPtr, int, int);
    void        (*SetCursorColors)(ScrnInfoPtr, int, int);
    long        maxPixelClock;
    long        MemoryClock;
    MessageType ClockFrom;
    MessageType MemClkFrom;
    Bool        SetMemClk;
    void        (*LoadPalette)(ScrnInfoPtr, int, int*, LOCO*, VisualPtr);
    void        (*PreInit)(ScrnInfoPtr);
    void        (*Save)(ScrnInfoPtr, vgaRegPtr, NVRegPtr, Bool);
    void        (*Restore)(ScrnInfoPtr, vgaRegPtr, NVRegPtr, Bool);
    Bool        (*ModeInit)(ScrnInfoPtr, DisplayModePtr);
} NVRamdacRec, *NVRamdacPtr;


typedef struct {
    RIVA_HW_INST        riva;
    RIVA_HW_STATE       SavedReg;
    RIVA_HW_STATE       ModeReg;
    EntityInfoPtr       pEnt;
    pciVideoPtr         PciInfo;
    PCITAG              PciTag;
    xf86AccessRec       Access;
    int                 Chipset;
    int                 ChipRev;
    Bool                Primary;
    Bool                Interleave;
    int                 HwBpp;
    int                 Rounding;
    int                 BppShift;
    Bool                HasFBitBlt;
    Bool                OverclockMem;
    int                 YDstOrg;
    int                 DstOrg;
    int                 SrcOrg;
    CARD32              IOAddress;
    CARD32              FbAddress;
    int                 FbBaseReg;
    unsigned char *     IOBase;
    unsigned char *     FbBase;
    unsigned char *     FbStart;
    long                FbMapSize;
    long                FbUsableSize;
    long                FbCursorOffset;
    NVRamdacRec         Dac;
    Bool                NoAccel;
    Bool                SyncOnGreen;
    Bool                Dac6Bit;
    Bool                HWCursor;
    Bool                UsePCIRetry;
    Bool                ShowCache;
    Bool                Overlay8Plus24;
    Bool                ShadowFB;
    unsigned char *     ShadowPtr;
    int                 ShadowPitch;
    int                 MemClk;
    int                 MinClock;
    int                 MaxClock;
    CARD32              BltScanDirection;
    CARD32              FilledRectCMD;
    CARD32              SolidLineCMD;
    CARD32              PatternRectCMD;
    CARD32              DashCMD;
    CARD32              NiceDashCMD;
    CARD32              AccelFlags;
    CARD32              PlaneMask;
    CARD32              FgColor;
    CARD32              BgColor;
    int                 StyleLen;
    XAAInfoRecPtr       AccelInfoRec;
    xf86CursorInfoPtr   CursorInfoRec;
    DGAModePtr          DGAModes;
    int                 numDGAModes;
    Bool                DGAactive;
    int                 DGAViewportStatus;
    CARD32              *Atype;
    CARD32              *AtypeNoBLK;
    void                (*PreInit)(ScrnInfoPtr pScrn);
    void                (*Save)(ScrnInfoPtr, vgaRegPtr, NVRegPtr, Bool);
    void                (*Restore)(ScrnInfoPtr, vgaRegPtr, NVRegPtr, Bool);
    Bool                (*ModeInit)(ScrnInfoPtr, DisplayModePtr);
    CloseScreenProcPtr  CloseScreen;
/*    unsigned int        (*ddc1Read)(ScrnInfoPtr);
    void (*DDC1SetSpeed)(ScrnInfoPtr, xf86ddcSpeed);
    Bool                (*i2cInit)(ScrnInfoPtr);
    I2CBusPtr           I2C;*/
    Bool                FBDev;
    int                 colorKey;
    /* Color expansion */
    Bool                useFifo;
    unsigned char       *expandBuffer;
    unsigned char       *expandFifo;
    int                 expandWidth;
    int                 expandRows;
    /* Cursor */
    unsigned short      curFg, curBg;
    unsigned int        curImage[MAX_CURS*2];
    /* Misc flags */
    unsigned int        opaqueMonochrome;
    int                 currentRop;
} NVRec, *NVPtr;

#define NVPTR(p) ((NVPtr)((p)->driverPrivate))


typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_RGB_BITS,
    OPTION_NOACCEL,
    OPTION_SHOWCACHE,
    OPTION_SHADOW_FB,
    OPTION_FBDEV,
    OPTION_COLOR_KEY,
    OPTION_SET_MCLK
} NVOpts;

#endif /* __NV_STRUCT_H__ */

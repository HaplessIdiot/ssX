/* $XFree86$ */

#include "initdef.h"

USHORT VBInfo,SetFlag,RVBHCFACT,RVBHCMAX,VGAVT,VGAHT,VT,HT,VGAVDE,VGAHDE;
USHORT VDE,HDE,RVBHRS,NewFlickerMode,RY1COE,RY2COE,RY3COE,RY4COE;                
USHORT LCDResInfo,LCDTypeInfo,LCDInfo,VCLKLen;

USHORT StResInfo[5][2]={{640,400},{640,350},{720,400},{720,350},{640,480}};
USHORT ModeResInfo[13][4]={{320,200,8,8},{320,240,8,8},{320,400,8,8},
                       {400,300,8,8},{512,384,8,8},{640,400,8,16},
                       {640,480,8,16},{800,600,8,16},{1024,768,8,16},
                       {1280,1024,8,16},{1600,1200,8,16},{1920,1440,8,16},
                       {2048,1536,8,16}};

USHORT NTSCTiming[61]={0x017,0x01D,0x003,0x009,0x005,0x006,0x00C,0x00C,
                    0x094,0x049,0x001,0x00A,0x006,0x00D,0x004,0x00A,
                    0x006,0x014,0x00D,0x004,0x00A,0x000,0x085,0x01B,
                    0x00C,0x050,0x000,0x099,0x000,0x0EC,0x04A,0x017,
                    0x088,0x000,0x04B,0x000,0x000,0x0E2,0x000,0x002,
                    0x003,0x00A,0x065,0x09D,0x008,
                    0x092,0x08F,0x040,0x060,0x080,0x014,0x090,0x08C,
                    0x060,0x014,0x050,0x000,0x040,
                    0x00044,0x002DB,0x0003B};    /* Ajust xxx */

USHORT PALTiming[61]={ 0x019,0x052,0x035,0x06E,0x004,0x038,0x03D,0x070,
                    0x094,0x049,0x001,0x012,0x006,0x03E,0x035,0x06D,
                    0x006,0x014,0x03E,0x035,0x06D,0x000,0x045,0x02B,
                    0x070,0x050,0x000,0x097,0x000,0x0D7,0x05D,0x017,
                    0x088,0x000,0x045,0x000,0x000,0x0E8,0x000,0x002,
                    0x00D,0x000,0x068,0x0B0,0x00B,
                    0x092,0x08F,0x040,0x060,0x080,0x014,0x090,0x08C,
                    0x060,0x014,0x063,0x000,0x040,
                    0x0003E,0x002E1,0x00028};    /* Ajust xxx */

USHORT NTSCGroup3Data[63]= {0x000,0x014,0x015,0x025,0x055,0x015,0x00B,0x089,
                         0x0D7,0x040,0x0B0,0x0B0,0x0FF,0x0C4,0x045,0x0A6,
                         0x025,0x02F,0x067,0x0F6,0x0BF,0x0FF,0x08E,0x020,
                         0x08C,0x0DA,0x060,0x092,0x0C8,0x055,0x08B,0x000,
                         0x051,0x004,0x018,0x00A,0x0F8,0x087,0x000,0x080,
                         0x03B,0x03B,0x000,0x0F0,0x0F0,0x000,0x0F0,0x0F0,
                         0x000,0x051,0x00F,0x00F,0x008,0x00F,0x008,0x06F,
                         0x018,0x005,0x005,0x005,0x04C,0x0AA,0x001};

USHORT PALGroup3Data[63]={0x000,0x01A,0x022,0x063,0x062,0x022,0x008,0x085,
                       0x0C3,0x020,0x0A4,0x0A4,0x055,0x047,0x02A,0x0A6,
                       0x025,0x02F,0x047,0x0FA,0x0C8,0x0FF,0x08E,0x020,
                       0x08C,0x0DC,0x060,0x092,0x0C8,0x04F,0x085,0x000,
                       0x056,0x036,0x04F,0x06E,0x0FE,0x083,0x054,0x081,
                       0x030,0x030,0x000,0x0F3,0x0F3,0x000,0x0A2,0x0A2,
                       0x000,0x048,0x0FE,0x07E,0x008,0x040,0x008,0x091,
                       0x018,0x005,0x018,0x005,0x04C,0x0A8,0x001};

VOID SetDefCRT2ExtRegs(USHORT BaseAddr);
BOOLEAN SetCRT2Group(USHORT BaseAddr,ULONG ROMAddr,USHORT ModeNo, ScrnInfoPtr pScrn);
USHORT GetRatePtrCRT2(ULONG ROMAddr, USHORT ModeNo);
BOOLEAN AjustCRT2Rate(ULONG ROMAddr);
VOID SaveCRT2Info(USHORT ModeNo);
VOID DisableLockRegs(VOID);
VOID DisableCRT2(VOID);
VOID DisableBridge(USHORT  BaseAddr);
VOID GetCRT2Data(ULONG ROMAddr,USHORT ModeNo);
VOID GetResInfo(ULONG ROMAddr,USHORT ModeNo);
VOID GetRAMDAC2DATA(ULONG ROMAddr,USHORT ModeNo);
VOID GetCRT2Ptr(ULONG ROMAddr,USHORT ModeNo);
VOID UnLockCRT2(USHORT BaseAddr);
VOID SetCRT2ModeRegs(USHORT BaseAddr,USHORT ModeNo);
VOID SetGroup1(USHORT BaseAddr,ULONG ROMAddr,USHORT ModeNo,ScrnInfoPtr pScrn);
VOID SetCRT2Offset(USHORT Part1Port,ULONG ROMAddr);
USHORT GetOffset(ULONG ROMAddr);
USHORT GetColorDepth(ULONG ROMAddr);
VOID SetCRT2FIFO(USHORT Part1Port,ULONG ROMAddr,USHORT ModeNo,ScrnInfoPtr pScrn);
USHORT GetVCLK(ULONG ROMAddr,USHORT ModeNo);
USHORT GetVCLKPtr(ULONG ROMAddr,USHORT ModeNo);
USHORT GetColorTh(ULONG ROMAddr);
USHORT GetMCLK(ULONG ROMAddr);
USHORT GetMCLKPtr(ULONG ROMAddr);
USHORT GetDRAMType(ULONG ROMAddr);
USHORT CalcDelayVB(VOID);
USHORT GetVCLK2Ptr(ULONG ROMAddr,USHORT ModeNo);
VOID SetCRT2Sync(USHORT BaseAddr,ULONG ROMAddr,USHORT ModeNo);
VOID GetCRT1Ptr(ULONG ROMAddr);
VOID SetRegANDOR(USHORT Port,USHORT Index,USHORT DataAND,USHORT DataOR);
USHORT GetVGAHT2(VOID);
VOID SetGroup2(USHORT  BaseAddr,ULONG ROMAddr);
VOID SetGroup3(USHORT  BaseAddr);
VOID SetGroup4(USHORT  BaseAddr,ULONG ROMAddr,USHORT ModeNo);
VOID SetCRT2VCLK(USHORT BaseAddr,ULONG ROMAddr,USHORT ModeNo);
VOID SetGroup5(USHORT  BaseAddr,ULONG ROMAddr);
VOID EnableCRT2(VOID);
VOID LoadDAC2(ULONG ROMAddr,USHORT Part5Port);
VOID WriteDAC2(USHORT Pdata,USHORT dl, USHORT ah, USHORT al, USHORT dh);
VOID LockCRT2(USHORT BaseAddr);
VOID SetLockRegs(VOID);
VOID EnableBridge(USHORT BaseAddr);
VOID GetVBInfo(USHORT BaseAddr,ULONG ROMAddr);
BOOLEAN BridgeIsEnable(USHORT BaseAddr);
BOOLEAN BridgeInSlave(VOID);
BOOLEAN GetLCDResInfo(USHORT P3d4);
VOID PresetScratchregister(USHORT P3d4);
BOOLEAN GetLCDDDCInfo(ScrnInfoPtr pScrn);
VOID SetTVSystem();
VOID LongWait();
USHORT GetQueueConfig();
VOID VBLongWait();
USHORT GetVCLKLen(ULONG ROMAddr);
BOOLEAN WaitVBRetrace(USHORT  BaseAddr);

extern USHORT MDA_DAC[];
extern USHORT CGA_DAC[];
extern USHORT EGA_DAC[];
extern USHORT VGA_DAC[];

extern USHORT   P3c4,P3d4,P3c0,P3ce,P3c2,P3ca,P3c6,P3c7,P3c8,P3c9,P3da;
extern USHORT   flag_clearbuffer; /* 0:no clear frame buffer 1:clear frame buffer */
extern int      RAMType;
extern int      ModeIDOffset,StandTable,CRT1Table,ScreenOffset,VCLKData,MCLKData, ECLKData;
extern int      REFIndex,ModeType;

extern VOID     SetReg1(USHORT, USHORT, USHORT);
extern VOID     SetReg3(USHORT, USHORT);
extern USHORT   GetReg1(USHORT, USHORT);
extern USHORT   GetReg2(USHORT);
extern BOOLEAN  SearchModeID(ULONG, USHORT);
extern BOOLEAN  GetRatePtr(ULONG, USHORT);
extern VOID SetReg4(USHORT, ULONG);
extern ULONG GetReg3(USHORT);

/* $XFree86: $ */

/* Header file for ViRGE server */

/* Declared in s3v_driver.c */
extern vgaVideoChipRec S3V;
extern vgaCRIndex, vgaCRReg;

/* Driver variables */

/* Driver data structure; this should contain all neeeded info for a mode */
typedef struct {     
   vgaHWRec std;
   unsigned char SR12, SR13, SR15, SR18; /* SR9-SR1C, extended sequencer */
   unsigned char Clock;
   unsigned char s3DacRegs[0x101];
   unsigned char CR31, CR32, CR33, CR34, CR36, CR3A, CR3B, CR3C, CR42, CR43;
   unsigned char CR51, CR53, CR58;   
   unsigned char CR59, CR5A, CR5D,CR5E, CR61, CR63, CR65, CR66, CR67, CR69; /* Video attrib. */
   unsigned char ColorStack[8]; /* S3 hw cursor color stack CR4A/CR4B */
   unsigned int  STREAMS[22];   /* Streams regs */
   unsigned int  MMPR0, MMPR2;   /* Memory port controller regs 0 and 2 */
} vgaS3VRec, *vgaS3VPtr;

/*
 * This is a convenience macro, so that entries in the driver structure
 * can simply be dereferenced with 'new->xxx'.
 */

#define new ((vgaS3VPtr)vgaNewVideoState)


/* PCI info structure */

typedef struct S3PCIInformation {
   int DevID;
   int ChipType;
   int ChipRev;
   unsigned long MemBase;
} S3PCIInformation;

/* Private data structure used for storing all variables needed in driver */
/* This is not exported outside of here, so no need to worry about packing */

typedef struct {
   int chip;
   unsigned short ChipId;
   pointer MmioMem;
   int MemOffScreen;
   int HorizScaleFactor;
   Bool STREAMSRunning;
   Bool NeedSTREAMS;
   int Width, Bpp,Bpl, ScissB;
} S3VPRIV;


/* Function prototypes */

S3PCIInformation * s3GetPCIInfo();


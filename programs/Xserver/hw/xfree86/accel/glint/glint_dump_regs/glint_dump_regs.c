/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/glint/glint_dump_regs/glint_dump_regs.c,v 1.1 1997/06/17 08:17:57 hohndel Exp $ */
#include <stdio.h>
#include <X11/Xmd.h>

#include "xf86_PCI.h"
#include "glint_regs.h"

#define PCI_VENDOR_3DLABS 0x3D3D
#define PCI_CHIP_3DLABS_300SX 0x0001
#define PCI_CHIP_3DLABS_500TX 0x0002
#define PCI_CHIP_3DLABS_DELTA 0x0003
#define PCI_CHIP_PERMEDIA     0x0004

/* #define FBMemoryCtl 0x1800 */

#define IBMRGB_INDEX_LOW 0x4020
#define IBMRGB_INDEX_DATA 0x4030 

#define MMIO_REGION 3
#define LINEAR_REGION 1

#define GLINT_WRITE_REG(v,r) \
	*(unsigned int *)((char*)GLINTMMIOBase+r) = v
#define GLINT_READ_REG(r) \
	*(unsigned int *)((char*)GLINTMMIOBase+r)

#define GLINT_WRITE_FRAMEBUFFER(v,p) \
        *(unsigned int *)((char *)glintVideoMem + p) = (unsigned int)v
#define GLINT_READ_FRAMEBUFFER(p) \
	*(unsigned int *)((char *)glintVideoMem + p)

typedef unsigned char *pointer;

volatile pointer GLINTMMIOBase;
volatile pointer glintVideoMem;

int xf86Verbose = 1;
int xf86PCIFlags = 1;

extern pointer xf86MapVidMem(int, int, pointer, unsigned);

extern void xf86ClearIOPortList(int);
extern void xf86AddIOPorts(int, int, unsigned *);
extern void xf86EnableIOPorts(int);
extern void xf86DisableIOPorts(int);

extern void outl(unsigned, unsigned long);
extern unsigned long inl(unsigned);

static void
glintEnableIO(int scrnIndex)
{
    /* This is enough to ensure that full I/O is enabled */
    unsigned pciIOPorts[] = { PCI_MODE1_ADDRESS_REG };
    int numPciIOPorts = sizeof(pciIOPorts) / sizeof(pciIOPorts[0]);

    xf86ClearIOPortList(scrnIndex);
    xf86AddIOPorts(scrnIndex, numPciIOPorts, pciIOPorts);
    xf86EnableIOPorts(scrnIndex);
}

static void
glintDisableIO(int scrnIndex)
{
    xf86DisableIOPorts(scrnIndex);
    xf86ClearIOPortList(scrnIndex);
}



void pix(unsigned int addr, unsigned int value)
{
  GLINT_WRITE_FRAMEBUFFER(value,addr);
}

FILE *wlogfile;

void set(unsigned int reg, unsigned int value)
{
  unsigned int written_value;
  unsigned long i;
  GLINT_WRITE_REG(value, reg);
  /* wait 268 Mio commands, i.e. about a second */
  for (i=0; i<0x1000000; i++);
  written_value = GLINT_READ_REG(reg);
  if (written_value != value) {
    printf("!!! ATTENTION: Register 0x%04x resetted to 0x%x (%u) ",
	   reg, written_value, written_value);
    printf("by graphic engine!!!\n"); 
    fprintf(wlogfile,"!!! ATTENTION: Register 0x%04x resetted to 0x%x (%u) ",
	   reg, written_value, written_value);
    fprintf(wlogfile,"by graphic engine!!!\n"); 
  }
}

void dacset(unsigned int reg, unsigned int value)
{
  unsigned int written_value;
  unsigned long i;
  GLINT_WRITE_REG(reg, IBMRGB_INDEX_LOW);
  GLINT_WRITE_REG(value, IBMRGB_INDEX_DATA);
  /* wait 268 Mio commands, i.e. about a second */
  for (i=0; i<0x1000000; i++);
  written_value = GLINT_READ_REG(IBMRGB_INDEX_DATA);
  if (written_value != value) {
    printf("!!! ATTENTION: Register 0x%04x resetted to 0x%x (%u) ",
	   reg, written_value, written_value);
    printf("by graphic engine!!!\n"); 
    fprintf(wlogfile,"!!! ATTENTION: Register 0x%04x resetted to 0x%x (%u) ",
	   reg, written_value, written_value);
    fprintf(wlogfile,"by graphic engine!!!\n"); 
  }
}

unsigned int get(unsigned int reg)
{
  return(GLINT_READ_REG(reg));
}

unsigned int dacget(unsigned int reg)
{
  GLINT_WRITE_REG(reg, IBMRGB_INDEX_LOW);
  return(GLINT_READ_REG(IBMRGB_INDEX_DATA));
}

char *wlogfilename="/tmp/write_regs.log";

int main(void)
{
  int i;
  int command;
  pciConfigPtr *pcrpp = NULL;
  pciConfigPtr pcrp = NULL;
  pciConfigPtr pcrpglint = NULL;
  pciConfigPtr pcrpdelta = NULL;
  int scrnIndex = 0;
  unsigned long glint300sx = 0;
  unsigned long glint500tx = 0;
  unsigned long glintdelta = 0;
  unsigned long glintcopro = 0;
  unsigned long basecopro = 0;
  unsigned long base3copro;
  unsigned long basedelta = 0;
  unsigned long *delta_pci_basep = NULL;
  int GLINTFrameBufferSize;
  int glintVideoRam;
  char dummy;

  volatile pointer glintVideoMemSave;
  unsigned long glintMemBase;

  unsigned int pix_addr;
  unsigned int pix_x;
  unsigned int set_reg, set_x;
  unsigned int get_reg, get_x;

  short int line;

  FILE *rfile, *wfile,*rlogfile;
  char *rfilename="read_regs.dat";
  char *wfilename="write_regs.dat";
  char *rlogfilename="/tmp/read_regs.log";
  char symbol[81];

  int failed;

  pcrpp = xf86scanpci(scrnIndex);

  i = -1;
  while ((pcrp = pcrpp[++i]) != (pciConfigPtr)NULL) {
    if (pcrp->_vendor == PCI_VENDOR_3DLABS)
      {
	switch(pcrp->_device)
	  {
	  case PCI_CHIP_3DLABS_300SX:
	    glint300sx = PCI_EN |
	      (pcrp->_bus << 16) |
	      (pcrp->_cardnum << 11) | (pcrp->_func << 8);
	    basecopro = pcrp->_base0;
	    pcrpglint = pcrp;
	    break;
	  case PCI_CHIP_3DLABS_500TX:
	    glint500tx = PCI_EN |
	      (pcrp->_bus << 16) |
	      (pcrp->_cardnum << 11) | (pcrp->_func << 8);
	    basecopro = pcrp->_base0;
	    pcrpglint = pcrp;
	    break;
	  case PCI_CHIP_3DLABS_DELTA:
	    glintdelta = PCI_EN |
	      (pcrp->_bus << 16) |
	      (pcrp->_cardnum << 11) | (pcrp->_func << 8);
	    basedelta = pcrp->_base0;
	    delta_pci_basep = &(pcrp->_base0);
	    pcrpdelta = pcrp;
	    break;
	  }
      }
  }
    
  if (!pcrpdelta)
    {
      /*
       * we only deal with cards that have a Delta installed as well
       */
      printf("oops ... no Glint Delta!\n");
      return -1; 
    }
  /*
   * due to a few bugs in the GLINT Delta we might have to relocate
   * the base address of config region of the Delta, if bit 17 of
   * the base addresses of config region of the Delta and the 500TX
   * or 300SX are different
   * We only handle config type 1 at this point
   */
  if( glintdelta && (glint500tx || glint300sx) )
    {
      if( glint500tx && glint300sx )
	{
	  /*
	   * can't handle if both of them are present
	   */
	  printf("oops ... Glint 500TX and Glint 300SX!\n");
	  return -1;
	}
      /*
       * now we know it is one or the other, so let's just use the one that is
     * set
     */
      glintcopro = glint500tx | glint300sx;
      
      if( (basedelta & 0x20000) ^ (basecopro & 0x20000) )
	{
	  /*
	   * if the base addresses are different at bit 17,
	   * we have to remap the base0 for the delta;
	   * as wrong as this looks, we can use the base3 of the
	   * 300SX/500TX for this. The delta is working as a bridge
	   * here and gives its own addresses preference. And we
	   * don't need to access base3, as this one is the bytw
	   * swapped local buffer which we don't need.
	   * Using base3 we know that the space is
	   * a) large enough
	   * b) free (well, almost)
	   *
	   * to be able to do that we need to enable IO
	   */
	  glintEnableIO(scrnIndex);
	  outl(PCI_MODE1_ADDRESS_REG, glintcopro | 0x1c); /* base3 */
	  base3copro  = inl(PCI_MODE1_DATA_REG);
	  if( (basecopro & 0x20000) ^ (base3copro & 0x20000) )
	    {
	      /*
	       * oops, still different; we know that base3 is at least
	       * 1 MB, so we just take 128k offset into it
	       */
	      base3copro += 0x20000;
	    }
	  outl(PCI_MODE1_ADDRESS_REG, glintdelta | 0x10);
	  outl(PCI_MODE1_DATA_REG,base3copro);
	  glintDisableIO(scrnIndex);
	  /*
	   * now update our internal structure accordingly
	   */
	  *delta_pci_basep = base3copro;
	}
    }

  /* IO Memory Mapping stuff */
  GLINTMMIOBase = xf86MapVidMem(0,MMIO_REGION,
				(pointer)pcrpdelta->_base0,0x20000);
	    
  printf("GLINTMMIOBase address at 0x%lx\n",
	 (unsigned long int) GLINTMMIOBase);
  
  /* Framebuffer Mapping stuff */
  glintMemBase = pcrpglint->_base2;
  printf("Framebuffer address at 0x%lx\n",glintMemBase);

  GLINTFrameBufferSize = GLINT_READ_REG(FBMemoryCtl);
  glintVideoRam = 1024 * (1 << ((GLINTFrameBufferSize &
					 0xE0000000) >> 29));
  printf("Videoram : %dk\n", glintVideoRam);

  glintVideoMem = xf86MapVidMem(scrnIndex, LINEAR_REGION,
				(pointer)(glintMemBase),
				glintVideoRam * 1024);

  printf("glintVideoMem address at 0x%lx\n",
	 (unsigned long int) glintVideoMem);
  
#if 0
  strcpy((char *)glintVideoMem,"This is the the framebuffer");
  printf("%s",glintVideoMem);
#endif

  do {
    printf("\n");
    printf("Available Commands: \n");
    printf(" 0: Exit\n");
    printf(" 1: Set Pixel\n");
    printf(" 2: Set value in register\n");
    printf(" 3: Get value from register\n");
    printf(" 4: FrameBuffer Test (32bpp)\n");
    printf(" 5: Write Registers according to config file\n");
    printf(" 6: Dump Registers according to config file\n");
    printf(" 7: Framebuffer Test (ASUS mainboards)\n");
    printf(" > ");
    scanf("%i",&command);
    
    switch(command)
      {
      case 0: 
	break;
      case 1:
	/* Set Pixel */
	printf("Offset(hex): ");
	scanf("%x",&pix_addr);
	printf("Value(dec): ");
	scanf("%x",&pix_x);
	printf("Setting pixel with offset 0x%x to value %i (0x%x)\n",
	       pix_addr,pix_x,pix_x);
        pix(pix_addr, pix_x);
	break;
      case 2:
	/* Set Register */
	printf("Register(hex): ");
	scanf("%x",&set_reg);
	printf("Value(hex): ");
	scanf("%x",&set_x);
	printf("Setting value 0x%x (%u) in Register 0x%x\n",
	       set_x, set_x, set_reg);
	set(set_reg,set_x);
	break;
      case 3:
	/* Read Register */
	printf("Register(hex): ");
	scanf("%x",&get_reg);
	get_x=get(get_reg);
	printf("Register 0x%x: 0x%x (%u)\n",
	       get_reg,get_x,get_x);
	break;
      case 4:
	/* Test FrameBuffer */
	printf("Press Return <>");
	scanf("%c",&dummy);
	pix_x=0x000000FF;
	printf("Value: 0x%08x (Blue)\n",pix_x);
	for (pix_addr=0; pix_addr<(8*1024*1024); pix_addr+=4)
	  {
	    pix(pix_addr, pix_x);
	    if (GLINT_READ_FRAMEBUFFER(pix_addr) != pix_x)
		{
		  printf("Value 0x%08x at address 0x%08x isn't 0x%08x\n",
			 GLINT_READ_FRAMEBUFFER(pix_addr),
			 pix_addr, pix_x);
		  break;
		}
	  }    
	printf("Press Return <>");
	scanf("%c",&dummy);
	pix_x=0x0000FF00;
	printf("Value: 0x%08x (Green)\n",pix_x);
	for (pix_addr=0; pix_addr<(8*1024*1024); pix_addr+=4)
	  {
	    pix(pix_addr,pix_x);
	    if (GLINT_READ_FRAMEBUFFER(pix_addr) != pix_x)
		{
		  printf("Value 0x%08x at address 0x%08x isn't 0x%08x\n",
			 GLINT_READ_FRAMEBUFFER(pix_addr),
			 pix_addr, pix_x);
		  break;
		}
	  }    
	printf("Press Return <>");
	scanf("%c",&dummy);
	pix_x=0x00FF0000;
	printf("Value: 0x%08x (Red) \n",pix_x);
	for (pix_addr=0; pix_addr<(8*1024*1024); pix_addr+=4)
	  {
	    pix(pix_addr,pix_x);
	    if (GLINT_READ_FRAMEBUFFER(pix_addr) != pix_x)
		{
		  printf("Value 0x%08x at address 0x%08x isn't 0x%08x\n",
			 GLINT_READ_FRAMEBUFFER(pix_addr),
			 pix_addr, pix_x);
		  break;
		}
	  }    
	printf("Press Return <>");
	scanf("%c",&dummy);
	pix_x=0xFF000000;
	printf("Value: 0x%08x (Black/Alpha)\n",pix_x);
	for (pix_addr=0; pix_addr<(8*1024*1024); pix_addr+=4)
	  {
	    pix(pix_addr,pix_x);
	    if (GLINT_READ_FRAMEBUFFER(pix_addr) != pix_x)
		{
		  printf("Value 0x%08x at address 0x%08x isn't 0x%08x\n",
			 GLINT_READ_FRAMEBUFFER(pix_addr),
			 pix_addr, pix_x);
		  break;
		}
	  }    

	break;
      case 5:
	/* Write Registers according to config file */
	if (!(wfile =(FILE*) fopen(wfilename,"rt"))) {
	  printf("\nCannot open file %s!\n", wfilename);
	  break;
	} 
	if (!(wlogfile =(FILE*) fopen(wlogfilename,"wt"))) {
	  printf("\nCannot open file %s!\n", wlogfilename);
	  break;
	} 
	line=1;
	do {
	  fscanf(wfile," %s", symbol);
	  if (!strcmp(symbol,"echo")) {
	    fscanf(wfile," %s",symbol);
	    printf("%-22s ",symbol);
	    fprintf(wlogfile,"%-20s ",symbol);
	  }
	  if (!strcmp(symbol,"echon")) {
	    fscanf(wfile," %s",symbol);
	    printf("%s\n",symbol);
	    fprintf(wlogfile,"%s\n",symbol);
	    line++;
	  }
	  if (!strcmp(symbol,"reg")) {
	    fscanf(wfile," %x %x", &set_reg, &set_x);
	    printf("(0x%04x) to value 0x%08x\n",
		   set_reg, set_x);
	    fprintf(wlogfile,"(0x%04x) to value 0x%08x\n",
		    set_reg, set_x);
	    set(set_reg, set_x); 
	    line++;
	  } 
	  if (!strcmp(symbol,"dacreg")) {
	    fscanf(wfile," %x %x", &set_reg, &set_x);
	    printf("(0x%04x) to value 0x%08x\n",
		   set_reg, set_x);
	    fprintf(wlogfile,"(0x%04x) to value 0x%08x\n",
		    set_reg, set_x);
	    dacset(set_reg, set_x); 
	    line++;
	  } 
	  if (!(line % 24)) {
	    scanf("%c",&dummy);
	    line=1;
	  }
	} while (!feof(wfile)); 
	fclose(wfile);
        fclose(wlogfile);
	break;
      case 6:
	/* Dump Registers according to config file */
	if (!(rfile =(FILE*) fopen(rfilename,"rt"))) {
	  printf("\nCannot open file %s!\n", rfilename);
	  break;
	} 
	if (!(rlogfile =(FILE*) fopen(rlogfilename,"wt"))) {
	  printf("\nCannot open file %s!\n", rlogfilename);
	  break;
	} 
	line=1;
	do {
	  fscanf(rfile," %s", symbol);
	  if (!strcmp(symbol,"echo")) {
	    fscanf(rfile," %s",symbol);
	    printf("%-22s ",symbol);
	    fprintf(rlogfile,"%-20s ",symbol);
	  }
	  if (!strcmp(symbol,"echon")) {
	    fscanf(rfile," %s",symbol);
	    printf("%s\n",symbol);
	    fprintf(rlogfile,"%s\n",symbol);
	    line++;
	  }
	  if (!strcmp(symbol,"reg")) {
	    fscanf(rfile," %x", &get_reg);
	    get_x=get(get_reg); 
	    printf("(0x%04x): 0x%08x (%10u)\n",
		   get_reg,get_x,get_x);
	    fprintf(rlogfile,"(0x%04x): 0x%08x (%10u)\n",
		   get_reg,get_x,get_x);
	    line++;
	  } 
	  if (!strcmp(symbol,"dacreg")) {
	    fscanf(rfile," %x", &get_reg);
	    get_x=dacget(get_reg); 
	    printf("(0x%04x): 0x%08x (%10u)\n",
		   get_reg,get_x,get_x);
	    fprintf(rlogfile,"(0x%04x): 0x%08x (%10u)\n",
		   get_reg,get_x,get_x);
	    line++;
	  } 
	  if ((line % 24) == 0) {
	    scanf("%c",&dummy);
	    line=1;
	  }
	} while (!feof(rfile)); 
	fclose(rfile);
        fclose(rlogfile);
	break;
      case 7:
	/* Test complete FrameBuffer (ASUS mainboards) */
	failed=1;
	for (pix_addr=0; pix_addr<(8*1024*1024); pix_addr++)
	  {
	    *(unsigned char*)glintVideoMem = 0x00;
	    if(*(unsigned char*)glintVideoMem == 0x00) {
	      *(unsigned char*)glintVideoMem = 0xFF;
	      if(*(unsigned char*)glintVideoMem == 0xFF) {
		printf("Framebuffer accessed successfully at adress 0x%08x",
		       pix_addr);
		failed=0;
	      }
	    }
	  }    
	if (failed)
	  printf("Can't access framebuffer on this mainboard at no adress!\n");
	else
	  printf("Can access framebuffer at several addressses (listed above).\n");
	break;
      default:
	break;
      }
  } while(command);
  
  return 0;
}









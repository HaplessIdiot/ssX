/* 
 * mio.c 
 * 
 * compile with   cc -O2 mio.c -o mio
 * 
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef linux
#include <unistd.h>
#include <sys/mman.h>
#include <asm/io.h>
#else
#ifdef DJGPP
#include <dpmi.h>
#include <pc.h>
#define inb(p)    inportb(p)
#define outb(v,p) outportb(p,v)
#else  /* BCC 4.0 */
#include <conio.h>
#include <dos.h>
#define inb(p)    inp(p)
#define outb(v,p) outp(p,v)
#endif
#endif

unsigned char cr53;

unsigned char *base;
unsigned long mm_base = 0;

void enable_io()
{
#ifdef linux
   iopl(3);
#endif
   outb(0x38, 0x3d4);   outb(0x48, 0x3d5);
   outb(0x39, 0x3d4);   outb(0xa5, 0x3d5);
}

void enable_mmio()
{
   outb(0x53, 0x3d4);
   cr53 = inb(0x3d5);
   if (mm_base == 0xa0000L)
      outb((cr53 & ~0x38) | 0x10, 0x3d5);
   else 
      outb((cr53 & ~0x38) | 0x18, 0x3d5);
}

void disable_mmio()
{
   outb(0x53, 0x3d4);
   outb(cr53, 0x3d5);
}

unsigned long get_base()
{
#ifndef PnP_PCI
#if defined(linux) || defined(DJGPP)
   outb(0x59, 0x3d4);
   mm_base  = inb(0x3d5) << 24;
   outb(0x5a, 0x3d4);
   mm_base |= inb(0x3d5) << 16;
#else
   mm_base = 0xa0000L;
#endif
#else
   FILE *f;
   char line[256];
   int vga=0;

   /* use /proc/pci for now... */
   if ((f=fopen("/proc/pci","r")) == NULL) {
      perror("can't open /proc/pci");
      exit(1);
   }

   while (fgets(line, 256, f) != NULL) {
      if (!strncmp("  Bus ",line,6))
	 vga = 0;
      if (!strncmp("    VGA compatible controller:", line, 30)) 
	 vga = 1;
      if (vga && !strncmp("      Non-prefetchable 32 bit memory at ", line, 40))
	 sscanf(line+40, "%lx", &mm_base);
   }
   fclose(f);

   if (!mm_base) {
      printf("can't find base address for VGA card\n");
      exit(1);
   }
#endif

   return mm_base;
}


unsigned char *map_base(long mm_base, long size)
{
   unsigned char *base = (unsigned char *) -1;

#ifdef linux
   int fd;

   if ((fd = open("/dev/mem", O_RDWR)) < 0) {
      perror("failed to open /dev/mem");
      exit(1);
   }
   
   base = (unsigned char*)mmap((caddr_t)0, size, PROT_READ|PROT_WRITE,
		      MAP_SHARED, fd, mm_base);
   close(fd);
#else
#ifdef DJGPP
   void main();
   __dpmi_version_ret vers;

   printf("__dpmi_get_version %d\n",__dpmi_get_version(&vers));
   printf("vers %d.%d flags %x cpu %d master_pic %d slave_pic %d\n",
	  vers.major, vers.minor, vers.flags, 
	  vers.cpu, vers.master_pic, vers.slave_pic);

   base = (char*)(((long)malloc(size + 4096) +0xfff) & ~0xfff);
   printf("main %p base %p\n", main, base);
	 
   if (__djgpp_map_physical_memory (base, size, mm_base) == -1) {
      perror("Could not mmap framebuffer1");
      mm_base = 0xa0000;
      if (__djgpp_map_physical_memory (base, size, mm_base) == -1) 
	 perror("Could not mmap framebuffer2");

      base = malloc(size + 4096);
      printf("main %p base %p\n", main, base);
      
      if (__djgpp_map_physical_memory (base, size, mm_base) == -1) {
	 perror("Could not mmap framebuffer3");
	 exit(1);
      }
   }
#else
#ifdef WINDOWS
#define STRICT
#include "windows.h"
       
   void main();
   typedef WORD SELECTOR;

   /*
      __A000h is an absolute value; by declaring it as a NEAR variable
      in our data segment we can take its "address" and get the
      16-bit absolute value.
      */
       
   extern BYTE NEAR CDECL _A000H;
       
   SELECTOR selVGA = (SELECTOR)&_A000H;
   
   base = MAKELP( selVGA, 0 );
#else
   base = (void*)(0xa000L << 16);
#endif
#endif
#endif
   
   if ((long)base == -1) {
      perror("Could not mmap framebuffer");
      exit(1);
   }

   return base;
}


#undef  inb
#undef  inw
#undef  inl
								 
#undef outb
#undef outw
#undef outl



#define  inb(p)		(*((volatile unsigned char* )(base + (p))))
#define  inw(p)		(*((volatile unsigned short*)(base + (p))))
#define  inl(p)		(*((volatile unsigned int*  )(base + (p))))
			 			        
#define outb(v,p)	(*((volatile unsigned char* )(base + (p)))) = (unsigned char)(v)
#define outw(v,p)	(*((volatile unsigned short*)(base + (p)))) = (unsigned short)(v)
#define outl(v,p)	(*((volatile unsigned long*  )(base + (p)))) = (unsigned long)(v)


#define getnumarg(a,i) ((a[++i][0]=='~') ? ~strtoul(a[i]+1,NULL,16) \
					 :  strtoul(a[i]  ,NULL,16))

void usage(char *progname)
{
   printf("%s [-v] [ [p|r]opcode[b|w|l] arg(s) ] ....\n"
	  "\n\tprepend p  to get output for output/mask operations\n"
	  "\t           and to omit output for input operations\n"
	  "\t        r  to re-read register before printing\n"
	  "\tappend  b  for  8 bit access (default)\n"
	  "\t        w  for 16 bit access\n"
	  "\t        l  for 32 bit access\n"
	  "\n\topcodes:\n"
	  "\t\ti   port                 input register\n"
	  "\t\to   port data            output 'data' to 'port'\n"
	  "\t\tm   port and   xor       output '(old & and) ^ xor'\n"
	  "\tindex access is implemented only for byte registers:\n"
	  "\t\tix  port index           input indexed register\n"
	  "\t\tox  port index data      output to indexed register\n"
	  "\t\tmx  port index and  xor  indexed output '(old & and) ^ xor'\n"
	  "\n\tnumbers are hex can be prepended by ~ to invert\n"
	  ,progname);

}




void main(int argc, char *argv[])
{
   int  i,r,x,p,rr;
   unsigned long v,o=0,and,xor;
   int verbose=0;
   char outbuf[10000], *op;
   outbuf[0] = 0;
   op = outbuf;

   if (argc<2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"-?")) {
      usage(argv[0]);
      exit(1);
   }

   if (!strcmp(argv[1],"-v")) {
      verbose++;
      argv++;
      argc--;
   }

   enable_io();
   mm_base = get_base();
   if (verbose) 
      printf("mmbase %08lx\n",mm_base);
#if 0
   disable_mmio(); exit(0);
#endif

   if (mm_base !=  0xa0000)
      mm_base |= 0x1000000;

   base = map_base(mm_base, 64*1024);
   
   enable_mmio();
   for(i=1; i<argc; i++) {
      if ((p  = (argv[i][0] == 'p'))) argv[i]++;      
      if ((rr = (argv[i][0] == 'r'))) argv[i]++;      
      if (!strcmp(argv[i],"i") || !strcmp(argv[i],"ib")) {
	 r = getnumarg(argv,i);
	 v = inb(r);
	 if (!p)
	    op += sprintf(op,"i  %4x  %02lx\n",r,v);
      }
      else if (!strcmp(argv[i],"o") || !strcmp(argv[i],"ob")) {
	 r = getnumarg(argv,i);
	 v = getnumarg(argv,i);
	 if (p || rr)
	    o = inb(r);
	 outb(v,r);
	 if (rr)
	    v = inb(r);
	 if (p || rr)
	    op += sprintf(op,"o  %4x  %02lx -> %02lx\n",r,o,v);
      }
      else if (!strcmp(argv[i],"iw")) {
	 r = getnumarg(argv,i);
	 v = inw(r);
	 if (!p)
	    op += sprintf(op,"i  %4x  %04lx\n",r,v);
      }
      else if (!strcmp(argv[i],"ow")) {
	 r = getnumarg(argv,i);
	 v = getnumarg(argv,i);
	 if (p || rr)
	    o = inw(r);
	 outw(v,r);
	 if (rr)
	    v = inw(r);
	 if (p || rr)
	    op += sprintf(op,"o  %4x  %04lx -> %04lx\n",r,o,v);
      }
      else if (!strcmp(argv[i],"il")) {
	 r = getnumarg(argv,i);
	 v = inl(r);
	 if (!p)
	    op += sprintf(op,"i  %4x  %08lx\n",r,v);
      }
      else if (!strcmp(argv[i],"ol")) {
	 r = getnumarg(argv,i);
	 v = getnumarg(argv,i);
	 if (p || rr)
	    o = inl(r);
	 outl(v,r);
	 if (rr)
	    v = inl(r);
	 if (p || rr)
	    op += sprintf(op,"o  %4x  %08lx -> %08lx\n",r,o,v);
      }
      else if (!strcmp(argv[i],"ix")) {
	 r = getnumarg(argv,i);
	 x = getnumarg(argv,i);
	 outb(x,r);
	 v = inb(r+1);
	 if (!p)
	    op += sprintf(op,"ix %4x  %02lx\n",r,v);
      }
      else if (!strcmp(argv[i],"ox")) {
	 r = getnumarg(argv,i);
	 x = getnumarg(argv,i);
	 v = getnumarg(argv,i);
	 if (p || rr) {
	    outb(x,r);
	    o = inb(r+1);
	 }
	 outb(x,r);
	 outb(v,r+1);
	 if (rr) {
	    outb(x,r);
	    v = inb(r+1);
	 }
	 if (p || rr)
	    op += sprintf(op,"ox %4x %02x  %02lx -> %02lx\n",r,x,o,v);
      }
      else if (!strcmp(argv[i],"m") || !strcmp(argv[i],"mb")) {
	 r   = getnumarg(argv,i);
	 and = getnumarg(argv,i);
	 xor = getnumarg(argv,i);
	 o = inb(r);
	 v = ((o & and) ^ xor) & 0xff;
	 outb(v,r);
	 if (rr)
	    v = inb(r);
	 if (p || rr)
	    op += sprintf(op,"m  %4x  %02lx -> %02lx\n",r,o,v);
      }
      else if (!strcmp(argv[i],"mw")) {
	 r   = getnumarg(argv,i);
	 and = getnumarg(argv,i);
	 xor = getnumarg(argv,i);
	 o = inw(r);
	 v = ((o & and) ^ xor) & 0xffff;
	 outw(v,r);
	 if (rr)
	    v = inw(r);
	 if (p || rr)
	    op += sprintf(op,"m  %4x  %04lx -> %04lx\n",r,o,v);
      }
      else if (!strcmp(argv[i],"ml")) {
	 r   = getnumarg(argv,i);
	 and = getnumarg(argv,i);
	 xor = getnumarg(argv,i);
	 o = inl(r);
	 v = (o & and) ^ xor;
	 if (rr)
	    v = inl(r);
	 if (p || rr)
	    op += sprintf(op,"m  %4x  %08lx -> %08lx\n",r,o,v);
	 outl(v,r);
      }
      else if (!strcmp(argv[i],"mx") || !strcmp(argv[i],"mxb")) {
	 r   = getnumarg(argv,i);
	 x   = getnumarg(argv,i);
	 and = getnumarg(argv,i);
	 xor = getnumarg(argv,i);
	 outb(x,r);
	 o = inb(r+1);
	 v = ((o & and) ^ xor) & 0xff;
	 outb(x,r);
	 outb(v,r+1);
	 if (rr) {
	    outb(x,r);
	    v = inb(r+1);
	 }
	 if (p || rr)
	    op += sprintf(op,"mx %4x %02x  %02lx -> %02lx\n",r,x,o,v);
      }
      else if (!strcmp(argv[i],"d")) {
	 long i;
	 for(i=0; i<0x10000; i++) {
	    if ((i&15) == 0) printf("%04lx",i);
	    if ((i&7)  == 0) printf(" ");
	    if ((i&3)  == 0) printf(" ");
	    printf(" %02x",inb(i));
	    if ((i&15) == 15) printf("\n");
	 }
      }
      else {
	 op += sprintf(op,"skipping unknown command #%d '%s'\n",i,argv[i]);
      }
   }
   disable_mmio();

   if (outbuf[0]) 
     printf("%s",outbuf);
   
   exit(0);
}

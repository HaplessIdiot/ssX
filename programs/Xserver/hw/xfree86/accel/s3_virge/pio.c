/* 
 * pio.c 
 * 
 * compile with   cc -O2 pio.c -o pio
 * 
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <asm/io.h>

#define getnumarg(a,i) ((a[++i][0]=='~') ? ~strtoul(a[i]+1,NULL,16) \
			                 :  strtoul(a[i]  ,NULL,16))

void usage(char *progname)
{
   printf("%s [ [p|r]opcode[b|w|l] arg(s) ] ....\n"
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

#ifdef linux
   iopl(3);
#endif

   if (argc<2 || !strcmp(argv[1],"-h") || !strcmp(argv[1],"-?")) {
      usage(argv[0]);
      exit(1);
   }

   for(i=1; i<argc; i++) {
      if ((p  = (argv[i][0] == 'p'))) argv[i]++;      
      if ((rr = (argv[i][0] == 'r'))) argv[i]++;      
      if (!strcmp(argv[i],"i") || !strcmp(argv[i],"ib")) {
	 r = getnumarg(argv,i);
	 v = inb(r);
	 if (!p)
	    printf("i  %4x  %02lx\n",r,v);
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
	    printf("o  %4x  %02lx -> %02lx\n",r,o,v);
      }
      else if (!strcmp(argv[i],"iw")) {
	 r = getnumarg(argv,i);
	 v = inw(r);
	 if (!p)
	    printf("i  %4x  %04lx\n",r,v);
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
	    printf("o  %4x  %04lx -> %04lx\n",r,o,v);
      }
      else if (!strcmp(argv[i],"il")) {
	 r = getnumarg(argv,i);
	 v = inl(r);
	 if (!p)
	    printf("i  %4x  %08lx\n",r,v);
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
	    printf("o  %4x  %08lx -> %08lx\n",r,o,v);
      }
      else if (!strcmp(argv[i],"ix")) {
	 r = getnumarg(argv,i);
	 x = getnumarg(argv,i);
	 outb(x,r);
	 v = inb(r+1);
	 if (!p)
	    printf("ix %4x  %02lx\n",r,v);
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
	    printf("ox %4x %02x  %02lx -> %02lx\n",r,x,o,v);
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
	    printf("m  %4x  %02lx -> %02lx\n",r,o,v);
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
	    printf("m  %4x  %04lx -> %04lx\n",r,o,v);
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
	    printf("m  %4x  %08lx -> %08lx\n",r,o,v);
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
	    printf("mx %4x %02x  %02lx -> %02lx\n",r,x,o,v);
      }
      else {
	 printf("skipping unknown command #%d '%s'\n",i,argv[i]);
      }
   }
}

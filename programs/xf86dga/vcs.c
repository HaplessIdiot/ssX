/* $XFree86$ */


#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/vt.h>

char *base;

int main(void){

   int   i = 0;
   int   fd, fd2;
   struct fbaddr fbaddr;

   fbaddr.fb_offset = 0xa0000;
   fbaddr.fb_size = 64 * 1024;

   if ((fd = open("/dev/vcs2", O_RDWR)) < 0)
   {
        fprintf(stderr, "xf86MapVidMem: failed to open /dev/mem (%s)\n",
                           strerror(errno));
        exit (0);
   }

   if ((fd2 = open("/dev/tty2", O_RDWR)) < 0)
   {
        fprintf(stderr, "xf86MapVidMem: failed to open /dev/mem (%s)\n",
                           strerror(errno));
        exit (0);
   }

   /* This requirers linux-1.3.45 or above */
   if (ioctl(fd2, VT_SETFBADDR, &fbaddr)) {
        fprintf(stderr, "ioctl: failed to open /dev/tty (%s)\n",
                           strerror(errno));
        exit (0);
   }

   base = (void *)mmap(0, 64*1024, PROT_READ|PROT_WRITE,
                             MAP_SHARED, fd, (off_t)0);

   if ( base == (char *) -1) {
	perror("vcs, mmap failed ");
	return -1;
   }
   printf ("vid = %X\n", base);


   srandom(getpid());
   for (i = 0; i< 3*1024; i++)
	base[i] = random() % 255;

   return(0);
}



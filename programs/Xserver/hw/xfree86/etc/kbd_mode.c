/* $XFree86$ */

/* Keyboard mode control program for 386BSD */

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "X.h"
#include "input.h"
#include "scrnintstr.h"

#include "xf86_OSlib.h"

static int fd;

void
msg (char* s)
{
  perror (s);
  close (fd);
  exit (-1);
}

int
main(int argc, char** argv)
{
    vtmode_t vtmode;
    Bool syscons = FALSE;

    if ((fd = open("/dev/vga",O_RDONLY,0)) <0)
      msg ("Cannot open /dev/vga");

    /* Check if syscons */
    if (ioctl(fd, VT_GETMODE, &vtmode) >= 0)
      syscons = TRUE;
    
    if (0 == strcmp (argv[1], "-u"))
      {
	if (syscons)
	  {
	    ioctl (fd, KDSKBMODE, K_RAW);
	  }
	else
	  {
	    if (ioctl (fd, CONSOLE_X_MODE_ON, 0) < 0)
	      {
	        close (fd);
	        exit (0);  /* Assume codrv, so nothing to do */
	      }
          }
      }
    else if (0 == strcmp (argv[1], "-a"))
      {
	if (syscons)
	  {
	    ioctl (fd, KDSKBMODE, K_XLATE);
	  }
	else
	  {
	    if (ioctl (fd, CONSOLE_X_MODE_OFF, 0) < 0)
	      {
	        close (fd);
	        exit (0);  /* Assume codrv, so nothing to do */
	      }
          }
      }
    else
      {
	close (fd);
	fprintf (stderr,"Usage: %s [-u|-a]\n",argv[0]);
	fprintf (stderr,"-u for sending up down key events in x mode.\n");
	fprintf (stderr,"-a for sending ascii keys in normal use.\n");
	exit (-1);
      }
    close (fd);
    exit (0);
}

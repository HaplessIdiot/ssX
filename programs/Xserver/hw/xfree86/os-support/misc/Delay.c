/* $XFree86: $ */
 
#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#include <time.h>

void
xf86UDelay(long usec)
{
    struct timeval start, interrupt;
    
    gettimeofday(&start,NULL);

    do {
	usleep(usec);
	gettimeofday(&interrupt,NULL);
	
	if ((usec = usec - (interrupt.tv_sec - start.tv_sec) * 1000000
	      - (interrupt.tv_usec - start.tv_usec)) < 0)
	    break;
	
    } while (1);
        
}


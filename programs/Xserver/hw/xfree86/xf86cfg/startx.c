/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 *
 * $XFree86$
 */

#include "config.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/*
 * Initialization
 */
static int xpid;
Display *DPY;

/*
 * Implementation
 */
Bool
startx(void)
{
    int timeout = 8;

    if (getenv("DISPLAY") != NULL)
	/* already running Xserver */
	return (False);

    if (XF86Config_path == NULL) {
	char *home, filename[PATH_MAX];

	if (system("XFree86 :8 -configure") != 0) {
	    fprintf(stderr, "Failed to run \"XFree86 -configure\".\n");
	    exit(1);
	}

	if ((home = getenv("HOME")) == NULL)
	    home = "/";

	XmuSnprintf(filename, sizeof(filename), "%s/XF86Config.new", home);

	/* this memory is never released, even if the value of XF86Config_path is
	 * changed.
	 */
	XF86Config_path = XtNewString(filename);
    }

    setenv("DISPLAY", ":8", 1);

    switch (xpid = fork()) {
	case 0: {
	    char path[PATH_MAX];

	    XmuSnprintf(path, sizeof(path), "%s/bin/XFree86", XFree86Dir);
	    execl(path, "X", ":8", /*"+xinerama",*/ "+accessx","-allowMouseOpenFail",
		  "-xf86config", XF86Config_path, NULL);
	    exit(-127);
	}   break;
	case -1:
	    fprintf(stderr, "Cannot fork.\n");
	    exit(1);
	    break;
	default:
	    break;
    }

    while (timeout > 0) {
	int status;

	sleep(timeout -= 2);
	if (waitpid(xpid, &status, WNOHANG | WUNTRACED) == xpid)
	    break;
	else {
	    DPY = XOpenDisplay(NULL);
	    if (DPY != NULL)
		break;
	}
    }

    if (DPY == NULL) {
	fprintf(stderr, "Cannot connect to X server.\n");
	exit(1);
    }

    return (True);
}

void
endx(void)
{
    if (xpid != 0)
	kill(xpid, SIGTERM);
}

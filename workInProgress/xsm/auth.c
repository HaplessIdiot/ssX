/* $XConsortium: auth.c,v 1.5 94/04/17 21:15:15 mor Exp $ */
/******************************************************************************

Copyright (c) 1993  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
******************************************************************************/

#include "xsm.h"


/*
 * Host Based Authentication Callback.  This callback is invoked if
 * the connecting client can't offer any authentication methods that
 * we can accept.  We can accept/reject based on the hostname.
 */

Bool
HostBasedProc (hostname)

char *hostname;

{
    return (0);	      /* For now, we don't support host based authentication */
}



/*
 * We use temporary files which contain commands to add/remove entries from
 * the .ICEauthority file.
 */

static void
write_iceauth (addfp, removefp, entry)

FILE		 *addfp;
FILE 		 *removefp;
IceAuthDataEntry *entry;

{
    fprintf (addfp,
	"add %s \"\" %s %s ",
	entry->protocol_name,
        entry->network_id,
        entry->auth_name);
    fprintfhex (addfp, entry->auth_data_length, entry->auth_data);
    fprintf (addfp, "\n");

    fprintf (removefp,
	"remove protoname=%s protodata=\"\" netid=%s authname=%s\n",
	entry->protocol_name,
        entry->network_id,
        entry->auth_name);
}



/*
 * Provide authentication data to clients that wish to connect
 */

#define MAGIC_COOKIE_LEN 16

Status
set_auth (count, listenObjs, authDataEntries)

int			count;
IceListenObj		*listenObjs;
IceAuthDataEntry	**authDataEntries;

{
    FILE	*addfp;
    FILE	*removefp;
    int		i;

    if (!(addfp = fopen (".xsm-add-auth", "w")))
	return (0);

    if (!(removefp = fopen (".xsm-rem-auth", "w")))
	return (0);

    *authDataEntries = (IceAuthDataEntry *) malloc (
	count * 2 * sizeof (IceAuthDataEntry));

    for (i = 0; i < count * 2; i += 2)
    {
	(*authDataEntries)[i].network_id =
	    IceGetListenConnectionString (listenObjs[i/2]);
	(*authDataEntries)[i].protocol_name = "ICE";
	(*authDataEntries)[i].auth_name = "MIT-MAGIC-COOKIE-1";

	(*authDataEntries)[i].auth_data =
	    IceGenerateMagicCookie (MAGIC_COOKIE_LEN);
	(*authDataEntries)[i].auth_data_length = MAGIC_COOKIE_LEN;

	(*authDataEntries)[i+1].network_id =
	    IceGetListenConnectionString (listenObjs[i/2]);
	(*authDataEntries)[i+1].protocol_name = "XSMP";
	(*authDataEntries)[i+1].auth_name = "MIT-MAGIC-COOKIE-1";

	(*authDataEntries)[i+1].auth_data = 
	    IceGenerateMagicCookie (MAGIC_COOKIE_LEN);
	(*authDataEntries)[i+1].auth_data_length = MAGIC_COOKIE_LEN;

	write_iceauth (addfp, removefp, &(*authDataEntries)[i]);
	write_iceauth (addfp, removefp, &(*authDataEntries)[i+1]);

	IceSetPaAuthData (2, &(*authDataEntries)[i]);

	IceSetHostBasedAuthProc (listenObjs[i/2], HostBasedProc);
    }

    fclose (addfp);
    fclose (removefp);

    system ("iceauth source .xsm-add-auth");

    return (1);
}



/*
 * Free up authentication data.
 */

void
free_auth (count, authDataEntries)

int			count;
IceAuthDataEntry 	*authDataEntries;

{
    /* Each transport has entries for ICE and XSMP */

    int i;

    for (i = 0; i < count * 2; i++)
    {
	free (authDataEntries[i].network_id);
	free (authDataEntries[i].auth_data);
    }

    free ((char *) authDataEntries);

    system ("iceauth source .xsm-rem-auth");
}

/* $XConsortium: xsm.c,v 1.35 94/04/17 21:15:19 mor Exp $ */
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

/*
 * X Session Manager.
 *
 * Written by Ralph Mor, X Consortium.
 *        and Jordan Brown, Quarterdeck Office Systems
 *
 * This program needs a fair amount of work to make it robust.
 * As it stands now, it serves as a useful test for session management.
 */

#include "xsm.h"
#include <signal.h>

AppResources app_resources;

#define Offset(field) XtOffsetOf(struct _AppResources, field)
static XtResource resources [] = {
    {"verbose",  "Verbose",  XtRBoolean, sizeof(Boolean), 
	 Offset(verbose), XtRImmediate, (XtPointer) False},
    {"debug",  "Debug",  XtRBoolean, sizeof(Boolean), 
	 Offset(debug), XtRImmediate, (XtPointer) False}
};
#undef Offset

static XrmOptionDescRec options[] = {
    {"-verbose",	"*verbose",	XrmoptionNoArg,		"TRUE"},
    {"-quiet",		"*verbose",	XrmoptionNoArg,		"FALSE"},
    {"-debug",		"*debug",	XrmoptionNoArg,		"TRUE"},
};

List		*PendingList;
ClientRec	*ClientList = NULL;
int		numClients = 0;
int		pingCount = 0;
int		saveDoneCount = 0;
int		interactCount = 0;
Bool		shutdownInProgress = False;
Bool		shutdownCancelled = False;
jmp_buf		JumpHere;

IceAuthDataEntry *authDataEntries = NULL;
int		numTransports = 0;

int		saveTypeData[] = {SmSaveGlobal,
			  	  SmSaveLocal,
		  		  SmSaveBoth};
Bool		shutdownData[] = {1, 0};
int		interactStyleData[] = {SmInteractStyleNone,
			       	       SmInteractStyleErrors,
		       		       SmInteractStyleAny};
Bool		fastData[] = {1, 0};

XtAppContext	appContext;

Widget		topLevel;

Widget		    mainWindow;

Widget		        listButton;
Widget			saveButton;
Widget			propButton;
Widget			pingButton;
Widget			startButton;

Widget		    savePopup;

Widget			saveForm;

Widget			    saveTypeLabel;
Widget			    saveTypeGlobal;
Widget			    saveTypeLocal;
Widget			    saveTypeBoth;

Widget			    shutdownLabel;
Widget			    shutdownYes;
Widget			    shutdownNo;

Widget			    interactStyleLabel;
Widget			    interactStyleNone;
Widget			    interactStyleErrors;
Widget			    interactStyleAny;

Widget			    fastLabel;
Widget			    fastYes;
Widget			    fastNo;

Widget			    saveOkButton;
Widget			    saveCancelButton;

Widget		    shutdownPopup;

Widget			shutdownDialog;

Widget			    shutdownOkButton;
Widget			    shutdownCancelButton;


void FreeClientInfo ();

extern Status InitWatchProcs ();
extern void restart_everything ();
extern void read_save ();
extern void write_save ();
extern Bool HostBasedProc ();
extern Status set_auth ();
extern void free_auth ();



print_prop(prop)
SmProp	*prop;
{
    int j;

    printf ("   Name:	%s\n", prop->name);
    printf ("   Type:	%s\n", prop->type);
    printf ("   Num values:	%d\n", prop->num_vals);
    if (strcmp(prop->type, SmCARD8) == 0) {
	char *card8 = prop->vals->value;
	int value = *card8;
	printf ("   Value 1:\t%d\n", value);
    } else {
	for (j = 0; j < prop->num_vals; j++) {
	    printf ("   Value %d:	%s\n", j + 1,
		    (char *) prop->vals[j].value);
	}
    }
    printf ("\n");
}



exit_sm ()
{
    if (app_resources.verbose)
	printf ("\nSESSION MANAGER GOING AWAY!\n");
    free_auth (numTransports, authDataEntries);
    exit (0);
}



SetInitialProperties(client, pendclient)
ClientRec	*client;
PendingClient	*pendclient;
{
    int			idx;
    SmProp		*prop;
    SmPropValue		*val;
    List		*pl;
    List		*vl;
    PendingProp		*pprop;
    PendingValue	*pval;

    if (app_resources.verbose)
	printf("Setting initial properties for %s\n", client->clientId);

    idx = 0;
    for(pl = ListFirst(pendclient->props); pl; pl = ListNext(pl)) {
	pprop = (PendingProp *)pl->thing;
	prop = (SmProp *)malloc(sizeof *prop);
	client->props[idx] = prop;
	prop->name = pprop->name;
	prop->type = pprop->type;
	prop->num_vals = ListCount(pprop->values);
	prop->vals =
	    (SmPropValue *)malloc(prop->num_vals * sizeof(SmPropValue));
	val = prop->vals;
	for(vl = ListFirst(pprop->values); vl; vl = ListNext(vl)) {
	    pval = (PendingValue *)vl->thing;
	    val->value = pval->value;
	    val->length = pval->length;
	    val++;
	}
	ListFreeAll(pprop->values);
	free(pprop);
	idx++;
    }
    client->numProps = idx;

    ListFreeAll(pendclient->props);
    XtFree(pendclient->clientId);
    free(pendclient);
}



/*
 * Session Manager callbacks
 */

Status
RegisterClientProc (smsConn, managerData, previousId)

SmsConn 	smsConn;
SmPointer 	managerData;
char 		*previousId;

{
    ClientRec	*client = (ClientRec *) managerData;
    char 	*id;
    List	*cl;

    if (app_resources.verbose) {
	printf (
	"On IceConn fd = %d, received REGISTER CLIENT [Previous Id = %s]\n",
	IceConnectionNumber (client->ice_conn),
	previousId ? previousId : "NULL");
	printf ("\n");
    }

    if (previousId)
    {
	id = (char *)malloc (strlen (previousId) + 1);
	strcpy (id, previousId);
    }
    else
	id = SmsGenerateClientID (smsConn);

    SmsRegisterClientReply (smsConn, id);
    client->clientId = id;
    client->clientHostname = SmsClientHostName (smsConn);

    if (app_resources.verbose) {
	printf (
	"On IceConn fd = %d, sent REGISTER CLIENT REPLY [Client Id = %s]\n",
	IceConnectionNumber (client->ice_conn), id);
	printf ("\n");
    }

    if(previousId) {
	for(cl = ListFirst(PendingList); cl; cl = ListNext(cl)) {
	    if(!strcmp(((PendingClient *)cl->thing)->clientId, previousId)) {
		SetInitialProperties(client, (PendingClient *)cl->thing);
		ListFreeOne(cl);
		break;
	    }
	}
	free (previousId);
    } else {
	SmsSaveYourself(smsConn, SmSaveLocal, False, SmInteractStyleNone,
			False);
    }

    return (1);
}



void
InteractRequestProc (smsConn, managerData, dialogType)

SmsConn 	smsConn;
SmPointer  	managerData;
int		dialogType;

{
    ClientRec	*client = (ClientRec *) managerData;

    if (app_resources.verbose) {
	printf ("Client Id = %s, received INTERACT REQUEST [Dialog Type = ",
		client->clientId);
	if (dialogType == SmDialogError)
	    printf ("Error]\n");
	else if (dialogType == SmDialogNormal)
	    printf ("Normal]\n");
	else
	    printf ("Error in SMlib: should have checked for bad value]\n");
    }

    client->interactPending = True;
    interactCount++;
}



void
InteractDoneProc (smsConn, managerData, cancelShutdown)
    SmsConn	smsConn;
    SmPointer 	managerData;
    Bool	cancelShutdown;

{
    ClientRec	*client = (ClientRec *) managerData;

    if (app_resources.verbose) {
	printf (
	"Client Id = %s, received INTERACT DONE [Cancel Shutdown = %s]\n",
	client->clientId, cancelShutdown ? "True" : "False");
    }

    client->interactPending = False;

    if (cancelShutdown && !shutdownCancelled) {
	shutdownCancelled = True;
	for (client = ClientList; client; client = client->next) {
	    SmsShutdownCancelled (client->smsConn);
	    if (app_resources.verbose) 
		printf ("Client Id = %s, sent SHUTDOWN CANCELLED\n",
			client->clientId);
	}
    }
}



void
SaveYourselfReqProc (smsConn, managerData, saveType,
    shutdown, interactStyle, fast, global)

SmsConn     smsConn;
SmPointer   managerData;
int	    saveType;
Bool	    shutdown;
int         interactStyle;
Bool        fast;
Bool        global;

{
    if (app_resources.verbose) 
	printf("SAVE YOURSELF REQUEST not supported!\n");
}



void
SaveYourselfDoneProc (smsConn, managerData, success)
    SmsConn     smsConn;
    SmPointer 	managerData;
    Bool	success;

{
    ClientRec	*client = (ClientRec *) managerData;

    if (app_resources.verbose) 
	printf("Client Id = %s, received SAVE YOURSELF DONE [Success = %s]\n",
	       client->clientId, success ? "True" : "False");

    if (shutdownCancelled && client->interactPending)
	client->interactPending = False;

    saveDoneCount++;
}



void
CloseConnectionProc (smsConn, managerData, count, reasonMsgs)
    SmsConn 	smsConn;
    SmPointer  	managerData;
    int		count;
    char 	**reasonMsgs;

{
    ClientRec	*client = (ClientRec *) managerData;
    ClientRec 	*next = client->next;
    ClientRec	*ptr;
    int i;

    if (app_resources.verbose) {
	printf("Client Id = %s, received CONNECTION CLOSED\n",
	       client->clientId);

	for (i = 0; i < count; i++)
	    printf ("   Reason string %d: %s\n", i + 1, reasonMsgs[i]);
	printf ("\n");
    }

    SmFreeReasons (count, reasonMsgs);
    SmsCleanUp (smsConn);

    if (app_resources.verbose) {
	printf ("ICE Connection closed, IceConn fd = %d\n",
		IceConnectionNumber (client->ice_conn));
	printf ("\n");
    }

    IceSetShutdownNegotiation (client->ice_conn, False);
    IceCloseConnection (client->ice_conn);

    if (client == ClientList)
    {
	FreeClientInfo (client);
	ClientList = next;
    }
    else
    {
	ptr = ClientList;
	while (ptr && ptr->next != client)
	    ptr = ptr->next;

	if (ptr->next == client)
	{
	    FreeClientInfo (client);
	    ptr->next = next;
	}
    }

    numClients--;

    if (shutdownInProgress && numClients == 0)
    {
	exit_sm ();
    }
}



void
SetProperty(client, prop)
ClientRec	*client;
SmProp		*prop;
{
    int	idx, j;

    for (j = 0; j < client->numProps; j++)
	if (strcmp (prop->name, client->props[j]->name) == 0)
	{
	    SmFreeProperty (client->props[j]);
	    break;
	}

    if (j < client->numProps)
	idx = j;
    else
    {
	idx = client->numProps;
	client->numProps++;

	if (client->numProps > MAX_PROPS)
	    return;
    }

    client->props[idx] = prop;
}



void
DeleteProperty(client, propname)
ClientRec	*client;
char		*propname;
{
    int	j;

    for (j = 0; j < client->numProps; j++)
	if (strcmp (propname, client->props[j]->name) == 0)
	{
	    SmFreeProperty (client->props[j]);

	    if (j < client->numProps - 1)
		client->props[j] = client->props[client->numProps - 1];

	    client->numProps--;
	    break;
	}
}



void
SetPropertiesProc (smsConn, managerData, numProps, props)

SmsConn 	smsConn;
SmPointer 	managerData;
int		numProps;
SmProp 		**props;

{
    ClientRec	*client = (ClientRec *) managerData;
    int		i;

    if (app_resources.verbose) {
	printf ("Client Id = %s, received SET PROPERTIES ", client->clientId);
	printf ("[Num props = %d]\n", numProps);
    }

    for (i = 0; i < numProps; i++) {
	if(app_resources.verbose)
	    print_prop(props[i]);
	SetProperty(client, props[i]);
    }
    free ((char *) props);
}



void
DeletePropertiesProc (smsConn, managerData, numProps, propNames)

SmsConn 	smsConn;
SmPointer 	managerData;
int		numProps;
char **		propNames;

{
    ClientRec	*client = (ClientRec *) managerData;
    int		i;

    if (app_resources.verbose) {
	printf ("Client Id = %s, received DELETE PROPERTIES ",
	    client->clientId);
	printf ("[Num props = %d]\n", numProps);
    }

    for (i = 0; i < numProps; i++) {
	if(app_resources.verbose)
	    printf ("   Name:	%s\n", propNames[i]);
	DeleteProperty(client, propNames[i]);
	free (propNames[i]);
    }
    free ((char *) propNames);
}



void
GetPropertiesProc (smsConn, managerData)

SmsConn 	smsConn;
SmPointer 	managerData;

{
    ClientRec	*client = (ClientRec *) managerData;
    int		i;

    if (app_resources.verbose) {
	printf ("Client Id = %s, received GET PROPERTIES\n", client->clientId);
	printf ("\n");

	for (i = 0; i < client->numProps; i++) {
	    print_prop(client->props[i]);
	}
	printf ("\n");
    }

    SmsReturnProperties (smsConn, client->numProps, client->props);

    if (app_resources.verbose) {
	printf ("Client Id = %s, sent PROPERTIES REPLY [Num props = %d]\n",
		client->clientId, client->numProps);
    }
}



void
PingReplyProc (ice_conn, client_data)

IceConn		ice_conn;
IcePointer	client_data;

{
    ClientRec *client = (ClientRec *) client_data;
    printf ("Client Id = %s, received PING REPLY\n", client->clientId);
    pingCount--;
}



Status
NewClientProc (smsConn, managerData, maskRet, callbacksRet, failureReasonRet)

SmsConn		smsConn;
SmPointer  	managerData;
unsigned long	*maskRet;
SmsCallbacks	*callbacksRet;
char 		**failureReasonRet;

{
    ClientRec *newClient = (ClientRec *) malloc (sizeof (ClientRec));
    *maskRet = 0;

    if (!newClient)
    {
	char *str = "Memory allocation failed";

	if ((*failureReasonRet = (char *) malloc (strlen (str) + 1)) != NULL)
	    strcpy (*failureReasonRet, str);

	return (0);
    }

    newClient->smsConn = smsConn;
    newClient->ice_conn = SmsGetIceConnection (smsConn);
    newClient->clientId = NULL;
    newClient->clientHostname = NULL;
    newClient->interactPending = False;
    newClient->numProps = 0;
    newClient->next = ClientList;

    ClientList = newClient;
    numClients++;

    if (app_resources.verbose) {
	printf("On IceConn fd = %d, client set up session mngmt protocol\n\n",
	       IceConnectionNumber (newClient->ice_conn));
    }

    /*
     * Set up session manager callbacks.
     */

    *maskRet |= SmsRegisterClientProcMask;
    callbacksRet->register_client.callback 	= RegisterClientProc;
    callbacksRet->register_client.manager_data  = (SmPointer) newClient;

    *maskRet |= SmsInteractRequestProcMask;
    callbacksRet->interact_request.callback 	= InteractRequestProc;
    callbacksRet->interact_request.manager_data = (SmPointer) newClient;

    *maskRet |= SmsInteractDoneProcMask;
    callbacksRet->interact_done.callback	= InteractDoneProc;
    callbacksRet->interact_done.manager_data    = (SmPointer) newClient;

    *maskRet |= SmsSaveYourselfRequestProcMask;
    callbacksRet->save_yourself_request.callback     = SaveYourselfReqProc;
    callbacksRet->save_yourself_request.manager_data = (SmPointer) newClient;

    *maskRet |= SmsSaveYourselfDoneProcMask;
    callbacksRet->save_yourself_done.callback 	   = SaveYourselfDoneProc;
    callbacksRet->save_yourself_done.manager_data  = (SmPointer) newClient;

    *maskRet |= SmsCloseConnectionProcMask;
    callbacksRet->close_connection.callback 	 = CloseConnectionProc;
    callbacksRet->close_connection.manager_data  = (SmPointer) newClient;

    *maskRet |= SmsSetPropertiesProcMask;
    callbacksRet->set_properties.callback 	= SetPropertiesProc;
    callbacksRet->set_properties.manager_data   = (SmPointer) newClient;

    *maskRet |= SmsDeletePropertiesProcMask;
    callbacksRet->delete_properties.callback	= DeletePropertiesProc;
    callbacksRet->delete_properties.manager_data   = (SmPointer) newClient;

    *maskRet |= SmsGetPropertiesProcMask;
    callbacksRet->get_properties.callback	= GetPropertiesProc;
    callbacksRet->get_properties.manager_data   = (SmPointer) newClient;

    return (1);
}



/*
 * Xt callbacks
 */

void
ListClientsXtProc (w, client_data, callData)
    Widget	w;
    XtPointer 	client_data;
    XtPointer 	callData;

{
    ClientRec *client = ClientList;

    printf ("\n");
    if (client == NULL) {
	printf ("There are no clients registered with the SM\n");
    } else {
	printf ("The following client IDs are registered with the SM:\n");
	printf ("\n");
    }
    while (client) {
	printf ("  Host = %s, ID = %s\n",
	    client->clientHostname, client->clientId);
	client = client->next;
    }
    printf ("\n");
}



void
SaveYourselfXtProc (w, client_data, callData)

Widget		w;
XtPointer 	client_data;
XtPointer 	callData;

{
    Position	x, y, rootx, rooty;

    XtVaGetValues (mainWindow, XtNx, &x, XtNy, &y, NULL);
    XtTranslateCoords (mainWindow, x, y, &rootx, &rooty);

    if (ClientList == NULL)
    {
	XtMoveWidget (shutdownPopup, rootx, rooty);
	XtPopup (shutdownPopup, XtGrabNone);
    }
    else
    {
	XawToggleSetCurrent (saveTypeBoth,
	    (XtPointer) &saveTypeData[2]);
	XawToggleSetCurrent (shutdownNo,
	    (XtPointer) &shutdownData[1]);
	XawToggleSetCurrent (interactStyleAny,
	    (XtPointer) &interactStyleData[2]);
	XawToggleSetCurrent (fastNo,
	    (XtPointer) &fastData[1]);

	XtMoveWidget (savePopup, rootx, rooty);
	XtPopup (savePopup, XtGrabNone);
    }

    XtSetSensitive (mainWindow, 0);
}



void
SaveOkXtProc (w, client_data, callData)

Widget		w;
XtPointer 	client_data;
XtPointer 	callData;

{
    ClientRec	*client;
    XtPointer	ptr;
    int		saveType;
    Bool	shutdown;
    int		interactStyle;
    Bool	fast;
    char	*_saveType;
    char	*_shutdown;
    char	*_interactStyle;
    char	*_fast;

    ptr = XawToggleGetCurrent (saveTypeGlobal /* just 1 of the group */);
    saveType = *((int *) ptr);

    ptr = XawToggleGetCurrent (shutdownYes /* just 1 of the group */);
    shutdown = *((Bool *) ptr);

    ptr = XawToggleGetCurrent (interactStyleNone /* just 1 of the group */);
    interactStyle = *((int *) ptr);

    ptr = XawToggleGetCurrent (fastYes /* just 1 of the group */);
    fast = *((Bool *) ptr);

    if (saveType == SmSaveGlobal)
	_saveType = "Global";
    else if (saveType == SmSaveLocal)
	_saveType = "Local";
    else
	_saveType = "Both";

    if (shutdown)
	_shutdown = "True";
    else
	_shutdown = "False";

    if (interactStyle == SmInteractStyleNone)
	_interactStyle = "None";
    else if (interactStyle == SmInteractStyleErrors)
	_interactStyle = "Errors";
    else
	_interactStyle = "Any";

    if (fast)
	_fast = "True";
    else
	_fast = "False";

    XtSetSensitive (savePopup, 0);

    for (client = ClientList; client; client = client->next)
    {
	SmsSaveYourself (client->smsConn,
	    saveType, shutdown, interactStyle, fast);

	if (app_resources.verbose) {
	    printf ("Client Id = %s, sent SAVE YOURSELF [", client->clientId);
	    printf ("Save Type = %s, Shutdown = %s, ", _saveType, _shutdown);
	    printf ("Interact Style = %s, Fast = %s]\n",
		    _interactStyle, _fast);
	}
    }
    if (app_resources.verbose) {
	printf ("\n");
	printf ("Sent SAVE YOURSELF to all clients.  Waiting for\n");
	printf ("SAVE YOURSELF DONE or INTERACT REQUEST from each client.\n");
	printf ("\n");
    }

    saveDoneCount = 0;
    interactCount = 0;

    while (saveDoneCount + interactCount < numClients) {
	XtAppProcessEvent (appContext, XtIMAll);
    }

    if (app_resources.verbose) {
	printf ("\n");
	printf ("Received %d SAVE YOURSELF DONEs, %d INTERACT REQUESTS\n",
		saveDoneCount, interactCount);
    }

    if (interactCount == 0 && saveDoneCount != numClients) {
	if (app_resources.verbose) {
	    printf ("\n");
	    printf ("INTERNAL ERROR IN PSEUDO-SM!  EXITING!\n");
	}
	exit (1);
    }

    if (interactCount > 0) {
	
	if (app_resources.verbose)
	    printf ("\n");

	client = ClientList;
	while (client) {
	    if (shutdownCancelled) {
		break;
	    }
	    else if (client->interactPending) {
		SmsInteract (client->smsConn);
		if (app_resources.verbose) {
		    printf ("Client Id = %s, sent INTERACT\n",
			    client->clientId);
		}
		while (client->interactPending) {
		    XtAppProcessEvent (appContext, XtIMAll);
		}
	    }
	    client = client->next;
	}

	if (app_resources.verbose) {
	    if (shutdownCancelled)
		printf ("\nThe shutdown was cancelled by a user\n\n");
	    else
		printf ("\nDone interacting with all clients\n\n");
	}
    }

    while (saveDoneCount < numClients) {
	XtAppProcessEvent (appContext, XtIMAll);
    }

    if (app_resources.verbose)
	printf ("\nAll clients issued SAVE YOURSELF DONE\n\n");

    write_save();

    if (shutdown && shutdownCancelled) {
	shutdownCancelled = False;
    } else if (shutdown) {
	shutdownInProgress = True;
	client = ClientList;
	while (client) {
	    SmsDie (client->smsConn);
	    if (app_resources.verbose)
		printf ("Client Id = %s, sent DIE\n", client->clientId);
	    client = client->next;
	}
    } else {
	client = ClientList;
	while (client) {
	    SmsSaveComplete (client->smsConn);
	    if (app_resources.verbose)
		printf ("Client Id = %s, sent SAVE COMPLETE\n",
		    client->clientId);
	    client = client->next;
	}
    }

    if (!shutdownInProgress) {
	XtPopdown (savePopup);
	XtSetSensitive (savePopup, 1);
	XtSetSensitive (mainWindow, 1);
    }
}



void
SaveCancelXtProc (w, client_data, callData)

Widget		w;
XtPointer 	client_data;
XtPointer 	callData;

{
    XtPopdown (savePopup);
    XtSetSensitive (mainWindow, 1);
}



void
ShutdownOkXtProc (w, client_data, callData)

Widget		w;
XtPointer 	client_data;
XtPointer 	callData;

{
    write_save();
    exit_sm ();
}



void
ShutdownCancelXtProc (w, client_data, callData)

Widget		w;
XtPointer 	client_data;
XtPointer 	callData;

{
    XtPopdown (shutdownPopup);
    XtSetSensitive (mainWindow, 1);
}



void
ListPropXtProc (w, client_data, callData)

Widget		w;
XtPointer 	client_data;
XtPointer 	callData;

{
    ClientRec *client = ClientList;

    if (client == NULL) {
	printf ("There are no clients registered with the SM\n");
	return;
    }

    while (client) {
	if (client->numProps == 0) {
	    printf("Client Id = %s, no properties are set\n",
		   client->clientId);
	} else {
	    int i;

	    printf ("Client Id = %s, the following properties are set:\n",
		    client->clientId);
	    for (i = 0; i < client->numProps; i++)
		print_prop(client->props[i]);
	}
	client = client->next;
    }
}



void
StartXtProc (w, client_data, callData)

Widget		w;
XtPointer 	client_data;
XtPointer 	callData;

{
    switch(fork()) {
	case -1:
	    perror("fork");
	    break;
	case 0:
	    execlp("xsmclient", "xsmclient", (char *)NULL);
	    perror("xsmclient");
	    _exit(255);
	default:
	    break;
    }
}



void
PingXtProc (w, client_data, callData)

Widget		w;
XtPointer 	client_data;
XtPointer 	callData;

{
    ClientRec *client = ClientList;

    pingCount = 0;
    printf ("\n");
    if (client == NULL ) {
	printf ("There are no clients registered with the SM\n");
	printf ("\n");
	return;
    }
    while (client) {
	IcePing (client->ice_conn, PingReplyProc, (IcePointer) client);
	pingCount++;
	printf ("Client Id = %s, sent PING\n", client->clientId);
	client = client->next;
    }
    printf ("\n");
    while (pingCount > 0)
	XtAppProcessEvent(appContext, XtIMAll);
    printf ("\n");
}



/*
 * Xt callback invoked when a client attempts to connect.
 */

void
newConnectionXtProc (client_data, source, id)

XtPointer	client_data;
int 		*source;
XtInputId	*id;

{
    IceConn 	ice_conn;
    char	*connstr;
    IceAcceptStatus status;

    ice_conn = IceAcceptConnection((IceListenObj) client_data, &status);
    if (! ice_conn) {
	if (app_resources.verbose)
	    printf ("IceAcceptConnection failed\n");
    } else {
	IceConnectStatus cstatus;

	while ((cstatus = IceConnectionStatus (ice_conn))==IceConnectPending) {
	    XtAppProcessEvent (appContext, XtIMAll);
	}

	if (cstatus == IceConnectAccepted) {
	    if (app_resources.verbose) {
		printf ("ICE Connection opened by client, IceConn fd = %d, ",
			IceConnectionNumber (ice_conn));
		connstr = IceConnectionString (ice_conn);
		printf ("Accept at networkId %s\n", connstr);
		free (connstr);
		printf ("\n");
	    }
	} else {
	    if (app_resources.verbose)
	    {
		if (cstatus == IceConnectIOError)
		    printf ("IO error opening ICE Connection!\n");
		else
		    printf ("ICE Connection rejected!\n");
	    }

	    IceCloseConnection (ice_conn);
	}
    }
}



/*
 * Install IO error handler.  This will detect clients that break their
 * connection with the SM unexpectidly.
 */

void
myIOErrorHandler (ice_conn)

IceConn 	ice_conn;

{
    if (ClientList == NULL)
    {
	/*
	 * The client must have disconnected before the ICE connection
	 * became valid.  Example: ICE authentication failed.
	 */

	IceSetShutdownNegotiation (ice_conn, False);
	IceCloseConnection (ice_conn);
    }
    else
    {
	ClientRec *ptr = ClientList;
	ClientRec *prev = NULL;
	
	while (ptr && ptr->ice_conn != ice_conn)
	{
	    prev = ptr;
	    ptr = ptr->next;
	}

	if (!ptr)
	{
	    fprintf (stderr, "Internal error; couldn't find ice_conn\n");
	    exit (1);
	}
	else
	{
	    if (prev == NULL)
		ClientList = ptr->next;
	    else
		prev->next = ptr->next;

	    SmsCleanUp (ptr->smsConn);
	    FreeClientInfo (ptr);
	    
	    if (app_resources.verbose)
	    {
		printf ("ICE Connection terminated (fd = %d)\n",
			IceConnectionNumber (ice_conn));
		printf ("\n");
	    }

	    IceSetShutdownNegotiation (ice_conn, False);
	    IceCloseConnection (ice_conn);

	    numClients--;

	    if (shutdownInProgress && numClients == 0)
	    {
		exit_sm ();
	    }
	}
    }


    /*
     * We can't return.  Must do a long jump.  Make sure any
     * popups are uppopped.
     */

    XtPopdown (shutdownPopup);
    XtPopdown (savePopup);
    XtSetSensitive (shutdownPopup, 1);
    XtSetSensitive (savePopup, 1);
    XtSetSensitive (mainWindow, 1);

    longjmp (JumpHere, 1);
}    



/*
 * Add toggle button
 */

Widget
AddToggle (widgetName, parent, label, state, radioGroup, radioData,
    fromHoriz, fromVert)

char 		*widgetName;
Widget 		parent;
char 		*label;
int 		state;
Widget 		radioGroup;
XtPointer 	radioData;
Widget 		fromHoriz;
Widget 		fromVert;

{
    Widget 		toggle;
    XtTranslations	translations;

    toggle = XtVaCreateManagedWidget (
	widgetName, toggleWidgetClass, parent,
	XtNlabel, label,
        XtNstate, state,
        XtNradioGroup, radioGroup,
        XtNradioData, radioData,
        XtNfromHoriz, fromHoriz,
        XtNfromVert, fromVert,
        NULL);

    translations = XtParseTranslationTable ("<Btn1Down>,<Btn1Up>:set()\n");
    XtOverrideTranslations (toggle, translations);

    return (toggle);
}



void
FreeClientInfo (client)

ClientRec *client;

{
    if (client)
    {
	int i;

	XtFree (client->clientId);
	XtFree (client->clientHostname);

	for (i = 0; i < client->numProps; i++)
	    SmFreeProperty (client->props[i]);

	free ((char *) client);
    }
}


static void Syntax(call)
    char *call;
{
    (void) fprintf(stderr, "usage: %s [-verbose] [-quiet]\n", call);
    exit(2);
}



main(argc, argv)
    int  argc;
    char **argv;
{
    IceListenObj *listenObjs;
    char 	*networkIds;
    int  	i;
    char	*p;
    char *	progName;
    char 	errormsg[256];
    static	char environment_name[] = "SESSION_MANAGER";

    umask (0077);	/* disallow non-owner access */

    p = strrchr(argv[0], '/');
    progName = (p ? p + 1 : argv[0]);
    topLevel = XtVaAppInitialize (&appContext, "SAMPLE-SM", options, 
	XtNumber(options), &argc, argv, NULL,
        XtNjoinSession, 0,	/* We are the SM */
	NULL);
	
    if (argc > 1) Syntax(progName);
    XtGetApplicationResources(topLevel, (XtPointer) &app_resources,
			      resources, XtNumber(resources), NULL, 0);
    
    /*
     * Set my own IO error handler.
     */

    IceSetIOErrorHandler (myIOErrorHandler);


    /*
     * Ignore SIGPIPE
     */

    signal (SIGPIPE, SIG_IGN);


    /*
     * Init SM lib
     */

    /* if these are errors they should write to stderr or an error file. */
    if (!SmsInitialize ("SAMPLE-SM", "1.0",
	NewClientProc, NULL,
	HostBasedProc, 256, errormsg))
    {
	printf ("%s\n", errormsg);
	exit (1);
    }

    if (!IceListenForConnections (&numTransports, &listenObjs,
	256, errormsg))
    {
	printf ("%s\n", errormsg);
	exit (1);
    }

    if (!set_auth (numTransports, listenObjs, &authDataEntries))
    {
	printf ("Could not set authorization\n");
	exit (1);
    }

    InitWatchProcs (appContext);

    mainWindow = XtCreateManagedWidget (
	"mainWindow", boxWidgetClass, topLevel, NULL, 0);

    listButton = XtVaCreateManagedWidget (
	"listButton", commandWidgetClass, mainWindow,
	XtNlabel, "List all registered clients",
	NULL);

    XtAddCallback (listButton, XtNcallback, ListClientsXtProc, 0);

    saveButton = XtVaCreateManagedWidget (
	"saveButton", commandWidgetClass, mainWindow,
	XtNlabel, "Save Yourself with option to Shutdown",
	NULL);

    XtAddCallback (saveButton, XtNcallback, SaveYourselfXtProc, 0);

    if (app_resources.debug)
    {
	propButton = XtVaCreateManagedWidget (
	    "propButton", commandWidgetClass, mainWindow,
	    XtNlabel, "List properties of each client",
	    NULL);

	XtAddCallback (propButton, XtNcallback, ListPropXtProc, 0);
    }

    pingButton = XtVaCreateManagedWidget (
	"pingButton", commandWidgetClass, mainWindow,
	XtNlabel, "Ping all clients",
	NULL);

    XtAddCallback (pingButton, XtNcallback, PingXtProc, 0);

    startButton = XtVaCreateManagedWidget (
	"startButton", commandWidgetClass, mainWindow,
	XtNlabel, "Start a new xsmclient",
	NULL);

    XtAddCallback (startButton, XtNcallback, StartXtProc, 0);


    /*
     * Pop up for Save Yourself button.
     */

    savePopup = XtVaCreatePopupShell (
	"savePopup", transientShellWidgetClass, topLevel,
	XtNtitle, "Save Yourself Parameters",
	NULL);
    
    saveForm = XtCreateManagedWidget (
	"saveForm", formWidgetClass, savePopup, NULL, 0);

    saveTypeLabel = XtVaCreateManagedWidget (
	"saveTypeLabel", labelWidgetClass, saveForm,
	XtNlabel, "Save Type     ",
        XtNfromHoriz, NULL,
        XtNfromVert, NULL,
        XtNborderWidth, 0,
	NULL);

    saveTypeGlobal = AddToggle (
	"saveTypeGlobal", 			/* widgetName */
	saveForm,				/* parent */
        "Global",				/* label */
	0,					/* state */
        NULL,					/* radioGroup */
        (XtPointer) &saveTypeData[0],		/* radioData */
        saveTypeLabel,				/* fromHoriz */
        NULL					/* fromVert */
    );

    saveTypeLocal = AddToggle (
	"saveTypeLocal", 			/* widgetName */
	saveForm,				/* parent */
        "Local",				/* label */
	0,					/* state */
        saveTypeGlobal,				/* radioGroup */
        (XtPointer) &saveTypeData[1],		/* radioData */
        saveTypeGlobal,				/* fromHoriz */
        NULL					/* fromVert */
    );

    saveTypeBoth = AddToggle (
	"saveTypeBoth", 			/* widgetName */
	saveForm,				/* parent */
        "Both",					/* label */
	1,					/* state */
        saveTypeGlobal,				/* radioGroup */
        (XtPointer) &saveTypeData[2],		/* radioData */
        saveTypeLocal,				/* fromHoriz */
        NULL					/* fromVert */
    );


    shutdownLabel = XtVaCreateManagedWidget (
	"shutdownLabel", labelWidgetClass, saveForm,
	XtNlabel, "Shutdown?",
        XtNfromHoriz, NULL,
        XtNfromVert, saveTypeLabel,
        XtNborderWidth, 0,
	NULL);

    shutdownYes = AddToggle (
	"shutdownYes", 				/* widgetName */
	saveForm,				/* parent */
        "Yes",					/* label */
	0,					/* state */
        NULL,					/* radioGroup */
        (XtPointer) &shutdownData[0],		/* radioData */
        saveTypeLabel,				/* fromHoriz */
        saveTypeLabel				/* fromVert */
    );

    shutdownNo = AddToggle (
	"shutdownNo", 				/* widgetName */
	saveForm,				/* parent */
        "No",					/* label */
	1,					/* state */
        shutdownYes,				/* radioGroup */
        (XtPointer) &shutdownData[1],		/* radioData */
        shutdownYes,				/* fromHoriz */
        saveTypeLabel				/* fromVert */
    );


    interactStyleLabel = XtVaCreateManagedWidget (
	"interactStyleLabel", labelWidgetClass, saveForm,
	XtNlabel, "Interact Style",
        XtNfromHoriz, NULL,
        XtNfromVert, shutdownLabel,
        XtNborderWidth, 0,
	NULL);

    interactStyleNone = AddToggle (
	"interactStyleNone", 			/* widgetName */
	saveForm,				/* parent */
        "None",					/* label */
	0,					/* state */
        NULL,					/* radioGroup */
        (XtPointer) &interactStyleData[0],	/* radioData */
        saveTypeLabel,				/* fromHoriz */
        shutdownLabel				/* fromVert */
    );

    interactStyleErrors = AddToggle (
	"interactStyleErrors", 			/* widgetName */
	saveForm,				/* parent */
        "Errors",				/* label */
	0,					/* state */
        interactStyleNone,			/* radioGroup */
        (XtPointer) &interactStyleData[1],	/* radioData */
        interactStyleNone,			/* fromHoriz */
        shutdownLabel				/* fromVert */
    );

    interactStyleAny = AddToggle (
	"interactStyleAny", 			/* widgetName */
	saveForm,				/* parent */
        "Any",					/* label */
	1,					/* state */
        interactStyleNone,			/* radioGroup */
        (XtPointer) &interactStyleData[2],	/* radioData */
        interactStyleErrors,			/* fromHoriz */
        shutdownLabel				/* fromVert */
    );


    fastLabel = XtVaCreateManagedWidget (
	"fastLabel", labelWidgetClass, saveForm,
	XtNlabel, "Fast?",
        XtNfromHoriz, NULL,
        XtNfromVert, interactStyleLabel,
        XtNborderWidth, 0,
	NULL);

    fastYes = AddToggle (
	"fastYes", 				/* widgetName */
	saveForm,				/* parent */
        "Yes",					/* label */
	0,					/* state */
        NULL,					/* radioGroup */
        (XtPointer) &fastData[0],		/* radioData */
        saveTypeLabel,				/* fromHoriz */
        interactStyleLabel			/* fromVert */
    );

    fastNo = AddToggle (
	"fastNo", 				/* widgetName */
	saveForm,				/* parent */
        "No",					/* label */
	1,					/* state */
        fastYes,				/* radioGroup */
        (XtPointer) &fastData[1],		/* radioData */
        fastYes,				/* fromHoriz */
        interactStyleLabel			/* fromVert */
    );


    saveOkButton = XtVaCreateManagedWidget (
	"saveOkButton",	commandWidgetClass, saveForm,
	XtNlabel, "OK",
        XtNfromHoriz, NULL,
        XtNfromVert, fastLabel,
        XtNvertDistance, 30,
        NULL);
    
    XtAddCallback (saveOkButton, XtNcallback, SaveOkXtProc, 0);

    saveCancelButton = XtVaCreateManagedWidget (
	"saveCancelButton", commandWidgetClass, saveForm,
        XtNlabel, "Cancel",
        XtNfromHoriz, saveOkButton,
        XtNfromVert, fastLabel,
        XtNvertDistance, 30,
        NULL);

    XtAddCallback (saveCancelButton, XtNcallback, SaveCancelXtProc, 0);


    /*
     * Pop up Shutdown confirmation.
     */

    shutdownPopup = XtVaCreatePopupShell ("shutdownPopup",
	transientShellWidgetClass, topLevel,
	XtNtitle, "Shutdown Prompt", NULL);
    
    shutdownDialog = XtVaCreateManagedWidget ("shutdownDialog",
	dialogWidgetClass, shutdownPopup,
	XtNlabel, "There are no active clients.  OK to shutdown?",
        NULL);					      

    shutdownOkButton = XtVaCreateManagedWidget ("shutdownOkButton",
	commandWidgetClass, shutdownDialog, XtNlabel, "OK", NULL);
    
    XtAddCallback (shutdownOkButton, XtNcallback, ShutdownOkXtProc, 0);

    shutdownCancelButton = XtVaCreateManagedWidget ("shutdownCancelButton",
	commandWidgetClass, shutdownDialog, XtNlabel, "Cancel", NULL);

    XtAddCallback (shutdownCancelButton, XtNcallback, ShutdownCancelXtProc, 0);


    /*
     * Realize top level.
     */

    XtRealizeWidget (topLevel);
    
    for (i = 0; i < numTransports; i++)
    {
	XtAppAddInput (appContext,
	    IceGetListenConnectionNumber (listenObjs[i]),
	    (XtPointer) XtInputReadMask,
	    newConnectionXtProc, (XtPointer) listenObjs[i]);
    }

    /* the sizeof includes the \0, so we don't need to count the '=' */
    networkIds = IceComposeNetworkIdList (numTransports, listenObjs);
    p = (char *) malloc((sizeof environment_name) + strlen(networkIds) + 1);
    if(!p) nomem();
    sprintf(p, "%s=%s", environment_name, networkIds);
    putenv(p);

    if (app_resources.verbose || app_resources.debug)
	printf ("setenv %s %s\n", environment_name, networkIds);

    read_save();
    restart_everything();

    if (app_resources.verbose)
	printf ("Waiting for connections...\n");

    free (networkIds);
    setjmp (JumpHere);
    XtAppMainLoop (appContext);
}

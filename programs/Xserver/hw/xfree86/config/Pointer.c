/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Pointer.c,v 1.1.2.5 1997/07/22 08:40:42 dawes Exp $ */
/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
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
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

extern LexRec val;

static xf86ConfigSymTabRec PointerTab[] =
{
	{PROTOCOL, "protocol"},
	{EMULATE3, "emulate3buttons"},
	{EM3TIMEOUT, "emulate3timeout"},
	{ENDSUBSECTION, "endsubsection"},
	{ENDSECTION, "endsection"},
#ifndef OSMOUSE_ONLY
	{PDEVICE, "device"},
	{PDEVICE, "port"},
	{BAUDRATE, "baudrate"},
	{SAMPLERATE, "samplerate"},
	{CLEARDTR, "cleardtr"},
	{CLEARRTS, "clearrts"},
	{CHORDMIDDLE, "chordmiddle"},
	{REPEATEDMIDDLE, "repeatedmiddle"},
#endif
	{DEVICE_NAME, "devicename"},
	{BUTTONS, "buttons"},
	{ALWAYSCORE, "alwayscore"},
	{-1, ""},
};

#define CLEANUP freePointer

XF86ConfPointerPtr
parsePointerSection (void)
{
	parsePrologue (XF86ConfPointerPtr, XF86ConfPointerRec)

		while ((token = xf86GetToken (PointerTab)) != ENDSECTION)
	{
		switch (token)
		{
		case PROTOCOL:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Protocol");
			ptr->pntr_protocol = val.str;
			break;
		case PDEVICE:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Device");
			ptr->pntr_device = val.str;
			break;
		case EMULATE3:
			ptr->pntr_emulate3Buttons = TRUE;
			break;
		case EM3TIMEOUT:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "Emulate3Timeout");
			ptr->pntr_emulate3Timeout = val.num;
			break;
		case CHORDMIDDLE:
			ptr->pntr_chordMiddle = TRUE;
			break;
		case BUTTONS:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "Buttons");
			ptr->pntr_buttons = val.num;
			break;
		case BAUDRATE:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "BaudRate");
			ptr->pntr_baudrate = val.num;
			break;
		case CLEARDTR:
			ptr->pntr_clearDtr = TRUE;
			break;
		case CLEARRTS:
			ptr->pntr_clearRts = TRUE;
			break;
		case ALWAYSCORE:
			ptr->pntr_alwaysCore = TRUE;
			break;
		case EOF_TOKEN:
			Error (UNEXPECTED_EOF_MSG, NULL);
			break;
		default:
			Error (INVALID_KEYWORD_MSG, xf86TokenString ());
			break;
		}
	}

#ifdef DEBUG
	printf ("Pointer section parsed\n");
#endif

	return ptr;
}

#undef CLEANUP

void
printPointerSection (FILE * cf, XF86ConfPointerPtr ptr)
{
	if (ptr == NULL)
		return;

	if (ptr->pntr_protocol)
		fprintf (cf, "\tProtocol     \"%s\"\n", ptr->pntr_protocol);
	if (ptr->pntr_device)
		fprintf (cf, "\tDevice       \"%s\"\n", ptr->pntr_device);
	if (ptr->pntr_emulate3Buttons)
		fprintf (cf, "\tEmulate3Buttons\n");
	if (ptr->pntr_emulate3Timeout)
		fprintf (cf, "\tEmulate3Timeout  %d\n", ptr->pntr_emulate3Timeout);
	if (ptr->pntr_chordMiddle)
		fprintf (cf, "\tChordMiddle\n");
	if (ptr->pntr_buttons)
		fprintf (cf, "\tButtons      %d\n", ptr->pntr_buttons);
	if (ptr->pntr_baudrate)
		fprintf (cf, "\tBaudRate     %d\n", ptr->pntr_baudrate);
	if (ptr->pntr_samplerate)
		fprintf (cf, "\tSampleRate   %d\n", ptr->pntr_samplerate);
	if (ptr->pntr_clearDtr)
		fprintf (cf, "\tClearDTR\n");
	if (ptr->pntr_clearRts)
		fprintf (cf, "\tClearRTS\n");
	if (ptr->pntr_alwaysCore)
		fprintf (cf, "\tAlwaysCore\n");
}

void
freePointer (XF86ConfPointerPtr ptr)
{
	if (ptr == NULL)
		return;

	TestFree (ptr->pntr_protocol);
	TestFree (ptr->pntr_device);

	xf86conffree (ptr);
}

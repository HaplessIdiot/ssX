/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Keyboard.c,v 1.1.2.3 1997/07/22 13:50:54 dawes Exp $ */
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

static xf86ConfigSymTabRec KeyboardTab[] =
{
	{ENDSECTION, "endsection"},
	{KPROTOCOL, "protocol"},
	{AUTOREPEAT, "autorepeat"},
	{SERVERNUM, "servernumlock"},
	{XLEDS, "xleds"},
	{VTINIT, "vtinit"},
	{LEFTALT, "leftalt"},
	{RIGHTALT, "rightalt"},
	{RIGHTALT, "altgr"},
	{SCROLLLOCK, "scrolllock"},
	{RIGHTCTL, "rightctl"},
	{VTSYSREQ, "vtsysreq"},
	{PANIX106, "panix106"},
	{XKBKEYMAP, "xkbkeymap"},
	{XKBCOMPAT, "xkbcompat"},
	{XKBTYPES, "xkbtypes"},
	{XKBKEYCODES, "xkbkeycodes"},
	{XKBGEOMETRY, "xkbgeometry"},
	{XKBSYMBOLS, "xkbsymbols"},
	{XKBDISABLE, "xkbdisable"},
	{XKBRULES, "xkbrules"},
	{XKBMODEL, "xkbmodel"},
	{XKBLAYOUT, "xkblayout"},
	{XKBVARIANT, "xkbvariant"},
	{XKBOPTIONS, "xkboptions"},
	{-1, ""},
};

static xf86ConfigSymTabRec KeyMapTab[] =
{
	{CONF_KM_META, "meta"},
	{CONF_KM_COMPOSE, "compose"},
	{CONF_KM_MODESHIFT, "modeshift"},
	{CONF_KM_MODELOCK, "modelock"},
	{CONF_KM_SCROLLLOCK, "scrolllock"},
	{CONF_KM_CONTROL, "control"},
	{-1, ""},
};

#define CLEANUP freeKeyboard

XF86ConfKeyboardPtr
parseKeyboardSection (void)
{
	int ntoken;
	int indx;
	parsePrologue (XF86ConfKeyboardPtr, XF86ConfKeyboardRec)

		while ((token = xf86GetToken (KeyboardTab)) != ENDSECTION)
	{
		switch (token)
		{
		case KPROTOCOL:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Protocol");
			ptr->keyb_protocol = val.str;
			break;
		case AUTOREPEAT:
			if (xf86GetToken (NULL) != NUMBER)
				Error (AUTOREPEAT_MSG, NULL);
			ptr->keyb_kbdDelay = val.num;
			if (xf86GetToken (NULL) != NUMBER)
				Error (AUTOREPEAT_MSG, NULL);
			ptr->keyb_kbdRate = val.num;
			break;
		case SERVERNUM:
			ptr->keyb_serverNumLock = TRUE;
			break;
		case XLEDS:
			while ((token = xf86GetToken (NULL)) == NUMBER)
				ptr->keyb_xleds |= 1L << (val.num - 1);
			xf86UnGetToken (token);
			break;
		case LEFTALT:
		case RIGHTALT:
		case SCROLLLOCK:
		case RIGHTCTL:
			switch (token)
			{
			case LEFTALT:
				indx = CONF_K_INDEX_LEFTALT;
				break;
			case RIGHTALT:
				indx = CONF_K_INDEX_RIGHTALT;
				break;
			case SCROLLLOCK:
				indx = CONF_K_INDEX_SCROLLLOCK;
				break;
			case RIGHTCTL:
				indx = CONF_K_INDEX_RIGHTCTL;
				break;
			}
			ntoken = xf86GetToken (KeyMapTab);
			switch (ntoken)
			{
			case CONF_KM_META:
			case CONF_KM_COMPOSE:
			case CONF_KM_MODESHIFT:
			case CONF_KM_MODELOCK:
			case CONF_KM_SCROLLLOCK:
			case CONF_KM_CONTROL:
				ptr->keyb_specialKeyMap[indx] = ntoken;
				break;
			case EOF_TOKEN:
				xf86ParseError (UNEXPECTED_EOF_MSG);
				CLEANUP (ptr);
				return (NULL);
				break;

			default:
				Error (INVALID_KEYWORD_MSG, xf86TokenString ());
				break;
			}
			break;
		case VTINIT:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "VTInit");
			ptr->keyb_vtinit = val.str;
			break;
		case VTSYSREQ:
			ptr->keyb_vtSysreq = TRUE;
			break;
		case XKBDISABLE:
			ptr->keyb_xkbDisable = TRUE;
			break;
		case XKBKEYMAP:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBKeymap");
			ptr->keyb_xkbkeymap = val.str;
			break;
		case XKBCOMPAT:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBCompat");
			ptr->keyb_xkbcompat = val.str;
			break;
		case XKBTYPES:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBTypes");
			ptr->keyb_xkbtypes = val.str;
			break;
		case XKBKEYCODES:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBKeycodes");
			ptr->keyb_xkbkeycodes = val.str;
			break;
		case XKBGEOMETRY:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBGeometry");
			ptr->keyb_xkbgeometry = val.str;
			break;
		case XKBSYMBOLS:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBSymbols");
			ptr->keyb_xkbsymbols = val.str;
			break;
		case XKBRULES:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBRules");
			ptr->keyb_xkbrules = val.str;
			break;
		case XKBMODEL:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBModel");
			ptr->keyb_xkbmodel = val.str;
			break;
		case XKBLAYOUT:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBLayout");
			ptr->keyb_xkblayout = val.str;
			break;
		case XKBVARIANT:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBVariant");
			ptr->keyb_xkbvariant = val.str;
			break;
		case XKBOPTIONS:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "XKBOptions");
			ptr->keyb_xkboptions = val.str;
			break;
		case PANIX106:
			ptr->keyb_panix106 = TRUE;
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
	printf ("Keyboard section parsed\n");
#endif

	return ptr;
}

#undef CLEANUP

void
printKeyboardSection (FILE * cf, XF86ConfKeyboardPtr ptr)
{
	if (ptr == NULL)
		return;

	if (ptr->keyb_protocol)
		fprintf (cf, "\tProtocol     \"%s\"\n", ptr->keyb_protocol);
	if (ptr->keyb_kbdDelay || ptr->keyb_kbdRate)
		fprintf (cf, "\tAutoRepeat   %d %d\n", ptr->keyb_kbdDelay,
						ptr->keyb_kbdRate);
	if (ptr->keyb_xkbkeymap)
		fprintf (cf, "\tXkbKeymap    \"%s\"\n", ptr->keyb_xkbkeymap);

}

void
freeKeyboard (XF86ConfKeyboardPtr ptr)
{
	if (ptr == NULL)
		return;

	TestFree (ptr->keyb_protocol);
	TestFree (ptr->keyb_vtinit);
	TestFree (ptr->keyb_xkbkeymap);
	TestFree (ptr->keyb_xkbkeymap);
	TestFree (ptr->keyb_xkbcompat);
	TestFree (ptr->keyb_xkbtypes);
	TestFree (ptr->keyb_xkbkeycodes);
	TestFree (ptr->keyb_xkbgeometry);
	TestFree (ptr->keyb_xkbsymbols);
	TestFree (ptr->keyb_xkbrules);
	TestFree (ptr->keyb_xkbmodel);
	TestFree (ptr->keyb_xkblayout);
	TestFree (ptr->keyb_xkbvariant);
	TestFree (ptr->keyb_xkboptions);

	xf86conffree (ptr);
}

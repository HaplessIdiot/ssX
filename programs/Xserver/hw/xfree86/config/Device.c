/* $XFree86: xc/programs/Xserver/hw/xfree86/config/Device.c,v 1.1 1998/01/24 16:57:41 hohndel Exp $ */
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

static
xf86ConfigSymTabRec DeviceTab[] =
{
	{ENDSECTION, "endsection"},
	{IDENTIFIER, "identifier"},
	{VENDOR, "vendorname"},
	{BOARD, "boardname"},
	{CHIPSET, "chipset"},
	{RAMDAC, "ramdac"},
	{DACSPEED, "dacspeed"},
	{CLOCKS, "clocks"},
	{OPTION, "option"},
	{VIDEORAM, "videoram"},
	{SPEEDUP, "speedup"},
	{NOSPEEDUP, "nospeedup"},
	{CLOCKPROG, "clockprog"},
	{BIOSBASE, "biosbase"},
	{MEMBASE, "membase"},
	{IOBASE, "iobase"},
	{DACBASE, "dacbase"},
	{COPBASE, "copbase"},
	{POSBASE, "posbase"},
	{INSTANCE, "instance"},
	{CLOCKCHIP, "clockchip"},
	{S3MNADJUST, "s3mnadjust"},
	{S3MCLK, "s3mclk"},
	{S3MCLK, "mclk"},
	{CHIPID, "chipid"},
	{CHIPREV, "chiprev"},
	{VGABASEADDR, "vgabase"},
	{S3REFCLK, "s3refclk"},
	{S3BLANKDELAY, "s3blankdelay"},
	{TEXTCLOCKFRQ, "textclockfreq"},
	{MEMCLOCK, "set_memclk"},
	{MEMCLOCK, "set_mclk"},
	{CARD, "card"},
	{DRIVER, "driver"},
	{PCI_TAG, "pcitag"},
	{-1, ""},
};

#define CLEANUP freeDeviceList

XF86ConfDevicePtr
parseDeviceSection (void)
{
	int i;
	int has_ident = FALSE;
	parsePrologue (XF86ConfDevicePtr, XF86ConfDeviceRec)

	ptr->dev_pci_tag = 0xffffffff;
	while ((token = xf86GetToken (DeviceTab)) != ENDSECTION)
	{
		switch (token)
		{
		case IDENTIFIER:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Identifier");
			ptr->dev_identifier = val.str;
			has_ident = TRUE;
			break;
		case VENDOR:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Vendor");
			ptr->dev_vendor = val.str;
			break;
		case BOARD:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Board");
			ptr->dev_board = val.str;
			break;
		case CHIPSET:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Chipset");
			ptr->dev_chipset = val.str;
			break;
		case CARD:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Card");
			ptr->dev_card = val.str;
			break;
		case DRIVER:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Driver");
			ptr->dev_driver = val.str;
			break;
		case RAMDAC:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Ramdac");
			ptr->dev_ramdac = val.str;
			break;
		case DACSPEED:
			for (i = 0; i < CONF_MAXDACSPEEDS; i++)
				ptr->dev_dacSpeeds[i] = 0;
			if (xf86GetToken (NULL) != NUMBER)
			{
				Error (DACSPEED_MSG, CONF_MAXDACSPEEDS);
			}
			else
			{
				ptr->dev_dacSpeeds[0] = (int) (val.realnum * 1000.0 + 0.5);
				for (i = 1; i < CONF_MAXDACSPEEDS; i++)
				{
					if (xf86GetToken (NULL) == NUMBER)
						ptr->dev_dacSpeeds[i] = (int)
							(val.realnum * 1000.0 + 0.5);
					else
					{
						xf86UnGetToken (token);
						break;
					}
				}
			}
			break;
		case VIDEORAM:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "VideoRam");
			ptr->dev_videoram = val.num;
			break;
		case SPEEDUP:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Speedup");
			ptr->dev_speedup = val.str;
			break;
		case NOSPEEDUP:
			TestFree (ptr->dev_speedup);
			ptr->dev_speedup = ConfigStrdup("none");
			break;
		case CLOCKPROG:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "ClockProg");
			if (val.str[0] != '/')
				Error ("Full pathname must be given for ClockProg \"%s\"",
					   val.str);
			ptr->dev_clockprog = val.str;
			if (xf86GetToken (NULL) == NUMBER)
			{
				ptr->dev_textclockvalue = (int) (val.realnum * 1000.0 + 0.5);
			}
			else
			{
				xf86UnGetToken (token);
			}
			break;

		case BIOSBASE:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "BIOSBase");
			ptr->dev_bios_base = val.num;
			break;
		case MEMBASE:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "MemBase");
			ptr->dev_mem_base = val.num;
			break;
		case IOBASE:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "IOBase");
			ptr->dev_io_base = val.num;
			break;
		case DACBASE:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "DACBase");
			ptr->dev_dac_base = val.num;
			break;
		case COPBASE:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "COPBase");
			ptr->dev_cop_base = val.num;
			break;
		case POSBASE:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "POSBase");
			ptr->dev_pos_base = val.num;
			break;
		case INSTANCE:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "Instance");
			ptr->dev_instance = val.num;
			break;
		case PCI_TAG:
			if (xf86GetToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "PciTag");
			ptr->dev_pci_tag = val.num;
			break;
		case CLOCKCHIP:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "ClockChip");
			ptr->dev_clockchip = val.str;
			break;

		case CLOCKS:
			token = xf86GetToken(NULL);
			for( i = ptr->dev_clocks;
			     token == NUMBER && i < CONF_MAXCLOCKS; i++ ) {
				ptr->dev_clock[i] = (int)(val.realnum * 1000.0 + 0.5);
				token = xf86GetToken(NULL);
			}
			ptr->dev_clocks = i;
			xf86UnGetToken (token);
			break;
		case OPTION:
			{
				char *name;
				if ((token = xf86GetToken (NULL)) != STRING)
					Error (BAD_OPTION_MSG, NULL);
				name = val.str;
				if ((token = xf86GetToken (NULL)) == STRING)
				{
					ptr->dev_option_lst = addNewOption (ptr->dev_option_lst,
														name, val.str);
				}
				else
				{
					ptr->dev_option_lst = addNewOption (ptr->dev_option_lst,
														name, NULL);
					xf86UnGetToken (token);
				}
			}
			break;
		case EOF_TOKEN:
			Error (UNEXPECTED_EOF_MSG, NULL);
			break;
		default:
			Error (INVALID_KEYWORD_MSG, xf86TokenString ());
			break;
		}
	}

	if (!has_ident)
		Error (NO_IDENT_MSG, NULL);

#ifdef DEBUG
	printf ("Device section parsed\n");
#endif

	return ptr;
}

#undef CLEANUP

void
printDeviceSection (FILE * cf, XF86ConfDevicePtr ptr)
{
	XF86OptionPtr optr;
	int i;

	while (ptr)
	{
		fprintf (cf, "Section \"Device\"\n");
		if (ptr->dev_identifier)
			fprintf (cf, "\tIdentifier  \"%s\"\n", ptr->dev_identifier);
		if (ptr->dev_driver)
			fprintf (cf, "\tDriver  \"%s\"\n", ptr->dev_driver);
		if (ptr->dev_vendor)
			fprintf (cf, "\tVendorName      \"%s\"\n", ptr->dev_vendor);
		if (ptr->dev_board)
			fprintf (cf, "\tBoardName   \"%s\"\n", ptr->dev_board);
		if (ptr->dev_chipset)
			fprintf (cf, "\tChipSet     \"%s\"\n", ptr->dev_chipset);
		if (ptr->dev_card)
			fprintf (cf, "\tCard     \"%s\"\n", ptr->dev_card);
		if (ptr->dev_ramdac)
			fprintf (cf, "\tRamDac      \"%s\"\n", ptr->dev_ramdac);
		if (ptr->dev_speedup)
			fprintf (cf, "\tSpeedup      \"%s\"\n", ptr->dev_speedup);
		if (ptr->dev_videoram)
			fprintf (cf, "\tVideoRam    %d\n", ptr->dev_videoram);
		if (ptr->dev_clockprog)
		{
			fprintf (cf, "\tClockProg      \"%s\"", ptr->dev_clockprog);
			if (ptr->dev_textclockvalue)
				fprintf (cf, " %d\n", ptr->dev_textclockvalue);
			else
				fprintf (cf, "\n");
		}
		if (ptr->dev_bios_base)
			fprintf (cf, "\tBiosBase    %d\n", ptr->dev_bios_base);
		if (ptr->dev_mem_base)
			fprintf (cf, "\tMemBase    %d\n", ptr->dev_mem_base);
		if (ptr->dev_io_base)
			fprintf (cf, "\tIOBase    %d\n", ptr->dev_io_base);
		if (ptr->dev_dac_base)
			fprintf (cf, "\tDACBase    %d\n", ptr->dev_dac_base);
		if (ptr->dev_cop_base)
			fprintf (cf, "\tCOPBase    %d\n", ptr->dev_cop_base);
		if (ptr->dev_pos_base)
			fprintf (cf, "\tPOSBase    %d\n", ptr->dev_pos_base);
		if (ptr->dev_instance)
			fprintf (cf, "\tInstance    %d\n", ptr->dev_instance);
		if (ptr->dev_pci_tag != 0xffffffff)
			fprintf (cf, "\tPciTag  0x%08x\n", ptr->dev_pci_tag);
		if (ptr->dev_clockchip)
			fprintf (cf, "\tClockChip      \"%s\"", ptr->dev_clockchip);

		for (optr = ptr->dev_option_lst; optr; optr = optr->list.next)
		{
			fprintf (cf, "\tOption      \"%s\"", optr->opt_name);
			if (optr->opt_val)
				fprintf (cf, " \"%s\"", optr->opt_val);
			fprintf (cf, "\n");
		}
		if( ptr->dev_clocks > 0 ) {
			fprintf (cf, "\tClocks ");
			for (i = 0; i < ptr->dev_clocks; i++ )
				fprintf (cf, "%d ", ptr->dev_clock[i] / 1000 );
			fprintf (cf, "\n");
		}
		fprintf (cf, "EndSection\n\n");
		ptr = ptr->list.next;
	}
}

void
freeDeviceList (XF86ConfDevicePtr ptr)
{
	XF86ConfDevicePtr prev;

	while (ptr)
	{
		TestFree (ptr->dev_identifier);
		TestFree (ptr->dev_vendor);
		TestFree (ptr->dev_board);
		TestFree (ptr->dev_chipset);
		TestFree (ptr->dev_card);
		TestFree (ptr->dev_driver);
		TestFree (ptr->dev_ramdac);
		TestFree (ptr->dev_speedup);
		TestFree (ptr->dev_clockprog);
		TestFree (ptr->dev_clockchip);
		xf86OptionListFree (ptr->dev_option_lst);

		prev = ptr;
		ptr = ptr->list.next;
		xf86conffree (prev);
	}
}

int
validateDevice (XF86ConfigPtr p)
{
  XF86ConfDevicePtr device = p->conf_device_lst;

  if (!device) {
    xf86ValidationError ("At least one Device section is required.");
    return (FALSE);
  }

#if defined(XFree86LOADER)
  while (device) {
    if (!device->dev_driver) {
      xf86ValidationError (UNDEFINED_DRIVER_MSG, device->dev_identifier);
      return (FALSE);
    }
  device = device->list.next;
  }
#endif
  return (TRUE);
}

XF86ConfDevicePtr
xf86FindDevice (char *ident, XF86ConfDevicePtr p)
{
	while (p)
	{
		if (StrCaseCmp (ident, p->dev_identifier) == 0)
			return (p);

		p = p->list.next;
	}
	return (NULL);
}

char *
ConfigStrdup (char *s)
{
	char *tmp = (char *) xf86confmalloc (sizeof (char) * (strlen (s) + 1));
	strcpy (tmp, s);
	return (tmp);
}

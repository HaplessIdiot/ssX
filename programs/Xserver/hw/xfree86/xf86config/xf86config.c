/* $XFree86: xc/programs/Xserver/hw/xfree86/xf86config/xf86config.c,v 3.2 1994/12/11 11:16:03 dawes Exp $ */

/*
 * This is a dumb configuration program that will create a base
 * XF86Config file based on menu choices. Its main feature is that
 * clueless users may be less inclined to select crazy sync rates
 * way over monitor spec, by presenting a menu with standard monitor
 * types. Also some people don't read docs unless an executable that
 * they can run tells them to.
 *
 * It assumes a 24-line or bigger text console.
 *
 * Revision history:
 * 25Sep94 Initial version.
 * 27Sep94 Fix hsync range of monitor types to match with best possible mode.
 *         Remove 'const'.
 *         Tweak descriptions.
 * 28Sep94 Fixes from J"org Wunsch:
 *           Don't use gets().
 *           Add mouse device prompt.
 *           Fix lines overrun for 24-line console.
 *         Increase buffer size for probeonly output.
 * 29Sep94 Fix bad bug with old XF86Config preserving during probeonly run.
 *         Add note about vertical refresh in interlaced modes.
 *         Name gets() replacement getstring().
 *         Add warning about binary paths.
 *         Fixes from David Dawes:
 *           Don't use 'ln -sf'.
 *           Omit man path reference in comment.
 *           Generate only a generic 320x200 SVGA section for accel cards.
 *	     Only allow writing to /usr/X11R6/lib/X11 if root, and use
 *	       -xf86config for the -probeonly phase (root only).
 *         Fix bug that forces screen type to accel in some cases.
 * 30Sep94 Continue after clocks probe fails.
 *         Note about programmable clocks.
 *         Rename to 'xf86config'. Not to be confused with XF86Config
 *           or the -xf86config option.
 * 07Oct94 Correct hsync in standard mode timings comments, and include
 *           the proper +/-h/vsync flags.
 * 11Oct94 Skip 'numclocks:' and 'pixel clocks:' lines when probing for
 * 	     clocks.
 * 18Oct94 Add check for existence of /usr/X11R6.
 *	   Add note about ctrl-alt-backspace.
 * 06Nov94 Add comment above standard mode timings in XF86Config.
 * 24Dec94 Add low-resolution modes using doublescan.
 *
 * Possible enhancements:
 * - Add more standard mode timings (also applies to README.Config). Missing
 *   are 1024x768 @ 72 Hz, 1152x900 modes, and 1280x1024 @ ~70 Hz.
 *   I suspect there is a VESA standard for 1024x768 @ 72 Hz with 77 MHz dot
 *   clock, and 1024x768 @ 75 Hz with 78.7 MHz dot clock.
 * - Add option for creation of clear, minimal XF86Config.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>



/*
 * Configuration variables.
 */

#define MAX_CLOCKS_LINES 16

int config_mousetype;		/* Mouse. */
int config_emulate3buttons;
int config_chordmiddle;
char *config_pointerdevice;
int config_monitortype;		/* Monitor. */
char *config_hsyncrange;
char *config_vsyncrange;
char *config_monitoridentifier;
char *config_monitorvendorname;
char *config_monitormodelname;
int config_videomemory;
int config_screentype;		/* mono, vga16, svga, accel */
char *config_deviceidentifier;
char *config_devicevendorname;
char *config_deviceboardname;
int config_numberofclockslines;
char *config_clocksline[MAX_CLOCKS_LINES];
char *config_modesline8bpp;
char *config_modesline16bpp;
char *config_modesline32bpp;
int config_virtualx8bpp, config_virtualy8bpp;
int config_virtualx16bpp, config_virtualy16bpp;
int config_virtualx32bpp, config_virtualy32bpp;


void write_XF86Config();


/*
 * This is the initial intro text that appears when the program is started.
 */

static char *intro_text =
"\n"
"This program will create a basic XF86Config file, based on menu selections you\n"
"make.\n"
"\n"
"The XF86Config file usually resides in /usr/X11R6/lib/X11. A sample XF86Config\n"
"file is supplied with XFree86; this program does not use it. The program\n"
"will ask for a pathname when it is ready to write the file.\n"
"\n"
"You can either take the sample XF86Config as a base and edit it for your\n"
"configuration, or let this program produce a base XF86Config file for your\n"
"configuration and fine-tune it. Refer to /usr/X11R6/lib/X11/doc/README.Config\n"
"for a detailed overview of the configuration process.\n"
"\n"
"For accelerated servers (including accelerated drivers in the SVGA server),\n"
"there are many chipset and card-specific options and settings. This program\n"
"does not know about these. On some configurations some of these settings must\n"
"be specified. Refer to the server man pages and chipset-specific READMEs.\n"
"\n"
"Before continuing with this program, make sure you know the chipset and\n"
"amount of video memory on your video card. SuperProbe can help with this.\n"
"It is also helpful if you know what server you want to run.\n"
"\n"
;

static char *finalcomment_text =
"File has been written. Take a look at it before running 'startx'. Note that\n"
"the XF86Config file must be in one of the directories searched by the server\n"
"(e.g. /usr/X11R6/lib/X11) in order to be used. Within the server press\n"
"ctrl, alt and '+' simultaneously to cycle video resolutions. Pressing ctrl,\n"
"alt and backspace simultaneously immediately exits the server (use if\n"
"the monitor doesn't sync for a particular mode).\n"
"\n"
"For further configuration, refer to /usr/X11R6/lib/X11/doc/README.Config.\n"
"\n";


void keypress() {
	printf("Press enter to continue, or ctrl-c to abort.");
	getchar();
	printf("\n");
}

void emptylines() {
	int i;
	for (i = 0; i < 50; i++)
		printf("\n");
}

int answerisyes(s)
	char *s;
{
	if (s[0] == '\'')	/* For fools that type the ' literally. */
		return tolower(s[1]) == 'y';
	return tolower(s[0]) == 'y';
}

/*
 * This is a replacement for gets(). Limit is 80 chars.
 * The 386BSD descendants scream about using gets(), for good reason.
 */

void getstring(s)
	char *s;
{
	char *cp;
	fgets(s, 80, stdin);
	cp = strchr(s, '\n');
	if (cp)
		*cp=0;
}


/*
 * Mouse configuration.
 */

static char *mouseintro_text =
"First specify a mouse protocol type. Choose one from the following list:\n"
"\n";

static char *mousetype_name[8] = {
	"Microsoft compatible (2-button protocol)",
	"Mouse Systems (3-button protocol)",
	"Bus Mouse",
	"PS/2 Mouse",
	"Logitech Mouse (old type, Logitech protocol)",
	"Logitech MouseMan (Microsoft compatible)",
	"MM Series",	/* XXXX These descriptions should be improved. */
	"MM HitTablet"
};

static char *mousetype_identifier[8] = {
	"Microsoft",
	"MouseSystems",
	"Busmouse",
	"PS/2",
	"Logitech",
	"MouseMan",
	"MMSeries",
	"MMHitTab"
};

static char *mousedev_text =
"Now give the full device name that the mouse is connected to, for example\n"
"/dev/tty00. Just pressing enter will use the default, /dev/mouse.\n"
"\n";

static char *mousecomment_text =
"If you have a two-button mouse, it is most likely of type 1, and if you have\n"
"a three-button mouse, it can probably support both protocol 1 and 2. There are\n"
"two main varieties of the latter type: mice with a switch to select the\n"
"protocol, and mice that default to 1 and require a button to be held at\n"
"boot-time to select protocol 2. Some mice can be convinced to do 2 by sending\n"
"a special sequence to the serial port.\n"
"\n";

static char *twobuttonmousecomment_text =
"You have selected a two-button mouse protocol. It is recommended that you\n"
"enable Emulate3Buttons.\n";

static char *threebuttonmousecomment_text =
"You have selected a three-button mouse protocol. It is recommended that you\n"
"do not enable Emulate3Buttons, unless the third button doesn't work.\n";

static char *unknownbuttonsmousecomment_text =
"If your mouse has only two buttons, it is recommended that you enable\n"
"Emulate3Buttons.\n";

static char *microsoftmousecomment_text =
"You have selected a Microsoft protocol mouse. If your mouse was made by\n"
"Logitech, you might want to enable ChordMiddle which could cause the\n"
"third button to work.\n";

static char *logitechmousecomment_text =
"You have selected a Logitech protocol mouse. This is only valid for old\n"
"Logitech mice.\n";

static char *mousemancomment_text =
"You have selected a Logitech MouseMan type mouse. You might want to enable\n"
"ChordMiddle which could cause the third button to work.\n";

void mouse_configuration() {
	int i;
	char s[80];
	printf("%s", mouseintro_text);
	
	for (i = 0; i < 8; i++)
		printf("%2d.  %s\n", i + 1, mousetype_name[i]);

	printf("\n");

	printf("%s", mousecomment_text);
	
	printf("Enter a protocol number: ");
	getstring(s);
	config_mousetype = atoi(s) - 1;

	printf("\n");

	if (config_mousetype == 4) {
		/* Logitech. */
		printf("%s", logitechmousecomment_text);
		printf("\n");
		printf("Please answer the following question with either 'y' or 'n'.\n");
		printf("Are you sure it's really not a Microsoft compatible one? ");
		getstring(s);
		if (!answerisyes(s))
			config_mousetype = 0;
		printf("\n");
	}

	config_chordmiddle = 0;
	if (config_mousetype == 0 || config_mousetype == 5) {
		/* Microsoft or MouseMan. */
		if (config_mousetype == 0)
			printf("%s", microsoftmousecomment_text);
		else
			printf("%s", mousemancomment_text);
		printf("\n");
		printf("Please answer the following question with either 'y' or 'n'.\n");
		printf("Do you want to enable ChordMiddle? ");
		getstring(s);
		if (answerisyes(s))
			config_chordmiddle = 1;
		printf("\n");
	}

	switch (config_mousetype) {
	case 0 : /* Microsoft compatible */
		if (config_chordmiddle)
			printf("%s", threebuttonmousecomment_text);
		else
			printf("%s", twobuttonmousecomment_text);
		break;
	case 1 : /* Mouse Systems. */
		printf("%s", threebuttonmousecomment_text);
		break;
	default :
		printf("%s", unknownbuttonsmousecomment_text);
		break;
	}

	printf("\n");

	printf("Please answer the following question with either 'y' or 'n'.\n");
	printf("Do you want to enable Emulate3Buttons? ");
	getstring(s);
	if (answerisyes(s))
		config_emulate3buttons = 1;
	else
		config_emulate3buttons = 0;
	printf("\n");

	printf("%s", mousedev_text);
	printf("Mouse device: ");
	getstring(s);
	if (strlen(s) == 0)
		config_pointerdevice = "/dev/mouse";
	else {
		config_pointerdevice = malloc(strlen(s) + 1);
		strcpy(config_pointerdevice, s);
       }
       printf("\n");
}


/*
 * Monitor configuration.
 */

static char *monitorintro_text =
"Now we want to set the specifications of the monitor. The two critical\n"
"parameters are the vertical refresh rate, which is the rate at which the\n"
"the whole screen is refreshed, and most importantly the horizontal sync rate,\n"
"which is the rate at which scanlines are displayed.\n"
"\n"
"The valid range for horizontal sync and vertical sync should be documented\n"
"in the manual of your monitor. If in doubt, check the monitor database\n"
"/usr/X11R6/lib/X11/doc/Monitors to see if your monitor is there.\n"
"\n";

static char *hsyncintro_text =
"You must indicate the horizontal sync range of your monitor. You can either\n"
"select one of the predefined ranges below that correspond to industry-\n"
"standard monitor types, or give a specific range.\n"
"\n";

static char *customhsync_text =
"Please enter the horizontal sync range of your monitor, in the format used\n"
"in the table of monitor types above. You can either specify one or more\n"
"continuous ranges (e.g. 15-25, 30-50), or one or more fixed sync frequencies.\n"
"\n";

static char *vsyncintro_text =
"You must indicate the vertical sync range of your monitor. You can either\n"
"select one of the predefined ranges below that correspond to industry-\n"
"standard monitor types, or give a specific range. For interlaced modes,\n"
"the number that counts is the high one (e.g. 87 Hz rather than 43 Hz).\n"
"\n"
" 1  50-70\n"
" 2  50-90\n"
" 3  50-100\n"
" 4  40-150\n"
" 5  Enter your own vertical sync range\n";

static char *monitordescintro_text =
"You must now enter a few identification/description strings, namely an\n"
"identifier, a vendor name, and a model name. Just pressing enter will fill\n"
"in default names.\n"
"\n";

#define NU_MONITORTYPES 9

static char *monitortype_range[NU_MONITORTYPES] = {
	"31.5",
	"31.5 - 35.1",
	"31.5, 35.5",
	"31.5, 35.15, 35.5",
	"31.5 - 37.9",
	"31.5 - 48.5",
	"31.5 - 57.0",
	"31.5 - 64.3",
	"31.5 - 79.0"
};

static char *monitortype_name[NU_MONITORTYPES] = {
	"Standard VGA, 640x480 @ 60 Hz",
	"Super VGA, 800x600 @ 56 Hz",
	"8514 Compatible, 1024x768 @ 87 Hz interlaced (no 800x600)",
	"Super VGA, 1024x768 @ 87 Hz interlaced, 800x600 @ 56 Hz",
	"Extended Super VGA, 800x600 @ 60 Hz, 640x480 @ 72 Hz",
	"Non-Interlaced SVGA, 1024x768 @ 60 Hz, 800x600 @ 72 Hz",
	"High Frequency SVGA, 1024x768 @ 70 Hz",
	"Monitor that can do 1280x1024 @ 60 Hz",
	"Monitor that can do 1280x1024 @ 74 Hz"
};

void monitor_configuration() {
	int i;
	char s[80];
	printf("%s", monitorintro_text);

	keypress();
	emptylines();

	printf("%s", hsyncintro_text);

	printf("    hsync in kHz; monitor type with characteristic modes\n");
	for (i = 0; i < NU_MONITORTYPES; i++)
		printf("%2d  %s; %s\n", i + 1, monitortype_range[i],
			monitortype_name[i]);

	printf("%2d  Enter your own horizontal sync range\n",
		NU_MONITORTYPES + 1);
	printf("\n");

	printf("Enter your choice (1-%d): ", NU_MONITORTYPES + 1);
	getstring(s);
	config_monitortype = atoi(s) - 1;

	printf("\n");

	if (config_monitortype < NU_MONITORTYPES)
		config_hsyncrange = monitortype_range[config_monitortype];
	else {
		/* Custom hsync range option selected. */
		printf("%s", customhsync_text);
		printf("Horizontal sync range: ");
		getstring(s);
		config_hsyncrange = malloc(strlen(s) + 1);
		strcpy(config_hsyncrange, s);
		printf("\n");
	}

	printf("%s", vsyncintro_text);
	printf("\n");

	printf("Enter your choice: ");
	getstring(s);
	printf("\n");
	switch (atoi(s)) {
	case 1 :
		config_vsyncrange = "50-70";
		break;
	case 2 :
		config_vsyncrange = "50-90";
		break;
	case 3 :
		config_vsyncrange = "50-100";
		break;
	case 4 :
		config_vsyncrange = "40-150";
		break;
	case 5 :
		/* Custom vsync range option selected. */
		printf("Vertical sync range: ");
		getstring(s);
		config_vsyncrange = malloc(strlen(s) + 1);
		strcpy(config_vsyncrange, s);
		printf("\n");
		break;
	}
	printf("%s", monitordescintro_text);
	printf("The strings are free-form, spaces are allowed.\n");
	printf("Enter an identifier for your monitor definition: ");
	getstring(s);
	if (strlen(s) == 0)
		config_monitoridentifier = "My Monitor";
	else {
		config_monitoridentifier = malloc(strlen(s) + 1);
		strcpy(config_monitoridentifier, s);
	}
	printf("Enter the vendor name of your monitor: ");
	getstring(s);
	if (strlen(s) == 0)
		config_monitorvendorname = "Unknown";
	else {
		config_monitorvendorname = malloc(strlen(s) + 1);
		strcpy(config_monitorvendorname, s);
	}
	printf("Enter the model name of your monitor: ");
	getstring(s);
	if (strlen(s) == 0)
		config_monitormodelname = "Unknown";
	else {
		config_monitormodelname = malloc(strlen(s) + 1);
		strcpy(config_monitormodelname, s);
	}
}


/*
 * Screen/video card configuration.
 */

static char *screenintro_text =
"Now we must configure video card specific settings. The first thing is\n"
"which server to run. Refer to the manpages and other documentation. The\n"
"following servers are available (they may not all be installed on your system):\n"
"\n"
" 1  The XF86_Mono server. This a monochrome server that should work on any\n"
"    VGA-compatible card, in 640x480 (more on some SVGA chipsets).\n"
" 2  The XF86_VGA16 server. This is a 16-color VGA server that should work on\n"
"    any VGA-compatible card.\n"
" 3  The XF86_SVGA server. This is a 256 color SVGA server that supports a\n"
"    a number of SVGA chipsets. It is accelerated on some Cirrus and WD\n"
"    chipsets; it supports 16/32-bit color on certain Cirrus configurations.\n"
" 4  The accelerated servers. These include XF86_S3, XF86_Mach32, XF86_Mach8,\n"
"    XF86_8514, XF86_P9000, XF86_AGX, and XF86_W32.\n"
"\n"
"These four server types correspond to the four different \"Screen\" sections in\n"
"XF86Config (vga2, vga16, svga, accel).\n"
"\n";

static char *screenlink_text =
"The server to run is selected by changing the symbolic link 'X'. For example,\n"
"'rm /usr/X11R6/bin/X; ln -s /usr/X11R6/bin/XF86_SVGA /usr/X11R6/bin/X' selects\n"
"the SVGA server.\n"
"\n";

static char *deviceintro_text =
"Now you must give information about your video card. This will be used for\n"
"the \"Device\" section of your video card in XF86Config.\n"
"\n";

static char *videomemoryintro_text =
"You must indicate how much video memory you have. It is probably a good\n"
"idea to use the same approximate amount as that detected by the server you\n"
"intend to use.\n"
"\n"
"How much video memory do you have on your video card:\n"
"\n";

static char *screenaccelservers_text =
"Select an accel server:\n"
"\n";

static char *carddescintro_text =
"You must now enter a few identification/description strings, namely an\n"
"identifier, a vendor name, and a model name. Just pressing enter will fill\n"
"in default names.\n"
"\n";

static char *devicevendornamecomment_text =
"You can simply press enter here if you have a generic card, or want to\n"
"describe your card with one string.\n";

static char *devicecomment_text =
"More options and settings can be specified in the Device section. For most\n"
"configurations, a Clocks line is useful since it prevents the slow and nasty\n"
"sounding clock probing at server start-up. Probed clocks are displayed at\n"
"server startup, along with other server and hardware configuration info.\n"
"You can save this information in a file by running\n"
"'X -probeonly 2>output_file'. Be warned that clock probing is inherently\n"
"imprecise; some clocks may be slightly too high (varies per run).\n"
"\n"
"Especially for accelerated servers, Ramdac, Dacspeed and ClockChip settings\n"
"or special options may be required in the Device section.\n"
"\n";

static char *deviceclocksquestion_text =
"At this point I can run X -probeonly, and try to extract the clock information\n"
"from the output. It is recommended that you do this yourself and add a clocks\n"
"line (note that the list of clocks may be split over multiple Clocks lines) to\n"
"your Device section afterwards. Be aware that a clocks line is not\n"
"appropriate for drivers that have a fixed set of clocks and don't probe by\n"
"default (e.g. Cirrus). Also, for the P9000 server you must simply specify\n"
"clocks line that matches the modes you want to use.  For the S3 server with\n"
"a programmable clock chip you need a 'ClockChip' line and no Clocks line.\n"
"\n"
"You must be root to be able to run X -probeonly now.\n"
"\n";

static char *modesorderintro_text =
"For each depth, a list of modes (resolutions) is defined. The default\n"
"resolution that the server will start-up with will be the first listed\n"
"mode that can be supported by the monitor and card.\n"
"Currently it is set to:\n"
"\n";

static char *modesorder_text2 =
"Note that 16bpp and 32bpp are only supported on a few configurations.\n"
"Modes that cannot be supported due to monitor or clock constraints will\n"
"be automatically skipped by the server.\n"
"\n"
" 1  Change the modes for 8pp (256 colors)\n"
" 2  Change the modes for 16bpp (32K/64K colors)\n"
" 3  Change the modes for 32bpp (24-bit color)\n"
" 4  The modes are OK, continue.\n"
"\n";

static char *modeslist_text =
"Please type the digits corresponding to the modes that you want to select.\n"
"For example, 432 selects \"1024x768\" \"800x600\" \"640x480\", with a\n"
"default mode of 1024x768.\n"
"\n";

static char *accelserver_name[7] = {
	"XF86_S3", "XF86_Mach32", "XF86_Mach8", "XF86_8514", "XF86_P9000",
	"XF86_AGX", "XF86_W32"
};

static int videomemory[5] = {
	256, 512, 1024, 2048, 4096
};

#define NU_MODESTRINGS 8

static char *modestring[NU_MODESTRINGS] = {
	"\"640x400\"",
	"\"640x480\"",
	"\"800x600\"",
	"\"1024x768\"",
	"\"1280x1024\"",
	"\"320x200\"",
	"\"320x240\"",
	"\"400x300\""
};

void screen_configuration() {
	int i, c;
	char s[80];
	printf("%s", screenintro_text);

	printf("Which one of these four screen types do you intend to run by default (1-4)? ");
	getstring(s);
	config_screentype = atoi(s);
	printf("\n");

	printf("%s", screenlink_text);
	printf("Please answer the following question with either 'y' or 'n'.\n");
	printf("Do you want me to set the symbolic link? ");
	getstring(s);
	printf("\n");
	if (answerisyes(s)) {
		char *servername;
		if (config_screentype == 4) {
			/* Accel server. */
			printf("%s", screenaccelservers_text);
			for (i = 0; i < 7; i++)
				printf("%2d  %s\n", i + 1,
					accelserver_name[i]);
			printf("\n");
			printf("Which accel server: ");
			getstring(s);
			servername = accelserver_name[atoi(s) - 1];
			printf("\n");
		}
		else
			switch (config_screentype) {
			case 1 : servername = "XF86_Mono"; break;
			case 2 : servername = "XF86_VGA16"; break;
			case 3 : servername = "XF86_SVGA"; break;
			}
		system("rm -f /usr/X11R6/bin/X");
		sprintf(s, "ln -s /usr/X11R6/bin/%s /usr/X11R6/bin/X",
			servername);
		system(s);
	}

	emptylines();

	/*
	 * Configure the "Device" section for the video card.
	 */

	printf("%s", deviceintro_text);

	printf("%s", videomemoryintro_text);

	for (i = 0; i < 5; i++)
		printf("%2d  %dK\n", i + 1, videomemory[i]);
	printf(" 6  Other\n\n");

	printf("Enter your choice: ");
	getstring(s);
	printf("\n");
	
	c = atoi(s) - 1;
	if (c < 5)
		config_videomemory = videomemory[c];
	else {
		printf("Amount of video memory in Kbytes: ");
		getstring(s);
		config_videomemory = atoi(s);
		printf("\n");
	}

	printf("%s", carddescintro_text);
	printf("The strings are free-form, spaces are allowed.\n");
	printf("Enter an identifier for your video card definition: ");
	getstring(s);
	if (strlen(s) == 0)
		config_deviceidentifier = "My Video Card";
	else {
		config_deviceidentifier = malloc(strlen(s) + 1);
		strcpy(config_deviceidentifier, s);
	}
	printf("%s", devicevendornamecomment_text);
	
	printf("Enter the vendor name of your video card: ");
	getstring(s);
	if (strlen(s) == 0)
		config_devicevendorname = "Unknown";
	else {
		config_devicevendorname = malloc(strlen(s) + 1);
		strcpy(config_devicevendorname, s);
	}
	printf("Enter the model (board) name of your video card: ");
	getstring(s);
	if (strlen(s) == 0)
		config_deviceboardname = "Unknown";
	else {
		config_deviceboardname = malloc(strlen(s) + 1);
                strcpy(config_deviceboardname, s);
	}
	printf("\n");

	emptylines();

	printf("%s", devicecomment_text);

	/*
	 * Initialize screen mode variables for svga and accel
	 * to default values.
	 * XXXX Doesn't leave room for off-screen caching in 16/32bpp modes
	 *      for the accelerated servers.
	 */
	config_modesline8bpp =
	config_modesline16bpp =
	config_modesline32bpp = "\"640x480\"";
	config_virtualx8bpp = config_virtualx16bpp = config_virtualx32bpp =
	config_virtualy8bpp = config_virtualy16bpp = config_virtualy32bpp = 0;
	if (config_videomemory >= 2048) {
		if (config_screentype == 4) {
			/*
			 * Allow room for font/pixmap cache for accel
			 * servers.
			 * Also the mach32 is has a limited width.
			 */
			config_virtualx8bpp = 1280;
			config_virtualy8bpp = 1024;
		}
		else {
			config_virtualx8bpp = 1600;
			config_virtualy8bpp = 1200;
		}
		if (config_screentype == 4) {
			config_virtualx16bpp = 1024;
			config_virtualy16bpp = 768;
		}
		else {
			config_virtualx16bpp = 1152;
			config_virtualy16bpp = 900;
		}
		config_virtualx32bpp = 800;
		config_virtualy32bpp = 600;
		config_modesline8bpp = "\"640x480\" \"800x600\" \"1024x768\" \"1280x1024\"";
		config_modesline16bpp = "\"640x480\" \"800x600\" \"1024x768\"";
		config_modesline32bpp = "\"640x480\" \"800x600\"";
	}
	else
	if (config_videomemory >= 1024) {
		if (config_screentype == 4) {
			/*
			 * Allow room for font/pixmap cache for accel
			 * servers.
			 */
			config_virtualx8bpp = 1024;
			config_virtualy8bpp = 768;
		}
		else {
			config_virtualx8bpp = 1152;
			config_virtualy8bpp = 900;
		}
		config_virtualx16bpp = 800; /* Forget about cache space;  */
		config_virtualy16bpp = 600; /* it's small enough as it is. */
		config_virtualx32bpp = 640;
		config_virtualy32bpp = 400;
		config_modesline8bpp = "\"640x480\" \"800x600\" \"1024x768\"";
		config_modesline16bpp = "\"640x480\" \"800x600\"";
		config_modesline32bpp = "\"640x400\"";
	}
	else
	if (config_videomemory >= 512) {
		config_virtualx8bpp = 800;
		config_virtualy8bpp = 600;
		config_modesline8bpp = "\"640x480\" \"800x600\"";
		config_modesline16bpp = "\"640x400\"";
	}
	else
	if (config_videomemory >= 256) {
		config_modesline8bpp = "640x400";
		config_virtualx8bpp = 640;
		config_virtualy8bpp = 400;
	}
	else {
		printf("Fatal error: Invalid amount of video memory.\n");
		exit(-1);
	}

	/*
	 * Optionally run X -probeonly to figure out the clocks.
	 */

	config_numberofclockslines = 0;

	printf("%s", deviceclocksquestion_text);
	printf("Do you want me to run 'X -probeonly' now? ");
	getstring(s);
	printf("\n");
	if (answerisyes(s)) {
		/*
		 * Write temporary XF86Config and run X -probeonly.
		 * Only allow when root.
		 */
		FILE *f;
		char *buf;
		if (getuid() != 0) {
			printf("Sorry, you must be root to do this.\n\n");
			goto endofprobeonly;
		}
		printf("Running X -probeonly -pn -xf86config /usr/X11R6/lib/X11/XF86Config.tmp.\n");
		write_XF86Config("/usr/X11R6/lib/X11/XF86Config.tmp");
		if (system("X -probeonly -pn -xf86config /usr/X11R6/lib/X11/XF86Config.tmp 2>/tmp/dumbconfig.2")) {
			printf("X -probeonly call failed.\n");
			printf("No Clocks line inserted.\n");
			goto clocksprobefailed;
		}
		/* Look for 'clocks:' (case sensitive). */		
		if (system("grep clocks\\: /tmp/dumbconfig.2 >/tmp/dumbconfig.3")) {
			printf("grep failed.\n");
			printf("Cannot find clocks in server output.\n");
			goto clocksprobefailed;
		}
		f = fopen("/tmp/dumbconfig.3", "r");
		buf = malloc(8192);
		/* Parse lines. */
		while (fgets(buf, 8192, f) != NULL) {
			char *clks;
			clks = strstr(buf, "clocks: ") + 8;
			if (clks >= buf + 3 && strcmp(clks - 11, "num") == 0)
				/* Reject lines with 'numclocks:'. */
				continue;
			if (clks >= buf + 8 && strcpy(clks - 14, "pixel ") == 0)
				/* Reject lines with 'pixel clocks:'. */
				continue;
			clks[strlen(clks) - 1] = '\0';	/* Remove '\n'. */
			config_clocksline[config_numberofclockslines] =
				malloc(strlen(clks) + 1);
			strcpy(config_clocksline[config_numberofclockslines],
				clks);
			printf("Clocks %s\n", clks);
			config_numberofclockslines++;
		}
		fclose(f);
clocksprobefailed:
		unlink("/tmp/dumbconfig.3");
		unlink("/tmp/dumbconfig.2");
		unlink("/usr/X11R6/lib/X11/XF86Config.tmp");
		printf("\n");

endofprobeonly:
		keypress();
	}

	/*
	 * For the mono and vga16 server, no further configuration is
	 * required.
	 */
	if (config_screentype == 1 || config_screentype == 2)
		return;

	/*
	 * Configure the modes order.
	 */
	 for (;;) {
	 	char modes[128];

		emptylines();

		printf("%s", modesorderintro_text);
		printf("%s for 8bpp\n", config_modesline8bpp);
		printf("%s for 16bpp\n", config_modesline16bpp);
		printf("%s for 32bpp\n", config_modesline32bpp);
		printf("\n");
		printf("%s", modesorder_text2);

		printf("Enter your choice: ");
		getstring(s);
		printf("\n");

		c = atoi(s) - 1;
		if (c < 0 || c >= 3)
			break;

		printf("Select modes from the following list:\n\n");

		for (i = 0; i < NU_MODESTRINGS; i++)
			printf("%2d  %s\n", i + 1, modestring[i]);
		printf("\n");

		printf("%s", modeslist_text);

		printf("Which modes? ");
		getstring(s);
		printf("\n");

		modes[0] = '\0';
		for (i = 0; i < strlen(s); i++) {
			if (s[i] < '1' || s[i] > '0' + NU_MODESTRINGS) {
				printf("Invalid mode skipped.\n");
				continue;
			}
			if (i > 0)
				strcat(modes, " ");
			strcat(modes, modestring[s[i] - '1']);
		}
		switch (c) {
		case 0 :
			config_modesline8bpp = malloc(strlen(modes) + 1);
			strcpy(config_modesline8bpp, modes);
			break;
		case 1 :
			config_modesline16bpp = malloc(strlen(modes) + 1);
			strcpy(config_modesline16bpp, modes);
			break;
		case 2 :
			config_modesline32bpp = malloc(strlen(modes) + 1);
			strcpy(config_modesline32bpp, modes);
			break;
		}
	}
}


/*
 * Create the XF86Config file.
 */

static char *XF86Config_firstchunk_text =
"# File generated by xf86config.\n"
"\n"
"#\n"
"# Copyright (c) 1994 by The XFree86 Project, Inc.\n"
"#\n"
"# Permission is hereby granted, free of charge, to any person obtaining a\n"
"# copy of this software and associated documentation files (the \"Software\"),\n"
"# to deal in the Software without restriction, including without limitation\n"
"# the rights to use, copy, modify, merge, publish, distribute, sublicense,\n"
"# and/or sell copies of the Software, and to permit persons to whom the\n"
"# Software is furnished to do so, subject to the following conditions:\n"
"# \n"
"# The above copyright notice and this permission notice shall be included in\n"
"# all copies or substantial portions of the Software.\n"
"# \n"
"# THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL\n"
"# THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
"# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF\n"
"# OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
"# SOFTWARE.\n"
"# \n"
"# Except as contained in this notice, the name of the XFree86 Project shall\n"
"# not be used in advertising or otherwise to promote the sale, use or other\n"
"# dealings in this Software without prior written authorization from the\n"
"# XFree86 Project.\n"
"#\n"
"\n"
"# **********************************************************************\n"
"# Refer to the XF86Config(4/5) man page for details about the format of \n"
"# this file.\n"
"# **********************************************************************\n"
"\n"
"# **********************************************************************\n"
"# Files section.  This allows default font and rgb paths to be set\n"
"# **********************************************************************\n"
"\n"
"Section \"Files\"\n"
"\n"
"    RgbPath	\"/usr/X11R6/lib/X11/rgb\"\n"
"\n"
"# Multiple FontPath entries are allowed (which are concatenated together),\n"
"# as well as specifying multiple comma-separated entries in one FontPath\n"
"# command (or a combination of both methods)\n"
"\n"
"    FontPath	\"/usr/X11R6/lib/X11/fonts/misc/\"\n"
"        FontPath	\"/usr/X11R6/lib/X11/fonts/Type1/\"\n"
"        FontPath	\"/usr/X11R6/lib/X11/fonts/Speedo/\"\n"
"        FontPath	\"/usr/X11R6/lib/X11/fonts/75dpi/\"\n"
"    FontPath	\"/usr/X11R6/lib/X11/fonts/100dpi/\"\n"
"\n"
"EndSection\n"
"\n"
"# **********************************************************************\n"
"# Server flags section.\n"
"# **********************************************************************\n"
"\n"
"Section \"ServerFlags\"\n"
"\n"
"# Uncomment this to cause a core dump at the spot where a signal is \n"
"# received.  This may leave the console in an unusable state, but may\n"
"# provide a better stack trace in the core dump to aid in debugging\n"
"\n"
"#    NoTrapSignals\n"
"\n"
"# Uncomment this to disable the <Crtl><Alt><BS> server abort sequence\n"
"\n"
"#    DontZap\n"
"\n"
"EndSection\n"
"\n"
"# **********************************************************************\n"
"# Input devices\n"
"# **********************************************************************\n"
"\n"
"# **********************************************************************\n"
"# Keyboard section\n"
"# **********************************************************************\n"
"\n"
"Section \"Keyboard\"\n"
"\n"
"    Protocol	\"Standard\"\n"
"\n"
"# when using XQUEUE, comment out the above line, and uncomment the\n"
"# following line\n"
"\n"
"#    Protocol	\"Xqueue\"\n"
"\n"
"    AutoRepeat	500 5\n"
"# Let the server do the NumLock processing.  This should only be required\n"
"# when using pre-R6 clients\n"
"#    ServerNumLock\n"
"\n"
"# Specifiy which keyboard LEDs can be user-controlled (eg, with xset(1))\n"
"#    Xleds      1 2 3\n"
"\n"
"# To set the LeftAlt to Meta, RightAlt key to ModeShift, \n"
"# RightCtl key to Compose, and ScrollLock key to ModeLock:\n"
"\n"
"#    LeftAlt     Meta\n"
"#    RightAlt    ModeShift\n"
"#    RightCtl    Compose\n"
"#    ScrollLock  ModeLock\n"
"\n"
"EndSection\n"
"\n"
"\n";

static char *pointersection_text1 = 
"# **********************************************************************\n"
"# Pointer section\n"
"# **********************************************************************\n"
"\n"
"Section \"Pointer\"\n";

static char *pointersection_text2 =
"\n"
"# When using XQUEUE, comment out the above two lines, and uncomment\n"
"# the following line.\n"
"\n"
"#    Protocol	\"Xqueue\"\n"
"\n"
"# Baudrate and SampleRate are only for some Logitech mice\n"
"\n"
"#    BaudRate	9600\n"
"#    SampleRate	150\n"
"\n"
"# Emulate3Buttons is an option for 2-button Microsoft mice\n"
"\n";

static char *monitorsection_text1 =
"# **********************************************************************\n"
"# Monitor section\n"
"# **********************************************************************\n"
"\n"
"# Any number of monitor sections may be present\n"
"\n"
"Section \"Monitor\"\n"
"\n";

static char *monitorsection_text2 =
"# Bandwidth is in MHz unless units are specified\n"
"\n"
"#    Bandwidth	25.2\n"
"\n"
"# HorizSync is in kHz unless units are specified.\n"
"# HorizSync may be a comma separated list of discrete values, or a\n"
"# comma separated list of ranges of values.\n"
"# NOTE: THE VALUES HERE ARE EXAMPLES ONLY.  REFER TO YOUR MONITOR\'S\n"
"# USER MANUAL FOR THE CORRECT NUMBERS.\n"
"\n";

static char *monitorsection_text3 =
"#    HorizSync	30-64         # multisync\n"
"#    HorizSync	31.5, 35.2    # multiple fixed sync frequencies\n"
"#    HorizSync	15-25, 30-50  # multiple ranges of sync frequencies\n"
"\n"
"# VertRefresh is in Hz unless units are specified.\n"
"# VertRefresh may be a comma separated list of discrete values, or a\n"
"# comma separated list of ranges of values.\n"
"# NOTE: THE VALUES HERE ARE EXAMPLES ONLY.  REFER TO YOUR MONITOR\'S\n"
"# USER MANUAL FOR THE CORRECT NUMBERS.\n"
"\n";

static char *monitorsection_text4 =
"# Modes can be specified in two formats.  A compact one-line format, or\n"
"# a multi-line format.\n"
"\n"
"# These two are equivalent\n"
"\n"
"#    ModeLine \"1024x768i\" 45 1024 1048 1208 1264 768 776 784 817 Interlace\n"
"\n"
"#    Mode \"1024x768i\"\n"
"#        DotClock	45\n"
"#        HTimings	1024 1048 1208 1264\n"
"#        VTimings	768 776 784 817\n"
"#        Flags		\"Interlace\"\n"
"#    EndMode\n"
"\n";

static char *modelines_text =
"# This is a set standard mode timings. Modes that are out of monitor spec\n"
"# are automatically deleted by the server, so there's no immediate need to\n"
"# delete mode timings (unless particular mode timings don't work on your\n"
"# monitor). With these modes, the best standard mode that your monitor\n"
"# and video card can support for a given resolution is automatically\n"
"# used.\n"
"\n"
"# 640x400 @ 70 Hz, 31.5 kHz hsync\n"
"Modeline \"640x400\"     25.175 640  664  760  800   400  409  411  450\n"
"# 640x480 @ 60 Hz, 31.5 kHz hsync\n"
"Modeline \"640x480\"     25.175 640  664  760  800   480  491  493  525\n"
"# 800x600 @ 56 Hz, 35.15 kHz hsync\n"
"ModeLine \"800x600\"     36     800  824  896 1024   600  601  603  625\n"
"# 1024x768 @ 87 Hz interlaced, 35.5 kHz hsync\n"
"Modeline \"1024x768\"    44.9  1024 1048 1208 1264   768  776  784  817 Interlace\n"
"\n"
"# 640x480 @ 72 Hz, 36.5 kHz hsync\n"
"Modeline \"640x480\"     31.5   640  680  720  864   480  488  491  521\n"
"# 800x600 @ 60 Hz, 37.8 kHz hsync\n"
"Modeline \"800x600\"     40     800  840  968 1056   600  601  605  628 +hsync +vsync\n"
"\n"
"# 800x600 @ 72 Hz, 48.0 kHz hsync\n"
"Modeline \"800x600\"     50     800  856  976 1040   600  637  643  666 +hsync +vsync\n"
"# 1024x768 @ 60 Hz, 48.4 kHz hsync\n"
"Modeline \"1024x768\"    65    1024 1032 1176 1344   768  771  777  806 -hsync -vsync\n"
"\n"
"# 1024x768 @ 70 Hz, 56.5 kHz hsync\n"
"Modeline \"1024x768\"    75    1024 1048 1184 1328   768  771  777  806 -hsync -vsync\n"
"# 1280x1024 @ 87 Hz interlaced, 51 kHz hsync\n"
"Modeline \"1280x1024\"   80    1280 1296 1512 1568  1024 1025 1037 1165 Interlace\n"
"\n"
"# 1024x768 @ 76 Hz, 62.5 kHz hsync\n"
"Modeline \"1024x768\"    85    1024 1032 1152 1360   768  784  787  823\n"
"# 1280x1024 @ 61 Hz, 64.2 kHz hsync\n"
"Modeline \"1280x1024\"  110    1280 1328 1512 1712  1024 1025 1028 1054\n"
"\n"
"# 1280x1024 @ 74 Hz, 78.85 kHz hsync\n"
"Modeline \"1280x1024\"  135    1280 1312 1456 1712  1024 1027 1030 1064\n"
"\n"
"# Low-res Doublescan modes\n"
"# If your chipset does not support doublescan, you get a 'squashed'\n"
"# resolution like 320x400.\n"
"\n"
"# 320x200 @ 70 Hz, 31.5 kHz hsync, 8:5 aspect ratio\n"
"Modeline \"320x200\"     12.588 320  336  384  400   200  204  205  225 Doublescan\n"
"# 320x240 @ 60 Hz, 31.5 kHz hsync, 4:3 aspect ratio\n"
"Modeline \"320x240\"     12.588 320  336  384  400   240  245  246  262 Doublescan\n"
"# 320x240 @ 72 Hz, 36.5 kHz hsync\n"
"Modeline \"320x240\"     15.750 320  336  384  400   240  244  246  262 Doublescan\n"
"# 400x300 @ 56 Hz, 35.2 kHz hsync, 4:3 aspect ratio\n"
"ModeLine \"400x300\"     18     400  416  448  512   300  301  602  312 Doublescan\n"
"# 400x300 @ 60 Hz, 37.8 kHz hsync\n"
"Modeline \"400x300\"     20     400  416  480  528   300  301  303  314 Doublescan\n"
"# 400x300 @ 72 Hz, 48.0 kHz hsync\n"
"Modeline \"400x300\"     25     400  424  488  520   300  319  322  333 Doublescan\n"
"# 480x300 @ 56 Hz, 35.2 kHz hsync, 8:5 aspect ratio\n"
"ModeLine \"480x300\"     21.656 480  496  536  616   300  301  302  312 Doublescan\n"
"# 480x300 @ 60 Hz, 37.8 kHz hsync\n"
"Modeline \"480x300\"     23.890 480  496  576  632   300  301  303  314 Doublescan\n"
"# 480x300 @ 63 Hz, 39.6 kHz hsync\n"
"Modeline \"480x300\"     25     480  496  576  632   300  301  303  314 Doublescan\n"
"# 480x300 @ 72 Hz, 48.0 kHz hsync\n"
"Modeline \"480x300\"     29.952 480  504  584  624   300  319  322  333 Doublescan\n"
"\n";

static char *devicesection_text =
"# **********************************************************************\n"
"# Graphics device section\n"
"# **********************************************************************\n"
"\n"
"# Any number of graphics device sections may be present\n"
"\n"
"# Standard VGA Device:\n"
"\n"
"Section \"Device\"\n"
"    Identifier	\"Generic VGA\"\n"
"    VendorName	\"Unknown\"\n"
"    BoardName	\"Unknown\"\n"
"    Chipset	\"generic\"\n"
"\n"
"#    VideoRam	256\n"
"\n"
"#    Clocks	25.2 28.3\n"
"\n"
"EndSection\n"
"\n"
"# Sample Device for accelerated server:\n"
"\n"
"# Section \"Device\"\n"
"#    Identifier	\"Actix GE32+ 2MB\"\n"
"#    VendorName	\"Actix\"\n"
"#    BoardName	\"GE32+\"\n"
"#    Ramdac	\"ATT20C490\"\n"
"#    Dacspeed	110\n"
"#    Option	\"dac_8_bit\"\n"
"#    Clocks	 25.0  28.0  40.0   0.0  50.0  77.0  36.0  45.0\n"
"#    Clocks	130.0 120.0  80.0  31.0 110.0  65.0  75.0  94.0\n"
"# EndSection\n"
"\n"
"# Device configured by xf86config:\n"
"\n";

static char *screensection_text1 =
"# **********************************************************************\n"
"# Screen sections\n"
"# **********************************************************************\n"
"\n";


void write_XF86Config(filename)
	char *filename;
{
	FILE *f;

	/*
	 * Write the file.
	 */

	f = fopen(filename, "w");
	if (f == NULL) {
		printf("Failed to open filename for writing.\n");
		if (getuid() != 0)
			printf("Maybe you need to be root to write to the specified directory?\n");
		exit(-1);
	}

	fprintf(f, "%s", XF86Config_firstchunk_text);

	/*
	 * Write pointer section.
	 */
	fprintf(f, "%s", pointersection_text1);
	fprintf(f, "    Protocol    \"%s\"\n",
		mousetype_identifier[config_mousetype]);
	fprintf(f, "    Device      \"%s\"\n", config_pointerdevice);
	fprintf(f, "%s", pointersection_text2);
	if (!config_emulate3buttons)
		fprintf(f, "#");
	fprintf(f, "    Emulate3Buttons\n\n");
	fprintf(f, "# ChordMiddle is an option for some 3-button Logitech mice\n\n");
	if (!config_chordmiddle)
		fprintf(f, "#");
	fprintf(f, "    ChordMiddle\n\n");
	fprintf(f, "EndSection\n\n\n");

	/*
	 * Write monitor section.
	 */
	fprintf(f, "%s", monitorsection_text1);
	fprintf(f, "    Identifier  \"%s\"\n", config_monitoridentifier);
	fprintf(f, "    VendorName  \"%s\"\n", config_monitorvendorname);
	fprintf(f, "    ModelName   \"%s\"\n", config_monitormodelname);
	fprintf(f, "\n");
	fprintf(f, "%s", monitorsection_text2);
	fprintf(f, "    HorizSync   %s\n", config_hsyncrange);
	fprintf(f, "\n");
	fprintf(f, "%s", monitorsection_text3);
	fprintf(f, "    VertRefresh %s\n", config_vsyncrange);
	fprintf(f, "\n");
	fprintf(f, "%s", monitorsection_text4);
	fprintf(f, "%s", modelines_text);
	fprintf(f, "EndSection\n\n\n");

	/*
	 * Write Device section.
	 */

	fprintf(f, "%s", devicesection_text);
	fprintf(f, "Section \"Device\"\n");
	fprintf(f, "    Identifier  \"%s\"\n", config_deviceidentifier);
	fprintf(f, "    VendorName  \"%s\"\n", config_devicevendorname);
	fprintf(f, "    BoardName   \"%s\"\n", config_deviceboardname);
	fprintf(f, "    VideoRam    %d\n", config_videomemory);
	if (config_numberofclockslines == 0)
		fprintf(f, "    # Insert Clocks lines here\n");
	else {
		int i;
		for (i = 0; i < config_numberofclockslines; i++)
			fprintf(f, "    Clocks %s\n", config_clocksline[i]);
	}
	fprintf(f, "EndSection\n\n\n");	

	/*
	 * Write Screen sections.
	 */

	fprintf(f, "%s", screensection_text1);

	/*
	 * SVGA screen section.
	 */
	if (config_screentype == 3)
		fprintf(f, 
			"# The Colour SVGA server\n"
			"\n"
			"Section \"Screen\"\n"
			"    Driver      \"svga\"\n"
			"    # Use Device \"Generic VGA\" for Standard VGA 320x200x256\n"
			"    #Device      \"Generic VGA\"\n"
			"    Device      \"%s\"\n"
			"    Monitor     \"%s\"\n"
			"    Subsection \"Display\"\n"
			"        Depth       8\n"
			"        # Omit the Modes line for the \"Generic VGA\" device\n"
			"        Modes       %s\n"
			"        ViewPort    0 0\n"
			"        # Use Virtual 320 200 for Generic VGA\n"
			"        Virtual     %d %d\n"
			"    EndSubsection\n"
			"    Subsection \"Display\"\n"
			"        Depth       16\n"
			"        Modes       %s\n"
			"        ViewPort    0 0\n"
			"        Virtual     %d %d\n"
			"    EndSubsection\n"
			"    Subsection \"Display\"\n"
			"        Depth       32\n"
			"        Modes       %s\n"
			"        ViewPort    0 0\n"
			"        Virtual     %d %d\n"
			"    EndSubsection\n"
			"EndSection\n"
			"\n",
			config_deviceidentifier,
			config_monitoridentifier,
			config_modesline8bpp,
			config_virtualx8bpp, config_virtualy8bpp,
			config_modesline16bpp,
			config_virtualx16bpp, config_virtualy16bpp,
			config_modesline32bpp,
 			config_virtualx32bpp, config_virtualy32bpp
		);
	else
		/*
		 * If the default server is not the SVGA server, generate
		 * an SVGA server screen for just generic 320x200.
		 */
		fprintf(f, 
			"# The Colour SVGA server\n"
			"\n"
			"Section \"Screen\"\n"
			"    Driver      \"svga\"\n"
			"    Device      \"Generic VGA\"\n"
			"    #Device      \"%s\"\n"
			"    Monitor     \"%s\"\n"
			"    Subsection \"Display\"\n"
			"        Depth       8\n"
			"        #Modes       %s\n"
			"        ViewPort    0 0\n"
			"        Virtual     320 200\n"
			"        #Virtual     %d %d\n"
			"    EndSubsection\n"
			"EndSection\n"
			"\n",
			config_deviceidentifier,
			config_monitoridentifier,
			config_modesline8bpp,
			config_virtualx8bpp, config_virtualy8bpp
		);

	/*
	 * VGA16 screen section.
	 */
	fprintf(f, 
		"# The 16-color VGA server\n"
		"\n"
		"Section \"Screen\"\n"
		"    Driver      \"vga16\"\n"
		"    Device      \"%s\"\n"
		"    Monitor     \"%s\"\n"
		"    Subsection \"Display\"\n"
		/*
		 * Depend on 800x600 to be deleted if not available due to
		 * dot clock or monitor constraints.
		 */
		"        Modes       \"640x480\" \"800x600\"\n"
		"        ViewPort    0 0\n"
		"        Virtual     800 600\n"
		"    EndSubsection\n"
		"EndSection\n"
		"\n",
		/*
		 * If mono or vga16 is configured as the default server,
		 * use the configured video card device instead of the
		 * generic VGA device.
		 */
		(config_screentype == 1 || config_screentype == 2) ?
			config_deviceidentifier :
			"Generic VGA",
		config_monitoridentifier
	);

	/*
	 * VGA2 screen section.
	 * This is almost identical to the VGA16 section.
	 */
	fprintf(f, 
		"# The Mono server\n"
		"\n"
		"Section \"Screen\"\n"
		"    Driver      \"vga2\"\n"
		"    Device      \"%s\"\n"
		"    Monitor     \"%s\"\n"
		"    Subsection \"Display\"\n"
		/*
		 * Depend on 800x600 to be deleted if not available due to
		 * dot clock or monitor constraints.
		 */
		"        Modes       \"640x480\" \"800x600\"\n"
		"        ViewPort    0 0\n"
		"        Virtual     800 600\n"
		"    EndSubsection\n"
		"EndSection\n"
		"\n",
		/*
		 * If mono or vga16 is configured as the default server,
		 * use the configured video card device instead of the
		 * generic VGA device.
		 */
		(config_screentype == 1 || config_screentype == 2) ?
			config_deviceidentifier :
			"Generic VGA",
		config_monitoridentifier
	);

	/*
	 * The Accel section.
	 */
	fprintf(f, 
		"# The accelerated servers (S3, Mach32, Mach8, 8514, P9000, AGX, W32)\n"
		"\n"
		"Section \"Screen\"\n"
		"    Driver      \"accel\"\n"
		"    Device      \"%s\"\n"
		"    Monitor     \"%s\"\n"
		"    Subsection \"Display\"\n"
		"        Depth       8\n"
		"        Modes       %s\n"
		"        ViewPort    0 0\n"
		"        Virtual     %d %d\n"
		"    EndSubsection\n"
		"    Subsection \"Display\"\n"
		"        Depth       16\n"
		"        Modes       %s\n"
		"        ViewPort    0 0\n"
		"        Virtual     %d %d\n"
		"    EndSubsection\n"
		"    Subsection \"Display\"\n"
		"        Depth       32\n"
		"        Modes       %s\n"
		"        ViewPort    0 0\n"
		"        Virtual     %d %d\n"
		"    EndSubsection\n"
		"EndSection\n"
		"\n",
		config_deviceidentifier,
		config_monitoridentifier,
		config_modesline8bpp,
		config_virtualx8bpp, config_virtualy8bpp,
		config_modesline16bpp,
		config_virtualx16bpp, config_virtualy16bpp,
		config_modesline32bpp,
		config_virtualx32bpp, config_virtualy32bpp
	);

	fclose(f);
}


/*
 * Ask where to write XF86Config to. Returns filename.
 */

char *ask_XF86Config_location() {
	char s[80];
	char *filename;

	printf(
"I am going to write the XF86Config file now. Make sure you don't accidently\n"
"overwrite a previously configured one.\n\n");

	if (getuid() == 0) {
		printf("Please answer the following question with either 'y' or 'n'.\n");
		printf("Shall I write it to the default location, /usr/X11R6/lib/X11/XF86Config? ");
		getstring(s);
		printf("\n");
		if (answerisyes(s))
			return "/usr/X11R6/lib/X11/XF86Config";
	}

	printf("Do you want it written to the current directory as 'XF86Config'? ");
	getstring(s);
	printf("\n");
	if (answerisyes(s))
		return "XF86Config";

	printf("Please give a path+filename to write to: ");
	getstring(s);
	printf("\n");
	filename = malloc(strlen(s) + 1);
	strcpy(filename, s);
	return filename;
}


/*
 * Check if an earlier version of XFree86 is installed; warn about proper
 * search path order in that case.
 */

static char *oldxfree86_text =
"The directory '/usr/X386/bin' exists. You probably have an old version of\n"
"XFree86 installed (XFree86 3.1 installs in '/usr/X11R6' instead of\n"
"'/usr/X386').\n"
"\n"
"It is imperative that the directory '/usr/X11R6/bin' is present in your\n"
"search path, *before* any occurrence of '/usr/X386/bin'. If you have installed\n"
"X program binaries that are not in the base XFree86 distribution in\n"
"'/usr/X386/bin', you can keep the directory in your path as long as it is\n"
"after '/usr/X11R6/bin'.\n"
"\n";

static char *notinstalled_text =
"The directory /usr/X11R6 does not exist. This probably means that you have\n"
"not yet installed an X11R6-based version of XFree86. Please install\n"
"XFree86 3.1+ before running this program, following the instructions in\n"
"the INSTALL or README that comes with the XFree86 distribution for your OS.\n"
"For a minimal installation it is sufficient to only install base binaries,\n"
"libraries, configuration files and a server that you want to use.\n"
"\n";

static char *pathnote_text =	
"Note that the X binary directory in your path may be a symbolic link.\n"
"In that case you could modify the symbolic link to point to the new binaries.\n"
"Example: 'rm -f /usr/bin/X11; ln -s /usr/X11R6/bin /usr/bin/X11', if the\n"
"link is '/usr/bin/X11'.\n"
"\n"
"Make sure the path is OK before continuing.\n";

void path_check() {
	char s[80];
	FILE *f;

	f = fopen("/usr/X11R6", "r");
	if (f == NULL) {
		printf("%s", notinstalled_text);
		printf("Do you want to continue? ");
		getstring(s);
		if (!answerisyes(s))
			exit(-1);
		printf("\n");
	}
	fclose(f);

	f = fopen("/usr/X386/bin", "r");
	if (f == NULL)
		return;

	fclose(f);
	printf("%s", oldxfree86_text);
	printf("Your PATH is currently set as follows:\n%s\n\n",
		getenv("PATH"));
	printf("%s", pathnote_text);
	keypress();
}


/*
 * Program entry point.
 */

void main() {
	emptylines();

	printf("%s", intro_text);

	keypress();
	emptylines();

	path_check();

	emptylines();

	mouse_configuration();

	emptylines();

	monitor_configuration();

	emptylines();

 	screen_configuration();

	emptylines();

	write_XF86Config(ask_XF86Config_location());

	printf("%s", finalcomment_text);

	exit(0);
}

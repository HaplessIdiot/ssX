XCOMM $XFree86: xc/programs/Xserver/hw/xfree86/XF86Conf.cpp,v 3.35 1999/01/24 03:13:49 dawes Exp $
XCOMM
XCOMM Copyright (c) 1994-1998 by The XFree86 Project, Inc.
XCOMM
XCOMM Permission is hereby granted, free of charge, to any person obtaining a
XCOMM copy of this software and associated documentation files (the "Software"),
XCOMM to deal in the Software without restriction, including without limitation
XCOMM the rights to use, copy, modify, merge, publish, distribute, sublicense,
XCOMM and/or sell copies of the Software, and to permit persons to whom the
XCOMM Software is furnished to do so, subject to the following conditions:
XCOMM 
XCOMM The above copyright notice and this permission notice shall be included in
XCOMM all copies or substantial portions of the Software.
XCOMM 
XCOMM THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
XCOMM IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
XCOMM FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
XCOMM THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
XCOMM WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
XCOMM OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
XCOMM SOFTWARE.
XCOMM 
XCOMM Except as contained in this notice, the name of the XFree86 Project shall
XCOMM not be used in advertising or otherwise to promote the sale, use or other
XCOMM dealings in this Software without prior written authorization from the
XCOMM XFree86 Project.
XCOMM
XCOMM $XConsortium: XF86Conf.cpp /main/22 1996/10/23 11:43:51 kaleb $

XCOMM **********************************************************************
XCOMM This is a sample configuration file only, intended to illustrate
XCOMM what a config file might look like.  Refer to the XF86Config(4/5)
XCOMM man page for details about the format of this file. This man page
XCOMM is installed as MANPAGE 
XCOMM **********************************************************************

XCOMM The ordering of sections is not important in version 4.0 and later.

XCOMM **********************************************************************
XCOMM Files section.  This allows default font and rgb paths to be set
XCOMM **********************************************************************

Section "Files"

XCOMM The location of the RGB database.  Note, this is the name of the
XCOMM file minus the extension (like ".txt" or ".db").  There is normally
XCOMM no need to change the default.

    RgbPath	RGBPATH

XCOMM Multiple FontPath entries are allowed (which are concatenated together),
XCOMM as well as specifying multiple comma-separated entries in one FontPath
XCOMM command (or a combination of both methods)

    FontPath	LOCALFONTPATH
    FontPath	MISCFONTPATH
    FontPath	DPI75USFONTPATH
    FontPath	DPI100USFONTPATH
    FontPath	T1FONTPATH
    FontPath	SPFONTPATH
    FontPath	DPI75FONTPATH
    FontPath	DPI100FONTPATH

XCOMM ModulePath can be used to set a search path for the X server modules.
XCOMM The default path is shown here.

XCOMM    ModulePath	MODULEPATH

EndSection

XCOMM **********************************************************************
XCOMM Module section -- this is an optional section which is used to specify
XCOMM which run-time loadable modules to load when the X server starts up.
XCOMM **********************************************************************

Section "Module"

XCOMM This loads the DBE extension module.

    Load	"dbe"

XCOMM This loads the miscellaneous extensions module, and disables
XCOMM initialisation of the XFree86-DGA extension within that module.

    SubSection	"extmod"
	Option	"omit xfree86-dga"
    EndSubSection

XCOMM This loads the Type1 and FreeType font modules

    Load	"type1"
    Load	"freetype"

XCOMM Some examples of XInput devices

XCOMM    SubSection "magellan"
XCOMM	Option "Device" "/dev/cua0"
XCOMM  	Option "DeviceName" "spaceball"
XCOMM    EndSubSection
XCOMM
XCOMM    SubSection "spaceorb"
XCOMM	Option "Device" "/dev/cua0"
XCOMM  	Option "DeviceName" "spaceball"
XCOMM    EndSubSection
XCOMM
XCOMM    SubSection "microtouch"
XCOMM  	Option "Identifier" "touchscreen0"
XCOMM  	Option "DeviceName" "touchscreen0"
XCOMM  	Option "Device" "/dev/ttyS0"
XCOMM  	Option "MinX" "1412"
XCOMM  	Option "MaxX" "15184"
XCOMM  	Option "MinY" "15372"
XCOMM  	Option "MaxY" "1230"
XCOMM  	Option "ScreenNumber" "0"
XCOMM  	Option "ReportingMode" "Scaled"
XCOMM  	Option "ButtonNumber" "1"
XCOMM  	Option "SendCoreEvents" "True"
XCOMM    EndSubSection
XCOMM
XCOMM    SubSection      "elo2300"
XCOMM  	Option  "Identifier" "touchscreen0"
XCOMM  	Option  "DeviceName" "touchscreen0"
XCOMM  	Option  "Device" "/dev/ttyS0"
XCOMM  	Option  "MinX" "231"
XCOMM  	Option  "MaxX" "3868"
XCOMM  	Option  "MinY" "3858"
XCOMM  	Option  "MaxY" "272"
XCOMM  	Option  "ScreenNumber" "0"
XCOMM  	Option  "ReportingMode" "Scaled"
XCOMM  	Option  "ButtonThreshold" "17"
XCOMM  	Option  "ButtonNumber" "1"
XCOMM  	Option  "SendCoreEvents" "True"
XCOMM    EndSubSection

EndSection


XCOMM **********************************************************************
XCOMM Server flags section.  This contains various server-wide Options.
XCOMM **********************************************************************

Section "ServerFlags"

XCOMM Uncomment this to cause a core dump at the spot where a signal is 
XCOMM received.  This may leave the console in an unusable state, but may
XCOMM provide a better stack trace in the core dump to aid in debugging

XCOMM    Option	"NoTrapSignals"

XCOMM Uncomment this to disable the <Crtl><Alt><BS> server abort sequence
XCOMM This allows clients to receive this key event.

XCOMM    Option	"DontZap"

XCOMM Uncomment this to disable the <Crtl><Alt><KP_+>/<KP_-> mode switching
XCOMM sequences.  This allows clients to receive these key events.

XCOMM    Option	"DontZoom"

XCOMM Uncomment this to disable tuning with the xvidtune client. With
XCOMM it the client can still run and fetch card and monitor attributes,
XCOMM but it will not be allowed to change them. If it tries it will
XCOMM receive a protocol error.

XCOMM    Option	"DisableVidModeExtension"

XCOMM Uncomment this to enable the use of a non-local xvidtune client.

XCOMM    Option	"AllowNonLocalXvidtune"

XCOMM Uncomment this to disable dynamically modifying the input device
XCOMM (mouse and keyboard) settings.

XCOMM    Option	"DisableModInDev"

XCOMM Uncomment this to enable the use of a non-local client to
XCOMM change the keyboard or mouse settings (currently only xset).

XCOMM    Option	"AllowNonLocalModInDev"

XCOMM Set the basic blanking screen saver timeout.

    Option	"blank time"	"10"	# 10 minutes

XCOMM Set the DPMS timeouts.  These are set here because they are global
XCOMM rather than screen-specific.  These settings alone don't enable DPMS.
XCOMM It is enabled per-screen (or per-monitor), and even then only when
XCOMM the driver supports it.

    Option	"standby time"	"20"
    Option	"suspend time"	"30"
    Option	"off time"	"60"

EndSection

XCOMM **********************************************************************
XCOMM Input devices
XCOMM **********************************************************************

XCOMM **********************************************************************
XCOMM Keyboard section
XCOMM **********************************************************************

Section "Keyboard"

XCOMM For most OSs, this is the correct Keyboard Protocol setting

    Protocol	"Standard"

XCOMM When using XQUEUE (only for SVR3 and SVR4, but not Solaris), comment
XCOMM out the above line, and uncomment the following line.

XCOMM    Protocol	"Xqueue"

XCOMM Set the keyboard auto repeat parameters.  Not all platforms implement
XCOMM this.

    AutoRepeat	500 5

XCOMM Specifiy which keyboard LEDs can be user-controlled (eg, with xset(1)).

XCOMM    Xleds      1 2 3

XCOMM To disable the XKEYBOARD extension, uncomment XkbDisable.

XCOMM XkbDisable

XCOMM To customise the XKB settings to suit your keyboard, modify the
XCOMM lines below (which are the defaults).  For example, for a European
XCOMM keyboard, you will probably want to use one of:
XCOMM
XCOMM    XkbModel	"pc102"
XCOMM    XkbModel	"pc105"
XCOMM
XCOMM If you have a Microsoft Natural keyboard, you can use:
XCOMM
XCOMM    XkbModel	"microsoft"
XCOMM
XCOMM If you have a US "windows" keyboard you will want:
XCOMM
XCOMM    XkbModel	"pc104"
XCOMM
XCOMM Then to change the language, change the Layout setting.
XCOMM For example, a german layout can be obtained with:
XCOMM
XCOMM    XkbLayout	"de"
XCOMM
XCOMM or:
XCOMM
XCOMM    XkbLayout	"de"
XCOMM    XkbVariant	"nodeadkeys"
XCOMM
XCOMM If you'd like to switch the positions of your capslock and
XCOMM control keys, use:
XCOMM
XCOMM    XkbOptions	"ctrl:swapcaps"


XCOMM These are the default XKB settings for XFree86
XCOMM
XCOMM    XkbRules	"xfree86"
XCOMM    XkbModel	"pc101"
XCOMM    XkbLayout	"us"
XCOMM    XkbVariant	""
XCOMM    XkbOptions	""

EndSection


XCOMM **********************************************************************
XCOMM Pointer section
XCOMM **********************************************************************

Section "Pointer"

XCOMM The mouse protocol and device.  The device is normally set to /dev/mouse,
XCOMM which is usually a symbolic link to the real device.

    Protocol	"Microsoft"
    Device	"/dev/mouse"

XCOMM On platforms where PnP mouse detection is supported the following
XCOMM protocol setting can be used when using a newer PnP mouse:

XCOMM    Protocol	"Auto"

XCOMM When using mouse connected to a PS/2 port (aka "MousePort), set the
XCOMM the protocol as follows.  On some platforms some other settings may
XCOMM be available.

XCOMM    Protocol	"PS/2"

XCOMM When using XQUEUE (only for SVR3 and SVR4, but not Solaris), use
XCOMM the following instead of any of the lines above.  The Device line
XCOMM is not required in this case.

XCOMM    Protocol	"Xqueue"

XCOMM Baudrate and SampleRate are only for some older Logitech mice.  In
XCOMM almost every case these lines should be omitted.

XCOMM    BaudRate	9600
XCOMM    SampleRate	150

XCOMM Emulate3Buttons is an option for 2-button mice
XCOMM Emulate3Timeout is the timeout in milliseconds (default is 50ms)

XCOMM    Emulate3Buttons
XCOMM    Emulate3Timeout	50

XCOMM ChordMiddle is an option for some 3-button Logitech mice, or any
XCOMM 3-button mouse where the middle button generates left+right button
XCOMM events.

XCOMM    ChordMiddle

EndSection


XCOMM **********************************************************************
XCOMM Monitor section
XCOMM **********************************************************************

XCOMM Any number of monitor sections may be present

Section "Monitor"

XCOMM The identifier line must be present.

    Identifier	"Generic Monitor"

XCOMM The VendorName and ModelName lines are optional.
    VendorName	"Unknown"
    ModelName	"Unknown"

XCOMM HorizSync is in kHz unless units are specified.
XCOMM HorizSync may be a comma separated list of discrete values, or a
XCOMM comma separated list of ranges of values.
XCOMM NOTE: THE VALUES HERE ARE EXAMPLES ONLY.  REFER TO YOUR MONITOR'S
XCOMM USER MANUAL FOR THE CORRECT NUMBERS.

XCOMM    HorizSync	31.5  # typical for a single frequency fixed-sync monitor
XCOMM    HorizSync	30-64         # multisync
XCOMM    HorizSync	31.5, 35.2    # multiple fixed sync frequencies
XCOMM    HorizSync	15-25, 30-50  # multiple ranges of sync frequencies

XCOMM VertRefresh is in Hz unless units are specified.
XCOMM VertRefresh may be a comma separated list of discrete values, or a
XCOMM comma separated list of ranges of values.
XCOMM NOTE: THE VALUES HERE ARE EXAMPLES ONLY.  REFER TO YOUR MONITOR'S
XCOMM USER MANUAL FOR THE CORRECT NUMBERS.

XCOMM    VertRefresh	60  # typical for a single frequency fixed-sync monitor

XCOMM    VertRefresh	50-100        # multisync
XCOMM    VertRefresh	60, 65        # multiple fixed sync frequencies
XCOMM    VertRefresh	40-50, 80-100 # multiple ranges of sync frequencies

XCOMM Modes can be specified in two formats.  A compact one-line format, or
XCOMM a multi-line format.

XCOMM A generic VGA 640x480 mode (hsync = 31.5kHz, refresh = 60Hz)
XCOMM These two are equivalent

XCOMM    ModeLine "640x480" 25.175 640 664 760 800 480 491 493 525

    Mode "640x480"
        DotClock	25.175
        HTimings	640 664 760 800
        VTimings	480 491 493 525
    EndMode

XCOMM These two are equivalent

XCOMM    ModeLine "1024x768i" 45 1024 1048 1208 1264 768 776 784 817 Interlace

XCOMM    Mode "1024x768i"
XCOMM        DotClock	45
XCOMM        HTimings	1024 1048 1208 1264
XCOMM        VTimings	768 776 784 817
XCOMM        Flags		"Interlace"
XCOMM    EndMode

XCOMM If a monitor has DPMS support, that can be indicated here.  This will
XCOMM enable DPMS when the montor is used with drivers that support it.

XCOMM    Option	"dpms"

XCOMM If a monitor requires that the sync signals be superimposed on the
XCOMM green signal, the following option will enable this when used with
XCOMM drivers that support it.  Only a relatively small range of hardware
XCOMM (and drivers) actually support this.

XCOMM    Option	"sync on green"

EndSection

XCOMM **********************************************************************
XCOMM Graphics device section
XCOMM **********************************************************************

XCOMM Any number of graphics device sections may be present

Section "Device"

XCOMM The Identifier must be present.

    Identifier	"Generic VGA"

XCOMM The VendorName and BoardName lines are optional.

    VendorName	"Unknown"
    BoardName	"Unknown"

XCOMM The Driver line must be present.  When using run-time loadable driver
XCOMM modules, this line instructs the server to load the specified driver
XCOMM module.  Even when not using loadable driver modules, this line
XCOMM indicates which driver should interpret the information in this section.

    Driver	"vga"

XCOMM The chipset line is optional in most cases.  It can be used to override
XCOMM the driver's chipset detection, and should not normally be specified.

XCOMM    Chipset	"generic"

XCOMM Various other lines can be specified to override the driver's automatic
XCOMM detection code.  In most cases they are not needed.

XCOMM    VideoRam	256
XCOMM    Clocks	25.2 28.3

XCOMM The BusID line is used to specify which of possibly multiple devices
XCOMM this section is intended for.  When this line isn't present, a device
XCOMM section can only match up with the primary video device.  For PCI
XCOMM devices a line like the following could be used.  This line should not
XCOMM normally be included unless there is more than one video device
XCOMM intalled.

XCOMM    BusID	"PCI:0:10:0"

XCOMM Various option lines can be added here as required.  Some options
XCOMM are more appropriate in Screen sections, Display subsections or even
XCOMM Monitor sections.

XCOMM    Option	"hw cursor" "off"

EndSection

Section "Device"
    Identifier	"any supported Trident chip"
    Driver	"trident"
    VendorName	"Trident"
EndSection

Section "Device"
    Identifier	"MGA Millennium I"
    Driver	"mga"
    Option	"hw cursor" "off"
    BusID	"PCI:0:10:0"
EndSection

Section "Device"
    Identifier	"MGA G200 AGP"
    Driver	"mga"
    BusID	"PCI:1:0:0"
    Option	"pci retry"
EndSection


XCOMM **********************************************************************
XCOMM Screen sections.
XCOMM **********************************************************************

XCOMM Any number of screen sections may be present.  Each describes
XCOMM the configuration of a single screen.  A single specific screen section
XCOMM may be specified from the X server command line with the "-screen"
XCOMM option.

Section "Screen"

XCOMM The Identifier, Device and Monitor lines must be present

    Identifier	"Screen 1"
    Device	"Generic VGA"
    Monitor	"Generic Monitor"

XCOMM The favoured Depth and/or Bpp may be specified here

    DefaultDepth 8

    SubSection "Display"
        Depth		8
        Modes		"640x480"
        ViewPort	0 0
        Virtual 	800 600
    EndSubsection

    SubSection "Display"
	Depth		4
        Modes		"640x480"
    EndSubSection

    SubSection "Display"
	Depth		1
        Modes		"640x480"
    EndSubSection

EndSection


Section "Screen"
    Identifier		"Screen MGA1"
    Device		"MGA Millennium I"
    Monitor		"Generic Monitor"
    Option		"no accel"
    DefaultDepth	16
XCOMM    DefaultDepth	24

    SubSection "Display"
	Depth		8
	Modes		"1280x1024"
	Option		"rgb bits" "8"
	Visual		"StaticColor"
    EndSubSection
    SubSection "Display"
	Depth		16
	Modes		"1280x1024"
    EndSubSection
    SubSection "Display"
	Depth		24
	Modes		"1280x1024"
    EndSubSection
EndSection


Section "Screen"
    Identifier		"Screen MGA2"
    Device		"MGA G200 AGP"
    Monitor		"Generic Monitor"
    DefaultDepth	8

    SubSection "Display"
	Depth		8
	Modes		"1280x1024"
	Option		"rgb bits" "8"
	Visual		"StaticColor"
    EndSubSection
EndSection


XCOMM **********************************************************************
XCOMM ServerLayout sections.
XCOMM **********************************************************************

XCOMM Any number of ServerLayout sections may be present.  Each describes
XCOMM the way multiple screens are organised.  A specific ServerLayout
XCOMM section may be specified from the X server command line with the
XCOMM "-layout" option.  In the absence of this, the first section is used.
XCOMM When now ServerLayout section is present, the first Screen section
XCOMM is used alone.

Section "ServerLayout"

XCOMM The Identifier line must be present

    Identifier	"Main Layout"

XCOMM Each Screen line specifies a Screen section name, and optionally
XCOMM the relative position of other screens.  The four names after
XCOMM primary screen name are the screens to the top, bottom, left and right
XCOMM of the primary screen.  In this example, screen 2 is located to the
XCOMM right of screen 1.

    Screen	"Screen MGA 1"	""	""	""	"Screen MGA 2"
    Screen	"Screen MGA 2"	""	""	"Screen MGA 1"	""

EndSection


Section "ServerLayout"
    Identifier	"another layout"
    Screen	"Screen 1"
    Screen	"Screen MGA 1"
EndSection


Section "ServerLayout"
    Identifier	"simple layout"
    Screen	"Screen 1"
EndSection


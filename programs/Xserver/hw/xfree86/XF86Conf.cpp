XCOMM $XFree86: xc/programs/Xserver/hw/xfree86/XF86Conf.cpp,v 3.2 1994/09/18 08:48:11 dawes Exp $
XCOMM
XCOMM Copyright (c) 1994 by The XFree86 Project, Inc.
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

XCOMM **********************************************************************
XCOMM Refer to the XF86Config(4/5) man page for details about the format of 
XCOMM this file. This man page is installed as MANPAGE 
XCOMM **********************************************************************

XCOMM **********************************************************************
XCOMM Files section.  This allows default font and rgb paths to be set
XCOMM **********************************************************************

Section "Files"

    RgbPath	RGBPATH

XCOMM Multiple FontPath entries are allowed (which are concatenated together),
XCOMM as well as specifying multiple comma-separated entries in one FontPath
XCOMM command (or a combination of both methods)

    FontPath	MISCFONTPATH
    USE_T1FONTS	T1FONTPATH
    USE_SPFONTS	SPFONTPATH
    USE_75FONTS	DPI75FONTPATH
    USE_100FONTS	DPI100FONTPATH

EndSection

XCOMM **********************************************************************
XCOMM Server flags section.
XCOMM **********************************************************************

Section "ServerFlags"

XCOMM Uncomment this to cause a core dump at the spot where a signal is 
XCOMM received.  This may leave the console in an unusable state, but may
XCOMM provide a better stack trace in the core dump to aid in debugging

XCOMM    NoTrapSignals

XCOMM Uncomment this to disable the <Crtl><Alt><BS> server abort sequence

XCOMM    DontZap

EndSection

XCOMM **********************************************************************
XCOMM Input devices
XCOMM **********************************************************************

XCOMM **********************************************************************
XCOMM Keyboard section
XCOMM **********************************************************************

Section "Keyboard"

    Protocol	"Standard"

XCOMM when using XQUEUE, comment out the above line, and uncomment the
XCOMM following line

XCOMM    Protocol	"Xqueue"

    AutoRepeat	500 5
    ServerNumLock

XCOMM Specifiy which keyboard LEDs can be user-controlled (eg, with xset(1))
XCOMM    Xleds      1 2 3

XCOMM To set the LeftAlt to Meta, RightAlt key to ModeShift, 
XCOMM RightCtl key to Compose, and ScrollLock key to ModeLock:

XCOMM    LeftAlt     Meta
XCOMM    RightAlt    ModeShift
XCOMM    RightCtl    Compose
XCOMM    ScrollLock  ModeLock

EndSection


XCOMM **********************************************************************
XCOMM Pointer section
XCOMM **********************************************************************

Section "Pointer"

    Protocol	"Microsoft"
    Device	MOUSEDEV

XCOMM When using XQUEUE, comment out the above two lines, and uncomment
XCOMM the following line.

XCOMM    Protocol	"Xqueue"

XCOMM Baudrate and SampleRate are only for some Logitech mice

XCOMM    BaudRate	9600
XCOMM    SampleRate	150

XCOMM Emulate3Buttons is an option for 2-button Microsoft mice

XCOMM	Emulate3Buttons

XCOMM ChordMiddle is an option for some 3-button Logitech mice

XCOMM	ChordMiddle

EndSection


XCOMM **********************************************************************
XCOMM Monitor section
XCOMM **********************************************************************

XCOMM Any number of monitor sections may be present

Section "Monitor"

    Identifier	"Generic Monitor"
    VendorName	"Unknown"
    ModelName	"Unknown"

XCOMM Bandwidth is in MHz unless units are specified

    Bandwidth	25.2

XCOMM HorizSync is in kHz unless units are specified.
XCOMM HorizSync may be a comma separated list of discrete values, or a
XCOMM comma separated list of ranges of values.

    HorizSync   31.5

XCOMM    HorizSync	30-64
XCOMM    HorizSync	31.5, 35.2
XCOMM    HorizSync	15-25, 30-50

XCOMM VertRefresh is in Hz unless units are specified.
XCOMM VertRefresh may be a comma separated list of discrete values, or a
XCOMM comma separated list of ranges of values.

    VertRefresh 60

XCOMM Modes can be specified in two formats.  A compact on-line format, or
XCOMM a multi-line format.

XCOMM    ModeLine "640x480" 25 640 664 760 800 480 491 493 525

    Mode "640x480"
        DotClock	25
        HTimings	640 664 760 800
        VTimings	480 491 493 525
    EndMode

XCOMM    ModeLine "1024x768i" 45 1024 1048 1208 1264 768 776 784 817 Interlace

XCOMM    Mode "1024x768i"
XCOMM        DotClock	45
XCOMM        HTimings	1024 1048 1208 1264
XCOMM        VTimings	768 776 784 817
XCOMM        Flags		"Interlace"
XCOMM    EndMode

EndSection

XCOMM **********************************************************************
XCOMM Graphics device section
XCOMM **********************************************************************

XCOMM Any number of graphics device sections may be present

Section "Device"
    Identifier	"Generic VGA"
    VendorName	"Unknown"
    BoardName	"Unknown"
    Chipset	"generic"

XCOMM    VideoRam	256

XCOMM    Clocks	25.2 28.3

EndSection

XCOMM Section "Device"
XCOMM    Identifier	"Any Trident TVGA 9000"
XCOMM    VendorName	"Trident"
XCOMM    BoardName	"TVGA 9000"
XCOMM    Chipset	"tvga9000"
XCOMM    VideoRam	512
XCOMM    Clocks	25 28 45 36 57 65 50 40 25 28 0 45 72 77 80 75
XCOMM EndSection

XCOMM Section "Device"
XCOMM    Identifier	"Actix GE32+ 2MB"
XCOMM    VendorName	"Actix"
XCOMM    BoardName	"GE32+"
XCOMM    Ramdac	"ATT20C490"
XCOMM    Dacspeed	110
XCOMM    Option	"dac_8_bit"
XCOMM    Clocks	 25.0  28.0  40.0   0.0  50.0  77.0  36.0  45.0
XCOMM    Clocks	130.0 120.0  80.0  31.0 110.0  65.0  75.0  94.0
XCOMM EndSection


XCOMM **********************************************************************
XCOMM Screen sections
XCOMM **********************************************************************

XCOMM The colour SVGA server

Section "Screen"
    Driver	"svga"
    Device	"Any Trident TVGA9000"
    Monitor	"Generic Monitor"
    Subsection "Display"
        Depth	    8
        Modes	    "640x480"
        ViewPort    0 0
        Virtual     800 600
    EndSubsection
EndSection

XCOMM The 16-colour VGA server

Section "Screen"
    Driver	"vga16"
    Device	"Generic VGA"
    Monitor	"Generic Monitor"
    Subsection "Display"
        Modes	    "640x480"
        ViewPort    0 0
        Virtual     800 600
    EndSubsection
EndSection

XCOMM The Mono server

Section "Screen"
    Driver	"vga2"
    Device	"Generic VGA"
    Monitor	"Generic Monitor"
    Subsection "Display"
        Modes	    "640x480"
        ViewPort    0 0
        Virtual     800 600
    EndSubsection
EndSection

XCOMM The accelerated servers (S3, Mach32, Mach8, 8514, P9000, AGX, W32)

Section "Screen"
    Driver	"accel"
    Device	"Actix GE32+ 2MB"
    Monitor	"Generic Monitor"
    Subsection  "Display"
        Depth	    8
        Modes	    "640x480"
        ViewPort    0 0
        Virtual	    1280 1024
    EndSubsection
    SubSection "Display"
        Depth	    16
        Weight	    565
        Modes	    "640x480"
        ViewPort    0 0
        Virtual	    1024 768
    EndSubsection
EndSection


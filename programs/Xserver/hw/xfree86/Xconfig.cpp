XCOMM $XConsortium: Xconfig.cpp,v 1.2 94/03/28 21:01:38 dpw Exp $
XCOMM
XCOMM Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
XCOMM Copyright 1992,93 by David Dawes, David Wexelblat
XCOMM
XCOMM Permission to use, copy, modify, distribute, and sell this software 
XCOMM and its documentation for any purpose is hereby granted without fee, 
XCOMM provided that the above copyright notice appear in all copies and 
XCOMM that both that copyright notice and this permission notice appear in 
XCOMM supporting documentation, and that the names of the above listed authors 
XCOMM not be used in advertising or publicity pertaining to distribution of 
XCOMM the software without specific, written prior permission.  The above 
XCOMM listed authors make no representations about the suitability of this 
XCOMM software for any purpose.  It is provided "as is" without express or 
XCOMM implied warranty.
XCOMM
XCOMM THE ABOVE LISTED AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS 
XCOMM SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
XCOMM FITNESS, IN NO EVENT SHALL THE ABOVE LISTED AUTHORS BE LIABLE FOR 
XCOMM ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
XCOMM RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
XCOMM CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
XCOMM CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
XCOMM
XCOMM Author:  Thomas Roell, roell@informatik.tu-muenchen.de
XCOMM
XCOMM Extensive modifications by the XFree86 Core Team

XCOMM **********************************************************************
XCOMM Refer to the Xconfig(4/5) man page for details about the format of 
XCOMM this file. This man page is installed as MANPAGE 
XCOMM **********************************************************************

XCOMM **********************************************************************
XCOMM Generic parameters section
XCOMM **********************************************************************

XCOMM
XCOMM some nice paths, to avoid conflicts with other X-servers
XCOMM
RGBPath		RGBPATH

XCOMM
XCOMM Multiple FontPath entries are allowed (which are concatenated together),
XCOMM as well as specifying multiple comma-separated entries in one FontPath
XCOMM command (or a combination of both methods)
XCOMM
FontPath	MISCFONTPATH
USE_T1FONTS	T1FONTPATH
USE_SPFONTS	SPFONTPATH
USE_75FONTS	DPI75FONTPATH
USE_100FONTS	DPI100FONTPATH

XCOMM
XCOMM Uncomment this to cause a core dump at the spot where a signal is 
XCOMM received.  This may leave the console in an unusable state, but may
XCOMM provide a better stack trace in the core dump to aid in debugging
XCOMM
XCOMM NoTrapSignals

XCOMM **********************************************************************
XCOMM Input devices
XCOMM **********************************************************************

XCOMM
XCOMM Enable this to use the XQUEUE driver for keyboard and mouse handling
XCOMM under System V.  This may go away in the future.
XCOMM
XCOMM Note - If you use XQUEUE, you must comment out the keyboard and
XCOMM        mouse definitions.
XCOMM
XCOMM Xqueue

XCOMM
XCOMM Keyboard and various keyboard-related parameters
XCOMM
Keyboard
  AutoRepeat 500 5
  ServerNumLock
XCOMM  Xleds      1 2 3
XCOMM  DontZap
XCOMM
XCOMM To set the LeftAlt to Meta, RightAlt key to ModeShift, 
XCOMM RightCtl key to Compose, and ScrollLock key to ModeLock:
XCOMM 
XCOMM  LeftAlt     Meta
XCOMM  RightAlt    ModeShift
XCOMM  RightCtl    Compose
XCOMM  ScrollLock  ModeLock

XCOMM
XCOMM Mouse definition and related parameters
XCOMM
Logitech	MOUSEDEV
XCOMM  BaudRate	9600
XCOMM  SampleRate	150
XCOMM  Emulate3Buttons

XCOMM **********************************************************************
XCOMM Graphics drivers
XCOMM **********************************************************************

XCOMM
XCOMM The 8-bit colour SVGA driver
XCOMM
VGA256

XCOMM
XCOMM To disable SpeedUp, use NoSpeedUp
XCOMM
XCOMM  NoSpeedUp
XCOMM  Virtual	1152 900

  Virtual	1024 1024
  ViewPort      0 0
  Modes	        "1024x768" "800x600" "640x480"

XCOMM 
XCOMM The 16-colour VGA driver
XCOMM
VGA16
  Virtual 	800 600
  Viewport 	0 0 
  Modes		"640x480"

XCOMM
XCOMM The 1-bit mono SVGA driver
XCOMM
VGA2

  Virtual	800 600
  ViewPort      0 0
  Modes	        "640x480"
XCOMM  Modes        "800x600" "640x480"

XCOMM
XCOMM The Hercules driver.  For Hercules, the only valid configuration option
XCOMM is ScreenNo (refer to the manual page).
XCOMM
XCOMM HGA2

XCOMM
XCOMM The alternate monochrome driver.  Refer to the XF86_Mono manual page.
XCOMM
XCOMM BDM2

XCOMM 
XCOMM The accelerated servers (S3, Mach32, Mach8, 8514)
XCOMM
ACCEL

  Virtual 	1024 768
  Viewport	0 0
  Modes		"1024x768" "800x600" "640x480"

XCOMM
XCOMM For boards with a programmable clock generator, you use a line like:
XCOMM
XCOMM Clocks "icd2061a"

XCOMM **********************************************************************
XCOMM Database of video modes
XCOMM **********************************************************************
ModeDB
XCOMM name        clock   horizontal timing     vertical timing      flags

XCOMM
XCOMM Refer to README.Config, modeDB.txt, and VideoModes.doc for actual
XCOMM modes, and information on how to calculate and adjust them.  
XCOMM
XCOMM DO NOT BLINDLY USE VIDEO MODES WITHOUT UNDERSTANDING WHAT YOU ARE
XCOMM DOING.  IT IS POSSIBLE TO DAMAGE THE MONITOR.  THE XFree86 CORE TEAM
XCOMM DISCLAIMS ALL LIABILITY FOR MONITOR DAMAGE, AS THE DOCUMENTATION
XCOMM ACCOMPANYING XFree86 HAS BEEN VERIFIED TO CONTAIN VALID AND SAFE
XCOMM MODES, AS LONG AS ONLY ONES WITHIN DOCUMENTED MONITOR SPECIFICATIONS
XCOMM ARE USED.
XCOMM

.\" Header:   //Mercury/Projects/archives/XFree86/4.0/siliconmotion.cpp-arc   1.4   29 Nov 2000 14:12:56   Frido  $
.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/siliconmotion/siliconmotion.cpp,v 1.1 2000/11/28 20:59:19 dawes Exp $
.TH siliconmotion __drivermansuffix__ "Version 4.0"  "XFree86"
.SH NAME
siliconmotion \- Silicon Motion video driver
.SH SYNOPSIS
.B "Section ""Device"""
.br
.BI "  Identifier """  devname """"
.br
.B  "  Driver ""siliconmotion"""
.br
\ \ ...
.br
\ \ [
.B "Option"
"optionname" ["optionvalue"]]
.br
.B EndSection
.SH DESCRIPTION
.B siliconmotion 
is an XFree86 driver for Silicon Motion based video cards.  The driver is fully
accelerated, and provides support for the following framebuffer depths:
8, 16, and 24.  All
visual types are supported for depth 8, and TrueColor
visuals are supported for the other depths.
.SH SUPPORTED HARDWARE
The
.B siliconmotion
driver supports PCI and AGP video cards based on the following Silicon Motion chips:
.TP 12
.B Lynx
SM910
.TP 12
.B LynxE
SM810
.TP 12
.B Lynx3D
SM820
.TP 12
.B LynxEM
SM710
.TP 12
.B LynxEM+
SM712
.TP 12
.B Lynx3DM
SM720
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.  All options names are case and white space insensitive when
parsed by the server, for exmaple,  "lynxe" and "LynxE" are equivalent.
.PP
The driver auto-detects the chipset type, but the following
.B ChipSet
names may optionally be specified in the config file
.B """Device"""
section, and will override the auto-detection:
.PP
.RS 4
"lynx", "lynxe", "lynx3d", "lynxem", "lynxem+", "lynx3dm".
.RE

.PP
The following Cursor
.B Options
are supported:
.TP
.BI "Option ""HWCursor"" [""" boolean """]
Enable or disable the HW cursor.  Default: on.
.TP
.BI "Option ""SWCursor"" [""" boolean """]
Inverse of "HWCursor".  Default: off.

.PP
The following display
.B Options
are supported:
.TP
.BI "Option ""ShadowFB"" [""" boolean """]
Use shadow framebuffer.  Default: off.
.TP
.BI "Option ""Rotate"" """ cw " | " ccw """
Rotate the screen CW - clockwise or CCW - counter clockwise.
Uses ShadowFB.  Default: no rotation.
.TP
.BI "Option ""VideoKey"" [""" integer """]
Set the video colorkey.  Default: a little off full blue.
.TP
.BI "Option ""ByteSwap"" [""" boolean """]
Turn on byte swapping for capturing using SMI demo board.  Default: off.
.TP
.BI "Option ""UseBIOS"" [""" boolean """]
Use the BIOS to set the modes. This is used for custom panel timings.
Default: on.

.PP
The following video memory
.B Options
are supported:
.TP
.BI "Option ""set_mclk"" """ integer """
sets the memory clock, where
.I integer
is in kHz, and
.I integer
<= 100000.  Default: probe the memory clock value,
and use it at server start.


.PP
The following acceleration and graphics engine
.B Options
are supported:
.TP
.B "Option ""NoAccel""
Disable acceleration.  Very useful for determining if the
driver has problems with drawing and acceleration routines.  This is the first
option to try if your server runs but you see graphic corruption on the screen.
Using it decreases performance, as it uses software emulation for drawing
operations the video driver can accelerate with hardware.
Default: acceleration is enabled.
.TP
.BR "Option ""fifo_aggressive " | " fifo_moderate " | " fifo_conservative"""
alter the settings
for the threshold at which the pixel FIFO takes over the internal 
memory bus to refill itself. The smaller this threshold, the better
the acceleration performance of the card. You may try the fastest 
setting
.RB ( "fifo_aggressive" )
and move down if you encounter pixel corruption.
The optimal setting will probably depend on dot-clock and on color 
depth. Note that specifying any of these options will also alter other
memory settings which may increase performance, so trying
.B "fifo_conservative"
will in most cases be a slight benefit (this uses the chip defaults).
If pixel corruption or transient streaking is observed during drawing
operations then removing any fifo options is recommended.  Default: none.

.PP
The following PCI bus
.B Options
are supported:
.TP
.BI "Option ""pci_burst"" [""" boolean """]
will enable PCI burst mode. This should work on all but a
few broken PCI chipsets, and will increase performance.  Default: off.
.TP
.BI "Option ""pci_retry"" [""" boolean """]
will allow the driver to rely on PCI Retry to program the 
ViRGE registers. 
.B "pci_burst"
must be enabled for this to work. 
This will increase performance, especially for small fills/blits, 
because the driver does not have to poll the ViRGE before sending it 
commands to make sure it is ready. It should work on most 
recent PCI chipsets.  Default: off.

.PP
The following additional
.B Options
are supported:
.TP
.BI "Option ""ShowCache"" [""" boolean """]
Enable or disable viewing offscreen cache memory.  A
development debug option.  Default: off.

.SH SEE ALSO
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(1)

.SH SUPPORT
For assistance with this driver, or XFree86 in general, check the XFree86 web
site at http://www.xfree86.org.  A FAQ is available on the web site at
http://www.xfree86.org/FAQ/.  If you find a problem with XFree86 or have a
question not answered in the FAQ please use our bug report form available on
the web site or send mail to XFree86@XFree86.org.  When reporting problems
with the driver send as much detail as possible, including chipset type, a 
server output log, and operating system specifics.

.SH AUTHORS
Kevin Brosius, 
Matt Grossman, 
Harald Koenig,
Sebastien Marineau,
Mark Vojkovich,
Frido Garritsen.

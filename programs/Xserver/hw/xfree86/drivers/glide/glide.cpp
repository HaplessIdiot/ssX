.\" $XFree86$
.TH GLIDE __drivermansuffix__ "Version 4.0"  "XFree86"
.SH NAME
glide \- Glide video driver
.SH SYNOPSIS
.B "Section ""Device"""
.br
.BI "  Identifier """  devname """"
.br
.B  "  Driver ""glide"""
.br
\ \ ...
.br
.B EndSection
.SH DESCRIPTION
.B glide 
is an XFree86 driver for Glide capable video cards (such as 3dfx
Voodoo cards).  The driver is a bit special because Voodoo cards are
very much NOT made for running 2D graphics. Therefore, this driver
uses no hardware acceleration (since there is no acceleration for 2D,
only 3D). Instead it is implemented with the help of a "shadow"
framebuffer that resides entirely in RAM. Selected portions of this
shadow framebuffer is then copied out to the voodoo board at the right
time. Because of this, the speed of the driver is very dependent on
the CPU. But since the CPU is nowadays actually rather fast at moving
data, we get very good speed anyway, especially since the whole shadow
framebuffer is in cached RAM.
.PP
This driver requires that you have installed Glide. (Which can, at the
time of this writing, be found at
http://glide.xxedgexx.com/3DfxRPMS.html).  Also, this driver requires
that you tell XFree86 where the libglide2x.so shared library file is
placed. This is done by adding a new ModulePath line to the "Files" section of your
XF86Config file.  For example, if you have libglide2x.so in /usr/lib
(which is the most common), add the following line to your "Files"
section:

  ModulePath "/usr/lib"

Make sure you put this line AFTER all other ModulePath lines in this section.
.PP
If you have installed /dev/3dfx, the driver will be able to turn on
the MTRR registers (through the glide library) if you have a CPU with
such registers (see http://glide.xxedgexx.com/MTRR.html). This will
speed up copying data to the voodoo board by as much as 2.7 times and
is very noticeable since this driver copies a lot of
data... Recommended.
.PP
This driver supports 16 and 24 bit color modes. The 24 bit color mode
uses a 32 bit framebuffer (it has no support for 24 bit packed-pixel
framebuffers). Notice that the voodoo boards can only display 16 bit
color, but the shadow framebuffer can be run in 24 bit color. The
point of supporting 24 bit mode is that this enables you to run in a
multihead configuration with Xinerama together with another board that
runs in real 24 bit color mode. (All boards must run the same color
depth when you use Xinerama).
.PP
Resolutions supported are: 640x480, 800x600, 960x720, 1024x768,
1280x1024 and 1600x1200. Note that not all modes will work on all
voodoo boards.  It seems that voodoo2 baords support no higher than
1024x768. If you see a message like this in the output from the server:
.PP
(EE) GLIDE(0): grSstWinOpen returned ...
.PP
Then you are probably trying to use a resolution that is supported by
the driver but not supported by the hardware.
.PP
Refresh rates supported are: 60Hz, 75Hz and 85Hz. The refresh rate
used is derived from the normal mode line according
to the following table:
.TP 28
Mode-line refresh rate
Used refresh rate
.TP 28
   0-74 Hz
  60 Hz
.TP 28
  74-84 Hz
  75 Hz
.TP 28
  84-   Hz
  85 Hz
.PP
Thus, if you use a modeline that for example has a 70Hz refresh rate 
you will only get a 60Hz refresh rate in actuality.
.PP
Multihead and Xinerama configurations are supported.
.PP
Limited support for DPMS screen saving is available. The "standby" and
"suspend" modes are just painting the screen black. The "off" mode turns
the voodoo board off and thus works correctly.
.PP
Since this driver uses Glide, it will work correctly on SLI
configurations, treating both boards as one.
.PP
Selecting which voodoo board to use with the driver is done by using
the "BusID" line in the "Device" section. (You need to select a board
even if you only have one). For example: To use the
first voodoo board, use a "Device" section like this, for example:
.PP
Section "Device"
.br
   Identifier  "Voodoo"
.br
   Driver      "glide"
.br
   Option      "dpms" "on"
.br
   BusID       "0" 
.br
EndSection
.PP
And if you have more than one voodoo board, add another "Device"
section with a BusID of 1, and so on. (You can use more than one
voodoo board, but SLI configured boards will be treated as a single board.)
.SH SUPPORTED HARDWARE
The
.B glide
driver supports any card that can be used with Glide (such as 3dfx Voodoo cards)
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The following driver
.B Options
are supported:
.TP
.BI "Option ""OnAtExit"" """ boolean """
If true, will leave the voodoo board on when the server exits. Useful in a multihead setup when
only the voodoo board is connected to a second monitor and you don't want that monitor to lose
signal when you quit the server.
Default: off.
.SH "EXAMPLE"
Here is an example of an XF86Config file that uses a multihead
configuration with two monitors. The first monitor is driven by the
fbdev video driver and the second monitor is driven by the glide
driver. Also, in this example, the libglide2x.so file is placed in
/usr/lib/libglide2x.so.
.PP
.br
Section "Module"
.br
  Load	"dbe"
.br
EndSection
.br

.br
Section "Files"
.br
  RgbPath    "/usr/X11R6/lib/X11/rgb"
.br
  FontPath   "/usr/X11R6/lib/X11/fonts/misc:unscaled"
.br
  FontPath   "/usr/X11R6/lib/X11/fonts/75dpi:unscaled"
.br
  FontPath   "/usr/X11R6/lib/X11/fonts/100dpi:unscaled"
.br
  FontPath   "/usr/X11R6/lib/X11/fonts/Type1"
.br
  FontPath   "/usr/X11R6/lib/X11/fonts/Speedo"
.br
  FontPath   "/usr/X11R6/lib/X11/fonts/misc"
.br
  FontPath   "/usr/X11R6/lib/X11/fonts/75dpi"
.br
  ModulePath "/usr/X11R6/lib/modules"
.br
  # The next line is important to find libglide2x.so
.br
  ModulePath "/usr/lib"
.br
EndSection
.br

.br
Section "ServerFlags"
.br
EndSection
.br

.br
Section "Keyboard"
.br
   Protocol        "Standard"
.br
   XkbRules        "xfree86"
.br
   XkbModel        "pc104"
.br
   XkbLayout       "us"
.br
   AutoRepeat	   500 5
.br
EndSection
.br

.br
Section "Pointer"
.br
   # I have a Logitech MouseMan+ (wheelmouse)
.br
   # connected to the PS/2 port
.br
   Protocol        "imps/2"
.br
   Device          "/dev/mouse"
.br
   ZAxisMapping 4 5
.br
EndSection
.br

.br
Section "Monitor"
.br
   Identifier      "Monitor 1"
.br
   VendorName      "Unknown"
.br
   ModelName       "Unknown"
.br
   HorizSync       30-70
.br
   VertRefresh     50-80
.br

.br
   # 1024x768 @ 76 Hz, 62.5 kHz hsync
.br
   Modeline "1024x768" 85 1024 1032 1152 1360 768 784 787 823
.br
EndSection
.br

.br
Section "Monitor"
.br
   Identifier      "Monitor 2"
.br
   VendorName      "Unknown"
.br
   ModelName       "Unknown"
.br
   HorizSync       30-70
.br
   VertRefresh     50-80
.br

.br
   # 1024x768 @ 76 Hz, 62.5 kHz hsync
.br
   Modeline "1024x768" 85 1024 1032 1152 1360 768 784 787 823
.br
EndSection
.br

.br
Section "Device"
.br
   Identifier  "fb"
.br
   Driver      "fbdev"
.br
   option      "shadowfb"
.br
   Option      "dpms" "on"
.br
   # My video card is on the AGP bus which is usually
.br
   # located as PCI bus 1, device 0, function 0.
.br
   BusID       "PCI:1:0:0"
.br
EndSection
.br

.br
Section "Device"
.br
   # I have a voodoo 2 board
.br
   Identifier  "Voodoo"
.br
   Driver      "glide"
.br
   Option      "OnAtExit"
.br
   Option      "dpms" "on"
.br
   # The next line says I want to use the first voodoo card
.br
   BusID       "0"
.br
EndSection
.br

.br
Section "Screen"
.br
  Identifier	"Screen 1"
.br
  Device	"fb"
.br
  Monitor	"Monitor 1"
.br
  DefaultDepth	16
.br
  Subsection "Display"
.br
    Depth	16
.br
    Modes	"1024x768"
.br
  EndSubSection
.br
EndSection
.br

.br
Section "Screen"
.br
  Identifier	"Screen 2"
.br
  Device	"Voodoo"
.br
  Monitor	"Monitor 2"
.br
  DefaultDepth	16
.br
  Subsection "Display"
.br
    Depth	16
.br
    Modes	"1024x768"
.br
  EndSubSection
.br
EndSection
.br

.br
Section "ServerLayout"
.br
  Identifier	"Main Layout"
.br
  # Screen 1 is to the right and screen 2 is to the left
.br
  Screen	"Screen 2" 
.br
  Screen	"Screen 1" "" "" "Screen 2" ""
.br
EndSection
.PP
If you use this configuration file and start the server with the
+xinerama command line option, the two monitors will be showing a
single large area where windows can be moved between monitors and
overlap from one monitor to the other. Starting the X server with the
Xinerama extension can be done for example like this:
.PP
$ xinit -- +xinerama
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(1)
.SH AUTHORS
Authors include: Henrik Harmsen.

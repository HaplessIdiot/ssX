.\" $XFree86: xc/programs/Xserver/hw/xfree86/XF86Config.cpp,v 1.1 2000/03/07 01:37:42 dawes Exp $
.TH XF86Config __filemansuffix__ "Version 4.0"  "XFree86"
.SH NAME
XF86Config - Configuration File for XFree86
.SH DESCRIPTION
.I XFree86
uses a configuration file called
.B XF86Config
for its initial setup.  This configuration file is searched for in the
following places when the server is started as a normal user:
.PP
.RS 4
.nf
.RI /etc/X11/ <cmdline>
.RI __projectroot__/etc/X11/ <cmdline>
.RB /etc/X11/ $XF86CONFIG
.RB __projectroot__/etc/X11/ $XF86CONFIG
/etc/X11/XF86Config-4
/etc/X11/XF86Config
/etc/XF86Config
.RI __projectroot__/etc/X11/XF86Config. <hostname>
__projectroot__/etc/X11/XF86Config-4
__projectroot__/etc/X11/XF86Config
.RI __projectroot__/lib/X11/XF86Config. <hostname>
__projectroot__/lib/X11/XF86Config-4
__projectroot__/lib/X11/XF86Config
.fi
.RE
.PP
where
.I <cmdline>
is a relative path (with no ".." components) specified with the
.B \-xf86config
command line option,
.B $XF86CONFIG
is the relative path (with no ".." components) specified by that
environment variable, and
.I <hostname>
is the machines hostname as reported by gethostname(3).
.PP
When the X server is started by the "root" user, the config file search
locations are as follows:
.PP
.RS 4
.nf
.I <cmdline>
.RI /etc/X11/ <cmdline>
.RI __projectroot__/etc/X11/ <cmdline>
.B $XF86CONFIG
.RB /etc/X11/ $XF86CONFIG
.RB __projectroot__/etc/X11/ $XF86CONFIG
.BR $HOME /XF86Config
/etc/X11/XF86Config-4
/etc/X11/XF86Config
/etc/XF86Config
.RI __projectroot__/etc/X11/XF86Config. <hostname>
__projectroot__/etc/X11/XF86Config-4
__projectroot__/etc/X11/XF86Config
.RI __projectroot__/lib/X11/XF86Config. <hostname>
__projectroot__/lib/X11/XF86Config-4
__projectroot__/lib/X11/XF86Config
.fi
.RE
.PP
where
.I <cmdline>
is the path specified with the
.B \-xf86config
command line option (which may be absolute or relative),
.B $XF86CONFIG
is the path specified by that
environment variable (absolute or relative),
.B $HOME
is the path specified by that environment variable (usually the home
directory), and
.I <hostname>
is the machines hostname as reported by gethostname(3).
.PP
The
.B XF86Config
file is composed of a number of sections which may be present in any
order.  Each section has
the form:
.PP
.RS 4
.nf
.BI "Section """ SectionName """
.I  "    SectionEntry"
    ...
.B EndSection
.fi
.RE
.PP
The section names are:
.PP
.RS 4
.nf
.BR "Files          " "File pathnames"
.BR "ServerFlags    " "Server flags"
.BR "Module         " "Dynamic module loading"
.BR "InputDevice    " "Input device description"
.BR "Device         " "Graphics device description"
.BR "VideoAdapter   " "Xv video adapter description"
.BR "Monitor        " "Monitor description"
.BR "Modes          " "Video modes descriptions"
.BR "Screen         " "Screen configuration"
.BR "ServerLayout   " "Overall layout"
.BR "DRI            " "DRI-specific configuration"
.BR "Vendor         " "Vendor-specific configuration"
.fi
.RE
.PP
The following obsolete section names are still recognised for compatibility
purposes.  In new config files, the
.B InputDevice
section should be used instead.
.PP
.RS 4
.nf
.BR "Keyboard       " "Keyboard configuration"
.BR "Pointer        " "Pointer/mouse configuration"
.fi
.RE
.PP
The old
.B XInput
section is no longer recognised.
.PP
Config file keywords are case-insensitive, and "_" characters are
ignored.  Most strings (including
.B Option
names) are also case-insensitive, and insensitive to white space and
"_" characters.
.PP
Each config file entry usually take up a single line in the file.
They consist of a keyword, which is possibly followed by one or
more arguments, with the number and types of the arguments depending
on the keyword.  The argument types are:
.PP
.RS 4
.nf
.BR "Integer     " "an integer number in decimal, hex or octal"
.BR "Real        " "a floating point number"
.BR "String      " "a string enclosed in double quote marks ("")"
.fi
.RE
.PP
A special keyword called
.B Option
may be used to provide free-form data to various components of the server.
The
.B Option
keyword takes either one or two string arguments.  The first is the option
name, and the optional second argument is the option value.  Some commonly
used option value types include:
.PP
.RS 4
.nf
.BR "Integer     " "an integer number in decimal, hex or octal"
.BR "Real        " "a floating point number"
.BR "String      " "a sequence of characters"
.BR "Boolean     " "a boolean value (see below)"
.BR "Frequency   " "a frequency value (see below)"
.fi
.RE
.PP
Note that
.I all
.B Option
values, not just strings, must be enclosed in quotes.
.PP
Boolean options may optionally have a value specified.  When no value
is specified, the option's value is
.BR TRUE .
The following boolean option values are recognised as
.BR TRUE :
.PP
.RS 4
.BR 1 ,
.BR on ,
.BR true ,
.B yes
.RE
.PP
and the following boolean option values are recognised as
.BR FALSE :
.PP
.RS 4
.BR 0 ,
.BR off ,
.BR false ,
.B no
.RE
.PP
If an option name is prefixed with
.RB """" No """",
then the option value is negated.
.PP
Example: the following option entries are equivalent:
.PP
.RS 4
.nf
.B "Option ""Accel""   ""Off"""
.B "Option ""NoAccel""
.B "Option ""NoAccel"" ""On""
.B "Option ""Accel""   ""false""
.B "Option ""Accel""   ""no""
.fi
.RE
.PP
Frequency option values consist of a real number that is optionally
followed by one of the following frequency units:
.PP
.RS 4
.BR Hz ,
.BR k ,
.BR kHz ,
.BR M ,
.B MHz
.RE
.PP
When the unit name is omitted, the correct units will be determined from
the value and the expectations of the appropriate range of the value.
It is recommended that the units always be specified when using frequency
option values to avoid any errors in determining the value.
.SH FILES SECTION
The
.B Files
section is used to specify some path names required by the server.
Some of these paths can also be set from the command line (see
.I Xserver(1)
and
.IR XFree86(1) ).
The command line settings override the values specified in the config
file.
The entries that can appear in this section are:
.TP 7
.BI "FontPath """ path """
sets the search path for fonts.  This path is a comma separated
list of font path elements which the X server searches for font databases.
Multiple
.B FontPath
entries may be specified, and they will be
concatenated to build up the fontpath used by the server.
Font path elements may be either absolute directory paths, or the
a font server identifier.  Font server identifiers have the form:
.PP
.RS 11
.IR <trans> / <hostname> : <port-number>
.RE
.PP
.RS 7
where
.I <trans>
is the transport type to use to connect to the font server (e.g.,
.B unix
for UNIX-domain sockets or
.B tcp
for a TCP/IP connection),
.I <hostname>
is the hostname of the machine running the font server, and
.I <port-number>
is the port number that the font server is listening on (usually 7100).
.PP
When this entry is not specified in the config file, the server falls back
to the compiled-in default font path, which contains the following
font path elements:
.PP
.RS 4
.nf
__projectroot__/lib/X11/fonts/misc/
__projectroot__/lib/X11/fonts/Speedo/
__projectroot__/lib/X11/fonts/Type1/
__projectroot__/lib/X11/fonts/CID/
__projectroot__/lib/X11/fonts/75dpi/
__projectroot__/lib/X11/fonts/100dpi/
.fi
.RE
.PP
The recommended font path contains the following font path elements:
.PP
.RS 4
.nf
__projectroot__/lib/X11/fonts/local/
__projectroot__/lib/X11/fonts/misc/
__projectroot__/lib/X11/fonts/75dpi/:unscaled
__projectroot__/lib/X11/fonts/100dpi/:unscaled
__projectroot__/lib/X11/fonts/Type1/
__projectroot__/lib/X11/fonts/CID/
__projectroot__/lib/X11/fonts/Speedo/
__projectroot__/lib/X11/fonts/75dpi/
__projectroot__/lib/X11/fonts/100dpi/
.fi
.RE
.PP
Font path elements that are found to be invalid are removed from the
font path when the server starts up.
.RE
.TP 7
.BI "RGBPath """ path """
sets the path name for the RGB color database.
When this entry is not specified in the config file, the server falls back
to the compiled-in default RGB path, which is:
.PP
.RS 11
__projectroot__/lib/X11/rgb
.RE
.TP 7
.BI "ModulePath """ path """
sets the search path for loadable X server modules.  This path is a
comma separated list of directories which the X server searches for
loadable modules loading in the order specified.  Multiple
.B ModulePath
entries may be specified, and they will be concatenated to build the
module search path used by the server.
.\" The LogFile keyword is not currently implemented
.ig
.TP 7
.BI "LogFile """ path """
sets the name of the X server log file.  The default log file name is
.PP
.RS 11
.RI __logdir__/XFree86. <n> .log
.RE
.PP
.RS 7
where
.I <n>
is the display number for the X server.
..
.SH SERVERFLAGS SECTION
The
.B ServerFlags
section is used to specify some global
X server options.  All of the entries in this section are
.BR Options ,
although for compatibility purposes some of the old style entries are
still recognised.  Those old style entries are not documented here, and
using them is discouraged.
.PP
.B Options
specified in this section may be overriden by
.B Options
specified in the active
.B ServerLayout
section.  Options with command line equivalents are overriden when their
command line equivalent is used.  The options recognised by this section
are:
.TP 7
.BI "Option ""NoTrapSignals""  """ boolean """
This prevents the X server from trapping a range of unexpected
fatal signals and exiting cleanly.  Instead, the X server will die
and drop core where the fault occurred.  The default behaviour is
for the X server exit cleanly, but still drop a core file.  In
general you never want to use this option unless you are debugging
an X server problem and know how to deal with the consequences.
.TP 7
.BI "Option ""DontZap""  """ boolean """
This disallows the use of the
.B Ctrl+Alt+Backspace
sequence.  That sequence is normally used to terminate the X server.
When this option is enabled, that key sequence has no special meaning
and is passed to clients.  Default: off.
.TP 7
.BI "Option ""DontZoom""  """ boolean """
This disallows the use of the
.B Ctrl+Alt+Keypad-Plus
and
.B Ctrl+Alt+Keypad-Minus
sequences.  These sequences allows you to switch between video modes.
When this option is enabled, those key sequences have no special meaning
and are passed to clients.  Default: off.
.TP 7
.BI "Option ""DisableVidModeExtension""  """ boolean """
This disables the parts of the VidMode extension used by the xvidtune client
that can be used to change the video modes.  Default: the VidMode extension
is enabled.
.TP 7
.BI "Option ""AllowNonLocalXvidtune""  """ boolean """
This allows the xvidtune client (and other clients that use the VidMode
extension) to connect from another host.  Default: off.
.TP 7
.BI "Option ""DisableModInDev""  """ boolean """
This disables the parts of the XFree86-Misc extension that can be used to
modify the input device settings dynamically.  Default: that functionality
is enabled.
.TP 7
.BI "Option ""AllowNonLocalModInDev""  """ boolean """
This allows a client to connect from another host and change keyboard
and mouse settings in the running server.  Default: off.
.TP 7
.BI "Option ""AllowMouseOpenFail""  """ boolean """
This allows the server to start up even if the mouse device can't be
opened/initialised.  Default: false.
.TP 7
.BI "Option ""VTInit""  """ command """
Runs
.I command
after the VT used by the server has been opened.
The command string is passed to "/bin/sh -c", and is run with the
real user's id with stdin and stdout set to the VT.  The purpose
of this option is to allow system dependent VT initialisation
commands to be run.  This option should rarely be needed.  Default: not set.
.TP 7
.BI "Option ""VTSysReq""  """ boolean """
enables the SYSV-style VT switch sequence for non-SYSV systems
which support VT switching.  This sequence is
.B Alt-SysRq
followed
by a function key
.RB ( Fn ).
This prevents the X server trapping the
keys used for the default VT switch sequence, which means that clients can
access them.  Default: off.
.\" The following four options are "undocumented".
.ig
.TP 7
.BI "Option ""PciProbe1"""
Use PCI probe method 1.  Default: set.
.TP 7
.BI "Option ""PciProbe2"""
Use PCI probe method 2.  Default: not set.
.TP 7
.BI "Option ""PciForceConfig1"""
Force the use PCI config type 1.  Default: not set.
.TP 7
.BI "Option ""PciForceConfig2"""
Force the use PCI config type 2.  Default: not set.
..
.TP 7
.BI "Option ""BlankTime""  """ time """
sets the inactivity timeout for the blanking phase of the screensaver.
.I time
is in minutes.  This is equivalent to the Xserver's `-s' flag, and the
value can be changed at run-time with \fIxset(1)\fP.  Default: 10 minutes.
.TP 7
.BI "Option ""StandbyTime""  """ time """
sets the inactivity timeout for the "standby" phase of DPMS mode.
.I time
is in minutes, and the value can be changed at run-time with \fIxset(1)\fP.
Default: 20 minutes.
This is only suitable for VESA DPMS compatible monitors, and may not be
supported by all video drivers.  It is only enabled for screens that
have the
.B """DPMS"""
option set. 
.TP 7
.BI "Option ""SuspendTime""  """ time """
sets the inactivity timeout for the "suspend" phase of DPMS mode.
.I time
is in minutes, and the value can be changed at run-time with \fIxset(1)\fP.
Default: 30 minutes.
This is only suitable for VESA DPMS compatible monitors, and may not be
supported by all video drivers.  It is only enabled for screens that
have the
.B """DPMS"""
option set. 
.TP 7
.BI "Option ""OffTime""  """ time """
sets the inactivity timeout for the "off" phase of DPMS mode.
.I time
is in minutes, and the value can be changed at run-time with \fIxset(1)\fP.
Default: 40 minutes.
This is only suitable for VESA DPMS compatible monitors, and may not be
supported by all video drivers.  It is only enabled for screens that
have the
.B """DPMS"""
option set. 
.TP 7
.BI "Option ""Pixmap""  """ bpp """
This sets the pixmap format to use for depth 24.  Allowed values for
.I bpp
are 24 and 32.  Default: 32 unless driver constraints don't allow this
(which is rare).  Note: some clients don't behave well when
this value is set to 24.
.TP 7
.BI "Option ""PC98""  """ boolean """
Specify that the machine is a Japanese PC-98 machine.  This should not
be enabled for anything other than the Japanese-specific PC-98
architecture.  Default: auto-detected.
.\" Doubt this should be documented.
.ig
.TP 7
.BI "Option ""EstimateSizesAggressively""  """ value """
This option affects the way that bus resource sizes are estimated.  Default: 0.
..
.TP 7
.BI "Option ""NoPM""  """ boolean """
Disables something to do with power management events.  Default: PM enabled
on platforms that support it.
.SH MODULE SECTION
The
.B Module
section is used to specify which X server modules should be loaded.
This section is ignored when the X server is built in static form.
The types of modules normally loaded in this section are X server
extension modules, and font rasteriser modules.  Most other module types
are loaded automatically when they are needed via other mechanisms.
.PP
Entries in this section may be in two forms.   The first and most commonly
used form is an entry that uses the
.B Load
keyword, as described here:
.TP 7
.BI "Load """ modulename """
This instructs the server to load the module called
.IR modulename .
The module name given should be the module's standard name, not the
module file name.  The standard name is case-sensitive, and does not
include the "lib" prefix, or the ".a", ".o", or ".so" suffixes.
.PP
.RS 7
Example: the Type 1 font rasteriser can be loaded with the following entry:
.PP
.RS 4
.B "Load ""type1"""
.RE
.RE
.PP
The second form of entry is a
.BR SubSection,
with the subsection name being the module name, and the contents of the
.B SubSection
being
.B Options
that are passed to the module when it is loaded.
.PP
Example: the extmod module (which contains a miscellaneous group of
server extensions) can be loaded, with the XFree86-DGA extension
disabled by using the following entry:
.PP
.RS 4
.nf
.B "SubSection ""extmod"""
.B "   Option  ""omit XFree86-DGA"""
.B EndSubSection
.fi
.RE
.PP
Modules are searched for in each directory specified in the
.B ModulePath
search path, and in the drivers, input, extensions, fonts, and
internal subdirectories of each of those directories.
In addition to this, operating system specific subdirectories of all
the above are searched first if they exist.
.PP
To see what font and extension modules are available, check the contents
of the following directories:
.PP
.RS 4
.nf
__projectroot__/lib/modules/fonts
__projectroot__/lib/modules/extensions
.fi
.RE
.PP
The "bitmap" font modules is loaded automatically.  It is recommended
that at very least the "extmod" extension module be loaded.  If it isn't
some commonly used server extensions (like the SHAPE extension) will not be
available.
.SH INPUTDEVICE SECTION
The config file may have multiple
.B InputDevice
sections.  There will normally be at least two: one for the core (primary)
keyboard, and one of the core pointer.
.PP
.B InputDevice
sections have the following format:
.PP
.RS 4
.nf
.B  "Section ""InputDevice"""
.BI "    Identifier """ name """
.BI "    Driver     """ inputdriver """
.I  "    options"
.I  "    ..."
.B  "EndSection"
.fi
.RE
.PP
The
.B Identifier
entry specifies the unique name for this input device.  The
.B Driver
entry specifies the name of the driver to use for this input device.
When using the loadable server, the input driver module
.RI """ inputdriver """
will be loaded for each active
.B InputDevice
section.  An
.B InputDevice
section is considered active if it is referenced by an active
.B ServerLayout
section, or if it is referenced by the
.B \-keyboard
or
.B \-pointer
command line options.
The most commonly used input drivers are "keyboard" and "mouse".
.PP
.B InputDevice
sections recognise some driver-independent
.BR Options ,
which are described here.  See the individual input driver manual pages
for a description of the device-specific options.
.TP 7
.BI "Option ""CorePointer"""
When this is set, the input device is installed as the core (primary)
pointer device.  There must be exactly one core pointer.  If this option
is not set here, or in the
.B ServerLayout
section, or from the
.B \-pointer
command line option, then the first input device that is capable of
being used as a core pointer will be selected as the core pointer.
This option is implicitly set when the obsolete
.B Pointer
section is used.
.TP 7
.BI "Option ""CoreKeyboard"""
When this is set, the input device is to be installed as the core
(primary) keyboard device.  There must be exactly one core keyboard.  If
this option is not set here, in the
.B ServerLayout
section, or from the
.B \-keyboard
command line option, then the first input device that is capable of
being used as a core keyboard will be selected as the core keyboard.
This option is implicitly set when the obsolete
.B Keyboard
section is used.
.TP 7
.BI "Option ""AlwaysCore""  """ boolean """
.TP 7
.BI "Option ""SendCoreEvents""  """ boolean """
Both of these options are equivalent, and when enabled cause the
input device to always report core events.  This can be used, for
example, to allow an additional pointer device to generate core
pointer events (like moving the cursor, etc).
.TP 4
.BI "Option ""HistorySize""  """ number """
Sets the motion history size.  Default: 0.
.TP 7
.BI "Option ""SendDragEvents""  """ boolean """
???
.SH DEVICE SECTION
The config file may have multiple
.B Device
sections.  There must be at least one, for the video card being used.
.PP
.B Device
sections have the following format:
.PP
.RS 4
.nf
.B  "Section ""Device"""
.BI "    Identifier """ name """
.BI "    Driver     """ driver """
.I  "    entries"
.I  "    ..."
.B  "EndSection"
.fi
.RE
.PP
The
.B Identifier
entry specifies the unique name for this graphics device.  The
.B Driver
entry specifies the name of the driver to use for this graphics device.
When using the loadable server, the driver module
.RI """ driver """
will be loaded for each active
.B Device
section.  A
.B Device
section is considered active if it is referenced by an active
.B Screen
section.
.PP
.B Device
sections recognise some driver-independent entries and
.BR Options ,
which are described here.  Not all drivers make use of these
driver-independent entries, and many of those that do don't require them
to be specified because the information is auto-detected.  See the
individual graphics driver manual pages for further information about
this, and for a description of the device-specific options.
Note that most of the
.B Options
listed here (but not the other entries) may be specified in the
.B Screen
section instead of here in the
.B Device
section.
.TP 7
.BI "BusID  """ bus-id """
This specifies the bus location of the graphics card.  For PCI/AGP cards,
the
.I bus-id
string has the form
.BI PCI: bus : device : function
(e.g., "PCI:1:0:0" might be appropriate for an AGP card).
This field is usually optional in single-head configurations when using
the primary graphics card.  In multi-head configurations, or when using
a secondary graphics card in a single-head configuration, this entry is
mandatory.  Its main purpose is to make an unambiguous connection between
the device section and the hardware it is representing.  This information
can usually be found by running the X server with the
.B \-scanpci
command line option.
.TP 7
.BI "Chipset  """ chipset """
This usually optional entry specifies the chipset used on the graphics
board.  In most cases this entry is not required because the drivers
will probe the hardware to determine the chipset type.  Don't
specify it unless the driver-specific documentation recommends that you
do.
.TP 7
.BI "Ramdac  """ ramdac-type """
This optional entry specifies the type of RAMDAC used on the graphics
board.  This is only used by a few of the drivers, and in most cases it
is not required because the drivers will probe the hardware to determine
the RAMDAC type where possible.  Don't specify it unless the
driver-specific documentation recommends that you do.
.TP 7
.BI "DacSpeed  " speed
.TP 7
.BI "DacSpeed  " "speed-8 speed-16 speed-24 speed-32"
This optional entry specifies the RAMDAC speed rating (which is usually
printed on the RAMDAC chip).  The speed is in MHz.  When one value is
given, it applies to all framebuffer pixel sizes.  When multiple values
are give, they apply to the framebuffer pixel sizes 8, 16, 24 and 32
respectively.  This is not used by many drivers, and only needs to be
specified when the speed rating of the RAMDAC is different from the
defaults built in to driver, or when the driver can't auto-detect the
correct defaults.  Don't specify it unless the driver-specific
documentation recommends that you do.
.TP 7
.BI "Clocks  " "clock ..."
specifies the pixel that are on your graphics board.  The clocks are in
MHz, and may be specified as a floating point number.  The value is
stored internally to the nearest kHz.  The ordering of the clocks is
important.  It must match the order in which they are selected on the
graphics board.  Multiple
.B Clocks
lines may be specified, and each is concatenated to form the list.  Most
drivers do not use this entry, and it is only required for some older
boards with non-programmable clocks.  Don't specify this entry unless
the driver-specific documentation explicitly recommends that you do.
.TP
.BI "ClockChip  """ clockchip-type """
This optional entry is used to specify the clock chip type on
graphics boards which have a programmable clock generator.  Only
a few X servers support programmable clock chips.  For details,
see the appropriate X server manual page.
.TP 7
.BI "VideoRam  " "mem"
This optional entry specifies the amount of video ram that is installed
on the graphics board. This is measured in kBytes.  In most cases this
is not required because the X server probes the graphics board to
determine this quantity.  The driver-specific documentation should
indicate when it might be needed.
.TP 7
.BI "BiosBase  " "baseaddress"
This optional entry specifies the base address of the video BIOS
for the VGA board.  This address is normally auto-detected, and should
only be specified if the driver-specific documentation recommends it.
.TP 7
.BI "MemBase  " "baseaddress"
This optional entry specifies the memory base address of a graphics
board's linear frame buffer.  This entry is not used by many drivers,
and it should only be specified if the driver-specific documentation
recommends it.
.TP 7
.BI "IOBase  " "baseaddress"
This optional entry specifies the IO base address.  This entry is not
used by many drivers, and it should only be specified if the
driver-specific documentation recommends it.
.TP 7
.BI "ChipID  " "id"
This optional entry specifies a numerical ID representing the chip type.
For PCI cards, it is usually the device ID.  This can be used to override
the auto-detection, but that should only be done when the driver-specific
documentation recommends it.
.TP 7
.BI "ChipRev  " "rev"
This optional entry specifies the chip revision number.  This can be
used to override the auto-detection, but that should only be done when
the driver-specific documentation recommends it.
.TP 7
.BI "TextClockFreq  " "freq"
This optional entry specifies the pixel clock frequency that is used
for the regular text mode.  The frequency is specified in MHz.  This is
rarely used.
.ig
.TP 7
This optional entry allows an IRQ number to be specified.
..
.TP 7
.BI "Option ""BackingStore""  """ boolean """
 ...
.SH VIDEOADAPTER SECTION
.SH MONITOR SECTION
The config file may have multiple
.B Monitor
sections.  There must be at least one, for the monitor being used.
.PP
.B Monitor
sections have the following format:
.PP
.RS 4
.nf
.B  "Section ""Monitor"""
.BI "    Identifier """ name """
.I  "    entries"
.I  "    ..."
.B  "EndSection"
.fi
.RE
.PP
The
.B Identifier
entry specifies the unique name for this monitor.
The other entries that may be used in
.B Monitor
sections are described below.
.TP 8
.B VendorName \fI"vendor"\fP
This optional entry specifies the monitor's manufacturer.
.TP 8
.B ModelName \fI"model"\fP
This optional entry specifies the monitor's model.
.TP 8
.B HorizSync \fIhorizsync-range\fP
gives the range(s) of horizontal sync frequencies supported by the
monitor.  \fIhorizsync-range\fP may be a comma separated list of
either discrete values or ranges of values.  A range of values is
two values separated by a dash.  By default the values are in units
of kHz.  They may be specified in MHz or Hz if \fBMHz\fP or \fBHz\fP
is added to the end of the line.  The data given here is used by the X
server to determine if video modes are within the specifications
of the monitor.  This information should be available in the
monitor's handbook.  If this entry is omitted, a default
range of 28\-33kHz is used.
.TP 8
.B VertRefresh \fIvertrefresh-range\fP
gives the range(s) of vertical refresh frequencies supported by
the monitor.  \fIvertrefresh-range\fP may be a comma separated list
of either discrete values or ranges of values.  A range of values
is two values separated by a dash.  By default the values are in
units of Hz.  They may be specified in MHz or kHz if \fBMHz\fP or
\fBkHz\fP is added to the end of the line.  The data given here is used
by the X server to determine if video modes are within the
specifications of the monitor.  This information should be available
in the monitor's handbook.  If this entry is omitted, a default range of
43-72Hz is used.
.TP 7
.BI "Gamma  " "gamma-value"
.TP 7
.BI "Gamma  " "red-gamma green-gamma blue-gamma"
This is an optional entry that can be used to specify the gamma
correction for the monitor.  It may be specified as either a single
value or as three separate RGB values.  Not all drivers are capable
of using this information.
.TP 7
.BI "UseModes  " "modesection-id"
 ...
.TP 8
.B Mode \fI"name"\fP
indicates the start of a multi-line video mode description.  The
mode description is terminated with an \fBEndMode\fP line.  The
mode description consists of the following entries:
.sp
.RS 8
.TP 4
.B DotClock \fIclock\fP
is the dot clock rate to be used for the mode.
.TP 4
.B HTimings \fIhdisp hsyncstart hsyncend htotal\fP
specifies the horizontal timings for the mode.
.TP 4
.B VTimings \fIvdisp vsyncstart vsyncend vtotal\fP
specifies the vertical timings for the mode.
.TP 4
.B Flags \fI"flag" ...\fP
specifies an optional set of mode flags.  \fB"Interlace"\fP indicates
that the mode is interlaced.  \fB"DoubleScan"\fP indicates a mode where
each scanline is doubled.  \fB"+HSync"\fP and \fB"-HSync"\fP can
be used to select the polarity of the HSync signal.  \fB"+VSync"\fP
and \fB"-VSync"\fP can be used to select the polarity of the VSync
signal.  \fB"Composite"\fP, can be used to specify composite sync on
hardware where this is supported.  Additionally, on some hardware,
\fB"+CSync"\fP and \fB"-CSync"\fP may be used to select the composite
sync polarity.
.TP 4
.B HSkew \fIhskew\fP
specifies the number of pixels (towards the right edge of the screen) by which
the display enable signal is to be skewed.  Not all drivers use this
information.  This option might become necessary to override the default
value supplied by the server (if any).  "Roving" horizontal lines indicate this
value needs to be increased.  If the last few pixels on a scan line appear on
the left of the screen, this value should be decreased.
.TP 4
.B VScan \fIvscan\fP
specifies the number of times each scanline is painted on the screen.  Not all
drivers use this information.  Values less than 1 are treated as 1, which is
the default.  Generally, the \fB"DoubleScan"\fP flag mentionned above doubles
this value.
.RE
.TP 8
.B Modeline \fI"name" mode-description\fP
is a single line format for specifying video modes.  The
\fImode-description\fP is in four sections, the first three of
which are mandatory.  The first is the pixel clock.  This is a
single number specifying the pixel clock rate for the mode.  The
second section is a list of four numbers specifying the horizontal
timings.  These numbers are the \fIhdisp\fP, \fIhsyncstart\fP,
\fIhsyncend\fP, \fIhtotal\fP.  The third section is a list of four
numbers specifying the vertical timings.  These numbers are
\fIvdisp\fP, \fIvsyncstart\fP, \fIvsyncend\fP, \fIvtotal\fP.  The
final section is a list of flags specifying other characteristics
of the mode.  \fBInterlace\fP indicates that the mode is interlaced.
\fBDoubleScan\fP indicates a mode where each scanline is doubled.  
\fB+HSync\fP and \fB\-HSync\fP can be used to select the polarity
of the HSync signal.  \fB+VSync\fP and \fB\-VSync\fP can be used
to select the polarity of the VSync signal.  \fBComposite\fP can be
used to specify composite sync on hardware where this is supported.
Additionally, on some hardware,
\fB+CSync\fP and \fB-CSync\fP may be used to select the composite
sync polarity.  The \fBHSkew\fP and \fBVScan\fP options mentioned above can
also be used here.
.SH MODES SECTION
.SH SCREEN SECTION
The \fBScreen\fP sections are used to specify which graphics boards
and monitors will be used with a particular X server, and the
configuration in which they are to be used.  The entries available
for this section are:
.TP 8
.B Driver \fI"driver-name"\fP
Each \fBScreen\fP section must begin with a \fBDriver\fP entry,
and the \fIdriver-name\fP given in each \fBScreen\fP section must
be unique.  The driver name determines which X server (or driver
type within an X server when an X server supports more than one
head) reads and uses a particular \fBScreen\fP section.  The driver
names available are:
.sp
.in 20
.nf
.B Accel
.B Mono
.B SVGA
.B VGA2
.B VGA16
.fi
.in -20
.RS 8
.PP
\fBAccel\fP is used by all the accelerated X servers (see
\fIXF86_Accel(1)\fP).  \fBMono\fP is used by the non-VGA mono
drivers in the 2-bit and 4-bit X servers (see \fIXF86_Mono(1)\fP
and \fIXF86_VGA16(1)\fP).  \fBVGA2\fP and \fBVGA16\fP are used by
the VGA drivers in the 2-bit and 4-bit X servers respectively.
\fBSVGA\fP is used by the XF86_SVGA X server.
.RE
.TP 8
.B Device \fI"device-id"\fP
specifies which graphics device description is to be used.
.TP 8
.B Monitor \fI"monitor-id"\fP
specifies which monitor description is to be used.
.TP 8
.B DefaultColorDepth \fIbpp-number\fP
specifies which color depth the server should use, when no -bpp command
line parameter was given.
.TP
.SH DISPLAY SUBSECTION
.B SubSection \fB"Display"\fP
This entry is a subsection which is used to specify some display
specific parameters.  This subsection is terminated by an
\fBEndSubSection\fP entry.  For some X servers and drivers (those
requiring a list of video modes) this subsection is mandatory.
For X servers which support multiple display depths, more than one
\fBDisplay\fP subsection may be present.  When multiple \fBDisplay\fP
subsections are present, each must have a unique \fBDepth\fP entry.
The entries available for the \fBDisplay\fP subsection are:
.RS 8
.TP 4
.B Depth \fIbpp\fP
This entry is mandatory when more than one \fBDisplay\fP subsection
is present in a \fBScreen\fP section.  When only one \fBDisplay\fP
subsection is present, it specifies the default depth that the X
server will run at.  When more than one \fBDisplay\fP subsection
is present, the depth determines which gets used by the X server.
The subsection used is the one matching the depth at which the X
server is run at.  Not all X servers (or drivers) support more than
one depth.  Permitted values for \fIbpp\fP are 8, 15, 16, 24 and 32.
Not all X servers (or drivers) support all of these values.
\fIbpp\fP values of 24 and 32 are treated equivalently by those X
servers which support them.
.TP 4
.B Weight \fIRGB\fP
This optional entry specifies the relative RGB weighting to be used
for an X server running at 16bpp.  This may also be specified from
the command line (see \fIXFree86(1)\fP).  Values supported by most
16bpp X servers are \fB555\fP and \fB565\fP.  For further details,
refer to the appropriate X server manual page.
.TP 4
.B Virtual \fIxdim ydim\fP
This optional entry specifies the virtual screen resolution to be
used.  \fIxdim\fP must be a multiple of either 8 or 16 for most
colour X servers, and a multiple of 32 for the monochrome X server.
The given value will be rounded down if this is not the case.  For
most X servers, video modes which are too large for the specified
virtual size will be rejected.  If this entry is not present, the
virtual screen resolution will be set to accommodate all the valid
video modes given in the \fBModes\fP entry.  Some X servers do not
support this entry.  Refer to the appropriate X server manual pages
for details.
.TP 4
.B ViewPort \fIx0 y0\fP
This optional entry sets the upper left corner of the initial
display.  This is only relevant when the virtual screen resolution
is different from the resolution of the initial video mode.  If
this entry is not given, then the initial display will be centered
in the virtual display area.
.TP 4
.B Modes \fI"modename" ...\fP
This entry is mandatory for most X servers, and it specifies the
list of video modes to use.  The video mode names must correspond
to those specified in the appropriate \fBMonitor\fP section.  Most
X servers will delete modes from this list which don't satisfy
various requirements.  The first valid mode in this list will be
the default display mode for startup.  The list of valid modes is
converted internally into a circular list.  It is possible to switch
to the next mode with \fBCtrl+Alt+Keypad-Plus\fP and to the previous
mode with \fBCtrl+Alt+Keypad-Minus\fP.
.TP 4
.B InvertVCLK \fI"modename"\fP \fR0|1\fP
This optional entry is specific to the S3 server only.  It may be used
to change the default VCLK invert/non-invert state for individual modes.
If \fI"modename"\fP is "\(**" the setting applies to all modes unless
unless overridden by later entries.
.TP 4
.B EarlySC \fI"modename"\fP \fR0|1\fP
This optional entry is specific to the S3 server only.  It may be used
to change the default EarlySC setting for individual modes.  This
setting can affect screen wrapping.
If \fI"modename"\fP is "\(**" the setting applies to all modes unless 
unless overridden by later entries.
.TP 4
.B BlankDelay \fI"modename" value1 value2\fP
This optional entry is specific to the S3 server only.  It may be used
to change the default blank delay settings for individual modes.  This
can affect screen wrapping.  \fIvalue1\fP and \fIvalue2\fP must be
integers in the range 0\-7.
If \fI"modename"\fP is "\(**" the setting applies to all modes unless
unless overridden by later entries.
.TP 4
.B Visual \fI"visual-name"\fP
This optional entry sets the default root visual type.  This may
also be specified from the command line (see \fIXserver(1)\fP).
The visual types available for 8bpp X servers are (default is
\fBPseudoColor\fP):
.RE
.sp
.in 20
.nf
.B StaticGray
.B GrayScale
.B StaticColor
.B PseudoColor
.B TrueColor
.B DirectColor
.fi
.in -20
.RS 12
.PP
The visual type available for the 16bpp and 32bpp X servers is 
\fBTrueColor\fP.
.PP
The visual type available for the 1bpp X server is \fBStaticGray\fP.
.PP
The visual types available for the 4bpp X server are (default is
\fBStaticColor\fP):
.RE
.sp
.in 20
.nf
.B StaticGray
.B GrayScale
.B StaticColor
.B PseudoColor
.fi
.in -20
.RS 8
.TP 4
.B Option \fI"optionstring"\fP
This optional entry allows the user to select certain options
provided by the drivers.  Multiple \fBOption\fP entries may be
given.  The supported values for \fIoptionstring\fP  are given in
the appropriate X server manual pages.
.TP 4
.B Black \fIred green blue\fP
This optional entry allows the ``black'' colour to be specified.  This
is only supported with the VGA2 driver in the XF86_Mono server (for
details see \fIXF86_Mono(1)\fP).
.TP 4
.B White \fIred green blue\fP
This optional entry allows the ``white'' colour to be specified.  This
is only supported with the VGA2 driver in the XF86_Mono server (for
details see \fIXF86_Mono(1)\fP).
.RE
.SH SERVERLAYOUT SECTION
  ...
.SH DRI SECTION
 ...
.SH VENDOR SECTION
..
.PP
.SH FILES
For an example of an XF86Config file, see the file installed as
<XRoot>/lib/X11/XF86Config.eg.
.PP
.nf
/etc/XF86Config
<XRoot>/lib/X11/XF86Config.\fIhostname\fP
<XRoot>/lib/X11/XF86Config
.sp 1
Note: <XRoot> refers to the root of the X11 install tree.
.fi
.SH "SEE ALSO"
X(1), Xserver(1), XFree86(1), XF86_SVGA(1), XF86_VGA16(1),
XF86_Mono(1), XF86_S3(1), XF86_8514(1), XF86_Mach8(1), XF86_Mach32(1),
XF86_P9000(1), XF86_AGX(1).
.SH AUTHORS
.PP
Refer to the
.I XFree86(1)
manual page.
.\" $TOG: XF86Conf.man /main/28 1997/07/19 09:22:00 kaleb $

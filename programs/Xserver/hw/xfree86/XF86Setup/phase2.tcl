# $XFree86: xc/programs/Xserver/hw/xfree86/XF86Setup/phase2.tcl,v 3.3 1996/08/13 11:28:30 dawes Exp $
#
# Phase II - Commands run after connection is made to VGA16 server
#

set_resource_defaults

wm withdraw .

create_main_window [set w .xf86setup]

# put up a message ASAP so the user knows we're still alive
label $w.waitmsg -text "Loading  -  Please wait...\n\n\n"
pack  $w.waitmsg -expand yes -fill both
update idletasks

set XKBrules $Xwinhome/lib/X11/xkb/rules/xfree86

if { [catch {set XKBhandle [xkb_read from_server]} res] } {
	$w.waitmsg configure -text \
	    "Unable to read keyboard information from the server.\n\n\
	    This problem most often occurs when you are running when\n\
	    you are running a server which does not have the XKEYBOARD\n\
	    extension or which has it disabled.\n\n\
	    The ability of this program to configure the keyboard is\n\
	    reduced without the XKEYBOARD extension, but is still\
	    functional.\n\n\
	    Continuing..."
	update idletasks
	after 10000
	set XKBinserver 0
} else {
	set XKBinserver 1
}

set retval [xkb_listrules $XKBrules]

if { [llength $retval] < 4 } {
	set XKBComponents(models,names)		 "pc101 pc102 microsoft"
	set XKBComponents(models,descriptions)	 [list \
		"Generic 101key PC" "Generic 102key PC" \
		"Microsoft Natural"]
	set XKBComponents(layouts,names)	 "us de it gb"
	set XKBComponents(layouts,descriptions)	 [list \
		"U.S. English" "German" "Italian" "U.K."]
	set XKBComponents(variants,names)	 ""
	set XKBComponents(variants,descriptions) ""
	set XKBComponents(options,names)	 ""
	set XKBComponents(options,descriptions)	 ""
} else {
	set XKBComponents(models,names)		 [lindex $retval 0]
	set XKBComponents(models,descriptions)	 [lindex $retval 1]
	set XKBComponents(layouts,names)	 [lindex $retval 2]
	set XKBComponents(layouts,descriptions)	 [lindex $retval 3]
	set XKBComponents(variants,names)	 [lindex $retval 4]
	set XKBComponents(variants,descriptions) [lindex $retval 5]
	set XKBComponents(options,names)	 [lindex $retval 6]
	set XKBComponents(options,descriptions)	 [lindex $retval 7]
}

# Setup the default bindings for the various widgets
source $tcl_library/tk.tcl

source $XF86Setup_library/mouse.tcl
source $XF86Setup_library/keyboard.tcl
source $XF86Setup_library/card.tcl
source $XF86Setup_library/monitor.tcl
source $XF86Setup_library/srvflags.tcl
source $XF86Setup_library/done.tcl

proc Intro_create_widgets { win } {
	global XF86Setup_library

	set w [winpathprefix $win]
	frame $w.intro -width 640 -height 420 \
		-relief ridge -borderwidth 5
	image create bitmap XFree86-logo \
		-foreground black -background cyan \
		-file $XF86Setup_library/pics/XFree86.xbm \
		-maskfile $XF86Setup_library/pics/XFree86.msk
	label $w.intro.logo -image XFree86-logo
	pack  $w.intro.logo

	text $w.intro.text
	$w.intro.text tag configure heading \
		-justify center -foreground yellow \
		-font -adobe-times-bold-i-normal--25-180-*-*-p-*-iso8859-1
	$w.intro.text insert end "Introduction to Configuration\
					with XF86Setup" heading
	$w.intro.text insert end "\n\n\
		There are five areas of configuration that need to\
			be completed, corresponding to the buttons\n\
		along the top:\n\n\
		\tMouse\t\t- Use this to set the protocol, baud rate, etc.\
			used by your mouse\n\
		\tKeyboard\t- Set the nationality and layout of\
			your keyboard\n\
		\tCard\t\t- Used to select the chipset, RAMDAC, etc.\
			of your card\n\
		\tMonitor\t\t- Use this to enter your\
			monitor's capabilities\n\
		\tOther\t\t- Configure some miscellaneous settings\n\n\
		When you've finished configuring all of these, press the\
			done button.\n\
		You can also press ? or click on the Help button at\
			any time for additional instructions\n\n\
		You probably will want to start with configuring your\
			mouse -- Just press \[Enter\] to start\n\n"
	pack $w.intro.text -fill both -expand yes -padx 10 -pady 10
	$w.intro.text configure -state disabled
}

proc Intro_activate { win } {
	set w [winpathprefix $win]
	pack $w.intro -side top -fill both -expand yes
}

proc Intro_deactivate { win } {
	set w [winpathprefix $win]
	pack forget $w.intro
}

proc Intro_popup_help { win } {
	toplevel .introhelp
	wm title .introhelp "Help"
	wm geometry .introhelp +30+30
	text   .introhelp.text
	.introhelp.text insert 0.0 "Do you really need help with this?"
	button .introhelp.ok -text "Okay" -command "destroy .introhelp"
	focus  .introhelp.ok
	pack   .introhelp.text .introhelp.ok
}

proc config_select { win } {
	global CfgSelection prevSelection

	set w [winpathprefix $win]
	$win configure -cursor watch
	${prevSelection}_deactivate $win
	set prevSelection $CfgSelection
	${CfgSelection}_activate $w
	$win configure -cursor top_left_arrow
}

proc config_help { win } {
	global CfgSelection

	set w [winpathprefix $win]
	${CfgSelection}_popup_help $win
}

frame $w.menu -width 640

radiobutton $w.menu.mouse -text Mouse -indicatoron false \
	-variable CfgSelection -value Mouse -underline 0 \
	-command [list config_select $w]
radiobutton $w.menu.keyboard -text Keyboard -indicatoron false \
	-variable CfgSelection -value Keyboard -underline 0 \
	-command [list config_select $w]
radiobutton $w.menu.card -text Card -indicatoron false \
	-variable CfgSelection -value Card -underline 0 \
	-command [list config_select $w]
radiobutton $w.menu.monitor -text Monitor -indicatoron false \
	-variable CfgSelection -value Monitor -underline 2 \
	-command [list config_select $w]
radiobutton $w.menu.other -text Other -indicatoron false \
	-variable CfgSelection -value Other -underline 0 \
	-command [list config_select $w]
pack $w.menu.mouse $w.menu.keyboard $w.menu.card $w.menu.monitor \
	$w.menu.other -side left -fill both -expand yes

frame $w.buttons
#label $w.buttons.xlogo -bitmap @/usr/X11R6/include/X11/bitmaps/xlogo16 -anchor w
#label $w.buttons.xlogo -bitmap @/usr/tmp/xfset1.xbm -anchor w \
	-foreground black
#pack $w.buttons.xlogo -side left -anchor w -expand no -padx 0 -fill x
button $w.buttons.abort -text Abort -underline 0 \
	-command "puts stderr Aborted;shutdown 1"
button $w.buttons.done  -text Done  -underline 0 \
	-command [list Done_execute $w]
button $w.buttons.help  -text Help  -underline 0 \
	-command [list config_help $w]
pack   $w.buttons.abort $w.buttons.done $w.buttons.help \
	-expand no -side left -padx 50

Intro_create_widgets	$w
Keyboard_create_widgets	$w
Mouse_create_widgets	$w
Card_create_widgets	$w
Monitor_create_widgets	$w
Other_create_widgets	$w
Done_create_widgets	$w

#bind $w <Alt-x>		[list I'm an error]
bind $w <Alt-a>		[list $w.buttons.abort invoke]
bind $w <Control-c>	[list $w.buttons.abort invoke]
bind $w <Alt-d>		[list $w.buttons.done invoke]
bind $w <Alt-h>		[list config_help $w]
bind $w ?		[list config_help $w]
bind $w <Alt-m>		[list $w.menu.mouse invoke]
bind $w <Alt-c>		[list $w.menu.card invoke]
bind $w <Alt-k>		[list $w.menu.keyboard invoke]
bind $w <Alt-n>		[list $w.menu.monitor invoke]
bind $w <Alt-o>		[list $w.menu.other invoke]

set CfgSelection  Intro
set prevSelection Intro
destroy $w.waitmsg
pack $w.menu -side top -fill x
pack $w.buttons -side bottom -expand no
config_select $w
focus $w.menu.mouse


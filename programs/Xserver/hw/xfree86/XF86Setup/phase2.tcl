
#
#

rename exit _std_exit
proc exit {args} {
	global ServerPID1 ServerPID2

	catch {destroy .}
	catch {exec kill $ServerPID1 >& /dev/null}
	catch {exec kill $ServerPID2 >& /dev/null}
	eval _std_exit $args
}

set curdir [pwd]
cd $Xwinhome/lib/X11/xkb
#exec xkbcomp keymap/sun/us compiled/xfsetup.xkm
#xkb_read compiled/xfsetup.xkm
if { [catch {set compspec [xkb_read from_server]} res] } {
	puts stderr "XKBread failed"
	exit 1
}
set comps [xkb_list "" "" "" "" * *]
set XKBComponents(symbols)  [lindex $comps 4]
set XKBComponents(geometry) [lindex $comps 5]
#set xx [xkb_load "" "" "" "" "" microsoft]
cd $curdir

# Colors chosen to work with the VGA16 server
option add *background			grey
option add *foreground			blue
option add *selectColor			cyan
option add *disabledForeground		""		;# stippled
option add *highlightBackground		grey
option add *font			fixed
option add *Frame.highlightThickness	0
option add *Listbox.exportSelection	no

# Setup the default bindings for the various widgets
source $tcl_library/tk.tcl

source $XF86Setup_library/mouse.tcl
source $XF86Setup_library/keyboard.tcl
source $XF86Setup_library/card.tcl
source $XF86Setup_library/monitor.tcl
source $XF86Setup_library/done.tcl

proc Intro_create_widgets { w } {
	global XF86Setup_library

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
	$w.intro.text insert end "Introduction to Configuration with XF86Setup" heading
	$w.intro.text insert end "\n\n\
		There are four areas of configuration that need to\
			be completed, corresponding to the buttons\n\
		along the top:\n\n\
		\tMouse\t\t- Use this to set the protocol, baud rate, etc.\
			used by your mouse\n\
		\tKeyboard\t- Set the nationality and layout of\
			your keyboard\n\
		\tCard\t\t- Used to select the chipset, RAMDAC, etc.\
			of your card\n\
		\tMonitor\t\t- Use this to enter your monitor's capabilities\n\
		When you've finished configuring all of these, press the\
			done button.\n\
		You can also press ? or click on the Help button at\
			any time for additional instructions\n\n\
		You probably will want to start with configuring your\
			mouse -- Just press \[Enter\] to start\n\n"
	pack $w.intro.text -fill both -expand yes -padx 10 -pady 10
	$w.intro.text configure -state disabled
}

proc Intro_activate { w } {
	pack $w.intro -side top -fill both -expand yes
}

proc Intro_deactivate { w } {
	pack forget $w.intro
}

proc Intro_popup_help { w } {
	toplevel .introhelp
	wm title .introhelp "Help"
	wm geometry .introhelp +30+30
	text .introhelp.text
	.introhelp.text insert 0.0 "Do you really need help with this?"
	button .introhelp.ok -text "Okay" -command "destroy .introhelp"
	focus .introhelp.ok
	pack .introhelp.text .introhelp.ok
}

proc config_select { w } {
	global CfgSelection prevSelection

	$w configure -cursor watch
	${prevSelection}_deactivate $w
	set prevSelection $CfgSelection
	${CfgSelection}_activate $w
	$w configure -cursor top_left_arrow
}

proc config_help { w } {
	global CfgSelection

	${CfgSelection}_popup_help $w
}

wm withdraw .

set w .xf86setup
toplevel $w
$w configure -height 480 -width 640 -highlightthickness 0
pack propagate $w no
wm geometry $w +0+0
#wm minsize $w 640 480

frame $w.menu -width 640
pack $w.menu -side top -fill x


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
pack $w.menu.mouse $w.menu.keyboard $w.menu.card $w.menu.monitor \
	-side left -fill both -expand yes

frame $w.buttons
pack $w.buttons -side bottom -expand no
#label $w.buttons.xlogo -bitmap @/usr/X11R6/include/X11/bitmaps/xlogo16 -anchor w
#label $w.buttons.xlogo -bitmap @/usr/tmp/xfset1.xbm -anchor w \
	-foreground black
#pack $w.buttons.xlogo -side left -anchor w -expand no -padx 0 -fill x
button $w.buttons.abort -text Abort -underline 0 -command "exit 1"
button $w.buttons.done  -text Done  -underline 0 \
	-command [list Done_execute $w]
button $w.buttons.help  -text Help  -underline 0 \
	-command [list config_help $w]
pack $w.buttons.abort $w.buttons.done $w.buttons.help \
	-expand no -side left -padx 50

Intro_create_widgets	$w
Keyboard_create_widgets	$w
Mouse_create_widgets	$w
Card_create_widgets	$w
Monitor_create_widgets	$w
#Other_create_widgets	$w
Done_create_widgets	$w

set CfgSelection Intro
set prevSelection Intro
config_select $w
focus $w.menu.mouse
bind $w <Alt-a>		[list $w.buttons.abort invoke]
bind $w <Control-c>	[list $w.buttons.abort invoke]
bind $w <Alt-d>		[list $w.buttons.done invoke]
bind $w <Alt-h>		[list config_help $w]
bind $w ?		[list config_help $w]
bind $w <Alt-m>		[list $w.menu.mouse invoke]
bind $w <Alt-c>		[list $w.menu.card invoke]
bind $w <Alt-k>		[list $w.menu.keyboard invoke]
bind $w <Alt-n>		[list $w.menu.monitor invoke]

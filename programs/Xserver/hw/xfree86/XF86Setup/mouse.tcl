# $XFree86$

#
#

set mseTypeList { Microsoft MouseSystems MMSeries Logitech BusMouse
		MouseMan PS/2 MMHitTab GlidePoint Xqueue OSMouse }

proc Mouse_proto_select { w } {
	global mseType baudRate chordMiddle clearDTR clearRTS sampleRate
	global Pointer_realdevice

	if {[lsearch -exact {BusMouse Xqueue OSMouse PS/2} $mseType] == -1} {
		foreach rate {1200 2400 4800 9600} {
			$w.mouse.brate.$rate configure -state normal
		}
		if { ![string compare MouseMan $mseType] } {
			$w.mouse.brate.2400 configure -state disabled
			$w.mouse.brate.4800 configure -state disabled
			if { $baudRate == 2400 || $baudRate == 4800 } {
				set baudRate 1200
			}
		}
	} else {
		foreach rate {1200 2400 4800 9600} {
			$w.mouse.brate.$rate configure -state disabled
		}
	}
	if { ![string compare MMHitTab $mseType] } {
		$w.mouse.srate.title configure -text "Lines/inch"
		$w.mouse.srate.scale configure -to 1200 -tickinterval 200 \
			-resolution 20
		$w.mouse.srate.scale configure -state normal
	} else {
		$w.mouse.srate.title configure -text "Sample Rate"
		$w.mouse.srate.scale configure -to 150 -tickinterval 25 \
			-resolution 1
		if {[lsearch -exact \
				{MouseMan BusMouse Xqueue OSMouse PS/2} \
				$mseType] == -1} {
			$w.mouse.srate.scale configure -state normal
		} else {
			$w.mouse.srate.scale configure -state disabled
			set sampleRate 0
		}
	}
	if { ![string compare MouseSystems $mseType] } {
		$w.mouse.flags.dtr configure -state normal
		$w.mouse.flags.rts configure -state normal
	} else {
		$w.mouse.flags.dtr configure -state disabled
		$w.mouse.flags.rts configure -state disabled
		set clearDTR [set clearRTS 0]
	}
	if {[lsearch -exact {Microsoft MouseMan} $mseType] == -1} {
		$w.mouse.chdmid configure -state disabled
		set chordMiddle 0
	} else {
		$w.mouse.chdmid configure -state normal
	}
	if { ![info exists Pointer_realdevice] } {
		$w.mouse.device.entry delete 0 end
		$w.mouse.device.entry insert 0 \
			[Mouse_defaultdevice $mseType]
	}
}

proc Mouse_create_widgets { w } {
	global mseType mseDev baudRate sampleRate mseTypeList clearDTR
	global emulate3Buttons emulate3Timeout chordMiddle clearRTS

	frame $w.mouse -width 640 -height 420 \
		-relief ridge -borderwidth 5
	frame $w.mouse.top
	frame $w.mouse.mid -relief sunken -borderwidth 3
	frame $w.mouse.bot
	pack $w.mouse.top -side top
	pack $w.mouse.mid -side top -fill x -expand yes
	pack $w.mouse.bot -side top

	label $w.mouse.top.title -text "Select the mouse protocol"
	frame $w.mouse.type
	pack $w.mouse.top.title $w.mouse.type -in $w.mouse.top -side top
	foreach Type $mseTypeList {
		set type [string tolower $Type]
		radiobutton $w.mouse.type.$type -text $Type \
			-indicatoron false \
			-variable mseType -value $Type \
			-command [list Mouse_proto_select $w]
		pack $w.mouse.type.$type -side left -anchor n
	}

	frame $w.mouse.mid.left
	pack $w.mouse.mid.left -side left -fill x -fill y
	frame $w.mouse.device
	pack $w.mouse.device -in $w.mouse.mid.left -side top \
		-pady 3m -padx 3m
	label $w.mouse.device.title -text "Mouse Device"
	entry $w.mouse.device.entry
	pack $w.mouse.device.title $w.mouse.device.entry -side top -fill x
	bind $w.mouse.device.entry <Return> "focus [list $w].mouse.em3but; \
		set Pointer_realdevice \[[list $w].mouse.device.entry get\]"

	frame $w.mouse.mid.left.buttons
	pack $w.mouse.mid.left.buttons -in $w.mouse.mid.left \
		-side top -fill x -pady 3m
	checkbutton $w.mouse.em3but -text Emulate3Buttons \
		-indicatoron no -variable emulate3Buttons \
		-command [list Mouse_set_em3but $w]
	checkbutton $w.mouse.chdmid -text ChordMiddle \
		-indicatoron no -variable chordMiddle \
		-command [list Mouse_set_chdmid $w]
	pack $w.mouse.em3but $w.mouse.chdmid -in $w.mouse.mid.left.buttons \
		-side top -fill x -padx 3m -anchor w

	frame $w.mouse.brate
	pack $w.mouse.brate -in $w.mouse.mid.left -side top -pady 3m
	label $w.mouse.brate.title -text "Baud Rate"
	pack $w.mouse.brate.title -side top
	foreach rate { 1200 2400 4800 9600 } {
		radiobutton $w.mouse.brate.$rate -text $rate \
			-variable baudRate -value $rate
		pack $w.mouse.brate.$rate -side top -anchor w
	}

	frame $w.mouse.flags
	pack $w.mouse.flags -in $w.mouse.mid.left -side top \
		-fill x -pady 3m
	label $w.mouse.flags.title -text Flags
	checkbutton $w.mouse.flags.dtr -text ClearDTR \
		-indicatoron no -variable clearDTR
	checkbutton $w.mouse.flags.rts -text ClearRTS \
		-indicatoron no -variable clearRTS
	pack $w.mouse.flags.title $w.mouse.flags.dtr $w.mouse.flags.rts \
		-side top -fill x -padx 3m -anchor w

	frame $w.mouse.srate
	pack $w.mouse.srate -in $w.mouse.mid -side left -fill y -expand yes
	label $w.mouse.srate.title -text "Sample Rate"
	scale $w.mouse.srate.scale -orient vertical -from 0 -to 150 \
		-tickinterval 25 -variable sampleRate -state disabled
	pack $w.mouse.srate.title -side top
	pack $w.mouse.srate.scale -side top -fill y -expand yes

	frame $w.mouse.em3tm
	pack $w.mouse.em3tm -in $w.mouse.mid -side left -fill y -expand yes
	label $w.mouse.em3tm.title -text "Emulate3Timeout"
	scale $w.mouse.em3tm.scale -orient vertical -from 0 -to 1000 \
		-tickinterval 250 -variable emulate3Timeout -resolution 5
	pack $w.mouse.em3tm.title -side top
	pack $w.mouse.em3tm.scale -side top -fill y -expand yes

	frame $w.mouse.mid.right
	pack $w.mouse.mid.right -side left
	set canv $w.mouse.mid.right.canvas
	canvas $canv -width 2.75i -height 4i -highlightthickness 0 \
			-takefocus 0
	$canv create rectangle 0.25i 1.25i 2.50i 3.75i -fill white \
			-tag {mbut mbut4}
	$canv create rectangle 0.25i 0.25i 1.00i 1.25i -fill white \
			-tag {mbut mbut1}
	$canv create rectangle 1.00i 0.25i 1.75i 1.25i -fill white \
			-tag {mbut mbut2}
	$canv create rectangle 1.75i 0.25i 2.50i 1.25i -fill white \
			-tag {mbut mbut3}
	$canv create text 1.375i 2.25i -tag coord

	button $w.mouse.mid.right.apply -text "Apply" \
		-command [list Mouse_setsettings $w]
	pack $canv $w.mouse.mid.right.apply -side top

	label $w.mouse.bot.wait -text "Applying changes..." -foreground grey
	label $w.mouse.bot.keylist -text "Press ? or Alt-H for a list of key bindings"
	pack $w.mouse.bot.wait $w.mouse.bot.keylist

	Mouse_getsettings $w
}

proc Mouse_activate { w } {
	pack $w.mouse -side top -fill both -expand yes

	set canv $w.mouse.mid.right.canvas
	bind $w <ButtonPress>	  [list $canv itemconfigure mbut%b -fill black]
	bind $w <ButtonRelease>	  [list $canv itemconfigure mbut%b -fill white]
	bind $w <ButtonPress-4>	  [list $canv itemconfigure mbut4 -fill black;
				   $canv itemconfigure coord -fill white]
	bind $w <ButtonRelease-4> [list $canv itemconfigure mbut4 -fill white;
				   $canv itemconfigure coord -fill black]
	bind $w <Motion>	  [list $canv itemconfigure coord -text (%X,%Y)]

	$canv itemconfigure mbut  -fill white
	$canv itemconfigure coord -fill black

	set ifcmd {if { [string compare [focus] %s.mouse.device.entry]
			!= 0 } { %s %s } }
			
	bind $w a [format $ifcmd $w Mouse_setsettings [list $w] ]
	bind $w b [format $ifcmd $w Mouse_nextbaudrate [list $w] ]
	bind $w c [format $ifcmd $w [list $w.mouse.chdmid] invoke ]
	bind $w d [format $ifcmd $w [list $w.mouse.flags.dtr] invoke ]
	bind $w e [format $ifcmd $w [list $w.mouse.em3but] invoke ]
	bind $w n [format $ifcmd $w Mouse_selectentry [list $w] ]
	bind $w p [format $ifcmd $w Mouse_nextprotocol [list $w] ]
	bind $w r [format $ifcmd $w [list $w.mouse.flags.rts] invoke ]
	bind $w s [format $ifcmd $w Mouse_incrsamplerate [list $w] ]
	bind $w t [format $ifcmd $w Mouse_increm3timeout [list $w] ]
}

proc Mouse_deactivate { w } {
	pack forget $w.mouse

	bind $w <ButtonPress>	  ""
	bind $w <ButtonRelease>	  ""
	bind $w <ButtonPress-4>	  ""
	bind $w <ButtonRelease-4> ""
	bind $w <Motion>	  ""

	bind $w a		  ""
	bind $w b		  ""
	bind $w c		  ""
	bind $w d		  ""
	bind $w e		  ""
	bind $w n		  ""
	bind $w p		  ""
	bind $w r		  ""
	bind $w s		  ""
	bind $w t		  ""
}

proc Mouse_popup_help { w } {
        toplevel .mousehelp
        wm title .mousehelp "Help"
	wm geometry .mousehelp +30+30
        text .mousehelp.text -takefocus 0 -width 90 -height 27
        .mousehelp.text insert end \
{   First select the protocol for your mouse, then if needed, change the device name.  If
 applicable, also set the baud rate (1200 should work).  Press 'a' to apply the changes
 and try moving your mouse around.  If the mouse pointer does not move properly, try a
 different protocol or device name.

   Once the mouse is moving properly, test that the various buttons also work correctly.
 If you have a three button mouse and the middle button does not work, try the buttons
 labeled ChordMiddle and Emulate3Buttons.

       Key    Function
     ------------------------------------------------------
        a  -  Apply changes
        b  -  Change to next baud rate
        c  -  Toggle the ChordMiddle button
        d  -  Toggle the ClearDTR button
        e  -  Toggle the Emulate3button button
        n  -  Set the name of the device
        p  -  Select the next protocol
        r  -  Toggle the ClearRTS button
        s  -  Increase the sample rate
        t  -  Increase the 3-button emulation timeout
     ------------------------------------------------------
 You can also use Tab, and Ctrl-Tab to move around and then use Enter to activate
 the selected button.
 
 See the documentation for more information
}

        button .mousehelp.ok -text "Okay" -command "destroy .mousehelp"
	focus .mousehelp.ok
	.mousehelp.text configure -state disabled
        pack .mousehelp.text .mousehelp.ok
}

proc Mouse_selectentry { w } {
	if { [ $w.mouse.device.entry cget -state] != "disabled" } {
		focus $w.mouse.device.entry
	}
}

proc Mouse_nextprotocol { w } {
	global mseType mseTypeList

	set idx [lsearch -exact $mseTypeList $mseType]
	do {
		incr idx
		if { $idx >= [llength $mseTypeList] } {
			set idx 0
		}
		set mseType [lindex $mseTypeList $idx]
		set mtype [string tolower $mseType]
	} while { [$w.mouse.type.$mtype cget -state] == "disabled" }
	Mouse_proto_select $w
}

proc Mouse_nextbaudrate { w } {
	global baudRate

	if { [$w.mouse.brate.$baudRate cget -state] == "disabled" } {
		return
	}
	do {
		set baudRate [expr $baudRate*2]
		if { $baudRate > 9600 } {
			set baudRate 1200
		}
	} while { [$w.mouse.brate.$baudRate cget -state] == "disabled" }
}

proc Mouse_incrsamplerate { w } {
	global sampleRate

	if { [$w.mouse.srate.scale cget -state] == "disabled" } {
		return
	}

	set max [$w.mouse.srate.scale cget -to]
	set interval [expr [$w.mouse.srate.scale cget -tickinterval]/2.0]
	if { $sampleRate+$interval > $max } {
		set sampleRate 0
	} else {
		set sampleRate [expr $sampleRate+$interval]
	}
}

proc Mouse_increm3timeout { w } {
	global emulate3Timeout

	if { [$w.mouse.em3tm.scale cget -state] == "disabled" } {
		return
	}
	set max [$w.mouse.em3tm.scale cget -to]
	set interval [expr [$w.mouse.em3tm.scale cget -tickinterval]/2.0]
	if { $emulate3Timeout+$interval > $max } {
		set emulate3Timeout 0
	} else {
		set emulate3Timeout [expr $emulate3Timeout+$interval]
	}
}

proc Mouse_set_em3but { w } {
	global emulate3Buttons chordMiddle

	if { $emulate3Buttons } {
		$w.mouse.em3tm.scale configure -state normal
	} else {
		$w.mouse.em3tm.scale configure -state disabled
	}
	if { $chordMiddle && $emulate3Buttons } {
		$w.mouse.chdmid invoke
	}
}

proc Mouse_set_chdmid { w } {
	global emulate3Buttons chordMiddle

	if { $chordMiddle && $emulate3Buttons } {
		$w.mouse.em3but invoke
	}
}

proc Mouse_setsettings { w } {
	global mseType mseDev baudRate sampleRate clearDTR
	global emulate3Buttons emulate3Timeout chordMiddle clearRTS
	global Pointer Pointer_realdevice

	#grab set $w.mouse.bot.wait
	$w.mouse.bot.wait configure -foreground black
	$w configure -cursor watch
	update idletasks
	set mseDev [$w.mouse.device.entry get]
	set em3but off
	set chdmid off
	if $emulate3Buttons {set em3but on}
	if $chordMiddle {set chdmid on}
	set flags ""
	if $clearDTR {lappend flags ClearDTR}
	if $clearRTS {lappend flags ClearRTS}
	if { "$mseType" == "MouseSystems" } {lappend flags ReOpen}
	if {[string length $Pointer(Device)] && [string length $mseDev]} {
		set Pointer_realdevice $mseDev
		unlink $Pointer(Device)
		link $Pointer_realdevice $Pointer(Device)
	}
	set result [catch { eval [list xf86misc_setmouse \
		$mseDev $mseType $baudRate $sampleRate \
		$em3but $emulate3Timeout $chdmid] $flags } ]
	if { $result } {
		bell -displayof $w
	} else {
		set Pointer(Protocol) $mseType
		if { [$w.mouse.brate.1200 cget -state] == "disabled" } {
			set Pointer(BaudRate) ""
		} else {
			set Pointer(BaudRate) $baudRate
		}
		if { [$w.mouse.srate.scale cget -state] == "disabled" } {
			set Pointer(SampleRate) ""
		} else {
			set Pointer(SampleRate) $sampleRate
		}
		set Pointer(Emulate3Buttons) [expr $emulate3Buttons?"ON":""]
		set Pointer(Emulate3Timeout) \
			[expr $emulate3Buttons?$emulate3Timeout:""]
		set Pointer(ChordMiddle) [expr $chordMiddle?"ON":""]
		set Pointer(ClearDTR) [expr $clearDTR?"ON":""]
		set Pointer(ClearRTS) [expr $clearRTS?"ON":""]
	}
	$w.mouse.bot.wait configure -foreground grey
	$w configure -cursor top_left_arrow
	#grab release $w.mouse.bot.wait
}

proc Mouse_getsettings { w } {
	global mseType mseTypeList mseDev baudRate sampleRate clearDTR
	global emulate3Buttons emulate3Timeout chordMiddle clearRTS
	global Pointer_realdevice

	set initlist	[xf86misc_getmouse]
	#set initlist	[list "" Xqueue 1200 0 on 50 off]
	#set initlist	[list /dev/tty00 MouseSystems 1200 120 on 50 off ClearRTS]
	set initdev	[lindex $initlist 0]
	set inittype	[lindex $initlist 1]
	set initbrate	[lindex $initlist 2]
	set initsrate	[lindex $initlist 3]
	set initem3btn	[lindex $initlist 4]
	set initem3tm	[lindex $initlist 5]
	set initchdmid	[lindex $initlist 6]
	set initflags	[lrange $initlist 7 end]
	#xf86misc_setmouse "" Xqueue 1200 0 on 50 off
	#xf86misc_setmouse "/dev/mouse" Microsoft 200 0 on 50 off

	if { [info exists Pointer_realdevice] } {
		$w.mouse.device.entry insert 0 $Pointer_realdevice
	} else {
		$w.mouse.device.entry insert 0 \
			[Mouse_defaultdevice $inittype]
	}
	$w.mouse.brate.$initbrate invoke

	set chordMiddle     [expr [string compare $initchdmid on] == 0]
	set emulate3Buttons [expr [string compare $initem3btn on] == 0]
	set emulate3Timeout $initem3tm
	set sampleRate      $initsrate
	set clearDTR  [expr [string first $initflags ClearDTR] >= 0]
	set clearRTS  [expr [string first $initflags ClearRTS] >= 0]

	set mtype [string tolower $inittype]
	if { $mtype == "osmouse" || $mtype == "xqueue" } {
		foreach mse $mseTypeList {
			$w.mouse.type.[string tolower $mse] \
				configure -state disabled
		}
		$w.mouse.type.$mtype  configure -state normal
		$w.mouse.device.entry configure -state disabled
	} else {
		$w.mouse.type.osmouse configure -state disabled
		$w.mouse.type.xqueue  configure -state disabled
	}
	$w.mouse.type.$mtype invoke
}

proc Mouse_defaultdevice { mousetype } {
	switch $mousetype {
		PS/2		{ return /dev/ps2aux }
		BusMouse	{ return /dev/bmse }
		default		{ return /dev/tty00 }
	}
}


# $XFree86: xc/programs/Xserver/hw/xfree86/XF86Setup/card.tcl,v 3.2 1996/08/13 11:28:20 dawes Exp $
#
#

proc Card_create_widgets { win } {
	global ServerList XF86Setup_library cardDevNum DeviceIDs
	global cardProbe cardDetail cardReadmeWasSeen UseConfigFile

	set w [winpathprefix $win]
	set cardDevNum 0
	frame $w.card -width 640 -height 420 \
		-relief ridge -borderwidth 5
	frame $w.card.top
	pack  $w.card.top -side top -fill x -padx 5m
	if { [llength $DeviceIDs] > 1 } {
		label $w.card.title -text "Card selected:" -anchor w
		pack  $w.card.title -side left -fill x -padx 5m -in $w.card.top
		combobox $w.card.cardselect -state disabled -bd 2
		pack  $w.card.cardselect -side left -in $w.card.top
		eval [list $w.card.cardselect linsert end] $DeviceIDs
		Card_cbox_setentry $w.card.cardselect [lindex $DeviceIDs 0]
		bind $w.card.cardselect.popup.list <ButtonRelease-1> \
			"+Card_cardselect $win"
	} else {
		label $w.card.title -text "Card selected: None" -anchor w
		pack  $w.card.title -side left -fill x -padx 5m -in $w.card.top
	}

	frame $w.card.list
	scrollbar $w.card.list.sb -command [list $w.card.list.lb yview] \
		-repeatdelay 1200 -repeatinterval 800
	listbox   $w.card.list.lb -yscroll [list $w.card.list.sb set] \
		-setgrid true -height 20
	bind  $w.card.list.lb <ButtonRelease-1> \
		[list Card_selected $win $w.card.list.lb]
	eval  $w.card.list.lb insert 0 [xf86cards_getlist]
	pack  $w.card.list.lb -side left -fill both -expand yes
	pack  $w.card.list.sb -side left -fill y

	image create bitmap cardpic -foreground yellow -background black \
		-file $XF86Setup_library/pics/vidcard.xbm \
		-maskfile $XF86Setup_library/pics/vidcard.msk
	label $w.card.list.pic -image cardpic
	pack  $w.card.list.pic -side left -padx 3m -pady 3m


	frame $w.card.bot -borderwidth 5
	pack  $w.card.bot -side bottom -fill x
	label $w.card.bot.message
	pack  $w.card.bot.message -side top -fill x

	frame $w.card.buttons
	pack  $w.card.buttons -side bottom -fill x
	button $w.card.probe -text "Probe Clocks" \
		-command [list Card_clockprobe $win]
	$w.card.probe configure -state disabled
	pack  $w.card.probe  -side left  -expand yes \
		-in $w.card.buttons

	button $w.card.readme -text "Read README file" \
		-command [list Card_display_readme $win]
	pack  $w.card.readme -side left -expand yes \
		-in $w.card.buttons

	button $w.card.modebutton -text "Detailed Setup" \
		-command [list Card_switchdetail $win]
	pack  $w.card.modebutton -side left -expand yes \
		-in $w.card.buttons

	frame $w.card.detail

	frame $w.card.server
	pack  $w.card.server -side top -fill x -in $w.card.detail
	label $w.card.server.title -text Server:
	pack  $w.card.server.title -side left
	foreach serv $ServerList {
		set lcserv [string tolower $serv]
		radiobutton $w.card.server.$lcserv -indicatoron no \
			-text $serv -variable cardServer -value $serv \
			-command [list Card_set_cboxlists $win]
		pack $w.card.server.$lcserv -anchor w -side left \
			-expand yes -fill x
	}

	frame $w.card.detail.cboxen
	pack  $w.card.detail.cboxen -side top

	frame $w.card.chipset
	pack  $w.card.chipset -side left -expand yes -fill x \
		-in $w.card.detail.cboxen -padx 5m
	label $w.card.chipset.title -text Chipset
	combobox $w.card.chipset.cbox -state disabled -bd 2
	pack  $w.card.chipset.title $w.card.chipset.cbox

	frame $w.card.ramdac
	pack  $w.card.ramdac -side left -expand yes -fill x \
		-in $w.card.detail.cboxen -padx 5m
	label $w.card.ramdac.title -text RamDac
	combobox $w.card.ramdac.cbox -state disabled -bd 2
	pack  $w.card.ramdac.title $w.card.ramdac.cbox

	frame $w.card.clockchip
	pack  $w.card.clockchip -side left -expand yes -fill x \
		-in $w.card.detail.cboxen -padx 5m
	label $w.card.clockchip.title -text ClockChip
	combobox $w.card.clockchip.cbox -state disabled -bd 2
	pack  $w.card.clockchip.title $w.card.clockchip.cbox

	frame $w.card.clocks
	label $w.card.clocks.title -text "Clock rates:"
	text  $w.card.clocks.text -yscroll [list $w.card.clocks.sb set] \
		-setgrid true -height 4 -background cyan
	scrollbar $w.card.clocks.sb -command [list $w.card.clocks.text yview]
	pack  $w.card.clocks -side bottom -fill x -in $w.card.detail \
		-pady 3m
	pack  $w.card.clocks.title -fill x -expand yes -side top
	pack  $w.card.clocks.text -fill x -expand yes -side left
	pack  $w.card.clocks.sb -side left -fill y

	if { $UseConfigFile } {
		set extr $w.card.extra
		frame $extr
		pack  $extr -side bottom -padx 5m \
			-fill x -expand yes -in $w.card.detail
		frame $extr.dacspeed
		pack  $extr.dacspeed -side left -fill x -expand yes
		label $extr.dacspeed.title -text "RAMDAC Max Speed"
		checkbutton $extr.dacspeed.probe -width 15 -text "Probed" \
			-variable cardDacProbe -indicator off \
			-command [list Card_dacspeed $win] \
			-highlightthickness 0
		scale $extr.dacspeed.value -variable cardDacSpeed \
			-orient horizontal -from 60 -to 300 -resolution 5
		pack  $extr.dacspeed.title -side top -fill x -expand yes
		pack  $extr.dacspeed.probe -side top -expand yes
		pack  $extr.dacspeed.value -side top -fill x -expand yes
		frame $extr.videoram
		pack  $extr.videoram -side left -fill x -expand yes
		label $extr.videoram.title -text "Video RAM"
		pack  $extr.videoram.title -side top -fill x -expand yes
		radiobutton $extr.videoram.mprobed -indicator off -width 15 \
			-variable cardRamSize -value 0 -text "Probed" \
			-highlightthickness 0
		pack  $extr.videoram.mprobed -side top -expand yes
		frame $extr.videoram.cols
		pack  $extr.videoram.cols -side top -fill x -expand yes
		frame $extr.videoram.col1
		frame $extr.videoram.col2
		pack  $extr.videoram.col1 $extr.videoram.col2 \
			-side left -fill x -expand yes \
			-in $extr.videoram.cols
		radiobutton $extr.videoram.m256k \
			-variable cardRamSize -value 256 -text "256K" \
			-highlightthickness 0
		radiobutton $extr.videoram.m512k \
			-variable cardRamSize -value 512 -text "512K" \
			-highlightthickness 0
		radiobutton $extr.videoram.m1m \
			-variable cardRamSize -value 1024 -text "1Meg" \
			-highlightthickness 0
		radiobutton $extr.videoram.m2m \
			-variable cardRamSize -value 2048 -text "2Meg" \
			-highlightthickness 0
		radiobutton $extr.videoram.m3m \
			-variable cardRamSize -value 3072 -text "3Meg" \
			-highlightthickness 0
		radiobutton $extr.videoram.m4m \
			-variable cardRamSize -value 4096 -text "4Meg" \
			-highlightthickness 0
		radiobutton $extr.videoram.m6m \
			-variable cardRamSize -value 6144 -text "6Meg" \
			-highlightthickness 0
		radiobutton $extr.videoram.m8m \
			-variable cardRamSize -value 8192 -text "8Meg" \
			-highlightthickness 0
		pack  $extr.videoram.m256k $extr.videoram.m512k \
		      $extr.videoram.m1m $extr.videoram.m2m \
			-side top -fill x -expand yes \
			-in $extr.videoram.col1
		pack  $extr.videoram.m3m $extr.videoram.m4m \
		      $extr.videoram.m6m $extr.videoram.m8m \
			-side top -fill x -expand yes \
			-in $extr.videoram.col2
	}
	frame $w.card.options
	pack  $w.card.options -side bottom -fill x -in $w.card.detail \
		-pady 2m
	text  $w.card.options.text -yscroll [list $w.card.options.sb set] \
		-setgrid true -height 6 -background white
	scrollbar $w.card.options.sb -command \
		[list $w.card.options.text yview]
	combobox $w.card.options.cbox -state disabled -width 80 -bd 2
	if { !$UseConfigFile } {
		label $w.card.options.title -text "Additional lines to\
			add to Device section of the XF86Config file:"
		pack  $w.card.options.title -fill x -expand yes -side top
		pack  $w.card.options.text -fill x -expand yes -side left
		pack  $w.card.options.sb -side left -fill y
	} else {
		label $w.card.options.title -text "Selected options:"
		$w.card.options.cbox.popup.list configure \
			-selectmode multiple
		pack  $w.card.options.title -side left
		pack  $w.card.options.cbox -fill x -expand yes -side left
	}

	$w.card.readme configure -state disabled
	for {set idx 0} {$idx < [llength $DeviceIDs]} {incr idx} {
		set cardProbe($idx)		0
		set cardReadmeWasSeen($idx)	0
	}
	if { $UseConfigFile } {
		set cardDetail		std
		Card_switchdetail $win
		$w.card.modebutton configure -state disabled
	} else {
		set cardDetail		detail
		Card_switchdetail $win
	}
}

proc Card_activate { win } {
	set w [winpathprefix $win]
	Card_get_configvars $win
	pack $w.card -side top -fill both -expand yes
}

proc Card_deactivate { win } {
	set w [winpathprefix $win]
	pack forget $w.card
	Card_set_configvars $win
}

proc Card_dacspeed { win } {
	global cardDacSpeed cardDacProbe

	set w [winpathprefix $win]
	if { $cardDacProbe } {
		#$w.card.extra.dacspeed.probe configure -text "Probed: Yes"
		$w.card.extra.dacspeed.value configure \
			-foreground [option get $w.card background *] -state disabled
	} else {
		#$w.card.extra.dacspeed.probe configure -text "Probed: No"
		$w.card.extra.dacspeed.value configure \
			-foreground [option get $w.card foreground *] -state normal
	}
}

proc Card_clockprobe { win } {
	global cardServer Xwinhome Confname cardClocks cardDevNum
	global serverNumber

	set w [winpathprefix $win]
	writeXF86Config $Confname-probe -noclocks -defaultmodes

	set server $cardServer
	if { ![file exists $Xwinhome/bin/XF86_$server] } {
		$w.card.bot.message configure -text \
			"The server you selected is not installed on this system.\n "
		bell
		return
	}
	#catch {exec $Xwinhome/bin/XF86_$server :[incr serverNumber] \
		-probeonly -xf86config $Confname-probe >& $Confname-pout}
	catch {exec $Xwinhome/bin/XF86_$server :5 \
		-probeonly -xf86config $Confname-probe >& $Confname-pout}
	#set fd [open $Confname-pout w]
	#puts $fd "(--) S3: clocks:  80.0 56.2 0.00 124.2 76.5  35.1 31.5 48.0"
	#puts $fd "(--) S3: clocks:  45.0 66.3 70.00 28.2 76.5  90.1 37.5 40.0"
	#close $fd
	if { ![file exists $Confname-pout] || ![file size $Confname-pout] } {
		$w.card.bot.message configure -text \
			"Unable to run clock probe!\n "
		bell
		return
	}
	set fd [open $Confname-pout r]
	set cardClocks ""
	set zerocount 0
	while {[gets $fd line] >= 0} {
		if {[regexp {\(.*: clocks: (.*)$} $line dummy clocks]} {
			set clocks [string trim [squash_white $clocks]]
			foreach clock [split $clocks] {
				lappend cardClocks $clock
				if { $clock < 0.1 } {
					incr zerocount
				}
			}
		}
	}
	close $fd
	set clockcount [llength $cardClocks]
	if { $clockcount == 0 } {
		$w.card.bot.message configure -text \
			"No clocks found!\n "
		bell
		return
	}
	if { 1.0*$zerocount/$clockcount > 0.25 } {
		if { $cardDetail == "std" } {
			$w.card.bot.message configure -text \
				"It appears you have a programmable clock chip.\n\
				Select Detailed Setup and pick your clock chip from the list"
		} else {
			$w.card.bot.message configure -text \
				"It appears you have a programmable clock chip.\n\
				Pick your clock chip from the list"
		}
		bell
		return
	}
	if { $cardDetail == "std" } {
		if { $cardReadmeWasSeen($cardDevNum) } {
		    $w.card.bot.message configure -text \
			"That's all there is to configuring your card\n\
			unless you would like to make changes to the\
			standard settings (by pressing Detailed Setup)"
		} else {
		    $w.card.bot.message configure -text \
			"That's probably all there is to configuring\
			your card, but you should probably check the\n\
			README to make sure. If any changes are needed,\
			press the Detailed Setup button"
		}
	}
	Card_showclockrates $win
}

proc Card_showclockrates { win } {
	global cardClocks

	set w [winpathprefix $win]
	set clockcount [llength $cardClocks]
	$w.card.clocks.text delete 0.0 end
	for {set i 0} {$i < $clockcount} {incr i} {
		$w.card.clocks.text insert end \
			[format "%6.2f " [lindex $cardClocks $i]]
		if { [expr $i%8] == 7 } {
			$w.card.clocks.text insert end "\n"
		}
	}
}

proc Card_switchdetail { win } {
	global cardDetail cardProbe cardDevNum

	set w [winpathprefix $win]
	if { $cardDetail == "std" } {
		set cardDetail detail
		$w.card.modebutton configure -text "Card List"
		pack forget $w.card.list
		pack $w.card.detail -expand yes -side top -fill both
		$w.card.bot.message configure -text \
			"First make sure the correct server is selected,\
			then make whatever changes are needed\n\
			If the Chipset, RamDac, or ClockChip entries\
			are left blank, they will be probed"
		$w.card.probe configure -state normal
	} else {
		set cardDetail std
		$w.card.modebutton configure -text "Detailed Setup"
		pack forget $w.card.detail
		pack $w.card.list   -expand yes -side top -fill both
		$w.card.bot.message configure -text \
			"Select your card from the list.\n\
			If your card is not listed,\
			click on the Detailed Setup button"
		if { $cardProbe($cardDevNum) } {
			$w.card.probe configure -state normal
		} else {
			$w.card.probe configure -state disabled
		}
			
	}
}

proc Card_cbox_setentry { cb text } {
	$cb econfig -state normal
	$cb edelete 0 end
	if [string length $text] {
		$cb einsert 0 $text
	}
	$cb econfig -state disabled
}

proc Card_selected { win lbox } {
	global cardServer cardProbe cardReadmeWasSeen cardDevNum

	set w [winpathprefix $win]
	set wframe $w.card$cardDevNum
	set cardentry [$lbox get [$lbox curselection]]
	set carddata [xf86cards_getentry $cardentry]
	set cardServer [lindex $carddata 2]
	Card_set_cboxlists $win
	$w.card.title configure -text "Card selected: $cardentry"
	#Card_cbox_setentry $w.card.chipset.cbox [lindex $carddata 1]
	Card_cbox_setentry $w.card.ramdac.cbox [lindex $carddata 3]
	Card_cbox_setentry $w.card.clockchip.cbox [lindex $carddata 4]
	$w.card.options.text insert 0.0 [lindex $carddata 6]
	set cardProbe($cardDevNum) \
		[expr [string first [lindex $carddata 7] NOCLOCKPROBE] < 0]
	if { $cardProbe($cardDevNum) } {
		$w.card.bot.message configure -text \
			"The clock rates of your clock chip should now be probed\n\
			Simply press the Probe Clocks button"
		$w.card.probe configure -state normal
	} else {
		
		if { $cardReadmeWasSeen($cardDevNum) } {
		    $w.card.bot.message configure -text \
			"That's all there is to configuring your card\n\
			unless you would like to make changes to the\
			standard settings (by pressing Detailed Setup)"
		} else {
		    $w.card.bot.message configure -text \
			"That's probably all there is to configuring\
			your card, but you should probably check the\n\
			README to make sure. If any changes are needed,\
			press the Detailed Setup button"
		}
		$w.card.probe configure -state disabled
	}
}

proc Card_set_cboxlists { win } {
	global CardChipSets CardRamDacs CardClockChips cardServer
	global CardReadmes cardReadmeWasSeen CardOptions

	set w [winpathprefix $win]
	if { [llength $CardReadmes($cardServer)] > 0 } {
		$w.card.readme configure -state normal
	} else {
		$w.card.readme configure -state disabled
	}
	$w.card.chipset.cbox ldelete 0 end
	if [llength $CardChipSets($cardServer)] {
		$w.card.chipset.cbox.button configure -state normal
		$w.card.chipset.cbox linsert end "<Probed>"
		eval [list $w.card.chipset.cbox linsert end] \
			$CardChipSets($cardServer)
	} else {
		$w.card.chipset.cbox.button configure -state disabled
	
	}
	set chipset [$w.card.chipset.cbox eget]
	if { [string length $chipset] && [lsearch \
			$CardChipSets($cardServer) $chipset] < 0} {
		Card_cbox_setentry $w.card.chipset.cbox ""
	}

	$w.card.ramdac.cbox ldelete 0 end
	if [llength $CardRamDacs($cardServer)] {
		$w.card.ramdac.cbox.button configure -state normal
		$w.card.ramdac.cbox linsert end "<Probed>"
		eval [list $w.card.ramdac.cbox linsert end] \
			$CardRamDacs($cardServer)
	} else {
		$w.card.ramdac.cbox.button configure -state disabled
	}
	set ramdac [$w.card.ramdac.cbox eget]
	if { [string length $ramdac] && [lsearch \
			$CardRamDacs($cardServer) $ramdac] < 0} {
		Card_cbox_setentry $w.card.ramdac.cbox ""
	}


	$w.card.clockchip.cbox ldelete 0 end
	if [llength $CardClockChips($cardServer)] {
		$w.card.clockchip.cbox.button configure -state normal
		$w.card.clockchip.cbox linsert end "<Probed>"
		eval [list $w.card.clockchip.cbox linsert end] \
			$CardClockChips($cardServer)
	} else {
		$w.card.clockchip.cbox.button configure -state disabled
	}
	set clockchip [$w.card.clockchip.cbox eget]
	if { [string length $clockchip] && [lsearch \
			$CardClockChips($cardServer) $clockchip] < 0} {
		Card_cbox_setentry $w.card.clockchip.cbox ""
	}

	$w.card.options.cbox ldelete 0 end
	if [llength $CardOptions($cardServer)] {
		$w.card.options.cbox.button configure -state normal
		eval [list $w.card.options.cbox linsert end] \
			$CardOptions($cardServer)
	} else {
		$w.card.options.cbox.button configure -state disabled
	}
	set options [$w.card.options.cbox eget]
	if { [string length $options] && [lsearch \
			$CardOptions($cardServer) $options] < 0} {
		Card_cbox_setentry $w.card.options.cbox ""
	}

}

proc Card_display_readme { win } {
	global cardServer CardReadmes cardReadmeWasSeen
	global cardDevNum Xwinhome

	set w [winpathprefix $win]
	catch {destroy .cardreadme}
        toplevel .cardreadme
	wm title .cardreadme "Chipset Specific README"
	wm geometry .cardreadme +30+30
	frame .cardreadme.file
        text .cardreadme.file.text -setgrid true \
		-xscroll ".cardreadme.horz.hsb set" \
		-yscroll ".cardreadme.file.vsb set"
	foreach file $CardReadmes($cardServer) {
		set fd [open $Xwinhome/lib/X11/doc/$file r]
		.cardreadme.file.text insert end [read $fd]
		close $fd
	}
	frame .cardreadme.horz
	scrollbar .cardreadme.horz.hsb -orient horizontal \
		-command ".cardreadme.file.text xview"
	scrollbar .cardreadme.file.vsb \
		-command ".cardreadme.file.text yview"
        button .cardreadme.ok -text "Okay" -command "destroy .cardreadme"
	focus .cardreadme.ok
        pack .cardreadme.file -side top -fill both
        pack .cardreadme.file.text -side left
        pack .cardreadme.file.vsb -side left -fill y
	#update idletasks
	#.cardreadme.horz configure -width [winfo width .cardreadme.file.text] \
		-height [winfo width .cardreadme.file.vsb]
        #pack propagate .cardreadme.horz 0
        #pack .cardreadme.horz -side top -anchor w
        #pack .cardreadme.horz.hsb -fill both
        pack .cardreadme.ok -side bottom
	set cardReadmeWasSeen($cardDevNum) 1
}

proc Card_cardselect { win } {
	global cardDevNum

	set w [winpathprefix $win]
	Card_set_configvars $win
	set cardDevNum [$w.card.cardselect curselection]
	Card_get_configvars $win
}

proc Card_set_configvars { win } {
	global DeviceIDs cardServer ServerList cardDevNum cardClocks
	global AccelServerList CardChipSets CardRamDacs CardClockChips
	global cardDacSpeed cardDacProbe cardRamSize UseConfigFile

	set w [winpathprefix $win]
	set devid [lindex $DeviceIDs $cardDevNum]
	global Device_$devid

	set Device_${devid}(Server)	$cardServer
	set Device_${devid}(Clocks)	$cardClocks
	set Device_${devid}(Chipset)	[$w.card.chipset.cbox eget]
	set Device_${devid}(Ramdac)	[$w.card.ramdac.cbox eget]
	set Device_${devid}(ClockChip)	[$w.card.clockchip.cbox eget]
	set Device_${devid}(ExtraLines)	[$w.card.options.text get 0.0 end]
	set Device_${devid}(Options)	[split [$w.card.options.cbox eget] ,]
	if { $UseConfigFile } {
	    if { $cardRamSize } {
	        set Device_${devid}(VideoRam)	$cardRamSize
	    } else {
	        set Device_${devid}(VideoRam)	""
	    }
	    if { $cardDacProbe } {
	        set Device_${devid}(DacSpeed)	""
	    } else {
	        set Device_${devid}(DacSpeed)	[expr $cardDacSpeed*1000]
	    }
	}
}

proc Card_get_configvars { win } {
	global DeviceIDs cardServer ServerList cardDevNum cardClocks
	global AccelServerList CardChipSets CardRamDacs CardClockChips
	global cardDacSpeed cardDacProbe cardRamSize UseConfigFile

	set w [winpathprefix $win]
	set devid [lindex $DeviceIDs $cardDevNum]
	global Device_$devid

	set cardServer		[set Device_${devid}(Server)]
	set cardClocks		[set Device_${devid}(Clocks)]
	Card_showclockrates $win
	Card_cbox_setentry $w.card.chipset.cbox [set Device_${devid}(Chipset)]
	Card_cbox_setentry $w.card.ramdac.cbox [set Device_${devid}(Ramdac)]
	Card_cbox_setentry $w.card.clockchip.cbox [set Device_${devid}(ClockChip)]
	$w.card.options.text delete 0.0 end
	$w.card.options.text insert 0.0 [set Device_${devid}(ExtraLines)]
	Card_cbox_setentry $w.card.options.cbox \
		[join [set Device_${devid}(Options)] ,]
	Card_set_cboxlists $win
	if { $UseConfigFile } {
	    set ram [set Device_${devid}(VideoRam)]
	    if { [string length $ram] > 0 } {
	        set cardRamSize	$ram
	    } else {
	        set cardRamSize	0
	    }
	    set speed [set Device_${devid}(DacSpeed)]
	    if { [string length $speed] > 0 } {
	        set cardDacSpeed	[expr int($speed/1000)]
	        set cardDacProbe	0
	    } else {
	        set cardDacSpeed	60
	        set cardDacProbe	1
	    }
	    Card_dacspeed $win
	}
}

proc Card_popup_help { win } {
	catch {destroy .cardhelp}
        toplevel .cardhelp
        wm title .cardhelp "Help"
	wm geometry .cardhelp +30+30
        text .cardhelp.text
        .cardhelp.text insert 0.0 "Help text for device configuration"
        button .cardhelp.ok -text "Okay" -command "destroy .cardhelp"
	focus .cardhelp.ok
        pack .cardhelp.text .cardhelp.ok
}


# $XFree86$
#

proc Done_create_widgets { w } {
	frame $w.done -width 640 -height 420 \
		-relief ridge -borderwidth 5
	frame $w.done.pad -relief raised
	pack $w.done.pad -padx 10 -pady 10 -expand yes
	text $w.done.pad.text -wrap word
	$w.done.pad.text insert end "If you've finished configuring\
		everything press the Okay button to start the\
		X server using the configuration you've selected.\n\n\
		If you still wish to configure some things,\
		press one of the buttons at the top."
	pack $w.done.pad.text -fill both -expand yes
	button $w.done.pad.okay -text "Okay" \
		-command [list Done_start_server $w]
	pack $w.done.pad.okay -side bottom
	focus $w.done.pad.okay
}

proc Done_activate { w } {
	pack $w.done -side top -fill both -expand yes
}

proc Done_deactivate { w } {
	pack forget $w.done
}

proc Done_popup_help { w } {
	toplevel .donehelp
	wm title .donehelp "Help"
	wm geometry .donehelp +30+30
	text .donehelp.text
	.donehelp.text insert 0.0 "Do you really need help with this?"
	button .donehelp.ok -text "Okay" -command "destroy .donehelp"
	focus .donehelp.ok
	pack .donehelp.text .donehelp.ok
}

proc Done_execute { w } {
	global CfgSelection

	set CfgSelection Done
	config_select $w
}

proc Done_start_server { w } {
	global Confname Xwinhome DeviceIDs env ServerPID2 StartServer TmpDir

	if $StartServer {
		set tmpfd [open $Confname-2 w]
		writeXF86Config $tmpfd -defaultmodes
		close $tmpfd

		set env(DISPLAY) :2
		set devid [lindex $DeviceIDs 0]
		global Device_$devid
		set server [set Device_${devid}(Server)]

		set ServerPID2 [exec $Xwinhome/bin/XF86_$server \
			$env(DISPLAY) -xf86config \
			$Confname-2 >& $TmpDir/XSout[pid].2 & ]

		sleep 15
		while { ![server_running $env(DISPLAY)] } {
			puts "waiting for server..."
			sleep 5
		}
	}
	set w .phase3
	toplevel $w -screen $env(DISPLAY)
	set wid [winfo screenwidth $w]
	set ht  [winfo screenheight $w]
	wm geometry $w ${wid}x${ht}+0+0
	pack propagate $w no
	label $w.l -text "Congratulations, you've got a running server!"
	label $w.q -text "Would you like to run xvidtune?"
	frame $w.f
	button $w.f.y -text "Yes" -command [list Done_run_xvidtune $w]
	button $w.f.n -text "No"  -command [list Done_the_end $w]
	button $w.f.a -text "Abort"  -command "exit 1"
	pack $w.l $w.q $w.f -side top
	pack $w.f.y $w.f.n $w.f.a -side left
	focus $w.f.n
}

proc Done_run_xvidtune { w } {
	global Xwinhome

	exec $Xwinhome/bin/xvidtune
}

proc Done_the_end { w } {
	global Confname ConfigFile

	set tmpfd [open $Confname-3 w]
	writeXF86Config $tmpfd -displayof $w
	close $tmpfd
	exec mv $ConfigFile $ConfigFile.bak
	exec cp $Confname-3 $ConfigFile
	pack forget $w.l $w.q $w.f
	label $w.text -text "The configuration has been completed.\n\
		A copy of your old configuration file has been saved as\n\
		$ConfigFile.bak"
	button $w.okay -text "Okay"  -command "exit 0"
	pack $w.text $w.okay -side top
}



#
# Phase IV - Commands run after second server is started
#

if $StartServer {
	set_resource_defaults

	wm withdraw .

	create_main_window [set w .xf86setup]

	# put up a message ASAP so the user knows we're still alive
	label $w.waitmsg -text "Loading  -  Please wait..."
	pack  $w.waitmsg -expand yes -fill both
	update idletasks

	source $tk_library/tk.tcl
	set msg "Congratulations, you've got a running server!\n\n"
} else {
	set msg ""
}

proc Phase4_run_xvidtune { win } {
	global Xwinhome

	exec $Xwinhome/bin/xvidtune
}

proc Phase4_nextphase { win } {
	global Confname ConfigFile StartServer

	set w [winpathprefix $win]
	writeXF86Config $Confname-3 -displayof $win
	set text "The configuration has been completed.\n"
	if { [getuid] != 0 } {
		set text "$text\
			No changes were made to the file $ConfigFile,\n\
			because you are not running as root."
	}
	if {![getuid] && ![catch {exec mv $ConfigFile $ConfigFile.bak} ret]} {
           set text "$text\
		A copy of your old configuration file has been saved as\n\
		$ConfigFile.bak"
	}
	if {![getuid] && [catch {exec cp $Confname-3 $ConfigFile} ret] != 0} {
           set text "$text\n\
		However, I am unable to save the configuration to\n
		the file $ConfigFile"
	}
	$w.text configure -text $text
	pack   forget $w.buttons
	set cmd shutdown
	if !$StartServer {
		append cmd {;source $XF86Setup_library/phase5.tcl}
	}
	button $w.okay -text "Okay"  -command $cmd
	pack   $w.text $w.okay -side top
}

label  $w.text -text " $msg\
	You can now run xvidtune to adjust your display settings,\n\
	if you want to change the size or placement of the screen image\n\n\
	If not, go ahead and exit"
frame  $w.buttons
button $w.buttons.xvidtune -text "Run xvidtune" \
	-command [list Phase4_run_xvidtune $w]
button $w.buttons.save -text "Save the configuration and exit" \
	-command [list Phase4_nextphase $w]
button $w.buttons.abort -text "Abort - Don't save the configuration" \
	-command "puts stderr Aborted;shutdown 1"
pack   $w.buttons.xvidtune $w.buttons.save $w.buttons.abort -side top \
	-pady 5m -fill x

catch {destroy $w.waitmsg}
pack    $w.text $w.buttons -side top -pady 10m
focus   $w.buttons.save


#
#

proc Keyboard_create_widgets { w } {
	global XKBComponents

	frame $w.keyboard -width 640 -height 420 \
		-relief ridge -borderwidth 5
	#pack propagate $w.keyboard 0
	frame $w.keyboard.xkb
	pack $w.keyboard.xkb -side top -expand yes -fill both
	label $w.keyboard.xkb.geotitle -text "Layout:"
	combobox $w.keyboard.xkb.geometry
	label $w.keyboard.xkb.symtitle -text "Symbol set:"
	combobox $w.keyboard.xkb.language
	eval [list $w.keyboard.xkb.geometry linsert end] \
		$XKBComponents(geometry)
	eval [list $w.keyboard.xkb.language linsert end] \
		$XKBComponents(symbols)
	pack $w.keyboard.xkb.geotitle -side left -expand yes -fill x
	pack $w.keyboard.xkb.geometry -side left -expand yes -fill x
	pack $w.keyboard.xkb.symtitle -side left -expand yes -fill x
	pack $w.keyboard.xkb.language -side left -expand yes -fill x
	frame $w.keyboard.graphic -width 600 -height 260
	pack $w.keyboard.graphic  -side top -expand yes -fill both
	bind $w.keyboard.graphic <Expose> [list xkb_show $w.keyboard.graphic]

	global ServerFlags otherZap otherZoom otherTrapSignals
	global otherXvidtune otherInpDevMods

	frame $w.other -relief sunken -borderwidth 3
	pack $w.other -side bottom -in $w.keyboard -fill both -expand yes
	label $w.other.title -text "Optional server settings:"
	pack $w.other.title -side top -fill x -expand yes
	checkbutton $w.other.zap         -indicatoron true \
		-text "Allow server to be killed with\
		hotkey sequence (Ctrl-Alt-Backspace)" -variable otherZap
	checkbutton $w.other.zoom        -indicatoron true \
		-text "Allow video mode switching" \
		-variable otherZoom
	checkbutton $w.other.trapsignals -indicatoron true \
		-text "Don't Trap Signals - prevents the server from exitting cleanly" \
		-variable otherTrapSignals
	checkbutton $w.other.nonlocalxvidtune -indicatoron true \
		-text "Allow video mode changes from other hosts" \
		-variable otherXvidtune
	checkbutton $w.other.nonlocalmodindev -indicatoron true \
		-text "Allow changes to keyboard and mouse settings\
		from other hosts" -variable otherInpDevMods
	pack $w.other.zap $w.other.zoom $w.other.trapsignals -anchor w
	pack $w.other.nonlocalxvidtune $w.other.nonlocalmodindev -anchor w
	set otherZap	[expr [string length $ServerFlags(DontZap)] == 0]
	set otherZoom	[expr [string length $ServerFlags(DontZoom)] == 0]
	set otherTrapSignals [expr [string length $ServerFlags(NoTrapSignals)] > 0]
	set otherXvidtune	[expr [string length $ServerFlags(AllowNonLocalXvidtune)] > 0]
	set otherInpDevMods	[expr [string length $ServerFlags(AllowNonLocalModInDev)] > 0]

}

proc Keyboard_activate { w } {
	pack $w.keyboard -side top -fill both -expand yes
	update idletasks
	raise $w.keyboard.graphic
}

proc Keyboard_deactivate { w } {
	global ServerFlags otherZap otherZoom otherTrapSignals
	global otherXvidtune otherInpDevMods

	pack forget $w.keyboard
	set ServerFlags(DontZap) [expr $otherZap?"":"DontZap"]
	set ServerFlags(DontZoom) [expr $otherZoom?"":"DontZoom"]
	set ServerFlags(NoTrapSignals) \
		[expr $otherTrapSignals?"NoTrapSignals":""]
	set ServerFlags(AllowNonLocalXvidtune) \
		[expr $otherXvidtune?"AllowNonLocalXvidtune":""]
	set ServerFlags(AllowNonLocalModInDev) \
		[expr $otherInpDevMods?"AllowNonLocalModInDev":""]
}

proc Keyboard_popup_help { w } {
        toplevel .keyboardhelp
        wm title .keyboardhelp "Help"
	wm geometry .keyboardhelp +30+30
        text .keyboardhelp.text
        .keyboardhelp.text insert 0.0 "Keyboard configuration settings help text"
        button .keyboardhelp.ok -text "Okay" -command "destroy .keyboardhelp"
        focus .keyboardhelp.ok
        pack .keyboardhelp.text .keyboardhelp.ok
}

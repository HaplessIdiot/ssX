
array set Pointer {
	Device		""
	Protocol	""
	BaudRate	""
	SampleRate	""
	Emulate3Buttons	""
	Emulate3Timeout	""
	ChordMiddle	""
	ClearDTR	""
	ClearRTS	""
}

set ConfigFile [xf86config_findfile]
if {![catch {xf86config_readfile $Xwinhome files server \
		keyboard mouse monitor device screen} tmp]} {
	if [info exists mouse] {
		set Pointer(Device) $mouse(Device)
	}
	foreach var {files server keyboard mouse}
		catch {unset $var}
	}
	foreach var [info vars monitor_*] {
		catch {unset $var}
	}
	foreach var [info vars device_*] {
		catch {unset $var}
	}
	foreach var [info vars screen_*] {
		catch {unset $var}
	}
}

source $XF86Setup_library/setuplib.tcl
set_resource_defaults
source $XF86Setup_library/mouse.tcl
Mouse_create_widgets .
Mouse_activate .
button .mouse.exit -text "Exit" -command "exit 0"
pack .mouse.exit -side bottom -expand yes -fill x
bind . <Alt-x>	"exit 0"

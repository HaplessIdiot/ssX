
#
# Phase III - Commands run after switching back to text mode
#     - responsible for starting second server
#

if [info exists env(TMPDIR)] {
	set TmpDir $env(TMPDIR)
} else {
	set TmpDir /tmp
}

source $tk_library/init.tcl
source $XF86Setup_library/setuplib.tcl
source $XF86Setup_library/carddata.tcl
source $XF86Setup_library/mondata.tcl
source $StateFileName

mesg "Attempting to start server..." info
sleep 2
writeXF86Config $Confname-2 -defaultmodes

set devid [lindex $DeviceIDs 0]
global Device_$devid
set server [set Device_${devid}(Server)]

set ServerPID [start_server $server $Confname-2 XSout[pid].2 ]

if { $ServerPID == -1 } {
	set msg "Unable to communicate with X server"
}

if { $ServerPID == 0 } {
	set msg "Unable to start X server"
}

if { $ServerPID < 1 } {
	mesg "$msg\n\nPress \[Enter\] to try configuration again" okay
	set Phase2FallBack 1
	if { [info exists $Confname-1g] } {
		set ServerPID [start_server $server \
			$Confname-2 XSout[pid].2 ]
	} else {
		set ServerPID [start_server $server \
			$Confname-2 XSout[pid].2 ]
	}
	if { $ServerPID < 1 } {
		mesg "Ack! Unable to get the VGA16 server going again!" info
		exit 1
	}
}


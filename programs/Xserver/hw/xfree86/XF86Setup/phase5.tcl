# $XFree86: xc/programs/Xserver/hw/xfree86/XF86Setup/phase5.tcl,v 3.1 1996/08/20 13:09:28 dawes Exp $
#
# Copyright 1996 by Joseph V. Moss <joe@XFree86.Org>
#
# See the file "LICENSE" for information regarding redistribution terms,
# and for a DISCLAIMER OF ALL WARRANTIES.
#

#
# Phase V - Final commands after return to text mode
#

check_tmpdirs
foreach fname [glob -nocomplain $TmpDir/*] {
	unlink $fname
}
rmdir $TmpDir
rmdir $XF86SetupDir

if { ![getuid] && [llength $DeviceIDs] == 1 } {
    # Link RealServer to X
    set devid [lindex $DeviceIDs 0]
    global Device_$devid
    set server [set Device_${devid}(Server)]
    set CWD [pwd]
    cd $Xwinhome/bin
    catch "unlink X" ret
    catch "link XF86_$server X" ret
    cd $CWD
    puts "You can now easily invoke X with xdm or startx\n\
	XF86_$server is linked to X"
}

clear_scrn
puts "\n\nConfiguration complete."

exit 0


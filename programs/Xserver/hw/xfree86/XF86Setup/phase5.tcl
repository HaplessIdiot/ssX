
#
# Phase V - Final commands after return to text mode
#

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

puts "\n\nConfiguration complete."

exit 0


# remove all whitespace from the string

proc zap_white { str } {
	regsub -all "\[ \t\n\]+" $str {} str
	return $str
}


# replace all sequences of whitespace with a single space

proc squash_white { str } {
	regsub -all "\[ \t\n\]+" $str { } str
	return $str
}


# implement do { ... } while loop
proc do { commands while expression } {
	uplevel $commands
	while { [uplevel [list expr $expression]] } {
		uplevel $commands
	}
}


# break a long line into shorter lines
proc parafmt { llen string } {
	set string [string trim [squash_white $string]]
	set retval ""
	while { [string length $string] > $llen } {
		set tmp [string range $string 0 $llen]
		#puts stderr "'$string'$tmp'$retval'"
		set pos [string last " " $tmp]
		if { $pos == -1 } {
			append retval [string range $string 0 [expr $llen-1]]\n
			set string [string range $string $llen end]
			continue
		}
		if { $pos == 0 } {
			append retval [string range $string 1 [expr $llen]]\n
			set string [string range $string $llen end]
			continue
		}
		if { $pos == $llen-1 } {
			append retval [string range $string 0 [expr $llen-2]]\n
			set string [string range $string $llen end]
			continue
		}
		append retval [string range $tmp 0 [expr $pos-1]]\n
		set string [string range $string [expr $pos+1] end]
	}
	#return [string trimright $retval \n]\n$string
	return $retval$string
}

#  convert the window name to a form that can be used as a prefix to
#    to the window names of child windows
#  - basically, avoid double dot
proc winpathprefix { w } {
	if ![string compare . $w] { return "" }
	return $w
}


# $XFree86$

dir XFT_TYPE1_DIR

#
# Check users config file
#

includeif	"~/.xftconfig"

match any family == "serif" edit    family =+ "times";
match any family == "sans" edit	    family =+ "helvetica";
match any family == "mono" edit	    family =+ "courier";
	

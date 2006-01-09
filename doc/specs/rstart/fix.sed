#! /bin/sed -f
#
# $XFree86$
#
s/o+/./g
s/|-/+/g
s/.//g
/FORMFEED\[Page/{
s/FORMFEED/        /
a\

}

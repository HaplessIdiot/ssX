#! /bin/nawk -f
# $XFree86$
#

BEGIN {
	ignore = 1;
}

/FORMFEED\[Page/	{
	sub("FORMFEED", "        ");
	print;
	print "";
	ignore = 1;
	next;
}

$0 == "" {
	if(ignore) next;
}

{
	ignore = 0;
	print;
}

#!/bin/sh

# $XFree86$
# This file is mean't to be used when building xedit on a operating system
# that does not have gperf.
# If you have gperf installed, add the lines:
#
#	#define HasGperf	YES
#	#define GperfCmd	gperf
#
# to xc/config/cf/host.def file or patch
# xc/config/cf/<your-operating-system>.cf

GPERF=`which gperf`
if [ X$GPERF != X ]; then
    # gperf is available, use it.
    exec $GPERF $@
fi

FUNCTION=
FILE=
for OPT in $*; do
    case $OPT in
	-*|1*|2*|3*|4*|5*|6*|7*|8*|9*)	;;
	*)  if [ X$FUNCTION = X ]; then
		FUNCTION=$OPT
	    elif [ X$FILE = X ]; then
		FILE=$OPT
	    else
		echo "$0: Bad arguments $*"
		exit 1
	    fi
	;;
    esac
done

if [ X$FUNCTION = X -o X$FILE = X ]; then
    echo "$0: Missing arguments $*"
    exit 1
fi

cat $FILE | (
read STRUCT

while read LINE; do
    if [ X"$LINE" = X"%%" ]; then
	break
    fi
done

while read LINE; do
    if [ X"$LINE" = X"%%" ]; then
	break
    fi
    LINE=`echo "$LINE" | sed -e 's/^/\"/' -e 's/,/\",/' -e 's/,/, /g'`
    FIELDS="$FIELDS
	{$LINE},"
done

cat << EOF
static int
qcompar(const void *a, const void *b)
{
    return (strcmp((($STRUCT*)a)->name,
		   (($STRUCT*)b)->name));
}

static int
bcompar(const void *a, const void *b)
{
    return (strcmp((char*)a, (($STRUCT*)b)->name));
}

$STRUCT *
$FUNCTION(str, len)
    register const char *str;
    register unsigned int len;
{
    static int first = 1;
    static $STRUCT wordlist[] = {$FIELDS
    };

    if (first) {
	qsort(wordlist, sizeof(wordlist) / sizeof(wordlist[0]),
	      sizeof($STRUCT), qcompar);
	first = 0;
    }

    return (bsearch(str, wordlist, sizeof(wordlist) / sizeof(wordlist[0]),
		    sizeof($STRUCT), bcompar));
}
EOF
)

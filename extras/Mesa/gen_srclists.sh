#! /bin/sh
#
# NOTICE: This script uses the "printf" shell utility which is, I guess, not
# very portable. It would probably be best to rewrite this thing in Perl.

# predefined source file patterns
c_pattern='.*\.[ch]'
cxx_pattern='.*\.\(cc\|C\|cpp\|hpp\)'

# flags
verbose=no
dry_run=no
# other settings
file_pattern=

# cmdline args
optstring=vhnp:l:
set -- `getopt $optstring $*`
while [ x$1 != x-- ]; do
    case $1 in
	-v)
	    verbose=yes;;
	-n)
	    dry_run=yes;;
        -p)
	    file_pattern="$optarg";;
        -l)
	    shift
	    old_pattern="$file_pattern\|"
	    if test "x$old_pattern" = "x\|"; then
		old_pattern=
	    fi
            case $1 in
		c|C)
		    file_pattern="$old_pattern\($c_pattern\)";;
		c++|C++)
		    file_pattern="$old_pattern\($cxx_pattern\)";;
		*)
		    echo $1: unknown language
	    esac;;
	-h)
	    cat <<EOF
Usage: ./gen_srclists.sh [option]... [directory]...

Generate source list files in directories containing a Makefile.am which in
turn contains exactly one (1) line matching /^include .*_SOURCES$/ where the
filename exactly matches the name of the desired SOURCES variable.
The list of directories to process can be specified on the command line. The
default is the current directory and all its subdirs that contain a Makefile.am.

Options:
  -v            Be verbose.
  -n            Dry run: do nothing, only report which files would be created.
  -p PATTERN    Put files matching PATTERN in the source list.
  -l LANG       Use a file pattern predefined for LANGUAGE. Multiple -l options
                can be specified and will be or-combined.
		Currently defined languages are C and C++.
  -h            Show this message and quit.
EOF
	    exit;;
    esac
    shift
done
shift

# set the file pattern to C by default
if test "x$file_pattern" = "x"; then
    file_pattern="$c_pattern"
fi
echo Source file pattern: $file_pattern

# build the directory list
if test "x$*" = "x"; then
    dirlist=`find . -name Makefile.am | sed -e 's,/*(Makefile.am)?,,'`
else
    dirlist=$*
fi

verb() {
    if test x$verbose = xyes; then
	echo "$1"
    fi
}

for dir_name in $dirlist; do
    if ! test -f "$dir_name/Makefile.am"; then
	verb "$dir_name: no Makefile.am, skipping"
	continue
    fi
    pushd $dir_name > /dev/null
    var_exp='^include .*_SOURCES$'
    var_count=`egrep -c "$var_exp" Makefile.am`
    if test x$var_count != x1; then
	verb "$dir_name/Makefile.am: source list include count $var_count; \
skipping dir"
    else
	var_name=`egrep "$var_exp" Makefile.am | sed -e 's/^include //'`
	if test x$dry_run != xyes; then
	    file_list=`find -regex "$file_pattern" | sed -e 's,^./,,' | sort`
	    printf "$var_name =" > "$var_name"
	    for file_name in $file_list; do
		    printf " \\\\\\n\\t$file_name" >> "$var_name"
	    done
	    printf "\\n" >> "$var_name"
	fi
	echo "$dir_name/$var_name"
    fi
    popd > /dev/null
done

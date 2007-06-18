#! /bin/sh

MODULES_DIRS=". test"

SCRIPT_DIR=`dirname $0`
SCRIPT_DIR=`cd $SCRIPT_DIR && pwd`

$SCRIPT_DIR/clean.sh

#
# check tools version number -- we require >= 1.9
#
function find_tools {
    tool=$1

    if `which $tool-1.9 > /dev/null 2>&1`; then
	TOOL=$tool-1.9
    elif `$tool --version | sed -e '1s/[^9]*//' -e q | grep -v '^$'`; then
	TOOL=$tool
    else
	echo "Required: $tool version 1.9"
	exit 1;
    fi
    
    echo "$TOOL"
}

ACLOCAL=$(find_tools aclocal)
AUTOMAKE=$(find_tools automake)

for module in $MODULES_DIRS; do
    test -f $module/configure.ac && (
	cd $module
	echo "Regenerating module: $module"
	libtoolize -c \
	    && autoheader \
	    && $ACLOCAL \
	    && $AUTOMAKE --add-missing --foreign \
	    && autoconf
    )
done

$AUTOMAKE --add-missing --foreign 
# Workaround a bug in automake which does
# not include depcomp in DIST_COMMON

#!/bin/sh

MODULES_DIRS=". config"

AUTOGEN_FILES="aclocal.m4 autom4te.cache configure config.guess config.log config.sub config.status depcomp install-sh compile libtool ltmain.sh missing mkinstalldirs .deps"

for module in $MODULES_DIRS; do
    test -d $module && (
	cd $module
	echo "Clean $module"
	
	for file in $AUTOGEN_FILES; do
	    rm -rf $file
	done    
	
	for file in `find -name "Makefile" | grep -v '.svn'`; do
	    test -f `dirname $file`/Makefile.in && rm -f $file
	done
	
	for file in `find -name "Makefile.in" | grep -v '.svn'`; do
	    test -f `dirname $file`/Makefile.am && rm -f $file
	done
    )
done

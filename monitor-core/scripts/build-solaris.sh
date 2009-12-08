#!/bin/bash

# If the code has just been taken from svn, this needs to be run first:
# PATH=/opt/csw/bin:$PATH ./bootstrap

if [ ! -f configure ];
then
  echo "Please run ./bootstrap first, it will generate configure, e.g."
  echo '  PATH=/opt/csw/bin:$PATH ./bootstrap'
  exit 1
fi

# Change these if you have installed them elsewhere

# APR is provided by CSWapache2rt (apache2rt)
# libConfuse is provided by CSWlibconfuse (libconfuse)

CSW_HOME=/opt/csw
APR=${CSW_HOME}/bin/apr-1-config
LIBCONFUSE=${CSW_HOME}/apache2

if [ ! -d ${CSW} ];
then
  echo "Couldn't find ${CSW} - please make sure that"
  echo "Blastwave is installed and the path is correct in $0"
  exit 1
fi

if [ ! -x ${APR} ];
then
  echo "Couldn't execute ${APR} - please make sure that"
  echo "apr is installed and the path is correct in $0"
  exit 1
fi

if [ ! -d ${LIBCONFUSE} ];
then
  echo "Couldn't find ${LIBCONFUSE} - please make sure that"
  echo "confuse is installed and the path is correct in $0"
  exit 1
fi

MAKE=${CSW_HOME}/bin/gmake

if [ ! -x ${GMAKE} ];
then
  echo "Couldn't execute ${GMAKE} - please make sure that"
  echo "gmake is installed and the path is correct in $0"
  exit 1
fi

MINOR=`uname -r | cut -d "." -f 2`
if [ $MINOR -lt 10 ];
then
  echo "C99 not supported for Solaris < 10"
  ac_cv_prog_cc_c99=no
  export ac_cv_prog_cc_c99
fi

./configure \
  --prefix=/usr/local \
  --without-gcc --disable-nls \
  --enable-shared --disable-static --disable-static-build \
  --without-gmetad --disable-python \
  --with-libapr=${APR} --with-libconfuse=${LIBCONFUSE} --with-libexpat=${CSW_HOME} \
  CC=/opt/SUNWspro/bin/cc \
  LD=/usr/ccs/bin/ld \
  MAKE=${MAKE} \
  AR=/usr/ccs/bin/ar \
  LDFLAGS="-R/opt/csw/lib"

${MAKE}



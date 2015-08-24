dnl
dnl
dnl "Copyright (c) 2001 by Matt Massie and The Regents of the University
dnl  of California.  All rights reserved."
dnl
dnl Permission to use, copy, modify, and distribute this software and its
dnl documentation for any purpose, without fee, and without written agreement is
dnl hereby granted, provided that the above copyright notice and the following
dnl two paragraphs appear in all copies of this software.
dnl
dnl IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
dnl DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
dnl OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
dnl CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
dnl
dnl THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
dnl INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
dnl AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
dnl ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
dnl PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
dnl

#AC_PREREQ(2.61)

AC_REVISION([m4_esyscmd_s([git describe --always])])
AC_INIT(gmond/gmond.c)
AC_CONFIG_AUX_DIR(build)

### BEGIN RELEASE VARIABLES ############################
# On making a release...
# If this release is just a minor bug fix release, increment
# GANGLIA_MICRO_VERSION.
#
# If this release adds significant features or breaks backward compatibility,
# increment GANGLIA_MINOR_VERSION and set GANGLIA_MICRO_VERSION to 0 (zero).
#
# If this release is a complete restructuring of
# the overall architecture of ganglia,
# increment GANGLIA_MAJOR_VERSION and set
# GANGLIA_MINOR_VERSION and GANGLIA_MICRO_VERSION
# to 0 (zero)
GANGLIA_MAJOR_VERSION=3
GANGLIA_MINOR_VERSION=7
GANGLIA_MICRO_VERSION=2

# REL is used for various packaging systems:
# RHEL:
# - it is the `Release:' parameter in the spec file
# - appended after the version number, using a hyphen, e.g. X.X.X-REL
# Solaris:
# - it can be used as part of the VERSION= parameter in the
#   pkginfo file (man 4 pkginfo), e.g. VERSION=X.X.X,REV=@REL@
#   appears to be consistent with the way Blastwave does things
REL=1

# If there are any changes to libganglia in this release, you need to...
#   LIBGANGLIA_MICRO_VERSION += 1;
#   LIBGANGLIA_INTERFACE_AGE += 1;
#   LIBGANGLIA_BINARY_AGE += 1;
# if any functions have been added, set LIBGANGLIA_INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set LIBGANGLIA_BINARY_AGE _and_ LIBGANGLIA_INTERFACE_AGE to 0.
LIBGANGLIA_MICRO_VERSION=0
LIBGANGLIA_INTERFACE_AGE=0
LIBGANGLIA_BINARY_AGE=0
### END RELEASE VARIABLES #############################

GANGLIA_VERSION=$GANGLIA_MAJOR_VERSION.$GANGLIA_MINOR_VERSION.$GANGLIA_MICRO_VERSION

AM_INIT_AUTOMAKE(ganglia, $GANGLIA_VERSION)

AC_SUBST(GANGLIA_MAJOR_VERSION)
AC_SUBST(GANGLIA_MINOR_VERSION)
AC_SUBST(GANGLIA_MICRO_VERSION)
AC_SUBST(GANGLIA_VERSION)
AC_SUBST(REL)

AC_SUBST(LIBGANGLIA_INTERFACE_AGE)
AC_SUBST(LIBGANGLIA_BINARY_AGE)

# libtool versioning
# the LT_RELEASE variable is passed with the --release option... it is just
# a way to tie our internal package library with the release numbers of ganglia in general
LT_RELEASE=$GANGLIA_VERSION
# The most current libganglia interface is MICRO - INTERFACE
LT_CURRENT=`expr $LIBGANGLIA_MICRO_VERSION - $LIBGANGLIA_INTERFACE_AGE`
# The implementation number of the current interface is simply LIBGANGLIA_INTERFACE_AGE
LT_REVISION=$LIBGANGLIA_INTERFACE_AGE
# The different between the new and oldest interfaces this library implements is BINARY-INTERFACE
LT_AGE=`expr $LIBGANGLIA_BINARY_AGE - $LIBGANGLIA_INTERFACE_AGE`

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

# Define version stuff
AC_DEFINE_UNQUOTED(GANGLIA_MAJOR_VERSION, $GANGLIA_MAJOR_VERSION, GANGLIA_MAJOR_VERSION)
AC_DEFINE_UNQUOTED(GANGLIA_MINOR_VERSION, $GANGLIA_MINOR_VERSION, GANGLIA_MINOR_VERSION)
AC_DEFINE_UNQUOTED(GANGLIA_MICRO_VERSION, $GANGLIA_MICRO_VERSION, GANGLIA_MICRO_VERSION)

AC_DEFINE_UNQUOTED(REL, $REL, REL)
AC_DEFINE_UNQUOTED(GANGLIA_VERSION_FULL, "${VERSION}-${REL}", GANGLIA_VERSION_FULL)

GWEB_VERSION=$GANGLIA_VERSION
AC_SUBST(GWEB_VERSION)

########################
AC_CANONICAL_HOST
AM_CONFIG_HEADER(config.h)

dnl The following cpu_vendor_os string goes into config.h.
dnl
AC_DEFINE_UNQUOTED(HOST_OS, "$host_os", HOST_OS)
AC_DEFINE_UNQUOTED(CPU_VENDOR_OS, "$host", CPU_VENDOR_OS)
dnl AC_CYGWIN

nl='
'
dnl ##################################################################
dnl Checks for programs.
dnl
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_LIBTOOL
m4_ifdef([AC_PROG_SED],
  [AC_PROG_SED],
  [
    SED="sed"
    AC_SUBST([SED])
  ]
)

AC_ARG_ENABLE( static-build,
[  --enable-static-build   Statically link libGanglia, APR, Expat and Confuse libraries (may disable some metric modules) ],
[ enable_static_build=yes ],[ enable_static_build=no ])

AC_SUBST(modpathstart)
AM_CONDITIONAL(STATIC_BUILD, test x"$enable_static_build" = xyes)

echo $ac_n "${nl}Configuring libmetrics ...${nl}"
# This uses our command line and environment to run a nested
# configure script in libmetrics
# Using AC_CONFIG_SUBDIRS rather than just calling the configure script
# directly is important to support cross-compiling.
AC_CONFIG_SUBDIRS([libmetrics])

modpathstart=""
if test x"$enable_static_build" = xyes; then
  modpathstart="#"
fi

dnl ##################################################################
dnl Special parameters
dnl

AC_ARG_WITH( static-modules,
[  --with-static-modules   link user-supplied static modules with gmond],
if test x"$withval" != xno; then STATIC_USER_MODULES=$withval; fi)

AC_ARG_WITH( gmetad,
[  --with-gmetad           compile and install the Ganglia Meta Daemon],
if test x"$withval" = xyes; then gmetad="yes"; fi)

AM_CONDITIONAL(BUILD_GMETAD, test x"$gmetad" = xyes)

AC_ARG_WITH( python,
[  --with-python=PATH      Specify prefix for python or full path to interpreter],
[if test x"$withval" != xno; then enable_python="yes"; PYTHON_BIN="$withval"; fi])

AC_ARG_WITH( perl,
[  --with-perl=PATH        Specify prefix for perl or full path to interpreter],
[if test x"$withval" != xno; then enable_perl="yes"; PERL_BIN="$withval"; fi])

AC_ARG_WITH( librrd,
[  --with-librrd=DIR       Specify location for librrd],
[if test x"$withval" != xno; then librrd="yes"; librrdpath="$withval"; fi])

AC_ARG_WITH( libapr,
[  --with-libapr=PATH      Specify location for libapr-1 or full path to apr-1-config],
[if test x"$withval" != xno; then libapr="yes"; libaprpath="$withval"; fi])

AC_ARG_WITH( libexpat,
[  --with-libexpat=DIR     Specify location for libexpat],
[if test x"$withval" != xno; then libexpat="yes"; libexpatpath="$withval"; fi])

AC_ARG_WITH( libconfuse,
[  --with-libconfuse=DIR   Specify location for libconfuse],
[if test x"$withval" != xno; then libconfuse="yes"; libconfusepath="$withval"; fi])

AC_ARG_WITH( libpcre,
[  --with-libpcre=DIR      Specify location for libpcre],
[if test x"$withval" != xno; then libpcre="yes"; libpcrepath="$withval"; fi],
[libpcre=yes; libpcrepath=yes ])

AC_ARG_WITH( moduledir,
[  --with-moduledir=DIR    Base directory for dynamically loadable modules],
  moduledir="$withval",
  moduledir="$libdir/ganglia")

AC_ARG_ENABLE(debug,
[
  --enable-debug          turn on debugging output and compile options],
[ CFLAGS="$CFLAGS -ggdb -O0"
  AC_DEFINE(DEBUG, 1, DEBUG)])
# Turn off optimizations when debugging.
#[ CFLAGS="$CFLAGS -O2"
#  AC_DEFINE(DEBUG, 0, DEBUG)])

AC_ARG_ENABLE(gexec,
[  --enable-gexec          turn on gexec support (platform-specific) ],
AC_DEFINE(SUPPORT_GEXEC, 1, SUPPORT_GEXEC), AC_DEFINE(SUPPORT_GEXEC, 0, SUPPORT_GEXEC))

setuid_user=nobody
AC_ARG_ENABLE(setuid,
[  --enable-setuid=USER    turn on setuid support (default setuid=nobody) ],
[if test x"$enableval" != xno; then no_setuid=0; setuid_user=$enableval ; fi],
[ no_setuid=0 ])
AC_DEFINE_UNQUOTED(NO_SETUID, $no_setuid, NO_SETUID)
AC_DEFINE_UNQUOTED(SETUID_USER, "$setuid_user", SETUID_USER)
AC_SUBST(setuid_user)

setgid_group=daemon
AC_ARG_ENABLE(setgid,
[  --enable-setgid=GROUP   turn on setgid support (default setgid=no) ],
[if test x"$enableval" != xno; then no_setgid=0; setgid_group=$enableval ; fi],
[ no_setgid=1 ])
AC_DEFINE_UNQUOTED(NO_SETGID, $no_setgid, NO_SETGID)
AC_DEFINE_UNQUOTED(SETGID_GROUP, "$setgid_group", SETGID_GROUP)
AC_SUBST(setgid_group)

AC_ARG_ENABLE(pedantic,
[  --enable-pedantic       turn on pedantic mode during compile],
[ CFLAGS="$CFLAGS --pedantic" ],)

AC_ARG_ENABLE(memcheck,
[  --enable-memcheck       turn on memory checking during compile
                           note: requires mpatrol from
                           http://www.cbmamiga.demon.co.uk/mpatrol/],
[ enable_memcheck="yes" ])

AC_ARG_ENABLE( python,
[  --disable-python        exclude mod_python and support for metric modules written in python],
[ if test x"$enableval" != xyes; then enable_python="no"; fi ], [ enable_python="yes" ] )

AC_ARG_ENABLE( perl,
[  --enable-perl           include mod_perl and support for metric modules written in perl],
[ if test x"$enableval" != xno; then enable_perl="yes"; fi ], [ enable_perl="no" ] )

AC_ARG_ENABLE( status,
[  --enable-status         compile and install modgstatus to enable gmond status metric module support],
[ enable_status="yes"
  AC_DEFINE(GSTATUS, 1, GSTATUS)],
[ enable_status="no" ] )

AM_CONDITIONAL(BUILD_STATUS, test x"$enable_status" = xyes)

AC_ARG_ENABLE( sflow,
[  --disable-sflow         exclude sFlow gateway],
[ if test x"$enableval" != xyes; then enable_sflow="no"; fi ], [ enable_sflow="yes" ] )
AM_CONDITIONAL(BUILD_SFLOW, test x"$enable_sflow" = xyes)

dnl ##################################################################
dnl Checks for libraries.
dnl The order of these tests is the *reverse* order of the libraries in
dnl the LIBS variable that is constructed: each new one gets prepended,
dnl not appended.
dnl
AC_CHECK_LIB(pthread, pthread_create)
dnl if test "$ac_cv_lib_pthread_pthread_create" = yes ; then
dnl    CFLAGS="$CFLAGS -D_REENTRANT"
dnl else
AC_CHECK_LIB(pthreads, pthread_create)
dnl    if test "$ac_cv_lib_pthreads_pthread_create" = yes ; then
dnl       CFLAGS="$CFLAGS -D_REENTRANT"
dnl    fi
dnl fi

if test x"$librrdpath" != x && test x"$librrdpath" != xyes; then
  CFLAGS="$CFLAGS -I$librrdpath/include"
  LDFLAGS="$LDFLAGS -L$librrdpath/lib"
fi

if test x"$gmetad" = xyes; then
   AC_CHECK_HEADERS(rrd.h)
   AC_CHECK_LIB(rrd, rrd_create, [echo "The ganglia graphs are created using RRDTool (http://www.rrdtool.org/)"], [], [-lm])
   if test x"$ac_cv_lib_rrd_rrd_create" != xyes; then
      echo "Trying harder by including the X library path"
      unset ac_cv_lib_rrd_rrd_create
      LDFLAGS="$LDFLAGS -Wl,-rpath,/usr/X11R6/lib"
      AC_CHECK_LIB(rrd, rrd_create, [echo "The ganglia graphs are created using RRDTool (http://www.rrdtool.org/)"], [], [-lm])
      if test x"$ac_cv_lib_rrd_rrd_create" != xyes; then
         echo
         echo "The Ganglia Meta Daemon uses the Round-Robin Database Tool (rrdtool)"
         echo "for storing historical information.  You have chosen to compile the"
         echo "monitoring core with gmetad but librrd could not be found.  Please"
         echo "visit http://www.rrdtool.org/, download rrdtool and then try again"
         echo
         exit 1;
      fi
   fi
fi

AC_SUBST(STATIC_USER_MODULES)

if test x"$enable_python" = xyes; then
  echo
  echo Checking for python

  # check for Python executable
  if test -z "$PYTHON_BIN"; then
    AC_PATH_PROG(PYTHON_BIN, python)
  else
    if test -d "$PYTHON_BIN"; then
      PYTHON_BIN="$PYTHON_BIN/bin/python"
    fi
  fi

  if test -n "$PYTHON_BIN"; then
    # find out python version
    AC_MSG_CHECKING(Python version)
    PyVERSION=`$PYTHON_BIN -c ['import sys; print sys.version[:3]'`]
    PyMAJVERSION=`$PYTHON_BIN -c ['import sys; print sys.version[:1]'`]
    AC_MSG_RESULT($PyVERSION)
    PYTHON_VERSION=$PyVERSION
    AC_SUBST(PYTHON_VERSION)

    PyEXEC_INSTALLDIR=`$PYTHON_BIN -c "import sys; print sys.exec_prefix"`
    if test -f "$PyEXEC_INSTALLDIR/include/python/Python.h"; then
      PYTHON_INCLUDES="-I$PyEXEC_INSTALLDIR/include/python"
    else
      if test -f "$PyEXEC_INSTALLDIR/include/python$PyVERSION/Python.h"; then
        PYTHON_INCLUDES="-I$PyEXEC_INSTALLDIR/include/python$PyVERSION"
      else
        PYTHON_INCLUDES=""
        enable_python="no"
      fi
    fi
    AC_SUBST(PYTHON_INCLUDES)
  else
    enable_python="no"
  fi
fi

AC_MSG_CHECKING(Python support)
AC_MSG_RESULT($enable_python)
AM_CONDITIONAL(BUILD_PYTHON, test x"$enable_python" = xyes)

if test x"$enable_perl" = xyes; then
  echo
  echo Checking for perl

  # check for Perl executable
  if test -z "$PERL_BIN"; then
    AC_PATH_PROG(PERL_BIN, perl)
  else
    if test -d "$PERL_BIN"; then
      PERL_BIN="$PERL_BIN/bin/perl"
    fi
  fi

  if test -n "$PERL_BIN" && test -x "$PERL_BIN"; then
    # Check if perl has ExtUtils::Embed module installed
    AC_MSG_CHECKING(for existence of ExtUtils::Embed module)
    `$PERL_BIN -MExtUtils::Embed -e '' >/dev/null 2>&1`
    if test $? -eq 0; then
      AC_MSG_RESULT(yes)
      PERL_INCLUDES=`$PERL_BIN -MExtUtils::Embed -e perl_inc`
      AC_SUBST(PERL_INCLUDES)
      PERL_CFLAGS=`$PERL_BIN -MExtUtils::Embed -e ccopts`
      AC_SUBST(PERL_CFLAGS)
      #CFLAGS="$CFLAGS $PERL_CFLAGS"
      PERL_LDFLAGS=`$PERL_BIN -MExtUtils::Embed -e ldopts`
      AC_SUBST(PERL_LDFLAGS)
    else
      AC_MSG_RESULT(no)
      AC_MSG_ERROR(Module not found, please install or specify --disable-perl to disable mod_perl support)
      exit 1
    fi
  else
    enable_perl="no"
  fi
fi

AC_MSG_CHECKING(Perl support)
AC_MSG_RESULT($enable_perl)
AM_CONDITIONAL(BUILD_PERL, test x"$enable_perl" = xyes)

LIB_SUFFIX=lib
case $host in
x86_64*linux*)
  LIB_SUFFIX=lib64
  ;;
esac

PKG_PROG_PKG_CONFIG

echo
echo Checking for apr
if test x"$libaprpath" != x && test x"$libaprpath" != xyes; then
  if test -f "$libaprpath" ; then
    APR_INCLUDES=`$libaprpath --includes`
    AC_SUBST(APR_INCLUDES)
    APR_LIBPATH=`$libaprpath --link-ld`
    LDFLAGS="$LDFLAGS -L`expr "x$APR_LIBPATH" : '.*-L\(.*\) '`"
    AC_CHECK_LIB(apr-1, apr_socket_send,,,[`$libaprpath --libs`])
  else
    CFLAGS="$CFLAGS -I$libaprpath/include"
    LDFLAGS="$LDFLAGS -L$libaprpath/${LIB_SUFFIX}"
    echo "Added -I$libaprpath/include to CFLAGS"
    echo "Added -L$libaprpath/${LIB_SUFFIX} to LDFLAGS"
    AC_CHECK_LIB(apr-1, apr_socket_send)
  fi

  if test x"$ac_cv_lib_apr_1_apr_socket_send" = xyes; then
    echo "The ganglia utilities use the Apache Portable Runtime functions (http://www.apache.org/)"
    echo "Found a suitable libapr1 library"
  else
    echo "libapr not found"
    exit 1;
  fi
else
  PKG_CHECK_MODULES([APR],[apr-1])
  LDFLAGS="$LDFLAGS $LIBS $APR_LIBS"
  APR_INCLUDES="$APR_CFLAGS"
fi

echo
echo Checking for libmemcached
AC_ARG_WITH( memcached,
[  --with-memcached        enable memcached metrics publishing],
if test x"$withval" = xyes; then memcache="yes"; fi)

if test x"$memcache" == xyes; then
  PKG_CHECK_MODULES(DEPS, libmemcached >= 1.0.2)
  AC_SUBST(DEPS_CFLAGS)
  AC_SUBST(DEPS_LIBS)
  CFLAGS="$CFLAGS -DWITH_MEMCACHED"
fi

AC_ARG_WITH( riemann, AS_HELP_STRING([--with-riemann], [enable riemann metric forwarding]),
if test x"$withval" = xyes; then with_riemann="yes"; fi)

if test x"$with_riemann" == xyes; then
echo
echo Checking for Protocol Buffers

  PKG_CHECK_MODULES([PROTOBUF_C], [libprotobuf-c],[],[AC_MSG_WARN([No pkg-config for protobuf-c.])] )
  if test "x$PROTOBUF_C_CFLAGS" = x; then
    echo "trying harder - bug your distro"
    AC_CHECK_LIB([protobuf-c], [protobuf_c_message_pack])
    if test x"$ac_cv_lib_protobuf_c_protobuf_c_message_pack" = xyes; then
      PROTOBUF_C_LIBS="-lprotobuf-c"
    else
      AC_MSG_ERROR([libprotobuf-c not found])  # package: libprotobuf-c0-dev
    fi
  fi
  AC_SUBST([PROTOBUF_C_CFLAGS])
  AC_SUBST([PROTOBUF_C_LIBS])
  AC_CHECK_PROGS([PROTOC_C], [protoc-c], [:])
  if test "$PROTOC_C" = :; then
    AC_MSG_ERROR([This package needs 'protoc-c' tool. Try installing the 'protobuf-c-compiler' package.])
  fi
  AC_CONFIG_COMMANDS([riemann_pb-c.c], [protoc-c -Igmetad/ --c_out=./gmetad gmetad/riemann.proto])
  CFLAGS="$CFLAGS -DWITH_RIEMANN"
fi
AM_CONDITIONAL(BUILD_RIEMANN, test x"$with_riemann" = xyes)

echo
echo Checking for confuse
if test x"$libconfusepath" != x && test x"$libconfusepath" != xyes; then
  CFLAGS="$CFLAGS -I$libconfusepath/include"
  LDFLAGS="$LDFLAGS -L$libconfusepath/${LIB_SUFFIX}"
  echo "Added -I$libconfusepath/include to CFLAGS"
  echo "Added -L$libconfusepath/${LIB_SUFFIX} to LDFLAGS"
fi
AC_CHECK_LIB(confuse, cfg_parse)
if test x"$ac_cv_lib_confuse_cfg_parse" = xno; then
  echo Trying harder including gettext
  unset ac_cv_lib_confuse_cfg_parse
  LDFLAGS="$LDFLAGS -lintl"
  AC_CHECK_LIB(confuse, cfg_parse)
  if test x"$ac_cv_lib_confuse_cfg_parse" = xno; then
    echo Trying harder including iconv
    unset ac_cv_lib_confuse_cfg_parse
    LDFLAGS="$LDFLAGS -liconv"
    AC_CHECK_LIB(confuse, cfg_parse)
    if test x"$ac_cv_lib_confuse_cfg_parse" = xno; then
      echo "libconfuse not found"
      exit 1
    fi
  fi
fi
if test x"$ac_cv_lib_confuse_cfg_parse" = xyes; then
  echo "Found a suitable Confuse library"
fi

echo
echo Checking for expat
if test x"$libexpatpath" != x && test x"$libexpatpath" != xyes; then
  CFLAGS="$CFLAGS -I$libexpatpath/include"
  LDFLAGS="$LDFLAGS -L$libexpatpath/${LIB_SUFFIX}"
  echo "Added -I$libexpatpath/include to CFLAGS"
  echo "Added -L$libexpatpath/${LIB_SUFFIX} to LDFLAGS"
fi
AC_CHECK_LIB(expat, XML_ParserCreate)
if test x"$ac_cv_lib_expat_XML_ParserCreate" = xyes; then
  echo "Found a suitable Expat library"
else
  echo "libexpat not found"
  exit 1;
fi

echo
echo Checking for pcre
if test x"$libpcrepath" != x && test x"$libpcrepath" != xyes; then
  CFLAGS="$CFLAGS -I$libpcrepath/include"
  LDFLAGS="$LDFLAGS -L$libpcrepath/${LIB_SUFFIX}"
  echo "Added -I$libpcrepath/include to CFLAGS"
  echo "Added -L$libpcrepath/${LIB_SUFFIX} to LDFLAGS"
fi
if test x"$libpcre" == xyes ; then
  AC_CHECK_HEADERS([pcre/pcre.h pcre.h])
  AC_CHECK_LIB(pcre, pcre_compile)
  if test x"$ac_cv_lib_pcre_pcre_compile" = xyes; then
    echo "Found a suitable pcre library"
  else
    echo "libpcre not found, specify --with-libpcre=no to build without PCRE support"
    exit 1;
  fi
else
  echo "building without PCRE support"
fi
echo

echo
echo Checking for systemd
AC_ARG_WITH([systemdsystemunitdir],
  AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
  [], [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
if test "x$with_systemdsystemunitdir" != xno; then
  AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
else
  AC_SUBST([systemdsystemunitdir], [/usr/lib/systemd/system])
fi
AM_CONDITIONAL(HAVE_SYSTEMD, [test -n "$with_systemdsystemunitdir" -a "x$with_systemdsystemunitdir" != xno ])

echo
echo Checking for zlib
AC_ARG_WITH([zlib],
  AS_HELP_STRING([--with-zlib=DIR], [Specify location for zlib]),
  [if test x"$withval" != xno; then libzlib="yes"; libzlibpath="$withval"; fi])
if test x"$libzlibpath" != x && test x"$libzlibpath" != xyes; then
  CFLAGS="$CFLAGS -I$libzlibpath/include"
  CPPFLAGS="$CPPFLAGS -I$libzlibpath/include"
  LDFLAGS="$LDFLAGS -L$libzlibpath/${LIB_SUFFIX}"
  echo "Added -I$libzlibpath/include to CFLAGS and CPPFLAGS"
  echo "Added -L$libzlibpath/${LIB_SUFFIX} to LDFLAGS"
fi
AC_CHECK_HEADERS([zlib.h])
AC_CHECK_LIB(z, deflate)
if test x"$ac_cv_lib_z_deflate" != xyes; then
  echo "zlib library not configured properly"; exit 1;
fi
echo "Found a suitable zlib"

echo


AC_CHECK_LIB(rpc, xdr_string)
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, inet_addr)
AC_CHECK_LIB(nsl, gethostbyaddr_r)
AC_CHECK_FUNC(inet_aton, , [AC_CHECK_LIB(resolv, inet_aton)])
AC_CHECK_LIB(kstat, kstat_open)
AC_CHECK_LIB(kvm, kvm_open)
AC_CHECK_LIB(odm, odm_initialize)
AC_CHECK_LIB(cfg, _system_configuration)
AC_CHECK_LIB(perfstat, perfstat_cpu_total)
AC_CHECK_LIB(dl, dlopen)
dnl AC_CHECK_LIB(crypto, RSA_sign)
dnl if test "$ac_cv_lib_crypto_RSA_sign" = no; then
dnl    echo "$PACKAGE $VERSION requires IPv4 OpenSSL."
dnl    echo "See http://www.openssl.org/."
dnl    exit 1;
dnl fi

dnl ##################################################################
dnl Checks for header files.
dnl

dnl Check for stdlib.h stdarg.h string.h float.h
AC_HEADER_STDC

AC_CHECK_HEADERS(syslog.h pthread.h fcntl.h signal.h sys/time.h sys/types.h sys/stat.h sys/socket.h sys/ioctl.h netinet/in.h arpa/inet.h netinet/tcp.h unistd.h stropts.h sys/sockio.h ctype.h errno.h netdb.h stdio.h sys/uio.h sys/wait.h sys/un.h sys/select.h sys/filio.h getopt.h net/if_dl.h net/raw.h poll.h)
AC_CHECK_HEADER([net/if.h], [], [],
[#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
])

AC_CHECK_HEADERS(inttypes.h limits.h sys/param.h sys/vfs.h sys/statfs.h sys/statvfs.h)
AC_CHECK_HEADER([sys/mount.h], [], [],
[#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
])

AC_CHECK_HEADERS(rpc/types.h)
AC_CHECK_HEADER([rpc/xdr.h], [],
   [AC_MSG_ERROR([your system is missing the Sun RPC (ONC/RPC) libraries])],
[#ifdef HAVE_RPC_TYPES_H
# include <rpc/types.h>
#endif
])

dnl ##################################################################
dnl Checks for typedefs.
dnl
dnl AC_TYPE_SIZE_T
AC_HEADER_TIME
dnl AC_SEARCH_TYPE is a custom macro in acinclude.m4
AC_SEARCH_TYPE(uchar_t,   unsigned char)
AC_SEARCH_TYPE(int8_t,    signed char)
AC_SEARCH_TYPE(uint8_t,   unsigned char)
AC_SEARCH_TYPE(int16_t,   signed short)
AC_SEARCH_TYPE(uint16_t,  unsigned short)
AC_SEARCH_TYPE(int32_t,   signed int)
AC_SEARCH_TYPE(uint32_t,  unsigned int)
AC_SEARCH_TYPE(size_t,    unsigned int)
AC_SEARCH_TYPE(ssize_t,   signed int)
AC_SEARCH_TYPE(socklen_t, unsigned int)
AC_SEARCH_TYPE(time_t,    long)

dnl AC_CHECK_SIZEOF(char)
dnl AC_CHECK_SIZEOF(short)
dnl AC_CHECK_SIZEOF(int)
dnl AC_CHECK_SIZEOF(long)
dnl AC_CHECK_SIZEOF(void *)

dnl ##################################################################
dnl Check for structure definitions.
dnl

dnl ##################################################################
dnl Check the compiler characteristics
dnl
dnl Don't need this now that all data is send in XDR
AC_C_BIGENDIAN

dnl ##################################################################
dnl Checks for library functions.
dnl
dnl AC_FUNC_MEMCMP
AC_CHECK_FUNCS([snprintf vsnprintf strlcat])

dnl ##################################################################
dnl Check for function prototypes in headers.
dnl
AC_CHECK_FUNCS(getopt_long inet_ntop strdup)
dnl AC_CHECK_FUNC(getopt_long)
dnl AC_CHECK_FUNC(pthread_rwlock_rdlock,
dnl   [AC_DEFINE(HAVE_PTHREAD_RWLOCK, 1)])

dnl ##################################################################
dnl Check for system services.

dnl ##################################################################
dnl If the compiler is gcc, enable all warnings.  Main purpose is to
dnl catch any function call where the function has not been prototyped.
dnl
dnl Add "-Wcast-qual again later"...
dnl if test "$ac_cv_prog_gcc" = yes; then
if test x"$ac_cv_gcc" = xyes; then
   CFLAGS="$CFLAGS -Wall -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes"
fi

dnl disable type punning optimization until verified safe
if test x"$ac_cv_c_compiler_gnu" = xyes; then
   SAVE_CFLAGS="$CFLAGS"
   CFLAGS="$CFLAGS -fno-strict-aliasing"
   AC_MSG_CHECKING([whether disabling type punning in gcc is possible])
   AC_TRY_COMPILE([], [], type_punning="yes", type_punning="no")
   if test x"$type_punning" = xno; then
      CFLAGS="$SAVE_CFLAGS"
   fi
   AC_MSG_RESULT($type_punning)
   unset type_punning
   unset SAVE_CFLAGS
   CFLAGS="$CFLAGS -Wall"
fi

if test x"$enable_memcheck" = xyes; then
   echo "Setting up for memory checking via mpatrol"
   CFLAGS="$CFLAGS -fcheck-memory-usage "
   LDFLAGS="$LDFLAGS -lmpatrol -lbfd -liberty "
fi

AC_MSG_CHECKING([for __thread specifier])
AC_TRY_COMPILE([static __thread int a;],[int b = a;],
	       [AC_DEFINE(HAVE___THREAD, 1, HAVE___THREAD)
	        AC_MSG_RESULT([exists, using native TLS])],
	       [AC_MSG_RESULT([not found, defaulting to pthreads for TLS])])

dnl Some system-specific stuff ...
dnl Some operating systems require additional flags in order to get all
dnl the definitions that we're looking for in some system headers.

dnl default VARSTATEDIR to /var/lib since that's the traditional location.
dnl
varstatedir="/var/lib"

# Used when static linking requested
EXPORT_SYMBOLS="-export-dynamic"
# Used when dynamic linking requested
EXPORT_SYMBOLS_DYNAMIC="-export-dynamic"
case "$host" in
*linux*)
		CFLAGS="$CFLAGS -D_REENTRANT"
		AC_DEFINE(LINUX, 1, LINUX)
dnl
dnl For fsusage.c - disk usage.
dnl
		AC_DEFINE(STAT_STATVFS, 1, STAT_STATVFS)
		AC_DEFINE(SUPPORT_GEXEC, 1, SUPPORT_GEXEC)
		if test x"$moduledir" = x"\${exec_prefix}/lib/ganglia"; then
		   if test x"$build_cpu" != xia64 && \
		      test x"$build_cpu" != xalpha; then
		      if test x"$libdir" = x"\${exec_prefix}/lib"; then
		         if test x"$exec_prefix" = xNONE; then
		            if test x"$prefix" = xNONE; then
		               libroot="$ac_default_prefix"
		            else
		               libroot="$prefix"
		            fi
		         else
		            libroot="$exec_prefix"
			 fi
	                 AC_TRY_COMPILE([], [],
		                        [file conftest.$ac_objext > conftest.file])
		         FILE=`cat conftest.file`
		         WIDTH_CPU=`expr "x$FILE" : '.*ELF \(.*\)-bit '`
		         rm -f conftest.file
		         if test x"$WIDTH_CPU" = x64; then
		            libdir="$libroot/lib64"
		         else
		            libdir="$libroot/lib"
		         fi
		      fi
		      moduledir="$libdir/ganglia"
		   fi
		fi
		;;
*ia64-*hpux*)	CFLAGS="$CFLAGS -D_PSTAT64 -D_HPUX_SOURCE"
		LIBS="-lpthread $LIBS"
		EXPORT_SYMBOLS="-Wl,-E"
		AC_DEFINE(SUPPORT_GEXEC, 0, SUPPORT_GEXEC)
		AC_DEFINE(IA64,1,IA64)
		AC_DEFINE(HPUX,1,HPUX)
		;;
*aix*)		CFLAGS="$CFLAGS -D_ALL_SOURCE -DAIX"
		EXPORT_SYMBOLS="-Wl,-bexpfull"
		AC_DEFINE(SUPPORT_GEXEC, 0, SUPPORT_GEXEC)
		if test x"$ac_cv_lib_perfstat_perfstat_cpu_total" = xyes; then
		   CFLAGS="$CFLAGS -DHAVE_PERFSTAT"
		else
		   echo "libperfstat not detected."
		   echo "For AIX 5 you need to install bos.perf.libperfstat"
		fi
		AC_DEFINE(AIX, 1, AIX)
		LIBS="-lm $LIBS"
		EXPORT_SYMBOLS_DYNAMIC="-Wl,-bexpfull"
		;;
*hpux*)		CFLAGS="$CFLAGS -D_HPUX_SOURCE"
		LIBS="-lpthread $LIBS"
		EXPORT_SYMBOLS="-Wl,-E"
		AC_DEFINE(SUPPORT_GEXEC, 0, SUPPORT_GEXEC)
		AC_DEFINE(HPUX,1,HPUX)
		;;
*irix*)		AC_DEFINE(SUPPORT_GEXEC, 0, SUPPORT_GEXEC)
		CFLAGS="$CFLAGS -D_IRIX_SOURCE"
		AC_DEFINE(IRIX,1,IRIX)
		;;
*osf*)		metric_source="osf.c"
		CFLAGS="$CFLAGS -D_SOCKADDR_LEN"
		AC_DEFINE(OSF,1,OSF)
		;;
*darwin*) 	AC_DEFINE(DARWIN,1,DARWIN)
		EXPORT_SYMBOLS="-export-all-symbols"
		;;
*solaris*)	AC_DEFINE(SUPPORT_GEXEC, 0, SUPPORT_GEXEC)
		CFLAGS="$CFLAGS -D__EXTENSIONS__ -DHAVE_STRERROR"
		if test x"$ac_cv_prog_cc_c99" = x -o x"$ac_cv_prog_cc_c99" = xno; then
		   CFLAGS="$CFLAGS -D_POSIX_C_SOURCE=199506L"
		else
		   CFLAGS="$CFLAGS -D_POSIX_C_SOURCE=200112L"
		fi
		if test x"$ac_cv_prog_gcc" != xyes; then
		   LIBS="-lm $LIBS"
		fi
		AC_DEFINE(SOLARIS,1,SOLARIS)
		;;
*freebsd*)	CFLAGS="-pthread $CFLAGS -D_REENTRANT"
		AC_DEFINE(SUPPORT_GEXEC, 0, SUPPORT_GEXEC)
		varstatedir="/var/db"
		osreldate=`awk '/^\#define[[:space:]]*__FreeBSD_version/ { print $3 }' /usr/include/osreldate.h`
		if test $osreldate -lt 502102; then
		   CFLAGS="-pthread $CFLAGS -D_REENTRANT"
		fi
		AC_DEFINE(FREEBSD, 1, FREEBSD)
		;;
*dragonfly*)	CFLAGS="-pthread $CFLAGS -D_REENTRANT"
		AC_DEFINE(SUPPORT_GEXEC, 0, SUPPORT_GEXEC)
		varstatedir="/var/db"
		AC_DEFINE(DRAGONFLYBSD, 1, DRAGONFLYBSD)
		;;
*netbsd*)	AC_DEFINE(SUPPORT_GEXEC, 0, SUPPORT_GEXEC)
		varstatedir="/var/db"
		AC_DEFINE(NETBSD, 1, NETBSD)
		;;
*openbsd*)	AC_DEFINE(SUPPORT_GEXEC, 0, SUPPORT_GEXEC)
		varstatedir="/var/db"
		AC_DEFINE(OPENBSD, 1, OPENBSD)
		;;
*cygwin*)	LDFLAGS="-L/bin"
		EXPORT_SYMBOLS="-export-all-symbols"
		AC_DEFINE(CYGWIN, 1, CYGWIN)
esac

AC_SUBST(EXPORT_SYMBOLS)
AC_SUBST(EXPORT_SYMBOLS_DYNAMIC)

dnl Define VARSTATEDIR in config.h
dnl
AC_SUBST(varstatedir)
AC_DEFINE_UNQUOTED(VARSTATEDIR, "$varstatedir", VARSTATEDIR)

dnl Where to look for libraries (/usr/lib vs /usr/lib64)
AC_SUBST(libdir)

if test x"$moduledir" = x"\${exec_prefix}/lib/ganglia"; then
   if test x"$exec_prefix" = xNONE; then
      if test x"$prefix" = xNONE; then
         exec_prefix="$ac_default_prefix"
      else
         exec_prefix="$prefix"
      fi
   fi
   moduledir=`eval echo "$moduledir"`
fi
AC_DEFINE_UNQUOTED(GANGLIA_MODULE_DIR, "$moduledir", GANGLIA_MODULE_DIR)
AC_SUBST(moduledir)

dnl This is for gmetad-python, where @gsysconfdir@ needs to be
dnl substituted into setup.py during the ./configure step as there is no
dnl make install step
dnl
if test x"$sysconfdir" = x"\${prefix}/etc"; then
   if test x"$prefix" = xNONE; then
      prefix="$ac_default_prefix"
   else
      prefix="$prefix"
   fi
fi
gsysconfdir=`eval echo "$sysconfdir"`
AC_DEFINE_UNQUOTED(GSYSCONFDIR, "$gsysconfdir", GSYSCONFDIR)
AC_SUBST(gsysconfdir)

AC_CONFIG_FILES([scripts/fixconfig], [chmod +x scripts/fixconfig])

AC_OUTPUT(Makefile
          tests/Makefile
          ganglia.spec
          gmetad/Makefile
          lib/default_conf.h
          lib/Makefile
          gmond/Makefile
          gstat/Makefile
          gmond/modules/Makefile
          gmond/modules/example/Makefile
          gmond/modules/cpu/Makefile
          gmond/modules/disk/Makefile
          gmond/modules/memory/Makefile
          gmond/modules/network/Makefile
          gmond/modules/perl/Makefile
          gmond/modules/python/Makefile
          gmond/modules/status/Makefile
          gmond/modules/system/Makefile
          gmond/perl_modules/Makefile
          gmond/perl_modules/example/Makefile
          gmond/python_modules/Makefile
          gmond/python_modules/apache_status/Makefile
          gmond/python_modules/cpu/Makefile
          gmond/python_modules/db/Makefile
          gmond/python_modules/disk/Makefile
          gmond/python_modules/example/Makefile
          gmond/python_modules/memcached/Makefile
          gmond/python_modules/memory/Makefile
          gmond/python_modules/nfs/Makefile
          gmond/python_modules/network/Makefile
          gmond/python_modules/process/Makefile
          gmond/python_modules/ssl/Makefile
          gmond/python_modules/varnish/Makefile
          gmond/python_modules/vm_stats/Makefile
          gmond/python_modules/xen/Makefile
          gmetad/conf.c
          gmetad-python/setup.py
          gmetad-python/Gmetad/gmetad_config.py
          gmetad-python/gmetad-python.conf
          gmetad-python/plugins/rrd_plugin.py
          gmetric/Makefile
          include/Makefile
          mans/Makefile
          )

echo ""
echo "Welcome to.."
echo "     ______                  ___"
echo "    / ____/___ _____  ____ _/ (_)___ _"
echo "   / / __/ __ \`/ __ \/ __ \`/ / / __ \`/"
echo "  / /_/ / /_/ / / / / /_/ / / / /_/ /"
echo "  \____/\__,_/_/ /_/\__, /_/_/\__,_/"
echo "                   /____/"
echo ""
echo "Copyright (c) 2005 University of California, Berkeley"
echo ""
echo "Version: $GANGLIA_VERSION"
echo "Library: Release $LT_RELEASE $LT_CURRENT:$LT_REVISION:$LT_AGE"
echo ""
echo "Type \"make\" to compile."

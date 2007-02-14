dnl
dnl Copyright goes here.
dnl
AC_INIT(auth.h)

dnl
dnl Sets PACKAGE and VERSION defines
dnl
AM_INIT_AUTOMAKE(gexec-authd,0.2.2)

dnl
dnl Creates config.h file with #defines instead of passing to compiler.
dnl
AM_CONFIG_HEADER(config.h)

dnl
dnl Change default installation prefix
dnl
AC_PREFIX_DEFAULT(/usr/local)

dnl
dnl Checks for programs.
dnl
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl
dnl Optional modules
dnl 

dnl
dnl Configurable parameters
dnl

dnl
dnl Checks for libraries.
dnl
AC_CHECK_LIB(e,llist_create,[],[AC_MSG_ERROR([
***
*** libe could not be found!
***])])

dnl
dnl Checks for header files.
dnl
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h unistd.h)

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_TYPE_UID_T

dnl
dnl Checks for library functions.
dnl
AC_TYPE_SIGNAL

dnl
dnl Generate Makefiles (and other files) with substitutions.
dnl
AC_OUTPUT(Makefile \
          gexec-authd.spec \
          config/gexec-authd, chmod 755 config/gexec-authd)
 


dnl
dnl Copyright goes here.
dnl
AC_INIT(e_error.c)

dnl
dnl Sets PACKAGE and VERSION defines
dnl
AM_INIT_AUTOMAKE(libe,0.3.0)

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
AC_PROG_RANLIB

dnl
dnl Optional modules
dnl 

dnl
dnl Configurable parameters
dnl

dnl
dnl Checks for libraries.
dnl

dnl
dnl Checks for header files.
dnl
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/time.h unistd.h)

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_TYPE_SIZE_T

dnl
dnl Checks for library functions.
dnl
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(gethostname socket)

dnl
dnl Generate Makefiles (and other files) with substitutions.
dnl
AC_OUTPUT(Makefile \
          test/Makefile \
          libe.spec)

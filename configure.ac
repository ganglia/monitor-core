dnl
dnl Copyright goes here.
dnl
AC_INIT(gexec.h)

dnl
dnl Sets PACKAGE and VERSION defines
dnl
AM_INIT_AUTOMAKE(gexec,0.3.8)

dnl
dnl Creates config.h file with #defines instead of passing to compiler.
dnl
AM_CONFIG_HEADER(config.h)

dnl
dnl Change default installation prefix
dnl
AC_PREFIX_DEFAULT(/usr)

dnl
dnl Checks for programs.
dnl
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_AWK
AC_PROG_LN_S

dnl
dnl Optional modules (static linking for libganglia for now)
dnl
AC_SUBST(libdir) 
AC_ARG_ENABLE(ganglia,
[  --enable-ganglia  Enable use of Ganglia for node selection.],
[ if test "$enableval" = "yes" ; then
     echo "Use of Ganglia for node selection enabled"
     AC_DEFINE(GANGLIA)
     GANGLIA_LIB="$libdir/libganglia.a"
  else
     GANGLIA_LIB=""
  fi
])
AC_SUBST(GANGLIA_LIB)

dnl
dnl Configurable parameters
dnl

dnl
dnl Checks for libraries.
dnl
AC_CHECK_LIB([auth -le -lssl], auth_init_credentials,[],[AC_MSG_ERROR(libauth not found)])
AC_CHECK_LIB(crypto, CRYPTO_free,[],[AC_MSG_ERROR(libcrypto test failed)])
AC_CHECK_LIB([e -lpthread], barrier_create,[],[AC_MSG_ERROR(libe not found)])
AC_CHECK_LIB(pthread, pthread_create,[],[AC_MSG_ERROR(pthread_create failed)])
AC_CHECK_LIB(ssl, RSA_free,[],[AC_MSG_ERROR(libssl RSA_free failed)])

dnl
dnl Checks for header files.
dnl
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(sys/time.h unistd.h)

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_TYPE_UID_T
AC_TYPE_PID_T
AC_HEADER_TIME

dnl
dnl Checks for library functions.
dnl
AC_FUNC_GETPGRP
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(gettimeofday putenv select)

dnl
dnl Generate Makefiles with substitutions.
dnl
AC_OUTPUT(Makefile     \
          config/gexecd \
          gexec.spec)
   

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
#AC_PREREQ(2.61)
AC_INIT(libmetrics.c)
AM_CONFIG_HEADER(config.h)
AC_CONFIG_AUX_DIR(build)
DATE_VERSION=`date +%Y%m%d%H%M`
AM_INIT_AUTOMAKE("libmetrics", $DATE_VERSION)

AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB(kinfo, kinfo_get_cpus)
AC_CHECK_LIB(kstat, kstat_open)
AC_CHECK_LIB(kvm, kvm_open)
AC_CHECK_LIB(odm, odm_initialize)
AC_CHECK_LIB(cfg, _system_configuration)
AC_CHECK_LIB(perfstat, perfstat_cpu_total)
AC_HAVE_LIBRARY(socket)
AC_HAVE_LIBRARY(resolv)
AC_HAVE_LIBRARY(nsl)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h nlist.h paths.h stdlib.h strings.h sys/filsys.h sys/fs/s5param.h sys/mount.h sys/param.h sys/socket.h sys/statfs.h sys/statvfs.h sys/systeminfo.h sys/time.h sys/vfs.h unistd.h utmp.h sys/sockio.h])

# Checks for typedefs, structures, and compiler characteristics.

dnl ##################################################################
dnl Check if sockaddr{} has sa_len member.
dnl
AC_CACHE_CHECK(if sockaddr{} has sa_len member, ac_cv_sockaddr_has_sa_len,
        AC_TRY_COMPILE([
#               include <sys/types.h>
#               include <sys/socket.h>],
                [unsigned int i = sizeof(((struct sockaddr *)0)->sa_len)],
        ac_cv_sockaddr_has_sa_len=yes,
        ac_cv_sockaddr_has_sa_len=no))
if test $ac_cv_sockaddr_has_sa_len = yes ; then
        AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, [We have a len attr in sockaddr])
fi

AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

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

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_CHECK_FUNCS([getmntinfo getpagesize gettimeofday memset pstat_getdynamic strchr strdup strndup strpbrk strstr strtol strtoull sysinfo uname vsnprintf strlcat])

if test x"$ac_cv_c_compiler_gnu" = xyes; then
   CFLAGS="$CFLAGS -Wall"
fi

OS="unknown"
case "$host" in
*linux*)
		GEXEC=1
		OS="linux"
		AC_DEFINE(LINUX, 1, LINUX)
dnl
dnl For fsusage.c - disk usage.
dnl
                AC_DEFINE(STAT_STATVFS, 1, STAT_STATVFS)
		;;
*ia64-*hpux*)	OS="hpux"
		CFLAGS="$CFLAGS -D_PSTAT64 -D_HPUX_SOURCE"
		AC_DEFINE(IA64, 1, IA64)
		AC_DEFINE(HPUX, 1, HPUX)
		;;
*aix*)		OS="aix"
		CFLAGS="$CFLAGS -D_ALL_SOURCE -DAIX"
		LDFLAGS="$LDFLAGS -Wl,-bexpfull"
		if test "$ac_cv_lib_perfstat_perfstat_cpu_total" = yes ; then
			CFLAGS="$CFLAGS -DHAVE_PERFSTAT"
		else
			echo "libperfstat not detected."
			echo "For AIX 5 you need to install bos.perf.libperfstat"
		fi
		AC_DEFINE(AIX, 1, AIX)
                ;;
*hpux*)		OS="hpux"
		CFLAGS="$CFLAGS -D_HPUX_SOURCE"
		AC_DEFINE(HPUX, 1, HPUX)
		;;
*irix*)		OS="irix"
		CFLAGS="$CFLAGS -D_IRIX_SOURCE"
		AC_DEFINE(IRIX, 1, IRIX)
		;;
*osf*)		OS="osf"
		CFLAGS="$CFLAGS -D_SOCKADDR_LEN"
		AC_DEFINE(OSF, 1, OSF)
		;;
*darwin*)	OS="darwin"
		AC_DEFINE(DARWIN, 1, DARWIN)
		;;
*solaris*)	OS="solaris"
		CFLAGS="$CFLAGS -DHAVE_STRERROR"
		AC_DEFINE(SOLARIS, 1, SOLARIS)
		;;
*freebsd*)	OS="freebsd"
		AC_DEFINE(FREEBSD, 1, FREEBSD)
		;;
*dragonfly*)	OS="dragonfly"
		AC_DEFINE(DRAGONFLYBSD, 1, DRAGONFLYBSD)
		;;
*netbsd*)	OS="netbsd"
		AC_DEFINE(NETBSD, 1, NETBSD)
		;;
*openbsd*)	OS="openbsd"
		AC_DEFINE(OPENBSD, 1, OPENBSD)
		;;
*cygwin*)      	OS="cygwin"
		AC_DEFINE(CYGWIN, 1, CYGWIN)
		;;
*mingw*)	OS="mingw"
		AC_DEFINE(MINGW, 1, MINGW)
		;;
esac
AC_SUBST(OS)

AC_CONFIG_FILES([Makefile
                 tests/Makefile
                 aix/Makefile
                 cygwin/Makefile
                 darwin/Makefile
                 dragonfly/Makefile
                 freebsd/Makefile
                 hpux/Makefile
                 irix/Makefile
                 linux/Makefile
                 mingw/Makefile
                 netbsd/Makefile
                 openbsd/Makefile
                 osf/Makefile
                 solaris/Makefile])
AC_OUTPUT

dnl
dnl Check for the Linux /proc filesystem
dnl
dnl usage:  AC_DNET_LINUX_PROCFS
dnl results:   HAVE_LINUX_PROCFS
dnl
AC_DEFUN([AC_DNET_LINUX_PROCFS],
    [AC_MSG_CHECKING(for Linux proc filesystem)
    AC_CACHE_VAL(ac_cv_dnet_linux_procfs,
   if test "x`cat /proc/sys/kernel/ostype 2>&-`" = "xLinux" ; then
       ac_cv_dnet_linux_procfs=yes
        else
       ac_cv_dnet_linux_procfs=no
   fi)
    AC_MSG_RESULT($ac_cv_dnet_linux_procfs)
    if test $ac_cv_dnet_linux_procfs = yes ; then
   AC_DEFINE(HAVE_LINUX_PROCFS, 1,
        [Define if you have the Linux /proc filesystem.])
    fi])

dnl
dnl Check for 4.4 BSD sa_len member in sockaddr struct
dnl
dnl usage:  AC_DNET_SOCKADDR_SA_LEN
dnl results:   HAVE_SOCKADDR_SA_LEN (defined)
dnl
AC_DEFUN([AC_DNET_SOCKADDR_SA_LEN],
    [AC_MSG_CHECKING(for sa_len in sockaddr struct)
    AC_CACHE_VAL(ac_cv_dnet_sockaddr_has_sa_len,
        AC_TRY_COMPILE([
#       include <sys/types.h>
#       include <sys/socket.h>],
        [u_int i = sizeof(((struct sockaddr *)0)->sa_len)],
        ac_cv_dnet_sockaddr_has_sa_len=yes,
        ac_cv_dnet_sockaddr_has_sa_len=no))
    AC_MSG_RESULT($ac_cv_dnet_sockaddr_has_sa_len)
    if test $ac_cv_dnet_sockaddr_has_sa_len = yes ; then
            AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1,
                      [Define if sockaddr struct has sa_len.])
    fi])

dnl
dnl ##################################################
dnl the AC_CHECK_TYPE macro only checks for a type in 
dnl <sys/types.h>, <stdlib.h> and <stddef.h>. this macro
dnl in more generic and allows for arbitrary headers to
dnl be included in the search
dnl
dnl
AC_DEFUN([AC_SEARCH_TYPE],
        [AC_MSG_CHECKING(if $1 defined)
        AC_CACHE_VAL(ac_cv_type_$1,
                AC_TRY_COMPILE(
[
#include        "confdefs.h"    /* the header built by configure so far */
#ifdef HAVE_STDINT_H
#  include      <stdint.h>
#endif
#ifdef  HAVE_INTTYPES_H
#  include      <inttypes.h>
#endif
#ifdef  HAVE_SYS_TYPES_H
#  include      <sys/types.h>
#endif
#ifdef  HAVE_SYS_SOCKET_H
#  include      <sys/socket.h>
#endif
#ifdef  HAVE_SYS_TIME_H
#  include    <sys/time.h>
#endif
#ifdef  HAVE_NETINET_IN_H
#  include    <netinet/in.h>
#endif
#ifdef  HAVE_ARPA_INET_H
#  include    <arpa/inet.h>
#endif
#ifdef  HAVE_ERRNO_H
#  include    <errno.h>
#endif
#ifdef  HAVE_FCNTL_H
#  include    <fcntl.h>
#endif
#ifdef  HAVE_NETDB_H
#  include      <netdb.h>
#endif
#ifdef  HAVE_SIGNAL_H
#  include      <signal.h>
#endif
#ifdef  HAVE_STDIO_H
#  include      <stdio.h>
#endif
#ifdef  HAVE_STDLIB_H
#  include      <stdlib.h>
#endif
#ifdef  HAVE_STRING_H
#  include      <string.h>
#endif
#ifdef  HAVE_SYS_STAT_H
#  include      <sys/stat.h>
#endif                                  
#ifdef  HAVE_SYS_UIO_H
#  include      <sys/uio.h>
#endif
#ifdef  HAVE_UNISTD_H
#  include      <unistd.h>
#endif
#ifdef  HAVE_SYS_WAIT_H
#  include      <sys/wait.h>
#endif
#ifdef  HAVE_SYS_UN_H
#  include      <sys/un.h>
#endif
#ifdef  HAVE_SYS_SELECT_H
# include   <sys/select.h>
#endif
#ifdef  HAVE_STRINGS_H
# include   <strings.h>
#endif
#ifdef  HAVE_SYS_IOCTL_H
# include   <sys/ioctl.h>
#endif
#ifdef  HAVE_SYS_FILIO_H
# include   <sys/filio.h>
#endif
#ifdef  HAVE_SYS_SOCKIO_H
# include   <sys/sockio.h>
#endif
#ifdef  HAVE_PTHREAD_H
#  include      <pthread.h>
#endif],
                [ $1 foo ],
                ac_cv_type_$1=yes,
                ac_cv_type_$1=no))
        AC_MSG_RESULT($ac_cv_type_$1)
        if test $ac_cv_type_$1 = no ; then
                AC_DEFINE($1, $2, $1)
        fi
])       

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([acknowledge_packet.c])
AM_INIT_AUTOMAKE([soliloque-server], [0.6.2])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile control_packets/Makefile database/Makefile])

# Checks for programs.
AC_LANG(C)
AC_PROG_CC(clang llvm-gcc gcc cc)
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([dbi],[dbi_conn_new])
AC_CHECK_LIB([config],[config_read])
AC_CHECK_LIB([crypto],[SHA256])
AC_CHECK_LIB([pthread],[pthread_create])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h inttypes.h limits.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h])
AC_CHECK_HEADERS([dbi/dbi.h libconfig.h openssl/sha.h pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_HEADER_TIME
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_REALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero gettimeofday inet_ntoa socket strdup strerror strndup uname])

AC_OUTPUT

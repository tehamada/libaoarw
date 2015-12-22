#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([libaoarw], [0.0.1], [])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/libaoarw.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([string.h unistd.h])
AC_CHECK_HEADER([libusb-1.0/libusb.h],,AC_MSG_ERROR([libusb.h not found]))

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SSIZE_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Soundb0ard], [1.0], [up@yerbum.com])
AC_CONFIG_SRCDIR([include/filter_moogladder.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lgtest':
AC_CHECK_LIB([gtest], [main])
# FIXME: Replace `main' with a function in `-llo':
AC_CHECK_LIB([lo], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lportaudio':
AC_CHECK_LIB([portaudio], [main])
# FIXME: Replace `main' with a function in `-lportmidi':
AC_CHECK_LIB([portmidi], [main])
# FIXME: Replace `main' with a function in `-lprofiler':
AC_CHECK_LIB([profiler], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])
# FIXME: Replace `main' with a function in `-lreadline':
AC_CHECK_LIB([readline], [main])
# FIXME: Replace `main' with a function in `-lsndfile':
AC_CHECK_LIB([sndfile], [main])

# Checks for header files.
AC_CHECK_HEADERS([locale.h stdint.h stdlib.h string.h strings.h sys/time.h termios.h unistd.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([floor getcwd memset pow regcomp setlocale strcasecmp])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

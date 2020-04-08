#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Soundb0ard], [1.0], [up@yerbum.com])
AC_CONFIG_SRCDIR([include/filter_moogladder.h])
AC_CONFIG_HEADERS([config.h])

AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
#AC_CHECK_LIB([gtest], [main], AC_MSG_NOTICE(Looking for GTest.), AC_MSG_ERROR(cannot find GTest))
#AC_CHECK_LIB([m], [main], AC_MSG_NOTICE(Looking for Math.), AC_MSG_ERROR(cannot find Math))
#AC_CHECK_LIB([portaudio], [Pa_CountDevices], AC_MSG_NOTICE(Looking for PortAudio.), AC_MSG_ERROR(cannot find PortAudio))
#AC_CHECK_LIB([portmidi], [main],AC_MSG_NOTICE(Looking for PortMidi.), AC_MSG_ERROR(cannot find PortMidi))
#AC_CHECK_LIB([profiler], [main],AC_MSG_NOTICE(Looking for Profiler.), AC_MSG_ERROR(cannot find Profiler))
#AC_CHECK_LIB([pthread], [main],AC_MSG_NOTICE(Looking for PThread.), AC_MSG_ERROR(cannot find PThread))
#AC_CHECK_LIB([readline], [main],AC_MSG_NOTICE(Looking for Readline.), AC_MSG_ERROR(cannot find Readline))
#AC_CHECK_LIB([sndfile], [main]AC_MSG_NOTICE(Looking for SndFile.), AC_MSG_ERROR(cannot find SndFile))

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

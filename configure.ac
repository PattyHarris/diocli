#                                               -*- Autoconf -*-
# DioCLI: Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([DioCLI], [source-1.1.3], [patty@diomedestorage.com])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([DioCLI/ConsoleControl.cpp])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AM_PROG_CC_C_O

# Doxygen support
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
#
DX_INIT_DOXYGEN($PACKAGE_NAME, ./Documentation/linuxDoxyfile.cfg)
#

# Checks for header files (1).
AC_HEADER_STDC

# Checks for command line options

# Configure options: --enable-debug[=no].
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
    [enable debug code (default is no)])],
  [debug="$enableval"], [debug=no])

# Checks for header files (2).
AC_CHECK_HEADERS([stdlib.h])

# Cure annoying default arguments for CFLAGS and CXXFLAGS
if test x"$CFLAGS" = x"-g -O2" ; then
    CFLAGS=""
fi
if test x"$CXXFLAGS" = x"-g -O2" ; then
    CXXFLAGS=""
fi

#====================================================================================
# Check for Boost date and time
#
AX_BOOST_BASE([1.35])
AX_BOOST_DATE_TIME

if test -z "$BOOST_DATE_TIME_LIB"; then
    AC_MSG_ERROR([Unable to find Boost.DateTime library, currently this is required.
Try using --with-boost=<path>, 
E.g., --with-boost=/usr/local/include/boost-1_35_0])
fi 

#====================================================================================
# Check for OpenSSL libs and headers

CHECK_SSL_AS_DEFAULT_AND_FLAGS
if test x"$HAVE_SSL" = x"no"; then    
    AC_MSG_ERROR([Unable to find OpenSSL library, currently this is required.
Try using --with-ssl=<path>, 
E.g., --with-ssl=/usr/local/openssl])
else
    AC_DEFINE([WITH_OPENSSL], 1, [Define to enable DioCLI SSL features.])
fi
 
#====================================================================================

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for ssl required libraries
AC_CHECK_LIB([ssl], [SSL_new])
AC_CHECK_LIB([crypto], [CRYPTO_add])
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([z], [gzopen])

# Configure DEBUG source code, if requested.
echo 
if test x"$debug" = x"yes"; then
    AC_DEFINE([DEBUG], 1, [Define to enable DioCLI debug features.])
    CPPFLAGS="$CPPFLAGS -Wall -fno-inline -c -fmessage-length=0 -DDEBUG -D_DEBUG"
    CXXFLAGS="$CXXFLAGS -g -O0"
else
    CPPFLAGS="$CPPFLAGS -Wall -c -fmessage-length=0 -DNDEBUG"
    CXXFLAGS="$CXXFLAGS -g -O2"
fi

# Checks for typedefs, structures, and compiler characteristics.
# Checks for library functions.

# Configure supported platforms' compiler and linker flags.
case $host in
  *-apple-darwin*)
    AC_DEFINE([OSX], [], 
      [Define if building on Apple OSX.])
    UsingOSX=yes ;;
    *)
	AC_CHECK_LIB([rt], [aio_suspend])
    UsingOSX=no
	LDFLAGS="$LDFLAGS -lrt";;
esac

AM_CONDITIONAL(USING_HOST_OSX, [test x"$UsingOSX" = xyes])

echo "checking host for the mac... $UsingOSX"

#====================================================================================
# Now use the information from the checking stage.

AC_SUBST(DEBUG)
AC_SUBST(USING_HOST_OSX)

AC_CONFIG_FILES([Makefile
                 Include/Makefile
                 Include/tclap/Makefile
                 Include/ErrorCodes/Makefile
                 Include/curl/Makefile
                 Util/Static/Makefile
                 Util/DLL/Makefile
                 gSoap/Static/Makefile
                 gSoap/DLL/Makefile
                 CPPSDK.Lib/Static/Makefile
                 CPPSDK.Lib/DLL/Makefile
                 CPPSDK.DLL/Makefile
		 DioCLI/Makefile])

LT_PREREQ([2.2])
LT_INIT([dlopen])
AC_CONFIG_MACRO_DIR([m4])

AC_OUTPUT

echo \
"-------------------------------------------------

 ${PACKAGE_NAME} Version ${PACKAGE_VERSION}

  Prefix.........: $prefix
  Host...........: $host
  Debug Build....: $debug
  C++ Compiler...: $CXX $CXXFLAGS $CPPFLAGS
  Boost..........: $BOOST_CPPFLAGS
  OpenSSL........: $SSL_CFLAGS
  Linker.........: $LD $LDFLAGS $LIBS

 Now type 'make @<:@<target>@:>@'
   where the optional <target> is:
     all                - build all binaries
     install            - install everything

--------------------------------------------------"




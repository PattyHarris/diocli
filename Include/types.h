/*********************************************************************
 *
 *  file:  types.h
 *
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 *
 *  Use, modification, and distribution is subject to
 *  the New BSD License (See accompanying file LICENSE).
 *
 * Purpose: Definitions for linux/windows cross compiler compatibility
 *
 *********************************************************************/

/**
 *	types.h - These definitions are for linux/windows cross compiler
 *            compatibility
 *
 * author: Eric Young 01/22/2004
 */


#ifndef _TYPES_H_
#define _TYPES_H_

//---------------------------------------------------------------------
// Ensure this constant is available but not redefined.
// On Windows, it's defined in windef.h - not all projects
// include windef.h, and so we define it here....
//---------------------------------------------------------------------

#ifndef MAX_PATH
    #define MAX_PATH    260
#endif

#ifndef MAX_DRIVE
    #define MAX_DRIVE   3
#endif

#ifndef MAX_DIR
    #define MAX_DIR     256
#endif

/*
#define MIN(a,b) ((a < b) ? a : b)
#define MAX(a,b) ((a > b) ? a : b)
#define CLAMP(val,lower,upper) MAX(MIN(val,upper),lower)
*/

//---------------------------------------------------------------------
// Definitions for Linux
//---------------------------------------------------------------------
#if !defined(WIN32)

  #if defined(linux)
     #include <unistd.h>
  #endif

  #include <algorithm>
  using std::min;
  using std::max;

  // this definitions are for linux and the mac
  #include <stdio.h>
  #include <stdlib.h>
  #include <fcntl.h>
  #include <math.h>
  #include <inttypes.h>
  #include <errno.h>
  #include <sched.h>

  #include <string.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/ioctl.h>
  #include <sys/mman.h>
  #include <sys/time.h>
  #include <sys/timeb.h>
  #include <unistd.h>

  /*
  #include <linux/ioctl.h>
  */

  #include <time.h>
  #include <pthread.h>

  // For Hash functions...
  #if defined(__CYGWIN__)
      #include <winerror.h>
  #endif

  // these are needed for Win32/Linux string comparisons
  #define stricmp  strcasecmp
  #define _stricmp strcasecmp
  #define strnicmp strncasecmp
  #define _tcsstr strstr
  #define _tcsnicmp strncasecmp
  #define _tcsncpy strncpy
  #define _tcschr strchr

  #ifndef ZeroMemory
  #define ZeroMemory(ptr, size) \
	    memset((ptr), 0, (size))
  #endif

  typedef long int           LONG;
  typedef short int          SHORT;
  typedef char               CHAR;

  typedef unsigned short     WORD;
  typedef unsigned char      BYTE;
  typedef unsigned int       UINT;

  typedef long long          INT64;
  typedef unsigned long long UINT64;

  typedef int                HANDLE;         // note that handle here is assumed to be
                                             // a pointer to a file decriptor
  typedef int*               PHANDLE;

  typedef int                PT_FILEHANDLE;
  typedef void             * DLL_HANDLE;

  /*
  typedef unsigned long*     LPDWORD;
  typedef int				 BOOL;
  typedef void*              LPOVERLAPPED;
  typedef void*              OVERLAPPED;

  typedef unsigned long      UINT32;
  typedef long long          LARGE_INTEGER;
  typedef unsigned char      BYTE;
  */

  typedef unsigned long      DWORD;
  typedef void*              LPVOID;
  typedef void*              PVOID;
  typedef void               VOID;

  typedef unsigned long      ULONG;
  typedef unsigned short     USHORT;
  typedef unsigned char      UCHAR;
  typedef long long          INT64;

  /* These are defined so we can use TCHAR compatible string calls */
  #define _tmain main
  #define _T(arg) arg

  #define _itoa  itoa
  #define _ltoa  ltoa
  #define _i64toa i64toa

  #define TCHAR char
  #define _TCHAR char
  #define LPCTSTR const char*

  #define tstrcpy  strcpy
  #define tstrncpy strncpy
  #define _tcscat  strcat
  #define _tcscpy(str1, str2) strcpy(str1, str2)
  #define _tcslen(str1) strlen(str1)
  #define _tfopen(filename, access)  fopen(filename, access)
  #define _gettc    getc
  #define _puttc    putc
  #define _stscanf  sscanf
  #define _ftscanf  fscanf
  #define _stprintf sprintf
  #define _tprintf  printf
  #define _vsnprintf vsnprintf


  /* common constants */
  #define SUCCESS         0
  #define FAILURE        -1

  #define IOCTL_FAIL(status)    (status < 0)

  /** unusual return codes */
  #define UNIMPLEMENTED	-1001

  // create some equivalent constants in linux that windows have
  #define STATIC             static

  #ifndef TRUE
        #define TRUE 1
  #endif

  #ifndef FALSE
        #define FALSE 0
  #endif

  #ifndef INVALID_HANDLE_VALUE
        #define INVALID_HANDLE_VALUE -1
  #endif

  /** sleep for x milliseconds */
  inline void nap(unsigned long msec) { usleep(msec*1000); }

  /*
  #define Sleep sleep
  */

  typedef double VWTIME;
  /** returns the amount of time in seconds since some arbitrary moment. */
  inline VWTIME VWGetTime() { return 0.0; }


#endif // end of the WIN32/Linux definitions


//---------------------------------------------------------------------
// Data Types
//---------------------------------------------------------------------
#if defined(SYMBIAN)

    #define LONG64 long
    #define ULONG64 unsigned LONG64

    #define DIOMEDE_LONG_FORMAT "%ld"
    #define DIOMEDE_ULONG_FORMAT "%lu"

#elif !defined(WIN32) || defined(__CYGWIN__) || defined(__GLIBC__) || defined(__GNU__)
    #ifndef LONG64
        #if defined(__GLIBC__)
            #include <bits/wordsize.h>
            #if (__WORDSIZE == 64)
                #define LONG64 int64_t
                #define ULONG64 uint64_t
            #else
                #define LONG64 long long
                #define ULONG64 unsigned LONG64

                #define DIOMEDE_LONG_FORMAT "%ld"
                #define DIOMEDE_ULONG_FORMAT "%lu"

            #endif
        #else
            #define LONG64 long long
            #define ULONG64 unsigned LONG64

            #define DIOMEDE_LONG_FORMAT "%ld"
            #define DIOMEDE_ULONG_FORMAT "%lu"

        #endif
    #endif
#elif defined(UNDER_CE)
    #define LONG64 __int64
    #define ULONG64 unsigned LONG64
#elif defined(__BORLANDC__)
    #define LONG64 __int64
    #define ULONG64 unsigned LONG64
#endif

// Final check for the Diomede format constant
#ifndef DIOMEDE_LONG_FORMAT
    #if !defined(WIN32)
        #define DIOMEDE_LONG_FORMAT "%lld"
    #else
        #define DIOMEDE_LONG_FORMAT "%I64d"
    #endif
#endif

#ifndef DIOMEDE_ULONG_FORMAT
    #if !defined(WIN32)
        #define DIOMEDE_ULONG_FORMAT "%llu"
    #else
        #define DIOMEDE_ULONG_FORMAT "%I64u"
    #endif
#endif

//---------------------------------------------------------------------
// Compiling under Cygwin
//---------------------------------------------------------------------
#if defined(WIN32)
    #if defined(__CYGWIN__) || defined(__GLIBC__) || defined(__GNU__)
		#define _T(arg) arg
        #define _itoa  itoa
        #define _ltoa  ltoa
        #define _i64toa  itoa64
    #endif
#endif

  // These are common declared types

  typedef unsigned char*		PU8;
  typedef unsigned char			U8;
  typedef unsigned short    	U16;
  typedef unsigned long     	U32;
  typedef unsigned long long 	UINT64;
  typedef signed char       	S8;
  typedef signed short      	S16;
  typedef signed long       	S32;

#if defined(_DEBUG)
  #define dbgprint(string) { printf string; fflush(stdout); }
#else
  #define dbgprint(string)
#endif

#endif // end of TYPES_H



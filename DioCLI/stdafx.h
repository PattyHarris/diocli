// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#ifdef WIN32
#define WINVER 0X502
#define _WIN32_WINNT 0x0400
#endif

#include <stdio.h>
#include <map>


#ifdef WIN32
#include <tchar.h>
#endif

using namespace std;

// TODO: reference additional headers your program requires here

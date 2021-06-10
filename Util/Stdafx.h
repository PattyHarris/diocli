// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_DEFS_H__E714415D_A969_4D79_9601_7F6B4AF8DD0C__INCLUDED_)
#define AFX_DEFS_H__E714415D_A969_4D79_9601_7F6B4AF8DD0C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#ifdef WIN32

#ifndef WINVER
#define WINVER 0X502
#endif

#if _MSC_VER >= 1400            // for now, allow deprecated functions when compiling under VS2005
#define _DEFINE_DEPRECATED_HASH_CLASSES 1
#define _CRT_NON_CONFORMING_SWPRINTFS 1
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
#define _CRT_SECURE_COPP_OVERLOAD_STANDARD_NAMES 1
#endif

#pragma warning(disable : 4996)
#pragma warning(disable : 4786)	// debug info truncated
#pragma warning(disable : 4251) // 'blah' needs to have dll-interface to be used by clients of class  'cls'
#pragma warning(disable : 4482) // allow enum names to be used as scopes, since this makes code more self-documenting

#include <tchar.h>
#include "windows.h"

#endif

#include "../Include/types.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>
#include <algorithm>
#include <list>

using namespace std;

#ifdef WIN32
__forceinline const char* CS(const std::string& s) { return s.c_str(); }
#else
inline const char* CS(const std::string& s) { return s.c_str(); }
#endif

#endif // !defined(AFX_DEFS_H__E714415D_A969_4D79_9601_7F6B4AF8DD0C__INCLUDED_)

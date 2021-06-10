// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <iostream>

#ifdef _WIN32
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
#endif

// TODO: reference additional headers your program requires here

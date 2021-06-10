/*********************************************************************
 * 
 *  file:  ErrorType.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: In conjunction with client logging, manages the
 *          error types per module.
 * 
 *********************************************************************/

#ifndef __ERROR_TYPE_H__
#define __ERROR_TYPE_H__

#include "Stdafx.h"
#include <string>

using namespace std;

typedef unsigned long ErrorCode;

// Generic errors
ErrorCode const ERR_NONE                =  0;
ErrorCode const ERR_INTERUPTED_CALL     =  1 << 10;
ErrorCode const ERR_IO                  =  2 << 10;
ErrorCode const ERR_MALFORMED_RESPONSE  =  3 << 10;
ErrorCode const ERR_NOT_SUPPORTED       =  4 << 10;
ErrorCode const ERR_OUT_OF_MEMORY       =  5 << 10;
ErrorCode const ERR_PERMISSION_DENIED   =  6 << 10;
ErrorCode const ERR_TIMED_OUT           =  7 << 10;
ErrorCode const ERR_WOULD_BLOCK         =  8 << 10;
ErrorCode const ERR_INVALID_PARAMETER   =  9 << 10;

// Error "components"
// Note:  If you modify, delete, or add to these values you must also
//        change the corresponding values in "ClientLog.h"
ErrorCode const UI_COMPONENT_ERROR			     =  0x20000000; // UI
ErrorCode const UTIL_COMPONENT_ERROR		     =  0x21000000; // Util

ErrorCode const DIOMEDE_FIRST_COMPONENT           = 0x20000000;  // First component
ErrorCode const DIOMEDE_LAST_COMPONENT            = 0x21000000;  // Last component

//*****************************************************************************
std::string GetErrorComponentName( unsigned int index );
unsigned int GetTotalComponents();
unsigned int GetComponentIndex( std::string& strComponentName );
unsigned int GetComponentIndex( int nComponentValue );

//////////////////////////////////////////////////////////////////////

class ErrorType {
public:
    ErrorType( ErrorCode c = ERR_NONE ) : m_errorCode( c ) { }
    ErrorType&  operator=( ErrorCode );

    operator    ErrorCode() const       { return m_errorCode; }

    std::string AsString() const;
    ErrorCode   GetComponent() const    { return m_errorCode & 0xFF000000; }
private:
    ErrorCode   m_errorCode;


//	See note below in fn.
//    friend bool operator==( ErrorType, ErrorCode );
};

////////// Inlines ////////////////////////////////////////////////////////////

inline ErrorType& ErrorType::operator=( ErrorCode c ) {
    m_errorCode = c;
    return *this;
}

inline bool operator==( ErrorType t, ErrorCode c ) {
// Changed August 1st by Pete to workaround an MSVC issue.
// Should result in the same code at run time
    return static_cast<ErrorCode>(t) == c;
//    return t.m_errorCode == c;
}

inline bool operator!=( ErrorType t, ErrorCode c ) {
    return !( t == c );
}

inline bool operator==( ErrorCode c, ErrorType t ) {
    return t == c;
}

inline bool operator!=( ErrorCode c, ErrorType t ) {
    return t != c;
}

#endif  // __ERROR_TYPE_H__


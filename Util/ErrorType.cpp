/*********************************************************************
 * 
 *  file:  ErrorType.cpp
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

#include "Stdafx.h"
#include <stdlib.h>
#include "ErrorType.h"
#include "ErrorCodes/UIErrors.h"
#include "ErrorCodes/UtilErrors.h"

static unsigned long ComponentIndex[] = {
    UI_COMPONENT_ERROR,
    UTIL_COMPONENT_ERROR };

static const char* ComponentNames[] = {
    _T("UI"),
    _T("Util") };


// Hard coded - must be updated with new additons.
// Macros calcualted this in the "new" version.
#define N_COMPONENTS 2

//*****************************************************************************

string ErrorType::AsString() const {
    switch ( m_errorCode ) {
		// Generic errors
        case ERR_NONE:                  return _T("No error.");
        case ERR_NOT_SUPPORTED:         return _T("Not supported.");
        case ERR_OUT_OF_MEMORY:         return _T("Out of memory.");
        case ERR_PERMISSION_DENIED:     return _T("Permission denied.");
        case ERR_TIMED_OUT:             return _T("Timed out.");
        case ERR_INVALID_PARAMETER:     return _T("Invalid parameter.");

        case UI_COULD_NOT_LOAD_DCF:     return _T("Could not load user configuration file.");
        case UI_INVALID_USERNAME_OR_PWD:
                                        return _T("Invalid username or password.");
        case UI_SERVER_NOT_AVAILABLE:   return _T("Server not available.");
        case UI_SERVER_CONNECTION:      return _T("Unable to connect to server.");

       default:
            {
	            char szTmpOutput[MAX_PATH];
	            sprintf(szTmpOutput, _T("%ld"), m_errorCode);
	            return string(szTmpOutput);
            }
    }
}

string GetErrorComponentName( unsigned int nComponentIndex )
{
    // Very hacky - if the given value is less than the number of
    // components, then use the array of names directly.  Otherwise,
    // assume the value is the component's actual value - use that
    // value to find the array index.

    // The shifting here is due to the nature of the original code and should
    // be cleaned up - values here should be consistent - that is, position in
    // arrays should have some relationship with the actual component value.
    unsigned int nFirstComponent = ((ULONG)DIOMEDE_FIRST_COMPONENT)>>24;
    unsigned int nLastComponent = ((ULONG)DIOMEDE_LAST_COMPONENT)>>24;

    if ( (nComponentIndex >= DIOMEDE_FIRST_COMPONENT) && (nComponentIndex <= DIOMEDE_LAST_COMPONENT)) {
        nComponentIndex = ((ULONG)nComponentIndex)>>24;
    }

    if (nComponentIndex > nLastComponent) {
        return NULL;
    }

    int nTmpIndex = nComponentIndex;

    if ( nComponentIndex >= nFirstComponent ) {
        nTmpIndex = GetComponentIndex(nComponentIndex);
    }

    return ComponentNames[nTmpIndex];
}

unsigned int GetTotalComponents()
{
    return N_COMPONENTS;
}

unsigned int GetComponentIndex( string& strComponentName )
{
    for( unsigned int i = 0; i < N_COMPONENTS; i++ ) {
        if ( 0 == _stricmp( (char*)strComponentName.c_str(), ComponentNames[i] ) )
            return i;
    }

    return N_COMPONENTS;
}

unsigned int GetComponentIndex( int nComponentValue )
{
    for( unsigned int i = 0; i < N_COMPONENTS; i++ ) {
        if ( nComponentValue == ((int)ComponentIndex[i]>>24) )
            return i;
    }

    return N_COMPONENTS;
}

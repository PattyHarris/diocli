/*********************************************************************
 * 
 *  file:  SimpleRedirect.cpp
 * 
 *  Copyright (c) Sommergyll Software 2007-2008
 *
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: 
 * 
 * 
 *********************************************************************/

#include "SimpleRedirect.h"
#include "types.h"

#ifdef WIN32
#include <tchar.h>
#endif

/////////////////////////////////////////////////////////////////////////////

SimpleRedirect* SimpleRedirect::m_pInstance = NULL;

/////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Creates the single object
//      @return A pointer to the only object
SimpleRedirect* SimpleRedirect::Instance()
{
    /*    
	static SimpleRedirect * iInstance = new SimpleRedirect;
    return iInstance;
    */
    
	static SimpleRedirect iInstance;
	m_pInstance = &iInstance;
	return &iInstance;
}

/////////////////////////////////////////////////////////////////////////////
SimpleRedirect::SimpleRedirect(void)
: iOldStdout(0), iOldStderr(0), 
  iIsRedirecting(false), iFile(NULL)  
{
}

/////////////////////////////////////////////////////////////////////////////
SimpleRedirect::~SimpleRedirect(void)
{

}

/////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Redirect both stdout and stderr
//      @param aFileName The file to write output to
//      @return true if successful
bool SimpleRedirect::StartRedirect(string aFileName)
{
    bool result = false;
    
    if (iIsRedirecting)
    {
        EndRedirect();
    }
    
    // open file for sharing and writing
    #ifdef WIN32
        iFile = _fsopen(aFileName.c_str(), _T("w+"), SH_DENYNO); 
        if (iFile != NULL)
        {            
            // copy the current stdout stream
            iOldStdout = _dup(1); 
            // copy the current stderr stream
            iOldStderr = _dup(2); 
            // redirect stdout and stderr to file
            if (!_dup2(_fileno(iFile), 1) && !_dup2(_fileno(iFile), 2))
            {
                iIsRedirecting = true;
                result = true;
            }
        }
    #else 
        iFile = fopen(aFileName.c_str(), _T("w+")); 
        if (iFile != NULL)
        {            
            // copy the current stdout stream
            iOldStdout = dup(1); 
            // copy the current stderr stream
            iOldStderr = dup(2); 
            // redirect stdout and stderr to file
            if (!dup2(fileno(iFile), 1) && !dup2(fileno(iFile), 2))
            {
                iIsRedirecting = true;
                result = true;
            }
        }
    #endif

    return result;

} // End StartRedirect

/////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Redirect back to original stdout and stderr. Will do nothing
//      if not currently redirecting
void SimpleRedirect::EndRedirect()
{
    if (iIsRedirecting)
    {
        // Key to get this working is to flush BEFORE the call to _dup
        // Original code had the reverse...
        iIsRedirecting = false;
        fflush(stdout);
        fclose(iFile);  

        #if WIN32
            // redirect back to the old values
            _dup2(iOldStdout, 1);        
            _dup2(iOldStderr, 2);
        #else 
            // redirect back to the old values
            dup2(iOldStdout, 1);        
            dup2(iOldStderr, 2);
        #endif

    }

} // End EndRedirect

/////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Redirect back to original stdout and stderr. Will do nothing
//      if not currently redirecting
void SimpleRedirect::Shutdown()
{
    EndRedirect(); 
    
	if ( this == m_pInstance ) {
		m_pInstance = NULL;
    }

    // delete this;

} // End Shutdown

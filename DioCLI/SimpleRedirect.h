/*********************************************************************
 *
 *  file:  SimpleRedirect.h
 *
 *  Copyright (c) Sommergyll Software 2007-2008
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Class to redirect stdout to a file.
 *
 *********************************************************************/

#ifndef __SIMPLE_REDIRECT_H__
#define __SIMPLE_REDIRECT_H__

#include <string>
#ifdef WIN32
#include <io.h>
#endif

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// Redirection of stdout and stderr to a file and retrieving of the content
// from the file
class SimpleRedirect
{
public:
    static SimpleRedirect* Instance();
    virtual ~SimpleRedirect(void);

    bool StartRedirect(string aFileName);
    void EndRedirect(void);
    void Shutdown(void);

private: // not implemented
    SimpleRedirect(void);
    SimpleRedirect(const SimpleRedirect&);
    SimpleRedirect& operator=(const SimpleRedirect&);

private:
    static              SimpleRedirect *m_pInstance;
    int                 iOldStdout;
    int                 iOldStderr;
    bool                iIsRedirecting;
    FILE*               iFile;
};

#endif // __SIMPLE_REDIRECT_H__
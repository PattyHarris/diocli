/*********************************************************************
 * 
 *  file:  configure.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Configuration file handler. Reads and writes 
 *          configuration files.
 * 
 *********************************************************************/

#ifndef __CONFIGURE_H__
#define __CONFIGURE_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Stdafx.h"
#include "dictionary.h"

#include <string>
using namespace std;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/*
* NAME:        define equality for our dictionary
*/

inline
int operator!=(const Dictionary<std::string>::iterator& Name1,
               const Dictionary<std::string>::iterator& Name2)
{
    return !((Name1.where == Name2.where)
        && (Name1.ihash == Name2.ihash));
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*
* Configuration file handler. Reads and writes configuration files.
*/

class Configure
{
protected:

    Dictionary<std::string>     m_ValSet;            // the set of config values
    bool                        m_bReadSuccessful;   // true if the file was read
    std::string                 m_strFilename;       // succesfully


    /*
     * NAME:        Read
     * ACTION:      Read the configuration file
     * PARAMETERS:  char* FileName - the name of the file to read
     */

    int Read(char* FileName);


    /*
     * NAME:        Init
     * ACTION:      Initialize the configuration file.  Should be handled
     *              by the derived class.
     * PARAMETERS:  char* FileName - the name of the file to initialize.
     */

    virtual void Init(char* FileName) {};

public:

    /*
     * NAME:        Configure
     * ACTION:      Create a new configuration handler
     * PARAMETERS:  char* FileName - the name of the configuration file
     */

    Configure(const char* szFileName);

protected:
	Configure() {};

public:
    /*
     * NAME:        ~Configure
     * ACTION:      Delete the configuration handler
     */

    ~Configure();



    /*
     * NAME:        ReadFileOk
     * ACTION:      Indicates if the file could be read ok
     * RETURNS:     bool - true if file was read ok
     */

    bool ReadFileOk() { return m_bReadSuccessful; };


    /*
     * NAME:        Get
     * ACTION:      Get a configuration value
     * PARAMETERS:  const char* Name - the name of the configuration parameter
     *              const char* Default - optional default value
     * RETURNS:     string - the value or Default if not found
     */

    std::string Get(const char* Name, const char* Default = NULL);

    /*
     * NAME:        GetInt
     * ACTION:      Get an integer configuration value
     * PARAMETERS:  char* Name - the name of the configuration parameter
     *              int Default - optional default value
     * RETURNS:     int - the value or Default if not found
     */

    int GetInt(const char* Name, int Default = 0);

    /*
     * NAME:        GetLong
     * ACTION:      Get a long configuration value
     * PARAMETERS:  char* Name - the name of the configuration parameter
     *              long Default - optional default value
     * RETURNS:     long - the value or Default if not found
     */

    long GetLong(const char* Name, long Default = 0);


    /*
     * NAME:        Set
     * ACTION:      Set a configuration value
     * PARAMETERS:  char* Name - the name of the configuration parameter
     *              char* Value - the value
     */

    void Set(const char* Name, const char* Value);



    /*
     * NAME:        SetInt
     * ACTION:      Set an integer configuration value
     * PARAMETERS:  char* Name - the name of the configuration parameter
     *              int Value - the value
     */

    void SetInt(const char* Name, int Value);

    /*
     * NAME:        SetLong
     * ACTION:      Set a long configuration value
     * PARAMETERS:  char* Name - the name of the configuration parameter
     *              long Value - the value
     */

    void SetLong(const char* Name, long Value);

    /*
     * NAME:        Save
     * ACTION:      Saves the file back to disk, not necessarily in a
     *              form which is very human-readable
     */

    int Save();

    /*
     * NAME:        AsciiToUnicode
     * ACTION:      Convert an ascii string to unicode
     */
#ifdef _UNICODE
    static bool AsciiToUnicode(const char* szAscii, std::string& szUnicode);
    static bool AsciiToUnicode(const char* szAscii, WCHAR* szUnicode);
#endif

    /*
     * NAME:        UnicodeToAscii
     * ACTION:      Convert an unicode string to ascii
     */

#ifdef _UNICODE
    static bool UnicodeToAscii(const char* szUnicode, char* szAscii, int nLen);
#endif
};

#endif // __CONFIGURE_H__

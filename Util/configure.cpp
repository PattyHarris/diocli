/*********************************************************************
 * 
 *  file:  configure.cpp
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

#include "Stdafx.h"
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>

#include <string>

#include "configure.h"
#include "Util.h"

#define MAXREADLINE 1024        // maximum length of a configuration file line

/*
 * NAME:        Configure
 * ACTION:      Create a new configuration handler
 * PARAMETERS:  char *FileName - the name of the configuration file
 */

Configure::Configure(const char* szFileName)
{
    //this->FileName = FileName;
    if ( szFileName && 0 != *szFileName ) {
        m_strFilename = szFileName;
        Read((char*)m_strFilename.c_str());
    }
}


/*
 * NAME:        ~Configure
 * ACTION:      Delete the configuration handler
 */

Configure::~Configure()
{
#if 0    // not needed since we're using strs now
    char *Name;
    Dictionary<string>::iterator ThisName;

    // delete all the value strs
    for (ThisName = m_ValSet.begin(); ThisName != m_ValSet.end(); ThisName++)
    {
        Name = *ThisName;
        free(m_ValSet[Name]);
    }
#endif
}

/*
* NAME:        define equality for our dictionary
*/
/*
int operator!=(const Dictionary<string>::iterator& Name1,
               const Dictionary<string>::iterator& Name2)
{
    return !((Name1.where == Name2.where)
        && (Name1.ihash == Name2.ihash));
}
*/

/*
 * NAME:        Read
 * ACTION:      Read the configuration file
 * PARAMETERS:  char *FileName - the name of the file to read
 */

int Configure::Read(char *FileName)
{
    FILE *InFile;
    char  InLine[MAXREADLINE];
    char *ReadLine;
    char *StartLine;
    char *EndLine;
    char *CPos;

    m_bReadSuccessful = false;

    // If the file hasn't been created yet, allow derived classes to
    // initialize the data before it's read in the first time.
    bool bDoesFileExist = Util::DoesFileExist(FileName);
    if (bDoesFileExist == false) {
        Init(FileName);
    }

    // Open the file
    InFile = _tfopen(FileName, _T("rt"));
    if (InFile == NULL) {
        return errno;
    }
    
    // Read it
    do
    {
        ReadLine = fgets(InLine, MAXREADLINE-1, InFile);
        if (ReadLine != NULL)
        {
            // process a line

            // comment
            // Various conversions: convert ReadLine to unicode array - first to
            // a string and then to an array??

            CPos = strchr(ReadLine, _T('#'));
            if (CPos != NULL)
                *CPos = '\0';

            // eliminate leading or trailing space
            for (StartLine = ReadLine; isspace(*StartLine); StartLine++)
            {};

            if (strlen(StartLine) > 0)
            {
                for (EndLine = StartLine + strlen(StartLine) - 1; isspace(*EndLine); EndLine--)
                    *EndLine = '\0';
            }

            // empty line?
            if (*StartLine != '\0')
            {
                // got some text on this line

                // find the name
                for (CPos = StartLine; isalnum(*CPos); CPos++)
                {};

                *CPos = '\0';   // null-term name string
                CPos++;

                // skip an '=' and any spaces
                while (*CPos == '=' || isspace(*CPos))
                    CPos++;

                // set the value
                Set(StartLine, CPos);
            }
        }

    } while (ReadLine != NULL);

    // close up
    fclose(InFile);
    m_bReadSuccessful = true;
    return 0;
}



/*
 * NAME:        Get
 * ACTION:      Get a configuration value
 * PARAMETERS:  const char* Name - the name of the configuration parameter
 *              const char* Default - optional default value
 * RETURNS:     string - the value or Default if not found
 */

string Configure::Get(const char* Name, const char* Default)
{
    if ( m_ValSet.exists(Name) ) {
        string FoundVal = m_ValSet[Name];

        if (FoundVal.empty())
            return Default;
        else
            return FoundVal;
    } else {
        return Default;
    }
}



/*
 * NAME:        GetInt
 * ACTION:      Get an integer configuration value
 * PARAMETERS:  char* Name - the name of the configuration parameter
 *              int Default - optional default value
 * RETURNS:     int - the value or Default if not found
 */

int Configure::GetInt(const char* Name, int Default)
{
    string FoundVal = m_ValSet[Name];

    if (FoundVal.empty())
        return Default;
    else
        return atoi(FoundVal.c_str());
}

/*
 * NAME:        GetLong
 * ACTION:      Get a long configuration value
 * PARAMETERS:  char* Name - the name of the configuration parameter
 *              long Default - optional default value
 * RETURNS:     long - the value or Default if not found
 */

long Configure::GetLong(const char* Name, long Default)
{
    string FoundVal = m_ValSet[Name];

    if (FoundVal.empty())
        return Default;
    else
        return atol(FoundVal.c_str());
}


/*
 * NAME:        Set
 * ACTION:      Set a configuration value
 * PARAMETERS:  char* Name - the name of the configuration parameter
 *              char* Value - the value
 */

void Configure::Set(const char* Name, const char* Value)
{
    m_ValSet[Name] = Value;
}

/*
 * NAME:        SetInt
 * ACTION:      Set an integer configuration value
 * PARAMETERS:  char *Name - the name of the configuration parameter
 *              char *Value - the value
 */

void Configure::SetInt(const char* Name, int Value)
{
    char IntBuf[MAXREADLINE*2];

#if defined(WIN32)
	sprintf(IntBuf, _T("%d"), Value);
#else
    snprintf(IntBuf, MAXREADLINE-1, "%d", Value);
#endif
	m_ValSet[Name] = IntBuf;
}

/*
 * NAME:        SetLong
 * ACTION:      Set a long configuration value
 * PARAMETERS:  char* Name - the name of the configuration parameter
 *              long Value - the value
 */

void Configure::SetLong(const char* Name, long Value)
{
    char LongBuf[MAXREADLINE*4];

#if defined(WIN32)
	sprintf(LongBuf, _T("%ld"), Value);
#else
    snprintf(LongBuf, MAXREADLINE-1, "%ld", Value);
#endif
	m_ValSet[Name] = LongBuf;
}


/*
 * NAME:        Save
 * ACTION:      Saves the file back to disk, not necessarily in a
 *              form which is very human-readable
 */

int Configure::Save()
{
    FILE *OutFile;
    string Name;
    Dictionary<string>::iterator ThisName;

    // open the file
    OutFile = fopen(m_strFilename.c_str(), _T("wt"));
    if (OutFile == NULL) {
        return errno;
    }

    // write it
    for (ThisName = m_ValSet.begin(); ThisName != m_ValSet.end(); ThisName++)
    {
        Name = *ThisName;
        // Name.c_str() is the value not the key:
        // _ftprintf(OutFile, "%s = %s\n", Name.c_str(), m_ValSet[Name.c_str()].c_str());
        fprintf(OutFile, _T("%s = %s\n"), ThisName.key().c_str(), Name.c_str());
    }

    // close up
    fflush(OutFile);
    fclose(OutFile);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Purpose:
//      Convert an ascii string to unicode.
//	Requires:
//	Returns:
//		true if successful, false otherwise
//
#ifdef _UNICODE
bool Configure::AsciiToUnicode(const char* szAscii, string& szUnicode)
{
    if (!szAscii) {
        return false;
    }

	char *pbuf = NULL;
	int iSize = MultiByteToWideChar( CP_ACP, 0, szAscii, strlen(szAscii), pbuf,  0);
	pbuf = new char[iSize+1];

	memset(pbuf, 0, (iSize+1) * sizeof(char) );
	MultiByteToWideChar( CP_ACP, 0, szAscii, strlen(szAscii), pbuf,  iSize);
	szUnicode = pbuf;
	delete[] pbuf;

	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Purpose:
//      Convert an ascii string to unicode.
//	Requires:
//	Returns:
//		true if successful, false otherwise
//
bool Configure::AsciiToUnicode(const char* szAscii, WCHAR* szUnicode)
{
    if (!szAscii) {
        return false;
    }

	char *pbuf = NULL;
	int iSize = MultiByteToWideChar( CP_ACP, 0, szAscii, strlen(szAscii), pbuf,  0);
	pbuf = new char[iSize+1];

	memset(pbuf, 0, (iSize+1) * sizeof(WCHAR) );
	MultiByteToWideChar( CP_ACP, 0, szAscii, strlen(szAscii), pbuf,  iSize);

	wcscpy(szUnicode, pbuf);
	delete[] pbuf;

	return true;
}
/////////////////////////////////////////////////////////////////////////////
//
// Purpose:
//      Convert a Unicode string to Ascii.
//	Requires:
//	Returns:
//		true if successful, false otherwise
//
bool Configure::UnicodeToAscii(const char* szUnicode, char* szAscii, int nLen)
{
	size_t uLength = strlen(szUnicode);
	if ((size_t)nLen >= uLength) {
	    return false;
	}

    bool bSuccess = true;
	memset(szAscii, 0, nLen * sizeof(char) );
	if (WideCharToMultiByte( CP_ACP, 0, szUnicode, uLength, szAscii, nLen, NULL, NULL) == 0) {
	   bSuccess = false;
	}

 	return bSuccess;
}
#endif


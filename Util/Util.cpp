/*********************************************************************
 * 
 *  file:  Util.cpp
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Contains miscellaneous utility functions.
 * 
 *********************************************************************/

//! \defgroup util Util
// Contains general utility functions.
//! @{

#include "Stdafx.h"
#include "Util.h"
#include "StringUtil.h"

#include <iostream>
#include <fstream>

#ifdef WIN32
#include <conio.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ShlObj.h>
#endif

#if defined(linux)
#include <unistd.h>
#endif

#ifndef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

// For GetFileName and helpers
#include <stddef.h>
#include <string.h>
#include <errno.h>

// For accessing home directory
#include <pwd.h>
#endif

#define ISSLASH(c) ( c=='/' )
#define EXT_DELIMIT '.'

#ifdef WIN32
    // Note: when we move to the MAC, the MAC is the same as
    // windows, except for the volume separator.
    static const std::string g_szDirectorySeparator = "\\";
    static const std::string g_szAltDirectorySeparator = "\\";
    static const std::string g_szVolumeSeparator = ":";
    static const bool g_bDirEqualsVolume = false;

    static const char g_szDirectorySeparatorChar = '\\';
    static const char g_szAltDirectorySeparatorChar = '\\';
    static const char g_szVolumeSeparatorChar = ':';
#else
    static const std::string g_szDirectorySeparator = "/";
    static const std::string g_szAltDirectorySeparator = "/";
    static const std::string g_szVolumeSeparator = "/";
    static const bool g_bDirEqualsVolume = true;

    static const char g_szDirectorySeparatorChar = '/';
    static const char g_szAltDirectorySeparatorChar = '/';
    static const char g_szVolumeSeparatorChar = '/';
#endif
    static const std::string g_szRelativeDirectorySeparator = "/";

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Custom getch to provide a *nix solution to getch (which is
//      not available by all stdio.h's.  The Window's version of
//      reads a single character from the console without echoing.
// Requires: nothing
// Returns: int reresenting the character read
int Util::CustomGetch()
{
    //-----------------------------------------------------------------
    //      TBD: extended key handling - for example:
    //       c = getch();   // retrieve second byte of extended code
    //       c *= 256;      // shift left by 8 bits to get extended code
    //-----------------------------------------------------------------

    int nChar = 0;

    #ifdef _WIN32
	    int nTmpChar = 0;
	    int nExtendedChar = 0;

        nChar = getch();

        if (nChar == 0) {
            nTmpChar = getch();
            nExtendedChar = nTmpChar | 0x0100;
            nChar = nTmpChar;

        }
        else if (nChar == 0x00e0) {
            nTmpChar = getch();
            nExtendedChar = nTmpChar | 0x0200;
            nChar = nTmpChar;
        }

        /* Similarly, we could use the following
        if ( kbhit()) {
            nTmpChar = getch();
            nExtendedChar = nTmpChar | 0x0200;
            nChar = nTmpChar;
        }
        */


    #else
        // This takes away the buffering in the Linux terminals so that you don't
        // have to press the Enter key afterwards.
        struct termios oldSetting, newSetting;

        tcgetattr ( STDIN_FILENO, &oldSetting );
        newSetting = oldSetting;
        newSetting.c_lflag &= ~( ICANON | ECHO );
        tcsetattr ( STDIN_FILENO, TCSANOW, &newSetting );

        nChar = getchar();

        tcsetattr ( STDIN_FILENO, TCSANOW, &oldSetting );
    #endif

  return nChar;

} // End CustomGetch

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Split a path into its parts
// Requires:
//      szPath: Object to split
//      szDrive: Logical drive , only for compatibility , not considered
//      szDirectory: Directory part of path
//      szFilename: File part of path
//      szExtension: Extension part of path (includes the leading point)
// Returns: Directory szFilename and szExtension are changed
// Comment: Note that the concept of an extension is not available in Linux,
//          nevertheless it is considered
void SplitPath(const char* szPath, char* szDrive, char* szDirectory,
                char* szFilename, char* szExtension)
{
    char* szCopyOfPath = (char*)szPath;
    int nCounter = 0;
    int nLast = 0;
    int nRest = 0;

    // No drives available in Linux.
    // Extensions are not common in Linux but considered anyway.
    szDrive = NULL;

    while(*szCopyOfPath != '\0') {
        // Search for the last slash
        while( *szCopyOfPath != '/' && *szCopyOfPath != '\0') {
            szCopyOfPath++;
            nCounter++;
        }

        if (*szCopyOfPath == '/') {
            szCopyOfPath++;
            nCounter++;
            nLast = nCounter;
        }
        else {
            nRest = nCounter - nLast;
        }
    }

    // Directory is the first part of the path until the
    // last slash appears
    strncpy(szDirectory, szPath, nLast);

    // strncpy doesnt add a '\0'
    szDirectory[nLast] = '\0';

    // In the case where we only need the directory...
    if (szFilename == NULL) {
        return;
    }

    // Filename is the part behind the last slash
    szCopyOfPath -= nRest;
    if (sizeof(szCopyOfPath) > 0) {
        strcpy(szFilename, szCopyOfPath);
    }

    std::string szCopyOfFile = std::string(szFilename);
    if (szCopyOfFile.find(EXT_DELIMIT) == size_t(-1)) {
        return;
    }

    // Get the extension if there is any

	int nPos = szCopyOfFile.find_last_of(_T("."));
	std::string szTmpExt = _T("");

	if ( (nPos > -1) && (nPos < (int)szCopyOfFile.size()) ) {
		szTmpExt = szCopyOfFile.substr(nPos, szCopyOfFile.size() - 1 );
		strcpy(szExtension, szTmpExt.c_str());
	}


    #if 0
    // This code is buggy - the above, although has more
    // overhead, is safe.
    while (*szCopyOfFile != '\0') {
        if (*szCopyOfFile == EXT_DELIMIT ) {
            while (*szCopyOfFile != '\0') {
                *szExtension = *szCopyOfFile;
                szExtension++;
                szCopyOfFile++;
            }
        }

        if (*szCopyOfFile != '\0') {
             szCopyOfFile++;
        }
    }
    #endif

    *szExtension = '\0';

} // End SplitPath

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Custom _splitpath for Linux - helper to GetFileName below.
// Requires:
//      szFilePath: complete file path
// Returns: filename from path
char* SplitPath(char const* szFilePath)
{
    char const* szBase = szFilePath;
    char const* szCurrent;

    for (szCurrent = szBase; *szCurrent; szCurrent++) {
        if ( ISSLASH (*szCurrent) )  {
            // Treat multiple adjacent slashes like a single slash.
            do szCurrent++;
            while (ISSLASH (*szCurrent));

            // If the file name ends in slash, use the trailing slash as
            // the basename if no non-slashes have been found.
            if (! *szCurrent) {
                if (ISSLASH (*szBase)) {
                    szBase = szCurrent - 1;
                }
                break;
            }

            // *szCurrent is a non-slash preceded by a slash.
            szBase = szCurrent;
        }
    }

    return (char *) szBase;

} // End SplitPath

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to suspend our process to give control
//          to other running processes.
// Requires:
//      nMilliseconds: sleep amount in milliseconds.
// Returns: nothing

///////////////////////////////////////////////////////////////////////
#if !defined( WIN32 )
extern "C"
{
    int	usleep(useconds_t useconds);
    #ifdef NANO_SECOND_SLEEP
        int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
    #endif
}
#endif

///////////////////////////////////////////////////////////////////////
void Util::PauseProcess( unsigned int nMilliseconds)
{
    #if defined(WIN32)
        Sleep(nMilliseconds);
    #else
        #ifdef NANO_SECOND_SLEEP
	        struct timespec interval, remainder;
	        nMilliseconds = nMilliseconds * 1000000;
	        interval.tv_sec= 0;
	        interval.tv_nsec = nMilliseconds;
	        nanosleep(&interval, &remainder);
        #else
	        usleep(nMilliseconds * 1000);
        #endif
    #endif

} // End PauseProcess

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Parses the filename and extension from the filepath.
// Requires:
//      szFilePath: complete file path
//      szFileName: file name + extension.
// Returns: true if successful, false otherwise
bool Util::GetFileName(const std::string szFilePath, std::string& szFileName)
{
#ifdef WIN32
    // We need the filename without the path.
    char szDrive[_MAX_DRIVE];
    char szDir[_MAX_DIR];
    char szTmpFileName[_MAX_FNAME];
    char szExt[_MAX_EXT];

    _splitpath(szFilePath.c_str(), szDrive, szDir, szTmpFileName, szExt);
    if (sizeof(szExt) != 0) {
        strcat(szTmpFileName, szExt);
    }

    szFileName = std::string(szTmpFileName);

#else
    // We need the filename without the path.
    char szDrive[3];
    char szDir[256];
    char szTmpFileName[256];
    char szExt[256];

    // The extension is attached to the filename, so catenation
    // is needed here.
    SplitPath(szFilePath.c_str(), szDrive, szDir, szTmpFileName, szExt);
    szFileName = std::string(szTmpFileName);

    #if 0
    // This version returns the filename minus the extension.
    char *szTmpFileName = new char[szFilePath.length()];
    char* szCurrent = szTmpFileName;

    strcpy(szTmpFileName, SplitPath(szFilePath.c_str() ) );
    while (strchr(szTmpFileName, EXT_DELIMIT) != NULL ) {
        if (strlen(szCurrent) < 2) {
            break;
        }
        if (*szCurrent) {
            szCurrent += strlen(szCurrent);
            while(*szCurrent != EXT_DELIMIT) {
                --szCurrent;
            }
            if (!*szCurrent) {
                --szCurrent;
                break;
            }
            else {
                *szCurrent=0x0;
            }
        }
    }

    szFileName = std::string(szTmpFileName);
    delete[] szTmpFileName;
    #endif

#endif

    return true;

} // End GetFileName

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Finds the parent directory from the path.
// Requires:
//      szFilePath: complete file path
//      szParentDir: parent directory
// Returns: nothing
void Util::GetParentDirFromDirPath(const string& szFilePath, string& szParentDir)
{
#ifdef WIN32
	char szDriveBuffer[_MAX_DRIVE];
	char szDirBuffer[_MAX_DIR];
	_splitpath(szFilePath.c_str(), szDriveBuffer, szDirBuffer, NULL, NULL);

	string szDir = szDirBuffer;
	string::size_type posLastBackslash = szDir.find_last_of(_T("\\"));
	string::size_type posLastSlash = szDir.find_last_of(_T("/"));
	string::size_type posLastSeparator = max((int)posLastBackslash, (int)posLastSlash);

	string szTmpParentDir = _T("");
    if (posLastSeparator != (size_t)-1) {
        if ( (szDir.length() == 1) && (int)(posLastSeparator == 0) ) {
	        szTmpParentDir = szDir.substr(0, 1);
        }
        else {
	        szTmpParentDir = szDir.substr(0, posLastSeparator);
	    }
	}

	char szParentBuffer[_MAX_PATH];
	_makepath(szParentBuffer, szDriveBuffer, szTmpParentDir.c_str(), NULL, NULL);
	szParentDir = szParentBuffer;

	// If the parent directory is empty, e.g. *.*, return the current folder.
	if (szParentDir.length() == 0) {
	    GetWorkingDirectory(szParentDir);
	}

#else
	char szDriveBuffer[MAX_DRIVE];
	char szDirBuffer[MAX_DIR];

    SplitPath(szFilePath.c_str(), szDriveBuffer, szDirBuffer, NULL, NULL);

	string szDir = szDirBuffer;
	string::size_type posLastBackslash = szDir.find_last_of(_T("/"));
	string::size_type posLastSlash = szDir.find_last_of(_T("\\"));
	string::size_type posLastSeparator = max((int)posLastBackslash, (int)posLastSlash);

	string szTmpParentDir = _T("");
    if ((int)posLastSeparator > 0) {
	    szTmpParentDir = szDir.substr(0, posLastSeparator);
	}

	char szParentBuffer[MAX_PATH];
	strcpy(szParentBuffer, szTmpParentDir.c_str());
	strcat(szParentBuffer, _T("/"));

	szParentDir = szParentBuffer;

	// If the parent directory is empty, e.g. *.*, return the current folder.
	if ( (szParentDir.length() == 0) ||
         ( (szParentDir.length() < 2) && (szParentDir[szParentDir.size() - 1] == '/')) ) {
	    GetWorkingDirectory(szParentDir);
	}

#endif

} // End GetParentDirFromDirPath

// defines

// The number of characters at the start of an absolute filename.  e.g. in DOS,
// absolute filenames start with "X:\" so this value should be 3, in UNIX they start
// with "\" so this value should be 1.
#ifdef WIN32
    #define ABSOLUTE_NAME_START 3
#else
    #define ABSOLUTE_NAME_START 1
#endif
#define MAX_FILENAME_LEN 512

// Set this to '\\' for DOS or '/' for UNIX
#ifdef WIN32
    #define SLASH '\\'
#else
    #define SLASH '/'
#endif

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Given the absolute current directory and an absolute file name,
//      returns a relative file name. For example, if the current
//      directory is C:\foo\bar and the filename C:\foo\whee\text.txt is
//      given, GetRelativeFilename will return ..\whee\text.txt.
//      Modified version of "Function to Determine a File's Relative Path",
//      Rob Fisher, www.CodeGuru.com
// Requires:
//      szCurrentDirectory: current working directory
//      szAbsoluteFilename: complete file path
// Returns: relative path
char* Util::GetRelativeFilename(char *szCurrentDirectory, char *szAbsoluteFilename)
{
    // Declarations - put here so this should work in a C compiler
    int nAbsoluteFileMarker = 0, nRelativeFileMarker = 0;
    int nCurrDirLen = 0, nAbsoluteFileLen = 0;
    int nIndex = 0;
    int nLevels = 0;

    static char relativeFilename[MAX_FILENAME_LEN+1];

    nCurrDirLen = strlen(szCurrentDirectory);
    nAbsoluteFileLen = strlen(szAbsoluteFilename);

    // Make sure the names are not too long or too short
    if (nCurrDirLen > MAX_FILENAME_LEN || nCurrDirLen < ABSOLUTE_NAME_START+1 ||
        nAbsoluteFileLen > MAX_FILENAME_LEN || nAbsoluteFileLen < ABSOLUTE_NAME_START+1) {
        return NULL;
    }

    // Handle DOS names that are on different drives:
    if (szCurrentDirectory[0] != szAbsoluteFilename[0]) {
        // not on the same drive, so only absolute filename will do
        strcpy(relativeFilename, szAbsoluteFilename);
        return relativeFilename;
    }

    // they are on the same drive, find out how much of the current directory
    // is in the absolute filename
    nIndex = ABSOLUTE_NAME_START;
    while (nIndex < nAbsoluteFileLen && nIndex < nCurrDirLen &&
           szCurrentDirectory[nIndex] == szAbsoluteFilename[nIndex]) {
        nIndex++;
    }

    if ( nIndex == nCurrDirLen && (szAbsoluteFilename[nIndex] == SLASH ||
       szAbsoluteFilename[nIndex-1] == SLASH) ) {
        // the whole current directory name is in the file name,
        // so we just trim off the current directory name to get the
        // current file name.
        if (szAbsoluteFilename[nIndex] == SLASH) {
            // a directory name might have a trailing slash but a relative
            // file name should not have a leading one...
            nIndex++;
        }

        strcpy(relativeFilename, &szAbsoluteFilename[nIndex]);
        return relativeFilename;
    }


    // The file is not in a child directory of the current directory, so we
    // need to step back the appropriate number of parent directories by
    // using "..\"s.  First find out how many levels deeper we are than the
    // common directory
    nAbsoluteFileMarker = nIndex;
    nLevels = 1;

    // Count the number of directory levels we have to go up to get to the
    // common directory
    while (nIndex < nCurrDirLen) {
        nIndex++;
        if (szCurrentDirectory[nIndex] == SLASH) {
            // Make sure it's not a trailing slash
            nIndex++;
            if (szCurrentDirectory[nIndex] != '\0') {
                nLevels++;
            }
        }
    }

    // Move the absolute filename marker back to the start of the directory name
    // that it has stopped in.
    while (nAbsoluteFileMarker > 0 && szAbsoluteFilename[nAbsoluteFileMarker-1] != SLASH) {
        nAbsoluteFileMarker--;
    }

    // Check that the result will not be too long
    if (nLevels * 3 + nAbsoluteFileLen - nAbsoluteFileMarker > MAX_FILENAME_LEN) {
        return NULL;
    }

    // Add the appropriate number of "..\"s.
    nRelativeFileMarker = 0;
    for (nIndex = 0; nIndex < nLevels; nIndex++) {
        relativeFilename[nRelativeFileMarker++] = '.';
        relativeFilename[nRelativeFileMarker++] = '.';
        relativeFilename[nRelativeFileMarker++] = SLASH;
    }

    // Copy the rest of the filename into the result string
    strcpy(&relativeFilename[nRelativeFileMarker], &szAbsoluteFilename[nAbsoluteFileMarker]);

    return relativeFilename;

} // End GetRelativeFilename

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to determin Gets a value indicating whether
//      the specified path string contains absolute or relative path
//      information.
// Requires:
//      szPath: returned home directory, empty on failure
// Returns: true if successful, false otherwise
bool IsPathRooted (std::string szPath)
{
    if (szPath.length() == 0) {
        return false;
    }

    const char InvalidPathCharsList[] = {
        '\x00', '\x08', '\x10', '\x11', '\x12', '\x14', '\x15', '\x16',
        '\x17', '\x18', '\x19', '\x22', '\x3C', '\x3E', '\x7C' };

    int nIndex;
    size_t posInvalidChar;

    for (nIndex = 0; nIndex < (int)sizeof(InvalidPathCharsList); nIndex ++) {
        posInvalidChar = szPath.find(InvalidPathCharsList[nIndex]);
        if (posInvalidChar != std::string::npos) {
            return false;
        }
    }

    char szFirstChar = szPath[0];

    return (szFirstChar == g_szDirectorySeparatorChar ||
            szFirstChar == g_szAltDirectorySeparatorChar ||
           (!g_bDirEqualsVolume && szPath.length() > 1 && szPath[1] == g_szVolumeSeparatorChar));

} // End IsPathRooted

///////////////////////////////////////////////////////////////////////
bool IsDirectorySeparatorChar(char szInChar)
{
	return (szInChar == g_szDirectorySeparatorChar) || (szInChar == g_szAltDirectorySeparatorChar);

} // End IsDirectorySeparatorChar

///////////////////////////////////////////////////////////////////////
std::string GetPathRoot(string szPath)
{
	if (szPath.length() == 0) {
		return _T("");
    }

	if (!IsPathRooted(szPath) ) {
		return _T("");
    }

	if (g_szDirectorySeparatorChar == '/') {
		// UNIX
		return IsDirectorySeparatorChar(szPath[0]) ? g_szDirectorySeparator : _T("");
	}
	else {
		// Windows
		int nLength = 2;

		if (szPath.length() == 1 && IsDirectorySeparatorChar(szPath[0]) ) {
			return g_szDirectorySeparator;
	    }
		else if (szPath.length() < 2) {
			return _T("");
		}

		if (IsDirectorySeparatorChar(szPath[0]) && IsDirectorySeparatorChar(szPath[1])) {
			// UNC: \\server or \\server\share
			// Get server
			while (nLength < (int)szPath.length() && !IsDirectorySeparatorChar(szPath[nLength])) {
			    nLength++;
		    }

			// Get share
			if (nLength < (int)szPath.length()) {
				nLength++;
				while (nLength < (int)szPath.length() && !IsDirectorySeparatorChar(szPath[nLength])) {
				    nLength++;
				}
			}

            std::string szTemp = szPath.substr(2, nLength - 2);

            size_t nFoundPos = szTemp.find(g_szAltDirectorySeparatorChar);
            while (nFoundPos != std::string::npos) {
                szTemp[nFoundPos] = g_szDirectorySeparatorChar;
                nFoundPos = szTemp.find(g_szAltDirectorySeparatorChar, ++nFoundPos );
            }

			return g_szDirectorySeparator +
				   g_szDirectorySeparator + szTemp;
		}
		else if (IsDirectorySeparatorChar(szPath[0])) {
			// Path starts with '\' or '/'
			return g_szDirectorySeparator;
		}
		else if (szPath[1] == g_szVolumeSeparatorChar) {
			// C:\folder
			if (szPath.length() >= 3 && (IsDirectorySeparatorChar(szPath [2]))) {
			    nLength++;
			}
		}
		else {
	        std::string szCurrentDir = _T("");
            Util::GetWorkingDirectory(szCurrentDir);
			return szCurrentDir.substr(0, 2); // + szPath.substr (0, nLength);
	    }

		return szPath.substr(0, nLength);
	}

} // End GetPathRoot

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Creates a relative path from one file or folder to another.
//      Converted to C++ from Paul Welter's Weblog,
//      "Create a Relative path code snippet"
// Requires:
//      szFromDirectory: Contains the directory that defines the start
//                       of the relative path.
//      szToPath: Contains the path that defines the endpoint of the
//                relative path.
//      szRelativePath: returned relative path from the start directory
//                      and end path.
// Returns: true if successful, false otherwise.
bool Util::RelativePathTo(std::string szFromDirectory, std::string szToPath,
                          std::string& szRelativePath)
{

    if (szFromDirectory.length() == 0) {
        return false;
    }

    if (szToPath.length() == 0) {
        return false;
    }

    bool bIsRooted = IsPathRooted(szFromDirectory)
        && IsPathRooted(szToPath);

    if (bIsRooted) {
        bool bIsDifferentRoot = strcmp(GetPathRoot(szFromDirectory).c_str(),
                                       GetPathRoot(szToPath).c_str()) != 0;

        if (bIsDifferentRoot) {
            szRelativePath = szToPath;
            return true;
        }
    }

    std::vector<string> relativePathList;

    std::vector<string> fromDirectoriesList;
    std::vector<string> toDirectoriesList;

    StringUtil::SplitString(szFromDirectory, g_szDirectorySeparator,
        fromDirectoriesList, false);
    StringUtil::SplitString(szToPath, g_szDirectorySeparator,
        toDirectoriesList, false);

    int nLength = min(
        fromDirectoriesList.size(),
        toDirectoriesList.size());

    int nLastCommonRoot = -1;

    // find common root
    int nIndex;
    for (nIndex = 0; nIndex < nLength; nIndex++) {
        if (0!=strcmp(fromDirectoriesList[nIndex].c_str(),
                      toDirectoriesList[nIndex].c_str())) {
            break;
        }

        nLastCommonRoot = nIndex;
    }

    if (nLastCommonRoot == -1) {
        szRelativePath = szToPath;
        return true;
    }

    // Add relative folders in "from" path
    for (nIndex = nLastCommonRoot + 1; nIndex < (int)fromDirectoriesList.size(); nIndex++) {
        if (fromDirectoriesList[nIndex].size() > 0) {
            relativePathList.push_back("..");
        }
    }

    // Add to folders to path
    for (nIndex = nLastCommonRoot + 1; nIndex < (int)toDirectoriesList.size(); nIndex++) {
        relativePathList.push_back(toDirectoriesList[nIndex]);
    }

    // Create relative path
    std::vector<string> relativePartsList;
    relativePartsList.assign(relativePathList.begin(), relativePathList.end());

    std::string szNewPath = _T("");
    std::vector<string>::iterator itBegin = relativePartsList.begin();
    std::vector<string>::iterator itEnd = relativePartsList.end();

    // Append first path segment
    if (itBegin != itEnd)  {
        szNewPath += (*itBegin);
        ++itBegin;
    }

    for (; itBegin != itEnd; ++itBegin) {
        // Add separator
        szNewPath += g_szRelativeDirectorySeparator;

        // Add the path segment
        szNewPath += (*itBegin);
    }

    // For Diomede purposes, the relative path does not contain the
    // file portion and is preceded by a //.
	size_t posLastBackslash = szNewPath.find_last_of(g_szDirectorySeparatorChar);

	#ifdef WIN32
	    size_t posLastSlash = szNewPath.find_last_of(_T("/"));
	#else
    	size_t posLastSlash = szNewPath.find_last_of(_T("\\"));
	#endif
	size_t posLastSeparator = max((int)posLastBackslash, (int)posLastSlash);

    if ((int)posLastSeparator > 0) {
	    szRelativePath = szNewPath.substr(0, posLastSeparator);
	}
	else {
	    szRelativePath = _T("");
	}

    /*
	if (IsDirectorySeparatorChar(szRelativePath[0]) && IsDirectorySeparatorChar(szRelativePath[1])) {
        return true;
	}

    if (IsDirectorySeparatorChar(szRelativePath[0])) {
		szRelativePath = g_szDirectorySeparator + szRelativePath;
        return true;
    }
    */

	szRelativePath = g_szRelativeDirectorySeparator + g_szRelativeDirectorySeparator + szRelativePath;
    return true;

} // End RelativePathTo

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Determines if the file path is is a directory.
// Requires:
//      szFileName: directory path
// Returns: true if it is a valid directory, false otherwise
bool Util::IsDirectory(const char* szFileName)
{
    #ifdef WIN32
        // We need Windows only code here to handle network shares
        // e.g. \\machine-name\folder
        const DWORD dwAttr = ::GetFileAttributes(szFileName);
        if (INVALID_FILE_ATTRIBUTES != dwAttr) {

            if ( (FILE_ATTRIBUTE_DIRECTORY & dwAttr) &&
                  0 != _tcscmp(_T("."), szFileName) &&
                  0 != _tcscmp(_T(".."), szFileName))
            {
                return true;
            }
        }
        return false;
    #else
        struct stat theStats;
        if (stat(szFileName, &theStats) != 0) return false;
        return ((theStats.st_mode & S_IFDIR) != 0);
    #endif

} // End IsDirectory

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Return the current working directory.
// Requires:
//      szCurrentDir: returned current directory, empty on failure
// Returns: true if successful, false otherwise
bool Util::GetWorkingDirectory(std::string& szCurrentDir)
{
    szCurrentDir = _T("");

#ifdef WIN32
	char szDirBuffer[_MAX_PATH];
	::GetCurrentDirectory(_MAX_PATH, szDirBuffer);
	szCurrentDir = szDirBuffer;
#else
   long lMaxPath;
   char* szDirBuffer;

   if ((lMaxPath = pathconf(".", _PC_PATH_MAX)) == -1) {
      // Failed to determine the pathname length
      return false;
   }
   if ((szDirBuffer = (char *) malloc(lMaxPath)) == NULL) {
      // Failed to allocate space for pathname
      return false;
   }
   if (getcwd(szDirBuffer, lMaxPath) == NULL) {
      // Failed to get current working directory
      return false;
   }

	szCurrentDir = szDirBuffer;

#endif

    return true;

} // End GetWorkingDirectory

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Sets the current working directory.
// Requires:
//      szCurrentDir: current directory
// Returns: true if successful, false otherwise
bool Util::SetWorkingDirectory(std::string szCurrentDir)
{

#ifdef WIN32
	::SetCurrentDirectory(szCurrentDir.c_str());
#else
   if (chdir(szCurrentDir.c_str()) == 0) {
      return false;
   }
#endif

    return true;

} // End SetWorkingDirectory

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to return the Linux "home" folder
// Requires:
//      szHomeDir: returned home directory, empty on failure
// Returns: true if successful, false otherwise
#ifndef WIN32
bool GetHomeDirectory(std::string& szHomeDir)
{
    char *szTempHomeDir;
    struct passwd *pwd;

    if ( (szTempHomeDir = getenv(_T("HOME")) )) {
        szHomeDir = std::string(szTempHomeDir);
    }
    else {
        if ((szTempHomeDir = getenv("USER"))) {
            pwd = getpwnam(szTempHomeDir);
        }
        else {
            pwd = getpwuid(getuid());
        }
        if (pwd) {
            szHomeDir = std::string(pwd->pw_dir);
        }
        else {
            szHomeDir = _T(".");
        }
    }
    return true;

} // End GetHomeDirectory
#endif

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Returns the specified system folder.
// Requires:
//      nFolder: CSIDL OR REFKNOWNFOLDERID (Vista and higher)
//      szSubPath: folder to add onto the special system folder.
//      szDataDir: returned path
// Returns: true if successful, false otherwise
#ifdef WIN32
bool Util::GetSystemDirectory(int nFolder, LPCTSTR szSubPath, std::string& szDataDir)
{
    szDataDir = _T("");

	std::string szTempDir = _T("");
	char szPath[MAX_PATH];

	BOOL bSuccess = ::SHGetSpecialFolderPath( NULL, &szPath[0], nFolder, true);
	if (bSuccess == FALSE ) {
	    return false;
	}

	szTempDir = szPath;
	szTempDir += szSubPath;

	char szCurrentDir[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, &szCurrentDir[0]);

	bool bExists = DoesFileExist(szTempDir.c_str());

	if ( !bExists) {
		if (::CreateDirectory(szTempDir.c_str(), NULL) == false) {
			// Failed to create DoesFileExist directory - perhaps out of space
			szTempDir = _T(".\\");
		}
	}

    // Reset our current directory
	::SetCurrentDirectory(szCurrentDir);
    szDataDir = szTempDir;

	return true;

} // End GetSystemDirectory
#endif

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Return the current data directory.
// Requires:
//      szDataDir: returned data directory, empty on failure
// Returns: true if successful, false otherwise
bool Util::GetDataDirectory(std::string& szDataDir)
{
	bool bSuccess = true;

    szDataDir = _T("");
    std::string szSlash = _T("\\");

    #ifdef WIN32
	    if ( GetSystemDirectory(CSIDL_APPDATA, _T("\\Diomede"), szDataDir) == false ) {
	        return false;
	    }

    	szDataDir += szSlash + _T("Data");

    #else
        // Use the user's home folder for storing the data.
        szSlash = _T("/");

        std::string szHomeDir = _T("");
        GetHomeDirectory(szHomeDir);

        szDataDir = szHomeDir + szSlash + _T(".diocli");

        if (false == IsDirectory(szDataDir.c_str() ) ) {
            if (mkdir(szDataDir.c_str(), 0777) == -1) {
                if (errno == ENOENT ) {
                    bSuccess = false;
                }
            }
        }

    #endif

	// Make sure the directory exists....
    if (false == IsDirectory(szDataDir.c_str() ) ) {
        #ifdef WIN32
            if (_mkdir(szDataDir.c_str()) == -1) {
                if (errno != EEXIST ) {
                    bSuccess = false;
                }
            }
        #else
            if (mkdir(szDataDir.c_str(), 0777) == -1) {
                if (errno == ENOENT ) {
                    bSuccess = false;
                }
            }
        #endif
    }

    return bSuccess;

} // End GetDataDirectory

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Looks for the configuration file, creates it if it doesn't exist.
// Requires:
//      szFilePath: file path
//      bPrependAppDir: pre-append the application directory.
//      If true, the supplied file path has the application prepended.
//		     filename.ext
//           subdir/filename.ext
//      If false, name will be relative to application directory or can
//      fully qualified.
// Returns: 0 if successful, errno otherwise
int Util::CreateConfig( const std::string szFilePath, bool bPrependAppDir /*true*/ )
{
	FILE* configfile = NULL;
	std::string szFullPath = _T("");

    std::string szSlash = _T("\\");
    #ifndef WIN32
        szSlash = _T("/");
    #endif

	if ( true == bPrependAppDir ) {
		Util::GetWorkingDirectory(szFullPath);
		szFullPath += szSlash;
	}

	szFullPath += szFilePath;

	int nReturn = 0;

    configfile = fopen( (TCHAR*)szFullPath.c_str(), _T("wt"));

	if (configfile != NULL) {
		fclose(configfile);
	}
	else {
	    nReturn = errno;
	}

	return nReturn;

} // End CreateConfig

/** @} */

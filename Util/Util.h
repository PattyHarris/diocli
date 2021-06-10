/*********************************************************************
 * 
 *  file:  Util.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Contains various utility functions.
 * 
 *********************************************************************/
//! \ingroup util
//! @{

#ifndef __UTIL_H__
#define __UTIL_H__

#include "Stdafx.h"

#include <queue>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

using namespace std;

    //-----------------------------------------------------------------
    // Keyboard constants (may need refining for other platforms
    //-----------------------------------------------------------------
    namespace UtilKeys {
        typedef enum {
            keyBack     = 0x0008,
            keyEsc      = 0x001b,
            keyHome     = 0x0047,
            keyUp       = 0x0048,
            keyPgUp     = 0x0049,
            keyLeft     = 0x004b,
            keyRight    = 0x004d,
            keyEnd      = 0x004f,
            keyDn       = 0x0050,
            keyPgDn     = 0x0051,
            keyDel      = 0x0053,
            // ...
            keyF1       = 0x013b,
            keyF2       = 0x013c,
            keyF3       = 0x013d,
            keyF4       = 0x013e,
            // ...
            keyHomeExt  = 0x0247,
            keyUpExt    = 0x0248,
            keyPgUpExt  = 0x0249,
            keyLeftExt  = 0x024b,
            keyRightExt = 0x024d,
            keyDownExt  = 0x0250,
            keyPgDnExt  = 0x0251,
            keyInsert   = 0x0252,
            keyDelExt   = 0x0253,
            keyEndExt   = 0x024f
        } KEY_TYPES;
    }
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// Util Class

class Util
{

public:
	Util() {};
	~Util() {};


    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Custom getch to provide a *nix solution to getch (which is
    //      not available by all stdio.h's.  The Window's version of
    //      reads a single character from the console without echoing.
    // Requires: nothing
    // Returns: int reresenting the character read
    static int CustomGetch(void);

	//-----------------------------------------------------------------
	// Helper file functions
	//-----------------------------------------------------------------

    ///////////////////////////////////////////////////////////////////////
    // Purpose: Helper function to suspend our process to give control
    //          to other running processes.
    // Requires:
    //      nMilliseconds: sleep amount in milliseconds.
    // Returns: nothing
    static void PauseProcess( unsigned int nMilliseconds);

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Parses the filename and extension from the filepath.
    // Requires:
    //      szFilePath: complete file path
    //      szFileName: file name + extension.
    // Returns: true if successful, false otherwise
    static bool GetFileName(const std::string szFilePath, std::string& szFileName);

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Finds the parent directory from the path.
    // Requires:
    //      szFilePath: complete file path
    //      szParentDir: parent directory
    // Returns: nothing
    static void GetParentDirFromDirPath(const std::string& szFilePath, std::string& szParentDir);

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
    static char* GetRelativeFilename(char *szCurrentDirectory, char *szAbsoluteFilename);

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
    static const char InvalidPathCharsList[15];

    static bool RelativePathTo(std::string szFromDirectory, std::string szToPath,
                               std::string& szRelativePath);

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Determines if the file path is valid.
    // Requires:
    //      szPath: complete file path
    // Returns: true if the file exists, false otherwise
    static bool DoesFileExist(std::string szPath) {
        #if defined(linux) || defined(__linux__) || defined(__APPLE__)
            struct stat bufStat;
            return stat(szPath.c_str(), &bufStat) != -1;
        #elif defined(_WIN32) // else, use Win32
            return GetFileAttributes(szPath.c_str()) != INVALID_FILE_ATTRIBUTES;
        #else
            return false;
        #endif
    }

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Determines if the file path is valid.
    // Requires:
    //      szFileName: complete file path
    // Returns: true if the file exists, false otherwise
    static bool DoesFileExist(const char* szFileName)
    {
        struct stat theStats;
        return (stat(szFileName, &theStats) == 0);
    }

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Determines if the file path is is a directory.
    // Requires:
    //      szFileName: directory path
    // Returns: true if it is a valid directory, false otherwise
    static bool IsDirectory(const char* szFileName);

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Returns the current working directory.
    // Requires:
    //      szCurrentDir: returned directory path
    // Returns: true if successful, false otherwise
    static bool GetWorkingDirectory(std::string& szCurrentDir);

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Sets the current working directory.
    // Requires:
    //      szCurrentDir: current directory
    // Returns: true if successful, false otherwise
    static bool SetWorkingDirectory(std::string szCurrentDir);

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Returns the specified system folder.
    // Requires:
    //      nFolder: CSIDL OR REFKNOWNFOLDERID (Vista and higher)
    //      szSubPath: folder to add onto the special system folder.
    //      szDataDir: returned path
    // Returns: true if successful, false otherwise
    #ifdef WIN32
        static bool GetSystemDirectory(int nFolder, LPCTSTR szSubPath, std::string& szDataDir);
    #endif

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Return the current data directory.
    // Requires:
    //      szDataDir: returned data directory, empty on failure
    // Returns: true if successful, false otherwise
    static bool GetDataDirectory(std::string& szDataDir);

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Returns the file length.
    // Requires:
    //      szFileName: input file
    // Returns: file length is successful, -1 on failures
    static double GetFileLength(const char* szFileName)
    {
        struct stat theStats;

        if (stat(szFileName, &theStats) == 0) {
            return theStats.st_size;
        }
        return -1;
    }

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Returns the file length using int64.
    // Requires:
    //      szFileName: input file
    // Returns: file length is successful, -1 on failures
    static LONG64 GetFileLength64(const char* szFileName)
    {
        #ifdef WIN32
            struct _stat64 theStats;

            if (_stati64(szFileName, &theStats) == 0) {
                return theStats.st_size;
            }
        #else
            // TBD: Linux -
            struct stat theStats;

            if (stat(szFileName, &theStats) == 0) {
                return theStats.st_size;
            }
        #endif
        return -1;
    }

    ///////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Returns the last modified time for a file.
    // Requires:
    //      szFileName: input file
    //      tModifiedTime: file last modified time
    // Returns: file length if successful, -1 on failures
    static int GetFileLastModifiedTime(const char* szFileName, time_t& tModifiedTime)
    {
        tModifiedTime = 0;

        #ifdef WIN32
            struct _stat theStats;
            int nFileDescriptor, nResult;

            nFileDescriptor = _sopen( szFileName, _O_RDONLY, _SH_DENYNO, S_IREAD );
            if ( nFileDescriptor == -1 ) {
                return errno;
            }

            // Get data associated with "fd":
            nResult = _fstat( nFileDescriptor, &theStats );

            // Check if statistics are valid:
            if ( nResult != 0 ) {
                return errno;
            }

            tModifiedTime = theStats.st_mtime;
            _close( nFileDescriptor );

            return 0;
        #else
            // Linux -
            struct stat theStats;

            if (stat(szFileName, &theStats) == 0) {
                tModifiedTime = theStats.st_mtime;
            }
            return 0;
        #endif
    }

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
    static int CreateConfig( const std::string szFilePath, bool bPrependAppDir = true);

}; // End Util

/** @} */

#endif // __UTIL_H__

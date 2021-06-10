/*********************************************************************
 * 
 *  file:  BuildVersionUtils.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Set of build version utility functions
 * 
 *********************************************************************/

#ifndef BUILD_VERSION_UTILS_H
#define BUILD_VERSION_UTILS_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <string>

/////////////////////////////////////////////////////////////////////////////
// BuildUtil Class

class BuildUtil
{

// Construction
private:

public:
	BuildUtil();
	virtual ~BuildUtil();

///////////////////////////////////////////////////////////////////
    //  Action   Returns the client platform - "Win32", or whatever
    //  Params:  string to hold the client platform type.
    //  Returns  true if successful, false otherwise.
	static bool GetClientPlatform(string& strBuildClientPlatform);

///////////////////////////////////////////////////////////////////
    //  Action   Returns the client version - Returns "1.0 Dev" for dev builds.
	//           For release candidates, returns the release version number
	//           (e.g. 1.0 Beta 4).
    //  Params:  string to hold the client version.
    //  Returns  true if successful, false otherwise.
	static bool GetClientVersion(string& strClientVersion);

///////////////////////////////////////////////////////////////////
    //  Action   Returns the build number.
    //  Params:  string to hold the build number.
    //  Returns  true if successful, false otherwise.
	static bool GetClientBuildNumber(string& strBuildNumber);

///////////////////////////////////////////////////////////////////
    //  Action   Returns the build display string.
    //  Params:  string to hold the build display string.
    //  Returns  true if successful, false otherwise.
	static bool GetClientBuildDisplayName(string& strBuildDisplayName);

///////////////////////////////////////////////////////////////////
    //  Action   Returns the client build date.
    //  Params:  string to hold the client build date.
    //  Returns  true if successful, false otherwise.
	static bool GetClientBuildDate(string& strBuildDate);

///////////////////////////////////////////////////////////////////
    //  Action   Returns the client build time.
    //  Params:  string to hold the client build time.
    //  Returns  true if successful, false otherwise.
	static bool GetClientBuildTime(string& strBuildTime);

///////////////////////////////////////////////////////////////////
    //  Action   Returns the client build timestamp - date + time.
    //  Params:  string to hold the client build stamp.
    //  Returns  true if successful, false otherwise.
	static bool GetClientBuildTimeStamp(string& strBuildBuidTimeStamp);

///////////////////////////////////////////////////////////////////
    //  Action   Returns the complete build information - version + build
	//           Does not return the timestamp
    //  Params:  string to hold the client build data.
    //  Returns  true if successful, false otherwise.
	static bool GetClientBuildInfo(string& strBuildInfo);

//////////////////////////////////////////////////////////////////////////
	//  Action   Returns the complete version information - version + build
	//  Params:  string to hold the client version + build
	//  Returns  true if successful, false otherwise.
    static bool GetClientVersionDisplayInfo(string& strVersionInfo);

///////////////////////////////////////////////////////////////////
    //  Action   Returns the Diomede server location specific address
    //  Params:  string to hold the Diomede server address
    //  Returns  true if successful, false otherwise.
	static bool GetClientProvAddress(string& strProvServerAddress);

///////////////////////////////////////////////////////////////////
    //  Action   Returns Diomede install exe size
    //  Params:  int to hold the install file size
    //  Returns  true if successful, false otherwise.
	static bool GetReveaInstallFileSize(int& nInstallExeFileSize);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

};

#endif //  BUILD_VERSION_UTILS_H




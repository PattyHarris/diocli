/*********************************************************************
 * 
 *  file:  BuildVersionUtils.cpp
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Provides build versioning utilities
 * 
 * 
 *********************************************************************/

#include "Stdafx.h"
#include "../Include/BuildVersionUtils.h"
#include "../Include/BuildVersion.h"


/////////////////////////////////////////////////////////////////////////////
//  BuildUtil Constructor
//
BuildUtil::BuildUtil()
{
} // End of constructor

/////////////////////////////////////////////////////////////////////////////
//  BuildUtil Destructor
//
BuildUtil::~BuildUtil()
{

}

//////////////////////////////////////////////////////////////////////////
    //  Action   Returns the client platform - "Windows", "MAC", whatever
    //  Params:  string to hold the client platform type.
    //  Returns  true if successful, false otherwise.
bool BuildUtil::GetClientPlatform(string& strClientPlatform)
{
    strClientPlatform = "<Unknown client platform>";
#ifdef WIN32
    strClientPlatform = "Win32";
#else
    strClientPlatform = "Linux";
#endif
	return true;
}

//////////////////////////////////////////////////////////////////////////
    //  Action   Returns the client version - Returns "Dev" for dev builds.
	//           For release candidates, returns the release version number
	//           (e.g. 1.0 Beta 4).
    //  Params:  string to hold the client version.
    //  Returns  true if successful, false otherwise.
bool BuildUtil::GetClientVersion(string& strClientVersion)
{
    strClientVersion = _T("<Unknown client version>");

    char szTmp[100];
	strcpy(szTmp, VERINFO_VERSIONNAME);

    string szVersion = szTmp;
	if (szVersion.size() == 0) {
	    return false;
	}

	if ( -1 != (int)szVersion.find(_T("UNDEFINED")) ) {
	    return false;
	}

    // Parse up to a "(" - currently assumes the build number follows
	// with "()"
	int nIndex = (int)szVersion.find(_T("("));

	if (nIndex < 0) {
	    strClientVersion = szVersion;
	}
	else {
	    strClientVersion = szVersion.substr(0, nIndex);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
    //  Action   Returns the build number.
    //  Params:  string to hold the build number.
    //  Returns  true if successful, false otherwise.
bool BuildUtil::GetClientBuildNumber(string& strBuildNumber)
{
    strBuildNumber = _T("<Unknown build number>");

	int nBuild = VERINFO_BUILDNUMBER;

	if (nBuild < 0) {
	    return false;
	}

    char szTmp[100];
    sprintf( szTmp, _T("%d"), VERINFO_BUILDNUMBER );

	strBuildNumber = szTmp;
	return true;
}

//////////////////////////////////////////////////////////////////////////
    //  Action   Returns the build display string.
    //  Params:  string to hold the build display string.
    //  Returns  true if successful, false otherwise.
	//
bool BuildUtil::GetClientBuildDisplayName(string& strBuildDisplayName)
{
    strBuildDisplayName = _T("<Unknown build number>");

	// BrancID should contain either 1.4 for a release branch or Dev
	// for the dev branch.
	string szBranchID;

    char szTmp[100];
    sprintf( szTmp, _T("%s"), VERINFO_BRANCHID );
    szBranchID = szTmp;

	if (szBranchID.size() == 0) {
	    return false;
	}

	if (szBranchID.find(_T("UNDEFINED")) != string::npos) {
	    return false;
	}

	string szBuildNumber;
	GetClientBuildNumber(szBuildNumber);

	if (szBuildNumber.find(_T("unknown")) != string::npos) {
		return false;
	}

	// For Dev builds, we working with dev and 524 - this will
	// result in a string dev.524

	// For Release builds, we working with 1.4 and 10 - this will
	// result in a string 1.4.10

	string szTmpBuildDisplayName;

	strcpy((char*)szTmpBuildDisplayName.c_str(), (char*)szBranchID.c_str());
	strcat((char*)szTmpBuildDisplayName.c_str(), _T("."));
	strcat((char*)szTmpBuildDisplayName.c_str(), (char*)(szBuildNumber.c_str()));

	strcpy((char*)strBuildDisplayName.c_str(), (char*)szTmpBuildDisplayName.c_str());
	return true;
}

//////////////////////////////////////////////////////////////////////////
    //  Action   Returns the client build date.
    //  Params:  string to hold the client build date.
    //  Returns  true if successful, false otherwise.
bool BuildUtil::GetClientBuildDate(string& strBuildDate)
{
    char szTmp[100];
    sprintf( szTmp, _T("%s"), VERINFO_BUILDDATE );
	strBuildDate = szTmp;
	return true;
}

//////////////////////////////////////////////////////////////////////////
    //  Action   Returns the client build date.
    //  Params:  string to hold the client build date.
    //  Returns  true if successful, false otherwise.
bool BuildUtil::GetClientBuildTime(string& strBuildTime)
{
    char szTmp[100];
    sprintf( szTmp, _T("%s"), VERINFO_BUILDTIME );
	strBuildTime = szTmp;
	return true;
}

//////////////////////////////////////////////////////////////////////////
    //  Action   Returns the client build timestamp - date + time.
    //  Params:  string to hold the client build stamp.
    //  Returns  true if successful, false otherwise.
bool BuildUtil::GetClientBuildTimeStamp(string& strBuildTimeStamp)
{
	char szBuildDate[100];
	char szBuildTime[100];

	strcpy(szBuildDate, VERINFO_BUILDDATE);
	strcpy(szBuildTime, VERINFO_BUILDTIME);

	char szTmp[MAX_PATH];

	strcpy(szTmp, szBuildDate);
	strcat(szTmp, _T(" "));
	strcat(szTmp, szBuildTime);

    strBuildTimeStamp = szTmp;

	return true;
}

//////////////////////////////////////////////////////////////////////////
	//  Action   Returns the complete build information - version + build
	//           Does not return the timestamp
	//  Params:  string to hold the client build data.
	//  Returns  true if successful, false otherwise.
bool BuildUtil::GetClientBuildInfo(string& strBuildInfo)
{
    strBuildInfo = _T("<Unknown build>");

    char szTmp[100];
    sprintf( szTmp, _T("%s"), VERINFO_VERSIONNAME );
    string szBuildInfo = szTmp;

	if (szBuildInfo.size() == 0) {
	    return false;
	}

	if (szBuildInfo.find(_T("UNDEFINED")) != string::npos) {
	    return false;
	}

	// Look for "Build" in the string - currently the build data is part of
	// the version name.
	if ( szBuildInfo.find(_T("build")) != string::npos ) {
	     strBuildInfo = szBuildInfo;
		 return true;
	}

    // Otherwise, build the information from the version number, version name
	// and build number.
	char szTmpBuildInfo[200];
    sprintf( szTmpBuildInfo, _T("%s (Build %d)"), VERINFO_VERSIONNAME, VERINFO_BUILDNUMBER);

	strBuildInfo = szTmpBuildInfo;

	return true;
}

//////////////////////////////////////////////////////////////////////////
	//  Action   Returns the complete version information - version + build
	//  Params:  string to hold the client version + build
	//  Returns  true if successful, false otherwise.
bool BuildUtil::GetClientVersionDisplayInfo(string& strVersionInfo)
{
    strVersionInfo = _T("<Unknown version>");

    char szTmp[100];
    sprintf( szTmp, _T("%s"), VERINFO_VERSIONNAME );
    string szVersionInfo = szTmp;

	if (szVersionInfo.size() == 0) {
	    return false;
	}

	if (szVersionInfo.find(_T("UNDEFINED")) != string::npos) {
	    return false;
	}

	// Look for "Build" in the string - currently the build data is part of
	// the version name - 6/13/03 - doesn't appear that build is part
    // of this string..
    /*
	if ( szVersionInfo.find("build") != string::npos ) {
	     strBuildInfo = szBuildInfo;
		 return true;
	}
    */

    // Otherwise, build the information from the version number + build number.

	string szBuildNumber;
	GetClientBuildNumber(szBuildNumber);

	char szTmpVersionInfo[100];

	strcpy(szTmpVersionInfo, szVersionInfo.c_str());
	strcat(szTmpVersionInfo, _T("."));
	strcat(szTmpVersionInfo, szBuildNumber.c_str());

	strVersionInfo = szTmpVersionInfo;

	return true;
}

///////////////////////////////////////////////////////////////////
    //  Action   Returns the Diomede server location specific address
    //  Params:  string to hold the server address
    //  Returns  true if successful, false otherwise.
bool BuildUtil::GetClientProvAddress(string& strProvServerAddress)
{
	strProvServerAddress = _T("Diomede.com");

    char szTmp[100];
    sprintf(szTmp, _T("%s"), VERINFO_WEBADDRESS);
    string szAddress = szTmp;

	if (szAddress.size() == 0) {
	    return false;
	}

	if (szAddress.find(_T("UNDEFINED")) != string::npos) {
	    return false;
	}

	strProvServerAddress = szAddress;
	return true;
}

///////////////////////////////////////////////////////////////////
    //  Action   Returns Diomede install exe size
    //  Params:  int to hold the install file size
    //  Returns  true if successful, false otherwise.
bool BuildUtil::GetReveaInstallFileSize(int& nInstallExeFileSize)
{
	int nBuild = VERINFO_FILESIZE;

	if (nBuild < 0) {
	    return false;
	}

	nInstallExeFileSize = nBuild;
	return true;
}


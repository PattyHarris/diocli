/*********************************************************************
 * 
 *  file:  UserProfileData.cpp
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Client interface to the profile data.
 * 
 *********************************************************************/

#include "Stdafx.h"

#ifdef  WIN32
#pragma warning (disable:4786)  /* identifier truncated to 255 characters */
#endif

#include "UserProfileData.h"
#include "dictionary.h"
#include "ClientLog.h"
#include "StringUtil.h"

#include "ClientLog.h"
#include "ErrorCodes/UtilErrors.h"
#include "Util.h"

#ifdef  WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <iostream>

using namespace ::std;

#define MAX_KEY_LENGTH 256

char* UserProfileData::ApplicationName = _T("");

// #define USE_REGISTRY

/////////////////////////////////////////////////////////////////////////////

/*
*/
UserProfileData::~UserProfileData()
{

} // End Destructor

/*
 * NAME:        Init
 * ACTION:      Initialize the configuration file.  Handled
 *              by the derived class.
 * PARAMETERS:  char* FileName - the name of the file to initialize.
 */
void UserProfileData::Init(char* szFileName)
{
    m_strFilename = szFileName;
    
    SetUserProfileInt(GEN_AUTO_LOGIN, GEN_AUTO_LOGIN_DF);
    SetUserProfileStr(UI_LASTLOGIN, _T(""));
    SetUserProfileInt(GEN_AUTO_LOGOUT, GEN_AUTO_LOGOUT_DF);

    SetUserProfileStr(GEN_SERVICE_ENDPOINT, GEN_SERVICE_ENDPOINT_DF);
    SetUserProfileStr(GEN_SECURE_SERVICE_ENDPOINT, GEN_SECURE_SERVICE_ENDPOINT_DF);

    SetUserProfileStr(GEN_TRANSFER_ENDPOINT, GEN_TRANSFER_ENDPOINT_DF);
    SetUserProfileStr(GEN_SECURE_TRANSFER_ENDPOINT, GEN_SECURE_TRANSFER_ENDPOINT_DF);

    SetUserProfileInt(GEN_SERVICE_SECURE_TYPE, GEN_SERVICE_SECURE_TYPE_DF);
    SetUserProfileInt(GEN_SEND_TIMEOUT, GEN_SEND_TIMEOUT_DF);
    SetUserProfileInt(GEN_RECV_TIMEOUT, GEN_RECV_TIMEOUT_DF);
    SetUserProfileInt(GEN_CONNECT_TIMEOUT, GEN_CONNECT_TIMEOUT_DF);

    SetUserProfileStr(GEN_RESULT_PAGE_SIZE, GEN_RESULT_PAGE_SIZE_DF);
    SetUserProfileStr(GEN_RESULT_PAGE, GEN_RESULT_PAGE_DF);
    SetUserProfileInt(GEN_VERBOSE_OUTPUT, GEN_VERBOSE_OUTPUT_DF);
    
    if (m_bCreateConfigFile) {
        Save();
    }

} // End Init

/////////////////////////////////////////////////////////////////////////////

/*
* NAME:        GetUserProfile
* ACTION:      Loads the configuration from either the named file
*              or window's registry and user DB.
*
* PARAMETERS:  const char* szFileName - the name of the configuration file
*              const char* szUserName - user name required for DB access
*              const char* szPassword - user's password
*              const char* szProfileGroup - optional group for the data.
*              Used only in Windows.
*              bool bCreateConfigFile - true to create the configuration file,
*              false otherwise. 
* RETURNS:     Error indicating the result.
*/
ErrorType UserProfileData::GetUserProfile(const char* szUserName /*_T("")*/,
                                          const char* szPassword /*_T("")*/,
                                          const char* szFileName /*_T("")*/,
										  const char* szProfileGroup /*_T("")*/,
										  bool bCreateConfigFile /*true*/)
{
    m_szUserName = szUserName;
    m_szUserPassword = szPassword;
    m_szProfileGroup = szProfileGroup;
    m_bCreateConfigFile = bCreateConfigFile;
    
    m_bReadSuccessful = true;

    m_etLastError = ERR_NONE;
    m_nLastError = 0;

#ifdef USE_REGISTRY

    // For windows, read the profile data from the registry.
    bool bReturn = GetRegistryProfileData();

	if (!bReturn) {
	    return ERR_IO;
	}

#endif

#ifdef WIN32
    if ( szFileName ) {

    	#ifdef USE_REGISTRY
            char szAppPath[MAX_PATH];
            string szDirectory;
            if (GetUserProfileRegistryStr( GEN_USER_APP_PATH, szAppPath )) {
                szDirectory = szAppPath;
                 m_szFilename = szDirectory + _T("\\") + string(szFileName);
            }
            else {
                 m_szFilename = szFileName;
            }
        #else
	        std::string szFullPath = _T("");

            // If we're creating the configuration file using
            // our data directory...otherwise, we'll look
            // locally for the file (as in the Windows CPPSDK.DLL.Test
            // scenario.
            if (m_bCreateConfigFile) {
	            Util::GetDataDirectory(szFullPath);
	            szFullPath += _T("\\");
	        }

	        szFullPath += szFileName;
            m_szFilename = szFullPath;
        #endif

        // Read can generate an error if the file can't be
        // created - but we'll let it go to allow usage of the class
        // as a static dictionary.
        Read((char*) m_szFilename.c_str());
    }
#else
    // May need some tweaking for Linux, Mac
    if ( *szFileName > 0 ) {

    	// Linux: we should be setting the path to ./diomede.dcf ?
        std::string szFullPath = _T("");

        Util::GetDataDirectory(szFullPath);
        szFullPath += _T("/");

        szFullPath += szFileName;
 
        // Read can generate an error if the file can't be
        // created - but we'll let it go to allow usage of the class
        // as a static dictionary.
        // m_szFilename = szFileName;
        m_szFilename = szFullPath;
        Read((char*)m_szFilename.c_str());
    }
#endif

	m_bReadDBProfileOK = false;

	// If the user name or password is empty - we assume that we'll skip access to
	// the database profile data -
        if ( ( m_szUserName.length() == 0 ) ||
	     ( m_szUserPassword.length() == 0 ) ) {
	    m_etLastError = ERR_NONE;
	    return ERR_NONE;
	}

	// Get the remaining data from the database.
	#if 0
    	m_etLastError = GetUserDBProfile(m_szUserName.c_str(),  m_szUserPassword.c_str());
	#else
        m_etLastError = ERR_NONE;
	#endif

	return m_etLastError;

} // End GetUserProfile

/*
* NAME:        SaveUserProfile
* ACTION:      Saves the profile data.  Server values are saved to
*              the user data.  The remaining are either saved to
*              the configuration file or to the registrry.
* RETURNS:     Error indicating the result.
*/
ErrorType UserProfileData::SaveUserProfile()
{
	#ifdef USE_REGISTRY
	    // Save the data to the registry.
		if (!SetRegistryProfileData() ) {
		   // TODO: Create appropriate error codes
		   return ERR_IO;
		}

		// Save the remaining data to the database - leaving this commented out
		// for now.  We may want to add it back in later...
		// return SaveUserDBProfile(m_szUserName.c_str(),  m_szUserPassword.c_str());
	#endif

    FILE *OutFile;
    string Name;
    
    m_etLastError = ERR_NONE;
    m_nLastError = 0;

    // open the file
    OutFile = _tfopen( m_szFilename.c_str(), _T("wt"));
    if (OutFile == NULL)
	{
		ClientLog(UTIL_COMPONENT_ERROR,ER, false, _T("Saving Config: Failed to write to %s."),  
		    m_szFilename.c_str());
		m_nLastError = errno;
		m_etLastError = ERR_IO;
        return m_etLastError;
	}

    // write it
    Dictionary<string>::iterator ThisName;
    for (ThisName = m_ValSet.begin(); ThisName != m_ValSet.end(); ThisName++)
    {
        Name = *ThisName;
        // ATLTRACE(_T("Save User config: %s %s\n"), ThisName.key().c_str(), Name.c_str());
        fprintf(OutFile, _T("%s = %s\n"), ThisName.key().c_str(), Name.c_str());
    }

    // close up
    fflush(OutFile);
    fclose(OutFile);
    
	// Update the user database fields.
	#if 0
	    m_etLastError = SaveUserDBProfile(m_szUserName.c_str(),  m_szUserPassword.c_str());
	#else
        m_etLastError = ERR_NONE;
	#endif

    return m_etLastError;
    
} // End SaveUserProfile

/*
* NAME:        SaveUserDBProfile
* ACTION:      Saves the DB specific profile data.
* RETURNS:     Error indicating the result.
*/
ErrorType UserProfileData::SaveUserDBProfile(const char* szUserName,
                                             const char* szPassword)
{
    m_etLastError = ERR_NOT_SUPPORTED;
	return m_etLastError;

} // End SaveUserDBProfile

/*
* NAME:        LogUserProfile
* ACTION:      Logs the contents of the configuration
*
* PARAMETERS:  none.
*/
void UserProfileData::LogUserProfile()
{
    string Name;

    Dictionary<string>::iterator ThisName;
    for (ThisName = m_ValSet.begin(); ThisName != m_ValSet.end(); ThisName++)
    {
        Name = *ThisName;
    }

} // End LogUserProfile


/*
* NAME:        GetUserProfileStr
* ACTION:      Get a configuration value
* PARAMETERS:  const char* szName - the name of the configuration parameter
*              const char* szDefault - optional default value
*              const bool bServer - if true, value is retreived from
*              the server.  Otherwise, the data is retreived either from
*              the configuration file or registry (Windows).
* RETURNS:     string - the value or Default if not found
*/

string UserProfileData::GetUserProfileStr( const char* szName,
                                           const char* szDefault /* NULL */,
                                           const bool bServer /* false */)
{
    if (bServer) {
		// TODO: for server side values, get the latest value from
		// the server.
	}

	string strDefault;

	if ( szDefault ) {
	    strDefault = szDefault;
	}


	return Get(szName, (char*)strDefault.c_str());

} // End GetUserProfileStr

#ifdef NO_ENCRYPTED_USER_PROFILE_KEYS
/*
 *   If NO_ENCRYPTED_USER_PROFILE_KEYS is defined the following two
 *   functions are aliased to the non-encrypted versions in
 *   UserProfileData.h
 */
/*
 * NAME:        SetUserProfileEncrypted
 * ACTION:      Serializes and encrypts the user data
 * PARAMETERS:  const char* szName - the name of the configuration parameter
 *              UserData& userData - contains the user data for serialization
 * RETURNS:     nothing
 */
void UserProfileData::SetUserProfileEncrypted(const char *szName, const UserData& userData)
{
    #ifdef WIN32
        FILETIME fileTime;
        fileTime.dwLowDateTime = fileTime.dwHighDateTime = 0;
        GetSystemTimeAsFileTime(&fileTime);

        m_dwEncryptLow  = fileTime.dwLowDateTime;
        m_dwEncryptHigh = fileTime.dwHighDateTime;

    #else
        timeval tv;
        gettimeofday(&tv,NULL);

        m_dwEncryptLow  = tv.tv_usec;
        m_dwEncryptHigh = 0;
    #endif

    SetUserProfileStr(UI_USERNAME, userData.UserName());
    SetUserProfileStr(UI_PASSWORD, userData.Password());

    // Set the keys as well, since they will be needed to decrypt the data.
    /**/
    SetUserProfileLong(UI_ENCRYPT_LOW, m_dwEncryptLow);
    SetUserProfileLong(UI_ENCRYPT_HIGH, m_dwEncryptHigh);
    /**/

} // End SetUserProfileEncrypted

/*
 * NAME:        GetUserProfileEncrypted
 * ACTION:      Returns the user profile string in an encrypted form.
 * PARAMETERS:  const char* szName - the name of the configuration parameter
 *              UserData& userData  - container for the deserialized data.
 * RETURNS:     true if successful, false otherwise
 */
bool UserProfileData::GetUserProfileEncrypted( const char *szName, UserData& userData)
{
    std::string szResult = _T("");

    // Get the keys first
    /**/
    m_dwEncryptLow = GetUserProfileLong(UI_ENCRYPT_LOW);
    m_dwEncryptHigh = GetUserProfileLong(UI_ENCRYPT_HIGH);
    /**/

    std::string szTemp = GetUserProfileStr(UI_USERNAME);
	userData.UserName(szTemp);

    szTemp = GetUserProfileStr(UI_PASSWORD);
	userData.Password(szTemp);

	return true;

} // End GetUserProfileEncrypted

#else
/*
* NAME:        SetUserProfileStrEncrypted
* ACTION:      Set a configuration value
* PARAMETERS:  char *szName - the name of the configuration parameter
*              char *szValue - the value
*/

void UserProfileData::SetUserProfileStrEncrypted(const char *szName, const char *szValue)
{
    #ifdef WIN32
        FILETIME fileTime;
        fileTime.dwLowDateTime = fileTime.dwHighDateTime = 0;
        GetSystemTimeAsFileTime(&fileTime);

        m_dwEncryptLow  = fileTime.dwLowDateTime;
        m_dwEncryptHigh = fileTime.dwHighDateTime;

    #else
        timeval tv;
        gettimeofday(&tv,NULL);

        m_dwEncryptLow  = tv.tv_usec;
        m_dwEncryptHigh = 0;
    #endif

	SetUserProfileInt(UI_CLIENT_LENGTH, (int)strlen(szValue));
	std::string szEncryptedData = StringUtil::EncryptString(m_dwEncryptLow, m_dwEncryptHigh,
	    szValue);
    SetUserProfileStr(szName, szEncryptedData.c_str());

    // Set the keys as well, since they will be needed to decrypt the data.
    SetUserProfileLong(UI_ENCRYPT_LOW, m_dwEncryptLow);
    SetUserProfileLong(UI_ENCRYPT_HIGH, m_dwEncryptHigh);

} // End SetUserProfileStrEncrypted

/*
 * NAME:        SetUserProfileEncrypted
 * ACTION:      Serializes and encrypts the user data
 * PARAMETERS:  const char* szName - the name of the configuration parameter
 *              UserData& userData - contains the user data for serialization
 * RETURNS:     nothing
 */
void UserProfileData::SetUserProfileEncrypted(const char *szName, const UserData& userData)
{
    #ifdef WIN32
        FILETIME fileTime;
        fileTime.dwLowDateTime = fileTime.dwHighDateTime = 0;
        GetSystemTimeAsFileTime(&fileTime);

        m_dwEncryptLow  = fileTime.dwLowDateTime;
        m_dwEncryptHigh = fileTime.dwHighDateTime;

    #else
        timeval tv;
        gettimeofday(&tv,NULL);

        m_dwEncryptLow  = tv.tv_usec;
        m_dwEncryptHigh = 0;
    #endif

    std::string szEncryptedData = Serialize(userData, true, m_dwEncryptLow, m_dwEncryptHigh);

    SetUserProfileStr(szName, szEncryptedData.c_str());

    // Set the keys as well, since they will be needed to decrypt the data.
    SetUserProfileLong(UI_ENCRYPT_LOW, m_dwEncryptLow);
    SetUserProfileLong(UI_ENCRYPT_HIGH, m_dwEncryptHigh);

} // End SetUserProfileEncrypted

/*
* NAME:        GetUserProfileStrEncrypted
* ACTION:      Get a configuration value
* PARAMETERS:  const char *szName - the name of the configuration parameter
*              const char *szDefault - optional default value
*              const bool bServer - if true, value is retreived from
*              the server.  Otherwise, the data is retreived either from
*              the configuration file or registry (Windows).
* RETURNS:     string - the value or Default if not found
*/
std::string UserProfileData::GetUserProfileStrEncrypted( const char *szName,
                                                         const char *szDefault /* NULL */,
                                                         const bool bServer /* false */)
{
    std::string szResult = _T("");

    // Get the keys first
    m_dwEncryptLow = GetUserProfileLong(UI_ENCRYPT_LOW);
    m_dwEncryptHigh = GetUserProfileLong(UI_ENCRYPT_HIGH);

    std::string szEncryptedData = GetUserProfileStr(UI_CLIENT_ENCRYPTED);
    if (szEncryptedData.length() == 0) {
        return false;
    }

	szResult = StringUtil::DecryptString(m_dwEncryptLow, m_dwEncryptHigh, szEncryptedData);
    return szResult;

} // End GetUserProfileStrEncrypted

/*
 * NAME:        GetUserProfileEncrypted
 * ACTION:      Returns the user profile string in an encrypted form.
 * PARAMETERS:  const char* szName - the name of the configuration parameter
 *              UserData& userData  - container for the deserialized data.
 * RETURNS:     nothing
 */
bool UserProfileData::GetUserProfileEncrypted( const char *szName, UserData& userData)
{
    std::string szResult = _T("");

    // Get the keys first
    m_dwEncryptLow = GetUserProfileLong(UI_ENCRYPT_LOW);
    m_dwEncryptHigh = GetUserProfileLong(UI_ENCRYPT_HIGH);

    std::string szEncryptedData = GetUserProfileStr(UI_CLIENT_ENCRYPTED);
    if (szEncryptedData.length() == 0) {
        return false;
    }

    bool bSuccess = Deserialize(szEncryptedData, true, m_dwEncryptLow, m_dwEncryptHigh, userData);
    return bSuccess;

} // End GetUserProfileEncrypted

/*
 *   Conversion of a userprofile into a serialised/deserialized format.
 *   This format is appropriate for http headers.
 */

const std::string DATUM_SEP     = _T("/");
const std::string VERSION_ID	= _T("0");

/*
* NAME:        Serialize
* ACTION:      Serialize and encrypt the user data
* PARAMETERS:  UserData& userData - contains the user data for serialization
*              bool bEncypt - true if encrypting
*              unsigned long i1 - key to encryption
*              unsigned long i2 - key to encryption
* RETURNS:     string - serialized result
*/
std::string UserProfileData::Serialize( const UserData& userData, bool bEncypt,
                                        unsigned long i1, unsigned long i2 )
{
	std::string szOutput;
	szOutput  =  VERSION_ID	+ DATUM_SEP;
	StringUtil::AppendIntToString(szOutput, (unsigned int)bEncypt, *DATUM_SEP.c_str());
	szOutput +=  userData.UserName() + DATUM_SEP;
	szOutput +=  userData.Password() + DATUM_SEP;

	#if 0
	// Currently, this data is not available to the client.
	szOutput +=  m_szLastName		+ DATUM_SEP;
	szOutput +=  m_szFirstName		+ DATUM_SEP;
	szOutput +=  m_szEmailAddress1	+ DATUM_SEP;
	szOutput +=  m_szEmailAddress2	+ DATUM_SEP;
	#endif

	if (bEncypt) {
    	SetUserProfileInt(UI_CLIENT_LENGTH, (int)szOutput.length());
		szOutput = StringUtil::EncryptString(i1, i2, szOutput);
	}

	return szOutput;

} // End Serialize

/*
* NAME:        Deserialize
* ACTION:      Deserialize and unencrypt the user data.
*              At this point all necessary hacks have taken place.
*              This list is postion dependant
* PARAMETERS:  listItems = vector of strings
*              startIndex = the first location to use INDEX NOT ITEM  eg starts @ 0
* RETURNS:     true if successful, false oterwise. - serialized result
*/
bool UserProfileData::Deserialize( const vector<std::string> listItems, UINT startIndex,
                                   UserData& userData)
{
	UINT index = startIndex;
	userData.UserName(listItems[index++]);
	userData.Password(listItems[index++]);

	#if 0
	m_szFirstName		= items[index++];
	m_szEmailAddress1	= items[index++];
	m_szEmailAddress2	= items[index++];
	#endif

	return true;

} // End Deserialize

/*
* NAME:        Deserialize
* ACTION:      Deserialize and unencrypt the user data.
*              At this point all necessary hacks have taken place.
*              This list is postion dependant
* PARAMETERS:  szSerializedData = data to deserialize
*              bEncrypted = true if encrypted
*              unsigned long i1 - key to encryption
*              unsigned long i2 - key to encryption
*              UserData& userData  - container for the deserialized data.
* RETURNS:     true if successful, false oterwise. - serialized result
*/
bool UserProfileData::Deserialize( const std::string szSerializedData,
                                   bool bEncrypted, unsigned long i1, unsigned long i2,
                                   UserData& userData)
{
	std::string szRawData = _T("");

	if (bEncrypted) {
	    int nOrigLength = GetUserProfileInt(UI_CLIENT_LENGTH, 0);
		StringUtil::DecryptString(i1, i2, szSerializedData, szRawData, nOrigLength);
	}
	else {
		szRawData = szSerializedData;
	}

	const UINT MINIMUM_NUMBER_ITEMS_IN_VALID_USER = 2;
	bool bSuccess = true;

	if ( (size_t)-1 == szRawData.find(DATUM_SEP)) {
		// The string does not contain any separators - error has occurred
		return false;
	}

	std::vector<std::string> listItems;

	int nCount = StringUtil::SplitString(szRawData, DATUM_SEP, listItems, true);
	int nVersionID = atoi(listItems[0].c_str());

	/* For Testing...
	bool bIsEncrypted = (1 == atoi(listItems[1].c_str()));
	*/

	switch (nVersionID) {
		case 0:
			if ( nCount < (int)MINIMUM_NUMBER_ITEMS_IN_VALID_USER ) {
				ClientLog(WHEREIAM, UTIL_COMP, ER, false,
				    _T("Incorrect number of elements in user profile serialized data %s"),
				    CS(szRawData));
				bSuccess = false;
				break;
			}
			// Here, the start index is 2
			bSuccess = Deserialize(listItems, 2, userData);
			break;
		default:
			ClientLog(WHEREIAM, UTIL_COMP, ER, false,
			    _T("Invalid version of user profile serialized data %d"), nVersionID);
			bSuccess = false;
	}

	return bSuccess;

} // End Deserialize

#endif

/*
* NAME:        GetUserProfileInt
* ACTION:      Get an integer configuration value
* PARAMETERS:  char* szName - the name of the configuration parameter
*              int nDefault - optional default value
*              const bool bServer - if true, value is retreived from
*              the server.  Otherwise, the data is retreived either from
*              the configuration file or registry (Windows).
*              UserData& userData  - container for the deserialized data.
* RETURNS:     int - the value or Default if not found
*/
int UserProfileData::GetUserProfileInt(const char* szName, int nDefault /* 0 */,
                                       const bool bServer /* false */)
{
    if (bServer) {
		// TODO: for server side values, get the latest value from
		// the server.
	}

	return GetInt(szName, nDefault);

} // End GetUserProfileInt

/*
 * NAME:        GetUserProfileLong
 * ACTION:      Get a long configuration value
 * PARAMETERS:  const char* szName - the name of the configuration parameter
 *              long lDefault - optional default value
 *              const bool bServer - if true, value is retreived from
 *				the server.  Otherwise, the data is retreived either from
 *              the configuration file or registry (Windows).
 * RETURNS:     long - the value or Default if not found
 */

long UserProfileData::GetUserProfileLong(const char* szName, long lDefault /*0*/,
                                         const bool bServer /*false*/)
{
    if (bServer) {
		// TODO: for server side values, get the latest value from
		// the server.
	}

	return GetLong(szName, lDefault);

} // End GetUserProfileLong

/*
* NAME:        SetUserProfileStr
* ACTION:      Set a configuration value
* PARAMETERS:  char* szName - the name of the configuration parameter
*              char* szValue - the value
*/

void UserProfileData::SetUserProfileStr(const char* szName,const char* szValue)
{
    Set(szName, szValue);

} // End SetUserProfileStr

/*
* NAME:        SetUserProfileInt
* ACTION:      Set an integer configuration value
* PARAMETERS:  char* Name - the name of the configuration parameter
*              int nValue - the value
*/
void UserProfileData::SetUserProfileInt(const char* szName, int nValue)
{
    SetInt(szName, nValue);

} // End SetUserProfileInt

/*
 * NAME:        SetUserProfileLong
 * ACTION:      Set a long configuration value
 * PARAMETERS:  const char* szName - the name of the configuration parameter
 *              long lValue - the value
 */

void UserProfileData::SetUserProfileLong(const char* szName, long lValue)
{
    SetLong(szName, lValue);

} // End SetUserProfileLong

//=================================================================
// User Database
//=================================================================

/*
 * NAME:        GetUserDBProfile
 * ACTION:      Retreives the DB specific profile fields.
 *
 * PARAMETERS:  const char* szUserName - user name required for DB access
 *              const char* szPassword - user's password
 */
ErrorType UserProfileData::GetUserDBProfile(const char* szUserName,
    const char* szPassword)
{
    m_etLastError = ERR_NOT_SUPPORTED;
	return m_etLastError;

} // End GetUserDBProfile


//=================================================================
// Windows's registry
//=================================================================

/*
* NAME:        GetRegistryProfileData
* ACTION:      Reads the user profile data from the registry
* PARAMETERS:  none.
*/
bool UserProfileData::GetRegistryProfileData()
{
#ifndef USE_REGISTRY
    return false;
#else
    unsigned long dw;

	HKEY hAppKey = NULL;
	HKEY hSoftKey = NULL;
	HKEY hCompanyKey = NULL;
	HKEY hGroupKey = NULL;

	HKEY hStartKey = NULL;
	HKEY hSectionKey = NULL;

	string strSectionKey;

	// get HKEY_CURRENT_USER//Software
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software"), 0, KEY_WRITE|KEY_READ,
		&hSoftKey) != ERROR_SUCCESS)
		return false;

	// get HKEY_CURRENT_USER//Software//Diomede
	if (RegCreateKeyEx(hSoftKey, _T("Diomede"), 0,
		REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hCompanyKey, &dw) != ERROR_SUCCESS)
	{
		RegCloseKey(hSoftKey);
		return false;
	}

	RegCloseKey(hSoftKey);

    // Get the application name
    char  szApplicationName[_MAX_PATH];

    char szDrive[_MAX_DRIVE];
    char szDir[_MAX_DIR];
    char szFileName[_MAX_FNAME];
    char szExt[_MAX_EXT];

    // If an application name is specified, use that.  Otherwise,
    // find the application name using the module.
    if ( strlen(ApplicationName) > 0) {
        lstrcpy(szFileName, ApplicationName);
    }
    else {
        HMODULE hMod = GetModuleHandle (NULL);
        GetModuleFileName (hMod, szApplicationName, sizeof(szApplicationName));

	    _splitpath(szApplicationName, szDrive, szDir, szFileName, szExt);

	    if ( strlen(szFileName) == 0) {
	        return false;
	    }
    }


	// get HKEY_CURRENT_USER//Software//Diomede
	if (RegCreateKeyEx(hCompanyKey, szFileName, 0, REG_NONE,
		REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hAppKey, &dw) != ERROR_SUCCESS)
	{
		RegCloseKey(hCompanyKey);
		return false;
	}

	hStartKey = hCompanyKey;
	hSectionKey = hAppKey;

	// For example, strSectionKey = "Diomede";
	lstrcpy((char*)strSectionKey.c_str(), szFileName);

	// If a subgroup has been specified, get the key for that level.
	char szTmpProfileGroup[100];
	strcpy(szTmpProfileGroup, m_szProfileGroup.c_str());
    int nApplicationGroup = _stricmp(szFileName, szTmpProfileGroup);

	if ( ( m_szProfileGroup.length() > 0) && (nApplicationGroup != 0) ) {
		if (RegCreateKeyEx(hAppKey, szTmpProfileGroup, 0, REG_NONE,
			REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
			&hGroupKey, &dw) != ERROR_SUCCESS)
		{
			RegCloseKey(hAppKey);
			return false;
		}

		hStartKey = hAppKey;
		hSectionKey = hGroupKey;

		lstrcpy((char*)strSectionKey.c_str(), (char*) m_szProfileGroup.c_str());
	}

    unsigned long   dwRtn = ERROR_SUCCESS;

	char szSectionKey[MAX_PATH];
	lstrcpy(szSectionKey, (char*)strSectionKey.c_str());

    if ( (dwRtn = ::RegOpenKeyEx(hStartKey, szSectionKey,
        0, KEY_READ, &hSectionKey )) != ERROR_SUCCESS) {
	    return false;
	}

	// Get all the values for this key.
	GetSubKeyValues(hSectionKey);

	if (hCompanyKey) {
		if (RegCloseKey(hCompanyKey) == ERROR_SUCCESS) {
			hCompanyKey = NULL;
		}
	}

	if (hAppKey) {
		if (RegCloseKey(hAppKey) == ERROR_SUCCESS) {
			hAppKey = NULL;
		}
	}

	if (hGroupKey) {
		if (RegCloseKey(hGroupKey) == ERROR_SUCCESS) {
			hGroupKey = NULL;
		}
	}

	if (hSectionKey) {
		if (RegCloseKey(hSectionKey) == ERROR_SUCCESS) {
			hSectionKey = NULL;
		}
	}

    // Do not save return code because error
    // has already occurred

   return true;
#endif

} // End GetRegistryProfileData

/*
* NAME:        SetRegistryProfileData
* ACTION:      Writes the user profile data to the registry
* PARAMETERS:  none.
*/
bool UserProfileData::SetRegistryProfileData()
{
#ifndef USE_REGISTRY
    return false;
#else
	unsigned long dw;

	HKEY hAppKey = NULL;
	HKEY hSoftKey = NULL;
	HKEY hCompanyKey = NULL;

	HKEY hGroupKey = NULL;
	HKEY hStartKey = NULL;

	// get HKEY_CURRENT_USER//Software
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software"), 0, KEY_WRITE|KEY_READ,
		&hSoftKey) != ERROR_SUCCESS)
		return false;

	// get HKEY_CURRENT_USER//Software//Diomede
	if (RegCreateKeyEx(hSoftKey, _T("Diomede"), 0,
		REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hCompanyKey, &dw) != ERROR_SUCCESS)
	{
		RegCloseKey(hSoftKey);
		return false;
	}

	RegCloseKey(hSoftKey);

    // Get the application name
    char  szApplicationName[_MAX_PATH];

    char szDrive[_MAX_DRIVE];
    char szDir[_MAX_DIR];
    char szFileName[_MAX_FNAME];
    char szExt[_MAX_EXT];

    // If an application name is specified, use that.  Otherwise,
    // find the application name using the module.
    if ( strlen(ApplicationName) > 0) {
        lstrcpy(szFileName, ApplicationName);
    }
    else {
        HMODULE hMod = GetModuleHandle (NULL);
        GetModuleFileName (hMod, szApplicationName, sizeof(szApplicationName));

	    _splitpath(szApplicationName, szDrive, szDir, szFileName, szExt);

	    if ( strlen(szFileName) == 0) {
	        return false;
	    }
    }


	// get HKEY_CURRENT_USER//Software//Diomede
	if (RegCreateKeyEx(hCompanyKey, szFileName, 0, REG_NONE,
		REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hAppKey, &dw) != ERROR_SUCCESS)
	{
		RegCloseKey(hCompanyKey);
		return false;
	}

	hStartKey = hAppKey;

	// If a subgroup has been specified, get the key for that level.
	char szTmpProfileGroup[100];
	lstrcpy(szTmpProfileGroup,  m_szProfileGroup.c_str());
    int nApplicationGroup = _stricmp(szFileName, szTmpProfileGroup);

	if ( ( m_szProfileGroup.length() > 0) && (nApplicationGroup != 0) ) {
		if (RegCreateKeyEx(hAppKey, szTmpProfileGroup, 0, REG_NONE,
			REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
			&hGroupKey, &dw) != ERROR_SUCCESS)
		{
			RegCloseKey(hAppKey);
			return false;
		}

		hStartKey = hGroupKey;
	}

    unsigned long   dwRtn;
	long    datasize = MAX_KEY_LENGTH;

    string Name;
    string szKey;
    char szTmpKey[MAX_PATH];

    Dictionary<string>::iterator ThisName;
    for (ThisName = m_ValSet.begin(); ThisName != m_ValSet.end(); ThisName++)
    {
        Name = *ThisName;
        lstrcpy(szTmpKey, ThisName.key().c_str());

		dwRtn = RegSetValueEx(hStartKey,                        // subkey handle
						      szTmpKey,                         // value name
						      0,                                // must be zero
						      REG_SZ,                           // value type
						      (LPBYTE)Name.c_str(),            // pointer to value data
						      (lstrlen(Name.c_str())+1)*sizeof(char));
									                            // length of value data


        if (dwRtn != ERROR_SUCCESS) {
            return false;
		}

    }

	if (hAppKey) RegCloseKey(hAppKey);
	if (hCompanyKey) RegCloseKey(hCompanyKey);
	if (hGroupKey) RegCloseKey(hGroupKey);
	return true;

#endif

} // End SetRegistryProfileData

#ifdef WIN32

/*
* NAME:        GetAppKey
* ACTION:      Retrieves the registry application specific key to
*              be used on subsequent access to the registry.
* PARAMETERS:  HKEY reference.
* RETURNS:     bool - true if access is successful.
*/
bool UserProfileData::GetAppKey(HKEY& hAppKey)
{

#ifndef USE_REGISTRY
    return false;
#else
	unsigned long dw;

	hAppKey = NULL;

	HKEY hSoftKey = NULL;
	HKEY hCompanyKey = NULL;

	// get HKEY_LOCAL_MACHINE//Software
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software"), 0, KEY_WRITE|KEY_READ,
		&hSoftKey) != ERROR_SUCCESS)
		return false;

	// get HKEY_LOCAL_MACHINE//Software//Diomede
	if (RegCreateKeyEx(hSoftKey, _T("Diomede"), 0,
		REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hCompanyKey, &dw) != ERROR_SUCCESS)
	{
		RegCloseKey(hSoftKey);
		return false;
	}

	RegCloseKey(hSoftKey);

    char szDrive[_MAX_DRIVE];
    char szDir[_MAX_DIR];
    char szFileName[_MAX_FNAME];
    char szExt[_MAX_EXT];

    // If an application name is specified, use that.  Otherwise,
    // find the application name using the module.
    if ( strlen(ApplicationName) > 0) {
        lstrcpy(szFileName, ApplicationName);
    }
    else {

        // Get the application name
        char  szApplicationName[_MAX_PATH];

        HMODULE hMod = GetModuleHandle (NULL);
        GetModuleFileName (hMod, szApplicationName, sizeof(szApplicationName));

	    _splitpath(szApplicationName, szDrive, szDir, szFileName, szExt);

	    if ( strlen(szFileName) == 0) {
	        return false;
	    }
    }

	// get HKEY_LOCAL_MACHINE//Software//Diomede
	if (RegCreateKeyEx(hCompanyKey, szFileName, 0, REG_NONE,
		REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hAppKey, &dw) != ERROR_SUCCESS)
	{
		RegCloseKey(hCompanyKey);
		return false;
	}

	if (hCompanyKey) RegCloseKey(hCompanyKey);
	return true;

#endif

} // End GetAppKey

/*
* NAME:        GetUserAppKey
* ACTION:      Retrieves the registry user specific applicatin key
*              to be used on subsequent access to the registry.
* PARAMETERS:  HKEY reference.
* RETURNS:     bool - true if access is successful.
*/

bool UserProfileData::GetUserAppKey(HKEY& hAppKey)
{
#ifndef USE_REGISTRY
    return false;
#else
	unsigned long dw;

	hAppKey = NULL;

	HKEY hSoftKey = NULL;
	HKEY hCompanyKey = NULL;

	// get HKEY_CURRENT_USER//Software
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software"), 0, KEY_WRITE|KEY_READ,
		&hSoftKey) != ERROR_SUCCESS)
		return false;

	// get HKEY_CURRENT_USER//Software//Diomede
	if (RegCreateKeyEx(hSoftKey, _T("Diomede"), 0,
		REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hCompanyKey, &dw) != ERROR_SUCCESS)
	{
		RegCloseKey(hSoftKey);
		return false;
	}

	RegCloseKey(hSoftKey);

    char szDrive[_MAX_DRIVE];
    char szDir[_MAX_DIR];
    char szFileName[_MAX_FNAME];
    char szExt[_MAX_EXT];

    // If an application name is specified, use that.  Otherwise,
    // find the application name using the module.
    if ( strlen(ApplicationName) > 0) {
        lstrcpy(szFileName, ApplicationName);
    }
    else {

        // Get the application name
        char szApplicationName[_MAX_PATH];

        HMODULE hMod = GetModuleHandle (NULL);
        GetModuleFileName (hMod, szApplicationName, sizeof(szApplicationName));

	    _splitpath(szApplicationName, szDrive, szDir, szFileName, szExt);

	    if ( strlen(szFileName) == 0) {
	        return false;
	    }
    }

	// get HKEY_CURRENT_USER//Software//Diomede
	if (RegCreateKeyEx(hCompanyKey, szFileName, 0, REG_NONE,
		REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hAppKey, &dw) != ERROR_SUCCESS)
	{
		RegCloseKey(hCompanyKey);
		return false;
	}

	if (hCompanyKey) RegCloseKey(hCompanyKey);
	return true;
#endif

} // End GetUserAppKey

/*
* NAME:        GetSubKeyValues
* ACTION:      Retreives the values under a key
* PARAMETERS:  HKEY
* RETURNS:     bool - true if access is successful.
*/
bool UserProfileData::GetSubKeyValues(HKEY hKey)
{
#ifndef USE_REGISTRY
    return false;
#else
    if (!hKey) {
	    return false;
	}

	unsigned long   dwBytes = 256;
	unsigned long   dwType = REG_SZ;

	unsigned long   dwcValues;			// Number of values for this key.
	unsigned long   dwcMaxValueName;	// Longest Value name.
	unsigned long   dwcMaxValueData;	// Longest Value data.

    char   szSubKey[MAX_KEY_LENGTH];	// (256) this should be dynamic.
	unsigned long   dwSubKeyLength;

	// Get Value count.
	RegQueryInfoKey(hKey,           // Key handle.
				   NULL,			// Buffer for class name.
				   NULL,			// Length of class string.
				   NULL,			// Reserved.
				   NULL,			// Number of sub keys.
				   NULL,			// Longest sub key size.
				   NULL,			// Longest class string.
				   &dwcValues,      // Number of values for this key.
				   &dwcMaxValueName,// Longest Value name.
				   &dwcMaxValueData,// Longest Value data.
				   NULL,			// Security descriptor.
				   NULL);			// Last write time.

	// Loop until RegEnumKey fails
	if (dwcValues)
	{
		for ( int nCount = 0, dwSubKeyRtn = ERROR_SUCCESS; nCount < (int)dwcValues;
		    nCount++) {

			dwBytes = 256;
			char szValue[256];
			szValue[0] = _T('\0');

	        dwSubKeyLength = sizeof(szSubKey) / sizeof(szSubKey[0]);

			dwSubKeyRtn = ::RegEnumValue(hKey, nCount, szSubKey, &dwSubKeyLength,
								       NULL, &dwType, (LPBYTE)szValue,
									   &dwBytes);

			if (dwSubKeyRtn != (unsigned long)ERROR_SUCCESS &&
				dwSubKeyRtn != ERROR_INSUFFICIENT_BUFFER) {
				continue;
			}

			SetUserProfileStr(szSubKey, szValue);
		} // end for loop
	} // End if

	return true;
#endif

} // End GetSubKeyValues

/*
 * NAME:        GetRegistryStr
 * ACTION:      Get a configuration value directly from the registry.
 * PARAMETERS:  HKEY hAppKey: HKEY_LOCAL_MACHINE or HKEY_LOCAL_USER
 *              application key.
 *              const char* szName - the name of the registry key
 *              char* szValue - holds the value
 *              const char* szDefault - optional default value
 * RETURNS:     bool where true = success
 */
bool UserProfileData::GetRegistryStr(HKEY hAppKey, const char* szName, char* szValue,
	const char* szDefault /*NULL*/)
{
#ifndef USE_REGISTRY
    return false;
#else
	string strDefault = _T("");

	if ( szDefault ) {
	    strDefault = szDefault;
	}

    // Caller is responsible for ensuring szValue is big enough to hold
	// the results.
	// char	szValue[256];

	unsigned long   dwSubKeyRtn;
	unsigned long   dwBytes = 256;
	unsigned long   dwType = REG_SZ;

	bool bReturn = true;

	if ( dwSubKeyRtn = RegQueryValueEx(hAppKey, szName, 0, &dwType,
		(unsigned char*)szValue, &dwBytes) != ERROR_SUCCESS) {

		lstrcpy(szValue, (char*)strDefault.c_str());
		bReturn = false;
	}

	return bReturn;
#endif

} // End GetRegistryStr

/*
 * NAME:        GetRegistryInt
 * ACTION:      Get an integer configuration value directly from the registry.
 * PARAMETERS:  HKEY hAppKey: HKEY_LOCAL_MACHINE or HKEY_LOCAL_USER
 *              application key.
 *              char* szName - the name of the configuration parameter
 *              int& nValue - value found
 *              int nDefault - optional default value
 * RETURNS:     bool where true = success
 */

bool UserProfileData::GetRegistryInt(HKEY hAppKey, const char* szName, int& nValue,
    int nDefault /*0*/)
{
#ifndef USE_REGISTRY
    return false;
#else
	char	szValue[256];
	unsigned long   dwSubKeyRtn;
	unsigned long   dwBytes = 256;
	unsigned long   dwType = REG_SZ;

	bool bReturn = false;

	if ( dwSubKeyRtn = RegQueryValueEx(hAppKey, szName, 0, &dwType,
		(unsigned char*)szValue, &dwBytes) == ERROR_SUCCESS) {

		// The key is set into the list - this will enable
		// cleanup when the list is destroyed.
        nValue = atoi(szValue);
		bReturn = true;
	}
	else {
		nValue = nDefault;
	}

	return bReturn;
#endif

} // End GetRegistryInt

/*
 * NAME:        SetRegistryStr
 * ACTION:      Set a configuration value directly to the registry.
 * PARAMETERS:  HKEY hAppKey: HKEY_LOCAL_MACHINE or HKEY_LOCAL_USER
 *              application key.
 *              char* szName - the name of the configuration parameter
 *              char* szValue - the value
 * RETURNS:     true if successful
 */

bool UserProfileData::SetRegistryStr(HKEY hAppKey, const char* szName, const char* szValue)
{
#ifndef USE_REGISTRY
    return false;
#else
    unsigned long   dwRtn;
	long    datasize = MAX_KEY_LENGTH;

	dwRtn = RegSetValueEx(hAppKey,                  // subkey handle
						  szName,                   // value name
						  0,                        // must be zero
						  REG_SZ,                   // value type
						  (LPBYTE)szValue,          // pointer to value data
						  (strlen(szValue)+1)*sizeof(char));
									                // length of value data

    return (dwRtn == ERROR_SUCCESS);
#endif

} // End SetRegistryStr

/*
 * NAME:        SetRegistryInt
 * ACTION:      Set an integer configuration value into the registry.
 * PARAMETERS:  HKEY hAppKey: HKEY_LOCAL_MACHINE or HKEY_LOCAL_USER
 *              application key.
 *              char* szName - the name of the configuration parameter
 *              int nValue - the value
 * RETURNS:     true if successful
 */

bool UserProfileData::SetRegistryInt(HKEY hAppKey, const char* szName, int nValue)
{
#ifndef USE_REGISTRY
    return false;
#else
	char	szValue[256];
    unsigned long   dwRtn;
	long    datasize = MAX_KEY_LENGTH;

    sprintf(szValue, _T("%d"), nValue);
	dwRtn = RegSetValueEx(hAppKey,                  // subkey handle
						  szName,                   // value name
						  0,                        // must be zero
						  REG_SZ,                   // value type
						  (LPBYTE)szValue,                  // pointer to value data
						  (strlen(szValue)+1)*sizeof(char));
									                // length of value data

    return (dwRtn == ERROR_SUCCESS);
#endif

} // End SetRegistryInt

//=================================================================
// HKEY_CURRENT_USER
//=================================================================

/*
 * NAME:        GetUserProfileRegistryStr
 * ACTION:      Get a configuration value directly from the registry.
 * PARAMETERS:  const char* szName - the name of the registry key
 *              char* szValue - holds the value
 *              const char* szDefault - optional default value
 * RETURNS:     bool where true = success
 */

bool UserProfileData::GetUserProfileRegistryStr(const char* szName,
    char* szValue, const char* szDefault /*NULL*/)
{
#ifndef USE_REGISTRY
    return false;
#else
    HKEY hAppKey;

	if ( !GetUserAppKey(hAppKey) ) {
        return false;
	}

    bool bReturn = GetRegistryStr(hAppKey, szName, szValue, szDefault);

	// If the value is not found, return the default.
	if (hAppKey) RegCloseKey(hAppKey);

	return bReturn;
#endif

} // End GetUserProfileRegistryStr

/*
 * NAME:        GetUserProfileRegistryInt
 * ACTION:      Get an integer configuration value directly from the registry.
 * PARAMETERS:  char* szName - the name of the configuration parameter
 *              int nDefault - optional default value
 * RETURNS:     bool where true = success
 */

bool UserProfileData::GetUserProfileRegistryInt(const char* szName, int& nValue,
    int nDefault /*0*/)
{
#ifndef USE_REGISTRY
    return false;
#else
    HKEY hAppKey;

	if ( !GetUserAppKey(hAppKey) ) {
	    return false;
	}

    bool bReturn = GetRegistryInt(hAppKey, szName, nValue, nDefault);

	// If the value is not found, return the default.
	if (hAppKey) RegCloseKey(hAppKey);

	return bReturn;
#endif

} // End GetUserProfileRegistryInt

/*
 * NAME:        SetUserProfileRegistryStr
 * ACTION:      Set a configuration value directly to the registry.
 * PARAMETERS:  char* szName - the name of the configuration parameter
 *              char* szValue - the value
 */

bool UserProfileData::SetUserProfileRegistryStr(const char* szName,
    const char* szValue)
{
#ifndef USE_REGISTRY
    return false;
#else
    HKEY hAppKey;

	if ( !GetUserAppKey(hAppKey) ) {
	    return false;
	}

    bool bReturn = SetRegistryStr(hAppKey, szName, szValue);

	if (hAppKey) RegCloseKey(hAppKey);

    return bReturn;
#endif

} // End SetUserProfileRegistryStr

/*
 * NAME:        SetUserProfileRegistryInt
 * ACTION:      Set an integer configuration value into the registry.
 * PARAMETERS:  char* szName - the name of the configuration parameter
 *              int nValue - the value
 */

bool UserProfileData::SetUserProfileRegistryInt(const char* szName, int nValue)
{
#ifndef USE_REGISTRY
    return false;
#else
    HKEY hAppKey;

	if ( !GetUserAppKey(hAppKey) ) {
	    return false;
	}

    bool bReturn = SetRegistryInt(hAppKey, szName, nValue);

	if (hAppKey) RegCloseKey(hAppKey);

    return bReturn;
#endif

} // End SetUserProfileRegistryInt

/*
 * NAME:        GetCompanyProfileRegistryStr
 * ACTION:      Get a configuration value directly from the registry
 *              stored at the company level.  Only get
 *              provided at this time.
 * PARAMETERS:  APIs called to retreive the application key.
 *              const char* szName - the name of the registry key
 *              char* szValue - holds the value
 *              const char* szDefault - optional default value
 * RETURNS:     bool where true = success
 */

bool UserProfileData::GetCompanyProfileRegistryStr(const char* szName, char* szValue,
	    const char* szDefault /*NULL*/)
{
#ifndef USE_REGISTRY
    return false;
#else
	unsigned long dw;

	HKEY hSoftKey = NULL;
	HKEY hCompanyKey = NULL;

	// get HKEY_CURRENT_USER//Software
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software"), 0, KEY_WRITE|KEY_READ,
		&hSoftKey) != ERROR_SUCCESS)
		return false;

	// get HKEY_CURRENT_USER//Software//Diomede
	if (RegCreateKeyEx(hSoftKey, _T("Diomede"), 0,
		REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hCompanyKey, &dw) != ERROR_SUCCESS)
	{
		RegCloseKey(hSoftKey);
		return false;
	}

	RegCloseKey(hSoftKey);

    bool bReturn = GetRegistryStr(hCompanyKey, szName, szValue, szDefault);

	// If the value is not found, return the default.
	if (hCompanyKey) RegCloseKey(hCompanyKey);

	return bReturn;
#endif

} // End GetCompanyProfileRegistryStr

/*
 * NAME:        GetCompanyProfileRegistryInt
 * ACTION:      Get a configuration value directly from the registry
 *              stored at the company level.  Only get is
 *              provided at this time.
 * PARAMETERS:  char* szName - the name of the configuration parameter
 *              int& nValue - value found
 *              int nDefault - optional default value
 * RETURNS:     bool where true = success
 */

bool UserProfileData::GetCompanyProfileRegistryInt(const char* szName, int& nValue,
    int nDefault /*0*/)
{
#ifndef USE_REGISTRY
    return false;
#else
	unsigned long dw;

	HKEY hSoftKey = NULL;
	HKEY hCompanyKey = NULL;

	// get HKEY_CURRENT_USER//Software
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software"), 0, KEY_WRITE|KEY_READ,
		&hSoftKey) != ERROR_SUCCESS)
		return false;

	// get HKEY_CURRENT_USER//Software//Diomede
	if (RegCreateKeyEx(hSoftKey, _T("Diomede"), 0,
		REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hCompanyKey, &dw) != ERROR_SUCCESS)
	{
		RegCloseKey(hSoftKey);
		return false;
	}

	RegCloseKey(hSoftKey);

    bool bReturn = GetRegistryInt(hCompanyKey, szName, nValue, nDefault);

	// If the value is not found, return the default.
	if (hCompanyKey) RegCloseKey(hCompanyKey);

	return bReturn;
#endif

} // End GetCompanyProfileRegistryInt

//=================================================================
// HKEY_LOCAL_MACHINE
//=================================================================

/*
 * NAME:        GetAppProfileRegistryStr
 * ACTION:      Get a configuration value directly from the registry.
 * PARAMETERS:  APIs called to retreive the application key.
 *              const char* szName - the name of the registry key
 *              char* szValue - holds the value
 *              const char* szDefault - optional default value
 * RETURNS:     bool where true = success
 */

bool UserProfileData::GetAppProfileRegistryStr(const char* szName, char* szValue,
	const char* szDefault /*NULL*/)
{
#ifndef USE_REGISTRY
    return false;
#else
    HKEY hAppKey;

	if ( !GetAppKey(hAppKey) ) {
        return false;
	}

    bool bReturn = GetRegistryStr(hAppKey, szName, szValue, szDefault);

	// If the value is not found, return the default.
	if (hAppKey) RegCloseKey(hAppKey);

	return bReturn;
#endif

} // End GetAppProfileRegistryStr

/*
 * NAME:        GetAppProfileRegistryInt
 * ACTION:      Get an integer configuration value directly from the registry.
 * PARAMETERS:  char* szName - the name of the configuration parameter
 *              int& nValue - value found
 *              int nDefault - optional default value
 * RETURNS:     bool where true = success
 */

bool UserProfileData::GetAppProfileRegistryInt(const char* szName, int& nValue,
    int nDefault /*0*/)
{
#ifndef USE_REGISTRY
    return false;
#else
    HKEY hAppKey;

	if ( !GetAppKey(hAppKey) ) {
	    return false;
	}

    bool bReturn = GetRegistryInt(hAppKey, szName, nValue, nDefault);

	// If the value is not found, return the default.
	if (hAppKey) RegCloseKey(hAppKey);

	return bReturn;
#endif

} // End GetAppProfileRegistryInt

/*
 * NAME:        SetAppProfileRegistryStr
 * ACTION:      Set a configuration value directly to the registry.
 * PARAMETERS:  char* szName - the name of the configuration parameter
 *              char* szValue - the value
 * RETURNS:     true if successful
 */

bool  UserProfileData::SetAppProfileRegistryStr(const char* szName,
    const char* szValue)
{
#ifndef USE_REGISTRY
    return false;
#else
    HKEY hAppKey;


	if ( !GetAppKey(hAppKey) ) {
	    return false;
	}

    bool bReturn = SetRegistryStr(hAppKey, szName, szValue);

	if (hAppKey) RegCloseKey(hAppKey);

    return bReturn;
#endif
} // End SetAppProfileRegistryStr

/*
 * NAME:        SetAppProfileRegistryInt
 * ACTION:      Set an integer configuration value into the registry.
 * PARAMETERS:  char* szName - the name of the configuration parameter
 *              int nValue - the value
 * RETURNS:     true if successful
 */

bool UserProfileData::SetAppProfileRegistryInt(const char* szName, int nValue)
{
#ifndef USE_REGISTRY
    return false;
#else
    HKEY hAppKey;

	if ( !GetAppKey(hAppKey) ) {
	    return false;
	}

    bool bReturn = SetRegistryInt(hAppKey, szName, nValue);

	if (hAppKey) RegCloseKey(hAppKey);

    return bReturn;
#endif

} // End SetAppProfileRegistryInt

#endif


///////////////////////////////////////////////////////////////////////////////


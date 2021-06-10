/*********************************************************************
 * 
 *  file:  UserProfileData.h
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

#ifndef __USER_PROFILE_DATA_H__
#define __USER_PROFILE_DATA_H__

/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Stdafx.h"
#include "configure.h"
#include "ErrorType.h"

#include <string>
using namespace std;

// #define NO_ENCRYPTED_USER_PROFILE_KEYS

/////////////////////////////////////////////////////////////////////////////
// HKEY_LOCAL_MACHINE keys
/////////////////////////////////////////////////////////////////////////////

// The path of the application set by
// a) The installation wizard
// b) The app itself during startup
// External applications use this key to find the location of diomede.exe.
#define GEN_APP_PATH							_T("ApplicationPath")

// The path of the startup shortcut.
#define GEN_STARTUP_LINK						_T("StartupLink")

/////////////////////////////////////////////////////////////////////////////
// HKEY_CURRENT_USER keys
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// The path of the application user data - set at initial launch.
// This cannot be set by the installer since it must be a writable folder
// (for users which do not have admin rights).
#define GEN_USER_APP_PATH						_T("UserApplicationPath")

// Login preferences
#define UI_LASTLOGIN							_T("LastLogin")
#define	UI_USERNAME								_T("UserName")
#define	UI_PASSWORD								_T("UserPassword")

// The new encrypted login
#define	UI_CLIENT_ENCRYPTED					    _T("EncryptedClient")
#define UI_CLIENT_LENGTH                        _T("OrigClientLength")

#define	UI_REMEMBER_PWD							_T("RememberPwd")
#define UI_REMEMBER_PWD_DF                      0

// Encryption keys
#define UI_ENCRYPT_LOW                          _T("EncryptLow")
#define UI_ENCRYPT_HIGH                         _T("EncryptHigh")

// Exception handling
#define CRASH_FILE  							_T("CrashFile")
#define CRASH_FILE_DF						    _T("")

#define CRASH_INFO  							_T("CrashInfo")
#define CRASH_INFO_DF						    _T("")

//--------------------------
// SetConf commands
//--------------------------
#define GEN_AUTO_LOGIN							_T("AutoLogin")
#define GEN_AUTO_LOGIN_DF						0

#define GEN_AUTO_LOGOUT							_T("AutoLogout")
#define GEN_AUTO_LOGOUT_DF						0

#define GEN_AUTO_CHECK_ACCOUNT					_T("AutoCheckAccount")
#define GEN_AUTO_CHECK_ACCOUNT_DF	    		1

#define GEN_SESSION_TOKEN   					_T("SessionToken")
#define GEN_SESSION_TOKEN_DF					_T("")

#define GEN_SESSION_TOKEN_EXPIRES				_T("SessionTokenExpires")
#define GEN_SESSION_TOKEN_EXPIRES_DF			0

#define GEN_SERVICE_SECURE_TYPE   				_T("ServiceSecureType")
#define GEN_SERVICE_SECURE_TYPE_DF				0

#define GEN_SERVICE_USE_DEFAULT_ENDPOINT        _T("UseDefaultEndpoint")
#define GEN_SERVICE_USE_DEFAULT_ENDPOINT_DF     1

#define GEN_SERVICE_ENDPOINT					_T("ServiceEnpoint")
#define GEN_SERVICE_ENDPOINT_DF                 _T("http://service.diomedestorage.com/1.1/DiomedeStorageService.svc")

#define GEN_SECURE_SERVICE_ENDPOINT				_T("SecureServiceEnpoint")
#define GEN_SECURE_SERVICE_ENDPOINT_DF          _T("https://service.diomedestorage.com/1.1/DiomedeStorageService.svc")

#define GEN_TRANSFER_ENDPOINT					_T("TransferEndpoint")
#define GEN_TRANSFER_ENDPOINT_DF                _T("http://transfer.diomedestorage.com/1.1/DiomedeStorageTransfer.svc")

#define GEN_SECURE_TRANSFER_ENDPOINT	    	_T("SecureTransferEndpoint")
#define GEN_SECURE_TRANSFER_ENDPOINT_DF         _T("https://transfer.diomedestorage.com/1.1/DiomedeStorageTransfer.svc")

#define GEN_PROXY_HOST                          _T("ProxyHost")
#define GEN_PROXY_HOST_DF                       _T("")

#define GEN_PROXY_PORT                          _T("ProxyPort")
#define GEN_PROXY_PORT_DF                       8080

#define GEN_PROXY_USERID                        _T("ProxyUserID")
#define GEN_PROXY_USERID_DF                     _T("")

#define GEN_PROXY_PASSWORD                      _T("ProxyPassword")
#define GEN_PROXY_PASSWORD_DF                   _T("")

#define GEN_RESULT_PAGE_SIZE					_T("ResultPageSize")
#define GEN_RESULT_PAGE_SIZE_DF		    		_T("1000")

#define GEN_RESULT_PAGE     					_T("ResultPage")
#define GEN_RESULT_PAGE_DF		    		    _T("")

#define GEN_COPY_TO_CLIPBOARD                   _T("CopyToClipboard")
#define GEN_COPY_TO_CLIPBOARD_DF		    	1

#define GEN_VERBOSE_OUTPUT     					_T("VerboseOutput")
#define GEN_VERBOSE_OUTPUT_DF		    		0

// Turn on or off console timing
#define GEN_SHOW_TIMING     					_T("ShowTiming")
#define GEN_SHOW_TIMING_DF	    	    		1

// Output logging to log files.
#define GEN_ENABLE_LOGGING					    _T("EnableLogging")
#define GEN_ENABLE_LOGGING_DF	    	        0

// Log timing to the log files.
#define GEN_LOG_TIMING     					    _T("LogTiming")
#define GEN_LOG_TIMING_DF	    	    		0

// Log timing to the log files.
#define GEN_THREAD_SLEEP     					_T("SystemSleep")
#define GEN_THREAD_SLEEP_DF	    	    		100

// Override the built in chunk sizes.
#define GEN_MIN_CHUNK_SIZE     					_T("MinChunkSize")
#define GEN_MIN_CHUNK_SIZE_DF	    	        4096

#define GEN_MAX_CHUNK_SIZE     					_T("MaxChunkSize")
#define GEN_MAX_CHUNK_SIZE_DF	    	        524288

// Log upload status to the log files.
#define GEN_LOG_UPLOAD     					    _T("LogUpload")
#define GEN_LOG_UPLOAD_DF	    	    		0

// Customization of resume intervals.  Time is in seconds.
#define GEN_RESUME_INTERVAL_1 				    _T("ResumeInterval1")
#define GEN_RESUME_INTERVAL_1_DF   	    		5

#define GEN_RESUME_INTERVAL_2 				    _T("ResumeInterval2")
#define GEN_RESUME_INTERVAL_2_DF   	    		30

#define GEN_RESUME_INTERVAL_3 				    _T("ResumeInterval3")
#define GEN_RESUME_INTERVAL_3_DF   	    		300

#define GEN_RESUME_INTERVAL_4 				    _T("ResumeInterval4")
#define GEN_RESUME_INTERVAL_4_DF   	    		600

#define GEN_RESUME_INTERVAL_5 				    _T("ResumeInterval5")
#define GEN_RESUME_INTERVAL_5_DF   	    		3600

#define GEN_SEND_TIMEOUT                        _T("SendTimeout")
#define GEN_SEND_TIMEOUT_DF                     30

#define GEN_RECV_TIMEOUT                        _T("ReceiveTimeout")
#define GEN_RECV_TIMEOUT_DF                     30

#define GEN_CONNECT_TIMEOUT                     _T("ConnectTimeout")
#define GEN_CONNECT_TIMEOUT_DF                  30

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class UserData;

/////////////////////////////////////////////////////////////////////////////

class UserProfileData : public Configure
{
protected:
    std::string         m_szUserName;                   // Login user name
    std::string         m_szUserPassword;               // Login user password
    std::string         m_szFilename;                   // Configuration filename

    std::string         m_szProfileGroup;               // Optional group name for data.
    bool                m_bCreateConfigFile;            // Optional configuration file creation.

	unsigned long	    m_dwEncryptLow;		            // Data for encryption
	unsigned long	    m_dwEncryptHigh;

    bool                m_bIsFirstRun;

	bool                m_bReadDBProfileOK;
	ErrorType           m_etLastError;                  // User profile error
	int                 m_nLastError;                   // System error

protected:
    /*
     * NAME:        Init
     * ACTION:      Initialize the configuration file.  Handled
     *              by the derived class.
     * PARAMETERS:  char* FileName - the name of the file to initialize.
     */

    void Init(char* szFileName);


public:
    // Used by the configuration wizard to correctly specify the
    // application name.
    static char* ApplicationName;

    /*
     * NAME:        UserProfileData
     * ACTION:      Create a new configuration handler
	 *
     */
	UserProfileData() : m_szUserName(_T("")), m_szUserPassword(_T("")), m_szFilename(_T("")),
	                    m_szProfileGroup(_T("")), m_bCreateConfigFile(true),
	                    m_dwEncryptLow(0), m_dwEncryptHigh(0), m_bReadDBProfileOK(true),
	                    m_etLastError(0), m_nLastError(0) {};
    ~UserProfileData();

    /*
     * NAME:        GetUserProfile
     * ACTION:      Loads the configuration from either the named file
	 *              or window's registry and user DB.
	 *
 	 * PARAMETERS:  const char* szUserName - user name required for DB access
	 *              const char* szPassword - user's password
     *              const char* szFileName - the name of the configuration file
     *              const char* szProfileGroup - optional group for the data.
     *              used only in Windows.
     *              bool bCreateConfigFile - true to create the configuration file,
     *              false otherwise.
     */
    ErrorType GetUserProfile(const char* szUserName = _T(""), const char* szPassword = _T(""),
	    const char* szFileName = _T(""), const char* szProfileGroup = _T(""),
	    bool bCreateConfigFile = true );

    /*
     * NAME:        SaveUserProfile
     * ACTION:      Saves the profile data.  Server values are saved to
	 *              the user data.  The remaining are either saved to
	 *              the configuration file or to the registrry.
	 * RETURNS:     Error indicating the result.
     */

    ErrorType SaveUserProfile();

    /*
     * NAME:        LogUserProfile
     * ACTION:      Logs the contents of the configuration
	 *
 	 * PARAMETERS:  none.
     */
	 void LogUserProfile();

public:

    /*
     * NAME:        GetLastError
     * ACTION:      Returns the last error code - used particularly
	 *              in the case of the constructor where no error
	 *              code can be returned.
     * RETURNS:     ErrorType
     */

	ErrorType GetLastErrorType() { return m_etLastError;  }

    /*
     * NAME:        GetLastSysError
     * ACTION:      Returns the last system error code
     * RETURNS:     int
     */

	int GetLastSysError() { return m_nLastError;  }

    /*
     * NAME:        GetUserProfileStr
     * ACTION:      Get a configuration value
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              const char* szDefault - optional default value
	 *              const bool bServer - if true, value is retreived from
	 *				the server.  Otherwise, the data is retreived either from
	 *              the configuration file or registry (Windows).
     * RETURNS:     string - the value or Default if not found
     */

    std::string GetUserProfileStr( const char* szName, const char* szDefault = NULL,
	    const bool bServer = false);

    std::string GetStr( const char* szName, const char* szDefault = NULL,
                   const bool bServer = false) {
        return GetUserProfileStr( szName, szDefault, bServer );
    }


    /*
     * NAME:        GetUserProfileInt
     * ACTION:      Get an integer configuration value
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              int nDefault - optional default value
	 *              const bool bServer - if true, value is retreived from
	 *				the server.  Otherwise, the data is retreived either from
	 *              the configuration file or registry (Windows).
     * RETURNS:     int - the value or Default if not found
     */

    int GetUserProfileInt(const char* szName, int nDefault = 0, const bool bServer = false);

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

    long GetUserProfileLong(const char* szName, long lDefault = 0, const bool bServer = false);

    /*
     * NAME:        SetUserProfileStr
     * ACTION:      Set a configuration value
     * PARAMETERS:  char* szName - the name of the configuration parameter
     *              char* szValue - the value
     */

    void SetUserProfileStr(const char* szName,const char* szValue);
    void SetStr(const char* szName,const char* szValue) {
        SetUserProfileStr(szName, szValue);
    }

    /*
     * NAME:        SetUserProfileInt
     * ACTION:      Set an integer configuration value
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              int nValue - the value
     */

    void SetUserProfileInt(const char* szName, int nValue);

    /*
     * NAME:        SetUserProfileLong
     * ACTION:      Set a long configuration value
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              long lValue - the value
     */

    void SetUserProfileLong(const char* szName, long lValue);

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
    ErrorType GetUserDBProfile(const char* szUserName, const char* szPassword);

	/*
	* NAME:        SaveUserDBProfile
	* ACTION:      Saves the DB specific profile data.
    * PARAMETERS:  const char* szUserName - user name required for DB access
    *              const char* szPassword - user's password
	* RETURNS:     Error indicating the result.
	*/

	ErrorType SaveUserDBProfile(const char* szUserName, const char* szPassword);


     /*
     * NAME:        ReadDBProfileOk
     * ACTION:      Indicates if the user DB could be read ok
     * RETURNS:     bool - true if DB was read ok
     */

    bool ReadDBProfileOk() { return m_bReadDBProfileOK; };

    //=================================================================
	// Windows's registry
    //=================================================================

	/*
	* NAME:        GetRegistryProfileData
	* ACTION:      Reads the user profile data from the registry
	* PARAMETERS:  none.
	*/

	bool GetRegistryProfileData();

	/*
	* NAME:        SetRegistryProfileData
	* ACTION:      Writes the user profile data to the registry
	* PARAMETERS:  none.
	*/

	bool SetRegistryProfileData();

#ifdef NO_ENCRYPTED_USER_PROFILE_KEYS

	/*
	 * NAME:        SetUserProfileStrEncrypted
	 * ACTION:      Sets the user profile string in an encrypted form.
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              const char* szValue - optional default value
     * RETURNS:     nothing
	 */
    void SetUserProfileStrEncrypted(const char *szName, const char *szValue ) {
		SetUserProfileStr(szName,szValue);
	}

	/*
	 * NAME:        SetUserProfileEncrypted
	 * ACTION:      Serializes and encrypts the user data
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              UserData& userData - contains the user data for serialization
     * RETURNS:     nothing
	 */
    void SetUserProfileEncrypted(const char *szName, const UserData& userData);

	/*
	 * NAME:        GetUserProfileStrEncrypted
	 * ACTION:      Returns the user profile string in an encrypted form.
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              const char* szDefault - optional default value
	 *              const bool bServer - if true, value is retrieved from
	 *				the server.  Otherwise, the data is retreived either from
	 *              the configuration file or registry (Windows).
     * RETURNS:     nothing
	 */
    std::string GetUserProfileStrEncrypted( const char *szName, const char *szDefault = NULL,
		const bool bServer = false) {
		return GetUserProfileStr(szName, szDefault, bServer);
	}

	/*
	 * NAME:        GetUserProfileEncrypted
	 * ACTION:      Returns the user profile string in an encrypted form.
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              UserData& userData  - container for the deserialized data.
     * RETURNS:     true if successful, false otherwise
	 */
    bool GetUserProfileEncrypted( const char *szName, UserData& userData);

#else
	/*
	 * NAME:        SetUserProfileStrEncrypted
	 * ACTION:      Sets the user profile string in an encrypted form.
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              const char* szValue - optional default value
     * RETURNS:     nothing
	 */
    void SetUserProfileStrEncrypted(const char *szName,const char *szValue);

	/*
	 * NAME:        SetUserProfileEncrypted
	 * ACTION:      Serializes and encrypts the user data
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              UserData& userData - contains the user data for serialization
     * RETURNS:     nothing
	 */
    void SetUserProfileEncrypted(const char *szName, const UserData& userData);

	/*
	 * NAME:        GetUserProfileStrEncrypted
	 * ACTION:      Returns the user profile string in an encrypted form.
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              const char* szDefault - optional default value
	 *              const bool bServer - if true, value is retrieved from
	 *				the server.  Otherwise, the data is retreived either from
	 *              the configuration file or registry (Windows).
     * RETURNS:     nothing
	 */
    std::string GetUserProfileStrEncrypted( const char *szName, const char *szDefault = NULL,
	    const bool bServer = false);

	/*
	 * NAME:        GetUserProfileEncrypted
	 * ACTION:      Returns the user profile string in an encrypted form.
     * PARAMETERS:  const char* szName - the name of the configuration parameter
     *              UserData& userData  - container for the deserialized data.
     * RETURNS:     true if successful, false otherwise
	 */
    bool GetUserProfileEncrypted( const char *szName, UserData& userData);

    /*
    * NAME:        Serialize
    * ACTION:      Serialize and encrypt the user data
    * PARAMETERS:  UserData& userData - contains the user data for serialization
    *              bool bEncypt - true if encrypting
    *              unsigned long i1 - key to encryption
    *              unsigned long i2 - key to encryption
    * RETURNS:     string - serialized result
    */
	std::string Serialize( const UserData& userData, bool bEncypt, unsigned long i1, unsigned long i2 );

    /*
    * NAME:        Deserialize
    * ACTION:      Deerialize and unencrypt the user data.
    *              At this point all necessary hacks have taken place.
    *              This list is postion dependant
    * PARAMETERS:  listItems = vector of strings
    *              startIndex = the first location to use INDEX NOT ITEM  eg starts @ 0
    *              UserData& userData  - container for the deserialized data.
    * RETURNS:     true if successful, false oterwise. - serialized result
    */
    bool Deserialize( const std::vector<std::string> listItems, UINT startIndex, UserData& userData );

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
    * RETURNS:     true if successful, false otherwise. - serialized result
    */
    bool Deserialize( const std::string szSerializedData, bool bEncrypted, unsigned long i1, unsigned long i2,
                      UserData& userData );


#endif

#ifdef _WIN32

private:

	/*
	* NAME:        GetUserAppKey
	* ACTION:      Retrieves the registration application key for
	*              the user profile data.
	* PARAMETERS:  HKEY reference.
    * RETURNS:     bool - true if access is successful.
	*/

	bool GetUserAppKey(HKEY& hAppKey);

	/*
	* NAME:        GetAppKey
	* ACTION:      Retrieves the registration application key for
	*              the application profile data.
	* PARAMETERS:  HKEY reference.
    * RETURNS:     bool - true if access is successful.
	*/

	bool GetAppKey(HKEY& hAppKey);

	/*
	* NAME:        GetSubKeyValues
	* ACTION:      Retreives the values under a key
	* PARAMETERS:  HKEY.
    * RETURNS:     bool - true if access is successful.
	*/

	bool GetSubKeyValues(HKEY hKey);

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

    bool GetRegistryStr(HKEY hAppKey, const char* szName, char* szValue,
	    const char* szDefault = NULL);

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

    bool GetRegistryInt(HKEY hAppKey, const char* szName, int& nValue,
        int nDefault = 0);

    /*
     * NAME:        SetRegistryStr
     * ACTION:      Set a configuration value directly to the registry.
	 * PARAMETERS:  HKEY hAppKey: HKEY_LOCAL_MACHINE or HKEY_LOCAL_USER
     *              application key.
     *              char* szName - the name of the configuration parameter
     *              char* szValue - the value
     * RETURNS:     true if successful
     */

    bool SetRegistryStr(HKEY hAppKey, const char* szName, const char* szValue);

    /*
     * NAME:        SetRegistryInt
     * ACTION:      Set an integer configuration value into the registry.
	 * PARAMETERS:  HKEY hAppKey: HKEY_LOCAL_MACHINE or HKEY_LOCAL_USER
     *              application key.
     *              char* szName - the name of the configuration parameter
     *              int nValue - the value
     * RETURNS:     true if successful
     */

    bool SetRegistryInt(HKEY hAppKey, const char* szName, int nValue);


public:
    //=================================================================
    // HKEY_CURRENT_USER
    //=================================================================

    /*
     * NAME:        GetUserProfileRegistryStr
     * ACTION:      Get a configuration value directly from the registry.
	 * PARAMETERS:  APIs called to retreive the application key.
	 *              const char* szName - the name of the registry key
	 *              char* szValue - holds the value
     *              const char* szDefault - optional default value
     * RETURNS:     bool where true = success
     */

    bool GetUserProfileRegistryStr(const char* szName, char* szValue,
	    const char* szDefault = NULL);

    /*
     * NAME:        GetUserProfileRegistryInt
     * ACTION:      Get an integer configuration value directly from the registry.
	 * PARAMETERS:  char* szName - the name of the configuration parameter
	 *              int& nValue - value found
     *              int nDefault - optional default value
     * RETURNS:     bool where true = success
     */

    bool GetUserProfileRegistryInt(const char* szName, int& nValue, int nDefault = 0);

    /*
     * NAME:        SetUserProfileRegistryStr
     * ACTION:      Set a configuration value directly to the registry.
     * PARAMETERS:  char* szName - the name of the configuration parameter
     *              char* szValue - the value
     * RETURNS:     true if successful
     */

    bool SetUserProfileRegistryStr(const char* szName, const char* szValue);

    /*
     * NAME:        SetUserProfileRegistryInt
     * ACTION:      Set an integer configuration value into the registry.
     * PARAMETERS:  char* szName - the name of the configuration parameter
     *              int nValue - the value
     * RETURNS:     true if successful
     */

    bool SetUserProfileRegistryInt(const char* szName, int nValue);

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

    bool GetCompanyProfileRegistryStr(const char* szName, char* szValue,
	    const char* szDefault = NULL);

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

    bool GetCompanyProfileRegistryInt(const char* szName, int& nValue, int nDefault = 0);

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

    bool GetAppProfileRegistryStr(const char* szName, char* szValue,
	    const char* szDefault = NULL);

    /*
     * NAME:        GetAppProfileRegistryInt
     * ACTION:      Get an integer configuration value directly from the registry.
	 * PARAMETERS:  char* szName - the name of the configuration parameter
	 *              int& nValue - value found
     *              int nDefault - optional default value
     * RETURNS:     bool where true = success
     */

    bool GetAppProfileRegistryInt(const char* szName, int& nValue, int nDefault = 0);

    /*
     * NAME:        SetAppProfileRegistryStr
     * ACTION:      Set a configuration value directly to the registry.
     * PARAMETERS:  char* szName - the name of the configuration parameter
     *              char* szValue - the value
     * RETURNS:     true if successful
     */

    bool SetAppProfileRegistryStr(const char* szName, const char* szValue);

    /*
     * NAME:        SetAppProfileRegistryInt
     * ACTION:      Set an integer configuration value into the registry.
     * PARAMETERS:  char* szName - the name of the configuration parameter
     *              int nValue - the value
     * RETURNS:     true if successful
     */

    bool SetAppProfileRegistryInt(const char* szName, int nValue);

#endif // WIN32

}; // UserProfileData

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Class to contain all the data for the logged in user.
//  on the system.
//
//  Copyright (C) 2008 Diomede Corporation.
//
//  Author: Patty Harris
//

class UserData {
public:
	UserData()
	{
	    m_szUserName = _T("");
	    m_szPassword = _T("");
	    m_szFirstName = _T("");
	    m_szLastName = _T("");
	    m_szEmailAddress = _T("");
	    m_szStreetAddress = _T("");
	    m_szStreet2Address = _T("");
	    m_szCity = _T("");
	    m_szState = _T("");
	    m_szZipCode = _T("");
	    m_szCountry = _T("");
	};

	virtual ~UserData() {};

	char const*	FirstName() const;
	void		FirstName( std::string const& );

	char const*	LastName() const;
	void		LastName( std::string const& );

	char const*	UserName() const;
	void		UserName( std::string const& );

	char const*	Password() const;
	void		Password( std::string const& );

	char const*	EmailAddress() const;
	void		EmailAddress( std::string const& );

	char const*	StreetAddress() const;
	void		StreetAddress( std::string const& );
	char const*	Street2Address() const;
	void		Street2Address( std::string const& );

	char const*	City() const;
	void		City( std::string const& );
	char const*	State() const;
	void		State( std::string const& );
	char const*	Zipcode() const;
	void		Zipcode( std::string const& );
	char const*	Country() const;
	void		Country( std::string const& );

private:
	std::string	        m_szUserName;
	std::string	        m_szPassword;
	std::string	        m_szFirstName;
	std::string	        m_szLastName;

    std::string         m_szEmailAddress;

    std::string         m_szStreetAddress;
    std::string         m_szStreet2Address;
    std::string         m_szCity;
    std::string         m_szState;
    std::string         m_szZipCode;
    std::string         m_szCountry;
};


////////// Inlines ////////////////////////////////////////////////////

inline char const* UserData::FirstName() const {
	return m_szFirstName.c_str();
}
inline void UserData::FirstName( std::string const &s ) {
	m_szFirstName = s;
}
inline char const* UserData::LastName() const {
	return m_szLastName.c_str();
}
inline void UserData::LastName( std::string const &s ) {
	m_szLastName = s;
}
inline char const* UserData::Password() const {
	return m_szPassword.c_str();
}
inline void UserData::Password( std::string const &s ) {
	m_szPassword = s;
}
inline char const* UserData::UserName() const {
	return m_szUserName.c_str();
}
inline void UserData::UserName( std::string const &s ) {
	m_szUserName = s;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

inline char const* UserData::EmailAddress() const {
	return m_szEmailAddress.c_str();
}
inline void UserData::EmailAddress( std::string const &s ) {
	m_szEmailAddress = s;
}

inline char const* UserData::StreetAddress() const {
	return m_szStreetAddress.c_str();
}
inline void UserData::StreetAddress( std::string const &s ) {
	m_szStreetAddress = s;
}
inline char const* UserData::Street2Address() const {
	return m_szStreet2Address.c_str();
}
inline void UserData::Street2Address( std::string const &s ) {
	m_szStreet2Address = s;
}
inline char const* UserData::City() const {
	return m_szCity.c_str();
}
inline void UserData::City( std::string const &s ) {
	m_szCity = s;
}
inline char const* UserData::State() const {
	return m_szState.c_str();
}
inline void UserData::State( std::string const &s ) {
	m_szState = s;
}
inline char const* UserData::Zipcode() const {
	return m_szZipCode.c_str();
}
inline void UserData::Zipcode( std::string const &s ) {
	m_szZipCode = s;
}
inline char const* UserData::Country() const {
	return m_szCountry.c_str();
}
inline void UserData::Country( std::string const &s ) {
	m_szCountry = s;
}


#endif // !defined(__USER_PROFILE_DATA_H__)


/*********************************************************************
 * 
 *  file:  ProfileManager.cpp
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: A simple singleton class to handle persistent user 
 *          profile data objects.
 * 
 *********************************************************************/

#include "Stdafx.h"
#include "ProfileManager.h"
#include "ErrorCodes/UtilErrors.h"

#include <stdlib.h>
#include <iostream>

using namespace ::std;

// Should properly delete the profile manager instance

ProfileManager* ProfileManager::m_pProfileMgr = NULL;

///////////////////////////////////////////////////////////////////////
ProfileManager* ProfileManager::Instance()
{
    /*
	static ProfileManager profileMgr;
	m_pProfileMgr = &profileMgr;
	return &profileMgr;
	*/

	if (m_pProfileMgr == NULL) {
	    m_pProfileMgr = new ProfileManager;
	}

	return m_pProfileMgr;

} // End Instance

///////////////////////////////////////////////////////////////////////
void ProfileManager::Shutdown()
{
    Dictionary<UserProfileData*>::iterator it;

    // Delete all the profiles and cleanup the manager itself.
	UserProfileData* usd;

	m_Lock.Lock();

    Dictionary<UserProfileData*>::iterator end = m_listProfiles.end();

    for (it = m_listProfiles.begin(); !(it == end); it++) {
        usd = *it;
		delete usd;
    }

	if ( this == m_pProfileMgr )
		m_pProfileMgr = NULL;

	delete this;

} // End Shutdown

///////////////////////////////////////////////////////////////////////
UserProfileData* ProfileManager::GetProfile( string szProfileName, string szUsername,
                                             string szPassword )
{
    m_Lock.Lock();

    if ( true == Exists(&szProfileName) ) {
        m_Lock.Unlock();
    } 
    else {
        m_Lock.Unlock();

        if ( ERR_NONE != InitProfile( szProfileName, szUsername, szPassword ) )
            return NULL;
    }

    return m_listProfiles[szProfileName.c_str()];

} // End GetProfile

///////////////////////////////////////////////////////////////////////
// Purpose: Get the profile data, initializing the data if it doesn't
//          exist.
// Requires:
//      szProfileName: profile name (e.g. Diomede)
//      bCreateConfigFile: option to create the configuration file
//                         if it doesn't exist.  If false, the 
//                         default values are used.
// Returns: UserProfileData if successful, NULL otherwise.
UserProfileData* ProfileManager::GetProfile( string szProfileName, bool bCreateConfigFile )
{
    m_Lock.Lock();

    if ( true == Exists(&szProfileName) ) {
        m_Lock.Unlock();
    } 
    else {
        m_Lock.Unlock();

        if ( ERR_NONE != InitProfile( szProfileName, _T(""), _T(""), bCreateConfigFile ) )
            return NULL;
    }

    return m_listProfiles[szProfileName.c_str()];

} // End GetProfile

///////////////////////////////////////////////////////////////////////
UserProfileData* ProfileManager::GetProfile( string szProfileName )
{
    MutexLock ml(m_Lock);

    if ( true == Exists(&szProfileName) ) {
        return m_listProfiles[szProfileName.c_str()];
    }
    else {
        return NULL;
    }

} // End GetProfile

///////////////////////////////////////////////////////////////////////
bool ProfileManager::Exists( string* pszProfileName )
{
    if ( pszProfileName && (true == MapProfileName(pszProfileName)) ) {
        return m_listProfiles.exists((*pszProfileName).c_str());
    }
    else {
        return false;
    }

} // End Exists

///////////////////////////////////////////////////////////////////////
bool ProfileManager::Exists( string szProfileName )
{
    return (true == MapProfileName(&szProfileName)) ? 
                m_listProfiles.exists(szProfileName.c_str()) : false;

} // End Exists

///////////////////////////////////////////////////////////////////////
bool ProfileManager::Remove(string szProfileName)
{
    bool bReturn = false;

    MutexLock ml(m_Lock);

    if ( true == Exists(&szProfileName) ) {
        UserProfileData *pProfile = m_listProfiles[szProfileName.c_str()];
        bReturn = m_listProfiles.remove(szProfileName.c_str());
        delete pProfile;
    }

    return bReturn;

} // End Remove

///////////////////////////////////////////////////////////////////////
string ProfileManager::ProfilePath()
{
    if ( m_szProfilePath.length() != 0 ) {
        return m_szProfilePath;
    }

    if ( m_szResourcePath.length() == 0 ) {
        m_szResourcePath = ResourcePath();
    }

	if ( m_szResourcePath == _T("./")) {
        m_szProfilePath = _T("./");
	}

    /*
    else
        m_szProfilePath = m_szResourcePath + _T("etc/");
    */

	return m_szProfilePath;

} // End ProfilePath

///////////////////////////////////////////////////////////////////////
string ProfileManager::ResourcePath()
{
#ifndef WIN32
    if ( 0 != m_szResourcePath.length() ) {
        return m_szResourcePath;
    }

    m_szResourcePath = _T("./");

#endif

    return m_szResourcePath;

} // End ResourcePath

///////////////////////////////////////////////////////////////////////
bool ProfileManager::MapProfileName( string *pszProfileName, string *pFilename )
{
    if ( !pszProfileName ) {
        return false;
    }

    bool result = true;

    const char* szFileName;

    /* For debugging
    size_t size = (*pszProfileName).size();
    */

    if ( _T("Logging") == *pszProfileName ) {
        szFileName = _T("logging.dcf");
    } else if ( _T("Diomede") == *pszProfileName ) {
        szFileName = _T("diomede.dcf");
    } else {
        *pszProfileName = _T("configure");
        szFileName = _T("configure.dcf");
    }

#ifdef WIN32
    if ( pFilename ) {
        *pFilename = szFileName;
    }
#else
    if ( pFilename ) {
        *pFilename = ProfilePath()  + szFileName;

        /*
        std::cout << m_szApplicationName << " profile (" << *pszProfileName << ") is "
             << *pFilename << endl;
        */
    }
#endif
    return result;

} // End MapProfileName

///////////////////////////////////////////////////////////////////////
// Purpose: Initialize the profile data
// Requires:
//      szProfileName: profile name (e.g. Diomede)
//      szUsername: user's name (useful in a DB/multi-user scenario
//      szPassword: user's password (useful in a DB/multi-user scenario
//      bCreateConfigFile: option to create the configuration file
//                         if it doesn't exist.  If false, the 
//                         default values are used.
// Returns: UserProfileData if successful, NULL otherwise.
ErrorType ProfileManager::InitProfile( string szProfileName, 
                                       string szUsername, string szPassword,
                                       bool bCreateConfigFile /*true*/ )
{
    UserProfileData* profileData;
    ErrorType error;

    MutexLock ml(m_Lock);

    if ( szProfileName.empty() ) {
        return ERR_PROFILE_NO_NAME;
    }

    string szFileName = _T("logging.dcf");
    if ( false == MapProfileName( &szProfileName, &szFileName ) ) {
        return ERR_PROFILE_CANT_MAP_NAME;
    }
    else {
        szFileName = _T("diomede.dcf");
        if ( false == MapProfileName( &szProfileName, &szFileName ) ) {
            return ERR_PROFILE_CANT_MAP_NAME;
        }
    }

    if ( true == Exists(szProfileName) ) {
        return ERR_PROFILE_ALREADY_EXIST;
    }

    profileData = new UserProfileData();

    if ( NULL == profileData ) {
        return ERR_OUT_OF_MEMORY;
    }

    error = profileData->GetUserProfile( szUsername.c_str(), szPassword.c_str(),
                                         szFileName.c_str(), szProfileName.c_str(),
                                         bCreateConfigFile );

    if ( ERR_NONE != error ) {
        delete profileData;
        return error;
    }

    m_listProfiles[szProfileName.c_str()] = profileData;

    return ERR_NONE;

} // End InitProfile



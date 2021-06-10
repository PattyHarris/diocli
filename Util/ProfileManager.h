/*********************************************************************
 * 
 *  file:  ProfileManager.h
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

#ifndef __PROFILE_MANAGER_H__
#define __PROFILE_MANAGER_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "UserProfileData.h"
#include "ErrorType.h"
#include "configure.h"
#include "CustomMutex.h"
#include <string>

using namespace std;

//////////////////////////////////////////////////////////////////////
class ProfileManager
{
  protected:
    static ProfileManager*                      m_pProfileMgr;
    Mutex                                       m_Lock;
    Dictionary<UserProfileData*>                m_listProfiles;
    std::string                                 m_szResourcePath;
    std::string                                 m_szProfilePath;
    std::string                                 m_szApplicationName;

    ProfileManager()
    {
        #ifdef WIN32
            m_szApplicationName = _T("DioCLI");
        #else
            m_szApplicationName = _T("diocli");
        #endif
        
        m_szResourcePath = _T("");
        m_szProfilePath = _T("");

    }

    bool MapProfileName( std::string*, std::string* = 0 );
    bool Exists( std::string *pszProfileName );

  public:
    static ProfileManager* Instance();

    ~ProfileManager()
    {
        m_Lock.Unlock();
    }

	// Cleanup the profile mamanager.
	void Shutdown();

    // Get a pointer to a profile, return NULL if it doesn't exist.
    UserProfileData* GetProfile( std::string szProfileName );

    // This one will call InitProfile to create the profile if it doesn't
    // exist, returns NULL if unable to create the profile.
    UserProfileData* GetProfile( std::string szProfileName, std::string szUsername, std::string szPassword );
    UserProfileData* GetProfile( std::string szProfileName, bool bCreateConfigFile );

    // Returns true if profile already exists in the manager
    bool Exists( std::string szProfileName );

    // Removes a profile from the manager by name
    bool Remove( std::string szProfileName );

    // Create a profile in the manager
    ErrorType InitProfile( std::string szProfileName, std::string szUsername = _T(""), 
                           std::string szPassword = _T(""), bool bCreateConfigFile=true );
    std::string ProfilePath();
    std::string ResourcePath();
    
    void SetAppName( std::string szApplicationName ) { 
        m_szApplicationName = szApplicationName; 
   }
};

#endif // __PROFILE_MANAGER_H__


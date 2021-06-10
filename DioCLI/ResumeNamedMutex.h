/*********************************************************************
 *
 *  file:  ResumeNamedMutex.h
 *
 *  Copyright (c) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Interprocess mutex wrapper
 *
 *********************************************************************/

#ifndef __RESUME_MUTEX_H__
#define __RESUME_MUTEX_H__

#include "stdafx.h"
#include "../Include/types.h"
#include "Util.h"

#include <string>

#if defined(linux)
#include <unistd.h>
#endif

#if defined(linux) || defined( __APPLE__)
#include <stdio.h>
#include <stdlib.h>
#endif

#include "boost/interprocess/sync/scoped_lock.hpp"
#include "boost/interprocess/sync/lock_options.hpp"
#include "boost/interprocess/sync/named_mutex.hpp"

using namespace boost::interprocess;
using namespace std;

//---------------------------------------------------------------------
// Interprocess mutex access types
//---------------------------------------------------------------------
#define CREATE_ONLY    0
#define OPEN_OR_CREATE 1
#define OPEN_ONLY      2

///////////////////////////////////////////////////////////////////////
// ResumeNamedMutex
class ResumeNamedMutex
{
private:
    named_mutex             m_namedMutex;

private:
    friend class ResumeScopedLock;

    ResumeNamedMutex(create_only_t c, const char *name)
        : m_namedMutex(c, name)
    {}

    ResumeNamedMutex(open_or_create_t c, const char *name)
        : m_namedMutex(c, name)
    {}

    ResumeNamedMutex(open_only_t c, const char *name)
        : m_namedMutex(c, name)
    {}

    named_mutex & GetMutex() {
        return m_namedMutex;
    }

    virtual ~ResumeNamedMutex(void) {}

    // No implementation for copy constructor and assignment.
    ResumeNamedMutex( const ResumeNamedMutex& );                // Forbid copy
    ResumeNamedMutex& operator=( const ResumeNamedMutex& );     // Forbid assignment

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static ResumeNamedMutex* CreateNamedMutex(int nAccess, std::string szMutexName,
        std::string& szLastError)
    {
        ResumeNamedMutex* pMutex = 0;
        szLastError = _T("");

        try {
            switch(nAccess) {
                case CREATE_ONLY:
                    pMutex = new ResumeNamedMutex(create_only, szMutexName.c_str());
                    break;
                case OPEN_OR_CREATE:
                    pMutex = new ResumeNamedMutex(open_or_create, szMutexName.c_str());
                    break;
                case OPEN_ONLY:
                    pMutex = new ResumeNamedMutex(open_only, szMutexName.c_str());
                    break;
            }
        }
        catch (interprocess_exception &ex) {
            szLastError = ex.what();
        }

        return pMutex;

    } // End CreateNamedMutex

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static void ResumeNamedMutexUnLock(ResumeNamedMutex* pMutex)
    {
        pMutex->GetMutex().unlock();
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static void ResumeNamedMutexLock(ResumeNamedMutex* pMutex)
    {
        pMutex->GetMutex().lock();
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static bool ResumeNamedMutextryLock(ResumeNamedMutex* pMutex)
    {
        return pMutex->GetMutex().try_lock();
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static bool ResumeNamedMutexTimedLock(ResumeNamedMutex* pMutex, int nTime)
    {
        std::time_t tTime(nTime);
        return pMutex->GetMutex().timed_lock(from_time_t(tTime));
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static bool ResumeNamedMutexRemove(std::string* szName)
    {
        bool bResult = named_mutex::remove(szName->c_str());
        return bResult;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static void ResumeNamedMutexFree(ResumeNamedMutex* pMutex)
    {
        delete pMutex;
    }

}; // End ResumeNamedMutex

///////////////////////////////////////////////////////////////////////
// ResumeScopedLock
class ResumeScopedLock
{
private:
    scoped_lock<named_mutex>*           m_pLock;

private:

    //-----------------------------------------------------------------
    // Scoped_lock
    //-----------------------------------------------------------------
    ResumeScopedLock(named_mutex & mutex)
    {
        m_pLock = new scoped_lock<named_mutex>(mutex);
    }

    //-----------------------------------------------------------------
    // Timed scoped_lock
    //-----------------------------------------------------------------
    ResumeScopedLock(named_mutex & mutex, int nWaitPeriod)
    {
        std::time_t tWaitPeriod(nWaitPeriod);
        m_pLock = new scoped_lock<named_mutex>(mutex, from_time_t(tWaitPeriod));
    }

    ~ResumeScopedLock()
    {
        delete m_pLock;
    }

    scoped_lock<named_mutex> * GetLock() {
        return m_pLock;
    }

    void lock() {
        m_pLock->lock();
    }

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static ResumeScopedLock* ResumeScopedLockCreate(ResumeNamedMutex* pMutex)
    {
        return new ResumeScopedLock(pMutex->GetMutex());
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static ResumeScopedLock* ResumeScopedLockCreateTimedLock(ResumeNamedMutex* pMutex, int nWaitPeriod)
    {
        return new ResumeScopedLock(pMutex->GetMutex(), nWaitPeriod);
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static void ResumeScopedLockFree(ResumeScopedLock* pLock)
    {
        delete pLock;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static void ResumeScopedLockLock(ResumeScopedLock* pLock)
    {
        pLock->GetLock()->lock();
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static bool ResumeScopedLockTryLock(ResumeScopedLock* pLock)
    {
        return pLock->GetLock()->try_lock();
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static bool ResumeScopedLockTimedLock(ResumeScopedLock* pLock, int time)
    {
        std::time_t tTime(time);
        return pLock->GetLock()->timed_lock(from_time_t(tTime));
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    static void ResumeScopedLockUnLock(ResumeScopedLock* pLock)
    {
        pLock->GetLock()->unlock();
    }

}; // End ResumeScopedLock

///////////////////////////////////////////////////////////////////////
// ResumeNamedMutexUtil
class ResumeNamedMutexUtil
{
private:
    ResumeNamedMutex*               m_pMutex;
    ResumeScopedLock*               m_pLock;
    std::string                     m_szLastError;

public:
	ResumeNamedMutexUtil(int nAccess, std::string szMutexName)
	    : m_pMutex(NULL), m_pLock(NULL), m_szLastError(_T(""))
	{
	    CreateLock(nAccess, szMutexName);
	}

	~ResumeNamedMutexUtil()
	{
	    if (m_pLock != NULL) {
	        ResumeScopedLock::ResumeScopedLockFree(m_pLock);
	        m_pLock = NULL;
	    }

	    if (m_pMutex != NULL) {
	        ResumeNamedMutex::ResumeNamedMutexFree(m_pMutex);
	        m_pMutex = NULL;
	    }
	};

	bool IsValid() {
	    return m_szLastError.length() == 0;
	}

	std::string GetLastError() {
	    return m_szLastError;
	}

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
	void CreateLock(int nAccess, std::string szMutexName,
	                int nSystemSleep=10, int nRetryCount=50)
	{
	    if (m_pLock != NULL) {
	        ResumeScopedLock::ResumeScopedLockFree(m_pLock);
	        m_pLock = NULL;
	    }

	    // Remove any leftover locks.
        ResumeNamedMutex::ResumeNamedMutexRemove(&szMutexName);

	    if (m_pMutex != NULL) {
	        ResumeNamedMutex::ResumeNamedMutexFree(m_pMutex);
	        m_pMutex = NULL;
	    }

	    // See if we can pause here to enable the current lock to become unlocked.
        int nLoopCount = nRetryCount;
	    if (m_pMutex == NULL) {

	        // Loop a max of 50 times to see if we can get a mutex
	        if (nSystemSleep > 0) {
	            while ( (nLoopCount > 0) && (m_pMutex == NULL) ) {
                    Util::PauseProcess(nSystemSleep);
            	    m_pMutex = ResumeNamedMutex::CreateNamedMutex(nAccess, szMutexName, m_szLastError);
            	    nLoopCount --;
	            }
	        }
	        else {
	            // Try one more time anyway
    	        m_pMutex = ResumeNamedMutex::CreateNamedMutex(nAccess, szMutexName, m_szLastError);
    	    }
	    }

	    if (m_pMutex != NULL) {
	        m_pLock = ResumeScopedLock::ResumeScopedLockCreate(m_pMutex);
	        if ((m_pLock == NULL) && (nSystemSleep > 0)) {
	            m_pLock = ResumeScopedLock::ResumeScopedLockCreateTimedLock(m_pMutex, nSystemSleep);
	        }
	    }

	} // End CreateLock

}; // End ResumeNamedMutexUtil

#endif // __RESUME_MUTEX_H__

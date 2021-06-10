/*********************************************************************
 * 
 *  file:  MemLogger.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Manages client logging to memory.
 * 
 *********************************************************************/

#ifndef __MEM_LOGGER_H__
#define __MEM_LOGGER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ClientLog.h"
#include "CriticalSection.h"
#include "Stdafx.h"
#include <string>

#ifndef WIN32
#include <sys/time.h>
#endif

/////////////////////////////////////////////////////////////////////////////
class CLogMsg
{
    public:
	    string          m_sLine;
	    ULONG           m_ulTimeAdded;

	    CLogMsg( const string& sLine, int len)
	    {
	        m_sLine = sLine;
	        
		#ifdef WIN32
	        m_ulTimeAdded = GetTickCount();
		#else
	        timeval tv;
	        gettimeofday(&tv, NULL);
	        m_ulTimeAdded = (tv.tv_usec / 1000) + (tv.tv_sec * 1000);
	    #endif
	    }

};

/////////////////////////////////////////////////////////////////////////////
class CMemLogger : public LogObserver
{
public:

	CMemLogger();
	virtual ~CMemLogger();
	bool Init( ULONG ulCacheTime);
	string GetCacheString();

	virtual bool IsIgnored( int nComponent, int nEvent);
	virtual void LogMessageCallback( const string& pMessage, int nLen);

    // Used for comparison within the list of log observers.
	virtual void SetLogObserverType() {
	    m_nLogObserverType = MEM_LOGGER_TYPE;
	}

protected:

	typedef std::list<CLogMsg>      	LogMsgList;
	LogMsgList                      	m_CircularBuffer;

	ULONG                           	m_ulCacheTime;
	DIOMEDE_CRITICAL::CCriticalSection	m_lock;
};

#endif // __MEM_LOGGER_H__

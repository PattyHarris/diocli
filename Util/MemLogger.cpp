/*********************************************************************
 * 
 *  file:  MemLogger.cpp
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

#include "Stdafx.h"
#include "MemLogger.h"

using DIOMEDE_CRITICAL::CCriticalSection;
using DIOMEDE_CRITICAL::Lock;

///////////////////////////////////////////////////////////////////////
CMemLogger::CMemLogger()
{
	m_ulCacheTime = 0;
}

//////////////////////////////////////////////////////////////////////
CMemLogger::~CMemLogger()
{
    while (m_CircularBuffer.size() > 0) {
        CLogMsg aLogMsg = (CLogMsg)m_CircularBuffer.front();
        m_CircularBuffer.pop_front();

    }
}

//////////////////////////////////////////////////////////////////////
bool CMemLogger::Init( ULONG ulCacheTime)
{
	m_ulCacheTime = ulCacheTime;
	return true;

} // End Init

//////////////////////////////////////////////////////////////////////
bool CMemLogger::IsIgnored( int nComponent, int nEvent)
{
	return false;

} // End IsIgnored

//////////////////////////////////////////////////////////////////////
void CMemLogger::LogMessageCallback( const string& pMessage, int nLen)
{
	ULONG ulNow = 0;
	#ifdef WIN32
		ulNow = GetTickCount();
	#else
	    timeval tv;
	    gettimeofday(&tv,NULL);
	    ulNow = (tv.tv_usec / 1000) + (tv.tv_sec * 1000);
	#endif

	CLogMsg aLogMsg(pMessage, nLen);

	Lock<CCriticalSection> lock( m_lock);
	m_CircularBuffer.push_back( aLogMsg);

	/* For debugging
	int nSize = (int)m_CircularBuffer.size();
	*/

	aLogMsg = m_CircularBuffer.front();

	// Remove all the old log events.
	while ((ulNow - aLogMsg.m_ulTimeAdded) > m_ulCacheTime) {
		    m_CircularBuffer.pop_front();

		    if (m_CircularBuffer.size() == 0) {
		        break;
		    }

		    aLogMsg = m_CircularBuffer.front();
	}

} // End LogMessageCallback

//////////////////////////////////////////////////////////////////////
string CMemLogger::GetCacheString()
{
	Lock<CCriticalSection> lock( m_lock);

	string sRetVal;
    LogMsgList::iterator lit;
    for (lit = m_CircularBuffer.begin(); lit != m_CircularBuffer.end(); lit++) {
       	CLogMsg aLogMsg = (*lit);
		sRetVal += aLogMsg.m_sLine;
	}

	return sRetVal;

} // End GetCacheString

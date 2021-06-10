/*********************************************************************
 * 
 *  file:  ILogObserver.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Client logging interface
 * 
 *********************************************************************/

//////////////////////////////////////////////////////////////////////

#ifndef __ILOG_OBSERVER_H__
#define __ILOG_OBSERVER_H__

#include <string>
using namespace std;

enum {
    FILE_LOGGER_TYPE,
    STD_OUTPUT_LOGGER_TYPE,
    MEM_LOGGER_TYPE,
    DEBUGGER_LOGGER_TYPE,
    UI_LOGGER_TYPE
};

class ILogObserver {
protected:
    int     m_nLogObserverType;
public:
	virtual void LogMessageCallback(const std::string& pMessage, int len)=0;
	virtual bool IsIgnored(int nComponent, int nEvent)=0;
	virtual void SetLogObserverType()=0;

	int GetLogObserverType() { return m_nLogObserverType; }

};

#endif // __ILOG_OBSERVER_H__

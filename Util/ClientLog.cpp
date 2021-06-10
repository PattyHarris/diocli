/*********************************************************************
 * 
 *  file:  ClientLog.cpp
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Provides client logging utilities
 * 
 *********************************************************************/
#include "Stdafx.h"

#include "CriticalSection.h"
#include "ILogObserver.h"

#include "ClientLog.h"
#include "XString.h"
#include <stdarg.h>
#include <stdio.h>


#include "boost/date_time/posix_time/posix_time.hpp"
#include <iostream>

using namespace boost::posix_time;
using namespace boost::gregorian;

using DIOMEDE_CRITICAL::CCriticalSection;
using DIOMEDE_CRITICAL::Lock;

#define HEADER_PADDING 100  //this is extra for the time, comp, event stamp

static const char* g_Events[] = { _T("DF"),  _T("ST"),  _T("EX"),  _T("ER"),  _T("WA"),  _T("SC"),  _T("PE"),  _T("SE") };

///////////////////////////////////////////////////////////////////////
class Logger {
public:
	Logger();
	~Logger();

	void EnableLogging(bool bOn) { m_bEnableLogging = bOn; }
	bool LoggingEnabled() const { return m_bEnableLogging; }

	bool SetDefaultLoggingEvents(int eventMask) {
		if ( eventMask & LOG_DEFAULT)
			return false;
		m_iDefaultLoggingEvents = eventMask;
		return true;
	}

	int GetDefaultLoggingEvents() { return m_iDefaultLoggingEvents; }

	bool SetComponentLogging(int componentIndex, int eventMask);

	bool RegisterLogObserver(ILogObserver* pLogObserver) {
        Lock<CCriticalSection> lock(m_observersLock);
		m_observers.push_back(pLogObserver);
		return true;
	}

	bool UnRegisterLogObserver(ILogObserver* pLogObserver) {
        Lock<CCriticalSection> lock(m_observersLock);

        LogObserverList::iterator lit;
        for (lit = m_observers.begin(); lit != m_observers.end(); lit++) {

    		ILogObserver* tmpObserver = (*lit);
			if (pLogObserver->GetLogObserverType() == tmpObserver->GetLogObserverType()) {
		        m_observers.erase(lit);
				return true;
			}
		}
		return false;
	}

	int GetComponentLogging(int componentIndex) {
		return m_logLevels[componentIndex];
	}

	void LogMessage(int nComponentNdx, int nEvent, const string& message, int len);

private:
	int                                 m_iObservers;

	typedef std::list<ILogObserver*>    LogObserverList;
	LogObserverList                     m_observers;

    CCriticalSection                    m_observersLock;

	bool                                m_bEnableLogging;
	int                                 m_iDefaultLoggingEvents;
	unsigned int                        m_logLevels[ERROR_TOTAL_COMPONENTS];
};

///////////////////////////////////////////////////////////////////////
class CLoggerWrapper
{
    public:
        CLoggerWrapper():m_pTheLogger(0) {
            m_pTheLogger = new Logger;
        }

        ~CLoggerWrapper() { delete m_pTheLogger; }

        Logger *ptr() {
            if ( !m_pTheLogger )
                m_pTheLogger = new Logger;
            return m_pTheLogger;
        }
    private:
        Logger* m_pTheLogger;
};

static CLoggerWrapper g_theLogger;

///////////////////////////////////////////////////////////////////////
Logger::Logger()
{
	unsigned int i;
    for(i=0; i < (unsigned int)ERROR_TOTAL_COMPONENTS; i++) {
        m_logLevels[i]=0;
    }
}

///////////////////////////////////////////////////////////////////////
Logger::~Logger()
{
    Lock<CCriticalSection> lock(m_observersLock);

     while (m_observers.size() > 0) {
    	/* For testing ..
        ILogObserver* pLogObserver = m_observers.front();
        */
        m_observers.pop_front();
    }

}

///////////////////////////////////////////////////////////////////////
bool RegisterLogObserver(ILogObserver* pLogObserver)
{
	return g_theLogger.ptr()->RegisterLogObserver(pLogObserver);

} // End RegisterLogObserver

///////////////////////////////////////////////////////////////////////
bool RegisterLogObserver(LogObserver* pLogObserver)
{
	return g_theLogger.ptr()->RegisterLogObserver(pLogObserver);

} // End RegisterLogObserver

///////////////////////////////////////////////////////////////////////
bool UnRegisterLogObserver(ILogObserver* pLogObserver)
{
	return g_theLogger.ptr()->UnRegisterLogObserver(pLogObserver);

} // End UnRegisterLogObserver

///////////////////////////////////////////////////////////////////////
bool UnRegisterLogObserver(LogObserver* pLogObserver)
{
	return g_theLogger.ptr()->UnRegisterLogObserver(pLogObserver);

} // End UnRegisterLogObserver

///////////////////////////////////////////////////////////////////////
void Logger::LogMessage( int nComponentIndex, int nEvent, const string& msg, int len )
{
    Lock<CCriticalSection> lock(m_observersLock);

 	int nTmpIndex = GetNormalizedIndex(nComponentIndex);

    LogObserverList::iterator lit;
    for (lit = m_observers.begin(); lit != m_observers.end(); lit++) {
		ILogObserver* observer = (ILogObserver*)(*lit);
		if ( !observer->IsIgnored( nTmpIndex, nEvent)) {
			observer->LogMessageCallback(msg, len);
		}
	}

} // End LogMessage

///////////////////////////////////////////////////////////////////////
bool Logger::SetComponentLogging(int nComponentIndex, int eventMask)
{
	unsigned int nTmpIndex = GetNormalizedIndex(nComponentIndex);
	unsigned int nAlwaysComponent = ((ULONG)ALWAYS_COMPONENT_INDEX)>>24;

    if (!((nTmpIndex >= 0 && nTmpIndex < (unsigned int)ERROR_TOTAL_COMPONENTS) ||
        nTmpIndex == nAlwaysComponent )) {
        // invalid componentIndex
        return false;
    }

    if (nTmpIndex == nAlwaysComponent) {
        for (unsigned int i=0; i < (unsigned int)ERROR_TOTAL_COMPONENTS; i++) {
           m_logLevels[i] = eventMask;
        }
    } else {
        m_logLevels[nTmpIndex] = eventMask;
    }

    return true;

} // End SetComponentLogging

///////////////////////////////////////////////////////////////////////
bool LogObserver::IsIgnored(int nComponentIndex, int nEvent)
{
	int nTmpIndex = GetNormalizedIndex(nComponentIndex);
	unsigned int nAlwaysComponent = ((unsigned int)ALWAYS_COMPONENT_INDEX)>>24;

	// If logging is disabled just return on out of here,
	if( !g_theLogger.ptr()->LoggingEnabled() && nComponentIndex != (int)nAlwaysComponent) {
		return true;
	}

    int eventsToLog = GetComponentLogging( nTmpIndex );
    if (eventsToLog == LOG_DEFAULT)
        eventsToLog = GetDefaultLoggingEvents();

    if ((eventsToLog & nEvent) == 0)
        return true;

	return false;

} // End IsIgnored

/////////////////////////////////////////////////////////////////////////////
// External ClientLog Interface

///////////////////////////////////////////////////////////////////////
void EnableLogging(bool bOn)
{
	g_theLogger.ptr()->EnableLogging(bOn);

} // End EnableLogging

///////////////////////////////////////////////////////////////////////
bool SetDefaultLoggingEvents(int eventMask)
{
	return g_theLogger.ptr()->SetDefaultLoggingEvents(eventMask);

} // End SetDefaultLoggingEvents

///////////////////////////////////////////////////////////////////////
int GetDefaultLoggingEvents()
{
    return g_theLogger.ptr()->GetDefaultLoggingEvents();

} // End GetDefaultLoggingEvents

///////////////////////////////////////////////////////////////////////
bool SetComponentLogging(int nComponentIndex, int eventMask)
{
    int nTmpIndex = GetNormalizedIndex(nComponentIndex);
	return g_theLogger.ptr()->SetComponentLogging(nTmpIndex, eventMask);

} // End SetComponentLogging

/////////////////////////////////////////////////////////////////////////////
int GetComponentLogging(int nComponentIndex)
{
	unsigned int nTmpIndex = GetNormalizedIndex(nComponentIndex);
	unsigned int nAlwaysComponent = ((ULONG)ALWAYS_COMPONENT_INDEX)>>24;

    if (!((nTmpIndex >= 0 && nTmpIndex < (unsigned int)ERROR_TOTAL_COMPONENTS) ||
        nTmpIndex == nAlwaysComponent)) {
        // invalid componentIndex
        return 0;
    }
    if (nTmpIndex == nAlwaysComponent)
        return LOG_ALL;

    return g_theLogger.ptr()->GetComponentLogging(nTmpIndex);

} // End GetComponentLogging

///////////////////////////////////////////////////////////////////////
void GetComponentName( int nComponentIndex, CLogEvent* pLogEvent )
{
    int nTmpIndex = GetNormalizedIndex(nComponentIndex);
	unsigned int nAlwaysComponent = ((ULONG)ALWAYS_COMPONENT_INDEX)>>24;

    if (nTmpIndex == (int)nAlwaysComponent) {
        pLogEvent->m_szComponentName =  _T("TIME");
        return;
    }

    string name = GetErrorComponentName(nTmpIndex);
    if (name.length() > 0) {
        pLogEvent->m_szComponentName = name;
    }
    else {
        pLogEvent->m_szComponentName = _T("UNKNWN");
    }
    return;

} // End GetComponentName

///////////////////////////////////////////////////////////////////////
static int logbase2(int bit)
{
    int c = 0;
    while (bit) {
        bit = bit>>1;
        c++;
    }
    return c;

} // End logbase2

///////////////////////////////////////////////////////////////////////
static void GetEventName(int nEvent, CLogEvent* pLogEvent)
{
    int base2 = logbase2(nEvent);
    if (base2 <= SECURITY_BIT && base2 > DEFAULT_BIT)
        pLogEvent->m_szEventName = g_Events[base2-1];
    else
        pLogEvent->m_szEventName = _T("??");

} // End GetEventName

size_t kMaxLineLen = 1024;

///////////////////////////////////////////////////////////////////////
int FormatClientLog(char* buffer, int nCount, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    int nResultCount = _vsnprintf(buffer, nCount, format, args);
    buffer[nResultCount] = _T('\0');
    va_end(args);

    return nResultCount;

} // End FormatClientLog

///////////////////////////////////////////////////////////////////////
void ClientLog( const int component, const int nEvent, const bool continuation,
                const char* message, ...)
{
    /*
	SYSTEMTIME systime;
	GetLocalTime(&systime);
	*/

    // Replacing the above with Boost date_time - note that second_clock is only precise
    // up to seconds, not milleseconds...using microsec_clock to achieve a higher
    // precision.
    ptime tmNow = microsec_clock::local_time();

    int componentIndex = ((ULONG)component)>>24;

    // Setup the component name and even name - in STL/UNICODE environments, the
    // strings need to be static and global.
    CLogEvent logEvent;
    GetComponentName( componentIndex, &logEvent );
    GetEventName( nEvent, &logEvent );

	va_list args;
	va_start(args, message);

    char lineBuffer[kMaxLogMessageSize+100];
    char messageBuffer[kMaxLogMessageSize];
    messageBuffer[kMaxLogMessageSize-1] = 0;

	int messageLen = vsnprintf(messageBuffer, kMaxLogMessageSize-1, message, args);

    if ((messageLen > kMaxLogMessageSize-1) || (messageLen < 0)) {
        // handle both linux and windows versions of return code
        // messageLen is now strlen(message)
        messageLen = kMaxLogMessageSize - 1;
		// indicate that message was truncated
		char* tail = messageBuffer + kMaxLogMessageSize - 4;
        memset( tail, '.', 3);
    }

    va_end(args);

    //-----------------------------------------------------------------
    // Simple test code for FormatClientLog
    //-----------------------------------------------------------------
    #if 0
	char testBuffer[100];
	string szLabel = _T("Component Name: =");
    int nTest = FormatClientLog( testBuffer, sizeof(testBuffer), _T("%s %s"), CS(szLabel),
                                 CS(logEvent.m_szComponentName));
    #endif

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
	// write the line header to the linebuffer
	long lHours = tmNow.time_of_day().hours();
	long lMinutes = tmNow.time_of_day().minutes();
	long lSeconds = tmNow.time_of_day().seconds();
	LONG64 lMilliseconds = tmNow.time_of_day().fractional_seconds();

    #ifdef WIN32
        std::string szMilliseconds = _format(_T("%3.3I64d"), lMilliseconds);
    #else
        std::string szMilliseconds = _format(_T("%3.3lld"), lMilliseconds);
    #endif

    int headerLen = FormatClientLog(lineBuffer, sizeof(lineBuffer),
                                    _T("%-8s %2ld:%02ld:%02ld.%s %-2s %c "),
                                    CS(logEvent.m_szComponentName),
                                    lHours,                             // systime.wHour,
                                    lMinutes,                           // systime.wMinute,
                                    lSeconds,                           // systime.wSecond,
                                    szMilliseconds.substr(0,3).c_str(), // systime.wMilliseconds,
                                    CS(logEvent.m_szEventName),
                                    (continuation ? _T('+') : _T('|') ));


    int pos = 0;
    char* eol = 0;
	bool firstTime = true;
    do {
        eol = strchr( messageBuffer + pos, _T('\n') );
		size_t lineLen = messageLen;
        char* lineStart = messageBuffer + pos;

        if (eol) {
            lineLen = eol-messageBuffer - pos;
            pos += (int)lineLen + 1; // advance past the newline
			messageLen -= (int)lineLen + 1;
        }
		if (lineLen) {
			// we need lineLen + header + '\n' + '\0' bytes for the line
            if (lineLen + headerLen + 2 > kMaxLineLen) {
                lineLen = kMaxLineLen - headerLen - 2;
				// indicate that message was truncated
				char* tail = lineStart + lineLen - 3;
                memset( tail, '.', 3);
			}

			string szLineBuffer = lineBuffer;
			string szLineStart = lineStart;

			string szMessageBuffer = szLineBuffer + szLineStart + _T('\n');

			/*
			memcpy(lineBuffer + headerLen, lineStart, lineLen);
			lineBuffer[headerLen + lineLen] = _T('\n');
			lineBuffer[headerLen + lineLen + 1] = _T('\0');
			*/

			g_theLogger.ptr()->LogMessage( componentIndex, nEvent, szMessageBuffer,
			                               (int)szMessageBuffer.length() );

			if (firstTime && messageLen) {
				// if there's more to output, set the continuation char to '*'
				firstTime = false;
				headerLen = FormatClientLog(lineBuffer, sizeof(lineBuffer),
				                            _T("%-8s %2ld:%02ld:%02ld.%s %-2s %c "),
									        CS(logEvent.m_szComponentName),
                                            lHours,                             // systime.wHour,
                                            lMinutes,                           // systime.wMinute,
                                            lSeconds,                           // systime.wSecond,
                                            szMilliseconds.substr(0,3).c_str(), // systime.wMilliseconds,
									        CS(logEvent.m_szEventName),
									        _T('*'));
			}
		}
    } while (eol && messageLen);

} // End ClientLog

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Version of client log that will prepend the File/line at the start
//      so that you can doubleclick on it in the output window
// Requires:
//      file: source file
//      line: line in source file
//      component: module (e.g. UI, etc.)
//      event: event type (e.g. status, error, etc.)
//      continuation: message is a continuation of the previous message
//      message: message to log
// Returns: nothing

void ClientLog(	const char *file,
				const int line,
				const int component,
				const int nEvent,
				const bool continuation,
				const char* message,
				...)
{
    /*
	SYSTEMTIME systime;
	GetLocalTime(&systime);
	*/

    // Replacing the above with Boost date_time - note that second_clock is only precise
    // up to seconds, not milleseconds...using microsec_clock to achieve a higher
    // precision.
    ptime tmNow = microsec_clock::local_time();

    int componentIndex = ((ULONG)component)>>24;

    // Setup the component name and even name - in STL/UNICODE environments, the
    // strings need to be static and global.
    CLogEvent logEvent;
    GetComponentName( componentIndex, &logEvent );
    GetEventName( nEvent, &logEvent );

	va_list args;
	va_start(args, message);

    char lineBuffer[kMaxLogMessageSize+100];
    char messageBuffer[kMaxLogMessageSize];
    messageBuffer[kMaxLogMessageSize-1] = 0;

	int messageLen = vsnprintf(messageBuffer, kMaxLogMessageSize-1, message, args);

    if ((messageLen > kMaxLogMessageSize-1) || (messageLen < 0)) {
        // handle both linux and windows versions of return code
        // messageLen is now strlen(message)
        messageLen = kMaxLogMessageSize - 1;
		// indicate that message was truncated
		char* tail = messageBuffer + kMaxLogMessageSize - 4;
        memset( tail, '.', 3);
    }

    va_end(args);


    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
	// write the line header to the linebuffer
	long lHours = tmNow.time_of_day().hours();
	long lMinutes = tmNow.time_of_day().minutes();
	long lSeconds = tmNow.time_of_day().seconds();
	LONG64 lMilliseconds = tmNow.time_of_day().fractional_seconds();

    #ifdef WIN32
        std::string szMilliseconds = _format(_T("%3.3I64d"), lMilliseconds);
    #else
        std::string szMilliseconds = _format(_T("%3.3lld"), lMilliseconds);
    #endif

    int headerLen = FormatClientLog(lineBuffer, sizeof(lineBuffer),
                                    _T("%-8s %2ld:%02ld:%02ld.%s %-2s %c "),
                                    CS(logEvent.m_szComponentName),
                                    lHours,                             // systime.wHour,
                                    lMinutes,                           // systime.wMinute,
                                    lSeconds,                           // systime.wSecond,
                                    szMilliseconds.substr(0,3).c_str(), // systime.wMilliseconds,
                                    CS(logEvent.m_szEventName),
                                    (continuation ? _T('+') : _T('|') ));
    int pos = 0;
    char* eol = 0;
	bool firstTime = true;
    do {
        eol = strchr( messageBuffer + pos, _T('\n') );
		size_t lineLen = messageLen;
        char* lineStart = messageBuffer + pos;

        if (eol) {
            lineLen = eol-messageBuffer - pos;
            pos += (int)lineLen + 1; // advance past the newline
			messageLen -= (int)lineLen + 1;
        }
		if (lineLen) {
			// we need lineLen + header + '\n' + '\0' bytes for the line
            if (lineLen + headerLen + 2 > kMaxLineLen) {
                lineLen = kMaxLineLen - headerLen - 2;
				// indicate that message was truncated
				char* tail = lineStart + lineLen - 3;
                memset( tail, '.', 3);
			}

			string szLineBuffer = lineBuffer;
			string szLineStart = lineStart;

            //---------------------------------------------------------
            //---------------------------------------------------------
			string szMessageBuffer;
			if ( 0==line) {  // If the line number is 0 then ignore the pre stuff
				szMessageBuffer = szLineBuffer + _T('\n');
			} else {
				char buffer[200];
				sprintf(buffer,_T("%25s (%4d): "),file,line);
				szMessageBuffer = buffer + szLineBuffer + szLineStart + _T('\n');
			}
            //---------------------------------------------------------
            //---------------------------------------------------------


			g_theLogger.ptr()->LogMessage( componentIndex, nEvent, szMessageBuffer,
			                              (int)szMessageBuffer.length() );

			if (firstTime && messageLen) {
				// if there's more to output, set the continuation char to '*'
				firstTime = false;
				headerLen = FormatClientLog(lineBuffer, sizeof(lineBuffer),
				                            _T("%-8s %2ld:%02ld:%02ld.%s %-2s %c "),
									        CS(logEvent.m_szComponentName),
                                            lHours,                             // systime.wHour,
                                            lMinutes,                           // systime.wMinute,
                                            lSeconds,                           // systime.wSecond,
                                            szMilliseconds.substr(0,3).c_str(), // systime.wMilliseconds,
									        CS(logEvent.m_szEventName),
									        _T('*'));
			}
		}
    } while (eol && messageLen);

} // End ClientLog

///////////////////////////////////////////////////////////////////////
ErrorType ClientLogError(ErrorType err, const int nEvent, const bool continuation,
                         const char* prefixString, ...)
{
	// TODO: is 1024 enough?
	char buffer[1024];
	va_list marker;
	va_start(marker, prefixString);

	std::string szError = err.AsString();

	if( sizeof(prefixString) > 0)
	{
		sprintf(buffer, _T("%s %s"), prefixString, szError.c_str());
		ClientLog(err.GetComponent(), nEvent, continuation, buffer, marker);
	}
	else
	{
		ClientLog(err.GetComponent(), nEvent, continuation, _T("%s"), szError.c_str());
	}

	return err;

} // End ClientLogError

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Adjust the component index -
//      Very hacky - if the given value is less than the number of
//      components, then use the array of names directly.  Otherwise,
//      assume the value is the component's actual value - use that
//      value to find the array index.
//	Requires:
//       nComponentIndex: as defined in ErrorTypes.h
//  Returns:
//      normalized value (e.g value < number of components.
int GetNormalizedIndex(unsigned int nComponentIndex)
{
    // The shifting here is due to the nature of the original code and should
    // be cleaned up - values here should be consistent - that is, position in
    // arrays should have some relationship with the actual component value.
    unsigned int nFirstComponent = ((ULONG)DIOMEDE_FIRST_COMPONENT)>>24;
    unsigned int nLastComponent = ((ULONG)DIOMEDE_LAST_COMPONENT)>>24;

    if ( (nComponentIndex >= DIOMEDE_FIRST_COMPONENT) && (nComponentIndex <= DIOMEDE_LAST_COMPONENT)) {
        nComponentIndex = ((ULONG)nComponentIndex)>>24;
    }

    if (nComponentIndex > nLastComponent) {
        return 0;
    }

    int nTmpIndex = nComponentIndex;

    if ( nComponentIndex >= nFirstComponent ) {
        nTmpIndex = GetComponentIndex(nComponentIndex);
    }

    return nTmpIndex;

} // End GetNormalizedIndex

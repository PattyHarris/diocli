/*********************************************************************
 * 
 *  file:  ClientLog.h
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

#ifndef __CLIENT_LOG_H__
#define __CLIENT_LOG_H__

#include "ErrorType.h"
#include "ILogObserver.h"
#include "configure.h"

//////////////////////////////////////////////////////////////////////
// For use with the new ClientLog
#define MAKE_IT_SHORT 0

#if MAKE_IT_SHORT
	#define WHEREIAM  _T(""),0
	#define WHEREIAMX _T(""),0,_T(""),
#else
    // eg ClientLog(WHEREIAM,PEER_........
	#define WHEREIAM  _T(__FILE__),__LINE__
	#define WHEREIAMX _T(__FILE__),__LINE__,_T(__FUNCTION__),
#endif

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
class CLogEvent
{
    public:
	    std::string         m_szComponentName;
	    std::string         m_szEventName;

        CLogEvent() {};

	    CLogEvent( const std::string& szComponentName, const std::string& szEventName)
	    {
	        m_szComponentName = szComponentName;
	        m_szEventName = szEventName;

	        /* wcscpy is marked as depracated
	        wcscpy((char*)m_szComponentName.c_str(), (char*)szComponentName.c_str());
	        wcscpy((char*)m_szEventName.c_str(), (char*)szEventName.c_str());
	        */
	    }

};

//EVENT TYPES
enum eEventBits {
    DEFAULT_BIT = 0,
    STATUS_BIT,
    EXCEPTION_BIT,
    ERROR_BIT,
    WARNING_BIT,
    STATECHANGE_BIT,
    PERFORMANCE_BIT,
    SECURITY_BIT
};

// Default events
int const LOG_DEFAULT       = 1 << DEFAULT_BIT;
//Status Messages
int const LOG_STATUS		= 1 << STATUS_BIT;
int const ST				= LOG_STATUS;
//Exception Messages
int const LOG_EXCEPTION		= 1 << EXCEPTION_BIT;
int const EX				= LOG_EXCEPTION;
//Error Messages
int const LOG_ERROR			= 1 << ERROR_BIT;
int const ER				= LOG_ERROR;
//Warning Messages
int const LOG_WARNING		= 1 << WARNING_BIT;
int const WA				= LOG_WARNING;
//State Change Messages
int const LOG_STATECHANGE	= 1 << STATECHANGE_BIT;
int const SC				= LOG_STATECHANGE;
//Performance Messages
int const LOG_PERFORMANCE	= 1 << PERFORMANCE_BIT;
int const PE				= LOG_PERFORMANCE;
//Security Messages
int const LOG_SECURITY		= 1 << SECURITY_BIT;
int const SE				= LOG_SECURITY;

int const LOG_ALL = LOG_WARNING | LOG_ERROR | LOG_EXCEPTION |
                    LOG_STATECHANGE | LOG_STATUS |
                    LOG_PERFORMANCE | LOG_SECURITY;

const int kMaxLogMessageSize = 4096;

void ClientLog(const int component,
			   const int event,
			   const bool continuation,
			   const char* message,
			   ...);

void ClientLog(	const char *file,		                // Source File
								const int line,			// line in source
								const int component,
								const int event,
								const bool continuation,
								const char* message,
								...);


ErrorType ClientLogError(ErrorType err,
						 const int event,
						 const bool continuation,
						 const char* prefixString,
						 ...);

/////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Toggles logging for all components
//	Requires:
//		bOn: Turns on or off logging
//	Returns:  nothing
//
void EnableLogging(bool bOn);

/////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Changes the default events logged
//      Individual components may override this setting
//	Requires:
//		events: events is a bit-pattern for the various log events that should
//              be enabled e.g. LOG_STATUS | LOG_WARNING | LOG_ERROR
//	Returns:
//      true if successful, false  if an invalid bit-pattern is specified.
//      It is not legal for the events argument to be LOG_DEFAULT
//
bool SetDefaultLoggingEvents(int eventMask);


/////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Toggles the logging events for all componets enabled.
//	Requires:
//		events: events is a bit-pattern for the various log events that should
//              be enabled e.g. LOG_STATUS | LOG_WARNING | LOG_ERROR
//	Returns:
//      true if successful, false  if an invalid bit-pattern is specified.
//      It is not legal for the events argument to be LOG_DEFAULT
//
int GetDefaultLoggingEvents();

/////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Toggles the logging events for a given component.
//	Requires:
//      componentIndex: the index of the component to change
//              the special component ALL_COMPONENTS sets all the
//              components
//		events: events is a bit-pattern for the various logging events
//              to enable for this component.  The special event, LOG_DEFAULT,
//              enables logging for the default events. To disable logging
//              for this component, events should be zero.
//	Returns:
//      true if successful, false if an invalid component or invalid bit-pattern
//      is specified.
//  Examples:
//       SetComponentLogging(MEDIAMIXER_COMPONENT_INDEX, LOG_ERROR | LOG_WARNING);
//       SetComponentLogging(ALL_COMPONENTS, LOG_DEFAULT);  // turn on logging for all default events
//


#define ERROR_TOTAL_COMPONENTS          (DIOMEDE_LAST_COMPONENT>>24)-(DIOMEDE_FIRST_COMPONENT>>24)
#define ALWAYS_COMPONENT_INDEX          0xFF
#define ALL_COMPONENTS                  ALWAYS_COMPONENT_INDEX

    enum { NULL_COMP=-1,
           COMPONENT_LIST,
           ALWAYS_COMP = ALWAYS_COMPONENT_INDEX << 24 };

    bool SetComponentLogging(int nComponentIndex, int eventMask);


    /////////////////////////////////////////////////////////////////////////////
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
    int GetNormalizedIndex(unsigned int nComponentIndex);

    /////////////////////////////////////////////////////////////////////////////
    // Purpose:
    //      Returns the logging events set for a given component
    //	Requires:
    //		events: events is a bit-pattern for the various logging events
    //              to enable for this component.  The special event, LOG_DEFAULT,
    //              enables logging for the default events. To disable logging
    //              for this component, events should be zero.
    //	Returns:
    //      bit-pattern for the various logging events to enable for this component
    //
    int GetComponentLogging(int componentIndex);

//////////////////////////////////////////////////////////////////////////
class LogObserver : public ILogObserver {

public:
	virtual void LogMessageCallback(const std::string& pMessage, int len)=0;
	virtual bool IsIgnored(int nComponent, int nEvent) ;
	virtual void SetLogObserverType()=0;
};

bool RegisterLogObserver(ILogObserver* pLogObserver);
bool RegisterLogObserver(LogObserver* pLogObserver);
bool UnRegisterLogObserver(ILogObserver* pLogObserver);
bool UnRegisterLogObserver(LogObserver* pLogObserver);

#endif // __CLIENT_LOG_H__



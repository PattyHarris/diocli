/*********************************************************************
 * 
 *  file:  MessageTimer.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: This class provides message timed related output
 * for the console.  Typically used for lengthy service commands.
 * 
 *********************************************************************/

#ifndef __MESSAGE_TIMER_H__
#define __MESSAGE_TIMER_H__

#include "Stdafx.h"
#include "XString.h"
#include "StringUtil.h"
#include "Thread.h"

#include "../Util/ClientLog.h"
#include "../Include/ErrorCodes/UIErrors.h"

#include "UserProfileData.h"
#include "ProfileManager.h"

#include "boost/date_time/posix_time/posix_time.hpp"
#include <iostream>

using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace StringUtil;

// Must match with time_resolutions enum in date_time/time_defs.h
#ifdef _DEBUG
const char* const resolution_names[] =
    {"Second", "Deci", "Centi", "Milli", "Ten_Thousanth", "Micro", "Nano"};
#endif

static const int MAX_STATUS_LEN = 33;
static const int MAX_LINE_LEN   = 79;

///////////////////////////////////////////////////////////////////////
namespace EndTimerTypes {
    typedef enum { useSingleLine = 0,
                   useNewLine = 1,
                   useSingleLongLine = 2,
                   useNoTimeOrDone = 3,
                   useSessionExpired = 4,
                   useNetworkConnectionError = 5,
                   useGenericServiceError = 6
                 } EndTimerType;
}

using namespace EndTimerTypes;

///////////////////////////////////////////////////////////////////////
class MessageTimer
{
private:
    ptime           m_tmFirstStart;
    ptime           m_tmLastStart;
    time_duration   m_totalElapsedTime;
    LONG64          m_l64Adjust;                // Adjust for microseconds

    int             m_nWaitPeriod;
    bool            m_bShowProgress;
    bool            m_bConfigShowProgress;      // Pulled from the config file.
    bool            m_bConfigLogProgress;       // Pulled from the config file.
                                                // Combination of these flags allows
                                                // one to turn off console timing
                                                // but log these values to the log
                                                // files as well.
    bool            m_bTimerStarted;
    bool            m_bTimerPaused;             // Pause and unpause should occur as
                                                // pairs...

    std::string     m_szStartText;
    int             m_nTextLength;

	CMutexClass     m_mutex;

public:

    //-----------------------------------------------------------------
    // Default constructor
    //-----------------------------------------------------------------
    MessageTimer();

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    MessageTimer(int nWaitPeriod, bool bShowProgress=true);
	~MessageTimer() {};

    void operator=( const MessageTimer& msgTimer);

    //-----------------------------------------------------------------
	// Override progress with a setting from the config file.
    //-----------------------------------------------------------------
	bool SetConfigProgress();

private:
    //-----------------------------------------------------------------
    // Output an error if the timer hasn't been started (optionally
    // we could assert).
    //-----------------------------------------------------------------
    bool CheckStart();

    //-----------------------------------------------------------------
    // Output an error if the timer hasn't been paused, the there
    // is an attempt to resume..
    //-----------------------------------------------------------------
    bool CheckPaused();

    //-----------------------------------------------------------------
    // Helper to format the output text to ensure the previous line
    // doesn't extend beyond the new line.
    //-----------------------------------------------------------------
	void PrintText(const std::string& szTimerText, int nTruncatePos=0);

    //-----------------------------------------------------------------
    // Helper to output the "done" status text.
    //-----------------------------------------------------------------
    void OutputEndStatus(time_duration elapsedTime, std::string szOutputText,
                    EndTimerTypes::EndTimerType endTimerType = EndTimerTypes::useSingleLine,
                    bool bAfterNewLine=true);


public:

    //-----------------------------------------------------------------
	// Use to turn off or on progress text.
    //-----------------------------------------------------------------
	void SetShowProgress(const bool& bShowProgress);

    //-----------------------------------------------------------------
	// Set or reset the start text (or continuation
	// text).
    //-----------------------------------------------------------------
	void SetStartText(const std::string& szStartText);

    //-----------------------------------------------------------------
    // Is the timer started?
    //-----------------------------------------------------------------
    bool IsStarted();

    //-----------------------------------------------------------------
    // Is the timer paused?
    //-----------------------------------------------------------------
    bool IsPaused();

    //-----------------------------------------------------------------
    // Timer start
    //-----------------------------------------------------------------
    void Start(std::string szOutputText);

    //-----------------------------------------------------------------
    // Continue timing showing the output text (e.g. ....)
    //-----------------------------------------------------------------
    void Continue(std::string szOutputText);

    //-----------------------------------------------------------------
    // Continue timing showing the time duration
    //-----------------------------------------------------------------
    void ContinueTime(std::string szOutputText=_T(""));

    //-----------------------------------------------------------------
    // Simple end of the timing period, returning the elapsed time
    //-----------------------------------------------------------------
    time_duration End();

    //-----------------------------------------------------------------
    // End or pause the timing period with the given text appended to the
    // current line e.g.
    // Uploading file c:\aTestShare\test2.doc    24064 byte(s)...  0.312500   Done:  0.359
    //-----------------------------------------------------------------
    void End(std::string szOutputText, EndTimerTypes::EndTimerType endTimerType = EndTimerTypes::useSingleLine);

    time_duration EndTime(std::string szOutputText,
                          EndTimerTypes::EndTimerType endTimerType = EndTimerTypes::useSingleLine,
                          bool bAfterNewLine = true);

    time_duration PauseTime(std::string szOutputText,
                            EndTimerTypes::EndTimerType endTimerType = EndTimerTypes::useSingleLine,
                            bool bAfterNewLine = true);

    //-----------------------------------------------------------------
    // UnPauseTime timing showing the time duration - this should
    // only occur following a PauseTime.
    //-----------------------------------------------------------------
    void UnPauseTime();

};

#endif // __MESSAGE_TIMER_H__
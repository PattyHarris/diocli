/*********************************************************************
 * 
 *  file:  MessageTimer.cpp
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: This class provides message timed related output
 *          for the console.  Typically used for lengthy service 
 *          commands.
 * 
 *********************************************************************/

//! \defgroup message_timer MessageTimer Methods
//! @{
#include "MessageTimer.h"

///////////////////////////////////////////////////////////////////////
//! \brief Constructor  - default constructor.
//
MessageTimer::MessageTimer() : m_tmFirstStart(not_a_date_time), m_tmLastStart(not_a_date_time),
                               m_totalElapsedTime(0,0,0), m_l64Adjust(0), m_nWaitPeriod(50),
                               m_bShowProgress(true), m_bTimerStarted(false), m_bTimerPaused(false),
                               m_szStartText(_T("")), m_nTextLength(0)
{
    SetConfigProgress();

} // End constructor

///////////////////////////////////////////////////////////////////////
//! \brief Constructor  - constructor from a given wait interval,
//!                       a progress option.
//
MessageTimer::MessageTimer(int nWaitPeriod, bool bShowProgress /*true*/)
{
    m_tmFirstStart = not_a_date_time;
    m_tmLastStart = not_a_date_time;
    m_totalElapsedTime = time_duration(0,0,0);
    m_l64Adjust = 0;

    m_nWaitPeriod = nWaitPeriod;
    m_bShowProgress = bShowProgress;
    m_bTimerStarted = false;
    m_bTimerPaused = false;
    m_szStartText = _T("");
    m_nTextLength = 0;

    SetConfigProgress();

} // End constructor

///////////////////////////////////////////////////////////////////////
//! \brief MessageTimer assignment operator
//! \param msgTimer: source object
void MessageTimer::operator=( const MessageTimer& msgTimer)
{
    m_tmFirstStart = msgTimer.m_tmFirstStart;
    m_tmLastStart = msgTimer.m_tmLastStart;
    m_totalElapsedTime = msgTimer.m_totalElapsedTime;
    m_l64Adjust = msgTimer.m_l64Adjust;

    m_nWaitPeriod = msgTimer.m_nWaitPeriod;
    m_bShowProgress = msgTimer.m_bShowProgress;
    m_bConfigShowProgress = msgTimer.m_bConfigShowProgress;
    m_bConfigLogProgress = msgTimer.m_bConfigLogProgress;

    m_bTimerStarted = msgTimer.m_bTimerStarted;
    m_bTimerPaused = msgTimer.m_bTimerPaused;

    m_szStartText = msgTimer.m_szStartText;
    m_nTextLength = msgTimer.m_nTextLength;

} // End assignment operator

///////////////////////////////////////////////////////////////////////
// Private Methods

///////////////////////////////////////////////////////////////////////
//! \brief Override progress with a setting from the config file.
bool MessageTimer::SetConfigProgress()
{
    // For testing purposes, allow timing to be turned off or on using
    // the configuration data - this has precedence over any settings
    // set in the code.
    m_bConfigShowProgress = true;
    m_bConfigLogProgress = false;

    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), false );

    if (pProfileData == NULL) {
        return false;
    }

    m_bConfigShowProgress =
        (pProfileData->GetUserProfileInt(GEN_SHOW_TIMING, GEN_SHOW_TIMING_DF) != 0);
    m_bConfigLogProgress =
        (pProfileData->GetUserProfileInt(GEN_LOG_TIMING, GEN_LOG_TIMING_DF) != 0);

    // We only need to turn off our normal progress if it's warrented by the
    // config setting.
    if (!m_bConfigShowProgress) {
        m_bShowProgress = m_bConfigShowProgress;
    }

    return true;

} // End SetConfigProgress

///////////////////////////////////////////////////////////////////////
//! \brief Output an error if the timer hasn't been started (optionally
//!        we could assert).
bool MessageTimer::CheckStart()
{
    if (m_bTimerStarted == true) {
        return true;
    }
    return false;

} // End CheckStart

///////////////////////////////////////////////////////////////////////
//! \brief Output an error if the timer hasn't been paused, the there
//!        is an attempt to resume..
bool MessageTimer::CheckPaused()
{
    if (m_bTimerPaused == true) {
        return true;
    }
    return false;

} // End CheckPaused

///////////////////////////////////////////////////////////////////////
//! \brief Helper function to print the timer status text.  Ensures
//!        that the previous text is "erased" by padding the string
//!        as needed.
//! \param szTimerText: text to output
//! \param nTruncatePos: where the text can be truncated if it exceed o
//!                      max line length.
void MessageTimer::PrintText(const std::string& szTimerText, int nTruncatePos /*0*/)
{
    std::string szPad = _T("");

    std::string szTempTimerText = szTimerText;
    int nNewLength = (int)szTimerText.length();

    if ( (nTruncatePos > 0) && (nNewLength > MAX_LINE_LEN + 1) ) {
        // Truncate the line left of the truncate position - which assumes
        // we want to keep the right most portion of the string.
        int nDiffLength = nTruncatePos - (nNewLength - (MAX_LINE_LEN + 1));
        szTempTimerText = szTimerText.substr(0, nDiffLength) + szTimerText.substr(nTruncatePos);

    }
    else if (nNewLength < m_nTextLength) {
        szPad = GetPadStr(m_nTextLength - nNewLength );
    }
    _tprintf(_T("%s%s"), szTempTimerText.c_str(), szPad.c_str());
    m_nTextLength = nNewLength;

    if (m_bConfigLogProgress) {
        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s%s"), szTempTimerText.c_str(), szPad.c_str());
    }

} // End PrintText

///////////////////////////////////////////////////////////////////////
//! \brief Helper function to output the "end" text using the
//!        elapsed time and the type of "done" format.
//!        that the previous text is "erased" by padding the string
//!        as needed.
//! \param elapsedTime: Elapsed time
//! \param szOutputText: final output text
//! \param endTimerType: directs the format of the "done" message.
//! \param bAfterNewLine: true to add a newline at the end, false otherwise.
//!
void MessageTimer::OutputEndStatus(time_duration elapsedTime,
                                   std::string szOutputText,
                                   EndTimerTypes::EndTimerType endTimerType /*useSingleLine*/,
                                   bool bAfterNewLine /*true*/)
{
    if (m_bShowProgress == false) {
        return;
    }

    std::string szDuration = _T("");
    std::string szDurationType = _T("");
    std::string szTimerText = _T("");

    FormatDuration(elapsedTime, szDuration, szDurationType, 3);

    switch (endTimerType) {
        case useSingleLine:
            szTimerText = _format(_T("\r%s...Done: %s %s" ), m_szStartText.c_str(),
                szDuration.c_str(), szDurationType.c_str());
            break;
        case useNewLine:
            szTimerText = _format(_T("\r%s...\n\r   Done: %s %s" ), m_szStartText.c_str(),
                szDuration.c_str(), szDurationType.c_str());
            break;
        case useSingleLongLine:
            {
                // Format the line to fill the 79 columns, formatting so the "done"
                // portion lines ups.  10 spaces are needed for the line
                // spacing and "...Done:"
                std::string szPad = GetPadStr(MAX_LINE_LEN - (int)m_szStartText.length() -
                    (int)szDuration.length() - (int)szDurationType.length() - 10);
                szTimerText = _format(_T("\r%s%s...Done: %s %s" ), m_szStartText.c_str(),
                    szPad.c_str(), szDuration.c_str(), szDurationType.c_str());
            }
            break;
        case useNoTimeOrDone:
            szTimerText = _format(_T("\r%s... " ), m_szStartText.c_str());
            break;
        case useSessionExpired:
            szTimerText = _format(_T("\r%s...session expired." ), m_szStartText.c_str());
            break;
        case useNetworkConnectionError:
            szTimerText = _format(_T("\r%s...connection dropped." ), m_szStartText.c_str());
            break;
        case useGenericServiceError:
            szTimerText = _format(_T("\r%s...an unknown error has occurred." ), m_szStartText.c_str());
            break;
        default:
            szTimerText = _format(_T("\r%s...Done: %s %s" ), m_szStartText.c_str(),
                szDuration.c_str(), szDurationType.c_str());
            break;
    }

    int nTruncatePos = (int)szTimerText.find(_T("..."));
    PrintText(szTimerText, nTruncatePos);

    if (bAfterNewLine) {
        _tprintf(_T("\n\r"));
        m_nTextLength = 0;
    }

    if ( szOutputText.length() > 0 ) {
        szTimerText = _format(_T("   %s\n\r"), szOutputText.c_str());
        PrintText(szTimerText);
    }

} // End OutputEndStatus

///////////////////////////////////////////////////////////////////////
// Public Methods

///////////////////////////////////////////////////////////////////////
//! \brief Use to turn off or on progress text.
//! \param bShowProgress: true to turn on progress, false otherwise
void MessageTimer::SetShowProgress(const bool& bShowProgress)
{
    // We only need to turn off our normal progress if it's warrented by the
    // config setting.
    if (m_bConfigShowProgress) {
        m_bShowProgress = bShowProgress;
    }

} // End SetShowProgress

///////////////////////////////////////////////////////////////////////
//! \brief Set or reset the start text (or continuation
//!        text).
//! \param szStartText: new starting text.
void MessageTimer::SetStartText(const std::string& szStartText)
{
    m_szStartText = szStartText;

} // End SetStartText

///////////////////////////////////////////////////////////////////////
//! \brief Is the timer started?
//! \return Returns true if the timer is start, false otherwise.
bool MessageTimer::IsStarted()
{
    return m_bTimerStarted;

} // End IsStarted

///////////////////////////////////////////////////////////////////////
//! \brief Is the timer paused?
//! \return Returns true if the timer is paused, false otherwise.
bool MessageTimer::IsPaused()
{
    return m_bTimerPaused;

} // End IsPaused

///////////////////////////////////////////////////////////////////////
//! \brief Timer start
//! \param szOutputText: timer output text.
void MessageTimer::Start(std::string szOutputText)
{
    #ifdef _DEBUG
    #if 0
    std::cout << "Resolution: "
              << resolution_names[time_duration::rep_type::resolution()]
              << " -- Ticks per second: "
              << time_duration::rep_type::res_adjust() << std::endl;
    #endif
    #endif

    // Key to getting fractional precision to time is to use the microsecond
    // clock.  Otheriwse, fractional seconds is always 0.
    m_bTimerStarted = true;
    m_szStartText = szOutputText;

    m_l64Adjust = time_duration::rep_type::res_adjust();

    if (m_bShowProgress && m_szStartText.length() > 0) {
        std::string szTimerText = _format(_T("\r%s..."), m_szStartText.c_str());

        int nTruncatePos = (int)szTimerText.find(_T("..."));
        PrintText(szTimerText, nTruncatePos);
    }

    m_tmFirstStart = microsec_clock::local_time();
    m_tmLastStart = m_tmFirstStart;
    m_totalElapsedTime = m_tmLastStart - m_tmFirstStart;

} // End Start

///////////////////////////////////////////////////////////////////////
//! \brief Continue timing showing the output text (e.g. ....)
//! \param szOutputText: timer output text.
void MessageTimer::Continue(std::string szOutputText)
{
    if (CheckStart() == false) {
        return;
    }

    m_mutex.Lock();

    ptime tmNow = microsec_clock::local_time();
    time_duration elapsedTime = tmNow - m_tmLastStart;

    if (elapsedTime.total_milliseconds() >= m_nWaitPeriod) {

        if (m_bShowProgress) {
            int nTruncatePos = (int)szOutputText.find(_T("..."));
            if (nTruncatePos == -1) {
                nTruncatePos = MAX_LINE_LEN - 1;
            }
            PrintText(szOutputText, nTruncatePos);
        }

        m_tmLastStart = tmNow;
    }

    m_mutex.Unlock();

} // End Continue

///////////////////////////////////////////////////////////////////////
//! \brief Continue timing showing the time duration
//! \param szOutputText: optional timer output text.
void MessageTimer::ContinueTime(std::string szOutputText /*_T("")*/)
{
    if (CheckStart() == false) {
        return;
    }

    // Unpause the time (e.g. move up the first and last timestamps
    // to where we are now...
    if (CheckPaused() == true) {
        UnPauseTime();
    }

    m_mutex.Lock();

    if (szOutputText.length() > 0) {
        m_szStartText = szOutputText;
    }

    ptime tmNow = microsec_clock::local_time();
    time_duration elapsedTime = tmNow - m_tmLastStart;

    time_duration totalElapsedTime = microsec_clock::local_time() - m_tmFirstStart + m_totalElapsedTime;

    // Uncomment for debugging...
    // double dblMicroseconds = static_cast<double>(totalElapsedTime.fractional_seconds());
    // double dblMilliseconds = static_cast<double>(dblMicroseconds / m_l64Adjust);
    // double dblSeconds =  static_cast<double>(totalElapsedTime.seconds()) + dblMilliseconds;

    std::string szDuration = _T("");
    std::string szDurationType = _T("");

    FormatDuration(totalElapsedTime, szDuration, szDurationType);

    if (elapsedTime.total_milliseconds() >= m_nWaitPeriod) {

        if (m_bShowProgress) {
            std::string szTimerText = _format(_T("\r%s... %s %s"), m_szStartText.c_str(),
                szDuration.c_str(), szDurationType.c_str());
            int nTruncatePos = (int)szTimerText.find(_T("..."));
            PrintText(szTimerText, nTruncatePos);
        }

        m_tmLastStart = tmNow;
    }

    m_mutex.Unlock();

} // End ContinueTime

///////////////////////////////////////////////////////////////////////
//! \brief End the timing period, returning the elapsed time
//! \return Returns the total elapsed time.
time_duration MessageTimer::End()
{
    if (CheckStart() == false) {
        return not_a_date_time;
    }

    time_duration elapsedTime = microsec_clock::local_time() - m_tmFirstStart + m_totalElapsedTime;
    return elapsedTime;

} // End End

///////////////////////////////////////////////////////////////////////
//! \brief End the timing period with the given text appended to the
//!        current line e.g.
//!        Uploading file c:\aTestShare\test2.doc    24064 byte(s)...  0.312500   Done:  0.359
//! \param szOutputText: final output text
//! \param endTimerType: directs the format of the "done" message.
void MessageTimer::End(std::string szOutputText, EndTimerType endTimerType /*useSingleLine*/)
{
    if (CheckStart() == false) {
        return;
    }

    m_mutex.Lock();

    time_duration elapsedTime = microsec_clock::local_time() - m_tmFirstStart + m_totalElapsedTime;

    std::string szDuration = _T("");
    std::string szDurationType = _T("");
    std::string szTimerText = _T("");

    if (m_bShowProgress) {

	    FormatDuration(elapsedTime, szDuration, szDurationType, 3);

        if (endTimerType == useNewLine) {
            _tprintf(_T("\n\r"));
            szTimerText = _format(_T("   Done: %s %s\n\r" ), szDuration.c_str(),
                                                             szDurationType.c_str());
        }
        else if (endTimerType == useSessionExpired) {
            szTimerText = _format(_T("\r%s...session expired.\n\r" ), m_szStartText.c_str());
        }
        else if (endTimerType == useNetworkConnectionError) {
            szTimerText = _format(_T("\r%s...connection dropped.\n\r" ), m_szStartText.c_str());
        }
        else if (endTimerType == useGenericServiceError) {
            szTimerText = _format(_T("\r%s...an unknown error has occurred.\n\r" ), m_szStartText.c_str());
        }
        else {
            szTimerText = _format(_T("   Done: %s %s\n\r" ), szDuration.c_str(), szDurationType.c_str());
        }

        int nTruncatePos = (int)szTimerText.find(_T("..."));
        PrintText(szTimerText, nTruncatePos);

        if ( szOutputText.length() > 0 ) {
            szTimerText = _format(_T("   %s\n\r"), szOutputText.c_str());
            PrintText(szTimerText);
        }
    }

    m_bTimerStarted = false;
    m_tmFirstStart = not_a_date_time;
    m_tmLastStart = not_a_date_time;

    m_mutex.Unlock();

    #if 0
    // One mechanism for working with the Boost api's using streams
    time_duration elapsedTime = microsec_clock::local_time() - m_tmFirstStart;
    std::stringstream streamTime;

    time_facet* pTimeFacet = new time_facet();

    /*
    streamTime.imbue(std::locale(std::locale::classic(), pTimeFacet));
    */

    streamTime.imbue(std::locale(streamTime.getloc(), pTimeFacet));

    pTimeFacet->time_duration_format(_T("%s"));

    streamTime << elapsedTime;

    std::string szFormattedTime = streamTime.str();
    _tprintf(_T("   Done: %s\n\r" ), szFormattedTime.c_str());
    #endif

} // End End

///////////////////////////////////////////////////////////////////////
//! \brief End the timing period with the given text appended to the
//!        current line e.g.
//!        Uploading file c:\aTestShare\test2.doc    24064 byte(s)...  0.312500   Done:  0.359
//! \param szOutputText: final output text
//! \param endTimerType: directs the format of the "done" message.
//! \param bAfterNewLine: true to add a newline at the end, false otherwise.
//!
//! \return Returns the total elapsed time.
time_duration MessageTimer::EndTime(std::string szOutputText,
                                    EndTimerTypes::EndTimerType endTimerType /*EndTimerTypes::useSingleLine*/,
                                    bool bAfterNewLine /*true*/)
{
    if (CheckStart() == false) {
        return not_a_date_time;
    }

    m_mutex.Lock();

    time_duration elapsedTime = microsec_clock::local_time() - m_tmFirstStart + m_totalElapsedTime;

    OutputEndStatus(elapsedTime, szOutputText, endTimerType, bAfterNewLine);

    m_bTimerStarted = false;
    m_tmFirstStart = not_a_date_time;
    m_tmLastStart = not_a_date_time;

    m_mutex.Unlock();
    return elapsedTime;

} // End EndTime

///////////////////////////////////////////////////////////////////////
//! \brief Pause the timing period with the given text appended to the
//!        current line e.g.
//!        Uploading file c:\aTestShare\test2.doc    24064 byte(s)...  0.312500   Done:  0.359
//! \param szOutputText: final output text
//! \param endTimerType: directs the format of the "done" message.
//! \param bAfterNewLine: true to add a newline at the end, false otherwise.
//!
//! \return Returns the total elapsed time.
time_duration MessageTimer::PauseTime(std::string szOutputText,
                        EndTimerTypes::EndTimerType endTimerType /*EndTimerTypes::useSingleLine*/,
                        bool bAfterNewLine /*true*/)
{
    if (CheckStart() == false) {
        return not_a_date_time;
    }

    m_mutex.Lock();

    m_bTimerPaused = true;
    m_bTimerStarted = true;

    ptime tmNow = microsec_clock::local_time();
    m_totalElapsedTime += tmNow - m_tmFirstStart;

    m_tmLastStart = tmNow;

    OutputEndStatus(m_totalElapsedTime, szOutputText, endTimerType, bAfterNewLine);

    m_mutex.Unlock();
    return m_totalElapsedTime;

} // End PauseTime

///////////////////////////////////////////////////////////////////////
//! \brief UnPauseTime timing showing the time duration - this should
//!        only occur following a PauseTime.
void MessageTimer::UnPauseTime()
{
    if (CheckStart() == false) {
        return;
    }

    if (CheckPaused() == false) {
        return;
    }

    m_mutex.Lock();

    // The difference between UnPause and Continue is that we need to continue
    // the timer as though there was no pause - that is, if we Pause at a 1 minute
    // and resume 2 mintues later, time begins again at 1 minutes, not 3 minutes.
    m_tmFirstStart = microsec_clock::local_time();
    m_tmLastStart = m_tmFirstStart;
    m_bTimerPaused = false;

    m_mutex.Unlock();

} // End UnPauseTime

/** @} */

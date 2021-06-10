/*********************************************************************
 * 
 *  file:  ClientLogUtils.h
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

#ifndef __CLIENT_LOG_UTILS_H__
#define __CLIENT_LOG_UTILS_H__

#include "Stdafx.h"
#include "ErrorType.h"
#include "configure.h"
#include <string>

using namespace std;

/////////////////////////////////////////////////////////////////////////////

    // Print the current stack trace to the log
    void LogStackframe(int component, int event);

    // if set to true, try to catch any runtime exceptions and log them
    // with a stack trace
    void LogExceptions(bool enable);

    // Print out information about the current build to the
    // log file
    void LogBuildVersion();

    // Read information from the registry to set the current log settings
    void GetRegistryLoggingSettings();


    // Setup logging to go to the default platform destination
    // Read logging information from the registry
    // and call LogBuildVersion
    // Note - if InitializeLogging is not called, the application
    // must add a LogObserver and individually enable/disable settings
    void InitializeLogging(std::string pathToSaveData=_T(""), bool logToStdOut = false, 
        bool memLogging = true, const std::string& szProfile = _T("Diomede"), 
        UINT numberOfLogFilesToRotate=2, UINT sizeOfEachLogFile=10000000);

    bool SaveActiveLogging(const std::string& szFullFileName, const std::string& szFileHeader = 0);
    std::string GetActiveLoggingString();

    #ifdef _WIN32
    // Turn off/on ClientLog output to OutputDebugString
    // Note: OutputDebugString has higher overhead than
    // file based logging
    void EnableDebuggerLogging(bool useDebugger);

    #endif

    #ifndef _WIN32
    void EnableFileLogging( const std::string &sPath, int nMaxFileSize, int nNumFiles);
    #endif

    // This is a convenience routine that will post a log report to the
    // Log Server synchronously (it may block for several seconds)
    ErrorType PublishLogReportToServer(std::string logstring=_T(""));

#endif // __CLIENT_LOG_UTILS_H__



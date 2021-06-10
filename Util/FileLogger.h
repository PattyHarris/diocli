/*********************************************************************
 * 
 *  file:  FileLogger.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Manages client logging to a file.
 * 
 *********************************************************************/

#ifndef __FILE_LOGGER_H__
#define __FILE_LOGGER_H__

/*
	Pulled out code dealing with rotating log files from ClientLog

	ClientLog has a generic AddLogObserver interface, which the new
	FileLogger uses
*/

#include "ClientLog.h"

class FileLogger : public LogObserver {
public:
	FileLogger();
	virtual ~FileLogger();

	// Start up the file logging service, in the specified
	// directory, rotating through the specified number of files
	// Choose the active log file based on the last touched log file
	// or the first log file
	// the return value indicates whether the function succeeded (true)
	// or failed (false)
	// if directory is null, use the current directory
	// if maxLogFileSize is 0, then don't limit log file size
	// (effectively turning off log rotation)
	bool Init( int files, const char* directory, int maxLogFileSize );

	// log the message to the currently active log file
	// log files are rotated once the max file size is reached
	virtual void LogMessageCallback(const string& szMessage, int len);

    // Used for comparison within the list of log observers.
	virtual void SetLogObserverType() {
	    m_nLogObserverType = FILE_LOGGER_TYPE;
	}

    void Flush() { if (m_logFile) fflush(m_logFile); }

private:
	FILE*           m_logFile;
	int             m_iFiles;
	char**          m_logFilenames;
	int             m_iMaxLogFileSize;
	int             m_iCurFile;
};

#endif // __FILE_LOGGER_H__


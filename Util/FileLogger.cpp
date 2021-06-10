/*********************************************************************
 * 
 *  file:  FileLogger.cpp
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

#include "Stdafx.h"
#include "FileLogger.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifdef WIN32
#include "direct.h"
#define DIR_SEPARATOR '\\'
#define stat _stat
#define getcwd _getcwd

#else
#define DIR_SEPARATOR '/'
#endif

/////////////////////////////////////////////////////////////////////////////
FileLogger::FileLogger()
: m_logFile(0),m_iFiles(0),m_logFilenames(0),m_iMaxLogFileSize(0),m_iCurFile(0)
{
}

/////////////////////////////////////////////////////////////////////////////
bool FileLogger::Init( int nNumberOfLogFiles, const char* szDirectory, int nMaxLogFileSize )
{
    m_iFiles = nNumberOfLogFiles;
	m_logFilenames = new char*[m_iFiles];
	
    int nIndex;
	for (nIndex=0; nIndex  <m_iFiles; nIndex++) {
		m_logFilenames[nIndex] = new char[MAX_PATH];
	}

	char szBuffer[MAX_PATH];
	if (szDirectory == 0) {
		getcwd(szBuffer, MAX_PATH);
	}

	const char* szTheDirectory = ( szDirectory ? szDirectory : szBuffer);

	m_iMaxLogFileSize = nMaxLogFileSize;

	struct stat theStats;
	struct stat lastModified;
	bool bFirstFile = true;

    for (nIndex = 0; nIndex < m_iFiles; nIndex++) {
        sprintf(m_logFilenames[nIndex], _T("%s%cLog%d.txt"),
                 szTheDirectory,
                 DIR_SEPARATOR,
                 nIndex+1);

		int nError = stat( m_logFilenames[nIndex], &theStats);

		if (nError != 0) {
			// failed - perhaps doesn't exist
		} 
		else if (bFirstFile) {
			bFirstFile = false;
			lastModified = theStats;
			m_iCurFile = nIndex;
		} 
		else {
            double dblDiff = difftime(theStats.st_mtime, lastModified.st_mtime);
			if ( dblDiff > 0 ||
               ( dblDiff == 0 && !((m_iCurFile == 0) && (nIndex == m_iFiles - 1) ))) {
                // If the times are equal, assume nIndex+1 file is more recent, unless
                // we comparing the first and last files (which wrap around)
				lastModified = theStats;
				m_iCurFile = nIndex;
			}
		}
	}
	
	bool bTruncate = false;
	if (!bFirstFile) {
		// we found at least one log file
		if (m_iMaxLogFileSize && (lastModified.st_size > m_iMaxLogFileSize)) {
			m_iCurFile = (m_iCurFile + 1) % m_iFiles;
			bTruncate = true;
		}
	}

	if (bTruncate) {
	    m_logFile = _tfopen( m_logFilenames[m_iCurFile], _T("wt"));
	}
	else {
    	m_logFile = _tfopen( m_logFilenames[m_iCurFile], _T("at"));
	}


	return (m_logFile != 0);
}

/////////////////////////////////////////////////////////////////////////////
FileLogger::~FileLogger()
{
	if (m_logFile) {
		fclose(m_logFile);
		m_logFile = 0;
	}

	if (m_logFilenames) {
		for (int i=0;i<m_iFiles;i++)
			delete[] m_logFilenames[i];
		delete[] m_logFilenames;
	}
}

/////////////////////////////////////////////////////////////////////////////
void FileLogger::LogMessageCallback(const string& szMessage, int len)
{
	if (m_logFile) {

		fputs( (char*)szMessage.c_str(), m_logFile );
		fflush( m_logFile );

		// Now check to see if the file exceeds the maximum file size
		// first lock the logging mutex so only one thread can write to
		// file at a time
		if (m_iMaxLogFileSize && ftell(m_logFile)>m_iMaxLogFileSize) {

			// Time to switch to the next log file
			fclose(m_logFile);
            m_iCurFile = (m_iCurFile + 1) % m_iFiles;

            if ((m_logFile = _tfopen(m_logFilenames[m_iCurFile], _T("wt"))) == NULL) {
		        // Error opening log file
#ifdef WIN32
				OutputDebugString(_T("Error opening rollover log file.\n"));
#endif
            }
        }

	}
}




/*********************************************************************
 * 
 *  file:  ClientLogUtils.cpp
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
#include "ClientLog.h"
#include "ClientLogUtils.h"
#include "../Include/BuildVersionUtils.h"
#include "ProfileManager.h"
#include "FileLogger.h"
#include "MemLogger.h"
#include "configure.h"
#include "types.h"

static CMemLogger theMemLogger;
static FileLogger theFileLogger;

/*
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain( HANDLE hModule, DWORD dwReason, LPVOID lpReserved )
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
*/

/////////////////////////////////////////////////////////////////////////////
void LogBuildVersion()
{
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T("=============================================================================="));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	string str;

    BuildUtil buildUtil;

	if( buildUtil.GetClientPlatform(str))
	{
		ClientLog(ALWAYS_COMP,ST,true,_T("Platform: %s"), (char*)str.c_str());
	}
	else
	{
		ClientLog(ALWAYS_COMP,ST,true,_T("Platform: Error getting platform type.\n"));
	}

	if( buildUtil.GetClientVersion(str))
	{
		ClientLog(ALWAYS_COMP,ST,true,_T("Version: %s"), str.c_str());
	}
	else
	{
		ClientLog(ALWAYS_COMP,ST,true, _T("Version: Error getting version number.\n"));
	}

	if( buildUtil.GetClientBuildNumber(str))
	{
		ClientLog(ALWAYS_COMP,ST, true, _T("Build #: %s"), str.c_str());
	}
	else
	{
		ClientLog(ALWAYS_COMP,ST,true,_T("Build: Error getting build number.\n"));
	}

	if( buildUtil.GetClientBuildDate(str))
	{
		ClientLog(ALWAYS_COMP,ST,true,_T("Build Date: %s"),str.c_str());
	}
	else
	{
		ClientLog(ALWAYS_COMP,ST,true,_T("Build Date: Error getting build date.\n"));
	}

	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T("=============================================================================="));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
	ClientLog(ALWAYS_COMP,ST,true,_T(" "));
}

/////////////////////////////////////////////////////////////////////////////
void GetRegistryLoggingSettings()
{
	// Registry Group = Configure
    UserProfileData *pConfigure =
        ProfileManager::Instance()->GetProfile(_T("Logging"), _T(""), _T(""));

	if (! pConfigure )
		return;

	// If no value, returns default = 1
    EnableLogging( pConfigure->GetUserProfileInt(_T("EnableLogging"), 0)!=0 );

    for (unsigned int i=0;i< (unsigned int)ERROR_TOTAL_COMPONENTS;i++) {
        string componentName = GetErrorComponentName(i);

        if (componentName.length() > 0) {
            int setting =  pConfigure->GetUserProfileInt((char*)componentName.c_str(), 0);
            SetComponentLogging( i, setting);
        }
    }

    int defaultEvents = 0;
    defaultEvents |= (pConfigure->GetUserProfileInt(_T("Exception"), 0)!=0? LOG_EXCEPTION : 0);
    defaultEvents |= (pConfigure->GetUserProfileInt(_T("Warning"), 0)!=0? LOG_WARNING : 0);
    defaultEvents |= (pConfigure->GetUserProfileInt(_T("Error"), 0)!=0? LOG_ERROR : 0);
    defaultEvents |= (pConfigure->GetUserProfileInt(_T("Security"), 0)!=0? LOG_SECURITY: 0);
    defaultEvents |= (pConfigure->GetUserProfileInt(_T("Status"), 0)!=0? LOG_STATUS : 0);
    defaultEvents |= (pConfigure->GetUserProfileInt(_T("StateChange"), 0)!=0? LOG_STATECHANGE : 0);
    defaultEvents |= (pConfigure->GetUserProfileInt(_T("Performance"), 0)!=0? LOG_PERFORMANCE : 0);

    SetDefaultLoggingEvents( defaultEvents );

	#ifdef _WIN32
	#ifndef _DEBUG
	    int useDebuggerDefault = 0;
	#else
	    int useDebuggerDefault = 1;
	#endif

	    int useDebugger = pConfigure->GetUserProfileInt(_T("DebuggerOutput"), useDebuggerDefault);
	    EnableDebuggerLogging( useDebugger ? true : false );
	#endif
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
class DebuggerLogger : public LogObserver {
public:
    virtual void LogMessageCallback(const string& message, int len) {
        OutputDebugString((char*)message.c_str());
    }
    // Used for comparison within the list of log observers.
	virtual void SetLogObserverType() {
	    m_nLogObserverType = DEBUGGER_LOGGER_TYPE;
	}

};

static DebuggerLogger theDebuggerLogger;
static bool gDebuggerLogging = false;

/////////////////////////////////////////////////////////////////////////////
LONG WINAPI MyExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
    theFileLogger.LogMessageCallback(_T("Caught exception") ,16);
    theFileLogger.Flush();
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

/////////////////////////////////////////////////////////////////////////////
class StdOutLogger : public LogObserver {
public:
    virtual void LogMessageCallback(const string& message, int len) {
        _tprintf(_T("%s"), message.c_str());
    }

    // Used for comparison within the list of log observers.
	virtual void SetLogObserverType() {
	    m_nLogObserverType = STD_OUTPUT_LOGGER_TYPE;
	}
};

static StdOutLogger theStdOutLogger;

/////////////////////////////////////////////////////////////////////////////
void InitializeLogging( string pathToSaveData, bool logToStdOut, bool memLogging,
                        const string& szProfile, UINT numberOfLogFiles,
                        UINT sizeEachLogFile )
{

#ifdef WIN32
    UserProfileData profile;

    string szDirectory;

    /* Some reason this doesn't work here - used to...using the above instead..
    */
	if ( pathToSaveData.size()>0 ) {
		szDirectory = pathToSaveData;
	} else {
		char szAppPath[MAX_PATH];
		if (profile.GetUserProfileRegistryStr( _T("UserApplicationPath"), szAppPath )) {
			szDirectory = szAppPath;
		}
	}
    /**/

    int maxLogFileSize;

    profile.GetUserProfileRegistryInt( _T("MaxLogFileSize"), maxLogFileSize, 1000000);

	//--------------------------------------------------------------------
	// const int NUMBER_OF_LOGFILE_TO_ROTATE = 2;
	// const int SIZE_OF_EACH_LOGFILE = 200000;
	//--------------------------------------------------------------------

    bool bSuccess = theFileLogger.Init( numberOfLogFiles, (char*)szDirectory.c_str(),
        sizeEachLogFile );
    if (bSuccess) {
        RegisterLogObserver( &theFileLogger );
	    theFileLogger.SetLogObserverType();
	}
#endif

	if ( memLogging)
	{
		theMemLogger.Init( 120000 );
		RegisterLogObserver( &theMemLogger );
	    theMemLogger.SetLogObserverType();
	}

    if (logToStdOut) {
        RegisterLogObserver( &theStdOutLogger );
	    theStdOutLogger.SetLogObserverType();
	}

    GetRegistryLoggingSettings();
    LogBuildVersion();
}

/////////////////////////////////////////////////////////////////////////////
bool SaveActiveLogging(const string& szFullFileName, const string& szFileHeader)
{
	bool bRetVal = false;
	FILE *logFile = fopen(szFullFileName.c_str(), _T("w"));
	if ( NULL == logFile)
		return false;

	ULONG nWritten = 0;

#ifndef _UNICODE
	if ( (char*)szFileHeader.c_str() && szFileHeader[0] != _T('\0'))
	{
		nWritten = (int)fwrite( szFileHeader.c_str(), sizeof(char), szFileHeader.size(), logFile);
		if (nWritten != (int)szFileHeader.size()) {
	    	fclose( logFile);
	    	return false;
		}
	}

#else
	char szTmpFileHeader[MAX_PATH];
	Configure::UnicodeToAscii((char*)szFileHeader.c_str(), szTmpFileHeader, szFileHeader.length());

	if ( (char*)szFileHeader.c_str() && szFileHeader[0] != _T('\0'))
	{
		fwrite( szTmpFileHeader, sizeof(char), strlen(szTmpFileHeader), logFile);
	}
#endif

	string sData = theMemLogger.GetCacheString();
	nWritten = (ULONG)fwrite( sData.data(), sizeof(char), sData.size(), logFile);

	if ( nWritten == (int)sData.size()) {
		bRetVal = true;
    }

	fclose( logFile);

    return bRetVal;
}

/////////////////////////////////////////////////////////////////////////////
string GetActiveLoggingString()
{
	return theMemLogger.GetCacheString();
}

#ifdef _WIN32
/////////////////////////////////////////////////////////////////////////////
// Turn off/on ClientLog output to OutputDebugString
// Note: OutputDebugString has higher overhead than
// file based logging
void EnableDebuggerLogging(bool useDebugger)
{
    if (useDebugger != gDebuggerLogging) {
        if (useDebugger) {
            RegisterLogObserver( &theDebuggerLogger );
            theDebuggerLogger.SetLogObserverType();
        } else {
            UnRegisterLogObserver( &theDebuggerLogger );
        }
        gDebuggerLogging = useDebugger;
    }
}

#endif

#ifndef _WIN32
void EnableFileLogging( const std::string &sPath, int nMaxFileSize, int nNumFiles)
{
    theFileLogger.Init( nNumFiles, sPath.c_str(), nMaxFileSize );
    RegisterLogObserver( &theFileLogger );
}
#endif

/////////////////////////////////////////////////////////////////////////////
// This is a convenience routine that will post a log report to a
// Log Server.  The functions spawns a background thread and returns
// right away.
ErrorType PublishLogReportToServer(string logstring) {
    return ERR_NONE;
}

/////////////////////////////////////////////////////////////////////////////
void CatchExceptions(  const string& szDumpFile, const string& szProfileName)
{
	/*
	TBD: how we catch exceptions for Diomede via the ClientLogger.
	*/
}


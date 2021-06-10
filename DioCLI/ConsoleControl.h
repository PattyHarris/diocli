/*********************************************************************
 * 
 *  file:  ConsoleControl.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Class implements the console command processing 
 *          from the user.
 * 
 *********************************************************************/

//! \ingroup consolecontrol ConsoleControl
//! @{

#ifndef __CONSOLE_CONTROL_H__
#define __CONSOLE_CONTROL_H__

#include "stdafx.h"
#include "CommandDefs.h"
#include "ResumeInfoData.h"
#include "Enum.h"

#include "../Include/tclap/CmdLine.h"
#include "DiomedeCmdLine.h"
#include "DiomedeStdOut.h"
#include "DiomedeUnlabeledValueArg.h"
#include "DiomedeUnlabeledMultiArg.h"
#include "DiomedeValueArg.h"
#include "DiomedeMultiArg.h"
#include "DiomedeSwitchArg.h"

#include "../Util/ClientLog.h"
#include "../Util/FileLogger.h"
#include "../Util/MessageTimer.h"

#include "../Include/DiomedeStorage.h"
#include "../Util/UserProfileData.h"
#include "DiomedeTask.h"

#include <queue>
#include <sys/stat.h>

using namespace std;
using namespace TCLAP;
using namespace boost::posix_time;
using namespace DIOMEDE_CONSOLE;

// Needed for building the mac.
using DIOMEDE_CONSOLE::CommandThread;

using namespace ResumeInfoTypes;
using namespace ResumeInfoIntervalTypes;

#define DEFAULT_DCF_FILENAME    _T("diomede.dcf")
#define DEFAULT_PREPEND_DCF     false

///////////////////////////////////////////////////////////////////////
class ConsoleControl : public LogObserver
{
protected:
    //-----------------------------------------------------------------
    //! \brief Upload file structure used for interacting with the CPPSDK
    //-----------------------------------------------------------------
    struct UploadFileInfo {
        UploadFileInfo() : m_commandThread(), m_msgTimer(50),
                   m_szFilePath(_T("")), m_szRelativePath(_T("")),
                   m_szParentDir(_T("")), m_szFileName(_T("")),
                   m_l64FileID(0), m_l64FileSize(0),
                   m_szHashMD5(_T("")), m_nBytesRead(0), m_nCurrentBlock(0), m_nUploadStatus(0),
                   m_szFormattedFileName(_T("")),
                   m_szFormattedBytes(_T("")), m_szFormattedBytesType(_T(""))
        {
        	m_commandThread.SetThreadType(ThreadTypeHomogeneous);
            m_commandThread.Start();
        };

        UploadFileInfo(const UploadFileInfo& uploadFileInfo) : m_commandThread()
        {
        	m_commandThread.SetThreadType(ThreadTypeHomogeneous);
            m_commandThread.Start();

            m_msgTimer = uploadFileInfo.m_msgTimer;
            m_resumeUploadInfoData = uploadFileInfo.m_resumeUploadInfoData;

            m_szFilePath = uploadFileInfo.m_szFilePath;
            m_szRelativePath = uploadFileInfo.m_szRelativePath;
            m_szParentDir = uploadFileInfo.m_szParentDir;
            m_szFileName = uploadFileInfo.m_szFileName;
            m_l64FileID = uploadFileInfo.m_l64FileID;
            m_l64FileSize = uploadFileInfo.m_l64FileSize;
            m_szHashMD5 = uploadFileInfo.m_szHashMD5;
            m_nBytesRead = uploadFileInfo.m_nBytesRead;
            m_nCurrentBlock = uploadFileInfo.m_nCurrentBlock;
            m_nUploadStatus = uploadFileInfo.m_nUploadStatus;

            m_szFormattedFileName = uploadFileInfo.m_szFormattedFileName;
            m_szFormattedBytes = uploadFileInfo.m_szFormattedBytes;
            m_szFormattedBytesType = uploadFileInfo.m_szFormattedBytesType;
        };

		~UploadFileInfo()
		{
            m_commandThread.Stop();
		};

        CommandThread               m_commandThread;
        MessageTimer                m_msgTimer;
        ResumeUploadInfoData        m_resumeUploadInfoData;

        std::string                 m_szFilePath;
        std::string                 m_szRelativePath;
        std::string                 m_szParentDir;
        std::string                 m_szFileName;
        LONG64                      m_l64FileID;
        LONG64                      m_l64FileSize;
        std::string                 m_szHashMD5;

        int                         m_nBytesRead;
        UINT                        m_nCurrentBlock;
        int                         m_nUploadStatus;

        std::string                 m_szFormattedFileName;
        std::string                 m_szFormattedBytes;
        std::string                 m_szFormattedBytesType;

       //-----------------------------------------------------------------
       // operator=
       //-----------------------------------------------------------------
        void operator=( const UploadFileInfo& uploadFileInfo)
        {
            m_msgTimer = uploadFileInfo.m_msgTimer;
            m_resumeUploadInfoData = uploadFileInfo.m_resumeUploadInfoData;

            m_szFilePath = uploadFileInfo.m_szFilePath;
            m_szRelativePath = uploadFileInfo.m_szRelativePath;
            m_szParentDir = uploadFileInfo.m_szParentDir;
            m_szFileName = uploadFileInfo.m_szFileName;
            m_l64FileID = uploadFileInfo.m_l64FileID;
            m_l64FileSize = uploadFileInfo.m_l64FileSize;
            m_szHashMD5 = uploadFileInfo.m_szHashMD5;
            m_nBytesRead = uploadFileInfo.m_nBytesRead;
            m_nCurrentBlock = uploadFileInfo.m_nCurrentBlock;
            m_nUploadStatus = uploadFileInfo.m_nUploadStatus;

            m_szFormattedFileName = uploadFileInfo.m_szFormattedFileName;
            m_szFormattedBytes = uploadFileInfo.m_szFormattedBytes;
            m_szFormattedBytesType = uploadFileInfo.m_szFormattedBytesType;
        }

       //-----------------------------------------------------------------
       // Clear all the data
       //-----------------------------------------------------------------
        void ClearAll()  {
            MessageTimer tmpTimer;
            m_msgTimer = tmpTimer;

            ResumeUploadInfoData tmpResumeUploadInfoData;
            m_resumeUploadInfoData = tmpResumeUploadInfoData;

            m_szFilePath = _T("");
            m_szRelativePath = _T("");
            m_szParentDir = _T("");
            m_szFileName = _T("");
            m_l64FileID = 0;
            m_l64FileSize = 0;
            m_szHashMD5 = _T("");
            m_nBytesRead = 0;
            m_nCurrentBlock = 0;
            m_nUploadStatus = 0;

            m_szFormattedFileName = _T("");
            m_szFormattedBytes = _T("");
            m_szFormattedBytesType = _T("");
        }
    };

    //-----------------------------------------------------------------
    //! \brief Download file structure used for interacting with the CPPSDK
    //-----------------------------------------------------------------
    struct DownloadFileInfo {
        DownloadFileInfo() : m_commandThread(), m_msgTimer(50),
                   m_szDownloadPath(_T("")), m_szDownloadURL(_T("")), m_szDownloadFileName(_T("")),
                   m_l64FileID(0),
                   m_nDownloadStatus(0), m_l64CurrentBytes(0), m_l64TotalBytes(0),
                   m_szFormattedFileName(_T("")),
                   m_szFormattedBytes(_T("")), m_szFormattedBytesType(_T(""))
        {
            m_commandThread.Start();
        };

        DownloadFileInfo(const DownloadFileInfo& downloadFileInfo) : m_commandThread()
        {
            m_commandThread.Start();

            m_msgTimer = downloadFileInfo.m_msgTimer;

            m_szDownloadPath = downloadFileInfo.m_szDownloadPath;
            m_szDownloadURL = downloadFileInfo.m_szDownloadURL;
            m_szDownloadFileName = downloadFileInfo.m_szDownloadFileName;
            m_l64FileID = downloadFileInfo.m_l64FileID;
            m_nDownloadStatus = downloadFileInfo.m_nDownloadStatus;

            m_l64CurrentBytes = downloadFileInfo.m_l64CurrentBytes;
            m_l64TotalBytes = downloadFileInfo.m_l64TotalBytes;

            m_szFormattedFileName = downloadFileInfo.m_szFormattedFileName;
            m_szFormattedBytes = downloadFileInfo.m_szFormattedBytes;
            m_szFormattedBytesType = downloadFileInfo.m_szFormattedBytesType;
        };

		~DownloadFileInfo()
		{
            m_commandThread.Stop();
		};

        CommandThread               m_commandThread;
        MessageTimer                m_msgTimer;

        std::string                 m_szDownloadPath;
        std::string                 m_szDownloadURL;
        std::string                 m_szDownloadFileName;

        LONG64                      m_l64FileID;
        int                         m_nDownloadStatus;

        LONG64                      m_l64CurrentBytes;
        LONG64                      m_l64TotalBytes;

        std::string                 m_szFormattedFileName;
        std::string                 m_szFormattedBytes;
        std::string                 m_szFormattedBytesType;


       //-----------------------------------------------------------------
       // operator=
       //-----------------------------------------------------------------
        void operator=( const DownloadFileInfo& downloadFileInfo)
        {
            m_msgTimer = downloadFileInfo.m_msgTimer;

            m_szDownloadPath = downloadFileInfo.m_szDownloadPath;
            m_szDownloadURL = downloadFileInfo.m_szDownloadURL;
            m_szDownloadFileName = downloadFileInfo.m_szDownloadFileName;
            m_l64FileID = downloadFileInfo.m_l64FileID;
            m_nDownloadStatus = downloadFileInfo.m_nDownloadStatus;

            m_l64CurrentBytes = downloadFileInfo.m_l64CurrentBytes;
            m_l64TotalBytes = downloadFileInfo.m_l64TotalBytes;

            m_szFormattedFileName = downloadFileInfo.m_szFormattedFileName;
            m_szFormattedBytes = downloadFileInfo.m_szFormattedBytes;
            m_szFormattedBytesType = downloadFileInfo.m_szFormattedBytesType;
        };

       //-----------------------------------------------------------------
       // Clear all the data
       //-----------------------------------------------------------------
        void ClearAll()  {
            MessageTimer tmpTimer;
            m_msgTimer = tmpTimer;

            m_szDownloadPath = _T("");
            m_szDownloadURL = _T("");
            m_szDownloadFileName = _T("");
            m_l64FileID = 0;
            m_nDownloadStatus = 0;

            m_l64CurrentBytes = 0;
            m_l64TotalBytes = 0;

            m_szFormattedFileName = _T("");
            m_szFormattedBytes = _T("");
            m_szFormattedBytesType = _T("");

        }
    };

    //-----------------------------------------------------------------
    //! \brief Display file structure used for interacting with the CPPSDK
    //-----------------------------------------------------------------
    struct DisplayFileInfo {
        DisplayFileInfo() : m_commandThread(), m_msgTimer(50),
                   m_szDownloadURL(_T("")), m_szDisplayFileName(_T("")), m_l64FileID(0),
                   m_nDisplayFileStatus(0), m_l64CurrentBytes(0), m_l64TotalBytes(0),
                   m_szFormattedFileName(_T("")),
                   m_szFormattedBytes(_T("")), m_szFormattedBytesType(_T(""))
        {
            m_commandThread.Start();
        };

		~DisplayFileInfo()
		{
            m_commandThread.Stop();
		};

        public:
            CommandThread               m_commandThread;
            MessageTimer                m_msgTimer;

            std::string                 m_szDownloadURL;
            std::string                 m_szDisplayFileName;

            LONG64                      m_l64FileID;
            int                         m_nDisplayFileStatus;

            LONG64                      m_l64CurrentBytes;
            LONG64                      m_l64TotalBytes;

            std::string                 m_szFormattedFileName;
            std::string                 m_szFormattedBytes;
            std::string                 m_szFormattedBytesType;

       //-----------------------------------------------------------------
       // Clear all the data
       //-----------------------------------------------------------------
        void ClearAll()  {
            MessageTimer tmpTimer;
            m_msgTimer = tmpTimer;

            m_szDownloadURL = _T("");
            m_szDisplayFileName = _T("");
            m_l64FileID = 0;
            m_nDisplayFileStatus = 0;

            m_l64CurrentBytes = 0;
            m_l64TotalBytes = 0;

            m_szFormattedFileName = _T("");
            m_szFormattedBytes = _T("");
            m_szFormattedBytesType = _T("");

        }
    };

    //-----------------------------------------------------------------
    //! \brief Display file enumeration structure used for interacting 
    //!        file enumeration prior to upload.
    //-----------------------------------------------------------------
    struct DisplayFileEnumInfo {
        DisplayFileEnumInfo() : m_commandThread(), m_msgTimer(50),
                   m_nDisplayFileEnumStatus(0)
        {
            m_commandThread.Start();
        };

		~DisplayFileEnumInfo()
		{
            m_commandThread.Stop();
		};

        public:
            CommandThread               m_commandThread;
            MessageTimer                m_msgTimer;
            int                         m_nDisplayFileEnumStatus;

       //-----------------------------------------------------------------
       // Clear all the data
       //-----------------------------------------------------------------
        void ClearAll()  {
            MessageTimer tmpTimer;
            m_msgTimer = tmpTimer;
            m_nDisplayFileEnumStatus = 0;
        }
    };

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    DiomedeStdOutput        m_stdOut;
    bool                    m_bRedirectedInput;
	bool			        m_bContinueProcessing;      ///< This is true until we want to quit
	bool			        m_bConnected;			    ///< State of connection to server
    bool                    m_bMaskInput;

	std::string             m_szUsername;               ///< User connected to the Diomede service.

	std::string             m_szClientData;
	std::string             m_szPlainTextPassword;

	std::string             m_szSessionToken;           ///< Session ID for the logged in user?
	bool                    m_bValidCertificate;        ///< Certificate created successfully.

	unsigned long           m_lWin32LastError;          ///< Windows system error code.
	int                     m_nLastError;               ///< Generic system error code.

	LONG64                  m_l64TotalUploadedBytes;    ///< Uploading helper attributes
	LONG64                  m_l64TotalUploadMilliseconds;
	time_duration           m_tdTotalUpload;

	UploadFileInfo*         m_pUploadInfo;              ///< Allocated task on the heap to
	DIOMEDE_CONSOLE::UploadTask* m_pTaskUpload;         ///< reuse the same thread for the
	DIOMEDE_CONSOLE::CreateFileTask* m_pTaskCreateFile; ///< entire upload.
	DIOMEDE_CONSOLE::SetFileMetaDataTask* m_pTaskSetFileMetaData;

	DownloadFileInfo*       m_pDownloadInfo;
	DisplayFileInfo*        m_pDisplayFileInfo;
	DisplayFileEnumInfo*    m_pDisplayFileEnumInfo;
	CEnum*                  m_pFileEnumerator;              ///< Allocated on the heap to allow us
	                                                        ///< to stop the enumeration if 
	                                                        ///< requested by the user.

	typedef std::map<std::string, int> MetaDataMap;
    MetaDataMap             m_listMetaDataFullPaths;        ///< For the current upload, map of current metadata's
    MetaDataMap             m_listMetaDataRelativePaths;    ///< IDs to full and relative file paths.

	class StorageTypeListImpl* m_pListStorageTypes;         ///< Diomede storage types. Filled on first access.


private:
    friend class CommandThread;

    std::string             m_szCommandPrompt;          ///< Current DioCLI command prompt.

    bool                    m_bSysCommandInput;         ///< Input is from the system command line.

    bool                    m_bIncompleteCommand;       ///< Current command is incomplete, user will
                                                        ///< be prompted for more.

    bool                    m_bIgnoreCommandInput;      ///< Input for this command is ignored (e.g. REM).
    bool                    m_bAutoLoginOn;
    bool                    m_bAutoLogoutOff;
    bool                    m_bAutoCheckAccountOn;
    bool                    m_bEchoOn;
    int                     m_nSystemSleep;             ///< Sleep period to give time back to other
                                                        ///< running processes.

	deque<string>		    m_actionStack;              ///< Current command and arguments.
	class CmdLine*          m_pCurrentCommand;          ///< Used for repeating the command
	int                     m_nCurrentNumArgs;
	DioCLICommands::COMMAND_ID m_currentCmdID;

	typedef std::map< DioCLICommands::COMMAND_ID, CmdLine* > CommandMap;
	typedef UnlabeledValueArg<string> UnlabledValueStrArg;

    CommandMap              m_listCommands;         ///< Command ID to command objects.
	unsigned int	        m_errorCode;		    ///< On exit this is the error code
	bool			        m_bEnableLogging;
	FileLogger		        m_fileLogger;           ///< ClientLog to file
	bool                    m_bEnableStdLogging;    ///< Turn off or on logging to the console.

    std::string             m_szAppVersion;         ///< Application vesion

    /*! Possible file ID types.
     */
    typedef enum FileIDTypes {
           fileType = 0,
           fileIDType,
           md5Type,
           sha1Type,
           unknownType
    } FileIDType;

	//-----------------------------------------------------------------
    /*! Possible error codes returned from the service or gSoap.
     *  gSoap does return additional errors, but we will address them
     *  when they become a problem.  The No Data error generally
     *  is received when there is no internet connnection.
     */
	//-----------------------------------------------------------------
    enum ServiceErrorTypes {
        SESSION_TOKEN_EXPIRES = 0,
        INVALID_SESSION_TOKEN,
        GSOAP_NO_DATA,
        GSOAP_HOST_NOT_FOUND,
        GSOAP_TCP_ERROR,
        WSA_HOST_UNREACHABLE,       // WSAEHOSTUNREACH: 10065
        WSA_NETWORK_UNREACHABLE,    // WSAENETUNREACH: 10051
        GENERIC_NETWORK_IS_UNREACHABLE,
        GSOAP_EOF,
        GENERIC_SERVICE,            // "An error has occurred.  Please try your
                                    // request again."
        LAST_SERVICE_TYPES_ENUM
    };

    /*! Possible service errors.
     */
    static const std::string ServiceErrorStrings[LAST_SERVICE_TYPES_ENUM+1];


private:
	//-----------------------------------------------------------------
	// Show the list of commands
	//-----------------------------------------------------------------
	void ShowCommandsMenu();
	void ShowConfigSettings();

	//-----------------------------------------------------------------
	// Command error handling
	//-----------------------------------------------------------------
	bool VerifySystemCommand(std::string szCmdName);
	void UnknownCommandError(std::string szCmdName, ArgException& e);
	void ParseCommandError(CmdLine* pCmdLine, ArgException& e);

	void PrintStatusMsg(std::string szCompletionMsg,
        bool bPrintNewLineBefore=false, bool bPrintNewLineAfter=true, bool bAllowLineWrap=false);
	void PrintSoapError(struct soap* soap, FILE* fd, std::string szErrorMsg=_T(""),
        bool bPrintNewLineBefore=false, bool bPrintNewLineAfter=true);
	void PrintServiceError(FILE* fd, std::string szErrorMsg,
        bool bPrintNewLineBefore=false, bool bPrintNewLineAfter=true,
        bool bRepeatCommand=true);
	void PrintResumeError(int nResumeResult, std::string szErrorMsg,
        bool bPrintNewLineBefore=true, bool bPrintNewLineAfter=false);
	void PrintResumeWarning(int nResumeResult, std::string szWarningMsg,
        bool bPrintNewLineBefore=true, bool bPrintNewLineAfter=false);

    bool CheckServiceErrorToRetry(std::string szErrorMsg, std::string& szFriendlyErrorMsg);
    bool CheckServiceErrorToResume(std::string szErrorMsg);

    bool CheckThread(CommandThread* pThread, std::string szThreadUsageType);

    int WriteResumeUploadData(ResumeUploadInfoData& resumeUploadData,
                              std::string szClientLogMsg=_T(""));

	//-----------------------------------------------------------------
	// User configuration handling
	//-----------------------------------------------------------------
    bool UseClipboard(CmdLine* pCmdLine);
	bool VerboseOutput(CmdLine* pCmdLine);

    bool GetConfigClipboard(bool& bCopyToClipboard);
	bool GetConfigVerbose(bool& bVerbose);
	bool GetConfigPageSize(LONG64& l64PageSize);
	bool GetConfigOffset(LONG64& l64Offset);
	bool GetConfigResumeIntervals(std::vector<int>& listResumeIntervals);

	bool CopyStringToClipboard(string szClipboardText);
	bool TestOutput(CmdLine* pCmdLine);
	bool ShowUsage(CmdLine* pCmdLine);
	bool PauseProcess();
	bool SetConsoleControlHandler();

	//-----------------------------------------------------------------
	// Processing of commands
	//-----------------------------------------------------------------

	void ProcessHelpCommand(int nNumArgs, CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessAboutCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
	// Login/Logout commands
	//-----------------------------------------------------------------
	void ProcessLoginCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool LoginUser(std::string szUsername, std::string szPassword, bool bShowProgress = true);
	void LoggedInUserError(std::string szErrorMsg=_T(""));
	time_t UpdateSessionTokenExpiration();
	bool HasLocalSessionTokenExpired();

	void ProcessChangePasswordCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessResetPasswordCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	void ProcessConfigCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool SetAutoLoginData(bool bAutoLogin=true,
	                      std::string szUsername=_T(""), std::string szPassword=_T(""));

	void ProcessLogoutCommand(CmdLine* pCmdLine, bool& bCommandFinished);
    void ProcessGetSessionTokenCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
	// User commands
	//-----------------------------------------------------------------
	void ProcessCreateUserCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool GetUserInfo(CmdLine* pCmdLine, UserData* pUserData, bool& bCommandFinished,
	    bool bAllFieldsRequired=true);
	bool GetUserInfoFromSysCommandLine(CmdLine* pCmdLine,
	                                   UserData* pUserData,
	                                   bool& bCommandFinished);
	bool IsUserInfoArgComplete(CmdLine* pCmdLine, DiomedeUnlabeledValueArg<string>* pArg,
	    std::string szCurrentPrompt, std::string szNextPrompt,
	    bool& bCommandFinished, bool bAllFieldsRequired);

	void ProcessDeleteUserCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	void ProcessSetUserInfoCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessGetUserInfoCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessDeleteUserInfoCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void InitUserInfo(dds__UserInfo* pUserInfo);
	void DeleteUserInfo(dds__UserInfo* pUserInfo);

	void ProcessGetEmailAddressesCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessAddEmailAddressCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessDeleteEmailAddressCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessSetPrimaryEmailAddressCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
	// Miscellaneous account commands
	//-----------------------------------------------------------------
	void ProcessCheckAccountCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	int CheckAccount();

	void ProcessSubscribeUserCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool HandleBillingDataInput(CmdLine* pCmdLine, bool& bCommandFinished, int& nResult);

	//-----------------------------------------------------------------
	// Billing commands
	//-----------------------------------------------------------------
	void ProcessSetBillingDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessGetBillingDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessDeleteBillingDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessSearchPaymentsCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
	//! \defgroup files File Commands
	//! \ingroup consolecontrol
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	//! \defgroup upload_rsume Upload and Resume Methods
	//! \ingroup files
    // @{
	//-----------------------------------------------------------------
	void ProcessUploadCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessResumeCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool DisplayResumeUploadList(ResumeInfoType nResumeInfoType);
	bool DisplayResumeUploadListVerbose(ResumeInfoType nResumeInfoType);

	int RepeatLastUploadTask(DiomedeTask* pTask);
	int UploadFileBlocks(bool bAddPathMetaData=false, bool bCreateMD5Digest=false,
	                     LONG64 l64TotalUploadedBytes=0);

	int CreateFile(class UploadImpl* pUploadData);
	int RepeatLastCreateFileTask(DiomedeTask* pTask);
	int ResumeCreateFile(DIOMEDE_CONSOLE::CreateFileTask* pTask, bool& bUserCancelled);

	int ResumeCurrentUpload(DIOMEDE_CONSOLE::UploadTask* pTask, bool& bUserCancelled);
	bool ResumeCountdown(ResumeInfoIntervalType nIntervalType);

public:
    void UpdateUploadStatus(int nUploadStatus, LONG64 l64CurrentBytes);
    static bool UploadStatus(void* pUploadUser, int nUploadStatus, LONG64 l64CurrentBytes)
    {
        ConsoleControl* pConsoleControl = (ConsoleControl*)pUploadUser;
        if (pConsoleControl) {
            pConsoleControl->UpdateUploadStatus(nUploadStatus, l64CurrentBytes);
        }

        return true;
    }

private:

	void ProcessGetUploadTokenCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
    // @}
	//-----------------------------------------------------------------

private:
	//-----------------------------------------------------------------
	//! \defgroup search Search Files Methods
	//! \ingroup files
    // @{
	//-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // Search files - work with CPPSDK.Lib
    //-----------------------------------------------------------------
	void ProcessSearchFilesCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool SetupSearchFilter(CmdLine* pCmdLine, SearchFileFilterImpl* pSearchFilter,
	                       bool& bIsDeleted, bool& bArgIsSet);

	void DisplaySearchFilesResults(class FilePropertiesListImpl* pListFileProperties,
	    bool bShowFileDate=true, bool bShowDeleted=false);
	void DisplaySearchFilesResultsVerbose(class FilePropertiesListImpl* pListFileProperties,
	                                      class LogicalPhysicalFilesInfoListImpl* pListLogicalPhysicalFiles,
	                                      bool bMaxPageSize=false);

    //-----------------------------------------------------------------
    // Search helper function
    //-----------------------------------------------------------------
	bool SearchFiles(DiomedeStorageService* pStorageService,
	                 class _sds__SearchFilesRequest* pFileRequest,
	                 class _sds__SearchFilesResponse* pFileResponse,
	                 bool bShowProgress = false);

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
	void ProcessSearchFilesTotal(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessSearchFilesTotalLog(CmdLine* pCmdLine, bool& bCommandFinished);

    bool DisplaySearchFilesTotalEntry(class FilesTotalLogEntryImpl* pFileTotalInfo,
                                       bool bShowHeader=true);
    bool DisplaySearchFilesTotalEntryVerbose(class FilesTotalLogEntryImpl* pFileTotalInfo,
                                             bool bLogDisplay=false);

	bool GetFileID(DiomedeStorageService* pStorageService, std::string szInFileID,
                   LONG64& l64FileID, std::string szErrorText=_T(""));
    bool GetFileData(DiomedeStorageService* pStorageService, std::string szInFileID,
                     FilePropertiesImpl* pFileProperties, std::string szErrorText /*""*/);

	//-----------------------------------------------------------------
    // @}
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	//! \defgroup download Download Methods
	//! \ingroup files
    // @{
	//-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // Download file
    //-----------------------------------------------------------------
	void ProcessDownloadCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	int RepeatLastDownloadTask(DiomedeTask* pTask);

public:
    // Download URL status
    void UpdateDownloadURLStatus(int nDownloadStatus, std::string szDownloadURL,
                                 std::string szDownloadFileName);
    static bool DownloadURLStatus(void* pDownloadUser, int nDownloadStatus, std::string szDownloadURL,
                                  std::string szDownloadFileName)
    {
        ConsoleControl* pConsoleControl = (ConsoleControl*)pDownloadUser;
        if (pConsoleControl) {
            pConsoleControl->UpdateDownloadURLStatus(nDownloadStatus, szDownloadURL, szDownloadFileName);
        }

        return true;
    }

    // Download byte status
    void UpdateDownloadStatus(int nDownloadStatus, LONG64 l64CurrentBytes = 0, LONG64 lTotalBytes = 0);
    static bool DownloadStatus(void* pDownloadUser, int nDownloadStatus, LONG64 l64CurrentBytes = 0,
                               LONG64 lTotalBytes = 0)
    {
        ConsoleControl* pConsoleControl = (ConsoleControl*)pDownloadUser;
        if (pConsoleControl) {
            pConsoleControl->UpdateDownloadStatus(nDownloadStatus, l64CurrentBytes, lTotalBytes);
        }

        return true;
    }

    // Display file status
    void UpdateDisplayFileStatus(int nDisplayFileStatus, LONG64 l64CurrentBytes = 0, LONG64 lTotalBytes = 0);
    static bool DisplayFileStatus(void* pDisplayFileUser, int nDisplayFileStatus, LONG64 l64CurrentBytes = 0,
                               LONG64 lTotalBytes = 0)
    {
        ConsoleControl* pConsoleControl = (ConsoleControl*)pDisplayFileUser;
        if (pConsoleControl) {
            pConsoleControl->UpdateDisplayFileStatus(nDisplayFileStatus, l64CurrentBytes, lTotalBytes);
        }

        return true;
    }
    
    // File enumeration status
    void UpdateDisplayFileEnumStatus(int nDisplayFileEnumStatus);
    static bool DisplayFileEnumStatus(void* pDisplayFileEnumUser, int nDisplayFileEnumStatus)
    {
        ConsoleControl* pConsoleControl = (ConsoleControl*)pDisplayFileEnumUser;
        if (pConsoleControl) {
            pConsoleControl->UpdateDisplayFileEnumStatus(nDisplayFileEnumStatus);
        }

        return true;
    }

    //-----------------------------------------------------------------
    // GetDownloadURL
    //-----------------------------------------------------------------
	void ProcessGetDownloadURL(CmdLine* pCmdLine, bool& bCommandFinished);
	bool GetDownloadURL(DiomedeStorageService* pStorageService,
	                    _sds__GetDownloadURLRequest* pDownloadURLRequest,
                        std::string& szDownloadURL, bool bShowProgress = true);
    bool DeleteGetDownloadURL(_sds__GetDownloadURLRequest* pDownloadURLRequest);

    //-----------------------------------------------------------------
    // GetDownloadURL Using Lib
    //-----------------------------------------------------------------
	void ProcessGetDownloadURLUsingLib(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
    // @}
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	//! \defgroup miscellaneous Miscellaneous File Processing Methods
	//! \ingroup files
    // @{
	//-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // Miscellaneous file processing
    //-----------------------------------------------------------------
	void ProcessRenameFileCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessDeleteFileCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessUndeleteFileCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessDisplayFileCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
    // @}
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	//! \defgroup metadata Metadata Methods
	//! \ingroup files
    // @{
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	// Metadata
	//-----------------------------------------------------------------
	void ProcessCreateMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	void ProcessCreateFileMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool CreateFileMetaData(LONG64 l64FileID, std::string szName, std::string szValue,
	                        int& nMetaDataID, bool bShowProgress = true);
	bool CheckMetaDataCache(std::string szName, std::string szValue, int& nMetaDataID);
	bool CheckForMetaDataOnServer(std::string szName, std::string szValue, int& nMetaDataID);

	void ProcessSetFileMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool SetFileMetaData(LONG64 l64FileID, int nMetaDataID, bool bShowProgress = true);

	void ProcessDeleteFileMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessDeleteMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	void ProcessGetFileMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessGetMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void DisplayMetaData(class MetaDataListImpl* pListMetaData);

	void ProcessEditMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
    // @}
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	//! \defgroup replication Replication Methods
	//! \ingroup files
    // @{
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	// Replication
	//-----------------------------------------------------------------
	void ProcessReplicateFileCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessUnReplicateFileCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessGetStorageTypesCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool GetStorageTypes(bool bShowProgress);
	bool GetStorageTypeName(int nStorageTypeID, std::string& szStorageTypeName);

	void ProcessGetPhysicalFilesCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool DisplayPhysicalFileInfo(LONG64 l64FileID,
	                             class PhysicalFileInfoListImpl* pListPhysicalFileInfo,
	                             bool bDisplayFileID=false);
    void DisplayPhysicalFileInfoEntry(PhysicalFileInfoImpl* pPhysicalFileInfoEntry, int nCopy=0);

	//-----------------------------------------------------------------
	// Replication policy
	//-----------------------------------------------------------------
	void ProcessCreateReplicationPolicyCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	bool SetupReplicationPolicyInfo(CmdLine* pCmdLine, bool& bCommandFinished,
	                                ReplicationPolicyInfoImpl* pReplicationPolicyInfo,
	                                bool& bArgIsSet);

	bool SetupReplicationPolicyID(CmdLine* pCmdLine, bool& bCommandFinished,
	                              int& nReplicationPolicyID);

	void ProcessGetReplicationPoliciesCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool DisplayReplicationPolicy(ReplicationPolicyInfoImpl* pReplicationPolicyInfo,
	                              bool bShowHeader=true);

	void ProcessEditReplicationPolicyCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessDeleteReplicationPolicyCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessSetReplicationPolicyCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessSetDefaultReplicationPolicyCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessGetDefaultReplicationPolicyCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
    // @}
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	// Purchasing and billing
	//-----------------------------------------------------------------
	void ProcessGetAllProductsCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessPurchaseProductCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	bool PurchaseProduct(const std::vector<std::string>& listRateIDs, int& nResult);

	void ProcessGetMyProductsCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessCancelProductCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	void ProcessGetAllContractsCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessPurchaseContractCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessGetMyContractsCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessCancelContractCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
	// Search logs
	//-----------------------------------------------------------------
	void ProcessSearchUploadLogCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessSearchDownloadLogCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void ProcessSearchLoginLogCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	void ProcessSearchInvoiceLogCommand(CmdLine* pCmdLine, bool& bCommandFinished);
	void TestSearchInvoicesData(InvoiceLogListImpl* pListInvoiceLogEntries,
	                            bool& bCreateNewInvoice);
	void TestSearchInvoicesCleanup(InvoiceLogListImpl* pListInvoiceLogEntries,
	                               bool bCreateNewInvoice);

	//-----------------------------------------------------------------
	// System commands
	//-----------------------------------------------------------------
	void ProcessSystemCommand(DioCLICommands::COMMAND_ID cmdID,
	                          CmdLine* pCmdLine, bool& bCommandFinished);

	void ProcessEchoSystemCommand(CmdLine* pCmdLine, bool& bCommandFinished);

	//-----------------------------------------------------------------
	// Helper functions (which may move to it's own utility class).
	//-----------------------------------------------------------------
	int HandleTask(DiomedeTask* pTask, std::string szStartMsg =_T(""),
	                                   std::string szEndMsg=_T(""),
	                                   bool bDoneNewLine=false,
	                                   bool bBeforeAndAfterNewLines=true,
	                                   bool bShowProgress=true);

	int RepeatLastTask(DiomedeTask* pTask, MessageTimer* pMsgTimer,
	                   std::string szStartMsg =_T(""), bool bShowProgress=true);

    bool ValidateDate(std::string szDate, time_t& epochSeconds);
    bool ParseYearMonth(std::string szDate, int& nYear, int& nMonth);
    void GetFileIDType(std::string szInFileID, int& nFileIDType);
    bool GetFileExtension(std::string szFilePath, std::string& szExtension);
    bool ValidateCreditCard(const char* szCredCardNumber);
    bool ValidateCVV(const std::string szCVV);

	//-----------------------------------------------------------------
	// Command setup
	//-----------------------------------------------------------------
    void SetupCommands();

    void SetupLoginCommand();
    void SetupCreateUserCommand();
    void SetupSetUserInfoCommand();
    void SetupSetBillingDataCommand(const std::string& szCommand, DioCLICommands::COMMAND_ID cmdID,
		                            const std::string& szMessage);

    void SetupMetaDataCommands();
    void SetupReplicationCommands();

    void SetupReplicationPolicyCommands();
    void SetupReplicationPolicyInfoCommand(const std::string& szCommand,
                                           DioCLICommands::COMMAND_ID cmdID,
		                                   const std::string& szMessage,
		                                   bool bAddReplicationPolicyID=false);

    void SetupSearchFilesCommand();
    void SetupSearchFilesTotalCommand();
    void SetupSearchFilesTotalLogCommand();
    void SetupSearchUploadsCommand();
    void SetupSearchDownloadsCommand();
    void SetupSearchLoginsCommand();
    void SetupSearchInvoiceLogCommand();

	//-----------------------------------------------------------------
	//-----------------------------------------------------------------
	void DisplayBanner();

public:
	ConsoleControl();
	virtual ~ConsoleControl();

	typedef struct {
		unsigned int	m_nCommandID;
		string			m_szCommandStr;
		vector<string>  m_listParameters;
	} ACTION_REQUEST;

    inline std::string GetCommandPrompt()  {
        return m_szCommandPrompt;
    }

	void SetCommandPrompt();
	void ResetLoginData();

	inline void PrintNewLine() {
	    _tprintf(_T("\n\r"));
	}

	void PrintTextInColor(std::string szOutputText, int nColor);
	void PrintTextInColorOn(int nColor, unsigned short& nOriginalColors);
	void PrintTextInColorOff(unsigned short nOriginalColors);

	bool IsSysCommandLineInput() { return m_bSysCommandInput; }
	void SetSysCommandLineInput(const bool& bSysCommandLineInput) {
	    m_bSysCommandInput = bSysCommandLineInput;
	}

	bool IsCommandIncomplete() { return m_bIncompleteCommand; }
    bool IsCommandInputIgnored() { return m_bIgnoreCommandInput; }

	bool IsRedirectedInput() { return m_bRedirectedInput; }
	void SetRedirectedInput(const bool& bIsRedirected = true) {
	    m_bRedirectedInput = bIsRedirected;
	}

	bool Finished()	{ return m_bContinueProcessing == false; }
	void SetFinished(const bool& bIsFinished = true) {
	     m_bContinueProcessing = bIsFinished;
	}

	bool IsMaskInput() { return m_bMaskInput == true; }
	void SetMaskInput(const bool& bMaskInput = true) {
	     m_bMaskInput = bMaskInput;
	}

	bool StartDioCLI( int argc, char** argv,
	                  const string szConfigFilename = DEFAULT_DCF_FILENAME,
				      const bool bUseDataDirectory = DEFAULT_PREPEND_DCF);
	bool StopDioCLI();

	//-----------------------------------------------------------------
	// Called once per execution cycle. Returns the value for sleep
	// Used for threading the console work (TBD).
	//-----------------------------------------------------------------
	unsigned int MainLoop();

	//-----------------------------------------------------------------
	// Common start and stop (TBD: to allow for this console to act
	// as a base class.)
	//-----------------------------------------------------------------

	bool CommonStartDioCLI( const string dcfFilename, const bool bUseDataDirectory);
	void CommonStopDioCLI();

	//-----------------------------------------------------------------
	// Proxy initialization and exit.
	//-----------------------------------------------------------------
	bool ExtractCertificate();
	bool IsPemGood();
	std::string GetErrorString();
	void ResetErrorCodes() {
	    m_lWin32LastError = 0;
	    m_nLastError = 0;
	}

	bool ShutdownProxyService();
	bool LogoutDiomedeService();

	//-----------------------------------------------------------------
	// Process any actions wating on the stack
	// Some actions are processed locally - others passed back to the
	// calling sub class
	//-----------------------------------------------------------------
	bool AddAction( const string szInput );
	bool AddActions(int argc, char** argv);
	bool AddQuotesToArgument(std::string szArg, std::string& szArgQuoted);
	bool RemoveQuotesFromArgument(std::string szArgQuoted, std::string& szArg);

	ACTION_REQUEST ProcessActionStack();
	bool ProcessCommand(DioCLICommands::COMMAND_ID cmdID, int nNumArgs,
	                    CmdLine* pCmdLine, bool& bCommandFinished);

    bool ClearLastArgumentFromActionStack();
	bool Trim( char* szCommandLine, std::string& szTrimmed );
	bool ProcessControlHandler(DWORD fdwCtrlType);

	//-----------------------------------------------------------------
	// Client logging
	//-----------------------------------------------------------------
	unsigned int GetErrorCode()	{ return m_errorCode; }
	void SetErrorCode( unsigned int newCode ) { m_errorCode = newCode;}
	void EnableLoggingToFile(const bool newSetting);
	void EnableLoggingToConsole(const bool newSetting);

	//-----------------------------------------------------------------
	// Callbacks
	//-----------------------------------------------------------------
	virtual void LogMessageCallback(const string& pMessage, int nLength);
	virtual void SetLogObserverType();

}; // End ConsoleControl

/** @} */

#endif // __CONSOLE_CONTROL_H__

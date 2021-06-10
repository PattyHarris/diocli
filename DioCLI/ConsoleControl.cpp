/*********************************************************************
 * 
 *  file:  ConsoleControl.cpp
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

//! \defgroup consolecontrol ConsoleControl
// Handles all console input and output.
//! @{

#include "stdafx.h"
#include "resource.h"
#include "../Include/types.h"

#include "ConsoleControl.h"
#include "../CPPSDK.Lib/IDiomedeLib.h"
#include "../CPPSDK.Lib/ServiceAttribs.h"

#include "../Util/ClientLogUtils.h"
#include "../Include/ErrorCodes/UIErrors.h"
#include "../Include/BuildVersionUtils.h"

#include "../Util/Util.h"
#include "../Util/UserProfileData.h"
#include "../Util/ProfileManager.h"
#include "../Util/StringUtil.h"
#include "../Util/MessageTimer.h"

#include "CommandDefs.h"
#include "DiomedePEM.h"
#include "BillingDefs.h"

#include "SimpleRedirect.h"
#include "ResumeManager.h"
#include "ResumeInfoData.h"

#include <iostream>
#include <fstream>

#include "../gSoap/soapStub.h"
#include "../Util/XString.h"

#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#if 0
#if !defined(__CYGWIN__) && !defined(WIN32)
#include "boost/detail/algorithm.hpp"
#endif
#endif

#include "openssl/md5.h"

#if defined(linux)
#include <unistd.h>
#endif

#ifndef WIN32
#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>

// For linked in Diomede.pem file
#if defined(linux)
    extern char _binary_Diomede_pem_start;
    extern char _binary_Diomede_pem_end;
    extern char _binary_Diomede_pem_size;
#elif defined(__APPLE__)
    char _binary_Diomede_pem_start;
    char _binary_Diomede_pem_end;
    char _binary_Diomede_pem_size;
#endif
#endif

//! \name Color code constants for color coded text
//!@{
#define COLOR_ORIGINAL      0
#define COLOR_DELETED       1
#define COLOR_INCOMPLETE    2
#define COLOR_BANNER        3
//!@}

//! Session expires or invalid session
//! retry limit.
#define MAX_LOGIN_RETRIES   1

using namespace DIOMEDE_CONSOLE;
using namespace EndTimerTypes;
using namespace StringUtil;
using namespace DiomedeResumeErrorCodes;

using namespace DIOMEDE_PEM;

using namespace boost::algorithm;
using namespace boost::posix_time;
using namespace std;

///////////////////////////////////////////////////////////////////////
//! \name Globals
//!@{

//! \var g_bUsingCtrlKey
//! \brief Control key has been entered - current
//! command is stopped.
//! True if a session expired error has occurred.
static bool g_bUsingCtrlKey = false;

//! \var g_bSessionError
//! \brief Session expired global.
//! True if a session expired error has occurred.
static bool g_bSessionError = false;

//! \var g_nSessionErrorType
//! \brief Session expired global.
//! Diomede redefined session expired types.
static int g_nSessionErrorType = -1;

//! \var g_nSessionRetries
//! \brief Session expired global.
//! Number retries to retry the login process when the session has
//! expired.
static int g_nSessionRetries = MAX_LOGIN_RETRIES;

//! \var g_bCanContinueTimer
//! \brief Upload timer flag used to allow the timing to continue
//! as part of the callback handled from upload, download.
static bool g_bCanContinueTimer = false;

//! \var ConsoleControl::ServiceErrorStrings
//! \brief Possible service errors.
const std::string ConsoleControl::ServiceErrorStrings[] = {
        _T("session expired"),
        _T("invalid session"),
        _T("no data"),
        _T("host not found"),
        _T("timeout"),
        _T("unreachable host"),
        _T("unreachable network"),
        _T("network is unreachable"),
        _T("end of file or no input"),
        _T("an error has occurred"),
        _T("")
};

//! \var g_mdSourceFullPath
//! \brief Metadata name for the uploaded file's full path value.
const std::string g_mdSourceFullPath        = _T("SourceFullPath");

//! \var g_mdSourceRelativePath
//! \brief Metadata name for the uploaded file's relative path value.
const std::string g_mdSourceRelativePath    = _T("SourceRelativePath");


//! \var g_szTaskFriendlyName
//! \brief Friendly name used for status and error messages.
std::string g_szTaskFriendlyName            = _T("");

//!@}

///////////////////////////////////////////////////////////////////////
//! ConsoleControl

///////////////////////////////////////////////////////////////////////
//! ConsoleControl Constructor
ConsoleControl::ConsoleControl()
{
	m_bConnected = false;

    m_bRedirectedInput = false;
	m_bContinueProcessing = false;
	m_bMaskInput = false;
	m_szCommandPrompt = _T("Diomede> ");
	m_bSysCommandInput = false;

	m_pCurrentCommand = NULL;
	m_nCurrentNumArgs = 0;
    m_currentCmdID = DioCLICommands::CMD_NULL;

	m_bIncompleteCommand = false;
	m_bIgnoreCommandInput = false;

	m_bEchoOn = true;
	m_bAutoLoginOn = false;
	m_bAutoLogoutOff = false;
	m_bAutoCheckAccountOn = false;
	m_nSystemSleep = 0;

	m_szUsername = _T("");
	m_szClientData = _T("");
	m_szPlainTextPassword = _T("");

	m_szSessionToken = _T("");
	m_bValidCertificate = false;
	m_lWin32LastError = 0;
	m_nLastError = 0;

	m_bEnableLogging = false;
    m_bEnableStdLogging = false;
	m_szAppVersion = _T("");

	m_l64TotalUploadedBytes = 0;
	m_l64TotalUploadMilliseconds = 0;
	m_tdTotalUpload = not_a_date_time;

	m_pUploadInfo = NULL;
	m_pTaskCreateFile = NULL;
	m_pTaskSetFileMetaData = NULL;
    m_pTaskUpload = NULL;

	m_pDownloadInfo = NULL;
	m_pDisplayFileInfo = NULL;
	m_pDisplayFileEnumInfo = NULL;
	m_pFileEnumerator = NULL;
	
	m_pListStorageTypes = NULL;
}


///////////////////////////////////////////////////////////////////////
//! ConsoleControl Destructor
ConsoleControl::~ConsoleControl()
{
    // Cleanup the list of commands - pairs of CmdLine and command IDs.
	for (CommandMap::iterator iter = m_listCommands.begin(); iter != m_listCommands.end(); iter++)
    {
		CmdLine *pCmdLine = (*iter).second;
		delete pCmdLine;
    }

    m_listCommands.clear();

    if (m_pUploadInfo != NULL) {
        delete m_pUploadInfo;
        m_pUploadInfo = NULL;
    }

    if (m_pTaskCreateFile != NULL) {
        delete m_pTaskCreateFile;
        m_pTaskCreateFile = NULL;
    }

    if (m_pTaskSetFileMetaData != NULL) {
        delete m_pTaskSetFileMetaData;
        m_pTaskSetFileMetaData = NULL;
    }

    if (m_pTaskUpload != NULL) {
        delete m_pTaskUpload;
        m_pTaskUpload = NULL;
    }

    if (m_pDownloadInfo != NULL) {
        delete m_pDownloadInfo;
        m_pDownloadInfo = NULL;
    }

    if (m_pDisplayFileInfo != NULL) {
        delete m_pDisplayFileInfo;
        m_pDisplayFileInfo = NULL;
    }

    if (m_pDisplayFileEnumInfo != NULL) {
        delete m_pDisplayFileEnumInfo;
        m_pDisplayFileEnumInfo = NULL;
    }

    if (m_pFileEnumerator != NULL) {
        delete m_pFileEnumerator;
        m_pFileEnumerator = NULL;
    }

    if (m_pListStorageTypes != NULL) {
        delete m_pListStorageTypes;
        m_pListStorageTypes = NULL;
    }

    SimpleRedirect::Instance()->Shutdown();
}

///////////////////////////////////////////////////////////////////////
// ConsoleControl Protected Methods

///////////////////////////////////////////////////////////////////////
// ConsoleControl Private Methods

//! \var MAX_LINE_LENGTH
//! \brief Max size of the command line length
const UINT MAX_LINE_LENGTH = 52;

//! \var MAX_VERBOSE_PAGE_SIZE
//! \brief Default size of a verbose listing.
const UINT MAX_VERBOSE_PAGE_SIZE = 50;

///////////////////////////////////////////////////////////////////////
// Purpose: Output the text in the given color attribute.
// Requires:
//      szOutputText: the text display.
//      nColor: color constant for the text.
// Returns: nothing
void ConsoleControl::PrintTextInColor(std::string szOutputText, int nColor)
{
#ifdef _WIN32
    //-----------------------------------------------------------------
    //  Black       = 0,
    //  b           = FOREGROUND_INTENSITY,
    //  LightGrey   = FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE,
    //  White       = FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    //  Blue        = FOREGROUND_BLUE,
    //  Green       = FOREGROUND_GREEN,
    //  Cyan        = FOREGROUND_GREEN | FOREGROUND_BLUE,
    //  Red         = FOREGROUND_RED,
    //  Purple      = FOREGROUND_RED   | FOREGROUND_BLUE,
    //  LightBlue   = FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
    //  LightGreen  = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    //  LightCyan   = FOREGROUND_GREEN | FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
    //  LightRed    = FOREGROUND_RED   | FOREGROUND_INTENSITY,
    //  LightPurple = FOREGROUND_RED   | FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
    //  Orange      = FOREGROUND_RED   | FOREGROUND_GREEN,
    //  Yellow      = FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    //-----------------------------------------------------------------

    unsigned short nColorAttribute = 0;
    switch (nColor) {
        case COLOR_DELETED:
            nColorAttribute = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        case COLOR_INCOMPLETE:
            nColorAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case COLOR_BANNER:
            nColorAttribute = FOREGROUND_INTENSITY;
            break;
        default:
            break;
    }

    if (nColorAttribute == 0) {
        // Shouldn't happen.
        return;
    }

    HANDLE hConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Get the current color
    CONSOLE_SCREEN_BUFFER_INFO *pConsoleInfo = new CONSOLE_SCREEN_BUFFER_INFO();
    GetConsoleScreenBufferInfo(hConsoleHandle, pConsoleInfo);
    unsigned short nOriginalColors = pConsoleInfo->wAttributes;

    // Set the new color and output the text.
    SetConsoleTextAttribute(hConsoleHandle,  nColorAttribute);
    _tprintf(_T("%s"), szOutputText.c_str());

    // Reset the color back to the original color
    SetConsoleTextAttribute(hConsoleHandle, nOriginalColors);
#else
    //-----------------------------------------------------------------
    // Color codes for Linux
    //  \033[22;31m - red
    //  \033[22;32m - green
    //  \033[22;33m - brown
    //  \033[22;34m - blue
    //  \033[22;35m - magenta
    //  \033[22;36m - cyan
    //  \033[22;37m - gray
    //  \033[01;30m - dark gray
    //  \033[01;31m - light red
    //  \033[01;32m - light green
    //  \033[01;33m - yellow
    //  \033[01;34m - light blue
    //  \033[01;35m - light magenta
    //  \033[01;36m - light cyan
    //  \033[01;37m - white
    //-----------------------------------------------------------------
    switch (nColor) {
        case COLOR_DELETED:
    	    _tprintf(_T("\033[22;31m"));
             break;
        case COLOR_INCOMPLETE:
    	    _tprintf(_T("\033[22;33m"));
             break;
        case COLOR_BANNER:
        {
            // Turn off bold.
            char szESC = 27;
            _tprintf(_T("%c[0m"), szESC);
            break;
        }
        default:
             break;
    }

    _tprintf(_T("%s"), szOutputText.c_str());
    _tprintf(_T("\033[22;0m"));
#endif

} // End PrintTextInColor

///////////////////////////////////////////////////////////////////////
// Purpose: Set the console's output text color to the given
//          attribute color.
// Requires:
//      nColor: color constant for the text
//      nOriginalColors: will contain the original text attributes
// Returns: nothing
void ConsoleControl::PrintTextInColorOn(int nColor, unsigned short& nOriginalColors)
{
#ifdef _WIN32
    unsigned short nColorAttribute = 0;
    switch (nColor) {
        case COLOR_DELETED:
             nColorAttribute = FOREGROUND_RED | FOREGROUND_INTENSITY;
             break;
        case COLOR_INCOMPLETE:
             nColorAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
             break;
        case COLOR_BANNER:
            nColorAttribute = FOREGROUND_INTENSITY;
            break;
        default:
             break;
    }

    if (nColorAttribute == 0) {
        // Shouldn't happen.
        return;
    }

    HANDLE hConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Get the current color
    CONSOLE_SCREEN_BUFFER_INFO *pConsoleInfo = new CONSOLE_SCREEN_BUFFER_INFO();
    GetConsoleScreenBufferInfo(hConsoleHandle, pConsoleInfo);
    nOriginalColors = pConsoleInfo->wAttributes;

    // Set the new color and output the text.
    if (nColor == COLOR_BANNER) {
        SetConsoleTextAttribute(hConsoleHandle, nOriginalColors | nColorAttribute);
    }
    else {
        SetConsoleTextAttribute(hConsoleHandle, nColorAttribute);
    }
#else
    // For Linux, 0 is used to reset the color change.
    switch (nColor) {
        case COLOR_DELETED:
    	    _tprintf(_T("\033[22;31m"));
             nOriginalColors = 0;
             break;
        case COLOR_INCOMPLETE:
    	    _tprintf(_T("\033[22;33m"));
             nOriginalColors = 0;
             break;
        case COLOR_BANNER:
        {
            // Leave color of text intact - yellow is too hard to
            // see. We'll just bold the text.
    	    // _tprintf(_T("\033[22;33m"));
            char szESC = 27;
            _tprintf(_T("%c[1m"), szESC);
            nOriginalColors = 0;
            break;
        }
        default:
             break;
    }

#endif

} // End PrintTextInColorOn

///////////////////////////////////////////////////////////////////////
// Purpose: Resets the console's output text color to its original
//          attribute color.
// Requires:
//      nColorAttribute: color for the text
//      nOriginalColors: will contain the original text attributes
// Returns: nothing
void ConsoleControl::PrintTextInColorOff(unsigned short nOriginalColors)
{
#ifdef _WIN32

    HANDLE hConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Get the current color
    CONSOLE_SCREEN_BUFFER_INFO *pConsoleInfo = new CONSOLE_SCREEN_BUFFER_INFO();
    GetConsoleScreenBufferInfo(hConsoleHandle, pConsoleInfo);

    // Reset the color back to the original color
    SetConsoleTextAttribute(hConsoleHandle, nOriginalColors);
#else
    // 0 is used to reset the color changes.
	_tprintf(_T("\033[22;0m"));
#endif

} // End PrintTextInColorOff

///////////////////////////////////////////////////////////////////////
// Purpose: Show the command menu
// Requires: nothing
// Returns: nothing
void ConsoleControl::ShowCommandsMenu()
{
	unsigned int nGroupIndex = 0;
	unsigned int nCommandIndex = 0;
	std::string szFormattedOut;

	while ( DioCLIGroupList[nGroupIndex].m_szGroupDesc != GROUP_LAST ) {

	    if ( DioCLIGroupList[nGroupIndex].m_bDisplayGroup == false ) {

	        while (DioCLICommandList[nCommandIndex].m_groupID == DioCLIGroupList[nGroupIndex].m_groupID )
	        {
	            // Move over the commands in this group.
	            nCommandIndex ++;
	        }

	        nGroupIndex ++;
	        continue;
	    }

        // Display the group header.
    	PrintNewLine();
        m_stdOut.spacePrint(std::cout, DioCLIGroupList[nGroupIndex].m_szGroupDesc, 72, 1, 1);

	    //----------------------------------------------------------------
	    // For efficiency, the commands are already sorted in the correct
	    // groupings.
	    //----------------------------------------------------------------
	    while ( DioCLICommandList[nCommandIndex].m_szCommand != CMD_LAST ) {

	        if (DioCLICommandList[nCommandIndex].m_groupID !=  DioCLIGroupList[nGroupIndex].m_groupID) {
	            // Bounce out to show the next group.
	            break;
	        }

		    DioCLICommands::COMMAND_ID cmdID = DioCLICommandList[nCommandIndex].m_commandID;

		    CmdLine* pCmdLine = NULL;
            CommandMap::iterator cmdIter = m_listCommands.find(cmdID);

            if (cmdIter != m_listCommands.end()) {
                // Show only those commands currently supported.
                pCmdLine = (*cmdIter).second;

                char szCommand[100];
	            strcpy(szCommand, DioCLICommandList[nCommandIndex].m_szCommand.c_str());

                _tcsupr(szCommand);

                // Doesn't seem to be a way line up the message text other
                // than breaking it up into words, and then putting them
                // back together in appropriate lengths.

                if (pCmdLine->getMessage().length() > MAX_LINE_LENGTH) {

                    std::string szMessage = _T("");
                    std::string szTmpMessage = _T("");
                    bool bFirstLine = true;

                    std::vector<std::string> msgItems;

    	            unsigned int nCount = SplitString(pCmdLine->getMessage(), _T(" "), msgItems, false);

    	            for (int nIndex = 0; nIndex < (int)nCount; nIndex ++) {
    	                szTmpMessage += msgItems[nIndex] + _T(" ");
    	                if (szTmpMessage.length() < MAX_LINE_LENGTH) {
    	                    szMessage = szTmpMessage;
    	                }
    	                else {
    	                    if (bFirstLine) {
                                _tprintf(_T(" %-26s%-52s\n"),
                                    szCommand, szMessage.c_str() );
                                bFirstLine = false;
                            }
                            else {
                                _tprintf(_T(" %-26s%-52s\n"), _T(""), szMessage.c_str());
                            }

                            szTmpMessage = msgItems[nIndex];
                            szMessage = _T("");
                        }

                    }

                    // Take care of the last segment
                    if (szTmpMessage.length() > 0) {
                        _tprintf(_T(" %-26s%-52s\n"), _T(""), szTmpMessage.c_str());
                    }

                }
                else {
                    _tprintf(_T(" %-26s%-52s\n"),
                        szCommand, pCmdLine->getMessage().c_str());
                }

                /*
		        cout << DioCLICommandList[nCommandIndex].m_szCommand << _T("\t\t")
		             << pCmdLine->getMessage() << _T("\n");
		        */
            }

		    nCommandIndex++;
	    }

	    nGroupIndex ++;
    }

    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

} // End ShowCommandsMenu

///////////////////////////////////////////////////////////////////////
// Purpose: Show the current configuation settings
// Requires: nothing
// Returns: nothing
void ConsoleControl::ShowConfigSettings()
{
    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );

    if (pProfileData == NULL) {
        PrintStatusMsg(_T("Error: Couldn't access user configuration data."));
        ClientLog(UI_COMP, LOG_ERROR, false, _T("Couldn't create user profile object"));
	    return;
	}

    std::string szYes = _T("yes");
    std::string szNo = _T("no");
    std::string szOn = _T("on");
    std::string szOff = _T("off");

    bool bConfig = (pProfileData->GetUserProfileInt(GEN_AUTO_LOGIN, GEN_AUTO_LOGIN_DF) != 0);
    std::string szSetting = bConfig ? _T("on") : _T("off");
    std::cout << "            Auto Login: " << szSetting << std::endl;

    if (bConfig) {
        szSetting = pProfileData->GetUserProfileStr(UI_LASTLOGIN);
        std::cout << "   Auto Login username: " << szSetting << std::endl;
    }

    bConfig = (pProfileData->GetUserProfileInt(GEN_AUTO_LOGOUT, GEN_AUTO_LOGOUT_DF) == 0);
    szSetting = bConfig ? _T("off") : _T("on");
    std::cout << "           Auto Logout: " << szSetting << std::endl;

    bConfig = (pProfileData->GetUserProfileInt(GEN_AUTO_CHECK_ACCOUNT, GEN_AUTO_CHECK_ACCOUNT_DF) != 0);
    szSetting = bConfig ? _T("on") : _T("off");
    std::cout << "    Auto check account: " << szSetting << std::endl;

    #ifdef NO_ENCRYPTED_USER_PROFILE_KEYS
        std::cout << "            Encryption: off" << std::endl;
    #else
        std::cout << "            Encryption: on" << std::endl;
    #endif

    int nSecurityType = pProfileData->GetUserProfileInt(GEN_SERVICE_SECURE_TYPE,
                                                        GEN_SERVICE_SECURE_TYPE_DF);
    if (nSecurityType == SECURE_SERVICE_PARTIAL) {
        std::cout << "              SSL mode: " << _T("default") << std::endl;
    }
    else if (nSecurityType == SECURE_SERVICE_ALL) {
        std::cout << "              SSL mode: " << _T("all") << std::endl;
    }
    else if (nSecurityType == SECURE_SERVICE_NONE) {
        std::cout << "              SSL mode: " << _T("none") << std::endl;
    }

    // The endpoints wrap - so we'll get the longest string, the HTTPS transfer endpoint,
    // and pad the strings according to it's length:
    szSetting = pProfileData->GetUserProfileStr(GEN_SECURE_TRANSFER_ENDPOINT, GEN_SECURE_TRANSFER_ENDPOINT_DF);
    std::string szPad = GetPadStr(MAX_LINE_LEN - szSetting.length() - 1);

    szSetting = pProfileData->GetUserProfileStr(GEN_SERVICE_ENDPOINT, GEN_SERVICE_ENDPOINT_DF);
    if (szSetting.length() > 0) {
        std::cout << "      Service endpoint: " << std::endl
                  << szPad << szSetting << std::endl;
    }

    szSetting = pProfileData->GetUserProfileStr(GEN_SECURE_SERVICE_ENDPOINT, GEN_SECURE_SERVICE_ENDPOINT_DF);
    if (szSetting.length() > 0) {
        std::cout << "  SSL service endpoint: " << std::endl
                  << szPad << szSetting << std::endl;
    }

    szSetting = pProfileData->GetUserProfileStr(GEN_TRANSFER_ENDPOINT, GEN_TRANSFER_ENDPOINT_DF);
    if (szSetting.length() > 0) {
        std::cout << "     Transfer endpoint: " << std::endl
                  << szPad << szSetting << std::endl;
    }

    szSetting = pProfileData->GetUserProfileStr(GEN_SECURE_TRANSFER_ENDPOINT, GEN_SECURE_TRANSFER_ENDPOINT_DF);
    if (szSetting.length() > 0) {
        std::cout << " SSL transfer endpoint: " << std::endl
                  << szPad << szSetting << std::endl;
    }

    szSetting = pProfileData->GetUserProfileStr(GEN_RESULT_PAGE_SIZE, GEN_RESULT_PAGE_SIZE_DF);
    if (szSetting.length() > 0) {
        std::cout << "      Result page size: " << szSetting << std::endl;
    }

    szSetting = pProfileData->GetUserProfileStr(GEN_RESULT_PAGE, GEN_RESULT_PAGE_DF);
    if (szSetting.length() > 0) {
        std::cout << "           Result page: " << szSetting << std::endl;
    }

    #ifdef WIN32
        bConfig = (pProfileData->GetUserProfileInt(GEN_COPY_TO_CLIPBOARD, GEN_COPY_TO_CLIPBOARD_DF) != 0);
        szSetting = bConfig ? _T("on") : _T("off");
        std::cout << " Copy URL to clipboard: " << szSetting << std::endl;
    #endif

    bConfig = (pProfileData->GetUserProfileInt(GEN_VERBOSE_OUTPUT, GEN_VERBOSE_OUTPUT_DF) != 0);
    szSetting = bConfig ? _T("on") : _T("off");
    std::cout << "        Verbose output: " << szSetting << std::endl;

    #if 0
    // Currently not available as a setting
    bConfig = (pProfileData->GetUserProfileInt(UI_REMEMBER_PWD, 0) != 0);
    szParam = bConfig ? _T("on") : _T("off");
    std::cout << std::endl << "Remember password is " << szParam << std::endl;
    #endif

} // End ShowConfigSettings

///////////////////////////////////////////////////////////////////////
// Purpose: Verify whether this command is allowed from the system
//          command prompt (e.g. logout is not allowed).
// Requires:
//      szCmdName: command name
// Returns: true if a valid command, false otherwise.
bool ConsoleControl::VerifySystemCommand(std::string szCmdName)
{
    // Currently, all commands are allowed....
    return true;

} // End VerifySystemCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Handle cleanup following an unknown command error
// Requires:
//      szCmdName: command name
//      e: exception
// Returns: nothing
void ConsoleControl::UnknownCommandError(std::string szCmdName, ArgException& e)
{
    m_stdOut.unknownCommand(m_actionStack.front(), e);
    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

    m_actionStack.pop_front();

    m_bIncompleteCommand = false;

    SetCommandPrompt();

} // End UnknownCommandError

///////////////////////////////////////////////////////////////////////
// Purpose: Handle cleanup following a command parse error
// Requires:
//      pCmdLine: pointer to the last command
//      e: exception
// Returns: nothing
void ConsoleControl::ParseCommandError(CmdLine* pCmdLine, ArgException& e)
{
    std::string szErrorMsg = e.error() + _T(" for arg ") + e.argId();
    ClientLog(UI_COMP, LOG_ERROR, false, _T("Command parse error: %s"), szErrorMsg.c_str());

    if (pCmdLine != NULL) {
        pCmdLine->resetArgs();
        pCmdLine->resetValues();
    }

    m_actionStack.pop_front();

    m_bIncompleteCommand = false;

    SetCommandPrompt();

} // End ParseCommandError

///////////////////////////////////////////////////////////////////////
// Purpose: Prints command completion message.
// Requires:
//      szCompletionMsg: message text
//      bPrintNewLineBefore: adds a new line before the text
//      bPrintNewLineBefore: adds a new line after the text
//      bAllowLineWrap: allow the line to wrap, that is, assume
//                      the line is formatted properly.
// Returns: nothing
void ConsoleControl::PrintStatusMsg(std::string szCompletionMsg,
    bool bPrintNewLineBefore /*false*/, bool bPrintNewLineAfter /*true*/,
    bool bAllowLineWrap /*false*/)
{
    if (bPrintNewLineBefore) {
        PrintNewLine();
    }

    std::string szOutMsg = szCompletionMsg;

    // There's really no good way to truncate the message - we'll assume that the
    // end of the message is most important (generally indicagtes success or
    // failure) and truncate somewhere in the middle: 0-20 + ... + 56-79
    if (bAllowLineWrap == false) {
        if (szOutMsg.length() > MAX_LINE_LEN) {
            int nDiffLength = (int)szOutMsg.length() - MAX_LINE_LEN;
            szOutMsg = szCompletionMsg.substr(0, 20) + _T("...") +
                       szCompletionMsg.substr(20 + nDiffLength + 3);
        }
    }

    _tprintf(_T("%s\n\r"), szOutMsg.c_str());

    if (!m_bSysCommandInput && bPrintNewLineAfter) {
        PrintNewLine();
    }

} // End PrintStatusMsg

///////////////////////////////////////////////////////////////////////
// Purpose: Prints the error returned from the service.
//          Checks if the return is "Session expired" and updates
//          the command prompt.
// Requires:
//      soap: number of arguments received
//      fd: file descriptor - typically stderr
//      szErrorMsg: optional message to preface the error string
//      bPrintNewLineBefore: adds a new line before the text
//      bPrintNewLineBefore: adds a new line after the text
// Returns: nothing
void ConsoleControl::PrintSoapError(struct soap* soap, FILE* fd, std::string szErrorMsg /*_T("")*/,
                                    bool bPrintNewLineBefore /*false*/,
                                    bool bPrintNewLineAfter /*true*/)
{
    // In debug mode, print out all the details.  Otherwise,
    // just print the error message returned.
    const char *szError = *soap_faultstring(soap);
    std::string szSoapErrorMsg = _T("");

    if ( (szError != NULL) && (sizeof(szError) > 0)) {
        szSoapErrorMsg = std::string(szError);
    }
    else {
        // No reason given, so try to get the detail.
        szSoapErrorMsg = _T("no reason");

        const char **szDetailError = soap_faultcode(soap);
        if (!*szDetailError) {
            soap_set_fault(soap);
        }

        szError = *soap_faultstring(soap);

        if ( (szError) && (sizeof(szError) > 0)) {
            szSoapErrorMsg = std::string(szError);
        }
        else {
	        szSoapErrorMsg = _T("no detail");
	    }
    }

    std::string szOutErrorMsg = _T("Error");
    if (szErrorMsg.length() > 0) {
        szOutErrorMsg = szErrorMsg;
    }

    if (bPrintNewLineBefore) {
        PrintNewLine();
    }

    #if 0
	    soap_print_fault(soap, fd);
    #else
	    fprintf(fd, _T("%s: %s\n\r"), szOutErrorMsg.c_str(), szSoapErrorMsg.c_str());
    #endif

    if (bPrintNewLineAfter) {
        PrintNewLine();
    }

    // We won't bother attempting to log out since our session token
    // is invalid anyway.
    std::string szFriendlyMsg = _T("");
    bool bSessionExpired = CheckServiceErrorToRetry(szSoapErrorMsg, szFriendlyMsg);
    if (bSessionExpired) {
	    ResetLoginData();
	    SetCommandPrompt();
   }

} // End PrintSoapError

///////////////////////////////////////////////////////////////////////
// Purpose: Prints the error returned from the service.
//          Checks if the error is "Session expired" and updates
//          the command prompt.
// Requires:
//      fd: file descriptor - typically stderr
//      szErrorMsg: service error message.
//      bPrintNewLineBefore: adds a new line before the text
//      bPrintNewLineBefore: adds a new line after the text
//      bRepeatCommand: If the error is due to an expired
//                      session, login the user and
//                      repeat the last command.
// Returns: nothing
void ConsoleControl::PrintServiceError(FILE* fd, std::string szErrorMsg,
                                       bool bPrintNewLineBefore /*false*/,
                                       bool bPrintNewLineAfter /*true*/,
                                       bool bRepeatCommand /*true*/)
{
    // Print the error
    if (bPrintNewLineBefore) {
        PrintNewLine();
    }

    if (szErrorMsg.length() == 0) {
        szErrorMsg = _T("An unknown service error has occurred.");
    }

    //-----------------------------------------------------------------
    // Check service errors for a friendlier output and whether the
    // message is of the type in which the user may need to re-login or
    // reconnect.
    //-----------------------------------------------------------------
    char* szMsgBuffer = new char[szErrorMsg.length() + 1];
    strcpy(szMsgBuffer, szErrorMsg.c_str());

    // For comparison, convert to lower case
    _strlwr(szMsgBuffer);
    std::string szTempErrorMsg = std::string(szMsgBuffer);

    std::string szFriendlyMsg = _T("");
    bool bResetPrompt = CheckServiceErrorToRetry(szTempErrorMsg, szFriendlyMsg);

    // Get our error string again to keep the case correct for output.
    szTempErrorMsg = szErrorMsg;

    if (bResetPrompt) {

        // Look for the last semi-colon, replacing the last bit of text
        // with the friendlier message.
	    int nSemiColonPos = (int)szTempErrorMsg.find_last_of(_T(':'));
	    if (nSemiColonPos > 0) {
	        szTempErrorMsg = szTempErrorMsg.substr(0, nSemiColonPos + 2) + szFriendlyMsg;
	    }
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    fprintf(fd, _T("%s\n\r"), szTempErrorMsg.c_str());

    if (!m_bSysCommandInput && bPrintNewLineAfter) {
        PrintNewLine();
    }

    //-----------------------------------------------------------------
    // If we get here and still encounter a session expired - the
    // service is probably messed up.  We should have tried
    // to repeat the last task already.
    // We won't bother attempting to log out since our session token
    // is invalid anyway.
    //-----------------------------------------------------------------

    g_bSessionError = false;
    delete[] szMsgBuffer;

    if ( bResetPrompt) {
	    g_bSessionError = true;
	    ResetLoginData();
	    SetCommandPrompt();
   }

} // End PrintServiceError

///////////////////////////////////////////////////////////////////////
// Purpose: Prints the error returned from the Resume Manager.
// Requires:
//      nResumeResult: resume error code
//      szErrorMsg: optional message to preface the error string
//      bPrintNewLineBefore: adds a new line before the text
//      bPrintNewLineBefore: adds a new line after the text
// Returns: nothing
void ConsoleControl::PrintResumeError(int nResumeResult, std::string szErrorMsg,
                                       bool bPrintNewLineBefore /*true*/,
                                       bool bPrintNewLineAfter /*false*/)
{
    std::string szOutErrorMsg = _T("Error");
    if (szErrorMsg.length() > 0) {
        szOutErrorMsg = szErrorMsg;
    }

    std::string szResumeErrorMsg = _T("");

    if (nResumeResult == RESUME_SYSTEM_IO_ERROR) {
        m_nLastError = ResumeManager::Instance()->GetLastError();
        szResumeErrorMsg = GetErrorString();
    }
    else {
        szResumeErrorMsg = DiomedeResumeErrorCodes::GetResumeError(nResumeResult);
    }
    std::string szTmpResumeErrorMsg =
        _format(_T("%s: %s\n\r"), szOutErrorMsg.c_str(), szResumeErrorMsg.c_str());
    PrintStatusMsg(szTmpResumeErrorMsg, bPrintNewLineBefore, bPrintNewLineAfter);

    ResetErrorCodes();

} // End PrintResumeError

///////////////////////////////////////////////////////////////////////
// Purpose: Prints the warning returned from the Resume Manager.
// Requires:
//      nResumeResult: resume error code
//      szWarningMsg: optional message to preface the warning string
//      bPrintNewLineBefore: adds a new line before the text
//      bPrintNewLineBefore: adds a new line after the text
// Returns: nothing
void ConsoleControl::PrintResumeWarning(int nResumeResult, std::string szWarningMsg,
                                       bool bPrintNewLineBefore /*true*/,
                                       bool bPrintNewLineAfter /*false*/)
{
    std::string szOutWarningMsg = _T("Warning");
    if (szWarningMsg.length() > 0) {
        szOutWarningMsg = szWarningMsg;
    }

    std::string szResumeWarningMsg = _T("");

    if (nResumeResult == RESUME_SYSTEM_IO_ERROR) {
        m_nLastError = ResumeManager::Instance()->GetLastError();
        szResumeWarningMsg = GetErrorString();
    }
    else {
        szResumeWarningMsg = DiomedeResumeErrorCodes::GetResumeError(nResumeResult);
    }
    std::string szTmpResumeWarningMsg =
        _format(_T("%s: %s\n\r"), szOutWarningMsg.c_str(), szResumeWarningMsg.c_str());
    PrintStatusMsg(szTmpResumeWarningMsg, bPrintNewLineBefore, bPrintNewLineAfter);

    ResetErrorCodes();

} // End PrintResumeWarning

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the whether the error is
//          of the types that allows us to retry.
// Requires:
//      szErrorMsg: error message.
//      szFriendlyErrorMsg: some errors are translated to a more
//                          friendly format.
// Returns: true if retry is allowed, false otherwise.
bool ConsoleControl::CheckServiceErrorToRetry(std::string szErrorMsg, std::string& szFriendlyErrorMsg)
{
    bool bCanRetry = false;

    int nIndex = 0;

    g_bSessionError = false;
    g_nSessionErrorType = -1;

    // Convert the message to lower case for comparison.
    char szMsgBuffer[MAX_PATH];
    strcpy(szMsgBuffer, szErrorMsg.c_str());

    _strlwr(szMsgBuffer);
    std::string szTempErrorMsg = std::string(szMsgBuffer);

    while ( ServiceErrorStrings[nIndex].length() > 0 ) {

        if ( szTempErrorMsg.find(ServiceErrorStrings[nIndex]) != (size_t)-1 ) {
            bCanRetry = true;
            break;
        }

        nIndex ++;
    }

    if (bCanRetry) {
        g_nSessionErrorType = nIndex;
        switch (nIndex) {
            case ConsoleControl::SESSION_TOKEN_EXPIRES:
                szFriendlyErrorMsg = _T("Session expired.  Please re-login.");
                break;
            case ConsoleControl::INVALID_SESSION_TOKEN:
                szFriendlyErrorMsg = _T("Session expired.  Please re-login.");
                break;
            case ConsoleControl::GSOAP_NO_DATA:
                szFriendlyErrorMsg = _T("Connection dropped.");
                break;
            case ConsoleControl::GSOAP_HOST_NOT_FOUND:
                szFriendlyErrorMsg = _T("Connection dropped.");
                break;
            case ConsoleControl::GSOAP_TCP_ERROR:
                szFriendlyErrorMsg = _T("Connection dropped.");
                break;
            case ConsoleControl::WSA_HOST_UNREACHABLE:
                szFriendlyErrorMsg = _T("Connection dropped.");
                break;
            case ConsoleControl::GENERIC_NETWORK_IS_UNREACHABLE:
                szFriendlyErrorMsg = _T("Connection dropped.");
                break;
            case ConsoleControl::GSOAP_EOF:
                szFriendlyErrorMsg = _T("Connection dropped.");
                break;
            case ConsoleControl::GENERIC_SERVICE:
                szFriendlyErrorMsg = _T("\n   An error has occurred. Please try request again.");
                break;
            default:
                break;
        }
    }

    if ( g_nSessionErrorType >= 0) {
        ClientLog(UI_COMP, LOG_STATUS, false, _T("CheckServiceErrorToRetry returning message type (%d): %s"),
            g_nSessionErrorType, szFriendlyErrorMsg.c_str());
    }
    else {
        ClientLog(UI_COMP, LOG_STATUS, false, _T("CheckServiceErrorToRetry return false - couldn't match %s"),
            szErrorMsg.c_str() );
    }

    return bCanRetry;

} // End CheckServiceErrorToRetry

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the whether the error is
//          of the types that allows us to resume.
// Requires:
//      szErrorMsg: error message.
// Returns: true if resume can be attempted, false otherwise
bool ConsoleControl::CheckServiceErrorToResume(std::string szErrorMsg)
{
    bool bCanRetry = false;
    int nIndex = 0;

    g_bSessionError = false;
    g_nSessionErrorType = -1;

    // Convert the message to lower case for comparison.
    char szMsgBuffer[MAX_PATH];
    strcpy(szMsgBuffer, szErrorMsg.c_str());

    _strlwr(szMsgBuffer);
    std::string szTempErrorMsg = std::string(szMsgBuffer);

    while ( ServiceErrorStrings[nIndex].length() > 0 ) {

        if ( szTempErrorMsg.find(ServiceErrorStrings[nIndex]) != (size_t)-1 ) {
            bCanRetry = true;
            break;
        }

        nIndex ++;
    }

    bool bCanResume = false;

    // We've detected an error that allows retry, but resume should only
    // happen with connection errors, not session expired errors (?)
    if (bCanRetry) {
        g_nSessionErrorType = nIndex;
        switch (nIndex) {
            case ConsoleControl::SESSION_TOKEN_EXPIRES:
            case ConsoleControl::INVALID_SESSION_TOKEN:
            case ConsoleControl::GENERIC_SERVICE:
                break;
            case ConsoleControl::GSOAP_NO_DATA:
            case ConsoleControl::GSOAP_HOST_NOT_FOUND:
            case ConsoleControl::GSOAP_TCP_ERROR:
            case ConsoleControl::WSA_HOST_UNREACHABLE:
            case ConsoleControl::GENERIC_NETWORK_IS_UNREACHABLE:
            case ConsoleControl::GSOAP_EOF:
                bCanResume = true;
                break;
            default:
                break;
        }
    }

    if ( g_nSessionErrorType >= 0) {
        ClientLog(UI_COMP, LOG_STATUS, false, _T("CheckServiceErrorToResume returning message type: %d"),
            g_nSessionErrorType);
    }
    else {
        ClientLog(UI_COMP, LOG_STATUS, false, _T("CheckServiceErrorToResume return false - couldn't match %s"),
            szErrorMsg.c_str() );
    }

    return bCanResume;

} // End CheckServiceErrorToResume

///////////////////////////////////////////////////////////////////////
// Purpose: Verify the thread has been created and output
//          an appropriate status message on error.
// Requires:
//          pThread: reference to the thread in question
//          szThreadUsageType: user for this thread, e.g. upload
//              download, display file
// Returns: true if the thread is good, false otherwise.
bool ConsoleControl::CheckThread(CommandThread* pThread, std::string szThreadUsageType)
{
    if (pThread->GetErrorFlags() == 0) {
        return true;
    }

    std::string szStatusMsg = _T("");
    std::string szErrorMsg = _T("");
    std::string szSystemErrorMsg = _T("");

    #ifdef WIN32
        m_lWin32LastError = GetLastError();
        if (m_lWin32LastError > 0) {
            szSystemErrorMsg = GetErrorString();
        }
    #else
        m_nLastError = errno;
    #endif

    szErrorMsg = pThread->GetErrorString();
    if (szErrorMsg.length() > 0) {
        szStatusMsg = _format(_T("%s failed due to the following user's system error: %s"),
            szThreadUsageType.c_str(), szErrorMsg.c_str());
    }
    else {
        szStatusMsg = _format(_T("%s failed due to an unknown user's system error."),
            szThreadUsageType.c_str());
    }

    PrintStatusMsg(szStatusMsg);
    return false;

} // End CheckThread

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to update the upload resume data.
// Requires:
//      resumeUploadInfoData: resume upload data
//      szClientLogMsg: optional text used for logging purposes.
// Returns: result of resume data update
int ConsoleControl::WriteResumeUploadData(ResumeUploadInfoData& resumeUploadInfoData,
                                          std::string szClientLogMsg /*_T("")*/)
{
    int nResumeResult = ResumeManager::Instance()->WriteResumeUploadData(RESUME_UPLOAD_FILENAME,
                                                                         resumeUploadInfoData);
    if (nResumeResult != 0) {

        // If the file can't be locked, sleep and try again.
        if (nResumeResult == RESUME_FILE_LOCKING_ERROR) {
            PauseProcess();
            nResumeResult = ResumeManager::Instance()->WriteResumeUploadData(RESUME_UPLOAD_FILENAME,
                                                                             resumeUploadInfoData);
            if (nResumeResult != 0) {
                PrintResumeWarning(nResumeResult, _T("Failed to write resume data "));
            }

            // ClientLogs here to alert when the resume file(s) are locked and
            // access was denied.

            if (szClientLogMsg.length() > 0) {
                ClientLog(UI_COMP, LOG_WARNING, false,
                    _T("%s: resume write of upload data - retry on file locking error."),
                    szClientLogMsg.c_str());
            }
            else {
                ClientLog(UI_COMP, LOG_WARNING, false,
                    _T("Resume write of upload data - retry on file locking error."));
            }
        }
        else if (nResumeResult == RESUME_NO_MATCHES_FOUND) {
            PauseProcess();
            nResumeResult = ResumeManager::Instance()->WriteResumeUploadData(RESUME_UPLOAD_FILENAME,
                                                                             resumeUploadInfoData);
            if (nResumeResult != 0) {
                PrintResumeWarning(nResumeResult, _T("Failed to write resume data "));
            }

            // ClientLogs here to alert when the resume file(s) are locked and
            // access was denied.

            if (szClientLogMsg.length() > 0) {
                ClientLog(UI_COMP, LOG_WARNING, false,
                    _T("%s: resume write of upload data - retry on no matches found error."),
                    szClientLogMsg.c_str());
            }
            else {
                ClientLog(UI_COMP, LOG_WARNING, false,
                    _T("Resume write of upload data - retry on no matches found error."));
            }
        }
    }

    return nResumeResult;

} // End WriteResumeUploadData

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the copy to clipboard flag for the
//          given command.
// Requires:
//      pCmdLine: pointer to the current command
// Returns: true if the flag is set, false otherwise.
bool ConsoleControl::UseClipboard(CmdLine* pCmdLine)
{
    #ifndef WIN32
        return false;
    #endif

    DiomedeSwitchArg* pClipboardArg = NULL;
    try {
        pClipboardArg = (DiomedeSwitchArg*)pCmdLine->getArg(ARG_CLIPBOARD);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    bool bClipboard = false;

    if (pClipboardArg && pClipboardArg->isSet()) {
        bClipboard = true;
    }
    else {
        GetConfigClipboard(bClipboard);
    }

    return bClipboard;

} // End UseClipboard

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the verbose flag for the
//          given command.
// Requires:
//      pCmdLine: pointer to the current command
// Returns: true if the flag is set, false otherwise.
bool ConsoleControl::VerboseOutput(CmdLine* pCmdLine)
{
    DiomedeSwitchArg* pVerboseArg = NULL;
    try {
        pVerboseArg = (DiomedeSwitchArg*)pCmdLine->getArg(ARG_VERBOSE_SWITCH);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    bool bVerboseOutput = false;

    if (pVerboseArg && pVerboseArg->isSet()) {
        bVerboseOutput = true;
    }
    else {
        GetConfigVerbose(bVerboseOutput);
    }

    return bVerboseOutput;

} // End VerboseOutput

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the config settings for
//          the copy URL to clipboard value.
// Requires:
//      bCopyToClipboard: returned clipboard settting
// Returns: true if successful, false otherise.
bool ConsoleControl::GetConfigClipboard(bool& bCopyToClipboard)
{
    #ifndef WIN32
        bCopyToClipboard = false;
        return false;
    #endif

    bCopyToClipboard = false;
    bool bSuccess = false;

    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );

    if (pProfileData) {
        bCopyToClipboard = (pProfileData->GetUserProfileInt(GEN_COPY_TO_CLIPBOARD,
                                                            GEN_COPY_TO_CLIPBOARD_DF) == 1);
        bSuccess = true;
    }

    return bSuccess;

} // End GetConfigClipboard

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the config settings for
//          the verbose output value.
// Requires:
//      bVerbose: returned verbose settting
// Returns: true if successful, false otherise.
bool ConsoleControl::GetConfigVerbose(bool& bVerbose)
{
    std::string szSetting = _T("");
    bool bSuccess = false;

    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
    if (pProfileData) {
        szSetting = pProfileData->GetUserProfileStr(GEN_VERBOSE_OUTPUT, GEN_VERBOSE_OUTPUT_DF);
    }

    if (szSetting.length() > 0) {
        bVerbose = (atoi(szSetting.c_str()) == 1);
        bSuccess = true;
    }

    return bSuccess;

} // End GetConfigVerbose

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the config settings for
//          the page size setting.
// Requires:
//      l64PageSize: returned page size
// Returns: true if successful, false otherise.
bool ConsoleControl::GetConfigPageSize(LONG64& l64PageSize)
{
    std::string szSetting = _T("");
    bool bSuccess = false;

    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
    if (pProfileData) {
        szSetting = pProfileData->GetUserProfileStr(GEN_RESULT_PAGE_SIZE, GEN_RESULT_PAGE_SIZE_DF);
    }

    if (szSetting.length() > 0) {
        l64PageSize = atoi64(szSetting.c_str());
        bSuccess = true;
    }

    return bSuccess;

} // End GetConfigPageSize

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the config settings for
//          the offset setting.
// Requires:
//      l64Offset: returned offset value
// Returns: true if successful, false otherise.
bool ConsoleControl::GetConfigOffset(LONG64& l64Offset)
{
    std::string szSetting = _T("");
    bool bSuccess = false;

    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
    if (pProfileData) {
        szSetting = pProfileData->GetUserProfileStr(GEN_RESULT_PAGE, GEN_RESULT_PAGE_DF);
    }

    if (szSetting.length() > 0) {
        l64Offset = atoi64(szSetting.c_str());
        bSuccess = true;
    }

    return bSuccess;

} // End GetConfigOffset

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to get the resume intervals.
// Requires:
//      listResumeIntervals: vector to hold the results.
// Returns: true if successful, false otherise.
bool ConsoleControl::GetConfigResumeIntervals(std::vector<int>& listResumeIntervals)
{
    std::string szSetting = _T("");

    // Setup the default values for the intervals.
    listResumeIntervals.clear();

    int nResumeInterval1 = 5;
    int nResumeInterval2 = 30;
    int nResumeInterval3 = 300;
    int nResumeInterval4 = 600;
    int nResumeInterval5 = 3600;

    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
    if (pProfileData) {
        nResumeInterval1 = pProfileData->GetUserProfileInt(GEN_RESUME_INTERVAL_1, GEN_RESUME_INTERVAL_1_DF);
        nResumeInterval2 = pProfileData->GetUserProfileInt(GEN_RESUME_INTERVAL_2, GEN_RESUME_INTERVAL_2_DF);
        nResumeInterval3 = pProfileData->GetUserProfileInt(GEN_RESUME_INTERVAL_3, GEN_RESUME_INTERVAL_3_DF);
        nResumeInterval4 = pProfileData->GetUserProfileInt(GEN_RESUME_INTERVAL_4, GEN_RESUME_INTERVAL_4_DF);
        nResumeInterval5 = pProfileData->GetUserProfileInt(GEN_RESUME_INTERVAL_5, GEN_RESUME_INTERVAL_5_DF);
    }

    listResumeIntervals.push_back(nResumeInterval1);
    listResumeIntervals.push_back(nResumeInterval2);
    listResumeIntervals.push_back(nResumeInterval3);
    listResumeIntervals.push_back(nResumeInterval4);
    listResumeIntervals.push_back(nResumeInterval5);

    return true;

} // End GetConfigResumeIntervals

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to copy a given string to the clipboard.
// Requires:
//      szClipboardText: text to copy to the clipboard
// Returns: true if successful, false otherwise.
bool ConsoleControl:: CopyStringToClipboard(std::string szClipboardText)
{
    #ifndef WIN32
        return false;
    #else
        // Test to see if we can open the clipboard first before
        // wasting any cycles with the memory allocation
        if ( ::OpenClipboard(NULL)) {

            // Empty the Clipboard. This also has the effect
            // of allowing Windows to free the memory associated
            // with any data that is in the Clipboard
            ::EmptyClipboard();

            // Ok. We have the Clipboard locked and it's empty.
            // Now let's allocate the global memory for our data.

            // Here I'm simply using the GlobalAlloc function to
            // allocate a block of data equal to the text in the
            // "to clipboard" edit control plus one character for the
            // terminating null character required when sending
            // ANSI text to the Clipboard.
            HGLOBAL hClipboardData;
            hClipboardData = ::GlobalAlloc(GMEM_DDESHARE,
                                        szClipboardText.length()+1);

            // Calling GlobalLock returns to me a pointer to the
            // data associated with the handle returned from
            // GlobalAlloc
            char * pchData;
            pchData = (char*)::GlobalLock(hClipboardData);

            // At this point, all I need to do is use the standard
            // C/C++ strcpy function to copy the data from the local
            // variable to the global memory.
            strcpy(pchData, szClipboardText.c_str());

            // Once done, I unlock the memory - remember you
            // don't call GlobalFree because Windows will free the
            // memory automatically when EmptyClipboard is next
            // called.
            ::GlobalUnlock(hClipboardData);

            // Now, set the Clipboard data by specifying that
            // ANSI text is being used and passing the handle to
            // the global memory.
            ::SetClipboardData(CF_TEXT, hClipboardData);

            // Finally, when finished I simply close the Clipboard
            // which has the effect of unlocking it so that other
            // applications can examine or modify its contents.
            ::CloseClipboard();
        }

        return true;

    #endif

} // End CopyStringToClipboard

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the test flag for the
//          given command.
// Requires:
//      pCmdLine: pointer to the current command
// Returns: true if the flag is set, false otherwise.
bool ConsoleControl::TestOutput(CmdLine* pCmdLine)
{
    DiomedeSwitchArg* pTestArg = NULL;
    try {
        pTestArg = (DiomedeSwitchArg*)pCmdLine->getArg(ARG_TEST_SWITCH);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    bool bTestOutput = false;

    if (pTestArg && pTestArg->isSet()) {
        bTestOutput = true;
    }

    return bTestOutput;

} // End TestOutput

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the ? flag for the
//          given command.
// Requires:
//      pCmdLine: pointer to the current command
// Returns: true if the flag is set, false otherwise.
bool ConsoleControl::ShowUsage(CmdLine* pCmdLine)
{
    SwitchArg* pHelpArg = NULL;
    try {
        pHelpArg = (SwitchArg*)pCmdLine->getArg(CMD_HELP_ALT1);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    bool bShowUsage = false;

    if (pHelpArg && pHelpArg->isSet()) {
        bShowUsage = true;
    }

    return bShowUsage;

} // End ShowUsage


///////////////////////////////////////////////////////////////////////
//! \brief Set a control handler for keyboard input (Windows only).
//!
//! \param fdwCtrlType: keyboard control key + key combination.
//!
//! \return TRUE if successful, FALSE otherwise
BOOL ConsoleCtrlHandler( DWORD fdwCtrlType )
{
#ifdef WIN32
    switch( fdwCtrlType )
    {
        // Handle the CTRL-C signal.
        case CTRL_C_EVENT:
            g_bUsingCtrlKey = true;
            return TRUE;

        default:
            return FALSE;
    }
#else
    return TRUE;
#endif

} // End ConsoleCtrlHandler

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to suspend our process to give control
//          to other running processes.
// Requires: nothing
// Returns: true to continue, false otherwise
bool ConsoleControl::PauseProcess()
{
    // Set the value once on first use....
    if (m_nSystemSleep > 0) {
        Util::PauseProcess(m_nSystemSleep);

        if (g_bUsingCtrlKey) {
            return false;
        }

        return true;
    }

    SetConsoleControlHandler();

    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
    int nThreadSleep = 0;

    if (pProfileData != NULL) {
        nThreadSleep =
            pProfileData->GetUserProfileInt(GEN_THREAD_SLEEP, GEN_THREAD_SLEEP_DF);
    }

    // Sleep here to suspend our process to give control to other
    // running processes.
    if (nThreadSleep > 0) {
        m_nSystemSleep = nThreadSleep;
        Util::PauseProcess(nThreadSleep);
    }

    return true;

} // End PauseProcess

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to ensure the ConsoleControl can trap
//          CTRL+C.  Windows only.
// Requires: nothing
// Returns: true if successful, false otherwise.
bool ConsoleControl::SetConsoleControlHandler()
{
#if !defined( WIN32 )
	return false;
#else

    BOOL fSuccess= SetConsoleCtrlHandler(
            (PHANDLER_ROUTINE) ConsoleCtrlHandler,
            TRUE);
    if (! fSuccess ) {
        // TBD: quit ? alert the user ?
        // std::cout << _T("Could not set control handler") << std::endl;
    }
    
    return (fSuccess == TRUE);
    
#endif
    
} // End SetConsoleControlHandler

///////////////////////////////////////////////////////////////////////
// Purpose: Process the help command.
// Requires:
//      nNumArgs: number of arguments received
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessHelpCommand(int nNumArgs, CmdLine* pCmdLine,
                                        bool& bCommandFinished)
{
    // The help command has no partial entry states.
    bCommandFinished = true;

    if (pCmdLine->foundHelpOrVersionSwitch() == true) {
        if (nNumArgs == 2) {
		    return;
		}
    }

    // If no arguments, show the help menu.
    if (nNumArgs < 2) {
		ShowCommandsMenu();
		return;
    }

    UnlabeledValueArg<std::string>* pHelpArg = NULL;
    try {
        pHelpArg = (UnlabeledValueArg<std::string>*)pCmdLine->getArg(CMD_HELP);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_HELP << endl;
    }

    if (pHelpArg == NULL) {
        return;
    }

    std::string szArgValue = pHelpArg->getValue();

    std::string szCommand = AltCommandStrToCommandStr(szArgValue);
    DioCLICommands::COMMAND_ID tmpCmdID = CommandStrToCommandID(szCommand);

    CmdLine* pTmpCmdLine = NULL;
    CommandMap::iterator tmpCmdIter = m_listCommands.find(tmpCmdID);
    if (tmpCmdIter != m_listCommands.end()) {
        pTmpCmdLine = (*tmpCmdIter).second;
    }

    // Show the usage for the given command using it's help visitor.
    if (pTmpCmdLine != NULL) {
        pTmpCmdLine->getHelpVisitor()->visit();
    }
    else {
        CmdLineParseException e(_T("Unknown command."), szArgValue);
        m_stdOut.unknownCommand(szArgValue, e);

        if (!m_bSysCommandInput) {
            PrintNewLine();
        }
    }

} // End ProcessHelpCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the about command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessAboutCommand(CmdLine* pCmdLine,
                                         bool& bCommandFinished)
{
    // The help command has no partial entry states.
    bCommandFinished = true;

    PrintNewLine();

    _tprintf(_T(" Diomede CLI [Version %s]\n\r"), m_szAppVersion.c_str());
    _tprintf(_T(" Copyright (c) 2010 Diomede Corporation.\n\r"));
	_tprintf(_T("       Type HELP for help.\n\r") );

    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

} // End ProcessAboutCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the login command.  The user is prompted for
//          additional arguments (e.g. name and password).
//          Logs the user into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessLoginCommand(CmdLine* pCmdLine,
                                         bool& bCommandFinished)
{
    // User name and password are required fields.  If both or password
    // (e.g. last argument) are missing, prompt the user for the missing
    // fields.
    bCommandFinished = false;

    // Get the list of arguments - this command is a multi-unlabeled argument.
    DiomedeUnlabeledValueArg<std::string>* pUsernameArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pPasswordArg = NULL;

    try {
        pUsernameArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_LOGIN_USERNAME);
        pPasswordArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_LOGIN_PASSWORD);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_LOGIN << endl;
    }

    if ( (pUsernameArg == NULL) || (pPasswordArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

	std::string szUsername = pUsernameArg->getValue();
	std::string szPassword = pPasswordArg->getValue();

	if ( (szUsername.length() == 0) && (szPassword.length() == 0) ) {
        // Check if the auto login feature is turned on, and if so, we'll
        // login the user with the stored data.
        UserProfileData* pProfileData =
            ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );

	    if ( pProfileData &&
	       ( pProfileData->GetUserProfileInt(GEN_AUTO_LOGIN, GEN_AUTO_LOGIN_DF) != 0) &&
	       ( pProfileData->GetUserProfileInt(UI_REMEMBER_PWD, UI_REMEMBER_PWD_DF) != 0) ) {

	        // Get the encrypted username and password.
	        UserData userData;
	        if (pProfileData->GetUserProfileEncrypted(UI_CLIENT_ENCRYPTED, userData) == true) {
	            szUsername = userData.UserName();
	            szPassword = userData.Password();
  	        }
	    }
	}

	if ((szUsername.length() > 0) && (szPassword.length() > 0)) {
	    bCommandFinished = true;
	}
	else {
	    bCommandFinished = false;
	}

    m_bMaskInput = false;
	if (szUsername.length() == 0) {
	    m_szCommandPrompt = _T("Username: ");
	}
	else if (szPassword.length() == 0) {
	    m_szCommandPrompt = _T("Password: ");
	    m_bMaskInput = true;
	}

	if (bCommandFinished == false) {
	    return;
	}

	bool bSuccess = LoginUser(szUsername, szPassword);
	if (!bSuccess) {
	    return;
	}

	// If auto login has been set prior to this login, encrypt and
	// save the data now.  Also, if login was entered from the system
	// command prompt, we'll assume autologin as well.
    UserProfileData* pProfileData =
	    ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );

	if ( ( pProfileData &&
	     ( pProfileData->GetUserProfileInt(GEN_AUTO_LOGIN, 0) != 0) &&
	     ( pProfileData->GetUserProfileInt(UI_REMEMBER_PWD, 0) != 0) ) ||
	     ( m_bSysCommandInput == true ) ) {

        SetAutoLoginData(true);
	}

} // End ProcessLoginCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to login the user.
// Requires:
//      szUsername: username
//      szPassword: plain text password.
//      bShowProgress: shows progress if true, no output otherwise
// Returns: true if successful, false otherwise.
bool ConsoleControl::LoginUser(std::string szUsername, std::string szPassword,
                               bool bShowProgress /*true*/)
{

    //-----------------------------------------------------------------
    // Process login
    //-----------------------------------------------------------------

    // If we're checking the account status as well, add a newline before
    // and turn off the newline after....
    if (m_bAutoCheckAccountOn) {
        PrintNewLine();
    }

    std::string szLoginUserStart = _format(_T("Login user %s"), szUsername.c_str());
    g_szTaskFriendlyName = _T("Login");

	DIOMEDE_CONSOLE::LoginTask taskLogin(szUsername, szPassword);
	int nResult = HandleTask(&taskLogin, szLoginUserStart,  _T(""),
	                         false, !m_bAutoCheckAccountOn, bShowProgress);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskLogin.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);
	    m_errorCode = UI_INVALID_USERNAME_OR_PWD;
	    return false;
	}

    m_szSessionToken = taskLogin.GetSessionToken();
    m_szUsername = szUsername;
    m_szPlainTextPassword = szPassword;

    if (m_bAutoCheckAccountOn) {
        nResult = CheckAccount();
    }

    m_bConnected = true;
    g_nSessionRetries = MAX_LOGIN_RETRIES;
    SetCommandPrompt();

    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
    if (pProfileData) {
        // If the "save" here fails, do we care?  This feature is more a nicety for the
        // user.
        pProfileData->SetUserProfileStr(GEN_SESSION_TOKEN, m_szSessionToken.c_str());
        pProfileData->SetUserProfileLong(GEN_SESSION_TOKEN_EXPIRES, (long)UpdateSessionTokenExpiration());
        pProfileData->SaveUserProfile();
    }

    return (nResult == SOAP_OK);

} // End LoginUser

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function present the error when the user needs
//          to be connected in order to proceed.
// Requires:
//      szErrorMsg: Optional error message to preface the instructions
//      to the user.
//      szPassword: plain text password.
// Returns: true if successful, false otherwise.
void ConsoleControl::LoggedInUserError(std::string szErrorMsg /*_T("")*/)
{
    PrintNewLine();
    std::string szOutErrorMsg = _T(" You must first log in to the Diomede service.\n\r");
    if (szErrorMsg.length() > 0) {
        szOutErrorMsg = szErrorMsg + _T(":") + szOutErrorMsg;
    }

    _tprintf(_T("%s"), szOutErrorMsg.c_str());

    PrintNewLine();
    _tprintf(_T("    For example:\n\r"));
    _tprintf(_T("    c:\\>DioCLI login <username> <password>\n\r"));

    PrintNewLine();
    _tprintf(_T("To set a default account to automatically login, use \"setconf /autologin=on\"\n\r"));

    PrintNewLine();
    _tprintf(_T("    For example:\n\r"));
    _tprintf(_T("    c:\\>DioCLI\n\r"));
    _tprintf(_T("    Diomede> login <username> <password>\n\r"));
    _tprintf(_T("    Diomede:\\<username> setconf /autologin=on\n\r"));

    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

} // End LoggedInUserError

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to update the session token expiration
//          stored locally.
// Requires: nothing
// Returns: updated session token expiration
time_t ConsoleControl::UpdateSessionTokenExpiration()
{
    time_t epochSeconds = 0;
    
    boost::posix_time::ptime currentTime = boost::posix_time::microsec_clock::local_time() +
        boost::posix_time::time_duration(24, 0, 0, 0);

    std::tm tmRef = boost::posix_time::to_tm(currentTime);
    epochSeconds = mktime(&tmRef);
    
    return epochSeconds;

} // End UpdateSessionTokenExpiration

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to determine if the locally saved session
//          token has expired.
// Requires: nothing
// Returns: true if the session has expired, false otherwise.
bool ConsoleControl::HasLocalSessionTokenExpired()
{
    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
    if (pProfileData == NULL) {
        return true;
    }

    time_t tSessionTokenExpires = (time_t)pProfileData->GetUserProfileLong(GEN_SESSION_TOKEN_EXPIRES);
    
    time_t epochSeconds = 0;    
    boost::posix_time::ptime currentTime = boost::posix_time::microsec_clock::local_time();
    
    std::tm tmRef = boost::posix_time::to_tm(currentTime);
    epochSeconds = mktime(&tmRef);
    
    bool bExpires = false;
    if (epochSeconds > tSessionTokenExpires) {
        bExpires = true;
    }
    
    return bExpires; 

} // End HasLocalSessionTokenExpired

///////////////////////////////////////////////////////////////////////
// Purpose: Process the set config command - allows the user to
//          set or reset various configuration file options.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessConfigCommand(CmdLine* pCmdLine,
                                          bool& bCommandFinished)
{
    bCommandFinished = true;

    // If no arguments, show the current contents of the configuration file.

    // Get the list of arguments - list will be added onto as more configuration
    // options are specified.  Each argument should be a /name=<value> pair.
    DiomedeValueArg<std::string>* pLoginArg = NULL;
    DiomedeValueArg<std::string>* pLogoffArg = NULL;
    DiomedeValueArg<std::string>* pCheckAccountArg = NULL;
    DiomedeValueArg<std::string>* pPageSizeArg = NULL;
    DiomedeValueArg<std::string>* pOffsetArg = NULL;
    DiomedeValueArg<std::string>* pSSLModeArg = NULL;
    DiomedeValueArg<std::string>* pServiceEndpointArg = NULL;
    DiomedeValueArg<std::string>* pSSLServiceEndpointArg = NULL;
    DiomedeValueArg<std::string>* pTransferEndPointArg = NULL;
    DiomedeValueArg<std::string>* pSSLTransferEndPointArg = NULL;
    DiomedeValueArg<std::string>* pClipboardArg = NULL;
    DiomedeValueArg<std::string>* pVerboseArg = NULL;

    try {
        pLoginArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_AUTOLOGIN);
        pLogoffArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_AUTOLOGOUT);
        pCheckAccountArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_CHECK_ACCOUNT);
        pPageSizeArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_PAGE_SIZE);
        pOffsetArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_OFFSET);
        pSSLModeArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_USE_SSL);
        pServiceEndpointArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_SERVICE_ENDPOINT);
        pSSLServiceEndpointArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_SSL_SERVICE_ENDPOINT);
        pTransferEndPointArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_TRANSFER_ENDPOINT);
        pSSLTransferEndPointArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_SSL_TRANSFER_ENDPOINT);
        #ifdef WIN32
            pClipboardArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_CLIPBOARD);
        #endif
        pVerboseArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_VERBOSE);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SETCONFIG << endl;
    }

    if ( (pLoginArg == NULL) ||
         (pLogoffArg == NULL) ||
         (pCheckAccountArg == NULL) ||
         (pPageSizeArg == NULL) ||
         (pOffsetArg == NULL) ||
         (pSSLModeArg == NULL) ||
         (pServiceEndpointArg == NULL) ||
         (pSSLServiceEndpointArg == NULL) ||
         (pTransferEndPointArg == NULL) ||
         (pSSLTransferEndPointArg == NULL) ||
         (pVerboseArg == NULL) ) {
        return;
    }

    #ifdef WIN32
        if (pClipboardArg == NULL) {
            return;
        }
    #endif

    bCommandFinished = true;
    bool bArgIsSet = false;

	//--------------------------------------------------------------------
	// Check each argument and process those that are set.
	//--------------------------------------------------------------------
	PrintNewLine();

	std::string szSetting = _T("");
	if ( pLoginArg->isSet() )  {

        if (m_bConnected == false) {
            PrintStatusMsg(_T("Auto login: you must first log into the Diomede service."));
            return;
        }

        bArgIsSet = true;
    	szSetting = pLoginArg->getValue();

        bool bAutoLogin = (0==stricmp(szSetting.c_str(), _T("on")));
        SetAutoLoginData(bAutoLogin);
	}

    UserProfileData* pProfileData =
	    ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
	if (!pProfileData) {
        std::string szStatusMsg =
            _format(_T("%s command failed. Couldn't access user configuration data."),
            pCmdLine->getCommandName().c_str());
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_ERROR, false, _T("Couldn't create user profile object"));
	    return;
	}

    // Set the auto logoff preference.
	szSetting = _T("");
	if ( pLogoffArg->isSet() )  {

        if (m_bConnected == false) {
            PrintStatusMsg(_T("Auto logoff: you must first log into the Diomede service."));
            return;
        }

        bArgIsSet = true;
    	szSetting = pLogoffArg->getValue();

        m_bAutoLogoutOff = (0==stricmp(szSetting.c_str(), _T("off")));
	    pProfileData->SetUserProfileInt(GEN_AUTO_LOGOUT, !m_bAutoLogoutOff);
	}

    // Set the check account (on login) preference.
	szSetting = _T("");
	if ( pCheckAccountArg->isSet() )  {

        if (m_bConnected == false) {
            PrintStatusMsg(_T("Check account: you must first log into the Diomede service."));
            return;
        }

        bArgIsSet = true;
    	szSetting = pCheckAccountArg->getValue();

        m_bAutoCheckAccountOn = (0==stricmp(szSetting.c_str(), _T("on")));
	    pProfileData->SetUserProfileInt(GEN_AUTO_CHECK_ACCOUNT, m_bAutoCheckAccountOn);
	}

	szSetting = _T("");
	if ( pPageSizeArg->isSet() )  {

        if (m_bConnected == false) {
            _tprintf(_T("%s command failed.\n\r"), pCmdLine->getCommandName().c_str());
            _tprintf(_T("Set page size: you must first log into the Diomede service.\n\r"));
            PrintNewLine();
            return;
        }

        bArgIsSet = true;
    	szSetting = pPageSizeArg->getValue();
	    pProfileData->SetUserProfileStr(GEN_RESULT_PAGE_SIZE, szSetting.c_str() );

	}

	szSetting = _T("");
	if ( pOffsetArg->isSet() )  {

        if (m_bConnected == false) {
            _tprintf(_T("%s command failed.\n\r"), pCmdLine->getCommandName().c_str());
            _tprintf(_T("Set page: you must first log into the Diomede service.\n\r"));
            PrintNewLine();
            return;
        }

        bArgIsSet = true;
    	szSetting = pOffsetArg->getValue();
	    pProfileData->SetUserProfileStr(GEN_RESULT_PAGE, szSetting.c_str() );
	}

    // Set the SSL setting
	szSetting = _T("");
	if ( pSSLModeArg->isSet() )  {
        bArgIsSet = true;
    	szSetting = pSSLModeArg->getValue();

    	int nSSLMode = SECURE_SERVICE_PARTIAL;

    	if  (0 != stricmp(szSetting.c_str(), _T("no"))) {
    	    // Make sure the PEM file exists or can be created.
            if ( ExtractCertificate() == true) {
                if (0 == stricmp(szSetting.c_str(), _T("yes"))) {
    	            nSSLMode = SECURE_SERVICE_PARTIAL;
                }
                else if  (0 == stricmp(szSetting.c_str(), _T("all"))) {
    	            nSSLMode = SECURE_SERVICE_ALL;
                }
            }
            else {
                // Couldn't create or find the PEM file - set the SSL mode
                // to "none"
                nSSLMode = SECURE_SERVICE_NONE;

                if ( false == IsPemGood() ) {
                    std::string szSysError = GetErrorString();
                    std::string szErrorMsg =
                        _format(_T("Error creating Diomede certficate: %s\n\rSSL mode set to none."),
                        szSysError.c_str());
                    PrintStatusMsg(szErrorMsg, false, true, true);

                    ResetErrorCodes();
                }
            }

    	}
    	else {
            nSSLMode = SECURE_SERVICE_NONE;
    	}

	    pProfileData->SetUserProfileInt(GEN_SERVICE_SECURE_TYPE, nSSLMode);
	}

	szSetting = _T("");
	bool bEndpointChanged = false;

	if ( pServiceEndpointArg->isSet() )  {

        if (m_bConnected == true) {
            _tprintf(_T("%s command failed.\n\r"), pCmdLine->getCommandName().c_str());
            _tprintf(_T("Set service endpoint: you must first log out of the Diomede service.\n\r"));
            PrintNewLine();
            return;
        }

        bArgIsSet = true;
        bEndpointChanged = true;

        // Shutdown the current service to allow us to reset the endpoints.
        ShutdownProxyService();

    	szSetting = pServiceEndpointArg->getValue();
	    pProfileData->SetUserProfileStr(GEN_SERVICE_ENDPOINT, szSetting.c_str() );
	}

	if ( pSSLServiceEndpointArg->isSet() )  {

        if (m_bConnected == true) {
            _tprintf(_T("%s command failed.\n\r"), pCmdLine->getCommandName().c_str());
            _tprintf(_T("Set SSL service endpoint: you must first log out of the Diomede service.\n\r"));
            PrintNewLine();
            return;
        }

        bArgIsSet = true;
        bEndpointChanged = true;

        // Shutdown the current service to allow us to reset the endpoints.
        if (bEndpointChanged == false ) {
            ShutdownProxyService();
        }

    	szSetting = pSSLServiceEndpointArg->getValue();
	    pProfileData->SetUserProfileStr(GEN_SECURE_SERVICE_ENDPOINT, szSetting.c_str() );
	}

	szSetting = _T("");
	if ( pTransferEndPointArg->isSet() )  {

        if (m_bConnected == true) {
            _tprintf(_T("%s command failed.\n\r"), pCmdLine->getCommandName().c_str());
            _tprintf(_T("Set transfer endpoint: you must first log out of the Diomede service.\n\r"));
            PrintNewLine();
            return;
        }

        bArgIsSet = true;

        // Shutdown the current service to allow us to reset the endpoints.
        if (bEndpointChanged == false ) {
            ShutdownProxyService();
        }

    	szSetting = pTransferEndPointArg->getValue();
	    pProfileData->SetUserProfileStr(GEN_TRANSFER_ENDPOINT, szSetting.c_str() );
	}

	szSetting = _T("");
	if ( pSSLTransferEndPointArg->isSet() )  {

        if (m_bConnected == true) {
            _tprintf(_T("%s command failed.\n\r"), pCmdLine->getCommandName().c_str());
            _tprintf(_T("Set SSL transfer endpoint: you must first log out of the Diomede service.\n\r"));
            PrintNewLine();
            return;
        }

        bArgIsSet = true;

        // Shutdown the current service to allow us to reset the endpoints.
        if (bEndpointChanged == false ) {
            ShutdownProxyService();
        }

    	szSetting = pSSLTransferEndPointArg->getValue();
	    pProfileData->SetUserProfileStr(GEN_SECURE_TRANSFER_ENDPOINT, szSetting.c_str() );
	}

    #ifdef WIN32
	    if ( pClipboardArg->isSet() )  {

            bArgIsSet = true;
    	    szSetting = pClipboardArg->getValue();
            bool bClipboard = (0==stricmp(szSetting.c_str(), _T("on")));
	        pProfileData->SetUserProfileInt(GEN_COPY_TO_CLIPBOARD, bClipboard);
	    }
    #endif

	if ( pVerboseArg->isSet() )  {

        bArgIsSet = true;
    	szSetting = pVerboseArg->getValue();
        bool bVerbose = (0==stricmp(szSetting.c_str(), _T("on")));
	    pProfileData->SetUserProfileInt(GEN_VERBOSE_OUTPUT, bVerbose);
	}

    if (bArgIsSet) {
    	ErrorType errorType = pProfileData->SaveUserProfile();
    	if (errorType != ERR_NONE) {
    	    int nLastError = pProfileData->GetLastSysError();
    	    std::string szErrorMsg = _format(_T("Configuration could not be saved: %s"),
    	        strerror(nLastError));
    	    PrintStatusMsg(szErrorMsg);
    	}
    }

    // Now, if the endpoint(s) have changed, we need to startup the proxy service
    // again.
    if (bEndpointChanged) {
        // Needed only if the service storage/transfer objects are kept
        // around.
    }

	// If no arguments have been set, show the current list of configuration options.
	// Re: US35 Show the configuration settings when the user turns off or on to give
	// the user feedback on their input.
	/*
	if (bArgIsSet == false) {
	    ShowConfigSettings();
	}
	*/

    ShowConfigSettings();
    if (!m_bSysCommandInput) {
	    PrintNewLine();
	}

} // End ProcessConfigCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to set the auto login properties
//          in the configuration file.
// Requires:
//      bAutoLogin: true to turn on auto login, false otherwise.
// Returns: true if successful, false otherwise.
bool ConsoleControl::SetAutoLoginData(bool bAutoLogin /*true*/,
	                                  std::string szUsername /*_T("")*/,
	                                  std::string szPassword /*T("")*/)
{
    m_bAutoLoginOn = bAutoLogin;

    UserProfileData* pProfileData =
	    ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );

	if (!pProfileData) {
        PrintStatusMsg(_T("Set login information failed."));
        ClientLog(UI_COMP, LOG_ERROR, false, _T("Couldn't create user profile object"));
	    return false;
	}

	// Allow for either input from the system command line or within the
	// DioCLI application.
	std::string szTmpUsername = szUsername;
	std::string szTmpPassword = szPassword;

	if (szTmpUsername.length() == 0) {
	    szTmpUsername = m_szUsername;
	    szTmpPassword = m_szPlainTextPassword;
	}

    // If the option has been turned off, clear out the username (?) and
    // password fields from the file.
    if (!m_bAutoLoginOn) {
    	pProfileData->SetUserProfileStrEncrypted(UI_CLIENT_ENCRYPTED, _T(""));
    	pProfileData->SetUserProfileInt(UI_CLIENT_LENGTH, 0);
    	pProfileData->SetUserProfileStr(GEN_SESSION_TOKEN, _T(""));
    	pProfileData->SetUserProfileLong(GEN_SESSION_TOKEN_EXPIRES, 0);

    	#ifdef NO_ENCRYPTED_USER_PROFILE_KEYS
    	    pProfileData->SetUserProfileStrEncrypted(UI_USERNAME, _T(""));
    	    pProfileData->SetUserProfileStrEncrypted(UI_PASSWORD, _T(""));
    	#endif
    }
	else if (m_bConnected) {
	    if (m_bAutoLoginOn) {
    	    // Save this user, and potentially the password as the last logged in user.
	        pProfileData->SetUserProfileStr(UI_LASTLOGIN, szTmpUsername.c_str() );

	        UserData userData;
	        userData.UserName(szTmpUsername);
	        userData.Password(szTmpPassword);

    	    pProfileData->SetUserProfileEncrypted(UI_CLIENT_ENCRYPTED, userData);
    	}

    	pProfileData->SetUserProfileStr(GEN_SESSION_TOKEN, m_szSessionToken.c_str());
    	pProfileData->SetUserProfileLong(GEN_SESSION_TOKEN_EXPIRES, (long)UpdateSessionTokenExpiration());
	}

	// Set the relevant profile settings.  For now, the auto login option
	// and remember password option are the same.  We may want to separate
	// them out in the future.
	pProfileData->SetUserProfileInt(GEN_AUTO_LOGIN, m_bAutoLoginOn);
	pProfileData->SetUserProfileInt(UI_REMEMBER_PWD, m_bAutoLoginOn);

	ErrorType errorType = pProfileData->SaveUserProfile();
	if (errorType != ERR_NONE) {
	    int nLastError = pProfileData->GetLastSysError();
	    std::string szErrorMsg = _format(_T("Set login information could not be saved: %s"),
	        strerror(nLastError));
	    PrintStatusMsg(szErrorMsg);
	    return false;
	}

	return true;

} // End SetAutoLoginData

///////////////////////////////////////////////////////////////////////
// Purpose: Process the logout command - user is logged out of the
//          Diomede service.  All connections are destroyed.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessLogoutCommand(CmdLine* pCmdLine,
                                          bool& bCommandFinished)
{
    // If logout was entered from the system command line, we'll assume the user wants
    // to clear their autologin settings.
    if (m_bSysCommandInput) {
        SetAutoLoginData(false);
	    PrintStatusMsg(_T("Logout Successful."));
	    return;
    }

    // User must be logged in to logout...
    if (m_bConnected == false) {
        bCommandFinished = true;

        PrintStatusMsg(_T("Logout user: you are not logged into the Diomede service."));

	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Logout failed: you are not logged into the Diomede service."));
	    return;
    }

    LogoutDiomedeService();

} // End ProcessLogoutCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the getsessiontoken command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetSessionTokenCommand(CmdLine* pCmdLine,
                                                   bool& bCommandFinished)
{
    // User must be logged in to receive a session token
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get session token"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get session token: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    ClientLog(UI_COMP, LOG_STATUS, false,_T("Get session token suuccessful %s."),
        m_szSessionToken.c_str());

    _tprintf(_T(" Session Token: %s \n\r"), m_szSessionToken.c_str());
    PrintNewLine();

    bool bUseClipboard = false;
    GetConfigClipboard(bUseClipboard);

    if (bUseClipboard) {
        CopyStringToClipboard(m_szSessionToken);
    }

} // End ProcessGetSessionTokenCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the create user command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessCreateUserCommand(CmdLine* pCmdLine,
                                              bool& bCommandFinished)
{
	UserData userData;

	if (!m_bSysCommandInput) {
        // User name, password, and email are required fields.
        if (false == GetUserInfo(pCmdLine, &userData, bCommandFinished)) {
            return;
        }
    }
    else {
        // Handle input from the system command prompt special - user name, password, and
        // email are required fields as above, but no further prompting is needed.
        if (false == GetUserInfoFromSysCommandLine(pCmdLine, &userData, bCommandFinished)) {
            return;
        }
    }

    CreateUserImpl createUserInfo;
    createUserInfo.SetUserName(userData.UserName());
    createUserInfo.SetPassword(userData.Password());
    createUserInfo.SetEmail(userData.EmailAddress());

    //-----------------------------------------------------------------
    // Process createuser
    //-----------------------------------------------------------------
    std::string szCreateUserStart = _format(_T("Creating user %s"), userData.UserName());

	DIOMEDE_CONSOLE::CreateUserTask taskCreateUser(&createUserInfo);
	int nResult = HandleTask(&taskCreateUser, szCreateUserStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskCreateUser.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Create new user failed for %s."),
	        userData.UserName());
	    return;
	}

    ClientLog(UI_COMP, LOG_STATUS, false, _T("Create new user successful for %s."),
        userData.UserName());

    std::string szStatusMsg = _format(_T("Create new user %s successful.  Type LOGIN to login."),
        userData.UserName());
    PrintStatusMsg(szStatusMsg);

    // The assumption is that if the user used the system command line to create
    // the user, we'll assume auto login.
    SetAutoLoginData(m_bSysCommandInput, userData.UserName(), userData.Password());

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessCreateUserCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function for creating a user and updating the user
//          info.  Handles user info input.
// Requires:
//      pCmdLine: current command line
//      pUserData: holds the user profile input.
//      bCommandFinished; set to true if the command is complete
//      ---------------------------------------------------------------
//      bAllFieldsRequired: indicates whether all fields are
//      required.  Originally added for updating the user info - problem
//      not yet solved, is how to handle blank fields when both
//      user prompts and single line input is allowed.
//      ---------------------------------------------------------------
// Returns: true if successful, false otherwise.
bool ConsoleControl::GetUserInfo(CmdLine* pCmdLine, UserData* pUserData,
                                 bool& bCommandFinished, bool bAllFieldsRequired /*true*/)
{
    bCommandFinished = false;

    // Get the list of arguments - this command is a multi-unlabeled argument.
    DiomedeUnlabeledValueArg<std::string>* pNameArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pPwdArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pEmailArg = NULL;

    try {
        pNameArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_USERNAME);
        pPwdArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_PASSWORD);
        pEmailArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_EMAIL);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    if ( (pNameArg == NULL ) ||
         (pPwdArg == NULL )||
         (pEmailArg == NULL ) ){

        bCommandFinished = true;
        return false;
    }

    m_bMaskInput = false;

    if ( false == IsUserInfoArgComplete(pCmdLine, pNameArg, _T("Username: "), _T("Password: "),
        bCommandFinished,  bAllFieldsRequired) ) {
        return false;
    }

    if ( false == IsUserInfoArgComplete(pCmdLine, pPwdArg, _T("Password: "), _T("Email: "),
        bCommandFinished,  bAllFieldsRequired) ) {
	    m_bMaskInput = true;
        return false;
    }

    if ( false == IsUserInfoArgComplete(pCmdLine, pEmailArg, _T("Email: "), _T(""),
        bCommandFinished,  true) ) {
        return false;
    }

    bCommandFinished = true;

    // We require only username, password, and email - therefore, no other
    // prompting is needed - this method and the one that follows are
    // candidates for combining - leaving separate for now...

	// If not all fields are needed, at least one field must be completed.
	// Otherwise, no work is needed.
	if (!bAllFieldsRequired) {
	    bool bUpdateNeeded = ( ( pNameArg->getValue().length() > 0 ) ||
	                           ( pPwdArg->getValue().length() > 0 ) ||
	                           ( pEmailArg->getValue().length() > 0 ) );
	    if (bUpdateNeeded == false) {
	        std::string szStatusMsg = _format(_T("%s: all fields are empty, no update will occur."),
	            pCmdLine->getCommandName().c_str());
	        PrintStatusMsg(szStatusMsg, true);
	        return false;
	    }
	}

    // Remove all quotes from the arguments.
    std::string szTemp = _T("");

    RemoveQuotesFromArgument(pNameArg->getValue(), szTemp);
    pUserData->UserName(szTemp);

    RemoveQuotesFromArgument(pPwdArg->getValue(), szTemp);
    pUserData->Password(szTemp);

    RemoveQuotesFromArgument(pEmailArg->getValue(), szTemp);
    pUserData->EmailAddress(szTemp);

    return true;

} // End GetUserInfo

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function for creating a user when the command is
//          entered from the system command line.  The difference
//          here from the above method is that the user needs only
//          to enter a username, password, and email.
// Requires:
//      pCmdLine: current command line
//      pUserData: holds the user profile input.
//      bCommandFinished; set to true if the command is complete
// Returns: true if successful, false otherwise.
bool ConsoleControl::GetUserInfoFromSysCommandLine(CmdLine* pCmdLine,
                                                   UserData* pUserData,
                                                   bool& bCommandFinished)
{
    bCommandFinished = false;

    // Get the list of arguments - this command is a multi-unlabeled argument.
    DiomedeUnlabeledValueArg<std::string>* pNameArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pPwdArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pEmailArg = NULL;

    try {
        pNameArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_USERNAME);
        pPwdArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_PASSWORD);
        pEmailArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_EMAIL);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    if ( (pNameArg == NULL ) ||
         (pPwdArg == NULL )||
         (pEmailArg == NULL ) ){

        bCommandFinished = true;
        return false;
    }

    m_bMaskInput = false;

    // Since input is was from the system command line, we can stop processing the
    // command once we have the username, password, and email.

    if ( false == IsUserInfoArgComplete(pCmdLine, pNameArg, _T("Username: "), _T("Password: "),
        bCommandFinished,  true) ) {
        return false;
    }

    if ( false == IsUserInfoArgComplete(pCmdLine, pPwdArg, _T("Password: "), _T("Email: "),
        bCommandFinished,  true) ) {
        m_bMaskInput = true;
        return false;
    }

    if ( false == IsUserInfoArgComplete(pCmdLine, pEmailArg, _T("Email: "), _T(""),
        bCommandFinished,  true) ) {
        return false;
    }

    bCommandFinished = true;

    // Remove all quotes from the arguments.
    std::string szTemp = _T("");

    RemoveQuotesFromArgument(pNameArg->getValue(), szTemp);
    pUserData->UserName(szTemp);

    RemoveQuotesFromArgument(pPwdArg->getValue(), szTemp);
    pUserData->Password(szTemp);

    RemoveQuotesFromArgument(pEmailArg->getValue(), szTemp);
    pUserData->EmailAddress(szTemp);

    return true;

} // End GetUserInfoFromSysCommandLine

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function for creating a user and updating the user
//          info.  Handles user info input.
// Requires:
//      pCmdLine: current command
//      pArg: current argument
//      szCurrentPrompt: current command prompt
//      szNextPrompt: next command prompt.
//      bCommandFinished:  set to true if command is complete (that is,
//      no further input from the user is needed).
//      bAllFieldsRequired: indicates whether all fields are
//      required.  For updating the user info, not all fields are
//      needed.
// Returns: true if successful, false otherwise.
bool ConsoleControl::IsUserInfoArgComplete(CmdLine* pCmdLine,
                                           DiomedeUnlabeledValueArg<std::string>* pArg,
                                           std::string szCurrentPrompt, std::string szNextPrompt,
                                           bool& bCommandFinished, bool bAllFieldsRequired)
{
    if (pArg == NULL) {
        return false;
    }

    bool bArgIsDone = false;

	if (pArg->getValue().length() == 0) {

	    if (bAllFieldsRequired || (pArg->getRepromptCount() < 2) ) {
	        m_szCommandPrompt = szCurrentPrompt;
	    }
	    else if (pArg->getRepromptCount() == 2) {
	        m_szCommandPrompt = szNextPrompt;
	        bArgIsDone = true;
	    }
	}
	else {
        m_szCommandPrompt = szNextPrompt;
	    bArgIsDone = true;
	}

	if (bArgIsDone) {
	    return true;
	}

    if (pArg->getRepromptCount() < 2) {
        pArg->incrementRepromptCount();
    }
    else {
        // We've prompted them already - move onto the
        // next command if allowed.
        if (bAllFieldsRequired) {
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
            bArgIsDone = true;
        }
    }

    return bArgIsDone;

} // End IsUserInfoArgComplete

///////////////////////////////////////////////////////////////////////
// Purpose: Process the change password command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessChangePasswordCommand(CmdLine* pCmdLine,
                                                  bool& bCommandFinished)
{
    // User must be logged in to change their password
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot change password"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Change password: you must be logged into the Diomede service."));
	    return;
    }

    DiomedeUnlabeledValueArg<std::string>* pOldPwdDArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pNewPwdArg = NULL;

    try {
        pOldPwdDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_OLDPASSWORD);
        pNewPwdArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_NEWPASSWORD);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_CHANGEPASSWORD << endl;
    }

    if ( ( pOldPwdDArg == NULL) || ( pNewPwdArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;
    m_bMaskInput = false;

	if (pOldPwdDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Old password: ");
	    pTmpArg = pOldPwdDArg;
	    m_bMaskInput = true;
	}
	else if (pNewPwdArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("New password: ");
	    pTmpArg = pNewPwdArg;
	    m_bMaskInput = true;
	}

	bCommandFinished = ( ( pOldPwdDArg->getValue().length() > 0 ) &&
	                     ( pNewPwdArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;

    std::string szNewPassword = _T("");
    std::string szOldPassword = _T("");

    RemoveQuotesFromArgument(pOldPwdDArg->getValue(), szNewPassword);
    RemoveQuotesFromArgument(pNewPwdArg->getValue(), szOldPassword);

    //-----------------------------------------------------------------
    // Process changepassword
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::ChangePasswordTask taskChangePassword(m_szSessionToken, szNewPassword,
	                                                       szOldPassword);
	int nResult = HandleTask(&taskChangePassword, _T("Changing password"));

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskChangePassword.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Change password from %s to %s failed."),
	        pOldPwdDArg->getValue().c_str(), pNewPwdArg->getValue().c_str());
	    return;
	}

    ClientLog(UI_COMP, LOG_STATUS, false,
        _T("Change password from %s to %s successful."),
        pOldPwdDArg->getValue().c_str(), pNewPwdArg->getValue().c_str());

    std::string szStatusMsg = _format(_T("Change password successful."));
    PrintStatusMsg(szStatusMsg);

    //-----------------------------------------------------------------
    // Cleanup - the user needs to be logged out and logged back in?
    //-----------------------------------------------------------------

    // Save the new login information - we'll need to write this out
    // to the config file as well.
    m_szPlainTextPassword = pNewPwdArg->getValue();

	// If auto login has been set prior to this login, decrypt and
	// save the data now.
    UserProfileData* pProfileData =
	    ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );

	if ( pProfileData &&
	   ( pProfileData->GetUserProfileInt(GEN_AUTO_LOGIN, 0) != 0) &&
	   ( pProfileData->GetUserProfileInt(UI_REMEMBER_PWD, 0) != 0) ) {

        SetAutoLoginData(true);
	}

} // End ProcessChangePasswordCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the rest password command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessResetPasswordCommand(CmdLine* pCmdLine,
                                                 bool& bCommandFinished)
{
    // User name is a required fields.
    bCommandFinished = false;

    // Get the list of arguments - this command is a multi-unlabeled argument.
    DiomedeUnlabeledValueArg<std::string>* pUsernameArg = NULL;

    try {
        pUsernameArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_LOGIN_USERNAME);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_RESETPASSWORD << endl;
    }

    if (pUsernameArg == NULL) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

    if (pUsernameArg->getValue().length() == 0) {
        m_szCommandPrompt = _T("Username: ");
        pTmpArg = pUsernameArg;
    }

    bCommandFinished = ( pUsernameArg->getValue().length() > 0 );

    if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

        return;
    }

    bCommandFinished = true;

    //-----------------------------------------------------------------
    // Process resetpassword
    //-----------------------------------------------------------------
    std::string szUsername = pUsernameArg->getValue();
    std::string szResetPasswordStart = _format(_T("Resetting password"));

	DIOMEDE_CONSOLE::ResetPasswordTask taskResetPassword(szUsername);

	/* We need to handle this task separate, since we really don't want to try to
	   handle retries?
	   int nResult = HandleTask(&taskResetPassword, szResetPasswordStart);
	*/

    DIOMEDE_CONSOLE::CommandThread commandThread;
    if (false == CheckThread(&commandThread, pCmdLine->getCommandName())) {
        return;
    }

    BOOL bReturn = commandThread.Event(&taskResetPassword);

    if (bReturn == FALSE) {
        return;
    }

    PrintNewLine();

    MessageTimer msgTimer(50, true);
    msgTimer.Start(szResetPasswordStart);

    while ( taskResetPassword.Status() != TaskStatusCompleted ) {
        msgTimer.ContinueTime();
        PauseProcess();
	}

    commandThread.Stop();
    msgTimer.EndTime(_T(""));

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
    int nResult = taskResetPassword.GetResult();

	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Reset password successful.  You will receive an email shortly with instructions on your next steps."));
        PrintStatusMsg(szStatusMsg, false, true, true);
        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskResetPassword.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Reset password failed."));
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessChangePasswordCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the delete user command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessDeleteUserCommand(CmdLine* pCmdLine,
                                              bool& bCommandFinished)
{
    // User must be logged in to delete a user.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot delete user"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Delete user failed: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    //-----------------------------------------------------------------
    // Process delete user
    //-----------------------------------------------------------------
    std::string szDeleteUserStart = _format(_T("Deleting user %s"), m_szUsername.c_str());

	DIOMEDE_CONSOLE::DeleteUserTask taskDeleteUser(m_szSessionToken);
	int nResult = HandleTask(&taskDeleteUser, szDeleteUserStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    ClientLog(UI_COMP, LOG_STATUS, false,_T("Delete user successful for %s."),
	        m_szUsername.c_str());
	    // A new line will be printed by the logout method.
	    std::string szStatusMsg = _format(_T("Delete user %s successful."), m_szUsername.c_str());
	    PrintStatusMsg(szStatusMsg);

	    // No need to log the user out - just reset our data and bumpt them back to
	    // the main prompt.
	    // LogoutDiomedeService();
	    ResetLoginData();
	    SetCommandPrompt();
	}
	else {
	    std::string szErrorMsg = taskDeleteUser.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Delete user failed for %s."),
	        m_szUsername.c_str());
	}

} // End ProcessDeleteUserCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get user info command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetUserInfoCommand(CmdLine* pCmdLine,
                                               bool& bCommandFinished)
{
    // User must be logged in to update the user information.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get user info"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get User Info: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    //-----------------------------------------------------------------
    // Process getuserinfo
    //-----------------------------------------------------------------
    std::string szGetUserInfoStart = _format(_T("Getting user information for %s"), m_szUsername.c_str());

    UserInfoImpl userInfo;

	DIOMEDE_CONSOLE::GetUserInfoTask taskGetUserInfo(m_szSessionToken, &userInfo);
	int nResult = HandleTask(&taskGetUserInfo, szGetUserInfoStart);

    //-----------------------------------------------------------------
    // Check results.
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskGetUserInfo.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get user info failed for %s."),
	        m_szUsername.c_str());
	    return;
	}

    //-----------------------------------------------------------------
    // Process Results
    //-----------------------------------------------------------------
    ClientLog(UI_COMP, LOG_STATUS, false,_T("Get user info successful for %s."),
        m_szUsername.c_str());

    _tprintf(_T(" User info for %s: \n\r"), m_szUsername.c_str());
    _tprintf(_T("     Account type: %d (%s)\n\r"), userInfo.GetUserAccountType(),
        userInfo.GetUserAccountTypeName().c_str());
    _tprintf(_T("       First name: %s\n\r"), userInfo.GetFirstName().c_str() );
    _tprintf(_T("        Last name: %s\n\r"), userInfo.GetLastName().c_str() );
    _tprintf(_T("     Company name: %s\n\r"), userInfo.GetCompanyName().c_str() );
    _tprintf(_T("     Web site URL: %s\n\r"), userInfo.GetWebSiteURL().c_str() );
    _tprintf(_T("            Phone: %s\n\r"), userInfo.GetPhone().c_str() );

    std::string szFormattedDate = _T("");
    if (userInfo.GetCreatedDate()) {
        StringUtil::FormatDateAndTime(userInfo.GetCreatedDate(), szFormattedDate);
    }
    _tprintf(_T("     Created date: %s\n\r"), szFormattedDate.c_str() );

    PrintNewLine();

} // End ProcessGetUserInfoCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the update user info command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSetUserInfoCommand(CmdLine* pCmdLine,
                                               bool& bCommandFinished)
{
    // User must be logged in to update the user information.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot update user info"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Update User Info: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    DiomedeValueArg<std::string>* pFirstNameArg = NULL;
    DiomedeValueArg<std::string>* pLastNameArg = NULL;
    DiomedeValueArg<std::string>* pCompanyArg = NULL;
    DiomedeValueArg<std::string>* pWebArg = NULL;
    DiomedeValueArg<std::string>* pPhoneArg = NULL;

    try {
        pFirstNameArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_FIRSTNAME);
        pLastNameArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_LASTNAME);
        pCompanyArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_COMPANY);
        pWebArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_WEBURL);
        pPhoneArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_PHONE);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    if ( (pFirstNameArg == NULL) ||
         (pLastNameArg == NULL)||
         (pCompanyArg == NULL)||
         (pWebArg == NULL)||
         (pPhoneArg == NULL) ){

        bCommandFinished = true;
        return;
    }

    //-----------------------------------------------------------------
    // Command is complete, setup data for service
    //-----------------------------------------------------------------
    bool bUpdateNeeded = false;

    //-----------------------------------------------------------------
    // Initializes all the fields - we'll remove any quoted
    // values as they are set in the request.
    //-----------------------------------------------------------------
    UserInfoImpl userInfo;

    std::string szTemp = _T("");

    if (pFirstNameArg && pFirstNameArg->isSet()) {
        RemoveQuotesFromArgument(pFirstNameArg->getValue(), szTemp);
        userInfo.SetFirstName(szTemp);
        bUpdateNeeded = true;
    }

    if (pLastNameArg && pLastNameArg->isSet()) {
        RemoveQuotesFromArgument(pLastNameArg->getValue(), szTemp);
        userInfo.SetLastName(szTemp);
        bUpdateNeeded = true;
    }

    if (pCompanyArg && pCompanyArg->isSet()) {
        RemoveQuotesFromArgument(pCompanyArg->getValue(), szTemp);
        userInfo.SetCompanyName(szTemp);
        bUpdateNeeded = true;
    }

    if (pWebArg && pWebArg->isSet()) {
        RemoveQuotesFromArgument(pWebArg->getValue(), szTemp);
        userInfo.SetWebSiteURL(szTemp);
        bUpdateNeeded = true;
    }

    if (pPhoneArg && pPhoneArg->isSet()) {
        RemoveQuotesFromArgument(pPhoneArg->getValue(), szTemp);
        userInfo.SetPhone(szTemp);
        bUpdateNeeded = true;
    }

    if (bUpdateNeeded == false) {
        _tprintf(_T("Setuserinfo: all fields are empty, no update will occur\n\r"));
        return;
    }

    //-----------------------------------------------------------------
    // Process setuserinfo
    //-----------------------------------------------------------------
    std::string szSetUserInfoStart = _format(_T("Setting user information for %s"), m_szUsername.c_str());

	DIOMEDE_CONSOLE::SetUserInfoTask taskSetUserInfo(m_szSessionToken, &userInfo);
	int nResult = HandleTask(&taskSetUserInfo, szSetUserInfoStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
        std::string szStatusMsg = _format(_T("Set user info successful for %s."), m_szUsername.c_str());
	    PrintStatusMsg(szStatusMsg);

	    ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskSetUserInfo.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Update user info failed for %s."),
	        m_szUsername.c_str());
	}

} // End ProcessSetUserInfoCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the delete user info command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessDeleteUserInfoCommand(CmdLine* pCmdLine,
                                                  bool& bCommandFinished)
{
    // User must be logged in to update the user information.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot delete user info"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Delete User Info: you must be logged into the Diomede service."));
	    return;
    }

    // If no arguments, delete all of the user info fields.

    // Get the list of arguments - list will be added onto as more configuration
    // options are specified.  Each argument should be a /name=<value> pair.
    DiomedeMultiArg<std::string>* pKeyArg = NULL;

    try {
        pKeyArg = (DiomedeMultiArg<std::string>*)pCmdLine->getArg(ARG_DELETEUSERINFO);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_DELETEUSERINFO << endl;
    }

    if (pKeyArg == NULL) {
        return;
    }

    bCommandFinished = true;


    //-----------------------------------------------------------------
    // Check for selected fields
    //-----------------------------------------------------------------
	std::vector<std::string> listFields = pKeyArg->getValue();
	std::string szField = _T("");

    int userInfoType = 0;

	for ( int nIndex = 0; static_cast<unsigned int>(nIndex) < listFields.size(); nIndex++ ) {

        szField = listFields[nIndex];

        if (szField == ARG_FIRSTNAME) {
            userInfoType |= DIOMEDE::firstName;
        }
        if (szField == ARG_LASTNAME) {
            userInfoType |= DIOMEDE::lastName;
        }
        if (szField == ARG_COMPANY) {
            userInfoType |= DIOMEDE::companyName;
        }
        if (szField == ARG_WEBURL) {
            userInfoType |= DIOMEDE::websiteUrl;
        }
        if (szField == ARG_PHONE) {
            userInfoType |= DIOMEDE::phone;
        }
	}

	// If no arguments were received, assume the user wants to delete all
	// of the user info fields.
	if (userInfoType == 0) {
	    userInfoType = DIOMEDE::allUserInfo;
	}

    //-----------------------------------------------------------------
    // Process deleteuserinfo
    //-----------------------------------------------------------------
    std::string szDeleteUserInfoStart = _format(_T("Deleting user information for %s"), m_szUsername.c_str());

	DIOMEDE_CONSOLE::DeleteUserInfoTask taskDeleteUserInfo(m_szSessionToken, userInfoType);
	int nResult = HandleTask(&taskDeleteUserInfo, szDeleteUserInfoStart);

    //-----------------------------------------------------------------
    // Check results.
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskDeleteUserInfo.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Delete user info failed for %s."),
	        m_szUsername.c_str());
	    return;
	}

    std::string szStatusMsg = _format(_T("Delete user info for user %s successful."),
        m_szUsername.c_str());
    PrintStatusMsg(szStatusMsg);

	ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessDeleteUserInfoCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to allocate all the data
//          used for the UserInfo structure.  All the data of this
//          structure required initialization to it's type - otherwise,
//          a serialization exception will be returned by the server.
// Requires:
//      pUserInfo: reference to the UserInfo structure.
// Returns: nothing
void ConsoleControl::InitUserInfo(dds__UserInfo* pUserInfo)
{
    //-----------------------------------------------------------------
    // General User Fields
    //-----------------------------------------------------------------
	pUserInfo->firstName = new std::string(_T(""));
	pUserInfo->lastName = new std::string(_T(""));
	pUserInfo->companyName = new std::string(_T(""));
	pUserInfo->websiteUrl = new std::string(_T(""));
	pUserInfo->phone = new std::string(_T(""));

    //-----------------------------------------------------------------
    // Billing Fields
    //-----------------------------------------------------------------
	pUserInfo->cardName = new std::string(_T(""));
	pUserInfo->cardNumber = new std::string(_T(""));
	pUserInfo->cardCvv2 = new std::string(_T(""));
	pUserInfo->cardCity = new std::string(_T(""));
	pUserInfo->cardAddress1 = new std::string(_T(""));
	pUserInfo->cardAddress2 = new std::string(_T(""));
	pUserInfo->cardCity = new std::string(_T(""));
	pUserInfo->cardState  = new std::string(_T(""));
	pUserInfo->cardCountry = new std::string(_T(""));
	pUserInfo->cardZip = new std::string(_T(""));

	pUserInfo->minMonthlyFee = 0;
	pUserInfo->monthlySupportFee = 0;
	pUserInfo->cardExpiryYear = 0;
	pUserInfo->cardExpiryMonth = 0;


} // End InitUserInfo

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to deallocate all the allocated data
//          used for the UserInfo structure
// Requires:
//      pUserInfo: reference to the UserInfo structure.
// Returns: nothing
void ConsoleControl::DeleteUserInfo(dds__UserInfo* pUserInfo)
{
    //-----------------------------------------------------------------
    // General User Fields
    //-----------------------------------------------------------------
    if (pUserInfo->firstName != NULL) {
        delete pUserInfo->firstName;
        pUserInfo->firstName = NULL;
    }

    if (pUserInfo->lastName != NULL) {
        delete pUserInfo->lastName;
        pUserInfo->lastName = NULL;
    }

    if (pUserInfo->companyName != NULL) {
        delete pUserInfo->companyName;
        pUserInfo->companyName = NULL;
    }

    if (pUserInfo->websiteUrl != NULL) {
        delete pUserInfo->websiteUrl;
        pUserInfo->websiteUrl = NULL;
    }

    if (pUserInfo->phone != NULL) {
        delete pUserInfo->phone;
        pUserInfo->phone = NULL;
    }

    //-----------------------------------------------------------------
    // Billing Fields
    //-----------------------------------------------------------------
    if (pUserInfo->cardName != NULL) {
        delete pUserInfo->cardName;
        pUserInfo->cardName = NULL;
    }

    if (pUserInfo->cardNumber != NULL) {
        delete pUserInfo->cardNumber;
        pUserInfo->cardNumber = NULL;
    }

    if (pUserInfo->cardCvv2 != NULL) {
        delete pUserInfo->cardCvv2;
        pUserInfo->cardCvv2 = NULL;
    }

    if (pUserInfo->cardCity != NULL) {
        delete pUserInfo->cardCity;
        pUserInfo->cardCity = NULL;
    }

    if (pUserInfo->cardAddress1 != NULL) {
        delete pUserInfo->cardAddress1;
        pUserInfo->cardAddress1 = NULL;
    }

    if (pUserInfo->cardAddress2 != NULL) {
        delete pUserInfo->cardAddress2;
        pUserInfo->cardAddress2 = NULL;
    }

    if (pUserInfo->cardCity != NULL) {
        delete pUserInfo->cardCity;
        pUserInfo->cardCity = NULL;
    }

    if (pUserInfo->cardState != NULL) {
        delete pUserInfo->cardState;
        pUserInfo->cardState = NULL;
    }

    if (pUserInfo->cardCountry != NULL) {
        delete pUserInfo->cardCountry;
        pUserInfo->cardCountry = NULL;
    }

    if (pUserInfo->cardZip != NULL) {
        delete pUserInfo->cardZip;
        pUserInfo->cardZip = NULL;
    }

} // End DeleteUserInfo

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get email address command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetEmailAddressesCommand(CmdLine* pCmdLine,
                                                     bool& bCommandFinished)
{
    // User must be logged in to get a list of email addresses.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get email addresses"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get email addresses: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    //-----------------------------------------------------------------
    // Process getemailaddresses
    //-----------------------------------------------------------------
    std::string szGetEmailAddressesStart = _format(_T("Getting email addresses for %s"), m_szUsername.c_str());

	DIOMEDE_CONSOLE::GetEmailAddressesTask taskGetEmailAddresses(m_szSessionToken);
	int nResult = HandleTask(&taskGetEmailAddresses, szGetEmailAddressesStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskGetEmailAddresses.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get email addressess failed for %s."),
	        m_szUsername.c_str());
	    return;
	}

    ClientLog(UI_COMP, LOG_STATUS, false,_T("Get email addressess suuccessful %s."),
        m_szUsername.c_str());

    EmailListImpl* pEmailAddresses = taskGetEmailAddresses.GetEmailListResults();
    std::vector<std::string >listEmailAddresses = pEmailAddresses->GetEmailList();

    _tprintf(_T("Emails for %s: \n\r"), m_szUsername.c_str());
    for (int nIndex = 0; nIndex < (int)listEmailAddresses.size(); nIndex ++) {
        _tprintf(_T("           %s \n\r"), listEmailAddresses[nIndex].c_str());
    }
    PrintNewLine();

} // End ProcessGetEmailAddressesCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the add email address command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessAddEmailAddressCommand(CmdLine* pCmdLine,
                                                   bool& bCommandFinished)
{
    // User must be logged in to add an email address.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot add email address"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Add email address: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pEmailArg = NULL;

    try {
        pEmailArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_EMAIL);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_ADDEMAILADDRESS << endl;
    }

    if ( pEmailArg == NULL) {
        bCommandFinished = true;
        return;
    }

	if (pEmailArg->getValue().length() == 0) {

        if (pEmailArg->getRepromptCount() == 0) {
    	    m_szCommandPrompt = _T("Email: ");
            pEmailArg->incrementRepromptCount();
        }
        else if (pEmailArg->getRepromptCount() < 2) {
            pEmailArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // an email...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;

    std::string szTemp = _T("");
    RemoveQuotesFromArgument(pEmailArg->getValue(), szTemp);

    //-----------------------------------------------------------------
    // Process addemailaddress
    //-----------------------------------------------------------------
    std::string szEmail = pEmailArg->getValue();
    std::string szAddEmailAddressStart = _format(_T("Adding email address %s"), szEmail.c_str());

	DIOMEDE_CONSOLE::AddEmailAddressTask taskAddEmailAddress(m_szSessionToken, szTemp);
	int nResult = HandleTask(&taskAddEmailAddress, szAddEmailAddressStart);

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {

	    std::string szStatusMsg = _format(_T("Add email address %s to user %s successful."),
	        szEmail.c_str(), m_szUsername.c_str());
	    PrintStatusMsg(szStatusMsg);

	    ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskAddEmailAddress.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Add email address %s failed for user %s."),
	        szEmail.c_str(), m_szUsername.c_str());
	}

} // End ProcessAddEmailAddressCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the delete email address command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessDeleteEmailAddressCommand(CmdLine* pCmdLine,
                                                      bool& bCommandFinished)
{
    // User must be logged in to remove an email address.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot delete email address"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Delete email address: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pEmailArg = NULL;

    try {
        pEmailArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_EMAIL);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_DELETEEMAILADDRESS << endl;
    }

    if ( pEmailArg == NULL) {
        bCommandFinished = true;
        return;
    }

	if (pEmailArg->getValue().length() == 0) {

        if (pEmailArg->getRepromptCount() == 0) {
    	    m_szCommandPrompt = _T("Email: ");
            pEmailArg->incrementRepromptCount();
        }
        else if (pEmailArg->getRepromptCount() < 2) {
            pEmailArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // an email...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;

    std::string szTemp = _T("");
    RemoveQuotesFromArgument(pEmailArg->getValue(), szTemp);

    //-----------------------------------------------------------------
    // Process removeemailaddress
    //-----------------------------------------------------------------
    std::string szEmail = pEmailArg->getValue();
    std::string szDeleteEmailAddressStart = _format(_T("Deleting email address %s"), szEmail.c_str());

	DIOMEDE_CONSOLE::DeleteEmailAddressTask taskDeleteEmailAddress(m_szSessionToken, szTemp);
	int nResult = HandleTask(&taskDeleteEmailAddress, szDeleteEmailAddressStart);

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Delete email address %s from user %s successful."),
	        szEmail.c_str(), m_szUsername.c_str());
	    PrintStatusMsg(szStatusMsg);
	    ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskDeleteEmailAddress.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Delete email address %s failed for user %s."),
	        szEmail.c_str(), m_szUsername.c_str());
	}

} // End ProcessDeleteEmailAddressCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the set primary email address command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSetPrimaryEmailAddressCommand(CmdLine* pCmdLine,
                                                          bool& bCommandFinished)
{
    // User must be logged in to set a primary email address.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot set primary email address"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Set primary email address: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pEmailArg = NULL;

    try {
        pEmailArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_EMAIL);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SETPRIMARYEMAILADDRESS << endl;
    }

    if ( pEmailArg == NULL) {
        bCommandFinished = true;
        return;
    }

	if (pEmailArg->getValue().length() == 0) {

        if (pEmailArg->getRepromptCount() == 0) {
    	    m_szCommandPrompt = _T("Email: ");
            pEmailArg->incrementRepromptCount();
        }
        else if (pEmailArg->getRepromptCount() < 2) {
            pEmailArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // an email...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;

    std::string szTemp = _T("");
    RemoveQuotesFromArgument(pEmailArg->getValue(), szTemp);

    //-----------------------------------------------------------------
    // Process setprimaryemailaddress
    //-----------------------------------------------------------------

    // The formatted string here needs to be less than 64 characters, otherwise our
    // little trick for showing the progress on a single line fails (since the line
    // will wrap).  Since this command is a likely to wrap, we'll conditionally show
    // the email as part of the status.

    std::string szEmail = pEmailArg->getValue();
    std::string szSetPrimaryEmailAddressStart = _T("");

    if ( (int)szEmail.length() > MAX_STATUS_LEN) {
        szSetPrimaryEmailAddressStart = _format(_T("Setting primary email address"));
    }
    else {
        szSetPrimaryEmailAddressStart = _format(_T("Setting primary email address %s"),
            szEmail.c_str());
    }

	DIOMEDE_CONSOLE::SetPrimaryEmailAddressTask taskSetPrimaryEmailAddress(m_szSessionToken, szTemp);
	int nResult = HandleTask(&taskSetPrimaryEmailAddress, szSetPrimaryEmailAddressStart);

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Set primary email address %s for user %s successful."),
	        szEmail.c_str(), m_szUsername.c_str());
	    PrintStatusMsg(szStatusMsg);
	    ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskSetPrimaryEmailAddress.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Set primary email address %s failed for user %s."),
	        szEmail.c_str(), m_szUsername.c_str());
	}

} // End ProcessSetPrimaryEmailAddressCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the check account user command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessCheckAccountCommand(CmdLine* pCmdLine,
                                                bool& bCommandFinished)
{
    // User must be logged in to subscribe.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot check account"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Check account: user not logged into service."));
	    return;
    }

    bCommandFinished = true;
    int nResult = CheckAccount();

} // End ProcessCheckAccountCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper to process check account either from the check
//          command or following login.  User must be logged into
//          the Diomede service.
// Requires: nothing
// Returns: SOAP_OK, service error otherwise.
int ConsoleControl::CheckAccount()
{
    //-----------------------------------------------------------------
    // Process getuserinfo
    //-----------------------------------------------------------------
    std::string szCheckAccountStart = _format(_T("Checking account status"));

    UserInfoImpl userInfo;

	DIOMEDE_CONSOLE::GetUserInfoTask taskGetUserInfo(m_szSessionToken, &userInfo);
	int nResult = HandleTask(&taskGetUserInfo, szCheckAccountStart, _T(""), false, false, true);

    //-----------------------------------------------------------------
    // Check results.
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskGetUserInfo.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Check account status failed for %s."),
	        m_szUsername.c_str());
	    return nResult;
	}

    //-----------------------------------------------------------------
    // Process Results
    //-----------------------------------------------------------------
    ClientLog(UI_COMP, LOG_STATUS, false,_T("Check account status successful for %s."),
        m_szUsername.c_str());

    PrintNewLine();
    if (userInfo.GetUserAccountType() == freeAccount) {
        _tprintf(_T("Status: Free trial. Warning: files in trial accounts are deleted every 24 hrs.\n\r"));
        _tprintf(_T("        Type SUBSCRIBE to purchase a Diomede subscription.\n\r"));
    }
    else {
        _tprintf(_T("Status: Subscribed. Thank you, support@diomedestorage.com.\n\r"));
    }

    PrintNewLine();
    return nResult;

} // End CheckAccount

///////////////////////////////////////////////////////////////////////
// Purpose: Process the subscribe user command.  User must be logged into
//          the Diomede service and SSL must be activated.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSubscribeUserCommand(CmdLine* pCmdLine,
                                                 bool& bCommandFinished)
{
    // User must be logged in to subscribe.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot subscribe to service"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Subscribe user: user not logged into service."));
	    return;
    }

    // SSL mode also must be enabled to either default or all.
    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
    if (pProfileData) {
        int nSecurityType = pProfileData->GetUserProfileInt(GEN_SERVICE_SECURE_TYPE,
                                                            GEN_SERVICE_SECURE_TYPE_DF);
        if (nSecurityType == SECURE_SERVICE_NONE) {
	        std::string szStatusMsg = _format(_T("SSL not enabled.  Stopping"));
	        PrintStatusMsg(szStatusMsg);
	        ClientLog(UI_COMP, LOG_ERROR, false, _T("Subscribe user: ssl mode set to none."));
	        return;
        }
    }

    bCommandFinished = false;

    // Use a static here to limit the message to a one time occurrence.
    static bool bShowHelpText = true;

    if (bShowHelpText) {

        PrintNewLine();
        _tprintf(_T("SSL enabled. Information submitted here is secure.\n\r"));
        _tprintf(_T("Please enter your credit card information to enable your Diomede subscription.\n\r"));
        PrintNewLine();
    }

    bShowHelpText = false;

    int nResult = 0;
    bool bSuccess = HandleBillingDataInput(pCmdLine, bCommandFinished, nResult);

    if (bCommandFinished == true) {
        bShowHelpText = true;
    }

    // If either a syntax or validation error occurred or a service error occurred,
    // bail...
    if ( (bSuccess == false) || (nResult != SOAP_OK) || (bCommandFinished == false) ) {
        return;
    }

    // Otherwise if the command is finished, purchase the default product for
    // the user.
    std::vector<std::string> listRateIDs;
    listRateIDs.push_back(_T("1"));

    nResult = 0;
    PurchaseProduct(listRateIDs, nResult);

} // End ProcessSubscribeUserCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper to SetBillingData and Subscribe commands - handles
//          input and validation of the billing data from the user.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
//      nResult: result of service call.
// Returns: true if successful, false otherwise
bool ConsoleControl::HandleBillingDataInput(CmdLine* pCmdLine, bool& bCommandFinished,
                                            int& nResult)
{
    bCommandFinished = false;
    nResult = 0;

    DiomedeUnlabeledValueArg<std::string>* pCardNameArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pCardNumberArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pExpiresArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pCVVArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pAddress1Arg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pCityArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pStateArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pZipArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pCountryArg = NULL;
    DiomedeValueArg<std::string>* pAddress2Arg = NULL;

    try {
        pCardNameArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_BILLING_NAME);
        pCardNumberArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_BILLING_NUMBER);
        pExpiresArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_BILLING_EXPIRES);
        pCVVArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_BILLING_CVV);
        pAddress1Arg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_BILLING_ADDRESS1);
        pCityArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_BILLING_CITY);
        pStateArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_BILLING_STATE);
        pZipArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_BILLING_ZIP);
        pCountryArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_BILLING_COUNTRY);
        pAddress2Arg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_BILLING_ADDRESS2);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    if ( (pCardNameArg == NULL) ||
         (pCardNumberArg == NULL)||
         (pExpiresArg == NULL)||
         (pCVVArg == NULL)||
         (pAddress1Arg == NULL)||
         (pCityArg == NULL)||
         (pStateArg == NULL)||
         (pZipArg == NULL)||
         (pCountryArg == NULL)||
         (pAddress2Arg == NULL) ) {

        bCommandFinished = true;
	    return false;
    }

    Arg *pTmpArg = NULL;

	if (pCardNameArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Name on credit card: ");
	    pTmpArg = pCardNameArg;
	}
	else if (pCardNumberArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Card number: ");
	    pTmpArg = pCardNumberArg;
	}
	else if (pExpiresArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Expires (yyyy-mm): ");
	    pTmpArg = pExpiresArg;
	}
	else if ( pCVVArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("CVV: ");
	    pTmpArg = pCVVArg;
	}
	else if ( pAddress1Arg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Address1: ");
	    pTmpArg = pAddress1Arg;
	}
	else if ( pCityArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("City: ");
	    pTmpArg = pCityArg;
	}
	else if ( pStateArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("State: ");
	    pTmpArg = pStateArg;
	}
	else if ( pZipArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Zip: ");
	    pTmpArg = pZipArg;
	}
	else if ( pCountryArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Country: ");
	    pTmpArg = pCountryArg;
	}

	// Verify the country code
	/*
	std::string szCountry = pCountryArg->getValue();
	DioCLICountries::COUNTRY_ID countryID = CountryStrToCountryID(szCountry);
	if (countryID == DioCLICountries::COUNTRY_LAST) {
	    _tprintf(_T("Country %s is not recognized.\n\r"), szCountry.c_str());
	    return;
	}
	*/

    //-----------------------------------------------------------------
    // Validate the credit card number
    //-----------------------------------------------------------------
	std::string szValue = _T("");

	if ( ( pCardNumberArg->getValue().length() > 0 ) &&
	     ( pCardNumberArg->isValidated() == false ) ) {
	    szValue = pCardNumberArg->getValue();

	    if (ValidateCreditCard(szValue.c_str()) == false) {
	        _tprintf(_T("Invalid credit card number.  Please try again (15 or 16 digits).\n\r"));

	        pCardNumberArg->resetValue();
	        m_szCommandPrompt = _T("Card number: ");
	        pTmpArg = pCardNumberArg;

	        ClearLastArgumentFromActionStack();

	    }
	    else {
	        pCardNumberArg->isValidated(true);
	    }
	}

    //-----------------------------------------------------------------
    // Validate the CVV
    //-----------------------------------------------------------------
    szValue = _T("");

	if ( pCVVArg->getValue().length() > 0 ) {
	    szValue = pCVVArg->getValue();

	    if (ValidateCVV(szValue) == false) {
	        _tprintf(_T("Invalid credit CVV.  Please try again (3 digits).\n\r"));

	        pCVVArg->resetValue();
	        m_szCommandPrompt = _T("CVV: ");
	        pTmpArg = pCVVArg;

	        ClearLastArgumentFromActionStack();

	    }
	    else {
	        pCVVArg->isValidated(true);
	    }
	}

    //-----------------------------------------------------------------
    // Validate the expiry date
    //-----------------------------------------------------------------
	int nYear = 0;
	int nMonth = 0;
	szValue = _T("");

	if ( pExpiresArg->getValue().length() > 0 ) {
	    szValue = pExpiresArg->getValue();

	    if (ParseYearMonth(szValue, nYear, nMonth) == false) {
	        _tprintf(_T("Expires %s is not valid.\n\r"), szValue.c_str());

	        pExpiresArg->resetValue();
	        m_szCommandPrompt = _T("Expires (yyyy-mm): ");
	        pTmpArg = pExpiresArg;

	        ClearLastArgumentFromActionStack();

	    }
	    else {
	        pExpiresArg->isValidated(true);
	    }
	}

	bCommandFinished = ( ( pCardNameArg->getValue().length() > 0 ) &&
	                     ( pCardNumberArg->getValue().length() > 0 ) &&
	                     ( pExpiresArg->getValue().length() > 0 ) &&
	                     ( pCVVArg->getValue().length() > 0 ) &&
	                     ( pAddress1Arg->getValue().length() > 0 ) &&
	                     ( pCityArg->getValue().length() > 0 ) &&
	                     ( pStateArg->getValue().length() > 0 ) &&
	                     ( pZipArg->getValue().length() > 0 ) &&
	                     ( pCountryArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // the next bit of data...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return true;
	}

    //-----------------------------------------------------------------
    // Initializes all the fields
    //-----------------------------------------------------------------
    UserInfoImpl userInfo;

	userInfo.SetCardName(pCardNameArg->getValue());
	userInfo.SetCardNumber(pCardNumberArg->getValue());
	userInfo.SetCardExpiryYear(nYear);
	userInfo.SetCardExpiryMonth(nMonth);
	userInfo.SetCardCVV2(pCVVArg->getValue());
	userInfo.SetCardAddress1(pAddress1Arg->getValue());
	userInfo.SetCardAddress2(pAddress2Arg->getValue());
	userInfo.SetCardCity(pCityArg->getValue());
	userInfo.SetCardState(pStateArg->getValue());
	userInfo.SetCardZip(pZipArg->getValue());
	userInfo.SetCardCountry(pCountryArg->getValue());

	/* Not needed unless we change country back to a country code.
	int nCountry = (int)countryID;
	userInfo.Country = new int;
	*userInfo.cardCountry = nCountry;
	*/

    //-----------------------------------------------------------------
    // Process setbillingdata - uses setuserinfo for processing
    // this command.
    //-----------------------------------------------------------------
    std::string szSetUserInfoStart = _format(_T("Setting billing data for %s"),
        m_szUsername.c_str());

	DIOMEDE_CONSOLE::SetUserInfoTask taskSetUserInfo(m_szSessionToken, &userInfo);
	nResult = HandleTask(&taskSetUserInfo, szSetUserInfoStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Set billing data for user %s successful."), m_szUsername.c_str());
	    PrintStatusMsg(szStatusMsg);

	    ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskSetUserInfo.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Set billing data for user %s failed."),
	        m_szUsername.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

    return true;

} // End HandleBillingDataInput

///////////////////////////////////////////////////////////////////////
// Purpose: Process the set billing data command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSetBillingDataCommand(CmdLine* pCmdLine,
                                                  bool& bCommandFinished)
{
    // User must be logged in to set the billing data.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot set billing data"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Set billing data: user not logged into service."));
	    return;
    }

    int nResult = 0;
    HandleBillingDataInput(pCmdLine, bCommandFinished, nResult);

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessSetBillingDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get billing data command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetBillingDataCommand(CmdLine* pCmdLine,
                                                  bool& bCommandFinished)
{
    // User must be logged in to set the billing data.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get billing data"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get billing data: user not logged into service."));
	    return;
    }

    bCommandFinished = true;

    //-----------------------------------------------------------------
    // Process getbillingdata - currently, we use the getuserinfo
    // APIs - TBD a separate service for this command.
    //-----------------------------------------------------------------

    std::string szGetBillingDataStart = _format(_T("Getting billing data for %s"), m_szUsername.c_str());

    UserInfoImpl userInfo;

	DIOMEDE_CONSOLE::GetUserInfoTask taskGetUserInfo(m_szSessionToken, &userInfo);
	int nResult = HandleTask(&taskGetUserInfo, szGetBillingDataStart);

    //-----------------------------------------------------------------
    // Check results.
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskGetUserInfo.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get billing data failed for %s."),
	        m_szUsername.c_str());
	    return;
	}

    //-----------------------------------------------------------------
    // Process Results
    //-----------------------------------------------------------------
    ClientLog(UI_COMP, LOG_STATUS, false,_T("Get billing data successful for %s."),
        m_szUsername.c_str());

    _tprintf(_T("Billing data for %s: \n\r"), m_szUsername.c_str());
    _tprintf(_T("   Name on card: %s\n\r"), userInfo.GetCardName().c_str() );
    _tprintf(_T("    Card number: %s\n\r"), userInfo.GetCardNumber().c_str() );

    std::string szTemp = _T("");
    if ( userInfo.GetCardExpiryYear() && userInfo.GetCardExpiryMonth() ) {

        struct tm tmExpiryDate;
        tmExpiryDate.tm_year = userInfo.GetCardExpiryYear() - 1900;
        tmExpiryDate.tm_mon = userInfo.GetCardExpiryMonth() - 1;

        char szBuffer[10];
        strftime (szBuffer, 10, "%Y-%m", &tmExpiryDate);
        szTemp = std::string(szBuffer);
    }

    _tprintf(_T("        Expires: %s\n\r"), szTemp.c_str() );
    _tprintf(_T("            CVV: %s\n\r"), userInfo.GetCardCVV2().c_str() );
    _tprintf(_T("       Address1: %s\n\r"), userInfo.GetCardAddress1().c_str() );
    _tprintf(_T("       Address2: %s\n\r"), userInfo.GetCardAddress2().c_str() );
    _tprintf(_T("           City: %s\n\r"), userInfo.GetCardCity().c_str() );
    _tprintf(_T("          State: %s\n\r"), userInfo.GetCardState().c_str() );
    _tprintf(_T("            Zip: %s\n\r"), userInfo.GetCardZip().c_str() );
    _tprintf(_T("        Country: %s\n\r"), userInfo.GetCardCountry().c_str() );

    //-----------------------------------------------------------------
    // Show date created here as well?
    //-----------------------------------------------------------------
    std::string szFormattedDate = _T("");
    if (userInfo.GetCreatedDate()) {
        StringUtil::FormatDateAndTime(userInfo.GetCreatedDate(), szFormattedDate);
    }
    _tprintf(_T("   Created date: %s\n\r"), szFormattedDate.c_str() );

    PrintNewLine();

} // End ProcessGetBillingDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the delete billing data command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessDeleteBillingDataCommand(CmdLine* pCmdLine,
                                                     bool& bCommandFinished)
{
    // User must be logged in to set the billing data.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot delete billing data"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Delete billing data: user not logged into service."));
	    return;
    }

    // If no arguments, delete all of the billing data fields.

    // Get the list of arguments - list will be added onto as more configuration
    // options are specified.  Each argument should be a /name=<value> pair.
    DiomedeMultiArg<std::string>* pKeyArg = NULL;

    try {
        pKeyArg = (DiomedeMultiArg<std::string>*)pCmdLine->getArg(ARG_DELETEBILLINGINFO);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_DELETEBILLINGINFO << endl;
    }

    if (pKeyArg == NULL) {
        return;
    }

    bCommandFinished = true;

    //-----------------------------------------------------------------
    // Check for selected fields
    //-----------------------------------------------------------------
	std::vector<std::string> listFields = pKeyArg->getValue();
	std::string szField = _T("");

    int userInfoType = 0;

	for ( int nIndex = 0; static_cast<unsigned int>(nIndex) < listFields.size(); nIndex++ ) {

        szField = listFields[nIndex];

        if (szField == ARG_BILLING_NAME) {
            userInfoType |= DIOMEDE::cardName;
        }
        if (szField == ARG_BILLING_NUMBER) {
            userInfoType |= DIOMEDE::cardNumber;
        }
        if (szField == ARG_BILLING_EXPIRES) {
            userInfoType |= DIOMEDE::cardExpiry;
        }
        if (szField == ARG_BILLING_CVV) {
            userInfoType |= DIOMEDE::cardCvv2;
        }
        if (szField == ARG_BILLING_ADDRESS1) {
            userInfoType |= DIOMEDE::cardAddress1;
        }
        if (szField == ARG_BILLING_ADDRESS2) {
            userInfoType |= DIOMEDE::cardAddress2;
        }
        if (szField == ARG_BILLING_CITY) {
            userInfoType |= DIOMEDE::cardCity;
        }
        if (szField == ARG_BILLING_STATE) {
            userInfoType |= DIOMEDE::cardState;
        }
        if (szField == ARG_BILLING_ZIP) {
            userInfoType |= DIOMEDE::cardZip;
        }
        if (szField == ARG_BILLING_COUNTRY) {
            userInfoType |= DIOMEDE::cardCountry;
        }
    }

	// If no arguments were received, assume the user wants to delete all
	// of the billing data fields.
	if (userInfoType == 0) {
	    userInfoType = DIOMEDE::allBillingInfo;
	}

    //-----------------------------------------------------------------
    // Process deletebillingdata
    //-----------------------------------------------------------------

    std::string szDeleteBillingDataStart = _format(_T("Deleting billing data for %s"), m_szUsername.c_str());

	DIOMEDE_CONSOLE::DeleteUserInfoTask taskDeleteUserInfo(m_szSessionToken, userInfoType);
	int nResult = HandleTask(&taskDeleteUserInfo, szDeleteBillingDataStart);

    //-----------------------------------------------------------------
    // Check results.
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskDeleteUserInfo.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Delete billing data failed for %s."),
	        m_szUsername.c_str());
	    return;
	}

    std::string szStatusMsg = _format(_T("Delete billing data for user %s successful."),
        m_szUsername.c_str());
    PrintStatusMsg(szStatusMsg);

    ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessDeleteBillingDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the search payments log command.  User must be logged into
//          the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSearchPaymentsCommand(CmdLine* pCmdLine,
                                                    bool& bCommandFinished)
{
    // User must be logged in to search the payment log.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot search payment log"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Search payment log: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pStartDateArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pEndDateArg = NULL;
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pStartDateArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_STARTDATE);
        pEndDateArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_ENDDATE);
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SEARCHPAYMENTS << endl;
    }

    if ( ( pStartDateArg == NULL) || ( pEndDateArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pStartDateArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Start date (yyyy-mm-dd): ");
	    pTmpArg = pStartDateArg;
	}
	else if (pEndDateArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("End date (yyyy-mm-dd): ");
	    pTmpArg = pEndDateArg;
	}

	bCommandFinished = ( ( pStartDateArg->getValue().length() > 0 ) &&
	                     ( pEndDateArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;

    time_t epochStartSeconds;
    time_t epochEndSeconds;

    // Validate the start and end date
    std::string szStatusMsg = _T("");
    if ( false == ValidateDate(pStartDateArg->getValue(), epochStartSeconds) ) {
        szStatusMsg = _format(_T("Start date %s is not a valid date."), pStartDateArg->getValue().c_str());
        PrintStatusMsg(szStatusMsg);
        return;
    }

    if ( false == ValidateDate(pEndDateArg->getValue(), epochEndSeconds) ) {
        szStatusMsg = _format(_T("End date %s is not a valid date."), pEndDateArg->getValue().c_str());
        PrintStatusMsg(szStatusMsg);
        return;
    }

    //-----------------------------------------------------------------
    // Process SearchPayments
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::SearchPaymentLogTask taskSearchPaymentLog(m_szSessionToken, epochStartSeconds, epochEndSeconds);
	int nResult = HandleTask(&taskSearchPaymentLog, _T("Searching payment log"));

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskSearchPaymentLog.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Search payment log for range %s to %s failed."),
	        pStartDateArg->getValue().c_str(), pEndDateArg->getValue().c_str());
	    return;
	}

    ClientLog(UI_COMP, LOG_STATUS, false,
        _T("Search payment log for range %s to %s successful."),
        pStartDateArg->getValue().c_str(), pEndDateArg->getValue().c_str());

    if (pOutputArg && pOutputArg->isSet()) {
        // Change output destination to the log file
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // Process results.
    //-----------------------------------------------------------------
    PaymentLogListImpl* pListLogEntries = taskSearchPaymentLog.GetSearchPaymentLogResults();
    if (pListLogEntries == NULL) {
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    std::vector<void * >listLogEntries = pListLogEntries->GetPaymentLogList();

    // If no entries found, output a message and return.
    if ((int)listLogEntries.size() == 0) {

        std::string szStatusMsg = _format(_T("Search payment log successful.  No matches found."));
        PrintStatusMsg(szStatusMsg);

        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    //-----------------------------------------------------------------
    // locale facet for formatting currency
    //-----------------------------------------------------------------
    std::locale localEnglish(DiomedeMoneyPunct::GetLocaleString().c_str());
    std::locale locDiomede(localEnglish, new DiomedeMoneyPunct(2));
    cout.imbue(locDiomede);

    std::string szFormattedDate = _T("");

    PrintNewLine();

    for (int nIndex = 0; nIndex < (int)listLogEntries.size(); nIndex ++) {
        PaymentLogEntryImpl* logEntry = (PaymentLogEntryImpl*)listLogEntries[nIndex];

        szFormattedDate = _T("");
        if (false == FormatDate(logEntry->GetCompletedDate(), szFormattedDate) ) {
            // Output an error?
            continue;
        }

        _tprintf(_T("     Payment date: %s \n\r"), szFormattedDate.c_str());

        // _tprintf(_T("   Payment amount: %lf \n\r"), *logEntry->amount);
        // Here we're showing 2 digit precision - so *100
        double dwAmount = (double)logEntry->GetAmount();
        cout << _T("   Payment amount: ") << std::showbase << Money(dwAmount) << endl;

        _tprintf(_T("         Currency: %s \n\r"), logEntry->GetCurrency().c_str());
        PrintNewLine();
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessSearchPaymentsCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the upload file command using the SDK CPP lib.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessUploadCommand(CmdLine* pCmdLine,
                                                  bool& bCommandFinished)
{
    // User must be logged in to upload a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot upload file"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Upload file: user not logged into service."));
	    return;
    }

    /* For testing filenames....
    std::string szTestFileName = _T("");
    std::string szTestFilePath = _T("/home/pharris/DiomedeTestFiles/test.doc");
    Util::GetFileName(szTestFilePath, szTestFileName);
    cout << "Test Linux Filename: " << szTestFileName << endl << endl;
    bCommandFinished = true;
    return;
    */

    bCommandFinished = false;

    DiomedeUnlabeledMultiArg<std::string>* pFileArg = NULL;
    DiomedeSwitchArg* pRecurseArg = NULL;
    DiomedeSwitchArg* pAddPathArg = NULL;
    DiomedeSwitchArg* pCreateMD5DigestArg = NULL;

    try {
        pFileArg = (DiomedeUnlabeledMultiArg<std::string>*)pCmdLine->getArg(ARG_FILENAME);
        pRecurseArg = (DiomedeSwitchArg*)pCmdLine->getArg(ARG_RECURSE_SWITCH);
        pAddPathArg = (DiomedeSwitchArg*)pCmdLine->getArg(ARG_PATHMETADATA_SWITCH);
        pCreateMD5DigestArg = (DiomedeSwitchArg*)pCmdLine->getArg(ARG_HASHMD5_SWITCH);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_UPLOAD << endl;
    }

    if ( pFileArg == NULL) {
        bCommandFinished = true;
        return;
    }

    // Recurse through sub-directories
    bool bRecurseDirs = false;
    if (pRecurseArg && pRecurseArg->isSet()) {
        bRecurseDirs = true;
    }

    // Add the file's full path as metadata
    bool bAddPath = false;
    if (pAddPathArg && pAddPathArg->isSet()) {
        bAddPath = true;
    }

    // Create the MD5 hash for the upload.
    bool bCreateMD5Digest = false;
    if (pCreateMD5DigestArg && pCreateMD5DigestArg->isSet()) {
        bCreateMD5Digest = true;
    }

	std::vector<std::string> listFiles = pFileArg->getValue();
	if (listFiles.size() == 0) {

	    if (pFileArg->getRepromptCount() == 0) {
	        m_szCommandPrompt = _T("File(s): ");
	        pFileArg->incrementRepromptCount();
	    }
	    else if (pFileArg->getRepromptCount() < 2) {
	        pFileArg->incrementRepromptCount();
	    }
	    else {
	        // We've prompted them already - alert them that they need
	        // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
	    }

	    return;
	}

	/*
	for ( int i = 0; static_cast<unsigned int>(i) < listFiles.size(); i++ )
		cout << "[  ] " << i << "  " <<  listFiles[i] << endl;
    */

    bCommandFinished = true;

    std::string szFilePath = _T("");
    LONG64 l64TotalBytesUploaded = 0;
    int nTotalFilesUploaded = 0;

    // To make sure there is no data leftover from a prior call.
    if (m_pTaskCreateFile != NULL) {
        delete m_pTaskCreateFile;
        m_pTaskCreateFile = NULL;
    }

    if (m_pTaskSetFileMetaData != NULL) {
        delete m_pTaskSetFileMetaData;
        m_pTaskSetFileMetaData = NULL;
    }

    if (m_pTaskUpload != NULL) {
        delete m_pTaskUpload;
        m_pTaskUpload = NULL;
    }

    if (m_pUploadInfo != NULL) {
        delete m_pUploadInfo;
        m_pUploadInfo = NULL;
    }

    m_pUploadInfo = new UploadFileInfo();
    if ( false == CheckThread(&m_pUploadInfo->m_commandThread, _T("Upload"))) {
        delete m_pUploadInfo;
        m_pUploadInfo = NULL;
        return;
    }

    m_pUploadInfo->ClearAll();

    // "Cache" of created metadata's to allow us to reuse metadata's created
    // for a given path.
    m_listMetaDataFullPaths.clear();
    m_listMetaDataRelativePaths.clear();

    //-----------------------------------------------------------------
    // Setup our resume information - open/create our resume files for
    // storing the progress of the upload.
    //-----------------------------------------------------------------
    m_pUploadInfo->m_resumeUploadInfoData.SetAddMetaData(bAddPath);

    bool bIsFirstRun = false;
    int nResumeResult = ResumeManager::Instance()->OpenResumeData(RESUME_UPLOAD_FILENAME, bIsFirstRun);
    if (nResumeResult != 0) {
        // If an error occurs, the files will be deleted - not a big issue
        // since we'll assume loosing the data is not critical.
        PrintResumeError(nResumeResult, _T("Error initializing resume"));
    }

    //-----------------------------------------------------------------
    // Total time and bytes for the upload
    //-----------------------------------------------------------------
    m_tdTotalUpload = time_duration(0, 0, 0, 0);
    m_l64TotalUploadedBytes = 0;

    std::string szParentDir = _T("");
    std::string szFileName = _T("");

    PrintNewLine();

	for ( int nIndex = 0; static_cast<unsigned int>(nIndex) < listFiles.size(); nIndex++ ) {

        // To make sure there is no data leftover from a prior call.
        if (m_pDisplayFileEnumInfo != NULL) {
            delete m_pDisplayFileEnumInfo;
            m_pDisplayFileEnumInfo = NULL;
        }

        m_pDisplayFileEnumInfo = new DisplayFileEnumInfo();
        m_pDisplayFileEnumInfo->ClearAll();
        
        m_pDisplayFileEnumInfo->m_msgTimer.Start("");
        m_pDisplayFileEnumInfo->m_msgTimer.ContinueTime("");

        szFilePath = listFiles[nIndex];

        // If the user has used wildcards as part of this path, we need to
        // enumerate the files once again.
        if (m_pFileEnumerator != NULL) {
            delete m_pFileEnumerator;
            m_pFileEnumerator = NULL;
        }

        m_pFileEnumerator = new CEnum();
        
	    #ifdef WIN32
	        m_pFileEnumerator->m_bNoCaseFiles = true;
	    #endif

	    szParentDir = _T("");
	    szFileName = _T("");

        if (false == Util::IsDirectory(szFilePath.c_str())) {
            Util::GetFileName(szFilePath, szFileName);
	        Util::GetParentDirFromDirPath(szFilePath, szParentDir);
	        
	        // If the user has added a -s for recursion, notify the user with
	        // a warning, but allow the command to continue.
            if (bRecurseDirs) {
                std::string szStatusMsg = 
                    _format(_T("...%s is a file.  The recursion argument is ignored."), 
                    szFileName.c_str());
                PrintStatusMsg(szStatusMsg);
                bRecurseDirs = false;
            }
        }
        else {
            szParentDir = szFilePath;
        }

	    m_pFileEnumerator->m_szIncPatternFiles = szFileName;
	    m_pFileEnumerator->m_bRecursive = bRecurseDirs;
	    m_pFileEnumerator->m_bFullPath = true;
	    m_pFileEnumerator->m_pfnStatusFunc = &DisplayFileEnumStatus;
	    m_pFileEnumerator->m_pUserData = this;

        /* REMOVE AFTER TESTING...
 	    m_pFileEnumerator->EnumerateAll(szParentDir);
 	    
 	    // If the user cancelled during the enumeration, quit now...
 	    if (m_pFileEnumerator->m_bCancelled) {
 	        break;
 	    }
 	    */

        // Make sure we can trap CTRL+C
        SetConsoleControlHandler();

        // The enumeration is moved to another thread to ensure we can
        // quit without having to sleep.
        DIOMEDE_CONSOLE::EnumerateFilesTask taskEnumerateFiles(m_pFileEnumerator, szParentDir);

        DIOMEDE_CONSOLE::CommandThread commandThread;
        if (false == CheckThread(&commandThread, _T("Enumerate Files"))) {
            return;
        }

        BOOL bReturn = commandThread.Event(&taskEnumerateFiles);

        if (bReturn == FALSE) {
            return;
        }

	    while ( taskEnumerateFiles.Status() != TaskStatusCompleted ) {
 	        if (m_pFileEnumerator->m_bCancelled) {
 	            break;
 	        }
	    }

        commandThread.Stop();

        if (m_pFileEnumerator->m_bCancelled) {
            break;
        }

	    std::list<std::basic_string<TCHAR> >* pListFiles = m_pFileEnumerator->GetFiles();
	    std::list<std::basic_string<TCHAR> >::iterator iter = pListFiles->begin();

	    /*
        cout << "Parent Directory: " << szParentDir << endl;
        int nFileListIndex = 0;
	    for(; iter != pListFiles->end(); ++iter, ++nFileListIndex) {
		    cout << "[" << nFileListIndex << "]" << "  " <<  iter->c_str() << endl;
	    }
	    return;
        */

        int nResult = 0;

        if (pListFiles->size() == 0) {
            ClientLog(UI_COMP, LOG_ERROR, false, _T("UploadFileBlocks for %s."),
                szFilePath.c_str());

            m_pUploadInfo->ClearAll();
            ResumeManager::Instance()->ClearResumeMgrData();

            m_pUploadInfo->m_szFilePath = szFilePath;
            m_pUploadInfo->m_szParentDir = szParentDir;
            m_pUploadInfo->m_resumeUploadInfoData.SetFilePath(szFilePath);

            time_t tmLastModified;
            int nResult = Util::GetFileLastModifiedTime(szFilePath.c_str(), tmLastModified);
            if (nResult != -1) {
                m_pUploadInfo->m_resumeUploadInfoData.SetLastModified(tmLastModified);
            }

            nResult = UploadFileBlocks(bAddPath, bCreateMD5Digest);

            l64TotalBytesUploaded = m_l64TotalUploadedBytes;
            if (nResult == 0) {
                nTotalFilesUploaded ++;
            }
        }
        else {
            // Loop through the list of files
	        for(; iter != pListFiles->end(); ++iter) {

	            std::string szTmpFilePath = (*iter);

                ClientLog(UI_COMP, LOG_STATUS, false, _T("UploadFileBlocks for %s."),
                    szTmpFilePath.c_str());

	            m_l64TotalUploadedBytes = 0;

                m_pUploadInfo->ClearAll();
                ResumeManager::Instance()->ClearResumeMgrData();

                m_pUploadInfo->m_szFilePath = szTmpFilePath;
                m_pUploadInfo->m_szParentDir = szParentDir;
                m_pUploadInfo->m_resumeUploadInfoData.SetFilePath(szTmpFilePath);

                time_t tmLastModified;
                int nResult = Util::GetFileLastModifiedTime(szTmpFilePath.c_str(), tmLastModified);
                if (nResult != -1) {
                    m_pUploadInfo->m_resumeUploadInfoData.SetLastModified(tmLastModified);
                }

                nResult = UploadFileBlocks(bAddPath, bCreateMD5Digest);

                l64TotalBytesUploaded += m_l64TotalUploadedBytes;
                if (nResult == 0) {
                    nTotalFilesUploaded ++;
                }
                else if (g_bSessionError == true) {
                    // If we still have an error, quit - it's unlikely at this point
                    // that we can recover if we haven't recovered already.
                    break;
                }
                else if (nResult == DIOMEDE_COMMAND_STOPPED_BY_USER) {
                    // If the upload has been cancelled (e.g. user entered CTRL+C),
                    // quit the entire upload process.
                    break;
                }
                else if (nResult == DIOMEDE_CREATE_THREAD_ERROR) {
                    // It's unlikely we can recover from this...
                    break;
                }
	        }
        }
	}

    //-----------------------------------------------------------------
    // Handle the CTRL+C here - this can occur if the user quits 
    // during the enumeration process.
    //-----------------------------------------------------------------
    if (g_bUsingCtrlKey) {
        g_bUsingCtrlKey = false;
        PrintStatusMsg("Cancelled!", true);
        return;
    }

	/* Return ?
	if (nTotalFilesUploaded == 0) {
	    return;
	}
	*/

	// Lots of casting here to eliminate truncation - calculate total seconds
	// from the accumulated milliseconds.

    //-----------------------------------------------------------------
    // Calculate bandwidth
    //-----------------------------------------------------------------
	std::string szBandwidth = _T("");
	std::string szBandwidthType = _T("");

	StringUtil::FormatBandwidth(l64TotalBytesUploaded, m_tdTotalUpload, szBandwidth, szBandwidthType);

    //-----------------------------------------------------------------
    // Format duration
    //-----------------------------------------------------------------
	std::string szDuration = _T("");
	std::string szDurationType = _T("");

	StringUtil::FormatDuration(m_tdTotalUpload, szDuration, szDurationType);

    //-----------------------------------------------------------------
    // Format total bytes
    //-----------------------------------------------------------------
    std::string szBytes = _T("");
    std::string szBytesSizeType = _T("");
    StringUtil::FormatByteSize(l64TotalBytesUploaded, szBytes, szBytesSizeType);

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
	std::string szFiles = (nTotalFilesUploaded == 1) ? _T("file") : _T("files");

    if (nTotalFilesUploaded > 0) {
	    _tprintf(_T("Done. %d %s uploaded, %s %s, %s %s, %s %s.\n\r"),
	        nTotalFilesUploaded, szFiles.c_str(), szBytes.c_str(), szBytesSizeType.c_str(),
	        szDuration.c_str(), szDurationType.c_str(),
	        szBandwidth.c_str(), szBandwidthType.c_str());
	}
	else {
	    // No files uploaded - no bandwidth is shown.
	    _tprintf(_T("Done. %d %s uploaded, %s %s.\n\r"),
	        nTotalFilesUploaded, szFiles.c_str(), szBytes.c_str(), szBytesSizeType.c_str(),
	        szDuration.c_str(), szDurationType.c_str());
	}

    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

} // End ProcessUploadCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the resume file command using the SDK CPP lib.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessResumeCommand(CmdLine* pCmdLine,
                                                  bool& bCommandFinished)
{
    // User must be logged in to resume uploading a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot resume uploading files"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Resume upload file: user not logged into service."));
	    return;
    }

    bCommandFinished = true;

    DiomedeUnlabeledMultiArg<std::string>* pFileArg = NULL;
    DiomedeSwitchArg* pListArg = NULL;
    DiomedeSwitchArg* pClearListArg = NULL;
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pFileArg = (DiomedeUnlabeledMultiArg<std::string>*)pCmdLine->getArg(ARG_FILENAME);
        pListArg = (DiomedeSwitchArg*)pCmdLine->getArg(ARG_RESUME_LIST_SWITCH);
        pClearListArg = (DiomedeSwitchArg*)pCmdLine->getArg(ARG_RESUME_CLEAR_SWITCH);
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_RESUME << endl;
    }

    bool bResumeAll = false;
    bool bArgIsSet = false;

    // If no files have been specified, assume all.  If the file isn't among the resumable
    // files, upload as normal.
    std::vector<std::string> listFiles;

    if (pFileArg) {
	    listFiles = pFileArg->getValue();
	    if (listFiles.size() == 0) {
	        bResumeAll = true;
	    }
	    else {
	        bArgIsSet = true;
	    }
	}

    // List the files in the resume-able list - this will happen first - we could use this
    // opportunity to prompt to continue...
    bool bListFiles = false;
    std::string szSetting = _T("");

    // We only store incomplete files, so we'll list them all.
    ResumeInfoType listResumeInfoType = resumeTypeUndefined;
    if (pListArg && pListArg->isSet()) {
        bListFiles = true;
    }

    bool bVerboseOutput = VerboseOutput(pCmdLine);

    // The list will be cleared at the end of everything else (any other actions,
    // except resume?).
    bool bClearList = false;
    szSetting = _T("");

    // We only store incomplete files, so we'll clear them all.
    ResumeInfoType clearResumeInfoType = resumeUploads;
    if (pClearListArg && pClearListArg->isSet()) {
        bClearList = true;
        bArgIsSet = true;

        clearResumeInfoType = ResumeInfoType(resumeTypeUndefined | resumeUploads);
    }

    //-----------------------------------------------------------------
    // List the files currently waiting for upload
    //-----------------------------------------------------------------
    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    if (bListFiles || bVerboseOutput) {
        if (bVerboseOutput) {
            DisplayResumeUploadListVerbose(listResumeInfoType);
        }
        else {
            DisplayResumeUploadList(listResumeInfoType);
        }

        // If no other arugments, just return (TBD)
        if ( bArgIsSet == false ) {
            SimpleRedirect::Instance()->EndRedirect();
            return;
        }
    }

    // Clear the files if required
    int nResult = 0;
    if (bClearList) {
        nResult = ResumeManager::Instance()->ClearResumeFiles(clearResumeInfoType);
        if (nResult != 0) {
            PrintResumeWarning(nResult, _T("Resume files could not be cleared "));
        }
        return;
    }

    SimpleRedirect::Instance()->EndRedirect();

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    LONG64 l64TotalBytesUploaded = 0;
    int nTotalFilesUploaded = 0;

    // To make sure there is no data leftover from a prior call.
    if (m_pTaskCreateFile != NULL) {
        delete m_pTaskCreateFile;
        m_pTaskCreateFile = NULL;
    }

    if (m_pTaskSetFileMetaData != NULL) {
        delete m_pTaskSetFileMetaData;
        m_pTaskSetFileMetaData = NULL;
    }

    if (m_pTaskUpload != NULL) {
        delete m_pTaskUpload;
        m_pTaskUpload = NULL;
    }

    if (m_pUploadInfo != NULL) {
        delete m_pUploadInfo;
        m_pUploadInfo = NULL;
    }

    m_pUploadInfo = new UploadFileInfo();
    if ( false == CheckThread(&m_pUploadInfo->m_commandThread, _T("Upload"))) {
        delete m_pUploadInfo;
        m_pUploadInfo = NULL;
        return;
    }

    m_pUploadInfo->ClearAll();

    //-----------------------------------------------------------------
    // Setup our resume information - open/create our resume files for
    // storing the progress of the upload.
    //-----------------------------------------------------------------
    bool bIsFirstRun = false;
    nResult = ResumeManager::Instance()->OpenResumeData(RESUME_UPLOAD_FILENAME, bIsFirstRun);
    if (nResult != 0) {
        // If an error occurs, the files will be deleted - not a big issue
        // since we'll assume loosing the data is not critical.
        PrintResumeError(nResult, _T("Error initializing resume"));
    }

    //-----------------------------------------------------------------
    // Get the resume data for any files given as arguments - or all
    // if no files given.
    //-----------------------------------------------------------------
    t_resumeUploadInfoList listResumeUploadInfo =
        ResumeManager::Instance()->GetResumeUploadInfoList(listFiles, true);

    //-----------------------------------------------------------------
    // Total time and bytes for the upload
    //-----------------------------------------------------------------
    std::string szFilePath = _T("");
    LONG64 l64FileID = 0;

    ResumeUploadInfoData resumeUploadInfoData;

    m_tdTotalUpload = time_duration(0, 0, 0, 0);
    m_l64TotalUploadedBytes = 0;

    bool bAddPath = false;
    bool bCreateMD5Digest = false;
    nResult = 0;

    if ( listResumeUploadInfo.size() == 0) {
        PrintStatusMsg("No resumable uploads available.");
        ClientLog(UI_COMP, LOG_ERROR, false,_T("Resume: no files found to resume."));
        return;
    }

    PrintNewLine();

	for (t_resumeUploadInfoList::iterator iter = listResumeUploadInfo.begin();
	     iter != listResumeUploadInfo.end(); iter++) {

		resumeUploadInfoData = *iter;

		// If by chance the file is finished uploading and it's still in our
		// list (maybe should assert here), skip it...
		if ( resumeUploadInfoData.GetFileSize() == resumeUploadInfoData.GetBytesRead()) {

            resumeUploadInfoData.SetResumeIntervalType(resumeIntervalDone);
            resumeUploadInfoData.SetResumeInfoType(ResumeInfoType(resumeDone | resumeUploads));

            WriteResumeUploadData(resumeUploadInfoData, _T("Process resume command"));
		    continue;
		}

        szFilePath = resumeUploadInfoData.GetFilePath();
        l64FileID = resumeUploadInfoData.GetFileID();

        m_pUploadInfo->ClearAll();
        ResumeManager::Instance()->ClearResumeMgrData();

        m_pUploadInfo->m_szFilePath = szFilePath;
        m_pUploadInfo->m_l64FileID = l64FileID;

        // Copy the stored data to our upload data structure.  The important item
        // here for resuming is to set the total upload bytes to reflect
        // what has been upload already.
        m_pUploadInfo->m_resumeUploadInfoData = resumeUploadInfoData;

        m_l64TotalUploadedBytes = resumeUploadInfoData.GetBytesRead();

        bAddPath = resumeUploadInfoData.GetAddMetaData();
        bCreateMD5Digest = resumeUploadInfoData.GetCreateMD5Hash();

        nResult = UploadFileBlocks(bAddPath, bCreateMD5Digest, m_l64TotalUploadedBytes);

        l64TotalBytesUploaded += m_l64TotalUploadedBytes;
        if (nResult == 0) {
            nTotalFilesUploaded ++;
        }
        else if (g_bSessionError == true) {
            // If we still have an error, quit - it's unlikely at this point
            // that we can recover if we haven't recovered already.
            break;
        }
    }

    //-----------------------------------------------------------------
    // Calculate bandwidth
    //-----------------------------------------------------------------
	std::string szBandwidth = _T("");
	std::string szBandwidthType = _T("");

	StringUtil::FormatBandwidth(l64TotalBytesUploaded, m_tdTotalUpload, szBandwidth, szBandwidthType);

    //-----------------------------------------------------------------
    // Format duration
    //-----------------------------------------------------------------
	std::string szDuration = _T("");
	std::string szDurationType = _T("");

	StringUtil::FormatDuration(m_tdTotalUpload, szDuration, szDurationType);

    //-----------------------------------------------------------------
    // Format total bytes
    //-----------------------------------------------------------------
    std::string szBytes = _T("");
    std::string szBytesSizeType = _T("");
    StringUtil::FormatByteSize(l64TotalBytesUploaded, szBytes, szBytesSizeType);

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
	std::string szFiles = (nTotalFilesUploaded == 1) ? _T("file") : _T("files");

    if (nTotalFilesUploaded > 0) {
	    _tprintf(_T("Done. %d %s uploaded, %s %s, %s %s, %s %s.\n\r"),
	        nTotalFilesUploaded, szFiles.c_str(), szBytes.c_str(), szBytesSizeType.c_str(),
	        szDuration.c_str(), szDurationType.c_str(),
	        szBandwidth.c_str(), szBandwidthType.c_str());
    }
	else {
	    // No files uploaded - no bandwidth is shown.
	    _tprintf(_T("Done. %d %s uploaded, %s %s.\n\r"),
	        nTotalFilesUploaded, szFiles.c_str(), szBytes.c_str(), szBytesSizeType.c_str(),
	        szDuration.c_str(), szDurationType.c_str());
	}

    if ( ( (int)listResumeUploadInfo.size() > 0)&& !m_bSysCommandInput ) {
        PrintNewLine();
    }

} // End ProcessResumeCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to return the resume filter type as a string
// Requires: nothing
// Returns: string respresenting the resume filter type.
std::string GetResumeFilterType(ResumeInfoType nResumeInfoType)
{
    std::string szResumeInfoType = _T("");

    if ( nResumeInfoType == resumeTypeUndefined ) {
        szResumeInfoType = _T("");
    }
    else if (nResumeInfoType == resumeDone) {
        szResumeInfoType = _T("complete ");
    }
    else {
        szResumeInfoType = _T("incomplete ");
    }

    return szResumeInfoType;

} // End GetResumeFilterType

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to display the list of items in the
//          resume upload list.
// Requires: nothing
// Returns: true if successful, false otherwise.
bool ConsoleControl::DisplayResumeUploadList(ResumeInfoType nResumeInfoType)
{
    int nResult = ResumeManager::Instance()->Load(ResumeInfoTypes::resumeUploads);
    if (nResult != 0) {
        if (nResult == RESUME_ZERO_FILE_LENGTH) {
            PrintStatusMsg(_T("No resumable uploads available."));
            return true;
        }

        PrintResumeError(nResult, _T("Error loading resume data"));
        return false;
    }

    t_resumeUploadInfoList listResumeUploadInfo = ResumeManager::Instance()->GetResumeUploadInfoList();

    if (listResumeUploadInfo.size() == 0) {
        return false;
    }

     // Run through the results to find the length we need for file IDs
    int nMaxFileIDLen = 0;
    std::string szOutFileID = _T("");
    int nFileIDLen = 0;

    std::vector<std::string> listFileID;

    ResumeUploadInfoData resumeUploadInfoData;
    int nIndex = 0;

    for (nIndex = 0; nIndex < (int)listResumeUploadInfo.size(); nIndex ++) {
        resumeUploadInfoData = listResumeUploadInfo[nIndex];

        // The list may have files which have been completed - those
        // will be skipped...
        if ( ( nResumeInfoType != resumeTypeUndefined ) &&
             ( nResumeInfoType != resumeUploadInfoData.GetResumeInfoType() ) ) {
            listFileID.push_back(_T(""));
            continue;
        }

        #ifdef WIN32
            szOutFileID = _format(_T("%I64d"), resumeUploadInfoData.GetFileID());
        #else
            szOutFileID = _format(_T("%lld"), resumeUploadInfoData.GetFileID());
        #endif

        // Save the formatted string to use in the final output
        listFileID.push_back(szOutFileID);
        nFileIDLen = (int)szOutFileID.length();
        nMaxFileIDLen = (nFileIDLen > nMaxFileIDLen) ? nFileIDLen : nMaxFileIDLen;
    }

    // Filename space ~= 79 - 19 (date+time) - 11 (bytes read) - 7 (percent) - (max file ID length + 3)
    int nDateTimeSpace = 19;
    int nFileNameSpace = 54 - nDateTimeSpace - nMaxFileIDLen;

    std::string szFormattedDate = _T("");
	std::vector<std::string> dateTime;

    std::string szFileSize = _T("");
    std::string szFileSizeType = _T("");
    std::string szOutFileSize = _T("");
    std::string szPercent = _T("");

    std::string szFilePath = _T("");

    int nPercent = 0;
    std::string szFormat = _T("");

    int nCountDisplayed = 0;
    LONG64 l64TotalBytes = 0;

    PrintNewLine();

    for (nIndex = 0; nIndex < (int)listResumeUploadInfo.size(); nIndex ++) {
        resumeUploadInfoData = listResumeUploadInfo[nIndex];

        // The list may have files which have been completed - those
        // will be skipped...
        if ( ( nResumeInfoType != resumeTypeUndefined ) &&
             ( nResumeInfoType != resumeUploadInfoData.GetResumeInfoType() ) ) {
            continue;
        }

        //-------------------------------------------------------------
        // Last uploaded date
        //-------------------------------------------------------------
        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(resumeUploadInfoData.GetLastStart(), szFormattedDate);

        // We need to split the date + time into a date and a time string
        // to allow us to allign them.
        int nCount = SplitString(szFormattedDate, dateTime, false);
        if (nCount < 2) {
            // Something is wrong with this entry - skip it....
            continue;
        }

        nCountDisplayed ++;

        // Date and time, 1 space between, 3 spaces afterwards -
        // see below for file size.
        _tprintf(_T("%s %s"), dateTime[0].c_str(), dateTime[1].c_str());

        //-------------------------------------------------------------
        // Bytes uploaded
        //-------------------------------------------------------------

        // Max the byte size in each category to 3 digits, e.g. 1000 KB = .97 MB
        LONG64 l64FileSize = resumeUploadInfoData.GetFileSize();
        LONG64 l64BytesRead = resumeUploadInfoData.GetBytesRead();

	    StringUtil::FormatByteSize(l64BytesRead, szFileSize, szFileSizeType, 999);

        nPercent =  (l64BytesRead == l64FileSize) ? 100 :
            (int) (((float)l64BytesRead/(float)l64FileSize) * 100);
        szPercent = _format(_T(" (%d%%)"), nPercent);

        // Figure out how much is needed for our file size column.
        // Create a variable length format string for the string..

        szFormat = _T("");
        int nCountFileSize = 11;
        int nFileIDPad = 4;
        int nPercentLength = szPercent.length();

        // Pad the percent string to make them all the same size.
        szPercent = szPercent + GetPadStr(7 - nPercentLength);
        nPercentLength = szPercent.length();

 	    szFileSize = szFileSize + _T(" ") + szFileSizeType + szPercent;

        #if 1
            // Better way to align the file size types
            if (szFileSizeType.length() < 2) {
                szFileSizeType += _T(" ");
            }

            nCountFileSize += nPercentLength + 1;
            nFileIDPad --;
        #else
            if (szFileSizeType.length() < 2) {
                nCountFileSize += nPercentLength;
            }
            else if (szFileSizeType.length() < 3) {
                nCountFileSize += nPercentLength + 1;
	            nFileIDPad --;
            }
        #endif

        szFormat = _format(_T("%%%ds"), nCountFileSize);
        _tprintf(szFormat.c_str(), szFileSize.c_str());

        //-------------------------------------------------------------
	    // File size
        //-------------------------------------------------------------
        #if 0
        // Leave this out pending comments - makes the line really
        // long.
	    szFileSize = _T("");
	    szFileSizeType = _T("");
	    szOutFileSize = _T("");

        // Max the byte size in each category to 3 digits, e.g. 1000 KB = .97 MB
	    StringUtil::FormatByteSize(l64FileSize, szFileSize, szFileSizeType, 999);

        szFormat = _T("");
        nCountFileSize = 11;

 	    szFileSize = szFileSize + _T(" ") + szFileSizeType;

        if (szFileSizeType.length() > 2) {
            nCountFileSize ++;
        }

        szFormat = _format(_T("%%%ds"), nCountFileSize);
        _tprintf(szFormat.c_str(), szFileSize.c_str());
        #endif

        //-------------------------------------------------------------
	    // File ID - 3 spaces + max width for all file IDs, right aligned
        //-------------------------------------------------------------
        szOutFileID = listFileID[nIndex];
        nFileIDLen = (int)szOutFileID.length();

        szOutFileID = GetPadStr(nMaxFileIDLen - nFileIDLen + nFileIDPad) + szOutFileID;

	    _tprintf(_T("%s"), szOutFileID.c_str());

        //-------------------------------------------------------------
	    // File path, truncated with ... if needed.  Our total line length is
	    // 79 - at 80, a console window set to 80 will wrap.
        //-------------------------------------------------------------
        int nFileNamePad = 3;

        // Show the complete file path, trimmed to 35 characters for non-verbose display
	    szFilePath = _T("");
	    TrimFileName(nFileNameSpace, resumeUploadInfoData.GetFilePath(), szFilePath);

	    szFilePath = GetPadStr(nFileNamePad) + szFilePath;
	    _tprintf(_T("%-s"), szFilePath.c_str());

        l64TotalBytes += resumeUploadInfoData.GetFileSize();

        dateTime.clear();
        PrintNewLine();
	}

    std::string szTotalBytes = _T("");
    szFileSizeType = _T("");

    StringUtil::FormatByteSize(l64TotalBytes, szTotalBytes, szFileSizeType);

    std::string szFilterType = GetResumeFilterType(nResumeInfoType);
    std::string szFilesText = _format(_T("%sfile"), szFilterType.c_str());

    if ( nCountDisplayed > 1 ) {
        szFilesText = _format(_T("%sfiles"), szFilterType.c_str());
    }

    // This is getting ugly - in order to add "incomplete" or "complete" to the
    // text, we need to append it to the total bytes.  To align the bytes, we
    // need to justify it along with the bytes in the list above, which is
    // about 23 characters - since this isn't time critical, we'll use cout...
    std::string szResults = _format(_T(", %d %s"), nCountDisplayed, szFilesText.c_str());

    std::string szTotalBytesText = szTotalBytes + _T(" ") + szFileSizeType;
    std::string szPad = GetPadStr(31 - szTotalBytesText.length());

    std::cout << szPad << szTotalBytesText << szResults << endl;

    if (!m_bSysCommandInput) {
	    PrintNewLine();
	}

	return true;

} // End DisplayResumeUploadList

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to display the list of items in the
//          resume upload list.  All data is displayed.
// Requires: nothing
// Returns: true if successful, false otherwise.
bool ConsoleControl::DisplayResumeUploadListVerbose(ResumeInfoType nResumeInfoType)
{
    int nResult = ResumeManager::Instance()->Load(ResumeInfoTypes::resumeUploads);
    if (nResult != 0) {

        if (nResult == RESUME_ZERO_FILE_LENGTH) {
            PrintStatusMsg(_T("No resume data found."));
            return true;
        }

        PrintResumeError(nResult, _T("Error loading resume data"));
        return false;
    }

    t_resumeUploadInfoList listResumeUploadInfo = ResumeManager::Instance()->GetResumeUploadInfoList();

    if (listResumeUploadInfo.size() == 0) {
        return false;
    }

    std::string szTempNumber = _T("");
    std::string szDefaultNumber = _T("0");

    std::string szFormattedDate = _T("");
    std::string szFileSize = _T("");
    std::string szFileSizeType = _T("");
    LONG64 l64FileID = 0;
    int nPercent = 0;

    std::string szFilePath = _T("");
    ResumeUploadInfoData resumeUploadInfoData;

    int nCountDisplayed = 0;
    LONG64 l64TotalBytes = 0;

    PrintNewLine();

	for (t_resumeUploadInfoList::iterator iter = listResumeUploadInfo.begin();
	     iter != listResumeUploadInfo.end(); iter++) {

		resumeUploadInfoData = *iter;

        if ( ( nResumeInfoType != resumeTypeUndefined ) &&
             ( nResumeInfoType != resumeUploadInfoData.GetResumeInfoType() ) ) {
            continue;
        }

        nCountDisplayed ++;

        //-------------------------------------------------------------
		// File ID
        //-------------------------------------------------------------

		// May be zero if the file failed to upload during the "create file" portion.
        l64FileID = resumeUploadInfoData.GetFileID();
        #ifdef WIN32
            _tprintf(_T("    Logical file ID: %I64d \n\r"), l64FileID);
        #else
            _tprintf(_T("    Logical file ID: %lld \n\r"), l64FileID);
        #endif

        //-------------------------------------------------------------
        // File path
        //-------------------------------------------------------------

        // Show the complete file path, trimmed to 58 characters
	    szFilePath = _T("");
	    TrimFileName(58, resumeUploadInfoData.GetFilePath(), szFilePath);
        _tprintf(_T("               File: %s \n\r"), szFilePath.c_str());


	    szTempNumber = _T("");
	    szFileSize = _T("");
	    szFileSizeType = _T("");

        // File size
        // Max the byte size in each category to 3 digits, e.g. 1000 KB = .97 MB
        LONG64 l64FileSize = resumeUploadInfoData.GetFileSize();
	    StringUtil::FormatByteSize(l64FileSize, szFileSize, szFileSizeType, 999);

        if (l64FileSize > 1024) {
            StringUtil::FormatNumber(l64FileSize, szTempNumber, szDefaultNumber);
            _tprintf(_T("          File size: %s %s (%s bytes) \n\r"),
                szFileSize.c_str(), szFileSizeType.c_str(), szTempNumber.c_str());
        }
        else {
            _tprintf(_T("          File size: %s %s \n\r"), szFileSize.c_str(), szFileSizeType.c_str());
        }

	    szTempNumber = _T("");
	    szFileSize = _T("");
	    szFileSizeType = _T("");

        //-------------------------------------------------------------
        // Bytes uploaded
        //-------------------------------------------------------------

        // Max the byte size in each category to 3 digits, e.g. 1000 KB = .97 MB
        LONG64 l64BytesRead = resumeUploadInfoData.GetBytesRead();
	    StringUtil::FormatByteSize(l64BytesRead, szFileSize, szFileSizeType, 999);

        nPercent =  (l64BytesRead == l64FileSize) ? 100 :
            (int) (((float)l64BytesRead/(float)l64FileSize) * 100);

        if (l64BytesRead > 1024) {
            StringUtil::FormatNumber(l64BytesRead, szTempNumber, szDefaultNumber);
            _tprintf(_T("     Bytes uploaded: %s %s (%s bytes) (%d%%) \n\r"),
                szFileSize.c_str(), szFileSizeType.c_str(), szTempNumber.c_str(), nPercent);
        }
        else {
            _tprintf(_T("     Bytes uploaded: %s %s  (%d%%) \n\r"),
                szFileSize.c_str(), szFileSizeType.c_str(), nPercent);
        }


        //-------------------------------------------------------------
        // Dates: file modified, first and last uploaded
        //-------------------------------------------------------------

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(resumeUploadInfoData.GetLastModified(), szFormattedDate);
        _tprintf(_T(" Last modified date: %s \n\r"), szFormattedDate.c_str());

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(resumeUploadInfoData.GetFirstStart(), szFormattedDate);
        _tprintf(_T("First uploaded date: %s \n\r"), szFormattedDate.c_str());

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(resumeUploadInfoData.GetLastStart(), szFormattedDate);
        _tprintf(_T(" Last uploaded date: %s \n\r"), szFormattedDate.c_str());

        l64TotalBytes += resumeUploadInfoData.GetFileSize();

        PrintNewLine();

	}

    std::string szTotalBytes = _T("");
    szFileSizeType = _T("");

    StringUtil::FormatByteSize(l64TotalBytes, szTotalBytes, szFileSizeType);

    std::string szFilterType = GetResumeFilterType(nResumeInfoType);
    std::string szFilesText = _format(_T("%sfile"), szFilterType.c_str());

    if ( nCountDisplayed > 1 ) {
        szFilesText = _format(_T("%sfiles"), szFilterType.c_str());
    }

    // This is getting ugly - in order to add "incomplete" or "complete" to the
    // text, we need to append it to the total bytes.  To align the bytes, we
    // need to justify it along with the bytes in the list above, which is
    // about 23 characters - since this isn't time critical, we'll use cout...
    std::string szResults = _format(_T(", %d %s"), nCountDisplayed, szFilesText.c_str());

    std::string szTotalBytesText = szTotalBytes + _T(" ") + szFileSizeType;
    std::string szPad = GetPadStr(31 - szTotalBytesText.length());

    std::cout << szPad << szTotalBytesText << szResults << endl;

    if (!m_bSysCommandInput) {
	    PrintNewLine();
	}

	return true;

} // End DisplayResumeUploadListVerbose

///////////////////////////////////////////////////////////////////////
// Purpose: Called from the callback which updates the upload
//          process.
// Requires:
//      nUploadStatus: status of the upload process.
//      l64CurrentBytes: bytes uploaded so far.
// Returns: nothing
void ConsoleControl::UpdateUploadStatus(int nUploadStatus, LONG64 l64CurrentBytes)
{
    if (m_pUploadInfo == NULL) {
        return;
    }

    m_pUploadInfo->m_nUploadStatus = nUploadStatus;
    m_pUploadInfo->m_nBytesRead = static_cast<int>(m_l64TotalUploadedBytes - l64CurrentBytes);

    std::string szUploadFile = _T("");
    int nPercent = 0;

    // We'll only update the status when the send is complete.  Status is sent when
    // the "send" occurs as well, but here we'll track only the completed sent.
    // To show a bit more responsiveness, we'll also handle the create file status
    // messages.
    bool bHandled = true;

    switch (nUploadStatus) {
        case DIOMEDE::uploadCreateFile:
            szUploadFile = _format(_T("%s (%s %s)... creating "),
                m_pUploadInfo->m_szFormattedFileName.c_str(), m_pUploadInfo->m_szFormattedBytes.c_str(),
                m_pUploadInfo->m_szFormattedBytesType.c_str(), nPercent);

            if (m_pUploadInfo->m_msgTimer.IsStarted() == false) {
                m_pUploadInfo->m_msgTimer.Start(szUploadFile);
            }
            m_pUploadInfo->m_msgTimer.ContinueTime(szUploadFile);
            g_bCanContinueTimer = true;
            break;
        case DIOMEDE::uploadCreateFileComplete:
            // The "false" here allows us to show progress on the same line - we'll
            // restart the time once the upload has started.
            g_bCanContinueTimer = false;
            m_pUploadInfo->m_msgTimer.EndTime(_T(""), EndTimerTypes::useSingleLongLine, false);
            m_pUploadInfo->m_msgTimer.SetShowProgress(false);
            break;
        case DIOMEDE::uploadStarted:
           {
                // In the case of resume, we can calculate the percent completed so far.
	            nPercent =  0;
                if (m_l64TotalUploadedBytes > 0) {
	                nPercent =  (m_l64TotalUploadedBytes == m_pUploadInfo->m_l64FileSize) ? 100 :
	                    (int) (((float)m_l64TotalUploadedBytes/(float)m_pUploadInfo->m_l64FileSize) * 100);
                }
                szUploadFile = _format(_T("%s (%s %s)... %d%%"),
                    m_pUploadInfo->m_szFormattedFileName.c_str(),
                    m_pUploadInfo->m_szFormattedBytes.c_str(),
                    m_pUploadInfo->m_szFormattedBytesType.c_str(), nPercent);

                // Restart the timer - if we're resuming mid-upload (and not via the
                // "resume" command), the timer is merely paused and does not need
                // restarting.
                if ( m_pUploadInfo->m_msgTimer.IsPaused() == false ) {
                    m_pUploadInfo->m_msgTimer.SetShowProgress(true);
                    m_pUploadInfo->m_msgTimer.Start(szUploadFile);
                }
                g_bCanContinueTimer = true;
            }
            break;

        case DIOMEDE::uploadSendComplete:
           {
                // To allow resume, the upload bytes must be updated on success only.
                m_l64TotalUploadedBytes = l64CurrentBytes;

	            nPercent =  (m_l64TotalUploadedBytes == m_pUploadInfo->m_l64FileSize) ? 100 :
	                (int) (((float)m_l64TotalUploadedBytes/(float)m_pUploadInfo->m_l64FileSize) * 100);

                szUploadFile = _format(_T("%s (%s %s)... %d%%"),
                    m_pUploadInfo->m_szFormattedFileName.c_str(),
                    m_pUploadInfo->m_szFormattedBytes.c_str(),
                    m_pUploadInfo->m_szFormattedBytesType.c_str(), nPercent);

                m_pUploadInfo->m_resumeUploadInfoData.ResetNextResumeIntervalType();
                m_pUploadInfo->m_resumeUploadInfoData.SetBytesRead(m_l64TotalUploadedBytes);

                WriteResumeUploadData(m_pUploadInfo->m_resumeUploadInfoData, _T("Upload callback"));

                // In the normal uplaod scenario, this check is handled during
                // the create file portion.  With resume, the file has already
                // been created, so we need to start the timer here.
                if (m_pUploadInfo->m_msgTimer.IsStarted() == false) {
                    m_pUploadInfo->m_msgTimer.Start(szUploadFile);
                }
                m_pUploadInfo->m_msgTimer.ContinueTime(szUploadFile);
            }
            break;
        default:
            bHandled = false;
            break;
    }

    if (!bHandled) {
        return;
    }

    if (nUploadStatus == DIOMEDE::uploadSendComplete) {
        m_pUploadInfo->m_nCurrentBlock ++;
    }

} // End UpdateUploadStatus

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to repeat the upload task, handled
//          separately due to the nature of the output required.
// Requires:
//      pTask: reference to a CTask derived object
// Returns: SOAP_OK if successful, service error otherwise.
int ConsoleControl::RepeatLastUploadTask(DiomedeTask* pTask)
{
    int nOriginalResult = pTask->GetResult();
    int nResult = 0;
    BOOL bReturn = FALSE;

    MessageTimer msgTimer;

    if (g_bSessionError) {

        g_bSessionError = false;

        // Keep the timer running, alert the user tht the original task ended
        // with session expired.
        EndTimerTypes::EndTimerType timerType = EndTimerTypes::useSingleLine;

        if ( (g_nSessionErrorType >= 0) &&
             (g_nSessionErrorType < ConsoleControl::LAST_SERVICE_TYPES_ENUM)) {

             if ( (g_nSessionErrorType == ConsoleControl::SESSION_TOKEN_EXPIRES) ||
                  (g_nSessionErrorType == ConsoleControl::INVALID_SESSION_TOKEN) ) {
                  timerType = EndTimerTypes::useSessionExpired;
             }
             else {
                  timerType = EndTimerTypes::useNetworkConnectionError;
             }
        }

        //-------------------------------------------------------------
        // Pause the create file/upload timer.
        //-------------------------------------------------------------
        m_pUploadInfo->m_msgTimer.PauseTime(_T(""), timerType);

        //-------------------------------------------------------------
        //-------------------------------------------------------------
        std::string szLoginUserStart = _format(_T("Login user %s"), m_szUsername.c_str());
	    DIOMEDE_CONSOLE::LoginTask taskLogin(m_szUsername, m_szPlainTextPassword);

        // Using a separate timer here for login...
        msgTimer.Start(szLoginUserStart);

        bReturn = m_pUploadInfo->m_commandThread.Event(&taskLogin);
        if (bReturn == FALSE) {
            return nOriginalResult;
        }

	    while ( taskLogin.Status() != TaskStatusCompleted ) {
            msgTimer.ContinueTime();
            PauseProcess();
	    }

	    msgTimer.EndTime(_T(""), EndTimerTypes::useNoTimeOrDone);
        nResult = taskLogin.GetResult();

        // If this fails, just bounce out - something must be amiss with the
        // service.
        if (nResult != SOAP_OK) {
            return nResult;
        }

        m_szSessionToken = taskLogin.GetSessionToken();
        g_nSessionRetries = MAX_LOGIN_RETRIES;

	    ClientLog(UI_COMP, LOG_STATUS, false,
	        _T("Repeat last command: new session token %s."), m_szSessionToken.c_str());

        UserProfileData* pProfileData =
            ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
        if (pProfileData) {
            // If the "save" here fails, do we care?  This feature is more a nicety for the
            // user.
	        pProfileData->SetUserProfileStr(GEN_SESSION_TOKEN, m_szSessionToken.c_str());
	        pProfileData->SetUserProfileLong(GEN_SESSION_TOKEN_EXPIRES, (long)UpdateSessionTokenExpiration());
	        pProfileData->SaveUserProfile();
	    }
    }

    PrintNewLine();

    // This will be set to true on the once the upload file status
    // has occurred.
    g_bCanContinueTimer = false;

    pTask->ResetTask();
    pTask->SetSessionToken(m_szSessionToken);

    bReturn = m_pUploadInfo->m_commandThread.Event(pTask);
    if (bReturn == FALSE) {
        return nOriginalResult;
    }

    while ( pTask->Status() != TaskStatusCompleted ) {
        if (g_bCanContinueTimer) {
            m_pUploadInfo->m_msgTimer.ContinueTime();
        }
        PauseProcess();
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
    nResult = pTask->GetResult();
    return nResult;

} // End RepeatLastUploadTask

///////////////////////////////////////////////////////////////////////
// Purpose: Handles upload of a single file in blocks.  Helper function to
//          ProcessUploadCommand.
// Requires:
//      bAddPathMetaData: /t addss the full path as metadata to the file.
//      bCreateMD5Digest: /md5 creates and uses an MD5 digest to upload the file.
//      l64TotalUploadedBytes: 0 or the total uploaded bytes for a given file
//                             so far (resume).
// Returns: 0 if successful, error code otherwise
int ConsoleControl::UploadFileBlocks(bool bAddPathMetaData /*false*/,
                                     bool bCreateMD5Digest /*false*/,
                                     LONG64 l64TotalUploadedBytes /*0*/)
{
    // Verify that the file exists.
    if (Util::DoesFileExist(m_pUploadInfo->m_szFilePath) == false) {
        std::string szStatusMsg = _format(_T("...Skipping %s:  File does not exist or is not accessible."),
                 m_pUploadInfo->m_szFilePath.c_str());
        PrintStatusMsg(szStatusMsg);
        ClientLog(UI_COMP, LOG_ERROR, false, _T("File %s is not accessible."),
                  m_pUploadInfo->m_szFilePath.c_str());
        return DIOMEDE_INVALID_FILEPATH;
    }

    // File size may have been set already with resume
    if (m_pUploadInfo->m_l64FileSize <= 0) {
        m_pUploadInfo->m_l64FileSize = Util::GetFileLength64(m_pUploadInfo->m_szFilePath.c_str());
        m_pUploadInfo->m_resumeUploadInfoData.SetFileSize(m_pUploadInfo->m_l64FileSize);
    }

    if (m_pUploadInfo->m_l64FileSize <= 0) {
        // Continue or upload anyway?
        _tprintf(_T("...Skipping %s:  File has zero length.\n\r"), m_pUploadInfo->m_szFilePath.c_str());
        ClientLog(UI_COMP, LOG_ERROR, false, _T("File %s has zero length."),
                  m_pUploadInfo->m_szFilePath.c_str());
        return DIOMEDE_ZERO_FILE_LENGTH;
    }

    //-----------------------------------------------------------------
    // In case of resume, set up the first and last start time values.
    //-----------------------------------------------------------------
    time_t rawTime;
    time ( &rawTime );

    if (m_pUploadInfo->m_resumeUploadInfoData.GetFirstStart() == 0) {
        m_pUploadInfo->m_resumeUploadInfoData.SetFirstStart(rawTime);
    }
    m_pUploadInfo->m_resumeUploadInfoData.SetLastStart(rawTime);

    //-----------------------------------------------------------------
    // Format the file size to show for status purposes.
    //-----------------------------------------------------------------
    StringUtil::FormatByteSize(m_pUploadInfo->m_l64FileSize, m_pUploadInfo->m_szFormattedBytes,
        m_pUploadInfo->m_szFormattedBytesType);

    //----------------------------------------------------------------
    // We need the filename without the path.
    //----------------------------------------------------------------
    m_pUploadInfo->m_szFileName = _T("");
    Util::GetFileName(m_pUploadInfo->m_szFilePath, m_pUploadInfo->m_szFileName);

    // Trim the file name for status purposes.
    m_pUploadInfo->m_szFormattedFileName = m_pUploadInfo->m_szFileName;
    if (m_pUploadInfo->m_szFileName.length() > 30) {
        TrimFileName(30, m_pUploadInfo->m_szFileName, m_pUploadInfo->m_szFormattedFileName);
    }

    //----------------------------------------------------------------
    // At this point, the resume data is complete enough to write
    // the initial data out to our resume files.
    //----------------------------------------------------------------
    #if 0
    // 4/15/2010: Decision to NOT added files with no file ID - this
    // to simplify logic...
    ResumeManager::Instance()->WriteResumeUploadData(RESUME_UPLOAD_FILENAME,
                                                     m_pUploadInfo->m_resumeUploadInfoData);
    #endif

    //----------------------------------------------------------------
    // Setup the upload data structure - in the normal upload
    // scenario, the total upload bytes is 0.  When resuming,
    // this value should be set the total value uploaded thus
    // far.
    //----------------------------------------------------------------
    UploadImpl uploadData;
    uploadData.SetFilePath(m_pUploadInfo->m_szFilePath);
    uploadData.SetFileName(m_pUploadInfo->m_szFileName);
    uploadData.SetFileID(m_pUploadInfo->m_l64FileID);

    uploadData.SetTotalUploadBytes(l64TotalUploadedBytes);
    uploadData.SetHashMD5(m_pUploadInfo->m_szHashMD5);
    uploadData.SetUploadCallback(&UploadStatus);
    uploadData.SetUploadUser(this);

    // Allow internal configurable chunk size.
    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );

    if (pProfileData != NULL) {
        int nChunkSize =
            pProfileData->GetUserProfileInt(GEN_MAX_CHUNK_SIZE, GEN_MAX_CHUNK_SIZE_DF);
        if (nChunkSize != GEN_MAX_CHUNK_SIZE_DF) {
            uploadData.SetMaxChunkSize(nChunkSize);
        }

        nChunkSize = pProfileData->GetUserProfileInt(GEN_MIN_CHUNK_SIZE, GEN_MIN_CHUNK_SIZE_DF);
        if (nChunkSize != GEN_MIN_CHUNK_SIZE_DF) {
            uploadData.SetMinChunkSize(nChunkSize);
        }

        bool bLogStatus =
            (pProfileData->GetUserProfileInt(GEN_LOG_UPLOAD, GEN_LOG_UPLOAD_DF) != 0);
        uploadData.SetLogStatus(bLogStatus);
    }

    g_bCanContinueTimer = false;

    //-----------------------------------------------------------------
    // Create the file and then begin the uploading
    //-----------------------------------------------------------------
    if (m_pUploadInfo->m_l64FileID == 0) {

        int nCreateFileResult = CreateFile(&uploadData);

        if (nCreateFileResult != 0) {
            return nCreateFileResult;
        }

        //-----------------------------------------------------------------
        // Process the /m option if present.
        //-----------------------------------------------------------------
        if (bAddPathMetaData) {
            if (m_pUploadInfo->m_l64FileID > 0) {
                // If we're adding metadata, get the relative path.
                Util::RelativePathTo(m_pUploadInfo->m_szParentDir, m_pUploadInfo->m_szFilePath,
                                     m_pUploadInfo->m_szRelativePath);

                //-----------------------------------------------------
                // For debugging from the console.
                //-----------------------------------------------------
                /*
    	        _tprintf(_T("Uploaded file metadata: parent path %s and relative path %s\n\r"),
    	            m_pUploadInfo->m_szParentDir.c_str(), m_pUploadInfo->m_szRelativePath.c_str());
                */

                DiomedeStorageService storageService;
                bool bIsInCache = false;
                int nMetaDataID = -1;

                std::string szParentDir = _T("");
                Util::GetParentDirFromDirPath(m_pUploadInfo->m_szFilePath, szParentDir);

                if (m_pUploadInfo->m_szParentDir.length() > 0) {
                    bIsInCache = CheckMetaDataCache(g_mdSourceFullPath, szParentDir,
                                                    nMetaDataID);
                    if (bIsInCache) {
                         SetFileMetaData(m_pUploadInfo->m_l64FileID, nMetaDataID, false);
                    }
                    else {
                        // Create the metadata, adding to the cache if successful.
                        if (true == CreateFileMetaData(m_pUploadInfo->m_l64FileID, g_mdSourceFullPath,
                            szParentDir, nMetaDataID, false) ) {
                            m_listMetaDataFullPaths.insert(std::make_pair(m_pUploadInfo->m_szParentDir, nMetaDataID));
                        }
                    }
                }
                if (m_pUploadInfo->m_szRelativePath.length() > 0) {
                    nMetaDataID = -1;
                    bIsInCache = CheckMetaDataCache(g_mdSourceRelativePath, m_pUploadInfo->m_szRelativePath,
                                                    nMetaDataID);
                    if (bIsInCache) {
                         SetFileMetaData(m_pUploadInfo->m_l64FileID, nMetaDataID, false);
                    }
                    else {
                        // Create the metadata, adding to the cache if successful.
                        if (true == CreateFileMetaData(m_pUploadInfo->m_l64FileID, g_mdSourceRelativePath,
                            m_pUploadInfo->m_szRelativePath, nMetaDataID, false) ) {
                            m_listMetaDataRelativePaths.insert(std::make_pair(m_pUploadInfo->m_szRelativePath, nMetaDataID));
                        }
                    }
                }
            }
            else {
                _tprintf(_T("Metadata not set for uploaded file %s...invalid file ID returned from the server.\n\r "),
                     m_pUploadInfo->m_szFilePath.c_str());
            }
        }

        //-----------------------------------------------------------------
        // End Add metadata following file creation
        //-----------------------------------------------------------------
    }

    //-----------------------------------------------------------------
    // Process upload
    //-----------------------------------------------------------------
	if (m_pTaskUpload == NULL) {
	    m_pTaskUpload = new DIOMEDE_CONSOLE::UploadTask(m_szSessionToken, &uploadData);
	}
	else {
	    // Clear out any errors, reset the session token and upload data.
	    // File Manager data, including the thread, remains intact.
	    m_pTaskUpload->ResetTask();
	    m_pTaskUpload->SetSessionToken(m_szSessionToken);
	    m_pTaskUpload->SetUploadImpl(&uploadData);
	}

    m_pUploadInfo->m_commandThread.Event(m_pTaskUpload);

    while ( m_pTaskUpload->Status() != TaskStatusCompleted ) {
        if (g_bCanContinueTimer) {
            m_pUploadInfo->m_msgTimer.ContinueTime();
        }

        if (false == PauseProcess() ) {
            m_pTaskUpload->CancelTask();
        }
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    
    // Clear the control key usage bool now that the thread has quit.
    g_bUsingCtrlKey = false;

    int nResult = m_pTaskUpload->GetResult();

    if (nResult == SOAP_OK) {
        // Handle the scenario where an error occurs - may not be a status
        // yet, in which case we'd spin around here unnecessarily.
        while (m_pUploadInfo->m_nUploadStatus < DIOMEDE::uploadComplete) {
            // Waiting for status to change to complete - callback
            // into this object updates the upload status.
            m_pUploadInfo->m_msgTimer.ContinueTime();
        }
    }

    //-------------------------------------------------------------
    // Check results
    //-------------------------------------------------------------

    if (nResult != SOAP_OK) {
        std::string szErrorMsg = m_pTaskUpload->GetServiceErrorMsg();

	    // Should we try to resume the upload?
	    if (CheckServiceErrorToResume(szErrorMsg)) {

	        bool bUserCancelled = false;
	        int nResumeResult = ResumeCurrentUpload(m_pTaskUpload, bUserCancelled);

	        if (nResumeResult != SOAP_OK) {
	            // ResumeCurrentUpload will handle any errors that may occur.
                return nResumeResult;
	        }
	        else {
	            nResult = nResumeResult;
	        }
	    }
	    else {
	        // Kill the timer...
            time_duration elapsedTimeWithError = m_pUploadInfo->m_msgTimer.EndTime(_T(""),
                EndTimerTypes::useSingleLine);
            m_tdTotalUpload += elapsedTimeWithError;

	        // In normal upload, we don't need to "retry" since any session errors
	        // will be caught in CreateFile...
	        PrintServiceError(stderr, szErrorMsg);
	        ClientLog(UI_COMP, LOG_ERROR, false,_T("Upload failed: (%d) %s"), nResult, szErrorMsg.c_str());
            return nResult;
        }
    }

    // If an error has occurred, we'll return above without handling the
    // last "done".   Reset the text to get rid of the number of bytes
    // upload thus far...

    #ifdef WIN32
        std::string szFileID = _format(_T("%I64d"), m_pUploadInfo->m_l64FileID);
    #else
        std::string szFileID = _format(_T("%lld"), m_pUploadInfo->m_l64FileID);
    #endif

    std::string szUploadFile = _format(_T("%s: %s (%s %s)"),
        szFileID.c_str(),
        m_pUploadInfo->m_szFormattedFileName.c_str(), m_pUploadInfo->m_szFormattedBytes.c_str(),
        m_pUploadInfo->m_szFormattedBytesType.c_str());
    m_pUploadInfo->m_msgTimer.SetStartText(szUploadFile);

    time_duration elapsedTime = m_pUploadInfo->m_msgTimer.EndTime(_T(""),
        EndTimerTypes::useSingleLine);
    m_tdTotalUpload += elapsedTime;

    /* For testing ...
    ClientLog(UI_COMP, LOG_STATUS, false, _T("Total Upload Milliseconds %I64d"), l64TotalMilliseconds);
    ClientLog(UI_COMP, LOG_STATUS, false, _T("Total Upload Milliseconds for file %s - %I64d (%I64d ms)"), szFileName.c_str(),
        elapsedTime.total_milliseconds(), elapsedTime.fractional_seconds() );
    */


    // And lastly, update the resume data to indicate this file is done.
    m_pUploadInfo->m_resumeUploadInfoData.SetResumeIntervalType(resumeIntervalDone);
    m_pUploadInfo->m_resumeUploadInfoData.SetResumeInfoType(resumeDone);

    WriteResumeUploadData(m_pUploadInfo->m_resumeUploadInfoData, _T("UploadFileBlocks"));

    return nResult;

} // End UploadFileBlocks

///////////////////////////////////////////////////////////////////////
// Purpose: Handles the initial file creation.  Helper function to
//          ProcessUploadCommand.
// Requires:
//      pUploadData: upload data used by the FileManager for communicating with the
//                   service.
// Returns: 0 if successful, error code otherwise
int ConsoleControl::CreateFile(UploadImpl* pUploadData)
{
    int nResult = 0;
    if (m_pUploadInfo->m_l64FileID != 0) {
        // Probably should assert since this is a programmer error.
        return DIOMEDE_INVALID_FILEID;
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------
    if (m_pTaskCreateFile == NULL) {
        m_pTaskCreateFile = new DIOMEDE_CONSOLE::CreateFileTask(m_szSessionToken, pUploadData);
    }
	else {
	    // Clear out any errors, reset the session token and upload data.
	    // File Manager data, including the thread, remains intact.
	    m_pTaskCreateFile->ResetTask();
	    m_pTaskCreateFile->SetSessionToken(m_szSessionToken);
	    m_pTaskCreateFile->SetUploadImpl(pUploadData);
	}

    m_pUploadInfo->m_commandThread.Event(m_pTaskCreateFile);

    while ( m_pTaskCreateFile->Status() != TaskStatusCompleted ) {
        if (g_bCanContinueTimer) {
            m_pUploadInfo->m_msgTimer.ContinueTime();
        }
        PauseProcess();
    }

    //-------------------------------------------------------------
    // Check results
    //-------------------------------------------------------------
    nResult = m_pTaskCreateFile->GetResult();

    if (nResult == SOAP_OK) {
        m_pUploadInfo->m_l64FileID = m_pTaskCreateFile->GetFileID();

        // Update the resume data as well.
        m_pUploadInfo->m_resumeUploadInfoData.SetFileID(m_pTaskCreateFile->GetFileID());

        WriteResumeUploadData(m_pUploadInfo->m_resumeUploadInfoData, _T("Create file"));
        return nResult;
    }

    // Else check the return code to determine if the task can be repeated (e.g.
    // the session has expired) or resumed.

    std::string szFriendlyMsg = _T("");
    std::string szErrorMsg = m_pTaskCreateFile->GetServiceErrorMsg();

    // Should we try to resume the create file?
    if (CheckServiceErrorToResume(szErrorMsg)) {

        bool bUserCancelled = false;
        nResult = ResumeCreateFile(m_pTaskCreateFile, bUserCancelled);

        if (nResult == SOAP_OK) {
            m_pUploadInfo->m_l64FileID = m_pTaskCreateFile->GetFileID();

            // Update the resume data as well.
            m_pUploadInfo->m_resumeUploadInfoData.SetFileID(m_pTaskCreateFile->GetFileID());

            WriteResumeUploadData(m_pUploadInfo->m_resumeUploadInfoData, _T("Create file retry service to resume"));
            return nResult;
        }
    }
    else if ( CheckServiceErrorToRetry(szErrorMsg, szFriendlyMsg)) {

        // The session retries is reset to MAX_RETRIES as soon as the
        // user is successfully logged back into the service.
        g_bSessionError = true;
        g_nSessionRetries --;

        if (g_nSessionRetries >= 0) {
            nResult = RepeatLastCreateFileTask(m_pTaskCreateFile);
        }

        if (nResult == SOAP_OK) {
            m_pUploadInfo->m_l64FileID = m_pTaskCreateFile->GetFileID();

            // Update the resume data as well.
            m_pUploadInfo->m_resumeUploadInfoData.SetFileID(m_pTaskCreateFile->GetFileID());

            WriteResumeUploadData(m_pUploadInfo->m_resumeUploadInfoData, _T("Create file retry service"));
            return nResult;
        }
        // Else the error returned from the service is returned.
    }

    // The error is unrecoverable - bail
    szErrorMsg = m_pTaskCreateFile->GetServiceErrorMsg();
    PrintServiceError(stderr, szErrorMsg, true);

    ClientLog(UI_COMP, LOG_ERROR, false,_T("Failed to create file %s."),
        m_pUploadInfo->m_szFileName.c_str());

    return m_pTaskCreateFile->GetResult();

} // End CreateFile

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to repeat the create file task, handled
//          separately due to the nature of the output required.
// Requires:
//      pTask: reference to a CTask derived object
// Returns: SOAP_OK if successful, service error otherwise.
int ConsoleControl::RepeatLastCreateFileTask(DiomedeTask* pTask)
{
    int nOriginalResult = pTask->GetResult();
    int nResult = 0;
    BOOL bReturn = FALSE;

    MessageTimer msgTimer;

    if (g_bSessionError) {

        g_bSessionError = false;

        // Keep the timer running, alert the user tht the original task ended
        // with session expired.
        EndTimerTypes::EndTimerType timerType = EndTimerTypes::useSingleLine;

        if ( (g_nSessionErrorType >= 0) &&
             (g_nSessionErrorType < ConsoleControl::LAST_SERVICE_TYPES_ENUM)) {

             if ( (g_nSessionErrorType == ConsoleControl::SESSION_TOKEN_EXPIRES) ||
                  (g_nSessionErrorType == ConsoleControl::INVALID_SESSION_TOKEN) ) {
                  timerType = EndTimerTypes::useSessionExpired;
             }
             else {
                  timerType = EndTimerTypes::useNetworkConnectionError;
             }
        }

        // Stop the create file/upload timer.
        m_pUploadInfo->m_msgTimer.EndTime(_T(""), timerType);

        std::string szLoginUserStart = _format(_T("Login user %s"), m_szUsername.c_str());
	    DIOMEDE_CONSOLE::LoginTask taskLogin(m_szUsername, m_szPlainTextPassword);

        // Using a separate timer here for login...
        msgTimer.Start(szLoginUserStart);

        bReturn = m_pUploadInfo->m_commandThread.Event(&taskLogin);
        if (bReturn == FALSE) {
            return nOriginalResult;
        }

	    while ( taskLogin.Status() != TaskStatusCompleted ) {
            msgTimer.ContinueTime();
            PauseProcess();
	    }

	    msgTimer.EndTime(_T(""), EndTimerTypes::useNoTimeOrDone);
        nResult = taskLogin.GetResult();

        // If this fails, just bounce out - something must be amiss with the
        // service.
        if (nResult != SOAP_OK) {
            return nResult;
        }

        m_szSessionToken = taskLogin.GetSessionToken();
        g_nSessionRetries = MAX_LOGIN_RETRIES;

	    ClientLog(UI_COMP, LOG_STATUS, false,
	        _T("Repeat last command: new session token %s."), m_szSessionToken.c_str());

        UserProfileData* pProfileData =
            ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
        if (pProfileData) {
            // If the "save" here fails, do we care?  This feature is more a nicety for the
            // user.
	        pProfileData->SetUserProfileStr(GEN_SESSION_TOKEN, m_szSessionToken.c_str());
	        pProfileData->SetUserProfileLong(GEN_SESSION_TOKEN_EXPIRES, (long)UpdateSessionTokenExpiration());
	        pProfileData->SaveUserProfile();
	    }
    }

    PrintNewLine();

    // This will be set to true on the once the create file status
    // has occurred.
    g_bCanContinueTimer = false;

    pTask->ResetTask();
    pTask->SetSessionToken(m_szSessionToken);

    bReturn = m_pUploadInfo->m_commandThread.Event(pTask);
    if (bReturn == FALSE) {
        return nOriginalResult;
    }

    while ( pTask->Status() != TaskStatusCompleted ) {
        if (g_bCanContinueTimer) {
            m_pUploadInfo->m_msgTimer.ContinueTime();
        }
        PauseProcess();
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
    nResult = pTask->GetResult();
    return nResult;

} // End RepeatLastCreateFileTask

///////////////////////////////////////////////////////////////////////
// Purpose: Resume, if possible, create file.  If the error
//          is such that resume is not possible, return control
//          back to the main create file handler.
// Requires:
//      pTask: reference to a create file task object
//      bUserCancelled: true if the file is successfully uploaded, false
//                       otherwise.
// Returns: SOAP_OK if successful, service error otherwise.
int ConsoleControl::ResumeCreateFile(DIOMEDE_CONSOLE::CreateFileTask* pTask, bool& bUserCancelled)
{
    int nOriginalResult = pTask->GetResult();

    UploadImpl* pUploadImpl = pTask->GetUploadImpl();
    ResumeUploadInfoData* pResumeInfo = &m_pUploadInfo->m_resumeUploadInfoData;

    // On the first run, the interval type is undefined.  Set it to the
    // first interval (or the next if on re-entry).
    ResumeInfoIntervalType nResumeInterval = pResumeInfo->GetNextResumeIntervalType();

    int nResult = 0;
    BOOL bReturn = FALSE;
    std::string szErrorMsg = _T("");

    // Additional sleep to play nice on Windows....

    // Keep the timer running, alert the user that the original task ended
    // with a connection error.  The following line of code screws up VS2005
    // intellisense - seems that if barfs on the enums...
    m_pUploadInfo->m_msgTimer.PauseTime(_T(""), EndTimerTypes::useNetworkConnectionError, false);

    DIOMEDE_CONSOLE::CommandThread commandThread;
    if (false == CheckThread(&commandThread, _T("Upload"))) {
        return DIOMEDE::DIOMEDE_CREATE_THREAD_ERROR;
    }

    bool bContinue = false;

    while (nResumeInterval < resumeIntervalDone) {
        szErrorMsg = _T("");
        bContinue = ResumeCountdown(nResumeInterval);

        if (bContinue == false) {
            PrintStatusMsg(_T("Resume cancelled."), true);
            bUserCancelled = true;
            return nOriginalResult;
        }

        // Has the file changed?
        if ( pResumeInfo->HasResumeFileChanged() ) {
            PrintStatusMsg(_T("File has changed.  Resume failed."), true);
            return nOriginalResult;
       }

        pTask->ResetTask();

        bReturn = commandThread.Event(pTask);
        if (bReturn == FALSE) {
            return nOriginalResult;
        }

	    while ( pTask->Status() != TaskStatusCompleted ) {
            if (g_bCanContinueTimer) {
                m_pUploadInfo->m_msgTimer.ContinueTime();
            }
            PauseProcess();
	    }

        //-----------------------------------------------------------------
        // Check results
        //-----------------------------------------------------------------
        nResult = pTask->GetResult();

        if (nResult == SOAP_OK) {
            // We're done - break out to allow the upload process to
            // complete our upload tasks (e.g. add metadata, etc.)
            break;
        }
        else {
            std::string szFriendlyMsg = _T("");
            szErrorMsg = pTask->GetServiceErrorMsg();

	        // Should we try to resume the upload?
	        if ( CheckServiceErrorToResume(szErrorMsg) ) {
                m_pUploadInfo->m_msgTimer.PauseTime(_T(""), EndTimerTypes::useNetworkConnectionError, false);
                nResumeInterval = pResumeInfo->GetNextResumeIntervalType();
	        }
	        else if ( CheckServiceErrorToRetry(szErrorMsg, szFriendlyMsg)) {
	            // Session expired so we'll need to relogin and then continue
	            // where we left off.
                g_bSessionError = true;
                g_nSessionRetries --;

                if (g_nSessionRetries >= 0) {
	                nResult = RepeatLastCreateFileTask(pTask);
	                if (nResult == SOAP_OK) {
	                    return nResult;
	                }
	            }
                nResumeInterval = pResumeInfo->GetNextResumeIntervalType();
            }
	        else {
	            // Recovery is not possible so we'll bail...
	            PrintServiceError(stderr, szErrorMsg);
	            ClientLog(UI_COMP, LOG_ERROR, false,_T("Create file failed: (%d) %s"),
	                nResult, szErrorMsg.c_str());
                return nResult;
            }
        }
	}


    if ( (nResult != SOAP_OK) && ( nResumeInterval == ResumeInfoIntervalTypes::resumeIntervalDone) ) {
        PrintStatusMsg(_T("Failed to resume."), true);

        /* We can print out the final error here if needed
        PrintServiceError(stderr, szErrorMsg);
        */

        ClientLog(UI_COMP, LOG_ERROR, false,
            _T("Create file failed - resumed all possible intervals: (%d) %s"), nResult, szErrorMsg.c_str());
    }

    commandThread.Stop();
    return nResult;

} // End ResumeCreateFile

///////////////////////////////////////////////////////////////////////
// Purpose: Resume, if possible, current upload.  If the error
//          is such that resume is not possible, return control
//          back to the main upload block handler.
// Requires:
//      pTask: reference to a upload task object
//      bUserCancelled: true if the cancelled the resume, false
//                       otherwise.
// Returns: SOAP_OK if successful, service error otherwise.
int ConsoleControl::ResumeCurrentUpload(DIOMEDE_CONSOLE::UploadTask* pTask, bool& bUserCancelled)
{
    int nOriginalResult = pTask->GetResult();

    UploadImpl* pUploadImpl = pTask->GetUploadImpl();
    pUploadImpl->SetTotalUploadBytes(m_l64TotalUploadedBytes);

    ResumeUploadInfoData* pResumeInfo = &m_pUploadInfo->m_resumeUploadInfoData;
    pResumeInfo->SetFileID(pUploadImpl->GetFileID());

    // On the first run, the interval type is undefined.  Set it to the
    // first interval (or the next if on re-entry).
    ResumeInfoIntervalType nResumeInterval = pResumeInfo->GetNextResumeIntervalType();

    int nResult = 0;
    BOOL bReturn = FALSE;
    std::string szErrorMsg = _T("");

    // Keep the timer running, alert the user that the original task ended
    // with a connection error.  The following line of code screws up VS2005
    // intellisense - seems that if barfs on the enums...
    m_pUploadInfo->m_msgTimer.PauseTime(_T(""), EndTimerTypes::useNetworkConnectionError, false);

    DIOMEDE_CONSOLE::CommandThread commandThread;
    if (false == CheckThread(&commandThread, _T("Upload"))) {
        return DIOMEDE::DIOMEDE_CREATE_THREAD_ERROR;
    }

    bool bContinue = false;

    while (nResumeInterval < resumeIntervalDone) {
        szErrorMsg = _T("");
        bContinue = ResumeCountdown(nResumeInterval);

        if (bContinue == false) {
            PrintStatusMsg(_T("Resume cancelled."), true);
            bUserCancelled = true;
            return nOriginalResult;
        }

        // Has the file changed?
        if ( pResumeInfo->HasResumeFileChanged() ) {
            PrintStatusMsg(_T("File has changed.  Resume failed"), true);
            return nOriginalResult;
       }

        pTask->ResetTask();

        bReturn = commandThread.Event(pTask);
        if (bReturn == FALSE) {
            return nOriginalResult;
        }

	    while ( pTask->Status() != TaskStatusCompleted ) {
            if (g_bCanContinueTimer) {
                m_pUploadInfo->m_msgTimer.ContinueTime();
            }
            PauseProcess();
	    }

        //-----------------------------------------------------------------
        // Check results
        //-----------------------------------------------------------------
        nResult = pTask->GetResult();

        if (nResult == SOAP_OK) {
            // We're done - break out to allow the upload process to
            // complete our upload tasks (e.g. add metadata, etc.)
            break;
        }
        else {
            std::string szFriendlyMsg = _T("");
            szErrorMsg = pTask->GetServiceErrorMsg();

            // Make sure we track the last successful upload
            pUploadImpl->SetTotalUploadBytes(m_l64TotalUploadedBytes);

	        // Should we try to resume the upload?
	        if ( CheckServiceErrorToResume(szErrorMsg) ) {
                m_pUploadInfo->m_msgTimer.PauseTime(_T(""), EndTimerTypes::useNetworkConnectionError, false);
                nResumeInterval = pResumeInfo->GetNextResumeIntervalType();
	        }
	        else if ( CheckServiceErrorToRetry(szErrorMsg, szFriendlyMsg)) {
	            // Session expired so we'll need to relogin and then continue
	            // where we left off.
                g_bSessionError = true;
                g_nSessionRetries --;

                if (g_nSessionRetries >= 0) {
	                nResult = RepeatLastUploadTask(pTask);
	                if (nResult == SOAP_OK) {
	                    return nResult;
	                }
	            }
                nResumeInterval = pResumeInfo->GetNextResumeIntervalType();
            }
	        else {
	            // Recovery is not possible so we'll bail...
	            PrintServiceError(stderr, szErrorMsg, true, false);
	            ClientLog(UI_COMP, LOG_ERROR, false,_T("Upload failed: (%d) %s"), nResult, szErrorMsg.c_str());
                return nResult;
            }
        }
	}


    if ( (nResult != SOAP_OK) && ( nResumeInterval == ResumeInfoIntervalTypes::resumeIntervalDone) ) {
        PrintStatusMsg(_T("Failed to resume."), true);

        /* We can print out the final error here if needed
        PrintServiceError(stderr, szErrorMsg);
        */

        ClientLog(UI_COMP, LOG_ERROR, false,
            _T("Upload failed - resumed all possible intervals: (%d) %s"), nResult, szErrorMsg.c_str());
    }

    commandThread.Stop();
    return nResult;

} // End ResumeCurrentUpload

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to ResumeCountdown to pad the output
//          as needed.
// Requires:
//      szTimerText: string to output
// Returns: nothing
void PrintText(const std::string& szTimerText)
{
    std::string szPad = _T("");
    static int nResumeTextLength = 0;

    std::string szTempTimerText = szTimerText;
    int nNewLength = (int)szTimerText.length();

    if (nNewLength < nResumeTextLength) {
        szPad = GetPadStr(nResumeTextLength - nNewLength );
    }

    std::string szOutText = _format(_T("%s%s"), szTempTimerText.c_str(), szPad.c_str());
    fprintf(stdout, _T("%s%s"), szTempTimerText.c_str(), szPad.c_str());
    fflush(stdout);

    //ClientLog(UI_COMP, LOG_STATUS, false, _T("%s%s"), szTempTimerText.c_str(), szPad.c_str());

    nResumeTextLength = nNewLength;

} // End PrintText

///////////////////////////////////////////////////////////////////////
// "Press Any Key" for Linux
#ifndef WIN32
struct termios origTermiosSettings;

///////////////////////////////////////////////////////////////////////
// Purpose: Helper to reset the terminal back to its original settings.
// Requires: nothing
// Returns: nothing
void ResetTerminalMode()
{
    tcsetattr(0, TCSANOW, &origTermiosSettings);

} // End ResetTerminalMode

///////////////////////////////////////////////////////////////////////
// Purpose: Helper to turn off canonical mode.
// Requires: nothing
// Returns: nothing
void SetConioicalTerminalMode()
{
    struct termios newTermiosSettings;

    tcgetattr( STDIN_FILENO, &origTermiosSettings);
    memcpy(&newTermiosSettings, &origTermiosSettings, sizeof(newTermiosSettings));

    newTermiosSettings.c_iflag&=~ICRNL;
    newTermiosSettings.c_lflag&=~ICANON;
    newTermiosSettings.c_lflag&=~ECHO;
    newTermiosSettings.c_cc[VMIN ]=1;
    newTermiosSettings.c_cc[VTIME]=0;
    newTermiosSettings.c_cc[VINTR]=0xFF;
    newTermiosSettings.c_cc[VSUSP]=0xFF;
    newTermiosSettings.c_cc[VQUIT]=0xFF;

    tcsetattr( STDIN_FILENO, TCSANOW, &newTermiosSettings);

} // End SetConioicalTerminalMode

///////////////////////////////////////////////////////////////////////
// Purpose: Helper to wait for input from stdin
// Requires: nothing
// Returns: result from stdin trigger
int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);

  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);

} // end kbhit

///////////////////////////////////////////////////////////////////////
// Purpose: Helper to read a single character from stdin
// Requires: nothing
// Returns: character read if successful, error otherwise
int getch()
{
    int nResult;
    unsigned char szChar;

    if ( (nResult = read(STDIN_FILENO, &szChar, sizeof(szChar)) ) < 0) {
        return nResult;
    }
    else {
        return szChar;
    }
} // End getch

#endif

///////////////////////////////////////////////////////////////////////
// Purpose: Handle the count down timer and UI for resuming tasks.
// Requires:
//      nIntervalType: interval type (e.g. 5 sec, 10 sec).
// Returns: true to continue, false to quit
bool ConsoleControl::ResumeCountdown(ResumeInfoIntervalType nIntervalType)
{
    int nHours = 0;
    int nMinutes = 0;
    int nSeconds = 0;
    int nFractionalSeconds = 0;

    int nTotalSeconds = 0;
    int nTotalMinutes = 0;
    int nTotalHours = 0;

    std::vector<int> listResumeIntervals;
    GetConfigResumeIntervals(listResumeIntervals);

    bool bContinueResume = true;

    switch (nIntervalType) {
        case resumeInterval1:
            nTotalSeconds = listResumeIntervals[0];
            break;
        case resumeInterval2:
            nTotalSeconds = listResumeIntervals[1];
            break;
        case resumeInterval3:
            nTotalSeconds = listResumeIntervals[2];
            break;
        case resumeInterval4:
            nTotalSeconds = listResumeIntervals[3];
            break;
        case resumeInterval5:
            nTotalSeconds = listResumeIntervals[4];
            break;
        default:
            bContinueResume = false;
            break;
    }

    if (bContinueResume == false) {
        return false;
    }

    nSeconds = nTotalSeconds % 60;
    nTotalMinutes = nTotalSeconds / 60;
    nMinutes = nTotalMinutes % 60;;
    nTotalHours = nTotalMinutes / 60;
    nHours = nTotalHours % 24;

    fflush(stdout);

    #ifdef WIN32
        // For windows, we can use the windows console API's to
        // any key capture via a task.  Linux, on the other
        // hand, does not work well using this same type of mechanism.
        // We'll hand the terminos functions here directly.
        DIOMEDE_CONSOLE::PressAnyKeyTask taskPressAnyKey;

        DIOMEDE_CONSOLE::CommandThread commandThread;
        if (false == CheckThread(&commandThread, _T("Resume"))) {
            return false;
        }

        BOOL bReturn = commandThread.Event(&taskPressAnyKey);

        if (bReturn == FALSE) {
            return false;
        }
    #endif

    PrintNewLine();

    ptime tmFirstStart = not_a_date_time;
    ptime tmLastStart = not_a_date_time;

    tmFirstStart = boost::posix_time::microsec_clock::local_time();
    tmLastStart = tmFirstStart;

    time_duration countDownTime(nHours, nMinutes, nSeconds, nFractionalSeconds);

    LONG64 l64Seconds = 0;
    std::string szDuration = _T("");
    std::string szDurationType = _T("");
    std::string szResumeOutput = _T("");

    bContinueResume = false;

    #ifdef WIN32
	    while ( taskPressAnyKey.Status() != TaskStatusCompleted ) {
    #else
        SetConioicalTerminalMode();

        while (!kbhit()) {
    #endif

        ptime tmNow = boost::posix_time::microsec_clock::local_time();
        time_duration elapsedTime = tmNow - tmLastStart;

        if (elapsedTime.total_milliseconds() >= 50) {

	        szDuration = _T("");
	        szDurationType = _T("");

	        StringUtil::FormatDuration(countDownTime, szDuration, szDurationType);
	        szResumeOutput = _format(_T("\rRetrying in %s %s..."), szDuration.c_str(),
	                                  szDurationType.c_str());
            PrintText(szResumeOutput);

            tmLastStart = tmNow;
            countDownTime = countDownTime - milliseconds(elapsedTime.total_milliseconds());
        }

        l64Seconds =  countDownTime.total_milliseconds();
        if ( l64Seconds <= 0) {
            #ifdef WIN32
                taskPressAnyKey.StopTask();
            #endif
            bContinueResume = true;
            break;
        }
        PauseProcess();
	}
    #ifdef WIN32
        commandThread.Stop();
    #else
        if (bContinueResume == false) {
            // Consume a character ...
            (void)getch();
        }
        ResetTerminalMode();
    #endif

    fflush(stdout);
    return bContinueResume;

} // End ResumeCountdown

///////////////////////////////////////////////////////////////////////
// Purpose: Process get upload resume info command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetUploadTokenCommand(CmdLine* pCmdLine,
                                                  bool& bCommandFinished)
{
    // User must be logged in to get upload tokens.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get upload token"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get upload token: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileArg = NULL;
    DiomedeValueArg<std::string>* pBytesArg = NULL;

    try {
        pFileArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILENAME);
        pBytesArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_TOKEN_BYTES);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETUPLOADTOKEN << endl;
    }

    if ( pFileArg == NULL) {
        bCommandFinished = true;
        return;
    }

	if (pFileArg->getValue().length() == 0) {

        if (pFileArg->getRepromptCount() == 0) {
    	    m_szCommandPrompt = _T("File: ");
            pFileArg->incrementRepromptCount();
        }
        else if (pFileArg->getRepromptCount() < 2) {
            pFileArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFilePath = pFileArg->getValue();

    // Verify that the file exists.
    if (Util::DoesFileExist(szFilePath) == false) {
        std::string szStatusMsg = _format(_T("...Skipping %s:  File does not exist or is not accessible."),
                 szFilePath.c_str());
        PrintStatusMsg(szStatusMsg);
        ClientLog(UI_COMP, LOG_ERROR, false, _T("File %s is not accessible."), szFilePath.c_str());
        return;
    }

    // Get the file name without the path (TBD: if this is correct for this API).
    std::string szFileName = _T("");
    Util::GetFileName(szFilePath, szFileName);

    //-----------------------------------------------------------------
    // Bytes: defaults to file size if omitted.
    //-----------------------------------------------------------------
    LONG64 l64Bytes = 0;
    if (pBytesArg && pBytesArg->isSet()) {
        l64Bytes = atoi64(pBytesArg->getValue().c_str());
    }
    else {
        l64Bytes = (LONG64)Util::GetFileLength(szFilePath.c_str());
    }

    UploadTokenImpl uploadToken;
    uploadToken.SetFileName(szFileName);
    uploadToken.SetReserveBytes(l64Bytes);

    //-----------------------------------------------------------------
    // Process getuploadtoken
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::GetUploadTokenTask taskGetUploadToken(m_szSessionToken, &uploadToken);
	int nResult = HandleTask(&taskGetUploadToken, _T("Getting upload token"));

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Token = %s"),
	        taskGetUploadToken.GetUploadToken().c_str());
        PrintStatusMsg(szStatusMsg);

	    ClientLog(UI_COMP, LOG_STATUS, false,
	        _T("Get upload token successful successful for file %s.  Token returned %s."),
	        szFilePath.c_str(), taskGetUploadToken.GetUploadToken().c_str());
	}
	else {
	    std::string szErrorMsg = taskGetUploadToken.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get upload token failed for file %s."),
	        szFilePath.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessUploadTokenRequestCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the search files command using the CPPSDK.Lib
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSearchFilesCommand(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User must be logged in to search for files.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot search files"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Search file: user not logged into service."));
	    return;
    }

    bCommandFinished = true;

    //-----------------------------------------------------------------
    //                  NOTE
    // The remainder of the arugments are processed in the helper
    // function SetupSearchFilter
    //-----------------------------------------------------------------
    DiomedeValueArg<std::string>* pPageSizeArg = NULL;
    DiomedeValueArg<std::string>* pOffsetArg = NULL;
    DiomedeValueArg<std::string>* pOutputArg = NULL;
    DiomedeSwitchArg* pPhyscialFilesArg = NULL;

    try {
        pPhyscialFilesArg = (DiomedeSwitchArg*)pCmdLine->getArg(ARG_SEARCH_PHYSICALFILES_SWITCH);
        pPageSizeArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_PAGE_SIZE);
        pOffsetArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_OFFSET);
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SEARCHFILES << endl;
    }

    bool bVerboseOutput = VerboseOutput(pCmdLine);

    bool bArgIsSet = bVerboseOutput;
    bool bIsDeleted = bArgIsSet;

    // Currently, inclusion or exclusion of physical files in the output is
    // only a switch allowed for "search files"...otherwise, we'll include this handling
    // in the SetupSearchFilter below...this will also turn on verbose output.
    bool bIncludePhysicalFiles = false;
    if (pPhyscialFilesArg && pPhyscialFilesArg->isSet()) {
        bIncludePhysicalFiles = true;
        bVerboseOutput = true;
    }

    SearchFileFilterImpl* pSearchFilter = new SearchFileFilterImpl;
    SetupSearchFilter(pCmdLine, pSearchFilter, bIsDeleted, bArgIsSet);

    // If the user has selected /v, limit the page size to MAX_VERBOSE_PAGE_SIZE
    // to keep results back to the user in a timely manner.
    LONG64 l64Value = MAX_VERBOSE_PAGE_SIZE;

    if (pPageSizeArg && pPageSizeArg->isSet()) {
        l64Value = atoi64(pPageSizeArg->getValue().c_str());
        pSearchFilter->SetPageSize(l64Value);
    }
    else {
        if (GetConfigPageSize(l64Value) ) {
            pSearchFilter->SetPageSize(l64Value);
        }
        else {
            pSearchFilter->SetPageSize(MAX_VERBOSE_PAGE_SIZE);
        }
    }

    // Reset the page size filter if any other arguments
    // have been set along with the verbose argument.

    bool bMaxPageSizeSet = false;
    if ( bVerboseOutput && ( l64Value > MAX_VERBOSE_PAGE_SIZE ) && !bArgIsSet ) {
        pSearchFilter->SetPageSize(MAX_VERBOSE_PAGE_SIZE);
        bMaxPageSizeSet = true;
    }

    if (pOffsetArg && pOffsetArg->isSet()) {
        l64Value = atoi64(pOffsetArg->getValue().c_str());
        pSearchFilter->SetOffset(l64Value);
    }
    else {
        if (GetConfigOffset(l64Value) ) {
            pSearchFilter->SetOffset(l64Value);
        }
    }

    if (bMaxPageSizeSet) {
        PrintNewLine();
        _tprintf(_T("Verbose limited to page size of %d \n\r"), MAX_VERBOSE_PAGE_SIZE);
    }

    //-----------------------------------------------------------------
    // Process searchfiles
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::SearchFilesTask taskFiles(m_szSessionToken, pSearchFilter, bIncludePhysicalFiles);
	int nResult = HandleTask(&taskFiles, _T("Searching"), _T(""), false, !bMaxPageSizeSet);

    // Since we've turned off the task new lines, add one now...
    if (bMaxPageSizeSet) {
        PrintNewLine();
    }

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pSearchFilter != NULL) {
        delete pSearchFilter;
        pSearchFilter = NULL;
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskFiles.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Search files failed."), szErrorMsg.c_str());

	    return;
	}

    // Else the search was successful.
    ClientLog(UI_COMP, LOG_STATUS, false, _T("Search files successful."));

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Another mechanism for redirecting output.
    //-----------------------------------------------------------------
    #if 0
    char szFilename[MAX_PATH];
    strcpy(szFilename, pOutputArg->getValue().c_str());

    int fd;
    fpos_t pos;

    fflush(stdout);                 // flush output stream
    fgetpos(stdout,&pos);           // get position of output stream
    fd=dup(fileno(stdout));         // duplicate output stream fd=stdout

    // re-direct output stream (stdout) to the file "filename"
    freopen(szFilename,"w",stdout);
    #endif

    FilePropertiesListImpl* pListFileProperties = taskFiles.GetSearchFilesResults();
    LogicalPhysicalFilesInfoListImpl* pListLogicalPhysicalFiles =
                                         taskFiles.GetSearchFilesResultsPhysicalFiles();

    if (bVerboseOutput) {
        DisplaySearchFilesResultsVerbose(pListFileProperties, pListLogicalPhysicalFiles);
    }
    else {
        DisplaySearchFilesResults(pListFileProperties, true, bIsDeleted);
    }

    //-----------------------------------------------------------------
    // Another mechanism for redirecting output.
    //-----------------------------------------------------------------
    #if 0
    fflush(stdout);
    dup2(fd,fileno(stdout));
    close(fd);

    clearerr(stdout);

    // restore position of output stream (stdout) to normal
    fsetpos(stdout,&pos);
    #endif
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessSearchFilesCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to SearchFiles and SearchFilesTotal to setup
//          the search filter object.
// Requires:
//      pCmdLine: current command line
//      pSearchFilter: will contain the search filter criteria set from the
//                     arguments.
//      bIsDeleted: will hold the state of the isDeleted search filter.
//      bArgIsSet: indicates whether any arguments have been set.
// Returns: true if successful, false otherwise
bool ConsoleControl::SetupSearchFilter(CmdLine* pCmdLine, SearchFileFilterImpl* pSearchFilter,
                                       bool& bIsDeleted, bool& bArgIsSet)
{
    DiomedeValueArg<std::string>* pFileNameArg = NULL;
    DiomedeValueArg<std::string>* pFileIDArg = NULL;
    DiomedeValueArg<std::string>* pHashMD5Arg = NULL;
    DiomedeValueArg<std::string>* pHashSHA1Arg = NULL;
    DiomedeValueArg<std::string>* pMinSizeArg = NULL;
    DiomedeValueArg<std::string>* pMaxSizeArg = NULL;
    DiomedeValueArg<std::string>* pStartDateArg = NULL;
    DiomedeValueArg<std::string>* pEndDateArg = NULL;
    DiomedeValueArg<std::string>* pIsDeletedArg = NULL;
    DiomedeValueArg<std::string>* pIsCompleteArg = NULL;
    DiomedeValueArg<std::string>* pMetaNameArg = NULL;
    DiomedeValueArg<std::string>* pMetaValueArg = NULL;

    try {
        pFileNameArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_FILENAME);
        pFileIDArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pHashMD5Arg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_HASHMD5);
        pHashSHA1Arg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_HASHSHA1);
        pMinSizeArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_SEARCH_MINSIZE);
        pMaxSizeArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_SEARCH_MAXSIZE);
        pStartDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_STARTDATE);
        pEndDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_ENDDATE);
        pIsDeletedArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_SEARCH_ISDELETED);
        pIsCompleteArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_SEARCH_ISCOMPLETE);
        pMetaNameArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_SEARCH_METANAME);
        pMetaValueArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_SEARCH_METAVALUE);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    // Logic with "isdeleted" is that if any arugment is set, we'll show the results.
    // Otherwise, depending on the IsDeleted setting or no arguments set, the
    // resulting output will skip the deleted files.  This is to solve the issue
    // where the user types >dir /fileid=1 and 1 happens to be deleted.

    if (pFileNameArg && pFileNameArg->isSet()) {
        pSearchFilter->SetFileName(pFileNameArg->getValue());
    }

    LONG64 l64Value = 0L;
    if (pFileIDArg && pFileIDArg->isSet()) {
        l64Value = atoi64(pFileIDArg->getValue().c_str());
        pSearchFilter->SetFileID(l64Value);
        bArgIsSet = true;
    }

    if (pHashMD5Arg && pHashMD5Arg->isSet()) {
        pSearchFilter->SetHashMD5(pHashMD5Arg->getValue());
        bArgIsSet = true;
    }
    if (pHashSHA1Arg && pHashSHA1Arg->isSet()) {
        pSearchFilter->SetHashSHA1(pHashSHA1Arg->getValue());
        bArgIsSet = true;
    }

    long lValue = 0L;
    if (pMaxSizeArg && pMaxSizeArg->isSet()) {
        lValue = atol(pMaxSizeArg->getValue().c_str());
        pSearchFilter->SetMaxSize((LONG64)lValue);
        bArgIsSet = true;
    }
    if (pMinSizeArg && pMinSizeArg->isSet()) {
        lValue = atol(pMinSizeArg->getValue().c_str());
        pSearchFilter->SetMinSize((LONG64)lValue);
        bArgIsSet = true;
    }

    time_t epochSeconds;

    if (pStartDateArg && pStartDateArg->isSet()) {
        if ( ValidateDate(pStartDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateStart(epochSeconds);
        }
        else {
	        _tprintf(_T("Start date %s is not a valid date and will be ignored.\n\r"),
	            pStartDateArg->getValue().c_str());
        }
        bArgIsSet = true;
    }
    if (pEndDateArg && pEndDateArg->isSet()) {
        if ( ValidateDate(pEndDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateEnd(epochSeconds);
        }
        else {
	        _tprintf(_T("End date %s is not a valid date and will be ignored.\n\r"),
	            pEndDateArg->getValue().c_str());
        }
        bArgIsSet = true;
    }

    if (pIsCompleteArg && pIsCompleteArg->isSet()) {
    	std::string szSetting = pIsCompleteArg->getValue();

        if (0==stricmp(szSetting.c_str(), _T("yes"))) {
            pSearchFilter->SetIsCompleted(DIOMEDE::boolTrue);
        }
        else if (0==stricmp(szSetting.c_str(), _T("no"))) {
            pSearchFilter->SetIsCompleted(DIOMEDE::boolFalse);
        }
        else {
            pSearchFilter->SetIsCompleted(DIOMEDE::boolNull);
        }

        bArgIsSet = true;
    }
    else {
        // Argument not specified, so show only completed files.  If any other
        // argument is set, we'll set this to "all" (we're checking the deleted argument
        // individually since we've not checked it's settings yet).
        if ( bArgIsSet || (pIsDeletedArg && pIsDeletedArg->isSet()) ) {
             pSearchFilter->SetIsCompleted(DIOMEDE::boolNull);
       }
       else {
            // Currently this is the default - I'm going to leave this
            // logic in place until we can confirm again - this
            // sets the default to "all" - the above code "could" be
            // used to set the default to "all" only if another argument is
            // set...
            pSearchFilter->SetIsCompleted(DIOMEDE::boolNull);
       }

    }

    bool bMetaDataValid = false;

    if (pMetaNameArg && pMetaNameArg->isSet()) {
        if (pMetaValueArg && pMetaValueArg->isSet()) {
            bMetaDataValid = true;
        }
        else {
            _tprintf(_T("Metadata name %s is missing a metadata value argment and will be ignored.\n\r"),
                pMetaNameArg->getValue().c_str());
        }
    }
    else if (pMetaValueArg && pMetaValueArg->isSet()) {
        if (pMetaNameArg && pMetaNameArg->isSet()) {
            bMetaDataValid = true;
        }
        else {
            _tprintf(_T("Metadata value %s is missing a metadata name argument and will be ignored.\n\r"),
                pMetaValueArg->getValue().c_str());
        }
    }

    if (bMetaDataValid) {
        // Metadata must be in name and value pairs - solution here
        // is not the best, but suffices for now.
        pSearchFilter->SetMetaDataName(pMetaNameArg->getValue());
        pSearchFilter->SetMetaDataValue(pMetaValueArg->getValue());
        bArgIsSet = true;
    }

    //-----------------------------------------------------------------
    // This check has to follow ALL the other checks for
    // arguments to reset the bIsDeleted flag if the argument
    // is set - if no other arguments have been set,
    // bIsDeleted will default to false.
    //-----------------------------------------------------------------
    bIsDeleted = false;

    if (pIsDeletedArg && pIsDeletedArg->isSet()) {

    	std::string szSetting = pIsDeletedArg->getValue();

        if (0==stricmp(szSetting.c_str(), _T("yes"))) {
            pSearchFilter->SetIsDeleted(DIOMEDE::boolTrue);
            bIsDeleted = true;
        }
        else if  (0==stricmp(szSetting.c_str(), _T("no"))) {
            pSearchFilter->SetIsDeleted(DIOMEDE::boolFalse);
        }
        else {
            pSearchFilter->SetIsDeleted(DIOMEDE::boolNull);
            bIsDeleted = true;
        }

        bArgIsSet = true;
    }
    else {
        // Argument not specified, show only non-deleted files if no
        // other argument has been set.
        if (!bArgIsSet) {
            pSearchFilter->SetIsDeleted(DIOMEDE::boolFalse);
        }
        else {
            // Really not necessary, but just to be clear - set to NULL
            // if any other agument is set...
            pSearchFilter->SetIsDeleted(DIOMEDE::boolNull);
            bIsDeleted = true;
        }
    }


    return true;

} // End SetupSearchFilter

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to display the search file response in
//          a verbose format.
// Requires:
//      pListFileProperties: reference to the list of response objects.
//      pListLogicalPhysicalFiles: map of file IDs to physical file info's
//      bMaxPageSize: if true, the page size has been limited to
//                    MAX_VERBOSE_PAGE_SIZE and a status message will be
//                    shown to the user.
// Returns: nothing
void ConsoleControl::DisplaySearchFilesResultsVerbose(FilePropertiesListImpl* pListFileProperties,
	                                                  LogicalPhysicalFilesInfoListImpl* pListLogicalPhysicalFiles,
	                                                  bool bMaxPageSize /*false*/)
{
    std::vector<void * >listFileProperties = pListFileProperties->GetFilePropertiesList();

    std::string szDeletedYes = _T("Yes");
    std::string szDeletedNo = _T("No");

    std::string szComplete = _T("Yes");
    std::string szIncomplete = _T("No");

    std::string szResults = _format(_T("%d file(s) found."), listFileProperties.size());
    _tprintf(_T("   %s\n\r"), szResults.c_str());
    PrintNewLine();

    std::string szTempNumber = _T("");
    std::string szDefaultNumber = _T("0");

    std::string szFormattedDate = _T("");
    std::string szFileSize = _T("");
    std::string szFileSizeType = _T("");
    LONG64 l64FileID = 0;

    if ( (int)listFileProperties.size() == 0) {
        PrintNewLine();
        return;
    }

    //-----------------------------------------------------------------
    // Display results
    //-----------------------------------------------------------------
    for (int nIndex = 0; nIndex < (int)listFileProperties.size(); nIndex ++) {
        FilePropertiesImpl* pFileProperties = (FilePropertiesImpl*)listFileProperties[nIndex];
        if (pFileProperties == NULL) {
            continue;
        }

        l64FileID = pFileProperties->GetFileID();
        #ifdef WIN32
            _tprintf(_T("      Logical file ID: %I64d \n\r"), l64FileID);
        #else
            _tprintf(_T("      Logical file ID: %lld \n\r"), l64FileID);
        #endif

        if (pFileProperties->GetReplicationPolicyID() > 0) {
            _tprintf(_T("Replication policy ID: %d \n\r"), pFileProperties->GetReplicationPolicyID());
        }
        else {
            _tprintf(_T("Replication policy ID: none \n\r"));
        }

        _tprintf(_T("             Filename: %s \n\r"), pFileProperties->GetFileName().c_str());

	    // File size

	    szTempNumber = _T("");
	    szFileSize = _T("");
	    szFileSizeType = _T("");

        // Max the byte size in each category to 3 digits, e.g. 1000 KB = .97 MB
        LONG64 l64FileSize = pFileProperties->GetFileSize();
	    StringUtil::FormatByteSize(l64FileSize, szFileSize, szFileSizeType, 999);

        if (l64FileSize > 1024) {
            StringUtil::FormatNumber(l64FileSize, szTempNumber, szDefaultNumber);
            _tprintf(_T("            File size: %s %s (%s bytes) \n\r"),
                szFileSize.c_str(), szFileSizeType.c_str(), szTempNumber.c_str());
        }
        else {
            _tprintf(_T("            File size: %s %s \n\r"), szFileSize.c_str(), szFileSizeType.c_str());
        }

        StringUtil::FormatNumber(pFileProperties->GetPower(), szTempNumber, 5, szDefaultNumber);
        _tprintf(_T("    Power consumption: %s microwatts \n\r"), szTempNumber.c_str());

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pFileProperties->GetCreatedDate(), szFormattedDate);
        _tprintf(_T("         Created date: %s \n\r"), szFormattedDate.c_str());

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pFileProperties->GetLastModifiedDate(), szFormattedDate);
        _tprintf(_T("   Last modified date: %s \n\r"), szFormattedDate.c_str());

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pFileProperties->GetLastAccess(), szFormattedDate);
        _tprintf(_T("     Last access date: %s \n\r"), szFormattedDate.c_str());

        _tprintf(_T("                  MD5: %s \n\r"), pFileProperties->GetHashMD5().c_str());
        _tprintf(_T("                 SHA1: %s \n\r"), pFileProperties->GetHashSHA1().c_str());

        if ( pFileProperties->GetIsDeleted() ) {
            _tprintf(_T("           Is deleted: "));
            PrintTextInColor(szDeletedYes, COLOR_DELETED);
            PrintNewLine();
        }
        else {
            _tprintf(_T("           Is deleted: %s \n\r"), szDeletedNo.c_str());
        }

        if ( pFileProperties->GetIsCompleted() ) {
            _tprintf(_T("          Is complete: %s \n\r"), szComplete.c_str());
        }
        else {
            _tprintf(_T("          Is complete: %s \n\r"), szIncomplete.c_str());
        }

        PhysicalFileInfoListImpl* pListPhysicalFileInfo =
            static_cast<PhysicalFileInfoListImpl*>(pListLogicalPhysicalFiles->GetPhysicalFileInfoList(l64FileID));
        if ( pListPhysicalFileInfo != NULL) {
            DisplayPhysicalFileInfo(l64FileID, pListPhysicalFileInfo, false);
        }
        else {
            _tprintf(_T("\n\r"));
        }

        if (g_bUsingCtrlKey) {
            g_bUsingCtrlKey = false;
            PrintStatusMsg("Cancelled!");
            break;
        }
    }

} // End DisplaySearchFilesResultsVerbose

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to display the search file response in
//          a short format.
// Requires:
//      pListFileProperties: reference to the list of response objects.
//      bShowFileDate: show file date (appears in the first column)
//      bShowDeleted: show whether or not the file has been deleted.
// Returns: nothing
void ConsoleControl::DisplaySearchFilesResults(class FilePropertiesListImpl* pListFileProperties,
	                                           bool bShowFileDate /*true*/,
	                                           bool bShowDeleted /*false*/)
{
    std::vector<void * >listFileProperties = pListFileProperties->GetFilePropertiesList();

    std::string szFormattedDate = _T("");
	std::vector<std::string> dateTime;

    std::string szFileSize = _T("");
    std::string szFileSizeType = _T("");
    std::string szFilename = _T("");
    std::string szPad = _T("");

    LONG64 l64TotalBytes = 0;

     // Run through the results to find the length we need for file IDs
    int nMaxFileIDLen = 0;
    std::string szOutFileID = _T("");
    int nFileIDLen = 0;

    std::vector<std::string> listFileID;
    std::string szIsDeleted = _T("d");
    std::string szIsIncomplete = _T("i");

    for (int nIndex = 0; nIndex < (int)listFileProperties.size(); nIndex ++) {
        FilePropertiesImpl* pFileProperties = (FilePropertiesImpl*)listFileProperties[nIndex];
        if (pFileProperties == NULL) {
            listFileID.push_back(_T(""));
            continue;
        }

	    if ( (bShowDeleted == false) && ( pFileProperties->GetIsDeleted() )) {
	        // Skip the deleted files if we're not showing them.
            listFileID.push_back(_T(""));
	        continue;
	    }

        #ifdef WIN32
            szOutFileID = _format(_T("%I64d"), pFileProperties->GetFileID());
        #else
            szOutFileID = _format(_T("%lld"), pFileProperties->GetFileID());
        #endif

        // Save the formatted string to use in the final output
        listFileID.push_back(szOutFileID);
        nFileIDLen = (int)szOutFileID.length();
        nMaxFileIDLen = (nFileIDLen > nMaxFileIDLen) ? nFileIDLen : nMaxFileIDLen;
    }

    // File name space - Our total line length is 79 - at 80, a console window
    // set to 80 will wrap.
    // Filename space = 79 - 19 (date+time) - 11 (file size) - (max file ID length + 3) - 3
    // for left pad.
    // If we're showing the deleted state:
    // Filename space = (above calculation) - 8 - 12/27/08 showing data in red only so
    // "deleted" is not shown.

    int nDateTimeSpace = bShowFileDate ? 19 : 0;
    #if 0
        // See comment about 12/27/08
        int nDeletedSpace = bShowDeleted ? 8 : 0;
    #else
        int nDeletedSpace = 0;
    #endif
    int nFileNameSpace = 62 - nDateTimeSpace - nDeletedSpace - nMaxFileIDLen;

    std::string szDeleted = _T(" DELETED");

    //-----------------------------------------------------------------
    // cout << _T("12345678901234567890123456789012345678901234567890123456789012345678901234567890") << endl;
    // Display results
    //-----------------------------------------------------------------

    int nCountDisplayed = 0;

    for (int nIndex = 0; nIndex < (int)listFileProperties.size(); nIndex ++) {
        FilePropertiesImpl* pFileProperties = (FilePropertiesImpl*)listFileProperties[nIndex];
        if (pFileProperties == NULL) {
            continue;
        }

	    if ( (bShowDeleted == false) && ( pFileProperties->GetIsDeleted() )) {
	        // Skip the deleted files if we're not showing them.
	        continue;
	    }

        if (bShowFileDate) {
            szFormattedDate = _T("");
            StringUtil::FormatDateAndTime(pFileProperties->GetLastModifiedDate(), szFormattedDate);

            // We need to split the date + time into a date and a time string
            // to allow us to allign them.
	        int nCount = SplitString(szFormattedDate, dateTime, false);
	        if (nCount < 2) {
	            // Something is wrong with this entry - skip it....
	            continue;
	        }

	        // Date and time, 1 space between, 3 spaces afterwards -
	        // see below for file size.
	        _tprintf(_T("%s %s"), dateTime[0].c_str(), dateTime[1].c_str());
	    }

        //-------------------------------------------------------------
        // If the file is deleted, text is in red.  If incomplete,
        // test is yellow.  If deleted and incomplete, text is red.
        //-------------------------------------------------------------

        unsigned short nOrigColors = 0;
	    if (bShowDeleted) {
            if ( pFileProperties->GetIsDeleted() ) {
        	    PrintTextInColorOn(COLOR_DELETED, nOrigColors);
            }
	    }
        if ( (nOrigColors == 0) && pFileProperties->GetIsCompleted() == false) {
    	    PrintTextInColorOn(COLOR_INCOMPLETE, nOrigColors);
        }

        //-------------------------------------------------------------
	    // File size
        //-------------------------------------------------------------
	    szFileSize = _T("");
	    szFileSizeType = _T("");

        // Max the byte size in each category to 3 digits, e.g. 1000 KB = .97 MB
        LONG64 l64FileSize = pFileProperties->GetFileSize();
	    StringUtil::FormatByteSize(l64FileSize, szFileSize, szFileSizeType, 999);

        // 12/27/3008: changed such that there's always 1 space between
        // the file size and the file size type...
        #if 0
    	    szPad = _T("");
	        if (szFileSizeType.length() < 2) {
	            szPad = GetPadStr(2);
	        }
	        else if (szFileSizeType.length() < 3) {
	            szPad = GetPadStr(1);
	        }
        #endif

        int nFileIDPad = 3;

	    szFileSize = szFileSize + _T(" ") + szFileSizeType;
        if (szFileSizeType.length() < 2) {
    	    _tprintf(_T("%11s"), szFileSize.c_str());
        }
        else if (szFileSizeType.length() < 3) {
	        _tprintf(_T("%12s"), szFileSize.c_str());
	        nFileIDPad --;
        }

        //-------------------------------------------------------------
	    // File ID - 3 spaces + max width for all file IDs, right aligned
        //-------------------------------------------------------------
        szOutFileID = listFileID[nIndex];
        nFileIDLen = (int)szOutFileID.length();

        szOutFileID = GetPadStr(nMaxFileIDLen - nFileIDLen + nFileIDPad) + szOutFileID;

        // We want to keep the file IDs lined up, so add the annotations for
        // deleted and incompleted at this point.
        int nFileNamePad = 3;
        if ( pFileProperties->GetIsDeleted() ) {
            szOutFileID += szIsDeleted;
            nFileNamePad --;
        }

        if ( pFileProperties->GetIsCompleted() == false ) {
            szOutFileID += szIsIncomplete;
            nFileNamePad --;
        }

	    _tprintf(_T("%s"), szOutFileID.c_str());
	    nCountDisplayed++;

        //-------------------------------------------------------------
	    // File name, truncated with ... if needed.  Our total line length is
	    // 79 - at 80, a console window set to 80 will wrap.
        //-------------------------------------------------------------
	    szFilename = _T("");
	    TrimFileName(nFileNameSpace, pFileProperties->GetFileName(), szFilename);
	    szFilename = GetPadStr(nFileNamePad) + szFilename;
	    _tprintf(_T("%-s"), szFilename.c_str());

        #if 0
        // 12/27/08 Print file name, file ID, size, and size type in red.
	    if (bShowDeleted) {
            if ( pFileProperties->GetIsDeleted() ) {
                PrintTextInColor(szDeleted, COLOR_DELETED);
            }
	    }
	    #endif

        dateTime.clear();
        PrintNewLine();

        l64TotalBytes += pFileProperties->GetFileSize();

		#ifdef WIN32
			if (nOrigColors != 0) {
				PrintTextInColorOff(nOrigColors);
			}
		#else
			PrintTextInColorOff(nOrigColors);
		#endif

        if (g_bUsingCtrlKey) {
            g_bUsingCtrlKey = false;
            PrintStatusMsg("Cancelled!");
            break;
        }

    }

    std::string szTotalBytes = _T("");
    szFileSizeType = _T("");

    StringUtil::FormatByteSize(l64TotalBytes, szTotalBytes, szFileSizeType);
    std::string szFilesText = _T("file, ");

    #if 0
        if ( (int)listFileProperties.size() > 1 ) {
            szFilesText = _T("files, ");
        }
        std::string szResults = _format(_T("%d %s"), listFileProperties.size(),
            szFilesText.c_str());
    #else
        if ( nCountDisplayed > 1 ) {
            szFilesText = _T("files, ");
        }
        // Seems like we really need to show the number of files displayed...
        std::string szResults = _format(_T("%d %s"), nCountDisplayed, szFilesText.c_str());
    #endif

    szResults += szTotalBytes + _T(" ") + szFileSizeType;

    if (szFileSizeType.length() < 2) {
	    _tprintf(_T("%30s\n\r"), szResults.c_str());
    }
    else if (szFileSizeType.length() < 3) {
        _tprintf(_T("%31s\n\r"), szResults.c_str());
    }

    if (!m_bSysCommandInput) {
	    PrintNewLine();
	}

} // End DisplaySearchFilesResults

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function for file search
// Requires:
//      pStorageService: reference to storage service
//      pFileRequest: file request structure
//      pFileResponse: reference to search file response
//      bShowProgress: shows progress if true, no output otherwise
// Returns: true if successful, false otherwise.
bool ConsoleControl::SearchFiles(DiomedeStorageService* pStorageService,
	                 _sds__SearchFilesRequest* pFileRequest,
	                 _sds__SearchFilesResponse* pFileResponse,
	                 bool bShowProgress /*false*/)
{
    // This search is primarily used for retrieving file data to access the
    // file ID - showing the search progress is not generally desirable.
    MessageTimer msgTimer(50, bShowProgress);
    std::string szStatus = _T("");

    if (bShowProgress) {
        PrintNewLine();
        szStatus = _T("Searching");
    }

    msgTimer.Start(szStatus);

	DIOMEDE_CONSOLE::SearchFilesTask taskFiles(pStorageService, pFileRequest, pFileResponse);

    DIOMEDE_CONSOLE::CommandThread commandThread;
    if (false == CheckThread(&commandThread, _T("Search files"))) {
        return false;
    }

    BOOL bReturn = commandThread.Event(&taskFiles);

    if (bReturn == FALSE) {
        return false;
    }

	while ( taskFiles.Status() != TaskStatusCompleted ) {
	    msgTimer.ContinueTime();
	    PauseProcess();
	}

    commandThread.Stop();
    msgTimer.EndTime(_T(""));

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
    int nResult = taskFiles.GetResult();
	if (nResult == SOAP_OK) {
        // Search was successful.
        ClientLog(UI_COMP, LOG_STATUS, false, _T("Search files successful."));
        return true;
	}

    std::string szErrorMsg = taskFiles.GetServiceErrorMsg();

    std::string szFriendlyMsg = _T("");
    if ( CheckServiceErrorToRetry(szErrorMsg, szFriendlyMsg)) {

        // The session retries is reset to MAX_RETRIES as soon as the
        // user is successfully logged back into the service.
        g_bSessionError = true;
        g_nSessionRetries --;

        if (g_nSessionRetries >= 0) {
            nResult = RepeatLastTask(&taskFiles, &msgTimer, _T(""), true);
        }

        if (nResult == SOAP_OK) {
            ClientLog(UI_COMP, LOG_STATUS, false, _T("Search files successful."));
            return true;
        }
        // Else the error returned from the service is returned.
    }

    PrintSoapError(pStorageService->soap, stderr);
    ClientLog(UI_COMP, LOG_ERROR, false,_T("Search files failed."));

    return false;

} // End SearchFiles

///////////////////////////////////////////////////////////////////////
// Purpose: Process the search files total command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSearchFilesTotal(CmdLine* pCmdLine,
                                             bool& bCommandFinished)
{

    // User must be logged in to retrieve the search files total.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot search file total"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Search file total: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    //-----------------------------------------------------------------
    //                  NOTE
    // The remainder of the arugments are processed in the helper
    // function SetupSearchFilter
    //-----------------------------------------------------------------
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SEARCHFILESTOTAL << endl;
    }

    bool bVerboseOutput = VerboseOutput(pCmdLine);

    bool bArgIsSet = bVerboseOutput;
    bool bIsDeleted = bArgIsSet;

    SearchFileFilterImpl* pSearchFilter = new SearchFileFilterImpl;
    SetupSearchFilter(pCmdLine, pSearchFilter, bIsDeleted, bArgIsSet);

    //-----------------------------------------------------------------
    // Process search
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::SearchFilesTotalTask taskFilesTotal(m_szSessionToken, pSearchFilter);
	int nResult = HandleTask(&taskFilesTotal, _T("Searching for files total"));

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pSearchFilter != NULL) {
        delete pSearchFilter;
        pSearchFilter = NULL;
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskFilesTotal.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Search file total failed.") );
	    return;
	}

    ClientLog(UI_COMP, LOG_STATUS, false,_T("Search file total suuccessful.") );

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process results.
    //-----------------------------------------------------------------
    FilesTotalLogEntryImpl* pFileTotalInfo = taskFilesTotal.GetSearchFilesTotalResults();

    if (bVerboseOutput) {
        DisplaySearchFilesTotalEntryVerbose(pFileTotalInfo);
    }
    else {
        DisplaySearchFilesTotalEntry(pFileTotalInfo);
    }

    if (!m_bSysCommandInput) {
	    PrintNewLine();
	}

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessSearchFilesTotals

///////////////////////////////////////////////////////////////////////
// Purpose: Process the search files total log command
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSearchFilesTotalLog(CmdLine* pCmdLine,
                                                bool& bCommandFinished)
{
    // User must be logged in to search the files total log.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot search files total log"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Search file: user not logged into service."));
	    return;
    }

    bCommandFinished = true;

    DiomedeValueArg<std::string>* pStartDateArg = NULL;
    DiomedeValueArg<std::string>* pEndDateArg = NULL;
    DiomedeValueArg<std::string>* pPageSizeArg = NULL;
    DiomedeValueArg<std::string>* pOffsetArg = NULL;
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pStartDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_STARTDATE);
        pEndDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_ENDDATE);
        pPageSizeArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_PAGE_SIZE);
        pOffsetArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_OFFSET);
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SEARCHFILESTOTALLOG << endl;
    }

    bool bVerboseOutput = VerboseOutput(pCmdLine);

    SearchLogFilterImpl* pSearchFilter = new SearchLogFilterImpl;

    time_t epochSeconds;

    if (pStartDateArg && pStartDateArg->isSet()) {
        if ( ValidateDate(pStartDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateStart(epochSeconds);
        }
        else {
	        _tprintf(_T("Start date %s is not a valid date and will be ignored.\n\r"),
	            pStartDateArg->getValue().c_str());
        }
    }
    if (pEndDateArg && pEndDateArg->isSet()) {
        if ( ValidateDate(pEndDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateEnd(epochSeconds);
        }
        else {
	        _tprintf(_T("End date %s is not a valid date and will be ignored.\n\r"),
	            pEndDateArg->getValue().c_str());
        }
    }

    LONG64 l64Value = 0L;

    if (pPageSizeArg && pPageSizeArg->isSet()) {
        l64Value = atoi64(pPageSizeArg->getValue().c_str());
        pSearchFilter->SetPageSize(l64Value);
    }
    else {
        if (GetConfigPageSize(l64Value) ) {
            pSearchFilter->SetPageSize(l64Value);
        }
    }

    if (pOffsetArg && pOffsetArg->isSet()) {
        l64Value = atoi64(pOffsetArg->getValue().c_str());
        pSearchFilter->SetOffset(l64Value);
    }
    else {
        if (GetConfigOffset(l64Value) ) {
            pSearchFilter->SetOffset(l64Value);
        }
    }

    //-----------------------------------------------------------------
    // Process search
    //-----------------------------------------------------------------
	SearchFilesTotalLog taskFilesTotalLog(m_szSessionToken, pSearchFilter);
	int nResult = HandleTask(&taskFilesTotalLog, _T("Searching file total log"));

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pSearchFilter != NULL) {
        delete pSearchFilter;
        pSearchFilter = NULL;
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
        // If there are no results returned the error is caught later on...
        if (nResult != DIOMEDE_NO_MATCHES_FOUND) {
	        std::string szErrorMsg = taskFilesTotalLog.GetServiceErrorMsg();
	        PrintServiceError(stderr, szErrorMsg);

	        ClientLog(UI_COMP, LOG_ERROR, false,_T("Search file total log failed.") );
	        return;
	    }
	}

    ClientLog(UI_COMP, LOG_STATUS, false,_T("Search file total log suuccessful."));

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process results.
    //-----------------------------------------------------------------
    FilesTotalLogListImpl* pFilesTotalLog = taskFilesTotalLog.GetSearchFilesTotalLogResults();

    if (pFilesTotalLog == NULL) {
        PrintStatusMsg("Search files total log: no log entries returned.");
        ClientLog(UI_COMP, LOG_ERROR, false,_T("Search files total log: no log entries returned."));
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    std::vector<void * > listFilesTotalLog = pFilesTotalLog->GetFileTotalLogList();

    // Edge case to ensure we did get something back.
    if (listFilesTotalLog.size() == 0) {
        PrintStatusMsg("Search files total log: no log entries returned.");
        ClientLog(UI_COMP, LOG_ERROR, false,_T("Search files total log: no log entries returned."));
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    bool bShowHeader = true;
    for (int nFilesTotalInfoIndex = 0; nFilesTotalInfoIndex < (int)listFilesTotalLog.size();
        nFilesTotalInfoIndex ++) {

        FilesTotalLogEntryImpl* pFileTotalInfo = (FilesTotalLogEntryImpl*)listFilesTotalLog[nFilesTotalInfoIndex];

        if (bVerboseOutput) {
            DisplaySearchFilesTotalEntryVerbose(pFileTotalInfo);
            PrintNewLine();
        }
        else {
            DisplaySearchFilesTotalEntry(pFileTotalInfo, bShowHeader);
            bShowHeader = false;
        }
    }

    if (!bVerboseOutput && !m_bSysCommandInput) {
        PrintNewLine();
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessSearchFilesTotalLog

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to display the search files total response in
//          a short format.
// Requires:
//      pListFileProperties: reference to the list of response objects.
//      bShowHeader: displays the header - the log should set this flag to
//                   false on subsequent calls.
// Returns: true if successful, false otherwise
bool ConsoleControl::DisplaySearchFilesTotalEntry(FilesTotalLogEntryImpl* pFileTotalInfo,
                                                  bool bShowHeader /*true*/ )
{
    if (pFileTotalInfo == NULL) {
        ClientLog(UI_COMP, LOG_ERROR, false,
            _T("Search files total: NULL storage info entry returned."));
        return false;
    }

    std::string szStoredNumber = _T("");
    std::string szUploadNumber = _T("");
    std::string szDownloadNumber = _T("");
    std::string szDeletedNumber = _T("");

    std::string szDefaultNumber = _T("0");

    std::string szBytes = _T("");
    std::string szBytesSizeType = _T("");
    std::string szFormattedDate = _T("");

    if (pFileTotalInfo->GetCreatedDate()) {
        StringUtil::FormatDateAndTime(pFileTotalInfo->GetCreatedDate(), szFormattedDate);
    }

    StringUtil::FormatNumber(pFileTotalInfo->GetFileCount(), szStoredNumber, szDefaultNumber);
    StringUtil::FormatNumber(pFileTotalInfo->GetUploadCount(), szUploadNumber, szDefaultNumber);
    StringUtil::FormatNumber(pFileTotalInfo->GetDownloadCount(), szDownloadNumber, szDefaultNumber);

    StringUtil::FormatNumber(pFileTotalInfo->GetDeleteCount(), szDeletedNumber, szDefaultNumber);
    if (szDeletedNumber.length() == 0) {
        szDeletedNumber = _T("0");
    }

    // Using streams here to provide easier column setup.
    if (bShowHeader) {
        cout << setw(35) << right << _T("Stored") << setw(13) <<  _T("Uploaded")
             << setw(13) <<  _T("Downloaded") << setw(13) <<  _T("Deleted") << endl;
    }
    cout << left << szFormattedDate << setw(16) << right << szStoredNumber
         << setw(13) << szUploadNumber << setw(13) << szDownloadNumber
         << setw(13) << szDeletedNumber << endl;

    return true;

} // End DisplaySearchFilesTotalEntry

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to display the search files total response in
//          a verbose format.
// Requires:
//      pListFileProperties: reference to the list of response objects.
//      bLogDisplay: display minor items differently for SearchFilesTotalLog -
//                   currently, both the entry and log versions are identical.
// Returns: true if successful, false otherwise
bool ConsoleControl::DisplaySearchFilesTotalEntryVerbose(FilesTotalLogEntryImpl* pFileTotalInfo,
                                                         bool bLogDisplay /*false*/)
{
    if (pFileTotalInfo == NULL) {
        ClientLog(UI_COMP, LOG_ERROR, false,
            _T("Search files total: NULL storage info entry returned."));
        return false;
    }

    //-----------------------------------------------------------------
    // Make sure we have the list of storage type names.
    //-----------------------------------------------------------------

    if (m_pListStorageTypes == NULL) {
        m_pListStorageTypes = new StorageTypeListImpl;
    }

    // If the storage types list is empty, get the list now...
    std::vector<void * > listStorageTypes = m_pListStorageTypes->GetStorageTypeList();
    if ( (int)listStorageTypes.size() == 0) {
        GetStorageTypes(false);
    }
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------

    std::string szTempNumber = _T("");
    std::string szDefaultNumber = _T("0");

    std::string szBytes = _T("");
    std::string szBytesSizeType = _T("");
    std::string szFormattedDate = _T("");
    std::string szStorageTypeName = _T("");

    if (pFileTotalInfo->GetCreatedDate()) {
        StringUtil::FormatDateAndTime(pFileTotalInfo->GetCreatedDate(), szFormattedDate);
    }
    _tprintf(_T("           Created date: %s \n\r"), szFormattedDate.c_str() );

    StringUtil::FormatByteSize(pFileTotalInfo->GetFileBytes(), szBytes, szBytesSizeType);
    StringUtil::FormatNumber(pFileTotalInfo->GetFileCount(), szTempNumber, szDefaultNumber);

    if (bLogDisplay) {
        _tprintf(_T("     Total files stored: %s (%s %s) \n\r"), szTempNumber.c_str(),
            szBytes.c_str(), szBytesSizeType.c_str());
    }
    else {
        _tprintf(_T("           Stored files: %s (%s %s) \n\r"), szTempNumber.c_str(),
            szBytes.c_str(), szBytesSizeType.c_str());
    }

    StringUtil::FormatByteSize(pFileTotalInfo->GetUploadBytes(), szBytes, szBytesSizeType);
    StringUtil::FormatNumber(pFileTotalInfo->GetUploadCount(), szTempNumber, szDefaultNumber);

    _tprintf(_T("         Uploaded files: %s (%s %s) \n\r"), szTempNumber.c_str(),
        szBytes.c_str(), szBytesSizeType.c_str());

    StringUtil::FormatByteSize((LONG64)pFileTotalInfo->GetDownloadBytes(), szBytes, szBytesSizeType);
    StringUtil::FormatNumber(pFileTotalInfo->GetDownloadCount(), szTempNumber, szDefaultNumber);

    _tprintf(_T("       Downloaded files: %s (%s %s) \n\r"), szTempNumber.c_str(),
        szBytes.c_str(), szBytesSizeType.c_str());

    StringUtil::FormatByteSize(pFileTotalInfo->GetDeleteBytes(), szBytes, szBytesSizeType);
    StringUtil::FormatNumber(pFileTotalInfo->GetDeleteCount(), szTempNumber, szDefaultNumber);
    if (szTempNumber.length() == 0) {
        szTempNumber = _T("0");
    }
    _tprintf(_T("          Deleted files: %s (%s %s) \n\r"), szTempNumber.c_str(),
        szBytes.c_str(), szBytesSizeType.c_str());


    FilesTotalStorageListImpl* pFilesTotalStorageList = pFileTotalInfo->GetFilesTotalStorageList();
    std::vector<void * > listStorageInfo = pFilesTotalStorageList->GetFileTotalStorageList();

    // If no entries found, output a message and return.
    if ((int)listStorageInfo.size() == 0) {
        /* TBD....
        PrintStatusMsg(_T("Search files total storage successful.  No storage info found."));
        */
        return false;
    }

    for (int nIndex = 0; nIndex < (int)listStorageInfo.size(); nIndex ++) {
        FilesTotalStorageEntryImpl* pInfoEntry = (FilesTotalStorageEntryImpl*)listStorageInfo[nIndex];

        if (pInfoEntry == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,
                _T("Search files total storage: NULL storage info entry returned."));
            continue;
        }

        PrintNewLine();

        szStorageTypeName = _T("");
        GetStorageTypeName(pInfoEntry->GetStorageTypeID(), szStorageTypeName);

        _tprintf(_T("           Storage type: %s (%d) \n\r"), szStorageTypeName.c_str(),
            pInfoEntry->GetStorageTypeID());

        StringUtil::FormatByteSize(pInfoEntry->GetFileBytes(), szBytes, szBytesSizeType);
        StringUtil::FormatNumber(pInfoEntry->GetFileCount(), szTempNumber, szDefaultNumber);

        _tprintf(_T("  Stored physical files: %s (%s %s) \n\r"), szTempNumber.c_str(),
            szBytes.c_str(), szBytesSizeType.c_str());

        StringUtil::FormatNumber(pInfoEntry->GetPower(), szTempNumber, 5, szDefaultNumber);
        _tprintf(_T("      Power consumption: %s microwatts \n\r"), szTempNumber.c_str());
    }

    return true;

} // End DisplaySearchFilesTotalEntryVerbose

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to get the file ID from file input where
//          file input is either file ID, file name, or hash.
// Requires:
//      pStorageService: reference to storage service
//      szInFileID: input file information (fileID, file name, hash)
//      l64FileID: returned file ID
//      szErrorText: error text to accompany failure scenarios.
// Returns: true if successful, false otherwise
bool ConsoleControl::GetFileID(DiomedeStorageService* pStorageService, std::string szInFileID,
                               LONG64& l64FileID, std::string szErrorText /*""*/)
{
    // Get the file ID type - it should be either a filename, file ID, MD5 or SHA1 hash.
    int nFileIDType = ConsoleControl::unknownType;
    GetFileIDType(szInFileID, nFileIDType);

    l64FileID = 0;
    bool bUseFirstOneOK = false;

    if ( nFileIDType == ConsoleControl::fileIDType ) {
        l64FileID = atoi64(szInFileID.c_str());
        return true;
    }

    // If it's not a file ID, search for the file to get it's ID.
    _sds__SearchFilesResponse searchFilesResponse;
    _sds__SearchFilesRequest searchFilesRequest;

    searchFilesRequest.sessionToken = &m_szSessionToken;
    searchFilesRequest.offset = new LONG64(0);
    searchFilesRequest.pageSize = new LONG64(100);

    std::string szSearchType = _T("File ID ");
    if ( nFileIDType == ConsoleControl::fileType ) {
        szSearchType = _T("Filename ");
        searchFilesRequest.fileName = new std::string(szInFileID);
    }
    else if (nFileIDType == ConsoleControl::md5Type) {
        szSearchType = _T("MD5 ");
        searchFilesRequest.hashMD5 = new std::string(szInFileID);
        bUseFirstOneOK = true;
    }
    else if (nFileIDType == ConsoleControl::sha1Type) {
        szSearchType = _T("SHA1 ");
        searchFilesRequest.hashSHA1 = new std::string(szInFileID);
        bUseFirstOneOK = true;
    }

    // Currently, the only way to get file properties is to search for the file.
    // Hopefully, we only get 1 response - we get multiple responses, alert
    // the user and return.
    bool bSuccess = SearchFiles(pStorageService, &searchFilesRequest, &searchFilesResponse);

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (searchFilesRequest.offset != NULL) {
        delete searchFilesRequest.offset;
        searchFilesRequest.offset = NULL;
    }
    if (searchFilesRequest.pageSize != NULL) {
        delete searchFilesRequest.pageSize;
        searchFilesRequest.pageSize = NULL;
    }
    if ( searchFilesRequest.fileName != NULL) {
        delete searchFilesRequest.fileName;
        searchFilesRequest.fileName = NULL;
    }
    if ( searchFilesRequest.hashMD5 != NULL) {
        delete searchFilesRequest.hashMD5;
        searchFilesRequest.hashMD5 = NULL;
    }
    if ( searchFilesRequest.hashSHA1 != NULL) {
        delete searchFilesRequest.hashSHA1;
        searchFilesRequest.hashSHA1 = NULL;
    }

    //-----------------------------------------------------------------
    // Process results
    //-----------------------------------------------------------------
    if ( false == bSuccess ) {
        std::string szErrorMsg = szSearchType + szInFileID + _T(" not found.  ");
        if (szErrorText.length() > 0) {
            szErrorMsg += szErrorText + _T(" failed.");
        }
        _tprintf(_T(" %s\n\r"), szErrorMsg.c_str());
        return false;
    }

    dds__ArrayOfFileProperties* pMatchedFiles = searchFilesResponse.matchedFiles;
    std::vector<class dds__FileProperties * > listFileProperties = pMatchedFiles->FileProperties;

    // Edge case to ensure we did get something back.
    if (listFileProperties.size() == 0) {
        std::string szErrorMsg = szSearchType + szInFileID + _T(" not found.  ");
        if (szErrorText.length() > 0) {
            szErrorMsg += szErrorText + _T(" failed.");
        }
        _tprintf(_T(" %s\n\r"), szErrorMsg.c_str());
        return false;
    }

    dds__FileProperties* pFileProperties = NULL;
    std::string szDeleted = _T("DELETED");

    std::string szBytes = _T("");
    std::string szBytesSizeType = _T("");

    // Check for multiple hits - means search came back with mulitple hits.
    if (!bUseFirstOneOK && (listFileProperties.size() > 1) ) {

        #if 1
            PrintNewLine();
            _tprintf( _T("%d files found matching %s.  This command works with a single file only.\n\r"),
                listFileProperties.size(), szInFileID.c_str());
            _tprintf( _T("%s failed.\n\r"), szErrorText.c_str());
            PrintNewLine();
        #else
            _tprintf( _T("%s is ambiguous.  Possible matches are: \n\r"), szInFileID.c_str());
            PrintNewLine();
            DisplaySearchFilesResults(&searchFilesResponse, true, true);
        #endif

        return false;

   } // Multiple matches returned

    // Otherwise, we have a single match
    pFileProperties = listFileProperties[0];
    l64FileID = *pFileProperties->fileID;

    return true;

} // End GetFileID

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to get the file data from file input where
//          file input is either file ID, file name, or hash.
// Requires:
//      pStorageService: reference to storage service
//      szInFileID: input file information (fileID, file name, hash)
//      pFileProperties: returned file data
//      szErrorText: error text to accompany failure scenarios.
//      bAlwaysSearch: if true, performs the search even with a file ID
//                     to return other file data (such as file size)
//                     back to the caller.
// Returns: true if successful, false otherwise
bool ConsoleControl::GetFileData(DiomedeStorageService* pStorageService, std::string szInFileID,
                                 FilePropertiesImpl* pFileProperties, std::string szErrorText /*""*/)
{
    // Get the file ID type - it should be either a filename, file ID, MD5 or SHA1 hash.
    int nFileIDType = ConsoleControl::unknownType;
    GetFileIDType(szInFileID, nFileIDType);

    LONG64 l64FileID = 0;
    bool bUseFirstOneOK = false;

    // If it's not a file ID, search for the file to get it's ID.
    _sds__SearchFilesResponse searchFilesResponse;
    _sds__SearchFilesRequest searchFilesRequest;

    searchFilesRequest.sessionToken = &m_szSessionToken;
    searchFilesRequest.offset = new LONG64(0);
    searchFilesRequest.pageSize = new LONG64(100);

    std::string szSearchType = _T("File ID ");
    if ( nFileIDType == ConsoleControl::fileIDType ) {
        szSearchType = _T("File ID ");
        l64FileID = atoi64(szInFileID.c_str());
        searchFilesRequest.fileID = new LONG64(l64FileID);
    }
    else if ( nFileIDType == ConsoleControl::fileType ) {
        szSearchType = _T("Filename ");
        searchFilesRequest.fileName = new std::string(szInFileID);
    }
    else if (nFileIDType == ConsoleControl::md5Type) {
        szSearchType = _T("MD5 ");
        searchFilesRequest.hashMD5 = new std::string(szInFileID);
        bUseFirstOneOK = true;
    }
    else if (nFileIDType == ConsoleControl::sha1Type) {
        szSearchType = _T("SHA1 ");
        searchFilesRequest.hashSHA1 = new std::string(szInFileID);
        bUseFirstOneOK = true;
    }

    // Currently, the only way to get file properties is to search for the file.
    // Hopefully, we only get 1 response - we get multiple responses, alert
    // the user and return.
    bool bSuccess = SearchFiles(pStorageService, &searchFilesRequest, &searchFilesResponse);

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (searchFilesRequest.offset != NULL) {
        delete searchFilesRequest.offset;
        searchFilesRequest.offset = NULL;
    }
    if (searchFilesRequest.pageSize != NULL) {
        delete searchFilesRequest.pageSize;
        searchFilesRequest.pageSize = NULL;
    }
    if ( searchFilesRequest.fileID != NULL) {
        delete searchFilesRequest.fileID;
        searchFilesRequest.fileID = NULL;
    }
    if ( searchFilesRequest.fileName != NULL) {
        delete searchFilesRequest.fileName;
        searchFilesRequest.fileName = NULL;
    }
    if ( searchFilesRequest.hashMD5 != NULL) {
        delete searchFilesRequest.hashMD5;
        searchFilesRequest.hashMD5 = NULL;
    }
    if ( searchFilesRequest.hashSHA1 != NULL) {
        delete searchFilesRequest.hashSHA1;
        searchFilesRequest.hashSHA1 = NULL;
    }

    //-----------------------------------------------------------------
    // Process results
    //-----------------------------------------------------------------
    if ( false == bSuccess ) {
        std::string szErrorMsg = szSearchType + szInFileID + _T(" not found.  ");
        if (szErrorText.length() > 0) {
            szErrorMsg += szErrorText + _T(" failed.");
        }
        _tprintf(_T(" %s\n\r"), szErrorMsg.c_str());
        return false;
    }

    dds__ArrayOfFileProperties* pMatchedFiles = searchFilesResponse.matchedFiles;
    std::vector<class dds__FileProperties * > listFileProperties = pMatchedFiles->FileProperties;

    // Edge case to ensure we did get something back.
    if (listFileProperties.size() == 0) {
        std::string szErrorMsg = szSearchType + szInFileID + _T(" not found.  ");
        if (szErrorText.length() > 0) {
            szErrorMsg += szErrorText + _T(" failed.");
        }
        _tprintf(_T(" %s\n\r"), szErrorMsg.c_str());
        return false;
    }

    // Check for multiple hits - means search came back with mulitple hits.
    if (!bUseFirstOneOK && (listFileProperties.size() > 1) ) {

        #if 1
            PrintNewLine();
            _tprintf( _T("%d files found matching %s.  This command works with a single file only.\n\r"),
                listFileProperties.size(), szInFileID.c_str());
            _tprintf( _T("%s failed.\n\r"), szErrorText.c_str());
            PrintNewLine();
        #else
            _tprintf( _T("%s is ambiguous.  Possible matches are: \n\r"), szInFileID.c_str());
            PrintNewLine();
            DisplaySearchFilesResults(&searchFilesResponse, true, true);
        #endif

        return false;

   } // Multiple matches returned

    // Otherwise, we have a single match
    dds__FileProperties* pSvcFileProperties = listFileProperties[0];
    if (pSvcFileProperties->fileID) {
        pFileProperties->SetFileID(*pSvcFileProperties->fileID);
    }
    if (pSvcFileProperties->fileName) {
        pFileProperties->SetFileName(*pSvcFileProperties->fileName);
    }
    if (pSvcFileProperties->hashMD5) {
        pFileProperties->SetHashMD5(*pSvcFileProperties->hashMD5);
    }
    if (pSvcFileProperties->hashSHA1) {
        pFileProperties->SetHashSHA1(*pSvcFileProperties->hashSHA1);
    }
    if (pSvcFileProperties->fileSize) {
        pFileProperties->SetFileSize(*pSvcFileProperties->fileSize);
    }
    if (pSvcFileProperties->lastModifiedDate) {
        pFileProperties->SetLastModifiedDate(*pSvcFileProperties->lastModifiedDate);
    }
    if (pSvcFileProperties->lastAccessDate) {
        pFileProperties->SetLastAccess(*pSvcFileProperties->lastAccessDate);
    }
    if (pSvcFileProperties->isDeleted) {
        pFileProperties->SetIsDeleted(*pSvcFileProperties->isDeleted);
    }
    if (pSvcFileProperties->isComplete) {
        pFileProperties->SetIsCompleted(*pSvcFileProperties->isComplete);
    }
    if (pSvcFileProperties->power) {
        pFileProperties->SetPower(*pSvcFileProperties->power);
    }

    return true;

} // End GetFileData

///////////////////////////////////////////////////////////////////////
// Purpose: Process the download command using the CPP SDK
// Requires:`
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessDownloadCommand(CmdLine* pCmdLine,
                                            bool& bCommandFinished)
{
    // User must be logged in to download a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot download file"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Download file: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    //-----------------------------------------------------------------
    // NOTE:
    // Here, the file ID can either be a filename, fileid, or hash value
    // (md5 or SHA1).
    //-----------------------------------------------------------------

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeValueArg<std::string>* pDirArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEINFO);
        pDirArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_DIRECTORY);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_DOWNLOAD << endl;
    }

    if ( pFileIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

	if (pFileIDArg->getValue().length() == 0) {

        if (pFileIDArg->getRepromptCount() == 0) {
    	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
            pFileIDArg->incrementRepromptCount();
        }
        else if (pFileIDArg->getRepromptCount() < 2) {
            pFileIDArg->incrementRepromptCount();
            bCommandFinished = true;
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
        }

	    return;
	}

    bCommandFinished = true;

    //----------------------------------------------------------------
    // Remove any quotes present
    //----------------------------------------------------------------
    std::string szFileID = _T("");
    RemoveQuotesFromArgument(pFileIDArg->getValue(), szFileID);

    //----------------------------------------------------------------
    // Get the filed ID from the input file argument.
    //----------------------------------------------------------------
    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    //----------------------------------------------------------------
    // Check for the optional directory path
    //----------------------------------------------------------------
    std::string szDirectory = _T("");
    if (pDirArg && pDirArg->isSet()) {
        szDirectory = pDirArg->getValue();

        char szDirBuffer[MAX_PATH];
	    strcpy(szDirBuffer, szDirectory.c_str());

        // Remove the quotes set by the user - if the path has spaces,
        // the quotes may be needed with wget (current testing
        // indicates wget does not handle quoted paths).
        std::string szTrimmedDir = _tcsicrem(szDirBuffer, '"');

        // Make sure it's a valid path
        if (false == Util::IsDirectory(szTrimmedDir.c_str())) {
            _tprintf(_T("Download file: the directory %s is not valid.\n\r"), szDirectory.c_str());
            return;
        }
    }

    // If no directory has been supplied, try to get the working directory.
    if (szDirectory.length() == 0) {
        Util::GetWorkingDirectory(szDirectory);
    }

    //----------------------------------------------------------------
    // Setup the download data structure
    //----------------------------------------------------------------
    DownloadImpl downloadData;
    downloadData.SetDownloadPath(szDirectory);
    downloadData.SetDownloadCallback(&DownloadStatus);
    downloadData.SetDownloadURLCallback(&DownloadURLStatus);
    downloadData.SetFileID(l64FileID);
    downloadData.SetDownloadUser(this);

    //-----------------------------------------------------------------
    // Curl proxy setup.
    //-----------------------------------------------------------------
    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), false );
    if (pProfileData) {
        std::string szTemp = pProfileData->GetUserProfileStr(GEN_PROXY_HOST, GEN_PROXY_HOST_DF);
        if (szTemp.length() > 0)
        {
            downloadData.SetProxyHost(szTemp);
                        
            int nPort = pProfileData->GetUserProfileInt(GEN_PROXY_PORT, GEN_PROXY_PORT_DF);
            downloadData.SetProxyPort(nPort);
            
            szTemp = pProfileData->GetUserProfileStr(GEN_PROXY_USERID, GEN_PROXY_USERID_DF);
            downloadData.SetProxyUserID(szTemp);
            
            szTemp = pProfileData->GetUserProfileStr(GEN_PROXY_PASSWORD, GEN_PROXY_PASSWORD_DF);
            downloadData.SetProxyPassword(szTemp);
        }
    }

    g_bCanContinueTimer = false;

    //-----------------------------------------------------------------
    // Process download
    //-----------------------------------------------------------------
    PrintNewLine();

    // To make sure there is no data leftover from a prior call.
    if (m_pDownloadInfo != NULL) {
        delete m_pDownloadInfo;
        m_pDownloadInfo = NULL;
    }

    m_pDownloadInfo = new DownloadFileInfo();
    if ( false == CheckThread(&m_pDownloadInfo->m_commandThread, _T("Download"))) {
        delete m_pDownloadInfo;
        m_pDownloadInfo = NULL;
        return;
    }

    m_pDownloadInfo->ClearAll();

    ResumeManager::Instance()->ClearResumeMgrData();

    // Object storage of directory and file ID may not be needed...
    m_pDownloadInfo->m_szDownloadPath = szDirectory;
    m_pDownloadInfo->m_l64FileID = l64FileID;

	DIOMEDE_CONSOLE::DownloadTask taskDownload(m_szSessionToken, &downloadData);
    m_pDownloadInfo->m_commandThread.Event(&taskDownload);

    while ( taskDownload.Status() != TaskStatusCompleted ) {
        if (g_bCanContinueTimer) {
            m_pDownloadInfo->m_msgTimer.ContinueTime();
        }

        if (false == PauseProcess() ) {
            taskDownload.CancelTask();
        }
    }

    //-----------------------------------------------------------------
    // Result here is 0 for success, 1 failure.
    //-----------------------------------------------------------------

    // Clear the control key usage bool now that the thread has quit.
    g_bUsingCtrlKey = false;

    int nResult =  taskDownload.GetResult();

    if (nResult == 0) {
        // Handle the scenario where an error occurs - may not be a status
        // yet, in which case we'd spin around here unnecessarily.
        while (m_pDownloadInfo->m_nDownloadStatus < DIOMEDE::downloadComplete) {
            // Waiting for status to change to complete - callback
            // into this object updates the upload status.
            m_pDownloadInfo->m_msgTimer.ContinueTime();
        }
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == 0) {

        m_pDownloadInfo->m_msgTimer.EndTime(_T(""));

        /* Seems too confusig to show the download URL here as well.  We'll just
           indicate that the file was saved...
        std::string szStatusMsg = _format(_T(" URL = %s"), szDownloadURL.c_str());
        */

        // We need to show the filename if one is present, so we'll extract whatever is
        // present after the URL - this may fail if there is no filename present.
        std::string szDisplayString = downloadData.GetDownloadFileName();

        std::string szStatusMsg = _format(_T("Saved: %s"), szDisplayString.c_str());
        PrintStatusMsg(szStatusMsg, true);

        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
        return;
    }

    // Else check the return code to determine if the task can be repeated (e.g.
    // the session has expired).

    // Check for the service expires error - in this case, we'll end the
    // task string with "session expired".
    std::string szErrorMsg = taskDownload.GetServiceErrorMsg();

    std::string szFriendlyMsg = _T("");
    if ( CheckServiceErrorToRetry(szErrorMsg, szFriendlyMsg)) {

        // The session retries is reset to MAX_RETRIES as soon as the
        // user is successfully logged back into the service.
        g_bSessionError = true;
        g_nSessionRetries --;

        if (g_nSessionRetries >= 0) {
            nResult = RepeatLastDownloadTask(&taskDownload);
        }

        if (nResult == SOAP_OK) {
            // We need to show the filename if one is present, so we'll extract whatever is
            // present after the URL - this may fail if there is no filename present.
            std::string szDisplayString = downloadData.GetDownloadFileName();

            std::string szStatusMsg = _format(_T("Saved: %s"), szDisplayString.c_str());
            PrintStatusMsg(szStatusMsg, true);

            ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
            return;
        }
        else {
            // Else the error returned from the service is returned.
            m_pDownloadInfo->m_msgTimer.EndTime(_T(""));
        }
    }
    else {
        // Else the error returned from the service is returned.
        m_pDownloadInfo->m_msgTimer.EndTime(_T(""));
    }

    szErrorMsg = taskDownload.GetServiceErrorMsg();
    PrintServiceError(stderr, szErrorMsg);

    ClientLog(UI_COMP, LOG_ERROR, false,_T("Download file failed."));

} // End ProcessDownloadCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to manage the message timing of tasks.
// Requires:
//      pTask: reference to a CTask derived object
// Returns: SOAP_OK if successful, service error otherwise.
int ConsoleControl::RepeatLastDownloadTask(DiomedeTask* pTask)
{
    int nOriginalResult = pTask->GetResult();
    int nResult = 0;
    BOOL bReturn = FALSE;

    MessageTimer msgTimer;

    if (g_bSessionError) {

        g_bSessionError = false;

        // Keep the timer running, alert the user tht the original task ended
        // with session expired.
        EndTimerTypes::EndTimerType timerType = EndTimerTypes::useSingleLine;

        if ( (g_nSessionErrorType >= 0) &&
             (g_nSessionErrorType < ConsoleControl::LAST_SERVICE_TYPES_ENUM)) {

             if ( (g_nSessionErrorType == ConsoleControl::SESSION_TOKEN_EXPIRES) ||
                  (g_nSessionErrorType == ConsoleControl::INVALID_SESSION_TOKEN) ) {
                  timerType = EndTimerTypes::useSessionExpired;
             }
             else {
                  timerType = EndTimerTypes::useNetworkConnectionError;
             }
        }

        // Stop the download timer - it may not have started yet, so we'll
        // start is just so we can get the "session expired" output.
        if (m_pDownloadInfo->m_msgTimer.IsStarted() == false) {
            m_pDownloadInfo->m_msgTimer.Start(_T("Downloading"));
        }
        m_pDownloadInfo->m_msgTimer.EndTime(_T(""), timerType);

        //-------------------------------------------------------------
        // 2/26/09: The thread class get's unhappy in this scenario, and exits.
        // For now, we'll just get rid of the current thread and
        // startup a new one (need to look into a better cross-platform
        // thread class).  The difference between this usage and the "normal"
        // cases handled by HandleTask, is that we're trying to re-use
        // the same thread as opposed to using a thread allocated on the stack.
        //-------------------------------------------------------------

        DownloadFileInfo* pNewDownloadInfo = new DownloadFileInfo(*m_pDownloadInfo);
        delete m_pDownloadInfo;
        m_pDownloadInfo = pNewDownloadInfo;

        std::string szLoginUserStart = _format(_T("Login user %s"), m_szUsername.c_str());
	    DIOMEDE_CONSOLE::LoginTask taskLogin(m_szUsername, m_szPlainTextPassword);

        // Using a separate timer here for login...
        msgTimer.Start(szLoginUserStart);

        bReturn = m_pDownloadInfo->m_commandThread.Event(&taskLogin);
        if (bReturn == FALSE) {
            return nOriginalResult;
        }

	    while ( taskLogin.Status() != TaskStatusCompleted ) {
            msgTimer.ContinueTime();
            PauseProcess();
	    }

	    msgTimer.EndTime(_T(""), EndTimerTypes::useNoTimeOrDone);
        nResult = taskLogin.GetResult();

        // If this fails, just bounce out - something must be amiss with the
        // service.
        if (nResult != SOAP_OK) {
            return nResult;
        }

        m_szSessionToken = taskLogin.GetSessionToken();
        g_nSessionRetries = MAX_LOGIN_RETRIES;

	    ClientLog(UI_COMP, LOG_STATUS, false,
	        _T("Repeat last command: new session token %s."), m_szSessionToken.c_str());

        UserProfileData* pProfileData =
            ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
        if (pProfileData) {
            // If the "save" here fails, do we care?  This feature is more a nicety for the
            // user.
	        pProfileData->SetUserProfileStr(GEN_SESSION_TOKEN, m_szSessionToken.c_str());
	        pProfileData->SetUserProfileLong(GEN_SESSION_TOKEN_EXPIRES, (long)UpdateSessionTokenExpiration());
	        pProfileData->SaveUserProfile();
	    }
    }

    PrintNewLine();

    // This will be set to true on the once the create file status
    // has occurred.
    g_bCanContinueTimer = false;

    pTask->ResetTask();
    pTask->SetSessionToken(m_szSessionToken);

    bReturn = m_pDownloadInfo->m_commandThread.Event(pTask);
    if (bReturn == FALSE) {
        return nOriginalResult;
    }

    while ( pTask->Status() != TaskStatusCompleted ) {
        if (g_bCanContinueTimer) {
            m_pDownloadInfo->m_msgTimer.ContinueTime();
        }
        PauseProcess();
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
    nResult = pTask->GetResult();
    return nResult;

} // End RepeatLastDownloadTask

///////////////////////////////////////////////////////////////////////
// Purpose: Called from the callback which updates the download
//          process - here the URL information is updated.
// Requires:
//      nDownloadStatus: status of the download process.
//      szDownloadURL: download URL
//      szDownloadFileName: download filename
// Returns: nothing
void ConsoleControl::UpdateDownloadURLStatus(int nDownloadStatus, std::string szDownloadURL,
                                             std::string szDownloadFileName)
{
    if (m_pDownloadInfo == NULL) {
        return;
    }

    m_pDownloadInfo->m_nDownloadStatus = nDownloadStatus;

    // Trim the file name for status purposes.
    m_pDownloadInfo->m_szDownloadFileName = szDownloadFileName;
    m_pDownloadInfo->m_szFormattedFileName = m_pDownloadInfo->m_szDownloadFileName;

    if (m_pDownloadInfo->m_szDownloadFileName.length() > 30) {
        TrimFileName(30, m_pDownloadInfo->m_szDownloadFileName, m_pDownloadInfo->m_szFormattedFileName);
    }

    ClientLog(UI_COMP, LOG_STATUS, false, _T("UpdateDownloadURLStatus %d: URL %s Filename %s"),
        nDownloadStatus, szDownloadURL.c_str(), szDownloadFileName.c_str());

} // End UpdateDownloadURLStatus

///////////////////////////////////////////////////////////////////////
// Purpose: Called from the callback which updates the download
//          process.
// Requires:
//      nDownloadStatus: status of the download process.
//      l64CurrentBytes: bytes downloaded so far.
//      lTotalBytes: total bytes that will be downloaded.
// Returns: nothing
void ConsoleControl::UpdateDownloadStatus(int nDownloadStatus, LONG64 l64CurrentBytes /*0*/,
                                          LONG64 lTotalBytes /*0*/)
{
    if (m_pDownloadInfo == NULL) {
        return;
    }

    m_pDownloadInfo->m_nDownloadStatus = nDownloadStatus;
    m_pDownloadInfo->m_l64CurrentBytes = l64CurrentBytes;
    m_pDownloadInfo->m_l64TotalBytes = lTotalBytes;

    if (lTotalBytes > 0) {
        StringUtil::FormatByteSize(m_pDownloadInfo->m_l64TotalBytes, m_pDownloadInfo->m_szFormattedBytes,
            m_pDownloadInfo->m_szFormattedBytesType);
    }

    std::string szDownloadFile = _T("");
    bool bHandled = true;
    int nPercent = 0;

    switch (nDownloadStatus) {
        case DIOMEDE::downloadReceiving:
            {
                if (lTotalBytes > 0) {

                    nPercent =  (l64CurrentBytes == lTotalBytes) ? 100 :
                        (int) (((float)l64CurrentBytes/(float)lTotalBytes) * 100);

                    szDownloadFile = _format(_T("%s (%s %s)... %d%%"),
                        m_pDownloadInfo->m_szFormattedFileName.c_str(),
                        m_pDownloadInfo->m_szFormattedBytes.c_str(),
                        m_pDownloadInfo->m_szFormattedBytesType.c_str(), nPercent);
                }
                else {
                    szDownloadFile = _T("Downloading");
                }

                if (m_pDownloadInfo->m_msgTimer.IsStarted() == false) {
                    m_pDownloadInfo->m_msgTimer.Start(szDownloadFile);
                }

                m_pDownloadInfo->m_msgTimer.ContinueTime(szDownloadFile);
                g_bCanContinueTimer = true;
            }
            break;
        default:
            bHandled = false;
            break;
    }

    if (!bHandled) {
        return;
    }

    /*
    if (nDownloadStatus > DIOMEDE::downloadStarted) {
        m_pDownloadInfo->m_msgTimer.ContinueTime();
    }
    else {
        m_pDownloadInfo->m_msgTimer.Start(_T(" Downloading"));
    }
    */

} // End UpdateDownloadStatus

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get download url command
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetDownloadURL(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User must be logged in to receive a download URL.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get download URL"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get download URL: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeValueArg<std::string>* pMaxDownloadsArg = NULL;
    DiomedeValueArg<std::string>* pLifeTimeHrsArg = NULL;
    DiomedeValueArg<std::string>* pMaxUniqueIPsArg = NULL;
    DiomedeValueArg<std::string>* pErrorRedirectArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pMaxDownloadsArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_MAXDOWNLOADS);
        pLifeTimeHrsArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_LIFETIMEHOURS);
        pMaxUniqueIPsArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_MAXUNIQUEIPS);
        pErrorRedirectArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_ERRORREDIRECT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETDOWNLOADURL << endl;
    }

    if ( ( pFileIDArg == NULL) ||
         ( pMaxDownloadsArg == NULL) ||
         ( pLifeTimeHrsArg == NULL) ||
         ( pMaxUniqueIPsArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

	if (pFileIDArg->getValue().length() == 0) {

        if (pFileIDArg->getRepromptCount() == 0) {
    	    m_szCommandPrompt = _T("File ID: ");
            pFileIDArg->incrementRepromptCount();
        }
        else if (pFileIDArg->getRepromptCount() < 2) {
            pFileIDArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    DiomedeStorageService storageService;
    _sds__GetDownloadURLRequest downloadURLRequest;

    if (pMaxDownloadsArg && pMaxDownloadsArg->isSet()) {
        int nMaxDownloads = atoi(pMaxDownloadsArg->getValue().c_str());
        downloadURLRequest.maxDownloads = new int(nMaxDownloads);
    }
    if (pLifeTimeHrsArg && pLifeTimeHrsArg->isSet()) {
        int nLifeTimeHrs = atoi(pLifeTimeHrsArg->getValue().c_str());
        downloadURLRequest.lifetimeHours = new int(nLifeTimeHrs);
    }
    if (pMaxUniqueIPsArg && pMaxUniqueIPsArg->isSet()) {
        int nMaxUniqueIPs = atoi(pMaxUniqueIPsArg->getValue().c_str());
        downloadURLRequest.maxUniqueIPs = new int(nMaxUniqueIPs);
    }

    downloadURLRequest.errorRedirect = new std::string(_T(""));
    if (pErrorRedirectArg && pErrorRedirectArg->isSet()) {
        *downloadURLRequest.errorRedirect = pErrorRedirectArg->getValue();
    }

    bCommandFinished = true;

    std::string szFileID = pFileIDArg->getValue();
    LONG64 l64FileID = atoi64(szFileID.c_str());

    downloadURLRequest.fileID = new LONG64(l64FileID);

    std::string szDownloadURL = _T("");

    if (true == GetDownloadURL(&storageService, &downloadURLRequest, szDownloadURL) ) {
	    std::string szStatusMsg = _format(_T("URL = %s"), szDownloadURL.c_str());
        PrintStatusMsg(szStatusMsg, false, true, true);

        bool bUseClipboard = false;
        GetConfigClipboard(bUseClipboard);

        if (bUseClipboard) {
            CopyStringToClipboard(szDownloadURL);
        }
    }

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    DeleteGetDownloadURL(&downloadURLRequest);

} // End ProcessGetDownloadURL

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to get the file ID from file input where
//          file input is either file ID, file name, or hash.
// Requires:
//      pStorageService: reference to storage service
//      pDownloadURLRequest: minimally contains the file ID
//      szDownloadURL: returned download URL
// Returns: true if successful, false otherwise
bool ConsoleControl::GetDownloadURL(DiomedeStorageService* pStorageService,
                                    _sds__GetDownloadURLRequest* pDownloadURLRequest,
                                    std::string& szDownloadURL,
                                    bool bShowProgress /*true*/)
{
    szDownloadURL = _T("");

    _sds__GetDownloadURLResponse downloadURLResponse;
    pDownloadURLRequest->sessionToken = &m_szSessionToken;

    // Make sure any strings are allocated and set to empty.
    if (pDownloadURLRequest->errorRedirect == NULL) {
        pDownloadURLRequest->errorRedirect = new std::string(_T(""));
    }

    //-----------------------------------------------------------------
    // Process getdownloadurl
    //-----------------------------------------------------------------
	std::string szGetDownloadURLStart = bShowProgress ? _T("Getting download URL") : _T("");

    MessageTimer msgTimer(50, bShowProgress);
    if (bShowProgress) {
        PrintNewLine();
    }

    msgTimer.Start(szGetDownloadURLStart);

	DIOMEDE_CONSOLE::GetDownloadURLTask taskGetDownloadURL(pStorageService, pDownloadURLRequest,
	                                                       &downloadURLResponse);

    DIOMEDE_CONSOLE::CommandThread commandThread;
    if (false == CheckThread(&commandThread, _T("Get download url"))) {
        return false;
    }

    commandThread.Event(&taskGetDownloadURL);

	while ( taskGetDownloadURL.Status() != TaskStatusCompleted ) {
        msgTimer.ContinueTime();
        PauseProcess();
	}

    commandThread.Stop();
    msgTimer.EndTime(_T(""));

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
    int nResult = taskGetDownloadURL.GetResult();
	if (nResult == SOAP_OK) {
	    szDownloadURL = *(downloadURLResponse.url);

	    ClientLog(UI_COMP, LOG_STATUS, false,
	        _T("Get download url successful successful for file ID %d.  Download URL returned %s."),
	        pDownloadURLRequest->fileID, downloadURLResponse.url->c_str());
	}
	else {
	    PrintSoapError(pStorageService->soap, stderr, false, !bShowProgress);
	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get download URL failed for file ID %d."),
	        pDownloadURLRequest->fileID);
	}

    return (nResult == SOAP_OK);

} // End GetDownloadURL

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to deallocated the attributes passed
//          into the GetDownloadURL request.
// Requires:
//      pDownloadURLRequest: minimally contains the file ID
// Returns: true if successful, false otherwise
bool ConsoleControl::DeleteGetDownloadURL(_sds__GetDownloadURLRequest* pDownloadURLRequest)
{
    if (pDownloadURLRequest->fileID != NULL) {
        delete pDownloadURLRequest->fileID;
        pDownloadURLRequest->fileID = NULL;
    }
    if (pDownloadURLRequest->maxDownloads != NULL) {
        delete pDownloadURLRequest->maxDownloads;
        pDownloadURLRequest->maxDownloads = NULL;
    }
    if (pDownloadURLRequest->lifetimeHours != NULL) {
        delete pDownloadURLRequest->lifetimeHours;
        pDownloadURLRequest->lifetimeHours = NULL;
    }
    if (pDownloadURLRequest->maxUniqueIPs != NULL) {
        delete pDownloadURLRequest->maxUniqueIPs;
        pDownloadURLRequest->maxUniqueIPs = NULL;
    }
    if (pDownloadURLRequest->errorRedirect != NULL) {
        delete pDownloadURLRequest->errorRedirect;
        pDownloadURLRequest->errorRedirect = NULL;
    }

    return true;

} // End DeleteGetDownloadURL

///////////////////////////////////////////////////////////////////////
// Purpose: Process the getdownloadurl command using the CPP SDK
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetDownloadURLUsingLib(CmdLine* pCmdLine,
                                                   bool& bCommandFinished)
{
    // User must be logged in to receive a download URL.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get download URL"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get download URL: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeValueArg<std::string>* pMaxDownloadsArg = NULL;
    DiomedeValueArg<std::string>* pLifeTimeHrsArg = NULL;
    DiomedeValueArg<std::string>* pMaxUniqueIPsArg = NULL;
    DiomedeValueArg<std::string>* pErrorRedirectArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pMaxDownloadsArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_MAXDOWNLOADS);
        pLifeTimeHrsArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_LIFETIMEHOURS);
        pMaxUniqueIPsArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_MAXUNIQUEIPS);
        pErrorRedirectArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_ERRORREDIRECT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETDOWNLOADURL << endl;
    }

    if ( ( pFileIDArg == NULL) ||
         ( pMaxDownloadsArg == NULL) ||
         ( pLifeTimeHrsArg == NULL) ||
         ( pMaxUniqueIPsArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

	if (pFileIDArg->getValue().length() == 0) {

        if (pFileIDArg->getRepromptCount() == 0) {
    	    m_szCommandPrompt = _T("File ID: ");
            pFileIDArg->incrementRepromptCount();
        }
        else if (pFileIDArg->getRepromptCount() < 2) {
            pFileIDArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    DownloadURLImpl downloadURLInfo;

    if (pMaxDownloadsArg && pMaxDownloadsArg->isSet()) {
        int nMaxDownloads = atoi(pMaxDownloadsArg->getValue().c_str());
        downloadURLInfo.SetMaxDownloads(nMaxDownloads);
    }
    if (pLifeTimeHrsArg && pLifeTimeHrsArg->isSet()) {
        int nLifeTimeHrs = atoi(pLifeTimeHrsArg->getValue().c_str());
        downloadURLInfo.SetLifetimeHours(nLifeTimeHrs);
    }
    if (pMaxUniqueIPsArg && pMaxUniqueIPsArg->isSet()) {
        int nMaxUniqueIPs = atoi(pMaxUniqueIPsArg->getValue().c_str());
        downloadURLInfo.SetMaxUniqueIPs(nMaxUniqueIPs);
    }

    if (pErrorRedirectArg && pErrorRedirectArg->isSet()) {
        downloadURLInfo.SetErrorRedirect(pErrorRedirectArg->getValue());
    }

    bCommandFinished = true;

    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    downloadURLInfo.SetFileID(l64FileID);

    //-----------------------------------------------------------------
    // Process searchfiles
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::GetDownloadURLTask taskGetDownloadURL(m_szSessionToken, &downloadURLInfo);
	int nResult = HandleTask(&taskGetDownloadURL, _T("Getting download URL"));

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
    std::string szDownloadURL = _T("");

	if (nResult == SOAP_OK) {
	    szDownloadURL = downloadURLInfo.GetDownloadURL();

	    std::string szStatusMsg = _format(_T("URL = %s"), szDownloadURL.c_str());
        PrintStatusMsg(szStatusMsg, false, true, true);

        bool bUseClipboard = false;
        GetConfigClipboard(bUseClipboard);

        if (bUseClipboard) {
            CopyStringToClipboard(szDownloadURL);
        }

	    ClientLog(UI_COMP, LOG_STATUS, false,
	        _T("Get download url successful successful for file ID %s.  Download URL returned %s."),
	        szFileID.c_str(), szDownloadURL.c_str());
	}
	else {
	    std::string szErrorMsg = taskGetDownloadURL.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get download URL failed for file ID %s: %s"),
	        szFileID.c_str(), szErrorMsg.c_str());
	}

} // End ProcessGetDownloadURLUsingLib

///////////////////////////////////////////////////////////////////////
// Purpose: Process the rename file command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessRenameFileCommand(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User must be logged in to rename a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot rename file"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Rename file: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pNewFileNameArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pNewFileNameArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILENAME);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_RENAMEFILE << endl;
    }

    if ( ( pFileIDArg == NULL) || ( pNewFileNameArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pFileIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
	    pTmpArg = pFileIDArg;
	}
	else if (pNewFileNameArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("New file name: ");
	    pTmpArg = pNewFileNameArg;
	}

	bCommandFinished = ( ( pFileIDArg->getValue().length() > 0 ) &&
	                     ( pNewFileNameArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    // Now that we have a file ID, we can rename the file.

    //-----------------------------------------------------------------
    // Process renamefile
    //-----------------------------------------------------------------
    std::string szNewFileName = pNewFileNameArg->getValue();
    std::string szRenameFileStart = _format(_T("Renaming file %s to %s"), szFileID.c_str(),
        szNewFileName.c_str());

	DIOMEDE_CONSOLE::RenameFileTask taskRenameFile(m_szSessionToken, l64FileID, szNewFileName);
	int nResult = HandleTask(&taskRenameFile, szRenameFileStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Rename file %s to %s successful."),
	        szFileID.c_str(), szNewFileName.c_str());
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskRenameFile.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Rename file %s to %s failed."),
	        szFileID.c_str(), szNewFileName.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessRenameFileCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the delete file command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessDeleteFileCommand(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User must be logged into the Diomede Service to delete a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot delete file"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Delete file: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    //-----------------------------------------------------------------
    // NOTE:
    // Here, the file ID can either be a filename, fileid, or hash value
    // (md5 or SHA1).
    //-----------------------------------------------------------------
    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEINFO);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_DELETEFILE << endl;
    }

    if ( pFileIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

	if (pFileIDArg->getValue().length() == 0) {

        if (pFileIDArg->getRepromptCount() == 0) {
    	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
            pFileIDArg->incrementRepromptCount();
        }
        else if (pFileIDArg->getRepromptCount() < 2) {
            pFileIDArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    // Now that we have a file ID, we can delete the file.

    //-----------------------------------------------------------------
    // Process deletefile
    //-----------------------------------------------------------------
    std::string szDeleteFileStart = _format(_T("Deleting file %s"), szFileID.c_str());

	DIOMEDE_CONSOLE::DeleteFileTask taskDeleteFile(m_szSessionToken, l64FileID);
	int nResult = HandleTask(&taskDeleteFile, szDeleteFileStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Delete file %s successful."),
	        szFileID.c_str());
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskDeleteFile.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Delete file %s failed."), szFileID.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessDeleteFileCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the undelete file command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessUndeleteFileCommand(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User must be logged into the Diomede Service to undelete a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot undelete file"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Undelete file: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    //-----------------------------------------------------------------
    // NOTE:
    // Here, the file ID can either be a filename, fileid, or hash value
    // (md5 or SHA1).
    //-----------------------------------------------------------------
    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEINFO);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_UNDELETEFILE << endl;
    }

    if ( pFileIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

	if (pFileIDArg->getValue().length() == 0) {

        if (pFileIDArg->getRepromptCount() == 0) {
    	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
            pFileIDArg->incrementRepromptCount();
        }
        else if (pFileIDArg->getRepromptCount() < 2) {
            pFileIDArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    // Now that we have a file ID, we can undelete the file.

    //-----------------------------------------------------------------
    // Process undeletefile
    //-----------------------------------------------------------------
    std::string szUndeleteFileStart = _format(_T("Undeleting file %s"), szFileID.c_str());

	DIOMEDE_CONSOLE::UndeleteFileTask taskUndeleteFile(m_szSessionToken, l64FileID);
	int nResult = HandleTask(&taskUndeleteFile, szUndeleteFileStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Undelete file %s successful."), szFileID.c_str());
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskUndeleteFile.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Undelete file %s failed."), szFileID.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessUndeleteFileCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Called from the callback which updates the display file
//          process.
// Requires:
//      nDisplayFileStatus: status of the display file process.
//      l64CurrentBytes: bytes displayed so far.
//      lTotalBytes: total bytes that will be displayed.
// Returns: nothing
void ConsoleControl::UpdateDisplayFileStatus(int nDisplayFileStatus, LONG64 l64CurrentBytes /*0*/,
                                             LONG64 lTotalBytes /*0*/)
{
    if (m_pDisplayFileInfo == NULL) {
        return;
    }

    m_pDisplayFileInfo->m_nDisplayFileStatus = nDisplayFileStatus;
    m_pDisplayFileInfo->m_l64CurrentBytes = l64CurrentBytes;

    if (nDisplayFileStatus == DIOMEDE::displayFileDisplaying) {
        m_pDisplayFileInfo->m_l64TotalBytes = lTotalBytes;
    }

    /*
    ClientLog(UI_COMP, LOG_STATUS, false, _T("UpdateDisplayFileStatus status %d bytes %I64d"),
        m_pDisplayFileInfo->m_nDisplayFileStatus, m_pDisplayFileInfo->m_l64TotalBytes);
    */

} // End UpdateDisplayFileStatus

///////////////////////////////////////////////////////////////////////
// Purpose: Called from the callback which updates the display file
//          enumeration process.
// Requires:
//      nDisplayFileEnumStatus: status of the display file process - 
//                              indicates the current count of files.
// Returns: nothing
void ConsoleControl::UpdateDisplayFileEnumStatus(int nDisplayFileEnumStatus)
{
    if (m_pDisplayFileEnumInfo == NULL) {
        return;
    }

    // Test the control key flag directly without using "pause".
    // We want the enumeration to happen as efficiently as possible
    // but at the same time allow for the user to quit.
    if ( g_bUsingCtrlKey ) {
        if (m_pFileEnumerator != NULL) {
            m_pFileEnumerator->m_bCancelled = true;
        }
    }

    // Status here represents the current count of files.
    m_pDisplayFileEnumInfo->m_nDisplayFileEnumStatus = nDisplayFileEnumStatus;

    // Update every 50 files, unless the user has cancelled.
    if ((nDisplayFileEnumStatus % 50 != 0) && (m_pFileEnumerator->m_bCancelled == false) ) {
            return;
    }
    
    std::string szEnumFiles =  _format(_T("%d ... enumerating files "), nDisplayFileEnumStatus);
    m_pDisplayFileEnumInfo->m_msgTimer.ContinueTime(szEnumFiles);

} // End UpdateDisplayFileEnumStatus

///////////////////////////////////////////////////////////////////////
// Purpose: Process the display file command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessDisplayFileCommand(CmdLine* pCmdLine,
                                               bool& bCommandFinished)
{
    // User must be logged into the Diomede Service to display a file contents
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot display file contents"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Display file contents: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    //-----------------------------------------------------------------
    // NOTE:
    // Here, the file ID can either be a filename, fileid, or hash value
    // (md5 or SHA1).
    //-----------------------------------------------------------------

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeValueArg<std::string>* pStartByteArg = NULL;
    DiomedeValueArg<std::string>* pEndByteArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEINFO);
        pStartByteArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_STARTBYTE);
        pEndByteArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_ENDBYTE);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_DISPLAYFILE << endl;
    }

    if ( pFileIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

	if (pFileIDArg->getValue().length() == 0) {

        if (pFileIDArg->getRepromptCount() == 0) {
    	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
            pFileIDArg->incrementRepromptCount();
        }
        else if (pFileIDArg->getRepromptCount() < 2) {
            pFileIDArg->incrementRepromptCount();
            bCommandFinished = true;
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
        }

	    return;
	}

    bCommandFinished = true;

    //----------------------------------------------------------------
    // Remove any quotes present
    //----------------------------------------------------------------
    std::string szFileID = _T("");
    RemoveQuotesFromArgument(pFileIDArg->getValue(), szFileID);

    //----------------------------------------------------------------
    // Get the filed ID from the input file argument.
    //----------------------------------------------------------------
    DiomedeStorageService storageService;
    FilePropertiesImpl fileProperties;

    if (false == GetFileData(&storageService, szFileID, &fileProperties, pCmdLine->getCommandName())) {
        return;
    }

    LONG64 l64FileID = fileProperties.GetFileID();
    LONG64 l64FileSize = fileProperties.GetFileSize();

    //----------------------------------------------------------------
    // Check for the start and end bytes
    //----------------------------------------------------------------
    LONG64 l64StartByte = 0;
    LONG64 l64EndByte = l64FileSize - 1;

    std::string szRange = _T("0");
    std::string szStartByte = _T("0");
    std::string szEndByte = _T("0");
    std::string szFileSize = _T("0");

    #ifdef WIN32
        szFileSize = _format(_T("%I64d"), l64FileSize - 1);
    #else
        szFileSize = _format(_T("%lld"), l64FileSize - 1);
    #endif

    szEndByte = szFileSize;

    if (pStartByteArg && pStartByteArg->isSet()) {
        l64StartByte = atoi64(pStartByteArg->getValue().c_str());
        if (l64StartByte < 0) {
            PrintStatusMsg(_T("Start byte cannot be less than 0."), true);
            return;
        }

        if (l64StartByte > l64FileSize) {
            std::string szErrorMsg = _T("Start byte cannot be greater than the file size ") + szFileSize;
            PrintStatusMsg(szErrorMsg, true);
            return;
        }

        szStartByte = pStartByteArg->getValue();
    }

    if (pEndByteArg && pEndByteArg->isSet()) {

        szEndByte = pEndByteArg->getValue();
        l64EndByte = atoi64(szEndByte.c_str());

        if ( (l64EndByte < 0) || (l64EndByte < l64StartByte) )  {
            PrintStatusMsg(_T("Start byte cannot be less than end byte."), true);
            return;
        }

        if (l64EndByte > l64FileSize) {
            l64EndByte = l64FileSize;
            szEndByte = szFileSize;
        }
    }

    szRange = szStartByte + _T("-") + szEndByte;

    //----------------------------------------------------------------
    // Setup the download data structure
    //----------------------------------------------------------------
    DisplayFileContentsImpl displayFileData;
    displayFileData.SetDisplayFileCallback(&DisplayFileStatus);
    displayFileData.SetDownloadURLCallback(&DownloadURLStatus);
    displayFileData.SetDisplayFileUser(this);

    displayFileData.SetFileID(l64FileID);
    displayFileData.SetStartByte(l64StartByte);
    displayFileData.SetEndByte(l64EndByte);
    displayFileData.SetFormattedDisplayRange(szRange);

    //-----------------------------------------------------------------
    // Curl proxy setup.
    //-----------------------------------------------------------------
    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), false );
    if (pProfileData) {
        std::string szTemp = pProfileData->GetUserProfileStr(GEN_PROXY_HOST, GEN_PROXY_HOST_DF);
        if (szTemp.length() > 0)
        {
            displayFileData.SetProxyHost(szTemp);
                        
            int nPort = pProfileData->GetUserProfileInt(GEN_PROXY_PORT, GEN_PROXY_PORT_DF);
            displayFileData.SetProxyPort(nPort);
            
            szTemp = pProfileData->GetUserProfileStr(GEN_PROXY_USERID, GEN_PROXY_USERID_DF);
            displayFileData.SetProxyUserID(szTemp);
            
            szTemp = pProfileData->GetUserProfileStr(GEN_PROXY_PASSWORD, GEN_PROXY_PASSWORD_DF);
            displayFileData.SetProxyPassword(szTemp);
        }
    }

    g_bCanContinueTimer = false;

    //-----------------------------------------------------------------
    // Process download
    //-----------------------------------------------------------------
    PrintNewLine();

    // To make sure there is no data leftover from a prior call.
    if (m_pDisplayFileInfo != NULL) {
        delete m_pDisplayFileInfo;
        m_pDisplayFileInfo = NULL;
    }

    m_pDisplayFileInfo = new DisplayFileInfo();
    if ( false == CheckThread(&m_pDisplayFileInfo->m_commandThread, _T("Display file"))) {
        delete m_pDisplayFileInfo;
        m_pDisplayFileInfo = NULL;
        return;
    }

    m_pDisplayFileInfo->ClearAll();

    // Object storage of directory and file ID may not be needed...
    m_pDisplayFileInfo->m_l64FileID = l64FileID;

	DIOMEDE_CONSOLE::DisplayFileTask taskDisplayFile(m_szSessionToken, &displayFileData);
    m_pDisplayFileInfo->m_commandThread.Event(&taskDisplayFile);

    while ( taskDisplayFile.Status() != TaskStatusCompleted ) {
        if (g_bCanContinueTimer) {
            m_pDisplayFileInfo->m_msgTimer.ContinueTime();
        }

        if (false == PauseProcess() ) {
            taskDisplayFile.CancelTask();
        }
    }

    //-----------------------------------------------------------------
    // Result here is 0 for success, 1 failure.
    //-----------------------------------------------------------------

    // Clear the control key usage bool now that the thread has quit.
    g_bUsingCtrlKey = false;

    int nResult =  taskDisplayFile.GetResult();

    if (nResult == 0) {
        // Handle the scenario where an error occurs - may not be a status
        // yet, in which case we'd spin around here unnecessarily.
        while (m_pDisplayFileInfo->m_nDisplayFileStatus < DIOMEDE::displayFileComplete) {
            // Waiting for status to change to complete - callback
            // into this object updates the upload status.
            m_pDisplayFileInfo->m_msgTimer.ContinueTime();
        }
    }

    m_pDisplayFileInfo->m_msgTimer.EndTime(_T(""));

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != 0) {
	    std::string szErrorMsg = taskDisplayFile.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg, true);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Display file failed."));
	    return;
	}

    // We need to show the filename if one is present, so we'll extract whatever is
    // present after the URL - this may fail if there is no filename present.
    std::string szDisplayString = displayFileData.GetDownloadFileName();

    LONG64 lStartByte = displayFileData.GetStartByte();
    LONG64 lEndByte = displayFileData.GetEndByte();

    std::string szTotalBytes = _T("");
    std::string szFileSizeType = _T("");

    StringUtil::FormatByteSize(m_pDisplayFileInfo->m_l64TotalBytes, szTotalBytes, szFileSizeType);
    std::string szResults = szTotalBytes + _T(" ") + szFileSizeType;

    #ifdef WIN32
        std::string szDisplayedRange = _format(_T("%I64d to %I64d"), lStartByte, lEndByte);
    #else
        std::string szDisplayedRange = _format(_T("%lld to %lld"), lStartByte, lEndByte);
    #endif

    std::string szStatusMsg = _format(_T("Displayed: %s bytes %s, %s"), szDisplayString.c_str(),
        szDisplayedRange.c_str(), szResults.c_str());

    // We need an extra newline here
    PrintNewLine();
    PrintStatusMsg(szStatusMsg, true);

    ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());

} // End ProcessDisplayFileCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the create metadata command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessCreateMetaDataCommand(CmdLine* pCmdLine,
                                                  bool& bCommandFinished)
{
    // User must be logged in to set metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot create metadata"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Create metadata: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pNameArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pValueArg = NULL;

    try {
        pNameArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_METANAME);
        pValueArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_METAVALUE);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_CREATEMETADATA << endl;
    }

    if ( ( pNameArg == NULL) || ( pValueArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pNameArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Metadata name: ");
	    pTmpArg = pNameArg;
	}
	else if (pValueArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Metadata value: ");
	    pTmpArg = pValueArg;
	}

	bCommandFinished = ( ( pNameArg->getValue().length() > 0 ) &&
	                     ( pValueArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;

    MetaDataInfoImpl* pMetaDataInfo = new MetaDataInfoImpl;
    pMetaDataInfo->SetMetaDataName(pNameArg->getValue());
    pMetaDataInfo->SetMetaDataValue(pValueArg->getValue());

    //-----------------------------------------------------------------
    // Process createmetadata
    //-----------------------------------------------------------------
    std::string szName = pNameArg->getValue();
    std::string szValue = pValueArg->getValue();

    std::string szCreateMetaDataStart = _format(_T("Create metadata for (%s, %s)"),
        szName.c_str(), szValue.c_str() );

	DIOMEDE_CONSOLE::CreateMetaDataTask taskCreateMetaData(m_szSessionToken, pMetaDataInfo);
	int nResult = HandleTask(&taskCreateMetaData, szCreateMetaDataStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Metadata ID for (%s, %s): %d"),
	        szName.c_str(), szValue.c_str(), pMetaDataInfo->GetMetaDataID());
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskCreateMetaData.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Create metadata for (%s, %s) failed."),
            szName.c_str(), szValue.c_str() );
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pMetaDataInfo != NULL) {
        delete pMetaDataInfo;
        pMetaDataInfo = NULL;
    }

} // End ProcessCreateMetaDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the create file metadata command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessCreateFileMetaDataCommand(CmdLine* pCmdLine,
                                                    bool& bCommandFinished)
{
    // User must be logged in to set metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot create file  metadata"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Create file metadata: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pNameArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pValueArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pNameArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_METANAME);
        pValueArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_METAVALUE);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_CREATEFILEMETADATA << endl;
    }

    if ( ( pFileIDArg == NULL) || ( pNameArg == NULL) || ( pValueArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pFileIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
	    pTmpArg = pFileIDArg;
	}
	else if (pNameArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Metadata name: ");
	    pTmpArg = pNameArg;
	}
	else if (pValueArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Metadata value: ");
	    pTmpArg = pValueArg;
	}

	bCommandFinished = ( ( pFileIDArg->getValue().length() > 0 ) &&
	                     ( pNameArg->getValue().length() > 0 ) &&
	                     ( pValueArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    // Now that we have a file ID, we can set the metadata for the file.
    int nMetaDataID = -1;
    CreateFileMetaData(l64FileID, pNameArg->getValue(), pValueArg->getValue(), nMetaDataID);

} // End ProcessCreateFileMetaDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to create a new metadata for a file.
// Requires:
//      l64FileID: file ID
//      szName: metadata name
//      szValue: metadata value
//      nMetaDataID: returned metadata ID
//      bShowProgress: shows progress if true, no output otherwise
// Returns: true if successful, false otherwise.
bool ConsoleControl::CreateFileMetaData(LONG64 l64FileID, std::string szName, std::string szValue,
                                        int& nMetaDataID, bool bShowProgress /*true*/)
{
    MetaDataInfoImpl* pMetaDataInfo = new MetaDataInfoImpl;
    pMetaDataInfo->SetMetaDataName(szName);
    pMetaDataInfo->SetMetaDataValue(szValue);

    //-----------------------------------------------------------------
    // Process createfilemetadata
    //-----------------------------------------------------------------
    #ifdef WIN32
        std::string szFileID = _format(_T("%I64d"), l64FileID);
    #else
        std::string szFileID = _format(_T("%lld"), l64FileID);
    #endif
    std::string szCreateFileMetaDataStart = _T("");

    if (bShowProgress) {
        szCreateFileMetaDataStart = _format(_T("Creating metadata (%s, %s) for file %s"),
            szName.c_str(), szValue.c_str(), szFileID.c_str());
    }

	DIOMEDE_CONSOLE::CreateFileMetaDataTask taskCreateFileMetaData(m_szSessionToken,
	                                                               l64FileID, pMetaDataInfo);
	int nResult = HandleTask(&taskCreateFileMetaData, szCreateFileMetaDataStart, _T(""),
	                         false, true, bShowProgress);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    nMetaDataID = pMetaDataInfo->GetMetaDataID();
	    std::string szStatusMsg = _format(_T("Metadata ID %d created and set for file %s."),
	        pMetaDataInfo->GetMetaDataID(), szFileID.c_str(), szName.c_str(), szValue.c_str());

	    if (bShowProgress) {
            PrintStatusMsg(szStatusMsg);
        }

        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskCreateFileMetaData.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Create metadata (%s, %s) for file %s failed."),
	        szName.c_str(), szValue.c_str(), szFileID.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pMetaDataInfo != NULL) {
        delete pMetaDataInfo;
        pMetaDataInfo = NULL;
    }

    return (nResult == SOAP_OK);

} // End CreateFileMetaData

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the local metadata cache of a given
//          value.  If it's not found locally, look on the server.
//          Add the server data to our cache if successful.
// Requires:
//      szName: metadata name
//      szValue: metadata value
//      nMetaDataID: found metadata ID, -1 otherwise
// Returns: true if successful, false otherwise.
bool ConsoleControl::CheckMetaDataCache(std::string szName, std::string szValue, int& nMetaDataID)
{
    nMetaDataID = -1;
    bool bSuccess = false;

    if (szName == g_mdSourceFullPath) {
        MetaDataMap::iterator iter = m_listMetaDataFullPaths.find(szValue);
        if ( iter != m_listMetaDataFullPaths.end() ) {
            nMetaDataID = iter->second;
            bSuccess = true;
        }
        else {
            bSuccess = CheckForMetaDataOnServer(szName, szValue, nMetaDataID);
            if (bSuccess) {
                m_listMetaDataFullPaths.insert(std::make_pair(szValue, nMetaDataID));
            }
        }
    }
    else if (szName == g_mdSourceRelativePath) {
        MetaDataMap::iterator iter = m_listMetaDataRelativePaths.find(szValue);
        if ( iter != m_listMetaDataRelativePaths.end() ) {
            nMetaDataID = iter->second;
            bSuccess = true;
        }
        else {
            bSuccess = CheckForMetaDataOnServer(szName, szValue, nMetaDataID);
            if (bSuccess) {
                m_listMetaDataRelativePaths.insert(std::make_pair(szValue, nMetaDataID));
            }
        }
    }

    return bSuccess;

} // End CheckMetaDataCache

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to check the local metadata cache of a given
//          value.  If it's not found locally, look on the server.
//          Add the server data to our cache if successful.
// Requires:
//      szName: metadata name
//      szValue: metadata value
//      nMetaDataID: found metadata ID, -1 otherwise
// Returns: true if successful, false otherwise.
bool ConsoleControl::CheckForMetaDataOnServer(std::string szName, std::string szValue, int& nMetaDataID)
{
    nMetaDataID = -1;
    bool bSuccess = false;

    MetaDataInfoImpl* pMetaDataInfo = new MetaDataInfoImpl;
    pMetaDataInfo->SetMetaDataName(szName);
    pMetaDataInfo->SetMetaDataValue(szValue);

    //-----------------------------------------------------------------
    // Process the getmetadata command
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::GetMetaDataTask taskGetMetaData(m_szSessionToken, pMetaDataInfo);
	int nResult = HandleTask(&taskGetMetaData, _T(""), _T(""), false, false, false);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    ClientLog(UI_COMP, LOG_ERROR, false,_T("CheckForMetaDataOnServer: Get metadata failed."));
	}
	else {
        ClientLog(UI_COMP, LOG_STATUS, false, _T("CheckForMetaDataOnServer; Get metadata successful."));

        MetaDataListImpl* pListMetaData = taskGetMetaData.GetMetaDataResults();

        // Currently, the server returns all hits using "starts with" - so we
        // need to do additional filtering here.
        if (pListMetaData->GetMetaDataList().size() > 0) {
            std::vector<void * >listMetaData = pListMetaData->GetMetaDataList();

            int nIndex = 0;
            MetaDataInfoImpl* pMetaData = NULL;

            for (int nIndex = 0; nIndex < (int)listMetaData.size(); nIndex ++) {

                pMetaData = (MetaDataInfoImpl*)listMetaData[nIndex];
                if (pMetaData != NULL) {
                    if ( (0==strcmp(pMetaData->GetMetaDataName().c_str(), szName.c_str())) &&
                         (0==strcmp(pMetaData->GetMetaDataValue().c_str(), szValue.c_str())) ) {
                        nMetaDataID = pMetaData->GetMetaDataID();
                        bSuccess = true;
                    }
                }
            }
        }
    }

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pMetaDataInfo != NULL) {
        delete pMetaDataInfo;
        pMetaDataInfo = NULL;
    }

    return bSuccess;

} // End CheckForMetaDataOnServer

///////////////////////////////////////////////////////////////////////
// Purpose: Process the set file metadata command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSetFileMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User must be logged in to set metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot set File metadata"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Set file metadata: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pMetaDataIDArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pMetaDataIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_METADATAID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SETFILEMETADATA << endl;
    }

    if ( ( pFileIDArg == NULL) || ( pMetaDataIDArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pFileIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
	    pTmpArg = pFileIDArg;
	}
	else if (pMetaDataIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Metadata ID: ");
	    pTmpArg = pMetaDataIDArg;
	}

	bCommandFinished = ( ( pFileIDArg->getValue().length() > 0 ) &&
	                     ( pMetaDataIDArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    // Now that we have a file ID, we can set the metadata for the file.
    std::string szMetaDataID = pMetaDataIDArg->getValue();
    int nMetaDataID = atoi(szMetaDataID.c_str());

    //-----------------------------------------------------------------
    // Process setfileemetadata
    //-----------------------------------------------------------------
    SetFileMetaData(l64FileID, nMetaDataID, true);

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessSetFileMetaDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to set an existing metadata to a file.
// Requires:
//      l64FileID: file ID
//      nMetaDataID: existing metadata ID
//      bShowProgress: shows progress if true, no output otherwise
// Returns: true if successful, false otherwise.
bool ConsoleControl::SetFileMetaData(LONG64 l64FileID, int nMetaDataID, bool bShowProgress /*true*/)
{
    //-----------------------------------------------------------------
    // Process setfilemetadata
    //-----------------------------------------------------------------
    #ifdef WIN32
        std::string szFileID = _format(_T("%I64d"), l64FileID);
    #else
        std::string szFileID = _format(_T("%lld"), l64FileID);
    #endif

    //-----------------------------------------------------------------
    // Process setfileemetadata
    //-----------------------------------------------------------------
    std::string szSetFileMetaDataStart = _format(_T("Setting metadata %d for file %s"),
        nMetaDataID, szFileID.c_str());

	// DIOMEDE_CONSOLE::SetFileMetaDataTask taskSetFileMetaData(m_szSessionToken, l64FileID, nMetaDataID);
    if (m_pTaskSetFileMetaData == NULL) {
        m_pTaskSetFileMetaData = new DIOMEDE_CONSOLE::SetFileMetaDataTask(m_szSessionToken,
                                                                          l64FileID, nMetaDataID);
    }
	else {
	    // Clear out any errors, reset the session token and upload data.
	    // File Manager data, including the thread, remains intact.
	    m_pTaskSetFileMetaData->ResetTask();
	    m_pTaskSetFileMetaData->SetSessionToken(m_szSessionToken);
	    m_pTaskSetFileMetaData->SetFileID(l64FileID);
	    m_pTaskSetFileMetaData->SetMetaDataID(nMetaDataID);
	}

	int nResult = HandleTask(m_pTaskSetFileMetaData, szSetFileMetaDataStart, _T(""),
	                         false, true, bShowProgress);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Set metadata %d for file %s successful."),
	        nMetaDataID, szFileID.c_str());

	    if (bShowProgress) {
            PrintStatusMsg(szStatusMsg);
        }

        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = m_pTaskSetFileMetaData->GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Set metadata %d for file %s failed."),
	        nMetaDataID, szFileID.c_str());
	}

    return true;

} // End SetFileMetaData

///////////////////////////////////////////////////////////////////////
// Purpose: Process the delet file metadata command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessDeleteFileMetaDataCommand(CmdLine* pCmdLine,
                                                      bool& bCommandFinished)
{
    // User must be logged in to delete metadata from a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot delete file metadata"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Delete file metadata: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pMetaDataIDArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pMetaDataIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_METADATAID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_DELETEFILEMETADATA << endl;
    }

    if ( ( pFileIDArg == NULL) || ( pMetaDataIDArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pFileIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
	    pTmpArg = pFileIDArg;
	}
	else if (pMetaDataIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Metadata ID: ");
	    pTmpArg = pMetaDataIDArg;
	}

	bCommandFinished = ( ( pFileIDArg->getValue().length() > 0 ) &&
	                     ( pMetaDataIDArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    // Now that we have a file ID, we can delete the metadata for the file.
    std::string szMetaDataID = pMetaDataIDArg->getValue();
    int nMetaDataID = atoi(szMetaDataID.c_str());

    //-----------------------------------------------------------------
    // Process deletefilemetadata
    //-----------------------------------------------------------------
    std::string szDeleteFileMetaDataStart = _format(_T("Deleting metadata %s for file %s"),
        szMetaDataID.c_str(), szFileID.c_str());

	DIOMEDE_CONSOLE::DeleteFileMetaDataTask taskDeleteFileMetaData(m_szSessionToken, l64FileID, nMetaDataID);
	int nResult = HandleTask(&taskDeleteFileMetaData, szDeleteFileMetaDataStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Delete metadata %s for file %s successful."),
	        szMetaDataID.c_str(), szFileID.c_str());
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskDeleteFileMetaData.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Delete metadata %s for file %s failed."),
	        szMetaDataID.c_str(), szFileID.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessDeleteFileMetaDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the delet metadata command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessDeleteMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User must be logged in to delete metadata from a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot delete metadata"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Delete metadata: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pMetaDataIDArg = NULL;

    try {
        pMetaDataIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_METADATAID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_DELETEMETADATA << endl;
    }

    if (pMetaDataIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pMetaDataIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Metadata ID: ");
	    pTmpArg = pMetaDataIDArg;
	}

	bCommandFinished = ( pMetaDataIDArg->getValue().length() > 0 );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;

    std::string szMetaDataID = pMetaDataIDArg->getValue();
    int nMetaDataID = atoi(szMetaDataID.c_str());

    //-----------------------------------------------------------------
    // Process deletemetadata
    //-----------------------------------------------------------------
    std::string szDeleteMetaDataStart = _format(_T("Deleting metadata %s"),
        szMetaDataID.c_str());

	DIOMEDE_CONSOLE::DeleteMetaDataTask taskDeleteMetaData(m_szSessionToken, nMetaDataID);
	int nResult = HandleTask(&taskDeleteMetaData, szDeleteMetaDataStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Delete metadata %s successful."),
	        szMetaDataID.c_str());
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskDeleteMetaData.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Delete metadata %s failed."),
	        szMetaDataID.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessDeleteMetaDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get file metadata command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetFileMetaDataCommand(CmdLine* pCmdLine,
                                                   bool& bCommandFinished)
{
    // User must be logged in to get metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get file metadata"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get file metadata: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETFILEMETADATA << endl;
    }

    if ( pFileIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pFileIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
	    pTmpArg = pFileIDArg;
	}

	bCommandFinished = ( pFileIDArg->getValue().length() > 0 );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    // Now that we have a file ID, we can remove the metadata for the file.

    //-----------------------------------------------------------------
    // Process getfilemetadata
    //-----------------------------------------------------------------
    std::string szGetFileMetaDataStart = _format(_T("Getting metadata for file %s"),
        szFileID.c_str());

	DIOMEDE_CONSOLE::GetFileMetaDataTask taskGetFileMetaData(m_szSessionToken, l64FileID);
	int nResult = HandleTask(&taskGetFileMetaData, szGetFileMetaDataStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {

	    std::string szErrorMsg = taskGetFileMetaData.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get metadata for file %s failed."),
	        szFileID.c_str());
	}
    else {
        ClientLog(UI_COMP, LOG_STATUS, false, _T("Get metadata for file %s successful."),
            szFileID.c_str());
        MetaDataListImpl* pListMetaData = taskGetFileMetaData.GetFileMetaDataResults();
        if (pListMetaData->GetMetaDataList().size() == 0) {
            PrintStatusMsg(_T("Get file metadata successful.  No metadata found."));
        }
        else {
            DisplayMetaData(pListMetaData);
        }
    }

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessGetFileMetaDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get metadata command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User must be logged in to get metadata.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get metadata"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get metadata: user not logged into service."));
	    return;
    }

    bCommandFinished = true;

    DiomedeValueArg<std::string>* pMetaDataIDArg = NULL;
    DiomedeValueArg<std::string>* pNameArg = NULL;
    DiomedeValueArg<std::string>* pValueArg = NULL;

    try {
        pMetaDataIDArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_METADATAID);
        pNameArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_METANAME);
        pValueArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_METAVALUE);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETMETADATA << endl;
    }

    MetaDataInfoImpl* pMetaDataInfo = new MetaDataInfoImpl;

    if (pMetaDataIDArg && pMetaDataIDArg->isSet()) {
        int nMetaDataID = atoi(pMetaDataIDArg->getValue().c_str());
        pMetaDataInfo->SetMetaDataID(nMetaDataID);
    }

    if (pNameArg && pNameArg->isSet()) {
        pMetaDataInfo->SetMetaDataName(pNameArg->getValue());
    }

    if (pValueArg && pValueArg->isSet()) {
        pMetaDataInfo->SetMetaDataValue(pValueArg->getValue());
    }

    //-----------------------------------------------------------------
    // Process getmetadata
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::GetMetaDataTask taskGetMetaData(m_szSessionToken, pMetaDataInfo);
	int nResult = HandleTask(&taskGetMetaData, _T("Getting metadata"));

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskGetMetaData.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get metadata failed."));
	}
	else {
        ClientLog(UI_COMP, LOG_STATUS, false, _T("Get metadata successful."));

        MetaDataListImpl* pListMetaData = taskGetMetaData.GetMetaDataResults();
        if (pListMetaData->GetMetaDataList().size() == 0) {
            PrintStatusMsg(_T("Get metadata successful.  No metadata found."));
        }
        else {
            DisplayMetaData(pListMetaData);
        }
    }

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pMetaDataInfo != NULL) {
        delete pMetaDataInfo;
        pMetaDataInfo = NULL;
    }

} // End ProcessGetMetaDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to display the metadata search results.
// Requires:
//      pListMetaData: list of metadata info objects
// Returns: nothing
void ConsoleControl::DisplayMetaData(MetaDataListImpl* pListMetaData)
{
    std::vector<void * >listMetaData = pListMetaData->GetMetaDataList();
    MetaDataInfoImpl* pMetaData = NULL;

    // Run through the results to find the length we need for metadata IDs
    int nMaxMetaDataNameLen = 0;
    int nMetaDataNameLen = 0;
    int nMaxMetaDataValueLen = 0;
    int nMetaDataValueLen = 0;

    int nMaxMetaDataIDLen = 0;
    std::string szOutMetaDataID = _T("");
    int nMetaDataIDLen = 0;

    std::vector<std::string> listMetaDataID;

    for (int nIndex = 0; nIndex < (int)listMetaData.size(); nIndex ++) {
        pMetaData = (MetaDataInfoImpl*)listMetaData[nIndex];
        if (pMetaData == NULL) {
            listMetaDataID.push_back(_T(""));
            continue;
        }

        szOutMetaDataID = _format(_T("%d"), pMetaData->GetMetaDataID());

        // Save the formatted string to use in the final output
        listMetaDataID.push_back(szOutMetaDataID);
        nMetaDataIDLen = (int)szOutMetaDataID.length();
        nMaxMetaDataIDLen = (nMetaDataIDLen > nMaxMetaDataIDLen) ? nMetaDataIDLen : nMaxMetaDataIDLen;

        nMetaDataNameLen = (int)pMetaData->GetMetaDataName().length();
        nMaxMetaDataNameLen = (nMetaDataNameLen > nMaxMetaDataNameLen) ? nMetaDataNameLen : nMaxMetaDataNameLen;

        nMetaDataValueLen = (int)pMetaData->GetMetaDataValue().length();
        nMaxMetaDataValueLen = (nMetaDataValueLen > nMaxMetaDataValueLen) ? nMetaDataValueLen : nMaxMetaDataValueLen;
    }

    // Space left over for the name and value pairs.  If the remaining space is > 20 (guess here)
    // we'll set the max size to 20.  The metadataspace also includes the created date + 3 spaces:
    int nDateTimeSpace = 19;
    int nMetaDataSpace = (70 - nMaxMetaDataIDLen - nDateTimeSpace) / 2;

    nMetaDataSpace = (nMetaDataSpace < 20) ? nMetaDataSpace : 20;

    if (nMaxMetaDataNameLen > nMetaDataSpace) {
        nMaxMetaDataNameLen = nMetaDataSpace;
    }

    // Give some extra space to the value column.
    if (nMaxMetaDataValueLen > nMetaDataSpace) {
        nMaxMetaDataValueLen = nMetaDataSpace + 6;
    }

    //-----------------------------------------------------------------
    // cout << _T("12345678901234567890123456789012345678901234567890123456789012345678901234567890") << endl;
    // Display results
    //-----------------------------------------------------------------

    // Display results
    std::string szName = _T("");
    std::string szValue = _T("");
    std::string szFormattedDate = _T("");

    for (int nIndex = 0; nIndex < (int)listMetaData.size(); nIndex ++) {
        pMetaData = (MetaDataInfoImpl*)listMetaData[nIndex];
        if (pMetaData == NULL) {
            continue;
        }

     	/*
        ClientLog(UI_COMP, LOG_STATUS, false, _T("DisplayMetaData: ID %d name %s value %s"),
            pMetaData->GetMetaDataID(), pMetaData->GetMetaDataName().c_str(),
            pMetaData->GetMetaDataValue().c_str());
        */

	    // Metadata ID - 3 spaces + max width for all metadata IDs, left aligned
        szOutMetaDataID = listMetaDataID[nIndex];
        nMetaDataIDLen = (int)szOutMetaDataID.length();

        szOutMetaDataID = GetPadStr(nMaxMetaDataIDLen - nMetaDataIDLen + 3) + szOutMetaDataID + GetPadStr(3);
	    _tprintf(_T("%s"), szOutMetaDataID.c_str());

	    // metadata name, truncated with ... if needed.  We'll max out both the name
	    // and value to 20 characters (unless the metadata IDs are large).
	    szName = _T("");

	    TrimFileName(nMaxMetaDataNameLen, pMetaData->GetMetaDataName(), szName);
	    szName += GetPadStr(nMaxMetaDataNameLen - szName.length() + 3);
	    _tprintf(_T("%s"), szName.c_str());

	    szValue = _T("");

	    TrimFileName(nMaxMetaDataValueLen, pMetaData->GetMetaDataValue(), szValue);
	    szValue += GetPadStr(nMaxMetaDataValueLen - szValue.length() + 3);
	    _tprintf(_T("%s"), szValue.c_str());

        szFormattedDate = _T("");

        if (StringUtil::FormatDateAndTime(pMetaData->GetCreatedDate(), szFormattedDate)) {
            if (szFormattedDate.length() > 0) {
    	        _tprintf(_T("%-s"), szFormattedDate.c_str());
            }
        }

        PrintNewLine();
    }

    if ( ((int)listMetaData.size() > 0) && !m_bSysCommandInput ) {
        PrintNewLine();
    }

} // End DisplayMetaData

///////////////////////////////////////////////////////////////////////
// Purpose: Process the edit metadata command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessEditMetaDataCommand(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot edit metadata"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Edit metadata: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pMetaDataIDArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pNameArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pValueArg = NULL;

    try {
        pMetaDataIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_METADATAID);
        pNameArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_METANAME);
        pValueArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_METAVALUE);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_EDITMETADATA << endl;
    }

    if ( (pMetaDataIDArg == NULL) || ( pNameArg == NULL) || ( pValueArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pMetaDataIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Metadata ID: ");
	    pTmpArg = pMetaDataIDArg;
	}
	else if (pNameArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("File metadata name: ");
	    pTmpArg = pNameArg;
	}
	else if (pValueArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("File metadata value: ");
	    pTmpArg = pValueArg;
	}

	bCommandFinished = ( ( pMetaDataIDArg->getValue().length() > 0 ) &&
	                     ( pNameArg->getValue().length() > 0 ) &&
	                     ( pValueArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;

    std::string szMetaDataID = pMetaDataIDArg->getValue();
    int nMetaDataID = atoi(szMetaDataID.c_str());

    MetaDataInfoImpl* pMetaDataInfo = new MetaDataInfoImpl;
    pMetaDataInfo->SetMetaDataID(nMetaDataID);
    pMetaDataInfo->SetMetaDataName(pNameArg->getValue());
    pMetaDataInfo->SetMetaDataValue(pValueArg->getValue());

    //-----------------------------------------------------------------
    // Process editmetadata
    //-----------------------------------------------------------------
    std::string szEditMetaDataStart = _format(_T("Edit metadata %s"),
        szMetaDataID.c_str());

	DIOMEDE_CONSOLE::EditMetaDataTask taskEditMetaData(m_szSessionToken, pMetaDataInfo);
	int nResult = HandleTask(&taskEditMetaData, szEditMetaDataStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Edit metadata %s successful."),
	        szMetaDataID.c_str());
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskEditMetaData.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Edit metadata %s failed."),
	        szMetaDataID.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pMetaDataInfo != NULL) {
        delete pMetaDataInfo;
        pMetaDataInfo = NULL;
    }

} // End ProcessEditMetaDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the replicate file command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessReplicateFileCommand(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot replicate file"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Replicate file: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pStorageTypeArg = NULL;
    DiomedeValueArg<std::string>* pExpiratinDateArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pStorageTypeArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_STORAGE_TYPE);
        pExpiratinDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_EXPIRATION_DATE);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_REPLICATEFILE << endl;
    }

    if ( ( pFileIDArg == NULL) || ( pStorageTypeArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pFileIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
	    pTmpArg = pFileIDArg;
	}
	else if (pStorageTypeArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Storage type: ");
	    pTmpArg = pStorageTypeArg;
	}

	bCommandFinished = ( ( pFileIDArg->getValue().length() > 0 ) &&
	                     ( pStorageTypeArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    PhysicalFileInfoImpl* pPhysicalFileInfo = new PhysicalFileInfoImpl;
    pPhysicalFileInfo->SetFileID(l64FileID);

    time_t epochSeconds;

    if (pExpiratinDateArg && pExpiratinDateArg->isSet()) {
        if ( ValidateDate(pExpiratinDateArg->getValue(), epochSeconds) ) {
            pPhysicalFileInfo->SetExpirationDate(epochSeconds);
        }
        else {
            // The expires date is required.
	        _tprintf(_T("Expires date %s is not a valid date.\n\r"),
	            pExpiratinDateArg->getValue().c_str());
	        return;
        }
    }

    int nTier = atoi(pStorageTypeArg->getValue().c_str());
    pPhysicalFileInfo->SetStorageTypeID(nTier);

    //-----------------------------------------------------------------
    // Process replicatefile
    //-----------------------------------------------------------------
    std::string szReplicateFileStart = _format(_T("Replicating logical file %s "), szFileID.c_str());

	DIOMEDE_CONSOLE::ReplicateFileTask taskReplicateFile(m_szSessionToken, pPhysicalFileInfo);
	int nResult = HandleTask(&taskReplicateFile, szReplicateFileStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
        #ifdef WIN32
	        std::string szStatusMsg =
	            _format(_T("Created request for new physical file ID: %I64d"),
	            pPhysicalFileInfo->GetPhysicalFileID() );
        #else
	        std::string szStatusMsg =
	            _format(_T("Created request for new physical file ID: %lld"),
	            pPhysicalFileInfo->GetPhysicalFileID() );
        #endif
        PrintStatusMsg(szStatusMsg);
        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskReplicateFile.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Replicate file failed for file %s."),
	        szFileID.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pPhysicalFileInfo != NULL) {
        delete pPhysicalFileInfo;
        pPhysicalFileInfo = NULL;
    }

} // End ProcessReplicateFileCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the unreplicate file command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessUnReplicateFileCommand(CmdLine* pCmdLine,
                                                   bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot unreplicate file"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Unreplicate file: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_UNREPLICATEFILE << endl;
    }

    if ( pFileIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pFileIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
	    pTmpArg = pFileIDArg;
	}

	bCommandFinished = ( pFileIDArg->getValue().length() > 0 );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    //-----------------------------------------------------------------
    // Process unreplicatefile
    //-----------------------------------------------------------------
    std::string szUnReplicateFileStart = _format(_T("Unreplicating physical file %s "), szFileID.c_str());

	DIOMEDE_CONSOLE::UnReplicateFileTask taskUnReplicateFile(m_szSessionToken, l64FileID);
	int nResult = HandleTask(&taskUnReplicateFile, szUnReplicateFileStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Unreplicate file %s successful."),
	        szFileID.c_str());
        PrintStatusMsg(szStatusMsg);
        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskUnReplicateFile.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Unreplicate file failed for file %s."),
	        szFileID.c_str());
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessUnReplicateFileCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get storage types command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetStorageTypesCommand(CmdLine* pCmdLine, bool& bCommandFinished)
{
    // User is not required to be logged into the service.
    bCommandFinished = false;
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETSTORAGETYPES << endl;
    }

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    bCommandFinished = true;

    //-----------------------------------------------------------------
    // Process GetStorageTypes
    //-----------------------------------------------------------------

    // Clear the list of prior entries
    if (m_pListStorageTypes != NULL) {
        delete m_pListStorageTypes;
        m_pListStorageTypes = NULL;
    }

    m_pListStorageTypes = new StorageTypeListImpl;

	DIOMEDE_CONSOLE::GetStorageTypesTask taskStorageTypes(m_pListStorageTypes);
	int nResult = HandleTask(&taskStorageTypes, _T("Getting storage types"), _T(""));

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskStorageTypes.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);
        SimpleRedirect::Instance()->EndRedirect();
	    return;
	}

    std::vector<void * > listStorageTypes = m_pListStorageTypes->GetStorageTypeList();

    // If no entries found, output a message and return.
    if ((int)listStorageTypes.size() == 0) {
        PrintStatusMsg(_T("Get storage types successful.  No matches found."));
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    // Run through the results to find the length we need for power consumption strings.
    int nMaxPowerLen = 0;
    int nPowerLen = 0;

    std::vector<std::string> listPowerConsumption;
    int nIndex = 0;

    std::string szTempNumber = _T("");
    std::string szDefaultNumber = _T("0");

    for (nIndex = 0; nIndex < (int)listStorageTypes.size(); nIndex ++) {

        StorageTypeInfoImpl* pStorageType = (StorageTypeInfoImpl*)listStorageTypes[nIndex];
        if (pStorageType == NULL) {
            listPowerConsumption.push_back(_T(""));
            continue;
        }

        StringUtil::FormatNumber(pStorageType->GetPowerPerMB(), szTempNumber, 5, szDefaultNumber);

        // Save the formatted string to use in the final output
        listPowerConsumption.push_back(szTempNumber);
        nPowerLen = (int)szTempNumber.length();
        nMaxPowerLen = (nPowerLen > nMaxPowerLen) ? nPowerLen : nMaxPowerLen;
    }

    // Output the storage types, lining up the power consumption rows....
    for (nIndex = 0; nIndex < (int)listStorageTypes.size(); nIndex ++) {

        StorageTypeInfoImpl* pStorageType = (StorageTypeInfoImpl*)listStorageTypes[nIndex];
        if (pStorageType == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get storage types: NULL storage type."));
            continue;
        }

        //-------------------------------------------------------------
        // Product <product ID>: <product name>
        //-------------------------------------------------------------
        #if 0
            // The following output separates the storage type and power on 2 lines
            cout << left << _T(" Storage ") << pStorageType->GetStorageTypeID() << _T(": ");
            cout <<  pStorageType->GetStorageTypeName() << endl;

            szTempNumber = listPowerConsumption[nIndex];
            cout << _T("     Power: ") << setw(nMaxPowerLen) << right << szTempNumber
                 << _T(" microwatts per MB") << endl;
        #else
            // Single line output
            szTempNumber = listPowerConsumption[nIndex];
            cout << left << _T(" Storage ID ") << pStorageType->GetStorageTypeID() << _T(": ")
                 <<  pStorageType->GetStorageTypeName()
                 << _T(" (") << szTempNumber << _T(" microwatts per MB)") << endl;
        #endif
    }

    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessGetStorageTypesCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to retrieve the list of available storage types.
// Requires:
//      bShowProgress: shows progress if true, no output otherwise
// Returns: true if successful, false otherwise.
bool ConsoleControl::GetStorageTypes(bool bShowProgress)
{
    // Make sure the list has been allocated.
    if (m_pListStorageTypes == NULL) {
        m_pListStorageTypes = new StorageTypeListImpl;
    }
    else {
        // If the list has entries, clear them out.
        std::vector<void * > listStorageTypes = m_pListStorageTypes->GetStorageTypeList();
        if (listStorageTypes.size() > 0) {
            delete m_pListStorageTypes;
            m_pListStorageTypes = NULL;

            m_pListStorageTypes = new StorageTypeListImpl;
        }
    }

    std::string szGetStorageTypesStart = _T("");
    if (bShowProgress) {
        szGetStorageTypesStart = _T("Getting storage types");
    }

    //-----------------------------------------------------------------
    // Process GetStorageTypes
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::GetStorageTypesTask taskStorageTypes(m_pListStorageTypes);
	int nResult = HandleTask(&taskStorageTypes, szGetStorageTypesStart, _T(""), false, true, false);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskStorageTypes.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);
	    return false;
	}

    return true;

} // End GetStorageTypes

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to retrieve the storage type name for the
//          given storage type ID.
// Requires:
//      nStorageTypeID: storage type ID
//      szStorageTypeName: on success, contains the associated storage type name.
// Returns: true if successful, false otherwise.
bool ConsoleControl::GetStorageTypeName(int nStorageTypeID, std::string& szStorageTypeName)
{
    // Probably should assert - this should not be called if there
    // are no storage types...
    if (m_pListStorageTypes == NULL) {
        // Caller can keep the same formatting for both success or failure.
        szStorageTypeName = _format(_T("%d"), nStorageTypeID);
        return false;
    }

    std::vector<void * > listStorageTypes = m_pListStorageTypes->GetStorageTypeList();
    StorageTypeInfoImpl* pStorageType = NULL;

    bool bSuccess = false;

	for (std::vector<void *>::iterator iter = listStorageTypes.begin();
	    iter != listStorageTypes.end(); iter++)
    {
		pStorageType = (StorageTypeInfoImpl*)(*iter);
		if (pStorageType->GetStorageTypeID() == nStorageTypeID) {
		    szStorageTypeName = pStorageType->GetStorageTypeName();
		    bSuccess = true;
		    break;
		}
	}

    return bSuccess;

} // End GetStorageTypeName

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get physical info command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetPhysicalFilesCommand(CmdLine* pCmdLine,
                                                    bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get physical file info"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get physical file info: user not logged into service."));
	    return;
    }

    bCommandFinished = false;
    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeValueArg<std::string>* pPhysicalFileIDArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pPhysicalFileIDArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_PHYSICALFILE_ID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETPHYSICALFILES << endl;
    }

    if ( pFileIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

    bool bGetPhysicalFile = false;
    LONG64 l64PhysicalFileID = 0;

    if (pPhysicalFileIDArg && pPhysicalFileIDArg->getValue().length() > 0) {
        l64PhysicalFileID = atoi64(pPhysicalFileIDArg->getValue().c_str());
        bGetPhysicalFile = true;
    }

	if (pFileIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
	    pTmpArg = pFileIDArg;
	}

	bCommandFinished = ( pFileIDArg->getValue().length() > 0 );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    //-----------------------------------------------------------------
    // Process getphysicalfileinfo
    //-----------------------------------------------------------------
    std::string szGetPhysicalFileInfoStart = _format(_T("Getting physical file info for file %s "),
        szFileID.c_str());

	DIOMEDE_CONSOLE::GetPhysicalFilesTask taskGetPhysicalFiles(m_szSessionToken, l64FileID, l64PhysicalFileID);
	int nResult = HandleTask(&taskGetPhysicalFiles, szGetPhysicalFileInfoStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskGetPhysicalFiles.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get physical file info for file failed for file %s."),
	        szFileID.c_str());
	    return;
	}

    if (bGetPhysicalFile) {
        DisplayPhysicalFileInfoEntry(taskGetPhysicalFiles.GetPhysicalFileInfoResult());
        PrintNewLine();
    }
    else {
        DisplayPhysicalFileInfo(l64FileID, taskGetPhysicalFiles.GetPhysicalFileInfoResults(),
            true);
    }

    std::string szStatusMsg = _format(_T("Get physical file info for file %s successful."),
        szFileID.c_str());
    ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessGetPhysicalFilesCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to display the physical file info for a file.
// Requires:
//      l64FileID: logical file ID associated with the physical file
//      pListPhysicalFileInfo: list of physical files associated with the logical file.
//      bDisplayFileID: display the logical file ID as part of the output.
// Returns: true if successful, false otherwise
bool ConsoleControl::DisplayPhysicalFileInfo(LONG64 l64FileID,
                                             PhysicalFileInfoListImpl* pListPhysicalFileInfo,
                                             bool bDisplayFileID /*false*/)
{
    if (pListPhysicalFileInfo == NULL) {
        return false;
    }

    //-----------------------------------------------------------------
    // Make sure we have the list of storage type names.
    //-----------------------------------------------------------------
    if (m_pListStorageTypes == NULL) {
        m_pListStorageTypes = new StorageTypeListImpl;
    }

    // If the storage types list is empty, get the list now...
    std::vector<void * > listStorageTypes = m_pListStorageTypes->GetStorageTypeList();
    if ( (int)listStorageTypes.size() == 0) {
        GetStorageTypes(false);
    }
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------

    if (bDisplayFileID) {
        #ifdef WIN32
            _tprintf(_T("      Logical file ID: %I64d \n\r"), l64FileID);
        #else
            _tprintf(_T("      Logical file ID: %lld \n\r"), l64FileID);
        #endif
    }

    std::vector<void * > listPhysicalFile = pListPhysicalFileInfo->GetPhysicalFileInfoList();

    int nCopies = (int)listPhysicalFile.size();
    _tprintf(_T("               Copies: %d \n\r"), nCopies);

    // No copies, we're done..
    if (nCopies == 0) {
        return true;
    }

    PrintNewLine();

    std::string szTempNumber = _T("");
    std::string szDefaultNumber = _T("0");
    std::string szStorageTypeName = _T("");
    std::string szFormattedDate = _T("");

    for (int nIndex = 0; nIndex < (int)listPhysicalFile.size(); nIndex ++) {

        PhysicalFileInfoImpl* pPhysicalFileInfoEntry = (PhysicalFileInfoImpl*)listPhysicalFile[nIndex];
        if (pPhysicalFileInfoEntry == NULL) {
            continue;
        }

        DisplayPhysicalFileInfoEntry(pPhysicalFileInfoEntry, (nIndex + 1) );

        #if 0
        _tprintf(_T("               Copy #: %d \n\r"), (nIndex + 1));

        #ifdef WIN32
            _tprintf(_T("     Physical file ID: %I64d \n\r"), pPhysicalFileInfoEntry->GetPhysicalFileID());
        #else
            _tprintf(_T("     Physical file ID: %lld \n\r"), pPhysicalFileInfoEntry->GetPhysicalFileID());
        #endif

        // Find the storage type name for the ID
        szStorageTypeName = _T("");
        GetStorageTypeName(pPhysicalFileInfoEntry->GetStorageTypeID(), szStorageTypeName);

        _tprintf(_T("         Storage type: %s (%d) \n\r"), szStorageTypeName.c_str(),
            pPhysicalFileInfoEntry->GetStorageTypeID());

        szTempNumber = _T("");
        StringUtil::FormatNumber(pPhysicalFileInfoEntry->GetPower(), szTempNumber, 5, szDefaultNumber);
        _tprintf(_T("    Power consumption: %s microwatts\n\r"), szTempNumber.c_str());

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pPhysicalFileInfoEntry->GetRequestedDate(), szFormattedDate);

        if (szFormattedDate.length() > 0) {
            _tprintf(_T("       Requested date: %s \n\r"), szFormattedDate.c_str());
        }
        else {
            _tprintf(_T("       Requested date: none \n\r"));
        }

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pPhysicalFileInfoEntry->GetCreatedDate(), szFormattedDate);
        _tprintf(_T("         Created date: %s \n\r"), szFormattedDate.c_str());

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pPhysicalFileInfoEntry->GetExercisedDate(), szFormattedDate);

        if (szFormattedDate.length() > 0) {
            _tprintf(_T("       Exercised date: %s \n\r"), szFormattedDate.c_str());
        }
        else {
            _tprintf(_T("       Exercised date: none \n\r"));
        }

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pPhysicalFileInfoEntry->GetExpirationDate(), szFormattedDate);

        if (szFormattedDate.length() > 0) {
            _tprintf(_T("         Expires date: %s \n\r"), szFormattedDate.c_str());
        }
        else {
            _tprintf(_T("         Expires date: none \n\r"));
        }
        #endif

        PrintNewLine();
    }

    return true;

} // End DisplayPhysicalFileInfo

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to display the physical file info entry for a file.
// Requires:
//      pPhysicalFileInfoEntry: a physical file info associated with the logical file.
//      nCopy: copy number
// Returns: nothing
void ConsoleControl::DisplayPhysicalFileInfoEntry(PhysicalFileInfoImpl* pPhysicalFileInfoEntry,
    int nCopy /*0*/)
{

    // If the storage types list is empty, get the list now...
    std::vector<void * > listStorageTypes = m_pListStorageTypes->GetStorageTypeList();
    if ( (int)listStorageTypes.size() == 0) {
        GetStorageTypes(false);
    }

    std::string szTempNumber = _T("");
    std::string szDefaultNumber = _T("0");
    std::string szStorageTypeName = _T("");
    std::string szFormattedDate = _T("");

    // When we get a single physical file info, the copy number is unknown....
    if (nCopy > 0) {
        _tprintf(_T("               Copy #: %d \n\r"), nCopy);
    }

    #ifdef WIN32
        _tprintf(_T("     Physical file ID: %I64d \n\r"), pPhysicalFileInfoEntry->GetPhysicalFileID());
    #else
        _tprintf(_T("     Physical file ID: %lld \n\r"), pPhysicalFileInfoEntry->GetPhysicalFileID());
    #endif

    // Find the storage type name for the ID
    szStorageTypeName = _T("");
    GetStorageTypeName(pPhysicalFileInfoEntry->GetStorageTypeID(), szStorageTypeName);

    _tprintf(_T("         Storage type: %s (%d) \n\r"), szStorageTypeName.c_str(),
        pPhysicalFileInfoEntry->GetStorageTypeID());

    szTempNumber = _T("");
    StringUtil::FormatNumber(pPhysicalFileInfoEntry->GetPower(), szTempNumber, 5, szDefaultNumber);
    _tprintf(_T("    Power consumption: %s microwatts\n\r"), szTempNumber.c_str());

    szFormattedDate = _T("");
    StringUtil::FormatDateAndTime(pPhysicalFileInfoEntry->GetRequestedDate(), szFormattedDate);

    if (szFormattedDate.length() > 0) {
        _tprintf(_T("       Requested date: %s \n\r"), szFormattedDate.c_str());
    }
    else {
        _tprintf(_T("       Requested date: none \n\r"));
    }

    szFormattedDate = _T("");
    StringUtil::FormatDateAndTime(pPhysicalFileInfoEntry->GetCreatedDate(), szFormattedDate);
    _tprintf(_T("         Created date: %s \n\r"), szFormattedDate.c_str());

    szFormattedDate = _T("");
    StringUtil::FormatDateAndTime(pPhysicalFileInfoEntry->GetExercisedDate(), szFormattedDate);

    if (szFormattedDate.length() > 0) {
        _tprintf(_T("       Exercised date: %s \n\r"), szFormattedDate.c_str());
    }
    else {
        _tprintf(_T("       Exercised date: none \n\r"));
    }

    szFormattedDate = _T("");
    StringUtil::FormatDateAndTime(pPhysicalFileInfoEntry->GetExpirationDate(), szFormattedDate);

    if (szFormattedDate.length() > 0) {
        _tprintf(_T("         Expires date: %s \n\r"), szFormattedDate.c_str());
    }
    else {
        _tprintf(_T("         Expires date: none \n\r"));
    }

} // End DisplayPhysicalFileInfo

///////////////////////////////////////////////////////////////////////
// Purpose: Process the create replication policy command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessCreateReplicationPolicyCommand(CmdLine* pCmdLine,
                                                           bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot create replication policy"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Create replication policy: user not logged into service."));
	    return;
    }

    bool bArgIsSet = false;

    ReplicationPolicyInfoImpl* pReplicationPolicyInfo = new ReplicationPolicyInfoImpl;
    bool bSuccess = SetupReplicationPolicyInfo(pCmdLine, bCommandFinished,
                                               pReplicationPolicyInfo, bArgIsSet);

    if (bSuccess == false) {
        if (pReplicationPolicyInfo) {
            delete pReplicationPolicyInfo;
            pReplicationPolicyInfo = NULL;
        }
        return;
    }

    //-----------------------------------------------------------------
    // Process createreplicationpolicy
    //-----------------------------------------------------------------
    std::string szCreateReplicationPolicyStart = _format(_T("Creating replication policy"));

	DIOMEDE_CONSOLE::CreateReplicationPolicyTask taskCreateReplicationPolicy(m_szSessionToken,
	                                                                         pReplicationPolicyInfo);
	int nResult = HandleTask(&taskCreateReplicationPolicy, szCreateReplicationPolicyStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Replication policy ID: %d"),
	        pReplicationPolicyInfo->GetReplicationPolicyID());
        PrintStatusMsg(szStatusMsg);
        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskCreateReplicationPolicy.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Create replication policy failed."));
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pReplicationPolicyInfo) {
        delete pReplicationPolicyInfo;
        pReplicationPolicyInfo = NULL;
    }

} // End ProcessCreateReplicationPolicyCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to CreateReplicationPolicy and
//          EditReplicationPolicy to setup the replication policy info object.
// Requires:
//      pCmdLine: current command line
//      pReplicationPolicyInfo: will contain the replication policy info from the
//                     arguments.
//      bArgIsSet: indicates whether any arguments have been set.
// Returns: true if successful, false otherwise
bool ConsoleControl::SetupReplicationPolicyInfo(CmdLine* pCmdLine, bool& bCommandFinished,
	                                ReplicationPolicyInfoImpl* pReplicationPolicyInfo,
	                                bool& bArgIsSet)
{
    DiomedeValueArg<std::string>* pDefOnlineArg = NULL;
    DiomedeValueArg<std::string>* pDefNearlineArg = NULL;
    DiomedeValueArg<std::string>* pDefOfflineArg = NULL;
    DiomedeValueArg<std::string>* pTriggerHoursArg = NULL;
    DiomedeValueArg<std::string>* pTriggerOnlineArg = NULL;
    DiomedeValueArg<std::string>* pTriggerNearlineArg = NULL;
    DiomedeValueArg<std::string>* pTriggerOfflineArg = NULL;
    DiomedeValueArg<std::string>* pExpiresHoursArg = NULL;

    try {
        pDefOnlineArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_DEF_ONLINE);
        pDefNearlineArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_DEF_NEARLINE);
        pDefOfflineArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_DEF_OFFLINE);
        pTriggerHoursArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_TRIGGER_HOURS);
        pTriggerOnlineArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_TRIGGER_ONLINE);
        pTriggerNearlineArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_TRIGGER_NEARLINE);
        pTriggerOfflineArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_TRIGGER_OFFLINE);
        pExpiresHoursArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_EXPIRE_HOURS);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    bCommandFinished = true;

    // The remaining commands are optionally...
    int nValue = 0;
    if (pDefOnlineArg && pDefOnlineArg->isSet()) {
        nValue = atoi(pDefOnlineArg->getValue().c_str());
        pReplicationPolicyInfo->SetDefaultOnline(nValue);
        bArgIsSet = true;
    }
    if (pDefNearlineArg && pDefNearlineArg->isSet()) {
        nValue = atoi(pDefNearlineArg->getValue().c_str());
        pReplicationPolicyInfo->SetDefaultNearline(nValue);
        bArgIsSet = true;
    }
    if (pDefOfflineArg && pDefOfflineArg->isSet()) {
        nValue = atoi(pDefOfflineArg->getValue().c_str());
        pReplicationPolicyInfo->SetDefaultOffline(nValue);
        bArgIsSet = true;
    }
    if (pTriggerHoursArg && pTriggerHoursArg->isSet()) {
        nValue = atoi(pTriggerHoursArg->getValue().c_str());
        pReplicationPolicyInfo->SetLastAccessTriggerHours(nValue);
        bArgIsSet = true;
    }
    if (pTriggerOnlineArg && pTriggerOnlineArg->isSet()) {
        nValue = atoi(pTriggerOnlineArg->getValue().c_str());
        pReplicationPolicyInfo->SetLastAccessTriggeOnline(nValue);
        bArgIsSet = true;
    }
    if (pTriggerNearlineArg && pTriggerNearlineArg->isSet()) {
        nValue = atoi(pTriggerNearlineArg->getValue().c_str());
        pReplicationPolicyInfo->SetLastAccessTriggeNearline(nValue);
        bArgIsSet = true;
    }
    if (pTriggerOfflineArg && pTriggerOfflineArg->isSet()) {
        nValue = atoi(pTriggerOfflineArg->getValue().c_str());
        pReplicationPolicyInfo->SetLastAccessTriggeOffline(nValue);
        bArgIsSet = true;
    }

    // Default for the expires hours is -1 -> TBD what the service
    // actually needs for this...
    if (pExpiresHoursArg && pExpiresHoursArg->isSet()) {
        nValue = atoi(pExpiresHoursArg->getValue().c_str());
        pReplicationPolicyInfo->SetExpiresHours(nValue);
        bArgIsSet = true;
    }

    return true;

} // End SetupReplicationPolicyInfo

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to replication policy commands that required
//          a replication policy ID argument.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: set to true if the command is finished, false
//                        otherwise.
//      nReplicationPolicyID: will contain the replication policy ID.
// Returns: true if successful, false otherwise
bool ConsoleControl::SetupReplicationPolicyID(CmdLine* pCmdLine,
                                              bool& bCommandFinished,
	                                          int& nReplicationPolicyID)
{
    bCommandFinished = false;
    DiomedeUnlabeledValueArg<std::string>* pReplicationPolicyIDArg = NULL;

    try {
        pReplicationPolicyIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_REPLICATION_POLICY_ID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << pCmdLine->getCommandName() << endl;
    }

    if (pReplicationPolicyIDArg == NULL) {
        bCommandFinished = true;
        return false;
    }

    Arg *pTmpArg = NULL;

    if (pReplicationPolicyIDArg->getValue().length() == 0) {
        m_szCommandPrompt = _T("Policy ID: ");
        pTmpArg = pReplicationPolicyIDArg;
    }

    bCommandFinished = ( pReplicationPolicyIDArg->getValue().length() > 0 );

    if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

        return false;
    }

    bCommandFinished = true;
    nReplicationPolicyID = atoi(pReplicationPolicyIDArg->getValue().c_str());

    return true;

} // End SetupReplicationPolicyID

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get replication policies command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetReplicationPoliciesCommand(CmdLine* pCmdLine,
                                                           bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get replication policies"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get replication policies: user not logged into service."));
	    return;
    }

    DiomedeValueArg<std::string>* pOutputArg = NULL;
    try {
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SEARCHFILESTOTALLOG << endl;
    }

    bCommandFinished = true;

    //-----------------------------------------------------------------
    // Process getreplicationpolicies
    //-----------------------------------------------------------------
    std::string szGetReplicationPoliciesStart = _format(_T("Getting all replication policies"));

	DIOMEDE_CONSOLE::GetReplicationPoliciesTask taskGetReplicationPolicies(m_szSessionToken);
	int nResult = HandleTask(&taskGetReplicationPolicies, szGetReplicationPoliciesStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskGetReplicationPolicies.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get replication policies failed."));
	    return;
	}

    ClientLog(UI_COMP, LOG_STATUS, false, _T("Get replication policies successful."));

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process results.
    //-----------------------------------------------------------------
    ReplicationPolicyInfoListImpl* pListReplicationPolicies =
        taskGetReplicationPolicies.GetReplicationPoliciesResults();

    std::vector<void *> listReplicationPolicies =
        pListReplicationPolicies->GetReplicationPolicyInfoList();

    // Edge case to ensure we did get something back.
    if (listReplicationPolicies.size() == 0) {
        PrintStatusMsg("Get replication policies: no policies returned.");
        ClientLog(UI_COMP, LOG_ERROR, false,_T("Get replication policies: no policies returned."));
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    bool bShowHeader = true;
    for (int nRepPolicyIndex = 0; nRepPolicyIndex < (int)listReplicationPolicies.size();
        nRepPolicyIndex ++) {

        ReplicationPolicyInfoImpl* pReplicationPolicyInfo =
            (ReplicationPolicyInfoImpl*)listReplicationPolicies[nRepPolicyIndex];

        DisplayReplicationPolicy(pReplicationPolicyInfo, bShowHeader);
        bShowHeader = false;
    }

    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessGetReplicationPoliciesCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to display the search files total response in
//          a short format.
// Requires:
//      pReplicationPolicyInfo: reference replication policy info
//      bShowHeader: displays the header - subsequent calls to this
//                   helper should set this value to false.
// Returns: true if successful, false otherwise
bool ConsoleControl::DisplayReplicationPolicy(ReplicationPolicyInfoImpl* pReplicationPolicyInfo,
                                              bool bShowHeader /*true*/ )
{
    if (pReplicationPolicyInfo == NULL) {
        ClientLog(UI_COMP, LOG_ERROR, false,
            _T("Replication policy: NULL replicaiton information object."));
        return false;
    }

    // Using streams here to provide easier column setup.
    if (bShowHeader) {
        cout << setw(4) << right << _T("RPID")
            << setw(7) << _T("On") << setw(7) << _T("Near") << setw(7) << _T("Off")
            << setw(14) << _T("THours")
            << setw(7) << _T("TOn") << setw(7) << _T("TNear") << setw(7) << _T("TOff")
            << setw(19) << _T("ExpireHours")
            << endl;
    }

    std::string szNever = _T("never");

    cout << setw(4) << right << pReplicationPolicyInfo->GetReplicationPolicyID()
         << setw(7) << pReplicationPolicyInfo->GetDefaultOnline()
         << setw(7) << pReplicationPolicyInfo->GetDefaultNearline()
         << setw(7) << pReplicationPolicyInfo->GetDefaultOffline()
         << setw(14) << pReplicationPolicyInfo->GetLastAccessTriggerHours()
         << setw(7) << pReplicationPolicyInfo->GetLastAccessTriggeOnline()
         << setw(7) << pReplicationPolicyInfo->GetLastAccessTriggeNearline()
         << setw(7) << pReplicationPolicyInfo->GetLastAccessTriggeOffline();

    if (  pReplicationPolicyInfo->GetExpiresHours() <= 0) {
        cout << setw(19) << szNever << endl;
    }
    else {
        cout << setw(19) << pReplicationPolicyInfo->GetExpiresHours() << endl;
    }

    return true;

} // End DisplayReplicationPolicy

///////////////////////////////////////////////////////////////////////
// Purpose: Process the edit replication policy command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessEditReplicationPolicyCommand(CmdLine* pCmdLine,
                                                           bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot edit replication policy"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Edit replication policy: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    int nReplicationPolicyID = 0;
    bool bSuccess = SetupReplicationPolicyID(pCmdLine, bCommandFinished,
                                             nReplicationPolicyID);
    if (bSuccess == false) {
        return;
    }

    bool bArgIsSet = false;
    ReplicationPolicyInfoImpl* pReplicationPolicyInfo = new ReplicationPolicyInfoImpl;

    bSuccess = SetupReplicationPolicyInfo(pCmdLine, bCommandFinished,
                                          pReplicationPolicyInfo, bArgIsSet);

    if (bSuccess == false) {
        return;
    }

    // Add in the replication policy ID...
    pReplicationPolicyInfo->SetReplicationPolicyID(nReplicationPolicyID);

    //-----------------------------------------------------------------
    // Process editereplicationpolicy
    //-----------------------------------------------------------------
    std::string szEditReplicationPolicyStart = _format(_T("Edit replication policy"));

	DIOMEDE_CONSOLE::EditReplicationPolicyTask taskEditReplicationPolicy(m_szSessionToken,
	                                                                     pReplicationPolicyInfo);
	int nResult = HandleTask(&taskEditReplicationPolicy, szEditReplicationPolicyStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Edit replication policy successful."));
        PrintStatusMsg(szStatusMsg);
        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskEditReplicationPolicy.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Edit replication policy failed."));
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessEditReplicationPolicyCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the delete replication policy command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessDeleteReplicationPolicyCommand(CmdLine* pCmdLine,
                                                           bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot delete replication policy"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Delete replication policy: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    int nReplicationPolicyID = 0;
    bool bSuccess = SetupReplicationPolicyID(pCmdLine, bCommandFinished,
                                             nReplicationPolicyID);
    if (bSuccess == false) {
        return;
    }

    //-----------------------------------------------------------------
    // Process deletereplicationpolicy
    //-----------------------------------------------------------------
    std::string szDeleteReplicationPolicyStart = _format(_T("Delete replication policy"));

	DIOMEDE_CONSOLE::DeleteReplicationPolicyTask taskDeleteReplicationPolicy(m_szSessionToken,
	                                                                         nReplicationPolicyID);
	int nResult = HandleTask(&taskDeleteReplicationPolicy, szDeleteReplicationPolicyStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Delete replication policy successful."));
        PrintStatusMsg(szStatusMsg);
        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskDeleteReplicationPolicy.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Delete replication policy failed."));
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessDeleteReplicationPolicyCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the set replication policy command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSetReplicationPolicyCommand(CmdLine* pCmdLine,
                                                         bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot set replication policy"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Set replication policy: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    DiomedeUnlabeledValueArg<std::string>* pFileIDArg = NULL;
    DiomedeUnlabeledValueArg<std::string>* pReplicationPolicyIDArg = NULL;

    try {
        pFileIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pReplicationPolicyIDArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_REPLICATION_POLICY_ID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SETREPLICATIONPOLICY << endl;
    }

    if ( ( pFileIDArg == NULL) || (pReplicationPolicyIDArg == NULL) ) {
        bCommandFinished = true;
        return;
    }

    Arg *pTmpArg = NULL;

	if (pFileIDArg->getValue().length() == 0) {
	    m_szCommandPrompt = _T("Filename, ID, or hash: ");
	    pTmpArg = pFileIDArg;
	}
    else if (pReplicationPolicyIDArg->getValue().length() == 0) {
        m_szCommandPrompt = _T("Policy ID: ");
        pTmpArg = pReplicationPolicyIDArg;
    }

	bCommandFinished =  ( ( pFileIDArg->getValue().length() > 0 ) &&
	                      ( pReplicationPolicyIDArg->getValue().length() > 0 ) );

	if (bCommandFinished == false) {

        if (pTmpArg->getRepromptCount() == 0) {
            pTmpArg->incrementRepromptCount();
        }
        else if (pTmpArg->getRepromptCount() < 2) {
            pTmpArg->incrementRepromptCount();
        }
        else {
            // We've prompted them already - alert them that they need
            // a file...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
        }

	    return;
	}

    bCommandFinished = true;
    std::string szFileID = pFileIDArg->getValue();

    DiomedeStorageService storageService;
    LONG64 l64FileID = 0;

    if (false == GetFileID(&storageService, szFileID, l64FileID, pCmdLine->getCommandName())) {
        return;
    }

    int nReplicationPolicyID = atoi(pReplicationPolicyIDArg->getValue().c_str());

    //-----------------------------------------------------------------
    // Process setreplicationpolicy
    //-----------------------------------------------------------------
    std::string szSetReplicationPolicyStart = _format(_T("Set replication policy"));

	DIOMEDE_CONSOLE::SetReplicationPolicyTask taskSetReplicationPolicy(m_szSessionToken,
	                                                                   l64FileID, nReplicationPolicyID);
	int nResult = HandleTask(&taskSetReplicationPolicy, szSetReplicationPolicyStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Set replication policy successful."));
        PrintStatusMsg(szStatusMsg);
        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskSetReplicationPolicy.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Set replication policy failed."));
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessSetReplicationPolicyCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the set default replication policy command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSetDefaultReplicationPolicyCommand(CmdLine* pCmdLine,
                                                           bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot set default replication policy"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Set default replication policy: user not logged into service."));
	    return;
    }

    bCommandFinished = false;

    int nReplicationPolicyID = 0;
    bool bSuccess = SetupReplicationPolicyID(pCmdLine, bCommandFinished,
                                             nReplicationPolicyID);
    if (bSuccess == false) {
        return;
    }

    //-----------------------------------------------------------------
    // Process setdefaultreplicationpolicy
    //-----------------------------------------------------------------
    std::string szSetDefaultReplicationPolicyStart = _format(_T("Set default replication policy"));

	DIOMEDE_CONSOLE::SetDefaultReplicationPolicyTask taskSetDefaultReplicationPolicy(m_szSessionToken,
	                                                                                 nReplicationPolicyID);
	int nResult = HandleTask(&taskSetDefaultReplicationPolicy, szSetDefaultReplicationPolicyStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    std::string szStatusMsg = _format(_T("Set default replication policy successful."));
        PrintStatusMsg(szStatusMsg);
        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	}
	else {
	    std::string szErrorMsg = taskSetDefaultReplicationPolicy.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Set default replication policy failed."));
	}

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------

} // End ProcessSetDefaultReplicationPolicyCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get default replication policy command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetDefaultReplicationPolicyCommand(CmdLine* pCmdLine,
                                                           bool& bCommandFinished)
{
    // User must be logged in to edit metadata for a file.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get default replication policy"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get default replication policy: user not logged into service."));
	    return;
    }

    bCommandFinished = true;
    ReplicationPolicyInfoImpl* pReplicationPolicyInfo = new ReplicationPolicyInfoImpl;

    //-----------------------------------------------------------------
    // Process getdefaultreplicationpolicy
    //-----------------------------------------------------------------
    std::string szGetDefaultReplicationPolicyStart = _format(_T("Get default replication policy"));

	DIOMEDE_CONSOLE::GetDefaultReplicationPolicyTask taskGetDefaultReplicationPolicy(m_szSessionToken,
	                                                                                 pReplicationPolicyInfo);
	int nResult = HandleTask(&taskGetDefaultReplicationPolicy, szGetDefaultReplicationPolicyStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskGetDefaultReplicationPolicy.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Get default replication policy failed."));
	}

    ClientLog(UI_COMP, LOG_STATUS, false, _T("Get default replication policy successful."));
    DisplayReplicationPolicy(pReplicationPolicyInfo, true);

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pReplicationPolicyInfo) {
        delete pReplicationPolicyInfo;
        pReplicationPolicyInfo = NULL;
    }

} // End ProcessGetDefaultReplicationPolicyCommand


///////////////////////////////////////////////////////////////////////
// Purpose: Process the get all products command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetAllProductsCommand(CmdLine* pCmdLine,
                                                   bool& bCommandFinished)
{
    // The user is not required to login to access this command.
    bCommandFinished = true;
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETALLPRODUCTS << endl;
    }

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process GetAllProducts
    //-----------------------------------------------------------------

    // New line here is fine - passing "false" to HandleTask....
    PrintNewLine();

	DIOMEDE_CONSOLE::GetAllProductsTask taskGetAllProducts;
	int nResult = HandleTask(&taskGetAllProducts, _T("Getting all products"), _T(""), false, false);

    //-----------------------------------------------------------------
    // Check GetAllProducts results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskGetAllProducts.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);
        SimpleRedirect::Instance()->EndRedirect();
	    return;
	}

    //-----------------------------------------------------------------
    // Process GetAllContracts
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::GetAllContractsTask taskGetAllContracts;
	nResult = HandleTask(&taskGetAllContracts, _T("Getting all contracts"), _T(""), false, false);

    //-----------------------------------------------------------------
    // Check GetAllContracts results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    std::string szErrorMsg = taskGetAllContracts.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);
        SimpleRedirect::Instance()->EndRedirect();
	    return;
	}

    //-----------------------------------------------------------------
    // Display results from products, supports, and contracts
    //-----------------------------------------------------------------

    // Since we've turned off the task new lines, add one now...
    PrintNewLine();

    ProductListImpl* pListProducts = taskGetAllProducts.GetProductResults();
	ContractListImpl* pListContracts = taskGetAllContracts.GetContractResults();

    //-----------------------------------------------------------------
    // Display products
    //-----------------------------------------------------------------
    ClientLog(UI_COMP, LOG_STATUS, false,_T("Get all products successful."));

    std::vector<void * > listProducts = pListProducts->GetProductList();

    // Edge case to ensure we did get something back.
    if (listProducts.size() == 0) {
        std::string szStatusMsg = _T("Get all products: no rates returned.");
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_ERROR, false, _T("%s"), szStatusMsg.c_str());
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    //-----------------------------------------------------------------
    // locale facet for formatting currency
    //-----------------------------------------------------------------
    std::locale localEnglish(DiomedeMoneyPunct::GetLocaleString().c_str());
    std::locale locDiomede(localEnglish, new DiomedeMoneyPunct(4));
    cout.imbue(locDiomede);

    //-----------------------------------------------------------------
    // Make sure we have the list of storage type names.
    //-----------------------------------------------------------------

    if (m_pListStorageTypes == NULL) {
        m_pListStorageTypes = new StorageTypeListImpl;
    }

    // If the storage types list is empty, get the list now...
    std::vector<void * > listStorageTypes = m_pListStorageTypes->GetStorageTypeList();
    if ( (int)listStorageTypes.size() == 0) {
        GetStorageTypes(false);
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------

    int nProdIndex = 0;
    int nCompIndex = 0;
    int nBandwidthIndex = 0;

    std::string szTemp = _T("");
    std::string szStorageTypeName = _T("");

    /*
    cout << _T("12345678901234567890123456789012345678901234567890123456789012345678901234567890") << endl;
    */

    for (nProdIndex = 0; nProdIndex < (int)listProducts.size(); nProdIndex ++) {

        ProductImpl* pProduct = (ProductImpl*)listProducts[nProdIndex];
        if (pProduct == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get all products: NULL rate set."));
            continue;
        }

        //-------------------------------------------------------------
        // Product <product ID>: <product name>
        //-------------------------------------------------------------
        cout << left << _T(" Product ") << pProduct->GetProductID() << _T(": ");
        cout <<  pProduct->GetName() << endl;

        //-------------------------------------------------------------
        // Minimum Monthly Fee
        // Support Fee
        //-------------------------------------------------------------
        cout << setw(4) << left << _T("") << setw(55) <<  _T("Minimum Monthly Fee ");

        double dwMinMonthlyFee = (double)pProduct->GetMinMonthlyFee();
        cout << setw(20) << std::showbase << Money(dwMinMonthlyFee) << endl;

        cout << setw(4) << _T("") << setw(55) <<  _T("Support Fee ");

        double dwSupportFee = (double)pProduct->GetSupportFee();
        cout << setw(20) << std::showbase << Money(dwSupportFee)<< endl << endl;

        ComponentListImpl* pProductComponents = pProduct->GetComponentList();

        if (pProductComponents == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get all products: NULL list of rate components."));
            continue;
        }

        //-------------------------------------------------------------
        // Each product component:
        // <storage type> <Meter Type>                  <rate per GB>
        //-------------------------------------------------------------
        std::vector<void * > listProductComponents = pProductComponents->GetComponentList();
        std::vector<int> listBandwidthEntries;

        for (nCompIndex = 0; nCompIndex < (int)listProductComponents.size(); nCompIndex ++) {
            ComponentImpl* pProductComponent = (ComponentImpl*)listProductComponents[nCompIndex];

            if (pProductComponent == NULL) {
                ClientLog(UI_COMP, LOG_ERROR, false,_T("Get all products: NULL rate component."));
                continue;
            }

            // We want to show the bandwith entries separate - so we'll skip them for now and
            // add them at end of this loop.
            if (pProductComponent->GetMeterType().find("Bandwidth") != (size_t)-1 ) {
                listBandwidthEntries.push_back(nCompIndex);
                continue;
            }

            szStorageTypeName = _T("");
            GetStorageTypeName(pProductComponent->GetStorageTypeID(), szStorageTypeName);

            szTemp = _format(_T("%s %s"), szStorageTypeName.c_str(),
                pProductComponent->GetMeterType().c_str());

            cout << setw(4) << left << _T("") << setw(55) << szTemp;

            // Rate should be sent as a double - to format properly at this point,
            // convert to a double and then back to our currency amount.
            double dwRatePerGB = (double)pProductComponent->GetRatePerGB();
            cout << setw(20) << std::showbase << Money(dwRatePerGB) << endl;
        }

        // Now show the bandwidth entries.
        if (listBandwidthEntries.size() > 0) {
            PrintNewLine();
            for (nBandwidthIndex = 0; nBandwidthIndex < (int)listBandwidthEntries.size(); nBandwidthIndex ++) {
                nCompIndex = listBandwidthEntries[nBandwidthIndex];
                ComponentImpl* pProductComponent = (ComponentImpl*)listProductComponents[nCompIndex];

                szStorageTypeName = _T("");
                GetStorageTypeName(pProductComponent->GetStorageTypeID(), szStorageTypeName);

                szTemp = _format(_T("%s %s"), szStorageTypeName.c_str(),
                    pProductComponent->GetMeterType().c_str());

                cout << setw(4) << left << _T("") << setw(55) << szTemp;

                // Rate should be sent as a double - to format properly at this point,
                // convert to a double and then back to our currency amount.
                double dwRatePerGB = (double)pProductComponent->GetRatePerGB();
                cout << setw(20) << std::showbase << Money(dwRatePerGB) << endl;
            }
        }
    }

    //-----------------------------------------------------------------
    // HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK
    //-----------------------------------------------------------------
    PrintNewLine();
    _tprintf(_T("    Nearline Storage Replication Read                      $ 0.0100 \n\r"));
    _tprintf(_T("    Nearline Storage Replication Read Request              $ 0.9500 \n\r"));
    _tprintf(_T("    Offline Storage Replication Read                       $ 0.0100 \n\r"));
    _tprintf(_T("    Offline Storage Replication Read Request               $ 9.9500 \n\r"));

    //-----------------------------------------------------------------
    // Display contracts
    //-----------------------------------------------------------------
    ClientLog(UI_COMP, LOG_STATUS, false,_T("Get all contracts successful."));

    std::vector<void * > listContracts = pListContracts->GetContractList();

    // Edge case to ensure we did get something back.
    if (listContracts.size() == 0) {
        std::string szStatusMsg = _T("Get all contracts: no contracts returned.");
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_ERROR, false, _T("%s"), szStatusMsg.c_str());
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    std::string szCommittedGB = _T("");
    std::string szStorage = _T("");

    int nContractIndex = 0;
    nCompIndex = 0;

    cout << endl;

    if ( (int)listContracts.size() > 0) {
        cout << left << _T(" Contracts: ") << endl;
    }

    for (nContractIndex = 0; nContractIndex < (int)listContracts.size(); nContractIndex ++) {
        szTemp = _T("");
        szCommittedGB = _T("");
        szStorage = _T("");

        ContractImpl* pContract = (ContractImpl*)listContracts[nContractIndex];

        if (pContract == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get all contracts: NULL contract."));
            continue;
        }

        //-------------------------------------------------------------
        // Each contract:
        // <zontract ID>: <contract term>-months, <committed GB> <storage type> <meter type>    <rate/term>
        //-------------------------------------------------------------
        ContractComponentListImpl* pListContractComponents = pContract->GetContractComponentList();
        if (pListContractComponents == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get all contracts: NULL contract components."));
            continue;
        }

        std::vector<void * > listContractComponents = pListContractComponents->GetContractComponentList();
        int nNumComponents = (int)listContractComponents.size();

        for (nCompIndex = 0; nCompIndex < nNumComponents; nCompIndex++) {

            ContractComponentImpl* pContractComponent = (ContractComponentImpl*)listContractComponents[nCompIndex];
            if (pContractComponent == NULL) {
                ClientLog(UI_COMP, LOG_ERROR, false,_T("Get all contracts: NULL list of contract components."));
                continue;
            }

            // No matter how many contract components we have, the contract ID and term
            // is display just once.
            if (nCompIndex == 0) {
                szTemp = _format(_T("%d: "), pContract->GetContractID());
                cout << setw(4) << left << _T("") << right << szTemp;

                int nTerm = pContract->GetTerm();
                if (nTerm < 10) {
                    szTemp = _format(_T(" %d-month, "), nTerm);
                }
                else {
                    szTemp = _format(_T("%d-months, "), nTerm);
                }

                cout << setw(11) << left << szTemp;

                if (nNumComponents > 1) {
                    // Rate should be sent as a double - to format properly at this point,
                    // convert to a double and then back to our currency amount.
                    double dwRatePerTerm = (double)pContract->GetRatePerTerm();

                    cout << setw(38) << _T("") << setw(20) << std::showbase
                         << Money(dwRatePerTerm) << endl;
                }
            }

            // If we have just one contract component, we'll display it all on one
            // line.  Otherwise, each component is displayed on its own line.
            // The term is display once - on the same line as the contract ID
            // (see above).
            StringUtil::FormatNumber(pContractComponent->GetCommittedGB(), szCommittedGB);
            szCommittedGB = szCommittedGB + _T(" GB ");

            szStorageTypeName = _T("");
            GetStorageTypeName(pContractComponent->GetStorageTypeID(), szStorageTypeName);

            szStorage = _format(_T("%s %s"), szStorageTypeName.c_str(),
                pContractComponent->GetMeterType().c_str());

            if (nNumComponents == 1) {
                cout << setw(11) << szCommittedGB;
                cout << setw(30) << szStorage;

                // Rate should be sent as a double - to format properly at this point,
                // convert to a double and then back to our currency amount.
                double dwRatePerTerm = (double)pContract->GetRatePerTerm();

                // TBD: we'll receive the rate as an int with possibly a
                // precision - to mimic that, the rate is multiplied by
                // 10000 (assuming 4 digit precision

                cout << setw(20) << std::showbase << Money(dwRatePerTerm) << endl;
            }
            else {
                cout << setw(11) << _T("") << setw(10) << szCommittedGB;
                cout << setw(30) << szStorage << endl;
            }
        }
    }

    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessGetAllProductsCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the purchase product command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessPurchaseProductCommand(CmdLine* pCmdLine,
                                                   bool& bCommandFinished)
{
    // User must be logged in to purchase a product.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot purchase product"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Purchase product: you must be logged into the Diomede service."));
	    return;
    }

    // Get the list of arguments - the list should contain one or more product IDs
    bCommandFinished = false;
    DiomedeUnlabeledMultiArg<std::string>* pRateIDArg = NULL;

    try {
        pRateIDArg = (DiomedeUnlabeledMultiArg<std::string>*)pCmdLine->getArg(ARG_PRODID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_PURCHASEPRODUCT << endl;
    }

    if (pRateIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

    //-----------------------------------------------------------------
    // Check for product IDs - TBD validate this input against a
    // known set of product IDs.
    //-----------------------------------------------------------------
	std::vector<std::string> listRateIDs = pRateIDArg->getValue();

	if (listRateIDs.size() == 0) {

	    if (pRateIDArg->getRepromptCount() == 0) {
	        m_szCommandPrompt = _T("Product(s): ");
	        pRateIDArg->incrementRepromptCount();
	    }
	    else if (pRateIDArg->getRepromptCount() < 2) {
	        pRateIDArg->incrementRepromptCount();
	    }
	    else {
	        // We've prompted them already - alert them that they need
	        // a product ID...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
	    }

	    return;
	}

    bCommandFinished = true;
    int nResult = 0;

    PurchaseProduct(listRateIDs, nResult);

} // End ProcessPurchaseProductCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper to PurchaseProduct and Subscribe commands - handles
//          sending the requested product IDs to the service.
// Requires:
//      listRateIDs: list of rate or product IDs.
//      nResult: result of service call.
// Returns: true if successful, false otherwise
bool ConsoleControl::PurchaseProduct(const std::vector<std::string>& listRateIDs, int& nResult)
{
    bool bSuccess = true;

    std::string szProductID = _T("");
    std::string szPurchaseProductStart = _T("");

    int nProductID = 0;
    nResult = 0;

	for ( int nIndex = 0; static_cast<unsigned int>(nIndex) < listRateIDs.size(); nIndex++ ) {

        szProductID = listRateIDs[nIndex];
        nProductID = atoi(szProductID.c_str());

        //-----------------------------------------------------------------
        // Process purchase
        //-----------------------------------------------------------------
        szPurchaseProductStart = _format(_T("Purchasing product %d"), nProductID);

	    DIOMEDE_CONSOLE::PurchaseProductTask taskPurchaseProduct(m_szSessionToken, nProductID);
	    nResult = HandleTask(&taskPurchaseProduct, szPurchaseProductStart);

        //-----------------------------------------------------------------
        // Check results
        //-----------------------------------------------------------------
	    if (nResult == SOAP_OK) {
	        std::string szStatusMsg = _format(_T("Purchase product %d successful."), nProductID);
            PrintStatusMsg(szStatusMsg);

            ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	    }
	    else {
	        std::string szErrorMsg = taskPurchaseProduct.GetServiceErrorMsg();
	        PrintServiceError(stderr, szErrorMsg);

	        ClientLog(UI_COMP, LOG_ERROR, false,_T("Purchase product %d failed."), nProductID);
	        bSuccess = false;
	    }
    }

    return bSuccess;

} // End ProcessPurchaseProductCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get my products command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetMyProductsCommand(CmdLine* pCmdLine,
                                                 bool& bCommandFinished)
{
    // User must be logged in to get their products.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get products"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get my products: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETMYPRODUCTS << endl;
    }

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process GetMyProducts
    //-----------------------------------------------------------------
    PrintNewLine();

	DIOMEDE_CONSOLE::GetMyProductsTask taskGetMyProducts(m_szSessionToken);
	int nResult = HandleTask(&taskGetMyProducts, _T("Getting my products"), _T(""), false, false);

    //-----------------------------------------------------------------
    // Check GetMyProducts results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
        // If there are no products, continue on..
        if (taskGetMyProducts.GetResult() != DIOMEDE_NO_MATCHES_FOUND) {
            std::string szErrorMsg = taskGetMyProducts.GetServiceErrorMsg();
            PrintServiceError(stderr, szErrorMsg, true);
	        return;
	    }
	}

    //-----------------------------------------------------------------
    // Process GetMyContracts
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::GetMyContractsTask taskGetMyContracts(m_szSessionToken);
	nResult = HandleTask(&taskGetMyContracts, _T("Getting my contracts"), _T(""), false, false);

    //-----------------------------------------------------------------
    // Check GetMyContracts results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
        // If there are no contracts, continue on..
        if (taskGetMyContracts.GetResult() != DIOMEDE_NO_MATCHES_FOUND) {
            std::string szErrorMsg = taskGetMyContracts.GetServiceErrorMsg();
            PrintServiceError(stderr, szErrorMsg, true);
	        return;
	    }
	}

    //-----------------------------------------------------------------
    // Display data from products, supports, and contracts
    //-----------------------------------------------------------------

    // Since we've turned off the task new lines, add one now...
    PrintNewLine();

    UserProductListImpl* pListUserProducts = taskGetMyProducts.GetUserProductResults();
    UserContractListImpl* pListUserContracts = taskGetMyContracts.GetUserContractResults();

    //-----------------------------------------------------------------
    // Display products
    //-----------------------------------------------------------------
    ClientLog(UI_COMP, LOG_STATUS, false,_T("Get my products successful."));

    std::vector<void * > listMyProducts = pListUserProducts->GetUserProductList();

    // Edge case to ensure we did get something back.
    if (listMyProducts.size() == 0) {
        std::string szStatusMsg = _T("Get my products: no rates returned.");
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_ERROR, false, _T("%s"), szStatusMsg.c_str());
        /*
        SimpleRedirect::Instance()->EndRedirect();
        return;
        */
    }

    //-----------------------------------------------------------------
    // Make sure we have the list of storage type names.
    //-----------------------------------------------------------------

    if (m_pListStorageTypes == NULL) {
        m_pListStorageTypes = new StorageTypeListImpl;
    }

    // If the storage types list is empty, get the list now...
    std::vector<void * > listStorageTypes = m_pListStorageTypes->GetStorageTypeList();
    if ( (int)listStorageTypes.size() == 0) {
        GetStorageTypes(false);
    }

    //-----------------------------------------------------------------
    // locale facet for formatting currency
    //-----------------------------------------------------------------
    std::locale localEnglish(DiomedeMoneyPunct::GetLocaleString().c_str());
    std::locale locDiomede(localEnglish, new DiomedeMoneyPunct(4));
    cout.imbue(locDiomede);

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------

    int nMyRateIndex = 0;
    int nCompIndex = 0;
    int nBandwidthIndex = 0;

    std::string szTemp = _T("");
    std::string szStorageTypeName = _T("");

    for (nMyRateIndex = 0; nMyRateIndex < (int)listMyProducts.size(); nMyRateIndex ++) {

        UserProductImpl* pUserProduct = (UserProductImpl*)listMyProducts[nMyRateIndex];

        if (pUserProduct == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get my products: NULL product set."));
            continue;
        }

        //-------------------------------------------------------------
        // Product <product ID>: <product name>
        //-------------------------------------------------------------
        cout << left << _T(" Product ") << pUserProduct->GetProductID() << _T(": ");
        cout <<  pUserProduct->GetName() << endl;

        //-------------------------------------------------------------
        // Minimum Monthly Fee
        // Support Fee
        //-------------------------------------------------------------
        cout << setw(4) << left << _T("") << setw(55) <<  _T("Minimum Monthly Fee ");

        double dwMinMonthlyFee = (double)pUserProduct->GetMinMonthlyFee();
        cout << setw(20) << std::showbase << Money(dwMinMonthlyFee) << endl;

        cout << setw(4) << _T("") << setw(55) <<  _T("Support Fee ");

        double dwSupportFee = (double)pUserProduct->GetSupportFee();
        cout << setw(20) << std::showbase << Money(dwSupportFee)<< endl << endl;

        ComponentListImpl* pProductComponents = pUserProduct->GetComponentList();

        if (pProductComponents == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get my products: NULL list of rate components."));
            continue;
        }

        //-------------------------------------------------------------
        // Each product component:
        // <storage type> <Meter Type>                  <rate per GB>
        //-------------------------------------------------------------
        std::vector<void * > listProductComponents = pProductComponents->GetComponentList();
        std::vector<int> listBandwidthEntries;

        for (nCompIndex = 0; nCompIndex < (int)listProductComponents.size(); nCompIndex ++) {
            ComponentImpl* pProductComponent = (ComponentImpl*)listProductComponents[nCompIndex];

            if (pProductComponent == NULL) {
                ClientLog(UI_COMP, LOG_ERROR, false,_T("Get all products: NULL rate component."));
                continue;
            }

            // We want to show the bandwith entries separate - so we'll skip them for now and
            // add them at end of this loop.
            if (pProductComponent->GetMeterType().find("Bandwidth") != (size_t)-1 ) {
                listBandwidthEntries.push_back(nCompIndex);
                continue;
            }

            szStorageTypeName = _T("");
            GetStorageTypeName(pProductComponent->GetStorageTypeID(), szStorageTypeName);

            szTemp = _format(_T("%s %s"), szStorageTypeName.c_str(),
                pProductComponent->GetMeterType().c_str());
            cout << setw(4) << left << _T("") << setw(55) << szTemp;

            // Rate should be sent as a double - to format properly at this point,
            // convert to a double and then back to our currency amount.
            double dwRatePerGB = (double)pProductComponent->GetRatePerGB();
            cout << setw(20) << std::showbase << Money(dwRatePerGB) << endl;
        }

        // Now show the bandwidth entries.
        if (listBandwidthEntries.size() > 0) {
            PrintNewLine();
            for (nBandwidthIndex = 0; nBandwidthIndex < (int)listBandwidthEntries.size(); nBandwidthIndex ++) {
                nCompIndex = listBandwidthEntries[nBandwidthIndex];
                ComponentImpl* pProductComponent = (ComponentImpl*)listProductComponents[nCompIndex];

                szStorageTypeName = _T("");
                GetStorageTypeName(pProductComponent->GetStorageTypeID(), szStorageTypeName);

                szTemp = _format(_T("%s %s"), szStorageTypeName.c_str(),
                    pProductComponent->GetMeterType().c_str());

                cout << setw(4) << left << _T("") << setw(55) << szTemp;

                // Rate should be sent as a double - to format properly at this point,
                // convert to a double and then back to our currency amount.
                double dwRatePerGB = (double)pProductComponent->GetRatePerGB();
                cout << setw(20) << std::showbase << Money(dwRatePerGB) << endl;
            }
        }

        PrintNewLine();
    }

    //-----------------------------------------------------------------
    // Display contracts
    //-----------------------------------------------------------------
    ClientLog(UI_COMP, LOG_STATUS, false,_T("Get my contracts successful."));

    std::vector<void * > listMyContracts = pListUserContracts->GetUserContractList();

    // Edge case to ensure we did get something back.
    if (listMyContracts.size() == 0) {
        std::string szStatusMsg = _T("Get my contracts: no contracts returned.");
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_ERROR, false, _T("%s"), szStatusMsg.c_str());
        /*
        SimpleRedirect::Instance()->EndRedirect();
        return;
        */
    }

    std::string szCommittedGB = _T("");
    std::string szStorage = _T("");

    int nMyContractIndex = 0;
    nCompIndex = 0;

    if ( (int)listMyContracts.size() > 0) {
        cout << left << _T(" Contracts: ") << endl;
    }

    for (nMyContractIndex = 0; nMyContractIndex < (int)listMyContracts.size(); nMyContractIndex ++) {

        //-------------------------------------------------------------
        // User contract ID
        //-------------------------------------------------------------
        /*
        cout << setw(24) << right << _T("User contract ID: ")
             << listMyContracts[nMyContractIndex]->contractID << endl;
        */

        //-------------------------------------------------------------
        // User contracts
        //-------------------------------------------------------------
        szTemp = _T("");
        szCommittedGB = _T("");
        szStorage = _T("");

        UserContractImpl* pUserContract = (UserContractImpl*)listMyContracts[nMyContractIndex];

        if (pUserContract == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get my contracts: NULL contract."));
            continue;
        }

        //-------------------------------------------------------------
        // Each contract:
        // <zontract ID>: <contract term>-months, <committed GB> <storage type> <meter type>    <rate/term>
        //-------------------------------------------------------------
        ContractComponentListImpl* pListContractComponents = pUserContract->GetContractComponentList();
        if (pListContractComponents == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get my contracts: NULL contract components."));
            continue;
        }

        std::vector<void * > listContractComponents = pListContractComponents->GetContractComponentList();
        int nNumComponents = (int)listContractComponents.size();

        for (nCompIndex = 0; nCompIndex < nNumComponents; nCompIndex++) {

            ContractComponentImpl* pContractComponent = (ContractComponentImpl*)listContractComponents[nCompIndex];
            if (pContractComponent == NULL) {
                ClientLog(UI_COMP, LOG_ERROR, false,_T("Get my contracts: NULL list of contract components."));
                continue;
            }

            // No matter how many contract components we have, the contract ID and term
            // is display just once.
            if (nCompIndex == 0) {
                szTemp = _format(_T("%d: "), pUserContract->GetContractID());
                cout << setw(12) << right << szTemp;

                int nTerm = pUserContract->GetTerm();
                if (nTerm < 10) {
                    szTemp = _format(_T(" %d-month, "), nTerm);
                }
                else {
                    szTemp = _format(_T("%d-months, "), nTerm);
                }

                cout << setw(11) << left << szTemp;

                if (nNumComponents > 1) {
                    // Rate should be sent as a double - to format properly at this point,
                    // convert to a double and then back to our currency amount.
                    double dwRatePerTerm = (double)pUserContract->GetRatePerTerm();

                    cout << setw(35) << _T("") << setw(20) << std::showbase
                         << Money(dwRatePerTerm) << endl;
                }
            }

            // If we have just one contract component, we'll display it all on one
            // line.  Otherwise, each component is displayed on its own line.
            // The term is display once - on the same line as the contract ID
            // (see above).
            StringUtil::FormatNumber(pContractComponent->GetCommittedGB(), szCommittedGB);
            szCommittedGB = szCommittedGB + _T(" GB ");

            szStorageTypeName = _T("");
            GetStorageTypeName(pContractComponent->GetStorageTypeID(), szStorageTypeName);

            szStorage = _format(_T("%s %s"), szStorageTypeName.c_str(),
                pContractComponent->GetMeterType().c_str());

            if (nNumComponents == 1) {
                cout << setw(11) << szCommittedGB;
                cout << setw(25) << szStorage;

                // Rate should be sent as a double - to format properly at this point,
                // convert to a double and then back to our currency amount.
                double dwRatePerTerm = (double)pUserContract->GetRatePerTerm();

                // TBD: we'll receive the rate as an int with possibly a
                // precision - to mimic that, the rate is multiplied by
                // 10000 (assuming 4 digit precision

                cout << setw(20) << std::showbase << Money(dwRatePerTerm) << endl;
            }
            else {
                cout << setw(11) << _T("") << setw(10) << szCommittedGB;
                cout << setw(30) << szStorage << endl;
            }
        }
    }

    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessGetMyProductsCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the cancel product command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessCancelProductCommand(CmdLine* pCmdLine,
                                                 bool& bCommandFinished)
{
    // User must be logged in to cancel a product.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot cancel product"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Cancel product: you must be logged into the Diomede service."));
	    return;
    }

    // Get the list of arguments - the list should contain one or more product IDs
    bCommandFinished = false;
    DiomedeUnlabeledMultiArg<std::string>* pRateIDArg = NULL;

    try {
        pRateIDArg = (DiomedeUnlabeledMultiArg<std::string>*)pCmdLine->getArg(ARG_PRODID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_CANCELPRODUCT << endl;
    }

    if (pRateIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

    //-----------------------------------------------------------------
    // Check for product IDs - TBD validate this input against a
    // known set of product IDs.
    //-----------------------------------------------------------------
	std::vector<std::string> listRateIDs = pRateIDArg->getValue();

	if (listRateIDs.size() == 0) {

	    if (pRateIDArg->getRepromptCount() == 0) {
	        m_szCommandPrompt = _T("Product(s): ");
	        pRateIDArg->incrementRepromptCount();
	    }
	    else if (pRateIDArg->getRepromptCount() < 2) {
	        pRateIDArg->incrementRepromptCount();
	    }
	    else {
	        // We've prompted them already - alert them that they need
	        // a product ID...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
	    }

	    return;
	}

    bCommandFinished = true;

    std::string szProductID = _T("");
    std::string szCancelProductStart = _T("");
    int nProductID = 0;
    int nResult = 0;

	for ( int nIndex = 0; static_cast<unsigned int>(nIndex) < listRateIDs.size(); nIndex++ ) {

        szProductID = listRateIDs[nIndex];
        nProductID = atoi(szProductID.c_str());

        //-----------------------------------------------------------------
        // Process cancelproduct
        //-----------------------------------------------------------------
        szCancelProductStart = _format(_T("Cancelling product %d"), nProductID);

	    DIOMEDE_CONSOLE::CancelProductTask taskCancelProduct(m_szSessionToken, nProductID);
	    nResult = HandleTask(&taskCancelProduct, szCancelProductStart);

        //-----------------------------------------------------------------
        // Check results
        //-----------------------------------------------------------------
	    if (nResult == SOAP_OK) {
	        std::string szStatusMsg = _format(_T("Cancel product %d successful."), nProductID);
            PrintStatusMsg(szStatusMsg);

            ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	    }
	    else {
            std::string szErrorMsg = taskCancelProduct.GetServiceErrorMsg();
            PrintServiceError(stderr, szErrorMsg);
	        ClientLog(UI_COMP, LOG_ERROR, false,_T("Cancel product %d failed."), nProductID);
	    }
    }

} // End ProcessCancelProductCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get all contracts command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetAllContractsCommand(CmdLine* pCmdLine,
                                                   bool& bCommandFinished)
{
    // The user is not required to login to access this command.
    bCommandFinished = true;
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETALLCONTRACTS << endl;
    }

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process GetAllContracts
    //-----------------------------------------------------------------
    PrintNewLine();

	DIOMEDE_CONSOLE::GetAllContractsTask taskGetAllContracts;
	int nResult = HandleTask(&taskGetAllContracts, _T("Getting all contracts"), _T(""), false, false);

    //-----------------------------------------------------------------
    // Check GetAllContracts results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
        std::string szErrorMsg = taskGetAllContracts.GetServiceErrorMsg();
        PrintServiceError(stderr, szErrorMsg);
	    return;
	}

    //-----------------------------------------------------------------
    // Make sure we have the list of storage type names.
    //-----------------------------------------------------------------

    if (m_pListStorageTypes == NULL) {
        m_pListStorageTypes = new StorageTypeListImpl;
    }

    // If the storage types list is empty, get the list now...
    std::vector<void * > listStorageTypes = m_pListStorageTypes->GetStorageTypeList();
    if ( (int)listStorageTypes.size() == 0) {
        GetStorageTypes(false);
    }

    //-----------------------------------------------------------------
    // Display results contracts
    //-----------------------------------------------------------------

    // Since we've turned off the task new lines, add one now...
    PrintNewLine();

    //-----------------------------------------------------------------
    // locale facet for formatting currency
    //-----------------------------------------------------------------
    std::locale localEnglish(DiomedeMoneyPunct::GetLocaleString().c_str());
    std::locale locDiomede(localEnglish, new DiomedeMoneyPunct(4));
    cout.imbue(locDiomede);

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------

    /*
    cout << _T("12345678901234567890123456789012345678901234567890123456789012345678901234567890") << endl;
    */

    //-----------------------------------------------------------------
    // Display contracts
    //-----------------------------------------------------------------
    ClientLog(UI_COMP, LOG_STATUS, false,_T("Get all contracts successful."));

	ContractListImpl* pListContracts = taskGetAllContracts.GetContractResults();
    std::vector<void * > listContracts = pListContracts->GetContractList();

    // Edge case to ensure we did get something back.
    if (listContracts.size() == 0) {
        std::string szStatusMsg = _T("Get all contracts: no contracts returned.");
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_ERROR, false, _T("%s"), szStatusMsg.c_str());
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    std::string szCommittedGB = _T("");
    std::string szStorage = _T("");

    std::string szTemp = _T("");
    std::string szStorageTypeName = _T("");

    int nContractIndex = 0;
    int nCompIndex = 0;

    cout << endl;
    if ( (int)listContracts.size() > 0) {
        cout << left << _T(" Contracts: ") << endl;
    }

    for (nContractIndex = 0; nContractIndex < (int)listContracts.size(); nContractIndex ++) {
        szTemp = _T("");
        szCommittedGB = _T("");
        szStorage = _T("");

        ContractImpl* pContract = (ContractImpl*)listContracts[nContractIndex];

        if (pContract == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get all contracts: NULL contract."));
            continue;
        }

        //-------------------------------------------------------------
        // Each contract:
        // <zontract ID>: <contract term>-months, <committed GB> <storage type> <meter type>    <rate/term>
        //-------------------------------------------------------------
        ContractComponentListImpl* pListContractComponents = pContract->GetContractComponentList();
        if (pListContractComponents == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get all contracts: NULL contract components."));
            continue;
        }

        std::vector<void * > listContractComponents = pListContractComponents->GetContractComponentList();
        int nNumComponents = (int)listContractComponents.size();

        for (nCompIndex = 0; nCompIndex < nNumComponents; nCompIndex++) {

            ContractComponentImpl* pContractComponent = (ContractComponentImpl*)listContractComponents[nCompIndex];
            if (pContractComponent == NULL) {
                ClientLog(UI_COMP, LOG_ERROR, false,_T("Get all contracts: NULL list of contract components."));
                continue;
            }

            // No matter how many contract components we have, the contract ID and term
            // is display just once.
            if (nCompIndex == 0) {
                szTemp = _format(_T("%d: "), pContract->GetContractID());
                cout << setw(4) << left << _T("") << right << szTemp;

                int nTerm = pContract->GetTerm();
                if (nTerm < 10) {
                    szTemp = _format(_T(" %d-month, "), nTerm);
                }
                else {
                    szTemp = _format(_T("%d-months, "), nTerm);
                }

                cout << setw(11) << left << szTemp;

                if (nNumComponents > 1) {
                    // Rate should be sent as a double - to format properly at this point,
                    // convert to a double and then back to our currency amount.
                    double dwRatePerTerm = (double)pContract->GetRatePerTerm();

                    cout << setw(38) << _T("") << setw(20) << std::showbase
                         << Money(dwRatePerTerm) << endl;
                }
            }

            // If we have just one contract component, we'll display it all on one
            // line.  Otherwise, each component is displayed on its own line.
            // The term is display once - on the same line as the contract ID
            // (see above).
            StringUtil::FormatNumber(pContractComponent->GetCommittedGB(), szCommittedGB);
            szCommittedGB = szCommittedGB + _T(" GB ");

            szStorageTypeName = _T("");
            GetStorageTypeName(pContractComponent->GetStorageTypeID(), szStorageTypeName);

            szStorage = _format(_T("%s %s"), szStorageTypeName.c_str(),
                pContractComponent->GetMeterType().c_str());

            if (nNumComponents == 1) {
                cout << setw(11) << szCommittedGB;
                cout << setw(27) << szStorage;

                // Rate should be sent as a double - to format properly at this point,
                // convert to a double and then back to our currency amount.
                double dwRatePerTerm = (double)pContract->GetRatePerTerm();

                // TBD: we'll receive the rate as an int with possibly a
                // precision - to mimic that, the rate is multiplied by
                // 10000 (assuming 4 digit precision

                cout << setw(20) << std::showbase << Money(dwRatePerTerm) << endl;
            }
            else {
                cout << setw(11) << _T("") << setw(10) << szCommittedGB;
                cout << setw(26) << szStorage << endl;
            }
        }
    }

    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessGetAllContractsCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the purchase contract command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessPurchaseContractCommand(CmdLine* pCmdLine,
                                                   bool& bCommandFinished)
{
    // User must be logged in to purchase a contract.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot purchase contract"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Purchase contract: you must be logged into the Diomede service."));
	    return;
    }

    // Get the list of arguments - the list should contain one or more contract IDs
    bCommandFinished = false;
    DiomedeUnlabeledMultiArg<std::string>* pContractIDArg = NULL;

    try {
        pContractIDArg = (DiomedeUnlabeledMultiArg<std::string>*)pCmdLine->getArg(ARG_CONTRACTID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_PURCHASECONTRACT << endl;
    }

    if (pContractIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

    //-----------------------------------------------------------------
    // Check for contract IDs - TBD validate this input against a
    // known set of contract IDs.
    //-----------------------------------------------------------------
	std::vector<std::string> listContractIDs = pContractIDArg->getValue();

	if (listContractIDs.size() == 0) {

	    if (pContractIDArg->getRepromptCount() == 0) {
	        m_szCommandPrompt = _T("Contract(s): ");
	        pContractIDArg->incrementRepromptCount();
	    }
	    else if (pContractIDArg->getRepromptCount() < 2) {
	        pContractIDArg->incrementRepromptCount();
	    }
	    else {
	        // We've prompted them already - alert them that they need
	        // a contract ID...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
	    }

	    return;
	}

    bCommandFinished = true;

    std::string szContractID = _T("");
    std::string szPurchaseContractStart = _T("");
    int nContractID = 0;
    int nResult = 0;

	for ( int nIndex = 0; static_cast<unsigned int>(nIndex) < listContractIDs.size(); nIndex++ ) {

        szContractID = listContractIDs[nIndex];
        nContractID = atoi(szContractID.c_str());

        //-----------------------------------------------------------------
        // Process purchase
        //-----------------------------------------------------------------
        szPurchaseContractStart = _format(_T("Purchasing contract %d"), nContractID);

	    DIOMEDE_CONSOLE::PurchaseContractTask taskPurchaseContract(m_szSessionToken, nContractID);
	    nResult = HandleTask(&taskPurchaseContract, szPurchaseContractStart);

        //-----------------------------------------------------------------
        // Check results
        //-----------------------------------------------------------------
        LONG64 l64UserContractID = -1;

	    if (nResult == SOAP_OK) {
	        l64UserContractID = taskPurchaseContract.GetUserContractID();
            #ifdef WIN32
	            std::string szStatusMsg = _format(_T("Purchase contract %d successful: %I64d"), nContractID,
	                l64UserContractID);
	        #else
	            std::string szStatusMsg = _format(_T("Purchase contract %d successful: %lld"), nContractID,
	                l64UserContractID);
	        #endif
            PrintStatusMsg(szStatusMsg);
            ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	    }
	    else {
            std::string szErrorMsg = taskPurchaseContract.GetServiceErrorMsg();
            PrintServiceError(stderr, szErrorMsg);
	        ClientLog(UI_COMP, LOG_ERROR, false,_T("Purchase contract %d failed."), nContractID);
	    }
    }

} // End ProcessPurchaseContractCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the get my contracts command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessGetMyContractsCommand(CmdLine* pCmdLine,
                                                 bool& bCommandFinished)
{
    // User must be logged in to get their contracts.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot get contracts"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Get my contracts: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_GETMYCONTRACTS << endl;
    }

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process GetMyContracts
    //-----------------------------------------------------------------
    PrintNewLine();

	DIOMEDE_CONSOLE::GetMyContractsTask taskGetMyContracts(m_szSessionToken);
	int nResult = HandleTask(&taskGetMyContracts, _T("Getting my contracts"), _T(""), false, false);

    //-----------------------------------------------------------------
    // Check Getmycontracts results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
        std::string szErrorMsg = taskGetMyContracts.GetServiceErrorMsg();
        PrintServiceError(stderr, szErrorMsg);
	    return;
	}

    //-----------------------------------------------------------------
    // Display data from contracts
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // Make sure we have the list of storage type names.
    //-----------------------------------------------------------------

    if (m_pListStorageTypes == NULL) {
        m_pListStorageTypes = new StorageTypeListImpl;
    }

    // If the storage types list is empty, get the list now...
    std::vector<void * > listStorageTypes = m_pListStorageTypes->GetStorageTypeList();
    if ( (int)listStorageTypes.size() == 0) {
        GetStorageTypes(false);
    }

    //-----------------------------------------------------------------
    // locale facet for formatting currency
    //-----------------------------------------------------------------
    std::locale localEnglish(DiomedeMoneyPunct::GetLocaleString().c_str());
    std::locale locDiomede(localEnglish, new DiomedeMoneyPunct(4));
    cout.imbue(locDiomede);

    //-----------------------------------------------------------------
    // Display contracts
    //-----------------------------------------------------------------

    // Since we've turned off the task new lines, add one now...
    PrintNewLine();

    ClientLog(UI_COMP, LOG_STATUS, false,_T("Get my contracts successful."));

    UserContractListImpl* pListUserContracts = taskGetMyContracts.GetUserContractResults();
    std::vector<void * > listMyContracts = pListUserContracts->GetUserContractList();

    // Edge case to ensure we did get something back.
    if (listMyContracts.size() == 0) {
        std::string szStatusMsg = _format("Get my contracts: no contracts returned.");
        PrintStatusMsg(szStatusMsg);

        ClientLog(UI_COMP, LOG_ERROR, false, _T("%s"), szStatusMsg.c_str());
        /*
        SimpleRedirect::Instance()->EndRedirect();
        return;
        */
    }

    std::string szCommittedGB = _T("");
    std::string szStorage = _T("");
    std::string szTemp = _T("");
    std::string szStorageTypeName = _T("");

    int nMyContractIndex = 0;
    int nCompIndex = 0;

    if ( (int)listMyContracts.size() > 0) {
        cout << left << _T(" Contracts: ") << endl;
    }

    for (nMyContractIndex = 0; nMyContractIndex < (int)listMyContracts.size(); nMyContractIndex ++) {

        //-------------------------------------------------------------
        // User contract ID
        //-------------------------------------------------------------
        /*
        cout << setw(24) << right << _T("User contract ID: ")
             << listMyContracts[nMyContractIndex]->contractID << endl;
        */

        //-------------------------------------------------------------
        // User contracts
        //-------------------------------------------------------------
        szTemp = _T("");
        szCommittedGB = _T("");
        szStorage = _T("");

        UserContractImpl* pUserContract = (UserContractImpl*)listMyContracts[nMyContractIndex];

        if (pUserContract == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get my contracts: NULL contract."));
            continue;
        }

        //-------------------------------------------------------------
        // Each contract:
        // <zontract ID>: <contract term>-months, <committed GB> <storage type> <meter type>    <rate/term>
        //-------------------------------------------------------------
        ContractComponentListImpl* pListContractComponents = pUserContract->GetContractComponentList();
        if (pListContractComponents == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Get my contracts: NULL contract components."));
            continue;
        }

        std::vector<void * > listContractComponents = pListContractComponents->GetContractComponentList();
        int nNumComponents = (int)listContractComponents.size();

        for (nCompIndex = 0; nCompIndex < nNumComponents; nCompIndex++) {

            ContractComponentImpl* pContractComponent = (ContractComponentImpl*)listContractComponents[nCompIndex];
            if (pContractComponent == NULL) {
                ClientLog(UI_COMP, LOG_ERROR, false,_T("Get my contracts: NULL list of contract components."));
                continue;
            }

            // No matter how many contract components we have, the contract ID and term
            // is display just once.
            if (nCompIndex == 0) {
                szTemp = _format(_T("%d: "), pUserContract->GetUserContractID());
                cout << setw(12) << right << szTemp;

                int nTerm = pUserContract->GetTerm();
                if (nTerm < 10) {
                    szTemp = _format(_T(" %d-month, "), nTerm);
                }
                else {
                    szTemp = _format(_T("%d-months, "), nTerm);
                }

                cout << setw(11) << left << szTemp;

                if (nNumComponents > 1) {
                    // Rate should be sent as a double - to format properly at this point,
                    // convert to a double and then back to our currency amount.
                    double dwRatePerTerm = (double)pUserContract->GetRatePerTerm();

                    cout << setw(38) << _T("") << setw(20) << std::showbase
                         << Money(dwRatePerTerm) << endl;
                }
            }

            // If we have just one contract component, we'll display it all on one
            // line.  Otherwise, each component is displayed on its own line.
            // The term is display once - on the same line as the contract ID
            // (see above).
            StringUtil::FormatNumber(pContractComponent->GetCommittedGB(), szCommittedGB);
            szCommittedGB = szCommittedGB + _T(" GB ");

            szStorageTypeName = _T("");
            GetStorageTypeName(pContractComponent->GetStorageTypeID(), szStorageTypeName);

            szStorage = _format(_T("%s %s"), szStorageTypeName.c_str(),
                pContractComponent->GetMeterType().c_str());

            if (nNumComponents == 1) {
                cout << setw(11) << szCommittedGB;
                cout << setw(25) << szStorage;

                // Rate should be sent as a double - to format properly at this point,
                // convert to a double and then back to our currency amount.
                double dwRatePerTerm = (double)pUserContract->GetRatePerTerm();

                // TBD: we'll receive the rate as an int with possibly a
                // precision - to mimic that, the rate is multiplied by
                // 10000 (assuming 4 digit precision

                cout << setw(20) << std::showbase << Money(dwRatePerTerm) << endl;
            }
            else {
                cout << setw(11) << _T("") << setw(10) << szCommittedGB;
                cout << setw(30) << szStorage << endl;
            }
        }
    }

    if (!m_bSysCommandInput) {
        PrintNewLine();
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessGetMyContractsCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the cancel contract command.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessCancelContractCommand(CmdLine* pCmdLine,
                                                  bool& bCommandFinished)
{
    // User must be logged in to cancel a contract.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot cancel contract"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Cancel contract: you must be logged into the Diomede service."));
	    return;
    }

    // Get the list of arguments - the list should contain one or more contract IDs
    bCommandFinished = false;
    DiomedeUnlabeledMultiArg<std::string>* pContractIDArg = NULL;

    try {
        pContractIDArg = (DiomedeUnlabeledMultiArg<std::string>*)pCmdLine->getArg(ARG_CONTRACTID);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_CANCELCONTRACT << endl;
    }

    if (pContractIDArg == NULL) {
        bCommandFinished = true;
        return;
    }

    //-----------------------------------------------------------------
    // Check for contract IDs - TBD validate this input against a
    // known set of contract IDs.
    //-----------------------------------------------------------------
	std::vector<std::string> listContractIDs = pContractIDArg->getValue();

	if (listContractIDs.size() == 0) {

	    if (pContractIDArg->getRepromptCount() == 0) {
	        m_szCommandPrompt = _T("Contract(s): ");
	        pContractIDArg->incrementRepromptCount();
	    }
	    else if (pContractIDArg->getRepromptCount() < 2) {
	        pContractIDArg->incrementRepromptCount();
	    }
	    else {
	        // We've prompted them already - alert them that they need
	        // a ContractI ID...
            pCmdLine->getHelpVisitor()->visit();
            bCommandFinished = true;
	    }

	    return;
	}

    bCommandFinished = true;

    std::string szContractID = _T("");
    std::string szCancelContractStart = _T("");
    LONG64 l64ContractID = 0;
    int nResult = 0;

	for ( int nIndex = 0; static_cast<unsigned int>(nIndex) < listContractIDs.size(); nIndex++ ) {

        szContractID = listContractIDs[nIndex];
        l64ContractID = atoi64(szContractID.c_str());

        //-----------------------------------------------------------------
        // Process Cancelcontract
        //-----------------------------------------------------------------
        szCancelContractStart = _format(_T("Cancelling contract %s"), szContractID.c_str());

	    DIOMEDE_CONSOLE::CancelContractTask taskCancelContract(m_szSessionToken, l64ContractID);
	    nResult = HandleTask(&taskCancelContract, szCancelContractStart);

        //-----------------------------------------------------------------
        // Check results
        //-----------------------------------------------------------------
	    if (nResult == SOAP_OK) {
	        std::string szStatusMsg = _format(_T("Cancel contract %s successful."), szContractID.c_str());
	        PrintStatusMsg(szStatusMsg);

	        ClientLog(UI_COMP, LOG_STATUS, false, _T("%s"), szStatusMsg.c_str());
	    }
	    else {
            std::string szErrorMsg = taskCancelContract.GetServiceErrorMsg();
            PrintServiceError(stderr, szErrorMsg);
	        ClientLog(UI_COMP, LOG_ERROR, false,_T("Cancel contract %s failed."), szContractID.c_str());
	    }
    }

} // End ProcessCancelContractCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the search upload log command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSearchUploadLogCommand(CmdLine* pCmdLine,
                                                   bool& bCommandFinished)
{
    // User must be logged in to get the upload log statistics.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot search upload log"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Search upload log: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    DiomedeValueArg<std::string>* pFileIDArg = NULL;
    DiomedeValueArg<std::string>* pStartDateArg = NULL;
    DiomedeValueArg<std::string>* pEndDateArg = NULL;
    DiomedeValueArg<std::string>* pPageSizeArg = NULL;
    DiomedeValueArg<std::string>* pOffsetArg = NULL;
    DiomedeValueArg<std::string>* pIPArg = NULL;
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pFileIDArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pStartDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_STARTDATE);
        pEndDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_ENDDATE);
        pIPArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_IP);
        pPageSizeArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_PAGE_SIZE);
        pOffsetArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_OFFSET);
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SEARCHUPLOADS << endl;
    }

    SearchUploadLogFilterImpl* pSearchFilter = new SearchUploadLogFilterImpl;

    if (pFileIDArg && pFileIDArg->isSet()) {
        LONG64 l64FileID = atoi64(pFileIDArg->getValue().c_str());
        pSearchFilter->SetFileID(l64FileID);
    }

    time_t epochSeconds;

    if (pStartDateArg && pStartDateArg->isSet()) {
        if ( ValidateDate(pStartDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateStart(epochSeconds);
        }
        else {
	        _tprintf(_T("Start date %s is not a valid date and will be ignored.\n\r"),
	            pStartDateArg->getValue().c_str());
        }
    }

    if (pEndDateArg && pEndDateArg->isSet()) {
        if ( ValidateDate(pEndDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateEnd(epochSeconds);
        }
        else {
	        _tprintf(_T("End date %s is not a valid date and will be ignored.\n\r"),
	            pEndDateArg->getValue().c_str());
        }
    }

    if (pIPArg && pIPArg->isSet()) {
        pSearchFilter->SetUploadIP(pIPArg->getValue());
    }

    LONG64 l64Value = 0L;
    if (pPageSizeArg && pPageSizeArg->isSet()) {
        l64Value = atoi64(pPageSizeArg->getValue().c_str());
        pSearchFilter->SetPageSize(l64Value);
    }
    else {
        if (GetConfigPageSize(l64Value) ) {
            pSearchFilter->SetPageSize(l64Value);
        }
    }

    if (pOffsetArg && pOffsetArg->isSet()) {
        l64Value = atoi64(pOffsetArg->getValue().c_str());
        pSearchFilter->SetOffset(l64Value);
    }
    else {
        if (GetConfigOffset(l64Value) ) {
            pSearchFilter->SetOffset(l64Value);
        }
    }

    //-----------------------------------------------------------------
    // Process search
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::SearchUploadLogTask taskUploadLog(m_szSessionToken, pSearchFilter);
	int nResult = HandleTask(&taskUploadLog, _T("Searching upload log"));

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pSearchFilter != NULL) {
        delete pSearchFilter;
        pSearchFilter = NULL;
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
        // If there are no results returned the error is caught later on...
        if (nResult != DIOMEDE_NO_MATCHES_FOUND) {
            std::string szErrorMsg = taskUploadLog.GetServiceErrorMsg();
    	    PrintServiceError(stderr, szErrorMsg);

    	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Search upload log failed.") );
	        return;
	    }
	}

    ClientLog(UI_COMP, LOG_STATUS, false,_T("Search upload log suuccessful."));

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process results.
    //-----------------------------------------------------------------
    UploadLogListImpl* pListUploadLog = taskUploadLog.GetSearchUploadLogResults();
    std::vector<void * > listLogEntries = pListUploadLog->GetUploadLogList();

    // If no entries found, output a message and return.
    if ((int)listLogEntries.size() == 0) {
        PrintStatusMsg(_T("Search upload log successful.  No matches found."));
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    std::string szFormattedDate = _T("");
    std::string szBytes = _T("");
    std::string szBytesSizeType = _T("");

    for (int nIndex = 0; nIndex < (int)listLogEntries.size(); nIndex ++) {

        UploadLogEntryImpl* pLogEntry = (UploadLogEntryImpl*)listLogEntries[nIndex];
        if (pLogEntry == NULL) {
            continue;
        }

		#ifdef WIN32
			_tprintf(_T("    Logical file ID: %I64d \n\r"), pLogEntry->GetFileID());
		#else
			_tprintf(_T("     Logical file ID: %lld \n\r"), pLogEntry->GetFileID());
		#endif

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pLogEntry->GetUploadTime(), szFormattedDate);
        _tprintf(_T("     Date of upload: %s \n\r"), szFormattedDate.c_str());

        _tprintf(_T("        Uploader IP: %s \n\r"), pLogEntry->GetUploaderIP().c_str());
        _tprintf(_T("\n\r"));
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessSearchUploadLogCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the search download log command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSearchDownloadLogCommand(CmdLine* pCmdLine,
                                                     bool& bCommandFinished)
{
    // User must be logged in to get the download log statistics.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot search download log"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Search download log: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    DiomedeValueArg<std::string>* pFileIDArg = NULL;
    DiomedeValueArg<std::string>* pStartDateArg = NULL;
    DiomedeValueArg<std::string>* pEndDateArg = NULL;
    DiomedeValueArg<std::string>* pIPArg = NULL;
    DiomedeValueArg<std::string>* pTokenArg = NULL;
    DiomedeValueArg<std::string>* pPageSizeArg = NULL;
    DiomedeValueArg<std::string>* pOffsetArg = NULL;
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pFileIDArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_FILEID);
        pStartDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_STARTDATE);
        pEndDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_ENDDATE);
        pTokenArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_TOKEN);
        pIPArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_IP);
        pPageSizeArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_PAGE_SIZE);
        pOffsetArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_OFFSET);
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SEARCHDOWNLOADS << endl;
    }

    SearchDownloadLogFilterImpl* pSearchFilter = new SearchDownloadLogFilterImpl;

    if (pFileIDArg && pFileIDArg->isSet()) {
        LONG64 l64FileID = atoi64(pFileIDArg->getValue().c_str());
        pSearchFilter->SetFileID(l64FileID);
    }

    time_t epochSeconds;

    if (pStartDateArg && pStartDateArg->isSet()) {
        if ( ValidateDate(pStartDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateStart(epochSeconds);
        }
        else {
	        _tprintf(_T("Start date %s is not a valid date and will be ignored.\n\r"),
	            pStartDateArg->getValue().c_str());
        }
    }

    if (pEndDateArg && pEndDateArg->isSet()) {
        if ( ValidateDate(pEndDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateEnd(epochSeconds);
        }
        else {
	        _tprintf(_T("End date %s is not a valid date and will be ignored.\n\r"),
	            pEndDateArg->getValue().c_str());
        }
    }

    if (pTokenArg && pTokenArg->isSet()) {
        pSearchFilter->SetDownloadToken(pTokenArg->getValue());
    }
    if (pIPArg && pIPArg->isSet()) {
        pSearchFilter->SetDownloadIP(pIPArg->getValue());
    }

    LONG64 l64Value = 0L;
    if (pPageSizeArg && pPageSizeArg->isSet()) {
        l64Value = atoi64(pPageSizeArg->getValue().c_str());
        pSearchFilter->SetPageSize(l64Value);
    }
    else {
        if (GetConfigPageSize(l64Value) ) {
            pSearchFilter->SetPageSize(l64Value);
        }
    }

    if (pOffsetArg && pOffsetArg->isSet()) {
        l64Value = atoi64(pOffsetArg->getValue().c_str());
        pSearchFilter->SetOffset(l64Value);
    }
    else {
        if (GetConfigOffset(l64Value) ) {
            pSearchFilter->SetOffset(l64Value);
        }
    }

    //-----------------------------------------------------------------
    // Process search
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::SearchDownloadLogTask taskDownloadLog(m_szSessionToken, pSearchFilter);
	int nResult = HandleTask(&taskDownloadLog, _T("Searching download log"));

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pSearchFilter != NULL) {
        delete pSearchFilter;
        pSearchFilter = NULL;
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
        // If there are no results returned the error is caught later on...
        if (nResult != DIOMEDE_NO_MATCHES_FOUND) {
	        std::string szErrorMsg = taskDownloadLog.GetServiceErrorMsg();
	        PrintServiceError(stderr, szErrorMsg);

	        ClientLog(UI_COMP, LOG_ERROR, false,_T("Search download log failed.") );
	        return;
	    }
	}

    ClientLog(UI_COMP, LOG_STATUS, false,_T("Search download log suuccessful."));

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process results.
    //-----------------------------------------------------------------
    DownloadLogListImpl* pListDownloadLog = taskDownloadLog.GetSearchDownloadLogResults();
    std::vector<void * > listLogEntries = pListDownloadLog->GetDownloadLogList();

    // If no entries found, output a message and return.
    if ((int)listLogEntries.size() == 0) {
        PrintStatusMsg(_T("Search download log successful.  No matches found."));
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    std::string szFormattedDate = _T("");
    std::string szBytes = _T("");
    std::string szBytesSizeType = _T("");

    for (int nIndex = 0; nIndex < (int)listLogEntries.size(); nIndex ++) {

        DownloadLogEntryImpl* pLogEntry = (DownloadLogEntryImpl*)listLogEntries[nIndex];
        if (pLogEntry == NULL) {
            continue;
        }

        #ifdef WIN32
			_tprintf(_T("    Logical file ID: %I64d \n\r"), pLogEntry->GetFileID());
            _tprintf(_T("         Start byte: %I64d \n\r"), pLogEntry->GetStartByte());
        #else
            _tprintf(_T("    Logical file ID: %lld \n\r"), pLogEntry->GetFileID());
            _tprintf(_T("         Start byte: %lld \n\r"), pLogEntry->GetStartByte());
        #endif

        StringUtil::FormatByteSize(pLogEntry->GetByteCount(), szBytes, szBytesSizeType);
        _tprintf(_T("   Bytes downloaded: %s %s \n\r"), szBytes.c_str(), szBytesSizeType.c_str());

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pLogEntry->GetStartTime(), szFormattedDate);
        _tprintf(_T("  Start of download: %s \n\r"), szFormattedDate.c_str());

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pLogEntry->GetEndTime(), szFormattedDate);
        _tprintf(_T("    End of download: %s \n\r"), szFormattedDate.c_str());

        _tprintf(_T("     Download token: %s \n\r"), pLogEntry->GetDownloadeToken().c_str());
        _tprintf(_T("      Downloader IP: %s \n\r"), pLogEntry->GetDownloaderIP().c_str());
        _tprintf(_T("\n\r"));
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessSearchDownloadLogCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the search login log command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSearchLoginLogCommand(CmdLine* pCmdLine,
                                                  bool& bCommandFinished)
{
    bCommandFinished = true;

    // User must be logged in to get the login log statistics.
    if (m_bConnected == false) {
        LoggedInUserError(_T("Cannot search login log"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Search login log: you must be logged into the Diomede service."));
	    return;
    }

    DiomedeValueArg<std::string>* pStartDateArg = NULL;
    DiomedeValueArg<std::string>* pEndDateArg = NULL;
    DiomedeValueArg<std::string>* pIPArg = NULL;
    DiomedeValueArg<std::string>* pPageSizeArg = NULL;
    DiomedeValueArg<std::string>* pOffsetArg = NULL;
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pStartDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_STARTDATE);
        pEndDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_ENDDATE);
        pIPArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_IP);
        pPageSizeArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_PAGE_SIZE);
        pOffsetArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_RESULT_OFFSET);
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SEARCHLOGINS << endl;
    }

    SearchLoginLogFilterImpl* pSearchFilter = new SearchLoginLogFilterImpl;

    time_t epochSeconds;

    if (pStartDateArg && pStartDateArg->isSet()) {
        if ( ValidateDate(pStartDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateStart(epochSeconds);
        }
        else {
	        _tprintf(_T("Start date %s is not a valid date and will be ignored.\n\r"),
	            pStartDateArg->getValue().c_str());
        }
    }
    if (pEndDateArg && pEndDateArg->isSet()) {
        if ( ValidateDate(pEndDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateEnd(epochSeconds);
        }
        else {
	        _tprintf(_T("End date %s is not a valid date and will be ignored.\n\r"),
	            pEndDateArg->getValue().c_str());
        }
    }

    if (pIPArg && pIPArg->isSet()) {
        pSearchFilter->SetLoginIP(pIPArg->getValue());
    }

    LONG64 l64Value = 0L;
    if (pPageSizeArg && pPageSizeArg->isSet()) {
        l64Value = atoi64(pPageSizeArg->getValue().c_str());
        pSearchFilter->SetPageSize(l64Value);
    }
    else {
        if (GetConfigPageSize(l64Value) ) {
            pSearchFilter->SetPageSize(l64Value);
        }
    }

    if (pOffsetArg && pOffsetArg->isSet()) {
        l64Value = atoi64(pOffsetArg->getValue().c_str());
        pSearchFilter->SetOffset(l64Value);
    }
    else {
        if (GetConfigOffset(l64Value) ) {
            pSearchFilter->SetOffset(l64Value);
        }
    }

    //-----------------------------------------------------------------
    // Process search
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::SearchLoginLogTask taskLoginLog(m_szSessionToken, pSearchFilter);
	int nResult = HandleTask(&taskLoginLog, _T("Searching login log"));

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pSearchFilter != NULL) {
        delete pSearchFilter;
        pSearchFilter = NULL;
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
        // If there are no results returned the error is caught later on...
        if (nResult != DIOMEDE_NO_MATCHES_FOUND) {
	        std::string szErrorMsg = taskLoginLog.GetServiceErrorMsg();
	        PrintServiceError(stderr, szErrorMsg);

	        ClientLog(UI_COMP, LOG_ERROR, false,_T("Search login log failed.") );
	        return;
	    }
	}

    ClientLog(UI_COMP, LOG_STATUS, false,_T("Search login log suuccessful."));

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process results.
    //-----------------------------------------------------------------
    LoginLogListImpl* pLoginlLog = taskLoginLog.GetSearchLoginLogResults();
    std::vector<void * > listLogEntries = pLoginlLog->GetLoginLogList();

    // If no entries found, output a message and return.
    if ((int)listLogEntries.size() == 0) {
        PrintStatusMsg(_T("Search login log successful.  No matches found."));
        SimpleRedirect::Instance()->EndRedirect();
        return;
    }

    std::string szFormattedDate = _T("");

    for (int nIndex = 0; nIndex < (int)listLogEntries.size(); nIndex ++) {

        LoginLogEntryImpl* pLogEntry = (LoginLogEntryImpl*)listLogEntries[nIndex];

        _tprintf(_T("            Login: %s \n\r"), pLogEntry->GetUserID().c_str());

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pLogEntry->GetLoginDate(), szFormattedDate);
        _tprintf(_T("             Date: %s \n\r"), szFormattedDate.c_str());

        _tprintf(_T("               IP: %s \n\r"), pLogEntry->GetLoginIP().c_str());
        _tprintf(_T("\n\r"));
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessSearchLoginLogCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Process the search invoice log command.  User must be
//          logged into the Diomede service.
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSearchInvoiceLogCommand(CmdLine* pCmdLine,
                                                    bool& bCommandFinished)
{
    // User must be logged in to get the invoice statistics.
    if (m_bConnected == false) {
        bCommandFinished = true;
        LoggedInUserError(_T("Cannot search invoice log"));
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("Search invoice Log: you must be logged into the Diomede service."));
	    return;
    }

    bCommandFinished = true;

    DiomedeValueArg<std::string>* pStartDateArg = NULL;
    DiomedeValueArg<std::string>* pEndDateArg = NULL;
    DiomedeMultiArg<std::string>* pStatusArg = NULL;
    DiomedeValueArg<std::string>* pOutputArg = NULL;

    try {
        pStartDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_STARTDATE);
        pEndDateArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_ENDDATE);
        pStatusArg = (DiomedeMultiArg<std::string>*)pCmdLine->getArg(ARG_INVOICE_STATUS);
        pOutputArg = (DiomedeValueArg<std::string>*)pCmdLine->getArg(ARG_OUTPUT);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_SEARCHINVOICES << endl;
    }

    bool bTestOutput = TestOutput(pCmdLine);

    SearchLogFilterImpl* pSearchFilter = new SearchLogFilterImpl;

    time_t epochSeconds;

    if (pStartDateArg && pStartDateArg->isSet()) {
        if ( ValidateDate(pStartDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateStart(epochSeconds);
        }
        else {
	        _tprintf(_T("Start date %s is not a valid date and will be ignored.\n\r"),
	            pStartDateArg->getValue().c_str());
        }
    }
    if (pEndDateArg && pEndDateArg->isSet()) {
        if ( ValidateDate(pEndDateArg->getValue(), epochSeconds) ) {
            pSearchFilter->SetDateEnd(epochSeconds);
        }
        else {
	        _tprintf(_T("End date %s is not a valid date and will be ignored.\n\r"),
	            pEndDateArg->getValue().c_str());
        }
    }

    //-----------------------------------------------------------------
    // Check for selected fields
    //-----------------------------------------------------------------
    int nStatus = 0;
	std::string szField = _T("");

    if (pStatusArg && pStatusArg->isSet()) {
	    std::vector<std::string> listFields = pStatusArg->getValue();

	    for ( int nIndex = 0; static_cast<unsigned int>(nIndex) < listFields.size(); nIndex++ ) {

            szField = listFields[nIndex];

            if (szField == ARG_PAID_STATUS) {
                nStatus = DIOMEDE::paid;
            }
            if (szField == ARG_UNPAID_STATUS) {
                nStatus = DIOMEDE::unPaid;
            }
        }
    }

    pSearchFilter->SetStatus(nStatus);

    //-----------------------------------------------------------------
    // Process search
    //-----------------------------------------------------------------
	DIOMEDE_CONSOLE::SearchInvoiceLogTask taskInvoiceLog(m_szSessionToken, pSearchFilter);
	int nResult = HandleTask(&taskInvoiceLog, _T("Searching invoice log"));

    //-----------------------------------------------------------------
    // Cleanup
    //-----------------------------------------------------------------
    if (pSearchFilter != NULL) {
        delete pSearchFilter;
        pSearchFilter = NULL;
    }

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult != SOAP_OK) {
	    // Could be an error indicating no results - in that case,
	    // allow for the test option to be processed.
        if (nResult != DIOMEDE_NO_MATCHES_FOUND) {
	        std::string szErrorMsg = taskInvoiceLog.GetServiceErrorMsg();
	        PrintServiceError(stderr, szErrorMsg);

	        ClientLog(UI_COMP, LOG_ERROR, false,_T("Get invoice log failed.") );
	        return;
	    }
	}

    ClientLog(UI_COMP, LOG_STATUS, false,_T("Search invoice log suuccessful."));

    if (pOutputArg && pOutputArg->isSet()) {
        SimpleRedirect::Instance()->StartRedirect(pOutputArg->getValue() );
    }

    //-----------------------------------------------------------------
    // Process results.
    //-----------------------------------------------------------------
    InvoiceLogListImpl* pListInvoiceLogEntries = taskInvoiceLog.GetSearchInvoiceLogEntriesResults();
    std::vector<void * > listLogEntries = pListInvoiceLogEntries->GetInvoiceLogList();

    // If no entries found, output a message and return.  If the "test" switch
    // has been specified, we'll read in some fake data to test the output
    // format.
    bool bCreateNewInvoice = false;
    bool bCleanupTest = false;

    if ((int)listLogEntries.size() == 0) {
        if (bTestOutput == false) {
            PrintStatusMsg(_T("Search invoice log successful.  No invoices found."));
            SimpleRedirect::Instance()->EndRedirect();
            return;
        }
        else {
            TestSearchInvoicesData(pListInvoiceLogEntries, bCreateNewInvoice);
            listLogEntries = pListInvoiceLogEntries->GetInvoiceLogList();
            bCleanupTest = true;
        }
    }

    std::string szFormattedDate = _T("");
    std::string szStatus;

    //-----------------------------------------------------------------
    // locale facet for formatting currency
    //-----------------------------------------------------------------
    std::locale localEnglish(DiomedeMoneyPunct::GetLocaleString().c_str());
    std::locale locDiomede(localEnglish, new DiomedeMoneyPunct(2));
    cout.imbue(locDiomede);

    for (int nIndex = 0; nIndex < (int)listLogEntries.size(); nIndex ++) {

        InvoiceLogEntryImpl* pInvoiceLogEntry = (InvoiceLogEntryImpl*)listLogEntries[nIndex];
        if (pInvoiceLogEntry == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Search invoice log: NULL invoice log entry."));
            continue;
        }

        szFormattedDate = _T("");
        StringUtil::FormatDateAndTime(pInvoiceLogEntry->GetInvoiceDate(), szFormattedDate);

        //-------------------------------------------------------------
        // Invoice #: 1234567 (4/21/2008)
        //-------------------------------------------------------------
        cout << _T(" Invoice #: ") << pInvoiceLogEntry->GetInvoiceNumber()
             << _T(" (") << szFormattedDate << _T(")") << endl;

        //-------------------------------------------------------------
        //   Online Storage (2/21/08 -> 3/20/08)                    $9.14
        //-------------------------------------------------------------
        InvoiceDetailListImpl* pInvoiceDetails = pInvoiceLogEntry->GetInvoiceDetailList();

        if (pInvoiceDetails == NULL) {
            ClientLog(UI_COMP, LOG_ERROR, false,_T("Search invoice log: NULL invoice detail."));
            continue;
        }

        std::vector<void * > listInvoiceDetail = pInvoiceDetails->GetInvoiceDetailList();

        int nNumInvoices = (int)listInvoiceDetail.size();
        if (nNumInvoices == 0) {
            cout << _T("No invoices found") << endl;
            ClientLog(UI_COMP, LOG_STATUS, false,_T("Search invoice log: no invoices found."));
            continue;
        }

        int nInvoiceIndex = 0;

        for (nInvoiceIndex = 0; nInvoiceIndex < nNumInvoices; nInvoiceIndex++) {

            InvoiceDetailImpl* pDetail = (InvoiceDetailImpl*)listInvoiceDetail[nInvoiceIndex];
            if (pDetail == NULL) {
                ClientLog(UI_COMP, LOG_ERROR, false,_T("Search invoice log: NULL invoice detail found."));
                continue;
            }

            cout << setw(4) << left << _T("") << setw(56) << pDetail->GetItemDescription();

            // Invoice amount should be sent as an int*10000 - to format properly at this point,
            // convert to a double and then back to our currency amount.

            // Here we're showing 2 digit precision - so *100
            double dwAmount = (double)pDetail->GetAmount();
            cout << setw(12) << right << std::showbase << Money(dwAmount) << endl;
        }

        cout << setw(60) << left << _T("");
        cout << setw(12) << right << _T("------------") << endl;

        double dwTotalAmount = (double)pInvoiceLogEntry->GetTotalAmount();

        cout << setw(60) << right << _T("Total Amount: ");
        cout << setw(12) << right << std::showbase << Money(dwTotalAmount) << endl;

        szStatus = _T("NOT PAID");
        if (pInvoiceLogEntry->GetIsPaid()) {
           szStatus = _T("PAID");
        }

        cout << setw(60) << right << _T("Status: ");
        cout << setw(12) << right << szStatus << endl;

        cout << endl;
    }

    if (bCleanupTest) {
        TestSearchInvoicesCleanup(pListInvoiceLogEntries, bCreateNewInvoice);
    }

    SimpleRedirect::Instance()->EndRedirect();

} // End ProcessSearchInvoiceLogCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to test the search invoices output layout.
// Requires:
//      pListInvoiceLogEntries: references to a searchi invoices result list.
// Returns: nothing
void ConsoleControl::TestSearchInvoicesData(InvoiceLogListImpl* pListInvoiceLogEntries,
    bool& bCreateNewInvoice)
{
    // The returned list shouldn't be NULL, but just in case.....
    if ( pListInvoiceLogEntries == NULL) {
        pListInvoiceLogEntries = new InvoiceLogListImpl;
        bCreateNewInvoice = true;
    }
    else {
        bCreateNewInvoice = false;
    }

    //-----------------------------------------------------------------
    // Add some fake entries
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // Entry Begin
    //-----------------------------------------------------------------
    InvoiceLogEntryImpl* pInvoiceLogEntry = new InvoiceLogEntryImpl;

    ULONG ulRand = rand();
    pInvoiceLogEntry->SetInvoiceNumber(ulRand);

    std::string szInvoiceDate = _T("2008-03-21");
    time_t epochSeconds;

    // Our validation methods returns the epoch secondsd needed for the invoice date.
    ValidateDate(szInvoiceDate, epochSeconds);
    pInvoiceLogEntry->SetInvoiceDate(epochSeconds);

    pInvoiceLogEntry->SetTotalAmount(3610914);
    pInvoiceLogEntry->SetIsPaid(true);

    InvoiceDetailListImpl* pListInvoiceDetail = pInvoiceLogEntry->GetInvoiceDetailList();

    //-----------------------------------------------------------------
    // Create a couple of items with description and amount.
    //-----------------------------------------------------------------
    InvoiceDetailImpl* pInvoiceDetail = new InvoiceDetailImpl;
    pInvoiceDetail->SetItemDescription( _T("Online Storage (2/21/08 -> 3/20/08)"));
    pInvoiceDetail->SetAmount(914);

    pListInvoiceDetail->AddInvoiceDetailEntry(pInvoiceDetail);

    pInvoiceDetail = new InvoiceDetailImpl;
    pInvoiceDetail->SetItemDescription( _T("Nearline Storage (2/21/08 -> 3/20/08)"));
    pInvoiceDetail->SetAmount(10014);

    pListInvoiceDetail->AddInvoiceDetailEntry(pInvoiceDetail);

    pInvoiceDetail = new InvoiceDetailImpl;
    pInvoiceDetail->SetItemDescription( _T("Online Storage Downloads (2/21/08 -> 3/20/08)"));
    pInvoiceDetail->SetAmount(3600000);

    pListInvoiceDetail->AddInvoiceDetailEntry(pInvoiceDetail);

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    pListInvoiceLogEntries->AddInvoiceLogEntry(pInvoiceLogEntry);

    //-----------------------------------------------------------------
    // Entry End
    //-----------------------------------------------------------------

} // End TestSearchInvoicesData

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to test the search invoices output layout.
// Requires:
//      pListInvoiceLogEntries: references to a searchi invoices result list.
// Returns: nothing
void ConsoleControl::TestSearchInvoicesCleanup(InvoiceLogListImpl* pListInvoiceLogEntries,
                                               bool bCreateNewInvoice)
{
    // This helper is simplied by the fact that the Lib objects take care
    // of cleanup.
    if (bCreateNewInvoice) {
        if (pListInvoiceLogEntries) {
            delete pListInvoiceLogEntries;
            pListInvoiceLogEntries = NULL;
        }
    }

} // End TestSearchInvoicesCleanup

///////////////////////////////////////////////////////////////////////
// Purpose: Processes system commands, e.g. CTRL+C
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessSystemCommand(DioCLICommands::COMMAND_ID cmdID,
                                          CmdLine* pCmdLine, bool& bCommandFinished)
{
    std::string sysCommand = pCmdLine->getCommandName();
    bCommandFinished = true;

    switch (cmdID) {
		case DioCLICommands::CMD_CLS:
            #ifdef WIN32
                system(sysCommand.c_str());
            #else
                // 99% of non-windows terminals will accept ANSI X3.64 escape codes
                // (not a real statistic)
                printf("\x1b[2J\x1b[1;1H");     // clear screen, move cursor to top left
                fflush(stdout);                 // stdout might be line-buffered
            #endif
            break;
		case DioCLICommands::CMD_REM:
		    // Ignored the remark commands
		    break;
		default:
		    break;
    }

} // End ProcessSystemCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Handle the system ECHO command
// Requires:
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
void ConsoleControl::ProcessEchoSystemCommand(CmdLine* pCmdLine,
                                              bool& bCommandFinished)
{
    std::string sysCommand = pCmdLine->getCommandName();
    bCommandFinished = true;

    // If no arguments, show the current status of echo.
    // Otherwise, turns off or on echo.

    // This is a bit redundant - either put the parameters back together or
    // use the action stack as is...using the system command doesn't work
    // within our console - any features of echo we need to implement we'll
    // need to do ourselves..
    // nReturn = system(m_actionStack.front().c_str());

    // Get the list of arguments - this command is a multi-unlabeled argument.
    DiomedeUnlabeledValueArg<std::string>* pEchoArg = NULL;

    try {
        pEchoArg = (DiomedeUnlabeledValueArg<std::string>*)pCmdLine->getArg(ARG_ECHO);
    }
    catch (CmdLineParseException &e) {
        // catch any exceptions
        cerr << "error: " << e.error() << " for arg " << CMD_ECHO << endl;
    }

    if (pEchoArg == NULL){
        return;
    }

	std::string szEchoParam = pEchoArg->getValue();
    bCommandFinished = true;

	if (szEchoParam.length() == 0)  {
	    std::string szParam = m_bEchoOn ? _T("on") : _T("off");
	    std::cout << std::endl << "Echo is " << szParam << std::endl;
	}
	else {
	    m_bEchoOn = (0==stricmp(szEchoParam.c_str(), _T("on")));
	}

} // End ProcessEchoSystemCommand

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to manage the message timing of tasks.
// Requires:
//      pTask: reference to a CTask derived object
//      szStartMsg: optional start message
//      szEndMsg: optional end message
//      bDoneNewLine: the "done" portion is on it's own line -
//          useful for very long lines
//      bBeforeAndAfterNewLines: if true, new lines occur before
//          and after the timing message
//      bShowProgress: if true, progress is output to the console.
// Returns: SOAP_OK if successful, service error otherwise.
int ConsoleControl::HandleTask(DiomedeTask* pTask, std::string szStartMsg /*_T("")*/,
                                                   std::string szEndMsg /*_T("")*/,
                                                   bool bDoneNewLine /*false*/,
                                                   bool bBeforeAndAfterNewLines /*true*/,
                                                   bool bShowProgress /*true*/)
{
    // Make sure we can create the thread we need.
    DIOMEDE_CONSOLE::CommandThread commandThread;
    if (false == CheckThread(&commandThread, g_szTaskFriendlyName)) {
        return DIOMEDE::DIOMEDE_CREATE_THREAD_ERROR;
    }

    if (bShowProgress && bBeforeAndAfterNewLines) {
        PrintNewLine();
    }

    MessageTimer msgTimer(50, bShowProgress);
    msgTimer.Start(szStartMsg);

    BOOL bReturn = commandThread.Event(pTask);
    if (bReturn == FALSE) {

        // On the MAC, sometimes we get stuck here - the task never gets started
        // or never completes - not sure.  Adding this client log to help
        // with debugging this issue.
        ClientLog(UI_COMP, LOG_ERROR, false, _T("Thread event failed: %s, task status %d"), 
            szStartMsg.c_str(), pTask->Status());

        // NOTE: this code is very untested....
        if (pTask->Status() == TaskStatusNotSubmitted) {
            PauseProcess();
            commandThread.Event(pTask);
        }        
    }

	while ( pTask->Status() != TaskStatusCompleted ) {
        msgTimer.ContinueTime();
        PauseProcess();
	}

    commandThread.Stop();

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
    int nResult = pTask->GetResult();

    //-----------------------------------------------------------------
    // End the timer - adjust the output if an error has occurred.
    //-----------------------------------------------------------------
    bool bShowEndProgress = bShowProgress && (nResult == SOAP_OK);

    if (bShowEndProgress) {
        msgTimer.SetShowProgress(bShowEndProgress);
    }

    if (nResult != SOAP_OK){
        // Check for the service expires error - in this case, we'll end the
        // task string with "session expired".
	    std::string szErrorMsg = pTask->GetServiceErrorMsg();

        std::string szFriendlyMsg = _T("");
        if ( CheckServiceErrorToRetry(szErrorMsg, szFriendlyMsg)) {

            // The session retries is reset to MAX_RETRIES as soon as the
            // user is successfully logged back into the service.
	        g_bSessionError = true;
	        g_nSessionRetries --;

	        if (g_nSessionRetries >= 0) {
	            nResult = RepeatLastTask(pTask, &msgTimer, szStartMsg, bShowProgress);
	        }

	        // Else the error returned from the service is returned.
        }
    }

    if (bDoneNewLine) {
        msgTimer.EndTime(szEndMsg, EndTimerTypes::useNewLine);
    }
    else {
        msgTimer.EndTime(szEndMsg, EndTimerTypes::useSingleLine);
    }

    if (bShowProgress && bBeforeAndAfterNewLines && !m_bSysCommandInput) {
        PrintNewLine();
    }

    return nResult;

} // End HandleTask

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to manage the message timing of tasks.
// Requires:
//      pTask: reference to a CTask derived object
//      pMsgTimer: timer used from the original task.
//      szStartMsg: optional start message
//      bShowProgress: if true, progress is output to the console.
// Returns: SOAP_OK if successful, service error otherwise.
int ConsoleControl::RepeatLastTask(DiomedeTask* pTask, MessageTimer* pMsgTimer,
                                   std::string szStartMsg /*_T("")*/,
                                   bool bShowProgress /*true*/)
{
    int nOriginalResult = pTask->GetResult();
    int nResult = 0;
    BOOL bReturn = FALSE;

    DIOMEDE_CONSOLE::CommandThread commandThread;
    if (false == CheckThread(&commandThread, _T(""))) {
        return DIOMEDE::DIOMEDE_CREATE_THREAD_ERROR;
    }

    if (g_bSessionError) {
        g_bSessionError = false;

        // Keep the timer running, alert the user tht the original task ended
        // with session expired.
        EndTimerTypes::EndTimerType timerType = EndTimerTypes::useSingleLine;

        // From testing, we shouldn't try to relogin with generic errors - we can
        // retry, but relogin doesn't seem userful....
        if (g_nSessionErrorType == ConsoleControl::GENERIC_SERVICE) {
            timerType = EndTimerTypes::useGenericServiceError;
            pMsgTimer->EndTime(_T(""), timerType);
        }
        else {
            if ( (g_nSessionErrorType >= 0) &&
                 (g_nSessionErrorType < ConsoleControl::LAST_SERVICE_TYPES_ENUM)) {

                 if ( (g_nSessionErrorType == ConsoleControl::SESSION_TOKEN_EXPIRES) ||
                      (g_nSessionErrorType == ConsoleControl::INVALID_SESSION_TOKEN) ) {
                      timerType = EndTimerTypes::useSessionExpired;
                 }
                 else {
                      timerType = EndTimerTypes::useNetworkConnectionError;
                 }
            }

            pMsgTimer->EndTime(_T(""), timerType);

            std::string szLoginUserStart = _format(_T("Login user %s"), m_szUsername.c_str());
	        DIOMEDE_CONSOLE::LoginTask taskLogin(m_szUsername, m_szPlainTextPassword);

            pMsgTimer->Start(szLoginUserStart);
            bReturn = commandThread.Event(&taskLogin);
            if (bReturn == FALSE) {
                return nOriginalResult;
            }

	        while ( taskLogin.Status() != TaskStatusCompleted ) {
                pMsgTimer->ContinueTime();
                PauseProcess();
	        }

	        pMsgTimer->EndTime(_T(""), EndTimerTypes::useNoTimeOrDone);
            nResult = taskLogin.GetResult();

            // If this fails, just bounce out - something must be amiss with the
            // service.
            if (nResult != SOAP_OK) {
                return nResult;
            }

            m_szSessionToken = taskLogin.GetSessionToken();
            g_nSessionRetries = MAX_LOGIN_RETRIES;

	        ClientLog(UI_COMP, LOG_STATUS, false,
	            _T("Repeat last command: new session token %s."), m_szSessionToken.c_str());

            UserProfileData* pProfileData =
                ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
            if (pProfileData) {
                // If the "save" here fails, do we care?  This feature is more a nicety for the
                // user.
	            pProfileData->SetUserProfileStr(GEN_SESSION_TOKEN, m_szSessionToken.c_str());
	            pProfileData->SetUserProfileLong(GEN_SESSION_TOKEN_EXPIRES, (long)UpdateSessionTokenExpiration());
	            pProfileData->SaveUserProfile();
	        }

            PrintNewLine();
        }
    }


    pMsgTimer->Start(szStartMsg);
    pTask->ResetTask();
    pTask->SetSessionToken(m_szSessionToken);

    bReturn = commandThread.Event(pTask);
    if (bReturn == FALSE) {
        return nOriginalResult;
    }

	while ( pTask->Status() != TaskStatusCompleted ) {
        pMsgTimer->ContinueTime();
        PauseProcess();
	}

    commandThread.Stop();

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
    nResult = pTask->GetResult();

    //-----------------------------------------------------------------
    // End the timer - adjust the output if an error has occurred.
    //-----------------------------------------------------------------
    bool bShowEndProgress = bShowProgress && (nResult == SOAP_OK);

    if (bShowEndProgress) {
        pMsgTimer->SetShowProgress(bShowEndProgress);
    }

    return nResult;

} // End RepeatLastTask

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Validate the date input - should be in the form yyyy-mm-dd
// Requires:
//      szDate: date string
//      epochSeconds: converted date to epoch seconds
// Returns: true if successful, false otherwise
bool ConsoleControl::ValidateDate(std::string szDate, time_t& epochSeconds)
{

#ifndef WIN32
    struct tm tmv = { 0, 0, 0, 0, 0, 0, -1, -1, -1 };

    const char* szDateFormat = _T("%Y-%m-%d");
    if (strptime(szDate.c_str(), szDateFormat, &tmv) == 0) {
        return false;
    }

    epochSeconds = mktime(&tmv);

    struct tm utm = *gmtime(&epochSeconds);
    epochSeconds = mktime(&utm);

    return true;
#else
    const char* szDateFormat = _T("%04d-%02d-%02d");

    int nYear, nMonth, nDay;
    int nResult = sscanf(szDate.c_str(), szDateFormat, &nYear, &nMonth, &nDay);

    if (nResult == 0 || nResult == EOF) {
        return false;
    }

    struct tm tmv = { 0, 0, 0, nDay, nMonth - 1, nYear - 1900, -1, -1, -1 };
    epochSeconds = mktime(&tmv);

    struct tm utm = *gmtime(&epochSeconds);
    epochSeconds = mktime(&utm);

    return true;

    #endif

} // End ValidateDate

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Parses the date input to retrieve the year and month - should
//      be in the form yyyy-mm
// Requires:
//      szDate: date string
//      nYear: returns parsed year from the date string
//      nMonth: returns parsed month from the date string
// Returns: true if successful, false otherwise
bool ConsoleControl::ParseYearMonth(std::string szDate, int& nYear, int& nMonth)
{
#ifndef WIN32
    struct tm tmv = { 0, 0, 0, 0, 0, 0, -1, -1, -1 };

    const char* szDateFormat = _T("%Y-%m");
    if (strptime(szDate.c_str(), szDateFormat, &tmv) == 0) {
        return false;
    }

    nYear = tmv.tm_year;
    nMonth = tmv.tm_mon;
    return true;
#else
    const char* szDateFormat = _T("%04d-%02d");

    int nParsedYear, nParsedMonth;
    int nResult = sscanf(szDate.c_str(), szDateFormat, &nParsedYear, &nParsedMonth);

    if (nResult == 0 || nResult == EOF) {
        return false;
    }

    nYear = nParsedYear;
    nMonth = nParsedMonth;
    return true;

    #endif

} // End ParseYearMonth

/////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Return the file ID type - filename, ID, MD5 or SHA hash.
//	Requires:
//      szInFileID: File identifier that is one of the above.
//      nFileIDType: references to the type
//	Returns:
//		true if successful, false otherwise.
//
void ConsoleControl::GetFileIDType(std::string szInFileID, int& nFileIDType)
{
    // If there's an extension, assume a filename.  Otherwise, determine whether it's
    // md5 or SHA1 hash.  Otherwise, assume a file ID.  The actual download requires
    // a file ID - we'll use the filename or hash to get the file ID for the file.
    std::string szExtension = _T("");
    bool bIsFileName = GetFileExtension(szInFileID, szExtension);

    if (bIsFileName) {
        nFileIDType = ConsoleControl::fileType;
        return;
    }

    nFileIDType = ConsoleControl::unknownType;

    if (!bIsFileName) {
        // Is it a valid hex string?
        if (VerifyIsHex(szInFileID) == true) {
            // MD5 - 32 hex, 120 bits
            if (szInFileID.length() == 32) {
                nFileIDType = ConsoleControl::md5Type;
            }
            else if (szInFileID.length() == 40) {
                // SHA1 = 40 hex, 160 bits
                nFileIDType = ConsoleControl::sha1Type;
            }
        }

        // If we still need figure out what the input is, try again.
        if (nFileIDType == ConsoleControl::unknownType) {
            if (VerifyIsDigit(szInFileID) == false) {
                // If it's not all digits, assume that, due to the
                // failure to find a valid length, that it's a filename.
                nFileIDType = ConsoleControl::fileType;
            }
        }
    }

    // If it's not a filename or hash, assume it's a file ID.
    if (nFileIDType == ConsoleControl::unknownType) {
        nFileIDType = ConsoleControl::fileIDType;
    }

} // End GetFileIDType

/////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Return the file extension from the given file path.
//	Requires:
//      szFilePath: File path
//      szExtension; returned file extension.  If none found, ".txt" is assumed.
//	Returns:
//		true if successful, false otherwise.
//
bool ConsoleControl::GetFileExtension(std::string szFilePath, std::string& szExtension)
{
    bool bSuccess = false;

	int nPos = (int)szFilePath.find_last_of(_T("."));

	/*
    ClientLog(UI_COMP, LOG_STATUS, false, _T("GetFileExtension for file %s: nPos %d"),
        szFilePath.c_str(), nPos);
   */

	if ( (nPos > -1) && (nPos < (int)szFilePath.size()) ) {
		szExtension = szFilePath.substr(nPos, szFilePath.size() - 1 );
		bSuccess = true;
	}

	return bSuccess;

} // End GetFileExtension

////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Validates the credit card number input for billing data.
//	    Checks whether a string of digits is a valid credit card number
//	    according to the Luhn algorithm.
//
//	    1. Starting with the second to last digit and moving left, double the value
//	       of all the alternating digits. For any digits that thus become 10 or more,
//	       add their digits together. For example, 1111 becomes 2121, while 8763
//	       becomes 7733 (from (1+6)7(1+2)3).
//
//	    2. Add all these digits together. For example, 1111 becomes 2121, then
//	       2+1+2+1 is 6; while 8763 becomes 7733, then 7+7+3+3 is 20.
//
//	    3. If the total ends in 0 (put another way, if the total modulus 10 is 0),
//	       then the number is valid according to the Luhn formula, else it is not
//	       valid. So, 1111 is not valid (as shown above, it comes out to 6), while
//	       8763 is valid (as shown above, it comes out to 20).
//	Requires:
//      szCredCardNumber: credit card number.
//	Returns:
//		true if successful, false otherwise.
//
bool ConsoleControl::ValidateCreditCard(const char* szCredCardNumber)
{
    bool bSuccess = false;

    int nLength, nIndex, nAlternate, nSum;

    if (!szCredCardNumber) {
        return false;
    }

    // For testing....
    if (0==strcmp(szCredCardNumber, _T("1111111111111914"))) {
        return true;
    }

    nLength = strlen(szCredCardNumber);

    if (nLength < 13 || nLength > 19) {
        return false;
    }

    for (nAlternate = 0, nSum = 0, nIndex = nLength - 1; nIndex > -1; --nIndex) {

        if (!isdigit(szCredCardNumber[nIndex])) {
            return false;
        }

        nLength = szCredCardNumber[nIndex] - '0';

        if (nAlternate) {
            nLength *= 2;
            if (nLength > 9) {
                nLength = (nLength % 10) + 1;
            }
        }

        nAlternate = !nAlternate;
        nSum += nLength;
    }

    bSuccess = (nSum % 10 == 0);
	return bSuccess;

} // End ValidateCreditCard

////////////////////////////////////////////////////////////////////////////
// Purpose:
//      Validates the CVV number input for billing data.
//	Requires:
//      szCVV: CVV number.
//	Returns:
//		true if successful, false otherwise.
//
bool ConsoleControl::ValidateCVV(const std::string szCVV)
{
    bool bSuccess = true;
    int nLength = 0;

    nLength = szCVV.length();

    if (nLength < 2 || nLength > 4) {
        return false;
    }

    int nPos = strcspn(szCVV.c_str(), _T("^\\d*$"));
    if ( nPos != nLength ) {
        bSuccess = false;
    }

	return bSuccess;

} // End ValidateCVV

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Setup the command handling.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupCommands()
{
    // If input is redirected from a file, all arguments that are
    // optionally, now are required (e.g no prompting from the batch file).
    // Alternatively, (TBD) read the data in as key/value pairs.

    // Add the commands - once we finalize their structure, arguments, the commands
    // can be read in from some sort of collection to make this a more automated
    // process.  At this point, the format is sketchy...
    try {

        DiomedeUnlabeledValueArg<std::string>* pArg = NULL;
        DiomedeUnlabeledMultiArg<std::string>* pMultiArg = NULL;

        DiomedeValueArg<std::string>* pValueArg = NULL;
        DiomedeMultiArg<std::string>* pMultiValueArg = NULL;

        DiomedeSwitchArg* pSwitchArg = NULL;

        CmdLine* pCmdLine = NULL;

        //-------------------------------------------------------------
        // Exit
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_EXIT,
            _T("Exit the Diomede Command Line interface."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        GetAltCommandStrs(CMD_EXIT, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_EXIT, pCmdLine));

        //-------------------------------------------------------------
        // Help
        // Example usage: >help login
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_HELP,
            _T("Show usage information for DioCLI commands."), ' ', m_bRedirectedInput,
            m_szAppVersion.c_str(), false);
        pCmdLine->setOutput(&m_stdOut);

        // Define a help value argument and add it to the command line.
        UnlabledValueStrArg* helpArg = new UnlabledValueStrArg(pCmdLine,
            CMD_HELP,
            _T("Show help for the given command"), false, _T("help"),
            _T("help command"));
        pCmdLine->add( helpArg );
        pCmdLine->deleteOnExit( helpArg );

        #if 0
        // For testing OptionalUnlabeledTracker class, set this ifdef to 1.
        UnlabledValueStrArg* valueArg = new UnlabledValueStrArg(pCmdLine,
        DioCLICommands::CMD_EXIT,
            "Exit the Diomede command line interface.", false, _T("string"),
            _T("exit the application"));
        pCmdLine->add( valueArg );
        pCmdLine->deleteOnExit( valueArg );
        #endif

        GetAltCommandStrs(CMD_HELP, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_HELP, pCmdLine));

        //-------------------------------------------------------------
        // Help
        // Example usage: >help login
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_ABOUT,
            _T("Shows general information about DioCLI."), ' ', m_bRedirectedInput,
            m_szAppVersion.c_str(), false);
        pCmdLine->setOutput(&m_stdOut);

        GetAltCommandStrs(CMD_ABOUT, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_ABOUT, pCmdLine));

        //-------------------------------------------------------------
        // Login
        // Example usage: >login john password
        //-------------------------------------------------------------
        SetupLoginCommand();

        //-------------------------------------------------------------
        // Logout
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_LOGOUT,
            _T("Logs out from the Diomede service."), ' ', m_bRedirectedInput,
            m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        GetAltCommandStrs(CMD_LOGOUT, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_LOGOUT, pCmdLine));

        //-------------------------------------------------------------
        // Getsessiontoken
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_SESSIONTOKEN,
            _T("Returns the current session token."), ' ', m_bRedirectedInput,
            m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        GetAltCommandStrs(CMD_SESSIONTOKEN, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SESSIONTOKEN, pCmdLine));

        //-------------------------------------------------------------
        // Setconfig
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_SETCONFIG,
            _T("Set a configuration option."),
            ' ', m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        // Define a auto login (on or off) value argument and add it to the command line.
        std::vector<std::string> allowedLoginArgs;
        allowedLoginArgs.push_back(ARG_ON);
        allowedLoginArgs.push_back(ARG_OFF);

        ValuesConstraint<std::string>* pAllowedLoginVals =
            new ValuesConstraint<std::string>( allowedLoginArgs );

        pValueArg = new DiomedeValueArg<std::string>(ARG_AUTOLOGIN,
            ARG_AUTOLOGIN,
            _T("Turn off or on auto login."), false, _T(""),
            pAllowedLoginVals /*_T("auto login argument")*/);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        // Define a auto logout (on or off) value argument and add it to the command line.
        std::vector<std::string> allowedLogoutArgs;
        allowedLogoutArgs.push_back(ARG_ON);
        allowedLogoutArgs.push_back(ARG_OFF);

        ValuesConstraint<std::string>* pAllowedLogoutVals =
            new ValuesConstraint<std::string>( allowedLogoutArgs );

        pValueArg = new DiomedeValueArg<std::string>(ARG_AUTOLOGOUT,
            ARG_AUTOLOGOUT,
            _T("Turn off or on auto logout."), false, _T(""),
            pAllowedLogoutVals /*_T("auto logout argument")*/);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        // Define a check account (on or off) value argument.
        std::vector<std::string> allowedCheckAccountArgs;
        allowedCheckAccountArgs.push_back(ARG_ON);
        allowedCheckAccountArgs.push_back(ARG_OFF);

        ValuesConstraint<std::string>* pAllowedCheckAccountVals =
            new ValuesConstraint<std::string>( allowedCheckAccountArgs );

        pValueArg = new DiomedeValueArg<std::string>(ARG_CHECK_ACCOUNT,
            ARG_CHECK_ACCOUNT,
            _T("Checks account subscription status on login."), false, _T(""),
            pAllowedCheckAccountVals /*_T("check account argument")*/);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_PAGE_SIZE,
            ARG_RESULT_PAGE_SIZE,
            _T("Set result page size."), false, _T(""),
            _T("page size"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_OFFSET,
            ARG_RESULT_OFFSET,
            _T("Set result page offset."), false, _T(""),
            _T("page offset"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        std::vector<std::string> allowedSSLModeArgs;
        allowedSSLModeArgs.push_back(ARG_YES);
        allowedSSLModeArgs.push_back(ARG_NO);
        allowedSSLModeArgs.push_back(ARG_ALL);

        ValuesConstraint<std::string>* pAllowedSSLModeVals =
            new ValuesConstraint<std::string>( allowedSSLModeArgs );

        pValueArg = new DiomedeValueArg<std::string>(ARG_USE_SSL,
            ARG_USE_SSL,
            _T("SSL mode."), false, _T(""),
            pAllowedSSLModeVals /*_T("ssl mode (default, all, none)")*/);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_SERVICE_ENDPOINT,
            ARG_SERVICE_ENDPOINT,
            _T("Service endpoint."), false, _T(""),
            _T("service endpoint"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_SSL_SERVICE_ENDPOINT,
            ARG_SSL_SERVICE_ENDPOINT,
            _T("SSL service endpoint."), false, _T(""),
            _T("ssl service endpoint"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_TRANSFER_ENDPOINT,
            ARG_TRANSFER_ENDPOINT,
            _T("Transfer endpoint."), false, _T(""),
            _T("transfer endpoint"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_SSL_TRANSFER_ENDPOINT,
            ARG_SSL_TRANSFER_ENDPOINT,
            _T("SSL transfer endpoint."), false, _T(""),
            _T("ssl transfer endpoint"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        #ifdef WIN32
            // Currently this argument is allowed on Windows only.
            std::vector<std::string> allowedClipboardArgs;
            allowedClipboardArgs.push_back(ARG_ON);
            allowedClipboardArgs.push_back(ARG_OFF);

            ValuesConstraint<std::string>* pAllowedClipboardVals =
                new ValuesConstraint<std::string>( allowedClipboardArgs );

            pValueArg = new DiomedeValueArg<std::string>(ARG_CLIPBOARD,
                ARG_CLIPBOARD,
                _T("Turn off or on copying URLs to the clipboard."), false, _T(""),
                pAllowedClipboardVals /*_T("clipboard argument")*/);
            pCmdLine->add( pValueArg );
            pCmdLine->deleteOnExit( pValueArg );
        #endif

        std::vector<std::string> allowedVerboseArgs;
        allowedVerboseArgs.push_back(ARG_ON);
        allowedVerboseArgs.push_back(ARG_OFF);

        ValuesConstraint<std::string>* pAllowedVerboseVals =
            new ValuesConstraint<std::string>( allowedVerboseArgs );

        pValueArg = new DiomedeValueArg<std::string>(ARG_VERBOSE,
            ARG_VERBOSE,
            _T("Turn off or on verbose output."), false, _T(""),
            pAllowedVerboseVals /*_T("verbose output argument")*/);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        GetAltCommandStrs(CMD_SETCONFIG, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SETCONFIG, pCmdLine));

       //-------------------------------------------------------------
        // Createuser
        // Example usage:
        //    >createuser john password /company="John Co"
        //    > email: john@someplace.com
        //    > first name: john
        //    > last name: smith
        // Example usage:
        //    >createuser john password john@someplace.com john smith "222 Home Lane" /company="John Co"
        //-------------------------------------------------------------
        SetupCreateUserCommand();

        //-------------------------------------------------------------
        // Changepassword
        // Example usage:
        //    >changepassword password1 password2
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_CHANGEPASSWORD,
            _T("Change your password."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_OLDPASSWORD,
            _T("Old password."),
            true, _T(""), _T("old password"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_NEWPASSWORD,
            _T("New password."), true, _T(""),
            _T("new password"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        GetAltCommandStrs(CMD_CHANGEPASSWORD, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_CHANGEPASSWORD, pCmdLine));

        //-------------------------------------------------------------
        // Resetpassword
        // Example usage:
        //    >Resetpassword bill
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_RESETPASSWORD,
            _T("Reset your password."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_LOGIN_USERNAME,
        _T("User's username for logging into the Diomede service."),
        true, _T(""), _T("login user name"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        GetAltCommandStrs(CMD_RESETPASSWORD, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_RESETPASSWORD, pCmdLine));

       //-------------------------------------------------------------
        // Deleteuser
        // Example usage (deletes the logged in user):
        //    >deleteuser
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_DELETEUSER, _T("Delete the currently logged in user."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        GetAltCommandStrs(CMD_DELETEUSER, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_DELETEUSER, pCmdLine));

        //-------------------------------------------------------------
        // Setuserinfo
        // NOTE: Arguments are optional, and therefore required the
        //       /arg=<value> syntax.  Username and password are
        //       no longer valid inputs (password is changed via
        //       the (TBD) command.
        // Example usage:
        //    > updateuserinfo /email=john@someplace.com
        //-------------------------------------------------------------
        SetupSetUserInfoCommand();

        //-------------------------------------------------------------
        // Getuserinfo
        // Example usage (returns the user info for the logged in user):
        //    >getuserinfo
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_GETUSERINFO,
            _T("Returns your user information."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        GetAltCommandStrs(CMD_GETUSERINFO, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETUSERINFO, pCmdLine));

        //-------------------------------------------------------------
        // Deleteuserinfo
        // Example usage (needs to be reworked to delete specific fields):
        //    >deleteuserinfo /company
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_DELETEUSERINFO,
            _T("Delete your user information field(s)."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        // Define multiple /key=value options
        std::vector<std::string> alloweDeleteArgs;
        alloweDeleteArgs.push_back(ARG_FIRSTNAME);
        alloweDeleteArgs.push_back(ARG_LASTNAME);
        alloweDeleteArgs.push_back(ARG_COMPANY);
        alloweDeleteArgs.push_back(ARG_WEBURL);
        alloweDeleteArgs.push_back(ARG_PHONE);

        ValuesConstraint<std::string>* pAllowedDeleteVals =
            new ValuesConstraint<std::string>( alloweDeleteArgs );

        pMultiValueArg = new DiomedeMultiArg<std::string>(ARG_DELETEUSERINFO,
            ARG_DELETEUSERINFO,
            _T("Delete one or more user info fields."), false,
            pAllowedDeleteVals /*_T("delete user info command argument")*/);
        pCmdLine->add( pMultiValueArg );
        pCmdLine->deleteOnExit( pMultiValueArg );

        GetAltCommandStrs(CMD_DELETEUSERINFO, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_DELETEUSERINFO, pCmdLine));

        //-------------------------------------------------------------
        // Getemailaddresses
        // Example usage (returns the email addressses for the logged in
        //                user):
        //    >getemailaddresses
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_GETEMAILADDRESSES, _T("Returns your email addresses."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        GetAltCommandStrs(CMD_GETEMAILADDRESSES, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETEMAILADDRESSES, pCmdLine));

        //-------------------------------------------------------------
        // Addemailaddress
        // Example usage: >addemailaddress bob@myweb.com
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_ADDEMAILADDRESS,
            _T("Add an email for the currently logged in user."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_EMAIL,
            _T("Email address to add to the logged in user."),
            true, _T(""), _T("email address"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        GetAltCommandStrs(CMD_ADDEMAILADDRESS, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_ADDEMAILADDRESS, pCmdLine));

        //-------------------------------------------------------------
        // Deleteemailaddress
        // Example usage: >removeemailaddress bob@myweb.com
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_DELETEEMAILADDRESS,
            _T("Deletes an email from the currently logged in user."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_EMAIL,
            _T("Email address to delete from the logged in user."),
            true, _T(""), _T("email address"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        GetAltCommandStrs(CMD_DELETEEMAILADDRESS, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_DELETEEMAILADDRESS, pCmdLine));

        //-------------------------------------------------------------
        // Setprimaryemailaddress
        // Example usage: >setprimaryemailaddress bob@myweb.com
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_SETPRIMARYEMAILADDRESS,
            _T("Set your primary email address."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_EMAIL,
            _T("Set the primary email address for the logged in user."),
            true, _T(""), _T("email address"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        GetAltCommandStrs(CMD_SETPRIMARYEMAILADDRESS, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SETPRIMARYEMAILADDRESS, pCmdLine));

        //-------------------------------------------------------------
        // Checkaccount
        // Example usage (returns the subscription data for the logged in
        //         user):
        //    >checkaccount
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_CHECKACCOUNT,
            _T("Checks account subscription status on login."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        GetAltCommandStrs(CMD_CHECKACCOUNT, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_CHECKACCOUNT, pCmdLine));

        //-------------------------------------------------------------
        // Subscribe
        // Example usage:
        //    > Subscribe "john smith" 112233445566  "07/08/2010" 589 /address2="Apt. 3B"
        //    > address1: 1234 My Address
        //    > city: My City
        //    > state: CA
        //    > country: USA
        // Followed by automatic purchase of product 1
        //-------------------------------------------------------------
        SetupSetBillingDataCommand(CMD_SUBSCRIBE, DioCLICommands::CMD_SUBSCRIBE,
                                   _T("Calls SETBILLINGINFO and PURCHASEPRODUCT."));

        //-------------------------------------------------------------
        // Setbillingdata
        // Example usage:
        //    > Setbillingdata "john smith" 112233445566  "07/08/2010" 589 /address2="Apt. 3B"
        //    > address1: 1234 My Address
        //    > city: My City
        //    > state: CA
        //    > country: USA
        //-------------------------------------------------------------
        SetupSetBillingDataCommand(CMD_SETBILLINGINFO, DioCLICommands::CMD_SETBILLINGINFO,
                                   _T("Set your billing information."));

        //-------------------------------------------------------------
        // Getbillingdata
        // Example usage (returns the billing data for the logged in user):
        //    >getbillingdata
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_GETBILLINGINFO,
            _T("Return the billing data for the current user."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        GetAltCommandStrs(CMD_GETBILLINGINFO, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETBILLINGINFO, pCmdLine));

        //-------------------------------------------------------------
        // Deletebillingdata
        // Example usage (needs to be reworked to delete specific fields):
        //    >deletebillingdata /company
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_DELETEBILLINGINFO,
            _T("Delete the user info field for the current user."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        // Define multiple /key=value options
        std::vector<std::string> alloweDeleteBillingArgs;
        alloweDeleteBillingArgs.push_back(ARG_BILLING_NAME);
        alloweDeleteBillingArgs.push_back(ARG_BILLING_NUMBER);
        alloweDeleteBillingArgs.push_back(ARG_BILLING_EXPIRES);
        alloweDeleteBillingArgs.push_back(ARG_BILLING_CVV);
        alloweDeleteBillingArgs.push_back(ARG_BILLING_ADDRESS1);
        alloweDeleteBillingArgs.push_back(ARG_BILLING_ADDRESS2);
        alloweDeleteBillingArgs.push_back(ARG_BILLING_CITY);
        alloweDeleteBillingArgs.push_back(ARG_BILLING_STATE);
        alloweDeleteBillingArgs.push_back(ARG_BILLING_ZIP);
        alloweDeleteBillingArgs.push_back(ARG_BILLING_COUNTRY);

        ValuesConstraint<std::string>* pAllowedDeleteBillingVals =
            new ValuesConstraint<std::string>( alloweDeleteBillingArgs );

        pMultiValueArg = new DiomedeMultiArg<std::string>(ARG_DELETEBILLINGINFO,
            ARG_DELETEBILLINGINFO,
            _T("Delete one or more billing data fields."), false,
            pAllowedDeleteBillingVals /*_T("delete billing data command argument")*/);
        pCmdLine->add( pMultiValueArg );
        pCmdLine->deleteOnExit( pMultiValueArg );

        GetAltCommandStrs(CMD_DELETEBILLINGINFO, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_DELETEBILLINGINFO, pCmdLine));

        //-------------------------------------------------------------
        // SearchPayments
        // Example usage: >addemailaddress bob@myweb.com
        //-------------------------------------------------------------
        #if 0
        // 6/1/2010: Add this back in once the bugs have been fixed...
        pCmdLine = new CmdLine(CMD_SEARCHPAYMENTS,
            _T("Search the payment log."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_STARTDATE,
            _T("Payment log search start date (yyyy-mm-dd)."),
            true, _T(""), _T("payment log search start date"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_ENDDATE,
            _T("Payment log search end date (yyyy-mm-dd)."),
            true, _T(""), _T("payment log search end date"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
            ARG_OUTPUT,
            _T("Direct search results to a file."), false, _T(""),
            _T("output filename"));
        pValueArg->useLowerCase(false);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        GetAltCommandStrs(CMD_SEARCHPAYMENTS, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SEARCHPAYMENTS, pCmdLine));
        #endif

        //-------------------------------------------------------------
        // Upload
        // Example usage: >upload file1.txt file2.doc file3.bmp
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_UPLOAD,
            _T("Upload files to Diomede."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

	    pSwitchArg = new DiomedeSwitchArg(ARG_RECURSE_SWITCH,
	        ARG_RECURSE_SWITCH, "Recurse through subdirectories", false);
        pCmdLine->add( pSwitchArg );
        pCmdLine->deleteOnExit( pSwitchArg );

	    pSwitchArg = new DiomedeSwitchArg(ARG_PATHMETADATA_SWITCH,
	        ARG_PATHMETADATA_SWITCH, "Add the full and relative path as metadata", false);
        pCmdLine->add( pSwitchArg );
        pCmdLine->deleteOnExit( pSwitchArg );

	    pSwitchArg = new DiomedeSwitchArg(ARG_HASHMD5_SWITCH,
	        ARG_HASHMD5_SWITCH, "Create the MD5 digest used for the upload.", false);
        pCmdLine->add( pSwitchArg );
        pCmdLine->deleteOnExit( pSwitchArg );

        // IMPORTANT: UnlabeledMultiArg's must be the last
        // argument added to a command - otherwise, for example,
        // the above switches are not parsed correctly.
        pMultiArg = new DiomedeUnlabeledMultiArg<std::string>(pCmdLine,
            ARG_FILENAME,
            _T("List of files for uploading to Diomede."),
            true, _T(""), _T("upload files"));

        // For files, we'll need to keep the data case correct.  This is
        // particular important in Linux.
        pMultiArg->useLowerCase(false);
        pCmdLine->add( pMultiArg );
        pCmdLine->deleteOnExit( pMultiArg );

        GetAltCommandStrs(CMD_UPLOAD, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_UPLOAD, pCmdLine));

        //-------------------------------------------------------------
        // Resume
        // Example usage: >resume file1.txt
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_RESUME,
            _T("Resume uploading files to Diomede."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pSwitchArg = new DiomedeSwitchArg(ARG_RESUME_LIST_SWITCH,
            ARG_RESUME_LIST_SWITCH, "List the contents of resumable uploads.", false);
        pCmdLine->add( pSwitchArg );
        pCmdLine->deleteOnExit( pSwitchArg );

        pSwitchArg = new DiomedeSwitchArg(ARG_RESUME_CLEAR_SWITCH,
            ARG_RESUME_CLEAR_SWITCH, "Clear the list of resumable uploads.", false);
        pCmdLine->add( pSwitchArg );
        pCmdLine->deleteOnExit( pSwitchArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
            ARG_OUTPUT,
            _T("Direct resume list to a file."), false, _T(""),
            _T("output filename"));
        pValueArg->useLowerCase(false);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pSwitchArg = new DiomedeSwitchArg(ARG_VERBOSE_SWITCH,
            ARG_VERBOSE_SWITCH, "Specify verbose output", false);
        pCmdLine->add( pSwitchArg );
        pCmdLine->deleteOnExit( pSwitchArg );

        // IMPORTANT: UnlabeledMultiArg's must be the last
        // argument added to a command - otherwise, for example,
        // the above switches are not parsed correctly.
        pMultiArg = new DiomedeUnlabeledMultiArg<std::string>(pCmdLine,
            ARG_FILENAME,
            _T("List of files for resuming upload."),
            true, _T(""), _T("resume files"));

        // For files, we'll need to keep the data case correct.  This is
        // particular important in Linux.
        pMultiArg->useLowerCase(false);
        pCmdLine->add( pMultiArg );
        pCmdLine->deleteOnExit( pMultiArg );

        GetAltCommandStrs(CMD_RESUME, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_RESUME, pCmdLine));

        //-------------------------------------------------------------
        // Download
        // Example usage: >upload c:\myfile.txt
        //                >download 12  (where 12 is the file ID from above)
        //                >download myfile.txt
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_DOWNLOAD,
            _T("Download a file from Diomede."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_FILEINFO,
            _T("Filename, file ID, or hash value."),
            true, _T(""), _T("file identifier"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_DIRECTORY,
            ARG_DIRECTORY,
            _T("Download output directory."), false, _T(""),
            _T("directory path"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        GetAltCommandStrs(CMD_DOWNLOAD, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_DOWNLOAD, pCmdLine));

        //-------------------------------------------------------------
        // Getdownloadurl
        // Example usage: >getdownloadurl <fileid>
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_GETDOWNLOADURL,
            _T("Returns a url for downloading a file."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_FILEID,
            _T("File ID for the file."),
            true, _T(""), _T("token file"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_MAXDOWNLOADS,
            ARG_MAXDOWNLOADS,
            _T("Maximum downloads allowed for this URL."), false, _T(""),
            _T("maximum downloads"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_LIFETIMEHOURS,
            ARG_LIFETIMEHOURS,
            _T("Lifetime hours for this URL."), false, _T(""),
            _T("lifetime hours"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_MAXUNIQUEIPS,
            ARG_MAXUNIQUEIPS,
            _T("Maximum unique IPs allowed for this URL."), false, _T(""),
            _T("maximum IPs"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_ERRORREDIRECT,
            ARG_ERRORREDIRECT,
            _T("Redirect to this URL on error."), false, _T(""),
            _T("URL"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        GetAltCommandStrs(CMD_GETDOWNLOADURL, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETDOWNLOADURL, pCmdLine));

        //-------------------------------------------------------------
        // GetUploadToken
        // Example usage: >getuploadtoken file1.txt /bytes=2000 /tier=1
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_GETUPLOADTOKEN,
            _T("Returns a token to use for alternate upload methods."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_FILENAME,
            _T("File to associate with the token."),
            true, _T(""), _T("token file"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_TOKEN_BYTES,
            ARG_TOKEN_BYTES,
            _T("Bytes reserved for the upload."), false, _T(""),
            _T("bytes reserved"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        GetAltCommandStrs(CMD_GETUPLOADTOKEN, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETUPLOADTOKEN, pCmdLine));

        //-------------------------------------------------------------
        // Searchfiles
        // Example usage: >searchfile (returns all files)
        //                >searchfile /fileid=22
        //-------------------------------------------------------------
        SetupSearchFilesCommand();

        //-------------------------------------------------------------
        // Searchfilestotal
        // Example usage (returns file totals (bytes, power, etc.)
        //    >searchfilestotal
        //-------------------------------------------------------------
        SetupSearchFilesTotalCommand();

        //-------------------------------------------------------------
        // Searchfilestotallog
        // Example usage (returns upload, download couts, etc.)
        //    >searchfilestotallog
        //-------------------------------------------------------------
        SetupSearchFilesTotalLogCommand();

        //-------------------------------------------------------------
        // Renamefile
        // Example usage: >renamefile 33 test2.doc
        //                >rename test2.doc test3.doc
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_RENAMEFILE,
            _T("Renames a file."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_FILEID,
            _T("File identifier (name, ID, hash) of the original file."),
            true, _T(""), _T("file identifier"));

        // For files, we'll need to keep the data case correct.  This is
        // particular important in Linux.
        pArg->useLowerCase(false);
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_FILENAME,
            _T("New file name for the file."), true, _T(""),
            _T("new file name"));
        pArg->useLowerCase(false);
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        GetAltCommandStrs(CMD_RENAMEFILE, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_RENAMEFILE, pCmdLine));

        //-------------------------------------------------------------
        // Deletefile
        // Example usage: >deletefile 33
        //                >deletefile test2.doc
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_DELETEFILE,
            _T("Deletes a file."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_FILEINFO,
            _T("File identifier (name, ID, hash) of the original file."),
            true, _T(""), _T("file identifier"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        GetAltCommandStrs(CMD_DELETEFILE, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_DELETEFILE, pCmdLine));

        //-------------------------------------------------------------
        // Undeletefile
        // Example usage: >undeletefile 33
        //                >undeletefile test2.doc
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_UNDELETEFILE,
            _T("Undeletes a file."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_FILEINFO,
            _T("File identifier (name, ID, hash) of the original file."),
            true, _T(""), _T("file identifier"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        GetAltCommandStrs(CMD_UNDELETEFILE, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_UNDELETEFILE, pCmdLine));

        //-------------------------------------------------------------
        // Cat (e.g. dispaly file contents)
        // Example usage: >cat c:\myfile.txt
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_DISPLAYFILE,
            _T("Display a range of file contents to stdout."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_FILEINFO,
            _T("Filename, file ID, or hash value."),
            true, _T(""), _T("file identifier"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_STARTBYTE,
            ARG_STARTBYTE,
            _T("Display file contents beginning byte."), false, _T(""),
            _T("start byte"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        pValueArg = new DiomedeValueArg<std::string>(ARG_ENDBYTE,
            ARG_ENDBYTE,
            _T("Display file contents ending byte."), false, _T(""),
            _T("end date"));
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        GetAltCommandStrs(CMD_DISPLAYFILE, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_DISPLAYFILE, pCmdLine));

        //-------------------------------------------------------------
        // Setup the metadata related commands
        //-------------------------------------------------------------
        SetupMetaDataCommands();

        //-------------------------------------------------------------
        // Setup the replication related commands
        //-------------------------------------------------------------
        SetupReplicationCommands();

        //-------------------------------------------------------------
        // Setup the replication policy related commands
        //-------------------------------------------------------------
        SetupReplicationPolicyCommands();

        //-------------------------------------------------------------
        // Getallproducts
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_GETALLPRODUCTS,
            _T("Returns all the Diomede products."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
            ARG_OUTPUT,
            _T("Direct search results to a file."), false, _T(""),
            _T("output filename"));
        pValueArg->useLowerCase(false);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        GetAltCommandStrs(CMD_GETALLPRODUCTS, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETALLPRODUCTS, pCmdLine));

        //-------------------------------------------------------------
        // Purchaseproduct
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_PURCHASEPRODUCT,
            _T("Purchase a Diomede product."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pMultiArg = new DiomedeUnlabeledMultiArg<std::string>(pCmdLine,
            ARG_PRODID,
            _T("Diomede product ID."),
            true, _T(""), _T("product identifier"));
        pCmdLine->add( pMultiArg );
        pCmdLine->deleteOnExit( pMultiArg );

        GetAltCommandStrs(CMD_PURCHASEPRODUCT, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_PURCHASEPRODUCT, pCmdLine));

        //-------------------------------------------------------------
        // Getmyproducts
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_GETMYPRODUCTS,
            _T("Returns the logged in user's Diomede products."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
            ARG_OUTPUT,
            _T("Direct search results to a file."), false, _T(""),
            _T("output filename"));
        pValueArg->useLowerCase(false);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        GetAltCommandStrs(CMD_GETMYPRODUCTS, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETMYPRODUCTS, pCmdLine));

        //-------------------------------------------------------------
        // Cancelproduct
        // Example usage: >cancelproduct 2
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_CANCELPRODUCT,
            _T("Cancels a product."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pMultiArg = new DiomedeUnlabeledMultiArg<std::string>(pCmdLine,
            ARG_PRODID,
            _T("Diomede product ID."),
            true, _T(""), _T("product identifier"));
        pCmdLine->add( pMultiArg );
        pCmdLine->deleteOnExit( pMultiArg );

        GetAltCommandStrs(CMD_CANCELPRODUCT, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_CANCELPRODUCT, pCmdLine));

        //-------------------------------------------------------------
        // Getallcontracts
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_GETALLCONTRACTS,
            _T("Returns all the Diomede contracts."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
            ARG_OUTPUT,
            _T("Direct search results to a file."), false, _T(""),
            _T("output filename"));
        pValueArg->useLowerCase(false);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        GetAltCommandStrs(CMD_GETALLCONTRACTS, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETALLCONTRACTS, pCmdLine));

        //-------------------------------------------------------------
        // Purchasecontract
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_PURCHASECONTRACT,
            _T("Purchase a Diomede contract."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pMultiArg = new DiomedeUnlabeledMultiArg<std::string>(pCmdLine,
            ARG_CONTRACTID,
            _T("Diomede contract ID."),
            true, _T(""), _T("contract identifier"));
        pCmdLine->add( pMultiArg );
        pCmdLine->deleteOnExit( pMultiArg );

        GetAltCommandStrs(CMD_PURCHASECONTRACT, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_PURCHASECONTRACT, pCmdLine));

         //-------------------------------------------------------------
        // Getmycontracts
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_GETMYCONTRACTS,
            _T("Returns the logged in user's Diomede contracts."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
            ARG_OUTPUT,
            _T("Direct search results to a file."), false, _T(""),
            _T("output filename"));
        pValueArg->useLowerCase(false);
        pCmdLine->add( pValueArg );
        pCmdLine->deleteOnExit( pValueArg );

        GetAltCommandStrs(CMD_GETMYCONTRACTS, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETMYCONTRACTS, pCmdLine));

       //-------------------------------------------------------------
        // Cancelcontract
        // Example usage: >cancelcontract 2
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_CANCELCONTRACT,
            _T("Cancels a contract."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        pMultiArg = new DiomedeUnlabeledMultiArg<std::string>(pCmdLine,
            ARG_CONTRACTID,
            _T("Diomede contract ID."),
            true, _T(""), _T("contract identifier"));
        pCmdLine->add( pMultiArg );
        pCmdLine->deleteOnExit( pMultiArg );

        GetAltCommandStrs(CMD_CANCELCONTRACT, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_CANCELCONTRACT, pCmdLine));

        //-------------------------------------------------------------
        // Searchuploads
        // Example usage (returns the download file statistics)
        //    >searchuploads /fileid=23
        //-------------------------------------------------------------
        SetupSearchUploadsCommand();

        //-------------------------------------------------------------
        // Searchdownloads
        // Example usage (returns the download file statistics)
        //    >searchdownloads /fileid=23 /statedate=2004-02-20
        //-------------------------------------------------------------
        SetupSearchDownloadsCommand();

        //-------------------------------------------------------------
        // Searchlogins
        // Example usage (returns the login statistics)
        //    >searchlogins /statedate=2004-02-20
        //-------------------------------------------------------------
        SetupSearchLoginsCommand();

        //-------------------------------------------------------------
        // Searchinvoices
        // Example usage (returns the invoice statistics)
        //    >searchinvoices /statedate=2004-02-20
        //-------------------------------------------------------------
        SetupSearchInvoiceLogCommand();

       //-------------------------------------------------------------
        // System command: cls
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_CLS,
            _T("Clear the Diomede Command Line interface."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        GetAltCommandStrs(CMD_CLS, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_CLS, pCmdLine));

        //-------------------------------------------------------------
        // System command: echo
        //-------------------------------------------------------------
        pCmdLine = new CmdLine(CMD_ECHO,
            _T("Changes the command echoing setting."),
            ' ', m_bRedirectedInput, m_szAppVersion.c_str());
        pCmdLine->setOutput(&m_stdOut);

        // Define a echo (on or off) value argument and add it to the command line.
        std::vector<std::string> allowedEchoArgs;
        allowedEchoArgs.push_back(ARG_ON);
        allowedEchoArgs.push_back(ARG_OFF);

        ValuesConstraint<std::string>* pAllowedEchoVals = new ValuesConstraint<std::string>( allowedEchoArgs );

        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_ECHO,
            _T("Turn off or on the Diomede Command Line interface echo."), false, _T(""),
            pAllowedEchoVals /*_T("echo command argument")*/);
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );

        GetAltCommandStrs(CMD_ECHO, pCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_ECHO, pCmdLine));

        //-------------------------------------------------------------
        // System command: rem
        //-------------------------------------------------------------
        DiomedeCmdLine* pDiomedeCmdLine = new DiomedeCmdLine(CMD_REM,
            _T("Indicates a remark to follow."), ' ',
            m_bRedirectedInput, m_szAppVersion.c_str());
        pDiomedeCmdLine->setOutput(&m_stdOut);

        pMultiArg = new DiomedeUnlabeledMultiArg<std::string>(pDiomedeCmdLine,
            ARG_REM,
            _T("Remark."),
            false, _T("remark"), true);

        pDiomedeCmdLine->add( pMultiArg );
        pDiomedeCmdLine->deleteOnExit( pMultiArg );

        GetAltCommandStrs(CMD_REM, pDiomedeCmdLine->getAltCommmandList());
        m_listCommands.insert(std::make_pair(DioCLICommands::CMD_REM, pDiomedeCmdLine));
    }
    catch (ArgException &e) {
        // catch any exceptions
        cerr << _T("error: ") << e.error() << _T(" for arg ") << e.argId() << endl;
    }

} // End SetupCommands

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the login command.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupLoginCommand()
{
    //-----------------------------------------------------------------
    // Login
    // Example usage: >login john password
    //-----------------------------------------------------------------
    DiomedeUnlabeledValueArg<std::string>* pArg = NULL;

    CmdLine* pCmdLine = new CmdLine(CMD_LOGIN,
        _T("Logs onto to the Diomede service."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    // Define a user name and passwod value arguments and add it to the command line.
    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_LOGIN_USERNAME,
        _T("User's username for logging into the Diomede service."),
        true, _T(""), _T("login user name"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_LOGIN_PASSWORD,
        _T("User's password for logging into the Diomede servce."),
        true, _T(""), _T("login password"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_LOGIN, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_LOGIN, pCmdLine));

} // End SetupLoginCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the create user command.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupCreateUserCommand()
{
    //-------------------------------------------------------------
    // Createuser
    // Example usage:
    //    >createuser john password /company="John Co"
    //    > email: john@someplace.com
    // Example usage:
    //    >createuser john password john@someplace.com
    //-------------------------------------------------------------
    DiomedeUnlabeledValueArg<std::string>* pArg = NULL;

    CmdLine* pCmdLine = new CmdLine(CMD_CREATEUSER,
        _T("Create a new Diomede user."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    // Define a user name and passwod value arguments and add it to the command line.
    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_USERNAME,
        _T("New user's username for logging into the Diomede servce."),
        true, _T(""), _T("new user username"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_PASSWORD,
        _T("New user's password for logging into the Diomede servce."),
        true, _T(""), _T("new user password"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_EMAIL,
        _T("New user's email."), true, _T(""),
        _T("new user email"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_CREATEUSER, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_CREATEUSER, pCmdLine));

} // End SetupCreateUserCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the update user info command.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupSetUserInfoCommand()
{
    //-------------------------------------------------------------
    // Setuserinfo
    // NOTE: Arguments are optional, and therefore required the
    //       /arg=<value> syntax.  Username and password are
    //       no longer valid inputs (password is changed via
    //       the (TBD) command.
    // Example usage:
    //    > updateuserinfo /email=john@someplace.com
    //-------------------------------------------------------------
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = new CmdLine(CMD_SETUSERINFO,
        _T("Updates information for the current user."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_FIRSTNAME,
        ARG_FIRSTNAME,
        _T("User's first name."), false, _T(""),
        _T("first name"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_LASTNAME,
        ARG_LASTNAME,
        _T("User's last name."), false, _T(""),
        _T("last name"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_COMPANY,
        ARG_COMPANY,
        _T("User's company."), false, _T(""),
        _T("company"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_WEBURL,
        ARG_WEBURL,
        _T("User's web site URL."), false, _T(""),
        _T("web site URL"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_PHONE,
        ARG_PHONE,
        _T("User's telephone number."), false, _T(""),
        _T("telephone number"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(CMD_SETUSERINFO, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SETUSERINFO, pCmdLine));

} // End SetupSetUserInfoCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the search files command.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupSearchFilesCommand()
{
    //-------------------------------------------------------------
    // Searchfiles
    // Example usage: >searchfile
    //                >searchfile /fileid=22
    //-------------------------------------------------------------
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = new CmdLine(CMD_SEARCHFILES,
        _T("Returns files matching the input filter."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_FILENAME,
        ARG_FILENAME,
        _T("Search for file(s) using a filename."), false, _T(""),
        _T("filename"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_FILEID,
        ARG_FILEID,
        _T("Search for file(s) using a file ID."), false, _T(""),
        _T("file ID"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_HASHMD5,
        ARG_HASHMD5,
        _T("Search for file(s) using a MD5 hash."), false, _T(""),
        _T("MD5 hash"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_HASHSHA1,
        ARG_HASHSHA1,
        _T("Search for file(s) using a SHA1 hash."), false, _T(""),
        _T("SHA1 hash"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_MINSIZE,
        ARG_SEARCH_MINSIZE,
        _T("Search for file(s) using a min file size."), false, _T(""),
        _T("min file size"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_MAXSIZE,
        ARG_SEARCH_MAXSIZE,
        _T("Search for file(s) using a max file size."), false, _T(""),
        _T("max file size"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_STARTDATE,
        ARG_STARTDATE,
        _T("Search for file(s) using a start date (yyyy-mm-dd)."), false, _T(""),
        _T("start date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_ENDDATE,
        ARG_ENDDATE,
        _T("Search for file(s) using a end date (yyyy-mm-dd)."), false, _T(""),
        _T("end date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    std::vector<std::string> allowedIsCompletedArgs;
    allowedIsCompletedArgs.push_back(ARG_YES);
    allowedIsCompletedArgs.push_back(ARG_NO);
    allowedIsCompletedArgs.push_back(ARG_ALL);

    ValuesConstraint<std::string>* pAllowedIsCompletedVals =
        new ValuesConstraint<std::string>( allowedIsCompletedArgs );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_ISCOMPLETE,
        ARG_SEARCH_ISCOMPLETE,
        _T("Search for file(s) where the upload is completed."), false, _T(""),
        pAllowedIsCompletedVals /*_T("search for completed files")*/);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    std::vector<std::string> allowedIsDeletedArgs;
    allowedIsDeletedArgs.push_back(ARG_YES);
    allowedIsDeletedArgs.push_back(ARG_NO);
    allowedIsDeletedArgs.push_back(ARG_ALL);

    ValuesConstraint<std::string>* pAllowedIsDeletedVals =
        new ValuesConstraint<std::string>( allowedIsDeletedArgs );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_ISDELETED,
        ARG_SEARCH_ISDELETED,
        _T("Search for file(s) that have marked as deleted."), false, _T(""),
        pAllowedIsDeletedVals /*_T("search for deleted files")*/);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_METANAME,
        ARG_SEARCH_METANAME,
        _T("Search for file(s) using a file metadata name and value."), false, _T(""),
        _T("metadata name"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_METAVALUE,
        ARG_SEARCH_METAVALUE,
        _T("Search for file(s) using a file metadata name and value."), false, _T(""),
        _T("metadata value"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_PAGE_SIZE,
        ARG_RESULT_PAGE_SIZE,
        _T("Limit search results to the page size."), false, _T(""),
        _T("page size"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_OFFSET,
        ARG_RESULT_OFFSET,
        _T("Return search results beginning at the given offset."), false, _T(""),
        _T("page offset"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    DiomedeSwitchArg* pSwitchArg = new DiomedeSwitchArg(ARG_SEARCH_PHYSICALFILES_SWITCH,
        ARG_SEARCH_PHYSICALFILES_SWITCH, "Include physical files in the results.", false);
    pCmdLine->add( pSwitchArg );
    pCmdLine->deleteOnExit( pSwitchArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
        ARG_OUTPUT,
        _T("Direct search results to a file."), false, _T(""),
        _T("output filename"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pSwitchArg = new DiomedeSwitchArg(ARG_VERBOSE_SWITCH,
        ARG_VERBOSE_SWITCH, "Specify verbose output", false);
    pCmdLine->add( pSwitchArg );
    pCmdLine->deleteOnExit( pSwitchArg );

    GetAltCommandStrs(CMD_SEARCHFILES, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SEARCHFILES, pCmdLine));

} // End SetupSearchFilesCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the search file total command.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupSearchFilesTotalCommand()
{
    //-------------------------------------------------------------
    // Searchfiles
    // Example usage: >searchfilestotal
    //-------------------------------------------------------------
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = new CmdLine(CMD_SEARCHFILESTOTAL,
        _T("Returns a file aggregation matching a filter."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_FILENAME,
        ARG_FILENAME,
        _T("Search for file(s) using a filename."), false, _T(""),
        _T("filename"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_FILEID,
        ARG_FILEID,
        _T("Search for file(s) using a file ID."), false, _T(""),
        _T("file ID"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_HASHMD5,
        ARG_HASHMD5,
        _T("Search for file(s) using a MD5 hash."), false, _T(""),
        _T("MD5 hash"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_HASHSHA1,
        ARG_HASHSHA1,
        _T("Search for file(s) using a SHA1 hash."), false, _T(""),
        _T("SHA1 hash"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_MINSIZE,
        ARG_SEARCH_MINSIZE,
        _T("Search for file(s) using a min file size."), false, _T(""),
        _T("min file size"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_MAXSIZE,
        ARG_SEARCH_MAXSIZE,
        _T("Search for file(s) using a max file size."), false, _T(""),
        _T("max file size"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_STARTDATE,
        ARG_STARTDATE,
        _T("Search for file(s) using a start date (yyyy-mm-dd)."), false, _T(""),
        _T("start date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_ENDDATE,
        ARG_ENDDATE,
        _T("Search for file(s) using a end date (yyyy-mm-dd)."), false, _T(""),
        _T("end date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    std::vector<std::string> allowedIsDeletedArgs;
    allowedIsDeletedArgs.push_back(ARG_YES);
    allowedIsDeletedArgs.push_back(ARG_NO);
    allowedIsDeletedArgs.push_back(ARG_ALL);

    ValuesConstraint<std::string>* pAllowedIsDeletedVals =
        new ValuesConstraint<std::string>( allowedIsDeletedArgs );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_ISDELETED,
        ARG_SEARCH_ISDELETED,
        _T("Search for file(s) that have marked as deleted."), false, _T(""),
        pAllowedIsDeletedVals /*_T("search for deleted files")*/);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    std::vector<std::string> allowedIsCompletedArgs;
    allowedIsCompletedArgs.push_back(ARG_YES);
    allowedIsCompletedArgs.push_back(ARG_NO);
    allowedIsCompletedArgs.push_back(ARG_ALL);

    ValuesConstraint<std::string>* pAllowedIsCompletedVals =
        new ValuesConstraint<std::string>( allowedIsCompletedArgs );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_ISCOMPLETE,
        ARG_SEARCH_ISCOMPLETE,
        _T("Search for file(s) where the upload is completed."), false, _T(""),
        pAllowedIsCompletedVals /*_T("search for completed files")*/);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_METANAME,
        ARG_SEARCH_METANAME,
        _T("Search for file(s) using a file metadata name and value."), false, _T(""),
        _T("metadata name"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_SEARCH_METAVALUE,
        ARG_SEARCH_METAVALUE,
        _T("Search for file(s) using a file metadata name and value."), false, _T(""),
        _T("metadata value"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
        ARG_OUTPUT,
        _T("Direct search results to a file."), false, _T(""),
        _T("output filename"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    DiomedeSwitchArg* pSwitchArg = new DiomedeSwitchArg(ARG_VERBOSE_SWITCH,
        ARG_VERBOSE_SWITCH, "Specify verbose output", false);
    pCmdLine->add( pSwitchArg );
    pCmdLine->deleteOnExit( pSwitchArg );

    GetAltCommandStrs(CMD_SEARCHFILESTOTAL, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SEARCHFILESTOTAL, pCmdLine));

} // End SetupSearchFilesTotalCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the search file total command.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupSearchFilesTotalLogCommand()
{
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = new CmdLine(CMD_SEARCHFILESTOTALLOG,
        _T("Returns aggregates for upload and download data."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_STARTDATE,
        ARG_STARTDATE,
        _T("Search for file total log entries using a start date (yyyy-mm-dd)."), false, _T(""),
        _T("start date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_ENDDATE,
        ARG_ENDDATE,
        _T("Search for file total log entries using a end date (yyyy-mm-dd)."), false, _T(""),
        _T("end date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_PAGE_SIZE,
        ARG_RESULT_PAGE_SIZE,
        _T("Limit search results to the page size."), false, _T(""),
        _T("page size"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_OFFSET,
        ARG_RESULT_OFFSET,
        _T("Return search results beginning at the given offset."), false, _T(""),
        _T("page offset"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
        ARG_OUTPUT,
        _T("Direct search results to a file."), false, _T(""),
        _T("output filename"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    DiomedeSwitchArg* pSwitchArg = new DiomedeSwitchArg(ARG_VERBOSE_SWITCH,
        ARG_VERBOSE_SWITCH, "Specify verbose output", false);
    pCmdLine->add( pSwitchArg );
    pCmdLine->deleteOnExit( pSwitchArg );

    GetAltCommandStrs(CMD_SEARCHFILESTOTALLOG, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SEARCHFILESTOTALLOG, pCmdLine));

} // End SetupSearchFilesTotalLogCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the search uploads command.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupSearchUploadsCommand()
{
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = new CmdLine(CMD_SEARCHUPLOADS,
        _T("Search the upload log."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_FILEID,
        ARG_FILEID,
        _T("Search for upload log entries using a file ID."), false, _T(""),
        _T("file ID"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_STARTDATE,
        ARG_STARTDATE,
        _T("Search for upload log entries using a start date (yyyy-mm-dd)."), false, _T(""),
        _T("start date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_ENDDATE,
        ARG_ENDDATE,
        _T("Search for upload log entries using a end date (yyyy-mm-dd)."), false, _T(""),
        _T("end date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_IP,
        ARG_IP,
        _T("Search for upload log entries using IP address."), false, _T(""),
        _T("IP"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_PAGE_SIZE,
        ARG_RESULT_PAGE_SIZE,
        _T("Limit search results to the page size."), false, _T(""),
        _T("page size"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_OFFSET,
        ARG_RESULT_OFFSET,
        _T("Return search results beginning at the given offset."), false, _T(""),
        _T("page offset"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
        ARG_OUTPUT,
        _T("Direct search results to a file."), false, _T(""),
        _T("output filename"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(CMD_SEARCHUPLOADS, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SEARCHUPLOADS, pCmdLine));

} // End SetupSearchUploadsCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the search downloads command.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupSearchDownloadsCommand()
{
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = new CmdLine(CMD_SEARCHDOWNLOADS,
        _T("Search the download log."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_FILEID,
        ARG_FILEID,
        _T("Search for download log entries using a file ID."), false, _T(""),
        _T("file ID"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_STARTDATE,
        ARG_STARTDATE,
        _T("Search for download log entries using a start date (yyyy-mm-dd)."), false, _T(""),
        _T("start date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_ENDDATE,
        ARG_ENDDATE,
        _T("Search for download log entries using a end date (yyyy-mm-dd)."), false, _T(""),
        _T("end date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_IP,
        ARG_IP,
        _T("Search for download log entriesusing IP address."), false, _T(""),
        _T("IP"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_TOKEN,
        ARG_TOKEN,
        _T("Search for download log entriesusing the download token."), false, _T(""),
        _T("download token"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_PAGE_SIZE,
        ARG_RESULT_PAGE_SIZE,
        _T("Limit search results to the page size."), false, _T(""),
        _T("page size"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_OFFSET,
        ARG_RESULT_OFFSET,
        _T("Return search results beginning at the given offset."), false, _T(""),
        _T("page offset"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
        ARG_OUTPUT,
        _T("Direct search results to a file."), false, _T(""),
        _T("output filename"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(CMD_SEARCHDOWNLOADS, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SEARCHDOWNLOADS, pCmdLine));

} // End SetupSearchDownloadsCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the search login log command.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupSearchLoginsCommand()
{
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = new CmdLine(CMD_SEARCHLOGINS,
        _T("Search the login log."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_STARTDATE,
        ARG_STARTDATE,
        _T("Search for login log entries using a start date (yyyy-mm-dd)."), false, _T(""),
        _T("start date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_ENDDATE,
        ARG_ENDDATE,
        _T("Search for login log entries using a end date (yyyy-mm-dd)."), false, _T(""),
        _T("end date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_IP,
        ARG_IP,
        _T("Search for login log entries using IP address."), false, _T(""),
        _T("IP"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_PAGE_SIZE,
        ARG_RESULT_PAGE_SIZE,
        _T("Limit search results to the page size."), false, _T(""),
        _T("page size"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_RESULT_OFFSET,
        ARG_RESULT_OFFSET,
        _T("Return search results beginning at the given offset."), false, _T(""),
        _T("page offset"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
        ARG_OUTPUT,
        _T("Direct search results to a file."), false, _T(""),
        _T("output filename"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(CMD_SEARCHLOGINS, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SEARCHLOGINS, pCmdLine));

} // End SetupSearchLoginsCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the search invoices command.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupSearchInvoiceLogCommand()
{
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = new CmdLine(CMD_SEARCHINVOICES,
        _T("Search the invoices."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_STARTDATE,
        ARG_STARTDATE,
        _T("Search for invoices using a start date (yyyy-mm-dd)."), false, _T(""),
        _T("start date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_ENDDATE,
        ARG_ENDDATE,
        _T("Search for invoices using a end date (yyyy-mm-dd)."), false, _T(""),
        _T("end date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    // Allowed values for the status argument
    std::vector<std::string> allowedStatusArgs;
    allowedStatusArgs.push_back(ARG_PAID_STATUS);
    allowedStatusArgs.push_back(ARG_UNPAID_STATUS);

    ValuesConstraint<std::string>* pAllowedStatusVals =
        new ValuesConstraint<std::string>( allowedStatusArgs );

    pValueArg = new DiomedeValueArg<std::string>(ARG_INVOICE_STATUS,
        ARG_INVOICE_STATUS,
        _T("Search for invoices using an invoice status."), false, _T(""),
        pAllowedStatusVals /*_T("search on status")*/);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
        ARG_OUTPUT,
        _T("Direct search results to a file."), false, _T(""),
        _T("output filename"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    DiomedeSwitchArg* pSwitchArg = new DiomedeSwitchArg(ARG_TEST_SWITCH,
        ARG_TEST_SWITCH, "Specify test input and output", false);
    pCmdLine->add( pSwitchArg );
    pCmdLine->deleteOnExit( pSwitchArg );

    GetAltCommandStrs(CMD_SEARCHINVOICES, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SEARCHINVOICES, pCmdLine));

} // End SetupSearchInvoiceLogCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the set billing data and subscribe
//      commands.
// Requires:
//      szCommand: command name
//      cmdID: command ID
//      szMessage: command message
// Returns: nothing
void ConsoleControl::SetupSetBillingDataCommand(const std::string& szCommand,
                                                DioCLICommands::COMMAND_ID cmdID,
		                                        const std::string& szMessage)
{
    //-------------------------------------------------------------
    // Setbillingdata (or subscribe)
    // Example usage:
    //    > Setbillingdata "john smith" 112233445566  "07/08/2010" 589 /address2="Apt. 3B"
    //    > address1: 1234 My Address
    //    > city: My City
    //    > state: CA
    //    > country: USA
    //-------------------------------------------------------------
    DiomedeUnlabeledValueArg<std::string>* pArg = NULL;
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = new CmdLine(szCommand, szMessage, ' ',
    m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_BILLING_NAME,
        _T("Name on the credit card."),
        true, _T(""), _T("credit card name"));
    pArg->useLowerCase(false);
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_BILLING_NUMBER,
        _T("Credit card number."),
        true, _T(""), _T("credit card number"));
    pArg->isValidated(false);
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_BILLING_EXPIRES,
        _T("Credit card expiration date (yyyy-mm)."), true, _T(""),
        _T("credit card expiration date"));
    pArg->isValidated(false);
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_BILLING_CVV,
        _T("Card Verification Value (CVV) on back of credit card (3 or 4 digits)."), true, _T(""),
        _T("credit card cvv"));
    pArg->isValidated(false);
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_BILLING_ADDRESS1,
        _T("Credit card billing address."), true, _T(""),
        _T("credit card biling address"));
    pArg->useLowerCase(false);
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_BILLING_CITY,
        _T("Credit card billing city."), true, _T(""),
        _T("credit card billing city"));
    pArg->useLowerCase(false);
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_BILLING_STATE,
        _T("Credit card billing state."), true, _T(""),
        _T("credit card billing state"));
    pArg->useLowerCase(false);
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_BILLING_ZIP,
        _T("Credit card billing zip code."), true, _T(""),
        _T("credit card billing zip code"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_BILLING_COUNTRY,
        _T("Credit card billing country."), true, _T(""),
        _T("credit card billing country"));
    pArg->useLowerCase(false);
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_BILLING_ADDRESS2,
        ARG_BILLING_ADDRESS2,
        _T("Credit card optional address2."), false, _T(""),
        _T("address2"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(szCommand, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(cmdID, pCmdLine));

} // End SetupSetBillingDataCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the metadata commands.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupMetaDataCommands()
{
    DiomedeUnlabeledValueArg<std::string>* pArg = NULL;
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = NULL;

    //-------------------------------------------------------------
    // Createmetadata
    // Example usage: >createmetadata newName newValue
    // TBD: pending further info on how this is supposed to work.
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_CREATEMETADATA,
        _T("Create metadata."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_METANAME,
        _T("Metadata name."), true, _T(""),
        _T("metadata name"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_METAVALUE,
        _T("Metadata value."), true, _T(""),
        _T("metadata value"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_CREATEMETADATA, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_CREATEMETADATA, pCmdLine));

    //-------------------------------------------------------------
    // Createfilemetadata
    // Example usage: >createfilemetadata 33 classical mozart
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_CREATEFILEMETADATA,
        _T("Create a new metadata for a file."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_FILEID,
        _T("File identifier (name, ID, hash) of the original file."),
        true, _T(""), _T("file identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_METANAME,
        _T("Metadata name."), true, _T(""),
        _T("metadata name"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_METAVALUE,
        _T("Metadata value."), true, _T(""),
        _T("metadata value"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_CREATEFILEMETADATA, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_CREATEFILEMETADATA, pCmdLine));

    //-------------------------------------------------------------
    // Setmetadata
    // Example usage: >setmetadata test.doc 123
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_SETFILEMETADATA,
        _T("Set an existing metadata for a file."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_FILEID,
        _T("File identifier (name, ID, hash) of the original file."),
        true, _T(""), _T("file identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_METADATAID,
        _T("Metadata identifier."),
        true, _T(""), _T("metadata identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_SETFILEMETADATA, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SETFILEMETADATA, pCmdLine));

    //-------------------------------------------------------------
    // Deletefilemetadata
    // Example usage: >deletefilemetadata test.doc 123
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_DELETEFILEMETADATA,
        _T("Delete file metadata."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_FILEID,
        _T("File identifier (name, ID, hash) of the original file."),
        true, _T(""), _T("file identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_METADATAID,
        _T("Metadata identifier."),
        true, _T(""), _T("metadata identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_DELETEFILEMETADATA, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_DELETEFILEMETADATA, pCmdLine));

    //-------------------------------------------------------------
    // Deletemetadata
    // Example usage: >deletemetadata 123
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_DELETEMETADATA,
        _T("Delete metadata."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_METADATAID,
        _T("Metadata identifier."),
        true, _T(""), _T("metadata identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_DELETEMETADATA, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_DELETEMETADATA, pCmdLine));

    //-------------------------------------------------------------
    // Getfilemetadata
    // Example usage: >getfilemetadata test.doc
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_GETFILEMETADATA,
        _T("Get file metadata."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_FILEID,
        _T("File identifier (name, ID, hash) of the original file."),
        true, _T(""), _T("file identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_GETFILEMETADATA, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETFILEMETADATA, pCmdLine));

    //-------------------------------------------------------------
    // Getmetadata
    // Example usage: >getmetadata 123 where 123 is a metadata ID
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_GETMETADATA,
        _T("Get metadata."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_METADATAID,
        ARG_METADATAID,
        _T("Retrieve using metadata identifier."),
        false, _T(""), _T("metadata ID"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_METANAME,
        ARG_METANAME, _T("Retrieve using metadata name."),
        false, _T(""), _T("metadata name"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_METAVALUE,
        ARG_METAVALUE, _T("Retrieve using metadata value."),
        false, _T(""), _T("metadata value"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(CMD_GETMETADATA, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETMETADATA, pCmdLine));

    //-------------------------------------------------------------
    // Editmetadata
    // Example usage: >editmetadata 123 newName newValue
    // TBD: pending further info on how this is supposed to work.
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_EDITMETADATA,
        _T("Edit metadata."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_METADATAID,
        _T("Metadata identifier."),
        true, _T(""), _T("metadata identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_METANAME,
        _T("Metadata name."), true, _T(""),
        _T("metadata name"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_METAVALUE,
        _T("Metadata value."), true, _T(""),
        _T("metadata value"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_EDITMETADATA, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_EDITMETADATA, pCmdLine));

} // End SetupMetaDataCommands

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the replication commands.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupReplicationCommands()
{
    DiomedeUnlabeledValueArg<std::string>* pArg = NULL;
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = NULL;

    //-------------------------------------------------------------
    // Replicatefile
    // Example usage: >replicatefile 33 200 /expires=2008-12-01
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_REPLICATEFILE,
        _T("Replicate a file."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_FILEID,
        _T("File identifier (name, ID, hash) of the original file."),
        true, _T(""), _T("file identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_STORAGE_TYPE,
        _T("Storage type where the replicated file will be stored."), true, _T(""),
        _T("storage type"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_EXPIRATION_DATE,
        ARG_EXPIRATION_DATE,
        _T("Expiration date (yyyy-mm-dd)."),
        false, _T(""), _T("expiration date"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(CMD_REPLICATEFILE, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_REPLICATEFILE, pCmdLine));

    //-------------------------------------------------------------
    // Unreplicatefile
    // Example usage: >unreplicatefile 33
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_UNREPLICATEFILE,
        _T("Unreplicate a file."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_FILEID,
        _T("File identifier (name, ID, hash) of the physical file."),
        true, _T(""), _T("file identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_UNREPLICATEFILE, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_UNREPLICATEFILE, pCmdLine));

    //-------------------------------------------------------------
    // Getstoragetypes
    // Example usage: >getstoragetypes
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_GETSTORAGETYPES,
        _T("Returns the available storage types."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
        ARG_OUTPUT,
        _T("Direct search results to a file."), false, _T(""),
        _T("output filename"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(CMD_GETSTORAGETYPES, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETSTORAGETYPES, pCmdLine));

    //-------------------------------------------------------------
    // Getphysicalfileinfo
    // Example usage: >getphysicalfileinfo 33
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_GETPHYSICALFILES,
        _T("Get physical file info for a logical file."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_PHYSICALFILE_ID,
        ARG_PHYSICALFILE_ID,
        _T("Returns info for the given physical file."), false, _T(""),
        _T("physical file ID"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_FILEID,
        _T("File identifier (name, ID, hash) of the logical file."),
        true, _T(""), _T("file identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_GETPHYSICALFILES, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETPHYSICALFILES, pCmdLine));

} // End SetupReplicationCommands

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the replication policy commands.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupReplicationPolicyCommands()
{
    DiomedeUnlabeledValueArg<std::string>* pArg = NULL;
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = NULL;

    //-------------------------------------------------------------
    // Createreplicationpolicy
    // Example usage: >createrp /online=1 /nearline=0 /offline=0 /trighrs=2
    //-------------------------------------------------------------
    SetupReplicationPolicyInfoCommand(CMD_CREATEREPLICATIONPOLICY,
                                      DioCLICommands::CMD_CREATEREPLICATIONPOLICY,
                                      _T("Create a replication policy."));

    //-------------------------------------------------------------
    // Getreplicationpolicies
    // Example usage (returns alll replication policies):
    //    >getrps
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_GETREPLICATIONPOLICIES,
        _T("Returns all replication policies."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
        ARG_OUTPUT,
        _T("Direct search results to a file."), false, _T(""),
        _T("output filename"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(CMD_GETREPLICATIONPOLICIES, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETREPLICATIONPOLICIES, pCmdLine));

    //-------------------------------------------------------------
    // Editreplicationpolicy
    // Example usage: >editrp /online=1 /nearline=0 /offline=0 /trighrs=2
    //-------------------------------------------------------------
    SetupReplicationPolicyInfoCommand(CMD_EDITREPLICATIONPOLICY,
                                      DioCLICommands::CMD_EDITREPLICATIONPOLICY,
                                      _T("Edit a replication policy."), true);

    //-------------------------------------------------------------
    // Deletereplicationpolicies
    // Example usage:
    //    >deleterps 1
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_DELETEREPLICATIONPOLICY,
        _T("Deletes a replication policy."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_REPLICATION_POLICY_ID,
        _T("Replication policy identifier."),
        true, _T(""), _T("replication policy identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_DELETEREPLICATIONPOLICY, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_DELETEREPLICATIONPOLICY, pCmdLine));

    //-------------------------------------------------------------
    // Setreplicationpolicy
    // Example usage: >setrp test.doc 123
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_SETREPLICATIONPOLICY,
        _T("Set an existing replication policy for a file."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_FILEID,
        _T("File identifier (name, ID, hash) of the original file."),
        true, _T(""), _T("file identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_REPLICATION_POLICY_ID,
        _T("Replication policy identifier."),
        true, _T(""), _T("replication policy identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_SETREPLICATIONPOLICY, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SETREPLICATIONPOLICY, pCmdLine));

    //-------------------------------------------------------------
    // Setdefaultreplicationpolicy
    // Example usage: >setdefrp 123
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_SETDEFREPLICATIONPOLICY,
        _T("Set the default replication policy."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
        ARG_REPLICATION_POLICY_ID,
        _T("Replication policy identifier."),
        true, _T(""), _T("replication policy identifier"));
    pCmdLine->add( pArg );
    pCmdLine->deleteOnExit( pArg );

    GetAltCommandStrs(CMD_SETDEFREPLICATIONPOLICY, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_SETDEFREPLICATIONPOLICY, pCmdLine));

    //-------------------------------------------------------------
    // Getdefaultreplicationpolicy
    // Example usage (returns the default replication policy):
    //    >getdefrps
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(CMD_GETDEFREPLICATIONPOLICY,
        _T("Returns the default replication policy."), ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    pValueArg = new DiomedeValueArg<std::string>(ARG_OUTPUT,
        ARG_OUTPUT,
        _T("Direct search results to a file."), false, _T(""),
        _T("output filename"));
    pValueArg->useLowerCase(false);
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(CMD_GETDEFREPLICATIONPOLICY, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(DioCLICommands::CMD_GETDEFREPLICATIONPOLICY, pCmdLine));

} // End SetupReplicationPolicyCommands

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to setup the create and edit replication policy
//      commands.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetupReplicationPolicyInfoCommand(const std::string& szCommand,
                                                       DioCLICommands::COMMAND_ID cmdID,
		                                               const std::string& szMessage,
		                                               bool bAddReplicationPolicyID /*false*/)
{
    DiomedeUnlabeledValueArg<std::string>* pArg = NULL;
    DiomedeValueArg<std::string>* pValueArg = NULL;

    CmdLine* pCmdLine = NULL;

    //-------------------------------------------------------------
    // Createreplicationpolicy (or editereplicationpolicy)
    // Example usage: >createrp /online=1 /nearline=0 /offline=0 /trighrs=2
    //-------------------------------------------------------------
    pCmdLine = new CmdLine(szCommand, szMessage, ' ',
        m_bRedirectedInput, m_szAppVersion.c_str());
    pCmdLine->setOutput(&m_stdOut);

    if (bAddReplicationPolicyID) {
        pArg = new DiomedeUnlabeledValueArg<std::string>(pCmdLine,
            ARG_REPLICATION_POLICY_ID,
            _T("Replication policy identifier."),
            true, _T(""), _T("replication policy identifier"));
        pCmdLine->add( pArg );
        pCmdLine->deleteOnExit( pArg );
    }

    pValueArg = new DiomedeValueArg<std::string>(ARG_DEF_ONLINE,
        ARG_DEF_ONLINE,
        _T("Requested number of online copies at upload."),
        false, _T(""), _T("default online"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_DEF_NEARLINE,
        ARG_DEF_NEARLINE,
        _T("Requested number of nearline copies at upload."),
        false, _T(""), _T("default nearline"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_DEF_OFFLINE,
        ARG_DEF_OFFLINE,
        _T("Requested number of offline copies at upload."),
        false, _T(""), _T("default offline"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_TRIGGER_HOURS,
        ARG_TRIGGER_HOURS,
        _T("Number of hours before the trigger is activated after the last access of the file."),
        false, _T(""), _T("trigger hours"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_TRIGGER_ONLINE,
        ARG_TRIGGER_ONLINE,
        _T("Number of online copies when trigger is activated."),
        false, _T(""), _T("trigger online"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_TRIGGER_NEARLINE,
        ARG_TRIGGER_NEARLINE,
        _T("Number of nearline copies when trigger is activated."),
        false, _T(""), _T("trigger nearline"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_TRIGGER_OFFLINE,
        ARG_TRIGGER_OFFLINE,
        _T("Number of offline copies when trigger is activated."),
        false, _T(""), _T("trigger offline"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    pValueArg = new DiomedeValueArg<std::string>(ARG_EXPIRE_HOURS,
        ARG_EXPIRE_HOURS,
        _T("Number of hours after which files will expire since the last modification or upload."),
        false, _T(""), _T("expire hours"));
    pCmdLine->add( pValueArg );
    pCmdLine->deleteOnExit( pValueArg );

    GetAltCommandStrs(szCommand, pCmdLine->getAltCommmandList());
    m_listCommands.insert(std::make_pair(cmdID, pCmdLine));

} // End SetupReplicationPolicyInfoCommand

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Display the initial console banner using the client log
//      to handle the output to the console.
// Requires: nothing
// Returns: nothing
void ConsoleControl::DisplayBanner()
{
    // No banner when input is from the system command line
    if (m_bSysCommandInput) {
        return;
    }

    std::string szSSLMode = _T("default");

    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
    if (pProfileData) {
        int nSecurityType = pProfileData->GetUserProfileInt(GEN_SERVICE_SECURE_TYPE,
                                                            GEN_SERVICE_SECURE_TYPE_DF);
        if (nSecurityType == SECURE_SERVICE_PARTIAL) {
            szSSLMode = _T("default");
        }
        else if (nSecurityType == SECURE_SERVICE_ALL) {
            szSSLMode = _T("all");
        }
        else if (nSecurityType == SECURE_SERVICE_NONE) {
            szSSLMode = _T("none");
        }
    }

	std::string szBuildNumber = _T("");
	BuildUtil::GetClientBuildNumber(szBuildNumber);

	int nBuildNumber = atoi(szBuildNumber.c_str());

    // For the console output
    PrintNewLine();

    unsigned short nOrigColors = 0;
    PrintTextInColorOn(COLOR_BANNER, nOrigColors);

    _tprintf(_T("          _.---._ \n\r"));
    _tprintf(_T("        _-       -_ \n\r"));

    // If we have build numbers working, show it.  Otherwise, eliminate it from
    // the display.
    if (nBuildNumber > 0) {
        std::string szBuildDisplay = _T("");
        BuildUtil::GetClientVersion(szBuildDisplay);
        
        _tprintf(_T("        |-._____.-|   Diomede CLI [Version %s Build %s]\n\r"), 
            szBuildDisplay.c_str(), szBuildNumber.c_str());
    }
    else {
        _tprintf(_T("        |-._____.-|   Diomede CLI [Version %s]\n\r"), m_szAppVersion.c_str());

    }
    _tprintf(_T("        ._       _.   Copyright (c) 2010 Diomede Corporation. Patents Pending.\n\r"));
    _tprintf(_T("        . -_____- .   \n\r"));
    _tprintf(_T("        |-._____.-|   www.diomedestorage.com\n\r"));
    _tprintf(_T("        ._       _.   \n\r"));
    _tprintf(_T("        . -_____- .   Type HELP for help.\n\r"));
    _tprintf(_T("        |-._____.-|   SSL mode: %s.\n\r"), szSSLMode.c_str());
    _tprintf(_T("        -_       _- \n\r"));
    _tprintf(_T("          -_____- \n\r"));

	#ifdef WIN32
		if (nOrigColors != 0) {
			PrintTextInColorOff(nOrigColors);
		}
	#else
		PrintTextInColorOff(nOrigColors);
	#endif

    // For the the logging
	ClientLog(ALWAYS_COMP, ST, false,
	    _T("-------------------------------------------------") );
	ClientLog(ALWAYS_COMP, ST, true, _T("       Diomede CLI [Version %s]"), m_szAppVersion.c_str() );
	ClientLog(ALWAYS_COMP, ST, true, _T("       Copyright (c) 2010 Diomede Corporation.") );
	ClientLog(ALWAYS_COMP, ST, true, _T("       All rights reserved") );

	std::string szTemp = _T("");

	BuildUtil::GetClientPlatform(szTemp);
	ClientLog(ALWAYS_COMP, ST, true, _T(" Platform       : %s"), CS(szTemp) );
	BuildUtil::GetClientVersion(szTemp);
	ClientLog(ALWAYS_COMP, ST, true, _T(" Client version : %s"), CS(szTemp) );

	ClientLog(ALWAYS_COMP, ST, true, _T(" Build number   : %s"), CS(szBuildNumber) );
	BuildUtil::GetClientBuildTimeStamp(szTemp);
	ClientLog(ALWAYS_COMP, ST, true, _T(" Build date     : %s"), CS(szTemp) );

    szTemp = _T("Release");
    #ifdef _DEBUG
    szTemp = _T("Debug");
    #endif

	ClientLog(ALWAYS_COMP, ST, true, _T(" Build type     : %s"), CS(szTemp) );

	ClientLog(ALWAYS_COMP, ST, true,
	    _T("-------------------------------------------------") );

    #if 0
    //-----------------------------------------------------------------
    // Test for GM Time: first get the current time using the
    // standard gmtime API.  Then using the same raw time value
    // format the date and time using the DioCLI custom
    // FormatDateAndTime.  Compare this time to GMT time from
    // sites such as wwwp.greenwichmeantime.com
    //-----------------------------------------------------------------
    time_t rawTime;
    tm * pTM;

    time ( &rawTime );
    pTM = gmtime ( &rawTime );

    char szBuffer [80];

    strftime (szBuffer, 80,_T("\n\rGMT: %I:%M %p.\n\r"), pTM);
    _tprintf (szBuffer);

    std::string szFormattedDate = _T("");
    struct tm* pLocalTM = std::localtime(&rawTime);

    const char* szDateBuffer = StringUtil::MakeFormatDate(_T("%Y-%m-%d %H:%M:%S"), pLocalTM);
    szFormattedDate = std::string(szDateBuffer);

    free( (void*)szDateBuffer);
    szDateBuffer = NULL;

    _tprintf(("Current formatted date: %s\n\r"), szFormattedDate.c_str());
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
   #endif

} // End DisplayBanner

///////////////////////////////////////////////////////////////////////
// ConsoleControl Public Methods

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Set the command prompt for the console.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetCommandPrompt()
{
    // Simulates echo on/off
    if (false == m_bEchoOn) {
        m_szCommandPrompt = _T("");
        return;
    }

    if (m_bSysCommandInput) {
        return;
    }

    if (false == m_bIncompleteCommand) {
        if (m_bConnected) {
	        m_szCommandPrompt = _T("Diomede:\\") + m_szUsername + _T("> ");
        }
        else {
	        m_szCommandPrompt = _T("Diomede> ");
        }
    }

} // End SetCommandPrompt

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to reset the login data
// Requires: nothing
// Returns: nothing
void ConsoleControl::ResetLoginData()
{
    m_szUsername = _T("");
    m_szSessionToken = _T("");
    m_bConnected = false;

    if (m_bSysCommandInput || m_bAutoLogoutOff) {
        UserProfileData* pProfileData =
            ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
        if (pProfileData) {
	        pProfileData->SetUserProfileStr(GEN_SESSION_TOKEN, _T(""));
    	    pProfileData->SetUserProfileLong(GEN_SESSION_TOKEN_EXPIRES, 0);
	        pProfileData->SaveUserProfile();
	    }
    }

} // End ResetLoginData

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Handle the startup tasks for the console.
// Requires:
//      argc: count of arguments
//      argv: input from the system command line.
//      dcfFilename: Diomede configuration file name (TBD)
//      bUseDataDirectory: specific directory for file location (TBD)
// Returns: true if successful, false otherwise.
bool ConsoleControl::StartDioCLI(int argc, char** argv,
                                 const std::string dcfFilename, const bool bUseDataDirectory)
{
	m_bSysCommandInput = (argc > 1);
	BuildUtil::GetClientVersion(m_szAppVersion);

	if ( this->CommonStartDioCLI( dcfFilename, bUseDataDirectory) == false ) {
		// Error code should be set as needed - this sets up the
		// profile information and certificate - user can continu....
		// return false;
	}

	m_bContinueProcessing = true;

    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );

	//-----------------------------------------------------------------
    // Extract the certificate
	//-----------------------------------------------------------------
    if ( ExtractCertificate() == false) {
        // Error code set in m_lWin32LastError/m_nLastError  - for now, continue logging into
        // the service.  The user will not be allowed to use
        // the https service access.

        #if 0
            // For debugging purposes.
            std::string szSysError = GetErrorString();
            std::string szErrorMsg =
                _format(_T("Error creating Diomede certficate: %s\n\rSSL mode set to none."),
                            szSysError.c_str());
            PrintStatusMsg(szErrorMsg);
            ResetErrorCodes();
        #endif

        // Set the profile data to use the SSL mode option=none
	    if (pProfileData) {
	        // The problem here is that if the user doesn't have access rights
	        // to create the PEM file, saving the settings doesn't work either.
    	    pProfileData->SetUserProfileInt(GEN_SERVICE_SECURE_TYPE, SECURE_SERVICE_NONE);
    	    pProfileData->SaveUserProfile();
    	}
    }

	//-----------------------------------------------------------------
    // All client log logging is done to the log files - so we'll turn this off
    // here - the client log banner will still output to the file.  The user
    // can user the "about" command to obtain this information.
	//-----------------------------------------------------------------
    EnableLoggingToConsole(false);
	RegisterLogObserver(this);

    if (pProfileData) {
        m_bEnableLogging = (pProfileData->GetUserProfileInt(GEN_ENABLE_LOGGING,
                                                            GEN_ENABLE_LOGGING_DF) == 1);
    	EnableLoggingToFile(m_bEnableLogging);
    }

	//-----------------------------------------------------------------
	//-----------------------------------------------------------------
	SetupCommands();
	DisplayBanner();

	//-----------------------------------------------------------------
	// For a threaded version, we'll call the thread's "start" function, which
	// will call a "root function" to start the thread processing of commands.
	// RootFunction();
	//-----------------------------------------------------------------
	bool bSuccess = true;

	// If the user entered a system command, and that command is the login
	// command, we'll replace the current autologin, if any, with this
	// user name and password.
	if (m_bSysCommandInput) {
	    std::string szCommand = AltCommandStrToCommandStr(std::string(argv[1]));
        if (szCommand == CMD_LOGIN) {
            return bSuccess;
        }
        else if (szCommand == CMD_LOGOUT) {
            // Reset the autologin data if present.
            return bSuccess;
        }
        else if (szCommand == CMD_HELP) {
            // No login needed to show the help..
            return bSuccess;
        }
	}

    // Check for a current session token - we'll use this session token
    // for the user's commands, if the user has entered the command
    // through the system command line.
    if (m_bSysCommandInput) {
        if (pProfileData) {
            m_szSessionToken = pProfileData->GetUserProfileStr(GEN_SESSION_TOKEN);
            if (m_szSessionToken.length() > 0) {
                m_bConnected = true;
                g_nSessionRetries = MAX_LOGIN_RETRIES;
            }
        }
    }

    // Check for the auto check account feature - this will be needed here if the
    // user also has auto login set as well.
    if (pProfileData) {
        m_bAutoCheckAccountOn = (pProfileData->GetUserProfileInt(GEN_AUTO_CHECK_ACCOUNT,
                                                                 GEN_AUTO_CHECK_ACCOUNT_DF) == 1);
    }

    bool bNeedPostBannerNewLine = true;

    // Check if the auto login feature is turned on, and if so, we'll
    // login the user.
	if ( pProfileData && ( pProfileData->GetUserProfileInt(GEN_AUTO_LOGIN, 0) != 0) &&
	                     ( pProfileData->GetUserProfileInt(UI_REMEMBER_PWD, 0) != 0) ) {

	    // Get the encrypted username and password.
	    UserData userData;
	    if (pProfileData->GetUserProfileEncrypted(UI_CLIENT_ENCRYPTED, userData) == true) {
	        m_szUsername = userData.UserName();
	        m_szPlainTextPassword = userData.Password();

            // If we still need to login (see above for the system command line
            // case), we'll login use the auto login data.  The username and
            // password will also be used if the session expires, independent
            // of whether the system command prompt is used or not.
            if (!m_bConnected) {
                bNeedPostBannerNewLine = false;
                
                // Check whether the locally stored session token and expiration
                // are still valid - if so use that instead.
                m_szSessionToken = pProfileData->GetUserProfileStr(GEN_SESSION_TOKEN);
                if ( (m_szSessionToken.length() > 0) && (false == HasLocalSessionTokenExpired()) ) {
                    bSuccess = true;
                    m_bConnected = true;
                    g_nSessionRetries = MAX_LOGIN_RETRIES;
                    
                    PrintNewLine();
                    SetCommandPrompt();
                }
                
                if (!m_bConnected) {                    
	                bSuccess = LoginUser(m_szUsername, m_szPlainTextPassword);
	            }
	        }
	   }
	        
	}

    // Get the autologoff setting now as well.
    if (pProfileData) {
        m_bAutoLogoutOff = (pProfileData->GetUserProfileInt(GEN_AUTO_LOGOUT, GEN_AUTO_LOGOUT_DF) == 0);
    }

    // May need a new line after the banner depending on whether the user has been auto-logged
    // into the service.
    if (bNeedPostBannerNewLine && !m_bSysCommandInput) {
        PrintNewLine();
    }

    // If the access to the certificate failed, alert the user now...
    if ( false == IsPemGood() ) {
        std::string szSysError = GetErrorString();
        std::string szErrorMsg =
            _format(_T("Error creating Diomede certficate: %s\n\rSSL mode set to none."),
            szSysError.c_str());
        PrintStatusMsg(szErrorMsg, false, true, true);

        ResetErrorCodes();
    }

	return bSuccess;

} // End StartDioCLI

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Stop the console command processing.
// Requires: nothing
// Returns: true if successful, false otherwise
bool ConsoleControl::StopDioCLI()
{
    ClientLog(UI_COMP, LOG_STATUS, false, _T("StopDioCLI"));
	CommonStopDioCLI();

    ClientLog(UI_COMP, LOG_STATUS, false, _T("UnRegisterLogObserver"));
	UnRegisterLogObserver(this);

	return true;

} // End StopDioCLI

///////////////////////////////////////////////////////////////////////
// Purpose:
//      (TBD:) used if threading the console.  Actions handled after
//      specified sleep period.
// Requires: nothing
// Returns: amount of sleep period.
unsigned int ConsoleControl::MainLoop()
{
	const unsigned int TIME_TO_SLEEP = 100;
	return TIME_TO_SLEEP;

} // End MainLoop

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Handles the common start tasks.  Currently has no functionality.
//      (TBD: open user configuration).
// Requires:
//      dcfFilename: user configuration file name
//      bUseDataDirectory: use the given (TBD) data directory
// Returns: true if successful, false otherwise.
bool ConsoleControl::CommonStartDioCLI( const std::string dcfFilename, const bool bUseDataDirectory)
{
	//-----------------------------------------------------------------
    // Setup the configuration data - the first call to the
    // ProfileManager will read in the configuratin file if it is
    // present.
	//-----------------------------------------------------------------

    UserProfileData* pProfileData =
	    ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );
    if (pProfileData == NULL) {
    	return false;
    }

	return true;

} // End CommonStartDioCLI

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Common stop console handler.
// Requires: nothing
// Returns: nothing
void ConsoleControl::CommonStopDioCLI()
{
    ClientLog(UI_COMP, LOG_STATUS, false, _T("CommonStopDioCLI"));

    // If the user has set autologout to "on", we'll set our
    // "is connected" flag to false - Logout is called within
    // the ShutdowProxyService and will be skipped if the
    // "is connected" flag is false.
    UserProfileData* pProfileData =
        ProfileManager::Instance()->GetProfile( _T("Diomede"), "", "" );

    if (pProfileData == NULL) {
        PrintStatusMsg(_T("Error: Couldn't access user configuration data."));
        ClientLog(UI_COMP, LOG_ERROR, false, _T("Couldn't create user profile object"));
	}
	else if ( m_bConnected ) {
        m_bAutoLogoutOff = (pProfileData->GetUserProfileInt(GEN_AUTO_LOGOUT, GEN_AUTO_LOGOUT_DF) == 0);
        m_bConnected = !m_bAutoLogoutOff;
    }

    ShutdownProxyService();

    ProfileManager::Instance()->Shutdown();
    ResumeManager::Instance()->Shutdown();

} // End CommonStopDioCLI

///////////////////////////////////////////////////////////////////////
// Purpose: Perform initialization of the console attibutes that
//          may return errors.  Any errors return should cause
//          notification to the user and likely exit.
// Requires: nothing
// Returns: true if successful, false otherwise
bool ConsoleControl::ExtractCertificate()
{
    // Load the certificate from either the resource or linked object
    // and write this data out to a file.
    std::string szSlash = _T("\\");
    #ifndef WIN32
        szSlash = _T("/");
    #endif

    std::string szFullPath = _T("");

    if ( true == Util::GetDataDirectory(szFullPath)) {
        szFullPath += szSlash;
    }
    else {
        szFullPath = _T("./");
    }

    szFullPath += m_szCertPem;

    FILE* pDiomedePemFile = fopen( szFullPath.c_str(), "r" );
    if (pDiomedePemFile != 0) {
        // File is there - we may need to read some portion of the
        // file to determine if it's up to date.
        return true;
    }

    int nFailures = 0;
    ResetErrorCodes();

    int nResult = DIOMEDE_PEM::WriteCertificate(szFullPath, nFailures);
    return (nResult == 0);

#if 0
    bool bSuccess = true;

// Prior Windows and Linux version - leaving here for now....
#ifdef WIN32
    // First find and locate the required resource.
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);
    HRSRC hResource = ::FindResource(hInstance, MAKEINTRESOURCE(IDR_DIOMEDE_PEM), _T("PEM"));
    if (!hResource) {
        return false;
    }

    HGLOBAL hFileResource = ::LoadResource(hInstance, hResource);
    if (hFileResource == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Now, open adn map this to a disk file.
    LPVOID lpFileData = ::LockResource(hFileResource);
    if (lpFileData == NULL) {
        return false;
    }

    DWORD dwSize = ::SizeofResource(hInstance, hResource);
    if (dwSize == 0) {
        return false;
    }

    // Method 1 uses virtual memory to write out the file.  This works, but the
    // final file had some "garbage" at the beginning of the file - this seems to
    // have disappeared after correcting a coding error (hopefully).
    #if 1
        // Open the file and filemap
        HANDLE hFile = ::CreateFile( szFullPath.c_str(), GENERIC_READ | GENERIC_WRITE,
            0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            HANDLE hFileMap = ::CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, dwSize, NULL);

            if (hFileMap != NULL) {
                LPVOID lpBaseAddress = ::MapViewOfFile(hFileMap, FILE_MAP_WRITE, 0, 0, 0);
                if (lpBaseAddress != NULL) {

                     // Write the file.
                     ::CopyMemory(lpBaseAddress, lpFileData, dwSize);

                    // Unmap the file and close the handles
                    ::UnmapViewOfFile(lpBaseAddress);
                }
                else {
                    bSuccess = false;
                }
                ::CloseHandle(hFileMap);
            }
            else {
                bSuccess = false;
            }
            ::CloseHandle(hFile);
        }
        else {
            // Caller should report error back to the user.
            m_lWin32LastError = GetLastError();
            bSuccess = false;
        }

   #else
        // Method 2 - simply write the locked resoures out to our file.
        // Which method is better?  Not sure at this point...
        pDiomedePemFile = fopen( szFullPath.c_str(), "wb");
        if (pDiomedePemFile) {
            if (dwSize != fwrite(lpFileData, 1, dwSize, pDiomedePemFile )) {
                bSuccess = false;
            }
            fclose(pDiomedePemFile);
        }
        else {
            // Caller should report error back to the user.
            m_nLastError = errno;
            bSuccess = false;
        }

        // Release the resource
        UnlockResource(hResource);
        ::FreeResource(hResource);

   #endif

   return bSuccess;

#else
    char* pPemStart = &_binary_Diomede_pem_start;
    char* pPemEnd = &_binary_Diomede_pem_end;
    char* szSizePem = &_binary_Diomede_pem_end;

    ULONG nSizeObj = 0;
    if (szSizePem != NULL) {
        nSizeObj = atol(szSizePem);
    }

    ULONG nSizeData = pPemEnd - pPemStart + sizeof(char);
    if (nSizeObj > 0) {
        nSizeData = nSizeObj;
    }

    ULONG nWritten = 0;

    pDiomedePemFile = fopen( szFullPath.c_str(), _T("wt"));
    if (pDiomedePemFile) {
        nWritten = fwrite(pPemStart, sizeof(char), nSizeData, pDiomedePemFile);
        if (nWritten != nSizeData) {
            bSuccess = false;
        }
        fclose(pDiomedePemFile);
    }
    else {
        // Caller should report error back to the user.
        m_nLastError = errno;
        bSuccess = false;
    }

    return bSuccess;
#endif
#endif

} // End ExtractCertificate

///////////////////////////////////////////////////////////////////////
// Purpose: Checks the error codes returned when attempting to
//          create the PEM file.
// Requires: nothing
// Returns: true if successful, false otherwise
bool ConsoleControl::IsPemGood()
{
    return ( (m_lWin32LastError == 0) && (m_nLastError == 0) );

} // End IsPemGood

///////////////////////////////////////////////////////////////////////
// Purpose: Returns the error string associated with the error
//          code return from a system failure, such as file
//          or thread creation.
// Requires: nothing
// Returns: system error string.
std::string ConsoleControl::GetErrorString()
{
    std::string szSystemError = _T("");

    if ( m_lWin32LastError > 0 ) {
        szSystemError = StringUtil::FormatErrorString(m_lWin32LastError);
    }
    else if (m_nLastError > 0) {
        szSystemError = std::string(strerror(m_nLastError));
    }

    return szSystemError;

} // End GetErrorString

///////////////////////////////////////////////////////////////////////
// Purpose: Perform shutdown tasks.
// Requires: nothing
// Returns: true if successful, false otherwise
bool ConsoleControl::ShutdownProxyService()
{
    // If we setup the proxy manually (e.g. init, setup server, etc.),
    // we'll perform the shutdown tasks here.
    ClientLog(UI_COMP, LOG_STATUS, false, _T("ShutdownProxyService"));

    if (!m_bSysCommandInput) {
        // Unnecessary extra line.....
        // PrintNewLine();
    }

    if (m_bConnected) {
        LogoutDiomedeService();
    }

#ifdef WITH_OPENSSL
    ClientLog(UI_COMP, LOG_STATUS, false, _T("Closing OpenSSL"));

    // Thread local cleanup
    ClientLog(UI_COMP, LOG_STATUS, false, _T("OpenSSL: ERR_remove_state"));
    ERR_remove_state(0);

    // Engine cleanup - not needed
    // ENGINE_cleanup();
    ClientLog(UI_COMP, LOG_STATUS, false, _T("OpenSSL: CONF_modules_unload"));
    CONF_modules_unload(1);

    ClientLog(UI_COMP, LOG_STATUS, false, _T("OpenSSL: ERR_free_strings"));
    ERR_free_strings();

    ClientLog(UI_COMP, LOG_STATUS, false, _T("OpenSSL: EVP_cleanup"));
    EVP_cleanup();

    ClientLog(UI_COMP, LOG_STATUS, false, _T("OpenSSL: CRYPTO_cleanup_all_ex_data"));
    CRYPTO_cleanup_all_ex_data();
#endif

    return true;

} // End ShutdownProxyService

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Logout of the diomede service.
// Requires: nothing
// Returns: true if successful, false otherwise.
bool ConsoleControl::LogoutDiomedeService()
{
    // If this a system command prompt scenario, we really only need
    // to process "logout" if the user has actually typed >diocli logout.

    // Caller is responsible for a new line before the status of this
    // command if one is to be present.
    if (m_bSysCommandInput && m_currentCmdID != DioCLICommands::CMD_LOGOUT) {
        ClientLog(UI_COMP, LOG_STATUS, false, _T("LogoutDiomedeService ignored for system command input."));
        return true;
    }

    ClientLog(UI_COMP, LOG_STATUS, false, _T("LogoutDiomedeService"));

    //-----------------------------------------------------------------
    // Process logout
    //-----------------------------------------------------------------
    std::string szLogoutUserStart = _format(_T("Logging out user %s"), m_szUsername.c_str());
    g_szTaskFriendlyName = _T("Logout");

	DIOMEDE_CONSOLE::LogoutTask taskLogout(m_szSessionToken);
	int nResult = HandleTask(&taskLogout, szLogoutUserStart);

    //-----------------------------------------------------------------
    // Check results
    //-----------------------------------------------------------------
	if (nResult == SOAP_OK) {
	    // Print new line after status is reported.
	    PrintStatusMsg(_T("Logout Successful."), m_bSysCommandInput);
	    ResetLoginData();
	    SetCommandPrompt();
	}
	else {
	    std::string szErrorMsg = taskLogout.GetServiceErrorMsg();
	    PrintServiceError(stderr, szErrorMsg);

	    ClientLog(UI_COMP, LOG_ERROR, false,_T("Logout failed for %s."),
	        m_szUsername.c_str());
	}

    return (nResult == SOAP_OK);

} // End LogoutDiomedeService

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Adds a command action to the list of actions.
// Requires:
//      szInput - input from the command line.
// Returns: true if successful, false otherwise
bool ConsoleControl::AddAction( const std::string szInput)
{
    // Add the action to the stack of actions.
    if (false == m_bIncompleteCommand) {

        if (szInput.length() > 0) {
            m_actionStack.push_back(szInput);
            return true;
        }

        // Repeat the prompt.
        return false;
    }

    // Otherwise, the current command is attached to the
    // top command on the stack.
    std::string szCurrentCommand = m_actionStack.front();
    m_actionStack.pop_front();

    // If the entry has spaces, and isn't quoted, we'll add quotes to
    // ensure it's parsed correctly.
    std::string szTmpInput = _T("");
    AddQuotesToArgument(szInput, szTmpInput);

    szCurrentCommand += _T(" ") + szTmpInput;
    m_actionStack.push_front(szCurrentCommand);

    return true;

} // End AddAction

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Adds input from the system command line.
// Requires:
//      argc - count of arguments
//      argv - input from the system command line.
// Returns: true if successful, false otherwise
bool ConsoleControl::AddActions(int argc, char** argv)
{
    std::string szCommandString = _T("");

  	for (int nArgIndex = 1; nArgIndex < argc; nArgIndex++) {

        // Check for spaces in the argument and quote if
        // needed.  Although quotes area added on the system
        // comman line, they are removed by the system before
        // we get the argument.
        std::string szArg = std::string(argv[nArgIndex]);
        std::string szTempArg = _T("");
        AddQuotesToArgument(szArg, szTempArg);

        szCommandString += szTempArg + _T(" ");
    }

    // Knock off the last space and add the command string to the stack.
    int nLength = (int)szCommandString.length();
    if (nLength > 1) {
        szCommandString = szCommandString.substr(0, (nLength-1));
    }

    #if 0
    std::cout << _T("Add Actions: ") << szCommandString << std::endl;
    #endif

	m_actionStack.push_back(szCommandString);
    return true;

} // End AddActions

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to quote the command input when spaces are
//      present and quotes have not been used.
// Requires:
//      szArg - input command argument
//      szArgQuoted - output command argument - quoted if spaces are found.
// Returns: true if spaces are found, false otherwise
bool ConsoleControl::AddQuotesToArgument(std::string szArg, std::string& szArgQuoted)
{
    bool bSpaceFound = false;

	std::string szSpace = _T(" ");
	std::string szQuote = _T("\"");

    szArgQuoted = szArg;

	int nPos = (int)szArg.find_first_of(szSpace);
	if ( (nPos > -1) && (nPos < (int)szArg.size()) ) {

	    bSpaceFound = true;

	    // Check for quotes -
	    int nQuotePos = (int)szArgQuoted.find_first_of(szQuote);
	    if ( (nQuotePos == -1) || (nQuotePos != 0 ) ) {
	        szArgQuoted = szQuote + szArgQuoted;
	    }
	    nQuotePos = (int)szArgQuoted.find_last_of(szQuote);
	    if ( (nQuotePos == -1) || (nQuotePos != ( (int)szArgQuoted.length() - 1) ) ) {
	        szArgQuoted += szQuote;
	    }
	}

	return bSpaceFound;

} // End AddQuotesToArgument

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to remove quotes from the command input when spaces are
//      present and quotes have been added.
// Requires:
//      szArgQuoted - input command argument
//      szArg - output command argument - quotes removed
// Returns: true if quotes are found, false otherwise
bool ConsoleControl::RemoveQuotesFromArgument(std::string szArgQuoted, std::string& szArg)
{
    bool bQuoteFound = false;
	std::string szQuote = _T("\"");

    szArg = szArgQuoted;

    // Check for quotes -
    int nQuotePos = (int)szArgQuoted.find_first_of(szQuote);
    if ( nQuotePos == 0 ){
        szArg = szArg.substr(1);
        bQuoteFound = true;
    }

    nQuotePos = (int)szArgQuoted.find_last_of(szQuote);
    if ( nQuotePos == ( (int)szArgQuoted.length() - 1) ) {
        szArg = szArg.substr(0, nQuotePos - 1);
        bQuoteFound = true;
    }

	return bQuoteFound;

} // End RemoveQuotesFromArgument

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Processes commands off the action stack.  Currently commands
//      are handled singularly.  Using a multi-thread scenario, commands
//      will be added to a stack and pulled off via some event/poll
//      mechanism.
// Requires: nothing
// Returns: action found
ConsoleControl::ACTION_REQUEST ConsoleControl::ProcessActionStack()
{
	ACTION_REQUEST ar;
	ar.m_nCommandID = DioCLICommands::CMD_NULL;

	CmdLine* pCmdLine = NULL;
	g_szTaskFriendlyName = _T("");

	bool bCommandFinished = true;
	m_bIgnoreCommandInput = false;
    g_bSessionError = false;

	if ( false == m_actionStack.empty() ) {
		std::vector<std::string> actionItems;
		unsigned int nCount = 0;

		try {
		    nCount = SplitString(m_actionStack.front(), actionItems, false);
		}
		catch (ArgException &e) {
		    UnknownCommandError(m_actionStack.front(), e);
            return ar;
		}

		if (nCount > 0) {
		    std::string szAltCommand = actionItems[0];

		    std::string szCommand = AltCommandStrToCommandStr(szAltCommand);
			DioCLICommands::COMMAND_ID cmdID = CommandStrToCommandID(szCommand);

			#if 0
			    // Debugging redirected output.
			    cout << _T("szCommand: ") << szCommand << _T(" and cmdID: ") << cmdID << endl;
			#endif

	        CommandMap::iterator cmdIter = m_listCommands.find(cmdID);
	        if (cmdIter != m_listCommands.end()) {
	            pCmdLine = (*cmdIter).second;
	        }
	        else {
                CmdLineParseException e(_T("Unknown command."), szCommand);
		        UnknownCommandError(m_actionStack.front(), e);
	            return ar;
	        }

			bool bParseSuccess = false;
		    pCmdLine->parse((int)actionItems.size(), actionItems, bParseSuccess);

		    /*
			try {
			    pCmdLine->parse((int)actionItems.size(), actionItems, bParseSuccess);
			}
	        catch (ArgException &e) {
	            // Catch any exceptions
    		    ParseCommandError(pCmdLine, e);
	            return ar;
	        }
	        */

			// If the thrown exception is handled by the parse method, our
			// catch above isn't processed.  Handle the error here...
			if (!bParseSuccess) {
                CmdLineParseException e(_T("Argument error."), szCommand);
    		    ParseCommandError(pCmdLine, e);
	            return ar;
			}

			// If the ? is used to show usage, just return.  The usage information will be
			// shown when the command is parsed.
			bool bShowUsage = ShowUsage(pCmdLine);

			if (bShowUsage == true) {
    		    bCommandFinished = true;
			}
			else {
			    //--------------------------------------------------------------
			    // Process the current command.
			    //--------------------------------------------------------------
			    m_pCurrentCommand = pCmdLine;
			    g_szTaskFriendlyName = pCmdLine->getCommandName();

			    m_nCurrentNumArgs = (int)actionItems.size();
			    m_currentCmdID = cmdID;

			    ProcessCommand(m_currentCmdID, m_nCurrentNumArgs, pCmdLine, bCommandFinished);
			}
		}
		else {
		    // Error must have occurred.
		    bCommandFinished = true;
		}

		if (bCommandFinished) {
		    m_actionStack.pop_front();
		}
	}

	// Make sure we command related data.
	if (bCommandFinished) {
	    m_bMaskInput = false;

	    // Clear the attributes used for repeating the last command.
	    m_pCurrentCommand = NULL;
	    g_szTaskFriendlyName = _T("");

	    m_nCurrentNumArgs = 0;
	    m_currentCmdID = DioCLICommands::CMD_NULL;
	}

    m_bIncompleteCommand = !bCommandFinished;

    if (pCmdLine != NULL) {
        pCmdLine->resetArgs();
        pCmdLine->resetValues();

        if (m_bIncompleteCommand == false) {
            pCmdLine->resetRepromptCount();
        }
    }

    // If input was via the system command line and the current
    // command is complete, we're done.  Otherwise,
    // continue with command input from the user.
    if (m_bSysCommandInput && (m_bIncompleteCommand == false)) {
        m_bContinueProcessing = false;
    }
    else  {
        SetCommandPrompt();
    }

	return ar;

} // End ProcessActionStack

///////////////////////////////////////////////////////////////////////
// Purpose: Process the current command.
// Requires:
//      cmdID: command ID
//      nNumArgs: number of arguments on the command line
//      pCmdLine: current command line
//      bCommandFinished: true if the command is finished, false otherwise.
// Returns: nothing
bool ConsoleControl::ProcessCommand(DioCLICommands::COMMAND_ID cmdID, int nNumArgs,
                                    CmdLine* pCmdLine, bool& bCommandFinished)
{
    switch(cmdID) {
	    case DioCLICommands::CMD_NULL:
		    break;
	    case DioCLICommands::CMD_EXIT:
	        // If the user has asked for help on the exit command,
	        // the usage is shown, we do not exit (TBD).
	        if (pCmdLine->foundHelpOrVersionSwitch() == false) {
			    m_bContinueProcessing = false;
	        }

		    break;
	    case DioCLICommands::CMD_HELP:
	        // Show the help on help, e.g. help -h
	        ProcessHelpCommand(nNumArgs, pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_ABOUT:
	        ProcessAboutCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_MENU:
		    ShowCommandsMenu();
		    break;
	    case DioCLICommands::CMD_LOGIN:
		    ProcessLoginCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_LOGOUT:
		    ProcessLogoutCommand(pCmdLine, bCommandFinished);
		    break;
		case DioCLICommands::CMD_SESSIONTOKEN:
		    ProcessGetSessionTokenCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SETCONFIG:
		    ProcessConfigCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_CREATEUSER:
		    ProcessCreateUserCommand(pCmdLine, bCommandFinished);
		    break;

	    case DioCLICommands::CMD_CHANGEPASSWORD:
		    ProcessChangePasswordCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_RESETPASSWORD:
		    ProcessResetPasswordCommand(pCmdLine, bCommandFinished);
		    break;

	    case DioCLICommands::CMD_DELETEUSER:
		    ProcessDeleteUserCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SETUSERINFO:
		    ProcessSetUserInfoCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETUSERINFO:
		    ProcessGetUserInfoCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_DELETEUSERINFO:
		    ProcessDeleteUserInfoCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETEMAILADDRESSES:
		    ProcessGetEmailAddressesCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_ADDEMAILADDRESS:
		    ProcessAddEmailAddressCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_DELETEEMAILADDRESS:
		    ProcessDeleteEmailAddressCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SETPRIMARYEMAILADDRESS:
		    ProcessSetPrimaryEmailAddressCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_CHECKACCOUNT:
		    ProcessCheckAccountCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SUBSCRIBE:
		    ProcessSubscribeUserCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SETBILLINGINFO:
		    ProcessSetBillingDataCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETBILLINGINFO:
		    ProcessGetBillingDataCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_DELETEBILLINGINFO:
		    ProcessDeleteBillingDataCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SEARCHPAYMENTS:
		    ProcessSearchPaymentsCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_UPLOAD:
		    ProcessUploadCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_RESUME:
		    ProcessResumeCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETUPLOADTOKEN:
		    ProcessGetUploadTokenCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SEARCHFILES:
		    ProcessSearchFilesCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SEARCHFILESTOTAL:
		    ProcessSearchFilesTotal(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SEARCHFILESTOTALLOG:
		    ProcessSearchFilesTotalLog(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_DOWNLOAD:
	        ProcessDownloadCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETDOWNLOADURL:
	        #if 0
		    ProcessGetDownloadURL(pCmdLine, bCommandFinished);
		    #else
		    ProcessGetDownloadURLUsingLib(pCmdLine, bCommandFinished);
		    #endif
		    break;
	    case DioCLICommands::CMD_RENAMEFILE:
		    ProcessRenameFileCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_DELETEFILE:
		    ProcessDeleteFileCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_UNDELETEFILE:
		    ProcessUndeleteFileCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_DISPLAYFILE:
		    ProcessDisplayFileCommand(pCmdLine, bCommandFinished);
		    break;

	    case DioCLICommands::CMD_CREATEMETADATA:
		    ProcessCreateMetaDataCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_CREATEFILEMETADATA:
		    ProcessCreateFileMetaDataCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SETFILEMETADATA:
		    ProcessSetFileMetaDataCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_DELETEFILEMETADATA:
		    ProcessDeleteFileMetaDataCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_DELETEMETADATA:
		    ProcessDeleteMetaDataCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETMETADATA:
		    ProcessGetMetaDataCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETFILEMETADATA:
		    ProcessGetFileMetaDataCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_EDITMETADATA:
		    ProcessEditMetaDataCommand(pCmdLine, bCommandFinished);
		    break;

	    case DioCLICommands::CMD_REPLICATEFILE:
		    ProcessReplicateFileCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_UNREPLICATEFILE:
		    ProcessUnReplicateFileCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETSTORAGETYPES:
		    ProcessGetStorageTypesCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETPHYSICALFILES:
		    ProcessGetPhysicalFilesCommand(pCmdLine, bCommandFinished);
		    break;

	    case DioCLICommands::CMD_CREATEREPLICATIONPOLICY:
		    ProcessCreateReplicationPolicyCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETREPLICATIONPOLICIES:
		    ProcessGetReplicationPoliciesCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_EDITREPLICATIONPOLICY:
		    ProcessEditReplicationPolicyCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_DELETEREPLICATIONPOLICY:
		    ProcessDeleteReplicationPolicyCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SETREPLICATIONPOLICY:
		    ProcessSetReplicationPolicyCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SETDEFREPLICATIONPOLICY:
		    ProcessSetDefaultReplicationPolicyCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETDEFREPLICATIONPOLICY:
		    ProcessGetDefaultReplicationPolicyCommand(pCmdLine, bCommandFinished);
		    break;

	    case DioCLICommands::CMD_GETALLPRODUCTS:
		    ProcessGetAllProductsCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_PURCHASEPRODUCT:
		    ProcessPurchaseProductCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETMYPRODUCTS:
		    ProcessGetMyProductsCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_CANCELPRODUCT:
		    ProcessCancelProductCommand(pCmdLine, bCommandFinished);
		    break;

        //-------------------------------------------------
        // To be removed.............
        //-------------------------------------------------
	    case DioCLICommands::CMD_GETALLCONTRACTS:
		    ProcessGetAllContractsCommand(pCmdLine, bCommandFinished);
		    break;
        //-------------------------------------------------
        //-------------------------------------------------

	    case DioCLICommands::CMD_PURCHASECONTRACT:
		    ProcessPurchaseContractCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_GETMYCONTRACTS:
		    ProcessGetMyContractsCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_CANCELCONTRACT:
		    ProcessCancelContractCommand(pCmdLine, bCommandFinished);
		    break;

	    case DioCLICommands::CMD_SEARCHUPLOADS:
		    ProcessSearchUploadLogCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SEARCHDOWNLOADS:
		    ProcessSearchDownloadLogCommand(pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_SEARCHLOGINS:
		    ProcessSearchLoginLogCommand(pCmdLine, bCommandFinished);
		    break;

	    case DioCLICommands::CMD_SEARCHINVOICES:
		    ProcessSearchInvoiceLogCommand(pCmdLine, bCommandFinished);
		    break;

	    case DioCLICommands::CMD_CLS:
		    ProcessSystemCommand(cmdID, pCmdLine, bCommandFinished);
		    break;
	    case DioCLICommands::CMD_REM:
	        // Remarks are handled special since we basically want to ignore
	        // everything except /? - this will be handled by the help
	        // attached to the command as it is parsed.  This bool is used
	        // by the command "getter" in DioCLI.cpp to perform a bit of foolery -
	        // we'll pass an empty prompt when asked for the prompt to ensure
	        // the last prompt is used.
	        m_bIgnoreCommandInput = true;
	        bCommandFinished = true;
		    break;
	    case DioCLICommands::CMD_ECHO:
		    ProcessEchoSystemCommand(pCmdLine, bCommandFinished);
		    break;
	    default:
		    break;
    } // End switch

    return true;

} // End ProcessCommand

//////////////////////////////////////////////////////////////////////
// Purpose: In the case of invalid data, clear the last
//          argument from the stack of commands and arguments.
//          User will be prompted to re-enter the data.
// Requires: nothing
// Returns: true if successful, false otherwise
bool ConsoleControl::ClearLastArgumentFromActionStack()
{
    // Make sure we actually have a command.
    if (m_actionStack.size() == 0) {
        return false;
    }

    std::string szCurrentCommand = m_actionStack.front();

    // Split the current command into strings
	std::vector<std::string> actionItems;
    unsigned int nCount = SplitString(szCurrentCommand, _T(" "), actionItems, false);

    if (nCount == 0) {
        return false;
    }

    // Remove the last string (e.g. argument).
    actionItems.pop_back();

    // Put all the strings back together
    int nIndex = 0;
    std::string szTmpInput = _T("");
    szCurrentCommand = actionItems[nIndex++];

    while ( nIndex < (int)actionItems.size() ) {
        szTmpInput = actionItems[nIndex];
        szCurrentCommand += _T(" ") + szTmpInput;
        nIndex ++;
    }

    // Push them back onto the front.
    m_actionStack.pop_front();
    m_actionStack.push_front(szCurrentCommand);

	return true;

} // End ClearLastArgumentFromActionStack

//////////////////////////////////////////////////////////////////////
// Purpose:
//      Trims the command off the stream.
// Requires:
//      szCommandLine: command string
//      szTrimmed: trimmed string
// Returns: true if successful, false otherwise
bool ConsoleControl::Trim( char* szCommandLine, std::string& szTrimmed )
{
    // Trim trailing and leading white spaces
    char szTempLine[2048];

	szTrimmed = _tcsrtrim(szCommandLine, _T(" \r\n\t"));
	strcpy(szTempLine, szTrimmed.c_str());
	szTrimmed = _tcsltrim(szTempLine, _T(" \r\n\t"));

	return true;

} // End Trim

//////////////////////////////////////////////////////////////////////
// Purpose:
//      Trap and handle some CTRL+<key> combinations.
// Requires:
//      fdwCtrlType: Control key combination.
// Returns: true if successful, false otherwise.
bool ConsoleControl::ProcessControlHandler(DWORD fdwCtrlType)
{
#if !defined( WIN32 )
	return false;
#else
    //-----------------------------------------------------------------
    // CTRL + C: treated as a command cancel.
    //-----------------------------------------------------------------
	std::vector<std::string> actionItems;
	CmdLine* pCmdLine = NULL;

    if (fdwCtrlType == CTRL_C_EVENT) {
        // If the user is in the middle of a command sequence, bounce back to
        // the main prompt.

        if (m_bIncompleteCommand) {
	        unsigned int nCount = SplitString(m_actionStack.front(), _T(" "), actionItems, false);

		    if (nCount > 0) {
			    DioCLICommands::COMMAND_ID cmdID = CommandStrToCommandID(actionItems[0]);

	            CommandMap::iterator cmdIter = m_listCommands.find(cmdID);
	            if (cmdIter != m_listCommands.end()) {
	                pCmdLine = (*cmdIter).second;
	            }

	            if (pCmdLine) {
	                pCmdLine->resetArgs();
	                pCmdLine->resetValues();
	            }

	            m_actionStack.pop_front();
	            m_bIncompleteCommand = false;
	        }
        }
        else {
            g_bUsingCtrlKey = true;
        }
    }

    SetCommandPrompt();
    return true;
#endif

} // End ProcessControlHandler

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Enables logging to to the text file.
// Requires:
//      bNewSetting: Used to turn on or restart logging.
//      Can be used following user action to modify the current
//      logging settings.
// Returns: nothing
void ConsoleControl::EnableLoggingToFile(const bool bNewSetting)
{
	if (false == bNewSetting) {
		return;
	}

    std::string szSlash = _T("\\");
    #ifndef WIN32
        szSlash = _T("/");
    #endif

    std::string szFullPath = _T("");

    if ( true == Util::GetDataDirectory(szFullPath)) {
        szFullPath += szSlash;
    }
    else {
        szFullPath = _T("./");
    }

	bool bSuccess = m_fileLogger.Init( 3, szFullPath.c_str(), 10000000 );
	if (!bSuccess) {
		return;
	}
    RegisterLogObserver( &m_fileLogger );
    m_fileLogger.SetLogObserverType();

} // End EnableLoggingToFile

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Enables logging to to the console.
// Requires:
//      bNewSetting: Used to turn on or restart logging.
//      Can be used following user action to modify the current
//      logging settings.
// Returns: nothing
void ConsoleControl::EnableLoggingToConsole(const bool bNewSetting)
{
	m_bEnableStdLogging = bNewSetting;

} // End EnableLoggingToConsole

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Logger message callback.
// Requires:
//      pMessage: client log message
//      nLen: message length
// Returns: nothing
void ConsoleControl::LogMessageCallback(const std::string& pMessage, int nLen)
{
    // Log to console?  Otherwise, just return.
    if (m_bEnableStdLogging == false) {
        return;
    }

	std::string szTemp = (CS(pMessage));
	cout << szTemp ;

} // End LogMessageCallback

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Sets the log observer type.
// Requires: nothing
// Returns: nothing
void ConsoleControl::SetLogObserverType()
{
    // STD_OUTPUT_LOGGER_TYPE,
    // MEM_LOGGER_TYPE,
    // DEBUGGER_LOGGER_TYPE,
    // UI_LOGGER_TYPE
	m_nLogObserverType = MEM_LOGGER_TYPE;

} // End SetLogObserverType

/** @} */

/*********************************************************************
 * 
 *  file:  DiomedeTask.cpp
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Class implements the classes derived from Task to 
 *          multi-thread processing of commands.
 * 
 *********************************************************************/

#include "stdafx.h"
#include "DiomedeTask.h"

#include "types.h"
#include "XString.h"

#include "../Util/ClientLog.h"
#include "../Util/ClientLogUtils.h"
#include "../Include/ErrorCodes/UIErrors.h"

#include "soapStub.h"

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
#endif


#ifdef WIN32
#include <conio.h>
#endif

using namespace std;
using namespace DIOMEDE;

namespace DIOMEDE_CONSOLE {

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CommandThread

CommandThread::CommandThread()
    :  m_bStop(false)
{
} // End Constructor

/////////////////////////////////////////////////////////////////////////////

CommandThread::~CommandThread()
{
	Stop();

} // End Destructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the command in this thread
// Requires: nothing
// Returns: nothing
DWORD CommandThread::DoCommand()
{
    if (m_bStop) {
	    return 0;
	}

	return 0;

} // End DoCommand

/////////////////////////////////////////////////////////////////////////////

void CommandThread::Start()
{
    // Pending information from the author, we'll add a dummy task here
    // to circumvent issues where the thread crashes if no event has
    // been added before the destructor is called.
    DiomedeTask dummyTask;
    Event(&dummyTask);

    // If this every gets stuck, look for a "return FALSE"
    // from any of the tasks - that causes the thread to exit...
    while ( dummyTask.Status() != TaskStatusCompleted ) {
    }

} // End Start

/////////////////////////////////////////////////////////////////////////////

void CommandThread::Stop()
{
    if (m_bStop)
		return;

	m_bStop = true;

} // End Stop

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Purpose: Formats the error returned from the soap service.
// Requires:
//      soap: number of arguments received
//      szOptErrorMsg: optional message to preface the error string
// Returns: nothing
void DiomedeTask::GetSoapError(struct soap* soap, std::string szOptErrorMsg /*_T("")*/)
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

    if (szOptErrorMsg.length() == 0) {
        szOutErrorMsg += _T("Search files") + szGenericErrorText;
    }
    else {
        szOutErrorMsg = szOptErrorMsg;
    }

    #if 0
	    soap_print_fault(soap, stderr);
	    _tprintf(_T("\n\r"));
    #endif

    m_szServiceErrorMsg = _format(_T(" %s: %s\n\r"), szOutErrorMsg.c_str(), szSoapErrorMsg.c_str());

} // End GetSoapError

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// PressAnyKeyTask

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Returns with any key pressed from the standard innput.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL PressAnyKeyTask::Task()
{
    #ifdef WIN32
        DWORD nMode, nCount;

        HANDLE hHandle = GetStdHandle( STD_INPUT_HANDLE );
        if (hHandle == NULL) {
            // Not a console, so return failure.
            return TRUE;
        }

        GetConsoleMode( hHandle, &nMode );
        SetConsoleMode( hHandle, nMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT) );

        m_szInput = 0;
        bool bKeepWaiting = true;

        INPUT_RECORD InRec;
        FlushConsoleInputBuffer(hHandle);

        while (bKeepWaiting) {
            GetNumberOfConsoleInputEvents(hHandle, &nCount);
            if (nCount > 0) {
                ReadConsoleInput( hHandle, &InRec, 1, &nCount );
                if (InRec.EventType == KEY_EVENT) {
                    bKeepWaiting = false;
                }
            }
            else if (m_bStop == true) {
                SetConsoleMode( hHandle, nMode );
                return TRUE;
            }
        }

        SetConsoleMode( hHandle, nMode );
    #else
        struct termios oldSettings, newSettings;

        tcgetattr( fileno( stdin ), &oldSettings );

        newSettings = oldSettings;

        newSettings.c_iflag&=~ICRNL;
        newSettings.c_lflag&=~ICANON;
        newSettings.c_lflag&=~ECHO;
        newSettings.c_cc[VMIN ]=1;
        newSettings.c_cc[VTIME]=0;
        newSettings.c_cc[VINTR]=0xFF;
        newSettings.c_cc[VSUSP]=0xFF;
        newSettings.c_cc[VQUIT]=0xFF;

        tcsetattr( fileno( stdin ), TCSANOW, &newSettings );

        bool bKeepWaiting = true;

        while ( bKeepWaiting ) {
            fd_set set;
            struct timeval tv;

            tv.tv_sec = 0;
            tv.tv_usec = 0;

            FD_ZERO( &set );
            FD_SET( STDIN_FILENO, &set );

            // Here, we can either pass a set timeout, or NULL for
            // no timeout.
            int nSelectResult = select( STDIN_FILENO+1, &set, NULL, NULL, &tv );

            if ( FD_ISSET(STDIN_FILENO, &set ) ) {

                unsigned char szChar;

                int nReadResult = read(STDIN_FILENO, &szChar, sizeof(szChar));
                if (nReadResult < 0) {
                    perror("Read error");
                }

                bKeepWaiting = false;
            }
            else if ( nSelectResult < 0 ) {
                perror( "Select error" );
                bKeepWaiting = false;
            }

            if (m_bStop == true) {
                bKeepWaiting = false;
            }

            if (bKeepWaiting == false) {
                 FD_CLR( STDIN_FILENO, &set);
            }
        }

        tcsetattr( STDIN_FILENO, TCSANOW, &oldSettings );
    #endif

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// EnumerateFilesTask

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Enumerate the files used for uploading.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL EnumerateFilesTask::Task()
{
    m_pFileEnumerator->EnumerateAll(m_szParentDirectory);

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// LoginTask

/////////////////////////////////////////////////////////////////////////////
LoginTask::LoginTask(std::string szUsername, std::string szPassword)
    :  DiomedeServiceTask(NULL), m_szUsername(szUsername),
       m_szPassword(szPassword)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the login user command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL LoginTask::Task()
{
    if (m_szUsername.length() > 0) {

        if (false == CreateUserManager() ) {
            return TRUE;
        }

        m_nResult = m_pUserManager->LoginUser(m_szUsername, m_szPassword, m_szSessionToken);

	    if (m_nResult != SOAP_OK) {
	        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
	    }

    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// LogoutTask

LogoutTask::LogoutTask( std::string szSessionToken )
    :  DiomedeServiceTask(szSessionToken)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the logout user command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL LogoutTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->LogoutUser(m_szSessionToken);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// CreateUserTask

CreateUserTask::CreateUserTask( DiomedeStorageService* pStorageProxy,
	                            _sds__CreateUserRequest* pCreateUserRequest,
	                            __sds__CreateUserResponse* pCreateUserResponse)

    :  DiomedeServiceTask(pStorageProxy),
       m_pCreateUserRequest(pCreateUserRequest),
       m_pCreateUserResponse(pCreateUserResponse),
       m_pCreateUser(NULL)
{
} // End Constructor

/////////////////////////////////////////////////////////////////////////////
// CreateUserTask- uses CPP SDK Lib

CreateUserTask::CreateUserTask( CreateUserImpl* pCreateUser)

    :  DiomedeServiceTask(NULL),
       m_pCreateUserRequest(NULL),
       m_pCreateUserResponse(NULL),
       m_pCreateUser(pCreateUser)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the CreateUser command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL CreateUserTask::Task()
{
    if (m_pCreateUserRequest != NULL) {
         m_nResult = m_pStorageProxy->__sds__CreateUser(m_pCreateUserRequest, *m_pCreateUserResponse);
    }
    else if (m_pCreateUser != NULL) {
        if (false == CreateUserManager() ) {
            return TRUE;
        }

        m_nResult = m_pUserManager->CreateUser(m_pCreateUser);

	    if (m_nResult != SOAP_OK) {
	        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
	    }

    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// DeleteUserTask

DeleteUserTask::DeleteUserTask( std::string szSessionToken )
    :  DiomedeServiceTask(szSessionToken)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the DeleteUser command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL DeleteUserTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->DeleteUser(m_szSessionToken);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// ChangePasswordTask

ChangePasswordTask::ChangePasswordTask(std::string szSessionToken, std::string szOldPassword,
	                                   std::string szNewPassword)
    :  DiomedeServiceTask(szSessionToken),
       m_szOldPassword(szOldPassword),
       m_szNewPassword(szNewPassword)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the ChangePassword command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL ChangePasswordTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->ChangePassword(m_szSessionToken, m_szOldPassword, m_szNewPassword);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// ResetsPasswordTask

ResetPasswordTask::ResetPasswordTask(std::string szUsername)
    :  DiomedeServiceTask(NULL),
       m_szUsername(szUsername)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the ResetsPassword command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL ResetPasswordTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->ResetPassword(m_szUsername);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SetUserInfoTask

SetUserInfoTask::SetUserInfoTask( std::string szSessionToken, UserInfoImpl* pUserInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_pUserInfo(pUserInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the SetUserInfo command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SetUserInfoTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->SetUserInfo(m_szSessionToken, m_pUserInfo);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetUserInfoTask

GetUserInfoTask::GetUserInfoTask( std::string szSessionToken, UserInfoImpl* pUserInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_pUserInfo(pUserInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the GetUserInfo command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetUserInfoTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->GetUserInfo(m_szSessionToken, m_pUserInfo);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// DeleteUserInfoTask

DeleteUserInfoTask::DeleteUserInfoTask( std::string szSessionToken, int userInfoType)
    :  DiomedeServiceTask(szSessionToken),
       m_nUserInfoType(userInfoType)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the DeleteUserInfo command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL DeleteUserInfoTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->DeleteUserInfo(m_szSessionToken,
                                       static_cast<DIOMEDE::UserInfoType>(m_nUserInfoType));

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetEmailAddressesTask

GetEmailAddressesTask::GetEmailAddressesTask( std::string szSessionToken)
    :  DiomedeServiceTask(szSessionToken)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the GetEmailAddresses command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetEmailAddressesTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->GetEmails(m_szSessionToken, &m_listEmails);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// AddEmailAddressTask

AddEmailAddressTask::AddEmailAddressTask( std::string szSessionToken, std::string szNewEmail)
    :  DiomedeServiceTask(szSessionToken),
       m_szNewEmail(szNewEmail)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the AddEmailAddress command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL AddEmailAddressTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->AddEmail(m_szSessionToken, m_szNewEmail);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// DeleteEmailAddressTask

DeleteEmailAddressTask::DeleteEmailAddressTask( std::string szSessionToken, std::string szOldEmail)
    :  DiomedeServiceTask(szSessionToken),
       m_szOldEmail(szOldEmail)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the RemoveEmailAddress command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL DeleteEmailAddressTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->DeleteEmail(m_szSessionToken, m_szOldEmail);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SetPrimaryEmailAddressTask

SetPrimaryEmailAddressTask::SetPrimaryEmailAddressTask( std::string szSessionToken,
                                                        std::string szPrimaryEmail)
    :  DiomedeServiceTask(szSessionToken),
       m_szPrimaryEmail(szPrimaryEmail)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the SetPrimaryEmailAddress command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SetPrimaryEmailAddressTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->SetPrimaryEmail(m_szSessionToken, m_szPrimaryEmail);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// CreateFileTask

CreateFileTask::CreateFileTask( DiomedeTransferService* pTransferProxy,
	                    _tds__CreateFileRequest* pCreateFileRequest,
	                    _tds__CreateFileResponse* pCreateFileResponse)
    :  DiomedeTransferTask(pTransferProxy),
       m_pCreateFileRequest(pCreateFileRequest),
       m_pCreateFileResponse(pCreateFileResponse),
       m_l64FileID(0),
       m_pUpload(NULL)
{
} // End Constructor

/////////////////////////////////////////////////////////////////////////////
// CreateFileTask - uses CPP SDK Lib

CreateFileTask::CreateFileTask( std::string szSessionToken, class UploadImpl* pUpload)
    :  DiomedeTransferTask(szSessionToken),
       m_pCreateFileRequest(NULL),
       m_pCreateFileResponse(NULL),
       m_l64FileID(0),
       m_pUpload(pUpload)
{

} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the create file command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL CreateFileTask::Task()
{
    /* For CreateFile without creating the file....
    Sleep(1000);
    return TRUE;
    */

    ClientLog(UI_COMP, LOG_STATUS, false, _T("CreateFileTask::Task --- start "));

    if (m_pCreateFileRequest != NULL) {
        m_nResult = m_pTransferProxy->__tds__CreateFile(m_pCreateFileRequest, m_pCreateFileResponse);
        if (m_nResult == SOAP_OK) {
            if (m_pCreateFileResponse->newFileID) {
                m_l64FileID = *m_pCreateFileResponse->newFileID;
            }
        }
    }
    else if (m_pUpload != NULL) {
        if (false == CreateFileManager() ) {
            return TRUE;
        }

        m_nResult = m_pFileManager->CreateFile(m_szSessionToken, m_pUpload);

        /*
        while (m_pUpload->m_nUploadStatus != DIOMEDE::uploadComplete) {
            // Waiting for status to change to complete - callback
            // into this object updates the upload status.
        }
        */

        if (m_nResult == SOAP_OK) {
            m_l64FileID = m_pUpload->GetFileID();
        }
        else {
            m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
        }
    }

    ClientLog(UI_COMP, LOG_STATUS, false, _T("CreateFileTask::Task --- end "));

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// UploadTask

UploadTask::UploadTask( DiomedeTransferService* pTransferProxy,
	                    _tds__UploadRequest* pUploadRequest,
	                    __tds__UploadResponse* pUploadResponse)
    :  DiomedeTransferTask(pTransferProxy),
       m_pUploadRequest(pUploadRequest),
       m_pUploadResponse(pUploadResponse),
       m_l64FileID(0),
       m_pUpload(NULL)
{
} // End Constructor

/////////////////////////////////////////////////////////////////////////////
// UploadTask - uses CPP SDK Lib

UploadTask::UploadTask( std::string szSessionToken, class UploadImpl* pUpload)
    :  DiomedeTransferTask(szSessionToken),
       m_pUploadRequest(NULL),
       m_pUploadResponse(NULL),
       m_l64FileID(0),
       m_pUpload(pUpload)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the upload file command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL UploadTask::Task()
{
    /* For upload without uploading....
    Sleep(1000);
    return TRUE;
    */

    ClientLog(UI_COMP, LOG_STATUS, false, _T("UploadTask::Task --- start "));

    if (m_pUploadRequest != NULL) {
        m_nResult = m_pTransferProxy->__tds__Upload(m_pUploadRequest, *m_pUploadResponse);
    }
    else if (m_pUpload != NULL) {
        if (false == CreateFileManager() ) {
            return TRUE;
        }

        m_nResult = m_pFileManager->Upload(m_szSessionToken, m_pUpload);

        /*
        while (m_pUpload->m_nUploadStatus != DIOMEDE::uploadComplete) {
            // Waiting for status to change to complete - callback
            // into this object updates the upload status.
        }
        */

        if (m_nResult == SOAP_OK) {
            m_l64FileID = m_pUpload->GetFileID();
        }
        else {
            m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
        }
    }
    else {
        m_nResult = DIOMEDE::DIOMEDE_NULL_OBJECT_RECEIVED;
        return TRUE;
    }

    ClientLog(UI_COMP, LOG_STATUS, false, _T("UploadTask::Task --- end "));

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Cancel the uplodad task.
// Requires: nothing
// Returns: 0 if successful, error code otherwise.
int UploadTask::CancelTask()
{
     if (m_pFileManager == NULL) {
        return DIOMEDE::DIOMEDE_NULL_OBJECT_RECEIVED;
    }

    // Since CancelTask's job is to set "stop" flags,
    // re-calling isn't harmful.
    FileManager::CancelTask(m_pFileManager);
    return 0;

} // End CancelTask

/////////////////////////////////////////////////////////////////////////////
// DownloadTask - uses CPP SDK Lib

DownloadTask::DownloadTask( std::string szSessionToken, class DownloadImpl* pDownload)
    :  DiomedeServiceTask(szSessionToken),
       m_szDownloadURL(_T("")),
       m_szDownloadPath(_T("")),
       m_pDownload(pDownload)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Public Methods

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Download the file using wget.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL DownloadTask::Task()
{
    // Separate out (for now)code to handle downloading using the
    // CPP SDK.
    if (m_pDownload == NULL) {
        m_nResult = DIOMEDE::DIOMEDE_NULL_OBJECT_RECEIVED;
        return TRUE;
    }

    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->Download(m_szSessionToken, m_pDownload);

    if (m_nResult == SOAP_OK) {
        m_szDownloadURL = m_pDownload->GetDownloadURL();
    }
    else {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Cancel the download file task.
// Requires: nothing
// Returns: 0 if successful, error code otherwise.
int DownloadTask::CancelTask()
{
    if (m_pFileManager == NULL) {
        return DIOMEDE::DIOMEDE_NULL_OBJECT_RECEIVED;
    }

    // Since CancelTask's job is to set "stop" flags,
    // re-calling isn't harmful.
    FileManager::CancelTask(m_pFileManager);
    return 0;

} // End CancelTask

/////////////////////////////////////////////////////////////////////////////
// DisplayFileTask

DisplayFileTask::DisplayFileTask( std::string szSessionToken,
                                  class DisplayFileContentsImpl* pDisplayFile)
    :  DiomedeServiceTask(szSessionToken),
       m_szDownloadURL(_T("")),
       m_pDisplayFile(pDisplayFile)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Public Methods

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Display the file contents using libcurl.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL DisplayFileTask::Task()
{
    if (m_pDisplayFile == NULL) {
        m_nResult = DIOMEDE::DIOMEDE_NULL_OBJECT_RECEIVED;
        return TRUE;
    }

    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->DisplayFile(m_szSessionToken, m_pDisplayFile);

    if (m_nResult == SOAP_OK) {
        m_szDownloadURL = m_pDisplayFile->GetDownloadURL();
    }
    else {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Cancel the display file task.
// Requires: nothing
// Returns: 0 if successful, error code otherwise.
int DisplayFileTask::CancelTask()
{
    if (m_pFileManager == NULL) {
        return DIOMEDE::DIOMEDE_NULL_OBJECT_RECEIVED;
    }

    // Since CancelTask's job is to set "stop" flags,
    // re-calling isn't harmful.
    FileManager::CancelTask(m_pFileManager);
    return 0;

} // End CancelTask

/////////////////////////////////////////////////////////////////////////////
// GetUploadTokenTask

GetUploadTokenTask::GetUploadTokenTask( std::string szSessionToken, class UploadTokenImpl* pUploadToken)
    :  DiomedeServiceTask(szSessionToken),
       m_pUploadToken(pUploadToken),
       m_szUploadToken(_T(""))
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the GetUploadToken command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetUploadTokenTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->GetUploadToken(m_szSessionToken, m_pUploadToken);

    if (m_nResult == SOAP_OK) {
        m_szUploadToken = m_pUploadToken->GetUploadToken();
    }
    else {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetDownloadURLTask

GetDownloadURLTask::GetDownloadURLTask( DiomedeStorageService* pStorageProxy,
	                              _sds__GetDownloadURLRequest* pGetDownloadURLRequest,
	                              _sds__GetDownloadURLResponse* pGetDownloadURLResponse)
    :  DiomedeServiceTask(pStorageProxy),
       m_pGetDownloadURLRequest(pGetDownloadURLRequest),
       m_pGetDownloadURLResponse(pGetDownloadURLResponse),
       m_szDownloadURL(_T("")),
       m_pDownloadURL(NULL)
{
} // End Constructor

/////////////////////////////////////////////////////////////////////////////
// GetDownloadURLTask - uses CPP SDK Lib

GetDownloadURLTask::GetDownloadURLTask( std::string szSessionToken,
                                        class DownloadURLImpl* pDownloadURL)
    :  DiomedeServiceTask(szSessionToken),
       m_pGetDownloadURLRequest(NULL),
       m_pGetDownloadURLResponse(NULL),
       m_szDownloadURL(_T("")),
       m_pDownloadURL(pDownloadURL)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the GetDownloadURL command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetDownloadURLTask::Task()
{
    if (m_pGetDownloadURLRequest != NULL) {
        m_nResult = m_pStorageProxy->__sds__GetDownloadURL(m_pGetDownloadURLRequest,
                                                           m_pGetDownloadURLResponse);
        if (m_nResult == SOAP_OK) {
            m_szDownloadURL = *m_pGetDownloadURLResponse->url;
        }
    }
    else if (m_pDownloadURL != NULL) {

        if (false == CreateFileManager() ) {
            return TRUE;
        }

        m_nResult = m_pFileManager->GetDownloadURL(m_szSessionToken, m_pDownloadURL);

        if (m_nResult == SOAP_OK) {
            m_szDownloadURL = m_pDownloadURL->GetDownloadURL();
        }
        else {
            m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
        }
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SearchFilesTask

SearchFilesTask::SearchFilesTask( DiomedeStorageService* pStorageProxy,
	                              _sds__SearchFilesRequest* pSearchFilesRequest,
	                              _sds__SearchFilesResponse* pSearchFilesResponse)
    :  DiomedeServiceTask(pStorageProxy),
       m_pSearchFilesRequest(pSearchFilesRequest),
       m_pSearchFilesResponse(pSearchFilesResponse),
       m_pSearchFilter(NULL),
       m_bAddPhysicalFileInfo(false)
{
} // End Constructor

/////////////////////////////////////////////////////////////////////////////
// SearchFilesTask

SearchFilesTask::SearchFilesTask( std::string szSessionToken, SearchFileFilterImpl* pSearchFilter,
                                  bool bAddPhysicalFileInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_pSearchFilesRequest(NULL),
       m_pSearchFilesResponse(NULL),
       m_pSearchFilter(pSearchFilter),
       m_bAddPhysicalFileInfo(bAddPhysicalFileInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the search files command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SearchFilesTask::Task()
{
    /* For testing threads
    Sleep(5000);
    return TRUE;
    */

    if (m_pSearchFilesRequest != NULL) {
        m_nResult = m_pStorageProxy->__sds__SearchFiles(m_pSearchFilesRequest, m_pSearchFilesResponse);

        if (m_nResult != SOAP_OK) {
            GetSoapError(m_pStorageProxy->soap);
        }
    }
    else if (m_pSearchFilter != NULL) {

        if (false == CreateFileManager() ) {
            return TRUE;
        }

        m_nResult = m_pFileManager->SearchFiles(m_szSessionToken, m_pSearchFilter,
                                            &m_listFileProperties);
        if (m_nResult != SOAP_OK) {
            m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
            return TRUE;
        }

        if (m_bAddPhysicalFileInfo == false) {
             return TRUE;
        }

        // Otherwise, make a list of the file IDs returned, and get the
        // physical file information for those files.

        std::vector<void * >listFileProperties = m_listFileProperties.GetFilePropertiesList();
        FileIDList listFileIDs;

        for (int nIndex = 0; nIndex < (int)listFileProperties.size(); nIndex ++) {
            FilePropertiesImpl* pFileProperties = (FilePropertiesImpl*)listFileProperties[nIndex];

            // The choice to use a separate array instead of the list of file properties
            // allows for the usage of the service without a prior search...may or not be
            // the best choice here, but can easily be changed...
            if (pFileProperties != NULL) {
                 listFileIDs.push_back(pFileProperties->GetFileID());
            }
        }

        // With our list of file IDs, get the physical files for those logical
        // file IDs.
        m_nResult = m_pFileManager->GetPhysicalFileInfo(m_szSessionToken, &listFileIDs,
                                                    &m_listLogicalPhysicalFilesInfo);

        // If there are errors, our list class contains a map of file IDs to error
        // codes and strings.
        if (m_nResult != SOAP_OK) {
            m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
            return TRUE;
        }
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SearchFilesTotalTask

SearchFilesTotalTask::SearchFilesTotalTask(std::string szSessionToken,
                                           class SearchFileFilterImpl* pSearchFilter)
    :  DiomedeServiceTask(szSessionToken),
       m_pSearchFilter(pSearchFilter)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the search file total command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SearchFilesTotalTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->SearchFilesTotal(m_szSessionToken, m_pSearchFilter,
                                             &m_fileTotalLogEntry);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SearchFilesTotalLog

SearchFilesTotalLog::SearchFilesTotalLog(std::string szSessionToken,
                                         class SearchLogFilterImpl* pSearchFilter)
    :  DiomedeServiceTask(szSessionToken),
       m_pSearchFilter(pSearchFilter)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the search file total log command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SearchFilesTotalLog::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->SearchFilesTotalLog(m_szSessionToken, m_pSearchFilter,
                                                &m_listFilesTotalLog);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// RenameFileTask

RenameFileTask::RenameFileTask( std::string szSessionToken, LONG64 l64FileID,
                                std::string szNewFileName)
    :  DiomedeServiceTask(szSessionToken),
       m_l64FileID(l64FileID),
       m_szNewFileName(szNewFileName)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the RenameFile command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL RenameFileTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->RenameFile(m_szSessionToken, m_l64FileID, m_szNewFileName);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// DeleteFileTask

DeleteFileTask::DeleteFileTask( std::string szSessionToken, LONG64 l64FileID)
    :  DiomedeServiceTask(szSessionToken),
       m_l64FileID(l64FileID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the DeleteFile command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL DeleteFileTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->DeleteFile(m_szSessionToken, m_l64FileID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// UndeleteFileTask

UndeleteFileTask::UndeleteFileTask( std::string szSessionToken, LONG64 l64FileID)
    :  DiomedeServiceTask(szSessionToken),
       m_l64FileID(l64FileID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the UndeleteFile command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL UndeleteFileTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->UndeleteFile(m_szSessionToken, m_l64FileID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// CreateMetaDataTask

CreateMetaDataTask::CreateMetaDataTask( std::string szSessionToken, MetaDataInfoImpl* pMetaDataInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_pMetaDataInfo(pMetaDataInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the CreateMetaData command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL CreateMetaDataTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->CreateMetaData(m_szSessionToken, m_pMetaDataInfo);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// CreateFileMetaDataTask

CreateFileMetaDataTask::CreateFileMetaDataTask( std::string szSessionToken, LONG64 l64FileID,
	                                            MetaDataInfoImpl* pMetaDataInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_l64FileID(l64FileID),
       m_pMetaDataInfo(pMetaDataInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the CreateFileMetaData command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL CreateFileMetaDataTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->CreateFileMetaData(m_szSessionToken, m_l64FileID, m_pMetaDataInfo);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SetFileMetaDataTask

SetFileMetaDataTask::SetFileMetaDataTask(std::string szSessionToken, LONG64 l64FileID, int nMetaDataID)
    :  DiomedeServiceTask(szSessionToken),
       m_l64FileID(l64FileID),
       m_nMetaDataID(nMetaDataID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the SetFileMetaData command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SetFileMetaDataTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->SetFileMetaData(m_szSessionToken, m_l64FileID, m_nMetaDataID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// DeleteFileMetaDataTask

DeleteFileMetaDataTask::DeleteFileMetaDataTask( std::string szSessionToken, LONG64 l64FileID,
                                                int nMetaDataID)
    :  DiomedeServiceTask(szSessionToken),
       m_l64FileID(l64FileID),
       m_nMetaDataID(nMetaDataID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the DeleteFileMetaData command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL DeleteFileMetaDataTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->DeleteFileMetaData(m_szSessionToken, m_l64FileID, m_nMetaDataID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// DeleteMetaDataTask

DeleteMetaDataTask::DeleteMetaDataTask( std::string szSessionToken, int nMetaDataID)
    :  DiomedeServiceTask(szSessionToken),
       m_nMetaDataID(nMetaDataID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the DeleteMetaData command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL DeleteMetaDataTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->DeleteMetaData(m_szSessionToken, m_nMetaDataID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetFileMetaDataTask

GetFileMetaDataTask::GetFileMetaDataTask( std::string szSessionToken, LONG64 l64FileID)
    :  DiomedeServiceTask(szSessionToken),
       m_l64FileID(l64FileID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the GetFileMetaData command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetFileMetaDataTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->GetFileMetaData(m_szSessionToken, m_l64FileID, &m_listMetaData);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetMetaDataTask

GetMetaDataTask::GetMetaDataTask( std::string szSessionToken, MetaDataInfoImpl* pMetaDataInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_pMetaDataInfo(pMetaDataInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the GetMetaData command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetMetaDataTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->GetMetaData(m_szSessionToken, m_pMetaDataInfo, &m_listMetaData);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// EditMetaDataTask

EditMetaDataTask::EditMetaDataTask( std::string szSessionToken, MetaDataInfoImpl* pMetaDataInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_pMetaDataInfo(pMetaDataInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the EditMetaData command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL EditMetaDataTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->EditMetaData(m_szSessionToken, m_pMetaDataInfo);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// ReplicateFileTask

ReplicateFileTask::ReplicateFileTask( std::string szSessionToken,
	                                  PhysicalFileInfoImpl* pPhysicalFileInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_pPhysicalFileInfo(pPhysicalFileInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the ReplicateFile command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL ReplicateFileTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->ReplicateFile(m_szSessionToken, m_pPhysicalFileInfo);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// UnReplicateFileTask

UnReplicateFileTask::UnReplicateFileTask( std::string szSessionToken, LONG64 l64FileID)
    :  DiomedeServiceTask(szSessionToken),
       m_l64FileID(l64FileID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the UnReplicateFile command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL UnReplicateFileTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->UnReplicateFile(m_szSessionToken, m_l64FileID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetStorageTypesTask

GetStorageTypesTask::GetStorageTypesTask(StorageTypeListImpl* pListStorageType)
    :  DiomedeServiceTask(NULL),
       m_pListStorageType(pListStorageType),
       m_bDestroyList(false)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
GetStorageTypesTask::~GetStorageTypesTask()
{
    if (m_bDestroyList) {
        delete m_pListStorageType;
        m_pListStorageType = NULL;
    }

} // End Destructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the GetStorageTypes command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetStorageTypesTask::Task()
{
    if (m_pListStorageType == NULL) {
        m_pListStorageType = new StorageTypeListImpl;
        m_bDestroyList = true;
    }

    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->GetStorageTypes(m_pListStorageType);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetPhysicalFilesTask

GetPhysicalFilesTask::GetPhysicalFilesTask( std::string szSessionToken,
                                            LONG64 l64FileID, LONG64 l64PhysicalFileID /*0*/)
    :  DiomedeServiceTask(szSessionToken),
       m_l64FileID(l64FileID),
       m_l64PhysicalFileID(l64PhysicalFileID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the GetPhysicalFileInfo command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetPhysicalFilesTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    if (m_l64PhysicalFileID > 0) {
        m_nResult = m_pFileManager->GetPhysicalFileInfo(m_szSessionToken, m_l64FileID,
                                                    m_l64PhysicalFileID,
                                                    &m_physicalFileEntry);
    }
    else {
        m_nResult = m_pFileManager->GetPhysicalFileInfo(m_szSessionToken, m_l64FileID,
                                                    &m_listPhysicalFileInfo);
    }

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// CreateReplicationPolicyTask

CreateReplicationPolicyTask::CreateReplicationPolicyTask( std::string szSessionToken,
                                                          ReplicationPolicyInfoImpl* pReplicationPolicyInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_pReplicationPolicyInfo(pReplicationPolicyInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the CreateReplicationPolicy command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL CreateReplicationPolicyTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->CreateReplicationPolicy(m_szSessionToken, m_pReplicationPolicyInfo);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetReplicationPoliciesTask

GetReplicationPoliciesTask::GetReplicationPoliciesTask( std::string szSessionToken)
    :  DiomedeServiceTask(szSessionToken)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the GetReplicationPolicies command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetReplicationPoliciesTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->GetReplicationPolicies(m_szSessionToken, &m_listReplicationPolicies);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// EditReplicationPolicyTask

EditReplicationPolicyTask::EditReplicationPolicyTask( std::string szSessionToken,
                                                      ReplicationPolicyInfoImpl* pReplicationPolicyInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_pReplicationPolicyInfo(pReplicationPolicyInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the EditReplicationPolicy command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL EditReplicationPolicyTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->EditReplicationPolicy(m_szSessionToken, m_pReplicationPolicyInfo);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// DeleteReplicationPolicyTask

DeleteReplicationPolicyTask::DeleteReplicationPolicyTask( std::string szSessionToken,
                                                          int nReplicationPolicyID)
    :  DiomedeServiceTask(szSessionToken),
       m_nReplicationPolicyID(nReplicationPolicyID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the DeleteReplicationPolicy command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL DeleteReplicationPolicyTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->DeleteReplicationPolicy(m_szSessionToken, m_nReplicationPolicyID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SetReplicationPolicyTask

SetReplicationPolicyTask::SetReplicationPolicyTask(std::string szSessionToken,
	                                               LONG64 l64FileID, int nReplicationPolicyID)
    :  DiomedeServiceTask(szSessionToken),
       m_l64FileID(l64FileID),
       m_nReplicationPolicyID(nReplicationPolicyID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the SetReplicationPolicy command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SetReplicationPolicyTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->SetReplicationPolicy(m_szSessionToken, m_nReplicationPolicyID,
                                                                   m_l64FileID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SetDefaultReplicationPolicyTask

SetDefaultReplicationPolicyTask::SetDefaultReplicationPolicyTask( std::string szSessionToken,
                                                                  int nReplicationPolicyID)
    :  DiomedeServiceTask(szSessionToken),
       m_nReplicationPolicyID(nReplicationPolicyID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the SetDefaultReplicationPolicy command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SetDefaultReplicationPolicyTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->SetDefaultReplicationPolicy(m_szSessionToken, m_nReplicationPolicyID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetDefaultReplicationPolicyTask

GetDefaultReplicationPolicyTask::GetDefaultReplicationPolicyTask( std::string szSessionToken,
                                            ReplicationPolicyInfoImpl* pReplicationPolicyInfo)
    :  DiomedeServiceTask(szSessionToken),
       m_pReplicationPolicyInfo(pReplicationPolicyInfo)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the GetPhysicalFiles command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetDefaultReplicationPolicyTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->GetDefaultReplicationPolicy(m_szSessionToken, m_pReplicationPolicyInfo);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SearchUploadLogTask

SearchUploadLogTask::SearchUploadLogTask( std::string szSessionToken,
                                          SearchUploadLogFilterImpl* pSearchFilter )
    :  DiomedeServiceTask(szSessionToken),
       m_pSearchFilter(pSearchFilter)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the search upload log command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SearchUploadLogTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->SearchUploadLog(m_szSessionToken, m_pSearchFilter, &m_listUploadLog);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SearchDownloadLogTask

SearchDownloadLogTask::SearchDownloadLogTask( std::string szSessionToken,
                                              SearchDownloadLogFilterImpl* pSearchFilter)
    :  DiomedeServiceTask(szSessionToken),
       m_pSearchFilter(pSearchFilter)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the search download log command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SearchDownloadLogTask::Task()
{
    if (false == CreateFileManager() ) {
        return TRUE;
    }

    m_nResult = m_pFileManager->SearchDownloadLog(m_szSessionToken, m_pSearchFilter, &m_listDownloadLog);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pFileManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SearchLoginLogTask

SearchLoginLogTask::SearchLoginLogTask( std::string szSessionToken,
                                        SearchLoginLogFilterImpl* pSearchFilter)
    :  DiomedeServiceTask(szSessionToken),
       m_pSearchFilter(pSearchFilter)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the search login log command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SearchLoginLogTask::Task()
{
    if (false == CreateUserManager() ) {
        return TRUE;
    }

    m_nResult = m_pUserManager->SearchLoginLog(m_szSessionToken, m_pSearchFilter, &m_listLoginLog);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pUserManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetAllProductsTask

GetAllProductsTask::GetAllProductsTask()
    :  DiomedeServiceTask(NULL)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the get all products command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetAllProductsTask::Task()
{
    if (false == CreateProductManager() ) {
        return TRUE;
    }

    m_nResult = m_pProductManager->GetAllProducts(&m_listProducts);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pProductManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// PurchaseProductTask

PurchaseProductTask::PurchaseProductTask( std::string szSessionToken, int nProductID )
    :  DiomedeServiceTask(szSessionToken),
       m_nProductID(nProductID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the purchase product command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL PurchaseProductTask::Task()
{
    if (false == CreateProductManager() ) {
        return TRUE;
    }

    m_nResult = m_pProductManager->PurchaseProduct(m_szSessionToken, m_nProductID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pProductManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetMyProductsTask

GetMyProductsTask::GetMyProductsTask( std::string szSessionToken)
    :  DiomedeServiceTask(szSessionToken)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the get my products command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetMyProductsTask::Task()
{
    if (false == CreateProductManager() ) {
        return TRUE;
    }

    m_nResult = m_pProductManager->GetMyProducts(m_szSessionToken, &m_listUserProducts);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pProductManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// CancelProductTask

CancelProductTask::CancelProductTask( std::string szSessionToken, int nProductID)
    :  DiomedeServiceTask(szSessionToken),
       m_nProductID(nProductID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the cancel product command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL CancelProductTask::Task()
{
    if (false == CreateProductManager() ) {
        return TRUE;
    }

    m_nResult = m_pProductManager->CancelProduct(m_szSessionToken, m_nProductID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pProductManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetAllContractsTask

GetAllContractsTask::GetAllContractsTask()
    :  DiomedeServiceTask(NULL)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the get all contracts command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetAllContractsTask::Task()
{
    if (false == CreateProductManager() ) {
        return TRUE;
    }

    m_nResult = m_pProductManager->GetAllContracts(&m_listContracts);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pProductManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// PurchaseContractTask

PurchaseContractTask::PurchaseContractTask( std::string szSessionToken, int nContractID)
    :  DiomedeServiceTask(szSessionToken),
       m_nContractID(nContractID), m_l64UserContractID(-1)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the purchase contract command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL PurchaseContractTask::Task()
{
    if (false == CreateProductManager() ) {
        return TRUE;
    }

    m_nResult = m_pProductManager->PurchaseContract(m_szSessionToken, m_nContractID,
        m_l64UserContractID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pProductManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// GetMyContractsTask

GetMyContractsTask::GetMyContractsTask( std::string szSessionToken)
    :  DiomedeServiceTask(szSessionToken)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the get my contracts command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL GetMyContractsTask::Task()
{
    if (false == CreateProductManager() ) {
        return TRUE;
    }

    m_nResult = m_pProductManager->GetMyContracts(m_szSessionToken, &m_listUserContracts);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pProductManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// CancelContractTask

CancelContractTask::CancelContractTask( std::string szSessionToken, LONG64 l64ContractID)
    :  DiomedeServiceTask(szSessionToken),
       m_l64ContractID(l64ContractID)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the cancel contract command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL CancelContractTask::Task()
{
    if (false == CreateProductManager() ) {
        return TRUE;
    }

    m_nResult = m_pProductManager->CancelContract(m_szSessionToken, m_l64ContractID);
    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pProductManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SearchPaymentLogTask

SearchPaymentLogTask::SearchPaymentLogTask(std::string szSessionToken, time_t tStartDate,
                                           time_t tEndDate)
    :  DiomedeServiceTask(szSessionToken),
       m_tStartDate(tStartDate), m_tEndDate(tEndDate)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the search payment log command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SearchPaymentLogTask::Task()
{
    if (false == CreatePurchasingManager() ) {
        return TRUE;
    }

    m_nResult = m_pPurchasingManager->SearchPayments(m_szSessionToken, m_tStartDate, m_tEndDate,
                                                 &m_listPaymentLog);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pPurchasingManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

/////////////////////////////////////////////////////////////////////////////
// SearchInvoiceLogTask

SearchInvoiceLogTask::SearchInvoiceLogTask( std::string szSessionToken,
                                            SearchLogFilterImpl* pSearchFilter)
    :  DiomedeServiceTask(szSessionToken),
       m_pSearchFilter(pSearchFilter)
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Process the search invoice log command.
// Requires: nothing
// Returns: TRUE if successful, FALSE otherwise
BOOL SearchInvoiceLogTask::Task()
{
    if (false == CreatePurchasingManager() ) {
        return TRUE;
    }

    m_nResult = m_pPurchasingManager->SearchInvoices(m_szSessionToken, m_pSearchFilter,
                                                 &m_listInvoiceLogEntries);

    if (m_nResult != SOAP_OK) {
        m_szServiceErrorMsg = m_pPurchasingManager->GetErrorMsg();
    }

    // Always return true - otherwise, the thread quits (in our current
    // implementation).
    return TRUE;

} // End Task

} // namespace DIOMEDE_CONSOLE


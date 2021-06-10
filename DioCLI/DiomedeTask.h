/*********************************************************************
 *
 *  file:  DiomedeTask.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Diomede derived Task classes used to multi-thread
 *          those commands which may take some time to process.
 *
 *********************************************************************/

#ifndef __DIOMEDE_TASKS_H__
#define __DIOMEDE_TASKS_H__

#include "stdafx.h"

#include "../Util/Thread.h"
#include "../Include/DiomedeStorage.h"
#include "../CPPSDK.Lib/ServiceAttribs.h"
#include "IDiomedeLib.h"
#include "Enum.h"

#include <queue>
#include <sys/stat.h>

using namespace std;
using namespace DIOMEDE;

namespace DIOMEDE_CONSOLE {

/////////////////////////////////////////////////////////////////////////////
// CommandThread
class CommandThread : public CThread
{

public:
	CommandThread();
	virtual ~CommandThread();

private:
	ThreadId_t					m_threadID;
	bool						m_bStop;

public:
    DWORD DoCommand();

	void Start();
	void Stop();

    // Called when a time interval has elapsed - must be overridden to
    // avoid an assert
	virtual BOOL OnTask() { return TRUE; }

}; // End CommandThread

/////////////////////////////////////////////////////////////////////////////
// PressAnyKeyTask
class PressAnyKeyTask : public CTask
{
protected:
    char            m_szInput;
    bool            m_bStop;

public:
	PressAnyKeyTask() : m_szInput(0), m_bStop(false) {};

	virtual ~PressAnyKeyTask() {};

	char GetKeyInput() { return m_szInput; }
	void StopTask() { m_bStop = true; }

	virtual BOOL Task();

	// Should be called on error if the task is re-run.
	virtual void ResetTask() {
	    m_szInput = 0;
	}

}; // End PressAnyKeyTask

/////////////////////////////////////////////////////////////////////////////
// EnumerateFilesTask
class EnumerateFilesTask : public CTask
{
protected:
    CEnum*                              m_pFileEnumerator; 
    std::string                         m_szParentDirectory;
    bool                                m_bStop;

public:
	EnumerateFilesTask(CEnum* pFileEnumerator, std::string szParentDirectory) 
	    : m_pFileEnumerator(pFileEnumerator), m_szParentDirectory(szParentDirectory), 
	      m_bStop(false) {};

	virtual ~EnumerateFilesTask() {};

	void StopTask() { m_bStop = true; }

	virtual BOOL Task();

	// Should be called on error if the task is re-run.
	virtual void ResetTask() {
	    m_pFileEnumerator = NULL;
	    m_szParentDirectory = "";
	    m_bStop = false;
	}

}; // End PressAnyKeyTask

/////////////////////////////////////////////////////////////////////////////
// DiomedeTask
class DiomedeTask : public CTask
{

protected:
    std::string                         m_szSessionToken;
    int                                 m_nResult;
    std::string                         m_szServiceErrorMsg;

public:
	DiomedeTask() :  m_szSessionToken(_T("")), m_nResult(0), m_szServiceErrorMsg(_T("")) {};

	virtual ~DiomedeTask() {};

	virtual int GetResult() { return m_nResult; }
	virtual std::string GetServiceErrorMsg() { return m_szServiceErrorMsg; }

	virtual std::string GetSessionToken() { return m_szSessionToken; }
	virtual void SetSessionToken(const std::string& szSessionToken) {
	    m_szSessionToken = szSessionToken;
	};

    // Called when a time interval has elapsed - must be overridden to
    // avoid an assert
	virtual BOOL Task() { return TRUE; }

    // Called to attempt cancellation of the current task.
    // Not all tasks are candidates for cancellation.
	virtual int CancelTask() { return 0; }

	// Should be called on error if the task is re-run.
	virtual void ResetTask() {
	    m_nResult = 0;
	    m_szServiceErrorMsg = _T("");
	}

	virtual void GetSoapError(struct soap* soap, std::string szOptErrorMsg=_T(""));

}; // End DiomedeTask

/////////////////////////////////////////////////////////////////////////////
// DiomedeServiceTask
class DiomedeServiceTask : public DiomedeTask
{

protected:
    DiomedeStorageService*              m_pStorageProxy;
    DIOMEDE::UserManager*               m_pUserManager;
    DIOMEDE::FileManager*               m_pFileManager;
    DIOMEDE::ProductManager*            m_pProductManager;
    DIOMEDE::PurchasingManager*         m_pPurchasingManager;

protected:
	DiomedeServiceTask(std::string szSessionToken)
	    :  m_pStorageProxy(NULL),
	       m_pUserManager(NULL),
	       m_pFileManager(NULL),
	       m_pProductManager(NULL),
	       m_pPurchasingManager(NULL)
	{
	    m_szSessionToken = szSessionToken;
	}

	DiomedeServiceTask(DiomedeStorageService* pStorageProxy)
	    :  m_pStorageProxy(pStorageProxy),
	       m_pUserManager(NULL),
	       m_pFileManager(NULL),
	       m_pProductManager(NULL),
	       m_pPurchasingManager(NULL)
	{
	    m_szSessionToken = _T("");
	}

	virtual ~DiomedeServiceTask() {};

protected:

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    bool CreateUserManager()
    {
        m_nResult = 0;

        if (m_pUserManager != NULL) {
            return true;
        }

        IDiomedeLibError error;
        m_pUserManager = DIOMEDE::UserManager::CreateInstance(error);

        if (m_pUserManager == NULL) {
            m_nResult = error.GetResult();
            m_szServiceErrorMsg = error.GetErrorMsg();
            return false;
        }

        return true;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    bool CreateFileManager()
    {
        m_nResult = 0;

        if (m_pFileManager != NULL) {
            return true;
        }

        IDiomedeLibError error;
        m_pFileManager = DIOMEDE::FileManager::CreateInstance(error);

        if (m_pFileManager == NULL) {
            m_nResult = error.GetResult();
            m_szServiceErrorMsg = error.GetErrorMsg();
            return false;
        }

        return true;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    bool CreatePurchasingManager()
    {
        m_nResult = 0;

        if (m_pPurchasingManager != NULL) {
            return true;
        }

        IDiomedeLibError error;
        m_pPurchasingManager = DIOMEDE::PurchasingManager::CreateInstance(error);

        if (m_pPurchasingManager == NULL) {
            m_nResult = error.GetResult();
            m_szServiceErrorMsg = error.GetErrorMsg();
            return false;
        }

        return true;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    bool CreateProductManager()
    {
        m_nResult = 0;

        if (m_pProductManager != NULL) {
            return true;
        }

        IDiomedeLibError error;
        m_pProductManager = DIOMEDE::ProductManager::CreateInstance(error);

        if (m_pProductManager == NULL) {
            m_nResult = error.GetResult();
            m_szServiceErrorMsg = error.GetErrorMsg();
            return false;
        }

        return true;
    }

public:
	virtual BOOL Task() { return TRUE; }

}; // End DiomedeServiceTask

/////////////////////////////////////////////////////////////////////////////
// DiomedeTransferTask
class DiomedeTransferTask : public DiomedeTask
{

protected:
    DiomedeTransferService*     m_pTransferProxy;
    DIOMEDE::FileManager*       m_pFileManager;

protected:
	DiomedeTransferTask(std::string szSessionToken)
	    :  m_pTransferProxy(NULL),
	       m_pFileManager(NULL)
	{
	    m_szSessionToken = szSessionToken;
	}

	DiomedeTransferTask(DiomedeTransferService* pTransferProxy)
	    :  m_pTransferProxy(pTransferProxy),
	       m_pFileManager(NULL)
	{
	    m_szSessionToken = _T("");
	}

	virtual ~DiomedeTransferTask()
	{
        if (m_pFileManager != NULL) {
            m_pFileManager->DestroyInstance();
            m_pFileManager = NULL;
        }
	};

    bool CreateFileManager()
    {
        m_nResult = 0;

        if (m_pFileManager != NULL) {
            return true;
        }

        IDiomedeLibError error;
        m_pFileManager = DIOMEDE::FileManager::CreateInstance(error);

        if (m_pFileManager == NULL) {
            m_nResult = error.GetResult();
            m_szServiceErrorMsg = error.GetErrorMsg();
            return false;
        }

        return true;

    }

public:
	virtual BOOL Task() { return TRUE; }


}; // End DiomedeTransferTask

/////////////////////////////////////////////////////////////////////////////
// LoginTask
class LoginTask : public DiomedeServiceTask
{

private:
    std::string                         m_szUsername;
    std::string                         m_szPassword;

public:
	LoginTask(std::string szUsername, std::string szPassword);

	virtual ~LoginTask() {};

	virtual BOOL Task();

}; // End LoginTask

/////////////////////////////////////////////////////////////////////////////
// LogoutTask
class LogoutTask : public DiomedeServiceTask
{

public:
	LogoutTask(std::string szSessionToken);

	virtual ~LogoutTask() {};

	virtual BOOL Task();

}; // End LogoutTask

/////////////////////////////////////////////////////////////////////////////
// CreateUserTask
class CreateUserTask : public DiomedeServiceTask
{

private:
    _sds__CreateUserRequest*            m_pCreateUserRequest;
    __sds__CreateUserResponse*          m_pCreateUserResponse;

    CreateUserImpl*                     m_pCreateUser;


public:
	CreateUserTask(DiomedeStorageService* pStorageProxy,
	               _sds__CreateUserRequest* pCreateUserRequest,
	               __sds__CreateUserResponse* pCreateUserResponse);

	CreateUserTask(CreateUserImpl* pCreateUser);

	virtual ~CreateUserTask() {};

	virtual BOOL Task();

}; // End CreateUserTask

/////////////////////////////////////////////////////////////////////////////
// DeleteUserTask
class DeleteUserTask : public DiomedeServiceTask
{

public:
	DeleteUserTask(std::string szSessionToken);

	virtual ~DeleteUserTask() {};

	virtual BOOL Task();

}; // End DeleteUserTask

/////////////////////////////////////////////////////////////////////////////
// ChangePasswordTask
class ChangePasswordTask : public DiomedeServiceTask
{

private:
    std::string                         m_szOldPassword;
    std::string                         m_szNewPassword;

public:
	ChangePasswordTask(std::string szSessionToken, std::string szOldPassword,
	                                               std::string szNewPassword);
	virtual ~ChangePasswordTask() {};

	virtual BOOL Task();

}; // End ChangePasswordTask

/////////////////////////////////////////////////////////////////////////////
// ResetPasswordTask
class ResetPasswordTask : public DiomedeServiceTask
{

private:
    std::string                         m_szUsername;

public:
	ResetPasswordTask(std::string szUsername);
	virtual ~ResetPasswordTask() {};

	virtual BOOL Task();

}; // End ResetPasswordTask

/////////////////////////////////////////////////////////////////////////////
// SetUserInfoTask
class SetUserInfoTask : public DiomedeServiceTask
{
private:
    UserInfoImpl*                       m_pUserInfo;

public:
	SetUserInfoTask(std::string szSessionToken, UserInfoImpl* pUserInfo);
	virtual ~SetUserInfoTask() {};

	virtual BOOL Task();

}; // End SetUserInfoTask

/////////////////////////////////////////////////////////////////////////////
// GetUserInfoTask
class GetUserInfoTask : public DiomedeServiceTask
{
private:
    UserInfoImpl*                       m_pUserInfo;

public:
	GetUserInfoTask(std::string szSessionToken, UserInfoImpl* pUserInfo);
	virtual ~GetUserInfoTask() {};

	virtual BOOL Task();

}; // End GetUserInfoTask

/////////////////////////////////////////////////////////////////////////////
// DeleteUserInfoTask
class DeleteUserInfoTask : public DiomedeServiceTask
{

private:
    int                                 m_nUserInfoType;

public:
	DeleteUserInfoTask(std::string szSessionToken, int userInfoType);
	virtual ~DeleteUserInfoTask() {};

	virtual BOOL Task();

}; // End DeleteUserInfoTask

/////////////////////////////////////////////////////////////////////////////
// GetEmailAddressesTask
class GetEmailAddressesTask : public DiomedeServiceTask
{

private:
    EmailListImpl                       m_listEmails;

public:
	GetEmailAddressesTask(std::string szSessionToken);
	virtual ~GetEmailAddressesTask() {};

	virtual BOOL Task();

	class EmailListImpl* GetEmailListResults() {
	    return &m_listEmails;
	}

}; // End GetEmailAddressesTask

/////////////////////////////////////////////////////////////////////////////
// AddEmailAddressTask
class AddEmailAddressTask : public DiomedeServiceTask
{

private:
    std::string                         m_szNewEmail;

public:
	AddEmailAddressTask(std::string szSessionToken, std::string szNewEmail);
	virtual ~AddEmailAddressTask() {};

	virtual BOOL Task();

}; // End AddEmailAddressTask

/////////////////////////////////////////////////////////////////////////////
// DeleteEmailAddressTask
class DeleteEmailAddressTask : public DiomedeServiceTask
{

private:
    std::string                         m_szOldEmail;

public:
	DeleteEmailAddressTask(std::string szSessionToken, std::string szOldEmail);
	virtual ~DeleteEmailAddressTask() {};

	virtual BOOL Task();

}; // End DeleteEmailAddressTask

/////////////////////////////////////////////////////////////////////////////
// SetPrimaryEmailAddressTask
class SetPrimaryEmailAddressTask : public DiomedeServiceTask
{

private:
    std::string                         m_szPrimaryEmail;

public:
	SetPrimaryEmailAddressTask(std::string szSessionToken, std::string szPrimaryEmail);
	virtual ~SetPrimaryEmailAddressTask() {};

	virtual BOOL Task();

}; // End SetPrimaryEmailAddressTask

/////////////////////////////////////////////////////////////////////////////
// CreateFileTask
class CreateFileTask : public DiomedeTransferTask
{

private:
    _tds__CreateFileRequest*            m_pCreateFileRequest;
    _tds__CreateFileResponse*           m_pCreateFileResponse;

    LONG64                              m_l64FileID;
    UploadImpl*                         m_pUpload;

public:
	CreateFileTask(DiomedeTransferService* pTransferProxy,
	           _tds__CreateFileRequest* pCreateFileRequest,
	           _tds__CreateFileResponse* pCreateFileResponse);

	CreateFileTask(std::string szSessionToken, class UploadImpl* pUpload);
	virtual ~CreateFileTask() {};

	//--------------------------------------------------------------------
    // Getters and setters - special for this task to allow re-use
    // over many file uploads.
	//--------------------------------------------------------------------

	// Access the upload data structure - allows modifications or
	// additions for tasks such as resume.
	UploadImpl* GetUploadImpl() {
	    return m_pUpload;
	}

	void SetUploadImpl(UploadImpl* pUpload) {
	    m_pUpload = pUpload;
	}

	// Access the file ID resulting from the upload.
	LONG64 GetFileID() {
	    return m_l64FileID;
	}

	//--------------------------------------------------------------------
	// Overrides
	//--------------------------------------------------------------------
	virtual BOOL Task();

}; // End CreateFileTask

/////////////////////////////////////////////////////////////////////////////
// UploadTask
class UploadTask : public DiomedeTransferTask
{

private:
    _tds__UploadRequest*                m_pUploadRequest;
    __tds__UploadResponse*              m_pUploadResponse;

    LONG64                              m_l64FileID;
    UploadImpl*                         m_pUpload;

public:
	UploadTask(DiomedeTransferService* pTransferProxy,
	           _tds__UploadRequest* pUploadRequest,
	           __tds__UploadResponse* pUploadResponse);

	UploadTask(std::string szSessionToken, class UploadImpl* pUpload);

	virtual ~UploadTask() {};

	//--------------------------------------------------------------------
    // Getters and setters - special for this task to allow re-use
    // over many file uploads.
	//--------------------------------------------------------------------

	// Access the upload data structure - allows modifications or
	// additions for tasks such as resume.
	UploadImpl* GetUploadImpl() {
	    return m_pUpload;
	}

	void SetUploadImpl(UploadImpl* pUpload) {
	    m_pUpload = pUpload;
	}

	// Access the file ID resulting from the upload.
	LONG64 GetFileID() {
	    return m_l64FileID;
	}

	//--------------------------------------------------------------------
	// Overrides
	//--------------------------------------------------------------------
	virtual BOOL Task();
	virtual int CancelTask();

}; // End UploadTask

/////////////////////////////////////////////////////////////////////////////
// DownloadTask
class DownloadTask : public DiomedeServiceTask
{

private:
    std::string                         m_szDownloadURL;
    std::string                         m_szDownloadPath;
    int                                 m_nResult;

    DownloadImpl*                       m_pDownload;

public:
	DownloadTask(std::string szSessionToken, class DownloadImpl* pDownload);

	virtual ~DownloadTask() {};

	int GetResult() { return m_nResult; }
	std::string GetDownloadURL() {
	    return m_szDownloadURL;
	}

	virtual BOOL Task();

	virtual int CancelTask();

}; // End DownloadTask

/////////////////////////////////////////////////////////////////////////////
// DisplayFileTask
class DisplayFileTask : public DiomedeServiceTask
{

private:
    std::string                         m_szDownloadURL;
    int                                 m_nResult;

    DisplayFileContentsImpl*            m_pDisplayFile;

public:
	DisplayFileTask(std::string szSessionToken, class DisplayFileContentsImpl* pDisplayFile);

	virtual ~DisplayFileTask() {};

	int GetResult() { return m_nResult; }
	std::string GetDownloadURL() {
	    return m_szDownloadURL;
	}

	virtual BOOL Task();

	virtual int CancelTask();

}; // End DisplayFileTask

/////////////////////////////////////////////////////////////////////////////
// GetUploadTokenTask
class GetUploadTokenTask : public DiomedeServiceTask
{

private:
    UploadTokenImpl*                    m_pUploadToken;
    std::string                         m_szUploadToken;


public:
	GetUploadTokenTask( std::string szSessionToken, class UploadTokenImpl* pUploadToken);
	virtual ~GetUploadTokenTask() {};

	virtual BOOL Task();

	std::string GetUploadToken() {
	    return m_szUploadToken;
	}

}; // End GetUploadTokenTask

/////////////////////////////////////////////////////////////////////////////
// GetDownloadURLTask
class GetDownloadURLTask : public DiomedeServiceTask
{

private:
    _sds__GetDownloadURLRequest*        m_pGetDownloadURLRequest;
    _sds__GetDownloadURLResponse*       m_pGetDownloadURLResponse;

    std::string                         m_szDownloadURL;
    DownloadURLImpl*                    m_pDownloadURL;

public:
	GetDownloadURLTask( DiomedeStorageService* m_pStorageProxy,
	                 _sds__GetDownloadURLRequest* pGetDownloadURLRequest,
	                 _sds__GetDownloadURLResponse* pGetDownloadURLResponse);
	GetDownloadURLTask(std::string szSessionToken, class DownloadURLImpl* pDownloadURL);

	virtual ~GetDownloadURLTask() {};

	virtual BOOL Task();

	std::string GetDownloadURL() {
	    return m_szDownloadURL;
	}

}; // End GetDownloadURLTask

/////////////////////////////////////////////////////////////////////////////
// SearchFilesTask
class SearchFilesTask : public DiomedeServiceTask
{

private:
    _sds__SearchFilesRequest*           m_pSearchFilesRequest;
    _sds__SearchFilesResponse*          m_pSearchFilesResponse;

    FilePropertiesListImpl              m_listFileProperties;
    LogicalPhysicalFilesInfoListImpl    m_listLogicalPhysicalFilesInfo;

    SearchFileFilterImpl*               m_pSearchFilter;
    bool                                m_bAddPhysicalFileInfo;

public:
	SearchFilesTask( DiomedeStorageService* m_pStorageProxy,
	                 _sds__SearchFilesRequest* pSearchFilesRequest,
	                 _sds__SearchFilesResponse* pSearchFilesResponse);

	SearchFilesTask(std::string szSessionToken, class SearchFileFilterImpl* pSearchFilter,
	                bool bAddPhysicalFileInfo);

	virtual ~SearchFilesTask() {};

	virtual BOOL Task();

	class FilePropertiesListImpl* GetSearchFilesResults() {
	    return &m_listFileProperties;
	}

	class LogicalPhysicalFilesInfoListImpl* GetSearchFilesResultsPhysicalFiles() {
	    return &m_listLogicalPhysicalFilesInfo;
	}

}; // End SearchFilesTask

/////////////////////////////////////////////////////////////////////////////
// SearchFilesTotalTask

class SearchFilesTotalTask : public DiomedeServiceTask
{
private:
    SearchFileFilterImpl*               m_pSearchFilter;
    FilesTotalLogEntryImpl              m_fileTotalLogEntry;

public:
	SearchFilesTotalTask(std::string szSessionToken, class SearchFileFilterImpl* pSearchFilter);

	virtual ~SearchFilesTotalTask() {};

	virtual BOOL Task();

	class FilesTotalLogEntryImpl* GetSearchFilesTotalResults() {
	    return &m_fileTotalLogEntry;
	}

}; // End SearchFilesTotalTask

/////////////////////////////////////////////////////////////////////////////
// SearchFilesTotalLogTask

class SearchFilesTotalLog : public DiomedeServiceTask
{
private:
    SearchLogFilterImpl*                m_pSearchFilter;
    FilesTotalLogListImpl               m_listFilesTotalLog;

public:
	SearchFilesTotalLog(std::string szSessionToken, class SearchLogFilterImpl* pSearchFilter);

	virtual ~SearchFilesTotalLog() {};

	virtual BOOL Task();

	class FilesTotalLogListImpl* GetSearchFilesTotalLogResults() {
	    return &m_listFilesTotalLog;
	}

}; // End SearchFilesTotalLog

/////////////////////////////////////////////////////////////////////////////
// RenameFileTask
class RenameFileTask : public DiomedeServiceTask
{

private:
    LONG64                              m_l64FileID;
    std::string                         m_szNewFileName;

public:
	RenameFileTask(std::string szSessionToken, LONG64 l64FileID, std::string szNewFileName);
	virtual ~RenameFileTask() {};

	virtual BOOL Task();

}; // End RenameFileTask

/////////////////////////////////////////////////////////////////////////////
// DeleteFileTask
class DeleteFileTask : public DiomedeServiceTask
{

private:
    LONG64                              m_l64FileID;

public:
	DeleteFileTask(std::string szSessionToken, LONG64 l64FileID);
	virtual ~DeleteFileTask() {};

	virtual BOOL Task();

}; // End DeleteFileTask

/////////////////////////////////////////////////////////////////////////////
// UndeleteFileTask
class UndeleteFileTask : public DiomedeServiceTask
{

private:
    LONG64                              m_l64FileID;

public:
	UndeleteFileTask(std::string szSessionToken, LONG64 l64FileID);
	virtual ~UndeleteFileTask() {};

	virtual BOOL Task();

}; // End UndeleteFileTask

/////////////////////////////////////////////////////////////////////////////
// CreateMetaDataTask
class CreateMetaDataTask : public DiomedeServiceTask
{

private:
    MetaDataInfoImpl*                   m_pMetaDataInfo;

public:
	CreateMetaDataTask(std::string szSessionToken, MetaDataInfoImpl* pMetaDataInfo);
	virtual ~CreateMetaDataTask() {};

	virtual BOOL Task();

}; // End CreateMetaDataTask

/////////////////////////////////////////////////////////////////////////////
// CreateFileMetaDataTask
class CreateFileMetaDataTask : public DiomedeServiceTask
{

private:
    LONG64                              m_l64FileID;
    MetaDataInfoImpl*                   m_pMetaDataInfo;

public:
	CreateFileMetaDataTask(std::string szSessionToken, LONG64 l64FileID,
	                       MetaDataInfoImpl* pMetaDataInfo);
	virtual ~CreateFileMetaDataTask() {};

	virtual BOOL Task();

}; // End CreateFileMetaDataTask

/////////////////////////////////////////////////////////////////////////////
// SetFileMetaDataTask
class SetFileMetaDataTask : public DiomedeServiceTask
{
private:
    LONG64                              m_l64FileID;
    int                                 m_nMetaDataID;

public:
	SetFileMetaDataTask(std::string szSessionToken, LONG64 l64FileID, int nMetaDataID);
	virtual ~SetFileMetaDataTask() {};

	//--------------------------------------------------------------------
    // Getters and setters - special for this task to allow re-use
    // over many file uploads.
	//--------------------------------------------------------------------
    LONG64 GetFileID() { return m_l64FileID; };
    void SetFileID(const LONG64& nInLong64) {
        m_l64FileID = nInLong64;
    }

    int GetMetaDataID() { return m_nMetaDataID; };
    void SetMetaDataID(const int& nInInt) {
        m_nMetaDataID = nInInt;
    }

	//--------------------------------------------------------------------
	// Overrides
	//--------------------------------------------------------------------
	virtual BOOL Task();

}; // End SetFileMetaDataTask

/////////////////////////////////////////////////////////////////////////////
// DeleteMetaDataTask
class DeleteFileMetaDataTask : public DiomedeServiceTask
{

private:
    LONG64                              m_l64FileID;
    int                                 m_nMetaDataID;

public:
	DeleteFileMetaDataTask(std::string szSessionToken, LONG64 l64FileID, int nMetaDataID);
	virtual ~DeleteFileMetaDataTask() {};

	virtual BOOL Task();

}; // End DeleteFileMetaDataTask

/////////////////////////////////////////////////////////////////////////////
// DeleteMetaDataTask
class DeleteMetaDataTask : public DiomedeServiceTask
{

private:
    int                                 m_nMetaDataID;

public:
	DeleteMetaDataTask(std::string szSessionToken, int nMetaDataID);
	virtual ~DeleteMetaDataTask() {};

	virtual BOOL Task();

}; // End DeleteMetaDataTask

/////////////////////////////////////////////////////////////////////////////
// GetFileMetaDataTask
class GetFileMetaDataTask : public DiomedeServiceTask
{

private:
    LONG64                              m_l64FileID;
    MetaDataListImpl                    m_listMetaData;

public:
	GetFileMetaDataTask(std::string szSessionToken, LONG64 l64FileID);
	virtual ~GetFileMetaDataTask() {};

	virtual BOOL Task();

	class MetaDataListImpl* GetFileMetaDataResults() {
	    return &m_listMetaData;
	}

}; // End GetFileMetaDataTask

/////////////////////////////////////////////////////////////////////////////
// GetMetaDataTask
class GetMetaDataTask : public DiomedeServiceTask
{

private:
    MetaDataInfoImpl*                   m_pMetaDataInfo;
    MetaDataListImpl                    m_listMetaData;

public:
	GetMetaDataTask(std::string szSessionToken, MetaDataInfoImpl* pMetaDataInfo);
	virtual ~GetMetaDataTask() {};

	virtual BOOL Task();

	class MetaDataListImpl* GetMetaDataResults() {
	    return &m_listMetaData;
	}

}; // End GetMetaDataTask

/////////////////////////////////////////////////////////////////////////////
// EditMetaDataTask
class EditMetaDataTask : public DiomedeServiceTask
{

private:
    MetaDataInfoImpl*                   m_pMetaDataInfo;

public:
	EditMetaDataTask(std::string szSessionToken, MetaDataInfoImpl* pMetaDataInfo);
	virtual ~EditMetaDataTask() {};

	virtual BOOL Task();

}; // End EditMetaDataTask

/////////////////////////////////////////////////////////////////////////////
// ReplicateFileTask
class ReplicateFileTask : public DiomedeServiceTask
{

private:
    PhysicalFileInfoImpl*               m_pPhysicalFileInfo;

public:
	ReplicateFileTask(std::string szSessionToken,
	                  PhysicalFileInfoImpl* pPhysicalFileInfo);
	virtual ~ReplicateFileTask() {};

	virtual BOOL Task();

}; // End ReplicateFileTask

/////////////////////////////////////////////////////////////////////////////
// UnReplicateFileTask
class UnReplicateFileTask : public DiomedeServiceTask
{

private:
    LONG64                              m_l64FileID;

public:
	UnReplicateFileTask(std::string szSessionToken, LONG64 l64FileID);
	virtual ~UnReplicateFileTask() {};

	virtual BOOL Task();

}; // End UnReplicateFileTask

/////////////////////////////////////////////////////////////////////////////
// GetStorageTypesTask
class GetStorageTypesTask : public DiomedeServiceTask
{
private:
    StorageTypeListImpl*                m_pListStorageType;
    bool                                m_bDestroyList;

public:
	GetStorageTypesTask(StorageTypeListImpl* pListStorageType);
	virtual ~GetStorageTypesTask();

	virtual BOOL Task();

	class StorageTypeListImpl* GetStorageTypesResults() {
	    return m_pListStorageType;
	}

}; // End GetStorageTypesTask

/////////////////////////////////////////////////////////////////////////////
// GetPhysicalFilesTask
class GetPhysicalFilesTask : public DiomedeServiceTask
{

private:
    LONG64                              m_l64FileID;
    LONG64                              m_l64PhysicalFileID;
    PhysicalFileInfoListImpl            m_listPhysicalFileInfo;
    PhysicalFileInfoImpl                m_physicalFileEntry;

public:
	GetPhysicalFilesTask(std::string szSessionToken, LONG64 l64FileID, LONG64 l64PhysicalFileID=0);
	virtual ~GetPhysicalFilesTask() {};

	virtual BOOL Task();

	class PhysicalFileInfoListImpl* GetPhysicalFileInfoResults() {
	    return &m_listPhysicalFileInfo;
	}

	class PhysicalFileInfoImpl* GetPhysicalFileInfoResult() {
	    return &m_physicalFileEntry;
	}

}; // End GetPhysicalFilesTask

/////////////////////////////////////////////////////////////////////////////
// CreateReplicationPolicyTask
class CreateReplicationPolicyTask : public DiomedeServiceTask
{

private:
    ReplicationPolicyInfoImpl*          m_pReplicationPolicyInfo;

public:
	CreateReplicationPolicyTask(std::string szSessionToken,
	                            ReplicationPolicyInfoImpl* pReplicationPolicyInfo);
	virtual ~CreateReplicationPolicyTask() {};

	virtual BOOL Task();

}; // End CreateReplicationPolicyTask

/////////////////////////////////////////////////////////////////////////////
// GetReplicationPoliciesTask
class GetReplicationPoliciesTask : public DiomedeServiceTask
{

private:
    ReplicationPolicyInfoListImpl      m_listReplicationPolicies;

public:
	GetReplicationPoliciesTask(std::string szSessionToken);
	virtual ~GetReplicationPoliciesTask() {};

	class ReplicationPolicyInfoListImpl* GetReplicationPoliciesResults() {
	    return &m_listReplicationPolicies;
	}

	virtual BOOL Task();

}; // End GetReplicationPoliciesTask

/////////////////////////////////////////////////////////////////////////////
// EditReplicationPolicyTask
class EditReplicationPolicyTask : public DiomedeServiceTask
{

private:
    ReplicationPolicyInfoImpl*          m_pReplicationPolicyInfo;

public:
	EditReplicationPolicyTask(std::string szSessionToken,
	                          ReplicationPolicyInfoImpl* pReplicationPolicyInfo);
	virtual ~EditReplicationPolicyTask() {};

	virtual BOOL Task();

}; // End EditReplicationPolicyTask

/////////////////////////////////////////////////////////////////////////////
// DeleteReplicationPolicyTask
class DeleteReplicationPolicyTask : public DiomedeServiceTask
{

private:
    int                                 m_nReplicationPolicyID;

public:
	DeleteReplicationPolicyTask(std::string szSessionToken, int nReplicationPolicyID);
	virtual ~DeleteReplicationPolicyTask() {};

	virtual BOOL Task();

}; // End DeleteReplicationPolicyTask

/////////////////////////////////////////////////////////////////////////////
// SetReplicationPolicyTask
class SetReplicationPolicyTask : public DiomedeServiceTask
{

private:
    LONG64                              m_l64FileID;
    int                                 m_nReplicationPolicyID;

public:
	SetReplicationPolicyTask(std::string szSessionToken, LONG64 l64FileID,
	                                                     int nReplicationPolicyID);
	virtual ~SetReplicationPolicyTask() {};

	virtual BOOL Task();

}; // End SetReplicationPolicyTask

/////////////////////////////////////////////////////////////////////////////
// SetDefaultReplicationPolicyTask
class SetDefaultReplicationPolicyTask : public DiomedeServiceTask
{

private:
    int                                 m_nReplicationPolicyID;

public:
	SetDefaultReplicationPolicyTask(std::string szSessionToken, int nReplicationPolicyID);
	virtual ~SetDefaultReplicationPolicyTask() {};

	virtual BOOL Task();

}; // End SetDefaultReplicationPolicyTask

/////////////////////////////////////////////////////////////////////////////
// GetDefaultReplicationPolicyTask
class GetDefaultReplicationPolicyTask : public DiomedeServiceTask
{

private:
    ReplicationPolicyInfoImpl*          m_pReplicationPolicyInfo;

public:
	GetDefaultReplicationPolicyTask(std::string szSessionToken,
	                                ReplicationPolicyInfoImpl* pReplicationPolicyInfo);
	virtual ~GetDefaultReplicationPolicyTask() {};

	virtual BOOL Task();

}; // End GetDefaultReplicationPolicyTask

/////////////////////////////////////////////////////////////////////////////
// SearchUploadLogTask
class SearchUploadLogTask : public DiomedeServiceTask
{

private:
    SearchUploadLogFilterImpl*          m_pSearchFilter;
    UploadLogListImpl                   m_listUploadLog;

public:
	SearchUploadLogTask( std::string szSessionToken, SearchUploadLogFilterImpl* pSearchFilter );
	virtual ~SearchUploadLogTask() {};

	virtual BOOL Task();

	class UploadLogListImpl* GetSearchUploadLogResults() {
	    return &m_listUploadLog;
	}

}; // End SearchUploadLogTask

/////////////////////////////////////////////////////////////////////////////
// SearchDownloadLogTask
class SearchDownloadLogTask : public DiomedeServiceTask
{

private:
    SearchDownloadLogFilterImpl*        m_pSearchFilter;
    DownloadLogListImpl                 m_listDownloadLog;

public:
	SearchDownloadLogTask( std::string szSessionToken, SearchDownloadLogFilterImpl* pSearchFilter );
	virtual ~SearchDownloadLogTask() {};

	virtual BOOL Task();

	class DownloadLogListImpl* GetSearchDownloadLogResults() {
	    return &m_listDownloadLog;
	}

}; // End SearchDownloadLogTask

/////////////////////////////////////////////////////////////////////////////
// SearchLoginLogTask
class SearchLoginLogTask : public DiomedeServiceTask
{

private:
    SearchLoginLogFilterImpl*           m_pSearchFilter;
    LoginLogListImpl                    m_listLoginLog;

public:
	SearchLoginLogTask( std::string szSessionToken, SearchLoginLogFilterImpl* pSearchFilter );
	virtual ~SearchLoginLogTask() {};

	virtual BOOL Task();

	class LoginLogListImpl* GetSearchLoginLogResults() {
	    return &m_listLoginLog;
	}

}; // End SearchLoginLogTask

/////////////////////////////////////////////////////////////////////////////
// GetAllProductsTask
class GetAllProductsTask : public DiomedeServiceTask
{

private:
   ProductListImpl                      m_listProducts;

public:
	GetAllProductsTask();
	virtual ~GetAllProductsTask() {};

	virtual BOOL Task();

	class ProductListImpl* GetProductResults() {
	    return &m_listProducts;
	}

}; // End GetAllProductsTask

/////////////////////////////////////////////////////////////////////////////
// PurchaseProductTask
class PurchaseProductTask : public DiomedeServiceTask
{

private:
    int                                 m_nProductID;

public:
	PurchaseProductTask( std::string szSessionToken, int nProductID );
	virtual ~PurchaseProductTask() {};

	virtual BOOL Task();

}; // End PurchaseProductTask

/////////////////////////////////////////////////////////////////////////////
// GetMyProductsTask
class GetMyProductsTask : public DiomedeServiceTask
{

private:
    UserProductListImpl                 m_listUserProducts;

public:
	GetMyProductsTask( std::string szSessionToken );
	virtual ~GetMyProductsTask() {};

	virtual BOOL Task();

	class UserProductListImpl* GetUserProductResults() {
	    return &m_listUserProducts;
	}

}; // End GetMyProductsTask

/////////////////////////////////////////////////////////////////////////////
// CancelProductTask
class CancelProductTask : public DiomedeServiceTask
{

private:
    int                                 m_nProductID;

public:
	CancelProductTask( std::string szSessionToken, int nProductID );
	virtual ~CancelProductTask() {};

	virtual BOOL Task();

}; // End CancelProductTask

/////////////////////////////////////////////////////////////////////////////
// GetAllContractsTask
class GetAllContractsTask : public DiomedeServiceTask
{

private:
    ContractListImpl                    m_listContracts;

public:
	GetAllContractsTask();
	virtual ~GetAllContractsTask() {};

	virtual BOOL Task();

	class ContractListImpl* GetContractResults() {
	    return &m_listContracts;
	}

}; // End GetAllContractsTask

/////////////////////////////////////////////////////////////////////////////
// PurchaseContractTask
class PurchaseContractTask : public DiomedeServiceTask
{

private:
    int                                 m_nContractID;
    LONG64								m_l64UserContractID;

public:
	PurchaseContractTask( std::string szSessionToken, int nContractID );
	virtual ~PurchaseContractTask() {};

	LONG64 GetUserContractID() {
		return m_l64UserContractID;
	}

	virtual BOOL Task();

}; // End PurchaseContractTask

/////////////////////////////////////////////////////////////////////////////
// GetMyContractsTask
class GetMyContractsTask : public DiomedeServiceTask
{

private:
    UserContractListImpl                m_listUserContracts;

public:
	GetMyContractsTask( std::string szSessionToken );
	virtual ~GetMyContractsTask() {};

	virtual BOOL Task();

	class UserContractListImpl* GetUserContractResults() {
	    return &m_listUserContracts;
	}

}; // End GetMyContractsTask

/////////////////////////////////////////////////////////////////////////////
// CancelContractTask
class CancelContractTask : public DiomedeServiceTask
{

private:
    LONG64                              m_l64ContractID;

public:
	CancelContractTask( std::string szSessionToken, LONG64 l64ContractID );
	virtual ~CancelContractTask() {};

	virtual BOOL Task();

}; // End CancelContractTask

/////////////////////////////////////////////////////////////////////////////
// SearchPaymentLogTask
class SearchPaymentLogTask : public DiomedeServiceTask
{

private:
    time_t                              m_tStartDate;
    time_t                              m_tEndDate;
    PaymentLogListImpl                  m_listPaymentLog;

public:
	SearchPaymentLogTask(std::string szSessionToken, time_t tStartDate, time_t tEndDate);
	virtual ~SearchPaymentLogTask() {};

	virtual BOOL Task();

	class PaymentLogListImpl* GetSearchPaymentLogResults() {
	    return &m_listPaymentLog;
	}

}; // End SearchPaymentLogTask

/////////////////////////////////////////////////////////////////////////////
// SearchInvoiceLogTask
class SearchInvoiceLogTask : public DiomedeServiceTask
{

private:
    SearchLogFilterImpl*                m_pSearchFilter;
    InvoiceLogListImpl                  m_listInvoiceLogEntries;

public:
	SearchInvoiceLogTask( std::string szSessionToken, SearchLogFilterImpl* pSearchFilter);
	virtual ~SearchInvoiceLogTask() {};

	virtual BOOL Task();

	class InvoiceLogListImpl* GetSearchInvoiceLogEntriesResults() {
	    return &m_listInvoiceLogEntries;
	}

}; // End SearchInvoiceLogTask

} // namespace DIOMEDE_CONSOLE

#endif // __DIOMEDE_TASKS_H__
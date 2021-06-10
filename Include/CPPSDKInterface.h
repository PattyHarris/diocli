/*********************************************************************
 *
 *  file:  CPPSDKInteface.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 *
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 *  Purpose: Defines all class interfaces that extend beyond DLL boundaries.
 *          (see How to Create a Heap-Safe and Binary Compatible DLL
 *          in C++, DigCode).
 *
 * Each attribute and service definition has the following format:
 *
 *  String Format
 *  virtual std::string Get() = 0;
 *  virtual void Set(const std::string& szInStr) = 0;
 *
 *  Unsigned char Format
 *  virtual unsigned char Get() = 0;
 *  virtual void Set(const unsigned char& szInChar) = 0;
 *
 *  Date/time Format
 *  virtual time_t Get() = 0;
 *  virtual void Set(const time_t& nInDate) = 0;
 *
 *  Int Format
 *  virtual int Get() = 0;
 *  virtual void Set(const int& nInInt) = 0;
 *
 *  Double Format
 *  virtual double Get() = 0;
 *  virtual void Set(const double& nInDouble) = 0;
 *
 *  LONG64 Format
 *  virtual LONG64 Get() = 0;
 *  virtual void Set(const LONG64& nInLong64) = 0;
 *
 *  Enum Format
 *  virtual Type GetType() = 0;
 *  virtual void SetType(const Type& nInType) = 0;
 *
 *********************************************************************/

#pragma once
#include <string>
#include "../CPPSDK.Lib/stdafx.h"
#include "types.h"
#include "DiomedeDLLInterface.h"
#include "DiomedeSDKDefs.h"

#include <map>
#include <vector>

typedef bool (*StatusFunc)(void*, int nStatus);
typedef bool (*UploadFileFunc)(void*, int nUploadStatus, LONG64 l64ByteCount);
typedef bool (*DownloadFileFunc)(void*, int nDownloadStatus, LONG64 lByteCount,
                                 LONG64 lTotalBytes);
typedef bool (*DownloadURLFunc)(void*, int nDownloadStatus, std::string szDownloadURL,
                                std::string szDownloadFileName);
typedef bool (*DisplayFileFunc)(void*, int nDisplayFileStatus, LONG64 lByteCount,
                                LONG64 lTotalBytes);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Service Attributes

/////////////////////////////////////////////////////////////////////////////
// User Management

/////////////////////////////////////////////////////////////////////////////
// ICreateUser
//      CreateUser info interfaces exposed across a DLL boundry.
class ICreateUser : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetUserName() = 0;
    virtual void SetUserName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetPassword() = 0;
    virtual void SetPassword(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetEmail() = 0;
    virtual void SetEmail(const std::string& szInStr) = 0;

}; // End ICreateUser

/////////////////////////////////////////////////////////////////////////////
// IUserInfo
//      User info interfaces exposed across a DLL boundry.
class IUserInfo : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------
public:

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetCreatedDate() = 0;
    virtual void SetCreatedDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetEmail() = 0;
    virtual void SetEmail(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCompanyName() = 0;
    virtual void SetCompanyName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetWebSiteURL() = 0;
    virtual void SetWebSiteURL(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetUserName() = 0;
    virtual void SetUserName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetLastName() = 0;
    virtual void SetLastName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetFirstName() = 0;
    virtual void SetFirstName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetPhone() = 0;
    virtual void SetPhone(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCardName() = 0;
    virtual void SetCardName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCardNumber() = 0;
    virtual void SetCardNumber(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetCardExpiryYear() = 0;
    virtual void SetCardExpiryYear(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetCardExpiryMonth() = 0;
    virtual void SetCardExpiryMonth(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCardCVV2() = 0;
    virtual void SetCardCVV2(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCardAddress1() = 0;
    virtual void SetCardAddress1(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCardAddress2() = 0;
    virtual void SetCardAddress2(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCardCity() = 0;
    virtual void SetCardCity(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCardState() = 0;
    virtual void SetCardState(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCardZip() = 0;
    virtual void SetCardZip(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCardCountry() = 0;
    virtual void SetCardCountry(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual DIOMEDE::UserAccountType GetUserAccountType() = 0;
    virtual void SetUserAccountType(const DIOMEDE::UserAccountType& nInType) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetUserAccountTypeName() = 0;
    virtual void SetUserAccountTypeName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetMinMonthlyFee() = 0;
    virtual void SetMinMonthlyFee(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetMonthlySupportFee() = 0;
    virtual void SetMonthlySupportFee(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual DIOMEDE::PaymentMethodsType GetPaymentMethodsType() = 0;
    virtual void SetPaymentMethodsType(const DIOMEDE::PaymentMethodsType& nInType) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetPaymentDate() = 0;
    virtual void SetPaymentDate(const time_t& nInDate) = 0;

}; // End IUserInfo

/////////////////////////////////////////////////////////////////////////////
// IEmailList
//      Email list interface exposed across a DLL boundry.
class IEmailList : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
public:
    virtual const std::vector<std::string> GetEmailList() const  = 0;
    virtual void AddEmailEntry(std::string szEmailEntry) = 0;

}; // End IEmailList

/////////////////////////////////////////////////////////////////////////////
// ILoginLogEntry
//      Login log entry interfaces exposed across a DLL boundry.
class ILoginLogEntry : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetUserID() = 0;
    virtual void SetUserID(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetLoginDate() = 0;
    virtual void SetLoginDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetLoginIP() = 0;
    virtual void SetLoginIP(const std::string& szInStr) = 0;

}; // End ILoginLogEntry

/////////////////////////////////////////////////////////////////////////////
// ILoginLogList
//      Login log interfaces exposed across a DLL boundry.
class ILoginLogList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetLoginLogList() const  = 0;
    virtual void AddLoginLogEntry(void* pLoginLogEntry) = 0;

}; // End ILoginLogList

/////////////////////////////////////////////////////////////////////////////
// File Management

/////////////////////////////////////////////////////////////////////////////
// ISearchFileFilter
//      Search filter interfaces exposed across a DLL boundry.
class ISearchFileFilter : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------
public:

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetFileName() = 0;
    virtual void SetFileName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetHashMD5() = 0;
    virtual void SetHashMD5(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetHashSHA1() = 0;
    virtual void SetHashSHA1(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetMinSize() = 0;
    virtual void SetMinSize(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetMaxSize() = 0;
    virtual void SetMaxSize(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetDateStart() = 0;
    virtual void SetDateStart(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetDateEnd() = 0;
    virtual void SetDateEnd(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual DIOMEDE::BoolStatusType GetIsDeleted() = 0;
    virtual void SetIsDeleted(const DIOMEDE::BoolStatusType& nInType) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual DIOMEDE::BoolStatusType GetIsCompleted() = 0;
    virtual void SetIsCompleted(const DIOMEDE::BoolStatusType& nInType) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetMetaDataName() = 0;
    virtual void SetMetaDataName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetMetaDataValue() = 0;
    virtual void SetMetaDataValue(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetOffset() = 0;
    virtual void SetOffset(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetPageSize() = 0;
    virtual void SetPageSize(const LONG64& nInLong64) = 0;

}; // End ISearchFileFilter

/////////////////////////////////////////////////////////////////////////////
// ISearchLogFilter
//      Log search filter interfaces exposed across a DLL boundry.
class ISearchLogFilter : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetDateStart() = 0;
    virtual void SetDateStart(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetDateEnd() = 0;
    virtual void SetDateEnd(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetStatus() = 0;
    virtual void SetStatus(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetOffset() = 0;
    virtual void SetOffset(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetPageSize() = 0;
    virtual void SetPageSize(const LONG64& nInLong64) = 0;


}; // End ISearchLogFilter

/////////////////////////////////////////////////////////////////////////////
// ISearchUploadLogFilter
//      Search Upload log filter interfaces exposed across a DLL boundry.
class ISearchUploadLogFilter : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetUploadIP() = 0;
    virtual void SetUploadIP(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetDateStart() = 0;
    virtual void SetDateStart(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetDateEnd() = 0;
    virtual void SetDateEnd(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetOffset() = 0;
    virtual void SetOffset(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetPageSize() = 0;
    virtual void SetPageSize(const LONG64& nInLong64) = 0;

}; // End ISearchUploadLogFilter

/////////////////////////////////////////////////////////////////////////////
// ISearchDownloadLogFilter
//      Search Download log filter interfaces exposed across a DLL boundry.
class ISearchDownloadLogFilter : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloadIP() = 0;
    virtual void SetDownloadIP(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloadToken()= 0;
    virtual void SetDownloadToken(const std::string& szInStr)= 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetDateStart() = 0;
    virtual void SetDateStart(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetDateEnd() = 0;
    virtual void SetDateEnd(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetOffset() = 0;
    virtual void SetOffset(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetPageSize() = 0;
    virtual void SetPageSize(const LONG64& nInLong64) = 0;

}; // End ISearchDownloadLogFilter

/////////////////////////////////////////////////////////////////////////////
// ISearchLoginLogFilter
//      Search login log filter interfaces exposed across a DLL boundry.
class ISearchLoginLogFilter : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetLoginIP() = 0;
    virtual void SetLoginIP(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetDateStart() = 0;
    virtual void SetDateStart(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetDateEnd() = 0;
    virtual void SetDateEnd(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetOffset() = 0;
    virtual void SetOffset(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetPageSize() = 0;
    virtual void SetPageSize(const LONG64& nInLong64) = 0;

}; // End ISearchLoginLogFilter

/////////////////////////////////////////////////////////////////////////////
// IFileProperties
//      Search filter interfaces exposed across a DLL boundry.
class IFileProperties : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetReplicationPolicyID() = 0;
    virtual void SetReplicationPolicyID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetFileName() = 0;
    virtual void SetFileName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetHashMD5() = 0;
    virtual void SetHashMD5(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetHashSHA1() = 0;
    virtual void SetHashSHA1(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileSize() = 0;
    virtual void SetFileSize(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetLastModifiedDate() = 0;
    virtual void SetLastModifiedDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetLastAccess() = 0;
    virtual void SetLastAccess(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual bool GetIsDeleted() = 0;
    virtual void SetIsDeleted(const bool& nInBool) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual bool GetIsCompleted() = 0;
    virtual void SetIsCompleted(const bool& nInBool) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual double GetPower() = 0;
    virtual void SetPower(const double& nInDouble) = 0;

}; // End IFileProperties

/////////////////////////////////////////////////////////////////////////////
// IFilePropertiesList
//      File properties list interfaces exposed across a DLL boundry.
class IFilePropertiesList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetFilePropertiesList() const  = 0;
    virtual void AddFilePropertyEntry(void* pFileProperty) = 0;

}; // End IFilePropertiesList

/////////////////////////////////////////////////////////////////////////////
// IFileSearchSummary
//      File search summary interfaces exposed across a DLL boundry.
class IFileSearchSummary : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetFileCount() = 0;
    virtual void SetFileCount(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetByteCount() = 0;
    virtual void SetByteCount(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual double GetWatts() = 0;
    virtual void SetWatts(const double& nInDouble) = 0;

}; // End IFileSearchSummary

/////////////////////////////////////////////////////////////////////////////
// IFilesTotalStorageEntry
//      Files total storage list entry interfaces exposed across a DLL boundry.
class IFilesTotalStorageEntry : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetStorageTypeID() = 0;
    virtual void SetStorageTypeID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetStorageName() = 0;
    virtual void SetStorageName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileBytes() = 0;
    virtual void SetFileBytes(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetFileCount() = 0;
    virtual void SetFileCount(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual double GetPower() = 0;
    virtual void SetPower(const double& nInDouble)  = 0;

}; // End IFilesTotalStorageEntry

/////////////////////////////////////////////////////////////////////////////
// IFilesTotalStorageList
//      Files total storage list interfaces exposed across a DLL boundry.
class IFilesTotalStorageList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetFileTotalStorageList() const = 0;
    virtual void AddFileTotalStorageEntry(void* pFileTotalStorageEntry) = 0;

}; // End IFilesTotalStorageList

/////////////////////////////////////////////////////////////////////////////
// IFilesTotalLogEntry
//      File total log entry interfaces exposed across a DLL boundry.
class IFilesTotalLogEntry : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetUploadCount() = 0;
    virtual void SetUploadCount(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetUploadBytes() = 0;
    virtual void SetUploadBytes(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetDownloadCount() = 0;
    virtual void SetDownloadCount(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetDownloadBytes() = 0;
    virtual void SetDownloadBytes(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetDeleteCount() = 0;
    virtual void SetDeleteCount(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetDeleteBytes() = 0;
    virtual void SetDeleteBytes(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetFileCount() = 0;
    virtual void SetFileCount(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileBytes() = 0;
    virtual void SetFileBytes(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetCreatedDate() = 0;
    virtual void SetCreatedDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual IFilesTotalStorageList* GetFilesTotalStorageList() = 0;

}; // End IFilesTotalLogEntry

/////////////////////////////////////////////////////////////////////////////
// IFilesTotalLogList
//      Search files total interfaces exposed across a DLL boundry.
class IFilesTotalLogList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetTotalResults() = 0;
    virtual void SetTotalResults(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetFileTotalLogList() const = 0;
    virtual void AddFileTotalLogEntry(void* pFileTotalLogEntry) = 0;

}; // End IFilesTotalLogList

/////////////////////////////////////////////////////////////////////////////
// IUpload
//      Upload interfaces exposed across a DLL boundry.
class IUpload : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual void* GetUploadUser() = 0;
    virtual void SetUploadUser(void* pInVoidPtr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual UploadFileFunc GetUploadCallback() = 0;
    virtual void SetUploadCallback(UploadFileFunc pfnCallback) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual bool GetUseMD5Hash() = 0;
    virtual void SetUseMD5Hash(const bool& nInBool) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetHashMD5() = 0;
    virtual void SetHashMD5(const std::string& szInStr)  = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetFilePath() = 0;
    virtual void SetFilePath(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetFileName() = 0;
    virtual void SetFileName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetTotalUploadBytes() = 0;
    virtual void SetTotalUploadBytes(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetMinChunkSize() = 0;
    virtual void SetMinChunkSize(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetMaxChunkSize() = 0;
    virtual void SetMaxChunkSize(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual bool GetLogStatus() = 0;
    virtual void SetLogStatus(const bool& nInBool) = 0;

}; // End IUpload

/////////////////////////////////////////////////////////////////////////////
// IUploadToken
//      UploadToken interfaces exposed across a DLL boundry.
class IUploadToken : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetFileName() = 0;
    virtual void SetFileName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCallbackAddress() = 0;
    virtual void SetCallbackAddress(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetReserveBytes() = 0;
    virtual void SetReserveBytes(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetUploadToken() = 0;
    virtual void SetUploadToken(const std::string& szInStr) = 0;

}; // End UploadTokenImpl

/////////////////////////////////////////////////////////////////////////////
// IUploadLogEntry
//      Upload log entry interfaces exposed across a DLL boundry.
class IUploadLogEntry : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetUploadTime() = 0;
    virtual void SetUploadTime(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetUploaderIP() = 0;
    virtual void SetUploaderIP(const std::string& szInStr) = 0;

}; // End IUploadLogEntry

/////////////////////////////////////////////////////////////////////////////
// IUploadLogList
//      Upload log interfaces exposed across a DLL boundry.
class IUploadLogList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetUploadLogList() const  = 0;
    virtual void AddUploadLogEntry(void* pUploadLogEntry) = 0;

}; // End IUploadLogList

/////////////////////////////////////////////////////////////////////////////
// IDownload
//      Download interfaces exposed across a DLL boundry.
class IDownload : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual DownloadFileFunc GetDownloadCallback() = 0;
    virtual void SetDownloadCallback(DownloadFileFunc pfnCallback) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual DownloadURLFunc GetDownloadURLCallback() = 0;
    virtual void SetDownloadURLCallback(DownloadURLFunc pfnCallback) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloadPath() = 0;
    virtual void SetDownloadPath(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloadURL() = 0;
    virtual void SetDownloadURL(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloadFileName() = 0;
    virtual void SetDownloadFileName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

private:

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetTotalDownloadBytes() = 0;
    virtual void SetTotalDownloadBytes(const LONG64& nInLong64) = 0;

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual bool GetCancelDownload() = 0;
    virtual void SetCancelDownload(const bool& nInBool) = 0;

    //-----------------------------------------------------------------
    // Curl interface
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GeDownloadResult() = 0;
    virtual void SetDownloadResult(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloadResultString() = 0;
    virtual void SetDownloadResultString(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetProxyHost() = 0;
    virtual void SetProxyHost(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetProxyPort() = 0;
    virtual void SetProxyPort(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetProxyUserID() = 0;
    virtual void SetProxyUserID(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetProxyPassword() = 0;
    virtual void SetProxyPassword(const std::string& szInStr) = 0;

}; // End IDownload

/////////////////////////////////////////////////////////////////////////////
// IDisplayFileContents
//      Display File Contents interfaces exposed across a DLL boundry.
class IDisplayFileContents : public DiomedeDLLInterface
{

    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual void* GetDisplayFileUser() = 0;
    virtual void SetDisplayFileUser(void* pInVoidPtr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual DisplayFileFunc GetDisplayFileCallback() = 0;
    virtual void SetDisplayFileCallback(DownloadFileFunc pfnCallback)  = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual DownloadURLFunc GetDownloadURLCallback() = 0;
    virtual void SetDownloadURLCallback(DownloadURLFunc pfnCallback) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual FILE* GetDisplayFileOutputStream() = 0;
    virtual void SetDisplayFileOutputStream(FILE* pOutputStream) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloadURL() = 0;
    virtual void SetDownloadURL(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloadFileName() = 0;
    virtual void SetDownloadFileName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetStartByte() = 0;
    virtual void SetStartByte(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetEndByte() = 0;
    virtual void SetEndByte(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetFormattedDisplayRange() = 0;
    virtual void SetFormattedDisplayRange(const std::string& szInStr) = 0;

private:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetTotalDownloadBytes() = 0;
    virtual void SetTotalDownloadBytes(const LONG64& nInLong64) = 0;

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual bool GetCancelDisplayFile() = 0;
    virtual void SetCancelDisplayFile(const bool& nInBool) = 0;

    //-----------------------------------------------------------------
    // Curl interface
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GeDisplayFileResult() = 0;
    virtual void SetDisplayFileResult(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDisplayFileResultString() = 0;
    virtual void SetDisplayFileResultString(const std::string& szInStr) = 0;
    
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetProxyHost() = 0;
    virtual void SetProxyHost(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetProxyPort() = 0;
    virtual void SetProxyPort(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetProxyUserID() = 0;
    virtual void SetProxyUserID(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetProxyPassword() = 0;
    virtual void SetProxyPassword(const std::string& szInStr) = 0;

}; // End IDisplayFileContents

/////////////////////////////////////////////////////////////////////////////
// IDownloadLogEntry
//      Download log entry interfaces exposed across a DLL boundry.
class IDownloadLogEntry : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetStartTime() = 0;
    virtual void SetStartTime(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetEndTime() = 0;
    virtual void SetEndTime(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloadeToken() = 0;
    virtual void SetDownloadeToken(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloaderIP() = 0;
    virtual void SetDownloaderIP(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetStartByte() = 0;
    virtual void SetStartByte(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetByteCount() = 0;
    virtual void SetByteCount(const LONG64& nInLong64) = 0;

}; // End IDownloadLogEntry

/////////////////////////////////////////////////////////////////////////////
// IDownloadLogList
//      Download log interfaces exposed across a DLL boundry.
class IDownloadLogList : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetDownloadLogList() const  = 0;
    virtual void AddDownloadLogEntry(void* pDownloadLogEntry) = 0;

}; // End IDownloadLogList

/////////////////////////////////////////////////////////////////////////////
// IDownloadURL
//      GetDownloadURL interfaces exposed across a DLL boundry.
class IDownloadURL : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GeMaxDownloads() = 0;
    virtual void SetMaxDownloads(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GeLifetimeHours() = 0;
    virtual void SetLifetimeHours(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GeMaxUniqueIPs() = 0;
    virtual void SetMaxUniqueIPs(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetErrorRedirect() = 0;
    virtual void SetErrorRedirect(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDownloadURL() = 0;
    virtual void SetDownloadURL(const std::string& szInStr) = 0;

}; // End IDownloadURL

/////////////////////////////////////////////////////////////////////////////
// IMetaDataInfo
//      MetaDataInfo interfaces exposed across a DLL boundry.
class IMetaDataInfo : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetMetaDataID() = 0;
    virtual void SetMetaDataID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetMetaDataName() = 0;
    virtual void SetMetaDataName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetMetaDataValue() = 0;
    virtual void SetMetaDataValue(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetCreatedDate() = 0;
    virtual void SetCreatedDate(const time_t& nInDate) = 0;

}; // End IMetaDataInfo

/////////////////////////////////////////////////////////////////////////////
// IMetaDataList
//      MetaDataList interfaces exposed across a DLL boundry.
class IMetaDataList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetMetaDataList() const = 0;
    virtual void AddMetaDataInfo(void* pMetaDataInfo) = 0;

}; // End IMetaDataList

/////////////////////////////////////////////////////////////////////////////
// IStorageType
//      StorageType interfaces exposed across a DLL boundry.
class IStorageTypeInfo : public DiomedeDLLInterface
{

    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetStorageTypeID() = 0;
    virtual void SetStorageTypeID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetStorageTypeName() = 0;
    virtual void SetStorageTypeName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual double GetPowerPerMB() = 0;
    virtual void SetPowerPerMB(const double& nInDouble) = 0;

}; // End IStorageTypeInfo

/////////////////////////////////////////////////////////////////////////////
// IStorageTypeList
//      IStorageTypeList interfaces exposed across a DLL boundry.
class IStorageTypeList : public DiomedeDLLInterface
{

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetStorageTypeList() const  = 0;
    virtual void AddStorageTypeInfo(void* pStorageTypeEntry) = 0;

}; // End IStorageTypeList

/////////////////////////////////////////////////////////////////////////////
// IPhysicalFileInfo
//      IPhysicalFileInfo interfaces exposed across a DLL boundry.
class IPhysicalFileInfo : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetFileID() = 0;
    virtual void SetFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetPhysicalFileID() = 0;
    virtual void SetPhysicalFileID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetStorageTypeID() = 0;
    virtual void SetStorageTypeID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual double GetPower() = 0;
    virtual void SetPower(const double& nInDouble) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetCreatedDate() = 0;
    virtual void SetCreatedDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetRequestedDate() = 0;
    virtual void SetRequestedDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetExercisedDate() = 0;
    virtual void SetExercisedDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetExpirationDate() = 0;
    virtual void SetExpirationDate(const time_t& nInDate) = 0;

}; // End IPhysicalFileInfo

/////////////////////////////////////////////////////////////////////////////
// IPhysicalFileInfoList
//      IPhysicalFileInfoList interfaces exposed across a DLL boundry.
class IPhysicalFileInfoList : public DiomedeDLLInterface
{

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetPhysicalFileInfoList() const  = 0;
    virtual void AddPhysicalFileInfo(void* pPhysicalFileEntry) = 0;

}; // End IPhysicalFileInfoList

/////////////////////////////////////////////////////////////////////////////
// ILogicalPhysicalFilesInfoList
//      ILogicalPhysicalFilesInfoList interfaces exposed across a DLL boundry.
class ILogicalPhysicalFilesInfoList : public DiomedeDLLInterface
{

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::map<LONG64, void*> GetLogicalPhysicalFilesInfoList() const = 0;
    virtual void AddPhysicalFileListInfo(LONG64 l64FileID, void*) = 0;
    virtual void AddServiceError(LONG64 l64FileID, int nResult, std::string szErrorMsg) = 0;

    // Returns the list of physical files for a given file ID.
    virtual void* GetPhysicalFileInfoList(LONG64 l64FileID) const  = 0;

    // Returns the service error, if one occurred for the given file ID.
    virtual bool GetServiceError(LONG64 l64FileID, int& nResult, std::string& szErrorMsg) = 0;

}; // End ILogicalPhysicalFilesInfoList

/////////////////////////////////////////////////////////////////////////////
// IReplicationPolicyInfo
//      IReplicationPolicyInfo interfaces exposed across a DLL boundry.
class IReplicationPolicyInfo : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetReplicationPolicyID() = 0;
    virtual void SetReplicationPolicyID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetDefaultOnline() = 0;
    virtual void SetDefaultOnline(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetDefaultNearline() = 0;
    virtual void SetDefaultNearline(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetDefaultOffline() = 0;
    virtual void SetDefaultOffline(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetLastAccessTriggerHours() = 0;
    virtual void SetLastAccessTriggerHours(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetLastAccessTriggeOnline() = 0;
    virtual void SetLastAccessTriggeOnline(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetLastAccessTriggeNearline() = 0;
    virtual void SetLastAccessTriggeNearline(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetLastAccessTriggeOffline() = 0;
    virtual void SetLastAccessTriggeOffline(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetExpiresHours() = 0;
    virtual void SetExpiresHours(const int& nInInt) = 0;

}; // End IReplicationPolicyInfo

/////////////////////////////////////////////////////////////////////////////
// IReplicationPolicyInfoList
//      IReplicationPolicyInfoList interfaces exposed across a DLL boundry.
class IReplicationPolicyInfoList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetReplicationPolicyInfoList() const = 0;
    virtual void AddReplicationPolicyInfo(void* pReplicationPolicyEntry) = 0;

}; // End IReplicationPolicyInfoList

/////////////////////////////////////////////////////////////////////////////
// Product Management

/////////////////////////////////////////////////////////////////////////////
// IComponent
//      Component interfaces exposed across a DLL boundry.
class IComponent : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDescription() = 0;
    virtual void SetDescription(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetStorageTypeID() = 0;
    virtual void SetStorageTypeID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetMeterType() = 0;
    virtual void SetMeterType(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetRatePerGB() = 0;
    virtual void SetRatePerGB(const int& nInInt) = 0;

}; // End IComponent

/////////////////////////////////////////////////////////////////////////////
// IComponentList
//      Component list interfaces exposed across a DLL boundry.
class IComponentList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetComponentList() const  = 0;
    virtual void AddComponentEntry(void* pComponent) = 0;

}; // End IComponentList

/////////////////////////////////////////////////////////////////////////////
// IProduct
//      Product interfaces exposed across a DLL boundry.
class IProduct : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetProductID() = 0;
    virtual void SetProductID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetName() = 0;
    virtual void SetName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDescription() = 0;
    virtual void SetDescription(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetMinMonthlyFee() = 0;
    virtual void SetMinMontlyFee(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetSupportFee() = 0;
    virtual void SetSupportFee(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual IComponentList* GetComponentList() = 0;

}; // End IProduct

/////////////////////////////////////////////////////////////////////////////
// IProductList
//      Product list interfaces exposed across a DLL boundry.
class IProductList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetProductList() const  = 0;
    virtual void AddProductEntry(void* pProduct) = 0;

}; // End IProductList

/////////////////////////////////////////////////////////////////////////////
// IUserProduct
//      User product interfaces exposed across a DLL boundry.
class IUserProduct : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual bool GetHasSupport() = 0;
    virtual void SetHasSupport(const bool& nInBool) = 0;

    //-----------------------------------------------------------------
    // Accessors to IProduct* owned by IUserProduct
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetProductID() = 0;
    virtual void SetProductID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetName() = 0;
    virtual void SetName(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetDescription() = 0;
    virtual void SetDescription(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetMinMonthlyFee() = 0;
    virtual void SetMinMontlyFee(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetSupportFee() = 0;
    virtual void SetSupportFee(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual IComponentList* GetComponentList() = 0;

}; // End IUserProduct

/////////////////////////////////////////////////////////////////////////////
// IUserProductList
//      User product list interfaces exposed across a DLL boundry.
class IUserProductList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetUserProductList() const  = 0;
    virtual void AddUserProductEntry(void* pUserProduct) = 0;

}; // End IUserProductList

/////////////////////////////////////////////////////////////////////////////
// IContractComponent
//      Contract component interfaces exposed across a DLL boundry.
class IContractComponent : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetStorageTypeID() = 0;
    virtual void SetStorageTypeID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetMeterType() = 0;
    virtual void SetMeterType(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetCommittedGB() = 0;
    virtual void SetCommittedGB(const int& nInInt) = 0;

}; // End IContractComponent

/////////////////////////////////////////////////////////////////////////////
// IContractComponentList
//      Contract component list interfaces exposed across a DLL boundry.
class IContractComponentList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetContractComponentList() const  = 0;
    virtual void AddContractComponentEntry(void* pContractComponent) = 0;

}; // End IContractComponentList

/////////////////////////////////////////////////////////////////////////////
// IContract
//      Contract interfaces exposed across a DLL boundry.
class IContract : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetContractID() = 0;
    virtual void SetContractID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetRatePerTerm() = 0;
    virtual void SetRatePerTerm(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetTerm() = 0;
    virtual void SetTerm(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetContractDescription() = 0;
    virtual void SetContractDescription(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual IContractComponentList* GetContractComponentList() = 0;

}; // End IContract

/////////////////////////////////////////////////////////////////////////////
// IContractList
//      Contract list interfaces exposed across a DLL boundry.
class IContractList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetContractList() const  = 0;
    virtual void AddContractEntry(void* pContract) = 0;

}; // End IContractList

/////////////////////////////////////////////////////////////////////////////
// IUserContract
//      User contract interfaces exposed across a DLL boundry.
class IUserContract : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetUserContractID() = 0;
    virtual void SetUserContractID(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetStartDate() = 0;
    virtual void SetStartDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetEndDate() = 0;
    virtual void SetEndDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetContractID() = 0;
    virtual void SetContractID(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetRatePerTerm() = 0;
    virtual void SetRatePerTerm(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetTerm() = 0;
    virtual void SetTerm(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetContractDescription() = 0;
    virtual void SetContractDescription(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual IContractComponentList* GetContractComponentList() = 0;

}; // End IUserContract

/////////////////////////////////////////////////////////////////////////////
// IUserContractList
//      User contract list interfaces exposed across a DLL boundry.
class IUserContractList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetUserContractList() const  = 0;
    virtual void AddUserContractEntry(void* pUserContract) = 0;

}; // End IUserContractList

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Payment and Billing Management

/////////////////////////////////////////////////////////////////////////////
// IInvoiceLogList
//      Invoice log list interfaces exposed across a DLL boundry.
class IInvoiceLogList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetInvoiceLogList() const  = 0;
    virtual void AddInvoiceLogEntry(void* pInviceLog) = 0;

}; // End IInvoiceLogList

/////////////////////////////////////////////////////////////////////////////
// IInvoiceDetail
//      Invoice detail interfaces exposed across a DLL boundry.
class IInvoiceDetail : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetItemDescription() = 0;
    virtual void SetItemDescription(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetAmount() = 0;
    virtual void SetAmount(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetRemarks() = 0;
    virtual void SetRemarks(const std::string& szInStr) = 0;

}; // End IInvoiceDetail

/////////////////////////////////////////////////////////////////////////////
// IInvoiceDetailList
//      Invoice detail list interfaces exposed across a DLL boundry.
class IInvoiceDetailList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetInvoiceDetailList() const  = 0;
    virtual void AddInvoiceDetailEntry(void* pInvoiceDetail) = 0;

}; // End IInvoiceDetailList

/////////////////////////////////////////////////////////////////////////////
// IInvoiceLogEntry
//      Invoice log entry interfaces exposed across a DLL boundry.
class IInvoiceLogEntry : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual LONG64 GetInvoiceNumber() = 0;
    virtual void SetInvoiceNumber(const LONG64& nInLong64) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetInvoiceDate() = 0;
    virtual void SetInvoiceDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetInvoiceNote() = 0;
    virtual void SetInvoiceNote(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetTotalAmount() = 0;
    virtual void SetTotalAmount(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual bool GetIsPaid() = 0;
    virtual void SetIsPaid(const bool& nInBool) = 0;

    //-----------------------------------------------------------------
    // Accessors to IPaymentInfo* owned by this object
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetPaymentDate() = 0;
    virtual void SetPaymentDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetAmount() = 0;
    virtual void SetAmount(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual DIOMEDE::PaymentMethodsType GetPaymentMethodsType() = 0;
    virtual void SetPaymentMethodsType(const DIOMEDE::PaymentMethodsType& nInType) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetPaymentNote() = 0;
    virtual void SetPaymentNote(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual IInvoiceDetailList* GetInvoiceDetailList() = 0;

}; // End IInvoiceLogEntry

/////////////////////////////////////////////////////////////////////////////
// IPaymentInfo
//      Payment info interfaces exposed across a DLL boundry.
class IPaymentInfo : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetPaymentDate() = 0;
    virtual void SetPaymentDate(const time_t& nInDate) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual int GetAmount() = 0;
    virtual void SetAmount(const int& nInInt) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual DIOMEDE::PaymentMethodsType GetPaymentMethodsType() = 0;
    virtual void SetPaymentMethodsType(const DIOMEDE::PaymentMethodsType& nInType) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetPaymentNote() = 0;
    virtual void SetPaymentNote(const std::string& szInStr) = 0;

}; // End IPaymentInfo

/////////////////////////////////////////////////////////////////////////////
// IPaymentLogEntry
//      Payment log entry interfaces exposed across a DLL boundry.
class IPaymentLogEntry : public DiomedeDLLInterface
{
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual std::string GetCurrency() = 0;
    virtual void SetCurrency(const std::string& szInStr) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual double GetAmount() = 0;
    virtual void SetAmount(const double& nInDouble) = 0;

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual time_t GetCompletedDate() = 0;
    virtual void SetCompletedDate(const time_t& nInDate) = 0;

}; // End IPaymentLogEntry

/////////////////////////////////////////////////////////////////////////////
// IPaymentLogList
//      Payment log list interfaces exposed across a DLL boundry.
class IPaymentLogList : public DiomedeDLLInterface
{
public:
    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    virtual const std::vector<void*> GetPaymentLogList() const  = 0;
    virtual void AddPaymentLogEntry(void* pPaymentLog) = 0;

}; // End IPaymentLogList

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Service Classes

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// User Management

/////////////////////////////////////////////////////////////////////////////
// IUserManager
//      User authorization interfaces exposed across a DLL boundry.
class IUserManager : public DiomedeDLLInterface
{

public:
	//-----------------------------------------------------------------
	// Service results
	//-----------------------------------------------------------------
	virtual bool GetServiceError(std::string& szServiceError) = 0;

	//-----------------------------------------------------------------
	// Login/Logout commands
	//-----------------------------------------------------------------

	virtual int LoginUser(std::string szUsername, std::string szPassword,
	    std::string& szSessionToken) = 0;

	virtual int LogoutUser(std::string szSessionToken) = 0;

	//-----------------------------------------------------------------
    // User creation commands
	//-----------------------------------------------------------------
	virtual int CreateUser(ICreateUser& createUserInfo) = 0;
	virtual int DeleteUser(std::string szSessionToken) = 0;
	virtual int ChangePassword(std::string szSessionToken, std::string szOldPassword,
	                                                       std::string szNewPassword) = 0;
	virtual int ResetPassword(std::string szUsername)= 0;


	//-----------------------------------------------------------------
	// Manage user information
	//-----------------------------------------------------------------
	virtual int SetUserInfo(std::string szSessionToken, IUserInfo& userInfo) = 0;
	virtual int GetUserInfo(std::string szSessionToken, IUserInfo& userInfo) = 0;
	virtual int DeleteUserInfo(std::string szSessionToken,
	                           const DIOMEDE::UserInfoType& typeUserInfo) = 0;

	//-----------------------------------------------------------------
	// Manage emails
	//-----------------------------------------------------------------
    virtual int GetEmails(std::string szSessionToken, IEmailList& listEmails) = 0;
    virtual int AddEmail(std::string szSessionToken, std::string szNewEmail) = 0;
    virtual int DeleteEmail(std::string szSessionToken, std::string szOldEmail) = 0;
    virtual int SetPrimaryEmail(std::string szSessionToken, std::string szPrimaryEmail) = 0;

	//-----------------------------------------------------------------
    // Return the login log entries
	//-----------------------------------------------------------------
	virtual int SearchLoginLog(std::string szSessionToken, ISearchLoginLogFilter& searchFilter,
	                           ILoginLogList& listLoginLog) = 0;


}; // End IUserManager

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// File Management

/////////////////////////////////////////////////////////////////////////////
// IFileManager
//      File management interfaces exposed across a DLL boundry.
class IFileManager : public DiomedeDLLInterface
{
public:
	//-----------------------------------------------------------------
	// Service results
	//-----------------------------------------------------------------
	virtual bool GetServiceError(std::string& szServiceError) = 0;

	//-----------------------------------------------------------------
	// Upload a file command
	//-----------------------------------------------------------------
	virtual int CreateFile(std::string szSessionToken, IUpload& uploadInfo) = 0;
	virtual int UploadFile(std::string szSessionToken, IUpload& uploadInfo) = 0;

	//-----------------------------------------------------------------
	// Download a file command
	//-----------------------------------------------------------------
	virtual int DownloadFile(std::string szSessionToken, IDownload& downloadInfo) = 0;

	//-----------------------------------------------------------------
	// Display a file's contents command
	//-----------------------------------------------------------------
	virtual int DisplayFile(std::string szSessionToken, IDisplayFileContents& displayFileInfo) = 0;

	//-----------------------------------------------------------------
    // Get an upload token
	//-----------------------------------------------------------------
	virtual int GetUploadToken(std::string szSessionToken, IUploadToken& uploadTokenInfo) = 0;

	//-----------------------------------------------------------------
    // Get a download URL command
	//-----------------------------------------------------------------
	virtual int GetDownloadURL(std::string szSessionToken, IDownloadURL& downloadURLInfo) = 0;

	//-----------------------------------------------------------------
	// Search stored files command
	//-----------------------------------------------------------------
	virtual int SearchStoredFiles(std::string szSessionToken,
	                              ISearchFileFilter& searchFilter,
	                              IFilePropertiesList& listFileProperties) = 0;


	//-----------------------------------------------------------------
    // Return the search files totals
	//-----------------------------------------------------------------
	virtual int SearchFilesTotal(std::string szSessionToken,
	                             ISearchFileFilter& searchFilter,
	                             IFilesTotalLogEntry& fileTotalLogEntry) = 0;

	//-----------------------------------------------------------------
    // Return the search files totals log
	//-----------------------------------------------------------------
	virtual int SearchFilesTotalLog(std::string szSessionToken,
	                                ISearchLogFilter& searchFilter,
	                                IFilesTotalLogList& listFilesTotalLog) = 0;

	//-----------------------------------------------------------------
	// File operation commands
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
    // Rename the file
	//-----------------------------------------------------------------
	virtual int RenameFile(std::string szSessionToken, LONG64 l64FileID,
	                       std::string szNewFileName) = 0;

	//-----------------------------------------------------------------
    // Delete the file
	//-----------------------------------------------------------------
	virtual int DeleteFile(std::string szSessionToken, LONG64 l64FileID) = 0;

	//-----------------------------------------------------------------
    // Undelete the file
	//-----------------------------------------------------------------
	virtual int UndeleteFile(std::string szSessionToken, LONG64 l64FileID) = 0;

	//-----------------------------------------------------------------
    // Create a metadata
	//-----------------------------------------------------------------
	virtual int CreateMetaData(std::string szSessionToken, IMetaDataInfo& metaDataInfo) = 0;

	//-----------------------------------------------------------------
    // Create a metadata for a file
	//-----------------------------------------------------------------
	virtual int CreateFileMetaData(std::string szSessionToken, LONG64 l64FileID,
	                               IMetaDataInfo& metaDataInfo) = 0;

	//-----------------------------------------------------------------
    // Assign a metadata to a file
	//-----------------------------------------------------------------
	virtual int SetFileMetaData(std::string szSessionToken, LONG64 l64FileID, int nMetaDataID) = 0;

	//-----------------------------------------------------------------
    // Delete a file metadata
	//-----------------------------------------------------------------
	virtual int DeleteFileMetaData(std::string szSessionToken, LONG64 l64FileID, int nMetaDataID) = 0;

	//-----------------------------------------------------------------
    // Delete a metadata
	//-----------------------------------------------------------------
	virtual int DeleteMetaData(std::string szSessionToken, int nMetaDataID) = 0;

	//-----------------------------------------------------------------
    // Gets the metadata for a file
	//-----------------------------------------------------------------
	virtual int GetFileMetaData(std::string szSessionToken, LONG64 l64FileID,
	                            IMetaDataList& listMetaData) = 0;

	//-----------------------------------------------------------------
    // Gets the metadata using the search criteria
	//-----------------------------------------------------------------
	virtual int GetMetaData(std::string szSessionToken, IMetaDataInfo& metaDataInfo,
	                        IMetaDataList& listMetaData) = 0;

	//-----------------------------------------------------------------
    // Edit the metadata
	//-----------------------------------------------------------------
	virtual int EditMetaData(std::string szSessionToken, IMetaDataInfo& metaDataInfo) = 0;

	//-----------------------------------------------------------------
    // Replicate the file
	//-----------------------------------------------------------------
	virtual int ReplicateFile(std::string szSessionToken, IPhysicalFileInfo& physicalFileInfo) = 0;

	//-----------------------------------------------------------------
    // Unreplicate the file
	//-----------------------------------------------------------------
	virtual int UnReplicateFile(std::string szSessionToken, LONG64 l64FileID) = 0;

	//-----------------------------------------------------------------
    // Returns the available storage types.
	//-----------------------------------------------------------------
	virtual int GetStorageTypes(IStorageTypeList& listStorageType) = 0;

	//-----------------------------------------------------------------
    // Returns the phyiscal file information for a given file.
	//-----------------------------------------------------------------
	virtual int GetPhysicalFileInfo(std::string szSessionToken, LONG64 l64FileID,
	                                IPhysicalFileInfoList& listPhysicalFileInfo) = 0;

	//-----------------------------------------------------------------
    // Returns the physical file information for a list of logical
    // file IDs.
	//-----------------------------------------------------------------
	virtual int GetPhysicalFileInfo(std::string szSessionToken, std::vector<LONG64>& listFileIDs,
	                                ILogicalPhysicalFilesInfoList& listLogicalPhysicalFilesInfo) = 0;

	//-----------------------------------------------------------------
    // Returns the phyiscal file information for a given logical file
    // and physical file.
	//-----------------------------------------------------------------
	virtual int GetPhysicalFileInfo(std::string szSessionToken, LONG64 l64FileID,
	                                LONG64 l64PhysicalFileID,
	                                IPhysicalFileInfo& physicalFileInfo) = 0;

	//-----------------------------------------------------------------
    // Create a replication policy
	//-----------------------------------------------------------------
	virtual int CreateReplicationPolicy(std::string szSessionToken,
	                                    IReplicationPolicyInfo& replicationPolicyInfo) = 0;

	//-----------------------------------------------------------------
    // Returns a list of replication policies
	//-----------------------------------------------------------------
	virtual int GetReplicationPolicies(std::string szSessionToken,
	                                  IReplicationPolicyInfoList& listReplicationPolicies) = 0;

	//-----------------------------------------------------------------
    // Edit a replication policy
	//-----------------------------------------------------------------
	virtual int EditReplicationPolicy(std::string szSessionToken,
	                                  IReplicationPolicyInfo& replicationPolicyInfo) = 0;

	//-----------------------------------------------------------------
    // Delete a replication policy
	//-----------------------------------------------------------------
	virtual int DeleteReplicationPolicy(std::string szSessionToken, int nReplicationPolicyID) = 0;

	//-----------------------------------------------------------------
    // Assign a replication policy to a file
	//-----------------------------------------------------------------
	virtual int SetReplicationPolicy(std::string szSessionToken,
	                                 int nReplicationPolicyID, LONG64 l64FileID) = 0;

	//-----------------------------------------------------------------
    // Set a default replication policy
	//-----------------------------------------------------------------
	virtual int SetDefaultReplicationPolicy(std::string szSessionToken,
	                                        int nReplicationPolicyID) = 0;

	//-----------------------------------------------------------------
    // Return the default replication policy
	//-----------------------------------------------------------------
	virtual int GetDefaultReplicationPolicy(std::string szSessionToken,
	                                        IReplicationPolicyInfo& replicationPolicyInfo) = 0;


	//-----------------------------------------------------------------
    // Return the upload log entries
	//-----------------------------------------------------------------
	virtual int SearchUploadLog(std::string szSessionToken, ISearchUploadLogFilter& searchFilter,
	                           IUploadLogList& listUploadLog) = 0;

	//-----------------------------------------------------------------
    // Return the download log entries
	//-----------------------------------------------------------------
    virtual int SearchDownloadLog(std::string szSessionToken, ISearchDownloadLogFilter& searchFilter,
                                  IDownloadLogList& listDownloadLog) = 0;
}; // End IFileManager

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Product Management

/////////////////////////////////////////////////////////////////////////////
// IProductManager
//      Product management interfaces exposed across a DLL boundry.
class IProductManager : public DiomedeDLLInterface
{

public:
	//-----------------------------------------------------------------
	// Service results
	//-----------------------------------------------------------------
	virtual bool GetServiceError(std::string& szServiceError) = 0;

	//-----------------------------------------------------------------
    // Return the list of service products.
	//-----------------------------------------------------------------
    virtual int GetAllProducts(IProductList& listProducts) = 0;

	//-----------------------------------------------------------------
    // Purchase a product
	//-----------------------------------------------------------------
    virtual int PurchaseProduct(std::string szSessionToken, int nProductID) = 0;

	//-----------------------------------------------------------------
    // Return the list of purchased products.
	//-----------------------------------------------------------------
    virtual int GetMyProducts(std::string szSessionToken, IUserProductList& listUserProducts) = 0;

	//-----------------------------------------------------------------
    // Cancel a purchased products.
	//-----------------------------------------------------------------
    virtual int CancelProduct(std::string szSessionToken, int nProductID) = 0;

	//-----------------------------------------------------------------
    // Return the list of service contracts.
	//-----------------------------------------------------------------
    virtual int GetAllContracts(IContractList& listContracts) = 0;

	//-----------------------------------------------------------------
    // Purchase a contract
	//-----------------------------------------------------------------
    virtual int PurchaseContract(std::string szSessionToken, int nContractID,
                                 LONG64& l64UserContractID) = 0;

	//-----------------------------------------------------------------
    // Return the list of purchased contracts.
	//-----------------------------------------------------------------
    virtual int GetMyContracts(std::string szSessionToken,
                               IUserContractList& listUserContracts) = 0;

	//-----------------------------------------------------------------
    // Cancel a purchased contracts.
	//-----------------------------------------------------------------
    virtual int CancelContract(std::string szSessionToken, LONG64 l64ContractID) = 0;

}; // End IProductManager

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Purchasing and Billing Management

/////////////////////////////////////////////////////////////////////////////
// IPurchasingManager
//      Purchasing and billing management interfaces exposed across a DLL boundry.
class IPurchasingManager : public DiomedeDLLInterface
{

public:
	//-----------------------------------------------------------------
	// Service results
	//-----------------------------------------------------------------
	virtual bool GetServiceError(std::string& szServiceError) = 0;

	//-----------------------------------------------------------------
    // Search the user's payment log.
	//-----------------------------------------------------------------
	virtual int SearchPayments(std::string szSessionToken, time_t tStartDate, time_t tEndDate,
	                           IPaymentLogList& listPaymentLogEntries) = 0;

	//-----------------------------------------------------------------
    // Search the user's invoice log.
	//-----------------------------------------------------------------
	virtual int SearchInvoices(std::string szSessionToken, ISearchLogFilter& searchFilter,
	                           IInvoiceLogList& listInvoiceLogEntries) = 0;

}; // End IPurchasingManagers

/*********************************************************************
 * 
 *  file:  ResumeInfoData.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Contains the classes for resuming tasks such as upload
 *          and download.
 * 
 *********************************************************************/

//! \ingroup resume_info
//! @{

#ifndef __REUSME_INFO_DATA_H__
#define __REUSME_INFO_DATA_H__

#include "StringUtil.h"

#include <string>
#ifdef WIN32
#include <io.h>
#endif

#include "boost/date_time/posix_time/posix_time.hpp"
#include <iostream>

//---------------------------------------------------------------------
//! ResumeInfo data types
//---------------------------------------------------------------------
namespace ResumeInfoTypes {
    typedef enum { resumeTypeUndefined  = 0x0001,        //! Type undefined
                   resumeUploads        = 0x0002,        //! Resume upload task
                   resumeDownloads      = 0x0004,        //! Resume download task
                   resumeDone           = 0x0008         //! Resume is complete - cleanup candidate
                 } ResumeInfoType;
}

namespace ResumeInfoIntervalTypes {
    typedef enum { resumeIntervalUndefined = 0,
                   resumeInterval1,                     //! Defaults to 5 seconds.
                   resumeInterval2,                     //! Defaults to 30 seconds.
                   resumeInterval3,                     //! Defaults to 5 minutes.
                   resumeInterval4,                     //! Defaults to 10 seconds.
                   resumeInterval5,                     //! Defaults to 1 hour.
                   resumeIntervalDone
                 } ResumeInfoIntervalType;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

using namespace StringUtil;
using namespace ResumeInfoTypes;
using namespace ResumeInfoIntervalTypes;

using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace std;

const std::string DATUM_SEP     = _T("|");
const std::string VERSION_ID	= _T("2");
const std::string RESUME_PAD    = _T("*");

///////////////////////////////////////////////////////////////////////
// ResumeInfoData
class ResumeInfoData
{
protected:
    ResumeInfoType              m_nResumeInfoType;              //! Type for serialization
    std::string                 m_szFilePath;                   //! File complete path

    ResumeInfoIntervalType      m_nIntervalType;                //! Current sequence type
                                                                //! e.g. 5 sec, 30 sec.
    time_t                      m_tmFirstStart;                 //! Time of first attempt
    time_t                      m_tmLastStart;                  //! Time of last attempt

    int                         m_nResumeFileIndex;             //! The index from the resume
                                                                //! index file constaining the
                                                                //! size and position of this data
                                                                //! in the data file.


public:
	virtual bool Deserialize( std::vector<std::string>listResumeData, unsigned int nStartIndex )
	    { return false; }

    ResumeInfoData() : m_nResumeInfoType(resumeTypeUndefined),
                       m_szFilePath(_T("")),
                       m_nIntervalType(resumeIntervalUndefined),
                       m_tmFirstStart(0),
                       m_tmLastStart(0), m_nResumeFileIndex(-1) {};

    ResumeInfoData(ResumeInfoType resumeInfoType) : m_nResumeInfoType(resumeInfoType),
                                                    m_szFilePath(_T("")),
                                                    m_nIntervalType(resumeIntervalUndefined),
                                                    m_tmFirstStart(0),
                                                    m_tmLastStart(0), m_nResumeFileIndex(-1) {};

    virtual ~ResumeInfoData() {};

	virtual void operator =( const ResumeInfoData &srcResumeInfo );
	virtual bool operator ==( const ResumeInfoData& srcResumeInfo );

    virtual std::string Serialize(bool bEncrypt, unsigned long i1, unsigned long i2) { return _T("");}
    virtual bool Deserialize(const std::string szSerializedData, bool bEncrypted = false,
                             unsigned long i1 = 0, unsigned long i2 = 0) { return false; }

public:
    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    ResumeInfoType GetResumeInfoType() { return m_nResumeInfoType; }
    void SetResumeInfoType(const ResumeInfoType& nInType) {
        m_nResumeInfoType = nInType;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    std::string GetFilePath() { return m_szFilePath; }
    void SetFilePath(const std::string& szInStr) {
        m_szFilePath = szInStr;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    ResumeInfoIntervalType GetResumeIntervalType() { return m_nIntervalType; }
    void SetResumeIntervalType(const ResumeInfoIntervalType& nInType) {
        m_nIntervalType = nInType;
    }

    //-----------------------------------------------------------------
    // Reset the interval - any successful upload will reset this
    // value back to the start.
    //-----------------------------------------------------------------
    void ResetNextResumeIntervalType() {
        m_nIntervalType = resumeIntervalUndefined;
    }

    //-----------------------------------------------------------------
    // Increment the interval to the next time span.
    //-----------------------------------------------------------------
    ResumeInfoIntervalType GetNextResumeIntervalType() {
        switch (m_nIntervalType) {
            case resumeIntervalUndefined:
                m_nIntervalType = resumeInterval1;
                break;
            case resumeInterval1:
                m_nIntervalType = resumeInterval2;
                break;
            case resumeInterval2:
                m_nIntervalType = resumeInterval3;
                break;
            case resumeInterval3:
                m_nIntervalType = resumeInterval4;
                break;
            case resumeInterval4:
                m_nIntervalType = resumeInterval5;
                break;
            case resumeInterval5:
                m_nIntervalType = resumeIntervalDone;
                break;
            case resumeIntervalDone:
                m_nIntervalType = resumeIntervalUndefined;
                break;
            default:
                break;
        }

        return m_nIntervalType;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    time_t GetFirstStart() { return m_tmFirstStart; };
    void SetFirstStart(const time_t& nInTime) {
        m_tmFirstStart = nInTime;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    time_t GetLastStart() { return m_tmLastStart; };
    void SetLastStart(const time_t& nInTime) {
        m_tmLastStart = nInTime;
    }

    //-----------------------------------------------------------------
    // Note: the index is NOT stored in the data file, but is set
    // to allow access to the correct index when working with a set
    // of resume data objects.
    //-----------------------------------------------------------------
    int GetResumeIndex() { return m_nResumeFileIndex; };
    void SetResumeIndex(const int& nInInt) {
        m_nResumeFileIndex = nInInt;
    }

}; // End ResumeInfoData

///////////////////////////////////////////////////////////////////////
//! \class ResumeUploadInfoData
//! \brief Handles the resume data associated with uploading files.
//!
//! Resume Upload Data:
//! -# Diomede file ID
//! -# file path
//! -# file size (at the start of upload)
//! -# last modified date
//! -# number of bytesupload thus far
//! -# time of first attempt (to retain order)
//! -# time of last attempt
//! -# time of next attempt
class ResumeUploadInfoData : public ResumeInfoData
{
private:
    LONG64                      m_l64FileID;                    //! File ID returned from first attempt.
    LONG64                      m_l64FileSize;                  //! File size
    bool                        m_bAddMetaData;                 //! Add metadata for the file.
    bool                        m_bCreateMD5Digest;             //! Create the MD5 hash as part of the
                                                                //! upload.
    time_t                      m_tmLastModifiedTime;           //! File last modified timestamp

    LONG64                      m_l64BytesRead;                 //! Bytes uploaded thus far

protected:
	virtual bool Deserialize( std::vector<std::string>listResumeData, unsigned int nStartIndex );

public:
    ResumeUploadInfoData(void);
    ResumeUploadInfoData(const std::string szSerializedData);

    virtual ~ResumeUploadInfoData() {};
	virtual void operator =( const ResumeUploadInfoData &srcResumeInfo );
	virtual bool operator ==( const ResumeUploadInfoData& srcResumeInfo );

    //! Serialization of resume upload data
    virtual std::string Serialize(bool bEncrypt, unsigned long i1, unsigned long i2);
    virtual bool Deserialize(const std::string szSerializedData, bool bEncrypted = false,
                             unsigned long i1 = 0, unsigned long i2 = 0);

    //! Has the file changed since the last time?
    bool HasResumeFileChanged();

    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    LONG64 GetFileID() { return m_l64FileID; };
    void SetFileID(const LONG64& nInLong64) {
        m_l64FileID = nInLong64;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    LONG64 GetFileSize() { return m_l64FileSize; };
    void SetFileSize(const LONG64& nInLong64) {
        m_l64FileSize = nInLong64;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    bool GetAddMetaData() { return m_bAddMetaData; }
    void SetAddMetaData(const bool& nInBool) {
        m_bAddMetaData = nInBool;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    bool GetCreateMD5Hash() { return m_bCreateMD5Digest; }
    void SetCreateMD5Hash(const bool& nInBool) {
        m_bCreateMD5Digest = nInBool;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    time_t GetLastModified() { return m_tmLastModifiedTime; };
    void SetLastModified(const time_t& nInDate) {
        m_tmLastModifiedTime = nInDate;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    LONG64 GetBytesRead() { return m_l64BytesRead; };
    void SetBytesRead(const LONG64& nInLong64) {
        m_l64BytesRead = nInLong64;
    }


}; // End ResumeUploadInfoData

///////////////////////////////////////////////////////////////////////
//! \class ResumeDownloadInfoData
//! \brief Handles the resume data associated with downloading files.
//!
//! Resume Download Data:
//! -# Diomede file ID
//! -# file path
//! -# number of bytes downloaded thus far
//! -# time of first attempt (to retain order)
//! -# time of last attempt
//! -# time of next attempt
class ResumeDownloadInfoData : public ResumeInfoData
{
private:
    LONG64                      m_l64FileID;                    //! File ID for the downloaded file.
    LONG64                      m_l64BytesReceived;             //! Bytes downloaded thus far

protected:
	virtual bool Deserialize( std::vector<std::string>listResumeData, unsigned int nStartIndex );

public:
    ResumeDownloadInfoData(const std::string szSerializedData);
    ResumeDownloadInfoData(void) :  ResumeInfoData(resumeDownloads),
                                    m_l64FileID(0),
                                    m_l64BytesReceived(0) {};

    virtual ~ResumeDownloadInfoData() {};
	virtual void operator =( const ResumeDownloadInfoData &srcResumeInfo );
	virtual bool operator == (const ResumeDownloadInfoData& srcResumeInfo );

    //! Serialization of resume upload data
    virtual std::string Serialize(bool bEncrypt, unsigned long i1, unsigned long i2);
    virtual bool Deserialize(const std::string szSerializedData, bool bEncrypted = false,
                             unsigned long i1 = 0, unsigned long i2 = 0);

    //-----------------------------------------------------------------
    // Getters and setters
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    LONG64 GetFileID() { return m_l64FileID; };
    void SetFileID(const LONG64& nInLong64) {
        m_l64FileID = nInLong64;
    }

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------
    LONG64 GetBytesReceived() { return m_l64BytesReceived; };
    void SetBytesReceived(const LONG64& nInLong64) {
        m_l64BytesReceived = nInLong64;
    }

}; // End ResumeUploadInfoData

/** @} */

#endif // __REUSME_INFO_DATA_H__

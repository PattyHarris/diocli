/*********************************************************************
 * 
 *  file:  ResumeManager.cpp
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Singleton class that takes care of reading and writing
 *          data associated with resuming selected user actions
 *          (e.g. such as upload).
 * 
 *********************************************************************/

//! \ingroup consolecontrol
//! @{

#include "stdafx.h"
#include "ResumeManager.h"
#include "ResumeNamedMutex.h"

#include "../Include/types.h"
#include "../Util/Util.h"

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <errno.h>

#include "../Util/ClientLog.h"
#include "../Util/ClientLogUtils.h"
#include "../Util/UserProfileData.h"
#include "../Util/ProfileManager.h"
#include "../Util/StringUtil.h"

#include "../Include/ErrorCodes/UIErrors.h"
#include "../Include/ErrorCodes/UtilErrors.h"

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/interprocess/detail/config_begin.hpp"
#include "boost/interprocess/sync/scoped_lock.hpp"
#include "boost/interprocess/sync/lock_options.hpp"

#if defined(linux)
#include <unistd.h>
#endif

#if defined(linux) || defined( __APPLE__)
#include <stdio.h>
#include <stdlib.h>
#endif

using namespace StringUtil;
using namespace DiomedeResumeErrorCodes;
using namespace boost::algorithm;
using namespace boost::posix_time;

// Used to place interprocess mutexes around writing
// the files - added to prevent errors that can occur
// when multiple instances of DioCLI are running.
// Global read/write named_mutex constants
const std::string g_szUploadWriteMutex = _T("ResumeUploadInfoWriteMutex");
const std::string g_szUploadReadMutex = _T("ResumeUploadInfoReadMutex");

//---------------------------------------------------------------------
//! RESUME_FILE_VER
//!
//! A version number for the resume.dcf file.  Update this version each
//! time the file format changes.  The parser should maintain its ability
//! to parse old file formats, but the file should always be written
//! in the latest format.
//---------------------------------------------------------------------
#define RESUME_FILE_VER 2

#ifdef _DEBUG
	#define USE_RESUME_ENCRYPTION false
#else
	// Make sure we ALWAYS use encryption in release mode
	#define USE_RESUME_ENCRYPTION false
#endif

#define ADD_NEWLINE_TO_FILE			true
#define CONSUME_NEWLINE_FROM_FILE	true

//! Resume Manager Singleton
ResumeManager* ResumeManager::m_pResumeMgr = NULL;

///////////////////////////////////////////////////////////////////////
// ResumeManager Constructor
ResumeManager::ResumeManager() : m_pResumeFile(0), m_pResumeIndexFile(0), m_szResumeFileName(_T("")),
                                 m_pMutexUtil(NULL), m_nLocks(0),
                                 m_bIsFirstRun(true), m_bIsFileValid(false),
                                 m_dwEncryptLow(0), m_dwEncryptHigh(0),
                                 m_nFileVersion(0), m_tResumeTime(0),
                                 m_szBuffer(NULL), m_szAppDirectory(_T(""))
{
} // End Constructor

///////////////////////////////////////////////////////////////////////
// ResumeManager Destructor
ResumeManager::~ResumeManager()
{
    if (m_szBuffer != NULL) {
        delete [] m_szBuffer;
        m_szBuffer = NULL;
    }

    CloseOpenFiles();

    if (m_pMutexUtil != NULL) {
        delete m_pMutexUtil;
        m_pMutexUtil = NULL;
    }
    m_nLocks = 0;

} // End Destructor

///////////////////////////////////////////////////////////////////////
ResumeManager* ResumeManager::Instance()
{
	if (m_pResumeMgr == NULL) {
	    m_pResumeMgr = new ResumeManager;
	}

	return m_pResumeMgr;

} // End Instance

///////////////////////////////////////////////////////////////////////
//! ResumeManager Shutdown
void ResumeManager::Shutdown()
{
	if ( this == m_pResumeMgr )
		m_pResumeMgr = NULL;

	delete this;

} // End Destructor

///////////////////////////////////////////////////////////////////////
// Private Methods

///////////////////////////////////////////////////////////////////////
//! \brief Sets up the named mutex and lock to lock access to the
//!        info data and index files.
//! \param szLoggingErrorMsg: optional string to use for logging.
//! \return 0 if successful, error otherwise.
//!
int ResumeManager::LockResources(std::string szLoggingErrorMsg /*T("")*/)
{
    // Resources already locked.
    if (m_pMutexUtil != NULL)
    {
        m_nLocks ++;
        return 0;
    }

    m_pMutexUtil = new ResumeNamedMutexUtil(OPEN_OR_CREATE, g_szUploadWriteMutex);

    if (m_pMutexUtil->IsValid() == false) {
        if (szLoggingErrorMsg.length() > 0) {
            ClientLog(UI_COMP, LOG_ERROR, false, _T("%s: %s"),
                szLoggingErrorMsg.c_str(), m_pMutexUtil->GetLastError().c_str() );
        }
        else {
            ClientLog(UI_COMP, LOG_ERROR, false, _T("Resume mutex lock error: %s"),
                m_pMutexUtil->GetLastError().c_str() );
        }

        delete m_pMutexUtil;
        m_pMutexUtil = NULL;
        return RESUME_FILE_LOCKING_ERROR;
    }

    m_nLocks ++;
    return 0;

} // End LockResources

///////////////////////////////////////////////////////////////////////
//! \brief Releases the locked access to the info data and index files.
//!
//! \return 0 if successful, error otherwise.
//!
int ResumeManager::UnlockResources()
{
    if (m_nLocks > 0) {
        m_nLocks --;
    }

    if (m_nLocks == 0) {
        assert(m_pMutexUtil != NULL);
        if (m_pMutexUtil != NULL) {
            delete m_pMutexUtil;
            m_pMutexUtil = NULL;
        }
    }

    return 0;

} // End UnlockResources

///////////////////////////////////////////////////////////////////////
//! \brief Helper function for Load
//!
//! \param szFilePath: file to be deleted
//! \return 0 if successful, error code otherwise
//!
int ResumeManager::FileDelete( const std::string& szFilePath )
{
    LockResources(_T("Resume file delete error"));

    ResetErrorCodes();
    int nReturn = 0;

    #ifdef WIN32
        nReturn = remove( szFilePath.c_str());
        if (nReturn == -1) {
            m_nLastError = errno;
            UnlockResources();
            return RESUME_SYSTEM_IO_ERROR;
        }
    #else
        nReturn = unlink( szFilePath.c_str());
        if (nReturn == -1) {
            m_nLastError = errno;
            UnlockResources();
            return RESUME_SYSTEM_IO_ERROR;
        }
    #endif

    UnlockResources();
	return nReturn;

} // End FileDelete

/////////////////////////////////////////////////////////////////////////////
void ResumeManager::GetEpochSeconds(time_t& epochSeconds)
{
    boost::posix_time::ptime currentTime = boost::posix_time::microsec_clock::local_time();

    std::tm tmRef = boost::posix_time::to_tm(currentTime);
    epochSeconds = mktime(&tmRef);

} // End GetEpochSeconds

/////////////////////////////////////////////////////////////////////////////
bool ResumeManager::CreateBuffer(int nBufferSize)
{
    if (m_szBuffer != NULL) {
		delete [] m_szBuffer;
		m_szBuffer = NULL;
    }

    m_szBuffer = new char[nBufferSize];
    return true;

} // End CreateBuffer

/////////////////////////////////////////////////////////////////////////////
//! Clear the read buffer
//!
void ResumeManager::ClearBuffer()
{
    if (m_szBuffer != NULL) {
        memset(m_szBuffer, 0, sizeof(m_szBuffer) * sizeof(char));
    }

} // End ClearBuffer

/////////////////////////////////////////////////////////////////////////////
//! \brief Sets up the resume info data for the first time.
//!
void ResumeManager::SetupForNewResumeFile()
{
	ClientLog(UI_COMP, LOG_STATUS, false, _T("Resume info file does not exist - must be first run."));
	m_bIsFirstRun = true;

} // End SetupForNewResumeFile

/////////////////////////////////////////////////////////////////////////////
//! \brief Reads in the file version and encryption fields.
//! \param szDataFileName: Resume index filename
//! \param bIsFirstRun: indicates the file(s) are empty
//! \return 0 if successful, error code otherise.
int ResumeManager::ReadFileHeaderData(std::string szDataFileName, bool& bIsFirstRun)
{
    LockResources(_T("Resume read file header error"));

    m_bIsFirstRun = true;
    bIsFirstRun = true;

    std::string szOpenFlags = _T("wt+");

    if (Util::DoesFileExist(szDataFileName) == true) {
        szOpenFlags = _T("rt+");
        m_bIsFirstRun = bIsFirstRun = false;
    }

	#ifdef WIN32
        m_pResumeFile = _fsopen( szDataFileName.c_str(), szOpenFlags.c_str(), SH_DENYNO);
    #else
    	m_pResumeFile = _tfopen( szDataFileName.c_str(), szOpenFlags.c_str());
    #endif

	if (m_pResumeFile == NULL) {
        UnlockResources();
	    return RESUME_OPEN_FILE_ERROR;
	}

	//--------------------------------------------------------------------
    // File versioning.  This is not the application version, but rather
    // a version stamp for the resume .dat/.idx files.  When the format changes,
    // we'll be able to parse old versions.  If no version number is found,
    // we'll assume we're dealing with a version 0 file.  If the file is in
    // fact corrupt, our parsing will break later and we'll deal with it then.
	//--------------------------------------------------------------------

    // Move to the end of the file - for some reason, this check needs to
    // happen right before the file is read.
    /**/
    int nResult = fseek(m_pResumeFile, 0, SEEK_END);
    if (nResult) {
        m_nLastError = errno;
	    UnlockResources();
        return RESUME_SYSTEM_IO_ERROR;
    }

    long lSize = ftell(m_pResumeFile);
    if (lSize < 0) {
        UnlockResources();
        return RESUME_OPEN_FILE_ERROR;
    }
    else if (lSize == 0) {
        m_bIsFirstRun = bIsFirstRun = true;
    }

	// If this is a first run, we're done...
	if (m_bIsFirstRun == true) {
        UnlockResources();
	    return 0;
	}
	/**/

    rewind(m_pResumeFile);

    int nFileVersion = 0;
    int nNumBytesRead = _ftscanf(m_pResumeFile, _T("Ver: %d\n"), &nFileVersion);

    //! Verify the file version - this will allow us to have different versions of the
    //! file at a later time.
    if ( nFileVersion < RESUME_FILE_VER ) {
	    ClientLog(UI_COMP, LOG_ERROR, false,
			    _T("Resume header: This version of the resume data file is no longer supported. It is version %d."),
			    nFileVersion);
	    UnlockResources();
	    return RESUME_INVALID_CONFIG_FORMAT;
    }

    unsigned long dwEncryptLow = 0;
    unsigned long dwEncryptHigh = 0;

    if ((nNumBytesRead > 0) && (nNumBytesRead != EOF)) {

        // Version number found. Next get the date/time stamp used in keying the encryption.
        nNumBytesRead = _ftscanf(m_pResumeFile, _T("%lu %lu\n\n"), &dwEncryptLow, &dwEncryptHigh);

        if ((nNumBytesRead <= 0) || (nNumBytesRead == EOF)) {
            // Error: could not read in the date/time stamp
    	    UnlockResources();
            return RESUME_INVALID_CONFIG_FORMAT;
        }
    }

    m_dwEncryptLow = dwEncryptLow;
    m_dwEncryptHigh = dwEncryptHigh;
    m_nFileVersion = nFileVersion;

    UnlockResources();
    return 0;

} // End ReadFileHeaderData

///////////////////////////////////////////////////////////////////////
//! \brief Open the index file, reading the contents into memory
//! \param szResumeFileName: Resume index filename
//! \return 0 if successful, error otherwise
//!
int ResumeManager::ReadResumeIndex( std::string szResumeFileName )
{
    LockResources(_T("Resume read index error"));
    ResetErrorCodes();

    // If the file opened is a different file, close all the open files..
    if ( ( m_szResumeFileName.length() > 0) &&
         ( m_szResumeFileName.find(szResumeFileName) == (size_t)-1 ) ) {
        CloseOpenFiles(szResumeFileName);
    }

    std::string szFullFilePath = ResumeManager::GetAppDataDir();
    m_szResumeFileName = szFullFilePath + szResumeFileName;

    std::string szIndexFileName = m_szResumeFileName + _T(".idx");

    bool bFileExists = false;
    std::string szOpenFlags = _T("wt+");

    if (Util::DoesFileExist(szIndexFileName) == true) {
        szOpenFlags = _T("rt+");
        bFileExists = true;
    }

    // Clear out any existing data.
    m_listResumeIndexInfo.clear();

    m_pResumeIndexFile = _tfopen( szIndexFileName.c_str(), szOpenFlags.c_str());
    if (m_pResumeIndexFile == NULL) {
	    UnlockResources();
        return RESUME_OPEN_FILE_ERROR;
    }

    // Nothing to do since the file didn't exist prior to this call..
    if (bFileExists == false) {
        fflush(m_pResumeIndexFile);
        fclose(m_pResumeIndexFile);
        m_pResumeIndexFile = 0;
	    UnlockResources();
        return 0;
    }

    // Move to the end of the file.
    int nResult = fseek(m_pResumeIndexFile, 0, SEEK_END);
    if (nResult) {
        m_nLastError = errno;
	    UnlockResources();
        return RESUME_SYSTEM_IO_ERROR;
    }

    long lSize = ftell(m_pResumeIndexFile);
    if (lSize == 0) {
	    UnlockResources();
        return RESUME_NO_MORE_DATA;
    }

    // First line should be the version
    int nFileVersion = 0;

    rewind(m_pResumeIndexFile);
    int nNumBytesRead = _ftscanf(m_pResumeIndexFile, _T("Ver: %d\n"), &nFileVersion);

    //! Verify the file version - this will allow us to have different versions of the
    //! file at a later time.
    if ( nFileVersion < RESUME_FILE_VER ) {
	    ClientLog(UI_COMP, LOG_ERROR, false,
			    _T("Resume index: This version of the resume data file is no longer supported. It is version %d."),
			    nFileVersion);
	    UnlockResources();
	    return RESUME_INVALID_CONFIG_FORMAT;
    }

    lSize = ftell(m_pResumeIndexFile);
    if (lSize == 0) {
	    UnlockResources();
        return RESUME_NO_MORE_DATA;
    }

    // Check for records....
    UINT nReadResult = 0;
    ResumeIndexStruct resumeIndexInfo;

    // Begin reading after the version number.
    // fseek(m_pResumeIndexFile, 0, SEEK_SET);

    while (nReadResult == 0 ) {
        memset(&resumeIndexInfo, 0, sizeof(resumeIndexInfo));

        #if 1
            nNumBytesRead = _ftscanf(m_pResumeIndexFile, _T("%d,%d\n"),
                                     &resumeIndexInfo.nPosition, &resumeIndexInfo.nSize);
        #else
            // For reading out the structure using a binary file...
            nNumBytesRead = static_cast<unsigned int>(fread( (char *)&resumeIndexInfo, 1,
                                                             static_cast<size_t>(sizeof(resumeIndexInfo)),
                                                             m_pResumeIndexFile ));
        #endif

        if ( (nNumBytesRead == 0) || (nNumBytesRead == EOF) )  {
            nReadResult = RESUME_NO_MORE_DATA;
        }
        else {
            m_listResumeIndexInfo.push_back(resumeIndexInfo);
            ClientLog(UI_COMP, LOG_STATUS, false, _T("ReadResumeIndex: position %d, size %d"),
                resumeIndexInfo.nPosition, resumeIndexInfo.nSize);
        }
    }

    fflush(m_pResumeIndexFile);
    fclose(m_pResumeIndexFile);
    m_pResumeIndexFile = 0;

    UnlockResources();
    return 0;

} // End ReadResumeIndex

///////////////////////////////////////////////////////////////////////
//! \brief Writes out the index file from the data in memory.
//! \param szResumeFileName: Resume index filename
//! \return 0 if successful, error otherwise
//!
int ResumeManager::WriteResumeIndex(std::string szResumeFileName)
{
    LockResources(_T("Resume write resume index error"));

    // Make sure the filename is valid
    if (szResumeFileName.length() == 0) {
        ClientLog(UI_COMP, LOG_ERROR, false, _T("WriteResumeIndex: resume file name is empty."));
	    UnlockResources();
        return RESUME_OPEN_FILE_ERROR;
    }

    ResetErrorCodes();


    // Close the resume index file - if this happens, we may need to consider
    // a list of opened index files.
    if (m_pResumeIndexFile != NULL) {
        fflush(m_pResumeIndexFile);
        fclose(m_pResumeIndexFile);
        m_pResumeIndexFile = 0;
    }

    // If the file opened is a different file, close all the open files..
    if ( ( m_szResumeFileName.length() > 0) &&
         ( m_szResumeFileName.find(szResumeFileName) == (size_t)-1 ) ) {
        CloseOpenFiles(szResumeFileName);
    }


    std::string szFullFilePath = ResumeManager::GetAppDataDir();
    m_szResumeFileName = szFullFilePath + szResumeFileName;

    std::string szIndexFileName = m_szResumeFileName + _T(".idx");

    std::string szOpenFlags = _T("wt+");
    m_pResumeIndexFile = _tfopen( szIndexFileName.c_str(), szOpenFlags.c_str());

    if (m_pResumeIndexFile == NULL) {
	    UnlockResources();
        return RESUME_OPEN_FILE_ERROR;
    }

    // Write some version info to help with future parsing of this file.
    fprintf(m_pResumeIndexFile, _T("Ver: %d\n"), RESUME_FILE_VER);

    int nIndex = 0;
    int nSize = 0;

    int nResult = 0;
    ResumeIndexStruct resumeIndexInfo;

    for (nIndex = 0; nIndex < (int)m_listResumeIndexInfo.size(); nIndex++) {
	    resumeIndexInfo = m_listResumeIndexInfo[nIndex];

        #if 1
            fprintf(m_pResumeIndexFile, _T("%d,%d\n"), resumeIndexInfo.nPosition, resumeIndexInfo.nSize);
            ClientLog(UI_COMP, LOG_STATUS, false, _T("WriteResumeIndex: position %d, size %d"),
                resumeIndexInfo.nPosition, resumeIndexInfo.nSize);
        #else
            // For writing to a binary file - we may want to go this route later.
            nSize = fwrite((char *)&resumeIndexInfo, sizeof(char), sizeof(resumeIndexInfo),
                            m_pResumeIndexFile);
            if (nSize != sizeof(resumeIndexInfo)) {
                m_nLastError = errno;
        	    UnlockResources();
                return RESUME_SYSTEM_IO_ERROR;
                break;
            }
        #endif
    }

    fflush(m_pResumeIndexFile);
    fclose(m_pResumeIndexFile);
    m_pResumeIndexFile = 0;

    UnlockResources();
    return 0;

} // End WriteResumeIndex

///////////////////////////////////////////////////////////////////////
//! \brief Helper to clear data associated with the last resume data handling.
//!        This must be called between resume tasks, such as between
//!        uploads and/or downloads.
//! \return 0 if successful, error otherwise
//!
int ResumeManager::ClearResumeMgrData()
{
    ClearBuffer();
    m_szResumeFileName = _T("");

    return 0;

} // End ClearResumeMgrData

///////////////////////////////////////////////////////////////////////
//! \brief Close all open resume data and index files.
//! \param szResumeFileName: resume file name preface
//! \return 0 if successful, error otherwise
//!
int ResumeManager::CloseOpenFiles(std::string szResumeFileName)
{
    LockResources(_T("Resume close open files error"));

	if (m_pResumeFile) {
	    fflush(m_pResumeFile);
	    fclose(m_pResumeFile);
	    m_pResumeFile = 0;
	}

    if (szResumeFileName.length() == 0) {
        // Close the resume index file if it's open.
	    if (m_pResumeIndexFile != NULL) {
    	    fflush(m_pResumeIndexFile);
            fclose(m_pResumeIndexFile);
            m_pResumeIndexFile = 0;
        }

    	m_listResumeIndexInfo.clear();
	    UnlockResources();
        return 0;
    }

    int nResult = 0;

    // Else, write out the current index data
    nResult = WriteResumeIndex(szResumeFileName);
    UnlockResources();
	return nResult;

} // End CloseOpenFiles

///////////////////////////////////////////////////////////////////////
//! \brief Opens the resume data file.  The associdated index file
//!        is read into memory.
//! \param szResumeFileName: Resume index filename
//! \param bIsFirstRun: indicates the file(s) are empty
//! \return 0 if successful, error otherwise
//!
int ResumeManager::OpenResumeData( std::string szResumeFileName, bool& bIsFirstRun )
{
    LockResources(_T("Resume open resume data error"));

    ClearResumeMgrData();

    std::string szFullFilePath = ResumeManager::GetAppDataDir();
    m_szResumeFileName = szFullFilePath + szResumeFileName;

    std::string szDataFileName = m_szResumeFileName + _T(".dat");
    std::string szIndexFileName = m_szResumeFileName + _T(".idx");

    if (m_pResumeFile != NULL) {
        // Close the file - if this happens, we may need to consider
        // a list of opened files.
        fflush(m_pResumeFile);
        fclose(m_pResumeFile);
        m_pResumeFile = 0;
    }

    // Figure out the resume type to handle invalid file format.
	ResumeInfoType clearResumeInfoType = resumeTypeUndefined;
	if (szResumeFileName == RESUME_UPLOAD_FILENAME) {
	    clearResumeInfoType = ResumeInfoType(resumeTypeUndefined | resumeUploads);
	}
	else {
	    clearResumeInfoType = ResumeInfoType(resumeTypeUndefined | resumeDownloads);
	}

    // Read the version and encryption information.
    int nResult = ReadFileHeaderData(szDataFileName, bIsFirstRun);

	// If this is a first run, we're done...
	if (bIsFirstRun == true) {
        UnlockResources();
	    return 0;
	}

    if (nResult != 0) {
        if (nResult == RESUME_INVALID_CONFIG_FORMAT) {
            // If we get an invalid config format, just delete the files...
            nResult = CloseOpenFiles(szResumeFileName);
            if (nResult == 0) {
                nResult = FileDelete(szDataFileName);
                if (nResult == 0) {
                    nResult = FileDelete(szIndexFileName);
                }
            }
        }
        UnlockResources();
        return nResult;
    }

    // Read in the index data
    nResult = ReadResumeIndex( szResumeFileName );
    if (nResult == RESUME_INVALID_CONFIG_FORMAT) {
        // If we get an invalid config format, just delete the files...
        nResult = CloseOpenFiles(szResumeFileName);
        if (nResult == 0) {
            nResult = FileDelete(szDataFileName);
            if (nResult == 0) {
                nResult = FileDelete(szIndexFileName);
            }
        }
    }

    UnlockResources();
	return nResult;

} // End OpenResumeData

///////////////////////////////////////////////////////////////////////
//! \brief Clear the contents of the resume files.
//! \param resumeInfoType: Resume type - undefined (all), completed (done),
//!                        incomplete (resume upload)
//! \return 0 if successful, error otherwise
//!
int ResumeManager::ClearResumeFiles(const ResumeInfoType resumeInfoType)
{
    LockResources(_T("Resume clear resume files error"));

    std::string szResumeFileName = _T("");
    bool bSuccess = true;
	ResumeInfoType clearResumeInfoType = resumeTypeUndefined;

    ResetErrorCodes();

    if (resumeInfoType & resumeUploads) {
        szResumeFileName = RESUME_UPLOAD_FILENAME;
        clearResumeInfoType = resumeUploads;
        m_listResumeUploadInfo.clear();

    }
    else if (resumeInfoType & resumeDownloads) {
        szResumeFileName = RESUME_DOWNLOAD_FILENAME;
        clearResumeInfoType = resumeDownloads;
        m_listResumeDownloadInfo.clear();
    }
    else {
        bSuccess = false;
    }

    if (bSuccess == false) {
        UnlockResources();
        return RESUME_INVALID_RESUME_TYPE;
    }

    std::string szFullFilePath = ResumeManager::GetAppDataDir();

    std::string szDataFileName = szFullFilePath + szResumeFileName + _T(".dat");
    std::string szIndexFileName = szFullFilePath + szResumeFileName + _T(".idx");

    // Use the "undefined" type to mean "all"
    if ( resumeInfoType & resumeTypeUndefined ) {
        // Close any open files..
        int nResult = CloseOpenFiles();

        if (nResult == 0) {
	        nResult = FileDelete(szDataFileName);
	        if (nResult == 0) {
	            nResult =  FileDelete(szIndexFileName);
	        }
	    }

        UnlockResources();
    	return nResult;
	}

    // Open and verify the data from the resume info (checking the header) and
    // the resume index files.
    bool bIsFirstRun = false;

    int nResult = OpenResumeData(szResumeFileName, bIsFirstRun);
    if (nResult != 0) {
        UnlockResources();
        return nResult;
    }

    // If the files are emptry, we're done...
    if (bIsFirstRun) {
        nResult = CloseOpenFiles(szResumeFileName);
        UnlockResources();
        return RESUME_NO_MORE_DATA;
    }

	// We're either keeping all the "done" records or "resume" records.
	if (resumeInfoType & resumeDone) {
	    clearResumeInfoType = resumeDone;
	}

    int nIndex = 0;
    std::string szData = _T("");

    int nResumeSize = (int)m_listResumeIndexInfo.size();

    if (resumeInfoType & resumeUploads) {
        for (nIndex = 0; nIndex < nResumeSize; nIndex++) {
            szData = _T("");

            ResumeUploadInfoData resumeInfoData;
            ResumeIndexStruct resumeIndexInfo = m_listResumeIndexInfo[nIndex];

            nResult = fseek(m_pResumeFile, resumeIndexInfo.nPosition, SEEK_SET);

            if (nResult) {
                m_nLastError = errno;
                UnlockResources();
                return RESUME_SYSTEM_IO_ERROR;
            }

            if (ResumeInfoFileReadString(szData, CONSUME_NEWLINE_FROM_FILE) == false) {
                continue;
            }

            if ( resumeInfoData.Deserialize(szData, USE_RESUME_ENCRYPTION, m_dwEncryptLow,
                                            m_dwEncryptHigh) == false) {
                continue;
            }

            // Save only the resume type that we're looking for...
            if (resumeInfoData.GetResumeInfoType() != clearResumeInfoType) {
                m_listResumeUploadInfo.push_back(resumeInfoData);
            }
        }

        // Write out our new list of resume upload data, recreating the index
        // values along the way.
        fflush(m_pResumeFile);
        fclose(m_pResumeFile);

        SaveResumeInfoFile(resumeUploads);

    }
    else {
        for (nIndex = 0; nIndex < nResumeSize; nIndex++) {
            szData = _T("");
	        ResumeDownloadInfoData resumeInfoData;

            ResumeIndexStruct resumeIndexInfo = m_listResumeIndexInfo[nIndex];

            nResult = fseek(m_pResumeFile, resumeIndexInfo.nPosition, SEEK_SET);

            if (nResult) {
                m_nLastError = errno;
                UnlockResources();
                return RESUME_SYSTEM_IO_ERROR;
            }

	        if (ResumeInfoFileReadString(szData, CONSUME_NEWLINE_FROM_FILE) == false) {
	            continue;
	        }

	        if ( resumeInfoData.Deserialize(szData, USE_RESUME_ENCRYPTION, m_dwEncryptLow,
	                                        m_dwEncryptHigh) == false) {
	            continue;
	        }

            if (resumeInfoData.GetResumeInfoType() != clearResumeInfoType) {
	            m_listResumeDownloadInfo.push_back(resumeInfoData);
            }
        }

        // Write out our new list of resume upload data, recreating the index
        // values along the way.
        fflush(m_pResumeFile);
        fclose(m_pResumeFile);

        SaveResumeInfoFile(resumeDownloads);
    }

    UnlockResources();
	return nResult;

} // End ClearResumeFiles

///////////////////////////////////////////////////////////////////////
//! \brief  Instead of writing out the "done" entries, we'll re-write
//!         the file out to clear this entry from the resume files.  This
//!         assumes that the resume files are small, and this task will
//!         not slow the upload process.
//!
//! \param resumeInfoType: Resume type - undefined (all), completed (done),
//!                        incomplete (resume upload)
//! \param szResumeFileName: Resume index filename
//! \param resumeUploadInfo: resume info to save
//!
//! \return 0 if successful, error otherwise
//!
int ResumeManager::ClearResumeInfo(const ResumeInfoType resumeInfoType,
                                   std::string szResumeFileName,
                                   ResumeUploadInfoData& resumeUploadInfo)
{
    LockResources(_T("Resume clear resume info error"));
    ResetErrorCodes();

    // If there's no file ID assigned yet, ignore the request.
    if (resumeUploadInfo.GetFileID() == 0) {
        UnlockResources();
        return RESUME_NO_FILEID;
    }

	ResumeInfoType clearResumeInfoType = resumeTypeUndefined;

    if (resumeInfoType & resumeUploads) {
        szResumeFileName = RESUME_UPLOAD_FILENAME;
        clearResumeInfoType = resumeUploads;
        m_listResumeUploadInfo.clear();

    }
    else if (resumeInfoType & resumeDownloads) {
        szResumeFileName = RESUME_DOWNLOAD_FILENAME;
        clearResumeInfoType = resumeDownloads;
        m_listResumeDownloadInfo.clear();
    }
    else {
        UnlockResources();
        return RESUME_INVALID_RESUME_TYPE;
    }

    std::string szFullFilePath = ResumeManager::GetAppDataDir();

    std::string szDataFileName = szFullFilePath + szResumeFileName + _T(".dat");
    std::string szIndexFileName = szFullFilePath + szResumeFileName + _T(".idx");

    // Open and verify the data from the resume info (checking the header) and
    // the resume index files.
    bool bIsFirstRun = false;

    int nResult = OpenResumeData(szResumeFileName, bIsFirstRun);
    if (nResult != 0) {
        UnlockResources();
        return nResult;
    }

    // If the files are emptry, we're done...
    if (bIsFirstRun) {
        CloseOpenFiles(szResumeFileName);
        UnlockResources();
        return RESUME_NO_MORE_DATA;
    }

    //-----------------------------------------------------------------
    // The following scenario can happen if the file has been
    // emptied - just return with a file lock error to force a
    // retry.
    //-----------------------------------------------------------------

    // Existing data or new data - new data is skipped since we'll get rid of
    // it anyway.
    int nCurrentIndex = resumeUploadInfo.GetResumeIndex();
    if (nCurrentIndex < 0) {
        nResult = CloseOpenFiles(szResumeFileName);
        UnlockResources();
        return nResult;
    }

    if (nCurrentIndex >= (int)m_listResumeIndexInfo.size() ) {
        UnlockResources();
        return RESUME_FILE_LOCKING_ERROR;
    }

    ResumeIndexStruct currentResumeIndexInfo = m_listResumeIndexInfo[nCurrentIndex];
    clearResumeInfoType = ResumeInfoType(clearResumeInfoType | resumeDone);

    //-----------------------------------------------------------------
    //-----------------------------------------------------------------

    int nIndex = 0;
    std::string szData = _T("");

    int nResumeSize = (int)m_listResumeIndexInfo.size();

    if (resumeInfoType & resumeUploads) {
        for (nIndex = 0; nIndex < nResumeSize; nIndex++) {
            szData = _T("");

            ResumeUploadInfoData resumeInfoData;
            ResumeIndexStruct resumeIndexInfo = m_listResumeIndexInfo[nIndex];

            // If this is the same index, skip this entry
            if (currentResumeIndexInfo == resumeIndexInfo) {
                continue;
            }

            nResult = fseek(m_pResumeFile, resumeIndexInfo.nPosition, SEEK_SET);
            if (nResult) {
                m_nLastError = errno;
                UnlockResources();
                return RESUME_SYSTEM_IO_ERROR;
            }

            if (ResumeInfoFileReadString(szData, CONSUME_NEWLINE_FROM_FILE) == false) {
                continue;
            }

            if ( resumeInfoData.Deserialize(szData, USE_RESUME_ENCRYPTION, m_dwEncryptLow,
                                            m_dwEncryptHigh) == false) {
                continue;
            }

            // Amd in case the resume data wasn't correctly added, if the
            // file "appears" to be complete, get rid of this entry as well.
            if ( resumeInfoData.GetFileSize() == resumeInfoData.GetBytesRead() ){
                continue;
            }

            // Save only the resume type that we're looking for...
            if (resumeInfoData.GetResumeInfoType() != clearResumeInfoType) {
	            m_listResumeUploadInfo.push_back(resumeInfoData);
            }
        }

        // Write out our new list of resume upload data, recreating the index
        // values along the way.
        fflush(m_pResumeFile);
        fclose(m_pResumeFile);
        m_pResumeFile = 0;

        SaveResumeInfoFile(resumeUploads);

    }
    else {
        for (nIndex = 0; nIndex < nResumeSize; nIndex++) {
            szData = _T("");
	        ResumeDownloadInfoData resumeInfoData;

            ResumeIndexStruct resumeIndexInfo = m_listResumeIndexInfo[nIndex];

            // If this is the same index, skip this entry
            if (currentResumeIndexInfo == resumeIndexInfo) {
                continue;
            }

            nResult = fseek(m_pResumeFile, resumeIndexInfo.nPosition, SEEK_SET);
            if (nResult) {
                m_nLastError = errno;
                UnlockResources();
                return RESUME_SYSTEM_IO_ERROR;
            }

	        if (ResumeInfoFileReadString(szData, CONSUME_NEWLINE_FROM_FILE) == false) {
	            continue;
	        }

	        if ( resumeInfoData.Deserialize(szData, USE_RESUME_ENCRYPTION, m_dwEncryptLow,
	                                        m_dwEncryptHigh) == false) {
	            continue;
	        }

            if (resumeInfoData.GetResumeInfoType() != clearResumeInfoType) {
	            m_listResumeDownloadInfo.push_back(resumeInfoData);
            }
        }

        // Write out our new list of resume upload data, recreating the index
        // values along the way.
        fflush(m_pResumeFile);
        fclose(m_pResumeFile);
        m_pResumeFile = 0;

        SaveResumeInfoFile(resumeDownloads);
    }

    if (m_pResumeFile != NULL) {
        fflush(m_pResumeFile);
        fclose(m_pResumeFile);
        m_pResumeFile = 0;
    }

    UnlockResources();
	return nResult;

} // End ClearResumeInfo

///////////////////////////////////////////////////////////////////////
//! \brief Reads a the resume info record from the resume info data
//!        file.
//! \param resumeUploadInfo: resume info for results
//! \param bAddNewEntries: Add any entries which are not found in
//!                        the current resume list.
//! \return 0 if successful, error otherwise
//!
int ResumeManager::ReadResumeUploadData( ResumeUploadInfoData& resumeUploadInfo,
                                         bool bAddNewEntries /*false*/)
{
    LockResources(_T("Resume read upload data error"));
    ResetErrorCodes();

    // Data file should be opened...
    if (m_pResumeFile == NULL) {
        UnlockResources();
        return RESUME_OPEN_FILE_ERROR;
    }

	int nNumBytesRead = 0;
	std::string szData = _T("");

	std::string szFilePath = resumeUploadInfo.GetFilePath();

	if (szFilePath.length() == 0) {
        UnlockResources();

        #if defined(__APPLE__)
            ClientLog(UI_COMP, LOG_ERROR, false,
                _T("Resume read upload data error - file path is empty - returning No match found for resume data. "));
        #endif
	    return RESUME_NO_MATCHES_FOUND;
	}

	int nResult = 0;
	bool bFoundMatch = false;
	LONG64 l64FileSize = 0;

	ResumeIndexStruct resumeIndexInfo;

	int nIndex = 0;

	for (nIndex = 0; nIndex < (int)m_listResumeIndexInfo.size(); nIndex ++)  {
		resumeIndexInfo = m_listResumeIndexInfo[nIndex];

        #if 0
            // We may want to go back to fread if the fscan proves to be
            // slower -
		    if ( CreateBuffer(resumeIndexInfo.nSize + 1) == false ) {
                UnlockResources();
		        return RESUME_CREATE_BUFFER_ERROR;
		    }

		    // Read the entry into our buffer
            int nResult = fseek(m_pResumeFile, resumeIndexInfo.nPosition, SEEK_SET);
            if (nResult) {
                m_nLastError = errno;
                UnlockResources();
                return RESUME_SYSTEM_IO_ERROR;
            }

            // Using the ResumeInfoFileReadString below instead...
            nNumBytesRead = static_cast<unsigned int>(fread( (char *)m_szBuffer, 1,
                                                             static_cast<size_t>(resumeIndexInfo.nSize + 1),
                                                             m_pResumeFile ));
            if (nNumBytesRead == 0) {
                UnlockResources();
                return RESUME_NO_MORE_DATA;
            }

		    m_szBuffer[resumeIndexInfo.nSize] = '\0';
		    szData = m_szBuffer;
		#endif

        nResult = fseek(m_pResumeFile, resumeIndexInfo.nPosition, SEEK_SET);
        if (nResult) {
            m_nLastError = errno;
            UnlockResources();
            return RESUME_SYSTEM_IO_ERROR;
        }

		// Deserialize the file data into our resume upload data.
		ResumeUploadInfoData tmpResumeUploadData;
		szData = _T("");

	    if (ResumeInfoFileReadString(szData, CONSUME_NEWLINE_FROM_FILE) == false) {
	        continue;
	    }

		if ( tmpResumeUploadData.Deserialize(szData, USE_RESUME_ENCRYPTION,
		                                     m_dwEncryptLow, m_dwEncryptHigh) == false) {
	        continue;
	    }

		// If it's not an upload type, move on
		ResumeInfoType nResumeInfoType = tmpResumeUploadData.GetResumeInfoType();
		if (nResumeInfoType != resumeUploads) {
		    continue;
		}

        bFoundMatch = false;
        l64FileSize = 0;

		// Is this the path we want?  This logic falls apart if there are multiple
		// entries with file path/name - if the file path, size, and last modified
		// time all match, we'll take this hit.
		if ( 0 == stricmp(tmpResumeUploadData.GetFilePath().c_str(), szFilePath.c_str())) {

		    // Check the file size and modified date as well...
		    l64FileSize = Util::GetFileLength64(szFilePath.c_str());
		    if (l64FileSize == tmpResumeUploadData.GetFileSize()) {

		        // And lastly, check the last modified date.
                time_t tmLastModified;

                if ( -1 != Util::GetFileLastModifiedTime(szFilePath.c_str(), tmLastModified) ) {

                    if ( tmLastModified == tmpResumeUploadData.GetLastModified() ) {

		                resumeUploadInfo = tmpResumeUploadData;
		                resumeUploadInfo.SetResumeIndex(nIndex);
		                bFoundMatch = true;
                    }
                }
		    }
		}

		if (bFoundMatch == true) {
		    break;
		}
	}

	if ( (bFoundMatch == false) && (bAddNewEntries == true) ) {
	    // Add the resume data as a new entry
        time_t tmLastModified;

        if (-1 != Util::GetFileLastModifiedTime(szFilePath.c_str(), tmLastModified)) {
            resumeUploadInfo.SetLastModified(tmLastModified);
        }

	    l64FileSize = Util::GetFileLength64(szFilePath.c_str());
	    resumeUploadInfo.SetFileSize(l64FileSize);

        bFoundMatch = true;
	}

    fflush(m_pResumeFile);

    if (bFoundMatch == false) {

        #if defined(__APPLE__)
            ClientLog(UI_COMP, LOG_ERROR, false,
                _T("Resume read upload data error - found match is false - returning No match found for resume data. "));
        #endif
        nResult = RESUME_NO_MATCHES_FOUND;
    }

    UnlockResources();
	return nResult;

} // End ReadResumeUploadData

///////////////////////////////////////////////////////////////////////
//! \brief Reads a the resume info record from the resume info data
//!        file.
//! \param szResumeFileName: Resume index/data filename
//! \param resumeUploadInfo: resume info for results
//! \param bAddNewEntries: Add any entries which are not found in
//!                        the current resume list.
//! \return 0 if successful, error otherwise
//!
int ResumeManager::ReadResumeUploadDataByFileID( ResumeUploadInfoData* pResumeUploadInfo,
                                                 bool bAddNewEntries /*false*/)
{
    LockResources(_T("Resume read upload data by file ID error"));

    ResetErrorCodes();
    m_listResumeUploadInfo.clear();

	int nResult = LoadResumeInfo(ResumeInfoTypes::resumeUploads);
    if (nResult != 0) {
        UnlockResources();
        return nResult;
    }

    // Not an error, since the data hasn't been added.
    if (pResumeUploadInfo->GetResumeIndex() < 0) {
        UnlockResources();
        return 0;
    }

    if (m_listResumeUploadInfo.size() == 0) {
        pResumeUploadInfo->SetResumeIndex(-1);
        UnlockResources();
        return RESUME_NO_MORE_DATA;
    }

    ResumeUploadInfoData tmpResumeUploadInfoData;
    int nIndex = 0;
    bool bFoundMatch = false;
    nResult = 0;

    for (nIndex = 0; nIndex < (int)m_listResumeUploadInfo.size(); nIndex ++) {
        tmpResumeUploadInfoData = m_listResumeUploadInfo[nIndex];

        // If we find the one we're looking for, update the index information -
        // if multipe instances of DioCLI are running, the position of this
        // data in the file may have changed.
        if (tmpResumeUploadInfoData.GetFileID() == pResumeUploadInfo->GetFileID() ) {
            bFoundMatch = true;
            pResumeUploadInfo->SetResumeIndex(tmpResumeUploadInfoData.GetResumeIndex());
            break;
        }
    }

    if (bFoundMatch == false) {
        // Setting the index to -1 will force this entry to the bottom
        // of the file with a new created index value and position.
        nResult = RESUME_NO_MATCHES_FOUND;
        pResumeUploadInfo->SetResumeIndex(-1);

        #if defined(__APPLE__)
            ClientLog(UI_COMP, LOG_ERROR, false,
                _T("Resume read upload data by file ID error - found match is false - returning No match found for resume data. "));
        #endif
    }

    UnlockResources();
	return nResult;

} // End ReadResumeUploadDataByFileID

///////////////////////////////////////////////////////////////////////
//! \brief Writes the resume info record to the resume info data
//!        file.
//! \param szResumeFileName: Resume index filename
//! \param resumeUploadInfo: resume info to save
//!
//! \return 0 if successful, error otherwise
//!
int ResumeManager::WriteResumeUploadData( std::string szResumeFileName,
                                          ResumeUploadInfoData& resumeUploadInfo )
{
    // If there's no file ID assigned yet, ignore the request.
    if (resumeUploadInfo.GetFileID() == 0) {
        return RESUME_NO_FILEID;
    }

    LockResources(_T("Resume write upload data error"));
    ResetErrorCodes();

    // Make sure the data we're getting is in sync with the data in the files.
    int nResult = ReadResumeUploadDataByFileID(&resumeUploadInfo, true);

    if (nResult != 0) {
        if (nResult == RESUME_NO_MATCHES_FOUND) {
            ClientLog(UI_COMP, LOG_WARNING, false,
                _T("Resume write upload data warning - no match found: %s"),
                resumeUploadInfo.GetFilePath().c_str() );
        }
        else if (nResult == RESUME_NO_MORE_DATA) {
            ClientLog(UI_COMP, LOG_WARNING, false,
                _T("Resume write upload data warning - data files empty: %s"),
                resumeUploadInfo.GetFilePath().c_str() );
        }
        else {
            // More than like a file locking error...
            UnlockResources();
            return nResult;
        }
    }

    // Clear the resume data for this file only if it's in the file....
    if ( resumeUploadInfo.GetResumeInfoType() & resumeDone) {
        if (nResult == 0) {
            nResult = ClearResumeInfo(ResumeInfoType(resumeDone | resumeUploads),
                                      szResumeFileName, resumeUploadInfo);        }
        else {
            ClientLog(UI_COMP, LOG_WARNING, false,
                _T("Resume write upload data warning - clear file info skipped since no data found: %s"),
                resumeUploadInfo.GetFilePath().c_str() );
        }

        UnlockResources();
        return nResult;
    }

    nResult = 0;

    // Data file should be opened.
    assert(m_pResumeFile != NULL);

    // If this is the first run of the file, write out the version and encryption
    // data.
    if (m_bIsFirstRun == true) {
        fprintf(m_pResumeFile, _T("Ver: %d\n"), RESUME_FILE_VER);

        #ifdef WIN32
            FILETIME fileTime;
            fileTime.dwLowDateTime = fileTime.dwHighDateTime = 0;
            GetSystemTimeAsFileTime(&fileTime);

            m_dwEncryptLow  = fileTime.dwLowDateTime;
            m_dwEncryptHigh = fileTime.dwHighDateTime;
            m_nFileVersion = RESUME_FILE_VER;

        #else
            timeval tv;
            gettimeofday(&tv, NULL);

            m_dwEncryptLow  = tv.tv_usec;
            m_dwEncryptHigh = 0;
        #endif

        fprintf(m_pResumeFile, _T("%lu %lu\n\n"), m_dwEncryptLow, m_dwEncryptHigh);
        m_bIsFirstRun = false;
    }

    // Serialize the upload data - this step here so we can verify we have enough
    // room for our storage in the case where the data already exists.
    std::string szData = resumeUploadInfo.Serialize(USE_RESUME_ENCRYPTION, m_dwEncryptLow,
                                                    m_dwEncryptHigh);

    // For debugging....
    assert(szData.length() > 0);

    ResumeIndexStruct resumeIndexInfo;
    bool bNewResumeInfo = false;
    long lSize = 0;

    //-----------------------------------------------------------------
    // The following scenario can happen if the file has been
    // emptied - just return with a file lock error to force a
    // retry.
    //-----------------------------------------------------------------
    int nCurrentIndex = resumeUploadInfo.GetResumeIndex();
    if ( nCurrentIndex >= (int)m_listResumeIndexInfo.size() ) {
        UnlockResources();
        return RESUME_FILE_LOCKING_ERROR;
    }

    // Existing data or new data to add to the end of the data file.
    if (nCurrentIndex >= 0) {
        resumeIndexInfo = m_listResumeIndexInfo[nCurrentIndex];

        // Position our file pointer
        nResult = fseek(m_pResumeFile, resumeIndexInfo.nPosition, SEEK_SET);
        if (nResult) {
            m_nLastError = errno;
            UnlockResources();
            return RESUME_SYSTEM_IO_ERROR;
        }
    }
    else {
        bNewResumeInfo = true;
        nResult = fseek(m_pResumeFile, 0, SEEK_END);
        if (nResult) {
            m_nLastError = errno;
            UnlockResources();
            return RESUME_SYSTEM_IO_ERROR;
        }

        lSize = ftell(m_pResumeFile);
    }

    // Write out the upload data: size of data + data (which, when serialized, ends with the
    // data separator.
    std::string szOutput = _T("");
    StringUtil::AppendIntToString(szOutput, (unsigned int)szData.length(), *DATUM_SEP.c_str());
    szOutput += szData;

    int nOutputLength = (int)szOutput.length();

    // Make sure there is room in the file - otherwise, we've got a coding error here...
    // We're subtracting 1 in our calculations to account for the addition of the newline
    // that follows.
    if ( !bNewResumeInfo && (resumeIndexInfo.nSize > nOutputLength )) {
        std::string szPadStr = StringUtil::GetPadStr(resumeIndexInfo.nSize - nOutputLength - 1);
        szOutput.append(szPadStr);
        nOutputLength = szOutput.length();
    }

    if (ADD_NEWLINE_TO_FILE == true) {
        szOutput += _T("\n");
        nOutputLength ++;
    }

    // Setup the size for the new entry or verify that the exisitng entry
    // can fit correctly within the space allocated.
    if (bNewResumeInfo) {
        resumeIndexInfo.nPosition = (int)lSize;
        resumeIndexInfo.nSize = nOutputLength;

        // Note that these are addede by reference - we'll add it now to
        // avoid having to find/erase/push again scenario.

        m_listResumeIndexInfo.push_back(resumeIndexInfo);
        WriteResumeIndex(RESUME_UPLOAD_FILENAME);

        nCurrentIndex = m_listResumeIndexInfo.size() - 1;
        resumeUploadInfo.SetResumeIndex(nCurrentIndex);

    }
    else {
        if ( resumeIndexInfo.nSize < nOutputLength) {
            UnlockResources();
            return RESUME_INDEX_RECORD_ERROR;
        }
    }

    int nSize = fwrite((char *)szOutput.c_str(), sizeof(char), nOutputLength,
                        m_pResumeFile);

    /*
    if (nSize != szOutput.length()) {
        m_nLastError = errno;
        return RESUME_SYSTEM_IO_ERROR;
    }
    */

    ClientLog(UI_COMP, LOG_STATUS, false, _T("Resume data[%d] (%d) (%d): %s"),
        nCurrentIndex, nOutputLength, nSize, szOutput.c_str());

    fflush(m_pResumeFile);
    UnlockResources();
	return nResult;

} // End WriteResumeUploadData

///////////////////////////////////////////////////////////////////////
//! \brief Return the number of resume records using the index file
//!        count.
//! \return count of indexed entries
//!
int ResumeManager::GetResumeUploadRecordCount()
{
	return (int)m_listResumeIndexInfo.size()-1;

} // End GetResumeUploadRecordCount

///////////////////////////////////////////////////////////////////////
//! \brief Reads a the resume info record from the resume info data
//!        file.
//! \param szResumeFileName: Resume index filename
//! \param pResumeInfoData: resume info for results
//! \param bAddNewEntries: Add any entries which are not found in
//!                        the current resume list.
//! \return 0 if successful, error otherwise
//!
int ResumeManager::ReadResumeDownloadData( std::string szResumeFileName,
                                           ResumeDownloadInfoData& resumeDownloadInfo,
                                           bool bAddNewEntries /*false*/)
{
    // Data file should be opened...
    if (m_pResumeFile == NULL) {
        return RESUME_OPEN_FILE_ERROR;
    }

    int nResult = 0;

    //! TBD

	return nResult;

} // End ReadResumeDownloadData

///////////////////////////////////////////////////////////////////////
//! \brief Writes the resume info record to the resume info data
//!        file.
//! \param szResumeFileName: Resume index filename
//! \param resumeUploadInfo: resume info to save
//!
//! \return 0 if successful, error otherwise
//!
int ResumeManager::WriteResumeDownloadData( std::string szResumeFileName,
                                            ResumeDownloadInfoData& resumeDownloadInfo )
{
    int nResult = 0;

    //! TBD

	return nResult;

} // End WriteResumeDownloadData

///////////////////////////////////////////////////////////////////////
//! \brief Return the number of resume records using the index file
//!        count.
//! \return count of indexed entries
//!
int ResumeManager::GetResumeDownloadRecordCount()
{
	return (int)m_listResumeIndexInfo.size()-1;

} // End GetResumeDownloadRecordCount

///////////////////////////////////////////////////////////////////////
int ResumeManager::SaveResumeInfoFile(const ResumeInfoType resumeInfoType)
{
    LockResources(_T("Resume save info file error"));

    // Close these files and open to "clear" - the file data will be updated
    // from the data in memory.
	if ( m_pResumeFile != NULL) {
	    fflush(m_pResumeFile);
	    fclose(m_pResumeFile);
	    m_pResumeFile = 0;
	}

	if ( m_pResumeIndexFile != NULL) {
	    fflush(m_pResumeIndexFile);
	    fclose(m_pResumeIndexFile);
	    m_pResumeIndexFile = 0;
	}

    // Clear the index list - the processing of saving the resume data
    // will also update the index list correctly.
	m_listResumeIndexInfo.clear();

    std::string szResumeFileName = RESUME_UPLOAD_FILENAME;
    if (resumeInfoType == resumeDownloads) {
        szResumeFileName = RESUME_DOWNLOAD_FILENAME;
    }

    // Open the resume file to overwrite/create the empty file.
    std::string szDataFileName = m_szResumeFileName + _T(".dat");

    #ifdef WIN32
        m_pResumeFile = _fsopen( szDataFileName.c_str(), _T("wt+"), SH_DENYNO);
    #else
	    m_pResumeFile = _tfopen( szDataFileName.c_str(), _T("wt+"));
    #endif

    if (m_pResumeFile == NULL) {
        UnlockResources();
        return RESUME_OPEN_FILE_ERROR;
    }

    // File versioning.  Write some version info to help with future parsing of this file.
    // Also, compute and write a date/time stamp.  This stamp will be salted and used to
    // key our password encryption before the password is written to this file.
    // Read the version and encryption information.
    fprintf(m_pResumeFile, _T("Ver: %d\n"), RESUME_FILE_VER);

    #ifdef WIN32
        FILETIME fileTime;
        fileTime.dwLowDateTime = fileTime.dwHighDateTime = 0;
        GetSystemTimeAsFileTime(&fileTime);

        m_dwEncryptLow  = fileTime.dwLowDateTime;
        m_dwEncryptHigh = fileTime.dwHighDateTime;

    #else
        timeval tv;
        gettimeofday(&tv, NULL);

        m_dwEncryptLow  = tv.tv_usec;
        m_dwEncryptHigh = 0;
    #endif

    fprintf(m_pResumeFile, _T("%lu %lu\n\n"), m_dwEncryptLow, m_dwEncryptHigh);

    if (resumeInfoType == resumeUploads) {
        //! Resume upload info
        ApplyToEachResumeUploadInfo(SaveResumeUploadState, m_listResumeUploadInfo);
    }
    else {
        //! Resume download info
        ApplyToEachResumeDownloadInfo(SaveResumeDownloadState, m_listResumeDownloadInfo);
    }

    // Write out the resume index data - at this point, the size and positions of the
    // new data have been added to the index list.
    WriteResumeIndex(szResumeFileName);

    // Last resume time + newline - at this point, knowing the last time we
    // resumed isn't needed.
    // time_t tLastResumeTime = GetLastResumeTime();
    // fprintf(m_pResumeFile, _T("%ld,\n"), tLastResumeTime);

    UnlockResources();
	return 0;

} // End SaveResumeInfoFile

///////////////////////////////////////////////////////////////////////
int ResumeManager::LoadResumeInfo(const ResumeInfoType resumeInfoType)
{
    LockResources(_T("Resume load resume info error"));
    ResetErrorCodes();

    std::string szResumeFileName = _T("");
    if (resumeInfoType == resumeUploads) {
        szResumeFileName = RESUME_UPLOAD_FILENAME;
        m_listResumeUploadInfo.clear();
    }
    else {
        szResumeFileName = RESUME_DOWNLOAD_FILENAME;
        m_listResumeDownloadInfo.clear();
    }

    bool bIsFirstRun = false;
    int nResult = OpenResumeData(szResumeFileName, bIsFirstRun);
    if (nResult != 0) {
        UnlockResources();
        return nResult;
    }

    // Files are empty
    if (bIsFirstRun) {
        UnlockResources();
        return RESUME_NO_MATCHES_FOUND;
    }

    // If the file is opened, it's closed since we're going to open it for
    // reading.
	if (m_pResumeFile != NULL) {
	    fflush(m_pResumeFile);
	    fclose(m_pResumeFile);
	    m_pResumeFile = 0;
	}

    // Open the resume file for reading
    std::string szDataFileName = m_szResumeFileName + _T(".dat");
	#ifdef WIN32
        m_pResumeFile = _fsopen( szDataFileName.c_str(), _T("rt+"), SH_DENYNO);
    #else
    	m_pResumeFile = _tfopen( szDataFileName.c_str(), _T("rt+"));
    #endif
	if (m_pResumeFile == NULL) {
        UnlockResources();
	    return RESUME_OPEN_FILE_ERROR;
	}

	//--------------------------------------------------------------------
    // Resume upload info
	//--------------------------------------------------------------------
	int nIndex = 0;
	std::string szData = _T("");
	int nResumeSize = 0;

	switch(m_nFileVersion) {
		case 0:
		case 1:
            UnlockResources();
		    return RESUME_INVALID_CONFIG_FORMAT;
		    break;
		case 2:
		    {
				nResumeSize = (int)m_listResumeIndexInfo.size();

                if (resumeInfoType == resumeUploads) {
				    for (nIndex = 0; nIndex < nResumeSize; nIndex++) {
				        szData = _T("");

					    ResumeUploadInfoData resumeInfoData;
                        ResumeIndexStruct resumeIndexInfo = m_listResumeIndexInfo[nIndex];

                        nResult = fseek(m_pResumeFile, resumeIndexInfo.nPosition, SEEK_SET);
                        if (nResult) {
                            m_nLastError = errno;
                            UnlockResources();
                            return RESUME_SYSTEM_IO_ERROR;
                        }

					    if (ResumeInfoFileReadString(szData, CONSUME_NEWLINE_FROM_FILE) == false) {
					        continue;
					    }

					    if ( resumeInfoData.Deserialize(szData, USE_RESUME_ENCRYPTION, m_dwEncryptLow,
					                                    m_dwEncryptHigh) == false) {
					        continue;
					    }

					    resumeInfoData.SetResumeIndex(nIndex);
					    m_listResumeUploadInfo.push_back(resumeInfoData);
				    }
				}
				else {
				    for (nIndex = 0; nIndex < nResumeSize; nIndex++) {
				        szData = _T("");
					    ResumeDownloadInfoData resumeInfoData;

                        ResumeIndexStruct resumeIndexInfo = m_listResumeIndexInfo[nIndex];

                        nResult = fseek(m_pResumeFile, resumeIndexInfo.nPosition, SEEK_SET);
                        if (nResult) {
                            m_nLastError = errno;
                            UnlockResources();
                            return RESUME_SYSTEM_IO_ERROR;
                        }

					    if (ResumeInfoFileReadString(szData, CONSUME_NEWLINE_FROM_FILE) == false) {
					        continue;
					    }

					    if ( resumeInfoData.Deserialize(szData, USE_RESUME_ENCRYPTION, m_dwEncryptLow,
					                                    m_dwEncryptHigh) == false) {
					        continue;
					    }

					    resumeInfoData.SetResumeIndex(nIndex);
					    m_listResumeDownloadInfo.push_back(resumeInfoData);
				    }
				}
			}
			break;
		default:
			break;
	}

	// Last resume time and newline - not needed at this time...
	#if 0
	    time_t epochSeconds = 0;
	    GetEpochSeconds(epochSeconds);

 	    nNumBytesRead =  _ftscanf(m_pResumeFile, _T("%ld,"), &epochSeconds);
	    if ((nNumBytesRead == 0) || (nNumBytesRead == EOF)) {
		    return true;
	    }
	    SetLastResumeTime(epochSeconds);

        // Newline
    	char szChar;
	    nNumBytesRead = _ftscanf(m_pResumeFile, _T("%c"), &szChar);
	    if ((nNumBytesRead == 0) || (nNumBytesRead == EOF)) {
		    return true;
	    }
	#endif

    UnlockResources();
	return 0;

} // End LoadResumeInfo

/////////////////////////////////////////////////////////////////////////////
//! \brief Apply a function to every resume info data in the list
//! \param ResumeListFunction: function pointer
//! \param resumeInfoList: list of resume info data objects
//
void ResumeManager::ApplyToEachResumeUploadInfo( int (*ResumeListFunction ) (ResumeManager*, ResumeUploadInfoData&),
                                                 t_resumeUploadInfoList& resumeInfoList)
{
	for (t_resumeUploadInfoList::iterator iter = resumeInfoList.begin(); iter != resumeInfoList.end(); ++iter)  {
		ResumeUploadInfoData& resumeInfoData = *iter;
		ResumeListFunction(this, resumeInfoData);
	}

} // End ApplyToEachResumeUploadInfo

/////////////////////////////////////////////////////////////////////////////
//! \brief Apply a function to every resume info data in the list
//! \param ResumeListFunction: function pointer
//! \param resumeInfoList: list of resume info data objects
//
void ResumeManager::ApplyToEachResumeDownloadInfo( int (*ResumeListFunction )(ResumeManager*, ResumeDownloadInfoData&),
                                                  t_resumeDownloadInfoList& resumeInfoList)
{
	for (t_resumeDownloadInfoList::iterator iter = resumeInfoList.begin(); iter != resumeInfoList.end(); ++iter)  {
		ResumeDownloadInfoData& resumeInfoData = *iter;
		ResumeListFunction(this, resumeInfoData);
	}

} // End ApplyToEachResumeDownloadInfo

///////////////////////////////////////////////////////////////////////
// Public Methods

/////////////////////////////////////////////////////////////////////////////
t_resumeUploadInfoList& ResumeManager::GetResumeUploadInfoList()
{
    if (m_listResumeUploadInfo.size() == 0) {
        // Load the list - we assume the user wants the data from
        // storage.
        Load(resumeUploads);
    }

	return m_listResumeUploadInfo;

} // End GetResumeUploadInfoList

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//! \brief Return the resume data associated with the list of files.
//! \param listFiles: list of files with file path
//! \param bAddNewEntries: Add any entries which are not found in
//!                        the current resume list.
//
t_resumeUploadInfoList& ResumeManager::GetResumeUploadInfoList(const std::vector<std::string>& listFiles,
                                                               bool bAddNewEntries /*false*/)
{
    m_listResumeUploadInfo.clear();

    // Empty list - return all the entries...
    if (listFiles.size() == 0) {
        return GetResumeUploadInfoList();
    }

    int nIndex = 0;
    std::string szFilePath = _T("");

    for (nIndex = 0; nIndex < (int)listFiles.size(); nIndex ++) {
        szFilePath = listFiles[nIndex];

        ResumeUploadInfoData resumeUploadData;
        resumeUploadData.SetFilePath(szFilePath);

        if (ReadResumeUploadData(resumeUploadData, bAddNewEntries) == 0) {
            m_listResumeUploadInfo.push_back(resumeUploadData);
        }
    }

	return m_listResumeUploadInfo;

} // End GetResumeUploadInfoList

/////////////////////////////////////////////////////////////////////////////
t_resumeDownloadInfoList& ResumeManager::GetResumeDownloadInfoList()
{
    if (m_listResumeDownloadInfo.size() == 0) {
        // Load the list - we assume the user wants the data from
        // storage.
        Load(resumeDownloads);
    }

	return m_listResumeDownloadInfo;

} // End GetResumeDownloadInfoList

/////////////////////////////////////////////////////////////////////////////
t_resumeDownloadInfoList& ResumeManager::GetResumeDownloadInfoList(const std::vector<std::string>& listFiles,
                                                                   bool bAddNewEntries /*false*/)
{
	return m_listResumeDownloadInfo;

} // End GetResumeDownloadInfoList

///////////////////////////////////////////////////////////////////////
//! \brief Load the resume data from the file.
//!
//! \param resumeInfoType: resume info type
//! \return 0 if successful, error code otherwise
//!
int ResumeManager::Load( const ResumeInfoType resumeInfoType )
{
    LockResources(_T("Resume load error"));

    // Open the .dat and .idx file.
    std::string szResumeFileName = _T("");
    bool bSuccess = true;

    switch (resumeInfoType) {
        case resumeUploads:
            szResumeFileName = RESUME_UPLOAD_FILENAME;
            break;
        case resumeDownloads:
            szResumeFileName = RESUME_DOWNLOAD_FILENAME;
            break;
        default:
            bSuccess = false;
            break;
    }

    if (bSuccess == false) {
        UnlockResources();
        return RESUME_INVALID_RESUME_TYPE;
    }

	int nResult = LoadResumeInfo(resumeInfoType);

    // Files are empty
    if (m_bIsFirstRun) {
        UnlockResources();
        return RESUME_ZERO_FILE_LENGTH;
    }

	CloseOpenFiles();

	if (nResult == RESUME_INVALID_CONFIG_FORMAT) {
		ClientLog(UI_COMP, LOG_ERROR, false, _T("Resume data file existed but is invalid"));

		// Something wrong in the file - delete and make like a new file
		m_bIsFirstRun = true;

        std::string szFullFilePath = ResumeManager::GetAppDataDir();

        std::string szDataFileName = szFullFilePath + szResumeFileName + _T(".dat");
        std::string szIndexFileName = szFullFilePath + szResumeFileName + _T(".idx");

		FileDelete(szDataFileName);
		FileDelete(szIndexFileName);

		SetupForNewResumeFile();
	}

	m_bIsFileValid = (nResult == 0);
    UnlockResources();
	return nResult;

} // End Load

///////////////////////////////////////////////////////////////////////
bool ResumeManager::Save( const ResumeInfoType resumeInfoType)
{
    LockResources(_T("Resume save error"));

    // Open the .dat and .idx file.
    std::string szResumeFileName = _T("");
    bool bSuccess = true;

    switch (resumeInfoType) {
        case resumeUploads:
            szResumeFileName = RESUME_UPLOAD_FILENAME;
            break;
        case resumeDownloads:
            szResumeFileName = RESUME_DOWNLOAD_FILENAME;
            break;
        default:
            bSuccess = false;
            break;
    }

    if (bSuccess == false) {
        UnlockResources();
        return false;
    }

    bool bIsFirstRun = false;
    int nResult = OpenResumeData(szResumeFileName, bIsFirstRun);
    if (nResult != 0) {
        UnlockResources();
        return false;
    }

	bSuccess = (SaveResumeInfoFile(resumeInfoType) == 0);
	CloseOpenFiles(szResumeFileName);

    UnlockResources();
	return bSuccess;

} // End Save

/////////////////////////////////////////////////////////////////////////////
int ResumeManager::SaveResumeUploadState( ResumeManager* pResumeMgr, ResumeUploadInfoData& resumeInfoData)
{
    assert(pResumeMgr != NULL);
    pResumeMgr->LockResources(_T("Resume save upload state error"));
    pResumeMgr->ResetErrorCodes();

    // Serialize the upload data - this step here so we can verify we have enough
    // room for our storage in the case where the data already exists.
	std::string szData = resumeInfoData.Serialize(USE_RESUME_ENCRYPTION, pResumeMgr->GetEncryptLow(),
	                                              pResumeMgr->GetEncryptHigh());

	FILE* pResumeFile = pResumeMgr->GetResumeInfoFile();
	if (pResumeFile == NULL) {
        pResumeMgr->UnlockResources();
	    return RESUME_OPEN_FILE_ERROR;
	}

    int nResult = fseek(pResumeFile, 0, SEEK_END);
    if (nResult) {
        pResumeMgr->SetLastError(errno);
        pResumeMgr->UnlockResources();
        return RESUME_SYSTEM_IO_ERROR;
    }

    // Write out the upload data: size of data + data (which, when serialized, ends with the
    // data separator.
    std::string szOutput = _T("");
	StringUtil::AppendIntToString(szOutput, (unsigned int)szData.length(), *DATUM_SEP.c_str());
	szOutput += szData;

    int nOutputLength = (int)szOutput.length();

    if (ADD_NEWLINE_TO_FILE == true) {
        szOutput += _T("\n");
        nOutputLength ++;
    }

    long lSize = ftell(pResumeFile);
    ResumeIndexStruct resumeIndexInfo;

    resumeIndexInfo.nPosition = (int)lSize;
    resumeIndexInfo.nSize = nOutputLength;

    t_resumeIndexInfoList* pListResumeIndexInfo = pResumeMgr->GetIndexInfoList();
    pListResumeIndexInfo->push_back(resumeIndexInfo);

    int nSize = fwrite((char *)szOutput.c_str(), sizeof(char), nOutputLength,
                        pResumeFile);

    fflush(pResumeFile);
    pResumeMgr->UnlockResources();
	return nResult;

    /*
	return pResumeMgr->ResumeInfoFileWriteString(resumeInfoData.Serialize(USE_RESUME_ENCRYPTION,
	                          pResumeMgr->GetEncryptLow(), pResumeMgr->GetEncryptHigh()),
	                          ADD_NEWLINE_TO_FILE);
    */

} // End SaveResumeUploadState

/////////////////////////////////////////////////////////////////////////////
int ResumeManager::SaveResumeDownloadState( ResumeManager* pResumeMgr, ResumeDownloadInfoData& resumeInfoData)
{
    assert(pResumeMgr != NULL);

    pResumeMgr->LockResources(_T("Resume save download state error"));
    pResumeMgr->ResetErrorCodes();

    // Serialize the upload data - this step here so we can verify we have enough
    // room for our storage in the case where the data already exists.
	std::string szData = resumeInfoData.Serialize(USE_RESUME_ENCRYPTION, pResumeMgr->GetEncryptLow(),
	                                              pResumeMgr->GetEncryptHigh());

	FILE* pResumeFile = pResumeMgr->GetResumeInfoFile();
	if (pResumeFile == NULL) {
        pResumeMgr->UnlockResources();
	    return RESUME_OPEN_FILE_ERROR;
	}

    int nResult = fseek(pResumeFile, 0, SEEK_END);
    if (nResult) {
        pResumeMgr->SetLastError(errno);
        pResumeMgr->UnlockResources();
        return RESUME_SYSTEM_IO_ERROR;
    }

    // Write out the upload data: size of data + data (which, when serialized, ends with the
    // data separator.
    std::string szOutput = _T("");
	StringUtil::AppendIntToString(szOutput, (unsigned int)szData.length(), *DATUM_SEP.c_str());
	szOutput += szData;

    int nOutputLength = (int)szOutput.length();

    if (ADD_NEWLINE_TO_FILE == true) {
        szOutput += _T("\n");
        nOutputLength ++;
    }

    long lSize = ftell(pResumeFile);
    ResumeIndexStruct resumeIndexInfo;

    resumeIndexInfo.nPosition = (int)lSize;
    resumeIndexInfo.nSize = nOutputLength;

    t_resumeIndexInfoList* pListResumeIndexInfo = pResumeMgr->GetIndexInfoList();
    pListResumeIndexInfo->push_back(resumeIndexInfo);

    int nSize = fwrite((char *)szOutput.c_str(), sizeof(char), nOutputLength,
                        pResumeFile);

    fflush(pResumeFile);
    pResumeMgr->UnlockResources();
	return nResult;

    /*
	return pResumeMgr->ResumeInfoFileWriteString(resumeInfoData.Serialize(USE_RESUME_ENCRYPTION,
	                          pResumeMgr->GetEncryptLow(), pResumeMgr->GetEncryptHigh()),
	                          ADD_NEWLINE_TO_FILE);
	*/

} // End SaveResumeDownloadState

/////////////////////////////////////////////////////////////////////////////
std::string& ResumeManager::GetAppDataDir()
{
	if (m_szAppDirectory.length() > 0) {
	    return m_szAppDirectory;
	}

    // Load the certificate from either the resource or linked object
    // and write this data out to a file.
    std::string szSlash = _T("\\");
    #ifndef WIN32
        szSlash = _T("/");
    #endif

    if ( true == Util::GetDataDirectory(m_szAppDirectory)) {
        m_szAppDirectory += szSlash;
    }
    else {
        m_szAppDirectory = _T("./");
    }

	return m_szAppDirectory;

} // End GetAppDataDir

/////////////////////////////////////////////////////////////////////////////
time_t *ResumeManager::ResumeTime()
{
	return &m_tResumeTime;

} // End ResumeTime

/////////////////////////////////////////////////////////////////////////////
time_t& ResumeManager::GetLastResumeTime()
{
	return *ResumeTime();

} // End GetLastResumeTime

/////////////////////////////////////////////////////////////////////////////
void ResumeManager::SetLastResumeTime(time_t& timeNow)
{
	*ResumeTime() = timeNow;

} // End SetLastResumeTime

/////////////////////////////////////////////////////////////////////////////
void ResumeManager::SetCurrentVersion(int major, int minor, int build)
{
} // End SetCurrentVersion

///////////////////////////////////////////////////////////////////////
int ResumeManager::ResumeInfoFileWriteString(std::string szIn, bool bAddNewLine)
{
    if (m_pResumeFile == NULL) {
        return 0;
    }

    LockResources(_T("Resume file write string error"));

	// As the resume data file is comma delineated, convert them
	// szIn = StringUtil::ConvertCommas(szIn);

	int nCount = fprintf(m_pResumeFile, _T("%d,"), szIn.size());
	nCount += fprintf(m_pResumeFile, _T("%s,"), szIn.c_str());
	if (bAddNewLine) {
		nCount += fprintf(m_pResumeFile, _T("\n"));
	}

    UnlockResources();
	return nCount;

} // End ResumeInfoFileWriteString

///////////////////////////////////////////////////////////////////////
bool ResumeManager::ResumeInfoFileReadString(std::string& szIn, bool bConsumeNewLine)
{
	if (m_pResumeFile == NULL) {
	    return false;
	}

    LockResources(_T("Resume file read string error"));

	char szValue[1024];
	int nLength = 0;

	int nNumBytesRead = _ftscanf(m_pResumeFile, _T("%d,"), &nLength);

	if ((nNumBytesRead == 0) || (nNumBytesRead == EOF) || (nLength <= 0)) {
        UnlockResources();
		return false;
	}

    char* szReturn = fgets(szValue, sizeof(szValue), m_pResumeFile);
    if (szReturn == NULL) {
        UnlockResources();
        return false;
    }

    szIn = std::string(szValue);

    #if 0
    // Unfortunately, this nice little trick falls apart if the file name
    // has spaces...sigh...

    // The following creates a formatted string to allow us to specify
    // a variable length in our format string...
    // see comp.lang.c FAQ list � Question 12.15

    char szFormat[10];
    sprintf(szFormat, "%%%ds,", nLength);

	nNumBytesRead = _ftscanf(m_pResumeFile, szFormat, szValue);
	if ((nNumBytesRead == 0) || (nNumBytesRead == EOF) || (nLength <= 0)) {
		return false;
	}

	szIn = szValue;
	if (bConsumeNewLine) {
		char szChar;
		nNumBytesRead = _ftscanf(m_pResumeFile, _T("%c"), &szChar);
		if ((nNumBytesRead == 0) || (nNumBytesRead == EOF)) {
			return false;
		}
	}
	#endif

    UnlockResources();
	return true;

} // End ResumeInfoFileReadString

///////////////////////////////////////////////////////////////////////
bool ResumeManager::ResumeInfoFileReadNumber( unsigned long& ulIn, bool bConsumeNewLine )
{
    if (m_pResumeFile == NULL) {
        return false;
    }

    LockResources(_T("Resume file read number error"));

	int nNumBytesRead;
	nNumBytesRead = _ftscanf(m_pResumeFile, _T("%d,"), &ulIn);
	if ((nNumBytesRead == 0) || (nNumBytesRead == EOF) ) {
        UnlockResources();
		return false;
	}

	if (bConsumeNewLine) {
		char szChar;
		nNumBytesRead = _ftscanf(m_pResumeFile, _T("%c"), &szChar);
	}

    UnlockResources();
	return true;

} // End ResumeInfoFileReadNumber

///////////////////////////////////////////////////////////////////////
int ResumeManager::ResumeInfoFileWriteNumber( unsigned long lIn, bool bAddNewLine )
{
    LockResources(_T("Resume file write number error"));

	std::string szData;
	StringUtil::AppendIntToString(szData, (long)lIn, _T(""));
	int nResult = ResumeInfoFileWriteString(szData, bAddNewLine);

    UnlockResources();
	return nResult;

} // End ResumeInfoFileWriteNumber



/** @} */

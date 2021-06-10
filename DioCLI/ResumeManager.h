/*********************************************************************
 * 
 *  file:  ResumeManager.h
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

#ifndef __RESUME_MANAGER_H__
#define __RESUME_MANAGER_H__

#include "stdafx.h"
#include "ResumeInfoData.h"
#include "ResumeNamedMutex.h"

#include <string>
#include <sstream>
#include <streambuf>
#include <iomanip>
#include <set>

#define RESUME_UPLOAD_FILENAME                          _T("resumeUpload")
#define RESUME_DOWNLOAD_FILENAME                        _T("resumeDownload")

typedef std::vector<ResumeUploadInfoData>               t_resumeUploadInfoList;
typedef std::vector<ResumeDownloadInfoData>             t_resumeDownloadInfoList;

namespace DiomedeResumeErrorCodes {

    typedef enum ResumeErrorCodeTypes {
        RESUME_NO_ERROR = 0,
        RESUME_INVALID_FILEPATH,
        RESUME_INVALID_FILENAME,
        RESUME_OPEN_FILE_ERROR,
        RESUME_FILE_LOCKING_ERROR,
        RESUME_INVALID_RESUME_TYPE,
        RESUME_CREATE_BUFFER_ERROR,
        RESUME_NO_MORE_DATA,
        RESUME_ZERO_FILE_LENGTH,
        RESUME_MD5_CREATE_ERROR,
        RESUME_AMBIGUOUS_FILEID,
        RESUME_INVALID_FILEID,
        RESUME_NO_FILEID,
        RESUME_NO_MATCHES_FOUND,
        RESUME_INDEX_RECORD_ERROR,
        RESUME_INVALID_CONFIG_FORMAT,
        RESUME_SYSTEM_IO_ERROR,
        RESUME_LAST_ERROR_CODE
    } DiomedeResumeErrorCodesType;

    static const std::string DiomedeResumeErrorStrings[RESUME_LAST_ERROR_CODE + 1] =
    {
        _T("No error."),
        _T("File path is invalid."),
        _T("File name is invalid."),
        _T("File cannot be opened."),
        _T("File could not be locked."),
        _T("Resume type is invalid."),
        _T("Could not create file read buffer."),
        _T("No data found."),
        _T("File has zero length."),
        _T("Failed to create MD5 hash."),
        _T("File ID is ambiguous."),
        _T("File ID is invalid."),
        _T("No file ID found."),
        _T("No match found for resume data."),
        _T("Indexing error."),
        _T("Invalid resume file format."),
        _T("System I/O error."),
        _T("")
    };

    static const std::string GetResumeError(int nErrorCode) {
        return DiomedeResumeErrorStrings[nErrorCode];
    }
}

//---------------------------------------------------------------------
//! ResumeInfo Comparison structure
//---------------------------------------------------------------------
struct ResumeInfoCompare
{
    bool operator()(ResumeInfoData fileInfo1, ResumeInfoData fileInfo2) const
    {
        bool bEqualEntries = false;

        std::string szFilePath1 = fileInfo1.GetFilePath();
        std::string szFilePath2 = fileInfo2.GetFilePath();

        ResumeInfoType nResumeInfoType1 = fileInfo1.GetResumeInfoType();
        ResumeInfoType nResumeInfoType2 = fileInfo2.GetResumeInfoType();

	    bool bEqualPath = (0 == stricmp(szFilePath1.c_str(), szFilePath2.c_str()));
	    if (bEqualPath) {
	        bEqualEntries = nResumeInfoType1 == nResumeInfoType2;
	    }
	    return bEqualEntries;
    }
};


//---------------------------------------------------------------------
//! Resume index data
//---------------------------------------------------------------------
typedef struct tagResumeIndexStruct {
	int nPosition;
	int nSize;

    inline bool operator ==( const tagResumeIndexStruct& resumeIndexInfo )
    {
        return (nPosition == resumeIndexInfo.nPosition) && (nSize == resumeIndexInfo.nSize);
    }
} ResumeIndexStruct;

typedef std::vector<ResumeIndexStruct> t_resumeIndexInfoList;

/////////////////////////////////////////////////////////////////////////////
// ResumeManager Class

class ResumeManager
{
protected:
    static ResumeManager*       m_pResumeMgr;

private:
	FILE*                       m_pResumeFile;
    FILE*                       m_pResumeIndexFile;
    std::string                 m_szResumeFileName;             //! + .idx is the index file
                                                                //! + .dat is the data file
    ResumeNamedMutexUtil*       m_pMutexUtil;                   //! When allocated, sets up a
                                                                //! named mutex that is locked.
    int                         m_nLocks;                       //! When count = 0, lock is
                                                                //! released.

	unsigned long	            m_dwEncryptLow;                 //! Encryption seeds
	unsigned long	            m_dwEncryptHigh;
	int                         m_nFileVersion;                 //! File version
	time_t                      m_tResumeTime;

	bool                        m_bIsFirstRun;                  //! Setup for first run of
	                                                            //! resume data.
	bool                        m_bIsFileValid;                 //! Indicates whether the file is
	                                                            //! valid.
    t_resumeUploadInfoList      m_listResumeUploadInfo;
    t_resumeDownloadInfoList    m_listResumeDownloadInfo;

    t_resumeIndexInfoList       m_listResumeIndexInfo;

    char*                       m_szBuffer;
    std::string                 m_szAppDirectory;

    int                         m_nLastError;                   //! Last file I/O error

public:
    static ResumeManager* Instance();
    virtual ~ResumeManager();

	// Cleanup the Resume Mamanager.
	void Shutdown();

private:
    ResumeManager();

    int LockResources(std::string szLoggingErrorMsg=_T(""));
    int UnlockResources();

    int FileDelete( const std::string& szFilePath );
    void GetEpochSeconds(time_t& epochSeconds);
    bool CreateBuffer(int nBufferSize);
    void ClearBuffer();

    void SetupForNewResumeFile();
    int ReadFileHeaderData(std::string szDataFileName, bool& bIsFirstRun);

	//-----------------------------------------------------------------
	//! Index and data file: Read/write the resume index.  Open
	//! the index and resume data files, loading only the index
	//! data into memory.
	//-----------------------------------------------------------------
    int ReadResumeIndex( std::string szResumeFileName);
    int WriteResumeIndex( std::string szResumeFileName);
    int CloseOpenFiles(std::string szResumeFileName=_T(""));

public:
    int ClearResumeMgrData();
    int OpenResumeData( std::string szResumeFileName, bool& bIsFirstRun );

    int ReadResumeUploadData( ResumeUploadInfoData& resumeUploadInfo, bool bAddNewEntries=false );
    int ReadResumeUploadDataByFileID( ResumeUploadInfoData* pResumeUploadInfo, bool bAddNewEntries=false );
    int WriteResumeUploadData( std::string szResumeFileName, ResumeUploadInfoData& resumeUploadInfo );
    int GetResumeUploadRecordCount();

    int ReadResumeDownloadData( std::string szResumeFileName, ResumeDownloadInfoData& resumeDownloadInfo,
                                bool bAddNewEntries=false );
    int WriteResumeDownloadData( std::string szResumeFileName, ResumeDownloadInfoData& resumeDownloadInfo );
    int GetResumeDownloadRecordCount();

    // Clears data of a given type from the files (e.g. originally meant to clear out all
    // "done" entries.
    int ClearResumeFiles(const ResumeInfoType resumeInfoType);

    // Instead of writing out the "done" entries, we'll re-write the file out to
    // clear this entry from the resume files.  This assumes that the resume files
    // are small, and this task will not slow the upload process.
    int ClearResumeInfo(const ResumeInfoType resumeInfoType,
                        std::string szResumeFileName,
                        ResumeUploadInfoData& resumeUploadInfo);

    //-----------------------------------------------------------------
    // Error returned from system I/O calls.
    //-----------------------------------------------------------------
	int GetLastError() {
	    return m_nLastError;
	}

	void SetLastError(int nLastError) {
	    m_nLastError = nLastError;
	}

	void ResetErrorCodes() {
	    m_nLastError = 0;
	}

	//-----------------------------------------------------------------
	//! Memory list access to resume data
	//-----------------------------------------------------------------
private:
	int SaveResumeInfoFile(const ResumeInfoType resumeInfoType );
	int LoadResumeInfo(const ResumeInfoType resumeInfoType );
	bool IsResumeListValid() { return m_bIsFileValid; }

	//-----------------------------------------------------------------
	//! Apply a function to every resume info data in the list
	//-----------------------------------------------------------------
	void ApplyToEachResumeUploadInfo( int (*ResumeListFunction ) (ResumeManager*, ResumeUploadInfoData&),
                                       t_resumeUploadInfoList& resumeInfoList);
	void ApplyToEachResumeDownloadInfo( int (*ResumeListFunction ) (ResumeManager*, ResumeDownloadInfoData&),
                                       t_resumeDownloadInfoList& resumeInfoList);

public:
	//-----------------------------------------------------------------
	//! \name Loading and saving of the resume info files.
	//! \param resumeInfoType: type of resume data loaded or saved.
	//! @{
    //-----------------------------------------------------------------
	int Load( const ResumeInfoType resumeInfoType  );
	bool Save( const ResumeInfoType resumeInfoType  );

private:
    // Methods for saving the resume data.
	static int SaveResumeUploadState(ResumeManager* pResumeMgr, ResumeUploadInfoData& resumeDataInfo);
	static int SaveResumeDownloadState(ResumeManager* pResumeMgr, ResumeDownloadInfoData& resumeDataInfo);

	FILE* GetResumeInfoFile() {
	    return m_pResumeFile;
	}

	t_resumeIndexInfoList* GetIndexInfoList() {
	    return &m_listResumeIndexInfo;
	}

	unsigned long GetEncryptLow() { return m_dwEncryptLow; }
	unsigned long GetEncryptHigh() { return m_dwEncryptHigh; }
	int GetFileVersion() { return m_nFileVersion; }

    //-----------------------------------------------------------------
	//! @}
    //-----------------------------------------------------------------

	//--------------------------------------------------------------------
	//! Returns true is this is the first run of the application,
	//! else returns false
	//--------------------------------------------------------------------
	bool IsFirstRun() { return m_bIsFirstRun; }

public:
    //-----------------------------------------------------------------
    //! File access to resume data
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    //! Memory access to resume data
    //-----------------------------------------------------------------

	t_resumeUploadInfoList& GetResumeUploadInfoList();
	t_resumeUploadInfoList& GetResumeUploadInfoList(const std::vector<std::string>& listFiles,
                                                    bool bAddNewEntries=false );

	t_resumeDownloadInfoList& GetResumeDownloadInfoList();
	t_resumeDownloadInfoList& GetResumeDownloadInfoList(const std::vector<std::string>& listFiles,
                                                        bool bAddNewEntries=false );

	//--------------------------------------------------------------------
	//! Set/Get the data directory - current a folder off the current
	//! working folder.
	//--------------------------------------------------------------------
	inline void	SetAppDataDir( const std::string& szNewAppDir ) {
		GetAppDataDir() = szNewAppDir;
	}
	std::string& GetAppDataDir();

    time_t* ResumeTime();
	time_t& GetLastResumeTime();
	void SetLastResumeTime(time_t& timeNow);

	void SetCurrentVersion(int major, int minor, int build);

	//--------------------------------------------------------------------
	//! ResumeInfoFile support
	//--------------------------------------------------------------------
	int ResumeInfoFileWriteString( std::string szInStr, bool bAddNewLine = false );
	bool ResumeInfoFileReadString( std::string &szIn, bool bConsumeNewLine = false );
	bool ResumeInfoFileReadNumber( unsigned long& ulIn, bool bConsumeNewLine = false );
	int ResumeInfoFileWriteNumber( unsigned long ulIn, bool bAddNewLine = false );

};

#endif // __RESUME_MANAGER_H__

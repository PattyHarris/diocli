/*********************************************************************
 * 
 *  file:  ResumeInfoData.cpp
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

//! \defgroup resume_info Resume Info Methods
//! \ingroup consolecontrol
//! @{

#include "ResumeInfoData.h"
#include "stdafx.h"
#include "types.h"

#include "../Util/Util.h"
#include "../Util/ClientLog.h"
#include "../Include/ErrorCodes/UIErrors.h"

#include "UserProfileData.h"
#include "ProfileManager.h"

#if defined(linux)
#include <unistd.h>
#endif

#if defined(linux) || defined( __APPLE__)
#include <stdio.h>
#include <stdlib.h>
#endif

#include "boost/date_time/posix_time/posix_time.hpp"
#include <iostream>

using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace StringUtil;

///////////////////////////////////////////////////////////////////////
//! \brief Resume info assignment operator
//! \param srcResumeInfo: source object
//
void ResumeInfoData::operator =( const ResumeInfoData &srcResumeInfo )
{
	m_nResumeInfoType   = srcResumeInfo.m_nResumeInfoType;
	m_szFilePath        = srcResumeInfo.m_szFilePath;
	m_nIntervalType     = srcResumeInfo.m_nIntervalType;
	m_tmFirstStart      = srcResumeInfo.m_tmFirstStart;
	m_tmLastStart       = srcResumeInfo.m_tmLastStart;
	m_nResumeFileIndex  = srcResumeInfo.m_nResumeFileIndex;

} // End assignment operator

///////////////////////////////////////////////////////////////////////
//! \brief Resume info comparison operator
//! \param srcResumeInfo: source object
//
bool ResumeInfoData::operator ==( const ResumeInfoData &srcResumeInfo )
{
    if ( (m_nResumeInfoType == srcResumeInfo.m_nResumeInfoType) &&
         (m_szFilePath == srcResumeInfo.m_szFilePath) ) {
		return true;
    }
	else {
		return false;
	}
} // End comparison operator

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//! \brief Constructor from serialized data - default constructor.
//
ResumeUploadInfoData::ResumeUploadInfoData() : ResumeInfoData(resumeUploads),
                                               m_l64FileID(0), m_l64FileSize(-1),
                                               m_bAddMetaData(false),
                                               m_bCreateMD5Digest(false),
                                               m_tmLastModifiedTime(0),
                                               m_l64BytesRead(0)
{

} // End constructor

///////////////////////////////////////////////////////////////////////
//! \brief Constructor from serialized data
//! \param szSerilizedData: searialized resume upload data
//
ResumeUploadInfoData::ResumeUploadInfoData(const std::string szSerilizedData)
                                           : ResumeInfoData(resumeUploads)
{
	Deserialize(szSerilizedData);

} // End constructor

///////////////////////////////////////////////////////////////////////
//! \brief Resume upload info assignment operator
//! \param srcResumeInfo: source object
//
void ResumeUploadInfoData::operator =( const ResumeUploadInfoData &srcResumeInfo )
{
    ResumeInfoData::operator =(srcResumeInfo);
	m_l64FileID	            = srcResumeInfo.m_l64FileID;
	m_l64FileSize			= srcResumeInfo.m_l64FileSize;
	m_bAddMetaData          = srcResumeInfo.m_bAddMetaData;
	m_bCreateMD5Digest      = srcResumeInfo.m_bCreateMD5Digest;
	m_tmLastModifiedTime	= srcResumeInfo.m_tmLastModifiedTime;
	m_l64BytesRead			= srcResumeInfo.m_l64BytesRead;

} // End assignment operator

///////////////////////////////////////////////////////////////////////
//! \brief Resume upload info comparison operator
//! \param srcResumeInfo: source object
//
bool ResumeUploadInfoData::operator ==( const ResumeUploadInfoData &srcResumeInfo )
{
    bool bIsEqual = ResumeInfoData::operator ==(srcResumeInfo);
    if (bIsEqual == false) {
        return false;
    }

    if ( ( m_l64FileID == srcResumeInfo.m_l64FileID ) &&
         ( m_l64FileSize == srcResumeInfo.m_l64FileSize ) &&
         ( m_tmLastModifiedTime == srcResumeInfo.m_tmLastModifiedTime ) ) {
		return true;
    }
	else {
		return false;
	}

} // End comparison operator

///////////////////////////////////////////////////////////////////////
// Protected Methods

///////////////////////////////////////////////////////////////////////
// \brief Conversion of resume uploads into a serialized/deserialized
//!       format
//! \param listResumeData: vector of serialized data
//! \param nIndexToStart: where to start in the vector
//! \return true if successful, false otherwise
bool ResumeUploadInfoData::Deserialize( std::vector<std::string> listResumeData,
                                        unsigned int nStartIndex )
{
	unsigned int nIndex = nStartIndex;
	m_nResumeInfoType   = (ResumeInfoType)atoi(listResumeData[nIndex++].c_str());
	m_l64FileID	        = atoi64(listResumeData[nIndex++].c_str());
	m_szFilePath		= listResumeData[nIndex++];

	m_l64FileSize		= atoi64(listResumeData[nIndex++].c_str());
	m_bAddMetaData      = atoi(listResumeData[nIndex++].c_str()) == 1;
	m_bCreateMD5Digest  = atoi(listResumeData[nIndex++].c_str()) == 1;

	m_tmLastModifiedTime= atol(listResumeData[nIndex++].c_str());
	m_l64BytesRead		= atoi64(listResumeData[nIndex++].c_str());

	// The pad can be ignored - it's there to allow room for the
	// file to be read completely.
	std::string szPad   = listResumeData[nIndex++];

    m_nIntervalType     = (ResumeInfoIntervalType)atoi(listResumeData[nIndex++].c_str());

	m_tmFirstStart		= atol(listResumeData[nIndex++].c_str());
	m_tmLastStart		= atol(listResumeData[nIndex++].c_str());

	return true;

} // End Deserialize

///////////////////////////////////////////////////////////////////////
// Public Methods

///////////////////////////////////////////////////////////////////////
// \brief Conversion of resume uploads into a serialized/deserialized
//!       format
//! \param bEncrypt: true if data is encrypted, false otherwise
//! \param i1: encryption seed
//! \param i2: encryption seed
//!
//! \return serialized data if successful, empty string otherwise.
//
std::string ResumeUploadInfoData::Serialize(bool bEncrypt, unsigned long i1, unsigned long i2)
{
	std::string szOutput = _T("");
	std::string szPadStr = _T("");

	szOutput  =  VERSION_ID	+ DATUM_SEP;
	StringUtil::AppendIntToString(szOutput, (unsigned int)bEncrypt, *DATUM_SEP.c_str());

	StringUtil::AppendIntToString(szOutput, (unsigned int)m_nResumeInfoType, *DATUM_SEP.c_str());

	if (m_l64FileID > 0) {
	    StringUtil::AppendIntToString(szOutput, m_l64FileID, *DATUM_SEP.c_str());
	}
	else {
    	szPadStr = StringUtil::GetPadStr(sizeof(LONG64), RESUME_PAD);
    	szOutput.append(szPadStr + *DATUM_SEP.c_str());
	}

	szOutput +=  m_szFilePath + DATUM_SEP;

	int nFileSizeLen = StringUtil::AppendIntToString(szOutput, m_l64FileSize, *DATUM_SEP.c_str());
	StringUtil::AppendIntToString(szOutput, (unsigned int)m_bAddMetaData, *DATUM_SEP.c_str());
	StringUtil::AppendIntToString(szOutput, (unsigned int)m_bCreateMD5Digest, *DATUM_SEP.c_str());

	StringUtil::AppendIntToString(szOutput, (long)m_tmLastModifiedTime, *DATUM_SEP.c_str());

	int nReadSizeLen = StringUtil::AppendIntToString(szOutput, m_l64BytesRead, *DATUM_SEP.c_str());

	// Pad the "read" length so that the buffer has enough room once the file is uploaded
	// completely.
	szPadStr = StringUtil::GetPadStr(nFileSizeLen - nReadSizeLen, RESUME_PAD);
	szOutput.append(szPadStr + *DATUM_SEP.c_str());

	StringUtil::AppendIntToString(szOutput, (unsigned int)m_nIntervalType, *DATUM_SEP.c_str());

	StringUtil::AppendIntToString(szOutput, (long)m_tmFirstStart, *DATUM_SEP.c_str());
	StringUtil::AppendIntToString(szOutput, (long)m_tmLastStart, *DATUM_SEP.c_str());

	if (bEncrypt) {
		szOutput = StringUtil::EncryptString(i1, i2, szOutput);
	}

	return szOutput;

} // End Serialize

///////////////////////////////////////////////////////////////////////
// \brief Conversion of resume uploads into a serialized/deserialized
//!       format
//! \param: szSerializedData: serialized data
//! \param bEncrypt: true if data is encrypted, false otherwise
//! \param i1: encryption seed
//! \param i2: encryption seed
//!
//! \return true if successful, false otherwise.
//
bool ResumeUploadInfoData::Deserialize(const std::string szSerializedData, bool bEncrypted,
                             unsigned long i1, unsigned long i2)
{
	std::string szRawData;

	if (bEncrypted) {
		szRawData = StringUtil::DecryptString(i1, i2, szSerializedData);
	}
	else {
		szRawData = szSerializedData;
	}

	const unsigned int VALID_NUMBER_RESUME_UPLOAD_ITEMS = 15;
	bool bSuccess = true;

	if ( (size_t)-1 == szRawData.find(DATUM_SEP)) {
		// The string does not contain any separators - error has occurred
		return false;
	}

	std::vector<std::string> listItems;

	int nCount = StringUtil::SplitString(szRawData, DATUM_SEP, listItems, true);
	int nVersionID = atoi(listItems[1].c_str());
	bool bIsEncrypted = false;

	switch (nVersionID) {
		case 0:
		case 1:
			ClientLog(WHEREIAM, UI_COMP, ER, false,
			    _T("Invalid version of resume serialized data %d"), nVersionID);
			bSuccess = false;
			break;
		case 2:
			if ( nCount < (int)VALID_NUMBER_RESUME_UPLOAD_ITEMS ) {
				ClientLog(WHEREIAM, UI_COMP, ER, false,
				    _T("Incorrect number of elements in resume serialized data %s"),
				    CS(szRawData));
				bSuccess = false;
				break;
			}
			else {
                bIsEncrypted = (1 == atoi(listItems[2].c_str()));
			}
			// Here, the start index is 3 - this was better behaved when
			// the data was read in with fscanf - which didn't work since
			// it stops at white spaces..
			bSuccess = Deserialize(listItems, 3);
			break;
		default:
			ClientLog(WHEREIAM, UI_COMP, ER, false,
			    _T("Invalid version of resume serialized data %d"), nVersionID);
			bSuccess = false;
	}

	return bSuccess;

} // End Deserialize

///////////////////////////////////////////////////////////////////////
// \brief Has the file changed since the last time?
//! \return Returns true if the last modified dates are different,
//!         false otherwise.
bool ResumeUploadInfoData::HasResumeFileChanged()
{
    time_t tmLastModified;
    int nResult = Util::GetFileLastModifiedTime(m_szFilePath.c_str(), tmLastModified);
    if (tmLastModified != m_tmLastModifiedTime) {
        return true;
    }

    LONG64 l64FileSize = Util::GetFileLength64(m_szFilePath.c_str());
    if (l64FileSize != m_l64FileSize) {
        return true;
    }

	return false;

} // End Deserialize

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// ResumeDownloadInfoData

///////////////////////////////////////////////////////////////////////
//! \brief Constructor from serialized data
//! \param szSerilizedData: searialized resume download data
//
ResumeDownloadInfoData::ResumeDownloadInfoData(const std::string szSerilizedData)
                     : ResumeInfoData(resumeDownloads)
{
	Deserialize(szSerilizedData);

} // End constructor

///////////////////////////////////////////////////////////////////////
//! \brief Resume upload info assignment operator
//! \param srcResumeInfo: source object
//
void ResumeDownloadInfoData::operator =( const ResumeDownloadInfoData &srcResumeInfo )
{
    ResumeInfoData::operator    =(srcResumeInfo);
	m_l64FileID	        = srcResumeInfo.m_l64FileID;
	m_l64BytesReceived  = srcResumeInfo.m_l64BytesReceived;

} // End assignment operator

///////////////////////////////////////////////////////////////////////
//! \brief Resume download info comparison operator
//! \param srcResumeInfo: source object
//
bool ResumeDownloadInfoData::operator ==( const ResumeDownloadInfoData &srcResumeInfo )
{
    bool bIsEqual = ResumeInfoData::operator ==(srcResumeInfo);
    if (bIsEqual == false) {
        return false;
    }

    if ( m_l64FileID == srcResumeInfo.m_l64FileID ) {
		return true;
    }
	else {
		return false;
	}

} // End comparison operator

///////////////////////////////////////////////////////////////////////
// Protected Methods

///////////////////////////////////////////////////////////////////////
// \brief Conversion of resume downalods into a serialized/deserialized
//!       format
//! \param listResumeData: vector of serialized data
//! \param nIndexToStart: where to start in the vector
//! \return true if successful, false otherwise
bool ResumeDownloadInfoData::Deserialize( std::vector<std::string> listResumeData,
                                        unsigned int nStartIndex )
{
	unsigned int nIndex = nStartIndex;
	m_nResumeInfoType   = (ResumeInfoType)atoi(listResumeData[nIndex++].c_str());
	m_l64FileID	        = atoi64(listResumeData[nIndex++].c_str());
	m_szFilePath		= listResumeData[nIndex++];
	m_l64BytesReceived	= atoi64(listResumeData[nIndex++].c_str());

	// The pad can be ignored - it's there to allow room for the
	// file to be downloaded completely.
	std::string szPad   = listResumeData[nIndex++];

    m_nIntervalType     = (ResumeInfoIntervalType)atoi(listResumeData[nIndex++].c_str());

	m_tmFirstStart		= atol(listResumeData[nIndex++].c_str());
	m_tmLastStart		= atol(listResumeData[nIndex++].c_str());

	return true;

} // End Deserialize

///////////////////////////////////////////////////////////////////////
// Public Methods

///////////////////////////////////////////////////////////////////////
// \brief Conversion of resume downloads into a serialized/deserialized
//!       format
//! \param bEncrypt: true if data is encrypted, false otherwise
//! \param i1: encryption seed
//! \param i2: encryption seed
//!
//! \return serialized data if successful, empty string otherwise.
//
std::string ResumeDownloadInfoData::Serialize(bool bEncrypt, unsigned long i1, unsigned long i2)
{
	std::string szOutput;
	szOutput  =  VERSION_ID	+ DATUM_SEP;
	StringUtil::AppendIntToString(szOutput, (unsigned int)bEncrypt, *DATUM_SEP.c_str());
	StringUtil::AppendIntToString(szOutput, (unsigned int)m_nResumeInfoType, *DATUM_SEP.c_str());

	StringUtil::AppendIntToString(szOutput, m_l64FileID, *DATUM_SEP.c_str());

	szOutput +=  m_szFilePath + DATUM_SEP;

	int nReceivedSize = StringUtil::AppendIntToString(szOutput, m_l64BytesReceived, *DATUM_SEP.c_str());

	// Pad the "read" length so that the buffer has enough room once the file is downloaded
	// completely.
	std::string szPadStr = StringUtil::GetPadStr(65 - nReceivedSize, RESUME_PAD);
	szOutput.append(szPadStr + *DATUM_SEP.c_str());

	StringUtil::AppendIntToString(szOutput, (unsigned int)m_nIntervalType, *DATUM_SEP.c_str());

	StringUtil::AppendIntToString(szOutput, (long)m_tmFirstStart, *DATUM_SEP.c_str());
	StringUtil::AppendIntToString(szOutput, (long)m_tmLastStart, *DATUM_SEP.c_str());

	if (bEncrypt) {
		szOutput = StringUtil::EncryptString(i1, i2, szOutput);
	}

	return szOutput;

} // End Serialize

///////////////////////////////////////////////////////////////////////
// \brief Conversion of resume downloads into a serialized/deserialized
//!       format
//! \param: szSerializedData: serialized data
//! \param bEncrypt: true if data is encrypted, false otherwise
//! \param i1: encryption seed
//! \param i2: encryption seed
//!
//! \return true if successful, false otherwise.
//
bool ResumeDownloadInfoData::Deserialize(const std::string szSerializedData, bool bEncrypted,
                             unsigned long i1, unsigned long i2)
{
	std::string szRawData;

	if (bEncrypted) {
		szRawData = StringUtil::DecryptString(i1, i2, szSerializedData);
	}
	else {
		szRawData = szSerializedData;
	}

	const unsigned int VALID_NUMBER_RESUME_DOWNLOAD_ITEMS = 9;
	bool bSuccess = true;

	if ( (size_t)-1 == szRawData.find(DATUM_SEP)) {
		// The string does not contain any separators - error has occurred
		return false;
	}

	std::vector<std::string> listItems;

	int nCount = StringUtil::SplitString(szRawData, DATUM_SEP, listItems, true);
	int nVersionID = atoi(listItems[1].c_str());
	bool bIsEncrypted = false;

	switch (nVersionID) {
		case 0:
		case 1:
			ClientLog(WHEREIAM, UI_COMP, ER, false,
			    _T("Invalid version of resume serialized data %d"), nVersionID);
			bSuccess = false;
			break;
		case 2:
			if ( nCount < (int)VALID_NUMBER_RESUME_DOWNLOAD_ITEMS ) {
				ClientLog(WHEREIAM, UI_COMP, ER, false,
				    _T("Incorrect number of elements in resume serialized data %s"),
				    CS(szRawData));
				bSuccess = false;
				break;
			}
			else {
                bIsEncrypted = (1 == atoi(listItems[2].c_str()));
			}
			// Here, the start index is 3
			bSuccess = Deserialize(listItems, 3);
			break;
		default:
			ClientLog(WHEREIAM, UI_COMP, ER, false,
			    _T("Invalid version of resume serialized data %d"), nVersionID);
			bSuccess = false;
	}

	return bSuccess;

} // End Deserialize


/** @} */


///////////////////////////////////////////////////////////////////////
//
// CEnum is a fast and simple class that lets you enumerate (i.e. make a list of) all
// files in a directory.
// It supports:
//      - the use of wild card characters * and ? (i.e. its a file globbing class too)
//		- separate enumeration of files and subdirectories
//		- separate include list, exclude list and ignore case option for both files
//        and directories
//		- recursive search (i.e. it can enumerate contest of subdirectories too)
//		- enumeration either using file's full path or just file name
//		- written in STL, but it optionally supports MFC collection classes
//		- UNICODE aware (in MS world UNICODE stands for UTF-16 little-endian)
//		- works as a wrapper around ::FindFirstFile and ::FindNextFile
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
//                                              ---LICENSE---
//
// Developed by loreia (loreia (at) net.hr) and published on CodeProject.com
//
// There is no limitation on use of this class.
// You may freely use, redistribute and modify this code (as whole or just parts of it)
// with or without author's premission in any sort of project regardless of
// license issues, in both commercial and open source projects.
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// Errata:
// 1. fatal error C1010: unexpected end of file while looking for precompiled header.
//	  Did you forget to add '#include "stdafx.h"' to your source?
// - Solution:
//		a) In the Solution Explorer pane of the project, right-click the project name, and
//         then click Properties.
//		b) In the left pane, click the C/C++ folder.
//		c) Click the Precompiled Headers node.
//		d) In the right pane, click Create/Use Precompiled Header, and then click Not
//         Using Precompiled Headers.
//
///////////////////////////////////////////////////////////////////////

#ifndef __ENUM_H__
#define __ENUM_H__

//#define UNICODE
//#define _UNICODE

#include <string>
#include <list>

#if defined(linux)
#include <unistd.h>
#endif

#ifndef _WIN32
    #include <sys/types.h>
    #include <sys/stat.h>
	#include <dirent.h>
	#include <errno.h>
	#include <vector>
	#include <string>
	#include <iostream>
#endif

#ifdef _WIN32
	#include <windows.h>
	#include "tchar.h"
	#include "stdio.h"
#endif

#include "../Util/XString.h"
#include "../Include/types.h"

using namespace std;

typedef basic_string<TCHAR> _stl_string;    // basic_string<TCHAR> will compile into std::string
                                            // or std::wstring, based on if _UNICODE was defined

// Diomede: callback function added to relay status back to the UI.
typedef bool (*EnumStatusFunc)(void*, int nStatus);

///////////////////////////////////////////////////////////////////////
class CEnum
{
public:
	_stl_string         m_szExcPatternDirs;			// Exclude pattern has precedence over 
	                                                // Include pattern
	_stl_string         m_szExcPatternFiles;
	_stl_string         m_szIncPatternDirs;
	_stl_string         m_szIncPatternFiles;

	bool                m_bRecursive;
	bool                m_bFullPath;
	bool                m_bNoCaseDirs;
	bool                m_bNoCaseFiles;
	
	EnumStatusFunc      m_pfnStatusFunc;
	void*               m_pUserData;
	int                 m_nFileCount;
	bool                m_bCancelled;

private:
	list<_stl_string > * m_pListDirs;			    // notice the space in front of the right
	                                                // angle bracket !!!
	list<_stl_string > * m_pListFiles;

	list<_stl_string > * m_pListExcPatternDirs;
	list<_stl_string > * m_pListExcPatternFiles;
	list<_stl_string > * m_pListIncPatternDirs;
	list<_stl_string > * m_pListIncPatternFiles;
	

public:
	CEnum()
	{
		m_pListDirs			= new list<_stl_string >;
		m_pListFiles		= new list<_stl_string >;

		m_szExcPatternDirs	= _T("");
		m_szExcPatternFiles	= _T("");
		m_szIncPatternDirs	= _T("");
		m_szIncPatternFiles	= _T("");

		m_bRecursive		= false;
		m_bFullPath			= false;
		m_bNoCaseDirs		= false;
		m_bNoCaseFiles		= false;
		
		m_pfnStatusFunc     = NULL;                 // Callback and user data for interacting
		m_pUserData         = NULL;                 // with the UI.
		m_nFileCount        = 0;
		m_bCancelled        = false;
	}

	CEnum
		(
			_stl_string szPath,
			_stl_string szExcludePatternDirs	= _T(""),
			_stl_string szExcludePatternFiles	= _T(""),
			_stl_string szIncludePatternDirs	= _T(""),
			_stl_string szIncludePatternFiles	= _T(""),
			bool bRecursiveSearch				= false,
			bool bUseFullPath					= false,
			bool bIgnoreCaseDirs				= false,
			bool bIgnoreCaseFiles				= false
		)
	{
		m_pListDirs			= new list<_stl_string >;
		m_pListFiles		= new list<_stl_string >;

		m_szExcPatternDirs	= szExcludePatternDirs;
		m_szExcPatternFiles	= szExcludePatternFiles;
		m_szIncPatternDirs	= szIncludePatternDirs;
		m_szIncPatternFiles	= szIncludePatternFiles;

		m_bRecursive		= bRecursiveSearch;
		m_bFullPath			= bUseFullPath;
		m_bNoCaseDirs		= bIgnoreCaseDirs;
		m_bNoCaseFiles		= bIgnoreCaseFiles;
		
		m_pfnStatusFunc     = NULL;                 // Callback and user data for interacting
		m_pUserData         = NULL;                 // with the UI.
		m_nFileCount        = 0;
		m_bCancelled        = false;

		EnumerateAll(szPath);
	}

	~CEnum()
	{
		delete m_pListDirs;
		delete m_pListFiles;
		m_pListDirs = NULL;
		m_pListFiles = NULL;
	}

	static bool CompareStrings(LPCTSTR szPattern, LPCTSTR szFileName, bool bNoCase)
	{
		bool bStar = false;
		TCHAR temp1[2] = _T("");
		TCHAR temp2[2] = _T("");

		while(*szFileName != 0)
		{
			switch (*szPattern)
			{
				case '?':
					++szFileName; ++szPattern;
					continue;
				case '*':
					++szPattern;
					if (!*szPattern) return true;
					bStar = true;
					continue;
				default:
					if(bNoCase)
					{
						// _tcsicmp works with strings not chars !!
						*temp1 = *szFileName;
						*temp2 = *szPattern;
						#ifdef WIN32
						if (_tcsicmp(temp1, temp2))
						{
							if (!bStar) return false;
							++szFileName;
							continue;
						}
						#else
						if (stricmp(temp1, temp2))
						{
							if (!bStar) return false;
							++szFileName;
							continue;
						}
						#endif
					}
					else if (*szFileName != *szPattern)	// bNoCase == false, compare chars directly
					{
						if (!bStar) return false;
						++szFileName;
						continue;
					}

					// chars are equal, set up the flag and move to the next character
					++szFileName; ++szPattern;
					bStar = false;
					continue;
			}
		}
		// check if there is anything left after last '*'
		while (*szPattern == '*') ++szPattern;
		return (!*szPattern);
	}

private:

	bool CompareList(list<_stl_string > * plsPattern, _stl_string & szFileName, bool bNoCase)
	{
		// Return "true" if at least one search pattern matches file name

		list<_stl_string >::iterator iter = plsPattern->begin();

		for(; iter != plsPattern->end(); ++iter)
		{
			if(CompareStrings(iter->c_str(), szFileName.c_str(), bNoCase)) return true;
		}

		return false;
	}

#ifdef _WIN32
	void Enumerate(_stl_string & sPath)
	{
	    // Check whether the user has cancelled - since this function is
	    // recursive, we'll check here at the top.
	    if (m_bCancelled) {
	        return;
	    }
	    
		// After enumerating files in each directory,
		// STL list's sort() method is called to sort list in alphabetic order.
		// This may cause file names to be sorted in different order than in windows explorer

		WIN32_FIND_DATA			FindFileData;
		HANDLE					hFind			= INVALID_HANDLE_VALUE;
		_stl_string				szFileName		= _T("");
		_stl_string				sSubDir			= _T("");
		list<_stl_string > *	pListDirs		= new list<_stl_string >;
		list<_stl_string > *	pListFiles		= new list<_stl_string >;

		sPath += _T("\\");

		// find the first file in the directory.
		// PH: replaced FindFirstFile with FindFirstFileEx to handle
		// netowrk shares.
		// hFind = FindFirstFile((sPath + _T("*")).c_str(), &FindFileData);
		hFind = FindFirstFileEx((sPath + _T("*")).c_str(), FindExInfoStandard, &FindFileData, 
		                FindExSearchNameMatch, NULL, 0);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			szFileName = FindFileData.cFileName;
			if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				EnumerateFile(sPath, szFileName, pListFiles);
			}
			else if (szFileName.compare(_T(".")) && szFileName.compare(_T("..")))
			{
				EnumerateDir(sPath, szFileName, pListDirs);
			}

			// find all the other files in this directory.
			while (FindNextFile(hFind, &FindFileData) != 0)
			{
				szFileName = FindFileData.cFileName;
				if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					EnumerateFile(sPath, szFileName, pListFiles);
				}
				else if (szFileName.compare(_T(".")) && szFileName.compare(_T("..")))
				{
					EnumerateDir(sPath, szFileName, pListDirs);
				}
								
				if (m_bCancelled)
				{
				    break;
				}
			}
		}

		if (m_bCancelled) {
		
		    if (hFind != INVALID_HANDLE_VALUE) {
		        FindClose(hFind);
		    }

		    delete pListDirs;
		    delete pListFiles;
		    pListDirs = NULL;
		    pListFiles = NULL;
		    return;
		}

		// sort in alphabetic order
		pListDirs->sort();
		pListFiles->sort();

		// copy to global list
		m_pListDirs->insert(m_pListDirs->end(), pListDirs->begin(), pListDirs->end());
		m_pListFiles->insert(m_pListFiles->end(), pListFiles->begin(), pListFiles->end());
		
		if (m_bRecursive)
		{
			list<_stl_string >::iterator iter = pListDirs->begin();
			for (;iter != pListDirs->end(); ++iter)
			{
				if (m_bFullPath)
				{
					Enumerate(*iter);
				}
				else
				{
					sSubDir = sPath + *iter;
					Enumerate(sSubDir);
				}

				if (m_bCancelled)
				{
				    break;
				}
			}
		}

		FindClose(hFind);

		delete pListDirs;
		delete pListFiles;
		pListDirs = NULL;
		pListFiles = NULL;
	}
#else
    //-----------------------------------------------------------------
    // Non-windows version - will need to be tested once we move
    // to non-windows platforms.
    //-----------------------------------------------------------------
	void Enumerate(_stl_string & sPath)
	{
	    // Check whether the user has cancelled - since this function is
	    // recursive, we'll check here at the top.
	    if (m_bCancelled) {
	        return;
	    }

		// After enumerating files in each directory,
		// STL list's sort() method is called to sort list in alphabetic order.
		// This may cause file names to be sorted in different order than in windows explorer

        DIR*                    pOpenDir        = NULL;
        dirent*                 pFileData       = NULL;

		_stl_string				szFileName		= _T("");
		_stl_string				sSubDir			= _T("");
		list<_stl_string > *	pListDirs		= new list<_stl_string >;
		list<_stl_string > *	pListFiles		= new list<_stl_string >;

        pOpenDir = opendir(sPath.c_str());
        if (pOpenDir == NULL) {
            return;
        }

        if (sPath.find_last_of(_T("/")) != sPath.length() - 1) {
		    sPath += _T("/");
        }

		// Loop through the files in the path.
		while ((pFileData = readdir(pOpenDir)) != NULL)
        {
			szFileName = pFileData->d_name;
			bool bIsDirectory = false;
            bool bIsSymbolicLink = false;

			// This could have used the struct dirent d_type field, but
			// since it's not portable, cygwin doesn't provide it.
            std::string szFilePath = _T("");
    	    szFilePath = _format(_T("%s/%s"), sPath.c_str(), pFileData->d_name);

		    struct stat theStats;
		    if (lstat(szFilePath.c_str(), &theStats) != 0) {
                return;
            }
            
            bIsDirectory = ((theStats.st_mode & S_IFDIR) != 0);
            bIsSymbolicLink = ((theStats.st_mode & S_IFLNK) != 0);

			if ( !bIsDirectory && !bIsSymbolicLink)
			{
				EnumerateFile(sPath, szFileName, pListFiles);

				if (m_pfnStatusFunc != NULL)
				{
					m_nFileCount++;
					m_pfnStatusFunc(m_pUserData, m_nFileCount);
				}

			}
			else if ( bIsDirectory && szFileName.compare(_T(".")) && szFileName.compare(_T("..")) )
			{
				EnumerateDir(sPath, szFileName, pListDirs);
			}
			
			if (m_bCancelled)
			{
			    break;
			}
		}

		if (m_bCancelled) {
		
		    if (pOpenDir != NULL) {
		        closedir(pOpenDir);
		    }

		    delete pListDirs;
		    delete pListFiles;
		    pListDirs = NULL;
		    pListFiles = NULL;
		    return;
		}

		// sort in alphabetic order
		pListDirs->sort();
		pListFiles->sort();

		// copy to global list
		m_pListDirs->insert(m_pListDirs->end(), pListDirs->begin(), pListDirs->end());
		m_pListFiles->insert(m_pListFiles->end(), pListFiles->begin(), pListFiles->end());

		if (m_bRecursive)
		{
			list<_stl_string >::iterator iter = pListDirs->begin();
			for (;iter != pListDirs->end(); ++iter)
			{
				if (m_bFullPath)
				{
					Enumerate(*iter);
				}
				else
				{
					sSubDir = sPath + *iter;
					Enumerate(sSubDir);
				}

				if (m_bCancelled)
				{
				    break;
				}
			}
		}

		closedir(pOpenDir);

		delete pListDirs;
		delete pListFiles;
		pListDirs = NULL;
		pListFiles = NULL;
	}
#endif

	void EnumerateFile(_stl_string & sPath, _stl_string & szFileName, list<_stl_string > * pList)
	{
	    // Enumerate file if (both patterns are empty) or (if exclude isn't found and
	    // include pattern is found)
		if 	(
				(m_szExcPatternFiles.empty() || !CompareList(m_pListExcPatternFiles, szFileName, m_bNoCaseFiles))
				 &&
				(m_szIncPatternFiles.empty() || CompareList(m_pListIncPatternFiles, szFileName, m_bNoCaseFiles))
			)
		{
			if (m_bFullPath) pList->push_back(sPath + szFileName);
			else pList->push_back(szFileName);

			if (m_pfnStatusFunc != NULL)
			{
			    m_nFileCount++;
			    m_pfnStatusFunc(m_pUserData, m_nFileCount);
			}
		}
	}

	void EnumerateDir(_stl_string & sPath, _stl_string & sDirName, list<_stl_string > * pList)
	{
	    // Enumerate directory if (both patterns are empty) or (if exclude isn't found
	    // and include pattern is found)
		if (
				(m_szExcPatternDirs.empty() || !CompareList(m_pListExcPatternDirs, sDirName, m_bNoCaseDirs))
				 &&
				(m_szIncPatternDirs.empty() || CompareList(m_pListIncPatternDirs, sDirName, m_bNoCaseDirs))
			)
		{
			if(m_bFullPath) pList->push_back(sPath + sDirName);
			else pList->push_back(sDirName);
		}
	}

	list<_stl_string > * Tokenize(_stl_string & sPattern)
	{
		// search strings are tokenized by ';' character

		string::size_type position = 0;
		string::size_type length = 0;

		list<_stl_string > * pListTokenized = new list<_stl_string >;

		while (length != string::npos)
		{
			position = sPattern.find_first_not_of(';', length);
			length = sPattern.find_first_of(';', length + 1);

			pListTokenized->push_back(sPattern.substr(position, length - position));
		}

		return pListTokenized;
	}

public:

	// one and only public method
	void EnumerateAll(_stl_string sPath)
	{
		// in case we are reusing the same object
		if(m_pListDirs) m_pListDirs->clear();
		if(m_pListFiles) m_pListFiles->clear();

		// basic error checking
		if (sPath.empty()) return;
		#ifdef WIN32
		    // remove last backslash
		    if (sPath[sPath.size() - 1] == '\\') {
		        sPath.erase(sPath.size() - 1);
		    }
		#else
		    if (sPath[sPath.size() - 1] == '/') {
		        sPath.erase(sPath.size() - 1);
		    }
		#endif

		// sets the locale to the user-default ANSI code page obtained from the operating system
		// required by _tcsicmp function
		if(m_bNoCaseDirs || m_bNoCaseFiles)
		{
			#ifdef WIN32
				if( _tsetlocale(LC_ALL, _T("")) == NULL) return;
			#endif
		}

		// tokenize all search patterns
		if(!m_szExcPatternDirs.empty())	m_pListExcPatternDirs	= Tokenize(m_szExcPatternDirs);
		if(!m_szExcPatternFiles.empty())m_pListExcPatternFiles	= Tokenize(m_szExcPatternFiles);
		if(!m_szIncPatternDirs.empty())	m_pListIncPatternDirs	= Tokenize(m_szIncPatternDirs);
		if(!m_szIncPatternFiles.empty())m_pListIncPatternFiles	= Tokenize(m_szIncPatternFiles);

 		Enumerate(sPath);	// fasten your seat belts, we are ready to go
	}


	list<_stl_string > * GetDirs() { return m_pListDirs; }
	list<_stl_string > * GetFiles() { return m_pListFiles; }

};

#endif // __ENUM_H__

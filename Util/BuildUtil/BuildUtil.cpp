///////////////////////////////////////////////////////////////////////
// BuildUtil.cpp : Simple build utility to increment the build number and 
//                 subsequently change the build strings in build related 
//                 files.   This code is a revised version of ssr
//
// By: Patty Harris
// Copyright 2008 Diomede Inc. All Rights Reserved
//


#include "stdafx.h"
#include "../../Include/types.h"
#include <cassert>
#include <fstream>
#include <string>

using namespace std;

///////////////////////////////////////////////////////////////////////

int Replace(int nOccurrence, std::string const& szSearchText, std::string const& szReplaceText, 
            std::string& szText);
int IncrementBuildNumber(std::string const& szSearchText, std::string& szText,
                         int& nOldBuildNumber, int& nNewBuildNumber);

int ProcessBuildNumber(std::string const& szInputFileName, int nInNewBuildNumber=-1);

int ProcessRevisionNumber(std::string const& szInputFileName, 
                          std::string const& szInSearchText,
                          std::string& szOutSearchText,
                          std::string& szOutReplaceText);
                       
int ProcessFile(std::string const& szInputFileName, 
                std::string szSearchText, 
                std::string szReplaceText);

bool GetCommand(char* szLine, int nLen, std::string szPrompt, FILE *pFile);

///////////////////////////////////////////////////////////////////////
// Main Entry
int _tmain(int argc, _TCHAR* argv[])
{
    if ( argc < 1 || argc > 3 ) {
        std::cerr <<  "Usage: BuildUtil <search_text> <optional replace text>\n"
                "  Use '' to represent \" in search and replace text.\n"
                "  Use /SSR_QUOTE/ to represent ' in search and replace text.\n"
                "  Use /SSR_NL/ to represent a new line in search and replace text.\n"
                "  Use /SSR_TAB/ to represent a tab in search and replace text.\n"
                "\n"
                "Version: 1.0\n";
        return 1;
    }

    std::string szInSearchText = "";        
    std::string szInReplaceText = "";

    if (argc > 1) {
        szInSearchText = std::string(argv[1]);
    }
    
    if (argc > 2) {
        szInReplaceText = std::string(argv[2]);
    }
    
    std::string szSearchText = _T("");
    std::string szReplaceText = szInReplaceText;

    int nNewBuildNumber = -1;
    
    // 1. Find the build number, increment it (could be done in one step).  If there is
    // a revision number increment or replacement, builds start again at 0
    if ( (szInSearchText.length() > 0) || (szInReplaceText.length() > 0) ) {
        nNewBuildNumber = 0;
    }
    
    int nResult = ProcessBuildNumber(_T("C:\\DiomedeDevelopment\\diocli\\Include\\BuildVersion.h"),
        nNewBuildNumber);
    if (nResult != 0) {
        return nResult;
    }

    // 2. Find the revision number, increment it, and replace the remaining version+revsion 
    //    strings with the new value.    
    if (szInSearchText.length() > 0) {
        nResult = ProcessRevisionNumber(_T("C:\\DiomedeDevelopment\\diocli\\Include\\BuildVersion.h"), 
            szInSearchText, szSearchText, szReplaceText);
        if (nResult != 0) {
            return nResult;
        }
        
        // 3. Using this new value, replace the strings in the remaining files.
        nResult = ProcessFile("C:\\DiomedeDevelopment\\diocli\\DioCLI\\DioCLI.cpp",
            szSearchText, szReplaceText);
        if (nResult != 0) {
            return nResult;
        }
    }

    // For testing
    #if 0
    std::string szPrompt = _T(" Press any key to continue ...");
    cout.flush();

    char szLine[2048];
    GetCommand(szLine, sizeof(szLine), szPrompt, stdin);
    #endif

    return nResult;
    
} // End of _tmain

///////////////////////////////////////////////////////////////////////
// Purpose: Replaces the build number (e.g. Build 20)
// Requires:
//      szInputFileName: the build version header file
//      nNewBuildNumber: if 1= -1, will be used as the new build number.
// Returns: 0 if successful, error number otherwise.
int ProcessBuildNumber(std::string const& szInputFileName, int nInNewBuildNumber /*-1*/)
{    
    int nOldBuildNumber = -1;
    int nNewBuildNumber = nInNewBuildNumber;
    std::string szSearchText = _T("VERINFO_BUILDNUMBER");                       
        
    // For our purposes, we always want in placed editting
    bool const bInPlaceEdit = true;
    
    int nOccurrence = 0;
    bool const bReplaceAll = nOccurrence == 0;

    std::string  szOutputFileName = szInputFileName;
    szOutputFileName += ".dio";

    Replace(0, "''", "\"", szSearchText);
    Replace(0, "/SSR_QUOTE/", "'", szSearchText);
    Replace(0, "/SSR_NL/", "\n", szSearchText);
    Replace(0, "/SSR_TAB/", "\t", szSearchText);

    std::ifstream ifsFileInput(szInputFileName.c_str());
    std::ofstream ofsFileOutput(szOutputFileName.c_str());
    std::string szLine;

    cout << _T("Processing build number in file \n") 
         << _T("    ") << szInputFileName << _T("\n");

    if( !ifsFileInput ) {
        std::cerr << "Error opening file, "
            << szInputFileName
            << "\n";
        return 2;
    }

    if( !ofsFileOutput ) {
        std::cerr << "Error opening file, "
            << szOutputFileName
            << "\n";
        return 3;
    }

    int nReturn = -1;
        
    // First, we need to increment or replace the revision number.
    while( std::getline(ifsFileInput, szLine) ) {
        // With the build number handled in a separate step, make
        // sure the rest of the file is copied back.
        if (nReturn < 0) {
             nReturn = IncrementBuildNumber(szSearchText, szLine, nOldBuildNumber, nNewBuildNumber);
        }

        ofsFileOutput << szLine;
        ofsFileOutput << "\n";
    }

    if (nReturn < 0) {
        std::cerr << _T("Error getting new build number...quitting...\n");
        return 1;
    }
    
    std::cout << _T("    Old build number = ") << nOldBuildNumber 
              << _T(", new build number = ") << nNewBuildNumber << _T("\n");

    ifsFileInput.close();
    ofsFileOutput.close();

    if ( bInPlaceEdit ) {
        if ( remove(szInputFileName.c_str()) != 0 ) {
            std::cerr << "Error deleting file, "
                << szInputFileName
                << "\n";
            ::remove(szOutputFileName.c_str());
            return 4;
        }

        if ( ::rename(szOutputFileName.c_str(), szInputFileName.c_str()) != 0 ) {
            std::cerr << "Error renaming file, "
                << szOutputFileName
                << " to "
                << szInputFileName
                << "\n";
            return 5;
        }
    }

    return 0;
    
} // End of ProcessBuildNumber

///////////////////////////////////////////////////////////////////////
int ProcessRevisionNumber(std::string const& szInputFileName, 
                          std::string const& szInSearchText,
                          std::string& szOutSearchText,
                          std::string& szOutReplaceText)
{
    // We know the initial search text - this value has the
    // build number.
    int nOldBuildNumber = -1;
    int nNewBuildNumber = -1;

    //-----------------------------------------------------------------    
    // If we have a replace text, it means we're not incrementing the build
    // number, but replacing it entirely.  In this case, we'll assume the
    // build number itself is the last digit of the string.  For example,
    // replacing 0.92 with 1.1.0 - 0 is the new build number.
    //-----------------------------------------------------------------    
    
    std::string szSearchText = _T("VERINFO_REVNUMBER");                       
    std::string szReplaceText = szOutReplaceText;
        
    // For our purposes, we always want in placed editting
    bool const bInPlaceEdit = true;
    
    int nOccurrence = 0;
    bool const bReplaceAll = nOccurrence == 0;

    std::string  szOutputFileName = szInputFileName;
    szOutputFileName += ".dio";

    Replace(0, "''", "\"", szSearchText);
    Replace(0, "/SSR_QUOTE/", "'", szSearchText);
    Replace(0, "/SSR_NL/", "\n", szSearchText);
    Replace(0, "/SSR_TAB/", "\t", szSearchText);

    std::ifstream ifsFileInput(szInputFileName.c_str());
    std::ofstream ofsFileOutput(szOutputFileName.c_str());
    std::string szLine;

    cout << _T("Processing revision number in file \n") 
         << _T("    ") << szInputFileName << _T("\n");

    if( !ifsFileInput ) {
        std::cerr << "Error opening file, "
            << szInputFileName
            << "\n";
        return 2;
    }

    if( !ofsFileOutput ) {
        std::cerr << "Error opening file, "
            << szOutputFileName
            << "\n";
        return 3;
    }

    int nReturn = -1;
    
    // Are we incrementing or replacing.  If we're replacing, set the
    // new buildnumber to the last digit of the given replace text.
    if (szReplaceText.length() > 0) {
	    int nPos = (int)szReplaceText.find_last_of(_T("."));
	    std::string szTmpBulidNumber = _T("");

	    if ( (nPos > -1) && (nPos < (int)szReplaceText.size()) ) {
		    szTmpBulidNumber = szReplaceText.substr(++nPos, szReplaceText.size() - 1 );
		    nNewBuildNumber = atoi(szTmpBulidNumber.c_str());
		}
    }
    
    // First, we need to increment or replace the revision number.
    while( std::getline(ifsFileInput, szLine) ) {
        nReturn = IncrementBuildNumber(szSearchText, szLine, nOldBuildNumber, nNewBuildNumber);

        ofsFileOutput << szLine;
        ofsFileOutput << "\n";
        
        if (nReturn != -1) {
             break;
        }
    }

    if (nReturn < 0) {
        std::cerr << _T("Error getting new revision number...quitting...\n");
        return 1;
    }
    
    std::cout << _T("    Old revsion number = ") << nOldBuildNumber 
              << _T(", new revision number = ") << nNewBuildNumber << _T("\n");

    // With the new build number, find the search text, and recreate the new
    // version + build string.
	char szNewBuildNumberBuffer[10];
     _itoa(nNewBuildNumber, szNewBuildNumberBuffer, 10);

	char szOldBuildNumberBuffer[10];
     _itoa(nOldBuildNumber, szOldBuildNumberBuffer, 10);

    szOutSearchText = szInSearchText + _T(".") + std::string(szOldBuildNumberBuffer);
    if (szReplaceText.length() == 0) {
        szOutReplaceText = szInSearchText + _T(".") + std::string(szNewBuildNumberBuffer);
    }
    else {
        szOutReplaceText = szReplaceText;
    }
    
    cout << _T("    Replacing version ") << szOutSearchText << _T(" with ") 
         << szOutReplaceText << _T("\n");


    while( std::getline(ifsFileInput, szLine) ) {
        if ( bReplaceAll ) {
            Replace(0, szOutSearchText, szOutReplaceText, szLine);
        }
        else if ( nOccurrence > 0 ) {
            int nFound = Replace(nOccurrence, szOutSearchText, szOutReplaceText, szLine);
            nOccurrence -= nFound;
        }
        ofsFileOutput << szLine;
        ofsFileOutput << "\n";
    }

    ifsFileInput.close();
    ofsFileOutput.close();

    if ( bInPlaceEdit ) {
        if ( remove(szInputFileName.c_str()) != 0 ) {
            std::cerr << "Error deleting file, "
                << szInputFileName
                << "\n";
            ::remove(szOutputFileName.c_str());
            return 4;
        }

        if ( ::rename(szOutputFileName.c_str(), szInputFileName.c_str()) != 0 ) {
            std::cerr << "Error renaming file, "
                << szOutputFileName
                << " to "
                << szInputFileName
                << "\n";
            return 5;
        }
    }

    return 0;
    
} // End of ProcessRevisionNumber

///////////////////////////////////////////////////////////////////////
int ProcessFile(std::string const& szInputFileName, 
                std::string szSearchText, std::string szReplaceText)
{
    std::string szOutputFileName;

    // For our purposes, we always want in placed editting
    bool const bInPlaceEdit = true;

    szOutputFileName = szInputFileName;
    szOutputFileName += ".dio";

    Replace(0, "''", "\"", szSearchText);
    Replace(0, "/SSR_QUOTE/", "'", szSearchText);
    Replace(0, "/SSR_NL/", "\n", szSearchText);
    Replace(0, "/SSR_TAB/", "\t", szSearchText);

    Replace(0, "''", "\"", szReplaceText);
    Replace(0, "/SSR_QUOTE/", "'", szReplaceText);
    Replace(0, "/SSR_NL/", "\n", szReplaceText);
    Replace(0, "/SSR_TAB/", "\t", szReplaceText);

    std::ifstream ifsFileInput(szInputFileName.c_str());
    std::ofstream ofsFileOutput(szOutputFileName.c_str());
    std::string szLine;

    cout << _T("\nProcessing ") << szInputFileName << _T("...\n");

    if( !ifsFileInput ) {
        std::cerr << "Error opening file, "
            << szInputFileName
            << "\n";
        return 2;
    }

    if( !ofsFileOutput ) {
        std::cerr << "Error opening file, "
            << szOutputFileName
            << "\n";
        return 3;
    }

    while( std::getline(ifsFileInput, szLine) ) {
        Replace(0, szSearchText, szReplaceText, szLine);

        ofsFileOutput << szLine;
        ofsFileOutput << "\n";
    }

    ifsFileInput.close();
    ofsFileOutput.close();

    if ( bInPlaceEdit ) {
        if ( remove(szInputFileName.c_str()) != 0 ) {
            std::cerr << "Error deleting file, "
                << szInputFileName
                << "\n";
            ::remove(szOutputFileName.c_str());
            return 4;
        }

        if ( ::rename(szOutputFileName.c_str(), szInputFileName.c_str()) != 0 ) {
            std::cerr << "Error renaming file, "
                << szOutputFileName
                << " to "
                << szInputFileName
                << "\n";
            return 5;
        }
    }

    return 0;
    
} // End of ProcessFile

///////////////////////////////////////////////////////////////////////
int Replace(int nOccurrence, std::string const& szSearchText, 
                             std::string const& szReplaceText, std::string& szText)
{
    assert(nOccurrence >= 0);

    int nFound = 0;

    std::string::size_type pos = szText.find(szSearchText);
    
    while( pos != std::string::npos ) {
        ++nFound;

        if( nFound == nOccurrence || nOccurrence == 0 ) {
            szText.replace(pos, szSearchText.length(), szReplaceText);
        }

        pos = szText.find(szSearchText, pos + szReplaceText.length());
    }

    return nFound;
    
} // End Replace

///////////////////////////////////////////////////////////////////////
int IncrementBuildNumber(std::string const& szSearchText, std::string& szText,
                            int& nOldBuildNumber, int& nNewBuildNumber)
{
    int nBuildNumber = -1;
    
    std::string::size_type pos = szText.find(szSearchText);
    
    if ( pos != std::string::npos ) {
        // The build number line is string + space + build number
        // e.g. #define VERINFO_REVNUMBER 4
        
        std::string::size_type posNumber = pos + szSearchText.length() + 1;
        
        nOldBuildNumber = atoi(szText.substr(posNumber).c_str());
        
        // If the build number hasn't been set, increment the
        // build number.
        if (nNewBuildNumber == -1) {
            nNewBuildNumber = nOldBuildNumber + 1;
        }
        
        char szBuildNumberBuffer[10];
        _itoa(nNewBuildNumber, szBuildNumberBuffer, 10);
        
        szText = szText.substr(0, posNumber) + std::string(szBuildNumberBuffer);
    }

    return (int)pos;

} // End IncrementBuildNumber

/////////////////////////////////////////////////////////////////////////////////
// Purpose: Retrieve the command from the command line.
// Requires:
//      szLine: string of characters from the keyboard stream.
//      nLen: maximum size of accepted string
//      szPrompt: prompt displayed
//      pFile: input stream
// Returns: true if successful, false otherwise
bool GetCommand(char* szLine, int nLen, std::string szPrompt, FILE *pFile)
{
    szLine[0] = 0;
    if (pFile == stdin) {
        _tprintf(szPrompt.c_str());
    }

    cout.flush();

    // Check for NULL indicates EOF...when the user is entering from the
    // command line, "\n" is not the same as NULL, so this check works
    // in that scenario as well.

    char* szReturn = fgets(szLine, nLen, pFile);
    if (szReturn == NULL) {
        return false;
    }

    cout.flush();
    return true;

} // GetCommand

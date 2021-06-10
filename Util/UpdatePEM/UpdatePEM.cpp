///////////////////////////////////////////////////////////////////////
// UpdatePEM.cpp : Simple utility to update the PEM file embedded in
//                 the header file DiomedePem.h
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

//---------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------
#ifdef WIN32
    // Note: when we move to the MAC, the MAC is the same as
    // windows, except for the volume separator.
    static const std::string g_szDirectorySeparator = "\\";
#else
    static const std::string g_szDirectorySeparator = "/";
#endif

//---------------------------------------------------------------------
// Function definitions
//---------------------------------------------------------------------
int ProcessFile(std::string szHeaderFileName, 
                std::string szPEMFileName);

bool GetCommand(char* szLine, int nLen, std::string szPrompt, FILE *pFile);

//---------------------------------------------------------------------
// These constants appear appropriately at the beginning and end of the PEM
// in the header file.
//---------------------------------------------------------------------
const std::string PEM_FIRST_LINE = _T("PEM_FIRST_LINE");
const std::string PEM_LAST_LINE  = _T("PEM_LAST_LINE");

///////////////////////////////////////////////////////////////////////
// Main Entry
int _tmain(int argc, _TCHAR* argv[])
{
    if ( argc < 1 || argc > 2 ) {
        std::cerr <<  "Usage: UpdatePEM <path>\n"
                "  Use the full path of the header file and PEM file.\n"
                "  For example:\n"
                "  UpdatePEM c:\\DiomedeDevelopment\\diocl\\DioCLI \n"
                "\n"
                "Version: 1.0\n";
        return 1;
    }

    std::string szInFilePath(argv[1]);
    
    std::string szHeaderFileName = szInFilePath + g_szDirectorySeparator + _T("DiomedePEM.h");
    std::string szPEMFileName = szInFilePath + g_szDirectorySeparator + _T("Diomede.pem");
    
    int nResult = ProcessFile(szHeaderFileName, szPEMFileName);
    if (nResult != 0) {
        return nResult;
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
int ProcessFile(std::string szHeaderFileName, 
                std::string szPEMFileName)
{
    // For our purposes, we always want in placed editting
    bool const bInPlaceEdit = true;

    std::string szOutputFileName = szHeaderFileName;
    szOutputFileName += ".dio";

    std::ifstream ifsFileInput(szHeaderFileName.c_str());
    std::ofstream ofsFileOutput(szOutputFileName.c_str());
    std::ifstream ifsFilePEMInput(szPEMFileName.c_str());
    
    std::string szLine;

    cout << _T("\nProcessing ") << szHeaderFileName << _T("...\n");

    if( !ifsFileInput ) {
        std::cerr << "Error opening file, "
            << szHeaderFileName
            << "\n";
        return 2;
    }

    if( !ifsFilePEMInput ) {
        std::cerr << "Error opening file, "
            << szHeaderFileName
            << "\n";
        return 2;
    }

    if( !ofsFileOutput ) {
        std::cerr << "Error opening file, "
            << szPEMFileName
            << "\n";
        return 3;
    }

    // Parse the header file until we come to our "first line" string - it
    // appears once as a constant definition and a second time in it's 
    // usage - should fix this...
    int bFoundFirstLine = 0;
    while ( std::getline(ifsFileInput, szLine) ) {
        if (szLine.find(PEM_FIRST_LINE) != size_t(-1)) {
            bFoundFirstLine++;
        }

        ofsFileOutput << szLine;
        ofsFileOutput << "\n";
        
        if (bFoundFirstLine > 1) {
            break;
        }
    }

    // If we got the end of the header file, and we never found our
    // starting line, quit - some error has occurred.
    if( bFoundFirstLine < 2 ) {
        std::cerr << "Header file not updated - no start key found!  "
            << szPEMFileName
            << "\n";
            
        ifsFileInput.close();
        ofsFileOutput.close();
        ifsFilePEMInput.close();

        return 4;
    }
    
    const std::string szLineBegin = _T(",_T(\"");
    const std::string szLineEnd = _T("\\n\")");
    
    // Now copy all the lines from the PEM file into the (temporary) header file,
    // adding the appriate string notations.
    while ( std::getline(ifsFilePEMInput, szLine) ) {
    
        ofsFileOutput << szLineBegin + szLine + szLineEnd;
        ofsFileOutput << "\n";
    }
    
    // And lastly, skip ahead to the "last line", copy the rest of the
    // header file to the temporary header file.
    bool bFoundLastLine = false;
    std::string szLastLine = _T("");
    while ( std::getline(ifsFileInput, szLine) ) {
        if (szLine.find(PEM_LAST_LINE) != size_t(-1)) {
            szLastLine = szLine;
            bFoundLastLine = true;
        }

        if (bFoundLastLine) {
            ofsFileOutput << szLine;
            ofsFileOutput << "\n";
        }        
    }

    if (bFoundLastLine == false) {
        std::cerr << "Header file not updated - no end key found!  "
            << szPEMFileName
            << "\n";
            
        ifsFileInput.close();
        ofsFileOutput.close();
        ifsFilePEMInput.close();

        return 5;
    }

    // Success!
    ifsFileInput.close();
    ofsFileOutput.close();
    ifsFilePEMInput.close();

    if ( bInPlaceEdit ) {
        if ( remove(szHeaderFileName.c_str()) != 0 ) {
            std::cerr << "Error deleting file, "
                << szHeaderFileName
                << "\n";
            ::remove(szOutputFileName.c_str());
            return 4;
        }

        if ( ::rename(szOutputFileName.c_str(), szHeaderFileName.c_str()) != 0 ) {
            std::cerr << "Error renaming file, "
                << szOutputFileName
                << " to "
                << szHeaderFileName
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
        // e.g. #define VERINFO_BUILDNUMBER 4
        
        std::string::size_type posNumber = pos + szSearchText.length() + 1;
        
        nOldBuildNumber = atoi(szText.substr(posNumber).c_str());
        nNewBuildNumber = nOldBuildNumber + 1;
        
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

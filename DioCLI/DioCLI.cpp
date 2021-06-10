/*********************************************************************
 * 
 *  file:  DioCLI.cpp
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Defines the entry point for the console application.
 * 
 *********************************************************************/

///////////////////////////////////////////////////////////////////////
//!
//! \file DioCLI.cpp : Defines the entry point for the console application.
//!
//! @mainpage    DioCLI: Command line interface to the Diomede services.
//! @version     1.1.3, 2010
//! @author      Patty Harris
//!
//!
#include "stdafx.h"
#include "types.h"

#ifdef WIN32
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include "crtdbg.h"
#endif

#include "resource.h"
#endif

#include <string>
#include <iostream>
#include <algorithm>
#include "tclap/CmdLine.h"
#include "ConsoleControl.h"

#include "CommandDefs.h"
#include "ApplicationDefs.h"
#include "../Util/Util.h"

#include "../Util/ProfileManager.h"
#include "../Util/ClientLog.h"
#include "../Util/ClientLogUtils.h"
#include "../Util/ErrorType.h"
#include "../Include/ErrorCodes/UIErrors.h"

using namespace TCLAP;

//! \defgroup diocli DioCLI
//! Main for the DioCLI command line interface.
//! @{

///////////////////////////////////////////////////////////////////////
//! \name Globals
//!@{

//! \def INIT_CTRL_KEY_TYPE
//! \brief CTRL + key trapped by the ControlHandler - Windows only.
//!        As a DWORD, use a large number as the initialized value.
#define INIT_CTRL_KEY_TYPE  100

//! \var g_ctrlType
//! \brief CTRL + key trapped by the ControlHandler - Windows only.
//! Indicates the control key type input.
//!
DWORD g_ctrlType;

//! \var g_bUsingCtrlKeys
//! \brief CTRL + key trapped by the ControlHandler - Windows only.
//! Indicates the control key type input.
bool g_bUsingCtrlKeys;

//! \var g_bFileInput
//! \brief Indicates input is from a file.
bool g_bFileInput;

//!@}

///////////////////////////////////////////////////////////////////////
// Methods

///////////////////////////////////////////////////////////////////////
//! \brief Display a simple console title.
//!
//! \return nothing
//!
//! \remarks Currently not implemented for Linux.
//!
void ShowTitle(void)
{
    #ifdef WIN32
        SetConsoleTitle(_T("DioCLI"));
	#else
	#endif

} // End ShowTitle

///////////////////////////////////////////////////////////////////////
//! \brief Retrieve the command from the command line.
//!
//! \param szLine: string of characters from the keyboard stream.
//! \param nLen: maximum size of accepted string
//! \param szPrompt: prompt displayed
//! \param pFile: input stream
//!
//! \return true if successful, false otherwise
bool GetCommand(char *szLine, int nLen, std::string szPrompt, FILE *pFile)
{
    szLine[0] = 0;
    if (pFile == stdin) {
        if (szPrompt.length() > 0) {
            _tprintf(_T("%s"), szPrompt.c_str());
        }
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

///////////////////////////////////////////////////////////////////////
//! \brief Retrieve the command from the command line, one character at a
//!        time.  The input is masked using asterisks (*).
//!
//! \param szLine: string of characters from the keyboard stream.
//! \param nLen: maximum size of accepted string
//! \param szPrompt: prompt displayed
//! \param pFile: input stream
//!
//! \return true if successful, false otherwise
bool GetMaskedCommand(char *szLine, int nLen, std::string szPrompt, FILE *pFile)
{
    szLine[0] = 0;
    if (pFile == stdin) {
        _tprintf(_T("%s"), szPrompt.c_str());
    }

    cout.flush();

    // Check for NULL indicates EOF...when the user is entering from the
    // command line, "\n" is not the same as NULL, so this check works
    // in that scenario as well.

    /*
    char* szReturn = fgetc(szLine, nLen, pFile);
    if (szReturn == NULL) {
        return false;
    }
    */

    int nChar;
    std::string szInput;
    int nCharCount = 0;

    while ((nChar = Util::CustomGetch()) != EOF
         && nChar != '\n'
         && nChar != '\r')
    {
        // Handle backspace and delete
        // Up, left, right, and down arrows = 72, 75, 77, and 80
        if (nChar == '\b' && nCharCount > 0) {
            _tprintf(_T("\b \b"));
            cout.flush();
            nCharCount--;

            szInput = szInput.substr(0, nCharCount);
        }
        else /*if (isalnum(nChar))*/ {
            // 4/20/2010: removing check for alphanumeric to handle auto-generated
            // passwords from the service (for those who forget their passwords).
            cout << '*';
            szInput += (char)nChar;
            nCharCount ++;
        }
    }

    strcpy(szLine, szInput.c_str());
    _tprintf(_T("\n\r"));

    return true;

} // GetMaskedCommand

///////////////////////////////////////////////////////////////////////
//! \brief Set a control handler for keyboard input (Windows only).
//!
//! \param fdwCtrlType: keyboard control key + key combination.
//!
//! \return TRUE if successful, FALSE otherwise
BOOL CtrlHandler( DWORD fdwCtrlType )
{
#ifdef WIN32
    switch( fdwCtrlType )
    {
        // Handle the CTRL-C signal.
        case CTRL_C_EVENT:
            // _tprintf( "Ctrl-C event\n\n" );
            g_ctrlType = fdwCtrlType;
            g_bUsingCtrlKeys = true;
            return TRUE;

        // CTRL-CLOSE: confirm that the user wants to exit.
        case CTRL_CLOSE_EVENT:
            // _tprintf( "Ctrl-Close event\n\n" );
            g_ctrlType = fdwCtrlType;
            g_bUsingCtrlKeys = true;
            ExitProcess(0);
            return TRUE;

        // Pass other signals to the next handler.
        case CTRL_BREAK_EVENT:
            // _tprintf( "Ctrl-Break event\n\n" );
            return FALSE;

        case CTRL_LOGOFF_EVENT:
            // _tprintf( "Ctrl-Logoff event\n\n" );
            return FALSE;

        case CTRL_SHUTDOWN_EVENT:
            // _tprintf( "Ctrl-Shutdown event\n\n" );
            return FALSE;

        default:
            return FALSE;
    }
#else
    return TRUE;
#endif

} // End CtrlHandler

///////////////////////////////////////////////////////////////////////
//! \brief Initialize console icon (Windows only)
//!
//! \param hNewIcon: handle to the new icon
//!
//! \return nothing
#ifdef WIN32
void ChangeIcon( const HICON hNewIcon )
{
    // Load kernel 32 library
    HMODULE hMod = LoadLibrary( _T( "Kernel32.dll" ));
    ASSERT( hMod );

    // Load console icon changing procedure
    typedef DWORD ( __stdcall GetSetConsoleIconFunc )( HICON );

    GetSetConsoleIconFunc* pfnSetConsoleIcon =
        reinterpret_cast<GetSetConsoleIconFunc*>( GetProcAddress( hMod, "SetConsoleIcon" ));
    ASSERT( pfnSetConsoleIcon );

    // Call function to change icon
    pfnSetConsoleIcon( hNewIcon );

    FreeLibrary( hMod );

} // End ChangeIcon
#endif

///////////////////////////////////////////////////////////////////////
//! \brief Initialize client logging.
//! \return nothing.
void SetLogging()
{
    // Client logging allows specification of output on a per component
    // basis.  Currently the client logger is setup for UI and UTIL only.
    SetComponentLogging( UI_COMPONENT_ERROR			, 1);
    SetComponentLogging( UTIL_COMPONENT_ERROR		, 1);

    EnableLogging( true );

    int defaultLoggingEvents = 0;
    defaultLoggingEvents |= LOG_STATUS;
    defaultLoggingEvents |= LOG_WARNING;
    defaultLoggingEvents |= LOG_ERROR;
    defaultLoggingEvents |= LOG_STATECHANGE;
    defaultLoggingEvents |= LOG_EXCEPTION;

    SetDefaultLoggingEvents( defaultLoggingEvents );

#ifdef WIN32
    EnableDebuggerLogging( true );
#endif

} // End SetLogging

///////////////////////////////////////////////////////////////////////
//! \brief Main Entry
//!
//! \param argc: An integer that contains the count of arguments that follow in argv.
//! \param argv: An array of null-terminated strings representing command-line arguments
//!
//! \return exit code
int _tmain(int argc, _TCHAR* argv[])
{
    #ifdef WIN32
        HMODULE hMainMod = GetModuleHandle( 0 );
        ASSERT( hMainMod );

        HICON hMainIcon = ::LoadIcon( hMainMod, MAKEINTRESOURCE( IDI_DIOCLI ));
        ASSERT( hMainIcon );

        // Change main window icon
        ChangeIcon( hMainIcon );
    #endif

    // Initializing ctrl type to a number greater than the
    // known ctrl-key combination - hacky at best.
    g_ctrlType = INIT_CTRL_KEY_TYPE;
    g_bUsingCtrlKeys = false;

    g_bFileInput = false;

	SetLogging();

	// Display a simple application banner - currently the startup of
	// logging displays a more complete banner.
	ShowTitle();

    // Install the control handler (Windows only)
    BOOL fSuccess = TRUE;
    #ifdef WIN32
        fSuccess = SetConsoleCtrlHandler(
            (PHANDLER_ROUTINE) CtrlHandler,
            TRUE);
    #endif
    if (! fSuccess ) {
        // TBD: quit ? alert the user ?
        std::cout << _T("Could not set control handler") << std::endl;
    }

	ConsoleControl consoleControl;

	#ifdef WIN32
		if (_isatty(_fileno( stdin )) == false ) {
		    consoleControl.SetRedirectedInput();
		    g_bFileInput = true;
		}

	#else
		if (isatty(fileno( stdin )) == false ) {
		    consoleControl.SetRedirectedInput();
		    g_bFileInput = true;
		}
	#endif

	//-----------------------------------------------------------------
	// Start up DioCLI
	// Pass the input arguments to the startup - if the input
	// command is "login", we'll ignore the autologin
	// settings.
	//-----------------------------------------------------------------
	bool bSystemCommandInput = (argc > 1);
	bool bSuccess = consoleControl.StartDioCLI(argc, argv);

    // For now, we'll only log to a file.  Our console output is sufficient for
    // general work...

	if (bSuccess == false) {
	    ClientLog(UI_COMP, LOG_ERROR, false,
	        _T("DioCLI was unable to start due to the following error: %s"),
	        ErrorType(consoleControl.GetErrorCode()).AsString().c_str());

	        // Success or failure here is dictated by the auto login process.
	        // If we fail, we need to quit only if input is from the system
	        // command line.  Otherwise, the user can re-enter the login
	        // information within DioCLI.
	        if (bSystemCommandInput) {
	            return 0;
	        }
	}

    char szLine[2048];
    std::string szPrompt = _T("");

	//-----------------------------------------------------------------
	// Check for entry from the system command line.
	//-----------------------------------------------------------------
	if (bSystemCommandInput) {
	    consoleControl.AddActions(argc, argv);
        consoleControl.ProcessActionStack();

        if ( false == consoleControl.IsCommandIncomplete()  ) {
	        consoleControl.StopDioCLI();
	        return 0;
        }
	}

	//-----------------------------------------------------------------
	// Else, prompt the user from our command window.
	//-----------------------------------------------------------------
    do {
        szPrompt = consoleControl.GetCommandPrompt();

        cout.flush();

        if ( consoleControl.IsMaskInput()) {
            if ( GetMaskedCommand(szLine, sizeof(szLine), szPrompt, stdin) == false) {
                consoleControl.SetFinished();
                break;
            }
        }
        else {
            if (g_bFileInput && consoleControl.IsCommandInputIgnored()) {
                szPrompt = _T("\r") + szPrompt;
            }
            if ( GetCommand(szLine, sizeof(szLine), szPrompt, stdin) == false) {
                consoleControl.SetFinished();
                break;
            }
        }

        if (g_bUsingCtrlKeys == true) {
            consoleControl.ProcessControlHandler(g_ctrlType);
            cout.flush();

            // The Windows cmd.exe puts a extra line in between the prompts -
            // This puts the prompt on the next line.
            _tprintf(_T("\n\r"));

        }
        else {
            std::string szTrimmedLine = _T("");
            consoleControl.Trim(szLine, szTrimmedLine);
            cout.flush();

            // We may need to process these actions in a separate thread - in which
            // case, commands are added to the stack of actions and processed
            // separately in the thread.
            if ( consoleControl.IsCommandIncomplete() || ( szTrimmedLine.length() > 0) ) {
                consoleControl.AddAction(szTrimmedLine);
                consoleControl.ProcessActionStack();
            }

            cout.flush();
        }

        g_ctrlType = INIT_CTRL_KEY_TYPE;
        g_bUsingCtrlKeys = false;

    } while( consoleControl.Finished() == false);

	consoleControl.StopDioCLI();

	#ifdef _DEBUG
    // _CrtDumpMemoryLeaks();
    #endif

	return 0;

} // End of main

//! @}

///////////////////////////////////////////////////////////////////////
// Purpose:
//		Makes g++ happy - a link error results otherwise (complaining about
//	    _WinMain.
// Requires: nothing
// Returns: nothing
/*
int main()
{
	return 0;

} // End of dummpy main
*/

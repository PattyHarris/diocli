/*********************************************************************
 *
 *  file:  DiomedeStdOutput.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Diomede specific output for usages and failures.
 *
 *********************************************************************/

#ifndef __DIOMEDE_STD_OUT_H__
#define __DIOMEDE_STD_OUT_H__

#include "../Include/tclap/CmdLine.h"
#include "../Util/XString.h"
#include <iostream>
#include <string>

using namespace TCLAP;
using namespace std;

///////////////////////////////////////////////////////////////////////
class DiomedeStdOutput : public StdOutput
{
	public:

		/**
		 * Prints the usage to stdout.  Can be overridden to
		 * produce alternative behavior.
		 * \param c - The CmdLine object the output is generated for.
		 */
		virtual void usage(CmdLineInterface& c);

		/**
		 * Prints the version to stdout. Can be overridden
		 * to produce alternative behavior.
		 * \param c - The CmdLine object the output is generated for.
		 */
		virtual void version(CmdLineInterface& c);

		/**
		 * Prints (to stderr) an error message, short usage
		 * Can be overridden to produce alternative behavior.
		 * \param c - The CmdLine object the output is generated for.
		 * \param e - The ArgException that caused the failure.
		 */
		virtual void failure(CmdLineInterface& c,
						     ArgException& e );

		/**
		 * Diomede: Prints (to stderr) that the command has
		 * failed and alerts the user to use "cmd" /? for
		 * the command usage.
		 * \param c - The CmdLine object the output is generated for.
		 * \param e - The ArgException that caused the failure.
		 */
		virtual void cmdFailure(CmdLineInterface& c,
						        ArgException& e );

		/**
		 * Prints (to stderr) an error message for an unknown command.
		 * Can be overridden to produce alternative behavior.
		 * \param cmdName - unknown command name
		 * \param e - The ArgException that caused the failure.
		 */
		virtual void unknownCommand(std::string cmdName,
						            ArgException& e );
	protected:

        /**
         * Writes a brief usage message with short args.
		 * \param c - The CmdLine object the output is generated for.
         * \param os - The stream to write the message to.
         */
        void _shortUsage( CmdLineInterface& c, std::ostream& os ) const;

        /**
         * Writes a brief listing of alternate versions of the command
		 * \param c - The CmdLine object the output is generated for.
         * \param os - The stream to write the message to.
         */
        void _altCommands( CmdLineInterface& c, std::ostream& os ) const;

        /**
		 * Writes a longer usage message with long and short args,
		 * provides descriptions and prints message.
		 * \param c - The CmdLine object the output is generated for.
		 * \param os - The stream to write the message to.
		 */
		void _longUsage( CmdLineInterface& c, std::ostream& os ) const;

public:
		/**
		 * This function inserts line breaks and indents long strings
		 * according the  params input. It will only break lines at spaces,
		 * commas and pipes.
		 * \param os - The stream to be printed to.
		 * \param s - The string to be printed.
		 * \param maxWidth - The maxWidth allowed for the output line.
		 * \param indentSpaces - The number of spaces to indent the first line.
		 * \param secondLineOffset - The number of spaces to indent the second
		 * and all subsequent lines in addition to indentSpaces.
		 */
		void spacePrint( std::ostream& os,
						 const std::string& s,
						 int maxWidth,
						 int indentSpaces,
						 int secondLineOffset ) const;

}; // End class

///////////////////////////////////////////////////////////////////////
inline void DiomedeStdOutput::version(CmdLineInterface& _cmd)
{
	std::string progName = _cmd.getProgramName();
	std::string version = _cmd.getVersion();

	std::cout << std::endl << progName << "  version: "
			  << version << std::endl << std::endl;
}

///////////////////////////////////////////////////////////////////////
inline void DiomedeStdOutput::usage(CmdLineInterface& _cmd )
{
    // Diomede: Print the command description.
	std::cout << std::endl << "DESCRIPTION: " << std::endl;

	std::string message = _cmd.getMessage();
	spacePrint( std::cout, message, 75, 3, 0 );

	std::cout << std::endl << "USAGE: " << std::endl;

	_shortUsage( _cmd, std::cout );
    _altCommands( _cmd, std::cout );

	std::cout  << std::endl << "ARGUMENTS: " << std::endl;

	_longUsage( _cmd, std::cout );

} // End usage

///////////////////////////////////////////////////////////////////////
inline void DiomedeStdOutput::failure( CmdLineInterface& _cmd,
				                       ArgException& e )
{
	std::string progName = _cmd.getProgramName();
	std::string cmdName = _cmd.getCommandName();

	std::cerr << "PARSE ERROR: " << e.argId() << std::endl
		      << "             " << e.error() << std::endl << std::endl;

	if ( _cmd.hasCommandHelp() )
	{
		std::cerr << "Brief USAGE: " << std::endl;

		_shortUsage( _cmd, std::cerr );
	    _altCommands( _cmd, std::cerr );

		std::cerr << std::endl << "For complete USAGE and HELP type: "
			      << std::endl << cmdName << "/?"
				  << std::endl << std::endl;
	}
	else
		usage(_cmd);

} // End failure

///////////////////////////////////////////////////////////////////////
inline void DiomedeStdOutput::cmdFailure( CmdLineInterface& _cmd,
				                ArgException& e )
{
	std::string cmdName = _cmd.getCommandName();

	std::cerr << "PARSE ERROR: " << e.argId() << std::endl
		      << "             " << e.error() << std::endl;

	if ( _cmd.hasCommandHelp() )
	{
	    #ifdef WIN32
		    std::cerr << std::endl << "For complete USAGE and HELP type: " << cmdName << " /?"
				      << std::endl << std::endl;
	    #else
		    std::cerr << std::endl << "For complete USAGE and HELP type: " << cmdName << " -?"
				      << std::endl << std::endl;
	    #endif
	}

} // End cmdFailure

///////////////////////////////////////////////////////////////////////
inline void DiomedeStdOutput::unknownCommand( std::string cmdName,
				                              ArgException& e )
{
	std::cerr << std::endl << "UNKNOWN COMMAND: " << cmdName << std::endl;
	std::cerr << e.what() << std::endl;

	std::cerr << std::endl << "For a complete list of available commands, type: "
		      << "help "
			  << std::endl;

} // End unknownCommand

///////////////////////////////////////////////////////////////////////
inline void DiomedeStdOutput::_altCommands( CmdLineInterface& _cmd,
                                            std::ostream& os ) const
{
	std::list<std::string>* altCommandList = _cmd.getAltCommmandList();

	// All commands have at least one command - the long version.
	if (altCommandList->size() <= 1) {
	    return;
	}

	std::string szLongCmd = _cmd.getCommandName();
	std::string s = "Alias: ";
	for( AltCmdListIterator it = altCommandList->begin(); it != altCommandList->end(); it++ )
	{
		if ( szLongCmd != (*it) ) {

            // Convert to upper case -
            char szAltCommand[20];
            strcpy(szAltCommand, (*it).c_str());

            _tcsupr(szAltCommand);
            std::string szTemp(szAltCommand);

		    s += szTemp + " ";
		}
    }

	spacePrint( std::cout, s, 75, 3, 0 );

} // End _altCommands

///////////////////////////////////////////////////////////////////////
inline void DiomedeStdOutput::_shortUsage( CmdLineInterface& _cmd,
				                           std::ostream& os ) const
{
	std::list<Arg*> argList = _cmd.getArgList();
	std::string cmdName = _cmd.getCommandName();
	XorHandler xorHandler = _cmd.getXorHandler();
	std::vector< std::vector<Arg*> > xorList = xorHandler.getXorList();

    // Convert to upper case -
    char szCommand[30];
    strcpy(szCommand, cmdName.c_str());

    // Print the command name first (original version also included
    // the program name).
    _tcsupr(szCommand);
    std::string szCommandName(szCommand);
	std::string s = szCommandName;

	// First the xor
	for ( int i = 0; static_cast<unsigned int>(i) < xorList.size(); i++ )
	{
		s += " {";
		for ( ArgVectorIterator it = xorList[i].begin();
						it != xorList[i].end(); it++ )
			s += (*it)->shortID() + "|";

		s[s.length()-1] = '}';
	}

	// Then the rest
	for (ArgListIterator it = argList.begin(); it != argList.end(); it++) {
		if ( !xorHandler.contains( (*it) ) ) {
		    // Diomede: only show required arguments in the command usage.
		    Arg* arg = (*it);
		    if (arg->isRequired()) {
			    s += " " + (*it)->shortID();
			}
	    }
    }

	// If the command name is too long, then adjust the second line offset
	int secondLineOffset = static_cast<int>(szCommandName.length()) + 2;
	if ( secondLineOffset > 75/2 )
			secondLineOffset = static_cast<int>(75/2);

	spacePrint( std::cout, s, 75, 3, secondLineOffset );

} // End _shortUsage

///////////////////////////////////////////////////////////////////////
inline void DiomedeStdOutput::_longUsage( CmdLineInterface& _cmd,
			                              std::ostream& os ) const
{
	std::list<Arg*> argList = _cmd.getArgList();
	XorHandler xorHandler = _cmd.getXorHandler();
	std::vector< std::vector<Arg*> > xorList = xorHandler.getXorList();

	// Next the xor which includes the arguments.
	for ( int i = 0; static_cast<unsigned int>(i) < xorList.size(); i++ )
	{
		for ( ArgVectorIterator it = xorList[i].begin();
			  it != xorList[i].end();
			  it++ )
		{
			spacePrint( os, (*it)->longID(), 75, 3, 3 );
			spacePrint( os, (*it)->getDescription(), 75, 5, 0 );

			if ( it+1 != xorList[i].end() )
				spacePrint(os, "-- OR --", 75, 9, 0);
		}
		os << std::endl << std::endl;
	}

	// The the rest - first show the required arguments, loop again to
	// show the optional arguments.
	for (ArgListIterator it = argList.begin(); it != argList.end(); it++)
		if ( !xorHandler.contains( (*it) ) )
		{
		    // Skip the optional arguemnts on this loop.
		    if (!(*it)->isRequired()) {
		        continue;
		    }

		    #ifndef _DEBUG
		        // Skip the test switches
		        if ( (*it)->longID() == _T("test")) {
		            continue;
		        }
		    #endif
			spacePrint( os, (*it)->longID(), 75, 3, 3 );
			spacePrint( os, (*it)->getDescription(), 75, 5, 0 );
			os << std::endl;
		}

	for (ArgListIterator it = argList.begin(); it != argList.end(); it++)
		if ( !xorHandler.contains( (*it) ) )
		{
		    // Skip the required arguemnts on this loop.
		    if ((*it)->isRequired()) {
		        continue;
		    }

		    #ifndef _DEBUG
		        // Skip the test switches
		        if ( (*it)->longID() == _T("test")) {
		            continue;
		        }
		    #endif
			spacePrint( os, (*it)->longID(), 75, 3, 3 );
			spacePrint( os, (*it)->getDescription(), 75, 5, 0 );
			os << std::endl;
		}

    // Seems to be an unnecessary extra line here...
	// os << std::endl;

} // End _longUsage

///////////////////////////////////////////////////////////////////////
inline void DiomedeStdOutput::spacePrint( std::ostream& os,
						                  const std::string& s,
						                  int maxWidth,
						                  int indentSpaces,
						                  int secondLineOffset ) const
{
	int len = static_cast<int>(s.length());

	if ( (len + indentSpaces > maxWidth) && maxWidth > 0 )
	{
		int allowedLen = maxWidth - indentSpaces;
		int start = 0;
		while ( start < len )
		{
			// find the substring length
			#ifdef WIN32
			int stringLen = min( len - start, allowedLen );
			#else
			int stringLen = std::min( len - start, allowedLen );
			#endif

			// trim the length so it doesn't end in middle of a word
			if ( stringLen == allowedLen )
				while ( s[stringLen+start] != ' ' &&
			   	        s[stringLen+start] != ',' &&
			   	        s[stringLen+start] != '|' &&
						stringLen >= 0 )
					stringLen--;

			// ok, the word is longer than the line, so just split
			// wherever the line ends
			if ( stringLen <= 0 )
				stringLen = allowedLen;

			// check for newlines
			for ( int i = 0; i < stringLen; i++ )
				if ( s[start+i] == '\n' )
					stringLen = i+1;

			// print the indent
			for ( int i = 0; i < indentSpaces; i++ )
				os << " ";

			if ( start == 0 )
			{
				// handle second line offsets
				indentSpaces += secondLineOffset;

				// adjust allowed len
				allowedLen -= secondLineOffset;
			}

			os << s.substr(start,stringLen) << std::endl;

			// so we don't start a line with a space
			while ( s[stringLen+start] == ' ' && start < len )
				start++;

			start += stringLen;
		}
	}
	else
	{
		for ( int i = 0; i < indentSpaces; i++ )
				os << " ";
		os << s << std::endl;
	}

} // End spacePrint

#endif // __DIOMEDE_STD_OUT_H__
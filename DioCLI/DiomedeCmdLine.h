/******************************************************************************
 *
 *  file:  DiomedeCmdLine.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Diomede specific command parser to handle commands such as REM.
 *
 *****************************************************************************/

#ifndef __DIOMEDE_CMDLINE_H__
#define __DIOMEDE_CMDLINE_H__

#include "../Include/tclap/CmdLine.h"
#include "../Include/tclap/SwitchArg.h"
#include "../Include/tclap/MultiSwitchArg.h"
#include "../Include/tclap/UnlabeledValueArg.h"
#include "../Include/tclap/UnlabeledMultiArg.h"

#include "../Include/tclap/XorHandler.h"
#include "../Include/tclap/HelpVisitor.h"
#include "../Include/tclap/VersionVisitor.h"
#include "../Include/tclap/IgnoreRestVisitor.h"

#include "../Include/tclap/CmdLineOutput.h"
#include "../Include/tclap/StdOutput.h"

#include "../Include/tclap/Constraint.h"
#include "../Include/tclap/ValuesConstraint.h"

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "types.h"
#include "CommandDefs.h"

namespace TCLAP {

/**
 * The base class that manages the command line definition and passes
 * along the parsing to the appropriate Arg classes.
 */
class DiomedeCmdLine : public CmdLine
{
	public:

		/**
		 * Command line constructor. Defines how the arguments will be
		 * parsed.
		 * \param message - The message to be used in the usage
		 * output.
		 * \param delimiter - The character that is used to separate
		 * the argument flag/name from the value.  Defaults to ' ' (space).
		 * \param version - The version number to be used in the
		 * --version switch.
		 * \param helpAndVersion - Whether or not to create the Help and
		 * Version switches. Defaults to true.
		 */
		DiomedeCmdLine(const std::string& message,
				const char delimiter = ' ',
				const std::string& version = "none",
				bool helpAndVersion = true);

		/**
		 * Command line constructor. Defines how the arguments will be
		 * parsed.
		 * \param command - The command itself
		 * \param message - The message to be used in the usage
		 * output.
		 * \param delimiter - The character that is used to separate
		 * the argument flag/name from the value.  Defaults to ' ' (space).
		 * \param userPrompts - The user will be prompted for missing required
		 * arguments.
		 * \param version - The version number to be used in the
		 * --version switch.
		 * \param helpAndVersion - Whether or not to create the Help and
		 * Version switches. Defaults to false.  Diomede uses a help
		 * command for showing the usage for a given command.
		 */
		DiomedeCmdLine(const std::string& command,
		        const std::string& message,
				const char delimiter = ' ',
				bool userPrompts = false,
				const std::string& version = "none",
				bool helpAndVersion = false);

		/**
		 * Deletes any resources allocated by a DiomedeCmdLine object.
		 */
		virtual ~DiomedeCmdLine();

		/**
		 * Diomede: check whether this argument is allowed.  For example,
		 * any one command can only contain one unlabeled argument.
		 * \param req - Argument is required
		 * \param argName - argument name
		 */
		void check( bool req, const std::string& argName );

		/**
		 * Parses the command line.
		 * \param argc - Number of arguments.
		 * \param argv - Array of arguments.
		 */
		void parse(int argc, char** argv);

		/**
		 * Diomede: Parses the command line.
		 * Parses the command line.
		 * \param argc - Number of arguments.
		 * \param args - Array of command and arguments.
		 * \param bSuccess - true if parse is successful, false otherwise
		 */
		virtual void parse(int argc, std::vector<std::string>& args, bool& bSuccess);
};

/**
 * Typedef of an Alternate command list iterator.
 */
typedef std::list<string>::iterator AltCmdListIterator;

///////////////////////////////////////////////////////////////////////////////
// DiomedeCmdLine

/**
 * Constructor
 */
inline DiomedeCmdLine::DiomedeCmdLine(const std::string& message,
				        char delimiter,
						const std::string& version,
						bool helpAndVersion )
: CmdLine(message, delimiter, version, helpAndVersion)
{
}

/**
 * Diomede: constructor with the additon of the command name.
 */
inline DiomedeCmdLine::DiomedeCmdLine(const std::string& command,
                        const std::string& message,
				        char delimiter,
				        bool userPrompts,
						const std::string& version,
						bool helpAndVersion )
: CmdLine(command, message, delimiter, userPrompts, version, helpAndVersion)
{
}

/**
 * Destructor
 */
inline DiomedeCmdLine::~DiomedeCmdLine()
{
}


/**
 * Diomede: check whether this argument is allowed.  For example,
 * any one command can only contain one unlabeled optional argument.
 * \param req - Argument is required
 * \param argName - argument name
 */
inline void DiomedeCmdLine::check( bool req, const std::string& argName )
{
    try {
        _unlabeledTracker.check(req, argName);
    }
    catch ( SpecificationException s) { _output->failure(*this,s); }
}

/**
 * Parses the command line.
 */
inline void DiomedeCmdLine::parse(int argc, char** argv)
{
	try {

	_progName = argv[0];

	// this step is necessary so that we have easy access to mutable strings.
	std::vector<std::string> args;
  	for (int i = 1; i < argc; i++)
		args.push_back(argv[i]);

	int requiredCount = 0;

  	for (int i = 1; static_cast<unsigned int>(i) < args.size(); i++)
	{
		bool matched = false;
		for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++)
        {
			if ( (*it)->processArg( &i, args ) )
			{
				requiredCount += _xorHandler.check( *it );
				matched = true;
				break;
			}
        }

		// checks to see if the argument is an empty combined switch ...
		// and if so, then we've actually matched it
		if ( !matched && _emptyCombined( args[i] ) )
			matched = true;

		if ( !matched && !Arg::ignoreRest() )
			throw(CmdLineParseException("Couldn't find match for argument",
			                             args[i]));
    }

	if ( requiredCount < _numRequired )
		throw(CmdLineParseException("One or more required arguments missing!"));

	if ( requiredCount > _numRequired )
		throw(CmdLineParseException("Too many arguments!"));

	} catch ( ArgException e ) { _output->failure(*this,e); exit(1); }
}

/**
 * Diomede: Parses the command line.  Handles specific commands such as REM
 * \param argc - Number of arguments.
 * \param args - Array of command and arguments.
 */
inline void DiomedeCmdLine::parse(int argc, std::vector<std::string>& args, bool& bSuccess)
{
	try {

    bSuccess = true;

	_commandName = args[0];

	int requiredCount = 0;

	// If the user has asked for either help or version information, the
	// visitor will set the flag for the command.

	_foundHelpOrVersionSwitch = false;

  	for (int i = 1; static_cast<unsigned int>(i) < args.size(); i++)
	{
		bool matched = false;
		for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++)
        {
            Arg* tmpArg = (Arg*)(*it);
			if ( tmpArg->processArg( &i, args ) )
			{
				requiredCount += _xorHandler.check( *it );
				matched = true;
				break;
			}
        }

		// checks to see if the argument is an empty combined switch ...
		// and if so, then we've actually matched it
		if ( !matched && _emptyCombined( args[i] ) )
			matched = true;

        // REM: ignore everything except /?
	    for( AltCmdListIterator it = _altCommandList.begin(); it != _altCommandList.end(); it++ ) {
		    if ( CMD_REM == (*it) ) {
			    matched = true;
			    break;
			}
        }

        // Not a parse error if the user usess the -h argument.
	    if ( !_foundHelpOrVersionSwitch && !matched && !Arg::ignoreRest() )
		    throw(CmdLineParseException("Couldn't find match for argument",
		                                 args[i]));
    }

    // Not a parse error if the user usess the /? argument.
    // Also not a parse error if the user will be prompted for missing
    // arguments.
	if ( _userPrompts && !_foundHelpOrVersionSwitch && ( requiredCount < _numRequired ) )
		throw(CmdLineParseException("One or more required arguments missing!"));

	if ( requiredCount > _numRequired )
		throw(CmdLineParseException("Too many arguments!"));

	}
	catch ( ArgException e ) {
	    _output->failure(*this,e);
	    bSuccess = false; /*exit(1);*/
	}
}


} // namespace TCLAP

#endif // __DIOMEDE_CMDLINE_H__

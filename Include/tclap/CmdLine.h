
/******************************************************************************
 *
 *  file:  CmdLine.h
 *
 *  Copyright (c) 2003, Michael E. Smoot .
 *  Copyright (c) 2004, Michael E. Smoot, Daniel Aarno.
 *  All rights reserved.
 *
 *  See the file COPYING in the top directory of this distribution for
 *  more information.
 *
 *  THE SOFTWARE IS PROVIDED _AS IS_, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#ifndef TCLAP_CMDLINE_H
#define TCLAP_CMDLINE_H

#include "SwitchArg.h"
#include "MultiSwitchArg.h"
#include "UnlabeledValueArg.h"
#include "UnlabeledMultiArg.h"

#include "XorHandler.h"
#include "HelpVisitor.h"
#include "VersionVisitor.h"
#include "IgnoreRestVisitor.h"

#include "CmdLineOutput.h"
#include "StdOutput.h"

#include "Constraint.h"
#include "ValuesConstraint.h"

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "../types.h"

namespace TCLAP {

/**
 * The base class that manages the command line definition and passes
 * along the parsing to the appropriate Arg classes.
 */
class CmdLine : public CmdLineInterface
{
	protected:

		/**
		 * The list of arguments that will be tested against the
		 * command line.
		 */
		std::list<Arg*> _argList;

		/**
		 * Diomede: The list of alternate commands that can be used for this
		 * command.
		 */
		std::list<string> _altCommandList;

		/**
		 * The name of the program.  Set to argv[0].
		 * Diomede: for our purposes, program name may or may not be the
		 * the first argument.
		 */
		std::string _progName;

		/**
		 * Diomede: The name of the command.
		 */
		std::string _commandName;

		/**
		 * A message used to describe the program or command.  Used in the usage output.
		 */
		std::string _message;

		/**
		 * The version to be displayed with the --version switch.
		 */
		std::string _version;

		/**
		 * The number of arguments that are required to be present on
		 * the command line. This is set dynamically, based on the
		 * Args added to the CmdLine object.
		 */
		int _numRequired;

		/**
		 * The character that is used to separate the argument flag/name
		 * from the value.  Defaults to ' ' (space).
		 */
		char _delimiter;

		/**
		 * The handler that manages xoring lists of args.
		 */
		XorHandler _xorHandler;

		/**
		 * A list of Args to be explicitly deleted when the destructor
		 * is called.  At the moment, this only includes the three default
		 * Args.
		 */
		std::list<Arg*> _argDeleteOnExitList;

		/**
		 * A list of Visitors to be explicitly deleted when the destructor
		 * is called.  At the moment, these are the Vistors created for the
		 * default Args.
		 */
		std::list<Visitor*> _visitorDeleteOnExitList;

		/**
		 * Object that handles all output for the CmdLine.
		 */
		CmdLineOutput* _output;

		/**
		 * Used to verify whether this command already has a unlabeled
		 * argument.
		 * Diomede: added to allow usage of mulitple commands with unlabeled
		 * arguments.
		 */
		OptionalUnlabeledTracker _unlabeledTracker;

		/**
		 * Checks whether a name/flag string matches entirely matches
		 * the Arg::blankChar.  Used when multiple switches are combined
		 * into a single argument.
		 * \param s - The message to be used in the usage.
		 */
		bool _emptyCombined(const std::string& s);

    public:
        /**
         * Diomede - provide public access to these methods
         */

		/**
		 * Perform a delete ptr; operation on ptr when this object is deleted.
		 */
		void deleteOnExit(Arg* ptr);

		/**
		 * Perform a delete ptr; operation on ptr when this object is deleted.
		 */
		void deleteOnExit(Visitor* ptr);

	protected:

		/**
		 * Encapsulates the code common to the constructors (which is all
		 * of it).
		 */
		void _constructor();

		/**
		 * Is set to true when a user sets the output object. We use this so
		 * that we don't delete objects that are created outside of this lib.
		 */
		bool _userSetOutput;

		/**
		 * Whether or not to automatically create help and version switches.
		 */
		bool _helpAndVersion;

        /**
         * Diomede: Indicates whether or not help is available for the command.
         */
		bool _commandHelp;

        /**
         * Diomede: Store the help visitor that is associated with each command.
         */
		HelpVisitor* _helpVisitor;

		/**
		 * Diomede: Set to true when the user types -h or --help
		 */
		bool _foundHelpOrVersionSwitch;

		/**
		 * Diomede: Whether or not to use the ignore switch.  Defaults to false.
		 */
		bool _ignoreArg;

		/**
		 * Diomede: User will be prompted for missing required arguments.
		 */
		bool _userPrompts;

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
		CmdLine(const std::string& message,
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
		CmdLine(const std::string& command,
		        const std::string& message,
				const char delimiter = ' ',
				bool userPrompts = false,
				const std::string& version = "none",
				bool helpAndVersion = false);

		/**
		 * Deletes any resources allocated by a CmdLine object.
		 */
		virtual ~CmdLine();

		/**
		 * Diomede: check whether this argument is allowed.  For example,
		 * any one command can only contain one unlabeled argument.
		 * \param req - Argument is required
		 * \param argName - argument name
		 */
		void check( bool req, const std::string& argName );

		/**
		 * Diomede: Following command usage, reset data as needed.,
		 */
		void resetArgs();

		/**
		 * Diomede: Following command usage, reset data as needed.,
		 */
		void resetValues();

		/**
		 * Diomede: Following command usage, reset the reprompt count.
		 * Should be called once the command is done with the current
		 * argument.
		 */
		void resetRepromptCount();

		/**
		 * Adds an argument to the list of arguments to be parsed.
		 * \param a - Argument to be added.
		 */
		void add( Arg& a );

		/**
		 * An alternative add.  Functionally identical.
		 * \param a - Argument to be added.
		 */
		void add( Arg* a );

		/**
		 * Diomede: Adds a command to the list of alternate commands.
		 * \param szAltCommand - Command to be added.
		 */
		void addAltCommand( string szAltCommand);

		/**
		 * Add two Args that will be xor'd.  If this method is used, add does
		 * not need to be called.
		 * \param a - Argument to be added and xor'd.
		 * \param b - Argument to be added and xor'd.
		 */
		void xorAdd( Arg& a, Arg& b );

		/**
		 * Add a list of Args that will be xor'd.  If this method is used,
		 * add does not need to be called.
		 * \param xors - List of Args to be added and xor'd.
		 */
		void xorAdd( std::vector<Arg*>& xors );

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

		/**
		 *
		 */
		CmdLineOutput* getOutput();

		/**
		 *
		 */
		void setOutput(CmdLineOutput* co);

		/**
		 *
		 */
		std::string& getVersion();

		/**
		 *
		 */
		std::string& getProgramName();

		/**
		 *
		 */
		void setProgramName(const std::string& progName);

		/**
		 *
		 */
		std::string& getCommandName();

		/**
		 *
		 */
		std::list<Arg*>& getArgList();

		/**
		 * Diomede: Returns the list of alternate commands.
		 */
		std::list<std::string>* getAltCommmandList();

		/**
		 * Diomede: Returns named argument
		 */
		Arg* getArg(std::string argName);

		/**
		 *
		 */
		XorHandler& getXorHandler();

		/**
		 *
		 */
		char getDelimiter();

		/**
		 *
		 */
		std::string& getMessage();

		/**
		 *
		 */
        OptionalUnlabeledTracker& getUnlabeledTracker();

		/**
		 *
		 */
		bool hasHelpAndVersion();

        /**
         */
        bool hasCommandHelp();

        /**
         */
        HelpVisitor* getHelpVisitor();

		/**
		 *
		 */
        bool hasIgnore();

		/**
		 *
		 */
        bool hasUserPrompts();

		/**
		 *
		 */
		bool foundHelpOrVersionSwitch();

		/**
		 *
		 */
		void usedHelpOrVersionSwitch(const bool& helpAndVersion);

};

/**
 * Typedef of an Alternate command list iterator.
 */
typedef std::list<string>::iterator AltCmdListIterator;

///////////////////////////////////////////////////////////////////////////////
//Begin CmdLine.cpp
///////////////////////////////////////////////////////////////////////////////

inline CmdLine::CmdLine(const std::string& m,
				        char delim,
						const std::string& v,
						bool help )
: _progName(_T("")),
  _commandName(_T("")),
  _message(m),
  _version(v),
  _numRequired(0),
  _delimiter(delim),
  _output(0),
  _userSetOutput(false),
  _helpAndVersion(help),
  _commandHelp(false),
  _helpVisitor(0),
  _foundHelpOrVersionSwitch(false),
  _ignoreArg(true),
  _userPrompts(false)
{
	_constructor();
}

// Diomede: constructor with the additon of the command name.
inline CmdLine::CmdLine(const std::string& c,
                        const std::string& m,
				        char delim,
				        bool userPrompts,
						const std::string& v,
						bool help )
: _progName(_T("DioCLI")),
  _commandName(c),
  _message(m),
  _version(v),
  _numRequired(0),
  _delimiter(delim),
  _output(0),
  _userSetOutput(false),
  _helpAndVersion(help),
  _commandHelp(true),
  _helpVisitor(0),
  _foundHelpOrVersionSwitch(false),
  _ignoreArg(false),
  _userPrompts(userPrompts)
{
	_constructor();
}

inline CmdLine::~CmdLine()
{
	ArgListIterator argIter;
	VisitorListIterator visIter;

	for( argIter = _argDeleteOnExitList.begin();
		 argIter != _argDeleteOnExitList.end();
		 ++argIter)
		delete *argIter;

	for( visIter = _visitorDeleteOnExitList.begin();
		 visIter != _visitorDeleteOnExitList.end();
		 ++visIter)
		delete *visIter;

	if ( !_userSetOutput )
		delete _output;
}

inline void CmdLine::_constructor()
{
	_output = new StdOutput;

	Arg::setDelimiter( _delimiter );

	Visitor* v = NULL;

	if ( _helpAndVersion )
	{
		v = new HelpVisitor( this, &_output );
		SwitchArg* help = new SwitchArg("h","help",
						"Displays usage information and exits.",
						false, v);
		add( help );
		deleteOnExit(help);
		deleteOnExit(v);

		v = new HelpVisitor( this, &_output );
		SwitchArg* helpAlt = new SwitchArg("?","?",
						"Displays usage information and exits.",
						false, v);
		add( helpAlt );
		deleteOnExit(helpAlt);
		deleteOnExit(v);

		v = new VersionVisitor( this, &_output );
		SwitchArg* vers = new SwitchArg("","version",
					"Displays version information and exits.",
					false, v);
		add( vers );
		deleteOnExit(vers);
		deleteOnExit(v);
	}
	else if (_commandHelp) {
	    // Diomede: help is added on a command basis, as opposed to arguments
		_helpVisitor = new HelpVisitor( this, &_output );

		SwitchArg* helpAlt = new SwitchArg("?","?",
						"Displays usage information.",
						false, _helpVisitor);
		add( helpAlt );
		deleteOnExit(helpAlt);

		deleteOnExit(_helpVisitor);
	}

    if (_ignoreArg) {
	    v = new IgnoreRestVisitor();
	    SwitchArg* ignore  = new SwitchArg(Arg::flagStartString(),
					       Arg::ignoreNameString(),
			               "Ignores the rest of the labeled arguments following this flag.",
					       false, v);
	    add( ignore );
	    deleteOnExit(ignore);
	    deleteOnExit(v);
	}
}

inline void CmdLine::xorAdd( std::vector<Arg*>& ors )
{
	_xorHandler.add( ors );

	for (ArgVectorIterator it = ors.begin(); it != ors.end(); it++)
	{
		(*it)->forceRequired();
		(*it)->setRequireLabel( "OR required" );

		add( *it );
	}
}

inline void CmdLine::xorAdd( Arg& a, Arg& b )
{
    std::vector<Arg*> ors;
    ors.push_back( &a );
    ors.push_back( &b );
	xorAdd( ors );
}

inline void CmdLine::add( Arg& a )
{
	add( &a );
}

inline void CmdLine::add( Arg* a )
{
	for( ArgListIterator it = _argList.begin(); it != _argList.end(); it++ )
		if ( *a == *(*it) )
			throw( SpecificationException(
			       	"Argument with same flag/name already exists!",
					a->longID() ) );

	a->addToList( _argList );

	if ( a->isRequired() )
		_numRequired++;
}

/**
 * Diomede: Adds a command to the list of alternate commands.
 * \param szAltCommand - Command to be added.
 */
inline void CmdLine::addAltCommand( string szAltCommand )
{
	for( AltCmdListIterator it = _altCommandList.begin(); it != _altCommandList.end(); it++ )
		if ( szAltCommand == (*it) )
			throw( SpecificationException(
			       	"Command with same name already exists!",
					szAltCommand ) );

	_altCommandList.push_back(szAltCommand);
}


/**
 * Diomede: check whether this argument is allowed.  For example,
 * any one command can only contain one unlabeled optional argument.
 * \param req - Argument is required
 * \param argName - argument name
 */
inline void CmdLine::check( bool req, const std::string& argName )
{
    try {
        _unlabeledTracker.check(req, argName);
    }
    catch ( SpecificationException s) { _output->failure(*this,s); }
}

/**
 * Diomede: Following command usage, reset data as needed.
 */
inline void CmdLine::resetArgs()
{
	for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++)
    {
        Arg* tmpArg = (Arg*)(*it);
        tmpArg->resetArg();
    }
}

/**
 * Diomede: Following command usage, reset data as needed.,
 */
inline void CmdLine::resetValues()
{
	for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++)
    {
        Arg* tmpArg = (Arg*)(*it);
        tmpArg->resetValue();
    }
}

/**
 * Diomede: Following command usage, reset the reprompt count.
 * Should be called once the command is done with the current
 * argument.
 */
inline void CmdLine::resetRepromptCount()
{
	for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++)
    {
        Arg* tmpArg = (Arg*)(*it);
        tmpArg->resetRepromptCount();
    }
}

/**
 * Parses the command line.
 */
inline void CmdLine::parse(int argc, char** argv)
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
 * Diomede: Parses the command line.
 * Parses the command line.
 * \param argc - Number of arguments.
 * \param args - Array of command and arguments.
 */
inline void CmdLine::parse(int argc, std::vector<std::string>& args, bool& bSuccess)
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

        // Not a parse error if the user usess the -h argument.
		if ( !_foundHelpOrVersionSwitch && !matched && !Arg::ignoreRest() )
			throw(CmdLineParseException("Couldn't find match for argument",
			                             args[i]));
    }

    // Not a parse error if the user usess the -h argument.
    // Also not a parse error if the user will be prompted for missing
    // arguments.
	if ( _userPrompts && !_foundHelpOrVersionSwitch && ( requiredCount < _numRequired ) )
		throw(CmdLineParseException("One or more required arguments missing!"));

	if ( requiredCount > _numRequired )
		throw(CmdLineParseException("Too many arguments!"));

	}
	catch ( ArgException e ) {
	    _output->cmdFailure(*this,e);
	    bSuccess = false; /*exit(1);*/
	}
}

/**
 */
inline bool CmdLine::_emptyCombined(const std::string& s)
{
	if ( s[0] != Arg::flagStartChar() )
		return false;

	for ( int i = 1; static_cast<unsigned int>(i) < s.length(); i++ )
		if ( s[i] != Arg::blankChar() )
			return false;

	return true;
}

/**
 */
inline void CmdLine::deleteOnExit(Arg* ptr)
{
	_argDeleteOnExitList.push_back(ptr);
}

/**
 */
inline void CmdLine::deleteOnExit(Visitor* ptr)
{
	_visitorDeleteOnExitList.push_back(ptr);
}

/**
 */
inline CmdLineOutput* CmdLine::getOutput()
{
	return _output;
}

/**
  * Diomede: Modified to cleanup _output if it exists.  Otherwise,
  * we have a memory leak.
 */
inline void CmdLine::setOutput(CmdLineOutput* co)
{
	if (0 != _output) {
	    delete _output;
	    _output = 0;
	}

	_userSetOutput = true;
	_output = co;
}

inline std::string& CmdLine::getVersion()
{
	return _version;
}

inline std::string& CmdLine::getProgramName()
{
	return _progName;
}

/**
  * Diomede: Added to allow usage of this parser for command entry.
  * Code assumes that the program name is the first entry - we need the
  * parser to also allow for commands to appear as the first entry.
 */
inline void CmdLine::setProgramName(const std::string& progName)
{
	_progName = progName;
}

inline std::string& CmdLine::getCommandName()
{
	return _commandName;
}

inline std::list<Arg*>& CmdLine::getArgList()
{
	return _argList;
}


/**
 * Diomede: Returns the list of alternate commands.
 */
inline std::list<std::string>* CmdLine::getAltCommmandList()
{
	return &_altCommandList;
}

/**
 * Diomemde: Returns named argument
 */
inline Arg* CmdLine::getArg(std::string argName)
{
	for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++)
    {
        Arg* tmpArg = (Arg*)(*it);
        if (tmpArg->getName() == argName) {
            return tmpArg;
        }
    }

    // No argument found with the given name.
	throw(CmdLineParseException("Couldn't find match for argument", argName));
}

inline XorHandler& CmdLine::getXorHandler()
{
	return _xorHandler;
}

inline char CmdLine::getDelimiter()
{
	return _delimiter;
}

inline std::string& CmdLine::getMessage()
{
	return _message;
}

/**
 * Used to verify whether this command already has a unlabeled
 * argument.
 * Diomede: added to allow usage of mulitple commands with unlabeled
 * arguments.
 */
inline OptionalUnlabeledTracker& CmdLine::getUnlabeledTracker()
{
	return _unlabeledTracker;
}

inline bool CmdLine::hasHelpAndVersion()
{
	return _helpAndVersion;
}

/**
 * Diomede: Indicates whether or not help is available for the command.
 */
inline bool CmdLine::hasCommandHelp()
{
    return _commandHelp;
}

/**
 * Diomede: Access to the help visitor
 */
inline HelpVisitor* CmdLine::getHelpVisitor()
{
	return _helpVisitor;
}

/**
 * Diomede: indicates whether the ignore switch is used.
 */
inline bool CmdLine::hasIgnore()
{
	return _ignoreArg;
}

/**
 * Diomede: indicates whether the user will be prompted for required
   arguments.
 */
inline bool CmdLine::hasUserPrompts()
{
	return _userPrompts;
}

/**
 * Diomede: Allows the user of this command to determine if the -h flag has
 * been used - e.g. if used with exit, determine whether or not to exit.
 */
inline bool CmdLine::foundHelpOrVersionSwitch()
{
	return _foundHelpOrVersionSwitch;
}

/**
 * Diomede: Set by the argument handler for this command when the
 * -h or -v switch has been used.  (TBD: remove these switches entirely).
 */
inline void CmdLine::usedHelpOrVersionSwitch(const bool& foundHelpOrVersionSwitch)
{
    _foundHelpOrVersionSwitch = foundHelpOrVersionSwitch;
}

///////////////////////////////////////////////////////////////////////////////
//End CmdLine.cpp
///////////////////////////////////////////////////////////////////////////////



} //namespace TCLAP
#endif

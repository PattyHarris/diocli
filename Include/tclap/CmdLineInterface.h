
/******************************************************************************
 *
 *  file:  CmdLineInterface.h
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

#ifndef TCLAP_COMMANDLINE_INTERFACE_H
#define TCLAP_COMMANDLINE_INTERFACE_H

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <algorithm>


namespace TCLAP {

class Arg;
class CmdLineOutput;
class XorHandler;
class HelpVisitor;

/**
 * The base class that manages the command line definition and passes
 * along the parsing to the appropriate Arg classes.
 */
class CmdLineInterface
{
	public:

		/**
		 * Destructor
		 */
		virtual ~CmdLineInterface() {}

		/**
		 * Diomede: check whether this argument is allowed.  For example,
		 * any one command can only contain one unlabeled argument.
		 * \param req - Argument is required
		 * \param argName - argument name
		 */
		virtual void check( bool req, const std::string& argName )=0;

		/**
		 * Diomede: Following command usage, reset data as needed.,
		 */
		virtual void resetArgs()=0;

		/**
		 * Diomede: Following command usage, reset data as needed.,
		 */
		virtual void resetValues()=0;

		/**
		 * Adds an argument to the list of arguments to be parsed.
		 * \param a - Argument to be added.
		 */
		virtual void add( Arg& a )=0;

		/**
		 * An alternative add.  Functionally identical.
		 * \param a - Argument to be added.
		 */
		virtual void add( Arg* a )=0;

		/**
		 * Diomede: Adds a command to the list of alternate commands.
		 * \param szAltCommand - Command to be added.
		 */
		virtual void addAltCommand( std::string szAltCommand )=0;

		/**
		 * Add two Args that will be xor'd.
		 * If this method is used, add does
		 * not need to be called.
		 * \param a - Argument to be added and xor'd.
		 * \param b - Argument to be added and xor'd.
		 */
		virtual void xorAdd( Arg& a, Arg& b )=0;

		/**
		 * Add a list of Args that will be xor'd.  If this method is used,
		 * add does not need to be called.
		 * \param xors - List of Args to be added and xor'd.
		 */
		virtual void xorAdd( std::vector<Arg*>& xors )=0;

		/**
		 * Parses the command line.
		 * \param argc - Number of arguments.
		 * \param argv - Array of arguments.
		 */
		virtual void parse(int argc, char** argv)=0;

		/**
		 * Parses the command line.
		 * \param argc - Number of arguments.
		 * \param args - Array of command and arguments.
		 * \param bSuccess - true if parse is successful, false otherwise
		 */
		virtual void parse(int argc, std::vector<std::string>& args, bool& bSuccess)=0;

		/**
		 * Returns the CmdLineOutput object.
		 */
		virtual CmdLineOutput* getOutput()=0;

		/**
		 * \param co - CmdLineOutput object that we want to use instead.
		 */
		virtual void setOutput(CmdLineOutput* co)=0;

		/**
		 * Returns the version string.
		 */
		virtual std::string& getVersion()=0;

		/**
		 * Returns the program name string.
		 */
		virtual std::string& getProgramName()=0;

		/**
		 * Sets the program name string
		 */
		virtual void setProgramName(const std::string& progName)=0;

		/**
		 * Returns the command name
		 */
		virtual std::string& getCommandName()=0;

		/**
		 * Returns the argList.
		 */
		virtual std::list<Arg*>& getArgList()=0;

		/**
		 * Diomede: Returns the list of alternate commands.
		 */
		virtual std::list<std::string>* getAltCommmandList()=0;

		/**
		 * Diomede: Returns named argument
		 */
		virtual Arg* getArg(std::string argName)=0;

		/**
		 * Returns the XorHandler.
		 */
		virtual XorHandler& getXorHandler()=0;

		/**
		 * Returns the delimiter string.
		 */
		virtual char getDelimiter()=0;

		/**
		 * Returns the message string.
		 */
		virtual std::string& getMessage()=0;

		/**
		 * Indicates whether or not the help and version switches were created
		 * automatically.
		 */
		virtual bool hasHelpAndVersion()=0;

		/**
		 * Diomede: Indicates whether or not help is available for the command.
		 */
		virtual bool hasCommandHelp()=0;

        /**
         * Diomede: Access to the help visitor
         */
        virtual HelpVisitor* getHelpVisitor()=0;

        /**
         * Diomede: indicates whether the ignore switch is used. Default is false.
         */
        virtual bool hasIgnore()=0;

        /**
         * Diomede: indicates whether the user will be prompted for required
           arguments.
         */
        virtual bool hasUserPrompts()=0;

		/**
		 * Indicates whether or not the help or version switches were used
		 * as part of the command.  Allows the user to view the usage
		 * and, for commands as exit, does not exit the application (TBD).
		 * Flag is set by the appropriate visitor.
		 */
        virtual bool foundHelpOrVersionSwitch()=0;
        virtual void usedHelpOrVersionSwitch(const bool& foundHelpOrVersionSwitch)=0;

};

} //namespace


#endif

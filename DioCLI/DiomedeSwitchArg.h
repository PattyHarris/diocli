/*********************************************************************
 *
 *  file:  DiomedeSwitchArg.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Diomede specific switch arguments.
 *
 *********************************************************************/
#ifndef __DIOMEDE_SWITCH_ARG_H__
#define __DIOMEDE_SWITCH_ARG_H__

#include <string>
#include <vector>

#include <tclap/Arg.h>

namespace TCLAP {

/**
 * A simple switch argument.  If the switch is set on the command line, then
 * the getValue method will return the opposite of the default value for the
 * switch.
 */
class DiomedeSwitchArg : public Arg
{
	protected:

		/**
		 * The value of the switch.
		 */
		bool _value;

	public:

        /**
		 * DiomedeSwitchArg constructor.
		 * \param flag - The one character flag that identifies this
		 * argument on the command line.
		 * \param name - A one word name for the argument.  Can be
		 * used as a long flag on the command line.
		 * \param desc - A description of what the argument is for or
		 * does.
		 * \param _default - The default value for this Switch.
		 * \param v - An optional visitor.  You probably should not
		 * use this unless you have a very good reason.
		 */
		DiomedeSwitchArg(const std::string& flag,
			             const std::string& name,
			             const std::string& desc,
			             bool _default = false,
				         Visitor* v = NULL);


		/**
		 * DiomedeSwitchArg constructor.
		 * \param flag - The one character flag that identifies this
		 * argument on the command line.
		 * \param name - A one word name for the argument.  Can be
		 * used as a long flag on the command line.
		 * \param desc - A description of what the argument is for or
		 * does.
		 * \param parser - A CmdLine parser object to add this Arg to
		 * \param _default - The default value for this Switch.
		 * \param v - An optional visitor.  You probably should not
		 * use this unless you have a very good reason.
		 */
		DiomedeSwitchArg(const std::string& flag,
			             const std::string& name,
			             const std::string& desc,
				         CmdLineInterface& parser,
			             bool _default = false,
				         Visitor* v = NULL);


        /**
		 * Handles the processing of the argument.
		 * This re-implements the Arg version of this method to set the
		 * _value of the argument appropriately.
		 * \param i - Pointer the the current argument in the list.
		 * \param args - Mutable list of strings. Passed
		 * in from main().
		 */
		virtual bool processArg(int* i, std::vector<std::string>& args);

		/**
		 * Diomede: following command usage, reset any arguments.
		 * TCLAP assumes commands allocated on the stack, not the
		 * and therefore, does not account for multiple command usage.
		 */
		virtual void resetArg();

		/**
		 * Diomede: following command usage, reset any argument values.
		 * TCLAP assumes commands allocated on the stack, not the
		 * and therefore, does not account for multiple command usage.
		 */
		virtual void resetValue();

		/**
		 * Returns the a short id string.  Used in the usage.
		 * \param val - value to be used.
		 */
		virtual std::string shortID(const std::string& val="val") const;

		/**
		 * Returns the a long id string.  Used in the usage.
		 * \param val - value to be used.
		 * \param bShowName - Diomede: Display the name flag and value.
		 */
		virtual std::string longID( const std::string& val = "val",
		                            const bool& bShowName = false  ) const;
		/**
		 * Checks a string to see if any of the chars in the string
		 * match the flag for this Switch.
		 */
		bool combinedSwitchesMatch(std::string& combined);

		/**
		 * Returns bool, whether or not the switch has been set.
		 */
		bool getValue();

		/**
		 * Pushes this to back of list rather than front.
		 * \param argList - The list this should be added to.
		 */
		virtual void addToList( std::list<Arg*>& argList ) const;
};

//////////////////////////////////////////////////////////////////////
//BEGIN DiomedeSwitchArg.cpp
//////////////////////////////////////////////////////////////////////
/**
 * Constructor implementation.
 */
inline DiomedeSwitchArg::DiomedeSwitchArg(const std::string& flag,
	 		                              const std::string& name,
     		   		                      const std::string& desc,
	     	    	                      bool _default,
					                      Visitor* v )
: Arg(flag, name, desc, false, false, v),
  _value( _default )
{
}

/**
 * Constructor implementation.
 */
inline DiomedeSwitchArg::DiomedeSwitchArg(const std::string& flag,
					                     const std::string& name,
					                     const std::string& desc,
					                     CmdLineInterface& parser,
					                     bool _default,
					                     Visitor* v )
: Arg(flag, name, desc, false, false, v),
  _value( _default )
{
	parser.add( this );
}

/**
 * Implementation of getValue
 */
inline bool DiomedeSwitchArg::getValue()
{
    return _value;
}

/**
 * Implementation of combinedSwitchesMatch
 */
 inline bool DiomedeSwitchArg::combinedSwitchesMatch(std::string& combinedSwitches )
{
    // For Diomede, we'll always return false - there's no way for us to distinquish
    // between a combined switch and our definition of value arguments.  That is, we
    // cannot distinquish between /filename=blah and -abc where -abc is really -a -b -c.
    // The main problem here is trying to solve /dir=filename=log1.?xt where ? is a
    // wildcard.
    return false;

	// make sure this is actually a combined switch
	if ( combinedSwitches[0] != Arg::flagStartString()[0] )
		return false;

	// make sure it isn't a long name
	if ( combinedSwitches.substr( 0, Arg::nameStartString().length() ) ==
		 Arg::nameStartString() )
		return false;

	// ok, we're not specifying a ValueArg, so we know that we have
	// a combined switch list.
	for ( unsigned int i = 1; i < combinedSwitches.length(); i++ )
		if ( combinedSwitches[i] == _flag[0] )
		{
			// update the combined switches so this one is no longer present
			// this is necessary so that no unlabeled args are matched
			// later in the processing.
			//combinedSwitches.erase(i,1);
			combinedSwitches[i] = Arg::blankChar();
			return true;
		}

	// none of the switches passed in the list match.
	return false;
}

/**
 * Implementation of processArg
 */
inline bool DiomedeSwitchArg::processArg(int *i, std::vector<std::string>& args)
{
	if ( _ignoreable && Arg::ignoreRest() )
		return false;

	if ( argMatches( args[*i] ) || combinedSwitchesMatch( args[*i] ) )
	{
		// If we match on a combined switch, then we want to return false
		// so that other switches in the combination will also have a
		// chance to match.
		bool ret = false;
		if ( argMatches( args[*i] ) )
			ret = true;

		if ( _alreadySet || ( !ret && combinedSwitchesMatch( args[*i] ) ) )
			throw(CmdLineParseException("Argument already set!", toString()));

		_alreadySet = true;

		if ( _value == true )
			_value = false;
		else
			_value = true;

		_checkWithVisitor();

		return ret;
	}
	else
		return false;
}

/**
 * Diomede: following command usage, reset any arguments.
 * TCLAP assumes commands allocated on the stack, not the
 * and therefore, does not account for multiple command usage.
 */
inline void DiomedeSwitchArg::resetArg()
{
    _alreadySet = false;
}

/**
 * Diomede: following command usage, reset any arguments.
 * TCLAP assumes commands allocated on the stack, not the
 * and therefore, does not account for multiple command usage.
 */
inline void DiomedeSwitchArg::resetValue()
{
    _value = false;
}

/**
 * Implementation of shortID.
 */
inline std::string DiomedeSwitchArg::shortID(const std::string& val) const
{
	std::string szID = "";

	if ( _flag != "" ) {
		szID = Arg::flagStartChar() + _flag;
    }
	else {
		szID = Arg::nameStartString() + _name;
    }

	if ( !_required )
		szID = "[" + szID + "]";

	return szID;
}

/**
 * Implementation of longID.
 */
inline std::string DiomedeSwitchArg::longID(const std::string& val, const bool& bShowName /*false*/ ) const
{
	std::string szID = "";

	if ( _flag != "" )
	{
		szID += flagStartChar() + _flag;

		if (bShowName) {
		    szID += ",  ";
		}
	}

	return szID;
}

/**
 * Implementation of addToList
 * Overridden by Args that need to added to the end of the list.
 */
inline void DiomedeSwitchArg::addToList( std::list<Arg*>& argList ) const
{
	argList.push_back( const_cast<Arg*>(static_cast<const Arg* const>(this)) );
}

//////////////////////////////////////////////////////////////////////
//End DiomedeSwitchArg.cpp
//////////////////////////////////////////////////////////////////////

} //namespace TCLAP

#endif //__DIOMEDE_SWITCH_ARG_H__

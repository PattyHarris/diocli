/*********************************************************************
 *
 *  file:  DiomedeMultiArg.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Diomede specific value arguments, specifically to
 *          allow for flag arguments using mulitple /argument=value where
 *          "agument" is a string of any length.
 *
 *********************************************************************/

#ifndef __DIOMEDE_MULTI_ARGUMENT_H__
#define __DIOMEDE_MULTI_ARGUMENT_H__

#include <string>
#include <vector>

#include "../Include/tclap/Arg.h"
#include "../Include/tclap/Constraint.h"
#include "../Util/XString.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define HAVE_SSTREAM
#endif

#if defined(HAVE_SSTREAM)
#include <sstream>
#elif defined(HAVE_STRSTREAM)
#include <strstream>
#else
#error "Need a stringstream (sstream or strstream) to compile!"
#endif

namespace TCLAP {

template<class T> class DiomedeMultiArg;

namespace DIOMEDE_MULTI_ARG_HELPER {

enum Error_e { EXTRACT_FAILURE = 1000, EXTRACT_TOO_MANY };

/**
 * This class is used to extract a value from an argument.
 * It is used because we need a special implementation to
 * deal with std::string and making a specialiced function
 * puts it in the T segment, thus generating link errors.
 * Having a specialiced class makes the symbols weak.
 * This is not pretty but I don't know how to make it
 * work any other way.
 */
template<class T>
class DiomedeValueExtractor
{
	friend class DiomedeMultiArg<T>;

	private:

		/**
		 * Reference to the vector of values where the result of the
		 * extraction will be put.
		 */
   		std::vector<T> &_values;

		/**
		 * Constructor.
		 * \param values - Where the values extracted will be put.
		 */
		DiomedeValueExtractor(std::vector<T> &values) : _values(values) {}

		/**
		 * Method that will attempt to parse the input stream for values
		 * of type T.
		 * \param val - Where the values parsed will be put.
		 */
		int extractValue( const std::string& val )
		{
			T temp;

#if defined(HAVE_SSTREAM)
			std::istringstream is(val);
#elif defined(HAVE_STRSTREAM)
			std::istrstream is(val.c_str());
#else
#error "Need a stringstream (sstream or strstream) to compile!"
#endif

			int valuesRead = 0;

			while ( is.good() )
			{
				if ( is.peek() != EOF )
					is >> temp;
				else
					break;

				valuesRead++;
			}

			if ( is.fail() )
				return EXTRACT_FAILURE;

			if ( valuesRead > 1 )
				return EXTRACT_TOO_MANY;

			_values.push_back(temp);

			return 0;
		}
};

/**
 * Specialization for string.  This is necessary because istringstream
 * operator>> is not able to ignore spaces...  meaning -x "X Y" will only
 * read 'X'... and thus the specialization.
 */
template<>
class DiomedeValueExtractor<std::string>
{
	friend class DiomedeMultiArg<std::string>;

   	private:

		/**
		 * Reference to the vector of strings where the result of the
		 * extraction will be put.
		 */
        std::vector<std::string> &_values;

		/**
		 * Constructor.
		 * \param values - Where the strings extracted will be put.
		 */
        DiomedeValueExtractor(std::vector<std::string> &values) : _values(values) {}

		/**
		 * Method that will attempt to parse the input stream for values
		 * of type std::string.
		 * \param val - Where the values parsed will be put.
		 */
        int extractValue( const std::string& val )
		{
            _values.push_back( val );
            return 0;
        }
};

} //namespace DIOMEDE_MULTI_ARG_HELPER

/**
 * An argument that allows multiple values of type T to be specified.  Very
 * similar to a ValueArg, except a vector of values will be returned
 * instead of just one.
 */
template<class T>
class DiomedeMultiArg : public Arg
{
    private:
		/**
		 * The delimiter that separates an argument flag/name from the
		 * value.
		 */
		static char& delimiterRef() { static char delim = '='; return delim; }

	protected:

		/**
		 * The list of values parsed from the CmdLine.
		 */
		std::vector<T> _values;

		/**
		 * The description of type T to be used in the usage.
		 */
		std::string _typeDesc;

		/**
		 * A list of constraint on this Arg.
		 */
		Constraint<T>* _constraint;

		/**
		 * Extracts the value from the string.
		 * Attempts to parse string as type T, if this fails an exception
		 * is thrown.
		 * \param val - The string to be read.
		 */
		void _extractValue( const std::string& val );

		/**
		 *
		 */
		bool _allowMore;

		/**
		 * Diomede: conver the extracted value to lower case.
		 */
		bool _useLowerCase;

		/**
		 * Diomede: argument is valid (e.g. set when the value has been
		 *          validated by the client.
		 */
		bool _isValidated;

	public:

   		/**
		 * Constructor.
		 * \param flag - The one character flag that identifies this
		 * argument on the command line.
		 * \param name - A one word name for the argument.  Can be
		 * used as a long flag on the command line.
		 * \param desc - A description of what the argument is for or
		 * does.
		 * \param req - Whether the argument is required on the command
		 * line.
		 * \param typeDesc - A short, human readable description of the
		 * type that this object expects.  This is used in the generation
		 * of the USAGE statement.  The goal is to be helpful to the end user
		 * of the program.
		 * \param v - An optional visitor.  You probably should not
		 * use this unless you have a very good reason.
		 */
		DiomedeMultiArg( const std::string& flag,
                         const std::string& name,
                         const std::string& desc,
                         bool req,
                         const std::string& typeDesc,
                         Visitor* v = NULL);

		/**
		 * Constructor.
		 * \param flag - The one character flag that identifies this
		 * argument on the command line.
		 * \param name - A one word name for the argument.  Can be
		 * used as a long flag on the command line.
		 * \param desc - A description of what the argument is for or
		 * does.
		 * \param req - Whether the argument is required on the command
		 * line.
		 * \param typeDesc - A short, human readable description of the
		 * type that this object expects.  This is used in the generation
		 * of the USAGE statement.  The goal is to be helpful to the end user
		 * of the program.
		 * \param parser - A CmdLine parser object to add this Arg to
		 * \param v - An optional visitor.  You probably should not
		 * use this unless you have a very good reason.
		 */
		DiomedeMultiArg( const std::string& flag,
                         const std::string& name,
                         const std::string& desc,
                         bool req,
                         const std::string& typeDesc,
                         CmdLineInterface& parser,
                         Visitor* v = NULL );

		/**
		 * Constructor.
		 * \param flag - The one character flag that identifies this
		 * argument on the command line.
		 * \param name - A one word name for the argument.  Can be
		 * used as a long flag on the command line.
		 * \param desc - A description of what the argument is for or
		 * does.
		 * \param req - Whether the argument is required on the command
		 * line.
		 * \param constraint - A pointer to a Constraint object used
		 * to constrain this Arg.
		 * \param v - An optional visitor.  You probably should not
		 * use this unless you have a very good reason.
		 */
		DiomedeMultiArg( const std::string& flag,
                         const std::string& name,
                         const std::string& desc,
                         bool req,
                         Constraint<T>* constraint,
                         Visitor* v = NULL );

		/**
		 * Constructor.
		 * \param flag - The one character flag that identifies this
		 * argument on the command line.
		 * \param name - A one word name for the argument.  Can be
		 * used as a long flag on the command line.
		 * \param desc - A description of what the argument is for or
		 * does.
		 * \param req - Whether the argument is required on the command
		 * line.
		 * \param constraint - A pointer to a Constraint object used
		 * to constrain this Arg.
		 * \param parser - A CmdLine parser object to add this Arg to
		 * \param v - An optional visitor.  You probably should not
		 * use this unless you have a very good reason.
		 */
		DiomedeMultiArg( const std::string& flag,
                         const std::string& name,
                         const std::string& desc,
                         bool req,
                         Constraint<T>* constraint,
                         CmdLineInterface& parser,
                         Visitor* v = NULL );

        /**
         * DiomedeMultiArg destructor.   Added to destroy allocated
         * constraints.
        */
    	virtual ~DiomedeMultiArg();

		/**
		 * Handles the processing of the argument.
		 * This re-implements the Arg version of this method to set the
		 * _value of the argument appropriately.  It knows the difference
		 * between labeled and unlabeled.
		 * \param i - Pointer the the current argument in the list.
		 * \param args - Mutable list of strings. Passed from main().
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
		 * Returns a vector of type T containing the values parsed from
		 * the command line.
		 */
		const std::vector<T>& getValue();

		/**
		 * Diomede: Indicates whether lower or upper case can be used on
		 * the extracted values.
		 */
		virtual void useLowerCase(const bool& bUseLowerCase) { _useLowerCase = bUseLowerCase; }

		/**
		 * Diomede: Normally is set to true, but set to false until validated
		 * by the client (e.g. dates, billing data).
		 */
		virtual void isValidated(const bool& bIsValidated) { _isValidated = bIsValidated; }

		/**
		 * Diomede: Returns if the argument has been validated.
		 */
		virtual bool isValidated() { return _isValidated; }

 		/**
		 * The delimiter that separates an argument flag/name from the
		 * value.
		 */
		static char delimiter() { return delimiterRef(); }

		/**
		 * Returns the a short id string.  Used in the usage.
		 * \param val - value to be used.
		 */
		virtual std::string shortID(const std::string& val="val") const;

		/**
		 * Returns the a long id string.  Used in the usage.
		 * \param valueId - The value used in the id.
		 * \param bShowName - Diomede: Display the name flag and value.
		 */
		virtual std::string longID( const std::string& valueId = "val",
		                            const bool& bShowName = false  ) const;

		/**
		 * Once we've matched the first value, then the arg is no longer
		 * required.
		 */
		virtual bool isRequired() const;

		virtual bool allowMore();

		/**
		 * Diomede overridden to add args to the end of the arg list.
		 * Adds this to the specified list of Args.
		 * \param argList - The list to add this to.
		 */
		virtual void addToList( std::list<Arg*>& argList ) const;

		/**
		 * Diomede specific for /argument=value
		 * Trims a value off of the flag.
		 * \param flag - The string from which the flag and value will be
		 * trimmed. Contains the flag once the value has been trimmed.
		 * \param value - Where the value trimmed from the string will
		 * be stored.
		 */
		virtual void trimFlag( std::string& flag, std::string& value ) const;

};

/**
 * Constructor implementation.
 */
template<class T>
DiomedeMultiArg<T>::DiomedeMultiArg(const std::string& flag,
                                    const std::string& name,
                                    const std::string& desc,
                                    bool req,
                                    const std::string& typeDesc,
                                    Visitor* v)
: Arg( flag, name, desc, req, true, v ),
  _typeDesc( typeDesc ),
  _constraint( NULL ),
  _allowMore(false),
  _useLowerCase(true),
  _isValidated(true)
{
	_acceptsMultipleValues = true;
}

/**
 * Constructor implementation.
 */
template<class T>
DiomedeMultiArg<T>::DiomedeMultiArg(const std::string& flag,
                                    const std::string& name,
                                    const std::string& desc,
                                    bool req,
                                    const std::string& typeDesc,
                                    CmdLineInterface& parser,
                                    Visitor* v)
: Arg( flag, name, desc, req, true, v ),
  _typeDesc( typeDesc ),
  _constraint( NULL ),
  _allowMore(false),
  _useLowerCase(true),
  _isValidated(true)
{
	parser.add( this );
	_acceptsMultipleValues = true;
}

/**
 * Constructor implementation.
 */
template<class T>
DiomedeMultiArg<T>::DiomedeMultiArg(const std::string& flag,
                                    const std::string& name,
                                    const std::string& desc,
                                    bool req,
                                    Constraint<T>* constraint,
                                    Visitor* v)
: Arg( flag, name, desc, req, true, v ),
  _typeDesc( constraint->shortID() ),
  _constraint( constraint ),
  _allowMore(false),
  _useLowerCase(true),
  _isValidated(true)
{
	_acceptsMultipleValues = true;
}

/**
 * Constructor implementation.
 */
template<class T>
DiomedeMultiArg<T>::DiomedeMultiArg(const std::string& flag,
                                    const std::string& name,
                                    const std::string& desc,
                                    bool req,
                                    Constraint<T>* constraint,
                                    CmdLineInterface& parser,
                                    Visitor* v)
: Arg( flag, name, desc, req, true, v ),
  _typeDesc( constraint->shortID() ),
  _constraint( constraint ),
  _allowMore(false),
  _useLowerCase(true),
  _isValidated(true)
{
	parser.add( this );
	_acceptsMultipleValues = true;
}

/**
 * DiomedeMultiArg destructor.   Added to destroy allocated
 * constraints.
*/
template<class T>
DiomedeMultiArg<T>::~DiomedeMultiArg()
{
    if (_constraint != NULL) {
        delete _constraint;
        _constraint = NULL;
    }

} // End Destructor

/**
 * Implementation of getValue.
 */
template<class T>
const std::vector<T>& DiomedeMultiArg<T>::getValue()
{
    return _values;
}

/**
 * Implementation of processArg.
 */
template<class T>
bool DiomedeMultiArg<T>::processArg(int *i, std::vector<std::string>& args)
{
 	if ( _ignoreable && Arg::ignoreRest() )
		return false;

    // _hasBlanks checks for an * which is used for the SwitchArg - for Diomede
    // purposes, this can be ignored (?)  TBD: replace * with some other character
    // that can be used in the same manner.
    /*
    if ( _hasBlanks( args[*i] ) )
        return false;
    */

	std::string flag = args[*i];
	std::string value = "";

   	trimFlag( flag, value );

   	if ( argMatches( flag ) )
   	{
   		if ( DiomedeMultiArg::delimiter() != ' ' && value == "" )
			throw( ArgParseException(
			           "Couldn't find delimiter for this argument!",
					   toString() ) );

		// always take the first one, regardless of start string
		if ( value == "" )
		{
			(*i)++;
			if ( static_cast<unsigned int>(*i) < args.size() )
				_extractValue( args[*i] );
			else
				throw( ArgParseException("Missing a value for this argument!",
                                         toString() ) );
		}
		else
			_extractValue( value );

		/*
		// continuing taking the args until we hit one with a start string
		while ( (unsigned int)(*i)+1 < args.size() &&
				args[(*i)+1].find_first_of( Arg::flagStartString() ) != 0 &&
		        args[(*i)+1].find_first_of( Arg::nameStartString() ) != 0 )
				_extractValue( args[++(*i)] );
		*/

		_alreadySet = true;
		_checkWithVisitor();

		return true;
	}
	else
		return false;
}

/**
 * Diomede: following command usage, reset any arguments.
 * TCLAP assumes commands allocated on the stack, not the
 * and therefore, does not account for multiple command usage.
 */
template<class T>
void DiomedeMultiArg<T>::resetArg()
{
    _alreadySet = false;
}

/**
 * Diomede: following command usage, reset any argument values.
 * TCLAP assumes commands allocated on the stack, not the
 * and therefore, does not account for multiple command usage.
 */
template<class T>
void DiomedeMultiArg<T>::resetValue()
{
    T val;
	for ( int i = 0; static_cast<unsigned int>(i) < _values.size(); i++ ) {
	    _values[i] = val;
	}
}

/**
 * Implementation of shortID.
 */
template<class T>
std::string DiomedeMultiArg<T>::shortID(const std::string& val) const
{
	std::string szID = Arg::shortID(_typeDesc) + " ... ";
	return szID;
}

/**
 * Implementation of longID.
 */
template<class T>
std::string DiomedeMultiArg<T>::longID(const std::string& valueId, const bool& bShowName /*false*/ ) const
{
	std::string szID = "";

	if ( _flag != "" )
	{
		szID += flagStartChar() + _flag;

	    std::string szDelim = " ";
	    szDelim = DiomedeMultiArg::delimiter();

		if ( _valueRequired )
			szID += szDelim + "<" + _typeDesc + ">";

		if (bShowName) {
		    szID += ",  ";
		}
	}

    // For Diomede purposes, we don't need to show the name flag "//"
    // since it's not currently used and will only add confusion.
    if (bShowName) {
	    szID += Arg::nameStartString() + _name;

	    if ( _valueRequired )
		    szID += " <" + _typeDesc + ">";
	}

	return szID;

    /*
	std::string szID = Arg::longID(_typeDesc, bShowName) + "  (accepted multiple times)";
	return szID;
	*/
}

/**
 * Once we've matched the first value, then the arg is no longer
 * required.
 */
template<class T>
bool DiomedeMultiArg<T>::isRequired() const
{
	if ( _required )
	{
		if ( _values.size() > 1 )
			return false;
		else
			return true;
   	}
   	else
		return false;

}

/**
 * Implementation of _extractValue.
 */
template<class T>
void DiomedeMultiArg<T>::_extractValue( const std::string& val )
{
    std::string szTmpVal = val;

    // Convert the value to lower case
    if ( _useLowerCase ) {

        char szConstraintBuffer[100];
        strcpy(szConstraintBuffer, val.c_str());

        _tcslwr(szConstraintBuffer);
        szTmpVal = std::string(szConstraintBuffer);
    }

	DIOMEDE_MULTI_ARG_HELPER::DiomedeValueExtractor<T> ve(_values);

	int err = ve.extractValue(szTmpVal);

	if ( err == DIOMEDE_MULTI_ARG_HELPER::EXTRACT_FAILURE )
		throw( ArgParseException("Couldn't read argument value "
                                 "from string '" + val + "'", toString() ) );

	if(err == DIOMEDE_MULTI_ARG_HELPER::EXTRACT_TOO_MANY)
	    throw( ArgParseException("More than one valid value "
                                 "parsed from string '" + val + "'",
								 toString() ) );
	if ( _constraint != NULL )
		if ( ! _constraint->check( _values.back() ) )
			throw( CmdLineParseException( "Value '" + val +
                                          "' does not meet constraint: " +
                                          _constraint->description(),
										  toString() ) );
}

/**
 * Implementation of allowMore.
 */
template<class T>
bool DiomedeMultiArg<T>::allowMore()
{
	bool am = _allowMore;
	_allowMore = true;
	return am;
}

/**
 * Implementation of addToList
 * Overridden by Args that need to added to the end of the list.
 */
template<class T>
void DiomedeMultiArg<T>::addToList( std::list<Arg*>& argList ) const
{
	argList.push_back( const_cast<DiomedeMultiArg*>(this) );
}

/**
 * Implementation of trimFlag.
 */
template<class T>
void DiomedeMultiArg<T>::trimFlag(std::string& flag, std::string& value) const
{
	int stop = 0;
	for ( int i = 0; static_cast<unsigned int>(i) < flag.length(); i++ )
		if ( flag[i] == DiomedeMultiArg::delimiter() )
		{
			stop = i;
			break;
		}

	if ( stop > 1 )
	{
		value = flag.substr(stop+1);
		flag = flag.substr(0,stop);
	}
}

} // namespace TCLAP

#endif // __DIOMEDE_MULTI_ARGUMENT_H__

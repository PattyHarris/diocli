/*********************************************************************
 *
 *  file:  DiomedeValueArg.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Diomede specific value arguments, specifically to
 *          allow for flag arguments using /argument=value where
 *          "agument" is a string of any length.
 *
 *********************************************************************/

#ifndef __DIOMEDE_VALUE_ARGUMENT_H__
#define __DIOMEDE_VALUE_ARGUMENT_H__

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

template<class T> class DiomedeValueArg;

namespace DIOMEDE_VALUE_ARG_HELPER {

enum Error_e { EXTRACT_FAILURE = 1000, EXTRACT_TOO_MANY };

/**
 * \class DiomedeValueExtractor
 * \brief This class is used to extract a value from an argument.
 * It is used because we need a special implementation to
 * deal with std::string and making a specialized function
 * puts it in the T segment, thus generating link errors.
 * Having a specialiced class makes the symbols weak.
 * This is not pretty but I don't know how to make it
 * work any other way.
 */
template<class T> class DiomedeValueExtractor
{
	/**
	 *
	 */
	friend class DiomedeValueArg<T>;

	private:

		/**
		 * Reference to the value where the result of the extraction will
		 * be put.
		 */
        T &_value;

		/**
		 * Constructor.
		 * \param value - Where the value extracted will be put.
		 */
        DiomedeValueExtractor(T &value) : _value(value) { }

		/**
		 * Method that will attempt to parse the input stream for a value
		 * of type T.
		 * \param val - Where the value parsed will be put.
		 */
        int extractValue( const std::string& val )
		{

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
                    is >> _value;
                else
                    break;

                valuesRead++;
            }

            if ( is.fail() )
                return EXTRACT_FAILURE;

            if ( valuesRead > 1 )
                return EXTRACT_TOO_MANY;

            return 0;
        }
};

/**
 * \class DiomedeValueExtractor
 * \brief Specialization for string.  This is necessary because istringstream
 * operator>> is not able to ignore spaces...  meaning -x "X Y" will only
 * read 'X'... and thus the specialization.
 */
template<> class DiomedeValueExtractor<std::string>
{
	/**
	 *
	 */
    friend class DiomedeValueArg<std::string>;

    private:

		/**
		 * Reference to the value where the result of the extraction will
		 * be put.
		 */
        std::string &_value;

		/**
		 * Constructor.
		 * \param value - Where the value extracted will be put.
		 */
        DiomedeValueExtractor(std::string &value) : _value(value) {}

		/**
		 * Method that will attempt to parse the input stream for a value
		 * of type std::string.
		 * \param val - Where the string parsed will be put.
		 */
        int extractValue( const std::string& val )
		{
            _value = val;
            return 0;
        }
};

} //namespace DIOMEDE_VALUE_ARG_HELPER

/**
 * \class DiomedeValueArg
 * \brief The basic labeled argument that parses a value.
 * This is a template class, which means the type T defines the type
 * that a given object will attempt to parse when the flag/name is matched
 * on the command line.  While there is nothing stopping you from creating
 * an unflagged DiomedeValueArg, it is unwise and would cause significant problems.
 * Instead use an UnlabeledValueArg.
 */
template<class T>
class DiomedeValueArg : public Arg
{
    private:
		/**
		 * The delimiter that separates an argument flag/name from the
		 * value.
		 */
		static char& delimiterRef() { static char delim = '='; return delim; }

    protected:

        /**
         * The value parsed from the command line.
         * Can be of any type, as long as the >> operator for the type
         * is defined.
         */
        T _value;

        /**
         * A human readable description of the type to be parsed.
         * This is a hack, plain and simple.  Ideally we would use RTTI to
         * return the name of type T, but until there is some sort of
         * consistent support for human readable names, we are left to our
         * own devices.
         */
        std::string _typeDesc;

        /**
         * A Constraint this Arg must conform to.
         */
        Constraint<T>* _constraint;

        /**
         * Extracts the value from the string.
         * Attempts to parse string as type T, if this fails an exception
         * is thrown.
         * \param val - value to be parsed.
         */
        void _extractValue( const std::string& val );

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
         * Labeled DiomedeValueArg constructor.
         * You could conceivably call this constructor with a blank flag,
         * but that would make you a bad person.  It would also cause
         * an exception to be thrown.   If you want an unlabeled argument,
         * use the other constructor.
         * \param flag - The one character flag that identifies this
         * argument on the command line.
         * \param name - A one word name for the argument.  Can be
         * used as a long flag on the command line.
         * \param desc - A description of what the argument is for or
         * does.
         * \param req - Whether the argument is required on the command
         * line.
         * \param value - The default value assigned to this argument if it
         * is not present on the command line.
         * \param typeDesc - A short, human readable description of the
         * type that this object expects.  This is used in the generation
         * of the USAGE statement.  The goal is to be helpful to the end user
         * of the program.
         * \param v - An optional visitor.  You probably should not
         * use this unless you have a very good reason.
         */
        DiomedeValueArg( const std::string& flag,
                  const std::string& name,
                  const std::string& desc,
                  bool req,
                  T value,
                  const std::string& typeDesc,
                  Visitor* v = NULL);


        /**
         * Labeled DiomedeValueArg constructor.
         * You could conceivably call this constructor with a blank flag,
         * but that would make you a bad person.  It would also cause
         * an exception to be thrown.   If you want an unlabeled argument,
         * use the other constructor.
         * \param flag - The one character flag that identifies this
         * argument on the command line.
         * \param name - A one word name for the argument.  Can be
         * used as a long flag on the command line.
         * \param desc - A description of what the argument is for or
         * does.
         * \param req - Whether the argument is required on the command
         * line.
         * \param value - The default value assigned to this argument if it
         * is not present on the command line.
         * \param typeDesc - A short, human readable description of the
         * type that this object expects.  This is used in the generation
         * of the USAGE statement.  The goal is to be helpful to the end user
         * of the program.
         * \param parser - A CmdLine parser object to add this Arg to
         * \param v - An optional visitor.  You probably should not
         * use this unless you have a very good reason.
         */
        DiomedeValueArg( const std::string& flag,
                  const std::string& name,
                  const std::string& desc,
                  bool req,
                  T value,
                  const std::string& typeDesc,
                  CmdLineInterface& parser,
                  Visitor* v = NULL );

        /**
         * Labeled DiomedeValueArg constructor.
         * You could conceivably call this constructor with a blank flag,
         * but that would make you a bad person.  It would also cause
         * an exception to be thrown.   If you want an unlabeled argument,
         * use the other constructor.
         * \param flag - The one character flag that identifies this
         * argument on the command line.
         * \param name - A one word name for the argument.  Can be
         * used as a long flag on the command line.
         * \param desc - A description of what the argument is for or
         * does.
         * \param req - Whether the argument is required on the command
         * line.
         * \param value - The default value assigned to this argument if it
         * is not present on the command line.
         * \param constraint - A pointer to a Constraint object used
		 * to constrain this Arg.
         * \param parser - A CmdLine parser object to add this Arg to.
         * \param v - An optional visitor.  You probably should not
         * use this unless you have a very good reason.
         */
        DiomedeValueArg( const std::string& flag,
                  const std::string& name,
                  const std::string& desc,
                  bool req,
                  T value,
                  Constraint<T>* constraint,
                  CmdLineInterface& parser,
                  Visitor* v = NULL );

        /**
         * Labeled DiomedeValueArg constructor.
         * You could conceivably call this constructor with a blank flag,
         * but that would make you a bad person.  It would also cause
         * an exception to be thrown.   If you want an unlabeled argument,
         * use the other constructor.
         * \param flag - The one character flag that identifies this
         * argument on the command line.
         * \param name - A one word name for the argument.  Can be
         * used as a long flag on the command line.
         * \param desc - A description of what the argument is for or
         * does.
         * \param req - Whether the argument is required on the command
         * line.
         * \param value - The default value assigned to this argument if it
         * is not present on the command line.
         * \param constraint - A pointer to a Constraint object used
		 * to constrain this Arg.
         * \param v - An optional visitor.  You probably should not
         * use this unless you have a very good reason.
         */
        DiomedeValueArg( const std::string& flag,
                  const std::string& name,
                  const std::string& desc,
                  bool req,
                  T value,
                  Constraint<T>* constraint,
                  Visitor* v = NULL );

        /**
         * DiomedeValueArg destructor.   Added to destroy allocated
         * constraints.
        */
    	virtual ~DiomedeValueArg();

       /**
         * Handles the processing of the argument.
         * This re-implements the Arg version of this method to set the
         * _value of the argument appropriately.  It knows the difference
         * between labeled and unlabeled.
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
         * Returns the value of the argument.
         */
        T& getValue() ;

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
         * Specialization of shortID.
         * \param valueId - value to be used.
         */
        virtual std::string shortID(const std::string& valueId = "val") const;

        /**
         * Specialization of longID.
		 * \param val - The value used in the id.
		 * \param bShowName - Diomede: Display the name flag and value.
		 */
		virtual std::string longID( const std::string& val = "val",
		                            const bool& bShowName = false  ) const;

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
DiomedeValueArg<T>::DiomedeValueArg(const std::string& flag,
                      const std::string& name,
                      const std::string& desc,
                      bool req,
                      T value,
                      const std::string& typeDesc,
                      Visitor* v)
: Arg(flag, name, desc, req, true, v),
    _value( value ),
    _typeDesc( typeDesc ),
    _constraint( NULL ),
    _useLowerCase(true)
{ }

template<class T>
DiomedeValueArg<T>::DiomedeValueArg(const std::string& flag,
                      const std::string& name,
                      const std::string& desc,
                      bool req,
                      T value,
                      const std::string& typeDesc,
                      CmdLineInterface& parser,
                      Visitor* v)
: Arg(flag, name, desc, req, true, v),
  _value( value ),
  _typeDesc( typeDesc ),
  _constraint( NULL ),
  _useLowerCase(true),
  _isValidated(true)
{
    parser.add( this );
}

template<class T>
DiomedeValueArg<T>::DiomedeValueArg(const std::string& flag,
                      const std::string& name,
                      const std::string& desc,
                      bool req,
                      T value,
                      Constraint<T>* constraint,
                      Visitor* v)
: Arg(flag, name, desc, req, true, v),
    _value( value ),
    _typeDesc( constraint->shortID() ),
    _constraint( constraint ),
  _useLowerCase(true),
  _isValidated(true)
{ }

template<class T>
DiomedeValueArg<T>::DiomedeValueArg(const std::string& flag,
                      const std::string& name,
                      const std::string& desc,
                      bool req,
                      T value,
                      Constraint<T>* constraint,
                      CmdLineInterface& parser,
                      Visitor* v)
: Arg(flag, name, desc, req, true, v),
    _value( value ),
    _typeDesc( constraint->shortID() ),
    _constraint( constraint ),
  _useLowerCase(true),
  _isValidated(true)
{
    parser.add( this );
}


/**
 * DiomedeValueArg destructor.   Added to destroy allocated
 * constraints.
*/
template<class T>
DiomedeValueArg<T>::~DiomedeValueArg()
{
    if (_constraint != NULL) {
        delete _constraint;
        _constraint = NULL;
    }

} // End Destructor

/**
 * Implementation of getValue().
 */
template<class T>
T& DiomedeValueArg<T>::getValue() { return _value; }

/**
 * Implementation of processArg().
 */
template<class T>
bool DiomedeValueArg<T>::processArg(int *i, std::vector<std::string>& args)
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
        if ( _alreadySet )
			throw( CmdLineParseException("Argument already set!", toString()) );

        if ( DiomedeValueArg::delimiter() != ' ' && value == "" )
			throw( ArgParseException(
							"Couldn't find delimiter for this argument!",
                             toString() ) );

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
void DiomedeValueArg<T>::resetArg()
{
    _alreadySet = false;
}

/**
 * Diomede: following command usage, reset any argument values.
 * TCLAP assumes commands allocated on the stack, not the
 * and therefore, does not account for multiple command usage.
 */
template<class T>
void DiomedeValueArg<T>::resetValue()
{
    T val;
    _value = val;
}

/**
 * Implementation of shortID.
 */
template<class T>
std::string DiomedeValueArg<T>::shortID(const std::string& valueId) const
{
	std::string szID = "";

	if ( _flag != "" ) {
		szID = Arg::flagStartChar() + _flag;
    }
	else {
		szID = Arg::nameStartString() + _name;
    }

	std::string szDelim = " ";
	szDelim = DiomedeValueArg::delimiter();

	if ( _valueRequired )
		szID += szDelim + "<" + _typeDesc  + ">";

	if ( !_required )
		szID = "[" + szID + "]";

	return szID;
}

/**
 * Implementation of longID.
 */
template<class T>
std::string DiomedeValueArg<T>::longID(const std::string& val, const bool& bShowName /*false*/ ) const
{
	std::string szID = "";

	if ( _flag != "" )
	{
		szID += flagStartChar() + _flag;

	    std::string szDelim = " ";
	    szDelim = DiomedeValueArg::delimiter();

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

}

/**
 * Implementation of trimFlag.
 */
template<class T>
void DiomedeValueArg<T>::trimFlag(std::string& flag, std::string& value) const
{
	int stop = 0;
	for ( int i = 0; static_cast<unsigned int>(i) < flag.length(); i++ )
		if ( flag[i] == DiomedeValueArg::delimiter() )
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

/**
 * Implementation of addToList
 * Overridden by Args that need to added to the end of the list.
 */
template<class T>
void DiomedeValueArg<T>::addToList( std::list<Arg*>& argList ) const
{
	argList.push_back( const_cast<DiomedeValueArg*>(this) );
}

/**
 * Implementation of _extractValue.
 */
template<class T>
void DiomedeValueArg<T>::_extractValue( const std::string& val )
{
    std::string szTmpVal = val;

    // Convert the value to lower case
    if ( _useLowerCase ) {

        char szConstraintBuffer[100];
        strcpy(szConstraintBuffer, val.c_str());

        _tcslwr(szConstraintBuffer);
        szTmpVal = std::string(szConstraintBuffer);
    }

	DIOMEDE_VALUE_ARG_HELPER::DiomedeValueExtractor<T> ve(_value);

	int err = ve.extractValue(szTmpVal);

	if ( err == DIOMEDE_VALUE_ARG_HELPER::EXTRACT_FAILURE )
		throw( ArgParseException("Couldn't read argument value from string '" +
	                             val + "'", toString() ) );

	if ( err == DIOMEDE_VALUE_ARG_HELPER::EXTRACT_TOO_MANY )
		throw( ArgParseException(
					"More than one valid value parsed from string '" +
				    val + "'", toString() ) );

	if ( _constraint != NULL ) {
		if ( ! _constraint->check( _value ) ) {
			throw( CmdLineParseException( "Value '" + val +
									      "' does not meet constraint: " +
										  _constraint->description(),
										  toString() ) );
		}
    }
}

} // namespace TCLAP

#endif // __DIOMEDE_VALUE_ARGUMENT_H__


/*********************************************************************
 *
 *  file:  DiomedeUnlabeledValueArg.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Diomede specific unlabeled arguments, specifically to
 *          allow for multiple optional unlabeled arguments per command.
 *
 *********************************************************************/
#ifndef __DIOMEDE_UNLABELED_VALUE_ARGUMENT_H__
#define __DIOMEDE_UNLABELED_VALUE_ARGUMENT_H__

#include <string>
#include <vector>

#include <tclap/ValueArg.h>
#include <tclap/OptionalUnlabeledTracker.h>
#include "DiomedeValueArg.h"


namespace TCLAP {

/**
 * The basic unlabeled argument that parses a value.
 * This is a template class, which means the type T defines the type
 * that a given object will attempt to parse when an UnlabeledValueArg
 * is reached in the list of args that the CmdLine iterates over.
 */
template<class T>
class DiomedeUnlabeledValueArg : public DiomedeValueArg<T>
{

	// If compiler has two stage name lookup (as gcc >= 3.4 does)
	// this is required to prevent undef. symbols
	using DiomedeValueArg<T>::_ignoreable;
	using DiomedeValueArg<T>::_hasBlanks;
	using DiomedeValueArg<T>::_extractValue;
	using DiomedeValueArg<T>::_typeDesc;
	using DiomedeValueArg<T>::_name;
	using DiomedeValueArg<T>::_description;
	using DiomedeValueArg<T>::_alreadySet;
	using DiomedeValueArg<T>::_value;
	using DiomedeValueArg<T>::_constraint;
	using DiomedeValueArg<T>::toString;

	public:

		/**
		 * DiomedeUnlabeledValueArg constructor.
		 * \param cmdOwner - validate this argument against those already
		 * owned by the command (Diomede).
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
		 * \param ignoreable - Allows you to specify that this argument can be
		 * ignored if the '--' flag is set.  This defaults to false (cannot
		 * be ignored) and should  generally stay that way unless you have
		 * some special need for certain arguments to be ignored.
		 * \param v - Optional Vistor.  You should leave this blank unless
		 * you have a very good reason.
		 */
		DiomedeUnlabeledValueArg( CmdLineInterface* cmdOwner,
                                  const std::string& name,
			                      const std::string& desc,
						          bool req,
				                  T value,
				                  const std::string& typeDesc,
						          bool ignoreable = false,
				                  Visitor* v = NULL);

		/**
		 * DiomedeUnlabeledValueArg constructor.
		 * \param cmdOwner - validate this argument against those already
		 * owned by the command (Diomede).
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
		 * \param ignoreable - Allows you to specify that this argument can be
		 * ignored if the '--' flag is set.  This defaults to false (cannot
		 * be ignored) and should  generally stay that way unless you have
		 * some special need for certain arguments to be ignored.
		 * \param v - Optional Vistor.  You should leave this blank unless
		 * you have a very good reason.
		 */
		DiomedeUnlabeledValueArg( CmdLineInterface* cmdOwner,
                                  const std::string& name,
		       	                  const std::string& desc,
			       			      bool req,
				                  T value,
				                  const std::string& typeDesc,
					       	      CmdLineInterface& parser,
						          bool ignoreable = false,
				                  Visitor* v = NULL );

		/**
		 * DiomedeUnlabeledValueArg constructor.
		 * \param cmdOwner - validate this argument against those already
		 * owned by the command (Diomede).
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
		 * \param ignoreable - Allows you to specify that this argument can be
		 * ignored if the '--' flag is set.  This defaults to false (cannot
		 * be ignored) and should  generally stay that way unless you have
		 * some special need for certain arguments to be ignored.
		 * \param v - Optional Vistor.  You should leave this blank unless
		 * you have a very good reason.
		 */
		DiomedeUnlabeledValueArg( CmdLineInterface* cmdOwner,
                                  const std::string& name,
			                      const std::string& desc,
						          bool req,
				                  T value,
				                  Constraint<T>* constraint,
						          bool ignoreable = false,
				                  Visitor* v = NULL );


		/**
		 * DiomedeUnlabeledValueArg constructor.
		 * \param cmdOwner - validate this argument against those already
		 * owned by the command (Diomede).
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
		 * \param parser - A CmdLine parser object to add this Arg to
		 * \param ignoreable - Allows you to specify that this argument can be
		 * ignored if the '--' flag is set.  This defaults to false (cannot
		 * be ignored) and should  generally stay that way unless you have
		 * some special need for certain arguments to be ignored.
		 * \param v - Optional Vistor.  You should leave this blank unless
		 * you have a very good reason.
		 */
		DiomedeUnlabeledValueArg( CmdLineInterface* cmdOwner,
                                  const std::string& name,
			                      const std::string& desc,
						          bool req,
				                  T value,
				                  Constraint<T>* constraint,
						          CmdLineInterface& parser,
						          bool ignoreable = false,
				                  Visitor* v = NULL);

		/**
		 * Deletes any resources allocated by a DiomedeUnlabeledValueArg object.
		 */
		virtual ~DiomedeUnlabeledValueArg();

		/**
		 * Handles the processing of the argument.
		 * This re-implements the Arg version of this method to set the
		 * _value of the argument appropriately.  Handling specific to
		 * unlabled arguments.
		 * \param i - Pointer the the current argument in the list.
		 * \param args - Mutable list of strings.
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
		 * Overrides shortID for specific behavior.
		 */
		virtual std::string shortID(const std::string& val="val") const;

		/**
		 * Overrides longID for specific behavior.
		 * \param val - The value used in the id.
		 * \param bShowName - Diomede: Display the name flag and value.
		 */
		virtual std::string longID( const std::string& val = "val",
		                            const bool& bShowName = false  ) const;

		/**
		 * Overrides operator== for specific behavior.
		 */
		virtual bool operator==(const Arg& a ) const;

		/**
		 * Instead of pushing to the front of list, push to the back.
		 * \param argList - The list to add this to.
		 */
		virtual void addToList( std::list<Arg*>& argList ) const;

};

/**
 * Constructor implemenation.
 */
template<class T>
DiomedeUnlabeledValueArg<T>::DiomedeUnlabeledValueArg(CmdLineInterface* cmdOwner,
                                        const std::string& name,
					                    const std::string& desc,
										bool req,
					                    T value,
					                    const std::string& typeDesc,
					                    bool ignoreable,
					                    Visitor* v)
: DiomedeValueArg<T>("", name, desc, req, value, typeDesc, v)
{
	_ignoreable = ignoreable;

    // Skip the check for optional arguments - Diomede needs
    // to handle mulitple optional unlabeled arguments.
	// cmdOwner->check(req, toString());

}

/**
 * Constructor implemenation.
 */
template<class T>
DiomedeUnlabeledValueArg<T>::DiomedeUnlabeledValueArg(CmdLineInterface* cmdOwner,
                                        const std::string& name,
					                    const std::string& desc,
										bool req,
					                    T value,
					                    const std::string& typeDesc,
					                    CmdLineInterface& parser,
					                    bool ignoreable,
					                    Visitor* v)
: DiomedeValueArg<T>("", name, desc, req, value, typeDesc, v)
{
	_ignoreable = ignoreable;

    // Skip the check for optional arguments - Diomede needs
    // to handle mulitple optional unlabeled arguments.
	// cmdOwner->check(req, toString());

	parser.add( this );
}

/**
 * Constructor implemenation.
 */
template<class T>
DiomedeUnlabeledValueArg<T>::DiomedeUnlabeledValueArg(CmdLineInterface* cmdOwner,
                                        const std::string& name,
                                        const std::string& desc,
										bool req,
                                        T value,
                                        Constraint<T>* constraint,
                                        bool ignoreable,
                                        Visitor* v)
: DiomedeValueArg<T>("", name, desc, req, value, constraint, v)
{
	_ignoreable = ignoreable;
    // Skip the check for optional arguments - Diomede needs
    // to handle mulitple optional unlabeled arguments.
	//cmdOwner->check(req, toString());
}

/**
 * Constructor implemenation.
 */
template<class T>
DiomedeUnlabeledValueArg<T>::DiomedeUnlabeledValueArg(CmdLineInterface* cmdOwner,
                                        const std::string& name,
					                    const std::string& desc,
										bool req,
					                    T value,
					                    Constraint<T>* constraint,
					                    CmdLineInterface& parser,
					                    bool ignoreable,
					                    Visitor* v)
: DiomedeValueArg<T>("", name, desc, req, value, constraint,  v)
{
	_ignoreable = ignoreable;

    // Skip the check for optional arguments - Diomede needs
    // to handle mulitple optional unlabeled arguments.
	// cmdOwner->check(req, toString());

	parser.add( this );
}

/**
 * Diomede: destructor to clean up constraints
 */
template<class T>
DiomedeUnlabeledValueArg<T>::~DiomedeUnlabeledValueArg()
{
    if (_constraint != NULL) {
        delete _constraint;
        _constraint = NULL;
    }
}

/**
 * Implementation of processArg().
 */
template<class T>
bool DiomedeUnlabeledValueArg<T>::processArg(int *i, std::vector<std::string>& args)
{
    // Diomede allows for a mix of unlabeled and labeled arguments.
    // If the current argument is a labeled argument, ignore it.
    std::string szTempArg = args[*i];

    /*
    if (szTempArg.find( _T("/") ) != -1) {
        return false;
    }
    */

    #if 0
    // I don't think we need to check this for unlabeled arguments.
	#ifdef WIN32
		if ( ( szTempArg.find( Arg::flagStartString(0), 0 ) != std::string::npos ) ||
	         ( szTempArg.find( Arg::flagStartString(1), 0 ) != std::string::npos ) ||
	         ( szTempArg.find( Arg::flagStartString(2), 0 ) != std::string::npos ) ) {
			return false;
		}
	#else
		if ( ( szTempArg.find( Arg::flagStartString(0), 0 ) != std::string::npos ) ||
			 ( szTempArg.find( Arg::flagStartString(1), 0 ) != std::string::npos ) ) {
			return false;
		}
	#endif
	#endif

	if ( _alreadySet )
		return false;

	if ( _hasBlanks( args[*i] ) )
		return false;

	// never ignore an unlabeled arg

	_extractValue( args[*i] );
	_alreadySet = true;
	return true;
}

/**
 * Diomede: following command usage, reset any arguments.
 * TCLAP assumes commands allocated on the stack, not the
 * and therefore, does not account for multiple command usage.
 */
template<class T>
void DiomedeUnlabeledValueArg<T>::resetArg()
{
    _alreadySet = false;
}

/**
 * Diomede: following command usage, reset any argument values.
 * TCLAP assumes commands allocated on the stack, not the
 * and therefore, does not account for multiple command usage.
 */
template<class T>
void DiomedeUnlabeledValueArg<T>::resetValue()
{
    T val;
	_value = val;
}

/**
 * Overriding shortID for specific output.
 */
template<class T>
std::string DiomedeUnlabeledValueArg<T>::shortID(const std::string& val) const
{
	std::string id = "<" + _typeDesc + ">";

	return id;
}

/**
 * Overriding longID for specific output.
 */
template<class T>
std::string DiomedeUnlabeledValueArg<T>::longID(const std::string& val,
                                                const bool& bShowName /*false*/ ) const
{
	// Ideally we would like to be able to use RTTI to return the name
	// of the type required for this argument.  However, g++ at least,
	// doesn't appear to return terribly useful "names" of the types.
	std::string id = "<" + _typeDesc + ">";

	return id;
}

/**
 * Overriding operator== for specific behavior.
 */
template<class T>
bool DiomedeUnlabeledValueArg<T>::operator==(const Arg& a ) const
{
	if ( _name == a.getName() || _description == a.getDescription() )
		return true;
	else
		return false;
}

/**
 * Overriding addToList for specific behavior.
 */
template<class T>
void DiomedeUnlabeledValueArg<T>::addToList( std::list<Arg*>& argList ) const
{
	argList.push_back( const_cast<Arg*>(static_cast<const Arg* const>(this)) );
}

}

#endif // __DIOMEDE_UNLABELED_VALUE_ARGUMENT_H__

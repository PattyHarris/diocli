/*********************************************************************
 *
 *  file:  DiomedeUnlabeledMultiArg.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 *
 * Purpose: Diomede specific unlabeled multiple arguments, specifically to
 *          allow for multiple optional arguments per command.
 *
 *********************************************************************/
#ifndef __DIOMEDE_MULTIPLE_UNLABELED_ARGUMENT_H__
#define __DIOMEDE_MULTIPLE_UNLABELED_ARGUMENT_H__

#include <string>
#include <vector>

#include <tclap/MultiArg.h>
#include <tclap/OptionalUnlabeledTracker.h>
#include "DiomedeMultiArg.h"

namespace TCLAP {

/**
 * Just like a MultiArg, except that the arguments are unlabeled.  Basically,
 * this Arg will slurp up everything that hasn't been matched to another
 * Arg.
 */
template<class T>
class DiomedeUnlabeledMultiArg : public DiomedeMultiArg<T>
{

	// If compiler has two stage name lookup (as gcc >= 3.4 does)
	// this is required to prevent undef. symbols
	using DiomedeMultiArg<T>::_ignoreable;
	using DiomedeMultiArg<T>::_hasBlanks;
	using DiomedeMultiArg<T>::_extractValue;
	using DiomedeMultiArg<T>::_typeDesc;
	using DiomedeMultiArg<T>::_name;
	using DiomedeMultiArg<T>::_description;
	using DiomedeMultiArg<T>::_alreadySet;
	using DiomedeMultiArg<T>::_values;
	using DiomedeMultiArg<T>::toString;

	public:

		/**
		 * Constructor.
		 * \param cmdOwner - validate this argument against those already
		 * owned by the command (Diomede).
		 * \param name - The name of the Arg. Note that this is used for
		 * identification, not as a long flag.
		 * \param desc - A description of what the argument is for or
		 * does.
		 * \param req - Whether the argument is required on the command
		 *  line.
		 * \param typeDesc - A short, human readable description of the
		 * type that this object expects.  This is used in the generation
		 * of the USAGE statement.  The goal is to be helpful to the end user
		 * of the program.
		 * \param ignoreable - Whether or not this argument can be ignored
		 * using the "--" flag.
		 * \param v - An optional visitor.  You probably should not
		 * use this unless you have a very good reason.
		 */
		DiomedeUnlabeledMultiArg( CmdLineInterface* cmdOwner,
		                          const std::string& name,
              		              const std::string& desc,
						          bool req,
				                  const std::string& typeDesc,
					              bool ignoreable = false,
				                  Visitor* v = NULL );
		/**
		 * Constructor.
		 * \param cmdOwner - validate this argument against those already
		 * owned by the command (Diomede).
		 * \param name - The name of the Arg. Note that this is used for
		 * identification, not as a long flag.
		 * \param desc - A description of what the argument is for or
		 * does.
		 * \param req - Whether the argument is required on the command
		 *  line.
		 * \param typeDesc - A short, human readable description of the
		 * type that this object expects.  This is used in the generation
		 * of the USAGE statement.  The goal is to be helpful to the end user
		 * of the program.
		 * \param parser - A CmdLine parser object to add this Arg to
		 * \param ignoreable - Whether or not this argument can be ignored
		 * using the "--" flag.
		 * \param v - An optional visitor.  You probably should not
		 * use this unless you have a very good reason.
		 */
		DiomedeUnlabeledMultiArg( CmdLineInterface* cmdOwner,
		                          const std::string& name,
				                  const std::string& desc,
						          bool req,
				                  const std::string& typeDesc,
						          CmdLineInterface& parser,
						          bool ignoreable = false,
				                  Visitor* v = NULL );

		/**
		 * Constructor.
		 * \param cmdOwner - validate this argument against those already
		 * owned by the command (Diomede).
		 * \param name - The name of the Arg. Note that this is used for
		 * identification, not as a long flag.
		 * \param desc - A description of what the argument is for or
		 * does.
		 * \param req - Whether the argument is required on the command
		 *  line.
		 * \param constraint - A pointer to a Constraint object used
		 * to constrain this Arg.
		 * \param ignoreable - Whether or not this argument can be ignored
		 * using the "--" flag.
		 * \param v - An optional visitor.  You probably should not
		 * use this unless you have a very good reason.
		 */
		DiomedeUnlabeledMultiArg( CmdLineInterface* cmdOwner,
		                          const std::string& name,
						          const std::string& desc,
						          bool req,
						          Constraint<T>* constraint,
						          bool ignoreable = false,
						          Visitor* v = NULL );

		/**
		 * Constructor.
		 * \param cmdOwner - validate this argument against those already
		 * owned by the command (Diomede).
		 * \param name - The name of the Arg. Note that this is used for
		 * identification, not as a long flag.
		 * \param desc - A description of what the argument is for or
		 * does.
		 * \param req - Whether the argument is required on the command
		 *  line.
		 * \param constraint - A pointer to a Constraint object used
		 * to constrain this Arg.
		 * \param parser - A CmdLine parser object to add this Arg to
		 * \param ignoreable - Whether or not this argument can be ignored
		 * using the "--" flag.
		 * \param v - An optional visitor.  You probably should not
		 * use this unless you have a very good reason.
		 */
		DiomedeUnlabeledMultiArg( CmdLineInterface* cmdOwner,
		                          const std::string& name,
						          const std::string& desc,
						          bool req,
						          Constraint<T>* constraint,
						          CmdLineInterface& parser,
						          bool ignoreable = false,
						          Visitor* v = NULL );

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
		 * Opertor ==.
		 * \param a - The Arg to be compared to this.
		 */
		virtual bool operator==(const Arg& a) const;

		/**
		 * Pushes this to back of list rather than front.
		 * \param argList - The list this should be added to.
		 */
		virtual void addToList( std::list<Arg*>& argList ) const;
};

template<class T>
DiomedeUnlabeledMultiArg<T>::DiomedeUnlabeledMultiArg(CmdLineInterface* cmdOwner,
		                                              const std::string& name,
				                                      const std::string& desc,
										              bool req,
					                                  const std::string& typeDesc,
										              bool ignoreable,
					                                  Visitor* v)
: DiomedeMultiArg<T>("", name, desc,  req, typeDesc, v)
{
	_ignoreable = ignoreable;
	cmdOwner->check(true, toString());
}

template<class T>
DiomedeUnlabeledMultiArg<T>::DiomedeUnlabeledMultiArg(CmdLineInterface* cmdOwner,
		                                              const std::string& name,
				                                      const std::string& desc,
										              bool req,
					                                  const std::string& typeDesc,
										              CmdLineInterface& parser,
										              bool ignoreable,
					                                  Visitor* v)
: DiomedeMultiArg<T>("", name, desc,  req, typeDesc, v)
{
	_ignoreable = ignoreable;

    // Skip the check for optional arguments - Diomede needs
    // to handle mulitple optional unlabeled arguments.
	// cmdOwner->check(req, toString());

	parser.add( this );
}


template<class T>
DiomedeUnlabeledMultiArg<T>::DiomedeUnlabeledMultiArg(CmdLineInterface* cmdOwner,
		                                              const std::string& name,
				                                      const std::string& desc,
										              bool req,
					                                  Constraint<T>* constraint,
										              bool ignoreable,
					                                  Visitor* v)
: DiomedeMultiArg<T>("", name, desc,  req, constraint, v)
{
	_ignoreable = ignoreable;
    // Skip the check for optional arguments - Diomede needs
    // to handle mulitple optional unlabeled arguments.
	// cmdOwner->check(req, toString());
}

template<class T>
DiomedeUnlabeledMultiArg<T>::DiomedeUnlabeledMultiArg(CmdLineInterface* cmdOwner,
		                                              const std::string& name,
				                                      const std::string& desc,
										              bool req,
					                                  Constraint<T>* constraint,
										              CmdLineInterface& parser,
										              bool ignoreable,
					                                  Visitor* v)
: DiomedeMultiArg<T>("", name, desc,  req, constraint, v)
{
	_ignoreable = ignoreable;
    // Skip the check for optional arguments - Diomede needs
    // to handle mulitple optional unlabeled arguments.
	// cmdOwner->check(req, toString());
	parser.add( this );
}


template<class T>
bool DiomedeUnlabeledMultiArg<T>::processArg(int *i, std::vector<std::string>& args)
{
    // Diomede allows for a mix of unlabeled and labeled arguments.
    // If the current argument is a labeled argument, ignore it.
    std::string szTempArg = args[*i];

    /*
    if (szTempArg.find( _T("/")) != -1) {
        return false;
    }
    */

    #if 0
    // I don't think we need to check for these flags for unlabled arguments.
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

    // _hasBlanks checks for an * which is used for the SwitchArg - for Diomede
    // purposes, this can be ignored (?)  TBD: replace * with some other character
    // that can be used in the same manner.
    /*
	if ( _hasBlanks( args[*i] ) )
		return false;
    */

	// never ignore an unlabeled multi arg

	// always take the first value, regardless of the start string
	_extractValue( args[(*i)] );

	/*
	// continue taking args until we hit the end or a start string
	while ( (unsigned int)(*i)+1 < args.size() &&
			args[(*i)+1].find_first_of( Arg::flagStartString() ) != 0 &&
            args[(*i)+1].find_first_of( Arg::nameStartString() ) != 0 )
		_extractValue( args[++(*i)] );
	*/

	_alreadySet = true;

	return true;
}

/**
 * Diomede: following command usage, reset any arguments.
 * TCLAP assumes commands allocated on the stack, not the
 * and therefore, does not account for multiple command usage.
 */
template<class T>
void DiomedeUnlabeledMultiArg<T>::resetArg()
{
    _alreadySet = false;
}

/**
 * Diomede: following command usage, reset any argument values.
 * TCLAP assumes commands allocated on the stack, not the
 * and therefore, does not account for multiple command usage.
 */
template<class T>
void DiomedeUnlabeledMultiArg<T>::resetValue()
{
    T val;
	for ( int i = 0; static_cast<unsigned int>(i) < _values.size(); i++ ) {
	    _values[i] = val;
	}

	_values.resize(0);
}

template<class T>
std::string DiomedeUnlabeledMultiArg<T>::shortID(const std::string& val) const
{
	std::string id = "<" + _typeDesc + "> ...";

	return id;
}

template<class T>
std::string DiomedeUnlabeledMultiArg<T>::longID(const std::string& val,
                                                const bool& bShowName /*false*/ ) const
{
	std::string id = "<" + _typeDesc + ">  (accepted multiple times)";

	return id;
}

template<class T>
bool DiomedeUnlabeledMultiArg<T>::operator==(const Arg& a) const
{
	if ( _name == a.getName() || _description == a.getDescription() )
		return true;
	else
		return false;
}

template<class T>
void DiomedeUnlabeledMultiArg<T>::addToList( std::list<Arg*>& argList ) const
{
	argList.push_back( const_cast<Arg*>(static_cast<const Arg* const>(this)) );
}

}

#endif // __DIOMEDE_MULTIPLE_UNLABELED_ARGUMENT_H__

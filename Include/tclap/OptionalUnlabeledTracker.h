

/******************************************************************************
 *
 *  file:  OptionalUnlabeledTracker.h
 *
 *  Copyright (c) 2005, Michael E. Smoot .
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


#ifndef TCLAP_OPTIONAL_UNLABELED_TRACKER_H
#define TCLAP_OPTIONAL_UNLABELED_TRACKER_H

#include <string>

namespace TCLAP {

class OptionalUnlabeledTracker
{
    protected:
        bool _ref;

	public:
		/**
		 * OptionalUnlabeledTracker constructor.
		 */
        OptionalUnlabeledTracker();

        // Diomede: causes link errors with Windows - the original statics need
        // to be declared and defined as below.
		//

		void check( bool req, const std::string& argName );

		void gotOptional() { _ref = true; }

		bool& alreadyOptional() { return alreadyOptionalRef(); }

	private:

		bool& alreadyOptionalRef() { return _ref; }

};

///////////////////////////////////////////////////////////////////////////////
// Begin OptionalUnlabeledTracker.cpp
///////////////////////////////////////////////////////////////////////////////

/**
 * Constructor implemenation.
 */
inline OptionalUnlabeledTracker::OptionalUnlabeledTracker()
    : _ref(false)
{
}

/**
  * Diomede: this attempts to ensure that one doesn't define multiple unlabeled
  * arguments for any one command.  The problem here is that it prevents multiple
  * commands with unlabeled arguments.
 */
inline void OptionalUnlabeledTracker::check( bool req, const std::string& argName )
{
    if ( alreadyOptional() )
        throw( SpecificationException(
            "You can't specify ANY Unlabeled Arg following an optional Unlabeled Arg",
            argName ) );

    if ( !req )
        gotOptional();
}


} // namespace TCLAP

#endif

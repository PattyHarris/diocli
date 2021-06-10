/*********************************************************************
 *
 *  file:  BillingDefs.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 * 
 *  Use, modification, and distribution is subject to the New BSD License 
 *  (See accompanying file LICENSE)
 * 
 * Purpose: Billing constant definitions
 *
 *********************************************************************/

#ifndef __BILLING_DEFS_H__
#define __BILLING_DEFS_H__

using namespace std;
#include <string>

const string USA_SHORT                  = _T("USA");
const string USA_LONG                   = _T("United States");
const string CA_SHORT                   = _T("CA");
const string CA_LONG                    = _T("Canada");

const string COUNTRY_LAST               = _T("last");

//---------------------------------------------------------------------
//---------------------------------------------------------------------
namespace DioCLICountries {
	typedef enum {
	     COUNTRY_USA
		,COUNTRY_CA
		,COUNTRY_LAST
	} COUNTRY_ID;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef struct {
	DioCLICountries::COUNTRY_ID     m_countryID;
	std::string	                    m_szCountryShort;
	std::string	                    m_szCountryLong;
} COUNTRY_SET;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
const COUNTRY_SET DioCLICountryList[] =  {
	 { DioCLICountries::COUNTRY_USA,        USA_SHORT,      USA_LONG        }
	,{ DioCLICountries::COUNTRY_CA,	        CA_SHORT,       CA_LONG         }

	,{ DioCLICountries::COUNTRY_LAST,       COUNTRY_LAST,   COUNTRY_LAST    }
};

#pragma once

//---------------------------------------------------------------------
//---------------------------------------------------------------------
inline DioCLICountries::COUNTRY_ID CountryStrToCountryID( const string szCountryStr )
{
	static map<string, COUNTRY_SET> mapCountries;
	if ( mapCountries.size() == 0) {

		// Create and initilize all the countries in a map.
		// This will be quicker and less prone to error that
		// all the if statments
		unsigned int nIndex = 0;

		do {
			if (DioCLICountryList[nIndex].m_countryID != DioCLICountries::COUNTRY_LAST) {
			    COUNTRY_SET couuntrySet = DioCLICountryList[nIndex];
				(mapCountries)[DioCLICountryList[nIndex].m_szCountryShort] = couuntrySet;
				(mapCountries)[DioCLICountryList[nIndex].m_szCountryLong] = couuntrySet;
			}
			nIndex++;

		} while (DioCLICountryList[nIndex].m_countryID != DioCLICountries::COUNTRY_LAST);


	}
	// Pull and return the id from the map
	return (mapCountries)[szCountryStr].m_countryID;

} // End CommandStrToCommandID

#endif // __BILLING_DEFS_H__
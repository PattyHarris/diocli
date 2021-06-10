/*********************************************************************
 * 
 *  file:  StringUtil.cpp
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: Various string utilities for working with various data 
 *          formats and encryption.
 * 
 *********************************************************************/

#include "Stdafx.h"
#include "StringUtil.h"
#include "XString.h"

#include "Blowfish.h"

#include "ClientLog.h"
#include "ErrorCodes/UtilErrors.h"
#include "tclap/ArgException.h"
#include "types.h"
#include "openssl/md5.h"

namespace StringUtil {

///////////////////////////////////////////////////////////////////////
std::string ConvertToString(unsigned int nInput)
{
	std::string szOut;
	AppendIntToString( szOut, nInput, WIDE_NULL );
	return szOut;

} // End ConvertToString

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Function to convert unsigned char to string of length 2
void Char2Hex(const unsigned char szInChar, char* szHex)
{
	unsigned char byte[2];
	byte[0] = szInChar/16;
	byte[1] = szInChar%16;
	for(int i=0; i<2; i++)
	{
		if(byte[i] >= 0 && byte[i] <= 9)
			szHex[i] = '0' + byte[i];
		else
			szHex[i] = 'A' + byte[i] - 10;
	}
	szHex[2] = 0;

} // End Char2Hex

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Function to convert string of length 2 to unsigned char
void Hex2Char(const char* szHex, unsigned char& szOutChar)
{
	szOutChar = 0;
	for (int i = 0; i < 2; i++) {
		if(*(szHex + i) >='0' && *(szHex + i) <= '9')
			szOutChar = (szOutChar << 4) + (*(szHex + i) - '0');
		else if(*(szHex + i) >='A' && *(szHex + i) <= 'F')
			szOutChar = (szOutChar << 4) + (*(szHex + i) - 'A' + 10);
		else if(*(szHex + i) >='a' && *(szHex + i) <= 'f')
			szOutChar = (szOutChar << 4) + (*(szHex + i) - 'a' + 10);
		else
			break;
	}

} // End Hex2Char

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Function to convert string of unsigned chars to string of chars
void CharStr2HexStr(const unsigned char* pucCharStr, char* pszHexStr, int iSize)
{
	int i;
	char szHex[3];
	pszHexStr[0] = 0;
	for(i=0; i<iSize; i++)
	{
		Char2Hex(pucCharStr[i], szHex);
		strcat(pszHexStr, szHex);
	}

} // End CharStr2HexStr

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Function to convert string of chars to string of unsigned chars
void HexStr2CharStr(const char* pszHexStr, unsigned char* pucCharStr, int iSize)
{
	int i;
	unsigned char ch;
	for(i=0; i<iSize; i++)
	{
		Hex2Char(pszHexStr+2*i, ch);
		pucCharStr[i] = ch;
	}
} // End HexStr2CharStr

///////////////////////////////////////////////////////////////////////
std::string& tolower(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int(*)(int))::tolower);
	return s;

} // End tolower

///////////////////////////////////////////////////////////////////////
std::string& tolower( const std::string& sIn, std::string& sOut )
{
	std::transform(sIn.begin(), sIn.end(), sOut.begin(), (int(*)(int))::tolower);
	return sOut;

} // End tolower

///////////////////////////////////////////////////////////////////////
// Purpose: C++ version char* style "itoa".  It would appear that
//          itoa() isn't ANSI C standard and doesn't work with GCC
//          on Linux
// Requires:
//		nValue: value to convert to ascii
//		szResult: buffer for the result
//		nBase: base for conversion
// Returns: value converted to ascii
char* itoa( int nValue, char* szResult, int nBase )
{
	// check that the base if valid
	if (nBase < 2 || nBase > 16) {
		 *szResult = 0;
		 return szResult;
    }

	char* szOut = szResult;
	int nQuotient = nValue;

	do {
		*szOut = _T("0123456789abcdef")[ std::abs( nQuotient % nBase ) ];
		++szOut;
		nQuotient /= nBase;
	} while ( nQuotient );

	// Only apply negative sign for base 10
	if ( nValue < 0 && nBase == 10) *szOut++ = '-';

	std::reverse( szResult, szOut );
	*szOut = 0;

	return szResult;

} // end itoa

///////////////////////////////////////////////////////////////////////
// Purpose: C++ version char* style "i64toa".  It would appear that
//          i64toa() isn't ANSI C standard and doesn't work with GCC
//          on Linux
// Requires:
//		l64Value: value to convert to ascii
//		szResult: buffer for the result
// Returns: value converted to ascii
char* i64toa(LONG64 l64Value, char* szResult)
{
    sprintf(szResult,_T("%lld"), l64Value);
    return szResult;

} // end i64toa

///////////////////////////////////////////////////////////////////////
// Purpose: Platform specfic versions of atoi64
//          atoi64() isn't ANSI C standard and doesn't work with GCC
//          on Linux
// Requires:
//		szValue: value to convert to LONG64
// Returns: value converted to LONG64
LONG64 atoi64(const char* szValue)
{
    LONG64 l64Result = 0;

    #ifdef WIN32
            l64Result = _atoi64(szValue);
    #else
            l64Result = atoll(szValue);
    #endif

    return l64Result;

} // end atoi64

///////////////////////////////////////////////////////////////////////
// Purpose: Platform specfic versions of ltoa
//          ltoa() isn't ANSI C standard and doesn't work with GCC
//          on Linux
// Requires:
//		lValue: value to convert to long
//      szResult: resuting string
//      ucRadix: Base of value (e.g.: 2 for binary, 10 for decimal, 
//      16 for hex)
// Returns: value converted to long

#define NUMBER_OF_DIGITS 32

void _ultoa(unsigned long ulValue, char* szResult, unsigned char ucRadix)
{
    unsigned char nIndex;
    
    // Space for NUMBER_OF_DIGITS + '\0'
    char szBuffer[NUMBER_OF_DIGITS + 1]; 

    nIndex = NUMBER_OF_DIGITS;

    do {
        szBuffer[--nIndex] = char('0' + (ulValue % ucRadix));
        if ( szBuffer[nIndex] > '9') szBuffer[nIndex] += 'A' - '9' - 1;
        ulValue /= ucRadix;
    } while (ulValue != 0);

    do {
        *szResult++ = szBuffer[nIndex++];
    } while ( nIndex < NUMBER_OF_DIGITS );

    // Lastly, terminate the string.
    *szResult = 0;  

} // End _ultoa

///////////////////////////////////////////////////////////////////////
void _ltoa(long lValue, char* szResult, unsigned char ucRadix)
{
    if (lValue < 0 && ucRadix == 10) {
        *szResult++ = '-';
        lValue = -lValue;
    }
    _ultoa(lValue, szResult, ucRadix);

} // End _ltoa

///////////////////////////////////////////////////////////////////////
const unsigned char* convert(const std::string& szIn)
{
    int nLength = (int)szIn.size()+1;
    unsigned char* szBytes = new unsigned char [nLength];
    std::copy(szIn.begin(),szIn.end(), szBytes);
    return(szBytes);

} // End convert

///////////////////////////////////////////////////////////////////////
void replaceall(std::string &szIn , const std::string& szOld, const std::string& szNew)
{
	size_t pos1 = szIn.find(szOld);
	while ( pos1 != (size_t)-1 )
	{
		szIn.replace(pos1, szOld.size(), szNew);
		size_t pos2 = pos1 + szOld.size()+ 1;
		pos1 = szIn.find(szOld, pos2);
	}

} // End replaceall

}; // namespace StringUtil

///////////////////////////////////////////////////////////////////////
// Purpose:
//      To append an int to a string
int StringUtil::AppendIntToString( std::string &szDest, unsigned short i,
                                    TCHAR szDeliminter )
{
	return AppendIntToString(szDest, (unsigned int)i, szDeliminter);

} // End AppendIntToString

///////////////////////////////////////////////////////////////////////
int StringUtil::AppendIntToString( std::string &szDest, unsigned int i , TCHAR szDeliminter  )
{
	TCHAR buffer[33];
	TCHAR bufferDelimiter[2];
	bufferDelimiter[1] = 0;
	bufferDelimiter[0] = szDeliminter;

	szDest.append(_itoa(i, buffer, 10));
	
	if (szDeliminter) {
	    szDest.append(bufferDelimiter);
	}
	
	return std::string(buffer).length();

} // End AppendIntToString

#ifdef USE_64BIT
///////////////////////////////////////////////////////////////////////
int StringUtil::AppendIntToString( std::string &szDest, long i,
                                   TCHAR szDeliminter )
{
	TCHAR buffer[33];
	TCHAR bufferDelimiter[2];
	bufferDelimiter[1] = 0;
	bufferDelimiter[0] = szDeliminter;
    
    _ltoa(i, buffer, 10);
	szDest.append(buffer);

    /* Another mechanism for handling ltoa
    #if defined(WIN32)
	    sprintf(buffer, _T("%ld"), i);
    #else
        snprintf(buffer, sizeof(buffer), "%ld", i);
    #endif

	szDest.append(buffer);
    */

 	if (szDeliminter) {
 		szDest.append(bufferDelimiter);
 	}
 	
	return std::string(buffer).length();

} // End AppendIntToString
#else 
///////////////////////////////////////////////////////////////////////
int StringUtil::AppendIntToString( std::string &szDest, long i,
                                   TCHAR szDeliminter )
{
	TCHAR buffer[33];
	TCHAR bufferDelimiter[2];
	bufferDelimiter[1] = 0;
	bufferDelimiter[0] = szDeliminter;
    
    _ltoa(i, buffer, 10);
	szDest.append(buffer);

    /* Another mechanism for handling ltoa
    #if defined(WIN32)
	    sprintf(buffer, _T("%ld"), i);
    #else
        snprintf(buffer, sizeof(buffer), "%ld", i);
    #endif

	szDest.append(buffer);
    */

 	if (szDeliminter) {
 		szDest.append(bufferDelimiter);
 	}
 	
	return std::string(buffer).length();

} // End AppendIntToString

///////////////////////////////////////////////////////////////////////
int StringUtil::AppendIntToString( std::string &szDest, LONG64 i,
                                   TCHAR szDeliminter )
{
	TCHAR buffer[65];
	TCHAR bufferDelimiter[2];
	bufferDelimiter[1] = 0;
	bufferDelimiter[0] = szDeliminter;

	#ifdef WIN32
		szDest.append(_i64toa(i, buffer, 10));
	#else
		szDest.append(i64toa(i, buffer));
	#endif

 	if (szDeliminter) {
 		szDest.append(bufferDelimiter);
 	}
 	
	return std::string(buffer).length();

} // End AppendIntToString
#endif
///////////////////////////////////////////////////////////////////////
int StringUtil::AppendIntToString( std::string &szDest, unsigned short i,
                                    std::string szDelimiter )
{
	return AppendIntToString(  szDest, i,  *szDelimiter.c_str() );

} // End AppendIntToString

///////////////////////////////////////////////////////////////////////
int StringUtil::AppendIntToString( std::string &szDest, unsigned int i, std::string szDelimiter )
{
	return AppendIntToString(  szDest, i,  *szDelimiter.c_str() );

} // End AppendIntToString

#ifdef USE_64BIT
///////////////////////////////////////////////////////////////////////
int StringUtil::AppendIntToString( std::string &szDest, long i, std::string szDelimiter )
{
	return AppendIntToString(  szDest, i,  *szDelimiter.c_str() );

} // End AppendIntToString
#else
///////////////////////////////////////////////////////////////////////
int StringUtil::AppendIntToString( std::string &szDest, long i, std::string szDelimiter )
{
	return AppendIntToString(  szDest, i,  *szDelimiter.c_str() );

} // End AppendIntToString

///////////////////////////////////////////////////////////////////////
int StringUtil::AppendIntToString( std::string &szDest, LONG64 i, std::string szDelimiter )
{
	return AppendIntToString(  szDest, i,  *szDelimiter.c_str() );

} // End AppendIntToString
#endif
///////////////////////////////////////////////////////////////////////
std::string StringUtil::ConvertSpaces( std::string szIn )
{
	return ReplaceAll(szIn, WIDE_SPACE, WIDE_ALT_SPACE);

} // End ConvertSpaces

///////////////////////////////////////////////////////////////////////
std::string StringUtil::UnConvertSpaces( std::string szIn )
{
	return ReplaceAll(szIn, WIDE_ALT_SPACE, WIDE_SPACE);

} // End ConvertSpaces

///////////////////////////////////////////////////////////////////////
std::string StringUtil::ConvertCommas(  std::string szIn )
{
    return StringUtil::ReplaceAll(szIn, WIDE_COMMA, WIDE_ALT_COMMA);

} // End ConvertCommas

///////////////////////////////////////////////////////////////////////
std::string StringUtil::UnConvertCommas( std::string szIn )
{
    return StringUtil::ReplaceAll(szIn, WIDE_ALT_COMMA, WIDE_COMMA);

} // End UnConvertCommas

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper to SplitString to reverse find the location of the
//      argument flag, looking to solve /<argument>=<value> where
//      the argument flag, /, can either be / or // or - or --
// Requires:
//      szInput - source string
//      int iPos - starting position
// Returns:
//      int iArgFlagPos - position of the argument flag, -1 if
//      failure
int GetArgFlagPos(std::string szInput, int iPos)
{
    int argFlagPos = -1;

    // Look for a forward slash e.g. /<argument>=<value>
    argFlagPos = (int)szInput.rfind(_T("/"), iPos);
    if (argFlagPos != -1) {
        return argFlagPos;
    }

    // Look for a double forward slash e.g. //<argument>=<value>
    argFlagPos = (int)szInput.rfind(_T("//"), iPos);
    if (argFlagPos != -1) {
        return argFlagPos;
    }

    // Look for a double forward slash e.g. //<argument>=<value>
    argFlagPos = (int)szInput.rfind(_T("//"), iPos);
    if (argFlagPos != -1) {
        return argFlagPos;
    }

    // Look for a dash e.g. -<argument>=<value>
    argFlagPos = (int)szInput.rfind(_T("-"), iPos);
    if (argFlagPos != -1) {
        return argFlagPos;
    }

    // Look for a double dash e.g. --<argument>=<value>
    argFlagPos = (int)szInput.rfind(_T("--"), iPos);
    return argFlagPos;

} // End GetArgFlagPos

///////////////////////////////////////////////////////////////////////
// Purpose:
//     Split a string into a list of sub strings using spaces and quotes
//     This version steps through the string (whereas the following
//     version uses "find" to parse out the delimters).  With quotes,
///    there is a need to ensure matching quotes, skipping spaces between
//     quotes, etc.
// Requires:
//     szInput			- source
//     listResults		- the list of strings
//     bIncludeEmpties	- bool to control if an empty string is added
//                        to the list or dropped
// Returns: the number of items in the list
int StringUtil::SplitString(const string& szInput, vector<string>& listResults,
                            bool bIncludeEmpties)
{
	int iPos = 0;
	int spacePos = -1;
	int quotePos = -1;
	int argFlagPos = -1;

	string szDelimiter = _T(" ");
	string szQuote = _T("\"");
	string szArgFlag = _T("/");
	string szEquals = _T("=");

	int sizeDelimiter = (int)szDelimiter.size();
	int sizeQuote = (int)szQuote.size();

	int sizeInput = (int)szInput.size();

	// clear the list of results
	listResults.clear();

	// if the szInput string or szDelimiter is an empty string, then no sub strings could be found
	if ( sizeInput == 0 ) {
		return 0;
	}

	spacePos = (int)szInput.find (szDelimiter, 0);
	quotePos = (int)szInput.find(szQuote, 0);

	if ( spacePos < 0 ) {
		// no szDelimiters were found, check for matching quotes before returning
		if (quotePos < 0) {
		    listResults.push_back(szInput);
		    return 1;
		}
	}

	int nQuoteNeeded = (quotePos != -1) ? 2 : 0;
	bool bSpaceNeeded = false;

	if (spacePos != -1) {
	    // Found a space, before the first quote
	    if (quotePos != -1) {
	        if (spacePos < quotePos) {
	            bSpaceNeeded = true;
	        }
	    }
	    else {
            bSpaceNeeded = true;
	    }
	}

	while ( (spacePos != -1) || (quotePos != -1) ) {
		string szTemp(_T(""));

	    // Space found first?
	    if ( bSpaceNeeded ) {

            // If the user enters a space before anything else, we'll ignore
            // the space?
	        if (iPos == 0) {
			    szTemp = szInput.substr( iPos, spacePos - iPos );

                // Put the argument in the vector of arguments
	            if( bIncludeEmpties || ( szTemp.size() > 0 ) ){
		            listResults.push_back(szTemp);
	            }

			    bSpaceNeeded = false;
	        }
	        else
	        {
			    if (spacePos >= iPos) {

    			    szTemp = szInput.substr( iPos, spacePos - iPos );

                    // Put the argument in the vector of arguments
	                if( bIncludeEmpties || ( szTemp.size() > 0 ) ){
		                listResults.push_back(szTemp);
	                }

			        bSpaceNeeded = false;
			    }
			}

            iPos = spacePos + sizeDelimiter;
            if (iPos < sizeInput) {

                // Find the next space from the current space position
	            spacePos = (int)szInput.find (szDelimiter, iPos);
	        }
	        else {
	            spacePos = -1;
	            bSpaceNeeded = false;
	        }

	    }
	    else if (nQuoteNeeded > 0) {

	        if (nQuoteNeeded < 2) {

	            // If there's an = character before this set of quotes, then
	            // we're looking at a /<agument>=<value> - we need the whole
	            // substring.
	            string szTemp = szInput.substr((iPos - 2), 1);

	            if ( (iPos > 0) && (szInput.substr((iPos - 2), 1) == szEquals ) ) {
	                // argFlagPos = (int)szInput.rfind(szArgFlag, iPos);
	                argFlagPos = GetArgFlagPos(szInput, iPos);
	                szTemp = szInput.substr( argFlagPos, quotePos - argFlagPos + 1 );
	            }
	            else {
	                szTemp = szInput.substr( iPos, quotePos - iPos );
	            }

                // Put the argument in the vector of arguments
	            if( bIncludeEmpties || ( szTemp.size() > 0 ) ){
		            listResults.push_back(szTemp);
	            }

	            nQuoteNeeded = 0;
	        }

            iPos = quotePos + sizeQuote;
            if (iPos < sizeInput) {
                // Find the next quote from the current quote position
                quotePos = (int)szInput.find (szQuote, iPos);
                if (quotePos != -1) {
                    // Do we need a matching quote or did we find the fist quote of
                    // a pair.
                    nQuoteNeeded = (nQuoteNeeded == 0) ? 2 : 1;
                }
            }
            else {
                quotePos = -1;
                nQuoteNeeded = 0;
            }

	        if ( nQuoteNeeded && (quotePos < 0) ) {
	            // Matching quote not found.
	            string szArg = szInput.substr(iPos);
			    throw (TCLAP::CmdLineParseException(_T("Couldn't find matching quote for argument."),
			        szArg.c_str()) );
                return -1;
	        }

	    }

	    // Check for next quote or space
        bSpaceNeeded = false;
	    if (spacePos != -1) {
            bSpaceNeeded = true;

	        if (nQuoteNeeded == 2) {
	            if (quotePos < spacePos) {
	                // Keep the space position, since it may occur
	                // after the second quote of the pair.
	                bSpaceNeeded = false;
	            }
	        }
	        else if (nQuoteNeeded == 1) {
	            if (quotePos > spacePos) {
	                // This space position is obsolete.  Find the next one after
	                // the quote position.
    	            spacePos = (int)szInput.find (szDelimiter, quotePos);
                    bSpaceNeeded = false;
	            }
	            else {
	                bSpaceNeeded = false;
	            }
	        }
	    }

	} // End of while

    // Check for the last bit of the string
    if ( (spacePos < 0) && (quotePos < 0) ) {
        // No more quotes or spaces - pull off the last part of the string.
        if (iPos < sizeInput) {
            string szTemp = szInput.substr( iPos );

            // Put the argument in the vector of arguments
            if( bIncludeEmpties || ( szTemp.size() > 0 ) ){
                listResults.push_back(szTemp);
            }
        }

    }

	return (int)listResults.size();

} // End SplitString

///////////////////////////////////////////////////////////////////////
// Purpose:
//     Split a string into a list of sub strings using a delimiter
// Requires:
//     szInput			- source
//     szDelimiter		- the separator
//     listResults		- the list of strings
//     bIncludeEmpties	- bool to control if an empty string is added
//                        to the list or dropped
// Returns: the number of items in the list
int StringUtil::SplitString(const string& szInput, const string& szDelimiter,
                            vector<string>& listResults, bool bIncludeEmpties)
{
	int iPos = 0;
	int newPos = -1;
	int sizeDelimiter = (int)szDelimiter.size();
	int sizeInput = (int)szInput.size();

	// clear the list of results
	listResults.clear();

	// if the szInput string or szDelimiter is an empty string, then no sub strings could be found
	if(  ( sizeInput == 0 ) || ( sizeDelimiter == 0 ) ) {
		return 0;
	}

	vector<int> positions;
	newPos = (int)szInput.find (szDelimiter, 0);
	if( newPos < 0 ) {
		// no szDelimiters were found, just return the szInput string
		listResults.push_back(szInput);
		return 1;
	}

	// look for all the szDelimiter positions in the szInput string
	int numFound = 0;
	while( newPos >= iPos ) {
		numFound++;
		positions.push_back(newPos);
		iPos = newPos;
		newPos = (int)szInput.find (szDelimiter, iPos + sizeDelimiter);
	}

	// extract all the sub strings based on the szDelimiter positions
	for( int nIndex = 0; nIndex <= (int)positions.size(); ++nIndex ) {
		string szTemp(_T(""));
		if ( nIndex == 0 ) {
			szTemp = szInput.substr( nIndex, positions[nIndex] );
		}
		else
		{
			int offset = positions[nIndex-1] + sizeDelimiter;
			if ( offset < sizeInput ) {
				if ( nIndex == (int)positions.size() ) {
					szTemp = szInput.substr(offset);
				}
				else if ( nIndex > 0 ) {
					szTemp = szInput.substr( offset,
					    positions[nIndex] - positions[nIndex-1] - sizeDelimiter );
				}
			}
		}
		if( bIncludeEmpties || ( szTemp.size() > 0 ) ){
			listResults.push_back(szTemp);
		}
	}
	return (int)listResults.size();

} // End SplitString

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Manipulation of comma delineated lists
//      RemoveFromCommaList ( "12,45,34" , "45" ) -> "12,34"
std::string StringUtil::RemoveFromCommaList( const std::string szIn,
                                             const std::string szItemsToRemove,
                                             bool bRemoveOnce /*false*/)
{
	vector<std::string> listStrings;
	int nCount = StringUtil::SplitString(szIn, StringUtil::WIDE_COMMA, listStrings, false);

	bool bAddComma = false;
	bool bRemoveEnabled = true;
	std::string szResult = _T("");

	if (nCount == 0) {
	    return szResult;
	}

	for ( int nIndex = 0; nIndex < nCount; nIndex++) {
		if ( (listStrings[nIndex] == szItemsToRemove) && bRemoveEnabled ) {
			// do nothing
			if (bRemoveOnce) {
			    bRemoveEnabled = false;
			}
		}
		else {
			szResult+= (bAddComma ? StringUtil::WIDE_COMMA:_T(""))+listStrings[nIndex];
			bAddComma = true;
		}
	}

	return szResult;

} // End RemoveFromCommaList

///////////////////////////////////////////////////////////////////////
std::string StringUtil::ReplaceAll(std::string szIn, std::string szFrom, std::string szTo)
{
	std::string::size_type foundPos = szIn.find(szFrom);
	while (foundPos != std::string::npos) {
		szIn.replace(foundPos, szFrom.length(), szTo);
		foundPos = szIn.find(szFrom, foundPos+szTo.length());
	}

    return szIn;

} // End ReplaceAll

///////////////////////////////////////////////////////////////////////
std::string StringUtil::EncryptString(unsigned long i1, unsigned long i2, std::string szPlainText)
{
    std::string szOut;
    EncryptString(i1, i2, szPlainText, szOut);
    return szOut;

} // End EncryptString

const UINT BLOWFISH_BLOCK_SIZE = 8;

///////////////////////////////////////////////////////////////////////
void StringUtil::EncryptString(unsigned long i1, unsigned long i2, std::string szPlainText,
                               std::string& szCipherText)
{
	int nSourceLen = (int)szPlainText.size();
    if (nSourceLen <= 0) {
        return; // nothing to encrypt
    }

    // init the Blowfish cipher
    unsigned char key[17];

    // invert the ft bits for mild obfuscation
    sprintf((char *)key, "%08X%08X", ~i1, ~i2 );

    // null terminate just in case
    key[16] = '\0';
    CBlowFish bf(key, 16);

    //-----------------------------------------------------------------
    // convert to ANSI chars, encrypt, convert to hex then assign back.
    //-----------------------------------------------------------------

    // We need to add a zero - but need to count i as part of the length
	nSourceLen++;
    unsigned char * strRaw = new UCHAR[nSourceLen];
	memset(strRaw, 0, nSourceLen * sizeof(char));

    // Diomede: not working with Unicode
    // u2a(szPlainText.c_str(), (char *)strRaw, nSourceLen);
    strncpy((char*)strRaw, szPlainText.c_str(), nSourceLen);

    // ALL CHAR* END WITH A 0
	strRaw[nSourceLen-1]= 0;

	UINT actualSizeInModBlocks = (((nSourceLen/BLOWFISH_BLOCK_SIZE)+1)*BLOWFISH_BLOCK_SIZE);

    int nEncLength = (actualSizeInModBlocks+2) + 1;
    unsigned char * strEnc = new UCHAR[nEncLength];
	memset(strEnc, 0,  nEncLength * sizeof(char));
	strEnc[nEncLength-1]= 0;

    // Ensure the len is mod 8 and no larger than buffer
    bf.Encrypt(strRaw, strEnc, actualSizeInModBlocks );

    // Blowfish will be twice as big
    int nHexEncLength = (actualSizeInModBlocks*2) + 1;

    unsigned char * strHexEncoded = new UCHAR[nHexEncLength];
	memset(strHexEncoded, 0, (nHexEncLength * sizeof(char)));
	strHexEncoded[nHexEncLength-1]= 0;

    CharStr2HexStr(strEnc, (char *)strHexEncoded, actualSizeInModBlocks ); // len);

    // The following leaves around the allocated buffers.  The key to successfully
    // deleting all the above buffers, is to ensure that each is ended with a 0.
    // The original code did not do this....

    // szCipherText = (char *)strHexEncoded;

    int nOutSize = (int)strlen( (char*)strHexEncoded);
    char* pOutBuffer = new char[nOutSize+1];
	memset(pOutBuffer, 0, (nOutSize+1) * sizeof(char) );

    strncpy(pOutBuffer, (char*)strHexEncoded, nOutSize);
    pOutBuffer[nOutSize]=0;
    szCipherText = pOutBuffer;

    delete [] pOutBuffer;

	delete [] strHexEncoded;
	delete [] strRaw;
	delete [] strEnc;

} // End EncryptString

///////////////////////////////////////////////////////////////////////
std::string StringUtil::DecryptString(unsigned long i1, unsigned long i2, std::string szCipherText)
{
   std::string szOut = _T("");
   DecryptString(i1, i2, szCipherText, szOut);
   return szOut;

} // End DecryptString

///////////////////////////////////////////////////////////////////////
void StringUtil::DecryptString(unsigned long i1, unsigned long i2,
		                       std::string szCipherText, std::string& szPlainText,
		                       int nOrigPlainLength /*0*/)
{
	int nSourceLen = (int)szCipherText.size();
    if (nSourceLen <= 0) {
        return;
    }

	unsigned char* szEncString = new unsigned char[nSourceLen + 1];
	memset(szEncString, 0, (nSourceLen + 1) * sizeof(char) );

	for (int nIndex = 0; nIndex < nSourceLen; nIndex ++) {
	    szEncString[nIndex] = szCipherText[nIndex];
	}
	szEncString[nSourceLen] = 0;

	//-----------------------------------------------------------------
	// Key
    // invert the ft bits.  this corresponds to the key munging used
    // in the encryption routine above.
	//-----------------------------------------------------------------
    unsigned char key[17];
    sprintf((char *)key, "%08X%08X", ~i1, ~i2);
    key[16] = 0;
    CBlowFish bf(key, 16);

	//-----------------------------------------------------------------
    // convert from hex to chars, decrypt (and if using unicode, convert
    // to unicode.
	// if (len<8) len=8;
	//-----------------------------------------------------------------
	int nHalfSourceLen = nSourceLen/2;
    unsigned char* strRaw = new unsigned char [nSourceLen*3];
	unsigned char* strEnc = new unsigned char [nSourceLen+1];

	memset(strEnc, 0, sizeof(strEnc));
    HexStr2CharStr((char *)szEncString, strEnc, min( (((nSourceLen/8)+1)*8), nHalfSourceLen ) ); //len);

    //len /= 2; // converting from hex to char cuts the byte count in half
 	UINT actualSizeInModBlocks = (((nSourceLen/BLOWFISH_BLOCK_SIZE))*BLOWFISH_BLOCK_SIZE);

 	// Ensure the len is mod 8 and no larger than buffer
 	int nOutSize = nOrigPlainLength;
 	if (nOutSize == 0) {
 	    nOutSize = actualSizeInModBlocks;
 	}

    bf.Decrypt(strEnc, strRaw, actualSizeInModBlocks );

    // Set the plain text to the raw buffer.

    // strRaw[nHalfSourceLen] = '\0';
    // szPlainText = (char*)&strRaw[0];

    char* pOutBuffer = new char[nOutSize];
	memset(pOutBuffer, 0, (nOutSize) * sizeof(char) );

    strncpy(pOutBuffer, (char*)strRaw, nOutSize);
    szPlainText = pOutBuffer;
    delete []pOutBuffer;

	delete []szEncString;
	delete []strRaw;
	delete []strEnc;

} // End DecryptString

///////////////////////////////////////////////////////////////////////
std::string	StringUtil::EncryptString( ULONG64 i, std::string szPlainText)
{
	unsigned long d1 = (unsigned long)(i&0xffffffff);
	unsigned long d2 = (unsigned long)((i>>32)&0xffffffff);
	return EncryptString(d1,d2,szPlainText);

} // End EncryptString

///////////////////////////////////////////////////////////////////////
std::string	StringUtil::DecryptString( ULONG64 i, std::string szCipherText)
{
	unsigned long d1 = (unsigned long)(i&0xffffffff);
	unsigned long d2 = (unsigned long)((i>>32)&0xffffffff);
	return DecryptString(d1, d2, szCipherText);

} // End EncryptString

///////////////////////////////////////////////////////////////////////
unsigned long StringUtil::ConvertStringIntoEncryptionNumber( std::string szIn )
{
	ULONG64 i64Number = 0;
	szIn += _T("    ");
	const TCHAR *tmpIn  = CS(szIn);
	int	nInLength = (int)szIn.size();

	//-----------------------------------------------------------------
	//  ULONG64         88 77 66 55 44 33 22 11
	//  unsigned int    44 33 22 11
	//-----------------------------------------------------------------

	for ( int x = 0; x < nInLength; x++) {
		i64Number ^= (i64Number>>32);
		i64Number ^= tmpIn[x];
		i64Number <<= 8; // Shift 8
	}

	return ULONG(i64Number & 0xffffffff);

} // End ConvertStringIntoEncryptionNumber

///////////////////////////////////////////////////////////////////////
std::string StringUtil::EncryptString(std::string i1, std::string i2,
                                      std::string szPlainText)
{
	ULONG d1 = ConvertStringIntoEncryptionNumber(i1);
	ULONG d2 = ConvertStringIntoEncryptionNumber(i2);
	return EncryptString(d1, d2, szPlainText);


} // End EncryptString

///////////////////////////////////////////////////////////////////////
std::string StringUtil::DecryptString(std::string i1, std::string i2,
                                      std::string szCipherText)
{
	ULONG d1 = ConvertStringIntoEncryptionNumber(i1);
	ULONG d2 = ConvertStringIntoEncryptionNumber(i2);
	return DecryptString(d1, d2, szCipherText);

} // End DecryptString

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Debug function to dump a std::string vector to client log
void StringUtil::DumpStringVectorToClientLog( vector<std::string> listDebug)
{
	for( UINT x = 0; x < listDebug.size(); x++) {
		ClientLog(WHEREIAM,ALWAYS_COMP,ST,true,_T("%3d %s"), x, listDebug[x].c_str());
	}

} // End DumpStringVectorToClientLog

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Return a list of items that exist in a but not b ( a-b )
//      a = abc  b= ab   r=c
vector<std::string> StringUtil::Subtract( vector<std::string> listOperand1,
                                          vector<std::string> listOperand2 )
{
	vector<std::string> listOut;
	for( vector<std::string>::iterator iter = listOperand1.begin(); iter!= listOperand1.end(); iter++ ) {
		if ( false== StringUtil::IsIn(listOperand2, *iter) ) {
			listOut.push_back(*iter);
		}
	}

	return listOut;

} // End Subtract

//////////////////////////////////////////////////////
// Returns true if item is in source
bool StringUtil::IsIn(std::vector<std::string>listInput, std::string szCompare)
{
	for( vector<std::string>::iterator iter = listInput.begin(); iter!=listInput.end(); iter++) {
		if ( *iter == szCompare) return true;
	}
	return false;

} // End IsIn

///////////////////////////////////////////////////////////////////////
std::string StringUtil::SerializeStringList( list<std::string> listInput,
                                             const std::string szDelimiter)
{
	std::string szOut;
	for( list<std::string>::iterator iter = listInput.begin(); iter != listInput.end(); iter++ ) {
		szOut += *iter;
		szOut += szDelimiter;
	}
	return szOut;

} // End SerializeStringList

///////////////////////////////////////////////////////////////////////
std::string StringUtil::SerializeStringVector( vector<std::string> listInput,
                                               const std::string szDelimiter)
{
	std::string szOut;
	for( vector<std::string>::iterator iter = listInput.begin(); iter != listInput.end(); iter++ ) {
		if (iter != listInput.begin()) szOut += szDelimiter;
		szOut +=*iter;
	}
	return szOut;

} // End SerializeStringVector

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Validate that data contains only ascii
// Requires:
//      szIn: input string
// Returns: true if all ascii, false otherwise
bool StringUtil::VerifyIsASCII( const std::string szIn)
{
	for( unsigned int i=0; i< szIn.size(); i++ ) {
		TCHAR szChar = szIn[i];
		if ( 0 == isprint(szChar) ) {
			return false;
		}
	}
	return true;

} // End VerifyIsASCII

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Validate that data contains only digits
// Requires:
//      szIn: input string
// Returns: true if all ascii, false otherwise
bool StringUtil::VerifyIsDigit( const std::string szIn)
{
	for( unsigned int i=0; i< szIn.size(); i++ ) {
		TCHAR szChar = szIn[i];
		if ( 0 == isdigit(szChar) ) {
			return false;
		}
	}
	return true;

} // End VerifyIsDigit

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Validate that data contains only hex digits
// Requires:
//      szIn: input string
// Returns: true if all hex, false otherwise
bool StringUtil::VerifyIsHex( const std::string szIn)
{
	for( unsigned int i=0; i< szIn.size(); i++ ) {
		TCHAR szChar = szIn[i];
		if ( 0 == isxdigit(szChar) ) {
			return false;
		}
	}
	return true;

} // End VerifyIsHex

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function to format numbers (see CodeProject
//      Format File Sizes in Human Readable Format) - converted
//      to STL and double
// Requires:
//      l64Number: number to format
//      szFormattedNumber: returned formatted number.
// Returns: nothing
void StringUtil::InsertSeparator (double dwNumber, std::string& szFormattedNumber)
{
    szFormattedNumber = _format(_T("%lf"), dwNumber);

    for (int nIndex = (int)szFormattedNumber.length()-3; nIndex > 0; nIndex -= 3) {
        szFormattedNumber.insert(nIndex, _T(","));
    }

} // End InsertSeparator

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Helper function for FormatByteSize (see CodeProject
//      Format File Sizes in Human Readable Format) - converted
//      to STL and LONG64
// Requires:
//      l64Number: number to format
//      szFormattedNumber: returned formatted number.
// Returns: nothing
void StringUtil::InsertSeparator (LONG64 l64Number, std::string& szFormattedNumber)
{
    #ifdef WIN32
        szFormattedNumber = _format(_T("%I64d"), l64Number);
    #else
        szFormattedNumber = _format(_T("%lld"), l64Number);
    #endif

    for (int nIndex = (int)szFormattedNumber.length()-3; nIndex > 0; nIndex -= 3) {
        szFormattedNumber.insert(nIndex, _T(","));
    }

} // End InsertSeparator

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Formats the bytes as nnnnn.n (TB|B|GB|MB) (see CodeProject
//      Format File Sizes in Human Readable Format) - converted
//      to STL and LONG64
// Requires:
//      l64ByteSize: input byte size
//      szByteSize: returned formatted byte size.
//      szByteSizeType: returned formatted byte size type.
//      nMaxNumber: max number within the category (e.g. if
//                  nMaxNumber is 999, 1000 KB is converted
//                  to 0.98 MB.
// Returns: true if successful, false otherwise
bool StringUtil::FormatByteSize(LONG64 l64ByteSize, std::string& szByteSize,
                                std::string& szByteSizeType, int nMaxNumber /*0*/)
{
    //-----------------------------------------------------------------
    //  kilo-   1000^1   1024^1 = 2^10 = 1,024
    //  mega-   1000^2   1024^2 = 2^20 = 1,048,576
    //  giga-   1000^3   1024^3 = 2^30 = 1,073,741,824
    //  tera-   1000^4   1024^4 = 2^40 = 1,099,511,627,776
    //  peta-   1000^5   1024^5 = 2^50 = 1,125,899,906,842,624
    //  exa-    1000^6   1024^6 = 2^60 = 1,152,921,504,606,846,976
    //  zetta-  1000^7   1024^7 = 2^70 = 1,180,591,620,717,411,303,424
    //  yotta-  1000^8   1024^8 = 2^80 = 1,208,925,819,614,629,174,706,176
    //-----------------------------------------------------------------

    static const LONG64 l64KB = 1024;           // Kilobyte
    static const LONG64 l64MB = 1024 * l64KB;   // Megabyte
    static const LONG64 l64GB = 1024 * l64MB;   // Gigabyte
    static const LONG64 l64TB = 1024 * l64GB;   // Terabyte
    static const LONG64 l64PB = 1024 * l64TB;   // Petabyte

    szByteSizeType = _T("");
    szByteSize = _T("");

    LONG64 l64Number, l64Remainder;
    std::string szNumber = _T("");
    std::string szRemainder = _T("");

    bool bAddRemainder = true;

    if (l64ByteSize < l64KB) {
        InsertSeparator(l64ByteSize, szByteSize);
        szByteSizeType = _T("B");
        bAddRemainder = false;
    }
    else {
        if (l64ByteSize < l64MB) {
            l64Number = l64ByteSize / l64KB;
            l64Remainder = (l64ByteSize * 100 / l64KB) % 100;

            // Max number dictates whether we use the bytes
            // caculated or a fractional value in the next
            // category.

            if ( (nMaxNumber > 0) && (l64Number > nMaxNumber)) {
                szNumber = _T("0");
                l64Remainder = (l64ByteSize * 100 / l64MB) % 100;
                szByteSizeType = _T("MB");
            }
            else {
                l64Remainder = (l64ByteSize * 100 / l64KB) % 100;
                InsertSeparator(l64Number, szNumber);
                szByteSizeType = _T("KB");
            }

            #ifdef WIN32
                szRemainder = _format(_T(".%02I64d"), l64Remainder);
            #else
                szRemainder = _format(_T(".%02lld"), l64Remainder);
            #endif
            szByteSize = szNumber + szRemainder;
        }
        else {
            // Megabyte
            if (l64ByteSize < l64GB) {
                l64Number = l64ByteSize / l64MB;

                // Max number dictates whether we use the bytes
                // caculated or a fractional value in the next
                // category.

                if ( (nMaxNumber > 0) && (l64Number > nMaxNumber)) {
                    szNumber = _T("0");
                    l64Remainder = (l64ByteSize * 100 / l64GB) % 100;
                    szByteSizeType = _T("GB");
                }
                else {
                    l64Remainder = (l64ByteSize * 100 / l64MB) % 100;
                    InsertSeparator(l64Number, szNumber);
                    szByteSizeType = _T("MB");
                }

                #ifdef WIN32
                    szRemainder = _format(_T(".%02I64d"), l64Remainder);
                #else
                    szRemainder = _format(_T(".%02lld"), l64Remainder);
                #endif
                szByteSize = szNumber + szRemainder;
            }
            else {
                // Gigabyte
                if (l64ByteSize < l64TB) {
                    l64Number = l64ByteSize / l64GB;

                    // Max number dictates whether we use the bytes
                    // caculated or a fractional value in the next
                    // category.

                    if ( (nMaxNumber > 0) && (l64Number > nMaxNumber)) {
                        szNumber = _T("0");
                        l64Remainder = (l64ByteSize * 100 / l64TB) % 100;
                        szByteSizeType = _T("TB");
                    }
                    else {
                        l64Remainder = (l64ByteSize * 100 / l64GB) % 100;
                        InsertSeparator(l64Number, szNumber);
                        szByteSizeType = _T("GB");
                    }

                    #ifdef WIN32
                        szRemainder = _format(_T(".%02I64d"), l64Remainder);
                    #else
                        szRemainder = _format(_T(".%02lld"), l64Remainder);
                    #endif
                    szByteSize = szNumber + szRemainder;
                }
                else {
                    // Terabyte
                    if (l64ByteSize >= l64TB) {
                        l64Number = l64ByteSize / l64TB;

                        // Max number dictates whether we use the bytes
                        // caculated or a fractional value in the next
                        // category.

                        if ( (nMaxNumber > 0) && (l64Number > nMaxNumber)) {
                            szNumber = _T("0");
                            l64Remainder = (l64ByteSize * 100 / l64PB) % 100;
                            szByteSizeType = _T("PB");
                        }
                        else {
                            l64Remainder = (l64ByteSize * 100 / l64TB) % 100;
                            InsertSeparator(l64Number, szNumber);
                            szByteSizeType = _T("TB");
                        }

                        #ifdef WIN32
                            szRemainder = _format(_T(".%02I64d"), l64Remainder);
                        #else
                            szRemainder = _format(_T(".%02lld"), l64Remainder);
                        #endif
                        szByteSize = szNumber + szRemainder;
                    }
                }
            }
        }
    }

    // Display decimal points only if needed
    // another alternative to this approach is to check before calling str.Format, and
    // have separate cases depending on whether l64Remainder == 0 or not.

    if (bAddRemainder) {
        // Remainder added in general, in cases except B(ytes)
        return true;
    }

    char szByteSizeBuffer[MAX_PATH];
    strcpy(szByteSizeBuffer, szByteSize.c_str());
    szByteSize = _tcsstrrem(szByteSizeBuffer, _T(".00"));

    return true;

} // End FormatByteSize

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Formats the bytes into bandwidth speed (kps)
// Requires:
//      l64ByteSize: input byte size
//      elapsedTime: elapsed time
//      szKiloBitsPerSec: returned formatted kbs.
//      szKiloBitsPerSecType: currently, always using kbs
// Returns: true if successful, false otherwise
bool StringUtil::FormatBandwidth(LONG64 l64ByteSize,
                                 const boost::posix_time::time_duration& elapsedTime,
                                 std::string& szKiloBitsPerSec,
                                 std::string& szKiloBitsPerSecType)
{
   szKiloBitsPerSec = _T("0");
   szKiloBitsPerSecType = _T("kbps");

    if (l64ByteSize == 0) {
       return false;
    }

    LONG64 l64Number = 0;
    LONG64 l64Remainder = 0;

    std::string szNumber = _T("");
    std::string szRemainder = _T("");

    static const LONG64 l64KB = 1000;

    double dblTotalSeconds =  static_cast<double>(elapsedTime.total_milliseconds()) / 1000;
    if (dblTotalSeconds == 0) {
       return false;
    }

    double dblBitPerSec = (l64ByteSize * 8) / dblTotalSeconds;

    l64Number = static_cast<LONG64>(dblBitPerSec / l64KB);
    l64Remainder = (static_cast<LONG64>(dblBitPerSec) * 100 / l64KB) % 100;

    InsertSeparator(l64Number, szNumber);

    #ifdef WIN32
        szRemainder = _format(_T(".%02I64d"), l64Remainder);
    #else
        szRemainder = _format(_T(".%02lld"), l64Remainder);
    #endif
    szKiloBitsPerSec = szNumber + szRemainder;
    szKiloBitsPerSecType = _T("kbps");

    // Display decimal points only if needed
    // another alternative to this approach is to check before calling str.Format, and
    // have separate cases depending on whether l64Remainder == 0 or not.

    char szKiloBitsPerSecBuffer[MAX_PATH];
    strcpy(szKiloBitsPerSecBuffer, szKiloBitsPerSec.c_str());
    szKiloBitsPerSec = _tcsstrrem(szKiloBitsPerSecBuffer, _T(".00"));

    return true;

} // End FormatBandwidth

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Formats the rate or monetary value into a readable format.
//      This assumes a 2 digit precision - use this if the locale
//      method does not work.
// Requires:
//      fAmount: input amount
//      szAmount: returned formatted amount.
//      nPrecision: float precision
// Returns: true if successful, false otherwise
bool StringUtil::SimpleFormatRate(float fAmount, std::string& szAmount, int nPrecision /*2*/)
{
	const std::string szRadix = _T(".");
	const std::string szThousands = _T(",");
	const std::string szUnit = _T("$");

	unsigned long lValue = (unsigned long) ((fAmount * (nPrecision*10.0)) + .5);
	std::string szFormat, szDigit;
	int nIndex = -2;
	do {
		if (nIndex == 0) {
			szFormat = szRadix + szFormat;
		}

		if ((nIndex > 0) && (!(nIndex % 3))) {
			szFormat = szThousands + szFormat;
		}

		szDigit = ((UINT)lValue % 10) + '0';
		szFormat = szDigit + szFormat;
		lValue /= 10;
		nIndex++;
	} while ((lValue) || (nIndex < 1));

    szAmount = szUnit + szAmount;
    return true;

} // End SimpleFormatRate

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Formats the LONG64 number into a string with inserted
//      commas.
// Requires:
//      l64Number: input number
//      szInt64Number: returned formatted number.
//      szDefault: default value if the string is empty
// Returns: true if successful, false otherwise
bool StringUtil::FormatNumber(const LONG64& l64Number, std::string& szInt64Number,
                              std::string szDefault /*_T("")*/)
{
    szInt64Number = szDefault;
    if (!l64Number) {
        return false;
    }

    // May need to refine this for Linux...searching seems to show that
    // _i64toa is available on Linux systems.
	char szl64NumBuffer[1024];
	#ifdef WIN32
     	_i64toa(l64Number, szl64NumBuffer, 10);
	#else
     	i64toa(l64Number, szl64NumBuffer);
	#endif

    szInt64Number = std::string(szl64NumBuffer);

    // The number of commas to be appended
    int nCountChars = (int)(szInt64Number.length()-1)/3;
    if (nCountChars < 1) {
        return true;
    }

    char* szFirst = (char*)szInt64Number.c_str();
    char* szLast = szFirst + szInt64Number.length()-1;

    char* szBuffer = new char[szInt64Number.length() + nCountChars + 1];
    szBuffer[szInt64Number.length() + nCountChars] = 0;
    char* szBufferIndex = szBuffer + szInt64Number.length()+ nCountChars - 1;

    int nPlaceCounter = 4;
    while(szFirst <= szLast) {
        nPlaceCounter--;
        if (nPlaceCounter == 0) {
            *szBufferIndex = ',';
            nPlaceCounter = 4;
        }
        else {
            *szBufferIndex = *szLast;
            szLast--;
        }
        szBufferIndex--;
    }

    szInt64Number = std::string(szBuffer);
    delete[] szBuffer;
    return true;

} // End FormatNumber

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Formats the double number into a string with inserted
//      commas.
// Requires:
//      dblNumber: input number
//      szDblNumber: returned formatted number.
//      nPrecision: number of digits past the decimal
//      szDefault: default value if the string is empty
// Returns: true if successful, false otherwise
bool StringUtil::FormatNumber(const double& dblNumber, std::string& szDblNumber,
                              unsigned int nPrecision, std::string szDefault /*_T("")*/)
{
    szDblNumber = szDefault;
    if (!dblNumber) {
        return false;
    }

    char *szCurrent, *szLast;

    nPrecision = nPrecision > 5 ? 5 : nPrecision;

	char szDblNumBuffer[1024];
    sprintf(szDblNumBuffer,"%.*lf", nPrecision, dblNumber);

    if ((szCurrent = strchr(szDblNumBuffer,'.')) == NULL) {
        szCurrent = strlen(szDblNumBuffer) + szDblNumBuffer;
    }

    for(szLast = szDblNumBuffer; *szLast != '\0'; szLast++) { };
    for( ; szCurrent - szDblNumBuffer > 3; szLast++) {
       szCurrent -= 3;
       memmove(szCurrent+1, szCurrent, szLast-szCurrent+1);
       // Insert the comma
       *szCurrent = ',';
   }

    szDblNumber = std::string(szDblNumBuffer);
    return true;

} // End FormatNumber

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Formats the int number into a string with inserted
//      commas.
// Requires:
//      nNumber: input number
//      szIntNumber: returned formatted number.
//      szDefault: default value if the string is empty
// Returns: true if successful, false otherwise
bool StringUtil::FormatNumber(const int& nNumber, std::string& szIntNumber,
                                                  std::string szDefault /*_T("")*/)
{
    szIntNumber = szDefault;
    if (!nNumber) {
        return false;
    }

    // May need to refine this for Linux...searching seems to show that
    // _itoa is available on Linux systems.
	char szIntNumBuffer[1024];
     _itoa(nNumber, szIntNumBuffer, 10);

    szIntNumber = std::string(szIntNumBuffer);

    // The number of commas to be appended
    int nCountChars = (int)(szIntNumber.length()-1)/3;
    if (nCountChars < 1) {
        return true;
    }

    char* szFirst = (char*)szIntNumber.c_str();
    char* szLast = szFirst + szIntNumber.length()-1;

    char* szBuffer = new char[szIntNumber.length() + nCountChars + 1];
    szBuffer[szIntNumber.length() + nCountChars] = 0;
    char* szBufferIndex = szBuffer + szIntNumber.length()+ nCountChars - 1;

    int nPlaceCounter = 4;
    while(szFirst <= szLast) {
        nPlaceCounter--;
        if (nPlaceCounter == 0) {
            *szBufferIndex = ',';
            nPlaceCounter = 4;
        }
        else {
            *szBufferIndex = *szLast;
            szLast--;
        }
        szBufferIndex--;
    }

    szIntNumber = std::string(szBuffer);
    delete[] szBuffer;
    return true;

} // End FormatNumber

///////////////////////////////////////////////////////////////////////
// Purpose: Helper function to FormatDateAndTime
// Requires:
//      szFormat: the format string
//      pTM: pointer to a tm struct
// Returns: formatted date 
const char* StringUtil::MakeFormatDate(const char* szFormat, const struct tm* pTM)
{
    // Read-only static local storage is, by definition, thread safe
    static const size_t MAXBUFSIZE = 256;

    // Function interface is again uncluttered when using dynamic memory, 
    // and the function is also thread safe. However, the function is no 
    // longer self-contained in that the returned buffer address *must* be 
    // captured [usually by assignment to a variable], and the allocated
    // memory later explicitly deallocated
    char* szDateBuffer = (char*) malloc(MAXBUFSIZE);
    return (szDateBuffer != NULL && 
           (std::strftime(szDateBuffer, MAXBUFSIZE, szFormat, pTM) > 0)) ? szDateBuffer : NULL;

} // End MakeFormatDate

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Formats the input date string to yyyy-dd-mm.
// Requires:
//      tmDate: input date value
//      szFormattedDate: returned formatted date string.
// Returns: true if successful, false otherwise
bool StringUtil::FormatDate(time_t tmDate, std::string& szFormattedDate)
{
    if (tmDate == 0) {
        szFormattedDate = _T("none");
        return false;
    }

    szFormattedDate = _T("");
    struct tm* pLocalTM = std::localtime(&tmDate);

    const char* szDateBuffer = MakeFormatDate(_T("%Y-%m-%d"), pLocalTM);
    szFormattedDate = std::string(szDateBuffer);

    free( (void*)szDateBuffer); 
    szDateBuffer = NULL;
    return true;

    #if 0
    std::stringstream streamDate;

    // Note: "imbue" is responsible for cleaning up the allocated
    // time_facet.
    date_facet* pDateFacet = new date_facet();
    streamDate.imbue(std::locale(std::locale::classic(), pDateFacet));

    pDateFacet->format(_T("%Y-%m-%d"));
    pDateFacet->set_iso_extended_format();

    streamDate << tmDate;

    szFormattedDate = streamDate.str();
    return true;
    #endif

} // End FormatDate

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Formats the input date string to yyyy-dd-mm.
// Requires:
//      tmDateTime: input date and time value
//      szFormattedDate: returned formatted date string.
// Returns: true if successful, false otherwise
bool StringUtil::FormatDateAndTime(time_t tmDateTime, std::string& szFormattedDate)
{
    if (tmDateTime == 0) {
        szFormattedDate = _T("none");
        return false;
    }

    szFormattedDate = _T("");
    struct tm* pLocalTM = std::localtime(&tmDateTime);

    const char* szDateBuffer = MakeFormatDate(_T("%Y-%m-%d %H:%M:%S"), pLocalTM);
    szFormattedDate = std::string(szDateBuffer);

    free( (void*)szDateBuffer); 
    szDateBuffer = NULL;
    return true;

    #if 0
    // It would be delightful if this worked, but it doesn't and is not intended
    // to work well. The author for this Boost library does not make it easy to
    // translate a time_t to a ptime intentionally to get folks switched entirely
    // to using Boost.
    std::stringstream streamDate;

    // Note: "imbue" is responsible for cleaning up the allocated
    // time_facet.
    
    time_facet* pTimeFacet = new time_facet();
    pTimeFacet->format(_T("%Y-%m-%d %H:%M:%S%F%Q"));
    pTimeFacet->time_duration_format("%H:%M:%S %%S");

    streamDate.imbue(std::locale(std::locale::classic(), pTimeFacet));
    streamDate.str(_T(""));

    streamDate << boost::posix_time::from_time_t(tmDateTime);

    szFormattedDate = streamDate.str();
    return true;
    #endif    

} // End FormatDateAndTime

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Formats the input duration into HH::MM::SS.sss.
// Requires:
//      elapsedTime: input duration value
//      szFormattedTime: returned formatted time string.
//      szTimeType: duration type (hours, minutes, seconds)
//      nPrecision: timer output precision (e.g. 3.1, 3.11, etc.)
// Returns: true if successful, false otherwise
bool StringUtil::FormatDuration(const boost::posix_time::time_duration& elapsedTime,
        std::string& szFormattedTime, std::string& szTimeType, int nPrecision /*1*/)
{
    szTimeType = _T("");
    szFormattedTime = _T("");

    int nHours = elapsedTime.hours();

    LONG64 lMinutes = elapsedTime.minutes();

    double dblMicroseconds = static_cast<double>(elapsedTime.fractional_seconds());
    double dblMilliseconds = static_cast<double>(dblMicroseconds / time_duration::rep_type::res_adjust());

    double dblSeconds =  static_cast<double>(elapsedTime.seconds()) + dblMilliseconds;

    if (nHours > 0) {
        #ifdef WIN32
            szFormattedTime = _format(_T("%d:%02I64d:%.*lf"), nHours, lMinutes, nPrecision, dblSeconds);
        #else
            szFormattedTime = _format(_T("%d:%02lld:%.*lf"), nHours, lMinutes, nPrecision, dblSeconds);
        #endif
        szTimeType = _T("hours");

    }
    else if (lMinutes > 0) {
        #ifdef WIN32
            szFormattedTime = _format(_T("%I64d:%.*lf"), lMinutes, nPrecision, dblSeconds);
        #else
            szFormattedTime = _format(_T("%lld:%.*lf"), lMinutes, nPrecision, dblSeconds);
        #endif
        szTimeType = _T("minutes");
    }
    else {
        szFormattedTime = _format(_T("%.*lf"), nPrecision, dblSeconds);
        szTimeType = _T("seconds");
    }

    return true;

} // End FormatDuration

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Converts Microsoft error return to ascii message.
// Requires:
//      dwError: Error code returned from GetLastError
// Returns: string representation of the error.
std::string StringUtil::FormatErrorString(unsigned long dwError)
{
   std::string szError = _T("");

#ifdef WIN32
    LPTSTR lpMsgBuf;

    if (::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, dwError, 0, (LPTSTR)&lpMsgBuf, 0, NULL) == 0) {
		// Unknown error code %08x (%d)
		szError = _T("Unknown error");
	}
    else {
        // Success - copy the message into the string.  First,
        // remove any CRLF present.
		LPTSTR pLineFeed = _tcschr(lpMsgBuf, _T('\r'));
		if (pLineFeed != NULL) {
			*pLineFeed = _T('\0');
		}

		szError = lpMsgBuf;
		::LocalFree(lpMsgBuf);
	}

#endif

    return szError;

} // FormatErrorString

///////////////////////////////////////////////////////////////////////
// Purpose:
//      Trims the filename if it's longer than the given width,
//      appending with ellipsis if needed.
// Requires:
//      nMaxWidth: Max width for the filename
//      szInFileName: input filename
//      szFormattedDate: output filename.
// Returns: true if successful, false otherwise
bool StringUtil::TrimFileName(int nMaxWidth, std::string szInFileName,
                               std::string& szOutFileName)
{
    // Check the length - if no changes, return.
    if ( (int)szInFileName.length() <= nMaxWidth) {
        szOutFileName = szInFileName;
        return false;
    }

    // Otherwise, trim the string, and append ...
    szOutFileName = szInFileName.substr(0, nMaxWidth - 3);
    szOutFileName = szOutFileName + _T("...");

    return true;

} // End TrimFileName

///////////////////////////////////////////////////////////////////////
// MD5

///////////////////////////////////////////////////////////////////////
// Purpose: Outputs the MD5 checksum for a file.
// Requires:
//      szFilePath: file (with path) to be hased
//      pMD5Digest: returned digest for file
// Returns: 0 if the file was found and could be opened,
//          errno otherwise
int StringUtil::MakeFileMd5Digest(char* szFilePath, unsigned char* pMD5Digest)
{
    MD5_CTX state;
    int cbRead;
    FILE *pFile;
    char szBuffer[1024*16];

    pFile = fopen(szFilePath, "rb");
    if (pFile == NULL) {
        return errno;
    }

    cbRead = (int)fread(szBuffer, 1, sizeof(szBuffer), pFile);
    MD5_Init(&state);
    while (cbRead > 0) {
        MD5_Update(&state, (const MD5_CTX *)szBuffer, cbRead);
        cbRead = (int)fread(szBuffer, 1, sizeof(szBuffer), pFile);
    }

    MD5_Final(pMD5Digest, &state);
    return 0;

} // End MakeFileMd5Digest

///////////////////////////////////////////////////////////////////////
// Purpose: Outputs the MD5 sum of a character string. NULL is accepted
//          as an empty string.
// Requires:
//      szInStr: input string
//      pMD5Digest: returned digest for the string
// Returns: nothing
void StringUtil::MakeStringMd5Digest(char* szInStr, unsigned char* pMD5Digest)
{
    MD5_CTX state;

    MD5_Init(&state);
    if (szInStr) {
        MD5_Update(&state, (const MD5_CTX *)szInStr, strlen(szInStr));
    }
    MD5_Final(pMD5Digest, &state);

} // End MakeStringMd5Digest

///////////////////////////////////////////////////////////////////////
// Purpose: Formats a digest into a string. String must be at least
//          33 bytes long
// Requires:
//      szOutStr: output string
//      pMD5Digest: input digest
// Returns: nothing
void StringUtil::ConvertDigestToString(std::string& szOutStr, unsigned char* pMD5Digest)
{
    int nIndex = 0;
    char* szTmpOut = new char[MD5_DIGEST_LENGTH*2 + 1];
    char* szSave = szTmpOut;

    memset(szTmpOut, 0, (MD5_DIGEST_LENGTH*2 + 1) * sizeof(unsigned char));

    for (nIndex = 0; nIndex < 16; ++nIndex) {
        sprintf(szTmpOut, _T("%02x"), pMD5Digest[nIndex]);
        szTmpOut += 2;
    }

    szTmpOut = szSave;

    char* pOutBuffer = new char[MD5_DIGEST_LENGTH*2 + 1];
	memset(pOutBuffer, 0, (MD5_DIGEST_LENGTH*2 + 1) * sizeof(char) );

    strncpy(pOutBuffer, (char*)szTmpOut, MD5_DIGEST_LENGTH*2 + 1);
    szOutStr = pOutBuffer;

    delete [] pOutBuffer;
    delete [] szTmpOut;

} // End ConvertDigestToString

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// DiomedeMoneyPunct

///////////////////////////////////////////////////////////////////////
// Purpose: moneypunct FOR USA LOCALE
//          Defined here to prevent linking issues.  Pulled from
//          Dr. Dobbs Portal, The Facet money_put, P.J. Plauger
//          April 1, 1998

std::money_base::pattern moneyFormat = {
    money_base::symbol, money_base::space,
    money_base::sign, money_base::value};

///////////////////////////////////////////////////////////////////////
DiomedeMoneyPunct::pattern DiomedeMoneyPunct::do_pos_format() const
{
    return (moneyFormat);

} // End do_pos_format

///////////////////////////////////////////////////////////////////////
DiomedeMoneyPunct::pattern DiomedeMoneyPunct::do_neg_format() const
{
    return (moneyFormat);

} // End do_neg_format

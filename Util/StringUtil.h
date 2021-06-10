/*********************************************************************
 * 
 *  file:  StringUtil.h
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

///////////////////////////////////////////////////////////////////////
// StringUtil.h :
//
// Various string utilities for working with various data formats
// and encryption.
//

#ifndef __STRING_UTIL_H__
#define __STRING_UTIL_H__

#include "Stdafx.h"
#include "types.h"

#include <string>
#include <locale>
#include <iostream>
#include <iterator>

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/local_time/local_time.hpp"
#include <iostream>

using namespace boost::posix_time;
using namespace boost::local_time;
using namespace boost::gregorian;

namespace StringUtil
{
    static const std::string WIDE_NULL				= _T("");
    static const std::string WIDE_SPACE				= _T(" ");
    static const std::string WIDE_ALT_SPACE			= _T("#&0");
    static const std::string WIDE_COMMA				= _T(",");
    static const std::string WIDE_ALT_COMMA			= _T("#&1");
    static const std::string WIDE_FORWARD_SLASH		= _T("/");


    static const std::string WIDE_YES		        = _T("Yes");
    static const std::string WIDE_NO		        = _T("No");

    std::string& tolower(std::string& szIn);
    std::string& tolower( const std::string& szIn, std::string& szOut );

    char* itoa( int value, char* result, int base );
    char* i64toa(LONG64 l64Value, char *szResult);
    LONG64 atoi64(const char* szValue);

    void replaceall(std::string &szIn , const std::string& szFrom,
                    const std::string& szTo);

    const unsigned char* convert(const std::string& szIn);

    //--------------------------------------------------------------------
    // Encrypt a string using Blowfish
    //	i1 & i2 are obscruing data - same must be used for de/encrypt
    //--------------------------------------------------------------------
    void EncryptString( unsigned long i1, unsigned long i2, std::string szPlainText,
                        std::string& szCipherText);
    std::string	EncryptString( unsigned long i1, unsigned long i2, std::string szPlainText);
    void DecryptString( unsigned long i1, unsigned long i2, std::string szCipherText,
        std::string& szPlainText, int nOrigPlainLength=0);
    std::string	DecryptString( unsigned long i1, unsigned long i2, std::string szCipherText);

    //--------------------------------------------------------------------
    // Given two strings as the keys, en/decrypt string
    //--------------------------------------------------------------------
    unsigned long ConvertStringIntoEncryptionNumber( std::string szIn );
    std::string	EncryptString( std::string i1, std::string i2, std::string szPlainText);
    std::string	DecryptString( std::string i1, std::string i2, std::string szCipherText);
    std::string	EncryptString( ULONG64 i, std::string szPlainText);
    std::string	DecryptString( ULONG64 i, std::string szCipherText);

    //--------------------------------------------------------------------
    // Validate that data contains only the specific type of data
    //--------------------------------------------------------------------
    bool VerifyIsASCII( const std::string d);
    bool VerifyIsDigit( const std::string d);
    bool VerifyIsHex( const std::string d);

    //--------------------------------------------------------------------
    // Conditional compilation for 64-bit
    //--------------------------------------------------------------------
    #if defined(_WIN64) 
        #define USE_64BIT 1
    #elif defined(__x86_64__)    
        #define USE_64BIT 1
    #endif

    //--------------------------------------------------------------------
    //	Replace all instances of from with to in src
    //--------------------------------------------------------------------
    std::string ReplaceAll(std::string src, std::string from, std::string to);

    //--------------------------------------------------------------------
    // Remove all spaces in a string and cange them to a unconvertable form
    //--------------------------------------------------------------------
    std::string ConvertSpaces(  std::string  s );
    std::string UnConvertSpaces( std::string  s );

    std::string ConvertCommas(  std::string  s ) ;
    std::string UnConvertCommas( std::string  s );

    //--------------------------------------------------------------------
    //--------------------------------------------------------------------
    int SplitString(const string& szInput, const string& szDelimiter,
        vector<string>& listResults, bool bIncludeEmpties);
    int SplitString(const string& szInput,
        vector<string>& listResults, bool bIncludeEmpties);

    //--------------------------------------------------------------------
    // Manipulation of comma delineated lists
    // RemoveFromCommaList ( "12,45,34" , "45" ) -> "12,34"
    // RemoveFromCommaList ( "12,45,34,45" , "45", true ) -> "12,34,45"
    //--------------------------------------------------------------------
    std::string RemoveFromCommaList( const std::string szIn,
                                     const std::string szItemToRemove,
                                     bool bRemoveOnce = false);

    //--------------------------------------------------------------------
    //--------------------------------------------------------------------
    int AppendIntToString( std::string &szDest, unsigned short i, TCHAR szDelimiter );
    int AppendIntToString( std::string &szDest, unsigned int i, TCHAR szDelimiter );
    #ifdef USE_64BIT    
        int AppendIntToString( std::string &szDest, long i, TCHAR szDelimiter  );
    #else
        int AppendIntToString( std::string &szDest, long i, TCHAR szDelimiter  );
        int AppendIntToString( std::string &szDest, LONG64 i, TCHAR szDelimiter );
    #endif

    int AppendIntToString( std::string &szDest, unsigned short i, std::string szDelimiter );
    int AppendIntToString( std::string &szDest, unsigned int i, std::string szDelimiter );
    #ifdef USE_64BIT    
        int AppendIntToString( std::string &szDest, long i, std::string szDelimiter );
    #else
        int AppendIntToString( std::string &szDest, long i, std::string szDelimiter );
        int AppendIntToString( std::string &szDest, LONG64 i, std::string szDelimiter );
    #endif

    std::string ConvertToString(unsigned int nInput);

    void DumpStringVectorToClientLog( vector<std::string> listDebug);

    //--------------------------------------------------------------------
    // Convert a list into a string
    //--------------------------------------------------------------------
    std::string SerializeStringList( list<std::string> listInput,
                                     const std::string szDelimiter=_T("/" ) );
    std::string SerializeStringVector( vector<std::string> listInput,
                                       const std::string szDelimiter=_T("," ) );

    //--------------------------------------------------------------------
    // Return a list of items that exist in a but b
    //--------------------------------------------------------------------
    vector<std::string> Subtract( vector<std::string>listOperand1,
                                  vector<std::string>listOperand2 );
    bool IsIn(std::vector<std::string>listInput, std::string szCompare);

    //--------------------------------------------------------------------
    // Format bytes into readable type
    //--------------------------------------------------------------------
    void InsertSeparator (double dwNumber, std::string& szFormattedNumber);
    void InsertSeparator (LONG64 l64Number, std::string& szFormattedNumber);
    bool FormatByteSize(LONG64 l64ByteSize, std::string& szByteSize,
                        std::string& szByteSizeType, int nMaxNumber=0);
    bool FormatBandwidth(LONG64 l64ByteSize,
                         const boost::posix_time::time_duration& elapsedTime,
                         std::string& szKiloBitsPerSec,
                         std::string& szKiloBitsPerSecType);

    //--------------------------------------------------------------------
    // Formats the number with thousands separator.
    //--------------------------------------------------------------------
    bool FormatNumber(const LONG64& l64Number, std::string& sz64Number,
                      std::string szDefault=_T(""));
    bool FormatNumber(const double& dblNumber, std::string& szDblNumber,
                      unsigned int nPrecision, std::string szDefault=_T(""));
    bool FormatNumber(const int& nNumber, std::string& szIntNumber,
                      std::string szDefault=_T(""));

    //--------------------------------------------------------------------
    // Formats time into a formatted date and/or time string
    //--------------------------------------------------------------------
    bool FormatDate(time_t tmDate, std::string& szFormattedDate);
    bool FormatDateAndTime(time_t tmDateTime, std::string& szFormattedDate);
    bool FormatDuration(const boost::posix_time::time_duration& elapsedTime,
        std::string& szFormattedTime, std::string& szTimeType, int nPrecision=1);
    const char* MakeFormatDate(const char* szFormat, const struct tm* pTM);
    
    //--------------------------------------------------------------------
    // Formats the error returned from GetLastError
    //--------------------------------------------------------------------
    std::string FormatErrorString(unsigned long dwError);

    //--------------------------------------------------------------------
    // Format a monetary amount into readable type
    //--------------------------------------------------------------------
    bool SimpleFormatRate(float fAmount, std::string& szAmount, int nPrecision=2);

    //--------------------------------------------------------------------
    // Trims a file name, appending ellipsis if needed
    //--------------------------------------------------------------------
    bool TrimFileName(int nMaxWidth, std::string szInFileName, std::string& szOutFileName);

    //--------------------------------------------------------------------
    // Helper to create a string of spaces.
    //--------------------------------------------------------------------
	inline std::string GetPadStr(const int& nNumPad, std::string szInPadStr=_T(" ")) {
	    std::string szOutPadStr = _T("");

	    if (nNumPad <= 0) {
	        return szOutPadStr;
	    }

	    int nIndex = 0;

	    for (nIndex = 0; nIndex < nNumPad; nIndex ++) {
	        szOutPadStr = szOutPadStr + szInPadStr;
	    }

	    return szOutPadStr;
	}

    //--------------------------------------------------------------------
    // MD5 Hash
    //--------------------------------------------------------------------
    int MakeFileMd5Digest(char *szFilePath, unsigned char* pMD5Digest);
    void MakeStringMd5Digest(char* szInStr, unsigned char* pMD5Digest);
    void ConvertDigestToString(std::string& szOutStr, unsigned char* pMD5Digest);
};


/*
although a BSTR is explicitly a unicode string, we can't actually pass std::strings around
between the app and its DLLs *except as consts*, because of the bullshit with EXEs and
DLLs potentially using different memory managers: if we create a std::string in a DLL and
pass it back to an EXE that frees it, windows WILL segfault if any one of a dozen
compiler options don't exactly match, let alone if it's actually using a different MM
I'd LIKE to skip this completely and just use _UNICODE _MT for everything, so I'm going
to leave this empty for now, but if it doesn't work out we'll need to implement this
to provide "nice" translations between the std::strings both sides use internally and the
BSTRs they have to pass between each other

class bstring {
    BSTR SysAllocString( const OLECHAR* sz );
    VOID SysFreeString( BSTR bstr );
};

*/

/*********************************************************************
 * Class Money
 *
 * Provodes formatting of currency amounts using std::locale
 * Example usage:
 * int main()
 * {
 *   std::cout.imbue(std::locale("german"));
 *   std::cout << std::showbase << money(100000000.0) << std::endl;
 * }
 *
 *********************************************************************/

///////////////////////////////////////////////////////////////////////
class Money
{
    long double     m_dwAmount;

public:
    explicit Money(long double dwAmount) : m_dwAmount(dwAmount) {}

    friend std::ostream& operator << (std::ostream& os, const Money& m)
    {
        std::use_facet<std::money_put<char> >(os.getloc()).put(
            std::ostreambuf_iterator<char>(os),
            false, os, os.fill(), m.m_dwAmount);
        return os;
    }

}; // End Money

/*********************************************************************
 * Class DiomedeMoneyPunct
 *
 * Provides specialization of the std::moneypunct.
 * Example usage:
 *     std::locale localEnglish("english");
 *     std::locale locDiomede(localEnglish, new DiomedeMoneyPunct);
 *     cout.imbue(locDiomede);
 *
 *********************************************************************/

///////////////////////////////////////////////////////////////////////
class DiomedeMoneyPunct
    : public moneypunct<char, false> {

private:
    int     m_nFracDigits;

public:
    explicit DiomedeMoneyPunct(int nFracDigits) : m_nFracDigits(nFracDigits) {}

    void SetFracDigits(int nFracDigits) {
       m_nFracDigits = nFracDigits;
    }

    //----------------------------------------------------------------- 
    // Helper to return the platform specific locale string   
    //-----------------------------------------------------------------    
    static std::string GetLocaleString()
    {
        #if defined( WIN32 )
            return _T("english");
        #else
            return _T("en_US.utf8");
        #endif
    }

protected:
    virtual char do_decimal_point() const
        {return ('.'); }
    virtual char do_thousands_sep() const
        {return (','); }
    virtual string do_grouping() const
        {return (string("\3")); }
    virtual string do_curr_symbol() const
        {return (string("$")); }
    virtual string do_positive_sign() const
        {return (string("")); }
    virtual string do_negative_sign() const
        {return (string("-")); }
    virtual int do_frac_digits() const
        {return (m_nFracDigits); }

    virtual pattern do_pos_format() const;
    virtual pattern do_neg_format() const;

}; // End DiomedeMoneyPunct

/*********************************************************************
 * Class DiomedeNumPunct
 *
 * Provides specialization of the std::numpunct.
 * Example usage:
 *     std::locale localEnglish("english");
 *     std::locale locDiomede(localEnglish, new DiomedeNumPunct);
 *     cout.imbue(locDiomede);
 *     cout << setw(3) << _T("") << std::showbase << l64ByteCount << _T(" byte(s) ") << endl;
 *
 *********************************************************************/

///////////////////////////////////////////////////////////////////////
class DiomedeNumPunct : public std::numpunct<char>
{
    std::string do_grouping() const {
      char szGroup[] = { 3, 0 };
      return szGroup;
    }

}; // End DiomedeNumPunct

#endif // __STRING_UTIL_H__

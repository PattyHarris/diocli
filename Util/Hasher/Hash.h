// CHash.h : header file
//

#ifndef _CHASH_H
#define _CHASH_H

// Choose which algorithms you want
// Put 1s to support algorithms, else 0 to not support
#define        SUPPORT_CRC32          0
#define        SUPPORT_GOSTHASH       0
#define        SUPPORT_MD2            0
#define        SUPPORT_MD4            0
#define        SUPPORT_MD5            1
#define        SUPPORT_SHA1           0
#define        SUPPORT_SHA2           0

/////////////////////////////////////////////////////////////////////////////
// CHash definitions

class CHash
{
// Construction
public:
	CHash();   // Standard constructor
	std::string DoHash();
	int GetHashAlgorithm();
	std::string GetHashFile();
	int GetHashFormat();
	int GetHashOperation();
	std::string GetHashString();
	int GetSHA2Strength();
	std::string GOSTHash();
	std::string MD2Hash();
	std::string MD4Hash();
	std::string MD5Hash();
	std::string SHA1Hash();
	std::string SHA2Hash();
	void SetHashAlgorithm(int Algo);
	void SetHashOperation(int Style);
	void SetHashFile(LPCSTR File);
	void SetHashFormat(int Style);
	void SetHashString(LPCSTR Hash);
	void SetSHA2Strength(int Strength);

// Implementation
protected:
	// Input string, algorithm, filename, formatting and such
	std::string     hashString;
	int             hashAlgo;
	std::string     hashFile;
	int             hashFormatting;
	int             hashStyle;
	int             hashOperation;
	int             sha2Strength;

	// Temporary working std::string
	std::string     tempHash;
};

// Definitions of some kind
#define        STRING_HASH            1
#define        FILE_HASH              2

#define        SIZE_OF_BUFFER         16000

// Algorithms
#define        CRC32                  1
#define        GOSTHASH               2
#define        MD2                    3
#define        MD4                    4
#define        MD5                    5
#define        SHA1                   6
#define        SHA2                   7

// Formatting options
#define        LOWERCASE_NOSPACES     1
#define        LOWERCASE_SPACES       2
#define        UPPERCASE_NOSPACES     3
#define        UPPERCASE_SPACES       4

#endif

/*********************************************************************
 *
 *  file:  CommandDefs.h
 *
 *  Copyright (C) 2010, Diomede Corporation
 *  All rights reserved.
 *
 * Purpose: Command Constant Definitions
 *
 *********************************************************************/

#ifndef __DIOMEDE_PEM_H__
#define __DIOMEDE_PEM_H__

using namespace std;

#include <string>
#include <vector>
#include <list>
#include "../Util/XString.h"
#include <iostream>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

namespace DIOMEDE_PEM {

const string PEM_FIRST_LINE = _T("firstLine");      // Can be used by an outside script to
const string PEM_LAST_LINE  = _T("lastLine");       // change the contents of this file.

const string DiomedePEMTextList[]  = {
PEM_FIRST_LINE
,_T("Certificate:\n")
,_T("    Data:\n")
,_T("        Version: 3 (0x2)\n")
,_T("        Serial Number: 769 (0x301)\n")
,_T("        Signature Algorithm: sha1WithRSAEncryption\n")
,_T("        Issuer: C=US, O=The Go Daddy Group, Inc., OU=Go Daddy Class 2 Certification Authority\n")
,_T("        Validity\n")
,_T("            Not Before: Nov 16 01:54:37 2006 GMT\n")
,_T("            Not After : Nov 16 01:54:37 2026 GMT\n")
,_T("        Subject: C=US, ST=Arizona, L=Scottsdale, O=GoDaddy.com, Inc., OU=http://certificates.godaddy.com/repository, CN=Go Daddy Secure Certification Authority/serialNumber=07969287\n")
,_T("        Subject Public Key Info:\n")
,_T("            Public Key Algorithm: rsaEncryption\n")
,_T("            RSA Public Key: (2048 bit)\n")
,_T("                Modulus (2048 bit):\n")
,_T("                    00:c4:2d:d5:15:8c:9c:26:4c:ec:32:35:eb:5f:b8:\n")
,_T("                    59:01:5a:a6:61:81:59:3b:70:63:ab:e3:dc:3d:c7:\n")
,_T("                    2a:b8:c9:33:d3:79:e4:3a:ed:3c:30:23:84:8e:b3:\n")
,_T("                    30:14:b6:b2:87:c3:3d:95:54:04:9e:df:99:dd:0b:\n")
,_T("                    25:1e:21:de:65:29:7e:35:a8:a9:54:eb:f6:f7:32:\n")
,_T("                    39:d4:26:55:95:ad:ef:fb:fe:58:86:d7:9e:f4:00:\n")
,_T("                    8d:8c:2a:0c:bd:42:04:ce:a7:3f:04:f6:ee:80:f2:\n")
,_T("                    aa:ef:52:a1:69:66:da:be:1a:ad:5d:da:2c:66:ea:\n")
,_T("                    1a:6b:bb:e5:1a:51:4a:00:2f:48:c7:98:75:d8:b9:\n")
,_T("                    29:c8:ee:f8:66:6d:0a:9c:b3:f3:fc:78:7c:a2:f8:\n")
,_T("                    a3:f2:b5:c3:f3:b9:7a:91:c1:a7:e6:25:2e:9c:a8:\n")
,_T("                    ed:12:65:6e:6a:f6:12:44:53:70:30:95:c3:9c:2b:\n")
,_T("                    58:2b:3d:08:74:4a:f2:be:51:b0:bf:87:d0:4c:27:\n")
,_T("                    58:6b:b5:35:c5:9d:af:17:31:f8:0b:8f:ee:ad:81:\n")
,_T("                    36:05:89:08:98:cf:3a:af:25:87:c0:49:ea:a7:fd:\n")
,_T("                    67:f7:45:8e:97:cc:14:39:e2:36:85:b5:7e:1a:37:\n")
,_T("                    fd:16:f6:71:11:9a:74:30:16:fe:13:94:a3:3f:84:\n")
,_T("                    0d:4f\n")
,_T("                Exponent: 65537 (0x10001)\n")
,_T("        X509v3 extensions:\n")
,_T("            X509v3 Subject Key Identifier:\n")
,_T("                FD:AC:61:32:93:6C:45:D6:E2:EE:85:5F:9A:BA:E7:76:99:68:CC:E7\n")
,_T("            X509v3 Authority Key Identifier:\n")
,_T("                keyid:D2:C4:B0:D2:91:D4:4C:11:71:B3:61:CB:3D:A1:FE:DD:A8:6A:D4:E3\n")
,_T("\n")
,_T("            X509v3 Basic Constraints: critical\n")
,_T("                CA:TRUE, pathlen:0\n")
,_T("            Authority Information Access:\n")
,_T("                OCSP - URI:http://ocsp.godaddy.com\n")
,_T("\n")
,_T("            X509v3 CRL Distribution Points:\n")
,_T("                URI:http://certificates.godaddy.com/repository/gdroot.crl\n")
,_T("\n")
,_T("            X509v3 Certificate Policies:\n")
,_T("                Policy: X509v3 Any Policy\n")
,_T("                  CPS: http://certificates.godaddy.com/repository\n")
,_T("\n")
,_T("            X509v3 Key Usage: critical\n")
,_T("                Certificate Sign, CRL Sign\n")
,_T("    Signature Algorithm: sha1WithRSAEncryption\n")
,_T("        d2:86:c0:ec:bd:f9:a1:b6:67:ee:66:0b:a2:06:3a:04:50:8e:\n")
,_T("        15:72:ac:4a:74:95:53:cb:37:cb:44:49:ef:07:90:6b:33:d9:\n")
,_T("        96:f0:94:56:a5:13:30:05:3c:85:32:21:7b:c9:c7:0a:a8:24:\n")
,_T("        a4:90:de:46:d3:25:23:14:03:67:c2:10:d6:6f:0f:5d:7b:7a:\n")
,_T("        cc:9f:c5:58:2a:c1:c4:9e:21:a8:5a:f3:ac:a4:46:f3:9e:e4:\n")
,_T("        63:cb:2f:90:a4:29:29:01:d9:72:2c:29:df:37:01:27:bc:4f:\n")
,_T("        ee:68:d3:21:8f:c0:b3:e4:f5:09:ed:d2:10:aa:53:b4:be:f0:\n")
,_T("        cc:59:0b:d6:3b:96:1c:95:24:49:df:ce:ec:fd:a7:48:91:14:\n")
,_T("        45:0e:3a:36:6f:da:45:b3:45:a2:41:c9:d4:d7:44:4e:3e:b9:\n")
,_T("        74:76:d5:a2:13:55:2c:c6:87:a3:b5:99:ac:06:84:87:7f:75:\n")
,_T("        06:fc:bf:14:4c:0e:cc:6e:c4:df:3d:b7:12:71:f4:e8:f1:51:\n")
,_T("        40:22:28:49:e0:1d:4b:87:a8:34:cc:06:a2:dd:12:5a:d1:86:\n")
,_T("        36:64:03:35:6f:6f:77:6e:eb:f2:85:50:98:5e:ab:03:53:ad:\n")
,_T("        91:23:63:1f:16:9c:cd:b9:b2:05:63:3a:e1:f4:68:1b:17:05:\n")
,_T("        35:95:53:ee\n")
,_T("-----BEGIN CERTIFICATE-----\n")
,_T("MIIE3jCCA8agAwIBAgICAwEwDQYJKoZIhvcNAQEFBQAwYzELMAkGA1UEBhMCVVMx\n")
,_T("ITAfBgNVBAoTGFRoZSBHbyBEYWRkeSBHcm91cCwgSW5jLjExMC8GA1UECxMoR28g\n")
,_T("RGFkZHkgQ2xhc3MgMiBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTAeFw0wNjExMTYw\n")
,_T("MTU0MzdaFw0yNjExMTYwMTU0MzdaMIHKMQswCQYDVQQGEwJVUzEQMA4GA1UECBMH\n")
,_T("QXJpem9uYTETMBEGA1UEBxMKU2NvdHRzZGFsZTEaMBgGA1UEChMRR29EYWRkeS5j\n")
,_T("b20sIEluYy4xMzAxBgNVBAsTKmh0dHA6Ly9jZXJ0aWZpY2F0ZXMuZ29kYWRkeS5j\n")
,_T("b20vcmVwb3NpdG9yeTEwMC4GA1UEAxMnR28gRGFkZHkgU2VjdXJlIENlcnRpZmlj\n")
,_T("YXRpb24gQXV0aG9yaXR5MREwDwYDVQQFEwgwNzk2OTI4NzCCASIwDQYJKoZIhvcN\n")
,_T("AQEBBQADggEPADCCAQoCggEBAMQt1RWMnCZM7DI161+4WQFapmGBWTtwY6vj3D3H\n")
,_T("KrjJM9N55DrtPDAjhI6zMBS2sofDPZVUBJ7fmd0LJR4h3mUpfjWoqVTr9vcyOdQm\n")
,_T("VZWt7/v+WIbXnvQAjYwqDL1CBM6nPwT27oDyqu9SoWlm2r4arV3aLGbqGmu75RpR\n")
,_T("SgAvSMeYddi5Kcju+GZtCpyz8/x4fKL4o/K1w/O5epHBp+YlLpyo7RJlbmr2EkRT\n")
,_T("cDCVw5wrWCs9CHRK8r5RsL+H0EwnWGu1NcWdrxcx+AuP7q2BNgWJCJjPOq8lh8BJ\n")
,_T("6qf9Z/dFjpfMFDniNoW1fho3/Rb2cRGadDAW/hOUoz+EDU8CAwEAAaOCATIwggEu\n")
,_T("MB0GA1UdDgQWBBT9rGEyk2xF1uLuhV+auud2mWjM5zAfBgNVHSMEGDAWgBTSxLDS\n")
,_T("kdRMEXGzYcs9of7dqGrU4zASBgNVHRMBAf8ECDAGAQH/AgEAMDMGCCsGAQUFBwEB\n")
,_T("BCcwJTAjBggrBgEFBQcwAYYXaHR0cDovL29jc3AuZ29kYWRkeS5jb20wRgYDVR0f\n")
,_T("BD8wPTA7oDmgN4Y1aHR0cDovL2NlcnRpZmljYXRlcy5nb2RhZGR5LmNvbS9yZXBv\n")
,_T("c2l0b3J5L2dkcm9vdC5jcmwwSwYDVR0gBEQwQjBABgRVHSAAMDgwNgYIKwYBBQUH\n")
,_T("AgEWKmh0dHA6Ly9jZXJ0aWZpY2F0ZXMuZ29kYWRkeS5jb20vcmVwb3NpdG9yeTAO\n")
,_T("BgNVHQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQEFBQADggEBANKGwOy9+aG2Z+5mC6IG\n")
,_T("OgRQjhVyrEp0lVPLN8tESe8HkGsz2ZbwlFalEzAFPIUyIXvJxwqoJKSQ3kbTJSMU\n")
,_T("A2fCENZvD117esyfxVgqwcSeIaha86ykRvOe5GPLL5CkKSkB2XIsKd83ASe8T+5o\n")
,_T("0yGPwLPk9Qnt0hCqU7S+8MxZC9Y7lhyVJEnfzuz9p0iRFEUOOjZv2kWzRaJBydTX\n")
,_T("RE4+uXR21aITVSzGh6O1mawGhId/dQb8vxRMDsxuxN89txJx9OjxUUAiKEngHUuH\n")
,_T("qDTMBqLdElrRhjZkAzVvb3du6/KFUJheqwNTrZEjYx8WnM25sgVjOuH0aBsXBTWV\n")
,_T("U+4=\n")
,_T("-----END CERTIFICATE-----\n")
,_T("Certificate:\n")
,_T("    Data:\n")
,_T("        Version: 3 (0x2)\n")
,_T("        Serial Number: 0 (0x0)\n")
,_T("        Signature Algorithm: sha1WithRSAEncryption\n")
,_T("        Issuer: C=US, O=The Go Daddy Group, Inc., OU=Go Daddy Class 2 Certification Authority\n")
,_T("        Validity\n")
,_T("            Not Before: Jun 29 17:06:20 2004 GMT\n")
,_T("            Not After : Jun 29 17:06:20 2034 GMT\n")
,_T("        Subject: C=US, O=The Go Daddy Group, Inc., OU=Go Daddy Class 2 Certification Authority\n")
,_T("        Subject Public Key Info:\n")
,_T("            Public Key Algorithm: rsaEncryption\n")
,_T("            RSA Public Key: (2048 bit)\n")
,_T("                Modulus (2048 bit):\n")
,_T("                    00:de:9d:d7:ea:57:18:49:a1:5b:eb:d7:5f:48:86:\n")
,_T("                    ea:be:dd:ff:e4:ef:67:1c:f4:65:68:b3:57:71:a0:\n")
,_T("                    5e:77:bb:ed:9b:49:e9:70:80:3d:56:18:63:08:6f:\n")
,_T("                    da:f2:cc:d0:3f:7f:02:54:22:54:10:d8:b2:81:d4:\n")
,_T("                    c0:75:3d:4b:7f:c7:77:c3:3e:78:ab:1a:03:b5:20:\n")
,_T("                    6b:2f:6a:2b:b1:c5:88:7e:c4:bb:1e:b0:c1:d8:45:\n")
,_T("                    27:6f:aa:37:58:f7:87:26:d7:d8:2d:f6:a9:17:b7:\n")
,_T("                    1f:72:36:4e:a6:17:3f:65:98:92:db:2a:6e:5d:a2:\n")
,_T("                    fe:88:e0:0b:de:7f:e5:8d:15:e1:eb:cb:3a:d5:e2:\n")
,_T("                    12:a2:13:2d:d8:8e:af:5f:12:3d:a0:08:05:08:b6:\n")
,_T("                    5c:a5:65:38:04:45:99:1e:a3:60:60:74:c5:41:a5:\n")
,_T("                    72:62:1b:62:c5:1f:6f:5f:1a:42:be:02:51:65:a8:\n")
,_T("                    ae:23:18:6a:fc:78:03:a9:4d:7f:80:c3:fa:ab:5a:\n")
,_T("                    fc:a1:40:a4:ca:19:16:fe:b2:c8:ef:5e:73:0d:ee:\n")
,_T("                    77:bd:9a:f6:79:98:bc:b1:07:67:a2:15:0d:dd:a0:\n")
,_T("                    58:c6:44:7b:0a:3e:62:28:5f:ba:41:07:53:58:cf:\n")
,_T("                    11:7e:38:74:c5:f8:ff:b5:69:90:8f:84:74:ea:97:\n")
,_T("                    1b:af\n")
,_T("                Exponent: 3 (0x3)\n")
,_T("        X509v3 extensions:\n")
,_T("            X509v3 Subject Key Identifier: \n")
,_T("                D2:C4:B0:D2:91:D4:4C:11:71:B3:61:CB:3D:A1:FE:DD:A8:6A:D4:E3\n")
,_T("            X509v3 Authority Key Identifier: \n")
,_T("                keyid:D2:C4:B0:D2:91:D4:4C:11:71:B3:61:CB:3D:A1:FE:DD:A8:6A:D4:E3\n")
,_T("                DirName:/C=US/O=The Go Daddy Group, Inc./OU=Go Daddy Class 2 Certification Authority\n")
,_T("                serial:00\n")
,_T("\n")
,_T("            X509v3 Basic Constraints: \n")
,_T("                CA:TRUE\n")
,_T("    Signature Algorithm: sha1WithRSAEncryption\n")
,_T("        32:4b:f3:b2:ca:3e:91:fc:12:c6:a1:07:8c:8e:77:a0:33:06:\n")
,_T("        14:5c:90:1e:18:f7:08:a6:3d:0a:19:f9:87:80:11:6e:69:e4:\n")
,_T("        96:17:30:ff:34:91:63:72:38:ee:cc:1c:01:a3:1d:94:28:a4:\n")
,_T("        31:f6:7a:c4:54:d7:f6:e5:31:58:03:a2:cc:ce:62:db:94:45:\n")
,_T("        73:b5:bf:45:c9:24:b5:d5:82:02:ad:23:79:69:8d:b8:b6:4d:\n")
,_T("        ce:cf:4c:ca:33:23:e8:1c:88:aa:9d:8b:41:6e:16:c9:20:e5:\n")
,_T("        89:9e:cd:3b:da:70:f7:7e:99:26:20:14:54:25:ab:6e:73:85:\n")
,_T("        e6:9b:21:9d:0a:6c:82:0e:a8:f8:c2:0c:fa:10:1e:6c:96:ef:\n")
,_T("        87:0d:c4:0f:61:8b:ad:ee:83:2b:95:f8:8e:92:84:72:39:eb:\n")
,_T("        20:ea:83:ed:83:cd:97:6e:08:bc:eb:4e:26:b6:73:2b:e4:d3:\n")
,_T("        f6:4c:fe:26:71:e2:61:11:74:4a:ff:57:1a:87:0f:75:48:2e:\n")
,_T("        cf:51:69:17:a0:02:12:61:95:d5:d1:40:b2:10:4c:ee:c4:ac:\n")
,_T("        10:43:a6:a5:9e:0a:d5:95:62:9a:0d:cf:88:82:c5:32:0c:e4:\n")
,_T("        2b:9f:45:e6:0d:9f:28:9c:b1:b9:2a:5a:57:ad:37:0f:af:1d:\n")
,_T("        7f:db:bd:9f\n")
,_T("-----BEGIN CERTIFICATE-----\n")
,_T("MIIEADCCAuigAwIBAgIBADANBgkqhkiG9w0BAQUFADBjMQswCQYDVQQGEwJVUzEh\n")
,_T("MB8GA1UEChMYVGhlIEdvIERhZGR5IEdyb3VwLCBJbmMuMTEwLwYDVQQLEyhHbyBE\n")
,_T("YWRkeSBDbGFzcyAyIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MB4XDTA0MDYyOTE3\n")
,_T("MDYyMFoXDTM0MDYyOTE3MDYyMFowYzELMAkGA1UEBhMCVVMxITAfBgNVBAoTGFRo\n")
,_T("ZSBHbyBEYWRkeSBHcm91cCwgSW5jLjExMC8GA1UECxMoR28gRGFkZHkgQ2xhc3Mg\n")
,_T("MiBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTCCASAwDQYJKoZIhvcNAQEBBQADggEN\n")
,_T("ADCCAQgCggEBAN6d1+pXGEmhW+vXX0iG6r7d/+TvZxz0ZWizV3GgXne77ZtJ6XCA\n")
,_T("PVYYYwhv2vLM0D9/AlQiVBDYsoHUwHU9S3/Hd8M+eKsaA7Ugay9qK7HFiH7Eux6w\n")
,_T("wdhFJ2+qN1j3hybX2C32qRe3H3I2TqYXP2WYktsqbl2i/ojgC95/5Y0V4evLOtXi\n")
,_T("EqITLdiOr18SPaAIBQi2XKVlOARFmR6jYGB0xUGlcmIbYsUfb18aQr4CUWWoriMY\n")
,_T("avx4A6lNf4DD+qta/KFApMoZFv6yyO9ecw3ud72a9nmYvLEHZ6IVDd2gWMZEewo+\n")
,_T("YihfukEHU1jPEX44dMX4/7VpkI+EdOqXG68CAQOjgcAwgb0wHQYDVR0OBBYEFNLE\n")
,_T("sNKR1EwRcbNhyz2h/t2oatTjMIGNBgNVHSMEgYUwgYKAFNLEsNKR1EwRcbNhyz2h\n")
,_T("/t2oatTjoWekZTBjMQswCQYDVQQGEwJVUzEhMB8GA1UEChMYVGhlIEdvIERhZGR5\n")
,_T("IEdyb3VwLCBJbmMuMTEwLwYDVQQLEyhHbyBEYWRkeSBDbGFzcyAyIENlcnRpZmlj\n")
,_T("YXRpb24gQXV0aG9yaXR5ggEAMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEFBQAD\n")
,_T("ggEBADJL87LKPpH8EsahB4yOd6AzBhRckB4Y9wimPQoZ+YeAEW5p5JYXMP80kWNy\n")
,_T("OO7MHAGjHZQopDH2esRU1/blMVgDoszOYtuURXO1v0XJJLXVggKtI3lpjbi2Tc7P\n")
,_T("TMozI+gciKqdi0FuFskg5YmezTvacPd+mSYgFFQlq25zheabIZ0KbIIOqPjCDPoQ\n")
,_T("HmyW74cNxA9hi63ugyuV+I6ShHI56yDqg+2DzZduCLzrTia2cyvk0/ZM/iZx4mER\n")
,_T("dEr/VxqHD3VILs9RaRegAhJhldXRQLIQTO7ErBBDpqWeCtWVYpoNz4iCxTIM5Cuf\n")
,_T("ReYNnyicsbkqWletNw+vHX/bvZ8=\n")
,_T("-----END CERTIFICATE-----\n")
,PEM_LAST_LINE
};

//---------------------------------------------------------------------
// Purpose:
//      Writes the above string out to a file.
// Requires:
//      szFullPath: full path of PEM file, including file name
//      nFailures: count of write failures - added to ensure
//                 success of task.
// Returns:
//      0 if successful, errno otherwise.
//---------------------------------------------------------------------
inline int WriteCertificate(std::string szFullPath, int& nFailures)
{
    // Open the file for writing
    FILE* pDiomedePemFile = fopen(szFullPath.c_str(), _T("wt"));
    if (pDiomedePemFile == NULL) {
        return errno;
    }

    int nIndex = 0;
    int nWritten = 0;
    nFailures = 0;
    int nLineLength = 0;

    do {

        if ( DiomedePEMTextList[nIndex] == PEM_FIRST_LINE) {
            nIndex ++;
            continue;
        }

        nLineLength = DiomedePEMTextList[nIndex].length();
        nWritten = (int)fwrite((char *)DiomedePEMTextList[nIndex].c_str(), sizeof(char), nLineLength,
                           pDiomedePemFile);

        if ( nLineLength != nWritten ) {
            nFailures++;
        }

        nIndex ++;

	} while (DiomedePEMTextList[nIndex] != DIOMEDE_PEM::PEM_LAST_LINE);

    // Close up
    fflush(pDiomedePemFile);
    fclose(pDiomedePemFile);

    return 0;
} // End WriteCertificate

} // End namespace DIOMEDE_PEM

#endif // __DIOMEDE_PEM_H_
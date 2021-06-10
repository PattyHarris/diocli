/*********************************************************************
 * 
 *  file:  BuildVersion.h
 * 
 *  (C) Copyright 2010, Diomede Corporation
 *  All rights reserved
 * 
 *  Use, modification, and distribution is subject to   
 *  the New BSD License (See accompanying file LICENSE).
 * 
 * Purpose: file containing #defines for version and build information
 * ***WARNING*** DO NOT MODIFY the contents of this file. The
 *               build scripts expect a certain format, etc for this file and
 *               changes, particularly the addition of new variables may break
 *               the build scripts.
 *
 * This file contains stubs for the symbols listed below. At compile-time
 * these stubs will be filled in with the appropriate information, generated
 * by the build script. The comments in this file are to document the purpose
 * of these symbols and give examples of what will be filled in at compile time.
 * 
 *********************************************************************/
 
//////////////////////////////////////////////////
// BUILDNUMBER -- the number of this build
//                Must be an integer
// Example: 455
//////////////////////////////////////////////////
#define VERINFO_BUILDNUMBER 4

//////////////////////////////////////////////////
// BUILDNUMBER -- the revision of this version.
//                Must be an integer
// Example: 455
//////////////////////////////////////////////////
#define VERINFO_REVNUMBER 3

///////////////////////////////////////////////////
// VERSIONNAME -- Major.Minor.Revision versions
//                Must be two integers separated
//                by a "."
//
// Example -- "2.0.13"
// Example -- "2.10.0"
///////////////////////////////////////////////////
#define VERINFO_VERSIONNAME _T("1.1.3")

//////////////////////////////////////////////////
// BRANCHID -- the name for this build
// Example: 1.4 for release and "dev" for Dev
//////////////////////////////////////////////////
#define VERINFO_BRANCHID _T("1.1.3")

//////////////////////////////////////////////////
// BUILDDATE -- date that this module was compiled
// __DATE__ is a build-in symbol in the compiler
//////////////////////////////////////////////////
#define VERINFO_BUILDDATE   _T(__DATE__)

//////////////////////////////////////////////////
// BUILDTIME -- time that this module was compiled
// __TIME__ is a build-in symbol in the compiler
//////////////////////////////////////////////////
#define VERINFO_BUILDTIME   _T(__TIME__)

//////////////////////////////////////////////////
// CLIENTPLATFORM -- Client platform for this build
// Example: Win32
//////////////////////////////////////////////////
#define VERINFO_CLIENTPLATFORM Win32

//////////////////////////////////////////////////
// Web Adddress -- Address for the web server
// Example: Diomede.com
//////////////////////////////////////////////////
#define VERINFO_WEBADDRESS _T("192.168.1.100")

//////////////////////////////////////////////////
// Install file size -- Diomede install file size
// Example: 8235860
//////////////////////////////////////////////////
#define VERINFO_FILESIZE 8235860


#undef BUILD_NUMBER
#undef BUILD_DATE
#define BUILD_NUMBER 0
#define BUILD_DATE __DATE__
//EOF

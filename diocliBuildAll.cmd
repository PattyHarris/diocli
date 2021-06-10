@ECHO OFF
color 0A
TITLE DioCLI and distributed libs and applications...
ECHO DioCLI Build Script
ECHO Version 1.0 - patty@diomedestorage.com
ECHO Last updated: January 29, 2010
ECHO ------------------------------------
ECHO Cleaning up...

REM Ensure loop variables have the correct value.
setlocal EnableDelayedExpansion

IF EXIST c:\dioclibuild.txt (del c:\dioclibuild.txt > nul)

set currentPath=%CD%

REM Check whether diocli is the current folder
FOR /F "delims=\" %%I in ("%CD%") do set currentFolder=%%~nI
IF NOT %currentFolder% == diocli GOTO failurePath

REM Count the number of arguments.
SET argC=0
FOR %%x IN (%*) DO SET /A argC+=1
REM ECHO Argument count = %argC%

IF %argC% GTR 3 GOTO badInput

REM If the first argument isn't empty, check for build type
REM Else, it defaults to rebuild

REM Set the build mode default
SET buildMode=rebuild

REM If no arguments, we do need to increment the build number
IF %argC% == 0 (
    call c:\DiomedeDevelopment\diocli\Util\BuildUtil\Debug\BuildUtil.exe 
    GOTO endParse
)

:parseLoop

IF "%1" == "" GOTO endParse

IF "%1" == "?" GOTO badInput

IF "%1"== clean GOTO cleanBuild

IF "%1"== rebuild (
    SET buildMode=rebuild
    GOTO checkBuild
)

IF "%1"== build (
    SET buildMode=build
    GOTO checkBuild
)

REM Must be a version number and optional new version number.
IF "%2" NEQ "" (
    call c:\DiomedeDevelopment\diocli\Util\BuildUtil\Debug\BuildUtil.exe "%1" "%2"
    GOTO endParse
) ELSE (
    call c:\DiomedeDevelopment\diocli\Util\BuildUtil\Debug\BuildUtil.exe "%1"
)

REM ==============================
REM Shift to the next argument.
SHIFT
GOTO parseLoop
REM ==============================

:endParse

REM Else, assume the first argument is a version number
:handleVersion

REM Echo Updating Subversion...
REM "C:\program files\TortoiseSVN\bin\TortoiseProc.exe" /command:update /path:"C:\DiomedeDevelopment\diocli\" /closeonend:1 /notempfile
REM IF ERRORLEVEL=1 GOTO success
REM IF ERRORLEVEL=0 GOTO failure
REM :success

IF NOT %buildMode%==clean GOTO checkBuild

:cleanBuild
ECHO Build Mode: %buildMode%

REM Clean all the projects
echo Cleaning DioCLI...
"C:\program files\Microsoft Visual Studio 8\Common7\IDE\devenv.exe" .\DioCLI\DioCLI.sln /%buildMode% Release /out c:\dioclibuild.txt
IF NOT ERRORLEVEL=0 GOTO failureBuild

echo Cleaning CPPSDK.DLL...Release
"C:\program files\Microsoft Visual Studio 8\Common7\IDE\devenv.exe" .\CPPSDK.DLL\CPPSDK.DLL.sln /%buildMode% Release /out c:\dioclibuild.txt
IF NOT ERRORLEVEL=0 GOTO failureBuild

echo Cleaning CPPSDK.DLL, Debug
"C:\program files\Microsoft Visual Studio 8\Common7\IDE\devenv.exe" .\CPPSDK.DLL\CPPSDK.DLL.sln /%buildMode% Debug /out c:\dioclibuild.txt
IF NOT ERRORLEVEL=0 GOTO failureBuild

echo Cleaning CPPSDK.DLL.Test, Release
"C:\program files\Microsoft Visual Studio 8\Common7\IDE\devenv.exe" .\CPPSDK.DLL\CPPSDK.DLL.Test\CPPSDK.DLL.Test.sln /%buildMode% Release /out c:\dioclibuild.txt
IF NOT ERRORLEVEL=0 GOTO failureBuild

del c:\dioclibuild.txt
TITLE DioCLI, CPPSDK.DLL, and CPPSDK.DLL.Test are now cleaned!
ECHO DioCLI, CPPSDK.DLL, and CPPSDK.DLL.Test are now cleaned!
pause
exit

:checkBuild
ECHO Build Mode: %buildMode%

IF %buildMode%==rebuild GOTO buildall
IF %buildMode%==build GOTO buildall
GOTO badInput

:buildall

REM ==============================
REM echo Testing testing testing....
REM pause
REM exit
REM ==============================

echo Building DioCLI...Release
"C:\program files\Microsoft Visual Studio 8\Common7\IDE\devenv.exe" .\DioCLI\DioCLI.sln /%buildMode% Release /out c:\dioclibuild.txt
IF NOT ERRORLEVEL=0 GOTO failureBuild

echo Building CPPSDK.DLL...Release
"C:\program files\Microsoft Visual Studio 8\Common7\IDE\devenv.exe" .\CPPSDK.DLL\CPPSDK.DLL.sln /%buildMode% Release /out c:\dioclibuild.txt
IF NOT ERRORLEVEL=0 GOTO failureBuild

echo Building CPPSDK.DLL, Debug
"C:\program files\Microsoft Visual Studio 8\Common7\IDE\devenv.exe" .\CPPSDK.DLL\CPPSDK.DLL.sln /%buildMode% Debug /out c:\dioclibuild.txt
IF NOT ERRORLEVEL=0 GOTO failureBuild

echo Copy DLLs and libs for CPPSDK.DLL.Test
copy C:\DiomedeDevelopment\diocli\DLL\CPPSDK.DLL.dll C:\DiomedeDevelopment\diocli\CPPSDK.DLL\CPPSDK.DLL.Test\CPPSDK.DLL.Test\Release /Y > nul
copy C:\DiomedeDevelopment\diocli\Lib\CPPSDK.DLL.lib C:\DiomedeDevelopment\diocli\CPPSDK.DLL\CPPSDK.DLL.Test\CPPSDK.DLL.Test\Release /Y > nul

copy C:\DiomedeDevelopment\diocli\DLL\CPPSDK.DLL.dll C:\DiomedeDevelopment\diocli\CPPSDK.DLL\CPPSDK.DLL.Test\CPPSDK.DLL.Test\ReDist /Y > nul
copy C:\DiomedeDevelopment\diocli\Lib\CPPSDK.DLL.lib C:\DiomedeDevelopment\diocli\CPPSDK.DLL\CPPSDK.DLL.Test\CPPSDK.DLL.Test\ReDist /Y > nul

copy C:\DiomedeDevelopment\diocli\DLL\CPPSDKd.DLL.dll C:\DiomedeDevelopment\diocli\CPPSDK.DLL\CPPSDK.DLL.Test\CPPSDK.DLL.Test\Debug /Y > nul

echo Building CPPSDK.DLL.Test, Release
"C:\program files\Microsoft Visual Studio 8\Common7\IDE\devenv.exe" .\CPPSDK.DLL\CPPSDK.DLL.Test\CPPSDK.DLL.Test.sln /%buildMode% Release /out c:\dioclibuild.txt
IF NOT ERRORLEVEL=0 GOTO failureBuild

del c:\dioclibuild.txt
TITLE DioCLI, CPPSDK.DLL, and CPPSDK.DLL.Test updated!
ECHO DioCLI, CPPSDK.DLL, and CPPSDK.DLL.Test are now updated!
pause
exit

:failure
echo Process failed.
pause
exit

:failureBuild
echo Build failed.  Output follows and can be found at c:\dioclibuild.txt:
pause
type c:\dioclibuild.txt
pause
exit

:failurePath
echo.Current folder is %currentFolder%
echo Please run this script from your diocli folder.
pause
exit

:badInput
echo Acceptable input: 
echo diocliBuildAll.cmd "<clean,rebuild,build> <current version> <new version>"
echo     examples
echo             c:\diocli\diocliBuildAll.cm build .92.4 
echo                Build and increments the version to .92.5 and increments 
echo                the build number.
echo     or
echo             c:\diocli\diocliBuildAll.cm .92.4 1.1.0
echo                Rebuild all and changes the version to 1.1.0.
pause
exit

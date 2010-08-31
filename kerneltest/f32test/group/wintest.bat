@echo off
rem
rem Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
rem All rights reserved.
rem This component and the accompanying materials are made available
rem under the terms of the License "Eclipse Public License v1.0"
rem which accompanies this distribution, and is available
rem at the URL "http://www.eclipse.org/legal/epl-v10.html".
rem
rem Initial Contributors:
rem Nokia Corporation - initial contribution.
rem
rem Contributors:
rem
rem Description:
rem

REM Check argument to see if epoc.ini should be created 
@if "%1" equ "btb" (
@set BTB_BUILD=@rem
) else (
@set BTB_BUILD=
)

setlocal

REM Get path to F32TEST directory
call :GetParentDirPath F32TEST_PATH
echo F32TEST_PATH = %F32TEST_PATH%
set EPOC32_DIR=%EPOCROOT%EPOC32\
set F32TEST_BUILD_PATH=%EPOC32_DIR%BUILD%F32TEST_PATH%GROUP\
if exist %EPOC32_DIR%data\z\test\f32test set F32TEST_BUILD_PATH=%EPOC32_DIR%data\z\test\f32test\
echo F32TEST_BUILD_PATH = %F32TEST_BUILD_PATH%
echo EPOC32_DIR = %EPOC32_DIR%
set EMUL_MEDIA_PATH=%EPOC32_DIR%DATA\MEDIA\
echo EMUL_MEDIA_PATH = %EMUL_MEDIA_PATH%
set RELEASE_PATH=%EPOC32_DIR%RELEASE\
echo RELEASE_PATH = %RELEASE_PATH%

REM If BASEPATH not set by basedoit.bat, because this is not a BTB build, then assume \os\
if "%BASEPATH%" == "" (
	set BASEPATH=\os\
)
echo BASEPATH = %BASEPATH%

call :DoSporadic
call :ClearEmulMedia
call :SetupCDrive wins
call :SetupCDrive winscw
call :SetupZDrive wins udeb
call :SetupZDrive wins urel
call :SetupZDrive winscw udeb
call :SetupZDrive winscw urel
%BTB_BUILD% call :SetupEpocDotIni

endlocal
goto :eof

:GetParentDirPath
call :GetPath %1 %~p0.
goto :eof

:GetPath
set %1=%~p2
goto :eof

:DoSporadic
if exist \asdfasdf		rmdir /s /q \asdfasdf
if exist \red			rmdir /s /q \red
if exist \f32-tst		rmdir /s /q \f32-tst
if exist \ford			rmdir /s /q \ford
if exist \session_test	rmdir /s /q \session_test
if exist \tmisc			rmdir /s /q \tmisc
if exist \volvo			rmdir /s /q \volvo
if exist \blue			del /q \blue
goto :eof

:ClearEmulMedia
if exist %EMUL_MEDIA_PATH% del /q %EMUL_MEDIA_PATH%*.bin
cd %EPOCROOT%
if exist %BASEPATH%emulator\wins\emuldrives.zip (
	Call unZip %BASEPATH%emulator\wins\emuldrives.zip
) else (
	Call unZip %BASEPATH%boardsupport\emulator\emulatorbsp\emuldrives.zip
)
goto :eof

:SetupCDrive
if exist %EPOC32_DIR%%1\C			rmdir /s /q %EPOC32_DIR%%1\C 2>NUL
if not exist %EPOC32_DIR%%1\C\F32	mkdir %EPOC32_DIR%%1\C\F32
goto :eof

:SetupZDrive
setlocal
set REL_DIR=%RELEASE_PATH%%1\%2
set EMUL_Z=%REL_DIR%\Z
echo REL_DIR = %REL_DIR%
echo EMUL_Z = %EMUL_Z%

if exist %EMUL_Z%		rmdir /s /q %EMUL_Z% 2>NUL
if not exist %EMUL_Z%\NOTINPATH		mkdir %EMUL_Z%\NOTINPATH
if not exist %EMUL_Z%\TEST			mkdir %EMUL_Z%\TEST
copy %F32TEST_PATH%\SERVER\T_FILE.CPP %EMUL_Z%\TEST\T_FILE.CPP
copy %F32TEST_PATH%\SERVER\T_FSRV.CPP %EMUL_Z%\TEST\T_FSRV.CPP
copy %F32TEST_PATH%\SERVER\T_RDSECT.TXT %EMUL_Z%\TEST\T_RDSECT.TXT
copy %F32TEST_BUILD_PATH%\%1.AUTO.BAT %EMUL_Z%\TEST\%1.AUTO.BAT
copy %REL_DIR%\T_CHKUID.EXE %EMUL_Z%\TEST\T_CHKUID.EXE

REM Use both Sys and System directories until the switch has been made

if not exist %EMUL_Z%\SYS\LIBS	mkdir %EMUL_Z%\SYS\LIBS
if not exist %EMUL_Z%\SYS\BIN	mkdir %EMUL_Z%\SYS\BIN
copy %REL_DIR%\ESHELL.EXE %EMUL_Z%\SYS\BIN\ESHELL.EXE
copy %REL_DIR%\T_PREL.DLL %EMUL_Z%\SYS\BIN\T_PREL.DLL
if not exist %EMUL_Z%\SYS\DATA	mkdir %EMUL_Z%\SYS\DATA
copy %F32TEST_PATH%\SERVER\PRELOAD.LST %EMUL_Z%\SYS\DATA\PRELOAD.LST
if exist %F32TEST_PATH%\..\EMULATOR\WINS_RESTRICTED\UNISTORE2\ESTART\ESTARTXSR.TXT (
	copy %F32TEST_PATH%\..\EMULATOR\WINS_RESTRICTED\UNISTORE2\ESTART\ESTARTXSR.TXT %EMUL_Z%\SYS\DATA\ESTART.TXT
) else (
	copy %BASEPATH%boardsupport\emulator\unistore2emulatorsupport\estart\estartxsr.txt %EMUL_Z%\SYS\DATA\ESTART.TXT
)

if not exist %EMUL_Z%\SYSTEM\LIBS	mkdir %EMUL_Z%\SYSTEM\LIBS
if not exist %EMUL_Z%\SYSTEM\BIN	mkdir %EMUL_Z%\SYSTEM\BIN
copy %REL_DIR%\ESHELL.EXE %EMUL_Z%\SYSTEM\BIN\ESHELL.EXE
copy %REL_DIR%\T_PREL.DLL %EMUL_Z%\SYSTEM\BIN\T_PREL.DLL
if not exist %EMUL_Z%\SYSTEM\DATA	mkdir %EMUL_Z%\SYSTEM\DATA
copy %F32TEST_PATH%\SERVER\PRELOAD.LST %EMUL_Z%\SYSTEM\DATA\PRELOAD.LST
copy %F32TEST_PATH%\SERVER\corruptTest\CorruptFileNames.lst %EMUL_Z%\SYSTEM\DATA\CorruptFileNames.lst
copy %F32TEST_PATH%\SERVER\corruptTest\BadFile1.txt %EMUL_Z%\SYSTEM\DATA\BadFile1.txt
copy %F32TEST_PATH%\SERVER\corruptTest\BadFile2.txt %EMUL_Z%\SYSTEM\DATA\BadFile2.txt

REM for T_SYSBIN
copy %REL_DIR%\T_SYSBIN.EXE %EMUL_Z%\SYS\BIN\T_SYSBINa.EXE
if not exist %EMUL_Z%\SYSTEM\PROGRAMS	mkdir %EMUL_Z%\SYSTEM\PROGRAMS
copy %REL_DIR%\T_SYSBIN.EXE %EMUL_Z%\SYSTEM\PROGRAMS\T_SYSBINb.EXE
copy %REL_DIR%\T_SYSBIN_DLL.DLL %EMUL_Z%\SYS\BIN\T_SYSBIN_DLLa.DLL
copy %REL_DIR%\T_SYSBIN_DLL.DLL %EMUL_Z%\SYS\BIN\T_SYSBIN_DLL_RAM.DLL
if not exist %EMUL_Z%\SYSTEM\LIBS	mkdir %EMUL_Z%\SYSTEM\LIBS
copy %REL_DIR%\T_SYSBIN_DLL.DLL %EMUL_Z%\SYSTEM\LIBS\T_SYSBIN_DLLb.DLL

REM for T_VIRUS
copy %REL_DIR%\t_vshook.pxt %EMUL_Z%\Test\t_vshook.pxt
copy %F32TEST_PATH%\plugins\version_1\virus\virusdef.txt %EMUL_Z%\Test\virusdef.txt
copy %F32TEST_PATH%\plugins\version_1\virus\virus1.txt %EMUL_Z%\Test\virus1.txt
copy %F32TEST_PATH%\plugins\version_1\virus\virus2.txt %EMUL_Z%\Test\virus2.txt
copy %F32TEST_PATH%\plugins\version_1\virus\clean.txt %EMUL_Z%\Test\clean.txt

REM for t_findcapall and t_findcapnone
copy %F32TEST_PATH%\SERVER\t_findcaptestfile.txt %EMUL_Z%\SYS\BIN\t_findcaptestfile.txt

REM for T_PLUGIN_V2BETA
copy %REL_DIR%\t_enchook.pxt %EMUL_Z%\Test\t_enchook.pxt
copy %REL_DIR%\t_hexhook.pxt %EMUL_Z%\Test\t_hexhook.pxt
copy %REL_DIR%\t_formathook.pxt %EMUL_Z%\Test\t_formathook.pxt

endlocal
goto :eof

:SetupEpocDotIni
if exist %EPOCROOT%epoc32\data\epoc.ini (	
	echo epoc.ini already exists and hasn't been overwritten
	echo epoc.ini may not contain build-and-test-system settings
)
if not exist %EPOCROOT%epoc32\data\epoc.ini (
	REM these settings should replicate those settings 
	REM used for the build-and-test-system see how 
	REM basetests.ini gets configured in 
	REM //EPOC/development/base/tools/master/common/basedoit.bat
	echo textshell >%EPOCROOT%epoc32\data\epoc.ini
	echo timerresolution 1 >>%EPOCROOT%epoc32\data\epoc.ini
	echo _epoc_drive_t %EPOCROOT%epoc32\build>>%EPOCROOT%epoc32\data\epoc.ini
	echo justintime none>>%EPOCROOT%epoc32\data\epoc.ini
	echo debugmask panic>>%EPOCROOT%epoc32\data\epoc.ini
	echo logtimestamp 0 >>%EPOCROOT%epoc32\data\epoc.ini
	echo logthreadid 0 >>%EPOCROOT%epoc32\data\epoc.ini
	echo FlashEraseTime 50000 >>%EPOCROOT%epoc32\data\epoc.ini
	echo FlashResumeTime 0 >>%EPOCROOT%epoc32\data\epoc.ini
	echo FlashWriteTime 0 >>%EPOCROOT%epoc32\data\epoc.ini
	echo NandDriverType=XSR >>%EPOCROOT%epoc32\data\epoc.ini
	echo MediaExtensionDriver=?medtestnfe.pdd >>%EPOCROOT%epoc32\data\epoc.ini
	if exist %BASEPATH%e32\rombuild\platsec.settings (
		type %BASEPATH%e32\rombuild\platsec.settings >>%EPOCROOT%epoc32\data\epoc.ini
	) else (
		type %BASEPATH%kernelhwsrv\kernel\eka\rombuild\platsec.settings >>%EPOCROOT%epoc32\data\epoc.ini
	)
)
goto :eof

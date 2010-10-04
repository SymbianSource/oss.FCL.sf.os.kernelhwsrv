@echo off
rem
rem Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
rem All rights reserved.
rem This component and the accompanying materials are made available
rem under the terms of the License "Eclipse Public License v1.0"
rem which accompanies this distribution, and is available
rem at the URL "http://www.eclipse.org/legal/epl-v10.html".
rem

rem Nokia Corporation - initial contribution.
rem
rem Contributors:
rem
rem Description:
rem
setlocal
set IMAGE_PATH=\_vmx\
set IMAGE_NAME=floppy
set ROMBUILD_PATH=\os\kernelhwsrv\kernel\eka\rombuild\
set X86PC_PATH=\os\boardsupport_internal\x86pc\
set TEMP_PATH=%IMAGE_PATH%_bfi_temp
if exist "%ProgramFiles%\VMWare\VMWare Workstation" (
	set VMPLAYER=%ProgramFiles%\VMWare\VMWare Workstation\vmplayer.exe
)
if exist "%ProgramFiles%\VMWare\VMWare Player" (
	set VMPLAYER=%ProgramFiles%\VMWare\VMWare Player\vmplayer.exe
)
set LOGFILE=%IMAGE_PATH%serial.out
md %IMAGE_PATH% 2>NUL
md %IMAGE_PATH%_bfi_temp 2>NUL
unzip -o -d %IMAGE_PATH% %ROMBUILD_PATH%symbian.vmx.zip
call %ROMBUILD_PATH%copyx86 %1 %TEMP_PATH%
copy /y %X86PC_PATH%pcboot\epocboot.com %TEMP_PATH%\epocboot.bin
pushd
echo bfi -t=%2 -f=%IMAGE_PATH%%IMAGE_NAME% -b=%X86PC_PATH%pcboot\BOOT.COM -o=epocboot.bin %TEMP_PATH%
call bfi -t=%2 -f=%IMAGE_PATH%%IMAGE_NAME% -b=%X86PC_PATH%pcboot\BOOT.COM -o=epocboot.bin %TEMP_PATH%
popd
del /f %LOGFILE%
start "%VMPLAYER%" "%IMAGE_PATH%symbian.vmx"

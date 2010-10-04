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
set IMAGE_PATH=\
set IMAGE_NAME=floppy
set X86PC_PATH=\os\boardsupport_internal\x86pc\
set TEMP_PATH=%IMAGE_PATH%_bfi_temp
rem set BOCHS_PATH=C:\Program Files\Bochs-2.2.1
set BOCHS_PATH=C:\Program Files\Bochs-2.2.6
set BOCHS=bochsdbg.exe
rem set BOCHS=bochs-smp.exe
md %IMAGE_PATH% 2>NUL
md %IMAGE_PATH%_bfi_temp 2>NUL
pushd \os\kernelhwsrv\kernel\eka\rombuild 2>NUL
call copyx86 %1 %TEMP_PATH%
popd
copy /y %X86PC_PATH%pcboot\epocboot.com %TEMP_PATH%\epocboot.bin
pushd
echo bfi -t=288 -f=%IMAGE_PATH%%IMAGE_NAME% -b=%X86PC_PATH%pcboot\BOOT.COM -o=epocboot.bin %TEMP_PATH%
call bfi -t=288 -f=%IMAGE_PATH%%IMAGE_NAME% -b=%X86PC_PATH%pcboot\BOOT.COM -o=epocboot.bin %TEMP_PATH%
if errorlevel 1 (
	popd
	goto :EOF
)
copy /y %IMAGE_PATH%%IMAGE_NAME% "%BOCHS_PATH%"
pushd "%BOCHS_PATH%"
call %BOCHS% -q
popd
popd

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

trace 2 2
z:
cd test


rem -------------------------------------------------------
rem -- run f32 performance tests for FAT file system only 
rem -- using MMC slot, drive E:
rem -- note, that MMC drive can be mapped onto different drive letters, e.g. D: or E:, depending on estart.txt



rem ----------------------------------------------------------------------------------------
rem --- File System performance tests set on FAT

echo <TST__FS_PERFORMANCE_TESTS_FAT>
echo ### starting FS performance tests on FAT File System

format c:
format e:

runtests f32_perf_test.bat -st -t 5400
echo <\TST__FS_PERFORMANCE_TESTS_FAT>






rem ----------------------------------------------------------------------------------------
rem --- Dir. operations performance tests set on FAT


echo <TST__DIRECTORY_PERFORMANCE_TESTS_FAT>

echo ### starting directory operations performance tests on FAT File System

format c:
format e:

runtests fat_perf_test.bat -st -t 5400 
echo <\TST__DIRECTORY_PERFORMANCE_TESTS_FAT>




rem ----------------------------------------------------------------------------------------
rem --- Crash the board
echo ### crashing the board
crash




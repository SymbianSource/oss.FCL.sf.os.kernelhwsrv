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
rem -- run f32 performance tests for FAT and the for exFAT filesystems
rem -- Also runs standard F32 tests for FAT and exFAT
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
rem --- Standard F32 test on FAT (The intention is to run them on FAT32, it implies appropriate media)


echo <TST__F32_STANDARD_TESTS_FAT>

echo ### starting standard F32 tests on FAT(32) File System 

format c:
format e:

runtests f32test.auto.bat -d e -st -t 5400 

echo <\TST__F32_STANDARD_TESTS_FAT>



rem ############################################
rem #### mount exFAT and re-run tests on it.
mount e: /u
mount e: fsy:exfat fs:exfat
rem ############################################



rem ----------------------------------------------------------------------------------------
rem --- File System performance tests set on exFAT. Use 5000 files for performance tests (it will be faster)

echo <TST__FS_PERFORMANCE_TESTS_EXFAT>
echo ### starting FS performance tests on exFAT File System

format c:
format e:

runtests f32_perf_test_5000files.bat  -st -t 5400
echo <\TST__FS_PERFORMANCE_TESTS_EXFAT>



rem ----------------------------------------------------------------------------------------
rem --- Dir. operations performance tests set on exFAT


echo <TST__DIRECTORY_PERFORMANCE_TESTS_EXFAT>

echo ### starting directory operations performance tests on exFAT File System

format c:
format e:

runtests fat_perf_test.bat -st -t 5400 
echo <\TST__DIRECTORY_PERFORMANCE_TESTS_EXFAT>



rem ----------------------------------------------------------------------------------------
rem --- Standard F32 test on exFAT 


echo <TST__F32_STANDARD_TESTS_EXFAT>

echo ### starting standard F32 tests on exFAT File System 

format c:
format e:

runtests f32test.auto.bat -d e -st -t 5400 

echo <\TST__F32_STANDARD_TESTS_EXFAT>




rem ----------------------------------------------------------------------------------------
rem --- Crash the board
echo ### crashing the board
crash


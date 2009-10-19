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
rem -------------------------------------------------------------------------------------------------------------------------
rem @SYMTestCaseID 		PBASE-FAT-PERF-1248	
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by creating a file when DirCache has 1.7 MB data
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB in ESTART.TXT
rem				2. Fill DirCache with 1.7 MB data
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Create  FILE_1.TXT in \\D1\\
rem				4. Measure the  time taken to create a file
rem
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB
rem				2. DirCache filled with 1.7 MB data
rem				3. Files Creation Successful
rem				4. Time in ms
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------------

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 1

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0
t_fat_perf e -c Create -p D1 -b FILE_1 -n 1 -m 0


rem -------------------------------------------------------------------------------------------------------------------------
rem @SYMTestCaseID 		PBASE-FAT-PERF-1249	
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by creating a file when DirCache has 3.4MB data
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB in ESTART.TXT
rem				2. Fill DirCache with 3.4 MB data
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Create  FILE_1.TXT in \\D1\\D2\\
rem				4. Measure the  time taken to create a file
rem
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB
rem				2. DirCache filled with 3.4MB data
rem				3. Files Creation Successful
rem				4. Time in ms
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------------

t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0
t_fat_perf e -c Create -p D1\D2 -b FILE_1 -n 1 -m 0


rem ----------------------------------------------------------------------------------------------
rem @SYMTestCaseID 		PBASE-FAT-PERF-1250	
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by creating a file when DirCache has 5.1MB data
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB in ESTART.TXT
rem				2. Fill DirCache with 5.1 MB data
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Create  FILE_1.TXT in \\D1\\D2\\ D3\\
rem				4.  Measure the  time taken to create a file
rem			
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB
rem				2. DirCache filled with  5.1MB data
rem				3. Files Creation Successful
rem				4. Time in ms
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem ----------------------------------------------------------------------------------------------

t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0
t_fat_perf e -c Create -p D1\D2\D3 -b FILE_1 -n 1 -m 0


rem ----------------------------------------------------------------------------------------------
rem @SYMTestCaseID 		PBASE-FAT-PERF-1360	
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by creating a file when DirCache has 6.8MB data
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB in ESTART.TXT
rem				2. Fill DirCache with 6.8 MB data
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Create  FILE_1.TXT in \\D1\\D2\\ D3\\ D4\\
rem				4.  Measure the  time taken to create a file
rem
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB
rem				2. DirCache filled with 6.8MB data
rem				3. Files Creation Successful
rem				4. Time in ms
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem ----------------------------------------------------------------------------------------------
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0
t_fat_perf e -c Create -p D1\D2\D3\D4 -b FILE_1 -n 1 -m 0


rem ---Cleanup------------------------------------------------------------------------------------
rem ---Do not consider this for performance measurement

t_fat_perf e -c Delete -p D1 -b FILE_1 -n 1 -m 0
t_fat_perf e -c Delete -p D1\D2 -b FILE_1 -n 1 -m 0
t_fat_perf e -c Delete -p D1\D2\D3\ -b FILE_1 -n 1 -m 0
t_fat_perf e -c Delete -p D1\D2\D3\D4 -b FILE_1 -n 1 -m 0

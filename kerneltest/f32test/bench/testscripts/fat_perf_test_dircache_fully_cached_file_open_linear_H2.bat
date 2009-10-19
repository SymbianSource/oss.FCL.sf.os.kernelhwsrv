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
rem -------------------------------------------------------------------------------------------------------------------
rem Open 8000 file in each of the directories in D1\D2\D3\D4 to fill the dir cache
rem This is the pre-condition to run all of the test cases below. This covers step#2 in @SYMTestActions in all cases below.
rem Note: It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 1
t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0
t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0


rem -------------------------------------------------------------------------------------------------------------------
rem @SYMTestCaseID 		PBASE-FAT-PERF-1361	
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by opening multiple files linearly when DirCache has 1.7MB data
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB in ESTART.TXT
rem				2. Fill DirCache with 1.7MB data
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Open the following files under \\D1\\: 
rem					ABCD~_20
rem					ABCD~_200
rem					ABCD~_1000
rem					ABCD~_3000
rem					ABCD~_4000
rem					ABCD~_6000
rem					ABCD~_8000
rem				4. Measure the time taken to open each of these files
rem		
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB
rem				2. DirCache filled with 1.7MB data
rem				3. File Open Successful
rem				4. Time in ms for each file
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------


t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_20 -n 1 -m 0
t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_200 -n 1 -m 0
t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1000 -n 1 -m 0
t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_3000 -n 1 -m 0
t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_4000 -n 1 -m 0
t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_6000 -n 1 -m 0
t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_8000 -n 1 -m 0

rem -------------------------------------------------------------------------------------------------------------------
rem @SYMTestCaseID 		PBASE-FAT-PERF-1362	
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by opening multiple files linearly when DirCache has 3.4MB data
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB in ESTART.TXT
rem				2. Fill DirCache with 3.4MB data 
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Open the following files under \\D1\\ D2\\  : 
rem					ABCD~_20
rem					ABCD~_200
rem					ABCD~_1000
rem					ABCD~_3000
rem					ABCD~_4000
rem					ABCD~_6000
rem					ABCD~_8000
rem				4. Measure the time taken to open each of these files
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB
rem				2. DirCache filled with 3.4MB data
rem				3. File Open Successful
rem				4. Time in ms for each file
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0

t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_20 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_200 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_3000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_4000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_6000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_8000 -n 1 -m 0

rem -------------------------------------------------------------------------------------------------------------------
rem @SYMTestCaseID 		PBASE-FAT-PERF-1363
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by opening multiple files linearly when DirCache has 5.1MB data	
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB in ESTART.TXT
rem				2. Fill DirCache with 5.1MB data
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Open the following files under \\D1\\ D2\\ D3\\: 
rem					ABCD~_20
rem					ABCD~_200
rem					ABCD~_1000
rem					ABCD~_3000
rem					ABCD~_4000
rem					ABCD~_6000
rem					ABCD~_8000
rem				4. Measure the  time taken to open each of these files
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB
rem				2. DirCache filled with 5.1MB data
rem				3. File Open Successful
rem				4. Time in ms for each file
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0

t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_20 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_200 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_3000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_4000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_6000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_8000 -n 1 -m 0

rem -------------------------------------------------------------------------------------------------------------------
rem @SYMTestCaseID 		PBASE-FAT-PERF-1364	
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by opening multiple files linearly when DirCache has 6.8MB data	
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB in ESTART.TXT
rem				2. Fill DirCache with 6.8MB data
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Open the following files under \\D1\\ D2\\ D3\\ D4\\ : 
rem					ABCD~_20
rem					ABCD~_200
rem					ABCD~_1000
rem					ABCD~_3000
rem					ABCD~_4000
rem					ABCD~_6000
rem					ABCD~_8000
rem				4. Measure the  time taken to open each of these files
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  =  128 KB and FAT_MaxDirCacheSize  = 9216 KB
rem				2. DirCache filled with 6.8MB data
rem				3. File Open Successful
rem				4. Time in ms for each file
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0

t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_20 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_200 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_3000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_4000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_6000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_8000 -n 1 -m 0

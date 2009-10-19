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
rem @SYMTestCaseID 		PBASE-FAT-PERF-1375	
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by opening multiple files linearly when 6.8MB data is used with 2MB DirCache 
rem				(i.e Partially Cached - 30% of data cached)
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  = 128 KB  and  FAT_MaxDirCacheSize  = 2048 KB  in ESTART.TXT
rem				2. Try to fill DirCache with 6.8 MB data
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Open the following files under \\D1\\D2\\D3\\D4\\
rem					ABCD~_20
rem					ABCD~_200
rem					ABCD~_1000
rem					ABCD~_3000
rem					ABCD~_4000
rem					ABCD~_6000
rem					ABCD~_8000
rem				4. Measure the  time taken to open each of these files
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  = 128 KB  and  FAT_MaxDirCacheSize  = 2048 KB  
rem				2. DirCache filled with 2MB data only
rem				3. File Open Successful
rem				4. Time in ms for each file
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 1
t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0
t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0

t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_20 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_200 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_3000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_4000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_6000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_8000 -n 1 -m 0


rem -------------------------------------------------------------------------------------------------------------------
rem @SYMTestCaseID 		PBASE-FAT-PERF-1376	
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by opening multiple files randomly when 6.8MB data is used with 2MB DirCache 
rem				(i.e Partially Cached - 30% of data cached)
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  = 128 KB  and  FAT_MaxDirCacheSize  =  2048 KB  in ESTART.TXT
rem				2. Try to fill DirCache with 8 MB data
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Open the following files under \\D1\\D2\\D3\\D4\\
rem					ABCD~_8000
rem					ABCD~_20
rem					ABCD~_4000
rem					ABCD~_1000
rem					ABCD~_3000
rem					ABCD~_200
rem					ABCD~_6000
rem				4. Measure the  time taken to open each of these files
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  = 128 KB  and  FAT_MaxDirCacheSize  =  2048 KB  
rem				2. DirCache filled with 2MB data only
rem				3. File Open Successful
rem				4. Time in ms for each file
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 1
t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0
t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0


t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0

t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_8000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_20 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_4000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_3000 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_200 -n 1 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_6000 -n 1 -m 0


rem -------------------------------------------------------------------------------------------------------------------
rem @SYMTestCaseID 		PBASE-FAT-PERF-1377	
rem @SYMTestType    		PT	
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		To check the directory cache performance by creating a file when 6.8MB data is used with 2MB DirCache
rem				(i.e Partially Cached - 30% of data cached)
rem @SYMTestActions		0. Make sure the Test pre-settings
rem				1. Set FAT_MinDirCacheSize  =  128 KB  and  FAT_MaxDirCacheSize  = 2048 KB in ESTART.TXT
rem				2. Try to fill DirCache with 6.8MB data
rem				   Note: Before the next step, ABCD~_1 file is opened at root dir \D1\ to set the LeafDirCache.
rem				   This should not be accounted for measurement purpose.
rem				3. Create  FILE_2.TXT in \\D1\\D2\\D3\\D4\\
rem				4. Measure the time taken to create a file
rem @SYMTestExpectedResults	1. FAT_MinDirCacheSize  =  128 KB  and  FAT_MaxDirCacheSize  = 2048 KB
rem				2. DirCache filled with 2MB data only
rem				3. Files Creation Successful
rem				4. Time in ms
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------


t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 1
t_fat_perf e -c Open -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0
t_fat_perf e -c Open -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0
t_fat_perf e -c Open -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0

t_fat_perf e -c Open -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 -n 1 -m 0
t_fat_perf e -c Create -p D1\D2\D3\D4 -b FILE_2 -n 1 -m 0


rem ---Cleanup------------------------------------------------------------------------------------
rem ---Do not consider this for performance measurement

t_fat_perf e -c Delete -p D1\D2\D3\D4 -b FILE_2 -n 1 -m 0


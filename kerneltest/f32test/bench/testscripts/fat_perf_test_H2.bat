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

t_fat_perf e -c Setup 


t_fat_perf e -c Create -p DIR1 -b ONEMOREFILE_1 -n 1 -m 1

t_fat_perf e -c Create -p DIR1\DIR11 -b ONEMOREFILE_1 -n 1 -m 1

t_fat_perf e -c Create -p DIR1\DIR12RAN -b ONEMOREFILE_1 -n 1 -m 1

t_fat_perf e -c Create -p DIR1\DIR11\DIR111 -b ONEMOREFILE_1 -n 1 -m 1



t_fat_perf e -c Create -p DIR1 -b SMALL_1 -n 1 -m 1

t_fat_perf e -c Create -p DIR1\DIR11 -b SMALL_1 -n 1 -m 1

t_fat_perf e -c Create -p DIR1\DIR12RAN -b SMALL_1 -n 1 -m 1

t_fat_perf e -c Create -p DIR1\DIR11\DIR111 -b SMALL_1 -n 1 -m 1



t_fat_perf e -c Create -p DIR1\DIR11\DIR111 -b ANOTHERLONGFILENAME_11 -n 1 -m 1

t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1500 -n 1 -m 1
t_fat_perf e -c Create -p DIR1\DIR11\DIR111 -b MOREFILE_1 -n 1 -m 0

t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1500 -n 1 -m 1
t_fat_perf e -c Open -p DIR2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1200 -n 1 -m 0
t_fat_perf e -c Create -p DIR1\DIR11\DIR111 -b NEWFILE_1 -n 1 -m 0



t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1600 -n 1 -m 1

t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1600 -n 1 -m 1

t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1600 -n 1 -m 1



t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1100 -n 1 -m 1
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1200 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1300 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1400 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1500 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1600 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1700 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1800 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1900 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_2000 -n 1 -m 0

t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1000 -n 1 -m 1
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1100 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1200 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1300 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1400 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1500 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1600 -n 1 -m 0

t_fat_perf e -c Open -p DIR1\DIR11\DIR111  -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1000 -n 1 -m 1
t_fat_perf e -c Open -p DIR1\DIR11\DIR111  -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1100 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111  -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1200 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111  -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1300 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111  -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1400 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111  -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1500 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111  -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1600 -n 1 -m 0



t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1700 -n 1 -m 1
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_2 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_300 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1400 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_100 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1900 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1100 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_800 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_200 -n 1 -m 0
t_fat_perf e -c Open -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1500 -n 1 -m 0

t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1600 -n 1 -m 1
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_2 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_300 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1400 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_100 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1100 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_800 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_200 -n 1 -m 0

t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1600 -n 1 -m 1
t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_2 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_300 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1400 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_100 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1100 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_800 -n 1 -m 0
t_fat_perf e -c Open -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_200 -n 1 -m 0



t_fat_perf e -c Delete -p DIR1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1500 -n 1 -m 1

t_fat_perf e -c Delete -p DIR1\DIR11 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1500 -n 1 -m 1

t_fat_perf e -c Delete -p DIR1\DIR11\DIR111 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1500 -n 1 -m 1

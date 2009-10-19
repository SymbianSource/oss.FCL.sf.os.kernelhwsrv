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
rem @SYMPREQ			PREQ1885
rem @SYMTestCaseDesc		Test Setup for Directory Cache performance tests - REQ10565	
rem @SYMTestActions		1. Format the card with cluster size = 4096 bytes
rem				2. Create nested directories \D1\D2\D3\D4 and each containing 8000 files, where file lenth is :
rem				ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_1 ~ 8000.txt
rem 				
rem @SYMTestExpectedResults	1. Card is formatted with cluster size = 4096 bytes
rem				2. Directories \D1\D2\D3\D4 and 8000 files are created successfully
rem Note:			It is assumed that memory card is mounted on e: drive on the target board
rem -------------------------------------------------------------------------------------------------------------------

format e: spc:8

md e:\D1
t_fat_perf e -c create -p D1 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 1

md e:\D1\D2
t_fat_perf e -c create -p D1\D2 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0

md e:\D1\D2\D3
t_fat_perf e -c create -p D1\D2\D3 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0

md e:\D1\D2\D3\D4
t_fat_perf e -c create -p D1\D2\D3\D4 -b ABCD1ABCD2ABCD3ABCD4ABCD5ABCD6ABCD7ABCD8ABCD9ABCD1ABCD2ABCD3ABCD4ABCD_ -n 8000 -m 0

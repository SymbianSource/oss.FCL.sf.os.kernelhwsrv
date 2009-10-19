// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// //File Name:	f32test\plugins\version_2\file64bit\src\t_file64bit_plugin.cpp
// //Description:This file tests the plugin framework to ensure that it
// //			  receives 64bit data correctly from the application.
// 
//




#include <f32file.h>
#include <f32dbg.h>
#include <e32test.h>
#include "t_server.h"
#include "t_chlffs.h"
#include "file64bit_plugin.h"
#include "t_file64bit_plugin.h"

#include <e32def.h>
#include <e32def_private.h>

RTest test(_L("Plugin Framework Test for Large File Support"));

#define _plugin_test(r)		if(!r) { TheFs.DismountPlugin(KFile64BitPluginName); test(r); }


// : public RPlugin
TInt RFile64BitPlugin::DoControl(TInt aFunction,TDes8& aDesc) const
	{
	return RPlugin::DoControl(aFunction, aDesc);
	}
	
void TestPluginData(TInt aValueId, TInt64 aExpectedValue)
//
// function to ensure that the data received by the plugin from the
// file server is correct.
//
	{
	TInt r;
	RFile64BitPlugin filePlugin;
	TInt64 pluginData;
	TPckg<TInt64> pkPluginData(pluginData);
	
	r = filePlugin.Open(TheFs,KFile64BitPluginPos);
	test(KErrNone == r);
	
	r = filePlugin.DoControl(aValueId, pkPluginData);
	test(KErrNone ==r);
	test(pluginData == aExpectedValue);
		
	filePlugin.Close();
	}


/**
@SYMTestCaseID      PBASE-T_FILE64BIT_PLUGIN-2356
@SYMTestPriority    High
@SYMTestRequirement REQ9542 
@SYMTestType        CIT
@SYMTestCaseDesc    Test that the 64-bit position in RFile64::Read()
					is correctly intercepted by the plugin.
@SYMTestActions     
1) Create a file and set its size to 4GB-1.
2) Read from position 0.
3) Check that the plugin has intercepted the data correctly.
4) Read from position 2GB.
5) Check that the plugin has intercepted the data correctly.
6) Read from position 3GB.
7) Check that the plugin has intercepted the data correctly.
8) Read from position 3GB, by making it the current position.
9) Check that the plugin has intercepted the data correctly.
@SYMTestExpectedResults
1) KErrNone.
2) KErrNone.
3) Data has been intercepted correctly.
4) KErrNone.
5) Data has been intercepted correctly.
6) KErrNone.
7) Data has been intercepted correctly.
8) KErrNone.
9) Data has been intercepted correctly.
@SYMTestStatus      Implemented
*/
void TestPluginFileRead()
	{
	test.Next(_L("RFile64::Read() test for plugin\n"));
	RFile64 file;
	TFullName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(_L(":\\F32-TST\\plugintestfile.txt"));
	
	test.Next(_L("Create a file and set its size to 4GB-1.\n"));
	TInt r = file.Replace(TheFs, fileName, EFileWrite|EFileRead);
	test(KErrNone == r);
	
	r = file.SetSize(K4GBMinusOne);
	test(KErrNone == r);

	TBuf8<256> readBuf;
	
	test.Next(_L("Read from position 0 and Check that the plugin has intercepted the data correctly.\n"));
	r = file.Read(0,readBuf);
	test(KErrNone == r);
	
	TestPluginData(KGetPosition,0);
	
	test.Next(_L("Read from position 2GB and Check that the plugin has intercepted the data correctly.\n"));
	r = file.Read(K2GB,readBuf);
	test(KErrNone == r);
	
	TestPluginData(KGetPosition,K2GB);
	
	test.Next(_L("Read from position 3GB and Check that the plugin has intercepted the data correctly.\n"));
	r = file.Read(K3GB,readBuf);
	test(KErrNone == r);
	
	TestPluginData(KGetPosition,K3GB);
	
	test.Next(_L("Read from current position and Check that the plugin has intercepted the data correctly.\n"));
	TInt64 pos = K3GB;
	r = file.Seek(ESeekStart,pos);
	test(KErrNone == r);
	
	r = file.Read(readBuf);
	test(KErrNone == r);
	
	TestPluginData(KGetPosition,K3GB);
	
	file.Close();
	
	r = TheFs.Delete(fileName);
	test(r == KErrNone);
	
	}
/**
@SYMTestCaseID      PBASE-T_FILE64BIT_PLUGIN-2357
@SYMTestPriority    High
@SYMTestRequirement REQ9542 
@SYMTestType        CIT
@SYMTestCaseDesc    Test that the 64-bit position in RFile64::Write()
					is correctly intercepted by the plugin.
@SYMTestActions     
1) Create a file, and set its size to 4GB-1
2) Write to position 0.
3) Check that the plugin has intercepted the data correctly.
4) Write to position 2GB.
5) Check that the plugin has intercepted the data correctly.
6) Write to position 3GB.
7) Check that the plugin has intercepted the data correctly.
8) Write to position 3GB, by making it the current position.
9) Check that the plugin has intercepted the data correctly.
@SYMTestExpectedResults
1) KErrNone.
2) KErrNone.
3) Data has been intercepted correctly.
4) KErrNone.
5) Data has been intercepted correctly.
6) KErrNone.
7) Data has been intercepted correctly.
8) KErrNone.
9) Data has been intercepted correctly.
@SYMTestStatus      Implemented
*/
	
	
void TestPluginFileWrite()
	{
	test.Next(_L("RFile64::Write() test for plugin\n"));
	RFile64 file;
	TFullName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(_L(":\\F32-TST\\plugintestfile.txt"));
	
	test.Next(_L("Create a file and set its size to 4GB-1.\n"));
	TInt r = file.Replace(TheFs, fileName, EFileWrite|EFileRead);
	test(KErrNone == r);
	
	r = file.SetSize(K4GBMinusOne);
	test(KErrNone == r);


	test.Next(_L("Write to position 0 and Check that the plugin has intercepted the data correctly.\n"));
	TBuf8<256> writeBuf(_L8("Test Data"));
	r = file.Write(writeBuf);
	test(KErrNone == r);
	
	TestPluginData(KGetPosition,0);

	test.Next(_L("Write to position 2GB and Check that the plugin has intercepted the data correctly.\n"));	
	r = file.Write(K2GB,writeBuf);
	test(KErrNone == r);
	
	TestPluginData(KGetPosition,K2GB);
	
	test.Next(_L("Write to position 3GB and Check that the plugin has intercepted the data correctly.\n"));
	r = file.Write(K3GB,writeBuf);
	test(KErrNone == r);
	
	TestPluginData(KGetPosition,K3GB);
	
	test.Next(_L("Write to current position and Check that the plugin has intercepted the data correctly.\n"));
	TInt64 pos = K3GB;
	r = file.Seek(ESeekStart,pos);
	test(KErrNone == r);
	
	r = file.Write(writeBuf);
	test(KErrNone == r);
	
	TestPluginData(KGetPosition,K3GB);
	
	file.Close();
	
	r = TheFs.Delete(fileName);
	test(r == KErrNone);
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT_PLUGIN-2358
@SYMTestPriority    High
@SYMTestRequirement REQ9542 
@SYMTestType        CIT
@SYMTestCaseDesc    Test that the 64-bit size in RFile64::SetSize()
					and RFile64::Size() is correctly intercepted 
					by the plugin.
@SYMTestActions     
1) Create a file.
2) Set the size of the file to 2GB using RFile64::SetSize()
3) Check that the plugin has intercepted the data correctly.
4) Get the size of the file using RFile64::Size()
5) Check that the plugin has intercepted the data correctly.
6) Set the size of the file to 3GB using RFile64::SetSize()
7) Check that the plugin has intercepted the data correctly.
8) Get the size of the file using RFile64::Size()
9) Check that the plugin has intercepted the data correctly.
10) Set the size of the file to 4GB - 1 using RFile64::SetSize()
11) Check that the plugin has intercepted the data correctly.
12) Get the size of the file using RFile64::Size()
13) Check that the plugin has intercepted the data correctly.
14) Set the size of the file to 4GB using RFile64::SetSize()
15) Check that the plugin has intercepted the data correctly.
16) Get the size of the file using RFile64::Size()
17) Check that the plugin has intercepted the data correctly.
@SYMTestExpectedResults
1) KErrNone.
2) KErrNone.
3) Data has been intercepted correctly.
4) KErrNone.
5) Data has been intercepted correctly.
6) KErrNone.
7) Data has been intercepted correctly.
8) KErrNone.
9) Data has been intercepted correctly.
10) KErrNone.
11) Data has been intercepted correctly.
12) KErrNone.
13) Data has been intercepted correctly.
14) KErrNotSupported.
15) Data has been intercepted correctly.
16) KErrNone.
17) Data has been intercepted correctly.
@SYMTestStatus      Implemented
*/

void TestPluginFileSetSize()
	
	{
	test.Next(_L("RFile64::SetSize() and RFile64::Size() test for plugin\n"));
	RFile64 file;
	TFullName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(_L(":\\F32-TST\\plugintestfile.txt"));
	
	test.Next(_L("Create a file and set its size to 4GB-1.\n"));
	TInt r = file.Replace(TheFs, fileName, EFileWrite|EFileRead);
	test(KErrNone == r);

	test.Next(_L("Set the size of the file to 2GB using RFile64::SetSize()\n"));
	TInt64 sizeToTest = K2GB;
	r = file.SetSize(sizeToTest);
	test(KErrNone == r);

	test.Next(_L("Check that the plugin has intercepted the data correctly.\n"));	
	TestPluginData(KGetSize,sizeToTest);
	
	test.Next(_L("Get the size of the file using RFile64::Size()\n"));
	TInt64 size;
	r = file.Size(size);
	test(KErrNone == r);
	test(sizeToTest == size);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly.\n"));	
	TestPluginData(KGetSize,size);
	
	test.Next(_L("Set the size of the file to 3GB using RFile64::SetSize()\n"));
	sizeToTest = K3GB;
	r = file.SetSize(sizeToTest);
	test(KErrNone == r);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly.\n"));	
	TestPluginData(KGetSize,sizeToTest);
	
	test.Next(_L("Get the size of the file using RFile64::Size()\n"));
	r = file.Size(size);
	test(KErrNone == r);
	test(sizeToTest == size);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly.\n"));	
	TestPluginData(KGetSize,size);
	
	test.Next(_L("Set the size of the file to 4GB-1 using RFile64::SetSize()\n"));
	sizeToTest = K4GBMinusOne;
	r = file.SetSize(sizeToTest);
	test(KErrNone == r);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly.\n"));	
	TestPluginData(KGetSize,sizeToTest);
	
	test.Next(_L("Get the size of the file using RFile64::Size()\n"));
	r = file.Size(size);
	test(KErrNone == r);
	test(sizeToTest == size);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly.\n"));	
	TestPluginData(KGetSize,size);
	
	test.Next(_L("Set the size of the file to 4GB using RFile64::SetSize()\n"));	
	sizeToTest = K4GB;
	r = file.SetSize(sizeToTest);
	
    const TBool bHugeFilesSupported = (r == KErrNone); //-- files >= 4GB are supported
    
    test(KErrNotSupported == r || r == KErrNone);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly.\n"));	
	TestPluginData(KGetSize,sizeToTest);
	
	test.Next(_L("Get the size of the file using RFile64::Size()\n"));
	r = file.Size(size);
	test(KErrNone == r);
	
    if(bHugeFilesSupported)
        test(K4GB == size);
    else
        test(K4GBMinusOne == size);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly.\n"));	
	TestPluginData(KGetSize,size);
	
	
	file.Close();
	r = TheFs.Delete(fileName);
	test(r == KErrNone);
	
	
	}
	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT_PLUGIN-2359
@SYMTestPriority    High
@SYMTestRequirement REQ9542 
@SYMTestType        CIT
@SYMTestCaseDesc    Test that the 64-bit position and length
					in RFile64::Lock() is correctly intercepted 
					by the plugin.
@SYMTestActions     
1) Create a file.
2) Lock the file with position = 0 and length = 100.
3) Check that the plugin has intercepted the position and length correctly.
4) Lock the file with position = 2GB and length = 100.
5) Check that the plugin has intercepted the position and length correctly.
6) Lock the file with position = 4GB and length = 2GB.
7) Check that the plugin has intercepted the position and length correctly.
8) Lock the file with position = 4GB-1 and length = 2GB.
9) Check that the plugin has intercepted the position and length correctly.
@SYMTestExpectedResults
1) KErrNone.
2) KErrNone.
3) Data has been intercepted correctly.
4) KErrNone.
5) Data has been intercepted correctly.
6) KErrNone.
7) Data has been intercepted correctly.
8) KErrLocked.
9) Data has been intercepted correctly.
@SYMTestStatus      Implemented
*/

void TestPluginFileLock()
	{
	test.Next(_L("RFile64::Lock() test for plugin\n"));
	RFile64 file;
	TFullName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(_L(":\\F32-TST\\plugintestfile.txt"));
	
	test.Next(_L("Create a file\n"));
	TInt r = file.Replace(TheFs, fileName, EFileWrite|EFileRead);
	test(KErrNone == r);

	test.Next(_L("Lock the file with position = 0 and length = 100.\n"));
	r = file.Lock(0,100);
	test(KErrNone == r);
	
	test.Next(_L("Check that the plugin has intercepted the position and length correctly\n"));
	TestPluginData(KGetPosition,0);
	TestPluginData(KGetLength,100);
	
	test.Next(_L("Lock the file with position = 2GB and length = 100.\n"));
	r = file.Lock(K2GB,100);
	test(KErrNone == r);

	test.Next(_L("Check that the plugin has intercepted the position and length correctly\n"));	
	TestPluginData(KGetPosition,K2GB);
	TestPluginData(KGetLength,100);
	
	//ToDo: uncomment when defect is fixed
	//test.Next(_L("Lock the file with position = 4GB and length = 2GB.\n"));
	//r = file.Lock(K4GB,K2GB);
	//test(KErrNone == r);
	
	////ToDo: uncomment when defect is fixed
	//test.Next(_L("Check that the plugin has intercepted the position and length correctly\n"));
	//TestPluginData(KGetPosition,K4GB);
	//TestPluginData(KGetLength,K2GB);
	
	//ToDo: uncomment when defect is fixed
	//test.Next(_L("Lock the file with position = 4GB-1 and length = 2GB.\n"));
	//r = file.Lock(K4GBMinusOne,K2GB);
	//test(KErrLocked == r);
	
	//ToDo: uncomment when defect is fixed
	//test.Next(_L("Check that the plugin has intercepted the position and length correctly\n"));
	//TestPluginData(KGetPosition,K4GBMinusOne);
	//TestPluginData(KGetLength,K2GB);
	
	
	file.Close();
	
	r = TheFs.Delete(fileName);
	test(r == KErrNone);
	
	
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT_PLUGIN-2360
@SYMTestPriority    High
@SYMTestRequirement REQ9542 
@SYMTestType        CIT
@SYMTestCaseDesc    Test that the 64-bit position and length
					in RFile64::UnLock() is correctly intercepted 
					by the plugin.
@SYMTestActions     
1) Create a file.
2) UnLock the file with position = 0 and length = 100.
3) Check that the plugin has intercepted the position and length correctly.
4) UnLock the file with position = 2GB and length = 100.
5) Check that the plugin has intercepted the position and length correctly.
6) UnLock the file with position = 4GB and length = 2GB.
7) Check that the plugin has intercepted the position and length correctly.
8) UnLock the file with position = 4GB-1 and length = 2GB.
9) Check that the plugin has intercepted the position and length correctly.
@SYMTestExpectedResults
1) KErrNone.
2) KErrNotFound.
3) Data has been intercepted correctly.
4) KErrNotFound.
5) Data has been intercepted correctly.
6) KErrNotFound.
7) Data has been intercepted correctly.
8) KErrNotFound.
9) Data has been intercepted correctly.
@SYMTestStatus      Implemented
*/


void TestPluginFileUnLock()
	{
	test.Next(_L("RFile64::UnLock() test for plugin\n"));
	RFile64 file;
	TFullName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(_L(":\\F32-TST\\plugintestfile.txt"));
	
	test.Next(_L("Create a file\n"));
	TInt r = file.Replace(TheFs, fileName, EFileWrite|EFileRead);
	test(KErrNone == r);

	test.Next(_L("UnLock the file with position = 0 and length = 100.\n"));
	r = file.UnLock(0,100);
	test(KErrNotFound == r);
	
	test.Next(_L("Check that the plugin has intercepted the position and length correctly\n"));
	TestPluginData(KGetPosition,0);
	TestPluginData(KGetLength,100);

	test.Next(_L("UnLock the file with position = 2GB and length = 100.\n"));	
	r = file.UnLock(K2GB,100);
	test(KErrNotFound == r);
	
	test.Next(_L("Check that the plugin has intercepted the position and length correctly\n"));
	TestPluginData(KGetPosition,K2GB);
	TestPluginData(KGetLength,100);
	
	//ToDo: uncomment when defect is fixed
	//test.Next(_L("UnLock the file with position = 4GB and length = 2GB.\n"));
	//r = file.UnLock(K4GB,K2GB);
	//test(KErrNotFound == r);
	
	//ToDo: uncomment when defect is fixed
	//test.Next(_L("Check that the plugin has intercepted the position and length correctly\n"));
	//TestPluginData(KGetPosition,K4GB);
	//TestPluginData(KGetLength,K2GB);
	
	//ToDo: uncomment when defect is fixed
	//test.Next(_L("UnLock the file with position = 4GB-1 and length = 2GB.\n"));
	//r = file.UnLock(K4GBMinusOne,K2GB);
	//test(KErrNotFound == r);
	
	//ToDo: uncomment when defect is fixed
	//test.Next(_L("Check that the plugin has intercepted the position and length correctly\n"));
	//TestPluginData(KGetPosition,K4GBMinusOne);
	//TestPluginData(KGetLength,K2GB);
	
	file.Close();
	
	r = TheFs.Delete(fileName);
	test(r == KErrNone);
	
	
	}

/**
@SYMTestCaseID      PBASE-T_FILE64BIT_PLUGIN-2361
@SYMTestPriority    High
@SYMTestRequirement REQ9542 
@SYMTestType        CIT
@SYMTestCaseDesc    Test that the 64-bit position in RFile64::Seek()
					is correctly intercepted by the plugin.
@SYMTestActions     
1) Create a file and set its size to 4GB-1.
2) Seek to the end of the file
3) Check that the plugin has intercepted the initial and the final position correctly.
4) Get the current position using seekMode = ESeekCurrent and pos =0
5) Check that the plugin has intercepted the initial and the final position correctly.
6) Seek to the position 2GB
7) Check that the plugin has intercepted the initial and the final position correctly.
8) Seek to the position 4GB-1
9) Check that the plugin has intercepted the initial and the final position correctly.

@SYMTestExpectedResults
1) KErrNone.
2) KErrNone.
3) Data has been intercepted correctly.
4) KErrNone.
5) Data has been intercepted correctly.
6) KErrNone.
7) Data has been intercepted correctly.
8) KErrNone.
9) Data has been intercepted correctly.
@SYMTestStatus      Implemented
*/

void TestPluginFileSeek()
	{
	test.Next(_L("RFile64::Seek() test for plugin\n"));
	RFile64 file;
	TFullName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(_L(":\\F32-TST\\plugintestfile.txt"));

	test.Next(_L("Create a file and set its size to 4GB-1\n"));	
	TInt r = file.Replace(TheFs, fileName, EFileWrite|EFileRead);
	test(KErrNone == r);

	r = file.SetSize(K4GBMinusOne);
	test(KErrNone == r);
	
	test.Next(_L("Seek to the end of the file\n"));	
	TInt64 position = 0;
	r = file.Seek(ESeekEnd,position);
	test(KErrNone == r);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly\n"));
	TestPluginData(KGetPosition,0);
	TestPluginData(KGetNewPosition,K4GBMinusOne);
	
	test.Next(_L("Get the current position using seekMode = ESeekCurrent and pos =0\n"));	
	position = 0;
	r = file.Seek(ESeekCurrent,position);
	test(KErrNone == r);

	test.Next(_L("Check that the plugin has intercepted the data correctly\n"));	
	TestPluginData(KGetPosition,0);
	TestPluginData(KGetNewPosition,K4GBMinusOne);
	
	
	test.Next(_L("Seek to the position 2GB\n"));
	position = K2GB;
	r = file.Seek(ESeekStart,position);
	test(KErrNone == r);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly\n"));
	TestPluginData(KGetPosition,K2GB);
	TestPluginData(KGetNewPosition,K2GB);
	
	test.Next(_L("Seek to the position 4GB-1\n"));
	position = K4GBMinusOne;
	r = file.Seek(ESeekStart,position);
	test(KErrNone == r);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly\n"));
	TestPluginData(KGetPosition,K4GBMinusOne);
	TestPluginData(KGetNewPosition,K4GBMinusOne);
	
	
	file.Close();
	
	r = TheFs.Delete(fileName);
	test(r == KErrNone);
	
	
	}
	
/**
@SYMTestCaseID      PBASE-T_FILE64BIT_PLUGIN-2362
@SYMTestPriority    High
@SYMTestRequirement REQ9542 
@SYMTestType        CIT
@SYMTestCaseDesc    Test that the 64-bit position in RFs::ReadFileSection()
					is correctly intercepted by the plugin.
@SYMTestActions     
1) Create a file and set the size of the file to 4GB-1.
2) Read from the file using RFs::ReadFileSection() using pos = 0
3) Check that the plugin has intercepted the data correctly.
4) Read from the file using RFs::ReadFileSection() using pos = 2GB
5) Check that the plugin has intercepted the data correctly.
6) Read from the file using RFs::ReadFileSection() using pos = 3GB
7) Check that the plugin has intercepted the data correctly.
8) Read from the file using RFs::ReadFileSection() using pos = 4GB-1
9) Check that the plugin has intercepted the data correctly.
10) Read from the file using RFs::ReadFileSection() using pos = 4GB
11) Check that the plugin has intercepted the data correctly.
@SYMTestExpectedResults
1) KErrNone.
2) KErrNone.
3) Data has been intercepted correctly.
4) KErrNone.
5) Data has been intercepted correctly.
6) KErrNone.
7) Data has been intercepted correctly.
8) KErrNone.
9) Data has been intercepted correctly.
10) KErrNone.
11) Data has been intercepted correctly.
@SYMTestStatus      Implemented
*/

void TestPluginFileReadFileSection()
	{
	test.Next(_L("RFs::ReadFileSection() test for plugin\n"));
	RFile64 file;
	TFullName fileName;
	fileName.Append(gDriveToTest);
	fileName.Append(_L(":\\F32-TST\\plugintestfile.txt"));
	
	test.Next(_L("Create a file and set its size to 4GB-1\n"));	
	TInt r = file.Replace(TheFs, fileName, EFileWrite|EFileRead);
	test(KErrNone == r);
	
	r = file.SetSize(K4GBMinusOne);
	test(KErrNone == r);

	test.Next(_L("Read from the file using RFs::ReadFileSection() using pos = 0\n"));	
	TBuf8<256> rFsBuf;
	r = TheFs.ReadFileSection(fileName,0,rFsBuf,1);
	test(r == KErrNone);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly\n"));
	TestPluginData(KGetPosition,0);

	test.Next(_L("	Read from the file using RFs::ReadFileSection() using pos = 2GB\n"));		
	r = TheFs.ReadFileSection(fileName,K2GB,rFsBuf,1);
	test(r == KErrNone);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly\n"));
	TestPluginData(KGetPosition,K2GB);
	
	test.Next(_L("	Read from the file using RFs::ReadFileSection() using pos = 3GB\n"));	
	r = TheFs.ReadFileSection(fileName,K3GB,rFsBuf,1);
	test(r == KErrNone);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly\n"));
	TestPluginData(KGetPosition,K3GB);
	
	test.Next(_L("	Read from the file using RFs::ReadFileSection() using pos = 4GB-1\n"));	
	r = TheFs.ReadFileSection(fileName,K4GBMinusOne,rFsBuf,1);
	test(r == KErrNone);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly\n"));
	TestPluginData(KGetPosition,K4GBMinusOne);
	
	test.Next(_L("	Read from the file using RFs::ReadFileSection() using pos = 4GB\n"));	
	r = TheFs.ReadFileSection(fileName,K4GB,rFsBuf,1);
	test(r == KErrNone);
	
	test.Next(_L("Check that the plugin has intercepted the data correctly\n"));
	TestPluginData(KGetPosition,K4GB);
	
	
	file.Close();
	
	r = TheFs.Delete(fileName);
	test(r == KErrNone);
	
	
	}
	

void CallTestsL()
	{
	TInt theDrive;
	TInt r = TheFs.CharToDrive(gDriveToTest,theDrive);
	test(r == KErrNone);
	TVolumeInfo volInfo;
	r = TheFs.Volume(volInfo, theDrive);
	if(KErrNone != r)
		{
		test.Printf(_L("\nError in getting drive volume information!! Error code is %d"),r);
		test(EFalse);
		}
	test.Printf(_L("\nDrive volume size is %15ld\n"), volInfo.iSize);

	
	if (volInfo.iFree <= K4GB)
		{
		test.Printf(_L("\nSkipping test: test requires disk with capacity more than 2 GB"));
		return;
		}

	test.Next(_L("Loading the plugin"));
	r = TheFs.AddPlugin(KFile64BitPluginFileName);
	if (r == KErrAlreadyExists) r = KErrNone;
	
	r = TheFs.MountPlugin(KFile64BitPluginName, (TUint)gDriveToTest.GetUpperCase() - 65);
	if (r == KErrNotSupported)
		{
		test.Printf(_L("Plugins are not supported on pagable drives.\nSkipping test.\n"));
		r = TheFs.RemovePlugin(KFile64BitPluginName);
		return;
		}
	
	TestPluginFileRead();
	TestPluginFileWrite();
	TestPluginFileSetSize();
	TestPluginFileLock();
	TestPluginFileUnLock();
	TestPluginFileSeek();
	TestPluginFileReadFileSection();
	
	test.Next(_L("Un-Loading the plugin"));
	r = TheFs.DismountPlugin(KFile64BitPluginName);
	
	r = TheFs.RemovePlugin(KFile64BitPluginName);

	}

// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_regram.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>

//
// This test takes forever to run and destroys the internal Ram drive.
//

LOCAL_D RTest test(_L("T_REGRAM"));

void deleteReg()
//
// Delete everything from the registry
//
	{
	TUid uid;
	TRegistryIter riter;
	riter.Reset();
	while (riter.Next(uid)==KErrNone)
		{
		TRegistryCategory cat(uid);
		cat.DeleteAllItems();
		}
	}

TInt fillReg(const TDesC8 &aDes, TInt aEntries)
//
// Fill the registry with big nasty monsters
// returns the number of entries made.
//
	{
	TBuf8<0x100> buf=_L8("");
	TInt i;
	for (i=0; i<8; i++)
		buf.Append(_L8("Big_ nasty_ monster_ chasing_ me"));
	
	TUid uid;
	uid=TUid::Uid(0x12345678);
	TRegistryCategory cat(uid);
	i=0;
	TBuf8<0x20> item;
	while (i<aEntries)
		{
		item.Format(_L8("%S %08x"),&aDes,i);
		TInt r=cat.SetItem(item, buf);
		if (r!=KErrNone)
			break;
		i++;
		}
	return i;
	}

TInt shrinkReg(const TDesC8 &aDes, TInt aEntries)
//
// Delete aEntries entries from the registry
// returns number of entries deleted.
//
	{


	TUid uid;
	uid=TUid::Uid(0x12345678);
	TRegistryCategory cat(uid);
	TBuf8<0x20> item;
	TInt i=aEntries-1;
	while (i>=0)
		{
		item.Format(_L8("%S %08x"),&aDes,i);
		TInt r=cat.DeleteItem(item);
		if (r!=KErrNone)
			break;
		i--;
		}
	return aEntries-i-1;
	}

TInt testReg(const TDesC8 &aDes, TInt aEntries)
//
// Test the first aEntries entries are set correctly
//
	{
	TBuf8<0x100> buf=_L8("");
	TInt i;
	for (i=0; i<8; i++)
		buf.Append(_L8("Big_ nasty_ monster_ chasing_ me"));
	
	TUid uid;
	uid=TUid::Uid(0x12345678);
	TRegistryCategory cat(uid);

	TBuf8<0x20> item;
	TBuf8<0x100> res;
	i=0;
	while (i<aEntries)
		{
		item.Format(_L8("%S %08x"),&aDes,i);
		if ((i&0x1f)==0)
			test.Printf(_L("."));
		res=cat.Item(item, _L8("Gone gone gone"));
		test(buf==res);
		i++;
		}
	test.Printf(_L("\n"));
	return i;
	}

void test1()
//
// Test growing and shrinking the registry
//
	{

	test.Start(_L("Cleanout the registry"));
	deleteReg();

	test.Next(_L("Grow the registry a bit"));
	TInt n=fillReg(_L8("Run Away"),40);
	test.Printf(_L("Made %d entries\n"), n);
	test.Next(_L("Test the content of the registry"));
	TInt m=testReg(_L8("Run Away"),n);
	test(n==m);
	test.Next(_L("Shrink it a bit"));
	m=shrinkReg(_L8("Run Away"),n);
	test.Printf(_L("Deleted %d entries\n"), m);
	test(n==m);

	test.Next(_L("Fill the registry with guff"));
	test.Printf(_L("\tTHIS TEST TAKES AGES\n"));
	n=fillReg(_L8("Run Away"),0x7fffffff);
	test.Printf(_L("Made %d entries\n"), n);
	test.Next(_L("Test the content of the registry"));
	m=testReg(_L8("Run Away"),n);
	test.Next(_L("Shrink the registry"));
	m=shrinkReg(_L8("Run Away"),n);
	test.Printf(_L("Deleted %d entries\n"), m);
	test(n==m);

	test.Next(_L("Grow a bigish registry"));
	n=fillReg(_L8("Lunge"),40);
	test.Next(_L("Save the registry"));
    TUint8* bigPtr=new TUint8[0x8000];
	TPtr8 big(bigPtr, 0x8000);
	TInt bigsize;
	TInt r=User::MachineConfiguration(big,bigsize);
	test(r==KErrNone);

	test.Next(_L("Clear it"));
	deleteReg();

	test.Next(_L("Grow a little registry"));
	m=fillReg(_L8("Groat"), 10);
	test.Next(_L("Save it"));
    TUint8* littlePtr=new TUint8[0x8000];
	TPtr8 little(littlePtr, 0x8000);
	TInt littlesize;
	r=User::MachineConfiguration(little,littlesize);
	test(r==KErrNone);

	test.Next(_L("Set to big registry"));
	r=User::SetMachineConfiguration(big);
	test(r==KErrNone);
	test.Next(_L("Test it"));
	TInt i=testReg(_L8("Lunge"), n);
	test(n==i);

	test.Next(_L("Set to little registry"));
	r=User::SetMachineConfiguration(little);
	test(r==KErrNone);
	test.Next(_L("Test it"));
	i=testReg(_L8("Groat"), m);
	test(i==m);
	
	test.Next(_L("Set to big registry again"));
	deleteReg();
	r=User::SetMachineConfiguration(big);
	test(r==KErrNone);
	test.Next(_L("Test it"));
	i=testReg(_L8("Lunge"), n);
	test(n==i);

	delete bigPtr;
	delete littlePtr;
	test.End();
	}

void fillRamDrive(TBusLocalDrive &aRamDrive)
//
// Fill the ram drive with data
//
	{

	TLocalDriveCapsV2 info;
	TPckg<TLocalDriveCapsV2> infoPckg(info);
	test(aRamDrive.Caps(infoPckg)==KErrNone);
	TInt size=info.iSize.Low();
	TInt i;
	TPckgBuf<TInt> dataBuf;
	TInt &data=dataBuf();
	for (i=0; i<size; i+=4)
		{
		data=i;
		aRamDrive.Write(i, dataBuf);
		}
	}

void testRamDrive(TBusLocalDrive &aRamDrive)
//
// Test the data is still OK
//
	{

	TLocalDriveCapsV2 info;
	TPckg<TLocalDriveCapsV2> infoPckg(info);
	test(aRamDrive.Caps(infoPckg)==KErrNone);
	TInt size=info.iSize.Low();
	TInt i;
	TPckgBuf<TInt> dataBuf;
	TInt &data=dataBuf();
	for (i=0; i<size; i+=4)
		{
		aRamDrive.Read(i, sizeof(TInt), dataBuf);
		if (i&0x3ff==0)
			test.Printf(_L("."));
	//	test.Printf(_L("%08x "), data);
		test(data==i);
		}
	test.Printf(_L("\n"));
	}

void testGrow()
	{

	test.Start(_L("Grow the registry a bit"));
	TInt n=fillReg(_L8("Run Away"),20);
	test(n==20);
	test.Next(_L("Test the content of the registry"));
	testReg(_L8("Run Away"),n);
	test(n==20);
	test.End();
	}

void setRamDriveSize(TBusLocalDrive aRamDrive, TInt aSize)
	{

	TLocalDriveCapsV2 info;
	TPckg<TLocalDriveCapsV2> infoPckg(info);
	test(aRamDrive.Caps(infoPckg)==KErrNone);
	TInt oldsize=info.iSize.Low();
	if (aSize>oldsize)
		test(aRamDrive.Enlarge(aSize-oldsize)==KErrNone);
	else
		test(aRamDrive.ReduceSize(0,oldsize-aSize)==KErrNone);
	}

GLDEF_C TInt E32Main(void)
//
// Test the Ram Drive and the Registry
//
	{

	test.Title();

	test.Start(_L("Connect to the Local Drive"));
	TBool changedFlag;
	TBusLocalDrive ramDrive;
	TInt r=ramDrive.Connect(0,changedFlag);
	test(r==KErrNone);

	test.Next(_L("Test testing the ram drive"));
	fillRamDrive(ramDrive);
	testRamDrive(ramDrive);

	test.Next(_L("Simple grow test"));
	fillRamDrive(ramDrive);
	testGrow();
	testRamDrive(ramDrive);	
	test.Next(_L("Shrink"));
	deleteReg();
	testRamDrive(ramDrive);

	{
	deleteReg();
	test.Next(_L("Grow the registry a bit"));
	TInt n=fillReg(_L8("Run Away"),40);
	test.Next(_L("Test the content of the registry"));
	TInt m=testReg(_L8("Run Away"),n);
	test(n==m);
	test.Next(_L("Shrink it a bit"));
	m=shrinkReg(_L8("Run Away"),n);
	test(n==m);
	test.Next(_L("test ram drive"));
	testRamDrive(ramDrive);
	}

/*	test.Next(_L("Run the tests with the current ram drive size"));
	fillRamDrive(ramDrive);
	test1();
	testRamDrive(ramDrive);*/

	test.Next(_L("Run the tests with no Ram Drive"));
	setRamDriveSize(ramDrive, 0);
	test1();

	test.Next(_L("Run the tests with small ram drive"));
	setRamDriveSize(ramDrive, 0x3000);
	fillRamDrive(ramDrive);
	test1();
	testRamDrive(ramDrive);

	test.Next(_L("Clear the registry"));
	deleteReg();

/*	test.Next(_L("Run the tests with a big ram drive"));
	r=KErrGeneral;
	while (r==KErrNone)
		r=ramDrive.Enlarge(0x1000);
	test.Printf(_L("%d"), r);
//	test(r==KErrDiskFull);
	r=ramDrive.ReduceSize(0, 0x2000);
	test(r==KErrNone);
	fillRamDrive(ramDrive);
	test1();
	testRamDrive(ramDrive);*/

	ramDrive.Disconnect();

	test.End();
	return(KErrNone);
	}


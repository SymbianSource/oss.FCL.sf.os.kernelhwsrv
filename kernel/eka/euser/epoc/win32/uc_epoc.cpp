// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\epoc\win32\uc_epoc.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32wins.h>

//#include <e32cmn.h>
#include <nwdl.h>

#if defined __SYMC__

//SL: Empty on FCL ?
//For now we use this for basic testing on our SYMC implementation


class CBaseTest: public CBase
	{

	};


GLDEF_C void MainL()
	{
	
	CBase* other=new(ELeave) CBase();
	CleanupStack::PushL(other);
	CBase* base=new(ELeave) CBase();
	CleanupStack::PushL(base);
	CleanupStack::PopAndDestroy(2,other);
	//delete base;
	
	//Testing cleanup stack
	TRAPD(err,
	base=new(ELeave) CBase();
	CleanupStack::PushL(base);
	User::Leave(KErrCancel);
	);

	ASSERT(err==KErrCancel);

	//Testing alloc failure
	TRAP(err,
	TUint8* test=new(ELeave) TUint8[1024*1024*10];
	delete[] test;
	);

	ASSERT(err==KErrNoMemory);
	
	//Testing unbalanced cleanup stack
	//base=new(ELeave) CBase();
	//CleanupStack::PushL(base);
	}


GLDEF_C TInt E32Main()
	{
	//What do we do then
	
	__UHEAP_MARK;

	//CBase* base=new(ELeave) CBase();
	CBase* base=new CBase();
	delete base;

	CBaseTest* baseTest=new CBaseTest();
	delete baseTest;

	HBufC* buf=HBufC::New(10);
	delete buf;

	CArrayFix<TInt>* active=new CArrayFixFlat<TInt>(10);
	delete active;

	TUint8* test=new TUint8[1024*9];
	delete[] test;

	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if (!cleanupStack)
		{
		return KErrNoMemory;
		}

	TInt err=KErrNone;
	TRAP(err,MainL());

	delete cleanupStack;

	__UHEAP_MARKEND;

	return err;
	}


TInt main()
	{
	User::InitProcess();
	//BootEpoc(ETrue);
	E32Main();

	User::Exit(0);
	return 0;
	}

#endif


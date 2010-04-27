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


GLDEF_C void MainL()
	{

	CBase* base=new(ELeave) CBase();
	delete base;
	
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

	}


GLDEF_C TInt E32Main()
	{
	//What do we do then
	
	__UHEAP_MARK;

	//CBase* base=new(ELeave) CBase();
	CBase* base=new CBase();
	delete base;

	TUint8* test=new TUint8[1024*9];
	delete[] test;

	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if (!cleanupStack)
		{
		return KErrNoMemory;
		}

	TRAPD(err,MainL());

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


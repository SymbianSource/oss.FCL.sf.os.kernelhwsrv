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


GLDEF_C TInt E32Main()
	{
	//What do we do then
	
	__UHEAP_MARK;

	//CBase* base=new(ELeave) CBase();
	CBase* base=new CBase();
	delete base;

	TUint8* test=new TUint8[1024*9];
	delete[] test;

	__UHEAP_MARKEND;

	return KErrNone;
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


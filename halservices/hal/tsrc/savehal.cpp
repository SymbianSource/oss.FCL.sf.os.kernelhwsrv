// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// hal\tsrc\savehal.cpp
// 
//

#include <e32test.h>
#include <f32file.h>
#include <hal.h>

_LIT(KHalDataFileName, "\\system\\data\\HAL.DAT");

RTest test(_L("SAVEHAL"));

const TInt KHalProperties=HAL::EEntryDynamic|HAL::EEntryValid;

TInt E32Main()
	{
	test.Title();

	test.Start(_L("Get HAL data"));
	HAL::SEntry* pE;
	TInt nEntries;
	TInt r=HAL::GetAll(nEntries, pE);
	test(r==KErrNone);
	test.Printf(_L("%d entries\n"),nEntries);

	const HAL::SEntry* pS=pE;
	const HAL::SEntry* pEnd=pS+nEntries;
	TInt* pD=(TInt*)pE;
	TInt s=0;
	for (; pS<pEnd; ++pS, ++s)
		{
		if ((pS->iProperties & KHalProperties)==KHalProperties)
			{
			TInt v=pS->iValue;
			*pD++=s;
			*pD++=v;
			}
		}
	TInt nSaved=(pD-(TInt*)pE)>>1;
	test.Printf(_L("%d entries to be saved\n"),nSaved);

	test.Next(_L("Connect to file server"));
	RFs fs;
	r=fs.Connect();
	test(r==KErrNone);

	test.Next(_L("Open file"));
	RFile file;
	r=file.Replace(fs,KHalDataFileName,EFileWrite|EFileShareExclusive);
	test(r==KErrNone);

	test.Next(_L("Save data"));
	TPckgBuf<TInt> muid;
	r=HAL::Get(HAL::EMachineUid, muid());
	test (r==KErrNone);
	r=file.Write(muid);
	test(r==KErrNone);
	TPtrC8 ptr((const TUint8*)pE, nSaved*8);
	r=file.Write(ptr);
	test(r==KErrNone);

	file.Close();
	fs.Close();
	User::Free(pE);

	test.End();

	return 0;
	}

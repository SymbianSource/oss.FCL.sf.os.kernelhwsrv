// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\epoc\arm\uc_data.cpp
// 
//

#include "us_data.h"

#ifdef __USERSIDE_THREAD_DATA__

TAny* TLocalThreadData::DllTls(TInt aHandle, TInt aDllUid)
	{
	if (!iTlsHeap)
		return NULL;
	STls tls;
	tls.iHandle=aHandle;
	TInt r=iTls.FindInSignedKeyOrder(tls);
	if (r>=0 && iTls[r].iDllUid==aDllUid)
		return iTls[r].iPtr;
	return NULL;
	}

TInt TLocalThreadData::DllSetTls(TInt aHandle, TInt aDllUid, TAny* aPtr)
	{
	if (!iTlsHeap)
		{
		new (&iTls) RArray<STls>(KTlsArrayGranularity, _FOFF(STls,iHandle));
		iHeap->Open();
		iTlsHeap = iHeap;
		}
	
	STls tls;
	tls.iHandle = aHandle;
	tls.iDllUid = aDllUid;
	tls.iPtr = aPtr;
	TInt i;
	TInt r=iTls.FindInSignedKeyOrder(tls,i);
	if (r==KErrNone)
		iTls[i] = tls;
	else
		{
		RAllocator* currentHeap = iHeap;
		iHeap = iTlsHeap;
		r = iTls.Insert(tls,i);
		iHeap = currentHeap;
		}
	return r;
	}

void TLocalThreadData::DllFreeTls(TInt aHandle)
	{
	if (!iTlsHeap)
		return;
	
	STls tls;
	tls.iHandle=aHandle;
	TInt r=iTls.FindInSignedKeyOrder(tls);
	if (r>=0)
		{
		RAllocator* currentHeap = iHeap;
		iHeap = iTlsHeap;
		iTls.Remove(r);
		iTls.Compress();
		iHeap = currentHeap;
		}
	}

void TLocalThreadData::Close()
	{
	RAllocator* currentHeap = iHeap;
	RAllocator* tlsHeap = iTlsHeap;
	if (tlsHeap)
		{
		iHeap = tlsHeap;
		iTls.Close();
		iHeap = currentHeap;
		iTlsHeap = NULL;
		tlsHeap->Close();
		}
	}

#endif

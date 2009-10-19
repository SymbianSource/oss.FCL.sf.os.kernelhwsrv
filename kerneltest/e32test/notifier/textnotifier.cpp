// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\notifier\textnotifier1.cpp
// 
//

#ifndef V2_NOTIFIER
#endif

#include <twintnotifier.h>
#include "textnotifier.h"

#ifdef V2_NOTIFIER

#define KUidTestTextNotifier KUidTestTextNotifier2
#define KUidOtherTestTextNotifier KUidTestTextNotifier1
#define NOTIFIER_BASE MNotifierBase2

#else

#define KUidTestTextNotifier KUidTestTextNotifier1
#define KUidOtherTestTextNotifier KUidTestTextNotifier2
class MEikSrvNotifierBase : public MNotifierBase {};
#define NOTIFIER_BASE MEikSrvNotifierBase

#endif


// Give each notifers a different channel so they don't block each other
// (this is required for the MNotiferManager testing to be valid.)
#define KUidOutputChannel KUidTestTextNotifier


class CTestNotifier : public CBase, public NOTIFIER_BASE
	{
public:
	CTestNotifier(TNotifierPriority aPriority);
	virtual void Release();
	virtual TNotifierInfo RegisterL();
	virtual TNotifierInfo Info() const;
	virtual TPtrC8 StartL(const TDesC8& aBuffer);
#ifdef V2_NOTIFIER
	virtual void StartL(const TDesC8& aBuffer, TInt aReplySlot, const RMessagePtr2& aMessage);
#else
	virtual void StartL(const TDesC8& aBuffer, const TAny* aReturnVal, RMessage aMessage);
#endif
	virtual void Cancel();
	virtual TPtrC8 UpdateL(const TDesC8& aBuffer);
private:
	TBool iCancel;
	TBuf8<256> iResponse;
	TBool iNotifierManagerTested;
	TNotifierPriority iPriority;
	};

void CTestNotifier::Release()
	{
	delete this;
	}

CTestNotifier::CTestNotifier(TNotifierPriority aPriority)
	: iPriority(aPriority)
	{}

CTestNotifier::TNotifierInfo CTestNotifier::RegisterL()
	{
	CTestNotifier::TNotifierInfo info;
	info.iUid = KUidTestTextNotifier;
	info.iChannel = KUidOutputChannel;
	info.iPriority = iPriority;
	return info;
	}


CTestNotifier::TNotifierInfo CTestNotifier::Info() const
	{
	CTestNotifier::TNotifierInfo info;
	info.iUid = KUidTestTextNotifier;
	info.iChannel = KUidOutputChannel;
	info.iPriority = iPriority;
	return info;
	}

TPtrC8 CTestNotifier::StartL(const TDesC8& aBuffer)
	{
	iCancel = EFalse;
	if(aBuffer==KMNotifierManager)
		{
		iNotifierManagerTested = ETrue;
		iResponse.SetMax();
		iResponse.FillZ();
		iResponse.Zero();
		iManager->StartNotifierL(KUidOtherTestTextNotifier,KStartData,iResponse);
		return TPtrC8(iResponse);
		}
	if(aBuffer!=KStartData)
		User::Leave(KErrGeneral);
	return TPtrC8(KResponseData);
	}

#ifdef V2_NOTIFIER
void CTestNotifier::StartL(const TDesC8& aBuffer, TInt aReplySlot, const RMessagePtr2& aMessage)
#else
void CTestNotifier::StartL(const TDesC8& aBuffer, const TAny* aReturnVal, RMessage aMessage)
#endif
	{
	TBool cancelled=iCancel;
	iCancel = EFalse;
	TInt r=KErrNone;
	if(aBuffer==KMNotifierManager || aBuffer==KMNotifierManagerWithCancelCheck)
		{
		iNotifierManagerTested = ETrue;
		iResponse.SetMax();
		iResponse.FillZ();
		iResponse.Zero();
		iManager->StartNotifierL(KUidOtherTestTextNotifier,KStartData,iResponse);
		if(aBuffer==KMNotifierManagerWithCancelCheck)
			r = cancelled ? KTestNotifierWasPreviouselyCanceled : KErrGeneral;
		}
	else if(aBuffer==KStartData)
		iResponse.Copy(KResponseData);
	else if(aBuffer==KHeapData)
		{
		iResponse.Zero();
		TInt allocSize;
		r=User::AllocSize(allocSize); 
		iResponse.Format(_L8("%d"),allocSize);
		}
	else if(aBuffer==KStartWithCancelCheckData)
		{
		iResponse.Copy(KResponseData);
		r = cancelled ? KTestNotifierWasPreviouselyCanceled : KErrGeneral;
		}
	else
		User::Leave(KErrGeneral);
#ifdef V2_NOTIFIER
	aMessage.WriteL(aReplySlot,iResponse);
#else
	aMessage.WriteL(aReturnVal,iResponse);
#endif
	aMessage.Complete(r);
	}

void CTestNotifier::Cancel()
	{
	if(iNotifierManagerTested)
		{
		iNotifierManagerTested = EFalse;
		iManager->CancelNotifier(KUidOtherTestTextNotifier);
		}
	iCancel = ETrue;
	}

TPtrC8 CTestNotifier::UpdateL(const TDesC8& aBuffer)
	{
	if(aBuffer==KMNotifierManager)
		{
		iNotifierManagerTested = ETrue;
		iManager->UpdateNotifierL(KUidOtherTestTextNotifier,KUpdateData,iResponse);
		}
	else if(aBuffer==KUpdateData)
		iResponse.Copy(KResponseData);
	else
		User::Invariant();
	return TPtrC8(iResponse);
	}


EXPORT_C CArrayPtr<NOTIFIER_BASE>* NotifierArray()
	{
    CArrayPtrFlat<NOTIFIER_BASE>* array = new (ELeave) CArrayPtrFlat<NOTIFIER_BASE>(2);
	CleanupStack::PushL(array);
    CTestNotifier* notifier = new (ELeave) CTestNotifier(NOTIFIER_BASE::ENotifierPriorityLow);
    CleanupStack::PushL(notifier);
    array->AppendL(notifier);
    CleanupStack::Pop(2,array);
    return array;
	}


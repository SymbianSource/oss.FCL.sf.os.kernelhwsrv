// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\emi\emitest_dev.h
// 
//

#ifndef __D_EMITEST_DEV_H__
#define __D_EMITEST_DEV_H__

/*
  Logical Device (factory class) for 'EMITest'
*/
class DEMITestFactory : public DLogicalDevice
	{
public:

	DEMITestFactory();
	~DEMITestFactory();
	//  Inherited from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};
 
	
/*
  Logical Channel class for 'EMITest'
*/
class DEMITestChannel : public DLogicalChannelBase
	{
public:

	DEMITestChannel();
	virtual ~DEMITestChannel();
	TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	// Inherited from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);

private:

	enum TPanic
		{
		ERequestFromWrongThread=1,
		ERequestAlreadyPending
		};
	TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	void DoCancel(TUint aMask);
	static void TagMaskDFC(TAny* aPtr);

	DThread* iClient;
	TDfc iTagMaskDFC;
	};

#endif

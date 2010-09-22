// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\digitiser\d_kerneldigitisertest.h
// 
//

#if !defined(__D_KERNELDIGITISERTEST_H__)
#define __D_KERNELDIGITISERTEST_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"D_KERNELDIGITISERTEST.LDD");

class TestTRawDigitiserEvent
	{
public:
	TestTRawDigitiserEvent(TRawEvent::TType aType,TInt aX,TInt aY,TInt aZ,TInt aScanCode,TInt aPhi,TInt aTheta,TInt aAlpha,TUint8 aPointerNumber,TUint8 iTip);
#ifdef __KERNEL_MODE__
	TestTRawDigitiserEvent();
	TInt TestEvents();
#endif
private:	
	TRawEvent::TType iType;
	TInt iX;
    TInt iY;
	TInt iZ;
	TInt iScanCode;
	TInt iPhi;
	TInt iTheta;
	TInt iAlpha;
	TUint8 iPointerNumber;
	TUint8 iTip;
	TRawEvent iDigitiser3DEvent;
	};

class RTestDigitiserLdd : public RBusLogicalChannel
	{
public:

	enum TControl
		{
		EStartTest=1
		};


public:
	inline TInt Open();
	inline TInt StartTest(TestTRawDigitiserEvent &aEventObject);
	};


#ifndef __KERNEL_MODE__
inline TInt RTestDigitiserLdd::Open()
	{
	return DoCreate(KLddName,TVersion(0,1,0),KNullUnit,NULL,NULL);
	}

inline TInt RTestDigitiserLdd::StartTest(TestTRawDigitiserEvent &aEventObject)
	{
    return DoControl(EStartTest,&aEventObject);
	}
#endif //__KERNEL_MODE__

#endif //__D_KERNELDIGITISERTEST_H__

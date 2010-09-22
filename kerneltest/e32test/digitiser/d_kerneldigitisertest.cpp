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
// e32test\digitiser\d_kerneldigitisertest.cpp
// LDD for testing class TRawEvent digitiser kernel side entries
// 
//

#include <kernel/kernel.h>

#include "d_kerneldigitisertest.h"

class DKLDDFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	DKLDDFactory();
	virtual TInt Install(); 								//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;				//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

class DKLDDChannel : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DKLDDChannel();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	};


DECLARE_STANDARD_LDD()
	{
	return new DKLDDFactory;
	}

//
// Constructor
//
DKLDDFactory::DKLDDFactory()
	{
	}

TInt DKLDDFactory::Create(DLogicalChannelBase*& aChannel)
	{
//
// Create new channel
//  
	aChannel=new DKLDDChannel;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DKLDDFactory::Install()
//
// Install the LDD - overriding pure virtual
	{
	return SetName(&KLddName);
	}

void DKLDDFactory::GetCaps(TDes8& /*aDes*/) const
//
// Get capabilities - overriding pure virtual
//
	{
	}

TInt DKLDDChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create channel
//
	{
	return KErrNone;
	}

DKLDDChannel::~DKLDDChannel()
//
// Destructor
//
	{
	}

TInt DKLDDChannel::Request(TInt aReqNo, TAny* a1, TAny* /*a2*/)
	{
	TInt r=KErrNone;
	TestTRawDigitiserEvent theEventObj;

	switch(aReqNo)
		{
		case RTestDigitiserLdd::EStartTest:
			kumemget(&theEventObj,a1,sizeof(TestTRawDigitiserEvent));
			theEventObj.TestEvents();
			break;

		default:
			r=KErrNotSupported;
			break;
		} 

	return r;
	}


//
// class TestTRawDigitiserEvent kernel side implementations
//

TestTRawDigitiserEvent::TestTRawDigitiserEvent()
	{}

TestTRawDigitiserEvent::TestTRawDigitiserEvent(TRawEvent::TType aType,TInt aX,TInt aY,TInt aZ,TInt aScanCode,TInt aPhi,TInt aTheta,TInt aAlpha,TUint8 aPointerNumber,TUint8 aTip)
:iType(aType),iX(aX),iY(aY),iZ(aZ),iScanCode(aScanCode),iPhi(aPhi),iTheta(aTheta),iAlpha(aAlpha),iPointerNumber(aPointerNumber),iTip(aTip)
	{}

TInt TestTRawDigitiserEvent::TestEvents()
	{
	if(!(iDigitiser3DEvent.Type()==0))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	iDigitiser3DEvent.Set(iType);
	if(!(iDigitiser3DEvent.Type()==iType))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	iDigitiser3DEvent.SetPointerNumber(iPointerNumber);
	if(!(iPointerNumber == iDigitiser3DEvent.PointerNumber()))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	iDigitiser3DEvent.Set(iType,iScanCode);
	//Set the Type temporarily to get through the assertion 
	iDigitiser3DEvent.Set(TRawEvent::EKeyDown);
    if(!(iScanCode==iDigitiser3DEvent.ScanCode()))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	iDigitiser3DEvent.Set(iType,iX,iY);
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointerMove);
	if(!( (iX==iDigitiser3DEvent.Pos().iX) && (iY==iDigitiser3DEvent.Pos().iY) ))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	iDigitiser3DEvent.Set(iType,iX,iY,iZ);
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointerMove);
	if(!((iX==iDigitiser3DEvent.Pos3D().iX) && (iY==iDigitiser3DEvent.Pos3D().iY) && (iZ==iDigitiser3DEvent.Pos3D().iZ) ))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	iDigitiser3DEvent.SetTip(iTip);
	if(!(TBool(iTip) == iDigitiser3DEvent.IsTip()))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	iDigitiser3DEvent.SetTilt(iType,iPhi,iTheta);
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointer3DTilt);
	TAngle3D rawEventAnge3D=iDigitiser3DEvent.Tilt();
	if(!((rawEventAnge3D.iPhi==iPhi) && (rawEventAnge3D.iTheta==iTheta)))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	iDigitiser3DEvent.SetRotation(iType,iAlpha);
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointer3DRotation);
	if(!(iAlpha == iDigitiser3DEvent.Rotation()))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	iDigitiser3DEvent.Set(iType,iX+1,iY+1,iZ+1,iPhi+1,iTheta+1,iAlpha+1);
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointer3DTiltAndMove);
	if(!( ((iX+1)==iDigitiser3DEvent.Pos3D().iX) && ((iY+1)==iDigitiser3DEvent.Pos3D().iY) && ((iZ+1)==iDigitiser3DEvent.Pos3D().iZ)))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
    rawEventAnge3D=iDigitiser3DEvent.Tilt();
	if(!((rawEventAnge3D.iPhi==iPhi+1) &&(rawEventAnge3D.iTheta==iTheta+1)))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	if(!((iAlpha+1) == iDigitiser3DEvent.Rotation()))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
#ifndef __X86GMP__
	iDigitiser3DEvent.Set(iType,iX+2,iY+2,iZ+2,static_cast<TUint8>(iPointerNumber+1));
	//Set the Type temporarily to get through the assertion
	iDigitiser3DEvent.Set(TRawEvent::EPointer3DTiltAndMove);
  	if(!(((iX+2)==iDigitiser3DEvent.Pos3D().iX) && ((iY+2)==iDigitiser3DEvent.Pos3D().iY) && ((iZ+2)==iDigitiser3DEvent.Pos3D().iZ)))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
	if(!((iPointerNumber+1) == iDigitiser3DEvent.PointerNumber()))
		{Kern::Printf("failed check at line %d, %S",__LINE__,__FILE__); return KErrArgument;}
#endif //__X86GMP__
	
	NKern::ThreadEnterCS();
	// queue the event
	Kern::AddEvent(iDigitiser3DEvent);
	NKern::ThreadLeaveCS();
	return KErrNone;
	}

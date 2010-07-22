// Copyright (c) 1998-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\xyin\d_xyin.cpp
// Generic digitiser driver
//
//


#include <drivers/xyin.h>
#include <kernel/kern_priv.h>
#include <hal_data.h>

_LIT(KLitDigitiser,"Digitiser");

LOCAL_C void sampleDfc(TAny* aPtr)
	{
	((DDigitiser*)aPtr)->ProcessRawSample();
	}

LOCAL_C void penUpDfc(TAny* aPtr)
	{
	((DDigitiser*)aPtr)->ProcessPenUp();
	}

LOCAL_C TInt halFunction(TAny* aPtr, TInt aFunction, TAny* a1, TAny* a2)
	{
	DDigitiser* pH=(DDigitiser*)aPtr;
	return pH->HalFunction(aFunction,a1,a2);
	}

LOCAL_C void rxMsg(TAny* aPtr)
	{
	DDigitiser& h=*(DDigitiser*)aPtr;
	TMessageBase* pM=h.iMsgQ.iMessage;
	if (pM)
		h.HandleMsg(pM);
	}

DDigitiser::DDigitiser()
	:	DPowerHandler(KLitDigitiser),
		iMsgQ(rxMsg,this,NULL,1),
		iSampleDfc(sampleDfc,this,5),
		iPenUpDfc(penUpDfc,this,5),
		iOrientation(HALData::EDigitiserOrientation_default)
	{
//	iBufferIndex=0;
//	iLastPos=TPoint(0,0);
//	iState=EIdle;
//	iCount=0;
//	iPointerOn=EFalse
	}

TInt DDigitiser::Create()
	{
	TInt r=DoCreate();				// do hardware-dependent initialisation

	if (r!=KErrNone)
		return r;

	__ASSERT_DEBUG(iDfcQ, Kern::Fault("DDigitiser::Create iDfcQ not set", __LINE__));
	iMsgQ.SetDfcQ(iDfcQ);
	iSampleDfc.SetDfcQ(iDfcQ);
	iPenUpDfc.SetDfcQ(iDfcQ);

	TInt n=iCfg.iPenUpDiscard;		// number of samples to delay
	iBuffer=(TPoint*)Kern::Alloc(n*sizeof(TPoint));
	if (!iBuffer)
		return KErrNoMemory;

	// install the HAL function
	r=Kern::AddHalEntry(EHalGroupDigitiser,halFunction,this);
	if (r!=KErrNone)
		return r;

	iMsgQ.Receive();

	// wait for pen down
	WaitForPenDown();

	return r;
	}

void DDigitiser::RawSampleValid()
//
// Called by hardware-dependent code when a raw sample is available
//
	{
	iSampleDfc.Add();
	}

void DDigitiser::PenUp()
//
// Called by hardware-dependent code when the pen goes up
//
	{
	iPenUpDfc.Add();
	}

void DDigitiser::ProcessRawSample()
//
// DFC to process a raw sample
//
	{
	TPoint p;
	TInt r;
	TBool ok=SamplesToPoint(p);
	if (!ok)
		{
		// wait for pen to stabilise
		__KTRACE_XY2(Kern::Printf("BS"));
		WaitForPenUpDebounce();
		return;
		}
	__KTRACE_XY2(Kern::Printf("GS (%d,%d) %d",p.iX,p.iY,iState));
	switch (iState)
		{
		case EIdle:
			// pen has just gone down
			iCount=iCfg.iPenDownDiscard;
			iState=EDiscardOnPenDown;
			// fall through
		case EDiscardOnPenDown:
			if (iCount)
				{
				// still discarding
				iCount--;
				break;
				}
			iState=EBufferFilling;
			iBufferIndex=0;
			iCount=iCfg.iPenUpDiscard;
			// fall through
		case EBufferFilling:
			if (iCount)
				{
				// buffer still filling
				iCount--;
				iBuffer[iBufferIndex++]=p;
				if (iBufferIndex==iCfg.iPenUpDiscard)
					iBufferIndex=0;
				break;
				}
			iState=EBufferFull;
			// fall through
		case EBufferFull:
			r=DelayAndConvertSample(p,iLastPos);
			if (r!=KErrNone)
				break;					// off the screen, so don't issue Pen Down Event
			iState=EPenDown;
			ResetPenMoveFilter();
			IssuePenDownEvent();
			break;
		case EPenDown:
			r=DelayAndConvertSample(p,p);
			if (r!=KErrNone)
				{
				iState=EIdle;			// off the screen, so treat as pen-up
				IssuePenUpEvent();
				break;
				}
			FilterPenMove(p);
			break;
		};
	WaitForPenUp();		// request another sample from the hardware
	}

void DDigitiser::ProcessPenUp()
//
// DFC to process pen-up events
//
	{
	__KTRACE_XY2(Kern::Printf("up %d",iState));
	switch (iState)
		{
		case EIdle:
		case EDiscardOnPenDown:
		case EBufferFilling:
		case EBufferFull:
			iState=EIdle;
			break;
		case EPenDown:
			iState=EIdle;
			IssuePenUpEvent();
			break;
		}
	WaitForPenDown();	// tell the hardware to watch for another pen-down
	}

TBool DDigitiser::SamplesToPoint(TPoint& aPoint)
//
// Average and validate the raw samples from the hardware
//
	{
#if defined(__DIGITISER_DEBUG2__)
	TBuf<80> buf;
#endif
	TInt i;
	TInt minx=KMaxTInt;
	TInt miny=KMaxTInt;
	TInt maxx=KMinTInt;
	TInt maxy=KMinTInt;
	TInt sumx=0;
	TInt sumy=0;
	TInt n=iCfg.iNumXYSamples;
	for (i=0; i<n; i++)
		{
		TInt x=iX[i];
		if (x<minx)
			minx=x;
		if (x>maxx)
			maxx=x;
		sumx+=x;
		TInt y=iY[i];
		if (y<miny)
			miny=y;
		if (y>maxy)
			maxy=y;
		sumy+=y;
//		__KTRACE_XY2(buf.AppendFormat(_L("(%d,%d) "),x,y));
		__KTRACE_XY2(Kern::Printf("(%d,%d) ",x,y));
		}
//	__KTRACE_XY2(Kern::Printf("%S", buf));

	TInt spreadx=maxx-minx;
	TInt spready=maxy-miny;
	if (iCfg.iDisregardMinMax)
		{
		sumx-=minx;					// disregard extremal values in average
		sumx-=maxx;
		sumy-=miny;
		sumy-=maxy;
		n-=2;
		}
	sumx/=n;	// average the values
	sumy/=n;
	if (spreadx<iCfg.iSpreadX && spready<iCfg.iSpreadY && sumx>=iCfg.iMinX && sumx<=iCfg.iMaxX && sumy>=iCfg.iMinY && sumy<=iCfg.iMaxY)
		{
		// samples are OK
		aPoint.iX=sumx;
		aPoint.iY=sumy;
		return ETrue;
		}
	// samples are dodgy
	return EFalse;
	}

TInt DDigitiser::DelayAndConvertSample(const TPoint& aSample, TPoint& aScreenPoint)
//
// Pass a sample through the delay line and convert to screen coordinates
//
	{
	if (iCfg.iPenUpDiscard != 0)
		{
		TPoint p=iBuffer[iBufferIndex];		// sample leaving delay line
		iBuffer[iBufferIndex++]=aSample;	// sample entering delay line
		if (iBufferIndex==iCfg.iPenUpDiscard)
			iBufferIndex=0;
		return DigitiserToScreen(p,aScreenPoint);
		}
	return DigitiserToScreen(aSample,aScreenPoint);
	}

void DDigitiser::IssuePenDownEvent()
	{
	TRawEvent e;
	e.Set(TRawEvent::EButton1Down,iLastPos.iX,iLastPos.iY);
	Kern::AddEvent(e);
	__KTRACE_XY2(Kern::Printf("D %d,%d",e.Pos().iX,e.Pos().iY));
	}

void DDigitiser::IssuePenUpEvent()
	{
	TRawEvent e;
	e.Set(TRawEvent::EButton1Up,iLastPos.iX,iLastPos.iY);
	Kern::AddEvent(e);
	__KTRACE_XY2(Kern::Printf("U %d,%d",e.Pos().iX,e.Pos().iY));
	}

void DDigitiser::IssuePenMoveEvent(const TPoint& aPoint)
	{
	TRawEvent e;
	e.Set(TRawEvent::EPointerMove,aPoint.iX,aPoint.iY);
	Kern::AddEvent(e);
	__KTRACE_XY2(Kern::Printf("M %d,%d",e.Pos().iX,e.Pos().iY));
	}

void DDigitiser::HandleMsg(TMessageBase* aMsg)
	{
	if (aMsg->iValue)
		DigitiserOn();
	else
		DigitiserOff();
	aMsg->Complete(KErrNone,ETrue);
	}

TInt DDigitiser::HalFunction(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNone;
__KTRACE_OPT(KEXTENSION,Kern::Printf("HalFunction %d", aFunction));
	switch(aFunction)
		{
		case EDigitiserHalXYInfo:
			{
			TPckgBuf<TDigitiserInfoV01> vPckg;
			DigitiserInfo(vPckg());
			Kern::InfoCopy(*(TDes8*)a1,vPckg);
			break;
			}
		case EDigitiserHalSetXYInputCalibration:
			{
			if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDigitiserHalSetXYInputCalibration")))
				return KErrPermissionDenied;
			TDigitizerCalibration cal;
			kumemget32(&cal,a1,sizeof(TDigitizerCalibration));
			r=SetXYInputCalibration(cal);
			break;
			}
		case EDigitiserHalCalibrationPoints:
			TDigitizerCalibration cal;
			r=CalibrationPoints(cal);
			kumemput32(a1,&cal,sizeof(TDigitizerCalibration));
			break;
		case EDigitiserHalSaveXYInputCalibration:
			r=SaveXYInputCalibration();
			break;
		case EDigitiserHalRestoreXYInputCalibration:
			if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDigitiserHalRestoreXYInputCalibration")))
				return KErrPermissionDenied;
			r=RestoreXYInputCalibration((TDigitizerCalibrationType)(TInt)a1);
			break;
		case EDigitiserHalSetXYState:
			{
			if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDigitiserHalSetXYState")))
				return KErrPermissionDenied;
			if ((TBool)a1)
				{
				TThreadMessage& m=Kern::Message();
				m.iValue = ETrue;
				m.SendReceive(&iMsgQ);
				}
			else
				{
				TThreadMessage& m=Kern::Message();
				m.iValue = EFalse;
				m.SendReceive(&iMsgQ);
				}
			}
			break;
		case EDigitiserHalXYState:
			kumemput32(a1, (TBool*)&iPointerOn, sizeof(TBool));
			break;
			
		// a2 = TBool aSet (ETrue for setting, EFalse for retrieval) 
		// a1 = TDigitizerOrientation (set)
		// a1 = &TDigitizerOrientation (get)
		case EDigitiserOrientation:	
			if ((TBool)a2)
				{
				// Set the orientation attribute
				// In case user thread, check it has WDD capability
				if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDigitiserOrientation")))
					return KErrPermissionDenied;
				iOrientation = (TInt)a1;
				}
			else
				{
				// Get the orientation attribute, safe copy it into user memory
				kumemput32(a1, &iOrientation, sizeof(TInt));	
				}
			break; 
			
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("Starting digitiser driver"));
	if (Kern::SuperPage().iCpuId & KCpuIdISS)
		return KErrNone;	// no digitiser on ARMULATOR
	DDigitiser* pD=DDigitiser::New();
	TInt r=KErrNoMemory;
	if (pD)
		r=pD->Create();
	__KTRACE_OPT(KEXTENSION,Kern::Printf("Returning %d",r));
	return r;
	}

#ifdef __BUILD_DEVICE_DRIVER__
class DDigitiserPdd : public DPhysicalDevice
	{
public:
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	};

_LIT(KPddName,"XYInput");

TInt DDigitiserPdd::Install()
	{
	return SetName(&KPddName);
	}

void DDigitiserPdd::GetCaps(TDes8&) const
	{
	}

TInt DDigitiserPdd::Create(DBase*& aChannel, TInt, const TDesC8*, const TVersion&)
	{
	aChannel=NULL;
	return KErrNone;
	}

TInt DDigitiserPdd::Validate(TInt, const TDesC8*, const TVersion&)
	{
	return KErrNotSupported;
	}

DECLARE_EXTENSION_PDD()
	{
	return new DDigitiserPdd;
	}
#endif

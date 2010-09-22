// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\digitiser\d_ldddigitisertest.cpp
// LDD for testing the Digitiser  Hal Calls.
// 
//

#include <kernel/kernel.h>

#include "d_ldddigitisertest.h"

const TUint KMaxDeviceNumber=10;

TUint 	gDeviceNumber=1;					// Device Number
TUint	gRegisteredDeviceNumber;			// Represents the registered Hal Entry
TBool	gHalEntryRegistered; 				// States HAL Entry Successfully registered or not
static TDigitiserInfoV02 gDigitserHalData;	// Buffer to  hold the HAL data; 
static TUint gPointer3DSupported;			// States whether the pointing device supports 3rd dimension.
static TRawEvent   kEvent;	


class DDigitiserLDDTestFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	virtual TInt Install(); 								//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;				//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

class DDigitiserLDDTestChannel : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DDigitiserLDDTestChannel();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
	};			

// Initialise Digitiser HAL Data
void initialiseDigitiserHalData()
	{
//  TDigitiserInfoV02 Class Data layout
  
//class TDigitiserInfoV02 : public TDigitiserInfoV01
//	{	
//public :
//	TInt iZRange;				/**< The maximum distance to screen a pointing device will be detected (settable).*/
//	TUint8 iThetaSupported;		/**< A Boolean value that indicates if Theta polar angle detection (tilt) is supported.*/
//	TUint8 iPhiSupported;		/**< A Boolean value that indicates if Phi polar angle detection (tilt) is supported.*/
//	TUint8 iAlphaSupported;		/**< A Boolean value that indicates if rotation of the pointing device along its main axis is supported.*/
//	TUint8 iPressureSupported; 	/**< A Boolean value that indicates if pressure applied on screen is supported.*/
//	TInt iProximityStep;    	/**<proximity resolution, e.g. proximity readings change in steps of 1, 5, 10, ... */
//	TInt iMaxPressure;      	/**< maximum pressure reading*/
//	TInt iPressureStep;     	/**< pressure resolution, */	
//	TUint8 iMaxPointers;    	/**< max number of multi-touch pointers supported by hardware/driver.*/ 
//	TUint8 iNumberOfPointers;	/**< Number of pointers it supports (settable).*/
//	};

    gPointer3DSupported=1;
    gDigitserHalData.iZRange=100;
	gDigitserHalData.iThetaSupported=1;
	gDigitserHalData.iPhiSupported=1;
	gDigitserHalData.iPressureSupported=1;
	gDigitserHalData.iAlphaSupported=1;
	gDigitserHalData.iProximityStep=5;
	gDigitserHalData.iMaxPointers=2;
	gDigitserHalData.iNumberOfPointers=2;
	gDigitserHalData.iMaxPressure=5000;
	gDigitserHalData.iPressureStep=500;
	}

LOCAL_C TInt halFunction(TAny* /*aPtr*/, TInt aFunction, TAny* a1, TAny* /*a2*/)
	{
	TInt r=KErrNone;
	TPckgBuf<TDigitiserInfoV02> vPckg(gDigitserHalData);
	switch(aFunction)
		{
		case EDigitiserHal3DInfo:
			// Get Request for all but EPointer3D. Copy the TDigitiserInfoV02 object to a1
			Kern::InfoCopy(*(TDes8*)a1,vPckg);
			break;
		case EDigitiserHal3DPointer:
			// Get request EPointer3D
			kumemput32(a1,&gPointer3DSupported,sizeof(gPointer3DSupported));
			break;
		case EDigitiserHalSetZRange:
			// Set requeest for EPointer3DMaxProximity (iZRange)
			gDigitserHalData.iZRange=(TInt)a1;
			break;
		case EDigitiserHalSetNumberOfPointers:
			// Set request for EPointerNumberOfPointers
			gDigitserHalData.iNumberOfPointers=static_cast<TUint8>((TInt)a1);
			break;
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

#ifdef GRAPHICS_VERSION
DECLARE_EXTENSION_LDD()
	{
	return new DDigitiserLDDTestFactory;
	}
DECLARE_STANDARD_EXTENSION()
	{
	TInt r;
	// Initialise Digitiser HAL Data first.
	initialiseDigitiserHalData();
	// Find device number
	do
		{
		r=Kern::AddHalEntry(EHalGroupDigitiser,halFunction,NULL,gDeviceNumber);
		}
	while((r==KErrInUse) && (++gDeviceNumber < KMaxDeviceNumber));
		
	if((gDeviceNumber < KMaxDeviceNumber) && (r==KErrNone))
		{
		gHalEntryRegistered = ETrue;
		gRegisteredDeviceNumber = gDeviceNumber;
		}
	else
		{
		gHalEntryRegistered = EFalse;
		r=KErrInUse;
		}
	return r;
	}
#else
DECLARE_STANDARD_LDD()
	{
	return new DDigitiserLDDTestFactory;
	}
#endif


TInt DDigitiserLDDTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
//
// Create new channel
//  
	aChannel=new DDigitiserLDDTestChannel;
	return aChannel?KErrNone:KErrNoMemory;
	}

TInt DDigitiserLDDTestFactory::Install()
//
// Install the LDD - overriding pure virtual
	{
	return SetName(&KLddName);
	}

void DDigitiserLDDTestFactory::GetCaps(TDes8& /*aDes*/) const
//
// Get capabilities - overriding pure virtual
//
	{
	}


TInt DDigitiserLDDTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
//
// Create channel
//
	{
	return KErrNone;
	}

DDigitiserLDDTestChannel::~DDigitiserLDDTestChannel()
//
// Destructor
//
	{
	}

TInt DDigitiserLDDTestChannel::Request(TInt aReqNo, TAny* a1, TAny* /*a2*/)
	{
	TInt r=KErrNone;
	TUsrEventBuf eventBuf;
	TPoint3D point3D;
	TPoint   point2D;
	TAngle3D angle3D;

	switch(aReqNo)
		{
		case (RLddDigitiserTest::EADDHALENTRY):
#ifndef GRAPHICS_VERSION
            NKern::ThreadEnterCS();
			do
			{
		    r=Kern::AddHalEntry(EHalGroupDigitiser,halFunction,this,gDeviceNumber);
			}
            while((r==KErrInUse) && (++gDeviceNumber < KMaxDeviceNumber));
			NKern::ThreadLeaveCS();

			if((gDeviceNumber < KMaxDeviceNumber) && (r==KErrNone))
				{
				gHalEntryRegistered=ETrue;
				gRegisteredDeviceNumber=gDeviceNumber;
				}
			else
				{
				gHalEntryRegistered=EFalse;
				r=KErrInUse;
				}
#else
			r=KErrNotSupported;
#endif
			break;
		case (RLddDigitiserTest::EREMOVEHALENTRY):
			if(gHalEntryRegistered)
				{
				r=Kern::RemoveHalEntry(EHalGroupDigitiser,gRegisteredDeviceNumber);
				}
		break;
		case (RLddDigitiserTest::EGETREGISTEREDDEVICENUMBER):
			if(gHalEntryRegistered)
				{
				r=gRegisteredDeviceNumber;
				}
			else
				{
				r=KErrNotFound;
				}
			break;
		case (RLddDigitiserTest::EINITIALISEHALDATA):
#ifndef GRAPHICS_VERSION
			initialiseDigitiserHalData();
#else
			r=KErrNone;
#endif
			break;
		case (RLddDigitiserTest::EGET_EPOINTER3D):
			r=gPointer3DSupported;
			break;
		case (RLddDigitiserTest::ESET_EPOINTER3D):
			gPointer3DSupported=(TUint32)a1;
			break;
		case (RLddDigitiserTest::EGET_EPOINTERMAXPROXIMITY):
			r=gDigitserHalData.iZRange;
			break;
		case (RLddDigitiserTest::ESET_EPOINTERMAXPROXIMITY):
			gDigitserHalData.iZRange=(TInt)a1;
			break;
		case (RLddDigitiserTest::EGET_EPOINTER3DTHETASUPPORTED):
			r=gDigitserHalData.iThetaSupported;
			break;
		case (RLddDigitiserTest::ESET_EPOINTER3DTHETASUPPORTED):
			gDigitserHalData.iThetaSupported=static_cast<TUint8>((TInt)a1);
			break;
		case (RLddDigitiserTest::EGET_EPOINTER3DPHISUPPORTED):
			r=gDigitserHalData.iPhiSupported;
			break;
		case (RLddDigitiserTest::ESET_EPOINTER3DPHISUPPORTED):
			gDigitserHalData.iPhiSupported=static_cast<TUint8>((TInt)a1);
			break;
		case (RLddDigitiserTest::EGET_EPOINTER3DROTATIONSUPPORTED):
			r=gDigitserHalData.iAlphaSupported;
			break;
		case (RLddDigitiserTest::ESET_EPOINTER3DROTATIONSUPPORTED):
			gDigitserHalData.iAlphaSupported=static_cast<TUint8>((TInt)a1);
			break;
		case (RLddDigitiserTest::EGET_EPOINTER3DPRESSURESUPPORTED):
			r=gDigitserHalData.iPressureSupported;
			break;
		case (RLddDigitiserTest::ESET_EPOINTER3DPRESSURESUPPORTED):
			gDigitserHalData.iPressureSupported=static_cast<TUint8>((TInt)a1);
			break;
		case (RLddDigitiserTest::EGET_EPOINTER3DPROXIMITYSTEP):
			r=gDigitserHalData.iProximityStep;
			break;
		case (RLddDigitiserTest::ESET_EPOINTER3DPROXIMITYSTEP):
			gDigitserHalData.iProximityStep=(TInt)a1;
			break;
		case (RLddDigitiserTest::EGET_EPOINTER3DMAXPOINTERS):
			r=gDigitserHalData.iMaxPointers;
			break;
		case (RLddDigitiserTest::ESET_EPOINTER3DMAXPOINTERS):
			gDigitserHalData.iMaxPointers=static_cast<TUint8>((TInt)a1);
			break;
		case (RLddDigitiserTest::EGET_EPOINTER3DNUMBEROFPOINTERS):
			r=gDigitserHalData.iNumberOfPointers;
			break;
		case (RLddDigitiserTest::ESET_EPOINTER3DNUMBEROFPOINTERS):
			gDigitserHalData.iNumberOfPointers=static_cast<TUint8>((TInt)a1);
			break;
		case (RLddDigitiserTest::EGET_EPOINTER3DMAXPRESSURE):
			r=gDigitserHalData.iMaxPressure;
			break;
		case (RLddDigitiserTest::ESET_EPOINTER3DMAXPRESSURE):
			gDigitserHalData.iMaxPressure=(TInt)a1;
			break;
		case (RLddDigitiserTest::EGET_EPOINTER3DPRESSURESTEP):
			r=gDigitserHalData.iPressureStep;
			break;
		case (RLddDigitiserTest::ESET_EPOINTER3DPRESSURESTEP):
			gDigitserHalData.iPressureStep=(TInt)a1; 		
			break;
		
		//Testing TRawEvents;


		case  (RLddDigitiserTest::ESET_TRAWEVENT_EVENTTYPE): 
			  kEvent.Set(static_cast<TRawEvent::TType>((TInt)a1));			
			  break;

		case  (RLddDigitiserTest::EGET_TRAWEVENT_EVENTTYPE):   
			    r=kEvent.Type();				 
				break;

		case  (RLddDigitiserTest::ESET_TRAWEVENT_TILT): 			   
			   kumemget32(&eventBuf,a1,sizeof(eventBuf));
			   kEvent.SetTilt(static_cast<TRawEvent::TType>(eventBuf().iType),eventBuf().iPhi,eventBuf().iTheta);
			   break;

				
		case  (RLddDigitiserTest::EGET_TRAWEVENT_TILT): 				
				angle3D=kEvent.Tilt();
				eventBuf().iPhi =	angle3D.iPhi;
				eventBuf().iTheta =	angle3D.iTheta;	
				kumemput32(a1,&eventBuf,sizeof(eventBuf));
			   break;


		case  (RLddDigitiserTest::ESET_TEVENT_DNMBR):   
			
			   kEvent.SetDeviceNumber((TInt)a1);			   
			   break;
		case  (RLddDigitiserTest::EGET_TEVENT_DNMBR): 
				r=kEvent.DeviceNumber();				
				break;

		case  (RLddDigitiserTest::ESET_TRAWEVENT_ROTATION): 				    			    
				kumemget32(&eventBuf,a1,sizeof(eventBuf));
				kEvent.SetRotation(static_cast<TRawEvent::TType>(eventBuf().iType),eventBuf().iAlpha);				
				break;

		case  (RLddDigitiserTest::EGET_TRAWEVENT_ROTATION):  
			    r=kEvent.Rotation();				
				break;

		case  (RLddDigitiserTest::ESET_TRAWEVENT_REPEAT): 
			   kumemget32(&eventBuf,a1,sizeof(eventBuf));
			   kEvent.SetRepeat(static_cast<TRawEvent::TType>(eventBuf().iType),eventBuf().iScanCode,eventBuf().iRepeats);
			   break;

		case  (RLddDigitiserTest::EGET_TRAWEVENT_REPEAT): 
				r=kEvent.Repeats();
				break;
	
		case  (RLddDigitiserTest::ESET_TRAWEVENT_SCANCODE):				    
				kumemget32(&eventBuf,a1,sizeof(eventBuf));
				kEvent.Set(static_cast<TRawEvent::TType>(eventBuf().iType),eventBuf().iScanCode);				
				break;
		case  (RLddDigitiserTest::EGET_TRAWEVENT_SCANCODE): 			 
				r=kEvent.ScanCode();				
				break;

		case  (RLddDigitiserTest::ESET_TRAWEVENT_POS2D):				
				kumemget32(&eventBuf,a1,sizeof(eventBuf));
				kEvent.Set(static_cast<TRawEvent::TType>(eventBuf().iType),eventBuf().iX,eventBuf().iY);
				break;
		case  (RLddDigitiserTest::EGET_TRAWEVENT_POS2D): 
				point2D=kEvent.Pos();
				eventBuf().iX =	point2D.iX;
				eventBuf().iY =	point2D.iY;	  			 				
				kumemput32(a1,&eventBuf,sizeof(eventBuf));
				break;

		
		case  (RLddDigitiserTest::ESET_TRAWEVENT_PTRNMBR): 
				kEvent.SetPointerNumber(static_cast<TUint8>((TInt)a1));		
				break;

		case  (RLddDigitiserTest::ESET_TRAWEVENT_3DNPTRNMBR):				
				kumemget32(&eventBuf,a1,sizeof(eventBuf));
				//IMPORT_C void Set (TType aType, TInt aX, TInt aY, TInt aZ, TUint8 aPointerNumbe
				kEvent.Set(static_cast<TRawEvent::TType>(eventBuf().iType),eventBuf().iX,eventBuf().iY,eventBuf().iZ,eventBuf().iPointerNumber);
				break;
		case  (RLddDigitiserTest::EGET_TRAWEVENT_PTRNMBR): 
				r=kEvent.PointerNumber();				
				break;
 
		case  (RLddDigitiserTest::EGET_TRAWEVENT_TICKS): 
				r=kEvent.Ticks();
				break;
	
		case  (RLddDigitiserTest::ESET_TRAWEVENT_TIP): 
				kEvent.SetTip(static_cast<TBool>((TInt)a1));			   
				break;
		case  (RLddDigitiserTest::EGET_TRAWEVENT_TIP):
				r=kEvent.IsTip();
				break;

		case  (RLddDigitiserTest::ESET_TRAWEVENT_POS3D):								
			kumemget32(&eventBuf,a1,sizeof(eventBuf));
			kEvent.Set(static_cast<TRawEvent::TType>(eventBuf().iType),eventBuf().iX,eventBuf().iY,eventBuf().iZ);
			break;
			
		case  (RLddDigitiserTest::EGET_TRAWEVENT_POS3D):

			point3D=kEvent.Pos3D();	
			eventBuf().iX =	point3D.iX;
			eventBuf().iY =	point3D.iY;	  
			eventBuf().iZ =	point3D.iZ;	  			
			kumemput32(a1,&eventBuf,sizeof(eventBuf));
			break;

			
		case  (RLddDigitiserTest::ESET_TRAWEVENT_ALL):								
			kumemget32(&eventBuf,a1,sizeof(eventBuf));
			kEvent.Set(static_cast<TRawEvent::TType>(eventBuf().iType),eventBuf().iX,eventBuf().iY,eventBuf().iZ,eventBuf().iPhi,eventBuf().iTheta,eventBuf().iAlpha);
			break;
			
		default:
			r=KErrNotSupported;
			break;
		} 
	return r;
	}

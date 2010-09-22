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
// e32test\digitiser\d_ldddigitisertest.h
// 
//

#if !defined(__DLDDDIGITISERTEST_H__)
#define __DLDDDIGITISERTEST_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32svr.h>
#endif

_LIT(KLddName,"D_LDDDIGITISERTEST.LDD");

struct TRawEventInfo
	{

	TInt iType;
	TInt iScanCode;
	TInt iRepeats;
	TInt iX;
    TInt iY;	   
	TInt iZ;
	TInt iPhi;
	TInt iTheta;
	TInt iAlpha;
	TUint8 iPointerNumber;
	TUint8 iTip;	
	};

 typedef TPckgBuf <TRawEventInfo> TUsrEventBuf;	
 typedef TRawEvent::TType  RawEventType;



class RLddDigitiserTest : public RBusLogicalChannel
	{
public:

	enum TControl
		{
		EADDHALENTRY = 1,
		EREMOVEHALENTRY,
		EGETREGISTEREDDEVICENUMBER,
		EINITIALISEHALDATA,

		EGET_EPOINTER3D,
		ESET_EPOINTER3D,

		EGET_EPOINTERMAXPROXIMITY,
		ESET_EPOINTERMAXPROXIMITY,
		
		EGET_EPOINTER3DTHETASUPPORTED,
		ESET_EPOINTER3DTHETASUPPORTED,

		EGET_EPOINTER3DPHISUPPORTED,
		ESET_EPOINTER3DPHISUPPORTED,

		EGET_EPOINTER3DROTATIONSUPPORTED,
		ESET_EPOINTER3DROTATIONSUPPORTED,

		EGET_EPOINTER3DPRESSURESUPPORTED,
		ESET_EPOINTER3DPRESSURESUPPORTED,

		EGET_EPOINTER3DPROXIMITYSTEP,
		ESET_EPOINTER3DPROXIMITYSTEP,

		EGET_EPOINTER3DMAXPOINTERS,
		ESET_EPOINTER3DMAXPOINTERS,

		EGET_EPOINTER3DNUMBEROFPOINTERS,
		ESET_EPOINTER3DNUMBEROFPOINTERS,
 
		EGET_EPOINTER3DMAXPRESSURE,
		ESET_EPOINTER3DMAXPRESSURE,

		EGET_EPOINTER3DPRESSURESTEP,
		ESET_EPOINTER3DPRESSURESTEP,		

       //TRawEvents Get Set Enums		
		
		ESET_TRAWEVENT_EVENTTYPE,
		EGET_TRAWEVENT_EVENTTYPE,

		ESET_TRAWEVENT_TILT,
		EGET_TRAWEVENT_TILT,


		ESET_TEVENT_DNMBR,
		EGET_TEVENT_DNMBR,

		ESET_TRAWEVENT_ROTATION,
		EGET_TRAWEVENT_ROTATION,

		ESET_TRAWEVENT_REPEAT,
		EGET_TRAWEVENT_REPEAT,
		

		ESET_TRAWEVENT_SCANCODE,
		EGET_TRAWEVENT_SCANCODE,

		ESET_TRAWEVENT_POS2D,
		EGET_TRAWEVENT_POS2D,
		
		ESET_TRAWEVENT_POS3D,
		EGET_TRAWEVENT_POS3D,

	

		
		ESET_TRAWEVENT_PTRNMBR,
		ESET_TRAWEVENT_3DNPTRNMBR,
		EGET_TRAWEVENT_PTRNMBR,


		EGET_TRAWEVENT_TICKS,	 

	
		ESET_TRAWEVENT_TIP,
		EGET_TRAWEVENT_TIP,
			
		ESET_TRAWEVENT_ALL		
		};

	


public:
	TInt Open();
	TInt registerHalHandler();
	TInt removeHalHandler();
	TInt getRegisteredDeviceNumber();
	TInt initialiseHalData();

	TInt getPointer3D();
	TInt setPointer3D(TUint aPointer3D);

	TInt getPointer3DMaxProximity();
	TInt setPointer3DMaxProximity(TUint aPointer3DMaxProximity);
	
	TInt getPointer3DThetaSupported();
	TInt setPointer3DThetaSupported(TUint aPointer3DThetaSupported);
	
	TInt getPointer3DPhiSupported();
	TInt setPointer3DPhiSupported(TUint aPointer3DPhiSupported);
	
	TInt getPointer3DRotationSupported();
	TInt setPointer3DRotationSupported(TUint aPointer3DRotationSupported);
	
	TInt getPointer3DPressureSupported();
	TInt setPointer3DPressureSupported(TUint aPointer3DPressureSupported);

	TInt getPointer3DProximityStep();
	TInt setPointer3DProximityStep(TUint aPointer3DProximityStep);
	
	TInt getPointerMaxPointers();
	TInt setPointerMaxPointers(TUint aPointerMaxPointers);
	
	TInt getPointerNumberOfPointers();
	TInt setPointerNumberOfPointers(TUint aPointerNumberOfPointers);
	
	TInt getPointer3DMaxPressure();
	TInt setPointer3DMaxPressure(TUint aPointer3DMaxPressure);
	
	TInt getPointer3DPressureStep();
	TInt setPointer3DPressureStep(TUint aPointer3DPressureStep);
	

	//TRawEvents

	TInt setTEvntType(TInt aType);
	TInt getTEvntType();

	TInt setTEvntDNum(TInt aDNum);
	TInt getTEvntDNum();
	
	TInt setTEvnt3DnPntr (TInt aType, TInt aX, TInt aY, TInt aZ, TUint8 aPointerNumber);
	TInt setTEvntPntr(TInt aPointerNumber);    
	TInt getTEvntPntr();

	TInt getTEvntTicks();
	 
	TInt setTEvntTip(TBool aTip);
	TBool TEvntTicksIsTip();		

	TInt setTEvntScanCode(TInt aType,TInt aScanCode);
	TInt getTEvntScanCode();

	TInt setTEvntRotation(TInt aType,TInt aAlpha);	 
	TInt getTEvntRotation();


	TInt setTEvntPos(TInt aType,TInt aX,TInt aY);	 
	TInt getTEvntPos(TUsrEventBuf& eventBuf);
	
	TInt setTEvntPos3D(TInt aType,TInt aX,TInt aY,TInt aZ);
	TInt getTEventPos3D(TUsrEventBuf& eventBuf);

	TInt setTEvntAll(TInt aType,TInt aX,TInt aY,TInt aZ,TInt aPhi,TInt aTheta,TInt aAlpha);

	TInt setTEvntTilt(TInt aType,TInt aPhi,TInt aTheta);										
	TInt getTEvntTilt(TUsrEventBuf& eventBuf);

	TInt setTEvntRepeat(TInt aType,TInt aScanCode,TInt aRepeats);
	TInt getTEvntRepeat();										 	
	};



#include "d_lddDigitisertest.inl"
#endif   //__DLDDDIGITISERTEST_H__

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
#include <e32std.h>
#endif

_LIT(KLddName,"D_LDDDIGITISERTEST.LDD");

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
		ESET_EPOINTER3DPRESSURESTEP
		};

public:
	inline TInt Open();
	inline TInt registerHalHandler();
	inline TInt removeHalHandler();
	inline TInt getRegisteredDeviceNumber();
	inline TInt initialiseHalData();

	inline TInt getPointer3D();
	inline TInt setPointer3D(TUint aPointer3D);

	inline TInt getPointer3DMaxProximity();
	inline TInt setPointer3DMaxProximity(TUint aPointer3DMaxProximity);
	
	inline TInt getPointer3DThetaSupported();
	inline TInt setPointer3DThetaSupported(TUint aPointer3DThetaSupported);
	
	inline TInt getPointer3DPhiSupported();
	inline TInt setPointer3DPhiSupported(TUint aPointer3DPhiSupported);
	
	inline TInt getPointer3DRotationSupported();
	inline TInt setPointer3DRotationSupported(TUint aPointer3DRotationSupported);
	
	inline TInt getPointer3DPressureSupported();
	inline TInt setPointer3DPressureSupported(TUint aPointer3DPressureSupported);

	inline TInt getPointer3DProximityStep();
	inline TInt setPointer3DProximityStep(TUint aPointer3DProximityStep);
	
	inline TInt getPointerMaxPointers();
	inline TInt setPointerMaxPointers(TUint aPointerMaxPointers);
	
	inline TInt getPointerNumberOfPointers();
	inline TInt setPointerNumberOfPointers(TUint aPointerNumberOfPointers);
	
	inline TInt getPointer3DMaxPressure();
	inline TInt setPointer3DMaxPressure(TUint aPointer3DMaxPressure);
	
	inline TInt getPointer3DPressureStep();
	inline TInt setPointer3DPressureStep(TUint aPointer3DPressureStep);
	};

#include "d_ldddigitisertest.inl"
#endif   //__DLDDDIGITISERTEST_H__

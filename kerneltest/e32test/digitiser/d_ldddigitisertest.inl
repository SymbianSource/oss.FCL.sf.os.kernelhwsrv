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
// e32test\digitiser\d_ldddigitisertest.inl
// 
//

#ifndef __KERNEL_MODE__
inline TInt RLddDigitiserTest::Open()
	{
	return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL,NULL);
	}

inline TInt RLddDigitiserTest::registerHalHandler()
	{
    return DoControl(EADDHALENTRY);
	}

inline TInt RLddDigitiserTest::removeHalHandler()
	{
    return DoControl(EREMOVEHALENTRY);
	}

inline TInt RLddDigitiserTest::getRegisteredDeviceNumber()
	{
    return DoControl(EGETREGISTEREDDEVICENUMBER);
	}

inline TInt RLddDigitiserTest::initialiseHalData()
	{
    return DoControl(EINITIALISEHALDATA);
	}

inline TInt RLddDigitiserTest::getPointer3D()
	{
    return DoControl(EGET_EPOINTER3D);
	}

inline TInt RLddDigitiserTest::setPointer3D(TUint aPointer3D)
	{
    return DoControl(ESET_EPOINTER3D, (TAny *)aPointer3D);
	}

inline TInt RLddDigitiserTest::getPointer3DMaxProximity()
	{
    return DoControl(EGET_EPOINTERMAXPROXIMITY);
	}

inline TInt RLddDigitiserTest::setPointer3DMaxProximity(TUint aPointer3DMaxProximity)	
	{
    return DoControl(ESET_EPOINTERMAXPROXIMITY, (TAny *)aPointer3DMaxProximity);
	}
	
inline TInt RLddDigitiserTest::getPointer3DThetaSupported()
	{
    return DoControl(EGET_EPOINTER3DTHETASUPPORTED);
	}

inline TInt RLddDigitiserTest::setPointer3DThetaSupported(TUint aPointer3DThetaSupported)
	{
    return DoControl(ESET_EPOINTER3DTHETASUPPORTED, (TAny *)aPointer3DThetaSupported);
	}
	
inline TInt RLddDigitiserTest::getPointer3DPhiSupported()
	{
    return DoControl(EGET_EPOINTER3DPHISUPPORTED);
	}

inline TInt RLddDigitiserTest::setPointer3DPhiSupported(TUint aPointer3DPhiSupported)
	{
    return DoControl(ESET_EPOINTER3DPHISUPPORTED, (TAny *)aPointer3DPhiSupported);
	}
		
inline TInt RLddDigitiserTest::getPointer3DRotationSupported()
	{
    return DoControl(EGET_EPOINTER3DROTATIONSUPPORTED);
	}

inline TInt RLddDigitiserTest::setPointer3DRotationSupported(TUint aPointer3DRotationSupported)
	{
    return DoControl(ESET_EPOINTER3DROTATIONSUPPORTED, (TAny *)aPointer3DRotationSupported);
	}
	
inline TInt RLddDigitiserTest::getPointer3DPressureSupported()
	{
    return DoControl(EGET_EPOINTER3DPRESSURESUPPORTED);
	}

inline TInt RLddDigitiserTest::setPointer3DPressureSupported(TUint aPointer3DPressureSupported)
	{
    return DoControl(ESET_EPOINTER3DPRESSURESUPPORTED, (TAny *)aPointer3DPressureSupported);
	}

inline TInt RLddDigitiserTest::getPointer3DProximityStep()
	{
    return DoControl(EGET_EPOINTER3DPROXIMITYSTEP);
	}

inline TInt RLddDigitiserTest::setPointer3DProximityStep(TUint aPointer3DProximityStep)
	{
    return DoControl(ESET_EPOINTER3DPROXIMITYSTEP, (TAny *)aPointer3DProximityStep);
	}
		
inline TInt RLddDigitiserTest::getPointerMaxPointers()
	{
    return DoControl(EGET_EPOINTER3DMAXPOINTERS);
	}

inline TInt RLddDigitiserTest::setPointerMaxPointers(TUint aPointerMaxPointers)
	{
    return DoControl(ESET_EPOINTER3DMAXPOINTERS, (TAny *)aPointerMaxPointers);
	}

 inline TInt RLddDigitiserTest::getPointerNumberOfPointers()
	{
    return DoControl(EGET_EPOINTER3DNUMBEROFPOINTERS);
	}

inline TInt RLddDigitiserTest::setPointerNumberOfPointers(TUint aPointerNumberOfPointers)
	{
    return DoControl(ESET_EPOINTER3DNUMBEROFPOINTERS, (TAny *)aPointerNumberOfPointers);
	}
			
inline TInt RLddDigitiserTest::getPointer3DMaxPressure()
	{
    return DoControl(EGET_EPOINTER3DMAXPRESSURE);
	}

inline TInt RLddDigitiserTest::setPointer3DMaxPressure(TUint aPointer3DMaxPressure)
	{
    return DoControl(ESET_EPOINTER3DMAXPRESSURE, (TAny *)aPointer3DMaxPressure);
	}

inline TInt RLddDigitiserTest::getPointer3DPressureStep()
	{
    return DoControl(EGET_EPOINTER3DPRESSURESTEP);
	}

inline TInt RLddDigitiserTest::setPointer3DPressureStep(TUint aPointer3DPressureStep)
	{
    return DoControl(ESET_EPOINTER3DPRESSURESTEP, (TAny *)aPointer3DPressureStep);
	}
#endif



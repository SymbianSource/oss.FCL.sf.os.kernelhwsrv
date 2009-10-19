// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/usb/t_usb_device/include/apitests.h
// 
//

#ifndef __APITESTS_H__
#define __APITESTS_H__

extern void SetupDescriptors(LDDConfigPtr aLddPtr,RDEVCLIENT* aPort, TUint16 aPid = 0);

extern void TestEndpointDescriptor(RDEVCLIENT* aPort,TInt aIfSetting, TInt aEpNumber,TUsbcEndpointInfo aEpInfo);

extern void TestInvalidSetInterface (RDEVCLIENT* aPort,TInt aNumSettings);

extern void TestInvalidReleaseInterface (RDEVCLIENT* aPort,TInt aNumSettings);

extern void TestDescriptorManipulation(TBool aHighSpeed, RDEVCLIENT* aPort, TInt aNumSettings);

extern void TestOtgExtensions(RDEVCLIENT* aPort);

extern void TestEndpoint0MaxPacketSizes(RDEVCLIENT* aPort);

#endif	// __APITESTS_H__

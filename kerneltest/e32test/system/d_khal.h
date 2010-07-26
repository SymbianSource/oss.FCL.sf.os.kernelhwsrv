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
// e32test\system\d_khal.h
// 
//

#if !defined(__D_KHAL_H__)
#define __D_KHAL_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"D_KHAL.LDD");

class RLddKHalTest : public RBusLogicalChannel
	{
public:

	enum TControl
		{
		EAddHalEntryDevice0 = 1,
		EAddHalEntryDeviceX,
		EAddHalEntryForExistingFixed,
		ERemoveHalEntryDevice0,
		ERemoveHalEntryDeviceX,
		ERemoveHalEntryExistingFixed,
		EGetRegisteredDeviceNumber,
		EFindHalEntryDevice0,
		EFindHalEntryDevice0Other,
		EFindHalEntryDeviceX
		};

	enum THalFunc
		{
		ETestHalFunc
		};

public:
	inline TInt Open();
	inline TInt AddHalEntryDevice0();
	inline TInt AddHalEntryDeviceX();
	inline TInt AddHalEntryForExistingFixed();
	inline TInt RemoveHalEntryDevice0();
	inline TInt RemoveHalEntryDeviceX();
	inline TInt RemoveHalEntryExistingFixed();
	inline TInt GetRegisteredDeviceNumber();
	inline TInt FindHalEntryDevice0();
	inline TInt FindHalEntryDevice0Other();
	inline TInt FindHalEntryDeviceX();
	};


#ifndef __KERNEL_MODE__
inline TInt RLddKHalTest::Open()
	{
	return DoCreate(KLddName,TVersion(0,1,0),KNullUnit,NULL,NULL);
	}

inline TInt RLddKHalTest::AddHalEntryDevice0()
	{
    return DoControl(EAddHalEntryDevice0);
	}

inline TInt RLddKHalTest::AddHalEntryDeviceX()
	{
    return DoControl(EAddHalEntryDeviceX);
	}

inline TInt RLddKHalTest::AddHalEntryForExistingFixed()
	{
    return DoControl(EAddHalEntryForExistingFixed);
	}

inline TInt RLddKHalTest::RemoveHalEntryDevice0()
	{
    return DoControl(ERemoveHalEntryDevice0);
	}

inline TInt RLddKHalTest::RemoveHalEntryDeviceX()
	{
    return DoControl(ERemoveHalEntryDeviceX);
	}

inline TInt RLddKHalTest::RemoveHalEntryExistingFixed()
	{
    return DoControl(ERemoveHalEntryExistingFixed);
	}

inline TInt RLddKHalTest::GetRegisteredDeviceNumber()
	{
    return DoControl(EGetRegisteredDeviceNumber);
	}

inline TInt RLddKHalTest::FindHalEntryDevice0()
	{
    return DoControl(EFindHalEntryDevice0);
	}

inline TInt RLddKHalTest::FindHalEntryDevice0Other()
	{
    return DoControl(EFindHalEntryDevice0Other);
	}

inline TInt RLddKHalTest::FindHalEntryDeviceX()
	{
    return DoControl(EFindHalEntryDeviceX);
	}
#endif //__KERNEL_MODE__

#endif   //__D_KHAL_H__

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
// e32test\bench\d_prof.h
// 
//

#if !defined(__D_PROFILE_H__)
#define __D_PROFILE_H__
#if !defined(__E32STD_H__)
#include <e32std.h>
#endif

class TCapsProfileV01
	{
public:
	TVersion	iVersion;
	};

class TProfileData
	{
public:
	TUint iTotalCpuTime;
	TUint iMaxContinuousCpuTime;
	TUint iMaxTimeBeforeYield;
	};

class RProfile : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlResetProfile,
		EControlReadProfile,
		};
public:
	inline TInt Open();
	inline TInt Reset();
	inline TInt Read(RThread& aThread,TProfileData& aData);
	};

inline TInt RProfile::Open()
	{return DoCreate(_L("Profile"),TVersion(0,1,1),KNullUnit,NULL,NULL);}

inline TInt RProfile::Reset()
	{return DoControl(EControlResetProfile);}

inline TInt RProfile::Read(RThread& aThread, TProfileData& aData)
	{return DoControl(EControlReadProfile, (TAny*)aThread.Handle(), (TAny*)&aData);}

#endif

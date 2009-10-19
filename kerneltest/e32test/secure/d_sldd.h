// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\secure\d_sldd.h
// 
//

#ifndef __D_SLDD_H__
#define __D_SLDD_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

class TCapsTestV01
	{
public:
	TVersion	iVersion;
	};

class RLddTest : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlTest1=0,
		EGetIds,
		EGetKernelConfigFlags,
		ESetKernelConfigFlags,
		ESetDisabledCapabilities0,
		EKernelTestData,
		EGetSecureInfos,
		};
	enum TRequest
		{
		};
	enum TTestValues
		{
		ETest1Value = 0x07654321,
		};
	
	struct TIds
		{
		TVendorId iThreadVID;
		TVendorId iProcessVID;
		TSecureId iThreadSID;
		TSecureId iProcessSID;
		};

public:
	inline TInt OpenLocal();
	inline TInt OpenProtected();
	inline TInt Test1();
	inline void GetIds(RLddTest::TIds& aIds);
	inline TUint32 GetKernelConfigFlags();
	inline void SetKernelConfigFlags(TUint32 aNewFlags);
	inline void SetDisabledCapabilities0(TUint32 aNewWord0);
	inline void KernelTestData(TUint32*& aAddress, TUint32& aData);
	inline void GetSecureInfos(TSecurityInfo* aThreadSecureInfo, TSecurityInfo* aProcessSecureInfo);
	};

#ifndef __KERNEL_MODE__
inline TInt RLddTest::OpenLocal()
	{ return DoCreate(_L("D_SLDD"),TVersion(0,1,1),KNullUnit,NULL,NULL); }
inline TInt RLddTest::OpenProtected()
	{ return DoCreate(_L("D_SLDD"),TVersion(0,1,1),KNullUnit,NULL,NULL,EOwnerThread,ETrue); }
inline TInt RLddTest::Test1()
	{ return DoControl(EControlTest1); }
inline void RLddTest::GetIds(RLddTest::TIds& aIds)
	{ DoControl(EGetIds,&aIds); }
inline TUint32 RLddTest::GetKernelConfigFlags()
	{ return DoControl(EGetKernelConfigFlags); }
inline void RLddTest::SetKernelConfigFlags(TUint32 aNewFlags)
	{ DoControl(ESetKernelConfigFlags, (TAny*)aNewFlags); }
inline void RLddTest::SetDisabledCapabilities0(TUint32 aNewWord0)
	{ DoControl(ESetDisabledCapabilities0, (TAny*)aNewWord0); }
inline void RLddTest::KernelTestData(TUint32*& aAddress, TUint32& aData)
	{ DoControl(EKernelTestData, &aAddress,&aData); }
inline void RLddTest::GetSecureInfos(TSecurityInfo* aThreadSecureInfo, TSecurityInfo* aProcessSecureInfo)
	{ DoControl(EGetSecureInfos, aThreadSecureInfo,aProcessSecureInfo); }
#endif

#endif

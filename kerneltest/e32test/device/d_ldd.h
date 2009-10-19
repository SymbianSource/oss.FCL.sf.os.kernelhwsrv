// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\d_ldd.h
// 
//

#if !defined(__D32LDD_H__)
#define __D32LDD_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"Test");
_LIT(KKInstallLddName,"TestKInstall");

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
		EControlTest2=1,
		EControlTest3=2,
		EControlTest4=3,
		EControlTest5=4,
		EControlTest6=5,
		EControlTest7=6,
		EControlTest8=7,
		EControlTest9=8,
		EControlTestKInstall=9,
		EControlLinkedTest1=10,
		EControlLinkedTest2=11,
		EControlLinkedTest3=12,
		EControlLinkedTest4=13,
		EControlLinkedTest5=14,
		EControlLinkedTest6=15,
		EControlLinkedTest7=16,
		EControlLinkedTest8=17,
		EControlLinkedTest9=18,
		};
	enum TRequest
		{
		};
public:
	inline TInt Open();
	inline TInt Test1();
	inline TInt Test2();
	inline TInt Test3();
	inline TInt Test4();
	inline TInt Test5();
	inline TInt Test6(TInt aValue);
	inline TUint32 Test7();
	inline void Test8(TUint32 aValue);
	inline TInt Test9();
	inline TInt LinkedTest1();
	inline TInt LinkedTest2();
	inline TInt LinkedTest3();
	inline TInt LinkedTest4();
	inline TInt LinkedTest5();
	inline TInt LinkedTest6(TInt aValue);
	inline TUint32 LinkedTest7();
	inline void LinkedTest8(TUint32 aValue);
	inline TInt LinkedTest9();
	inline TInt TestKInstall();
	static inline TInt UnloadAndWait();
	static inline TInt Unload();
	};

class RKInstallTest : public RBusLogicalChannel
	{
public:
	inline TInt Open();
	};

#include "d_ldd.inl"
#endif


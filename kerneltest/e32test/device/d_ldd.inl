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
// e32test\device\d_ldd.inl
// 
//

#ifndef __KERNEL_MODE__

#include <e32svr.h>
#include <u32hal.h>

inline TInt RLddTest::Open()
	{ return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL,NULL); }
inline TInt RLddTest::Test1()
	{ return DoControl(EControlTest1); }
inline TInt RLddTest::Test2()
	{ return DoControl(EControlTest2); }
inline TInt RLddTest::Test3()
	{ return DoControl(EControlTest3); }
inline TInt RLddTest::Test4()
	{ return DoControl(EControlTest4); }
inline TInt RLddTest::Test5()
	{ return DoControl(EControlTest5); }
inline TInt RLddTest::Test6(TInt aValue)
	{ return DoControl(EControlTest6, (TAny*)aValue); }
inline TUint32 RLddTest::Test7()
	{ return (TUint32)DoControl(EControlTest7); }
inline void RLddTest::Test8(TUint32 aValue)
	{ DoControl(EControlTest8, (TAny*)aValue); }
inline TInt RLddTest::Test9()
	{ return DoControl(EControlTest9); }
inline TInt RLddTest::LinkedTest1()
	{ return DoControl(EControlLinkedTest1); }
inline TInt RLddTest::LinkedTest2()
	{ return DoControl(EControlLinkedTest2); }
inline TInt RLddTest::LinkedTest3()
	{ return DoControl(EControlLinkedTest3); }
inline TInt RLddTest::LinkedTest4()
	{ return DoControl(EControlLinkedTest4); }
inline TInt RLddTest::LinkedTest5()
	{ return DoControl(EControlLinkedTest5); }
inline TInt RLddTest::LinkedTest6(TInt aValue)
	{ return DoControl(EControlLinkedTest6, (TAny*)aValue); }
inline TUint32 RLddTest::LinkedTest7()
	{ return (TUint32)DoControl(EControlLinkedTest7); }
inline void RLddTest::LinkedTest8(TUint32 aValue)
	{ DoControl(EControlLinkedTest8, (TAny*)aValue); }
inline TInt RLddTest::LinkedTest9()
	{ return DoControl(EControlLinkedTest9); }
inline TInt RLddTest::TestKInstall()
	{ return DoControl(EControlTestKInstall); }
inline TInt RLddTest::Unload()
	{ return User::FreeLogicalDevice(KLddName); }
inline TInt RKInstallTest::Open()
	{ return DoCreate(KKInstallLddName,TVersion(0,1,1),KNullUnit,NULL,NULL); }
inline TInt RLddTest::UnloadAndWait()
	{
	TInt r = User::FreeLogicalDevice(KLddName);
	if (r == KErrNone)
		r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, (TAny*)5000, 0);
	return r;
	}
#endif

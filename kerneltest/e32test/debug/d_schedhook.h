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
// e32test\debug\d_schedhook.h
// 
//

#ifndef __D_SCHEDHOOK_H__
#define __D_SCHEDHOOK_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif
#include <kernel/arm/arm_types.h>

class TCapsTestV01
	{
public:
	TVersion	iVersion;
	};

class RSchedhookTest : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EInstall,
		EUninstall,
		EInsertHooks,
		ERemoveHooks,
		EEnableCallback,
		EDisableCallback,
		ESetTestThread,
		EGetTestCount,
		EGetThreadContext,
		};
	enum TRequest
		{
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open();
	inline TInt Install();
	inline TInt Uninstall();
	inline TInt InsertHooks();
	inline TInt RemoveHooks();
	inline TInt EnableCallback();
	inline TInt DisableCallback();
	inline TInt SetTestThread(TThreadId aThreadId);
	inline TInt GetTestCount();
	inline TInt GetThreadContext(TThreadId aThreadId,TDes8& aContext);
#endif
	};

#ifndef __KERNEL_MODE__
inline TInt RSchedhookTest::Open()
	{ return DoCreate(_L("D_SCHEDHOOK"),TVersion(0,1,1),KNullUnit,NULL,NULL); }
inline TInt RSchedhookTest::Install()
	{ return DoControl(EInstall); }
inline TInt RSchedhookTest::Uninstall()
	{ return DoControl(EUninstall); }
inline TInt RSchedhookTest::InsertHooks()
	{ return DoControl(EInsertHooks); }
inline TInt RSchedhookTest::RemoveHooks()
	{ return DoControl(ERemoveHooks); }
inline TInt RSchedhookTest::EnableCallback()
	{ return DoControl(EEnableCallback); }
inline TInt RSchedhookTest::DisableCallback()
	{ return DoControl(EDisableCallback); }
inline TInt RSchedhookTest::SetTestThread(TThreadId aThreadId)
	{ return DoControl(ESetTestThread,(TAny*)(TUint)aThreadId); }
inline TInt RSchedhookTest::GetTestCount()
	{ return DoControl(EGetTestCount); }
inline TInt RSchedhookTest::GetThreadContext(TThreadId aThreadId,TDes8& aContext)
	{ return DoControl(EGetThreadContext,(TAny*)(TUint)aThreadId,&aContext); }
#endif

#endif


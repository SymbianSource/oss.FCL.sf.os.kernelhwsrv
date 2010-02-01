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
// f32test\loader\d_ldrtst.h
// 
//

#ifndef __D_LDRTST_H__
#define __D_LDRTST_H__
#include <e32ldr.h>
#include <e32ldr_private.h>

_LIT(KLdrTestLddName, "LdrTest");


#ifndef TMODULEHANDLE_DEFINED
#define TMODULEHANDLE_DEFINED
class TModuleHandle_dummy_class;
typedef TModuleHandle_dummy_class* TModuleHandle;
#endif

/**
@internalComponent
*/
class TCapsTestV01
	{
public:
	TVersion	iVersion;
	};

/**
@internalComponent
*/
class RLdrTest : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlGetCodeSegInfo,
		EControlProcessCodeSeg,
		EControlLibraryCodeSeg,
		EControlModuleCodeSeg,
		EControlGetCodeSegList,
		EControlCodeSegFromAddr,
		EControlModuleHandleFromAddr,
		EControlProcessSMPUnsafeCount,
		};

/**
@internalComponent
*/
	struct SEntry
		{
		TAny* iHandle;
		TUint32 iUid3;
		};
public:
	inline TInt Open();
	inline TInt GetCodeSegInfo(TAny* aHandle, TCodeSegCreateInfo& aInfo);
	inline TAny* ProcessCodeSeg(TInt aProcessHandle);
	inline TAny* LibraryCodeSeg(TInt aLibraryHandle);
	inline TAny* ModuleCodeSeg(TModuleHandle aModuleHandle);
	inline TInt GetCodeSegList(SEntry* aList, TInt aMax);
	inline TAny* CodeSegFromAddr(TLinAddr aAddr);
	inline TModuleHandle ModuleHandleFromAddr(TLinAddr aAddr);
	inline TInt ProcessSMPUnsafeCount(TInt aProcessHandle);
	};

#ifndef __KERNEL_MODE__
inline TInt RLdrTest::Open()
	{ return DoCreate(KLdrTestLddName, TVersion(0,1,1), KNullUnit, NULL, NULL); }
inline TInt RLdrTest::GetCodeSegInfo(TAny* aHandle, TCodeSegCreateInfo& aInfo)
	{ return DoControl(EControlGetCodeSegInfo, aHandle, &aInfo); }
inline TAny* RLdrTest::ProcessCodeSeg(TInt aProcessHandle)
	{ return (TAny*)DoControl(EControlProcessCodeSeg, (TAny*)aProcessHandle); }
inline TAny* RLdrTest::LibraryCodeSeg(TInt aLibraryHandle)
	{ return (TAny*)DoControl(EControlLibraryCodeSeg, (TAny*)aLibraryHandle); }
inline TAny* RLdrTest::ModuleCodeSeg(TModuleHandle aModuleHandle)
	{ return (TAny*)DoControl(EControlModuleCodeSeg, (TAny*)aModuleHandle); }
inline TInt RLdrTest::GetCodeSegList(SEntry* aList, TInt aMax)
	{ return DoControl(EControlGetCodeSegList, aList, (TAny*)aMax); }
inline TAny* RLdrTest::CodeSegFromAddr(TLinAddr aAddr)
	{ return (TAny*)DoControl(EControlCodeSegFromAddr, (TAny*)aAddr); }
inline TModuleHandle RLdrTest::ModuleHandleFromAddr(TLinAddr aAddr)
	{ return (TModuleHandle)DoControl(EControlModuleHandleFromAddr, (TAny*)aAddr); }
inline TInt RLdrTest::ProcessSMPUnsafeCount(TInt aProcessHandle)
	{ return DoControl(EControlProcessSMPUnsafeCount, (TAny*)aProcessHandle); }
#endif

#endif


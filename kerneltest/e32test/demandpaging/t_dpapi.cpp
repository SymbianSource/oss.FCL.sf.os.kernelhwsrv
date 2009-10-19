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
// e32test\demandpaging\t_dpapi.cpp
// 
//

//
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <dptest.h>
#include <e32hal.h>
#include <u32exec.h>
#include <e32svr.h>
#include <e32panic.h>
#include "u32std.h"

#include "t_dpcmn.h"

RTest test(_L("T_DPAPI"));

TInt TestingTChunkCreate();
TInt TestingTChunkHeapCreate();
TInt TestingTThreadCreate();

void TestGlobalConfig()
	{
	test_Equal(gDataPagingSupported, gDataPagingPolicy != EKernelConfigDataPagingPolicyNoPaging);
	}

enum TPagedSetting
	{
	EDefault,
	EPaged,
	EUnpaged
	};

TPagedSetting GetMmpPagedSetting()
	{
	// t_dpapi suffixes:
	//   c => ram loaded code
	//   p => pageddata
	//   u => unpageddata
	
	TFileName name = RProcess().FileName();
	test.Printf(_L("%S\n"), &name);
	TInt pos = name.LocateReverse('\\');
	test(pos >= 0 && pos < (name.Length() - 1));
	TPtrC leaf = name.Mid(pos + 1);
	if (leaf == _L("t_dpapi_p.exe") || leaf == _L("t_dpapi_cp.exe"))
		return EPaged;
	else if (leaf == _L("t_dpapi_u.exe") || leaf == _L("t_dpapi_cu.exe"))
		return EUnpaged;
	test(leaf == _L("t_dpapi.exe") || leaf == _L("t_dpapi_c.exe"));
	return EDefault;
	}

TPagedSetting ExpectedProcessPagedSetting(TPagedSetting aMmpPagedSetting)
	{
	switch (gDataPagingPolicy)
		{
		case EKernelConfigDataPagingPolicyAlwaysPage:
			return EPaged;

		case EKernelConfigDataPagingPolicyNoPaging:
			return EUnpaged;

		case EKernelConfigDataPagingPolicyDefaultUnpaged:
			return aMmpPagedSetting == EDefault ? EUnpaged : aMmpPagedSetting;
			
		case EKernelConfigDataPagingPolicyDefaultPaged:
			return aMmpPagedSetting == EDefault ? EPaged : aMmpPagedSetting;

		default:
			test(EFalse);
		}
	return EDefault;
	}
	
void TestMmpFileDataPagedKeyword()
	{
	TPagedSetting expected = ExpectedProcessPagedSetting(GetMmpPagedSetting());
	TPagedSetting actual = gProcessPaged ? EPaged : EUnpaged;
	test_Equal(expected, actual);
	}

TInt E32Main()
	{
	test.Title();
	test_KErrNone(GetGlobalPolicies());

	test.Start(_L("Test global datapaging configuration"));
	TestGlobalConfig();
	
	test.Next(_L("Test mmp file data paged keyword"));
	TestMmpFileDataPagedKeyword();
	
	test.Next(_L("TestingTChunkCreate"));
	TestingTChunkCreate();
	
	test.Next(_L("TestingTThreadCreate"));
	TestingTThreadCreate();
	
	test.Next(_L("TestingTChunkHeapCreate"));
	TestingTChunkHeapCreate();
	
	test.End();
	return 0;
	}

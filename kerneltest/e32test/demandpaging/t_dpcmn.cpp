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
// e32test\demandpaging\t_dpcmn.cpp
// 
//

#include <e32test.h>
#include <dptest.h>
#include <e32hal.h>
#include <u32exec.h>
#include <e32svr.h>
#include <e32panic.h>
#include "u32std.h"

TInt gPageSize;
TBool gPagingSupported = EFalse;
TBool gRomPagingSupported = EFalse;
TBool gCodePagingSupported = EFalse;
TBool gDataPagingSupported = EFalse;
TInt gDataPagingPolicy;
TBool gProcessPaged;

TBool gGlobalNoPaging = EFalse;		
TBool gGlobalAlwaysPage = EFalse;
TBool gGlobalDefaultUnpaged = EFalse;	
TBool gGlobalDefaultPage = EFalse;

// Size of paging cache in pages
TUint gMinCacheSize = 0;
TUint gMaxCacheSize = 0;
TUint gCurrentCacheSize = 0;

RChunk gChunk;

//
// GetGlobalPolicies
//
// Determine the global dapaging policies as specified at rombuild time
//
TInt GetGlobalPolicies()
	{
	// Determine which types of paging are supported
	TUint32 attrs = DPTest::Attributes();

	gRomPagingSupported = (attrs & DPTest::ERomPaging) != 0;
	gCodePagingSupported = (attrs & DPTest::ECodePaging) != 0;
	gDataPagingSupported = (attrs & DPTest::EDataPaging) != 0;
	gPagingSupported = gRomPagingSupported || gCodePagingSupported || gDataPagingSupported;
	
	if (gRomPagingSupported)
		RDebug::Printf("Rom paging supported");
	if (gCodePagingSupported)
		RDebug::Printf("Code paging supported");
	if (gDataPagingSupported)
		RDebug::Printf("Data paging supported");	
	
	// Determine the data paging attributes
	TInt kernelFlags = UserSvr::HalFunction(EHalGroupKernel, EKernelHalConfigFlags, 0, 0);
	if (kernelFlags < 0)
		return kernelFlags;
	
	gDataPagingPolicy = kernelFlags & EKernelConfigDataPagingPolicyMask;
	if (gDataPagingPolicy == EKernelConfigDataPagingPolicyNoPaging)
		{
		RDebug::Printf("Global NO PAGING policy is set");
		gGlobalNoPaging = ETrue;
		}
	if (gDataPagingPolicy == EKernelConfigDataPagingPolicyAlwaysPage)
		{
		RDebug::Printf("Global ALWAYS PAGE policy is set");
		gGlobalAlwaysPage = ETrue;
		}
	if (gDataPagingPolicy == EKernelConfigDataPagingPolicyDefaultUnpaged)
		{
		RDebug::Printf("Global DEFAULT UNPAGED policy is set");
		gGlobalDefaultUnpaged = ETrue;
		}
	if (gDataPagingPolicy == EKernelConfigDataPagingPolicyDefaultPaged)
		{
		RDebug::Printf("Global DEFAULT PAGED policy is set");
		gGlobalDefaultPage = ETrue;
		}

	// Determine this process' data paging attribute
	RProcess process;	// Default to point to current process.
	gProcessPaged = process.DefaultDataPaged();
	RDebug::Printf(gProcessPaged ? "This process is paged" : "This process is not paged");

	// Get page size
	TInt r = UserHal::PageSizeInBytes(gPageSize);
	if (r != KErrNone)
		return r;

	if (gPagingSupported)
		{
		TUint minSize;
		TUint maxSize;
		TUint currentSize;
		r = DPTest::CacheSize(minSize, maxSize, currentSize);
		if (r != KErrNone)
			return r;

		gMinCacheSize = minSize / gPageSize;
		gMaxCacheSize = maxSize / gPageSize;
		gCurrentCacheSize = currentSize / gPageSize;
		RDebug::Printf("Cache size (pages): min == %d, max == %d, current == %d",
					   gMinCacheSize, gMaxCacheSize, gCurrentCacheSize);
		}
	
	return r;
	}

//
// IsDataPagingSupported
//
// Determine whether data paging is supported
//
TBool IsDataPagingSupported()
	{
	return gDataPagingSupported;
	}

//
// UpdatePaged
//
// Update whether the expected paged status based on the global paging policy.
//
void UpdatePaged(TBool& aPaged)
	{
	if (gGlobalNoPaging)
		aPaged = EFalse;
	if (gGlobalAlwaysPage)
		aPaged = ETrue;
	if (!gDataPagingSupported)
		aPaged = EFalse;
	}


// 
// TestThreadExit
//
// Resume the specified thread and verify the exit reason.
//
TInt TestThreadExit(RThread& aThread, TExitType aExitType, TInt aExitReason)
	{
	// Disable JIT debugging.
	TBool justInTime=User::JustInTime();
	User::SetJustInTime(EFalse);

	TRequestStatus status;
	aThread.Logon(status); 
	aThread.Resume();
	User::WaitForRequest(status);
	if (aExitType != aThread.ExitType())
		return KErrGeneral;

	if (aExitReason != status.Int())
		return KErrGeneral;
	
	if (aExitReason != aThread.ExitReason())
		return KErrGeneral;


	if (aExitType == EExitPanic)
		{
		if (aThread.ExitCategory()!=_L("USER"))
			{
			return KErrGeneral;
			}
		}
			
	CLOSE_AND_WAIT(aThread);

	// Put JIT debugging back to previous status.
	User::SetJustInTime(justInTime);
	return KErrNone;
	}

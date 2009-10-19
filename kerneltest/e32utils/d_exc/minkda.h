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
// e32utils\d_exc\minkda.h
// API exposed by example of kernel-side debug agent.
// 
//

#ifndef __MINKDA_H__
#define __MINKDA_H__

_LIT(KKdaLddName, "MINKDA");

inline TVersion KKdaLddVersion() { return TVersion(1, 0, 1); }


#if defined(__MARM__)

/** 
 ARM user registers.
 Size must be multiple of 4 bytes.
 */

class TDbgRegSet
	{
public:
	enum { KRegCount = 16 };
	TUint32 iRn[KRegCount];
	TUint32 iCpsr;
	};


/** 
 ARM-specific exception-related data.
 Size must be multiple of 4 bytes.
 */ 

class TDbgCpuExcInfo
	{
public:
	enum TExcCode 
		{ 
		EPrefetchAbort=0,
		EDataAbort=1,
		EUndefinedOpcode=2,
		};
public:
	TExcCode iExcCode;
	/** Point to instruction which caused exception */
	TUint32 iFaultPc;
	/** 
     Address which caused exception (System Control Coprocessor Fault
	 Address Register)
	*/
	TUint32 iFaultAddress;
	/** System Control Coprocessor Fault Status Register */
	TUint32 iFaultStatus;
	/** R13 supervisor mode banked register */
	TUint32 iR13Svc;
	/** R14 supervisor mode banked register */
	TUint32 iR14Svc;
	/** SPSR supervisor mode banked register */
	TUint32 iSpsrSvc;
	};

#else

class TDbgRegSet
	{
	};

class TDbgCpuExcInfo
	{
	};

#endif


/** 
 Exception or panic information block.
 Size must be multiple of 4 bytes.
 */

class TDbgCrashInfo
	{
public:
	enum TType { EException, EPanic };
public:
	TType iType;
	/** ID of thread which crashed */
	TUint iTid;
	/** CPU-specific information (exception only) */
	TDbgCpuExcInfo iCpu;
	};


/**
 Thread information block.
 Size must be multiple of 4 bytes.
 */

class TDbgThreadInfo
	{
public:
	TFullName iFullName;
	/** ID of owning process */
	TUint iPid;
	/** user stack base address */
	TLinAddr iStackBase;
	/** user stack size */
	TInt iStackSize;
	TExitCategoryName iExitCategory;
	TInt iExitReason;
	/** User context */
	TDbgRegSet iCpu;
	};


/** 
 Code Segment Information Block 
 Size must be multiple of 4 bytes.
 */

class TDbgCodeSegInfo
	{
public:
	TPath iPath;
	TUint32 iCodeBase;
	TUint32 iCodeSize;
	};


/** 
 API exposed by minimal kernel debug agent.

 This API is provided as an example only.  There are no guarantees of
 binary/source compatibility.
 */

class RMinKda : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ETrap,
		ECancelTrap,
		EKillCrashedThread,
		EGetThreadInfo,
		EReadMem,
		EGetCodeSegs,
		EGetCodeSegInfo,
		};
	// Size of following structs must be multiple of 4 bytes.
	struct TCodeSnapshotParams
		{
		TUint iPid;
		TAny** iHandles;
		TInt* iCountPtr;
		};
	struct TCodeInfoParams
		{
		TUint iPid;
		TAny* iHandle;
		TDes8* iPathPtr;
		TUint32 iCodeBase;
		TUint32 iCodeSize;
		};
	struct TReadMemParams
		{
		TUint iTid;
		TLinAddr iAddr;
		TDes8* iDes;
		};
	struct TDbgInfoParams
		{
		TBuf8<KMaxFullName> iFullName;
		TUint iPid;
		TLinAddr iStackBase;
		TInt iStackSize;
		TBuf8<KMaxExitCategoryName> iExitCategory;
		TInt iExitReason;
		TDbgRegSet iCpu;
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open();
	inline void Trap(TRequestStatus& aStatus, TDbgCrashInfo& aInfo);
	inline void CancelTrap();
	inline void KillCrashedThread();
	inline TInt GetThreadInfo(TUint aTid, TDbgThreadInfo& aInfo);
	inline TInt ReadMem(TUint aTid, TLinAddr aSrc, TDes8& aDest);
	inline TInt GetCodeSegs(TUint aPid, TAny** aHandleArray, TInt& aHandleCount);
	inline TInt GetCodeSegInfo(TAny* aHandle, TUint aPid, TDbgCodeSegInfo& aInfo);
#endif
	};


#ifndef __KERNEL_MODE__

inline TInt RMinKda::Open()
	{
	return DoCreate(KKdaLddName, KKdaLddVersion(), KNullUnit, NULL, NULL, EOwnerThread);
	}

/** 
 Ask to be notified of the next panic or exception occurence.

 The crashed thread will remain suspended until KillCrashedThread() is
 called allowing more information to be gathered (e.g. stack content).

 If more threads panic or take an exception, they will be suspended
 too until the first one is killed.  

 @param aStatus Request to complete when panic/exception occurs
 @param aInfo 	Filled when request completes
 */

inline void RMinKda::Trap(TRequestStatus& aStatus, TDbgCrashInfo& aInfo)
	{
	DoControl(ETrap, &aStatus, &aInfo);
	}

/** Cancel previous call to Trap() */

inline void RMinKda::CancelTrap()
	{
	DoControl(ECancelTrap, NULL, NULL);
	}

/** 
 Kill the thread which crashed causing the Trap() request to complete.
 */

inline void RMinKda::KillCrashedThread()
	{
	DoControl(EKillCrashedThread, NULL, NULL);
	}


/**
 Return information about thread identified by its ID.
 */

inline TInt RMinKda::GetThreadInfo(TUint aTid, TDbgThreadInfo& aInfo)
	{
	TDbgInfoParams params;
	TInt r=DoControl(EGetThreadInfo, (TAny*)aTid, &params);
	if (r==KErrNone)
		{
		aInfo.iFullName.Copy(params.iFullName);
		aInfo.iPid = params.iPid;
		aInfo.iStackBase = params.iStackBase;
		aInfo.iStackSize = params.iStackSize;
		aInfo.iExitCategory.Copy(params.iExitCategory);
		aInfo.iExitReason = params.iExitReason;
		aInfo.iCpu = params.iCpu;
		}
	return r;
	}

/**
 Read memory from designated thread address space.

 @param aTid	Thread id
 @param aSrc	Source address
 @param aDest	Destination descriptor.  Number of bytes to read is
 				aDes.MaxSize().
 
 @return standard error code
 */

inline TInt RMinKda::ReadMem(TUint aTid, TLinAddr aSrc, TDes8& aDest)
	{
	TReadMemParams params;
	params.iTid = aTid;
	params.iAddr = aSrc;
	params.iDes = &aDest;
	return DoControl(EReadMem, &params, NULL);
	}

inline TInt RMinKda::GetCodeSegs(TUint aPid, TAny** aHandleArray, TInt& aHandleCount)
	{
	TCodeSnapshotParams params;
	params.iPid = aPid;
	params.iHandles = aHandleArray;
	params.iCountPtr = &aHandleCount;
	return DoControl(EGetCodeSegs, &params, NULL);
	}

inline TInt RMinKda::GetCodeSegInfo(TAny* aHandle, TUint aPid, TDbgCodeSegInfo& aInfo)
	{
	TBuf8<KMaxPath> path;
	TCodeInfoParams params;
	params.iPid = aPid;
	params.iHandle = aHandle;
	params.iPathPtr = &path;
	TInt r = DoControl(EGetCodeSegInfo, &params, NULL);
	if (r == KErrNone)
		{
		aInfo.iCodeBase = params.iCodeBase;
		aInfo.iCodeSize = params.iCodeSize;
		aInfo.iPath.Copy(path);
		}
	return r;
	}

#endif

#endif

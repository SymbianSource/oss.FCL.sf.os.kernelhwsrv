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
// e32test\mmu\d_shadow.h
// 
//

#if !defined(__D_SHADOW_H__)
#define __D_SHADOW_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif


enum TMemModel
	{
	EMemModelOther,
	EMemModelMoving,
	EMemModelMultiple,
	EMemModelFlexible
	};
	
enum TCpu
	{
	ECpuUnknown,
	ECpuArm,
	ECpuX86
	};


class TCapsShadowV01
	{
public:
	TVersion	iVersion;
	};

	const TUint KGlobalPageDirectory=0xFFFFFFFF;
	const TUint KErrNoPageTable = 0x80000000;
	const TUint KPageOffsetMask = 0x7FFFFFFF;

class RShadow : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlAllocShadow,
		EControlFreeShadow,
		EControlWriteShadow,
		EControlFreezeShadow,
		EControlSetPriority,
		EControlRead,
		EControlMmuId,
		EControlCacheType,
		EControlMeasureKernStackUse,
		EControlMeasureKernHeapFree,
		EControlWalkHeap,
		EControlCallFunction,
		EControlAllocPhys,
		EControlFreePhys,
		EControlClaimPhys,
		EControlGetMemoryArchitecture,
		EControlGetMemModelInfo,
		EControlGetPdInfo		
		};
		
public:
	inline TInt Open();
	inline TInt Alloc(TUint anAddr);				// allocate a shadow ROM page
	inline TInt Free(TUint anAddr);					// free a shadow ROM page
	inline TInt Write(TUint anAddr, TAny* aSrc);	// write to a shadow ROM page
	inline TInt Freeze(TUint anAddr);				// freeze a shadow ROM page
	inline TInt SetPriority(TInt aHandle, TInt aPriority);
	inline TUint Read(TUint aLinAddr);
	inline TUint MmuId();
	inline TUint CacheType();
	inline TInt KernStackUsed();
	inline TInt KernHeapFree();
	inline void WalkHeap(TInt aThreadHandle);
	inline TInt CallFunction(TThreadFunction aFunction, TAny* aPtr);
	inline TInt AllocPhysicalRam(TUint32& aAddr, TInt aSize, TInt aAlign);
	inline TInt FreePhysicalRam(TUint32 aAddr, TInt aSize);
	inline TInt ClaimPhysicalRam(TUint32 aAddr, TInt aSize);
	inline void GetMemoryArchitecture(TCpu &aCpu, TUint &aCR);
	inline TMemModel GetMemModelInfo(TUint &aPageTable, TUint &aNumPds);
	inline TInt GetPdInfo(TUint aPdNo, TUint &aPdSize, TUint &aPdBase, TUint &aOffset);
	};


#ifndef __KERNEL_MODE__
inline TInt RShadow::Open()
	{
	return DoCreate(_L("Shadow"),TVersion(0,1,1),KNullUnit,NULL,NULL);
	}

inline TInt RShadow::Alloc(TUint anAddr)
	{ return DoControl(EControlAllocShadow,(TAny*)anAddr); }

inline TInt RShadow::Free(TUint anAddr)
	{ return DoControl(EControlFreeShadow,(TAny*)anAddr); }

inline TInt RShadow::Write(TUint anAddr, TAny *aSrc)
	{ return DoControl(EControlWriteShadow,(TAny*)anAddr,aSrc); }

inline TInt RShadow::Freeze(TUint anAddr)
	{ return DoControl(EControlFreezeShadow,(TAny*)anAddr); }

inline TInt RShadow::SetPriority(TInt aHandle, TInt aPriority)
	{ return DoControl(EControlSetPriority, (TAny*)aHandle, (TAny*)aPriority); }

inline TUint RShadow::Read(TUint aLinAddr)
	{ return DoControl(EControlRead, (TAny*)aLinAddr); }

inline TUint RShadow::MmuId()
	{ return DoControl(EControlMmuId); }

inline TUint RShadow::CacheType()
	{ return DoControl(EControlCacheType); }

inline TInt RShadow::KernStackUsed()
	{ return DoControl(EControlMeasureKernStackUse); }

inline TInt RShadow::KernHeapFree()
	{ return DoControl(EControlMeasureKernHeapFree); }

inline void RShadow::WalkHeap(TInt aThreadHandle)
	{ DoControl(EControlWalkHeap,(TAny*)aThreadHandle); }

inline TInt RShadow::CallFunction(TThreadFunction aFunction, TAny* aPtr)
	{ return DoControl(EControlCallFunction, (TAny*)aFunction, aPtr); }

inline TInt RShadow::AllocPhysicalRam(TUint32& aAddr, TInt aSize, TInt aAlign)
	{ TInt r=DoControl(EControlAllocPhys, (TAny*)aSize, (TAny*)aAlign); if (r>=0) aAddr=TUint32(r)<<4; return r<0?r:KErrNone; }

inline TInt RShadow::FreePhysicalRam(TUint32 aAddr, TInt aSize)
	{ return DoControl(EControlFreePhys, (TAny*)aAddr, (TAny*)aSize); }

inline TInt RShadow::ClaimPhysicalRam(TUint32 aAddr, TInt aSize)
	{ return DoControl(EControlClaimPhys, (TAny*)aAddr, (TAny*)aSize); }
	
inline void RShadow::GetMemoryArchitecture(TCpu &aCpu, TUint &aCR)
	{ DoControl(EControlGetMemoryArchitecture, (TAny*) &aCpu, (TAny*) &aCR); }

inline TMemModel RShadow::GetMemModelInfo(TUint &aPageTable, TUint &aNumPds)
	{ return (TMemModel) DoControl(EControlGetMemModelInfo,(TAny*) &aPageTable, (TAny*) &aNumPds); }

inline TInt RShadow::GetPdInfo(TUint aPdNo, TUint &aPdSize, TUint &aPdBase, TUint &aOffset)
	{
	aPdSize=aPdNo;
	TUint ret= (TUint) DoControl(EControlGetPdInfo,(TAny*) &aPdSize, (TAny*) &aPdBase);
	aOffset =  ret & KPageOffsetMask;
	return (ret & KErrNoPageTable)?KErrNotFound:KErrNone;
	}
	
#endif

#endif

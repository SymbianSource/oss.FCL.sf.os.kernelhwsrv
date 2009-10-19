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
// e32test\dma\d_dma.h
// User-side API for LDD used to test DMA framework.
// 
//

#ifndef __D_DMA_H__
#define __D_DMA_H__

#include <e32cmn.h>
#ifndef __KLIB_H__
#include <e32std.h>
#endif

#ifdef __DMASIM__
_LIT(KTestDmaLddName, "TestDmaSim");
#else
_LIT(KTestDmaLddName, "TestDma");
#endif

inline TVersion TestDmaLddVersion() { return TVersion(1, 0, 1); }

class RTestDma : public RBusLogicalChannel
	{
public:
	struct TFragmentInfo
		{
		TInt iRequestIdx;
		TInt iSrcBufIdx;
		TInt iDestBufIdx;
		TInt iSize;
		TRequestStatus* iRs;
		};
	struct TOpenInfo
		{
		enum { EGetInfo, EOpen } iWhat;
		union
			{
			TAny* iInfo;
			struct
				{
				TUint32 iId;
				TInt iDesCount;
				TInt iMaxTransferSize;
				} iOpen;
			} U;
		};
	enum TControl
		{
		EAllocBuffer,
		EFreeAllBuffers,
		EFillBuffer,
		ECheckBuffer,
		EFragment,
		EExecute,
		EFailNext,
		EFragmentCount,
		EMissInterrupts,
		};
	enum { KMaxChannels = 32 };
	struct TInfo
		{
		TInt iMaxTransferSize;
		TUint iMemAlignMask;
		TInt iMaxSbChannels;
		TUint32 iSbChannels[KMaxChannels];
		TInt iMaxDbChannels;
		TUint32 iDbChannels[KMaxChannels];
		TInt iMaxSgChannels;
		TUint32 iSgChannels[KMaxChannels];
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt GetInfo(TInfo& aInfo);
	inline TInt Open(TUint32 aId, TInt aDesCount, TInt aMaxTransferSize);
	inline TInt AllocBuffer(TInt aBufIdx, TInt aSize);
	inline void FreeAllBuffers();
	inline void FillBuffer(TInt aBufIdx, TUint8 aFillValue);
	inline TBool CheckBuffer(TInt aBufIdx, TUint8 aValue);
	inline TInt Fragment(TInt aRequestIdx, TInt aSrcBufIdx, TInt aDestBufIdx, TInt aSize, TRequestStatus* aRs=NULL);
	inline TInt Execute(const TDesC8& aCmd);
	inline TInt FailNext(TInt aFragmentCount);
	inline TInt FragmentCount(TInt aRequestIdx);
	inline TInt MissNextInterrupts(TInt aInterruptCount);
	inline TBool FragmentCheck(TInt aRequestIdx, TInt aExpectedCount);
#endif
	};

#ifndef __KERNEL_MODE__

inline TInt RTestDma::GetInfo(TInfo& aInfo)
	{
	TPckgBuf<TOpenInfo> infoBuf;
	infoBuf().iWhat = TOpenInfo::EGetInfo;
	infoBuf().U.iInfo = &aInfo;
	return DoCreate(KTestDmaLddName, TestDmaLddVersion(), 0, NULL, &infoBuf) == KErrDied ? KErrNone : KErrGeneral;
	}

inline TInt RTestDma::Open(TUint32 aId, TInt aDesCount, TInt aMaxTransferSize)
	{
	TPckgBuf<TOpenInfo> infoBuf;
	infoBuf().iWhat = TOpenInfo::EOpen;
	infoBuf().U.iOpen.iId = aId;
	infoBuf().U.iOpen.iDesCount = aDesCount;
	infoBuf().U.iOpen.iMaxTransferSize = aMaxTransferSize;
	return DoCreate(KTestDmaLddName, TestDmaLddVersion(), 0, NULL, &infoBuf, EOwnerThread);
	}

inline TInt RTestDma::AllocBuffer(TInt aBufIdx, TInt aSize)
	{
	return DoControl(EAllocBuffer, (TAny*)aBufIdx, (TAny*)aSize);
	}

inline void RTestDma::FreeAllBuffers()
	{
	DoControl(EFreeAllBuffers, NULL, NULL);
	}

inline void RTestDma::FillBuffer(TInt aBufIdx, TUint8 aFillValue)
	{
	DoControl(EFillBuffer, (TAny*)aBufIdx, (TAny*)(TUint)aFillValue);
	}

inline TBool RTestDma::CheckBuffer(TInt aBufIdx, TUint8 aValue)
	{
	return DoControl(ECheckBuffer, (TAny*)aBufIdx, (TAny*)(TUint)aValue);
	}

inline TInt RTestDma::Fragment(TInt aRequestIdx, TInt aSrcBufIdx, TInt aDestBufIdx, TInt aSize, TRequestStatus* aRs)
	{
	TFragmentInfo info;
	info.iRequestIdx = aRequestIdx;
	info.iSrcBufIdx = aSrcBufIdx;
	info.iDestBufIdx = aDestBufIdx;
	info.iSize = aSize;
	info.iRs = aRs;
	if (aRs != NULL)
		*aRs = KRequestPending;
	return DoControl(EFragment, &info, NULL);
	}

inline TInt RTestDma::Execute(const TDesC8& aCmd)
	{
	return DoControl(EExecute, (TDesC8*)&aCmd, NULL);
	}

inline TInt RTestDma::FailNext(TInt aFragmentCount)
	{
	return DoControl(EFailNext, (TAny*)aFragmentCount, NULL);
	}

inline TInt RTestDma::FragmentCount(TInt aRequestIdx)
	{
	return DoControl(EFragmentCount, (TAny*)aRequestIdx, NULL);
	}

inline TInt RTestDma::MissNextInterrupts(TInt aInterruptCount)
	{
	return DoControl(EMissInterrupts, (TAny*)aInterruptCount, NULL);
	}

/**
@param The number of fragments expected, 0 means don't check
*/
inline TBool RTestDma::FragmentCheck(TInt aRequestIdx, TInt aExpectedCount)
	{
	if(aExpectedCount)
		{
		TInt actualCount = FragmentCount(aRequestIdx);
		if(actualCount == aExpectedCount)
			return ETrue;
		else
			{
			RDebug::Printf("Fragment count error: expected %d, got %d", aExpectedCount, actualCount);
			return EFalse;
			}
		}
	else
		return ETrue;
	}
#endif // #ifndef __KERNEL_MODE__

#endif

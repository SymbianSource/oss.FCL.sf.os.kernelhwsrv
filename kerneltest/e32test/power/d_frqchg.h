// Copyright (c) 2010-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\power\d_frqchg.h
// 
//

#if !defined(__D_FRQCHG_H__)
#define __D_FRQCHG_H__

#include <e32cmn.h>

struct SRatio;
struct SRatioInv;

#ifndef __KERNEL_MODE__
#include <e32std.h>

struct SRatio
	{
	TUint32		iM;		// mantissa, normalised so bit 31=1
	TInt16		iX;		// -exponent.
	TUint8		iSpare1;
	TUint8		iSpare2;
	};

struct SRatioInv
	{
	SRatio		iR;
	SRatio		iI;
	};
#endif

_LIT(KLddName,"D_FRQCHG.LDD");

class RFrqChg : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControl_RatioSet,
		EControl_RatioReciprocal,
		EControl_RatioMult,
		EControl_RatioInvSet,
		EControl_FrqChgTestPresent,
		EControl_SetCurrentThreadPriority,
		EControl_SetCurrentThreadCpu,
		EControl_SetCurrentThreadTimeslice,
		EControl_SetLocalTimerPrescaler,
		EControl_ReadGlobalTimerAndTimestamp,
		EControl_SetGlobalTimerPrescaler,
		ENumControls
		};

public:
	inline TInt Open();
	inline TInt RatioSet(SRatio& aRatio, TUint32 aInt, TInt aDivisorExp=0);
	inline TInt RatioReciprocal(SRatio& aRatio);
	inline TInt RatioMult(SRatio& aRatio, TUint32& aInt32);
	inline TInt RatioInvSet(SRatioInv& aRI, const SRatio* aR);
	inline TInt FrqChgTestPresent();
	inline TInt SetCurrentThreadPriority(TInt aPri);
	inline TInt SetCurrentThreadCpu(TUint32 aCpu, TUint32* aOldAffinity = NULL);
	inline TInt SetCurrentThreadTimeslice(TInt aSlice);
	inline TInt SetLocalTimerPrescaler(TUint32 aCpus, TInt aPrescale);
	inline TInt ReadGlobalTimerAndTimestamp(TUint64& aTimerValue, TUint64& aTimestamp);
	inline TInt SetGlobalTimerPrescaler(TInt aPrescale);
	};

#ifndef __KERNEL_MODE__
inline TInt RFrqChg::Open()
	{ return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL,NULL); }
inline TInt RFrqChg::RatioSet(SRatio& aR, TUint32 aInt, TInt aDivisorExp)
	{ aR.iM=aInt; return DoControl(EControl_RatioSet, (TAny*)&aR, (TAny*)aDivisorExp); }
inline TInt RFrqChg::RatioReciprocal(SRatio& aR)
	{ return DoControl(EControl_RatioReciprocal, (TAny*)&aR); }
inline TInt RFrqChg::RatioMult(SRatio& aR, TUint32& aInt32)
	{ return DoControl(EControl_RatioMult, (TAny*)&aR, (TAny*)&aInt32); }
inline TInt RFrqChg::RatioInvSet(SRatioInv& aRI, const SRatio* aR)
	{ return DoControl(EControl_RatioInvSet, (TAny*)&aRI, (TAny*)aR); }
inline TInt RFrqChg::FrqChgTestPresent()
	{ return DoControl(EControl_FrqChgTestPresent); }
inline TInt RFrqChg::SetCurrentThreadPriority(TInt aPri)
	{ return DoControl(EControl_SetCurrentThreadPriority, (TAny*)aPri); }
inline TInt RFrqChg::SetCurrentThreadCpu(TUint32 aCpu, TUint32* aOldAffinity)
	{ return DoControl(EControl_SetCurrentThreadCpu, (TAny*)aCpu, (TAny*) aOldAffinity); }
inline TInt RFrqChg::SetCurrentThreadTimeslice(TInt aSlice)
	{ return DoControl(EControl_SetCurrentThreadTimeslice, (TAny*)aSlice); }
inline TInt RFrqChg::SetLocalTimerPrescaler(TUint32 aCpus, TInt aPrescale)
	{ return DoControl(EControl_SetLocalTimerPrescaler, (TAny*)aCpus, (TAny*)aPrescale); }
inline TInt RFrqChg::ReadGlobalTimerAndTimestamp(TUint64& aTimerValue, TUint64& aTimestamp)
	{ return DoControl(EControl_ReadGlobalTimerAndTimestamp, (TAny*)&aTimerValue, (TAny*) &aTimestamp); }
inline TInt RFrqChg::SetGlobalTimerPrescaler(TInt aPrescale)
	{ return DoControl(EControl_SetGlobalTimerPrescaler, (TAny*)aPrescale); }
#endif

#endif   //__D_FRQCHG_H__

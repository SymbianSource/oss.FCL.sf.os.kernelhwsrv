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
// e32test\power\d_frqchg.cpp
// LDD for testing frequency changing
// 
//

#include <kernel/kernel.h>
#include "d_frqchg.h"

#if defined(__EPOC32__) && defined(__SMP__) && defined(__MARM__)
#define __SUPPORT_LOCAL_TIMER_PRESCALE__

#include <nk_priv.h>
#include <arm_tmr.h>
#endif


#ifdef __PLATFORM_SUPPORTS_DVFS__
/**
  Baseport needs to supply this function to disable DVFS whilst test is running. 
  The test relies on changing prescalers in local and global timer directly rather than
  actually changing frequency. Consequently DVFS must be disabled when the test is running

  This function when driver is loaded. 
  @return KErrNone if succesful
 */
extern TInt DisableDvfs();

/**
   if plaftorm supports DVFS this function will be called when the driver is unloaded
 */
extern void RestoreDvfs();
#endif



#if defined(__SUPPORT_LOCAL_TIMER_PRESCALE__)
TInt Multiply(SRatio& aDest, const SRatio& aSrc)
	{
	TUint64 x = aDest.iM;
	TUint64 y = aSrc.iM;
	x *= y;
	if (x==0)
		{
		aDest.iM = 0;
		aDest.iX = 0;
		return KErrNone;
		}
	TInt exp = aDest.iX + aSrc.iX + 32;
	if (TInt64(x) >= 0)
		x<<=1, --exp;
	aDest.iM = I64HIGH(x);
	if (I64LOW(x) & 0x80000000u)
		{
		if (++aDest.iM == 0)
			aDest.iM = 0x80000000u, ++exp;
		}
	if (exp > 32767)
		{
		aDest.iM = 0xffffffffu;
		aDest.iX = 32767;
		return KErrOverflow;
		}
	if (exp < -32768)
		{
		aDest.iM = 0;
		aDest.iX = 0;
		return KErrUnderflow;
		}
	aDest.iX = (TInt16)exp;
	return KErrNone;
	}

// Calculate frequency ratio for specified prescale value
// Ratio = (default+1)/(current+1)
void PrescaleRatio(SRatio& aR, TInt aDefault, TInt aCurrent)
	{
	SRatio df;
	df.Set(TUint32(aDefault+1));
	aR.Set(TUint32(aCurrent+1));
	aR.Reciprocal();
	Multiply(aR, df);
	}
#endif

class DFrqChgFactory : public DLogicalDevice
//
// Test LDD factory
//
	{
public:
	DFrqChgFactory();
	virtual ~DFrqChgFactory();
	virtual TInt Install(); 					//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel); 	//overriding pure virtual
	};

class DFrqChg : public DLogicalChannelBase
//
// Test logical channel
//
	{
public:
	virtual ~DFrqChg();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
#if defined(__SUPPORT_LOCAL_TIMER_PRESCALE__)
	void PopulateDefaultPrescaleList();
	void SetLocalTimerPrescaler(TUint32 aCpus, TInt aPrescale);
	TScheduler* iS;
	TInt iDefaultPrescale[KMaxCpus];
#if defined(__CPU_ARM_HAS_GLOBAL_TIMER_BLOCK) && defined( __NKERN_TIMESTAMP_USE_SCU_GLOBAL_TIMER__)
	void SetGlobalTimerPrescaler(TInt aPrescale);
	TInt iDefaultGTPrescale;
#endif
#endif

	};



DECLARE_STANDARD_LDD()
	{
	return new DFrqChgFactory;
	}

//
// Constructor
//
DFrqChgFactory::DFrqChgFactory()
	{
	}

//
// Destructor, called on unload
//
DFrqChgFactory::~DFrqChgFactory()
	{
#ifdef __PLATFORM_SUPPORTS_DVFS__
	RestoreDvfs();
#endif
	}



//
// Create new channel
//
TInt DFrqChgFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DFrqChg;
	return aChannel?KErrNone:KErrNoMemory;
	}


//
// Install the LDD - overriding pure virtual
//
TInt DFrqChgFactory::Install()
	{
#ifdef __PLATFORM_SUPPORTS_DVFS__
	TInt r = DisableDvfs();
	if (KErrNone != r) return r;
#endif
	return SetName(&KLddName);
	}


//
// Get capabilities - overriding pure virtual
//
void DFrqChgFactory::GetCaps(TDes8& /*aDes*/) const
	{
	}


//
// Create channel
//
TInt DFrqChg::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
#if defined(__SUPPORT_LOCAL_TIMER_PRESCALE__)
	iS = (TScheduler*)TScheduler::Ptr();
	PopulateDefaultPrescaleList();
#endif
	return KErrNone;
	}


//
// Destructor
//
DFrqChg::~DFrqChg()
	{
#if defined(__SUPPORT_LOCAL_TIMER_PRESCALE__)
	// restore prescalers
	SetLocalTimerPrescaler((TUint32) -1, -1);
#if defined(__CPU_ARM_HAS_GLOBAL_TIMER_BLOCK) && defined( __NKERN_TIMESTAMP_USE_SCU_GLOBAL_TIMER__)
	SetGlobalTimerPrescaler(-1);
#endif
#endif
	}


TInt DFrqChg::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
	SRatioInv ri;
	TInt r = KErrNone;
	switch (aReqNo)
		{
		case RFrqChg::EControl_RatioSet:
			{
			kumemget32(&ri.iR, a1, sizeof(SRatio));
			ri.iR.Set(ri.iR.iM, (TUint32)a2);
			kumemput32(a1, &ri.iR, sizeof(SRatio));
			break;
			}
		case RFrqChg::EControl_RatioReciprocal:
			{
			kumemget32(&ri.iR, a1, sizeof(SRatio));
			r = ri.iR.Reciprocal();
			kumemput32(a1, &ri.iR, sizeof(SRatio));
			break;
			}
		case RFrqChg::EControl_RatioMult:
			{
			kumemget32(&ri.iR, a1, sizeof(SRatio));
			kumemget32(&ri.iI.iM, a2, sizeof(TUint32));
			r = ri.iR.Mult(ri.iI.iM);
			kumemput32(a2, &ri.iI.iM, sizeof(TUint32));
			break;
			}
		case RFrqChg::EControl_RatioInvSet:
			{
			SRatio ratio;
			const SRatio* p = 0;
			if (a2)
				{
				kumemget32(&ratio, a2, sizeof(SRatio));
				p = &ratio;
				}
			ri.Set(p);
			kumemput32(a1, &ri, sizeof(SRatioInv));
			break;
			}
#if defined(__EPOC32__) && defined(__SMP__) && defined(__MARM__)
		case RFrqChg::EControl_FrqChgTestPresent:
			break;
		case RFrqChg::EControl_SetCurrentThreadPriority:
			NKern::ThreadSetPriority(NKern::CurrentThread(), (TInt)a1);
			break;
		case RFrqChg::EControl_SetCurrentThreadCpu:
			{
			TUint32 old = 0;
			old =  NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), (TUint32)a1);
			if (a2) 
				{
				kumemput32(a2, &old, sizeof(TUint32));
				}
			
			old =  NKern::ThreadSetCpuAffinity(NKern::CurrentThread(), (TUint32)a1);
			}
			
			break;
		case RFrqChg::EControl_SetCurrentThreadTimeslice:
			{
			TInt ts = NKern::TimesliceTicks((TUint32)a1);
			NKern::ThreadSetTimeslice(NKern::CurrentThread(), ts);
			NKern::YieldTimeslice();
			break;
			}
#endif
#if defined(__SUPPORT_LOCAL_TIMER_PRESCALE__)
		case RFrqChg::EControl_SetLocalTimerPrescaler:
			{
			TUint32 cpus = (TUint32)a1;
			TInt prescale = (TInt)a2;
			SetLocalTimerPrescaler(cpus, prescale);
			break;
			}
#if defined(__CPU_ARM_HAS_GLOBAL_TIMER_BLOCK) && defined( __NKERN_TIMESTAMP_USE_SCU_GLOBAL_TIMER__)
	case RFrqChg::EControl_ReadGlobalTimerAndTimestamp:
		    {
			ArmGlobalTimer* tmr = iS->iSX.iGlobalTimerAddr;
			TUint32 highlow[2];
			do
				{
				highlow[1] = tmr->iTimerCountHigh;
				highlow[0] = tmr->iTimerCountLow;
				} while(highlow[1]!=tmr->iTimerCountHigh);
			TUint64 ts = NKern::Timestamp();
			kumemput32(a1,&highlow[0],sizeof(TUint64));
			kumemput32(a2,&ts,sizeof(TUint64));
			break;
		 }
	case RFrqChg::EControl_SetGlobalTimerPrescaler:
		    {
			SetGlobalTimerPrescaler((TInt)a1);
			break;
		 }
#endif
#endif
		default:
			r = KErrNotSupported;
			break;
		}
	return r;
	}


#if defined(__SUPPORT_LOCAL_TIMER_PRESCALE__)
void DFrqChg::PopulateDefaultPrescaleList()
	{
	TInt nc = NKern::NumberOfCpus();
	NThread* nt = NKern::CurrentThread();
	TUint32 aff0 = NKern::ThreadSetCpuAffinity(nt, 0);
	TInt i;
	for (i=0; i<nc; ++i)
		{
		NKern::ThreadSetCpuAffinity(nt, i);
		ArmLocalTimer* tmr = (ArmLocalTimer*)iS->iSX.iLocalTimerAddr;
		TInt pv = (tmr->iTimerCtrl & E_ArmTmrCtrl_PrescaleMask) >> E_ArmTmrCtrl_PrescaleShift;
		iDefaultPrescale[i] = pv;
		}
	NKern::ThreadSetCpuAffinity(nt, aff0);
#if defined(__CPU_ARM_HAS_GLOBAL_TIMER_BLOCK) && defined( __NKERN_TIMESTAMP_USE_SCU_GLOBAL_TIMER__)
	ArmGlobalTimer* tmr = iS->iSX.iGlobalTimerAddr;
	TInt pv = (tmr->iTimerCtrl & E_ArmGTmrCtrl_PrescaleMask) >> E_ArmGTmrCtrl_PrescaleShift;
	iDefaultGTPrescale = pv;
#endif
	}

void DFrqChg::SetLocalTimerPrescaler(TUint32 aCpus, TInt aPrescale)
	{
	TInt nc = NKern::NumberOfCpus();
	NThread* nt = NKern::CurrentThread();
	TUint32 aff0 = NKern::ThreadSetCpuAffinity(nt, 0);
	TInt i;
	for (i=0; i<nc; ++i)
		{
		NKern::ThreadSetCpuAffinity(nt, i);
		}
	for (i=0; i<nc; ++i)
		{
		NKern::ThreadSetCpuAffinity(nt, i);
		TInt pv = aPrescale;
		if (pv < 0)
			pv = iDefaultPrescale[i];
		if (aCpus & (1u<<i))
			{
			TInt irq = NKern::DisableAllInterrupts();
			ArmLocalTimer* tmr = (ArmLocalTimer*)iS->iSX.iLocalTimerAddr;
			tmr->iTimerCtrl = (tmr->iTimerCtrl &~ E_ArmTmrCtrl_PrescaleMask) | ((pv << E_ArmTmrCtrl_PrescaleShift) & E_ArmTmrCtrl_PrescaleMask);
			__e32_io_completion_barrier();
			NKern::RestoreInterrupts(irq);
			}
		}
	NKern::ThreadSetCpuAffinity(nt, aff0);
	if (aCpus & 0x80000000u)
		{
		// notify nanokernel of frequency changes
		SVariantInterfaceBlock* vib = iS->iVIB;
		SRatio ratio[KMaxCpus];
		for (i=0; i<nc; ++i)
			{
			if (aCpus & (1u<<i))
				{
				if (aPrescale<0)
					ratio[i].Set(1);
				else
					PrescaleRatio(ratio[i], iDefaultPrescale[i], aPrescale);
				vib->iTimerFreqR[i] = &ratio[i];
				}
			}
		(*vib->iFrqChgFn)();
		}
	}

#if defined(__CPU_ARM_HAS_GLOBAL_TIMER_BLOCK) && defined( __NKERN_TIMESTAMP_USE_SCU_GLOBAL_TIMER__)
void DFrqChg::SetGlobalTimerPrescaler(TInt aPrescale)
	{
	TInt pv = aPrescale;
	if (pv <= 0)
		pv = iDefaultGTPrescale;

	ArmGlobalTimer* tmr = iS->iSX.iGlobalTimerAddr;
	// TInt irq = NKern::DisableAllInterrupts(); 
	tmr->iTimerCtrl = (tmr->iTimerCtrl &~ E_ArmGTmrCtrl_PrescaleMask) | ((pv << E_ArmGTmrCtrl_PrescaleShift) & E_ArmGTmrCtrl_PrescaleMask);
	__e32_io_completion_barrier();
	// NKern::RestoreInterrupts(irq);

	// notify nanokernel of frequency changes
	SVariantInterfaceBlock* vib = iS->iVIB;
	SRatio ratio;
	
	if (aPrescale<=0)
		ratio.Set(1);
	else
		PrescaleRatio(ratio, iDefaultGTPrescale, aPrescale);

	vib->iGTimerFreqR = &ratio;
	(*vib->iFrqChgFn)();
	}

#endif
#endif


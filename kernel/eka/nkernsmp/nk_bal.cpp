// Copyright (c) 2009-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\nkernsmp\nk_bal.cpp
// 
//

// NThreadBase member data
#define __INCLUDE_NTHREADBASE_DEFINES__

// TDfc member data
#define __INCLUDE_TDFC_DEFINES__

#include "nk_bal.h"

#include "nk_priv.h"
#include "nk_irq.h"

#include <e32cmn.h>

/******************************************************************************
 * Load balancing
 ******************************************************************************/

enum TCCState
	{
	ECCReqPending = 0x80000000u,
	ECCReqDeferred = 0x40000000u,
	ECCPowerUpInProgress = 0x20000000u,
	ECCPowerDownInProgress = 0x10000000u,
	ECCRebalanceRequired = 0x08000000u,
	ECCRebalanceTimerQueued = 0x04000000u,
	ECCPeriodicBalancingActive = 0x02000000u,
	};

const TUint K_CpuMask	= 0x1fu;
const TUint K_Keep		= 0x20u;
const TUint K_SameCpu	= 0x40u;
const TUint K_NewCpu	= 0x80u;
const TUint K_CpuSticky	= 0x40u;
const TUint K_CheckCpu	= 0x100u;

#define	PERCENT(fsd, percent)					(((fsd)*(percent)+50)/100)

const TUint K_LB_HeavyThreshold					= PERCENT(4095, 90);
const TUint K_LB_GravelThreshold_RunAvg			= PERCENT(4095, 1);
const TUint K_LB_GravelThreshold_RunActAvg		= PERCENT(4095, 50);
const TInt	K_LB_HeavyCapacityThreshold			= PERCENT(4095, 1);
const TInt	K_LB_BalanceInterval				= 107;
const TInt	K_LB_CpuLoadDiffThreshold			= 128;

//const TUint K_LB_HeavyStateThreshold			= 128;
const TUint K_LB_HeavyPriorityThreshold			= 25;

inline TBool IsHeavy(NSchedulable* a)
	{
	TUint x = 0xffu ^ a->iLbInfo.iLbHeavy;
	return (x&(x-1))==0;
	}

inline TBool IsNew(NSchedulable* a)
	{ return a->iLbState & NSchedulable::ELbState_PerCpu; }

struct SPerPri : public SDblQue
	{
	inline SPerPri() : iTotalRun(0), iTotalAct(0), iCount(0), iHeavy(0) {}

	TUint32	iTotalRun;
	TUint32	iTotalAct;
	TUint16	iCount;
	TUint16	iHeavy;
	};

struct SCpuAvailability
	{
	enum
		{
		EIdle = 4095,
		EMaxedOut = -268435456,
		EUnavailable = KMinTInt
		};

	void	Init(TUint32 aActive);
	TInt	FindMax() const;
	TInt	FindMax(NSchedulable* aS) const;
	TInt	PickCpu(NSchedulable* aS, TBool aDropped) const;
	TInt	SetMaxed(TInt aCpu);
	void	AddLoad(TInt aCpu, TInt aLoad);
	inline	TInt operator[](TInt aCpu) const
		{	return iRemain[aCpu]; }
	inline	TInt TotalRemain() const
		{	return iTotalRemain; }

	TInt	iRemain[KMaxCpus];
	TInt	iCount;
	TInt	iTotalRemain;
	};

TUint32 HotWarmUnit;
TUint32 LB_DormantThreshold;
volatile TUint32 LBDelayed = 0;

void CalcHotWarm(TUint8& aOut, TUint64 aTime)
	{
	TUint8 out = 0;
	if (aTime>0)
		{
		aTime /= TUint64(HotWarmUnit);
		if (I64HIGH(aTime))
			out = 255;
		else
			{
			aTime *= aTime;
			out = __e32_find_ms1_64(aTime) + 1;
			}
		}
	aOut = (TUint8)out;
	}

void TScheduler::InitLB()
	{
	TScheduler& s = TheScheduler;
	TDfcQue* rbQ = s.iRebalanceDfcQ;
	s.iBalanceTimer.SetDfcQ(rbQ);
	s.iCCReactivateDfc.SetDfcQ(rbQ);
	s.iCCRequestDfc.SetDfcQ(rbQ);
	s.iCCPowerDownDfc.SetDfcQ(rbQ);
	s.iFreqChgDfc.SetDfcQ(rbQ);
	NThreadBase* lbt = rbQ->iThread;
	lbt->iRebalanceAttr = 1;
	TUint32 f = NKern::CpuTimeMeasFreq();
	HotWarmUnit = f / 1000000;
	TUint8 y = 0;
	CalcHotWarm(y, f/5);
	LB_DormantThreshold = y;
	__KTRACE_OPT(KBOOT,DEBUGPRINT("InitLB()"));
	__KTRACE_OPT(KBOOT,DEBUGPRINT("LB_DormantThreshold=%d", LB_DormantThreshold));
	}

void TSubScheduler::GetLbThreads(SDblQue& aQ)
	{
	NKern::Lock();
	iReadyListLock.LockOnly();
	if (!iLbQ.IsEmpty())
		{
		aQ.MoveFrom(&iLbQ);
		iLbCounter ^= NSchedulable::ELbState_Generation;
		}
	iReadyListLock.UnlockOnly();
	NKern::Unlock();
	}

void TScheduler::GetLbThreads(SDblQue& aQ)
	{
	NKern::Lock();
	iBalanceListLock.LockOnly();
	if (!iBalanceList.IsEmpty())
		{
		aQ.MoveFrom(&iBalanceList);
		iLbCounter ^= NSchedulable::ELbState_Generation;
		}
	iBalanceListLock.UnlockOnly();
	NKern::Unlock();
	}

void NSchedulable::InitLbInfo()
	{
	}

void NSchedulable::NominalPriorityChanged()
	{
	}

void NSchedulable::LbDone(TUint aFlags)
	{
	BTrace8(BTrace::EHSched, BTrace::ELbDone, this, aFlags);
#ifdef KSCHED3
	if (IsGroup())
		{
		__KTRACE_OPT(KSCHED3,DEBUGPRINT("LbDone %G %x", this, aFlags));
		}
	else
		{
		__KTRACE_OPT(KSCHED3,DEBUGPRINT("LbDone %T %x", this, aFlags));
		}
#endif
	TBool keep = aFlags & K_Keep;
	TInt cpu = aFlags & K_CpuMask;
	TBool setcpu = aFlags & K_NewCpu;
	TBool keepcpu = aFlags & K_SameCpu;
	TBool checkcpu = aFlags & K_CheckCpu;
	LAcqSLock();
	TBool died = iLbState & ELbState_ExtraRef;
	if (keep && !died)
		{
		TScheduler& s = TheScheduler;
		s.iBalanceListLock.LockOnly();
		s.iBalanceList.Add(&iLbLink);
		iLbState = s.iLbCounter;
		s.iBalanceListLock.UnlockOnly();
		if (setcpu)
			SetCpuAffinityT(cpu | KCpuAffinityPref | (aFlags & K_CpuSticky));
		else
			{
			if (!keepcpu)
				iPreferredCpu = 0;
			if (checkcpu)
				SetCpuAffinityT(NTHREADBASE_CPU_AFFINITY_MASK);	// move it if it's on a core which is shutting down
			}
		}
	else
		{
		if (!keepcpu)
			iPreferredCpu = 0;
		iLbState = ELbState_Inactive;
		iLbLink.iNext = 0;
		iLbInfo.iRecentTime.i64 = 0;
		iLbInfo.iRecentCpuTime.i64 = 0;
		iLbInfo.iRecentActiveTime.i64 = 0;
		iLbInfo.iLbRunAvg = 0;
		iLbInfo.iLbActAvg = 0;
		iLbInfo.iLbRunActAvg = 0;
		if (checkcpu && !died)
			SetCpuAffinityT(NTHREADBASE_CPU_AFFINITY_MASK);	// move it if it's on a core which is shutting down
		}
	RelSLockU();
	if (died)
		{
		NKern::Lock();
		DropRef();
		NKern::Unlock();
		}
	}

void CalcRatio(TUint16& aRatio, TUint64 aN, TUint64 aD)
	{
	TInt ms1 = __e32_find_ms1_64(aD);
	if (ms1 < 0)
		{
		aRatio = 4095;
		return;
		}
	if (ms1 >= 20)
		{
		TInt shift = ms1 - 19;
		aD >>= shift;
		aN >>= shift;
		}
	// aD, aN now < 2^20
	TUint32 d = I64LOW(aD);
	TUint32 n = I64LOW(aN);
	if (n>d) n=d;
	TUint32 r = (n*4095+(d>>1))/d;
	if (r>4095) r=4095;	// shouldn't really happen
	aRatio = (TUint16)r;
	}

void CalcRatios(TUint16& aRT, TUint16& aAT, TUint16& aRA, TUint64 aDT, TUint64 aDR, TUint64 aDA)
	{
	TInt ms1 = __e32_find_ms1_64(aDT);
	if (ms1 >= 20)
		{
		TInt shift = ms1 - 19;
		aDT >>= shift;
		aDR >>= shift;
		aDA >>= shift;
		}
	// aDT, aDR, aDA now all < 2^20
	TUint32 t = I64LOW(aDT);
	TUint32 rtd = I64LOW(aDR);
	TUint32 atd = I64LOW(aDA);
	if (rtd>t) rtd=t;
	if (atd>t) atd=t;
	TUint32 rtt = (rtd*4095+(t>>1))/t;
	TUint32 att = (atd*4095+(t>>1))/t;
	TUint32 rta = atd ? (rtd*4095+(atd>>1))/atd : 0;
	if (rta>4095) rta=4095;	// shouldn't really happen
	aRT = (TUint16)rtt;
	aAT = (TUint16)att;
	aRA = (TUint16)rta;
	}

void NSchedulable::GetLbStats(TUint64 aTime)
	{
	SCpuStats stats;
	LAcqSLock();
	if (IsGroup())
		{
		NThreadGroup* g = (NThreadGroup*)this;
		if (g->iNThreadList.IsEmpty())
			iLbInfo.iLbNomPri = 1;
		else
			{
			NThreadBase* t = (NThreadBase*)g->iNThreadList.First();
			iLbInfo.iLbNomPri = t->iNominalPri;
			}
		}
	else
		iLbInfo.iLbNomPri = ((NThreadBase*)this)->iNominalPri;
	GetCpuStatsT(E_AllStats, stats);
	iLbInfo.iRecentTime.i64 += aTime;
	iLbInfo.iRecentCpuTime.i64 += stats.iRunTimeDelta;
	iLbInfo.iRecentActiveTime.i64 += stats.iActiveTimeDelta;
	TUint32 aff = iCpuAffinity;
	RelSLockU();
	CalcRatios(iLbInfo.iLbRunTime, iLbInfo.iLbActTime, iLbInfo.iLbRunAct, aTime, stats.iRunTimeDelta, stats.iActiveTimeDelta);
	iLbInfo.iLbRunAvg = TUint16((iLbInfo.iLbRunAvg + iLbInfo.iLbRunTime) >> 1);
	iLbInfo.iLbActAvg = TUint16((iLbInfo.iLbActAvg + iLbInfo.iLbActTime) >> 1);
	CalcRatio(iLbInfo.iLbRunActAvg, iLbInfo.iRecentCpuTime.i64, iLbInfo.iRecentActiveTime.i64);

	if (aff & NTHREADBASE_CPU_AFFINITY_MASK)
		iLbInfo.iLbAffinity = (TUint8)(aff & 0xff);
	else
		iLbInfo.iLbAffinity = 1u << aff;
	CalcHotWarm(iLbInfo.iLbHot, stats.iLastRunTime);
	CalcHotWarm(iLbInfo.iLbWarm, stats.iLastActiveTime);
	if (IsNew(this))
		{
		if (iLbInfo.iLbNomPri <= K_LB_HeavyPriorityThreshold)
			iLbInfo.iLbHeavy = 0xffu;
		else
			iLbInfo.iLbHeavy = 0;
		}
	iLbInfo.iLbHeavy >>= 1;
	if (iLbInfo.iLbActTime > K_LB_HeavyThreshold)
		iLbInfo.iLbHeavy |= 0x80u;
/*
	TUint64 blx = NKern::CpuTimeMeasFreq();
	blx *= 3;
	if (i_NSchedulable_Spare3 && iLbInfo.iLbRunActAvg<400 && stats.iActiveTime>blx)
		{
		__crash();
		}
*/	}

void AddToSortedQueue(SPerPri* aQ, NSchedulable* aS)
	{
	TInt k = aS->iLbInfo.iLbNomPri;
	if (k >= KNumPriorities)
		k = KNumPriorities;
	SPerPri* q = aQ + k;
	TBool h = IsHeavy(aS);
	SDblQueLink* anchor = &q->iA;
	SDblQueLink* p = q->First();
	for (; p!=anchor; p=p->iNext)
		{
		NSchedulable* s = _LOFF(p, NSchedulable, iLbLink);
		if (h)
			{
			if (!IsHeavy(s))
				continue;
			if (aS->iLbInfo.iLbRunActAvg < s->iLbInfo.iLbRunActAvg)
				break;
			}
		else
			{
			if (IsHeavy(s))
				break;
			if (aS->iLbInfo.iLbRunAvg > s->iLbInfo.iLbRunAvg)
				break;
			}
		}
	aS->iLbLink.InsertBefore(p);
	++q->iCount;
	if (h)
		{
		++q->iHeavy;
		}
	else
		{
		q->iTotalRun += aS->iLbInfo.iLbRunAvg;
		if (q->iTotalRun>4095)
			q->iTotalRun=4095;
		q->iTotalAct += aS->iLbInfo.iLbActAvg;
		}
	}

void SCpuAvailability::Init(TUint32 a)
	{
	iCount = __e32_find_ms1_32(a) + 1;
	iTotalRemain = 0;
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		if (a & (1<<i))
			{
			iRemain[i] = EIdle;
			iTotalRemain += EIdle;
			}
		else
			iRemain[i] = EUnavailable;
		}
	}

TInt SCpuAvailability::SetMaxed(TInt aCpu)
	{
	TInt x = iRemain[aCpu];
	if (x>0)
		iTotalRemain -= x;
	iRemain[aCpu] = EMaxedOut;
	return x;
	}

void SCpuAvailability::AddLoad(TInt aCpu, TInt aLoad)
	{
	if (TUint32(aLoad) > TUint32(EIdle))
		__crash();
	TInt& x = iRemain[aCpu];
	TInt orig = x;
	x -= aLoad;
	if (x < EMaxedOut)
		x = EMaxedOut;
	if (orig > 0)
		iTotalRemain -= ((orig > aLoad) ? aLoad : orig);
	}

TInt SCpuAvailability::FindMax() const
	{
	TInt maxv = KMinTInt;
	TInt maxi = -1;
	TInt i;
	for (i=0; i<iCount; ++i)
		{
		if (iRemain[i] > maxv)
			{
			maxv = iRemain[i];
			maxi = i;
			}
		}
	return maxi;
	}

TInt SCpuAvailability::FindMax(NSchedulable* aS) const
	{
	TUint32 s = aS->iLbInfo.iLbAffinity;
	s &= TheScheduler.iThreadAcceptCpus;
	if ( (s&(s-1)) == 0 )
		return __e32_find_ms1_32(s);
	TInt maxv = KMinTInt;
	TInt maxi = -1;
	TInt i = 0;
	for (; s; s>>=1, ++i)
		{
		if ((s&1) && iRemain[i] > maxv)
			{
			maxv = iRemain[i];
			maxi = i;
			}
		}
	return maxi;
	}

TInt SCpuAvailability::PickCpu(NSchedulable* aS, TBool aDropped) const
	{
	TUint32 s0 = aS->iLbInfo.iLbAffinity & TheScheduler.iThreadAcceptCpus;
	TUint32 s = s0;
//	BTrace12(BTrace::EHSched, 0x90u, aS, s, aPtr);
	if ( (s&(s-1)) == 0 )
		return __e32_find_ms1_32(s);
	TInt maxv = KMinTInt;
	TInt maxi = -1;
	TInt i = 0;
	for (; s; s>>=1, ++i)
		{
//		BTrace12(BTrace::EHSched, 0x91u, s, maxv, aPtr[i]);
		if ((s&1) && iRemain[i] > maxv)
			{
			maxv = iRemain[i];
			maxi = i;
			}
		}
	if (IsNew(aS))
		{
		// this thread hasn't run for a while
		// pick the highest numbered CPU with a near-maximum availability
		i = __e32_find_ms1_32(s0);
		for (; i>maxi; --i)
			{
			if ( (s0&(1u<<i)) && maxv-iRemain[i]<K_LB_CpuLoadDiffThreshold)
				return i;
			}
		}
	else
		{
		// this thread has run recently - see if we can keep it on the same CPU
		TInt threshold = aDropped ? 1 : (TInt)K_LB_CpuLoadDiffThreshold;
		TInt lcpu = aS->iLastCpu;
		if ( (s0&(1u<<lcpu)) && maxv-iRemain[lcpu]<threshold)
			return lcpu;
		}
	// use highest availability CPU
	return maxi;
	}

void TScheduler::BalanceTimerExpired(TAny* aPtr)
	{
	((TScheduler*)aPtr)->PeriodicBalance();
	}

TBool TScheduler::ReBalance(SDblQue& aQ, TBool aCC)
	{
	ModifyCCState(~ECCRebalanceRequired, 0);

	SPerPri sbq[KNumPriorities+1];
	NSchedulable* s = 0;
	TInt i;
	TUint64 now = NKern::Timestamp();
	TUint64 lbt = iLastBalanceTime;
	iLastBalanceTime = now;
	TUint64 bpl = now - lbt;		// balance period length
	TUint cc = aCC ? K_CheckCpu : 0;

	TInt nact = __e32_bit_count_32(iThreadAcceptCpus);	// number of CPUs available

	// aQ holds list of threads/groups to be considered
	TInt ns = 0;	// number for further consideration
	TInt nd = 0;	// number dropped this time round
	SCpuAvailability avail;
	avail.Init(iThreadAcceptCpus);
	TUint32 gravel = 0;
	TInt totalN = 0;
	TInt checked = 0;
	while (!aQ.IsEmpty())
		{
		NThread* t = 0;
		++totalN;
		s = _LOFF(aQ.First()->Deque(), NSchedulable, iLbLink);
		if (!s->IsGroup())
			{
			t = (NThread*)s;
			if (t->iRebalanceAttr & 1)
				++checked;
			}
		s->GetLbStats(bpl);
		if (
			(s->iLbInfo.iLbWarm >= LB_DormantThreshold)	// hasn't run for a while
		||	(s->iLbInfo.iLbWarm>0 && s->iLbInfo.iLbRunAvg<K_LB_GravelThreshold_RunAvg && s->iLbInfo.iLbRunActAvg>K_LB_GravelThreshold_RunActAvg)	// gravel
		)
			{
			TUint32 a = s->iLbInfo.iLbAffinity;
			if ( (a&(a-1)) == 0)
				avail.AddLoad(__e32_find_ms1_32(a), s->iLbInfo.iLbRunAvg);
			else
				gravel += s->iLbInfo.iLbRunAvg;
			if (!IsNew(s))
				++nd;
			s->LbDone(cc);		// drop it
			}
		else if (nact==1)
			{
			s->LbDone(cc|K_Keep);	// keep it but only 1 CPU so don't balance
			}
		else if (t && t->iCoreCycling)
			{
			s->LbDone(cc|K_Keep);	// keep it but don't balance
			}
		else
			{
			++ns;
			AddToSortedQueue(&sbq[0], s);
			}
		}

	gravel /= TUint(nact);
	for (i=0; i<KMaxCpus; ++i)
		{
		if (iThreadAcceptCpus & (1<<i))
			avail.AddLoad(i, gravel);
		}
	if (ns>0)
		{
		TInt k;
		for (k=KNumPriorities; k>=0; --k)
			{
			SPerPri& q = sbq[k];
			if (q.iCount==0)
				{
				__NK_ASSERT_ALWAYS(q.IsEmpty());
				continue;
				}
			if (nact==0)
				goto dump_remaining;
			while (!q.IsEmpty())
				{
				s = _LOFF(q.First(), NSchedulable, iLbLink);
//				BTrace12(BTrace::EHSched, 0x80u, s, s->iLbInfo.iLbRunAvg, s->iLbInfo.iLbRunActAvg);
				if (IsHeavy(s))
					break;
				s->iLbLink.Deque();
				TInt cpu = avail.PickCpu(s, nd);
//				BTrace12(BTrace::EHSched, 0x81u, cpu, remain[cpu], totalremain);
				avail.AddLoad(cpu, s->iLbInfo.iLbRunAvg);
//				BTrace8(BTrace::EHSched, 0x82u, remain[cpu], totalremain);
				s->LbDone(cc|K_Keep|K_NewCpu|cpu);
				}
			if (q.iHeavy > nact)
				{
				TInt hr = avail.TotalRemain() / q.iHeavy;
				TInt n = q.iHeavy;
				TInt j;
				for (j=0; j<nact; ++j)
					{
					// don't bother about keeping same CPU since we must rotate
					// threads between CPUs to even out the run times.
					TInt cpu = avail.FindMax();
//					BTrace12(BTrace::EHSched, 0x83u, cpu, remain[cpu], totalremain);
					TInt capacity = avail.SetMaxed(cpu);
//					BTrace8(BTrace::EHSched, 0x84u, remain[cpu], totalremain);
					TInt nh = 0;
					if (hr > K_LB_HeavyCapacityThreshold)
						{
						if (j == nact-1)
							nh = n;
						else
							nh = capacity / hr;
						}
					else
						nh = n / (nact-j);
					n -= nh;
					for (; nh>0; --nh)
						{
						if (q.IsEmpty())
							__crash();
						s = _LOFF(q.First()->Deque(), NSchedulable, iLbLink);
						s->LbDone(cc|K_Keep|K_NewCpu|cpu);
						}
					}
				nact = 0;
				}
			else
				{
				while (!q.IsEmpty())
					{
					s = _LOFF(q.First()->Deque(), NSchedulable, iLbLink);
					TInt cpu = avail.PickCpu(s, nd);
//					BTrace12(BTrace::EHSched, 0x85u, cpu, remain[cpu], totalremain);
					avail.SetMaxed(cpu);
//					BTrace8(BTrace::EHSched, 0x86u, remain[cpu], totalremain);
					s->LbDone(cc|K_Keep|K_NewCpu|cpu);
					--nact;
					}
				}
			__NK_ASSERT_ALWAYS(q.IsEmpty());
			if (nact==0)
				{
dump_remaining:
				while (!q.IsEmpty())
					{
//					BTrace4(BTrace::EHSched, 0x87u, s);
					s = _LOFF(q.First()->Deque(), NSchedulable, iLbLink);
					s->LbDone(cc|K_Keep);	// keep it but lose preferred CPU
					}
				continue;
				}
			}
		}

	// return TRUE if the only threads which ran were this one and the NTimer thread
	return (totalN==2 && checked==2);
	}

void TScheduler::PeriodicBalance()
	{
	iNeedBal = 0;
	ModifyCCState( ~ECCRebalanceTimerQueued, 0 );
	SDblQue rbq;	// raw balance queue
	GetLbThreads(rbq);
	TInt i;
	for (i=0; i<iNumCpus; ++i)
		iSub[i]->GetLbThreads(rbq);
	TBool bored = ReBalance(rbq, FALSE);
	if (!bored || iNeedBal)
		StartRebalanceTimer(FALSE);
	}


void TScheduler::StartPeriodicBalancing()
	{
#ifdef KBOOT
	__KTRACE_OPT(KBOOT,DEBUGPRINT("StartPeriodicBalancing()"));
	TInt i;
	for (i=0; i<KMaxCpus; ++i)
		{
		TSubScheduler& ss = TheSubSchedulers[i];
		volatile TUint32* p = (volatile TUint32*)ss.iUncached;
		__KTRACE_OPT(KBOOT,DEBUGPRINT("CPU %1d: iUncached=%08x -> %08x %08x %08x %08x", i, p, p[0], p[1], p[2], p[3]));
		}
#endif
	TheScheduler.StartRebalanceTimer(TRUE);
	}

void TScheduler::StartRebalanceTimer(TBool aRestart)
	{
	TInt interval = K_LB_BalanceInterval;
	TUint32 mask = aRestart ? (ECCRebalanceTimerQueued|ECCPeriodicBalancingActive) : (ECCRebalanceTimerQueued);
	TUint32 orig = ModifyCCState(~mask, mask);
	TUint32 ns = (orig &~ mask) ^ mask;
	__KTRACE_OPT(KSCHED3,DEBUGPRINT("StrtRbTmr %08x %08x %08x", mask, orig, ns));
	if ((ns & ECCPeriodicBalancingActive) && !(orig & ECCRebalanceTimerQueued))
		{
		TInt r = KErrArgument;
		if (orig & ECCPeriodicBalancingActive)
			{
			r = iBalanceTimer.Again(interval);
			if (r == KErrArgument)
				{
				++LBDelayed;	// so we can see if this happened
				}
			}
		if (r == KErrArgument)
			{
			r = iBalanceTimer.OneShot(interval);
			}
		if (r != KErrNone)
			__crash();
		}
	}

void TScheduler::StopRebalanceTimer(TBool aTemp)
	{
	TUint32 mask = aTemp ? ECCRebalanceTimerQueued : (ECCRebalanceTimerQueued|ECCPeriodicBalancingActive);
	TUint32 orig = ModifyCCState(~mask, 0);
	__KTRACE_OPT(KSCHED3,DEBUGPRINT("StopRbTmr %08x %08x", mask, orig));
	if (orig & ECCRebalanceTimerQueued)
		iBalanceTimer.Cancel();
	}



/******************************************************************************
 * Core Control
 ******************************************************************************/

/*

TScheduler fields used for core control:

iThreadAcceptCpus
	Bit n = 1 iff CPU n is available to threads with no specific affinity.
	Bits corresponding to existing CPUs are set at boot time.
	Subsequently this word is only modified by load balancer thread.
	Bit n is cleared when a decision is made to shut down core n.


iIpiAcceptCpus
	Bit n = 1 iff CPU n is accepting generic IPIs
	Bits corresponding to existing CPUs are set at boot time.
	Bit n is cleared when CPU n makes the decision to ask the idle handler to power down
		At the same time, bit n of iCpusGoingDown is set.
	Bit n is set when CPU n returns from the idle handler after waking up.
	Protected by iGenIPILock

iCpusComingUp
	Bit n = 1 iff CPU n is in the process of powering up
	All bits zero at boot
	Bit n set when the load balancer decides to initiate power up of CPU n, provided iCCDeferCount==0
	Bit n cleared when the load balancer sets iThreadAcceptCpus bit n
	Protected by iGenIPILock

iCpusGoingDown
	Bit n = 1 iff CPU n is in the process of powering down and is no longer accepting IPIs
	All bits zero at boot
	Bit n is set when CPU n makes the decision to ask the idle handler to power down
	?Bit n is cleared when?
		- when TCoreCycler observes the CPU has detached
		- when the load balancer observes the CPU has detached
		- when the load balancer decides to reactivate the CPU
	Protected by iGenIPILock

iCCDeferCount
	If this is positive CPUs being shut down will not proceed to clear iIpiAcceptCpus
	In this case bits can be set in iIpiAcceptCpus but cannot be cleared.
	Also (iIpiAcceptCpus|iCpusComingUp) remains constant
	Protected by iGenIPILock

iCCSyncCpus
	Bit n = 1 iff a change has been made to iThreadAcceptCpus which CPU n should observe
	but it has not yet observed it.
	Bit n set by the load balancer after a change is made to iThreadAcceptCpus, provided bit n
	is also set in iIpiAcceptCpus.
	Bit n cleared when CPU n services the core control sync IPI if iKernCSLocked==0 or the
	next time iKernCSLocked becomes zero otherwise.

iCCReactivateCpus
	Bit n = 1 if CPU n is being reactivated after being removed from iThreadAcceptCpus
	Bit n is set if a thread is made ready, cannot be assigned to any active CPU on
		account of affinity restrictions and is assigned to CPU n.
	Bit n is also set when CPU n wakes up from being retired.
	Protected by iGenIPILock

iCCState
	Bit 31 (ECCReqPending)	Set when an external request to change the number of cores is in progress

iCCRequestLevel
	The number of CPUs last requested to be active.

iGenIPILock

iCCSyncIDFC
	Runs when all CPUs have observed a change to iThreadAcceptCpus

iCCReactivateDfc
	Runs whenever one or more bits have been set in iCCReactivateCpus

iCCRequestDfc
	Runs whenever a request is received to change the number of active cores

TSubScheduler fields used for core control:


*/

void TScheduler::CCUnDefer()
	{
	TUint32 powerOn = 0;
	TBool doDeferredReq = FALSE;
	TInt irq = iGenIPILock.LockIrqSave();
	if (--iCCDeferCount == 0)
		{
		// Kick cores waiting to power off
		__holler();

		// See if any cores are waiting to power on
		powerOn = iCCReactivateCpus &~ iCpusComingUp;

		// See if a core control request has been deferred
		if (iCCState & ECCReqDeferred)
			{
			if (iCpusComingUp==0 && iCCReactivateCpus==0)
				doDeferredReq = TRUE;
			}
		}
	iGenIPILock.UnlockIrqRestore(irq);
	if (powerOn)
		iCCReactivateDfc.Enque();
	if (doDeferredReq)
		iCCRequestDfc.Enque();
	}

void TScheduler::CCSyncDone(TAny* aPtr)
	{
	NFastSemaphore* s = (NFastSemaphore*)aPtr;
	s->Signal();
	}

void CCSyncIPI(TGenericIPI*)
	{
	TScheduler& s = TheScheduler;
	TSubScheduler& ss = SubScheduler();
	if (ss.iKernLockCount)
		{
		ss.iCCSyncPending = 1;
		ss.iRescheduleNeededFlag = 1;
		return;
		}
	TUint32 m = ss.iCpuMask;
	if (__e32_atomic_and_ord32(&s.iCCSyncCpus, ~m)==m)
		{
		s.iCCSyncIDFC.Add();
		}
	}

void TScheduler::ChangeThreadAcceptCpus(TUint32 aNewMask)
	{
	NThread* lbt = LBThread();
	if (NKern::CurrentThread() != lbt)
		__crash();
	TInt irq = iGenIPILock.LockIrqSave();
	++iCCDeferCount;
	iThreadAcceptCpus = aNewMask;
	TUint32 cpus = iIpiAcceptCpus;
	iCCSyncCpus = cpus;
	iCpusComingUp &= ~aNewMask;
	iGenIPILock.UnlockIrqRestore(irq);

	NFastSemaphore sem(0);
	iCCSyncIDFC.iPtr = &sem;
	TGenericIPI ipi;
	ipi.Queue(&CCSyncIPI, cpus);

	NKern::FSWait(&sem);
	CCUnDefer();
	}

template<int N> struct Log2 {};

TEMPLATE_SPECIALIZATION struct Log2<1> { enum {Log=0u}; };
TEMPLATE_SPECIALIZATION struct Log2<2> { enum {Log=1u}; };
TEMPLATE_SPECIALIZATION struct Log2<4> { enum {Log=2u}; };
TEMPLATE_SPECIALIZATION struct Log2<8> { enum {Log=3u}; };
TEMPLATE_SPECIALIZATION struct Log2<16> { enum {Log=4u}; };
TEMPLATE_SPECIALIZATION struct Log2<32> { enum {Log=5u}; };


class TCpuSet
	{
public:
	enum {
		EBitsPerTUint8Shift=3u,
		EBitsPerTUint32Shift=EBitsPerTUint8Shift+Log2<sizeof(TUint32)>::Log,
		EBitsPerTUint8=1u<<EBitsPerTUint8Shift,
		EBitsPerTUint32=1u<<EBitsPerTUint32Shift,
		EWords=1u<<(KMaxCpus-EBitsPerTUint32Shift),
		EBytes=1u<<(KMaxCpus-EBitsPerTUint8Shift),
		EBits=1u<<KMaxCpus,
		};
public:
	TCpuSet(TUint32 aMask);
	void Consider(TUint32 aAffinity);
	TCpuSet& operator&=(const TCpuSet&);
	TCpuSet& operator|=(const TCpuSet&);
	TCpuSet& Not();
	TBool IsEmpty() const;
	TInt Profile(TInt* aOut) const;
	TUint32 Select(TInt aDesiredNumber, TUint32 aCurrent, TUint32 aIgnore) const;
private:
	/**
	Bitfield: Bit n	= bit (n%8) of byte INT(n/8)
					= bit (n%32) of word INT(n/32)
	Bit n is set if the subset S of CPUs represented by the bits of n in the
	canonical way (i.e. x \in S <=> bit x of n = 1) is acceptable.
	*/
	TUint32	iMask[EWords];
	};

TCpuSet::TCpuSet(TUint32 aM)
	{
	memset(iMask, 0, sizeof(iMask));
	TInt i;
	TUint32 m=1;	// empty set only
	for (i=0; i<EBitsPerTUint32Shift; ++i)
		{
		TUint32 ibit = 1u<<i;
		if (aM & ibit)
			m |= (m<<ibit);
		}
	iMask[0] = m;
	for (; i<KMaxCpus; ++i)
		{
		TUint32 ibit = 1u<<i;
		if (aM & ibit)
			{
			TInt ws = 1<<(i-EBitsPerTUint32Shift);
			TInt j;
			for (j=0; j<ws; ++j)
				iMask[ws+j] = iMask[j];
			}
		}
	}

TCpuSet& TCpuSet::operator&=(const TCpuSet& aS)
	{
	TInt i;
	for (i=0; i<EWords; ++i)
		iMask[i] &= aS.iMask[i];
	return *this;
	}

TCpuSet& TCpuSet::operator|=(const TCpuSet& aS)
	{
	TInt i;
	for (i=0; i<EWords; ++i)
		iMask[i] |= aS.iMask[i];
	return *this;
	}

TCpuSet& TCpuSet::Not()
	{
	TInt i;
	for (i=0; i<EWords; ++i)
		iMask[i] = ~iMask[i];
	return *this;
	}

TBool TCpuSet::IsEmpty() const
	{
	TInt i;
	TUint32 x = 0;
	for (i=0; i<EWords; ++i)
		x |= iMask[i];
	return !x;
	}

void TCpuSet::Consider(TUint32 aAffinity)
	{
	TUint32 am = AffinityToMask(aAffinity);
	am &= EBits-1;
	if (am == EBits-1 || am==0)
		return;	// no restrictions

	TCpuSet bad(am ^ (EBits-1));	// sets incompatible with aAffinity
	TInt i;
	for (i=0; i<EWords; ++i)
		iMask[i] &= ~bad.iMask[i];	// knock out sets incompatible with aAffinity
	}

const TUint32 Pmask[6] =
	{
	0x00000001,			// no bits set
	0x00010116,			// 1 bit set (10000, 01000, 00100, 00010, 00001 -> 16,8,4,2,1)
	0x01161668,			// 2 bits set (11000, 10100, 10010, 10001, 01100, 01010, 01001, 00110, 00101, 00011 -> 24,20,18,17,12,10,9,6,5,3)
	0x16686880,			// 3 bits set (11100, 11010, 11001, 10110, 10101, 10011, 01110, 01101, 01011, 00111 -> 28,26,25,22,21,19,14,13,11,7)
	0x68808000,			// 4 bits set (11110, 11101, 11011, 10111, 01111 -> 30,29,27,23,15)
	0x80000000			// 5 bits set
	};

/**
	Sets aOut[n] = number of entries with n CPUs present (0<=n<=KMaxCpus)
	Returns total number of entries
*/
TInt TCpuSet::Profile(TInt* aOut) const
	{
	TInt i,j;
	TInt r = 0;
	memset(aOut, 0, (KMaxCpus+1)*sizeof(TInt));
	for (i=0; i<EWords; ++i)
		{
		TUint32 m = iMask[i];
		if (!m)
			continue;
		TInt n1 = __e32_bit_count_32(i);
		for (j=0; j<=EBitsPerTUint32Shift; ++j)
			{
			TInt dr = __e32_bit_count_32(m & Pmask[j]);
			r += dr;
			aOut[n1+j] += dr;
			}
		}
	return r;
	}

/**
	Given a desired number of active cores and the mask of currently
	running cores, returns the new mask of active cores.
*/
TUint32 TCpuSet::Select(TInt aDesiredNumber, TUint32 aCurrent, TUint32 aIgnore) const
	{
	TInt max = __e32_bit_count_32(aCurrent);
	if (aDesiredNumber > max)
		return 0;
	TInt profile[KMaxCpus+1] = {0};
	Profile(profile);
	TInt dn;
	for (dn=aDesiredNumber; dn<=max && profile[dn]==0; ++dn)
		{}
	if (dn > max)
		return 0;
	TInt wix;
	TUint32 bestMask = 0;
	TInt bestDiff = KMaxTInt;
	TInt stop = max - dn;
	for (wix=0; wix<EWords; ++wix)
		{
		TUint32 candidate = wix << EBitsPerTUint32Shift;
		TUint32 m = iMask[wix];
		if (!m)
			continue;
		TInt n1 = __e32_bit_count_32(wix);
		if (n1 > dn)
			continue;
		m &= Pmask[dn-n1];
		for (; m; m>>=1, ++candidate)
			{
			if (!(m&1))
				continue;
			TUint32 diff = (candidate&~aIgnore) ^ aCurrent;
			TInt wt = __e32_bit_count_32(diff);
			if (wt < bestDiff)
				{
				bestDiff = wt;
				bestMask = candidate;
				if (bestDiff == stop)
					{
					wix = EWords;
					break;
					}
				}
			}
		}
	return bestMask;
	}

void NSchedulable::LbTransfer(SDblQue& aDestQ)
	{
	if (iLbState & ELbState_PerCpu)
		{
		TSubScheduler* ss = &TheSubSchedulers[iLbState & ELbState_CpuMask];
		ss->iReadyListLock.LockOnly();
		if (iLbState == ss->iLbCounter)
			{
			iLbLink.Deque();
			}
		ss->iReadyListLock.UnlockOnly();
		}
	else if ((iLbState & ELbState_CpuMask) == ELbState_Global)
		{
		TScheduler& s = TheScheduler;
		s.iBalanceListLock.LockOnly();
		if (iLbState == s.iLbCounter)
			{
			iLbLink.Deque();
			}
		s.iBalanceListLock.UnlockOnly();
		}
	else if (iLbState != ELbState_Inactive)
		{
		// shouldn't happen
		__crash();
		}
	iLbState = ELbState_Temp;
	aDestQ.Add(&iLbLink);
	}

void GetAll(SDblQue& aOutQ, SIterDQ* aInQ)
	{
	TScheduler& s = TheScheduler;
	SIterDQIterator iter;
	TInt maxSteps = NKern::NumberOfCpus() + 2;
	TInt r;
	NKern::Lock();
	s.iEnumerateLock.LockOnly();
	iter.Attach(aInQ);
	FOREVER
		{
		SIterDQLink* link = 0;
		r = iter.Step(link, maxSteps);
		if (r == KErrEof)
			break;
		if (r == KErrNone)
			{
			NSchedulable* sch = _LOFF(link, NSchedulable, iEnumerateLink);
			sch->AcqSLock();
			sch->LbTransfer(aOutQ);
			sch->RelSLock();
			}
		s.iEnumerateLock.FlashPreempt();
		}
	iter.Detach();
	s.iEnumerateLock.UnlockOnly();
	NKern::Unlock();
	}

void GetAll(SDblQue& aOutQ)
	{
	TScheduler& s = TheScheduler;
	GetAll(aOutQ, &s.iAllGroups);
	GetAll(aOutQ, &s.iAllThreads);
/*
	SDblQueLink* l0 = aOutQ.Last();
	SDblQueLink* anchor = &aOutQ.iA;
	GetLbThreads(aOutQ);
	TInt i;
	for (i=0; i<s.iNumCpus; ++i)
		s.iSub[i]->GetLbThreads(aOutQ);
	SDblQueLink* l = l0->iNext;
	for (; l!=anchor; l=l->iNext)
		{
		NSchedulable* sch = _LOFF(l, NSchedulable, iLbLink);
		sch->LAcqSLock();
		sch->iLbState = (sch->iLbState & ELbState_ExtraRef) | ELbState_Temp;
		sch->RelSLockU();
		}
*/
	}

void GetCpuSet(TCpuSet& aSet, SDblQue& aQ)
	{
	SDblQueLink* anchor = &aQ.iA;
	SDblQueLink* l = aQ.First();
	for (; l!=anchor; l=l->iNext)
		{
		NSchedulable* sch = _LOFF(l, NSchedulable, iLbLink);
		if (!sch->IsGroup() && ((NThreadBase*)sch)->i_NThread_Initial )
			continue;	// skip idle threads since they are locked to their respective CPU
		TUint32 aff = sch->iCpuAffinity;
		aSet.Consider(aff);
		}
	}


void TScheduler::CCReactivateDfcFn(TAny* a)
	{
	((TScheduler*)a)->CCReactivate(0);
	}

void TScheduler::CCRequestDfcFn(TAny* a)
	{
	((TScheduler*)a)->CCRequest();
	}

void TScheduler::CCIpiReactivateFn(TAny* a)
	{
	((TScheduler*)a)->CCIpiReactivate();
	}

TUint32 TScheduler::ModifyCCState(TUint32 aAnd, TUint32 aXor)
	{
	TInt irq = iGenIPILock.LockIrqSave();
	TUint32 orig = iCCState;
	iCCState = (orig & aAnd) ^ aXor;
	iGenIPILock.UnlockIrqRestore(irq);
	return orig;
	}


/**
Runs if a thread is made ready on a CPU marked for shutdown (apart from on
account of core cycling) or if a core wakes up from shutdown.
*/
void TScheduler::CCReactivate(TUint32 aMore)
	{
	TUint32 startPowerUp = 0;		// cores which need to be powered up
	TUint32 finishPowerUp = 0;		// cores which have just powered up
	TInt irq = iGenIPILock.LockIrqSave();
	iCCReactivateCpus |= aMore;
	TUint32 cu = iCpusComingUp | iIpiAcceptCpus;
	finishPowerUp = iCCReactivateCpus & cu;
	iCCReactivateCpus &= ~finishPowerUp;
	if (iCCDeferCount == 0)
		{
		startPowerUp = iCCReactivateCpus &~ cu;
		iCCReactivateCpus = 0;
		iCpusComingUp |= startPowerUp;
		}
	TUint32 ccs = iCCState;
	iGenIPILock.UnlockIrqRestore(irq);
	if (startPowerUp)
		{
		// Begin powering up cores
		CCInitiatePowerUp(startPowerUp);
		}
	if (finishPowerUp)
		{
		// ?Rebalance load to new cores now or wait till next periodic?
		ChangeThreadAcceptCpus(iThreadAcceptCpus | finishPowerUp);
		if ((iThreadAcceptCpus & (iThreadAcceptCpus-1)) && !(ccs & ECCPeriodicBalancingActive))
			{
			// more than 1 core so restart periodic balancing
			StartRebalanceTimer(TRUE);
			}
		if (startPowerUp == 0)
			ModifyCCState(~ECCPowerUpInProgress, 0);
		}
	if (iNeedBal)
		{
		if ( (ccs & (ECCPeriodicBalancingActive|ECCRebalanceTimerQueued)) == ECCPeriodicBalancingActive)
			{
			StartRebalanceTimer(FALSE);
			}
		}
	}

extern "C" void wake_up_for_ipi(TSubScheduler* aSS, TInt)
	{
	TScheduler& s = *aSS->iScheduler;
	if (__e32_atomic_ior_ord32(&s.iCCIpiReactivate, aSS->iCpuMask)==0)
		{
		s.iCCIpiReactIDFC.RawAdd();
		}
	}

/**
Runs if a core needs to wake up on account of a transferred tied IRQ or IDFC
*/
void TScheduler::CCIpiReactivate()
	{
	TUint32 cores = __e32_atomic_swp_ord32(&iCCIpiReactivate, 0);
	TInt irq = iGenIPILock.LockIrqSave();
	iCCReactivateCpus |= cores;
	iGenIPILock.UnlockIrqRestore(irq);
	iCCReactivateDfc.DoEnque();
	}

TUint32 TScheduler::ReschedInactiveCpus(TUint32 aMask)
	{
	TUint32 rm = aMask & 0x7FFFFFFFu;
	if (aMask & 0x80000000u)
		{
		TSubScheduler& ss = SubScheduler();
		TUint32 me = ss.iCpuMask;
		if (__e32_atomic_and_ord32(&iCCSyncCpus, ~me) == me)
			{
			rm |= me;
			iCCSyncIDFC.RawAdd();
			}
		}
	return rm;
	}

TUint32 TScheduler::CpuShuttingDown(TSubScheduler& aSS)
	{
	TUint32 m = aSS.iCpuMask;
	iIpiAcceptCpus &= ~m;		// no more IPIs for us
	iCpusGoingDown |= m;		// we are now past the 'point of no return'
	TUint32 more = iIpiAcceptCpus &~ (iThreadAcceptCpus | iCpusComingUp | iCCReactivateCpus);
	if (more)
		return more;
	if (iCCState & ECCPowerDownInProgress)
		return KMaxTUint32;
	return 0;
	}

// Called just before last CPU goes idle
void TScheduler::AllCpusIdle()
	{
	}

// Called just after first CPU wakes up from idle
void TScheduler::FirstBackFromIdle()
	{
	}


struct SCoreControlAction
	{
	SCoreControlAction();

	TInt	iPowerUpCount;			// number of cores to power on ...
	TUint32	iPowerUpCandidates;		// ... out of these
	TUint32 iPowerUpChoice;			// chosen to power on
	TInt	iPowerDownCount;		// number of cores to power off ...
	TUint32	iPowerDownCandidates;	// ... out of these
	TUint32 iPowerDownChoice;		// chosen to power off

	// snapshot of core control state
	TInt	iCCRequestLevel;
	TUint32	iThreadAcceptCpus;
	TUint32	iIpiAcceptCpus;
	TUint32	iCpusComingUp;
	TUint32 iCCReactivateCpus;

	TBool	iCCDefer;
	SDblQue	iBalanceQ;
	};

SCoreControlAction::SCoreControlAction()
	:	iPowerUpCount(0),
		iPowerUpCandidates(0),
		iPowerUpChoice(0),
		iPowerDownCount(0),
		iPowerDownCandidates(0),
		iPowerDownChoice(0),
		iCCRequestLevel(0),
		iThreadAcceptCpus(0),
		iIpiAcceptCpus(0),
		iCpusComingUp(0),
		iCCReactivateCpus(0),
		iCCDefer(0)
	{
	}

void TScheduler::InitCCAction(SCoreControlAction& aA)
	{
	aA.iPowerUpCount = 0;
	aA.iPowerUpCandidates = 0;
	aA.iPowerUpChoice = 0;
	aA.iPowerDownCount = 0;
	aA.iPowerDownCandidates = 0;
	aA.iPowerDownChoice = 0;
	aA.iCCDefer = FALSE;

	TUint32 all = (1u<<iNumCpus)-1;

	TInt irq = iGenIPILock.LockIrqSave();

	// cores fully operative and not being powered off
	TUint32 c1 = iThreadAcceptCpus;

	// cores in the process of being retired
	TUint32 c0 = iIpiAcceptCpus &~ (iThreadAcceptCpus | iCpusComingUp | iCCReactivateCpus);

	// cores on (including those being retired) or coming up
	TUint32 c2 = (iIpiAcceptCpus | iCpusComingUp | iCCReactivateCpus);
	TInt n2 = __e32_bit_count_32(c2);

	// cores on and not being retired, plus cores being reactivated
	TUint32 c3 = c2 &~ c0;
	TInt n3 = __e32_bit_count_32(c3);

	TInt req = iCCRequestLevel;

	// take snapshot of state
	aA.iCCRequestLevel = req;
	aA.iThreadAcceptCpus = c1;
	aA.iIpiAcceptCpus = iIpiAcceptCpus;
	aA.iCpusComingUp = iCpusComingUp;
	aA.iCCReactivateCpus = iCCReactivateCpus;

	if (req > n2)
		{
		// need to activate some more cores
		aA.iPowerUpCount = req - n2;
		aA.iPowerUpCandidates = all &~ c2;
		iCCReactivateCpus |= c0;	// revive cores currently in the process of powering down
		iCCState &= ~ECCReqPending;
		iCCState |= ECCPowerUpInProgress;
		}
	else if (req > n3)
		{
		// need to reactivate some cores which are currently powering down
		aA.iPowerUpCount = req - n3;
		aA.iPowerUpCandidates = c0;
		iCCState &= ~ECCReqPending;
		iCCState |= ECCPowerUpInProgress;
		aA.iCCDefer = TRUE;
		++iCCDeferCount;	// stop cores going down past recovery
		}
	else if (req == n3)
		{
		// don't need to do anything
		iCCState &= ~ECCReqPending;
		}
	else if (iCpusComingUp | iCCReactivateCpus)
		{
		// defer this request until reactivations in progress have happened
		iCCState |= ECCReqDeferred;
		}
	else
		{
		// need to retire some more cores
		aA.iPowerDownCount = n3 - req;
		aA.iPowerDownCandidates = c3;
		iCCState &= ~ECCReqPending;
		iCCState |= ECCPowerDownInProgress;
		}
	iGenIPILock.UnlockIrqRestore(irq);
	}


/**
Runs when a request is made to change the number of active cores
*/
void TScheduler::CCRequest()
	{
	SCoreControlAction action;
	InitCCAction(action);
	if (action.iPowerDownCount > 0)
		{
		TCpuSet cpuSet(action.iIpiAcceptCpus);
		GetAll(action.iBalanceQ);
		GetCpuSet(cpuSet, action.iBalanceQ);

		TUint32 leaveOn = cpuSet.Select(action.iCCRequestLevel, action.iIpiAcceptCpus, action.iIpiAcceptCpus&~action.iPowerDownCandidates);
		if (leaveOn)
			{
			action.iPowerDownChoice = action.iPowerDownCandidates &~ leaveOn;

			// remove CPUs to be shut down from iThreadAcceptCpus
			ChangeThreadAcceptCpus(iThreadAcceptCpus &~ action.iPowerDownChoice);
			}

		// rebalance to remaining cores
		StopRebalanceTimer(TRUE);
		ReBalance(action.iBalanceQ, TRUE);
		if (iThreadAcceptCpus & (iThreadAcceptCpus - 1))
			{
			// more than 1 CPU on
			ModifyCCState(~ECCPowerDownInProgress, 0);
			StartRebalanceTimer(FALSE);
			}
		else
			ModifyCCState(~(ECCPowerDownInProgress|ECCPeriodicBalancingActive), 0);	// stop periodic balancing
		}
	if (action.iPowerUpCount > 0)
		{
		TUint32 ch = 0;
		TUint32 ca = action.iPowerUpCandidates;
		TInt n = action.iPowerUpCount;
		while(n)
			{
			TInt b = __e32_find_ls1_32(ca);
			ch |= (1u<<b);
			ca &= ~(1u<<b);
			--n;
			}
		action.iPowerUpChoice = ch;
		CCReactivate(action.iPowerUpChoice);
		if (action.iCCDefer)
			CCUnDefer();
		}
	}

/**
Initiates a change to the number of active cores
*/
EXPORT_C void NKern::SetNumberOfActiveCpus(TInt aNumber)
	{
	__NK_ASSERT_ALWAYS(aNumber>0 && aNumber<=NKern::NumberOfCpus());
	TScheduler& s = TheScheduler;
	if (!s.CoreControlSupported())
		return;
	TBool chrl = FALSE;
	TBool kick = FALSE;
	NKern::Lock();
	TInt irq = s.iGenIPILock.LockIrqSave();
	if (s.iCCRequestLevel != (TUint32)aNumber)
		{
		s.iCCRequestLevel = aNumber;
		chrl = TRUE;
		}

	// cores in the process of being retired
	TUint32 c0 = s.iIpiAcceptCpus &~ (s.iThreadAcceptCpus | s.iCpusComingUp | s.iCCReactivateCpus);

	// cores on (including those being retired) or coming up
	TUint32 c2 = (s.iIpiAcceptCpus | s.iCpusComingUp | s.iCCReactivateCpus);

	// cores on and not being retired, plus cores being reactivated
	TUint32 c3 = c2 &~ c0;
	TUint32 cc_active = __e32_bit_count_32(c3);

	if (s.iCCRequestLevel != cc_active)
		{
		if (chrl || !(s.iCCState & (ECCReqPending|ECCPowerDownInProgress|ECCPowerUpInProgress) ))
			{
			kick = TRUE;
			}
		s.iCCState |= ECCReqPending;
		}
	s.iGenIPILock.UnlockIrqRestore(irq);
	if (kick)
		s.iCCRequestDfc.Add();
	NKern::Unlock();
	}






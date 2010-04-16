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
// e32test\dma\dmasim.cpp
// DMA framework Platform Specific Layer (PSL) for software-emulated
// DMA controller used for testing the DMA framework PIL.
// 
//

#include <drivers/dma.h>
#include <kernel/kern_priv.h>


const char KDmaPanicCat[] = "DMASIM";

const TInt KMaxTransferSize = 0x1FFF;
const TInt KMemAlignMask = 3; // memory addresses passed to DMAC must be multiple of 4
const TInt KBurstSize = 0x800;

typedef void (*TPseudoIsr)();

const TInt KChannelCount = 4;								// # of channels per controller
const TInt KDesCount = 256;									// # of descriptors allocated per controller

//////////////////////////////////////////////////////////////////////////////
// SOFTWARE DMA CONTROLLER SIMULATION
//////////////////////////////////////////////////////////////////////////////

class DmacSb
/** Single-buffer DMA controller software simulation */
	{
public:
	enum { ECsRun = 0x80000000 };
public:
	static void DoTransfer();
private:
	static void BurstTransfer();
private:
	static TInt CurrentChannel;
public:
	// pseudo registers
	static TUint8* SrcAddr[KChannelCount];
	static TUint8* DestAddr[KChannelCount];
	static TInt Count[KChannelCount];
	static TUint32 ControlStatus[KChannelCount];
	static TUint32 CompletionInt;
	static TUint32 ErrorInt;
	// hook for pseudo ISR
	static TPseudoIsr Isr;
	// transfer failure simulation
	static TInt FailCount[KChannelCount];
	};

TUint8* DmacSb::SrcAddr[KChannelCount];
TUint8* DmacSb::DestAddr[KChannelCount];
TInt DmacSb::Count[KChannelCount];
TUint32 DmacSb::ControlStatus[KChannelCount];
TUint32 DmacSb::CompletionInt;
TUint32 DmacSb::ErrorInt;
TPseudoIsr DmacSb::Isr;
TInt DmacSb::FailCount[KChannelCount];
TInt DmacSb::CurrentChannel;

void DmacSb::DoTransfer()
	{
	if (ControlStatus[CurrentChannel] & ECsRun)
		{
		if (FailCount[CurrentChannel] > 0 && --FailCount[CurrentChannel] == 0)
			{
			ControlStatus[CurrentChannel] &= ~ECsRun;
			ErrorInt |= 1 << CurrentChannel;
			Isr();
			}
		else
			{
			//__KTRACE_OPT(KDMA, Kern::Printf("DmacSb::DoTransfer channel %d", CurrentChannel));
			if (Count[CurrentChannel] == 0)
				{
				//__KTRACE_OPT(KDMA, Kern::Printf("DmacSb::DoTransfer transfer complete"));
				ControlStatus[CurrentChannel] &= ~ECsRun;
				CompletionInt |= 1 << CurrentChannel;
				Isr();
				}
			else
				BurstTransfer();
			}
		}

	CurrentChannel++;
	if (CurrentChannel >= KChannelCount)
		CurrentChannel = 0;
	}

void DmacSb::BurstTransfer()
	{
	//__KTRACE_OPT(KDMA, Kern::Printf("DmacSb::BurstTransfer"));
	TInt s = Min(Count[CurrentChannel], KBurstSize);
	memcpy(DestAddr[CurrentChannel], SrcAddr[CurrentChannel], s);
	Count[CurrentChannel] -= s;
	SrcAddr[CurrentChannel] += s;
	DestAddr[CurrentChannel] += s;
	}

//////////////////////////////////////////////////////////////////////////////

class DmacDb
/** Double-buffer DMA controller software simulation */
	{
public:
	enum { ECsRun = 0x80000000, ECsPrg = 0x40000000 };
public:
	static void Enable(TInt aIdx);
	static void DoTransfer();
private:
	static TInt CurrentChannel;
private:
	// internal pseudo-registers
	static TUint8* ActSrcAddr[KChannelCount];
	static TUint8* ActDestAddr[KChannelCount];
	static TInt ActCount[KChannelCount];
public:
	// externally accessible pseudo-registers
	static TUint32 ControlStatus[KChannelCount];
	static TUint8* PrgSrcAddr[KChannelCount];
	static TUint8* PrgDestAddr[KChannelCount];
	static TInt PrgCount[KChannelCount];
	static TUint32 CompletionInt;
	static TUint32 ErrorInt;
	// hook for pseudo ISR
	static TPseudoIsr Isr;
	// transfer failure simulation
	static TInt FailCount[KChannelCount];
	static TInt InterruptsToMiss[KChannelCount];
	};

TUint8* DmacDb::PrgSrcAddr[KChannelCount];
TUint8* DmacDb::PrgDestAddr[KChannelCount];
TInt DmacDb::PrgCount[KChannelCount];
TUint8* DmacDb::ActSrcAddr[KChannelCount];
TUint8* DmacDb::ActDestAddr[KChannelCount];
TInt DmacDb::ActCount[KChannelCount];
TUint32 DmacDb::ControlStatus[KChannelCount];
TUint32 DmacDb::CompletionInt;
TUint32 DmacDb::ErrorInt;
TPseudoIsr DmacDb::Isr;
TInt DmacDb::FailCount[KChannelCount];
TInt DmacDb::InterruptsToMiss[KChannelCount];
TInt DmacDb::CurrentChannel;

void DmacDb::Enable(TInt aIdx)
	{
	if (ControlStatus[aIdx] & ECsRun)
		ControlStatus[aIdx] |= ECsPrg;
	else
		{
		ActSrcAddr[aIdx] = PrgSrcAddr[aIdx];
		ActDestAddr[aIdx] = PrgDestAddr[aIdx];
		ActCount[aIdx] = PrgCount[aIdx];
		ControlStatus[aIdx] |= ECsRun;
		}
	}

void DmacDb::DoTransfer()
	{
	if (ControlStatus[CurrentChannel] & ECsRun)
		{
		if (FailCount[CurrentChannel] > 0 && --FailCount[CurrentChannel] == 0)
			{
			ControlStatus[CurrentChannel] &= ~ECsRun;
			ErrorInt |= 1 << CurrentChannel;
			Isr();
			}
		else
			{
			if (ActCount[CurrentChannel] == 0)
				{
				if (ControlStatus[CurrentChannel] & ECsPrg)
					{
					ActSrcAddr[CurrentChannel] = PrgSrcAddr[CurrentChannel];
					ActDestAddr[CurrentChannel] = PrgDestAddr[CurrentChannel];
					ActCount[CurrentChannel] = PrgCount[CurrentChannel];
					ControlStatus[CurrentChannel] &= ~ECsPrg;
					}
				else
					ControlStatus[CurrentChannel] &= ~ECsRun;
				if (InterruptsToMiss[CurrentChannel] > 0)
					InterruptsToMiss[CurrentChannel]--;
				else
					{
					CompletionInt |= 1 << CurrentChannel;
					Isr();
					}
				}
			else
				{
				TInt s = Min(ActCount[CurrentChannel], KBurstSize);
				memcpy(ActDestAddr[CurrentChannel], ActSrcAddr[CurrentChannel], s);
				ActCount[CurrentChannel] -= s;
				ActSrcAddr[CurrentChannel] += s;
				ActDestAddr[CurrentChannel] += s;
				}
			}
		}

	CurrentChannel++;
	if (CurrentChannel >= KChannelCount)
		CurrentChannel = 0;
	}


//////////////////////////////////////////////////////////////////////////////

class DmacSg
/** Scatter/gather DMA controller software simulation */
	{
public:
	enum { EChannelBitRun = 0x80000000 };
	enum { EDesBitInt = 1 };
	struct SDes
		{
		TUint8* iSrcAddr;
		TUint8* iDestAddr;
		TInt iCount;
		TUint iControl;
		SDes* iNext;
		};
public:
	static void DoTransfer();
	static void Enable(TInt aIdx);
private:
	static TInt CurrentChannel;
	static TBool IsDescriptorLoaded[KChannelCount];
public:
	// externally accessible pseudo-registers
	static TUint32 ChannelControl[KChannelCount];
	static TUint8* SrcAddr[KChannelCount];
	static TUint8* DestAddr[KChannelCount];
	static TInt Count[KChannelCount];
	static TUint Control[KChannelCount];
	static SDes* NextDes[KChannelCount];
	static TUint32 CompletionInt;
	static TUint32 ErrorInt;
	// hook for pseudo ISR
	static TPseudoIsr Isr;
	// transfer failure simulation
	static TInt FailCount[KChannelCount];
	static TInt InterruptsToMiss[KChannelCount];
	};

TUint32 DmacSg::ChannelControl[KChannelCount];
TUint8* DmacSg::SrcAddr[KChannelCount];
TUint8* DmacSg::DestAddr[KChannelCount];
TInt DmacSg::Count[KChannelCount];
TUint DmacSg::Control[KChannelCount];
DmacSg::SDes* DmacSg::NextDes[KChannelCount];
TUint32 DmacSg::CompletionInt;
TUint32 DmacSg::ErrorInt;
TPseudoIsr DmacSg::Isr;
TInt DmacSg::FailCount[KChannelCount];
TInt DmacSg::InterruptsToMiss[KChannelCount];
TInt DmacSg::CurrentChannel;
TBool DmacSg::IsDescriptorLoaded[KChannelCount];


void DmacSg::DoTransfer()
	{
	if (ChannelControl[CurrentChannel] & EChannelBitRun)
		{
		if (FailCount[CurrentChannel] > 0 && --FailCount[CurrentChannel] == 0)
			{
			ChannelControl[CurrentChannel] &= ~EChannelBitRun;
			ErrorInt |= 1 << CurrentChannel;
			Isr();
			}
		else
			{
			if (IsDescriptorLoaded[CurrentChannel])
				{
				if (Count[CurrentChannel] == 0)
					{
					IsDescriptorLoaded[CurrentChannel] = EFalse;
					if (Control[CurrentChannel] & EDesBitInt)
						{
						if (InterruptsToMiss[CurrentChannel] > 0)
							InterruptsToMiss[CurrentChannel]--;
						else
							{
							CompletionInt |= 1 << CurrentChannel;
							Isr();
							}
						}
					}
				else
					{
					TInt s = Min(Count[CurrentChannel], KBurstSize);
					memcpy(DestAddr[CurrentChannel], SrcAddr[CurrentChannel], s);
					Count[CurrentChannel] -= s;
					SrcAddr[CurrentChannel] += s;
					DestAddr[CurrentChannel] += s;
					}
				}
			// Need to test again as new descriptor must be loaded if
			// completion has just occured.
			if (! IsDescriptorLoaded[CurrentChannel])
				{
				if (NextDes[CurrentChannel] != NULL)
					{
					SrcAddr[CurrentChannel] = NextDes[CurrentChannel]->iSrcAddr;
					DestAddr[CurrentChannel] = NextDes[CurrentChannel]->iDestAddr;
					Count[CurrentChannel] = NextDes[CurrentChannel]->iCount;
					Control[CurrentChannel] = NextDes[CurrentChannel]->iControl;
					NextDes[CurrentChannel] = NextDes[CurrentChannel]->iNext;
					IsDescriptorLoaded[CurrentChannel] = ETrue;
					}
				else
					ChannelControl[CurrentChannel] &= ~EChannelBitRun;
				}
			}
		}

	CurrentChannel++;
	if (CurrentChannel >= KChannelCount)
		CurrentChannel = 0;
	}


void DmacSg::Enable(TInt aIdx)
	{
	SrcAddr[aIdx] = NextDes[aIdx]->iSrcAddr;
	DestAddr[aIdx] = NextDes[aIdx]->iDestAddr;
	Count[aIdx] = NextDes[aIdx]->iCount;
	Control[aIdx] = NextDes[aIdx]->iControl;
	NextDes[aIdx] = NextDes[aIdx]->iNext;
	IsDescriptorLoaded[aIdx] = ETrue;
	ChannelControl[aIdx] |= EChannelBitRun;
	}

//////////////////////////////////////////////////////////////////////////////

class DmacSim
/** 
 Harness calling the various DMA controller simulators periodically.
 */
	{
public:
	static void StartEmulation();
	static void StopEmulation();
	static TBool InISR();
	static void Synchronize();
private:
	enum { KPeriod = 1 }; // in ms
	enum { EDmaSimIdle=0u, EDmaSimStarted=1u, EDmaSimInISR=2u, EDmaSimStopping=0x80000000u };
	static void TickCB(TAny* aThis);
	static NTimer Timer;
	static volatile TInt StartStop;
	};

NTimer DmacSim::Timer;
volatile TInt DmacSim::StartStop;

void DmacSim::StartEmulation()
	{
	__DMA_ASSERTA(StartStop==EDmaSimIdle);
	new (&Timer) NTimer(&TickCB, 0);
	__e32_atomic_store_ord32(&StartStop, EDmaSimStarted);
	__DMA_ASSERTA(Timer.OneShot(KPeriod, EFalse) == KErrNone);
	}

void DmacSim::StopEmulation()
	{
	TInt orig = __e32_atomic_tas_ord32(&StartStop, (TInt)EDmaSimStarted, (TInt)EDmaSimStopping, 0);
	if (orig == EDmaSimIdle)
		return;		// wasn't running
	// loop until we succeed in cancelling the timer or the timer callback
	// notices that we are shutting down
	while (!Timer.Cancel() && __e32_atomic_load_acq32(&StartStop)!=EDmaSimIdle)
		{}
	__e32_atomic_store_ord32(&StartStop, EDmaSimIdle);
	}

void DmacSim::TickCB(TAny*)
	{
	TInt orig = (TInt)__e32_atomic_ior_acq32(&StartStop, EDmaSimInISR);
	if (orig >= 0)
		{
		DmacSb::DoTransfer();
		DmacDb::DoTransfer();
		DmacSg::DoTransfer();
		}
	orig = (TInt)__e32_atomic_and_rel32(&StartStop, (TUint32)~EDmaSimInISR);
	if (orig < 0)
		{
		__e32_atomic_store_rel32(&StartStop, EDmaSimIdle);
		return;
		}
	TInt r = Timer.Again(KPeriod);
	if (r == KErrArgument)
		r = Timer.OneShot(KPeriod);
	__DMA_ASSERTA(r == KErrNone);
	}

TBool DmacSim::InISR()
	{
	return __e32_atomic_load_acq32(&StartStop) & EDmaSimInISR;
	}

void DmacSim::Synchronize()
	{
	while (InISR())
		{}
	}

//////////////////////////////////////////////////////////////////////////////
// PSL FOR DMA SIMULATION
//////////////////////////////////////////////////////////////////////////////

class DSimSbController : public TDmac
	{
public:
	DSimSbController();
private:
	static void Isr();
	// from TDmac
	virtual void Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aHdr);
	virtual void StopTransfer(const TDmaChannel& aChannel);
	virtual TInt FailNext(const TDmaChannel& aChannel);
	virtual TBool IsIdle(const TDmaChannel& aChannel);
	virtual TInt MaxTransferSize(TDmaChannel& aChannel, TUint aFlags, TUint32 aPslInfo);
	virtual TUint MemAlignMask(TDmaChannel& aChannel, TUint aFlags, TUint32 aPslInfo);
public:
	static const SCreateInfo KInfo;
	TDmaSbChannel iChannels[KChannelCount];
	};

DSimSbController SbController;

const TDmac::SCreateInfo DSimSbController::KInfo =
	{
	KChannelCount,
	KDesCount,
	0,
	sizeof(SDmaPseudoDes),
	0,
	};

DSimSbController::DSimSbController()
	: TDmac(KInfo)
	{
	DmacSb::Isr = Isr;
	}


void DSimSbController::Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aHdr)
	{
	TUint32 i = aChannel.PslId();
	const SDmaPseudoDes& des = HdrToDes(aHdr);
	DmacSb::SrcAddr[i] = (TUint8*) des.iSrc;
	DmacSb::DestAddr[i] = (TUint8*) des.iDest;
	DmacSb::Count[i] = des.iCount;
	DmacSb::ControlStatus[i] |= DmacSb::ECsRun;
	}


void DSimSbController::StopTransfer(const TDmaChannel& aChannel)
	{
	__e32_atomic_and_ord32(&DmacSb::ControlStatus[aChannel.PslId()], (TUint32)~DmacSb::ECsRun);
	DmacSim::Synchronize();
	}


TInt DSimSbController::FailNext(const TDmaChannel& aChannel)
	{
	DmacSb::FailCount[aChannel.PslId()] = 1;
	return KErrNone;
	}


TBool DSimSbController::IsIdle(const TDmaChannel& aChannel)
	{
	return (DmacSb::ControlStatus[aChannel.PslId()] & DmacSb::ECsRun) == 0;
	}


TInt DSimSbController::MaxTransferSize(TDmaChannel& /*aChannel*/, TUint /*aFlags*/, TUint32 /*aPslInfo*/)
	{
	return KMaxTransferSize;
	}


TUint DSimSbController::MemAlignMask(TDmaChannel& /*aChannel*/, TUint /*aFlags*/, TUint32 /*aPslInfo*/)
	{
	return KMemAlignMask;
	}


void DSimSbController::Isr()
	{
	for (TInt i = 0; i < KChannelCount; i++)
		{
		TUint32 mask = (1 << i);
		if (DmacSb::CompletionInt & mask)
			{
			DmacSb::CompletionInt &= ~mask;
			HandleIsr(SbController.iChannels[i], ETrue);
			}
		if (DmacSb::ErrorInt & mask)
			{
			DmacSb::ErrorInt &= ~mask;
			HandleIsr(SbController.iChannels[i], EFalse);
			}
		}
	}

//////////////////////////////////////////////////////////////////////////////

class DSimDbController : public TDmac
	{
public:
	DSimDbController();
private:
	static void Isr();
	// from TDmac
	virtual void Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aHdr);
	virtual void StopTransfer(const TDmaChannel& aChannel);
	virtual TInt FailNext(const TDmaChannel& aChannel);
	virtual TInt MissNextInterrupts(const TDmaChannel& aChannel, TInt aInterruptCount);
	virtual TBool IsIdle(const TDmaChannel& aChannel);
	virtual TInt MaxTransferSize(TDmaChannel& aChannel, TUint aFlags, TUint32 aPslInfo);
	virtual TUint MemAlignMask(TDmaChannel& aChannel, TUint aFlags, TUint32 aPslInfo);
public:
	static const SCreateInfo KInfo;
	TDmaDbChannel iChannels[KChannelCount];
	};

DSimDbController DbController;

const TDmac::SCreateInfo DSimDbController::KInfo =
	{
	KChannelCount,
	KDesCount,
	0,
	sizeof(SDmaPseudoDes),
	0,
	};


DSimDbController::DSimDbController()
	: TDmac(KInfo)
	{
	DmacDb::Isr = Isr;
	}


void DSimDbController::Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aHdr)
	{
	TUint32 i = aChannel.PslId();
	const SDmaPseudoDes& des = HdrToDes(aHdr);
	DmacDb::PrgSrcAddr[i] = (TUint8*) des.iSrc;
	DmacDb::PrgDestAddr[i] = (TUint8*) des.iDest;
	DmacDb::PrgCount[i] = des.iCount;
	DmacDb::Enable(i);
	}


void DSimDbController::StopTransfer(const TDmaChannel& aChannel)
	{
	__e32_atomic_and_ord32(&DmacDb::ControlStatus[aChannel.PslId()], (TUint32)~(DmacDb::ECsRun|DmacDb::ECsPrg));
	DmacSim::Synchronize();
	}


TInt DSimDbController::FailNext(const TDmaChannel& aChannel)
	{
	DmacDb::FailCount[aChannel.PslId()] = 1;
	return KErrNone;
	}


TInt DSimDbController::MissNextInterrupts(const TDmaChannel& aChannel, TInt aInterruptCount)
	{
	__DMA_ASSERTD((DmacDb::ControlStatus[aChannel.PslId()] & DmacDb::ECsRun) == 0);
	__DMA_ASSERTD(aInterruptCount >= 0);
	// At most one interrupt can be missed with double-buffer controller
	if (aInterruptCount == 1)
		{
		DmacDb::InterruptsToMiss[aChannel.PslId()] = aInterruptCount;
		return KErrNone;
		}
	else
		return KErrNotSupported;
	}


TBool DSimDbController::IsIdle(const TDmaChannel& aChannel)
	{
	return (DmacDb::ControlStatus[aChannel.PslId()] & DmacDb::ECsRun) == 0;
	}


TInt DSimDbController::MaxTransferSize(TDmaChannel& /*aChannel*/, TUint /*aFlags*/, TUint32 /*aPslInfo*/)
	{
	return KMaxTransferSize;
	}


TUint DSimDbController::MemAlignMask(TDmaChannel& /*aChannel*/, TUint /*aFlags*/, TUint32 /*aPslInfo*/)
	{
	return KMemAlignMask;
	}


void DSimDbController::Isr()
	{
	for (TInt i = 0; i < KChannelCount; i++)
		{
		TUint32 mask = (1 << i);
		if (DmacDb::CompletionInt & mask)
			{
			DmacDb::CompletionInt &= ~mask;
			HandleIsr(DbController.iChannels[i], ETrue);
			}
		if (DmacDb::ErrorInt & mask)
			{
			DmacDb::ErrorInt &= ~mask;
			HandleIsr(DbController.iChannels[i], EFalse);
			}
		}
	}

//////////////////////////////////////////////////////////////////////////////

class DSimSgController : public TDmac
	{
public:
	DSimSgController();
private:
	static void Isr();
	// from TDmac
	virtual void Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aHdr);
	virtual void StopTransfer(const TDmaChannel& aChannel);
	virtual TBool IsIdle(const TDmaChannel& aChannel);
	virtual TInt MaxTransferSize(TDmaChannel& aChannel, TUint aFlags, TUint32 aPslInfo);
	virtual TUint MemAlignMask(TDmaChannel& aChannel, TUint aFlags, TUint32 aPslInfo);
	virtual void InitHwDes(const SDmaDesHdr& aHdr, TUint32 aSrc, TUint32 aDest, TInt aCount,
						   TUint aFlags, TUint32 aPslInfo, TUint32 aCookie);
	virtual void ChainHwDes(const SDmaDesHdr& aHdr, const SDmaDesHdr& aNextHdr);
	virtual void AppendHwDes(const TDmaChannel& aChannel, const SDmaDesHdr& aLastHdr,
							 const SDmaDesHdr& aNewHdr);
	virtual void UnlinkHwDes(const TDmaChannel& aChannel, SDmaDesHdr& aHdr);
	virtual TInt FailNext(const TDmaChannel& aChannel);
	virtual TInt MissNextInterrupts(const TDmaChannel& aChannel, TInt aInterruptCount);
private:
	inline DmacSg::SDes* HdrToHwDes(const SDmaDesHdr& aHdr);
public:
	static const SCreateInfo KInfo;
	TDmaSgChannel iChannels[KChannelCount];
	};

DSimSgController SgController;

const TDmac::SCreateInfo DSimSgController::KInfo =
	{
	KChannelCount,
	KDesCount,
	KCapsBitHwDes,
	sizeof(DmacSg::SDes),
#ifdef __WINS__
	0,
#else
	EMapAttrSupRw|EMapAttrFullyBlocking,
#endif
	};


inline DmacSg::SDes* DSimSgController::HdrToHwDes(const SDmaDesHdr& aHdr)
	{
	return static_cast<DmacSg::SDes*>(TDmac::HdrToHwDes(aHdr));
	}


DSimSgController::DSimSgController()
	: TDmac(KInfo)
	{
	DmacSg::Isr = Isr;
	}


void DSimSgController::Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aHdr)
	{
	TUint32 i = aChannel.PslId();
	DmacSg::NextDes[i] = HdrToHwDes(aHdr);
	DmacSg::Enable(i);
	}


void DSimSgController::StopTransfer(const TDmaChannel& aChannel)
	{
	__e32_atomic_and_ord32(&DmacSg::ChannelControl[aChannel.PslId()], (TUint32)~DmacSg::EChannelBitRun);
	DmacSim::Synchronize();
	}


void DSimSgController::InitHwDes(const SDmaDesHdr& aHdr, TUint32 aSrc, TUint32 aDest, TInt aCount,
								 TUint /*aFlags*/, TUint32 /*aPslInfo*/, TUint32 /*aCookie*/)
	{
	DmacSg::SDes& des = *HdrToHwDes(aHdr);
	des.iSrcAddr = reinterpret_cast<TUint8*>(aSrc);
	des.iDestAddr = reinterpret_cast<TUint8*>(aDest);
	des.iCount = static_cast<TInt16>(aCount);
	des.iControl |= DmacSg::EDesBitInt;
	des.iNext = NULL;
	}


void DSimSgController::ChainHwDes(const SDmaDesHdr& aHdr, const SDmaDesHdr& aNextHdr)
	{
	DmacSg::SDes& des = *HdrToHwDes(aHdr);
	des.iControl &= ~DmacSg::EDesBitInt;
	des.iNext = HdrToHwDes(aNextHdr);
	}


void DSimSgController::AppendHwDes(const TDmaChannel& aChannel, const SDmaDesHdr& aLastHdr,
								   const SDmaDesHdr& aNewHdr)
	{
	TUint32 i = aChannel.PslId();
	DmacSg::SDes* pNewDes = HdrToHwDes(aNewHdr);
	TInt prevLevel = NKern::DisableAllInterrupts();

	if ((DmacSg::ChannelControl[i] & DmacSg::EChannelBitRun) == 0)
		{
		DmacSg::NextDes[i] = pNewDes;
		DmacSg::Enable(i);
		}
	else if (DmacSg::NextDes[i] == NULL)
		DmacSg::NextDes[i] = pNewDes;
	else
		HdrToHwDes(aLastHdr)->iNext = pNewDes;

	NKern::RestoreInterrupts(prevLevel);
	}


void DSimSgController::UnlinkHwDes(const TDmaChannel& /*aChannel*/, SDmaDesHdr& aHdr)
	{
  	DmacSg::SDes* pD = HdrToHwDes(aHdr);
	pD->iNext = NULL;
	pD->iControl |= DmacSg::EDesBitInt;
	}


TInt DSimSgController::FailNext(const TDmaChannel& aChannel)
	{
	__DMA_ASSERTD((DmacSg::ChannelControl[aChannel.PslId()] & DmacSg::EChannelBitRun) == 0);
	DmacSg::FailCount[aChannel.PslId()] = 1;
	return KErrNone;
	}


TInt DSimSgController::MissNextInterrupts(const TDmaChannel& aChannel, TInt aInterruptCount)
	{
	__DMA_ASSERTD((DmacSg::ChannelControl[aChannel.PslId()] & DmacSg::EChannelBitRun) == 0);
	__DMA_ASSERTD(aInterruptCount >= 0);
	DmacSg::InterruptsToMiss[aChannel.PslId()] = aInterruptCount;
	return KErrNone;
	}


TBool DSimSgController::IsIdle(const TDmaChannel& aChannel)
	{
	return (DmacSg::ChannelControl[aChannel.PslId()] & DmacSg::EChannelBitRun) == 0;
	}


TInt DSimSgController::MaxTransferSize(TDmaChannel& /*aChannel*/, TUint /*aFlags*/, TUint32 /*aPslInfo*/)
	{
	return KMaxTransferSize;
	}


TUint DSimSgController::MemAlignMask(TDmaChannel& /*aChannel*/, TUint /*aFlags*/, TUint32 /*aPslInfo*/)
	{
	return KMemAlignMask;
	}


void DSimSgController::Isr()
	{
	for (TInt i = 0; i < KChannelCount; i++)
		{
		TUint32 mask = (1 << i);
		if (DmacSg::CompletionInt & mask)
			{
			DmacSg::CompletionInt &= ~mask;
			HandleIsr(SgController.iChannels[i], ETrue);
			}
		if (DmacSg::ErrorInt & mask)
			{
			DmacSg::ErrorInt &= ~mask;
			HandleIsr(SgController.iChannels[i], EFalse);
			}
		}
	}


//////////////////////////////////////////////////////////////////////////////
// Channel opening/closing

enum TController { ESb=0, EDb=1, ESg=2 };

const TUint32 KControllerMask = 0x30;
const TUint32 KControllerShift = 4;
const TUint32 KChannelIdxMask = 3;

#define MKCHN(type, idx) (((type)<<KControllerShift)|idx)

static TUint32 TestSbChannels[] = { MKCHN(ESb,0), MKCHN(ESb,1), MKCHN(ESb,2), MKCHN(ESb,3) };
static TUint32 TestDbChannels[] = { MKCHN(EDb,0), MKCHN(EDb,1), MKCHN(EDb,2), MKCHN(EDb,3) };
static TUint32 TestSgChannels[] = { MKCHN(ESg,0), MKCHN(ESg,1), MKCHN(ESg,2), MKCHN(ESg,3) };

static TDmaTestInfo TestInfo =
	{
	KMaxTransferSize,
	KMemAlignMask,
	0,
	KChannelCount,
	TestSbChannels,
	KChannelCount,
	TestDbChannels,
	KChannelCount,
	TestSgChannels,
	};

EXPORT_C const TDmaTestInfo& DmaTestInfo()
	{
	return TestInfo;
	}

// Keep track of opened channels so Tick callback used to fake DMA
// transfers is enabled only when necessary.
static TInt OpenChannelCount = 0;


TDmaChannel* DmaChannelMgr::Open(TUint32 aOpenId)
	{
	TInt dmac = (aOpenId & KControllerMask) >> KControllerShift;
	__DMA_ASSERTD(dmac < 3);
	TInt i = aOpenId & KChannelIdxMask;
	TDmaChannel* pC = NULL;
	TDmac* controller = NULL;
	switch (dmac)
		{
	case ESb:
		pC = SbController.iChannels + i;
		controller = &SbController;
		break;
	case EDb:
		pC = DbController.iChannels + i;
		controller = &DbController;
		break;
	case ESg:
		pC = SgController.iChannels + i;
		controller = &SgController;
		break;
	default:
		__DMA_CANT_HAPPEN();
		}

	if (++OpenChannelCount == 1)
		{
		__KTRACE_OPT(KDMA, Kern::Printf("Enabling DMA simulation"));
		DmacSim::StartEmulation();
		}
	if (pC->IsOpened())
		return NULL;
	pC->iController = controller;
	pC->iPslId = i;
	return pC;
	}


void DmaChannelMgr::Close(TDmaChannel* /*aChannel*/)
	{
	if (--OpenChannelCount == 0)
		{
		DmacSim::StopEmulation();
		__KTRACE_OPT(KDMA, Kern::Printf("Stopping DMA simulation"));
		}
	}

TInt DmaChannelMgr::StaticExtension(TInt /*aCmd*/, TAny* /*aArg*/)
	{
	return KErrNotSupported;
	}

//////////////////////////////////////////////////////////////////////////////

//
// On hardware, this code is inside a kernel extension.
//

DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KDMA, Kern::Printf("Starting DMA simulator..."));
	TInt r;
	r = SbController.Create(DSimSbController::KInfo);
	if (r != KErrNone)
		return r;
	r = DbController.Create(DSimDbController::KInfo);
	if (r != KErrNone)
		return r;
	r = SgController.Create(DSimSgController::KInfo);
	if (r != KErrNone)
		return r;

	return KErrNone;
	}

//
// On WINS, this code is inside a LDD (see mmp file) so we need some
// bootstrapping code to call the kernel extension entry point.
//

class DDummyLdd : public DLogicalDevice
	{
public:
	// from DLogicalDevice
	TInt Install();
	void GetCaps(TDes8& aDes) const;
	TInt Create(DLogicalChannelBase*& aChannel);
	};

TInt DDummyLdd::Create(DLogicalChannelBase*& aChannel)
    {
	aChannel=NULL;
	return KErrNone;
    }

TInt DDummyLdd::Install()
    {
	_LIT(KLddName, "DmaSim");
    TInt r = SetName(&KLddName);
	if (r == KErrNone)
		r = InitExtension();
	return r;
    }

void DDummyLdd::GetCaps(TDes8& /*aDes*/) const
    {
    }

EXPORT_C DLogicalDevice* CreateLogicalDevice()
	{
    return new DDummyLdd;
	}


//---

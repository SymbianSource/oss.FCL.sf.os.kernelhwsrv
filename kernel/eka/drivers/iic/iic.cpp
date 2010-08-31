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
// e32\drivers\iic.cpp
// IIC Controller and public API Implementation
//

#include <drivers/iic.h>
#include <drivers/iic_channel.h>
#include "iic_priv.h"

#ifdef IIC_INSTRUMENTATION_MACRO
#include <drivers/iic_trace.h>
#endif

// Global Controller pointer
static DIicBusController* TheController = NULL;

#ifdef IIC_SIMULATED_PSL
DIicBusController*& gTheController = TheController;
#endif

//
//		Implementation of generic IicBus API for client interface
//
EXPORT_C TInt IicBus::QueueTransaction(TInt aBusId, TIicBusTransaction* aTransaction)
	{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MQTRANSSYNC_START_PIL_TRACE;
#endif
	TInt r=TheController->QueueTransaction(aBusId, aTransaction);

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MQTRANSSYNC_END_PIL_TRACE;
#endif
	return r;
	}

EXPORT_C TInt IicBus::QueueTransaction(TInt aBusId, TIicBusTransaction* aTransaction, TIicBusCallback* aCallback)
	{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MQTRANSASYNC_START_PIL_TRACE;
#endif
	TInt r=TheController->QueueTransaction(aBusId, aTransaction, aCallback);

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MQTRANSASYNC_END_PIL_TRACE;
#endif
	return r;
	}

EXPORT_C TInt IicBus::CancelTransaction(TInt aBusId, TIicBusTransaction* aTransaction)
	{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MCANCELTRANS_START_PIL_TRACE;
#endif
	TInt r=TheController->CancelTransaction(aBusId, aTransaction);

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MCANCELTRANS_END_PIL_TRACE;
#endif
	return r;
	}

EXPORT_C TInt IicBus::CaptureChannel(TInt aBusId, TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch)
	{
#ifdef IIC_INSTRUMENTATION_MACRO
	if(!aAsynch)
		{
		IIC_SCAPTCHANSYNC_START_PIL_TRACE;
		}
	else
		{
		IIC_SCAPTCHANASYNC_START_PIL_TRACE;
		}
#endif
	TInt r=TheController->CaptureChannel(aBusId, aConfigHdr, aCallback, aChannelId, aAsynch);

#ifdef IIC_INSTRUMENTATION_MACRO
	if(!aAsynch)
		{
		IIC_SCAPTCHANSYNC_END_PIL_TRACE;
		}
#endif
	return r;
	}

EXPORT_C TInt IicBus::ReleaseChannel(TInt aChannelId)
	{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SRELCHAN_START_PIL_TRACE;
#endif
	TInt r=TheController->ReleaseChannel(aChannelId);

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SRELCHAN_END_PIL_TRACE;
#endif
	return r;
	}

EXPORT_C TInt IicBus::RegisterRxBuffer(TInt aChannelId, TPtr8 aRxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset)
	{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SREGRXBUF_START_PIL_TRACE;
#endif
	TInt r=TheController->RegisterRxBuffer(aChannelId, aRxBuffer, aBufGranularity, aNumWords, aOffset);

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SREGRXBUF_END_PIL_TRACE;
#endif
	return r;
	}

EXPORT_C TInt IicBus::RegisterTxBuffer(TInt aChannelId, TPtr8 aTxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset)
	{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SREGTXBUF_START_PIL_TRACE;
#endif
	TInt r=TheController->RegisterTxBuffer(aChannelId, aTxBuffer, aBufGranularity, aNumWords, aOffset);

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SREGTXBUF_END_PIL_TRACE;
#endif
	return r;
	}

EXPORT_C TInt IicBus::SetNotificationTrigger(TInt aChannelId, TInt aTrigger)
	{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SNOTIFTRIG_START_PIL_TRACE;
#endif
	TInt r=TheController->SetNotificationTrigger(aChannelId, aTrigger);

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SNOTIFTRIG_END_PIL_TRACE;
#endif
	return r;
	}

EXPORT_C TInt IicBus::StaticExtension(TUint aId, TUint aFunction, TAny* aParam1, TAny* aParam2)
	{
	return(TheController->StaticExtension(aId, aFunction, aParam1, aParam2));
	}


//
//		Bus Controller
//

//  auxiliary function for ordering entries in the array of channels
TInt DIicBusController::OrderEntries(const DIicBusChannel& aMatch, const DIicBusChannel& aEntry)
	{
	TUint8 l=(TUint8)aMatch.ChannelNumber();
	TUint8 r=(TUint8)aEntry.ChannelNumber();
	if(l>r)
		return -1;
	else if(l<r)
		return 1;
	else
		return 0;
	}

// global ordering object to be passed to RPointerArray InsertInOrderXXX and FindInOrder
TLinearOrder<DIicBusChannel> EntryOrder(DIicBusController::OrderEntries);

// Implementation for DIicBusController
//

TInt DIicBusController::Create()
	{
	TInt r=KErrNone;
	iChanLock = new TSpinLock(TSpinLock::EOrderGenericIrqLow2);  // Semi-arbitrary, low priority value
	iCaptLock = new TSpinLock(TSpinLock::EOrderGenericIrqLow2);  // Semi-arbitrary, low priority value
	if((iChanLock == NULL)||(iCaptLock == NULL))
		{
		delete iChanLock;
		delete iCaptLock;
		r=KErrNoMemory;
		}
	return r;
	}

DIicBusController::~DIicBusController()
	{
#ifdef IIC_SIMULATED_PSL
	for(TInt i=0; i<iChannelArray.Count(); i++)
		{
		DIicBusChannel* ptr=iChannelArray[i];
		// Remove the channel from the array
		iChannelArray.Remove(i);
		// Delete the channel object
		delete ptr;
		};

	iChannelArray.Reset();
	delete iChanLock;
	delete iCaptLock;
#endif
	}

TInt DIicBusController::GetChanWriteAccess()
	{
	// Can only have one insertion or removal active at any one time
	// Can not perform an insertion or removal while a read is in progress
	// If either of the two above conditions exist, return KErrInUse
	// Otherwise, set the flag to indicate that a write is in progress
	// and return KErrNone.
	TInt chanIntState=0;
	chanIntState=__SPIN_LOCK_IRQSAVE(*iChanLock);
	if(iChanRwFlags != 0)
		{
		__SPIN_UNLOCK_IRQRESTORE(*iChanLock,chanIntState);
		return KErrInUse;
		}
	iChanRwFlags |= EWriteInProgress;
	__SPIN_UNLOCK_IRQRESTORE(*iChanLock,chanIntState);
	return KErrNone;
	}

void DIicBusController::FreeChanWriteAccess()
	{
	// If an insertion or removal is in progress, no other modifying operation
	// can be active. Reads are also not permitted - so iChanRwFlags can only be
	// EWriteInProgress.
	__ASSERT_DEBUG(iChanRwFlags == EWriteInProgress, Kern::Fault(KIicPanic,__LINE__));
	TInt chanIntState=0;
 	chanIntState=__SPIN_LOCK_IRQSAVE(*iChanLock);
 	iChanRwFlags &= ~EWriteInProgress;
 	__SPIN_UNLOCK_IRQRESTORE(*iChanLock,chanIntState);
	}

TInt DIicBusController::GetChanReadAccess()
	{
	// No reads are permitted while an insertion or removal is in progress
	// If one of the above operations is in progress return KErrInUse
	// Can have several concurrent reads at any one time - so increment
	// the count of such operations as well as ensuring the flag is set to indicate
	// a read is in progress
	TInt chanIntState=0;
	chanIntState=__SPIN_LOCK_IRQSAVE(*iChanLock);
	if(iChanRwFlags == EWriteInProgress)
		{
		__SPIN_UNLOCK_IRQRESTORE(*iChanLock,chanIntState);
		return KErrInUse;
		}
	__ASSERT_DEBUG(iChanRdCount!=0xFFFFFFFF, Kern::Fault(KIicPanic,__LINE__)); // Overflow
	iChanRdCount++;
	iChanRwFlags |= EReadInProgress;
	__SPIN_UNLOCK_IRQRESTORE(*iChanLock,chanIntState);
	return KErrNone;
	}

void DIicBusController::FreeChanReadAccess()
	{
	// No insertions or removals are permitted while a read is in progress
	// so iChanRwFlags can only be EReadInProgress
	// Multiple reads can be in progress concurrently, so the count must be decremented
	TInt chanIntState=0;
	chanIntState=__SPIN_LOCK_IRQSAVE(*iChanLock);
	__ASSERT_DEBUG(iChanRwFlags == EReadInProgress, Kern::Fault(KIicPanic,__LINE__));
	__ASSERT_DEBUG(iChanRdCount>0, Kern::Fault(KIicPanic,__LINE__));
	iChanRdCount--;
	if(iChanRdCount == 0)
		iChanRwFlags &= ~EReadInProgress;
	__SPIN_UNLOCK_IRQRESTORE(*iChanLock,chanIntState);
	}

TInt DIicBusController::RequestTypeSupported(const TInt aBusId, DIicBusChannelMaster* const aChannel)
	{
	TInt32 reqBusType;
	reqBusType = GET_BUS_TYPE(aBusId);
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::RequestTypeSupported, BusType=0x%x\n", reqBusType));

	if(reqBusType != aChannel->BusType())
		{
		return KErrNotSupported;
		}

	return KErrNone;
	}


EXPORT_C TInt DIicBusController::RegisterChannels(DIicBusChannel** aListChannels, TInt aNumberChannels)
	{
// To be used by Channel implementations to register a list of supported channels
    __KTRACE_OPT(KIIC, Kern::Printf("\nDIicBusController::RegisterChannels, aListChannels=0x%x, aNumberChannels=%d\n",aListChannels,aNumberChannels));
	__ASSERT_DEBUG(aListChannels!=NULL, Kern::Fault(KIicPanic,__LINE__));

	RPointerArray<DIicBusChannel>* chanArray = TheController->ChannelArray();

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_REGISTERCHANS_START_PIL_TRACE;
#endif
	// Get access to the channel pointer array - exit if it is currently being modfied
	TInt r=KErrNone;
	if((r=TheController->GetChanWriteAccess()) == KErrNone)
		{
#ifdef _DEBUG
		__KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::RegisterChannels - On entry, iChannelArray ...\n"));
		TheController->DumpChannelArray();
#endif
		// Loop for aNumberChannels	and write directly to the channel array
		DIicBusChannel** chanIterator = aListChannels;
		for(TInt iteration = 0; iteration < aNumberChannels; ++iteration, ++chanIterator)
			{
			DIicBusChannel* chanPtr = *chanIterator;
			__KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::RegisterChannels - adding channel number %d\n",chanPtr->ChannelNumber()));
			TInt r = chanArray->InsertInOrder(chanPtr,EntryOrder);
			if(r!=KErrNone)
				break;
			}

#ifdef _DEBUG
		 __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::RegisterChannels - On exit, iChannelArray ...\n"));
		TheController->DumpChannelArray();
#endif
		TheController->FreeChanWriteAccess();
		}
	else
		{
		__KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::GetChanWriteAccess returned %d\n",r));
		}

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_REGISTERCHANS_END_PIL_TRACE;
#endif
	return r;
	}


EXPORT_C TInt DIicBusController::DeRegisterChannel(DIicBusChannel* aChannel)
	{
// To be used by Channel implementations to deregister a channel
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::DeRegisterChannel, aChannel=0x%x\n",aChannel));
	if(aChannel == NULL)
		return KErrArgument;

	RPointerArray<DIicBusChannel>* chanArray = TheController->ChannelArray();

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_DEREGISTERCHAN_START_PIL_TRACE;
#endif
	TInt r=KErrNone;
	// Get access to the channel pointer array - exit if it is currently unavailable
	// Gaining write access will prevent a client of a Master Channel from instigating a new QueueTransaction
	// (or CancelTransaction), and it will obstruct a client of a Slave Channel in CaptureChannel.
	if((r=TheController->GetChanWriteAccess())!=KErrNone)
		return r;

	// Check channel is registered
	TInt chanIndex = chanArray->FindInOrder(aChannel,EntryOrder);
	if(chanIndex<0)
		{
		TheController->FreeChanWriteAccess();
		return KErrNotFound;
		}

#ifdef _DEBUG
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::DeRegisterChannel - On entry, iChannelArray ...\n"));
	TheController->DumpChannelArray();
#endif

	// Remove the channel from the array
	// Note that this does not delete the channel object
	chanArray->Remove(chanIndex);

#ifdef _DEBUG
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::DeRegisterChannel - On exit, iChannelArray ...\n"));
	TheController->DumpChannelArray();
#endif
	TheController->FreeChanWriteAccess();

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_DEREGISTERCHAN_END_PIL_TRACE;
#endif
	return KErrNone;
	}

TInt DIicBusController::FindCapturedChanById(TCapturedChannel aCapturedChan, TInt& aIndex)
	{
	TInt index=0;
	TInt r=KErrNotFound;
	do
		{
		if(iCapturedChannels[index].iChannelId == aCapturedChan.iChannelId)
			{
			aIndex=index;
			r=KErrNone;
			}
		index++;
		} while ((index < KMaxNumCapturedChannels)&&(r == KErrNotFound));
	return r;
	}

TInt DIicBusController::FindCapturedChan(TCapturedChannel aCapturedChan, TInt& aIndex)
	{
	TInt index=0;
	TInt r=KErrNotFound;
	do
		{
		if(iCapturedChannels[index] == aCapturedChan)
			{
			aIndex=index;
			r=KErrNone;
			}
		index++;
		} while ((index < KMaxNumCapturedChannels)&&(r == KErrNotFound));
	return r;
	}

TInt DIicBusController::InsertCaptChanInArray(TCapturedChannel aCapturedChan)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::InsertCaptChanInArray \n"));
	// Ensure the channel hasn't already been inserted in the array
	// If found, fault the Kernel
	TInt dumInt = 0;
	TInt r=FindCapturedChan(aCapturedChan,dumInt);
	__ASSERT_DEBUG(r!=KErrNone, Kern::Fault(KIicPanic,__LINE__));

	// Loop the array and insert in the first available slot
	// If no slots are available return KErrNotReady
	TInt index=0;
	TCapturedChannel emptyChan;
	for(;index<KMaxNumCapturedChannels;++index)
		{
		if(iCapturedChannels[index] == emptyChan)
			{
			// Found a space
			iCapturedChannels[index]=aCapturedChan;
			break;
			}
		}
	if(index>=KMaxNumCapturedChannels)
		r = KErrNotReady;
	return r;
	}

TInt DIicBusController::RemoveCaptChanFromArray(TCapturedChannel aCapturedChan)
	{
	// Remove the entry from the array
	// If the entry is not present return KErrArgument
	TInt index=-1;
	TInt r=FindCapturedChan(aCapturedChan,index);
	if((r!=KErrNone)||(index>=KMaxNumCapturedChannels))
		return KErrArgument;
	iCapturedChannels[index].iChanPtr=NULL;
	iCapturedChannels[index].iChannelId=0;
	return KErrNone;
	}


TInt DIicBusController::InstallCapturedChannel(const TInt aChannelId, const DIicBusChannelSlave* aChanPtr)
	{
#ifdef _DEBUG
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::InstallCapturedChannel - On entry, iCapturedChannels ...\n"));
	DumpCapturedChannels();
#endif
	TInt r=KErrNone;
	TCapturedChannel capturedChan((TInt)aChannelId,(DIicBusChannelSlave*)aChanPtr);
	// Because insertions are bounded by the size of the array and do not involve allocating
	// or freeing memory, simply take the spinlock at the start of the operation and release at the end
	TInt captIntState=__SPIN_LOCK_IRQSAVE(*iCaptLock);
	r=InsertCaptChanInArray(capturedChan);
	__SPIN_UNLOCK_IRQRESTORE(*iCaptLock,captIntState);
	if(r!=KErrNone)
		return r;

#ifdef _DEBUG
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::InstallCapturedChannel - On exit, iCapturedChannels ...\n"));
	DumpCapturedChannels();
#endif
	return KErrNone;
	}

TInt DIicBusController::DeInstallCapturedChannel(const TInt aChannelId, const DIicBusChannelSlave* aChanPtr)
	{
#ifdef _DEBUG
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::DeInstallCapturedChannel - On entry, iCapturedChannels ...\n"));
	DumpCapturedChannels();
#endif
	TInt r = KErrNone;
	TCapturedChannel capturedChan((TInt) aChannelId, (DIicBusChannelSlave*) aChanPtr);
	// Because removals are bounded by the size of the array and do not involve allocating
	// or freeing memory, simply take the spinlock at the start of the operation and release at the end
	TInt captIntState = __SPIN_LOCK_IRQSAVE(*iCaptLock);
	r = RemoveCaptChanFromArray(capturedChan);
	__SPIN_UNLOCK_IRQRESTORE(*iCaptLock, captIntState);
	if(r != KErrNone)
		return r;

#ifdef _DEBUG
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::DeInstallCapturedChannel - On exit, iCapturedChannels ...\n"));
	DumpCapturedChannels();
#endif
	return KErrNone;
	}

	// Master-side API
TInt DIicBusController::QueueTransaction(TInt aBusId, TIicBusTransaction* aTransaction)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::QueueTransaction, aBusId=0x%x,aTransaction=0x%x\n", aBusId, aTransaction));
	if(!aTransaction)
		{
		return KErrArgument;
		}

	// Get a pointer to the channel
	TInt dumInt = 0;
	DIicBusChannel* chanPtr = NULL;
	// Can only read the channel array if it is not currently being modified
	TInt r = GetChanReadAccess();
	if(r != KErrNone)
		{
		return r;
		}
	r = GetChanPtr(aBusId, dumInt, chanPtr);
	if(r == KErrNone)
		{
		if(!chanPtr)
			{
			r = KErrArgument;
			}
		else
			{
			switch(chanPtr->ChannelType())
				{
				// QueueTransaction requests are only supported by channels in Master mode.
				case DIicBusChannel::ESlave:
					{
					r = KErrNotSupported;
					break;
					}
				// If the request is supported by the Master channel, send it to the channel for processing in its thread
				case DIicBusChannel::EMasterSlave:
					{
					r = RequestTypeSupported(aBusId, ((DIicBusChannelMasterSlave*)chanPtr)->iMasterChannel);
					if(r == KErrNone)
						{
						aTransaction->iBusId = aBusId;
						r = (((DIicBusChannelMasterSlave*) chanPtr)->QueueTransaction(aTransaction));
						}
					break;
					}
				case DIicBusChannel::EMaster:
					{
					r = RequestTypeSupported(aBusId, (DIicBusChannelMaster*)chanPtr);
					if(r == KErrNone)
						{
						aTransaction->iBusId = aBusId;
						r = (((DIicBusChannelMaster*) chanPtr)->QueueTransaction(aTransaction));
						}
					break;
					}
				default:
					{
					r = KErrGeneral;
					}
				}
			}
		}
	FreeChanReadAccess();
	return r;
	}

TInt DIicBusController::QueueTransaction(TInt aBusId, TIicBusTransaction* aTransaction, TIicBusCallback* aCallback)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::QueueTransaction, aBusId=0x%x,aTransaction=0x%x,aCallback=0x%x\n",aBusId,aTransaction,aCallback));
	if(!aTransaction || !aCallback)
		{
		return KErrArgument;
		}

	// Get a pointer to the channel
	TInt dumInt = 0;
	DIicBusChannel* chanPtr = NULL;
	// Can only read the channel array if it is not currently being modified
	TInt r = GetChanReadAccess();
	if(r == KErrNone)
		{
		r = GetChanPtr(aBusId, dumInt, chanPtr);
		if(r == KErrNone)
			{
			if(!chanPtr)
				{
				r = KErrArgument;
				}
			else
				{
				switch(chanPtr->ChannelType())
					{
					// QueueTransaction requests are only supported by channels in Master mode.
					case DIicBusChannel::ESlave:
						{
						r = KErrNotSupported;
						break;
						}
					// If the request is supported by the Master channel, send it to the channel for processing in its thread
					case DIicBusChannel::EMasterSlave:
						{
						r = RequestTypeSupported(aBusId, ((DIicBusChannelMasterSlave*)chanPtr)->iMasterChannel);
						if(r == KErrNone)
							{
							aTransaction->iBusId = aBusId;
							r = (((DIicBusChannelMasterSlave*) chanPtr)->QueueTransaction(aTransaction, aCallback));
							}
						break;
						}
					case DIicBusChannel::EMaster:
						{
						r = RequestTypeSupported(aBusId, (DIicBusChannelMaster*)chanPtr);
						if(r == KErrNone)
							{
							aTransaction->iBusId = aBusId;
							r = (((DIicBusChannelMaster*) chanPtr)->QueueTransaction(aTransaction, aCallback));
							}
						break;
						}
					default:
						{
						r = KErrGeneral;
						}
					}
				}
			}
		}
	FreeChanReadAccess();
	return r;
	}


TInt DIicBusController::GetChanPtr(const TInt aBusId, TInt &aIndex, DIicBusChannel*& aChan)
	{
    __KTRACE_OPT(KIIC, 	Kern::Printf("DIicBusController::GetChanPtr, aBusId=0x%x\n",aBusId));

	TInt32 chanId;
	chanId = GET_CHAN_NUM(aBusId);

	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::GetChanPtr, chanId=0x%x\n", chanId));
	DIicBusChannelSearcher searchChannel(DIicBusChannel::EMasterSlave, DIicBusChannel::ESccb, DIicBusChannel::EFullDuplex);
	searchChannel.SetChannelNumber((TInt8)chanId);

	TInt r = KErrNotFound;
	aIndex = iChannelArray.FindInOrder(&searchChannel, EntryOrder);
	if(aIndex >= 0)
		{
		aChan = iChannelArray[aIndex];
		r = KErrNone;
		}

	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::GetChanPtr, chanPtr=0x%x, index=%d\n", aChan, aIndex));
	return r;
	}


TInt DIicBusController::CancelTransaction(TInt aBusId, TIicBusTransaction* aTransaction)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::CancelTransaction, aBusId=0x%x,aTransaction=0x%x\n", aBusId, aTransaction));
	if(!aTransaction)
		{
		return KErrArgument;
		}

	// Get the channel
	TInt dumInt = 0;
	DIicBusChannel* chanPtr = NULL;

	// Can only read the channel array if it is not currently being modified
	TInt r = GetChanReadAccess();
	if(r == KErrNone)
		{
		r = GetChanPtr(aBusId, dumInt, chanPtr);
		if(r == KErrNone)
			{
			if(!chanPtr)
				{
				r = KErrArgument;
				}
			else
				{
				// QueueTransaction requests are only supported by channels in Master mode.
				switch(chanPtr->ChannelType())
					{
					case DIicBusChannel::ESlave:
						{
						r = KErrNotSupported;
						break;
						}
					case DIicBusChannel::EMasterSlave:
						{
						r = RequestTypeSupported(aBusId, ((DIicBusChannelMasterSlave*)chanPtr)->iMasterChannel);
						if(r == KErrNone)
							{
							r = (((DIicBusChannelMasterSlave*) chanPtr)->CancelTransaction(aTransaction));
							}
						break;
						}
					case DIicBusChannel::EMaster:
						{
						r = RequestTypeSupported(aBusId, (DIicBusChannelMaster*)chanPtr);
						if(r == KErrNone)
							{
							r = (((DIicBusChannelMaster*) chanPtr)->CancelTransaction(aTransaction));
							}
						break;
						}
					default:
						{
						r = KErrGeneral;
						}
					}
				}
			}
		}
	FreeChanReadAccess();
	return r;
	}

	// Slave-side API
TInt DIicBusController::CaptureChannel(TInt aBusId, TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch)
	{
	// Check that that aCallback!=NULL and aConfigHdr!=NULL - if not, return KErrArgument
	if(!aCallback || !aConfigHdr)
		{
		return KErrArgument;
		}

	// Get the channel
	TInt chanIndex = 0;
	DIicBusChannel* chanPtr = NULL;

	// Can only read the channel array if it is not currently being modified
	TInt r = GetChanReadAccess();
	if(r == KErrNone)
		{
		r = GetChanPtr(aBusId, chanIndex, chanPtr);
		if(r == KErrNone)
			{
			if(!chanPtr)
				{
				r = KErrArgument;
				}
			else
				{
				DIicBusChannelSlave* slaveChanPtr = NULL;
				switch(chanPtr->ChannelType())
					{
					// CaptureChannel requests are only supported by channels in Slave mode.
					case DIicBusChannel::EMaster:
						{
						r = KErrNotSupported;
						break;
						}
					case DIicBusChannel::EMasterSlave:
						{
						slaveChanPtr = ((DIicBusChannelMasterSlave*) chanPtr)->iSlaveChannel;
						__ASSERT_DEBUG(slaveChanPtr!=NULL, Kern::Fault(KIicPanic,__LINE__)); // MasterSlave channel should have a valid Slave channel
						// Send the request to the channel
						slaveChanPtr->iController = this;
						r = ((DIicBusChannelMasterSlave*) chanPtr)->CaptureChannel(aConfigHdr, aCallback, aChannelId, aAsynch);
						break;
						}
					case DIicBusChannel::ESlave:
						{
						slaveChanPtr = (DIicBusChannelSlave*) chanPtr; // chanPtr is non-NULL
						// Send the request to the channel
						slaveChanPtr->iController = this;
						r = (slaveChanPtr->CaptureChannel(aConfigHdr, aCallback, aChannelId, aAsynch));
						break;
						}
					default:
						{
						r = KErrArgument;
						}
					}
				// For synchronous capture, if successful then install the channel
				if(r == KErrNone && slaveChanPtr)
					{
					if(!aAsynch)
						{
						InstallCapturedChannel(aChannelId, slaveChanPtr);
						}
					}
				}
			}
		}
	FreeChanReadAccess();
	return r;
	}


TInt DIicBusController::GetSlaveChanPtr(TInt aChannelId, DIicBusChannelSlave*& aSlaveChanPtr)
	{
	TInt r=KErrNone;
	// Check that the channelID is recognised
	TCapturedChannel capturedChan(aChannelId,NULL);
	TInt chanIndex=-1;
	// Ensure the array of captured channels will not be modified before it has been searched
	// Because searches are bounded by the size of the array and do not involve allocating
	// or freeing memory, simply take the spinlock at the start of the operation and release at the end
	TInt captIntState=__SPIN_LOCK_IRQSAVE(*iCaptLock);
	r=FindCapturedChanById(capturedChan, chanIndex);
	if((chanIndex < 0)||(r == KErrNotFound))
		r=KErrArgument;
	else
		aSlaveChanPtr = (DIicBusChannelSlave*)(iCapturedChannels[chanIndex].iChanPtr);
	__SPIN_UNLOCK_IRQRESTORE(*iCaptLock,captIntState);

	__ASSERT_DEBUG(aSlaveChanPtr!=NULL, Kern::Fault(KIicPanic,__LINE__));
	return r;
	}


TInt DIicBusController::ReleaseChannel(TInt aChannelId)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::ReleaseChannel, channelID = 0x%x \n",aChannelId));
	TInt r = KErrNone;
	DIicBusChannel* chanPtr = NULL;
	
	// Get the pointer to the Slave Channel
	DIicBusChannelSlave* slaveChanPtr = NULL;
	if((r = GetSlaveChanPtr(aChannelId, slaveChanPtr)) != KErrNone)
		return r;
		
	DIicBusChannelSearcher searchChannel(DIicBusChannel::EMasterSlave, DIicBusChannel::ESccb, DIicBusChannel::EFullDuplex);
	searchChannel.SetChannelNumber(slaveChanPtr->ChannelNumber());

	TInt dumIndex = iChannelArray.FindInOrder(&searchChannel, EntryOrder);
	if(dumIndex < 0)
		{
		return KErrNotFound;
		}
	chanPtr = iChannelArray[dumIndex];

	__ASSERT_DEBUG(chanPtr!=NULL, Kern::Fault(KIicPanic,__LINE__));

	//if it is the masterslave channel, then call the masterslave's RelaseChannel
	// which will call the slave channel's ReleaseChannel internally
	if(chanPtr->ChannelType() == DIicBusChannel::EMasterSlave)
		r = ((DIicBusChannelMasterSlave*)chanPtr)->ReleaseChannel();
	else // Call the slave only ReleaseChannel
		r = slaveChanPtr->ReleaseChannel();
	
	// In either case de-install the captured slave channel
	if(r == KErrNone)
		{
		r = DeInstallCapturedChannel(aChannelId, slaveChanPtr);
		}

	// No need to unset iController - there is only one IIC Controller
	return r;
	}


TInt DIicBusController::RegisterRxBuffer(TInt aChannelId, TPtr8 aRxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::RegisterRxBuffer, channelID=0x%x,aRxBuffer=0x%x,aBufGranularity=0x%x,aNumWords=0x%x,aOffset=0x%x \n",aChannelId,(TInt)&aRxBuffer,aBufGranularity,aNumWords,aOffset));

    // Acquire the pointer to the Slave Channel
	DIicBusChannelSlave* slaveChanPtr = NULL;
	TInt r = GetSlaveChanPtr(aChannelId, slaveChanPtr);
	if(r != KErrNone)
		{
		return r;
		}

	// Instigate the channel functionality
	return(slaveChanPtr->RegisterRxBuffer(aRxBuffer,aBufGranularity,aNumWords,aOffset));
	}

TInt DIicBusController::RegisterTxBuffer(TInt aChannelId, TPtr8 aTxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::RegisterTxBuffer, channelID=0x%x,aTxBuffer=0x%x,aBufGranularity=0x%x,aNumWords=0x%x,aOffset=0x%x \n",aChannelId,(TInt)&aTxBuffer,aBufGranularity,aNumWords,aOffset));

	// Acquire the pointer to the Slave Channel
	DIicBusChannelSlave* slaveChanPtr = NULL;
	TInt r = GetSlaveChanPtr(aChannelId, slaveChanPtr);
	if(r != KErrNone)
		{
		return r;
		}

	// Instigate the channel functionality
	return (slaveChanPtr->RegisterTxBuffer(aTxBuffer, aBufGranularity, aNumWords, aOffset));
	}


TInt DIicBusController::SetNotificationTrigger(TInt aChannelId, TInt aTrigger)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusController::SetNotificationTrigger - for aChannelId=0x%x, aTrigger=0x%x\n",aChannelId,aTrigger));
	// Acquire the pointer to the Slave Channel
	DIicBusChannelSlave* slaveChanPtr = NULL;
	TInt r = GetSlaveChanPtr(aChannelId, slaveChanPtr);
	if( r != KErrNone)
		{
		return r;
		}

	// Instigate the channel functionality
	return(slaveChanPtr->SetNotificationTrigger(aTrigger));
	}


TInt DIicBusController::StaticExtension(TUint aId, TUint aFunction, TAny* aParam1, TAny* aParam2)
	{
//		The IIC controller and channel classes are generic, and can serve many differing client and
//		bus implementations. If a client and bus make use of specific functionality that is not
//		common to other bus types, it makes sense to provide only the minimum-required support in the
//		generic code. Here, the channel identifier is checked but all other parameters are passed
//		directly to the bus implementation channel for processing; if the channel does not provide
//		StaticExtension implementation, the generic DIicBusChannel::StaticExtension method is invoked.

#ifdef IIC_INSTRUMENTATION_MACRO
	if((aFunction & KControlIoMask) == KMasterSlaveControlIo)
		{
		IIC_MSSTATEXT_START_PIL_TRACE
		}
	else if((aFunction & KControlIoMask) == KMasterControlIo)
		{
		IIC_MSTATEXT_START_PIL_TRACE
		}
	else if((aFunction & KControlIoMask) == KSlaveControlIo)
		{
		IIC_SSTATEXT_START_PIL_TRACE
		}
//	else - Unexpected value - just pass silently to the PSL ...
#endif

	// Get the channel
	TInt dumInt = 0;
	DIicBusChannel* chanPtr = NULL;
	// Can only read the channel array if it is not currently being modified
	TInt r = GetChanReadAccess();
	if(r == KErrNone)
		{
		r = GetChanPtr(aId, dumInt, chanPtr);
		if(r == KErrNone)
			{
			if(!chanPtr)
				{
				r = KErrArgument;
				}
			else
				{
				r = chanPtr->StaticExtension(aFunction, aParam1, aParam2);
				}
			}
		}

#ifdef IIC_INSTRUMENTATION_MACRO
	if((aFunction & KControlIoMask) == KMasterSlaveControlIo)
		{
		IIC_MSSTATEXT_START_PIL_TRACE
		}
	else if((aFunction & KControlIoMask) == KMasterControlIo)
		{
		IIC_MSTATEXT_START_PIL_TRACE
		}
	else if((aFunction & KControlIoMask) == KSlaveControlIo)
		{
		IIC_SSTATEXT_START_PIL_TRACE
		}
//	else	... do nothing
#endif
	FreeChanReadAccess();
	return r;
	}


#ifdef _DEBUG

void DIicBusController::DumpCapturedChannels()
	{
	// Print iCapturedChannels ...
	TInt count=0;
	TInt i=0;
	TCapturedChannel emptyChan;
	for(;i<KMaxNumCapturedChannels;++i)
		{
		if(iCapturedChannels[i] == emptyChan)
			continue;
		++count;
		}

	i = 0;
    __KTRACE_OPT(KIIC, Kern::Printf("	- Count gave %d\n",count));
	for(;i<KMaxNumCapturedChannels;++i)
		{
		if(iCapturedChannels[i] == emptyChan)
			continue;
		DIicBusChannel* ptr=(DIicBusChannel*)(iCapturedChannels[i]).iChanPtr;
	    __KTRACE_OPT(KIIC, Kern::Printf("	- ptr %d=0x%x\n",i,ptr));
		ptr->StaticExtension(KCtrlIoDumpChan,0,0);
		};
	}

void DIicBusController::DumpChannelArray()
	{
	TInt i = 0;
	__KTRACE_OPT(KIIC, Kern::Printf("\nDIicBusController::DumpChannelArray\n"));
    __KTRACE_OPT(KIIC, Kern::Printf("	- Count gave %d\n",iChannelArray.Count()));
	for(i=0; i<iChannelArray.Count(); i++)
		{
		DIicBusChannel* ptr=iChannelArray[i];
	    __KTRACE_OPT(KIIC, Kern::Printf("	- ptr %d=0x%x\n",i,ptr));
		ptr->StaticExtension(KCtrlIoDumpChan,0,0);
		};
	}

#endif

#ifndef IIC_SIMULATED_PSL

// Client interface entry point
DECLARE_EXTENSION_WITH_PRIORITY(KExtensionMaximumPriority-1)	// highest priority after Resource Manager
	{
	TheController = new DIicBusController;
	if(!TheController)
		return KErrNoMemory;
	TInt r=TheController->Create();
	return r;
	}
#endif



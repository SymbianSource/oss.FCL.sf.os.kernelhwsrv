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
// e32\drivers\iic_channel.cpp
// IIC Channel Platform Independent Layer (PIL)
//

#include <drivers/iic_channel.h>
#ifdef IIC_INSTRUMENTATION_MACRO
#include <drivers/iic_trace.h>
#endif

// The timer call back function which calls the PSL's HandleSlaveTimeout()
// Note that this assumes that the channel thread has been unblocked - if this
// is not the case, the callback will never get to run

TInt DIicBusChannelMaster::DoCreate()
    {return KErrNone;}

void DIicBusChannelMaster::Lock()
    {NKern::FMWait(&iTransactionQLock);}

void DIicBusChannelMaster::Unlock()
    {NKern::FMSignal(&iTransactionQLock);}

TIicBusTransaction* DIicBusChannelMaster::NextTrans(TIicBusTransaction* aTrans)
    {
    // call multi-transaction call back function to get next transaction
    if((aTrans->iFlags&KTransactionWithPreamble)&&(aTrans->iFlags&KTransactionWithMultiTransc))
        return ((TIicBusTransactionPreambleExt*)aTrans)->iMultiTransc(aTrans, ((TIicBusTransactionPreambleExt*)aTrans)->iMultiTranscArg);
    else if(aTrans->iFlags&KTransactionWithMultiTransc)
        return ((TIicBusTransactionMultiTransc*)aTrans)->iMultiTransc(aTrans, ((TIicBusTransactionMultiTransc*)aTrans)->iMultiTranscArg);
    else
        return NULL;
    }

void DIicBusChannelMaster::UnlockAndKick()
    {iTransQDfc.Enque(&iTransactionQLock);}

void DIicBusChannelMaster::SlaveTimeoutCallback(TAny* aPtr)
	{

	DIicBusChannelMaster* aChanMaster=(DIicBusChannelMaster* )aPtr;
	TInt r = aChanMaster->HandleSlaveTimeout();
	aChanMaster->CompleteRequest(r);
	}

TInt DIicBusChannelMaster::TransFlow(TIicBusTransaction* aTransaction)
    {
    if(aTransaction->iHalfDuplexTrans == NULL)
        return KErrArgument;
    else if(aTransaction->iFullDuplexTrans == NULL)
        return DIicBusChannel::EHalfDuplex;
    else return DIicBusChannel::EFullDuplex;
    }

TInt8 DIicBusChannelMaster::IsMasterBusy()
    {
    if((iTransCount&~KTransCountMsBit) == 0)
        return 0;
    else return 1;
    }

DIicBusChannelMaster::DIicBusChannelMaster(TBusType aBusType, TChannelDuplex aChanDuplex)
		: DIicBusChannel(DIicBusChannel::EMaster, aBusType, aChanDuplex),
		iTransQDfc(DIicBusChannelMaster::MsgQFunc, this, NULL, 1), iChannelReady(EFalse)
	{
	new(&iTimeoutTimer) NTimer(SlaveTimeoutCallback,this);
	}

DIicBusChannelMaster::~DIicBusChannelMaster()
	{
	delete iSlaveTimeoutDfc;
	}

TInt DIicBusChannelMaster::Init()
	{
	iSlaveTimeoutDfc = new TDfc(SlaveTimeoutCallback,(TAny*)this, 7);	// Highest Dfc priority
	if(!iSlaveTimeoutDfc)
		return KErrNoMemory;
	else
		return KErrNone;
	}

// Function to used to indicate if the Slave response has exceeded
// an expected time
TInt DIicBusChannelMaster::StartSlaveTimeOutTimer(TInt aTime)
	{
	TInt r = iTimeoutTimer.OneShot(NKern::TimerTicks(aTime),(*iSlaveTimeoutDfc));
	return r;
	}

void DIicBusChannelMaster::SetDfcQ(TDfcQue* aDfcQue)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::SetDfcQ 0x%x\n",aDfcQue));
	__ASSERT_DEBUG(aDfcQue!=NULL, Kern::Fault(KIicChannelPanic,__LINE__));
	iDfcQ=aDfcQue;
	iTransQDfc.SetDfcQ(iDfcQ);
	iSlaveTimeoutDfc->SetDfcQ(iDfcQ);
	Lock();
	__ASSERT_DEBUG(!iChannelReady, Kern::Fault(KIicChannelPanic,__LINE__));
	if (!iTransactionQ.IsEmpty())
		{
		iTransaction=(TIicBusTransaction*)(iTransactionQ.First()->Deque());
		__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::SetDfcQ got %08x",iTransaction));
		iTransaction->iState=TIicBusTransaction::EAccepted;
		iCurrentTransaction = iTransaction;
		UnlockAndKick();
		}
	else
		{
		__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::SetDfcQ"));
		iChannelReady=ETrue;
		iTransaction=NULL;
		iCurrentTransaction = NULL;
		Unlock();
		}
	}

void DIicBusChannelMaster::CompleteRequest(TInt aResult)
	{
	// Ensure the timeout timer has been cancelled
	CancelTimeOut();

	TIicBusTransaction* nextTrans=NextTrans(iCurrentTransaction);

	if((aResult != KErrNone)||(nextTrans == NULL))
		EndTransaction(iTransaction,aResult,iTransaction->iCallback);
	else
		{
		nextTrans->iBusId = iCurrentTransaction->iBusId; // Pass the bus configuration info to the PSL
		iCurrentTransaction = nextTrans;
		DoRequest(nextTrans);
		}
	}

#ifdef MASTER_MODE

/*
For Master-side transaction queuing APIs
the Channel implementation sends the transaction as a message to the Channel's message queue,
optionally blocking the client thread on the message's semaphore (synchronous APIs).
*/
TInt DIicBusChannelMaster::QueueTransaction(TIicBusTransaction* aTransaction)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::QueueTransaction, aTransaction=0x%x\n",aTransaction));
	// Send the transaction as a message to the Channel's message queue
	// Synchronous API, so block the calling thread during the processing
	TInt r = QueueTransaction(aTransaction, NULL);
	if(r!=KErrNone)
		return r;	// Transaction was not queued - so don't wait for a notification that it completed.

	__KTRACE_OPT(KIIC, Kern::Printf("<DIicBusChannelMaster::QueueTransaction ret %d",aTransaction->iResult));
	return aTransaction->iResult;
	}

TInt DIicBusChannelMaster::QueueTransaction(TIicBusTransaction* aTransaction, TIicBusCallback* aCallback)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::QueueTransaction, aTransaction=0x%x, aCallback=0x%x\n",aTransaction,aCallback));

	// Check aTransaction is non-NULL (aCallback may be NULL if the synchronous operation is required).
	if(aTransaction == NULL)
		{
		return KErrArgument;
		}

	// Send the transaction as a message to the Channel's message queue and return
	aTransaction->iCallback = aCallback;
	if(aCallback != NULL)
		{
		aCallback->iTransaction = aTransaction;
		}

	// Call the PSL implementation to check that the header is valid for this channel
	TInt r = CheckHdr(aTransaction->iHeader);
	if(r!=KErrNone)
		{
		return r;
		}

	// Duplex operation is indicated in the transaction object
	if((TransFlow((TIicBusTransaction*)aTransaction) == DIicBusChannel::EFullDuplex) &&
	   (ChannelDuplex()                              != DIicBusChannel::EFullDuplex))
		{
		return KErrNotSupported;
		}

	DThread* pC =& Kern::CurrentThread();
	Lock();
	__ASSERT_DEBUG(aTransaction->iState == TIicBusTransaction::EFree, Kern::Fault(KIicChannelPanic,__LINE__));
	if(!(iTransCount & KTransCountMsBit))
		{
		if(iTransCount < ~KTransCountMsBit)
			{
			++iTransCount;
			}
		else
			{
			Unlock();
			return KErrOverflow;
			}
		}

	aTransaction->iSyncNotification.iCount = 0;
	aTransaction->iSyncNotification.iOwningThread = &pC->iNThread;
	pC->Open();
	if (iChannelReady)
		{
		aTransaction->iState = TIicBusTransaction::EAccepted;
		iTransaction = aTransaction;
		iCurrentTransaction = aTransaction;
		iChannelReady = EFalse;
		UnlockAndKick();
		}
	else
		{
		iTransactionQ.Add(aTransaction);
		aTransaction->iState = TIicBusTransaction::EDelivered;
		Unlock();
		}

	// Wait on a semaphore if called from synchronous version
	if(aCallback == NULL)
		{
		NKern::FSWait(&aTransaction->iSyncNotification);
		}

	return KErrNone;
	}

TInt DIicBusChannelMaster::CancelTransaction(TIicBusTransaction* aTransaction)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::CancelTransaction, aTransaction=0x%x\n",aTransaction));

	// Check aTransaction is non-NULL
	if(aTransaction == NULL)
		{
		return KErrArgument;
		}
	// If the method is called on a synchronous transaction return KErrNotSupported
	if(aTransaction->iCallback == NULL)
		{
		return KErrNotSupported;
		}
	DThread* pT = NULL;
	Lock();

	TInt r = KErrNone;
	switch(aTransaction->iState)
		{
		case TIicBusTransaction::EDelivered:
			{
			aTransaction->Deque();
			pT=_LOFF(aTransaction->iSyncNotification.iOwningThread,DThread,iNThread);
			aTransaction->iState=TIicBusTransaction::EFree;
			--iTransCount; // Count must be greater than zero if the transaction is in this state
			r = KErrCancel;
			break;
			}

		case TIicBusTransaction::EAccepted:
			{
			r = KErrInUse;
			break;
			}

		case TIicBusTransaction::EFree:
			{
			r = KErrCancel;
			break;
			}
		}
	Unlock();
	if (pT)
		{
		pT->AsyncClose();
		}

	return r;
	}

#else /*MASTER_MODE*/

TInt DIicBusChannelMaster::QueueTransaction(TIicBusTransaction* /*aTransaction*/)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::QueueTransaction invoked when not in MASTER_MODE!\n"));
	return KErrNotSupported;
	}

TInt DIicBusChannelMaster::QueueTransaction(TIicBusTransaction* /*aTransaction*/, TIicBusCallback* /*aCallback*/)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::QueueTransaction invoked when not in MASTER_MODE!\n"));
	return KErrNotSupported;
	}

TInt DIicBusChannelMaster::CancelTransaction(TIicBusTransaction* /*aTransaction*/)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::CancelTransaction invoked when not in MASTER_MODE!\n"));
	return KErrNotSupported;
	}
#endif/*MASTER_MODE*/

// Invoked in response to receiving a message
// Function argument is a pointer to the required channel object
// Invoke the channel's PSL implementation of the DoRequest method with a pointer to the transaction object
//
void DIicBusChannelMaster::MsgQFunc(TAny* aPtr)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::MsgQFunc, aPtr=0x%x\n",aPtr));
	DIicBusChannelMaster* channel=(DIicBusChannelMaster*)aPtr;
	TIicBusTransaction* trans = channel->iTransaction;

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MPROCESSTRANS_START_PIL_TRACE;
#endif

    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::MsgQFunc trans->iHeader=0x%x\n",trans->iHeader));
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::MsgQFunc trans->iHalfDuplexTrans=0x%x\n",trans->iHalfDuplexTrans));
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::MsgQFunc trans->iFullDuplexTrans=0x%x\n",trans->iFullDuplexTrans));
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::MsgQFunc trans->iCallback=0x%x\n",trans->iCallback));
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMaster::MsgQFunc trans->iFlags=0x%x\n",trans->iFlags));

	TInt r = KErrNone;
	// invoke the preamble callback function supplied by the client of the IIC if there is any
	if(GetTransFlags(trans) & KTransactionWithPreamble)
		{
		TIicBusTransactionPreamble* transPreamble = (TIicBusTransactionPreamble*)trans;
		TIicBusPreamble funcPtr=NULL;
		funcPtr=(GetPreambleFuncPtr(transPreamble));
		funcPtr(transPreamble,GetPreambleFuncArg(transPreamble));
		}
	r = channel->DoRequest(trans);	// Instigate processing in the PSL
	if(r!=KErrNone)
		channel->EndTransaction(trans, r, trans->iCallback);
	}

void DIicBusChannelMaster::EndTransaction(TIicBusTransaction* aTrans, TInt aResult, TIicBusCallback* aCb)
	{
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_MPROCESSTRANS_END_PIL_TRACE;
#endif
	Complete(aResult,aTrans);
	if(aCb != NULL)
		{
		aCb->iResult = aResult;
		aCb->Enque();
		}
	}

void DIicBusChannelMaster::CancelTimeOut()
	{
	// Silently cancel the timer and associated DFC
	//
	// NTimer::Cancel returns ETrue if cancelled, EFalse otherwise - which may mean it wasn't active
	// TDfc::Cancel returns ETrue if actually de-queued, EFalse otherwise - which may mean it wasn't queued
	//
	iTimeoutTimer.Cancel();
	iSlaveTimeoutDfc->Cancel();
	}

void DIicBusChannelMaster::Complete(TInt aResult, TIicBusTransaction* aTransaction) //Completes a kernel message and receive the next one
	{
	__KTRACE_OPT(KIIC, Kern::Printf("MsgB::Complete %08x, %d",this,aResult));
	Lock();
	__ASSERT_DEBUG(aTransaction->iState == TIicBusTransaction::EAccepted, Kern::Fault(KIicChannelPanic,__LINE__));
	aTransaction->iResult=aResult;
	aTransaction->iState=TIicBusTransaction::EFree;
	--iTransCount;
	DThread* pT=_LOFF(aTransaction->iSyncNotification.iOwningThread,DThread,iNThread);
	__ASSERT_DEBUG(!iChannelReady, Kern::Fault(KIicChannelPanic,__LINE__));
	if (!iTransactionQ.IsEmpty())
		{
		TIicBusTransaction* pM=(TIicBusTransaction*)iTransactionQ.First()->Deque();
		__KTRACE_OPT(KIIC, Kern::Printf("rxnext: got %08x",pM));
		pM->iState=TIicBusTransaction::EAccepted;
		iTransaction = pM;
		iCurrentTransaction = pM;
		iTransQDfc.Enque();
		}
	else
		{
		__KTRACE_OPT(KIIC, Kern::Printf("rxnext"));
		iChannelReady=ETrue;
		iTransaction=NULL;
		iCurrentTransaction = NULL;
		}
	NKern::FSSignal(&aTransaction->iSyncNotification,&iTransactionQLock);
	pT->AsyncClose();
	}

TInt DIicBusChannelMaster::StaticExtension(TUint /*aFunction*/, TAny* /*aParam1*/, TAny* /*aParam*/)
 	{
 	return KErrNotSupported;
 	}

TInt DIicBusChannelMaster::Spare1(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}

#ifdef SLAVE_MODE

TInt DIicBusChannelSlave::CaptureChannel(TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch)
	{
	// Only one client can have access to the Slave channel at any one time. Any subsequent attempts to capture the
	// same channel should return an error.
	//
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::CaptureChannel\n"));
	if((aConfigHdr == NULL) || (aCallback == NULL))
		{
	    __KTRACE_OPT(KIIC, Kern::Printf("ERROR: non-NULL argument aConfigHdr=0x%x, aCallback=0x%x\n",aConfigHdr,aCallback));
		return KErrArgument;
		}
	//
	// Check the header is valid for the channel
	TInt r = CheckHdr(aConfigHdr);
	if(r == KErrNone)
		{
		// Check Slave channel is available for capture
		// If iChannelInUse is not set, capture should succeed
		// If this Slave channel is part of a MasterSlave channel iChannelInUse will already be set
		// but iClient will still be NULL. In this case, the capture should succeed.
		TInt intState=__SPIN_LOCK_IRQSAVE(iSpinLock);
		DThread* pT=&(Kern::CurrentThread());
		if((iChannelInUse)&&(iClient!=NULL))
			r=KErrInUse;
		else
			{
			iChannelInUse=1;
			iClient=pT;
			}
		__SPIN_UNLOCK_IRQRESTORE(iSpinLock,intState);

		if(r == KErrNone)
			{
			iClient->Open();
			aCallback->iChannel=this;
			iNotif = aCallback;
			iConfigHeader=aConfigHdr;	// Header alread checked, so just assign it

			// Invoke the PSL processing
			if(aAsynch)
				{
				aChannelId = 0; // the client should read iChannelId from the callback object.
				r=DoRequest(EAsyncConfigPwrUp);
				}
			else
				r=DoRequest(ESyncConfigPwrUp);

			if(r == KErrNone)
				{
				if(!aAsynch)	// For asynchronous version there is nothing more to do until the callback is invoked
					{
					SetChannelId(aChannelId);
					iClientTimeoutDfc->SetDfcQ(iNotif->iDfcQ);
					}
				}
			else
				{
				// PSL encountered an error
				ReleaseChannel();
				if(aAsynch)
					CompleteAsynchCapture(r);	// Queue the client callback for execution
				}
			}
		}
	return r;
	}

TInt DIicBusChannelSlave::ReleaseChannel()
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::ReleaseChannel\n"));
	 // Release a previously-captured channel.
	TInt r=KErrNone;;
	// Ensure that only the channel's client may release the channel
	DThread* pT=&(Kern::CurrentThread());
	if(iClient!=pT)			// Direct access since iClient can't be modified while channel is still captured
		return KErrAccessDenied;

	r=SetNotificationTrigger(0);			// Attempt to clear notification requests
	if((r!=KErrNone)&&(r!=KErrTimedOut))	// KErrTimedOut refers to an earlier transaction, and is for information only
		return r;
	StopTimer();
	r=DoRequest(EPowerDown);
	if(r == KErrNone)
		{
		TInt intState=__SPIN_LOCK_IRQSAVE(iSpinLock);
		iClient=NULL;
		iChannelInUse=0;	// Channel now available for capture by other clients
		__SPIN_UNLOCK_IRQRESTORE(iSpinLock,intState);
		pT->AsyncClose();	// Allow Client thread to close now channel has been released
		}
	else
		{
		// PSL error when releasing the channel - have to assume the hardware has a problem.
		// The channel is no longer considered "captured" by the controller, i.e. will not accept commands
		// But not having cleared the busy flag means that it can not be used for master channel
		// operations if it is part of a MasterSlave channel
		// Must Fault the Kernel.
		__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::ReleaseChannel - PSL returned error code %d\n",r));
		__ASSERT_ALWAYS(EFalse, Kern::Fault(KIicChannelPanic,__LINE__));
		}
	return r;
	}

TInt DIicBusChannelSlave::RegisterRxBuffer(TPtr8 aRxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::RegisterRxBuffer\n"));
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SREGRXBUF_START_PIL_TRACE;
#endif
	TInt r=KErrNone;
	// Ensure that only the channel's client may perform this operation
	DThread* pT=&(Kern::CurrentThread());
	if(iClient!=pT)			// Direct access since iClient can't be modified while channel is still captured
		return KErrAccessDenied;
	//If the buffer pointer is NULL, return KErrArgument
	if(aRxBuffer.Ptr() == NULL)
		return KErrArgument;
	// If a buffer is already registered, a subsequent request to do the same should return KErrAlreadyExists
	// This will be the case if SetNotificationTrigger has been invoked with any of ERxAllBytes, ERxUnderrun or ERxOverrun
	if(iReqTrig&(ERxAllBytes|ERxUnderrun|ERxOverrun))
		r=KErrAlreadyExists;
	else
		{
		iRxBuf=(TInt8*)(aRxBuffer.Ptr());
		iRxGranularity=aBufGranularity;
		iNumRxWords=aNumWords;
		iRxOffset=aOffset;
		}
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SREGRXBUF_END_PIL_TRACE;
#endif
	 return r;
	}

TInt DIicBusChannelSlave::RegisterTxBuffer(TPtr8 aTxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::RegisterTxBuffer	- default implementation\n"));
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SREGTXBUF_START_PIL_TRACE;
#endif
	TInt r=KErrNone;
	// Ensure that only the channel's client may perform this operation
	DThread* pT=&(Kern::CurrentThread());
	if(iClient!=pT)			// Direct access since iClient can't be modified while channel is still captured
		return KErrAccessDenied;
	//If the buffer pointer is NULL, return KErrArgument
	if(aTxBuffer.Ptr() == NULL)
		return KErrArgument;
	// If a buffer is already registered and a request is pending, a subsequent request to register a buffer should return
	// KErrAlreadyExists
	// This will be the case if SetNotificationTrigger has been invoked with any of ETxAllBytes, ETxUnderrun or ETxOverrun
	if(iReqTrig&(ETxAllBytes|ETxUnderrun|ETxOverrun))
		r=KErrAlreadyExists;
	else
		{
		iTxBuf=(TInt8*)(aTxBuffer.Ptr());
		iTxGranularity=aBufGranularity;
		iNumTxWords=aNumWords;
		iTxOffset=aOffset;
		}
#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SREGTXBUF_END_PIL_TRACE;
#endif
	return r;
	}

TInt DIicBusChannelSlave::SetNotificationTrigger(TInt aTrigger)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::SetNotificationTrigger invoked with aTrigger=0x%x\n",aTrigger));
	// Ensure that only the channel's client may perform this operation
	DThread* pT=&(Kern::CurrentThread());
	if(iClient!=pT)			// Direct access since iClient can't be modified while channel is still captured
		return KErrAccessDenied;

	TInt retVal = KErrNone;
	TInt trigger = aTrigger;
	switch (iTimerState)	// Handle existing timer conditions
		{
		case DIicBusChannelSlave::EInactive:
			{
			// In this state no timers have been started - so no action required
			break;
			}
		case DIicBusChannelSlave::EWaitForClient:
			{
			// Client has responded within the given time period, so stop the timer.
			StopTimer();
			break;
			}
		case DIicBusChannelSlave::EWaitForMaster:
			{
			// If both Rx and Tx events had been requested, and if ERxOverrun had occurred, the Client
			// may have called this function with new requests for Rx notifications, in order to
			// continue reading data sent by the Master. At this point, all Rx request flags in iReqTrig
			// will have been cleared.
			// If both Rx and Tx events had been requested, and if ETxUnderrun had occurred, the Client
			// may have called this function with new requests for Tx notifications, in order to
			// continue sending data to the Master. At this point, all Tx request flags in iReqTrig will
			// have been cleared.
			//
			// To handle the ERxOverrun situation, aTrigger may specify only the new Rx trigger settings,
			// or it may also re-specify the exisiting Tx settings. Similarly for the ETxUnderrun, aTrigger
			// may specify only the new Tx triggers, or both the new Tx triggers and the existing Rx triggers.
			//
			// However, if Rx flags are still set in iReqTrig, a request to change the Rx settings
			// will be rejected (similarly, for Tx).
			//
			// If the requested notification is zero, which would represent an attempt to clear all triggers
			// while the Master may have commenced a transfer, the request will be rejected.
			__ASSERT_DEBUG(iReqTrig != 0, Kern::Fault(KIicChannelPanic,__LINE__));
			if(trigger == 0)
				{
				return KErrInUse;
				}
			TInt allRxFlags = ERxAllBytes | ERxOverrun | ERxUnderrun;
			TInt allTxFlags = ETxAllBytes | ETxOverrun | ETxUnderrun;
			// Check the Rx flags
			TInt rxTrig = iReqTrig & allRxFlags;
			TInt reqRxTrig = trigger & allRxFlags;
			if(rxTrig == 0)
				{
				rxTrig = reqRxTrig;
				}
			else if(reqRxTrig != 0)
				{
				// New Rx triggers specified - check that Client is not attempting to modify the existing
				// settings
				if(rxTrig ^ reqRxTrig)
					{
					// Attempting to change the trigger settings - so reject the request
					return KErrInUse;
					}
				}
			// Check the Tx flags
			TInt txTrig = iReqTrig & allTxFlags;
			TInt reqTxTrig = trigger & allTxFlags;
			if(txTrig == 0)
				{
				txTrig = reqTxTrig;
				}
			else if(reqTxTrig != 0)
				{
				// New Tx triggers specified - check that Client is not attempting to modify the existing
				// settings
				if(txTrig ^ reqTxTrig)
					{
					// Attempting to change the trigger settings - so reject the request
					return KErrInUse;
					}
				}
			// Udate iReqTrig for the new requested trigger
			// and cancel the timer - since we are now starting a new transfer, we should
			// allow the Master the time to perform it
			trigger = rxTrig | txTrig;
			StopTimer();
			break;
			}
		case DIicBusChannelSlave::EClientTimeout:
			{
			// The Client did not respond within the expected time for the previous transfer. As a result,
			// the transaction will have been terminated for the Client.
			// Set the return value to inform the Client that it previously exceeded the expected response time
			retVal = KErrTimedOut;
			break;
			}
		default:
			{
			__ASSERT_DEBUG(0, Kern::Fault(KIicChannelPanic,__LINE__));
			break;
			}
		}
	// Ensure that requests for notification of asynchronous capture of channel is removed, since this
	// is not a valid event to request (the channel will already have been captured to get this far).
	// Also ensure that requests for EGeneralBusError are removed, since they are redundant (a notification
	// for a bus error is unconditional) and just represent overhead.
	trigger &= ~(EAsyncCaptChan | EGeneralBusError);

	iReqTrig = (TInt8)trigger; 				// Not atomic access since only client thread modifies iReqTrig
	iAccumTrig = 0;						// New transfer, so initialise accumulated event record
	TInt reqFlags=0;
	// Overrun and/or underrun may be requested if Client is unsure how much data is to follow,
	// so need to instigate Rx/Tx operation for any such request
	if(iReqTrig & (ERxOverrun|ERxUnderrun|ERxAllBytes))
		{
		reqFlags |= EReceive;
		}
	if(iReqTrig & (ETxOverrun|ETxUnderrun|ETxAllBytes))
		{
		reqFlags |= ETransmit;
		}
	TInt r = DoRequest(reqFlags);
	if(r != KErrNone)
		{
		// PSL encountered an error in intiating the requested trigger. Set the return value accordingly.
		// Assume triggers have been cancelled - if they have not, the client-provided callback will still
		// be invoked, but it will have been warned to expecte erroneous behaviour by the value assigned to retVal.
		iReqTrig = 0;
		retVal = KErrGeneral;
		}
	else	// PSL accepted the request, so update timer and state information
		{
		switch (iTimerState)
			{
			case DIicBusChannelSlave::EInactive:
				{
				// Do not start the timer. Must wait for the Master to access a Slave buffer before considering
				// a transaction as started.
				break;
				}
			case DIicBusChannelSlave::EWaitForClient:
				{
				// Client has responded within the given time period. The next state is
				// dependent on the requested trigger - if set to zero, the Client is explicitly
				// ending the transaction, so the next state is EInactive; otherwise, the
				// Client has indicated the next action expected from the Master and so the
				// timer is started and next state is EWaitForMaster
				if(iReqTrig == 0)
					{
					iTimerState = DIicBusChannelSlave::EInactive;
					}
				else
					{
					iTimerState = DIicBusChannelSlave::EWaitForMaster;
					StartTimerByState();
					}
				break;
				}
			case DIicBusChannelSlave::EClientTimeout:
				{
				// For the previous transfer, the Client failed to respond within the required time - and
				// the PSL will have been instructed to indicate a bus error (so the Master
				// will have been informed). The error code returned by this function will be KErrTimedOut
				// so the Client will be informed of what has happened.
				// A transaction is considered to start when the Slave is addressed by the Master
				// (as indicated by the PSL invoking NotifyClient) - which has not yet happened -
				// so the next state is EInactive.
				iTimerState=DIicBusChannelSlave::EInactive;
				break;
				}
			case DIicBusChannelSlave::EWaitForMaster:
				{
				// In this case we are handling a new requested trigger from the client to handle ERxOverrun or
				// ETxUnderrun. The PSL has accepted the new trigger, so must allow the Master sufficient time
				// to perform the newly-requested transfer; the timer has already been stopped, so just start it again..
				StartTimerByState();
				break;
				}
			default:
				{
				__ASSERT_DEBUG(0, Kern::Fault(KIicChannelPanic,__LINE__));
				break;
				}
			}
		}

	return retVal;
	}
#else /*SLAVE_MODE*/

TInt DIicBusChannelSlave::CaptureChannel(TDes8* /*aConfigHdr*/, TIicBusSlaveCallback* /*aCallback*/, TInt& /*aChannelId*/, TBool /*aAsynch*/)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::CaptureChannel invoked when not in SLAVE_MODE!\n"));
	 return KErrNotSupported;
	}

TInt DIicBusChannelSlave::ReleaseChannel()
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::ReleaseChannel invoked when not in SLAVE_MODE!\n"));
	 return KErrNotSupported;
	}

TInt DIicBusChannelSlave::RegisterRxBuffer(TPtr8 /*aRxBuffer*/, TInt8 /*aBufGranularity*/, TInt8 /*aNumWords*/, TInt8 /*aOffset*/)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::RegisterRxBuffer invoked when not in SLAVE_MODE!\n"));
	 return KErrNotSupported;
	}

TInt DIicBusChannelSlave::RegisterTxBuffer(TPtr8 /*aTxBuffer*/, TInt8 /*aBufGranularity*/, TInt8 /*aNumWords*/, TInt8 /*aOffset*/)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::RegisterTxBuffer invoked when not in SLAVE_MODE!\n"));
	 return KErrNotSupported;
	}

TInt DIicBusChannelSlave::SetNotificationTrigger(TInt /*aTrigger*/)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::SetNotificationTrigger invoked when not in SLAVE_MODE!\n"));
	 return KErrNotSupported;
	}
#endif/*SLAVE_MODE*/

DIicBusChannelSlave::DIicBusChannelSlave(TBusType aBusType, TChannelDuplex aChanDuplex, TInt16 aChannelId)
	: DIicBusChannel(DIicBusChannel::ESlave, aBusType, aChanDuplex),
	iChannelId(aChannelId), iTimerState(EInactive),
	iMasterWaitTime(KSlaveDefMWaitTime), iClientWaitTime(KSlaveDefCWaitTime),
	iSpinLock(TSpinLock::EOrderGenericIrqLow2)  // Semi-arbitrary, low priority value
	{
#ifndef STANDALONE_CHANNEL
	iController = NULL;
#endif
	}

DIicBusChannelSlave::~DIicBusChannelSlave()
    {
    delete iClientTimeoutDfc;
    }

void DIicBusChannelSlave::SlaveStaticCB(TAny* aPtr)
	{
	DIicBusChannelSlave* chan = (DIicBusChannelSlave*)aPtr;
	chan->SlaveTimerCallBack();
	return;
	}

TInt DIicBusChannelSlave::Init()
	{
	iClientTimeoutDfc = new TDfc(SlaveStaticCB,(TAny*)this, 7);	// Highest Dfc priority
	if(!iClientTimeoutDfc)
		return KErrNoMemory;
	else
		return KErrNone;
	}

void DIicBusChannelSlave::ChanCaptureCallback(TInt aResult)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("ChanCaptureCallback: aChannel=0x%x, aResult=%d\n",this,aResult));

	TInt r=aResult;
	TInt channelId = 0;
	if(aResult == KErrNone)
		{
		SetChannelId(channelId);
#ifndef STANDALONE_CHANNEL
		__ASSERT_DEBUG(iController, Kern::Fault(KIicChannelPanic,__LINE__));
		iController->InstallCapturedChannel(channelId, this);
#endif
		iClientTimeoutDfc->SetDfcQ(iNotif->iDfcQ);
		r=KErrCompletion;
		}
	else
		ReleaseChannel();

#ifdef IIC_INSTRUMENTATION_MACRO
	IIC_SCAPTCHANASYNC_END_PIL_TRACE;
#endif
	CompleteAsynchCapture(r);	// Queue the client callback for execution
	}

void DIicBusChannelSlave::SlaveTimerCallBack()
	{
    __KTRACE_OPT(KIIC, Kern::Printf("SlaveTimerCallBack"));
	if(iTimerState == DIicBusChannelSlave::EWaitForMaster)
		{
		// Master timeout. Consider the transaction terminated - call NotifyClient
		// to inform both the Client and the PSL, and update the state machine
		NotifyClient(EGeneralBusError);
		}
	else if(iTimerState == DIicBusChannelSlave::EWaitForClient)
		{
		// Client timeout. Instigate the PSL-specific bus error indication
		iTimerState=DIicBusChannelSlave::EClientTimeout;
		SendBusErrorAndReturn();
		}
	else
		{
		__ASSERT_DEBUG(0, Kern::Fault(KIicChannelPanic,__LINE__));
		}
	}


void DIicBusChannelSlave::StartTimerByState()
	{
	if(iTimerState == DIicBusChannelSlave::EWaitForMaster)
		{
		iTimeoutTimer.OneShot(NKern::TimerTicks(iMasterWaitTime),(*iClientTimeoutDfc));
		}
	else if(iTimerState == DIicBusChannelSlave::EWaitForClient)
		{
		iTimeoutTimer.OneShot(NKern::TimerTicks(iClientWaitTime),(*iClientTimeoutDfc));
		}
	else
		{
		__ASSERT_DEBUG(NULL, Kern::Fault(KIicChannelPanic,__LINE__));
		}
	}

void DIicBusChannelSlave::StopTimer()
	{
	// Silently cancel the timer and associated DFC
	//
	// NTimer::Cancel returns ETrue if cancelled, EFalse otherwise - which may mean it wasn't active
	// TDfc::Cancel returns ETrue if actually de-queued, EFalse otherwise - which may mean it wasn't queued
	//
	iTimeoutTimer.Cancel();
	iClientTimeoutDfc->Cancel();
	}

TInt DIicBusChannelSlave::UpdateReqTrig(TInt8& aCbTrigVal, TInt& aCallbackRet)
	{
    __KTRACE_OPT(KIIC, Kern::Printf("UpdateReqTrig"));

	TInt nextSteps = 0;
	iAccumTrig |= iNotif->iTrigger;	// Update the accumulated event history, regardless of if the trigger was requested

	if(iNotif->iTrigger & EGeneralBusError)
		{
		// In the event of a bus error, always cancel the timer and call the Client callback
		StopTimer();
		iTimerState = EInactive;
		nextSteps = EInvokeCb;
		aCallbackRet = KErrGeneral;
		}
	else if(iNotif->iTrigger == EAsyncCaptChan)
		{
		// For asynchronous channel capture, no timers are involved - just call the Client callback
		nextSteps = EInvokeCb;
		aCallbackRet = KErrCompletion;
		}
	else if((iNotif->iTrigger & iReqTrig) != 0)
		{
		// If a requested Rx event has occurred, clear all Rx flags from the requested triggers (similarly for Tx)
		if(iNotif->iTrigger & (ERxAllBytes | ERxUnderrun | ERxOverrun))
			{
			iReqTrig &= ~(ERxAllBytes | ERxUnderrun | ERxOverrun);
			}
		if(iNotif->iTrigger & (ETxAllBytes | ETxUnderrun | ETxOverrun))
			{
			iReqTrig &= ~(ETxAllBytes | ETxUnderrun | ETxOverrun);
			}

		if(iTimerState == EInactive)
			{
			nextSteps |= (EStartTimer | EInvokeCb);
			// The next state in the state machine depends on if all the requested events have occurred
			if(iReqTrig == 0)
				{
				// All triggers required have occurred, so transition to state EWaitForClient
				iTimerState = EWaitForClient;
				}
			else
				{
				// The Client can request both Rx an Tx triggers; if only one has occurred, must wait for
				// the Master to generate the other
				iTimerState = EWaitForMaster;
				}
			aCallbackRet = KErrNone;
			}
		else if(iTimerState == EWaitForMaster)
			{
			// The next state in the state machine depends on if all the requested events have occurred
			if(iReqTrig == 0)
				{
				// All triggers required have occurred, so transition to state EWaitForClient
				iTimerState = EWaitForClient;
				StopTimer();
				nextSteps = (EInvokeCb | EStartTimer);
				}
			else
				{
				// The Client can request both Rx an Tx triggers; if only one has occurred, must wait for
				// the Master to generate the other - so remain in this state, do not cancel the timer or
				// re-start it with a new timeout period. Still invoke the callback to notify the client
				// that at least one of the requested triggers has occurred.
				nextSteps |= EInvokeCb;
				}
			aCallbackRet = KErrNone;
			}
		else if((iTimerState == EWaitForClient) || (iTimerState == EClientTimeout))
			{
			// No triggers are expected in these states (iReqTrig==0).
			__ASSERT_DEBUG(NULL, Kern::Fault(KIicChannelPanic,__LINE__));
			}
		}
	aCbTrigVal = iAccumTrig;
	return nextSteps;
	}


void DIicBusChannelSlave::NotifyClient(TInt aTrigger)
	{
	TIicBusSlaveCallback* notif = iNotif;
	notif->iTrigger = aTrigger;	// Ensure ProcessData is provided with the trigger

	if(NKern::CurrentContext() == NKern::EThread && &(Kern::CurrentThread()) == iClient)
		{
		// PSL will update notif to represent the events that have occurred
		ProcessData(aTrigger, notif);
		// Only invoke the client's callback (and update the state machine) if one of the requested triggers has
		// occurred or if a bus error has been witnessed
		TInt8 callbackTrig=0;
		TInt callbackRet=0;
		TInt nextSteps = UpdateReqTrig(callbackTrig, callbackRet);
		if(nextSteps & EStopTimer)
			{
			__ASSERT_DEBUG(NULL, Kern::Fault(KIicChannelPanic,__LINE__));
			}
		if(nextSteps & EInvokeCb)
			{
			(notif->iCallback)(notif->iChannelId, (TInt)callbackRet, callbackTrig, notif->iRxWords, notif->iTxWords, notif->iParam);
			// Callback now processed, so re-initialise callback object members
			notif->iTrigger=0;
			notif->iReturn=KErrNone;
			notif->iRxWords=0;
			notif->iTxWords=0;
			iAccumTrig = 0;	// and re-initialise the accumulated history as the transaction is considered terminated
			}
		if(nextSteps & EStartTimer)
			{
			StartTimerByState();
			}
		}
	else if(NKern::CurrentContext() == NKern::EInterrupt)
			notif->Add();
	else
		notif->Enque();
	}

TInt DIicBusChannelSlave::SetMasterWaitTime(TInt8 aWaitTime)
	{
	if((aWaitTime<0)||(aWaitTime>KMaxWaitTime))
		return KErrArgument;
	iMasterWaitTime=aWaitTime;
	return KErrNone;
	}

TInt DIicBusChannelSlave::SetClientWaitTime(TInt8 aWaitTime)
	{
	if((aWaitTime<0)||(aWaitTime>KMaxWaitTime))
		return KErrArgument;
	iClientWaitTime=aWaitTime;
	return KErrNone;
	}

void DIicBusChannelSlave::SendBusErrorAndReturn()
	{
	DoRequest(EAbort);
	}

void DIicBusChannelSlave::SetChannelId(TInt& aChannelId)
    {
    ++iInstanceCount;
    aChannelId = (iInstanceCount<<16);
    //
    // The PSL-specific channel identifier was stored in this generic class' member iChannelId at registration time
    aChannelId |= iChannelId;
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::SetChannelId: iInstanceCount=0x%x, iChannelId=0x%x returned aChannelId=0x%x\n",iInstanceCount,iChannelId,aChannelId));
    iNotif->iChannelId=aChannelId;
    }

void DIicBusChannelSlave::CompleteAsynchCapture(TInt aResult)
    {
    __KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlave::CompleteAsynchCapture aResult = %d",aResult));
    if(NKern::CurrentContext() == NKern::EThread && &Kern::CurrentThread() == iClient)
        {
        iNotif->iCallback(iNotif->iChannelId, aResult, EAsyncCaptChan, NULL, NULL, iNotif->iParam);
        return;
        }
    else
        {
        iNotif->iReturn=aResult;
        iNotif->iTrigger=EAsyncCaptChan;
        iNotif->iTxWords=NULL;
        iNotif->iRxWords=NULL;
        }
    if(NKern::CurrentContext() == NKern::EInterrupt)
        iNotif->Add();
    else
        iNotif->Enque();
    }

TInt DIicBusChannelSlave::StaticExtension(TUint /*aFunction*/, TAny* /*aParam1*/, TAny* /*aParam*/)
 	{
 	return KErrNotSupported;
 	}

TInt DIicBusChannelSlave::Spare1(TInt /*aVal*/, TAny* /*aPtr1*/, TAny* /*aPtr2*/)
	{
	return KErrNotSupported;
	}

TInt DIicBusChannelMasterSlave::QueueTransaction(TIicBusTransaction* aTransaction)
	{
	return QueueTransaction(aTransaction,NULL);
	};

TInt DIicBusChannelMasterSlave::QueueTransaction(TIicBusTransaction* aTransaction, TIicBusCallback* aCallback)
	{
	TInt r=KErrNone;
	iMasterChannel->Lock();
	if(iSlaveChannel->iChannelInUse)
		r=KErrInUse;
	else
		{
		TInt16 count=(TInt16)((iMasterChannel->iTransCount)&~KTransCountMsBit);
		if(count<~KTransCountMsBit)
			{
			++count;
			count|=KTransCountMsBit;
			}
		else
			r=KErrInUse;
		}
	iMasterChannel->Unlock();
	if(r == KErrNone)
		r=(iMasterChannel->QueueTransaction(aTransaction, aCallback));
	return r;
	};

TInt DIicBusChannelMasterSlave::CaptureChannel(TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch)
	{
	iMasterChannel->Lock();
	TInt r=KErrNone;
	if(iSlaveChannel->iChannelInUse)
		r=KErrInUse;
	else
		{
		if(iMasterChannel->IsMasterBusy())
			r=KErrInUse;
		else
			iSlaveChannel->iChannelInUse = 1;
		}
	iMasterChannel->Unlock();
	if(r == KErrNone)
		r=iSlaveChannel->CaptureChannel(aConfigHdr, aCallback, aChannelId, aAsynch);
	return r;
	};


TInt DIicBusChannelMasterSlave::ReleaseChannel()
    {
	iMasterChannel->Lock();
	TInt r=iSlaveChannel->ReleaseChannel();
	iMasterChannel->Unlock();
	return r;
	};

TInt DIicBusChannelMasterSlave::StaticExtension(TUint /*aFunction*/, TAny* /*aParam1*/, TAny* /*aParam*/)
 	{
 	return KErrNotSupported;
 	}

#ifdef STANDALONE_CHANNEL
EXPORT_C DIicBusChannelMasterSlave::DIicBusChannelMasterSlave(TBusType aBusType, TChannelDuplex aChanDuplex, DIicBusChannelMaster* aMasterChan, DIicBusChannelSlave* aSlaveChan)
    : DIicBusChannel(DIicBusChannel::EMasterSlave, aBusType, aChanDuplex),
    iMasterChannel(aMasterChan),
    iSlaveChannel(aSlaveChan)
    {
    //If in stand-alone channel mode, the client assigns a channel number to the MasterSlave channel it creates.
    }
#endif


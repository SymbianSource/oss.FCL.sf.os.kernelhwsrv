/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


#include <drivers/iic.h>
#include <drivers/iic_channel.h>
// #include <gpio.h>	// Include if using GPIO functionality
#include "iic_psl.h"
#include "iic_slave.h"

// The timeout period to wait for a response from the client, expressed in milliseconds
// This is converted to timer ticks by the PIL, so the maximum value is 2147483.
// The value should be selected to allow for the longest, slowest transfer
// const TInt KClientWaitTime = 2; // 2mS, when debugging might set up to KMaxWaitTime


// In an SMP system, use a spin lock to guard access to member variables iTrigger and iInProgress
#ifdef __SMP__
static TSpinLock IicPslSpinLock = TSpinLock(TSpinLock::EOrderGenericIrqLow3);
#endif

// Callback function for the iHwGuardTimer timer. 
//
// Called in ISR context if the iHwGuardTimer expires. Sets iTransactionStatus to KErrTimedOut
//
void DIicBusChannelSlavePsl::TimeoutCallback(TAny* aPtr)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DCsiChannelMaster::TimeoutCallback"));
	DIicBusChannelSlavePsl *a = (DIicBusChannelSlavePsl*) aPtr;
	a->iTransactionStatus = KErrTimedOut;
	}


// Static method called by the ISR when the Master has ended a transfer
//
// The method checks and reports the Rx and Tx status to the PIL by calling NotifyClient with a bitmask described as follows:.
// - If a Tx transfer has ended before all the data was transmitted, bitmask = (ETxAllBytes | ETxOverrun)
// - If a Tx transfer has ended and all the data was transmitted, bitmask = ETxAllBytes
// - If a Rx transfer has ended before the expected amount of data was received, bitmask = (ERxAllBytes | ERxUnderrun)
// - If a Rx transfer has ended and the expected amount of data was received, bitmask = ERxAllBytes
//
void DIicBusChannelSlavePsl::NotifyClientEnd(DIicBusChannelSlavePsl* aPtr)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("NotifyClientEnd, iTrigger %x", aPtr->iTrigger));

	// Since a transfer has ended, may wish to disable interrupts at this point
	// This will likely be supported with calls similar to the following:
	//		AsspRegister::Write32(aPtr->iChannelBase + KBusInterruptEnableOffset, KIicPslBusDisableBitMask);
	//		Interrupt::Disable(aPtr->iRxInterruptId);
	//		Interrupt::Disable(aPtr->iTxInterruptId);

	// iTrigger will have bits ETransmit and EReceive set according to the operation requested in the call to DoRequest
	// Use variable flag for the bitmask to pass into the PIL method NotifyClient
	TInt flag = 0;
	if(aPtr->iTrigger & EReceive)
		{
		// Requested Rx operation has ended - check for RxUnderrun
		flag = ERxAllBytes;
		if(aPtr->iRxDataEnd != aPtr->iRxData)
			{
			flag |= ERxUnderrun;
			}
		}
	if(aPtr->iTrigger & ETransmit)
		{
		// Requested Tx operation has ended - check for RxOverrun
		flag |= ETxAllBytes;
		if(aPtr->iTxDataEnd != aPtr->iTxData)
			{
			flag |= ETxOverrun;
			}
		}
	aPtr->NotifyClient(flag);
	}


// ISR Handler
//
// The ISR handler identifies the cause of the interrupt that lead to its invocation:
// if the cause was transfer-related, it calls the PIL function NotifyClient to report a summary of the transfer status;
// if the cause was completion of asynchronous channel capture, PIL function ChanCaptureCallback is called 
//
// The ISR also clears the source of the interrupt, and (for transfer-related interrupts) transfers the next data 
// between buffers and the hardware and updates the member variable iInProgress to indicate if a transfer has started or
// ended. If a transfer has ended before the expected amount of data has been transfered it calls function NotifyClientEnd.
//
void DIicBusChannelSlavePsl::IicPslIsr(TAny* /*aPtr*/)
	{
	//		DIicBusChannelSlavePsl *a = (DIicBusChannelSlavePsl*) aPtr;

	//		TInt intState = 0;	// Variable to support use of spin lock

	//		TInt trigger = 0; // Record the Rx and Tx transfers

	//		TUint32 intStatus = 0; // Record of the interrupts that are being reported

	// Identify the cause of the interrupt. If this can be achieved by reading a single register,
	// code similar to the following could be used:
	//		intStatus = AsspRegister::Read32(a->iChannelBase + KIntStatusOffset);

	// Optional (not required if asynchronous channel capture is not supported)
	// If the cause of the interrupt is completion of asynchronous channel capture, the ISR will check the appropriate
	// indicator for confirmation of success - for a real PSL, this may be by querying a bitmask in a register. For the template PSL,
	// however, a dummy member variable (iAsyncConfig) has been used to represent the asynchronous operation instead.
	//
	//		if(iAsyncConfig == 1)	// Replace with a check of the indicator that the interrupt was due to asynchrous channel capture
	//			{
	//			// The PIL function ChanCaptureCallback is now to be invoked. It takes as an argument either KErrNone or a
	//			// system-wide error code to indicate the cause of failure. For a real PSL, the argument would likely be determined
	//			// by reading a bitmask in a status register - but for the template port, just use KErrNone.
	//			//
	//			a->ChanCaptureCallback(KErrNone);
	//			return;
	//			}

	// If an interrupt indicates that a transfer has started, or that it has now ended, (such as a chip select 
	// line transition for a SPI bus the member variable iInProgress should be modified accordingly. This should
	// be done under the guard of spin lock macros since iInProgress can be accessed in the context of the Client
	// thread (in DoRequest, ProcessData and InitTransfer). The following structure should be adopted:
	//		intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
	//		<access a->iInProgress>
	//		__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);
	//
	// If a transfer has ended before the expected amount of data has been transfered, function NotifyClientEnd
	// should be called, as follows:
	//		a->NotifyClientEnd(a);
	//		return;	// Return now - the interrupt indicated transfer end, not receipt or transmission of data.

	// The transfers that had been started are indicated by the bitmask held in member variable iTrigger.
	// This must be accessed under the guard of a spin lock since it can be accessed in the context of the
	// Client thread (in DoRequest, ProcessData and InitTransfer). The following structure should be adopted:
	//		intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
	//		trigger = a->iTrigger;
	//		__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);

	// If the interrupt was raised for a Tx event, and a Tx transfer had been started (so the interrupt was not spurious)
	// then either prepare the next data to send, or, if all the data has been sent, call the PIL function NotifyClient
	// with bitmask (ETxAllBytes | ETxUnderrun) so that, if the Client specified a ETxUnderrun notification, it will be alerted
	// and can determine whether another buffer of data should be provide for transmission.
	// Code similar to the following could be used:
	//		if(intStatus & KTxInterruptBitMask)
	//			{
	//			if(trigger & ETransmit)
	//				{
	//				// Interrupt was not spurious
	//				if(a->iTxData == a->iTxDataEnd)
	//					{
	//					// All the data to be transmitted has been sent, so call the PIL method NotifyClient
	//					a->NotifyClient(ETxAllBytes | ETxUnderrun);
	//					}
	//				else
	//					{
	//					// There is more data to be sent
	//					// TUint8 nextTxValue = *iTxData;	// For this example, assumes one byte of data is to be transmitted
	//														// but if operating in 16-bit mode, bytes may need arranging for
	//														// endianness
	//
	//					// Write to the Tx register with something similar to the following:
	//					//		AsspRegister::Write32(iChannelBase + KTxFifoOffset, nextTxValue);
	//
	//					iTxData += iWordSize;	// Then increment the pointer to the data. In this example, 8-bit mode is assumed
	//											// (iWordSize=1), but if operating in 16-bit mode iTxData would be incremented
	//											// by the number of bytes specified in iWordSize
	//					}
	//				}
	//			}

	// If the interrupt was raised for a Rx event, and a Rx transfer had been started (so the interrupt was not spurious)
	// read the received data from the hardware to the buffer. If a Rx FIFO is being used, use a loop to drain it - until
	// the FIFO is empty or the buffer is full. If data remains after the buffer is full, an RxOverrun condition has occurred
	// - so the PIL function NotifyClient should be called with bitmask (ERxAllBytes | ERxOverrun) so that, if the Client specified
	// a ERxOverrun notification, it will be alerted and can determine whether another buffer should be provided to continue reception.
	// Code similar to the following could be used:
	//		if(intStatus & KRxInterruptBitMask)
	//			{
	//			if(trigger & EReceive)
	//				{
	//				// Interrupt was not spurious
	//				while(AsspRegister::Read32(a->iChannelBase + KRxFifoLevelOffset))
	//					{
	//					if((a->iRxData - a->iRxDataEnd) >= a->iWordSize)
	//						{
	//						// Space remains in the buffer, so copy the received data to it
	//						TUint8 nextRxValue = AsspRegister::Read32(a->iChannelBase + KRxFifoOffset);
	//						*a->iRxData = nextRxValue;	// For this example, assumes one byte of data is to be transmitted
	//													// but if operating in 16-bit mode, bytes may need arranging for
	//													// endianness
	//
	//						a->iRxData += a->iWordSize;	// Then increment the pointer to the data. In this example, 8-bit mode is assumed
	//													// (iWordSize=1), but if operating in 16-bit mode iRxData would be incremented
	//													// by the number of bytes specified in iWordSize
	//						}
	//					else
	//						{
	//						// The buffer is full but more data has been received - so there is an RxOverrun condition
	//						// Disable the hardware from receiving any more data and call the PIL function NotifyClient
	//						// with bitmask (ERxAllBytes | ERxOverrun).
	//						AsspRegister::Write32(a->iChannelBase + KRxFifoControl, KRxFifoDisableBitMask);
	//						a->NotifyClient(ERxAllBytes | ERxOverrun);
	//						break;
	//						}
	//					}
	//				}
	//			else
	//				{
	//				// If the interrupt was spurious, ignore the data, and reset the FIFO
	//				AsspRegister::Write32(a->iChannelBase + KRxFifoControl, KRxFifoClearBitMask);
	//				}

	// Once the interrupts have been processed, clear the source. If this can be achieve by writing to
	// a single register, code similar to the following could be used:
	//		AsspRegister::Write32(a->iChannelBase + KIntStatusOffset, KAIntBitMask);

	}


// Constructor, first stage
//
// The PSL is responsible for setting the channel number - this is passed as the first parameter to
// this overload of the base class constructor
//
DIicBusChannelSlavePsl::DIicBusChannelSlavePsl(TInt aChannelNumber, TBusType aBusType, TChannelDuplex aChanDuplex) :
	DIicBusChannelSlave(aBusType, aChanDuplex, 0),	// Base class constructor. Initalise channel ID to zero.
	iHwGuardTimer(TimeoutCallback, this)			// Timer to guard against hardware timeout
	{
	iChannelNumber = aChannelNumber;
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlavePsl::DIicBusChannelSlavePsl, iChannelNumber = %d\n", iChannelNumber));
	}


// Second stage construction
//
// Allocate and initialise objects required by the PSL channel implementation
//
TInt DIicBusChannelSlavePsl::DoCreate()
	{
	__KTRACE_OPT(KIIC, Kern::Printf("\nDIicBusChannelSlavePsl::DoCreate, ch: %d \n", iChannelNumber));

	TInt r = KErrNone;

	// PIL Base class initialization.
	r = Init();
	if(r == KErrNone)
		{
		// At a minimum, this function must set the channel's unique channel ID.
		// When the channel is captured, this value will be combined with an instance count
		// provided by the PIL to generate a value that will be used by a client as a unique
		// identifer in subsequent calls to the Slave API.
		//
		// There is no set format for the ID, it just needs to be unique.
		// Un-comment and complete the following line:
//		iChannelId = 
		
		// This method may also be concerned with setting the base register address iChannelBase), and allocating
		// any objects that will be required to support operaton until the channel is deleted.
		//
		// Un-comment and complete the following line:
//		iChannelBase = 
		}
	return r;
	}

// static method used to construct the DIicBusChannelSlavePsl object.
DIicBusChannelSlavePsl* DIicBusChannelSlavePsl::New(TInt aChannelNumber, const TBusType aBusType, const TChannelDuplex aChanDuplex)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlavePsl::NewL(): aChannelNumber = %d, BusType =%d", aChannelNumber, aBusType));
	DIicBusChannelSlavePsl *pChan = new DIicBusChannelSlavePsl(aChannelNumber, aBusType, aChanDuplex);

	TInt r = KErrNoMemory;
	if (pChan)
		{
		r = pChan->DoCreate();
		}
	if (r != KErrNone)
		{
		delete pChan;
		pChan = NULL;
		}

	return pChan;
	}


// Validates the configuration information specified by the client when capturing the channel
//
// Called by the PIL as part of the Slave CaptureChannel processing
//
// If the pointer to the header is NULL, return KErrArgument.
// If the content of the header is not valid for this channel, return KErrNotSupported.
// 
TInt DIicBusChannelSlavePsl::CheckHdr(TDes8* aHdrBuff)
	{
	TInt r = KErrNone;

	if(!aHdrBuff)
		{
		r = KErrArgument;
		}
	else
		{
		// Check that the contents of the header are valid
		//
		// The header will be specific to a particular bus type. Using a fictional
		// bus type Abc,code similar to the following could be used to validate each
		// member of the header:
		// 
		//		TConfigAbcBufV01* headerBuf = (TConfigAbcBufV01*) aHdrBuff;
		//		TConfigAbcV01 &abcHeader = (*headerBuf)();
		//		if(	(abcHeader.iHeaderMember < ESomeMinValue)	||
		//			(abcHeader.iHeaderMember > ESomeMaxValue))
		//			{
		//			__KTRACE_OPT(KIIC, Kern::Printf("iHeaderMember %d not supported",abcHeader.iHeaderMember));
		//			r = KErrNotSupported;
		//			}

		}
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlavePsl::CheckHdr() r %d", r));

	return r;
	}


// Method called in the context of the client thread, as a consequence of the PSL invocation of the 
// PIL method NotifyClient when a bus event occurs.
//
// This method updates the bitmask of requested operations (held in member variable iTrigger) and the
// PIL counts of data received and transmitted. If the event was a bus error, the bitmask of requested operations
// is cleared.
//
void DIicBusChannelSlavePsl::ProcessData(TInt aTrigger, TIicBusSlaveCallback* aCb)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlavePsl::ProcessData(), trigger: %x\n", aTrigger));

	TInt intState;

	// If using the iInProgress member variable to indicate transitions on a chip-select line, and an interrupt
	// occurred as a transfer was to end, then must ensure the transmission of data has ceased.
	//
	// Must use spin lock to guard access since iInProgress is accessed by the ISR
	//
	TInt inProgress;
	intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
	inProgress = iInProgress;
	__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);
	//
	if(!inProgress &&								// Transfer has now ended
	   (aTrigger & (ERxAllBytes | ETxAllBytes)))	// Master has not yet finished transferring data
		{
		// Use the guard timer to make sure that transfer ends with an expected time - if this does not cease 
		// before the timer expires, iTransactionStatus will be set to KErrTimedOut by the callback function TimeoutCallback
		//
		// Poll the relevant register to check for transfer activity, using code similar to the following:
		//		TInt8 transferring = AsspRegister::Read32(iChannelBase + KStatusRegisterOffset) & KTransferringBitMask);
		// For the template port, use a dummy variable instead of the register access (transferring = 1)
		//
		TInt8 transferring = 1;
		iTransactionStatus = KErrNone;
		iHwGuardTimer.OneShot(NKern::TimerTicks(KTimeoutValue));

		while((iTransactionStatus == KErrNone) &&
		      transferring);		// Replace transferring with a register read, as described above

		// At this point, either the transfer has ceased, or the timer expired - in either case, may disable the interrupt
		// for the transfer now, using code similar to the following:
		//		AsspRegister::Write32(iChannelBase + KIntEnableRegisterOffset, KIntDisableBitMask);

		// Check for guard timer expiry
		if(iTransactionStatus != KErrNone)
			{
			__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlavePsl::ProcessData - Transaction timed-out"));
			return;
			}
		else
			{
			iHwGuardTimer.Cancel();
			}

		// If all transfer activity has now ceased, clear iTrigger
		// Must use spin lock to guard access since iInProgress is accessed by the ISR
		//
		intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
		iTrigger = 0;
		__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);
		}

	// If the PSL called the PIL function NotifyClient to indicate transfer activity (or error), the reason
	// will be specified as a bitmask in aTrigger
	//  - if a Rx event occurred, the ERxAllBytes flag will be set
	//  - if a Tx event occurred, the ETxAllBytes flag will be set
	//  - if a bus error occurred, the EGeneralBusError flag will be set
	//
	if(aTrigger & ERxAllBytes)
		{
		__KTRACE_OPT(KIIC, Kern::Printf("ProcessData - Rx Buf:    %x\n", iRxData));
		__KTRACE_OPT(KIIC, Kern::Printf("ProcessData - Rx Bufend: %x\n", iRxDataEnd));

		// Clear the internal EReceive flag
		// This must be done under guard of a spin lock since iTrigger is accessed by the ISR
		intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
		iTrigger &= ~EReceive;
		__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);

		// Update the PIL count of Rx data (in the Callback object)
		aCb->SetRxWords(iNumRxWords - ((iRxDataEnd - iRxData) / iWordSize));
		}
	// 
	if(aTrigger & ETxAllBytes)
		{
		__KTRACE_OPT(KIIC, Kern::Printf("ProcessData - Tx Buf:    %x\n", iTxData));
		__KTRACE_OPT(KIIC, Kern::Printf("ProcessData - Tx Bufend: %x\n", iTxDataEnd));

		// Clear the internal ETransmit flag..
		// This must be done under guard of a spin lock since iTrigger is accessed by the ISR
		intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
		iTrigger &= ~ETransmit;
		__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);

		// Update the PIL count of Tx data (in the Callback object)
		aCb->SetTxWords(iNumTxWords - ((iTxDataEnd - iTxData) / iWordSize));
		}
	//
	if(aTrigger & EGeneralBusError)
		{
		__KTRACE_OPT(KIIC, Kern::Printf("BusError.."));

		// Clear and disable relevant interrupts, possibly using code similar to the following:
		//		AsspRegister::Write32(iChannelBase + KIntEnableRegisterOffset, KIntDisableBitMask);

		// Clear internal flags
		// This must be done under guard of a spin lock since iTrigger is accessed by the ISR
		intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
		iTrigger = 0;
		__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);
		}

	// Set the callback's trigger, for use by the PIL
	aCb->SetTrigger(aTrigger | aCb->GetTrigger());
	}



// Method to initialise the hardware in accordance with the data provided by the Client
// in the configuration header when capturing the channel
//
// This method is called from DoRequest and is expected to return a value to indicate success
// or a system wide error code to inform of the failure
//
TInt DIicBusChannelSlavePsl::ConfigureInterface()
	{
	__KTRACE_OPT(KIIC, Kern::Printf("ConfigureInterface()"));

	TInt r = KErrNone;

	// The header is stored in member variable iConfigHeader, and will be specific to a particular bus type.
	// Using a fictional bus type Abc, code similar to the following could be used to access each
	// member of the header:
	// 
	//		TConfigAbcBufV01* headerBuf = (TConfigAbcBufV01*) iConfigHeader;
	//		TConfigAbcV01 &abcHeader = (*headerBuf)();
	//		TInt value = abcHeader.iTintMember;

	// Initialising the hardware may be achieved with calls similar to the following:
	//		AsspRegister::Write32(a->iChannelBase + KBusModeControlOffset, KIicPslModeControlBitMask);
	//		GPIO::SetPinMode(aPinId, GPIO::EEnabled);

	// Binding an ISR may be achieved with calls similar to the following:
	//		r = Interrupt::Bind(iRxInterruptId, DIicBusChannelSlavePsl::IicPslIsr, this);
	//		r = Interrupt::Bind(iTxInterruptId, DIicBusChannelSlavePsl::IicPslIsr, this);
	// Enabling interrupts may be achieved with calls similar to the following:
	//		r = Interrupt::Enable(iRxInterruptId);
	//		r = Interrupt::Enable(iTxInterruptId);

	// Modifying a hardware register may not be a zero-delay operation. The member variable iHwGuardTimer could be used to guard a
	// continuous poll of the hardware register that checks for the required change in the setting; TimeoutCallback is already 
	// assigned as the callback function for iHwGaurdTimer, and it modifies member variable iTransactionStatus to indicate a timeout
	// - so the two could be used together as follows:
	//		iTransactionStatus = KErrNone;
	//		iHwGuardTimer.OneShot(NKern::TimerTicks(KTimeoutValue));
	//		while((iTransactionStatus == KErrNone) &&
	//		       AsspRegister::Read32(iChannelBase + KRegisterOffset) & KRegisterFlagBitMask);
	//		if(iTransactionStatus != KErrNone)
	//			{
	//			r = KErrGeneral;
	//			}
	//		else
	//			{
	//			iHwGuardTimer.Cancel();
	//			}

	// DoRequest checks the return value so the variable r should be modified in the event of failure with a system-wide error code
	// for example, if a register could not be modified,
	//			r = KErrGeneral;
	//			__KTRACE_OPT(KIIC, Kern::Printf("ConfigureInterface failed with error %d\n",r));
	return r;
	}


// Method to start asynchronous initialisation of the hardware, in accordance with the data provided by the Client
// in the configuration header when capturing the channel. This differs from ConfigureInterface in that it
// merely starts the initialisation, then returns immediately; 
//
// The PSL is expected to be implemented as an asynchronous state machine, where events (for example hardware 
// interrupts, or timer expiry) invoke callback functions that advance the state machine to the next state. Once
// all the required states have been transitioned, so that the PSL part of the CaptureChannel processing is 
// complete, the ISR should be invoked, which will then call PIL method ChanCaptureCallback
//
// This method is called from DoRequest and is expected to return a value to indicate success
// or a system wide error code to inform of the failure
//
TInt DIicBusChannelSlavePsl::AsynchConfigureInterface()
	{
	__KTRACE_OPT(KIIC, Kern::Printf("ConfigureInterface()"));

//	TInt r = KErrNone;	// A real implementation would use this as the return value to indicate success / failure

	// Precisely what processing is done to 'start' the asynchronous processing is entirely platform-specific;
	// it may be the set-up and activation of a long-running operation that completes asynchronously. Regardless of what
	// is done, its completion is expected to result in the ISR being run.
	//
	// Whatever the operation, there must be some means of the ISR recognising that an asynchronous initialisation has
	// been performed
	// In a real PSL, this may be be checking a bitmask in a status register. For the template PSL, however,
	// a dummy class member will be used (iAsyncConfig)
	// Since this member will be accessed by the ISR, it should, strictly speaking, be accessed under the guard of a spin lock
	TInt intState;
	intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
	iAsyncConfig = 1;
	__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);

	return KErrNone;	// A real implementation would return an indication of success / failure
	}

// Method called from DoRequest to start Tx and-or Rx transfer.
//
// The method will initialise the hardware and pointers used to manage transfers, before returning a value to report success
// (KErrNone) or a system-wide error code that indicates the cause of failure.
//
TInt DIicBusChannelSlavePsl::InitTransfer()
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelSlavePsl::InitTransfer()"));

	TInt r = KErrNone;

	// Local copies of member variables that must be accessed in a synchronised manner
	TInt inProgress;
	TInt trigger;

	TInt intState;

	// Check if a transfer is already in progress. 
	// If variable iInProgress is being used, this must be determined in a synchronised manner because the ISR modifies it.
	// Bus types that do not rely on chip-select transitions may use an alternative method to indicate if a transfer is in
	// progress
	intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
	inProgress = iInProgress;
	__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);

	if(!inProgress)
		{
		// If no transfers are in progress, it may be necessary to initialise the hardware to support those that
		// are being requested. This may include FIFO and interrupt initialisation, 
		//
		// Initialising the hardware may be achieved with calls similar to the following:
		//		AsspRegister::Write32(iChannelBase + KBusModeControlOffset, KIicPslModeControlBitMask);
		//		GPIO::SetPinMode(aPinId, GPIO::EEnabled);
		}

	// Check the current operations. This must be determined in a synchronised manner because ProcessData
	// runs in the context of the Client thread and it modifies the value of iTrigger
	intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
	trigger = iTrigger;
	__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);

	if(trigger & ETransmit)
		{
		// If Tx transfers were not previously active, it may be necessary to initialise the Tx hardware here, e.g.
		//		AsspRegister::Write32(iChannelBase + KBusModeControlOffset, KIicPslTxModeBitMask);
		
		// Initialise the Tx pointers
		iTxData = iTxBuf + (iWordSize * iTxOffset);
		iTxDataEnd = iTxData + (iWordSize * iNumTxWords);

		__KTRACE_OPT(KIIC, Kern::Printf("Tx Buf:    %x", iTxData));
		__KTRACE_OPT(KIIC, Kern::Printf("Tx Bufend: %x", iTxDataEnd));

		// If using a FIFO, copy the data to it until either the FIFO is full or all the data has been copied
		// This could be achieved with something similar to the following lines:
		//		while(AsspRegister::Read32(iChannelBase + KFifoLevelOffset) <= (KFifoMaxLevel - iWordSize) &&
		//		      iTxData != iTxDataEnd)
		// For the template port, will just use a dummy variable (dummyFifoLvlChk )in place of the register read
		TInt dummyFifoLvlChk = 0;
		while((dummyFifoLvlChk)	&&	// Replace this dummy variable with a read of the hardware
			(iTxData != iTxDataEnd))
			{
			// TUint8 nextTxValue = *iTxData;	// For this example, assumes one byte of data is to be transmitted
												// but if operating in 16-bit mode, bytes may need arranging for
												// endianness

			// Write to the Tx register with something similar to the following:
			//		AsspRegister::Write32(iChannelBase + KTxFifoOffset, nextTxValue);

			iTxData += iWordSize;	// Then increment the pointer to the data. In this example, 8-bit mode is assumed
									// (iWordSize=1), but if operating in 16-bit mode iTxData would be incremented
									// by the number of bytes specified in iWordSize
			}
		// If a Tx FIFO is not being used, a single Tx value would be written - in which case the above loop would be replaced
	
		__KTRACE_OPT(KIIC, Kern::Printf("After adding:\n\rTx Buf:    %x", iTxData));
		__KTRACE_OPT(KIIC, Kern::Printf("Tx Bufend: %x", iTxDataEnd));
		}

	if(trigger & EReceive) 
		{
		// Initialise the Rx pointers
		iRxData = iRxBuf + (iWordSize * iRxOffset);
		iRxDataEnd = iRxData + (iWordSize * iNumRxWords);

		__KTRACE_OPT(KIIC, Kern::Printf("Rx Buffer:  %x", iRxData));
		__KTRACE_OPT(KIIC, Kern::Printf("Rx Bufend: %x", iRxDataEnd));

		// If Rx transfers were not previously active, it may be necessary to initialise the Rx hardware here, e.g.
		//		AsspRegister::Write32(iChannelBase + KBusModeControlOffset, KIicPslRxModeBitMask);
		}

	// If there is some common configuration required to support Rx, Tx transfers, may do it here

	return r;
	}


// The gateway function for PSL implementation
//
// This method is called by the PIL to perform one or more operations indicated in the bitmask aOperation,
// which corresponds to members of the TPslOperation enumeration.
//
TInt DIicBusChannelSlavePsl::DoRequest(TInt aOperation)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("\nDIicBusChannelSlavePsl::DoRequest, Operation 0x%x\n", aOperation));

	TInt r = KErrNone;
	TInt intState;

	if (aOperation & EAsyncConfigPwrUp)
		{
		// The PIL has requested asynchronous operation of CaptureChannel.
		// The PSL should start the processing required for a channel to be captured, and then return immediately with
		// error code KErrNone (if the processing was started without error), so that the client thread will be unblocked. 
		// The PSL is expected to be implemented as an asynchronous state machine, where events (for example hardware 
		// interrupts, or timer expiry) invoke callback functions that advance the state machine to the next state. Once
		// all the required states have been transitioned, so that the PSL part of the CaptureChannel processing is 
		// complete, the PSL should call the PIL function ChanCaptureCallback - this will lead to the Client-provided
		// callback being executed in the context of the client thread
		// 
		__KTRACE_OPT(KIIC, Kern::Printf("EAsyncConfigPwrUp"));
		r = AsynchConfigureInterface();
		if (r != KErrNone)
			{
			__KTRACE_OPT(KIIC, Kern::Printf("AsynchConfigureInterface returned %d\n", r));
			}
		return r;
		}

	if (aOperation & ESyncConfigPwrUp)
		{
		// The PIL has requested synchronous operation of CaptureChannel.
		// The PSL should perform the processing required for a channel to be captured, and return a system-wide error
		// code when this is complete to indicate the status of the capture.
		// Capturing a channel is expected to include initialisation of the hardware to enable operation in accordance
		// with the configuration specified in the PIL member variable iConfigHeader, which holds the configuration
		// specified by the Client.
		//
		__KTRACE_OPT(KIIC, Kern::Printf("ESyncConfigPwrUp"));
		r = ConfigureInterface();
		if (r != KErrNone)
			{
			__KTRACE_OPT(KIIC, Kern::Printf("ConfigureInterface returned %d\n", r));
			return r;
			}
		}

	if (aOperation & ETransmit)
		{
		// The PIL has requested that a Tx operation be started. 
		// Since the SPL may support simultaneous Rx and Tx operations, just set the flag in the iTrigger bitmask to 
		// indicate what has been requested. If both Rx and Tx operations are requested, and one completes ahead of the other,
		// requiring the Client to provide a new buffer and associated call to DoRequest (as is the case if the Master wishes
		// to transfer more data than the Slave buffer supported), it is possible that the other transfer could complete while
		// this function is running; consequently, it may attempt to access iTrigger, and so cause data corruption. To cater for
		// such situations, use a spin lock to guard access to iTrigger.
		// When the same check has been performed for Rx, call the InitTransfer function to start the required transfers.
		//
		__KTRACE_OPT(KIIC, Kern::Printf("ETransmit"));
		intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
		iTrigger |= ETransmit;
		__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);
		}

	if (aOperation & EReceive)
		{
		// The PIL has requested that a Rx operation be started. 
		// Since the SPL may support simultaneous Rx and Tx operations, just set the flag in the iTrigger bitmask to 
		// indicate what has been requested. If both Rx and Tx operations are requested, and one completes ahead of the other,
		// requiring the Client to provide a new buffer and associated call to DoRequest (as is the case if the Master wishes
		// to transfer more data than the Slave buffer supported), it is possible that the other transfer could complete while
		// this function is running; consequently, it may attempt to access iTrigger, and so cause data corruption. To cater for
		// such situations, use a spin lock to guard access to iTrigger.
		// When the same check has been performed for Tx, call the InitTransfer function to start the required transfers.
		//
		__KTRACE_OPT(KIIC, Kern::Printf("EReceive"));
		intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
		iTrigger |= EReceive;
		__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);
		}

	if (aOperation & (EReceive | ETransmit))
		{
		// This code should only be executed once it has been checked whether Rx and Tx operations are required.
		r = InitTransfer();
		}

	if (aOperation & EAbort)
		{
		// The PIL has requested that the current transaction be aborted.
		// This is the case if the Client has not responded within an expected time to specify the next steps in
		// the transaction processing. The time allowed is specified by calling PIL function SetClientWaitTime, otherwise 
		// the time defaults to KSlaveDefCWaitTime.
		// If the PSL is able to satisfy this request it should, at a minimum, disable interrupts and update the member 
		// variables that indicate a transaction is in progress. If the PSL is unable to satisfy the request then the same
		// behaviour will follow as if this request had not been made, so there is no point in modifying the state variables.
		// If both Rx and Tx operations had been requested, and one completes ahead of the other, it is possible that the other 
		// transfer could complete while this function is running; consequently, it may attempt to access iTrigger and iInProgress,
		// and so cause data corruption. To cater for such situations, use a spin lock to guard access to iTrigger.
		// The PIL makes no assumptions of whether the PSL can support this request or not, and does not check the return 
		// value - so there is no need to set one.
		//
		TUint8 dummyCanAbort = 1;	// Dummy variable to represent a check of if it is possible to abort the current transaction
		__KTRACE_OPT(KIIC, Kern::Printf("EAbort"));
		intState = __SPIN_LOCK_IRQSAVE(IicPslSpinLock);
		if(dummyCanAbort)
			{
			// The spin lock has been acquired, so it is safe to modify data and hardware registers that may be accessed as part of 
			// interrupt processing performed by an ISR - this is assuming that the ISR has been written to acquire the same spin lock.
			// Limit the processing to only that which is necessary to be processed under spin lock control, so as to not delay other
			// threads of execution that are waiting for the spin lock to be freed.
			// Hardware may be configured using code similar to the following:
			//		AsspRegister::Write32(iChannelBase + KBusInterruptEnableOffset, KIicPslBusDisableBitMask);
			iInProgress = EFalse;
			iTrigger = 0;
			}
		__SPIN_UNLOCK_IRQRESTORE(IicPslSpinLock, intState);
		// Having released the spin lock, now perform any actions that are not affected by pre-emption by an ISR, this may include code
		// such as the following
		//		Interrupt::Disable(iRxInterruptId);
		//		Interrupt::Disable(iTxInterruptId);
		}

	if (aOperation & EPowerDown)
		{
		// The PIL has requested that the channel be released.
		// If this channel is not part of a MasterSlave channel, the next Client will operate in Slave mode. In this case, it may only
		// be necessary to disable interrupts, and reset the channel hardware.
		// If this channel represents the Slave of a MasterSlave channel, it is possible that some of the hardware is shared between the
		// Master and Slave sub-channels. Since it may not be known whether the next Client of the parent channel will require operation
		// in either Master or Slave mode, some additional processing may be required to allow for subsequent Master operation (for example.
		// unbinding an interrupt).
		//
		__KTRACE_OPT(KIIC, Kern::Printf("EPowerDown"));

		// Resetting the hardware may be achieved with calls similar to the following:
		//		AsspRegister::Write32(iChannelBase + KBusInterruptEnableOffset, KIicPslBusDisableBitMask);
		//		GPIO::SetPinMode(aPinId, GPIO::EDisabled);

		// Disable interrupts may be achieved with calls similar to the following:
		//		Interrupt::Disable(iRxInterruptId);
		//		Interrupt::Disable(iTxInterruptId);

		// Unbinding an ISR may be achieved with calls similar to the following:
		//		Interrupt::Unbind(iRxInterruptId);
		//		Interrupt::Unbind(iTxInterruptId);

		// The PIL checks the return value so the variable r should be modified in the event of failure with a system-wide error code
		// for example, if a register could not be modified,
		//		r = KErrGeneral;
		//		__KTRACE_OPT(KIIC, Kern::Printf("EPowerDown failed with error %d\n",r));

		}
	return r;
	}


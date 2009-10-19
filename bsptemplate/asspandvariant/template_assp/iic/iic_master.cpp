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
// #include <gpio.h>	// Include if using GPIO functionality
#include "iic_psl.h"
#include "iic_master.h"


// Method called when transmission activity is ended. The method is used to indicate
// the success or failure reported in the first parameter
//
// All timers are cancelled, relevant interrupts are disabled and the transaction
// request is completed by calling the relevant PIL method.
//
void DIicBusChannelMasterPsl::ExitComplete(TInt aErr, TBool aComplete /*= ETrue*/)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::ExitComplete, aErr %d, aComplete %d", aErr, aComplete));

	// Disable interrupts for the channel
	// with something similar to the following lines:
	//		Interrupt::Disable(iRxInterruptId);
	//		Interrupt::Disable(iTxInterruptId);

	// Cancel timers and DFCs..
	CancelTimeOut();
	iHwGuardTimer.Cancel();
	iTransferEndDfc.Cancel();

	// Change the channel state to EIdle so that subsequent transaction requests can be accepted
	// once the current one has been completed
	iState = EIdle;

	// Call the PIL method to complete the request
	if(aComplete)
		{
		CompleteRequest(aErr);
		}
	}

// Callback function for the iHwGuardTimer timer. 
//
// Called in ISR context if the iHwGuardTimer expires. Sets iTransactionStatus to KErrTimedOut
//
void DIicBusChannelMasterPsl::TimeoutCallback(TAny* aPtr)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::TimeoutCallback"));
	DIicBusChannelMasterPsl *a = (DIicBusChannelMasterPsl*) aPtr;
	a->iTransactionStatus = KErrTimedOut;
	}

// HandleSlaveTimeout
//
// This method is called by the PIL in the case of expiry of a timer started by the PSL. It is
// specificaly intended to guard against the Slave not responding within an expected time
//
// The PIL method StartSlaveTimeoutTimer is available for the PSL to start the timer (this is 
// called from ProcessNextTransfers, below).
// The PIL method CancelTimeOut is available for the PSL to cancel the same timer (this is called
// from ExitComplete, above)
//
// The PIL will call CompleteRequest() after this function returns, so the PSL needs only to clean-up
//
TInt DIicBusChannelMasterPsl::HandleSlaveTimeout()
	{
	__KTRACE_OPT(KIIC, Kern::Printf("HandleSlaveTimeout"));

	// Ensure that the hardware has ceased transfers, with something similar to the following line:
	//		AsspRegister::Write32(a->aRegisterSetBaseAddress + KBusConfigOffset, KIicPslBusDisableBit);
	//		GPIO::SetPinMode(aPinId, GPIO::EDisabled);

	// Stop the PSL's operation, and inform the PIL of the timeout
	ExitComplete(KErrTimedOut, EFalse);

	// Perform any further hardware manipulation necessary
	//

	return KErrTimedOut;
	}


// DFC 
//
// For execution when a Rx buffer has been filled or a Tx buffer has been emptied
//
void DIicBusChannelMasterPsl::TransferEndDfc(TAny* aPtr)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::TransferEndDfc"));
	DIicBusChannelMasterPsl *a = (DIicBusChannelMasterPsl*) aPtr;

	//	Start of optional processing - not necessary for all implementations
	//
	// When operating full-duplex transfers, one of the Rx and Tx operations may have caused an interrupt
	// before the other has completed. For the example here, the Tx is assumed to have completed before the Rx
	// and a timer is used to ensure that the outstanding Rx operation completes within an expected time.
	//
	// If there has been no error so far, may want to check if we are still receiving the data
	if(a->iTransactionStatus == KErrNone)
		{
		// Use an active wait since this is likely to be a brief check.
		// Start the guard timer (which will update iTransactionStatus with KErrTimedOut if it expires)
		// while also polling the hardware to check if transmission has ceased.
		// 
		// Polling the hardware would be something like the line below
		//		(AsspRegister::Read32(a->aRegisterSetBaseAddress + KStatusRegisterOffset) & KTransmissionActive));
		// but for the template port will use a dummy condition (value of 1)
		//
		a->iHwGuardTimer.OneShot(NKern::TimerTicks(KTimeoutValue));
		while((a->iTransactionStatus == KErrNone) && 1);	// Replace 1 with a register read
		}
	//
	// Now the timer has expired, deactivate the slave select until the current state has been processed, but only
	// if this is not an extended transaction, in which case we want to leave the bus alone so that the multiple
	// transactions making up the extended transaction become one big transaction as far as the bus is concerned.
	// Do this with something similar to the following line:
	// if (!(a->iCurrTransaction->Flags() & KTransactionWithMultiTransc))
	//	{
	//	GPIO::SetPinMode(aPinId, GPIO::EDisabled);
	//	}
	//
	// Disable the hardware from further transmissions
	// AsspRegister::Write32(a->aRegisterSetBaseAddress + KBusConfigOffset, KIicPslBusDisableBit);
	//
	// Check if the guard timer expired
	if(a->iTransactionStatus != KErrNone)
		{
		__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::TransferEndDfc(): error %d",a->iTransactionStatus));
		a->ExitComplete(a->iTransactionStatus); // report the error
		return;
		}
	else
		{
		// Transfer completed successfully, so just cancel the guard timer
		a->iHwGuardTimer.Cancel();
		}
	//	End of optional processing - not necessary for all implementations

	// At this point, prepare for subsequent transfers by performing any necessary clean-up.
	// As stated above, for this example, it is assumed that any Rx or Tx transfer has completed - 
	// the following just checks if an Rx, Tx operation was started, and assumes that they completed.

	if(a->iOperation.iOp.iIsReceiving)
		{
		// If the channel has been receiving data, may need to ensure that any FIFO used has been drained
		// The example here checks if one extra data item remains in the FIFO
		// To check if data remains in a FIFO, something similar to the following could be used:
		//		TInt8 dataRemains = AsspRegister::Read32(a->aRegisterSetBaseAddress + KRxFifoLevel);
		// Reading data from the FIFO would be achieved with something like the line below
		//		value = AsspRegister::Read32(a->aRegisterSetBaseAddress + KRxData);
		// but for the template port will just use a dummy values (data remains=1, value = 1)
		//
		TInt8 dataRemains = 1;	// Would be a check of a Rx FIFO level

		// If data remains in the Rx FIFO and the Rx buffer is not full, copy the data to the buffer
		if(dataRemains && (a->iRxDataEnd - a->iRxData >= a->iWordSize) )
			{
			TUint8 value = 1;			// Would be a read of the Rx FIFO data
			*a->iRxData = value;		// For this example, assumes one byte of data has been read from the FIFO
										// but if operating in 16-bit mode, two "values" would be written the buffer
			a->iRxData += a->iWordSize;	// In this example, 8-bit mode is assumed (iWordSize=1)
										// but if operating in 16-bit mode a->iRxData would be incremented by 
										// the number of bytes specified in a->iWordSize
			}
		}

	if(a->iOperation.iOp.iIsTransmitting)
		{
		// If the channel has been transmitting data, may need to ensure that any FIFO used has been flushed
		// To check if data remains in a FIFO, something similar to the following could be used:
		//		TInt8 dataRemains = AsspRegister::Read32(a->aRegisterSetBaseAddress + KTxFifoLevel);
		// The means to flush the FIFO will be platform specific, and so no example is given here.
		}

	// Start the next transfer for this transaction, if any remain
	if(a->iState == EBusy)
		{
		TInt err = a->ProcessNextTransfers();
		if(err != KErrNone)
			{
			// If the next transfer could not be started, complete the transaction with
			// the returned error code
			a->ExitComplete(err);
			}
		}
	}


// ISR Handler
//
// If the channel is to be event driven, it will use interrupts that indicate the 
// hardware has received or transmitted. To support this an ISR is required.
//
void DIicBusChannelMasterPsl::IicIsr(TAny* aPtr)
	{
	DIicBusChannelMasterPsl *a = (DIicBusChannelMasterPsl*) aPtr;

	// The processing for Rx and Tx will differ, so must determine the status of the interrupts.
	// This will be PSL-specific, but is likely to achieved by reading a status register, in a
	// way similar to this:
	//		TUint32 status = AsspRegister::Read32(aRegisterSetBaseAddress + KStatusRegisterOffset);
	//
	// For the purposes of compiling the template port, just initialise status to zero.
	TUint32 status = 0;

	if(status & KIicPslTxInterrupt)
		{
		// Tx interrupt processing

		// Clear the interrupt source, with something similar to the following line:
		//		AsspRegister::Write32(a->aRegisterSetBaseAddress + KStatusRegisterOffset, KIicPslTxInterrupt);

		// Then check whether all the required data has been transmitted.
		if(a->iTxData == a->iTxDataEnd)
			{
			// All data sent, so disable the Tx interrupt and queue a DFC to handle the next steps.
			//		Interrupt::Disable(a->iTxInterruptId);
			a->iTransferEndDfc.Add();
			}
		else
			{
			if(a->iOperation.iOp.iIsTransmitting)
				{
				// Data remaining - copy the next value to send to the Tx register

				// TUint8 nextTxValue = *a->iTxData;	// For this example, assumes one byte of data is to be transmitted
														// but if operating in 16-bit mode, bytes may need arranging for
														// endianness

				// Write to the Tx register with something similar to the following:
				//		AsspRegister::Write32(a->aRegisterSetBaseAddress + KTxRegisterOffset, nextTxValue);

				a->iTxData += a->iWordSize;	// Then increment the pointer to the data. In this example, 8-bit mode is assumed
											// (iWordSize=1), but if operating in 16-bit mode a->iTxData would be incremented
											// by the number of bytes specified in a->iWordSize
				}
			}
		}

	if(status & KIicPslRxInterrupt)
		{
		// Rx interrupt processing

		// Copy the received data to the Rx buffer. 
		// Do this in a loop in case there are more than one units of data to be handled. Data availability
		// will be indicated by a PSL-specific register, and so may be handled by code similar to the following:
		//		while(AsspRegister::Read32(a->aRegisterSetBaseAddress + KRxData))
		// 
		// But, to allow compilation of the template port, just use a dummy condition (while(1)):
		while(1)
			{
			// While there is space in the buffer, copy received data to it
			if((a->iRxDataEnd - a->iRxData) >= a->iWordSize)
				{
				TUint8 nextRxValue = 0;
				// Read from the Rx register with something similar to the following:
				//		TUint8 nextRxValue = AsspRegister::Read32(aRegisterSetBaseAddress + KRxRegisterOffset);
				*a->iRxData = nextRxValue;
				}
			else
				{
				// If there is no space left in the buffer an Overrun has occurred
				a->iTransactionStatus = KErrOverflow;
				break;
				}

			// Increment the pointer to the received data
			a->iRxData += a->iWordSize;
			}

		// If the Rx buffer is now full, finish the transmission.
		if(a->iRxDataEnd == a->iRxData)
			{
			// Disable the interrupt since it is no longer required
			//		Interrupt::Disable(a->iRxInterruptId);

			// Then queue a DFC to perform the next steps
			a->iTransferEndDfc.Add();
			}
		
		// After processing the data, clear the interrupt source (do it last to prevent the ISR being
		// re-invoked before this ISR is finished), with something similar to the following line:
		//		AsspRegister::Write32(a->aRegisterSetBaseAddress + KStatusRegisterOffset, KIicPslRxInterrupt);
		}
	}

// Constructor, first stage
//
// The PSL is responsible for setting the channel number - this is passed as the first parameter to
// this overload of the base class constructor
//
DIicBusChannelMasterPsl::DIicBusChannelMasterPsl(TInt aChannelNumber, TBusType aBusType, TChannelDuplex aChanDuplex) :
	DIicBusChannelMaster(aBusType, aChanDuplex),				// Base class constructor
	iTransferEndDfc(TransferEndDfc, this, KIicPslDfcPriority),	// DFC to handle transfer completion
	iHwGuardTimer(TimeoutCallback, this)						// Timer to guard against hardware timeout
	{
	iChannelNumber = aChannelNumber;	// Set the iChannelNumber of the Base Class
	iState = EIdle;						// Initialise channel state machine

	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::DIicBusChannelMasterPsl: iChannelNumber = %d", iChannelNumber));
	}

// Second stage construction
//
// Allocate and initialise objects required by the PSL channel implementation
//
TInt DIicBusChannelMasterPsl::DoCreate()
	{
	__KTRACE_OPT(KIIC, Kern::Printf("\nDIicBusChannelMasterPsl::DoCreate() ch: %d \n", iChannelNumber));

	TInt r = KErrNone;

	// Interrupt IDs (such as iRxInterruptId, iTxInterruptId, used in this file)would be initialised here.
	//
	// Also, information relevant to the channel number (such as the base register address, 
	// aRegisterSetBaseAddress) would be initialised here.

	// Create the DFCQ to be used by the channel
	if(!iDfcQ)
		{
		TBuf8<KMaxName> threadName (KIicPslThreadName);
		threadName.AppendNum(iChannelNumber);		// Optional: append the channel number to the name
		r = Kern::DfcQCreate(iDfcQ, KIicPslThreadPriority, &threadName);
		if(r != KErrNone)
			{
			__KTRACE_OPT(KIIC, Kern::Printf("DFC Queue creation failed, channel number: %d, r = %d\n", iChannelNumber, r));
			return r;
			}
		}

	// PIL Base class initialization - this must be called prior to SetDfcQ(iDfcQ)
	r = Init();			
	if(r == KErrNone)
		{
		// Call base class function to set DFCQ pointers in the required objects
		// This also enables the channel to process transaction requests
		SetDfcQ(iDfcQ); 

		// PSL DFCQ initialisation for local DFC
		iTransferEndDfc.SetDfcQ(iDfcQ); 

#ifdef CPU_AFFINITY_ANY
		NKern::ThreadSetCpuAffinity((NThread*)(iDfcQ->iThread), KCpuAffinityAny);
#endif
		// Bind interrupts.
		// This would be with something similar to the following lines:
		//		iMasterIntId = Interrupt::Bind(interruptIdToUse, IicPslIsr, this);
		//
		// Interrupt::Bind returns interruptId or an error code (negative value)
		if(iMasterIntId < KErrNone)
			{
			__KTRACE_OPT(KIIC, Kern::Printf("ERROR: InterruptBind error.. %d", r));
			r = iMasterIntId;
			}
		}
	return r;
	}

// New
//
// A static method used to construct the DIicBusChannelMasterPsl object.
DIicBusChannelMasterPsl* DIicBusChannelMasterPsl::New(TInt aChannelNumber, const TBusType aBusType, const TChannelDuplex aChanDuplex)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::NewL(): ChannelNumber = %d, BusType =%d", aChannelNumber, aBusType));
	DIicBusChannelMasterPsl *pChan = new DIicBusChannelMasterPsl(aChannelNumber, aBusType, aChanDuplex);

	TInt r = KErrNoMemory;
	if(pChan)
		{
		r = pChan->DoCreate();
		}
	if(r != KErrNone)
		{
		delete pChan;
		pChan = NULL;
		}
	return pChan;
	}

// Optional method - that determines if the previous transaction header is different to the current one.
// If configuration is the same, the hardware may not need to be re-initialized. 
TBool DIicBusChannelMasterPsl::TransConfigDiffersFromPrev()
	{
	// 
	// The header will be specific to a particular bus type. Using a fictional
	// bus type Abc, code similar to the following could be used to compare each
	// member of the previous header with the current one
	// 
	//		TConfigAbcBufV01* oldHdrBuf = (TConfigAbcBufV01*) iPrevHeader;
	//		TConfigAbcV01 &oldHeader = (*oldHdrBuf)();
	//		TConfigAbcBufV01* newHdrBuf = (TConfigAbcBufV01*) iCurrHeader;
	//		TConfigAbcV01 &newHeader = (*newHdrBuf)();
	//		if(	(newHeader.iHeaderMember != oldHeader.iHeaderMember)	||
	//			... )
	//			{
	//			return EFalse;
	//			}
	return ETrue;
	}


// CheckHdr is called by the PIL when a transaction is queued, in function
// QueueTransaction. This is done in the context of the Client's thread.
//
// The PSL is required to check that the transaction header is valid for
// this channel.
//
// If the pointer to the header is NULL, return KErrArgument.
// If the content of the header is not valid for this channel, return KErrNotSupported.
//
TInt DIicBusChannelMasterPsl::CheckHdr(TDes8* aHdrBuff)
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
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::CheckHdr() r %d", r));
	return r;
	}

// Initialise the hardware with the data provided in the transaction and slave-address field
//
// If a specified configuration is not supported, return KErrNotSupported
// If a configuration is supported, but the implementing configuration fails, return KErrGeneral
//
TInt DIicBusChannelMasterPsl::ConfigureInterface()
	{
	__KTRACE_OPT(KIIC, Kern::Printf("ConfigureInterface()"));

	// This method will be platform-specific and will configure the hardware as required to support
	// the current transacation. This will be supported with something similar to the following:
	//		AsspRegister::Write32(a->aRegisterSetBaseAddress + KBusConfigOffset, KIicPslBusEnableBit);
	//		GPIO::SetPinMode(aPinId, GPIO::EEnabled);

	// Depending on the platform, timers (such as iHwGuardTimer) may be used to check that the hardware
	// responds in the required way within an allowed timeout. Since this is configuring the channel for
	// an operation, it is acceptable to perform an active wait, with something similar to the following:
	//		iTransactionStatus = KErrNone;
	//		iHwGuardTimer.OneShot(NKern::TimerTicks(KTimeoutValue));
	//		while((iTransactionStatus == KErrNone) &&
	//			AsspRegister::Read32(a->aRegisterSetBaseAddress + KBusConfigOffset, KIicPslBusEnableBit);
	//		if(iTransactionStatus != KErrNone)
	//			{
	//			return KErrGeneral;
	//			}
	//		else
	//			{
	//			iHwGuardTimer.Cancel();
	//			}
	return KErrNone;
	}


// Method called by StartTransfer to actually initiate the transfers. It manipulates the hardware to
// perform the required tasks.
//
TInt DIicBusChannelMasterPsl::DoTransfer(TInt8 *aBuff, TUint aNumOfBytes, TUint8 aType)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("\nDIicBusChannelMasterPsl::DoTransfer() - aBuff=0x%x, aNumOfBytes=0x%x\n",aBuff,aNumOfBytes));

	TInt r = KErrNone;

	// Validate the input arguments
	if((aBuff == NULL) || (aNumOfBytes == 0))
		{
		r = KErrArgument;
		}
	else
		{
		// This method will be platform-specific and will configure the hardware as required
		// This will likely be supported with calls similar to the following:
		//		AsspRegister::Write32(a->aRegisterSetBaseAddress + KBusConfigOffset, KIicPslBusEnableBit);
		//		GPIO::SetPinMode(aPinId, GPIO::EEnabled);
		//		Interrupt::Enable(aInterruptId);
		//
		// Steps that may be typically required are described below

		switch(aType)
			{
			case TIicBusTransfer::EMasterWrite:
				{
				// If using a Tx FIFO, may wish to disable transmission until the FIFO has been filled to certain level
				// If using a Tx FIFO, may wish to flush it and re-initialise any counters or pointers used

				// If using a FIFO, copy data to it until either the FIFO is full or all data has been copied
				// Checking the FIFO is full will be reading a flag in a status register, by use of code similar the following
				//		TUint32 status = AsspRegister::Read32(aRegisterSetBaseAddress + KStatusRegisterOffset);
				//		if(status & KTxFifoFullBitMask)
				//			... FIFO is full
				//
				// For this example base port, represent the FIFO full status by a dummy value (zero).
				TUint8 txFifoFull = 0;
				while(!txFifoFull && (iTxData != iTxDataEnd))
					{
					// TUint8 nextTxValue = *iTxData;	// For this example, assumes one byte of data is to be transmitted
															// but if operating in 16-bit mode, bytes may need arranging for
															// endianness

					// Write to the Tx FIFO register with something similar to the following:
					//		AsspRegister::Write32(a->aRegisterSetBaseAddress + KTxRegisterOffset, nextTxValue);

					iTxData += iWordSize;	// Then increment the pointer to the data. In this example, 8-bit mode is assumed
												// (iWordSize=1), but if operating in 16-bit mode a->iTxData would be incremented
												// by the number of bytes specified in a->iWordSize
					}
				// May wish to enable transmission now - or wait until after the Read transfer has been initialised
				break;
				}
			case TIicBusTransfer::EMasterRead:
				{
				// If using an Rx FIFO, it will already have been drained at the end of the last transfer by TransferEndDfc
				// so no need to do it again here.

				// May wish to enable reception now - or group with the code, below
				break;
				}
			default:
				{
				__KTRACE_OPT(KIIC, Kern::Printf("Unsupported TransactionType %x", aType));
				r = KErrArgument;
				break;
				}
			}

		// Final stages of hardware preparation
		//
		// Enable hardware interrupts
		// Finally, enable (start) transmission and reception
		}

	return r;
	}


// This method performs the initialisation required for either a read or write transfer
// and then invokes the next stage of the processing (DoTransfer)
//
TInt DIicBusChannelMasterPsl::StartTransfer(TIicBusTransfer* aTransferPtr, TUint8 aType)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::StartTransfer() - aTransferPtr=0x%x, aType=%d",aTransferPtr,aType));

	if(aTransferPtr == NULL)
		{
		__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::StartTransfer - NULL pointer\n"));
		return KErrArgument;
		}

	TInt r = KErrNone;

	switch(aType)
		{
		case TIicBusTransfer::EMasterWrite:
			{
			__KTRACE_OPT(KIIC, Kern::Printf("Starting EMasterWrite, duplex=%d", iFullDTransfer));

			// Get a pointer to the transfer object's buffer, to facilitate passing arguments to DoTransfer
			const TDes8* aBufPtr = GetTferBuffer(aTransferPtr);

			__KTRACE_OPT(KIIC, Kern::Printf("Length %d, iWordSize %d", aBufPtr->Length(), iWordSize));

			// Store the current address and ending address for Transmission - they are required by the ISR and DFC
			iTxData = (TInt8*) aBufPtr->Ptr();
			iTxDataEnd = (TInt8*) (iTxData + aBufPtr->Length());

			__KTRACE_OPT(KIIC, Kern::Printf("Tx: Start: %x, End %x, bytes %d\n\n", iTxData, iTxDataEnd, aBufPtr->Length()));

			// Set the flag to indicate that we'll be transmitting data
			iOperation.iOp.iIsTransmitting = ETrue;

			// initiate the transmission..
			r = DoTransfer((TInt8 *) aBufPtr->Ptr(), aBufPtr->Length(), aType);
			if(r != KErrNone)
				{
				__KTRACE_OPT(KIIC, Kern::Printf("Starting Write failed, r = %d", r));
				}
			break;
			}

		case TIicBusTransfer::EMasterRead:
			{
			__KTRACE_OPT(KIIC, Kern::Printf("Starting EMasterRead, duplex=%x", iFullDTransfer));

			// Get a pointer to the transfer object's buffer, to facilitate passing arguments to DoTransfer
			const TDes8* aBufPtr = GetTferBuffer(aTransferPtr);

			// Store the current address and ending address for Reception - they are required by the ISR and DFC
			iRxData = (TInt8*) aBufPtr->Ptr();
			iRxDataEnd = (TInt8*) (iRxData + aBufPtr->Length());

			__KTRACE_OPT(KIIC, Kern::Printf("Rx: Start: %x, End %x, bytes %d", iRxData, iRxDataEnd, aBufPtr->Length()));

			// Set the flag to indicate that we'll be receiving data
			iOperation.iOp.iIsReceiving = ETrue;

			// initiate the reception
			r = DoTransfer((TInt8 *) aBufPtr->Ptr(), aBufPtr->Length(), aType);
			if(r != KErrNone)
				{
				__KTRACE_OPT(KIIC, Kern::Printf("Starting Read failed, r = %d", r));
				}
			break;
			}

		default:
			{
			__KTRACE_OPT(KIIC, Kern::Printf("Unsupported TransactionType %x", aType));
			r = KErrArgument;
			break;
			}
		}

	return r;
	}

// This method determines the next transfers to be processed, and passes them to the next stage
// in the processing (StartTransfer).
//
// This is called from DoRequest (for the first transfer) and TransferEndDfc (after a transfer
// has completed)
//
TInt DIicBusChannelMasterPsl::ProcessNextTransfers()
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::ProcessNextTransfers(),BUSY=%d", iState));

	// Since new transfers are strating, clear exisiting flags
	iOperation.iValue = TIicOperationType::ENop;

	// Some hardware preparation may be required before starting the transfer using something similar
	// to the following line:
	//		AsspRegister::Write32(a->aRegisterSetBaseAddress + KBusConfigOffset, KIicPslBusEnableBit);

	// If this is the first transfer in the transaction the channel will be in state EIdle
	if(iState == EIdle)
		{
		// Get the pointer to half-duplex transfer object..
		iHalfDTransfer = GetTransHalfDuplexTferPtr(iCurrTransaction);

		// Get the pointer to full-duplex transfer object..
		iFullDTransfer = GetTransFullDuplexTferPtr(iCurrTransaction);

		// Update the channel state to EBusy and initialise the transaction status
		iState = EBusy;
		iTransactionStatus = KErrNone;

		// Use the PIL funcitonality to start a timer that will timeout if the transaction
		// is not completed within a specified time period (the client may have specified a period
		// to use in the transaction header - some something similar tot he following could be used)
		//		StartSlaveTimeOutTimer(iCurrHeader->iTimeoutPeriod);
		//
		// If the timer expires, callback function HandleSlaveTimeout (implemented by the PSL, above)
		// will be called. This will ensure that the hardware ceases transfer activity, and calls ExitComplete
		// with KErrTImedOut, which will return the channel state to EIdle.
		}
	else
	// If not in state EIdle, process the next transfer in the linked-list held by the transaction
		{
		// Get the pointer the next half-duplex transfer object..
		iHalfDTransfer = GetTferNextTfer(iHalfDTransfer);

		// Get the pointer to the next half-duplex transfer object..
		if(iFullDTransfer)
			{
			iFullDTransfer = GetTferNextTfer(iFullDTransfer);
			}
		}

	TInt r = KErrNone;
	if(!iFullDTransfer && !iHalfDTransfer)
		{
		// If all of the transfers were completed, just notify the PIL and return.
		// (if either Rx or Tx has not finished properly ExitComplete() would have been called
		// from TransferEndDfc if there was an error during the transfer)
		__KTRACE_OPT(KIIC, Kern::Printf("All transfers completed successfully"));

		ExitComplete(KErrNone);
		}
	else
		{
		// Transfers remain to be processed
		//
		// For full-duplex transfers, the order in which read and write transfers are started may be significant.
		// Below is an example where the read transfer is explicitly started before the write transfer.
		TInt8 hDTrType = (TInt8) GetTferType(iHalfDTransfer);

		if(iFullDTransfer)
			{
			if(hDTrType == TIicBusTransfer::EMasterRead)
				{
				r = StartTransfer(iHalfDTransfer, TIicBusTransfer::EMasterRead);
				if(r != KErrNone)
					{
					return r;
					}
				r = StartTransfer(iFullDTransfer, TIicBusTransfer::EMasterWrite);
				}
			else // hDTrType == TIicBusTransfer::EMasterWrite)
				{
				r = StartTransfer(iFullDTransfer, TIicBusTransfer::EMasterRead);
				if(r != KErrNone)
					{
					return r;
					}
				r = StartTransfer(iHalfDTransfer, TIicBusTransfer::EMasterWrite);
				}
			}
		else
		// This is a HalfDuplex transfer - so just start it
			{
			r = StartTransfer(iHalfDTransfer, hDTrType);
			}
		}
	return r;
	}

// The gateway function for PSL implementation
//
// This method is called by the PIL to initiate the transaction. After finishing it's processing,
// the PSL calls the PIL function CompleteRequest to indicate the success (or otherwise) of the request
//
TInt DIicBusChannelMasterPsl::DoRequest(TIicBusTransaction* aTransaction)
	{
	__KTRACE_OPT(KIIC, Kern::Printf("DIicBusChannelMasterPsl::DoRequest (aTransaction=0x%x)\n", aTransaction));

	// If the pointer to the transaction passed in as a parameter, or its associated pointer to the
	// header information is NULL, return KErrArgument
	if(!aTransaction || !GetTransactionHeader(aTransaction))
		{
		return KErrArgument;
		}

	// The PSL operates a simple state machine to ensure that only one transaction is processed
	// at a time - if the channel is currently busy, reject the request with KErrInUse.
	if(iState != EIdle)
		{
		return KErrInUse;
		}

	// Make a copy of the pointer to the transaction
	iCurrTransaction = aTransaction;

	// Configure the hardware to support the transaction
	TInt r = KErrNone;
	if(TransConfigDiffersFromPrev())	// Optional: check if hardware needs reconfiguration
		{
		r = ConfigureInterface();
		if(r != KErrNone)
			{
			return r;
			}
		}

	// start processing transfers of this transaction.
	r = ProcessNextTransfers();
	return r;
	}


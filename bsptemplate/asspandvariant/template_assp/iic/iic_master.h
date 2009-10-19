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



#ifndef __IIC_MASTER_H_
#define __IIC_MASTER_H_

#include <drivers/iic_channel.h>
// #include platform specific header files here ...

_LIT(KIicPslThreadName,"IicPslChannelThread_");

const TInt KIicPslDfcPriority = 0; // Arbitrary, can be 0-7, 7 highest
const TInt KIicPslThreadPriority = 24;

// DIicBusChannelMasterPsl class declaration:
class DIicBusChannelMasterPsl: public DIicBusChannelMaster
	{
public:
	// Method to create a channel
	static DIicBusChannelMasterPsl* New(TInt aChannelNumber, const TBusType aBusType, const TChannelDuplex aChanDuplex);

	// Gateway function for PSL implementation
	virtual TInt DoRequest(TIicBusTransaction* aTransaction);

	// Overloaded constructor
	DIicBusChannelMasterPsl(TInt aChannelNumber, const TBusType aBusType, const TChannelDuplex aChanDuplex);

private:
	// Override base-class pure virtual methods
	virtual TInt DoCreate(); // 2nd stage construction.
	virtual TInt CheckHdr(TDes8* aHdr);
	virtual TInt HandleSlaveTimeout();

	// Internal methods
	TInt ConfigureInterface();
	TBool TransConfigDiffersFromPrev();		// Optional method - potentially saving hardware re-configuration
	TInt DoTransfer(TInt8 *aBuff, TUint aNumOfBytes, TUint8 aType);
	TInt StartTransfer(TIicBusTransfer* aTransferPtr, TUint8 aType);
	TInt ProcessNextTransfers();
	void ExitComplete(TInt aErr, TBool aComplete = ETrue);

	// Dfc and timeout Callback functions
	static void TransferEndDfc(TAny* aPtr);
	static void TransferTimeoutDfc(TAny* aPtr);
	static void TimeoutCallback(TAny* aPtr);

	// ISR handler.
	static void IicIsr(TAny* aPtr);

	// DFC for handling transfer completion
	TDfc iTransferEndDfc;

	// Flags indicating the current Rx/Tx activity
	TIicOperationType iOperation;

	// Granularity of data transmitted
	TUint8 iWordSize;

	// Current state of the channel. The channel shuould not accept requests for new
	// transactions until the current one is complete. The following enumeration and 
	// state variable are used to control this.
	enum TMyState
		{
		EIdle,
		EBusy
		};
	TUint8 iState;

	// Register base for the Master channel
	TUint iMasterChanBase;

	// Interrupt ID for the Master channel
	TInt iMasterIntId;

	// Pointers used to store current transfers information
	TIicBusTransfer* iHalfDTransfer;
	TIicBusTransfer* iFullDTransfer;

	// Pointer to the current transaction.
	TIicBusTransaction* iCurrTransaction;

	// Pointers to buffers used for Rx and Tx transfers
	TInt8 *iTxData;
	TInt8 *iRxData;
	TInt8 *iTxDataEnd;
	TInt8 *iRxDataEnd;

	// Timer to guard against hardware timeout
	NTimer iHwGuardTimer;

	// Status of the transaction
	volatile TInt iTransactionStatus;

	// Optional - pointers to the previous, and newly-requested transaction's headers
	// These would be of a type that is speific to the bus type supported by the channel.
	// Here, they are of a fictional bus type, 'Abc', and so are commented-out
	//		TConfigAbcBufV01* iPrevHeader;
	//		TConfigAbcBufV01* iCurrHeader
	};

#endif //__IIC_MASTER_H_

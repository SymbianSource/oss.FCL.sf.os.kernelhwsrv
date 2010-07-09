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
// e32/include/drivers/iic.h
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.

/**
@file
@internalTechnology
*/

#ifndef __IIC_H__
#define __IIC_H__

#include <kernel/kern_priv.h> // for DThread;
#include <e32ver.h>

#include <drivers/iic_transaction.h>

const TInt KIicBusImpExtMaxPriority = KExtensionMaximumPriority-9;	// A kernel extension priority to use as an offset
																	// for all IIC bus implemenetations
const TInt KMaxNumCapturedChannels = 4;	// Arbitrary limit on the maximum number of channels captured for Slave operation

const TUint KControlIoMask =			0xF0000000;
const TUint KMasterControlIo =		0x40000000;
const TUint KSlaveControlIo =		0x00000000;
const TUint KMasterSlaveControlIo =	0x80000000;

class TIicBusCallback; // Forward declaration
class DIicBusController;


//
// Generic interface for clients
//

class IicBus
	{
public:
	// Master-side API

    /**
	@publishedPartner
	@prototype 9.6
    Queues a transaction synchronously.

    @param aBusId			A token containing the bus realisation variability.
    @param aTransaction		A pointer to a transaction object containing the details of the transaction.

    @return KErrNone, when successfully completed;
			KErrArgument, if aBusId is not a valid token or aTransaction is NULL;
			KErrTimedOut, if the channel terminates the transaction because  the addressed Slave exceeded the alloted time to respond;
            KErrNotSupported, if either the channel does not support Master mode or the transaction is not valid on this channel (e.g. valid full duplex transaction queued on half duplex channel).
    */
	IMPORT_C static TInt QueueTransaction(TInt aBusId, TIicBusTransaction* aTransaction);

    /**
	@publishedPartner
	@prototype 9.6
    Queues a transaction asynchronously.

    @param aBusId			A token containing the bus realisation variability.
    @param aTransaction		A pointer to a transaction object containing the details of the transaction.
    @param aCallback		A pointer to a callback object.

    @return KErrNone, if successfully accepted; KErrArgument, if aBusId is not a valid token or either aTransaction or aCallback are NULL;
            KErrNotSupported, if either the channel does not support Master mode or the transaction is not valid on this channel(e.g. valid full duplex transaction queued on half duplex channel).
    */
	IMPORT_C static TInt QueueTransaction(TInt aBusId, TIicBusTransaction* aTransaction, TIicBusCallback* aCallback);

    /**
	@publishedPartner
	@prototype 9.6
    Cancels a previously queued transaction.

    @param aBusId			A token containing the bus realisation variability.
    @param aTransaction		A pointer to a transaction object containing the details of the transaction.

    @return KErrCancel, if successfully cancelled; KErrArgument, if aBusId is not a valid token or aTransaction is NULL;
			KErrNotFound if the transaction cannot be found on channel's queue of transactions;
			KErrInUse if this method is called on a transaction that has already been started;
            KErrNotSupported, if the channel does not support Master mode, or the method is called on a synchronous transaction..
    */
	IMPORT_C static TInt CancelTransaction(TInt aBusId, TIicBusTransaction* aTransaction);

	// Slave-side API

	/**
	@publishedPartner
	@prototype 9.6
    Capture a Slave channel.

    @param aBusId		A token containing the bus realisation variability.
    @param aConfigHdr	A pointer to a descriptor containing the device specific configuration option applicable to all transactions.
    @param aCallback	A pointer to a callback to be called upon specified triggers.
    @param aChannelId	If this API is to complete synchronously, and the processing was successful, then on return aChannelId
                      contains a platform-specific cookie that uniquely identifies the channel instance to be used by this client.
                      If the processing was unsuccessful for the synchronous completion case, aChannelId will be unchanged.
                      If the API was called to complete asynchronously, aChannelId will, in all cases, be set to zero; the valid
                      value for the cookie will be set by the callback.
	@param aAsynch		A boolean value that indicates if this API is to complete synchronously (EFalse) or asynchronously (ETrue).

    @return KErrNone, if successfully captured, or if API is asynchronous, if the request to capture the channel was accepted;
			KErrInUse if channel is already in use; KErrArgument, if aBusId is not a valid token or aCallback is NULL;
            KErrNotSupported, if the channel does not support Slave mode.
    */
	IMPORT_C static TInt CaptureChannel(TInt aBusId, TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch=EFalse);

	/**
	@publishedPartner
	@prototype 9.6
    Release a previously captured Slave channel.

    @param aChannelId	The channel identifier cookie for this client.

    @return KErrNone, if successfully released;
			KErrInUse if a transaction is currently underway on this channel; KErrArgument, if aChannelId is not a valid cookie;
    */
	IMPORT_C static TInt ReleaseChannel(TInt aChannelId);

	/**
	@publishedPartner
	@prototype 9.6
	Register a receive buffer with this Slave channel.

    @param aChannelId	The channel identifier cookie for this client.
    @param aRxBuffer	A pointer to the receive buffer, in a client created descriptor.
	@param aBufGranularity The number of buffer bytes used to store a word.
    @param aNumWords	The number of words expected to be received.
    @param aOffset		The offset from the start of the buffer where to store the received data.

    @return KErrNone, if succesfully registered;
			KErrAlreadyExists if a receive buffer is already pending;
			KErrArgument, if aChannelId is not a valid cookie, or if the pointer descriptor is NULL;
    */
	IMPORT_C static TInt RegisterRxBuffer(TInt aChannelId, TPtr8 aRxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset);

	/**
	@publishedPartner
	@prototype 9.6
	Register a transmit buffer with this Slave channel.
	This client may create a single buffer for the entire transaction and control where the received data
	is to be placed and the transmit data is to be found, by specifying the number of bytes to transmit (receive)
	and the offset into the buffer.

    @param aChannelId	The channel identifier cookie for this client.
    @param aTxBuffer	A pointer to the transmit buffer, in a client created descriptor.
	@param aBufGranularity The number of buffer bytes used to store a word.
    @param aNumWords	The number of words to be transmitted.
    @param aOffset		The offset from the start of the buffer where to fetch the data to be transmitted.

    @return KErrNone, if successfully registered;
			KErrAlreadyExists if a transmit buffer is already pending;
			KErrArgument, if aChannelId is not a valid cookie, or if the pointer descriptor is NULL;
    */
	IMPORT_C static TInt RegisterTxBuffer(TInt aChannelId, TPtr8 aTxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset);

	/**
	@publishedPartner
	@prototype 9.6
	Sets the notification triggers and readies the receive path and/or kick starts a transmit (if the node is being addressed).
	It is only after a receive buffer has been registered and this API has been called that the channel is ready to receive data (when addressed).
	If a transmit buffer has been registered and this API has been called the channel will immediately transmit when addressed by the Master
	If the channel supports full duplex, both paths can be readied in one call to this API.

    @param aChannelId	The channel identifier cookie for this client.
    @param aTrigger		A bitmask specifying the notification trigger. Masks for individual triggers are specified by the TIicBusSlaveTrigger enumeration.

    @return KErrNone, if successful;
			KErrArgument, if aChannelId is not a valid cookie, or if the trigger is invalid for this channel;
			KErrInUse if a transaction is already underway on this channel;
			KErrTimedOut, if the client exceeded the alloted time to respond by invoking this API;
    */
	IMPORT_C static TInt SetNotificationTrigger(TInt aChannelId, TInt aTrigger);

	/**
	@publishedPartner
	@prototype 9.6
	Interface to provide extended functionality

    @param aId			A token containing the bus realisation variability or the channel identifier cookie for this client.
    @param aFunction	A function identifier. If bit 31 is set and bit 30 cleared it is used to extend the Master-Slave channel;
						if bit 31 is cleared and bit 30 is set, it extends the Master channel; if both bits 31 and 30 are cleared it
						extends the Slave channel interface.
    @param aParam1		A generic argument to be passed to the function identified by aFunction.
    @param aParam2		A generic argument to be passed to the function identified by aFunction.

    @return KErrNone, if successful;
			KErrNotSupported, function is not supported;
			Any other system wide error code.
	*/
    IMPORT_C static TInt StaticExtension(TUint aId, TUint aFunction, TAny* aParam1, TAny* aParam2);
	};

// Forward declaration
class DIicBusChannel;
class DIicBusChannelSearcher;

NONSHARABLE_CLASS(DIicBusController) : public DBase
	{
public:
	// constructor
	inline DIicBusController() {};
	~DIicBusController();

	TInt Create();

	/**
	@publishedPartner
	@prototype 9.6
	Interface to be used by Channel implementations to register a list of supported channels. This method
	will check if the array of pointers to channels is already being modified and, if so, will return KErrInUse.
	Otherwise, the iArrayBusy flag will be set and each entry in the list inserted into the array according
	to a sorting algorithm; the iArrayBusy flag will then be cleared after inserting the last entry.

    @param aListChannels	A pointer to a linked list of DIicBusChannel instances to be registered
    @param aNumberChannels	The number of channels to be registered.

    @return KErrNone, if successful;
			KErrInUse, if the array of pointers to channels is already being modified;
			Any other system wide error code.
	*/
	IMPORT_C static TInt RegisterChannels(DIicBusChannel** aListChannels, TInt aNumberChannels);

	/**
	@publishedPartner
	@prototype 9.6
	Interface to be used by Channel implementations to deregister a channel. This method
	will check if the array of pointers to channels is already being modified and, if so, will return KErrInUse.
	Otherwise, the channel will be removed from the array according to a sorting algorithm; the iArrayBusy
	flag will be set before searching for the channel and cleared immediately after removing it.

    @param aListChannels	A pointer to a linked list of DIicBusChannel instances to be registered
    @param aNumberChannels	The number of channels to be registered.

    @return KErrNone, if successful;
			KErrInUse, if the array of pointers to channels is already being modified;
			KErrArgument, if aChannel is a NULL pointer or represents and invalid channel type
			KErrNotFound, if aChannel is not found in the array of pointers to channels
			Any other system wide error code.
	*/
	IMPORT_C static TInt DeRegisterChannel(DIicBusChannel* aChannel);

	// implementation of public interface APIs
	TInt QueueTransaction(TInt aBusId, TIicBusTransaction* aTransaction);
	TInt QueueTransaction(TInt aBusId, TIicBusTransaction* aTransaction, TIicBusCallback* aCallback);
	TInt CancelTransaction(TInt aBusId, TIicBusTransaction* aTransaction);
	TInt CaptureChannel(TInt aBusId, TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch);
	TInt ReleaseChannel(TInt aChannelId);
	TInt RegisterRxBuffer(TInt aChannelId, TPtr8 aRxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset);
	TInt RegisterTxBuffer(TInt aChannelId, TPtr8 aTxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset);
	TInt SetNotificationTrigger(TInt aChannelId, TInt aTrigger);
	TInt StaticExtension(TUint aId, TUint aFunction, TAny* aParam1, TAny* aParam2);

	static TInt OrderEntries(const DIicBusChannel& aMatch, const DIicBusChannel& aEntry);

private:
	TInt GetSlaveChanPtr(TInt aChannelId, DIicBusChannelSlave*& aSlaveChanPtr);
	TInt GetChanPtr(const TInt aBusId, TInt &aIndex, DIicBusChannel*& aChan);

	inline RPointerArray<DIicBusChannel>* ChannelArray();

	TInt GetChanWriteAccess();
	void FreeChanWriteAccess();
	TInt GetChanReadAccess();
	void FreeChanReadAccess();

	TInt RequestTypeSupported(const TInt aBusId, DIicBusChannelMaster* const aChannel);

public:
	/**
	@internalComponent
	@prototype 9.6
	Class used by DIicBusController for locating channels captured by clients for Slave Mode operation
	*/
	class TCapturedChannel	// Generated by DIicBusChannelSlave
		{
		public:
			inline TCapturedChannel();
			inline TCapturedChannel(TInt aChannelId, DIicBusChannelSlave* aChanPtr);

			inline TCapturedChannel& operator=(TCapturedChannel& aChan);
			inline TInt operator==(TCapturedChannel& aChan);

		public:
			TInt iChannelId;	// ID to be used by client
			DIicBusChannelSlave* iChanPtr;	// Index to iChannelArray
		};
public:
	/**
	@internalTechnology
	@prototype 9.6
	Function to register a Slave channel with DIicBusController as captured
	*/
	virtual TInt InstallCapturedChannel(const TInt aChannelId, const DIicBusChannelSlave* aChanPtr);
private:
	RPointerArray<DIicBusChannel> iChannelArray;
	TCapturedChannel iCapturedChannels[KMaxNumCapturedChannels];
	TInt InsertCaptChanInArray(TCapturedChannel aCapturedChan);
	TInt RemoveCaptChanFromArray(TCapturedChannel aCapturedChan);
	TInt FindCapturedChan(TCapturedChannel aCapturedChan, TInt& aIndex);
	TInt FindCapturedChanById(TCapturedChannel aCapturedChan, TInt& aIndex);

	TInt DeInstallCapturedChannel(const TInt aChannelId, const DIicBusChannelSlave* aChanPtr);

#ifdef _DEBUG
	void DumpCapturedChannels();
	void DumpChannelArray();
#endif

private:
	enum TArrayActivity
		{
		EWriteInProgress=0x1,
		EReadInProgress=0x2
		};

	TSpinLock *iChanLock;
	TUint32 iChanRdCount;	// Maximum 32-bit count for concurrent reads
	TSpinLock *iCaptLock;

	TInt8 iChanRwFlags;		// Bit 0 for write, bit 1 for read
	};


#include <drivers/iic.inl>

#endif  // #ifndef __IIC_H__


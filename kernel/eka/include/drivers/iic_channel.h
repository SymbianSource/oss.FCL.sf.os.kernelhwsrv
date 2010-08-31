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
// e32/include/drivers/iic_channel.h
// Include file for channel implementation
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.

/**
@file
@internalTechnology
*/
#ifndef __IIC_CHANNEL_H__
#define __IIC_CHANNEL_H__

#ifdef STANDALONE_CHANNEL
#include <drivers/iic_transaction.h>
#else
#include <drivers/iic.h>
#endif

static const char KIicChannelPanic[]="Iic Channel PIL";

const static TInt KChannelTypeMask	= 0x03;
const static TInt KBusTypeShift		= 2;
const static TInt KBusTypeMask		= 0x07<<KBusTypeShift; // number of buses may grow in future
const static TInt KChannelDuplexShift= 5;
const static TInt KChannelDuplexMask	= 0x01<<KChannelDuplexShift;
const static TInt16 KTransCountMsBit = (TInt16)((TUint16)0x8000);

const static TInt8 KMaxWaitTime = 0x7E; // Maximum allowable time in milliseconds for a Slave channel to wait for a response
										// (from Master or Client). This constant is used to limit run-time selected values
										// for timeouts. The value stated here is semi-arbitrary.

#ifdef IIC_SIMULATED_PSL
// In a real system, the following timeout values are likely to be excessive. However, they are available
// for use in the test framework, to account for the processing overhead.
// In particular, for the simulated master timeout tests.
const TInt KSlaveDefMWaitTime = 125;
const TInt KSlaveDefCWaitTime = 124;

#else
const TInt KSlaveDefMWaitTime = 1;	// Default wait time for Master timeout. PSL can use SetMasterWaitTime to override.
const TInt KSlaveDefCWaitTime = 1;	// Default wait time for Client timeout. PSL can use SetClientWaitTime to override.
#endif


/**
@internalComponent
@prototype 9.6
Base class for a Channel (not directly instantiable)
*/
class DIicBusChannel : public DBase
	{
public:
	enum TChannelType
		{
		EMaster			= 0,
		ESlave			= 0x01,
		EMasterSlave	= 0x02
		};
	enum TBusType
		{
		EI2c	   = 0,
		ESpi	   = 0x01,
		EMicrowire = 0x02,
		ECci	   = 0x03,
		ESccb	   = 0x04
		};
	enum TChannelDuplex
		{
		EHalfDuplex = 0,	// supports only half duplex transactions (even if bus spec supports full duplex)
		EFullDuplex = 0x1	// supports full duplex transactions (queud transactions may still be half duplex)
		};

public:
	virtual TInt StaticExtension(TUint aFunction, TAny* /*aParam1*/, TAny* /*aParam2*/)
		{
#ifdef _DEBUG
		if(aFunction == KCtrlIoDumpChan)
			{
			DumpChannel();
			return KErrNone;
			}
		else
#else
			(void)aFunction;
#endif

			return KErrNotSupported;
		};
protected:
	// constructor
	inline DIicBusChannel(TChannelType aChanType, TBusType aBusType, TChannelDuplex aChanDuplex);
	// second phase construction - empty, to be implemented by derived types if required
	virtual TInt DoCreate()=0;

	// helper function to read an set flags
	inline TChannelType ChannelType();
	inline void SetChannelType(TChannelType aChanType);
	inline TBusType BusType();
	inline void SetBusType(TBusType aBusType);
	inline TChannelDuplex ChannelDuplex();
	inline void SetChannelType(TChannelDuplex aChanDuplex);
	inline TInt8 ChannelNumber() const;

	virtual TInt CheckHdr(TDes8* aHdr) = 0;	// PSL to check the header is valid for this channel
protected:
#ifdef _DEBUG
	inline void DumpChannel();
#endif
protected:
	TInt8 iChannelNumber;	// this is the Key for ordering channels in the array
	TUint8 iFlags;			// combination of TChannelType, TChannelDuplex and TBusType
	TInt8 iSpare1;
	TInt8 iSpare2;
private:
	TAny* iReserved;

	friend class DIicBusController;
	};

/**
@publishedPartner
@prototype 9.6

Base class for a Master Channel (not directly instantiable)

*/
class DIicBusChannelMaster : public DIicBusChannel
	{
public:
	// interface to Bus Controller (implemented by PIL)
	// For stand-alone channel, there is no controller. So some parts of
	// the interface are exported for client direct use. 
	/**
    @publishedPartner
    @prototype 9.6
    Master channel interface to queue a transaction synchronously.

    @param aTransaction		A pointer to a transaction object containing the details of the transaction.

    @return KErrNone, when successfully completed;
			KErrArgument, if aTransaction is NULL;
			KErrTimedOut, if the channel terminates the transaction because  the addressed Slave exceeded the alloted time to respond;
            KErrNotSupported, if either the channel does not support Master mode or the transaction is not valid on this channel (e.g. valid full duplex transaction queued on half duplex channel).
    */
	virtual TInt QueueTransaction(TIicBusTransaction* aTransaction);
    /**
	@publishedPartner
	@prototype 9.6
    Master channel interface to queue a transaction asynchronously.

    @param aTransaction		A pointer to a transaction object containing the details of the transaction.
    @param aCallback		A pointer to a callback object.

    @return KErrNone, if successfully accepted; KErrArgument, if either aTransaction or aCallback are NULL;
            KErrNotSupported, if either the channel does not support Master mode or the transaction is not valid on this channel(e.g. valid full duplex transaction queued on half duplex channel).
    */
	virtual TInt QueueTransaction(TIicBusTransaction* aTransaction, TIicBusCallback* aCallback);
    /**
	@publishedPartner
	@prototype 9.6
    Master channel interface to cancel a previously queued transaction.

    @param aTransaction		A pointer to a transaction object containing the details of the transaction.

    @return KErrCancel, if successfully cancelled; KErrArgument, if aTransaction is NULL;
			KErrNotFound if the transaction cannot be found on channel's queue of transactions;
			KErrInUse if this method is called on a transaction that has already been started;
            KErrNotSupported, if the channel does not support Master mode, or the method is called on a synchronous transaction..
    */
	virtual TInt CancelTransaction(TIicBusTransaction* aTransaction);
	/**
	@publishedPartner
	@prototype 9.6
	Master channel interface interface to provide extended functionality

    @param aFunction	A function identifier. If bit 31 is set and bit 30 cleared it is used to extend the Master-Slave channel;
						if bit 31 is cleared and bit 30 is set, it extends the Master channel; if both bits 31 and 30 are cleared it
						extends the Slave channel interface.
    @param aParam1		A generic argument to be passed to the function identified by aFunction.
    @param aParam2		A generic argument to be passed to the function identified by aFunction.

    @return KErrNone, if successful;
			KErrNotSupported, function is not supported;
			Any other system wide error code.
	*/
	virtual TInt StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2);

//
	virtual TInt Spare1(TInt aVal, TAny* aPtr1, TAny* aPtr2);

	/**
	@publishedPartner
	@prototype 9.6
	Destructor for DIicBusChannelMaster
	*/
	~DIicBusChannelMaster();

protected:
	// PSL implemented
	/**
	@publishedPartner
	@prototype 9.6
	Gateway function for PSL implementation (to be called by the interface APIs)
	*/
	virtual TInt DoRequest(TIicBusTransaction* aTransaction) = 0;
	/**
	@publishedPartner
	@prototype 9.6
	Function to be invoked in the event of a Slave timeout. May be overridden by the PSL.
	*/
	virtual TInt HandleSlaveTimeout() = 0;
	/**
	@publishedPartner
	@prototype 9.6
	Second phase constructor to be implemented by the PSL
	*/
	virtual TInt DoCreate() = 0;

	// Called by PSL
	/**
	@publishedPartner
	@prototype 9.6

	Constructor for DIicBusChannelMaster

	@param aBusType		Argument to specify the type of Bus
    @param aChanDuplex	Argument to specify the duplex support offered by this channel
	*/
	DIicBusChannelMaster(TBusType aBusType, TChannelDuplex aChanDuplex);
	/**
	@publishedPartner
	@prototype 9.6
	Function to instigate DIicBusChannel initialisation

	@return KErrNone if no error
			KErrNoMemory if allocation of required objects
	*/
	TInt Init();
	/**
	@publishedPartner
	@prototype 9.6

	Function to start the timer to check the Slave responsiveness.

	@param aTime		Timeout in milliseconds

	@return KErrNone if no error,
			KErrInUse if timer is already active.

	*/
	TInt StartSlaveTimeOutTimer(TInt aTime);
	/**
	@publishedPartner
	@prototype 9.6

	Function to specify the DFC queue for the channel to use

	@param aDfcQue		Pointer to the DFC queue to use

	*/
	void SetDfcQ(TDfcQue* aDfcQue);
	/**
	@publishedPartner
	@prototype 9.6

	Function to Complete the transaction being processed by the channel

	@param aResult		Error code to complete the transaction with

	*/
	void CompleteRequest(TInt aResult);
	/**
	@publishedPartner
	@prototype 9.6

	Function to cancel the timer to check the Slave responsiveness.

	Call either in a thread or an IDFC context. Do not call from an ISR.

	*/
	void CancelTimeOut();

	// Methods to make private data of TIicBusTransaction object accessible to derivatives of this class
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the Transaction Header of a specified TIicBusTransaction object

	@return The Transaction Header of the specified TIicBusTransaction object

	@see TIicBusTransaction
	*/
	static inline TDes8* GetTransactionHeader(const TIicBusTransaction* aTransaction);
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the Half Duplex Transfer pointer of a specified TIicBusTransaction object

	@return The Half Duplex Transfer pointer of the specified TIicBusTransaction object

	@see TIicBusTransaction
	*/
	static inline TIicBusTransfer* GetTransHalfDuplexTferPtr(const TIicBusTransaction* aTransaction);
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the Full Duplex Transfer pointer of a specified TIicBusTransaction object

	@return The Full Duplex Transfer pointer of the specified TIicBusTransaction object

	@see TIicBusTransaction
	*/
	static inline TIicBusTransfer* GetTransFullDuplexTferPtr(const TIicBusTransaction* aTransaction);
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the address of the callback object of a specified TIicBusTransaction object

	@return The address of the callback object of the specified TIicBusTransaction object

	@see TIicBusTransaction
	*/
	static inline TIicBusCallback* GetTransCallback(const TIicBusTransaction* aTransaction);
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the value of the TransFlags member of a specified TIicBusTransaction object

	@return The value of the TransFlags member of the specified TIicBusTransaction object

	@see TIicBusTransaction
	*/
	static inline TUint8 GetTransFlags(const TIicBusTransaction* aTransaction);

	// Methods to make private data of TIicBusTransfer object accessible to derivatives of this class
	/**
	@publishedPartner
	@prototype 9.6

	Function to return Transfer Type the of a specified TIicBusTransfer object

	@return The Transfer Type of the specified TIicBusTransfer object

	@see TIicBusTransfer
	*/
	static inline TInt8 GetTferType(const TIicBusTransfer* aTransfer);
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the Buffer Granularity of a specified TIicBusTransfer object

	@return The Buffer Granularity of the specified TIicBusTransfer object

	@see TIicBusTransfer
	*/
	static inline TInt8 GetTferBufGranularity(const TIicBusTransfer* aTransfer);
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the descriptor for the data for a specified TIicBusTransfer object

	@return The descriptor for the data for the specified TIicBusTransfer object

	@see TIicBusTransfer
	*/
	static inline const TDes8* GetTferBuffer(const TIicBusTransfer* aTransfer);
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the address of the next transfer for a specified TIicBusTransfer object

	@return The address of the next transfer for the specified TIicBusTransfer object

	@see TIicBusTransfer
	*/
	static inline TIicBusTransfer* GetTferNextTfer(const TIicBusTransfer* aTransfer);

	// Methods to make private data of TIicBusTransactionPreamble object accessible to derivatives of this class
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the function pointer for a specified TIicBusTransactionPreamble object

	@return The function pointer for the specified TIicBusTransactionPreamble object

	@see TIicBusTransactionPreamble
	*/
	static inline TIicBusPreamble GetPreambleFuncPtr(const TIicBusTransactionPreamble* aTransfer);
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the function argument for a specified TIicBusTransactionPreamble object

	@return The function argument for the specified TIicBusTransactionPreamble object

	@see TIicBusTransactionPreamble
	*/
	static inline TAny* GetPreambleFuncArg(const TIicBusTransactionPreamble* aTransfer);

	/**
	@publishedPartner
	@prototype 9.6

	Function to return the function pointer of a specified TIicBusTransactionMultiTransc object

	@return The function pointer of the specified TIicBusTransactionMultiTransc object

	@see TIicBusTransactionMultiTransc
	*/
	static inline TIicBusMultiTranscCbFn GetMultiTranscFuncPtr(const TIicBusTransactionMultiTransc* aTransfer);
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the function argument of a specified TIicBusTransactionMultiTransc object

	@return The function argument of the specified TIicBusTransactionMultiTransc object

	@see TIicBusTransactionMultiTransc
	*/
	static inline TAny* GetMultiTranscFuncArg(const TIicBusTransactionMultiTransc* aTransfer);

	/**
	@publishedPartner
	@prototype 9.6

	Function to return the function pointer of a specified TIicBusTransactionPreambleExt object

	@return The function pointer of the specified TIicBusTransactionPreambleExt object

	@see TIicBusTransaction
	*/
	static inline TIicBusMultiTranscCbFn GetExtTranscFuncPtr(const TIicBusTransactionPreambleExt* aTransfer);
	/**
	@publishedPartner
	@prototype 9.6

	Function to return the function argument of a specified TIicBusTransactionPreambleExt object

	@return The function argument of the specified TIicBusTransactionPreambleExt object

	@see TIicBusTransaction
	*/
	static inline TAny* GetExtTranscFuncArg(const TIicBusTransactionPreambleExt* aTransfer);

private:
	// Function to acquire the NFastMutex of the channel
	void Lock();
	// Function to release the NFastMutex of the channel
	void Unlock();

	// function to run on receiving a message
    static void MsgQFunc(TAny* aPtr);

	TIicBusTransaction* NextTrans(TIicBusTransaction* aTrans);
	void EndTransaction(TIicBusTransaction* aTrans, TInt aResult, TIicBusCallback* aCb);
	void Complete(TInt aResult, TIicBusTransaction* aTransaction);
	void UnlockAndKick();

	static void SlaveTimeoutCallback(TAny*);

	// Used by DIidBusController (a friend of this class)
	TInt TransFlow(TIicBusTransaction* aTransaction);

	TInt8 IsMasterBusy();

protected:
	TDfcQue* iDfcQ;

private:
	TDfc iTransQDfc;
	SOrdQue iTransactionQ;
	TIicBusTransaction* iTransaction;			// Pointer to current transaction
	TIicBusTransaction* iCurrentTransaction;	// Pointer to current fragment of a multiple transaction

	NFastMutex iTransactionQLock;

	TDfc* iSlaveTimeoutDfc;

	NTimer iTimeoutTimer;	// timeout timer

	TInt16 iTransCount; // Count of pending transactions
	TInt8 iChannelReady;
	TInt8 iSpare1;

private:
	TAny* iReserved1;
	TAny* iReserved2;

	friend class DIicBusChannelMasterSlave;
	friend class DIicBusController; // For static method DIicBusController::DeRegisterChannel
	};


/**
@publishedPartner
@prototype 9.6

Base class for a Slave Channel (not directly instantiable)

*/
class DIicBusChannelSlave : public DIicBusChannel
	{
protected:
    /**
	@publishedPartner
	@prototype 9.6
	The set of operation values for processing by the PSL
    */
	enum TPslOperation
		{
		ESyncConfigPwrUp = 0x01,
		EAsyncConfigPwrUp = 0x02,
		EPowerDown = 0x04,
		ETransmit = 0x08,
		EReceive = 0x10,
		EAbort = 0x20
		};
private:
	// Values used by the internal state machine
	enum TSlaveTimerStates
		{
		EInactive = 0x01,
		EWaitForMaster = 0x02,
		EWaitForClient = 0x04,
		EClientTimeout = 0x08
		};

public:

	// Interface to Controller.
    // For stand-alone channel, the interface is exported.
    /**
    @publishedPartner
    @prototype 9.6
    Capture this Slave channel.

    @param aConfigHdr	A pointer to a descriptor containing the device specific configuration option applicable to all transactions.
    @param aCallback	A pointer to a callback to be called upon specified triggers.
    @param aChannelId	If this API is to complete synchronously, and the processing was successful, then on return aChannelId
                      contains a platform-specific cookie that uniquely identifies the channel instance to be used by this client.
                      If the processing was unsuccessful for the synchronous completion case, aChannelId will be unchanged.
                      If the API was called to complete asynchronously, aChannelId will, in all cases, be set to zero; the valid
                      value for the cookie will be set by the callback.
    @param aAsynch		A boolean value that indicates if this API is to complete synchronously (EFalse) or asynchronously (ETrue).

    @return KErrNone, if successfully captured, or if API is asynchronous, if the request to capture the channel was accepted;
			KErrInUse if channel is already in use; KErrArgument, if aCallback is NULL;
            KErrNotSupported, if the channel does not support Slave mode.
    */
	virtual TInt CaptureChannel(TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch);
	/**
	@publishedPartner
	@prototype 9.6
    Release this previously captured Slave channel.

    @return KErrNone, if successfully released;
			KErrInUse if a transaction is currently underway on this channel; KErrArgument
    */
	virtual TInt ReleaseChannel();
	/**
	@publishedPartner
	@prototype 9.6
	Register a receive buffer with this Slave channel.

    @param aRxBuffer	A pointer to the receive buffer, in a client created descriptor.
	@param aBufGranularity The number of buffer bytes used to store a word.
    @param aNumWords	The number of words expected to be received.
    @param aOffset		The offset from the start of the buffer where to store the received data.

    @return KErrNone, if successfully registered;
			KErrAlreadyExists if a receive buffer is already pending;
			KErrArgument, if the pointer descriptor is NULL;
    */
	virtual TInt RegisterRxBuffer(TPtr8 aRxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset);
	/**
	@publishedPartner
	@prototype 9.6
	Register a transmit buffer with this Slave channel.
	This client may create a single buffer for the entire transaction and control where the received data
	is to be placed and the transmit data is to be found, by specifying the number of bytes to transmit (receive)
	and the offset into the buffer.

    @param aTxBuffer	A pointer to the transmit buffer, in a client created descriptor.
	@param aBufGranularity The number of buffer bytes used to store a word.
    @param aNumWords	The number of words to be transmitted.
    @param aOffset		The offset from the start of the buffer where to fetch the data to be transmitted.

    @return KErrNone, if successfully registered;
			KErrAlreadyExists if a transmit buffer is already pending;
			KErrArgument, if the pointer descriptor is NULL;
    */
	virtual TInt RegisterTxBuffer(TPtr8 aTxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset);
	/**
	@publishedPartner
	@prototype 9.6
	Sets the notification triggers and readies the receive path and/or kick starts a transmit (if the node is being addressed).
	It is only after a receive buffer has been registered and this API has been called that the channel is ready to receive data (when addressed).
	If a transmit buffer has been registered and this API has been called the channel will immediately transmit when addressed by the Master
	If the channel supports full duplex, both paths can be readied in one call to this API.

    @param aTrigger		A bitmask specifying the notification trigger. Masks for individual triggers are specified by the TIicBusSlaveTrigger enumeration.

    @return KErrNone, if successful;
			KErrArgument, if the trigger is invalid for this channel;
			KErrInUse if a transaction is already underway on this channel;
			KErrTimedOut, if the client exceeded the alloted time to respond by invoking this API;
    */
	virtual TInt SetNotificationTrigger(TInt aTrigger);
	/**
	@publishedPartner
	@prototype 9.6
	Interface to provide extended functionality

    @param aFunction	A function identifier. If bit 31 is set and bit 30 cleared it is used to extend the Master-Slave channel;
						if bit 31 is cleared and bit 30 is set, it extends the Master channel; if both bits 31 and 30 are cleared it
						extends the Slave channel interface.
    @param aParam1		A generic argument to be passed to the function identified by aFunction.
    @param aParam2		A generic argument to be passed to the function identified by aFunction.

    @return KErrNone, if successful;
			KErrNotSupported, function is not supported;
			Any other system wide error code.
	*/
	virtual TInt StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2);
//
	/**
	@internalTechnology
	@prototype 9.6
	Function reserved for future use
	*/
	virtual TInt Spare1(TInt aVal, TAny* aPtr1, TAny* aPtr2);

	// Interface to TIicBusSlaveCallback
	//
	// ProcessData is a channel-specific function to be provided by the PSL. Its purpose is to process
	// data received/transmitted (called from NotifyClient or DFC queued from it). It Must fill in the
	// aCb object's iReturn, iRxWords and/or iTxWords using the appropriate functions
	//
	// The functions UpdateReqTrig, StartTimerByState and StopTimer are for use by the PIL, only.
	//
	/**
	@publishedPartner
	@prototype 9.6
	Function invoked when an asynchronous event occurs on the Slave channel. Implemented by the PSL.
    @param aTrigger		Argument to indicate the type of event that occurred
    @param aCb			Address of the Slave client callback object to process

    @return KErrNone, if successful;
			KErrNotSupported, function is not supported;
			Any other system wide error code.
	*/
#ifdef STANDALONE_CHANNEL
	friend class TIicBusSlaveCallback;
protected:
    virtual void ProcessData(TInt aTrigger, TIicBusSlaveCallback*  aCb) = 0;
private:
    TInt UpdateReqTrig(TInt8& aCbTrigVal, TInt& aCallbackRet);
    void StartTimerByState();
    void StopTimer();
#else
public:
    virtual void ProcessData(TInt aTrigger, TIicBusSlaveCallback*  aCb) = 0;
    virtual TInt UpdateReqTrig(TInt8& aCbTrigVal, TInt& aCallbackRet);
    virtual void StartTimerByState();
    virtual void StopTimer();
#endif

public:
	// Values used by the Interface to TIicBusSlaveCallback
	enum TSlaveNotifProcSteps
		{
		EStopTimer = 0x01,
		EInvokeCb = 0x02,
		EStartTimer = 0x04
		};

protected:
	// PSL implemented
	/**
	@publishedPartner
	@prototype 9.6

	PSL specific second phase constructor

	@return KErrNone, if succesful;
			KErrNotSupported, function is not supported;
			Any other system wide error code.
	*/
	virtual TInt DoCreate() = 0;
	/**
	@publishedPartner
	@prototype 9.6

	Gateway function for PSL implementation: aOperation is a bitmask made of TPslOperation (to be called by the  interface APIs)

	@return KErrNone, if succesful;
			KErrNotSupported, function is not supported;
			Any other system wide error code.
	*/
	virtual TInt DoRequest(TInt aOperation) = 0;

	// Called by PSL
	/**
	@publishedPartner
	@prototype 9.6

	Constructor for DIicBusChannelSlave

	@param aBusType		Argument to specify the type of Bus
    @param aChanDuplex	Argument to specify the duplex support offered by this channel
	@param aChannelId	Argument to specify the identifier to use for this channel
	*/
	DIicBusChannelSlave(TBusType aBusType, TChannelDuplex aChanDuplex, TInt16 aChannelId);
	/**
	@publishedPartner
	@prototype 9.6
	Function to instigate DIicBusChannelSlave initialisation

	@return KErrNone
	*/
	TInt Init();
	
	/**
	@publishedPartner
	@prototype 9.6
	Destructor for DIicBusChannelSlave
	*/
	~DIicBusChannelSlave();

	/**
	@publishedPartner
	@prototype 9.6
	Function invoked when an asynchronous channel capture completes

	@param aResult		Argument specifying the error code reurned by the capture operation

	@return KErrNone
	*/
	void ChanCaptureCallback(TInt aResult);
	/**
	@publishedPartner
	@prototype 9.6
	Function invoked to instigate processing by the PSL, PIL and Client when an asynchronous event occurs

	@param aResult		Argument specifying the trigger value associated with the asynchronous event

	@return KErrNone
	*/
	void NotifyClient(TInt aTrigger);

	/**
	@publishedPartner
	@prototype 9.6
	Function invoked by the PSL to set the timeout period to wait for a response from the bus master

	@param aWaitTime		Argument specifying the wait time, in milliseconds (limit=KMaxWaitTime)

	@return KErrNone, if succesful;
	KErrArgument, if aWaitTime > KMaxWaitTime
	*/
	TInt SetMasterWaitTime(TInt8 aWaitTime);
	/**
	@publishedPartner
	@prototype 9.6
	Function invoked by the PSL to get the timeout period to wait for a response from the bus master

	@return The wait time, in milliseconds
	*/
	inline TInt8 GetMasterWaitTime();
	/**
	@publishedPartner
	@prototype 9.6
	Function invoked by the PSL to set the timeout period to wait for a response from the Client

	@param aWaitTime		Argument specifying the wait time, in milliseconds (limit=KMaxWaitTime)
	@return KErrNone
	*/
	TInt SetClientWaitTime(TInt8 aWaitTime);
	/**
	@publishedPartner
	@prototype 9.6
	Function invoked by the PSL to get the timeout period to wait for a response from the Client

	@return The wait time, in milliseconds
	*/
	inline TInt8 GetClientWaitTime();
private:
	//Method to instruct PSL to indicate a bus error to the bus Master, then return
	void SendBusErrorAndReturn();
	void SetChannelId(TInt& aChannelId);

	void CompleteAsynchCapture(TInt aResult);
	void SlaveTimerCallBack();
	static void SlaveStaticCB(TAny* aDumPtr);

protected:
	TInt8 iRxGranularity;
	TInt8 iTxGranularity;
	TInt8 iNumRxWords;
	TInt8 iNumTxWords;
	TInt8 iRxOffset;
	TInt8 iTxOffset;
private:
	TInt8 iChannelInUse;
	TInt8 iSpare1;
protected:
	TInt16 iChannelId;		// channel identifier to be returned to client (in aChannelId)
private:
	TInt16 iInstanceCount;	// instance count part of aChannelId
protected:
	TDes8* iConfigHeader;
	TInt8* iTxBuf;
	TInt8* iRxBuf;
private:
	TIicBusSlaveCallback* iNotif;
	TDfc* iClientTimeoutDfc;	// To be queued on the dfc queue used by iNotif
	DThread* iClient;		// stored when client captures channel
#ifndef STANDALONE_CHANNEL
	DIicBusController* iController;
#endif

	TInt8 iTimerState;
	TInt8 iMasterWaitTime;	// 8 bits allows maximum wait time of 0.25 seconds
	TInt8 iClientWaitTime;	// 8 bits allows maximum wait time of 0.25 seconds
	TInt8 iSpare2;

	TInt8 iReqTrig;			// Represents the trigger required by the Client (bitmask from TIicBusSlaveTrigger).
	TInt8 iAccumTrig;		// Represents the events accumulated during the current trigger period
	TInt16 iSpare3;

	TSpinLock iSpinLock;
	NTimer iTimeoutTimer;	// timeout timer
	TAny* iReserved1;
	TAny* iReserved2;

	friend class DIicBusChannelMasterSlave;
	friend class DIicBusController; // For static method DIicBusController::DeRegisterChannel
	};

/**
@internalComponent
@prototype 9.6
The Master-Slave Channel class (not for derivation)
*/
class DIicBusChannelMasterSlave : public DIicBusChannel
	{
public:
	// constructor
#ifdef STANDALONE_CHANNEL
    IMPORT_C DIicBusChannelMasterSlave(TBusType aBusType, TChannelDuplex aChanDuplex, DIicBusChannelMaster* aMasterChan, DIicBusChannelSlave* aSlaveChan);    
#else
	inline DIicBusChannelMasterSlave(TBusType aBusType, TChannelDuplex aChanDuplex, DIicBusChannelMaster* aMasterChan, DIicBusChannelSlave* aSlaveChan);
#endif
	~DIicBusChannelMasterSlave(){delete iMasterChannel; delete iSlaveChannel; }
	inline TInt DoCreate();
	// Master side interface to Bus Controller
	virtual TInt QueueTransaction(TIicBusTransaction* aTransaction);
	virtual TInt QueueTransaction(TIicBusTransaction* aTransaction, TIicBusCallback* aCallback);
	inline TInt CancelTransaction(TIicBusTransaction* aTransaction);

	// Slave side interface to Bus Controller
	virtual TInt CaptureChannel(TDes8* aConfigHdr, TIicBusSlaveCallback* aCallback, TInt& aChannelId, TBool aAsynch);
	virtual TInt ReleaseChannel();  
	inline TInt RegisterRxBuffer(TPtr8 aRxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset);
	inline TInt RegisterTxBuffer(TPtr8 aTxBuffer, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset);
	inline TInt SetNotificationTrigger(TInt aTrigger);
	virtual TInt StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2);

private:
	// Base class support
	virtual TInt CheckHdr(TDes8* /*aHdr*/){	__ASSERT_DEBUG(0,Kern::Fault("DIicBusChannelMasterSlave::CheckHdr",__LINE__));	\
											return KErrGeneral;}; // Not accessed. PSL implementation for Master or Slave used.
protected:
	DIicBusChannelMaster* iMasterChannel;
	DIicBusChannelSlave* iSlaveChannel;
private:

	friend class DIicBusChannelMaster;
	friend class DIicBusChannelSlave;
	friend class DIicBusController; // For static method DIicBusController::DeRegisterChannel
	};

#include <drivers/iic_channel.inl>

#endif  // #ifndef __IIC_CHANNEL_H__


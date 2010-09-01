/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* e32/include/drivers/iic_transaction.h
*
*/

// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.

/**
@file
@internalTechnology
*/

#ifndef __IIC_TRANSACTION_H__
#define __IIC_TRANSACTION_H__

#include <kernel/kern_priv.h> // for DThread;
#include <e32ver.h>

static const char KIicPanic[]="Iic PIL";

const TInt KNumTrancPriorities=8;
const TUint8 KTransactionWithPreamble = 0x80;
const TUint8 KTransactionWithMultiTransc = 0x40;

#ifdef _DEBUG
static const TUint KCtrlIoDumpChan = 1;	// StaticExtension value reserved for printing a channel
#endif

/**
@publishedPartner
@prototype 9.6

The set of endianness values for use with IIC buses

*/
enum TEndianness
	{
	EBigEndian,
	ELittleEndian
	};

/**
@publishedPartner
@prototype 9.6

The set of bit-order values for use with IIC buses

*/
enum TBitOrder
	{
	ELsbFirst,
	EMsbFirst
	};

//
// Bus-specific configuration for SPI bus
//

/**
@publishedPartner
@prototype 9.6

The set of word-width values for use with TConfigSpiV01

@see TConfigSpiV01

*/
enum TSpiWordWidth
	{
	ESpiWordWidth_8,
	ESpiWordWidth_10,
	ESpiWordWidth_12,
	ESpiWordWidth_16
	};

/**
@publishedPartner
@prototype 9.6

The set of clock mode values for use with TConfigSpiV01

@see TConfigSpiV01

*/
enum TSpiClkMode
	{
	ESpiPolarityLowRisingEdge,		// Active high, odd edges
	ESpiPolarityLowFallingEdge,		// Active high, even edges
	ESpiPolarityHighFallingEdge,	// Active low,  odd edges
	ESpiPolarityHighRisingEdge		// Active low,  even edges
	};

/**
@publishedPartner
@prototype 9.6

The set of values to represent the pin sense when selecting a Slave

@see TConfigSpiV01

*/
enum TSpiSsPinMode
    {
    ESpiCSPinActiveLow,      // Active low
    ESpiCSPinActiveHigh     // Active high
    };

/**
@publishedPartner
@prototype 9.6

Class to represent the configuration data for a SPI bus channel registered with IIC

*/
class TConfigSpiV01
	{
public:
	TSpiWordWidth iWordWidth;
	TInt32        iClkSpeedHz;
	TSpiClkMode   iClkMode;
	TInt32        iTimeoutPeriod;
	TEndianness   iEndianness;
	TBitOrder     iBitOrder;
	TUint         iTransactionWaitCycles;
	TSpiSsPinMode iSSPinActiveMode;
	};

typedef TPckgBuf <TConfigSpiV01> TConfigSpiBufV01;


//
// Bus-specific configuration for I2C bus
//

/**
@publishedPartner
@prototype 9.6

The set of address types for use with TConfigI2cV01

@see TConfigI2cV01

*/
enum TI2cAddrType
	{
	EI2cAddr7Bit,
	EI2cAddr10Bit
	};


/**
@publishedPartner
@prototype 9.6

Class to represent the configuration data for a I2C bus channel registered with IIC

*/
class TConfigI2cV01
	{
	public:
	TI2cAddrType	iAddrType;		// 7 or 10-bit addressing
	TInt32			iClkSpeedHz;
	TEndianness		iEndianness;
	TInt32			iTimeoutPeriod;
	};

typedef TPckgBuf <TConfigI2cV01> TConfigI2cBufV01;



// Bus realisation configuration
//
// 31 30 29 28 | 27 26 25 24 | 23 22 21 20 | 19 18 17 16 | 15 14 13 12 | 11 10  9  8 |  7  6  5  4 |  3  2  1  0
//
// 31:29 - HS Master address (I2C only)
// 28    - HS address valid bit
// 27:23 - Reserved
// 22:20 - Bus type
// 19:15 - Channel number
// 14:10 - Transaction speed
//  9:0  - Slave address
#define HS_MASTER_ADDR_SHIFT 29
#define HS_MASTER_ADDR_MASK 0x7
#define HS_ADDR_VALID_SHIFT 28
#define HS_ADDR_VALID_MASK 0x1
#define BUS_TYPE_SHIFT 20
#define BUS_TYPE_MASK 0x7
#define CHANNEL_NO_SHIFT 15
#define CHANNEL_NO_MASK 0x1F
#define TRANS_SPEED_SHIFT 10
#define TRANS_SPEED_MASK 0x1F
#define SLAVE_ADDR_SHIFT 0
#define SLAVE_ADDR_MASK 0x3FF

// The SET_CONF_FIELD and GET_CONF_FIELD are for internal use, only.
// They are to support the set of macros below which access particular fields of the Bus realisation configuration
#define SET_CONF_FIELD(aBusId,aField,aMask,aShift) aBusId=(aBusId&~(aMask<<aShift))|((aField&aMask)<<aShift);
#define GET_CONF_FIELD(aBusId,aMask,aShift) (((aBusId)>>(aShift))&(aMask))

/**
@publishedPartner
@prototype 9.6
Macro to get the HS Master address of a Bus realisation configuration
*/
#define GET_HS_MASTER_ADDR(aBusId) GET_CONF_FIELD(aBusId,HS_MASTER_ADDR_MASK,HS_MASTER_ADDR_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to set the HS Master address of a Bus realisation configuration
*/
#define SET_HS_MASTER_ADDR(aBusId,aHsMasterAddr) SET_CONF_FIELD(aBusId,aHsMasterAddr,HS_MASTER_ADDR_MASK,HS_MASTER_ADDR_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to get the HS address valid bit of a Bus realisation configuration
*/
#define GET_HS_VALID(aBusId,aHsValid) GET_CONF_FIELD(aBusId,aHsValid,HS_ADDR_VALID_MASK,HS_ADDR_VALID_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to set the HS address valid bit of a Bus realisation configuration
*/
#define SET_HS_VALID(aBusId,aHsValid) SET_CONF_FIELD(aBusId,aHsValid,HS_ADDR_VALID_MASK,HS_ADDR_VALID_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to get the Bus type of a Bus realisation configuration
*/
#define GET_BUS_TYPE(aBusId) GET_CONF_FIELD(aBusId,BUS_TYPE_MASK,BUS_TYPE_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to set the Bus type of a Bus realisation configuration
*/
#define SET_BUS_TYPE(aBusId,aBusType) SET_CONF_FIELD(aBusId,aBusType,BUS_TYPE_MASK,BUS_TYPE_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to get the Channel number of a Bus realisation configuration
*/
#define GET_CHAN_NUM(aBusId) GET_CONF_FIELD(aBusId,CHANNEL_NO_MASK,CHANNEL_NO_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to set the Channel number of a Bus realisation configuration
*/
#define SET_CHAN_NUM(aBusId,aChanNum) SET_CONF_FIELD(aBusId,aChanNum,CHANNEL_NO_MASK,CHANNEL_NO_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to set the Transaction speed of a Bus realisation configuration
*/
#define SET_TRANS_SPEED(aBusId,aTransSpeed) SET_CONF_FIELD(aBusId,aTransSpeed,TRANS_SPEED_MASK,TRANS_SPEED_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to get the Transaction speed of a Bus realisation configuration
*/
#define GET_TRANS_SPEED(aBusId) GET_CONF_FIELD(aBusId,TRANS_SPEED_MASK,TRANS_SPEED_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to set the Slave address of a Bus realisation configuration
*/
#define SET_SLAVE_ADDR(aBusId,aSlaveAddr) SET_CONF_FIELD(aBusId,aSlaveAddr,SLAVE_ADDR_MASK,SLAVE_ADDR_SHIFT)
/**
@publishedPartner
@prototype 9.6
Macro to get the Slave address of a Bus realisation configuration
*/
#define GET_SLAVE_ADDR(aBusId) GET_CONF_FIELD(aBusId,SLAVE_ADDR_MASK,SLAVE_ADDR_SHIFT)


// The SET_CONFIG_FIELD and GET_CONFIG_FIELD are for internal use, only.
// They are to support the TIicBusRealisation class
// These macros should be considered deprecated - they will be removed when the TIicBusRealisation class is removed
#define SET_CONFIG_FIELD(aField,aMask,aShift) iConfig=(iConfig&~(aMask<<aShift))|((aField&aMask)<<aShift)
#define GET_CONFIG_FIELD(aField,aMask,aShift) aField=((iConfig>>aShift)&aMask)

/**
@publishedPartner
@deprecated

Class to represent and provide access to configuration data used on a per-transaction basis with IIC

*/
class TIicBusRealisationConfig
	{
public:
	// default constructor - just resets the value..
	inline TIicBusRealisationConfig() : iConfig(0) {}

	inline TIicBusRealisationConfig& operator=(const TIicBusRealisationConfig &aObj){iConfig = aObj.GetConfig(); return *this;}

	inline void Clear() {iConfig = 0;}
	inline TUint32 GetConfig() const {return iConfig;}
	inline void SetConfig(TUint32 aConfig) {iConfig = aConfig;}

	inline void SetHsMasterAddr(TUint8 aHsMasterAddr) {SET_CONFIG_FIELD(aHsMasterAddr,HS_MASTER_ADDR_MASK,HS_MASTER_ADDR_SHIFT);};
	inline void GetHsMasterAddr(TUint8& aHsMasterAddr) const {{TUint32 tempVar;GET_CONFIG_FIELD(tempVar,HS_MASTER_ADDR_MASK,HS_MASTER_ADDR_SHIFT);aHsMasterAddr=(TUint8)tempVar;};};

	inline void SetHsValid(TUint8 aHsValid) {SET_CONFIG_FIELD(aHsValid,HS_ADDR_VALID_MASK,HS_ADDR_VALID_SHIFT);};
	inline void GetHsValid(TUint8& aHsValid) const{{TUint32 tempVar;GET_CONFIG_FIELD(tempVar,HS_ADDR_VALID_MASK,HS_ADDR_VALID_SHIFT);aHsValid=(TUint8)tempVar;};};

	inline void SetBusType(TUint8 aBusType) {SET_CONFIG_FIELD(aBusType,BUS_TYPE_MASK,BUS_TYPE_SHIFT);};
	inline void GetBusType(TUint8& aBusType) const {{TUint32 tempVar;GET_CONFIG_FIELD(tempVar,BUS_TYPE_MASK,BUS_TYPE_SHIFT);aBusType=(TUint8)tempVar;};};

	inline void SetChanNum(TUint8 aChanNum) {SET_CONFIG_FIELD(aChanNum,CHANNEL_NO_MASK,CHANNEL_NO_SHIFT);};
	inline void GetChanNum(TUint8& aChanNum) const {{TUint32 tempVar;GET_CONFIG_FIELD(tempVar,CHANNEL_NO_MASK,CHANNEL_NO_SHIFT);aChanNum=(TUint8)tempVar;};};

	inline void SetTransSpeed(TUint8 aTransSpeed) {SET_CONFIG_FIELD(aTransSpeed,TRANS_SPEED_MASK,TRANS_SPEED_SHIFT);};
	inline void GetTransSpeed(TUint8& aTransSpeed) const {{TUint32 tempVar;GET_CONFIG_FIELD(tempVar,TRANS_SPEED_MASK,TRANS_SPEED_SHIFT);aTransSpeed=(TUint8)tempVar;};};

	inline void SetSlaveAddr(TUint16 aSlaveAddr) {SET_CONFIG_FIELD(aSlaveAddr,SLAVE_ADDR_MASK,SLAVE_ADDR_SHIFT);};
	inline void GetSlaveAddr(TUint16& aSlaveAddr) const {{TUint32 tempVar;GET_CONFIG_FIELD(tempVar,SLAVE_ADDR_MASK,SLAVE_ADDR_SHIFT);aSlaveAddr=(TUint16)tempVar;};};

private:
	TUint32 iConfig;
	};


// Forward declarations
class TIicBusTransaction;
class DIicBusChannelMaster;

//
// Master-side items
//

/**
@publishedPartner
@prototype 9.6

Class to represent and provide access to configuration data used on a per-transaction basis with IIC

@see TIicBusTransaction

*/
class TIicBusTransfer
	{
public:
	enum TReqType
		{
		EMasterRead,
		EMasterWrite,
		};
	// the client interface for creating and linking simple requests
	// default constructor..
	inline TIicBusTransfer();
	inline TIicBusTransfer(TReqType aType, TInt8 aGranularity, TDes8* aBuffer);
	inline void LinkAfter(TIicBusTransfer* aPrev);
	inline TInt8 WordWidth();
	inline TReqType Direction();
	inline TInt Length();
	inline const TIicBusTransfer* Next();

	inline TInt SetTransferData(TReqType aType, TInt8 aGranularity, TDes8* aBuffer);
    const TDes8* GetBuffer() {return (const TDes8*)iBuffer;}

private:
	TInt8 iType;			// as one of TReqType
	TInt8 iBufGranularity;	// width of a transfer word in bits
	TInt8 iSpare1;
	TDes8* iBuffer;	// the data for this transfer (packed into 8-bit words with padding). Access must be guarded for safety.
	TIicBusTransfer* iNext;
	TIicBusTransaction* iTransaction;	// pointer to the TIicBusTransaction owning the list that this transfer has been added to

	friend class TIicBusTransaction;
	friend class DIicBusChannelMaster;
	};

const TInt8 KTransferQueued = 0x01;
/**
@publishedPartner
@prototype 9.6

Definition of function prototype for a callback function provided by a client
for invocation on the after the client transaction has been processed by a Master
channel. The function will be executed in the context of the client thread.

@see TIicBusTransaction

*/
typedef void (*TIicBusCbFn)(TIicBusTransaction* /*aTransction*/,
	                  TInt  /*aBusId*/,
	                  TInt  /*aResult*/,
	                  TAny* /*aParam*/);

class TIicBusCallback; // Forward declaration

/**
@publishedPartner
@prototype 9.6

Class to represent a transaction for processing by an IIC Master Channel

@see IicBus

*/
class TIicBusTransaction : public SOrdQueLink
	{
    public:
        enum TState {EFree,EDelivered,EAccepted};
    
        //default constuctor.
        inline TIicBusTransaction();
    
        // the client interface for creating half duplex transactions
        inline TIicBusTransaction(TDes8* aHeader, TIicBusTransfer* aHdTrans, TInt aPriority=0);
        inline ~TIicBusTransaction();
        inline TInt SetHalfDuplexTrans(TDes8* aHeader, TIicBusTransfer* aHdTrans);
    
        // The client interface for setting full duplex transaction: the API checks that it is possible to have the 2 transactions done in parallel.
        // It does not check if the channel supports full duplex, so the transaction may still fail at queuing time.
        inline TInt SetFullDuplexTrans(TIicBusTransfer* aFdTrans);
		inline TInt RemoveTrans(TIicBusTransfer* aTrans);
		// Client interface to explicitly disassociate transfer lists from the transaction, when the 
		// transaction is in state EFree.
		// These methods are for optional use. If the transfer list will not be further accessed,
		// then there is no need to explicitly disassociate it from a transaction. However, if
		// it will be accessed again its iTransaction pointer will refer to this object, so should
		// be cleared to prevent corruption - and this is the purpose of these methods.
		inline TInt RemoveHalfDuplexTrans();
		inline TInt RemoveFullDuplexTrans();

		// Accessor method to allow PSL access to iFlags to determine transaction type
		inline TUint Flags();
	protected:
		inline TIicBusTransaction(TDes8* aHeader, TIicBusTransfer* aHdTrans, TUint8 aFlags, TInt aPriority=0);
	public:
		inline TUint8 State();
		inline TInt GetBusId();
	private:
		TDes8* iHeader;
		TUint8 iFlags;	// used to indicate if it supports a preamble
		TUint8 iState;
		TInt8 iSpare1;
		TInt8 iSpare2;
#ifdef STANDALONE_CHANNEL
	//iBusId is needed by client when calling QueueTransaction
	public:
		TInt iBusId; 
#else
	private:
		TInt iBusId;
#endif
		TIicBusTransfer* iHalfDuplexTrans;
		TIicBusTransfer* iFullDuplexTrans;
		TIicBusCallback* iCallback;

		NFastSemaphore iSyncNotification;
		TInt iResult;
		friend class DIicBusChannelMaster;
#ifndef STANDALONE_CHANNEL
		friend class DIicBusController;
#endif
		};


/**
@publishedPartner
@prototype 9.6

Class to represent a callback object for use with asynchronous transactions processed by IIC Master channels

@see IicBus

*/
class TIicBusCallback : public TDfc
	{
public:
	inline TIicBusCallback(TIicBusCbFn aFn, TAny* aPtr, TDfcQue* aQue, TInt aPriority);
	inline ~TIicBusCallback();
private:
    inline static void DfcFunc(TAny* aPtr);
private:
	TIicBusTransaction* iTransaction;
	TInt iBusId;
	TInt iResult;	// the result of this transaction as a system wide error
	TAny* iParam;
	TIicBusCbFn iCallback;

	friend class DIicBusChannelMaster;
	};

// Forward declarations
class DIicBusChannelSlave;
class TIicBusSlaveCallback;

/**
@publishedPartner
@prototype 9.6

Definition of function prototype for a callback function provided by a client
for invocation when an asynchronous event (such as a notification, or completion
of the asynchronous capture of a channel) occurs for a Slave channel.

@see IicBus

*/
typedef void (*TIicBusSlaveCbFn)(TInt	/*aChannelId*/,
								 TInt	/*aReturn*/,
								 TInt   /*aTrigger*/,
								 TInt16 /*aRxWords*/,
								 TInt16 /*aTxWords*/,
								 TAny*	/*aParam*/);

/**
@publishedPartner
@prototype 9.6

A Slave callback notification object.
It wraps a DFC in client's thread which is queued by the channel generic implementation
when the transfer are completed asynchronously (e.g. in ISR).
Alternatively, if the entire transaction is processed synchronously in the client's thread,
the callback function will be called directly.

@see IicBus

*/
NONSHARABLE_CLASS(TIicBusSlaveCallback) : public TDfc
	{
public:
	inline TIicBusSlaveCallback(TIicBusSlaveCbFn aFn, TAny* aPtr, TDfcQue* aQue, TInt aPriority);
	inline void SetReturn(TInt aRet);	// to be used by PSL
	inline void SetTxWords(TInt16 aTxWords);	// to be used by PSL
	inline void SetRxWords(TInt16 aRxWords);	// to be used by PSL

	inline TInt GetTrigger();	// to be used by PSL
	inline void SetTrigger(TInt aTrigger);	// to be used by PSL

private:
	IMPORT_C static void DfcFunc(TAny* aPtr);

private:
	TInt iChannelId;
	TInt iReturn;		// a system wide error code
	TInt iTrigger;		// a bitmask containing the reason(s) why this callback was queued (see TIicBusSlaveTrigger)
	TInt16 iRxWords;	// number of words received
	TInt16 iTxWords;	// number of words transmitted
	TAny* iParam;
	DIicBusChannelSlave* iChannel;
	TIicBusSlaveCbFn iCallback;

	friend class DIicBusChannelSlave;
	};

/**
@publishedPartner
@prototype 9.6

 Prototype function pointer for a client supplied preamble:
The supplied function shall not:
	- spin
	- block or wait on a fast mutex
	- use any Kernel or base port services that do any of the above (e.g. alloc/free memory,
	  signal a DMutex, complete a request, access user side memory)

@see IicBus

*/
typedef void (*TIicBusPreamble)(TIicBusTransaction* /*aTransaction*/,
                                TAny*				/*aArg*/);

/**
@publishedPartner
@prototype 9.6

Transaction which support preamble

@see TIicBusTransaction

*/
class TIicBusTransactionPreamble : public TIicBusTransaction
	{
public:
	// the client interface for creating transactions that support a preamble
	inline TIicBusTransactionPreamble(TDes8* aHeader, TIicBusTransfer* aHdTrans, TIicBusPreamble aPreamble, TAny* aArg, TInt aPriority=0);
private:
	TIicBusPreamble iPreamble;	// preamble function pointer
	TAny* iPreambleArg;			// argument to be passed to preamble
protected:
	inline TIicBusTransactionPreamble(TDes8* aHeader, TIicBusTransfer* aHdTrans, TIicBusPreamble aPreamble, TAny* aArg, TUint8 aFlags, TInt aPriority=0);
	friend class DIicBusChannelMaster;
	};

/**
@publishedPartner
@prototype 9.6

Prototype function pointer for a client supplied multi transaction callback.
The function should return a pointer to a new transaction which will be send
within the same request. The function is called in the channel thread after
calling channel PSL DoRequest

@see TIicBusTransaction

*/
typedef TIicBusTransaction* (*TIicBusMultiTranscCbFn) (
								TIicBusTransaction* /*aTransaction*/,
								TAny*	/*aArg*/);

/**
@publishedPartner
@prototype 9.6

Transaction which support multi-transaction

@see TIicBusTransaction

*/
class TIicBusTransactionMultiTransc : public TIicBusTransaction
	{
public:
	inline TIicBusTransactionMultiTransc(TDes8* aHeader, TIicBusTransfer* aHdTrans, TIicBusMultiTranscCbFn aMultiTransc, TAny* aArg, TInt aPriority=0);
private:
	TIicBusMultiTranscCbFn iMultiTransc;
	TAny* iMultiTranscArg;
	friend class DIicBusChannelMaster;
	};

/**
@publishedPartner
@prototype 9.6

Transaction which support both preamble and multi-transaction

@see TIicBusTransactionPreamble

*/
class TIicBusTransactionPreambleExt : public TIicBusTransactionPreamble
	{
public:
	inline TIicBusTransactionPreambleExt(TDes8* aHeader, TIicBusTransfer* aHdTrans,
			TIicBusPreamble aPreamble, TAny* aPreambleArg,
			TIicBusMultiTranscCbFn aMultiTransc, TAny* aMultiTranscArg, TInt aPriority=0);
private:
	TIicBusMultiTranscCbFn iMultiTransc;
	TAny* iMultiTranscArg;
	friend class DIicBusChannelMaster;
	};


/**
@publishedPartner
@prototype 9.6

Enumeration of IIC Slave channel trigger values

@see IicBus
*/
enum TIicBusSlaveTrigger
	{
	ERxAllBytes			= 0x01,    // Master has written the required number of bytes
	ERxUnderrun			= 0x02,    // Master has written less than the required number of bytes, and ceased transmitting
	ERxOverrun			= 0x04,    // Master has written the required number of bytes and is continuing to transmit
	ETxAllBytes			= 0x08,    // Master has read the required number of bytes
	ETxUnderrun			= 0x10,    // Master has read the required number of bytes and is continuing to read
	ETxOverrun			= 0x20,    // Master has read less than the required number of bytes, and ceased reading
	EGeneralBusError	= 0x40,    // An error has occurred during a transaction
	EAsyncCaptChan		= 0x80     // Completion of asynchronous CaptureChannel
	};

#include <drivers/iic_transaction.inl>

#endif // __IIC_TRANSACTION_H__

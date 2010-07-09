// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\iic\t_iic.h
//

#ifndef __T_IIC_H__
#define __T_IIC_H__

#include <e32ver.h>

const TInt KIicClientMajorVersionNumber = 1;
const TInt KIicClientMinorVersionNumber = 0;
const TInt KIicClientBuildVersionNumber = KE32BuildVersionNumber;

const TInt KPriorityTestNum = 6; // 1 blocking transaction + 5 test transactions

// For IIC,
// If bit 31 is set and bit 30 cleared it is used to extend the Master-Slave channel;
// if bit 31 is cleared and bit 30 is set, it extends the Master channel; 
// if both bits 31 and 30 are cleared it extends the Slave channel interface.
// However,
// since the kernel-side proxy clients interpret the msb being set as indicative of an
// asynchronous request, the values here will have the static extension pattern represented
// in bits 30 and 29, instead. In addition, to support communication with the slave-side proxy,
// the Slave extension value will be represented as bits 30 and 29 set, so that it can be distinguished
// from 'normal' synchronous operations.
//
const TUint KTestControlIoMask =			0x60000000;
const TUint KTestMasterControlIo =		0x20000000;
const TUint KTestSlaveControlIo =		0x60000000;
const TUint KTestMasterSlaveControlIo =	0x40000000;
const TUint KTestControlIoPilOffset =	0x00000002;	// Corresponds to 1 higher than the number used by PIL
const TUint KTestControlUnitTestOffset = 0x10000000;

//
// Enumerations TReqType and TBusType defined in kernel-side class TIicBusTransfer
// The user-side test, and the kernel-side proxy client require access to this
enum TReqType
	{
	EMasterRead,
	EMasterWrite
	};
enum TBusType
	{
	EI2c	   = 0,
	ESpi	   = 0x01,
	EMicrowire = 0x02,
	ECci	   = 0x03,
	ESccb	   = 0x04,
	EInvalidBus
	};

#define MAX_TRANS_LENGTH 20	


#ifndef __KERNEL_MODE__
//
//	For convenience, selected kernel-side information is replicated here
//  to allow the user-side test to populate buffers accordingly
//
// Bus-specific configuration
//
enum TEndianness
	{
	EBigEndian,
	ELittleEndian
	};

enum TBitOrder
	{
	ELsbFirst,
	EMsbFirst
	};

//
// Bus-specific configuration for SPI bus
//

enum TSpiWordWidth
	{
	ESpiWordWidth_8,
	ESpiWordWidth_10,
	ESpiWordWidth_12,
	ESpiWordWidth_16
	};

enum TSpiClkMode
	{
	ESpiPolarityLowRisingEdge,		// Active high, odd edges
	ESpiPolarityLowFallingEdge,		// Active high, even edges
	ESpiPolarityHighFallingEdge,	// Active low,  odd edges
	ESpiPolarityHighRisingEdge		// Active low,  even edges
	};

enum TSpiSsPinMode
    {
    ESpiCSPinActiveLow,      // Active low
    ESpiCSPinActiveHigh     // Active high
    };

class TConfigSpiV01
	{
public:
	TSpiWordWidth	iWordWidth;
	TInt32			iClkSpeedHz;
	TSpiClkMode		iClkMode;
	TInt32			iTimeoutPeriod;
	TEndianness		iEndianness;
	TBitOrder		iBitOrder;
	TUint			iTransactionWaitCycles;
	TSpiSsPinMode	iSSPinActiveMode;
	};

typedef TPckgBuf <TConfigSpiV01> TConfigSpiBufV01;


//
// Bus-specific configuration for I2C bus
//

enum TI2cAddrType
	{
	EI2cAddr7Bit,
	EI2cAddr10Bit
	};

class TConfigI2cV01
	{
	public:
	TI2cAddrType	iAddrType;		// 7 or 10-bit addressing
	TInt32			iClkSpeedHz;
	TEndianness		iEndianness;
	TInt32			iTimeoutPeriod;
	};

typedef TPckgBuf <TConfigI2cV01> TConfigI2cBufV01;


inline static TInt CreateSpiBuf(TConfigSpiBufV01*& aBuf,
								TSpiWordWidth	aWordWidth,
								TInt32			aClkSpeedHz,
								TSpiClkMode		aClkMode,
								TInt32			aTimeoutPeriod,
								TEndianness		aEndianness,
								TBitOrder		aBitOrder,
								TUint			aTransactionWaitCycles,
								TSpiSsPinMode	aSSPinActiveMode)
// Utility function to create a buffer for the SPI bus
	{
	aBuf = new TConfigSpiBufV01();
	if(aBuf==NULL)
		return KErrNoMemory;
	TConfigSpiV01 *buf = &((*aBuf)());
	buf->iWordWidth = aWordWidth;
	buf->iClkSpeedHz = aClkSpeedHz;
	buf->iClkMode = aClkMode;
	buf->iTimeoutPeriod = aTimeoutPeriod;
	buf->iEndianness = aEndianness;
	buf->iBitOrder = aBitOrder;
	buf->iTransactionWaitCycles = aTransactionWaitCycles;
	buf->iSSPinActiveMode = aSSPinActiveMode;
	return KErrNone;
	}

inline static TInt CreateI2cBuf(TConfigI2cBufV01*& aBuf,
								TI2cAddrType	aAddrType,
								TInt32			aClkSpeedHz,
								TEndianness		aEndianness,
								TInt32			aTimeoutPeriod)
// Utility function to create a buffer for the I2C bus
	{
	aBuf = new TConfigI2cBufV01();
	if(aBuf==NULL)
		return KErrNoMemory;
	TConfigI2cV01 *buf = &((*aBuf)());
	buf->iAddrType = aAddrType;
	buf->iClkSpeedHz = aClkSpeedHz;
	buf->iEndianness = aEndianness;
	buf->iTimeoutPeriod = aTimeoutPeriod;
	return KErrNone;
	}

//
// Enumerations for channel type and channel duplex defined in kernel-side class DIicBusChannel
// duplicated for temporary test
enum TChannelType
	{
	EMaster			= 0,
	ESlave			= 0x01,
	EMasterSlave	= 0x02,
	EInvalidType
	};
enum TChannelDuplex
	{
	EHalfDuplex = 0,	// supports only half duplex transactions (even if bus spec supports full duplex)
	EFullDuplex = 0x1,	// supports full duplex transactions (queud transactions may still be half duplex)
	EInvalidDuplex
	};
//
// Bus realisation configuration
//
// 31 30 29 28 | 27 26 25 24 | 23 22 21 20 | 19 18 17 16 | 15 14 13 12 | 11 10  9  8 |  7  6  5  4 |  3  2  1  0
//
// 31:29 - HS Master address (I2C only)
// 28    - HS address valid bit
// 27:23 - Reserved
// 22:20 - Bus type
// 19:15 - Channel number
// 14:10 - Transaction speed	// deprecated
//  9:0  - Slave address
#define HS_MASTER_ADDR_SHIFT 29
#define HS_MASTER_ADDR_MASK 0x7
#define HS_ADDR_VALID_SHIFT 28
#define HS_ADDR_VALID_MASK 0x1
#define BUS_TYPE_SHIFT 20
#define BUS_TYPE_MASK 0x7
#define CHANNEL_NO_SHIFT 15
#define CHANNEL_NO_MASK 0x1F
//#define TRANS_SPEED_SHIFT 10
//#define TRANS_SPEED_MASK 0x1F
#define SLAVE_ADDR_SHIFT 0
#define SLAVE_ADDR_MASK 0x3FF
//
// Macros to access fields within Bus Realisation Configuration data, used on a per-transaction basis with IIC
#define SET_CONFIG_FIELD(aBusId,aField,aMask,aShift) aBusId=(aBusId&~(aMask<<aShift))|((aField&aMask)<<aShift);
#define GET_CONFIG_FIELD(aBusId,aMask,aShift) (((aBusId)>>(aShift))&(aMask))

#define GET_HS_MASTER_ADDR(aBusId) GET_CONFIG_FIELD(aBusId,HS_MASTER_ADDR_MASK,HS_MASTER_ADDR_SHIFT)
#define SET_HS_MASTER_ADDR(aBusId,aHsMasterAddr) SET_CONFIG_FIELD(aBusId,aHsMasterAddr,HS_MASTER_ADDR_MASK,HS_MASTER_ADDR_SHIFT)
#define GET_HS_VALID(aBusId) GET_CONFIG_FIELD(aBusId,HS_ADDR_VALID_MASK,HS_ADDR_VALID_SHIFT)
#define SET_HS_VALID(aBusId,aHsValid) SET_CONFIG_FIELD(aBusId,aHsValid,HS_ADDR_VALID_MASK,HS_ADDR_VALID_SHIFT)
#define GET_BUS_TYPE(aBusId) GET_CONFIG_FIELD(aBusId,BUS_TYPE_MASK,BUS_TYPE_SHIFT)
#define SET_BUS_TYPE(aBusId,aBusType) SET_CONFIG_FIELD(aBusId,aBusType,BUS_TYPE_MASK,BUS_TYPE_SHIFT)
#define GET_CHAN_NUM(aBusId) GET_CONFIG_FIELD(aBusId,CHANNEL_NO_MASK,CHANNEL_NO_SHIFT)
#define SET_CHAN_NUM(aBusId,aChanNum) SET_CONFIG_FIELD(aBusId,aChanNum,CHANNEL_NO_MASK,CHANNEL_NO_SHIFT)
//#define SET_TRANS_SPEED(aBusId,aTransSpeed) SET_CONFIG_FIELD(aBusId,aTransSpeed,TRANS_SPEED_MASK,TRANS_SPEED_SHIFT)
//#define GET_TRANS_SPEED(aBusId) GET_CONFIG_FIELD(aBusId,TRANS_SPEED_MASK,TRANS_SPEED_SHIFT)
#define SET_SLAVE_ADDR(aBusId,aSlaveAddr) SET_CONFIG_FIELD(aBusId,aSlaveAddr,SLAVE_ADDR_MASK,SLAVE_ADDR_SHIFT)
#define GET_SLAVE_ADDR(aBusId) GET_CONFIG_FIELD(aBusId,SLAVE_ADDR_MASK,SLAVE_ADDR_SHIFT)

static const TUint8 KTransactionWithPreamble = 0x80;
static const TUint8 KTransactionWithMultiTransc = 0x40;

enum TIicBusSlaveTrigger
	{
	ERxAllBytes			= 0x01,
	ERxUnderrun			= 0x02,
	ERxOverrun			= 0x04,
	ETxAllBytes			= 0x08,
	ETxUnderrun			= 0x10,
	ETxOverrun			= 0x20,
	EGeneralBusError	= 0x40,
	EAsyncCaptChan		= 0x80
	};

#endif // #ifndef __KERNEL_MODE__

//
// User-Side abbreviation of kernel side classes TIicBusTransfer and TIicBusTransaction
//
struct TUsideTferDesc
	{
	TInt8 iType;			// as one of TReqType
	TInt8 iBufGranularity;	// width of a transfer word in bits
	TDes8* iBuffer;	// the data for this transfer (packed into 8-bit words with padding)
	TUsideTferDesc* iNext;
	};

struct TUsideTracnDesc
	{
	TBusType iType;
	TDes8* iHeader;
	TUsideTferDesc* iHalfDuplexTrans;
	TUsideTferDesc* iFullDuplexTrans;
	TUint8 iFlags;				// used to indicate if it supports a preamble
	TAny* iPreambleArg;			// used for preamble argument
	TAny* iMultiTranscArg;		// used for multi transc argument
	};

class RBusDevIicClient : public RBusLogicalChannel
	{
    public:
	enum TControl
		{
// Master mode operations
		EQTransSync=1,						/**< Queue Transaction (Synchronous version)					*/
// Slave mode operations
		EInitSlaveClient,					/**< Instigate Slave initialisation required to support testing	*/
		ECaptureChanSync,					/**< Capture Channel (Synchronous version)						*/
		EReleaseChan,						/**< ReleaseChannel												*/
		ERegisterRxBuffer,					/**< Register a buffer for receiving data						*/
		ERegisterTxBuffer,					/**< Register a buffer for transmitting data					*/
		ESetNotifTrigger,					/**< Set the notification triggers                               */
		ETestSpare1,
		ETestStaticEx
		};

	enum TStaticExt
		{
		ECtlIoNone = 0,
		ECtlIoDumpChan = 1, // KCtrlIoDumpChan - defined only for UDEB
// ControlIO codes for Master follow
		ECtlIoBlockReqCompletion=(KTestMasterControlIo+KTestControlIoPilOffset),
		ECtlIoUnblockReqCompletion,
		ECtlIoDeRegChan,
		ECtlIoTracnOne,
		ECtlIoPriorityTest,
		EGetTestResult,
		ECtlIoSetTimeOutFlag,
		ECtlIoTestFullDuplexTrans,
// ControlIO codes for Slave follow
		ECtrlIoRxWords=(KTestSlaveControlIo+KTestControlIoPilOffset),
		ECtrlIoTxWords,
		ECtrlIoRxTxWords,
		ECtrlIoTxChkBuf,
		ECtlIoBusError,
		ECtrlIoBlockNotification,
		ECtrlIoUnblockNotification,
		ECtrlIoUpdTimeout,
		ECtrlIoNotifNoTrigger
		};
	
	enum TTestFullDuplexTrans
		{
		ETestValidFullDuplexTrans=1,
		ETestInvalidFullDuplexTrans1,
		ETestInvalidFullDuplexTrans2,
		ETestLastNodeFullDuplexTrans,
		ETestDiffNodeNoFullDuplexTrans,
		ETestNone
		};

	enum TRequest
		{
// Master mode operations
		EQTransAsync=1,						/**< Queue Transaction (Asynchronous version)					*/
		ECtrlIoTestBufReUse,
// Slave mode operations
		ECaptureChanAsync,					/**< Capture Channel (Asynchronous version)						*/
		ECtrlIoOvUndRunRxTx
		};
		
	enum TTestMessages
	    {
	    ETestIicChannelInlineFunc=KTestControlUnitTestOffset    
	    };
	
#ifndef __KERNEL_MODE__
	public:   
	inline TInt TestIiicChannelInlineFunc(){return DoControl (ETestIicChannelInlineFunc, NULL, NULL);}	
	// Master mode functions
	inline TInt Open(TDesC& aProxyName) {return (DoCreate(aProxyName,TVersion(KIicClientMajorVersionNumber,KIicClientMinorVersionNumber,KIicClientBuildVersionNumber),-1,NULL,NULL,EOwnerThread));}

	inline TInt QueueTransaction(TInt aBusId, TUsideTracnDesc* aTransaction) {return(DoControl(EQTransSync,(TAny*)aBusId,(TAny*)aTransaction));}

	inline void QueueTransaction(TRequestStatus& aStatus, TInt aBusId, TUsideTracnDesc* aTransaction) {DoRequest(EQTransAsync,aStatus,(TAny*)aBusId,(TAny*)aTransaction);}

	inline void CancelAsyncOperation(TRequestStatus* aStatus, TInt aBusId)	{TInt* parms[2]; parms[0]=(TInt*)aStatus; parms[1]=(TInt*)aBusId;DoCancel((TInt)&parms[0]);} 

	// Slave mode functions
	inline TInt InitSlaveClient() {return(DoControl(EInitSlaveClient,NULL,NULL));}
	inline TInt CaptureChannel(TInt aBusId, TDes8* aConfigHdr, TInt& aChannelId) {TInt* parms[2]; parms[0]=(TInt*)aBusId; parms[1]=&aChannelId;return(DoControl(ECaptureChanSync,(TAny*)aConfigHdr,(TAny*)(&parms[0])));}

	inline TInt CaptureChannel(TInt aBusId, TDes8* aConfigHdr, TInt& aChannelId, TRequestStatus& aStatus) {TInt* parms[2]; parms[0]=(TInt*)aBusId; parms[1]=&aChannelId;DoRequest(ECaptureChanAsync,aStatus,(TAny*)aConfigHdr,(TAny*)(&parms[0]));return KErrNone;}

	inline TInt ReleaseChannel(TInt aChannelId){return(DoControl(EReleaseChan,(TAny*)aChannelId,NULL));};
	inline TInt RegisterRxBuffer(TInt aChannelId, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset){TInt8 parms[3]; parms[0]=aBufGranularity; parms[1]=aNumWords; parms[2]=aOffset;return(DoControl(ERegisterRxBuffer,(TAny*)aChannelId,(TAny*)(&parms[0])));};
	inline TInt RegisterTxBuffer(TInt aChannelId, TInt8 aBufGranularity, TInt8 aNumWords, TInt8 aOffset){TInt8 parms[3]; parms[0]=aBufGranularity; parms[1]=aNumWords; parms[2]=aOffset;return(DoControl(ERegisterTxBuffer,(TAny*)aChannelId,(TAny*)(&parms[0])));};
	inline TInt SetNotificationTrigger(TInt aChannelId, TInt aTrigger, TRequestStatus* aStatus){TInt parms[2]; parms[0]=aChannelId; parms[1]=aTrigger; *aStatus=KRequestPending; return(DoControl(ESetNotifTrigger,(TAny*)aStatus,(TAny*)(&parms[0])));};

	// ControlIO functions follow
	inline TInt BlockReqCompletion(TInt aBusId) {return(DoControl(ECtlIoBlockReqCompletion,(TAny*)aBusId));}
	inline TInt UnblockReqCompletion(TInt aBusId) {return(DoControl(ECtlIoUnblockReqCompletion,(TAny*)aBusId));}
	inline TInt DeRegisterChan(TInt aBusId) {return(DoControl(ECtlIoDeRegChan,(TAny*)aBusId));}
	inline TInt TestTracnOne(TInt aBusId) {return(DoControl(ECtlIoTracnOne, (TAny*)aBusId));}
	inline TInt SetTimeOutFlag(TInt aBusId){return(DoControl(ECtlIoSetTimeOutFlag,(TAny*)aBusId));}
	inline TInt CancelTimeOutFlag(TInt aBusId){return(DoControl(ECtlIoNone,(TAny*)aBusId));}
	inline TInt TestPriority(TInt aBusId) {return(DoControl(ECtlIoPriorityTest, (TAny*)aBusId));}

	inline TInt TestValidFullDuplexTrans(TInt aBusId) {return(DoControl(ECtlIoTestFullDuplexTrans, (TAny*)aBusId, (TAny*)ETestValidFullDuplexTrans));}
	inline TInt TestInvalidFullDuplexTrans1(TInt aBusId) {return(DoControl(ECtlIoTestFullDuplexTrans, (TAny*)aBusId, (TAny*)ETestInvalidFullDuplexTrans1));}
	inline TInt TestInvalidFullDuplexTrans2(TInt aBusId) {return(DoControl(ECtlIoTestFullDuplexTrans, (TAny*)aBusId, (TAny*)ETestInvalidFullDuplexTrans2));}
	
	inline TInt TestLastNodeFullDuplexTrans(TInt aBusId) {return(DoControl(ECtlIoTestFullDuplexTrans, (TAny*)aBusId, (TAny*)ETestLastNodeFullDuplexTrans));}
	inline TInt TestDiffNodeNumFullDuplexTrans(TInt aBusId) {return(DoControl(ECtlIoTestFullDuplexTrans, (TAny*)aBusId, (TAny*)ETestDiffNodeNoFullDuplexTrans));}

	inline void TestBufferReUse(TInt aBusId, TRequestStatus& aStatus) {DoRequest(ECtrlIoTestBufReUse,aStatus,(TAny*)aBusId,NULL);}

	inline TInt SimulateRxNWords(TInt aBusId, TInt aChannelId, TInt aNumWords){TInt parms[2]; parms[0]=aChannelId; parms[1]=aNumWords;return(DoControl(ECtrlIoRxWords,(TAny*)aBusId,(TAny*)(&parms[0])));};
	inline TInt SimulateTxNWords(TInt aBusId, TInt aChannelId, TInt aNumWords){TInt parms[2]; parms[0]=aChannelId; parms[1]=aNumWords;return(DoControl(ECtrlIoTxWords,(TAny*)aBusId,(TAny*)(&parms[0])));};
	inline TInt SimulateRxTxNWords(TInt aBusId, TInt aChannelId, TInt aNumRxWords, TInt aNumTxWords){TInt parms[3]; parms[0]=aChannelId; parms[1]=aNumRxWords; parms[2]=aNumTxWords;return(DoControl(ECtrlIoRxTxWords,(TAny*)aBusId,(TAny*)(&parms[0])));};
	inline TInt SimulateBusErr(TInt aBusId, TInt aChannelId) {return(DoControl(ECtlIoBusError,(TAny*)aBusId,(TAny*)aChannelId));}
	inline TInt BlockNotification(TInt aBusId, TInt aChannelId) {return(DoControl(ECtrlIoBlockNotification,(TAny*)aBusId,(TAny*)aChannelId));}
	inline TInt UnblockNotification(TInt aBusId, TInt aChannelId) {return(DoControl(ECtrlIoUnblockNotification,(TAny*)aBusId,(TAny*)aChannelId));}
	inline TInt UpdateTimeoutValues(TInt aBusId, TInt aChannelId) {return(DoControl(ECtrlIoUpdTimeout,(TAny*)aBusId,(TAny*)aChannelId));}
	inline TInt SetNotifNoTrigger(TInt aChannelId, TInt aTrigger){return(DoControl(ECtrlIoNotifNoTrigger,(TAny*)aChannelId,(TAny*)aTrigger));};

	inline void TestOverrunUnderrun(TInt aBusId, TInt aChannelId, TRequestStatus& aStatus) {DoRequest(ECtrlIoOvUndRunRxTx,aStatus,(TAny*)aBusId,(TAny*)aChannelId);}
	
	inline TInt TestSpare1(TInt aBusId) {return (DoControl(ETestSpare1, (TAny*)aBusId));}
	inline TInt TestStaticExtension(TInt aBusId) {return (DoControl(ETestStaticEx, (TAny*)aBusId));}
#endif
	};


#ifdef __KERNEL_MODE__

// Definition of function prototype for a callback function provided by the PSL
// to be invoked when the part played by the hardware in processing a transfer
// has completed.
typedef void (*THwDoneCbFn)(TAny* );

#endif

// Data used to support tests

// Transaction One
//
const TUint8 KTransOneTferOne[21] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
const TUint8 KTransOneTferTwo[8] = {17,18,19,20,21,22,23,24};
const TUint8 KTransOneTferThree[6] = {87,85,83,81,79,77};
const TUint8 KPriorityTestHeader[6] = {0,1,2,3,4,10}; 
const TInt KPriorityTestPrio[6] = {1,2,3,4,5,0};

const TInt KRxBufSizeInBytes = 64;
const TInt KTxBufSizeInBytes = 64;

#endif

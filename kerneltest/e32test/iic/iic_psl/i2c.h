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
// e32test/iic/iic_psl/i2c.h
//

#ifndef I2C_H_
#define I2C_H_

#include <drivers/iic_channel.h>
#include "../t_iic.h"

#define NUM_CHANNELS 3 // Arbitrary

#ifdef LOG_I2C
#define I2C_PRINT(str) Kern::Printf str
#else
#define I2C_PRINT(str)
#endif

_LIT(KI2cThreadName,"I2cChannelThread");

#ifndef STANDALONE_CHANNEL
#if defined(MASTER_MODE)
const TInt8 KI2cChannelNumBase = 10;	// Arbitrary, real platform may consult the Configuration Repository
										// Note limit of 5 bit representation (0-31)

#else/*MASTER_MODE*/
const TInt8 KI2cChannelNumBase = 10 + NUM_CHANNELS;	// For Slave mode, want to provide different response
													// If client assumes Master mode, should be informed not available
#endif/*MASTER_MODE*/
#endif/*STANDALONE_CHANNEL*/


const TInt KI2cThreadPriority = 5; // Arbitrary, can be 0-7, 7 highest

const TInt16 KI2cSlaveChannelIdBase = 0x1D00;	// Arbitrary

const TInt KI2cSlaveAsyncDelaySim = 20;	// Arbitrary delay, for timer to simulate asynchronous processing

//Macros MASTER_MODE and SLAVE_MODE are intentionally omitted from this file
//This is for master and slave stubs to exercise the channel class,
//and we need these stubs for code coverage tests.
class DSimulatedIicBusChannelMasterI2c : public DIicBusChannelMaster
	{
	// platform specific implementation
	public:
#ifdef STANDALONE_CHANNEL
	IMPORT_C
#endif
	DSimulatedIicBusChannelMasterI2c(const TBusType aBusType, const TChannelDuplex aChanDuplex);
	inline TInt Create() {return DoCreate();}
	TInt DoCreate();


private:
	TInt CheckHdr(TDes8* aHdr);	// Check the header is valid for this channel
	TInt StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2);	
public:
	// gateway function for PSL implementation
	TInt DoRequest(TIicBusTransaction* aTransaction);
	TInt HandleSlaveTimeout();

	// Test functions to allow simulating delayed processing of requests
	static TBool IsRequestDelayed(DSimulatedIicBusChannelMasterI2c* aChan);
	static void SetRequestDelayed(DSimulatedIicBusChannelMasterI2c* aChan,TBool aDelay);

	TInt ProcessTrans(); // Accessed by callback
	inline TInt8 GetChanNum() {return iChannelNumber;};
private:
	TDynamicDfcQue*	iDynamicDfcQ;

	static TInt8 iCurrentChanNum;

	TBool iReqDelayed;
	};
#ifndef STANDALONE_CHANNEL
TInt8 DSimulatedIicBusChannelMasterI2c::iCurrentChanNum = KI2cChannelNumBase; // Initialise static member of DSimulatedIicBusChannelMasterI2c
#endif

class DSimulatedIicBusChannelSlaveI2c : public DIicBusChannelSlave
	{
public:
	// platform specific implementation
#ifdef STANDALONE_CHANNEL
	IMPORT_C
#endif
	DSimulatedIicBusChannelSlaveI2c(const DIicBusChannel::TBusType aBusType, const DIicBusChannel::TChannelDuplex aChanDuplex);
	~DSimulatedIicBusChannelSlaveI2c();
	// gateway function for PSL implementation
	TInt DoRequest(TInt aTrigger);
	void ProcessData(TInt aTrigger, TIicBusSlaveCallback*  aCb);
	TInt StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2);	

	inline TInt Create() {return DoCreate();}
	virtual TInt DoCreate();

	static void SlaveAsyncSimCallback(TAny* aPtr);
	inline void ChanCaptureCb(TInt aResult) {ChanCaptureCallback(aResult);}

	inline void SetChanNum(TInt8 aChanNum) {iChannelNumber = aChanNum;};

	enum TAsyncEvent
		{
		ENoEvent = 0,
		EAsyncChanCapture,
		ERxWords,
		ETxWords,
		ERxTxWords
		};
	inline void ChanNotifyClient(TInt aTrigger) {NotifyClient(aTrigger);}

	protected:
		virtual void SendBusErrorAndReturn() {return;} // Not implemented in simulated PSL


	private:
		TInt CheckHdr(TDes8* aHdr);	// Check the header is valid for this channel
		virtual TInt CaptureChannelPsl(TBool aAsynch);
		virtual TInt ReleaseChannelPsl();
		TInt PrepareTrigger(TInt aTrigger);
	private:

		TInt8 iDeltaWordsToRx;
		TInt8 iDeltaWordsToTx;
		TInt8 iNumWordsWereRx;
		TInt8 iNumWordsWereTx;
		TInt8 iRxTxUnderOverRun;

		TInt8* iTxCheckBuf;

		TInt iBlockedTrigger;
		TBool iBlockNotification;

		TAsyncEvent iAsyncEvent;
		TInt iRxTxTrigger;

		NTimer iSlaveTimer; // Used to simulate an asynchronous capture operation
		TSpinLock iEventSpinLock; // To serialise simulated bus events - Rx, Tx or Rx+Tx
		};

class DSimulatedIicBusChannelMasterSlaveI2c : public DIicBusChannelMasterSlave
	{
public:
#ifdef STANDALONE_CHANNEL
	IMPORT_C
#endif
	DSimulatedIicBusChannelMasterSlaveI2c(TBusType /*aBusType*/, TChannelDuplex aChanDuplex, DSimulatedIicBusChannelMasterI2c* aMasterChan, DSimulatedIicBusChannelSlaveI2c* aSlaveChan);
				
	TInt StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2);	
	};

#endif /*I2C_H_*/

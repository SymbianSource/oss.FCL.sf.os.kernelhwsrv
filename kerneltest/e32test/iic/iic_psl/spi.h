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
// e32test/iic/iic_psl/spi.h
//

#ifndef SPI_H_
#define SPI_H_

#include <drivers/iic_channel.h>
#include "../t_iic.h"

#ifdef LOG_SPI
#define SPI_PRINT(str) Kern::Printf str
#else
#define SPI_PRINT(str)
#endif

#ifndef STANDALONE_CHANNEL
const TInt8 KSpiChannelNumBase = 1;	// Arbitrary, real platform may consult the Configuration Repository
									// Note limit of 5 bit representation (0-31)
#endif

class THwDoneCallBack : public TDfc
	{
public:
	inline THwDoneCallBack(THwDoneCbFn aFn, TAny* aPtr, TDfcQue* aQue, TInt aPriority) : TDfc(DfcFn, this, aQue, aPriority),iCbFn(aFn),iParam(aPtr) {}
	inline ~THwDoneCallBack(){}
	
private:
	inline static void DfcFn(TAny* aPtr)
		{
		THwDoneCallBack* pCb = (THwDoneCallBack*) aPtr;
		pCb -> iCbFn(pCb->iParam);
		}
	
private:
	THwDoneCbFn iCbFn;
	TAny* iParam;
	};

class DSimulatedIicBusChannelMasterSpi : public DIicBusChannelMaster
	{
	// platform specific implementation

	enum TTestState
		{
		ETestNone=0,
		ETestWaitPriorityTest,
		ETestWaitTransOne,
		ETestSlaveTimeOut
		};
	
	enum TMyState
		{
		EIdle,
		EBusy
		};
	
	enum TIsrCause
		{
		EHwTransferDone,
		ETimeExpired
		};
	
public:
#ifdef STANDALONE_CHANNEL
	IMPORT_C
#endif
	DSimulatedIicBusChannelMasterSpi(const TBusType aBusType, const TChannelDuplex aChanDuplex);
	~DSimulatedIicBusChannelMasterSpi(){iDynamicDfcQ->Destroy();};
	inline TInt Create() {return DoCreate();}
	TInt DoCreate();
	TInt ReleaseChanArray();
	void CompleteReq(TInt aResult);

private:
	TInt CheckHdr(TDes8* aHdr);	// Check the header is valid for this channel
	virtual TInt StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2);	

	TInt CompareTransactionOne(TIicBusTransaction* aTransaction);

public:
	// gateway function for PSL implementation
	TInt DoRequest(TIicBusTransaction* aTransaction);
	TInt HandleSlaveTimeout();
	// Test functions to allow simulating delayed processing of requests
	static TBool IsRequestDelayed(DSimulatedIicBusChannelMasterSpi* aChan);
	static void SetRequestDelayed(DSimulatedIicBusChannelMasterSpi* aChan,TBool aDelay);

	TInt ProcessTrans(TIicBusTransaction* aTransaction); // Accessed by callback
	TInt AsynchStateMachine(TInt aReason);
	TInt DoSimulatedTransaction();
	TInt DoHwPreparation();
	static void TransactionTimerCallBack(TAny*);
	
	inline TInt8 GetChanNum() {return iChannelNumber;};
private:
	TDynamicDfcQue*	iDynamicDfcQ;	// Use TDynamicDfcQue since will want to DeRegister channels
	
	TIicBusTransaction* iCurrTrans;
	
	static TInt8 iCurrentChanNum;

	TInt8 iTestState;
	TInt8 iChannelState;
	
	TBool iReqDelayed;
	THwDoneCallBack *iCb;
	TInt iPriorityTestResult[KPriorityTestNum];
	TBool iPriorityTestDone;
	};
#ifndef STANDALONE_CHANNEL
TInt8 DSimulatedIicBusChannelMasterSpi::iCurrentChanNum = KSpiChannelNumBase; // Initialise static member of DSimulatedIicBusChannelMasterSpi
#endif
class DSimulatedIicBusChannelSlaveSpi : public DIicBusChannelSlave
	{
public:
	// platform specific implementation
#ifdef STANDALONE_CHANNEL
	IMPORT_C
#endif
	DSimulatedIicBusChannelSlaveSpi(const DIicBusChannel::TBusType aBusType, const DIicBusChannel::TChannelDuplex aChanDuplex);
	inline TInt Create() {return DoCreate();}
	TInt DoCreate();
	// gateway function for PSL implementation
	TInt DoRequest(TInt aTrigger);
	void ProcessData(TInt aTrigger, TIicBusSlaveCallback*  aCb);
	virtual TInt StaticExtension(TUint aFunction, TAny* aParam1, TAny* aParam2);	
	static void SlaveAsyncSimCallback(TAny* aPtr);
	inline void ChanCaptureCb(TInt aResult) {ChanCaptureCallback(aResult);}
	inline void SetChanNum(TInt8 aChanNum) {iChannelNumber = aChanNum;};
protected:
	virtual void SendBusErrorAndReturn() {return;} // Not implemented in simulated PSL

	private:
	TInt CheckHdr(TDes8* aHdr);	// Check the header is valid for this channel
	virtual TInt CaptureChannelPsl(TDes8* aConfigHdr, TBool aAsynch);

private:
	NTimer iSlaveTimer;
	};



#endif /*SPI_H_*/

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
// e32\include\drivers\comm.h
// 
//

/**
 @file
 @internalComponent
*/

#ifndef __M32COMM_H__
#define __M32COMM_H__
#include <platform.h>
#include <kernel/kpower.h>
#include <d32comm.h>
#include <e32ver.h>
//
const TInt KCommsMajorVersionNumber=1;
const TInt KCommsMinorVersionNumber=1;
const TInt KCommsBuildVersionNumber=KE32BuildVersionNumber;
//
const TInt KDefaultRxBufferSize=0x800;
const TInt KTxBufferSize=0x400;
const TInt KMaxHighWaterMark=0x080;
//
/**
	@publishedPartner
	@released
*/
const TUint KReceiveIsrParityError=0x10000000;

/**
	@publishedPartner
	@released
*/
const TUint KReceiveIsrFrameError=0x20000000;

/**
	@publishedPartner
	@released
*/
const TUint KReceiveIsrOverrunError=0x40000000;

/**
	@publishedPartner
	@released
*/
const TUint KReceiveIsrBreakError=0x80000000;

const TUint KReceiveIsrMaskError=0xF0000000;
//
const TInt KTxNoChar=-1;
//
const TUint KReceiveIsrTermChar=0x80000000;
const TUint KReceiveIsrMaskComplete=0xf0000000;
const TUint KReceiveIsrShift=24;
const TUint KReceiveIsrShiftedMask=0x0f;

/**
	@publishedPartner
	@released
*/
const TUint KDTEInputSignals=(KSignalCTS|KSignalDSR|KSignalDCD|KSignalRNG);

/**
	@publishedPartner
	@released
	
	An enumeration listing the stopping modes supported by this driver, to be passed to the Stop function.
*/
enum TStopMode 
	{
	/**
	Stopping due to normal operational reasons.
	*/
	EStopNormal,
	/**
	Stopping due to system power down.
	*/
	EStopPwrDown,
	/**
	Emergency stop. Deprecated.
	*/
	EStopEmergency
	};
	
	 
class DChannelComm;

/**
	@publishedPartner
	@released
	
	An abstract class for a serial comm PDD.
*/
class DComm : public DBase
	{
public:
	/**
	Starts receiving characters.
	@return KErrNone if successful; otherwise one of the other system wide error codes.
	*/
	virtual TInt Start() =0;
	
	/**
	Stops receiving characters.
	@param aMode The stopping reason as one of TStopMode.
	@see TStopMode
 	*/
	virtual void Stop(TStopMode aMode) =0;
	
	/**
	Starts or stop the uart breaking.
	@param aState ETrue to enable break signal(LCR) and EFalse disable break signal(LCR).
	*/
	virtual void Break(TBool aState) =0;
	
	/**
	Starts transmitting characters.
	*/
	virtual void EnableTransmit() =0;
	
	/**
	Read and translate the modem control lines.
	@return State changes. 
			For Example:
			RTS, DSR, RI, Carrier Detect.
	*/
	virtual TUint Signals() const =0;
	
	/**
	Set signals.
	@param aSetMask   A bit mask for those modem control signals which are to be asserted.
	@param aClearMask A bit mask for those modem control signals which are to be de-asserted.
					  Each bit in the bit masks above corresponds to a modem control signal. 
					  Bits are defined as one of:
					  KSignalCTS
					  KSignalDSR
					  KSignalDCD
					  KSignalRNG
					  KSignalRTS
					  KSignalDTR
					  KSignalBreak
	
	*/
	virtual void SetSignals(TUint aSetMask,TUint aClearMask) =0;
	
	/**
	Validates a new configuration.
	@param  aConfig Const reference to the comms configuration structure; to hold the configuration settings for serial comm port.
	@return KErrNone if successful; otherwise one of the other system wide error codes.
	@see TCommConfigV01
	*/
	virtual TInt ValidateConfig(const TCommConfigV01 &aConfig) const =0;
	
	/**
	Configures the hardware device. This is device specific API, that provides functionality to configure the uart.
	@param aConfig configuration settings for the device.
	@see TCommConfigV01
	*/
	virtual void Configure(TCommConfigV01 &aConfig) =0;
	
	/**
	Gets the capabilities of the comm PDD.
	@param aCaps On return this descriptor should have been filled with capabilities. 
	*/
	virtual void Caps(TDes8 &aCaps) const =0;
	
	/**
	Checks the configuration.
	@param aConfig A reference to the structure TCommConfigV01 with configuration to check.
	@see TCommConfigV01
	*/
	virtual void CheckConfig(TCommConfigV01& aConfig)=0;
	
	/**
	Disable all IRQs.
	@return The state of the interrupts before disable, which is used to restore the interrupt state.
	*/
	virtual TInt DisableIrqs()=0;
	
	/**
	Restore IRQs to the passed level.
	@param  aIrq The level to restore the IRQs to.
	*/
	virtual void RestoreIrqs(TInt aIrq)=0;
	
	/**
	Returns a pointer to the DFC queue that should be used by the comm LDD.
	@param 	aUnit Unit for which the DfcQ is retrieved.
	@return A Pointer to the DFC queue that should be used by the USB LDD.
	@see TDfcQue
	*/
	virtual TDfcQue* DfcQ(TInt aUnit)=0;
	
	/**
	Checks power status.
	@return ETrue if status is good, EFalse otherwise.
	*/
	inline TBool PowerGood();
	inline void SetCurrent(TInt aCurrent);
	inline void ReceiveIsr(TUint* aChar, TInt aCount, TInt aXonXoff);
	inline TInt TransmitIsr();
	inline void CheckTxBuffer();
	inline void StateIsr(TUint aSignals);
	inline TBool Transmitting();
public:
	/**
	Pointer to the logical channel object which is derived from DLogicChannel.
	*/
	DChannelComm *iLdd;
	/**
	A Boolean flag to indicate when transmission is in progress [ETrue=(Trasnmission in progress)].
	*/
	TBool iTransmitting;
	};

/**
@internalComponent
*/
class DDeviceComm : public DLogicalDevice
	{
public:
	DDeviceComm();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};


//
// TClientSingleBufferRequest
//
class TClientSingleBufferRequest
{
public:
	TClientSingleBufferRequest() 
		{
		Reset();
		}
	~TClientSingleBufferRequest() 
		{
		if (iBufReq)
			Kern::DestroyClientBufferRequest(iBufReq);
		Reset();
		}
	void Reset()
		{
		iBufReq = NULL;
		iBuf = NULL;
		iLen = 0;
		}
	TInt Create()
		{
		if (iBufReq)
			return KErrNone;
		TInt r = Kern::CreateClientBufferRequest(iBufReq, 1, TClientBufferRequest::EPinVirtual);
		return r;
		}
	TInt Setup(TRequestStatus* aStatus, TAny* aDes, TInt aLen=0) 
		{
		TInt r = iBufReq->Setup(iBuf, aStatus, aDes);
		if (r == KErrNone)
			iLen = aLen;
		return r;
		}
	TInt SetupFromPtr(TRequestStatus* aStatus, TLinAddr aPtr, TInt aLen) 
		{
		TInt r = iBufReq->Setup(iBuf, aStatus, aPtr, aLen);
		iLen = aLen;
		return r;
		}
	void Complete(DThread* aClient, TInt aReason) 
		{
		if (iBufReq)
			{
			iBuf = NULL;
			Kern::QueueBufferRequestComplete(aClient, iBufReq, aReason);
			}
		}
	TClientBufferRequest* iBufReq;
	TClientBuffer* iBuf;
	TInt iLen;
};

class DCommPowerHandler;
/**
@internalComponent
*/
class DChannelComm : public DLogicalChannel
	{
public:
	enum TState {EOpen,EActive,EClosed};
	enum TRequest {ERx=1, ETx=2, ESigChg=4, EBreak=8, EAll=0xff};

	DChannelComm();
	~DChannelComm();
	virtual void ReceiveIsr(TUint* aChar, TInt aCount, TInt aXonXoff);
	virtual void CheckTxBuffer();
	virtual void StateIsr(TUint aSignals);
	virtual TInt TransmitIsr();
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);

	/**	@publishedPartner
		@released */
	virtual void UpdateSignals(TUint aSignals);
	inline void SetStatus(TState aStatus);
	virtual TInt SendMsg(TMessageBase* aMsg);
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(TMessageBase* aMsg);
	void DoCancel(TInt aMask);
	TInt DoControl(TInt aId, TAny* a1, TAny* a2);
	void DoRequest(TInt aId, TAny* a1, TAny* a2);
	void DoPowerUp();
	void Start();
	TInt Shutdown();
	void BreakOn();
	void BreakOff();
	void AssertFlowControl();
	void ReleaseFlowControl();
	TInt SetRxBufferSize(TInt aSize);
	void ResetBuffers(TBool aResetTx);
	void DoDrainRxBuffer(TInt aEndIndex);
	void DoFillTxBuffer();
	void DoCompleteRx();
	void DoCompleteTx();
	void Complete(TInt aMask, TInt aReason);
	inline void DrainRxBuffer()	{ iRxDrainDfc.Add(); }
	inline void RxComplete();
	inline void TxComplete();
protected:
	inline void EnableTransmit();
	inline TInt IsLineFail(TUint aFailSignals);
	inline TInt PddStart();
	inline void Stop(TStopMode aMode);
	inline void PddBreak(TBool aState);
	inline TUint Signals() const;
	inline void SetSignals(TUint aSetMask,TUint aClearMask);
	inline TInt ValidateConfig(const TCommConfigV01 &aConfig) const;
	inline void PddConfigure(TCommConfigV01 &aConfig);
	inline void PddCaps(TDes8 &aCaps) const;
	inline void PddCheckConfig(TCommConfigV01& aConfig);
	inline TBool Transmitting();
private:
	static void PowerUpDfc(TAny* aPtr);
	static void PowerDownDfc(TAny* aPtr);
	static void DrainRxDfc(TAny* aPtr);
	static void FillTxDfc(TAny* aPtr);
	static void CompleteRxDfc(TAny* aPtr);
	static void CompleteTxDfc(TAny* aPtr);
	static void TimerDfcFn(TAny* aPtr);
	static void SigNotifyDfc(TAny* aPtr);
	void TimerDfc();
	static void MsCallBack(TAny* aPtr);
	inline TBool IsTerminator(TUint8 aChar);
	inline void SetTerminator(TUint8 aChar);
	inline TInt RxCount();
	inline TInt TxCount();
	inline TBool AreAnyPending() const;
	void InitiateRead(TInt aLength);
	void InitiateWrite();
	void DoSigNotify();
	void UpdateAndProcessSignals();

	
	TUint FailSignals(TUint aHandshake);
	TUint HoldSignals(TUint aHandshake);
	TUint FlowControlSignals(TUint aHandshake);
	TUint AutoSignals(TUint aHandshake);
	TInt SetConfig(TCommConfigV01& aConfig);
	void CheckOutputHeld();
	void RestartDelayedTransmission();

	static void FinishBreak(TAny* aSelf); // Called when timer indicating break should finish expires
	void QueueFinishBreakDfc();	// Called to queue dfc to finish break
	static void FinishBreakDfc(TAny* aSelf); // Dfc called to finish break
	void FinishBreakImplementation(TInt aError); // Actual implementation to finish break

public:
	// Port configuration
	TCommConfigV01 iConfig;

	/**	@publishedPartner
		@released */
	TUint iRxXonChar;

	/**	@publishedPartner
		@released */
	TUint iRxXoffChar;

	TInt TurnaroundSet(TUint aNewTurnaroundMilliSeconds);
	TBool TurnaroundStopTimer();
	TInt TurnaroundClear();
	TInt RestartTurnaroundTimer();
	static void TurnaroundStartDfc(TAny* aSelf);
	void TurnaroundStartDfcImplementation(TBool inIsr);
	static void TurnaroundTimeout(TAny* aSelf);
	void TurnaroundTimeoutImplementation();

	// General items
	DThread* iClient;
	DCommPowerHandler* iPowerHandler;
	TDfc iPowerUpDfc;
	TDfc iPowerDownDfc;
	TState iStatus;
	TDfc iRxDrainDfc;
	TDfc iRxCompleteDfc;
	TDfc iTxFillDfc;
	TDfc iTxCompleteDfc;
	TDfc iTimerDfc;
	TDfc iSigNotifyDfc;
	TUint iFlags;				//
	TUint iSignals;				// State of handshake lines
	TUint iFailSignals;			// 1 bit means line low causes line fail error
	TUint iHoldSignals;			// 1 bit means line low halts TX
	TUint iFlowControlSignals;	// 1 bit means signal is used for RX flow control
	TUint iAutoSignals;			// 1 bit means signal is high when channel is open
	TUint8 iTerminatorMask[32];	// 1 bit means character is a terminator
	TUint8 iStandby;			// ETrue means the machine is transiting to/from standby
	TUint8 iMsgHeld;			// ETrue means a message has been held up waiting the end of from standby transition 

	// Min Turnaround time between Rx and Tx
	TUint		iTurnaroundMicroSeconds;		// delay after a receive before transmission in us
	TUint		iTurnaroundMinMilliSeconds;		// delay after a receive before transmission in ms
	TUint       iTurnaroundTimerStartTime;      // stores the start time of the turnaround timer.
	TUint8      iTurnaroundTimerStartTimeValid; // stores turnaround timer status 0 after boot, 1 if the timestamp is valid, and 2 if invalid
	TUint8		iTurnaroundTimerRunning;		// a receive has started the timer
	TUint8		iTurnaroundTransmitDelayed;		// a transmission is held until time elapses after a receive
	TUint8		iSpare;
	NTimer	iTurnaroundTimer;				// used to delay transmission after a receive
	TDfc		iTurnaroundDfc;					// used in interrupt space, to trigger a call in user space

	// RX buffer related items
	TUint8 *iRxCharBuf;			// stores received characters
	TInt iRxBufSize;			// Size of the LDD receive buffer. 
	TUint8 *iRxErrorBuf;		// stores received character error status
	volatile TInt iRxPutIndex;	// Index for next RX char to be stored
	TInt iRxGetIndex;			// Index for next RX char to be retrieved
	TInt iFlowControlLowerThreshold;	// release flow control threshold
	TInt iFlowControlUpperThreshold;	// assert flow control threshold
	TInt iRxDrainThreshold;				// drain rx buffer before completion threshold
	TInt iRxBufCompleteIndex;	// One after last char to be forwarded due to completion
	TBool iInputHeld;			// TRUE if we have asserted flow control

	// RX client related items
	TClientSingleBufferRequest iRxBufReq;
	TInt iRxDesPos;				// pos of next char to be stored in client descriptor
	TUint8 iRxOutstanding;		// TRUE if a client read is outstanding
	TUint8 iNotifyData;			// TRUE if data available notifier outstanding
	TInt iRxError;
	NTimer iTimer;				// timer for ReadOneOrMore
	TInt iTimeout;				// timeout period for ReadOneOrMore
	TInt iRxOneOrMore;

	// TX buffer related items
	TUint8 *iTxBuffer;			// stores characters awaiting transmission
	TInt iTxPutIndex;			// Index for next TX char to be stored
	volatile TInt iTxGetIndex;	// Index for next TX char to be output
	TInt iTxBufSize;
	TInt iTxFillThreshold;		// fill tx buffer threshold
	TInt iOutputHeld;			// bits set if peer has asserted flow control
	TInt iJamChar;				// character to jam into TX output stream

	// TX client related items
	TClientSingleBufferRequest iTxBufReq;
	TInt iTxDesPos;				// pos of next char to be fetched from client descriptor
	TBool iTxOutstanding;		// TRUE if a client write is outstanding
	TInt iTxError;

	// Signal change notification
	TUint iNotifiedSignals;
	TUint iSigNotifyMask;
	TClientDataRequest<TUint>* iSignalsReq;

	// hackery
	TVirtualPinObject* iPinObjSetConfig;
	TInt iReceived;
	
	// Break related items
	TInt		 iBreakTimeMicroSeconds;
	TTickLink iBreakTimer; // Used to time how long the break should last for
	TDfc		 iBreakDfc;	
	TClientRequest* iBreakStatus;
	TBool		iBreakDelayedTx;
	TBool		iTurnaroundBreakDelayed;

	TSpinLock iLock;
	};

/**
@internalComponent
*/
class DCommPowerHandler : public DPowerHandler
	{
public: // from DPOwerHandler
	void PowerUp();
	void PowerDown(TPowerState);
public:
	DCommPowerHandler(DChannelComm* aChannel);
public:
	DChannelComm* iChannel;
	};

#include <drivers/comm.inl>

#endif

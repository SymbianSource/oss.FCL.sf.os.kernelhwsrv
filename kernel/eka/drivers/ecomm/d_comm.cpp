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
// e32\drivers\ecomm\d_comm.cpp
// 
//

#include <drivers/comm.h>
#include <kernel/kern_priv.h>
#include <e32hal.h>
#include <e32uid.h>

// Logging
#define LOG_ON(x) Kern::Printf##x
#define LOG_OFF(x)
#define LOG		LOG_OFF


//#define __UART_RX_ERROR(x)    *(TUint*)0xfeedface=(x)
//#define __OVERRUN() *(TUint*)0xfaece5=0

#define __UART_RX_ERROR(x)
#define __OVERRUN()

_LIT(KLddName,"Comm");


const TUint KXoffSignal=0x80;
//
const TUint KBreaking=0x02;
const TUint KBreakPending=0x04;
//
enum TPanic
	{
	ESetConfigWhileRequestPending,
	ESetSignalsSetAndClear,
	EResetBuffers,
	ESetReceiveBufferLength,
	};

DECLARE_STANDARD_LDD()
	{
	return new DDeviceComm;
	}

DDeviceComm::DDeviceComm()
//
// Constructor
//
	{
	LOG(("DDeviceComm::DDeviceComm"));
	iParseMask=KDeviceAllowAll;
	iUnitsMask=0xffffffff; // Leave units decision to the PDD
	iVersion=TVersion(KCommsMajorVersionNumber,KCommsMinorVersionNumber,KCommsBuildVersionNumber);
	}

TInt DDeviceComm::Install()
//
// Install the device driver.
//
	{
	LOG(("DDeviceComm::Install"));
	return(SetName(&KLddName));
	}

void DDeviceComm::GetCaps(TDes8& aDes) const
//
// Return the Comm capabilities.
//
	{
	LOG(("DDeviceComm::GetCaps"));
	TPckgBuf<TCapsDevCommV01> b;
	b().version=TVersion(KCommsMajorVersionNumber,KCommsMinorVersionNumber,KCommsBuildVersionNumber);
	Kern::InfoCopy(aDes,b);
	}

TInt DDeviceComm::Create(DLogicalChannelBase*& aChannel)
//
// Create a channel on the device.
//
	{
	LOG(("DDeviceComm::Create"));
	aChannel=new DChannelComm;
	return aChannel?KErrNone:KErrNoMemory;
	}

DChannelComm::DChannelComm()
//
// Constructor
//
	:	iPowerUpDfc(DChannelComm::PowerUpDfc,this,3),
		iPowerDownDfc(DChannelComm::PowerDownDfc,this,3),
		iRxDrainDfc(DChannelComm::DrainRxDfc,this,2),
		iRxCompleteDfc(DChannelComm::CompleteRxDfc,this,2),
		iTxFillDfc(DChannelComm::FillTxDfc,this,2),
		iTxCompleteDfc(DChannelComm::CompleteTxDfc,this,2),
		iTimerDfc(DChannelComm::TimerDfcFn,this,3),
		iSigNotifyDfc(DChannelComm::SigNotifyDfc,this,2),
//		iTurnaroundMinMilliSeconds(0),
//		iTurnaroundTimerRunning(EFalse),
//		iTurnaroundTransmitDelayed(EFalse),
		iTurnaroundTimer(DChannelComm::TurnaroundStartDfc, this),
		iTurnaroundDfc(DChannelComm::TurnaroundTimeout, this, 2),
		iTimer(DChannelComm::MsCallBack,this),
		iBreakDfc(DChannelComm::FinishBreakDfc, this, 2),
		iLock(TSpinLock::EOrderGenericIrqLow3)
	{
	LOG(("DChannelComm"));
//
// Setup the default config
//
	iConfig.iRate=EBps9600;
	iConfig.iDataBits=EData8;
	iConfig.iStopBits=EStop1;
	iConfig.iParity=EParityNone;
	iConfig.iFifo=EFifoEnable;
	iConfig.iHandshake=KConfigObeyCTS;
	iConfig.iParityError=KConfigParityErrorFail;
	iConfig.iSIREnable=ESIRDisable;
//	iConfig.iTerminatorCount=0;
//	iConfig.iTerminator[0]=0;
//	iConfig.iTerminator[1]=0;
//	iConfig.iTerminator[2]=0;
//	iConfig.iTerminator[3]=0;
	iConfig.iXonChar=0x11; // XON
	iConfig.iXoffChar=0x13; // XOFF
//	iConfig.iSpecialRate=0;
//	iConfig.iParityErrorChar=0;
	iRxXonChar=0xffffffff;
	iRxXoffChar=0xffffffff;
	iStatus=EOpen;
//	iFlags=0;
//	iSignals=0;
//	iFailSignals=0;
//	iHoldSignals=0;
//	iFlowControlSignals=0;
//	iAutoSignals=0;
//	iTerminatorMask[0...31]=0;
//	iShutdown=EFalse;
//	iRxCharBuf=NULL;
//	iRxErrorBuf=NULL;
//	iRxPutIndex=0;
//	iRxGetIndex=0;
//	iRxBufSize=0;
//	iFlowControlLowerThreshold=0;
//	iFlowControlUpperThreshold=0;
//	iRxDrainThreshold=0;
//	iRxBufCompleteIndex=0;
//	iInputHeld=EFalse;
//	iRxClientBufReq=NULL;
//	iRxDesPos=0;
//	iRxLength=0;
//	iRxOutstanding=EFalse;
//	iRxError=KErrNone;
//	iTxBuffer=NULL;
//	iTxPutIndex=0;
//	iTxGetIndex=0;
//	iTxBufSize=0;
//	iTxFillThreshold=0;
	iOutputHeld=0;
	iJamChar=KTxNoChar;
//	iTxDesPtr=NULL;
//	iTxDesPos=0;
//	iTxDesLength=0;
//	iTxOutstanding=EFalse;
//	iTxError=KErrNone;

//	iTimeout=10;
	iTimeout=NKern::TimerTicks(5);
	iClient=&Kern::CurrentThread();
	iClient->Open();
//	iSigNotifyMask=0;
//	iSignalsPtr=NULL;
//	iSigNotifyStatus=NULL;
	iBreakStatus=NULL;
	iNotifiedSignals=0xffffffff;
	iPinObjSetConfig=NULL;
	}

DChannelComm::~DChannelComm()
//
// Destructor
//
	{
	LOG(("~DChannelComm"));
	if (iPowerHandler)
		{
		iPowerHandler->Remove(); 
		delete iPowerHandler;
		}
    if (iRxCharBuf)
        Kern::Free(iRxCharBuf);
    if (iTxBuffer)
        Kern::Free(iTxBuffer);
	if (iBreakStatus)
		Kern::DestroyClientRequest(iBreakStatus);
	if (iSignalsReq)
		Kern::DestroyClientRequest(iSignalsReq);
	if (iPinObjSetConfig)
		Kern::DestroyVirtualPinObject(iPinObjSetConfig);
	Kern::SafeClose((DObject*&)iClient, NULL);
	}


void DChannelComm::Complete(TInt aMask, TInt aReason)
	{
	LOG(("Complete(aMask=%x aReason=%d)", aMask, aReason));
	if (aMask & ERx)
		iRxBufReq.Complete(iClient, aReason);
	if (aMask & ETx)
		iTxBufReq.Complete(iClient, aReason);
	if (aMask & ESigChg)
		Kern::QueueRequestComplete(iClient, iSignalsReq, aReason);
	if ((aMask & EBreak) && iBreakStatus && iBreakStatus->IsReady())
		Kern::QueueRequestComplete(iClient, iBreakStatus, aReason);
	}

TInt DChannelComm::Shutdown()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("DChannelComm::Shutdown()"));
	LOG(("Shutdown()"));

    if (iStatus == EActive)
        Stop(EStopPwrDown);

    Complete(EAll, KErrAbort);

	// UART interrupts are disabled; must make sure DFCs are not queued.
	iRxDrainDfc.Cancel();
	iRxCompleteDfc.Cancel();
	iTxFillDfc.Cancel();
	iTxCompleteDfc.Cancel();
	iTimer.Cancel();
	iTurnaroundTimer.Cancel();
	iTurnaroundDfc.Cancel();
	iTimerDfc.Cancel();
	iSigNotifyDfc.Cancel();
	iPowerUpDfc.Cancel();
	iPowerDownDfc.Cancel();
	iBreakTimer.Cancel();
	iBreakDfc.Cancel();
	
	if (iPdd)
		SetSignals(0,iFlowControlSignals|iAutoSignals);

	return KErrCompletion;
	}

TInt DChannelComm::DoCreate(TInt aUnit, const TDesC8* /*anInfo*/, const TVersion &aVer)
//
// Create the channel from the passed info.
//
	{
	LOG(("DoCreate(aUnit=%d,...)", aUnit));
	if(!Kern::CurrentThreadHasCapability(ECapabilityCommDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by ECOMM.LDD (Comm Driver)")))
		return KErrPermissionDenied;
	if (!Kern::QueryVersionSupported(TVersion(KCommsMajorVersionNumber,KCommsMinorVersionNumber,KCommsBuildVersionNumber),aVer))
		return KErrNotSupported;

	// set up the correct DFC queue
	SetDfcQ(((DComm*)iPdd)->DfcQ(aUnit));
	iPowerUpDfc.SetDfcQ(iDfcQ);
	iPowerDownDfc.SetDfcQ(iDfcQ);
	iRxDrainDfc.SetDfcQ(iDfcQ);
	iRxCompleteDfc.SetDfcQ(iDfcQ);
	iTxFillDfc.SetDfcQ(iDfcQ);
	iTxCompleteDfc.SetDfcQ(iDfcQ);
	iTimerDfc.SetDfcQ(iDfcQ);
	iSigNotifyDfc.SetDfcQ(iDfcQ);
	iTurnaroundDfc.SetDfcQ(iDfcQ);
	iBreakDfc.SetDfcQ(iDfcQ);
	iMsgQ.Receive();

	// initialise the TX buffer
	iTxBufSize=KTxBufferSize;
	iTxBuffer=(TUint8*)Kern::Alloc(iTxBufSize);
	if (!iTxBuffer)
		return KErrNoMemory;
	iTxFillThreshold=iTxBufSize>>1;

	// initialise the RX buffer
	iRxBufSize=KDefaultRxBufferSize;
	iRxCharBuf=(TUint8*)Kern::Alloc(iRxBufSize<<1);
	if (!iRxCharBuf)
		return KErrNoMemory;
	iRxErrorBuf=iRxCharBuf+iRxBufSize;
	iFlowControlLowerThreshold=iRxBufSize>>2;
	iFlowControlUpperThreshold=3*iRxBufSize>>2;
	iRxDrainThreshold=iRxBufSize>>1;

	// Create request objects
	TInt r = Kern::CreateClientDataRequest(iSignalsReq);
	if (r==KErrNone)
		r = Kern::CreateClientRequest(iBreakStatus);
	if (r==KErrNone)
		r = iRxBufReq.Create();
	if (r==KErrNone)
		r = iTxBufReq.Create();
	if (r==KErrNone)
		r = Kern::CreateVirtualPinObject(iPinObjSetConfig);
	if (r != KErrNone)
		return r;

	((DComm *)iPdd)->iLdd=this;
	PddCheckConfig(iConfig);
	iFailSignals=FailSignals(iConfig.iHandshake);
	iHoldSignals=HoldSignals(iConfig.iHandshake);
	iFlowControlSignals=FlowControlSignals(iConfig.iHandshake);
	iAutoSignals=AutoSignals(iConfig.iHandshake);

	// create the power handler
	iPowerHandler=new DCommPowerHandler(this);
	if (!iPowerHandler)
		return KErrNoMemory;
	iPowerHandler->Add();
	DoPowerUp();

	return KErrNone;
	}

TInt DChannelComm::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	// Ensure that each channel can only be used by a single thread.
	return (aThread!=iClient) ?  KErrAccessDenied : KErrNone;
	}

void DChannelComm::MsCallBack(TAny* aPtr)
	{
	// called from ISR when timer completes
	DChannelComm *pC=(DChannelComm*)aPtr;
	pC->iTimerDfc.Add();
	}

void DChannelComm::TimerDfcFn(TAny* aPtr)
	{
	DChannelComm *pC=(DChannelComm*)aPtr;
	pC->TimerDfc();
	}

void DChannelComm::TimerDfc()
	{
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	if (iRxOutstanding)
		{
		if (iRxGetIndex==iRxPutIndex)
			{
			// buffer empty after timeout period, so complete
			iRxBufCompleteIndex=iRxPutIndex;
			iRxOutstanding=EFalse;
			iRxOneOrMore=0;
			__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
			DoCompleteRx();
			return;
			}
		// buffer not empty, so drain buffer and requeue timer
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
		DoDrainRxBuffer(iRxPutIndex);
		return;
		}
	__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
	}

void DChannelComm::DrainRxDfc(TAny* aPtr)
	{
	DChannelComm *pC=(DChannelComm*)aPtr;
	pC->DoDrainRxBuffer(pC->iRxPutIndex);
	}

// Drain RX buffer in a DFC
void DChannelComm::DoDrainRxBuffer(TInt aEndIndex)
	{
	// if RX completion DFC is queued, leave buffer draining to it
	if (iRxCompleteDfc.Queued())
		return;

	LOG(("DoDrainRxBuffer(aEndIndex=%d) iRxDesPos=%d iRxBufReq.iLen=%d", aEndIndex, iRxDesPos, iRxBufReq.iLen));
    
	// If there's an Rx request with bytes outstanding...
	if (iRxBufReq.iBuf && iRxDesPos<iRxBufReq.iLen)
        {
        TInt space=iRxBufReq.iLen-iRxDesPos; // the amount of the client buffer left to fill
        TInt avail=aEndIndex-iRxGetIndex;	 // the amount of data in the Rx buffer to copy to the client buffer
        if (avail<0) // true if the data to drain wraps around the end of the buffer (i.e. the last byte to copy has a linear address less than that of the first byte)
            avail+=iRxBufSize;
        TInt len=Min(space,avail); // total number of bytes to drain

		// Drain up to (but not beyond) the end of the Rx buffer
        TInt len1=Min(len,iRxBufSize-iRxGetIndex);  // number of bytes to the end of the buffer
        TPtrC8 des(iRxCharBuf+iRxGetIndex,len1);

		TInt r = Kern::ThreadBufWrite(iClient, iRxBufReq.iBuf, des, iRxDesPos, KChunkShiftBy0, iClient);
        if (r != KErrNone)
            {
            iRxError=r;
            DoCompleteRx();
            return;
            }

		// Update the client buffer offset and the Rx buffer read pointer with what we've done so far
        TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
        iRxDesPos += len1;
        iRxGetIndex+=len1;
        if (iRxGetIndex>=iRxBufSize)
            iRxGetIndex-=iRxBufSize;
        __SPIN_UNLOCK_IRQRESTORE(iLock, irq);

		// If the data wraps around the end of the Rx buffer, now write out the second part
		// which starts at the beginning of the Rx buffer.
        len-=len1;
        if (len)
            {
            des.Set(iRxCharBuf,len);
			r=Kern::ThreadBufWrite(iClient, iRxBufReq.iBuf, des, iRxDesPos, KChunkShiftBy0, iClient);
            if (r != KErrNone)
                {
                iRxError=r;
                DoCompleteRx();
                return;
                }

			// Update client buffer offset and Rx buffer read offset
            irq = __SPIN_LOCK_IRQSAVE(iLock);
            iRxDesPos += len;
            iRxGetIndex+=len;
            __SPIN_UNLOCK_IRQRESTORE(iLock, irq);
            }

        // release flow control if necessary
        if (iInputHeld && RxCount()<=iFlowControlLowerThreshold)
            ReleaseFlowControl();

        // if we are doing ReadOneOrMore, start the timer
        if (iRxOneOrMore>0)
            {
            iTimer.OneShot(iTimeout);
            }
        }
    }


void DChannelComm::RxComplete()
{
	if (NKern::CurrentContext()==NKern::EInterrupt)
		iRxCompleteDfc.Add();
	else
		DoCompleteRx();			
}


void DChannelComm::CompleteRxDfc(TAny* aPtr)
	{
	DChannelComm *pC=(DChannelComm*)aPtr;
	pC->DoCompleteRx();
	}

void DChannelComm::DoCompleteRx()
	{
    LOG(("DoCompleteRx()"));
	if (iRxOneOrMore>0)
		iTimer.Cancel();
	if (iRxBufReq.iLen)
        {
        iRxOneOrMore=0;
        DoDrainRxBuffer(iRxBufCompleteIndex);
		iRxBufReq.Complete(iClient, iRxError);
		iRxDesPos=0;

        iRxError=KErrNone;
        // start Turnaround timer (got here because it received all data, timed out on a ReadOneOrMore or was terminated
        // early by FailSignals)
        RestartTurnaroundTimer();
        }
    else
        {
        Complete(ERx,KErrNone);
        // do not start Turnaround (got here on a request Data Available Notification)
        }
    }


void DChannelComm::TxComplete()
{
	if (NKern::CurrentContext()==NKern::EInterrupt)
		iTxCompleteDfc.Add(); 
	else
		DoCompleteTx();			
}


void DChannelComm::FillTxDfc(TAny* aPtr)
	{
	DChannelComm *pC=(DChannelComm*)aPtr;
	pC->DoFillTxBuffer();
	}

// Fill TX buffer in a DFC
void DChannelComm::DoFillTxBuffer()
	{
    LOG(("DFTB %d =%d",iTxDesPos,iTxBufReq.iLen));
	if (iTxBufReq.iBuf && iTxDesPos<iTxBufReq.iLen)
        {
        TInt space=iTxBufSize-TxCount()-1;
        TInt remaining=iTxBufReq.iLen-iTxDesPos;
        TInt len=Min(space,remaining);              // number of chars to transfer
        TInt len1=Min(len,iTxBufSize-iTxPutIndex);  // number of chars to wrap point
        TPtr8 des(iTxBuffer+iTxPutIndex,len1,len1);
        LOG(("DFTxB sp = %d rem = %d iOPH = %d",space, remaining,iOutputHeld));
		TInt r=Kern::ThreadBufRead(iClient, iTxBufReq.iBuf, des, iTxDesPos, KChunkShiftBy0);
        if (r != KErrNone)
            {
            iTxError=r;
            DoCompleteTx();
            return;
            }

        TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
        iTxDesPos+=len1;
        iTxPutIndex+=len1;
        if (iTxPutIndex>=iTxBufSize)
            iTxPutIndex-=iTxBufSize;
        __SPIN_UNLOCK_IRQRESTORE(iLock, irq);

        len-=len1;
        if (len)
            {
            des.Set(iTxBuffer,len,len);
			r=Kern::ThreadBufRead(iClient, iTxBufReq.iBuf, des, iTxDesPos, KChunkShiftBy0);
            if (r != KErrNone)
                {
                iTxError=r;
                DoCompleteTx();
                return;
                }

            irq = __SPIN_LOCK_IRQSAVE(iLock);
            iTxDesPos+=len;
            iTxPutIndex+=len;
            __SPIN_UNLOCK_IRQRESTORE(iLock, irq);
            }
        if (iTxDesPos==iTxBufReq.iLen)
            {
            // we have used up the client descriptor
            if (iConfig.iHandshake & KConfigWriteBufferedComplete)
                {
                iTxOutstanding=EFalse;
                DoCompleteTx();
                }
            }
        // if TX buffer not empty and not flow controlled, make sure TX is enabled
        if (iTxPutIndex!=iTxGetIndex  && (!iOutputHeld))
            {
            LOG(("Calling - DoTxBuff->ETx"));
            EnableTransmit();
            }
        }
    }

void DChannelComm::CompleteTxDfc(TAny* aPtr)
	{
	DChannelComm *pC=(DChannelComm*)aPtr;
	pC->DoCompleteTx();
	}

void DChannelComm::DoCompleteTx()
	{
	Complete(ETx,iTxError);
	iTxError=KErrNone;
	}

void DChannelComm::Start()
//
// Start the driver receiving.
//
	{
	LOG(("Start()"));
	if (iStatus!=EClosed)
		{
		PddConfigure(iConfig);
		PddStart();
		iStatus=EActive;
		if ((iConfig.iHandshake & KConfigSendXoff) && iJamChar>=0)
			EnableTransmit(); // Send XOn if there is one
		}
	}

void DChannelComm::BreakOn()
//
// Start the driver breaking.
//
	{
	LOG(("BreakOn()"));
	iFlags&=(~KBreakPending);
	iFlags|=KBreaking;
	PddBreak(ETrue);
	iBreakTimer.OneShot(iBreakTimeMicroSeconds, DChannelComm::FinishBreak, this);
	}

void DChannelComm::BreakOff()
//
// Stop the driver breaking.
//
	{
	LOG(("BreakOff()"));
	PddBreak(EFalse);
	iFlags&=(~(KBreakPending|KBreaking));
	}

void DChannelComm::AssertFlowControl()
	{
	iInputHeld=ETrue;
	SetSignals(0,iFlowControlSignals);
	if (iConfig.iHandshake&KConfigSendXoff)		// Doing input XON/XOFF
		{
		iJamChar=iConfig.iXoffChar;				// set up to send Xoff
		EnableTransmit();						// Make sure we are transmitting
		}
	}

void DChannelComm::ReleaseFlowControl()
	{
	iInputHeld=EFalse;
	SetSignals(iFlowControlSignals,0);
	if (iConfig.iHandshake&KConfigSendXoff)		// Doing input XON/XOFF
		{
		iJamChar=iConfig.iXonChar;				// set up to send Xon
		EnableTransmit();						// Make sure we are transmitting
		}
	}

TInt DChannelComm::SetRxBufferSize(TInt aSize)
//
// Set the receive buffer size.
//
	{
	LOG(("SetRxBufferSize(aSize=0x%X)", aSize));
	aSize=(aSize+3)&~3;
	TUint8 *newBuf=(TUint8*)Kern::ReAlloc(iRxCharBuf,aSize<<1);
	if (!newBuf)
		return KErrNoMemory;
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	iRxCharBuf=newBuf;
	iRxErrorBuf=newBuf+aSize;
	iRxBufSize=aSize;
	iFlowControlLowerThreshold=aSize>>2;
	iFlowControlUpperThreshold=3*aSize>>2;
	iRxDrainThreshold=aSize>>1;
	__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
	ResetBuffers(EFalse);
	return KErrNone;
	}

TInt DChannelComm::TurnaroundSet(TUint aNewTurnaroundMilliSeconds)
	{
	LOG(("TurnaroundSet(val=0x%X)", aNewTurnaroundMilliSeconds));
	TInt r = KErrNone;
	iTurnaroundMinMilliSeconds = aNewTurnaroundMilliSeconds;
	return r;
	}

TBool DChannelComm::TurnaroundStopTimer()
// Stop the timer and DFC
	{
	LOG(("TurnaroundStopTimer()"));
	
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	TBool result = iTurnaroundTimerRunning;
	if(result)
		iTurnaroundTimerRunning = EFalse;	
	__SPIN_UNLOCK_IRQRESTORE(iLock, irq);

	if (result)
		{
		iTurnaroundTimer.Cancel();
		iTurnaroundDfc.Cancel();
		}
	return result;
	}

TInt DChannelComm::TurnaroundClear()
// Clear any old timer and start timer based on new turnaround timer
// Called for any change: from T > 0 to T == 0 or (T = t1 > 0) to (T = t2 > 0)
// POLICY: If a write has already been delayed, it will be started immediately if the requested 
// turnaround time is elapsed else will only start after it is elapsed.
	{
	LOG(("TurnaroundClear()"));
	TInt r = KErrNone;
	TUint delta = 0;

	if(iTurnaroundTimerStartTimeValid == 1)
		{
		//Calculate the turnaround time elapsed so far.
		delta = (NKern::TickCount() - iTurnaroundTimerStartTime) * NKern::TickPeriod();
		}
    if(delta < iTurnaroundMicroSeconds)
		{
        iTurnaroundMinMilliSeconds = (iTurnaroundMicroSeconds - delta)/1000;
        iTurnaroundTimerStartTimeValid = 3; //Just to make sure that the turnaround timer start time is not captured.
        RestartTurnaroundTimer();
		}
    else
		{
		if(TurnaroundStopTimer())
			{
			// if a write is waiting, start a DFC to run it
			TurnaroundStartDfcImplementation(EFalse);
			}
		}
	iTurnaroundMinMilliSeconds = 0;
	return r;
	}

TInt DChannelComm::RestartTurnaroundTimer()
	{
	LOG(("RestartTurnaroundTimer()"));
	TInt r=KErrNone;

	// POLICY: if timer is running from a previous read, stop it and re-start it
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	TBool cancelDfcs = (iTurnaroundMinMilliSeconds > 0) && iTurnaroundTimerRunning;
	__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
	if (cancelDfcs) 
		{
		iTurnaroundTimer.Cancel();
		iTurnaroundDfc.Cancel();
		}

	// Start the timer & update driver state to reflect that the timer is running
	TInt timeout = 0;
	irq = __SPIN_LOCK_IRQSAVE(iLock);
	if(iTurnaroundMinMilliSeconds > 0)
		{
		iTurnaroundTimerRunning = ETrue;
		timeout = NKern::TimerTicks(iTurnaroundMinMilliSeconds);
		//Record the time stamp of turnaround timer start
		if(iTurnaroundTimerStartTimeValid != 3)
		    iTurnaroundTimerStartTime = NKern::TickCount();
		iTurnaroundTimerStartTimeValid = 1;
		}
	__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
	if (timeout)
		r=iTurnaroundTimer.OneShot(timeout);
	return r;
	}

void DChannelComm::TurnaroundStartDfc(TAny* aSelf)
	{
	DChannelComm* self = (DChannelComm*)aSelf;
	self->TurnaroundStartDfcImplementation(ETrue);		// in ISR so Irqs are already disabled
	}

void DChannelComm::TurnaroundStartDfcImplementation(TBool aInIsr)
	{
	LOG(("TurnaroundStartDfcImplementation(inIsr=%d)", aInIsr));
	TInt irq=0;
    if(!aInIsr)
		irq = __SPIN_LOCK_IRQSAVE(iLock);
	else 
		__SPIN_LOCK(iLock);

	iTurnaroundTimerRunning = EFalse;
	if(iTurnaroundTransmitDelayed || iTurnaroundBreakDelayed)
		{
        if(aInIsr)
			iTurnaroundDfc.Add();
		else
			{
			if(!aInIsr)
				__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
			else 
				__SPIN_UNLOCK(iLock);
			iTurnaroundDfc.Enque();
			return;
			}
		}
    if(!aInIsr)
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
	else 
		__SPIN_UNLOCK(iLock);
	}

void DChannelComm::TurnaroundTimeout(TAny* aSelf)
	{
	DChannelComm* self = (DChannelComm*)aSelf;
	self->TurnaroundTimeoutImplementation();
	}

void DChannelComm::TurnaroundTimeoutImplementation()
	{
	LOG(("TurnaroundTimeoutImplementation()"));
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	
	if(iTurnaroundBreakDelayed)
		{
		iTurnaroundBreakDelayed=EFalse;
		if (iStatus==EClosed)
			{
            __SPIN_UNLOCK_IRQRESTORE(iLock, irq);
			Complete(EBreak, KErrNotReady);
			return;
			}
		else if(IsLineFail(iFailSignals))	// have signals changed in the meantime?
			{
            __SPIN_UNLOCK_IRQRESTORE(iLock, irq);
			Complete(EBreak, KErrCommsLineFail);	// protected -> changed in signals ISR
			return;
			}
		if (iTurnaroundTransmitDelayed)
			{
			//delay write by break instead of turnaround
			iBreakDelayedTx = ETrue;
			iTurnaroundTransmitDelayed=EFalse;
			}
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
        BreakOn();
		}
	else if(iTurnaroundTransmitDelayed)
		{
		iTurnaroundTransmitDelayed = EFalse;		// protected -> prevent reentrant ISR
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
		
		RestartDelayedTransmission();
		}
	else 
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
	}

void DChannelComm::ResetBuffers(TBool aResetTx)
//
// Reset the receive and maybe the transmit buffer.
//
	{
	LOG(("ResetBuffers(aResetTx=%d)", aResetTx));
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	iRxPutIndex=0;
	iRxGetIndex=0;
	iRxBufCompleteIndex=0;
	if (aResetTx)
		{
		iTxPutIndex=0;
		iTxGetIndex=0;
		}
	__SPIN_UNLOCK_IRQRESTORE(iLock, irq);

	if (iStatus==EActive)
		ReleaseFlowControl();
	iInputHeld=EFalse;
	}

TInt DChannelComm::TransmitIsr()
//
// Return the next character to be transmitted to the ISR
//
	{
	TInt tChar=iJamChar;			// Look for control character to jam in
    if (tChar>=0)					// Control character to send
        {
		iJamChar=KTxNoChar;
		}
    else if (!iOutputHeld && iTxGetIndex!=iTxPutIndex)
        {
		// Get spinlock, disable interrupts to ensure we can reach the unlock 
		// statement. An FIQ before unlock that attempted to get lock would 
		// lead to CPU deadlock
		TInt irqstate = __SPIN_LOCK_IRQSAVE(iLock);
		
		// output not held and buffer not empty, get next char
		tChar=iTxBuffer[iTxGetIndex++];
		if (iTxGetIndex==iTxBufSize)
			iTxGetIndex=0;
			
		__SPIN_UNLOCK_IRQRESTORE(iLock, irqstate);
		}

	return tChar;
	}

void DChannelComm::ReceiveIsr(TUint* aChar, TInt aCount, TInt aXonXoff)
//
// Handle received character block from the ISR.
// aChar points to received characters, aCount=number received,
// aXonXoff=1 if XON received, -1 if XOFF received, 0 if neither
//
	{
	if (aXonXoff>0)
		{
		iOutputHeld &= ~KXoffSignal;	// Mark output ok. for XON/XOFF
		if (iOutputHeld==0)
			EnableTransmit();
		}
	else if (aXonXoff<0)
		{
		iOutputHeld |= KXoffSignal;		// Mark output held for XON/XOFF
		}
	if (aCount==0)						// if only XON or XOFF received
		return;

	// Get spinlock, disable interrupts to ensure we can reach the unlock 
	// statement. An FIQ before unlock that attempted to get lock would 
	// lead to CPU deadlock
	TInt irqstate = __SPIN_LOCK_IRQSAVE(iLock);

	TInt count = RxCount();
	iReceived++;

	// At or above the high water mark send xoff every other character
    if (count>=iFlowControlUpperThreshold && ((count&1)!=0 || aCount>1))
		AssertFlowControl();

	TUint* pE=aChar+aCount;
	TInt e=KErrNone;
	TInt i=iRxPutIndex;
	TInt g=iRxGetIndex;
	TInt s=iRxBufSize;
	g=g?g-1:s-1;
	TInt p=iRxOutstanding?-1:0;
    TInt thresh=iRxBufReq.iLen-iRxDesPos;
	while(aChar<pE)
		{
		TUint c=*aChar++;

		// Check for parity errors and replace char if so configured.
		if (c & KReceiveIsrParityError)
			{
			// Replace bad character
			if (iConfig.iParityError==KConfigParityErrorReplaceChar)
				c = c & ~(0xff|KReceiveIsrParityError) | iConfig.iParityErrorChar;
			// Ignore parity error
			if (iConfig.iParityError==KConfigParityErrorIgnore)
				c = c & ~KReceiveIsrParityError;
			}
		
		if (i!=g)
			{
			iRxCharBuf[i]=(TUint8)c;
			iRxErrorBuf[i]=(TUint8)(c>>24);

			if (c & KReceiveIsrMaskError)
				{
				__UART_RX_ERROR(c);
				if (c & KReceiveIsrOverrunError)
					e = KErrCommsOverrun;
				else if (c & KReceiveIsrBreakError)
					e = KErrCommsBreak;
				else if (c & KReceiveIsrFrameError)
					e = KErrCommsFrame;
				else if (c & KReceiveIsrParityError)
					e = KErrCommsParity;
				}
			count++;
			if (++i==s)
				i=0;
			if (p<0)
				{
				if (e || IsTerminator(TUint8(c)) || count==thresh)
					{
					// need to complete client request
					iRxError = e;
					p=i;
					}
				}
			}
		else
			{
			__OVERRUN();
			// buffer overrun, discard character
			e=KErrCommsOverrun;

			// make sure client is informed of overrun error
			iRxError=e;

			// discard remaining characters and complete
			p=i;
			break;
			}
		}
	iRxPutIndex=i;

	if (iRxOutstanding)
		{
		if (p>=0)
			{
			// need to complete client request
			iRxBufCompleteIndex=p;
			iRxOutstanding=EFalse;
            RxComplete();
			}
		else if (count>=iRxDrainThreshold)
			{
			// drain buffer but don't complete
			DrainRxBuffer();
			}
		else if (iRxOneOrMore<0)
			{
			// doing read one or more - drain the buffer
			// this will start the timer
			iRxOneOrMore=1;
			DrainRxBuffer();
			}
		}

	__SPIN_UNLOCK_IRQRESTORE(iLock, irqstate);

	if (iNotifyData)
		{
		iNotifyData=EFalse;
        RxComplete();
		}
	}

void DChannelComm::CheckTxBuffer()
	{
	// if buffer count < threshold, fill from client buffer
	TInt count=TxCount();
    if (iTxOutstanding && iTxDesPos<iTxBufReq.iLen && count<iTxFillThreshold)
		iTxFillDfc.Add();
	else if (count==0)
		{
		// TX buffer is now empty - see if we need to complete anything
		if (iTxOutstanding)
			{
            if (iTxBufReq.iLen==0)
				{
				// request was a zero-length write - complete if hardware flow control
				// is not asserted
				if ((~iSignals & iHoldSignals)==0)
					{
					iTxOutstanding=EFalse;
                    TxComplete();
					}
				}
			else
				{
				// request was normal TX - complete now if not doing early completion
				if (!(iConfig.iHandshake&KConfigWriteBufferedComplete))
					{
					iTxOutstanding=EFalse;
					TxComplete();
					}
				}
			}
		}
	}


//
// Pdd callback
//
void DChannelComm::UpdateSignals(TUint aSignals)
	{
    __KTRACE_OPT(KHARDWARE,Kern::Printf("CommSig: Upd %08x",aSignals));
    iSignals=(iSignals&~KDTEInputSignals)|(aSignals&KDTEInputSignals);
    DoSigNotify();	
	}



/**
 Handle a state change from the PDD. Called in ISR or DFC context.
 */
void DChannelComm::StateIsr(TUint aSignals)
    {
    iSignals=(iSignals&~KDTEInputSignals)|(aSignals&KDTEInputSignals);
    if (iSignalsReq->IsReady() && ((iSignals^iNotifiedSignals)&iSigNotifyMask) )
        {
        iSigNotifyDfc.Add();
        }
    if (IsLineFail(iFailSignals))
        {
        if (iRxOutstanding)
            {
            iRxError=KErrCommsLineFail;
            iRxBufCompleteIndex=iRxPutIndex;
            iRxOutstanding=EFalse;
			RxComplete();				
            }
        if (iTxOutstanding)
            {
            iTxError = KErrCommsLineFail;
            iTxOutstanding=EFalse;
			TxComplete();
			}
        }

	//
	// Now we must determine if output is to be held
	//
    TUint status = ~iSignals & iHoldSignals;
    if (iOutputHeld & KXoffSignal)
        status |= KXoffSignal;      // Leave the xon/xoff handshake bit

    LOG(("State - ISR - 0x%x",status));
    iOutputHeld=status;             // record new flow control state
    if (iTxGetIndex==iTxPutIndex)
        {
        // Tx buffer is empty
        if (iTxOutstanding && iTxBufReq.iLen==0 && (status&~KXoffSignal)==0)
            {
            // if hardware flow control released, complete zero-length write
            iTxOutstanding=EFalse;
			TxComplete();
			}
        }
    else if (status==0)
        {
        // Tx buffer not empty and flow control released, so restart transmission
        LOG(("Calling LDD:EnTx"));
        EnableTransmit();
        }
    }

// check if transmitter is flow controlled
void DChannelComm::CheckOutputHeld()
	{
	iOutputHeld=(iOutputHeld & KXoffSignal) | (~iSignals & iHoldSignals);
    LOG(("CheckOPH IOH = %d",iOutputHeld));
	}

void DChannelComm::HandleMsg(TMessageBase* aMsg)
	{

	if (iStandby)
		{ // postpone message handling to transition from standby
		iMsgHeld=ETrue;
		return;
		}

	TThreadMessage& m=*(TThreadMessage*)aMsg;
	LOG(("HandleMsg(%x a1=%x, a2=%x)", m.iValue, m.Int1(), m.Int2()));
	TInt id=m.iValue;
	if (id==(TInt)ECloseMsg)
		{
		Shutdown();
		iStatus = EClosed;
		m.Complete(KErrNone, EFalse);
		return;
		}
	else if (id==KMaxTInt)
		{
		// DoCancel
		DoCancel(m.Int0());
		m.Complete(KErrNone,ETrue);
		return;
		}

	if (id<0)
		{
		// DoRequest
        DoRequest(~id,m.Ptr1(),m.Ptr2());
		m.Complete(KErrNone,ETrue);
		}
	else
		{
		// DoControl
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}

void DChannelComm::DoCancel(TInt aMask)
//
// Cancel an outstanding request.
//
	{
	LOG(("DoCancel(%d)", aMask));
	if (aMask & RBusDevComm::ERequestReadCancel)
		{
		TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
		iRxOutstanding=EFalse;
		iNotifyData=EFalse;
		iRxDesPos=0;
        iRxBufReq.iLen=0;
		iRxError=KErrNone;
		iRxOneOrMore=0;
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
		iRxCompleteDfc.Cancel();
		iRxDrainDfc.Cancel();
		iTimer.Cancel();
		iTimerDfc.Cancel();
		Complete(ERx,KErrCancel);
		}
	if (aMask & RBusDevComm::ERequestWriteCancel)
		{
		TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
        iTurnaroundTransmitDelayed = EFalse;
        iTxPutIndex=0;
        iTxGetIndex=0;
        iTxOutstanding=EFalse;
        iTxDesPos=0;
        iTxBufReq.iLen=0;
		iTxError=KErrNone;
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
		iTxCompleteDfc.Cancel();
		iTxFillDfc.Cancel();
		Complete(ETx,KErrCancel);
		}
    if (aMask & RBusDevComm::ERequestNotifySignalChangeCancel)
        {
        iSigNotifyDfc.Cancel();
        Complete(ESigChg,KErrCancel);
        }
    if (aMask & RBusDevComm::ERequestBreakCancel)
		{
	 	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
		if (iTurnaroundBreakDelayed)
			iTurnaroundBreakDelayed=EFalse;
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);

		iBreakDfc.Cancel();
		iBreakTimer.Cancel();
		FinishBreakImplementation(KErrCancel);
		}
	}

/**
 Intercept messages in client context before they are sent to the DFC queue
 */
TInt DChannelComm::SendMsg(TMessageBase* aMsg)
	{
	TInt r = KErrNone;
	TInt max;
	TInt len = 0;
	TThreadMessage* m = (TThreadMessage*)aMsg;

	// Handle ECloseMsg & Cancel
    TInt id=aMsg->iValue;
    if (id==(TInt)ECloseMsg || id==KMaxTInt)
        {
		LOG(("SendMsg(%s)", (id==KMaxTInt)?"Cancel":"ECloseMsg"));
		// do nothing cos these are handled on the DFC side
        }
	
	// Handle control messages that access user memory here in client context
    else if (id >= 0) 
		{
		TAny* a1 = m->iArg[0];
		switch (aMsg->iValue) 
			{
			case RBusDevComm::EControlConfig:
				{			
				LOG(("SendMsg(EControlConfig, %x)", a1));
				TPtrC8 cfg((const TUint8*)&iConfig,sizeof(iConfig));
				return Kern::ThreadDesWrite(iClient,a1,cfg,0,KTruncateToMaxLength,iClient);
				}
			case RBusDevComm::EControlSetConfig:
				{
				LOG(("SendMsg(EControlSetConfig, %x)", a1));
				if (AreAnyPending()) 
					; // r = ESetConfigWhileRequestPending;
				else
					r = Kern::PinVirtualMemory(iPinObjSetConfig, (TLinAddr)a1, sizeof(TCommConfigV01));
				}
				break;
			case RBusDevComm::EControlCaps:
				{
				LOG(("SendMsg(EControlCaps, %x)", a1));
				TCommCaps2 caps;
				PddCaps(caps);
				return Kern::ThreadDesWrite(iClient,a1,caps,0,KTruncateToMaxLength,iClient);
				}
			default:
				// Allow other control messages to go to DFC thread
				LOG(("SendMsg(Ctrl %d, %x)", aMsg->iValue, a1));
				break;
			}
		}


	// Handle requests
	else 
		{
		TRequestStatus* status = (TRequestStatus*)m->iArg[0];
		TAny* a1 = m->iArg[1];
		TAny* a2 = m->iArg[2];
		TInt reqNo = ~aMsg->iValue;
		TInt irq;
		switch (reqNo)
			{
			case RBusDevComm::ERequestRead:
				{
			    iNotifyData=EFalse;
				// If client has *not* provided a buffer pointer, it means they only want
				// to know when data becomes available.
				if (!a1)
					{
					irq = __SPIN_LOCK_IRQSAVE(iLock);
					TBool isEmpty = (iRxPutIndex==iRxGetIndex);
					iNotifyData = isEmpty;
					__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
					if (!isEmpty) // if Rx buffer has bytes in it we can complete the request immediately
						{
						Kern::RequestComplete(status, KErrNone);
						return KErrNone;
						}
					// Do not start the Turnaround timer as this is not a Read request but a request for Data Available notification
					LOG(("--Buf Empty--"));
					}

				// Get buffer length if one has been given
				if (a2)
					r = Kern::ThreadRawRead(iClient,a2,&len,sizeof(len)); 

				// Check the client descriptor is valid and large enough to hold the required amount of data.
				if (a1 && r==KErrNone) 
					{
					max = Kern::ThreadGetDesMaxLength(iClient, a1);
					if (max<Abs(len) || max<0)
						r = KErrGeneral; // do not start the Turnaround timer (invalid Descriptor this read never starts)
					}

				LOG(("SendMsg(ERequestRead, %x, len=%d) max=%d r=%d", a1, len, max, r));

				// Set client descriptor length to zero & set up client buffer object
				if (a1 && r==KErrNone) 
					{
					TPtrC8 p(NULL,0);
					r = Kern::ThreadDesWrite(iClient,a1,p,0,0,iClient);
					if (r == KErrNone)
						r = iRxBufReq.Setup(status, a1, len);
					}
				}
			break;


			//
			// ERequestWrite
			//
			case RBusDevComm::ERequestWrite:
				if (iStatus==EClosed)
					r = KErrNotReady;
				else if (!a1) 
					r = KErrArgument;
				else 
					r=Kern::ThreadRawRead(iClient, a2, &len, sizeof(len));
				LOG(("SendMsg(ERequestWrite, %x, len=%d) r=%d", a1, len, r));

				// Setup pending client request for this write
				if (r==KErrNone)
					r = iTxBufReq.Setup(status, a1, len);		
				break;


			//
			// ERequestBreak: a1 points to the number of microseconds to break for
			//
			case RBusDevComm::ERequestBreak:
				r = Kern::ThreadRawRead(iClient, a1, &iBreakTimeMicroSeconds, sizeof(TInt));
				if (r == KErrNone)
					r = iBreakStatus->SetStatus(status);					
				LOG(("SendMsg(ERequestBreak, %x) bktime=%d r=%d", a1, iBreakTimeMicroSeconds, r));
				break;


			//
			// ERequestNotifySignalChange:	a1 points to user-side int to receive the signals bitmask
			//								a2 points to the bitmask of signals the user is interested in
			//
			case RBusDevComm::ERequestNotifySignalChange:
				LOG(("SendMsg(ERequestNotifySignalChange, %x, %x)", a1, a2));
				if (!a1 || !a2)
					{
					r = KErrArgument;
					break;
					}
				// Setup word-sized client buffer
				r = Kern::ThreadRawRead(iClient,a2,&iSigNotifyMask,sizeof(TUint));
				irq = __SPIN_LOCK_IRQSAVE(iLock);
				if (r==KErrNone) 
					{
					r = iSignalsReq->SetStatus(status);
					if (r==KErrNone) 
						iSignalsReq->SetDestPtr(a1);
					}
				LOG(("ERequestNotifySignalChange: mask is %x, r is %d", iSigNotifyMask, r));
				__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
				break;


			// Unknown request
			default:
				LOG(("SendMsg(req %d, %x, %x)", reqNo, a1, a2));
				r = KErrNotSupported;
				break;

			}

			// If the request has an error, complete immediately
			if (r!=KErrNone)
				Kern::RequestComplete(status, r);
		}

	// Send the client request to the DFC queue unless there's been an error
	if (r==KErrNone)
		r = DLogicalChannel::SendMsg(aMsg);
	LOG(("<SendMsg ret %d", r));
	return r;

	}


/**
 Handle asynchronous requests. Called in DFC context.
 */
void DChannelComm::DoRequest(TInt aReqNo, TAny* a1, TAny* a2)
    {
	LOG(("DoRequest(%d %x %x)", aReqNo, a1, a2));

    //
    // First check if we have started
    //
    if (iStatus==EOpen)
        {
        Start();
        CheckOutputHeld();
        SetSignals(iAutoSignals,0);
        LOG(("DReq- RFC"));
        ReleaseFlowControl();
        }
    //
    // Check for a line fail
    //
    if (IsLineFail(iFailSignals))
		{
		Complete(EAll, KErrCommsLineFail);
		return;
		}	

    //
    // Now we can dispatch the async request
    //
    switch (aReqNo)
        {
        case RBusDevComm::ERequestRead:
			InitiateRead(iRxBufReq.iLen);
            break;

        case RBusDevComm::ERequestWrite:
            {
			
			// See if we need to delay the write
            TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
			iTurnaroundTransmitDelayed = iTurnaroundTimerRunning!=0;
			iBreakDelayedTx = (iFlags & KBreaking);
			__SPIN_UNLOCK_IRQRESTORE(iLock, irq);

			// If we do need to delay the write
            if (iTurnaroundTransmitDelayed || iBreakDelayedTx)
                break;
			
			//
			InitiateWrite();
            break;
            }

        case RBusDevComm::ERequestNotifySignalChange:
            iNotifiedSignals = iSignals;
			DoSigNotify();
            break;
            
        case RBusDevComm::ERequestBreak:
			if(iTurnaroundTimerRunning)
				iTurnaroundBreakDelayed = ETrue;
			else
				BreakOn();
			break;

        }
    }

/**
 Called in DFC context upon receipt of ERequestRead
 */
void DChannelComm::InitiateRead(TInt aLength)
    {
    LOG(("InitiateRead(%d)", aLength));
    iRxOutstanding=EFalse;
	iRxOneOrMore=0;

    // Complete zero-length read immediately
    if (aLength==0)
        {
		iRxBufReq.Complete(iClient, KErrNone);
        RestartTurnaroundTimer();
        return;
        }

	TBool length_negative = (aLength<0);
	if (length_negative)
		aLength = -aLength;
	iRxBufReq.iLen=aLength;

    // If the RX buffer is empty, we must wait for more data
    TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
    if (iRxPutIndex==iRxGetIndex)
        {
		if (length_negative)
            iRxOneOrMore=-1;        // -1 because timer not started
        iRxOutstanding=ETrue;
        __SPIN_UNLOCK_IRQRESTORE(iLock, irq);
        return;
        }
    __SPIN_UNLOCK_IRQRESTORE(iLock, irq);

    // RX buffer contains characters, must scan buffer and then complete
	if (length_negative)
        {
        // ReceiveOneOrMore, up to -aLength characters
        iRxOneOrMore=1;
        }
    TInt getIndex=iRxGetIndex;
    TInt count=0;
    TUint stat=0;
	TBool complete=EFalse;
    while(!complete)
        {
        while(count<aLength && getIndex!=iRxPutIndex)
            {
            if ((stat=iRxErrorBuf[getIndex])!=0 || IsTerminator(iRxCharBuf[getIndex]))
                {
                // this character will complete the request
                if (++getIndex==iRxBufSize)
                    getIndex=0;
                count++;
                complete=ETrue;
                break;
                }
            if (++getIndex==iRxBufSize)
                getIndex=0;
            count++;
            }
        if (count==aLength)
            complete=ETrue;
        if (!complete)
            {
            TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
            if (getIndex==iRxPutIndex)
                {
                // not enough chars to complete request, so set up to wait for more
                iRxOutstanding=ETrue;
                __SPIN_UNLOCK_IRQRESTORE(iLock, irq);
                if (count)
                    DoDrainRxBuffer(getIndex);
                return;
                }
            // more characters have arrived, loop again
            __SPIN_UNLOCK_IRQRESTORE(iLock, irq);
            }
        }

    // can complete request right now
    TInt e=KErrNone;
    if (stat)
        {
        stat<<=24;
        if (stat & KReceiveIsrOverrunError)
            e = KErrCommsOverrun;
        else if (stat & KReceiveIsrBreakError)
	        e = KErrCommsBreak;
        else if (stat & KReceiveIsrFrameError)
            e = KErrCommsFrame;
        else if (stat & KReceiveIsrParityError)
            e = KErrCommsParity;
        }
    if (iRxError==KErrNone)
        iRxError=e;
    iRxBufCompleteIndex=getIndex;
    DoCompleteRx();
    }

/**
 Called in DFC context to start a write or a delayed write
 */
void DChannelComm::InitiateWrite()
    {
    LOG(("InitiateWrite() len=%d", iTxBufReq.iLen));


	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	iTxDesPos=0;
	iTurnaroundTimerStartTime = 0;
	iTurnaroundTimerStartTimeValid = 2;
	if (~iSignals & iFailSignals)
		{
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
		iTxBufReq.Complete(iClient, KErrCommsLineFail);
		return;
		}
	if (iTxBufReq.iLen==0)
		{
		if (iTxPutIndex==iTxGetIndex && (~iSignals & iHoldSignals)==0)
			{
			__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
			iTxBufReq.Complete(iClient, KErrNone);
			return;
			}
		}

	iTxOutstanding=ETrue;
	__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
	if (iTxBufReq.iLen!=0)
		DoFillTxBuffer();
	}

void DChannelComm::SigNotifyDfc(TAny* aPtr)
	{
	((DChannelComm*)aPtr)->DoSigNotify();
	}

void DChannelComm::DoSigNotify()
    {
	// Atomically update iNotifiedSignals and prepare to signal
	TBool do_notify = EFalse;
    TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
    TUint orig_sig=iNotifiedSignals;
    if (iSignalsReq->IsReady() && ( iNotifiedSignals==0xffffffff || ((iSignals^iNotifiedSignals)&iSigNotifyMask) ) )
        {
        iNotifiedSignals=iSignals;
        do_notify=ETrue;
        }
    __SPIN_UNLOCK_IRQRESTORE(iLock, irq);
    __KTRACE_OPT(KHARDWARE,Kern::Printf("CommSig: Orig=%08x New %08x Mask %08x",orig_sig,iNotifiedSignals,iSigNotifyMask));
    if (do_notify)
        {
        TUint changed=iSigNotifyMask;
        if (orig_sig!=0xffffffff)
            changed&=(orig_sig^iNotifiedSignals);
        changed=(changed<<12)|(iNotifiedSignals&iSigNotifyMask);

		// Write the result back to client memory and complete the request
		__KTRACE_OPT(KHARDWARE,Kern::Printf("CommSig: Notify %08x",changed));
		LOG(("DoSigNotify: %08x",changed));
		TUint& rr = iSignalsReq->Data();
		rr = changed;
		Kern::QueueRequestComplete(iClient, iSignalsReq, KErrNone);
		}
    }


/**
 Manually read and act on signals
 */
void DChannelComm::UpdateAndProcessSignals()
    {
    TUint signals=Signals();
    TBool notify=EFalse;
    TBool complete_rx=EFalse;
    TBool complete_tx=EFalse;
    TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
    iSignals=(iSignals&~KDTEInputSignals)|(signals&KDTEInputSignals);
    if (iSignalsReq->IsReady() && ((iSignals^iNotifiedSignals)&iSigNotifyMask) )
        {
        notify=ETrue;
        }
    if (IsLineFail(iFailSignals))
        {
        if (iRxOutstanding)
            {
            iRxError=KErrCommsLineFail;
            iRxBufCompleteIndex=iRxPutIndex;
            iRxOutstanding=EFalse;
            complete_rx=ETrue;
            }
        if (iTxOutstanding)
            {
            iTxError = KErrCommsLineFail;
            iTxOutstanding=EFalse;
            complete_tx=ETrue;
            }
        }
    //
    // Now we must determine if output is to be held
    //
    TUint status = ~iSignals & iHoldSignals;
    if (iOutputHeld & KXoffSignal)
        status |= KXoffSignal;      // Leave the xon/xoff handshake bit

    iOutputHeld=status;             // record new flow control state
    if (iTxGetIndex==iTxPutIndex)
        {
        // Tx buffer is empty
        if (iTxOutstanding && iTxBufReq.iLen==0 && (status&~KXoffSignal)==0)
            {
            // if hardware flow control released, complete zero-length write
            iTxOutstanding=EFalse;
            complete_tx=ETrue;
            }
        }
    else if (status==0)
        {
        // Tx buffer not empty and flow control released, so restart transmission
        EnableTransmit();
        }
    __SPIN_UNLOCK_IRQRESTORE(iLock, irq);
    if (notify)
        DoSigNotify();
    if (complete_rx)
        DoCompleteRx();
    if (complete_tx)
        DoCompleteTx();
    }


TUint DChannelComm::FailSignals(TUint aHandshake)
	{
	TUint r=0;
	if ((aHandshake&(KConfigObeyCTS|KConfigFailCTS))==(KConfigObeyCTS|KConfigFailCTS))
		r|=KSignalCTS;
	if ((aHandshake&(KConfigObeyDSR|KConfigFailDSR))==(KConfigObeyDSR|KConfigFailDSR))
		r|=KSignalDSR;
	if ((aHandshake&(KConfigObeyDCD|KConfigFailDCD))==(KConfigObeyDCD|KConfigFailDCD))
		r|=KSignalDCD;
	return r;
	}

TUint DChannelComm::HoldSignals(TUint aHandshake)
	{
	TUint r=0;
	if (aHandshake & KConfigObeyCTS)
		r|=KSignalCTS;
	if (aHandshake & KConfigObeyDSR)
		r|=KSignalDSR;
	if (aHandshake & KConfigObeyDCD)
		r|=KSignalDCD;
	return r;
	}

TUint DChannelComm::FlowControlSignals(TUint aHandshake)
	{
	TUint r=0;
	if (!(aHandshake & KConfigFreeRTS))
		r|=KSignalRTS;
	else if (!(aHandshake & KConfigFreeDTR))
		r|=KSignalDTR;
	return r;
	}

TUint DChannelComm::AutoSignals(TUint aHandshake)
	{
	TUint r=0;
	if (!(aHandshake & KConfigFreeRTS) && !(aHandshake & KConfigFreeDTR))
		r|=KSignalDTR;
	return r;
	}

TInt DChannelComm::SetConfig(TCommConfigV01& c)
	{
	LOG(("SetConfig(...)"));
	TBool restart = EFalse;
	TBool purge = EFalse;
	TBool changeTerminators=EFalse;
	TInt irq;
	TInt r;

	if(c.iTerminatorCount>KConfigMaxTerminators)
		return KErrNotSupported;
	if ((r=ValidateConfig(c))!=KErrNone)
		return r;
	TUint failSignals=FailSignals(c.iHandshake);
	if (IsLineFail(failSignals))
		return KErrCommsLineFail;
	if (iConfig.iRate != c.iRate
		|| iConfig.iDataBits != c.iDataBits
		|| iConfig.iStopBits != c.iStopBits
		|| iConfig.iParity != c.iParity
		|| iConfig.iFifo != c.iFifo
		|| iConfig.iSpecialRate != c.iSpecialRate
		|| iConfig.iSIREnable != c.iSIREnable
		|| iConfig.iSIRSettings != c.iSIRSettings)
		{
		restart = ETrue;
		}
	else if (iConfig.iParityErrorChar != c.iParityErrorChar
		|| iConfig.iParityError != c.iParityError
		|| iConfig.iXonChar != c.iXonChar
		|| iConfig.iXoffChar != c.iXoffChar
		|| (iConfig.iHandshake&(KConfigObeyXoff|KConfigSendXoff))
			!= (c.iHandshake&(KConfigObeyXoff|KConfigSendXoff)))
		{
		purge = ETrue;
		}
	else
		{
		if (iConfig.iTerminatorCount==c.iTerminatorCount)
			{
			for (TInt i=0; i<iConfig.iTerminatorCount; i++)
				{
				if (iConfig.iTerminator[i]!=c.iTerminator[i])
					{
					changeTerminators=ETrue;
					break;
					}
				}
			}
		else
			changeTerminators=ETrue;
		if (!changeTerminators && c.iHandshake == iConfig.iHandshake)
			return r;	// nothing to do.
		}
	if (iStatus==EActive && (restart || purge))
		{
		SetSignals(0,iFlowControlSignals|iAutoSignals); // Drop RTS
		Stop(EStopNormal);
		iStatus=EOpen;
		if(purge)
			ResetBuffers(ETrue);
		iConfig=c;
		iFailSignals=failSignals;
		iHoldSignals=HoldSignals(c.iHandshake);
		iFlowControlSignals=FlowControlSignals(c.iHandshake);
		iAutoSignals=AutoSignals(c.iHandshake);
		Start();
		CheckOutputHeld();
		SetSignals(iFlowControlSignals|iAutoSignals,0); // Assert RTS
		irq = __SPIN_LOCK_IRQSAVE(iLock);
		}
	else
		{
		irq = __SPIN_LOCK_IRQSAVE(iLock);
		if(purge)
			ResetBuffers(ETrue);
		iConfig=c;
		iFailSignals=failSignals;
		iHoldSignals=HoldSignals(c.iHandshake);
		iFlowControlSignals=FlowControlSignals(c.iHandshake);
		iAutoSignals=AutoSignals(c.iHandshake);
		}
	if (iConfig.iHandshake&KConfigObeyXoff)
		{
		iRxXonChar=c.iXonChar;
		iRxXoffChar=c.iXoffChar;
		}
	else
		{
		iRxXonChar=0xffffffff;
		iRxXoffChar=0xffffffff;
		iOutputHeld&=~KXoffSignal;
		}
	__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
	if (iStatus==EActive)
		ReleaseFlowControl();

	// no request pending here, so no need to protect this against interrupts
	if (restart || purge || changeTerminators)
		{
		memclr(iTerminatorMask, 32);
		TInt i;
		for (i=0; i<iConfig.iTerminatorCount; i++)
			{
			SetTerminator(iConfig.iTerminator[i]);
			}
		}
	return r;
	}

TInt DChannelComm::DoControl(TInt aFunction, TAny* a1, TAny* a2)
//
// Sync requests.
//
	{
	LOG(("DoControl(aFunction=%d, a1=%x, a2=%x)", aFunction, a1, a2));

	TInt r=KErrNone;

	switch (aFunction)
		{
		case RBusDevComm::EControlSetConfig:
			{
			TCommConfigV01 c;
			memclr(&c, sizeof(c));
			TPtr8 cfg((TUint8*)&c,0,sizeof(c));
			r=Kern::ThreadDesRead(iClient,a1,cfg,0,0);
			if (r==KErrNone)
				r=SetConfig(c);
			}
			Kern::UnpinVirtualMemory(iPinObjSetConfig);
			break;

		case RBusDevComm::EControlSignals:
			{
			UpdateAndProcessSignals();
			r=iSignals;
			break;
			}
		case RBusDevComm::EControlSetSignals:
			{
			TUint set=(TUint)a1;
			TUint clear=(TUint)a2;
			if (set & clear)
;//				Kern::PanicCurrentThread(_L("D32COMM"), ESetSignalsSetAndClear);
			else
				{
				if (iStatus==EOpen)
					{
					Start();
					if (!(iConfig.iHandshake & KConfigFreeDTR) && !(clear & KSignalDTR))
						set|=KSignalDTR; // Assert DTR
					if (!(iConfig.iHandshake & KConfigFreeRTS) && !(clear & KSignalRTS))
						set|=KSignalRTS; // Assert RTS
					if (iConfig.iHandshake & KConfigSendXoff)
						iJamChar=iConfig.iXonChar;
					iInputHeld = EFalse;
					CheckOutputHeld();
					}
				__e32_atomic_axo_ord32(&iSignals, ~(clear|set), set);
				SetSignals(set,clear);
				}
			break;
			}
		case RBusDevComm::EControlQueryReceiveBuffer:
			r=RxCount();
			break;
		case RBusDevComm::EControlResetBuffers:
			if (AreAnyPending())
;//				Kern::PanicCurrentThread(_L("D32COMM"), EResetBuffers);
			else
				ResetBuffers(ETrue);
			break;
		case RBusDevComm::EControlReceiveBufferLength:
			r=iRxBufSize;
			break;

		case RBusDevComm::EControlSetReceiveBufferLength:
			if (AreAnyPending())
;//				iThread->Panic(_L("D32COMM"),ESetReceiveBufferLength);
			else
				r=SetRxBufferSize((TInt)a1);
			break;
		// ***************************************

		case RBusDevComm::EControlMinTurnaroundTime:
			r = iTurnaroundMicroSeconds;			// used saved value
			break;

		case RBusDevComm::EControlSetMinTurnaroundTime:
				{
				if (a1<0)
					a1=(TAny*)0;
				iTurnaroundMicroSeconds = (TUint)a1;			// save this
				TUint newTurnaroundMilliSeconds = (TUint)a1/1000;	// convert to ms
				if(newTurnaroundMilliSeconds != iTurnaroundMinMilliSeconds)
					{
                    // POLICY: if a new turnaround time is set before the previous running timer has expired 
 					// then the timer is adjusted depending on the new value and if any
                    // write request has been queued, transmission will proceed after the timer has expired.
					if(iTurnaroundTimerStartTimeValid == 0)
						{	
						 iTurnaroundTimerStartTimeValid = 1;
						 iTurnaroundTimerStartTime = NKern::TickCount();
						}
				    if(iTurnaroundTimerStartTimeValid != 2)
						TurnaroundClear();
					if(newTurnaroundMilliSeconds > 0)
						{
						r = TurnaroundSet(newTurnaroundMilliSeconds);
						}
					}
				}
			break;
		default:
			r=KErrNotSupported;
			}
		return(r);
		}

void DChannelComm::DoPowerUp()
//
// Called at switch on and upon Opening.
//
    {
	LOG(("DoPowerUp()"));
	__KTRACE_OPT(KPOWER,Kern::Printf("DChannelComm::DoPowerUp()"));

	ResetBuffers(ETrue);
	iRxOutstanding=EFalse;
	iNotifyData=EFalse;
	iTxOutstanding=EFalse;
    iTxDesPos=0;
    iFlags=0;

	// Cancel turnaround
	iTurnaroundMinMilliSeconds = 0;
	iTurnaroundTimerRunning = EFalse;
	iTurnaroundTransmitDelayed = EFalse;

	// cancel any DFCs/timers
	iRxDrainDfc.Cancel();
	iRxCompleteDfc.Cancel();
	iTxFillDfc.Cancel();
	iTxCompleteDfc.Cancel();
	iTimer.Cancel();
	iTurnaroundTimer.Cancel();
	iTurnaroundDfc.Cancel();
	iTimerDfc.Cancel();
	iSigNotifyDfc.Cancel();

	Complete(EAll, KErrAbort);
	if (!Kern::PowerGood())
		return;
	TUint hand=iConfig.iHandshake;
	if (hand&(KConfigFreeRTS|KConfigFreeDTR))
		{
		Start();
		if (!Kern::PowerGood())
			return;
		if (hand&KConfigFreeRTS)
			{
			if (iSignals&KSignalRTS)
				SetSignals(KSignalRTS,0);
			else
				SetSignals(0,KSignalRTS);
			}
		if (!Kern::PowerGood())
			return;
		if (hand&KConfigFreeDTR)
			{
			if (iSignals&KSignalDTR)
				SetSignals(KSignalDTR,0);
			else
				SetSignals(0,KSignalDTR);
			}
		CheckOutputHeld();
		}
	else
		{
		if (iStatus==EActive)
			iStatus=EOpen;
		}
	}

void DChannelComm::PowerUpDfc(TAny* aPtr)
	{
	
	DChannelComm* d = (DChannelComm*)aPtr;
	__PM_ASSERT(d->iStandby);
	if (d->iStatus != EClosed)
		d->DoPowerUp();
	else
		// There is racing Close(): driver was already closed (ECloseMsg) but the DPowerHandler was not destroyed yet.
		{}
	d->iStandby = EFalse;
	d->iPowerHandler->PowerUpDone();
	if (d->iMsgHeld)
		{
		__PM_ASSERT(d->iStatus != EClosed);
		d->iMsgHeld = EFalse;
		d->HandleMsg(d->iMsgQ.iMessage);
		}
	}

void DChannelComm::PowerDownDfc(TAny* aPtr)
	{
	DChannelComm* d = (DChannelComm*)aPtr;
	__PM_ASSERT(!d->iStandby);
	d->iStandby = ETrue;
	if (d->iStatus != EClosed)
		d->Shutdown();
	else
		// There is racing Close(): driver was already closed (ECloseMsg) but the DPowerHandler was not destroyed yet.
		{}
	d->iPowerHandler->PowerDownDone();
	}

DCommPowerHandler::DCommPowerHandler(DChannelComm* aChannel)
	:	DPowerHandler(KLddName), 
		iChannel(aChannel)
	{
	}

void DCommPowerHandler::PowerUp()
	{
	iChannel->iPowerUpDfc.Enque();
	}

void DCommPowerHandler::PowerDown(TPowerState)
	{
	iChannel->iPowerDownDfc.Enque();
	}

void DChannelComm::FinishBreak(TAny* aSelf)
	{
	DChannelComm* self = (DChannelComm*)aSelf;
	self->QueueFinishBreakDfc();
	}

void DChannelComm::QueueFinishBreakDfc()
	{
	iBreakDfc.Enque();
	}
	
	
void DChannelComm::FinishBreakDfc(TAny* aSelf)
	{
	DChannelComm* self = (DChannelComm*)aSelf;
	self->FinishBreakImplementation(KErrNone);
	}

void DChannelComm::FinishBreakImplementation(TInt aError)
	{
	if (iStatus==EClosed)
		{
		Complete(EBreak, KErrNotReady);
		}
	else
		{
		BreakOff();
		Complete(EBreak, aError);
		}

	// re-setup transmission if needed, for writes after a break
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	if (iBreakDelayedTx)
		{
		iBreakDelayedTx = EFalse;		// protected -> prevent reentrant ISR
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);

		RestartDelayedTransmission();
		}
	else
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
	}
void DChannelComm::RestartDelayedTransmission()
	{
	LOG(("RestartDelayedTransmission()"));
	TInt irq = __SPIN_LOCK_IRQSAVE(iLock);
	TBool completeTx=EFalse;
	
	iBreakDelayedTx = EFalse;		// protected -> prevent reentrant ISR
	__SPIN_UNLOCK_IRQRESTORE(iLock, irq);

	if (iStatus==EClosed)
		{
		irq = __SPIN_LOCK_IRQSAVE(iLock);
		iTxError = KErrNotReady;		// protected -> changed in signals ISR
		completeTx = ETrue;
		}

	else if(IsLineFail(iFailSignals))	// have signals changed in the meantime?
		{
		irq = __SPIN_LOCK_IRQSAVE(iLock);
		iTxError = KErrCommsLineFail;	// protected -> changed in signals ISR
		completeTx = ETrue;
		}

	else
		{
		InitiateWrite();
		}


	if(completeTx)
		{
		iTxError = KErrNone;
		__SPIN_UNLOCK_IRQRESTORE(iLock, irq);
		Complete(ETx, iTxError);
		}
	}

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
// e32\drivers\ethernet\d_ethernet.cpp
// LDD the ethernet which has no power managment and is not PCCard based
// 
//

/**
 @file d_ethernet.cpp
*/

#include <drivers/ethernet.h>
#include <kernel/kern_priv.h>
#include <e32hal.h>
#include <e32uid.h>


_LIT(KLddName,"Ethernet");

#ifdef KNETWORK1
const TUint8 ETHER2_TYPE_IP_MSB = 0x08;
const TUint8 ETHER2_TYPE_IP_LSB = 0x00;
const TUint8 IP_TYPE_TCP        = 0x06;

static TBool IsTcp(TDesC8 &aFrame)
	{
	return (aFrame[12] == ETHER2_TYPE_IP_MSB && aFrame[13] == ETHER2_TYPE_IP_LSB && aFrame[23] == IP_TYPE_TCP);
	}

static TInt GetTcpSeqNumber(TDesC8 &aFrame)
	{
	TInt seqNum = 0;
	if (IsTcp(aFrame))
		seqNum = aFrame[38] << 24 | aFrame[39] << 16 | aFrame[40] << 8| aFrame[41];
	return seqNum;
	}

static TInt GetTcpAckNumber(TDesC8 &aFrame)
	{
	TInt ackNum = 0;
	if (IsTcp(aFrame))
		ackNum = aFrame[42] << 24 | aFrame[43] << 16 | aFrame[44] << 8| aFrame[45];
	return ackNum;
	}
#endif

DDeviceEthernet::DDeviceEthernet()
// Constructor
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DDeviceEthernet::DDeviceEthernet()"));
    iParseMask=KDeviceAllowAll;
    iUnitsMask=0xffffffff; // Leave units decision to the PDD
    iVersion=TVersion(KEthernetMajorVersionNumber,
		      KEthernetMinorVersionNumber,
		      KEthernetBuildVersionNumber);
    }


TInt DDeviceEthernet::Install()
// Install the device driver.
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DDeviceEthernet::Install()"));
    return(SetName(&KLddName));
    }


void DDeviceEthernet::GetCaps(TDes8& aDes) const
// Return the Ethernet capabilities.
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DDeviceEthernet::GetCaps(TDes8& aDes) const"));
    TPckgBuf<TCapsDevEthernetV01> b;
    b().version=TVersion(KEthernetMajorVersionNumber,
			 KEthernetMinorVersionNumber,
			 KEthernetBuildVersionNumber);
    Kern::InfoCopy(aDes,b);
    }


TInt DDeviceEthernet::Create(DLogicalChannelBase*& aChannel)
// Create a channel on the device.
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DDeviceEthernet::Create(DLogicalChannelBase*& aChannel)"));
    aChannel=new DChannelEthernet;

    return aChannel?KErrNone:KErrNoMemory;
    }


DChannelEthernet::DChannelEthernet()
// Constructor
    :   iRxCompleteDfc(CompleteRxDfc, this, 2)
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DChannelEthernet()"));

    // Setup the default config
    iConfig.iEthSpeed      = KEthSpeed10BaseT;
    iConfig.iEthDuplex     = KEthDuplexHalf;
    iConfig.iEthAddress[0] = 0x00;
    iConfig.iEthAddress[1] = 0x00;
    iConfig.iEthAddress[2] = 0x00;
    iConfig.iEthAddress[3] = 0x00;
    iConfig.iEthAddress[4] = 0x00;
    iConfig.iEthAddress[5] = 0x00;

    iStatus=EOpen;
    
    iClient=&Kern::CurrentThread();
	// Increse the Dthread's ref count so that it does not close without us
	iClient->Open();
    }


DChannelEthernet::~DChannelEthernet()
// Destructor
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::~DChannelEthernet()"));
	Kern::DestroyClientRequest(iWriteRequest);
	Kern::DestroyClientBufferRequest(iReadRequest);
	// decrement it's reference count
	Kern::SafeClose((DObject*&)iClient, NULL);
    }


void DChannelEthernet::Complete(TInt aMask, TInt aReason)
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::Complete(TInt aMask, TInt aReason)"));
    if (aMask & ERx)
		{
    	__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::Complete iReadRequest"));
		if (iReadRequest->IsReady())
			{
			Kern::QueueBufferRequestComplete(iClient, iReadRequest, aReason);
			}
		}
    if (aMask & ETx)
		{
    	__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::Complete iWriteRequest"));
		__KTRACE_OPT(KNETWORK2, Kern::Printf(" >ldd tx: PRE complete reason=%d iClient=%08x iTxStatus=%08x\n", aReason, iClient, iWriteRequest->StatusPtr()));
		Kern::QueueRequestComplete(iClient, iWriteRequest, aReason);
		}
	}


void DChannelEthernet::Shutdown()
    {
    __KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf("DChannelEthernet::Shutdown()"));
	
	// Suspend the chip and disable interrupts
	(static_cast<DEthernet*>(iPdd))->Stop(EStopNormal);
	
	Complete(EAll, KErrAbort);
    
	if (iRxCompleteDfc.Queued())
		{
    	__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::Shutdown()- Cancelling old dfc"));
		// Ethernet interrupts are disabled; must make sure DFCs are not queued.
	    iRxCompleteDfc.Cancel();
		}

	// No harm in doing this
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::Shutdown()- Completing message"));
    iMsgQ.iMessage->Complete(KErrNone,EFalse);
    }


TInt DChannelEthernet::DoCreate(TInt aUnit, 
                                const TDesC8* /*anInfo*/, 
                                const TVersion &aVer)
// Create the channel from the passed info.
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer)"));
	
	if(!Kern::CurrentThreadHasCapability(ECapabilityCommDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by ENET.LDD (Ethernet Driver)")))
		return KErrPermissionDenied;
    
	if (!Kern::QueryVersionSupported(TVersion(KEthernetMajorVersionNumber,
					      KEthernetMinorVersionNumber,
					      KEthernetBuildVersionNumber),
				     aVer))
		return KErrNotSupported;

	TInt r = Kern::CreateClientBufferRequest(iReadRequest, 1, TClientBufferRequest::EPinVirtual);
	if (r != KErrNone)
		return r;

	r = Kern::CreateClientRequest(iWriteRequest);
	if (r != KErrNone)
		return r;

	SetDfcQ(((DEthernet*)iPdd)->DfcQ(aUnit));
	iRxCompleteDfc.SetDfcQ(iDfcQ);
    iMsgQ.Receive();
        
    ((DEthernet *)iPdd)->iLdd=this;

    //Get config from the device
    ((DEthernet *)iPdd)->GetConfig(iConfig);

    PddCheckConfig(iConfig);
    return KErrNone;
    }


void DChannelEthernet::Start()
// Start the driver receiving.
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::Start()"));
    if (iStatus!=EClosed)
		{
		PddStart();
		iStatus=EActive;
		}
    }


void DChannelEthernet::ReceiveIsr()
	// Copies data into the iFifo's buffers
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::ReceiveIsr()"));
    TBuf8<KMaxEthernetPacket+32> * buffer;
//	TInt err;

    buffer = iFIFO.GetFree();
    if(buffer == NULL)
		{
	    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::ReceiveIsr()- Dropping a frame"));
		/*err =*/ PddReceive(*buffer, EFalse); //Need to call as must drain RX buffer
		// Should something be done about returned errors?
		return;
		}
    else
		/*err =*/ PddReceive(*buffer, ETrue);
		// Should something be done about returned errors?

	// Add another DFc as we have a buffer and is not already queued
	if (!iRxCompleteDfc.Queued())
		{
        if (NKern::CurrentContext()==NKern::EInterrupt)
 			iRxCompleteDfc.Add();
 		else
 			iRxCompleteDfc.Enque();
		return;
		}

	__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::ReceiveIsr()- DFC already added"));
    }


void DChannelEthernet::CompleteRxDfc(TAny* aPtr)
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::CompleteRxDfc(TAny* aPtr)"));
    DChannelEthernet *pC=(DChannelEthernet*)aPtr;
    pC->DoCompleteRx();
    }


void DChannelEthernet::DoCompleteRx()
    {
	__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoCompleteRx()"));
	__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd isr triggered..."));


	if (iReadRequest->IsReady())
    	{
		__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoCompleteRx()- Read waiting"));
		TBuf8<KMaxEthernetPacket+32> * buffer;

		buffer = iFIFO.GetNext();
		if (buffer == NULL)
			{
			__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoCompleteRx()- Unexpected read request earlier"));
			__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd isr - miss = empty"));
			Complete(ERx, KErrNone);
			return;	
			}

		// RX buffer has data, must scan buffer and then complete
		TInt r = Kern::ThreadBufWrite(iClient, iClientReadBuffer, *(const TDesC8*)buffer, 0, KChunkShiftBy0, iClient);
		iFIFO.SetNext();
		__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd isr - hit = rx tcp seq=%u ack=%u\n", GetTcpSeqNumber(*buffer), GetTcpAckNumber(*buffer)) );
		Complete(ERx, r);
		} 
	else
		{
		__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd isr - skipped! no request pending\n"));
		}

    }

//Override sendMsg to allow data copy in the context of client thread for WDP.
TInt DChannelEthernet::SendMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = m.iValue;
	TInt r = KErrNone;
	if (id != (TInt)ECloseMsg && id != KMaxTInt)
		{
		if (id<0)
			{
			TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
			r = SendRequest(aMsg);
			if (r != KErrNone)
				{	
				Kern::RequestComplete(pS,r);
				}
			}
		else
			{
			r = SendControl(aMsg);
			}
		}
	else
		{
		r = DLogicalChannel::SendMsg(aMsg);
		}
	return r;
	}


TInt DChannelEthernet::SendControl(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = m.iValue;
	TInt bufLen;
	TInt bufMaxLen;
	TEthernetConfigV01 config;
	TEthernetCaps caps;
	TAny* a1 = m.Ptr0();
	TInt r = KErrNone;
	switch(id)
		{
		case RBusDevEthernet::EControlConfig:
			{

			__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::SendControl EControlConfig"));
			Kern::KUDesInfo(*(const TDesC8*)a1, bufLen, bufMaxLen);
			if((TUint)bufMaxLen < sizeof(TEthernetConfigV01))
				{
				return KErrUnderflow;
				}
			m.iArg[0] = &config;
			break;
			}
		case RBusDevEthernet::EControlCaps:
			{
			__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::SendControl EControlCaps"));
			Kern::KUDesInfo(*(const TDesC8*)a1, bufLen, bufMaxLen);
			if((TUint)bufMaxLen < sizeof(caps))
				{
				return KErrUnderflow;
				}
			m.iArg[0] = &caps;
			break;
			}
		case RBusDevEthernet::EControlSetConfig:
		case RBusDevEthernet::EControlSetMac:
			{
			__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::SendControl EControlSetConfig"));
			Kern::KUDesInfo(*(const TDesC8*)a1, bufLen, bufMaxLen);
			if((TUint)bufLen > sizeof(config))
				{
				return KErrOverflow;
				}
			memset(&config,0,sizeof(config));
			TPtr8 cfg((TUint8*)&config,0,sizeof(config));
			Kern::KUDesGet(*(TDes8*)&cfg, *(const TDesC8*)a1);
			m.iArg[0] = (TAny*)&config;
			break;
			}
		default:
			return KErrNotSupported;
		}
	r = DLogicalChannel::SendMsg(aMsg);
	if(r != KErrNone)
		return r;
	switch(id)
		{
		case RBusDevEthernet::EControlConfig:
			{
			Kern::InfoCopy(*(TDes8*)a1, (const TUint8*)&config, sizeof(TEthernetConfigV01));
			break;
			}
		case RBusDevEthernet::EControlCaps:
			{
			Kern::KUDesPut(*(TDes8*)a1, (const TDesC8&)caps);
			break;
			}
		}
	return r;
	}

TInt DChannelEthernet::SendRequest(TMessageBase* aMsg)
	{
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = ~m.iValue;
	TRequestStatus* pS = (TRequestStatus*)m.Ptr0();
	TAny* a1 = m.Ptr1();
	TAny* a2 = m.Ptr2();
	TInt r = KErrNone;
	switch(id)
		{
		case RBusDevEthernet::ERequestWrite:
			{
			if(iStatus == EClosed)
				{
				return KErrNotReady;
				}
			TInt len;
			umemget32(&len, a2, sizeof(len));
			if(len == 0)
				{	
				return KErrNone;
				}
			if(!a1)
				{
				return KErrArgument;
				}
			Kern::KUDesGet((TDes8&)iFIFO.iTxBuf, *(const TDesC8*)a1);
			iFIFO.iTxBuf.SetLength(len);
			iWriteRequest->SetStatus(pS);
			break;
			}
		case RBusDevEthernet::ERequestRead:
			{
			TInt len;
			TInt bufLen;
			TInt bufMaxLen;
			umemget32(&len, a2, sizeof(len));
			if(len == 0)
				{
				return KErrNone;
				}
			Kern::KUDesInfo(*(const TDesC8*)a1, bufLen, bufMaxLen);
			if(bufMaxLen < Abs(len))
				{	
				return KErrGeneral;
				}
			if(bufLen != 0)
				{
				Kern::KUDesSetLength(*(TDes8*)a1, 0);
				}
			r = iReadRequest->StartSetup(pS);
			if (r != KErrNone)
				{
				return r;
				}
			r = iReadRequest->AddBuffer(iClientReadBuffer, a1);
			if (r != KErrNone)
				{
				return KErrNoMemory;
				}
			iReadRequest->EndSetup();
			break;
			}
#ifdef ETH_CHIP_IO_ENABLED
		case RBusDevEthernet::EChipDiagIOCtrl:
			{
			Kern::KUDesGet((TDes8&)iChipInfo, *(const TDesC8*)a1);
			break;
			}
#endif
		default:
			return KErrNotSupported;
		}
	r = DLogicalChannel::SendMsg(aMsg);
#ifdef ETH_CHIP_IO_ENABLED
	if(r == KErrNone)
		{
		switch(id)
			{
			case RBusDevEthernet::EChipDiagIOCtrl:
				{
				Kern::KUDesPut(*(TDes8*)a1, (const TDesC8&)iChipInfo);
				Kern::RequestComplete(pS,r);
				break;
				}
			}
		}
#endif
	return r;
	}

void DChannelEthernet::HandleMsg(TMessageBase* aMsg)
    {
    TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;
    
    __KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd: DChannelEthernet::HandleMsg(TMessageBase* aMsg) id=%d\n", id));
	
	if (id==(TInt)ECloseMsg)
		{
		Shutdown();
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
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r = DoRequest(~id, pS, m.Ptr1(), m.Ptr2());
		//Kern::Printf(" >ldd: about to complete TThreadMessage id=%d...\n", id);
		m.Complete(r,ETrue);
		}
    else
		{
		// DoControl
		__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd: do control id=%d...\n", id));
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}


void DChannelEthernet::DoCancel(TInt aMask)
// Cancel an outstanding request.
    {
	__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoCancel(TInt aMask = %d)", aMask));

	if (aMask & RBusDevEthernet::ERequestReadCancel)
		{
		if (iRxCompleteDfc.Queued())
			{
    		__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf("DChannelEthernet::DoCancel()- Cancelling old dfc"));
			iRxCompleteDfc.Cancel();
			}
		Complete(ERx,KErrCancel);
		}

	if (aMask & RBusDevEthernet::ERequestWriteCancel)
		{
		__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf("> ldd: DChannelEthernet::DoCancel - Completing write cancel\n"));
		Complete(ETx,KErrCancel);
		}
    }


TInt DChannelEthernet::DoRequest(TInt aReqNo, TRequestStatus* /*aStatus*/, TAny* a1, TAny* /*a2*/)
// Async requests.
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)"));
    // First check if we have started
    if (iStatus==EOpen)
		{
		Start();
		}

    // Now we can dispatch the request
    TInt r=KErrNone;
    TInt len=0;
    switch (aReqNo)
		{
		case RBusDevEthernet::ERequestRead:
			InitiateRead(a1,len);
		    break;
	    
		case RBusDevEthernet::ERequestWrite:
		    __KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd tx: RBusDevEthernet::ERequestWrite..."));
			InitiateWrite(a1,len);
		    break;
#ifdef ETH_CHIP_IO_ENABLED
        case RBusDevEthernet::EChipDiagIOCtrl:
            {
			r=((DEthernet*)iPdd)->BgeChipIOCtrl(iChipInfo); 
            break;
            }
#endif             
		}
	    return r;
    }


void DChannelEthernet::InitiateRead(TAny* /*aRxDes*/, TInt aLength)
    {
	__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::InitiateRead(TAny *aRxDes, TInt aLength)"));
	__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd client triggered read...\n"));
    iRxLength=Abs(aLength);
    TBuf8<KMaxEthernetPacket+32> * next;
    next = iFIFO.GetNext();
    if (next == NULL)
 		{
		__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::InitiateRead - RX buffer empty"));
		__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd client - miss = rx buf empty...\n"));
	 	return;
 		}
    
    iFIFO.SetNext();
    // RX buffer has data, must scan buffer and then complete
	TInt r = Kern::ThreadBufWrite(iClient, iClientReadBuffer, *(const TDesC8*)next, 0, KChunkShiftBy0, iClient);
	__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd client - hit = rx tcp seq=%u ack=%d\n", GetTcpSeqNumber(*next), GetTcpAckNumber(*next)) );
    Complete(ERx,r);
    }


void DChannelEthernet::InitiateWrite(TAny* /*aTxDes*/, TInt /*aLength*/)
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::InitiateWrite(TAny *aTxDes, TInt aLength)"));
	__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >ldd tx: tcp seq=%u ack=%u\n", GetTcpSeqNumber(iFIFO.iTxBuf), GetTcpAckNumber(iFIFO.iTxBuf)) );
	if(PddSend(iFIFO.iTxBuf) != KErrNone)
		Complete(ETx, KErrCommsLineFail);
	else
		Complete(ETx, KErrNone);
    }


TInt DChannelEthernet::SetConfig(TEthernetConfigV01& c)
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::SetConfig(TEthernetConfigV01& c)"));
    TInt r;

	if ((r=ValidateConfig(c))!=KErrNone)
		return r;    

    iConfig = c;

    PddConfigure(iConfig);

    return r;
    }


TInt DChannelEthernet::SetMAC(TEthernetConfigV01& c)
    {
    __KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::SetMAC(TEthernetConfigV01& c)"));
    TInt r;

    if ((r=ValidateConfig(c))!=KErrNone)
		return r;    

    iConfig = c;

    MacConfigure(iConfig);
    PddConfigure(iConfig);

    return r;
    }


TInt DChannelEthernet::DoControl(TInt aFunction, TAny* a1, TAny* /*a2*/)
// Sync requests.
    {
    __KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf("DChannelEthernet::DoControl(TInt aFunction, TAny* a1, TAny* a2)") );
	TInt r=KErrNone;

    switch (aFunction)
		{
		case RBusDevEthernet::EControlConfig:
			{
			__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoControl EControlConfig"));
			*(TEthernetConfigV01*)a1 = iConfig;
			break;
			}
		case RBusDevEthernet::EControlSetConfig:
			{
			__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoControl EControlSetConfig"));
	    	r=SetConfig(*(TEthernetConfigV01*)a1);
			break;
			}
		case RBusDevEthernet::EControlSetMac:
			{
			__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoControl EControlSetMac"));
			r = SetMAC(*(TEthernetConfigV01*)a1);
			break;
			}
		case RBusDevEthernet::EControlCaps:
			{
			__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoControl EControlCaps"));
			TEthernetCaps capsBuf;
			PddCaps(capsBuf);
			*(TEthernetCaps*)a1 = capsBuf;
			break;
			}
		default:
			{
			__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernet::DoControl default 0x%x", aFunction));
			r=KErrNotSupported;
			}
		}
	    return(r);
    }

// Implementation of driver Rx / Tx FIFO class.
DChannelEthernetFIFO::DChannelEthernetFIFO()
    {
    iRxQueIterNext = 0;
    iRxQueIterFree = 0;
    iNumFree = KNumRXBuffers;
    }

DChannelEthernetFIFO::~DChannelEthernetFIFO()
    {
    }

TBuf8<KMaxEthernetPacket+32> * DChannelEthernetFIFO::GetFree()
    // Return location of current Rx FIFO free element
    {
    if(iNumFree == 0)
		{
		__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf("DChannelEthernetFIFO::GetFree()- No free free buffers"));
		return NULL;
		}

    iNumFree--;

	TBuf8<KMaxEthernetPacket+32> &rxBuf = iRxBuf[iRxQueIterFree++];
    if(iRxQueIterFree == KNumRXBuffers)
		{
		iRxQueIterFree = 0;
		__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernetFIFO::GetFree()- free wrap return 0"));
		}
    
	__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernetFIFO::GetFree()- free return %d", iRxQueIterFree));
	return &rxBuf;
    }


TBuf8<KMaxEthernetPacket+32> * DChannelEthernetFIFO::GetNext()
    // Return location of next data element within Rx FIFO
    {
	__KTRACE_OPT2(KNETWORK1, KNETWORK2, Kern::Printf(" >LDDx: GetNext() - iNumFree=%d iRxQueIterNext(read)=%d iRxQueIterFree(write)=%d", iNumFree, iRxQueIterNext, iRxQueIterFree));

    if(iNumFree == KNumRXBuffers)
		{
		__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernetFIFO::GetNext()- No data waiting"));
		return NULL;
		}

	TBuf8<KMaxEthernetPacket+32> &rxBuf = iRxBuf[iRxQueIterNext++];
    if(iRxQueIterNext == KNumRXBuffers)
		{
		__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernetFIFO::GetNext()- Wrap next return 0"));
		iRxQueIterNext = 0;
		}
    
	__KTRACE_OPT(KNETWORK1, Kern::Printf("DChannelEthernetFIFO::GetNext()- Data found return %d", iRxQueIterNext));
	return &rxBuf;
    }

void DChannelEthernetFIFO::SetNext()
    // Increment location of next data element within Rx FIFO
    {
	// locked since iNumFree is decremented in function which could be called
 	// from interrupt context
	// DENNIS: SOUNDS DODGY
    __e32_atomic_add_ord32(&iNumFree, 1);
    }

/** @addtogroup enet Ethernet Drivers
 *  Kernel Ethernet Support
 */

/** @addtogroup enet_ldd Driver LDD's
 * @ingroup enet
 */

/**
 * @addtogroup enet_ldd_nopm_nopccard Ethernet LDD Not PCMCIA and No Power Managment
 * @ingroup enet_ldd
 * @{
 */

/**
 * Real entry point from the Kernel: return a new driver
 */
DECLARE_STANDARD_LDD()
	{
	return new DDeviceEthernet;
	}


/** @} */ // end of assabet group





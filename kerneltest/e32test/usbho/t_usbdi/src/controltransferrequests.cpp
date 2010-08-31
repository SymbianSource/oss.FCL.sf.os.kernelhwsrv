// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// @file controltransferrequests.cpp
// @internalComponent
// 
//

#include "controltransferrequests.h"
#include "testdebug.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "controltransferrequestsTraces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
	
CEp0Transfer::CEp0Transfer(RUsbInterface& aInterface0)
:	CActive(EPriorityStandard),
	iUsbInterface0(aInterface0),
	iDataRequest(EFalse)
	{
	OstTraceFunctionEntryExt( CEP0TRANSFER_CEP0TRANSFER_ENTRY, this );
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CEP0TRANSFER_CEP0TRANSFER_EXIT, this );
	}
	
	
CEp0Transfer::~CEp0Transfer()
	{
	OstTraceFunctionEntry1( CEP0TRANSFER_CEP0TRANSFER_ENTRY_DUP01, this );
	Cancel();
	OstTraceFunctionExit1( CEP0TRANSFER_CEP0TRANSFER_EXIT_DUP01, this );
	}
	
	
void CEp0Transfer::DoCancel()
	{
	OstTraceFunctionEntry1( CEP0TRANSFER_DOCANCEL_ENTRY, this );
	// Pretend cancel
	TRequestStatus* s = &iStatus;
	User::RequestComplete(s,KErrCancel);
	OstTraceFunctionExit1( CEP0TRANSFER_DOCANCEL_EXIT, this );
	}

void CEp0Transfer::CancelSendRequest()
	{
	OstTraceFunctionEntry1( CEP0TRANSFER_CANCELSENDREQUEST_ENTRY, this );
	// Pretend cancel
	iUsbInterface0.CancelEP0Transfer();
	OstTraceFunctionExit1( CEP0TRANSFER_CANCELSENDREQUEST_EXIT, this );
	}


void CEp0Transfer::SendRequest(TEmptyRequest& aSetupPacket,MCommandObserver* aObserver) 
	{
	OstTraceFunctionEntryExt( CEP0TRANSFER_SENDREQUEST_ENTRY, this );
	iObserver = aObserver;	
	
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST, "bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP01, "bRequest     : 0x%02x",aSetupPacket.iRequest);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP02, "wValue       : 0x%04x",aSetupPacket.iValue);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP03, "wIndex       : 0x%04x",aSetupPacket.iIndex);
	OstTrace0(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP04, "total sent   : 8");
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,KNullDesC8,iTemp,iStatus);
	iRequestTime.HomeTime();
	SetActive();	
	OstTraceFunctionExit1( CEP0TRANSFER_SENDREQUEST_EXIT, this );
	}


void CEp0Transfer::SendRequest(TDataSendRequest& aSetupPacket,MCommandObserver* aObserver)
	{
	OstTraceFunctionEntryExt( CEP0TRANSFER_SENDREQUEST_ENTRY_DUP01, this );
	iObserver = aObserver;
	
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP10, "bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP11, "bRequest     : 0x%02x",aSetupPacket.iRequest);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP12, "wValue       : 0x%04x",aSetupPacket.iValue);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP13, "wIndex       : 0x%04x",aSetupPacket.iIndex);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP14, "data length  : %d",aSetupPacket.iSendData.Length());
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP15, "total sent   : %d",8+aSetupPacket.iSendData.Length());
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,aSetupPacket.iSendData,iTemp,iStatus);
	iRequestTime.HomeTime();
	SetActive();
	OstTraceFunctionExit1( CEP0TRANSFER_SENDREQUEST_EXIT_DUP01, this );
	}
	
void CEp0Transfer::SendRequest(TWriteSynchronousCachedReadDataRequest& aSetupPacket,MCommandObserver* aObserver)
	{
	OstTraceFunctionEntryExt( CEP0TRANSFER_SENDREQUEST_ENTRY_DUP02, this );
	iObserver = aObserver;
	
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP20, "bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP21, "bRequest     : 0x%02x",aSetupPacket.iRequest);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP22, "wValue       : 0x%04x",aSetupPacket.iValue);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP23, "wIndex       : 0x%04x",aSetupPacket.iIndex);
	OstTrace0(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP24, "total sent   : 8");
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,KNullDesC8,iTemp,iStatus);
	iRequestTime.HomeTime();
	SetActive();
	OstTraceFunctionExit1( CEP0TRANSFER_SENDREQUEST_EXIT_DUP02, this );
	}


void CEp0Transfer::SendRequest(TEndpointReadRequest& aSetupPacket,MCommandObserver* aObserver)
	{
	OstTraceFunctionEntryExt( CEP0TRANSFER_SENDREQUEST_ENTRY_DUP03, this );
	iObserver = aObserver;
	
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP30, "bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP31, "bRequest     : 0x%02x",aSetupPacket.iRequest);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP32, "wValue       : 0x%04x",aSetupPacket.iValue);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP33, "wIndex       : 0x%04x",aSetupPacket.iIndex);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP34, "data length  : %d",aSetupPacket.iReadSpecificationData.Length());
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP35, "total sent   : %d",8+aSetupPacket.iReadSpecificationData.Length());
	TLex8 lex(aSetupPacket.iReadSpecificationData);
	TUint numBytes = 0;
	lex.Val(numBytes, EDecimal);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP06, "Read length required (in bytes)   : %d",numBytes);
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,aSetupPacket.iReadSpecificationData,iTemp,iStatus);
	iRequestTime.HomeTime();
	SetActive();
	OstTraceFunctionExit1( CEP0TRANSFER_SENDREQUEST_EXIT_DUP03, this );
	}
	
void CEp0Transfer::SendRequest(TClassDataSendRequest& aSetupPacket,MCommandObserver* aObserver)
	{
	OstTraceFunctionEntryExt( CEP0TRANSFER_SENDREQUEST_ENTRY_DUP04, this );
	iObserver = aObserver;
	
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP40, "bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP41, "bRequest     : 0x%02x",aSetupPacket.iRequest);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP42, "wValue       : 0x%04x",aSetupPacket.iValue);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP43, "wIndex       : 0x%04x",aSetupPacket.iIndex);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP44, "data length  : %d",aSetupPacket.iSendData.Length());
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP45, "total sent   : %d",8+aSetupPacket.iSendData.Length());
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,aSetupPacket.iSendData,iTemp,iStatus);
	iRequestTime.HomeTime();
	SetActive();
	OstTraceFunctionExit1( CEP0TRANSFER_SENDREQUEST_EXIT_DUP04, this );
	}
	
void CEp0Transfer::SendRequest(TDataRecvRequest& aSetupPacket,MCommandObserver* aObserver)
	{
	OstTraceFunctionEntryExt( CEP0TRANSFER_SENDREQUEST_ENTRY_DUP05, this );
	iObserver = aObserver;
	
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP50, "bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP51, "bRequest     : 0x%02x",aSetupPacket.iRequest);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP52, "wValue       : 0x%04x",aSetupPacket.iValue);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP53, "wIndex       : 0x%04x",aSetupPacket.iIndex);
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_SENDREQUEST_DUP54, "data length  : %d",aSetupPacket.iRecvData.Length());
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,KNullDesC8,aSetupPacket.iRecvData,iStatus);
	iRequestTime.HomeTime();
	SetActive();
	OstTraceFunctionExit1( CEP0TRANSFER_SENDREQUEST_EXIT_DUP05, this );
	}


void CEp0Transfer::RunL()
	{
	OstTraceFunctionEntry1( CEP0TRANSFER_RUNL_ENTRY, this );
	iCompletionTime.HomeTime();
	TInt completionCode(iStatus.Int());
	
	OstTrace1(TRACE_NORMAL, CEP0TRANSFER_RUNL, "Client command sent, Completion code: %d",completionCode);
	iObserver->Ep0TransferCompleteL(completionCode);
	OstTraceFunctionExit1( CEP0TRANSFER_RUNL_EXIT, this );
	}
	
	
TInt CEp0Transfer::RunError(TInt aError)
	{
	OstTraceFunctionEntryExt( CEP0TRANSFER_RUNERROR_ENTRY, this );
	OstTraceFunctionExitExt( CEP0TRANSFER_RUNERROR_EXIT, this, KErrNone );
	return KErrNone;
	}

void CEp0Transfer::LastRequestStartTime( TTime& aDuration)
	{
	OstTraceFunctionEntryExt( CEP0TRANSFER_LASTREQUESTSTARTTIME_ENTRY, this );
	aDuration= iRequestTime.Int64();
	OstTraceFunctionExit1( CEP0TRANSFER_LASTREQUESTSTARTTIME_EXIT, this );
	}

void CEp0Transfer::LastRequestCompletionTime( TTime& aDuration)
	{
	OstTraceFunctionEntryExt( CEP0TRANSFER_LASTREQUESTCOMPLETIONTIME_ENTRY, this );
	aDuration= iCompletionTime.Int64();
	OstTraceFunctionExit1( CEP0TRANSFER_LASTREQUESTCOMPLETIONTIME_EXIT, this );
	}
	
	}



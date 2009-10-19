// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

namespace NUnitTesting_USBDI
	{
	
	
CEp0Transfer::CEp0Transfer(RUsbInterface& aInterface0)
:	CActive(EPriorityStandard),
	iUsbInterface0(aInterface0),
	iDataRequest(EFalse)
	{
	CActiveScheduler::Add(this);
	}
	
	
CEp0Transfer::~CEp0Transfer()
	{
	LOG_FUNC
	Cancel();
	}
	
	
void CEp0Transfer::DoCancel()
	{
	// Pretend cancel
	TRequestStatus* s = &iStatus;
	User::RequestComplete(s,KErrCancel);
	}

void CEp0Transfer::CancelSendRequest()
	{
	// Pretend cancel
	iUsbInterface0.CancelEP0Transfer();
	}


void CEp0Transfer::SendRequest(TEmptyRequest& aSetupPacket,MCommandObserver* aObserver) 
	{
	LOG_FUNC
	iObserver = aObserver;	
	
	RDebug::Printf("bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	RDebug::Printf("bRequest     : 0x%02x",aSetupPacket.iRequest);
	RDebug::Printf("wValue       : 0x%04x",aSetupPacket.iValue);
	RDebug::Printf("wIndex       : 0x%04x",aSetupPacket.iIndex);
	RDebug::Printf("total sent   : 8");
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,KNullDesC8,iTemp,iStatus);
	iRequestTime.HomeTime();
	SetActive();	
	}


void CEp0Transfer::SendRequest(TDataSendRequest& aSetupPacket,MCommandObserver* aObserver)
	{
	LOG_FUNC
	iObserver = aObserver;
	
	RDebug::Printf("bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	RDebug::Printf("bRequest     : 0x%02x",aSetupPacket.iRequest);
	RDebug::Printf("wValue       : 0x%04x",aSetupPacket.iValue);
	RDebug::Printf("wIndex       : 0x%04x",aSetupPacket.iIndex);
	RDebug::Printf("data length  : %d",aSetupPacket.iSendData.Length());
	RDebug::Printf("total sent   : %d",8+aSetupPacket.iSendData.Length());
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,aSetupPacket.iSendData,iTemp,iStatus);
	iRequestTime.HomeTime();
	SetActive();
	}
	
void CEp0Transfer::SendRequest(TWriteSynchronousCachedReadDataRequest& aSetupPacket,MCommandObserver* aObserver)
	{
	LOG_FUNC
	iObserver = aObserver;
	
	RDebug::Printf("bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	RDebug::Printf("bRequest     : 0x%02x",aSetupPacket.iRequest);
	RDebug::Printf("wValue       : 0x%04x",aSetupPacket.iValue);
	RDebug::Printf("wIndex       : 0x%04x",aSetupPacket.iIndex);
	RDebug::Printf("total sent   : 8");
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,KNullDesC8,iTemp,iStatus);
	iRequestTime.HomeTime();
	SetActive();
	}


void CEp0Transfer::SendRequest(TEndpointReadRequest& aSetupPacket,MCommandObserver* aObserver)
	{
	LOG_FUNC
	iObserver = aObserver;
	
	RDebug::Printf("bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	RDebug::Printf("bRequest     : 0x%02x",aSetupPacket.iRequest);
	RDebug::Printf("wValue       : 0x%04x",aSetupPacket.iValue);
	RDebug::Printf("wIndex       : 0x%04x",aSetupPacket.iIndex);
	RDebug::Printf("data length  : %d",aSetupPacket.iReadSpecificationData.Length());
	RDebug::Printf("total sent   : %d",8+aSetupPacket.iReadSpecificationData.Length());
	TLex8 lex(aSetupPacket.iReadSpecificationData);
	TUint numBytes = 0;
	lex.Val(numBytes, EDecimal);
	RDebug::Printf("Read length required (in bytes)   : %d",numBytes);
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,aSetupPacket.iReadSpecificationData,iTemp,iStatus);
	iRequestTime.HomeTime();
	SetActive();
	}
	
void CEp0Transfer::SendRequest(TClassDataSendRequest& aSetupPacket,MCommandObserver* aObserver)
	{
	LOG_FUNC
	iObserver = aObserver;
	
	RDebug::Printf("bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	RDebug::Printf("bRequest     : 0x%02x",aSetupPacket.iRequest);
	RDebug::Printf("wValue       : 0x%04x",aSetupPacket.iValue);
	RDebug::Printf("wIndex       : 0x%04x",aSetupPacket.iIndex);
	RDebug::Printf("data length  : %d",aSetupPacket.iSendData.Length());
	RDebug::Printf("total sent   : %d",8+aSetupPacket.iSendData.Length());
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,aSetupPacket.iSendData,iTemp,iStatus);
	iRequestTime.HomeTime();
	SetActive();
	}
	
void CEp0Transfer::SendRequest(TDataRecvRequest& aSetupPacket,MCommandObserver* aObserver)
	{
	LOG_FUNC
	iObserver = aObserver;
	
	RDebug::Printf("bmRequestType: 0x%02x",aSetupPacket.iRequestType);
	RDebug::Printf("bRequest     : 0x%02x",aSetupPacket.iRequest);
	RDebug::Printf("wValue       : 0x%04x",aSetupPacket.iValue);
	RDebug::Printf("wIndex       : 0x%04x",aSetupPacket.iIndex);
	RDebug::Printf("data length  : %d",aSetupPacket.iRecvData.Length());
	
	iUsbInterface0.Ep0Transfer(aSetupPacket,KNullDesC8,aSetupPacket.iRecvData,iStatus);
	iRequestTime.HomeTime();
	SetActive();
	}


void CEp0Transfer::RunL()
	{
	iCompletionTime.HomeTime();
	LOG_FUNC
	TInt completionCode(iStatus.Int());
	
	RDebug::Printf("Client command sent, Completion code: %d",completionCode);
	iObserver->Ep0TransferCompleteL(completionCode);
	}
	
	
TInt CEp0Transfer::RunError(TInt aError)
	{
	LOG_FUNC
	return KErrNone;
	}

void CEp0Transfer::LastRequestStartTime( TTime& aDuration)
	{
	aDuration= iRequestTime.Int64();
	}

void CEp0Transfer::LastRequestCompletionTime( TTime& aDuration)
	{
	aDuration= iCompletionTime.Int64();
	}
	
	}



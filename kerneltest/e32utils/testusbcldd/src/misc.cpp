// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\testusbcldd\src\misc.cpp
// 
//

#include "dtestusblogdev.h"

TUsbcInterface::TUsbcInterface(TUsbcInterfaceSet* aIfcSet, TUint8 aSetting)
	: iEndpoints(2), iInterfaceSet(aIfcSet), iSettingCode(aSetting)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcInterface::TUsbcInterface()")));
	}


TUsbcInterface::~TUsbcInterface()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcInterface::~TUsbcInterface()")));
	iEndpoints.ResetAndDestroy();
	}


TUsbcInterfaceSet::TUsbcInterfaceSet(const DBase* aClientId, TUint8 aIfcNum)
	: iInterfaces(2), iClientId(aClientId), iInterfaceNumber(aIfcNum), iCurrentInterface(0)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcInterfaceSet::TUsbcInterfaceSet()")));
	}


TUsbcInterfaceSet::~TUsbcInterfaceSet()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcInterfaceSet::~TUsbcInterfaceSet()")));
	iInterfaces.ResetAndDestroy();
	}
	
const TUsbcInterface* TUsbcInterfaceSet::CurrentInterface() const
/** Returns a pointer to the currently selected (active) setting of this interface.

	@return A pointer to the currently selected (active) setting of this interface.
*/
	{
	return iInterfaces[iCurrentInterface];
	}


TUsbcInterface* TUsbcInterfaceSet::CurrentInterface()
/** Returns a pointer to the currently selected (active) setting of this interface.

	@return A pointer to the currently selected (active) setting of this interface.
*/
	{
	return iInterfaces[iCurrentInterface];
	}

TUsbcLogicalEndpoint::TUsbcLogicalEndpoint(TUint aEndpointNum, const TUsbcEndpointInfo& aInfo,
										   TUsbcInterface* aInterface)
	: iLEndpointNum(aEndpointNum), iInfo(aInfo), iInterface(aInterface)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcLogicalEndpoint::TUsbcLogicalEndpoint()")));
	}


TUsbcLogicalEndpoint::~TUsbcLogicalEndpoint()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("TUsbcLogicalEndpoint::~TUsbcLogicalEndpoint: #%d"), iLEndpointNum));
	}

DTestUsbcEndpoint::DTestUsbcEndpoint()
	{
	}
	
DTestUsbcEndpoint::~DTestUsbcEndpoint()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("Deleting buffer 0x%x"), iBuffer));
	delete iBuffer;
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("Buffer deleted")));
	}
	
TInt DTestUsbcEndpoint::Create(const TUsbcEndpointCaps& aCaps)
	{
	iCaps = aCaps;
	if (iBuffer == NULL)
		{
		__NEWPLATBUF(iBuffer, KEndpointBufferSize);
		if (iBuffer == NULL)
			return KErrNoMemory;
		}
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("Allocated buffer 0x%x"), iBuffer));
	return KErrNone;
	}

void DTestUsbcEndpoint::DoCancel()
	{
	if (iRequestPending)
		{
		//Cancel client request
		iRequestPending = EFalse;
		Kern::RequestComplete(iClient, iClientStatus, KErrCancel);
		}
	if (iHostRequestPending)
		{
		//Cancel host request
		iHostRequestPending = EFalse;
		Kern::RequestComplete(iHost, iHostStatus, KErrCancel);
		}
	}

TInt DTestUsbcEndpoint::NewRequest(DThread* aClient, TRequestStatus* aStatus, 
								   TEndpointTransferInfo& aInfo, TTransferType aType)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DTestUsbcEndpoint::NewRequest")));
	//Only supports one request pending per endpoint
	if (iRequestPending)
		{
		return ERequestAlreadyPending;
		}
	iClient = aClient;
	iClientStatus = aStatus;
	iClientTransferInfo = aInfo;
	iDataTransferred = 0;
	iRequestPending = ETrue;
	iRequestType = aType;
	
	//Copy data to local buffer if this is a write request
	if (iRequestType == ETransferTypeWrite)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("Write request")));
		TInt err;		
		__THREADREADPLATBUF(aClient, iClientTransferInfo.iDes, iBuffer, err);
		if (err != KErrNone)
			{
			iRequestPending = EFalse;
			return err;
			}
		}
	else
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("Read request")));
		}
	
	if (iHostRequestPending)
		{
		TryToComplete();
		}
	
	return KErrNone;
	}

TInt DTestUsbcEndpoint::NewHostRequest(DThread* aHost, TRequestStatus* aStatus,
									   TEndpointTransferInfo& aInfo, TTransferType aType)
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DTestUsbcEndpoint::NewHostRequest")));
	//Only supports one request pending per endpoint
	if (iHostRequestPending)
		{
		return ERequestAlreadyPending;
		}
	iHost = aHost;
	iHostStatus = aStatus;
	iHostTransferInfo = aInfo;
	iHostDataTransferred = 0;
	iHostRequestPending = ETrue;
	iHostRequestType = aType;
	
	//Copy data to local buffer if this is a write request
	if (iHostRequestType == ETransferTypeWrite)
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("Write request")));
		TInt err;		
		__THREADREADPLATBUF(aHost, iHostTransferInfo.iDes, iBuffer, err);
		if (err != KErrNone)
			{
			iRequestPending = EFalse;
			return err;
			}
		}
	else
		{
		__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("Read request")));
		}
		
	if (iRequestPending)
		{
		TryToComplete();
		}
	
	return KErrNone;
	}

TInt DTestUsbcEndpoint::TryToComplete()
	{
	__KTRACE_OPT(KUSB, Kern::Printf(__KSTRING("DTestUsbcEndpoint::TryToComplete")));
    TInt err = KErrNone;
	TInt len = iHostTransferInfo.iTransferSize - iHostDataTransferred;
	
	if (SupportsDir(KUsbEpDirBidirect))
		{
		//Make sure host and client transfer types don't conflict
		if (iRequestType == iHostRequestType)
			{
			iRequestPending = EFalse;
			iHostRequestPending = EFalse;
			Kern::RequestComplete(iClient, iClientStatus, KErrUsbEpBadDirection);
			Kern::RequestComplete(iHost, iHostStatus, KErrUsbEpBadDirection);
			return KErrUsbEpBadDirection;
			}
		}
		
	if (SupportsDir(KUsbEpDirIn) || (SupportsDir(KUsbEpDirBidirect) && iRequestType == ETransferTypeWrite))
		{
		err = CopyData(iDataTransferred, iHost, iHostTransferInfo.iDes, iHostDataTransferred, len);
		}
	else if (SupportsDir(KUsbEpDirOut) || (SupportsDir(KUsbEpDirBidirect) && iRequestType == ETransferTypeReadData))
		{
		err = CopyData(iHostDataTransferred, iClient, iClientTransferInfo.iDes, iDataTransferred, len);
		}
	else
		{
		err = KErrNotSupported;
		}
	
	if (err != KErrNone)
		{
		//Problems copying data. Complete requests with error and return.
		iRequestPending = EFalse;
		iHostRequestPending = EFalse;
		Kern::RequestComplete(iClient, iClientStatus, err);
		Kern::RequestComplete(iHost, iHostStatus, err);
		return err;
		}
	iDataTransferred += len;
	iHostDataTransferred += len;
	
    iRequestPending = EFalse;
    Kern::RequestComplete(iClient, iClientStatus, KErrNone);
    iHostRequestPending = EFalse;
    Kern::RequestComplete(iHost, iHostStatus, KErrNone);
	return KErrNone;
	}

/**
Copies data from a source descriptor to a destination descriptor, both in user space.

@param aSrcClient The thread that owns the source descriptor
@param aSrc Pointer to the source descriptor
@param aSrcOffset Offset in aSrc from where to start reading data
@param aDestClient The thread that owns the destination descriptor
@param aDest Pointer to the destination descriptor
@param aDestOffset Offset in aDest from where to start writing data
@param aLen Amount of bytes to copy
@return KErrNone is successful, otherwise a standard Symbian error code
*/
TInt DTestUsbcEndpoint::CopyData(TInt aSrcOffset, DThread* aDestClient, TDesC8* aDest,
								 TInt aDestOffset, TInt aLen)
	{	
	TInt err;
	
	// Get the descriptor length in the client's context.
	TInt rxLen[2] = {0,0}; 
	err=Kern::ThreadRawRead(aDestClient,aDest,&rxLen,sizeof(rxLen));
	if (err!=KErrNone)
		return err; 
	
	// copy mo more than max number of chars in receive buffer
	aLen = Min(aLen, rxLen[1]);
	
	while(aLen > 0)
		{
		TInt len = iBuffer->Length() - aSrcOffset;
		//Make sure we only copy aLen bytes, no more
		if (len > aLen)
			{
			len = aLen;
			}
		TPtrC8 src(iBuffer->Ptr() + aSrcOffset, len);
		err = __THREADWRITEOFFSET(aDestClient, aDest, src, aDestOffset);
		if (err != KErrNone)
			{
			return err;
			}
		aLen -= len;
		aSrcOffset += len;
		aDestOffset += len;
		}
	
	return KErrNone;
	}

TInt DTestUsbcEndpoint::Halt()
	{
	iHalted = ETrue;
	if (iNotifyHost != NULL)
		{
		Kern::RequestComplete(iNotifyHost, iHostNotifyStatus, KErrNone);
		iNotifyHost = NULL;
		}
	return KErrNone;
	}
	
TInt DTestUsbcEndpoint::Clear()
	{
	iHalted = EFalse;
	if (iRequestPending)
		{
		iRequestPending = EFalse;
		Kern::RequestComplete(iClient, iClientStatus, KErrNone);
		}
	if (iHostRequestPending)
		{
		iHostRequestPending = EFalse;
		Kern::RequestComplete(iHost, iHostStatus, KErrNone);
		}
	if (iClearCallback != NULL)
		{
		iClearCallback->EndpointStatusNotifyCallback();
		}
	return KErrNone;
	}
	
void DTestUsbcEndpoint::SetClearCallback(DLddTestUsbcChannel* aCallback)
	{
	iClearCallback = aCallback;
	}
	
TBool DTestUsbcEndpoint::IsHalted()
	{
	return iHalted;
	}

TInt DTestUsbcEndpoint::HostStatusNotify(DThread* aHost, TRequestStatus* aStatus)
	{
	const TRequestStatus s(KRequestPending);
	__THREADRAWWRITE(aHost, aStatus, (TUint8*)&s, (TInt)sizeof(TRequestStatus));
	iNotifyHost = aHost;
	iHostNotifyStatus = aStatus;
	return KErrNone;
	}
	
TBool DTestUsbcEndpoint::SupportsDir(TUint aDir)
	{
	if ((iCaps.iTypesAndDir & aDir) == aDir)
		{
		return ETrue;
		}
	return EFalse;
	}
	
TBool DTestUsbcEndpoint::EndpointSuitable(const TUsbcEndpointInfo& aInfo)
	{
	return (!iReserve &&
			(iCaps.iSizes == (TUint)aInfo.iSize) &&
			((iCaps.iTypesAndDir & aInfo.iDir) == aInfo.iDir) &&
			((iCaps.iTypesAndDir & aInfo.iType) == aInfo.iType));
	}
	

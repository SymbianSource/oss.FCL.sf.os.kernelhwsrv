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
//

/**
 @file
 @internalTechnology
*/

#ifndef CBULKONLYTRANSPORT_H
#define CBULKONLYTRANSPORT_H

class CUsbInterfaceHandler;

/**
The Bulk Only Transport implementation
*/
class CBulkOnlyTransport : public CBase, public MTransport
    {
public:
	static CBulkOnlyTransport* NewL(TUint aInterfaceId);
private:
	CBulkOnlyTransport();
	~CBulkOnlyTransport();
    void ConstructL(TUint aInterfaceId);

public:
    void SetLun(TLun aLun);

    TInt SendDataTxCmdL(const MClientCommandServiceReq* aCommand,
                        TDesC8& aData,
                        TUint aPos,
                        TInt& aLen);

    TInt SendControlCmdL(const MClientCommandServiceReq* aCommand);

    TInt SendControlCmdL(const MClientCommandServiceReq* aReq,
                         MClientCommandServiceResp* aResp);

    TInt SendDataRxCmdL(const MClientCommandServiceReq* aCommand,
                        TDes8& aData,
                        TInt& aLen);

	void Suspend(TRequestStatus &aStatus);
	void Resume();

private:
	TInt Reset();
	void UnInitialiseTransport();
	TInt InitialiseTransport();
	TInt GetEndpointAddress(RUsbInterface& aUsbInterface,
                            TInt aInterfaceSetting,
                            TUint8 aTransferType,
                            TUint8 aDirection,
                            TInt& aEndpointAddress);

	void DoResetRecovery();
	void GetMaxLun(TLun* aReceiveData, const RMessage2& aMessage);
    void SendCbwL(const MClientCommandServiceReq* aReq,
                  TBotCbw::TCbwDirection aDirection,
                  TUint32 aTransferLength);

    void ReceiveCswL();
    TInt ProcessZeroTransferL();
    TInt ProcessInTransferL(TUint32& aDataReceived);
    TInt ProcessOutTransferL(TUint32& aDataReceived);

private:
    static const TInt KCbwPacketSize = 31;
    static const TInt KResponsePacketSize = 1024  * 128;
    // Though the csw valid size is 13, we do try to read 64 in order to validate
    // the device's operation
    static const TInt KCswPacketSize = 64;

private:
    TLun iLun;

	RUsbBulkTransferDescriptor iBulkOutCbwTd;
	RUsbBulkTransferDescriptor iBulkDataTd;
	RUsbBulkTransferDescriptor iBulkInCswTd;
	RUsbPipe iBulkPipeOut;
	RUsbPipe iBulkPipeIn;

	RUsbInterface iInterface;
	TRequestStatus iStatus;
    TBotCbw iCbw;
    TUint32 iCbwTag;
	TBotCsw iCsw;
	CUsbInterfaceHandler* iUsbInterfaceHandler;
    };


#ifdef MASSSTORAGE_PUBLISHER
class TMsPublisher
    {
public:
    static const TInt KMyUid = 0x10285B2E;

    enum TPropertyKeys
        {
        KBotResetProperty = 1,
        KStallProperty = 2
        };


    TMsPublisher(TPropertyKeys aKey);
    };

#endif

#endif // CBULKONLYTRANSPORT_H


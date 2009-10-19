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

#ifndef MTRANSPORT_H
#define MTRANSPORT_H

class MClientCommandServiceReq;
class MClientCommandServiceResp;

/**
Interface class to the device transport layer
*/
class MTransport
    {
public:
    /** Reset the device */
    virtual TInt Reset() = 0;

    /**
    Assign a LUN to this transport instance

    @param iLun The LUN value
    */
    virtual void SetLun(TLun iLun) = 0;

    /**
    Asynchronous request to get the Maximum LUN supported by the device,
    intended to be called from a CActive class.

    @param aMaxLun MaxLun returned by the device
    @param aMessage

    @return TInt KErrNone if successful or KErrGeneral if the device fails
    */
	virtual void GetMaxLun(TLun* aMaxLun,
                           const RMessage2& aMessage) = 0;

    /**
    Send a command to the device with a data package. The transport layer
    encodes the command fields via the encoding function supplied by the
    MClientCommandServiceReq interface.

    @param aCommand The command
    @param aData The buffer containing the data
    @param aPos Offset in the buffer to the data to be copied
    @param aLen [IN] The number of bytes to be sent, [OUT] on return the number
    of bytes actually transferred.

    @return TInt KErrNone if successful
    */
    virtual TInt SendDataTxCmdL(const MClientCommandServiceReq* aCommand,
                                TDesC8& aData,
                                TUint aPos,
                                TInt& aLen) = 0;

    /**
    Send a command to the device. The transport layer encodes the command fields
    via the encoding function supplied by the MClientCommandServiceReq
    interface.  The function leaves if the device response is not compliant with
    the protocol standard.

    @param aCommand The command

    @return TInt KErrNone if successful otherwise KErrCommandFailed to indicate
    a device status error
    */
    virtual TInt SendControlCmdL(const MClientCommandServiceReq* aCommand) = 0;

    /**
    Send a command to the device and receive a response. The transport layer
    encodes the command fields via the encoding function supplied by the
    MClientCommandServiceReq interface. The transport layer also decodes the
    response into the aResp object using the MClientCommandServiceResp decoding
    method. The function leaves if the device response is not compliant with the
    protocol standard.

    @param aReq [IN] The request object
    @param aResp [OUT] The response object

    @return TInt KErrNone if successful otherwise KErrCommandFailed to indicate
    a device status error
    */
    virtual TInt SendControlCmdL(const MClientCommandServiceReq* aReq,
                                 MClientCommandServiceResp* aResp) = 0;

    /**
    Send a command to the device and receive a data package. The transport layer
    encodes the command fields via the encoding function supplied by the
    MClientCommandServiceReq interface.  The function leaves if the device
    response is not compliant with the protocol standard.

    @param aCommand The command
    @param aData The buffer to copy the data into
    @param aLen [IN] Expected length of data to receive [OUT] actual length of
    data

    @return TInt KErrNone if successful otherwise KErrCommandFailed to indicate
    a device status error
    */
    virtual TInt SendDataRxCmdL(const MClientCommandServiceReq* aCommand,
                                TDes8& aData,
                                TInt& aLen) = 0;

    /**
    Suspend the USB Host interface to the mass storage device
	*/
	virtual void Suspend(TRequestStatus &aStatus) = 0;

    /**
    Resume the USB Host interface to the mass storage device
	*/
	virtual void Resume() = 0;

    /** Destructor */
    virtual ~MTransport() {};
    };

#endif // MTRANSPORT_H


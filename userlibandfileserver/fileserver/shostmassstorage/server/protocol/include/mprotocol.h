// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// protocol.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef MPROTOCOL_H
#define MPROTOCOL_H

class MTransport;
struct TCapsInfo;

/**
Interface class to control a Mass Storage device
*/
class MProtocol
    {
public:
    /** Command to initialise LUN */
    virtual void InitialiseUnitL() = 0;

    /** Command to uninitialise the LUN */
    virtual void UninitialiseUnitL() = 0;

    /**
    Command to read the media.

    @param aPos
    @param aLen
    @param aCopybuf
    */
    virtual void ReadL(TUint64 aPos, TDes8& aCopybuf, TInt aLen) = 0;

    /**
    Command to write to the media.

    @param aPos
    @param aLen
    @param aCopybuf
    */
    virtual void WriteL(TUint64 aPos, TDesC8& aCopybuf, TInt aLen) = 0;

    /**
    command to find the capacity of the media

    @param aCopybuf
    */
    virtual void GetCapacityL(TCapsInfo& aCapsInfo) = 0;

    /** unit testing */
    virtual void CreateSbcInterfaceL(TUint32 aBlockLen, TUint32 aLastLba) = 0;

	virtual TBool DoScsiReadyCheckEventL() = 0;

    /**
    Media change notification

    @param aMessage
    */
	virtual	void NotifyChange(const RMessage2& aMessage) = 0;

    /**
    Force notification to be sent
    */
    virtual void ForceCompleteNotifyChangeL() = 0;
    /**
    Force notification to be sent
    */
    virtual void CancelChangeNotifierL() = 0;
    /**
    Suspends the logical unit
    */
	virtual void SuspendL() = 0;
    /**
    Resumes the logical unit
    */
	virtual void ResumeL() = 0;
    /**
    Resets the media change and finalisation request status check timer
    */
	virtual TBool IsConnected() = 0;

    /** Destructor */
    virtual ~MProtocol();
    };

inline MProtocol::~MProtocol()
    {
    }

/**
Interface class to encode SCSI request into a byte stream
*/
class MClientCommandServiceReq
    {
public:
    /**
    Encode the command data into a byte stream.

    @param aBuffer Buffer to place the encoded data into
    @return TInt Length of the encoded data
    */
    virtual TInt EncodeRequestL(TDes8& aBuffer) const = 0;
    };


/**
Interface class to decode a SCSI response from byte stream into corresponding
class
*/
class MClientCommandServiceResp
    {
public:
    /**
    Returns the length of the RESPONSE data stream.

    @return TInt Length in bytes
    */
    virtual TInt DataLength() const = 0;

    /**
    Decode data into RESPONSE structure of implementation class.

    @param aPtr Descriptor (byte stream) containing the data to be decoded
    */
    virtual void DecodeL(const TDesC8& aPtr) = 0;
    };


#endif // MPROTOCOL_H


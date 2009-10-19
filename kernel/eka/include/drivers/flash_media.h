// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\flash_media.h
// 
//

/**
 @file
 @internalComponent
*/

#ifndef __FLASH_MEDIA_H__
#define __FLASH_MEDIA_H__
#include <drivers/locmedia.h>
#include <platform.h>

GLREF_C TDfcQue FlashDfcQ;




/** 
@publishedPartner
@released

Base class for the LFFS media driver.

The class provides an implementation for the generic layer of
the LFFS media driver.
Code that is specifc to a flash device is implemented as a class
derived from this.
*/
class DMediaDriverFlash : public DMediaDriver
	{
public:
    /**
    Defines a set of values that are passed to Complete()
    informing the generic layer about the type of request
    that is being completed.
    */
	enum TRequest {
	               /**
	               Indicates that a read request is being completed.
	               */
	               EReqRead=0,
           	       
           	       /**
	               Indicates that a write request is being completed.
	               */
	               EReqWrite=1,
	               
           	       /**
	               Indicates that an erase request is being completed.
	               */
	               EReqErase=2};
public:
    /**
    Creates an instance of the LFFS media driver.
    
    Although declared in this class, this function is not implemented by
    Symbian OS, but must be implemented by the port.
    
    It should return an instance of the class derived from DMediaDriverFlash.
    The following is an example taken from the Lubbock reference platform:
    @code
    DMediaDriverFlash* DMediaDriverFlash::New(TInt aMediaId)
    {
    return new DMediaDriverFlashLA(aMediaId);
    }
    @endcode
    
    @param  aMediaId The unique media ID specifed when the media driver was registered.
                     This value is just propagated through.
    
    @return An instance of a class derived from DMediaDriverFlash
    */
	static DMediaDriverFlash* New(TInt aMediaId);
	
	DMediaDriverFlash(TInt aMediaId);
public:
	// replacing pure virtual
       
	virtual TInt Request(TLocDrvRequest& aRequest);

	virtual TInt PartitionInfo(TPartitionInfo& anInfo);
    
	virtual void NotifyPowerDown();

	virtual void NotifyEmergencyPowerDown();

public:
	// pure virtual - FLASH device specific stuff

    /** 
    Called by the generic layer of the LFFS media driver, and implemented by
    the specific layer to perform any initialisation required by
    the specific layer.
    
    @return KErrNone, if successful; otherwise one of the other system
            wide error codes.
    */
	virtual TInt Initialise()=0;



    /** 
    Called by the generic layer of the LFFS media driver, and implemented by
    the specific layer to get the size of the flash erase block.
    
    @return The size of the flash erase block, in bytes.
    */
	virtual TUint32 EraseBlockSize()=0;



    /** 
    Called by the generic layer of the LFFS media driver, and implemented by
    the specific layer to get the total size of the flash.

    @return The total size of the flash, in bytes.
    */
	virtual TUint32 TotalSize()=0;



    /** 
    Called by the generic layer of the LFFS media driver, and implemented by
    the specific layer to start a read request.
    
    The information for the request is in iReadReq.
    
    If the read operation cannot be started immediately, then this function
    should return KMediaDriverDeferRequest; this asks the generic layer
    to defer the request and re-start it later. A read request might be
    deferred, for example, if there is a write or an erase operation
    already in progress.
    
    Note that this an asynchronous request, i.e. it starts the operation;
    the driver must call:
    @code
    Complete(EReqRead, result);
    @endcode
    when the operation is complete, where result is KErrNone if sucessful, or
    one of the system-wide error codes, otherwise.

    @return KErrNone, if the request has been sucessfully initiated;
            KErrNotSupported, if the request cannot be handled by the device;
            KMediaDriverDeferRequest, if the request cannot be handled
            immediately because of an outstanding request

    @see iReadReq
    @see DMediaDriverFlash::Complete()
    */
	virtual TInt DoRead()=0;



    /** 
    Called by the generic layer of the LFFS media driver, and implemented by
    the specific layer to start a write request.

    The information for the request is in iWriteReq.
    
    If the write operation cannot be started immediately, then this function
    should return KMediaDriverDeferRequest; this asks the generic layer
    to defer the request and re-start it later. A write request might be
    deferred, for example, if there is a read or an erase operation
    already in progress.
    
    Note that this an asynchronous request, i.e. it starts the operation;
    the driver must call:
    @code
    Complete(EReqWrite, result);
    @endcode
    when the operation is complete, where result is KErrNone if sucessful, or
    one of the system-wide error codes, otherwise.

    @return KErrNone, if the request has been sucessfully initiated;
            KErrNotSupported, if the request cannot be handled by the device;
            KMediaDriverDeferRequest, if the request cannot be handled
            immediately because of an outstanding request

    @see iWriteReq
    @see DMediaDriverFlash::Complete()
    */
	virtual TInt DoWrite()=0;
    
    
    
    /** 
    Called by the generic layer of the LFFS media driver, and implemented by
    the specific layer to start a block erase request.

    The information for the request is in iEraseReq.
    
    If the erase operation cannot be started immediately, then this function
    should return KMediaDriverDeferRequest; this asks the generic layer
    to defer the request and re-start it later. An erase request might be
    deferred, for example, if there is a read or a write operation
    already in progress.
    
    Note that this an asynchronous request, i.e. it starts the operation;
    the driver must call:
    @code
    Complete(EReqErase, result);
    @endcode
    when the operation is complete, where result is KErrNone if sucessful, or
    one of the system-wide error codes, otherwise.

    @return KErrNone, if the request has been sucessfully initiated;
            KErrNotSupported, if the request cannot be handled by the device;
            KMediaDriverDeferRequest, if the request cannot be handled
            immediately because of an outstanding request

    @see iEraseReq
    @see DMediaDriverFlash::Complete()
    */
	virtual TInt DoErase()=0;
public:
	TInt DoCreate(TInt aMediaId);

	virtual TInt Caps(TLocalDriveCapsV2& aCaps);

	void Complete(TInt aRequest, TInt aResult);
public:


    /**
    Location for request information.
    
    An array of three entries to contain request information for read,
    write and erase requests respectively.
    
    NB Do not access this array directly; instead use the macro definitions:
    iReadReq, iWriteReq and iEraseReq. These ensure that you access the correct
    TLocDrvRequest items within the array.
    
    @see iReadReq
    @see iWriteReq
    @see iEraseReq
    @see TLocDrvRequest
    */
	TLocDrvRequest* iRequests[3];
	};
  
  
  
  
/** 
Read request information for LFFS media drivers.

@see TLocDrvRequest
@see DMediaDriverFlash
@see DMediaDriverFlash::iRequests
*/
#define iReadReq	iRequests[EReqRead]




/** 
Write request information for LFFS media drivers.

@see TLocDrvRequest
@see DMediaDriverFlash
@see DMediaDriverFlash::iRequests
*/
#define iWriteReq	iRequests[EReqWrite]




/** 
Erase request information for LFFS media drivers.

@see TLocDrvRequest
@see DMediaDriverFlash
@see DMediaDriverFlash::iRequests
*/
#define iEraseReq	iRequests[EReqErase]



#endif

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
// e32\drivers\medlfs\flash_media.cpp
// 
//

#include <drivers/flash_media.h>
#include "variantmediadef.h"

_LIT(KPddName, "Media.Flash");
_LIT(KFlashThreadName,"FlashThread");

const TInt KFlashThreadPriority=24;	// same as file server

GLDEF_C TDfcQue FlashDfcQ;

class DPhysicalDeviceMediaFlash : public DPhysicalDevice
	{
public:
	DPhysicalDeviceMediaFlash();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aMediaId, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aDeviceType, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Info(TInt aFunction, TAny* a1);
	};
								
DPhysicalDeviceMediaFlash::DPhysicalDeviceMediaFlash()
//
// Constructor
//
	{
	iUnitsMask=0x2;
	iVersion=TVersion(KMediaDriverInterfaceMajorVersion,KMediaDriverInterfaceMinorVersion,KMediaDriverInterfaceBuildVersion);
	}

TInt DPhysicalDeviceMediaFlash::Install()
//
// Install the media drives PDD.
//
	{

	return SetName(&KPddName);
	}

void DPhysicalDeviceMediaFlash::GetCaps(TDes8& /*aDes*/) const
//
// Return the media drivers capabilities.
//
	{
	}

TInt DPhysicalDeviceMediaFlash::Create(DBase*& aChannel, TInt aMediaId, const TDesC8* /* anInfo */,const TVersion &aVer)
//
// Create an LFFS media driver.
//
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		return KErrNotSupported;
	DMediaDriverFlash *pD=DMediaDriverFlash::New(aMediaId);
	aChannel=pD;
	TInt r=KErrNoMemory;
	if (pD)
		r=pD->DoCreate(aMediaId);
	if (r==KErrNone)
		pD->OpenMediaDriverComplete(KErrNone);
	return r;
	}

TInt DPhysicalDeviceMediaFlash::Validate(TInt aDeviceType, const TDesC8* /*anInfo*/, const TVersion& aVer)
	{
	if (!Kern::QueryVersionSupported(iVersion,aVer))
		return KErrNotSupported;
	if (aDeviceType!=MEDIA_DEVICE_LFFS)
		return KErrNotSupported;
	return KErrNone;
	}

TInt DPhysicalDeviceMediaFlash::Info(TInt aFunction, TAny*)
//
// Return the priority of this media driver
//
	{
	if (aFunction==EPriority)
		return KMediaDriverPriorityNormal;
	return KErrNotSupported;
	}




/**
@internalComponent
*/
DMediaDriverFlash::DMediaDriverFlash(TInt aMediaId)
//
// Constructor.
//
	:	DMediaDriver(aMediaId)
	{}




/**
@internalComponent
*/
TInt DMediaDriverFlash::DoCreate(TInt /*aMediaId*/)
//
// Create the media driver.
//
	{
	
	TInt r=Initialise();		// interrogate FLASH etc.
	if (r==KErrNone)
		{
		TUint32 size=TotalSize();
		SetTotalSizeInBytes(size);
		}
	return r;
	}




/** 
A function called by the local media subsystem to deal with a request;
this is implemented by the generic layer of the LFFS media driver.

The implementation delegates the handling of reading, writing and erasing
to the specific layer's DoRead(), DoWrite() and DoErase() functions 
respectively.
	
@param aRequest An object that encapsulates information about the request.
    
@return A value indicating the result:
        KErrNone, if the request has been sucessfully initiated;
        KErrNotSupported, if the request cannot be handled by the device;
        KMediaDriverDeferRequest, if the request cannot be handled
        immediately because of an outstanding request (this request will be
        deferred until the outstanding request has completed);
        otherwise one of the other system-wide error codes.
        
@see DMediaDriverFlash::DoRead()        
@see DMediaDriverFlash::DoWrite()
@see DMediaDriverFlash::DoErase()
*/
TInt DMediaDriverFlash::Request(TLocDrvRequest& m)
	{
	TInt r=KErrNotSupported;
	TInt id=m.Id();
	__KTRACE_OPT(KLOCDRV,Kern::Printf(">DMediaDriverFlash::Request %d",id));
	if (id==DLocalDrive::ECaps)
		{
  		TLocalDriveCapsV4& c=*(TLocalDriveCapsV4*)m.RemoteDes();
		r=Caps(c);
		c.iSize=m.Drive()->iPartitionLen;
		c.iPartitionType=m.Drive()->iPartitionType;
		SetTotalSizeInBytes(c);
		return r;
		}
	switch (id)
		{
		case DLocalDrive::ERead:
			if (iReadReq)
				return KMediaDriverDeferRequest;	// read already in progress so defer this one
			iReadReq=&m;
			r=DoRead();
			if (r!=KErrNone)
				iReadReq=NULL;
			break;
		case DLocalDrive::EWrite:
			if (iWriteReq)
				return KMediaDriverDeferRequest;	// write already in progress so defer this one
			iWriteReq=&m;
			r=DoWrite();
			if (r!=KErrNone)
				iWriteReq=NULL;
			break;
		case DLocalDrive::EFormat:
			if (iEraseReq)
				return KMediaDriverDeferRequest;	// erase already in progress so defer this one
			iEraseReq=&m;
			r=DoErase();
			if (r!=KErrNone)
				iEraseReq=NULL;
			break;
		case DLocalDrive::EEnlarge:
		case DLocalDrive::EReduce:
		default:
			r=KErrNotSupported;
			break;
		}
	__KTRACE_OPT(KLOCDRV,Kern::Printf("<DMediaDriverFlash::Request %d",r));
	if (r<0)
		DMediaDriver::Complete(m,r);
	return r;
	}




/**
A function called by the local media subsystem to inform the driver
that the device should power down.

The default implementation does nothing.
*/
void DMediaDriverFlash::NotifyPowerDown()
	{
	// no action required
	}




/**
A function called by the local media subsystem to inform the driver
that the device is to be immediately powered down.

The default implementation does nothing.
*/
void DMediaDriverFlash::NotifyEmergencyPowerDown()
	{
	// no action required
	}




/**
Called by the specific layer of the LFFS media driver to inform
the generic layer that a request is complete.
    
@param aRequest The type of the request that is complete. This is one of
                the TRequest enum values:
                EReqRead,  EReqWrite or  EReqErase as appropriate.
@param aResult  KErrNone, if the request has been completed successfully, 
                otherwise one if the other system-wide error codes.
                
@see DMediaDriverFlash::TRequest                
*/
void DMediaDriverFlash::Complete(TInt aRequest, TInt aResult)
	{
	__KTRACE_OPT(KLOCDRV,Kern::Printf("Flash:Complete(%d,%d)",aRequest,aResult));
	TLocDrvRequest* pR=iRequests[aRequest];
	iRequests[aRequest]=NULL;
	DMediaDriver::Complete(*pR,aResult);
	}



/**
Called by the generic layer to get the capabilities of the flash device.

The default implementation is synchronous, and returns KErrCompletion.

@param aCaps On return, descriptor data contains capability information
		about the flash device, in the form of a class derived from
		TLocalDriveCapsV2. The size of the derived class should not exceed
		KMaxLocalDriveCapsLength which is defined and used in
		e32\drivers\locmedia\locmedia.cpp. If a larger sized capabilities
		class is used, and this code is modified to write to member data
		beyond KMaxLocalDriveCapsLength this will cause a fault.

@return KErrCompletion, if the operation has been done synchronously and is successful;
        one of the other system wide error codes (not KErrNone), if
        the operation has been done synchronously but UNSUCCESSFULLY;
        KErrNone, if the operation is being done asynchronously.
@see TLocalDriveCapsV2
*/

TInt DMediaDriverFlash::Caps(TLocalDriveCapsV2& caps)
	{
	caps.iType=EMediaFlash;
	caps.iConnectionBusType=EConnectionBusInternal;
	caps.iDriveAtt=KDriveAttLocal|KDriveAttInternal;
	caps.iMediaAtt=KMediaAttFormattable;
    caps.iBaseAddress=(TUint8*)TInternalRamDrive::Base();
	caps.iFileSystemId=KDriveFileSysLFFS;
	caps.iHiddenSectors=0;
	caps.iEraseBlockSize=EraseBlockSize();

    __KTRACE_OPT( KLOCDRV, Kern::Printf("MLFS: ) type=%d", caps.iType) );
    __KTRACE_OPT( KLOCDRV, Kern::Printf("MLFS: ) connectionbustype=%d", caps.iConnectionBusType) );
    __KTRACE_OPT( KLOCDRV, Kern::Printf("MLFS: ) driveatt=0x%x", caps.iDriveAtt) );
    __KTRACE_OPT( KLOCDRV, Kern::Printf("MLFS: ) mediaatt=0x%x", caps.iMediaAtt) );
    __KTRACE_OPT( KLOCDRV, Kern::Printf("MLFS: ) filesystemid=0x%x", caps.iFileSystemId) );
    __KTRACE_OPT( KLOCDRV, Kern::Printf("MLFS: ) eraseblocksize=0x%x", caps.iEraseBlockSize) );

	return KErrCompletion;	// synchronous completion
	}




/**
A function called by the local media subsystem to get partition information
for the flash device.
	
It is called once the subsystem has been notified that the media driver
is open and has been succesfully initialised.

The function should be overriden by the specific layer of
the LFFS media driver.

The default implementation is synchronous
and sets:

- the partition count to 1, meaning that there is only the one partition.
- the partition base address to 0.
- the partition length to the total size of the flash device.
- the size of the media to the total size of the flash device.
- the partition type to KPartitionTypeEneaLFFS.

@param anInfo An object that, on successful return, contains
              the partition information.
	
@return KErrNone, if retrieval of partition information is to be
        done asynchronously;
        KErrCompletion, if retrieval of partition information has been
        done synchronously, and successfully;
        one of the other system-wide error codes, if retrieval of partition
        information has been done synchronously, but unsuccessfully.
*/
TInt DMediaDriverFlash::PartitionInfo(TPartitionInfo& aInfo)
	{
	aInfo.iPartitionCount				= 1;
	aInfo.iEntry[0].iPartitionBaseAddr	= 0;
	aInfo.iEntry[0].iPartitionLen		= TotalSizeInBytes();
	aInfo.iEntry[0].iPartitionType		= KPartitionTypeEneaLFFS;
	
	aInfo.iMediaSizeInBytes				= TotalSizeInBytes();
	return KErrCompletion;
	}


DECLARE_EXTENSION_PDD()
	{
	return new DPhysicalDeviceMediaFlash;
	}

static const TInt LffsDriveNumbers[LFFS_DRIVECOUNT]={LFFS_DRIVELIST};	
_LIT(KFlashDriveName,LFFS_DRIVENAME);

DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("Registering FLASH drive"));
	if (Kern::SuperPage().iCpuId & KCpuIdISS)
		return KErrNone;	// no FLASH on ARMULATOR

	TInt r=Kern::DfcQInit(&FlashDfcQ,KFlashThreadPriority,&KFlashThreadName);
	if (r==KErrNone)
		{
		DPrimaryMediaBase* pM=new DPrimaryMediaBase;
		if (pM)
			{
			pM->iDfcQ=&FlashDfcQ;
			r=LocDrv::RegisterMediaDevice(EFixedMedia1,LFFS_DRIVECOUNT,&LffsDriveNumbers[0],pM,LFFS_NUMMEDIA,KFlashDriveName);
			if (r==KErrNone)
				pM->iMsgQ.Receive();
			}
		}
	__KTRACE_OPT(KBOOT,Kern::Printf("Registering FLASH drive - return %d",r));
	return r;
	}


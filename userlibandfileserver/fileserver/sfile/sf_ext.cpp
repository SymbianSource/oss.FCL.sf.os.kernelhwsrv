// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_ext.cpp
// 
//

#include "sf_std.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "sf_extTraces.h"
#endif
typedef CProxyDriveFactory*(*TExtensionNew)();
typedef CExtProxyDriveFactory*(*TProxyDriveNew)();


/**
Constructor.

Initialises drive extension count to zero.
*/
TDriveExtInfo::TDriveExtInfo(){iCount=0;}


/**
Constructor.

Note that the class is intended only as an abstract base for other classes.

@panic FSERV 51 if the supplied CMountCB pointer is NULL.
*/
CProxyDrive::CProxyDrive(CMountCB* aMount)
:iMount(aMount)
	{
//	__ASSERT_DEBUG(iMount!=NULL,Fault(EProxyDriveConstruction));
	}


/**
Destructor.

Frees resources before destruction of the object.
*/
CProxyDrive::~CProxyDrive()
	{
	delete iBody;
	}


/**
An interface with which control commands can be passed to 
the appropriate driver layer.

This base implementation performs no operations.

@param aMessage Message to be sent.
@param aCommand Command type.
@param aParam1  1st parameter of control message.
@param aParam2  2nd parameter of control message.

@return KErrNone
*/
EXPORT_C TInt CProxyDrive::ControlIO(const RMessagePtr2& /*aMessage*/,TInt /*aCommand*/,TAny* /*aParam1*/,TAny* /*aParam2*/)
//
// General purpose function for use by specific file systems and extensions
//
	{
	return(KErrNone);
	}

/**
General purpose read function for use by specific file systems and extensions.

This base implementation performs no operations.

@param aPos    The address from where the read begins.
@param aLength The length of the read.
@param aTrg    A descriptor of the memory buffer from which to read.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aTrg to read the data from.
@param aFlags  Flags to be passed into the driver.

@return KErrNone
*/
EXPORT_C TInt CProxyDrive::Read(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aTrg*/,TInt /*aThreadHandle*/,TInt /*aOffset*/, TInt /*aFlags*/)
	{
	return(KErrNone);
	}

/**
General purpose write function for use by specific file systems and extensions.

This base implementation performs no operations.

@param aPos    The address from where the write begins.
@param aLength The length of the write.
@param aSrc    A descriptor of the memory buffer from which to write.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aSrc to write the data to.
@param aFlags  Flags to be passed into the driver.

@return KErrNone
*/
EXPORT_C TInt CProxyDrive::Write(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aSrc*/,TInt /*aThreadHandle*/,TInt /*aOffset*/, TInt /*aFlags*/)
	{
	return(KErrNone);
	}


/**
Issue a notification that a physical delete has occurred. 
For example a cluster or partition has been freed.

This base implementation performs no operations.

@param aPos    The position of the data which is being deleted.
@param aLength The length of the data which is being deleted.

@return KErrNone
*/
EXPORT_C TInt CProxyDrive::DeleteNotify(TInt64 /*aPos*/, TInt /*aLength*/)
	{
	return(KErrNone);
	}
	
/**
An interface with which information can be retrieved about disk errors.

This base implementation performs no operations.

@param aErrorInfo Reference to a descriptor containing disk error information.

@return KErrNone
*/	
EXPORT_C TInt CProxyDrive::GetLastErrorInfo(TDes8& /*aErrorInfo*/)
	{
	return(KErrNotSupported);
	}
	
EXPORT_C TInt CProxyDrive::GetLocalDrive(TBusLocalDrive*& aLocDrv)
	{
	return (GetInterface(EGetLocalDrive, (TAny*&)aLocDrv, (TAny*)this));	// this GetInterface does the dirty work for you...
	}

EXPORT_C TInt CProxyDrive::Finalise(TBool aIsFinalised)
	{
	TAny* dummyInterface;
	return (GetInterface(EFinalised, dummyInterface, (TAny*)aIsFinalised));
	}


/**
Called to get a Proxy drive interface.

@param aInterfaceId Interface identifier of the interface to be retrieved.
@param aInterface Address of variable that retrieves the specified interface.
@param aInput Data required for the instantiation of the interface.

This base implementation performs no operations.

@return KErrNotSupported
*/	
EXPORT_C TInt CProxyDrive::GetInterface(TInt /*aInterfaceId*/,TAny*& /*aInterface*/,TAny* /*aInput*/)
	{
	return(KErrNotSupported);
	}	


TInt CProxyDrive::SetAndOpenLibrary(RLibrary aLibrary)
	{
	TInt r = KErrNone;
	if (iBody)
		{
		iBody->iLibrary = aLibrary;
		r = iBody->iLibrary.Duplicate(RThread(), EOwnerProcess);
		}
	return r;
	}

/**
Gets a copy of the Proxy Drive library (previously opened by OpenLibrary()) prior to deleting the proxy drive itself.
After deleting the proxy drive, RLibrary::Close() should be called on the returned RLibrary object.
*/
RLibrary CProxyDrive::GetLibrary()
	{
	RLibrary lib;
	if (iBody)
		{
		lib = iBody->iLibrary;
		iBody->iLibrary.SetHandle(NULL);
		}
	return lib;
	}

/**
Constructor.
*/
EXPORT_C CLocDrvMountCB::CLocDrvMountCB() {}


/**
Destructor.

Frees assigned Proxy drive before destruction of the object.
*/
EXPORT_C CLocDrvMountCB::~CLocDrvMountCB()
	{
	__PRINT1(_L("CLocDrvMountCB::~CLocDrvMountCB() 0x%x"),this);
	if(iProxyDrive && !LocalDrives::IsProxyDrive(Drive().DriveNumber()))
		{
		RLibrary lib = iProxyDrive->GetLibrary();
		delete iProxyDrive;
		lib.Close();
		}
	}


/**
Creates and initialises the local drive.

@param aLocDrv The local drive to be created

@return System wide error code.
*/		
EXPORT_C TInt CLocDrvMountCB::CreateLocalDrive(TBusLocalDrive& aLocDrv)
	{
	__PRINT(_L("CLocDrvMountCB::CreateLocalDrive()"));
	if(iProxyDrive!=NULL)
		return(KErrNone);
	TInt r;
	CProxyDrive* pConcrete=CLocalProxyDrive::New(this,aLocDrv);
	if(pConcrete==NULL)
		{
		r=KErrNoMemory;
		}
	else
		{
		// if failure then pConcrete will be deleted by CreateProxyDriveL()
		TRAP(r,iProxyDrive=CreateProxyDriveL(pConcrete,this));	
		}
	if(r==KErrNone)
		r=InitLocalDrive();
	__PRINT1(_L("CreateLocalDrive r=%d"),r);
	return(r);
	}

EXPORT_C TInt CLocDrvMountCB::CreateDrive(TInt aDriveNumber)
/** Create drive
	Ascertain if the drive is mapped to a local drive or a proxy drive, and create the drive
	as appropriate
	@param	aDriveNumber	drive number
	@return	KErrNone		on success
			KErrArgument	if the drive is not mapped to a proxy or a local drive or if the number
							is invalid
*/
	{
	// dunno why we are using TInts instead of TUints here
	__PRINT(_L("CLocDrvMountCB::CreateDrive()"));

	if (aDriveNumber<0 || aDriveNumber>=KMaxDrives) return KErrArgument;
	TInt r = KErrNone;
	TInt aDriveLocal = LocalDrives::DriveNumberToLocalDriveNumber(aDriveNumber);
	if (aDriveLocal == KDriveInvalid) return KErrArgument;
	if (aDriveLocal < KMaxLocalDrives)
		{
		return CreateLocalDrive(LocalDrives::GetLocalDrive(aDriveNumber));	// drive is really local
		}
	else
		{
		CExtProxyDrive* pProxyDrive = LocalDrives::GetProxyDrive(aDriveNumber);
		__ASSERT_ALWAYS(pProxyDrive != NULL,User::Panic(_L("CreateDrive() - pProxyDrive == NULL"), -999));

		TRAP(r, iProxyDrive = CreateProxyDriveL(pProxyDrive,this));
		__ASSERT_ALWAYS(r == KErrNone,User::Panic(_L("CreateDrive() - CreateProxyDriveL error"), -999));

		r = InitLocalDrive();
		}
	
	return r;
	}


/**
Initialise the local drive

@panic FSERV 52 if initialise when no local drive exists.

@return system wide error code
*/
EXPORT_C TInt CLocDrvMountCB::InitLocalDrive()
	{
	__ASSERT_ALWAYS(iProxyDrive!=NULL,Fault(ELocDrvInitLocalDrive));
	iProxyDrive->SetMount(this);
	return(iProxyDrive->Initialise());
	}

/**
Dismount the local drive

@panic FSERV 53 if dismount when no local drive exists.
*/
EXPORT_C void CLocDrvMountCB::DismountedLocalDrive()
	{
	__ASSERT_ALWAYS(iProxyDrive!=NULL,Fault(ELocDrvDismountedLocalDrive));
	iProxyDrive->Dismounted();
	}


/**
static constructor.

Instatiates a CLocalProxyDrive objects with the given arguments.

@param aMount The mount control block
@param aLocDrv The local drive.

@return pointer to instantiated CLocalProxyDrive object.
*/
CLocalProxyDrive* CLocalProxyDrive::New(CMountCB* aMount,TBusLocalDrive& aLocDrv)
	{
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVENEW, "aMount %x drive %d", (TUint) aMount, (TUint) aMount->DriveNumber());
	CLocalProxyDrive* proxyDrive = new CLocalProxyDrive(aMount,aLocDrv);
	
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVENEWRET, "proxyDrive %x", proxyDrive);
	return proxyDrive;
	}

/**
Constructor.

@param aMount The mount control block.
@param aLocDrv The local drive.
*/
CLocalProxyDrive::CLocalProxyDrive(CMountCB* aMount,TBusLocalDrive& aLocDrv)
:CProxyDrive(aMount),iLocDrv(aLocDrv)
	{
	__PRINT(_L("CLocalProxyDrive::CLocalProxyDrive()"));
	}


/**
Initialise the connected drive.

This implementation performs no operations.

@return KErrNone.
*/
TInt CLocalProxyDrive::Initialise()
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEINITIALISE, "this %x", this);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEINITIALISERET, "r %d", KErrNone);
	return(KErrNone);
	}


/**
Ensures any cached data is flushed before unmounting drive.

This implementation performs no operations.

@return KErrNone.
*/
TInt CLocalProxyDrive::Dismounted()
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEDISMOUNTED, "this %x", this);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEDISMOUNTEDRET, "r %d", KErrNone);
	return(KErrNone);
	}


/**
Increase the size of the connected drive by the specified length (in bytes).

@param aLength The length/size (in bytes) by which the drive is to be increased.

@return system wide error code.
*/
TInt CLocalProxyDrive::Enlarge(TInt aLength)
	{
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEENLARGE, "this %x aLength %d", (TUint) this, (TUint) aLength);
	TInt r = iLocDrv.Enlarge(aLength);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEENLARGERET, "r %d", r);
	return r;
	}


/**
Reduce the size of the connected drive by removing the specified length
(in bytes) starting at the specified position.
Refer to relevant media driver documentation for implementation/restriction 
notes.

@param aPos    The start position of area to be removed.
@param aLength The length of the data which is being removed.

@return system wide error code.
*/
TInt CLocalProxyDrive::ReduceSize(TInt aPos, TInt aLength)
	{
	OstTraceExt3(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEREDUCESIZE, "this %x aPos %x aLength %d", (TUint) this, (TUint) aPos, (TUint) aLength);
	TInt r = iLocDrv.ReduceSize(aPos,aLength);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEREDUCESIZERET, "r %d", r);
	return r;
	}


/**
Read from the connected drive, and pass flags to driver.

@param aPos    The address from where the read begins.
@param aLength The length of the read.
@param aTrg    A descriptor of the memory buffer from which to read.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aTrg to read the data from.
@param aFlags  Flags to be passed into the driver.

@return system wide error code.
*/
TInt CLocalProxyDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt aOffset, TInt aFlags)
	{
	TRACETHREADIDH(aThreadHandle);
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEREAD1, "this %x clientThreadId %x aPos %x:%x aLength %d", (TUint) this, (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aLength);
	TInt r = iLocDrv.Read(aPos,aLength,aTrg,aThreadHandle,aOffset,aFlags);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEREAD1RET, "r %d", r);
	return r;
	}

/**
Read from the connected drive.

@param aPos    The address from where the read begins.
@param aLength The length of the read.
@param aTrg    A descriptor of the memory buffer from which to read.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aTrg to read the data from.

@return system wide error code.
*/
TInt CLocalProxyDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset)
	{
	TRACETHREADIDH(aThreadHandle);
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEREAD2, "this %x clientThreadId %x aPos %x:%x aLength %d", (TUint) this, (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aLength);
	TInt r = iLocDrv.Read(aPos,aLength,aTrg,aThreadHandle,anOffset);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEREAD2RET, "r %d", r);
	return r;
	}

/**
Read from the connected drive.

@param aPos    The address from where the read begins.
@param aLength The length of the read.
@param aTrg    A descriptor of the memory buffer from which to read.

@return system wide error code.
*/
TInt CLocalProxyDrive::Read(TInt64 aPos,TInt aLength,TDes8& aTrg)
	{
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEREAD3, "this %x aPos %x:%x aLength %d aTrg %x", (TUint) this, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aLength, (TUint) &aTrg);
	TInt r = iLocDrv.Read(aPos,aLength,aTrg);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEREAD3RET, "r %d", r);
	return r;
	}


/**
Write to the connected drive and pass flags to driver.

@param aPos    The address from where the write begins.
@param aLength The length of the write.
@param aSrc    A descriptor of the memory buffer from which to write.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aSrc to write the data to.
@param aFlags  Flags to be passed into the driver.

@return system wide error code.
*/
TInt CLocalProxyDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt aOffset,TInt aFlags)
	{
	TRACETHREADIDH(aThreadHandle);
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEWRITE1, "this %x clientThreadId %x aPos %x:%x aLength %d", (TUint) this, (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aLength);
	TInt r = iLocDrv.Write(aPos,aLength,aSrc,aThreadHandle,aOffset,aFlags);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEWRITE1RET, "r %d", r);
	return r;
	}


/**
Write to the connected drive.

@param aPos    The address from where the write begins.
@param aLength The length of the write.
@param aSrc    A descriptor of the memory buffer from which to write.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aSrc to write the data to.

@return system wide error code.
*/
TInt CLocalProxyDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset)
	{
	TRACETHREADIDH(aThreadHandle);
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEWRITE2, "this %x clientThreadId %x aPos %x:%x aLength %d", (TUint) this, (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aLength);
	TInt r = iLocDrv.Write(aPos,aLength,aSrc,aThreadHandle,anOffset);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEWRITE2RET, "r %d", r);
	return r;
	}


/**
Write to the connected drive.

@param aPos    The address from where the write begins.
@param aSrc    A descriptor of the memory buffer from which to write.

@return system wide error code.
*/
TInt CLocalProxyDrive::Write(TInt64 aPos,const TDesC8& aSrc)
	{
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEWRITE3, "this %x aPos %x:%x aLength %d aSrc %x", (TUint) this, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aSrc.Length(), (TUint) &aSrc);
	TInt r = iLocDrv.Write(aPos,aSrc);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEWRITE3RET, "r %d", r);
	return r;
	}
	
	
/**
Get the connected drive's capabilities information.

@param anInfo A descriptor of the connected drives capabilities.

@return system wide error code
*/
TInt CLocalProxyDrive::Caps(TDes8& anInfo)
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVECAPS, "this %x", this);
	TInt r = iLocDrv.Caps(anInfo);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVECAPSRET, "r %d", r);
	return r;
	}


/**
Format the connected drive.

@param anInfo Device specific format information.

@return system wide error code.
*/
TInt CLocalProxyDrive::Format(TFormatInfo& anInfo)
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEFORMAT1, "this %x", this);
	TInt r = iLocDrv.Format(anInfo);
	OstTraceExt4(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEFORMAT1RET, "r %d iFormatIsCurrent %d i512ByteSectorsFormatted %d iMaxBytesPerFormat %d", (TUint) r, (TUint) anInfo.iFormatIsCurrent, (TUint) anInfo.i512ByteSectorsFormatted, (TUint) anInfo.iMaxBytesPerFormat);
	return r;
	}


/**
Format the connected drive.

@param aPos    The position of the data which is being formatted.
@param aLength The length of the data which is being formatted.

@return system wide error code.
*/
TInt CLocalProxyDrive::Format(TInt64 aPos,TInt aLength)
	{
	OstTraceExt4(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEFORMAT2, "this %x aPos %x:%x aLength %d", (TUint) this, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), (TUint) aLength);
	TInt r = iLocDrv.Format(aPos,aLength);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEFORMAT2RET, "r %d", r);
	return r;
	}


/**
Set the mount information on the local drive.

@param aMountInfo Information passed down to the media driver. The meaning of this information depends on the media driver.
@param aMountInfoThreadHandle  Message thread handle number.

@return system wide error code.
*/
TInt CLocalProxyDrive::SetMountInfo(const TDesC8* aMountInfo,TInt aMountInfoThreadHandle)
	{
	OstTraceExt3(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVESETMOUNTINFO, "this %x aMountInfo %x aMountInfoThreadHandle %d", (TUint) this, (TUint) aMountInfo, (TUint) aMountInfoThreadHandle);
	TInt r = iLocDrv.SetMountInfo(aMountInfo,aMountInfoThreadHandle);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVESETMOUNTINFORET, "r %d", r);
	return r;
	}


/**
Forces a remount on the local drive

@param aFlags Flags to be passed into the driver.

@return system wide error code.
*/
TInt CLocalProxyDrive::ForceRemount(TUint aFlags)
	{
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEFORCEREMOUNT, "this %x aFlags %x", (TUint) this, (TUint) aFlags);
	TInt r = iLocDrv.ForceRemount(aFlags);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEFORCEREMOUNTRET, "r %d", r);
	return r;
	}

/**
An interface with which control commands can be passed to 
the appropriate driver layer.

@param aMessage Message to be sent.
@param aCommand Command type.
@param aParam1  1st parameter of control message.
@param aParam2  2nd parameter of control message.

@return system wide error code.
*/
TInt CLocalProxyDrive::ControlIO(const RMessagePtr2& /*aMessage*/,TInt aCommand,TAny* aParam1,TAny* aParam2)
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVECONTROLIO, "this %x", this);
	TInt r = iLocDrv.ControlIO(aCommand,aParam1,aParam2);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVECONTROLIORET, "r %d", r);
	return r;
	}
	
	
/**
Unlocks a password-enabled device.

@param aPassword A descriptor containing the existing password.
@param aStorePassword If ETrue, the password is added to the password store.

@return system wide error code.
*/
TInt CLocalProxyDrive::Unlock(TMediaPassword &aPassword, TBool aStorePassword)
	{
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEUNLOCK, "this %x aPassword %d", (TUint) this, (TUint) aStorePassword);
	TInt r = iLocDrv.Unlock(aPassword,aStorePassword);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEUNLOCKRET, "r %d", r);
	return r;
	}


/**
Locks a password-enabled device with the new password.

@param aOldPassword A descriptor containing the existing password.
@param aNewPassword A descriptor containing the new password.
@param aStorePassword If ETrue, the password is added to the password store.

@return system wide error code.
*/
TInt CLocalProxyDrive::Lock(TMediaPassword &aOldPassword, TMediaPassword &aNewPassword, TBool aStorePassword)
	{
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVELOCK, "this %x aPassword %d", (TUint) this, (TUint) aStorePassword);
	TInt r = iLocDrv.SetPassword(aOldPassword,aNewPassword,aStorePassword);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVELOCKRET, "r %d", r);
	return r;
	}


/**
Clears a password from a device - controller sets password to null.

@param aPassword A descriptor containing the password.

@return system wide error code.
*/
TInt CLocalProxyDrive::Clear(TMediaPassword &aPassword)
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVECLEAR, "this %x", this);
	TInt r = iLocDrv.Clear(aPassword);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVECLEARRET, "r %d", r);
	return r;
	}

/**
Forcibly unlock a password-enabled drive.

@return system wide error code.
*/
TInt CLocalProxyDrive::ErasePassword()
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEERASEPASSWORD, "this %x", this);
	TInt r = iLocDrv.ErasePassword();
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEERASEPASSWORDRET, "r %d", r);
	return r;
	}

/**
Notify the media driver that an area of the partition has been deleted.
Used by some media drivers (e.g. NAND flash) for garbage collection.

@param aPos    The position of the data which is being deleted.
@param aLength The length of the data which is being deleted.
@return System wide error code.
*/
TInt CLocalProxyDrive::DeleteNotify(TInt64 aPos, TInt aLength)
	{
	OstTraceExt4(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEDELETENOTIFY, "this %x aPos %x:%x aLength %d", (TUint) this, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), (TUint) aLength);
	TInt r = iLocDrv.DeleteNotify(aPos, aLength);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEDELETENOTIFYRET, "r %d", r);
	return r;
	}


/**
Retrieve disk error information.

@param aErrorInfo Reference to a descriptor containing disk error information.

@return System wide error code.
*/	
TInt CLocalProxyDrive::GetLastErrorInfo(TDes8 &aErrorInfo)
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEGETLASTERRORINFO, "this %x", this);
	TInt r = iLocDrv.GetLastErrorInfo(aErrorInfo);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEGETLASTERRORINFORET, "r %d", r);
	return r;
	}


TInt CLocalProxyDrive::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
	{
	OstTraceExt3(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEGETINTERFACE, "this %x aInterfaceId %d aInput %x", (TUint) this, (TUint) aInterfaceId, (TUint) aInput);
	TInt r;
	switch(aInterfaceId)
		{
		case EGetLocalDrive:
			__ASSERT_ALWAYS((CProxyDrive*)aInput==this,Fault(ELocDrvInvalidLocalDrive));
			(TBusLocalDrive*&)aInterface=&iLocDrv;
			r = KErrNone;
			break;

        case ELocalBufferSupport:
            aInterface = NULL;
			r = KErrNone;
			break;

		default:
			r= CProxyDrive::GetInterface(aInterfaceId,aInterface,aInput);
		}
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECLOCALPROXYDRIVEGETINTERFACERET, "r %d aInterface %x", (TUint) r, (TUint) aInterface);
	return r;
	}



/**
Constructor.

@panic FSERV 54 if the supplied CMountCB pointer is NULL.
*/
EXPORT_C CBaseExtProxyDrive::CBaseExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount)
:CProxyDrive(aMount),iProxy(aProxyDrive)
	{
	__ASSERT_DEBUG(iProxy!=NULL,Fault(EBaseExtConstruction));
	}	
	
/**
Destructor.

Frees resources before destruction of the object.
*/	
EXPORT_C CBaseExtProxyDrive::~CBaseExtProxyDrive()
	{
	delete(iProxy);
	}


/**
Initialise the proxy drive.

This implementation performs no operations.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Initialise()
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEINITIALISE, "this %x", this);
	TInt r = iProxy->Initialise();
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEINITIALISERET, "r %d", r);
	return r;
	}


/**
Calls Dismounted() on the proxy drive.

@return KErrNone.
*/
EXPORT_C TInt CBaseExtProxyDrive::Dismounted()
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEDISMOUNTED, "this %x", this);
	TInt r = iProxy->Dismounted();
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEDISMOUNTEDRET, "r %d", r);
	return r;
	}

/**
Increase the size of the proxy drive by the specified length (in bytes).

@param aLength The length (in bytes) of which the drive is to be increased by.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Enlarge(TInt aLength)
	{
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEENLARGE, "this %x aLength %d", (TUint) this, (TUint) aLength);
	TInt r = iProxy->Enlarge(aLength);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEENLARGERET, "r %d", r);
	return r;
	}


/**
Reduce the size of the proxy drive by removing the specified length
(in bytes) starting at the specified position.
Refer to relevant media driver documentation for implementation/restriction 
notes.

@param aPos    The start position of area to be removed.
@param aLength The length/size (in bytes) by which the drive is to be reduced.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::ReduceSize(TInt aPos, TInt aLength)
	{
	OstTraceExt3(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEREDUCESIZE, "this %x aPos %x aLength %d", (TUint) this, (TUint) aPos, (TUint) aLength);
	TInt r = iProxy->ReduceSize(aPos,aLength);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEREDUCESIZERET, "r %d", r);
	return r;
	}


/**
Read from the proxy drive, and pass flags to driver.

@param aPos    The address from where the read begins.
@param aLength The length of the read.
@param aTrg    A descriptor of the memory buffer from which to read.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aTrg to read the data from.
@param aFlags  Flags to be passed into the driver.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt aOffset,TInt aFlags)
	{
	TRACETHREADIDH(aThreadHandle);
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEREAD1, "this %x clientThreadId %x aPos %x:%x aLength %d", (TUint) this, (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aLength);
	TInt r = iProxy->Read(aPos,aLength,aTrg,aThreadHandle,aOffset,aFlags);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEREAD1RET, "r %d", r);
	return r;
	}


/**
Read from the proxy drive.

@param aPos    The address from where the read begins.
@param aLength The length of the read.
@param aTrg    A descriptor of the memory buffer from which to read.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aTrg to read the data from.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset)
	{
	TRACETHREADIDH(aThreadHandle);
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEREAD2, "this %x clientThreadId %x aPos %x:%x aLength %d", (TUint) this, (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aLength);
	TInt r = iProxy->Read(aPos,aLength,aTrg,aThreadHandle,anOffset);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEREAD2RET, "r %d", r);
	return r;
	}


/**
Read from the proxy drive.

@param aPos    The address from where the read begins.
@param aLength The length of the read.
@param aTrg    A descriptor of the memory buffer from which to read.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Read(TInt64 aPos,TInt aLength,TDes8& aTrg)
	{
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEREAD3, "this %x aPos %x:%x aLength %d aTrg %x", (TUint) this, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aLength, (TUint) &aTrg);
		
	TInt r = iProxy->Read(aPos,aLength,aTrg);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEREAD3RET, "r %d", r);
	return r;
	}


/**
Write to the proxy drive and pass flags to driver.

@param aPos    The address from where the write begins.
@param aLength The length of the write.
@param aSrc    A descriptor of the memory buffer from which to write.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aSrc to write the data to.
@param aFlags  Flags to be passed into the driver.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt aOffset,TInt aFlags)
	{
	TRACETHREADIDH(aThreadHandle);
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEWRITE1, "this %x clientThreadId %x aPos %x:%x aLength %d", (TUint) this, (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aLength);
	TInt r = iProxy->Write(aPos,aLength,aSrc,aThreadHandle,aOffset,aFlags);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEWRITE1RET, "r %d", r);
	return r;
	}


/**
Write to the proxy drive.

@param aPos    The address from where the write begins.
@param aLength The length of the write.
@param aSrc    A descriptor of the memory buffer from which to write.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aSrc to write the data to.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset)
	{
	TRACETHREADIDH(aThreadHandle);
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEWRITE2, "this %x clientThreadId %x aPos %x:%x aLength %d", (TUint) this, (TUint) threadId, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aLength);
	TInt r = iProxy->Write(aPos,aLength,aSrc,aThreadHandle,anOffset);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEWRITE2RET, "r %d", r);
	return r;
	}


/**
Write to the proxy drive.

@param aPos    The address from where the write begins.
@param aSrc    A descriptor of the memory buffer from which to write.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Write(TInt64 aPos,const TDesC8& aSrc)
	{
	OstTraceExt5(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEWRITE3, "this %x aPos %x:%x aLength %d aSrc %x", (TUint) this, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), aSrc.Length(), (TUint) &aSrc);
	TInt r = iProxy->Write(aPos,aSrc);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEWRITE3RET, "r %d", r);
	return r;
	}


/**
Get the proxy drive's capabilities information.

@param anInfo A descriptor of the connected drives capabilities.

@return system wide error code
*/
EXPORT_C TInt CBaseExtProxyDrive::Caps(TDes8& anInfo)
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVECAPS, "this %x", this);
	TInt r = iProxy->Caps(anInfo);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVECAPSRET, "r %d", r);
	return r;
	}


/**
Format the connected drive.

@param anInfo Device specific format information.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Format(TFormatInfo& anInfo)
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEFORMAT1, "this %x", this);
	TInt r = iProxy->Format(anInfo);
	OstTraceExt4(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEFORMAT1RET, "r %d iFormatIsCurrent %d i512ByteSectorsFormatted %d iMaxBytesPerFormat %d", (TUint) r, (TUint) anInfo.iFormatIsCurrent, (TUint) anInfo.i512ByteSectorsFormatted, (TUint) anInfo.iMaxBytesPerFormat);
	return r;
	}


/**
Format the proxy drive.

@param aPos    The position of the data which is being formatted.
@param aLength The length of the data which is being formatted.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Format(TInt64 aPos,TInt aLength)
	{
	OstTraceExt4(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEFORMAT2, "this %x aPos %x:%x aLength %d", (TUint) this, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), (TUint) aLength);
	TInt r = iProxy->Format(aPos,aLength);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEFORMAT2RET, "r %d", r);
	return r;
	}


/**
Set the mount information on the proxy drive.

@param aMountInfo Information passed down to the media driver. The meaning of this information depends on the media driver.
@param aMountInfoThreadHandle  Message thread handle number.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::SetMountInfo(const TDesC8* aMountInfo,TInt aMountInfoThreadHandle)
	{
	OstTraceExt3(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVESETMOUNTINFO, "this %x aMountInfo %x aMountInfoThreadHandle %x", (TUint) this, (TUint) aMountInfo, (TUint) aMountInfoThreadHandle);
	TInt r = iProxy->SetMountInfo(aMountInfo,aMountInfoThreadHandle);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVESETMOUNTINFORET, "r %d", r);
	return r;
	}


/**
Forces a remount on the proxy drive

@param aFlags Flags to be passed into the driver.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::ForceRemount(TUint aFlags)
	{
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEFORCEREMOUNT, "this %x aFlags%x", (TUint) this, (TUint) aFlags);
	TInt r = iProxy->ForceRemount(aFlags);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEFORCEREMOUNTRET, "r %d", r);
	return r;
	}


/**
Unlocks a password-enabled proxy drive.

@param aPassword A descriptor containing the existing password.
@param aStorePassword If ETrue, the password is added to the password store.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Unlock(TMediaPassword &aPassword, TBool aStorePassword)
	{
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEUNLOCK, "this %x aPassword %d", (TUint) this, (TUint) aStorePassword);
	TInt r = iProxy->Unlock(aPassword,aStorePassword);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEUNLOCKRET, "r %d", r);
	return r;
	}


/**
Locks a password-enabled proxy drive with the new password.

@param aOldPassword A descriptor containing the existing password.
@param aNewPassword A descriptor containing the new password.
@param aStorePassword If ETrue, the password is added to the password store.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Lock(TMediaPassword &aOldPassword, TMediaPassword &aNewPassword, TBool aStorePassword)
	{
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVELOCK, "this %x aPassword %d", (TUint) this, (TUint) aStorePassword);
	TInt r = iProxy->Lock(aOldPassword,aNewPassword,aStorePassword);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVELOCKRET, "r %d", r);
	return r;
	}


/**
Clears a password from a proxy drive - controller sets password to null.

@param aPassword A descriptor containing the password.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::Clear(TMediaPassword &aPassword)
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVECLEAR, "this %x", this);
	TInt r = iProxy->Clear(aPassword);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVECLEARRET, "r %d", r);
	return r;
	}

/**
Forcibly unlock a password-enabled proxy drive.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::ErasePassword()
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEERASEPASSWORD, "this %x", this);
	TInt r = iProxy->ErasePassword();
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEERASEPASSWORDRET, "r %d", r);
	return r;
	}

/**
An interface with which control commands can be passed to 
the appropriate driver layer.

@param aMessage Message to be sent.
@param aCommand Command type.
@param aParam1  1st parameter of control message.
@param aParam2  2nd parameter of control message.

@return system wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2)
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVECONTROLIO, "this %x", this);
	TInt r = iProxy->ControlIO(aMessage,aCommand,aParam1,aParam2);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVECONTROLIORET, "r %d", r);
	return r;
	}


/**
Initialise the provided interface extension.

@param aInterfaceId Interface identifier of the interface to be retrieved.
@param aInterface Address of variable that retrieves the specified interface.
@param aInput Data required for the instantiation of the interface.

@return system wide error code.
*/	
EXPORT_C TInt CBaseExtProxyDrive::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
	{
	OstTraceExt3(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEGETINTERFACE, "this %x aInterfaceId %d aInput %x", (TUint) this, (TUint) aInterfaceId, (TUint) aInput);
		
	TInt r;
	if (aInterfaceId==EGetLocalDrive)
		{
		r = iProxy->GetLocalDrive((TBusLocalDrive*&)aInterface);		// iProxy is of type CLocalProxyDrive, so OK to reenter
		}
	else	
		r = CProxyDrive::GetInterface(aInterfaceId,aInterface,aInput);
	OstTraceExt2(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEGETINTERFACERET, "r %d aInterface %x", (TUint) r, (TUint) aInterface);
	return r;
	}	


/**
Retrieve proxy drive disk error information.

@param aErrorInfo Reference to a descriptor containing disk error information.

@return System wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::GetLastErrorInfo(TDes8 &aErrorInfo)
	{
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEGETLASTERRORINFO, "this %x", this);
	TInt r = iProxy->GetLastErrorInfo(aErrorInfo);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEGETLASTERRORINFORET, "r %d", r);
	return r;
	}


/**
Issue a notification that a physical delete has occurred. 
For example a cluster or partition has been freed.

@param aPos    The position of the data which is being deleted.
@param aLength The length of the data which is being deleted.

@return System wide error code.
*/
EXPORT_C TInt CBaseExtProxyDrive::DeleteNotify(TInt64 aPos, TInt aLength)
    {
	OstTraceExt4(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEDELETENOTIFY, "this %x aPos %x:%x aLength %d", (TUint) this, (TUint) I64HIGH(aPos), (TUint) I64LOW(aPos), (TUint) aLength);
    TInt r = iProxy->DeleteNotify(aPos, aLength);
	OstTrace1(TRACE_DRIVE, PROXYDRIVE_ECBASEEXTPROXYDRIVEDELETENOTIFYRET, "r %d", r);
	return r;
    }


/**
Constructor.
*/
EXPORT_C CProxyDriveFactory::CProxyDriveFactory()
	{}


/**
Remove the Proxy driver factory.

This implementation performs no operations.

@return KErrNone
*/
EXPORT_C TInt CProxyDriveFactory::Remove()
	{
	return(KErrNone);
	}

GLDEF_C CExtProxyDriveFactory* GetProxyDriveFactory(const TDesC& aName)
//
// Lookup an extension by name.
//
	{

	TInt h=0;
	TInt r=ProxyDrives->FindByName(h,aName);
	if (r!=KErrNone)
		return(NULL);
	return((CExtProxyDriveFactory*)ProxyDrives->At(h));
	}


// construct a extension proxy drive device
EXPORT_C CExtProxyDriveFactory::CExtProxyDriveFactory()
	{
	}


EXPORT_C TInt CExtProxyDriveFactory::Remove()
	{
	return KErrNone;
	}


EXPORT_C void CExtProxyDriveFactory::AsyncEnumerate()
	{
	}


/**
Create a proxy drive using the local proxy drive passed in
and any extensions that have been added to the drive.

@param aConcreteDrive local proxy drive
@param aMount local proxy drive mount control block

@return pointer to instantiated CProxyDrive object.
*/
EXPORT_C CProxyDrive* CreateProxyDriveL(CProxyDrive* aConcreteDrive,CMountCB* aMount)
	{
	__PRINT(_L("CreateProxyDriveL()"));
	__ASSERT_DEBUG(aMount!=NULL,Fault(ECreateProxyDriveL));
	TDrive& drive=TheDrives[aMount->Drive().DriveNumber()];
	if(drive.ExtInfo().iCount==0)
		return(aConcreteDrive);

	TBool extSupported = drive.FSys().IsExtensionSupported();
	OstTraceExt2(TRACE_DRIVE, FSYS_ECFILESYSTEMISEXTENSIONSUPPORTED2, "%x r %d", (TUint) &drive.FSys(), (TUint) extSupported);
	if(!extSupported)
		{
		delete(aConcreteDrive);
		User::Leave(KErrAccessDenied);
		}
	CProxyDrive* pOrig=aConcreteDrive;
	CProxyDrive* pFinal=NULL;
	__ASSERT_DEBUG(drive.ExtInfo().iCount<=KMaxExtensionCount,Fault(EExtensionInfoCount2));
	for(TInt i=0;i<drive.ExtInfo().iCount;++i)
		{
		__PRINT1TEMP(_L("adding extension %S"),drive.ExtInfo().iInfo[i].iFactory->Name());
		__PRINT1(_L("extension is primary = %d"),drive.ExtInfo().iInfo[i].iIsPrimary);
		TRAPD(r,pFinal=drive.ExtInfo().iInfo[i].iFactory->NewProxyDriveL(pOrig,aMount));
		if(r!=KErrNone)
			{
			delete(pOrig);
			User::Leave(r);
			}
		pOrig=pFinal;
		}
	return(pOrig);
	}

/**
Lookup a proxy drive extension by name.

@param aName name of extension to be found

@return system wide error code
*/
CProxyDriveFactory* GetExtension(const TDesC& aName)
	{

	TInt h=0;
	TInt r=Extensions->FindByName(h,aName);
	if (r!=KErrNone)
		return(NULL);
	return((CProxyDriveFactory*)Extensions->At(h));
	}


// construct a extension proxy drive
EXPORT_C CExtProxyDrive::CExtProxyDrive(CMountCB* aMount,CExtProxyDriveFactory* aDevice)
  : CProxyDrive(aMount),
	iFactory(aDevice)
	{
	}

// delete a extension proxy drive
EXPORT_C CExtProxyDrive::~CExtProxyDrive()
	{
	if(iMediaChangeNotifier)
		{
		delete iMediaChangeNotifier;
		}
	}


EXPORT_C TInt CExtProxyDrive::NotifyChange(TDes8 &/*aChanged*/, TRequestStatus* /*aStatus*/)
	{
	return KErrNotSupported;
	}	

EXPORT_C void CExtProxyDrive::NotifyChangeCancel()
	{
	}	

EXPORT_C TInt CExtProxyDrive::SetInfo(const RMessage2& /*aMsg*/, TAny* /*aMessageParam2*/, TAny* /*aMessageParam3*/)
    {
	return KErrNone;
    }

/**
Initialise the provided interface extension.

@param aInterfaceId Interface identifier of the interface to be retrieved.
@param aInterface Address of variable that retrieves the specified interface.
@param aInput Data required for the instantiation of the interface.

@return system wide error code.
*/	
EXPORT_C TInt CExtProxyDrive::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
	{
	return(CProxyDrive::GetInterface(aInterfaceId,aInterface,aInput));
	}	

TInt CExtProxyDrive::SetupMediaChange()
	{
	if(iMediaChangeNotifier)
		{
		TRAPD(err, iMediaChangeNotifier->RequestL());
		return err;
		}

	TRAPD(err, iMediaChangeNotifier = CExtNotifyMediaChange::NewL(this));	

	return(err == KErrNotSupported ? KErrNone : err);
	}

TInt TFsAddExtension::DoRequestL(CFsRequest* aRequest)
//
// Add an extension
//
	{
	__PRINT(_L("TFsAddExtension::DoRequestL(CFsRequest* aRequest)"));
	
	RLibrary lib;
	// Get library handle
	lib.SetHandle(aRequest->Message().Int0());
	if (lib.Type()[1]!=TUid::Uid(KFileSystemUidValue))
		return KErrNotSupported;

    TExtensionNew e=(TExtensionNew)lib.Lookup(1);
	if (!e)
		return KErrCorrupt;
	CProxyDriveFactory* pP=(*e)();
	if(!pP)
		return KErrNoMemory;
	TInt r=pP->Install();
	__PRINT1TEMP(_L("InstallExtension %S"),pP->Name());
	if (r==KErrNone)
		{
		__PRINT(_L("TRAP(r,Extensions->AddL(pP,ETrue))"));
		TRAP(r,Extensions->AddL(pP,ETrue))
		__PRINT1TEMP(_L("r == %d"), r);
		if(r!=KErrNone)
			pP->Remove();
		}
	__PRINT1TEMP(_L("r == %d"), r);
	if (r==KErrNone)
		pP->SetLibrary(lib);
	else
		pP->Close();
	return(r);
	}


TInt TFsAddExtension::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TSecurityPolicy policy(RProcess().SecureId(), ECapabilityTCB);
	if (!policy.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Add File System Extension")))
		return KErrPermissionDenied;
	return KErrNone;
	}

TInt TFsAddProxyDrive::DoRequestL(CFsRequest* aRequest)
//
// Load a proxy drive
//
	{
	__PRINT(_L("TFsAddProxyDrive::DoRequestL(CFsRequest* aRequest)"));
	
	RLibrary lib;
	// Get library handle
	lib.SetHandle(aRequest->Message().Int0());
	if (lib.Type()[1]!=TUid::Uid(KFileSystemUidValue))
		return KErrNotSupported;

    TProxyDriveNew e=(TProxyDriveNew)lib.Lookup(1);
	if (!e)
		return KErrCorrupt;
	CExtProxyDriveFactory* pP=(*e)();
	if(!pP)
		return KErrNoMemory;
	TInt r=pP->Install();
	__PRINT1TEMP(_L("Install Proxy Drive %S"),pP->Name());
	if (r==KErrNone)
		{
		__PRINT(_L("TRAP(r,ProxyDrives->AddL(pP,ETrue))"));
		TRAP(r,ProxyDrives->AddL(pP,ETrue))
		__PRINT1TEMP(_L("r == %d"), r);
		if(r!=KErrNone)
			pP->Remove();
		}
	__PRINT1TEMP(_L("r == %d"), r);
	if (r==KErrNone)
		pP->SetLibrary(lib);
	else
		pP->Close();
	return(r);
	}


TInt TFsAddProxyDrive::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TSecurityPolicy policy(RProcess().SecureId(), ECapabilityTCB);
	if (!policy.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Add File System Proxy Drive")))
		return KErrPermissionDenied;
	return KErrNone;
	}

TInt TFsMountExtension::DoRequestL(CFsRequest* aRequest)
//
// Mount an extension
//
	{
	TFullName name;
	aRequest->ReadL(KMsgPtr0,name);
	CProxyDriveFactory* pE=GetExtension(name);
	if (pE==NULL)
		return(KErrNotFound);
	return(aRequest->Drive()->MountExtension(pE,EFalse));
	}

TInt TFsMountExtension::Initialise(CFsRequest* aRequest)
	{
    TInt r;

    //-- check extension name length. It should not exceed KMaxFSNameLength (32 characters)
    r = aRequest->GetDesLength(KMsgPtr0);
    if(r <=0 || r >KMaxFSNameLength)
        return KErrArgument;
    
    r = ValidateDrive(aRequest->Message().Int1(),aRequest);
	if(r!=KErrNone)
		return r;
	
	if(aRequest->Drive()->IsSubsted())
		return KErrNotSupported;
	
    return r;
	}


TInt TFsDismountExtension::DoRequestL(CFsRequest* aRequest)
//
// Dismount extension
//
	{
	TFSName name;
	aRequest->ReadL(KMsgPtr0,name);
	CProxyDriveFactory* pE=GetExtension(name);
	if (pE==NULL)
		return(KErrNotFound);
	return(aRequest->Drive()->DismountExtension(pE,EFalse));
	}


TInt TFsDismountExtension::Initialise(CFsRequest* aRequest)
	{
    TInt r;

    //-- check extension name length. It should not exceed KMaxFSNameLength (32 characters)
    r = aRequest->GetDesLength(KMsgPtr0);
    if(r <=0 || r >KMaxFSNameLength)
        return KErrArgument;
	
	if (!KCapFsDismountExtension.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Dismount File Extension")))
		return KErrPermissionDenied;

	r = ValidateDrive(aRequest->Message().Int1(),aRequest);
	if(r!=KErrNone)
		return r;

	if(aRequest->Drive()->IsSubsted())
		return(KErrNotSupported);
	
    return r;
	}

TInt TFsRemoveExtension::DoRequestL(CFsRequest* aRequest)
//
// Remove an extension
//
	{
	TFullName name;
	aRequest->ReadL(KMsgPtr0,name);
	CProxyDriveFactory* pE=GetExtension(name);
	if (pE==NULL)
		return(KErrNotFound);
	TInt r=pE->Remove();
	if (r!=KErrNone)
		return(r);
	RLibrary lib=pE->Library();
	pE->Close();
	lib.Close();
	return(KErrNone);
	}


TInt TFsRemoveExtension::Initialise(CFsRequest* aRequest)
//
//
//
	{
	if (!KCapFsRemoveExtension.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Remove File Extension")))
		return KErrPermissionDenied;
	return KErrNone;
	}

TInt TFsRemoveProxyDrive::DoRequestL(CFsRequest* aRequest)
//
// Remove a proxy drive
//
	{
	TFullName name;
	aRequest->ReadL(KMsgPtr0,name);

	CExtProxyDriveFactory* pD=GetProxyDriveFactory(name);
	// are there any mounted drives using this extension?
	if (LocalDrives::IsProxyDriveInUse(pD)) return KErrInUse;
	if (pD==NULL)
		return(KErrNotFound);
	TInt r=pD->Remove();
	if (r!=KErrNone)
		return(r);
	RLibrary lib=pD->Library();
	pD->Close();
	lib.Close();

	return(KErrNone);
	}


TInt TFsRemoveProxyDrive::Initialise(CFsRequest* aRequest)
//
//
//
	{
	if (!KCapFsRemoveProxyDrive.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Remove Proxy Drive")))
		return KErrPermissionDenied;
	return KErrNone;
	}

TInt TFsExtensionName::DoRequestL(CFsRequest* aRequest)
//
// Return the name of an extension for a given drive and extension chain position
//
	{
	TFullName name;
	TInt r=aRequest->Drive()->ExtensionName(name,aRequest->Message().Int2());
	if(r==KErrNone)
		aRequest->WriteL(KMsgPtr0,name);
	return(r);
	}

TInt TFsExtensionName::Initialise(CFsRequest* aRequest)
//
//
//
	{
	TInt r=ValidateDrive(aRequest->Message().Int1(),aRequest);
	if(r!=KErrNone)
		return(r);
	if(aRequest->Drive()->IsSubsted())
		return(KErrNotSupported);
	return(r);
	}

TInt TFsDismountProxyDrive::DoRequestL(CFsRequest* aRequest)
//
// Dismount a proxy extension
//
	{

	__PRINT(_L("TFsDismountProxyDrive::DoRequestL"));

	return aRequest->Drive()->DismountProxyDrive();
	}


TInt TFsDismountProxyDrive::Initialise(CFsRequest* aRequest)
//
//
//
	{
	if (!KCapFsDismountProxyDrive.CheckPolicy(aRequest->Message(), __PLATSEC_DIAGNOSTIC_STRING("Dismount Proxy Drive")))
		return KErrPermissionDenied;

	TInt r=ValidateDrive(aRequest->Message().Int0(),aRequest);
	if(r!=KErrNone)
		return(r);

	return KErrNone;
	}


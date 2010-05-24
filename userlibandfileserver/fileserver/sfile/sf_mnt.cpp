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
//

#include "sf_std.h"

#ifdef OST_TRACE_COMPILER_IN_USE
#include "sf_mntTraces.h"
#endif




/**
Default constructor.
*/
EXPORT_C CMountCB::CMountCB()
	              :iMountQ(_FOFF(CFileCB,iMountLink))
	{
    __PRINT1(_L("CMountCB::CMountCB()[0x%x]"),this);
	}




EXPORT_C CMountCB::~CMountCB()
	{
	__PRINT1(_L("CMountCB::~CMountCB()[0x%x]"),this);
	delete iBody;
	delete iVolumeName;
	}


//-----------------------------------------------------------------------------

/**
    Compares the specified mount control block with this one for inequality.

    Two mount control blocks are different if either the addresses of
    the mount control blocks ar different, or their iMountNumber values
    are different.

    @param aMount The mount control block to be compared.

    @return True, if the mount control blocks are different, false otherwise.
*/
EXPORT_C TBool CMountCB::operator!=(const CMountCB& aMount) const
	{

	if (this==(&aMount) && MountNumber()==aMount.MountNumber())
		return(FALSE);
	return(TRUE);
	}


//-----------------------------------------------------------------------------

/**
    Tests whether the given attribute mask matches given criteria (specified
    in another mask).

    This function is used when filtering entries. aMatt specifies the criteria
    to be matched by anAtt.

    Volumes are ignored, so if anAtt has the volume attribute set then
    the function returns false.

    If aMatt does not have a hidden (KEntryAttHidden), system (KEntryAttSystem),
    or directory (KEntryAttDir) attribute set and anAtt does have
    the corresponding attribute set, then the function returns false.
    Alternatively, if aMatt has KEntryAttMustBeFile set and anAtt has
    the directory attribute set, then the function also returns EFalse.

    Further matching behaviour can be implemented using
    KEntryAttMatchExclusive and KEntryAttMatchExclude.

    @param anAtt Attribute mask to be tested.
    @param aMatt The attribute match mask.

    @return True, if the anAtt entry attributes match, false otherwise.

    @see KEntryAttHidden
    @see KEntryAttSystem
    @see KEntryAttDir
    @see KEntryAttMustBeFile
    @see KEntryAttMatchExclusive
    @see KEntryAttMatchExclude
*/
EXPORT_C TBool CMountCB::MatchEntryAtt(TUint anAtt,TUint aMatt) const
    {
	if (aMatt&KEntryAttMatchExclude)
		{ // Include any except
		if ((anAtt&aMatt)==0)
			return(ETrue);
		return(EFalse);
		}

	anAtt&=KEntryAttMaskSupported;
	if ((aMatt&KEntryAttMustBeFile) && (anAtt&KEntryAttIllegal))
		return(EFalse); // Not a file

	if ((aMatt&KEntryAttHidden)==0 && (anAtt&KEntryAttHidden))
		return(EFalse); // Ignore hidden unless requested
	if ((aMatt&KEntryAttSystem)==0 && (anAtt&KEntryAttSystem))
		return(EFalse); // Ignore system unless requested
	if ((aMatt&KEntryAttDir)==0 && (anAtt&KEntryAttDir))
		return(EFalse); // Ignore directory unless requested
	if (anAtt&KEntryAttVolume)
		return(EFalse); // Ignore volumes

	anAtt&=(~(KEntryAttHidden|KEntryAttSystem)); // remove hidden and system

	if (aMatt&KEntryAttMatchExclusive)
		{ // Exclude all except
		if ((anAtt&aMatt)!=0)
			return(ETrue);
		return(EFalse);
		}
	return(ETrue);
	}



//-----------------------------------------------------------------------------
/**
    Gets the address of the specified file if it is found in ROM. 

    The default implementation sets aFileStart to NULL, and this should only
    be overridden by ROM file systems.

    @param aFileName  A reference to a descriptor containing the full file name.
    @param aFileStart On return, the address of the file, aFileName.
                      The default implementation returns NULL.
*/
EXPORT_C void CMountCB::IsFileInRom(const TDesC& /*aFileName*/,TUint8*& aFileStart)
	{

	aFileStart=NULL;
	}


//-----------------------------------------------------------------------------

/**
    Notifies the file server that the free disk space of the mounted volume
    has changed outside of the standard file system operations.

    For example, the background thread of the log flash file system will call
    this function after a background roll-forward.

    @param aFreeSpace New free disk space value.
*/
EXPORT_C void CMountCB::SetDiskSpaceChange(TInt64 aFreeSpace)
	{
	const TInt drv=Drive().DriveNumber();
	__ASSERT_ALWAYS(aFreeSpace>=0,Fault(ESvrFreeDiskSpace));
	__PRINT3(_L("CMountCB::SetDiskSpaceChange(%LU) drv:%d, %dKB"), aFreeSpace, drv, (TUint32)(aFreeSpace>>10));

	// Notifying involves memory de-allocation on the file server's heap -
	// check if we need to switch heaps.
	RAllocator* current_alloc = &User::Heap();
	RAllocator* svr_alloc = ServerThreadAllocator;
	if (current_alloc != svr_alloc)
		User::SwitchHeap(svr_alloc);

	FsNotify::HandleDiskSpace(drv, aFreeSpace);
		
		if (current_alloc != svr_alloc)
			User::SwitchHeap(current_alloc);
	}
	
//-----------------------------------------------------------------------------
/**
    Initialize the MountCB object. 
    
    @param  aDrive          TDrive object that will be used by the mount for drive access
    @param  apFileSystem    pointer to the File System object that has produced this CMountCB
*/
EXPORT_C void CMountCB::InitL(TDrive& aDrive, CFileSystem* apFileSystem)
	{
	__PRINT3(_L("CMountCB::InitL()[0x%x] drv:%d, FS:0x%x"), this, aDrive.DriveNumber(),  apFileSystem);
    ASSERT(apFileSystem);
    

    SetDrive(&aDrive);
    DoInitL(aDrive.DriveNumber());

	// see whether the file system supports the CFileCB extended API 
	MFileAccessor* fileAccessor = NULL;

	GetInterfaceTraced(CMountCB::EFileAccessor, (void*&) fileAccessor, NULL);

	MFileExtendedInterface* fileExtInterface = NULL;
	GetInterface(CMountCB::EFileExtendedInterface, (void*&) fileExtInterface, NULL);

    if(!iBody)
        {
        iBody = new(ELeave)CMountBody(this, fileAccessor, fileExtInterface);
        }
    else
        {
        //-- some file systems can call this method several times for the same mount.
        //-- composite FS, for example.
        __PRINT1(_L("CMountCB::InitL !!re-initialisation!! iBody:0x%x"), iBody);
        }
	
    SetFileSystem(apFileSystem); //-- associate MountCB object with the file system it belongs to; this relies on iBody
    }

/**
Reports whether the specified interface is supported - if it is,
the a supplied interface object is modified to it

@param aInterfaceId		The interface of interest
@param aInterface		The interface object
@return					KErrNone if the interface is supported, otherwise KErrNotFound 
*/
EXPORT_C TInt CMountCB::GetInterface(TInt /*aInterfaceId*/,TAny*& /*aInterface*/,TAny* /*aInput*/)
	{
	return(KErrNotSupported);
	}

/**
Creates a clamp for the named file, and provides clamp handle data to the caller.

@param aDriveNo		The number of the drive on which the file can be found
@param aName		Name of the file to clamp
@param aHandle		Pointer to the clamp handle data
@return				KErrNone if successful, otherwise one of the systme-wide eror codes.
*/
TInt CMountCB::ClampFile(const TInt aDriveNo, const TDesC& aName, TAny* aHandle)
	{
	return(iBody?iBody->ClampFile(aDriveNo,aName,aHandle):KErrNotSupported);
	};

/**
Reports whether the named file is clamped.

@param aName		Name of the file to clamp
@return				0 if the file is not clamped, 1 if it is, or a system-wide error code.
*/
EXPORT_C TInt CMountCB::IsFileClamped(/*const TDesC& aName,*/ const TInt64 aUniqueId)
	{
	return(iBody?iBody->IsFileClamped(aUniqueId):KErrNotSupported);
	};

/**
Removes the clamp indicated by the specified handle.
If this was the last clamp for the drive, any pending dismount is performed.

@param aHandle		Pointer to the clamp handle data
@return				KErrNone if successful, a system-wide error code otherwise
*/
TInt CMountCB::UnclampFile(RFileClamp* aHandle)
	{
	return(iBody?iBody->UnclampFile(aHandle):KErrNotSupported);
	};

/**
Returns the current number of clamps 

@return				the number of clamps
*/
TInt CMountCB::NoOfClamps()
	{
	return(iBody?iBody->NoOfClamps():KErrNotSupported);
	}




/**
Gets the sub type name of mounted file system (e.g. FAT12, FAT16 or FAT32 of Fat file system).
Uses GetInterface() API to avoid binary compatibility break while providing polymorphism.
The real implementations is done by classes multiple inherited from both CMountCB and
MFileSystemSubType that have implemented MFileSystemSubType::SubType() virtual function.
File system that do not support sub types will have KErrNotSupported returned.

@param aSubTypeName A descriptor contains return of the sub type name.
@return KErrNone if successful; otherwise another of the system wide error codes; 
		KErrNotSupported if sub type is not supported by mounted file system;

@see MFileSystemSubType::SubType()
*/
TInt CMountCB::FileSystemSubType(TDes& aName)
	{
	MFileSystemSubType* interface = NULL;
	TAny* dummy = NULL;
	TInt rel = GetInterfaceTraced(EGetFileSystemSubType, (TAny*&)(interface), dummy);
	if((interface != NULL) && (rel == KErrNone))
		{
		rel = interface->SubType(aName);
		}
	
	return rel;
	}

/**
Gets the cluster size of mounted file system.
Uses GetInterface() API to avoid binary compatibility break while providing polymorphism.
The real implementation is done by classes multiple inherited from both CMountCB and
MFileSystemClusterSize that have implemented MFileSystemClusterSize::ClusterSize() function.
File system that do not support concept of 'clusters' will have the default KErrNotSupported returned.

@return Cluster size value if successful; otherwise another of the system wide error codes; 
		KErrNotSupported if cluster is not supported by mounted file system;

@see MFileSystemClusterSize::ClusterSize()
*/
TInt CMountCB::FileSystemClusterSize()
	{
	MFileSystemClusterSize* interface = NULL;
	TAny* dummy = NULL;
	TInt rel = GetInterfaceTraced(EGetClusterSize, (TAny*&)(interface), dummy);
	if((interface != NULL) && (rel == KErrNone))
		{
		rel = interface->ClusterSize();
		}
		
	return rel;
	}


/**
@prototype

Reads a specified section of the file, regardless of the file's lock state.

Uses GetInterface() API to avoid binary compatibility break while providing polymorphism.
The real implementation is done by classes multiple inherited from both CMountCB and
MFileSystemExtendedInterface that have implemented MFileSystemExtendedInterface::ExtendedReadSectionL() function.
File system that do not support large file will implement the default CMountCB::ReadSectionL() function.

@see CMountCB::ReadSectionL()

@see MFileSystemExtendedInterface::ExtendedReadSectionL()
*/
void CMountCB::ReadSection64L(const TDesC& aName,TInt64 aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage)
	{
	iBody->iFileExtendedInterface->ReadSection64L(aName, aPos, aTrg, aLength, aMessage);
	}

TInt CMountCB::GetFileUniqueId(const TDesC& aName, TInt64& aUniqueId)
	{
	return (iBody->iFileAccessor->GetFileUniqueId(aName, aUniqueId));
	}
	
TInt CMountCB::Spare3(TInt aVal, TAny* aPtr1, TAny* aPtr2)
	{
	return (iBody->iFileAccessor->Spare3(aVal, aPtr1, aPtr2));
	}
	
TInt CMountCB::Spare2(TInt aVal, TAny* aPtr1, TAny* aPtr2)
	{
	return (iBody->iFileAccessor->Spare2(aVal, aPtr1, aPtr2));
	}
	
TInt CMountCB::Spare1(TInt aVal, TAny* aPtr1, TAny* aPtr2)
	{
	return (iBody->iFileAccessor->Spare1(aVal, aPtr1, aPtr2));
	}

TInt CMountCB::GetInterfaceTraced(TInt aInterfaceId, TAny*& aInterface, TAny* aInput)
	{
	OstTraceExt3(TRACE_FILESYSTEM, FSYS_ECMOUNTCBGETINTERFACE, "drive %d aInterfaceId %d aInput %x", (TUint) DriveNumber(), (TUint) aInterfaceId, (TUint) aInput);

	TInt r = GetInterface(aInterfaceId, aInterface, aInput);

	OstTraceExt2(TRACE_FILESYSTEM, FSYS_ECMOUNTCBGETINTERFACERET, "r %d aInterface %x", (TUint) r, (TUint) aInterface);

	return r;
	}

//-----------------------------------------------------------------------------
/**
    Get the file system name. I.e. The name of the file system that produced this object of CMountCB
    @param  aName buffer for the name
*/
void CMountCB::FileSystemName(TDes& aName)
{
    aName = FileSystem()->Name();
}



//-----------------------------------------------------------------------------
/**
    Associate CFileSystem object (the factory) and the produced CMountCB object.
    @param  aFS pointer to the file system that produced this mount.
*/
void CMountCB::SetFileSystem(CFileSystem* aFS)
    {
    ASSERT(iBody);
    iBody->SetFileSystem(aFS);
    }

/**
    Get reference to the filesystem, associated with this mount.
*/
EXPORT_C CFileSystem* CMountCB::FileSystem() const
    {
    ASSERT(iBody);
    CFileSystem* pFSys = iBody->GetFileSystem();
    ASSERT(pFSys);
    return pFSys;
    }

void CMountCB::SetProxyDriveDismounted()
	{
	iBody->SetProxyDriveDismounted();
	}

TBool CMountCB::ProxyDriveDismounted()
	{
	return iBody->ProxyDriveDismounted();
	}


/**
    Factory method. Produces CFileCB object.
*/
CFileCB* CMountCB::NewFileL() const
    {
    return FileSystem()->NewFileL();
    }

/**
    Factory method. Produces CDirCB object.
*/
CDirCB* CMountCB::NewDirL() const
    {
    return FileSystem()->NewDirL();
    }

    
/**
    Factory method. Produces CFormatCB object.
*/
CFormatCB* CMountCB::NewFormatL() const
    {
    return FileSystem()->NewFormatL();
    }

















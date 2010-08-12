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
// This file system extension provides a way to access a drive on the MS system
// in "raw format". It can be used to test large files / drives
//

/** @file
@internalTechnology
*/

#include <f32fsys.h>

#include "hostusbmsproxy.h"
#include "debug.h"


CUsbHostMsProxyDrive::CUsbHostMsProxyDrive(CMountCB* aMount, CExtProxyDriveFactory* aDevice)
:   CExtProxyDrive(aMount,aDevice)
	{
	__MSFNSLOG
	}

CUsbHostMsProxyDrive::~CUsbHostMsProxyDrive()
	{
	__MSFNSLOG
	iUsbHostMsLun.UnInitialise();
	}

TInt CUsbHostMsProxyDrive::InitialiseOffset(TCapsInfo& aCapsInfo)
	{
	__MSFNSLOG
    RBuf8 partitionInfo;
    TInt r;
    TRAP(r, partitionInfo.CreateL(aCapsInfo.iBlockLength));
    if (r != KErrNone)
        {
        return r;
        }

	r = iUsbHostMsLun.Read(0, aCapsInfo.iBlockLength, partitionInfo);
	if (r != KErrNone)
        {
		__PXYPRINT1(_L("!! Reading medium failed with %d !!"), r);
        partitionInfo.Close();
		return r;
        }
	TUint8* buffer = const_cast<TUint8*>(partitionInfo.Ptr());

	// Read of the first sector successful so check for a Master Boot Record
	if (*(reinterpret_cast<TUint16*>(&buffer[KMBRSignatureOffset]))!= KMBRSignature)
		{
		__PXYPRINT(_L("MBR not present"));
        iMsDataMemMap.Reset();
        }
	else
		{
		// Move the partition entries to a 4 byte boundary
		memcpy(&buffer[0],&buffer[KMBRFirstPartitionOffset],(sizeof(TMBRPartitionEntry)<<2));
		// Search for a x86 default boot partition - let this be the first
		TMBRPartitionEntry* pe = reinterpret_cast<TMBRPartitionEntry*>(&buffer[0]);

		TInt firstValidPartitionCount = -1;
		TInt defaultPartitionNumber = -1;
		TInt partitionCount = 0;
		for (TInt i = 0; i < KMBRMaxPrimaryPartitions; i++, pe++)
			{
			if (pe->IsValidDosPartition() || pe->IsValidFAT32Partition() || pe->IsValidExFATPartition())
				{
				__PXYPRINT(_L("Found a Valid Partition"));
				partitionCount++;

				if (firstValidPartitionCount < 0)
					firstValidPartitionCount = i;

				if (pe->iX86BootIndicator == KBootIndicatorBootable)
                    {
					defaultPartitionNumber = i;
					break;
                    }
				}
			else
				{
				__PXYPRINT(_L("!! Invalid Partition !!"));
				}
			}

		// Check the validity of the partition address boundaries
	    if (partitionCount > 0)
		    {
            __PXYPRINT1(_L("Using Partition %d"), partitionCount);
			pe = reinterpret_cast<TMBRPartitionEntry*>(&buffer[0]);
            TInt partitionIndex = firstValidPartitionCount;
            if (defaultPartitionNumber > 0)
                {
                partitionIndex = defaultPartitionNumber;
                }

            TMBRPartitionEntry& partitionEntry = pe[partitionIndex];

			iMsDataMemMap.InitDataArea(partitionEntry.iFirstSector,
                                       partitionEntry.iNumSectors,
                                       aCapsInfo.iBlockLength);
			__PXYPRINT2(_L("paritioncount = %d defaultpartition = %d"),
						partitionCount, partitionIndex);
			__PXYPRINT3(_L("iFirstSector = x%x iNumSectors = x%x iSectorSize = x%x"),                         
						partitionEntry.iFirstSector,
						partitionEntry.iNumSectors,
                        aCapsInfo.iBlockLength);
			}
		else
			{
            __PXYPRINT(_L("No partition found"));
			iMsDataMemMap.InitDataArea(0, aCapsInfo.iNumberOfBlocks, aCapsInfo.iBlockLength);
			__PXYPRINT3(_L("iFirstSector = x%x iNumSectors = x%x iSectorSize = x%x"),
						0, 
                        aCapsInfo.iNumberOfBlocks,
                        aCapsInfo.iBlockLength);
			}
		}

    partitionInfo.Close();
	return KErrNone;
	}

/**
Initialise the proxy drive.
@return system wide error code.
*/
TInt CUsbHostMsProxyDrive::Initialise()
	{
	__MSFNSLOG
    __HOSTPRINT(_L(">>> CUsbHostMsProxyDrive::Initialise()"));

	if(Mount())
		{
		// as we can't currently handle remounting devices that have
		// been removed by unplugging the USB cable, disable critical notifiers
		// as there's no point in asking the user to re-insert the disk.
		Mount()->SetNotifyOff();
		}

    // Check for media presence
	TCapsInfo capsInfo;
	TInt err = iUsbHostMsLun.Caps(capsInfo);

    if (err == KErrNone && capsInfo.iMediaType == EMediaHardDisk)
        {
        err = InitialiseOffset(capsInfo);
        }

    __HOSTPRINT1(_L("<<< CUsbHostMsProxyDrive::Initialise() err = %d"), err);
    return err;
	}

TInt CUsbHostMsProxyDrive::SetInfo(const RMessage2 &msg, TAny* aMessageParam2, TAny* aMessageParam3)
    {
	__MSFNSLOG
    __HOSTPRINT(_L(">>> CUsbHostMsProxyDrive::SetInfo()"));
	TMassStorageUnitInfo iUnitInfo;
    TPckg<TMassStorageUnitInfo> infoPckg(iUnitInfo);
	TRAPD(err, msg.ReadL(2, infoPckg));

	if(err != KErrNone)
		{
		__PXYPRINT1(_L("Cant read from the RMessage %d"), err);
        __HOSTPRINT1(_L("<<< CUsbHostMsProxyDrive::SetInfo() err = %d"), err);
		return err;
		}

	err = iUsbHostMsLun.Initialise(msg, 3, iUnitInfo.iLunID);
	if(err != KErrNone)
		{
		__PXYPRINT1(_L("Initialising logical unit failed %d"), err);
        __HOSTPRINT1(_L("<<< CUsbHostMsProxyDrive::SetInfo() err = %d"), err);
		return err;
		}

    __HOSTPRINT1(_L("<<< CUsbHostMsProxyDrive::SetInfo() err = %d"), err);
	return err;
    }

TInt CUsbHostMsProxyDrive::Dismounted()
	{
	__MSFNSLOG
	return KErrNone;
	}

TInt CUsbHostMsProxyDrive::Enlarge(TInt /*aLength*/)
	{
	__MSFNSLOG
	return KErrNotSupported;
	}


TInt CUsbHostMsProxyDrive::ReduceSize(TInt /*aPos*/, TInt /*aLength*/)
	{
	__MSFNSLOG
	return KErrNotSupported;
	}

#define GetIndex(msg, aAddress, aIndex)			\
	aIndex = msg.Ptr0() == aAddress ? 0 :				\
				msg.Ptr1() == aAddress ? 1 :			\
					msg.Ptr1() == aAddress ? 2 :		\
						msg.Ptr1() == aAddress ? 3 : -1;

/**
Read from the proxy drive.

@param aPos    The address from where the read begins.
@param aLength The length of the read.
@param aTrg    A descriptor of the memory buffer from which to read.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aTrg to read the data from.

@return system wide error code.
*/
TInt CUsbHostMsProxyDrive::Read(TInt64 aPos, TInt aLength,
                                const TAny* aTrg, TInt aThreadHandle, TInt aOffset)
	{
	__MSFNSLOG
    __HOSTPRINT4(_L("\n>>> HOST Read Pos=0x%lx LBA=0x%lx 0x%x 0x%x"),
                 aPos, aPos/KBlockSize, aLength, aOffset);

	TBool localMessage = (aThreadHandle == KLocalMessageHandle);

	//
	// Set file position to where we want to read...
	//
	if(!localMessage)
		{
		RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
		localMessage = (msg.Handle() == KLocalMessageHandle);
		}

	TInt index = 0;
	if (!localMessage)
		{
		RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
		GetIndex(msg, aTrg, index);

		if (index < 0)
            {
            __HOSTPRINT1(_L("<<< HOST Read ret=%d"), KErrArgument);
            return KErrArgument;
            }
		}

	/* Calculate the end position */
	TInt64 end = aPos + static_cast<TInt64>(aLength);

	/* check whether there is enough source data to write to the destination descriptor */
	TInt64 truncate;
	if(localMessage)
		{
		truncate = aLength - (((TPtr8* )aTrg)->MaxLength() - aOffset);
	    __PXYPRINT1(_L("Descriptor length: %08x"), ((TPtr8* )aTrg)->MaxLength());
		}
	else
		{
		RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
		truncate = aLength - (msg.GetDesMaxLength(index) - aOffset);
	    __PXYPRINT1(_L("Descriptor length: %08x"), msg.GetDesMaxLength(index));
		}

	__PXYPRINT1(_L("Offset: %08x"), aOffset);
	__PXYPRINT1(_L("Truncate: 0x%lx"), truncate);

	if (truncate > 0)
		{
		end -= truncate;
		}

	iBuf.SetMax();
    TInt r;
    TInt64 mediaPos;
	while (aPos < end)
        {
		TInt len = end - aPos;
        mediaPos = aPos;
        r = iMsDataMemMap.CheckBlockInRange(mediaPos, len);
        if (r != KErrNone)
            {
            __HOSTPRINT1(_L("<<< HOST Read ret=%d"), r);
            return r;
            }

		if (localMessage)
			{
			TPtr8* pTrgPtr = (TPtr8*)aTrg;
			TPtr8 trgDes((TUint8*)(pTrgPtr->MidTPtr(aOffset).Ptr()), pTrgPtr->MaxLength() - aOffset);
			r = iUsbHostMsLun.Read(mediaPos, len, trgDes);
			if (r != KErrNone)
				return r;
			pTrgPtr->SetLength(aOffset + trgDes.Length());
			}
		else
			{
			if (len > iBuf.MaxLength())
				len = iBuf.MaxLength();

            r = iUsbHostMsLun.Read(mediaPos, len, iBuf);
			if (r != KErrNone)
                {
                __HOSTPRINT1(_L("<<< HOST Read ret=%d"), r);
                return r;
                }

			iBuf.SetLength(len);

			RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
			r = msg.Write(index, iBuf, aOffset);
			if (r != KErrNone)
                {
                __HOSTPRINT1(_L("<<< HOST Read ret=%d"), r);
                return r;
                }
			}

        aPos += len;
        aOffset += len;
        }

    __HOSTPRINT1(_L("<<< HOST Read ret=%d"), KErrNone);
	return KErrNone;
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
TInt CUsbHostMsProxyDrive::Read(TInt64 aPos, TInt aLength,
                                const TAny* aTrg, TInt aThreadHandle, TInt aOffset, TInt /* aFlags */)
	{
	__MSFNSLOG
	return Read(aPos, aLength, aTrg, aThreadHandle, aOffset);
	}

/**
Read from the proxy drive.

@param aPos    The address from where the read begins.
@param aLength The length of the read.
@param aTrg    A descriptor of the memory buffer from which to read.

@return system wide error code.
*/
TInt CUsbHostMsProxyDrive::Read(TInt64 aPos, TInt aLength, TDes8& aTrg)
	{
	__MSFNSLOG
    __HOSTPRINT3(_L("\n>>> HOST Read Pos=0x%lx LBA=0x%lx 0x%x"),
                 aPos, aPos/KBlockSize, aLength);
	return iUsbHostMsLun.Read(iMsDataMemMap.GetDataPos(aPos), aLength, aTrg);
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
TInt CUsbHostMsProxyDrive::Write(TInt64 aPos, TInt aLength,
                                 const TAny* aSrc, TInt aThreadHandle, TInt aOffset)
	{
	//
	// Set file position to where we want to write...
	//
	__MSFNSLOG
    __HOSTPRINT4(_L("\n>>> HOST Write Pos=0x%lx LBA=0%lx 0x%x 0x%x"),
                 aPos, aPos/KBlockSize, aLength, aOffset);

	TBool localMessage = (aThreadHandle == KLocalMessageHandle);

	if(!localMessage)
		{
		RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
		localMessage = (msg.Handle() == KLocalMessageHandle);
		}

	TInt index = 0;
	if(!localMessage)
		{
		RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
		GetIndex(msg, aSrc, index);

		if (index < 0)
			return KErrArgument;
		}

	/* Calculate the end position */
	TInt64 end =  aPos + static_cast<TInt64>(aLength);
	/* check whether there is enough source data to read */
	TInt64 truncate;
	if (localMessage)
		{
		truncate = aLength - (((TPtr8* )aSrc)->Length() - aOffset);
	    __PXYPRINT1(_L("Descriptor length: %08x"), ((TPtr8* )aSrc)->Length());
		}
	else
		{
		RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
		truncate = aLength - (msg.GetDesLength(index) - aOffset);
	    __PXYPRINT1(_L("Descriptor length: %08x"), msg.GetDesLength(index));
		}

	__PXYPRINT1(_L("Offset: %08x"), aOffset);
	__PXYPRINT1(_L("Truncate: 0x%lx"), truncate);

	/* if truncate is > 0  we are short of source data as claimed by the aLength. Hence adjust the 'end' */
	if (truncate > 0)
		{
		end -= truncate;
		}

	iBuf.SetMax();

    TInt r;
    TInt64 mediaPos;
	while (aPos < end)
        {
		TInt len = end - aPos;
        mediaPos = aPos;
        r = iMsDataMemMap.CheckBlockInRange(mediaPos, len);
        if (r != KErrNone)
            {
            __HOSTPRINT1(_L("<<< HOST Write ret=%d"), r);
            return r;
            }

		if (localMessage)
			{
			r = iUsbHostMsLun.Write(mediaPos, len, ((TPtr8*)aSrc)->MidTPtr(aOffset));

			if (r != KErrNone)
                {
                __HOSTPRINT1(_L("<<< HOST Write ret=%d"), r);
                return r;
                }
			}
		else
			{
			if (len > iBuf.Length())
				len = iBuf.Length();

			RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
			r = msg.Read(index, iBuf, aOffset);
			if (r != KErrNone)
                {
                __HOSTPRINT1(_L("<<< HOST Write ret=%d"), r);
                return r;
                }

			r = iUsbHostMsLun.Write(mediaPos, len, iBuf);
			if (r != KErrNone)
                {
                __HOSTPRINT1(_L("<<< HOST Write ret=%d"), r);
                return r;
                }
			}

        aPos += len;
        aOffset += len;
        }

    __HOSTPRINT1(_L("<<< HOST Write ret=%d"), KErrNone);
	return KErrNone;
	}


/**
Write to the proxy drive and pass flags to driver

@param aPos    The address from where the write begins.
@param aLength The length of the write.
@param aSrc    A descriptor of the memory buffer from which to write.
@param aThreadHandle The handle-number representing the drive thread.
@param aOffset Offset into aSrc to write the data to.
@param aFlags  Flags to be passed into the driver.

@return system wide error code.
*/
TInt CUsbHostMsProxyDrive::Write(TInt64 aPos, TInt aLength,
                                 const TAny* aSrc, TInt aThreadHandle, TInt aOffset, TInt /* aFlags */)
	{
	__MSFNSLOG
	return Write(aPos, aLength, aSrc, aThreadHandle, aOffset);
	}

/**
Write to the proxy drive.

@param aPos    The address from where the write begins.
@param aSrc    A descriptor of the memory buffer from which to write.

@return system wide error code.
*/
TInt CUsbHostMsProxyDrive::Write(TInt64 aPos,const TDesC8& aSrc)
	{
	__MSFNSLOG
    __HOSTPRINT3(_L("\n>>> HOST Write Pos=0x%lx LBA=0x%lx 0x%x"),
                 aPos, aPos/KBlockSize, aSrc.Length());
	return iUsbHostMsLun.Write(iMsDataMemMap.GetDataPos(aPos), aSrc.Length(), aSrc);
	}


/**
Get the proxy drive's capabilities information.

@param anInfo A descriptor of the connected drives capabilities.

@return system wide error code
*/
TInt CUsbHostMsProxyDrive::Caps(TDes8& anInfo)
	{
	__MSFNSLOG
    __HOSTPRINT(_L(">>> HOST Caps"));
	TLocalDriveCapsV6Buf caps;
    caps.FillZ();

    TLocalDriveCapsV6& c = caps();

    c.iConnectionBusType = EConnectionBusUsb;
	c.iDriveAtt = KDriveAttLocal | KDriveAttRemovable | KDriveAttExternal;
	c.iMediaAtt = KMediaAttFormattable;
	c.iFileSystemId = KDriveFileSysFAT;

	TCapsInfo capsInfo;
	TInt r = iUsbHostMsLun.Caps(capsInfo);

	if (KErrNone == r)
		{
        c.iType = capsInfo.iMediaType;

        if (capsInfo.iMediaType == EMediaHardDisk)
            {
            c.iBlockSize = capsInfo.iBlockLength;
            TUint64 size = iMsDataMemMap.DataSize();
    
            if (size == 0)
                {
                // No valid partitions so specify the size of the disk
                size = static_cast<TUint64>(capsInfo.iNumberOfBlocks) * capsInfo.iBlockLength;
                }
            c.iSize = size;
    
            c.iEraseBlockSize = 0;
    
            if (capsInfo.iWriteProtect)
                {
                c.iMediaAtt |= KMediaAttWriteProtected;
                }
                
            static const TInt K512ByteSectorSize = 0x200; // 512
            if(K512ByteSectorSize != capsInfo.iBlockLength)
                {
                // not formattable if sector size is not 512
                c.iMediaAtt &= ~KMediaAttFormattable;
                }
            __HOSTPRINT4(_L("<<< HOST Caps Block[num=0x%x size=0x%x] Media[size=0x%lx WP=0x%x]"),
                        capsInfo.iNumberOfBlocks, capsInfo.iBlockLength,
                        caps().iSize, caps().iMediaAtt);
            }
        else if (capsInfo.iMediaType == EMediaCdRom)
            {
            // not formattable
            c.iMediaAtt &= ~KMediaAttFormattable;
            __HOSTPRINT(_L(">>> HOST Caps MediaType = EMediaCdRom"));
            }
        else
            {
            // do nothing
            }
		}
	else if (KErrNotReady == r)
        {
        __HOSTPRINT(_L("<<< HOST Caps Media Not Present"));
		c.iType = EMediaNotPresent;		
		r = KErrNone;
        }
	else if (KErrGeneral == r)
        {
        __HOSTPRINT(_L("<<< HOST Caps Unable to communicate with media"));
		c.iType = EMediaUnknown;		
        }

    else
        {
        __HOSTPRINT(_L("<<< HOST Caps Unknown Error"));
        c.iType = EMediaUnknown;
		r = KErrUnknown;
        }
	anInfo = caps.Left(Min(caps.Length(),anInfo.MaxLength()));
	return r;
	}


/**
Format the proxy drive. The drive is assumed to be a single partition. The
partition size is equivalent to the size of the media.

@param aPos    The position of the data which is being formatted.
@param aLength [IN] The length of the data which is being formatted. [OUT] The
length of data formatted, truncated when end of drive is reached.

@return system wide error code.
*/
TInt CUsbHostMsProxyDrive::Erase(TInt64 aPos, TInt& aLength)
	{
	__MSFNSLOG
    __HOSTPRINT3(_L("\n HOST Erase Pos=0x%lx LBA=0x%lx 0x%x"),
                 aPos, aPos/KBlockSize, aLength);
    TInt err = iMsDataMemMap.TranslateDataPos(aPos, aLength);

    if (err)
        return err;

    err = iUsbHostMsLun.Erase(aPos, aLength);
    return err;
	}


/**
Format the proxy drive.

@param aPos    The position of the data which is being formatted.
@param aLength The length of the data which is being formatted.

@return system wide error code.
*/
TInt CUsbHostMsProxyDrive::Format(TInt64 aPos, TInt aLength)
	{
	__MSFNSLOG
    return Erase(aPos, aLength);
	}


/**
Format the connected drive.

@param anInfo Device specific format information.

@return system wide error code.
*/
TInt CUsbHostMsProxyDrive::Format(TFormatInfo& aInfo)
	{
	__MSFNSLOG

    const TInt KDefaultMaxBytesPerFormat = 0x100 * iMsDataMemMap.BlockLength();  // 128K

    if (aInfo.i512ByteSectorsFormatted < 0)
        return KErrArgument;

    if (!aInfo.iFormatIsCurrent)
        {
        aInfo.iFormatIsCurrent = ETrue;
        aInfo.i512ByteSectorsFormatted = 0;
        aInfo.iMaxBytesPerFormat = KDefaultMaxBytesPerFormat;

		TLocalDriveCapsV6Buf caps;
		TInt r = Caps(caps);
		if (r != KErrNone)
			return r;

        iMsDataMemMap.InitDataArea(caps().iSize);
        }

    TInt64 pos = static_cast<TInt64>(aInfo.i512ByteSectorsFormatted) << iMsDataMemMap.FormatSectorShift();
    TInt length = aInfo.iMaxBytesPerFormat;
    TInt r = Erase(pos, length);

    if (r == KErrNone)
        {
        length += iMsDataMemMap.BlockLength() - 1;
        length >>= iMsDataMemMap.FormatSectorShift();
        aInfo.i512ByteSectorsFormatted += length;
        }

    return r;
    }


TInt CUsbHostMsProxyDrive::NotifyChange(TDes8 &aChanged,TRequestStatus* aStatus)
	{
	__MSFNSLOG
	iUsbHostMsLun.NotifyChange(aChanged, *aStatus);

	if(*aStatus != KRequestPending)
		return KErrUnknown;

	return KErrNone;
	}

void CUsbHostMsProxyDrive::NotifyChangeCancel()
	{
	__MSFNSLOG
	iUsbHostMsLun.NotifyChangeCancel();
	}

TInt CUsbHostMsProxyDrive::SetMountInfo(const TDesC8* /*aMountInfo*/,TInt /*aMountInfoThreadHandle=KCurrentThreadHandle*/)
    {
	__MSFNSLOG
    return KErrNone;
    }

TInt CUsbHostMsProxyDrive::ForceRemount(TUint aFlags)
    {
	__MSFNSLOG
    iUsbHostMsLun.ForceRemount(aFlags);
    return KErrNone;
    }

TInt CUsbHostMsProxyDrive::Unlock(TMediaPassword& /*aPassword*/, TBool /*aStorePassword*/)
    {
	__MSFNSLOG
    return KErrNotSupported;
    }

TInt CUsbHostMsProxyDrive::Lock(TMediaPassword& /*aOldPassword*/, TMediaPassword& /*aNewPassword*/, TBool /*aStorePassword*/)
    {
	__MSFNSLOG
    return KErrNotSupported;
    }

TInt CUsbHostMsProxyDrive::Clear(TMediaPassword& /*aPassword*/)
    {
	__MSFNSLOG
    return KErrNotSupported;
    }

TInt CUsbHostMsProxyDrive::ErasePassword()
    {
	__MSFNSLOG
    return KErrNotSupported;
    }

TInt CUsbHostMsProxyDrive::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
	{
	switch(aInterfaceId)
		{
		case ELocalBufferSupport:
			return KErrNone;
		case EFinalised:
			{
			TBool isFinalised = (TBool)aInput;
			if(isFinalised)
				{
				iUsbHostMsLun.SuspendLun();
				}
			}
			return KErrNone;
		default:
			return KErrNotSupported;
		}
	}

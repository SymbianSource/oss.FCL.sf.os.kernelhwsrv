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
#include "tmbr.h"

#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
static const TUint KBlockSize = 0x200;
#include "hostusbmsproxyTraces.h"
#endif


CUsbHostMsProxyDrive::CUsbHostMsProxyDrive(CMountCB* aMount, CExtProxyDriveFactory* aDevice)
:   CExtProxyDrive(aMount,aDevice)
    {
    }

CUsbHostMsProxyDrive::~CUsbHostMsProxyDrive()
    {
    iUsbHostMsLun.UnInitialise();
    }

TInt CUsbHostMsProxyDrive::InitialiseOffset(TCapsInfo& aCapsInfo)
    {
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
        OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_10,
                  "!! Reading medium failed with %d !!", r);
        }
    else
        {
        TMBRPartitionEntry partitionEntry;
        TInt partitionCount = TMbr::GetPartition(partitionInfo, partitionEntry);

        if (partitionCount == 0)
            {
            OstTrace0(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_11,
                      "No partition found");
            iMsDataMemMap.InitDataArea(0, aCapsInfo.iNumberOfBlocks, aCapsInfo.iBlockLength);
            OstTraceExt3(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_12,
                         "iFirstSector = x%x iNumSectors = x%x iSectorSize = x%x",
                         0,
                         aCapsInfo.iNumberOfBlocks,
                         aCapsInfo.iBlockLength);

            }
        else if (partitionCount > 0)
            {
            iMsDataMemMap.InitDataArea(partitionEntry.iFirstSector,
                                       partitionEntry.iNumSectors,
                                       aCapsInfo.iBlockLength);
            OstTraceExt3(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_13,
                         "iFirstSector = x%x iNumSectors = x%x iSectorSize = x%x",
                         partitionEntry.iFirstSector,
                         partitionEntry.iNumSectors,
                         aCapsInfo.iBlockLength);
            }
        else    // MBR is not valid
            {
            iMsDataMemMap.Reset();
            OstTrace0(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_14,
                      "MBR not present");
            }
        }
    partitionInfo.Close();
    return r;
    }


/**
Initialise the proxy drive.
@return system wide error code.
*/
TInt CUsbHostMsProxyDrive::Initialise()
    {
    OstTrace0(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_20,
              ">>> CUsbHostMsProxyDrive::Initialise()");

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

    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_21,
              "<<< CUsbHostMsProxyDrive::Initialise() err = %d", err);
    return err;
    }

TInt CUsbHostMsProxyDrive::SetInfo(const RMessage2 &msg, TAny* aMessageParam2, TAny* aMessageParam3)
    {
    OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_22,
              ">>> CUsbHostMsProxyDrive::SetInfo()");
    TMassStorageUnitInfo iUnitInfo;
    TPckg<TMassStorageUnitInfo> infoPckg(iUnitInfo);
    TRAPD(err, msg.ReadL(2, infoPckg));

    if(err != KErrNone)
        {
        OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_23,
                  "Cant read from the RMessage %d", err);
        OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_24,
                  "<<< CUsbHostMsProxyDrive::SetInfo() err = %d", err);
        return err;
        }

    err = iUsbHostMsLun.Initialise(msg, 3, iUnitInfo.iLunID);
    if(err != KErrNone)
        {
        OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_25,
                  "Initialising logical unit failed %d", err);
        OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_26,
                  "<<< CUsbHostMsProxyDrive::SetInfo() err = %d", err);
        return err;
        }

    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_27,
              "<<< CUsbHostMsProxyDrive::SetInfo() err = %d", err);
    return err;
    }

TInt CUsbHostMsProxyDrive::Dismounted()
    {
    return KErrNone;
    }

TInt CUsbHostMsProxyDrive::Enlarge(TInt /*aLength*/)
    {
    return KErrNotSupported;
    }


TInt CUsbHostMsProxyDrive::ReduceSize(TInt /*aPos*/, TInt /*aLength*/)
    {
    return KErrNotSupported;
    }

#define GetIndex(msg, aAddress, aIndex)         \
    aIndex = msg.Ptr0() == aAddress ? 0 :               \
                msg.Ptr1() == aAddress ? 1 :            \
                    msg.Ptr1() == aAddress ? 2 :        \
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
    OstTraceExt4(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_300,
                 ">>> HOST Read Pos=0x%x %x LBA=0x%x %x",
                 I64HIGH(aPos), I64LOW(aPos), I64HIGH(aPos/KBlockSize), I64LOW(aPos/KBlockSize));
    OstTraceExt2(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_301,
                 ">>> Len 0x%x Offset 0x%x",
                 (TUint)aLength, (TUint)aOffset);

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
            OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_302,
                      "<<< HOST Read ret=%d", KErrArgument);
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
        OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_303,
                  "Descriptor length: %08x", ((TPtr8* )aTrg)->MaxLength());
        }
    else
        {
        RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
        truncate = aLength - (msg.GetDesMaxLength(index) - aOffset);
        OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_304,
                  "Descriptor length: %08x", msg.GetDesMaxLength(index));
        }

    OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_305,
              "Offset: %08x", aOffset);
    OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_306,
              "Truncate: 0x%lx", truncate);

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
            OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_307,
                      "<<< HOST Read ret=%d", r);
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
                OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_308,
                          "<<< HOST Read ret=%d", r);
                return r;
                }

            iBuf.SetLength(len);

            RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
            r = msg.Write(index, iBuf, aOffset);
            if (r != KErrNone)
                {
                OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_309,
                          "<<< HOST Read ret=%d", r);
                return r;
                }
            }

        aPos += len;
        aOffset += len;
        }

    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_310,
              "<<< HOST Read ret=%d", KErrNone);
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
    OstTraceExt5(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_40,
                 ">>> HOST Read Pos=0x%x %x LBA=0x%x %x 0x%x",
                 I64HIGH(aPos), I64LOW(aPos), I64HIGH(aPos/KBlockSize), I64LOW(aPos/KBlockSize), aLength);
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
    OstTraceExt4(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_50,
                 ">>> HOST Write Pos=0x%x %x LBA=0%x %x",
                 I64HIGH(aPos), I64LOW(aPos), I64HIGH(aPos/KBlockSize), I64LOW(aPos/KBlockSize));
    OstTraceExt2(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_501,
                 "Len=0x%x Offset=0x%x",
                 (TUint)aLength, (TUint)aOffset);

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
        OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_51,
                  "Descriptor length: %08x", ((TPtr8* )aSrc)->Length());
        }
    else
        {
        RMessage2 msg(*(RMessagePtr2 *) &aThreadHandle);
        truncate = aLength - (msg.GetDesLength(index) - aOffset);
        OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_52,
                  "Descriptor length: %08x", msg.GetDesLength(index));
        }

    OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_53,
              "Offset: %08x", aOffset);
    OstTrace1(TRACE_SHOSTMASSSTORAGE_PROXY, HOSTUSBMSPROXY_54,
              "Truncate: 0x%lx", truncate);

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
            OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_55,
                      "<<< HOST Write ret=%d", r);
            return r;
            }

        if (localMessage)
            {
            r = iUsbHostMsLun.Write(mediaPos, len, ((TPtr8*)aSrc)->MidTPtr(aOffset));

            if (r != KErrNone)
                {
                OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_56,
                          "<<< HOST Write ret=%d", r);
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
                OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_57,
                          "<<< HOST Write ret=%d", r);
                return r;
                }

            r = iUsbHostMsLun.Write(mediaPos, len, iBuf);
            if (r != KErrNone)
                {
                OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_58,
                          "<<< HOST Write ret=%d", r);
                return r;
                }
            }

        aPos += len;
        aOffset += len;
        }

    OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_59,
              "<<< HOST Write ret=%d", KErrNone);
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
    OstTraceExt5(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_60,
                 ">>> HOST Write Pos=0x%x %x LBA=0x%x %x Len=0x%x",
                 I64HIGH(aPos), I64LOW(aPos), I64HIGH(aPos/KBlockSize), I64LOW(aPos/KBlockSize), aSrc.Length());
    return iUsbHostMsLun.Write(iMsDataMemMap.GetDataPos(aPos), aSrc.Length(), aSrc);
    }


/**
Get the proxy drive's capabilities information.

@param anInfo A descriptor of the connected drives capabilities.

@return system wide error code
*/
TInt CUsbHostMsProxyDrive::Caps(TDes8& anInfo)
    {
    OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_70,
              ">>> HOST Caps");
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
            OstTraceExt2(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_710,
                         "<<< HOST Caps Block[num=0x%x size=0x%x]",
                         capsInfo.iNumberOfBlocks, capsInfo.iBlockLength);
            OstTraceExt2(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_711,
                         "Media size=0x%x %x", I64HIGH(caps().iSize), I64LOW(caps().iSize));
            OstTrace1(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_712,
                      "WP=0x%x", caps().iMediaAtt);
            }
        else if (capsInfo.iMediaType == EMediaCdRom)
            {
            // not formattable
            c.iMediaAtt &= ~KMediaAttFormattable;
            OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_72,
                      ">>> HOST Caps MediaType = EMediaCdRom");
            }
        else
            {
            // do nothing
            }
        }
    else if (KErrNotReady == r)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_73,
                  "<<< HOST Caps Media Not Present");
        c.iType = EMediaNotPresent;
        r = KErrNone;
        }
    else if (KErrGeneral == r)
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_74,
                  "<<< HOST Caps Unable to communicate with media");
        c.iType = EMediaUnknown;
        }

    else
        {
        OstTrace0(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_75,
                  "<<< HOST Caps Unknown Error");
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
    OstTraceExt5(TRACE_SHOSTMASSSTORAGE_HOST, HOSTUSBMSPROXY_80,
                 "HOST Erase Pos=0x%x %x LBA=0x%x %x 0x%x",
                 I64HIGH(aPos), I64LOW(aPos), I64HIGH(aPos/KBlockSize), I64LOW(aPos/KBlockSize), aLength);
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
    return Erase(aPos, aLength);
    }


/**
Format the connected drive.

@param anInfo Device specific format information.

@return system wide error code.
*/
TInt CUsbHostMsProxyDrive::Format(TFormatInfo& aInfo)
    {
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
    iUsbHostMsLun.NotifyChange(aChanged, *aStatus);

    if(*aStatus != KRequestPending)
        return KErrUnknown;

    return KErrNone;
    }

void CUsbHostMsProxyDrive::NotifyChangeCancel()
    {
    iUsbHostMsLun.NotifyChangeCancel();
    }

TInt CUsbHostMsProxyDrive::SetMountInfo(const TDesC8* /*aMountInfo*/,TInt /*aMountInfoThreadHandle=KCurrentThreadHandle*/)
    {
    return KErrNone;
    }

TInt CUsbHostMsProxyDrive::ForceRemount(TUint aFlags)
    {
    iUsbHostMsLun.ForceRemount(aFlags);
    return KErrNone;
    }

TInt CUsbHostMsProxyDrive::Unlock(TMediaPassword& /*aPassword*/, TBool /*aStorePassword*/)
    {
    return KErrNotSupported;
    }

TInt CUsbHostMsProxyDrive::Lock(TMediaPassword& /*aOldPassword*/, TMediaPassword& /*aNewPassword*/, TBool /*aStorePassword*/)
    {
    return KErrNotSupported;
    }

TInt CUsbHostMsProxyDrive::Clear(TMediaPassword& /*aPassword*/)
    {
    return KErrNotSupported;
    }

TInt CUsbHostMsProxyDrive::ErasePassword()
    {
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
